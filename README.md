RFOrth - a Forth like language
==============================

2025-10-22 RFO

Starting with some code
-----------------------

RFOrth requires an Arduino with at least 6 KBytes of SRAM, and the example assumes pin 13 is hooked
up to the (onboard) Led.

The code below consumes 131 bytes of heap space to hold the compiled code and dictionary entries.


```
13 CONSTANT Led

: POut (pin --) 
  NATIVE Pin.ModeOut ;
  
: Flash (--) 
  50 1 Led 
  NATIVE Pin.PulseDigitalMs ;
  
: Sleep (ms --) 
  NATIVE Sys.Delay ;
  
: Flashes (count --) 
  => count 
  BEGIN Flash 50 Sleep 
    count 1 sub dup => count 
  AGAIN? ;
  
(generate seq. of 5 flashes)
: Blinks (count --) 
  => count 
  Led POut 
  BEGIN 
    5 Flashes 
    500 Sleep 
    count 1 sub => count
    count AGAIN? ;

(run example)
10 Blinks
```

(...) are comments in Forth. The first comment following the ": something" describes what
is expected to be found on the stack, and if it returns data on the stack, following the "--" separator.

The "=> name" creates or updates a local variable with value from the data stack.


### Case sensitive

Unlike traditional Forth's, this language is case sensitive. That means the word CONSTANT can not
be written "constant", and so on.


Introduction to Forth
=====================

Words
-----
Functions in Forth are called words. Creating a Forth program means creating words. Words call
other words, by putting parameters, if there are any, on the stack, call the word, and pick
up the result, if any, from the stack.

Available words are listed in a dictionary memory structure. It consists of dictionary entries
which point to the word as a string, and to the code of that word, a status code, which redefines
the meaning of the code pointer for some cases, and finally a pointer to the previously defined
word. 

The dictionary is a linked list, and new words are always added to the top. 

Naming
------
Words in Forth do not need to follow classic "identifier" syntax. They can consist of any
sequence of letters, apart from whitespace, and apart from looking like numbers. Special characters
are frequently used in traditional Forth, such as

```
R>
>R
.
,
```

The two first interact with the return stack in traditional Forth. The comma means "copy value from stack,
allocate memory for it, and copy it there", more or less, and the single dot means remove and display top
value from the stack.

Of these four, RFOrth implements the DOT word. The two first are irrelevant due to how local variables
are implemented, and the COMMA, well, maybe some day. There are other central words missing, like CREATE
and DOES>, but that's an advanced topic.

Syntax
------
Forth is a language that has no syntax. Syntax is traditionally defined via words written in Forth, 
implementing control structures. RFOrth has only two control structures, the IF-ELSE-THEN and BEGIN-AGAIN?
for conditionals and loops. These were implemented in RFOrth initially, but were then migrated into
the "firmware", which is found in ACode.txt.

This code is "assembled" offline, and generates an array of bytes, which is patched into the C code,
forming the REPL. This implements the COLON compiler, the mentioned control structures, and a few
other useful words.

Compiled?
---------
Forth is unique in that it is interactive *and* compiled. To define new words in Forth, the syntax
is as follows:

```
: name ... ;
```

Hence, the compiler is normally referred to as the colon compiler. Example:

```
: number 42 ;
```

This creates word called "number". When we call it, it puts the value 42 on the stack.

```
number
.
```

The second line is a single dot ("."), which is a Forth word which takes the top value off
the stack and prints it. It should result in displaying 42.

Interactive
-----------
Forth is an interactive programming language. Usually one connects to the running instance
via a serial terminal. This is the same for RFOrth. 

This gives us the ability to debug code, word by word, in the actual environment, which is
particularly useful on microcontrollers, which is the target system for RFOrth. With a sufficient
*dictionary* of useful words, one can probe and inspect the state, find and fix bugs, and run
parts of the code from the command line, to verify the output.

Until the Pi Pico and its scaled down Python came along, as far as I know Forth was the only
programming environment that could do this on microcontrollers.

Tiny
----
Traditionally, Forth was tiny. Forth systems were constructed using a few kilobytes of
machine code, implementing the REPL, the colon compiler, and a number of standard words
for managing memory, etc. 

RFOrth is not written in real assembly. Instead it runs on a virtual machine defined by
around 60 single byte instructions (plus 128 for representing numeric literals). Short names
have been defined for each of these instructions, together with simple syntax for defining
and looking up tags. This essentially forms a virtual assembly language. 

