# include "Stacks.hh"

static int dataStack[DATA_STACK_SIZE];
static int dsNext=0;

void dsPush (int val) {
  LOG("dsPush val=",val);
  if (dsNext >= DATA_STACK_SIZE) {
    err("dsPush",dsNext);
  } else {
    dataStack[dsNext++]=val;
  }
}

bool dsEmpty() {
  return (dsNext==0);
}

int dsPeek () {
  LOG("dsPeek dsNext=",dsNext);
  if (dsNext <= 0) {
    err("dsPeek",dsNext);
  }
  return dataStack[dsNext-1];
}

int dsPop () {
  LOG("dsPop dsNext=",dsNext);
  dsNext--;
  if (dsNext < 0) {
    dsNext=0;
    err("dsPop",dsNext);
  } else {
    return dataStack[dsNext];
  }
}

int dsCount() {
  LOG("dsCount",dsNext);
  return dsNext;
}

int dsGet (int pos) {
  LOG("dsGet",pos);
  return dataStack[pos];
}

// ------------------

static CStackFrame callStack[CALL_STACK_SIZE];
static int csNext=0;

void csPush (byte *theCode) {
  LOG("csPush",csNext);
  if (csNext >= CALL_STACK_SIZE) {
    err("csPush",csNext);
  } else {
    LOG("csPush.init StackFrame",csNext);
    CStackFrame *f=callStack+csNext;
    csNext++;
    f->code=theCode;
    f->pc=0;
    for (int i=0; i<LOCAL_VARIABLE_COUNT; i++) {
      f->localVariables[i]=0;
    }
  }
}

bool csEmpty () {
  return (csNext==0);
}

CStackFrame *csPeek () {
  LOG("csPeek",csNext);
  if (csEmpty()) err("csPeek",csNext);
  return callStack+(csNext-1);
}

void csPop() {
  LOG("csPop",csNext);
  if (csEmpty()) err("csPop", csNext);
  csNext--;
  if (csNext < 0) csNext=0;
}

int csCount() {
  LOG("csCount",csNext);
  return csNext;
}

// ---------

void stacksReset() {
  LOG("stacksReset",0);
  dsNext=0;
  csNext=0;
}
