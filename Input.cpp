#include "Input.hh"

static char inputBuf[INPUT_BUF_SIZE];
static int inpStart=0;
static int inpNext=0;

static char *tokens[INPUT_TOKEN_COUNT];
static int nextToken=0;

static int tokenParsePos=0;

static char *localVariables[LOCAL_VARIABLE_COUNT];
static int nextLocalVariable=0;


void inpReset() {
  inpStart=0;
  inpNext=0;

  nextToken=0;
  tokenParsePos=0;
  nextLocalVariable=0;
}

void inpAddChar (char c) {
  if (inpNext >= INPUT_BUF_SIZE) {
    ERR1(FUNC_inpAddChar,inpNext);
  } else {
    inputBuf[inpNext++]=c;
  }
}

bool inpEmpty() {
  return (inpNext==inpStart);
}

char *inpChop () {
  inpAddChar('\0');
  char *s=inputBuf+inpStart;
  inpStart=inpNext;
  return s;
}


void inpAddToken (char *token) {
  if (nextToken >= INPUT_TOKEN_COUNT) {
    ERR1(FUNC_inpAddToken, nextToken);
  } else {
    tokens[nextToken++]=token;
  }
  //Serial.print(F("Adding token "));
  //Serial.println(token);
}


bool inpTokenMatches (char *s) {
  if (!strcmp(tokens[tokenParsePos],s)) {
    tokenParsePos++;
    return true;
  } else {
    return false;
  }
}

void inpTokenAdvance () {
  tokenParsePos++;
}

char *inpTokenGet() {
  return tokens[tokenParsePos];
}


int inpLocalVariablePos (char *name) {
  for (int i=0; i<nextLocalVariable; i++) {
    if (!strcmp(localVariables[i], name)) return i;
  }
  return -1;
}

bool inpLocalVariablesFull() {
  return (nextLocalVariable >= LOCAL_VARIABLE_COUNT);
}

int inpLocalVariableAdd (char *name) {
  if (nextLocalVariable >= LOCAL_VARIABLE_COUNT) ERR1(FUNC_inpLocalVariableAdd,nextLocalVariable);
  localVariables[nextLocalVariable++]=name;
  return nextLocalVariable-1;
}
