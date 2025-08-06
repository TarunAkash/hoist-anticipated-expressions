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

1. **Finding Anticipated Expressions**  
   - We check the program from the end towards the start (backward analysis) to see which expressions will definitely be needed later on every path.  
   - An expression is “anticipated” if it will be used in the future without its values changing in between.

2. **How We Calculate**  
   - **Transfer Function**: Looks at each basic block and updates which expressions are kept or removed.  
   - **Confluence Operator**: We take the common expressions from all paths (intersection) because they must be needed in all cases.

3. **Hoisting (Moving Up)**  
   - After finding the anticipated expressions, we move them to the earliest safe place in the program where they can be computed without changing the meaning of the program.  
   - This removes repeated calculations in later parts of the code.

4. **Keeping Program Correct**  
   - We make sure moving the expressions does not change how the program works.

5. **Testing**  
   - We test with `.ll` files to confirm our pass is correct and actually reduces repeated work.


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

## License

This project is released under the MIT License.

