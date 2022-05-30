#ifndef RF_STORAGE_HH
#define RF_STORAGE_HH

#include "Common.hh"

// string data (in RAM) - session persistent

int  psSearch (char *str);
int  psGetOrAddString (char *str);
int  psCount();
char *psGetStringPointer (int pos);

// binary data (in RAM) - session persistent

void pcInt7bit (int b);
void pcInt14bit (unsigned int i);
void pcInt (long i);


void pcAddByte (byte b);
int pcGetMark();
void pcResetToMark(int mark);
int pcChopInt ();
byte *pcGetCodePointer (int pos);
int pcGetLocalPos();

void pcSetBytesLocalU14 (int localPos, unsigned int value);
int pcCount();

// word map (in RAM) - session persistent

void mapAddCompiledWord (int strPos, int codePos);
int mapLookupCodePos (int strPos);
int mapGetWordCount ();
char *mapGetWordName (int pos);

void dumpHex ();


#endif
