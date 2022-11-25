/*

    Dprint_parameter_routine()

    Create a parameter execution routine which will be called for 
    every instruction containing a parameter (dataname) in the source 
    code.

    There is one parameter to the ___Zparam routine that is created:  a
    long word offset into the "dsetpar" table for the current data set
    (the offset is into the "param" table if there is zero or one
    dataset).  This routine figures out which parameter code to execute
    based upon the instruction that called ___Zparam and the dataset
    that is currently being considered.  Note that no registers are
    destroyed nor are the condition codes affected.

    #if there are dynamic parameters
		text
		global	___Zparam
	___Zparam:
		subq.w	&6,%sp		   # space for %cc and return address
		mov.w	%cc,(%sp)	   # save %cc
		pea	(%a0)		   # save %a0
     #if there is more than one data set
		mov.l	cdsetpar,%a0	   # beginning of table
     #else
		lea	param,%a0
     #endif
		add.l	14(%sp),%a0	   # add table offset
		mov.l	(%a0),6(%sp)	   # insert into return address area
		mov.l	(%sp)+,%a0	   # restore %a0
		rtr			   # jump to parameter code
    #endif

*/

#include "fizz.h"

static char *parameter_routine1[] = {
	"\ttext",
	"\tglobal\t___Zparam",
	"___Zparam:",
	"\tsubq.w\t&6,%sp",
	"\tmov.w\t%cc,(%sp)",
	"\tpea\t(%a0)",
};

static char *parameter_routine2[] = {
	"\tadd.l\t14(%sp),%a0",
	"\tmov.l\t(%a0),6(%sp)",
	"\tmov.l\t(%sp)+,%a0",
	"\trtr",
};

void Dprint_parameter_routine()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (!gParameterCount) return;
    for (count = 0; count < sizeof(parameter_routine1)/sizeof(char *); count++)
        print(output, "%s\n", parameter_routine1[count]);
    if (gDatasetCount > 1) print(output, "\tmov.l\tcdsetpar,%%a0\n");
    else print(output, "\tlea\tparam,%%a0\n");
    for (count = 0; count < sizeof(parameter_routine2)/sizeof(char *); count++)
        print(output, "%s\n", parameter_routine2[count]);

    return;
}
