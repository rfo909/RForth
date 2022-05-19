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
 
  #define P_STRING_SIZE     70
  #define P_CODE_SIZE       170 

#else

  #define P_STRING_SIZE     70
  #define P_CODE_SIZE       200 

#endif

// Storage.cpp
#define MAP_SIZE          30



void err (char *s, long a);
void err2 (char *s, long a, long b);
void warn (char *s, long a, long b);
void halt();

#define LOG(a,b)  log(a,b)
//#define LOG(a,b)

void log (char *s, int i);



#endif
