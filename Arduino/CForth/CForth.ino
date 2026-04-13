#define ATMEGA328p

#include <avr/pgmspace.h>
#include "Constants.h"




void setup() {
  Serial.begin(9600);
  Serial.print(F_CPU / 1000000.0);
  Serial.println(F(" MHz"));
  Serial.println(F("Ok"));
  memInit();
  stacksInit();
}

char nextWord[MAX_WORD_LENGTH+1];  // input buffer

Word programCounter=0;
unsigned long instructionCount=0;

DictEntry *dictionaryHead=NULL; 


boolean errorFlag = false;

char temp[8];




void sPrint (char *msg) {
  Serial.print(msg);
}

void sPrintWord (Word word) {
  Serial.print(word);
}

void sPrintByte (Byte b) {
  Serial.print(b);
}

void sPrintDouble (double d) {
  Serial.print(d);
}

void sPrintln () {
  Serial.println();
}


void clearHasError() {
  errorFlag=false;
}

void setHasError () {
  if (!errorFlag) {
    errorFlag=true;
    Serial.println();
    for (Byte i=0; i<20; i++) Serial.print("-");
    Serial.println();
    Serial.println(F("Error"));
    for (Byte i=0; i<20; i++) Serial.print("-");
    Serial.println();
    delay(1000);
    clearInputBuffer();
  }
}

Byte hasError() {
  return errorFlag;
}


void clearInputBuffer() {
  while (Serial.available()) Serial.read();
}

boolean keyPressed() {
  return (boolean) Serial.available();
}

Word readSerialChar () {
  for(;;) {
    int ch=Serial.read();
    if (ch >= 0) {
      if (ch==13 || ch==10) {
        if (READC_ECHO) Serial.println();
      }  else {
        if (READC_ECHO) Serial.print((char) ch);
      }
      return (Word) ch;
    }
  }
}

void printChar (Word ch) {
  sprintf(temp,"%c", ch);
  Serial.print(temp);
}

void printStr (Word ptr) {
  Word len=readByte(ptr);
  for (int i=0; i<len; i++) {
    printChar(readByte(ptr+i+1));
  }
}

int commentLevel=0;  // nested parantheses count
void readNextWord () {
  Byte pos=0;

  for(;;) {
    char ch=readSerialChar();
    if (ch=='(') {
      commentLevel++;
      continue;
    } else if (ch==')') {
      commentLevel--;
      if (commentLevel < 0) commentLevel=0;
      continue;
    }
    if (ch==13 || ch==10 || ch==32 || commentLevel>0) {
      if (pos > 0) {
        nextWord[pos]='\0';
        return;
      } // else ignore
    } else {
      if (pos > MAX_WORD_LENGTH-1) {
        setHasError();
        Serial.println(F("word too long"));
        nextWord[pos]='\0';

        return;
      }
      nextWord[pos++]=ch;
    }
  }
}



// when an op requires additional Bytes (bval and cval - push Byte and cell value)
Byte getOpcodeParameter() {
  return codeSegmentGet(programCounter++);
}




DictEntry *dictLookupNextWord () {
  DictEntry *ptr=dictionaryHead;
  while (ptr != NULL) {
    if (!strcmp(ptr->name, nextWord)) {
      return ptr;
    }
    ptr=ptr->next;
  }
  return NULL;
}


// Used by disassembler
DictEntry *dictLookupByAddr (Word addr) {
  DictEntry *ptr=dictionaryHead;
  addr=generateCodeAddress(addr);
  while (ptr != NULL) {
    if (ptr->address==addr) return ptr;
    ptr=ptr->next;
  }
  return NULL;
}


void compileNumberByte (Byte b) {
    if (b==0) {
      compileOut(OP_ZERO);
    } else if (b==1) {
      compileOut(OP_ONE);
    } else {
      compileOut(OP_BVAL);
      compileOut(b);
    }
}

void compileNumberCell (Word w) {
    compileOut(OP_CVAL);
    compileOut((w>>8) & 0xFF);
    compileOut(w & 0xFF);
}

