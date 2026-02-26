# Case 01 — Return Value Registers (Win64 ABI)

---

## Context

The Win64 ABI defines register-based return conventions for scalar types and certain small aggregates.

Under this ABI:

- Integral and pointer types are typically returned in RAX.
- Floating-point types are returned in XMM0.
- Larger aggregates may require memory-based return via a hidden pointer parameter.

This experiment verifies these behaviors under clang-cl (Visual Studio 2026).

---

## Hypothesis

Under the Win64 ABI:

1. `int` and `std::uint64_t` are returned in RAX.
2. Floating point always in XMM0 _when not optimized away_.
3. Small aggregates (≤ 8 bytes) may be returned in registers.
4. Aggregates are returned in registers only if their size fits ABI rules.

---

## Experimental Setup

| Component | Value |
|------------|--------|
| Architecture | x64 (Win64 ABI) |
| Compiler | clang-cl |
| IDE | Visual Studio 2026 |
| Standard | ISO C++23 |
| Optimization Levels | O0, O1, O2 |
| Frame Pointer Omission | Disabled |
| Debug Info | Full (/Zi) |

Assembly is generated using `/Fa`.

> All tests use separate translation units to avoid compiler inlining and constant propagation, ensuring genuine ABI behavior.

---

## Code Under Test

See `main.cpp`.

The experiment defines multiple return functions with different return types and observes their materialization in generated assembly.

---

## Observations

### Scalar Types

- `ReturnInt()` places the result in **RAX**.
- `ReturnU64()` places the result in **RAX**.
- `ReturnDouble()` places the result in **XMM0**.
- `ReturnBool()` is materialized in **AL/RAX** depending on optimization level.

### Inlining and Return Materialization

When all functions were in a single translation unit with optimization (O1/O2), the compiler inlined the functions, and returns collapsed into single instruction sequences, often using the base return register (RAX). This obscured the calling ABI behavior.

To observe true ABI register materialization, functions were placed in separate translation units and marked `[[clang::noinline]]` to prevent inlining and constant propagation.

### Constant vs Computed Floating-Point Returns

When a function returns a compile-time constant of floating-point type, the compiler may optimize by embedding the constant into memory or immediate data, and then use general-purpose moves to return it. This does not violate the ABI, but hides expected XMM materialization.

To force an observable XMM0 return, the floating-point result should depend on a non-constant (e.g., a function parameter), preventing trivial constant folding.

### Aggregate Return Behavior

Win64 ABI classifies aggregates by size and alignment for return:
* <= 8 bytes: may use registers
* <= 16 bytes: may use two registers
* \> 16 bytes: uses memory return via hidden pointer (sret)

The observed behavior matches these rules:
- Small aggregates returned in registers
- Medium aggregates often in paired registers
- Large aggregates passed via hidden pointer

---

## Conclusion

### ABI Compliance Summary

The observations confirm compliance with Win64 ABI:
- Integers and pointers are returned in RAX
- Floating-point results use XMM0 when visible
- Aggregates are classified by size for register vs memory return

### Practical Implications

- Partial register writes (e.g., via `setcc`) require zero-extension to full register width to meet ABI contract.
- Constant folding may obscure expected return registers; experiments should avoid compile-time constants.
- Someone mixing handwritten assembly with C++ must ensure high portions of registers aren’t assumed preserved, as the ABI only guarantees the lower bits for return registers.
