/*

    Dprint_line_code()

    Print code to print a line of 75 dashes.

		data
	line:	byte	\"--------------------------------------------------\"
		byte	\"-------------------------\",10,10  # extra linefeed

		text
		mov.l	file,-(%sp)
		pea	77.w
		pea	1.w
		pea	line
		jsr	_fwrite		# fwrite(line,1,77,file);
		lea	16(%sp),%sp

*/

#include "fizz.h"

static char *line_code[] = {
	"\tdata",
	"line:\tbyte\t\"--------------------------------------------------\"",
	"\tbyte\t\"-------------------------\",10,10",
	"\ttext",
	"\tmov.l\tfile,-(%sp)",
	"\tpea\t77.w",
	"\tpea\t1.w",
	"\tpea\tline",
	"\tjsr\t_fwrite",
	"\tlea\t16(%sp),%sp",
};

void Dprint_line_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    for (count = 0; count < sizeof(line_code)/sizeof(char *); count++)
	print(output, "%s\n", line_code[count]);
    return;
}
