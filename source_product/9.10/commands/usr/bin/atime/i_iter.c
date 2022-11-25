/*

    instr_iterate()

    Process an "iterate" pseudo-op.

*/

#include "fizz.h"

void instr_iterate(ptr)
char *ptr;
{
    char *tk_start, *tk_end;
    char *iterate;

    /* must be in the fizz initialization section */
    if (gSection != FIZZ_INIT) {
	error_locate(76);
	return;
    };

    /* get iteration count */
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start == (char *) 0) {
	error_locate(52); /* error if end-of-line */
	return;
    };

    /* store iteration count */
    CREATE_STRING(iterate, tk_end - tk_start + 1);
    (void) strncpy(iterate, tk_start, tk_end - tk_start);
    iterate[tk_end - tk_start] = '\0';
    store_iterations(INFILE, iterate);
    free(iterate);

    /* check for extraneous tokens */
    ptr = tk_end;
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 71 : 52);
    return;
}
