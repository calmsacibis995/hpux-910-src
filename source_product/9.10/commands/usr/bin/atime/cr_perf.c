/*

    create_performance_driver()

    Create an assembly file to perform performance analysis.  This
    routine is little more than a driver to call various other routines
    to create the appropriate assembly language driver routine.

*/

#include "fizz.h"

void create_performance_driver()
{
    CREATE_STRING(gDriverFile, L_tmpnam);
    (void) tmpnam(gDriverFile);
    open_output_file(gDriverFile);

    Dprint_startup_code();		/* routine prologue */
    Dprint_open_code();			/* open the output file */
    Dprint_line_code();			/* print separator line */
    Dprint_title_code();		/* print title */
    Dprint_comment_code();		/* print comments */
    Dprint_info_code();			/* print name, machine, date...*/
    Dprint_iterations_code();		/* print iteration count */
    Dprint_verify_code();		/* call code to perform assertions */
    Dprint_time_code();			/* call code to perform timing */
    Dprint_stat_code();			/* print timing information */
    Dprint_listing_code();		/* print input file listing */
    Dprint_cleanup_code();		/* routine epilogue */
    Dprint_count_table();		/* dataset counts table */
    Dprint_dsetpar_table();		/* parameter tables */
    Dprint_parameter_routine();		/* routine for parameters */
    Dprint_dsetver_table();		/* assert tables */
    Dprint_dataset_names_table();	/* dataset names table */
    Dprint_assertion_names_table();	/* assertion names table */
    Dprint_assertion_routine();		/* perform assertion */
    Dprint_stackeven_routine();		/* stack even routine */
    Dprint_stackodd_routine();		/* stack odd routine */

    close_output_file(gDriverFile);

    return;
}
