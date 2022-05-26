#ifndef RFCOMMON_HH
#define RFCOMMON_HH


#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

typedef unsigned char byte;

#define NULL    0


#define VERSION "v0.1.8"


/// Input.cpp
#define INPUT_BUF_SIZE      400
#define INPUT_TOKEN_COUNT   100

// Stacks.hh / Input.cpp
#define LOCAL_VARIABLE_COUNT    8

// Stacks.cpp
#define DATA_STACK_SIZE     30
#define CALL_STACK_SIZE     20


// Storage.hh

#define P_STRING_SIZE     2000
#define P_CODE_SIZE       3000


// Storage.cpp
#define MAP_SIZE          200
#define P_CODE_MAX_SIZE   127




void LOG2 (int code, long a, long b);

void setAbortCodeExecution ();




#endif
