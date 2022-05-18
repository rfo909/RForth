#include "Common.hh"
#include "Input.hh"
#include "Stacks.hh"
#include "Storage.hh"
#include "OpCodes.hh"

/**
 * A simple forth compiler. Syntax:
 * 
 * : name 3 3 + ;     # define function 'name'
 * 
 * expr =a            # set local variable
 * 
 * bool if{ ... }
 * loop{ .... bool break }
 * 
 * Numbers on formats:
 *  13
 *  b0000_1101
 *  0x0D
 * 
 * Stack values are signed 16 bit int's
 * 
 * Interactive commands:
 * 
 * clear  - clear stack
 * hex - show top value as hex
 * bin - show top value as binary
 */




int getHeapSize() {
  int maxSize=2000;
  for(int size=maxSize; size>0; size-=10) {
    byte *ptr=malloc(size);
    if (ptr != NULL) {
      free(ptr);
      Serial.print(F("Heap size: "));
      Serial.println(size);
      return size;
    }
  }
  return 0;
}


void err (char *s, int i) {
  Serial.print(F("ERROR: "));
  Serial.print(s);
  Serial.print(" ");
  Serial.println(i);
  for(;;) ;
}

void log (char *s, int i) {
  Serial.print(F("**** LOG: "));
  Serial.print(s);
  Serial.print(" ");
  Serial.println(i);
}

static void reset() {
  inpReset();
  //stacksReset();
  
  Serial.println(F("Ok."));
}

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for connect
  Serial.setTimeout(10);

  Serial.println(F("-----------------"));
  Serial.print(F("RForth "));
  Serial.print(VERSION);
  Serial.println(F("-----------------"));
  getHeapSize();

  reset();
}



void parseTokens();

bool inComment=false;

void loop() {
  // put your main code here, to run repeatedly:
  char c=Serial.read();
  if (c==-1) return;
  if (c=='#') {
    inComment=true;
    return;
  }
  if (c==' ' || c=='\r' || c=='\n' || c=='\t') {
    if (c=='\n' || c=='\r') {
      inComment=false;
    }
    if (!inpEmpty()) {
      char *str=inpChop();
      inpAddToken(str);
      if (!strcmp(str,";")) {
        parseTokens();
        reset();
      }
    }
  } else {
    if (!inComment) inpAddChar(c);
  }
}


// ---------------------------------------------
// Disassembler
// ---------------------------------------------

// code MUST end with OP_EOF
void disassemble (byte *code) {
  
#ifdef ENABLE_DISASSEMBLER

  for (int pos=0; pos<128; pos++) {
    Serial.print("  ");
    Serial.print(pos);
    Serial.print("  ");
    byte b=code[pos];
    Serial.print(b);
    Serial.print("  ");
    if (b & 0x80) {
      // PUSH
      Serial.print("PUSH");
      Serial.print(" ");
      Serial.println(b & 0x7F);
    } else {
      char *op=getOp(b);
      if (op!=NULL) {
        Serial.println(op);
      } else {
        Serial.println(b);
      }
    }
    if (b==OP_EOF) break;
  }

#endif

}


// ---------------------------------------------
// parseTokens()
// ---------------------------------------------


void parseTokens() {
  if (inpTokenMatches(":")) {
    int mark=pcGetMark();
    bool ok=parseWordDef();
    if (!ok) {
      // remove compiled code
      Serial.print(F("Word function parse failed. Resetting pc to mark "));
      Serial.println(mark);
      pcResetToMark(mark);
    }
  } else {
    int mark=pcGetMark();
    parseImmediateCode();
    pcResetToMark(mark);
  }
}

void executeCode (byte *initialCode);

// Defining a word with ":" colon
bool parseWordDef() {
  if (inpTokenMatches(";")) {
    Serial.println(F("word def: name missing"));
    return false;
  }
  char *name=inpTokenGet();
  inpTokenAdvance();
  
  while (!inpTokenMatches(";")) {
    char *tok=inpTokenGet();
    if (!parseWord()) {
      return false;  
    }
  }
  pcAddByte(OP_EOF);

  int codePos=pcChopInt();
  byte *code=pcGetPointer(codePos);
  
  disassemble(code);
  
  Serial.print(F("Saving word "));
  Serial.print(name);
  Serial.print(F(" at position "));
  Serial.println(codePos);
  
  psAddStr(name);
  name=psChop();
  mapAddPos(name,codePos);
  return true;
}

