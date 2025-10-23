RFOrth - a Forth like language
==============================

2025-10-22 RFO

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

This code consumes 131 bytes of heap space to hold the compiled code and dictionary entries.

RFOrth is currently being developed for the Arduino Nano Every. It has 6 KBytes of SRAM. The code can
be made to run on less, possibly even the original Nano and Uno's with 2 KBytes of SRAM only, by
adjusting the Forth heap size, defined in Constants.h. The example above also assumes that pin 13
is hooked up to the onboard Led.

The "=> name" creates or updates a local variable with value from the data stack. Locals are pushed
on the *call stack* which corresponds to the return stack of other Forth implementations. It can be
managed manually, but there's no point with explicitly named local variables.

### Case sensitive

Unlike traditional Forth's, this language is case sensitive. That means the word CONSTANT can not
be written "constant", and so on.


Important words
---------------

```
?           ;; list words in dictionary (can also use "words")
.s	        ;; show stack content
clear       ;; clear stack
NATIVE ?    ;; list native words
```


Introduction to Forth?
----------------------

If the above example makes little or no sense, perhaps read my [Introduction to Forth](ForthTutorial.md).



Implementation
==============

Cells
-----
RFOrth uses a 2-byte cell size, which is the size of all stack values. The same goes for variables
and constants; those are also cell sized values. 

Variables are constants that point to an allocated cell containing the value, as I believe is how
it works in other Forth's as well.


Bytecode virtual machine
------------------------
RFOrth is an experiment, and a toy language, implemented as a virtual machine 
with an invented assembly language. The file ACode.txt forms a "firmware" level,
by implementing the necessary elements to run a Forth-like language.

The virtual machine executes *bytecode* generated both by the Assembler (a script) and
by the internal compiler of the language.


The data stack
--------------

RFOrth has four stacks, but as a programmer we deal only with one, which
is the *data stack*. Sometimes it is called parameter stack. This is where
we put parameters to words, and get values back from words. Programming in
Forth we interact with the stack all the time.

The other stacks
----------------

The second stack, is named the *call stack* in RFOrth. In traditional Forth's it
is called the return stack. When a word calls another word, the return address
is pushed on this stack. It is also used, both in traditional Forth and in RFOrth
or local variables. In RFOrth, local variables are fully handled in an abstracted
way by the colon compiler, by name, so there should be very little need to interact with
this stack in code. The operations for doing so are cpush, cget and cset. 

The third stack is the *frame stack*.

It keeps track of invocation frames on the call stack. Whenever defining a new local variable,
it registers on the frame stack, so that when we return from the current word, it
knows how to find the return address in the code that called the word. There are no
words in RFOrth to manipulate this stack; it is handled by system words call and ret, as
well as cpush, which is used to define a new local variable on the call stack.

The fourth and final stack (so far) is the *compile stack*. It is used when compiling
code, to keep track of forward and backward jumps within the code for a word, like
conditionals and loops. 

At runtime, the three first stacks are managed by the C code, while the fourth is 
managed by the ACode.txt, when compiling certain immediate words like conditionals and loops.




The assembly level
------------------
The RFOrth REPL is written in a virtual assembly language, which is run through an Assembler
script (written in [CFT](https://github.com/rfo909/CFT)), which in turn outputs binary data.
These are interpreted by a C program that runs on Arduino. 

The ACode.txt file is the "assembly" base code, also referred to as "firmware", because most of
it recides in Flash when put onto an Arduino. It implements the REPL, the COLON compiler
and a few more key features available in Forth.

Note that all the "assembly" instructions are directly available to RFOrth, via the dictionary,
as are some select functions and addresses from ACode.

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
program the "firmware" (ACode.txt), are also available in the RFOrth language. 

This means words like "add" are really assembly words, that correspond to single byte
operations. 

Forth makes no distinction between different types of words. Be it a library function, an
assembly token, or a Forth word you just created. As long as code interacts via the stack
and has a known name, Forth can use it.


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

If or when physical acccess to register bits and SRAM becomes desirable, new bytecode ops will have 
to be introduced, which for wider architectures may comprise a base register plus an offset value to
produce 32 bit addresses. 

### NATIVE = written in C

For now, the only interface to hardware goes through the NATIVE interface. Native functions
are written in C, and are listed in a table in the C code. These have access to the stacks, 
for grabbing parameters, and for leaving return values to the RFOrth runtime, as well as the 
Forth heap.

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

value addr !           ;; write cell value (2 bytes) 
addr @                 ;; read cell value
```

Values
------
All values on the data stack are *2 byte cells*. 

The REPL recognizes number literals on the following formats:

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
Strings are stored in buffers, up to 256 bytes of length. The first byte is the length, followed
by up to 255 single byte characters. 

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
```

### Examples

```
: max (a b -- n)
  => b => a
  b a gt IF
    b
  ELSE
    a
  THEN
;

: count (max -- )
  => max 
  0 => counter
  BEGIN
    cr counter .
    counter 1+ => counter
    counter max lt AGAIN?
  ;
```

The word "AGAIN?" is a conditional jump back to BEGIN.

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


Custom dictionaries
-------------------
RFOrth lets us define custom dictionaries.

```
Dict Accumulator
Accumulator DictUse

0 VARIABLE count

: add (n --) count @ add count ! ;

DictClear
```

The first line creates a dictionary and stores in on the global dictionary under the name Accumulator.
We then call the word, which puts a pointer on the stack, and call DictUse.

It puts the link to the Accumulator dictionary into a special field for one "extra" dictionary, which
if defined, is traversed first, when looking for a symbol. 

We then go ahead and define content as usual, before calling DictClear, which nulls the "extra" dictionary field.

Listing the words with "?" will now show the "Accumulator" word only.

### Reason

The point of separate dictionaries is two fold. It frees us from inventing unique names, and in doing
so it helps avoid polluting the global dictionary name space. 

### View

To view the content of a custom dictionary, just use it with "DictUse", then type "?" - the "extra" dictionary
is listed, followed by a separator and then the global dictionary. 

### Use in code

RFOrth implements an immediate word "->" which takes the two following words from the input stream
as the name of the dictionary, and the word inside.

```
10 -> Accumulator add    (calls the "add" word inside the custom dictionary, NOT the addition op)
-> Accumulator count @ . (should now print 10)
```

This code works both interactively and from inside compiled words. 

### No call overhead

The "->" word is implemented in ACode (tag :->), where it just identifies the dictionary entry within the
custom dictionary, then leaves the processing to the same &ExecuteWord that is used elsewhere. 

It is tagged as an IMMEDIATE word in the Forth initial dictionary, near the end of ACode.txt.

What this means is that there is no overhead calling a word inside a dictionary; the reference is 
resolved fully at compile time, just as for words in the global dictionary.

```
: showAcc (--) -> Accumulator count @ . ;
```

Type checking
-------------
Hah hah hah ... :-)

Seriously though, the system enforces some pointer validation for the read/write operations, ensuring
we can not access memory above HERE. Also, since custom dictionaries are constants, the DictUse word
actually checks that the word given as name of dictionary, refers to a constant. 

Other than that: nope!

This is firmly within the Forth tradition, I think!
