#include "Constants.h"
#include <Print.h>

static char temp[10];

void printChar (Byte ch) {
  temp[0]=ch;
  temp[1]='\0';
  sPrint(temp);

# ifdef MONITOR_C_STACK
  checkCStackSize();
# endif
}

void printStr (Word ptr) {
  Word len=readByte(ptr);
  for (int i=0; i<len; i++) {
    printChar(readByte(ptr+i+1));
  }
}



Boolean mixedStreq (Word strPtr, char *s) {
  Word len=readByte(strPtr);
  if (strlen(s) != len) return false;
  for (Byte i=0; i<len; i++) {
    Byte a=readByte(strPtr+i+1);
    Byte b=s[i];
    if (a != b) return false;
  }
  return true;
}


/*
Enhanced atoi, recognizes decimal, hex (0x...) and binary (b....) and negative, returns boolean to 
indicate success or failure. Writes result int *target
*/
Boolean myAtoi (char *nextWord, int *target) {
  int readPos=0;
  
  Boolean negative=false;
  if (nextWord[readPos]=='-') {
    negative=true;
    readPos++;
    if(nextWord[readPos] == '\0') return false;
  }

  long value=0; 

  if (!strncmp(nextWord+readPos,"0x",2)) {
    // parsing hex
    readPos+=2;

    while(nextWord[readPos] != '\0') {
      char c=nextWord[readPos++];
      if (c=='\0') return false; // no digits

      if (c>='0' && c <= '9') {
        value=value*16 + (c-'0');
      } else if (c>='A' && c<='F') {
        value=value*16 + (c-'A') + 10;
      } else if (c>='a' && c<='f') {
        value=value*16 + (c-'a') + 10;
      } else {
        return false;  // fail
      }
    }
    // ok
  } else if (nextWord[readPos]=='b') {
    // possibly binary
    readPos++;
    if (nextWord[readPos]=='\0') return false; // no digits

    while(nextWord[readPos] != '\0') {
      char c=(char) nextWord[readPos++];
      if (c>='0' && c <= '1') {
        value=value*2 + (c-'0');
      } else {
        return false;  // fail
      }
    }
    // ok
  } else {
    // decimal?
    while(nextWord[readPos] != '\0') {
      char c=(char) nextWord[readPos++];
      if (c>='0' && c <= '9') {
        value=value*10 + (c-'0');
      } else {
        return false;  // fail
      }
    }
    // ok
  } 

  if (negative) value=-value;
  *target=value;
  return true; // success
}


static Word maxStackSize=0;
void checkCStackSize() {
  volatile Byte b=0;
  Word val=0x08FF - (Word) &b;
  if (val>maxStackSize) maxStackSize=val;
}

Word getCStackMaxSize() {
  return maxStackSize;
}
