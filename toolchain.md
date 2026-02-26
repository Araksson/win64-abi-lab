# Toolchain and Build Configuration

This document specifies the exact toolchain configuration used in win64-abi-lab.
The primary goal is strict reproducibility.

---

## Environment

| Component | Value |
|-----------|--------|
| OS        | Windows x64 |
| IDE       | Visual Studio 2026 |
| Compiler  | clang-cl (LLVM) |
| Target    | x64 (Win64 ABI) |

## Language Standards

| Language | Version |
|-----------|--------|
| C++      | ISO C++23|
| C      | ISO C17|

---

## Warning Configuration

The project is compiled with strict diagnostic settings to surface implicit or ambiguous behavior.

Primary warning flags:

```cmd
/clang:-Wall
/clang:-Wpedantic
/clang:-Wshadow
/clang:-Wconversion
/clang:-Wsign-conversion
/clang:-Wuninitialized
/clang:-Wconditional-uninitialized
/clang:-Wnull-dereference
/clang:-Wdouble-promotion
/clang:-Wformat=2
/clang:-Wzero-as-null-pointer-constant
/clang:-Wreserved-identifier
/clang:-Werror=old-style-cast
/clang:-ferror-limit=9999
```

- Visual Studio warning level: 4

These settings intentionally enforce strict compilation discipline.

---

## Optimization Configuration

Experiments are evaluated under different optimization levels:

- O0 — no optimization
- O1 — moderate optimization (favor size)
- O2 — aggressive optimization

### Relevant flags:

```cmd
/O1
/fp:precise
```

### Additional settings:

| Component | Value |
|-----------|--------|
| Intrinsic functions | Enabled |
| Intrinsic expansion | Automatic |
| Frame pointer omission | Disabled |

---

## Linker Configuration

| Component | Value |
|-----------|--------|
| Generate debug information | FULL |
| Subsystem | Windows |
| Target architecture | x64 |

Any linker options affecting observed behavior are documented within the corresponding experiment case.

---

## Binary Inspection Tools

Binary analysis and verification may involve:

- dumpbin
- llvm-objdump
- Ghidra (optional)

---

## Reproducibility Requirements

To reproduce results precisely:

- Use Visual Studio 2026.
- Use clang-cl targeting x64.
- Apply identical compiler flags.
- Match optimization level exactly.
- Avoid modifying project defaults unless the experiment specifies otherwise.

Each case explicitly documents the conditions under which the phenomenon is observable.
