#include "rfcommon.h"


/*
The point of this file is to implement a circular buffer where log
data is written, in order to keep a track of events before a PANIC
*/

#define SIZE        2048


static char circBuffer[SIZE];
static int pos=0;
bool init=false;


static void add (char *s) {
    if (!init) {
        for (int i=0; i<SIZE; i++) circBuffer[i] = ' ';
        init=true;
        serialEmitStr("Debug.c init ok\r\n");
    }
    while (*s != '\0') {
        circBuffer[pos] = *s;
        pos=(pos+1) %  SIZE;
        s++;
    }
    //serialEmitStr(s);
}

void DUMP_DEBUG () {
    serialEmitStr("------\r\n");
    for (int i=0; i<SIZE; i++) {
        int p=(pos+i) % SIZE;
        serialEmitChar(circBuffer[p]);
    }
    serialEmitStr("------\r\n");
}

void DEBUG (char *msg) {
    add(msg);
}

void DEBUGint (char *name, int value) {
    add("   ");
    add(name);
    char buf[20];
    sprintf(buf,"=%d",value);
    add(buf);
    add("\r\n");
}

void DEBUGstr (char *name, char *value) {
    add("   ");
    add(name);
    add("=");
    add(value);
    add("\r\n");
}
