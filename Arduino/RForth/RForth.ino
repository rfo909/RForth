#include <Wire.h>

#include "Constants.h"
#include "Firmware.h"

Word dStack[DSTACK_SIZE];
byte dStackNext=0;

Word cStack[CSTACK_SIZE];
byte cStackNext=0;

Word fStack[FSTACK_SIZE];
byte fStackNext=0;


// opCode array of function pointers
void (*ops[128]) ();

char temp[10];

// ------

byte hasError=0;

// ------
// Memory handling: need to duplicate "firmware" memory from the PROTECT tag, into RAM, updating 
// initial HERE value accordingly. For references below the PROTECT tag, we refer
// to the "firmware" flash buffer, and for values above, we subtract the PROTECT tag value
// and work on the "heap" array defined below.
//
// See readByte() function.

byte heap[RAM_SIZE];   // dictionary and data

Word HERE=0;
Word programCounter=0;
unsigned long instructionCount=0;


void setError (char *msg) {
  Serial.println();
  Serial.println(msg);
  hasError=true;
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(100);

  fpush(0,0);
  populateOps();
  populateRAM();

  Serial.print(F("(Ready) HERE="));
  Serial.println(HERE);
}

void doReset() {
  dStackNext=0;
  cStackNext=0;
  fStackNext=0;
  fpush(0,0);
  programCounter=0;
  hasError=false;
  Serial.println();
  Serial.print(F("(Ready) HERE="));
  Serial.println(HERE);

}

void cforce (Word addr) {
  // clear data stack, then modify call stack and frame stack so that
  // local variables are preserved, but next "ret" returns to given
  // address, typically 0.
  dStackNext=0;

  // identify local variables in current frame
  Word base=fpeekBase();
  Word size=fpeekSize(); // #locals

  // push single return address on cstack 
  cStackNext=0;

  // (can not use cpush, as it manages the frame stack)
  cStack[cStackNext++]=addr; 
 
  // preserve local variables on cstack (if any)
  for (int i=0; i<size; i++) {
    cStack[cStackNext++]=cStack[base+i];
  }
  
  // push two frames on frame stack
  fStackNext=0;
  fpush(0,1);  // return address frame
  fpush(1,size);

  showState();
  Serial.println();
  Serial.print(F("(cforce ready) HERE="));
  Serial.println(HERE);

}


void populateRAM() {
  // copy Flash data from PROTECT location into "heap" (SRAM)
  for (int i=firmwareProtectTag; i<firmwareSize; i++) {
    heap[i-firmwareProtectTag] = firmware[i];
  }
  HERE=firmwareSize;  // see read/write functions calculating offset based on firmwareProtectTag
}

void loop() {
  if (hasError) {  
    Serial.println();
    showState();

    // got an error situation
    Serial.print(F("Got ERROR, press ENTER"));
    clearSerialInputBuffer();
    readSerialChar();

    doReset();
    return;
  }
  Word pc=programCounter;
  Word op=readByte(programCounter);

  if (op & 0x80) {
    // number literal
    if (op & 0x40) {
      push(op & 0x3F);
    } else {
      Word w=(pop() << 6) | (op & 0x3F);
      push(w);
    }
  } else {
    // not number literal
    
    void (*funcPtr) () = ops[op];
    
    if (funcPtr==0) {
      Serial.print(F("funcPtr is 0, op="));
      Serial.println( op);
      setError("x");
      return;
    }

    // call op
    funcPtr ();
  }

  if (hasError) return;
  
  // increase programcounter, unless this op has modified it
  if (programCounter==pc) programCounter++;

  instructionCount++;

}

// ------------------------------------------------------------------------------------------------
// Memory management: the bytes of the firmware up to the PROTECT tag have addresses starting at 0
// We copy the data from the firmware array (Flash) from the PROTECT tag to the heap
// Need to subtract PROTECT tag value from addresses to get heap addresses
// ------------------------------------------------------------------------------------------------

void writeWord (Word addr, Word value) {
  writeByte(addr,(value>>8) & 0xFF);
  writeByte(addr+1,value & 0xFF);
}

