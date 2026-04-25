#include "Constants.h"


Byte codeSegment[CODE_SEGMENT_SIZE];
Byte dataSegment[DATA_SEGMENT_SIZE];

// PROGMEM data later
Byte flashCode[]={1,2,3};
const Word flashCodeLength=3;

#define CODE_ADDRESS(x) (x)
#define DATA_ADDRESS(x) ((x) | DATA_BIT)

// ROREG = readonly-register

#define ROREG_DSTACK_SIZE       DATA_ADDRESS(0)
#define ROREG_RSTACK_SIZE       DATA_ADDRESS(2)
#define ROREG_XSTACK_SIZE       DATA_ADDRESS(4)

#define ROREG_STATIC_COUNT      DATA_ADDRESS(6)

#define ROREG_DSTACK_BASE       DATA_ADDRESS(8)
#define ROREG_RSTACK_BASE       DATA_ADDRESS(10)
#define ROREG_XSTACK_BASE       DATA_ADDRESS(12)
#define ROREG_NEXTWORD_BASE     DATA_ADDRESS(14)
#define ROREG_NEXTWORD_LENGTH   DATA_ADDRESS(16)

#define REG_DICT_PTR            DATA_ADDRESS(18)

#define REG_CODE_NEXT           DATA_ADDRESS(20)
#define REG_DATA_NEXT           DATA_ADDRESS(22)

#define REG_DSTACK_NEXT         DATA_ADDRESS(24)
#define REG_RSTACK_NEXT         DATA_ADDRESS(26)
#define REG_XSTACK_NEXT         DATA_ADDRESS(28)

#define REGISTER_COUNT    15

#define INVALID_BYTE 0xFF
#define INVALID_WORD 0xFFFF

Boolean errorFlag = false;

void setHasError() {
  errorFlag=true;
}

Boolean hasError() {
  return errorFlag;
}

void clearHasError() {
  if (errorFlag) {
    delay(2000);
    clearInputBuffer();
  }
  errorFlag=false;
}

void clearInputBuffer() {
  while (Serial.available()) Serial.read();
}

// Must not point to flash code!
Byte *refToPointer (Word addr) {
  if (addr & DATA_BIT) {
    return &(dataSegment[addr & DATA_MASK]);
  } else {
    return &(codeSegment[addr-flashCodeLength]);
  }
}

Byte readFlashCodeByte (Word addr) {
  if (addr >= flashCodeLength) {
    setHasError();
    Serial.print(F("readFlashByte invalid addr "));
    Serial.println(addr);
    return INVALID_BYTE;
  }
  return flashCode[addr];
}

Byte readByte (Word addr) {
  if (addr & DATA_BIT) {
    addr=addr & DATA_MASK;
    if (addr >= DATA_SEGMENT_SIZE) {
      setHasError();
      Serial.print(F("readByte: Data segment overflow: "));
      Serial.println(addr);
      return INVALID_BYTE;
    }
    return dataSegment[addr];
  } else {
    if (addr < flashCodeLength) {
      return readFlashCodeByte(addr);
    } 
    addr=addr-flashCodeLength;
    if (addr >= CODE_SEGMENT_SIZE) {
      setHasError();
      Serial.print(F("readByte: Code segment overflow "));
      Serial.println(addr);
      return INVALID_BYTE;
    }
    return codeSegment[addr];
  }
}

void writeByte (Word addr, Byte val) {
  if (addr & DATA_BIT) {
    addr=addr & DATA_MASK;
    if (addr >= DATA_SEGMENT_SIZE) {
      setHasError();
      Serial.print(F("writeByte: Data segment overflow: "));
      Serial.println(addr);
      return;
    }
    dataSegment[addr]=val;
  } else {
    if (addr < flashCodeLength) {
      setHasError();
      Serial.print(F("writeByte: Code segment address in flash: "));
      Serial.println(addr);
      return;
    } 
    addr=addr-flashCodeLength;
    if (addr >= CODE_SEGMENT_SIZE) {
      setHasError();
      Serial.print(F("writeByte: Code segment overflow: "));
      Serial.println(addr);
      return;
    }
    codeSegment[addr]=val;
  }
}

Word readWord (Word addr) {
  Byte a=readByte(addr);
  if (hasError()) return INVALID_WORD;
  Byte b=readByte(addr+1);
  if (hasError()) return INVALID_WORD;
  return (a<<8) | b;
}

void writeWord (Word addr, Word val) {
  Byte a=(val >> 8) & 0xFF;
  Byte b=val & 0xFF;
  writeByte(addr,a);
  if (hasError()) return;
  writeByte(addr+1,b);
}

