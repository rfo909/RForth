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


#define VERSION "v0.2.6"


#ifdef ARDUINO_AVR_MEGA
  #define INPUT_BUF_SIZE      400
  #define INPUT_TOKEN_COUNT   100
#else
  #define INPUT_BUF_SIZE      150
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
  #define PSDATA_SIZE       1500
  #define PCDATA_SIZE       3500
#else
  #define PSDATA_SIZE       250
  #define PCDATA_SIZE       350
#endif

// Storage.cpp
#ifdef ARDUINO_AVR_MEGA
  #define MAP_SIZE          200
  #define CONST_MAP_SIZE    30
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

// ADDR types (4 bits, up to 0x0F
#define ATYP_SYMBOL     0x01
#define ATYP_BLOB       0x02  // length encoded as three bytes in start



void setAbortCodeExecution ();




#endif
