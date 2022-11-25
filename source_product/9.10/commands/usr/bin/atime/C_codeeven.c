/*

    Cprint_codeeven_code()

    Create code to perform "code even".

		bra.b	___Z{label}	# "file", line number
					# code even
		lalign	4
		nop
	___Z{label}: nop

*/

#include "fizz.h"

void Cprint_codeeven_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;
    unsigned long label;

    label = create_label();

    print(output, "\tbra.b\t___Z%u\t# \"%s\", line %u\n", label, gFile, gLine);
    print(output, "\t\t\t# code even\n");
    print(output, "\tlalign\t4\n");
    print(output, "\tnop\n");
    print(output, "___Z%u:\tnop\n", label);

    return;
}
