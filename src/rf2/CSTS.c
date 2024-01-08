#include "rfcommon.h"

// CS = Call Stack
// TS = Temp Stack

// Call Stack Frame (CSF) field offsets
#define CSF_code            0   // Ref
#define CSF_tempStackBase   2   // byte
#define CSF_tempStackNext   3   // byte
#define CSF_pc              4   // Ref

#define CSF_n               6

// Temp stack frame (TSF) field offsets
#define TSF_symbol           0   // Ref
#define TSF_value            2   // Long

#define TSF_n                6


/*
TODO

Read and apply limits defined in ACode by tags 

:CS_MAX_DEPTH		_*CS_maxDepth		# byte = number of frames
:TS_MAX_DEPTH		_*TS_maxDepth		# byte = number of frames

*/

void csInit () {
    writeByte(H_CS_NEXT_FRAME,0);
}

bool csEmpty() {
    return (readByte(H_CS_NEXT_FRAME)==null);
}

Ref csGetCurrFrame () {
    Byte nextFrame=readByte(H_CS_NEXT_FRAME);
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
    //Ref codeRef = readRef(csf+CSF_code);

    Ref pc=readRef(csf+CSF_pc);
    Byte opCode=readByte(pc);
    pc++;
    writeRef(csf+CSF_pc, pc);  
    return opCode;
}

static Ref csGetPC () {
    Ref currCSF = csGetCurrFrame();
    return readRef(currCSF + CSF_pc);    
}

void csJumpToRef (Ref addr) {
    Ref currCSF = csGetCurrFrame();
    writeRef(currCSF + CSF_pc, addr); 
}

void csJumpToPC (Byte pc) {
    Ref currCSF = csGetCurrFrame();
    Ref code = readRef(currCSF + CSF_code);

    writeRef(currCSF + CSF_pc, code + pc);    
}

void csCall (Ref addr) {
    Byte nextFrame = 0;
    Byte tsNext = 0;    // temp stack next position

    if (!csEmpty()) {
        Ref currCSF = csGetCurrFrame();
        // get first available temp stack position (index, byte)
        tsNext = readByte(currCSF + CSF_tempStackNext);
        // also read global value for next frame on call stack (index, byte)
        nextFrame = readByte(H_CS_NEXT_FRAME);
        DEBUG("csCall, isolated tsNext\r\n");
        DEBUGint("tsNext",tsNext);
    }
    writeByte(H_CS_NEXT_FRAME, nextFrame + 1);

    Ref newCSF = csGetCurrFrame();  
    writeRef(newCSF + CSF_code, addr);
    writeByte(newCSF + CSF_tempStackBase, tsNext);
    writeByte(newCSF + CSF_tempStackNext, tsNext);
    writeRef(newCSF + CSF_pc, addr); 
}

void csReturn() {
    Byte x = readByte(H_CS_NEXT_FRAME);
    writeByte(H_CS_NEXT_FRAME, x - 1);
}

void csSetLocal (Ref sym, Long value) {
    Ref csf=csGetCurrFrame();
    Ref tsBase = readRef(H_TS_BASE_REF);

    Byte tsBasePos = readByte(csf + CSF_tempStackBase);
    Byte tsNextPos = readByte(csf + CSF_tempStackNext);

    // search for symbol
    for (Byte i = tsBasePos; i<tsNextPos; i++) {
        Ref tsFrame = tsBase + i*TSF_n;
        if (readRef(tsFrame + TSF_symbol) == sym) {
            // found it
            writeInt4(tsFrame + TSF_value, value);
            return;
        }
    }
    // not found
    Byte newPos=tsNextPos;
    // update call stack frame 
    writeByte(csf + CSF_tempStackNext, newPos+1);

    // save data into new frame
    Ref tsFrame = tsBase + newPos*TSF_n;
    writeRef(tsFrame + TSF_symbol, sym);
    writeInt4(tsFrame + TSF_value, value);
}


Long csGetLocal (Ref sym) {
    Ref csf=csGetCurrFrame();
    Ref tsBase = readRef(H_TS_BASE_REF);

    Byte tsBasePos = readByte(csf + CSF_tempStackBase);
    Byte tsNextPos = readByte(csf + CSF_tempStackNext);

    // search for symbol
    for (Byte i = tsBasePos; i<tsNextPos; i++) {
        Ref tsFrame = tsBase + i*TSF_n;
        if (readRef(tsFrame + TSF_symbol) == sym) {
            // found it
            Long value=readInt4(tsFrame + TSF_value);
            return value;
        }
    }
    // not found - PANIC
    DEBUG("Value not found\r\n");
    DEBUGstr("Name",safeGetString(sym));
    PANIC("Undefined local variable");
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
        case 48: { return "JMP1";}
        case 49: { return "COND_JMP1";}
        default: { return "<UNKNOWN>"; }
    }
}

void csShowOp () {
    Ref csf=csGetCurrFrame();
    //Ref codeRef = readRef(csf+CSF_code);

    Ref pc=readRef(csf+CSF_pc);
    Byte opCode=readByte(pc);

    int next1 = readByte(pc+1);
    int next2  = (next1 << 8) + readByte(pc+2);


    char buf[100];

 

    sprintf(buf,"%5d: op=%10s op=%2d next1=%3d next2=%5d",
        (int) pc,
        getOpName(opCode),
        (int) opCode,
        next1,
        next2
    );
    DEBUG(buf);
    DEBUG("  Stack: ");
    for (int i=0; i<20; i++) {
        Long value=dsPeek(i);
        if (value==99999) break;
        sprintf(buf,"%d ", value);
        DEBUG(buf);
    }
    DEBUG("\r\n");
    /*
    DEBUG("First20: ");
    for (int i=0; i<20; i++) {
        sprintf(buf,"%3d ", (int) readInt1(i));
        DEBUG(buf);
    }
    DEBUG("\r\n");
    */
}


