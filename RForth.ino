#include "EEPROM.h"
#include <SPI.h>
#include <Wire.h>

#include "Common.hh"
#include "Input.hh"
#include "Stacks.hh"
#include "Storage.hh"
#include "OpCodes.hh"


/**
   A simple forth compiler. Syntax:

   : name 3 3 + ;     # define function 'name'

   expr =a            # set local variable

   bool if{ ... }
   loop{ .... bool break }

   Numbers on formats:
    13
    b0000_1101
    0x0D

   Stack values are signed 16 bit int's

   Interactive commands:

   clear  - clear stack
   stats  - show memory use
   dis:xxx - disassemble word function xxx
   hex - show top value as hex
   bin - show top value as binary
*/



bool abortCodeExecution = false;

void setAbortCodeExecution () {
  abortCodeExecution = true;
}




static void reset() {

  inpReset();
  stacksReset();
  if (abortCodeExecution)  {
    Serial.println(F("[Abort]"));
  }
  abortCodeExecution = false;

  Serial.println(F("Ok."));
}

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // wait for connect
  Serial.setTimeout(10);

  Serial.println(F("-----------------"));
  Serial.print(F("RForth "));
  Serial.println(VERSION);
  Serial.print(F("F_CPU="));
  Serial.println(F_CPU);
  Serial.println(F("-----------------"));

  reset();
}



void parseTokens();

static bool inComment = false;

void loop() {

  if (abortCodeExecution) {
    reset();
    return;
  }

  char c = Serial.read();
  if (c == -1) return;

  // echo input - disabled (better with local echo (minicom))
  //Serial.print(c);

  if (c == '\b') {
    inpUngetChar();
    return;
  }

  // if no current word, and char is '#' then it is considered a comment

  if (inpEmpty() && c == '#') {
    inComment = true;
    return;
  }
  if (inComment && (c == '\r' || c == '\n')) {
    inComment = false;
  }
  if (!inComment) {
    if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
      if (!inpEmpty()) {
        char *str = inpChop();
        inpAddToken(str);

        if (!strcmp(str, ";")) {
          parseTokens();
          reset();
        }
      }

      // If first token not colon, we are in interactive mode.
      // Serial Monitor sends newlines only, while minicom for some
      // reason sends CR only ...
      //
      // Since the parser requires a semicolon at the end of input,
      // we add one before calling parser.
      // --
      if ((c == '\r' || c == '\n') && inpTokenCount() > 0 && strcmp(inpTokenAtPos(0), ":")) { // no colon means interactive mode
        inpAddChar(';');
        inpAddToken(inpChop());
        parseTokens();
        reset();
      }
    } else {
      if (!inComment) inpAddChar(c);
    }
  }
}


// ---------------------------------------------
// Disassembler
// ---------------------------------------------

// code MUST end with OP_EOF
void disassemble (byte *code) {

  int pos = 0;
  for (;;) {
    Serial.print("  ");
    Serial.print(pos);
    Serial.print("  ");
    byte b = code[pos];
    Serial.print(b);
    Serial.print("  ");
    if (b & 0x80) {
      // PUSH
      Serial.print(F("PUSH "));
      Serial.println(b & 0x7F);
    } else {
      printOpName(b);
      Serial.println();
    }
    if (b == OP_EOF) break;


    pos++;
    if (pos > P_CODE_MAX_SIZE) return;
  }

}


// ---------------------------------------------
// parseTokens()
// ---------------------------------------------

bool parseWordDef();
void parseConst();
bool parseImmediateCode();

