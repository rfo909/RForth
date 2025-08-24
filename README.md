Forth never ceases to fascinate !
=================================

Version 3 (alpha)

2025-08-24 RFO

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the quality of the toolchain and that deploy is so easy. 

Still, apart from handling serial I/O, the hardware isn't too important,
because bytecode. 

The virtual machine that runs the bytecode is a stack machine, where
most of the instructions are Forth in syntax and semantics, and 
so end up in the system dictionary. 

External assembler
------------------

An assembler is written as a [CFT](https://github.com/rfo909/CFT) script, 
which is a script language I've written myself (in Java). It is perfect for
prototyping (as well as daily tool). The assembler reads the source file
and generates a sequence of single byte instructions.

Numeric constants, both tags (addresses) and in the code, are converted via a lookup
map to single byte values in the range 128-255. Static strings in the code are
stored in a separate memory area, with offsets into that area managed as other
numeric constants.

This results in compact bytecode output, at the expense of lookups.

CFT interpreter
---------------

The initial interpreter, for testing and stepping through code, is also 
written in CFT. This has been invaluable for validating the assembler. It
correctly simulates a byte oriented memory, on which the three stacks of the
language also live. 

The REPL
--------
The REPL is implemented as a state machine in the "assembly" language, in
the file ACode.txt, after I found out how both to process input character by
character, and what little effort it takes to compile to "machine code" in
my simulated environment.


Memory map
----------
The ACode.txt file is a memory map, with code and data, such as the initial dictionary.
The system has some memory protection abilities. 

It uses two special tags in the source code

```
:DATA
:PROTECT
```

The first one aids the integrated disassembler (in the Assembler script) in presenting
data, while the second is used by the interpreter as a lower limit for the write
instructions "!" and "writeb".


A journey ...
-------------

Writing code, deleting code, pondering, reading and learning ...

The "assembly" level code for the system is in the ACode.txt file, and it has
been over a few revisions after starting version 3 of RForth. I needed some
time learning to do things simple in the assembly language, which is VERY
similar to Forth.

It became clear to me that there is no big distinction between the compile
loop and the interpret loop. Most of the code deals with recognizing numbers,
isolating words, and traversing the dictionary, looking up words. The difference
is then what to do, create code for a call, or do the call. 

Words have three states, registered in the dictionary:

- NORMAL: invoked in interactive mode, have calls to them generated in compile mode
- IMMEDIATE: invoked in compile mode, have no meaning interactively??
- INLINE: like normal, but the code pointer field is used to contain a single instruction, 
	not a subroutine pointer, from where we expect
	to be returned by a call to "ret".
 	
The bytecode of the interpreter/compiler along with some 300 bytes of reserved memory
for state, compile buffers and compile stack, totals about 600 bytes. 

The initial dictionary is also about 600 bytes, 



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

Note that the REPL state machine does not use local variables, as it uses only
jumps between states, no "call - ret". Local variables are a feature of callable
subroutines (addresses in assembly, words in Forth). 


The three stacks
----------------
The data stack is just that. It is used for parameter passing and results. It stores single words
regardless of if values are read or written as bytes from/to memory.

The word size is set to 2 bytes. This gives an address space of 64k.

Then there is the call stack ("return stack"). It is also a stack of words. When doing a call to a subroutine,
the return address is pushed on the call stack. But in addition, the call stack is where
up to 3 local variables can be created with cpush. To control data content on the call stack,
we added a third stack:

The frame stack keeps track of what belongs to which call frame on the call stack. It contains
pairs of words, called base and size. The base is the position on the call stack (counted from
the bottom), where the current frame starts, and the size is the number of words pushed on
the call stack in the current frame.

Creating a local variable by calling cpush increases size by 1, and so does calling some
function, as we then push the return position (PC+1) on the call stack. When returning
from a call (ret), we have all the information we need to locate the return position. It
is removed from the call stack, and size for the frame is decremented by one, before
updating the PC, effectively doing a jump.


Debugging
---------

The Assembler script contains a disassembler that is immediately applied to the output
from the Assembler, to show what has been done. 

Together with the memory dump feature and general information about stacks, variables and
globals in the Interpreter, and the "halt" instruction together with dynamic breakpoints,
it is possible to debug the code quite efficiently.




- [Instruction set](InstructionSet.md) for regular opcodes 33-127


