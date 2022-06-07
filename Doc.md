# Introduction

*2022-06-07 v0.2.7*

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
```

Puts 5 on the data stack.

```
DDRB
```

Runs word DDRB and puts value 0x24 on data stack.



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


## Logical expressions (returns :bool)

```
> < >= <= == !=

and              # Logical and 
or               # Logical or
not              # Logical not
```


## Flow control

```
return           # return to where current word was called
abort            # terminates execution
```


## Access to memory (registers)

```
write            ( :byte =value :int =address -- )
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
:sym                 # literal tokens: 'something
:addr
:bool
```

Note that type casts also strip bits from the value, except the :bool, which converts the value to 0 or 1,
based on whether the number on the stack is equal to zero or not. 

Ex.

```
1024 :byte
	# returns 0 as the lower 8 bits of 1024 are all zeroes
	
1024 :bool :byte
	# returns 1, as :bool sees a value that is non-zero as boolean true, which is in turn cast
	# as a byte to value 1
```

In addition to type casting, the type cast functions are also checks, which may fail if the cast is
invalid.

Currently only :sym and :addr can not be converted to any of the int types, nor vice-versa. Note however
that :sym values *are a subtype* of :addr values.

The :addr type is, as of v0.2.4, still undergoing development.


### Default value type :long

The default value type, for example after adding or subtracting, is signed long, which for the Arduino (Nano) means a
4 byte, 32-bit value. This can be cast to other integer types, ranging from :ulong down to :bool. 



## Misc

```
millis        ( -- :ulong )                       # millis since program start
```

## AVR On-chip EEPROM

```
ee:read       ( :uint =addr -- :byte )
ee:write      ( :byte =value :uint =addr -- )
```

## SPI

```
spi:begin     ( :bool =msbFirst :uint =addr -- )
spi:transfer  ( :byte =value -- :byte )
spi:end       ( -- )
```
## I2C / TWI

```
twi:begin     ( -- )
twi:tr:begin  ( :long =addr -- )
twi:write     ( :byte =value -- )
twi:tr:end    ( -- :byte )
twi:request   ( :uint =count -- )
twi:read      ( -- :byte )
twi:setclock  ( :ulong =freq -- )
```


# Literal values in code

```
3
0x03
b0011
b0000_0011

'literal-token

null

true
false
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

Also, to abort execution completely, use the abort function. In order to
provide info about what failed, it may be nice to but a literal word on
the stack, such as the name of the word:

```
: sanityCheck 1 2 > if{ 'sanityCheck abort } ;
```


# Types

RForth has a type system. The data stack is based on 4 byte longs, which can be mapped
to other numeric types. 

The point of types is that CForth has an extended version of (...) notation, that in
regular Forth is used to describe stack inputs and outputs from a word, but seemingly (?)
mostly just a comment.

```
: someWord ( ... -- ...)
```

In RForth, the (...) notation has a strict syntax, that both functions as documentation
and useful code.


# Word functions - stack description

Having introdued local variables, it made sense using the ( ... ) notation 
to assign local variables, and using types for extra information about those, 
as well as basic type checking.

*As of v0.1.8 this is partially implemented* : the input parameters part
is implemented but anything following the optional "--" is ignored, and so 
is now purely of informational value in the source code.

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
: HasBit ( :byte =mask :int =addr -- :byte ) ... ;
```

The compiler verifies that the content inside is on the format :something =something ...

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
