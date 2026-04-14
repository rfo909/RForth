#include "Constants.h"

static Byte codeSegment[CODE_SEGMENT_SIZE];
static Byte dataSegment[DATA_SEGMENT_SIZE];

static Word staticCodeBytes=0;
static Word codeNext;  // "code.here" - programCounter 0 means no code running (keeping it unsigned)
static Word compileNext;  // "comp.next"

static Word dataNext;   // "HERE"


void memInit(void) {
  dataNext=0;

  staticCodeBytes = staticDataSize();
  codeNext=staticCodeBytes;
  compileNext=staticCodeBytes;
}


void compileOut (Byte b) {
  /*
  Serial.print(F("compileOut compileNext="));
  Serial.print(compileNext);
  Serial.print(F(" Byte="));
  Serial.println(b);
  */
  codeSegment[compileNext++]=b;
}

Word getCodeNext() {
  return codeNext;
}

void setCodeNext (Word w) {
  codeNext=w;
}

void initCompileNext() {
  compileNext = codeNext;
}

Word getCompileNext() {
  return compileNext;
}


Word generateCodeAddress (Word ptr) {
  if (ptr & DATA_BIT) {
    setHasError();
    sPrint("invalid");
    sPrint(" ");
    sPrint("code");
    sPrint(" ");
    sPrint("address");
    sPrint(" ");
    sPrint(":");
    sPrint(" ");
    sPrint("bit");
    sPrint(" ");
    sPrint("15");
    sPrint(" ");
    sPrint("set");
    sPrint(" ");
    sPrint("=");
    sPrint(" ");
    sPrint(">");
    sPrint(" ");
    sPrint("data");
    sPrint(" ");
    sPrint("segment");
    sPrint(" ");
    sPrintln();
    return;
  }
  return (ptr & ADDR_CODE_MASK);   // 00xxxxxx xxxxxxxx
}

Word generateDataAddress (Word ptr) {
  return (ptr & ADDR_DATA_MASK) | DATA_BIT;   // 01xxxxxx xxxxxxxx
}

Word generateCallAddress (Word ptr) {
  return generateCodeAddress(ptr) | CALL_BIT;  // 10xxxxxx xxxxxxxx
}

Word HERE() {
  return generateDataAddress(dataNext);
}

void memAllot (Word count) {
    if (dataNext+count >= DATA_SEGMENT_SIZE) {
    setHasError();
    sPrint("data");
    sPrint(" ");
    sPrint("segment");
    sPrint(" ");
    sPrint("overflow");
    sPrintln();
    return;
  }
  dataNext += count;
}


static void verifyReadWriteAddress (Word addr) {
  if (addr & DATA_BIT) {
    // data segment
    addr=addr & ADDR_DATA_MASK;
    if (addr >= dataNext) {
      setHasError();
      sPrint("invalid");
      sPrint(" ");
      sPrint("data");
      sPrint(" ");
      sPrint("segment");
      sPrint(" ");
      sPrint("address");
      sPrint(" ");
      sPrint(":");
      sPrint(" ");
      sPrintWord(addr);
      sPrint(" ");
      sPrint("data");
      sPrint("next");
      sPrint(" ");
      sPrint("=");
      sPrint(" ");
      sPrintWord(dataNext);
      sPrintln();
      return;
    }
  } else {
    // code segment
    addr=addr & ADDR_CODE_MASK;
    if (addr >= CODE_SEGMENT_SIZE + staticCodeBytes) {
      setHasError();
      sPrint("invalid");
      sPrint(" ");
      sPrint("address");
      sPrint("=");
      sPrintWord(addr);
      sPrint(" ");
      sPrint("code");
      sPrint("_");
      sPrint("segment");
      sPrint("_");
      sPrint("size");
      sPrint("=");
      sPrintWord(CODE_SEGMENT_SIZE);
      sPrint(" ");
      sPrint("static");
      sPrint("=");
      sPrintWord(staticCodeBytes);
      sPrint(" ");
      sPrint("=>");
      sPrintWord(CODE_SEGMENT_SIZE + staticCodeBytes);
      sPrintln();
      return;
    }
  }
}

void writeByte (Word addr, Byte b) {
  verifyReadWriteAddress(addr);
  /* sPrint("writeByte addr=");
  sPrintWord(addr);
  sPrintln(); */
  if (hasError()) return;

  if (addr & DATA_BIT) {
    //sPrint("DATA_SEGMENT");
    //sPrintln();
    // data segment
    addr=addr & ADDR_DATA_MASK;
    dataSegment[addr]=b;
  } else {
    //sPrint("CODE_SEGMENT");
    //sPrintln();
    // code segment
    addr=addr & ADDR_CODE_MASK;
    if (addr < staticCodeBytes) {
      setHasError();
      sPrint("invalid");
      sPrint(" ");
      sPrint("address");
      sPrint(":");
      sPrint(" ");
      sPrint("static");
      sPrintln();
      return;
    }
    codeSegment[addr-staticCodeBytes] = b;
  }

}

Byte readByte (Word addr) {
  verifyReadWriteAddress(addr);
  if (hasError()) return 255;

  if (addr & DATA_BIT) {
    // data segment
    addr=addr & ADDR_DATA_MASK;
    return dataSegment[addr];
  } else {
    // code segment
    addr=addr & ADDR_CODE_MASK;
    if (addr < staticCodeBytes) {
      return staticDataRead(addr);
    } else {
      return codeSegment[addr-staticCodeBytes];
    }
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


Byte codeSegmentGet (Word pos) {
  return codeSegment[pos];
}

void codeSegmentSet (Word pos, Byte val) {
  /*
  sPrint("codeSegmentSet, pos=");
  sPrintWord(pos);
  sPrint(" val=");
  sPrintWord(val);
  sPrintln();
  */
  codeSegment[pos] = val;
}
