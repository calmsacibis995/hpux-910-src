/*	SCCS	REV(47.4);	DATE(90/05/16	13:31:35) */
/* KLEENIX_ID @(#)callgraph.c	47.4 90/03/30 */

#include "mfile2"
#include "c0.h"

pPD firstoutpd;		/* chain head for output order */
pPD lastoutpd;		/* last one */
long iorder;		/* relative integration order */

/*****************************************************************************/

/* cleancg -- clean call graph by removing calls to non-integrable procs,
 *		calls to non-source procs
 */
cleancg()
{
    register pPD pd;
    register long i;

    /* Pass1 -- for all pd procs:
	mark "inlineOK" field
	set proc pd field in namerecord (pnp)
	if inlineOK
	    cp np->ffrom to pd
	    go through from list and correct "to" ptrs to point to pd
	else
	    set pd->callsfrom = NULL
	    go through from list and set "to" ptrs to zero
     */

    for (pd = firstpd; pd; pd = pd->next)
	{
	/* Set inlineOK field in pd -- tells whether OK to inline proc.
	 * The following conditions disqualify a procedure:
	 *	- is or contains an ENTRY statement
	 *	- returns CHARACTER or COMPLEX value
	 *	- too many formal parameters
	 *	- any CHARACTER or COMPLEX formal parameters
	 *	- contains ASSIGN'd FORMAT usage
	 */
	{
	long isOK;
	unsigned short type;
	register TWORD *p, *pend;

	isOK = !(pd->typeproc & 2);  			/* this is ENTRY */
 	if (pd->next && ((pd->next)->typeproc & 2))	/* contains ENTRY */
	    isOK = 0;
	if (!valid_return_type(pd->rettype))
	    isOK = 0;
	if (pd->nparms > MAXARGS)	/* too many formal parms */
	    isOK = 0;
	else
	    /* check formal parameter types */
	    for (p = pd->parmtype, pend = p + pd->nparms; p < pend; p++)
	        {
		if (!valid_param_type(*p,*p))
		    {
		    pd->hasCharOrCmplxParms = YES;  /* flag for explanatory
						     * message */
		    isOK = 0;
		    break;
		    }
	        }
	if (pd->hasasgnfmts)	/* has ASSIGN'd FORMAT usage */
	    isOK = 0;
	if (pd->imbeddedAsm)	/* has imbedded asm statement (C only) */
	    isOK = 0;
	pd->inlineOK = isOK;	/* set field in pd record */
	}

	/* set pd pointer in name record */
        {
        register pNameRec np;
	np = (pNameRec) (pnp + pd->pnpoffset);
	np->pd = pd;

	pd->noemitflag = np->noemitflag;  /* transfer noemit flag */

	/* for all inline-able procedures we clean up the call graph structure
	 * by setting the "from" field in the pd record.  Also set the "to"
	 * pointers in the individual call nodes to point to the pd, rather
	 * than the pnp record.  Calls which will not be inlined (don't meet
	 * force/size criteria) have corresponding call graph nodes removed.
	 *
	 * For non-line-able procs, we throw away all call graph info --
	 * set the "from" field to null and step through the "from" chain
	 * setting the "to" field to null (later recognized by call graph
	 * manipulation routines as a "discarded" entry.
	 */
	if (pd->inlineOK)	/* OK to inline this procedure */
	    {
	    register pCallRec cp = np->ffrom;

	    pd->callsfrom = cp;		/* set "from" pointer in pd */
	    while (cp)		/* Check all calls to this procedure */
		{
		/* check if OK procedure to inline */
		if ((cp->forced			/* call is forced */
		  || (pd->nlines < cp->clines)) /* procedure meets size */
		 && (cp->calledfrom != pd))	/* not direct recursion */
		    cp->callsto = pd;		/* point to pd record */
		else   				/* didn't meet criteria */
		    cp->callsto = (pPD) 0;	/* mark as deleted */
		cp = cp->nextfrom;
		}
	    }
	else   /* not OK to inline this procedure */
	    {
	    register pCallRec cp = np->ffrom;

	    pd->callsfrom = (pCallRec) 0;	/* no called-from chain */
	    while (cp)				/* throw away all called-from
						 * records */
		{
		cp->callsto = (pPD) 0;
		cp = cp->nextfrom;
		}
	    }
	}
	} /* for */

    /* Pass2 -- for all items in name pool (go through hash list):
     *	if pd field == 0  ( no source for procedure )
     *	    go through entire "from" list and set "to" pointers to 0
     */

    for (i = 0; i < HASHSIZE; i++)
	{
	long offset;		/* pnp offset of name record */
	register pNameRec np;	/* pointer to current name record */
	register pCallRec cp;	/* pointer to current call graph record */

	if (offset = hash[i])	/* there is a bucket for this hash value */
	    {
	    while (offset)	/* step thru all records in bucket chain */
		{
	        np = (pNameRec) (pnp + offset);
	        if (np->pd == ((pPD) 0))	/* no source for procedure */
		    {
		    /* Mark all call graph nodes to this procedure as discarded */
		    for (cp = np->ffrom; cp; cp = cp->nextfrom)
		        cp->callsto = (pPD) 0;
		    np->ffrom = (pCallRec) 0;
		    }
		offset = np->onext;    /* step to next record in bucket chain */
		}
	    }
	}
     
}  /* cleancg */

