// Case 04 — Float point models
// Compiler: clang-cl (VS2026)
// Standard: ISO C++23

#include <iostream>
#include <DirectXMath.h>
#include "HalfFloat.h"

using namespace DirectX;

[[clang::noinline]]
//XMM0 <-- v
static void ByValue(XMVECTOR v)
{
    volatile float x = XMVectorGetX(v);
    (void)x;
}

[[clang::noinline]]
//RCX <-- &v
static void ByReference(const XMVECTOR& v)
{
    volatile float x = XMVectorGetX(v);
    (void)x;
}

[[clang::noinline]]
static void TestFloatSumDiff()
{
    float a = 3.14f;
    float b = a + 1.00f;

    std::cout << "is (3.14 + 1.00f) == 4.14f ?? " << (((a + 1.00f) == 4.14f) ? ">>true<<" : ">>false<<") << " but, 'b' is " << b << std::endl;
}

[[clang::noinline]]
static void TestHalfFloat()
{
    half h = 0_h;
    for (float f = 0.0f; f < 10.0f; f += 0.1f)
    {
        h = f;
        std::cout << f << " -> " << h << " error: " << (f - h) << std::endl;
    }

    //How to Save half?
    std::uint16_t wHalfRaw = h.Bits();
    std::cout << "Raw Last half value: " << wHalfRaw << std::endl;

    /*After saving it as a 16 - bit integer, you can simply push it to the GPU pipeline as a set of values.
    In DirectX, it's recommended to group all possible values into 32-bit values, resulting in the following pattern:
    
    For example: RGBA values
    std::uint16_t wRGB[4] = { r.Bits(), g.Bits(), b.Bits(), a.Bits() }; --> total size: 8 Bytes -> "dual int32", NON DUAL FLOAT!!

    Then in the shader this 8-byte array matches 1:1 with the following:

    half r;
    half g;
    half b;
    half a;

    */
}

/*
    The differences between using /fp:precise and /fp:fast are only visible in the context of
    intensive value accumulation (such as matrix multiplication or multiple vector scaling). 
    However, the results shown in this module's README.md file are obtained on a custom
    platform that implements a 3D model with over ~150,000 vertices multiplied by 16 bones
    (transformation matrices) per KeyFrame. Interpolations are then performed between
    KeyFrameNum and ClientScreenFrames to generate intermediate transformations and achieve
    movement at 30-60-120-144-240 FPS with the same 3D model.

    It is within this specific context that the difference between the two floating precision
    flags becomes very evident.
*/

int main()
{
    //Test 1: Check operations '=='
    TestFloatSumDiff(); //3.14f + 1.0f != 4.14f

    //Test 2: Check parameters
    XMVECTOR vVect = {};
    ByValue(vVect);//MOVAPS XMMWORD PTR [RSI], XMM0
    ByReference(vVect);//MOV RCX, ESI

    //Test 3: Half Float
    TestHalfFloat();

    return 0;
}