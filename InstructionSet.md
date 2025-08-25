2024-08-25


```
Assembler:IDoc

   0: !        | write word
   1: .str     | print string
   2: <<       | left shift  ( val n -- val )
   3: >>       | right shift  ( val n -- val )
   4: @        | read word
   5: HERE     | ( -- value )
   6: W+       | ( n -- n+WORDSIZE )
   7: a        | read local variable a ( -- value )
   8: a!       | set local variable a ( value -- )
   9: add      | ( a b -- a+b )
  10: allot    | ( N -- )
  11: and      | boolean ( a b -- 0|1 )
  12: andb     | binary and ( a b -- c )
  13: b        | read local variable b ( -- value )
  14: b!       | set local variable b ( value -- )
  15: c        | read local variable c ( -- value )
  16: c!       | set local variable c ( value -- )
  17: call     | ( addr -- )
  18: cpush    | push value from data stack on call stack ( x -- )
  19: cr       | print newline
  20: dcopy:   | ( a b c 1 -- a b c b )
  21: dget:    | (a b c 1 -- a c b)
  22: div      | ( a b -- a/b )
  23: drop     | ( a -- )
  24: dump     | show stack contents ( -- )
  25: dup      | ( x -- x x )
  26: eq       | ( a b -- 0|1 )
  27: ge       | ( a b -- 0|1 )
  28: global   | get global variable ( N -- value)
  29: global!  | set global variable ( value N -- )
  30: gt       | ( a b -- 0|1 )
  31: halt     | terminate execution
  32: inv      | binary not  ( a -- b )
  33: jmp      | ( addr -- ) updates PC
  34: jmp?     | ( cond addr -- ) conditionally updates PC
  35: le       | ( a b -- 0|1 )
  36: lt       | ( a b -- 0|1 )
  37: memcpy   | ( ptrSrc ptrTarget bytecount -- )
  38: mul      | ( a b -- a*b )
  39: n2code   | ( val addr -- N ) convert value to code, written to addr, returns number of bytes written
  40: n2code3  | ( val addr -- ) convert value (0-16k) to 3 code bytes, written to addr+
  41: ne       | boolean ( a b -- 0|1 )
  42: not      | boolean ( x -- 0|1 )
  43: null     | ( -- 0 ) pushes a single zero on the stack
  44: or       | boolean ( a b -- 0|1 )
  45: orb      | binary or ( a b -- c )
  46: over     | ( a b -- a b a )
  47: print    | print top value on hex format
  48: print#   | print top value on decimal format
  49: printb   | print top value on binary format
  50: printc   | print byte as character
  51: readb    | read byte ( addr -- value )
  52: readc    | ( -- byte ) - read single character - blocking
  53: ret      | return
  54: show     | ( sym sym ... count -- )
  55: streq    | compare strings, return 0 if not and 1 if equal
  56: sub      | ( a b -- a-b )
  57: swap     | ( a b -- b a )
  58: sym2s    | convert symbol offset to string pointer
  59: wordsize | ( -- N )
  60: writeb   | write byte ( addr -- value )

```