/*****************************************************************************/

/* order -- determine integration order -- the order procedures will
 *	be written to the output file
 */
order()
{
    long change;	/* has anything been added to output list this cycle */
    register pPD pd;	/* current pd */
    register pCallRec cp;  /* current call record pointer */
    register pCallRec *pcp;  /* pointer to previous call record next pointer */
    pPD *pprevpd;	/* pointer to previous next pointer for pd's not yet in
			   output ordering -- for pointer fixup */
    long toseen;	/* flag == 1 if non-null "to" seen */

    lastoutpd = firstoutpd = (pPD) 0;	/* initialize output chain pointers */

  while (firstpd)	/* while there's a procedure not in the output order */
    {
    change = 1;		/* to start the loop */
    while (change)	/* a new procedure has been added to output order */
	{
	change = 0;	/* nothing new this time (yet) */

	/* step through all procedure not yet in the output ordering */
	for (pd = firstpd, pprevpd = &firstpd; pd; pd = pd->next)
	    {

	    /* first remove all NULL entries from "to" list */
	    pcp = &(pd->callsto);
	    cp = pd->callsto;
	    toseen = 0;
	    while (cp)
		{
		if (cp->callsto == ((pPD) 0))
		    {
		    *pcp = (pCallRec) 0;
		    }
		else
		    {
		    *pcp = cp;
		    pcp = &(cp->nextto);
		    toseen = 1;
		    }
		cp = cp->nextto;
		}

    	    /* if no remaining "to" entries, add this proc to output order
	     * because it doesn't call anything else
	     */
	    if (toseen)
	        {
	        *pprevpd = pd;
	        pprevpd = &(pd->next);
	        }
	    else
	        {
	        *pprevpd = (pPD) 0;
		addtooutput(pd);
	        change = 1;		/* added item to integration list */
	        }
	    }
	}  /* while change */

    /* Nothing changed.  If not all procedures went into output list (firstpd
     *  != NULL), there must be an indirect recursion cycle.
     *  The strategy for handling these cycle is:
     *	    a) try to find a procedure not called by any one else
     *		then find any cycle in the calling chain from this proc
     *		add the procedure with a cyclic call to the output order
     *   else
     *	    b) find the procedure with the most calls to it.  Add it to
     *		the output order
     *  After adding a single procedure to the output order, go back to
     *  the regular method of determining output order.
     */
    
    if (firstpd)	/* Yep, there's a cycle here */
	{
	long	maxcalls = -1;
	long	currcalls;	/* calls to current procedure */
	long	fromseen;	/* have I seen a call to current procedure ? */
	pPD	maxpd = (pPD) 0;  /* pointer to proc with most calls */
	pPD	*maxprevpd;	/* pd before one with most calls (to update
				 * forward link) */
	
	/* try to find a procedure with no calls to it.  At the same time
	 * find the procedure with the most calls to it.
	 */
	for (pd = firstpd, pprevpd = &firstpd ; pd ;
	     pprevpd = &(pd->next), pd=pd->next)
	    {
	    currcalls = 0;		/* no calls seen yet */
	    pcp= &(pd->callsfrom);	/* address of previous "next" pointer */
	    cp = pd->callsfrom;
	    fromseen = 0;		/* no calls to current proc yet */
	    while (cp)			/* step through entire "from" chain */
		{
		if (cp->callsto == (pPD) 0)	/* deleted record -- remove */
		    *pcp = (pCallRec) 0;
		else
		    {
		    *pcp = cp;		/* add this call rec to valid chain */
		    pcp = &(cp->nextfrom);
		    fromseen = 1;
		    currcalls += cp->count;   /* update count */
		    }
		cp = cp->nextfrom;
		}  /* while */

	    if (fromseen)		/* yes, calls to this proc */
		{
		if (currcalls > maxcalls)	/* this is current max */
		    {
		    maxcalls = currcalls;
		    maxpd = pd;
		    maxprevpd = pprevpd;
		    }
		}
	    else		/* no calls to this proc -- find forced cycle */
		{
		goto findcycle;
		}
	    }  /* for */

	/* everyone waiting on everyone else -- output one with most calls */
	*maxprevpd = maxpd->next;
	addtooutput(maxpd);
	continue;	/* goto top of outermost loop -- resume old method */

findcycle:
	/* starting with "pd" which has no calls to it, find the first
	 * which calls another one in the chain.
	 * To do this we set a flag in the pd entries telling whether that
	 * proc has already been visited.  First proc that must call an already
	 * visited proc (depth-first search) gets added to the output order.
	 */

	currpd = pd;	/* proc with no calls */

	/* zero flag in all pd entries */
	for (pd = firstpd; pd; pd = pd->next)
	    pd->integorder = 0;

	while (currpd)
	    {
	    currpd->integorder = -1;	/* This proc has been visited */
	    cp = currpd->callsto;
	    while (cp)		/* find the first valid "to" call record */
		{
		if (((pd = cp->callsto) != ((pPD) 0)) && (pd->integorder == 0))
		    {	/* calls an uncalled proc -- update curr proc */
		    currpd = pd;
		    goto nextpd;
		    }
		else	/* calls an already visited proc -- search further */
		    {
		    cp = cp->nextto;
		    }
		}  /* while (cp) */

	    /* found a forced cycle -- emit this procedure.  First find
	     * previous one in chain to update pointers. */
	    if (firstpd == currpd)
		firstpd = currpd->next;
	    else
		{
		for (pd = firstpd; pd; pd = pd->next)
		    if (pd->next == currpd)
			break;
		pd->next = currpd->next;
		}

	    addtooutput(currpd);	/* add current proc to output order */
	    break;		/* to outermost while loop */
nextpd:
	    continue;
	    }  /* while (currpd) */
	}  /* if */
    }  /* while (firstpd) */
}  /* order */

/*****************************************************************************/

/* addtooutput -- add procedure to output order */
addtooutput(pd)
pPD pd;
{
    register pCallRec cp;

    /* add to end of output order chain */
    if (lastoutpd)
	lastoutpd->nextout = pd;
    else
	firstoutpd = pd;
    lastoutpd = pd;

    pd->integorder = ++iorder;	/* set relative order counter */

    /* delete all "from" items from call graph */
    for (cp = pd->callsfrom; cp; cp = cp->nextfrom)
        cp->callsto = (pPD) 0;
}  /* addtooutput */
