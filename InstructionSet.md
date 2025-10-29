2025-10-29

```
Assembler:IDoc

!        | (value addr -- ) write cell
.str     | print string
1+       | ( n -- n+1 )
<<       | left shift  ( val n -- val )
>>       | right shift  ( val n -- val )
@        | ( addr -- value ) read cell
CELL+    | ( n -- n+WORDSIZE )
HERE     | ( -- value )
PANIC    | ( -- ) Clean up stacks and set PC=0
PC       | ( -- addr )
add      | ( a b -- a+b )
allot    | ( N -- ) increase HERE value by N
and      | boolean ( a b -- 0|1 )
andb     | binary and ( a b -- c )
atoi     | ( strPtr targetPtr -- bool ) decimal, 0x... hex b... binary
call     | ( addr -- )
cellsize | ( -- N )
cforce   | (addr -- ) modify stacks, so "ret" returns to given addr
cget     | ( index -- value ) get value by index from call stack, indexed from call frame base
clear    | Clear stack
cpush    | ( value -- ) push value from data stack on call stack ( x -- )
cr       | print newline
crget    | (--addr) get return address
cset     | (value index -- ) set value in stack frame (must have been cpush'ed first)
dcall    | (addr-ptr -- ) Call NORMAL word from dictionary, uses Addr-ptr-ptr in order to support redef of words
div      | ( a b -- a/b )
drop     | ( a -- )
dump     | show stack contents ( -- )
dup      | ( x -- x x )
eq       | ( a b -- 0|1 )
ge       | ( a b -- 0|1 )
gt       | ( a b -- 0|1 )
halt     | terminate execution
inv      | binary not  ( a -- b )
jmp      | ( addr -- ) updates PC
jmp?     | ( cond addr -- ) conditionally updates PC
le       | ( a b -- 0|1 )
lt       | ( a b -- 0|1 )
memcpy   | ( ptrSrc ptrTarget bytecount -- )
mul      | ( a b -- a*b )
n2code   | ( num addr nbytes -- N ) num to code, write to addr, nbytes=0 means variable, returns #bytes
native   | ( ... addr -- int? ) - call native function, see nativec
nativec  | ( str -- addr ) - compile native reference into numeric format
ne       | ( a b -- 0/1 )  boolean
not      | boolean ( x -- 0|1 )
null     | ( -- 0 ) pushes a single zero on the stack
or       | boolean ( a b -- 0|1 )
orb      | binary or ( a b -- c )
over     | ( a b -- a b a )
print    | print top value on hex format
print#   | print top value on unsigned decimal format
print#s  | print top value on signed decimal format
printb   | print top value on binary format
printc   | print byte as character
rback?   | ( bool offset -- ) subtracts offset from PC - for relocatable forth word code
readb    | ( addr -- value ) read byte
readc    | ( -- byte ) - read single character - blocking
ret      | ( -- ) return
rfwd?    | ( bool offset -- ) adds offset to PC - for relocatable forth word code
streq    | compare strings, return 0 if not and 1 if equal
sub      | ( a b -- a-b )
swap     | ( a b -- b a )
u2spc    | convert underscore to space in string in RAM
writeb   | ( value addr -- ) write byte  
```

