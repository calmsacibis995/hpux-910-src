/*

    Dprint_startup_code()

    Print code to do necessary driver initialization.

		text
		global	_main
	_main:	link	%a6,&-24

	 -4(%a6) = current entry in dsetpar table
	 -8(%a6) = current entry in dsetver table
	-12(%a6) = dataset counter
	-16(%a6) = current count counter
	-20(%a6) = iteration counter
	-24(%a6) = loop counter

*/

#include "fizz.h"

static char *startup_code[] = {
	"\ttext",
	"\tglobal\t_main",
	"_main:\tlink\t%a6,&-24",
};

void Dprint_startup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    for (count = 0; count < sizeof(startup_code)/sizeof(char *); count++)
	print(output, "%s\n", startup_code[count]);
    return;
}
