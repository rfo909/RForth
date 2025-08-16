Forth never ceases to fascinate !
=================================

Version 3 (alpha)

2025-08-16 RFO

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the toolchain and the deploy is so easy.

It is unlikely this Forth will ever be used for anything but the experience
of bringing up a full language from nothing, since libraries and interfacing
hardware at the lowest level, is an entirely different matter.

The effort so far consists of making a byte code assembler, for a stack based
instruction set, while developing using a simple base code source file written
in this assembly language, and verifying it using a stand-alone interpreter.

CFT assembler
-------------

The assembler is written as a [CFT](https://github.com/rfo909/CFT) script, 
which is a script language I've written myself (in Java). It is perfect for
prototyping (as well as daily tool). The assembler reads the source file
and generates a sequence of single byte instructions.

An initial design decision to use printable non-space characters for instructions,
effectively reserved the range 33-127 for normal opcodes. The original design
inlined numeric constants by defining the following single byte instructions:

- "x" to push zero on the stack
- "0-9A-F" to multiply value on stack by 16 before adding 0-15.

This worked, but the problem was it grew the generated code, as the wordsize
is 2 bytes, and jump addresses are as well. That meant that any tag lookup
resulted in xNNNN in the bytecode, using 5 bytes.

With "normal" opcodes kept in a defined range, the modified scheme was to
put all numeric constants as a list of 2-byte values, and refer to these
by opcodes 128-255. So now any numeric constant is pushed on the stack
using a single byte. 

This means the number of unique tags plus the number of unique symbols ("non-space strings"),
plus the number of unique numeric constants in the code, must not exceed 127.


CFT interpreter
---------------

The initial interpreter, for testing and stepping through code, is also 
written in CFT. This has been invaluable for validating the assembler. It
correctly simulates a byte oriented memory, on which the three stacks of the
language also live. 

After sectioning off constant values into a separate sequence of bytes, and
also supporting symbols (non-space strings) for reporting and dictionary 
purposes, the three sequences of bytes are strung together to form a 
single hex-string, which is the input to the interpreter along with
offsets for where things are found.

The interpreter, when it starts, allocates bytes in simulated memory
and put the bytes from the hex-string into it. The code starts at PC=0.

Then it allocates the three stacks, as well as some few words of storage
that serve the "global" variables. This is a mechanism for creating the
very first global structures, and currently a maximum of 4 is supported,
although really just one would suffice.

After adding all this to the simulated memory, the HERE value (top of
heap) is stored internally, as a barrier for the read and write instructions, 
leaving them to work only from this address and up. In effect we've created
a combined code and system data segment that isn't freely available to
the "assembly language".


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
like dup and swap. Those are implemented as well, as single byte instructions.

Using cpush to push more than 3 values has no meaning, and might be considered
a bug, as there is no way from inside the language to access those.

Doing update of a variable with a! (or b/c) without cpush'ing a value first, is risky,
because those words may be overwritten if calling some subroutine.

Using the [abc] lookup instructions without cpush is a hackish way of
accessing local variables of the last called subroutine.

We settled on 3 local variables, since if more are needed, in conjunction
with 2 on the stack, the function is too complex.

Stack operations
----------------

The dup, swap, over and drop operations are implemented as single-byte instructions, but there
are a couple more.

Specifically created dcopy and dget, which are general enough to implement rot and other stuff. 

Experience so far seems to be that dup, swap, combined with using local variables
is what gives the best code.

The three stacks
----------------
The most used stack is the data stack. Most operations interact with it. It stores single words.

Then there is the call stack. It is also a stack of words. When doing a call to a subroutine,
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


References
----------

- [Instruction set](InstructionSet.md) for regular opcodes 33-127


