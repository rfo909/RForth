Forth never ceases to fascinate
================================

Version 3

2025-08-14 RFO

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the toolchain and the deploy is so easy. 

It is unlikely this Forth will ever be used for anything but testing
and development.

The initial effort consisted of making a byte code assembler, and 
developing and compiling the code in the ACode.txt file, which is 
the base code. 

The compiled byte code is kept a printable string, for copy-paste
into the interpreter. 

A major milestone was getting the "call" and "ret" instructions
up and working. 

We manage our own call
stack, which is also the home for up to 3 local variables inside functions,
which means that calling and returning from functions ("words") are
implemented as JMP's. This means we are not using the microcontroller's
native call and return stack (although the C implementation of the 
interpreter, naturally will).

External assembler
------------------

The plan is to compile the base code (ACode.txt) outside the Pico,
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


The interpreter has been essential to validate each of the (currently) 56 instructions.

Byte code
---------
Having soon used all printable non-space characters for instructions, the idea when
moving on to implementing Forth, is to compile Forth words into individual byte
codes outside those used for instructions, which means that invoking those
will take the shape of a single byte in the byte code.

Going Forth ... (hah hah)
-------------------------

Before we can build a Forth prompt and compiler and dictionaries, we must
decide how to interface serial input, and test it with the CFT interpreter.

At some point we create Forth dictionaries and Forth words out of the 
low level ("assembly") functions, and the capability of defining new words,
compiling them to "assembly". At this point, after sending the base
byte code, we may pipe Forth word definitions over serial, and also
interact with it, probing and examining both memory and functionality.

When the base program stabilizes, it will get included internally in 
the interpreter (written in C), so it can execute automatically without
having to be piped into the microcontroller every time. 

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

It should be possible implementing the basics of dictionaries and running
a first (non-compiled) Forth engine within an additional 100-150 bytes.

Local variables
---------------
After first inlining code to calculate location of local variables relative to
cbase (call stack base) and the frame data (fstack), local variables are now
streamlined into the following:

```
	cpush     # push value from data stack onto call stack
	          # the first value is denoted a, the second b, and the third c
	          
	a         # read value of variable a
	#2 a!     # set variable a to 2
	
	(same for b and c)
```

With the cpush and the variable read and writes being single byte instructions, 
the generated code rivals the regular stack operations. 

Stack operations
----------------
The dup and swap operations are also implemented as single-byte instructions, but there
are a couple more.

```
	N dcopy
	
	ex.
		(x y z)
		0 dcopy
		(x y z z)
		
		(x y z)
		1 dcopy
		(z y z y)
		
	N dget
	
	ex.
		(x y z)
		1 dget
		(x z y)
```

Using these, the instructions over, rot and 2dup are inlined as follows

```
	over            #1 dcopy
	rot             #2 dget
	2dup            over over
```


