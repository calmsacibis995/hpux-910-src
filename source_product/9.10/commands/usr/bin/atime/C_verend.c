/*

    Cprint_verify_cleanup_code()

    Create code for verify section cleanup.

	text
	subq.w	&6,%sp		# for condition codes and return address
	mov.w	%cc,(%sp)	# store condition codes
	mov.l	___ZSP,2(%sp)	# store return address
	rtr

*/

#include "fizz.h"

static char *cleanup_code[] = {
	"\ttext",
	"\tsubq.w\t&6,%sp",
	"\tmov.w\t%cc,(%sp)",
	"\tmov.l\t___ZSP,2(%sp)",
	"\trtr",
};

void Cprint_verify_cleanup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gfVerifySection)
	for (count = 0; count < sizeof(cleanup_code)/sizeof(char *); count++)
	    print(output, "%s\n", cleanup_code[count]);
    return;
}
