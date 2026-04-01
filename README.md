# bfc — Brainfuck JIT/AOT Compiler

A Brainfuck compiler with both JIT (execute immediately via LLVM ORC) and AOT (compile to native binary) modes.

**License:** GNU GPL 3.0 or later

## Requirements

- LLVM ≥ 14 (tested with LLVM 21)
- C++20 compiler (GCC 11+ or Clang 13+)
- Meson ≥ 0.60
- A C compiler on PATH for linking (used by AOT mode to produce executables)

## Building

```sh
meson setup build
cd build
ninja
```

Install system-wide:

```sh
ninja install
```

## Usage

```
Usage: bfc [options] <file>

Options:
  -o <file>          Output file (default: derived from input)
  -O0                No optimization
  -O1                Basic optimization
  -O2                Standard optimization (default)
  -O3                Aggressive optimization
  --jit              Execute via JIT (no output file)
  -c                 Compile to object file
  -S                 Emit LLVM IR text
  --emit-llvm        Emit LLVM bitcode
  --tape-size <n>    Tape size in cells (default: 30000)
  --dump-ast         Print the AST to stderr
  --dump-ir          Print LLVM IR to stderr
  -v, --verbose      Verbose output
  --version          Print version and exit
  -h, --help         Print this help and exit
```

### Examples

Compile and run Hello World:

```sh
bfc examples/hello.bf -o hello
./hello
```

Run immediately with JIT:

```sh
bfc --jit examples/hello.bf
```

Compile to object file for manual linking:

```sh
bfc -c examples/hello.bf -o hello.o
cc hello.o -o hello
```

Inspect generated LLVM IR:

```sh
bfc -S -O0 examples/hello.bf -o hello.ll
cat hello.ll
```

Use aggressive optimization:

```sh
bfc -O3 examples/fibonacci.bf -o fib
```

Increase tape size for programs that need more memory:

```sh
bfc --tape-size 100000 my_program.bf
```

## Running Tests

```sh
cd build
ninja test
```

## Optimization Pipeline

1. **BF-level (O1+):** Contract repeated `+`/`-`/`>`/`<` into single ops with a count
2. **BF-level (O1+):** Recognize `[-]`/`[+]` and replace with a single `Clear` node
3. **BF-level (O2+):** Recognize `Clear` followed by `Add(n)` and replace with `Set(n)`
4. **BF-level (O3+):** Remove no-op moves and additions that cancel out
5. **LLVM-level:** Standard LLVM optimization pipeline at the requested level
