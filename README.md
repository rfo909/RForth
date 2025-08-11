Forth never ceases to fascinate
================================

- 2025-08 V3

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the toolchain and the deploy is so easy. 

It is unlikely this Forth will ever be used for anything but testing
and development.

The design is making a byte code assembler, compiling the code in
the ACode.txt file into a printable string, and over time extending this
to encompass dictionaries and a Forth language on top.

A major milestone was getting the "call" and "ret" quasi instructions
up and working. They are not real instructions, but instead represent
inlining a number of instructions (7 each). We manage our own call
stack, which is also the home for local variables inside functions,
which means that calling and returning from functions ("words") are
implemented as JMP's.

---

The plan is to compile the base code (ACode.txt) outside the Pico,
using the Assembler script (written in [CFT](https://github.com/rfo909/CFT)),
and send the bytecode string (which is kept on printable format) to
it over serial. 

Later, we create Forth dictionaries and Forth words out of the 
low level ("assembly") functions, and the capability of defining new ones,
compiling them to "assembly". At this point, after sending the base
byte code, we can send Forth word definitions, and talk interactively
with it over serial. 

If this doesn't sound like fun, I don't know what will! 
-------------------------------------------------------

The previous attempt(s) were bogged down in assembly complexities,
which I think are solved now, by introducing the xNNN number literals
as singlebyte instructions, and of course the assembler supporting
local tags within functions.

The handling of symbols ('xxx) by the assembler, as well as the
data and call stacks, feels like an improvement, compared to previous
attempts at defining as much as possible within the bytecode.

Currently the number of instructions is 33 + 17 (for numeric literals), giving a total 
of 50. We have used printable characters A-Z0-9ghi. The a-f range is free, but there exist
quasi instructions for up to 6 local variables a-f. Waiting with allocating those as
byte codes. The j-z range contains 17 characters, which should be enough.

There will be more instructions dealing with serial input, either using
a TIB as in Forth, or something else.

