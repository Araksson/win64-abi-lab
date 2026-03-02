#pragma once
#include <cstdint>

void Compute_Clean(float* __restrict pDest, const float* __restrict pValA, const float* __restrict pValB, const float* __restrict pValC, int nCount);
void Compute_Unrolled(float* __restrict pDest, const float* __restrict pValA, const float* __restrict pValB, const float* __restrict pValC, int nCount);

std::uint64_t CleanScreenNoInline();
std::uint64_t CleanScreenInline();

float ComputeParametersValue();
float ComputeParametersRef();

void TestingNoRestrict();
void TestingRestrict();

void BenchPrefix();
void BenchPostfix();