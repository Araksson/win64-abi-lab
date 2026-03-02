#include <cstdint>

static constexpr int MAX_CAPACITY_TEST = 1 << 10;

struct alignas(64) BigStruct
{
    float data[16];
};

[[clang::noinline]]
static float ComputeByValue(BigStruct s)
{
    float sum = 0.f;
    for (int i = 0; i < 16; ++i)
    {
        sum += s.data[i] * 1.5f;
    }

    return sum;
}

[[clang::noinline]]
static float ComputeByConstRef(const BigStruct& s)
{
    float sum = 0.f;
    for (int i = 0; i < 16; ++i)
    {
        sum += s.data[i] * 1.5f;
    }

    return sum;
}

float ComputeParametersValue()
{
    float fResult = 0.0f;
    BigStruct hLocalStructList[MAX_CAPACITY_TEST];

    for (int i = 0; i < MAX_CAPACITY_TEST; ++i)
    {
        for (int j = 0; j < 16; ++j)
        {
            hLocalStructList[i].data[j] = static_cast<float>(i + (j * MAX_CAPACITY_TEST));
        }
    }

    for (int i = 0; i < MAX_CAPACITY_TEST; ++i)
    {
        fResult += ComputeByValue(hLocalStructList[i]);
    }

    return fResult;
}

float ComputeParametersRef()
{
    float fResult = 0.0f;
    BigStruct hLocalStructList[MAX_CAPACITY_TEST];

    for (int i = 0; i < MAX_CAPACITY_TEST; ++i)
    {
        for (int j = 0; j < 16; ++j)
        {
            hLocalStructList[i].data[j] = static_cast<float>(i + (j * MAX_CAPACITY_TEST));
        }
    }

    for (int i = 0; i < MAX_CAPACITY_TEST; ++i)
    {
        fResult += ComputeByConstRef(hLocalStructList[i]);
    }

    return fResult;
}