An assembler program runs through the assembly code (in the file ACode.txt) and
generates byte code, which implements the REPL, colon compiler and some other stuff. It
does this in about 3000 bytes. The low byte count follows not only from the base simplicity
of Forth, but also from the virtual machine design. 

The virtual machine architecture is ... surprise, a stack machine. So the assembly operations
are essentially Forth words. 

Actually, when setting up the default dictionary, which is done in the ACode file, 
all the assembly op's are added as words, which correspond to single byte instructions
instead of pointers to code, as for "normal" Forth words. 

Why the assembly level?
-----------------------
With assembly operations being Forth words, why bother with the assembler to produce byte code,
and not just go straight to Forth.

The answer is both simple and complex. The simple answer it that it was fun doing it this way,
and got me up to speed programming both the assembly language and programming *in* the assembly
language pretty fast. I created an assembler and an interpreter in a scripting language (which
I wrote myself).

The more complex answer is that it is hard to define the colon compiler using a colon, so
it must be defined in an environment where a single colon does not yet have any meaning. I also
wanted as much of the program to be portable. Writing real functionality in virtual assembly
meant the interpreter (both the first one and the C version) would be simpler.

Second, targeting devices with very little RAM, it is important to shuffle as much functionality
away from that RAM. The ACode gets assembled into a byte buffer in C, of type "const", which
means it gets located in Flash when the code is copied to the microcontroller.

Developing in the "assembly" language of the ACode file is pretty close to programming in Forth,
with some exceptions, like no control structures, just jumps to tags, and very primitive
support for local variables. 


In RFOrth, RAM is used for three (?) things, basically.

- Stacks and system buffers
- System status fields
- Compiled Forth words

The third refers to words where we enter a colon definition. When this happens, if the word compiles
correctly, a dictionary entry is allocated, and its next field points to the predefined initial
dictionary, which at runtime resides in flash. 

To make this work, the implementation makes Flash and RAM into a continous address space, after copying
some of the buffers defined towards the end in ACode into RAM, since Flash is basically read only.

Of the around 3k bytes generated from ACode, only about 190 bytes represent dynamic values, and that 
range is copied into RAM when the system boots. 

Simple and complex.


Immediate words
---------------
The control structures such as IF THEN and loops, could be implemented in Forth itself, through a
mechanism called IMMEDIATE. It is really just a status flag in the dictionary entry for a
word, but this single thing is one of the defining characteristics of Forth. Because with it
we can extend the compiler.

The Forth base compiler is very primitive. It consumes a sequence of characters, using space
to identify words, and then basically does the following:

- does the word look like a number, then generate code which puts that number on the stack at runtime
- otherwise look the word up in the dictionary
- if the word is tagged as IMMEDIATE, *call it* - that's right!
- otherwise, generate code to call the word

This is the RFOrth compiler. Traditional Forth allows words to look like numbers, and instead
saying that only if dictionary lookup fails, if it is a number, then generate code that
pushed it on the stack at runtime. Basically moving the first point last. 

The power of IMMEDIATE words is immense! 

For example, defining a word IF and making it IMMEDIATE, means that when we compile code,
and come across an IF, we call code which may not only generate some code, but also modify
the compiler state, or introduce new state variables, lists and stacks.

We then create the word THEN, which is also IMMEDIATE. Its code can then pick up on the state
and data saved by the IF, and so manage conditional and unconditional jumps forwards and
backwards in the output of the compiler.

IF THEN
-------
The notation for conditionals in Forth is a bit special, since it is a stack language.

```
<cond> IF ... (if true) ... THEN ... 
<cond> IF ... (if true) ... ELSE ... (if false) ... THEN ...
```



Implementation
==============

Bytecode virtual machine
------------------------
RFOrth is an experiment, and a toy language, implemented as a virtual machine 
with an invented assembly language. The file ACode.txt forms a "firmware" level,
by implementing the necessary elements to run a Forth-like language.

The virtual machine executes *bytecode* generated both by the Assembler (a script) and
by the internal compiler of the language.


Stack language
--------------
Like traditional Forth, RFOrth is also stack oriented. As with Forth,
it is a language without syntax, everything is a word. Words are separated
by whitespace. RFOrth supports immediate words, which augment the compiler,
and it uses a dictionary.

```
2 3 add 4 mul .
```

Should print 20.

The "." word means: print top value from the stack as signed decimal followed by a space.
It is implemented in ACode.txt under tag :DOT and linked into the static initial dictionary
which starts at :Dictionary tag.



