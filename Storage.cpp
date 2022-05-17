#include "Storage.hh"
#include "OpCodes.hh"

// ps = persistent string data

static char psData[P_STRING_SIZE];
static int psStart=0;
static int psNext=0;

void psAddChar (char c) {
  if (psNext >= P_STRING_SIZE) err("psAddChar",psNext);
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


// pc = persistent code data 

static byte pcData[P_CODE_SIZE];
static int pcStart=0;
static int pcNext=0;

void pcAddByte (byte b) {
  LOG("pcAddByte",b);
  if (pcNext >= P_CODE_SIZE) err("pcAddByte", pcNext);
  pcData[pcNext++]=b;
}

int pcGetMark() {
  LOG("pcGetMark",pcStart);
  return pcStart;
}

void pcResetToMark(int mark) {
  LOG("pcResetToMark", mark);
  pcStart=mark;
  pcNext=mark;
}

int pcChopInt () {
  LOG("pcChopInt",pcStart);
  int i=pcStart;
  pcStart=pcNext;
  return i;
}

byte *pcGetPointer (int pos) {
  LOG("pcGetPointer",pos);
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
  LOG("pcInt7bit", b);
  return pcAddByte(to7BitPush(b));
}



void pcInt (int i) {
  LOG("pcInt",i);
  bool neg=(i<0);
  if (neg) i=-i;
  
  if(i<127) {
    pcInt7bit(i);
  } else if (i < 16384) { // 14 bits
    pcInt7bit( (i & 0x1fc0) >> 7);
    
    pcInt7bit(7); // arg for LSHIFT
    pcAddByte(OP_LSHIFT);
    
    pcInt7bit(i & 0x7f);
    pcAddByte(OP_B_OR);  
  } else {
    pcInt7bit( i & 0x7F);
    pcInt7bit( (i >> 7) & 0x7F);
    
    pcInt7bit( 7 );
    pcAddByte(OP_LSHIFT);
    
    pcInt7bit( (i >> 14) & 0x7F);
    
    pcInt7bit(14);
    pcAddByte(OP_LSHIFT);
    
    pcAddByte(OP_B_OR);
    pcAddByte(OP_B_OR);
  }
  if (neg) {
    pcAddByte(OP_NEG);
  }

}



// ---

typedef struct {
  char *name;
  int codePos;
} MapValue;


static MapValue mapData[MAP_SIZE];
static int mapNext=0;

void mapAddPos (char *name, int codePos) {
  LOG("mapAddPos",codePos);
  int oldPos=mapLookupPos(name);
  if (oldPos >= 0) {
    // overwrite existing def
    mapData[oldPos].codePos=codePos;
    return;
  }
  // add new
  if (mapNext >= MAP_SIZE) err("mapAddPos",mapNext);
  mapData[mapNext].name=name;
  mapData[mapNext].codePos=codePos;
  mapNext++;
}

int mapLookupPos (char *name) {
  LOG("mapLookupPos",mapNext);
  for (int i=0; i<mapNext; i++) {
    if (!strcmp(mapData[i].name, name)) return mapData[i].codePos;
  }
  return -1;
}

int mapCount () {
  LOG("mapCount",mapNext);
  return mapNext;
}

char *mapGetName (int pos) {
  LOG("mapGetName", pos);
  return mapData[pos].name;
}
