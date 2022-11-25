/*

    instr_include()

    Process an "include" pseudo-op.

*/

#include "fizz.h"

void instr_include(ptr, eflag)
char *ptr;
BOOLEAN eflag;	/* report warnings? */
{
    char *tk_start, *tk_end, *t1, *t2;
    char *include;

    /* get include file name */
    get_next_token(ptr, &tk_start, &tk_end);
    if ((tk_start == (char *) 0) || (*tk_start == '#')) {
	if (eflag) error_locate(101); /* error if end-of-line or comment */
	return;
    };

    /* check for extraneous token(s) */
    if (eflag) {
        get_next_token(tk_end, &t1, &t2);
        if (t1 != (char *) 0) error_locate(*t1 == '#' ? 70 : 124);
    };

    /* remove leading and/or trailing quote(s) */
    if (*tk_start == '"') tk_start++;
    if (*(tk_end - 1) == '"') tk_end--;

    /* store include file name */
    CREATE_STRING(include, tk_end - tk_start + 1);
    (void) strncpy(include, tk_start, tk_end - tk_start);
    include[tk_end - tk_start] = '\0';
    open_input_file(include, TRUE);
    free(include);

    return;
}
