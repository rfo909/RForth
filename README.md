# RForth v4

*2026-04-18 RFO*

## Introduction

The v3 version was quite successful at compiling my Forth into a bytecode format,
which is then executed. But there were issues, and in hindsight, it was overly
complicated.

So I tried writing a new kernel, with the REPL and colon compiler implemented
in C instead of bytecode, as I did in v3. And I got it up in just a few hours.

Since then the code base has grown steadily, and gone through a few basic changes,
but as of 2026-04-18 version 3 has been moved out of the way.

```
?             (list words)
ops           (list ops - written in C)
```


## Changes

The v3 suffered from a few bad design decisions. The most important was that
there was no separation between code and data. This meant there was no easy
way of persisting code to Flash, without risk of making variables readonly.

Key points for v4

- specifically for atmega328p
- code segment and data segment
- efficient Forth word calls

I also have not (yet?) implemented local variables.

### atmega328p

While other CPU's like the 4809 (Nano Every) look good on paper, with more 
peripherals, more RAM and more Flash, the good old atmega328p has one major
advantage: proper deep sleep.

The power consumed while running is one thing, but for solar/battery powered applications,
it is vital that there is a deep sleep mode that runs on micro-amperes, not just
milli-amps.

The atmega328p running at 5V sleeps around 5-7 uA, for 8 seconds at a time, using
the watchdog timer to wake up. 


### Code vs data segments

The code segment is a mix of bytes stored in Flash, and bytes in RAM, while the
data segment only exists in RAM.

The code segment contains compiled words, and constants, and the dictionary
structure. 

The data segment initially contains 2 bytes pointing to
the top dictionary element.

See Dev.md for details.

### Calling words / Memory model

To maximize functionality in a memory starved environment, plus also
managing without local variables, means creating short words that call each other. This 
means calling words should be as efficient as possible.

The first design idea was using a single byte, allowing up to 127 words, but that felt
restrictive, as the plans for putting compiled Forth words into Flash matured. 

In v3, calling a word would frequently require 4 bytes of code, each a separate
byte code, which meant doing a C call, and working the stack (more calls). Three bytes to
represent addresses over 12 bits, and one byte to do the call. With REPL and compiler written 
in bytecode, which rested at the start of the memory space, 12+ bits of references
were quickly the standard.

In v4, compiled code works as follows:

- if a byte has high bit == 0, then it is a built-in "op", implemented in C
- the high bit is named the CALL_BIT
- the second highest bit is used to separate between the code segment (0) and data
  segment (1). The second highest bit is called the DATA_BIT
- having the databit set for a CALL is an error
- After detecting a call, the following byte is combined with six lower bits of the first byte
  forming a *14 bit* address into the code segment.
  

This means the address range in v4 is 32Kb divided equally into 16Kb of code and 16Kb of
data (2 x 14 bits).

The call instruction is therefor implicit, represented by that single high bit. For dynamic
calls picking up an address from the stack, there is the "DCALL" op. 

### HERE

The HERE word returns 16384 + n, where n is the number of bytes of data that have been
reserved. On boot, the system reserves two bytes for the dictionary pointer. 


# Language features

The language implements a data stack and a return stack. It has the >R and R> words to
temporarily put stuff on the return stack (at your own peril).

The colon compiler supports up to 5 tag defs and/or up to 5 tag references. These form
basic support for both loops and conditionals.

There is no support for "immediate" ops, only Forth words can be immediate. So in order
to support string literals, and without implementing special code in C, the word '"' 
(double quote) is written in Forth. It creates a string up to 255 characters long. It
can only be used in compiled words:

```
: welcome " this is a welcome string" ;
```

Note that this doesn't print the string. To print a string we use the op

```
.str
```

The implementation of the '"' word illustrates tags, comments in (), code generation,
using the return stack, and the effect of immediate words. It generates the op OP_BLOB,
followed by number of characters and then the characters.

Since it doesn't know the number of characters in advance, it writes a zero byte, storing
the address on the return stack, in order to patch in the real count once it is known, after
finding the corresponding '"' character.

```
(ops with fixed byte codes)

2 constant OP_BVAL
3 constant OP_CVAL
4 constant OP_RET
5 constant OP_JMP
6 constant OP_COND_JMP
7 constant OP_BLOB
8 constant OP_ZERO
9 constant OP_ONE

34 constant CH_QUOT		(The " character)

(
Create blob (string) of bytes up to the end quote. Only works 
for compile, as it generates code into (net yet allocated) memory
in the code segment. The OP_BLOB executes by reading the length, 
advancing the program pointer past the length byte and the following
bytes, then pushing the address of the length byte on the stack
)
: " ( -- )
	OP_BLOB comp.out
	
	comp.next >R   (store address of length field)
	0 comp.out    (length field dummy value)
	
	0	(count)
	/0
		readc 
		dup 			(count char char)
			CH_QUOT == &2 jmp?   (--- matching quote found)
		(char) comp.out           (count)
		1+
		&0 jmp
	/2
		drop
		R> 
			b!  (patch the length field)
; immediate
```

Here the conditional, when we match the end quote, is implemented with the "jmp?" op,
which is a conditional jump. There is also the "ret?" op, which is conditional
return.

The "comp.next" op points to the next byte in code space while compiling. The
compiler writes to unallocated memory (past code.next), while compiling. The
"comp.out" op adds a single byte to the output.

The /0 and /2 are tags, and the &0 and &2 are the lookups. Tags are resolved both
forward and backward, supporting forward jumps, as in this example when 
matching quote found.


# Code in Flash

After having developed words we like to keep, we can call the special op:

```
code.export
```

This generates C code to paste into the Static.cpp source file.

Note that it will include test words, so usually after testing, we reset the mcu, 
and re-enter the code (I use slow-send script to read from file), then make a clean export. 

After pasting the code into the cpp-file, and recompiling, the words that previously
only lived in RAM, no are stored in Flash (PROGMEM).

The "code.export" op includes what was previously stored in Flash, combined with
new content in the code segment (up to code.next), and handles storing the
dictionary pointer in the exported data as well.

On boot, the dictionary pointer is read from the Flash location (bytes 1 and 2) and
copied into the data segment, so we can create new words as expected.

# NOTES / TODO

## code.export

In order to keep the design simple, the code.export op only exports bytes from the
code segment, and no copy of data from RAM (data segment). 

This means that Forth words to be stored in Flash can not create variables or allot
data memory.

## Autorun

The way to define an autorun, as well as how to block it, hasn't been decided yet. Presumably
a special word, such as Main may be autorun, and pulling some pin high or low may enter
interactive mode.

## Clock frequency

The Arduino Uno as well as the original Nano, run at 16 MHz, but I intend to clock it down
to 8 MHz, and at boot time, via some pin inform the CPU what crystal it is currently
running off. Using CLKDIV bits (fuses and dynamically) means we can run at 8 MHz even
on a 16 MHz crystal, and have all timings be correct.

This is important, in order to continue using the Uno R3 for programming the chips, which has
the 16 MHz crystal, before moving the bare atmega328p into a separate harness with a crystal
and some power source, now with the option of running at either 5V or 3.3V.

