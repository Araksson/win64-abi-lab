#include <cstdint>

static constexpr int MAX_ITERATIONS_BENCH = 1'000'000;

struct BigCounter
{
    std::uint64_t hPayload[16];

    BigCounter()
    {
        for (int i = 0; i < 16; ++i)
        {
            hPayload[i] = 0;
        }
    }

    //Prefix
    BigCounter& operator++()
    {
        ++hPayload[0];
        ++hPayload[1];
        ++hPayload[2];
        ++hPayload[3];
        ++hPayload[4];
        ++hPayload[5];
        ++hPayload[6];
        ++hPayload[7];
        ++hPayload[8];
        ++hPayload[9];
        ++hPayload[10];
        ++hPayload[11];
        ++hPayload[12];
        ++hPayload[13];
        ++hPayload[14];
        ++hPayload[15];
        return *this;
    }

    //Postfix
    BigCounter operator++(int)
    {
        BigCounter hTemp = *this;
        hPayload[0]++;
        hPayload[1]++;
        hPayload[2]++;
        hPayload[3]++;
        hPayload[4]++;
        hPayload[5]++;
        hPayload[6]++;
        hPayload[7]++;
        hPayload[8]++;
        hPayload[9]++;
        hPayload[10]++;
        hPayload[11]++;
        hPayload[12]++;
        hPayload[13]++;
        hPayload[14]++;
        hPayload[15]++;
        return hTemp;
    }
};

[[clang::noinline]]
static std::uint64_t TestPrefix(BigCounter& Counter, int nIterations)
{
    std::uint64_t nSum = 0;
    for (int i = 0; i < nIterations; ++i)
    {
        ++Counter;
        nSum += Counter.hPayload[i % 16];
    }

    return nSum;
}

[[clang::noinline]]
static std::uint64_t TestPostfix(BigCounter& Counter, int nIterations)
{
    std::uint64_t nSum = 0;
    for (int i = 0; i < nIterations; ++i)
    {
        Counter++;
        nSum += Counter.hPayload[i % 16];
    }

    return nSum;
}

void BenchPrefix()
{
    BigCounter hCounter = {};
    volatile auto Val = TestPrefix(hCounter, MAX_ITERATIONS_BENCH);
    (void)Val;
}

void BenchPostfix()
{
    BigCounter hCounter = {};
    volatile auto Val = TestPostfix(hCounter, MAX_ITERATIONS_BENCH);
    (void)Val;
}