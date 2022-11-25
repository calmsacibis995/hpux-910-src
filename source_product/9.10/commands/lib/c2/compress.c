/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

node *release();
char *copy(), *strcat(), *strcpy();

/*
 * Compress redundant "constant" strings.
 *
 * For example:
 *
 *	L1  dc.b  'hi there'	=>	L1  dc.b  'hi there'
 *	L2  dc.b  'there'	=>	L2  equ   L1+3
 *
 * This is not always cool, of course, since your strings might not
 * be as constant as you think they are.  Use this option with caution.
 */
 
#define TABLE_SIZE 2000
#define DUMMY_ENTRY ((struct node *) 1)

compression()
{
	typedef node *nodeptr;
	nodeptr strs[TABLE_SIZE];
	register nodeptr p, next, *tabp, *p1, *p2;
	register int n;
	char buf[100];

	/* First, collapse adjacent strings without labels */
	recombine();

	for (p=first.forw, tabp=strs; p!=NULL; p = p->forw) {

		/*
		 * We want an L<number> label,
		 * followed by a dc.b.
		 */
		if (p->op!=LABEL)
			continue;
			
		next = p->forw;
		if (next==NULL || next->op!=DC || next->subop!=BYTE)
			continue;


		if (tabp < &strs[TABLE_SIZE-1])
			*tabp++ = p;
	}
	
	*tabp = NULL;				/* terminate the table */

	/* Look through the table */
	for (p1=strs; *p1!=NULL; p1++) {
		p = *p1;
		for (p2=strs; *p2!=NULL; p2++) {
			if (*p2==DUMMY_ENTRY)		/* a replaced entry? */
				continue;		/* continue if so */
			if (p1==p2)			/* the same string? */
				continue;		/* continue if so */
			
			n =  is_a_tail(p->forw->string1,(*p2)->forw->string1);

			if (n != -1) {			/* Do we match? */
				/*
				 * A match!
				 * Transform this one into an equate
				 * and wipe out its table entry.
				 */
				p=p->forw;		/* point to dc.b */
				cleanse(&p->op1);	/* wipe out string */
				p->op=EQU;
				p->mode1=ABS_L;
				if (n==0) {		/* Downright equal? */
				    p->type1=INTLAB;	/* no offset needed */
				    p->labno1=(*p2)->labno1;
				} else {		/* we have an offset */
				    p->type1=STRING;
				    sprintf(buf, "L%d+%d", (*p2)->labno1, n);
				    p->string1 = copy(buf);
				}

				/* Eliminate this table entry */
				*p1 = DUMMY_ENTRY;

				break;		/* go on to next p1 */
			}
		}
	}
}


int
is_a_tail(a,b)
register char *a, *b;
{
	register int la, lb;

	la = strlen(a);
	lb = strlen(b);

	if (la==0 || lb==0)		/* Are they null strings? */
		return (-1);		/* No sense compressing those! */

	if (la>lb)			/* If first string is larger */
		return (-1);		/* then a can't be a substring of b */

	if (equstr(a, b+lb-la))		/* If a is a tail of b: */
		return (lb-la);		/* return the offset */
	else
		return (-1);		/* return failure */
}



/*
 * recombine()
 *
 * recombine adjacent strings without labels into a single string.
 *
 * For example:
 *
 * L1	dc.b	'hell'		=>  L1	dc.b	'hello there'
 *	dc.b	'o there'	=>
 *
 * If a string becomes too long to print it will be split at output time.
 */

recombine()
{
	register node *p, *next;

	for (p=first.forw; p!=NULL; p = p->forw) {

		/*
		 * Look for a L<number> label
		 * followed by a dc.b.
		 */
		if (p->op!=LABEL)
			continue;

		p=p->forw;			/* go on to dc.b */

		if (p==NULL) break;
		if (p->op!=DC || p->subop!=BYTE)
			continue;

		/*
		 * As long as dc.b's follow, combine them with us.
		 */
		
		while ((next=p->forw)!=NULL
		&& next->op==DC && next->subop==BYTE
		&& strlen(p->string1)<200) {
			char buffer[2000];

			strcpy(buffer, p->string1);
			strcat(buffer, next->string1);
			cleanse(&p->op1);
			p->mode1=ABS_L;
			p->type1=STRING;
			p->string1=copy(buffer);
			release(next);
		}
	}
}
