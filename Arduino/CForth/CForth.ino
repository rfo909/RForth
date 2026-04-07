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

OpCodes: zero bval cval ret + cr . dup >R R> ? jmp jmp? ?W

Shortcuts? 
  DictEntry is a C struct.
  No support for dataSegment use
    - VARIABLE + allot?

Example output
--------------  
: a 1 2 3 + + ; Ok

; 9 bytes
: b a a + ; Ok

; 4 bytes
b . Ok
12 

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

typedef struct DictEntry {
  char *name;
  byte type;
  byte address;   // subtract OPCODE_COUNT to get index into definedWords array of actual addresses

  struct DictEntry *next;
} DictEntry;


#define OP_ZERO  2
#define OP_BVAL  3
#define OP_CVAL  4
#define OP_RET   5
#define OP_JMP   6
#define OP_COND_JMP 7

#define DE_TYPE_NORMAL 1
#define DE_TYPE_IMMEDIATE 2
#define MAX_WORD_LENGTH   16

#define OPCODE_COUNT    128
    // 0-127 = 0nnn_nnnn bytes
    // 128-255 = 1nnn_nnnn bytes => point to Forth words via definedWords array



byte codeSegment[200];
byte dataSegment[200];

Word codeSegmentNext=1;  // programCounter 0 means no code running
Word dataSegmentNext=0;

char nextWord[MAX_WORD_LENGTH+1];

Word definedWords[255-OPCODE_COUNT]; 
  // addressed by position in this array, with offset of OPCODE_COUNT, so
  // first defined word has single byte address at 0 in this array, but
  // public byte address of OPCODE_COUNT.

byte nextDefinedWord=0;

Word programCounter=0;

DictEntry *dictionaryHead=NULL; 

#define READC_ECHO  true

void setup() {
  Serial.begin(9600);
  Serial.println("Ok");
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
        nextWord[pos++]='\0';
        return;
      } // else ignore
    } else {
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




// -------
// opcodes
// -------

void op_undefined() {Serial.print("XX");}

void op_colon() {  
  Word currPos=codeSegmentNext;

  // name of word
  readNextWord();
  char *ptr=(char *) malloc(strlen(nextWord)+1);
  strcpy(ptr,nextWord);

  // write dummy length byte (patched at semicolon)
  // (this enables disassembling)
  codeSegment[currPos++]=0;

  for (;;) {
    readNextWord();
    
    if (!strcmp(nextWord,";")) {
      codeSegment[currPos++]=OP_RET;

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
      de->address=OPCODE_COUNT + nextDefinedWord; // single byte
      definedWords[nextDefinedWord]=codeSegmentNext+1; // past length byte
      nextDefinedWord++;

      dictionaryHead=de;
      codeSegmentNext=currPos;

      return;
    }
    if (!strcmp(nextWord,"0")) {
      codeSegment[currPos++] = OP_ZERO;
      continue;
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
      continue;
    } 
    
    DictEntry *de=dictLookupNextWord();
    if (de != NULL) {
      if (de->type==DE_TYPE_NORMAL) {
        codeSegment[currPos++] = de->address;
      } else if (de->type==DE_TYPE_IMMEDIATE) {
        executeInstruction(de->address);
      }
      continue;
    }

    byte addr=assembleNextWord();
    if (addr == 0) {
      Serial.print("Unknown: ");
      Serial.println(nextWord);
      return;
    } else {
      codeSegment[currPos++] = addr;
    }
  }
}


void op_zero() {push(0);}
void op_bval() {push(getOpcodeParameter());}
void op_cval() {push(getOpcodeParameter()<<8 | getOpcodeParameter());}
void op_add() {Word b=pop(); Word a=pop(); push(a+b);}
void op_cr() {Serial.println();}
void op_dot() {Word x=pop(); Serial.print(x); Serial.print(" ");}
void op_ret() {programCounter=rpop();}
void op_immediate() {
  if (dictionaryHead != NULL) dictionaryHead->type=DE_TYPE_IMMEDIATE;
}
void op_dup() {Word x=pop(); push(x); push(x);}
void op_words() {
  DictEntry *ptr=dictionaryHead;
  Serial.println();
  while (ptr != NULL) {
    Serial.print(ptr->name);
    Serial.print(" ");
    Serial.println(ptr->address);
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
    push(definedWords[de->address]);
  }
}

const OpCode opCodes[]={
  {"", &op_undefined}, 
  {":", &op_colon},

  {"zero", &op_zero},     // 2
  {"bval", &op_bval},     // 3
  {"cval", &op_cval},     // 4
  {"ret", &op_ret},       // 5
  {"jmp", &op_jmp},       // 6
  {"jmp?", &op_cond_jmp},       // 7
  
  {"immediate", &op_immediate}, 
  {"+", &op_add},
  {"cr", &op_cr},
  {".", &op_dot},
  {"dup", &op_dup},
  {">R", &op_to_r},
  {"R>", &op_r_from},
  {"?", &op_words},
  {"?W", &op_word_addr},

  // end-marker
  {"",0}
};



byte assembleNextWord () {
  byte pos=0;
  for (byte pos=0; true ; pos++) {
    if (opCodes[pos].f == 0) return 0;
    if (pos >= OPCODE_COUNT) {
      Serial.println("OPCODE_COUNT");
    }
    if (!strcmp(nextWord,opCodes[pos].name)) return pos;
  }
  return 0;
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

  readNextWord();
  int i=atoi(nextWord);
  if (i != 0) {
    push(i);
  } else {
    DictEntry *de=dictLookupNextWord();
    if (de != NULL) {
      executeInstruction(de->address);
    } else {
      byte op = assembleNextWord();
      if (op==0) {
        Serial.print("Unknown: ");
        Serial.println(nextWord);
      } else {
        // execute op
        opCodes[op].f();
      }
    }
  }
}


