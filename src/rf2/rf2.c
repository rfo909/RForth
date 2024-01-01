#include "rfcommon.h"

int main()
{
    initSerial();
    //initHeap();
    initTIB();

    char buf[64];

    // This actually works through the FTDI chip!!
    for(;;) {
        serialEmitStr("ok ");
        
        // skip space
        while (getTIBrChar()==' ') {
            advanceTIBr();
        }
        DEBUG("Skipped space");
        confirmTIBr();

        // look for next space
        while (getTIBrChar() != ' ') {
            serialEmitChar(getTIBrChar());
            advanceTIBr();
        }
        serialEmitNewline();
        DEBUG("Got word");

        // manually show the word from TIBs to TIBr
        while (getTIBsChar() != ' ') {
            serialEmitChar(getTIBsChar());
            advanceTIBs();
        }
        serialEmitNewline();

        if (serialLostChars() != 0) {
            sprintf(buf,"Lost=%d\n", serialLostChars());
            DEBUG(buf);
        }
    }
}
