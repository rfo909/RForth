#include "common.h"





static Long dstack[DSTACK_DEPTH];
static Byte dStackPos=0;




void dsPushRef(Ref value) {
    dstack[dStackPos++] = value;
}
void dsPushByte(Byte value) {
    dstack[dStackPos++] = value;
}
void dsPushValue(Long value) {
    dstack[dStackPos++] = value;
}


Long dsPopValue () {
    if (dStackPos > 0) {
        return dstack[dStackPos--];
    } else {
        return 0;
    }
}

Ref dsPopRef ()  {
    return (Ref) dsPopValue();
}

Ref dsPopByte ()  {
    return (Ref) dsPopValue();
}