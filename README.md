# abs
Abs - a minimalistic scripting programming language!

I tried to create a programming language with minimal syntax.
In Abs everything has functional syntax:

```
function(a, b, c)
```
 
For example, hello world:
```
print("Hello world\")
```
Even conditional blocks like if or switch has the same syntax.
Custom recursive functions are supported too.
For example, find Nth number of Fibonacci Sequence:

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

Watch the following video tutorial on YouTube:
https://www.youtube.com/watch?v=Kyp9772UYHI

The language supports including of external files with include() and it may work as an interactive shell when running without command like parameters or it may run a script file in this way:
```
# abs fib.abs
```

Currently there are about 120 built-in functions for text processing, working with files, date and time and there are even all math functions like trigonometry and random generator functions. More information is located in Wiki.

Built it with command:
```
g++ src/*.cpp -Iinclude/ -o abs -O3 -s -static
```