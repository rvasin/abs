# The abs language

abs is a minimal, purely functional scripting language. This page covers what
the language actually does, including the places where it will surprise you.
Every example here is covered by a test in `tests/`, so the output shown is the
output you get.

For the list of built-in functions see [builtins.md](builtins.md).

## Hello

```
print("Hello, world!\n")
```

Run it with `abs hello.abs`. With no file argument, abs starts a REPL.

## The shape of a program

A program is a sequence of **statements** separated by whitespace. Newlines are
just whitespace, so a whole program can be on one line.

```
set(total,0)
set(i,1)
while(less(i,11), set(total,add(total,i)) inc(i))
print("1..10 = ",total,"\n")
```

Every function call is `name(arg,arg,...)`. Arguments are separated by commas.

### There are no infix operators

`+`, `-`, `*`, `/` and `%` are not operators. The number lexer reads digits up to
the next space or comma, and `atoi` stops at the first non-digit, so this is
**not** an error — it just gives the wrong answer:

```
print(6/2)      # prints 6
print(3*4)      # prints 3
```

Use the functions:

```
print(div(6,2))    # 3
print(mult(3,4))   # 12
```

This also applies to variable names: `print(x0+x1)` is a lookup of a variable
literally called `x0+x1`.

### There is no `;`

`;` is not a statement terminator. A semicolon becomes a bogus variable node
that evaluates to an error string and is then thrown away, so the whole line
silently does nothing:

```
set(a,1);print(a)    # prints nothing at all
```

Just use whitespace or a newline.

### Comments

`#` starts a comment that runs to the end of the line. It is only recognised
where whitespace is allowed — after a `,` or `(`, or between tokens — so it
cannot appear inside a string or in the middle of a name.

```
# a whole-line comment
set(a,1)  # a trailing comment
```

## Values

There are three types: **integers**, **doubles** and **strings**. There is no
null, and no boolean — comparisons and logic return the integers `1` and `0`.

Types are inferred from the literal and coerced on demand. `add(1,2.5)` is `3.5`
and `add(1,"2")` is `3`. Doubles print with 5 decimal places, so `div(7.0,2)`
is `3.5` but `div(7,2)` is `3`: **integer division truncates** unless one
operand is a double.

Lists are written `(1,2,3)` and are covered in [builtins.md](builtins.md).

## Variables

There is one global namespace. `set()` creates or assigns and `get()` reads:

```
set(x,10)
print(get(x))    # 10
```

The name may be any expression, which is how you fake an array:

```
set(cat("item",0),"apple")
set(cat("item",1),"pear")
print(item0)     # apple
```

`unset("x")` removes a variable. It takes the name as a **string**, so
`unset("x")` works and `unset(x)` does not.

An undefined variable does not abort the script. It evaluates to the string
`"error: variable x not found"`, which is easy to miss because it only shows up
wherever you print it:

```
print(nosuchvar)   # error: variable nosuchvar not found
```

An undefined *function* is quieter still: it returns the integer `0`.

## Functions

```
fun(square,n,mult(n,n))
print(square(7))    # 49
```

The **last** parameter is the body, and the body is what the call evaluates to.
Everything before it names the arguments, positionally from the second
parameter.

The rule that matters in practice: **commas separate arguments, and the body is
whatever follows the last argument up to the closing parenthesis.** So a
multi-statement body must *not* have a comma before its last statement.

```
# correct: 2 is the only argument, and the rest is the body
fun(bump,
   set(g,add(g,1))
   g)

# wrong: the comma makes "set(g,add(g,1))" the *name* of an argument,
# so the body is just "g" and the function never does anything
fun(bump,
   set(g,add(g,1)),
   g)
```

For a function with **no** arguments, wrap a multi-statement body in an explicit
`cmd()` so there is nothing to mistake for an argument list:

```
fun(bump, cmd(set(g,add(g,1)),g))
```

And never close the parenthesis right after the name:

```
fun(bump) set(bump,"x") bump    # the statements after this become parameters
```

`cmd()` is the sequencing primitive: it evaluates its arguments in order and
returns the last one. The parser also inserts one implicitly when it sees several
statements in a row, which is why the first example above works.

Functions are global, recursive, and a later definition replaces an earlier one.

### Scope

