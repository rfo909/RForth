#include "rfcommon.h"


Byte *heap;
int heapPointer=0;



Byte *refToPointer (Ref ref) {
    return (heap+ref);
}

Ref heapMalloc (int length) {
    Ref here=readRef(H_HERE); // first available byte position
    Ref nextHere = here + length;
    writeRef(H_HERE, nextHere);
    return here;
}

#define MAX_STRING_LENGTH       256


// Safely get a string from the heap, checking that the characters are printable, 
// and that it terminates correctly, or PANIC
char *safeGetString (Ref ref) {
    Byte *ptr=refToPointer(ref);
    for (int i=0; i<MAX_STRING_LENGTH; i++) {
        Byte b=*(ptr+i);
        if( (b>=32 && b<126) || b=='\n' || b=='\r' || b=='\t') {
            // ok
        } else {
            PANIC("Invalid string reference");
            return "<Invalid string reference>";
        }
    }
    return ptr;
}



Byte readByte(Ref addr) {
    return (Byte) readInt1 (addr);
}

Ref readRef(Ref addr) {
    return (Ref) readInt2(addr);
}




Byte readInt1(Ref addr) {
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



#include "Heap.h"
int dataLength = COMPILED_DATA_LENGTH;

void initHeap() {
    // ACode.txt 
    heap=malloc(HEAP_SIZE);
    Byte data[] = COMPILED_DATA;
    memcpy(heap, data, dataLength);
    heapPointer=dataLength;
}

// ------------------------------------------------------
// Assembler:Compile output
// ------------------------------------------------------


