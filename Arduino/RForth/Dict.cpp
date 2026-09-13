#include "Constants.h"
#include <stdlib.h>

#define DE_SIZE     7

Word getDeNamePtr(Word dePtr) {return readWord(dePtr);}
Byte getDeType(Word dePtr) {return readByte(dePtr+2);}
Word getDeAddress(Word dePtr) {return readWord(dePtr+3);}
Word getDeNextPtr(Word dePtr) {return readWord(dePtr+5);}

// modifying a existing dictionary entry
void setDeType(Word dePtr, Byte b) {writeByte(dePtr+2,b);}
void setDeAddress(Word dePtr, Word w) {writeWord(dePtr+3,w);}

Word getDictionaryHead() {
  return readWord(generateDataAddress(0));
}

static void setDictionaryHead (Word ptr) {
  writeWord(generateDataAddress(0), ptr);
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
  Word dePtr=codeHERE();
  codeAllot(DE_SIZE);

  writeWord(dePtr,namePtr);             // name
  writeByte(dePtr+2,DE_TYPE_CONSTANT);  // type
  writeWord(dePtr+3,0);                 // address
  writeWord(dePtr+5,getDictionaryHead());   // next

  setDictionaryHead(dePtr);
}


Word dictLookupDE (char *word) {
  Word ptr=getDictionaryHead();
  while (ptr != 0) {
    if (mixedStreq(getDeNamePtr(ptr), word)) return ptr;
    ptr=getDeNextPtr(ptr);
  }
  return 0;
}


// Used by disassembler
Word dictLookupDEByAddr (Word addr) {
  Word ptr=getDictionaryHead();
  while (ptr != 0) {
    if (getDeAddress(ptr)==addr) return ptr;
    ptr=getDeNextPtr(ptr);
  }
  return 0;
}


