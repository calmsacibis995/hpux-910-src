/*

    Dprint_profile_count_routine()

    Create code for a routine to increment a profile count.

		text
		global	___Zprof
	___Zprof:
		mov.w	%cc,-(%sp)	# save condition codes
		pea	(%a0)		# save %a0
		lea	pcount,%a0	# profile count table
		add.l	10(%sp),%a0	# get index into table
		addq.l	&1,(%a0)	# increment profile count
		bcc.b	Zp1		# profile count overflow?
		subq.l	&1,(%a0)	# yes: never exceed 0xFFFFFFFF
	Zp1:	mov.l	(%sp)+,%a0	# restore %a0
		rtr

*/

#include "fizz.h"

static char *prof_routine[] = {
	"\ttext",
	"\tglobal\t___Zprof",
	"___Zprof:",
	"\tmov.w\t%cc,-(%sp)",
	"\tpea\t(%a0)",
	"\tlea\tpcount,%a0",
	"\tadd.l\t10(%sp),%a0",
	"\taddq.l\t&1,(%a0)",
	"\tbcc.b\tZp1",
	"\tsubq.l\t&1,(%a0)",
	"Zp1:\tmov.l\t(%sp)+,%a0",
	"\trtr",
};

void Dprint_profile_count_routine()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (!get_instruction_count()) return;
    for (count = 0; count < sizeof(prof_routine)/sizeof(char *); count++)
	print(output, "%s\n", prof_routine[count]);

    return;
}
