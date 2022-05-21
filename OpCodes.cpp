#include "OpCodes.hh"

#include "Common.hh"


int lookupSymbol (char *sym) {
  if (!strcmp(sym,"+")) return OP_ADD;
  if (!strcmp(sym,"-")) return OP_SUB;
  if (!strcmp(sym,"*")) return OP_MUL;
  if (!strcmp(sym,"/")) return OP_DIV;
  if (!strcmp(sym,"%")) return OP_MOD;
  if (!strcmp(sym,"<<")) return OP_LSHIFT;
  if (!strcmp(sym,">>")) return OP_RSHIFT;
  if (!strcmp(sym,"|")) return OP_B_OR;
  if (!strcmp(sym,"&")) return OP_B_AND;
  if (!strcmp(sym,"!")) return OP_B_NOT;
  if (!strcmp(sym,">")) return OP_GT;
  if (!strcmp(sym,"<")) return OP_LT;
  if (!strcmp(sym,">=")) return OP_GE;
  if (!strcmp(sym,"<=")) return OP_LE;
  if (!strcmp(sym,"==")) return OP_EQ;
  if (!strcmp(sym,"!=")) return OP_NE;
  if (!strcmp(sym,"and")) return OP_L_AND;
  if (!strcmp(sym,"or")) return OP_L_OR;
  if (!strcmp(sym,"not")) return OP_L_NOT;
  
  if (!strcmp(sym,"return")) return OP_RET;
  if (!strcmp(sym,"read")) return OP_READ;
  if (!strcmp(sym,"write")) return OP_WRITE;
  if (!strcmp(sym,"dup")) return OP_DUP;
  if (!strcmp(sym,"pop")) return OP_POP;
  if (!strcmp(sym,"neg")) return OP_NEG;

  if (!strcmp(sym,":byte")) return OP_AS_BYTE;
  if (!strcmp(sym,":int")) return OP_AS_INT;
  if (!strcmp(sym,":uint")) return OP_AS_UINT;
  if (!strcmp(sym,":long")) return OP_AS_LONG;
  if (!strcmp(sym,":ulong")) return OP_AS_ULONG;

  if (!strcmp(sym,"millis")) return OP_MILLIS;
  
  return -1; 
}
