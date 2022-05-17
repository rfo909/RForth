#ifndef RFCOMMON_HH
#define RFCOMMON_HH


#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;

#define NULL    0


// Input.cpp
#define INPUT_BUF_SIZE      100
#define INPUT_TOKEN_COUNT   25

// Stacks.hh / Input.cpp
#define LOCAL_VARIABLE_COUNT    8

// Stacks.cpp
#define DATA_STACK_SIZE     16
#define CALL_STACK_SIZE     10

// Storage.hh
#define P_STRING_SIZE     100
#define P_CODE_SIZE       200 

// Storage.cpp
#define MAP_SIZE          10

// .ino
//#define ENABLE_DISASSEMBLER 


void err (char *s, int i);

// #define LOG(a,b)  log(a,b)

#define LOG(a,b)
void log (char *s, int i);



#endif
