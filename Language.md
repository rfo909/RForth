RFOrth - a Forth like language
==============================

2025-10-21 RFO

Bytecode virtual machine
------------------------
RFOrth is an experiment, and a toy language, implemented as a virtual machine 
with an invented assembly language. The file ACode.txt forms a "firmware" level,
by implementing the necessary elements to run a Forth-like language.

The virtual machine executes *bytecode* generated both by the Assembler (a script) and
by the internal compiler of the Forth-like language.


Stack language
--------------
Like traditional Forth, RForth is also stack oriented. As with Forth,
it is a language without syntax, everything is a word. Words are separated
by whitespace. RFOrth supports native words, which augment the compiler,
and it uses a dictionary.

```
2 3 add 4 mul .
```

Should print 20 (the "." word means print top value from the stack)


The stacks
----------

RFOrth has three major stacks, plus one very internal. 

The main stack is the *data stack*. Sometimes it is called parameter stack. This 
is where we put parameters to words, and get values back. As programmers we interact
directly with the data stack all the time.

Then there is the *call stack*, which is usually called the return stack.

When one word calls another, we push the return address here, but in addition,
it also contains local variables. It can be managed manually, but rarely will be.

The third major stack, is a system stack, called *frame stack*. It keeps track of how many
local variables have been added to the call stack, for each word invocation. There exist
no primitives in Forth to interact with it.

The fourth stack, which is considered internal, is a compile stack. It is used to
implement control structures, such as IF THEN and BEGIN AGAIN?.

The assembly level
------------------
The RFOrth REPL is written in a virtual assembly language, which is run through an Assembler
script (written in CFT), which in turn outputs binary data. These are interpreted by a C program
that runs on Arduino, but can also run with an Interpreter script, in order to step through code,
view memory, etc. 

The ACode.txt file is the "assembly" base code, also referred to as "firmware", because most of
it resices in Flash when put onto an Arduino. It implements the REPL, the COLON compiler
and a few more key features available in Forth.

But also, all the "assembly" instructions are directly available to RFOrth.

See the [Instruction set](InstructionSet.md).

Note that I prefer names over symbols, so where a traditional Forth would use the words + - * /,
I use add, sub, mul and div. This goes for most of the assembly instructions, as it makes
code more readable to me. 

Non-standard
------------
The RFOrth implementation has no intention of following Forth standards. It is a toy
language, even as I plan to use it on microcontrollers.

It currently (2025-10) does not implement the CREATE DOES> but it still can do the same
by means of reading the &IsCompiling status byte, and depending on its value do
one thing at compile time, and another at runtime. See the ACode.txt implementation
of the NATIVE word for an example.

Interactive
-----------
As usual with Forth, RFOrth implements a REPL (read-eval-print-loop), which takes input, processes
it and repeats. In Forth the "print" part is implemented as the DOT word (".")

```
3 5 add .
```
This prints 8

Compiled
--------
As in normal Forth, RFOrth has a colon compiler, which lets us define new words.
```
: NewWord .... ;
```

The colon must be space separated from the name of the new word, and the colon definition
is terminated by a semicolon. 

The colon compiler sets a flag at address &IsCompiling, which affects how each word following
the name of the new word, and the semicolon, is processed. 

### Compile mode

- If it is a number, generate code that pushes the number on the data stack
- If it is a local variable, generate code to push the variable value (to the data stack)
- Look the word up in the dictionary - if not found produce error
- If it is an assembly instruction, inline a single byte
- If it is an immediate word, call it
- Otherwise create code for a call to that word

### Interactive mode

- If it is a number, push it on the data stack 
- Look the word up in the dictionary - if not found produce error
- If it is an assembly instruction, execute it
- Execute the word


Close to assembly
-----------------
Forth to me is like some hefty macro assembler. Words are the macros, and much of base code
is assembly. So also in RFOrth, where the virtual "assembly" operations, which are used to
write ACode.txt is also available in the RFOrth language. 

This means words like "add" are really assembly words, that correspond to single byte
operations. 

Hardware - NATIVE functions
---------------------------
Traditionally Forth runs very close to the hardware, interfacing registers and physical
memory.

