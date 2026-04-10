#pragma once
#include <Windows.h>

template<typename CallBack>
double BenchmarkQPC(CallBack&& Function)
{
    LARGE_INTEGER liFrequency;
    LARGE_INTEGER liStart;
    LARGE_INTEGER liEnd;

    QueryPerformanceFrequency(&liFrequency);
    QueryPerformanceCounter(&liStart);

    Function();

    QueryPerformanceCounter(&liEnd);

    const LONGLONG llElapsedCounts = liEnd.QuadPart - liStart.QuadPart;
    return  static_cast<double>(llElapsedCounts) * 1000.0 / static_cast<double>(liFrequency.QuadPart);
}