#ifndef RF_OPCODES_HH
#define RF_OPCODES_HH

#include "Common.hh"

// Need library functions:
// : true 1 :boolean ;
// : false 0 :boolean ;

#define OP_EOF    0
#define OP_RET    1
#define OP_CALL    2
#define OP_JMP    3
#define OP_ZJMP    4
#define OP_CJMP    5
#define OP_POP    6
#define OP_DUP    7
#define OP_READ    8
#define OP_WRITE    9
#define OP_LSET    10
#define OP_LGET    11
#define OP_ADD    12
#define OP_SUB    13
#define OP_MUL    14
#define OP_DIV    15
#define OP_MOD    16
#define OP_NEG    17
#define OP_GT    18
#define OP_LT    19
#define OP_GE    20
#define OP_LE    21
#define OP_EQ    22
#define OP_NE    23
#define OP_L_AND    24
#define OP_L_OR    25
#define OP_L_NOT    26
#define OP_LSHIFT    27
#define OP_RSHIFT    28
#define OP_B_AND    29
#define OP_B_OR    30
#define OP_B_NOT    31

#define OP_LSET0      32
#define OP_LSET1      33
#define OP_LSET2      34
#define OP_LSET3      35
#define OP_LGET0      36
#define OP_LGET1      37
#define OP_LGET2      38
#define OP_LGET3      39

#define OP_AS_BYTE    40
#define OP_AS_INT     41
#define OP_AS_UINT    42
#define OP_AS_LONG    43
#define OP_AS_ULONG   44

#define OP_MILLIS     45
#define OP_EE_READ    46
#define OP_EE_WRITE   47
#define OP_EE_LENGTH  48

#define OP_NULL       49
#define OP_NOP        50

#define OP_AS_SYM     51
#define OP_AS_ADDR    52

#define OP_ABORT      53
#define OP_AS_BOOL    54

#define OP_ADDR       55

#define OP_SYMBOL     99


int lookupSymbol (char *sym);
void printOpName (int opCode);

#endif
