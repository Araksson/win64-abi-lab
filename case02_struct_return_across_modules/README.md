# Case 02 — Struct Return Across Modules

## Status

Planned.

## Objective

This experiment will study how small and medium-sized structures are returned across separately compiled translation units under the Win64 ABI.

## Focus Areas

- ABI rules for aggregate return
- Cross-module consistency
- Optimization effects
- Generated calling conventions

## Reproducibility

Build using:

```cmd
build.bat O1
```

Assembly output will be generated inside the `build/` directory.
