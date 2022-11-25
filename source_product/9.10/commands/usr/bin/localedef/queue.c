/* @(#) $Revision: 70.1 $ */     
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

static int front;		/* front element of queue */
static int rear;		/* rear element of queue */
static queue q[MAX_Q+1];	/* priority or don't-care queue */

void
q_init()
{
	front = rear = MAX_Q;
}

static int
q_empty()
{
	if (front == rear) {
		return TRUE;
	} else {
		return FALSE;
	}
}

queue *
q_remove()
{
	if (q_empty()) {
		return NULL;
	} else {
		if (front == MAX_Q) {
			front = 0;
		} else {
			front += 1;
		}
		return &q[front];
	}
}

void
q_insert(x)
queue *x;
{
	if (rear == MAX_Q) {
		rear = 0;
	} else {
		rear += 1;
	}
	if (rear == front ) {
		error(Q_OVER);
	} else {
		q[rear].num1 = x->num1;
		q[rear].num2 = x->num2;
		q[rear].type = x->type;
	}
}


/******************************************************************************/
extern list *two1_head, *two1_tail;

/* l_insert: insert an element into the single linked list
** which contains the infomation about 2-1 characters.
** As we insert the element, we put this linked list in
** ascending order according to the first char of a 2-1.
*/
void
l_insert(x)
list *x;
{
	list *sp, *last;			/* list search pointer */

	if (two1_head == NULL)			/* first element */
		two1_head = two1_tail = x;
	else {
		for (sp = two1_head ;
		     sp != NULL && sp->first <= x->first ;
		     last = sp, sp = sp->next)	/* find the correct position */
			;
		if (sp == two1_head) {		/* at beginning of the list */
			x->next = two1_head;
			two1_head = x;
		}
		else if (sp != NULL) {		/* in the middle of the list */
			x->next = last->next;
			last->next = x;
		} else {			/* at the end of the list */
			x->next = NULL;
			two1_tail->next = x;
			two1_tail = x;
		}
	}
}
