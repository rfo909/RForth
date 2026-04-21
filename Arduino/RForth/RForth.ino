#include <avr/pgmspace.h>
#include <math.h>
#include "Constants.h"

#include <Wire.h>
#include <EEPROM.h>



static char nextWord[MAX_WORD_LENGTH+1];  // input buffer

static Word programCounter=0;
static unsigned long instructionCount=0;

static Boolean errorFlag = false;

static Word dStack[DSTACK_SIZE];
static Byte dStackNext = 0;
static Word rStack[RSTACK_SIZE];
static Byte rStackNext = 0;
static Long xStack[XSTACK_SIZE];
static Byte xStackNext;

void setup() {
  Serial.begin(9600);
  Serial.print(F_CPU / 1000000.0);
  Serial.println(F(" MHz"));
  Serial.print(F("RAM bytes   ; "));
  Serial.print(RAMEND-RAMSTART+1);
  Serial.println();
  Serial.print(F("Code segment: "));
  Serial.println(CODE_SEGMENT_SIZE);
  Serial.print(F("Data segment: "));
  Serial.println(DATA_SEGMENT_SIZE);

  memInit();
  Serial.println();
  Serial.println(F("Use '?' to show Forth words and 'ops' to show words written in C"));

  // copy Dictionary Header pointer from static data segment lower two bytes to RAM
  // (note address 0 in code segment is reserved, so data are in location 1 and 2)

  Word dictPointer=readWord(generateCodeAddress(1));
  Word dataNext=readWord(generateCodeAddress(3));
  Serial.print(F("dictPointer: "));
  Serial.println(dictPointer);

  if (dataNext==0) dataNext=2; // for storing the dict pointer

  Serial.print(F("dataNext: "));
  Serial.println(dataNext);
  // update dataNext, enabling all variable defs and allots done by code (Flash)
  dataAllot(dataNext);
  // write dictionary pointer to RAM
  writeWord(generateDataAddress(0), dictPointer);

  disableWatchdogInterrupt();

}

// ##########################################
// atmega328p specific code

#if defined(__AVR_ATmega328P__)
  // -----------------------------
  // Deep sleep stuff
  // -----------------------------

  ISR(WDT_vect) {
    cli();
    disableWatchdogInterrupt();
    sei();
  }


  void disableWatchdogInterrupt() {
    WDTCSR &= ~(1<<WDIE);
  }

  void enableADC() {
    ADCSRA |= (1<<7);
  }

  void doDeepSleep() {

    // store data direction registers for pins
    Byte ddrb=DDRB;
    Byte ddrc=DDRC;
    Byte ddrd=DDRD;

    Byte portb=PORTB;
    Byte portc=PORTC;
    Byte portd=PORTD;

    for (int i=0; i<20; i++) {
      pinMode(i,OUTPUT);  // saves milliamps
      digitalWrite(i,0);
    }

    //blip(2);
    // https://www.youtube.com/watch?v=urLSDi7SD8M

    // disable ADC
    ADCSRA &= ~(1<<7);

    cli();

    // Setup watchdog timer to wake from sleep
    // 7     6   5    4    3   2    1    0
    // WDIF WDIE WDP3 WDCE WDE WDP1 WDP1 WDP0
    // WDP* are prescaler bits, note WDP3 separate from the others

    WDTCSR = (1<<WDCE) | (1<<WDE) ; // b00011000;  // (24); // change enable and WDE - also resets


    WDTCSR = (1<<WDP3) | (1<<WDP0) ; // b00100001;  // (33); // prescalers only, get rid of WDCE and WDE - 8 seconds
    //WDTCSR = (1<<WDP2) | (1<<WDP1) | (1<<WDP0) ; // 2 seconds
    //WDTCSR = (1<<WDP3) ; // 4 seconds
    
    WDTCSR |= (1<<WDIE)  ; // (1<<6); // enable interrupt mode: WDE=0, WDIE=1 (but also requires FUSE WDTON=1)
    //WDTCSR |= (1<<WDE) | (1<<WDIE)  ; // interrupt and system reset
    
      // WDE | WDIE = interrupt and system reset

      // Even when only settng WDIE we get system reset. Do I need to create an interrupt handler
  
    sei();
  
    // enable sleep
    SMCR |= (1<<2); // power down mode
    SMCR |= 1; // enable sleep

    
    // BOD disable - must go to sleep within 4 clock cycles, so inline here
    MCUCR |= (3<<5);  // set boths BODS and BODSE at the same time
    MCUCR = (MCUCR & ~(1<<5)) | (1<<6); // set BODS and clear BODSE
    __asm__ __volatile__("sleep");

    // back from sleep
    DDRB=ddrb;
    DDRC=ddrc;
    DDRD=ddrd;
    PORTB=portb;
    PORTC=portc;
    PORTD=portd;
    disableWatchdogInterrupt();
    enableADC();

  }

