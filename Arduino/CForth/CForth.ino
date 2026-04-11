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
  Implemented the ' <word> to get the address of a word, and the dis which
  is a disassembler

v0.0.1b added up to 5 tags /0 to /4 and up to 5 references to tags &0 to &4, to use with 
  jmp and jmp? to create loops and conditionals. Patches both forward and backward.

2026-04-11
----------
Added proper integer parsing, supporting signed hex, binary and decimal
  0xCAFE 
  b10010
Fixed various bugs.

Code of the day:
----------------
: prime (n -- bool)
	dup
	(n) 2 /		(counter value, counting down)
	/1
		2dup % 
			0 == &2 jmp?
		(n count) 1 -
			dup 1 == &3 jmp?
		&1 jmp
	/2 drop drop 0 ret  (divisor found)
	/3 drop drop 1  (count reach zero - diviser not found => prime)
;
		
(speed test: 106543 ops/second on 20MHz Arduino Every Nano)

: test
	[test 31013 prime drop test]
;
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


boolean hasError = false;

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
unsigned long instructionCount=0;

DictEntry *dictionaryHead=NULL; 

#define READC_ECHO  true

void setup() {
  Serial.begin(9600);
  Serial.print(F_CPU / 1000000.0);
  Serial.println(" MHz");
  Serial.println("Ok");
}

void error (char *category, char *detail) {
  hasError=true;
  Serial.println();
  for (byte i=0; i<20; i++) Serial.print("-");
  Serial.println();
  Serial.print("Error: ");
  Serial.print(category);
  Serial.print(" ");
  Serial.println(detail);
  for (byte i=0; i<20; i++) Serial.print("-");
  Serial.println();
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

Word pick (Word n) {
  if (dStack-1-n < 0) {
    error("underflow","dStack");
    return 0;
  }
  return dStack[dStackNext-1-n];
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


/*
* Compile nextWord, true if ok, false if error
*/
boolean compileNextWord () {

  // check for number (TODO: replace atoi - its an ugly hack)
  int i=0;
  if (myAtoi(&i)) {
    Word w=(Word) i;
    compileNumber(i);
    return true;
  } 




  if (!strcmp(nextWord,"0")) {
    compileNumber(0);
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
      error("unknown","tag");
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
          error("null","tag");
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
      Serial.println(" bytes");

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
      error("unknown", nextWord);
      return;
    }
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
  Serial.print("[");
  for (byte i=0; i<dStackNext; i++) {
    if (i>0) Serial.print(" ");
    Serial.print(dStack[i]);
  }
  Serial.println("]");
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
  Serial.print("op/s ");
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

void executeCode() {
  while (programCounter != 0) {
    if (hasError) {
      programCounter=0;
      return;     
    }

    byte b=codeSegment[programCounter++];
    opCodes[b].f();
    instructionCount++;
  }
}

// interpreting main loop
void loop() {
  hasError=false;

  executeCode();
  if (hasError) return;

  readNextWord();

  int i=0;
  if (myAtoi(&i)) {
    push ((Word) i);
    return;
  }

  DictEntry *de=dictLookupNextWord();
  if (de != NULL) {
    callForthWord(de);
    return;
  }
  
  int op = lookupOpCode();
  if (op>=0) {
    opCodes[op].f();
    return;
  }
  error("unknown",nextWord);
}