`var()` declares a **function-local**. Anything else you assign inside a
function with `set()` becomes a **global** — there is no implicit function
scope:

```
fun(counter,
   cmd(set(n,add(get("n"),1)),
       n))
set(n,0)
counter()
print(n)      # 1 - no var(), so the function wrote to the global
```

Adding `var(n)` to that body would make `n` local, and the global would still be
`0` when `counter()` returned:

```
fun(counter,
   cmd(var(n),
       set(n,add(get("n"),1)),
       n))
set(n,0)
counter()
print(n)      # 0 - get("n") found the local, not the global
```

A local shadows a global of the same name, and when the local goes out of scope
the global becomes visible again.

## Control flow

`if`, `while`, `do` and `switch` all return `1` or `0` — **never** the value of
the branch they ran. That is the single most common source of confusion:

```
# wrong: if() returns 1, and the branch value is discarded
print(if(1,42))     # 1

# right: set() inside the branch
set(r,"")
if(1,set(r,"yes"),set(r,"no"))
print(r)            # yes
```

A missing branch is not evaluated, so `if(cond)` and `if(cond,then)` are both
legal and simply return `1` or `0`.

`while(cond,body)` repeats while the condition is non-zero. `do(body,cond)` runs
the body at least once. Both return `0`.

`switch` takes condition/action pairs and falls back to a final default action.
Because the arguments are pairs, the total count must be even for the last one to
be treated as the default:

```
switch(choice,
   "a",set(choice,"got a"),
   "b",set(choice,"got b"),
   set(choice,"fell through"))
```

It returns `1` when a case matched and `0` when the default ran.

## String literals

Strings are double-quoted. These escapes are recognised: `\"`, `\n`, `\r`, `\t`
and `\\`. Anything else after a backslash is dropped, so `\d` is just `d`.

```
print("tab:\there\n")
```

## Errors are values

abs never aborts on a bad name, a missing file or an undefined function. Errors
are ordinary values that flow through your program:

| Mistake | Result |
| --- | --- |
| undefined variable | the string `"error: variable X not found"` |
| undefined function | the integer `0` |
| `fileread` on a missing file | an empty string, no error |
| `include` of a missing file | nothing happens |
| division by zero | undefined behaviour, usually a crash |

A typo therefore fails silently. When a value looks wrong, run with `-d` to
trace evaluation, or check for the error string.

## Including other files

`include("lib.abs")` evaluates the file in the current global scope.
`includeonce()` will not include the same path twice.

Paths resolve against the **process working directory**, not the directory of the
script, so this fails:

```
abs scripts/app.abs      # include("lib.abs") looks for ./lib.abs, not ./scripts/lib.abs
```

Run from the directory that holds the file, or use an absolute path.

## A complete example

```
# FizzBuzz
set(i,1)
while(less(i,16),
   set(out,"")
   if(eq(mod(i,3),0),set(out,cat(out,"Fizz")))
   if(eq(mod(i,5),0),set(out,cat(out,"Buzz")))
   if(eq(len(out),0),set(out,str(i)))
   print(out,"\n")
   inc(i))
```

## Known sharp edges

These are documented rather than fixed, because fixing them properly means
changing the language or the evaluator's ownership model.

- **Silent failure is the default.** An undefined variable, an undefined
  function, a missing file and a missing include all do something rather than
  failing. Check with `fileexists()` where a wrong path would be expensive.
- **Timestamps are 32-bit.** Anything outside roughly 1901–2038 wraps. Use
  `date()` to build one and avoid literals beyond `INT_MAX`.
- **List copies are shallow.** `listcopy()` and plain assignment share nested
  storage, so mutating a nested list is visible through both names.
- **In-place functions need a variable name.** `treplace`, `append`, `tok` and
  `rtok` take a name, and passing a value edits an error string and creates a
  junk global.
- **Division by zero is not guarded.** It usually crashes the interpreter
  rather than returning something.
- **`sys()` returns a wait status**, not an exit code: `sys("exit 3")` is `768`.

## Where to look next

- [builtins.md](builtins.md) — all 119 functions
- [building.md](building.md) — building, running, and the command line
- `examples/` — runnable scripts
- `tests/cases/` — scripts with expected output, the best way to see real
  behaviour
