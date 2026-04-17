#define ATMEGA328p

#include <avr/pgmspace.h>
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

void setup() {
  Serial.begin(9600);
  Serial.print(F_CPU / 1000000.0);
  Serial.println(F(" MHz"));

  memInit();
  Serial.println(F("Use '?' to show words and 'ops' to show ops (written in C)"));


  // copy Dictionary Header pointer from static data segment lower two bytes to RAM
  // (note address 0 in code segment is reserved, so data are in location 1 and 2)

  dataAllot(2);
  Word srcAddr=generateCodeAddress(1);
  Word dataAddr=generateDataAddress(0); // points into the static part
  Serial.print(F("Dictionary head: 0x"));
  Serial.println(dataAddr,16);

  writeByte(dataAddr, readByte(srcAddr));  
  writeByte(dataAddr+1, readByte(srcAddr+1));  

}


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

void sPrintDouble (double d) {
  Serial.print(d);
}

void sPrintln () {
  Serial.println();
}


// -------------------------------------------
// Stacks
// -------------------------------------------


static void inline push (Word v) {
  if (dStackNext >= DSTACK_SIZE-1) {
    setHasError();
    sPrint("overflow");
    sPrint(" ");
    sPrint("data");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return;
  }
  dStack[dStackNext++]=v;
}

static Word inline pop () {
  if (dStackNext==0) {
    setHasError();
    sPrint("underflow");
    sPrint(" ");
    sPrint("data");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return 0;
  }
  return dStack[--dStackNext];
}

static Word pick (Word n) {
  if (dStack-1-n < 0) {
    setHasError();
    sPrint("underflow");
    sPrint(" ");
    sPrint("data");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return 0;
  }
  return dStack[dStackNext-1-n];
}

static void rpush (Word v) {
  /* Print("rpush ");
  sPrintWord(v);
  sPrintln(); */
  if (dStackNext >= RSTACK_SIZE-1) {
    setHasError();
    sPrint("overflow");
    sPrint(" ");
    sPrint("return");
    sPrint(" ");
    sPrint("stack");
    sPrintln();
    return;
  }
  rStack[rStackNext++]=v;
}

static Word rpop () {
  if (rStackNext==0) {
    setHasError();
    sPrint("underflow");
    sPrint(" ");
    sPrint("return");
    sPrint(" ");
    sPrint("stack");
    sPrintln();

    return 0;
  }
  rStackNext--;
  Word v = rStack[rStackNext];
  /* sPrint("rpop ");
  sPrintWord(v);
  sPrintln(); */
  return v;
}

static void dStackShow() {
  sPrintln();
  sPrint("stack");
  sPrint(" ");
  sPrint(":");
  sPrint(" ");
  sPrint("[");
  for (Byte i=0; i<dStackNext; i++) {
    if (i>0) sPrint(" ");
    sPrintWord(dStack[i]);
  }
  sPrint("]");
  sPrintln();
}

