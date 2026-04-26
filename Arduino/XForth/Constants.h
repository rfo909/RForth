#include <Arduino.h>

// signed types
typedef int SWord;
typedef char SByte;
typedef long SLong;

// unsigned types
typedef unsigned int Word;
typedef unsigned char Byte;

typedef void (*XT) (Word);
typedef void (*OP) (void);

typedef Byte Boolean;

#define CELLSIZE	2
#define DOUBLESIZE	4

#define  NULL ((void *) 0)

#define DSTACK_SIZE     16
#define RSTACK_SIZE     16
#define XSTACK_SIZE     10

#define DATA_SEGMENT_SIZE     800
#define CODE_SEGMENT_SIZE     600

// #define DATA_SEGMENT_SIZE     (256 + ((RAMEND - RAMSTART) / 8))   // min 512 bytes on 2K SRAM
// #define CODE_SEGMENT_SIZE     (RAMEND - RAMSTART - DATA_SEGMENT_SIZE - 880 - 200)
  // global variables apart from the data segment is around 830 bytes
  // adjust if problems

#define DATA_BIT    0x8000

// use this to remove DATA_BIT
#define DATA_MASK      0x7FFF

#define MAX_WORD_LENGTH   20

#define READC_ECHO  true

#define WORD_INVALID      0xFFFF
#define BYTE_INVALID      0xFF
