# RForth

Having read about Forth on microcontrollers, I decided to write my own.

Had to re-learn C and C++ but ran into troubles with memory management, and
ended up with a pure C implementation. 

# Why?

For fun, and for doing interactive programming on the tiny little Arduino
Nano / Uno, with its 2Kb of SRAM. 

# Examples?

```
: DDRB 0x24 ;
: PORTB 0x25 ;

: mask =count 1 count << ;
: M 5 mask ;

```

With these four words, we can test turning on and off the LED on pin 13.

Note that the parser waits until it sees a semicolon even in interactive mode,
before parsing:

```
# set pin as output (DDR = data direction register)
# then write high to pin 5 in port B (led on)
# and 0 to port B (led off). Note that this nulls
# all the other pins on port B as well.

M DDRB write ;  
M PORTB write ; 
0 PORTB write ; 

# An improvement, to affect only the desired bit, creating
# two words, on and off:

: on PORTB read M | PORTB write ;
: off PORTB read M ! & PORTB write ;


on ;
off ;
on ;
off ;

(endless fun)

```

The =x define up to 8 differently named local variables inside 
each word function. After entering a function, or immediate code, 
the compiled byte-code is displayed.

When the stack gets too big, type "clear" followed by semicolon.

# Conditionals and loops

```
# If: 

: set =val val if{ on } val not if{ off } ;

# Now we can do

1 set
0 set

# Looping
0 =a loop{ a 1 + =a a 10 > break a } ;

# Puts values 1-10 on stack. Remember to clear
# stack after, because max data stack depth is limited.

```


# Compiler - byte code

Mainly due to space considerations, but possibly also speed, all input is 
compiled to byte code, both when we define new words and when we enter code
for immediate exeution.


When entering immedate code (that does not start with colon) the code is compiled,
then disassembled to screen, before being run.

To disassemble a word, use the "dis:xxx" command:

```
dis:TheWord ; 
```

## Blink 

An example of a small program for the atmega328p is to blink the LED on
pin 13, or as the MCU knows it, on bit 5 of PORTB.

```
: DDRB 0x24 ;
: PORTB 0x25 ;

: mask =count 1 count << ;
: M 5 mask ;

: init DDRB read M | DDRB write ;

init ;

: on PORTB read M | PORTB write ;
: off PORTB read M ! & PORTB write ;

: wait =ms millis ms + =ms loop{ millis ms > break  } ;

: blink loop{ on 1000 wait off 500 wait } ;
```

Entering the blink word starts an infinite loop that blinks the LED.

But before doing that, we may want to inspect the compiled code,
by typing 
```
dis:blink ;

  0  151  PUSH 23
  1  2  OP_CALL
  2  135  PUSH 7
  3  135  PUSH 7
  4  27  OP_LSHIFT
  5  232  PUSH 104
  6  30  OP_B_OR
  7  172  PUSH 44
  8  2  OP_CALL
  9  161  PUSH 33
  10  2  OP_CALL
  11  131  PUSH 3
  12  135  PUSH 7
  13  27  OP_LSHIFT
  14  244  PUSH 116
  15  30  OP_B_OR
  16  172  PUSH 44
  17  2  OP_CALL
  18  128  PUSH 0
  19  3  OP_JMP
  20  0  OP_EOF
```

The instruction 0-1 is the call to "on". Instructions 3-6 push 1000 on the stack,
and instruction 7-8 call "wait". Then instructions 9-10 call the "off" word.

Then follow code to push 500 on the stack, before calling wait again, in 
instructions 16-17. Instruction 18-19 perform a jump back to instruction 0, repeating
the loop forever.


