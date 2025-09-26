Forth never ceases to fascinate !
=================================

Version 3 (alpha)

2025-09-25

Introduction
=============
I'm designing a virtual machine instruction set, stack based. This means many instructions are
quite similar to Forth primitives. On top of this runs a REPL which implements some Forth
words like COLON, SEMICOLON, IF, THEN, ELSE, CONSTANT and VARIABLE.

The point of this whole project is just to have fun. Although I intend writing C code for
the Pi Pico, that is as of yet not a high priority. Experiencing how to get a Forth REPL
up and running from simulated bare metal may just as well pave the way for doing ARM assembly
instead of a C interpreter.

Realizing how little low level code is required to bring up a REPL and a compiler, has been great
fun! And I'm quite sure I'm not even near the optimal way of doing so, as it seems to me when
searching the internet, that most words are MUCH shorter than mine, because they are using
a host of other words, designed very smartly.

I find the Forth way of extending the compiler, by using immediate words, to be really clever!

Writing code in Forth, to compile Forth ... that's awesome! 


Design decision - word size 2 bytes
-----------------------------------
To keep code fairly small, the core of the language operates with 2 bytes word size. All stacks
store word size values, while the byte code instructions are single byte sized. 

To address physical memory, flash, devices, registers on the Pi Pico, which has 32 bits word
length, we might combine two RForth words to form a base and add an offset to that to address
bytes or words. The actual workings of interfacing hardware will have to be resolved later.

Note: javing added a native interface in order to call specially named functions in the Interpreter
implementation language, makes it less likely messing about with SPI, I2C and GPIO
hardware via registers, at least for a while.


Design decision - byte code
---------------------------
In order to make the interpreter as simple as possible, like using an array with function pointers
in C, all instructions are a single byte. Number literals, like addresses of functions, are 
represented using a scheme as follows:

First off, all non-number opcodes stay below 127, and so far all are printable, which makes
stuff more readable (range 33-127 to be exact). To represent numeric values, the high bit
is set to 1, and the second highest bit is set to 1 to indicate pushing a zero to the stack. If not, 
we assume there is a value on the stack to work with. That value is multiplied by 64, before adding the 
value of the last 6 bits (0-63).

```
Example 1
---------
0xC1 = 1100 0001 

Push 0 to stack
Multiply it by 64 and add 1

Result 1

Example 2
---------
0xFF = 1111 1111

Push 0 to the stack
Multiply it by 64 and add 63

Result 63

Example 3
---------

0xC1 0x82

0xC1 = 1100 0001

Push 0 to stack
Multiply by 64 and add 1

0x82 = 1000 0010
Multiply value on stack by 64 and add 2

Result 66
```

For global jumps, addresses are encoded as 3 bytes, or 18 bits. The word size (traditionally 
called "cells" in Forth) is two bytes, which allows for addressing 64k of memory. 

For jumps inside words, I use relative offsets, for forward and back movements, and those are encoded as a single
byte, which allows a max jump length of 63 positions.



Design decisions - heap / jumps inside words
--------------------------------------------

I employ a bit if memory protection, to catch stray pointers. The base code and memory layout
in ACode.txt contains a tag :PROTECT that prevents writes to addresses below that point, currently
enforced in the Interpreter.

There is also a check against the HERE value, preventing any memory access above it. This means
data has to be allot'ed before use. I decided to keep using a fixed compile buffer which is
statically allocated after the PROTECT tag. 

This in turn led me to
make code relocatable, because once a word is compiled, the code to be copied to permanent
storage created with allot. This meant internal jumps could not be absolute. So I created two conditional
relative offset jumps, moving forward or backward, since the base code works only with
unsigned values.


The compile stack
-----------------
A simple word sized compile stack is statically allocated in ACode.txt. Each word
pushed contains a type in the first byte and an absolute location within the compile
buffer in the second. The compile buffer is limited to 255 bytes.

This allows both nesting and recognition of different types, so that if we were to 
allow complex structures like LOOP  (bool) IF BREAK THEN ... ENDLOOP then the 
compile stack can support it, using different type identifiers to lookup
different stuff. 

The THEN looks up the conditional forward jump produced by IF, without mixing it up with
the at that point still unresolved BREAK forward jump, etc.



Assembler and interpreter
-------------------------

I've made an Assembler written in [CFT](https://github.com/rfo909/CFT) script, 
which is a script language I've written myself (in Java). It is perfect for
prototyping. 

I also created an Interpreter in CFT, with various options for inspecting
data structures etc.

The interpreter is growing in complexity, and it is also relatively slow,
chewing through 5-10k assembly instructions per second on my laptop. 

That may sound a lot, but with assembly code doing the whole REPL, interpreter and
compiler, especially the dictionary lookups, compilation speed is slow.

Speed?
------

A real microcontroller doing perhaps 100 instructions per simulated assembly
instruction, running at 80 MHz (Pi Pico), at 1 instruction per clock, should
result in a throughput of 800 K instructions per second, or about 80-160 times the
speed of my CFT interpreter.

Compiling code is the hardest work, due to the dictionary traversals.

Running a tight iteration, it counts and prints values 0-5000 in about 10-12 seconds.


```
: count
	0 cpush   (set local variable a)
	BEGIN
		a .
		a 1 add a!
		a 5000 lt AGAIN?
	;
```

Another example:

```
3 CONSTANT pi
ok

pi pi mul .      (interpreting: 251 ms)

: x pi pi mul . ;   (compiling word: 817 ms)

x			(calling word: 28 ms)
```

Running compiled code is MUCH faster than compiling, and also than interpreting, because
of dictionary traversals. Most of the 251 ms is spent looking up the "mul" assembly instruction,
because if we eliminate it, we get

```
pi .           (71 ms)

: x pi . ;

x              (28 ms)
```