bool parseImmediateCode() {
  while (!inpTokenMatches(";")) {
    if (inpTokenMatches("words")) {
      int cnt=mapCount();
      for (int i=0; i<cnt; i++) {
        Serial.print("   ");
        Serial.println(mapGetName(i));
      }
      continue;
    }
    if (inpTokenMatches("clear")) {
      while (!dsEmpty()) dsPop();
      continue;
    }
    if (inpTokenMatches("hex")) {
      if (dsEmpty()) {
        Serial.println(F("hex: no value on stack"));
        continue;
      }
      char buf[10];
      itoa(dsPeek(),buf,16);
      Serial.print(F("--> 0x"));
      Serial.println(buf);
      continue;
    }
    
    if (inpTokenMatches("bin")) {
      if (dsEmpty()) {
        Serial.println(F("bin: no value on stack"));
        continue;
      }
      char buf[20];
      itoa(dsPeek(),buf,2);
      Serial.print(F("--> b"));
      Serial.println(buf);
      continue;
    }
    
    if (!parseWord()) {
      return false;
    }
  }

  // compile ok
  pcAddByte(OP_EOF);
  
  int codePos=pcChopInt();
  //Serial.print(F("parseImmediateCode, codePos="));
  //Serial.println(codePos);
  byte *code=pcGetPointer(codePos);
  
  disassemble(code);

  executeCode(code);
  
  return true;
}

bool isDigit(char c) {
  return (c=='0' || c=='1' || c=='2' || c=='3' || c=='4' || c=='5' || c=='6' || c=='7' || c=='8' || c=='9');
}

bool parseIf();
bool parseLoop();

int parseHex (char *s) {
  int val=0;
  for (int i=0; i<strlen(s); i++) {
    char c=s[i];
    val=val*16;
    if (c=='a' || c=='A') val += 10;
    else if (c=='b' || c=='B') val += 11;
    else if (c=='c' || c=='C') val += 12;
    else if (c=='d' || c=='D') val += 13;
    else if (c=='e' || c=='E') val += 14;
    else if (c=='f' || c=='F') val += 15;
    else if (c=='1') val += 1;
    else if (c=='2') val += 2;
    else if (c=='3') val += 3;
    else if (c=='4') val += 4;
    else if (c=='5') val += 5;
    else if (c=='6') val += 6;
    else if (c=='7') val += 7;
    else if (c=='8') val += 8;
    else if (c=='9') val += 9;
  }
  return val;
}

bool parseWord() {

  if (inpTokenMatches("if{")) {
    return parseIf();
  } 
  if (inpTokenMatches("loop{")) {
    return parseLoop();
  }
  
  char *word=inpTokenGet();

  // built-in symbols mapping to opcodes
  int code=lookupSymbol(word);
  if (code >= 0) {
    pcAddByte(code);
    
    inpTokenAdvance();
    return true;
  }

  // local variable assign
  if (word[0]=='=') {
    char *varName=word+1;
    int pos=inpLocalVariablePos(varName);
    if (pos >= 0) {
      pcInt7bit(pos);
      pcAddByte(OP_LSET);
      
      inpTokenAdvance();
      return true;
    } else {
      if (inpLocalVariablesFull()) {
        Serial.print(F("Too many local variables, max "));
        Serial.print(LOCAL_VARIABLE_COUNT);
        Serial.println(F(" per word function."));
        return false;
      }
      // add new variable
      pos = inpLocalVariableAdd(varName);
      pcInt7bit(pos);
      pcAddByte(OP_LSET);
      
      inpTokenAdvance();
      return true;
    }
  }

  // local variable lookup
  int varPos=inpLocalVariablePos(word);
  if (varPos >= 0) {
    pcInt7bit(varPos);
    pcAddByte(OP_LGET);
    
    inpTokenAdvance();
    return true;
  }

  // enter hex number as 0x...
  if (word[0]=='0' && word[1]=='x') {
    int val=parseHex(word+2);
    pcInt(val);

    inpTokenAdvance();
    return true;
  }

  // enter binary number as b001001, plus support underscore _ for readability
  if (word[0]=='b' && (word[1]=='1' || word[1]=='0')) {
    int val=0;
    for (int i=1; i<strlen(word); i++) {
      if (word[i]=='_') continue;
      val = val << 1;
      if (word[i]=='1') val++;
    }
    pcInt(val);

    inpTokenAdvance();
    return true;
  }  

  // decimal numbers
  if (isDigit(word[0])) {
    pcInt(atoi(word));
    
    inpTokenAdvance();
    return true;
  }

  // call word function
  int codePos=mapLookupPos(word);
  if (codePos >= 0) {
    pcInt(codePos);
    pcAddByte(OP_CALL);

    inpTokenAdvance();
    return true;
  }

  Serial.print(F("Unrecognized word: "));
  Serial.println(word);

  return false;
  
}


