# Building and running abs

abs is four translation units and has no dependencies beyond the C++ standard
library. There is no configure step and nothing to install.

## Quick start

```
cmake --preset dev
cmake --build build
./build/abs examples/fib.abs
```

The binary lands in `build/`. To run the tests:

```
python3 -m pytest
```

## Requirements

| | |
| --- | --- |
| CMake | 3.15 or newer (3.31 was used in development) |
| A C++11 compiler | GCC, Clang, or MSVC |
| Python 3 | only to run the tests |
| pytest | only to run the tests |

## Build presets

Two presets are provided. Both put the binary in `build/`.

| Preset | Type | Optimisation | Output |
| --- | --- | --- | --- |
| `dev` | Debug | `-O0 -g` | with debug info, for a debugger or sanitizers |
| `release` | Release | `-O3 -DNDEBUG`, static linking | one self-contained binary |

```
cmake --preset release
cmake --build build
```

`release` links the standard library statically, so the result runs on a machine
without a matching toolchain. Pass `-DABS_STATIC=ON` to any configure to get the
same behaviour without the preset:

```
cmake -B build -DABS_STATIC=ON
cmake --build build
```

Reconfigure from scratch by deleting `build/` first; CMake caches the generator
and options.

## Building by hand

The CMake build is a convenience, not a requirement. The whole interpreter is:

```
g++ src/*.cpp -Iinclude/ -o abs -O3 -s -static
```

Note the `src/*.cpp` glob: building only `src/abs.cpp` would miss three
translation units. That mattered when `include/atoment.h` used `vector`
without including `<vector>` — only `src/atoment.cpp` included that header
first, so it was the single TU that failed while the others still compiled.
The header now includes `<vector>` itself, qualifies every type it uses, and
every file builds standalone. `tests/test_build.py` compiles each one in
isolation so the two properties cannot silently regress.

On MinGW use `-std=gnu++11` rather than `-std=c++11`: `run()` needs `popen`,
which is a GNU extension there. The CMake build sets
`CMAKE_CXX_EXTENSIONS ON` so this is handled for you.

## Running

```
abs script.abs          # run a file
abs                    # start the REPL
```

| Flag | Effect |
| --- | --- |
| `-t` | print elapsed milliseconds when the script finishes |
| `-d` | dump the source and trace every evaluation step |
| `-r` | print the parsed syntax tree |
| `-td`, `-tr` | combinable; letters of one flag are scanned |

Flags are recognised on either side of the script name. `-d` is verbose and
slow, but it is the fastest way to find out why a value is wrong — it shows
each function as it is called and the value it produces.

## The REPL

With no script argument abs reads lines from stdin and prints an `abs> ` prompt.

The REPL prints only what `print()` and friends emit. A bare expression is
evaluated and discarded:

```
abs> set(a,10)
abs> print(add(a,5))
15
abs> add(a,5)          # no output
```

**The REPL does not exit on end of input.** It loops until it reads a line equal
to `exit`, and reading past the end of stdin yields an empty string forever, so
a piped session spins printing `abs> ` until you kill it. When piping input,
always finish with `exit`.

```
printf 'print(1)\nexit\n' | ./build/abs
```

## Tests

The suite is pytest, and every test is a black-box integration test: it runs the
real binary in a subprocess and asserts on stdout. There is no C++ unit test
target, because what needs protecting is the language — evaluation order,
scoping, and the exact text the built-ins print.

```
python3 -m pytest                     # everything
python3 -m pytest tests/test_builtins.py -k list
```

The interpreter is located in this order:

1. `$ABS_BIN`, if it points at an executable
2. `build/abs`, then `build/Release/abs`
3. `abs` in the repository root

So the usual loop is `cmake --build build && python3 -m pytest`. To test a
binary somewhere else, point `ABS_BIN` at it:

```
ABS_BIN=/tmp/abs python3 -m pytest
```

Subprocesses run with `TZ=UTC` and `LC_ALL=C` so that date functions and number
formatting do not change with the developer's locale. `tests/cases/` holds
whole scripts paired with their expected output; adding a script and its
`.expected` file is enough to add a new end-to-end test.

## Sanitizers

The tree-walking evaluator and the list storage are the risky parts, and both
are worth checking after a change:

```
g++ src/*.cpp -Iinclude/ -o /tmp/abs-asan -g -O0 \
    -fsanitize=address,undefined -fno-omit-frame-pointer
ABS_BIN=/tmp/abs-asan python3 -m pytest
```

This is clean except for two known pre-existing reports, neither of which is a
memory-safety bug:

- a load of the invalid value `(TFunCodes)-1`, which is how an unknown function
  name is represented (`include/abs.h`)
- leaks from a list built inline as an argument, such as
  `print(listsize(list(10,20)))`, where nothing takes ownership of the
  temporary

Use `ASAN_OPTIONS=detect_leaks=0` to silence the second while investigating.

## Repository layout

```
include/     headers: abs.h (parser, evaluator, TreeNode), atoment.h (AtomEnt)
src/         abs.cpp (interpreter), atoment.cpp, utils.cpp, absmain.cpp (entry)
examples/    runnable sample scripts
tests/       pytest suite; tests/cases holds scripts plus expected output
docs/        this documentation
backup/      archived makefile, Code::Blocks project and a demo video (ignored)
```
