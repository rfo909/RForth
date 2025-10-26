
Development log
===============

In order to create a Forth REPL, I started out defining a virtual stack machine, which executes
byte code instructions. I then made an assembler for this, which handles addresses. In this assembly
language I wrote the REPL for Forth, and a few key Forth words, like COLON, SEMICOLON, IF, THEN, ELSE, 
CONSTANT and VARIABLE.

The "assembly" code is found ACode.txt

I created an interpreter in a script language, to play around with, stepping through code, setting break
points, examine memory structures etc.

The point of this whole project is just to have fun. 

REPL and compiler
-----------------
Realizing how little low level code was required to bring up a REPL and a Forth compiler, has been great
fun! I also find the Forth way of extending the compiler, by using immediate words, to be really clever!

Writing code in Forth, to compile Forth ... that's awesome! 


Design decision - cell size 2 bytes
-----------------------------------
To keep code fairly small, the core of the language operates with 2 bytes cell size. All stacks
store word size values, while the byte code instructions are single byte sized. 

To address physical memory, flash, devices, registers on the Pi Pico, which has 32 bits word
length, we might combine two RForth cells to form a base and add an offset in order to address
bytes or words. The actual workings of interfacing hardware will have to be resolved later.

Note: javing added a native interface in order to call specially named functions in the Interpreter
implementation language, makes it less likely messing about with SPI, I2C and GPIO
hardware via registers, at least for a while.


Design decision - byte code
---------------------------
In order to make the interpreter as simple as possible, like using an array with function pointers
in C, all instructions are a single byte. Number literals, like addresses of functions, are 
represented as single bytes as well.


### Number literals

First off, all non-number opcodes stay below 127. To represent numeric values, the high bit
is set to 1. The second highest bit is set to 1 to indicate pushing the value of the remaining 6 bits
on to the stack. If second highest bit is 0, then instead pop a value from the stack, multiply by 64, then add the
value of the last 6 bits (value 0-63).

```
Example 1
---------
0xC5 = 1100 0101 

The x1xxxxxx bit indicates that the 6 bit value following should be pushed directly to stack

Result 5

Example 2
---------
0xFF = 1111 1111

Result 63

Example 3
---------
Representing the value 500

0xC7 = 1100 1111 = Push 7 to stack
0xB4 = 1011 0100 = Pop 7, shift left 6 places (multiply by 64), add 52

Two bytes on this format can represent values 0-4095.
```

For global jumps, addresses are encoded as 3 bytes, or 18 bits. The cell size
is two bytes, which allows for addressing 64k of memory. 

For jumps inside words, I use relative offsets, for forward and back movements, and those are encoded as a single
byte, which allows a max jump length of 63 positions.


Design decisions - heap / jumps inside words
--------------------------------------------

I employ a bit if memory protection, to catch stray pointers. The base code and memory layout
in ACode.txt contains a tag :PROTECT that prevents writes to addresses below that point, currently
enforced in the Interpreter.

There is also a check against the HERE value, preventing any memory access above it. This means
data has to be allot'ed *before use*. I decided to keep using a fixed compile buffer which is
statically allocated after the PROTECT tag. 

This in turn led me to
make code *relocatable*, because once a word is compiled, the code has to be copied to permanent
storage created with allot. This meant internal jumps could not be absolute. So I created two conditional
relative offset jumps, moving forward or backward, since the base code works only with
unsigned values (and thus double the range). 


The compile stack
-----------------

A simple cell sized compile stack is statically allocated in ACode.txt. Each value
pushed contains a type in the first byte and an absolute location within the compile
buffer in the second. The compile buffer is limited to 127 bytes.

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
chewing through 5-10k byte code instructions per second on my laptop. 

That may sound a lot, but byte code running the whole REPL, interpreter and
compiler, means it must do the dictionary traversals. And since all of my byte code
assembly codes are available to Forth, the initial dictionary has about 60 entries just
for those, plus some 30 others for base Forth words. This means compile is slow.

Speed?
------

A real microcontroller doing perhaps 100 machine instructions per simulated assembly
instruction, running at 80 MHz (Pi Pico), at 1 instruction per clock, should
result in a throughput of 800 K byte code instructions per second, or about 80-160 times the
speed of my CFT interpreter.

