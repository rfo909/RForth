#ifndef RFCOMMON_HH
#define RFCOMMON_HH


#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;

#define NULL    0


#define VERSION "0.1.1"

// .ino
// #define ENABLE_DISASSEMBLER 
  // enabling costs us almost 200 bytes of SRAM due to 
  // message strings on heap inside the disassemble() function.


// Input.cpp
#define INPUT_BUF_SIZE      200
#define INPUT_TOKEN_COUNT   30

// Stacks.hh / Input.cpp
#define LOCAL_VARIABLE_COUNT    8

// Stacks.cpp
#define DATA_STACK_SIZE     20
#define CALL_STACK_SIZE     10


// Storage.hh

#ifdef ENABLE_DISASSEMBLER
 
  #define P_STRING_SIZE     100
  #define P_CODE_SIZE       200 

#else

  #define P_STRING_SIZE     200
  #define P_CODE_SIZE       300 

#endif

// Storage.cpp
#define MAP_SIZE          40



void err (char *s, int i);

#define LOG(a,b)  log(a,b)
//#define LOG(a,b)

void log (char *s, int i);



#endif
