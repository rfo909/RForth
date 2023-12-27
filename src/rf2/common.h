typedef __UINT16_TYPE__    Ref;
typedef __UINT_FAST8_TYPE__ Byte;
typedef __INT32_TYPE__ Long;

#define HEAP_SIZE       8192

// Heap locations for system variables
#define H_MAIN_SETUP                1   // Ref
#define H_MAIN_LOOP                 3   // Ref
#define H_CS_BASE_REF               5   // Ref
#define H_CS_NEXT_FRAME             7   // Byte
#define H_SYM_TOP_REF               8   // Ref

#define H_TS_BASE_REF               10  // Ref
#define H_TS_NEXT_FRAME             12  // Byte
#define H_DICT_TOP                  13  // Ref
#define H_HERE                      15  // Ref
#define H_HERE_SHADOW               17  // Ref



#define null                    0

// Forth true and false
#define F_TRUE                  -1
#define F_FALSE                 0


// data stack size
#define DSTACK_DEPTH            30

// CSF offsets

#define CSF_code            0   // Ref
#define CSF_tempStackBase   2   // byte
#define CSF_tempStackNext   3   // byte
#define CSF_pc              4   // Ref

#define CSF_n               6



#define OP_JMP                 0
#define OP_COND_JMP            1
#define OP_CALL                2
#define OP_RETURN              3
#define OP_PANIC               4
#define OP_HEAP_MAX            5
#define OP_TIBs                6
#define OP_TIBr                7
#define OP_TIBr_ADD            8
#define OP_TIBs_ADD            9
#define OP_TIBr_BACK           10
#define OP_TIBs_FWD            11
#define OP_TIBW                12
#define OP_EMIT                13
#define OP_EMITWORD            14
#define OP_ADD                 15
#define OP_SUBTRACT            16
#define OP_MUL                 17
#define OP_DIV                 18
#define OP_MOD                 19
#define OP_RIGHTSHIFT          20
#define OP_LEFTSHIFT           21
#define OP_AND                 22
#define OP_OR                  23
#define OP_NOT                 24
#define OP_EQ                  25
#define OP_NE                  26
#define OP_GT                  27
#define OP_LT                  28
#define OP_GE                  29
#define OP_LE                  30
#define OP_DROP                31
#define OP_SETLOCAL            32
#define OP_GETLOCAL            33
#define OP_WORD                34
#define OP_WRITE1              35
#define OP_WRITE2              36
#define OP_WRITE4              37
#define OP_READ1               38
#define OP_READ2               39
#define OP_READ4               40
#define OP_READ1g              41
#define OP_WRITE1g             42
#define OP_LITERAL1            43
#define OP_LITERAL2            44
#define OP_LITERAL4            45
#define OP_CHECK_PARSE         46
#define OP_PARSE               47
