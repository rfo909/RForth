#include "Constants.h"
#include "Firmware.h"

Word dStack[DSTACK_SIZE];
byte dStackNext=0;

Word rStack[RSTACK_SIZE];
byte rStackNext=0;

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


void setError (char *msg) {
  Serial.println(msg);
  hasError=true;
}

void setup() {
  Serial.begin(9600);

  fpush(0,0);
  populateOps();

  populateRAM();

  Serial.print("(Ready) HERE=");
  Serial.println(HERE);

  Serial.print("firmware[0] ");
  Serial.println(firmware[0]);
}


void populateRAM() {
  // copy Flash data from PROTECT location into "heap" (SRAM)
  for (int i=firmwareProtectTag; i<firmwareSize; i++) {
    heap[i] = firmware[i];
  }
  HERE=firmwareSize;  // see read/write functions calculating offset based on firmwareProtectTag
}

void loop() {
  Serial.print("PC=");
  Serial.print(programCounter);
  Serial.print(" HERE=");
  Serial.print(HERE);
  Serial.print(" stack=<");
  for (int i=0; i<dStackNext; i++) {
    if (i>0) Serial.print(" ");
    Serial.print(dStack[i]);
  }
  Serial.println(">");
  readSerialChar();

  if (hasError) {
    // got an error situation
    Serial.print(F("Press ENTER"));
    readSerialChar();

    // clear error message
    //*errorMessage='\0';
    hasError=false;
  }
  Word pc=programCounter;
  Word op=readByte(programCounter);
  Serial.print("op=");
  Serial.println(op);

  if (op & 0x80) {
    // number literal
    if (op & 0x40) {
      Serial.println("11xxxxxx");
      push(op & 0x3F);
    } else {
      Serial.println("10xxxxxx");
      Word w=pop() << 6;
      push(w | (op & 0x3F));
    }
  } else {
    // not number literal
    void (*funcPtr) () = ops[op];
    
    if (funcPtr==0) {
      Serial.println("Null op: " + op);
      Serial.println("(ENTER)");
      readSerialChar();
      return;
    }

    // call op
    funcPtr ();
  }

  // increase programcounter, unless this op has modified it
  if (programCounter==pc) programCounter++;

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
  if (addr < firmwareProtectTag || addr >= HERE) {
    Serial.print(F("writeByte: invalid address "));
    Serial.println(addr);
    setError("x"); 
    return;
  }
  // calculate heap address
  addr=addr-firmwareProtectTag;
  heap[addr]=value & 0xFF;
}

Word readWord (Word addr) {
  return readByte(addr) << 8 | readByte(addr+1);
}

