/*

    store_assert_filename()

    Store the assertion file name.

*/

#include "fizz.h"

void store_assert_filename(source, filename)
short source;	/* command line or input file */
char *filename;
{
    static BOOLEAN cmdline = FALSE;
    static BOOLEAN cmdline_error = FALSE;
    static BOOLEAN infile = FALSE;
    static BOOLEAN infile_error = FALSE;
    BOOLEAN store;
    unsigned long length;

    store = FALSE;

    /* command line assertion file name takes precedence over assertion
       file in input file; check for multiple declarations */
    if (source == CMDLINE) {
	if (cmdline) {
	    if(!cmdline_error) {	/* multiple from command line? */
	        error(35, (char *) 0);
	        cmdline_error = TRUE;
	    };
	} else {
	    store = TRUE;
	    cmdline = TRUE;
        };
    } else {
	if (infile) {
	    if (!infile_error) {	/* multiple from input file? */
	        error_locate(36);
	        infile_error = TRUE;
	    };
	} else {
	    if (!cmdline) store = TRUE;	/* command line takes precedence */
	    infile = TRUE;
        };
    };

    /* store the file name */
    if (store) {
	length = strlen(filename);
	if (*filename == '"') { /* remove leading quote */
	    filename++;
	    length--;
	};
	if (*(filename + length - 1) == '"')  /* remove trailing quote */
	    length--;
	CREATE_STRING(gAssertFile, length + 1);
	(void) strncpy(gAssertFile, filename, length);
	gAssertFile[length] = '\0';
	gfAssertionFile = TRUE;
    };

    return;
}