void parseTokens() {
  if (inpTokenMatches(":")) {
    int mark = pcGetMark();
    bool ok = parseWordDef();
    if (!ok) {
      // remove compiled code
      Serial.print(F("Word function parse failed. Resetting pc to mark "));
      Serial.println(mark);
      pcResetToMark(mark);
    }
  } else if (inpTokenMatches("const")) {
    parseConst();
  } else {
    int mark = pcGetMark();
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
  char *name = inpTokenGet();
  inpTokenAdvance();

  if (inpTokenMatches("(")) {
    bool dash = false;
    bool done = false;

    int pos[LOCAL_VARIABLE_COUNT]; // max number of parameters
    int count = 0;

    bool matchAssign = false;
    for (;;) {
      if (inpTokenMatches("--")) {
        dash = true;
        break;
      }
      if (inpTokenMatches(")")) {
        done = true;
        break;
      }
      char *tok = inpTokenGet();
      if (!matchAssign) {
        if (!tok[0] == ':') {
          Serial.println(F("word def: ( inputs -- outputs ) : input must be sequence of :type =var"));
          return false;
        } else {
          inpTokenAdvance();
        }
      } else {
        if (tok[0] != '=') {
          Serial.println(F("word def: ( inputs -- outputs ) : input must be sequence of :type =var"));
          return false;
        } else {
          pos[count++] = inpTokenStreamPos();
          inpTokenAdvance();
        }
      }
      matchAssign = !matchAssign;
    }

    int afterInputPos = inpTokenStreamPos();

    for (int i = count - 1; i >= 0; i--) {
      inpTokenStreamSetPos(pos[i] - 1);
      if (!parseWord()) return false; // the type check
      if (!parseWord()) return false; // assignment
    }

    inpTokenStreamSetPos(afterInputPos); // after input and either "--" or ")"
    if (dash) {
      // skip output part for now
      for (;;) {
        if (inpTokenMatches(")")) break;
        inpTokenAdvance();
      }
    }
  }

  // rest of word code
  while (!inpTokenMatches(";")) {
    char *tok = inpTokenGet();
    if (!parseWord()) {
      return false;
    }
  }
  pcAddByte(OP_EOF);

  int codePos = pcChopInt();

  //disassemble(code);

  Serial.print(F("Saving word '"));
  Serial.print(name);
  Serial.print(F("' with code position "));
  Serial.println(codePos);

  int strPos = psGetOrAddString(name);

  mapAddCompiledWord(strPos, codePos);
  return true;
}


void parseConst () {
  if (inpTokenMatches(";")) {
    Serial.println(F("Invalid const"));
    return;
  }
  char *name = inpTokenGet();
  inpTokenAdvance();

  if (inpTokenMatches(";")) {
    Serial.println(F("Invalid const"));
    return;
  }
  char *value = inpTokenGet();
  inpTokenAdvance();

  if (!inpTokenMatches(";")) {
    Serial.println(F("Invalid const, expected ';'"));
    return;
  }

  constAdd(name, value);

}

void eepromFormat () {
  Serial.print(F("Starting format of EEPROM: "));
  unsigned long length = EEPROM.length();
  Serial.print(length);
  Serial.println(F(" bytes capacity"));

  for (int i = 0; i < length; i++) {
    EEPROM.update(i, 0);
  }


  Serial.println(F("Validating format (0-bytes)"));
  for (int i = 0; i < length; i++) {
    if (EEPROM.read(i) != 0) {
      Serial.println(F("ERROR: format incomplete"));
    }
  }
  Serial.println(F("EEPROM format completed"));
}


// Helper for 'words' command

int outWordLineLength = 0;

void outWord(char *word) {
  if (outWordLineLength > 60) {
    Serial.println();
    outWordLineLength = 0;
  }
  outWordLineLength += strlen(word) + 1;
  Serial.print(" ");
  Serial.print(word);
}


bool parseImmediateCode() {
  while (!inpTokenMatches(";")) {
    if (inpTokenMatches("help")) {
      Serial.println(F("Interactive commands"));
      Serial.println(F("--------------------"));
      Serial.println(F("words          - show defined words"));
      Serial.println(F("stats          - show memory use"));
      Serial.println(F("clear          - clear data stack"));
      Serial.println(F("hex            - show value from stack in hex"));
      Serial.println(F("bin            - show value from stack in binary"));
      Serial.println(F("dis:xxx        - disassemble word function xxx"));
      Serial.println(F("ee:format:yes  - format EEPROM storage"));
      Serial.println(F("dump           - dump code as hex"));
      continue;
    }
    if (inpTokenMatches("words") || inpTokenMatches("?")) {
      outWordLineLength = 0;

      int cnt = mapGetWordCount();
      for (int i = 0; i < cnt; i++) {
        char *fname = mapGetWordName(i);
        outWord(fname);
      }
      cnt = constGetCount();
      for (int i = 0; i < cnt; i++) {
        char *cname = constGetName(i);
        if (cname != NULL) {
          outWord(cname);
        }
      }
      Serial.println();
      continue;
    }
    if (inpTokenMatches("stats")) {
      Serial.print(F("  ps: "));
      Serial.print(psCount());
      Serial.print(" of ");
      Serial.println(PSDATA_SIZE);

      Serial.print(F("  pc: "));
      Serial.print(pcCount());
      Serial.print(" of ");
      Serial.println(PCDATA_SIZE);
      Serial.print(F("  ps + pc: "));
      Serial.println(psCount() + pcCount());

      Serial.print(F("  map:"));
      Serial.print(mapGetWordCount());
      Serial.print(" of ");
      Serial.println(MAP_SIZE);

      Serial.print(F("  Max data stack depth: "));
      Serial.println(getDsMaxStackSize());

      Serial.print(F("  Max call stack depth: "));
      Serial.println(getCsMaxStackSize());

      continue;
    }
    if (inpTokenMatches("ee:format:yes")) {
      eepromFormat();
      continue;
    }
    char *inpToken = inpTokenGet();
    if (strlen(inpToken) > 4 && inpToken[0] == 'd' && inpToken[1] == 'i' && inpToken[2] == 's' && inpToken[3] == ':') {
      char *word = inpToken + 4;
      bool ok = false;
      int strPos = psSearch(word);
      if (strPos >= 0) {
        int codePos = mapLookupCodePos(strPos);
        if (codePos >= 0) {
          Serial.println();
          disassemble(pcGetCodePointer(codePos));
          Serial.println();
          ok = true;
        }
      }
      if (!ok) Serial.println(F("Undefined word"));
      inpTokenAdvance();
      continue;
    }
    if (inpTokenMatches("clear")) {
      while (!dsEmpty()) dsPopValue();
      continue;
    }
    if (inpTokenMatches("hex")) {
      if (dsEmpty()) {
        Serial.println(F("hex: no value on stack"));
        continue;
      }
      char buf[10];
      ltoa(dsPeek(), buf, 16);
      Serial.print(F("--> 0x"));
      Serial.println(buf);
      continue;
    }

    if (inpTokenMatches("bin")) {
      if (dsEmpty()) {
        Serial.println(F("bin: no value on stack"));
        continue;
      }
      char buf[34];
      ltoa(dsPeek(), buf, 2);
      Serial.print(F("--> b"));
      Serial.println(buf);
      continue;
    }
    if (inpTokenMatches("dump")) {
      dumpHex();
      continue;
    }

    if (!parseWord()) {
      return false;
    }
  }

  // compile ok
  pcAddByte(OP_EOF);

  int codePos = pcChopInt();
  //Serial.print(F("parseImmediateCode, codePos="));
  //Serial.println(codePos);
  byte *code = pcGetCodePointer(codePos);

  //disassemble(code);

  executeCode(code);

  return true;
}

bool isDigit(char c) {
  return (c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9');
}

bool parseIf();
bool parseLoop();
bool parseSymbol();

unsigned long parseHex (char *s) {
  unsigned long val = 0;
  for (int i = 0; i < strlen(s); i++) {
    char c = s[i];
    val = val * 16;
    if (c == 'a' || c == 'A') val += 10;
    else if (c == 'b' || c == 'B') val += 11;
    else if (c == 'c' || c == 'C') val += 12;
    else if (c == 'd' || c == 'D') val += 13;
    else if (c == 'e' || c == 'E') val += 14;
    else if (c == 'f' || c == 'F') val += 15;
    else if (c == '1') val += 1;
    else if (c == '2') val += 2;
    else if (c == '3') val += 3;
    else if (c == '4') val += 4;
    else if (c == '5') val += 5;
    else if (c == '6') val += 6;
    else if (c == '7') val += 7;
    else if (c == '8') val += 8;
    else if (c == '9') val += 9;
  }
  return val;
}


bool processInteger (char *word) {

  // enter hex number as 0x...
  if (word[0] == '0' && word[1] == 'x') {
    long val = parseHex(word + 2);
    pcInt(val);
    return true;
  }

  // enter binary number as b001001, plus support underscore _ for readability
  if ((word[0] == 'b' || word[0] == 'B') && (word[1] == '1' || word[1] == '0')) {
    int val = 0;
    for (int i = 1; i < strlen(word); i++) {
      if (word[i] == '_') continue;
      val = val << 1;
      if (word[i] == '1') val++;
    }
    pcInt(val);
    return true;
  }

  // decimal numbers
  if (isDigit(word[0])) {
    pcInt(atol(word));
    return true;
  }

  return false;
}

bool parseWord() {

  if (inpTokenMatches("if{")) {
    return parseIf();
  }
  if (inpTokenMatches("loop{")) {
    return parseLoop();
  }
  if (inpTokenMatches("true")) {
    pcInt(1);
    pcAddByte(OP_AS_BOOL);
    return true;
  }
  if (inpTokenMatches("false")) {
    pcInt(0);
    pcAddByte(OP_AS_BOOL);
    return true;
  }

  char *word = inpTokenGet();

  if (word[0] == '\'' && strlen(word) > 1) {
    return parseSymbol();
  }


  // built-in symbols mapping to opcodes
  int code = lookupSymbol(word);
  if (code >= 0) {
    pcAddByte(code);

    inpTokenAdvance();
    return true;
  }

  // local variable assign
  if (word[0] == '=') {
    char *varName = word + 1;
    int pos = inpLocalVariablePos(varName);
    if (pos >= 0) {

      if (pos == 0) {
        pcAddByte(OP_LSET0);
      } else if (pos == 1) {
        pcAddByte(OP_LSET1);
      } else if (pos == 2) {
        pcAddByte(OP_LSET2);
      } else if (pos == 3) {
        pcAddByte(OP_LSET3);
      } else {
        pcInt7bit(pos);
        pcAddByte(OP_LSET);
      }

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

      if (pos == 0) {
        pcAddByte(OP_LSET0);
      } else if (pos == 1) {
        pcAddByte(OP_LSET1);
      } else if (pos == 2) {
        pcAddByte(OP_LSET2);
      } else if (pos == 3) {
        pcAddByte(OP_LSET3);
      } else {
        pcInt7bit(pos);
        pcAddByte(OP_LSET);
      }

      inpTokenAdvance();
      return true;
    }
  }

  // local variable lookup
  int varPos = inpLocalVariablePos(word);
  if (varPos >= 0) {

    if (varPos == 0) {
      pcAddByte(OP_LGET0);
    } else if (varPos == 1) {
      pcAddByte(OP_LGET1);
    } else if (varPos == 2) {
      pcAddByte(OP_LGET2);
    } else if (varPos == 3) {
      pcAddByte(OP_LGET3);
    } else {
      pcInt7bit(varPos);
      pcAddByte(OP_LGET);
    }

    inpTokenAdvance();
    return true;
  }

  // const
  char *constValue = constLookup(word);
  if (constValue != NULL) {
    if (processInteger(constValue)) {
      inpTokenAdvance();
      return true;
    } else {
      Serial.println(F("Invalid constant, must be hex number"));
      return false;
    }
  }

  if (processInteger(word)) {
    inpTokenAdvance();
    return true;
  }

  // call word function?
  int strPos = psSearch(word);
  if (strPos >= 0) {
    int codePos = mapLookupCodePos(strPos);
    if (codePos >= 0) {

      pcInt(strPos);
      pcAddByte(OP_CALL);

      inpTokenAdvance();
      return true;
    }
  }


  Serial.print(F("Unrecognized word: "));
  Serial.println(word);

  return false;

}


bool parseIf() {
  int jmpToEndAddr = pcGetLocalPos();
  pcInt7bit(0);
  pcAddByte(OP_ZJMP);

  while (!inpTokenMatches("}")) {
    if (inpTokenMatches(";")) {
      Serial.println(F("if{...} contains ';'"));
      return false;
    }
    if (!parseWord()) return false;
  }

  int endLoc = pcGetLocalPos();
  // insert endLoc into the
  pcSetByteLocal7bit(jmpToEndAddr, endLoc);

  return true;
}


bool parseLoop() {
  int startPos = pcGetLocalPos();
  int breakJmpAddr = -1;

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
      breakJmpAddr = pcGetLocalPos();
      pcInt7bit(0);
      pcAddByte(OP_CJMP);
      continue;
    }
    if (!parseWord()) return false;
  }

  // jump back to start
  pcInt(startPos);
  pcAddByte(OP_JMP);

  // after loop
  if (breakJmpAddr >= 0) {
    int endLoc = pcGetLocalPos();
    pcSetByteLocal7bit(breakJmpAddr, endLoc);
  }

  return true;
}