void writeByte (Word addr, Word value) {
  if (addr < firmwareProtectTag) {
    Serial.print(F("writeByte: protected memory address 0x"));
    Serial.println(addr,16);
    setError("writeByte");
    return;
  } else if (addr >= HERE) {
    Serial.print(F("writeByte: unallocated memory address 0x"));
    Serial.println(addr,16);
    setError("writeByte"); 
    return;
  }
  // calculate heap address
  heap[addr-firmwareProtectTag]=(value & 0xFF);
}

Word readWord (Word addr) {
  Word a=readByte(addr);
  Word b=readByte(addr+1);
  Word value = a << 8 | b;
  return value;
}

Word readByte (Word addr) {
  if (addr >= HERE) {
    Serial.print(F("readByte: invalid address 0x"));
    Serial.println(addr,16);
    setError("> HERE");
    return;
  }
  if (addr < firmwareProtectTag) {
    return firmware[addr];
  } else {
    // calculate heap address
    Word pos=addr-firmwareProtectTag;
    return heap[pos];
  }
}

void allot (Word count) {
  if (HERE-firmwareProtectTag+count >= RAM_SIZE) setError("allot: heap overflow"); else HERE += count;
}




void populateOps() {
  for (int i=0; i<128; i++) {
    ops[i]=0;
  }

  ops[38]=&op_and;
  ops[42]=&op_mul;
  ops[43]=&op_add;
  ops[45]=&op_sub;
  ops[47]=&op_div;
  ops[48]=&op_PANIC;
  ops[49]=&op_atoi;
  ops[50]=&op_n2code;
  ops[51]=&op_rget;
  ops[52]=&op_rset;
  ops[54]=&op_rfwd_opt;
  ops[55]=&op_rback_opt;
  ops[60]=&op_lt;
  ops[61]=&op_eq;
  ops[62]=&op_gt;
  ops[66]=&op_streq;
  ops[67]=&op_dot_str;
  ops[68]=&op_memcpy;
  ops[69]=&op_ge;
  ops[70]=&op_le;
  ops[71]=&op_readb;
  ops[72]=&op_writeb;
  ops[73]=&op_read;
  ops[74]=&op_write;
  ops[75]=&op_allot;
  ops[76]=&op_printb;
  ops[79]=&op_cpush;
  ops[80]=&op_PC;
  ops[81]=&op_call;
  ops[82]=&op_ret;
  ops[83]=&op_jmp;
  ops[84]=&op_jmp_optional;
  ops[85]=&op_printc;
  ops[87]=&op_print;
  ops[88]=&op_cr;
  ops[89]=&op_print_decimal_unsigned;
  ops[90]=&op_halt;
  ops[95]=&op_u2spc;
  ops[97]=&op_native;
  ops[98]=&op_print_decimal_signed;
  ops[99]=&op_nativec;
  ops[100]=&op_1_plus;
  ops[102]=&op_cforce;
  ops[103]=&op_HERE;
  ops[104]=&op_ne;
  ops[105]=&op_not;
  ops[106]=&op_drop;
  ops[107]=&op_wordsize;
  ops[108]=&op_dup;
  ops[109]=&op_swap;
  ops[110]=&op_W_plus;
  ops[111]=&op_over;
  ops[112]=&op_dump;
  ops[113]=&op_andb;
  ops[114]=&op_orb;
  ops[115]=&op_inv;
  ops[116]=&op_shift_left;
  ops[117]=&op_shift_right;
  ops[118]=&op_readc;
  ops[119]=&op_clear;
  ops[122]=&op_null;
  ops[124]=&op_or;
}

void op_and () {Word b=pop(); Word a=pop(); push(a != 0 && b != 0);}
void op_mul () {Word b=pop(); Word a=pop(); push(a*b);}
void op_add () {Word b=pop(); Word a=pop(); push(a+b);}
void op_sub () {Word b=pop(); Word a=pop(); push(a-b);}
void op_div () {Word b=pop(); Word a=pop(); push(a/b);}
void op_PANIC () {setError("PANIC");}

void op_atoi () {Word targetPtr=pop(); Word strPtr=pop(); push(myAtoi(strPtr, targetPtr));}
void op_n2code () {Word nbytes=pop(); Word addr=pop(); Word num=pop(); push(n2code(num, addr, nbytes)); }

