// Case 03 — Structure Layout, Padding, Alignment and ABI Semantics (Win64)
// Compiler: clang-cl (VS2026)
// Standard: ISO C++23

#include <type_traits>
#include <cstdint>
struct StructTest1
{
    char c1;        //0x0
    //Padding       //0x1
    int  i;         //0x4
    char c2;        //0x8
    //Padding       //0x9
};

static_assert(sizeof(StructTest1) == 0xC);

struct StructTest2
{
    char c1;        //0x0
    char c2;        //0x1
    //Padding       //0x2
    int  i;         //0x4
};

static_assert(sizeof(StructTest2) == 0x8);

struct StructBase
{
    int a;          //0x0
};

struct StructDerived : StructBase
{
    int b;          //0x4
};

static_assert(sizeof(StructDerived) == 0x8);
static_assert(sizeof(StructBase) == 0x4);

struct StructWithVirtual
{
    //If you have at least one virtual function, the VTable is already generated at 0x0
    virtual void f();       //0x0

    int a;                  //0x8
    //Padding               //0xC
};

static_assert(sizeof(StructWithVirtual) == 0x10);

struct StructStandardLayout
{
    int a;
    int b;
    int c;
    int d;
};

static_assert(std::is_standard_layout_v<StructStandardLayout>);

struct StructNonStandardLayout
{
    virtual void a();
    int b;
};

static_assert(!std::is_standard_layout_v<StructNonStandardLayout>);

#pragma pack(push, 1)
struct StructPacked
{
    char c;
    int  i;
};
#pragma pack(pop)

static_assert(sizeof(StructPacked) == 0x5);

struct alignas(16) StructAligned
{
    double d;
};

static_assert(alignof(StructAligned) == 0x10);

struct alignas(16) StructCorrect
{
    double a;
    std::uint64_t b;
    int c;
    int d;
    double e;
};

static_assert(sizeof(StructCorrect) == 0x20);
static_assert(alignof(StructCorrect) == 0x10);
static_assert(std::is_standard_layout_v<StructCorrect>);

int main()
{
    return 0;
}
