# Computer Architecture and Operating Systems

Three independent university programming assignments: a C library and two x86-64 assembly programs.

## Projects

| Project | Language | Main techniques | Links |
| --- | --- | --- | --- |
| Recursive Stacks | C23 | Nested stacks, cyclic references, mark-and-sweep garbage collection, allocation-failure handling | [Source](recursive_stacks/solution/rstack.c) · [Task](recursive_stacks/statement/AKSO-zadanie-1.md) · [API and tests](recursive_stacks/statement/) |
| Arithmetic Sequence | x86-64 assembly (NASM) | Signed multiword arithmetic, carry/borrow propagation, sign extension, System V AMD64 calling convention | [Source](arithmetic_sequence/solution/arithmetic_sequence.asm) · [Task](arithmetic_sequence/statement/AKSO-zadanie-2.md) · [Test driver](arithmetic_sequence/statement/arithmetic_sequence_example.c) |
| Discrete Fractals | x86-64 assembly (NASM) | String rewriting, direct Linux system calls, memory mapping and dynamic buffer management | [Source](discrete_fractal/solution/discrete_fractal.asm) · [Task](discrete_fractal/statement/AKSO-zadanie-3.md) · [Test inputs](discrete_fractal/tests/) |

**Recursive Stacks** implements a shared library for stacks containing unsigned 64-bit values or references to other stacks. The library provides a C API, file input/output and garbage collection for unreachable stacks, including cycles.

**Arithmetic Sequence** computes `A_k = A_0 + k * (A_1 - A_0)` for signed integers stored in arrays of 64-bit words. The result is split between an output array and the additional high 128 bits returned to the C caller.

**Discrete Fractals** repeatedly applies character-replacement rules to an initial string. It is a standalone Linux executable using system calls rather than the C standard library.

## Build and run

Requirements: Linux x86-64, GCC 13 or newer with C23 support, NASM and GNU binutils. The commands use the `c2x`/`gnu2x` spelling of the C23 compiler options. Run them from the repository root.

```sh
mkdir -p build
```

### Recursive Stacks

Build the shared library with the allocation wrappers supplied with the assignment, then link the example driver:

```sh
gcc -std=gnu2x -Wall -Wextra -O2 -fPIC \
  -I recursive_stacks/statement \
  -shared recursive_stacks/solution/rstack.c \
  recursive_stacks/statement/memory_tests.c \
  -Wl,--wrap=malloc -Wl,--wrap=calloc -Wl,--wrap=realloc \
  -Wl,--wrap=reallocarray -Wl,--wrap=free \
  -Wl,--wrap=strdup -Wl,--wrap=strndup \
  -o build/librstack.so

gcc -std=gnu2x -Wall -Wextra -O2 \
  -I recursive_stacks/statement \
  recursive_stacks/statement/rstack_example.c \
  -L build -lrstack '-Wl,-rpath,$ORIGIN' \
  -o build/rstack_example

(
  cd recursive_stacks/statement
  for test in zero one two three four five memory; do
    ../../build/rstack_example "$test" || exit 1
  done
)
```

The driver checks the library with assertions. The `memory` case also injects allocation failures. Run it from the statement directory so it can find its input files.

### Arithmetic Sequence

```sh
nasm -f elf64 -w+all -w+error \
  -o build/arithmetic_sequence.o \
  arithmetic_sequence/solution/arithmetic_sequence.asm

gcc -std=c2x -Wall -Wextra -O2 -z noexecstack \
  arithmetic_sequence/statement/arithmetic_sequence_example.c \
  build/arithmetic_sequence.o -o build/arithmetic_sequence_example

./build/arithmetic_sequence_example
```

The C driver includes `test_data.c` and prints `PASS` or `FAIL` for each case.

### Discrete Fractals

```sh
nasm -f elf64 -w+all -w+error -w-unknown-warning -w-reloc-rel \
  -o build/discrete_fractal.o \
  discrete_fractal/solution/discrete_fractal.asm

ld -pie -I /lib64/ld-linux-x86-64.so.2 --fatal-warnings \
  -o build/discrete_fractal build/discrete_fractal.o

printf 'A\nAAB\nBA\n' | ./build/discrete_fractal 4
```

Expected output: `ABAABABA`. The first input line is the initial string; each subsequent line contains a character followed by its replacement. The command-line argument sets the number of iterations.

## Assignment materials

The `solution/` directories contain my implementations. Assignment statements, headers and example drivers are kept in the corresponding `statement/` directories; additional fixtures are in `tests/` where present. The original statements are in Polish.
