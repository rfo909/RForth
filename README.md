Forth never ceases to fascinate !
=================================

Version 3

2025-08-14 RFO

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the toolchain and the deploy is so easy.

It is unlikely this Forth will ever be used for anything but the experience
of bringing up a full language from nothing, since libraries and interfacing
hardware at the lowest level, is an entirely different matter.

The effort so far consists of making a byte code assembler, for a stack based
instruction set, while developing a base code in this assembly language, and
verifying it using a stand-alone interpreter.

The compiled byte code is kept a printable string, for copy-paste
into the interpreter. 

CFT assembler
-------------

The plan is to compile the base code (ACode.txt) outside the microcontroller,
using the Assembler script (written in [CFT](https://github.com/rfo909/CFT)),
and send the bytecode string (which is kept on printable format) to
it over serial. 

Eventually this code will implement a Forth REPL which can be used to
examine state and test functionality interactively.



CFT interpreter
---------------

An interpreter written in CFT has also been created, in order to step through
the byte code, to verify the assembler, and as a measuring stick
for the C code interpreter. This turned out to be completely vital for
finding bugs, and testing each individual instruction as it is created.


Great fun!!
-----------

If all this doesn't sound like fun, I don't know what will! 



Local variables
---------------

The assembly language supports pushing values from the data stack to the call
stack, with the cpush instruction. The first three values pushed that way
get avaiable as local variables a, b and c.

```
	cpush     # push value from data stack onto call stack
	          # the first value is denoted a, the second b, and the third c
	          
	a         # read value of variable a
	2 a!      # set variable a to 2
	
	(same for b and c)
```

With the cpush and the variable read and writes being single byte instructions, 
the generated code compactness rivals the regular stack operations. 

Pushing more values with cpush has no meaning, as those are not available
in any way. 

Setting a value with [abc]! without cpush'ing a value first, is risky,
because those bytes will be overwritten if calling some subroutine.

Using the [abc] lookup instructions without cpush is a hackish way of
accessing local variables of the last called subroutine. :-)

We settled on 3 local variables, since if more are needed, in conjunction
with 2 on the stack, the function is too complex.

Stack operations
----------------

The dup, swap, over and drop operations are implemented as single-byte instructions, but there
are a couple more.

Specifically created dcopy and dget, which are general enough to implement rot and other stuff. 

Experience so far seems to be that dup, swap, combined with using local variables
is what gives the best code.



