/*

    Cprint_variables_code()

    Print code to create a place to store the stack pointer for returning
    from user-supplied assembly code. In addition, there is space for
    storing an effective address value and conditions codes during parameter
    evaluation.

		data
		lalign	4
		global	___ZSP
	___ZSP:	space	4		# save stack pointer
	___ZEA:	space	4		# save effective address value
	___ZCC:	space	2		# save condition codes

*/

#include "fizz.h"

static char *stack_pointer_code[] = {
	"\tdata",
	"\tlalign\t4",
	"\tglobal\t___ZSP",
	"___ZSP:\tspace\t4",
	"___ZEA:\tspace\t4",
	"___ZCC:\tspace\t2",
};

void Cprint_variables_code()
{
    register int count;
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;

    for (count = 0; count < sizeof(stack_pointer_code)/sizeof(char *); count++)
	print(output, "%s\n", stack_pointer_code[count]);
    return;
}
