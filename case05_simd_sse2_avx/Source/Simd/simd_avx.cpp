#include <immintrin.h>
#include "../VertexStruct.h"
#include "simd_dispatch.h"
#include <assert.h>

[[clang::noinline]]
void Transform_AVX_AoS(AoSVertex* __restrict pIn, AoSVertex* __restrict pOut, size_t nCount, float fScale)
{
    assert((nCount % 2) == 0);

    //VPERMILPS+VINSERTF128
    __m256 scale = _mm256_set1_ps(fScale);

    //Aligned Instructions
    for (size_t i = 0; i + 2 <= nCount; i += 2)
    {
        //VMOVAPS
        __m256 v = _mm256_load_ps(reinterpret_cast<float*>(pIn + i));

        //VMULPS
        v = _mm256_mul_ps(v, scale);

        //VMOVAPS
        _mm256_store_ps(reinterpret_cast<float*>(pOut + i), v);
    }
}

[[clang::noinline]]
void Transform_AVX_SoA(SoAVertexs* __restrict pIn, SoAVertexs* __restrict pOut, size_t nCount, float fScale)
{
    assert((nCount % 8) == 0);

    //VPERMILPS+VINSERTF128
    __m256 scale = _mm256_set1_ps(fScale);

    float* pX = pIn->x.data();
    float* pY = pIn->y.data();
    float* pZ = pIn->z.data();

    float* pXo = pOut->x.data();
    float* pYo = pOut->y.data();
    float* pZo = pOut->z.data();

    //Unaligned Instructions
    for (size_t i = 0; i + 8 <= nCount; i += 8)
    {
        //VMOVUPS
        __m256 vx = _mm256_loadu_ps(pX + i);
        __m256 vy = _mm256_loadu_ps(pY + i);
        __m256 vz = _mm256_loadu_ps(pZ + i);

        //VMULPS
        vx = _mm256_mul_ps(vx, scale);
        vy = _mm256_mul_ps(vy, scale);
        vz = _mm256_mul_ps(vz, scale);

        //VMOVUPS
        _mm256_storeu_ps(pXo + i, vx);
        _mm256_storeu_ps(pYo + i, vy);
        _mm256_storeu_ps(pZo + i, vz);
    }
}