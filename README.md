RFOrth v2
---------
v0.0.0a

RFOrth v2 is a Forth bytecode compiler/interpreter, with local variables 
instead of DUP and SWAP and ROTATE and all that stuff.

The aim is to create a fully working Forth for the Raspberry Pi Pico, and also
to experiment with features such as

- struct's
- lists
- objects
- hierarchical dictionaries / dotted lookup


Basics
------

The language uses a cell size of 2 bytes, although aiming at running on a 4 byte
architecture. The data stack and local variables are 4 bytes, while the heap
space (which is a single malloc in C) uses two byte references, allowing for
a max heap of 64k.

All symbols in the language, that is, all words, but also static strings, are
stored in a *symbol storage* inside the heap. This means all words get reduced
to a unique two byte reference. 

When compiling, we don't do *strcmp* but instead compare integer values representing
different symbols, such as the COLON and SEMICOLON etc.

We use the word Ref to talk about references inside the designated heap, as
they are two byte unsigned integers. 

The parser is modified into Recursive Descent, instead of the flat model depending
on stacks. 

The main loop interacts with the TIB (Terminal Input Buffer) which is a circular
buffer managed in C, via a number of TIBx words in Forth. Examining characters
forward from current position may block until new input comes in (over serial or
otherwise).

Status
------

A "pretend assembly" language is getting finalized. It contains simple system calls,
like CALL, RETURN, ADD, SUBTRACT etc, as well as primitives for processing words
ahead, via the TIB.

Local variables are going to be stored on a new tempStack, which allocates six bytes
per variable, two referring to the name, and four for the value. The point of storing
the names, is both for lookup when setting or getting values, and second to provide
breakpoints where variables can be inspected.