RFOrth on the other hand, lives in a virtualized and somewhat protected environment. Its "RAM" is
an array of bytes in C.

Its address space combines a readonly flash part, which is the result
from assembling ACode, and the SRAM part, which is read/write.

RFOrth uses 16 bit words (called "cells" in normal Forth), and though it could easily address
real memory and registers on the smaller 8-bit architectures, it is not capable of directly
creating 32-bit values. 

If or when physical acccess to register bits and SRAM, new bytecode ops will have to be introduced, 
which for wider architectures may comprise a base register plus an offset value to produce
32 bit addresses. 

For now, the only interface to hardware goes through the NATIVE interface. Native functions
are written in C, and are listed in a table in the C code. These have access to the stacks, 
for grabbing parameters, and for leaving return values to the RFOrth runtime. 

Currently native words have been created for Pin control and the i2c ("Wire") library on
Arduino.

To list all native words, run the native word called "?" - it lists all native words
available, with descriptions

```
NATIVE ?
```

Read/write
----------
RFOrth has four assembly instructions for reading from and writing to the Forth heap, which
is that array of bytes defined in C. 

```
value addr writeb      ;; write byte
addr readb             ;; read byte

value addr !           ;; write word-sized value (2 bytes) 
addr @                 ;; read word-sized value
```

Values
------
All values on the data stack are *words*, which in RFOrth means 2 bytes. The REPL recognizes
number literals on the following formats:

```
0xCAFE			;; hex
b1100101011111110	;; binary
51966			;; decimal unsigned
-13570			;; decimal signed
```

Similarly, there are four ways to print values off the stack:

```
(value) print		;; print as hex
(value) printb		;; print as binary
(value) print#		;; print unsigned decimal
(value) print#s		;; print signed decimal
```


Buffers and strings
-------------------
Strings are stored in buffers, up to 255 bytes of length. The first byte is the length.

This is also the case for the &CompileBuf and &NextWord buffers, which are used by
the colon compiler, and are available for other uses at runtime.

String literals are written in two ways in RFOrth, using single or double quotes; the
difference is that with double quites, the underscore character get replaced by space.

To print a string, use the ".str" word. 

```
cr "Welcome_to_this_program .str
```

The "cr" word means carriage return.



Allocating memory
-----------------
As in regular Forth, RFOrth uses the word "HERE" to return the next address on the heap,
and "allot" to allocate bytes of heap.

Differing from regular Forth, RFOrth does not allow memory access at HERE or beyond,
so memory must be allot'ed before used.

```
HERE 20 allot CONSTANT buf
```

Note that the ordering is important here. The following fails horribly:

```
HERE CONSTANT buf
20 allot
```

The reason is that the "CONSTANT buf" also allocated memory to create a new
dictionary entry, and the value of HERE that gets stored as the constant "buf"
now refers to that dictionary entry.


Global variables and constants
------------------------------
We saw constants in action above. Global variables follow a similar pattern.

```
"Hello CONSTANT greeting
0 VARIABLE x
```

There is no differing between bytes and word values; all constants, variables,
and stack entries, are word-sized values (2 bytes). The only time we operate
on bytes, is when using "readb" or "writeb" to read or write single bytes.

To read and update a variable:

```
0 VARIABLE x
x @			;; read variable
5 x !			;; write variable
```


Control structures
-------------------
```
bool IF ... THEN ...
bool IF ... ELSE ... THEN ...
BEGIN .... bool AGAIN?

: example ( -- ) 
  (prints numbers 0-9)
  0 => count
  BEGIN
     cr count .
     count 1+ => count
     count 10 lt 
     AGAIN?
     "done .str
  ;
```

Extending the compiler
----------------------
As in regular Forth, immediate words are used to extend the compiler. To make
a word native, include the word NATIVE anywhere *inside* the colon definition. Not
after as in normal Forth, but inside. 

The COLON word initiates compile mode, by setting a state variable at &IsCompiling,
which affects how words are compiled, until hitting the SEMICOLON word, which terminates
the compile mode, creates the word and cleans up.


