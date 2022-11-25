/*

    Dprint_dataset_names_table()

    Print tables with dataset name information.  These names are used
    when it is necessary to print out assertion error information.  When
    there is one dataset, the type of "dname" is "char *dname".  If
    there is more than one dataset, the type of "dname" is "char *dname[]".

    #if there are datasets
     #if there are assertions
      #if mode is not assertion listing

		data
		lalign	4
	dname:

       #if there is more than one data set
		long	dname0
		long	dname1
		long	dname2
		long	dname3
		long	dname4
		long	dname5
		long	dname6
		long	dname7
		     .
		     .
		     .
	dname0:
		byte	{name}
		byte	0
	dname1:
		byte	{name}
		byte	0
		     .
		     .
		     .
       #else
		byte	{name}
		byte	0
       #endif

      #endif
     #endif
    #endif

*/

#include "fizz.h"

void Dprint_dataset_names_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gDatasetCount && gAssertionCount && (gMode != ASSERTION_LISTING)) {
	print(output, "\tdata\n");
	print(output, "\tlalign\t4\n");
	print(output, "dname:\n");
	if (gDatasetCount > 1) {
            for (count = 0; count < gDatasetCount; count++)
		print(output, "\tlong\tdname%u\n", count);
            for (count = 0; count < gDatasetCount; count++) {
		print(output, "dname%u:\n", count);
		print_byte_info(get_dataset_name(count));
		print(output, "\tbyte\t0\n");
	    };
	} else if (gDatasetCount) {
	    print_byte_info(get_dataset_name(0));
	    print(output, "\tbyte\t0\n");
	};
    };

    return;
}
