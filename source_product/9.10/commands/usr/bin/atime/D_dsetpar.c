/*

    Dprint_dsetpar_table()

    Print tables with parameter information.  These tables are accessed
    whenever there is an instruction which uses one or more datanames.
    In general, what happens is that "dsetpar" contains one entry per
    dataset and each of these points to another table of pointers.  The
    latter table has an entry for each instruction which contains a
    dataname references.  These pointers point to code sequences to
    execute the appropriate code for the given instruction.  The
    variable "cdsetpar" contains an entry from the "dsetpar" table which
    corresponds to the dataset currently under consideration.  This
    reduces the time needed for table access.

			---*---

    Note: {datasets} = the number of datasets

    #if there are dynamic parameters

		data
		lalign	4

     #if there is more than one data set
	cdsetpar:
		space	4
	dsetpar:
		long	param0
		long	param1
		long	param2
		long	param3
		long	param4
		long	param5
		long	param6
		      .
		      .
		      .
	param0:
		long	___Zcode{0+0*{datasets}}
		long	___Zcode{0+1*{datasets}}
		long	___Zcode{0+2*{datasets}}
		long	___Zcode{0+3*{datasets}}
		long	___Zcode{0+4*{datasets}}
		long	___Zcode{0+5*{datasets}}
		      .
		      .
		      .
	param1:
		long	___Zcode{1+0*{datasets}}
		long	___Zcode{1+1*{datasets}}
		long	___Zcode{1+2*{datasets}}
		long	___Zcode{1+3*{datasets}}
		long	___Zcode{1+4*{datasets}}
		long	___Zcode{1+5*{datasets}}
		      .
		      .
		      .

		      .
		      .
		      .
     #else
	param:
		long	___Zcode0
		long	___Zcode1
		long	___Zcode2
		long	___Zcode3
		long	___Zcode4
		      .
		      .
		      .
     #endif

    #endif

*/

#include "fizz.h"

void Dprint_dsetpar_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register unsigned long i, j, label;

    if (gParameterCount) {
	print(output, "\tdata\n");
	print(output, "\tlalign\t4\n");
	if (gDatasetCount > 1) {
	    print(output, "cdsetpar:\tspace\t4\n");
	    print(output, "dsetpar:\n");
	    for (i = 0; i < gDatasetCount; i++)
		print(output, "\tlong\tparam%u\n", i);
	    for (i = 0; i < gDatasetCount; i++) {
		print(output, "param%u:\n", i);
		label = i;
		for (j = 0; j < gParameterCount; j++, label += gDatasetCount)
		    print(output, "\tlong\t___Zcode%u\n", label);
	    };
	} else {
	    print(output, "param:\n");
	    for (i = 0; i < gParameterCount; i++)
		print(output, "\tlong\t___Zcode%u\n", i);
	};
    };

    return;
}
