/*

    Dprint_cleanup_code()

    Print code to do any necessary cleanup before exiting.

		data
		lalign	4
	exitval:
		space	4		# for storing exit value
	execfile:
		byte	....		# name of the target executing file
		byte	0

		text
		clr.w	exitval		# for exit(0)
	cleanup:
		pea	execfile
		jsr	_unlink		# unlink(execfile)
					# the file unlinks itself before
					# exiting so that it will disappear
		mov.l	exitval,(%sp)
		jsr	_exit		# exit(exitval)

*/

#include "fizz.h"

static char *cleanup_code1[] = {
	"\tdata",
	"\tlalign\t4",
	"exitval:",
	"\tspace\t4",
	"execfile:",
};

static char *cleanup_code2[] = {
	"\tbyte\t0",
	"\ttext",
	"\tclr.l\texitval",
	"cleanup:",
	"\tpea\texecfile",
	"\tjsr\t_unlink",
	"\tclr.l\t(%sp)",
	"\tjsr\t_exit",
};

void Dprint_cleanup_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;

    /* Create the name of the execute file */
    CREATE_STRING(gExecFile, L_tmpnam);
    (void) tmpnam(gExecFile);

    for (count = 0; count < sizeof(cleanup_code1)/sizeof(char *); count++)
	print(output, "%s\n", cleanup_code1[count]);
    print_byte_info(gExecFile);
    for (count = 0; count < sizeof(cleanup_code2)/sizeof(char *); count++)
	print(output, "%s\n", cleanup_code2[count]);
    return;
}
