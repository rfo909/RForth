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


static const char RETURN[] PROGMEM = "return";
static const char READ[] PROGMEM = "read";
static const char WRITE[] PROGMEM = "write";

static const char DUP[] PROGMEM = "dup";
static const char POP[] PROGMEM = "pop";
static const char NEG[] PROGMEM = "neg";

static const char T_BYTE[] PROGMEM = ":byte";
static const char T_INT[] PROGMEM = ":int";
static const char T_UINT[] PROGMEM = ":uint";
static const char T_LONG[] PROGMEM = ":long";
static const char T_ULONG[] PROGMEM = ":ulong";

static const char MILLIS[] PROGMEM = "millis";
static const char EE_READ[] PROGMEM = "ee:read";
static const char EE_WRITE[] PROGMEM = "ee:write";
static const char EE_LENGTH[] PROGMEM = "ee:length";

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
  
  if (!strcmp_P(sym,RETURN)) return OP_RET;
  if (!strcmp_P(sym,READ)) return OP_READ;
  if (!strcmp_P(sym,WRITE)) return OP_WRITE;
  if (!strcmp_P(sym,DUP)) return OP_DUP;
  if (!strcmp_P(sym,POP)) return OP_POP;
  if (!strcmp_P(sym,NEG)) return OP_NEG;

  if (!strcmp_P(sym,T_BYTE)) return OP_AS_BYTE;
  if (!strcmp_P(sym,T_INT)) return OP_AS_INT;
  if (!strcmp_P(sym,T_UINT)) return OP_AS_UINT;
  if (!strcmp_P(sym,T_LONG)) return OP_AS_LONG;
  if (!strcmp_P(sym,T_ULONG)) return OP_AS_ULONG;

  if (!strcmp_P(sym,MILLIS)) return OP_MILLIS;
  if (!strcmp_P(sym,EE_READ)) return OP_EE_READ;
  if (!strcmp_P(sym,EE_WRITE)) return OP_EE_WRITE;
  if (!strcmp_P(sym,EE_LENGTH)) return OP_EE_LENGTH;
  
  return -1; 
}
