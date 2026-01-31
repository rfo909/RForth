2025-11-28



```
!        | (value addr -- ) write cell
!=       | ( a b -- 0/1 )  boolean - ne
&        | ( a b -- c ) - binary and
&&       | ( a b -- 0|1 ) - boolean and
*        | ( a b -- a*b ) mul
+        | ( a b -- a+b ) add
-        | ( a b -- a-b ) sub
.str     | print string
/        | ( a b -- a/b ) div
%        | ( a b -- a%b ) modulo
1+       | ( n -- n+1 )
<        | ( a b -- 0|1 ) lt
<<       | left shift  ( val n -- val )
<=       | ( a b -- 0|1 ) le
==       | ( a b -- 0|1 ) eq
>        | ( a b -- 0|1 ) gt
>=       | ( a b -- 0|1 ) ge
>>       | right shift  ( val n -- val )
@        | ( addr -- value ) read cell
CELL+    | ( n -- n+WORDSIZE )
HERE     | ( -- value )
PANIC    | ( -- ) Clean up stacks and set PC=0
PC       | ( -- addr )
allot    | ( N -- ) increase HERE value by N
atoi     | ( strPtr targetPtr -- bool ) decimal, 0x... hex b... binary
call     | ( addr -- )
cellsize | ( -- N )
cget     | ( index -- value ) get value by index from call stack, indexed from call frame base
clear    | Clear stack
cpush    | ( value -- ) push value from data stack on call stack ( x -- )
cr       | print newline
crget    | (--addr) get return address
cset     | (value index -- ) set value in stack frame (must have been cpush'ed first)
dcall    | (addr-ptr -- ) Call NORMAL word from dictionary, uses Addr-ptr-ptr in order to support redef of words
drop     | ( a -- )
dump     | show stack contents ( -- )
dup      | ( x -- x x )
halt     | terminate execution
inv      | ( a -- b ) - binary not
jmp      | ( addr -- ) updates PC
jmp?     | ( cond addr -- ) conditionally updates PC
key      | (--byte) - read single character - non-blocking - 0 if no char
memcpy   | ( ptrSrc ptrTarget bytecount -- )
n2code   | ( num addr nbytes -- N ) num to code, write to addr, nbytes=0 means variable, returns #bytes
native   | ( ... addr -- int? ) - call native function, see nativec
nativec  | ( str -- addr ) - compile native reference into numeric format
not      | boolean ( x -- 0|1 )
null     | ( -- 0 ) pushes a single zero on the stack
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
swap     | ( a b -- b a )
u2spc    | convert underscore to space in string in RAM
writeb   | ( value addr -- ) write byte
|        | ( a b -- c ) - binary or
||       | ( a b -- 0|1 ) - boolean or
```

