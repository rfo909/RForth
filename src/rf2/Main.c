#include "rfcommon.h"


// This is the RFOrth v2 interpreter, which executes the instructions from common.h.
//
// Data structures
//
// The DataStack.c completely manages the data stack.
//
// The Heap.c file sets up the initial heap, as generated by the Assembler CFT script. It implements
// read and write operations on the heap, as well as the "global" read/write, which gives access to 
// hardware registers etc. The Heap.c does NOT keep track of the use of the heap, as all that is 
// implemented in the assembly code (ACode.txt). 
//
// 
// The call stack (CS) and temp stack (TS) are allocated statically inside the heap memory block,
// but managed from C as part of CALL and RETURN for allocating and deallocating call stack
// frames (CSF), and SETLOCAL and GETLOCAL which work on the temp stack, updating fields 
// in the CSF which point to first and next stack frame of TS.
//
// As these structures are located in the heap, the C code only consists of code, no separate
// data structures.
//
// ---
//
// The RFOrth compiler and some utility functions are implemented in ACode.txt together with
// the Assembler CFT script.
//
// The Dictionary is completely managed by ACode.


static char *panicMessage = (char *) null;


void PANIC (char *msg) {
    panicMessage=msg;
}

bool hasPanicFlag() {
    return panicMessage != (char *) null;
}

void clearPanicFlag () {
    panicMessage = (char *) null;
}



// Execute one op-code (may involve more than one call to csNextCodeByte if first byte is a literal, as
// those are followed by data bytes)

static void execute()
{
    csShowOp();

    int op = csNextCodeByte();

    switch (op) {
    case OP_JMP: {
        Ref addr = dsPopRef();
        csJumpToRef(addr);
    }
    break;
    case OP_COND_JMP: {
        Ref addr = dsPopRef();
        Long cond = dsPopValue();
        if (cond != null)
        {
            csJumpToRef(addr);
        }
    }
    break;
    case OP_CALL: {
        Ref addr = dsPopRef();
        csCall(addr);
    }
    break;
    case OP_RETURN: {
        csReturn();
    }
    break;
    case OP_PANIC: {
        Ref word = dsPopRef();
        char *str = safeGetString(word);
        PANIC(str);
    }
    break;
    case OP_HEAP_MAX: {
        dsPushValue(HEAP_SIZE);
    }
    break;
    case OP_TIBs: {
        dsPushValue(getTIBsChar());
    }
    break;
    case OP_TIBr: {
        dsPushValue(getTIBrChar());
    }
    break;
    case OP_TIBr_ADD: {
        advanceTIBr();
    }
    break;
    case OP_TIBs_ADD: {
        advanceTIBs();
    }
    break;
    case OP_TIBr_BACK: {
        resetTIBr();
    }
    break;
    case OP_TIBs_FWD: {
        confirmTIBr();
    }
    break;
    case OP_TIBW: {
        Ref ref = symCreateTIBWord();
        dsPushRef(ref);
    }
    break;
    case OP_EMIT: {
        char c = dsPopByte();
        serialEmitChar(c);
    }
    break;
    case OP_EMITWORD: {
        Ref ref = dsPopRef();
        char *str = safeGetString(ref);
        serialEmitStr(str);
    }
    break;
    case OP_ADD: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        dsPushValue(a + b);
    }
    break;
    case OP_SUBTRACT: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        dsPushValue(a - b);
    }
    break;
    case OP_MUL: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        dsPushValue(a * b);
    }
    break;
    case OP_DIV: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        dsPushValue(a / b);
    }
    break;
    case OP_MOD: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        dsPushValue(a % b);
    }
    break;
    case OP_RIGHTSHIFT: {
        Long n = dsPopValue();
        Long val = dsPopValue();
        dsPushValue(val >> n);
    }
    break;
    case OP_LEFTSHIFT: {
        Long n = dsPopValue();
        Long val = dsPopValue();
        dsPushValue(val << n);
    }
    break;
    case OP_AND: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a & b != 0)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_OR: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a | b != 0)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_NOT: {
        Long val = dsPopValue();
        dsPushValue(~val);
    }
    break;
    case OP_EQ: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a == b)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_NE: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a != b)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_GT: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a > b)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_LT: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a < b)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_GE: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a >= b)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_LE: {
        Long b = dsPopValue();
        Long a = dsPopValue();
        if (a <= b)
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_DROP: {
        dsPopValue();
    }
    break;
    case OP_SETLOCAL: {
        Ref sym = dsPopRef();
        Long value = dsPopValue();
        csSetLocal(sym, value);
    }
    break;
    case OP_GETLOCAL: {
        Ref sym = dsPopRef();
        dsPushValue(csGetLocal(sym));
    }
    break;
    case OP_WORD: {
        // create word from substring of existing word
        Ref ref = dsPopRef();
        char *str = safeGetString(ref);
        addSymbol(str);
    }
    break;
    case OP_WRITE1: {
        Ref ref = dsPopRef();
        long val = dsPopValue();
        writeByte(ref, val);
    }
    break;
    case OP_WRITE2: {
        Ref ref = dsPopRef();
        long val = dsPopValue();
        writeInt2(ref, val);
    }
    break;
    case OP_WRITE4: {
        Ref ref = dsPopRef();
        long val = dsPopValue();
        writeInt4(ref, val);
    }
    break;
    case OP_READ1: {
        Ref ref = dsPopRef();
        dsPushByte(readInt1(ref));
    }
    break;
    case OP_READ2: {
        Ref ref = dsPopRef();
        dsPushValue(readInt2(ref));
    }
    break;
    case OP_READ4: {
        Ref ref = dsPopRef();
        dsPushValue(readInt4(ref));
    }
    break;
    case OP_READ1g: {
        Long addr = dsPopValue();
        dsPushByte(readGlobal(addr));
    }
    break;
    case OP_WRITE1g: {
        Long addr = dsPopValue();
        Byte value = dsPopByte();
        writeGlobal(addr, value);
    }
    break;
    case OP_LITERAL1: {
        dsPushByte(csNextCodeByte());
    }
    break;
    case OP_LITERAL2: {
        Byte a = csNextCodeByte();
        Byte b = csNextCodeByte();
        dsPushValue(a << 8 | b);
    }
    break;
    case OP_LITERAL4: {
        Byte a = csNextCodeByte();
        Byte b = csNextCodeByte();
        Byte c = csNextCodeByte();
        Byte d = csNextCodeByte();
        dsPushValue(a << 24 | b << 12 | c << 8 | d);
    }
    break;
    case OP_CHECK_PARSE: {
        Ref symRef = dsPopRef();
        char *str = safeGetString(symRef);
        if (checkParseInt(str))
            dsPushValue(F_TRUE);
        else
            dsPushValue(F_FALSE);
    }
    break;
    case OP_PARSE: {
        Ref symRef = dsPopRef();
        char *str = safeGetString(symRef);
        dsPushValue(parseInt(str));
    }
    break;
    case OP_JMP1: {
        Byte pc = dsPopByte();
        csJumpToPC(pc);
    }
    break;
    case OP_COND_JMP1: {
        Byte pc = dsPopByte();
        Long condition = dsPopValue();
        if (condition != 0) csJumpToPC(pc);
    }
    break;
    default: {
        DEBUG("Unknown op-code\r\n");
        DEBUGint("op",op);
        PANIC("Unknown op-code");
    }
    }
}



