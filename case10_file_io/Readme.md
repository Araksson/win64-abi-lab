# Case 10 - File I/O: Buffered vs Memory-Mapped vs Access Patterns

## Table of Contents

- [Objective](#objective)
- [Background](#background)
- [Standard C I/O](#user-content-std-cio)
- [Win32 API](#user-content-win32-fileapi)
- [Memory-Mapped Files](#user-content-memory-mapped-files)
- [Sequential vs Random Access](#sequential-vs-random-access)
- [Kernel-Level Considerations](#user-content-kernel-level)
- [Synchronous vs Asynchronous I/O](#user-content-sync-async)

---

## Objective

To demonstrate how file I/O can become a major performance bottleneck depending on:

- API choice.
- Buffering strategy.
- Access pattern (sequential vs random).

This case compares:

- Standard C I/O (`fread` / `fwrite`).
- Win32 API (`CreateFile` / `ReadFile` / `WriteFile`).
- Memory-mapped files (`CreateFileMapping` / `MapViewOfFile`).

---

## Background

File I/O performance is often misunderstood.

Many developers assume:

```cmd
Disk access is slow -> use async -> problem solved
```

However:

```cmd
In many real-world systems, poor access patterns cost more than synchronous I/O itself.
```

---

## Standard C I/O (`fread` / `fwrite`)  <a id="user-content-std-cio"></a>

## Characteristics

- Buffered by the C runtime (CRT).
- Easy to use.
- Sequential access oriented.
- Limited control over OS-level behavior.

### Example

```cpp
FILE* f = fopen("file.bin", "rb");

char buffer[4096];
fread(buffer, 1, sizeof(buffer), f);

fclose(f);
```

### Limitations

- Less control over buffering.
- No direct control over OS flags.
- Random access requires manual `fseek`.
- Extra abstraction layer over system calls.

> The functions that handle `FILE*` files are quite useful for performing fast reads and writes without the need for special parameters or asynchronous access. In this case, all open, read, write, seek, and close operations are synchronous and straightforward.

> Consider using the safe versions for added security; these end with the suffix `_s` and return `errno_t`, where `== 0` is success and `!= 0` is failure. To enable them, `#define __STDC_WANT_LIB_EXT1__`, but their use depends on the compiler.

---

## Win32 File API (`CreateFile`, `ReadFile`, `WriteFile`)  <a id="user-content-win32-fileapi"></a>

### Characteristics

- Direct access to OS-level file handling.
- Fine control via flags.
- Supports both sequential and random access.
- Can bypass some CRT overhead.

### Example

```cpp
HANDLE hFile = CreateFileA("file.bin", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

char buffer[4096];
DWORD bytesRead;

ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, nullptr);

CloseHandle(hFile);
```

[Here](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea) you can read more information about this type of function, mainly `CreateFile`, which is the most important since it handles the entire flag system when initializing a `HANDLE`.

### Useful Flags

These flags influence OS behavior:

- `FILE_FLAG_SEQUENTIAL_SCAN`
- `FILE_FLAG_RANDOM_ACCESS`
- `FILE_FLAG_NO_BUFFERING` (advanced use)
- `FILE_FLAG_WRITE_THROUGH`

Example:

```cpp
CreateFileA(..., FILE_FLAG_SEQUENTIAL_SCAN, ...);
```

These hints help the OS optimize caching and read-ahead.

> Personally, I found these functions more useful than the previously mentioned case. However, they require more advanced knowledge when setting flags and parameters correctly. A misplaced flag or a corrupted parameter can cause very serious problems.

> However, these functions are very useful for performing non-sequential writes and are necessary for mapping files (if applicable).

---

## Memory-Mapped Files  <a id="user-content-memory-mapped-files"></a>

### Characteristics

- File is mapped directly into virtual memory
- OS handles paging automatically
- No explicit read/write calls
- Excellent for random access

### Example

```cpp
HANDLE hFile = CreateFileA(...);

HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);

char* data = (char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

// Direct memory access
char value = data[12345];

UnmapViewOfFile(data);
CloseHandle(hMap);
CloseHandle(hFile);
```

### Advantages

- Eliminates explicit I/O calls.
- Efficient for random access.
- OS manages caching and paging.

> This system is ideal for reading very large sections of files, as it doesn't load everything into memory but instead generates a `void*` pointer to virtual memory. The mapped memory is released like a "wave in the sea" —some pages are initialized, and unused pages are released.

> The memory located in the return pointer of `MapViewOfFile` is not the process's own memory but belongs to the OS, so it can use virtual (disk) memory or physical (RAM) memory.

### Trade-offs

- Page faults can occur.
- Less predictable latency.
- Requires careful lifetime management.

> It is not recommended for use with files smaller than 256KB. In my experience, using mapping for small files is counterproductive.

> Use mapping only when you want to read the file once and in read-only mode.

> With 16KB of RAM, you can handle files larger than 10GB, provided you read them sequentially and in a single thread. If you map the file using multithreading, you'll get multiple "waves" and therefore RAM usage will increase considerably (> 10MB).

---

## Sequential vs Random Access

### Sequential Access

```cmd
Read -> Read -> Read -> ...
```

- Highly optimized by OS.
- Benefits from read-ahead.
- Fewer system calls.

> The best way to optimize RAM, CPU, and I/O usage is by reading fixed-size chunks sequentially without any interruptions (for example, when reading music, ideally, it should be read sequentially to optimize usage. In the case of a WAV file >100MB, with 16KB of RAM you can perfectly read the entire file and play it if it is done solely and exclusively sequentially from the header).

> This previous example is relevant because in my custom music system, I specifically checked that detail where any PCM audio `>1MB` mapped with `MapViewOfFile` can be read perfectly without delay and without affecting the overall RAM/IO consumption of the other tasks, simply by reading from the mapping pointer sequentially without cuts or jumps.

### Random Access

```cmd
Seek -> Read -> Seek -> Read -> ...
```

- Breaks prefetching.
- Higher latency.
- More system overhead.

> In this case, depending on how it's done, it can be balanced or very inefficient. In my custom file management system, I manage compiled and compressed files (using specific compression/decompression algorithms), and this will inevitably be done randomly. So (broadly speaking), the mapping is balanced by simply accessing the offset of the large file, decompressing it (if necessary) to an external buffer, and then the mapping pointer is automatically released after a certain time (managed by the OS). This causes the "wave" of RAM usage to "calm down" and then "risen" again in another area, maintaining consistent RAM usage. Otherwise, too many `SysCalls` are made to move the file offset, read, temporary buffers of temporary buffers (yes, basically double temporary buffers), and it ends up being very difficult to keep the system balanced.

### Key Insight

```cmd
Sequential access can outperform more advanced APIs used with poor access patterns.
```

---

## Kernel-Level Considerations  <a id="user-content-kernel-level"></a>

Win32 APIs internally call kernel functions (`NtCreateFile`, `NtReadFile`, etc.).

Developers can influence behavior via flags, but:

- Direct use of NT APIs is rarely necessary.
- Proper flag usage is usually sufficient.

Example optimization:

```cpp
FILE_FLAG_SEQUENTIAL_SCAN
```

This can significantly improve performance for linear reads.

> If you want to get the most out of file manipulation tools, you could try using the Nt versions, which have extended features over the Win32 API. They are much more difficult to use, but can give you greater control, although they also require more skill to use correctly.

---

## Synchronous vs Asynchronous I/O  <a id="user-content-sync-async"></a>

Windows supports asynchronous I/O via OVERLAPPED structures.

However:

- Introduces complexity.
- Requires manual state tracking.
- Harder to debug.

### Practical Guidance

Before using asynchronous I/O:

- Optimize access patterns.
- Minimize system calls.
- Use appropriate buffering.
- Consider memory mapping.

### Important Note

```cmd
In many real-world systems, poor access patterns cost more than synchronous I/O itself.
```

> The use of `OVERLAPPED` is rarely really required; if you know how to properly optimize the use of I/O functions in the project in question, you won't need to use asynchronous functions, and what's more, in many cases it ends up being counterproductive because access to the file content is required immediately after the read call, so if you don't want to add unnecessary complexity you shouldn't use this.

### When to Use Each Approach

| Scenario                 | Recommended Approach    |
|--------------------------|--------------------------|
| Simple sequential read   | `fread` / `ReadFile`     |
| Controlled behavior      | Win32 API                |
| Large random access      | Memory Mapping           |
| Extreme scalability      | Consider async (advanced)|

### Observations

- API choice matters less than access pattern.
- OS caching is highly effective.
- Memory mapping can simplify logic and improve performance.
- Over-engineering I/O can reduce maintainability.

### Practical Takeaway

Efficient file I/O is not about using the most advanced API.

It is about:

- Choosing the right access pattern.
- Minimizing overhead.
- Leveraging the operating system effectively.

> By optimizing the general use of file Read and Write functions, you will see that it is not necessary to use asynchronous operations except in very specific cases where they are unavoidable.

> My personal recommendation is that you research and experiment with the use of all possible file reading and writing systems (to decide from your own experience which one suits you best, but you must use only one to homogenize your project, otherwise it will be difficult to maintain) and that you design optimized algorithms for the consistent use of file buffers, you should not skimp on optimization expenses in this section since it is the main bottleneck of practically all projects that require them.