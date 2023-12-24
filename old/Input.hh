#ifndef RF_INPUT_HH
#define RF_INPUT_HH

#include "Common.hh"


void inpReset();
int inpSetMark ();
void inpResetToMark (int mark);
void inpAddChar (char c);
void inpUngetChar ();
bool inpEmpty();
int  inpGetStartPos(); // of current string - before chop()

char *inpChop ();
void inpAddToken (char *token);
bool inpTokenMatches (char *s);
void inpTokenAdvance ();
char *inpTokenGet();
int inpTokenCount();
char *inpTokenAtPos (int pos);
int inpTokenStreamPos();
void inpTokenStreamSetPos (int pos);

int inpLocalVariablePos (char *name);
bool inpLocalVariablesFull();
int inpLocalVariableAdd (char *name);



#endif
