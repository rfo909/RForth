#define ATMEGA328p


typedef struct {
  char *name;
  void (*f) ();
} OpCode;

typedef unsigned int Word;

#define DSTACK_SIZE     16
#define RSTACK_SIZE     16

#define CODE_SEGMENT_SIZE     200
#define DATA_SEGMENT_SIZE     200

Word dStack[DSTACK_SIZE];
byte dStackNext=0;
Word rStack[RSTACK_SIZE];
byte rStackNext=0;

#define DE_TYPE_NORMAL      1
#define DE_TYPE_IMMEDIATE   2
#define DE_TYPE_CONSTANT    3  // value stored in address field

typedef struct DictEntry {
  char *name;
  byte type;
  Word address;

  struct DictEntry *next;
} DictEntry;


#define OP_BVAL  2
#define OP_CVAL  3
#define OP_RET   4
#define OP_JMP   5
#define OP_COND_JMP 6
#define OP_BLOB  7

#define MAX_WORD_LENGTH   16

#define READC_ECHO  true

#define WORD_INVALID      0xFFFF

boolean hasError = false;

byte codeSegment[CODE_SEGMENT_SIZE];
byte dataSegment[DATA_SEGMENT_SIZE];

Word codeNext=1;  // "codeHERE" - programCounter 0 means no code running (keeping it unsigned)
Word compileNext=1;

Word dataNext=0;   // "dataHERE"

Word generateCodeAddress (Word codeSegmentPos) {
  return codeSegmentPos | 0x8000;   // 10xxxxxx xxxxxxxx
}

Word generateDataAddress (Word dataSegmentPos) {
  return dataSegmentPos | 0xC000;   // 11xxxxxx xxxxxxxx
}

Word codeHere() {
  return generateDataAddress(codeNext);
}

Word dataHere() {
  return generateDataAddress(dataNext);
}



void compileOut (byte b) {
  /*
  Serial.print(F("compileOut compileNext="));
  Serial.print(compileNext);
  Serial.print(F(" byte="));
  Serial.println(b);
  */
  codeSegment[compileNext++]=b;
}

void verifyAddress (Word addr) {
  if (addr & 0x8000 == 0) {
    // opcode
    setHasError();
    Serial.print(F("Invalid address (opcode space): "));
    Serial.println(addr);
    return;
  }
  if (addr & 0x4000) {
    // data segment
    addr=addr & 0x3FFF;
    if (addr >= dataNext) {
      setHasError();
      Serial.print(F("Invalid data segment address: "));
      Serial.print(addr);
      Serial.print(" HERE=");
      Serial.println(dataHere());
      return;
    }
  } else {
    // code segment
    addr=addr & 0x3FFF;
    if (addr >= CODE_SEGMENT_SIZE) {
      setHasError();
      Serial.print(F("Invalid code segment address: "));
      Serial.print(addr);
      Serial.print(F(" CODE_SEGMENT_SIZE="));
      Serial.println(CODE_SEGMENT_SIZE);
      return;
    }
  }
}

void writeByte (Word addr, byte b) {
  verifyAddress(addr);
  if (hasError) return;

  if (addr & 0x4000) {
    // data segment
    addr=addr & 0x3FFF;
    dataSegment[addr]=b;
  } else {
    // code segment
    addr=addr & 0x3FFF;
    codeSegment[addr] = b;
  }

}

byte readByte (Word addr) {
  verifyAddress(addr);
  if (hasError) return 0;

  if (addr & 0x4000) {
    // data segment
    addr=addr & 0x3FFF;
    return dataSegment[addr];
  } else {
    // code segment
    addr=addr & 0x3FFF;
    return codeSegment[addr];
  }

}

Word readWord (Word addr) {
  Word a=readByte(addr);
  Word b=readByte(addr+1);
  Word value = a << 8 | b;
  return value;
}

void writeWord (Word addr, Word value) {
  writeByte(addr, (value >> 8) & 0xFF);
  writeByte(addr+1, value & 0xFF);
}



char nextWord[MAX_WORD_LENGTH+1];  // input buffer

Word programCounter=0;
unsigned long instructionCount=0;

DictEntry *dictionaryHead=NULL; 

char temp[8];




