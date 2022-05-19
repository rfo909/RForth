#ifndef RF_STACKS_HH
#define RF_STACKS_HH

#include "Common.hh"

typedef struct {
  byte type;
  long val;
} DStackValue;

#define DS_TYPE_INT      0     // default: signed int
#define DS_TYPE_UINT     1
#define DS_TYPE_BYTE     2
#define DS_TYPE_LONG     3
#define DS_TYPE_ULONG    4

#define DS_LAST_NUMBER_TYPE   9
  // number types can be cast freely between each other

#define DS_TYPE_WORD    10
typedef struct {
  byte *code;
  int pc;
  long localVariables[LOCAL_VARIABLE_COUNT];
} CStackFrame;



bool dsEmpty();
int dsCount();

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
CStackFrame *csPeek ();
void csPop();
int csCount();

void stacksReset();



#endif
