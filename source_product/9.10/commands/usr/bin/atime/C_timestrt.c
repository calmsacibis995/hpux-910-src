/*

    Cprint_time_startup_code()

    Create code for timing section startup.

		text
		global	___Ztime
	___Ztime:	space	0	# hang label on "space" in case no
					# text follows

*/

#include "fizz.h"

static char *startup_code[] = {
	"\ttext",
	"\tglobal\t___Ztime",
	"___Ztime:\tspace\t0",
};

void Cprint_time_startup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    for (count = 0; count < sizeof(startup_code)/sizeof(char *); count++)
	print(output, "%s\n", startup_code[count]);
    return;
}
