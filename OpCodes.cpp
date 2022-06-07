#include "OpCodes.hh"

#include "Common.hh"

static const char ADD[] PROGMEM = "+";
static const char SUB[] PROGMEM = "-";
static const char MUL[] PROGMEM = "*";
static const char DIV[] PROGMEM = "/";
static const char MOD[] PROGMEM = "%";

static const char LSHIFT[] PROGMEM = "<<";
static const char RSHIFT[] PROGMEM = ">>";

static const char B_OR[] PROGMEM = "|";
static const char B_AND[] PROGMEM = "&";
static const char B_NOT[] PROGMEM = "!";

static const char GT[] PROGMEM = ">";
static const char LT[] PROGMEM = "<";
static const char GE[] PROGMEM = ">=";
static const char LE[] PROGMEM = "<=";
static const char EQ[] PROGMEM = "==";
static const char NE[] PROGMEM = "!=";

static const char AND[] PROGMEM = "and";
static const char OR[] PROGMEM = "or";
static const char NOT[] PROGMEM = "not";


static const char SYS_RETURN[] PROGMEM = "sys:return";
static const char SYS_READ[] PROGMEM = "read";
static const char SYS_WRITE[] PROGMEM = "write";


static const char SYS_DUP[] PROGMEM = "dup";
static const char SYS_POP[] PROGMEM = "pop";
static const char SYS_NEG[] PROGMEM = "neg";

static const char T_BYTE[] PROGMEM = ":byte";
static const char T_INT[] PROGMEM = ":int";
static const char T_UINT[] PROGMEM = ":uint";
static const char T_LONG[] PROGMEM = ":long";
static const char T_ULONG[] PROGMEM = ":ulong";

static const char SYS_MILLIS[] PROGMEM = "millis";
static const char EE_READ[] PROGMEM = "ee:read";
static const char EE_WRITE[] PROGMEM = "ee:write";
static const char EE_LENGTH[] PROGMEM = "ee:length";

static const char _NULL[] PROGMEM = "null";

static const char T_SYM[] PROGMEM = ":sym";
static const char T_ADDR[] PROGMEM = ":addr";

static const char SYS_ABORT[] PROGMEM = "abort";
static const char T_BOOL[] PROGMEM = ":bool";

static const char SYS_ADDR[] PROGMEM = "sys:addr";

static const char SPI_BEGIN[] PROGMEM = "spi:begin";
static const char SPI_TRANSFER[] PROGMEM = "spi:transfer";
static const char SPI_END[] PROGMEM = "spi:end";

static const char NOP[] PROGMEM = "nop";

static const char TWI_BEGIN[] PROGMEM = "twi:begin";
static const char TWI_END[] PROGMEM = "twi:end";
static const char TWI_SETCLOCK[] PROGMEM = "twi:setclock";
static const char TWI_BEGIN_TR[] PROGMEM = "twi:tr:begin";
static const char TWI_END_TR[] PROGMEM = "twi:tr:end";
static const char TWI_WRITE[] PROGMEM = "twi:write";
static const char TWI_REQUEST[] PROGMEM = "twi:request";
static const char TWI_READ[] PROGMEM = "twi:read";


