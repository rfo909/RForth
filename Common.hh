#ifndef RFCOMMON_HH
#define RFCOMMON_HH


#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

typedef unsigned char byte;

#define NULL    0


#define VERSION "0.1.5"


// Input.cpp
#define INPUT_BUF_SIZE      200
#define INPUT_TOKEN_COUNT   100

// Stacks.hh / Input.cpp
#define LOCAL_VARIABLE_COUNT    4

// Stacks.cpp
#define DATA_STACK_SIZE     15
#define CALL_STACK_SIZE     10


// Storage.hh

#define P_STRING_SIZE     300
#define P_CODE_SIZE       400 


// Storage.cpp
#define MAP_SIZE          40
#define P_CODE_MAX_SIZE   127


void ERR1 (int code, long a);
void ERR2 (int code, long a, long b);
void WARN2 (int code, long a, long b);
void halt();

void LOG2 (int code, long a, long b);

void setAbortCodeExecution ();




#endif
