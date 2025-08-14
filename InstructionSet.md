2024-07-14

```
Assembler:IDoc

   0: !              | write word
   1: 0              | ( m -- n )  n=16*m + digit value
   2: 1              | ( m -- n )  n=16*m + digit value
   3: 2              | ( m -- n )  n=16*m + digit value
   4: 3              | ( m -- n )  n=16*m + digit value
   5: 4              | ( m -- n )  n=16*m + digit value
   6: 5              | ( m -- n )  n=16*m + digit value
   7: 6              | ( m -- n )  n=16*m + digit value
   8: 7              | ( m -- n )  n=16*m + digit value
   9: 8              | ( m -- n )  n=16*m + digit value
  10: 9              | ( m -- n )  n=16*m + digit value
  11: <<             | left shift  ( val n -- val )
  12: >>             | right shift  ( val n -- val )
  13: @              | read word
  14: A              | ( m -- n )  n=16*m + digit value
  15: B              | ( m -- n )  n=16*m + digit value
  16: C              | ( m -- n )  n=16*m + digit value
  17: D              | ( m -- n )  n=16*m + digit value
  18: E              | ( m -- n )  n=16*m + digit value
  19: F              | ( m -- n )  n=16*m + digit value
  20: HERE           | ( -- value )
  21: SET_DATA_START | ( addr -- )
  22: W+             |   ( n -- n+WORDSIZE )
  23: a              | read local variable a ( -- value )
  24: a!             | set local variable a ( value -- )
  25: add            | ( a b -- a+b )
  26: allot          |  ( N -- )
  27: and            | boolean ( a b -- 0|1 )
  28: andb           | binary and ( a b -- c )
  29: b              | read local variable b ( -- value )
  30: b!             | set local variable b ( value -- )
  31: c              | read local variable c ( -- value )
  32: c!             | set local variable c ( value -- )
  33: call           | ( addr -- )
  34: cpush          | push value from data stack on call stack ( x -- )
  35: cr             | print newline
  36: dcopy:         | ( a b c 1 -- a b c b )
  37: dget:          | (a b c 1 -- a c b)
  38: div            | ( a b -- a/b )
  39: drop           |  ( a -- )
  40: dump           | show stack contents ( -- )
  41: dup            | ( x -- x x )
  42: eq             | ( a b -- 0|1 )
  43: gt             | ( a b -- 0|1 )
  44: inv            | binary not  ( a -- b )
  45: jmp            | ( addr -- ) updates PC
  46: jmp?           | ( cond addr -- ) conditionally updates PC
  47: lt             | ( a b -- 0|1 )
  48: mul            | ( a b -- a*b )
  49: ne             | boolean ( a b -- 0|1 )
  50: not            | boolean ( x -- 0|1 )
  51: null           | push a zero ( -- 0 )
  52: or             | boolean ( a b -- 0|1 )
  53: orb            | binary or ( a b -- c )
  54: over           | ( a b -- a b a )
  55: print          | print top value on hex format
  56: print#         | print top value on decimal format
  57: printb         | print top value on binary format
  58: readb          | read byte ( addr -- value )
  59: ret            | return
  60: show           | ( sym sym ... count -- )
  61: sub            | ( a b -- a-b )
  62: swap           | ( a b -- b a )
  63: wordsize       | ( -- N )
  64: writeb         | write byte ( addr -- value )

```

