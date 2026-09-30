# Kaleidoscope

A small programming language implemented in C++ as a project for learning LLVM and compiler construction.

The project currently includes lexing, parsing, an abstract syntax tree, LLVM IR generation, optimization, and JIT compilation.

## Current Status

Implemented so far:

- Lexer and tokenization
- Recursive descent parser
- Abstract Syntax Tree (AST)
- LLVM IR generation
- Function definitions and extern declarations
- Top-level expression evaluation
- LLVM optimization passes
- JIT compilation

## Requirements

- C++17 compiler
- CMake
- Ninja
- LLVM
- Clang

## Building

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

## Running

```bash
./build/kaleidoscope
```

## Project Structure

```text
include/    Header files
src/        Lexer, parser, AST, code generation, and JIT
```

## Goals

- Learn the LLVM C++ API
- Understand how a compiler frontend is structured
- Generate and optimize LLVM IR
- Explore JIT compilation
