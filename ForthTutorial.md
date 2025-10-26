Introduction to Forth
=====================

2025-10-26

Stack language
--------------
Forth (and RFOrth) are stack languages. This means they do things differently. Instead of

```
2+3
```
which is how expressions are written in most other languages, Forth uses *postfix* notation,
where each element either puts something on a stack, or takes something from it, doing something
with it, and putting the result back.

```
2 3 add
```

First the value 2 is pushed on the stack. Then the value 3. The stack now looks like this

```
3
2
```

The operation "add" is defined as: take the two topmost values off the stack, add them together
and push the result on the stack. The result:

```
5 
```



Words
-----
Functions in Forth are called words. Creating a Forth program means creating words. Words call
other words, by putting parameters, if there are any, on the stack, call the word, and pick
up the result, if any, from the stack.

Available words are listed in a dictionary memory structure. It consists of dictionary entries
which point to the string representation for the word, to the code of that word, a status code, which redefines
the meaning of the code pointer for some cases, and finally a pointer to the previously defined
word. 

The dictionary is a linked list, and new words are always added to the top. 

To list all words on the dictionary, type a "?" and press Enter.

```
?

! .str 1+ << >> @ CELL+ HERE PANIC PC add allot and andb atoi call cellsize 
cforce cget clear cpush cr crget cset div drop dump dup eq ge gt halt 
inv jmp jmp? le lt memcpy mul n2code native nativec ne not null or orb 
over print print# print#s printb printc rback? readb readc ret rfwd? streq 
sub swap u2spc writeb . IF THEN ELSE BEGIN AGAIN? CONSTANT VARIABLE NATIVE 
Dict DictUse DictClear IN .W >>str ?C CELL CELLS CREATE , DOES> BufReset 
BufAdd ShowBuffer BufCopy BufCreateCopy EmitNumber EmitByte GetNextWord 
PreCompile PostCompile SetCompilingWord &DictionaryHead &DebugFlag &CompileBuf 
&CompileBufEnd &LVBuf &LVBufEnd &NextWord &NextWordEnd &AllBuffers &AllBuffersEnd 
&IsCompiling &CompilingWordMode &CompilingWord ? .s words : => ; IMMEDIATE  
```


Naming
------
Word names in Forth do not need to follow classic "identifier" rules. They can consist of any
sequence of letters, apart from whitespace, and apart from looking like numbers. The following
are examples of valid word names

```
:
.
;
R>
DOES>
my:something
x/y-%
name#
```


Comments
--------
Text written inside parantheses is considered a comment in Forth:

```
(this word does so and so)
: myWord  ... ;
```

It is customary to follow the new word with a special comment that identifies
the values that the word consumes from the stack, followed by two dashes, and then
the values it puts back. It is still just a comment, but very useful.

```
: times (count string -- string) ... ;
```

Syntax
------
Forth is a language that has no syntax. It is just a stream of words separated by space. 

What syntax there is, is defined by creating words, written in Forth, implementing control structures. 

RFOrth has only two control structures, the IF-ELSE-THEN and BEGIN-AGAIN?
for conditionals and loops. These were implemented in RFOrth initially, but were then migrated into
the "firmware", which is found in ACode.txt, mainly to save precious RAM.


Compiled?
---------
Forth is unique in that it is both *interactive and compiled!* To define new words in Forth, the syntax
is as follows:

```
: name ... ;
```

This is called a colon definition, and the colon initiates the compile mode, and is called
the colon compiler. Example:

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


Memory
------
Forth has system words for managing memory. Traditionally memory is organized in terms of
two sizes, the CELL and single bytes. 

The RFOrth CELL size is 2 bytes; on other Forth's it may differ. The point of the CELL definition
is that this is the size of addresses, which in turn defines the addressable space. 

### Read and write

```
<value> <addr> !            (write cell value to address)
<addr> @                    (read cell value from address)

<value> <addr> writeb       (write byte value to address)
<addr> readb                (read byte value from address)
```

The values that are read from memory are put on the data stack.

### Constants and variables

