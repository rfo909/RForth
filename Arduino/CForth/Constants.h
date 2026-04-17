#include <string.h>

#define MONITOR_C_STACK

typedef unsigned int Word;
typedef unsigned char Byte;

typedef struct {
  char *name;
  void (*f) ();
} OpCode;

#define  NULL ((void *) 0)

#define DSTACK_SIZE     16
#define RSTACK_SIZE     16

#define CODE_SEGMENT_SIZE     350
#define DATA_SEGMENT_SIZE     700

// dictionary entry types
#define DE_TYPE_NORMAL      0
#define DE_TYPE_IMMEDIATE   1
#define DE_TYPE_CONSTANT    2

typedef Byte Boolean;

#define NUM_TAGS_REFS     5
  // Colon compiler supports tags for loops and conditionals, combined with jmp and jmp?
  //
  // Define tags as "/0" to "/4" and resolve them into addresses using "&0" to "&4", before jmp or jmp?
  // This number is the total allowed tags and references within the main body of a colon word. Since we
  // don't have immediate op-codes, IF and loops must be implemented in Forth.


#define OP_BVAL  2
#define OP_CVAL  3
#define OP_RET   4
#define OP_JMP   5
#define OP_COND_JMP 6
#define OP_BLOB  7
#define OP_ZERO  8
#define OP_ONE   9

#define BYTE_CALL_BIT   0x80
#define CALL_BIT    0x8000      
  // high bit of 2 Byte word, indicates the remaining 15 bits is an 
  // address that is to be auto-called
#define DATA_BIT    0x4000

#define ADDR_CODE_MASK      0x3FFF
#define ADDR_DATA_MASK      0x3FFF


#define MAX_WORD_LENGTH   16

#define READC_ECHO  true

#define WORD_INVALID      0xFFFF


// CForth.ino

void sPrint (char *msg);
void sPrintWord (Word word);
void sPrintByte (Byte b);
void sPrintDouble (double d);
void sPrintln ();

void setHasError();
Byte hasError();

// ------------------------------------------------------
// Dict.cpp
// ------------------------------------------------------

Word getDeNamePtr();
Byte getDeType();
Word getDeAddress();
Word getDeNextPtr();

void setDeType(Byte b);
void setDeAddress(Word w);

Word getDictionaryHead();
void dictEntryFetch (Word ptr);
void dictEntrySave();
void dictCreate (char *newWord);
Boolean dictLookup (char *word);
Boolean dictLookupByAddr (Word addr);

// ------------------------------------------------------
// Static.cpp
// ------------------------------------------------------


Word staticDataSize();
Byte staticDataRead (Word pos);


// ------------------------------------------------------
// Mem.cpp
// ------------------------------------------------------

void memInit(void);
void compileOut (Byte b);
Word getCodeNext();
void setCodeNext (Word w);
void startCompile();
Word getCompileNext();
void confirmCompile();

Word generateCodeAddress (Word ptr);
Word generateDataAddress (Word ptr);
Word generateCallAddress (Word ptr);
Word HERE();
Word codeHERE();
void dataAllot (Word count);
void codeAllot (Word count);

void writeByte (Word addr, Byte b);
Byte readByte (Word addr);
Word readWord (Word addr);
void writeWord (Word addr, Word value);


void memDump();
void memCodeExport();

Byte readByteFast (Word addr);

// ------------------------------------------------------
// Util.cpp
// ------------------------------------------------------

void printChar (Byte ch);
void printStr (Word ptr);
Boolean mixedStreq (Word strPtr, char *s);
Byte myAtoi (char *nextWord, int *target);
void checkCStackSize();
Word getCStackMaxSize();