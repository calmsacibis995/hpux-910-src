/* @(#) $Revision: 70.4 $ */   
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"


/* pri_range: process a priority set range number.
** Called from rang_num() through rang_exec.
*/
void
pri_range()
{
	extern void q_insert();
	queue x;			/* a queue element */
	extern int pri_seq;

	x.num1 = left;			/* set 'left' range for queue */
	x.num2 = right;			/* set 'right' range for queue */
	x.type = CONSTANT;		/* flag a range value (not 1-2, 2-1) */
	q_insert(&x);			/* put element on the queue */
	left = right = AVAILABLE;	/* flag range numbers available */
	pri_seq = TRUE; /* used for new collation method of Posix 11.2
			   See coll.c for its usage
			 */
}

/* pri_exec: process a priority set.
** Three kinds of things can be in a priority set: a range,
** 2-1 characters and 1-2 characters.  Get elements off the queue one
** by one.  Use a switch to determine the type of the elememt.
* Set the sequence number and type information,
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
	extern int pri_seq;

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
	pri_seq = FALSE; /* see coll.c for its usage */
	q_init();
}


/*
** PRIORITY SET TWO TO ONE CHARACTERS
*/

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
	pri_seq = TRUE;
}

/*
** PRIORITY SET ONE TO TWO CHARACTERS
*/

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
	pri_seq = TRUE;
}
