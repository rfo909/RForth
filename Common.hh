#ifndef RFCOMMON_HH
#define RFCOMMON_HH


#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

typedef unsigned char byte;

#define NULL    0

// Will be using NANO_EVERY for RForth, as NANO CLASSIC is too small SRAM,
// and MEGA 2560 isn't breadboard friendly.

#ifndef AVR_NANO_EVERY
  #ifdef ARDUINO_AVR_NANO_EVERY
    #define AVR_NANO_EVERY
  #endif
#endif

//#ifndef AVR_NANO_EVERY
//  #ifdef AVR_NANO_EVERY2560
//    #define AVR_NANO_EVERY
//  #endif
//#endif


#define VERSION "v0.2.8"


#ifdef AVR_NANO_EVERY
  #define INPUT_BUF_SIZE      400
  #define INPUT_TOKEN_COUNT   100
#else
  #define INPUT_BUF_SIZE      150
  #define INPUT_TOKEN_COUNT   50
#endif


#define LOCAL_VARIABLE_COUNT    4


#ifdef AVR_NANO_EVERY
  #define DATA_STACK_SIZE     30
  #define CALL_STACK_SIZE     20
#else
  #define DATA_STACK_SIZE     10
  #define CALL_STACK_SIZE     10
#endif

// Storage.hh
#ifdef AVR_NANO_EVERY
  #define PSDATA_SIZE       1200
  #define PCDATA_SIZE       2000
#else
  #define PSDATA_SIZE       150
  #define PCDATA_SIZE       370
#endif

// Storage.cpp
#ifdef AVR_NANO_EVERY
  #define MAP_SIZE          150
  #define CONST_MAP_SIZE    50
#else
  #define MAP_SIZE          40
  #define CONST_MAP_SIZE    20
#endif


#define P_CODE_MAX_SIZE   127


// ADDR locations (4 bits, up to 0x0F)
#define ALOC_OC_STR       0x01   // (Storage.c) session persistent string storage in RAM
#define ALOC_OC_BIN       0x02   // (Storage.c) session persistent binary storage in RAM
#define ALOC_OC_PROGMEM   0x03   // static progmem binary array PROGMEM_DATA
#define ALOC_OC_EEPROM    0x04

// ADDR types (4 bits, up to 0x0F)
#define ATYP_SYMBOL     0x01
#define ATYP_BLOB       0x02  // length encoded as single byte in start, data length is P_CODE_MAX_SIZE
#define ATYP_CONS       0x03  // cons cell

#define ALOC_EXT_BIT    B00001000


void setAbortCodeExecution ();




#endif
