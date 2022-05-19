# include "Stacks.hh"

static DStackValue dataStack[DATA_STACK_SIZE];
static int dsNext=0;


bool dsEmpty() {
  return (dsNext==0);
}

int dsCount() {
  return dsNext;
}


void dsPushValue (byte type, int val) {
  if (dsNext >= DATA_STACK_SIZE) {
    err("dsPushValue",dsNext);
  } else {
    DStackValue *x=dataStack + dsNext;
    dsNext++;
    x->type=type;
    x->val=val;
  }
}

void dsPush (int val) {
  dsPushValue(DS_TYPE_NUM, val);
}



DStackValue *dsPeekValue() {
  if (dsNext <= 0) {
    err("dsPeekValue",dsNext);
  }
  return dataStack + (dsNext-1);;
}

int dsPeek () {
  DStackValue *x = dsPeekValue();
  if (x->type != DS_TYPE_NUM) {
    err("dsPeek: not number", x->type);
  }
  return x->val;
}



DStackValue *dsPopValue() {
  dsNext--;
  if (dsNext < 0) {
    dsNext=0;
    err("dsPopValue",dsNext);
  } 
  return dataStack + dsNext;
}

int dsPop () {
  DStackValue *x = dsPopValue();
  if (x->type != DS_TYPE_NUM) {
    err("dsPop: not number", x->type);
  }
  return x->val; 
}


DStackValue *dsGetValue (int pos) {
  return dataStack+pos;
}

int dsGet (int pos) {
  DStackValue *x=dsGetValue(pos);
  if (x->type != DS_TYPE_NUM) {
    err("dsGet: not number", pos);
  }
  return x->val;
}

void dsDupValue() {
  DStackValue *x=dsPeekValue();
  dsPushValue(x->type, x->val);
}

// ------------------

static CStackFrame callStack[CALL_STACK_SIZE];
static int csNext=0;

void csPush (byte *theCode) {
  if (csNext >= CALL_STACK_SIZE) {
    err("csPush",csNext);
  } else {
    CStackFrame *f=callStack+csNext;
    csNext++;
    f->code=theCode;
    f->pc=0;
    for (int i=0; i<LOCAL_VARIABLE_COUNT; i++) {
      int *ptr=f->localVariables;
      *(ptr+i)=0;
    }
  }
}

bool csEmpty () {
  return (csNext==0);
}

CStackFrame *csPeek () {
  if (csEmpty()) err("csPeek",csNext);
  return callStack+(csNext-1);
}

void csPop() {
  if (csEmpty()) err("csPop", csNext);
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
