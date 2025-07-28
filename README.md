# Hoist Anticipated Expressions — LLVM Pass

This project implements a custom LLVM pass `hoist-anticipated-expressions` as part of the **E0255 (Jan–Apr 2025)** compiler design course assignment at IISc. The pass uses **dataflow analysis** to detect and hoist **anticipated expressions**, optimizing the code by **reducing repeated computations** and improving performance.

## What is Hoisting Anticipated Expressions?

**Anticipated expressions** are computations that will be evaluated along all paths before any of their operands are redefined. The idea behind **hoisting** such expressions is to move them **earlier** in the control flow (typically to dominator blocks) to avoid **recomputing** them multiple times.

For example:

```c
if (cond) {
    x = a + b;
    ...
} else {
    y = a + b;
    ...
}
```

Since `a + b` is computed in both branches and is not redefined in between, it is **anticipated** and can be hoisted:

```c
tmp = a + b;
if (cond) {
    x = tmp;
    ...
} else {
    y = tmp;
    ...
}
```

This reduces redundant computation of `a + b`.

---

## Toolchain Requirements (Tested for LLVM 19.1.7)

| Tool            | Required Version                                                      |
| --------------- | --------------------------------------------------------------------- |
| Git             | None specified                                                        |
| C/C++ toolchain | ≥ Clang 5.0  /  ≥ Apple Clang 10.0  /  ≥ GCC 7.4  /  ≥ MSVC 2019 16.8 |
| CMake           | ≥ 3.20.0                                                              |
| Ninja           | None specified                                                        |
| Python          | ≥ 3.8                                                                 |

---

## Prerequisites

1. Clone and build **LLVM 19.1.7** from the [official repository](https://github.com/llvm/llvm-project) using:

```bash
git clone --branch llvmorg-19.1.7 https://github.com/llvm/llvm-project.git
```

2. Build LLVM with:

```bash
cmake -S llvm -B llvm-build -G Ninja -DLLVM_ENABLE_PROJECTS="clang" -DCMAKE_BUILD_TYPE=Release
ninja -C llvm-build
```

3. Export your build directory:

```bash
export LLVM_DIR=$(pwd)/llvm-build/lib/cmake/llvm
```

---

## Build Instructions for This Pass

Clone this repository and build using `ninja`:

```bash
git clone git@github.com:TarunAkash/hoist-anticipated-expressions.git
cd hoist-anticipated-expressions

cmake -S . -B build -G Ninja -DLLVM_DIR=$LLVM_DIR
ninja -C build
```

This will generate:

```
build/LLVMHoistAnticipatedExpressions.so
```

---

## How to Run the Pass

To test your pass on a sample `.ll` IR file:

```bash
opt -load-pass-plugin ./build/LLVMHoistAnticipatedExpressions.so \
    -passes=hoist-anticipated-expressions \
    test/sample.ll -S -o test/output.ll
```

You can compare `test/output.ll` with the original to verify hoisting.

---

## Enabling Debug Output (Optional)

1. Ensure your LLVM was built with:

   ```bash
   -DLLVM_ENABLE_ASSERTIONS=ON
   ```

2. In your pass file, set:

   ```cpp
   #define DEBUG_TYPE "hoist-anticipated-expressions"
   ```

3. Then run:

   ```bash
   opt -debug-only=hoist-anticipated-expressions ...
   ```

This will print IN/OUT sets and internal states, but **will not be tested** during grading.

---

## Project Structure

```
hoist-anticipated-expressions/
├── CMakeLists.txt                  # CMake build config
├── lib/
│   └── HoistAnticipatedExpressions.cpp  # Main pass implementation
├── test/
│   └── sample.ll                   # Sample IR input
├── build/                          # Ninja build output (not versioned)
├── .gitignore
└── README.md
```

---

## Submission Instructions

1. Format your patch:

   ```bash
   git clang-format HEAD~
   ```

2. Check for whitespace issues:

   ```bash
   git diff --check HEAD~
   ```

3. Generate a patch:

   ```bash
   git format-patch HEAD~
   ```

4. Email the patch and your `README.txt` file to:

   ```
   udayb@iisc.ac.in
   ```

   With subject:

   ```
   E0255 Asst-1 submission
   ```

---

## Author

Tarun Akash\
GitHub: [https://github.com/TarunAkash](https://github.com/TarunAkash)

