#include "Constants.h"

static Byte codeSegment[CODE_SEGMENT_SIZE];
static Byte dataSegment[DATA_SEGMENT_SIZE];

static Word staticCodeBytes=0;
static Word codeNext;  // "code.here" - programCounter 0 means no code running (keeping it unsigned)
static Word compileNext;  // "comp.next"

static Word dataNext;   // "HERE"


void memInit(void) {
  dataNext=0;

  // null all bytes in data segment
  for (int i=0; i<DATA_SEGMENT_SIZE; i++) {
    dataSegment[i]=0;
  }

  staticCodeBytes = staticDataSize();
  sPrint("Flash");
  sPrint(" ");
  sPrint("code");
  sPrint(" ");
  sPrint(" ");
  sPrint(":");
  sPrint(" ");
  sPrintWord(staticCodeBytes);
  sPrint(" ");
  sPrint("bytes");
  sPrintln();

  codeNext=staticCodeBytes;
  compileNext=codeNext;
}



void compileOut (Byte b) {
  /*sPrint("compile");
  sPrint(" ");
  sPrint("out");
  sPrint(" ");
  sPrintWord(compileNext);
  sPrint("=>");
  sPrintWord(b);
  sPrintln();*/
  codeSegment[compileNext-staticCodeBytes]=b;
  compileNext++;
}

Word getCodeNext() {
  return codeNext;
}

void setCodeNext (Word w) {
  codeNext=w;
}

void startCompile() {
  compileNext = codeNext;
}

Word getCompileNext() {
  return compileNext;
}

void confirmCompile() {
  codeNext=compileNext;
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
    return 0;
  }
  return (ptr & ADDR_CODE_MASK);   // 00xxxxxx xxxxxxxx
}

Word generateDataAddress (Word ptr) {
  return (ptr & ADDR_DATA_MASK) | DATA_BIT;   // 01xxxxxx xxxxxxxx
}

Word generateCallAddress (Word ptr) {
  if (ptr & DATA_BIT) {
    setHasError();
    sPrint("generate");
    sPrint(" ");
    sPrint("call");
    sPrint(" ");
    sPrint("address");
    sPrint(":");
    sPrint(" ");
    sPrint("data");
    sPrint("bit");
    sPrint(" ");
    sPrint("set");
    sPrintln();
    return 0;
  }
  return generateCodeAddress(ptr) | CALL_BIT;  // 10xxxxxx xxxxxxxx
}

Word HERE() {
  return generateDataAddress(dataNext);
}

Word codeHERE() {
  return generateCodeAddress(codeNext);
}

void dataAllot (Word count) {
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
  compileNext = dataNext;
}

void codeAllot (Word count) {
  if (codeNext+count >= CODE_SEGMENT_SIZE + staticCodeBytes) {
    setHasError();
    sPrint("code");
    sPrint(" ");
    sPrint("segment");
    sPrint(" ");
    sPrint("overflow");
    return;
  }
  codeNext += count;
}


static void verifyReadWriteAddress (Word addr) {
# ifdef MONITOR_C_STACK
  checkCStackSize();
# endif

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
      sPrint(" ");
      sPrintWord(addr);
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
  Word value = (a << 8) | b;
  return value;
}

void writeWord (Word addr, Word value) {
  writeByte(addr, (value >> 8) & 0xFF);
  writeByte(addr+1, value & 0xFF);
}

void memDump() {
  sPrintln();
  sPrint("code");
  sPrint(" ");
  sPrint("segment");
  sPrintln();
  for (Word i=staticCodeBytes; i<codeNext; i++) {
    sPrintWord(i);
    sPrint("=>");
    Byte val=codeSegment[i-staticCodeBytes];
    sPrintWord(val);
    if (val > 32 && val < 128) {
      sPrint(" ");
      printChar(val);
    }
    sPrintln();
  }
  sPrintln();
  sPrint("data");
  sPrint(" ");
  sPrint("segment");
  sPrintln();
  for (Word i=0; i<dataNext; i++) {
    sPrintWord(i);
    sPrint("=>");
    Byte val=dataSegment[i];
    sPrintWord(val);
    if (val > 32 && val < 128) {
      sPrint(" ");
      printChar(val);
    }
    sPrintln();
  }
}


void memCodeExport() {
  sPrintln();
  sPrint("static Word size=");
  sPrintWord(codeNext);
  sPrint(";");
  sPrintln();
  sPrintln();
  sPrint("static const PROGMEM Byte dataSegmentStatic[]={");
  sPrintln();

  // 5 first bytes require special attention: dummy + dictionary header from RAM + dataNext
  sPrint("  ");
  sPrintWord(255);
  sPrint(",");
  sPrintWord(readByte(generateDataAddress(0)));
  sPrint(",");
  sPrintWord(readByte(generateDataAddress(1)));
  Word a=(dataNext>>8) & 0xFF;
  Word b=dataNext & 0xFF;
  sPrint(",");
  sPrintWord(a);
  sPrint(",");
  sPrintWord(b);
  sPrintln();

  Byte count=0;

  for (Word i=5; i<codeNext; i++) {
    if (count==0) sPrint("  ");
    if (i>0) sPrint(",");
    Byte b=readByte(i);  // read static and codeSegment alike
    sPrintWord(b);
    count=(count+1)%20;
    if (count==0) sPrintln();
  }
  if (count != 0) sPrintln();
  sPrint("};");
  sPrintln();
}

void showFreeMem() {
  int x;
  sPrintln();
  sPrint("code");
  sPrint(" ");
  sPrint("used");
  sPrint(":");
  x=codeNext-staticCodeBytes;
  sPrintWord(x);
  sPrint(" ");
  sPrint("free");
  sPrint(":");
  x=CODE_SEGMENT_SIZE-codeNext+staticCodeBytes;
  sPrintWord(x);
  sPrintln();

  sPrint("data");
  sPrint(" ");
  sPrint("used");
  sPrint(":");
  sPrintWord(dataNext);
  sPrint(" ");
  sPrint("free");
  sPrint(":");
  x=DATA_SEGMENT_SIZE-dataNext;
  sPrintWord(x);
  sPrintln();
}



// For executing code, less checks
Byte readByteFast (Word addr) {
  if (addr & DATA_BIT) {
    // data segment - remove data bit
    addr=addr & ADDR_DATA_MASK;
    return dataSegment[addr];
  } else {
    // code segment - strip call bit
    addr=addr & ADDR_CODE_MASK;
    if (addr < staticCodeBytes) {
      return staticDataRead(addr);
    } else {
      return codeSegment[addr-staticCodeBytes];
    }
  }

}
