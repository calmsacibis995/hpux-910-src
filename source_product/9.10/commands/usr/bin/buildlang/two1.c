/* @(#) $Revision: 64.1 $ */      
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

/* two1_init: initialize a stand-alone 2-1 character.
** Get here when you see a '<' after the keyword 'sequence' but not
** inside a priority set.
*/
void
two1_init()
{
	extern void rang_num();
	extern void con_num();
	extern void two1x();

	if (left != AVAILABLE && right == AVAILABLE)
		rang_num();		/* be sure we have a range */

	number = con_num;		/* do a shift number on number */
	two1_xec = two1x;		/* do a 2-1 execute here */
	left = right = AVAILABLE;	/* numeric pair available */
}

/* two1x: process a stand-alone 2-1 character.
** Called from two1_end().
*/
void
two1x()
{
	extern void l_insert();
	list *p;

	p = (list *) malloc(sizeof(list));	/* set up list element */
	p->first = left;		/* set 1st char of 2-1 */
	p->second = right;		/* set 2nd char of 2-1 */
	p->seq_no = seq_no++;		/* set sequence number of this 2-1 */
	p->pri_no = 0;			/* stand-alone 2-1 has priority 0 */
	p->next = NULL;
	l_insert(p);			/* insert into the list */
	left = right = AVAILABLE;	/* numeric pair available */
}

/* two1_end: process a right angle bracket.
** Get here when we see a '>' (within or outside a priority set).
** This ends a 2-1 character pair.
** We must have seen only 1 left & right angle bracket, and must have
** a pair of numbers.  After this syntax checking,
** execute the 2-1 character routines.
*/
void
two1_end()
{
	if ( lab!=1 || rab!=1 )
		error(EXPR);
	else if (left == AVAILABLE || right == AVAILABLE)
		error(EXPR);
	else {
		lab = rab = 0;
		(*two1_xec)();
	}
	number = rang_num;
}