void op_rget () {Word index=pop(); Word base=fpeekBase(); Word value=cStack[base+index]; push(value);}
void op_rset () {Word index=pop(); Word value=pop(); Word base=fpeekBase(); cStack[base+index]=value;}

void op_rfwd_opt () {Word x=pop(); Word cond=pop(); if(cond) programCounter=programCounter+x;}
void op_rback_opt () {Word x=pop(); Word cond=pop(); if(cond) programCounter=programCounter-x;}
void op_lt () {Word b=pop(); Word a=pop(); push(a<b);}
void op_eq () {Word b=pop(); Word a=pop(); push(a==b);}
void op_gt () {Word b=pop(); Word a=pop(); push(a>b);}
void op_streq () {Word b=pop(); Word a=pop(); push(_streq(a,b));}
void op_dot_str () {Word ptr=pop(); Word len=readByte(ptr); for (int i=0; i<len; i++) printChar(readByte(ptr+i+1));}
void op_memcpy () {Word count=pop(); Word target=pop(); Word source=pop(); doMemcpy(source,target,count);}
void op_ge () {Word b=pop(); Word a=pop(); push(a>=b);}
void op_le () {Word b=pop(); Word a=pop(); push(a<=b);}

void op_readb () {Word addr=pop(); push(readByte(addr));}
void op_writeb () {Word addr=pop(); Word value=pop(); writeByte(addr,value);}
void op_read () {Word addr=pop(); push(readWord(addr));}
void op_write () {Word addr=pop(); Word value=pop(); writeWord(addr,value);}
void op_allot () {Word count=pop(); allot(count);}

void op_printb () {Word value=pop(); printBinary(value);}
void op_printc () {Word ch=pop(); printChar(ch);}
void op_print () {Word w=pop(); printHex(w);}
void op_cr () { Serial.println();}
void op_print_decimal_signed () {Word w=pop(); printSignedDecimal(w);}
void op_print_decimal_unsigned () {Word w=pop(); printUnsignedDecimal(w);}

void op_cpush () {Word value=pop(); cpush(value);}
void op_PC () {push(programCounter);}
void op_call () {Word targetAddr=pop(); Word returnAddr=programCounter+1; callCode(targetAddr, returnAddr);}
void op_ret () {returnFromCode();}
void op_jmp () {programCounter=pop();}
void op_jmp_optional () {Word addr=pop(); Word cond=pop(); if (cond != 0) programCounter=addr;}
void op_halt () { Serial.println("Halting"); for(;;) ; }
void op_u2spc () { Word ptr=pop(); u2spc(ptr); }
void op_native () { Word pos=pop(); callNative(pos);}
void op_nativec () { Word strPtr=pop(); push(lookupNative(strPtr));}
void op_1_plus () {push(pop()+1);}
void op_cforce () {Word addr=pop(); cforce(addr);}
void op_HERE () {push(HERE);}
void op_ne () {Word b=pop(); Word a=pop(); push(a!=b);}
void op_not () {Word x=pop(); push(!x);}
void op_drop () {pop();}
void op_wordsize () {push(WORDSIZE);}
void op_dup () {Word x=pop(); push(x); push(x);}
void op_swap () {Word b=pop(); Word a=pop(); push(b); push(a);}
void op_W_plus () {Word x=pop(); push(x+WORDSIZE);}
void op_over () {Word b=pop(); Word a=pop(); push(a); push(b); push(a);}
void op_dump () {op_cr(); for (int i=0; i<dStackNext; i++) {Serial.print(dStack[i]); Serial.print(" ");} op_cr();}
void op_andb () {Word b=pop(); Word a=pop(); push((Word) (a & b));}
void op_orb () {Word b=pop(); Word a=pop(); push((Word) (a | b));}
void op_inv () {Word x=pop(); push(~x);}
void op_shift_left () {Word b=pop(); Word a=pop(); push((Word) (a<<b));}
void op_shift_right () {Word b=pop(); Word a=pop(); push((Word) (a>>b));}
void op_readc () {push(readSerialChar());}
void op_clear () {dStackNext=0;}
void op_null () {push((Word) 0);}
void op_or () {Word b=pop(); Word a=pop(); push((Word) (a != 0 || b != 0));}



