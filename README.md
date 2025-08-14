Forth never ceases to fascinate
================================

Version 3

2025-08-14 RFO

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the toolchain and the deploy is so easy. 

It is unlikely this Forth will ever be used for anything but the experience
of bringing up a full language from nothing, since libraries and interfacing
hardware at the lowest level, is an entirely different matter.

The initial effort consisted of making a byte code assembler, and 
developing and compiling the code in the ACode.txt file, which is 
the base code. 

The compiled byte code is kept a printable string, for copy-paste
into the interpreter. 

A major milestone was getting the "call" and "ret" instructions
up and working. 

We manage our own call stack, which means that calling and returning from 
functions (just tags so far) are implemented as JMP's. This means we are not
using the microcontroller's native call and return stack (although the C 
implementation of the interpreter, naturally will).

CFT assembler
-------------

The plan is to compile the base code (ACode.txt) outside the microcontroller,
using the Assembler script (written in [CFT](https://github.com/rfo909/CFT)),
and send the bytecode string (which is kept on printable format) to
it over serial.


CFT interpreter
---------------

An interpreter written in CFT has also been created, in order to step through
the byte code, to verify the assembler, and as a measuring stick
for the C code interpreter. Had to write a Memory class, for correct 
handling of bytes and words, as well as a MemStack class, which is a stack
that operates in an allot'ed area inside the Memory.


The interpreter has been essential to validate each of the (currently) 60 instructions.


Going full Forth ... 
--------------------

Before we can build a Forth prompt and compiler and dictionaries, we must
decide how to interface serial input, and test it with the CFT interpreter.

At some point we create Forth dictionaries and Forth words out of the 
low level ("assembly") functions, and the capability of defining new words,
compiling them to "assembly". At this point, after sending the base
byte code, we may pipe Forth word definitions over serial, and also
interact with it, probing and examining both memory and functionality.

When the base program stabilizes, it will get included internally in 
the interpreter (written in C), so it can execute automatically without
having to be piped into the microcontroller every time. Plus it may exist
in flash instead of RAM. 

Now we talk only Forth over the serial connection.

Great fun!!
-----------

If all this doesn't sound like fun, I don't know what will! 

Bytecode size
-------------

With bytecode being more space efficient than real assembly, actual size is
of interest. 

The current version of ACode.txt implements allocating and freeing CONS cells,
using a freelist.

With test code commented out, it now compiles to 88 bytes, which is very good!
Adding some symbols to generate output, and some code to check the CONS freelist,
bumps the generated code up to 340 bytes. It looks like this:

```
x0107Zxx0107Jxx010BJXx010Fx0115x2VgWXx0118x0122x0128x3VxAOXaYx0089Qax1
-ldx>x003ATXx010Fx0115x2VgWXx012Fx0122x0138x3Vx00AEQXx0142x1Vx0083Sx01
07OgOkx2*KaIbJbaJXx0122x0148x2VbWRx0107IOax=x00CATaIlWXdx00B5Sx0138x01
22x014Ex3VRx0107OaIxhx00F3Tx0089QaIlIaJRx0107OlamJaJRxxxxxxxx04HERE01:
08Creating04CONS05cells07Showing08freelist04Done04addr04done
```

Local variables
---------------

After first inlining code to calculate location of local variables relative to
cbase (call stack base) and the frame data (fstack), we now have up to three
local variables available in every function.
```
	cpush     # push value from data stack onto call stack
	          # the first value is denoted a, the second b, and the third c
	          
	a         # read value of variable a
	2 a!      # set variable a to 2
	
	(same for b and c)
```

With the cpush and the variable read and writes being single byte instructions, 
the generated code compactness rivals the regular stack operations. 

Stack operations
----------------
The dup, swap, over and drop operations are implemented as single-byte instructions, but there
are a couple more.

Specifically created dcopy and dget, which are general enough to implement rot and other stuff.

```
	N dcopy
	
	ex.
		(x y z)
		0 dcopy
		(x y z z) - corresponds to dup
		
		(x y z)
		1 dcopy
		(x y z y) - corresponds to over
		
	N dget
	
	ex.
		(x y z)
		2 dget
		(y z x) - corresponds to rot
```

Using these, the instructions rot and 2dup are inlined as follows

```
	rot             2 dget
	2dup            over over
```


