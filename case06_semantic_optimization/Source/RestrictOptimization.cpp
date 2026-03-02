#include <cstdint>
#include <cstddef>

static constexpr std::size_t N = 1'000'000;

alignas(64) static float buffer[N * 3];

[[clang::noinline]]
static void AddNoRestrict(float* pA, float* pB, float* pC, std::size_t nCapacity)
{
    for (std::size_t i = 0; i < nCapacity; ++i)
    {
        pC[i] = pA[i] + pB[i];
    }
}

[[clang::noinline]]
static void AddRestrict(float* __restrict pA, float* __restrict pB, float* __restrict pC, std::size_t nCapacity)
{
    for (std::size_t i = 0; i < nCapacity; ++i)
    {
        pC[i] = pA[i] + pB[i];
    }
}

void TestingNoRestrict()
{
    float* A = buffer;
    float* B = buffer + N;
    float* C = buffer + 2 * N;

    AddNoRestrict(A, B, C, N);
}

void TestingRestrict()
{
    float* A = buffer;
    float* B = buffer + N;
    float* C = buffer + 2 * N;

    AddRestrict(A, B, C, N);
}