void forthMainLoop () {

    initSerial();

    serialNextChar();

    serialEmitStr("initSerial ok\r\n");

    initHeap();
    initTIB();

    dsInit(); // initialize data stack
    csInit(); // call stack
    // (temp-stack is auto-initialized when pushing first frame on call stack)



    serialEmitStr("Press 3x any key before calling H_MAIN_SETUP\r\n");
    serialNextChar();
    serialNextChar();
    serialNextChar();

    Ref setup=readRef(H_MAIN_SETUP);
    Ref loop=readRef(H_MAIN_LOOP);

    serialEmitStr("forthMainLoop\r\n");
 

    serialEmitStr("Calling H_MAIN_SETUP\r\n");
    serialEmitStr("Press any key\r\n");
    serialNextChar();



    csCall(setup); // create call stack frame

    Long opCount=0;

    // outer loop - never terminates
    for (;;) {
        if (hasPanicFlag()) {
            DEBUG("PANIC: ");
            DEBUG(panicMessage);
            DEBUG("\r\n");
            DEBUGint("opCount",opCount);

            DUMP_DEBUG();

            serialEmitStr("\r\nPANIC: " );
            serialEmitStr(panicMessage);
            serialEmitNewline();
            clearPanicFlag();
            dsInit();
            csInit();
            serialEmitStr("Calling H_MAIN_LOOP\r\n");
            csCall(loop);
            serialEmitStr("Press any key\r\n");
            serialNextChar();
        } 
        if (csEmpty()) {
            serialEmitStr("CS is empty - code must have returned\r\n");
            dsInit();
            csInit();
            serialEmitStr("Calling H_MAIN_LOOP\r\n");
            csCall(loop);
            serialEmitStr("Press any key\r\n");
            serialNextChar();
        }
        // execute one instruction
        execute();
        opCount++;
        // wait for keypress
        //serialNextChar();

    }
}


