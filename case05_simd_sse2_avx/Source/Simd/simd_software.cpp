#include "../VertexStruct.h"
#include "simd_dispatch.h"

[[clang::noinline]]
void Transform_Scalar_AoS(AoSVertex* __restrict pIn, AoSVertex* __restrict pOut, size_t nCount, float fScale)
{
    for (size_t i = 0; i < nCount; ++i)
    {
        pOut[i].x = pIn[i].x * fScale;
        pOut[i].y = pIn[i].y * fScale;
        pOut[i].z = pIn[i].z * fScale;
    }
}

[[clang::noinline]]
void Transform_Scalar_SoA(SoAVertexs* __restrict pIn, SoAVertexs* __restrict pOut, size_t nCount, float fScale)
{
    float* pX = pIn->x.data();
    float* pY = pIn->y.data();
    float* pZ = pIn->z.data();

    float* pXo = pOut->x.data();
    float* pYo = pOut->y.data();
    float* pZo = pOut->z.data();

    for (size_t i = 0; i < nCount; ++i)
    {
        pXo[i] = pX[i] * fScale;
        pYo[i] = pY[i] * fScale;
        pZo[i] = pZ[i] * fScale;
    }
}