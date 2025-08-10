
2025-08 RFO v3

# NOTE: the C code is completely obsolete with this new design.
# Must first implement an interpreter in CFT, to verify the code.

The big change compared to the previous attempt is that we operate with local
tags between each of the global ones. This simplifies code greatly.

Another major change is leaving the data stack and the call stack to the C implementation,
instead of managing it all in code, and how one extra instruction cpush opens 
up for local variables. 

Also, defining the system from the bottom up, and employing CONS objects for 
dynamic data structures. 

And the inlines which implement the crucial call and ret (pseudo-)instructions,
by inlining code reading the PC etc. 

---

Using single byte opcodes means our "machine language" is more space efficient
than real machine code, and also tailored for a stack machine. 

-----------------------------------
Introduction
-----------------------------------


Instead of creating a virtual register machine, with instructions encoded
with parameters, instead we we have 
	- a data stack
	- a call stack
	- a program counter
	- single-byte instructions

Even literal numbers consist of single byte instructions, operating on
the data stack. They are always converted to hex, and emitted as code as
follows:

	xNNN...
	
For jump addresses we always use 4 digits (two bytes), which in the code
becomes xNNNN. This gives us an address space of 64Kb. 

Even if local tag jumps probably never will need more than a single byte 
relative offset, using global addresses simplifies the jmp instructions.

The "x" instruction simply pushes a zero on the stack, and each digit 
0-9A-F are also instructions, which multiply the value on the stack with 16 
before adding the value of that digit. 

Then there are some inlines (macros in a masm): 
	- call
	- ret
	- WORDSIZE
	- a b c d e f  (local variables)
	- (see Inlines function)
	
Local variables: see cpush. 

Example of call and ret

:Main
	#2 #3 &SomeFunc call print  ;; should show 5 
	halt
	
:SomeFunc
	add ret

Numbers are written #nnn for decimal or xNNN for hex, and are always
encoded as xNNN. The 'x' puts a zero on the stack, while the digits
are supposed to multiply it with 16 and add the digit value.

Also added support for symbols using a single tick, like 'x - these
are written after the :DATA tag.

Functions are tagged with global tags, like 

	:Main      ;; defines tag
	&Main        ;; tag lookup, results in code xNNNN being generated.

Inside functions, we support local tags, like

	:*Else     ;; defines tag
	&*Else     ;; tag lookup, results in xNNNN being generated

All tag lookups are added to a patch list, while generating as x0000 while
traversing the words. Then, we iterate over the patch list and insert the
actual values everywhere.

The compiled code is kept readable, so it can be copy-pasted into a serial
console at some point.

We even support local variables via the cpush instruction which pushes N
values from the stack onto the call stack frame. N can be 1-5. The values
which are always words, can be accessed as a, b, c, d or e, which are
inline functions, calculating the offset from cbase, returning pointers.


