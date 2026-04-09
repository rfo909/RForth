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

v0.0.1b added up to 10 tags &1 to &9 and up to 5 references to tag *1 to *9, to use with 
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

Word dStack[16];
byte dStackNext=0;
Word rStack[16];
byte rStackNext=0;

#define DE_TYPE_NORMAL 1
#define DE_TYPE_IMMEDIATE 2

typedef struct DictEntry {
  char *name;
  byte type;
  byte address;

  struct DictEntry *next;
} DictEntry;


#define OP_ZERO  2
#define OP_BVAL  3
#define OP_CVAL  4
#define OP_RET   5
#define OP_JMP   6
#define OP_COND_JMP 7


#define MAX_WORD_LENGTH   16


Word errorCode = 0;

byte codeSegment[200];
byte dataSegment[200];

Word codeSegmentNext=1;  // programCounter 0 means no code running (keeping it unsigned)
Word dataSegmentNext=0;

char nextWord[MAX_WORD_LENGTH+1];  // input buffer

Word definedWords[127]; 
  // map single bytes with upper bit stripped to Word addresses for compiled Forth words, 
  // stored in the codeSegment


byte nextDefinedWord=0;

Word programCounter=0;

DictEntry *dictionaryHead=NULL; 

#define READC_ECHO  true

void setup() {
  Serial.begin(9600);
  Serial.println("Ok");
}

void error (Word code, char *msg) {
  Serial.print("Error ");
  Serial.print(code);
  Serial.print(" ");
  Serial.println(msg);
  errorCode=code;
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
        error(1,"Input word too long");
        nextWord[pos]='\0';

        return;
      }
      nextWord[pos++]=ch;
    }
  }
}

void push (Word v) {
  //Serial.print("Push: ");
  //Serial.println(v);
  dStack[dStackNext++]=v;
}

Word pop () {
  Word value = dStack[--dStackNext];
  //Serial.print("Pop: ");
  //Serial.println(value);
  return value;
}

void rpush (Word v) {
  //Serial.print("Push: ");
  //Serial.println(v);
  rStack[rStackNext++]=v;
}

