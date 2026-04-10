#include <iostream>
#include <vector>
#include "Benchmark.h"

constexpr size_t N = 5'000'000;
constexpr size_t BLOCK_SIZE = 64;
constexpr size_t SLAB_SIZE = 4096;

volatile int gnSink = 0;

//------------------------------------------------------------
// Test 1 — malloc/free
//------------------------------------------------------------
static void Test_malloc()
{
	for (size_t i = 0; i < N; i++)
	{
		char* pPtr = reinterpret_cast<char*>(malloc(BLOCK_SIZE));
		if (pPtr)
		{
			pPtr[0] = static_cast<char>(i);
			gnSink += pPtr[0];
			free(pPtr);
		}
	}
}

//------------------------------------------------------------
// Pool Allocator (simple)
//------------------------------------------------------------
struct Pool
{
	char* pMemory;
	size_t nOffset;
	size_t nCapacity;

	Pool(size_t nSize)
	{
		this->pMemory = reinterpret_cast<char*>(HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, nSize));
		this->nOffset = 0;
		this->nCapacity = nSize;
	}

	~Pool()
	{
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, this->pMemory);
		this->pMemory = nullptr;
	}

	void* alloc(size_t size)
	{
		if (this->nOffset + size > this->nCapacity)
		{
			return nullptr;
		}

		void* pPtr = this->pMemory + this->nOffset;
		this->nOffset += size;
		return pPtr;
	}

	void reset()
	{
		this->nOffset = 0;
	}
};

//------------------------------------------------------------
// Test 2 — Pool allocator
//------------------------------------------------------------
static void Test_PoolAlloc()
{
	Pool hPool(N * BLOCK_SIZE);

	for (size_t i = 0; i < N; i++)
	{
		char* pPtr = reinterpret_cast<char*>(hPool.alloc(BLOCK_SIZE));
		if (pPtr)
		{
			pPtr[0] = static_cast<char>(i);
			gnSink += pPtr[0];
		}
	}

	hPool.reset();
}

//------------------------------------------------------------
// Slab Allocator
//------------------------------------------------------------
struct Slab
{
	char* pMemory;
	void* pFreeList;
	size_t nBlockSize;
	size_t nCapacity;

	Slab(size_t nBlockSizeIn)
	{
		this->nBlockSize = nBlockSizeIn;
		this->nCapacity = SLAB_SIZE / nBlockSizeIn;

		this->pMemory = reinterpret_cast<char*>(VirtualAlloc(nullptr, SLAB_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		this->pFreeList = nullptr;

		// build free list
		for (size_t i = 0; i < this->nCapacity; i++)
		{
			void* pPtr = this->pMemory + i * nBlockSizeIn;
			*reinterpret_cast<void**>(pPtr) = this->pFreeList;
			this->pFreeList = pPtr;
		}
	}

	~Slab()
	{
		VirtualFree(this->pMemory, 0, MEM_RELEASE);
		this->pMemory = nullptr;
	}

	void* alloc()
	{
		if (!this->pFreeList)
		{
			return nullptr;
		}

		void* pPtr = this->pFreeList;
		this->pFreeList = *reinterpret_cast<void**>(this->pFreeList);
		return pPtr;
	}

	void free_block(void* pPtr)
	{
		*reinterpret_cast<void**>(pPtr) = this->pFreeList;
		this->pFreeList = pPtr;
	}
};

//------------------------------------------------------------
// Test 3 — Slab allocator
//------------------------------------------------------------
static void Test_SlabAlloc()
{
	Slab hSlab(BLOCK_SIZE);

	std::vector<void*> vAllocated;
	vAllocated.reserve(hSlab.nCapacity);

	// allocate all
	for (size_t i = 0; i < hSlab.nCapacity; i++)
	{
		void* pPtr = hSlab.alloc();
		if (pPtr)
		{
			(reinterpret_cast<char*>(pPtr))[0] = static_cast<char>(i);
			gnSink += (reinterpret_cast<char*>(pPtr))[0];
			vAllocated.push_back(pPtr);
		}
	}

	// free all
	for (void* pPtr : vAllocated)
	{
		hSlab.free_block(pPtr);
	}
}

//------------------------------------------------------------
// MAIN
//------------------------------------------------------------
int main()
{
	std::cout << "Memory Case Benchmarks\n\n";

	std::cout << "malloc/free: "    << BenchmarkQPC(Test_malloc)    << " ms" << std::endl;
	std::cout << "Pool Allocator: " << BenchmarkQPC(Test_PoolAlloc) << " ms" << std::endl;
	std::cout << "Slab Allocator: " << BenchmarkQPC(Test_SlabAlloc) << " ms" << std::endl;

	system("pause");
	return 0;
}