#else
  // #############################
  // Not compiling for atmega328p: dummy functions

  void disableWatchdogInterrupt() {
  }

  void doDeepSleep() {
    delay(8000);
  }

#endif

// -------------------------------------------
// Serial output (public)
// -------------------------------------------


void sPrint (char *msg) {
  Serial.print(msg);
}

void sPrintWord (Word word) {
  Serial.print(word);
}

void sPrintByte (Byte b) {
  Serial.print(b);
}


void sPrintln () {
  Serial.println();
}

// -------------------------------------------
// Stacks
// -------------------------------------------



static Long floatToLong (Float f) {
  Long val=0;
  Word *src = (Word *) &f;
  Word *target = (Word *) &val;
  
  target[0]=src[0];
  target[1]=src[1];
  return val;
}

static Float longToFloat (Long val) {
  Float f=0.0;
  Word *src = (Word *) &val;
  Word *target = (Word *) &f;
  
  target[0]=src[0];
  target[1]=src[1];
  return f;
}


static void inline push (Word v) {
  if (dStackNext >= DSTACK_SIZE-1) {
    setHasError();
    Serial.println(F("dStack overflow"));
    return;
  }
  dStack[dStackNext++]=v;
}

static Word inline pop () {
  if (dStackNext==0) {
    setHasError();
    Serial.println(F("dStack underflow"));
    return 0;
  }
  return dStack[--dStackNext];
}

static void inline pushLong (Long val) {
  if (xStackNext >= XSTACK_SIZE-1) {
    setHasError();
    Serial.println(F("xStack overflow"));
    return;
  }
  xStack[xStackNext++]=val;
}

static Long inline popLong () {
  if (xStackNext==0) {
    setHasError();
    Serial.println(F("xStack underflow"));
    return 0;
  }
  return xStack[--xStackNext];
}


static void inline pushFloat (Float f) {
  pushLong(floatToLong(f));
}

static Float popFloat () {
  return longToFloat(popLong());
}



static Word pick (Word n) {
  if (dStackNext-1-n < 0) {
    setHasError();
    Serial.println(F("dStack underflow (pick)"));
    return 0;
  }
  return dStack[dStackNext-1-n];
}

static Word rpick (Word n) {
  if (rStackNext-1-n < 0) {
    setHasError();
    Serial.println(F("rStack underflow (rpick)"));
    return 0;
  }
  return rStack[rStackNext-1-n];
}

static Long pickLong (Word n) {
  if (xStackNext-1-n < 0) {
    setHasError();
    Serial.println(F("xStack underflow (pickLong)"));
    return 0;
  }
  return xStack[xStackNext-1-n];
}

static Float pickFloat (Word n) {
  return longToFloat(pickLong(n));
}



static void rpush (Word v) {
  /* Print("rpush ");
  sPrintWord(v);
  sPrintln(); */
  if (dStackNext >= RSTACK_SIZE-1) {
    setHasError();
    Serial.println(F("rStack overflow"));
    return;
  }
  rStack[rStackNext++]=v;
}

static Word rpop () {
  if (rStackNext==0) {
    setHasError();
    Serial.println(F("rStack underflow"));
    return 0;
  }
  rStackNext--;
  Word v = rStack[rStackNext];
  /* sPrint("rpop ");
  sPrintWord(v);
  sPrintln(); */
  return v;
}

static void showStacks() {
  Serial.println();
  Serial.print(F("Data  ["));
  Byte i;
  for (i=0; i<dStackNext; i++) {
    if (i>0) Serial.print(" ");
    SWord w=(SWord) dStack[i];
    Serial.print(w);
  }
  Serial.println("]");
  
  if (xStackNext == 0) return;

  Serial.print("Long  [");
  for (i=0; i<xStackNext; i++) {
    if (i>0) Serial.print(" ");
    SLong val=(SLong) xStack[i];
    Serial.print(val);
  }
  Serial.println("]");

  Serial.print("Float [");
  for (i=0; i<xStackNext; i++) {
    if (i>0) Serial.print(" ");
    Float f=longToFloat(xStack[i]);
    Serial.print(f);
  }
  Serial.println("]");
}


void op_xdup() {
  pushLong(pickLong(0));
}

void op_xdrop() {
  popLong();
}

void op_xswap() {
  Long b=popLong();
  Long a=popLong();
  pushLong(b);
  pushLong(a);
}

void op_xpick() {
  Word n=pop();
  pushLong(pickLong(n));
  pushLong(pickLong(n));
}

static void dStackClear() {
  dStackNext=0;
}

static void xStackClear() {
  xStackNext=0;
}

// -------------------------------------------
// Error handling
// -------------------------------------------



