/* $Header: slvtol.c,v 2.0 86/06/10 09:12:53 nicklin Exp $ */

/*
 * slvtol() converts a vector back into a singly-linked list and deletes the
 * vector. Each non-NULL item in the vector is added to the end of the list.
 * The vector is assumed to be the same length as the original list.
 */
#include <stdio.h>
#include "null.h"
#include "slist.h"

void
slvtol(slist, slv)
	SLIST *slist;			/* pointer to list head block */
	SLBLK **slv;			/* ptr to singly-linked list vector */
{
	SLBLK *cblk;			/* current list block */

	int cbi;			/* current block index */
	int nk = 0;			/* number of items in new list */
	register SLBLK *cbp;		/* current block pointer */
	
	slist->head = slist->tail = NULL;

	for (cbi=0; cbi < SLNUM(slist); cbi++)
		{
		cbp = slv[cbi];
		if (cbp != NULL)
			{
			cbp->next = NULL;
			if (slist->tail == NULL)
				{
				slist->head = slist->tail = cbp;
				}
			else	{
				slist->tail = slist->tail->next = cbp;
				}
			nk++;
			}
		}
	slist->nk = nk;
	free((char *) slv);
}
