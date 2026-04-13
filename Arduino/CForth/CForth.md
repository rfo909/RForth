2026-04-08 CForth
=================

Created a Forth core with interpreter (eval loop) and colon compiler, in the
CForth.in under the Arduino directory.

The intent is to create a fast and low-memory footprint Forth to hopefully run
on the atmega328p (original Nano or Uno) with 2Kb SRAM. 

A major difference is that the REPL and compiler is written in C, instead of
implemented in ACode.txt. This should make the compiler faster, and generally
use less RAM.

Also, the representation of literal values is more straight forward, using
"bval" and "cval" op-codes, followed by 1 or 2 bytes, respectively. 

v0.0.1
------ 
Imagined it would be nice to have forth word calls as compact as possible, using
single bytes. Would discriminate on top bit, allowing 127 op-codes written in C,
and 127 Forth word references. 

This worked, but allocating the array for "definedWords" in advance, with each 
element being Cell sized, meant wasting 254 bytes that may never be fully used.

Has the compiler working, as well as the REPL, and the runtime.

Implemented the ' <word> to get the address of a word, and the dis which
is a disassembler



v0.0.2
------
Rewrote the code into a more traditional format, with the address in the
dictionary entries being Cell sized (2 bytes), and creating a "call" opCode
which is followed by the address (2 bytes).

Then created the "dcall" opcode, in order for Forth to call any address,
from the stack.

Added up to 5 tags /0 to /4 and up to 5 references to tags on format &0 to &4, 
to use with jmp and jmp? to create loops and conditionals. Patches both
forward and backward at base level inside the colon compiler.

v0.0.3 2026-04-11
-----------------
Added proper integer parsing, supporting signed hex, binary and decimal 0xCAFE, b10010.
  
Fixed various bugs.

Removed the "call" opCode. Decided that a 15 bit total address space is
more than enough for this toy Forth. That corresponds to 32 K, which we
then split into two again, for 16K addressable space of compiled Forth, 
and 16K addressable space for data.

Depending on the microcontroller, the actual sizes of these segments
might be (much) smaller. 

Intending to make the code segment span both a Flash part and a "live"
part, which can at most total 16k.

=> This decision opened for reducing Forth calls to 2 bytes. 

The built-in opCodes count 43 now, and are projected to be far fewer
than 127, which means their byte codes have high bit zero.

Calls are now compiled into a byte with high bit 1, followed by a 0, and
then 6 bits + another byte = 14 bits. The runtime uses this high bit to
detect a call, and assembles a 14 bit address using that byte and the next.


It works!

I am keeping the "dcall" opCode (d for dynamic) which takes the Forth call
address off the stack, for function pointer support.

v0.0.4 2026-04-12
-----------------
Optimized for low memory on atmega328p.

Added ops for

- !
- @
- b!
- b@
- comp.out
- comp.next    (compileNext)
- csegHERE     (codeNext)
- HERE         (dataHere)
- variable
- constant
- .str         (print string)
- .c           (print character)

Got "immediate" to work, and tested generation of byte code via comp.out. 

Implemented and tested the " word (quote) in Forth. It is an immediate word, which
builds code (OP_BLOB) with length byte and sequence of characters up until finding matching
quote.



OpCodes
-------

Updated 2026-04-12

65 opcodes

```
bval n                    push single byte value on stack 
cval n n                  push cell value on stack
ret                       return
<addr> jmp                jump to address 
<cond> <addr> jmp?        conditional jump to address if <cond> != 0
blob n ...                just skips the data bytes and returns pointer to the n byte
zero
one

<addr> dcall              dynamic call, taking address from stack
<cond> ret?               conditional return

create (name)             create dictionary entry as type constant, value 0
immediate                 set newest word immediate

+
-
* 
/
%
1+

>
>=
<
<=
==
!=

and                       logical and
or                        logical or
not                       logical not

&                         binary and
|                         binary or
inv                       binary not

cr                        carriage return
.                         print TOS value (signed)
.u                        unsigned
.hex                      hex  
<byte> .c                 print single character
<addr> .str               print string (n ...)

csegHERE                  address of next byte in code segment
comp.next                 address of next byte - when compiling
<byte> comp.out           add byte to code segment - when compiling
HERE                      next address on data segment
<n> allot                 increase HERE 
<value> constant (name)
<value> variable (name)

<value> <addr> !          write cell sized value 
<addr> @                  read cell sized value
<byte> <addr> b!          write byte sized value
<addr> b@                 read byte

dup
swap
2dup
drop
over
pick

?                         list words in dictionary
.s                        show stack
clear                     clear stack

[test                     start timed test for opcount/second
test]                     end timed test

>R                        move value from data stack to return stack
R>                        move value from return stack to data stack

key                       return boolean for if a key has been pressed (serial)
readc                     read character from serial

' (word)                  get address of word
<addr> dis                disassemble word
```


Tags
----
The colon compiler supports up to 5 tags, and up to 5 lookups of tags. The tags
have the names /0 to /4 and the lookups are on the format &0 to &4. 

Patching of tag references is done when hitting the semicolon, so the following
works as expected, with both forward and back jumps.

```
: count-up 1 
  /1 
  dup . 
  1 + 
  dup 30 > &2 jmp? 
  &1 jmp 
  /2 drop ;
```

Note that the tags feature is a colon compiler baseline feature, which means it
will not be known inside nested structures, if and when such are created. 

(Although the support for tags possibly precludes the creation of IF ELSE THEN and BEGIN AGAIN).

NOTE: basic opCodes do not have the option of being IMMEDIATE (yet), so all code that needs to
be, must be written in Forth. Like the '"' word (quote).

Todo
====

- Move dictionary into codeSegment
- Including static strings
- Move dictionaryHead into codeSegment


- Move opCodes content to text format, and generate PROGMEM arrays. Should save approx 500 bytes of RAM
	Currently using 1306 for global variables, including 400 bytes for code/data-segments
	1300 - 500 - 400 = overhead 400 bytes
	-> adding frame stack 10 deep = 20 bytes => 450 bytes overhead
	=> That is 1600 bytes free

	Reserving 200 for C locals and C call stack
	
	=> data 1000 bytes
	=> code 400 bytes
	
On boot, create variable (in RAM) that points to dictionaryHead, in first 2 bytes of the data segment
(code generate C constant for it)

Generate symbol for it in dictionary (_dictHead)


Frame stack of words keeping track of what's added to return stack -> local variables
	Works the same as in RForth


- Build code in C / Forth to export binary data from code segment, to paste into C code, for
standard Forth words, and fix address resolution towards Flash vs codeSegment (simple offset).


- Plugins form a different array of bytes in PROGMEM. Instead of compiling to index, we
can compile directly to C pointers (2 bytes on atmega328p)




---

To save the current state would then mean to save data from the RAM part of the code segment,
plus the data in the data segment.


The address space of the system must contain

- the Flash code (lower code segment)
- the RAM part of the code segment (at offset)
- at an offset: the data segment


