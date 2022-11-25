/* $Header: slvect.c,v 2.0 86/06/10 09:12:17 nicklin Exp $ */

/*
 * slvect() converts a singly-linked list into a vector. Each item in
 * the vector points to a list item. Returns a pointer to the vector,
 * otherwise NULL if no items in list or out of memory.
 */
#include <stdio.h>
#include "null.h"
#include "slist.h"

SLBLK **
slvect(slist)
	SLIST *slist;			/* pointer to list head block */
{
	char *malloc();			/* memory allocator */
	SLBLK **vp;			/* pointer to block pointer array */
	SLBLK **svp;			/* ptr to start of block ptr array */
	SLBLK *cblk;			/* current list block */

	if (SLNUM(slist) <= 0)
		return(NULL);
	else if ((svp = (SLBLK **) malloc((unsigned)SLNUM(slist)*sizeof(SLBLK *))) == NULL)
		{
		nocore();
		return(NULL);
		}
	for (vp=svp, cblk=slist->head; cblk != NULL; vp++, cblk=cblk->next)
		{
		*vp = cblk;
		}
	return(svp);
}
