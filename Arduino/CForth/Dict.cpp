#include "Constants.h"
#include <stdlib.h>

static DictEntry *dictionaryHead=NULL; 


static DictEntry currDE;



DictEntry *getDictionaryHead() {
  return dictionaryHead;
}

static void setDictionaryHead (DictEntry *de) {
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


void dictCreate (char *nextWord) {
  char *ptr=(char *) malloc(strlen(nextWord)+1);
  strcpy(ptr,nextWord);

  DictEntry *de=(DictEntry *) malloc(sizeof(DictEntry));
  de->name=ptr;
  de->type=DE_TYPE_CONSTANT;
  de->address=0;
  de->next=getDictionaryHead();
  setDictionaryHead(de);
}



