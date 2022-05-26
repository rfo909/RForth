# Introduction

*2022-05-25 v0.2.0*

*RForth* is a compact programming language inspired by Forth. It is stack based,
but does not depend on stack manipulations, as it introduces local variables
inside word functions.

The language runs as a C program on Arduinos, being developed on a Nano with
atmega328p and 2Kbytes SRAM. This is sufficient for running the compiler, which
reduces input text (via Serial monitor) to a binary byte format, and then execute
this.

# Defining words ("compile mode")

As in regular Forth, functions (or "words") are defined with ":" colon 
and ";" semicolon.

Ex.

```
: DDRB 0x24 ;
: PORTB 0x25 ;
```

This defines two words DDRB and PORTB, which puts different values on the stack.

# Interactive code

The interpreter also accepts "interactive" code, which simply is code
that does not start with
a colon. It gets compiled and executed when pressing ENTER. 


Ex.

```
2 3 + 
DDRB
```




# Local variables

Instead of manipulating the data stack with dup, swap and various rotates, RForth
uses local variables inside word functions. They take values off the stack, and
give them names.

Ex.

```
: SetBit =addr =mask  # pop values off stack
	addr sys:read 
		mask |
			addr sys:write ;
```

Here we read an address, then logically OR the mask into that value, and write
it back at the same address.

The number of variables per call-stack frame is configurable, from at least 4,
perhaps up to 8. It is a trade off, as it costs a bit or memory.

Once compiled we can call the word:

```
b0010_0000 DDRB SetBit
b0010_0000 PORTB SetBit
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
sys:return           # return to where current word was called
sys:abort            # terminates execution
```


## Access to memory (registers)

```
address sys:read 
value address sys:write
```

## Stack manipulation

```
sys:dup 
sys:pop
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


## Misc

```
sys:millis           # millis since program start (ulong)

address ee:read
value address ee:write
```


# Literal values in code

```
$null
3
0x03
b0011
b0000_0011
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
	addr sys:read 
		mask |
			addr sys:write ;
```

As with normal Forth, the order of the parameters inside ( ... ), from left to 
right, is the call order, not the pop order.

 
The compiler reverses the order, first processing the addr value, then mask.

For words returning data, these are specified following the "--", for example.

```
: HasBit ( :byte =mask, :int =addr -- :byte ) ... ;
```


# Interactive commands

The following are commands that are executed directly when seen as part of an interactive
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
