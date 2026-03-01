# Case 3 — Structure Layout, Padding, Alignment and ABI Semantics (Win64)

## Table of Contents

- [Objective](#objective)
- [Structures Are Binary Layouts](#structures-are-binary-layouts)
- [Natural Alignment (Win64)](#natural-alignment-win64)
- [Padding Example](#padding-example)
- [Member Order Changes Layout](#member-order-changes-layout)
- [ABI Classification (Microsoft x64)](#abi-classification-microsoft-x64)
- [Inheritance Layout](#inheritance-layout)
- [Virtual Functions Insert Hidden State](#virtual-functions-insert-hidden-state)
- [offsetof and Standard Layout](#offsetof-and-standard-layout)
- [Detecting Padding (Compiler Warnings)](#detecting-padding-compiler-warnings)
- [#pragma pack](#pragma-pack)
- [alignas](#alignas)
- [Key Insight](#key-insight)
- [Header vs Source: Layout Consistency Across Translation Units](#header-vs-source-layout-consistency-across-translation-units)
- [Conclusion](#conclusion)

---

## Objective

This case explores the physical memory representation of C++ structures under the Microsoft x64 ABI and Clang

We focus on:

- Memory layout
- Padding rules
- Alignment semantics
- ABI classification
- Structure passing rules
- Inheritance layout
- Virtual table side effects
- offsetof restrictions
- #pragma pack
- alignas
- Compiler diagnostics for layout issues

**This is not conceptual C++**

**This is binary-level object representation**

---

## Structures Are Binary Layouts

A C++ structure is not an abstract container.
It is:

- A sequence of bytes
- With alignment constraints
- With compiler-inserted padding
- Governed by ABI rules

**The ABI defines how objects exist in memory and how they move across function boundaries**

---

## Natural Alignment (Win64)

| Type    | Size | Alignment |
| ------- | ---- | --------- |
| char    | 1    | 1         |
| short   | 2    | 2         |
| int     | 4    | 4         |
| float   | 4    | 4         |
| double  | 8    | 8         |
| pointer | 8    | 8         |

### Rules:

- Each member is aligned to its natural alignment
- The structure size is rounded up to the largest alignment used by its members

---

## Padding Example

```cpp
struct A
{
    char c1;
    int  i;
    char c2;
};

static_assert(sizeof(A) == 0xC);
```

Layout:

```cmd
offset 0x0      : char c1
offset 0x1-0x3  : padding
offset 0x4      : int i
offset 0x8      : char c2
offset 0x9-0xB : padding
```

**Padding is not optional**

**It is required for correct alignment**

---

## Member Order Changes Layout

```cpp
struct B
{
    char c1;
    char c2;
    int  i;
};

static_assert(sizeof(B) == 0x8);
```

- Same members
- Different order
- Different total size
- Different ABI classification

---

## ABI Classification (Microsoft x64)

Simplified rules:

- 1, 2, 4, or 8-byte aggregates -> passed in registers
- Larger aggregates -> passed by reference (hidden pointer)
- Alignment may influence classification

Changing member order may change:

- Register passing
- Stack layout
- Generated assembly

**Layout affects calling convention**

---

## Inheritance Layout

Single Inheritance

```cpp
struct Base
{
    int a;
};

struct Derived : Base
{
    int b;
};
```

Layout:

```cmd
offset 0x0: Base::a
offset 0x4: Derived::b
```

**The base subobject is placed at offset 0x0**

---

## Virtual Functions Insert Hidden State

```cpp
struct Base
{
    virtual void f();
    int a;
};
```

Layout:

```cmd
offset 0x0: vptr (8 bytes)
offset 0x8: int a
```

Adding `virtual`:

- Increases size
- Changes offsets
- Changes alignment
- Alters ABI classification
- Makes the type non-standard-layout

**Virtual functions are not free, they modify physical layout**

---

## `offsetof` and Standard Layout

`offsetof` is only valid for standard-layout types.
A type stops being standard-layout when it has:

- Virtual functions
- Complex inheritance
- Virtual inheritance
- Certain access rule violations

### Compiler Differences

#### MSVC

- Often allows `offsetof` even when technically non-standard
- May compile without error
- May produce non-portable assumptions

#### Clang

- Stricter enforcement
- Emits warnings or errors
- Respects standard-layout requirements more aggressively

If layout matters, use:

```cpp
static_assert(std::is_standard_layout_v<T>);
```

**Never assume portability otherwise**

---

## Detecting Padding (Compiler Warnings)

### Clang

Use:

```cmd
-Wpadded
```

This emits warnings when:

- Internal padding is inserted
- Trailing padding is added

**Extremely useful for ABI-sensitive code**

### MSVC

There is no direct equivalent to `-Wpadded`.
Useful flags:

```cmd
/Wall
```

For layout inspection:

```cmd
/d1reportSingleClassLayoutTypeName
```

Example:

```cmd
/d1reportSingleClassLayoutA
```

**This prints the exact layout in compiler output, very powerful for ABI debugging**

---

## `#pragma pack`

Maximum alignment control.
Example:

```cpp
#pragma pack(push, 1)

struct Packed
{
    char c;
    int  i;
};

#pragma pack(pop)

static_assert(sizeof(Packed) == 0x5);
```

**Removes padding**

### When to Use

- Binary serialization
- Network protocols
- Hardware-mapped structures
- File format definitions

### When **NOT** to Use

- General-purpose objects
- Performance-critical code
- Polymorphic types
- Virtual inheritance
- SIMD types

Packing may cause:

- Misaligned access
- Performance penalties
- ABI incompatibility

**Always localize packing with `push`/`pop`**

**Never apply globally**

---

## `alignas`

Standard C++ alignment specifier

```cpp
struct alignas(16) Aligned
{
    double d;
};

static_assert(alignof(Aligned) == 0x10);
```

Used to:

- Increase alignment
- Satisfy SIMD requirements
- Guarantee ABI contracts

| Mechanism      | Effect                      |
| -------------- | --------------------------- |
| `#pragma pack` | Reduces alignment           |
| `alignas(X)`   | Enforces specific alignment |

**`alignas` is portable and standard**

**`#pragma pack` is compiler-specific**

---

## Key Insight

Most structure-related bugs are not logic errors.
They are:

- Layout assumptions
- Ignored padding
- Hidden vptr insertion
- ABI reclassification
- Misaligned packing

**If you reason only conceptually, you will debug illusions**

**If you reason physically, the bugs become obvious**

---

## Header vs Source: Layout Consistency Across Translation Units

Fundamental Rule
> The structure layout must be identical in every translation unit

If it is not, you have an ABI violation

And the program becomes undefined behavior — silently

### Why does this matter?

In C++, every `.cpp` file is compiled independently.
If a structure is defined in a header:

```cpp
// MyStruct.h
struct S
{
    char c;
    int  i;
};
```

Every `.cpp` that includes this header must see exactly the same alignment context

If not, you have ODR + ABI breakage

### The Dangerous Scenario

File A.cpp
```cpp
#pragma pack(push, 1)
#include "MyStruct.h"
#pragma pack(pop)
```

File B.cpp
```cpp
#include "MyStruct.h"
```

Now:

- In A.cpp -> `sizeof(S) == 0x5`
- In B.cpp -> `sizeof(S) == 0x8`

**The type is now physically different across translation units, this is catastrophic**

Symptoms:

- Memory corruption
- Incorrect field reads
- Crashes far from origin
- Impossible-to-debug behavior

### Header Alignment Rules

**Never depend on external packing state**

Bad:
```cpp
// header
struct S
{
    char c;
    int i;
};
```

**If someone wraps the include with `#pragma pack(1)`, layout changes**

### Correct Defensive Pattern

If packing is required:

```cpp
#pragma pack(push, 1)

struct PackedS
{
    char c;
    int i;
};

#pragma pack(pop)
```

**This localizes packing**

**Never leak packing state across headers**

### `alignas` in Headers

If a structure requires strict alignment:

```cpp
struct alignas(16) S
{
    double d;
};
```

This must be in the header.
If you forget `alignas` in the header but assume it elsewhere:

- Layout changes
- ABI classification changes
- Stack alignment may break
- SIMD loads may fault

**Alignment requirements must live with the type definition**

### Virtual and Layout Drift

If you add a virtual function in the header:
```cpp
struct S
{
    virtual void f();
    int a;
};
```

Every translation unit must be recompiled.
Otherwise:

- Some TUs assume old layout
- Others assume new layout
- vptr offset mismatches
- Undefined behavior

**This is why clean rebuilds are mandatory after layout changes**

### ODR (One Definition Rule)

C++ requires that a type have:

- Exactly one definition
- Identical in all translation units

**Violating layout consistency is an ODR violation**

**The compiler does not always detect this**

**The linker does not always detect this**

**The program may run — incorrectly**

### ABI Stability Rule

If a structure:

- Is exposed in a public header
- Is part of a library interface
- Crosses DLL boundaries
- Is serialized

Then:

> Its layout is part of the ABI contract.

Changing:

- Member order
- Adding a virtual
- Adding a member
- Changing alignment
- Changing packing

**Breaks binary compatibility**

### Defensive Techniques

For layout-sensitive structures:

```cpp
static_assert(sizeof(S) == ExpectedSize);
static_assert(alignof(S) == ExpectedAlignment);
static_assert(std::is_standard_layout_v<S>);
```

**Place these in headers if ABI matters**

**Fail early**

### Clean Build Requirement

After modifying:

- Member order
- Virtual functions
- Packing
- Alignment
- Inheritance

**You must perform a full rebuild**

**Incremental builds may retain object files compiled with old layout**

**This is a real-world source of subtle crashes**

### Practical Rule

If a structure is layout-sensitive:

- Define it fully in the header
- Control packing locally
- Use `alignas` explicitly if required
- Avoid implicit alignment assumptions
- Enforce `static_assert` checks
- Always clean rebuild after modification

---

## Conclusion

Structure layout is not just a compile-time detail.

It is:

- A cross-module binary contract
- An ABI surface
- A memory law enforced by the compiler

**If alignment differs in one translation unit, the entire program becomes physically inconsistent**

**This is not a stylistic concern**

**It is a binary integrity issue**

All the examples can be seen in the Source/main.cpp file