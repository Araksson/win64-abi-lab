# Case 11 - Memory Management: General Allocators vs Custom Strategies

## Table of Contents

- [Objective](#objective)
- [Background](#background)
- [General-Purpose Allocators](#user-content-gen-prop-all)
- [Fragmentation](#fragmentation)
- [Alignment](#alignment)
- [Allocation Patterns Matter](#user-content-all-patt-matt)
- [Custom Memory Strategies](#user-content-custom-mem-str)
- [Practical Patterns](#user-content-pract-patt)
- [Multithreading Considerations](#user-content-multi-cons)

---

## Objective

To explore how memory allocation strategies affect performance and scalability.

This case focuses on:

- General-purpose allocators (`malloc`, `HeapAlloc`, `VirtualAlloc`).
- Fragmentation and allocation overhead.
- Memory alignment.
- Custom allocation strategies (pooling, slabs).

> Basically a brief description will be given of why it is more efficient for RAM and CPU to use certain specific initialization techniques, instead of simply using brute force with allocs and frees without special containers.

---

## Background

Memory allocation is often treated as a low-cost operation.

However, frequent allocations and deallocations can introduce:

- Fragmentation.
- Cache inefficiency.
- Synchronization overhead (in multithreaded allocators).

> When using hundreds or thousands of 'alloc' and 'free' in production (without container), they can generate inevitable fragmentation that at first "is insignificant" but later one will see that there is a progressive accumulation of memory that "seems like it shouldn't be there."
> This case seeks to explain why in these cases apparently inexplicable accumulations of RAM begin to occur and even the program slows down progressively.

---

## General-Purpose Allocators  <a id="user-content-gen-prop-all"></a>

### Common APIs

- `malloc / free`
- `HeapAlloc / HeapFree`
- `VirtualAlloc / VirtualFree`

### Characteristics

### `malloc` / `HeapAlloc`

- Flexible.
- Handle variable sizes.
- May introduce fragmentation.
- Often involve internal locks.

> These functions are containers of the following one (`VirtualAlloc`).

> Both functions do not guarantee exact memory alignment.

> `malloc` or `HeapAlloc` do not call the structure constructor by default (if used in custom constructors), so it is recommended to use the following pattern to avoid leaks or unexpected behavior:

```cpp

//... = Some Parameters
MyStruct* NewMyStructPtr(...)
{
    MyStruct* pPtr = reinterpret_cast<MyStruct*>(malloc(sizeof(MyStruct)));
    new(pPtr) MyStruct(...);
    return pPtr;
}
```

> Don't forget to call `free` or `HeapFree` as appropriate. Even so, be careful that the freed pointer is nullified (`pPtr = nullptr`) after its release, because otherwise there could be an invalid memory leak (such as accessing fields of a later structure in that same position) or access invalidations (in case it is on the edge of a program memory region).

> A discovered (UNDOCUMENTED) characteristic of these functions when you have exaggerated amounts of RAM (> 16GB) is the little "reuse" of pointers after freeing them, hence the fragmentation of these functions is considerable compared to custom containers.

> In many cases I noticed that the memory grew incessantly despite having freed (or HeapFree, as the case may be) at the appropriate time. But for cases > 1-10MB the release is certain (that is, it is observed reflected in debugging tools that effectively "X MB of system RAM memory was released"), which does not happen for small memories.

> Now, for cases with amounts less than 4GB of RAM or 32 bits (x86), the consumption in `malloc`/`free` or `HeapAlloc`/`HeapFree` is much more consistent in that sense since the system "understands that there are important limitations" and then forces itself to release more.

```text
In x86 the less RAM you have, the more efficient it is in USE.
In x64 seeks to improve performance and reuse is restricted a little more.
```

> In my experiments in very restricted memory environments (to verify RAM tuning limits in very controlled environments), < 1GB performs even better at managing available ram "gaps" and compaction at specific system times.

> In a single case, the issue of accumulation and fragmentation that occurred to me could not be resolved when I tried to allocate hundreds and thousands of packets with dynamic memory for use in a multiThread environment (in NetCore for Client and Server connections), and here, since the packets were of variable size, it was practically impossible to "unify blocks" and the system collapsed. Then I solved it by unifying the packet size and creating pointer recyclers to avoid overhead (and reduced fragmentation, on this system, to 100%).

> This may lead to an erroneous conclusion that "more memory is synonymous with more unbridled consumption", but the reality is that this is done by prioritizing performance over the MB used.

> This behavior (on x64 systems > 4GB) can be perfectly fixed using custom containers that correctly handle small memory blocks, and that releases in the program's total allocation limit are directly cleaned in the background, it is possible to replicate the same behavior as on x86 systems with ridiculously small amounts of RAM today. Basically, it is possible to make memory management with dynamic memory control according to capacity work exactly the same for all systems (taking as a reference the best possible case that would be similar to thinking "how to manage RAM on x32 systems with 128MB of RAM and Windows 98" but in modern environments).

```cpp
//windows 98 - 128Mb RAM
void* pPtr = malloc(1*1024*1024);//1MB asign

//memory used: > 1MB -> remain ~127MB (Obviously rounding off, let's ignore the fact that Windows takes up some RAM, it's a scheme)

...

//end of program
free(pPtr);//RAM usage is reflected directly without waiting
pPtr = NULL;//Let's respect the fact that in w98 C++ as we know it did not exist

//Memory used: 0MB -> remain ~128MB

//Programming in Windows 98 (and in previous versions as well, obviously, but this
//is an example) sought to make the most of resources without wasting space, which is
//why here before reallocating new memory, previous blocks were joined together to 
//form a large one and they would never "think" about continuing to allocate at their
//discretion because "there was plenty", quite the opposite.
```

```cpp
//windows 11 - 128GB Ram
void* pPtr = malloc(1*1024*1024);//1MB

//memory used: > 1MB ->remain 127.999GB

...

//end of program
free(pPtr);
pPrt = nullptr;//Here we will use nullptr because it is more current

//memory used: > 1MB --> remain 127.999GB

//At this point, the program "never releases that used MB of RAM", it simply leaves it to its fate.

/*If later here I want to allocate 1MB and 1 Byte of additional RAM, obviously 
it does not fit into that 1MB and there is a gap, but since there is a ton of RAM
later on, it allocates more memory directly and does not return to itself (in the 
Heap or in the block assigned for the program) until it reaches the "optimal" 
memory value according to the OS directly and only then begins to reuse freed 
pointers locations.*/

```

> **And the latter finally leads to a partial conclusion of:
In modern times, RAM usage is equally if not more important than overall optimization of how you allocate it. Taking the aforementioned case as a reference, it is insisted that the optimization of RAM memory USE should be taken into account.**


### VirtualAlloc

- Page-based allocation (typically 4 KB).
- No fragmentation inside the allocation.
- Expensive for small allocations.

> General allocators are optimized for flexibility, not for specific workloads.

> In this particular case, things change a lot with respect to malloc or HeapAlloc. Here the allocated memory must first be RESERVED (with the MEM_RESERVE flag), you must say if that memory is READ, WRITE, EXECUTE, etc. It is generally general purpose (no code execution), so the PAGE_READWRITE flag is used and the "first pointer" is assigned.

```cpp
//You can allocate a memory with the 'PAGE_NOACCESS' flag so that more specific exceptions appear
void* pPtr = VirtualAlloc(NULL, 1 * 1024 * 1024, MEM_RESERVE, PAGE_NOACCESS);
if(!pPtr)
    //Error managment
```

> This reserved "first pointer" must be committed to actually be used. If you try to access UNCOMMITTED memory you will get an unauthorized access violation.

```cpp

/*
    VirtualAlloc requires values that are multiples of the page size, generally
    4096Bytes, which is why it is done this way. If multiples of the page size
    are not used, the system rounds to the nearest value, since it only aligns to
    these values. Consider getting used to aligning memory values to multiples of
    this value.
*/

void* pCommitedPtr = VirtualAlloc(pPtr, 8 * 1024, MEM_COMMIT, PAGE_READWRITE);
if(!pCommitedPtr)
    //Error managment

```

> Also if you want to allocate a "direct memory" without much processing, you can use MEM_RESERVE | MEM_COMMIT and the memory is ready for use.

> Then, committed internal memories must be allocated in order to be used. At first glance it seems super complex, in part it is, but that is because it is the lowest level function that we can access as Windows programmers before going to the direct ASM with the internal Syscall that allows its initialization.

> The important thing to explain how `VirtualAlloc` works is because in a custom memory initializer, you will probably want to use this method instead of the traditional method with `malloc` or `HeapAlloc` (lower level) since it will offer absolute control over the memory ranges of the program. You can even read about an even more low-level version, [NtAllocateVirtualMemory](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntallocatevirtualmemory), much more difficult to understand, but more efficient if you use it with extreme caution.

---

## Fragmentation

Fragmentation occurs when memory is allocated and freed in non-uniform patterns.

Example:

```text
[ A ][ B ][ C ][ D ]
free(B), free(C)
```

Results in:

```text
[ A ][   ][   ][ D ]
```

Even if enough total memory exists, large allocations may fail.

> Fragmentation has been discussed a bit previously, but it basically consists of leaving blocks of memory without apparent access, unidentified and without the possibility of reuse in the middle of the RAM allocation block.

> This is extremely common on heavy `malloc`/`free` systems, being reduced a bit with `HeapAlloc`/`HeapFree`.

> The easiest way to reduce fragmentation in these scenarios is to manually handle a control of each particular case where intensive memory allocations and releases are carried out, it is the easiest way to do it since it does not require implementing very complex structures. You can simply make a small checklist where when cache pointers are discarded instead of doing free (or HeapFree) you send it to a pDequeuedList and then when you want to reassign another pointer like that, you can simply do: pNewPtr = pDequeuedList; and pDequeuedList = pDequeuedList->pNext; (with the appropriate controls in each case, obviously) and thus minimize the use of malloc/HeapAlloc, and then the memory would be intact. However, it is mandatory that the elements in the list be of exactly the same size to avoid problems later.

```cpp
struct MyStructControl
{
    MyStructControl* pNext;
    BYTE pbData[8];//The '8' is initial alignment, it is actually to indicate "the specific memory of the pointer starts here"
}

static MyStructControl* pDequeuedList = nullptr;

/*
    Let's consider the simplification that we only have pNext, but the reality is
    that we can have whatever we want in the Header of the management pointer. It 
    seems completely absurd to do it like this, but it should be considered that each 
    special control variable must be placed before the "array of pointer things"
*/
void* NewMyStruct()
{
    MyStructControl* pNew;
    if(pDequeuedList)
    {
        pNew = pDequeuedList;
        pDequeuedList = pDequeuedList->pNext;
    }
    else
    {
        pNew = malloc(sizeof(MyStructControl));
    }

    memset(pNew, 0x0, sizeof(MyStructControl));
    //asign custom things
    return pNew->pbData;//Here is the "writeable/readable" memory
}

void FreeMyStruct(void* pPtr)
{
    if(!pPtr)
    {
        return;   
    }
    
    //I 'Free' from the pNext that is in '- sizeof(void*)' from the pointer that I have
    MyStructControl* pMyStructPointer = reinterpret_cast<MyStructControl*>(reinterpret_cast<BYTE*>(pPtr) - sizeof(void*));

    /*
        Simple swap, the entire Dequeued list is placed in the pNext, and the
        global variable is updated with the recent "free". Hence the importance
        of having a pointer of a single size (although the content can
        obviously vary), otherwise you will have to make a special
        pDequeuedList for each pointer of each size to be released.
    */
    pMyStructPointer->pNext = pDequeuedList;
    pDequeuedList = pMyStructPointer;
}

/*
    At the end of the program or when you want to refresh the used memory (after
    intensive activities), you can do this and force pDequeued to be cleaned
*/
void GlobalFreeDequeued()
{
    if(!pDequeuedList)
    {
        return;
    }

    MyStructControl* pNext = nullptr;
    for(MyStructControl* pCurr = pDequeuedList; pCurr; pCurr = pNext)
    {
        pNext = pCurr->pNext;
        free(pCurr);
    }

    pDequeuedList = nullptr;
}
```

The best way to do it, and the most efficient globally in terms of RAM consumption, is to implement a MemoryManagment with low overhead that simply groups initialized memories that are internally assigned to their own pDequeuedList (for this, a subStructure so that it can be managed in a general way is enough, it is not difficult) and reusing them internally. This would be a container for the aforementioned and, as it is encapsulated internally, there is no risk of "making a mistake" in each specific case or of manipulating pointers where alignment could be a problem. In this case I recommend using VirtualAlloc (or its NT version), assign a large block as RESERVED, then within the reserved block assign small blocks of 4096Bytes subdivided into slabs of known sizes, to return these slabs instead of reassigning a complete block just for a small memory. Let's remember that the minimum size of VirtualAlloc is 4096Bytes (one page), so you have to manage those small blocks consciously.

---

## Alignment

Modern CPUs operate more efficiently with aligned memory.

Typical alignments:

- 4 Bytes (32-bit Pointer - int32_t).
- 8 Bytes (64-bit Pointer - int64_t).
- 16 Bytes (SIMD - _m128i).
- 32 Bytes (AVX - _m256i).
- 64 Bytes (Cache Line - AVX 512 - _m512i).
- 4096 Bytes (Page Size).

### Benefits of Alignment

- Fewer cache line splits.
- Better SIMD performance.
- Reduced false sharing.

> This was already explained in a previous [case](../case03_structure_padding/). However, the purpose of this section is to remind you that alignment is extremely important in this case if maximum efficiency is the goal.

### Page Alignment

Large allocations are aligned to page boundaries:

```cpp
/*
    Don't forget to precalculate the paging size to manually truncate the memory sizes
    you want to allocate, so you don't generate internal overhead
*/

#include <windows.h>

DWORD GetPageSize()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;//Generally 4096Bytes
}
```

This is important for:

- VirtualAlloc.
- Memory mapping.
- Large buffers.

> Page size allows the system to initialize memory in specific blocks without much internal overhead. In fact, the system should be "helped" to optimize its memory consumption; this is why there's a strong emphasis on dividing fixed-size pages into "smaller pages" (virtual slabs), thus making better use of space. Initializing many small memory blocks with `VirtualAlloc` and `MEM_RESERVE | MEM_COMMIT` will waste the extra space between `nPageSize` and `nSizeOfMemory` (if `nSizeOfMemory < nPageSize`). This may not seem like much, but it can lead to excessive fragmentation.

---

## Allocation Patterns Matter  <a id="user-content-all-patt-matt"></a>

Performance depends heavily on how memory is used:

- Many small allocations -> fragmentation.
- Frequent alloc/free -> overhead.
- Scattered memory -> cache misses.

> This is emphasized to complement what has already been discussed, because many people do not understand that in high-performance systems, an alloc/free operation in small memories (`< 4096 Bytes`) generates approximately the same overhead as in large memories (`>= 4096 Bytes &&  < 1MB`) (and obviously much less than huge memories `> 32MB`), but the point is that the initialization of memories `< nPageSize` (which in many large projects are the majority of dynamic memories) can be optimized so that the overhead of creating and destroying them is practically 0 (internal subdivision of `nPageSize * nBlocks` and updating a `pNext` pointer for list reuse is practically trivial in modern CPUs/RAM, instead of asking the OS for an nPageSize for each tiny memory).

---

## Custom Memory Strategies  <a id="user-content-custom-mem-str"></a>

Instead of relying on general allocators, many systems use custom strategies.

### Pool Allocators

Pre-allocate a large block and reuse memory.

Advantages:

- Fast allocation.
- Predictable behavior.
- Reduced fragmentation.

> This is purely optional, and in my experience, it's absolutely worth taking the time to study it, as it can improve project performance by approximately 75-100% (depending on the dynamic memory initialization rate) and reduce RAM consumption by 30-50% if you were previously only using `malloc`/`free` or `HeapAlloc`/`HeapFree` (replacing them with `VirtualAlloc` containers + `pPrev`/`pNext` pointers to manage the position, order, deletion, and recycling of various memory locations). Implementing it with slabs (memory < 4096 bytes in `nPageSizes`, and memory >= 4096 bytes in individual recyclable blocks) can further improve CPU/RAM usage.

> **The greatest performance gains are achieved with slabs.**

> **The greatest reduction in fragmentation is achieved by reusing pointers with `pNext`/`pBack` lists.**

> Implementing both cases with VirtualAlloc can yield exaggerated performance gains that may allow one to expand the project indefinitely.

### Slab Allocators

Divide memory into fixed-size blocks.

Example:

- Page size: 4096 bytes.
- Object size: 64 bytes.

```text
4096 / 64 = 64 objects per slab
```

### Benefits of Slabs

- No fragmentation.
- Constant-time allocation.
- Cache-friendly layout.

> Slabs are ideal for managing small memories continuously; in fact, if vectors were initialized in this way, much higher performance could be obtained than if they were initialized "in random positions within the MEM_RESERVED block".

> A very interesting strategy for implementing optimal Memory Management is the use of multiple IF statements to control which type of memory you want to initialize. If you are sufficiently predictive in your usage, you can create an automatic initialization of an N-capacity vector of elements of size < nPageSize (forcibly aligned to 64 bytes including the initial header, to adapt it to the size of a cache line) linked to nPageSize * nPagesNeed. By doing this contiguously, aligned, and without breaking the coherence of the cache line, you could obtain ultra-efficient vectors (more efficient is impossible).

> With one case for vectors (aligning the entire memory block to 4096 bytes and the access fields to 64 bytes, intended for complex structures, not for vectors of simple numbers), one case for dynamic lists (aligned to 16-32 bytes for SIMD/AVX optimizations, or to 64 bytes to optimize multithreading environments), one case for general slabs (aligned to 4096 bytes per large block and to 16-32-64 bytes per small block), and one case for general-purpose memory (aligned to 4096 bytes), these four cases make it possible to implement a very robust and efficient dynamic memory allocation system for high-performance systems.

> It must be remembered that VirtualAlloc ALWAYS, when executed with MEM_RESERVE, returns a pointer aligned to 4096 bytes (or the current page size), so it is up to the programmer to generate memories aligned to other values from there.

---

## Practical Patterns  <a id="user-content-pract-patt"></a>

Common patterns in high-performance systems:

- Frame allocators (reset every frame).
- Object pools.
- Slab allocators.
- Arena allocators.

> These systems are examples of allocators based on the design of the project in question. There will always be the possibility that one might be better than another under certain scenarios; it's necessary to study them in detail to decide which one to choose.

> In my personal experience, the Slab Allocator is more efficient for the type of systems I'm used to designing.

> However, you can (and very possibly should) use multiple systems simultaneously integrated into a single Memory Management system.

---

## Multithreading Considerations  <a id="user-content-multi-cons"></a>

General allocators may:

- Use locks.
- Cause contention.

Custom allocators can:

- Use thread-local pools.
- Avoid synchronization.

> It is highly recommended to use custom synchronization mechanisms as needed in the system. Generally, it is advisable to design Memory Management with containment mechanisms as needed, using `thread_local` in special situations where its use is recommended (such as RAM consumption in each Thread, for example), using `SRWLock` or `CRITICAL_SECTION` in situations where containment is vital (Alloc, Realloc, Free, etc.) and avoiding its use in situations where it is not necessary (debug or tracking mode).

> If you want to design a simple system, synchronization is not necessary, but it cannot be used in multithreading environments without concurrency (unless you design a kind of "Custom Heap per Thread").