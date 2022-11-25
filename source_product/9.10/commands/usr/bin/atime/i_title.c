/*

    instr_title()

    Process a "title" pseudo-op.

*/

#include "fizz.h"

void instr_title(ptr)
register char *ptr;
{
    char *title;
    char *tk_start, *tk_end;

    /* must be fizz initialization section */
    if (gSection != FIZZ_INIT) {
	error_locate(111);
	return;
    };

    /* look for title string */
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start == (char *) 0) {
	error_locate(58);	/* no title string */
	return;
    };

    /* store title */
    CREATE_STRING(title, strlen(tk_start) + 1);
    (void) strcpy(title, tk_start);
    store_title(INFILE, title);
    free(title);

    return;
}
