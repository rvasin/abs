# abs

Abs — a minimalistic, purely functional scripting language, implemented as a
tree-walking interpreter in about 2,500 lines of dependency-free C++.

Everything has a functional syntax:

```
function(a, b, c)
```

Hello world:

```
print("Hello world\n")
```

Even conditional blocks like `if` or `switch` have the same syntax. Custom
recursive functions are supported too. For example, the Nth number of the
Fibonacci sequence:

```
fun(fib,n,
   var(r)
   if(eq(n,0),set(r,0),
      if(eq(n,1),set(r,1),
         set(r,add(fib(sub(n,2)),fib(sub(n,1))))
      )
   )
   r)

print("Fibonacci test\n")
print(fib(10),"\n")
```

There are 119 built-in functions for text processing, files, date and time, plus
the usual maths including trigonometry.

Two things to know before you start: there are **no infix operators** (use
`div()`, `mult()`, `add()`), and `if()` evaluates to `1` or `0` rather than to
the branch it ran.

## Build

```
cmake --preset dev
cmake --build build
```

That produces `build/abs`. For an optimised, statically linked binary use
`cmake --preset release` instead. There are no dependencies beyond a C++11
compiler and CMake, and nothing to install.

To build without CMake:

```
g++ src/*.cpp -Iinclude/ -o abs -O3 -s -static
```

On MinGW use `-std=gnu++11`, because `run()` needs `popen`.

## Run

```
./build/abs examples/fib.abs    # run a script
./build/abs                     # interactive shell
```

| Flag | Effect |
| --- | --- |
| `-t` | print elapsed milliseconds |
| `-d` | dump the source and trace every evaluation |
| `-r` | print the parsed syntax tree |

Flags are combinable (`-td`) and are accepted on either side of the script name.

## Test

```
python3 -m pytest
```

The suite is black-box: every test runs the real binary and asserts on its
output. It looks for `$ABS_BIN`, then `build/abs`, then `build/Release/abs`,
then `./abs`.

## Documentation

| Page | What is in it |
| --- | --- |
| [docs/language.md](docs/language.md) | syntax, semantics, scoping, and the traps |
| [docs/builtins.md](docs/builtins.md) | all 119 built-in functions with examples |
| [docs/building.md](docs/building.md) | building, the command line, the REPL, sanitizers |
| [docs/README.md](docs/README.md) | documentation index |

Runnable examples are in [`examples/`](examples/), and `tests/cases/` holds
whole scripts paired with their expected output.

Video tutorial: https://www.youtube.com/watch?v=Kyp9772UYHI

## License

BSD 2-Clause. Copyright (c) 2022, Roman Vasin. See [LICENSE](LICENSE).
