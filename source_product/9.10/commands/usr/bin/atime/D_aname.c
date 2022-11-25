/*

    Dprint_assertion_names_table()

    Print tables with assertion name information.  These names are used
    when it is necessary to print out assertion information.  If there
    is one dataset, the type of "aname" is "char *aname".  If there is
    more than one dataset, the type of "aname" is "char *aname[]".

    #if there are assertions
		data
		lalign	4
	aname:
      #if there is more than one assertion
		long	aname0
		long	aname1
		long	aname2
		long	aname3
		long	aname4
		long	aname5
		long	aname6
		     .
		     .
		     .
	aname0:
		byte	{name}
		byte	0
	aname1:
		byte	{name}
		byte	0
     #else
		byte	{name}
		byte	0
     #endif
    #endif

*/

#include "fizz.h"

void Dprint_assertion_names_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    if (gAssertionCount) {
	print(output, "\tdata\n");
	print(output, "\tlalign\t4\n");
	print(output, "aname:\n");
	if (gAssertionCount > 1) {
            for (count = 0; count < gAssertionCount; count++)
		print(output, "\tlong\taname%u\n", count);
            for (count = 0; count < gAssertionCount; count++) {
		print(output, "aname%u:\n", count);
		print_byte_info(get_assertion_name(count));
		print(output, "\tbyte\t0\n");
	    };
	} else if (gAssertionCount) {
	    print_byte_info(get_assertion_name(0));
	    print(output, "\tbyte\t0\n");
	};
    };

    return;
}
