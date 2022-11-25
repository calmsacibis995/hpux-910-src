/*

    instr_code()

    Process a "code even" or "code odd" pseudo-op.

*/

#include "fizz.h"

void instr_code(ptr)
char *ptr;
{
    char *tk_start, *tk_end;

    /* must be in fizz initialization or initialization section */
    if (gSection == FIZZ_INIT) {
	gSection = INIT;  /* this is executable: switch to init section */
	if (!gfInitSection) {
	    gfInitSection = TRUE; /* print init start code */
	    Cprint_init_startup_code();
	};
    } else if (gSection != INIT) {
	error_locate(65);
	return;
    };

    /* get "even" or "odd" */
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start == (char *) 0) {
	error_locate(50);	/* end-of-line */
	return;
    };

    /* which is it? even or odd? */
    if ((tk_end - tk_start == 4) && !strncmp(tk_start, "even", 4))
	Cprint_codeeven_code();	/* create code even sequence */
    else if ((tk_end - tk_start == 3) && !strncmp(tk_start, "odd", 3))
	Cprint_codeodd_code();	/* create code odd sequence */
    else {
	error_locate(50);	/* bad argument */
	return;
    };

    /* check for tokens following code instruction */
    ptr = tk_end;
    get_next_token(ptr, &tk_start, &tk_end);
    if (tk_start != (char *) 0) error_locate(*tk_start == '#' ? 69 : 50);

    return;
}