static void clearHasError() {
  errorFlag=false;
}

void setHasError () {
  if (!errorFlag) {
    errorFlag=true;
    Serial.println();
    for (Byte i=0; i<20; i++) Serial.print("-");
    Serial.println();
    Serial.println(F("Error"));
    for (Byte i=0; i<20; i++) Serial.print("-");
    Serial.println();
    delay(1000);
    clearInputBuffer();
  }
}

Boolean hasError() {
  return errorFlag;
}


void clearInputBuffer() {
  while (Serial.available()) Serial.read();
}

Boolean keyPressed() {
  return (Boolean) Serial.available();
}

Word readSerialChar () {
  for(;;) {
    int ch=Serial.read();
    if (ch >= 0) {
      if (ch==13 || ch==10) {
        if (READC_ECHO) Serial.println();
      }  else {
        if (READC_ECHO) Serial.print((char) ch);
      }
      return (Word) ch;
    }
  }
}


static int commentLevel=0;  // nested parantheses count
void readNextWord () {
  Byte pos=0;

  for(;;) {
    char ch=readSerialChar();
    if (ch=='(') {
      commentLevel++;
      continue;
    } else if (ch==')') {
      commentLevel--;
      if (commentLevel < 0) commentLevel=0;
      continue;
    }
    if (ch==13 || ch==10 || ch==32 || commentLevel>0) {
      if (pos > 0) {
        nextWord[pos]='\0';
        return;
      } // else ignore
    } else {
      if (pos > MAX_WORD_LENGTH-1) {
        setHasError();
        Serial.println(F("word too long"));
        nextWord[pos]='\0';

        return;
      }
      nextWord[pos++]=ch;
    }
  }
}

void op_readNextWord () {
  readNextWord();
}
void op_nextWordEq () {
  Word strRef=pop();
  if (mixedStreq (strRef, nextWord)) push(1); else push(0);
}

void op_compile () {
  compileNextWord();
}

void op_nextWord_write() {
  Word ptr=pop();
  Word len=readByte(ptr);
  if (len>=MAX_WORD_LENGTH) {
    setHasError();
    Serial.print(F("op_nextWord_write: too long "));
    Serial.println(len);
    return;
  }
  for (Word i=0; i<len; i++) {
    nextWord[i]=(char) readByte(ptr+1+i);
  }
  nextWord[len]='\0';
}



// when an op requires additional Bytes (bval and cval - push Byte and cell value)
Byte getOpcodeParameter() {
  return readByteFast(programCounter++);
}





void compileNumberByte (Byte b) {
    compileOut(OP_BVAL);
    compileOut(b);
}

void compileNumberCell (Word w) {
    compileOut(OP_CVAL);
    compileOut((w>>8) & 0xFF);
    compileOut(w & 0xFF);
}

void compileNumber (Word w) {
  if (w < 255) {
    compileNumberByte(w);
  } else {
    compileNumberCell(w);
  }
}


void callForth (Word addr) {
  rpush(programCounter);
  programCounter=addr & ADDR_CODE_MASK;  // strip call bit
  //Serial.print("callForth addr=");
  //Serial.println(programCounter);
}


/*
* Compile nextWord, true if ok, false if error
*/
void compileNextWord () {

  // check for number
  int i=0;
  if (myAtoi(nextWord,&i)) {
    Word w=(Word) i;
    compileNumber(i);
    return;
  } 

  Boolean dictFound=dictLookup(nextWord);
  if (dictFound) {
    byte type=getDeType();
    if (type==DE_TYPE_NORMAL) {
      // ensure 14 bit address, then set high bit=1
      Word addr=generateCallAddress(getDeAddress());
      compileOut((addr>>8) & 0xFF); 
      compileOut(addr & 0xFF);
    } else if (type==DE_TYPE_IMMEDIATE) {
      callForth(getDeAddress());
      executeCode();
    } else if (type==DE_TYPE_CONSTANT) {
      compileNumber(getDeAddress());
    }
    return;
  }

  int opcode=lookupOpCode();
  if (opcode >= 0) {
    compileOut(opcode);
    return;
  }

  setHasError();
  Serial.print(F("Unknown: "));
  Serial.println(nextWord);
}


void create () {
  readNextWord();
  dictCreate(nextWord);
}


// -------
// opcode implemenations
// -------


Boolean getTagNumber (char *ptr, int *tag) {
  if (*ptr == '\0') return false;
  if (*(ptr+1) != '\0') return false; // single digit only
  if (*ptr >= '0' && *ptr <= '4') {
    *tag = *ptr - '0';
    if (*tag < 0 || *tag >= NUM_TAGS_REFS) {
      setHasError();
      Serial.print(F("Unknown tag "));
      Serial.println(*tag);
      return false;
    }
    return true;
  }
  return false;
}

