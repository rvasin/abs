# abs
Abs — a minimalistic scripting language!

I set out to create a programming language with minimal syntax.
In Abs everything has a functional syntax:

```
function(a, b, c)
```

For example, hello world:
```
print("Hello world\n")
```
Even conditional blocks like if or switch have the same syntax.
Custom recursive functions are supported too.
For example, find the Nth number of the Fibonacci sequence:

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

More examples can be found in the /examples/ folder.

Watch the following video tutorial on YouTube:
https://www.youtube.com/watch?v=Kyp9772UYHI

The language supports including external files with include(). It may also work as an interactive shell when run without command-line parameters, or it may run a script file like this:
```
# abs fib.abs
```

Currently there are about 120 built-in functions for text processing, working with files, date and time, plus all the math functions, including trigonometry and random number generators. More information is located in the Wiki.

Build it with:
```
g++ src/*.cpp -Iinclude/ -o abs -O3 -s -static
```
