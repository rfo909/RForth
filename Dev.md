2026-04-08 RForth v4
====================

This is now VERSION 4 of my "bytecode Forth" adventure.

---

Created a Forth core with interpreter (eval loop) and colon compiler.

The intent is to create a fast and low-memory footprint Forth to hopefully run
on the atmega328p (original Nano or Uno) with 2Kb SRAM. 

A major difference is that the REPL and compiler is written in C, instead of
implemented in ACode.txt. This should make the compiler faster, and generally
use less RAM.

Also, the representation of literal values is more straight forward, using
"bval" and "cval" op-codes, followed by 1 or 2 Bytes, respectively. 

2026-04-08
----------
Imagined it would be nice to have forth word calls as compact as possible, using
single Bytes. Would discriminate on top bit, allowing 127 op-codes written in C,
and 127 Forth word references. 

This worked, but allocating the array for "definedWords" in advance, with each 
element being Cell sized, meant wasting 254 Bytes that may never be fully used.

Has the compiler working, as well as the REPL, and the runtime.

Implemented the ' <word> to get the address of a word, and the dis which
is a disassembler



2026-04-09
----------
Rewrote the code into a more traditional format, with the address in the
dictionary entries being Cell sized (2 Bytes), and creating a "call" opCode
which is followed by the address (2 Bytes).

Then created the "dcall" opcode, in order for Forth to call any address,
from the stack.

Added up to 5 tags /0 to /4 and up to 5 references to tags on format &0 to &4, 
to use with jmp and jmp? to create loops and conditionals. Patches both
forward and backward at base level inside the colon compiler.

2026-04-11
----------
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

=> This decision opened for reducing Forth calls to 2 Bytes. 

The built-in opCodes count 43 now, and are projected to be far fewer
than 127, which means their Byte codes have high bit zero.

Calls are now compiled into a Byte with high bit 1, followed by a 0, and
then 6 bits + another Byte = 14 bits. The runtime uses this high bit to
detect a call, and assembles a 14 bit address using that Byte and the next.


It works!

I am keeping the "dcall" opCode (d for dynamic) which takes the Forth call
address off the stack, for function pointer support.

2026-04-12
----------
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

Got "immediate" to work, and tested generation of Byte code via comp.out. 

Implemented and tested the " word (quote) in Forth. It is an immediate word, which
builds code (OP_BLOB) with length Byte and sequence of characters up until finding matching
quote.

2026-04-13
----------
Refactoring code into multiple files, for easier handling.


2026-04-14
----------
Got the dictionary fully represented in the codeSegment, instead of as a C
structure. This is a prerequisite for the next step ...

Created an export function that generated C code to paste into the
Static.cpp file. It is used to store functionality in Flash. After
compiling the RForth code, those words that were compiled to
the RAM part of the code segment, become part of Flash.

The dictionary header is pointed to by a Word in the two first bytes of
the dataSegment. When exporting the code segment, this is copied into
the byte 1 and 2 of the CodeSegment.

As the device boots, it copies those bytes from the CodeSegment 
to the dataSegment, allowing us to continue adding words. Both
dictionary entries, word names (Forth strings) and the compiled code
go into the codeSegment.

The only issue is global variables, which after boot point to 
unallocated memory in the dataSegment, which means reading or writing
those addresses will fail with error.

SOLUTIONS:

	- either not create system words that depend on global variables
	- or export the dataSegment as well
		+ programCounter + stacks?
		
=> Trying to keep lib-code independent of data!


2025-04-15
----------
I've done preparations for moving the opCodes data array from RAM into two
PROGMEM arrays called "names" and "functions". Those are added in the INO file,
and it all compiles.

I have not written code to traverse these, but once that's done, and works,
the opCodes RAM table will be deleted, saving hundreds of bytes.

Number of opCodes = 68


2026-04-16
----------
Did a crude profiling of the size of the C stack, and it never exceeded 84 bytes.
Attached the check inside the verifyReadWriteAddress() inside Mem.cpp because any and
all op that interacts with memory, will call this.

The code is very "flat", executing single op after single op, with some housekeeping.

Reserving 200 bytes for C call stack is generous!

Modified the Words CFT script, generating PROGMEM code as well as old config, to ensure
everything is in sync.

The compiler uses the new structure, opNames, for looking up op codes. The index in the tables is
the opCode byte, and the runtime executing the compiled code also uses the new PROGMEM
table opFunctions. So it works.

What remains is a single fix in the disassembler to show the name of an op from its index.
Commenting that out removes the last reference to the opCodes, and recompiling changes

"Global variables use 1567 bytes"
to
Global variables use 1011 bytes

Which corresponds to saving 556 bytes of global variable space!!

This allowed me to change from 400 code + 50 data bytes to

#define CODE_SEGMENT_SIZE     400
#define DATA_SEGMENT_SIZE     800

Still leaving 287 bytes of room for stack, which should be plenty!

2026-04-17
----------
Added printOpNameByPos(), calling it from op_dis(), so disassemler now works fully again.

Also added native functions as ops into RForth

2026-04-18
----------
Added op deep-sleep8, and reorganized the project so that the RForth v3 is tucked away
in a separate directory, and this is the main version!

Added dynamic size for code and data segments, calculated from amount of RAM (Constants.h)

2026-04-19
----------
Added an extra stack, called xStack, which is 4 bytes wide, and containing both
Long and Float data, as well as a number of ops for that purpose. The stack
operations are named xdup, xdrop, xswap and xpick, as they are common to 
Long and Float content.

Example:

```
: pi 314 >F 100 >F F/ ;
```

2026-04-20
----------
I changed the "variable" op to always initializing variable to 0, and not taking value from the 
stack. This means variables are just declared, and have to be initialized by code. 

This means code persisted to flash can have variables, as long as it initializes them. The
only thing needed for this, was to persist the dataNext ("HERE") value when doing code.export,
to bytes 3 and 4, and when booting, in setup() allot this number of bytes, so that references
into that area are valid.

I added initializing the data segment to all zeroes, so that even alloted buffers are clear.

---

In order to reduce the number of ops, which touched 115, I reimplemented the following in
Forth, in the "basics" file under code. 

- constant
- variable
- immediate
- over
- 2dup


Added a new word *dictHead which points to the top dictionary entry. Keeping the create word
in C, as the colon compiler depends on it. 

Also removed the zero and one ops.

Net result, now "just" 109 ops ...

2026-04-21
----------
Standardizing a bit, plus moving content from C to Forth. 

Implementing the following in Forth now

- type (previous but not quite .str in C - is explicitly taking pointer and count params)
- .str 
- cr

Renamed in C

- key (was "readc" - read character from serial, blocking)
- key? (was "key" - return character from serial or 0, nonblocking)



TODO:
-----
To implement IF and LOOP, we need access to 

getNextWord
" ..." nextWordEq (mixedStreq)
compileNextWord

 
OpCodes
-------
See the ops.txt file 


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

- Rig to use 8 MHz always, regardless of external crystal being 16Mhz (Fuse CKDIV8 + dynamic CKDIV 2).
	(Detect on some pin in 8MHz harness)
	
- consider Frame stack, as in RForth v3 for local variables
  
- Autorun-mechanism with option of physical override

