/*

    Cprint_init_startup_code()

    Create code for initialization startup. This is the code the begins
    the initialization section.

		text
		global	___Zinit
	___Zinit: space	0

*/

#include "fizz.h"

static char *startup_code[] = {
	"\ttext",
	"\tglobal\t___Zinit",
	"___Zinit:\tspace\t0",
};

void Cprint_init_startup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gfInitSection)
	for (count = 0; count < sizeof(startup_code)/sizeof(char *); count++)
	    print(output, "%s\n", startup_code[count]);
    return;
}
