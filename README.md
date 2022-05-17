# RForth

Having read about Forth on microcontrollers, I decided to write my own.

Had to re-learn C and C++ but ran into troubles with memory management, and
ended up with a pure C implementation. 

# Why?

For fun, and for doing interactive programming on the tiny little Arduino
Nano / Uno, with its 2Kb of SRAM. 

# Examples?

```
: PINB 0x23 ;
: DDRB 0x24 ;
: PORTB 0x25 ;

: mask =count 1 count << ;
```

With these four words, we can test turning on and off the LED on pin 13.

Note that the parser waits until it sees a semicolon even in interactive mode,
before parsing:

```
: M 5 mask ;

M DDRB write ;  # set pin 13 to output
M PORTB write ;  # turn led on
0 PORTB write ;  # turn led off (together with the other 7 pins on the port)

: on PORTB read M | PORTB write ;
: off PORTB read M ! & PORTB write ;

on ;
off ;
on ;
off ;

```

The =x define up to 8 differently named local variables inside 
each word function.

When the stack gets too big, type "clear" followed by semicolon.

