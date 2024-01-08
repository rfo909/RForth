#include "rfcommon.h"


// NOTE maps \r \n \t to space

#define TIB_SIZE        100

static char *buf;
static int fStartPos;
static int fReadPos;

static int inputWritePos;

void initTIB() {
    buf=malloc(TIB_SIZE);
    fStartPos=0;
    fReadPos=0; 
    inputWritePos=0;
}

static char filter (char c) {
    if (c=='\t' || c=='\n' || c=='\r') return ' ';
    return c;
}

// Call serialNextChar() which may block until there are data
static void getInputCharacter() {
    char c = serialNextChar();
    serialEmitChar(c);
    c=filter(c);
    buf[inputWritePos]=c;
    inputWritePos=(inputWritePos+1) % TIB_SIZE;
}


char getTIBsChar() {
    if (fStartPos == inputWritePos) getInputCharacter();
    return buf[fStartPos];
}
char getTIBrChar() {
    if (fReadPos == inputWritePos) getInputCharacter();
    return buf[fReadPos];
}

void advanceTIBr() {
    if (fReadPos == inputWritePos) getInputCharacter();
    fReadPos=(fReadPos+1)%TIB_SIZE;
}

void advanceTIBs() {
    if (fStartPos == inputWritePos) getInputCharacter();
    if (fReadPos==fStartPos) {
        fStartPos=(fStartPos+1) % TIB_SIZE;
        fReadPos=fStartPos;
    } else {
        fStartPos=(fStartPos+1) % TIB_SIZE;
    }
}
void resetTIBr() {
    fReadPos=fStartPos;
}
void confirmTIBr() {
    fStartPos=fReadPos;
}

// return number of characters from startPos to readPos (circular array, can not just subtract)
int getTIBWordLength() {
    int rp=fReadPos;
    if (rp < fStartPos) rp += TIB_SIZE;
    return rp-fStartPos;
}