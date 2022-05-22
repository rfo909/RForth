#include "Storage.hh"
#include "OpCodes.hh"

// ps = persistent string data

static char psData[P_STRING_SIZE];
static int psStart=0;
static int psNext=0;

void psAddChar (char c) {
  if (psNext >= P_STRING_SIZE) {
    Serial.println(F("psAddChar: no more space"));
    setAbortCodeExecution();

    return;
  }
  psData[psNext++]=c; 
}

void psAddStr (char *str) {
  for (int i=0; i<strlen(str); i++) psAddChar(str[i]);
}

char *psChop () {
  psAddChar('\0');
  char *s=psData+psStart;
  psStart=psNext;
  return s;
}

int psCount() {
  return psNext;
}


// pc = persistent code data 

static byte pcData[P_CODE_SIZE];
static int pcStart=0;
static int pcNext=0;

void pcAddByte (byte b) {
  if (pcNext >= P_CODE_SIZE) {
    Serial.println(F("psAddByte: no more space"));
    setAbortCodeExecution();
    return;
  }
  pcData[pcNext++]=b;
}

int pcGetMark() {
  return pcStart;
}

void pcResetToMark(int mark) {
  pcStart=mark;
  pcNext=mark;
}

int pcChopInt () {
  int i=pcStart;
  pcStart=pcNext;
  return i;
}

byte *pcGetPointer (long pos) {
  return pcData+pos;
}


int pcGetLocalPos() {
  return pcNext-pcStart;
}

void setLocalPosByte (int pos, byte b) {
  pcData[pcStart+pos]=b;
}




int to7BitPush (int b) {
  return 0x80 | (b & 0x7f);
}

void pcInt7bit (int b) {
  return pcAddByte(to7BitPush(b));
}



void pcInt (long i) {
  bool neg=(i<0);
  if (neg) i=-i;
  
  if(i<127) {
    pcInt7bit(i);
  } else if (i < 16384) { // 14 bits
    pcInt7bit( (i & 0x1fc0) >> 7);
    
    pcInt7bit(7); // arg for LSHIFT
    pcAddByte(OP_LSHIFT);
    
    pcInt7bit(i & 0x7fff);
    pcAddByte(OP_B_OR);  
  } else if (i < 32700) {
    pcInt7bit( i & 0x7F);
    pcInt7bit( (i >> 7) & 0x7F);
    
    pcInt7bit( 7 );
    pcAddByte(OP_LSHIFT);
    
    pcInt7bit( (i >> 14) & 0x7F);
    
    pcInt7bit(14);
    pcAddByte(OP_LSHIFT);
    
    pcAddByte(OP_B_OR);
    pcAddByte(OP_B_OR);
  } else {
    unsigned long xx = (unsigned long) i;
    
    // full 32 bit long
    pcInt7bit(xx & 0x7F);
    
    pcInt7bit( (xx >> 7) & 0x7F);
    pcInt7bit( 7 );
    pcAddByte(OP_LSHIFT);
    pcAddByte(OP_B_OR);
    
    pcInt7bit( (xx >> 14) & 0x7F);
    pcInt7bit(14);
    pcAddByte(OP_LSHIFT);
    pcAddByte(OP_B_OR);

    pcInt7bit( (xx >> 21) & 0x7F);
    pcInt7bit(21);
    pcAddByte(OP_LSHIFT);
    pcAddByte(OP_B_OR);

    pcInt7bit( (xx >> 28) & 0x7F);
    pcInt7bit(28);
    pcAddByte(OP_LSHIFT);
    pcAddByte(OP_B_OR);

  }
  if (neg) {
    pcAddByte(OP_NEG);
  }

}


int pcCount() {
  return pcNext;
}


// ---

typedef struct {
  char *name;
  int codePos;
} MapValue;


static MapValue mapData[MAP_SIZE];
static int mapNext=0;

int mapLookupMapIndex (char *name) {
  for (int i=0; i<mapNext; i++) {
    if (!strcmp(mapData[i].name, name)) return i;
  }
  return -1;
}

void mapAddPos (char *name, int codePos) {
  int mapIndex=mapLookupMapIndex(name);
  if (mapIndex >= 0) {
    //LOG("mapAddPos: overwriting mapIndex",mapIndex);
    // overwrite existing def
    (mapData+mapIndex)->codePos=codePos;
    return;
  }
  // add new
  if (mapNext >= MAP_SIZE) {
    Serial.println(F("mapAddPos: no more space"));
    setAbortCodeExecution();
    return ;
  }
  mapData[mapNext].name=name;
  mapData[mapNext].codePos=codePos;
  mapNext++;
}

int mapLookupPos (char *name) {
  for (int i=0; i<mapNext; i++) {
    if (!strcmp(mapData[i].name, name)) return mapData[i].codePos;
  }
  return -1;
}

int mapCount () {
  return mapNext;
}

char *mapGetName (int pos) {
  return mapData[pos].name;
}

int mapGetLength (int pos) {
  if (pos < mapNext-1) {
    return mapData[pos+1].codePos - mapData[pos].codePos;
  } else {
    return pcNext-mapData[pos].codePos;
  }
}
