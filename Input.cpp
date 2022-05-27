#include "Input.hh"

static char inputBuf[INPUT_BUF_SIZE];
static int inpStart=0;
static int inpNext=0;

static char *tokens[INPUT_TOKEN_COUNT];
static int nextToken=0;

static int tokenParsePos=0;

static char *localVariables[LOCAL_VARIABLE_COUNT];
static int nextLocalVariable=0;

// Should rename from inpXxx to tmpXxx as that is what these data
// are, containing tokens and other temporary string data during
// execution

void inpReset() {
  inpStart=0;
  inpNext=0;

  nextToken=0;
  tokenParsePos=0;
  nextLocalVariable=0;
}

int inpSetMark () {
  return inpStart;
}

void inpResetToMark (int mark) {
 inpStart=mark;
 inpNext=mark;
}


void inpAddChar (char c) {
  if (inpNext >= INPUT_BUF_SIZE) {
    Serial.println(F("inpAddChar: buffer full"));
    setAbortCodeExecution();
  } else {
    inputBuf[inpNext++]=c;
  }
}

void inpUngetChar () {
  if (inpNext > inpStart) inpNext--;
}

bool inpEmpty() {
  return (inpNext==inpStart);
}

int inpGetStartPos() {
  return inpStart;
}

char *inpChop () {
  inpAddChar('\0');
  char *s=inputBuf+inpStart;
  inpStart=inpNext;
  return s;
}


void inpAddToken (char *token) {
  if (nextToken >= INPUT_TOKEN_COUNT) {
    Serial.println(F("inpAddToken: out of space"));
    setAbortCodeExecution();
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

int inpTokenCount() {
  return nextToken;
}

char *inpTokenAtPos (int pos) {
  return tokens[pos];
}

int inpTokenStreamPos() {
  return tokenParsePos;
}

void inpTokenStreamSetPos (int pos) {
  tokenParsePos=pos;
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
  if (nextLocalVariable >= LOCAL_VARIABLE_COUNT) {
    Serial.println(F("inpLocalVariableAdd: out of space"));
    setAbortCodeExecution();
    return 0;
  }
  localVariables[nextLocalVariable++]=name;
  return nextLocalVariable-1;
}
