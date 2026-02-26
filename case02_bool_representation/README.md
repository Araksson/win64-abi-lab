# Case 2 — Boolean Semantics, ABI Behavior and Normalization (Win64)

## Objective

This case demonstrates:

- The semantic differences between `BOOL` (WinAPI), `BOOLEAN` (WinNT), `_Bool` (C) and `bool` (C++).
- How the Microsoft x64 ABI handles boolean returns.
- Why mixing APIs leads to subtle and frequent bugs.
- How to correctly test and normalize boolean values.
- A robust solution using an intermediate normalized type (BlBool).

## The Fundamental Problem

The system does not understand what a boolean is.

At the ABI level:

> A boolean is just an integer returned in EAX.

The meaning (`true`/`false`) is purely a language-level abstraction.

## Boolean Types in Windows and C/C++

|   Type   |	Origin  |	Size    |	Semantic Guarantee   |
|----------|------------|-----------|------------------------|
|   BOOL    |   WinAPI  |	32-bit  |	0 = FALSE, !=0 = TRUE   |
|   BOOLEAN    |   WinNT  |	8-bit  |	0 = FALSE, !=0 = TRUE   |
|   _Bool    |   C99  |	8-bit  |	Always 0 or 1   |
|   bool    |   C++  |	8-bit  |	Always 0 or 1   |

### Key difference:

- `BOOL` and `BOOLEAN` can legally return -1 (0xFFFFFFFF and 0xFF respectively).
- `bool` and `_Bool` are always normalized to 0 or 1.

## What the Win64 ABI Actually Does

In Microsoft x64 calling convention:

- Any integer ≤ 32 bits is returned in EAX.
- Writing to EAX zero-extends into RAX.

> The ABI does not encode “boolean-ness”. 
> It only guarantees the return register.

## Experiment 1 — BOOL Return

```cpp
[[clang::noinline]]
BOOL IsGreater(int a, int b)
{
    return a > b;
}
```

Typical generated assembly:

```asm
cmp ecx, edx
setg al
movzx eax, al
ret
```

Explanation:

- setg al -> produces 0 or 1 in 8 bits
- movzx eax, al -> zero-extends to 32 bits
- Return contract satisfied

### **Important:**

The compiler ensures normalization to a 32-bit value in EAX.

## Experiment 2 — C++ bool Return

```cpp
[[clang::noinline]]
bool IsGreaterCpp(int a, int b)
{
    return a > b;
}
```

Assembly is nearly identical.

However:

- The language guarantees only 0 or 1.
- You cannot store -1 in a bool.

This is a semantic guarantee, not an ABI feature.

## Where Bugs Start — Mixing APIs

### Incorrect Comparison

```cpp
if (result == TRUE)
```

If a WinAPI function returns:

```cpp
return -1;   // TRUE
```

Then:

```cpp
if (-1 == 1)  // false
```

But:

```cpp
if (result)   // correct
```

This is one of the most common WinAPI bugs.

## Cross Assignment Pitfalls

```cpp
BOOL winValue = -1;
bool cppValue = winValue;   // normalized to 1
```

Now:

```cpp
if (winValue == cppValue)
```

May fail depending on expectations.
The semantic model differs.

## Register-Level Danger (Manual ASM)

### Incorrect:

```asm
mov al, 1
ret
```

If `EAX` was not cleared previously:

- Upper bits may contain garbage.
- ABI contract is violated.

### Correct:

```asm
mov eax, 1
ret
```

or

```asm
xor eax, eax
ret
```

**Explicit normalization is critical.**

## Testing Values Correctly

### Recommended:

```cpp
if (value)
```

### Avoid:

```cpp
if (value == TRUE)
```

Unless you guarantee strict normalization.

## Forcing Normalization

Instead of:

```cpp
return condition;
```

You can explicitly normalize:

```cpp
return condition ? 1 : 0;
```

This guarantees:

- EAX = 1 (TRUE)
- or EAX = 0 (FALSE)

Which produces assembly like:

```asm
mov eax, 1
ret
```

or:

```asm
xor eax, eax
ret
```

**This makes behavior explicit and deterministic.**

## The Structural Solution — BlBool

To eliminate semantic mismatch, we introduce a normalized boolean wrapper.

```cpp
struct alignas(4) BlBool
{
    std::int32_t nValue;

    constexpr BlBool() noexcept : nValue(0) {}
    constexpr BlBool(bool bInValue) noexcept : nValue(bInValue ? 1 : 0) {}
    constexpr BlBool(std::int32_t nInValue) noexcept : nValue((nInValue != 0) ? 1 : 0) {}
};
```

**For the complete definition of the BlBool structure, see Source/Main.cpp**

This guarantees:

- Stored value is always 0 or 1.
- Compatible with both C and C++.
- Safe for WinAPI interaction.
- Deterministic comparison behavior.

## Normalized Return Example

```cpp
[[clang::noinline]]
BlBool BL_IsGreater(int a, int b)
{
    return a > b;
}
```

**What actually happens at the assembly level**

The comparison:

```asm
cmp ecx, edx
setg al
```

Produces an 8-bit boolean result (0 or 1).

Then the `BlBool(int)` constructor performs normalization:

### Conceptually:

```cpp
nValue = (nInValue != 0) ? 1 : 0;
```

Which typically generates something like:

```asm
test eax, eax
setne al
movzx eax, al
ret
```

### Important clarification

`BlBool` does not force:

```asm
mov eax, 1
```

or

```asm
xor eax, eax
```

Instead, it:

1. Tests the incoming value
2. Re-normalizes it using setne
3. Zero-extends the result into EAX

The final guarantee is the same:

- `EAX` = 0
- or `EAX` = 1

**But the mechanism is different.**

The normalization is achieved by logical re-evaluation, not by hardcoding the return value.
The constructor guarantees that any non-zero input collapses to 1, ensuring consistent cross-API semantics while still respecting the ABI return contract.

## Key Takeaways

- The ABI does not know what a boolean is.
- BOOL and bool are semantically different.
- WinAPI uses “non-zero = TRUE”.
- C++ guarantees normalization.
- Comparing against TRUE can introduce bugs.
- Explicit normalization removes ambiguity.
- A wrapper type (like BlBool) eliminates cross-API hazards.

## Practical Recommendation

When mixing:

- WinAPI
- C
- C++

Do not rely on implicit semantics.
Normalize values at boundaries.

Avoid assuming:

```cpp
TRUE == 1
```

Instead, enforce:

```cpp
value = (value != 0);
```

## Final Insight

Boolean bugs are rarely syntax errors.
They are semantic mismatches hidden behind ABI guarantees.
Normalization is not optional in cross-layer systems code.