Forth lets us define both constants and (global) variables. Their names are put onto the 
dictionary, but practical use differs a little:

```
42 CONSTANT const
42 VARIABLE var
```

To use the value of a constant, just refer its name:

```
const .
```

This will print 42.

The variable is different, as it is really a constant as well, but the constant value is
the address of a location in memory where the dynamic value is stored. To read and update it,
we use the words for read and write:

```
var @ .       (prints 42)
55 var !      (changes variable value)
var @ .       (prints 55)
```

### Allocating memory

All Forth's implement two words for managing memory, specifically for allocating data from
the heap.

```
HERE         (returns the current top of the heap)
<n> allot    (allocate n bytes)
```

In RFOrth, the HERE value is the next available byte on the heap. It grows upward,
and RFOrth enforces a simple memory protection, so that trying to read or write to addresses 
starting from the HERE value, is detected and fails with an error. 

The "allot" word simply increases the HERE value, up until all available heap space is 
used.

Example, building a linked list (stack) of values

```
0 VARIABLE ListHead

: listAdd (value -- ) 
  (store parameter in local variable)
  => value

  (store HERE into data local variable)
  HERE => data

  (allocate 2 cells worth of data - advances HERE by 4 bytes)
  2 CELLS allot

  (write value into the first cell)
  value data !

  (write ListHead value into second cell - this is the next-pointer)
  ListHead @
    data CELL+ !

  (store pointer to the new list element in ListHead variable)
  data ListHead !
;

: showList (--)
  ListHead @ => ptr
  BEGIN
    ptr 0 ne IF 
      cr ptr @ . 
      ptr CELL+ @ => ptr
    THEN
    ptr AGAIN?
;

4 listAdd
5 listAdd
showList
```

    


Interactive
-----------
Forth is an interactive programming language. Usually one connects to the running instance
via a serial terminal. This is the same for RFOrth. 

The interactive interface is usually just referred to as the REPL (read, execute, print, loop).

This gives us the ability to debug code, word by word, in the actual environment, which is
particularly useful on microcontrollers, which is the target system for RFOrth. With a sufficient
*dictionary* of useful words, one can probe and inspect the state, find and fix bugs, and run
parts of the code from the command line, to verify the output.

Until the Pi Pico and its scaled down Python came along, as far as I know Forth was the only
programming environment that could do this on microcontrollers. LISP is of course also interactive, 
but I don't think it fits into typical small microcontrollers with a few KBytes of RAM.

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

The answer is both simple and complex. 


### Simple (the real reason?)

The simple answer it that it was *fun* doing it this way, and got me up to speed programming both
the assembly language and programming *in* the assembly language pretty fast. I created an assembler 
and an interpreter in a scripting language (which I wrote myself). It allows me to step through code,
inspect simulated memory, set break points etc. 

The Interpreter script has been invaluable for debugging the ACode, in a way an actual C implementation
running on an actual microcontroller can hardly be.

### Complex (also important)

The more complex answer is that it is hard to define the colon compiler using a colon, so
it must be defined in an environment where a single colon does not yet have any meaning. Of course
it could have been written in C, but I also wanted as much as possible of the system to be 
portable. Writing real functionality in virtual assembly meant the interpreter (both the first one,
and the C version) would be simpler.

The interpreters don't know about Forth words at all; they handle stacks and address space, and execute the
single byte instructions of code. Yes, it is slower than implementing the REPL and compiler in
C, but it doesn't really matter much. What does matter is execution speed when running the compiled
 application code. Besides, as was mentioned above: great fun!!

Second, targeting devices with very little RAM, it is important to shuffle as much functionality
away from that RAM. The ACode gets assembled into a byte buffer in C, of type "const", which
means it gets located in Flash when the code is copied to the microcontroller. 

(Side note: some
Arduinos require such data to be both "const" and marked with "PROGMEM", but for the Nano Every, 
that combination fails, and "const" alone works perfectly)

Developing in the "assembly" language of the ACode file is pretty close to programming in Forth,
with some exceptions, like no control structures, just conditional and unconditional jumps to tags,
which represent addresses, and very primitive support for local variables. 