struct Ref {
  Byte tag;
  Word addr;
};

static Word tags[NUM_TAGS_REFS]; 
static struct Ref refs[NUM_TAGS_REFS];


void op_colon() {  

  for (int i=0; i<NUM_TAGS_REFS; i++) {
    tags[i]=0;
  }
  
  Byte nextRef=0;

  // create dictionary entry (CONSTANT address=0)
  create();

  startCompile();
  Word startPos=getCodeNext();

  /*Serial.print("After create codeHERE ");
  Serial.println(codeHERE());*/

  // write dummy length Byte (patched at semicolon)
  // (this enables disassembling)
  compileOut(96);

  for (;;) {
    if (hasError()) return;

    readNextWord();

    // define tag?
    if (*nextWord=='/') {
      int tag=0;
      if (getTagNumber(nextWord+1, &tag)) {
        tags[tag] = getCompileNext();
        continue;
      }
    }

    // tag lookup?
    if (*nextWord=='&') {
      int tag=0;
      if (getTagNumber(nextWord+1, &tag)) {

        compileOut(OP_CVAL);

        refs[nextRef].tag=tag;
        refs[nextRef].addr=getCompileNext();
        // place holder to be patched
        compileOut(0);
        compileOut(0);
        
        nextRef++;
        continue;
      }
    }

    if (!strcmp(nextWord,";")) {
      compileOut(OP_RET);
      confirmCompile();

      // patch tag references
      for (int i=0; i<nextRef; i++) {
        Byte tag=refs[i].tag;
        Word tagAddr=tags[tag];
        if (tagAddr==0) {
          setHasError();
          Serial.print(F("Invalid tag reference: "));
          Serial.println(tag);
          return;
        }
        Word patchAddr=refs[i].addr;

        writeByte(patchAddr, (tagAddr >> 8) & 0xFF);
        writeByte(patchAddr+1, tagAddr & 0xFF);
      }



      Byte byteCount=(Byte) (getCodeNext()-startPos-1);  // length Byte not included

      dictEntryFetch(getDictionaryHead());  // from call to create() 

      printStr(getDeNamePtr());
      Serial.print(" ");
      Serial.print(byteCount);
      Serial.println(F(" Bytes"));

      // patch length Byte
      
      writeByte(startPos, byteCount);

      setDeAddress(generateCodeAddress(startPos+1));  // past length Byte
      setDeType(DE_TYPE_NORMAL);

      // save changes to dict entry
      dictEntrySave();

      return;
    }

    // all other words except semicolon
    compileNextWord();
  }
}


void op_bval() {push(getOpcodeParameter());}
void op_cval() {push(getOpcodeParameter()<<8 | getOpcodeParameter());}

void op_dcall() {     // dynamic call
  Word addr=pop();
  callForth(addr);
}
void op_add() {Word b=pop(); Word a=pop(); push(a+b);}
void op_sub() {Word b=pop(); Word a=pop(); push(a-b);}
void op_mul() {Word b=pop(); Word a=pop(); push(a*b);}
void op_div() {Word b=pop(); Word a=pop(); push(a/b);}
void op_modulo() {Word b=pop(); Word a=pop(); push(a%b);}

void op_gt() {Word b=pop(); Word a=pop(); push(a>b ? 1 : 0);}
void op_ge() {Word b=pop(); Word a=pop(); push(a>=b ? 1 : 0);}
void op_lt() {Word b=pop(); Word a=pop(); push(a<b ? 1 : 0);}
void op_le() {Word b=pop(); Word a=pop(); push(a<=b ? 1 : 0);}
void op_eq() {Word b=pop(); Word a=pop(); push(a==b ? 1 : 0);}
void op_ne() {Word b=pop(); Word a=pop(); push(a!=b ? 1 : 0);}
void op_and() {Word b=pop(); Word a=pop(); push(a != 0 && b != 0 ? 1 : 0);}
void op_or() {Word b=pop(); Word a=pop(); push(a != 0 || b != 0 ? 1 : 0);}
void op_not() {Word x=pop(); push(x==0 ? 1 : 0);}
void op_bin_and() {Word b=pop(); Word a=pop(); push(a&b);}
void op_bin_or() {Word b=pop(); Word a=pop(); push(a|b);}
void op_bin_inv() {Word x=pop(); push(~x);}
void op_lshift() {Word b=pop(); Word a=pop(); push(a<<b);}
void op_rshift() {Word b=pop(); Word a=pop(); push(a>>b);}


