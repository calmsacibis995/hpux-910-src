/*

    store_nolist

    Set for no listing.

*/

#include "fizz.h"

void store_nolist(source)
short source; /* command line or input file? */
{
    static BOOLEAN cmdline = FALSE;
    static BOOLEAN cmdline_error = FALSE;
    static BOOLEAN infile = FALSE;
    static BOOLEAN infile_error = FALSE;

    if (source == CMDLINE) {
	if (cmdline) {
	    if(!cmdline_error) {
	        error(39, (char *) 0);	/* multiple nolist's in command line? */
	        cmdline_error = TRUE;
	    };
	} else {
	    gfNolist = TRUE;
	    cmdline = TRUE;
        };
    } else {
	if (infile) {
	    if (!infile_error) {
	        error_locate(40);	/* multiple nolist's in input file? */
	        infile_error = TRUE;
	    };
	} else {
	    gfNolist = TRUE;
	    infile = TRUE;
        };
    };

    return;
}
