/*
A quick and dirty Forth without the ACode mess, implementing instead
the colon compiler in C. Changing representation of numbers into an
opcode followed by 1 or 2 data bytes. Also changed addressing for Forth
words, into single byte, using the following logic:

When processing a code byte, if its upper bit is 1, strip it and use the 
rest as lookup in a table called definedWords, which in turn gives the 
address of the code. If upper bit is 0, it is an opCode, which we look up
in the opCodes table, and calls via function pointer. 

v0.0.1 has the compiler working, as well as the interpreter, and the runtime.codeSegment
  Implemented the ?W <word> to get the address of a word, and the dis which
  is a disassembler

OpCodes: zero bval cval ret + cr . dup >R R> ? jmp jmp? ?W dis

v0.0.1b added up to 5 tags &1 to &5 and up to 5 references to tags *1 to *5, to use with 
  jmp and jmp? to create loops and conditionals. Patches both forward and backward.

ex.

: test 1 
  &1 
  dup . 
  1 + 
  dup 30 > *2 jmp? 
  *1 jmp 
  &2 drop ;

Compiles to 21 bytes no IF, no LOOP.


Shortcuts? 
  DictEntry is a C struct.
  No support for dataSegment use
    - VARIABLE + allot?


*/

typedef struct {
  char *name;
  void (*f) ();
} OpCode;

typedef unsigned int Word;

#define DSTACK_SIZE     16
#define RSTACK_SIZE     16

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
#define OP_CALL  7


#define MAX_WORD_LENGTH   16


boolean errorCode = false;

byte codeSegment[200];
byte dataSegment[200];

Word codeNext=1;  // programCounter 0 means no code running (keeping it unsigned)
Word compileNext=1;

void compileOut (byte b) {
  codeSegment[compileNext++]=b;
}

Word dataSegmentNext=0;

char nextWord[MAX_WORD_LENGTH+1];  // input buffer

Word programCounter=0;

DictEntry *dictionaryHead=NULL; 

#define READC_ECHO  true

void setup() {
  Serial.begin(9600);
  Serial.println("Ok");
}

void error (char *category, char *detail) {
  Serial.print("Error ");
  Serial.print(category);
  Serial.print(": ");
  Serial.println(detail);
  errorCode=true;
  clearInputBuffer();
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
        if (READC_ECHO) Serial.println(" Ok");
      }  else {
        if (READC_ECHO) Serial.print((char) ch);
      }
      return (Word) ch;
    }
  }
}

void readNextWord () {
  byte pos=0;
  for(;;) {
    Word ch=readSerialChar();
    if (ch==13 || ch==10 || ch==32) {
      if (pos > 0) {
        nextWord[pos]='\0';
        return;
      } // else ignore
    } else {
      if (pos > MAX_WORD_LENGTH-1) {
        error("overflow","nextword");
        nextWord[pos]='\0';

        return;
      }
      nextWord[pos++]=ch;
    }
  }
}

void push (Word v) {
  if (dStackNext >= DSTACK_SIZE-1) {
    error("overflow","dStack");
    return;
  }
  dStack[dStackNext++]=v;
}

Word pop () {
  if (dStackNext==0) {
    error("underflow","dStack");
    return 0;
  }
  return dStack[--dStackNext];
}

void rpush (Word v) {
  if (dStackNext >= RSTACK_SIZE-1) {
    error("overflow","rStack");
    return;
  }
  rStack[rStackNext++]=v;
}

