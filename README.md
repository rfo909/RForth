Forth never ceases to fascinate !
=================================

Version 3 (alpha)



Introduction
=============
I'm designing a virtual machine instruction set, stack based. This means many instructions are
quite similar to Forth primitives. On top of this runs a REPL which implements some Forth
words like COLON and SEMICOLON.

The point of this whole project is just to have fun. Although I intend writing C code for
the Pi Pico, that is as of yet not a high priority. Experiencing how to get a Forth REPL
up and running from simulated bare metal may just as well pave the way for doing ARM assembly
instead of a C interpreter.

Realizing how little low level code is required to bring up a REPL has been great fun! The 
Forth way of extending the compiler is really clever!

Design decision - byte code
---------------------------
In order to make the interpreter as simple as possible, like using an array with function pointers
in C, all instructions are a single byte. Number literals, like addresses of functions, are 
represented using a scheme as follows:

First off, all non-number opcodes stay below 127, and so far all are printable, which makes
stuff more readable (range 33-127 to be exact). To represent numeric values, the high bit
is set to 1, and the second highest bit is set to 1 to indicate pushing a zero to the stack. If not, 
we assume there is a value on the stack to work with. That value is multiplied by 64, before adding the 
value of the last 6 bits (0-63).

```
Example 1
---------
0xC1 = 1100 0001 

Push 0 to stack
Multiply it by 64 and add 1

Result 1

Example 2
---------
0xFF = 1111 1111

Push 0 to the stack
Multiply it by 64 and add 63

Result 63

Example 3
---------

0xC1 0x82

0xC1 = 1100 0001

Push 0 to stack
Multiply by 64 and add 1

0x82 = 1000 0010
Multiply value on stack by 64 and add 2

Result 66
```

For global jumps, addresses are encoded as 3 bytes, or 18 bits. The word size (traditionally 
called "cells" in Forth) is two bytes.

For jumps inside words, offsets are encoded as a single byte, or 6 bits, which allows max movement
forward or back to be 63 positions.



Design decisions - heap / jumps inside words
--------------------------------------------
I employ a bit if memory protection, to catch stray pointers. The base code and memory layout
in ACode.txt contains a tag :PROTECT that prevents writes to addresses below that point, currently
enforced in the Interpreter.

There is also a check against the HERE value, preventing any memory access above it. This means
data has to be allot'ed before use. This meant I had to use a static compile buffer
between the PROTECT and the end of allocated memory (HERE). This in turn led me to
make code relocatable, because once a word is compiled, the code to be copied to permanent
storage created with allot, and without internal jumps going wild. So I created two conditional
relative offset jumps, moving forward or backward, since the base code works only with
unsigned values.


The compile stack
-----------------
A simple word sized compile stack is statically allocated in ACode.txt. Each word
pushed contains a type in the first byte and an absolute location within the compile
buffer in the second. The compile buffer is limited to 255 instructions.



Assembler and interpreter
-------------------------

I've made an Assembler written in [CFT](https://github.com/rfo909/CFT) script, 
which is a script language I've written myself (in Java). It is perfect for
prototyping. 

I also created an Interpreter in CFT, with various options for inspecting
data structures etc.

The interpreter is growing in complexity, and it is also relatively slow,
chewing through 5-10k assembly instructions per second on my laptop. 

That may sound a lot, but with assembly code doing the whole REPL, interpreter and
compiler, and dictionary lookups, compilation speed is slow.

Speed?
------
A real microcontroller doing perhaps 100 instructions per simulated assembly
instruction, running at 80 MHz (Pi Pico), at 1 instruction per clock, should
result in a throughput of 800 K instructions per second, or about 80-160 times the
speed of my CFT interpreter.

Compiling code is the hardest work, due to the dictionary traversals, but when
running a tight iteration, it counts and prints values 0-5000 in about 10-12 seconds.


```
: count
	0 cpush   (set local variable a)
	DO
		a .
		a 1 add a!
		a 5000 lt AGAIN?
	;
```

Another example:

```
3 CONSTANT pi
ok

pi pi mul .      (251 ms)

: x pi pi mul . ;   (817 ms)

x			(28 ms)
```

