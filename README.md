# RForth v4


*2026-04-18 RFO*

Updated: 2026-10-24 RFO

## Introduction

This implementation is the fourth (!) attempt of a bytecode compiling Forth-like
language, and was initiated April 2026. I decided to put the REPL and colon compiler 
in C, and I made some changes to the memory architecture, splitting it into two
segments, *code* and *data*. More on that below. 

The bytecode operations are single-byte values known as *ops*.

```
?             (list words)
ops           (list ops = words written in C)
.s            (show stacks)
clear         (clear stacks)
```


## Changes

The previous version (v3) suffered from a few bad design decisions. The most important was that
there was no separation between code and data. This meant there was no easy
way of persisting code to Flash, without risk of making variables readonly.

Key points for v4

- deepsleep specifically for atmega328p
- code segment and data segment
- efficient (2 byte) Forth word calls

I also have not implemented local variables, as I realized that creating small
words, and using the return stack, they are not needed. 

### atmega328p

While other CPU's like the 4809 (Nano Every) look good on paper, with more 
peripherals, more RAM and more Flash, the good old atmega328p has one major
advantage: proper deep sleep. 

Plus it is my favourite, as it usually comes
in a DIP 28-pin package, which with less than five components can run
happily on a breadboard or stripboard.

At deep sleep, the atmega328p consumes around 5-7 uA, for at most 8 seconds at a
time, using the watchdog timer to wake up. This sleep is available via the op "deep-sleep8".

On other Arduino's that op is implemented as a simple 8 second delay.


### Code vs data segments

The code segment is a mix of bytes stored in Flash, and bytes in RAM, while the
data segment only exists in RAM.

The code segment contains compiled words, and constants, as well as the dictionary
structure, and offers a single address space, regardless of what is stored in
Flash.

The data segment initially contains 2 bytes pointing to
the top dictionary element.

See Dev.md for details.

### Calling words / Memory model

To maximize functionality in a memory starved environment, plus also
managing without local variables, means creating short words that call each other. This 
leads to making the calling of Forth words as efficient as possible.

The first design idea was using a single byte, allowing up to 127 words, but that felt
restrictive, as the plans for putting compiled Forth words into Flash matured. 

In v3, calling a word would frequently require 4 bytes of code, each a separate
byte code, which meant doing a C call, and working the stack (more calls). Three bytes to
represent addresses over 12 bits, and one byte to do the call. With REPL and compiler written 
in bytecode, which rested at the start of the memory space, 12+ bits of references
were quickly the standard.

The CALL_BIT
------------

In v4, compiled code works as follows:

- When processing a code byte, we look at its highest bit, which is the CALL_BIT
- If it is 0, then the remaining 7 bits identify an "op" implemented in C
- This means there can be at most 127 op-codes
- If CALL_BIT is 1, then the current byte plus the following byte (in compiled code)
  point to a Forth word. However, the second highest bit, called DATA_BIT, limits the
  code space for Forth words from 15 to 14 bits. 
- The DATA_BIT is not strictly relevant for generated code, because there we
  only differ between doing an "op" or calling a Forth word, but handling Forth
  word addresses in code requires that we know how to differ between the code segment
  and the data segment.  
- Generating code for a Forth call with the DATA_BIT set to 1 makes no sense, semantically,
  and is an error, because such a value can't be processed correctly as a code pointer.

The DATA_BIT
------------

Apart from generated code, ops will also push addresses on to the data stack as unsigned
int values, and here the DATA_BIT matters, as it differentiates between the two
segments (code and data).

With the CALL_BIT set to zero, the remaining 15 bits of an address come out as
0 being the start of the code segment, and 16384 the start of the data segment (DATA_BIT set).

Data segment access is guarded by the value of HERE, which is the next available byte. Read
and write of addresses larger than or equal to HERE, is an error.

The code segment on the other hand, lives partly in Flash and partly in RAM. While all
valid addresses can be read, with no regard for the "code.next" pointer, which defines where
the compiler will write output, trying to write to Flash addresses clearly will fail.

Also, the actual amount of SRAM allocated to the segments will be (much) smaller than 16k, so
we check read/write addresses against those limits as well. This means that if the microcontroller
has allocated 500 bytes of code segment in SRAM, and there exists 1000 bytes of the code
segment in Flash, the code segment addresses are in the range 0-1499.

### HERE

