/*

    Cprint_init_cleanup_code()

    Create code for initialization cleanup.

    #if there is an initialization section
		subq.w	&6,%sp		# space for %cc and return address
		mov.w	%cc,(%sp)	# save %cc
		mov.l	___ZSP,2(%sp)	# store return address
		rtr			# return
		nop			# preserve code alignment
    #endif

*/

#include "fizz.h"

static char *cleanup_code[] = {
	"\tsubq.w\t&6,%sp",
	"\tmov.w\t%cc,(%sp)",
	"\tmov.l\t___ZSP,2(%sp)",
	"\trtr",
	"\tnop",
};

void Cprint_init_cleanup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gfInitSection)
	for (count = 0; count < sizeof(cleanup_code)/sizeof(char *); count++)
	    print(output, "%s\n", cleanup_code[count]);
    return;
}
