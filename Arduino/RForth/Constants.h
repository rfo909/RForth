#define DSTACK_SIZE       30    // data stack
#define RSTACK_SIZE       30    // return stack
#define FSTACK_SIZE       20    // return stack frames: each frame is 2 values

#define WORDSIZE    2

#define WORD_INVALID      0xFFFF
#define WORD_NULL         0

#define RAM_SIZE          4096

typedef unsigned int Word;

// Assembler:NNTDoc

const char *opNames[] = {
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"and",  // 38
"<undefined>",
"<undefined>",
"<undefined>",
"mul",  // 42
"add",  // 43
"<undefined>",
"sub",  // 45
"<undefined>",
"div",  // 47
"PANIC",  // 48
"atoi",  // 49
"n2code",  // 50
"cget",  // 51
"cset",  // 52
"<undefined>",
"rfwd?",  // 54
"rback?",  // 55
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"lt",  // 60
"eq",  // 61
"gt",  // 62
"<undefined>",
"<undefined>",
"<undefined>",
"streq",  // 66
".str",  // 67
"memcpy",  // 68
"ge",  // 69
"le",  // 70
"readb",  // 71
"writeb",  // 72
"@",  // 73
"!",  // 74
"allot",  // 75
"printb",  // 76
"<undefined>",
"<undefined>",
"cpush",  // 79
"PC",  // 80
"call",  // 81
"ret",  // 82
"jmp",  // 83
"jmp?",  // 84
"printc",  // 85
"<undefined>",
"print",  // 87
"cr",  // 88
"print#",  // 89
"halt",  // 90
"<undefined>",
"<undefined>",
"<undefined>",
"<undefined>",
"u2spc",  // 95
"<undefined>",
"native",  // 97
"print#s",  // 98
"nativec",  // 99
"1+",  // 100
"<undefined>",
"<undefined>",
"HERE",  // 103
"ne",  // 104
"not",  // 105
"drop",  // 106
"wordsize",  // 107
"dup",  // 108
"swap",  // 109
"W+",  // 110
"over",  // 111
"dump",  // 112
"andb",  // 113
"orb",  // 114
"inv",  // 115
"<<",  // 116
">>",  // 117
"readc",  // 118
"clear",  // 119
"<undefined>",
"<undefined>",
"null",  // 122
"<undefined>",
"or",  // 124
"<undefined>",
"<undefined>",
"<undefined>"
};

