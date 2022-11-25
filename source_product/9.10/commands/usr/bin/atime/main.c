/*****************************************************************
**								**
**  FIZZ - an assembly language sequence timing utility		**
**								**
**  Author: Mark McDowell					**
**  Date:   October 1987					**
**  								**
**  Basic operation is as follows:				**
**      1. initialize						**
**      2. parse command line					**
**      3. read input file: generate code file and store	**
**	   pertinent data internally				**
**      4. assemble the code file				**
**	5. generate appropriate driver source file depending	**
** 	   on fizz mode						**
**	6. assemble the driver file				**
**	7. link object files and execute			**
**  								**
**  In general, the code file consists of three subroutines     **
**  which are called from the driver file. These three subrou-  **
**  tines are: initialization (___Zinit), timing (___Ztime),    **
**  and verify (___Zverify). A fourth subroutine (___Zovhd) is  **
**  used to determine timing overhead during performance anal-  **
**  ysis, but is actually a subset part of the timing routine.  **
**  Not all of these subroutines may exist in all cases because **
**  of the way that the user has written his code, or because   **
**  of the particular fizz mode (performance analysis, execu-   **
**  tion profiling, or assertion listing). These subroutines    **
**  are called in proper succession to perform the desired op-  **
**  eration, being especially careful to preserve exact machine **
**  state between calls including stack pointer, condition      **
**  codes, and allowing for the fact that the user may have 	**
**  completely convoluted the environment. These subroutines    **
**  may in turn call routines in the driver file to perform	**
**  such things as "stack even", fetch dynamic parameters, in-  **
**  struction hit counts, perform assertions, etc. In general,  **
**  everything that doesn't have to be in the code file will be **
**  put into the driver file to minimize the possibility of     **
**  name collisions between the user's code and the code gener-	**
**  ated by fizz.						**
**  								**
**  The driver file contains everything else --- program start- **
**  up, support routines, printing routines, the iterations     **
**  loop, etc. When the driver and code programs are assembled  **
**  and linked, they form a complete, standalone program and    **
**  fizz is no longer needed. In fact, this new program is then **
**  exec'd over fizz so that there will not be an extra process **
**  executing on the system and therefore contending for re-    **
**  sources that would skew fizz results.			**
**  								**
**  The code file is generated as the input file is scanned the **
**  first time. It is also during this scan that all pertinent  **
**  information is stored internally for later generating a     **
**  driver file. A second scan of the input file may be neces-  **
**  sary to generate a table for program listing.		**
**  								**
**  Fizz is not overly meticulous when it comes to the actual   **
**  instructions in the input file. It recognizes fizz pseudo-  **
**  ops and verifies their syntax. It also recognizes all 68020 **
**  instructions as well as 68881 and Dragon mnemonics and var- **
**  ious generic pseudo-ops, but it does not verify their syn-  **
**  taxes. This would make fizz quite large and much more com-  **
**  plex. Instead, this syntax checking has been left to the    **
**  assembler itself. This could leave the user in the dark if  **
**  there were an error in his code, but this is conpensated by **
**  fizz's leaving file name and line number comments in the    **
**  assembly file which the user can then use to easily trace   **
**  back to the problem.					**
**  								**
**  Actually, fizz only has a very few reasons for recognizing  **
**  assembler instructions at all. One is for verifying that a  **
**  particular instruction can support parameters. Another is   **
**  to determine if an instruction is executable or not for the **
**  execution profiling mode. Also in this mode, it is impor-   **
**  tant to translate certain instructions to equivalent forms  **
**  to prevent offset overflows resulting from added code.	**
**  								**
**  Note that routines beginning with "D" create code for the   **
**  driver file and those beginning with "C" create code for    **
**  the code file. Global variables begin with a "g" and are 	**
**  all defined in this file. Global flags begin with "gf".	**
**  								**
*****************************************************************/

static char *HPUX_ID = "@(#) $Revision: 56.2 $";

#include "fizz.h"
#include <signal.h>

char		gBuffer[BUFFER_SIZE];		/* input line buffer         */

short		gMode 		= PERFORMANCE_ANALYSIS;
short		gSection 	= FIZZ_INIT;

