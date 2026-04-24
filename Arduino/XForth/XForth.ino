#include "Constants.h"


Byte codeSegment[CODE_SEGMENT_SIZE];
Byte dataSegment[DATA_SEGMENT_SIZE];

Byte flashCode[]={1,2,3};
const Word flashCodeLength=3;

#define DATA_ADDRESS(x) (x | DATA_BIT)

#define REG_DICT_PTR      DATA_ADDRESS(0)
#define REG_STATIC_COUNT  DATA_ADDRESS(2)
#define REG_CODE_NEXT     DATA_ADDRESS(4)
#define REG_DATA_NEXT     DATA_ADDRESS(6)

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

void memInit() {

}

void setup() {
  Serial.begin(9600);
  Serial.println(F("XForth"));
  
  memInit();
}

void loop() {
  // put your main code here, to run repeatedly:

}