The stacks
----------

RFOrth has four stacks, but as a programmer we deal only with one.

That's the *data stack*. Sometimes it is called parameter stack. This is where
we put parameters to words, and get values back from words. Programming in
Forth we interact with the stack all the time.

### Those we don't need to care about

The second stack, is named the *call stack* in RFOrth. In traditional Forth's it
is called the return stack. When a word calls another word, the return address
is pushed on this stack. It is also used, both in traditional Forth and in RFOrth
or local variables. In RFOrth, local variables are fully handled in an abstracted
way by the colon compiler, so there should be very little need to interact with this
stack in code. 

The third stack, which we also don't need to care about, is the *frame stack*. It keeps
track of invocation frames on the call stack. Whenever defining a new local variable,
it registers on the frame stack, so that when we return from the current word, it
knows how to find the return address in the code that called the word. There are no
words in RFOrth to manipulate this stack; it is handled by system words call and ret, as
well as cpush, which is used to define a new local variable on the call stack.

The fourth and final stack (so far) is the *compile stack*. It is used when compiling
code, to keep track of forward and backward jumps within the code for a word, like
conditionals and loops. 

At runtime, the three first stacks are managed by the C code, while the fourth is 
managed by the ACode.txt, which is where the colon compiler is defined.


First use
---------

```
?		;; list words in dictionary (can also use "words")
.s		;; show stack content
clear		;; clear stack
NATIVE ?	;; list native words
```


