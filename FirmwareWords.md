Firmware words
--------------

2025-11-09

In addition to the Forth words that are assembly op's from the virtual
bytecode machine architecture, a number of useful words are made available
from the ACode.txt file, where the initial dictionary is created.

These fall into two categories:

- System words, implemented in ACode (some of which are standard Forth, some not)
- Useful addresses from ACode

System words
------------
These are implemented in ACode.

```
:                   COLON -- initiate compile mode for word
;                   SEMICOLON -- complete compile mode and create dictionary entry for new word
.                   DOT -- pop value, print it on signed decimal form
IF ELSE THEN        <condition> IF ... [ELSE ...] THEN ...
BEGIN AGAIN?        BEGIN ... <condition> AGAIN?
CONSTANT            <value> CONSTANT <name>
VARIABLE            <value> VARIABLE <name>
CELL                return cell size (2 bytes)
CELLS               CELL mul
CREATE DOES>        compile only ...
,                   COMMA -- pop value, allocate CELL, write it there
.s                  show stack content
?                   show dictionary
words               show dictionary
IMMEDIATE           makes newest word immediate


Dict                Dict <name> -- create dictionary
DictUse             (Dict -- ) -- Use custom dictionary
DictClear           stop using custom dictionary
IN                  IN <DictName> <WordName> -- call word in custom Dict
=>                  => <name> -- create or update local variable inside colon def
.W                  .W <word> -- prints address of Dictionary Entry for word (in hex)
?C                  (str -- addr) 'str ?C -- returns address of code for word (on stack)
Save                (--) Save state to EEPROM on address 0, see ACode.txt for defs (EE_*)
Load                (--) Load state from EEPROM on address 0, see ACode.txt for defs (EE_*)
BufReset            (bufPtr -- ) Write a 0 in the first byte (length)
BufAdd              (byte bufPtr -- ) Append byte to buf
BufAddC             (cell bufPtr -- ) Append two byte cell value to buffer
BufShow             (size bufPtr -- ) Raw dump of buffer bytes
BufCopy             (src target -- ) Copy buffer to another buffer
BufCreateCopy       (src -- ptr) Allocate memory for content in src buffer, copy it, return pointer
PreCompile          (--) Initiate compile mode
PostCompile         (--) Cleanup compile mode 
EmitNumber          (value digits -- ) Add byte code for value to &CompileBuf, use digits=0 for lowest possible byte count
EmitByte            (byte -- ) Add byte to &CompileBuf, typically op's
GetNextWord         (--) Reads next word from input, store in &NextWord
SetCompilingWord    (str -- ) Copy string into &CompilingWord
>>str               (ptr -- ptr) advance pointer past string or buffer (length byte + 1)
NATIVE              NATIVE <name> -- call native function, use NATIVE ? to list 
EMPTY-WORD          (--) Creates empty dictionary entry for normal word
PCREATE             (str -- ) Same as CREATE except takes word as string parameter
```

(See also the [Bytecode instruction set](InstructionSet.md) which contains base functionality such
as "+" and "-", "cr" etc)


Useful addresses 
----------------
A number of both memory areas are allocated in the ACode file. When the program starts,
all data above a special :PROTECT tag, are copied into the Forth heap in RAM. Addresses
below :PROTECT are mapped to flash, and above, mapped to Forth heap array (in C).

Some of these require intimate knowledge of how RFOrth compiles code. The various buffers
come with both a begin and end-address, in order to calculate their sizes. The rule for
using these in code, is that it is okay as long as not compiling, but note also that
the CompileBuf will be corrupted when running assembly op's from the command line, such
as add or print.

```
Status fields
-------------
&DictionaryHead              (cell)
&DebugFlag                   (byte) 0/1
&IsCompiling                 (byte) 0/1
&CompilingWordMode           (byte) 0=NORMAL 1=IMMEDIATE 2=INSTR 3=CONSTANT
&CREATE-HERE                 (word) points to data to be used by DOES> 

Individual buffers
------------------
&CompilingWord
&CompilingWordEnd
&CompileBuf 
&CompileBufEnd
&NextWord
&NextWordEnd
&LVBuf
&LVBufEnd
&CompileStack
&CompileStackEnd

All buffers as one
------------------
&AllBuffers
&AllBuffersEnd 
```

The &CompileBuf, &LVBuf, &NextWord and &CompilingWord can be used as independent buffers, but
they are located next to each other, and can alternatively be used via the &AllBuffers and &AllBuffersEnd
addresses.

Buffers and strings
-------------------
When referring to buffers and strings, these are assumed to have the length in the first byte,
and the data following. This means max length is 255 bytes. 


CREATE DOES>
------------
The CREATE stores the value of HERE in the &CREATE-HERE cell-sized field, so that any allot's following
CREATE gets pointed to by that value, and picked up by DOES>.

If we want to supply a pointer to data, instead of allot'ing memory after CREATE, we do this as follows:
```
: incr (ptr) CREATE &CREATE-HERE ! DOES> (... [ptr]) ... ;
```

The CREATE stores HERE into the &CREATE-HERE, but immediately following it, instead of allot'ing
memory, we just overwrite the value in &CREATE-HERE. Then follows DOES> and after it, a specification
of custom parameters "..." followed by the ptr, which is supplied automatically for the does code.



