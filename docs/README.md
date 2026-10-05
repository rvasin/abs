# abs documentation

abs is a minimal, purely functional scripting language: a tree-walking
interpreter in about 2,500 lines of dependency-free C++.

```
print("Hello, world!\n")
```

```
cmake --preset dev && cmake --build build
./build/abs examples/fib.abs
```

## Contents

| Page | What is in it |
| --- | --- |
| [language.md](language.md) | syntax, semantics, functions, scoping, and the traps |
| [builtins.md](builtins.md) | all 119 built-in functions with examples |
| [building.md](building.md) | building, the command line, the REPL, and the tests |

## If you only read one thing

abs has **no infix operators**. `print(6/2)` prints `6` rather than erroring,
because the number lexer stops at the first non-digit. Use `div()`, `mult()`,
`add()` and `sub()`.

The other two that catch everyone:

- `if()` evaluates to `1` or `0`, never to the branch it ran, so you must
  `set()` inside the branch.
- Inside a function, only `var()` creates a local. Any other `set()` writes to
  the **global** scope.

Both are explained, with the reasoning, in [language.md](language.md).

## Design

Everything is an expression: there are no statements, no operators and no
separate declaration syntax. A program is a list of expressions, and the
sequencing primitive is `cmd()`. Data is untyped integers, doubles, strings and
lists, coerced on demand.

Errors are values rather than failures. An undefined variable evaluates to the
string `"error: variable x not found"`; an undefined function evaluates to `0`;
a missing file is a silent no-op. A typo never stops the script, which is the
main thing to keep in mind when reading unfamiliar abs code.

## Status

The language is stable and the interpreter passes its test suite, but it is a
small hobby project with sharp edges. The ones that are documented rather than
fixed are listed at the end of [language.md](language.md) and in `AGENTS.md`.
