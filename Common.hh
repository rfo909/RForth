#ifndef RFCOMMON_HH
#define RFCOMMON_HH


#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

typedef unsigned char byte;

#define NULL    0

#ifndef ARDUINO_AVR_MEGA
  #ifdef ARDUINO_AVR_MEGA2560
    #define ARDUINO_AVR_MEGA
  #endif
#endif


#define VERSION "v0.1.8"


#ifdef ARDUINO_AVR_MEGA
  #define INPUT_BUF_SIZE      400
  #define INPUT_TOKEN_COUNT   100
#else
  #define INPUT_BUF_SIZE      100
  #define INPUT_TOKEN_COUNT   80
#endif


#define LOCAL_VARIABLE_COUNT    6


#ifdef ARDUINO_AVR_MEGA
  #define DATA_STACK_SIZE     30
  #define CALL_STACK_SIZE     20
#else
  #define DATA_STACK_SIZE     10
  #define CALL_STACK_SIZE     8
#endif

// Storage.hh
#ifdef ARDUINO_AVR_MEGA
  #define P_STRING_SIZE     1500
  #define P_CODE_SIZE       3000
#else
  #define P_STRING_SIZE     150
  #define P_CODE_SIZE       300
#endif

// Storage.cpp
#ifdef ARDUINO_AVR_MEGA
  #define MAP_SIZE          200
#else
  #define MAP_SIZE          50
#endif


#define P_CODE_MAX_SIZE   127


// ADDR locations (4 bits, up to 0x0F)
#define ALOC_OC_ASP       0x01   // address space
#define ALOC_OC_STR       0x02   // (Storage.c) session persistent string storage in RAM
#define ALOC_OC_BIN       0x03   // (Storage.c) session persistent binary storage in RAM
#define ALOC_OC_PROGMEM   0x04   // static progmem binary array PROGMEM_DATA
#define ALOC_OC_EEPROM    0x05

// ADDR types (4 bits, up to 0x0F
#define ATYP_BYTE       0x01
#define ATYP_UINT       0x02
#define ATYP_BLOB       0x03  // length encoded as three bytes in start
#define ATYP FUNC       0x04
#define ATYP_SYMBOL     0x05


#define LOG_SRC_STORAGE     1


void LOG (int logSrcId, const __FlashStringHelper *str);
void LOG (int logSrcId, char *s);
void LOG (int logSrcId, char c);
void LOG (int logSrcId, byte i);
void LOG (int logSrcId, int i);
void LOG (int logSrcId, unsigned int i);
void LOG (int logSrcId, long i);
void LOG (int logSrcId, unsigned long i);
void LOG_newline (int logSrcId);

void setAbortCodeExecution ();




#endif
