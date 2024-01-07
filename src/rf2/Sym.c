#include "rfcommon.h"


// Manages the symbol store. It consists of simple CONS cells, with a 2 byte value
// Ref and a 2 byte next Ref.

// Create symbol for string between TIBs and TIBr

#define BUF_SIZE        64

static Byte buf[BUF_SIZE];

static Ref lookupSymbol (char *symbol);
static Ref addNewSymbol (char *symbol);


// Returns ref to the string stored in the symbol stack (heap), not the CONS cells
// that IS the stack
Ref symCreateTIBWord () {
    int length=getTIBWordLength();
    if (length > BUF_SIZE-1) {
        PANIC("Symbol-too-long");
        return (Ref) null;
    }
    // copy characters
    int pos=0;
    while (getTIBWordLength() > 0) {
        buf[pos++] = getTIBsChar();
        advanceTIBs();
    }
    buf[pos++]='\0';
    return addSymbol(buf);
}

Ref addSymbol (char *str) {
    Ref ref=lookupSymbol(str);
    if (ref == null) {
        ref = addNewSymbol(str);
    }
    return ref;
}


static Ref lookupSymbol (char *symbol) {
    Ref top = readRef(H_SYM_TOP_REF);
    while (top != null) {
        Ref strRef = readRef(top);
        char *str=(char *) refToPointer(strRef);

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
    Ref strSpace = heapMalloc(strlen(symbol)+1);
    strcpy(refToPointer(strSpace), symbol);

    Ref cons=heapMalloc(4);
    writeRef(cons, strSpace);
    writeRef(cons+2, readRef(H_SYM_TOP_REF));
    writeRef(H_SYM_TOP_REF, cons);

    return strSpace;
}