Word rpop () {
  if (rStackNext==0) {
    error("underflow","rStack");
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
* Compile nextWord, true if ok, false if error
*/
boolean compileNextWord () {
  if (!strcmp(nextWord,"0")) {
    compileNumber(0);
    return true;
  }
  int i=atoi(nextWord);  // can not match 0 with atoi, generating zero above
  if (i != 0) {
    compileNumber(i);
    return true;
  } 
  
  DictEntry *de=dictLookupNextWord();
  if (de != NULL) {
    if (de->type==DE_TYPE_NORMAL) {
      compileOut(OP_CALL);
      compileOut((de->address >> 8) & 0xFF);
      compileOut(de->address & 0xFF);
    } else if (de->type==DE_TYPE_IMMEDIATE) {
      callForthWord(de);
    } else if (de->type==DE_TYPE_CONSTANT) {
      compileNumber(de->address);
    }
    return true;
  }

  byte addr=lookupOpCode();
  if (addr == 0) {
    error("unknown",nextWord);
    return false;
  }
  compileOut(addr);
  return true;
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
// opcodes
// -------

struct Ref {
  byte tag;
  Word addr;
};

#define NUM_TAGS_REFS     5

void op_reserved() {
  Serial.print("undefined");
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
    readNextWord();

    // define tag?
    if (*nextWord=='*') {
      int i=atoi(nextWord+1);
      if (i > 0) {
        tags[i-1] = compileNext;
        continue;
      }
    }

    // look up tag?
    if (*nextWord=='&') {
      int i=atoi(nextWord+1);
      if (i>0) {

        compileOut(OP_CVAL);

        refs[nextRef].tag=i-1;
        refs[nextRef].addr=compileNext;

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
        Word patchAddr=refs[i].addr;
        codeSegment[patchAddr] = (tagAddr >> 8) & 0xFF;
        codeSegment[patchAddr+1] = tagAddr & 0xFF;
      }

      byte byteCount=(byte) (compileNext-startPos-1);  // length byte not included
      DictEntry *de=dictionaryHead;  // from call to create() 

      Serial.println();
      Serial.print(de->name);
      Serial.print(" ");
      Serial.print(byteCount);
      Serial.println(" bytes");

      // patch length byte
      codeSegment[startPos]=byteCount;

      de->address=startPos+1;  // past length byte
      de->type=DE_TYPE_NORMAL;

      codeNext=compileNext;

      return;
    }
    boolean ok = compileNextWord();
    if (!ok) return;  // error
  }
}


void op_bval() {push(getOpcodeParameter());}
void op_cval() {push(getOpcodeParameter()<<8 | getOpcodeParameter());}
void op_call() {
  Word addr=getOpcodeParameter()<<8 | getOpcodeParameter();
  DictEntry *de=dictLookupByAddr(addr);
  if (de==NULL) {
    error("unknown","address");
  } else {
    callForthWord(de);
  }
}
void op_dcall() {     // dynamic call
  Word addr=pop();
  DictEntry *de=dictLookupByAddr(addr);
  if (de==NULL) {
    error("unknown","address");
  } else {
    callForthWord(de);
  }
}
void op_add() {Word b=pop(); Word a=pop(); push(a+b);}
void op_sub() {Word b=pop(); Word a=pop(); push(a-b);}
void op_mul() {Word b=pop(); Word a=pop(); push(a*b);}
void op_div() {Word b=pop(); Word a=pop(); push(a/b);}

void op_gt() {Word b=pop(); Word a=pop(); push(a>b ? 1 : 0);}
void op_lt() {Word b=pop(); Word a=pop(); push(a<b ? 1 : 0);}
void op_eq() {Word b=pop(); Word a=pop(); push(a==b ? 1 : 0);}
void op_and() {Word b=pop(); Word a=pop(); push(a != 0 && b != 0 ? 1 : 0);}
void op_or() {Word b=pop(); Word a=pop(); push(a != 0 || b != 0 ? 1 : 0);}
void op_not() {Word x=pop(); push(x==0 ? 1 : 0);}

void op_cr() {Serial.println();}
void op_dot() {Word x=pop(); Serial.print(x); Serial.print(" ");}
void op_ret() {programCounter=rpop();}
void op_cond_ret() {Word cond=pop(); if (cond != 0) programCounter=rpop();}
void op_zret() {Word cond=pop(); if (cond==0) programCounter=rpop(); else push(cond);}

void op_create() {
  create();
}
void op_immediate() {
  if (dictionaryHead != NULL) dictionaryHead->type=DE_TYPE_IMMEDIATE;
}
void op_dup() {Word x=pop(); push(x); push(x);}
void op_drop() {pop();}

void op_words() {
  DictEntry *ptr=dictionaryHead;
  Serial.println();
  while (ptr != NULL) {
    Serial.print(ptr->name);
    Serial.print(" ");
    Serial.print(ptr->address);
    Serial.print(" ");
    if (ptr->type==DE_TYPE_IMMEDIATE) Serial.print("immediate");
    if (ptr->type==DE_TYPE_CONSTANT) Serial.print("constant");
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

void op_word_addr() {
  readNextWord();
  DictEntry *de=dictLookupNextWord();
  if (de==NULL) {
    push(0);
  } else {
    push(de->address);
  }
}

void op_key() {
  push(keyPressed() ? 1 : 0);
}

void op_readc() {
  push(readSerialChar());
}


const OpCode opCodes[]={
  {"", &op_reserved}, 
  {":", &op_colon},

  {"bval", &op_bval},     // 2
  {"cval", &op_cval},     // 3
  {"ret", &op_ret},       // 4
  {"jmp", &op_jmp},       // 5
  {"jmp?", &op_cond_jmp}, // 6
  {"call", &op_call},     // 7
  
  {"dcall", &op_dcall},
  {"ret?", &op_cond_ret},
  {"zret?", &op_zret},            // return on zero otherwise leave value on stack

  {"create", &op_create}, 
  {"immediate", &op_immediate}, 
  {"+", &op_add},
  {"-", &op_sub},
  {"*", &op_mul},
  {"/", &op_div},

  {">", &op_gt},
  {"<", &op_lt},
  {"==", &op_eq},
  {"and", &op_and},
  {"or", &op_or},
  {"not", &op_not},

  {"cr", &op_cr},
  {".", &op_dot},

  {"dup", &op_dup},
  {"drop", &op_drop},
  
  {">R", &op_to_r},
  {"R>", &op_r_from},
  {"?", &op_words},
  {"?W", &op_word_addr},
  {"key", &op_key},
  {"readc", &op_readc},


  {"dis", &op_dis},

  // end-marker
  {"",0}
};

void op_dis() {
  Word codeAddr = pop();
  byte len=codeSegment[codeAddr-1];
  Serial.println();
  Serial.print("length=");
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
  
    Serial.print(opCodes[op].name);
    if (op==OP_BVAL) {
      dataBytes=1;
    } else if (op==OP_CVAL) {
      dataBytes=2;
      Word val=(codeSegment[addr+1] << 8) | codeSegment[addr+2];
      Serial.print(" ");
      Serial.print(val);
    } else if (op==OP_CALL) {
      dataBytes=2;
      Word val=(codeSegment[addr+1] << 8) | codeSegment[addr+2];
      Serial.print(" ");
      Serial.print(val);

      DictEntry *de=dictLookupByAddr(val);
      Serial.print(" ");
      Serial.print(de->name);
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



void callForthWord (DictEntry *de) {
  if (de->type==DE_TYPE_CONSTANT) {
    push(de->address);
  } else {
    rpush(programCounter);
    programCounter=de->address;
  }
}

void executeInstruction (byte opcode) {
  opCodes[opcode].f();
}

void executeCode() {
  while (programCounter != 0) {
    byte b=codeSegment[programCounter++];
    executeInstruction(b);
  }
}

// interpreting main loop
void loop() {
  executeCode();

  readNextWord();
  if (!strcmp(nextWord,"0")) {
    push(0);
    return;
  }
  int i=atoi(nextWord);
  if (i != 0) {
    push(i);
    return;
  }

  DictEntry *de=dictLookupNextWord();
  if (de != NULL) {
    callForthWord(de);
  } else {
    int op = lookupOpCode();
    if (op<0) {
      error("unknown",nextWord);
    } else {
      // execute op
      opCodes[op].f();
    }
  }
}