*Arduino update:* the Nano Every runs at 20 MHz, which should give about ~200K byte code instructions
per second, or 20-40x the speed of the CFT interpreter

Compiling code is the hardest work, due to the dictionary traversals.

Examples with the CFT interpreter
---------------------------------
```
3 CONSTANT pi
ok

pi pi mul .      (interpreting: 251 ms)

: x pi pi mul . ;   (compiling word: 817 ms)

x			(calling word: 28 ms)
```

Running compiled code is MUCH faster than compiling, and also than interpreting, because
of dictionary traversals.

Memory map
----------
The ACode.txt file implements the REPL, which interprets input and when
hitting a COLON, enters compile mode, and compiles code. The REPL is fairly 
primitive, as it should be.

But it also contains notation for allocating static structures (non code),
such as the initial dictionary, compile buffer, next word buffer and various
state.


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
NATIVE ?       (list all native functions)
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

(Note: this was easily implemented in the scripted Interpreter, but has not been brought into the C implementation).


2025-09-03 ELSE, BEGIN and AGAIN?
------------------------------
Implemented a compile stack in ACode.txt, and got ELSE up together with IF and THEN modified to
use the stack. Also implemented the loop construct BEGIN ... (bool) AGAIN?

Nested loops works fine, as do nested conditionals. 



2025-09-04 CONSTANT and VARIABLE
--------------------------------
Variables are constants pointing to single cell sized memory blocks allotted by the VARIABLE word. To
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
Added instructions "nativec" and "native", to invoke functions inside the interpreter. It is intended
for implementing stuff like GPIO, SPI, I2C etc, as well as other system functions. 

Note that calling native functions is a two step process. The "nativec" (compile) takes a string,
and returns an address (int), which is used with the actual call, "native". 

Implemented a NATIVE word (defined in ACode.txt) handles this, both compile mode and interpreted. It
takes the name of the native word from the input as follows (pX are parameters). 

```
: testNative p1 p2 .. NATIVE test . ;
```

This implementation has two purposes: the "nativec" at compile time detects when trying to call an invalid
native function, and it speeds up the actual call, as it returns an index into the list of native
functions, which is what the "native" op expects. This makes the actual call quite faster than the compile
step.


2025-09-25 Custom dictionaries
------------------------------
Added some code to ACode.txt, which lets us create a custom dictionary. The Dict word creates
a named dictionary on the current dictionary. That should be the global dictionary in most
cases.

Then we do a DictUse on it, and define words as usual. When done, we run DictClear. 

There can be only one custom dictionary, which if activated with DictUse is searched before
the global dictionary, and new words are added to it instead of the global. 

After having created a dictionary, we can call words inside it, both interactively and at
compile time by the "IN" word (originally named "->")

Note that the IN comes in front of both the dictionary and the word, because it accesses
both via &GetNextWord. This simplifies the implementation, and makes the code size the exact
same as if calling a word in the same dictionary or in the global one.

```
Dict Something
Something DictUse
: magic "It's_magic .str ;
DictClear

(interactive)
IN Something magic

(in word)
:x IN Something magic ;
x
```

The point of dictionaries is bundling related functionality and get it out of the way. To
look at a custom dictionary, use it, then type the '?' command

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


2025-10-07 Arduino 
------------------

While originally intended for the Pi Pico, I have instead started out writing code for the Arduino
Nano Every, with the ATmega4809 processor, which has 6 Kbytes of SRAM, and a full
48 kbytes of flash, with room for plenty of "firmware". 

I also dug out some 32 Kbytes EEPROM chips which I plan to hook up over i2c, for storing source
and who knows what.

Memory model
------------

The current memory model is fairly simple. The ACode.txt contains a tag PROTECT, which is meant to
protect data below that address from writes. I copy and paste the output from the Assembler script into 
the Firmware.h file.

The init code then copies bytes from the Protect location to the end (currently 157 bytes), which contains
variables, into RAM, in the form of a buffer called "heap" in the code.

The readByte() and writeByte() functions examine the given address. If it is below the PROTECT tag, it is 
a readonly value from the flash data. Otherwise, we subtract the PROTECT tag and have read/write access to
the RAM buffer. The initial value of HERE is set to the total size of the firmware data.

