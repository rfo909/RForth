#include "rfcommon.h"

int main()
{
    initSerial();
    initHeap();
    initTIB();

    char buf[64];

    // Serial ok - running at 9600 baud
    // dropped characters solved by implementing circular buffer in Serial.c
    // TIB* has separate circular buffer for TIB* pointer logic
    // The TIB functions seem to work
    // Sym.c seems to work

    for(;;) {

        serialEmitStr("\r\nok ");
        
        // skip space
        while (getTIBrChar()==' ') {
            advanceTIBr();
        }
        confirmTIBr();

        // look for next space
        while (getTIBrChar() != ' ') {
            advanceTIBr();
        }
        Ref symbolRef = symCreateTIBWord();
        sprintf(buf,"symbolRef=%d\r\n", symbolRef);
        serialEmitStr(buf);
        //char *s = (char *) refToPointer(symbolRef);
        //sprintf(buf,"%d ", symbolRef);
        //serialEmitStr(buf);
        //serialEmitStr(s);
        serialEmitNewline();
        
        confirmTIBr();
        if (serialLostChars() != 0) {
            sprintf(buf,"Lost=%d\n", serialLostChars());
            DEBUG(buf);
        }
    }
}
