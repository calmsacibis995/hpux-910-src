/* @(#) $Revision: 27.1 $ */    
/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

/*
 * Flush stdout.
 */
__cflush()
{
	fflush(SP->term_file);
}
