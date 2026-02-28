#include <Windows.h>
#include <intrin.h>
#include <vector>
#include "VertexStruct.h"
#include "Simd/simd_dispatch.h"
#include <chrono>
#include <iostream>

//CPUID SSE2 and AVX
struct CpuCaps
{
    bool sse2;
    bool avx;
};

static CpuCaps DetectCpuCaps()
{
    int info[4]{};
    CpuCaps caps{};

    __cpuid(info, 1);

    caps.sse2 = (info[3] & (1 << 26)) != 0;
    caps.avx = (info[2] & (1 << 28)) != 0;

    return caps;
}

static CpuCaps hCPUInfo = {};
//

template<typename T, typename CallBack>
double BenchmarkQPC(CallBack&& Function, T* pIn, T* pOut, size_t nCount, float fScale, int nIterations)
{
    LARGE_INTEGER liFrequency;
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;

    QueryPerformanceFrequency(&liFrequency);

    //Warmup (avoids cold cache measurements)
    Function(pIn, pOut, nCount, fScale);

    QueryPerformanceCounter(&liStart);

    for (int i = 0; i < nIterations; ++i)
    {
        Function(pIn, pOut, nCount, fScale);
    }

    QueryPerformanceCounter(&liEnd);

    const LONGLONG llElapsedCounts = liEnd.QuadPart - liStart.QuadPart;

    const double dElapsedMilliseconds = static_cast<double>(llElapsedCounts) * 1000.0 / static_cast<double>(liFrequency.QuadPart);

    //Avoid dead optimization
    volatile float fSink = 0.0f;
    fSink += reinterpret_cast<float*>(pOut)[0];

    return dElapsedMilliseconds;
}

/*
    The SSE2 and AVX cases are designed as examples; in a real-world scenario,
    fallback instructions should be implemented after vectored calls for cases
    involving data sizes < 16 bytes and < 32 bytes (respectively).

    In a serious project, AVX should be configured to operate within certain data
    sizes (e.g., >8192–16384 bytes), and SSE2 should also be configured within
    certain data sizes (e.g., >256–1024 bytes). This is important because in the
    smallest data sizes (< 512 bytes for AVX and < 128 bytes for SSE2), the
    performance difference compared to scalar fallbacks of 4 or 8 bytes is
    practically negligible.
*/

int main()
{
    constexpr int nMaxVertex = 32 * 1024 * 1024;//% 8 == 0

    hCPUInfo = DetectCpuCaps();

    std::vector<AoSVertex> vAOS = {};
    SoAVertexs vSOA = {};

    vAOS.resize(nMaxVertex);

    vSOA.x.resize(nMaxVertex);
    vSOA.y.resize(nMaxVertex);
    vSOA.z.resize(nMaxVertex);

    std::vector<AoSVertex> vAOS_Save = {};
    SoAVertexs vSOA_Save = {};

    vAOS_Save.resize(nMaxVertex);
    vSOA_Save.x.resize(nMaxVertex);
    vSOA_Save.y.resize(nMaxVertex);
    vSOA_Save.z.resize(nMaxVertex);

    decltype(&Transform_Scalar_AoS) hSelectedAoS = nullptr;
    decltype(&Transform_Scalar_SoA) hSelectedSoa = nullptr;

    if (hCPUInfo.avx)
    {
        hSelectedAoS = Transform_AVX_AoS;
        hSelectedSoa = Transform_AVX_SoA;
    }
    else if (hCPUInfo.sse2)
    {
        hSelectedAoS = Transform_SSE2_AoS;
        hSelectedSoa = Transform_SSE2_SoA;
    }
    else
    {
        hSelectedAoS = Transform_Scalar_AoS;
        hSelectedSoa = Transform_Scalar_SoA;
    }

    double dTimeOfAOS = BenchmarkQPC(hSelectedAoS, vAOS.data(), vAOS_Save.data(), nMaxVertex, 2.34f, 1);
    double dTimeOfSOA = BenchmarkQPC(hSelectedSoa, &vSOA, &vSOA_Save, nMaxVertex, 2.34f, 1);

    std::cout << "Time of AoS: " << dTimeOfAOS << "ms" << std::endl;
    std::cout << "Time of SoA: " << dTimeOfSOA << "ms" << std::endl;
}