### RAM

In RFOrth, RAM is used for three things, basically.

- Stacks and system buffers
- System status fields
- Compiled Forth words: code and dictionary entries

The third refers to words where we enter a colon definition. When we compile the very first colon
word, if it compiles correctly, a dictionary entry is allocated, and its next field points to the
predefined initial dictionary, which at runtime resides in flash. 

To make this work, the C implementation makes Flash and RAM into a continous address space, after copying
the buffers and state variables defined in ACode into RAM (some 190 bytes), as the Flash part of the address
space is read only.

Simple and complex.


Immediate words
---------------
The control structures such as conditionals and loops, can always be implemented in Forth itself, through a
mechanism called IMMEDIATE. This is really just a status flag in the dictionary entry for a
word, but this single thing is one of the defining characteristics of Forth. Because with it
we can extend the compiler.

The Forth base compiler is very primitive. It consumes a sequence of characters, using space
to identify words, and then basically does the following:

- if the word looks like a number, then generate code which puts that value on the stack at runtime
- otherwise look the word up in the dictionary
- if the word is tagged as IMMEDIATE, *call it* - that's right!
- otherwise, generate code to call the word

This is the RFOrth compiler. Traditional Forth allows words to look like numbers, and instead
saying that only if dictionary lookup fails, if it is a number, then generate code that
pushed it on the stack at runtime. Basically moving the first point last. 

I suspect the reason for that ordering, is one of efficiency, not that it is useful defining words
that look like numbers. 

The power of IMMEDIATE words is immense! 

### IF THEN

The notation for conditionals in Forth is a bit special, since it is a stack language.

```
<cond> IF ... (if true) ... THEN ... 
<cond> IF ... (if true) ... ELSE ... (if false) ... THEN ...
```

### How they work

Defining a word IF and making it IMMEDIATE, means that when we compile code,
and come across an IF, instead of creating code to call the IF word, we call it immediately. This
lets us write code which not only generates some (binary) code, but also modifies
the compiler state, or even introduce new state variables, lists and stacks.

In RFOrth, the ACode has a small reserved stack called the compile stack. The IF word 
needs to create a forward jump to either ELSE or THEN, if the condition preceding the
IF, is false. 

Similarly the words THEN and ELSE are also immediate. They access the compile stack 
and use the information to patch the forward conditional jump at the IF location,
to jump to the correct address within the compiled code, which now is known.

Loops are simpler in that they only contain a back jump, to from the bottom of the 
loop to the top, but still need to know where that top is. The compile stack does
this, and at the same time allows for nested structures, both for conditionals
and loops.

CREATE and DOES> (and COMMA)
----------------------------

This is considered an advanced mechanism, often referred to as "the pearl of Forth", because
of the ways it can extend the language. Here goes.

These words are used to create new words *that create words*. We have already seen examples of this
with the CONSTANT and VARIABLE words, which create new words (the names of the constant or variable).

The CONSTANT and VARIABLE implementations are more efficient than using CREATE and DOES> but here
follows an example of how to do it.

```
: Const (value --) CREATE , DOES> @ ;
```

Const takes a value from the stack, and must be followed by a name, just like CONSTANT: 

```
5 Const A
```

The CREATE word is the one that identifies the next word *from the input stream*, which
means "A". 

The COMMA word takes a value from the data stack (5) and stores it on the heap, by allocating
a CELL and writing the value there. 

The DOES> word is kind of advanced, and we don't need to understand the details, other than
that the word being compiled ("Const"), when it is runs, executes the code up to the DOES>, which
creates the new word ("A"), which *in turn* executes only the code following DOES>. 

That code, when running, also is supplied with a reference to the data allocated 
after CREATE, using COMMA in this example, on the stack.

Calling the "@" word reads the value of that CELL, which is 5.

We have a constant.

The CREATE DOES> can make your head spin. It is an impressive mechanism. My implementation
is non-standard, since my memory management is non-standard, but that's another story
of interest only to implementers. 


