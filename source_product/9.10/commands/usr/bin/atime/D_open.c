/*

    Dprint_open_code()

    Print code to open the output file.

		data
		lalign	4
	file:	space	4		# global variable for file stream

    #if output is to go to stdout
		text
		mov.l	&___iob+16,file	# file = stdout
    #else (output is to go to a file)
     #if mode is assertion listing
	open1:	byte	"w+",0		# file is overwritten
     #else
	open1:	byte	"a+",0		# append to file
     #endif
	open2:
		byte	....		# output file name
		byte	0		# name terminator

		text
		pea	open1
		pea	open2
		jsr	_fopen		# file = fopen(open2,open1)
		addq.w	&8,%sp
		mov.l	%d0,file
    #endif

*/

#include "fizz.h"

static char *output_file;

static char *open_code1[] = {
	"\tdata",
	"\tlalign\t4",
	"file:\tspace\t4",
};

static char *open_code2[] = {
	"\ttext",
	"\tmov.l\t&__iob+16,file",
};

static char *open_code3[] = {
	"\tbyte\t0",
	"\ttext",
	"\tpea\topen1",
	"\tpea\topen2",
	"\tjsr\t_fopen",
	"\taddq.w\t&8,%sp",
	"\tmov.l\t%d0,file",
};

void Dprint_open_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    for (count = 0; count < sizeof(open_code1)/sizeof(char *); count++)
	print(output, "%s\n", open_code1[count]);
    if (output_file == (char *) 0) {
        for (count = 0; count < sizeof(open_code2)/sizeof(char *); count++)
	    print(output, "%s\n", open_code2[count]);
    } else {
	print(output, "\tdata\nopen1:\tbyte\t\"%c+\",0\nopen2:\n",
	    gMode == ASSERTION_LISTING ? 'w' : 'a');
	print_byte_info(output_file);
        for (count = 0; count < sizeof(open_code3)/sizeof(char *); count++)
	    print(output, "%s\n", open_code3[count]);
    };
    return;
}

/*

    store_output_filename()

    Store the output file name.

*/

void store_output_filename(source, filename)
short source;
char *filename;
{
    static BOOLEAN cmdline = FALSE;
    static BOOLEAN infile = FALSE;
    static BOOLEAN infile_error = FALSE;
    BOOLEAN store;
    unsigned long length;

    store = FALSE;

    if (source == CMDLINE) {
	if (strcmp(filename, "-")) store = TRUE;
	cmdline = TRUE;
    } else {
	if (infile) {
	    if (!infile_error) {
	        error_locate(20);
	        infile_error = TRUE;
	    };
	} else {
	    if (!cmdline && (gMode == PERFORMANCE_ANALYSIS)) store = TRUE;
	    infile = TRUE;
        };
    };

    if (store) {
	length = strlen(filename);
	if (*filename == '"') {
	    filename++;
	    length--;
	};
	if (*(filename + length - 1) == '"') length--;
	CREATE_STRING(output_file, strlen(filename) + 1);
        (void) strncpy(output_file, filename, length);
	output_file[length] = '\0';
    };

    return;
}
