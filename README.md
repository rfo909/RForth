Forth never ceases to fascinate !
=================================

Version 3 (alpha)

2025-08-16 RFO

It's been said that the best way of understanding Forth is to build one
yourself. This project aims at the Pi Pico as the run platform,
simply because the toolchain and the deploy is so easy.

It is unlikely this Forth will ever be used for anything but the experience
of bringing up a full language from nothing, since libraries and interfacing
hardware at the lowest level, is an entirely different matter.

The effort so far consists of making a byte code assembler, for a stack based
instruction set, while developing using a simple base code source file written
in this assembly language, and verifying it using a stand-alone interpreter.

CFT assembler
-------------

The assembler is written as a [CFT](https://github.com/rfo909/CFT) script, 
which is a script language I've written myself (in Java). It is perfect for
prototyping (as well as daily tool). The assembler reads the source file
and generates a sequence of single byte instructions.

An initial design decision to use printable non-space characters for instructions,
effectively reserved the range 33-127 for normal opcodes. The original design
inlined numeric constants by defining the following single byte instructions:

- "x" to push zero on the stack
- "0-9A-F" to multiply value on stack by 16 before adding 0-15.

This worked, but the problem was it grew the generated code, as the wordsize
is 2 bytes, and jump addresses are as well. That meant that any tag lookup
resulted in xNNNN in the bytecode, using 5 bytes.

With "normal" opcodes kept in a defined range, the modified scheme was to
put all numeric constants as a list of 2-byte values, and refer to these
by opcodes 128-255. So now any numeric constant is pushed on the stack
using a single byte. 

This means the number of unique tags plus the number of unique symbols ("non-space strings"),
plus the number of unique numeric constants in the code, must not exceed 127.


CFT interpreter
---------------

The initial interpreter, for testing and stepping through code, is also 
written in CFT. This has been invaluable for validating the assembler. It
correctly simulates a byte oriented memory, on which the three stacks of the
language also live. 

After sectioning off constant values into a separate sequence of bytes, and
also supporting symbols (non-space strings) for reporting and dictionary 
purposes, the three sequences of bytes are strung together to form a 
single hex-string, which is the input to the interpreter along with
offsets for where things are found.

The interpreter, when it starts, allocates bytes in simulated memory
and put the bytes from the hex-string into it. The code starts at PC=0.

Then it allocates the three stacks, as well as some few words of storage
that serve the "global" variables. This is a mechanism for creating the
very first global structures, and currently a maximum of 4 is supported,
although really just one would suffice.

After adding all this to the simulated memory, the HERE value (top of
heap) is stored internally, as a barrier for the read and write instructions, 
leaving them to work only from this address and up. In effect we've created
a combined code and system data segment that isn't freely available to
the "assembly language".


Local variables
---------------

The assembly language supports pushing values from the data stack to the call
stack, with the cpush instruction. The first three values pushed that way
get avaiable as local variables a, b and c.

```
	45 cpush   # Take value from data stack, push onto call stack
	           # The first value is denoted a, the second b, and the third c
	          
	a          # Read value of variable a (45)
	a 1 add a! # Update variable a to 46
	
	(same for b and c)
```

With the cpush and the variable operations being single byte instructions, 
the generated code compactness rivals the use of traditional stack operations, 
like dup and swap. Those are implemented as well, as single byte instructions.

Using cpush to push more than 3 values has no meaning, and might be considered
a bug, as there is no way from inside the language to access those.

Doing update of a variable with a! (or b/c) without cpush'ing a value first, is risky,
because those words may be overwritten if calling some subroutine.

Using the [abc] lookup instructions without cpush is a hackish way of
accessing local variables of the last called subroutine.

We settled on 3 local variables, since if more are needed, in conjunction
with 2 on the stack, the function is too complex.

Stack operations
----------------

The dup, swap, over and drop operations are implemented as single-byte instructions, but there
are a couple more.

Specifically created dcopy and dget, which are general enough to implement rot and other stuff. 

Experience so far seems to be that dup, swap, combined with using local variables
is what gives the best code.

The three stacks
----------------
The most used stack is the data stack. Most operations interact with it. It stores single words.

Then there is the call stack. It is also a stack of words. When doing a call to a subroutine,
the return address is pushed on the call stack. But in addition, the call stack is where
up to 3 local variables can be created with cpush. To control data content on the call stack,
we added a third stack:

The frame stack keeps track of what belongs to which call frame on the call stack. It contains
pairs of words, called base and size. The base is the position on the call stack (counted from
the bottom), where the current frame starts, and the size is the number of words pushed on
the call stack in the current frame.

Creating a local variable by calling cpush increases size by 1, and so does calling some
function, as we then push the return position (PC+1) on the call stack. When returning
from a call (ret), we have all the information we need to locate the return position. It
is removed from the call stack, and size for the frame is decremented by one, before
updating the PC, effectively doing a jump.

Disassembler
------------
The Assembler script also contains a disassembler that is immediately applied to the output
from the Assembler, to show what has been done. 

Example source:

```
:Init
	;; create variables in normal memory, 
	HERE 0 global!
	2 wordsize mul allot
	&Main jmp

:ConsHead
	0 global ret
	
:DictHead
	0 global W+ ret
	
:Main
 	cr 'HERE ': 2 show HERE print 
 	
	&ConsGet call
	&ConsDispose call
	&ConsGet call

 	cr 'After ': 'HERE ': 4 show HERE print 
 	
	
:Halt 
	halt
	&Halt jmp



;; Generalized free list mechanism
;; -------------------------------

;; Pop an element off the freelist, or null if freelist empty

:FreePop   ; ( freeHead -- null|ptr )
	cpush			;; a=address of freelist head word
	a @ dup cpush null ne &*notEmpty jmp?  ;; b=address of first element in free list
		null ret

	:*notEmpty
	b @ a!			;; store b.next (first word) into freelist head
	b ret

;; Push an element on the freelist


:FreePush ; ( dataAddr freeHead -- )
	cpush		;; a=freeHead
	cpush		;; b=dataAddr

	a @ b !		;; store freeHead into dataAddr.next (first word)
	b a !		;; store dataAddr in freeHead
	
	ret
	
	
	
	
	
;; Global data structures depend on
;; ConsHead and DictHead, which point to words on the heap 
;; (see Init)
;; ---------------------------------------------------------	

;; Dictionary stuff
;;	
	
; (TODO)

;; CONS cells are created as a two-word CAR+CDR tuple. They are allotted
;; as needed, but also freelisted when disposed off. To calculate the CDR address
;; from the Cons pointer: W+


:ConsGet  ; ( -- ConsPtr) - get Cons cell from free list (call ConsAlloc first if empty)
	&ConsHead call cpush     ;; a=&ConsHead pointer
	a &FreePop call cpush ;; b=ptr/null
	b null ne &*ok jmp?
		HERE b!				;; a=ptr new cons
		wordsize 2 mul allot  ;; reserve the space
	:*ok
	b ret

	
:ConsDispose  ;; ( ptr -- )
	&ConsHead call &FreePush call ret

```



Output:

```
   0: 0000  | :Init HERE              |       | HERE       
   1: 0001  | 0                       | [128] | push 0x0000
   2: 0002  | global!                 |       | global!    
   3: 0003  | 2                       | [129] | push 0x0002
   4: 0004  | wordsize                |       | wordsize   
   5: 0005  | mul                     |       | mul        
   6: 0006  | allot                   |       | allot      
   7: 0007  | &Main                   | [130] | push 0x0010
   8: 0008  | jmp                     |       | jmp        
   9: 0009  | :ConsHead 0             | [128] | push 0x0000
  10: 000A  | global                  |       | global     
  11: 000B  | ret                     |       | ret        
  12: 000C  | :DictHead 0             | [128] | push 0x0000
  13: 000D  | global                  |       | global     
  14: 000E  | W+                      |       | W+         
  15: 000F  | ret                     |       | ret        
  16: 0010  | :Main cr                |       | cr         
  17: 0011  | 'HERE                   | [128] | push 0x0000
  18: 0012  | ':                      | [133] | push 0x0006
  19: 0013  | 2                       | [129] | push 0x0002
  20: 0014  | show                    |       | show       
  21: 0015  | HERE                    |       | HERE       
  22: 0016  | print                   |       | print      
  23: 0017  | &ConsGet                | [134] | push 0x0043
  24: 0018  | call                    |       | call       
  25: 0019  | &ConsDispose            | [135] | push 0x0057
  26: 001A  | call                    |       | call       
  27: 001B  | &ConsGet                | [134] | push 0x0043
  28: 001C  | call                    |       | call       
  29: 001D  | cr                      |       | cr         
  30: 001E  | 'After                  | [131] | push 0x0009
  31: 001F  | ':                      | [133] | push 0x0006
  32: 0020  | 'HERE                   | [128] | push 0x0000
  33: 0021  | ':                      | [133] | push 0x0006
  34: 0022  | 4                       | [136] | push 0x0004
  35: 0023  | show                    |       | show       
  36: 0024  | HERE                    |       | HERE       
  37: 0025  | print                   |       | print      
  38: 0026  | :Halt halt              |       | halt       
  39: 0027  | &Halt                   | [137] | push 0x0026
  40: 0028  | jmp                     |       | jmp        
  41: 0029  | :FreePop cpush          |       | cpush      
  42: 002A  | a                       |       | a          
  43: 002B  | @                       |       | @          
  44: 002C  | dup                     |       | dup        
  45: 002D  | cpush                   |       | cpush      
  46: 002E  | 0                       | [128] | push 0x0000
  47: 002F  | ne                      |       | ne         
  48: 0030  | &*notEmpty              | [139] | push 0x0034
  49: 0031  | jmp?                    |       | jmp?       
  50: 0032  | 0                       | [128] | push 0x0000
  51: 0033  | ret                     |       | ret        
  52: 0034  | :*notEmpty b            |       | b          
  53: 0035  | @                       |       | @          
  54: 0036  | a!                      |       | a!         
  55: 0037  | b                       |       | b          
  56: 0038  | ret                     |       | ret        
  57: 0039  | :FreePush cpush         |       | cpush      
  58: 003A  | cpush                   |       | cpush      
  59: 003B  | a                       |       | a          
  60: 003C  | @                       |       | @          
  61: 003D  | b                       |       | b          
  62: 003E  | !                       |       | !          
  63: 003F  | b                       |       | b          
  64: 0040  | a                       |       | a          
  65: 0041  | !                       |       | !          
  66: 0042  | ret                     |       | ret        
  67: 0043  | :ConsGet &ConsHead      | [131] | push 0x0009
  68: 0044  | call                    |       | call       
  69: 0045  | cpush                   |       | cpush      
  70: 0046  | a                       |       | a          
  71: 0047  | &FreePop                | [138] | push 0x0029
  72: 0048  | call                    |       | call       
  73: 0049  | cpush                   |       | cpush      
  74: 004A  | b                       |       | b          
  75: 004B  | 0                       | [128] | push 0x0000
  76: 004C  | ne                      |       | ne         
  77: 004D  | &*ok                    | [141] | push 0x0055
  78: 004E  | jmp?                    |       | jmp?       
  79: 004F  | HERE                    |       | HERE       
  80: 0050  | b!                      |       | b!         
  81: 0051  | wordsize                |       | wordsize   
  82: 0052  | 2                       | [129] | push 0x0002
  83: 0053  | mul                     |       | mul        
  84: 0054  | allot                   |       | allot      
  85: 0055  | :*ok b                  |       | b          
  86: 0056  | ret                     |       | ret        
  87: 0057  | :ConsDispose &ConsHead  | [131] | push 0x0009
  88: 0058  | call                    |       | call       
  89: 0059  | &FreePush               | [140] | push 0x0039
  90: 005A  | call                    |       | call       
  91: 005B  | ret                     |       | ret       ```
```


References
----------

- [Instruction set](InstructionSet.md) for regular opcodes 33-127