static void dStackClear() {
  dStackNext=0;
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



// when an op requires additional Bytes (bval and cval - push Byte and cell value)
Byte getOpcodeParameter() {
  return readByteFast(programCounter++);
}





void compileNumberByte (Byte b) {
    if (b==0) {
      compileOut(OP_ZERO);
    } else if (b==1) {
      compileOut(OP_ONE);
    } else {
      compileOut(OP_BVAL);
      compileOut(b);
    }
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
  /*
  Serial.println();
  Serial.print("compileNextWord ");
  Serial.print(nextWord);
  Serial.print(" compileNext=");
  Serial.println(compileNext);
  */
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
        /*
        Serial.println();
        Serial.print("*** tag ");
        Serial.print(tag);
        Serial.print("=");
        Serial.println(compileNext);
        */
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
        /*
        Serial.println();
        Serial.print("*** ref ");
        Serial.print(tag);
        Serial.print(" at ");
        Serial.println(compileNext);
        */
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
        /*
        Serial.println();
        Serial.print("*** PATCHING ref ");
        Serial.print(tag);
        Serial.print(" at ");
        Serial.print(patchAddr);
        Serial.print(" -> ");
        Serial.println(tagAddr);
        */

        writeByte(patchAddr, (tagAddr >> 8) & 0xFF);
        writeByte(patchAddr+1, tagAddr & 0xFF);
      }



      Byte byteCount=(Byte) (getCodeNext()-startPos-1);  // length Byte not included

      dictEntryFetch(getDictionaryHead());  // from call to create() 

      /*Serial.println();
      Serial.print("getDeNamePtr: ");
      Serial.println(getDeNamePtr());*/
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
void op_zero() {push(0);}
void op_one() {push(1);}
void op_add() {Word b=pop(); Word a=pop(); push(a+b);}
void op_sub() {Word b=pop(); Word a=pop(); push(a-b);}
void op_mul() {Word b=pop(); Word a=pop(); push(a*b);}
void op_div() {Word b=pop(); Word a=pop(); push(a/b);}
void op_modulo() {Word b=pop(); Word a=pop(); push(a%b);}
void op_incr() {Word x=pop(); push(x+1);}

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


void op_cr() {Serial.println();}
void op_dot() {int i=(int) pop(); Serial.print(i); Serial.print(" ");}
void op_dot_u() {Word x=pop(); Serial.print(x); Serial.print(" ");}
void op_dot_hex() {Word x=pop(); Serial.print("0x"); Serial.print(x,16); Serial.print(" ");}
void op_emit() {Word x=pop(); char c=(x&0xFF); Serial.print(c);}
void op_dot_str() {Word addr=pop(); printStr(addr); }

void op_code_here() {Word addr=generateCodeAddress(getCodeNext()); push(addr);}
void op_comp_next() {Word addr=generateCodeAddress(getCompileNext()); push(addr);}
void op_comp_out() {Word x=pop(); compileOut(x & 0xFF);}

void op_HERE() {push(HERE());}

void op_allot() {
  Word count=pop();
  dataAllot(count);
}

void op_constant() {
  Word value=pop();
  create();
  dictEntryFetch(getDictionaryHead());

  setDeType(DE_TYPE_CONSTANT);
  setDeAddress(value);

  dictEntrySave();
}

void op_variable() {
  Word variableAddr=HERE(); 
  dataAllot(2); 
  writeWord(variableAddr,pop());
  push(variableAddr);
  op_constant();
}

void op_write() {Word addr=pop(); Word value=pop(); writeWord(addr,value);}
void op_read() {Word addr=pop(); push(readWord(addr));}
void op_writeb() {Word addr=pop(); Word x=pop(); writeByte(addr,x);}
void op_readb() {Word addr=pop(); push(readByte(addr));}

void op_ret() {programCounter=rpop();}
void op_cond_ret() {Word cond=pop(); if (cond != 0) programCounter=rpop();}

void op_create() {
  create();
}
void op_immediate() {
  Word head=getDictionaryHead();
  if (head==0) return;
  dictEntryFetch(head);
  setDeType(DE_TYPE_IMMEDIATE);
  dictEntrySave();
}

void op_dup() {push(pick(0));}
void op_swap() {Word b=pop(); Word a=pop(); push(b); push(a);}
void op_2dup() {push(pick(1)); push(pick(1));}
void op_drop() {pop();}
void op_over() {push(pick(1));}
void op_pick() {Word n=pop(); push(pick(n));}


void op_show_stack() {
  dStackShow();
}

void op_clear_stack() {
  dStackClear();
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

void op_key() {
  push(keyPressed() ? 1 : 0);
}

void op_readc() {
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

// --------------------------------------------------------------------------------

const Byte numOps=86;

static const PROGMEM char opNames[]="\
create \
: \
bval \
cval \
ret \
jmp \
jmp? \
blob \
zero \
one \
constant \
variable \
immediate \
dcall \
ret? \
+ \
- \
* \
/ \
% \
1+ \
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
cr \
. \
.u \
.hex \
emit \
.str \
code.HERE \
comp.next \
comp.out \
HERE \
allot \
! \
@ \
b! \
b@ \
dup \
swap \
2dup \
drop \
over \
pick \
? \
.s \
clear \
[test \
test] \
>R \
R> \
key \
readc \
' \
dump \
code.export \
step \
dis \
ops \
delay \
delay_us \
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
,&op_zero
,&op_one
,&op_constant
,&op_variable
,&op_immediate
,&op_dcall
,&op_cond_ret
,&op_add
,&op_sub
,&op_mul
,&op_div
,&op_modulo
,&op_incr
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
,&op_cr
,&op_dot
,&op_dot_u
,&op_dot_hex
,&op_emit
,&op_dot_str
,&op_code_here
,&op_comp_next
,&op_comp_out
,&op_HERE
,&op_allot
,&op_write
,&op_read
,&op_writeb
,&op_readb
,&op_dup
,&op_swap
,&op_2dup
,&op_drop
,&op_over
,&op_pick
,&op_words
,&op_show_stack
,&op_clear_stack
,&op_start_test
,&op_end_test
,&op_to_r
,&op_r_from
,&op_key
,&op_readc
,&op_word_addr
,&op_dump
,&op_code_export
,&op_step
,&op_dis
,&op_ops
,&op_delay
,&op_delay_us
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
};

// --------------------------------------------------------------------------------




void op_ops() {
  Byte length=0;
  Word i=0;
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
  dStackShow();

  Byte op=readByteFast(programCounter++);
  Serial.print(F("op="));
  Serial.println(op);

  executeCodeByte(op);
  Serial.print(F("programCounter="));
  Serial.println(programCounter);
  dStackShow();
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





// Check if string in opNames PROGMEM array starting at pos matches the given string
static boolean opNameMatch (Word pos, char *s) {
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
    pos=opSkipWord(pos);
    op--;
  }
  for(;;) {
    Byte ch=pgm_read_byte(&(opNames[pos]));
    if (ch==32 || ch==0) return;
    printChar(ch);
    pos++;
  }
}



   


Word opSkipWord (Word pos) {
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
    if (opNameMatch(pos,nextWord)) {
      return wordNumber;
    }
    pos=opSkipWord(pos);
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
