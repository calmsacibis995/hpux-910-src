/*

    Cprint_codeodd_code()

    Create code to perform "code odd".
	
		bra.w	___Z{label}	# "file", line number
					# code odd
		lalign	4
	___Z{label}: nop

*/

#include "fizz.h"

void Cprint_codeodd_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;
    unsigned long label;

    label = create_label();

    print(output, "\tbra.w\t___Z%u\t# \"%s\", line %u\n", label, gFile, gLine);
    print(output, "\t\t\t# code odd\n");
    print(output, "\tlalign\t4\n");
    print(output, "___Z%u:\tnop\n", label);

    return;
}