void compileNumber (Word w) {
  if (w < 255) {
    compileNumberByte(w);
  } else {
    compileNumberCell(w);
  }
}

/*
Enhanced atoi, recognizes decimal, hex (0x...) and binary (b....) and negative, returns boolean to 
indicate success or failure. Writes result int *target
*/
boolean myAtoi (int *target) {
  int readPos=0;
  
  boolean negative=false;
  if (nextWord[readPos]=='-') {
    negative=true;
    readPos++;
    if(nextWord[readPos] == '\0') return false;
  }

  long value=0; 

  if (!strncmp(nextWord+readPos,"0x",2)) {
    // parsing hex
    readPos+=2;

    while(nextWord[readPos] != '\0') {
      char c=nextWord[readPos++];
      if (c=='\0') return false; // no digits

      if (c>='0' && c <= '9') {
        value=value*16 + (c-'0');
      } else if (c>='A' && c<='F') {
        value=value*16 + (c-'A') + 10;
      } else if (c>='a' && c<='f') {
        value=value*16 + (c-'a') + 10;
      } else {
        return false;  // fail
      }
    }
    // ok
  } else if (nextWord[readPos]=='b') {
    // possibly binary
    readPos++;
    if (nextWord[readPos]=='\0') return false; // no digits

    while(nextWord[readPos] != '\0') {
      char c=(char) nextWord[readPos++];
      if (c>='0' && c <= '1') {
        value=value*2 + (c-'0');
      } else {
        return false;  // fail
      }
    }
    // ok
  } else {
    // decimal?
    while(nextWord[readPos] != '\0') {
      char c=(char) nextWord[readPos++];
      if (c>='0' && c <= '9') {
        value=value*10 + (c-'0');
      } else {
        return false;  // fail
      }
    }
    // ok
  } 

  if (negative) value=-value;
  *target=value;
  return true; // success
}


void callForth (Word addr) {
  rpush(programCounter);
  programCounter=addr & ADDR_CODE_MASK;
  //Serial.print("callForth addr=");
  //Serial.println(programCounter);
}


/*
* Compile nextWord, true if ok, false if error
*/
void compileNextWord () {
  /*
  Serial.println();
  Serial.print("compileNextWord ");
  Serial.print(nextWord);
  Serial.print(" compileNext=");
  Serial.println(compileNext);
  */
  // check for number
  int i=0;
  if (myAtoi(&i)) {
    Word w=(Word) i;
    compileNumber(i);
    return;
  } 

  DictEntry *de=dictLookupNextWord();
  if (de != NULL) {
    if (de->type==DE_TYPE_NORMAL) {
      // ensure 14 bit address, then set high bit=1
      Word addr=generateCallAddress(de->address);
      compileOut((addr>>8) & 0xFF); 
      compileOut(addr & 0xFF);
    } else if (de->type==DE_TYPE_IMMEDIATE) {
      callForth(de->address);
      executeCode();
    } else if (de->type==DE_TYPE_CONSTANT) {
      compileNumber(de->address);
    }
    return;
  }

  int opcode=lookupOpCode();
  if (opcode >= 0) {
    compileOut(opcode);
    return;
  }

  setHasError();
  Serial.print(F("Unknown: "));
  Serial.println(nextWord);
}


void create () {
  readNextWord();
  char *ptr=(char *) malloc(strlen(nextWord)+1);
  strcpy(ptr,nextWord);

  DictEntry *de=(DictEntry *) malloc(sizeof(DictEntry));
  de->name=ptr;
  de->type=DE_TYPE_CONSTANT;
  de->address=0;
  de->next=dictionaryHead;
  dictionaryHead=de;
}


// -------
// opcode implemenations
// -------

#define NUM_TAGS_REFS     5
  // 0-4

boolean getTagNumber (char *ptr, int *tag) {
  if (*ptr == '\0') return false;
  if (*(ptr+1) != '\0') return false; // single digit only
  if (*ptr >= '0' && *ptr <= '4') {
    *tag = *ptr - '0';
    if (*tag < 0 || *tag >= NUM_TAGS_REFS) {
      setHasError();
      Serial.print(F("Unknown tag "));
      Serial.println(*tag);
      return false;
    }
    return true;
  }
  return false;
}

