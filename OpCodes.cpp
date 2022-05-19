#include "OpCodes.hh"

#include "Common.hh"


char *getOp (byte b) {
    // auto-generated
    if (b==OP_EOF) return "EOF";
    if (b==OP_RET) return "RET";
    if (b==OP_CALL) return "CALL";
    if (b==OP_JMP) return "JMP";
    if (b==OP_ZJMP) return "ZJMP";
    if (b==OP_CJMP) return "CJMP";
    if (b==OP_POP) return "POP";
    if (b==OP_DUP) return "DUP";
    if (b==OP_READ) return "READ";
    if (b==OP_WRITE) return "WRITE";
    if (b==OP_LSET) return "LSET";
    if (b==OP_LGET) return "LGET";
    if (b==OP_ADD) return "ADD";
    if (b==OP_SUB) return "SUB";
    if (b==OP_MUL) return "MUL";
    if (b==OP_DIV) return "DIV";
    if (b==OP_MOD) return "MOD";
    if (b==OP_NEG) return "NEG";
    if (b==OP_GT) return "GT";
    if (b==OP_LT) return "LT";
    if (b==OP_GE) return "GE";
    if (b==OP_LE) return "LE";
    if (b==OP_EQ) return "EQ";
    if (b==OP_NE) return "NE";
    if (b==OP_L_AND) return "L_AND";
    if (b==OP_L_OR) return "L_OR";
    if (b==OP_L_NOT) return "L_NOT";
    if (b==OP_LSHIFT) return "LSHIFT";
    if (b==OP_RSHIFT) return "RSHIFT";
    if (b==OP_B_AND) return "B_AND";
    if (b==OP_B_OR) return "B_OR";
    if (b==OP_B_NOT) return "B_NOT";

    if (b==OP_LSET0) return "LSET0";
    if (b==OP_LSET1) return "LSET1";
    if (b==OP_LSET2) return "LSET2";
    if (b==OP_LSET3) return "LSET3";
    if (b==OP_LGET0) return "LGET0";
    if (b==OP_LGET1) return "LGET1";
    if (b==OP_LGET2) return "LGET2";
    if (b==OP_LGET3) return "LGET3";

    if (b==OP_AS_BYTE)  return "AS_BYTE";
    if (b==OP_AS_INT)   return "AS_INT";
    if (b==OP_AS_UINT)  return "AS_UNIT";
    if (b==OP_AS_LONG)  return "AS_LONG";
    if (b==OP_AS_ULONG) return "AS_ULONG";

    if (b==OP_MILLIS) return "MILLIS";

    return NULL;
}





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
  if (!strcmp(sym,".")) return OP_POP;
  if (!strcmp(sym,"neg")) return OP_NEG;

  if (!strcmp(sym,":byte")) return OP_AS_BYTE;
  if (!strcmp(sym,":int")) return OP_AS_INT;
  if (!strcmp(sym,":uint")) return OP_AS_UINT;
  if (!strcmp(sym,":long")) return OP_AS_LONG;
  if (!strcmp(sym,":ulong")) return OP_AS_ULONG;

  if (!strcmp(sym,"millis")) return OP_MILLIS;
  
  return -1; 
}
