/*

    error()

    Print an error or warning message.


*/

#include "fizz.h"

#define WARN 0
#define ERROR 1
#define ABORT 2

static struct {
    short type;
    char *message;
} error_message[] = {
/* 000 */ {ABORT, "cannot open file: \"%s\"\n"},
/* 001 */ {ABORT, "%s: include directive in include file\n"},
/* 002 */ {ABORT, "no available memory\n"},
/* 003 */ {ABORT, "both -p and -l option given in command line\n"},
/* 004 */ {ERROR, "illegal character in iteration count: %s\n"},
/* 005 */ {ERROR, "%s: illegal dataname\n"},
/* 006 */ {ERROR, "%s: illegal dataset name\n"},
/* 007 */ {ERROR, "illegal dataset name in command line\n"},
/* 008 */ {ERROR, "%s: illegal character in dataset count\n"},
/* 009 */ {ERROR, "sum of all counts in dataset directives overflows\n"},
/* 010 */ {ERROR, "%s: dataset count overflow\n"},
/* 011 */ {ERROR, "%s: dataname directive not in initialization section\n"},
/* 012 */ {ERROR, "%s: dataset directive not in initialization section\n"},
/* 013 */ {ERROR, "%s: no dataname directive preceding dataset directive\n"},
/* 014 */ {ERROR, "incompatible dataset count(s) and iteration count\n"},
/* 015 */ {ERROR, "iteration count overflow: %s\n"},
/* 016 */ {ERROR, "%s: count missing from dataset directive\n"},
/* 017 */ {ERROR, "%s: dataset name missing from dataset directive\n"},
/* 018 */ {ERROR, "%s: missing ')' in dataset directive\n"},
/* 019 */ {ERROR, "%s: multiple dataname directives\n"},
/* 020 */ {ERROR, "%s: multiple output directives\n"},
/* 021 */ {ERROR, "%s: missing arguments in dataname directive\n"},
/* 022 */ {WARN,  "%s: illegal number\n"},
/* 023 */ {WARN,  "%s: illegal assertion name\n"},
/* 024 */ {WARN,  "%s: illegal dataset name\n"},
/* 025 */ {WARN,  "%s: illegal suffix\n"},
/* 026 */ {WARN,  "cannot close file \"%s\"\n"},
/* 027 */ {WARN,  "%s: comment directive not in initialization section\n"},
/* 028 */ {WARN,  "%s: extra ',' in dataname directive list\n"},
/* 029 */ {WARN,  "%s: extra ',' in dataset directive list\n"},
/* 030 */ {WARN,  "%s: no data for assertion\n"},
/* 031 */ {WARN,  "%s: missing ',' in dataname directive\n"},
/* 032 */ {WARN,  "%s: missing ',' in dataset directive\n"},
/* 033 */ {WARN,  "%s: missing dataset name\n"},	
/* 034 */ {WARN,  "%s: missing suffix\n"},
/* 035 */ {WARN,  "multiple -a options in command line\n"},
/* 036 */ {WARN,  "%s: multiple assert directives\n"},
/* 037 */ {WARN,  "multiple -i options in command line\n"},
/* 038 */ {WARN,  "%s: multiple iterate directives\n"},
/* 039 */ {WARN,  "multiple -n options in command line\n"},
/* 040 */ {WARN,  "%s: multiple nolist directives\n"},
/* 041 */ {WARN,  "multiple -t options in command line\n"},
/* 042 */ {WARN,  "%s: multiple title directives\n"},
/* 043 */ {WARN,  "%s: number overflow\n"},
/* 044 */ {ERROR, "undefined dataset: %s\n"},
/* 045 */ {ERROR, "%s: assert directive not in verify section\n"},
/* 046 */ {ERROR, "%s: assert directive not in initialization section\n"},
/* 047 */ {ERROR, "%s: illegal assertion name\n"},
/* 048 */ {ERROR, "%s: syntax error in assert instruction\n"},
/* 049 */ {WARN,  "unrecognized command line option\n"},
/* 050 */ {ERROR, "%s: syntax error in code instruction\n"},
/* 051 */ {ABORT, "illegal source file name\n"},
/* 052 */ {ERROR, "%s: syntax error in iterate statement\n"},
/* 053 */ {ERROR, "%s: syntax error in ldopt statement\n"},
/* 054 */ {ABORT, "illegal output file name\n"},
/* 055 */ {ERROR, "%s: syntax error in use of dataname\n"},
/* 056 */ {ERROR, "%s: syntax error in stack statement\n"},
/* 057 */ {WARN,  "%s: comments not allowed with time directive\n"},
/* 058 */ {WARN,  "%s: title missing from title directive\n"},
/* 059 */ {WARN,  "%s: comments not allowed with verify directive\n"},
/* 060 */ {ABORT, "cannot assemble file: \"%s\"\n"},
/* 061 */ {WARN,  "cannot close file \"%s\"\n"},
/* 062 */ {ABORT, "cannot link file: \"%s\"\n"},
/* 063 */ {ABORT, "cannot open output file: \"%s\"\n"},
/* 064 */ {WARN,  "cannot remove file: \"%s\"\n"},
/* 065 */ {ERROR, "%s: code directive not in initialization section\n"},
/* 066 */ {ABORT, "execution failure\n"},
/* 067 */ {WARN,  "extra options in command line\n"},
/* 068 */ {WARN,  "%s: comments not allowed with assert directive\n"},
/* 069 */ {WARN,  "%s: comments not allowed with code directive\n"},
/* 070 */ {WARN,  "%s: comments not allowed with include directive\n"},
/* 071 */ {WARN,  "%s: comments not allowed with iterate directive\n"},
/* 072 */ {WARN,  "%s: comments not allowed with nolist directive\n"},
/* 073 */ {WARN,  "%s: comments not allowed with output directive\n"},
/* 074 */ {WARN,  "%s: comments not allowed with stack directive\n"},
/* 075 */ {ERROR, "iteration count = 0\n"},
/* 076 */ {ERROR, "%s: iterate directive not in initialization section\n"},
/* 077 */ {WARN,  "%s: assert directive cannot be labelled\n"},
/* 078 */ {WARN,  "%s: assert.b cannot be labelled\n"},
/* 079 */ {WARN,  "%s: assert.l cannot be labelled\n"},
/* 080 */ {WARN,  "%s: assert.w cannot be labelled\n"},
/* 081 */ {WARN,  "%s: code directive cannot be labelled\n"},
/* 082 */ {WARN,  "%s: comment directive cannot be labelled\n"},
/* 083 */ {WARN,  "%s: dataname directive cannot be labelled\n"},
/* 084 */ {WARN,  "%s: dataset directive cannot be labelled\n"},
/* 085 */ {WARN,  "%s: include directive cannot be labelled\n"},
/* 086 */ {WARN,  "%s: iterate directive cannot be labelled\n"},
/* 087 */ {WARN,  "%s: ldopt directive cannot be labelled\n"},
/* 088 */ {WARN,  "%s: nolist directive cannot be labelled\n"},
/* 089 */ {WARN,  "%s: output directive cannot be labelled\n"},
/* 090 */ {WARN,  "%s: stack directive cannot be labelled\n"},
/* 091 */ {WARN,  "%s: time directive cannot be labelled\n"},
/* 092 */ {WARN,  "%s: title directive cannot be labelled\n"},
/* 093 */ {WARN,  "%s: verify directive cannot be labelled\n"},
/* 094 */ {ERROR, "%s: ldopt directive not in initialization section\n"},
/* 095 */ {ABORT, "no input file specified\n"},
/* 096 */ {ERROR, "%s: multiple time directives\n"},
/* 097 */ {ERROR, "%s: multiple verify directives\n"},
/* 098 */ {WARN,  "%s: nolist directive not in initialization section\n"},
/* 099 */ {WARN,  "no file name given with -a option\n"},
/* 100 */ {ERROR, "%s: no file name in assert directive\n"},
/* 101 */ {ERROR, "%s: no file name in include directive\n"},
/* 102 */ {ERROR, "%s: no file name in output directive\n"},
/* 103 */ {WARN,  "no count given with -i option\n"},
/* 104 */ {ERROR, "no time section\n"},
/* 105 */ {ERROR, "%s: output directive not in initialization section\n"},
/* 106 */ {ERROR, "%s: dataname referenced in time section\n"},
/* 107 */ {ERROR, "%s: dataname referenced in verify section\n"},
/* 108 */ {ERROR, "%s: dataname cannot be used in instruction\n"},
/* 109 */ {ERROR, "%s: stack directive not in initialization section\n"},
/* 110 */ {ERROR, "%s: time directive in verify section\n"},
/* 111 */ {WARN,  "%s: title directive not in initialization section\n"},
/* 112 */ {ERROR, "%s: dataname is not defined\n"},
/* 113 */ {ERROR, "%s: iteration count = 0\n"},
/* 114 */ {ERROR, "%s: incorrect number of data in dataset directive\n"},
/* 115 */ {ERROR, "%s: dataset count = 0\n"},
/* 116 */ {WARN,  "%s: comments not allowed with dataname directive\n"},
/* 117 */ {WARN,  "%s: comments not allowed with dataset directive\n"},
/* 118 */ {ERROR, "dataname directive with no dataset directive(s)\n"},
/* 119 */ {WARN,  "%s: comments not allowed with ldopt directive\n"},
/* 120 */ {ERROR, "%s: multiple ldopt directives\n"},
/* 121 */ {WARN,  "%s: duplicate dataset name\n"},
/* 122 */ {ERROR, "%s: duplicate dataname\n"},
/* 123 */ {WARN,  "%s: duplicate assertion name\n"},
/* 124 */ {ERROR, "%s: syntax error in include directive\n"},
/* 125 */ {ERROR, "%s: syntax error in output directive\n"},
/* 126 */ {ERROR, "%s: syntax error in time directive\n"},
/* 127 */ {ERROR, "%s: syntax error in verify directive\n"},
/* 128 */ {ERROR, "%s: syntax error in nolist directive\n"},
};

void error(number, message)
unsigned long number;
char *message;
{
    short type;

    if (!gfMessages) return;	/* no messages during cleanup */

    type = error_message[number].type;

    /* print message based on type */
    if (type == WARN) fprintf(stderr, "WARNING: ");
    else fprintf(stderr, "ERROR: ");
    fprintf(stderr, error_message[number].message, message);

    /* determine further action */
    if (type == ERROR) gfAbort = TRUE;
    else if (type == ABORT) abort();

    return;
}
