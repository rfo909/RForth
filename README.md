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
advantage: power consumption.

The power consumed while running is one thing, but for solar powered applications,
it is vital that there is a deep sleep mode that runs on micro-amperes, not just
milli-amps.

The atmega328p running at 5V sleeps around 5-7 uA, for 8 seconds at a time, using
the watchdog timer to wake up.

### Code vs data segments

The code segment is a mix of bytes stored in Flash, and bytes in RAM, while the
data segment only exists in RAM.

The code segment contains compiled words, and constants, and the dictionary
structure. The data segment initially contains 2 bytes pointing to
the top dictionary element.

See Dev.md for details.

### Word calling / memory model

To maximize functionality in a memory starved environment, plus also
managing without local variables, points to creating short words that call each
other frequently. 

In v3, calling a word would frequently require 4 bytes of code, each a separate
byte code, which meant doing a C call, and working the stack (more calls). Three bytes to
represent addresses over 12 bits, and one byte to do the call. With REPL and compiler written 
in bytecode, which rested at the start of the memory space, 12+ bits of references
were quickly the standard.

In v4, compiled code works as follows:

- if a byte has high bit == 0, then it is a built-in "op", implemented in C
- otherwise, if high bit == 1, we strip that bit, get another byte from the code, which
  together forms a *15 bit* address, which is then called. The high bit is called the CALL_BIT.
- the second highest bit is used to separate between the code segment (0) and data
  segment (1). The second highest bit is called the DATA_BIT-
  
This means the address range in v4 is 32Kb divided equally into 16Kb of code and 16Kb of
data. 

When we call the HERE word, it returns 16384 + n, where n is the number of bytes of data
that have been reserved. Initially that is two, because the two first bytes contain the
dictionary pointer.

# Language

The language implements a data stack and a return stack. It has the >R and R> words to
temporarily put stuff on the return stack (at your own peril).

The colon compiler supports up to 5 tag defs and/or up to 5 tag references. These form
basic support for both loops and conditionals.

There is no support for "immediate" ops, only Forth words can be immediate. So in order
to support string literals, the word '"' (double quote) is written in Forth. It creates
a string up to 255 characters long. It can only be used in compiled words:

```
: welcome " this is a welcome string" ;
```

Note that this doesn't print the string. To print a string we use the word

```
.str
```

The implementation of the '"' word illustrates tags, comments in (), code generation,
using the return stack, and being an immediate word.

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


# Code in Flash

After having developed words like the one above, and ensured it works, I reset the
mcu, and feed only that word into it. I then call the special op:

```
code.export
```

This generates C code to paste into the Static.cpp source file. 

Recompiling, the data exported now get stored in Flash (PROGMEM).

The "code.export" op includes what was previously stored in Flash, combined with
new content in the code segment (up to code.next), and handles storing the
dictionary pointer in the exported data as well.

On boot, the dictionary pointer is read from the Flash location (bytes 1 and 2) and
copied into the data segment, so we can create new words as expected.

