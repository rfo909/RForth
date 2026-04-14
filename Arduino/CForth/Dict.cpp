#include "Constants.h"
#include <stdlib.h>

#define DE_SIZE     7

// contains data about one dictionary entry at a time
static Word deLocation;

static Word deNamePtr;
static Byte deType;
static Word deAddress;
static Word deNextPtr;

Word getDeNamePtr() {return deNamePtr;}
Byte getDeType() {return deType;}
Word getDeAddress() {return deAddress;}
Word getDeNextPtr() {return deNextPtr;}

void setDeType(Byte b) {deType=b;}
void setDeAddress(Word w) {deAddress=w;}

Word getDictionaryHead() {
  return readWord(generateDataAddress(0));
}

static void setDictionaryHead (Word ptr) {
  writeWord(generateDataAddress(0), ptr);
}

void dictEntryFetch (Word ptr) {
  deLocation=ptr;
  deNamePtr=readWord(ptr);
  deType=readByte(ptr+2);
  deAddress=readWord(ptr+3);
  deNextPtr=readWord(ptr+5);
}

void dictEntrySave() {
  Word ptr=deLocation;
  if (ptr > codeHERE-DE_SIZE) {
    setHasError();
    sPrint("dict");
    sPrint("entry");
    sPrint("save");
    sPrint(" ");
    sPrintWord(ptr);
    sPrintln();
    return;
  }
  writeWord(ptr,deNamePtr);
  writeByte(ptr+2,deType);
  writeWord(ptr+3,deAddress);
  writeWord(ptr+5,deNextPtr);
}

void dictCreate (char *newWord) {
  // store name in code segment
  Word namePtr=codeHERE();
  Byte len=strlen(newWord);
  codeAllot(len+1);
  writeByte(namePtr, len);    // length byte
  for (Byte i=0; i<len; i++) {
    writeByte(namePtr+1+i, newWord[i]);
  }
  // populate global variables
  deNamePtr=namePtr;
  deType=DE_TYPE_CONSTANT;
  deAddress=0;
  deNextPtr=getDictionaryHead();

  deLocation=codeHERE();
  codeAllot(DE_SIZE);

  dictEntrySave();
  setDictionaryHead(deLocation);
}


Boolean dictLookup (char *word) {
  Word ptr=getDictionaryHead();
  while (ptr != 0) {
    dictEntryFetch(ptr);
    if (mixedStreq(deNamePtr, word)) return true;
    ptr=deNextPtr;
  }
  return false;
}


// Used by disassembler
Boolean dictLookupByAddr (Word addr) {
  Word ptr=getDictionaryHead();
  while (ptr != 0) {
    dictEntryFetch(ptr);
    if (deAddress==addr) return true;
    ptr=deNextPtr;
  }
  return false;
}


