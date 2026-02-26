# win64-abi-lab

![Architecture](https://img.shields.io/badge/arch-x64-blue)
![Compiler](https://img.shields.io/badge/compiler-clang--cl-orange)
![Standard](https://img.shields.io/badge/C%2B%2B-23-purple)
![License](https://img.shields.io/badge/license-MIT-green)

## Overview

win64-abi-lab is a reproducible experiment laboratory focused on analyzing binary behavior and calling convention (ABI) details under the Win64 (x64) platform, using Clang (clang-cl) integrated in Visual Studio 2026.

The repository documents and studies subtle interactions between separately compiled modules, return value conventions, structure layout, padding, optimization effects, and code generation decisions observable in compiled binaries.

Each experiment is designed as a minimal reproducible case accompanied by technical analysis based on generated assembly and binary inspection.

---

## Objectives

- Analyze Win64 ABI behavior in function return conventions.
- Study cross-module interactions under separate compilation.
- Explore structure layout, alignment, and padding effects.
- Examine differences between logical representation and actual binary layout.
- Document Clang code generation decisions under different optimization levels.
- Compare behavior under O0, O1, and O2 optimization levels.

This repository does not aim to propose general-purpose solutions. Instead, it focuses on documenting specific, observable low-level phenomena under controlled conditions.

---

## Scope

- Target architecture: Win64 (x64)
- Operating system: Windows
- IDE: Visual Studio 2026
- Primary toolchain: clang-cl (LLVM frontend compatible with MSVC toolchain)
- Focus: generated code behavior and observable ABI characteristics
- Analysis level: low-level (register usage, memory layout, return conventions)

---

## Getting Started

### Requirements

- Windows x64
- Visual Studio 2026
- LLVM/Clang toolchain enabled (clang-cl)
- x64 Native Tools Command Prompt

### Build

**To test the code you can use win64-abi-lab.slnx, and within it you will find each project separately.**

---

## Repository Structure

Each experiment is isolated in its own directory:

```bash
/caseXX_experiment_name
```

### Each case contains:

- Minimal reproducible source code
- Exact compiler and linker flags
- Optimization level used
- Relevant assembly excerpts
- Technical observations
- Conclusions

---

## Methodology

The laboratory follows strict principles:

- Full reproducibility
- Explicit configuration documentation
- Variable isolation
- Assembly-level verification
- Binary inspection using professional tooling

Conclusions are based exclusively on observable compiler output and binary behavior.

---

## Toolchain

The exact compiler and linker configuration is documented in:

```bash
toolchain.md
```

Reproducing results requires using the same compiler version, flags, architecture, and optimization level.

---

## Intended Audience

This repository is intended for:

- Low-level C/C++ developers
- Systems engineers
- Engine developers
- Professionals working with ABI-sensitive code
- Developers interested in binary layout and cross-module behavior

---

## License

This project is distributed under the MIT License.
