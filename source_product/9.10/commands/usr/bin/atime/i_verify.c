/*

    instr_verify()

    Process a "verify" pseudo-op.

*/

#include "fizz.h"

void instr_verify(ptr)
char *ptr;
{
    char *tk_start, *tk_end;

    /* check for multiple verify instructions */
    if (gSection == VERIFY) {
	error_locate(97);
	return;
    };

    gSection = VERIFY;
    gfVerifySection = TRUE;

    /* print time cleanup code */
    Cprint_time_cleanup_code();

    /* print verify startup code */
    Cprint_verify_startup_code();

    /* check for extraneous tokens */
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 59 : 127);

    return;
}