The assembly level
------------------
The RFOrth REPL is written in a virtual assembly language, which is run through an Assembler
script (written in [CFT](https://github.com/rfo909/CFT)), which in turn outputs binary data.
These are interpreted by a C program that runs on Arduino. 

The ACode.txt file is the "assembly" base code, also referred to as "firmware", because most of
it recides in Flash when put onto an Arduino. It implements the REPL, the COLON compiler
and a few more key features available in Forth.

Note that all the "assembly" instructions are directly available to RFOrth.

See the [Instruction set](InstructionSet.md).

Note that I prefer names over symbols, so where a traditional Forth would use the words + - * /,
I use add, sub, mul and div. This makes code more readable to me. 

```
+	add
-	sub
*	mul
/	div

==	eq
!=	ne
>	gt
<	lt
>=	ge
<=	le
!	not	(logical)

&&	and	(logical)
||	or
	
&	andb	(bitwise)
|	orb
```


Non-standard
------------
The RFOrth implementation has no intention of following Forth standards. It is a toy
language, for discovering what can be done with some stacks and bytecode, but I still
plan to put it to real use.

It currently (2025-10) does not implement the CREATE ... DOES> but it still can do the same
by means of reading the &IsCompiling status byte, and depending on its value do
one thing at compile time, and another at runtime. 

See implementation of the NATIVE word for an example at tag :NATIVE in ACode.txt.

Interactive
-----------
As usual with Forth, RFOrth implements a REPL (read-eval-print-loop), which takes input, processes
it and repeats. 

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

The colon must be space separated from the name of the new word and the colon definition
is terminated by a semicolon. I mention this, because in ACode.txt the colon is *not* separate
from the name, because in that file, those are *tags* for the Assembler, not invocations of
the colon compiler. The colon compiler is initiated at tag :COLON in ACode.

In ACode the returning from a function is done with the assembly code "ret". The SEMICOLON word
in Forth calls "ret" as well, after doing various housekeeping.

The colon compiler sets a flag at address &IsCompiling, which affects how each word following
the name of the new word, up to the semicolon, is processed. 

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
write ACode.txt, are also available in the RFOrth language. 

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

RFOrth uses 16 bit (data) words (called "cells" in normal Forth), and though it could easily address
real memory and registers on the smaller 8-bit architectures, it is not capable of directly
creating 32-bit values. 

If or when physical acccess to register bits and SRAM, new bytecode ops will have to be introduced, 
which for wider architectures may comprise a base register plus an offset value to produce
32 bit addresses. 

### NATIVE = written in C

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

This is also the case for the &CompileBuf, &NextWord and &LVBuf buffers, which are used by
the colon compiler, and are available for other uses at runtime. The notation for addresses
is the same as used in ACode, but those available for Forth are explicitly defined in the
Dictionary. 

String literals are written in two ways in RFOrth, using single or double quotes; the
difference is that with double quites, the underscore character get replaced by space.

To print a string, use the ".str" word. 

```
cr "Welcome_to_this_program .str
```

The "cr" word means carriage return. It is Forth tradition to do carriage return before
printing something instead of after.

### Available buffers

At runtime, when executing code (not compiling), the following buffers are available for general
use: 

- &CompileBuf
- &LVBuf
- &NextWord

Each is fitted with an end-tag to calculate available bytes:

```
&CompileBufEnd &CompileBuf sub .   (64)
&LVBufEnd &LVBuf sub .             (32)
&NextWordEnd &NextWord sub .       (32)
```

For additional flexibility, all data fields related to the interpreter and compiler,
are placed next to each other in memory. To use all this memory as a single buffer,
called &AllBuffers:

```
&AllBuffersEnd &AllBuffers sub .    (180)
```

Allocating memory
-----------------
As in regular Forth, RFOrth uses the word "HERE" to return the next address on the heap,
and "allot" to allocate bytes of heap.

Differing from regular Forth, RFOrth does not allow memory access at HERE or beyond,
so memory must be allot'ed before used. The "allot" word just advanced the HERE value,
making new memory available for use.

Note a little piece of subtlety that cost me half an hour of serious doubts. If one wants
to allocate a 20 byte buffer, and store a reference to it as a constant, it would seem one 
could do this:

```
HERE CONSTANT buf
20 allot
```


It seems logical that storing the value of HERE in a constant and then advancing the HERE by
allocating 20 bytes, should work just fine.

### This crashes horribly!

The problem is that the "CONSTANT" word also allocates memory to create a new
dictionary entry, and the value of HERE that gets stored as the constant "buf"
now refers to that dictionary entry. Writing to it corrupts the top dictionary entry,
and with it the link to the rest of the dictionary. 

The correct way is as follows:

```
HERE 20 allot CONSTANT buf
```

:-)


Global variables and constants
------------------------------
We saw constants in action above. Global variables follow a similar pattern.

```
"Hello CONSTANT greeting

0 VARIABLE x
```

Note that all variables and constants, as well as all stack content, are
data words (2 bytes), not bytes. 

The only time we operate
on bytes, is when using "readb" or "writeb" to read or write single bytes.

To retrieve the value of a constant, just enter the constant name; that will push
the value of the constant on the stack. 

For variables, entering the name of the variable pushes a "constant" on the stack as well,
but this time it is an *address* to where the data is stored, which can then be read and 
written.

```
"Hello CONSTANT greeting
greeting .

0 VARIABLE x
x @ 			;; read variable
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

### Structure

These words add a certain syntax to Forth, and work as follows:

```
: max (a b -- n)
  => b => a
  b a gt IF
    b
  ELSE
    a
  THEN
;
```
The condition comes first, followed by IF, and then comes the code that executes if
the condition is true. Then comes ELSE and the else-code, and finally the THEN word,
which indicates the end of the structure, like "... and then we continue with ..."

The ELSE is optional, can also use just IF ... THEN.

```
: count (max -- )
  => max 
  0 => counter
  BEGIN
    cr counter .
    counter 1+ => counter
    counter max lt AGAIN?
  ;
```

The word "AGAIN?" is a condition jump back to BEGIN. The condition is that counter is
less than max. 

### Break?

The BEGIN ... AGAIN? currently (October 2025) is the only loop construct, and it does
not support breaking out of the loop other than supplying false to the AGAIN? word.

There is a way somewhat around this, which is the assembly instruction "ret", which means
return from current word. Granted, it's not the same as a break, which would imply 
continuing code after the AGAIN?, but since words should be kept short and do small
incremental stuff towards an end-goal, doing much after an AGAIN? isn't necessarily
a good idea.

```
: someWord
  ...
  x y eq IF ret THEN
  ...
  ;
```



Extending the compiler
----------------------
As in regular Forth, immediate words are used to extend the compiler. To make
a word immediate, include the word IMMEDIATE anywhere *inside* the colon definition. Not
after as in normal Forth, but inside. 

The COLON word initiates compile mode, by setting a state variable at &IsCompiling,
which affects how words are compiled, until hitting the SEMICOLON word, which terminates
the compile mode, creates the word and cleans up. The IMMEDIATE words changes a state variable
which is picked up by the SEMICOLON word, when adding the new word to the dictionary.


Strings
-------
The notation for string constants is as follows:

```
'Name
"Your_name
```

The difference is that the second form replaces underscores with space. 

### String compare?

There currently exists only one string operation, which compares to strings
and returns 1 if they are equal, otherwise 0. It is an assembly level function.

```
"test 'test streq .
```

This should print a 1
