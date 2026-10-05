# AGENTS.md

Abs — a minimalistic, purely functional scripting language implemented in C++ (tree-walking interpreter). 4 TUs, ~2.5k lines, no dependencies.

## Build

```
cmake --preset dev && cmake --build build      # build/abs
cmake --preset release && cmake --build build  # -O3, static
python3 -m pytest                              # 136 black-box tests
```

CMake 3.15+, C++11 with extensions ON. The extension matters: on MinGW `run()` needs `popen`, so `-std=c++11` fails and `-std=gnu++11` works. `docs/building.md` covers the CMake options (`ABS_STATIC`) and the hand-rolled `g++ src/*.cpp -Iinclude/ -o abs` fallback.

All four translation units compile standalone under `-std=c++11`. That was not true before `941059c`: `include/atoment.h` used `vector` but had no `#include <vector>`, so it only built if something else included it first, and `src/atoment.cpp` — the one TU that includes `atoment.h` first — failed with ~24 errors. The header now includes `<vector>` itself, and every type it uses is qualified (`std::string`, `std::vector`), so it no longer depends on the `using namespace std` that `utils.h` brings in and include order cannot change whether it parses. `tests/test_build.py` pins both properties — don't reintroduce a bare `string` or `vector` there.

`.gitignore` covers `build/`, the CMake files, Python caches, `backup/`, the bare `abs` binary and `abs.zip`.

## Running & debugging flags

- `./abs script.abs` runs a file. With no file arg it enters a REPL.
- Flags are single letters, combinable (`-td`), and accepted on either side of the script name — `ParseCommandLineParams` (`src/abs.cpp`) scans every letter of every dash-argument: `-t` print elapsed msecs, `-d` dump source + eval trace, `-r` print the parsed tree.
- REPL prints **nothing** for bare expressions — only `print()` output appears. It exits only on a literal `exit` line; on EOF `getline` returns `""` forever and it spins printing `abs> ` (`src/absmain.cpp`). When piping input, always end with `exit`.
- `argc()`/`argv()` expose the raw process argv, so index 0 is the binary path, not a user argument.

## Language semantics that will bite you

- **No infix operators.** The number lexer consumes everything up to whitespace or `,()` and `atoi`/`atof` stop at the first non-digit, so `print(6/2)` prints `6` and `print(3*4)` prints `3` — silently. Always `div()`, `mult()`, `add()`.
- **Separators are only `,` `(` `)`** (`is_sep`, `src/utils.cpp`). Newlines and spaces are whitespace. `;` is *not* a terminator: `set(a,1);print(a)` prints nothing at all, because the `;` becomes a bogus variable node whose error string is discarded by `cmd()`. Never use semicolons.
- **In a `fun` parameter list, commas separate *arguments*, and the body is everything after the last argument up to the closing paren.** So a comma before the last statement of a multi-statement body turns that statement into an *argument name* and the function silently becomes a no-op. For a zero-argument function, wrap the body in an explicit `cmd()`. Never write `fun(name)` with the paren closed right after the name — the parser then swallows the following statements as further parameters. `tests/cases/functions.abs` pins all four forms.
- `fun(name, arg1, ..., body)`: arguments bind positionally from param 1, and the **last** param is the body/return expression.
- `var(x)` declares a function-local. Anything else `set()` inside a function becomes a **global** — there is no implicit scoping.
- `if(cond, a, b)` returns `1`/`0`, never the branch value. You must `set()` inside the branch (see `examples/fib.abs`). Same for `while`/`do`. A missing branch is legal and simply not evaluated.
- `switch()` returns `1`/`0`. Args are cond/action pairs, so the count must be even for the last one to be the default arm.
- `tok(v, sep)`, `rtok(v, sep)`, `treplace(v, ...)` and `append(v, ...)` take a **variable name** and mutate it in place. Passing a literal silently edits `"error: variable ... not found"` and creates a junk global. The `fun_tok` macro writes back with `SetVar(VarName, param1)` and deliberately omits `locvars`, so calling it on a function-local corrupts or creates a global.
- `unset()` takes the name as a **string** (`unset("x")`); `unset(x)` does not unset anything.
- `include()` resolves relative to the **process CWD**, not the script's directory, and a missing file is a silent no-op (`file_read` always returns true and yields an empty body, `src/utils.cpp`). Included files share the global variable scope.
- `run(cmd)` shells out via `popen` and returns captured stdout *including* the trailing newline. `sys(cmd)` uses `system()` and returns the raw wait status (`sys("exit 3")` is `768`), not an exit code.
- `date(y,m,d,h,min,s)` is a **constructor** returning a timestamp; `year()`, `strdate()` etc. go the other way. Timestamps are truncated to a signed 32-bit int, so the usable range is ~1901–2038, and a literal beyond `INT_MAX` (`4102444800`) silently becomes negative.
- `printf()` writes to stdout and returns 0; `format()` returns the string. They are not aliases.
- Doubles print with 5 decimals; integer division truncates unless an operand is a double.
- **Errors are values, not failures.** An undefined function silently returns int `0`; an undefined variable evaluates to the *string* `"error: variable X not found"`, visible only with `-d`. Typos never abort the script.