int lookupSymbol (char *sym) {
  if (!strcmp_P(sym,ADD)) return OP_ADD;
  if (!strcmp_P(sym,SUB)) return OP_SUB;
  if (!strcmp_P(sym,MUL)) return OP_MUL;
  if (!strcmp_P(sym,DIV)) return OP_DIV;
  if (!strcmp_P(sym,MOD)) return OP_MOD;
  if (!strcmp_P(sym,LSHIFT)) return OP_LSHIFT;
  if (!strcmp_P(sym,RSHIFT)) return OP_RSHIFT;
  if (!strcmp_P(sym,B_OR)) return OP_B_OR;
  if (!strcmp_P(sym,B_AND)) return OP_B_AND;
  if (!strcmp_P(sym,B_NOT)) return OP_B_NOT;
  if (!strcmp_P(sym,GT)) return OP_GT;
  if (!strcmp_P(sym,LT)) return OP_LT;
  if (!strcmp_P(sym,GE)) return OP_GE;
  if (!strcmp_P(sym,LE)) return OP_LE;
  if (!strcmp_P(sym,EQ)) return OP_EQ;
  if (!strcmp_P(sym,NE)) return OP_NE;
  if (!strcmp_P(sym,AND)) return OP_L_AND;
  if (!strcmp_P(sym,OR)) return OP_L_OR;
  if (!strcmp_P(sym,NOT)) return OP_L_NOT;
  
  if (!strcmp_P(sym,SYS_RETURN)) return OP_RET;
  if (!strcmp_P(sym,SYS_READ)) return OP_READ;
  if (!strcmp_P(sym,SYS_WRITE)) return OP_WRITE;
  if (!strcmp_P(sym,SYS_DUP)) return OP_DUP;
  if (!strcmp_P(sym,SYS_POP)) return OP_POP;
  if (!strcmp_P(sym,SYS_NEG)) return OP_NEG;

  if (!strcmp_P(sym,T_BYTE)) return OP_AS_BYTE;
  if (!strcmp_P(sym,T_INT)) return OP_AS_INT;
  if (!strcmp_P(sym,T_UINT)) return OP_AS_UINT;
  if (!strcmp_P(sym,T_LONG)) return OP_AS_LONG;
  if (!strcmp_P(sym,T_ULONG)) return OP_AS_ULONG;

  if (!strcmp_P(sym,SYS_MILLIS)) return OP_MILLIS;
  if (!strcmp_P(sym,EE_READ)) return OP_EE_READ;
  if (!strcmp_P(sym,EE_WRITE)) return OP_EE_WRITE;
  if (!strcmp_P(sym,EE_LENGTH)) return OP_EE_LENGTH;
  
  if (!strcmp_P(sym,_NULL)) return OP_NULL;

  if (!strcmp_P(sym,T_SYM)) return OP_AS_SYM;
  if (!strcmp_P(sym,T_ADDR)) return OP_AS_ADDR;
  if (!strcmp_P(sym,SYS_ABORT)) return OP_ABORT;
  if (!strcmp_P(sym,T_BOOL)) return OP_AS_BOOL;
  
  if (!strcmp_P(sym,SYS_ADDR)) return OP_ADDR;

  if (!strcmp_P(sym,SPI_BEGIN)) return OP_SPI_BEGIN;
  if (!strcmp_P(sym,SPI_TRANSFER)) return OP_SPI_TRANSFER;
  if (!strcmp_P(sym,SPI_END)) return OP_SPI_END;

  if (!strcmp_P(sym,NOP)) return OP_NOP; // for testing

  if (!strcmp_P(sym,TWI_BEGIN)) return OP_TWI_BEGIN; 
  if (!strcmp_P(sym,TWI_END)) return OP_TWI_END; 
  if (!strcmp_P(sym,TWI_SETCLOCK)) return OP_TWI_SETCLOCK; 
  if (!strcmp_P(sym,TWI_BEGIN_TR)) return OP_TWI_BEGIN_TR; 
  if (!strcmp_P(sym,TWI_END_TR)) return OP_TWI_END_TR; 
  if (!strcmp_P(sym,TWI_WRITE)) return OP_TWI_WRITE; 
  if (!strcmp_P(sym,TWI_REQUEST)) return OP_TWI_REQUEST; 
  if (!strcmp_P(sym,TWI_READ)) return OP_TWI_READ;

  return -1; 
}



