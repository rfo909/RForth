#ifndef RF_STACKS_HH
#define RF_STACKS_HH

#include "Common.hh"

typedef struct {
  byte type;
  long val;
} DStackValue;

#define DS_TYPE_INT      0x01
#define DS_TYPE_UINT     0x02
#define DS_TYPE_BYTE     0x04
#define DS_TYPE_LONG     0x08
#define DS_TYPE_ULONG    0x10
#define DS_TYPE_WORD     0x20
#define DS_TYPE_STRUCT   0x40
#define DS_TYPE_LIST     0x80

#define DS_TYPE_NUMBER_MASK   0x1F

#define DS_TYPE_NULL     0xFF



typedef struct {
  byte *code;
  int pc;
  long localVariables[LOCAL_VARIABLE_COUNT];
} CStackFrame;



bool dsEmpty();
int dsCount();
int getDsMaxStackSize();

void dsPushValue (byte type, long val);
void dsPush (long val);

DStackValue *dsPeekValue();
long dsPeek ();

DStackValue *dsPopValue ();
long dsPop ();

DStackValue *dsGetValue (int pos); 
long dsGet (int pos);

void dsDupValue();
bool dsTypeCast (byte newType);



void csPush (byte *code);
bool csEmpty ();
int getCsMaxStackSize();
CStackFrame *csPeek ();
void csPop();
int csCount();

void stacksReset();



#endif