void setup() {
  Serial.begin(9600);
  Serial.print(F_CPU / 1000000.0);
  Serial.println(F(" MHz"));
  Serial.println(F("Ok"));
}

void setHasError () {
  if (!hasError) {
    hasError=true;
    Serial.println();
    for (byte i=0; i<20; i++) Serial.print("-");
    Serial.println();
    Serial.println(F("Error"));
    for (byte i=0; i<20; i++) Serial.print("-");
    Serial.println();
    delay(1000);
    clearInputBuffer();
  }
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
  byte pos=0;

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


void push (Word v) {
  if (dStackNext >= DSTACK_SIZE-1) {
    setHasError();
    Serial.println(F("overflow data stack"));
    return;
  }
  dStack[dStackNext++]=v;
}

Word pop () {
  if (dStackNext==0) {
    setHasError();
    Serial.println(F("underflow data stack"));
    return 0;
  }
  return dStack[--dStackNext];
}

Word pick (Word n) {
  if (dStack-1-n < 0) {
    setHasError();
    Serial.println(F("underflow data stack"));
    return 0;
  }
  return dStack[dStackNext-1-n];
}

void rpush (Word v) {
  if (dStackNext >= RSTACK_SIZE-1) {
    setHasError();
    Serial.println(F("overflow return stack"));
    return;
  }
  rStack[rStackNext++]=v;
}

Word rpop () {
  if (rStackNext==0) {
    setHasError();
    Serial.println(F("underflow return stack"));

    return 0;
  }
  return rStack[--rStackNext];
}


// when an op requires additional bytes (bval and cval - push byte and cell value)
byte getOpcodeParameter() {
  return codeSegment[programCounter++];
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


DictEntry *dictLookupByAddr (Word addr) {
  DictEntry *ptr=dictionaryHead;
  while (ptr != NULL) {
    if (ptr->address==addr) return ptr;
    ptr=ptr->next;
  }
  return NULL;
}


void compileNumberByte (byte b) {
    compileOut(OP_BVAL);
    compileOut(b);
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
  programCounter=addr;
}


/*
* Compile nextWord, true if ok, false if error
*/
boolean compileNextWord () {
  // check for number
  int i=0;
  if (myAtoi(&i)) {
    Word w=(Word) i;
    compileNumber(i);
    return true;
  } 

  DictEntry *de=dictLookupNextWord();
  if (de != NULL) {
    if (de->type==DE_TYPE_NORMAL) {
      // ensure 14 bit address, then set high bit=1
      Word addr=(de->address & 0x3FFF) | 0x8000;
      compileOut((addr>>8) & 0xFF); 
      compileOut(addr & 0xFF);
    } else if (de->type==DE_TYPE_IMMEDIATE) {
      callForth(de->address);
    } else if (de->type==DE_TYPE_CONSTANT) {
      compileNumber(de->address);
    }
    return true;
  }

  int opcode=lookupOpCode();
  if (opcode >= 0) {
    compileOut(opcode);
    return true;
  }

  return false;
  
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
  byte tag;
  Word addr;
};


void op_reserved() {
  Serial.print(F("undefined"));
}


void op_colon() {  
  compileNext=codeNext;
  Word startPos=codeNext;

  Word tags[NUM_TAGS_REFS]; 
  struct Ref refs[NUM_TAGS_REFS];

  for (int i=0; i<NUM_TAGS_REFS; i++) {
    tags[i]=0;
  }
  
  byte nextRef=0;

  // create dictionary entry (CONSTANT address=0)
  create();

  // write dummy length byte (patched at semicolon)
  // (this enables disassembling)
  compileOut(99);

  for (;;) {
    if (hasError) return;

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
        tags[tag] = compileNext;
        continue;
      }
    }

    // tag lookup?
    if (*nextWord=='&') {
      int tag=0;
      if (getTagNumber(nextWord+1, &tag)) {

        compileOut(OP_CVAL);

        refs[nextRef].tag=tag;
        refs[nextRef].addr=compileNext;
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
        byte tag=refs[i].tag;
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

        codeSegment[patchAddr] = (tagAddr >> 8) & 0xFF;
        codeSegment[patchAddr+1] = tagAddr & 0xFF;
      }

      byte byteCount=(byte) (compileNext-startPos-1);  // length byte not included
      DictEntry *de=dictionaryHead;  // from call to create() 

      Serial.println();
      Serial.print(de->name);
      Serial.print(" ");
      Serial.print(byteCount);
      Serial.println(F(" bytes"));

      // patch length byte
      codeSegment[startPos]=byteCount;

      de->address=startPos+1;  // past length byte
      de->type=DE_TYPE_NORMAL;

      codeNext=compileNext;

      return;
    }

    // all other words except semicolon
    boolean ok = compileNextWord();
    if (!ok) {
      setHasError();
      Serial.print(F("Unknown word: "));
      Serial.println(nextWord);
      return;
    }
  }
}


