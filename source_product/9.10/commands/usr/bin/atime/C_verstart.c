/*

    Cprint_verify_startup_code()

    Create code for verify section startup.

		text
		global	___Zverify
	___Zverify:	space	0	# hang label on "space" in case no
					# text follows

*/

#include "fizz.h"

static char *startup_code[] = {
	"\ttext",
	"\tglobal\t___Zverify",
	"___Zverify:\tspace\t0",
};

void Cprint_verify_startup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gfVerifySection)
	for (count = 0; count < sizeof(startup_code)/sizeof(char *); count++)
	    print(output, "%s\n", startup_code[count]);
    return;
}
