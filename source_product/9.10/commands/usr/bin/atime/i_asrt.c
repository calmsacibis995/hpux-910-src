/*

    instr_assert()

    Process an "assert" pseudo-op.

*/

#include "fizz.h"

void instr_assert(ptr)
char *ptr;
{
    char *tk_start, *tk_end;
    char *assert;

    /* must be in fizz initialization section */
    if (gSection != FIZZ_INIT) {
	error_locate(46);
	return;
    };

    /* get next token after "assert" */
    get_next_token(ptr, &tk_start, &tk_end);
    if ((tk_start == (char *) 0) || (*tk_start == '#')) {
	error_locate(100);	/* error if end-of-line or comment */
	return;
    };

    /* store the assert file name */
    CREATE_STRING(assert, tk_end - tk_start + 1);
    (void) strncpy(assert, tk_start, tk_end - tk_start);
    assert[tk_end - tk_start] = '\0';
    store_assert_filename(INFILE, assert);
    free(assert);

    /* check for tokens following "assert" instruction */
    ptr = tk_end;
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 68 : 48);

    return;
}
