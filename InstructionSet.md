2024-07-16

```
Assembler:IDoc

   0: !        | write word
   1: <<       | left shift  ( val n -- val )
   2: >>       | right shift  ( val n -- val )
   3: @        | read word
   4: HERE     | ( -- value )
   5: W+       |   ( n -- n+WORDSIZE )
   6: a        | read local variable a ( -- value )
   7: a!       | set local variable a ( value -- )
   8: add      | ( a b -- a+b )
   9: allot    |  ( N -- )
  10: and      | boolean ( a b -- 0|1 )
  11: andb     | binary and ( a b -- c )
  12: b        | read local variable b ( -- value )
  13: b!       | set local variable b ( value -- )
  14: c        | read local variable c ( -- value )
  15: c!       | set local variable c ( value -- )
  16: call     | ( addr -- )
  17: cpush    | push value from data stack on call stack ( x -- )
  18: cr       | print newline
  19: dcopy:   | ( a b c 1 -- a b c b )
  20: dget:    | (a b c 1 -- a c b)
  21: div      | ( a b -- a/b )
  22: drop     |  ( a -- )
  23: dump     | show stack contents ( -- )
  24: dup      | ( x -- x x )
  25: eq       | ( a b -- 0|1 )
  26: global   | - get global variable ( N -- value)
  27: global!  | - set global variable ( value N -- )
  28: gt       | ( a b -- 0|1 )
  29: halt     | - terminate execution
  30: inv      | binary not  ( a -- b )
  31: jmp      | ( addr -- ) updates PC
  32: jmp?     | ( cond addr -- ) conditionally updates PC
  33: lt       | ( a b -- 0|1 )
  34: mul      | ( a b -- a*b )
  35: ne       | boolean ( a b -- 0|1 )
  36: not      | boolean ( x -- 0|1 )
  37: or       | boolean ( a b -- 0|1 )
  38: orb      | binary or ( a b -- c )
  39: over     | ( a b -- a b a )
  40: print    | print top value on hex format
  41: print#   | print top value on decimal format
  42: printb   | print top value on binary format
  43: readb    | read byte ( addr -- value )
  44: ret      | return
  45: show     | ( sym sym ... count -- )
  46: sub      | ( a b -- a-b )
  47: swap     | ( a b -- b a )
  48: wordsize | ( -- N )
  49: writeb   | write byte ( addr -- value )

```

