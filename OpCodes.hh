#ifndef RF_OPCODES_HH
#define RF_OPCODES_HH

#include "Common.hh"

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


char *getOp (byte b);

//bool executeOp (byte b);

int lookupSymbol (char *sym);

#endif