struct Ref {
  Byte tag;
  Word addr;
};


void op_reserved() {
  Serial.print(F("undefined"));
}


void op_colon() {  
  initCompileNext();
  Word startPos=getCodeNext();

  Word tags[NUM_TAGS_REFS]; 
  struct Ref refs[NUM_TAGS_REFS];

  for (int i=0; i<NUM_TAGS_REFS; i++) {
    tags[i]=0;
  }
  
  Byte nextRef=0;

  // create dictionary entry (CONSTANT address=0)
  create();

  // write dummy length Byte (patched at semicolon)
  // (this enables disassembling)
  compileOut(99);

  for (;;) {
    if (hasError()) return;

    readNextWord();

    // define tag?
    if (*nextWord=='/') {
      int tag=0;
      if (getTagNumber(nextWord+1, &tag)) {
        /*
        Serial.println();
        Serial.print("*** tag ");
        Serial.print(tag);
        Serial.print("=");
        Serial.println(compileNext);
        */
        tags[tag] = getCompileNext();
        continue;
      }
    }

    // tag lookup?
    if (*nextWord=='&') {
      int tag=0;
      if (getTagNumber(nextWord+1, &tag)) {

        compileOut(OP_CVAL);

        refs[nextRef].tag=tag;
        refs[nextRef].addr=getCompileNext();
        /*
        Serial.println();
        Serial.print("*** ref ");
        Serial.print(tag);
        Serial.print(" at ");
        Serial.println(compileNext);
        */
        // place holder to be patched
        compileOut(0);
        compileOut(0);
        
        nextRef++;
        continue;
      }
    }

    if (!strcmp(nextWord,";")) {
      compileOut(OP_RET);

      // patch tag references
      for (int i=0; i<nextRef; i++) {
        Byte tag=refs[i].tag;
        Word tagAddr=tags[tag];
        if (tagAddr==0) {
          setHasError();
          Serial.print(F("Invalid tag reference: "));
          Serial.println(tag);
          return;
        }
        Word patchAddr=refs[i].addr;
        /*
        Serial.println();
        Serial.print("*** PATCHING ref ");
        Serial.print(tag);
        Serial.print(" at ");
        Serial.print(patchAddr);
        Serial.print(" -> ");
        Serial.println(tagAddr);
        */

        codeSegmentSet(patchAddr, (tagAddr >> 8) & 0xFF);
        codeSegmentSet(patchAddr+1, tagAddr & 0xFF);
      }

      Byte ByteCount=(Byte) (getCompileNext()-startPos-1);  // length Byte not included
      DictEntry *de=dictionaryHead;  // from call to create() 

      Serial.println();
      Serial.print(de->name);
      Serial.print(" ");
      Serial.print(ByteCount);
      Serial.println(F(" Bytes"));

      // patch length Byte
      
      codeSegmentSet(startPos, ByteCount);

      de->address=generateCodeAddress(startPos+1);  // past length Byte
      de->type=DE_TYPE_NORMAL;

      setCodeNext(getCompileNext());

      return;
    }

    // all other words except semicolon
    compileNextWord();
  }
}


void op_bval() {push(getOpcodeParameter());}
void op_cval() {push(getOpcodeParameter()<<8 | getOpcodeParameter());}

void op_dcall() {     // dynamic call
  Word addr=pop();
  callForth(addr);
}
void op_zero() {push(0);}
void op_one() {push(1);}
void op_add() {Word b=pop(); Word a=pop(); push(a+b);}
void op_sub() {Word b=pop(); Word a=pop(); push(a-b);}
void op_mul() {Word b=pop(); Word a=pop(); push(a*b);}
void op_div() {Word b=pop(); Word a=pop(); push(a/b);}
void op_modulo() {Word b=pop(); Word a=pop(); push(a%b);}
void op_incr() {Word x=pop(); push(x+1);}

