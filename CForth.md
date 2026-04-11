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


First attempt
-------------
Imagined it would be nice to have forth word calls as compact as possible, using
single bytes. Would discriminate on top bit, allowing 127 op-codes written in C,
and 127 Forth word references. 

This worked, but allocating the array for "definedWords" in advance, with each 
element being Cell sized, meant wasting 254 bytes that may never be fully used.

Created a word to look up the address of a word, and a disassembler, in order
to debug the workings of the compiler.


Second version
--------------
Rewrote the code into a more traditional format, with the address in the
dictionary entries being Cell sized (2 bytes), and creating a "call" opCode
which is followed by the address (2 bytes).

Then created the "dcall" opcode, in order for Forth to call any address,
from the stack.


OpCodes
-------

2026-04-11: 44 opcodes so far

```
bval n                    push single byte value on stack 
cval n n                  push cell value on stack
ret                       return
<addr> jmp                jump to address 
<cond> <addr> jmp?        conditional jump to address if <cond> != 0
call n n                  call Forth code on given address
<addr> dcall              dynamic call, taking address from stack
<cond> ret?               conditional return

create (name)             create dictionary entry as type constant, value 0
immediate                 set newest word immediate

+
-
* 
/
%

>
>=
<
<=
==
!=

and                       logical and
or                        logical or
not                       logical not

cr                        carriage return
.                         print TOS value (signed)
.u                        unsigned
.hex                      hex  

dup
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
have the names /1 to /5 and the lookups are on the format &1 to &5. 

Patching of tag references is done when hitting the semicolon, so the following
works as expected, with both forward and back jumps.

: count-up 1 
  /1 
  dup . 
  1 + 
  dup 30 > &2 jmp? 
  &1 jmp 
  /2 drop ;
  

Note that the tags feature is a colon compiler baseline feature, which means it
will not be known inside nested structures, if and when such are created. 

(Although the support for tags possibly precludes the creation of IF ELSE THEN and BEGIN AGAIN).

Todo
====

Immediate words have not been tested. The compiler should support it, but we need to add opCode
or emitting bytes.

- bitwise andb orb neg
- byte read write b@ b!
- compileOut
- codeNext

Also have no way of *interacting with the data segment*.

The idea here is that the codeSegment may eventually live in flash, with a continuation for
new words, living in RAM, while the data segment always lives in RAM. 

To save the current state would then mean to save data from the RAM part of the code segment,
plus the data in the data segment.

Haven't decided how to allocate memory in the dataSegment, only that allot will work on it,
while both constants and variables (which are constants containing addresses) live in the
code segment.

The address space of the system must contain

- the Flash part of the code segment
- the RAM part of the code segment
- at an offset: the data segment

We may just split the address space in two, allowing up to 32 K for each (code and data).

Doing this rigorously, letting data addresses always be above 32k means the actual size
of the code segment may change, without affecting data pointers.

The data segment addresses must be

