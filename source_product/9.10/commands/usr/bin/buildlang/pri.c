/* @(#) $Revision: 64.1 $ */   
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

/* pri_init: initialize a sequence priority set.
** Get here when you see a '('.
*/
void
pri_init()
{
	extern void q_init();
	extern void rang_num();
	extern void pri_range();
	extern void p_2_1init();
	extern void p_1_2init();

	if (left != AVAILABLE && right == AVAILABLE) {
		lp--;
		rang_num();
		left = right = AVAILABLE;
		lp++;
	}

	q_init();			/* initialize queue */
	number = rang_num;		/* tell main to do a range on number */
	rang_exec = pri_range;		/* give rang_num an execute routine */
	l_angle = p_2_1init;		/* tell main to come here on '<' */
	l_square = p_1_2init;		/* tell main to come here on '[' */
}

/* pri_range: process a priority set range number.
** Called from rang_num() through rang_exec.
*/
void
pri_range()
{
	extern void q_insert();
	queue x;			/* a queue element */

	x.num1 = left;			/* set 'left' range for queue */
	x.num2 = right;			/* set 'right' range for queue */
	x.type = CONSTANT;		/* flag a range value (not 1-2, 2-1) */
	q_insert(&x);			/* put element on the queue */
	left = right = AVAILABLE;	/* flag range numbers available */
}

/* pri_exec: process a priority set.
** Three kinds of things can be in a priority set: a range,
** 2-1 characters and 1-2 characters.  Get elements off the queue one
** by one.  Use a switch to determine the type of the elememt.
** Set the sequence number and type information,
** or other appropriate actions.
*/
void
pri_exec()
{
	extern queue *q_remove();
	extern void l_insert();			/* for process 2-1 char */
	queue *x;
	list *p;
	register int i,j;

	if (META) error(EXPR);
	for (i=0 ; x = q_remove() ; ) {
		switch (x->type) {
		case CONSTANT:
			/* set the sequence number & type infomation. */
			for (j=x->num1; j<=x->num2 ; j++) {
				seq_tab[j].seq_no = seq_no;
				seq_tab[j].type_info = i++;
			}
			break;
		case TWO1:
			/* insert into a temporary linked list. */
			left = x->num1; right = x->num2;
			p = (list *) malloc(sizeof(list));
			p->first = left;
			p->second = right;
			p->seq_no = seq_no;
			p->pri_no = i++;
			p->next = NULL;
			l_insert(p);
			break;
		case ONE2:
			/* set the sequence number & type infomation. */
			/* also set the entries of one2_tab, */
			/* except the seq_no field of the 2nd char */
			/* is stored the actual char for now. */
			left = x->num1; right = x->num2;
			seq_tab[left].seq_no = seq_no;
			seq_tab[left].type_info = ONE2 | onei;
			one2_tab[onei].seq_no = right;
			one2_tab[onei++].type_info = i++;
			break;
		}
		left = right = AVAILABLE;
	}
	if (--i > max_pri_no)
		max_pri_no = i;
}

/* pri_end: process a right paren.
** Get here when we see a ')'.  This ends a priority set.
** We must have seen only 1 left & right paren and must have
** a range of numbers.  After this syntax checking, execute the
** priority set routine. At the end, reset the l_angle function
** to process standalone 2-1 characters; reset the rang_exec
** function to process single collate range/character.
*/
void
pri_end()
{
	extern void pri_exec();
	extern void col_exec();
	extern void two1_init();
	extern void one2_init();

	if ( lp!=1 || rp!=1 )
		error(EXPR);
	else if (left != AVAILABLE && right == AVAILABLE)
		rang_num();

	lp = rp = 0;
	pri_exec();
	seq_no++;
	l_angle = two1_init;
	l_square = one2_init;
	rang_exec = col_exec;
}

/*
** PRIORITY SET TWO TO ONE CHARACTERS
*/

/* p_2_1init: initialize a 2-1 character within a priority set.
** Get here when you see a '<' within a priority set.
*/
void
p_2_1init()
{
	extern void con_num();
	extern void rang_num();
	extern void p_2_1exec();

	if (left != AVAILABLE && right == AVAILABLE)
		rang_num();		/* be sure we have a range */

	number = con_num;		/* do a shift number on number */
	two1_xec = p_2_1exec;		/* do a 2-1 execute here */
	left = right = AVAILABLE;	/* numeric pair available */
}

/* p_2_1exec: process a priority set 2-1 character.
** Called from two1_end().
*/
void
p_2_1exec()
{
	extern void q_insert();
	queue x;			/* a queue element */

	x.num1 = left;			/* set 'left' range for queue */
	x.num2 = right;			/* set 'right' range for queue */
	x.type = TWO1;			/* flag a 2-1 (not 1-2 or range) */
	q_insert(&x);			/* put element on the queue */
	left = right = AVAILABLE;	/* flag range numbers available */
}

/*
** PRIORITY SET ONE TO TWO CHARACTERS
*/

/* p_1_2init: initialize a 1-2 character within a priority set.
** Get here when you see a '[' within a priority set.
*/
void
p_1_2init()
{
	extern void con_num();
	extern void rang_num();
	extern void p_1_2exec();

	if (left != AVAILABLE && right == AVAILABLE)
		rang_num();		/* be sure we have a range */

	number = con_num;		/* do a shift number on number */
	one2_xec = p_1_2exec;		/* do a 1-2 execute here */
	left = right = AVAILABLE;	/* numeric pair available */
}

/* p_1_2exec: process a priority set 1-2 character.
** Called from one2_end().
*/
void
p_1_2exec()
{
	extern void q_insert();
	queue x;			/* a queue element */

	x.num1 = left;			/* set 'left' range for queue */
	x.num2 = right;			/* set 'right' range for queue */
	x.type = ONE2;			/* flag a 1-2 (not 2-1 or range) */
	q_insert(&x);			/* put element on the queue */
	left = right = AVAILABLE;	/* flag range numbers available */
}
