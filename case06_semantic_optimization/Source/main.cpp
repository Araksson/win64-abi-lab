#include <Windows.h>
#include <intrin.h>
#include <vector>
#include <iostream>
#include <assert.h>
#include "Exports.h"
#include "CPUID_info.h"
#include "Benchmark.h"

static constexpr int MaxBenchmarkSize = 1 << 16;
static constexpr int MaxIterations = 1 << 7;

static CpuCaps hCPUInfo = {};

template<typename Callable>
static double SIMDOptimizationBenchmark(Callable&& CallBack)
{
    std::vector<float> vDestInfo;
    vDestInfo.resize(MaxBenchmarkSize);

    std::vector<float> vInputA;
    std::vector<float> vInputB;
    std::vector<float> vInputC;

    vInputA.resize(MaxBenchmarkSize);
    vInputB.resize(MaxBenchmarkSize);
    vInputC.resize(MaxBenchmarkSize);

    auto Benchmark = [&] ()
    {
        for (int i = 0; i < MaxIterations; ++i)
        {
            CallBack(vDestInfo.data(), vInputA.data(), vInputB.data(), vInputC.data(), MaxBenchmarkSize);
        }
    };

    return BenchmarkQPC(Benchmark);
}

template<typename Callable>
static double BenchmarkCallBackSimulation(Callable&& CallBack)
{
    auto Benchmark = [&] ()
    {
        for (int i = 0; i < MaxIterations; ++i)
        {
            CallBack();
        }
    };

    return BenchmarkQPC(Benchmark);
}

int main()
{
    hCPUInfo = DetectCpuCaps();
    assert(hCPUInfo.sse2);

    /*
        It is shown that using clean lines can have slightly lower performance
        than using several consecutive lines (without iteration) due to proper
        vectorization.
    */
    std::cout << "Testing SIMD Clean vs SIMD Unrolled" << std::endl;
    double dSimdClean = SIMDOptimizationBenchmark(Compute_Clean);
    double dSimdUnrolled = SIMDOptimizationBenchmark(Compute_Unrolled);

    std::cout << "SIMD_Clean Time: " << dSimdClean << "ms" << std::endl;
    std::cout << "SIMD_Unrolled Time: " << dSimdUnrolled << "ms" << std::endl;

    /*
        The test is to simulate a ClearScreen as if using GDI, where each pixel
        is processed by the CPU. This demonstrates the difference between inline
        and non-inline hotpaths in the same situation.
    */
    system("pause");
    std::cout << "Testing Inline vs No Inline" << std::endl;

    double dNoInlineCleanScreen = BenchmarkCallBackSimulation(CleanScreenNoInline);
    double dInlineCleanScreen = BenchmarkCallBackSimulation(CleanScreenInline);

    std::cout << "No Inline clean screen Time: " << dNoInlineCleanScreen << "ms" << std::endl;
    std::cout << "Inline clean screen Time: " << dInlineCleanScreen << "ms" << std::endl;

    /*
        Here you can see how forcing a function with parameter passing by copy
        versus by reference from a structure that far exceeds the capacity of
        the registers can generate performance differences.

        In some cases, the compiler resolves large structures by value as a
        const reference or directly renders it inline despite being marked
        as noinline, and it may appear that performance is even superior to
        explicit reference. Although it is clear that it is bad practice and
        the different compilations modify the behavior in that case.
    */
    system("pause");
    std::cout << "Testing using Parameters by Value vs by Ref" << std::endl;

    double dParametersValue = BenchmarkCallBackSimulation(ComputeParametersValue);
    double dParametersRef = BenchmarkCallBackSimulation(ComputeParametersRef);

    std::cout << "Parameters by Value Time: " << dParametersValue << "ms" << std::endl;
    std::cout << "Parameters by Ref Time: " << dParametersRef << "ms" << std::endl;

    /*
        The __restrict case is interesting because if the compiler cannot prove
        that the pointers overlap or not, the __restrict matters. Now, if 3
        independent global variables are created and passed as parameters to
        functions with __restrict and without __restrict, the compiler detects
        that they are independent and handles both cases as __restrict, achieving
        identical results in the benchmark.
    */
    system("pause");
    std::cout << "Testing __restrict vs no __restrict" << std::endl;

    double dNoRestrict = BenchmarkCallBackSimulation(TestingNoRestrict);
    double dRestrict = BenchmarkCallBackSimulation(TestingRestrict);

    std::cout << "Using __restrict Time: " << dRestrict << "ms" << std::endl;
    std::cout << "Not using __restrict Time: " << dNoRestrict << "ms" << std::endl;

    /*
        In the prefix and postfix tests, it is observed how performance changes
        after performing both operations with the same number of iterations.
        Internally, a field-by-field increment (without loop) was added to
        simulate a variable structure.
        The differences in ++X vs X++ times are considerable.
    */
    system("pause");
    std::cout << "Testing Prefix vs Postfix" << std::endl;

    double dPrefix = BenchmarkCallBackSimulation(BenchPrefix);
    double dPostfix = BenchmarkCallBackSimulation(BenchPostfix);

    std::cout << "Using Prefix ++X Time: " << dPrefix << "ms" << std::endl;
    std::cout << "Using Postfix X++ Time: " << dPostfix << "ms" << std::endl;

    return 0;
}