2025-10-16 Nano Every runs RFOrth
---------------------------------

650 lines of C code, plus about 320 lines of defines and const-data. This is the "firmware", which is the
assembled output of ACode.txt, plus a list of op-names as strings. 

Seems to execute about at an initial speed of 85k bytecode instructions per second. Not what I hoped for,
but acceptable.

2025-10-16 Native functions Arduino
-----------------------------------

The NATIVE Forth word, which uses the "nativec" and "native" instructions, and long since implemented in
ACode.txt now works with actual native functions on the Nano Every. There are currently two native functions, 
the first is called "?", which lists all native functions, including itself. 

The other is called "Sys.Free", and returns the number of bytes free on the heap. For use both 
interpreted and compiled. 

```
NATIVE ?
NATIVE Sys.Free .
```

2025-10-17 Blink
----------------
Added a few native functions related to GPIO pins on Arduino, enabling the traditional blink example.

```
: pOut NATIVE Pin.ModeOut ;
: flash cpush 50 1 a NATIVE Pin.PulseDigitalMs ;
: Z NATIVE Sys.Delay ;
: blink 13 pOut BEGIN 13 flash 500 Z 1 AGAIN? ;
```

Compiling this consumes 78 bytes, which stores both the compiled code and the dictionary entries.



2025-10-18 save Forth heap to EEPROM - cforce
---------------------------------------------

I want to save the heap with all compiled words and the dictionary entries for those. 

I thought moving the stacks from the C heap to Forth heap would make sense, in terms of
saving current state. But it turns out to be the opposite, because in order to save and
restore a runnable image, we need to execute code. That code must exist in the "firmware"
part (below the PROTECT tag), which means it lives only in flash.

Local variables and parameters require the three stacks to operate, so they must exist 
outside the block of memory being saved or loaded back in.

The actual stack content should not be necessary to restart the system. If it starts
running with PC=0, it should then operate normally. The code should be extended to reading a
start word from somewhere, unless some pin X is HIGH or LOW, deciding between automatic
running or interactive mode. 

The Forth heap contains the following:

- compiled code: words, constants and variables
- dictionary entries
- data between the PROTECT tag and the end of the firmware (ACode.txt run through assembler).

Calling code from compiled Forth to load an image means the return address back to that Forth word
becomes invalid. To fix this, I added a bytecode op, "cforce", which takes an address (usually zero).

It clears all three stacks, then "fakes" the given return address on to the call stack. It
keeps local variables from current frame, but when returning from current frame, we instead
return to the address given as parameter to the cforce op, typically 0.

Example:

```
: test 0xcafe cpush 0xface cpush 0 cforce cr a print cr b print ;
: xx test "return_from_test cr .str ;
```

Calling xx in turn calls test. It pushes two values on the call stack, 
then calls 0 cforce. The code continues, running normally, as demonstrated by printing
the variables a and b, but when it returns, it does not return to the xx word, 
instead it picks up from address 0, which re-initiates the REPL.

Still a work in progress ...


2025-10-21 Forth local variables
---------------------------------
Implemented proper local variables inside COLON-words. Removed the automatic names
a,b,c,d for the four first values on the call stack (over the return address). 

```
: x (a b -- a * b) 
  => b => a a b mul ; ;

: x 0 
  => count BEGIN count . count 1+ => count count 100 lt AGAIN? ;
```

The colon compiler maintains a symbol table for local variables, in the LVBuf buffer.
It is 32 bytes long, and variable names are stored on the format:

```
N...N...0
```
Their position in the sequence corresponds to the indexed value on the call stack,
created with cpush and accessed via cget and cset.

Note that local variables don't work interactively. Variable names are limited
to 7 characters (8 bytes). 

In ACode.txt it is still the old abcdef variables. 

2025-10-21 CompileBuf limit test
--------------------------------
Added code to check that the colon compiler doesn't generate compiled words longer
than the CompileBuf


2025-10-26 CREATE COMMA DOES>
-----------------------------
Got this working as intended. The implementation got fairly elegant.

Also added bounds check for NextWord and CompilingWord. NextWord is
bigger, because it is also used to process string constants. 


References
----------

- [Instruction set](InstructionSet.md) for regular opcodes 33-127


