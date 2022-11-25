/*

    instr_stack()

    Process a "stack even" or "stack odd" pseudo-op.

*/

#include "fizz.h"

void instr_stack(ptr)
char *ptr;
{
    char *tk_start, *tk_end;

    /* must be in the fizz initialization or initialization section */
    if (gSection == FIZZ_INIT) {
	gSection = INIT;
	if (!gfInitSection) {
	    gfInitSection = TRUE;	/* "stack" is an executable instr */
	    Cprint_init_startup_code(); /* print startup code */
	};
    } else if (gSection != INIT) {
	error_locate(109);
	return;
    };

    /* get "even" or "odd" */
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start == (char *) 0) {
	error_locate(56);	/* error if end-of-line */
	return;
    };

    /* which is it? even or odd? */
    if ((tk_end - tk_start == 4) && !strncmp(tk_start, "even", 4)) {
	Cprint_stackeven_code(); /* print stack even code */
	gfStackeven = TRUE;	 /* will need stack even routine */
    } else if ((tk_end - tk_start == 3) && !strncmp(tk_start, "odd", 3)) {
	Cprint_stackodd_code();	 /* print stack odd code */
	gfStackodd = TRUE;	 /* will need stack odd routine */
    } else {
	error_locate(56);	 /* unrecognized token */
	return;
    };

    /* check for extraneous tokens */
    ptr = tk_end;
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 74 : 56);

    return;
}
