/*

    instr_output()

    Process an "output" pseudo-op.

*/

#include "fizz.h"

void instr_output(ptr)
char *ptr;
{
    char *tk_start, *tk_end;
    char *output;

    /* must be in fizz initialization section */
    if (gSection != FIZZ_INIT) {
	error_locate(105);
	return;
    };

    /* get output file name */
    get_next_token(ptr, &tk_start, &tk_end);
    if ((tk_start == (char *) 0) || (*tk_start == '#')) {
	error_locate(102); /* error if end-of-line or comment */
	return;
    };

    /* store output file name */
    CREATE_STRING(output, tk_end - tk_start + 1);
    (void) strncpy(output, tk_start, tk_end - tk_start);
    output[tk_end - tk_start] = '\0';
    store_output_filename(INFILE, output);
    free(output);

    /* check for extraneous tokens */
    ptr = tk_end;
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 73 : 125);

    return;
}
