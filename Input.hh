#ifndef RF_INPUT_HH
#define RF_INPUT_HH

#include "Common.hh"


void inpReset();
void inpAddChar (char c);
void inpUngetChar ();
bool inpEmpty();
char *inpChop ();
void inpAddToken (char *token);
bool inpTokenMatches (char *s);
void inpTokenAdvance ();
char *inpTokenGet();
int inpTokenStreamPos();
void inpTokenStreamSetPos (int pos);

int inpLocalVariablePos (char *name);
bool inpLocalVariablesFull();
int inpLocalVariableAdd (char *name);



#endif
