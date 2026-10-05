# abs built-in functions

abs has **119** built-in functions. There are no user-defined operators, no
infix arithmetic, and no standard library beyond this list.

Two rules cover every function:

- **Arguments are positional and untyped.** `add(1,"2")` is legal and coerces.
- **Almost everything returns `0`.** Functions that mutate return `0`; the
  branch and loop functions return `1` for "taken" and `0` for "not taken".
  Read the value you want out of a variable you `set()` inside the call.

## Contents

- [Arithmetic](#arithmetic)
- [Comparison and logic](#comparison-and-logic)
- [Variables and functions](#variables-and-functions)
- [Control flow](#control-flow)
- [Strings](#strings)
- [String formatting](#string-formatting)
- [Lists](#lists)
- [Maths](#maths)
- [Date and time](#date-and-time)
- [Files and shell](#files-and-shell)
- [Interpreter](#interpreter)

## Arithmetic

| Function | Result | Example → output |
| --- | --- | --- |
| `add(a,b,...)` | sum of all arguments | `add(1,2,3)` → `6` |
| `mult(a,b,...)` | product of all arguments | `mult(2,3,4)` → `24` |
| `sub(a,b)` | `a - b` | `sub(10,3)` → `7` |
| `div(a,b)` | `a / b`, integer when both are integers | `div(7,2)` → `3`, `div(7.0,2)` → `3.5` |
| `mod(a,b)` | remainder, sign follows the dividend | `mod(7,3)` → `1` |
| `inc(a[,step])` | `a + step` (default 1); `set()`s the variable | `set(a,1) inc(a)` → `2` |
| `dec(a[,step])` | `a - step` (default 1); `set()`s the variable | `set(a,1) dec(a)` → `0` |
| `frac(a)` | fractional part | `frac(2.5)` → `0.5` |
| `sqr(a)` | square (a plain multiply, not `x^2`) | `sqr(5)` → `25` |
| `sign(a)` | `-1`, `0` or `1` | `sign(-4)` → `-1` |
| `abs(a)` | absolute value | `abs(-7)` → `7` |

There is **no** `/`, `*`, `+` or `-` operator, and no `%`. `print(6/2)` prints
`6`, because the number lexer reads up to the next space and `atoi` stops at the
first non-digit. Use `div()` and `mult()`.

## Comparison and logic

All comparisons are numeric. `eq` and `noteq` also compare strings.

| Function | True when | Example → output |
| --- | --- | --- |
| `eq(a,b)` | equal (`1` and `1.0` are equal) | `eq(1,1.0)` → `1` |
| `noteq(a,b)` | not equal | `noteq(1,2)` → `1` |
| `less(a,b)` | `a < b` | `less(1,2)` → `1` |
| `lesseq(a,b)` | `a <= b` | `lesseq(2,2)` → `1` |
| `gr(a,b)` | `a > b` | `gr(2,1)` → `1` |
| `greq(a,b)` | `a >= b` | `greq(2,2)` → `1` |
| `and(a,b)` | both non-zero | `and(1,1)` → `1` |
| `or(a,b)` | either non-zero | `or(0,1)` → `1` |
| `not(a)` | zero | `not(0)` → `1` |

`pi()` and `e()` are **functions**, not constants: `pi()` is `3.14159`, while a
bare `pi` is an undefined variable.

## Variables and functions

| Function | Result |
| --- | --- |
| `set(name,value)` | assigns; returns `0` |
| `get(name)` | current value |
| `unset("name")` | removes a variable; `0` if removed, `1` if absent |
| `var(name)` | declares a **function-local**; returns `0` |
| `fun(name,arg,...,body)` | defines a function; returns `0` |

The name argument to `set`, `get` and `var` may be any expression, so
`set(cat("x",0),10)` creates `x0`. `unset` is the exception: it takes the name
as a **string** and will not unset a variable when handed the variable itself.

Inside a function, `var()` declares a local; anything else assigned with `set()`
becomes a **global**. A local shadows a global of the same name, and once it is
gone the global is visible again.

## Control flow

| Function | Meaning | Returns |
| --- | --- | --- |
| `if(cond,then[,else])` | runs `then` when `cond` is non-zero | `1` / `0` |
| `while(cond,body)` | repeats `body` while `cond` | `0` |
| `do(body,cond)` | runs `body` at least once | `0` |
| `switch(v,c,a,c,a,...,default)` | first matching `c`, else the default | `1` / `0` |
| `cmd(a,b,...)` | runs each argument in order | last argument's value |
| `exit([code])` | ends the script with that status | — |

`if` never evaluates to the branch value, so a bare `if` is useless as an
expression — `set()` inside the branch instead. A missing branch is simply not
evaluated, so `if(cond)` and `if(cond,then)` are legal.

`switch` takes condition/action pairs, so the argument count must be even for
the final argument to be treated as the default.

## Strings

| Function | Result | Example → output |
| --- | --- | --- |
| `cat(a,b,...)` | concatenation, stringifying numbers | `cat("a",1,2.5)` → `a12.5` |
| `len(s)` | length | `len("hello")` → `5` |
| `pos(s,sub)` | 0-based index, or `-1` | `pos("hello","ll")` → `2` |
| `upper(s)` / `lower(s)` | case change | `upper("ab")` → `AB` |
| `trim(s)` | strips both ends | `trim("  x  ")` → `x` |
| `trimleft(s)` / `trimright(s)` | strips one end | `trimright("x  ")` → `x` |
| `copy(s,start[,len])` | substring from `start` | `copy("hello",1,3)` → `ell` |
| `erase(s,start,len)` | substring with a range removed | `erase("hello",1,2)` → `hlo` |
| `insert(s,pos,txt)` | insert at `pos` | `insert("hello",1,"XY")` → `hXYello` |
| `replace(s,old,new)` | returns a new string | `replace("aaa","a","b")` → `bbb` |
| `treplace(var,old,new)` | edits the **variable**, returns the count | `set(s,"aaa") treplace(s,"a","b")` → `3` |
| `append(var,txt)` | appends to a variable | `set(s,"a") append(s,"b")` → `0`, `s` is `ab` |
| `tok(var,sep)` / `rtok(var,sep)` | mutate `var`, returning the token | |
| `rand()` | random integer | varies |
| `str(n)` | number to string | `str(12)` → `12` |
| `int(s)` / `float(s)` | string to number | `int("34")` → `34` |
| `char(n)` | character for a code | `char(65)` → `A` |

`treplace`, `append`, `tok` and `rtok` take a **variable name**, not a value.
Passing a literal silently edits the string `"error: variable ... not found"`
and creates a junk global.

## String formatting

| Function | Result |
| --- | --- |
| `format(fmt,args...)` | returns the formatted string |
| `printf(fmt,args...)` | writes to stdout and returns `0` |

Supported conversions are `%s`, `%d`, `%i`, `%u`, `%o`, `%x`, `%X`, `%f`, `%F`,
`%e`, `%E`, `%g`, `%G`, `%a`, `%A`. `%p` and `%n` are not supported, and a
conversion with no matching argument is dropped.

`format("%05.2f|%d|%s",3.14159,7,"z")` → `03.14|7|z`

## Lists

Lists are written `(a,b,c)`. They are mutable, and `listget` is 1-based.

| Function | Result |
| --- | --- |
| `list(a,b,...)` | a new list |
| `listsize(l)` | number of elements |
| `listget(l,i)` | element `i` |
| `listset(l,i,v)` | overwrites element `i`; returns `0` |
| `listappend(l,v)` | appends; returns `0` |
| `listremove(l,i)` | removes element `i`; returns `0` |
| `listinsert(l,i,v)` | inserts before `i`; returns `0` |
| `listcreate(n,default)` | `n` copies of `default` |
| `listcopy(l)` | a **one level** copy; returns `0` |

`listcopy` is shallow: the copy shares nested list storage with the original, so
mutating a nested list is visible through both. Assigning a list to another
variable also shares storage.

## Maths

`round` rounds half away from zero; `trunc` drops the fraction; `floor` and
`ceil` round towards -inf and +inf. All four return an **integer**.

`round(2.6)` → `3`, `round(2.4)` → `2`, `trunc(2.9)` → `2`, `floor(2.9)` → `2`,
`ceil(2.1)` → `3`

`max` and `min` accept any number of arguments. `pow(a,b)` is exponentiation,
`sqrt(a)` the square root.

Trigonometry: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sinh`, `cosh`,
`tanh`, `asinh`, `acosh`, `atanh`. Logarithms: `log` (natural), `log10`, and
`exp`.

Doubles print with **5 decimal places** by default: `sqrt(2)` → `1.41421`,
`pi()` → `3.14159`.

## Date and time

Timestamps are **signed 32-bit** integers, so dates outside roughly 1901–2038
wrap. The 2038 limit comes from `int`, not from the platform.

| Function | Result |
| --- | --- |
| `now()` | current time as a timestamp |
| `date(y,m,d,h,min,s)` | **builds** a timestamp from parts |
| `strdate(t)` | `"YYYY-MM-DD HH:MM:SS"` |
| `year(t)`, `month(t)`, `day(t)` | date parts |
| `hour(t)`, `minute(t)`, `second(t)` | time parts |
| `week(t)`, `dayofweek(t)`, `dayofyear(t)` | week number, weekday (Sunday = 1), day of year |
| `difftime(a,b)` | `a - b` in seconds |
| `isleapyear(t)`, `daysinyear(t)`, `daysinmonth(m,year)` | calendar helpers |

`date()` is a constructor, not a getter — `year()`, `strdate()` and friends go
the other way:

```
date(2021,1,2,3,4,5)   -> 1609556645
strdate(1609556645)    -> 2021-01-02 03:04:05
year(1609556645)       -> 2021
```

Omitted arguments to `date()` default to zero, so `date(2021,1,2)` is midnight
in the local timezone. `daysinmonth(13,2021)` is `0` for an out-of-range month.

## Files and shell

| Function | Result |
| --- | --- |
| `fileexists(path)` | `1` if the file exists |
| `fileread(path)` | the file's contents as a string |
| `filewrite(path,contents)` | writes, returns `0` |
| `fileappend(path,contents)` | appends, returns `0` |
| `run(cmd)` | runs `cmd`, returning its **stdout** including the trailing newline |
| `sys(cmd)` | runs `cmd` via `system()`, returning the **raw wait status** |
| `env(name)` | value of an environment variable, or `""` |
| `input(prompt)` | prints `prompt` and reads one line |

`sys("exit 3")` returns `768`, not `3` — that is the raw status. Use `run()` if
you want the output instead.

A **missing file is a silent no-op**: `fileread` reports success and yields an
empty string, so a typo in a path never raises an error. Check with
`fileexists()` when that matters.

`include("lib.abs")` resolves against the **process working directory**, not the
script's own directory, so a script in a subdirectory will not find a sibling
file. `include()` runs the file in the same global scope, and `includeonce()`
will not include the same path twice.

## Interpreter

| Function | Result |
| --- | --- |
| `argc()` | number of process arguments, **including argv[0] and the flags** |
| `argv(i)` | argument `i` |
| `eval(code)` | evaluates a string of abs code |
| `print(...)` | writes its arguments, with no trailing newline |
| `puts(s)` | writes a string and a newline |
| `input(prompt)` | prompt, then read a line |

`argc()` and `argv()` expose the raw process arguments, so index `0` is the
interpreter path and index `1` is usually the first flag or the script itself.

Because the number lexer stops at non-digits, a literal beyond `INT_MAX` such as
`4102444800` silently becomes a negative number. Use a value that fits in 32
bits, or compute it with `add`.