bool parseIf() {
  int jmpToEndAddr=pcGetLocalPos();
  pcInt7bit(0);
  pcAddByte(OP_ZJMP);

  while (!inpTokenMatches("}")) {
    if (inpTokenMatches(";")) {
      Serial.println(F("if{...} contains ';'"));
      return false;
    }
    if (!parseWord()) return false;
  }

  int endLoc=pcGetLocalPos();
  // insert endLoc into the 
  setLocalPosByte(jmpToEndAddr,to7BitPush(endLoc));

  return true;
}


bool parseLoop() {
  Serial.println(F("parseLoop"));
  int startPos=pcGetLocalPos();
  int breakJmpAddr=-1;
  
  while (!inpTokenMatches("}")) {
    if (inpTokenMatches(";")) {
      Serial.println(F("loop{...} contains ';'"));
      return false;
    }
    if (inpTokenMatches("break")) {
      if (breakJmpAddr >= 0) {
        Serial.println(F("loop{...} can only have one break"));
        return false;
      }
      breakJmpAddr=pcGetLocalPos();
      pcInt7bit(0);
      pcAddByte(OP_CJMP);
      continue;
    }
    if (!parseWord()) return false;
  }

  // jump back to start
  pcInt7bit(startPos);
  pcAddByte(OP_JMP);

  // after loop
  if (breakJmpAddr >= 0) {
    int endLoc=pcGetLocalPos();
    setLocalPosByte(breakJmpAddr, to7BitPush(endLoc));
  }

  return true;
}


// -------------------------------------
// execute code
// -------------------------------------

bool executeOneOp();



void executeCode (byte *initialCode) {
  csPush(initialCode);

  int executeOpCount=0;
  unsigned long startTime=millis();
  while(!csEmpty()) {
    if (!executeOneOp()) {
      return;
    }
    executeOpCount++;
  }
  unsigned long endTime=millis();
  Serial.print(endTime-startTime);
  Serial.print(F(" ms #op="));
  Serial.println(executeOpCount);
  
  // Show data stack
  int count = dsCount();
  //Serial.print(F("Number of values on stack: "));
  //Serial.println(count);

  Serial.println("--");
  for (int i=count-1; i>=0; i--) {
    Serial.print("  ");
    Serial.println(dsGet(i));
  }
  
}

