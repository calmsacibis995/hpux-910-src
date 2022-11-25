/*

    Cprint_stackodd_code()

    Create code to perform "stack odd".

		jsr	___ZSO		# "file", line number
					# stack odd

*/

#include "fizz.h"

void Cprint_stackodd_code()
{
    fprintf(gOutput, "\tjsr\t___ZSO\t# \"%s\", line %d\n", gFile, gLine);
    fprintf(gOutput, "\t\t\t# stack odd\n");

    return;
}
