#ifndef RFCOMMON_HH
#define RFCOMMON_HH


#include <string.h>
#include <stdlib.h>

typedef unsigned char byte;

#define NULL    0


#define VERSION "0.1.3"


// Input.cpp
#define INPUT_BUF_SIZE      160
#define INPUT_TOKEN_COUNT   30

// Stacks.hh / Input.cpp
#define LOCAL_VARIABLE_COUNT    6

// Stacks.cpp
#define DATA_STACK_SIZE     15
#define CALL_STACK_SIZE     10


// Storage.hh

#define P_STRING_SIZE     200
#define P_CODE_SIZE       400 

// Storage.cpp
#define MAP_SIZE          60



void ERR1 (int code, long a);
void ERR2 (int code, long a, long b);
void WARN2 (int code, long a, long b);
void halt();

void LOG2 (int code, long a, long b);

void printOpName (int opCode);

#define ERR_UNKNOWN_OP        1
#define FUNC_csPop              2
#define FUNC_csPeek            3
#define FUNC_inpLocalVariableAdd 4
#define FUNC_csPush   5
#define FUNC_mapAddPos 6
#define FUNC_pcAddByte 7
#define FUNC_psAddChar 8
#define ERR_dsGet_not_number 9
#define ERR_dsPop_not_number  10
#define FUNC_dsPopValue 11
#define ERR_dsPeek_not_number 12
#define FUNC_inpAddToken 13
#define FUNC_inpAddChar 14
#define FUNC_dsPeekValue 15
#define FUNC_dsPushValue 16
#define ERR_INVALID_TYPE_CAST 17




#endif