Apart from compile times, it is really not a bad idea having an interpreter running at 
diminished capacity compared to real hardware, because it highlights bottlenecks.


Memory map
----------
The ACode.txt file implements the REPL, which interprets input and when
hitting a COLON, enters compile mode, and compiles code. The REPL is fairly 
primitive, as it should be.

But it also contains notation for allocating static structures (non code).


Levels of code
--------------

The project results in code at three different levels. 

- At the bottom there is the simulated CPU that runs a very Forth-like set of instructions. This is the Interpreter.
- Then there is the "assembly" language, which is represented by ACode.txt. 
It is translated to byte format by the Assembler.
- And on top, there is the Forth language, read and processed by the REPL written in ACode.txt. 

Separating the "assembly" level from Forth is perhaps not so relevant, since the two mix and mingle pretty tight,
particularly since the assembly language is stack oriented. Like, adding numbers with the "word" add, really
just means inserting the byte-sized opcode for add, since it is part of the assembly language I invented.

The initial Forth code is found in Forth.txt, piped as input to the REPL before user input.

The Forth language has access to most of the "assembly" level instructions as well as all 
functions and data defined in ACode via tag lookups like &CompileBuf etc. 


Local variables
---------------

The assembly language supports pushing values from the data stack to the call
stack, with the cpush instruction. These are then accessed and modified with
cget and cset, via defines in ACode.txt, as well as made available to Forth
code. 

```
	45 cpush   # Take value from data stack, push onto call stack
	           # The first value is denoted a, the second b, then c and d
	          
	a          # Read value of variable a (45)
	a 1 add a! # Update variable a to 46
	
	(same for b, c and d)
```

Fetching variable values consist of two byte operations, first the index of
the variable in the current frame on the call stack (0=a, 1=b etc), then the
cget instruction.

```
N cget
```

For updating, there is also two bytes, assuming the value is already on the
data stack:

```
N cset
```


Note that local variables are implemented as defines in ACode.txt, and
with small blocks of "assembly" referred by the dictionary, for the same
functionality in Forth, at least until bothering making a better solution with
custom named variables.

Debugging
---------

The Assembler script contains a disassembler that is immediately applied to the output
from the Assembler, to show what has been done. 

Together with the features of the Interpreter, it is possible to debug both ACode.txt and
the top level Forth code.


Important words
---------------
```
?              (list words)
.s             (show stack, displays values as both uint and hex)
.W name        (show dictionary entry address for word)
clear          (delete all stack content)
```


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


2025-08-31 Tag lookup
---------------------
Forth code can look up data and functions in ACode.txt by tag, typically &CompileBuf as well as &GetNextWord, and
with all the assembly instructions available as words, it can call code as well. 


2025-09-03 ELSE, BEGIN and AGAIN?
------------------------------
Implemented a compile stack in ACode.txt, and got ELSE up together with IF and THEN modified to
use the stack. Also implemented the loop construct BEGIN ... (bool) AGAIN?

Nested loops works fine, as do nested conditionals. 



2025-09-04 CONSTANT and VARIABLE
--------------------------------
Variables are constants pointing to single word size memory blocks allotted by the VARIABLE word. To
store bigger structures, manually allot the memory, and store the pointer to that memory as a constant.

```
55 CONSTANT x
55 VARIABLE y

y @ .      (prints 55)
99 y !     (changes variable value)
```

The Dictionary Entry format is unchanged, and currently eliminates the implementation of the DOES> word
of Forth, since I reuse the code pointer for data. Each word has a mode, and currently there are four
different modes:

- NORMAL
- IMMEDIATE
- INLINE		(code pointer is a single byte instruction)
- CONSTANT		(code pointer is a constant data value)


2025-09-22 native
-----------------
Added instructions "nativec" and "native", to invoke functions inside the interpreter. Using this
to implement function "u2spc" which converts underscores to spaces in a string, calling it from
ProcessString in ACode.txt

Note that calling native functions is a two step process. The "nativec" (compile) takes a string,
and returns an address (int), which is used with the actual call, "native". 

Implemented a NATIVE word in Forth, using these, then migrated it into ACode.txt It handles both compile mode and interpret mode,
and takes the name of the native function from the input, which saves us from creating a string
constant each time calling native.

```
: testNative '300/50+10 NATIVE calc . ;

```

This implementation has two purposes: it detects at compile time when trying to call an invalid
native function, and it speeds up the actual call, as it doesn't have to search the internal
list of native functions.


2025-09-25 Custom dictionaries
------------------------------
Added some code to ACode.txt, which lets us create a custom dictionary. The Dict word creates
a named dictionary on the current dictionary. That should be the global dictionary in most
cases.

Then we do a DictUse on it, and define words as usual. When done, we run DictClear. 

There can be only one custom dictionary, which if activated with DictUse is searched before
the global dictionary, and new words are added to it instead of the global. 

After having created a dictionary, we can call words inside it, both interactively and at
compile time by the "->" word.

```
Dict Something
Something DictUse
: magic "It_is_magic .str ;
DictClear

(interactive)
Something -> magic

(in word)
:x Something -> magic ;
x
```

The point of dictionaries is bundling related functionality and get it out of the way. To
look at a custom dictionary:

```
Something DictUse
?
```




Dictionary entry format
-----------------------

The format is as follows:

- symbolLength: 1 byte
- symbolData: ... (symbolLength bytes)
- codePointer: word (2 bytes)
- mode: word (2 bytes)
- next: word (2 bytes)

The dictionary is a linked list, and the top is stored in statically allocated location &DictionaryHead 
defined in ACode.txt, which is readable from Forth as:
```
&DictionaryHead @
```


 

References
----------

- [Instruction set](InstructionSet.md) for regular opcodes 33-127


