# Kaleidoscope

A small programming language implemented in C++ as a project for learning LLVM and compiler construction.

The project currently includes lexing, parsing, an abstract syntax tree, LLVM IR generation, optimization, JIT compilation, and control flow.

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
- `if/then/else` expressions
- `for` loops

Currently working on User-Defined Operators.

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
