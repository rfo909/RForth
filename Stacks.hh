#ifndef RF_STACKS_HH
#define RF_STACKS_HH

#include "Common.hh"

typedef struct {
  byte type;
  long val;
} DStackValue;

#define DS_TYPE_ULONG    0x01
#define DS_TYPE_LONG     0x02
#define DS_TYPE_INT      0x03
#define DS_TYPE_UINT     0x04
#define DS_TYPE_BYTE     0x05
#define DS_TYPE_CHAR     0x06

#define DS_TYPE_BOOL     0x10
#define DS_TYPE_SYM      0x20   // literal word 
#define DS_TYPE_ADDR     0x30   // uses some bits for location and the rest for offset
#define DS_TYPE_COMPLEX  0x40   // points to symbol (literal word)

#define DS_TYPE_NULL     0xFF


#define DS_TYPE_NUMBER_MASK   0x0F




typedef struct {
  byte *code;
  int pc;
  DStackValue localVars[LOCAL_VARIABLE_COUNT];
} CStackFrame;



bool dsEmpty();
int dsCount();
int getDsMaxStackSize();

void dsPushValue (byte type, long val);
void dsPushValueCopy (DStackValue *value);
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