Word rpop () {
  Word value = rStack[--rStackNext];
  //Serial.print("Pop: ");
  //Serial.println(value);
  return value;
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


DictEntry *dictLookupByAddr (byte addr) {
  DictEntry *ptr=dictionaryHead;
  while (ptr != NULL) {
    if (ptr->address==addr) return ptr;
    ptr=ptr->next;
  }
  return NULL;
}

/*
* Compile nextWord, return currPos or -1 if error
*/
int compileNextWord (int currPos) {
  if (!strcmp(nextWord,"0")) {
    codeSegment[currPos++] = OP_ZERO;
    return currPos;
  }
  int i=atoi(nextWord);  // can not match 0 with atoi, generating OP_ZERO above
  if (i != 0) {
    if (i < 255) {
      codeSegment[currPos++]=OP_BVAL;
      codeSegment[currPos++]=i;
    } else {
      codeSegment[currPos++]=OP_CVAL;
      codeSegment[currPos++]=(i>>8) & 0xFF;
      codeSegment[currPos++]=i & 0xFF;
    }
    return currPos;
  } 
  
  DictEntry *de=dictLookupNextWord();
  if (de != NULL) {
    if (de->type==DE_TYPE_NORMAL) {
      codeSegment[currPos++] = de->address;
    } else if (de->type==DE_TYPE_IMMEDIATE) {
      executeInstruction(de->address);
    }
    return currPos;
  }

  byte addr=assembleNextWord();
  if (addr == 0) {
    Serial.print("Unknown: ");
    Serial.println(nextWord);
    return 0;
  }
  codeSegment[currPos++] = addr;
  return currPos;
}

// -------
// opcodes
// -------

void op_reserved() {Serial.print("Undefined");}

struct Ref {
  byte tag;
  Word addr;
};

#define NUM_TAGS_REFS     5

void op_colon() {  
  Word currPos=codeSegmentNext;

  Word tags[NUM_TAGS_REFS]; 
  struct Ref refs[NUM_TAGS_REFS];

  for (int i=0; i<NUM_TAGS_REFS; i++) {
    tags[i]=0;
  }
  
  byte nextRef=0;

  // name of word
  readNextWord();
  char *ptr=(char *) malloc(strlen(nextWord)+1);
  strcpy(ptr,nextWord);

  // write dummy length byte (patched at semicolon)
  // (this enables disassembling)
  codeSegment[currPos++]=99;

  for (;;) {
    readNextWord();

    if (*nextWord=='&') {
      int i=atoi(nextWord+1);
      if (i > 0) {
        tags[i-1] = currPos;
        continue;
      }
    }

    if (*nextWord=='*') {
      int i=atoi(nextWord+1);
      if (i>0) {
        codeSegment[currPos++]=OP_CVAL;
        refs[nextRef].tag=i-1;
        refs[nextRef].addr=currPos;

        // place holder to be patched
        codeSegment[currPos++]=0;
        codeSegment[currPos++]=0;
        
        nextRef++;
        continue;
      }
    }

    if (!strcmp(nextWord,";")) {
      codeSegment[currPos++]=OP_RET;

      // patch tag references
      for (int i=0; i<nextRef; i++) {
        byte tag=refs[i].tag;
        Word tagAddr=tags[tag];
        Word patchAddr=refs[i].addr;
        codeSegment[patchAddr] = (tagAddr >> 8) & 0xFF;
        codeSegment[patchAddr+1] = tagAddr & 0xFF;
      }

      DictEntry *de=(DictEntry *) malloc(sizeof(DictEntry));
      de->name=ptr;
      de->type=DE_TYPE_NORMAL;
      de->next=dictionaryHead;

      Word byteCount=currPos-codeSegmentNext-1;  // length byte not included

      Serial.println();
      Serial.print(ptr);
      Serial.print(" ");
      Serial.print(byteCount);
      Serial.println(" bytes");

      // patch length byte
      codeSegment[codeSegmentNext] = byteCount;

      // using byte-indexed indirection array, for custom words, 
      // containing the actual address as a Word
      de->address=nextDefinedWord | 0x80; // set high bit
      definedWords[nextDefinedWord]=codeSegmentNext+1; // past length byte
      nextDefinedWord++;

      dictionaryHead=de;
      codeSegmentNext=currPos;

      return;
    }
    int x = compileNextWord(currPos);
    if (x<0) return;  // error
    else currPos=(Word) x;

  }
}


void op_zero() {push(0);}
void op_bval() {push(getOpcodeParameter());}
void op_cval() {push(getOpcodeParameter()<<8 | getOpcodeParameter());}

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
    Serial.println(definedWords[ptr->address & 0x7F]);
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
  if (de==0) {
    push(0);
  } else {
    push(definedWords[de->address  & 0x7F]);
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

  {"zero", &op_zero},     // 2
  {"bval", &op_bval},     // 3
  {"cval", &op_cval},     // 4
  {"ret", &op_ret},       // 5
  {"jmp", &op_jmp},       // 6
  {"jmp?", &op_cond_jmp},       // 7
  
  {"ret?", &op_cond_ret},
  {"zret?", &op_zret},            // return on zero otherwise leave value on stack
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
    if (op & 0x80) {
      // scan dictionary to get name
      DictEntry *de = dictLookupByAddr(op);
      if (de==NULL) {
        Serial.println("<NULL>");
      } else {
        Serial.println(de->name);
      }
    } else {
      Serial.println(opCodes[op].name);
      if (op==OP_BVAL) {
        dataBytes=1;
      } else if (op==OP_CVAL) {
        dataBytes=2;
        Word val=(codeSegment[addr+1] << 8) | codeSegment[addr+2];
        Serial.print("  (");
        Serial.print(val);
        Serial.println(")");
      }
    }
  }
}

// check nextWord against the opCodes array, return index or -1 if not found
int assembleNextWord () {
  byte pos=0;
  for (byte pos=0; pos < 0x80 ; pos++) {
    if (opCodes[pos].f == 0) return 0;
    if (!strcmp(nextWord,opCodes[pos].name)) return pos;
  }
  return -1;
}



void callForthWord (Word codeAddress) {
  rpush(programCounter);
  programCounter=codeAddress;
}

void executeInstruction (byte addr) {
  if (addr & 0x80) {
    // high bit 1 means Forth word
    Word codeAddress=definedWords[addr & 0x7F];
    callForthWord(codeAddress);
  } else {
    // high bit 0 means opcode
    opCodes[addr].f();
  }
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

  /*
  // debug  show codeSegment
  Serial.print("code: ");
  for (int i=0; i<codeSegmentNext; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(codeSegment[i]);
    Serial.println();
  }
  Serial.println();
  */
  readNextWord();
  int i=atoi(nextWord);
  if (i != 0) {
    push(i);
  } else {
    DictEntry *de=dictLookupNextWord();
    if (de != NULL) {
      executeInstruction(de->address);
    } else {
      int op = assembleNextWord();
      if (op<0) {
        Serial.print("Unknown: ");
        Serial.println(nextWord);
      } else {
        // execute op
        opCodes[op].f();
      }
    }
  }
}


