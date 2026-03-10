# Brainfuck LLVM Compiler

A small Brainfuck compiler written in C++ using LLVM. It reads Brainfuck source code from standard input and outputs LLVM IR.

The generated IR can then be compiled with `clang` to produce a native executable.

## Build

Requires LLVM and `llvm-config`.

```bash
make
```

This builds the compiler:

```
bf_compiler
```

## Usage

Compile a Brainfuck program to LLVM IR:

```bash
./bf_compiler < program.bf > program.ll
```

Compile the generated IR:

```bash
clang program.ll -o program
```

Run it:

```bash
./program
```

## Test

A simple test command is included:

```bash
make test
```

This compiles brainfuck HelloWorld `test.bf`, builds the resulting program, and runs it.
