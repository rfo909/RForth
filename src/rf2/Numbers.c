#include "rfcommon.h"


static bool checkParseIntWithBase (char *s, int base) {
    for (int i = 0; i < strlen(s); i++) {
        char c = s[i];
        if (c=='_') continue;
        if (c=='0' || c=='1') continue;
        if (base >= 10) {
            if (c=='2') continue;
            if (c=='3') continue;
            if (c=='4') continue;
            if (c=='5') continue;
            if (c=='6') continue;
            if (c=='7') continue;
            if (c=='8') continue;
            if (c=='9') continue;
        } 
        if (base==16) {
            if (c=='a' || c=='A') continue;
            if (c=='b' || c=='B') continue;
            if (c=='c' || c=='C') continue;
            if (c=='d' || c=='D') continue;
            if (c=='e' || c=='E') continue;
            if (c=='f' || c=='F') continue;
        }
        return false;
    }
    return true;
}

bool checkParseInt (char *s) {

  // enter hex number as 0x...
  if (s[0] == '0' && s[1] == 'x') {
    return checkParseIntWithBase(s + 2,16);
  }

  // enter binary number as b001001, plus support underscore _ for readability
  if ((s[0] == 'b' || s[0] == 'B') && (s[1] == '1' || s[1] == '0')) {
    return checkParseIntWithBase(s + 1, 2);
  }

  // decimal numbers
  return checkParseIntWithBase (s, 10);
}



static Long parseIntWithBase (char *s, int base) {
  Long val = 0;
  for (int i = 0; i < strlen(s); i++) {
    char c = s[i];
    if (c=='_') continue;

    val=val*base;

    if (c == '0') continue;
    if (c == '1') val += 1;

    if (base >= 10) {
        if (c == '2') val += 2;
        else if (c == '3') val += 3;
        else if (c == '4') val += 4;
        else if (c == '5') val += 5;
        else if (c == '6') val += 6;
        else if (c == '7') val += 7;
        else if (c == '8') val += 8;
        else if (c == '9') val += 9;
    } 
    if (base == 16) {
        if (c == 'a' || c == 'A') val += 10;
        else if (c == 'b' || c == 'B') val += 11;
        else if (c == 'c' || c == 'C') val += 12;
        else if (c == 'd' || c == 'D') val += 13;
        else if (c == 'e' || c == 'E') val += 14;
        else if (c == 'f' || c == 'F') val += 15;
    }
  }
  return val;
}


Long parseInt (char *s) {

  // enter hex number as 0x...
  if (s[0] == '0' && s[1] == 'x') {
    return parseIntWithBase(s + 2,16);
  }

  // enter binary number as b001001, plus support underscore _ for readability
  if ((s[0] == 'b' || s[0] == 'B') && (s[1] == '1' || s[1] == '0')) {
    return parseIntWithBase(s + 1, 2);
  }

  // decimal numbers
  return parseIntWithBase (s, 10);
}

