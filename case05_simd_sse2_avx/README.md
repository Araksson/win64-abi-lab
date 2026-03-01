# Case 5 - SIMD, SSE2, AVX and Architectural Discipline

## Table of Contents

- [Overview](#overview)
- [What SIMD Actually Is](#what-simd-actually-is)
- [Scalar vs SSE2 vs AVX - Practical Example](#scalar-vs-sse2-vs-avx---practical-example)
- [The Hidden Problem - VEX Encoding Appearing Unexpectedly](#the-hidden-problem---vex-encoding-appearing-unexpectedly)
- [SSE Domain vs AVX Domain](#sse-domain-vs-avx-domain)
- [Domain Transition and vzeroupper](#domain-transition-and-vzeroupper)
- [Why is it important to isolate AVX code?](#why-is-it-important-to-isolate-avx-code)
- [AoS vs SoA - Data Layout Is Critical](#aos-vs-soa---data-layout-is-critical)
- [Auto-Vectorization vs Manual Intrinsics](#auto-vectorization-vs-manual-intrinsics)
- [Runtime SIMD Detection (CPUID)](#runtime-simd-detection-cpuid)
- [Hardware vs Software SIMD](#hardware-vs-software-simd)
- [Benchmarking Discipline](#benchmarking-discipline)
- [Engineering Takeaways](#engineering-takeaways)
- [Final Conclusion](#final-conclusion)

---

## Overview

This case explores real-world SIMD usage beyond simple intrinsics.

It covers:

- Scalar vs SSE2 vs AVX implementations.
- Assembly-level differences (legacy SSE vs VEX encoding).
- Register pressure and domain transitions.
- AoS vs SoA data layouts.
- Auto-vectorization vs manual intrinsics.
- Runtime SIMD detection via CPUID.
- Why isolating AVX code is critical in legacy systems.

**This case is based on practical investigation: inspecting generated assembly, identifying unintended AVX encodings inside SSE2 paths, and restructuring the codebase to prevent cross-domain contamination.**

---

## What SIMD Actually Is

> SIMD = Single Instruction, Multiple Data

Scalar execution:

```cpp
a0 + b0
a1 + b1
a2 + b2
a3 + b3
```

SSE2 (128-bit, XMM registers):

```cpp
[a0 a1 a2 a3] + [b0 b1 b2 b3]
```

AVX (256-bit, YMM registers):

```cpp
[a0 a1 a2 a3 a4 a5 a6 a7] + [b0 ... b7]
```

Important:

> SIMD does not reduce algorithmic complexity. It increases data throughput per instruction.

---

## Scalar vs SSE2 vs AVX - Practical Example

### Scalar

```cpp
void AddScalar(float* a, float* b, float* out, int count)
{
    for (int i = 0; i < count; ++i)
    {
        out[i] = a[i] + b[i];
    }
}
```

### SSE2 (128-bit)

```cpp
#include <emmintrin.h>

void AddSSE2(float* a, float* b, float* out, int count)
{
    for (int i = 0; i < count; i += 4)
    {
        __m128 va = _mm_load_ps(&a[i]);
        __m128 vb = _mm_load_ps(&b[i]);
        __m128 vr = _mm_add_ps(va, vb);
        _mm_store_ps(&out[i], vr);
    }
}
```

Typical Assembly (legacy SSE encoding):

```asm
movaps xmm0, xmmword ptr [rcx]
addps  xmm0, xmmword ptr [rdx]
movaps xmmword ptr [r8], xmm0
```

### AVX (256-bit)

```cpp
#include <immintrin.h>

void AddAVX(float* a, float* b, float* out, int count)
{
    for (int i = 0; i < count; i += 8)
    {
        __m256 va = _mm256_load_ps(&a[i]);
        __m256 vb = _mm256_load_ps(&b[i]);
        __m256 vr = _mm256_add_ps(va, vb);
        _mm256_store_ps(&out[i], vr);
    }
}
```

Typical Assembly (VEX encoded):

```asm
vmovaps ymm0, ymmword ptr [rcx]
vaddps  ymm0, ymm0, ymmword ptr [rdx]
vmovaps ymmword ptr [r8], ymm0
```

---

## The Hidden Problem - VEX Encoding Appearing Unexpectedly

When compiling with `/arch:AVX` or `/arch:AVX2`, even 128-bit operations may be emitted as VEX-encoded instructions:

```asm
vmovaps xmm0, xmmword ptr [rcx]
vaddps  xmm0, xmm0, xmmword ptr [rdx]
```

**Even though the register is XMM, the encoding is AVX (VEX).**

This has consequences:

- Changes instruction encoding domain.
- Can increase register pressure.
- May introduce transition penalties.
- Affects legacy SSE2-only code paths.

This was discovered through direct assembly inspection.

---

## SSE Domain vs AVX Domain

### Legacy SSE

```asm
movaps xmm0, xmm1
addps  xmm0, xmm2
```

### AVX VEX Encoding

```asm
vmovaps xmm0, xmm1
vaddps  xmm0, xmm1, xmm2
```

Even if using 128-bit registers, VEX encoding switches the execution domain.

---

## Domain Transition and `vzeroupper`

Mixing legacy SSE and AVX instructions historically required:

```cmd
vzeroupper
```

Without it, transition penalties could stall the pipeline.

While modern CPUs mitigate this better, penalties still exist in:

- Register state transitions.
- Front-end decode.
- Scheduler pressure.

**This becomes critical in large legacy systems.**

---

## Why is it important to isolate AVX code?

When AVX is globally enabled:

- The compiler emits VEX instructions everywhere.
- Even legacy SSE2 paths get contaminated.
- Code intended to remain SSE2 becomes AVX-encoded.

Professional decision:

> Separate SSE2 and AVX implementations into isolated translation units.

Recommended structure:

```cmd
simd/
 ├── simd_scalar.cpp   (no SIMD flags)
 ├── simd_sse2.cpp     (/arch:SSE2)
 ├── simd_avx2.cpp     (/arch:AVX2)
 ```

Expose only a unified interface:

 ```cpp
 void Add(float* a, float* b, float* out, int count);
 ```

**The rest of the engine never mixes domains.**

**This prevents cross-contamination.**

---

## AoS vs SoA - Data Layout Is Critical

### Array of Structures (AoS)

```cpp
struct Particle
{
    float x,y,z;
    float vx,vy,vz;
};
```

Memory layout:

```cmd
x y z vx vy vz | x y z vx vy vz | ...
```

**Poor for SIMD loads.**

### Structure of Arrays (SoA)

```cpp
struct Particles
{
    float x[N];
    float y[N];
    float z[N];
};
```

Memory layout:

```cmd
x x x x x ...
y y y y y ...
z z z z z ...
```

Ideal for:

- Contiguous loads.
- Cache line utilization.
- Prefetch efficiency.

**SIMD performance depends more on layout than instruction width.**

> The choice depends on the use: SoA for **vector calculations** and **partial access**, AoS for **full access** and **readability.**

## Example - This is the behavior of the structures:

### SoA Layout (3 vertices)

```cpp
struct SoA
{
    float x[3];
    float y[3];
    float z[3];
};
```
Memory layout:
```cmd
x0 x1 x2
y0 y1 y2
z0 z1 z2
```

Now SIMD load:

```asm
movups xmm0, [rcx]          ; load x0 x1 x2 x?
movups xmm1, [rcx+0x10]     ; load y0 y1 y2 y?
movups xmm2, [rcx+0x20]     ; load z0 z1 z2 z?
```

You now have:

```cmd
xmm0 = X components
xmm1 = Y components
xmm2 = Z components
```

This is SIMD-optimal for operations like:

- Vector addition.
- Dot products across vertices.
- Batch transforms.

**No shuffles required.**

### AoS Layout (1 vertex * 3)

```cpp
struct Vertex
{
    float x;
    float y;
    float z;
};

Vertex v[3];
```

Memory layout:

```cmd
x0 y0 z0
x1 y1 z1
x2 y2 z2
```

Now if you try SIMD loading:

```asm
movups xmm0, [rcx]        ; x0 y0 z0 x1
movups xmm1, [rcx+16]     ; y1 z1 x2 y2
movups xmm2, [rcx+32]     ; z2 ?? ?? ??
```

**This is garbage for SIMD math.**

To extract X components you must:

```asm
shufps xmm?, xmm?, ...
unpcklps
movhlps
```

You need:

- Shuffles.
- Unpack operations.
- More uops.
- More register pressure.

**That’s the real cost**

### The Real Difference (Instruction-Level View)

SoA:

```cmd
3 loads
0 shuffles
0 unpack
```

AoS:

```cmd
3 loads
3–6 shuffle/unpack instructions
extra register pressure
```

The real difference lies in:

- Extra shuffle instructions.
- Port pressure.
- µop count (Micro-operations).
- Dependency chains.
- Reduced ILP (Incremental Level of Parallelism).

**That is what truly makes SoA superior for SIMD.**

| Data regime     | Winner            |
| ----------- | --------------- |
| L1/L2 bound < 8K vertex | AoS Win (~25%) |
| L3 bound  ~65K Vertex  | AoS = SoA (±5%)          |
| DRAM bound > 1M Vertex | SoA Win (~100%)     |


---

## Auto-Vectorization vs Manual Intrinsics

Simple loop:

```cpp
for (int i = 0; i < count; ++i)
{
    out[i] = a[i] + b[i];
}   
```

With `/O2` `/arch:AVX2`, compilers may generate:

```asm
vaddps ymm0, ymm1, ymm2
```

Meaning:

> The compiler can already vectorize simple loops.

Manual intrinsics are justified when:

- Precise control is required.
- Alignment guarantees are known.
- Complex branching prevents auto-vectorization.
- Specialized instructions are needed.

---

## Runtime SIMD Detection (CPUID)

Important for legacy compatibility.

SSE2 detection:

```cpp
#include <intrin.h>

bool SupportsSSE2()
{
    int info[4];
    __cpuid(info, 1);
    return (info[3] & (1 << 26)) != 0;
}
```

AVX detection (CPU + OS support):

```cpp
bool SupportsAVX()
{
    int info[4];
    __cpuid(info, 1);

    bool osUsesXSAVE = (info[2] & (1 << 27)) != 0;
    bool cpuAVX      = (info[2] & (1 << 28)) != 0;

    if (osUsesXSAVE && cpuAVX)
    {
        unsigned long long mask = _xgetbv(0);
        return (mask & 0x6) == 0x6;
    }

    return false;
}
```

Usage pattern:

- Global
```cpp
static bool gHasAVX = false;
static bool gHasSSE2 = false;
```

- In Begin of Program
```cpp
gHasAVX  = SupportsAVX();
gHasSSE2 = SupportsSSE2();
```

- Using in Code
```cpp
if (gHasAVX)
{
    AddAVX(...);
}
else if (gHasSSE2)
{
    AddSSE2(...);
}
else
{
    AddScalar(...);
}
```

**This decision should be made once at startup.**

---

## Hardware vs Software SIMD

Important clarification:

- Loop unrolling ≠ SIMD.
- SIMD ≠ always faster.
- If memory-bound, AVX provides limited benefit.

If the bottleneck is:

- Cache misses.
- Memory bandwidth.
- Poor data layout.

**Wider registers will not help.**

---

## Benchmarking Discipline

To measure correctly:

- Use large datasets.
- Warm up caches.
- Compile in Release.
- Avoid debugger.
- Prevent dead-code elimination.
- Align memory (16 bytes SSE, 32 bytes AVX).

Alignment example:

```cpp
alignas(32) float data[1024];
```

---

## Engineering Takeaways

- SIMD performance depends more on data layout than instruction width.
- AVX should be isolated to avoid domain contamination.
- Global `/arch:AVX2` can silently alter legacy code behavior.
- Runtime detection ensures compatibility.
- Assembly inspection is sometimes necessary to understand compiler output.
- Not all CPUs support AVX, especially legacy systems.

---

## Final Conclusion

SIMD is not just about intrinsics.

It is about:

- Architectural discipline.
- Instruction encoding awareness.
- Data-oriented design.
- Controlled compilation flags.
- Informed benchmarking.

**Blindly enabling AVX can degrade legacy paths.**

**Isolating SIMD implementations provides clarity, control, and predictable performance.**

