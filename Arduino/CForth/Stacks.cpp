#include "Constants.h"

static Word dStack[DSTACK_SIZE];
static Byte dStackNext;
static Word rStack[RSTACK_SIZE];
static Byte rStackNext;

void stacksInit(void) {
  dStackNext=0;
  rStackNext=0;
}

void push (Word v) {
  if (dStackNext >= DSTACK_SIZE-1) {
    setHasError();
    sPrint("overflow");
    sPrint(" ");
    sPrint("data");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return;
  }
  dStack[dStackNext++]=v;
}

Word pop () {
  if (dStackNext==0) {
    setHasError();
    sPrint("underflow");
    sPrint(" ");
    sPrint("data");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return 0;
  }
  return dStack[--dStackNext];
}

Word pick (Word n) {
  if (dStack-1-n < 0) {
    setHasError();
    sPrint("underflow");
    sPrint(" ");
    sPrint("data");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return 0;
  }
  return dStack[dStackNext-1-n];
}

void rpush (Word v) {
  /* Print("rpush ");
  sPrintWord(v);
  sPrintln(); */
  if (dStackNext >= RSTACK_SIZE-1) {
    setHasError();
    sPrint("overflow");
    sPrint(" ");
    sPrint("return");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return;
  }
  rStack[rStackNext++]=v;
}

Word rpop () {
  if (rStackNext==0) {
    setHasError();
    sPrint("underflow");
    sPrint(" ");
    sPrint("return");
    sPrint(" ");
    sPrint("stack");
    sPrintln();

    return 0;
  }
  rStackNext--;
  Word v = rStack[rStackNext];
  /* sPrint("rpop ");
  sPrintWord(v);
  sPrintln(); */
  return v;
}


void dStackShow() {
  sPrintln();
  sPrint("stack");
  sPrint(" ");
  sPrint(":");
  sPrint(" ");
  sPrint("[");
  for (Byte i=0; i<dStackNext; i++) {
    if (i>0) sPrint(" ");
    sPrintWord(dStack[i]);
  }
  sPrint("]");
  sPrintln();
}

void dStackClear() {
  dStackNext=0;
}
