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
ops           (list ops = words written in C)
```


## Changes

The v3 suffered from a few bad design decisions. The most important was that
there was no separation between code and data. This meant there was no easy
way of persisting code to Flash, without risk of making variables readonly.

Key points for v4

- specifically for atmega328p
- code segment and data segment
- efficient Forth word calls

I also have not implemented local variables, as I realized that creating small
words, they are not needed.

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

The colon compiler supports up to 4 tag defs and/or up to 4 tag references. These form
basic support for both loops and conditionals, and are named /0-/3 and &0-&3.

There is no support for "immediate" ops, only Forth words can be immediate. So in order
to support string literals, and without implementing special code in C, the word 's"' 
is written in Forth. It creates a string up to 255 characters long. It
can only be used in compiled words:

```
: welcome s" this is a welcome string" ;
```

Note that this doesn't print the string. To print a string we use the word

```
.str
```

The base Forth words are found in the code/basics file.

## Tags and refs

Instead of initially creating LOOP and IF, the colon compiler, which is written
in C, supports up to four tags, and up to four tag references. It is the
most flexible (albeit assembler-like) mechanism for conditionals and loops.

```
: count
	1                                 (initial count value)
	/0                                (tag)
		dup 100 > &1 jmp?         (&1 is a tag reference, and jmp? is conditional jump)
		dup .                     (print value)
		1 +                       (increase count)
		&0 jmp                    (repeat loop)
	/1                                
		drop                      (cleanup)
;
```

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

# NOTES

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

The Arduino Uno as well as the original Nano, runs at 16 MHz, but I intend to clock it down
to 8 MHz, and at boot time, via some pin inform the CPU what crystal it is currently
running off. Using CLKDIV bits (fuses and dynamically) means we can run at 8 MHz even
on a 16 MHz crystal, and have all timings be correct.

This is important, in order to continue using the Uno R3 for programming the chips, as it has
the 16 MHz crystal, before moving the bare atmega328p into a separate harness with a crystal
and some power source, which means it can run on 3.3V.

