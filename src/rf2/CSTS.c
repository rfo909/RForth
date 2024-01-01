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
    Byte opCode=readByte(codeRef + pc);
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


// disassembler for next op

static char *getOpName (int op) {
    switch(op) {
        case 0: { return "JMP";}
        case 1: { return "COND_JMP";}
        case 2: { return "CALL";}
        case 3: { return "RETURN";}
        case 4: { return "PANIC";}
        case 5: { return "HEAP_MAX";}
        case 6: { return "TIBs";}
        case 7: { return "TIBr";}
        case 8: { return "TIBr+";}
        case 9: { return "TIBs+";}
        case 10: { return "TIBr<";}
        case 11: { return "TIBs>";}
        case 12: { return "TIBW";}
        case 13: { return "EMIT";}
        case 14: { return "EMITWORD";}
        case 15: { return "ADD";}
        case 16: { return "SUBTRACT";}
        case 17: { return "MUL";}
        case 18: { return "DIV";}
        case 19: { return "MOD";}
        case 20: { return "RIGHTSHIFT";}
        case 21: { return "LEFTSHIFT";}
        case 22: { return "AND";}
        case 23: { return "OR";}
        case 24: { return "NOT";}
        case 25: { return "EQ";}
        case 26: { return "NE";}
        case 27: { return "GT";}
        case 28: { return "LT";}
        case 29: { return "GE";}
        case 30: { return "LE";}
        case 31: { return "DROP";}
        case 32: { return "SETLOCAL";}
        case 33: { return "GETLOCAL";}
        case 34: { return "WORD";}
        case 35: { return "WRITE1";}
        case 36: { return "WRITE2";}
        case 37: { return "WRITE4";}
        case 38: { return "READ1";}
        case 39: { return "READ2";}
        case 40: { return "READ4";}
        case 41: { return "READ1g";}
        case 42: { return "WRITE1g";}
        case 43: { return "LITERAL1";}
        case 44: { return "LITERAL2";}
        case 45: { return "LITERAL4";}
        case 46: { return "CHECK_PARSE";}
        case 47: { return "PARSE";}
        default: { return "<UNKNOWN>"; }
    }
}

void csShowOp () {
    Ref csf=csGetCurrFrame();
    Ref codeRef = readRef(csf+CSF_code);

    Byte pc=readByte(csf+CSF_pc);
    Byte opCode=readByte(codeRef + pc);

    DEBUG("csShowOp - next op");
    DEBUGint("codeRef", codeRef);
    DEBUGint("pc",(int) pc);
    DEBUGint("opCode",(int) opCode);
    DEBUGstr("getOpName",getOpName(opCode));
}


