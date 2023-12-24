#include "Storage.hh"
#include "OpCodes.hh"

// ps = persistent string data

static char psData[PSDATA_SIZE];
static int psNext=0;

// traverse psData looking for string, returns index if found, otherwise -1
int  psSearch (char *str) {
  int pos=0;
  while (pos < psNext) {    
    if (!strcmp(psData+pos, str)) return pos;
    int chars=strlen(psData+pos);
    pos=pos+chars+1; // past NULL
  }
  return -1;
}

int  psGetOrAddString (char *str) {
  int pos=psSearch(str);
  if (pos >= 0) return pos;
  int len=strlen(str);
  if (PSDATA_SIZE - psNext < len+1) {
    Serial.println(F("psGetOrAddString: out of space"));
    return -1; 
  }
  pos=psNext;
  strcpy((psData+psNext), str);
  psNext+=len;
  psData[psNext++]='\0';
  return pos;
}

int psCount() {
  return psNext;
}

char *psGetStringPointer (int strPos) {
  return psData+strPos;
}



// pc = persistent code data 

static byte pcData[PCDATA_SIZE];
static int pcStart=0;
static int pcNext=0;

void pcAddByte (byte b) {
  if (pcNext >= PCDATA_SIZE) {
    Serial.println(F("psAddByte: no more space (PCDATA_SIZE)"));
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

byte *pcGetCodePointer (int pos) {
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


// patch 7 bit address relative to pcStart, for forward JMP's (if, loop)
void pcSetByteLocal7bit (int localPos, byte value) {
  pcData[pcStart+localPos] = to7BitPush(value & B01111111);
}



void pcInt (long i) {
  bool neg=(i<0);
  if (neg) i=-i;
  
  if(i<128) { // 7 bits
    pcInt7bit(i);
  } else if (i < 16384) { // 14 bits
    pcInt14bit( (unsigned int) i);
  } else if (i < 2097152L) { // 21 bits
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

// Map over compiled words



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

void mapAddCompiledWord (int strPos, int codePos) {

  // check if already defined
  int mapIndex=mapGetCompiledWordIndex(strPos);
  
  if (mapIndex >= 0) {
    // overwrite existing def in map
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

int mapLookupCodePos (int strPos) {
  
  if (strPos < 0) {
    Serial.print(F("mapLookupCodePos: fails for name "));
    Serial.println(psData+strPos);
    return -1;
  }
  
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




typedef struct {
  int nameStrPos;
  int valueStrPos;
} MapConst;

static MapConst constData[CONST_MAP_SIZE];
static int constNext=0;

char *constLookup (char *name) {
  for (int i=0; i<constNext; i++) {
    char *s=psGetStringPointer((constData+i)->nameStrPos);
    if (!strcmp(name,s)) {
      return psGetStringPointer((constData+i)->valueStrPos);
    }
  }
  return NULL;
}

void constAdd (char *name, char *value) {
  int n=psGetOrAddString(name);
  int v=psGetOrAddString(value);
  if (constNext >= CONST_MAP_SIZE) {
    Serial.println(F("Const map overflow"));
    return;
  }
  MapConst *x=constData+constNext;
  x->nameStrPos=n;
  x->valueStrPos=v;
  constNext++;
}

int constGetCount() {
  return constNext;
}

char *constGetName (int pos) {
  MapConst *x=constData+pos;
  return psGetStringPointer(x->nameStrPos);
}

char *constGetValue (int pos) {
  MapConst *x=constData+pos;
  return psGetStringPointer(x->valueStrPos);
}



// DUMP HEX

int outCounter=0;

void out (int i) {
  Serial.print("0x");
  if (i < 16) {
    Serial.print("0");
  }
  Serial.print(i,HEX);
  Serial.print(",");
  outCounter++;
  if (outCounter % 16 == 0) Serial.println();
}


void dumpHex () {
  outCounter=0;
  
  for (int i=0; i<psNext; i++) {
    byte b=psData[i];
    out(b);
  }
  out(0);
  
  for (int i=0; i<mapNext; i++) {
    MapValue *ptr=mapData+i;
    int strPos=ptr->strPos;
    int codePos=ptr->codePos;
    int a=( ((strPos>>7) & 0x7F) | 0x80 );
    int b=( (strPos & 0x7F) | 0x80 );
    out(a);
    out(b);

    for (int j=codePos; true; j++) {
      out(pcData[j]);
      if (pcData[j]==0) break;
    }
  }
  out(0);
  Serial.println();
}