unsigned long	gDatanameCount 	= 0;		/* number of datanames       */
unsigned long	gDatasetCount 	= 0;		/* number of datasets        */
unsigned long	gParameterCount = 0;		/* number of parameter stmts */
unsigned long	gAssertionCount = 0;		/* number of assertion stmts */

BOOLEAN		gfAbort 	= FALSE;	/* program abort flag        */
BOOLEAN		gfInitSection 	= FALSE;        /* code has init section     */
BOOLEAN		gfTimeSection 	= FALSE;        /* code has time section     */
BOOLEAN		gfVerifySection = FALSE;        /* code has verify section   */
BOOLEAN		gfAssertionFile = FALSE;        /* assert file exists        */
BOOLEAN		gfStackeven 	= FALSE;	/* "stack even" stmt used    */
BOOLEAN		gfStackodd 	= FALSE;	/* "stack odd" stmt used     */
BOOLEAN		gfNolist 	= FALSE;	/* nolist flag		     */
BOOLEAN		gfMessages 	= TRUE;		/* print error msgs flag     */

unsigned long	gIterations 	= 1000000;	/* number of iterations      */

char		*gFile 		= (char *) 0;   /* current file name         */
unsigned long	gLine 		= 0;            /* current line number       */
FILE		*gOutput;			/* output file               */

char		*gInputFile 	= (char *) 0;   /* input file name           */
char		*gAssertFile 	= (char *) 0;   /* assertion file name       */
char		*gDriverFile 	= (char *) 0;   /* driver file name          */
char		*gDriverObjFile = (char *) 0;   /* driver object file name   */
char		*gCodeFile 	= (char *) 0;   /* code file name            */
char		*gCodeObjFile 	= (char *) 0;   /* code object file name     */
char		*gExecFile 	= (char *) 0;   /* executable file name      */

/*

    main

*/

main(argc, argv)
int argc;
char *argv[];
{

    /* Handle signals */
    signal(SIGINT, signal_abort);
    signal(SIGQUIT, signal_abort);
    signal(SIGTERM, signal_abort);

    /* Parse the command line */
    parse_command_line(argc, argv);
    if (gfAbort) abort();

    /* Scan the input file and create the code file */
    process_input_file();
    if (gfAbort) abort();

    /* Finish the last subroutine put into the code file */
    if (gSection == TIME) Cprint_time_cleanup_code();
    else if (gSection == VERIFY) Cprint_verify_cleanup_code();

    /* Error if no time section */
    if (!gfTimeSection) {
	error(104, (char *) 0);
	abort();
    };

    /* If there were dynamic parameters, e.g. "mov.l &$x,%d1", 
       create the code to perform these instructions */
    if (gParameterCount) Cprint_parameter_routines();

    /* Close files */
    close_input_files();
    close_output_file(gCodeFile);

    /* Verify that datasets referenced from the command line are defined */
    if (gMode != PERFORMANCE_ANALYSIS) {
	verify_list();
        if (gfAbort) abort();
    };

    /* If there's a "dataname", there must be one or more "datasets" */
    if (gDatanameCount && !gDatasetCount) {
	error(118, (char *) 0);
	abort();
    };

    /* Assemble the code file */
    assemble_code();
    if (gfAbort) abort();

    /* If there's an assertion file, process it */
    if (gfAssertionFile) process_assert_file();
    if (gfAbort) abort();

    /* Calculate what the true number of iterations will be */
    calculate_iterations();
    if (gfAbort) abort();

    /* Create the appropriate driver file */
    if (gMode == PERFORMANCE_ANALYSIS) create_performance_driver();
    else if (gMode == EXECUTION_PROFILING) create_profiling_driver();
    else create_assertion_listing_driver();
    if (gfAbort) abort();

    /* Assemble the driver file */
    assemble_driver();
    if (gfAbort) abort();

    /* Link the driver and code object files together */
    link_executable();
    if (gfAbort) abort();

    /* Execute the created program */
    execute();   /* ...should never return... */
    if (gfAbort) abort();

    cleanup(TRUE);

    return;
}
