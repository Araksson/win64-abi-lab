#pragma once
#include <vector>

//AoS
struct alignas(16) AoSVertex
{
    float x, y, z, w;
};

//SoA
struct SoAVertexs
{
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;
};