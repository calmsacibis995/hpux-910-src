/*

    Dprint_stackodd_routine()

    Print a routine to do a "stack odd".

    #if there is a "stack odd" instruction
		text
		global	___ZSO
	___ZSO:	mov.w	%cc,-(%sp)		# save %cc
		mov.w	%d0,-(%sp)		# save %d0
		mov.w	%sp,%d0			# check %sp alignment
		addq.w	&2,%d0
		and.w	&3,%d0
		beq.b	odd			# odd alignment?
		sub.w	%d0,%sp			# no: adjust %sp
		mov.l	0(%sp,%d0.w),(%sp)	# move %d0 and %cc
		mov.l	4(%sp,%d0.w),4(%sp)	# move return address
	odd:	mov.w	(%sp)+,%d0		# restore %d0
		rtr				# restore %cc and return
    #endif

*/

#include "fizz.h"

static char *stackodd_routine[] = {
	"\ttext",
	"\tglobal\t___ZSO",
	"___ZSO:\tmov.w\t%cc,-(%sp)",
	"\tmov.w\t%d0,-(%sp)",
	"\tmov.w\t%sp,%d0",
	"\taddq.w\t&2,%d0",
	"\tand.w\t&3,%d0",
	"\tbeq.b\todd",
	"\tsub.w\t%d0,%sp",
	"\tmov.l\t0(%sp,%d0.w),(%sp)",
	"\tmov.l\t4(%sp,%d0.w),4(%sp)",
	"odd:\tmov.w\t(%sp)+,%d0",
	"\trtr",
};

void Dprint_stackodd_routine()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gfStackodd)
        for (count = 0; count < sizeof(stackodd_routine)/sizeof(char *); 
	  count++) print(output, "%s\n", stackodd_routine[count]);
    return;
}