void op_gt() {Word b=pop(); Word a=pop(); push(a>b ? 1 : 0);}
void op_ge() {Word b=pop(); Word a=pop(); push(a>=b ? 1 : 0);}
void op_lt() {Word b=pop(); Word a=pop(); push(a<b ? 1 : 0);}
void op_le() {Word b=pop(); Word a=pop(); push(a<=b ? 1 : 0);}
void op_eq() {Word b=pop(); Word a=pop(); push(a==b ? 1 : 0);}
void op_ne() {Word b=pop(); Word a=pop(); push(a!=b ? 1 : 0);}
void op_and() {Word b=pop(); Word a=pop(); push(a != 0 && b != 0 ? 1 : 0);}
void op_or() {Word b=pop(); Word a=pop(); push(a != 0 || b != 0 ? 1 : 0);}
void op_not() {Word x=pop(); push(x==0 ? 1 : 0);}
void op_bin_and() {Word b=pop(); Word a=pop(); push(a&b);}
void op_bin_or() {Word b=pop(); Word a=pop(); push(a|b);}
void op_bin_inv() {Word x=pop(); push(~x);}


void op_cr() {Serial.println();}
void op_dot() {int i=(int) pop(); Serial.print(i); Serial.print(" ");}
void op_dot_u() {Word x=pop(); Serial.print(x); Serial.print(" ");}
void op_dot_hex() {Word x=pop(); Serial.print("0x"); Serial.print(x,16); Serial.print(" ");}
void op_dot_c() {Word x=pop(); char c=(x&0xFF); Serial.print(c);}
void op_dot_str() {Word addr=pop(); printStr(addr); }

void op_cseg_here() {Word addr=generateCodeAddress(getCodeNext()); push(addr);}
void op_comp_next() {Word addr=generateCodeAddress(getCompileNext()); push(addr);}
void op_comp_out() {Word x=pop(); compileOut(x & 0xFF);}

void op_HERE() {push(addrHERE());}

void op_allot() {
  Word count=pop();
  allot(count);
}

void op_constant() {
  Word value=pop();
  create();
  dictionaryHead->type=DE_TYPE_CONSTANT;
  dictionaryHead->address=value;
}

void op_variable() {
  Word variableAddr=addrHERE(); 
  allot(2); 
  writeWord(variableAddr,pop());
  push(variableAddr);
  op_constant();
}

void op_write() {Word addr=pop(); Word value=pop(); writeWord(addr,value);}
void op_read() {Word addr=pop(); push(readWord(addr));}
void op_writeb() {Word addr=pop(); Word x=pop(); writeByte(addr,x);}
void op_readb() {Word addr=pop(); push(readByte(addr));}

void op_ret() {programCounter=rpop();}
void op_cond_ret() {Word cond=pop(); if (cond != 0) programCounter=rpop();}

void op_create() {
  create();
}
void op_immediate() {
  if (dictionaryHead != NULL && dictionaryHead->type==DE_TYPE_NORMAL) dictionaryHead->type=DE_TYPE_IMMEDIATE;
}

void op_dup() {push(pick(0));}
void op_swap() {Word b=pop(); Word a=pop(); push(b); push(a);}
void op_2dup() {push(pick(1)); push(pick(1));}
void op_drop() {pop();}
void op_over() {push(pick(1));}
void op_pick() {Word n=pop(); push(pick(n));}


void op_show_stack() {
  dStackShow();
}

void op_clear_stack() {
  dStackClear();
}

// Speed testing

unsigned long testStart=0L;
unsigned long testCount=0L;

void op_start_test () {
  testStart=millis();
  testCount=instructionCount;
}

void op_end_test () {
  Serial.println();
  unsigned long duration=millis()-testStart;
  unsigned long count=instructionCount-testCount;
  double seconds=duration/1000.0;
  Serial.print(F("op/s "));
  double op_per_s = count/seconds;
  Serial.println(op_per_s);
}


