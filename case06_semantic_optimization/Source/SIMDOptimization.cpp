#include "Exports.h"
#include <xmmintrin.h>

[[clang::noinline]]
void Compute_Clean(float* __restrict pDest, const float* __restrict pValA, const float* __restrict pValB, const float* __restrict pValC, int nCount)
{
    for (int i = 0; i < nCount; i += 4)
    {
        __m128 va = _mm_load_ps(pValA + i);
        __m128 vb = _mm_load_ps(pValB + i);
        __m128 vc = _mm_load_ps(pValC + i);

        __m128 vmul = _mm_mul_ps(va, vb);
        __m128 vadd = _mm_add_ps(vmul, vc);

        _mm_store_ps(pDest + i, vadd);
    }
}

[[clang::noinline]]
void Compute_Unrolled(float* __restrict pDest, const float* __restrict pValA, const float* __restrict pValB, const float* __restrict pValC, int nCount)
{
    for (int i = 0; i < nCount; i += 16)
    {
        __m128 a0 = _mm_load_ps(pValA + i + 0);
        __m128 a1 = _mm_load_ps(pValA + i + 4);
        __m128 a2 = _mm_load_ps(pValA + i + 8);
        __m128 a3 = _mm_load_ps(pValA + i + 12);

        __m128 b0 = _mm_load_ps(pValB + i + 0);
        __m128 b1 = _mm_load_ps(pValB + i + 4);
        __m128 b2 = _mm_load_ps(pValB + i + 8);
        __m128 b3 = _mm_load_ps(pValB + i + 12);

        __m128 c0 = _mm_load_ps(pValC + i + 0);
        __m128 c1 = _mm_load_ps(pValC + i + 4);
        __m128 c2 = _mm_load_ps(pValC + i + 8);
        __m128 c3 = _mm_load_ps(pValC + i + 12);

        a0 = _mm_add_ps(_mm_mul_ps(a0, b0), c0);
        a1 = _mm_add_ps(_mm_mul_ps(a1, b1), c1);
        a2 = _mm_add_ps(_mm_mul_ps(a2, b2), c2);
        a3 = _mm_add_ps(_mm_mul_ps(a3, b3), c3);

        _mm_store_ps(pDest + i + 0, a0);
        _mm_store_ps(pDest + i + 4, a1);
        _mm_store_ps(pDest + i + 8, a2);
        _mm_store_ps(pDest + i + 12, a3);
    }
}