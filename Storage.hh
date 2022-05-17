#ifndef RF_STORAGE_HH
#define RF_STORAGE_HH

#include "Common.hh"


void psAddChar (char c);
void psAddStr (char *str);
char *psChop ();
int to7BitPush (int b);
void pcInt7bit (int b);
void pcInt (int i);


void pcAddByte (byte b);
int pcGetMark();
void pcResetToMark(int mark);
byte *pcChop ();
int pcChopInt ();
byte *pcGetPointer (int pos);
int pcGetLocalPos();
void setLocalPosByte (int pos, byte b);

void mapAddPos (char *name, int codePos);
int mapLookupPos (char *name);
int mapCount ();
char *mapGetName (int pos);


#endif