// -----------------------------
// Some very basic functions

Word HERE() {
  return readWord(REG_DATA_NEXT);
}

Word codeHERE() {
  return readWord(REG_CODE_NEXT);
}

void allot (Word count) {
  Word here=HERE();
  //Serial.print("allot: HERE=");
  //Serial.println(here);
  Word index=(here & DATA_MASK) + count;
  if (index >= DATA_SEGMENT_SIZE) {
    setHasError();
    Serial.println(F("allot: out of data space"));
    return;
  }
  writeWord(REG_DATA_NEXT, here+count);
}

void codeAllot (Word count) {
  Word here=codeHERE();
  Word index=here-flashCodeLength+count;
  if (index >= CODE_SEGMENT_SIZE) {
    setHasError();
    Serial.println(F("codeAllot: out of code space"));
    return;
  }
  Serial.print("codeAllot: codeHERE=");
  Serial.print(here);
  Serial.print(" count=");
  Serial.println(count);

  writeWord(REG_CODE_NEXT, here+count);
}

void codeOutByte (Byte b) {
  Word w=codeHERE();
  if (w-flashCodeLength + 1 >= CODE_SEGMENT_SIZE) {
    setHasError();
    Serial.println(F("codeOut: out of code space"));
    return;
  }
}

void codeOutWord (Word w) {
  codeOutByte((w>>8) & 0xFF);
  if (hasError()) return;
  codeOutByte(w & 0xFF);
}


// -------------------------
// Util

