/*

    Dprint_stackeven_routine()

    Create a routine to do a "stack even".

    #if there is a "stack even" instruction
		text
		global	___ZSE
	___ZSE:	mov.w	%cc,-(%sp)		# save %cc
		mov.w	%d0,-(%sp)		# save %d0
		mov.w	%sp,%d0			# check stack alignment
		and.w	&3,%d0
		beq.b	even			# no work if even alignment
		sub.w	%d0,%sp			# align %sp
		mov.l	0(%sp,%d0.w),(%sp)	# move %d0 and %cc
		mov.l	4(%sp,%d0.w),4(%sp)	# move return address
	even:	mov.w	(%sp)+,%d0		# restore %d0
		rtr				# restore %cc and return
    #endif

*/

#include "fizz.h"

static char *stackeven_routine[] = {
	"\ttext",
	"\tglobal\t___ZSE",
	"___ZSE:\tmov.w\t%cc,-(%sp)",
	"\tmov.w\t%d0,-(%sp)",
	"\tmov.w\t%sp,%d0",
	"\tand.w\t&3,%d0",
	"\tbeq.b\teven",
	"\tsub.w\t%d0,%sp",
	"\tmov.l\t0(%sp,%d0.w),(%sp)",
	"\tmov.l\t4(%sp,%d0.w),4(%sp)",
	"even:\tmov.w\t(%sp)+,%d0",
	"\trtr",
};

void Dprint_stackeven_routine()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gfStackeven)
        for (count = 0; count < sizeof(stackeven_routine)/sizeof(char *); 
	  count++) print(output, "%s\n", stackeven_routine[count]);
    return;
}
