/*

    Cprint_profile_count_code()

    Create code to increment a profile count. The parameter sent to
    this routine is the number of the current profiled instruction.

		pea	{index}*4	# to use as table index
		jsr	___Zprof
		addq.w	&4,%sp


*/

#include "fizz.h"

void Cprint_profile_count_code(index)
unsigned long index;
{
    fprintf(gOutput, "\tpea\t%u\n", index << 2);
    fprintf(gOutput, "\tjsr\t___Zprof\n");
    fprintf(gOutput, "\taddq.w\t&4,%%sp\n");

    return;
}
