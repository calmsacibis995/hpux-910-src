/*

    Cprint_stackeven_code()

    Create code to perform "stack even".

		jsr	___ZSE		# "file", line number
					# stack even

*/

#include "fizz.h"

void Cprint_stackeven_code()
{
    fprintf(gOutput, "\tjsr\t___ZSE\t# \"%s\", line %u\n", gFile, gLine);
    fprintf(gOutput, "\t\t\t# stack even\n");

    return;
}