void op_words() {
  DictEntry *ptr=dictionaryHead;
  Serial.println();
  while (ptr != NULL) {
    Serial.print(ptr->name);
    Serial.print(" ");
    Serial.print(ptr->address);
    Serial.print(" ");
    if (ptr->type==DE_TYPE_IMMEDIATE) Serial.print(F("immediate"));
    if (ptr->type==DE_TYPE_CONSTANT) Serial.print(F("constant"));
    Serial.println();
    ptr=ptr->next;
  }
}
void op_to_r() {
  rpush(pop());
}
void op_r_from() {
  push(rpop());
}

void op_jmp() {
  programCounter=pop();
}
void op_cond_jmp() {
  Word addr=pop();
  Word cond=pop();
  if (cond != 0) programCounter=addr;
}

void op_blob() {
  // followed by length field n, then n Bytes
  // skip the data, and push the address of the length Byte 
  Word lengthPointer = programCounter;
  Byte length=getOpcodeParameter();
  programCounter += length;
  // convert to address by setting high bit
  push(generateCodeAddress(lengthPointer));
}

void op_key() {
  push(keyPressed() ? 1 : 0);
}

void op_readc() {
  push(readSerialChar());
}

void op_word_addr() {
  readNextWord();
  DictEntry *de=dictLookupNextWord();
  if (de==NULL) {
    push(0);
  } else {
    push(generateCodeAddress(de->address));
  }
}

// may represent this list fully as a string of PROGMEM Bytes

const OpCode opCodes[]={
  {"", &op_reserved}, 
  {":", &op_colon},

  {"bval", &op_bval},     // 2
  {"cval", &op_cval},     // 3
  {"ret", &op_ret},       // 4
  {"jmp", &op_jmp},       // 5
  {"jmp?", &op_cond_jmp}, // 6
  {"blob", &op_blob},     // 7
  {"zero", &op_zero},     // 8
  {"one", &op_one},     // 9
  
  {"dcall", &op_dcall},
  {"ret?", &op_cond_ret},

  {"create", &op_create}, 
  {"immediate", &op_immediate}, 
  {"+", &op_add},
  {"-", &op_sub},
  {"*", &op_mul},
  {"/", &op_div},
  {"%", &op_modulo},
  {"1+", &op_incr},

  {">", &op_gt},
  {">=", &op_ge},
  {"<", &op_lt},
  {"<=", &op_le},
  {"==", &op_eq},
  {"!=", &op_ne},
  {"and", &op_and},
  {"or", &op_or},
  {"not", &op_not},

  {"&", &op_bin_and},
  {"|", &op_bin_or},
  {"inv", &op_bin_inv},


  {"cr", &op_cr},
  {".", &op_dot},
  {".u", &op_dot_u},
  {".hex", &op_dot_hex},
  {".c", &op_dot_c},
  {".str", &op_dot_str},

  {"csegHERE", &op_cseg_here},
  {"comp.next", &op_comp_next},
  {"comp.out", &op_comp_out},
  {"HERE", &op_HERE},
  {"allot", &op_allot},
  {"constant", &op_constant},
  {"variable", &op_variable},
   
  {"!", &op_write},
  {"@", &op_read},

  {"b!", &op_writeb},
  {"b@", &op_readb},

  {"dup", &op_dup},
  {"swap", &op_swap},
  {"2dup", &op_2dup},
  {"drop", &op_drop},
  {"over", &op_over},
  {"pick", &op_pick},

  {"?", &op_words},
  {".s", &op_show_stack},
  {"clear", &op_clear_stack},

  {"[test", &op_start_test},
  {"test]", &op_end_test},
  
  {">R", &op_to_r},
  {"R>", &op_r_from},
  {"key", &op_key},
  {"readc", &op_readc},


  {"'", &op_word_addr},
  {"step", &op_step},
  {"dis", &op_dis},

  // end-marker
  {"",0}
};


void op_step() {
  programCounter=pop();
  Serial.print(F("programCounter="));
  Serial.println(programCounter);
  dStackShow();

  Byte op=codeSegmentGet(programCounter++);
  Serial.print(F("op="));
  Serial.println(op);

  executeCodeByte(op);
  Serial.print(F("programCounter="));
  Serial.println(programCounter);
  dStackShow();
  push(programCounter);

  // prevent auto processing
  programCounter=0;
}

