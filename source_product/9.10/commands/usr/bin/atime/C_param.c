/*

    Cprint_parameter_code()

    Create code to access a dataname parameter.  For each instruction in
    the initialization which uses a dataname parameter, this code will
    be emitted.  The {index} which is pushed as a parameter in this
    created code is four times the number of such instructions
    encountered.  This provides a table offset.

		pea	{index}		# "file", line number
					# abbreviated instruction
		jsr	___Zparam

    Note that the {index} parameter is deleted by ___Zparam so
    instructions using parameters can access the stack.

*/

#include "fizz.h"

void Cprint_parameter_code(index)
unsigned long index;
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    char *buffer;

    print(output, "\tpea\t%u\t# \"%s\", line %u\n", index << 2, gFile, gLine);
    buffer = compact_instruction();
    print(output, "\t\t\t# %s\n", buffer);
    free(buffer);
    print(output, "\tjsr\t___Zparam\n");

    return;
}
