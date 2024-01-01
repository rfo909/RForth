RFOrth v2
---------
v0.0.0a

RFOrth v2 is a Forth bytecode compiler/interpreter, with local variables 
instead of DUP and SWAP and ROTATE and all that stuff.

The aim is to create a fully working Forth for the Raspberry Pi Pico, and also
to experiment with features such as

- struct's
- lists
- objects
- hierarchical dictionaries / dotted lookup


Basics
------

The language uses a cell size of 2 bytes, although aiming at running on a 4 byte
architecture. The data stack and local variables are 4 bytes, while the heap
space (which is a single malloc in C) uses two byte references, allowing for
a max heap of 64k.

All symbols in the language, that is, all words, but also static strings, are
stored in a *symbol storage* inside the heap. This means all words get reduced
to a unique two byte reference. 

When compiling, we don't do *strcmp* but instead compare integer values representing
different symbols, such as the COLON and SEMICOLON etc.

We use the word Ref to talk about references inside the designated heap, as
they are two byte unsigned integers. 

The parser is modified into Recursive Descent, instead of the flat model depending
on stacks. 

The main loop interacts with the TIB (Terminal Input Buffer) which is a circular
buffer managed in C, via a number of TIBx words in Forth. Examining characters
forward from current position may block until new input comes in (over serial or
otherwise).

Op-codes
--------

These op-codes are executed by the interpreter.
```


# Control flow
	
JMP				# ( ref -- )
COND_JMP		# ( ref cond -- )
CALL 			# ( ref -- )		# push frame on call stack (CS)
RETURN					# pop frame off call stack


PANIC			# ( symbol -- )

		# reset input buffer, clear data- and call stacks, invoke :MainLoop   
		
		# See :PANIC in ACode.txt - it logs and calls PANIC
		# When we invoke :MainLoop (pointed to from ref at :MAIN_LOOP) it
		# detects if there has been been allocated uncommitted memory from the heap,
		# and attempts to clean up


# Heap

HEAP_MAX		# return two byte value for total number of bytes on heap (max 64k)

# TIB = Terminal Input Buffer

TIBs	# Start index - returns start character
TIBr    # read index - returns character
TIBr+   # advance TIBr
TIBs+   # advance TIBc - if TIBr==TIBc it gets advanced too
TIBr<   # move TIBr back to TIBs
TIBs>   # move TIBs forward to TIBr
	# TIBr is the pointer we use to examine input
	# TIBs is the pointer we use to remember the start
	# The system write pointer can advance to TIBs-1 (modulo TIB_SIZE)
	# Advancing TIBs/TIBr past last character from serial blocks the interpreter (in Forth)
TIBW	# Get symbol reference for string [TIBs - TIBr> return Ref


# Print to stdout

EMIT # ( char -- )
EMITWORD  # ( symbolRef -- ) - null-terminated string

# The following op-codes are emitted as bytes
# from the word compiler in ACode.txt, but are
# also used directly int the code in that file
# --------------------------------------------

ADD
SUBTRACT
MUL
DIV
MOD

RIGHTSHIFT	# ( value n -- value )
LEFTSHIFT	# ( value n -- value )

AND   # Bitwise, so that false=0, true=0xffffffff (-1 signed long)
OR
NOT

EQ
NE
GT
LT
GE
LE


# Data stack

DROP

# regular stack operations are replaced by the local variables $n
# so no dup, swap or rotate etc


# Local variables

SETLOCAL		# ( value symbol -- )
GETLOCAL		# ( symbol -- value )

WORD			# ( Ref -- )
	# for making new word from substring of existing word


# Read and write different number of bytes
# --

WRITE1 # ( value addr -- )
WRITE2 
WRITE4 

READ1  # ( addr -- value )
READ2
READ4 

# Global read and write, outside of Forth heap - bytes only
# -----

READ1g		
WRITE1g 

# Literal values in code

LITERAL1  # followed by 1, 2 or 4 bytes
LITERAL2
LITERAL4


CHECK_PARSE	# ( symbol -- bool )
PARSE  # ( word -- number )  - recognizes decimal, 0x0a, b1010


```

Status
------


The bulk of the C code is in place, except for local variables in Forth, which
will live on the temp stack. 

Have created a simple main loop stepping one instruction
at a time, showing the name of the instruction, and blocking on serial for each
step ahead.

Quite a bit of debugging remains, to see that the code executes as intended. 