void op_dot() {int i=(int) pop(); Serial.print(i); Serial.print(" ");}
void op_dot_u() {Word x=pop(); Serial.print(x); Serial.print(" ");}
void op_dot_hex() {Word x=pop(); Serial.print("0x"); Serial.print(x,16); Serial.print(" ");}
void op_l_dot_u() {Long val=popLong(); Serial.print(val); Serial.print(" ");}
void op_l_dot_s() {SLong val=(SLong) popLong(); Serial.print(val); Serial.print(" ");}
void op_l_dot_hex() {Long x=popLong(); Serial.print("0x"); Serial.print(x,16); Serial.print(" ");}
void op_dot_f() {Serial.print(popFloat()); Serial.print(" ");}
void op_emit() {Word x=pop(); char c=(x&0xFF); Serial.print(c);}

void op_code_next() {Word addr=generateCodeAddress(getCodeNext()); push(addr);}
void op_comp_next() {Word addr=generateCodeAddress(getCompileNext()); push(addr);}
void op_comp_out() {Word x=pop(); compileOut(x & 0xFF);}

void op_HERE() {push(HERE());}

void op_allot() {
  Word count=pop();
  dataAllot(count);
}

void op_write() {Word addr=pop(); Word value=pop(); writeWord(addr,value);}
void op_read() {Word addr=pop(); push(readWord(addr));}
void op_writec() {Word addr=pop(); Word x=pop(); writeByte(addr,x);}
void op_readc() {Word addr=pop(); push(readByte(addr));}

void op_ret() {programCounter=rpop();}
void op_cond_ret() {Word cond=pop(); if (cond != 0) programCounter=rpop();}

void op_create() {
  create();
}

void op_star_dictHead() {
  push(getDictionaryHead());
}

void op_dup() {push(pick(0));}
void op_swap() {Word b=pop(); Word a=pop(); push(b); push(a);}
void op_drop() {pop();}
void op_pick() {Word n=pop(); push(pick(n));}


void op_show_stack() {
  showStacks();
}

void op_clear_stacks() {
  dStackClear();
  xStackClear();
}

// Speed testing

unsigned long testStart=0L;
unsigned long testCount=0L;

void op_start_test () {
  testStart=millis();
  testCount=instructionCount;
}

void op_end_test () {
  Serial.println();
  unsigned long duration=millis()-testStart;
  unsigned long count=instructionCount-testCount;
  double seconds=duration/1000.0;
  Serial.print(F("op/s "));
  double op_per_s = count/seconds;
  Serial.println(op_per_s);
}



void op_words() {
  Word ptr=getDictionaryHead();
  Serial.println();
  Byte len=0;
  while (ptr != 0) {
    dictEntryFetch(ptr);
    printStr(getDeNamePtr());
    len += readByte(getDeNamePtr());
    if (len > 60) {
      Serial.println();
      len=0;
    } else {
      Serial.print(" ");
    }
    ptr=getDeNextPtr();
  }
  Serial.println();
}
void op_to_r() {
  rpush(pop());
}
void op_r_from() {
  push(rpop());
}

void op_r_pick() {
  Word n=pop();
  rpush(rpick(n));
}

void op_jmp() {
  programCounter=pop();
}
void op_cond_jmp() {
  Word addr=pop();
  Word cond=pop();
  if (cond != 0) programCounter=addr;
}

void op_blob() {
  // followed by length field n, then n Bytes
  // skip the data, and push the address of the length Byte 
  Word lengthPointer = programCounter;
  Byte length=getOpcodeParameter();
  programCounter += length;
  // convert to address by setting high bit
  push(generateCodeAddress(lengthPointer));
}

void op_key_check() {
  push(keyPressed() ? 1 : 0);
}

void op_key() {
  push(readSerialChar());
}

void op_word_addr() {
  readNextWord();
  if (dictLookup(nextWord)) {
    push(generateCodeAddress(getDeAddress()));
  } else {
    push(0);
  }
}


// all ops names separated by space
// upate Words script and and run "gen" to get this code
// "Global variables use 1559 bytes"

// ---------------------------------------------------------------------------+

const Byte numOps=113;

static const PROGMEM char opNames[]="\
create \
: \
bval \
cval \
ret \
jmp \
jmp? \
blob \
*dictHead \
dcall \
ret? \
+ \
- \
* \
/ \
% \
> \
>= \
< \
<= \
== \
!= \
and \
or \
not \
& \
| \
inv \
<< \
>> \
. \
.u \
.hex \
L. \
L.s \
L.hex \
F. \
emit \
code.next \
comp.next \
comp.out \
HERE \
allot \
! \
@ \
c! \
c@ \
dup \
swap \
drop \
pick \
? \
.s \
clear \
[test \
test] \
>R \
R> \
rpick \
key? \
key \
' \
dump \
code.export \
step \
dis \
ops \
delay \
delay-us \
deep-sleep8 \
free-mem \
Pin.modeOut \
Pin.modeIn \
Pin.modeInPullup \
Pin.writeDigital \
Pin.writeAnalog \
Pin.pulseDigitalMs \
Pin.pulseDigitalUs \
Pin.readDigital \
Pin.readAnalog \
EE.length \
EE.write \
EE.read \
I2C.masterWrite \
I2C.masterWWait \
I2C.masterRead \
xdup \
xdrop \
xswap \
xpick \
>F \
F> \
F>L \
F+ \
F- \
F* \
F/ \
>L \
L>F \
L> \
L+ \
L- \
L* \
L/ \
L% \
L<< \
L>> \
hw@ \
hw! \
readNextWord \
nextWordEq \
compile \
nextWord! \
";

