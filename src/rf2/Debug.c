#include "rfcommon.h"


/*
The point of this file is to implement a circular buffer where log
data is written, in order to keep a track of events before a PANIC
*/

#define SIZE        600


static char circBuffer[SIZE];
static int pos=0;
static bool init=false;
static bool enableDirectLogging=false;

void DEBUGEnable (bool enable) {
    enableDirectLogging=enable;
}


static void add (char *s) {
    if (!init) {
        for (int i=0; i<SIZE; i++) circBuffer[i] = ' ';
        init=true;
        serialEmitStr("Debug.c init ok\r\n");
    }
    if (enableDirectLogging) serialEmitStr(s);

    char *ptr=s;
    while (*ptr != '\0') {
        circBuffer[pos] = *ptr;
        pos=(pos+1) %  SIZE;
        ptr++;
    }
}

void DUMP_DEBUG () {
    serialEmitStr("\r\n------(DUMP_DEBUG)------------------------\r\n");
    for (int i=0; i<SIZE; i++) {
        int p=(pos+i) % SIZE;
        serialEmitChar(circBuffer[p]);
    }
    serialEmitStr("\r\n------------------------------------------\r\n");
}

void DEBUG (char *msg) {
    add(msg);
}

void DEBUGint (char *name, int value) {
    add(" (");
    add(name);
    char buf[20];
    sprintf(buf,"=%d",value);
    add(buf);
    add(")");
}

void DEBUGstr (char *name, char *value) {
    add(" (");
    add(name);
    add("=");
    add(value);
    add(")");
}
