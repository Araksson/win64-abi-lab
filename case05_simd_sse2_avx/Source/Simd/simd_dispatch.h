#pragma once

#include "../VertexStruct.h"

void Transform_Scalar_AoS(AoSVertex* __restrict pIn, AoSVertex* __restrict pOut, size_t nCount, float fScale);
void Transform_Scalar_SoA(SoAVertexs* __restrict pIn, SoAVertexs* __restrict pOut, size_t nCount, float fScale);

void Transform_SSE2_AoS(AoSVertex* __restrict pIn, AoSVertex* __restrict pOut, size_t nCount, float fScale);
void Transform_SSE2_SoA(SoAVertexs* __restrict pIn, SoAVertexs* __restrict pOut, size_t nCount, float fScale);

void Transform_AVX_AoS(AoSVertex* __restrict pIn, AoSVertex* __restrict pOut, size_t nCount, float fScale);
void Transform_AVX_SoA(SoAVertexs* __restrict pIn, SoAVertexs* __restrict pOut, size_t nCount, float fScale);