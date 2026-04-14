#include "Constants.h"

DictEntry *dictionaryHead=NULL; 


DictEntry *getDictionaryHead() {
  return dictionaryHead;
}

void setDictionaryHead (DictEntry *de) {
  dictionaryHead=de;
}

DictEntry *dictLookup (char *word) {
  DictEntry *ptr=dictionaryHead;
  while (ptr != NULL) {
    if (!strcmp(ptr->name, word)) {
      return ptr;
    }
    ptr=ptr->next;
  }
  return NULL;
}


// Used by disassembler
DictEntry *dictLookupByAddr (Word addr) {
  DictEntry *ptr=dictionaryHead;
  addr=generateCodeAddress(addr);
  while (ptr != NULL) {
    if (ptr->address==addr) return ptr;
    ptr=ptr->next;
  }
  return NULL;
}



