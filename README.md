Forth never ceases to fascinate !
=================================

Version 3 (alpha)

2025-08-31 Tag lookup
---------------------
Forth code can look up data and functions by tag, typically &CompileBuf as well as &GetNextWord. 

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


Introduction
-------------
I'm designing a virtual machine instruction set, stack based. This means many instructions are
quite similar to Forth primitives. On top of this runs a REPL which implements some Forth
words like COLON and SEMICOLON.

The point of this whole project is just to have fun. Although I intend writing C code for
the Pi Pico, that is as of yet not a high priority. Experiencing how to get a Forth REPL
up and running from simulated bare metal may just as well pave the way for doing ARM assembly
instead of a C interpreter.

Realizing how little low level code is required to bring up a REPL has been great fun! The 
Forth way of extending the compiler is really clever!


Assembler and interpreter
-------------------------

I've made an Assembler written in [CFT](https://github.com/rfo909/CFT) script, 
which is a script language I've written myself (in Java). It is perfect for
prototyping. 

I also created an Interpreter in CFT, with various options for inspecting
data structures etc.

The interpreter is growing in complexity, and it is also relatively slow,
chewing through 10k assembly instructions per second on my laptop. 

That may sound a lot, but with assembly code doing the whole REPL, interpreter and
compiler, and dictionary lookups, resulting speed is slow. Compiling
about 1k of Forth source involves executing 60+K instructions ...

Speed?
------
A real microcontroller doing perhaps 100 instructions per simulated assembly
instruction, running at 133 MHz (Pi Pico), at 1 instruction per clock, should
result in a throughput of 1.3 M instructions per second, or about 100 times the
speed of my CFT interpreter.

Compiling code is the hardest work, due to the dictionary traversals, so hopefully
it will be enough for doing actual work on a microcontroller.

Running a simple test using IF THEN requires about 500 instructions. Realistically
this simple piece of code should be well below one millisecond. With 1.3 M instr/sec
those 500 should take about 0.3 millis.

```
: t2 ( cond -- 32 or 32,63...)
	IF 32 THEN 63 ;
	
1 t2
```

Memory map
----------
The ACode.txt file implements the REPL, which interpretes input and when
hitting a COLON, enters compile mode, and compiles code. The REPL is fairly 
primitive, as it should be.

But it also contains notation for allocating static structures (non code).


A journey ...
-------------

Writing code, rewriting, pondering, reading and learning ...

The "assembly" level code for the system is in the ACode.txt file, and it has
been over a few revisions after starting version 3 of RForth. After defining
the assembly instructions, I needed some time learning how to do things
in that language, which is QUITE similar to Forth.

It became clear to me that there is no big distinction between the compile
loop and the interpret loop. Most of the code deals with recognizing numbers,
isolating words, and traversing the dictionary, looking up words. The difference
is then to either create code for a call, or do the call. Same with numbers,
create code that when run puts the number on the stack, or just put it on the
stack right now. 

Creating and experiencing IMMEDIATE words has been very much fun!

Words have three states, registered in the dictionary:

- NORMAL: invoked in interactive mode, have calls to them generated in compile mode
- IMMEDIATE: invoked in compile mode, have no meaning interactively??
- INLINE: like normal, but the code pointer field is used to contain a single instruction, 
	not a subroutine pointer, from where we expect
	to be returned by a call to "ret".

 	

Levels of code
--------------
The project results in code at three different levels. 

- At the bottom there is the simulated CPU that runs a very Forth-like set of instructions. This is the Interpreter.
- Then there is the "assembly" language, which is represented by ACode.txt
- And on top, there is the Forth language. The initial Forth code is found in Forth.txt

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




- [Instruction set](InstructionSet.md) for regular opcodes 33-127


