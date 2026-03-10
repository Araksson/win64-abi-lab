#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include "Benchmark.h"

constexpr size_t N = 10'000'000;
constexpr int It = 1;

#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

#ifdef max
#undef max
#endif

//Branch version
/*
	mov eax, [vData]
	test eax, eax
	jle Skip
	add nSum, eax

	Skip:
*/
static NOINLINE int Branch_Sum(const std::vector<int>& vData)
{
	int nSum = 0;
	for (size_t i = 0; i < vData.size(); ++i)
	{
		int x = vData[i];
		if (x > 0)
		{
			nSum += x;
		}
	}

	return nSum;
}

//Branchless version
/*
	mov eax, [vData]
	test eax, eax
	setg dl
	imul eax, edx << with std::max switch to 'cmovg' instead of 'setg' + 'imul'
	add nSum, eax
*/
NOINLINE static int Branchless_Sum(const std::vector<int>& vData)
{
	int nSum = 0;
	for (size_t i = 0; i < vData.size(); ++i)
	{
		int x = vData[i];
		//nSum += (x > 0) * x;//Fast
		nSum += std::max(x, 0);//Low
	}

	return nSum;
}

//Dataset 1 - Perfectly predictable
static void DataSet_Predictable(std::vector<int>& vData)
{
	for (size_t i = 0; i < vData.size(); ++i)
	{
		vData[i] = static_cast<int>(i) + 1;//always positive
	}
}

//Dataset 2 - Pattern predictable
static void DataSet_Pattern(std::vector<int>& vData)
{
	for (size_t i = 0; i < vData.size(); ++i)
	{
		if (i % 2 == 0)
		{
			vData[i] = 1;
		}
		else
		{
			vData[i] = -1;
		}
	}
}

//Dataset 3 - Random
static void DataSet_Random(std::vector<int>& vData)
{
	std::mt19937 hRng(12345);
	std::uniform_int_distribution<int> hDist(-1, 1);

	for (size_t i = 0; i < vData.size(); ++i)
	{
		vData[i] = hDist(hRng);
	}
}

/*
	Depending on the CPU, the compiler, and the load (whether it's a hot path or not), the
	performance can differ greatly between the two benchmarks or work virtually the same.
	
	The vector wasn't optimized because visible values are required. Applying optimizations
	like using [pointer] instead of vData[] would yield much better results, but only at
	very small millisecond values.

	However, this must be combined with everything explained in case 6 of optimizations.
*/
int main()
{
	std::vector<int> vData(N);

	//Wrappers lambda
	auto CBBranchSum = [&] ()
	{
		for (int i = 0; i < It; ++i)
		{
			Branch_Sum(vData);
		}
	};

	auto CBBranchlessSum = [&] ()
	{ 
		for (int i = 0; i < It; ++i)
		{
			Branchless_Sum(vData);
		}
	};

	//Perfect prediction
	std::cout << "\nPredictable dataset\n";

	DataSet_Predictable(vData);

	std::cout << "Perfect prediction - Branch: " << BenchmarkQPC(CBBranchSum) << "ms\n";
	std::cout << "Perfect prediction - Branchless: " << BenchmarkQPC(CBBranchlessSum) << "ms\n";

	//Pattern prediction
	std::cout << "\nPattern dataset\n";

	DataSet_Pattern(vData);

	std::cout << "Pattern prediction - Branch: " << BenchmarkQPC(CBBranchSum) << "ms\n";
	std::cout << "Pattern prediction - Branchless: " << BenchmarkQPC(CBBranchlessSum) << "ms\n";

	//Random prediction
	std::cout << "\nRandom dataset\n";

	DataSet_Random(vData);

	std::cout << "Random prediction - Branch: " << BenchmarkQPC(CBBranchSum) << "ms\n";
	std::cout << "Random prediction - Branchless: " << BenchmarkQPC(CBBranchlessSum) << "ms\n";
}