#include "rfcommon.h"


static Byte *heap;
static int heapPointer=0;


#include "Heap.h"

static Ref heapStaticDataBegin = (Ref) 0;
static Ref heapStaticDataEnd = (Ref) 0;


void initHeap() {
    // ACode.txt 
    heap=malloc(HEAP_SIZE);
    int data[] = COMPILED_DATA;
    for (int i=0; i<COMPILED_DATA_LENGTH; i++) {
        *(heap+i)=(Byte) data[i];
    }
    heapPointer=COMPILED_DATA_LENGTH;

    heapStaticDataBegin = readRef(H_HEAP_STATIC_DATA_BEGIN);
    heapStaticDataEnd = readRef(H_HEAP_STATIC_DATA_END);
    //heapStaticDataBegin=19;
    //heapStaticDataEnd=3619
 
    char buf[100];
    sprintf(buf,"initHeap heapStaticDataBegin=%d heapStaticDataEnd=%d\r\n", heapStaticDataBegin, heapStaticDataEnd);

    serialEmitStr(buf);
}


static void verifyHeapWriteAddr (Ref addr) {

    if (addr >= heapStaticDataBegin && addr < heapStaticDataEnd) {
        DEBUG("Invalid WRITE address - inside static data\r\n");
        DEBUGint("addr",addr);
        DEBUGint("heapStaticDataBegin", heapStaticDataBegin);
        DEBUGint("heapStaticDataEnd", heapStaticDataEnd);
        PANIC("Invalid WRITE");
    } 
    if (addr >= HEAP_SIZE) {
        DEBUG("Invalid write address - outside heap\r\n");
        DEBUGint("addr",addr);
        PANIC("Invalid WRITE");
    }
}

static void verifyHeapReadAddr (Ref addr) {
    if (addr >= HEAP_SIZE) {
        DEBUG("Invalid read address - outside heap\r\n");
        DEBUGint("addr",addr);
        PANIC("Invalid READ");
    }
}

Byte *refToPointer (Ref ref) {
    return (heap+ref);
}


#define MAX_STRING_CHECK_LENGTH       256


// Safely get a string from the heap, checking that the characters are printable, 
// and that it terminates correctly, or PANIC
char *safeGetString (Ref ref) {
    //DEBUG("safeGetString");
    //DEBUGint("ref",ref);

    char *ptr=refToPointer(ref);
    for (int i=0; i<MAX_STRING_CHECK_LENGTH; i++) {
        char c=*(ptr+i);
        if (c=='\0') break; // end of string
        bool ok = (c>=32 && c<126) || c=='\n' || c=='\r' || c=='\t';
        if (!ok) {
            DEBUGint("Failing at pos", i);
            PANIC("Invalid string reference: nonprintable characters");
            return "<NOT-A-STRING>";
        }
    }
    return ptr;
}



Ref heapMalloc (int length) {
    Ref here=readRef(H_HERE); // first available byte position
    Ref nextHere = here + length;
    if (nextHere > HEAP_SIZE) {
        PANIC("Heap overflow");
        return 0;
    }
    writeRef(H_HERE, nextHere);
    return here;
}




Byte readByte(Ref addr) {
    return (Byte) readInt1 (addr);
}

Ref readRef(Ref addr) {
    return (Ref) readInt2(addr);
}




Byte readInt1(Ref addr) {
    verifyHeapReadAddr(addr);
    return *(heap+addr);
}

Long readInt2(Ref addr) {
    return readInt1(addr) << 8 | readInt1(addr+1);
}
Long readInt4(Ref addr) {
    return readInt2(addr) << 16 | readInt2(addr+2);
}



void writeByte (Ref addr, Byte value) {
    writeInt1(addr,value);
}

void writeRef (Ref addr, Ref value) {
    writeInt2(addr,value);
}



void writeInt1 (Ref addr, Long value) {
    verifyHeapWriteAddr(addr);
    *(heap+addr)=(Byte) value;
}
void writeInt2 (Ref addr, Long value) {
    writeInt1(addr, value>>8);
    writeInt1(addr+1, value);
}
void writeInt4 (Ref addr, Long value) {
    writeInt2(addr, value >> 16);
    writeInt2(addr+2, value);
}


Byte readGlobal (Long addr) {
    printf("readGlobal");
    return (Byte) 0;
}

void writeGlobal (Long addr, Byte value) {
    printf("writeGlobal");
}




