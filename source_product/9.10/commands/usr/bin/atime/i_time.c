/*

    instr_time()

    Process a "time" pseudo-op.

*/

#include "fizz.h"

void instr_time(ptr)
char *ptr;
{
    char *tk_start, *tk_end;

    /* must be fizz initialization or initialization section */
    if (gSection == TIME) {
	error_locate(96);
	return;
    };

    if (gSection == VERIFY) {
	error_locate(110);
	return;
    };

    gSection = TIME;
    gfTimeSection = TRUE;

    /* print init section cleanup code */
    if (gfInitSection) Cprint_init_cleanup_code();

    /* print time section startup code */
    Cprint_time_startup_code();

    /* check for extraneous tokens */
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 57 : 126);

    return;
}
