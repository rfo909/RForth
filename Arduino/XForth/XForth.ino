#include "Constants.h"


Byte codeSegment[CODE_SEGMENT_SIZE];
Byte dataSegment[DATA_SEGMENT_SIZE];

// PROGMEM data later
Byte flashCode[]={1,2,3};
const Word flashCodeLength=3;

#define DATA_ADDRESS(x) ((x) | DATA_BIT)

#define REG_DICT_PTR      DATA_ADDRESS(0)
#define REG_STATIC_COUNT  DATA_ADDRESS(2)
#define REG_CODE_NEXT     DATA_ADDRESS(4)
#define REG_HERE      DATA_ADDRESS(6)

#define REG_DSTACK        DATA_ADDRESS(8)
#define REG_RSTACK        DATA_ADDRESS(10)

#define REGISTER_COUNT    6

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
  errorFlag=false;
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
  return readWord(REG_HERE);
}

Word codeNext() {
  return readWord(REG_CODE_NEXT);
}

void allot (Word count) {
  Word here=HERE()+count;
  if (here >= DATA_SEGMENT_SIZE) {
    setHasError();
    Serial.println(F("allot: out of data space"));
    return;
  }
  writeWord(REG_HERE, here+count);
}

void codeOut (Byte b) {
  Word w=codeNext();
  if (w-flashCodeLength + 1 >= CODE_SEGMENT_SIZE) {
    setHasError();
    Serial.println(F("codeOut: out of code space"));
    return;
  }
}

void codeOutWord (Word w) {
  codeOut((w>>8) & 0xFF);
  if (hasError()) return;
  codeOut(w & 0xFF);
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

  // all registers are initialized to 0 when setting all bytes to 0
  allot(REGISTER_COUNT * CELLSIZE);

  writeWord(REG_STATIC_COUNT, flashCodeLength);
  writeWord(REG_CODE_NEXT, flashCodeLength);

  // create dStack
  Word dStack=HERE();
  allot(DSTACK_SIZE * CELLSIZE);
  Word rStack=HERE();
  allot(RSTACK_SIZE * CELLSIZE);

  writeWord(REG_DSTACK, dStack);
  writeWord(REG_RSTACK, rStack);
}

// -----------------------------
// Dictionary initalialization

/*
The dictionary entry consists of 
  - previousPointer (Word)
  - nameReference (Word)
  - flags (Byte)
  - codePointer 
*/

void dictionaryInit() {

}


void setup() {
  Serial.begin(9600);
  Serial.println(F("XForth"));
  
  memInit();
  Serial.println(F("Memory init ok"));
  dictionaryInit();
  Serial.println(F("Dictionary ready"));
}

void loop() {
  // put your main code here, to run repeatedly:

}