void op_bval() {push(getOpcodeParameter());}
void op_cval() {push(getOpcodeParameter()<<8 | getOpcodeParameter());}

void op_dcall() {     // dynamic call
  Word addr=pop();
  callForth(addr);
}
void op_add() {Word b=pop(); Word a=pop(); push(a+b);}
void op_sub() {Word b=pop(); Word a=pop(); push(a-b);}
void op_mul() {Word b=pop(); Word a=pop(); push(a*b);}
void op_div() {Word b=pop(); Word a=pop(); push(a/b);}
void op_modulo() {Word b=pop(); Word a=pop(); push(a%b);}

void op_gt() {Word b=pop(); Word a=pop(); push(a>b ? 1 : 0);}
void op_ge() {Word b=pop(); Word a=pop(); push(a>=b ? 1 : 0);}
void op_lt() {Word b=pop(); Word a=pop(); push(a<b ? 1 : 0);}
void op_le() {Word b=pop(); Word a=pop(); push(a<=b ? 1 : 0);}
void op_eq() {Word b=pop(); Word a=pop(); push(a==b ? 1 : 0);}
void op_ne() {Word b=pop(); Word a=pop(); push(a!=b ? 1 : 0);}
void op_and() {Word b=pop(); Word a=pop(); push(a != 0 && b != 0 ? 1 : 0);}
void op_or() {Word b=pop(); Word a=pop(); push(a != 0 || b != 0 ? 1 : 0);}
void op_not() {Word x=pop(); push(x==0 ? 1 : 0);}



void op_cr() {Serial.println();}
void op_dot() {int i=(int) pop(); Serial.print(i); Serial.print(" ");}
void op_dot_u() {Word x=pop(); Serial.print(x); Serial.print(" ");}
void op_dot_hex() {Word x=pop(); Serial.print("0x"); Serial.print(x,16); Serial.print(" ");}
void op_dot_str() {Word addr=pop(); printStr(addr); }

void op_comp_out() {Word x=pop(); compileOut(x & 0xFF);}
void op_comp_next() {
  // convert to valid address, setting high bit to 1
  push(codeHere);
}
void op_HERE() {
  // convert into address with bit 15 set
  push(dataHere());
}

void op_allot() {
  Word count=pop();
  if (dataNext+count >= DATA_SEGMENT_SIZE) {
    setHasError();
    Serial.println(F("Data segment overflow"));
    return;
  }
  dataNext += count;
}

void op_constant() {
  Word value=pop();
  create();
  dictionaryHead->type=DE_TYPE_CONSTANT;
  dictionaryHead->address=value;
}

void op_variable() {
  Word variableAddr=dataHere();  
  dataNext += 2;
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
  if (dictionaryHead != NULL) dictionaryHead->type=DE_TYPE_IMMEDIATE;
}

void op_dup() {push(pick(0));}
void op_2dup() {push(pick(1)); push(pick(1));}
void op_drop() {pop();}
void op_over() {push(pick(1));}
void op_pick() {Word n=pop(); push(pick(n));}


void op_show_stack() {
  Serial.println();
  Serial.print(F("Stack: ["));
  for (byte i=0; i<dStackNext; i++) {
    if (i>0) Serial.print(" ");
    Serial.print(dStack[i]);
  }
  Serial.println(F("]"));
}
void op_clear_stack() {
  dStackNext=0;
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
  // followed by length field n, then n bytes
  // skip the data, and push the address of the length byte 
  Word lengthPointer = programCounter;
  byte length=getOpcodeParameter();
  programCounter += length;
  // convert to address by setting high bit
  push(lengthPointer | 0x8000);
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
    push(de->address);
  }
}