Running compiled code is MUCH faster than compiling, and also that interpreting, because
of directory traversals. Most of the 251 ms is spend looking up the "mul" assembly instruction,
because if we eliminate it, we get

```
pi .           (71 ms)

: x pi . ;

x              (28 ms)
```


Memory map
----------
The ACode.txt file implements the REPL, which interpretes input and when
hitting a COLON, enters compile mode, and compiles code. The REPL is fairly 
primitive, as it should be.

But it also contains notation for allocating static structures (non code).


Levels of code
--------------

The project results in code at three different levels. 

- At the bottom there is the simulated CPU that runs a very Forth-like set of instructions. This is the Interpreter.
- Then there is the "assembly" language, which is represented by ACode.txt. It is translated to byte format by the Assembler.
- And on top, there is the Forth language, read and processed by the REPL written in ACode.txt. 

The initial Forth code is found in Forth.txt

The Forth language has access to most of the "assembly" level instructions as well as all 
functions and data defined in ACode by tag lookups like &CompileBuf etc.


Local variables
---------------

The assembly language supports pushing values from the data stack to the call
stack, with the cpush instruction. The first three values pushed that way
get avaiable as local variables a, b and c.

```
	45 cpush   # Take value from data stack, push onto call stack
	           # The first value is denoted a, the second b, and the third c
	          
	a          # Read value of variable a (45)
	a 1 add a! # Update variable a to 46
	
	(same for b and c)
```

With the cpush and the variable operations being single byte instructions, 
the generated code compactness rivals the use of traditional stack operations, 
like dup and swap. 


Debugging
---------

The Assembler script contains a disassembler that is immediately applied to the output
from the Assembler, to show what has been done. 

Together with the features of the Interpreter, it is possible to debug both ACode.txt and
the top level Forth code.



2025-08-31 IF THEN
------------------
A milestone. Got IF and THEN implemented in Forth. It is kind of messy, doesn't handle
nesting yet, and no ELSE, but it works, written as IMMEDIATE words, mostly implemented
in "assembly". 

Underways, I invented two instructions, for jumps inside words, rfwd? and rback? conditional
relative jumps. These make the compiled code relocatable, as I found it practical to always compile
to a static CompileBuf, then move the code to a custom length allocated buffer when compilation
succeeds, at which time we create the dictionary entry.

From there on the code can not be relocated, since additional words calling the word in question
do so by direct addressing, for speed and simplicity.


2025-08-31 Tag lookup
---------------------
Forth code can look up data and functions in ACode.txt by tag, typically &CompileBuf as well as &GetNextWord. 



2025-09-03 ELSE, DO and AGAIN?
------------------------------
Implemented a compile stack in ACode.txt, and got ELSE up together with IF and THEN modified to
use the stack. Also implemented the loop construct DO ... (bool) AGAIN?

Nested loops works fine, as do nested conditionals. 



2025-09-04 CONSTANT and VARIABLE
--------------------------------
Variables are constants pointing to single word size memory blocks allotted by the VARIABLE word. To
store bigger structures, manually allot the memory, and store the pointer to that memory as a constant.

```
55 CONSTANT x
55 VARIABLE y

y @ .      (prints 55)
99 y !     (changes variable value)
```

The Dictionary Entry format is unchanged, and currently eliminates the implementation of the DOES> word
of Forth, since I reuse the code pointer for data. Each word has a mode, and currently there are four
different modes:

- NORMAL
- IMMEDIATE
- INLINE		(code pointer is a single byte instruction)
- DATA			(code pointer is a constant data value)


Dictionary entry format
-----------------------

The format is as follows:

- symbolLength: 1 byte
- symbolData: ...
- codePointer: word (2 bytes)
- mode: word (2 bytes)
- next: word (2 bytes)

The dictionary is a linked list, and the top is stored in statically allocated location &DictionaryHead 
defined in ACode.txt, which is readable from Forth by

```
&DictionaryHead @
```


 

References
----------

- [Instruction set](InstructionSet.md) for regular opcodes 33-127


