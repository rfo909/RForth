#ifndef RF_STACKS_HH
#define RF_STACKS_HH

#include "Common.hh"

typedef struct {
  byte type;
  long val;
} DStackValue;

#define DS_TYPE_NUM      0     // default: signed int
#define DS_TYPE_UINT     1
#define DS_TYPE_BYTE     2
#define DS_TYPE_LONG     3
#define DS_TYPE_ULONG    4

typedef struct {
  byte *code;
  int pc;
  int localVariables[LOCAL_VARIABLE_COUNT];
} CStackFrame;



bool dsEmpty();
int dsCount();

void dsPushValue (byte type, int val);
void dsPush (int val);

DStackValue *dsPeekValue();
int dsPeek ();

DStackValue *dsPopValue ();
int dsPop ();

DStackValue *dsGetValue (int pos); 
int dsGet (int pos);

void dsDupValue();



void csPush (byte *code);
bool csEmpty ();
CStackFrame *csPeek ();
void csPop();
int csCount();

void stacksReset();



#endif