typedef void (*FUNC)();

static const PROGMEM FUNC opFunctions[]={
&op_create
,&op_colon
,&op_bval
,&op_cval
,&op_ret
,&op_jmp
,&op_cond_jmp
,&op_blob
,&op_star_dictHead
,&op_dcall
,&op_cond_ret
,&op_add
,&op_sub
,&op_mul
,&op_div
,&op_modulo
,&op_gt
,&op_ge
,&op_lt
,&op_le
,&op_eq
,&op_ne
,&op_and
,&op_or
,&op_not
,&op_bin_and
,&op_bin_or
,&op_bin_inv
,&op_lshift
,&op_rshift
,&op_dot
,&op_dot_u
,&op_dot_hex
,&op_l_dot_u
,&op_l_dot_s
,&op_l_dot_hex
,&op_dot_f
,&op_emit
,&op_code_next
,&op_comp_next
,&op_comp_out
,&op_HERE
,&op_allot
,&op_write
,&op_read
,&op_writec
,&op_readc
,&op_dup
,&op_swap
,&op_drop
,&op_pick
,&op_words
,&op_show_stack
,&op_clear_stacks
,&op_start_test
,&op_end_test
,&op_to_r
,&op_r_from
,&op_r_pick
,&op_key_check
,&op_key
,&op_word_addr
,&op_dump
,&op_code_export
,&op_step
,&op_dis
,&op_ops
,&op_delay
,&op_delay_us
,&op_deep_sleep_8s
,&op_free_mem
,&natPinModeOut
,&natPinModeIn
,&natPinModeInPullup
,&natPinWriteDigital
,&natPinWriteAnalog
,&natPinPulseDigitalMs
,&natPinPulseDigitalUs
,&natPinReadDigital
,&natPinReadAnalog
,&natEELength
,&natEEWrite
,&natEERead
,&natI2CmasterWrite
,&natI2CmasterWWait
,&natI2CmasterRead
,&op_xdup
,&op_xdrop
,&op_xswap
,&op_xpick
,&op_sw_to_f
,&op_f_to_sw
,&op_f_to_sl
,&op_f_add
,&op_f_sub
,&op_f_mul
,&op_f_div
,&op_sw_to_wl
,&op_sl_to_f
,&op_sl_to_sw
,&op_l_add
,&op_l_sub
,&op_l_mul
,&op_l_div
,&op_l_mod
,&op_l_lshift
,&op_l_rshift
,&op_hw_read
,&op_hw_write
,&op_readNextWord
,&op_nextWordEq
,&op_compile
,&op_nextWord_write
};

// --------------------------------------------------------------------------

void op_ops() {
  Byte length=0;
  Word i=0;
  sPrintln();
  for(;;) {
    Byte ch=pgm_read_byte(opNames+i);
    if (ch==0) {Serial.println(); return;}
    printChar(ch);
    length++;
    i++;
    if (length > 60 && ch==' ') {
      Serial.println();
      length=0;
    }
  }
}






void op_dump() {
  memDump();
}

void op_code_export() {
  memCodeExport();
}

void op_step() {
  programCounter=pop();
  Serial.print(F("programCounter="));
  Serial.println(programCounter);
  showStacks();

  Byte op=readByteFast(programCounter++);
  Serial.print(F("op="));
  Serial.println(op);

  executeCodeByte(op);
  Serial.print(F("programCounter="));
  Serial.println(programCounter);
  showStacks();
  push(programCounter);

  // prevent auto processing
  programCounter=0;
}

