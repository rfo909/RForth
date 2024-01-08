#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "stdlib.h"
#include <string.h> 

#include "pico/types.h"
#include "pico/stdlib.h"


#define WELCOME_STRING      "\nWelcome to RFOrth\n"


typedef uint16_t            Ref;
typedef uint8_t         Byte;
typedef int32_t        Long;

#define HEAP_SIZE       16384

// Heap locations for system variables

#define H_MAIN_SETUP                1   // Ref
#define H_MAIN_LOOP                 3   // Ref
#define H_CS_BASE_REF               5   // Ref
#define H_CS_NEXT_FRAME             7   // Byte
#define H_SYM_TOP_REF               8   // Ref

#define H_TS_BASE_REF               10  // Ref
#define H_COMPILE_BUF_POS           12  // Byte
#define H_DICT_TOP                  13  // Ref
#define H_HERE                      15  // Ref
#define H_HERE_SHADOW               17  // Ref

// (STATIC_DATA_BEGIN is here in the assembled code)

#define H_CS_MAX_DEPTH              19  // Byte
#define H_TS_MAX_DEPTH              20  // Byte

#define H_HEAP_STATIC_DATA_BEGIN    21  // Ref
#define H_HEAP_STATIC_DATA_END      23  // Ref




#define null                    0

// Forth true and false
#define F_TRUE                  -1
#define F_FALSE                 0


// data stack size
#define DSTACK_DEPTH            30




// JMP to absolute address in HEAP (Ref)
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

// JMP to relative position in compiled code (byte)
#define OP_JMP1                48
#define OP_COND_JMP1           49


// Main.c

void PANIC (char *msg);
bool hasPanicFlag();
void clearPanicFlag ();
void forthMainLoop();

// TIB.c

void initTIB();

char getTIBsChar();
char getTIBrChar();
void advanceTIBr();
void advanceTIBs();
void resetTIBr();
void confirmTIBr();
int getTIBWordLength();


// Sym.c

Ref symCreateTIBWord();
Ref addSymbol (char *str);

// DataStack.c

void dsInit();

void dsPushRef (Ref value);
void dsPushByte (Byte value);
void dsPushValue (Long value);

Ref dsPopRef ();
Byte dsPopByte ();
Long dsPopValue ();

bool dsEmpty ();
Long dsPeek (int offset); 


// Heap.c
void initHeap();

Byte *refToPointer (Ref ref);
char *safeGetString (Ref ref);

Ref heapMalloc (int length);

Byte readByte(Ref addr);
Ref readRef(Ref addr);

Byte readInt1(Ref addr);
Long readInt2(Ref addr);
Long readInt4(Ref addr);

void writeByte (Ref addr, Byte value);
void writeRef (Ref addr, Ref value);

void writeInt1 (Ref addr, Long value);
void writeInt2 (Ref addr, Long value);
void writeInt4 (Ref addr, Long value);

Byte readGlobal (Long addr);
void writeGlobal (Long addr, Byte value);

// Serial.c

void initSerial();

char serialNextChar();

void serialEmitChar (char c);
void serialEmitStr (char *str);
void serialEmitNewline ();
int serialLostChars();

// Debug.c

void DEBUGEnable (bool enable);
void DEBUG (char *msg);
void DEBUGint (char *name, int value);
void DEBUGstr (char *name, char *value);
void DUMP_DEBUG ();

// CSTS.c  -- call stack and temp stack

void csInit();
bool csEmpty();

Ref csGetCurrFrame ();
Byte csNextCodeByte ();

void csJumpToRef (Ref addr);
void csJumpToPC (Byte pc);

void csCall (Ref addr);
void csReturn ();

void csSetLocal (Ref sym, Long value);
Long csGetLocal (Ref sym);

void csShowOp ();

// Numbers.c

bool checkParseInt (char *s);
Long parseInt (char *s);