2024-08-25


```
Assembler:IDoc

   0: !        | write word
   1: .str     | print string
   2: 2W+      | ( m -- n+WORDSIZE*2 )
   3: <<       | left shift  ( val n -- val )
   4: >>       | right shift  ( val n -- val )
   5: @        | read word
   6: HERE     | ( -- value )
   7: W+       | ( n -- n+WORDSIZE )
   8: a        | read local variable a ( -- value )
   9: a!       | set local variable a ( value -- )
  10: add      | ( a b -- a+b )
  11: allot    | ( N -- )
  12: and      | boolean ( a b -- 0|1 )
  13: andb     | binary and ( a b -- c )
  14: b        | read local variable b ( -- value )
  15: b!       | set local variable b ( value -- )
  16: c        | read local variable c ( -- value )
  17: c!       | set local variable c ( value -- )
  18: call     | ( addr -- )
  19: cpush    | push value from data stack on call stack ( x -- )
  20: cr       | print newline
  21: dcopy:   | ( a b c 1 -- a b c b )
  22: dget:    | (a b c 1 -- a c b)
  23: div      | ( a b -- a/b )
  24: drop     | ( a -- )
  25: dump     | show stack contents ( -- )
  26: dup      | ( x -- x x )
  27: eq       | ( a b -- 0|1 )
  28: ge       | ( a b -- 0|1 )
  29: global   | get global variable ( N -- value)
  30: global!  | set global variable ( value N -- )
  31: gt       | ( a b -- 0|1 )
  32: halt     | terminate execution
  33: inv      | binary not  ( a -- b )
  34: jmp      | ( addr -- ) updates PC
  35: jmp?     | ( cond addr -- ) conditionally updates PC
  36: le       | ( a b -- 0|1 )
  37: lt       | ( a b -- 0|1 )
  38: memcpy   | ( ptrSrc ptrTarget bytecount -- )
  39: mul      | ( a b -- a*b )
  40: n2code   | ( val addr -- N ) convert value to code, written to addr, returns number of bytes written
  41: n2code3  | ( val addr -- ) convert value (0-16k) to 3 code bytes, written to addr+
  42: ne       | boolean ( a b -- 0|1 )
  43: not      | boolean ( x -- 0|1 )
  44: null     | ( -- 0 ) pushes a single zero on the stack
  45: or       | boolean ( a b -- 0|1 )
  46: orb      | binary or ( a b -- c )
  47: over     | ( a b -- a b a )
  48: print    | print top value on hex format
  49: print#   | print top value on decimal format
  50: printb   | print top value on binary format
  51: printc   | print byte as character
  52: readb    | read byte ( addr -- value )
  53: readc    | ( -- byte ) - read single character - blocking
  54: ret      | return
  55: show     | ( sym sym ... count -- )
  56: streq    | compare strings, return 0 if not and 1 if equal
  57: sub      | ( a b -- a-b )
  58: swap     | ( a b -- b a )
  59: sym2s    | convert symbol offset to string pointer
  60: wordsize | ( -- N )
  61: writeb   | write byte ( addr -- value )


```

