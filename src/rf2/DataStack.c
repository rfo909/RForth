#include "rfcommon.h"





static Long dstack[DSTACK_DEPTH];
static Byte dstackNext=0;


void dsInit() {
    dstackNext=0;
}


void dsPushRef(Ref value) {
    dsPushValue((Long) value);
}
void dsPushByte(Byte value) {
    dsPushValue((Long) value);
}
void dsPushValue(Long value) {
    if (dstackNext >= DSTACK_DEPTH) {
        PANIC("dsPush - stack overflow");
        return;
    }
    dstack[dstackNext++] = value;
}


Long dsPopValue () {
    if (dstackNext > 0) {
        dstackNext--;
        return dstack[dstackNext];
    } else {
        PANIC("dsPop - stack empty");
        return 0;
    }
}

Ref dsPopRef () {
    return (Ref) dsPopValue();
}

Byte dsPopByte () {
    return (Byte) dsPopValue();
}

bool dsEmpty () {
    if (dstackNext==0) return true; else return false;
}

Long dsPeek () {
    if (dsEmpty) {
        return 99999; // no PANIC, this is just peek, for debugging
    }
    return dstack[dstackNext-1];
}