void op_dis() {
  Word codeAddr = pop();
  Byte len=readByte(codeAddr-1);
  Serial.println();
  Serial.print(F("codeAddr="));
  Serial.print(codeAddr);
  Serial.print(F(" length="));
  Serial.println(len);

  Byte dataBytes=0;
  for (Byte i=0; i<len; i++) {
    Word addr=codeAddr + i;
    Serial.print(addr);
    Serial.print("  ");

    Byte op=readByte(addr);
    Serial.print(op);
  
    if (dataBytes > 0) {
      Serial.print(F(" (data)"));
      if (op > 32 && op < 127) {
        Serial.print(" ");
        printChar(op);
      }
      Serial.println();
      dataBytes--;
      continue;
    }

    Serial.print(" ");
    if (op & BYTE_CALL_BIT) {
      dataBytes=1;
      Serial.print(F("(call)"));
      Word forthAddr=(op << 8) | readByte(addr+1);
      // strip away topmost 2 bits
      forthAddr=forthAddr & ADDR_CODE_MASK;
      Serial.print(F(" forthAddr="));
      Serial.print(forthAddr);

      Boolean found=dictLookupByAddr(forthAddr);
      if (!found) {
        setHasError();
        Serial.print(F("unknown forth code address "));
        Serial.println(getDeAddress(),16);
        return;
      } 
      Serial.print(" ");
      Serial.print(F("=>"));
      Serial.print(" ");
      printStr(getDeNamePtr());
      Serial.println();

      Byte type=getDeType();
      if (type != DE_TYPE_NORMAL && type != DE_TYPE_IMMEDIATE) {
        setHasError();
        Serial.println(" constant - not callable!");
        return;
      }
      continue;      
    }
  
    // op = opcode 
    printOpNameByPos(op);
    if (op==OP_BVAL) {
      Word val=readByte(addr+1);
      Serial.print(" ");
      Serial.print(val);
      Serial.print(" ");
      Serial.print("0x");
      Serial.print(val,16);
      dataBytes=1;
    } else if (op==OP_CVAL) {
      dataBytes=2;
      Word val=(readByte(addr+1) << 8) | readByte(addr+2);
      Serial.print(" ");
      Serial.print(val);
      Serial.print(" ");
      Serial.print("0x");
      Serial.print(val,16);
    } else if (op==OP_BLOB) {
      dataBytes=readByte(addr+1)+1;
    }
    Serial.println();
  }
}

// # NATIVE calls from RForth

void op_delay() {
  Word ms=pop();
  delay(ms);
}

void op_delay_us() {
  Word us=pop();
  delayMicroseconds(us);
}

void op_deep_sleep_8s() {
  doDeepSleep();
}

