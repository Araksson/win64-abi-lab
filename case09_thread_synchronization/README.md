# Case 9 - Thread Synchronization

## Table of Contents

- [Objective](#objective)
- [Background](#background)
- [CRITICAL_SECTION characteristics](#user-content-criticalsection)
- [SRWLock Characteristics](#srwlock-characteristics)
- [Raw Lock/Unlock Cost](#raw-lockunlock-cost)
- [Contention](#contention)
- [Lock Scope Design](#lock-scope-design)
- [RAII Safety Wrappers](#raii-safety-wrappers)
- [`thread_local` Storage](#user-content-threadlocal)
- [Atomic Operations](#atomic-operations)
- [Cache Line](#cache-line)
- [Practical Takeaway](#practical-takeaway)

---

## Objective

To explore synchronization mechanisms in Windows and understand the performance characteristics of two commonly used primitives:

- `CRITICAL_SECTION`.
- `SRWLOCK` (Slim Reader/Writer Lock).

This case demonstrates how synchronization overhead behaves in hot paths and how the size of the protected region can dramatically affect performance.

The goal is not only to compare primitives but also to illustrate proper synchronization design.

Analyze an alternative for specific cases: the use of `thread_local`.

And considerations on the use of `std::atomic` (from C++ 11 onwards) as a modern alternative to `Interlocked` functions for counters, and the use of `std::atomic` in the same cache line.

---

## Background

Multithreaded systems must protect shared resources to prevent race conditions.

Windows provides several synchronization primitives. Two of the most commonly used are:

### CRITICAL_SECTION  
A lightweight user-mode mutex optimized for intra-process synchronization.

### SRWLOCK  
A slim reader/writer lock that supports both shared (read) and exclusive (write) access.

**Both primitives operate mostly in user mode and only enter the kernel when contention occurs.**

### thread_local
The `thread_local` keyword was introduced in C++11.
Modern compilers (GCC 4.8+, Clang 3.3+, MSVC 2015+) support it on systems such as Linux, Windows, macOS, and Android.

It is used to declare variables with local storage per thread. This means that each thread has its own independent copy of the variable, and it is not shared with other threads.

### std::atomic
Type allows safe concurrent access to a variable without using explicit locks (C++11).

**But precautions should be taken when using it.**

---

## CRITICAL_SECTION characteristics <a id="user-content-criticalsection"></a>

CRITICAL_SECTION provides:

- Fast uncontended locking.
- Recursive locking support.
- Thread ownership tracking.

Typical usage:

```cpp
EnterCriticalSection(&cs);
++shared_resource;
LeaveCriticalSection(&cs);
```

Advantages:

- Extremely fast when uncontended.
- Simple semantics.
- Widely used in legacy Windows code.

Limitations:

- Exclusive access only.
- Slightly larger structure.
- Recursion support adds overhead.
- Requires the use of initialization and destruction functions to use (`InitializeCriticalSection` and `DeleteCriticalSection`)

> It is very useful for specific containment and debug.

> If the use of reentrant is required (calling several functions with locks), `CRITICAL_SECTION` is ideal for that since in the same thread it is allowed to enter the critical section indefinitely without risk of producing a deadlock.

---

## SRWLock Characteristics

`SRWLOCK` provides a more modern design with two locking modes:

Shared mode (multiple readers)

Exclusive mode (single writer)

Typical usage:

```cpp
AcquireSRWLockExclusive(&lock);
++shared_resource;
ReleaseSRWLockExclusive(&lock);
```

Advantages:

- Smaller structure.
- No recursion tracking.
- Faster in many uncontended scenarios.
- Supports reader/writer patterns.
- No initialization or destruction required.

Limitations:

- No recursion support.
- Less flexible ownership semantics.

> In the case of `SWRLOCK`, it is designed to be small in size (pointer size, `4 Bytes` in x86 and `8 Bytes` in x64), and is used exclusively for short work without re-entry, since by not storing the ID of the thread that took it, when trying to do re-entry (anywhere in the system) an immediate deadlock occurs (even if it is the same thread that took it previously).

---

## Raw Lock/Unlock Cost

Measure the cost of entering and leaving a lock repeatedly.

Example:

```cpp
for (int i = 0; i < N; ++i)
{
    EnterCriticalSection(&cs);
    ++counter;
    LeaveCriticalSection(&cs);
}
```

Compare with:

```cpp
for (int i = 0; i < N; ++i)
{
    AcquireSRWLockExclusive(&lock);
    ++counter;
    ReleaseSRWLockExclusive(&lock);
}
```

> Both primitives are extremely fast when uncontended, but `SRWLOCK` may show slightly lower overhead.

> If they are used for simple variable increment or little pointer manipulation, both perform practically the same.

---

## Contention

Spawn multiple threads incrementing the same counter.

This highlights the cost of contention and scheduler interaction.

Important observation:

Under heavy contention the cost difference between primitives becomes much smaller compared to the cost of thread scheduling and waiting.

---

## Lock Scope Design

One of the most common performance mistakes is locking too much code.

Bad design:

```cpp
EnterCriticalSection(&cs);

for (auto& node : list)
{
    process(node);
}

LeaveCriticalSection(&cs);
```

Better design:

```cpp
EnterCriticalSection(&cs);
Node* first = list_head;//We get the start of the list
list_head = nullptr;//The list is nullptr to work cleanly from the outside without interfering with its content
LeaveCriticalSection(&cs);

process_nodes(first);
```

Reducing the critical region dramatically improves scalability.

> If you get used to making list designs similar to the example, you can reduce the TimeLock in the main thread and optimize all consecutive actions. You just have to be careful that the list that is going to be nulled in the Thread with contention is then correctly cleared from the memory of the nodes. For the latter, the use of a dequeued list is recommended to reuse memory and avoid memory-fragmentation in systems with high demand.

---

## RAII Safety Wrappers

To avoid synchronization mistakes, small RAII wrappers can guarantee proper lock release.

Example:

```cpp
class CriticalSectionSafe
{
public:
    CriticalSectionSafe(CRITICAL_SECTION* lpSync) : m_cs(lpSync)
    {
        EnterCriticalSection(m_cs);
    }

    ~CriticalSectionSafe()
    {
        LeaveCriticalSection(m_cs);
    }

private:
    CRITICAL_SECTION* m_cs;
};
```

Usage:

```cpp
{
    CriticalSectionSafe lock(&lSync);
    ++shared_counter;
}
```

A similar wrapper can be implemented for SRWLOCK.

```cpp
class LockNT
{
public:
    LockNT(SRWLOCK* hpLock) : m_Lock(hpLock)
    {
        AcquireSRWLockExclusive(m_Lock);
    }

    ~LockNT()
    {
        ReleaseSRWLockExclusive(m_Lock);
    }

private:
	SRWLOCK* m_Lock;
};

```

Usage:
```cpp
{
    LockNT Lock(&hLock);
    ++shared_counter;
}
```

> Note that in both cases a pointer is used because the structure that handles the Lock must be strictly external to the LockSafe system, and must be mutable.

```cpp
    CriticalSectionSafe(CRITICAL_SECTION* lpSync)...

    LockNT(SRWLOCK* hpLock)...
```

Furthermore, its existence is not checked for two main reasons:
- It must always point to a valid object (generally, a flat structure somewhere in the program is recommended).
- Better exception handling here (or debug): if for any reason it becomes `nullptr`, the system is poorly designed (and it's **a good time to fix it**).

If you want to use dynamic memory for initializing the structure (such as `CRITICAL_SECTION*`, `PCRITICAL_SECTION` or `LPCRITICAL_SECTION`, are equivalent; or `SRWLOCK*` or `PSRWLOCK`, also equivalents) or for the lock container (such as `ptr->lpSync`), you must check for the existence of the pointer (`lpSync` and/or `ptr`) before the control structure. Although the overhead of an additional `IF` statement is practically zero, for code cleanliness, it is recommended to keep the `Enter` and `Leave` statements of custom `LockSafe` systems as clean as possible.

> Using containment wrappers is extremely useful when designing large amounts of code where only a small portion requires containment, as it allows you to avoid mess the code and also avoid using lambdas, inline functions, or macros to perform this procedure. Simply encapsulating the code fragment with curly braces `{ ... }` and then declaring the variable "LockSafe" allows the compiler to resolve the constructor (`EnterCriticalSection` / `AcquireSRWLockExclusive`) and destructor (`LeaveCriticalSection` / `ReleaseSRWLockExclusive`) by opening and closing the braces, respectively.

---

## `thread_local` Storage <a id="user-content-threadlocal"></a>

Another technique to reduce synchronization overhead is the use of thread-local storage.

The `thread_local` keyword allows each thread to maintain its own independent instance of a variable.

Instead of protecting a shared variable with a lock, each thread writes to its own local copy.

Example:

```cpp
thread_local int local_counter = 0;

void worker()
{
    for (int i = 0; i < 1'000'000; i++)
    {
        ++local_counter;
    }
}
```

Each thread increments its own counter without any synchronization.

No locks are required because the variable is not shared.

### Aggregating Thread-Local Data

Often the `thread_local` values are combined later.

Example:

```cpp
thread_local int local_sum = 0;

void worker(int value)
{
    local_sum += value;
}
```

At the end of execution the values can be merged into a global result.

### Advantages

`thread_local` storage eliminates several common problems in concurrent systems:

- Lock contention.
- Cache line bouncing.
- Synchronization overhead.

It is particularly useful in:

- Counters.
- Statistics.
- Temporary buffers.
- Per-thread caches.

### Limitations

`thread_local` storage is not a universal replacement for synchronization.

It cannot be used when threads must:

- Coordinate shared state.
- Enforce ordering.
- Maintain strict global consistency.

Additionally, merging `thread_local` values into a global result still requires synchronization.

### Technical considerations

A thread_local in ASM looks like this:
```asm
MOV RAX, GS:[0x58]              ; Pointer to TLS array
MOV ECX, _tls_index             ; Module Index
MOV RBX, [RAX + RCX * 8]        ; RBX = pointer to the TLS block of the module
...
MOV EAX, [RBX + tls_offset]     ; tls_offset is fixed, calculated by the linker 
```

- All `thread_local` variables within the same module use the same `_tls_index`.
- Each variable has a fixed offset within the module's `.tls block`.
- The final access method is: `GS[0x58] + _tls_index*8` gives the block, and then the internal offset is added.

> This generates a small overhead at the beginning/end of each thread due to the initialization of variables in `GS[0x58]`.

- During thread initialization: The system must allocate memory for the TLS block of each module (`.tls`) and resolve offsets using `GS[0x58]` and `_tls_index`. This process is fast, but not zero.

- During thread termination: If the `thread_local` variables have destructors (which are not trivial), the runtime registers callbacks (via `__tlregdtor` or `__cxa_thread_atexit`) that are invoked when the thread terminates. This involves:
  - Iterating through a list of destructors.
  - Calling each destructor in order (with potential ordering issues in some implementations, such as MinGW-w64).
  - Releasing the memory of the `.tls` block.

This overhead is per thread, not per variable, and is generally small, but it can accumulate in applications with many threads or many TLS destructors.

> The overhead in reading is minimal due to direct memory access (`array access`)

If a thread is detached (`std::thread::detach`, for example), its termination can occur concurrently with the global program destruction phase. In that scenario, there is no order guarantee between the `thread_local` and `global destructors`, which can cause use-after-release if a `thread_local` destructor accesses a global variable that has already been destroyed.

**This problem is discussed in standard documents such as [N2880](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2009/n2880.html), which warns about this race condition.**

> Always ensure that threads terminate when instructed. To avoid these problems, consider using termination `events`.

> For simple variables (without memory requirements or complex destructors), `thread_local` has no problem.

---
## Atomic Operations

Another alternative to full synchronization primitives is the use of atomic operations.

The C++ `std::atomic` type allows safe concurrent access to a variable without using explicit locks.

Example:

```cpp
std::atomic<int> counter = 0;

void worker()
{
    for (int i = 0; i < N; ++i)
    {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
```

Atomic operations provide:

- Lock-free (or low-level) synchronization.
- Hardware-supported instructions (e.g., `lock xadd`).
- Well-defined memory ordering semantics.

### Advantages

Compared to locks:

- Lower overhead in uncontended scenarios.
- No need for explicit lock/unlock.
- Simpler usage for basic operations.

**It's important to consider the memory order in which specific `std::atomic` operations are performed:**

- `std::memory_order_relaxed`: It only guarantees atomicity and that a subsequent read from the same thread will not see a previous value. There is no synchronization or ordering with other operations.
- `std::memory_order_consume`: Do not use, obsolete as of C++26.
- `std::memory_order_acquire`: Applies to loads, ensures that all previous writes on another thread with a release are visible, used in consumers (e.g., concurrent queue reads).
- `std::memory_order_release`: Applies to storage, makes previous writes visible to threads that perform an acquire on the same variable, used in producers.
- `std::memory_order_acq_rel`: For read-modify-write operations (like `fetch_add`), combines `acquire` and `release`: acts as both in a single operation, ensures bidirectional visibility with other threads using `acquire`/`release`.
- `std::memory_order_seq_cst`: The most restrictive and predetermined, it combines `acq_rel` and establishes a global ordering of all `seq_cst` operations, all threads see the same operations in the same order, more expensive due to full memory barriers, but safer and more predictable.

> In most cases, using `std::memory_order_relaxed` is sufficient. When using basic operations like `++` or `--`, `std::memory_order_relaxed` is always used (but they should not be combined with allocations, only basic increment, otherwise, `std::memory_order_seq_cst` will be forced). However, it should only be used in general counters because it is less safe than using `std::memory_order_seq_cst` or `std::memory_order_acq_relaxed`.

### Limitations

Atomics are not a replacement for locks in all cases.

They are limited to:

- Simple operations (increment, exchange, compare-and-swap).
- No protection for complex data structures.

Additionally, atomics still cause:

- Cache line contention.
- Memory ordering constraints.

> It is not recommended to use it with uncommon structures or datatypes in C++, as atomicity is not guaranteed in these cases. It can be forced, and it might work, but that doesn't mean it will work correctly.

### `std::atomic` vs `thread_local`

While atomics remove the need for locks, they still operate on shared memory.

This means that multiple threads modifying the same atomic variable will still generate contention at the hardware level.

`thread_local` storage avoids this entirely by eliminating shared state.

- Use `std::atomic` for simple shared state updates.
- Use `thread_local` storage when possible to eliminate sharing.
- Use locks when complex data structures must be protected.

> `std::atomic` can be used without specific atomic methods, as long as they are with "atomizable" data types, for example in `int` (or any integer) you can use operations like `++` or `--` and its atomicity is guaranteed.

In ASM, an atomic operation is guaranteed as follows:

```asm
INC.LOCK ECX            ;++
;or
ADD.LOCK ECX, Value     ;+= Value
```

```cpp
std::atomic<int> nCounter = 0;
...
++nCounter
```

> If you use assignment when incrementing the value, this operation is also atomically assured.

> Internally, synchronization methods like `CRITICAL_SECTION` and `SRWLOCK` use `std::atomic` in its most primitive `C` version (such as `InterlockedIncrement`, `InterlockedExchange`, etc.). However, thanks to this, it is possible to invent an alternative synchronization method using `std::atomic` that matches (or improves upon) the quality of the two code synchronization methods discussed here, and is exactly suited to the code being created.

### `std::atomic` does not remove containment, it only removes the lock.

---

## Cache Line

A cache line is the smallest unit of memory that the CPU moves between:

- RAM <-> Cache
- Cache <-> other cores

In modern CPUs (x86), typically:

```cmd
Cache line = 64 bytes
```

Two variables are in the same cache line if they fall within the same 64-byte block in memory.

```cpp
struct Data
{
    int a;//4 bytes
    int b;//4 bytes
};
```

Layout:
```cmd
[ a ][ b ][ padding... ]   -> total within 64 bytes
```

Both variables (`a` and `b`) are in the same cache line.

### What problems does it cause?

Problem: **False Sharing**

Suppose:

- Thread 1 modifies `a`.
- Thread 2 modifies `b`.

Even though they don't share logic, this happens:

```cmd
Thread 1 modifies cache line -> invalidates in Thread 2
Thread 2 modifies cache line -> invalidates in Thread 1
Thread 1 modifies -> invalidates again
...
```

This is called:

### Cache Line Ping-Pong

And it generates:

- Constant traffic between Threads.
- Stalls.
- A brutal performance loss.

Example:

```cpp
struct Data
{
    std::atomic<int> a;
    std::atomic<int> b;
};
```

With 2 Threads:
```cpp
//Thread 1
a.fetch_add(1);

//Thread 2
b.fetch_add(1);
```

- Although they are different variables, they share the same cache line.
- Result: containment is the same as if they were the same variable.

### Solution

Separating the variables into different cache lines:

```cpp
struct alignas(64) PaddedInt
{
    std::atomic<int> value;
};
```

```cpp
struct Data
{
    PaddedInt a;
    PaddedInt b;
};
```

Now:

```cmd
[a.................] <- 64 bytes
[b.................] <- 64 bytes
```

False Sharing:
``` cmd
|--------- cache line ---------|
| a | b | padding............. |
```

No Sharing:
```cmd
|--------- cache line ---------|   |--------- cache line ---------|
| a | padding................. |   | b | padding................. |
```

> The CPU doesn't see variables, only 64-byte blocks.

- An `std::atomic` without sharing is practically free.
- An `std::atomic` with false sharing is extremely expensive.

In the example of Source/Main.cpp you will notice results similar to this:
```cmd
FalseSharing: 67 ms
NoSharing: 4 ms
```
> This shows that here, in this particular case, **architecture is what guides**.

---

## Practical Takeaway

Choosing the correct synchronization primitive is important, but correct lock design is even more critical.

In performance-sensitive systems:

- Minimize the size of critical regions.
- Avoid unnecessary contention.
- Use RAII wrappers to guarantee proper release.
- Measure performance under realistic workloads.
- Whenever possible, prefer `thread_local` storage over shared mutable state.
- Reducing shared memory is one of the most effective ways to improve scalability in multithreaded systems.
- For shared counters use `std::atomic`.

|       Method      |       Time        |
|-------------------|-------------------|
|   `thread_local`  |       fastest     |
|   `std::atomic`   |       medium      |
|   SRW / CS        |slowest (under contention)|

**If everything is done with a single thread, the execution time of these synchronization methods is exactly the same.**

**If you work with more than one thread (the purpose of these structures), you will get the following result in velocity: `thread_local` >> `std::atomic` >> locks.**

> When working with multiple variables and behaviors shared between threads using `std::atomic`, consider using `alignas(64)` for shared data to avoid `false sharing` issues. Although it wastes space, the performance difference is significant. If there is more than one consecutive shared atomic variable, performance can be considerably compromised if they share the same cache line.

> There are other LockSafe systems in `std::`, such as `std::unique_lock` and `std::lock_guard`, both use `std::mutex` as their synchronization method, but in my experience they were not clear enough and so far `SRWLOCK`, `CRITICAL_SECTION` and `thread_local` have solved 100% of the synchronization problems I work with.

If you'd like to learn more about using `std::mutex`, `std::unique_lock`, and `std::lock_guard`, you can check out the following [article](https://en.cppreference.com/w/cpp/thread/lock_guard.html), and this [article](https://en.cppreference.com/w/cpp/thread/unique_lock.html).

> `std::mutex` is cross-platform portable, but in Windows 7+ it is implemented at a low level through the internal use of `SRWLOCK`. RTL recommends using it over `CRITICAL_SECTION` and `SRWLOCK` in modern software (due to its portability and security, among other things), but with the LockSafe structures (I just presented) its use is practically the same as `CRITICAL_SECTION` or `SRWLOCK` and it has the same security on Windows.

> I want to clarify that using `CRITICAL_SECTION`, `SRWLOCK`, and `thread_local` has worked for me, and I understood them perfectly. This doesn't mean that using `std::mutex` is "*superior or inferior*" in my experience; on the contrary, it's possible that in certain scenarios, `std::mutex` might be more efficient and others that are less efficient.

### Last Example

Example With LockSafe struct:
```cpp
int Counter()
{
    CriticalSectionSafe Lock(&lSync);
    return ++shared_counter;
}
```

Example Without LockSafe struct:
```cpp
int Counter()
{
    int nSharedValue;

    EnterCriticalSection(&lSync);
    nSharedValue = ++shared_counter;
    LeaveCriticalSection(&lSync);

    return nSharedValue;
}
```

```asm
PUSH RCX
MOV RCX, QWORD PTR[lSync]
CALL EnterCriticalSection

INC DWORD PTR [shared_counter]
MOV EAX, DWORD PTR [shared_counter]

CALL LeaveCriticalSection
POP RCX

RET
```

> Some compilers will throw a warning if you don't declare the `nSharedValue` variable outside the critical section.

Both ASM codes are exactly the same, but in the first case it is safe and in the second case it is potentially dangerous (both for the variable declaration and for "forgetting" to do the Leave section)