bool parseSymbol() {
  // just identified token starting with apostrophe

  char *token = inpTokenGet() + 1; // skip the apos

  inpTokenAdvance();

  int strPos = psGetOrAddString(token);
  pcInt(strPos);
  pcAddByte(OP_SYMBOL);

  return true;
}

// -------------------------------------
// execute code
// -------------------------------------

bool executeOneOp();

void printPadded (char *leftPad, int w1, char *str, char *rightPad, int w2) {
  w1 = w1 - strlen(str);
  w2 = w2 - strlen(str);
  for (int i = 0; i < w1; i++) {
    Serial.print(leftPad);
  }
  Serial.print(str);
  for (int i = 0; i < w2; i++) {
    Serial.print(rightPad);
  }
}

void displayStackValue (DStackValue *x) {
  char buf[34];
  switch (x->type) {
    case DS_TYPE_INT : {
        itoa((int) x->val, buf, 10);
        printPadded(" ", 10, buf, " ", 0);
        Serial.print(" 0x");
        itoa((int) x->val, buf, 16);
        printPadded("0", 4, buf, " ", 0);
        Serial.println("  INT");
        return;
      }
    case DS_TYPE_UINT : {
        itoa((unsigned int) x->val, buf, 10);
        printPadded(" ", 10, buf, " ", 8);
        itoa((unsigned int) x->val, buf, 16);
        Serial.print(" 0x");
        printPadded("0", 4, buf, " ", 8);
        Serial.println("  UINT");
        return;
      }
    case DS_TYPE_BYTE : {
        itoa((byte) x->val, buf, 10);
        printPadded(" ", 10, buf, " ", 0);
        Serial.print(" 0x");
        itoa((byte) x->val, buf, 16);
        printPadded("0", 2, buf, " ", 0);
        Serial.print(" b");
        itoa((byte) x->val, buf, 2);
        printPadded("0", 8, buf, " ", 0);
        Serial.println("  BYTE");
        return;
      }
    case DS_TYPE_LONG : {
        ltoa(x->val, buf, 10);
        printPadded(" ", 10, buf, " ", 0);
        Serial.print(" 0x");
        ltoa(x->val, buf, 16);
        printPadded("0", 8, buf, " ", 0);
        Serial.println(F("  LONG"));
        return;
      }
    case DS_TYPE_ULONG : {
        ultoa((unsigned long) x->val, buf, 10);
        printPadded(" ", 10, buf, " ", 0);
        Serial.print(" 0x");
        ultoa((unsigned long) x->val, buf, 16);
        printPadded("0", 8, buf, " ", 0);
        Serial.println(F("  ULONG"));
        return;
      }
    case DS_TYPE_ADDR : {
        unsigned long val = (unsigned long) x->val;
        unsigned long offset = (val & 0x00FFFFFF);
        byte controlByte = (byte) (val >> 24);
        // splits into locationId (4 bits) and typeId (4 bits)
        // showing as byte in hex here
        itoa(controlByte, buf, 16);
        Serial.print("      0x");
        printPadded("0", 2, buf, " ", 1);

        itoa((unsigned long) offset, buf, 16);
        Serial.print(F(" 0x"));
        printPadded("0", 8, buf, " ", 2);
        Serial.print(F("  ADDR"));
        int aloc = (controlByte >> 4) & 0x0F;
        int atyp = (controlByte & 0x0F);

        Serial.print(F(" aloc="));
        if (aloc & ALOC_EXT_BIT) {
          int id = aloc & 0x07;
          Serial.print("ext:");
          Serial.print(id);
        } else {
          int id = aloc & 0x07;
          Serial.print("oc:");
          Serial.print(id);
        }
        Serial.print(F(" atyp="));
        Serial.print(atyp);

        if (aloc == ALOC_OC_STR && atyp == ATYP_SYMBOL) {
          Serial.print(F(" SYMBOL: "));
          Serial.print(psGetStringPointer(offset));
        }
        Serial.println();
        return;
      }
    case DS_TYPE_NULL : {
        Serial.println(F("null"));
        return;
      }
    case DS_TYPE_BOOL : {
        if (x->val) {
          Serial.println(F("true"));
        } else {
          Serial.println(F("false"));
        }
        return true;
      }
  }

  Serial.print(F("** Unknown type: "));
  Serial.println(x->type);

  itoa(x->val & 0xFF, buf, 10);
  printPadded(" ", 6, buf, " ", 0);
  Serial.print(" 0x");
  itoa(x->val & 0xFF, buf, 16);
  printPadded("0", 8, buf, " ", 0);
  Serial.println();
}