/*
Enhanced atoi, recognizes decimal, hex (0x...) and binary (b....) and negative, returns boolean to 
indicate success or failure. Writes result int *target
*/
Boolean myAtoi (char *nextWord, int *target) {
  int readPos=0;
  
  Boolean negative=false;
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


// -------------------------
// Stack operations

void push (Word baseAddr, Word sizeAddr, Word nextAddr, Word value) {
  Word addr=readWord(baseAddr);
  Word size=readWord(sizeAddr);
  Word next=readWord(nextAddr);

  if (next-addr+1 >= size) {
    setHasError();
    return;
  }
  
  writeWord(next,value);
  writeWord(nextAddr,next+CELLSIZE);
}

Word pop (Word baseAddr, Word nextAddr) {
  Word addr=readWord(baseAddr);
  Word next=readWord(nextAddr);
  
  if (next-addr < CELLSIZE) {
    setHasError();
    return INVALID_WORD;
  }
  next -= CELLSIZE;
  writeWord(nextAddr, next);
  return readWord(next);
}



void dPush (Word value) {
  push(ROREG_DSTACK_BASE, ROREG_DSTACK_SIZE, REG_DSTACK_NEXT, value);
  if (hasError()) {
    Serial.println(F("dPush overflow"));
  }
}


Word dPop () {
  Word value=pop(ROREG_DSTACK_BASE, REG_DSTACK_NEXT);
  if (hasError()) {
    Serial.println(F("dPop underflow"));
    return INVALID_WORD;
  }
  return value;
}

void rPush (Word value) {
  push(ROREG_RSTACK_BASE, ROREG_RSTACK_SIZE, REG_RSTACK_NEXT, value);
  if (hasError()) {
    Serial.println(F("rPush overflow"));
  }
}


Word rPop () {
  Word value=pop(ROREG_RSTACK_BASE, REG_RSTACK_NEXT);
  if (hasError()) {
    Serial.println(F("rPop underflow"));
    return INVALID_WORD;
  }
  return value;
}


// -----------------------------
// Memory initialization (data segment) 

void memInit() {
  Word i;
  for (i=0; i<DATA_SEGMENT_SIZE; i++) {
    dataSegment[i]=0;
  }
  for (i=0; i<CODE_SEGMENT_SIZE; i++) {
    codeSegment[i]=0;
  }

  // set data next in order to use allot() and HERE()
  writeWord(REG_DATA_NEXT, DATA_ADDRESS(0));
  
  // allocate room for the registers
  allot(REGISTER_COUNT * CELLSIZE);

  writeWord(ROREG_DSTACK_SIZE, DSTACK_SIZE);
  writeWord(ROREG_RSTACK_SIZE, RSTACK_SIZE);
  writeWord(ROREG_XSTACK_SIZE, XSTACK_SIZE);

  writeWord(ROREG_STATIC_COUNT, flashCodeLength);

  // create stacks 
  writeWord(ROREG_DSTACK_BASE, HERE());
  allot(DSTACK_SIZE * CELLSIZE);

  writeWord(ROREG_RSTACK_BASE, HERE());
  allot(RSTACK_SIZE * CELLSIZE);

  writeWord(ROREG_XSTACK_BASE, HERE());
  allot(XSTACK_SIZE * DOUBLESIZE);

  // allocate nextWord buffer
  writeWord(ROREG_NEXTWORD_BASE, HERE());
  writeWord(ROREG_NEXTWORD_LENGTH, MAX_WORD_LENGTH); 
  
  allot(MAX_WORD_LENGTH+2);   // both counted and null-terminated!
  writeByte(readWord(ROREG_NEXTWORD_BASE), 0); // set length field to 0
  writeByte(readWord(ROREG_NEXTWORD_BASE+1), 0); // and terminate it C style
  
  // set next pointers for stacks to base
  writeWord(REG_DSTACK_NEXT, readWord(ROREG_DSTACK_BASE));
  writeWord(REG_RSTACK_NEXT, readWord(ROREG_RSTACK_BASE));
  writeWord(REG_XSTACK_NEXT, readWord(ROREG_XSTACK_BASE));

  // set data segment pointers
  // (HERE was set initially)
  writeWord(REG_CODE_NEXT, CODE_ADDRESS(flashCodeLength));

  Serial.print(F("memInit dataSegment used: "));
  Serial.println(HERE()-DATA_ADDRESS(0));
}


// ---------------------
// ops = words written in C

void op_add () {Serial.println("add");Word b=dPop(); Word a=dPop(); dPush(a+b);}
void op_sub () {Word b=dPop(); Word a=dPop(); dPush(a-b);}
void op_mul () {Word b=dPop(); Word a=dPop(); dPush(a*b);}
void op_div () {Word b=dPop(); Word a=dPop(); dPush(a/b);}
void op_lshift () {Word b=dPop(); Word a=dPop(); dPush(a<<b);}
void op_rshift () {Word b=dPop(); Word a=dPop(); dPush(a>>b);}

void op_seq () {   // string equal?
  Word b=dPop();
  Word a=dPop();
  Byte len=readByte(a);
  for (Byte i=0; i<len+1; i++) {
    if (readByte(a+i) != readByte(b+i)) {
      dPush(0); // mismatch, includes length field
    }
  }
  dPush(1);
}

void op_key_pressed() {
  if (Serial.available()) {
    dPush(1);
  } else {
    dPush(0);
  }
}

void op_dot () {
  Serial.print(dPop());
  Serial.print(" ");
}




// ---------------------------
// Execution token functions

void XT_C_Call (Word paramField) {
  Serial.println("XT_C_Call");
  OP f=readWord(paramField);
  f();
}



// -----------------------------
// Dictionary initalialization

/*
The dictionary entry consists of 
  
  - previousPointer (Word)
  - nameReference (Word)
  - flags (Word)
  - codePointer (XT) (Word)
  
  + parameter field 

The codePointer points to an XT function (in C), and is given a single
parameter, which is a Word pointing to the memory following the 
codePointer (in the dictionary entry), called the parameter field, which
may be used or ignored by the XT in question.
*/


void dictAddOp(Word name, Word op) {
  Word dictEntry=codeHERE();
  codeAllot(10);

  writeWord(dictEntry,readWord(REG_DICT_PTR));   // previousPointer
  writeWord(dictEntry+2, name); // name
  writeWord(dictEntry+4,0); // flags
  writeWord(dictEntry+6, (Word) &XT_C_Call);  // XT
  // parameter field = pointer to C function
  writeWord(dictEntry+8, op);
  writeWord(REG_DICT_PTR,dictEntry);  // link into dictionary
}

void dictAddOp1 (char c1, Word op) {
  Word name=codeHERE(); 
  codeAllot(2);
  writeByte(name,1);
  writeByte(name+1,c1);
  dictAddOp(name,op);
}

void dictAddOp2 (char c1, char c2, Word op) {
  Word name=codeHERE(); 
  codeAllot(3);
  writeByte(name,1);
  writeByte(name+1,c1);
  writeByte(name+2,c2);
  dictAddOp(name,op);
}

void dictAddOp3 (char c1, char c2, char c3, Word op) {
  Word name=codeHERE(); 
  codeAllot(4);
  writeByte(name,1);
  writeByte(name+1,c1);
  writeByte(name+2,c2);
  writeByte(name+3,c3);
  dictAddOp(name,op);
}

void dictAddOp4 (char c1, char c2, char c3, char c4, Word op) {
  Word name=codeHERE(); 
  codeAllot(5);
  writeByte(name,1);
  writeByte(name+1,c1);
  writeByte(name+2,c2);
  writeByte(name+3,c3);
  writeByte(name+4,c3);
  dictAddOp(name,op);
}

// ---------------------------
// Call execution token function

void callXT (Word dictEntry) {
  XT xt=readWord(dictEntry+6);
  Word paramField=dictEntry+8;
  xt(paramField);
}


void printWord (Word ref) {
  Word length=readByte(ref);
  for (int i=0; i<length; i++) {
    char c=readByte(ref+1+i);
    Serial.print(c);
  }
}

void cr () {
  Serial.println();
}

void printNextWord() {
  Word nextWord=readWord(ROREG_NEXTWORD_BASE);
  printWord(nextWord);
}

void words() {
  Word ref=readWord(REG_DICT_PTR);
  while (ref != 0) {
    if (hasError()) return;
    Word deName=ref+2;
    printWord(deName);
    Serial.print(" ");
    ref=readWord(ref); // prev-pointer
  }
}

void dump () {
  Word addr=dPop();
  for (Word i=0; i<20; i++) {
    Serial.print(addr+i);
    Serial.print(" ");
    Serial.println(readByte(addr+i));
  }
}


// Look up in dictionary, return ref to XT
void executeNextWord () {
  Serial.println(F("executeNextWord"));

  Word nextWord=readWord(ROREG_NEXTWORD_BASE);
  char *cstring=refToPointer(nextWord+1);
  int intValue=0;
  if (myAtoi(cstring, &intValue)) {
    dPush((Word) intValue);
    return;
  }

  Word ref=readWord(REG_DICT_PTR);
  while (ref != 0) {
    Word deName=ref+2;
    Serial.print("Checking: ");
    printWord(deName);
    cr();
    dPush(nextWord);
    dPush(ref+2); // name field
    op_seq();
    if (dPop()) {
      // match
      callXT(ref);
      return;
    }
    ref=readWord(ref); // prev-pointer
  }
  setHasError();
  Serial.println(F("Unknown word")); 
}

void dictionaryInit() {
  dictAddOp1('+', &op_add);
  dictAddOp4('d','u','m','p', &dump);
/*
  dictAddOp1('-', &op_sub);
  dictAddOp1('*', &op_mul);
  dictAddOp1('/', &op_div);
  dictAddOp1('.', &op_dot);
  dictAddOp1('?', &words);

  dictAddOp2('<','<', &op_lshift);
  dictAddOp2('>','>', &op_rshift);

  dictAddOp3('s','e','q', &op_seq);

  dictAddOp4('k','e','y','?', &op_key_pressed);
  dictAddOp4('d','u','m','p', &dump);
*/
  Serial.println(F("Dictionary ready"));
}


void setup() {
  Serial.begin(9600);
  Serial.println(F("XForth"));
  
  memInit();
  dictionaryInit();

  dPush(15);
  dPush(3);
  Word ref=readWord(REG_DICT_PTR); 
  Word xt=ref+6;
  Word params=ref+8;

  Word w=(Word) &op_add;
  ((OP) w)();
  words();
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

void loop() {
  clearHasError();
  
  Word nextWordBase=readWord(ROREG_NEXTWORD_BASE);
  Serial.print("nextWord at ");
  Serial.println(nextWordBase);
  Byte length=readByte(nextWordBase);

  // show nextWord
  printNextWord();
  Serial.println();
  // show data stack
  Serial.print("nextWord=");
  printNextWord();
  Serial.println();
  
  Serial.print(F("dStack: [ "));
  Word dStackBase=readWord(ROREG_DSTACK_BASE);
  Word dStackNext=readWord(REG_DSTACK_NEXT);
  for (Word i=dStackBase; i<dStackNext; i+=CELLSIZE) {
    Serial.print(readWord(i));
    Serial.print(" ");
  }
  Serial.println("]");

  Word ch=readSerialChar();
  if (ch=='M') {
    dump();
    return;
  } else if (ch=='0') {
    dPush(0);
    return;
  } else if (ch=='D') {
    dPush(readWord(REG_DICT_PTR));
    return;
  }


  if (ch == '\r' || ch=='\n' || ch==' ' || ch=='\t') {
    if (length > 0) {
      executeNextWord();
      writeByte(nextWordBase,0); // clear length
      return;
    } else {
      // ignore
      return;
    }
  } else if (ch>='0' && ch<='9') {
    dPush(ch-'0');
  } else {
    if (length+1 >= MAX_WORD_LENGTH) {
      setHasError();
      Serial.println(F("Word too long"));
      return;
    }
    writeByte(nextWordBase + 1 + length, ch);
    writeByte(nextWordBase + 1 + length +1, 0); // c-string terminator
    writeByte(nextWordBase, length+1);

  }

}