## Fixed defects

Each is pinned by a regression test.

- `listcopy()` **segfaulted**: `AddListElem` ran before `SetAtomType(atList)`/`CreateListValue()`. Fixed.
- `lesseq(a,b)` used `<` instead of `<=`, so `lesseq(2,2)` was `0`. Fixed.
- `unset()` was a stub returning 0 and removing nothing. Now erases from whichever map owns the name (local first, then global) and returns 0/1.
- `switch()` never returned a result and could never reach its default arm. Now returns 1/0 and the default runs.
- `strtrimright()` wrapped an unsigned index past zero — heap-buffer-overflow. Fixed.
- `GetDaysInMonth()` accepted months outside 1–12. Now returns 0.
- `date()` used an uninitialized `struct tm`, so omitted fields (notably `tm_isdst`) were read from indeterminate memory. Now value-initialized.
- **`fun()` recorded a raw `TreeNode*` into `custcodes` while `Process()` freed the tree.** Harmless for a script, but the REPL runs one `Process()` per line, so a definition dangled and the next line read freed memory — a reliable segfault. `custcodes` now owns a deep copy (`CloneTreeNode`), and the lookup uses `find()` so a missing function no longer inserts a NULL entry.
- **`if()` read its second parameter unconditionally**, so `print(if(1))` indexed past the parameter vector and segfaulted. `if`/`while`/`do`/`switch` now all tolerate missing arguments.

## Known remaining issues

Verified, not fixed, because a real fix means changing the language or the ownership model:

- Shallow copies: `AtomEnt::Assign` copies the list pointer, so `listcopy()` and plain assignment share nested storage. `listcopy` is one level deep by design.
- Leaks when a list is built **inline as an argument** (`print(listsize(list(10,20)))`): nothing takes ownership of the temporary. Visible under ASan/LSan; use `ASAN_OPTIONS=detect_leaks=0` to silence.
- Invalid load of `(TFunCodes)-1` (`include/abs.h`), which is how an unknown function name is represented. Benign but reported by UBSan.
- Division by zero is unguarded.
- The REPL cannot be exited by EOF or Ctrl-D.

## Tests

`tests/` is pytest, black-box only: every test runs the real binary in a subprocess and asserts on stdout. No CTest, no C++ unit target — what needs protecting is the language.

- `conftest.py` — binary discovery (`$ABS_BIN`, `build/abs`, `build/Release/abs`, `./abs`), `TZ=UTC` + `LC_ALL=C` for every subprocess, `out`/`run`/`run_repl` helpers. `run` takes `args` (before the script) and `trailing` (after it).
- `test_language.py` — syntax, scoping, functions, control flow, plus regression tests.
- `test_builtins.py` — the built-ins, grouped by area.
- `test_cli.py` — flags, `argc`/`argv`, `include` resolution, the REPL.
- `cases/*.abs` + `cases/*.expected` — whole scripts with byte-exact expected output. Adding a pair adds a test. `cases/functions.abs` is the clearest writeup of the `fun` parameter rules.

When adding a test, run it against the real interpreter before trusting the expectation. Roughly half the expectations written from reading the source were wrong; the surprising ones are documented above.

## Adding a built-in function

Three order-sensitive edits, all must agree:

1. `enum TFunCodes` — `include/abs.h`
2. `string funnames[]` — `src/abs.cpp`. **Index order must match the enum exactly**; it is used as the enum's value, so a mismatch silently mis-dispatches every later built-in.
3. `case f_x:` in the `ByteCode::EvalTreeNode` switch — `src/abs.cpp`

Reuse the existing boilerplate macros rather than hand-writing cases: `fun_double_one`, `fun_comp`, `fun_minmax`, `fun_incdec`, `fun_str`, `fun_datepart`, `fun_tok`. All 119 current built-ins have handlers; none fall through to the custom-function `default:`.

## Repo state

- Only 4 `.cpp` files. Despite the name, this is **unrelated** to `abs-lang/abs` (odino's terminal-scripting language, Go lexer/parser, process management) — no shared history, different architecture, different author. Do not assume upstream parity or try to merge.
- `docs/` holds the prose documentation (`language.md`, `builtins.md`, `building.md`); keep it in sync with behaviour changes.
- `backup/` holds the retired `makefile`, the Code::Blocks project files and a demo video. It is gitignored; nothing there is tracked.
- License: BSD 2-Clause.
