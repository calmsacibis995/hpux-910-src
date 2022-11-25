/*

    create_assertion_listing_driver()

    Create an assembly file to perform assertion listing. This routine
    is little more than a driver to call various other routines to
    create the appropriate assembly language driver routine.

*/

#include "fizz.h"

void create_assertion_listing_driver()
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
    Dprint_verify_code();		/* call code to perform assertions */
    Dprint_cleanup_code();		/* routine epilogue */
    Dprint_dataset_info_table();	/* table of datasets and datanames */
    Dprint_dsetpar_table();		/* parameter tables */
    Dprint_parameter_routine();		/* create routine for parameters */
    Dprint_dsetver_table();		/* assert tables */
    Dprint_dataset_names_table();	/* dataset name table */
    Dprint_assertion_names_table();	/* assertion name table */
    Dprint_assertion_listing_routine(); /* routine for assertion listing */
    Dprint_stackeven_routine();		/* stack even routine */
    Dprint_stackodd_routine();		/* stack odd routine */

    close_output_file(gDriverFile);

    return;
}
