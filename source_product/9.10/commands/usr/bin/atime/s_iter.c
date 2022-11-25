/*

    store_iterations()

    Store the iteration count.

*/

#include "fizz.h"

void store_iterations(source, iterations)
short source;	/* command line or input file */
char *iterations;
{
    static BOOLEAN cmdline = FALSE;
    static BOOLEAN cmdline_error = FALSE;
    static BOOLEAN infile = FALSE;
    static BOOLEAN infile_error = FALSE;
    BOOLEAN parse_iterations, error_flag;
    register unsigned long c, count;
    register char *ptr;

    parse_iterations = FALSE;

    if (source == CMDLINE) {
	if (cmdline) {
	    if(!cmdline_error) {
	        error(37, (char *) 0);	/* multiple iterate's in command line */
	        cmdline_error = TRUE;
	    };
	} else {
	    parse_iterations = TRUE;
	    cmdline = TRUE;
        };
    } else {
	if (infile) {
	    if (!infile_error) {	/* multiple iterate's in input file */
	        error_locate(38);
	        infile_error = TRUE;
	    };
	} else {
	    parse_iterations = TRUE;
	    infile = TRUE;
        };
    };

    /* parse the iterations string */
    if (parse_iterations) {
	ptr = iterations;
	count = 0;
        error_flag = FALSE;
	while (c = (unsigned long) *ptr++) { /* get next character */
	    if ((c >= '0') && (c <= '9')) {  /* digit? */
		if ((count > 0x19999999) || ((count == 0x19999999) 
		  && (c >= '6'))) {          /* overflow? */
		    error_flag = TRUE;
		    error(15, iterations);
		    break;
		};
		count += count;		     /* update count */
		count += count << 2;
		count += c - '0';
	    } else {	/* bad character */
		error_flag = TRUE;
		error(4, iterations);
		break;
	    };
	};

	if (!error_flag) {
	    if (!count) {	/* zero count is an error */
		if (source == CMDLINE) error(75, (char *) 0);
		else error_locate(113);
	    } else if ((source == CMDLINE) || !cmdline) gIterations = count;
	};
    };

    return;
}
