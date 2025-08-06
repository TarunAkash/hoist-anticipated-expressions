# Hoist Anticipated Expressions LLVM Pass

## Overview

This project implements an **LLVM function pass** that **hoists anticipated expressions**
computations that are guaranteed to be needed on all control‑flow paths to the earliest  
common point in the control flow graph.  

By hoisting such expressions:
* Redundant computations in different branches are eliminated.
* Loop‑invariant expressions are moved out of loops.
* Execution time is reduced by avoiding repeated work.

This pass is designed to work with **LLVM 22.0** (New Pass Manager) and can be built out‑of‑tree.

## Example

### Input IR
```llvm
define i32 @simple_if_else(i32 %a, i32 %b) {
entry:
  %cmp = icmp ugt i32 %a, 2
  br i1 %cmp, label %then, label %else

then:
  %m1 = mul i32 %a, %a
  %a1 = add i32 %m1, %a
  %a2 = add i32 %a1, 5
  br label %exit

else:
  %m2 = mul i32 %a, %a
  %a3 = add i32 %m2, %a
  %a4 = add i32 %a3, %a
  br label %exit

exit:
  %phi = phi i32 [%a2, %then], [%a4, %else]
  ret i32 %phi
}
```

### After Pass
```llvm
define i32 @simple_if_else(i32 %a, i32 %b) {
entry:
  %m = mul i32 %a, %a
  %a1 = add i32 %m, %a
  %a2 = add i32 %a1, 5
  %cmp = icmp ugt i32 %a, 2
  br i1 %cmp, label %then, label %else

then:
  br label %exit

else:
  br label %exit

exit:
  %phi = phi i32 [%a2, %then], [%a2, %else]
  ret i32 %phi
}
```

The duplicated `mul` and `add` instructions in `then`/`else` are replaced by a single computation in `entry`.


## Our Approach

### 1. Understanding the Problem  
Many times in a program, the same calculation is repeated in different branches or inside loops.  
If we can safely do this calculation earlier (only once) and reuse the result,  
we remove repeated work and save execution time.  

### 2. Step-by-Step Method  

#### **Step 1: Backward Dataflow Analysis**  
- We go from the end of the program towards the start.  
- At each point, we check if an expression will definitely be needed later on **all** possible paths.  
- An expression is **anticipated** if:  
  1. It will be computed in the future on every path.  
  2. Its input values are not changed before it is computed.  

#### **Step 2: Use, Def, In, and Out Sets**  
- **UseSet** : Instructions used in a block before being redefined there.  
- **DefSet** : Instructions defined (given a new value) in the block.  
- **InSet** : Expressions anticipated at the start of the block.  
- **OutSet** : Expressions anticipated at the end of the block.  

#### **Step 3: Transfer Function**  
```

In\[B] = Use\[B] ∪ (Out\[B] − Def\[B])

```
This tells which expressions are anticipated at the start of a block.  

#### **Step 4: Confluence Function**  
For blocks with multiple successors:  
```

Out\[B] = Intersection of In\[Successors of B]

```
We take the intersection because the expression must be needed on **all** paths.  

#### **Step 5: Hoisting (Moving Up)**  
- After finding anticipated expressions,  
  we move them to the earliest safe place (dominator block).  
- We check that moving does not change program behaviour and that the operands are still valid there.  

#### **Step 6: Removing Duplicates**  
- After hoisting, we remove the same calculations from later blocks.  
- We update all uses to refer to the single hoisted instruction.  

#### **Step 7: Testing**  
- We prepare `.ll` test cases and run the pass.  
- We verify the output using **FileCheck** to confirm correctness and optimisation.




## Requirements

* LLVM **22.0.0** or newer
* CMake ≥ 3.13
* C++17 compiler (Clang or GCC)

## Building

```bash
git clone git@github.com:TarunAkash/hoist-anticipated-expressions.git
cd hoist-anticipated-expressions

mkdir build && cd build

cmake -DLLVM_DIR=$(llvm-config --cmakedir) ..

ninja
```

The output will be:
```
libHoistAnticipatedExpressions.so
```

## Running the Pass

With LLVM's `opt` tool:

```bash
opt -load-pass-plugin ./libHoistAnticipatedExpressions.so \
    -passes=hoist-anticipated-expressions input.ll -S -o output.ll
```

To see debug output of the Use/Def/In/Out sets and hoisting decisions:

```bash
opt -debug -load-pass-plugin ./libHoistAnticipatedExpressions.so \
    -passes=hoist-anticipated-expressions input.ll -disable-output
```

## Testing with FileCheck

A test file `test.ll` with `; CHECK:` directives is provided. Run:

```bash
opt -load-pass-plugin ./libHoistAnticipatedExpressions.so \
    -passes=hoist-anticipated-expressions test.ll -S | \
FileCheck test.ll
```

If no output appears, all checks have passed.

## Implementation Notes

* **Analysis**  
  The pass computes:
  * **UseSet**: non‑side‑effecting instructions used in a block
  * **DefSet**: instructions defined in the block
  * **InSet** / **OutSet**: liveness-like dataflow sets to detect expressions present on all successor paths.

* **Safety checks**  
  * Ignores instructions with side effects, memory reads/writes (unless known pure library calls).
  * Avoids hoisting when an identical instruction already exists in the target block.

* **Hoisting**  
  Anticipated expressions in `OutSet` are moved before the block terminator and duplicates in successors are removed, with uses redirected.