void printOpName (const int opCode) {
  switch(opCode) {
    case OP_EOF: {
      Serial.print(F("OP_EOF"));
      return;
    }
    case OP_RET: {
      Serial.print(F("OP_RET"));
      return;
    }
    case OP_CALL: {
      Serial.print(F("OP_CALL"));
      return;
    }
    case OP_JMP: {
      Serial.print(F("OP_JMP"));
      return;
    }
    case OP_ZJMP: {
      Serial.print(F("OP_ZJMP"));
      return;
    }
    case OP_CJMP: {
      Serial.print(F("OP_CJMP"));
      return;
    }
    case OP_POP: {
      Serial.print(F("OP_POP"));
      return;
    }
    case OP_DUP: {
      Serial.print(F("OP_DUP"));
      return;
    }
    case OP_READ: {
      Serial.print(F("OP_READ"));
      return;
    }
    case OP_WRITE: {
      Serial.print(F("OP_WRITE"));
      return;
    }
    case OP_LSET: {
      Serial.print(F("OP_LSET"));
      return;
    }
    case OP_LGET: {
      Serial.print(F("OP_LGET"));
      return;
    }
    case OP_ADD: {
      Serial.print(F("OP_ADD"));
      return;
    }
    case OP_SUB: {
      Serial.print(F("OP_SUB"));
      return;
    }
    case OP_MUL: {
      Serial.print(F("OP_MUL"));
      return;
    }
    case OP_DIV: {
      Serial.print(F("OP_DIV"));
      return;
    }
    case OP_MOD: {
      Serial.print(F("OP_MOD"));
      return;
    }
    case OP_NEG: {
      Serial.print(F("OP_NEG"));
      return;
    }
    case OP_GT: {
      Serial.print(F("OP_GT"));
      return;
    }
    case OP_LT: {
      Serial.print(F("OP_LT"));
      return;
    }
    case OP_GE: {
      Serial.print(F("OP_GE"));
      return;
    }
    case OP_LE: {
      Serial.print(F("OP_LE"));
      return;
    }
    case OP_EQ: {
      Serial.print(F("OP_EQ"));
      return;
    }
    case OP_NE: {
      Serial.print(F("OP_NE"));
      return;
    }
    case OP_L_AND: {
      Serial.print(F("OP_L_AND"));
      return;
    }
    case OP_L_OR: {
      Serial.print(F("OP_L_OR"));
      return;
    }
    case OP_L_NOT: {
      Serial.print(F("OP_L_NOT"));
      return;
    }
    case OP_LSHIFT: {
      Serial.print(F("OP_LSHIFT"));
      return;
    }
    case OP_RSHIFT: {
      Serial.print(F("OP_RSHIFT"));
      return;
    }
    case OP_B_AND: {
      Serial.print(F("OP_B_AND"));
      return;
    }
    case OP_B_OR: {
      Serial.print(F("OP_B_OR"));
      return;
    }
    case OP_B_NOT: {
      Serial.print(F("OP_B_NOT"));
      return;
    }
    case OP_LSET0: {
      Serial.print(F("OP_LSET0"));
      return;
    }
    case OP_LSET1: {
      Serial.print(F("OP_LSET1"));
      return;
    }
    case OP_LSET2: {
      Serial.print(F("OP_LSET2"));
      return;
    }
    case OP_LSET3: {
      Serial.print(F("OP_LSET3"));
      return;
    }
    case OP_LGET0: {
      Serial.print(F("OP_LGET0"));
      return;
    }
    case OP_LGET1: {
      Serial.print(F("OP_LGET1"));
      return;
    }
    case OP_LGET2: {
      Serial.print(F("OP_LGET2"));
      return;
    }
    case OP_LGET3: {
      Serial.print(F("OP_LGET3"));
      return;
    }
    case OP_AS_BYTE: {
      Serial.print(F("OP_AS_BYTE"));
      return;
    }
    case OP_AS_INT: {
      Serial.print(F("OP_AS_INT"));
      return;
    }
    case OP_AS_UINT: {
      Serial.print(F("OP_AS_UINT"));
      return;
    }
    case OP_AS_LONG: {
      Serial.print(F("OP_AS_LONG"));
      return;
    }
    case OP_AS_ULONG: {
      Serial.print(F("OP_AS_ULONG"));
      return;
    }
    case OP_MILLIS: {
      Serial.print(F("OP_MILLIS"));
      return;
    }
    case OP_EE_READ: {
      Serial.print(F("OP_EE_READ"));
      return;
    }
    case OP_EE_WRITE: {
      Serial.print(F("OP_EE_WRITE"));
      return;
    }
    case OP_EE_LENGTH: {
      Serial.print(F("OP_EE_LENGTH"));
      return;
    }
    case OP_NULL: {
      Serial.print(F("OP_NULL"));
      return;
    }
    case OP_NOP: {
      Serial.print(F("OP_NOP"));
      return;
    }
    case OP_AS_SYM: {
      Serial.print(F("OP_AS_SYM"));
      return;
    }
    case OP_AS_ADDR: {
      Serial.print(F("OP_AS_ADDR"));
      return;
    }
    case OP_ABORT: {
      Serial.print(F("OP_ABORT"));
      return;
    }
    case OP_AS_BOOL: {
      Serial.print(F("OP_AS_BOOL"));
      return;
    }
    case OP_ADDR: {
      Serial.print(F("OP_ADDR"));
      return;
    }
    case OP_U14: {
      Serial.print(F("OP_U14"));
      return;
    }
    case OP_SPI_BEGIN: {
      Serial.print(F("OP_SPI_BEGIN"));
      return;
    }
    case OP_SPI_TRANSFER: {
      Serial.print(F("OP_SPI_TRANSFER"));
      return;
    }
    case OP_SPI_END: {
      Serial.print(F("OP_SPI_END"));
      return;
    }
    case OP_SYMBOL: {
      Serial.print(F("OP_SYMBOL"));
      return;
    }
    case OP_TWI_BEGIN: {
      Serial.print(F("OP_TWI_BEGIN"));
      return;
    }
    case OP_TWI_END: {
      Serial.print(F("OP_TWI_END"));
      return;
    }
    case OP_TWI_SETCLOCK: {
      Serial.print(F("OP_TWI_SETCLOCK"));
      return;
    }
    case OP_TWI_BEGIN_TR: {
      Serial.print(F("OP_TWI_BEGIN_TR"));
      return;
    }
    case OP_TWI_END_TR: {
      Serial.print(F("OP_TWI_END_TR"));
      return;
    }
    case OP_TWI_WRITE: {
      Serial.print(F("OP_TWI_WRITE"));
      return;
    }
    case OP_TWI_REQUEST: {
      Serial.print(F("OP_TWI_REQUEST"));
      return;
    }
    case OP_TWI_READ: {
      Serial.print(F("OP_TWI_READ"));
      return;
    }
  }
  Serial.print(F("Unknown OP "));
  Serial.print(opCode);
}