Word readByte (Word addr) {
  if (addr >= HERE) {
    Serial.print(F("readByte: invalid address "));
    Serial.println(addr);
    setError("x");
    return;
  }
  if (addr < firmwareProtectTag) {
    Serial.print("Reading firmware ");
    Serial.println(addr);
    return firmware[addr];
  } else {
    // calculate heap address
    addr=addr-firmwareProtectTag;
    Serial.print("Reading heap ");
    Serial.println(addr);
    return heap[addr];
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
  ops[79]=&op_rpush;
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
  ops[97]=&op_native;
  ops[98]=&op_print_decimal_signed;
  ops[99]=&op_nativec;
  ops[100]=&op_1_plus;
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

void op_and () {Word b=pop(); Word a=pop(); push(a != 00 && b != 0);}
void op_mul () {Word b=pop(); Word a=pop(); push(a*b);}
void op_add () {Word b=pop(); Word a=pop(); push(a+b);}
void op_sub () {Word b=pop(); Word a=pop(); push(a-b);}
void op_div () {Word b=pop(); Word a=pop(); push(a/b);}
void op_PANIC () {setError("panic");}

void op_atoi () {Word targetPtr=pop(); Word strPtr=pop(); push(myAtoi(strPtr, targetPtr));}
void op_n2code () {Word nbytes=pop(); Word addr=pop(); Word num=pop(); push(n2code(num, addr, nbytes)); }

void op_rget () {Word index=pop(); Word base=fpeekBase(); Word value=rStack[base+index]; push(value);}
void op_rset () {Word index=pop(); Word value=pop(); Word base=fpeekBase(); rStack[base+index]=value;}

void op_rfwd_opt () {Word x=pop(); Word cond=pop(); if(cond) programCounter=programCounter+x;}
void op_rback_opt () {Word x=pop(); Word cond=pop(); if(cond) programCounter=programCounter-x;}
void op_lt () {Word b=pop(); Word a=pop(); push(a<b);}
void op_eq () {Word b=pop(); Word a=pop(); push(a==b);}
void op_gt () {Word b=pop(); Word a=pop(); push(a>b);}
void op_streq () {Word b=pop(); Word a=pop(); push(_streq(a,b));}
void op_dot_str () {Word ptr=pop(); Word len=heap[ptr]; for (int i=0; i<len; i++) printChar(heap[ptr+i+1]);}
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
void op_cr () { Serial.print("\r\n");}
void op_print_decimal_signed () {Word w=pop(); printSignedDecimal(w);}
void op_print_decimal_unsigned () {Word w=pop(); printUnsignedDecimal(w);}

void op_rpush () {Word value=pop(); rpush(value);}
void op_PC () {push(programCounter);}
void op_call () {Word targetAddr=pop(); Word returnAddr=programCounter+1; callCode(targetAddr, returnAddr);}
void op_ret () {returnFromCode();}
void op_jmp () {programCounter=pop();}
void op_jmp_optional () {Word addr=pop(); Word cond=pop(); if (cond != 0) programCounter=addr;}
void op_halt () { Serial.println("Halting"); for(;;) ; }
void op_native () {}
void op_nativec () {}
void op_1_plus () {push(pop()+1);}
void op_HERE () {push(HERE);}
void op_ne () {Word b=pop(); Word a=pop(); push(a!=b);}
void op_not () {Word b=pop(); Word a=pop(); push(!a);}
void op_drop () {pop();}
void op_wordsize () {push(WORDSIZE);}
void op_dup () {Word x=pop(); push(x); push(x);}
void op_swap () {Word b=pop(); Word a=pop(); push(b); push(a);}
void op_W_plus () {Word x=pop(); push(x+WORDSIZE);}
void op_over () {Word b=pop(); Word a=pop(); push(a); push(b); push(a);}
void op_dump () {for (int i=0; i<dStackNext; i++) {Serial.print(dStack[i]); Serial.print(" ");} op_cr();}
void op_andb () {Word b=pop(); Word a=pop(); push((Word) a & b);}
void op_orb () {Word b=pop(); Word a=pop(); push((Word)a | b);}
void op_inv () {Word x=pop(); push(~x);}
void op_shift_left () {Word b=pop(); Word a=pop(); push((Word) a<<b);}
void op_shift_right () {Word b=pop(); Word a=pop(); push((Word) a>>b);}
void op_readc () {readSerialChar();}
void op_clear () {dStackNext=0;}
void op_null () {push(0);}
void op_or () {Word b=pop(); Word a=pop(); push(a||b);}



Word pop() {
  if (dStackNext > 0) {
    return dStack[--dStackNext];
  } else {
    setError("data stack underflow");
    return 0;
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

void rpush (Word value) {
  if (rStackNext >= RSTACK_SIZE) {
    setError("rStack overflow");
  } else if (fStackNext < 1) {
    setError("fStack underflow");
  } else {
    rStack[rStackNext++] = value;
    // update frame size 
    int size=fStack[fStackNext-1];
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
  if (rStackNext >= RSTACK_SIZE-1) {
    setError("rStack overflow");
  } else if (fStackNext >= FSTACK_SIZE) {
    setError("fStack overflow");
  } else {
    // posh return address
    rStack[rStackNext++]=returnAddr;
    // calculate next frame base
    Word base=fpeekBase();
    Word size=fpeekSize();
    Word newBase=base+size;
    Word newSize=0;
    // push frame data on frame stack
    fStack[fStackNext++]=newBase;
    fStack[fStackNext++]=newSize;
    // update PC
    programCounter=targetAddr;
  }
}

void returnFromCode () {
  if (fStackNext < 2) {
    setError("fStack underflow");
  } else {
    // pop top frame off frame stack
    fStackNext -= 2;
    // calculate location of return address
    rStackNext=fpeekBase() + fpeekSize() - 1;
    if (rStackNext < 0) {
      setError("rStack underflow");
      rStackNext=0;
    } else {
      programCounter=rStack[rStackNext];
    }
  }
  // pop frame data from frame stack
}

void doMemcpy(Word source, Word target, Word count) {
  memcpy(heap+target, heap+source,count);
}


Word _streq (Word a, Word b) {
  if (heap[a] != heap[b]) return 0;    // different length
  for (Word i=0; i<=heap[a]; i++) {
    Word apos=a+1+i;
    Word bpos=b+1+i;
    if (heap[apos] != heap[bpos]) return 0;
  }
  return 1;
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
  Serial.print(value, HEX);
}

void printBinary (Word value) {
  Serial.print(value, BIN);
}

void printChar (Word ch) {
  sprintf(temp,"%c", ch);
  Serial.print(temp);
}

void readSerialChar () {
  for(;;) {
    int ch=Serial.read();
    if (ch >= 0) {
      push((Word) ch);
      break;
    }
  }
}
