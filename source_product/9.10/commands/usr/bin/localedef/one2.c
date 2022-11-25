/* @(#) $Revision: 70.1 $ */      
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

/* one2_init: initialize a stand-alone 1-2 character.
** Get here when you see a '[' after the keyword 'sequence' but not
** inside a priority set.
*/
void
one2_init()
{
	extern void rang_num();
	extern void con_num();
	extern void one2x();

	if (left != AVAILABLE && right == AVAILABLE) {
		rang_num();		/* be sure we have a range */
	}

	number = con_num;		/* do a shift number on number */
	one2_xec = one2x;		/* do a 1-2 execute here */
	left = right = AVAILABLE;	/* numeric pair available */
}

/* one2x: process a stand-alone 1-2 character.
** Called from one2_end().
*/
void
one2x()
{
	seq_tab[left].seq_no = seq_no++;	/* set sequence number */
	seq_tab[left].type_info = ONE2 | onei;	/* set flag & index info */
	one2_tab[onei].seq_no = right;	/* store the actual char here for now */
	one2_tab[onei++].type_info = 0;	/* stand-alone 1-2 has priority 0 */
	left = right = AVAILABLE;	/* numeric pair available */
}

/* one2_end: process a right square bracket.
** Get here when we see a ']' (within or outside a priority set).
** This ends a 1-2 character pair.
** We must have seen only 1 left & right square bracket, and must have
** a pair of numbers.  After this syntax checking,
** execute the 1-2 character routines.
*/
void
one2_end()
{
	if ( lb!=1 || rb!=1 )
		error(EXPR);
	else if (left == AVAILABLE || right == AVAILABLE)
		error(EXPR);
	else {
		lb = rb = 0;
		(*one2_xec)();
	}
	number = rang_num;
}