Word pop() {
  if (dStackNext > 0) {
    Word value=dStack[dStackNext-1];
    dStackNext--;
    return value;
  } else {
    setError("data stack underflow");
    return WORD_INVALID;
  }
}

void push (Word value) {
  if (dStackNext < DSTACK_SIZE-1) {
    dStack[dStackNext++]=value;
  } else {
    setError("data stack overflow");
  }
}

void fpush (Word base, Word size) {
  if (fStackNext >= FSTACK_SIZE-1) {
    setError("frame stack overflow");
  } else {
    fStack[fStackNext++] = base;
    fStack[fStackNext++] = size;
  }
}

void fpop () {
    if (fStackNext < 2) {
      setError("fstack underflow");
    } else {
        fStackNext-=2;
    }
}

Word fpeekSize () {
  if (fStackNext < 2) {
    setError("fstack underflow");
    return WORD_INVALID;
  } else {
    return fStack[fStackNext-1];
  }
}

Word fpeekBase () {
  if (fStackNext < 2) {
    setError("fstack underflow");
    return WORD_INVALID;
  } else {
    return fStack[fStackNext-2];
  }
}

void cpush (Word value) {
  if (cStackNext >= CSTACK_SIZE) {
    setError("cStack overflow");
  } else if (fStackNext < 1) {
    setError("cpush: fStack underflow");
  } else {
    cStack[cStackNext++] = value;
    // update frame size 
    Word size=fStack[fStackNext-1];
    fStack[fStackNext-1]=size+1;
  }
}


/*
Enhanced atoi, recognizes decimal, hex (0x...) and binary (b....) and negative, returns boolean 1 or 0 to 
indicate success or failure. Writes Word to targetPtr (not as code, just as value)
*/
Word myAtoi (Word strPtr, Word targetPtr) {
  int remaining=readByte(strPtr);
  if (remaining==0) return 0;

  int readPos=strPtr+1; // past length byte
  int negative=0;
  if (readByte(readPos)=='-') {
    negative=true;
    readPos++;
    remaining--;
  }

  int value=0; 

  if (remaining > 2 && readByte(readPos)=='0' && readByte(readPos+1)=='x') {
    // parsing hex?
    readPos+=2;
    remaining -= 2;

    for (int i=0; i<remaining; i++) {
      char c=(char) readByte(readPos++);
      if (c>='0' && c <= '9') {
        value=value*16 + (c-'0');
      } else if (c>='A' && c<='F') {
        value=value*16 + (c-'A') + 10;
      } else if (c>='a' && c<='f') {
        value=value*16 + (c-'a') + 10;
      } else {
        return 0;  // fail
      }
    }
    // ok
  } else if (remaining > 1 && readByte(readPos)=='b') {
    // possibly binary?
    readPos++;
    remaining--;

    for (int i=0; i<remaining; i++) {
      char c=(char) readByte(readPos++);
      if (c>='0' && c <= '1') {
        value=value*2 + (c-'0');
      } else {
        return 0;  // fail
      }
    }
    // ok
  } else if (remaining > 0) {
    // parse decimal?

    for (int i=0; i<remaining; i++) {
      char c=(char) readByte(readPos++);
      if (c>='0' && c <= '9') {
        value=value*10 + (c-'0');
      } else {
        return 0;  // fail
      }
    }
    // ok
  } else {
    // fail
    return 0;
  }

  if (negative) value=-value;
  Word w=(Word) value;  
  writeWord(targetPtr, w);
  return 1; // success
}


Word n2code (Word num, Word addr, Word nbytes) {  // returns number of bytes generated
  Word copy=num>>6;
  Word requiredByteCount=1;
  for(;;) {
    if (copy==0) break;
    copy=copy >> 6;
    requiredByteCount++;
  }
  if (nbytes != 0 && requiredByteCount > nbytes) {
    setError("n2code overflow");
    return WORD_INVALID;
  }
  Word zeroBytes=0;
  if (nbytes != 0 && nbytes > requiredByteCount) {
    zeroBytes=nbytes - requiredByteCount;
  }

  Word actualByteCount=requiredByteCount+zeroBytes;

  // write bytes to memory
  for (Word i=0; i<actualByteCount; i++) {
    byte b = (i==0 ? 0xC0 : 0x80);  // 11000000 or 10000000  
    if (i<zeroBytes) {
      // write b at addr+i
      writeByte(addr+i, b);
    } else {
      // merge in 6 data bits, write to addr+i
      int remainingBytes=actualByteCount-i-1;
      int shiftBits=6*remainingBytes;
      b = b | ((num >> shiftBits) & 0x3F); 
      writeByte(addr+i, b);
    }
  }

  return actualByteCount;
}