void showDataStack() {
  // Show data stack
  int count = dsCount();

  Serial.println("--");
  for (int i = count - 1; i >= 0; i--) {
    Serial.print("  ");
    DStackValue *x = dsGetValue(i);
    displayStackValue(x);
  }
}

unsigned long executeStartTime = 0L;
 
void executeCode (byte *initialCode) {
  csPush(initialCode);

  long executeOpCount = 0;
  unsigned long startTime = millis();
  executeStartTime=millis();
    // can be modified by OP_KEEP_ALIVE, otherwise ensures execution timeout
    // after 10 seconds

  while (!csEmpty() && !abortCodeExecution) {

    // the execute* functions called must advance curr->pc past the
    // instruction (or instruction + data), so it refers to next instr.

    if (!executeOneOp()) {
      setAbortCodeExecution();
      break;
    }
    executeOpCount++;

    if (millis() - executeStartTime > 10000L) {
      Serial.println(F("Aborting after 10 seconds since executeStartTime"));
      Serial.println(F("- to prevent this, call keep-alive function regularly"));
      break;
    }
  }
  unsigned long endTime = millis();
  Serial.print(endTime - startTime);
  Serial.print(F(" ms #op="));
  Serial.println(executeOpCount);

  showDataStack();

}


bool executeOneOp () {
  CStackFrame *curr = csPeek();
  byte *code = curr->code;

  int pos = curr->pc;
  curr->pc = curr->pc + 1; // may also be modified by JMP-instructions

  byte b = code[pos];

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
        if (csEmpty()) {
          Serial.println(F("Call stack underflow"));
          return false;
        }
        csPop();
        return true;
      }
    case OP_CALL: {
        if (dsEmpty()) {
          Serial.println(F("OP_CALL - data stack empty"));
          return false;
        }
        int strPos = dsPop();
        int codePos = mapLookupCodePos(strPos);

        byte *code = pcGetCodePointer(codePos);
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
        long pos = dsPop();
        curr->pc = (byte) pos;

        return true;
      }
    case OP_ZJMP : {
        if (dsEmpty()) {
          Serial.println(F("OP_ZJMP - data stack empty"));
          return false;
        }
        long pos = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_ZJMP - data stack empty"));
          return false;
        }
        int cond = dsPop();
        if (cond == 0) curr->pc = (byte) pos;

        return true;
      }
    case OP_CJMP : {
        if (dsEmpty()) {
          Serial.println(F("OP_CJMP - data stack empty"));
          return false;
        }
        long pos = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_CJMP - data stack empty"));
          return false;
        }
        int cond = dsPop();
        if (cond != 0) curr->pc = (byte) pos;

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
        dsDupValue();

        return true;
      }
    case OP_READ : {
        if (dsEmpty()) {
          Serial.println(F("OP_READ - data stack empty"));
          return false;
        }
        long addr = dsPop();
        byte *ptr = (byte *) addr;
        dsPushValue(DS_TYPE_BYTE, *ptr);
        return true;
      }
    case OP_WRITE : {
        if (dsEmpty()) {
          Serial.println(F("OP_WRITE - data stack empty"));
          return false;
        }
        long addr = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_WRITE - data stack empty"));
          return false;
        }
        long value = dsPop();
        byte *ptr = (byte *) addr;
        *ptr = (byte) value;
        return true;
      }
    case OP_LSET : {
        if (dsEmpty()) {
          Serial.println(F("OP_LSET - data stack empty"));
          return false;
        }
        long varNo = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_LSET - data stack empty"));
          return false;
        }
        DStackValue *value = dsPopValue();

        DStackValue *lv = curr->localVars + varNo;
        lv->type = value->type;
        lv->val = value->val;

        return true;
      }
    case OP_LGET : {
        if (dsEmpty()) {
          Serial.println(F("OP_LGET - data stack empty"));
          return false;
        }
        long varNo = dsPop();

        DStackValue *lv = curr->localVars + varNo;
        dsPushValueCopy(lv);
        return true;
      }
    case OP_ADD : {
        if (dsEmpty()) {
          Serial.println(F("OP_ADD - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_ADD - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPush(a + b);
        return true;
      }
    case OP_SUB : {
        if (dsEmpty()) {
          Serial.println(F("OP_SUB - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_SUB - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPush(a - b);
        return true;
      }
    case OP_MUL : {
        if (dsEmpty()) {
          Serial.println(F("OP_MUL - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_MUL - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPush(a * b);
        return true;
      }
    case OP_DIV : {
        if (dsEmpty()) {
          Serial.println(F("OP_DIV - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_DIV - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPush(a / b);
        return true;
      }
    case OP_MOD : {
        if (dsEmpty()) {
          Serial.println(F("OP_MOD - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_MOD - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPush(a % b);
        return true;
      }
    case OP_NEG : {
        if (dsEmpty()) {
          Serial.println(F("OP_NEG - data stack empty"));
          return false;
        }
        long value = dsPop();
        dsPush(-value);
        return true;
      }
    case OP_GT : {
        if (dsEmpty()) {
          Serial.println(F("OP_GT - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_GT - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a > b);
        return true;
      }
    case OP_LT : {
        if (dsEmpty()) {
          Serial.println(F("OP_LT - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_LT - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a < b);
        return true;
      }
    case OP_GE : {
        if (dsEmpty()) {
          Serial.println(F("OP_GE - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_GE - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a >= b);
        return true;
      }
    case OP_LE : {
        if (dsEmpty()) {
          Serial.println(F("OP_LE - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_LE - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a <= b);
        return true;
      }
    case OP_EQ : {
        if (dsEmpty()) {
          Serial.println(F("OP_EQ - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_EQ - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a == b);
        return true;
      }
    case OP_NE : {
        if (dsEmpty()) {
          Serial.println(F("OP_NE - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_NE - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a != b);
        return true;
      }
    case OP_L_AND : {
        if (dsEmpty()) {
          Serial.println(F("OP_L_AND - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_L_AND - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a && b);
        return true;
      }
    case OP_L_OR : {
        if (dsEmpty()) {
          Serial.println(F("OP_L_OR - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_L_OR - data stack empty"));
          return false;
        }
        long a = dsPop();
        dsPushValue(DS_TYPE_BOOL, a || b);
        return true;
      }
    case OP_L_NOT : {
        if (dsEmpty()) {
          Serial.println(F("OP_L_NOT - data stack empty"));
          return false;
        }
        long val = dsPop();
        dsPushValue(DS_TYPE_BOOL, !val);
        return true;
      }
    case OP_LSHIFT : {
        if (dsEmpty()) {
          Serial.println(F("OP_LSHIFT - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_LSHIFT - data stack empty"));
          return false;
        }
        unsigned long a = dsPop();
        dsPush(a << b);
        return true;
      }
    case OP_RSHIFT : {
        if (dsEmpty()) {
          Serial.println(F("OP_RSHIFT - data stack empty"));
          return false;
        }
        long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_RSHIFT - data stack empty"));
          return false;
        }
        unsigned long a = dsPop();
        dsPush(a >> b);
        return true;
      }
    case OP_B_AND : {
        if (dsEmpty()) {
          Serial.println(F("OP_B_AND - data stack empty"));
          return false;
        }
        unsigned long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_B_AND - data stack empty"));
          return false;
        }
        unsigned long a = dsPop();
        dsPush(a & b);
        return true;
      }
    case OP_B_OR : {
        if (dsEmpty()) {
          Serial.println(F("OP_B_OR - data stack empty"));
          return false;
        }
        unsigned long b = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_B_OR - data stack empty"));
          return false;
        }
        unsigned long a = dsPop();
        dsPush(a | b);
        return true;
      }
    case OP_B_NOT : {
        if (dsEmpty()) {
          Serial.println(F("OP_B_NOT - data stack empty"));
          return false;
        }
        unsigned long val = dsPop();
        dsPush(~val);
        return true;
      }
    case OP_LSET0 : {
        if (dsEmpty()) {
          Serial.println(F("OP_LSET0 - data stack empty"));
          return false;
        }
        DStackValue *value = dsPopValue();

        DStackValue *lv = curr->localVars;
        lv->type = value->type;
        lv->val = value->val;
        return true;
      }
    case OP_LSET1 : {
        if (dsEmpty()) {
          Serial.println(F("OP_LSET1 - data stack empty"));
          return false;
        }
        DStackValue *value = dsPopValue();

        DStackValue *lv = curr->localVars + 1;
        lv->type = value->type;
        lv->val = value->val;
        return true;
      }
    case OP_LSET2 : {
        if (dsEmpty()) {
          Serial.println(F("OP_LSET2 - data stack empty"));
          return false;
        }
        DStackValue *value = dsPopValue();

        DStackValue *lv = curr->localVars + 2;
        lv->type = value->type;
        lv->val = value->val;
        return true;
      }
    case OP_LSET3 : {
        if (dsEmpty()) {
          Serial.println(F("OP_LSET3 - data stack empty"));
          return false;
        }
        DStackValue *value = dsPopValue();

        DStackValue *lv = curr->localVars + 3;
        lv->type = value->type;
        lv->val = value->val;
        return true;
      }
    case OP_LGET0 : {
        dsPushValueCopy(curr->localVars);
        return true;
      }
    case OP_LGET1 : {
        dsPushValueCopy(curr->localVars + 1);
        return true;
      }
    case OP_LGET2 : {
        dsPushValueCopy(curr->localVars + 2);
        return true;
      }
    case OP_LGET3 : {
        dsPushValueCopy(curr->localVars + 3);
        return true;
      }
    case OP_AS_BYTE : {
        if (!dsTypeCast(DS_TYPE_BYTE)) {
          Serial.println(F(":byte failed"));
          return false;
        } else {
          return true;
        }
      }
    case OP_AS_INT : {
        if (!dsTypeCast(DS_TYPE_INT)) {
          Serial.println(F(":int failed"));
          return false;
        } else {
          return true;
        }
      }
    case OP_AS_UINT : {
        if (!dsTypeCast(DS_TYPE_UINT)) {
          Serial.println(F(":uint failed"));
          return false;
        } else {
          return true;
        }
      }
    case OP_AS_LONG : {
        if (!dsTypeCast(DS_TYPE_LONG)) {
          Serial.println(F(":long failed"));
          return false;
        } else {
          return true;
        }
      }
    case OP_AS_ULONG : {
        if (!dsTypeCast(DS_TYPE_ULONG)) {
          Serial.println(F(":ulong failed"));
          return false;
        } else {
          return true;
        }
      }
    case OP_MILLIS : {
        dsPushValue(DS_TYPE_ULONG, millis());
        return true;
      }
    case OP_EE_READ : {
        if (dsEmpty()) {
          Serial.println(F("OP_EE_READ - data stack empty"));
          return false;
        }
        long addr = dsPop();
        dsPushValue(DS_TYPE_BYTE, EEPROM.read(addr));
        return true;
      }
    case OP_EE_WRITE : {
        if (dsEmpty()) {
          Serial.println(F("OP_EE_WRITE - data stack empty"));
          return false;
        }
        long addr = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_EE_WRITE - data stack empty"));
          return false;
        }
        long value = dsPop();
        EEPROM.update(addr, value);
        return true;
      }
    case OP_EE_LENGTH : {
        dsPushValue(DS_TYPE_INT, EEPROM.length());
        return true;
      }
    case OP_NULL : {
        dsPushValue(DS_TYPE_NULL, 0);
        return true;
      }
    case OP_NOP : {
        // do nothing
        return true;
      }
    case OP_AS_SYM : {
        if (!dsTypeCast(DS_TYPE_ADDR)) {
          Serial.println(F(":sym failed"));
          return false;
        }
        DStackValue *val = dsPeekValue();
        int controlByte = (val->val >> 24);
        int locationId = (controlByte & B11110000) >> 4;
        int typeId = controlByte & B00001111;
        if (typeId == ATYP_SYMBOL) {
          return true;
        }

        Serial.println(F(":sym invalid ADDR - not referring to symbol"));
        return false;
      }
    case OP_AS_ADDR : {
        if (!dsTypeCast(DS_TYPE_ADDR)) {
          Serial.println(F(":addr failed"));
          return false;
        } else {
          return true;
        }
      }
    case OP_ABORT : {
        Serial.println(F("OP_ABORT: listing stack, and terminating"));
        setAbortCodeExecution();
        return true;
      }
    case OP_AS_BOOL : {
        if (!dsTypeCast(DS_TYPE_BOOL)) {
          Serial.println(F(":bool failed"));
          return false;
        } else {
          return true;
        }
      }
    case OP_ADDR : { // Create ADDR value
        if (dsEmpty()) {
          Serial.println(F("OP_ADDR - data stack empty"));
          return false;
        }
        unsigned long offset = (unsigned long) dsPop();

        if (dsEmpty()) {
          Serial.println(F("OP_ADDR - data stack empty"));
          return false;
        }
        int typeId = dsPop();

        if (dsEmpty()) {
          Serial.println(F("OP_ADDR - data stack empty"));
          return false;
        }
        int locationId = dsPop();

        unsigned long content = (( locationId & 0x0F ) << 4) | ( typeId & 0x0F );
        Serial.println(content);
        unsigned long result = (content << 24) | ( offset & 0x00FFFFFF );

        dsPushValue (DS_TYPE_ADDR, result);
        return true;
      }
    case OP_U14: { // create int from two 7 bits push (high + low)
        if (dsEmpty()) {
          Serial.println(F("OP_U14 - data stack empty"));
          return false;
        }
        unsigned int low = dsPop();

        if (dsEmpty()) {
          Serial.println(F("OP_U14 - data stack empty"));
          return false;
        }
        unsigned int high = dsPop();

        dsPushValue(DS_TYPE_LONG, (high << 7) | low);
        return true;
      }

    case OP_SPI_BEGIN : { // ( :bool =MSBFIRST :byte =SPIMODE )
        if (dsEmpty()) {
          Serial.println(F("OP_SPI_BEGIN - data stack empty"));
          return false;
        }
        int spiMode = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_SPI_BEGIN - data stack empty"));
          return false;
        }
        byte msbFirst = dsPop();

        SPI.begin();
        if (msbFirst) {
          SPI.setBitOrder(MSBFIRST);
        } else {
          SPI.setBitOrder(LSBFIRST);
        }

        SPI.setDataMode(spiMode);
        SPI.setClockDivider(SPI_CLOCK_DIV4);
        return true;
      }
    case OP_SPI_TRANSFER : {  // ( :byte =in -- :byte )
        if (dsEmpty()) {
          Serial.println(F("OP_SPI_TRANSFER - data stack empty"));
          return false;
        }
        byte val = dsPop();
        dsPush(SPI.transfer(val));

        return true;
      }
    case OP_SPI_END : {
        SPI.end();
        return true;
      }
    case OP_SYMBOL : {
        if (dsEmpty()) {
          Serial.println(F("OP_SYMBOL - data stack empty"));
          return false;
        }
        int strPos = dsPop();

        // create ADDR value
        unsigned long markControlByte = (ALOC_OC_STR << 4) | (ATYP_SYMBOL);
        unsigned long addrVal = (unsigned long) strPos;
        unsigned long value = (markControlByte << 24) | addrVal;

        dsPushValue (DS_TYPE_ADDR, (long) value);
        return true;
      }
    case OP_TWI_BEGIN : {
        Wire.begin();
        return true;
      }
    case OP_TWI_END : {
        Wire.end();
        return true;
      }
    case OP_TWI_SETCLOCK : {
        if (dsEmpty()) {
          Serial.println(F("OP_TWI_SETCLOCK - data stack empty"));
          return false;
        }
        unsigned long freq = dsPop();
        Wire.setClock(freq);
        return true;
      }
    case OP_TWI_TR_BEGIN : {
        if (dsEmpty()) {
          Serial.println(F("OP_TWI_TR_BEGIN - data stack empty"));
          return false;
        }
        byte address = dsPop();
        Wire.beginTransmission(address);
        return true;
      }
    case OP_TWI_TR_END : {
        dsPush(Wire.endTransmission());
        return true;
      }
    case OP_TWI_WRITE : {
        if (dsEmpty()) {
          Serial.println(F("OP_TWI_WRITE - data stack empty"));
          return false;
        }
        byte value = dsPop();
        Wire.write(value);
        return;
      }
    case OP_TWI_REQUEST : {
        if (dsEmpty()) {
          Serial.println(F("OP_TWI_REQUEST - data stack empty"));
          return false;
        }
        int count = dsPop();
        if (dsEmpty()) {
          Serial.println(F("OP_TWI_REQUEST - data stack empty"));
          return false;
        }
        int address = dsPop();
        Wire.requestFrom(address, count);
        return true;
      }
      case OP_TWI_READ : {
        dsPush(Wire.read());
        return true;
      }
      case OP_KEEP_ALIVE : {
        executeStartTime=millis();
        return true;
      }

  }
  Serial.print(F("Unknown OP "));
  Serial.println(b);
  return false;
}




# define PROGMEM_DATA_TERMINATOR    0x00

// Serialized save format for firmware code on format
//
// symbol \0
// symbol \0
// 0
// int7bit (high)
// int7bit (low)
// instructions ... EOF
// 0

byte *lookupProgmemCode (char *name) {
  return NULL;
}

const byte PROGMEM_DATA[] PROGMEM = {
  // ps
  82, 101, 103, 33, 0,
  119, 97, 105, 116, 0,
  36, 97, 108, 111, 99, 58, 111, 99, 58, 115, 116, 114, 0,
  36, 97, 108, 111, 99, 58, 111, 99, 58, 98, 105, 110, 0,
  36, 97, 108, 111, 99, 58, 111, 99, 58, 112, 114, 111, 103, 109, 101, 109, 0,
  36, 97, 108, 111, 99, 58, 111, 99, 58, 101, 101, 0,
  36, 97, 116, 121, 112, 58, 115, 121, 109, 98, 111, 108, 0,
  36, 97, 116, 121, 112, 58, 98, 108, 111, 98, 0,
  0,

  // code
  // Reg!
  128, 128, // strPos
  41, 32, 40, 33, 40, 34, 36, 8, 38, 31, 29, 37, 38, 29, 30, 36, 9, 0,
  // wait
  128, 133, // strPos
  44, 32, 45, 36, 12, 32, 45, 36, 18, 128, 143, 56, 5, 134, 3, 0,
  // $aloc:oc:str
  128, 138, // strPos
  129, 0,
  // $aloc:oc:bin
  128, 151, // strPos
  130, 0,
  // $aloc:oc:progmem
  128, 164, // strPos
  131, 0,
  // $aloc:oc:ee
  128, 181, // strPos
  132, 0,
  // $atyp:symbol
  128, 193, // strPos
  129, 0,
  // $atyp:blob
  128, 206, // strPos
  130, 0,
  0,

  PROGMEM_DATA_TERMINATOR,
  PROGMEM_DATA_TERMINATOR,
};