void op_free_mem() {
  showFreeMem();
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

// ----------------
// Onboard EEPROM
// ----------------

void natEELength () { // ( -- length )
  push((Word) EEPROM.length());
}

void natEEWrite () { // ( byte index -- ) 
  Word pos=pop();
  Word value=pop();
  EEPROM.write(pos,value);
}

void natEERead () { // ( index -- byte ) 
  Word pos=pop();
  push(EEPROM.read(pos));
}



// -------------------------------
// I2C.*
// -------------------------------

// (sendBufPtr sendCount addr -- )
void natI2CmasterWrite() {
  Word addr=pop();
  Word sendBuf=pop();

  Word sendCount=readByte(sendBuf);

  Wire.beginTransmission((byte) addr);
  for (byte i=0; i<sendCount; i++) {
    Wire.write((byte)readByte(sendBuf+i+1));
  }
  Wire.endTransmission();
}

// EEPROM's are sometimes slow at doing page writes etc (thank you, ChatGPT)
void natI2CmasterWWait () {
  Word addr=pop();  
  while (true) {
    Wire.beginTransmission((int) addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) break;  // ACK received
    delay(1);            // Small delay to avoid hammering the bus
  }
}

// (recvBufPtr addr -- )
// reads length from buffer, attempts to read that many bytes
// updates buffer length when done, with actual read count
void natI2CmasterRead() {
  Word addr=pop();
  Word recvBuf=pop();
  // desired count
  Word count=readByte(recvBuf);

  Wire.requestFrom((int) addr, (int) count);
  Word i=0;
  while (i<count && Wire.available()) {
    byte b = Wire.read();
    writeByte(recvBuf+i+1, b);
    i++;
  }
  writeByte(recvBuf,count);
}


// -----------------------------------------
// Float and Long
// -----------------------------------------

void op_sw_to_f () {
  SWord val=(SWord) pop();
  Float f=(Float) val;
  pushFloat(f);
}

void op_f_to_sw () {
  Float f=popFloat();
  SWord val=(SWord) f;
  push(val);
}


void op_f_to_sl () {
  Float f=popFloat();
  SLong x=(SLong) f;
  pushLong(x);
}

void op_f_add () {
  Float b=popFloat();
  Float a=popFloat();
  pushFloat(a+b);
}

void op_f_sub () {
  Float b=popFloat();
  Float a=popFloat();
  pushFloat(a-b);
}

void op_f_mul () {
  Float b=popFloat();
  Float a=popFloat();
  pushFloat(a*b);
}

void op_f_div () {
  Float b=popFloat();
  Float a=popFloat();
  pushFloat(a/b);
}

void op_sw_to_wl () {
  // signed
  SWord x=(SWord) pop();
  pushLong((SLong) x);
}

void op_sl_to_f () {
  SLong x=(SLong) popLong();
  Float f=(Float) x;
  pushFloat(f);
}

void op_sl_to_sw () {
  SLong val=(SLong) popLong();
  Boolean negative=false;
  if (val<0) {
    negative=true;
    val=-val;
  }
  SWord w=(SWord) val;
  if (negative) w=-w;
  push(w);
}

void op_l_add () {
  Long b=popLong();
  Long a=popLong();
  pushLong(a+b);
}

void op_l_sub () {
  Long b=popLong();
  Long a=popLong();
  pushLong(a-b);
}

void op_l_mul () {
  Long b=popLong();
  Long a=popLong();
  pushLong(a*b);
}

void op_l_div () {
  Long b=popLong();
  Long a=popLong();
  pushLong(a/b);
}

void op_l_mod () {
  Long b=popLong();
  Long a=popLong();
  pushLong(a%b);
}

void op_l_lshift () {
  Word n=pop();
  Long val=popLong();
  pushLong(val<<n);
}

void op_l_rshift () {
  Word n=pop();
  Long val=popLong();
  pushLong(val>>n);
}


// -----------------------------------------
// C memory access (registers)
// -----------------------------------------

void op_hw_read () {
  Word addr=pop();
  Byte *loc=(Byte *) addr;
  push(*loc);
}

void op_hw_write () {
  Word addr=pop();
  Byte value=(Byte) (pop() & 0xFF);
  Byte *loc=(Byte *) addr;
  *loc=value;
}


// Check if string in opNames PROGMEM array starting at pos matches the given string
static boolean opsNameMatch (Word pos, char *s) {
  Byte len=strlen(s);
  for (Byte i=0; i<len; i++) {
    char ch=pgm_read_byte(&(opNames[pos+i]));
    if (ch != s[i]) return false;
  }
  // all characters match, ensure that next character in PROGMEM buffer is
  // blank or 0
  Byte ch=pgm_read_byte(&(opNames[pos+len]));
  return (ch==32 || ch==0);
}

// used by disassembler
void printOpNameByPos(Byte op) {
  Word pos=0;
  while (op>0) {
    pos=opsSkipWord(pos);
    op--;
  }
  for(;;) {
    Byte ch=pgm_read_byte(&(opNames[pos]));
    if (ch==32 || ch==0) return;
    printChar(ch);
    pos++;
  }
}



   


Word opsSkipWord (Word pos) {
  for(;;) {
    Byte ch=pgm_read_byte(&(opNames[pos]));
    if (ch==32) return pos+1;
    if (ch==0) return pos;  // end of PROGMEM area
    pos++;
  }
  return 9999;  // should not happen
}



// check if nextWord is the name of an op, return index or -1 if not found
int lookupOpCode () {
  Word pos=0; // in opNames PROGMEM string
  int wordNumber=0;
  while (wordNumber < numOps) {
    if (opsNameMatch(pos,nextWord)) {
      return wordNumber;
    }
    pos=opsSkipWord(pos);
    wordNumber++;
  }
  return -1;
}

void executeCodeByte (Byte b) {
  // detect high bit set, this indicates a Forth call address (14 bits)

  if (b & BYTE_CALL_BIT) {
    Word address = (b<<8) | getOpcodeParameter();
    /*Serial.print("call=");
    Serial.println(address);*/
    callForth(address);
  } else {
    // opcode
    Word pointer=pgm_read_word(&(opFunctions[b]));
    ((FUNC) pointer)();
  }
  instructionCount++;
}


void executeCode() {
  while (programCounter != 0) {
    if (hasError()) {
      programCounter=0;
      return;     
    }
    executeCodeByte(readByteFast(programCounter++));
  }
}


// interpreting main loop
void loop() {
  clearHasError();

  if (programCounter != 0) {
    executeCode();
    if (hasError()) {
      memDump();
      for(;;);
    }
    Serial.println();
    Serial.println("Ok");
  }
# ifdef MONITOR_C_STACK
  Word maxCStackSize=getCStackMaxSize();
  Serial.print(F("Max C stack: "));
  Serial.println(maxCStackSize);
# endif

  readNextWord();

  int i=0;
  if (myAtoi(nextWord,&i)) {
    push ((Word) i);
    return;
  }

  if (dictLookup(nextWord)) {
    if (getDeType()==DE_TYPE_CONSTANT) {
      push(getDeAddress());
    } else {
      callForth(getDeAddress());
    }
    return;
  }
  
  int op = lookupOpCode();
  if (op>=0) {
    Word func=pgm_read_word(&(opFunctions[op]));
    ((FUNC) func)();
    return;
  }
  setHasError();
  Serial.print(F("Unknown word: "));
  Serial.println(nextWord);
}