void callCode (Word targetAddr, Word returnAddr) {
  if (cStackNext >= CSTACK_SIZE-1) {
    setError("cStack overflow");
  } else if (fStackNext >= FSTACK_SIZE) {
    setError("fStack overflow");
  } else {
    // push return address
    cpush(returnAddr);
    // calculate next frame base
    Word base=fpeekBase();
    Word size=fpeekSize();
    Word newBase=base+size;
    Word newSize=0;
    // push frame data on frame stack
    fpush(newBase, newSize);
    // update PC
    programCounter=targetAddr;
  }
}

void returnFromCode () {
  // pop top frame off frame stack
  fpop();
 
  // calculate location of return address

  cStackNext=fpeekBase() + fpeekSize() - 1;  // -1 to move below the return address
  if (cStackNext < 0) {
    setError("cStack underflow");
    return;
  } else {
    programCounter=cStack[cStackNext];  // the return address
  }
  Word base=fpeekBase();
  Word size=fpeekSize()-1;
  fpop();
  fpush(base,size);
}


void doMemcpy(Word source, Word target, Word count) {
  for (int i=0; i<count; i++) {
    byte b=readByte(source+i);
    writeByte(target+i,b);
  }
}


Word _streq (Word a, Word b) {
  if (readByte(a) != readByte(b)) return 0; // different length
  byte len=readByte(a);
  for (Word i=0; i<len; i++) {
    Word apos=a+1+i;
    Word bpos=b+1+i;
    if (readByte(apos) != readByte(bpos)) return 0;
  }
  return 1;
}

void u2spc (Word ptr) {
  // convert underlines to spaces for string
  Word len=readByte(ptr);
  for (int i=0; i<len; i++) {
    Word b=readByte(ptr+1+i);
    if (b=='_') {
      writeByte(ptr+1+i, 32); // space
    }
  }
}

void printSignedDecimal (Word value) {
  sprintf(temp,"%d", value);
  Serial.print(temp);
}

void printUnsignedDecimal (Word value) {
  sprintf(temp,"%u", value);
  Serial.print(temp);
}

void printHex (Word value) {
  Serial.print("0x");
  Serial.print(value, HEX);
}

void printBinary (Word value) {
  Serial.print(value, BIN);
}

void printChar (Word ch) {
  sprintf(temp,"%c", ch);
  Serial.print(temp);
}

void printStr (Word ptr) {
  Word len=readByte(ptr);
  for (int i=0; i<len; i++) {
    printChar(readByte(ptr+i+1));
  }
}

Word readSerialChar () {
  for(;;) {
    int ch=Serial.read();
    if (ch >= 0) {
      if (ch==13 || ch==10) {
        if (READC_ECHO) Serial.println();
        ch=32; // map to space
      }  else {
        if (READC_ECHO) printChar(ch);
      }
      return (Word) ch;
    }
  }
}

void clearSerialInputBuffer() {
  while (Serial.available()>0) Serial.read();
}

void showState() {
  Serial.print("#instr=");
  Serial.print(instructionCount);
  Serial.print(" pc=0x");
  Serial.print(programCounter,16);
  Serial.print(" HERE=0x");
  Serial.println(HERE,16);

  Serial.print(" dStack=<");
  for (int i=0; i<dStackNext; i++) {
    if (i>0) Serial.print(" ");
    Serial.print("0x");
    Serial.print(dStack[i],16);
  }
  Serial.println(">");  
  Serial.print(" fStack=<");
  for (int i=0; i<fStackNext; i++) {
    if (i>0) Serial.print(" ");
    Serial.print("0x");
    Serial.print(fStack[i],16);
  }
  Serial.println(">");

  Serial.print(" cStack=<");
  for (int i=0; i<cStackNext; i++) {
    if (i>0) Serial.print(" ");
    Serial.print("0x");
    Serial.print(cStack[i],16);
  }
  Serial.println(">");
  Serial.println();

}


