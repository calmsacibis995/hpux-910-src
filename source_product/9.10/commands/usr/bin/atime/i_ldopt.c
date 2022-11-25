/*

    instr_ldopt()

    Process an "ldopt" pseudo-op.

*/

#include "fizz.h"

static char *ldopt = (char *) 0;

void instr_ldopt(ptr)
char *ptr;
{
    char *line_start, *tk_start, *tk_end;
    static BOOLEAN error_flag = FALSE;

    /* must be in fizz initialization section */
    if (gSection != FIZZ_INIT) {
	error_locate(94);
	return;
    };

    /* check for multiple ldopt instructions */
    if (ldopt != (char *) 0) {
	if (!error_flag) {
	    error_locate(120);
	    error_flag = TRUE;
	};
	return;
    };

    /* get first ldopt argument */
    get_next_token(ptr, &line_start, &tk_end);
    if ((line_start == (char *) 0) || (*line_start == '#')) {
	error_locate(53); /* error if end-of-line or comment */
	return;
    };

    /* check for comments (not allowed) */
    ptr = tk_end;
    for (;;) {
	get_next_token(ptr, &tk_start, &tk_end);
        if (tk_start == (char *) 0) break;
	if (*tk_start == '#') {
	    error_locate(119);
	    break;
	};
	ptr = tk_end;
    };

    /* store ldopt argument string */
    CREATE_STRING(ldopt, ptr - line_start + 1);
    (void) strncpy(ldopt, line_start, ptr - line_start);
    ldopt[ptr - line_start] = '\0';

    return;
}

/*

    link_executable()

    Link driver and code object files into an executable. The command to do
    this is of the form:

    /bin/ld /lib/crt0.o -x -N {driver}.o {code}.o {ldopts} -lc -o {execfile}

*/

void link_executable()
{
    char *command;
    unsigned long length;
    int status;

    /* create link instruction */
    length = strlen(gExecFile) + 50;
    if (ldopt != (char *) 0) length += strlen(ldopt);
    CREATE_STRING(command, length);
    if (ldopt != (char *) 0)
	(void) sprintf(command,"/bin/ld /lib/crt0.o -x -N %s %s %s -lc -o %s\n",
	  gDriverObjFile, gCodeObjFile, ldopt, gExecFile);
    else (void) sprintf(command, "/bin/ld /lib/crt0.o -x -N %s %s -lc -o %s\n", 
      gDriverObjFile, gCodeObjFile, gExecFile);

    /* execute link instruction */
    status = system(command);
    free(command);
    if (status) error(62, (char *) 0);

    return;
}
