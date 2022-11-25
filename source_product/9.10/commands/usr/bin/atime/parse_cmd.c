/*

    parse_command_line()

    Parse the fizz command line.

*/

#include "fizz.h"

void parse_command_line(argc, argv)
int argc;
char *argv[];
{
    int i;
    BOOLEAN infile;
    char *ptr;

    infile = FALSE;	/* no input file yet */

    for (i = 1; i < argc; i++) {
	ptr = argv[i];
	if (!infile && *ptr++ == '-') { /* option? */
	    switch(*ptr++) {
	    case 'a':	/* assert file */
		if (!*ptr) error(99, (char *) 0);
		else store_assert_filename(CMDLINE, ptr); 
		break;
	    case 'i':   /* iteration count */
		if (!*ptr) error(103, (char *) 0);
		else store_iterations(CMDLINE, ptr); 
		break;
	    case 'l':   /* assertion listing dataset name */
		store_list_name(ASSERTION_LISTING, ptr); 
		break;
	    case 'n':   /* nolist */
		if (*ptr) error(49, argv[i]);
		store_nolist(CMDLINE, ptr); 
		break;
	    case 'p':	/* execution profiling dataset name */
		store_list_name(EXECUTION_PROFILING, ptr); 
		break;
	    case 't': 	/* title */
		store_title(CMDLINE, ptr); 
		break;
	    default:	/* unrecognized option */
		error(49, argv[i]);
		break;
	    };
	} else {
	    ptr = argv[i];
	    if (infile) {  /* if we have input file, this must be output file */
		if (!*ptr) error(54, 0); /* null name */
		else store_output_filename(CMDLINE, ptr);
		i++;
		break;
	    } else { 	   /* must be input file name */
		if (!*ptr) error(51, 0); /* null name */
		else store_input_filename(ptr);
		infile = TRUE;
	    };
	};
     };

     if (i < argc) error(67, (char *) 0);	/* extra arguments? */
     else if (!infile) error(95, (char *) 0);	/* missing input file? */

     return;
}
