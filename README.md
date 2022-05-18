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

