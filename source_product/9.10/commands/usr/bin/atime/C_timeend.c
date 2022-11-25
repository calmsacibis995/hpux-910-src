/*

    Cprint_time_cleanup_code()

    Create code for timing section cleanup.

		global	___Zovhd
	___Zovhd:
		subq.w	&6,%sp		# for condition codes & return address
		mov.w	%cc,(%sp)	# save condition codes
		mov.l	___ZSP,2(%sp)	# get return address
		rtr

*/

#include "fizz.h"

static char *cleanup_code[] = {
	"\tglobal\t___Zovhd",
	"___Zovhd:",
	"\tsubq.w\t&6,%sp",
	"\tmov.w\t%cc,(%sp)",
	"\tmov.l\t___ZSP,2(%sp)",
	"\trtr",
};

void Cprint_time_cleanup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    for (count = 0; count < sizeof(cleanup_code)/sizeof(char *); count++)
        print(output, "%s\n", cleanup_code[count]);
    return;
}
