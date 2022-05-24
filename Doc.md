# Introduction

*2022-05-24 v0.1.8*

*RForth* is a compact programming language inspired by Forth. It is stack based,
but does not depend on stack manipulations, as it introduces local variables
inside word functions.

The language runs as a C program on Arduinos, being developed on a Nano with
atmega328p and 2Kbytes SRAM. This is sufficient for running the compiler, which
reduces input text (via Serial monitor) to a binary byte format, and then execute
this.

# Defining words

As in regular Forth, functions are defined with ":" colon and ";" semicolon.

Ex.

```
: DDRB 0x24 ;
: PORTB 0x25 ;
```

This defines two words DDRB and PORTB, which puts different values on the stack.

# Immediate code

The interpreter also accepts "immediate" code, which simply does not start with
a colon. It still must end with ";" to signal that the code should be compiled.

Ex.

```
2 3 + ;
```

The explicit terminator ";" even on immediate code, allowed for a 
a very simple recursive-descent parser / compiler, 
but at the cost of an input buffer, so it will eventually have to be 
rewritten, parsing each word individually.


# Local variables

Instead of manipulating the data stack with dup, swap and various rotates, RForth
uses local variables inside word functions. They take values off the stack, and
give them names.

Ex.

```
: SetBit =addr =mask  # pop values off stack
	addr read 
		mask |
			addr write ;
```

Here we read an address, then logically OR the mask into that value, and write
it back at the same address.

The number of variables per call-stack frame is configurable, from at least 4,
perhaps up to 8. It is a trade off, as it costs a bit or memory.

Once compiled we can call the word:

```
b0010_0000 DDRB SetBit ;
b0010_0000 PORTB SetBit ;
```

The first sets Arduino Pin 13 as output, and the second turns on the LED on this 
Pin (Arduino Nano specific).


# Built-in functions

RForth has the following built-in functions

## Arithmetic

```
+ - * / %

neg              # Flip sign of number
```


## Bitwise logic

```
value count <<   # left-shift
value count >>   # right-shift

|                # Bitwise OR 
&                # Bitwise AND
!                # Bitwise NOT (~ in C)
```


## Logical expressions

```
> < >= <= == !=

and              # Logical and (0 or 1)
or               # Logical or
not              # Logical not
```


## Flow control

```
return
abort            # terminates execution
```


## Access to memory (registers)

```
address read 
value address write
```

## Stack manipulation

```
dup 
pop
```


## Type checks / casts

```
:byte
:int
:uint
:long
:ulong
:sym
:addr
:bool
```

## EEPROM

```
ee:read
ee:write
ee:length
```

## Misc

```
millis           # millis since program start (ulong)
```


# Literal values in code

```
null
3
0x03
b1001
```

# Flow control

These constructs are implemented by the compiler, using JMP, ZJMP and CJMP 
op-codes. ZJMP jumps if value is 0, and CJMP is "conditional", if value nonzero,
and JMP of course unconditional.

```
<bool> if{ ... }

loop{ ...  <bool> break ... }
```

The loop may contain maximum one break, and it must be located directly
in the loop body, or it will not be recognized. 

# Types

RForth has a type system. The data stack is based on 4 byte longs, which can be mapped
to other numeric types. The check is mostly cosmetic, and the higher bits are *not* nulled,
so that if an int is type cast to :byte then back to :int, the original value remains.

The point of types is that CForth has an extended ( ... ) notation which in regular Forth
is used to describe stack movements of functions, but functioning almost (always?) like
a comment.

# Stack movement 

Having introdued local variables, it made sense using the ( ... ) notation 
to assign local variables, and using types for extra information about those, 
as well as basic type checking.

*As of v0.1.8 this is partially implemented* : the input parameters part
is implemented but anything following "--" is ignored, and the "--" is not
mandatory either.


```
: SetBit ( :byte =mask :int =addr -- )
	addr read 
		mask |
			addr write ;
```

As with normal Forth, the order of the parameters inside ( ... ), from left to 
right, is the call order, not the pop order.

 
The compiler reverses the order, first processing the addr value, then mask.

For words returning data, these are specified following the "--", for example.

```
: HasBit ( :byte =mask, :int =addr -- :byte ) ... ;
```


# Interactive commands

The following are commands that are executed directly when seen as part of an immediate
sequence of tokens, that is, not part of a word definition (starting with colon).

```
words 
stats
clear
hex
bin
ee:format:yes
help
dis:xxx
```

# Disassembler

The dis:xxx disassembles the byte code for word xxx. This is useful while developing the
compiler.
