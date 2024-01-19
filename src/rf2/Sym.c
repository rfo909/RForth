#include "rfcommon.h"


// Manages the symbol store. It consists of simple CONS cells, with a 2 byte value
// Ref and a 2 byte next Ref.

// Create symbol for string between TIBs and TIBr

#define BUF_SIZE        64


static Ref lookupSymbol (char *symbol);
static Ref addNewSymbol (char *symbol);


// Returns ref to the string stored in the symbol stack (heap), not the CONS cells
// that IS the stack
Ref symCreateTIBWord () {
    DEBUG("symCreateTIBWord");

    int length=getTIBWordLength();
    DEBUGint("length",length);
    if (length > BUF_SIZE-1) {
        DEBUG("symCreateTIBWord, symbol too long\r\n");
        DEBUGint("length",length);
        DEBUGint("limit", BUF_SIZE);
        PANIC("Symbol too long");
        return (Ref) null;
    }

    // copy characters
    char buf[BUF_SIZE];
    int pos=0;
    while (pos<length) {
        *(buf+pos)=getTIBsChar();
        advanceTIBs();
        pos++;
    }
    *(buf+pos) = 0; // terminate string
    return addSymbol(buf);
}


Ref addSymbol (char *str) {
    DEBUG("addSymbol\r\n");
    DEBUGstr("str",str);
    Ref ref=lookupSymbol(str);
    if (ref == null) {
        ref = addNewSymbol(str);
    }
    return ref;
}


static Ref lookupSymbol (char *symbol) {
    Ref top = readRef(H_SYM_TOP_REF);  // cons cell  
    while (top != null && !hasException()) {
        Ref strRef = readRef(top);
        char *str=safeGetString(strRef);
        DEBUG("lookupSymbol: comparing '");
        DEBUG(symbol);
        DEBUG("' to '");
        DEBUG(str);
        DEBUG("'\r\n");
        if (!strcmp(str,symbol)) {
            return strRef;
        }
        top=readRef(top+2); // next
    }
    return (Ref) null;
}


// Returns ref to the string stored in the symbol stack (heap), not the CONS cells
// that IS the stack
static Ref addNewSymbol (char *symbol) {
    DEBUG("addNewSymbol");
    DEBUG(symbol);
    Ref strSpace = heapMalloc(strlen(symbol)+1);
    strcpy(refToPointer(strSpace), symbol);

    Ref cons=heapMalloc(4);
    writeRef(cons, strSpace);
    writeRef(cons+2, readRef(H_SYM_TOP_REF));
    writeRef(H_SYM_TOP_REF, cons);

    return strSpace;
}

