// case01_return_value_registers
// Win64 ABI return value analysis
// Compiler: clang-cl (VS2026)
// Standard: ISO C++23

#include <cstdint>
#include <assert.h>

namespace experiment
{
    struct SmallAggregate
    {
        std::uint32_t a;
    };

    struct MediumAggregate
    {
        std::uint64_t a;
        std::uint64_t b;
    };

    struct LargeAggregate
    {
        std::uint64_t a;
        std::uint64_t b;
        std::uint64_t c;
    };

    //Return in EAX
    [[nodiscard]]
    [[clang::noinline]] 
    int ReturnInt(int nValue) noexcept
    {
        return nValue - 1;
    }

    //Return in RAX
    [[nodiscard]] 
    [[clang::noinline]]
    std::uint64_t ReturnU64(std::uint64_t qwValue) noexcept
    {
        return 0xADDDADDDADDDADDDULL + qwValue;
    }

    //Force non-constant FP value to ensure XMM0 return
    [[nodiscard]] 
    [[clang::noinline]]
    double ReturnDouble(double dValue) noexcept
    {
        return 3.141592653589793 * dValue;
    }

    //Return in AL but require zero-extension into RAX
    [[nodiscard]]
    [[clang::noinline]]
    bool ReturnBool(bool bValue) noexcept
    {
        return bValue && true;
    }

    //Return EAX
    [[nodiscard]]
    [[clang::noinline]]
    SmallAggregate ReturnSmall(std::uint32_t dwSmallValue) noexcept
    {
        return { dwSmallValue * 2 };
    }

    //Return RAX - RDX
    [[nodiscard]]
    [[clang::noinline]]
    MediumAggregate ReturnMedium(std::uint64_t qwMediumValue1, std::uint64_t qwMediumValue2) noexcept
    {
        return { qwMediumValue1 * 2, qwMediumValue2 * 3 };
    }

    //Return RCX (data) and RAX
    [[nodiscard]]
    [[clang::noinline]]
    LargeAggregate ReturnLarge(std::uint64_t qwLargeValue1, std::uint64_t qwLargeValue2, std::uint64_t qwLargeValue3) noexcept
    {
        return { qwLargeValue1 * 3, qwLargeValue2 * 4, qwLargeValue3 * 5 };
    }
}

int main()
{
    using namespace experiment;

    //Dummy var to simulate noinline
    std::uint64_t qwCount = ReturnU64(1ULL);

    volatile auto a = ReturnInt(static_cast<int>(++qwCount));
    volatile auto b = ReturnU64(++qwCount);
    volatile auto c = ReturnDouble(static_cast<double>(++qwCount));
    volatile auto d = ReturnBool(qwCount++ > 9ULL);
    volatile auto e = ReturnSmall(static_cast<std::uint32_t>(++qwCount));
    volatile auto f = ReturnMedium(qwCount + 1ULL, qwCount + 2ULL);
    volatile auto g = ReturnLarge(qwCount + 3ULL, qwCount + 4ULL, qwCount + 5ULL);

    //The invisible parameter in RCX is forced for observation
    assert(g.a > 0);
    assert(g.b > 0);
    assert(g.c > 0);

    //Force the function to run even though the results are not used.
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)e;
    (void)f;

    return 0;
}
