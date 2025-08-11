Forth never ceases to fascinate
================================

Version 3

2025-08-11 RFO

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the toolchain and the deploy is so easy. 

It is unlikely this Forth will ever be used for anything but testing
and development.

The initial design comprised making a byte code assembler, and compiling 
the code in the ACode.txt file, which is the base code. The compiled
byte code is kept a printable string, for copy-paste into the
interpreter. 

Over time the goal is to extend the base code into supporting dictionaries
and a Forth engine running on top of the base code.

A major milestone was getting the "call" and "ret" quasi instructions
up and working. They are not real instructions, but instead represent
inlining a number of instructions (7 each). We manage our own call
stack, which is also the home for local variables inside functions,
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

A CFT interpreter will also need to be created, in order to step through
the byte code, in order to verify the assembler, and as a measuring stick
for the C code interpreter.

Going Forth
----------- 

Later, we create Forth dictionaries and Forth words out of the 
low level ("assembly") functions, and the capability of defining new ones,
compiling them to "assembly". At this point, after sending the base
byte code, we can send Forth word definitions, and talk interactively
with it over serial. 

If this doesn't sound like fun, I don't know what will! 

Third attempt ...
-----------------

The previous attempt(s) were bogged down in assembly complexities,
which I think are solved now, by introducing the xNNN number literals
as singlebyte instructions, and of course the assembler supporting
local tags within functions.

The handling of symbols ('xxx) by the assembler, as well as the
data and call stacks, feels like an improvement, compared to previous
attempts at defining as much as possible within the bytecode. It got 
very complex and would have been quite inefficient, even though the
Pico has lots of horsepower.

Currently the number of instructions is 33 + 17 (for numeric literals), giving a total 
of 50. We have used printable characters A-Z0-9ghi. The a-f range is free, but there exist
quasi instructions for up to 6 local variables a-f. Waiting with allocating those as
byte codes. The j-z range contains 17 characters, which should be enough.

```
   0: !
   1: @
   2: HERE
   3: PC
   4: add
   5: allot
   6: and
   7: cbase
   8: cmark
   9: cpop
  10: cpush
  11: cr
  12: div
  13: dup
  14: eq
  15: gt
  16: halt
  17: jmp
  18: jmp?
  19: lt
  20: memcpy
  21: mul
  22: ne
  23: not
  24: or
  25: print
  26: readb
  27: show
  28: sub
  29: swap
  30: tPtr
  31: tSymbol
  32: writeb
```

There will be some additional instructions dealing with serial input, either using
a TIB as in Forth, or something else.
