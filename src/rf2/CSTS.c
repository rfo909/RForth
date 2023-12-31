#include "rfcommon.h"

// CS = Call Stack
// TS = Temp Stack

// Call Stack Frame (CSF) field offsets
#define CSF_code            0   // Ref
#define CSF_tempStackBase   2   // byte
#define CSF_tempStackNext   3   // byte
#define CSF_pc              4   // Ref

#define CSF_n               6

Ref csGetCurrFrame () {
    Ref nextFrame=readRef(H_CS_NEXT_FRAME);
    if (nextFrame==0) {
        PANIC("CS empty");
        return null;
    }
    return readRef(H_CS_BASE_REF) + (nextFrame-1)*CSF_n;
}

/*
Return next code byte, and increment PC inside CSF
*/
Byte csNextCodeByte () {
    Ref csf=csGetCurrFrame(); // current call stack frame
    Ref pc=readInt2(csf+CSF_pc);
    Byte opCode=readInt1(pc);
    pc++;
    writeInt2(csf+CSF_pc, pc);  
    return opCode;
}

static Ref csGetPC () {
    Ref currCSF = csGetCurrFrame();
    return readRef(currCSF + CSF_pc);    
}

void csJump (Ref value) {
    Ref currCSF = csGetCurrFrame();
    writeRef(currCSF + CSF_pc, value);    
}

void csCall (Ref addr) {
    Ref currCSF = csGetCurrFrame();
    Byte nextFrame = readByte(H_CS_NEXT_FRAME);
    writeByte(H_CS_NEXT_FRAME, nextFrame + 1);
    Ref newCSF = csGetCurrFrame();
    Byte tsNext = readByte(currCSF + CSF_tempStackNext);
    writeRef(newCSF + CSF_code, addr);
    writeByte(newCSF + CSF_tempStackBase, tsNext);
    writeByte(newCSF + CSF_tempStackNext, tsNext);
    writeRef(newCSF + CSF_pc, 0);
}

void csReturn() {
    Byte x = readByte(H_CS_NEXT_FRAME);
    writeByte(H_CS_NEXT_FRAME, x - 1);

}

void csSetLocal (Ref sym, Long value) {

}

Long csGetLocal (Ref sym) {
    return null;
}