typedef struct {
  char *name;
  void (*f) ();
  char *info;
} NativeFunction;


// ----------------------------------------------------------------
// Native functions - use NATIVE name to call
// ----------------------------------------------------------------

// Doing i2c requires buffers. Since this is a runtime operations, we may use:
//
// &CompileBuf - 64 bytes
// &NextWord - 32 bytes
//
// These can be partitioned any way we like and double as scratch memory 

const NativeFunction nativeFunctions[]={
  {"?",           &natList,                 "( -- ) show list of native functions"},

  {"Sys.Free",    &natSysFree,              "( -- n) return number of free bytes of heap space"},
  {"Sys.Delay",   &natSysDelay,             "(ms -- ) sleep a number of millis"},
  {"Sys.DelayUs",   &natSysDelayUs,         "(us -- ) sleep a number of microseconds"},

  {"Pin.ModeOut", &natPinModeOut,           "(pin -- ) set pin mode"},
  {"Pin.ModeIn",  &natPinModeIn,            "(pin -- ) set pin mode"},
  {"Pin.ModeInPullup",  &natPinModeInPullup,"(pin -- ) set pin mode"},
  {"Pin.WriteDigital", &natPinWriteDigital, "(value pin --)"},
  {"Pin.WriteAnalog",  &natPinWriteAnalog,  "(value pin --) value is 0-255"},
  {"Pin.PulseDigitalMs", &natPinPulseDigitalMs, "(ms value pin -- ) pulse digital pin / milliseconds"},
  {"Pin.PulseDigitalUs", &natPinPulseDigitalUs, "(us value pin -- ) pulse digital pin / microseconds"},
  {"Pin.ReadDigital",  &natPinReadDigital,  "(pin -- value) returns 0 or 1"},
  {"Pin.ReadAnalog",   &natPinReadAnalog,   "(pin -- value) returns 0-1023"},

  {"I2C.MasterWrite",   &natI2CMasterWrite,   "(sendBufPtr sendCount addr -- )"},
  {"I2C.MasterWWait",   &natI2CMasterWWait,   "(addr -- ) wait for data write (eeprom) to complete"},
  {"I2C.MasterRead",    &natI2CMasterRead,    "(count recvBufPtr addr -- recvCount)"},

  {"Test.EEPROM",   &natTestEEPROM,   "(i2cAddr memAddr -- )"},

  {"",0,""} 
};



// The nativec (compile) op, looks up the index in the NativeFunctions[] array,
// sets error flag if not found
Word lookupNative (Word strPtr) {
  Word pos=0;
  for(;;) {
    if (nativeFunctions[pos].f==0) {
      // end of list
      Serial.print(F("Unknown native function "));
      printStr(strPtr);
      setError("Native");
      return WORD_INVALID;
    }
    if (mixedStreq(strPtr, nativeFunctions[pos].name)) {
      return pos;
    }
    // not found
    pos++;
  }
}


// The native (call) op
Word callNative (Word pos) {
  nativeFunctions[pos].f();
}


int mixedStreq (Word strPtr, char *s) {
  Word len=readByte(strPtr);
  if (strlen(s) != len) return 0;
  for (int i=0; i<len; i++) {
    byte a=readByte(strPtr+i+1);
    byte b=s[i];
    if (a != b) return 0;
  }
  return 1;
}

// -------------------------------
// List native functions
// -------------------------------

void natList() {
  // list native words
  Word pos=0;
  for(;;) {
    if (nativeFunctions[pos].f==0) {
      return pos;  // count
    }

    Serial.print(nativeFunctions[pos].name);
    Serial.print("  ");
    Serial.println(nativeFunctions[pos].info);
    pos++;
  }

}

// -------------------------------
// Sys.*
// -------------------------------

void natSysFree () {
  Word here=HERE-firmwareProtectTag; // actual index in heap
  push(RAM_SIZE-here);
}

void natSysDelay() {
  Word ms=pop();
  delay(ms);
}

void natSysDelayUs() {
  Word us=pop();
  delayMicroseconds(us);
}

// -------------------------------
// Pin.*
// -------------------------------

