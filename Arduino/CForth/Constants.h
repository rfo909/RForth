

typedef unsigned int Word;
typedef unsigned char Byte;

typedef struct {
  char *name;
  void (*f) ();
} OpCode;



#define DSTACK_SIZE     16
#define RSTACK_SIZE     16

#define CODE_SEGMENT_SIZE     200
#define DATA_SEGMENT_SIZE     50

#define DE_TYPE_NORMAL      1
#define DE_TYPE_IMMEDIATE   2
#define DE_TYPE_CONSTANT    3  // value stored in address field

typedef struct DictEntry {
  char *name;
  Byte type;
  Word address;

  struct DictEntry *next;
} DictEntry;


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

void clearHasError();
void setHasError();
Byte hasError();

// Stacks.c

void stacksInit(void);
void push (Word v);
Word pop ();
Word pick (Word n);
void rpush (Word v);
Word rpop ();

void dStackShow();
void dStackClear();

// Mem.c
void memInit(void);
void compileOut (Byte b);
Word getCodeNext();
void setCodeNext (Word w);
void initCompileNext();
Word getCompileNext();

Word generateCodeAddress (Word ptr);
Word generateDataAddress (Word ptr);
Word generateCallAddress (Word ptr);
Word addrHERE();
void allot (Word count);
void writeByte (Word addr, Byte b);
Byte readByte (Word addr);
Word readWord (Word addr);
void writeWord (Word addr, Word value);

Byte codeSegmentGet (Word pos);
void codeSegmentSet (Word pos, Byte val);