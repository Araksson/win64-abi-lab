# Case 8 - Branch Prediction vs Branchless Code

## Table of Contents

- [Objective](#objective)
- [Explanation](#explanation)
- [Simple Branch vs Branchless](#simple-branch-vs-branchless)
- [Dataset Impact](#dataset-impact)
- [Switch vs If Else Chain](#switch-vs-if-else-chain)
- [Switch vs Callback Table](#switch-vs-callback-table)
- [Context Matters](#context-matters)
- [Instruction-Level Considerations](#instruction-level-considerations)
- [Observations](#observations)
- [Practical Takeaway](#practical-takeaway)

---

## Objective

To demonstrate how CPU branch prediction affects performance and when replacing conditional branches with branchless techniques may or may not improve execution speed.

This case explores several common patterns found in real code:

- Simple conditional branches.
- Branchless arithmetic.
- Switch statements.
- Callback dispatch using tables.

The goal is to understand how modern CPUs execute these patterns and why naive "branchless is always faster" assumptions are incorrect.

---

## Explanation

Modern CPUs rely heavily on **branch prediction** to maintain high instruction throughput.

When a conditional branch is encountered, the CPU attempts to **predict the outcome before the condition is resolved**. If the prediction is correct, execution continues without interruption.

However, if the prediction is wrong, the CPU must:

- Flush the pipeline.
- Discard speculative work.
- Restart execution.

This process introduces a **misprediction penalty**, typically between **10–20 cycles** depending on the CPU microarchitecture.

Because of this, developers sometimes replace branches with **branchless code** that uses arithmetic or conditional instructions instead of control flow.

However, branchless code is not automatically faster. In many cases it can be slower due to:

- Additional instructions.
- Data dependencies.
- Blocked vectorization.
- Increased register pressure.

Understanding when each approach is beneficial is essential for performance-critical systems.

> In a hot path, a poorly placed `branchless` branch can cause the code to generate high latency, and an allocation operation that the system should resolve in 1 or 2 CPU cycles ends up taking more than 10 or 20 cycles. When you accumulate the cycles after each frame, you'll see a decrease in overall performance of >3-5%, and even an increase in CPU usage of >1-2%. This may seem insignificant, but we're talking about a very intensive allocation, not a complex operation. If this is replicated in more than one simple operation in a hot path, we're talking about very significant performance losses.

> In many complex systems, simply changing the approach from `branchless` -> `branch prediction` can significantly reduce CPU usage. "CPU usage" here refers to the CPU core usage, not the overall CPU usage, as CPUs can have 1 core or more than 64, so the overall percentage is very different from the individual core percentage. When a program runs, it utilizes one or more cores, so the reference percentage should be considered for the core executing the program.

---

## Simple Branch vs Branchless

### Branch implementation

```cpp
int sum = 0;

for (int i = 0; i < N; i++)
{
    int x = data[i];
    if (x > 0)
        sum += x;
}
```

Typical assembly (simplified):

```asm
mov eax, [data + rcx]
cmp eax, 0
jle Skip
add sum, eax

Skip:
```

If the branch is predictable, this executes extremely efficiently.

### Branchless implementation

```cpp
int sum = 0;

for (int i = 0; i < N; i++)
{
    int x = data[i];
    sum += (x > 0) * x;
}
```

Possible assembly:

```asm
mov eax, [data + rcx]
test eax, eax
setg dl
imul eax, edx
add sum, eax
```

This avoids branches but introduces:

- Extra instructions.
- Multiplication.
- Dependency chain.

Depending on the CPU, this may be slower.

> In this example, The technical explanation for why one is more efficient than the other is due to the incorporation of explicit multiplications or sets in one solution and not in the other. One uses `CMP` and `ADD`, and the other uses `TEST`, `SETG`, `IMUL`, and `ADD`.

---

## Dataset Impact

### Predictable Data

Example dataset:

```cmd
+ + + + + + + + + + + + +
```

- Branch predictor learns quickly.
- Branch version performs extremely well.
- Branchless version usually loses.

### Random Data

Example dataset:

```cmd
+ - + - - + - + - + - +
```

- Branch prediction becomes unreliable.
- Frequent pipeline flushes occur.
- Branchless code may outperform the branch version.

> The modern predictor branch is extremely efficient, which is why many optimizations related to this topic may seem "micro" but are very effective.

---

## Switch vs If Else Chain

### If Else Chain

```cpp
int process(int op, int x)
{
    if (op == 0) return x + 1;
    else if (op == 1) return x + 2;
    else if (op == 2) return x + 3;
    else if (op == 3) return x + 4;

    return x;
}
```

Typical assembly:

```asm
cmp op,0
je case0
cmp op,1
je case1
cmp op,2
je case2
cmp op,3
je case3
```

If one case dominates (for example `op == 0` most of the time), this becomes **very predictable and very fast**.

### Switch Implementation

```cpp
int process(int op, int x)
{
    switch(op)
    {
        case 0: return x + 1;
        case 1: return x + 2;
        case 2: return x + 3;
        case 3: return x + 4;
        default: return x;
    }
}
```

Compilers often generate a **jump table**:

```asm
jmp [jump_table + op * 8]
```

Advantages:

- O(1) dispatch.
- Constant-time branching.

Disadvantages:

- Indirect branch.
- Worse prediction on some CPUs.

> In some cases, the compiler resolves this `switch` directly as an `if` `else` `if` ... `else` sequence, or in the case of the `dominant JMP`, it resolves it as an `if` and the `else` as a `switch`, precisely to take advantage of branch prediction. But for that you need to have all the program code that contextualizes the use of the `switch`, otherwise the compiler will follow exactly what is programmed.

> But this doesn't mean that `switch` is less efficient than `if else if`, it depends on the order of the `if` conditions. If the data is very scattered and there's no clear sequence of conditions, `switch` can be more efficient than `if else if` if the first condition of the `if` is not the most common condition of all the others.

---

## Switch vs Callback Table

Another common pattern replaces branching with a function pointer table.

### Switch version

```cpp
int process(int op, int x)
{
    switch(op)
    {
        case 0: return add1(x);
        case 1: return add2(x);
        case 2: return add3(x);
        case 3: return add4(x);
        default: return x;
    }
}
```

Possible assembly:

```asm
cmp op,3
ja default
jmp [jump_table + op * 8]
```

> In cases where the compiler can `inline`, the `switch` is more efficient than Callback table because it removes call overhead.

> In hot paths, indirectly `JMP` a callback table can cause missprediction and misscache. And in the same context, in the `switch` statement, the compiler generates more optimized code.

### Callback table

```cpp
using Func = int(*)(int);

Func table[4] =
{
    add1,
    add2,
    add3,
    add4
};

int process(int op, int x)
{
    return table[op](x);
}
```

Typical assembly:

```asm
mov rax, [table + op*8]
call rax
```

This removes branching entirely but introduces:

- Indirect function call.
- Lost inlining opportunity.
- Return prediction cost.

In many cases this performs worse than a simple switch.

> In hot paths, a `callback table` with predictable indices gains performance against a `switch` due to `cache prefetching`.

> In cases where the table is dynamic (plugins, hooks, contexts, etc.), the `switch` should have internal control structures to compensate for the dynamism, and the Callback table maintains direct access and constant access code.

---

## Context Matters

The cost of a branch depends heavily on the surrounding code.

### Case A — Cheap work inside the branch

```cpp
if (flag)
    sum += x;
```

- Here the branch overhead can dominate execution.
- Branchless may help.

### Case B — Expensive work inside the branch

```cpp
if (flag)
    result += heavy_function(x);
```

- The cost of the function dominates.
- Branchless transformation would force execution of `heavy_function` every time, which is disastrous.
- Branch prediction is clearly superior here.

---

## Instruction-Level Considerations

We will observe another difference in `ASM` code between branchless and branch prediction.

### Code with branch prediction

- Use conditional branches (`JZ`, `JNZ`, `JL`, etc.).
- Performance depends on the accuracy of the branch predictor.
- Penalty if there is misprediction (~10–20 cycles in modern CPUs).

Typical example:

```asm
cmp eax, ebx
jg  Greater
mov ecx, 0
jmp End

Greater:
mov ecx, 1

End:   
```

### Branchless code

Avoid jumps using:

- CMOVcc (conditional move): `cmovg edx, ecx`.
- SETcc: `setg al` -> converts comparison result to `0` or `1`.
- Arithmetic/bitwise operations:
```asm
sub eax, 50
sar eax, 31      ;converts to mask 0 or -1
and ebx, eax     ;applies condition without branches
```

Branchless:

- No penalty for misprediction.
- Can lengthen dependency chains (cannot be executed in parallel).

Branch prediction:
- If predictable (>75%), it can be faster.
- If unpredictable, it's very costly.

> Use branchless when the condition is unpredictable or in critical loops.

> Use branch prediction if they are highly predictable.

---

## Observations

Common outcomes of this experiment:

- Predictable branches are extremely fast.
- Random branches can significantly degrade performance.
- Branchless code avoids misprediction penalties but introduces extra work.
- Indirect dispatch mechanisms (function pointer tables) may reduce predictability and prevent inlining.

> Don't forget that it depends on the context (visible and invisible to the compiler)

---

## Practical Takeaway

Branchless programming is not a universal optimization.

Before replacing a branch, consider:

- branch predictability
- cost of instructions in each path
- possibility of inlining
- surrounding workload

In many real-world hot paths, a well-predicted branch is faster than a branchless alternative.

Performance decisions should always be guided by **measurement and inspection of generated assembly**, not assumptions.

> It is recommended to read [case 6](../case06_semantic_optimization) to implement (or understand) all the necessary optimizations.