void op_dis() {
  Word codeAddr = pop();
  Byte len=codeSegmentGet(codeAddr-1);
  Serial.println();
  Serial.print(F("codeAddr="));
  Serial.print(codeAddr);
  Serial.print(F(" length="));
  Serial.println(len);

  Byte dataBytes=0;
  for (Byte i=0; i<len; i++) {
    Word addr=codeAddr + i;
    Serial.print(addr);
    Serial.print("  ");

    Byte op=codeSegmentGet(addr);
    Serial.print(op);
  
    if (dataBytes > 0) {
      Serial.println(F(" (data)"));
      dataBytes--;
      continue;
    }

    Serial.print(" ");
    if (op & BYTE_CALL_BIT) {
      dataBytes=1;
      Serial.print(F("(call)"));
      Word forthAddr=(op << 8) | codeSegmentGet(addr+1);
      // strip away topmost 2 bits
      forthAddr=forthAddr & ADDR_CODE_MASK;
      Serial.print(F(" forthAddr="));
      Serial.print(forthAddr);

      DictEntry *de=dictLookupByAddr(forthAddr);
      if (de==NULL) {
        setHasError();
        Serial.println(F("unknown forth code address"));
        return;
      } 
      Serial.print(" ");
      Serial.print(F("=>"));
      Serial.print(" ");
      Serial.println(de->name);

      if (de->type != DE_TYPE_NORMAL && de->type != DE_TYPE_IMMEDIATE) {
        setHasError();
        Serial.println(" constant - not callable!");
        return;
      }
      continue;      
    }
  
    // op = opcode 
    Serial.print(opCodes[op].name);
    if (op==OP_BVAL) {
      Word val=codeSegmentGet(addr+1);
      Serial.print(" ");
      Serial.print(val);
      Serial.print(" ");
      Serial.print("0x");
      Serial.print(val,16);
      dataBytes=1;
    } else if (op==OP_CVAL) {
      dataBytes=2;
      Word val=(codeSegmentGet(addr+1) << 8) | codeSegmentGet(addr+2);
      Serial.print(" ");
      Serial.print(val);
      Serial.print(" ");
      Serial.print("0x");
      Serial.print(val,16);
    } else if (op==OP_BLOB) {
      dataBytes=codeSegmentGet(addr+1)+1;
    }
    Serial.println();
  }
}

// check nextWord against the opCodes array, return index or -1 if not found
int lookupOpCode () {
  Byte pos=0;
  for (Byte pos=0; pos < 255 ; pos++) {
    if (opCodes[pos].f == 0) return -1;  // end of list, not found
    if (!strcmp(nextWord,opCodes[pos].name)) return pos;
  }
  return -1;
}

void executeCodeByte (Byte b) {
  // detect high bit set, this indicates a Forth call address (14 bits)

  if (b & BYTE_CALL_BIT) {
    Word address = (b<<8) | getOpcodeParameter();
    /*Serial.print("call=");
    Serial.println(address);*/
    callForth(address);
  } else {
    // opcode
    opCodes[b].f();
  }
  instructionCount++;
}


void executeCode() {
  while (programCounter != 0) {
    if (hasError()) {
      programCounter=0;
      return;     
    }
    executeCodeByte(codeSegmentGet(programCounter++));
  }
}

// interpreting main loop
void loop() {
  clearHasError();

  executeCode();
  if (hasError()) {
    return;
  }
  
  readNextWord();

  int i=0;
  if (myAtoi(&i)) {
    push ((Word) i);
    return;
  }

  DictEntry *de=dictLookupNextWord();
  if (de != NULL) {
    if (de->type==DE_TYPE_CONSTANT) {
      push(de->address);
    } else {
      callForth(de->address);
    }
    return;
  }
  
  int op = lookupOpCode();
  if (op>=0) {
    opCodes[op].f();
    return;
  }
  setHasError();
  Serial.print(F("Unknown word: "));
  Serial.println(nextWord);
}

////////

const PROGMEM Byte lookupTable[] = {0,1,2,3,5};

void readExample() {
  uint16_t verdi = pgm_read_byte_near(2);
}
