#include "Storage.hh"
#include "OpCodes.hh"

// ps = persistent string data

static char psData[P_STRING_SIZE];
static int psStart=0;
static int psNext=0;

static void psAddChar (char c) {
  if (psNext >= P_STRING_SIZE) {
    Serial.println(F("psAddChar: no more space"));
    setAbortCodeExecution();

    return;
  }
  psData[psNext++]=c; 
}

static void psAddStr (char *str) {
  for (int i=0; i<strlen(str); i++) psAddChar(str[i]);
}

static int psChopInt() {
  psAddChar('\0');
  int pos = psStart;
  psStart=psNext;
  return pos;
}


int psCount() {
  return psNext;
}

char *psStringAtPos (int strPos) {
  return psData+strPos;
}



// pc = persistent code data 

static byte pcData[P_CODE_SIZE];
static int pcStart=0;
static int pcNext=0;

void pcAddByte (byte b) {
  if (pcNext >= P_CODE_SIZE) {
    Serial.println(F("psAddByte: no more space (P_CODE_SIZE)"));
    setAbortCodeExecution();
    return;
  }
  if (pcNext-pcStart >= P_CODE_MAX_SIZE) {
    Serial.println(F("psAddByte: function too long (P_CODE_MAX_SIZE)"));
    setAbortCodeExecution();
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

byte *pcGetPointer (int pos) {
  return pcData+pos;
}


int pcGetLocalPos() {
  return pcNext-pcStart;
}




static int to7BitPush (int b) {
  return 0x80 | (b & 0x7f);
}

void pcInt7bit (int b) {
  pcAddByte(to7BitPush(b));
}

void pcInt14bit (unsigned int b) {
  // high bits
  unsigned int high=(b>>7) & B01111111;
  unsigned int low=b & B01111111;

  pcInt7bit(high);
  pcInt7bit(low);
  pcAddByte(OP_U14);
}


// patch 14 bit address relative to pcStart, for forward JMP's (if, loop)
void pcSetBytesLocalU14 (int localPos, unsigned int value) {
  
  pcData[pcStart+localPos] = to7BitPush((value >> 7) & B01111111);
  pcData[pcStart+localPos+1] = to7BitPush(value & B01111111);
}



void pcInt (long i) {
  bool neg=(i<0);
  if (neg) i=-i;
  
  if(i<127) {
    pcInt7bit(i);
  } else if (i < 16384) { // 14 bits
    pcInt14bit( (unsigned int) i);
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

// Indexing the strings in temporary string store (above)

typedef struct {
  char *str;
  int strPos;
} StrValue;

static StrValue strData[MAP_SIZE];
static int strMapNext = 0;

static void mapAddStringPos (int strPos) {
  char *str=psData + strPos;
  
  if (strMapNext >= MAP_SIZE) {
    Serial.println(F("mapAddStringPos: out of space"));
    setAbortCodeExecution();
    return;
  }
  (strData+strMapNext)->str=str;
  (strData+strMapNext)->strPos=strPos;
  strMapNext++;
}

int mapGetOrAddString (char *str) {
  int strPos=mapGetStringPos(str);
  if (strPos < 0) {
    psAddStr(str);
    strPos=psChopInt();

    mapAddStringPos(strPos);
  }
  return strPos;
}


int mapGetStringPos (char *str) {
  
  for (int i=0; i<strMapNext; i++) {
    
    if (!strcmp((strData+i)->str, str)) {
      return (strData+i)->strPos;
    }
  }

  return -1;
}




typedef struct {
  int strPos;
  int codePos;
} MapValue;


static MapValue mapData[MAP_SIZE];
static int mapNext=0;

static int mapGetCompiledWordIndex (int strPos) {
  for (int i=0; i<mapNext; i++) {
    if ((mapData+i)->strPos==strPos) return i;
  }
  return -1;
}

void mapAddCompiledWord (char *name, int codePos) {

  int strPos=mapGetOrAddString(name);

  // check if already defined
  int mapIndex=mapGetCompiledWordIndex(strPos);
  
  if (mapIndex >= 0) {
    // overwrite existing def
    (mapData+mapIndex)->codePos=codePos;
    return;
  }
  // add new
  if (mapNext >= MAP_SIZE) {
    Serial.println(F("mapAddCompiledWord: out of space"));
    setAbortCodeExecution();
    return ;
  }
  (mapData+mapNext)->strPos=strPos;
  (mapData+mapNext)->codePos=codePos;
  mapNext++;
}

int mapLookupCodePos (char *name) {
  
  int strPos=mapGetStringPos(name);

  if (strPos < 0) return -1;
  
  for (int i=0; i<mapNext; i++) {
    if ((mapData+i)->strPos==strPos) return (mapData+i)->codePos;
  }
  return -1;
}

int mapGetWordCount () {
  return mapNext;
}

char *mapGetWordName (int mapDataPos) {
   int strPos = (mapData+mapDataPos)->strPos;
   return psData+strPos;
}
