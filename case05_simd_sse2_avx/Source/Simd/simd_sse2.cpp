#include <emmintrin.h>
#include "../VertexStruct.h"
#include "simd_dispatch.h"
#include <assert.h>

[[clang::noinline]]
void Transform_SSE2_AoS(AoSVertex* __restrict pIn, AoSVertex* __restrict pOut, size_t nCount, float fScale)
{
    //PERMILPS
    __m128 scale = _mm_set1_ps(fScale);

    //Aligned instructions
    for (size_t i = 0; i < nCount; ++i)
    {
        //MOVAPS
        __m128 v = _mm_load_ps(reinterpret_cast<float*>(pIn + i));

        //MULPS
        v = _mm_mul_ps(v, scale);

        //MOVAPS
        _mm_store_ps(reinterpret_cast<float*>(pOut + i), v);
    }
}

[[clang::noinline]]
void Transform_SSE2_SoA(SoAVertexs* __restrict pIn, SoAVertexs* __restrict pOut, size_t nCount, float fScale)
{
    assert((nCount % 4) == 0);

    //PERMILPS
    __m128 scale = _mm_set1_ps(fScale);

    float* pX = pIn->x.data();
    float* pY = pIn->y.data();
    float* pZ = pIn->z.data();

    float* pXo = pOut->x.data();
    float* pYo = pOut->y.data();
    float* pZo = pOut->z.data();

    //Unaligned instructions
    for (size_t i = 0; i + 4 <= nCount; i += 4)
    {
        //MOVUPS
        __m128 vx = _mm_loadu_ps(pX + i);
        __m128 vy = _mm_loadu_ps(pY + i);
        __m128 vz = _mm_loadu_ps(pZ + i);

        //MULPS
        vx = _mm_mul_ps(vx, scale);
        vy = _mm_mul_ps(vy, scale);
        vz = _mm_mul_ps(vz, scale);

        //MOVUPS
        _mm_storeu_ps(pXo + i, vx);
        _mm_storeu_ps(pYo + i, vy);
        _mm_storeu_ps(pZo + i, vz);
    }
}