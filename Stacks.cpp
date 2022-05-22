# include "Stacks.hh"

static DStackValue dataStack[DATA_STACK_SIZE];
static int dsNext=0;
static int dsMaxStackSize=0;


bool dsEmpty() {
  return (dsNext==0);
}

int dsCount() {
  return dsNext;
}

int getDsMaxStackSize() {
  return dsMaxStackSize;
}

void dsPushValue (byte type, long val) {
  if (dsNext > dsMaxStackSize) dsMaxStackSize=dsNext;
  
  if (dsNext >= DATA_STACK_SIZE) {
    Serial.println(F("dsPushValue, stack overflow"));
    setAbortCodeExecution();
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
    Serial.println(F("dsPeekValue: stack empty"));
    setAbortCodeExecution();
  }
  return dataStack + (dsNext-1);;
}

long dsPeek () {
  DStackValue *x = dsPeekValue();
  if (!(x->type & DS_TYPE_NUMBER_MASK) && !(x->type == DS_TYPE_NULL)) {
    Serial.println(F("dsPeek: not a number"));
    setAbortCodeExecution();
    return 0L;
  }
  return x->val;
}



DStackValue *dsPopValue() {
  dsNext--;
  if (dsNext < 0) {
    dsNext=0;
    Serial.println(F("dsPop: stack empty"));
    setAbortCodeExecution();
    return dataStack;
  } 
  return dataStack + dsNext;
}

long dsPop () {
  DStackValue *x = dsPopValue();
  if (!(x->type & DS_TYPE_NUMBER_MASK)) {
    Serial.println(F("dsPop: not a number"));
    setAbortCodeExecution();
    return 0L;
  }
  return x->val; 
}


DStackValue *dsGetValue (int pos) {
  return dataStack+pos;
}

long dsGet (int pos) {
  DStackValue *x=dsGetValue(pos);
  if (x->type != DS_TYPE_INT) {
    Serial.println(F("dsGet: not a number"));
    setAbortCodeExecution();
    return 0L;
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
  if (x->type == newType) {
    return true;
  }

  // a null is a valid value of all types, but can not be type cast to anything else
  if (x->type==DS_TYPE_NULL) {
    return false; 
  }
  
  if (x->type & DS_TYPE_NUMBER_MASK && newType & DS_TYPE_NUMBER_MASK) {
    x->type=newType;
    return true;
  }

  if (x->type & DS_TYPE_NUMBER_MASK && (newType==DS_TYPE_BYTE || newType==DS_TYPE_CHAR || newType==DS_TYPE_BOOL)) {
    x->type=newType;
    return true;
  }
  if (x->type == DS_TYPE_BYTE && newType==DS_TYPE_CHAR) {
    x->type=newType;
    return true;
  }
  if (x->type == DS_TYPE_CHAR && newType==DS_TYPE_BYTE) {
    x->type=newType;
    return true;
  }
  if ((x->type & DS_TYPE_NUMBER_MASK || x->type==DS_TYPE_BYTE) && newType==DS_TYPE_BOOL) {
    x->type=newType;
    return true;
  }

  Serial.print(F("Invalid type cast "));
  Serial.print(x->type);
  Serial.print(F(" to "));
  Serial.println(newType);

  return false;  
}

// ------------------

static CStackFrame callStack[CALL_STACK_SIZE];
static int csNext=0;
static int csMaxStackSize=0;

void csPush (byte *theCode) {
  if (csNext > csMaxStackSize) csMaxStackSize=csNext;
  if (csNext >= CALL_STACK_SIZE) {
    Serial.println(F("csPush: stack overflow"));
    setAbortCodeExecution();
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

int getCsMaxStackSize() {
  return csMaxStackSize;
}


CStackFrame *csPeek () {
  if (csEmpty()) {
    Serial.println(F("csPeek: stack empty"));
    setAbortCodeExecution();
    return callStack;
  }
  return callStack+(csNext-1);
}

void csPop() {
  if (csEmpty()) {
    Serial.println(F("csPop: stack empty"));
    setAbortCodeExecution();
    return;
  }
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