const OpCode opCodes[]={
  {"", &op_reserved}, 
  {":", &op_colon},

  {"bval", &op_bval},     // 2
  {"cval", &op_cval},     // 3
  {"ret", &op_ret},       // 4
  {"jmp", &op_jmp},       // 5
  {"jmp?", &op_cond_jmp}, // 6
  {"blob", &op_blob},     // 7
  
  {"dcall", &op_dcall},
  {"ret?", &op_cond_ret},

  {"create", &op_create}, 
  {"immediate", &op_immediate}, 
  {"+", &op_add},
  {"-", &op_sub},
  {"*", &op_mul},
  {"/", &op_div},
  {"%", &op_modulo},

  {">", &op_gt},
  {">=", &op_ge},
  {"<", &op_lt},
  {"<=", &op_le},
  {"==", &op_eq},
  {"!=", &op_ne},
  {"and", &op_and},
  {"or", &op_or},
  {"not", &op_not},

  {"cr", &op_cr},
  {".", &op_dot},
  {".u", &op_dot_u},
  {".hex", &op_dot_hex},
  {".str", &op_dot_str},

  {"comp.out", &op_comp_out},
  {"comp.next", &op_comp_next},
  {"HERE", &op_HERE},
  {"allot", &op_allot},
  {"constant", &op_constant},
  {"variable", &op_variable},
   
  {"!", &op_write},
  {"@", &op_read},

  {"b!", &op_writeb},
  {"b@", &op_readb},


  

  {"dup", &op_dup},
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
  {"dis", &op_dis},

  // end-marker
  {"",0}
};

void op_dis() {
  Word codeAddr = pop();
  byte len=codeSegment[codeAddr-1];
  Serial.println();
  Serial.print(F("length="));
  Serial.println(len);

  byte dataBytes=0;
  for (byte i=0; i<len; i++) {
    Word addr=codeAddr + i;
    Serial.print(addr);
    Serial.print("  ");

    byte op=codeSegment[addr];
    if (dataBytes > 0) {
      Serial.print("  ");
      Serial.println(op);
      dataBytes--;
      continue;
    }

    if (op & 0x80) {
      // high bit indicates two byte representation of 14 bit Forth call address
      dataBytes=1;
      Serial.print("  ");
      Serial.print(op);
      Serial.print(" ");
      Serial.print(F("(call)"));
      Word forthAddr=(op << 8) | codeSegment[addr+1];
      // strip away topmost 2 bits
      forthAddr=forthAddr & 0x3FFF;
      Serial.print(" ");
      Serial.print(forthAddr);

      DictEntry *de=dictLookupByAddr(forthAddr);
      Serial.print(" ");
      if (de==NULL) {
        Serial.print(F("unknown"));
        Serial.print(" ");
        Serial.println(F("address"));
      } else {
        Serial.print(F("=>"));
        Serial.print(" ");
        Serial.println(de->name);
      }
      continue;      
    }
  
    Serial.print(opCodes[op].name);
    if (op==OP_BVAL) {
      dataBytes=1;
    } else if (op==OP_CVAL) {
      dataBytes=2;
      Word val=(codeSegment[addr+1] << 8) | codeSegment[addr+2];
      Serial.print(" ");
      Serial.print(val);
    }
    Serial.println();
  }
}

// check nextWord against the opCodes array, return index or -1 if not found
int lookupOpCode () {
  byte pos=0;
  for (byte pos=0; pos < 255 ; pos++) {
    if (opCodes[pos].f == 0) return -1;  // end of list, not found
    if (!strcmp(nextWord,opCodes[pos].name)) return pos;
  }
  return -1;
}



void executeCode() {
  while (programCounter != 0) {
    if (hasError) {
      programCounter=0;
      return;     
    }

    byte b=codeSegment[programCounter++];

    // detect high bit set, this indicates a Forth call address (14 bits)

    if (b & 0x80) {
      // high bit indicates Forth word call (14 bits)
      Word address = (b<<8) | getOpcodeParameter();
      address = address & 0x3FF;  // strip two top bits
      callForth(address);
    } else {
      opCodes[b].f();
    }
    instructionCount++;
  }
}

// interpreting main loop
void loop() {
  hasError=false;

  executeCode();
  if (hasError) {
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

