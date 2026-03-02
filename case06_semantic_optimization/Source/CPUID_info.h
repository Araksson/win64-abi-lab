#pragma once
#include <intrin.h>

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