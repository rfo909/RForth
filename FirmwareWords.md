Firmware words
--------------

2025-10-29

In addition to the Forth words that are assembly op's from the virtual
bytecode machine architecture, a number of useful words are made available
from the ACode.txt file, where the initial dictionary is created.

These fall into three categories.

- Standard forth words, implemented in ACode
- System words, implemented in ACode
- Useful addresses from ACode

Standard Forth words
--------------------
Although adopted from traditional or standard Forth's, the implementation
is not necessarily standard, stemming from the memory model in particular, 
where compilation works with a CompileBuf, and neither COLON nor CREATE actually
create a dictionary entry, as that is handled by the SEMICOLON word.

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
```

System words
------------
As with the standard Forth words above, these are also implemented in ACode, but
they are not attempted borrowed from any standard (that I know of).

```
Dict                Dict <name> -- create dictionary
DictUse             <Dict> DictUse -- use custom dictionary
DictClear           stop using custom dictionary
IN                  IN <DictName> <WordName> -- call word in custom Dict
=>                  => <name> -- create or update local variable inside colon def
.W                  .W <word> -- prints address of Dictionary Entry for word (in hex)
?C                  (str -- addr) 'str ?C -- returns address of code for word (on stack)
BufReset            (bufPtr -- ) Write a 0 in the first byte (length)
BufAdd              (byte bufPtr -- ) Append byte to buf
ShowBuffer          (bufPtr size -- ) Raw dump of buffer bytes
BufCopy             (src target -- ) Copy zero-byte-length buffer to another buffer
BufCreateCopy       (src -- ptr) Allocate memory for content in src buffer, copy it, return pointer
PreCompile          (--) Initiate compile mode
PostCompile         (--) Cancel compile mode 
EmitNumber          (value digits --) -- Add byte code for value to &CompileBuf, use digits=0 for lowest possible byte count
EmitByte            (byte --) -- Add byte to &CompileBuf, typically op's
GetNextWord         (--) -- Reads next word from input, store in &NextWord
SetCompilingWord    (str --) -- Copy string into &CompilingWord
>>str               (ptr -- ptr) -- advance pointer past string or buffer (length byte + 1)
NATIVE              NATIVE <name> -- call native function, use NATIVE ? to list 
EMPTY-WORD          (--) -- creates empty dictionary entry for normal word
PCREATE             (str--) -- same as CREATE except takes word as string
```

(See also the [Bytecode instruction set](InstructionSet.md) which contains base functionality, like add,
sub, print, cr etc)

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

