/* @(#) $Revision: 70.3 $ */    
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

/* dc_init: initialize a sequence don't-care set.
** Get here when you see a '{'.
void
dc_init()
{
	extern void q_init();
	extern void rang_num();
	extern void dc_range();

	if (left != AVAILABLE && right == AVAILABLE) {
		lcb--;
		rang_num();
		left = right = AVAILABLE;
		lcb++;
	}

	q_init();			/* initialize queue 
	number = rang_num;		/* tell main to do a range on number 
	rang_exec = dc_range;		/* give rang_num an execute routine 
}
*/

/* dc_range: process a don't-care set range number.
** Called from rang_num() through rang_exec.
*/
void
dc_range()
{
	extern void q_insert();
	queue x;			/* a queue element */

	x.num1 = left;			/* set 'left' range for queue */
	x.num2 = right;			/* set 'right' range for queue */
	q_insert(&x);			/* put element on the queue */
	left = right = AVAILABLE;	/* flag range numbers available */
}

/* dc_exec: process a don't-care set.
** Get elements off the queue one by one.  Set the sequence number to be zero.
*/
void
dc_exec()
{
	extern queue *q_remove();
	queue *x;
	register int i;

/*	if (META) error(EXPR); */

	while (x = q_remove()) {
		for (i=x->num1; i<=x->num2 ; i++) {
			seq_tab[i].seq_no = 0;
			seq_tab[i].type_info = DC;
		}
	}
}

/* dc_end: process a right curly brace.
** Get here when we see a '}'.  This ends a don't-care set.
** We must have seen only 1 left & right brace and must have
** a range of numbers.  After this syntax checking,
** execute the don't-care set routine.
** At the end, reset the rang_exec function.
*/
void
dc_end()
{
	extern void dc_exec();
	extern void col_exec();
	/*
	 * Buildlang code
	if ( lcb!=1 || rcb!=1 )
		error(EXPR);
	else
	*/
	if ( left != AVAILABLE && right == AVAILABLE)
		rang_num();

	lcb = rcb = 0;
	dc_exec();
	rang_exec = col_exec;

	left = right = AVAILABLE;
}
