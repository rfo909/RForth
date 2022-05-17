#ifndef RF_STACKS_HH
#define RF_STACKS_HH

#include "Common.hh"

typedef struct {
  byte *code;
  int pc;
  int localVariables[LOCAL_VARIABLE_COUNT];
} CStackFrame;



void dsPush (int val);
bool dsEmpty();
int dsPeek ();
int dsPop ();
int dsCount();
int dsGet (int pos); 

void csPush (byte *code);
bool csEmpty ();
CStackFrame *csPeek ();
void csPop();
int csCount();

void stacksReset();



#endif
