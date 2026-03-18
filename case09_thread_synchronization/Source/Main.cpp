#include <iostream>
#include "Benchmark.h"
#include <thread>
#include <vector>

static constexpr int nTotalIterations = 1'000'000;
static constexpr int nTotalThreads = 8;

/*
    The Critical Section requires InitializeCriticalSection and DeleteCriticalSection.
    This must be done before the thread in question enters the main thread; that's why
    it was done in the main thread.
*/
static CRITICAL_SECTION hcsSync = {};
static int gnCSCounter = 0;

static void WorkerCS(int nIterations)
{
    for (int i = 0; i < nIterations; ++i)
    {
        /*
            CALL EnterCriticalSection
            INC DWORD PTR[gnCSCounter]
            CALL LeaveCriticalSection
        */

        EnterCriticalSection(&hcsSync);
        ++gnCSCounter;
        LeaveCriticalSection(&hcsSync);
    }
}

/*
    SRWLOCK does not require initialization or destruction, as long as in the first iteration
    it is initialized with {0} or SRWLOCK_INIT (which is basically {0} or = {})
*/
static SRWLOCK hsrwSync = SRWLOCK_INIT;
static int gnSRWCounter = 0;

static void WorkerSRW(int nIterations)
{
    for (int i = 0; i < nIterations; ++i)
    {
        /*
            CALL AcquireSRWLockExclusive
            INC DWORD PTR[gnSRWCounter]
            CALL ReleaseSRWLockExclusive
        */
        AcquireSRWLockExclusive(&hsrwSync);
        ++gnSRWCounter;
        ReleaseSRWLockExclusive(&hsrwSync);
    }
}

/*
    The atomic counter does not need special initialization;
    simply set it to = 0 (or initialize an int variable, in this case, as you wish).
*/
static std::atomic<int> gnAtomicCounter = 0;

static void WorkerAtomic(int nIterations)
{
    for (int i = 0; i < nIterations; ++i)
    {
        /*
            INC.LOCK DWORD PTR[gnAtomicCounter] ;++

            ;or

            ADD.LOCK DWORD PTR[gnAtomicCounter], Value ;+= Value
        */
        gnAtomicCounter.fetch_add(1, std::memory_order_relaxed);
    }
}

/*
    Here, this variable with thread_local is independent between threads, so it should be
    used for debugging purposes or specific states.
*/
static thread_local int nThreadLocalCounter = 0;

static void WorkerTLS(int nIterations)
{
    for (int i = 0; i < nIterations; ++i)
    {
        /*
            MOV RCX, QWORD PTR GS:[0x58]                    ;TLSPointer
            MOV EAX, DWORD PTR [RCX + _tls_index * 8 + 4]   ;The + 4 depends on the location of the variable in the TLSPointer
            INC EAX                                         ;++
        */
        ++nThreadLocalCounter;
    }
}

/*
    False Sharing case
*/

constexpr size_t CACHE_LINE_SIZE = 64;

/*
    Note that the structure has multiple std::atomic values on
    the same cache line. This generates false sharing.
*/
struct alignas(CACHE_LINE_SIZE) FalseSharingData 
{
    std::atomic<int> nValue[nTotalThreads];
};

/*
    Here, a 64-byte slot is generated for each std::atomic;
    there is no false sharing here, each cache line is separate from the others.
*/
struct alignas(CACHE_LINE_SIZE) NoSharingData
{
    std::atomic<int> nValue;
};

static void WorkerSharing(std::atomic<int>& aiValue, int nIterations)
{
    for (int i = 0; i < nIterations; ++i)
    {
        aiValue.fetch_add(1, std::memory_order_relaxed);
    }
}

/*
    Here, a vector of threads is generated. When "emplace_back" is executed, the thread is
    automatically launched. Then, "join" is used below to wait for the thread to finish its
    execution (when iterating over N threads, all are waited for), and only after all N
    threads have finished executing can the main thread continue.
*/
static void ThreadBenchmark(void(*fpFN)(int), int nIterations, int nThreads)
{
    std::vector<std::thread> vWorkers;
    for (int i = 0; i < nThreads; ++i)
    {
        vWorkers.emplace_back(fpFN, nIterations);
    }

    for (auto& Thread : vWorkers)
    {
        Thread.join();
    }
}

static void ThreadBenchmarkFalseSharing(void(*fpFN)(std::atomic<int>& aiValue, int), int nIterations, int nThreads)
{
    std::vector<std::thread> vWorkers;
    
    FalseSharingData* pData = reinterpret_cast<FalseSharingData*>(_aligned_malloc(sizeof(FalseSharingData), CACHE_LINE_SIZE));
    memset(pData, 0x0, sizeof(FalseSharingData));

    for (int i = 0; i < nThreads; ++i)
    {
        vWorkers.emplace_back(fpFN, std::ref(pData->nValue[i]), nIterations);
    }

    for (auto& Thread : vWorkers)
    {
        Thread.join();
    }

    _aligned_free(pData);
}

static void ThreadBenchmarkNoSharing(void(*fpFN)(std::atomic<int>& aiValue, int), int nIterations, int nThreads)
{
    std::vector<std::thread> vWorkers;

    NoSharingData* pNoSharing = reinterpret_cast<NoSharingData*>(_aligned_malloc(sizeof(NoSharingData) * static_cast<uint32_t>(nThreads), CACHE_LINE_SIZE));
    memset(pNoSharing, 0x0, sizeof(NoSharingData) * static_cast<uint32_t>(nThreads));

    for (int i = 0; i < nThreads; ++i)
    {
        vWorkers.emplace_back(fpFN, std::ref(pNoSharing[i].nValue), nIterations);
    }

    for (auto& Thread : vWorkers)
    {
        Thread.join();
    }

    _aligned_free(pNoSharing);
}

int main()
{
    InitializeCriticalSection(&hcsSync);

    auto BenchMarkCS = [] ()
    {
        ThreadBenchmark(WorkerCS, nTotalIterations, nTotalThreads);
    };

    auto BenchMarkSRW = [] ()
    {
        ThreadBenchmark(WorkerSRW, nTotalIterations, nTotalThreads);
    };

    auto BenchMarkAtomic = [] ()
    {
        ThreadBenchmark(WorkerAtomic, nTotalIterations, nTotalThreads);
    };

    auto BenchMarkTLS = [] ()
    {
        ThreadBenchmark(WorkerTLS, nTotalIterations, nTotalThreads);
    };

    auto BenchMarkFalseSharing = [] ()
    {
        ThreadBenchmarkFalseSharing(WorkerSharing, nTotalIterations, nTotalThreads);
    };

    auto BenchMarkNoSharing = [] ()
    {
        ThreadBenchmarkNoSharing(WorkerSharing, nTotalIterations, nTotalThreads);
    };

    std::cout << "Benchmark with " << nTotalThreads << " Threads and " << nTotalIterations << " Iterations per Thread\n";
    std::cout << "CRITICAL_SECTION: " << BenchmarkQPC(BenchMarkCS) << " ms\n";
    std::cout << "SRWLOCK: " << BenchmarkQPC(BenchMarkSRW) << " ms\n";
    std::cout << "Atomic: " << BenchmarkQPC(BenchMarkAtomic) << " ms\n";
    std::cout << "TLS: " << BenchmarkQPC(BenchMarkTLS) << " ms\n";
    std::cout << "FalseSharing: " << BenchmarkQPC(BenchMarkFalseSharing) << " ms\n";
    std::cout << "NoSharing: " << BenchmarkQPC(BenchMarkNoSharing) << " ms\n";

    DeleteCriticalSection(&hcsSync);
    system("pause");

    return 0;
}