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
  
  if (dsNext >= DATA_STACK_SIZE) {
    Serial.println(F("dsPushValue, stack overflow"));
    setAbortCodeExecution();
  } else {
    DStackValue *x=dataStack + dsNext;
    dsNext++;
    x->type=type;
    x->val=val;
  }

  if (dsNext > dsMaxStackSize) dsMaxStackSize=dsNext;
}

void dsPushValueCopy (DStackValue *value) {
  dsPushValue(value->type, value->val);
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


// This function has a two-fold purpose. It is both a check and a type cast.
// These two are normally one, except for the case of the null value, which is
// a valid value for all types, but can not be cast into anything else.
// 
// This means a word can return null, and have it pass through some
//
//     ( .... -- :byte )
//
// This states the result to be byte, but a null must pass through. 
//
// Returns true if type check ok. For most cases this will also mean
// modifying the type of the value (on top of stack)

bool dsTypeCast (byte newType) {
  DStackValue *x=dsPeekValue();
  if (x->type == newType) {
    return true;
  }

  // a null is a valid value of all types, but can not be type cast to anything else, so
  // we don't change the type of the value, but return true to affirm the type check
  if (x->type==DS_TYPE_NULL) {
    return true; 
  }

  bool ok=false;
  
  if (x->type & DS_TYPE_NUMBER_MASK && newType & DS_TYPE_NUMBER_MASK) {
    ok=true;
  }

  if (x->type & DS_TYPE_NUMBER_MASK && (newType==DS_TYPE_BYTE || newType==DS_TYPE_CHAR || newType==DS_TYPE_BOOL)) {
    ok=true;
  }

  if (x->type & DS_TYPE_NUMBER_MASK && newType==DS_TYPE_BOOL) {
    ok=true;
  }

  if (ok) {
    // perform type cast, which means stripping bits if moving down
    if (newType==DS_TYPE_LONG) {
     x->val=(long) x->val;
    } else if (newType==DS_TYPE_ULONG) {
     x->val=(unsigned long) x->val;
    } else if (newType==DS_TYPE_INT) {
      x->val=(int) x->val;
    } else if (newType==DS_TYPE_UINT) {
      x->val=(unsigned int) x->val;
    } else if (newType==DS_TYPE_BYTE) {
      x->val=(byte) x->val;
    } else if (newType==DS_TYPE_CHAR) {
      x->val=(char) x->val;
    } else if (newType==DS_TYPE_BOOL) {
      x->val=(x->val == 0 ? 0 : 1);
    }
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
  if (csNext >= CALL_STACK_SIZE) {
    Serial.println(F("csPush: stack overflow"));
    setAbortCodeExecution();
  } else {
    CStackFrame *f=callStack+csNext;
    csNext++;
    f->code=theCode;
    f->pc=0;
    for (register int i=0; i<LOCAL_VARIABLE_COUNT; i++) {
      DStackValue *ptr=f->localVars;
      ptr=ptr+i;
      ptr->type=DS_TYPE_LONG;
      ptr->val=0L;
    }
  }
  if (csNext > csMaxStackSize) csMaxStackSize=csNext;
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

void stacksReset () {
  // Cleaning up unwanted content on the call stack, which
  // may follow if there is a sys:abort or internal errors.
 
  csNext=0;

  // The data stack is monitored in manual mode, and in non-manual mode,
  // there will be some infinite loop, and this function will
  // not be invoked.

}
