/* $Header: slinit.c,v 2.0 86/06/10 09:51:12 nicklin Exp $ */

/*
 * Author: Peter J. Nicklin
 */

/*
 * slinit() returns a pointer to the head block of a new list, or null
 * pointer if out of memory.
 */
#include <stdio.h>
#include "null.h"
#include "slist.h"

SLIST *
slinit()
{
	char *malloc();			/* memory allocator */
	SLIST *slist;			/* pointer to list head block */

	if ((slist = (SLIST *) malloc(sizeof(SLIST))) == NULL)
		{
		nocore();
		return(NULL);
		}
	slist->nk = 0;
	slist->maxkey = 0;
	slist->head = slist->curblk = slist->tail = NULL;
	return(slist);
}
