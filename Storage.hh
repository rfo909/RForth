#ifndef RF_STORAGE_HH
#define RF_STORAGE_HH

#include "Common.hh"

// string data (in RAM) - session persistent

int psCount();
char *psStringAtPos (int pos);

// binary data (in RAM) - session persistent

int to7BitPush (int b);
void pcInt7bit (int b);
void pcInt (long i);


void pcAddByte (byte b);
int pcGetMark();
void pcResetToMark(int mark);
int pcChopInt ();
byte *pcGetPointer (int pos);
int pcGetLocalPos();
void pcSetByteLocal (int pos, byte b);
int pcCount();

// maps (in RAM) - session persistent

//void mapAddStringPos (char *str, int strPos);
int  mapGetStringPos (char *str);
int  mapGetOrAddString (char *str);

void mapAddCompiledWord (char *name, int codePos);
int mapLookupCodePos (char *name);
int mapGetWordCount ();
char *mapGetWordName (int pos);


#endif
