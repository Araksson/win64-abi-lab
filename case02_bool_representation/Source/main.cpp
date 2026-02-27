// Case 02 — Booleans
// Boolean representation experiment
// Compiler: clang-cl (VS2026)
// Standard: ISO C++23

#include <Windows.h>
#include <cstdint>
#include <assert.h>

struct alignas(4) BlBool
{
    std::int32_t nValue;

    //Constructs
    constexpr BlBool() noexcept : nValue(0) {}
    constexpr BlBool(bool bInValue) noexcept : nValue(bInValue ? 1 : 0) {}
    constexpr BlBool(std::int32_t nInValue) noexcept : nValue((nInValue != 0) ? 1 : 0) {}

    //Explicit Assign
    constexpr BlBool& operator=(bool bVal) noexcept
    {
        nValue = bVal ? 1 : 0;
        return *this;
    }

    constexpr BlBool& operator=(std::int32_t nAssign) noexcept
    {
        nValue = (nAssign != 0) ? 1 : 0;
        return *this;
    }

    //Compare
    [[nodiscard]]
    constexpr bool operator==(const BlBool& Boolean) const noexcept
    {
        return nValue == Boolean.nValue;
    }

    [[nodiscard]]
    constexpr bool operator!=(const BlBool& Boolean) const noexcept
    {
        return nValue != Boolean.nValue;
    }

    //Not used Operators
    bool operator<(const BlBool&) const = delete;
    bool operator>(const BlBool&) const = delete;
    bool operator<=(const BlBool&) const = delete;
    bool operator>=(const BlBool&) const = delete;
};

static_assert(sizeof(BlBool) == sizeof(std::int32_t), "BlBool only has sizeof(std::int32_t)");

//Global variables to use in place of TRUE, FALSE, true and false for use in BlBool
constexpr BlBool BL_TRUE = { true };
constexpr BlBool BL_FALSE = { false };

[[clang::noinline]]
static BlBool DummyCalc(std::int32_t nValue)
{
    nValue -= 4;
    return nValue != 0;
    /*
    ASM Here:
        [...]
        setne al <<< Normal behavior of BlBool as return
        [...]
    */
}

std::int32_t main()
{
    static std::int32_t nCount = 0;

    ++nCount;
    volatile auto a = static_cast<bool>(nCount++ != 0);
    volatile auto b = static_cast<BOOL>(nCount++ != 0);
    volatile auto c = static_cast<BOOLEAN>(nCount++ != 0);

    BlBool d = DummyCalc(nCount);

    (void)a;
    (void)b;
    (void)c;

    //It will definitely fail in one of these 2 codes
    assert(d == BL_FALSE);
    assert(d == BL_TRUE);

    return 0;
}
