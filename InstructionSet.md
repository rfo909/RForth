2024-08-31


```
Assembler:IDoc

   0: !        | write word
   1: .str     | print string
   2: <<       | left shift  ( val n -- val )
   3: >>       | right shift  ( val n -- val )
   4: @        | read word
   5: HERE     | ( -- value )
   6: PANIC    | ( -- ) Clean up stacks and set PC=0
   7: PC       | ( -- addr )
   8: W+       | ( n -- n+WORDSIZE )
   9: a        | read local variable a ( -- value )
  10: a!       | set local variable a ( value -- )
  11: add      | ( a b -- a+b )
  12: allot    | ( N -- )
  13: and      | boolean ( a b -- 0|1 )
  14: andb     | binary and ( a b -- c )
  15: b        | read local variable b ( -- value )
  16: b!       | set local variable b ( value -- )
  17: c        | read local variable c ( -- value )
  18: c!       | set local variable c ( value -- )
  19: call     | ( addr -- )
  20: call?    | 
  21: clear    | Clear stack
  22: cpush    | push value from data stack on call stack ( x -- )
  23: cr       | print newline
  24: dcopy:   | ( a b c 1 -- a b c b )
  25: dget:    | (a b c 1 -- a c b)
  26: div      | ( a b -- a/b )
  27: drop     | ( a -- )
  28: dump     | show stack contents ( -- )
  29: dup      | ( x -- x x )
  30: eq       | ( a b -- 0|1 )
  31: ge       | ( a b -- 0|1 )
  32: global   | get global variable ( N -- value)
  33: global!  | set global variable ( value N -- )
  34: gt       | ( a b -- 0|1 )
  35: halt     | terminate execution
  36: inv      | binary not  ( a -- b )
  37: jmp      | ( addr -- ) updates PC
  38: jmp?     | ( cond addr -- ) conditionally updates PC
  39: le       | ( a b -- 0|1 )
  40: lt       | ( a b -- 0|1 )
  41: memcpy   | ( ptrSrc ptrTarget bytecount -- )
  42: mul      | ( a b -- a*b )
  43: n2code   | ( num addr nbytes -- N ) num to code, write to addr, nbytes=0 means variable, returns #bytes
  44: ne       | boolean ( a b -- 0|1 )
  45: not      | boolean ( x -- 0|1 )
  46: null     | ( -- 0 ) pushes a single zero on the stack
  47: or       | boolean ( a b -- 0|1 )
  48: orb      | binary or ( a b -- c )
  49: over     | ( a b -- a b a )
  50: print    | print top value on hex format
  51: print#   | print top value on decimal format
  52: printb   | print top value on binary format
  53: printc   | print byte as character
  54: rback?   | ( bool offset -- ) subtracts offset from PC - for relocatable forth word code
  55: readb    | read byte ( addr -- value )
  56: readc    | ( -- byte ) - read single character - blocking
  57: ret      | return
  58: rfwd?    | ( bool offset -- ) adds offset to PC - for relocatable forth word code
  59: show     | ( sym sym ... count -- )
  60: streq    | compare strings, return 0 if not and 1 if equal
  61: sub      | ( a b -- a-b )
  62: swap     | ( a b -- b a )
  63: sym2s    | convert symbol offset to string pointer
  64: wordsize | ( -- N )
  65: writeb   | write byte ( addr -- value )

```

