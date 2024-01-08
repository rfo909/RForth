#include "rfcommon.h"

int main()
{
    // ready to test stepping through the assembly code, just need to do:
    // - implement getLocal and setLocal (Main.c)
    // - generate disassembler code to show next instruction before executing it

    // handles calling a setup function then a main loop, both defined in assembly,
    // and processing panic's.


    forthMainLoop();
}
