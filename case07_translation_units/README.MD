# Case 7 - Single Translation Unit vs Multi-Translation Unit Projects

## Table of Contents

- [Objective](#objective)
- [The Single Translation Unit](#the-single-translation-unit-approach)
- [The Scaling Problem](#the-hidden-scaling-problem)
- [The Multi Translation Unit](#the-multi-translation-unit-model)
- [Benefits of Multi-Unit Projects](#benefits-of-multi-unit-projects)
- [When Each Approach Makes Sense](#when-each-approach-makes-sense)
- [Hybrid Strategy](#a-practical-hybrid-strategy)
- [Practical Advice](#practical-advice)

---

## Objective

Demonstrate the practical differences between two project structures:

### Single Translation Unit architecture

- One `.cpp`.
- Many `.h` files.

### Multi-Translation Unit architecture

- Many `.cpp`.
- Minimal `.h` files used only for interface/export

### *The goal is to show:*

- When the single-CPP architecture is useful.
- Why it breaks down in large projects.
- How modern C++ projects organize code to avoid these issues.

---

## The Single Translation Unit Approach

**Structure**

Example project layout:
```cmd
project/
│
├── main.cpp
│
├── math.h
├── renderer.h
├── audio.h
├── physics.h
├── memory.h
├── input.h
```

`main.cpp`

```cpp
#include "math.h"
#include "renderer.h"
#include "audio.h"
#include "physics.h"
#include "memory.h"
#include "input.h"
```

All implementation effectively compiles as one large compilation unit.

### *Why this works well for small projects*

Advantages:

### - **Immediate error detection:**

All headers compile together in a single context.

Common mistakes appear immediately:

- Missing includes.
- Wrong include order.
- Hidden dependencies.
- Incomplete types.

Example:

```cpp
struct A { B value; };   // B must already be defined
```

If `B` is not visible, compilation fails instantly.

> It's even possible that `B` is visible to `main.cpp`, but the inclusion order is simply incorrect, even if no visible error appears (before compilation).

### - **Structure layout consistency**

When everything is compiled together:
- The compiler sees the full definition of every type.
- Struct layout mismatches cannot occur between modules.

This is especially useful when working with:

- Memory layouts.
- Binary formats.
- Serialization.
- Engine core structures.

> For small projects, maintaining a single layout of a critical structure linked only to a single `.cpp` file (e.g. `main.cpp`) prevents the structure from changing layouts if there are alignment errors in another `.cpp` file (e.g. `OtherSourceFile.cpp`). This often generates phantom errors that are very difficult to detect unless the exact `alignment` of the layout is specified (or the corresponding warnings are triggered).

### - **Very fast compilation (for small projects)**

Since the compiler builds one large unit, there is:
- No repeated parsing.
- No repeated template instantiations.
- No repeated preprocessing.

Small projects often compile **extremely quickly**.

Example observation:

```cmd
Single Translation Unit build time:
< ~20 seconds
```

> Experimentally, in medium-sized projects (e.g. 1 `.cpp` + 30 `.h` vs 30 `.cpp` + 30 `.h`) you can get compilation differences of ~2 to >10 seconds per `.cpp` file, and this scales indefinitely.

---

## The Hidden Scaling Problem

As the project grows, the model starts to fail.

#### **Problem 1 - Massive header dependency graph**

Example:

```cmd
A.h -> B.h
B.h -> C.h
C.h -> D.h
```

Eventually you get:

```cmd
main.cpp
 ├── A.h
 │    └── B.h
 │         └── C.h
 │              └── D.h
 ```

 **A small modification triggers recompilation of everything.**

 #### **Problem 2 - Maintainability collapse**

Large header trees create problems:

- Circular includes.
- Fragile include order.
- Implicit dependencies.
- Unclear module boundaries.

A small change in a header may break dozens of systems.

#### **Problem 3 - Code navigation becomes difficult**

When everything lives in headers:

- Implementation and interface mix.
- Dependencies become implicit.
- Modular reasoning becomes harder.

> When I was developing one of my main projects, I came across these 3 problems progressively, and I can assure you that after passing a certain number of headers (approximately >50) maintenance became very difficult and I had to modularize the project in another way.

---

## The Multi-Translation Unit Model

Modern projects instead use separate compilation units.

Typical structure:

```cmd
project/
│
├── math.cpp
├── renderer.cpp
├── audio.cpp
├── physics.cpp
├── input.cpp
│
├── math.h
├── renderer.h
├── audio.h
├── physics.h
├── input.h
```

Each module owns its implementation.

Example:

`math.h`

```cpp
#pragma once

float Add(float a, float b);
```

`math.cpp`

```cpp
#include "math.h"

float Add(float a, float b)
{
    return a + b;
}
```

> Having multiple `.cpp` files makes code modularization more efficient. It improves organization, and modifying a small part of the code only requires recompiling that single module; you don't have to recompile the entire project (it seems obvious, but sometimes this behavior can cause `undefined behavior` in other modules that call it, which is why when exporting functions from `.cpp` to `.h`, you must define them very well to prevent potential problems. See [case 2](../case02_bool_representation/) and [case 6](../case06_semantic_optimization/). This last point does not apply in the case of [Single Translation Unit](#the-single-translation-unit-approach)).

---

## Benefits of Multi-Unit Projects

### -**Clear module boundaries**

Headers expose only the public interface.

*Implementation stays private.*

This improves:

- Maintainability.
- Readability.
- System architecture.

### -**Better scalability**

Large projects can contain:

- Hundreds of modules.
- Thousands of source files.

Each `.cpp` compiles independently.

### -**Incremental builds**

When modifying:
```cmd
physics.cpp
```

Only **physics.cpp** recompiles.

This drastically reduces build time for large systems.

### -**Example**

Large project compile comparison:

| Architecture             | Compile Time |
| ------------------------ | ------------ |
| Single CPP + 250 headers | ~20s         |
| Fully modular multi CPP  | ~3 minutes   |

**While the modular version compiles slower initially, it scales far better as the project grows.**

---

## When Each Approach Makes Sense

### **Single Translation Unit**

Best for:

- Small tools.
- Experimental code.
- Prototypes.
- Learning projects.
- Extremely tight structural control.

Advantages:

- Simpler dependency model.
- Immediate error detection.
- Easier structural experimentation.

### **Multi-Translation Unit**

Best for:

- Medium projects.
- Large software systems.
- Libraries.
- Team development.
- Long-term maintenance.

Advantages:

- Modular design.
- Scalable builds.
- Better code organization.

---

## A Practical Hybrid Strategy

Many experienced developers use a hybrid model:

### Early development

```cmd
1 cpp
many headers
```

Focus:

- Experimentation.
- Rapid iteration.
- Structural design.

### Mature project

Refactor to:

```cmd
many cpp
interface headers only
```

Focus:

- Maintainability.
- Modularity.
- Clean interfaces.

> Considering these compilation times, and taking into account the advantages and disadvantages of using Single or Multi, you'll likely want to use Single for some parts and Multi for others. This is perfectly reasonable in specific situations such as: *Using templates*, *Creating generic structures*, *Creating wrappers*, and *Performing optimized special calculations* (for `Single`); and *Creating systems in general*, *Immutable code*, and *Multitasking* (for `Multi`), etc. 

> For very large projects, it is probably not the most efficient to build them strictly `Unique` or strictly `Multi`; the `mix` in the examples mentioned can be extremely efficient.

---

## Practical Advice

Some useful guidelines:

- Headers should expose interfaces, not implementation.
> Except in specific situations (`templates`, `wrappers`, etc, that must be `inline` in ~90% of cases), never implement everything in the `.h` file.

- Avoid unnecessary includes in headers.
> The reference to the external structure can be created in a `.h` file without the need to use `#include` to avoid unnecessary references.

- Prefer forward declarations when possible.
> If the structure is very sensitive, do not expose it completely. Instead, you can create a simple reference like `struct MyStruct;` or `class MyClass;`, so that it can be used externally as a reference without exposing its contents to prevent direct modification or missalign.

- Keep dependency trees shallow.
> To avoid potential corruption

> Never use references like `struct MyStruct;` or `class MyClass;` within a `namespace` (unless it matches the same `namespace` to which `MyStruct` or `MyClass` is defined) because in that case the layout will not match and a compilation error will occur.

`OtherSrc.cpp`

```cpp
struct MyStruct
{
    int a;
    int b;
};
```

`SomeHeader.h` or `SomeSrc.cpp` ≠ `OtherSrc.cpp`

*Wrong*
```cpp
namespace MyNameSpace
{
...
struct MyStruct;
...
}
```

*Good*

```cpp
struct MyStruct;

namespace MyNameSpace
{
...
...
}
```

In this way, the referenced structure can be used correctly when linked, and (in the case of `.h` files) adding an `#include` directive is avoided.

Special care must be taken when declaring `mutable global variables` in `.h` files, as they are copied to every `.cpp` file.

For `immutable global variables` or `constexpr`, the opposite is true, if they need to be reused in multiple `.cpp` files, placing them in `.h` is **ideal**.

> Don't forget to modularize when necessary. Read [case 6](../case06_semantic_optimization/) to fully understand why so much emphasis is placed on `mixed` use.