The HERE word returns 16384 + n, where n is the number of bytes of data that have been
reserved. On boot, the system reserves two bytes for the dictionary pointer. 

Note that the value of HERE is not stored in Forth memory space. It exists as a variable
in the C code.

# Language features

The language implements a data stack and a return stack. It has the >R and R> words to
temporarily put stuff on the return stack.

The colon compiler supports up to 4 tag defs and/or up to 4 tag references. These form
basic support for both loops and conditionals, and are named /0-/3 (defs) and &0-&3 (refs).

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

The base Forth words are found in the code/basics file, which also includes the
apostrophe word, which returns the code pointer for a word, frequently used to
invoke the disassembler:

```
' someWord dis
```




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

# Loops and conditionals

In addition to the above, which is very efficient, we have also "begin ... again"
with the option of multiple calls to break out of the loop. 

Conditionals follows the normal "if ... then" and "if ... else ... then" pattern
of Forth.


```
: count (limit --)
	0 
	begin
		2dup == break?
		dup .
		1+
		key? break?  (-- key pressed? --)
	again
	2drop
;

: count2 (limit --)
	0
	/1
		2dup == if 2drop &2 jmp then
		dup .
		1+
		&1 jmp
	/2
;
```


# Code in Flash

After having developed words we like to keep, we can call the special op:

```
code.export
```

This generates C code to paste into the Static.cpp source file.

After pasting the code into the cpp-file, and recompiling, the words that previously
only lived in RAM, now are stored in Flash (PROGMEM).

The "code.export" op includes what was previously stored in Flash, combined with
new content in the code segment (up to code.next), and handles storing the
dictionary pointer in the exported data as well.

On boot, the dictionary pointer is read from the Flash location (bytes 1 and 2) and
copied into the data segment, so we can create new words as expected.

# NOTES

## code.export

In order to keep the design simple, the code.export op only exports bytes from the
code segment, and no copy of data from RAM (data segment). 

This means that Forth words to be stored in Flash can not create variables.

## Variables

In traditional Forth, variables are given an initial value on being declared. In RForth
declaring a variable creates a CELL of data on the data segment, which a name in the 
dictionary refers to (it is really a constant). 

When doing code.export, the number of bytes allot'ed from the data segment is 
persisted, but the data content is not. This means that even if we allot a number of bytes
to create a table referred by a variable before persisting with code.export, when booting
the system after compiling the code into Flash, the data are allot'ed but without
original values.

This is the reason variable definitions don't come with a default value, because in RForth
variables and data structures must be initialized with code anyways.

## Autorun

To autorun the topmost word on the dictionary, connect pins D2 and D4 and restart.

## Clock frequency

The Arduino Uno as well as the original Nano, runs at 16 MHz, but I intend to clock it down
to 8 MHz, and at boot time, via some pin inform the CPU what crystal it is currently
running off. Using CLKDIV bits (fuses and dynamically) means we can run at 8 MHz even
on a 16 MHz crystal, and have all timings be correct.

This is important, in order to continue using the Uno R3 for programming the chips, as it has
the 16 MHz crystal, before moving the bare atmega328p into a separate harness with a crystal
and some power source, which means it can run on 3.3V.

## Speed

As of september 2026, the speed running on an atmega328p at 16 MHz is about 87.500 Forth ops
per second. See the "speed" source file under code.

## Interact over serial

(Rewrite October 2026)

A CFT script called SlowSend handles sending both interactive input and dumping text files
to the arduino, doing so at reduced speed so the serial input buffer doesn't overflow while 
searching the dictionary.

The SlowSend script doesn't handle output on serial, so I use a separate terminal.

As of October 2026, the SlowSend configures the serial port correctly on Linux,
by running an *stty* command. The speed is 9600 baud. 

After hooking up the Arduino via USB:
```
sudo chmod 777 /dev/ttyACM0
```
Then in the input terminal, running CFT, load the SlowSend script and
run the RForth function inside.


```
cd RForth
:load SlowSend
RForth
```

This will ask for the device, then configure it, before entering the 
main loop. 

Now we can open a second terminal, and just do:


```
cat /dev/ttyACM0
```

Alternatively, if not uploading files of source code, interactive use is
possible with GNU screen or any other terminal emulator:

```
screen /dev/ttyACM0 9600
```

Also note that the "cat" must be killed before trying to upload from the IDE.


It doesn't interact with the tty other than when sending, so it co-exists nicely
with upload and "serial monitor" in the IDE.

