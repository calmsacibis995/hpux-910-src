/*

    instr_nolist()

    Process a "nolist" pseudo-op.

*/

#include "fizz.h"

void instr_nolist(ptr)
register char *ptr;
{
    char *tk_start, *tk_end;

    /* must be in fizz initialization section */
    if (gSection != FIZZ_INIT) {
	error_locate(98);
	return;
    };

    /* note that the nolist flag is set */
    store_nolist(INFILE);

    /* check for extraneous tokens */
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 72 : 128);

    return;
}
