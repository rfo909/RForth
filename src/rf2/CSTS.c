#include "rfcommon.h"

// CS = Call Stack
// TS = Temp Stack

// Call Stack Frame (CSF) field offsets
#define CSF_code            0   // Ref
#define CSF_tempStackBase   2   // byte
#define CSF_tempStackNext   3   // byte
#define CSF_pc              4   // Byte

#define CSF_n               5

void csInit () {
    writeRef(H_CS_NEXT_FRAME,0);
}

bool csEmpty() {
    return (readRef(H_CS_NEXT_FRAME)==null);
}

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
    Ref codeRef = readRef(csf+CSF_code);

    Byte pc=readByte(csf+CSF_pc);
    Byte opCode=readInt1(codeRef + pc);
    pc++;
    writeByte(csf+CSF_pc, pc);  
    return opCode;
}

static Byte csGetPC () {
    Ref currCSF = csGetCurrFrame();
    return readByte(currCSF + CSF_pc);    
}

void csJump (Byte value) {
    Ref currCSF = csGetCurrFrame();
    writeByte(currCSF + CSF_pc, value);    
}

void csCall (Ref addr) {
    DEBUG("csCall");
    DEBUGint("addr",addr);
    Byte nextFrame = 0;
    Byte tsNext = 0;
    if (!csEmpty()) {
        Ref currCSF = csGetCurrFrame();
        Byte tsNext = readByte(currCSF + CSF_tempStackNext);
        nextFrame = readByte(H_CS_NEXT_FRAME);
    }
    writeByte(H_CS_NEXT_FRAME, nextFrame + 1);
    Ref newCSF = csGetCurrFrame();  
    writeRef(newCSF + CSF_code, addr);
    writeByte(newCSF + CSF_tempStackBase, tsNext);
    writeByte(newCSF + CSF_tempStackNext, tsNext);
    writeByte(newCSF + CSF_pc, 0);
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