void natPinModeOut () {
  Word pin=pop();
  pinMode(pin, OUTPUT);
}

void natPinModeIn () {
  Word pin=pop();
  pinMode(pin, INPUT);
}

void natPinModeInPullup () {
  Word pin=pop();
  pinMode(pin, INPUT_PULLUP);
}

void natPinWriteDigital () {
  Word pin=pop();
  Word value=pop();
  digitalWrite(pin,value==0 ? LOW : HIGH);
}

void natPinWriteAnalog () {
	Word pin=pop();
	Word value=pop();
	analogWrite(pin,value);
}

void natPinPulseDigitalMs () {
	Word pin=pop();
	Word value=pop();
	Word ms=pop();

	if (value==0) {
		digitalWrite(pin, LOW);
		delay(ms);
		digitalWrite(pin, HIGH);
	} else {
		digitalWrite(pin, HIGH);
		delay(ms);
		digitalWrite(pin, LOW);
	}
}

void natPinPulseDigitalUs () {
	Word pin=pop();
	Word value=pop();
	Word us=pop();

	if (value==0) {
		digitalWrite(pin, LOW);
		delayMicroseconds(us);
		digitalWrite(pin, HIGH);
	} else {
		digitalWrite(pin, HIGH);
		delayMicroseconds(us);
		digitalWrite(pin, LOW);
	}
}

void natPinReadDigital () {
	Word pin=pop();
	push(digitalRead(pin));
}

void natPinReadAnalog () {
	Word pin=pop();
	push(analogRead(pin));
}

// -------------------------------
// I2C.*
// -------------------------------

// (sendBufPtr sendCount addr -- )
void natI2CMasterWrite() {
  Word addr=pop();
  Word sendCount=pop();
  Word sendBuf=pop();
  
  Wire.beginTransmission((byte) addr);
  for (byte i=0; i<sendCount; i++) {
    Wire.write((byte)readByte(sendBuf+i+1));
  }
  Wire.endTransmission();
}

// EEPROM's are sometimes slow at doing page writes etc (thank you, ChatGPT)
void natI2CMasterWWait () {
  Word addr=pop();  
  while (true) {
    Wire.beginTransmission((int) addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) break;  // ACK received
    delay(1);            // Small delay to avoid hammering the bus
  }
}

// count recvBufPtr addr -- recvCount)
void natI2CMasterRead() {
  Word addr=pop();
  Word recvBuf=pop();
  Word count=pop();

  Wire.requestFrom((int) addr, (int) count);
  Word i=0;
  while (i<count && Wire.available()) {
    byte b = Wire.read();
    writeByte(recvBuf+i+1, b);
    i++;
  }
  Wire.endTransmission();
  push(i);  // received count
}

// -------------------------------
// Test.*
// -------------------------------

// This works, even without external pullup resistors; delays for 3ms (zzz) after each write :-)
void natTestEEPROM () {
  int memAddr = (int) pop();
  int i2cAddr=(int) pop();
  for (uint8_t value=1; value<255; value++) {
    // write one byte
    Wire.beginTransmission(i2cAddr);
    Wire.write((memAddr >> 8) & 0xFF);   // High address byte
    Wire.write(memAddr & 0xFF);          // Low address byte
    Wire.write(value);
    Wire.endTransmission();

    // wait for write to complete
    while (true) {
      Wire.beginTransmission(i2cAddr);
      uint8_t err = Wire.endTransmission();
      if (err == 0) break;  // ACK received
      Serial.print("z");
      delay(1);            // Small delay to avoid hammering the bus
    }
    Serial.println();

    // read back value - set address
    Wire.beginTransmission(i2cAddr);
    Wire.write((memAddr >> 8) & 0xFF);
    Wire.write(memAddr & 0xFF);
    Wire.endTransmission();

    // don't need to wqit here, because the two first
    // bytes are stored in the address registers and are
    // not written to the memory, so no delay
    
    // read value
    uint8_t rvalue=0;
    Wire.requestFrom(i2cAddr, 1);
    if (Wire.available()) {
      rvalue=Wire.read();
    }
    Serial.print("Expected ");
    Serial.print(value);
    Serial.print(" got ");
    Serial.println(rvalue);
  } // for
}

