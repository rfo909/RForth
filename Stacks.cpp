# include "Stacks.hh"

static DStackValue dataStack[DATA_STACK_SIZE];
static int dsNext=0;


bool dsEmpty() {
  return (dsNext==0);
}

int dsCount() {
  return dsNext;
}


void dsPushValue (byte type, long val) {
  if (dsNext >= DATA_STACK_SIZE) {
    ERR1(FUNC_dsPushValue,dsNext);
  } else {
    DStackValue *x=dataStack + dsNext;
    dsNext++;
    x->type=type;
    x->val=val;
  }
}

void dsPush (long val) {
  dsPushValue(DS_TYPE_LONG, val);
}



DStackValue *dsPeekValue() {
  if (dsNext <= 0) {
    ERR1(FUNC_dsPeekValue,dsNext);
  }
  return dataStack + (dsNext-1);;
}

long dsPeek () {
  DStackValue *x = dsPeekValue();
  if (x->type > DS_LAST_NUMBER_TYPE) {
    ERR1(ERR_dsPeek_not_number, x->type);
  }
  return x->val;
}



DStackValue *dsPopValue() {
  dsNext--;
  if (dsNext < 0) {
    dsNext=0;
    ERR1(FUNC_dsPopValue,dsNext);
  } 
  return dataStack + dsNext;
}

long dsPop () {
  DStackValue *x = dsPopValue();
  if (x->type > DS_LAST_NUMBER_TYPE) {
    ERR1(ERR_dsPop_not_number, x->type);
  }
  return x->val; 
}


DStackValue *dsGetValue (int pos) {
  return dataStack+pos;
}

long dsGet (int pos) {
  DStackValue *x=dsGetValue(pos);
  if (x->type != DS_TYPE_INT) {
    ERR1(ERR_dsGet_not_number, pos);
  }
  return x->val;
}

void dsDupValue() {
  DStackValue *x=dsPeekValue();
  dsPushValue(x->type, x->val);
}

// change type of top value on stack, return true if ok, 
// false if illegal
bool dsTypeCast (byte newType) {
  DStackValue *x=dsPeekValue();
  if (x->type <= DS_LAST_NUMBER_TYPE && newType <= DS_LAST_NUMBER_TYPE) {
    x->type=newType;
    return true;
  } else {
    ERR2(ERR_INVALID_TYPE_CAST,x->type,newType);
    halt();
  }
  return false;  
}
// ------------------

static CStackFrame callStack[CALL_STACK_SIZE];
static int csNext=0;

void csPush (byte *theCode) {
  if (csNext >= CALL_STACK_SIZE) {
    ERR1(FUNC_csPush,csNext);
  } else {
    CStackFrame *f=callStack+csNext;
    csNext++;
    f->code=theCode;
    f->pc=0;
    for (int i=0; i<LOCAL_VARIABLE_COUNT; i++) {
      long *ptr=f->localVariables;
      *(ptr+i)=0;
    }
  }
}

bool csEmpty () {
  return (csNext==0);
}

CStackFrame *csPeek () {
  if (csEmpty()) ERR1(FUNC_csPeek,csNext);
  return callStack+(csNext-1);
}

void csPop() {
  if (csEmpty()) ERR1(FUNC_csPop, csNext);
  csNext--;
  if (csNext < 0) csNext=0;
}

int csCount() {
  return csNext;
}

// ---------

void stacksReset() {
  dsNext=0;
  csNext=0;
}