bool executeOneOp () {
  CStackFrame *curr=csPeek();
  byte *code=curr->code;

  int pos=curr->pc;
  curr->pc=curr->pc+1; // may also be modified by JMP-instructions
 
  byte b=code[pos];

  if (b & 0x80) {
    // PUSH
    //Serial.print("PUSH ");
    //Serial.println(b & 0x7F);
    dsPush(b & 0x7F);
    return true;
  }

  switch (b) {
    case OP_EOF:
    case OP_RET: {
      //Serial.println(F("EOF / RET: csPop()"));
      csPop();
      return true;
    }
    case OP_CALL: {
      if (dsEmpty()) {
        Serial.println(F("OP_CALL - data stack empty"));
        return false;  
      }
      int codePos=dsPop();
      byte *code=pcGetPointer(codePos);
      //Serial.print(F("OP_CALL csPush "));
      //Serial.println(codePos);
      csPush(code);
      return true;
    }

    case OP_JMP : {
      if (dsEmpty()) {
        Serial.println(F("OP_JMP - data stack empty"));
        return false;
      }
      int pos=dsPop();
      curr->pc=(byte) pos;
      
      return true;
    }
    case OP_ZJMP : {
      if (dsEmpty()) {
        Serial.println(F("OP_ZJMP - data stack empty"));
        return false;
      }
      int pos=dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_ZJMP - data stack empty"));
        return false;
      }
      int cond=dsPop();
      if (cond==0) curr->pc=(byte) pos;
      
      return true;
    }
    case OP_CJMP : {
      if (dsEmpty()) {
        Serial.println(F("OP_CJMP - data stack empty"));
        return false;
      }
      int pos=dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_CJMP - data stack empty"));
        return false;
      }
      int cond=dsPop();
      if (cond!=0) curr->pc=(byte) pos;
      
      return true;
    }
    case OP_POP : {
      if (dsEmpty()) {
        Serial.println(F("OP_POP - data stack empty"));
        return false;
      }
      (void) dsPop();
      return true;
    }
    case OP_DUP : {
      if (dsEmpty()) {
        Serial.println(F("OP_DUP - data stack empty"));
        return false;
      }
      int value=dsPop();
      dsPush(value);
      dsPush(value);
      
      return true;
    }
    case OP_READ : {
      if (dsEmpty()) {
        Serial.println(F("OP_READ - data stack empty"));
        return false;
      }
      int addr=dsPop();
      byte *ptr=(byte *) addr;
      dsPush(*ptr);
      return true;
    }
    case OP_WRITE : {
      if (dsEmpty()) {
        Serial.println(F("OP_WRITE - data stack empty"));
        return false;
      }
      int addr=dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_WRITE - data stack empty"));
        return false;
      }
      int value=dsPop();
      byte *ptr=(byte *) addr;
      *ptr = (byte) value;
      return true;
    }
    case OP_LSET : {
      if (dsEmpty()) {
        Serial.println(F("OP_LSET - data stack empty"));
        return false;
      }
      int varNo=dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_LSET - data stack empty"));
        return false;
      }
      int value=dsPop();
      
      curr->localVariables[varNo]=value;
      return true;
    }
    case OP_LGET : {
      if (dsEmpty()) {
        Serial.println(F("OP_LGET - data stack empty"));
        return false;
      }
      int varNo=dsPop();
      
      dsPush(curr->localVariables[varNo]);
      return true;
    }
    case OP_ADD : {
      if (dsEmpty()) {
        Serial.println(F("OP_ADD - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_ADD - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a+b);
      return true;
    }
    case OP_SUB : {
      if (dsEmpty()) {
        Serial.println(F("OP_SUB - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_SUB - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a-b);
      return true;
    }
    case OP_MUL : {
      if (dsEmpty()) {
        Serial.println(F("OP_MUL - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_MUL - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a*b);
      return true;
    }
    case OP_DIV : {
      if (dsEmpty()) {
        Serial.println(F("OP_DIV - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_DIV - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a/b);
      return true;
    }
    case OP_MOD : {
      if (dsEmpty()) {
        Serial.println(F("OP_MOD - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_MOD - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a % b);
      return true;
    }
    case OP_NEG : {
      if (dsEmpty()) {
        Serial.println(F("OP_NEG - data stack empty"));
        return false;
      }
      int value = dsPop();
      dsPush(-value);
      return true;
    }
    case OP_GT : {
      if (dsEmpty()) {
        Serial.println(F("OP_GT - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_GT - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a > b);
      return true;
    }
    case OP_LT : {
      if (dsEmpty()) {
        Serial.println(F("OP_LT - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_LT - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a < b);
      return true;
    }
    case OP_GE : {
      if (dsEmpty()) {
        Serial.println(F("OP_GE - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_GE - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a >= b);
      return true;
    }
    case OP_LE : {
      if (dsEmpty()) {
        Serial.println(F("OP_LE - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_LE - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a <= b);
      return true;
    }
    case OP_EQ : {
      if (dsEmpty()) {
        Serial.println(F("OP_EQ - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_EQ - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a == b);
      return true;
    }
    case OP_NE : {
      if (dsEmpty()) {
        Serial.println(F("OP_NE - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_NE - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a != b);
      return true;
    }
    case OP_L_AND : {
      if (dsEmpty()) {
        Serial.println(F("OP_L_AND - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_L_AND - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a && b);
      return true;
    }
    case OP_L_OR : {
      if (dsEmpty()) {
        Serial.println(F("OP_L_OR - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_L_OR - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a || b);
      return true;
    }
    case OP_L_NOT : {
      if (dsEmpty()) {
        Serial.println(F("OP_L_NOT - data stack empty"));
        return false;
      }
      int val = dsPop();
      dsPush(!val);
      return true;
    }
    case OP_LSHIFT : {
      if (dsEmpty()) {
        Serial.println(F("OP_LSHIFT - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_LSHIFT - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a << b);
      return true;
    }
    case OP_RSHIFT : {
      if (dsEmpty()) {
        Serial.println(F("OP_RSHIFT - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_RSHIFT - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a >> b);
      return true;
    }
    case OP_B_AND : {
      if (dsEmpty()) {
        Serial.println(F("OP_B_AND - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_B_AND - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a & b);
      return true;
    }
    case OP_B_OR : {
      if (dsEmpty()) {
        Serial.println(F("OP_B_OR - data stack empty"));
        return false;
      }
      int b = dsPop();
      if (dsEmpty()) {
        Serial.println(F("OP_B_OR - data stack empty"));
        return false;
      }
      int a = dsPop();
      dsPush(a | b);
      return true;
    }
    case OP_B_NOT : {
      if (dsEmpty()) {
        Serial.println(F("OP_B_NOT - data stack empty"));
        return false;
      }
      int val = dsPop();
      dsPush(~val);
      return true;
    };
  }   
  err("Unknown OP", b);
  return false;
}
