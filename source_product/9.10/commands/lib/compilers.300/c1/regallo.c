/* file regallo.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)regallo.c	16.1 90/11/05 */


/*****************************************************************************
 *
 *  File regallo.c contains routines for allocating registers to the most
 *  promising webs and loops.
 *
 *****************************************************************************
 */


#include "c1.h"


/* Global variables used in this file */
LOCAL SET *newavailregs;
LOCAL SET *newifvarset;
LOCAL SET *newstmtset;
LOCAL SET *stmttestset;
LOCAL long *awsts_block_vector;
LOCAL long *mr_stmt_vector;

/* Routines defined in this file */

LOCAL void add_webloop_stmts_to_set();
LOCAL void allocate_reg_class();
      void allocate_regs();
LOCAL void color_web();
LOCAL void make_regmap();
LOCAL void prune_registers();
LOCAL void rewrite_dbra();

/*****************************************************************************
 *
 *  ADD_WEBLOOP_STMTS_TO_SET()
 *
 *  Description:	Add all statements of a web/loop to the statement
 *			set.
 *
 *  Called by:		color_web()
 *
 *  Input Parameters:	wb	-- web/loop with statements to be added
 *			stmtset -- statement set
 *
 *  Output Parameters:	stmtset -- statement ranges added
 *
 *  Globals Referenced:	dfo[]
 *			num_to_region[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	addsetrange()
 *
 *****************************************************************************
 */

LOCAL void add_webloop_stmts_to_set(wb, stmtset)
WEB *wb;
register SET *stmtset;
{
    SET *blockset;
    register long *pblockno;
    register long firststmt;
    register long laststmt;
    register BBLOCK *bp;
    flag ownblockset = NO;

    /* Find the set of blocks to add */
    if (wb->isweb)
	{
	if (wb->isinterblock)
	    {
	    blockset = wb->webloop.inter.live_range;
	    }
	else	/* intrablock web */
	    {
	    addsetrange(wb->webloop.intra.first_stmt,
			wb->webloop.intra.last_stmt, stmtset);
	    return;
	    }
	}
    else	/* is loop */
	{
	REGION *rp;

	rp = num_to_region[wb->webloop.loop.regionno];
	if (fpa881loopsexist && !rp->isFPA881loop && rp->child
	 && ((wb->var->an.register_type == REG_FPA_FLOAT_SAVINGS)
	  || (wb->var->an.register_type == REG_FPA_DOUBLE_SAVINGS)))
	    {
	    blockset = new_set(maxnumblocks);
	    setassign(rp->blocks, blockset);
	    difference(fpa881loopblocks, blockset, blockset);
	    ownblockset = YES;
	    }
	else
	    blockset = rp->blocks;
	}

    /* Add the statement ranges in the blocks to the statement set */
    laststmt = -10;
    set_elem_vector(blockset, awsts_block_vector);
    pblockno = awsts_block_vector;
    while (*pblockno >= 0)
	{
	bp = dfo[*(pblockno++)];
	if (bp->bb.nstmts <= 0)
	    continue;

	/* accumulate maximum contiguous ranges before calling addsetrange() */
        if (bp->bb.firststmt !=  (laststmt + 1))
	    {
	    if (laststmt >= 0)	/* first time ??? */
		addsetrange(firststmt, laststmt, stmtset);
	    firststmt = bp->bb.firststmt;
	    }
	laststmt = bp->bb.firststmt + bp->bb.nstmts - 1;
	}

    if (laststmt >= 0)	/* first time ??? */
	addsetrange(firststmt, laststmt, stmtset);

    if (ownblockset)
	FREESET(blockset);
    
}  /* add_webloop_stmts_to_set */


/*****************************************************************************
 *
 *  ALLOCATE_REG_CLASS()
 *
 *  Description:	Allocate registers for a given register class.
 *			Input: list of candidate webs and loops.
 *			Create interference graph and select best items to
 *			live in registers.  Assign actual register numbers.
 *			Create register statement map showing which items
 *			occupy which registers -- for every statement in the
 *			procedure.
 *
 *  Called by:		allocate_regs()
 *
 *  Input Parameters:	plist -- pointer to regclasslist structure -- contains
 *				list of candidate webs and loops
 *			highreg -- highest numbered reg available
 *			nregs -- number of registers available
 *			storecost -- cost of save/restore of register
 *
 *  Output Parameters:	plist -- regclasslist structure modifified to include
 *				the colored variables list.
 *				Candidate list deleted.
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *			ckalloc()
 *			color_web()
 *			make_regmap()
 *			new_set()
 *			prune_registers()
 *
 *****************************************************************************
 */
LOCAL void allocate_reg_class(plist, highreg, nregs, storecost)
register REGCLASSLIST *plist;
char highreg;
char nregs;
short storecost;
{
    register WEB *wb;
    register WEB *wbnext;

    /* Allocate a vector for holding the colored items */
    plist->high_regno = highreg;
    plist->nregs = nregs;
    if (nregs == 0)
	return;
    plist->colored_items =
		(WEB **)ckalloc((plist->nlist_items) * sizeof(WEB *));
    plist->ncolored_items = 0;
    plist->colored_vars = (struct colored_var *)
		ckalloc((plist->nlist_items+1) * sizeof(struct colored_var));
    plist->ncolored_vars = 1;		/* 0 is reserved */
    plist->colored_vars[0].var = (HASHU *)NIL;
    plist->storecost = storecost;

    /* Allocate a temp set for use by color_web() */
    newifvarset = new_set(plist->nlist_items + 1);

    /* Add each web to the interference graph, if registers available */
    for (wb = plist->list; wb; wb = wbnext)
	{
	wbnext = wb->next;
	color_web(wb, plist, nregs);
	}

#ifdef COUNTING
    _n_colored_vars += plist->ncolored_vars - 1;
#endif

    /* Free the temp set used by color_web() */
    FREESET(newifvarset);

    /* Prune out allocated registers if savings <= load/store cost */
    prune_registers( plist );

    /* Create the register statement map */
    make_regmap( plist, highreg, nregs );

}  /* allocate_reg_class */

/*****************************************************************************
 *
 *  ALLOCATE_REGS()
 *
 *  Description:	For each register class, construct an interference
 *			graph, select candidates to live in registers, and
 *			assign actual register numbers to each candidate.
 *			Construct a register statements map for each reg
 *			class showing which items occupy which registers
 *			for every statement in the trees.
 *
 *  Called by:		register_allocation() -- in register.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	flibflag
 *			fpaflag
 *			regclasslist[]
 *			reg_types
 *
 *  Globals Modified:	newavailregs
 *			newstmtset
 *			stmttestset
 *
 *  External Calls:	allocate_reg_class()
 *
 *****************************************************************************
 */
void allocate_regs()
{
    long nregs;
    int  n_A_regs;

    /* allocate temporary sets for use by color_web */
    newavailregs = new_set(16);
    newstmtset = new_set(laststmtno + 1);
    stmttestset = new_set(laststmtno + 1);

    /* allocate temporary element vectors */
    awsts_block_vector = (long *) ckalloc((numblocks + 2) * sizeof(long));
    mr_stmt_vector = (long *) ckalloc((laststmtno + 2) * sizeof(long));

    /* Allocate regs and build reg maps */

    /* A registers */
    n_A_regs = reg_types[REGATYPE].nregs;
    if (fpaflag && saw_dragon_access)
      n_A_regs -= 1;
   /* If PIC is specified: Reserve an A register if
    * an access to a global variable has been seen 
    * or if bfpa was specified and a dragon card 
    * operation was seen.
    */
    if (pic_flag && 
        (saw_global_access || 
         ((fpaflag == BFPA) && saw_dragon_access)))
      {
      n_A_regs -= 1;
      saw_global_access = YES; /* This simplifies same test later */
      }

    allocate_reg_class(&(regclasslist[ADDRCLASS]),
		       reg_types[REGATYPE].high_regno,
		       n_A_regs,
			reg_save_restore_cost[REG_A_SAVINGS] );

    /* D registers */
    allocate_reg_class(&(regclasslist[INTCLASS]),
		       reg_types[REGDTYPE].high_regno,
		       reg_types[REGDTYPE].nregs,
			reg_save_restore_cost[REG_D_SAVINGS]);

    /* float registers */
    if ((fpaflag == FFPA) && saw_dragon_access)		/* FPA */
	{
	allocate_reg_class(&(regclasslist[F881CLASS]),
			       reg_types[REG8TYPE].high_regno,
			       (fpa881loopsexist && !flibflag) ?
					reg_types[REG8TYPE].nregs : 0,
				reg_save_restore_cost[REG_881_DOUBLE_SAVINGS]);
	allocate_reg_class(&(regclasslist[FPACLASS]),
			       reg_types[REGFTYPE].high_regno,
			       flibflag ? 0 : reg_types[REGFTYPE].nregs,
				reg_save_restore_cost[REG_FPA_DOUBLE_SAVINGS]);
	}
    else if ((fpaflag == BFPA) && saw_dragon_access)		/* FPA */
	{
	allocate_reg_class(&(regclasslist[F881CLASS]),
			       reg_types[REG8TYPE].high_regno,
			       0,	/* no 881 regs */
				reg_save_restore_cost[REG_881_DOUBLE_SAVINGS]);
	nregs = flibflag ? 0 : (reg_types[REGFTYPE].nregs - 8);
	if (nregs < 0)
	    nregs = 0;
	allocate_reg_class(&(regclasslist[FPACLASS]),
			       reg_types[REGFTYPE].high_regno - 8,
			       nregs,
				reg_save_restore_cost[REG_FPA_DOUBLE_SAVINGS]);
	}
    else			/* 881 */
	{
	allocate_reg_class(&(regclasslist[F881CLASS]),
			       reg_types[REG8TYPE].high_regno,
			       flibflag ? 0 : reg_types[REG8TYPE].nregs,
				reg_save_restore_cost[REG_881_DOUBLE_SAVINGS]);
	allocate_reg_class(&(regclasslist[FPACLASS]),
			       reg_types[REGFTYPE].high_regno,
			       0,	/* no FPA regs */
				reg_save_restore_cost[REG_FPA_DOUBLE_SAVINGS]);
	}

    /* free temporary sets used by color_web */
    FREESET(newavailregs);
    FREESET(newstmtset);
    FREESET(stmttestset);
    FREEIT(awsts_block_vector);
    FREEIT(mr_stmt_vector);
}  /* allocate_regs */

/*****************************************************************************
 *
 *  COLOR_WEB()
 *
 *  Description:	Attempt to color a single web/loop.  Check this var
 *			with previously colored variables.  Try to add
 *			statements from this web/loop to set of statements
 *			already colored for this var.
 *			If enough registers are available, color this item
 *			also.  Else delete all traces of this web/loop.
 *
 *  Called by:		allocate_reg_class()
 *
 *  Input Parameters:	wb -- web/loop being processed
 *			plist -- pointer to structure containing the list
 *				of all previously-colored items
 *			nregs -- number of regs available
 *
 *  Output Parameters:	plist -- wb may be added to colored list
 *				 subsumed webs may be deleted
 *			wb -- web may be discarded by this routine
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	newavailregs
 *			newifvarset
 *			newstmtset
 *			stmttestset
 *
 *  External Calls:	add_webloop_stmts_to_set()
 *			addsetrange()
 *			adelement()
 *			clearset()
 *			delelement()
 *			discard_web()
 *			intersect()
 *			isemptyset()
 *			new_set()
 *			nextel()
 *			setassign()
 *
 *****************************************************************************
 */
LOCAL void color_web(wb, plist, nregs)
WEB *wb;
REGCLASSLIST *plist;
long nregs;
{
    HASHU *sp_var;	/* var's symtab pointer */
    long newcvarno;	/* var's index in colored_vars vector */
    long nsameregs;	/* # of colored, interfering vars in same reg as this
			 * var */
    register long i;
    short newregno;	/* new register number for this var */
    long ifcvarno;	/* same reg interfering var's index in colored_vars */
    long emptyslot;

    /* find var in colored vars list */
    sp_var = wb->var;
    for (newcvarno = 1; newcvarno < plist->ncolored_vars; ++newcvarno)
	{
	if (plist->colored_vars[newcvarno].var == sp_var)
	    {
	    setassign(plist->colored_vars[newcvarno].stmtset, newstmtset);
	    goto found;
	    }
	}

    /* not found */
    plist->colored_vars[newcvarno].var = sp_var;
    plist->colored_vars[newcvarno].regno = -1;	/* no register yet */
    plist->colored_vars[newcvarno].varifset = (SET *)NIL;
    plist->colored_vars[newcvarno].stmtset = (SET *)NIL;
    plist->colored_vars[newcvarno].deleted = NO;
    clearset(newstmtset);	

found:

    /* add all statements in this web/loop to the statement set */
    add_webloop_stmts_to_set(wb, newstmtset);

    clearset(newifvarset);	/* no interfering vars, yet */
    clearset(newavailregs);
    addsetrange(0, nregs-1, newavailregs);
    nsameregs = 0;

    /* find which colored vars interfere with this one */
    for (i = 1; i < plist->ncolored_vars; ++i)
	{
	if (plist->colored_vars[i].var != sp_var)
	    {
	    intersect(newstmtset, plist->colored_vars[i].stmtset, stmttestset);
	    if (!isemptyset(stmttestset))
		{
		adelement(i, newifvarset, newifvarset);
		delelement(plist->colored_vars[i].regno, newavailregs,
							 newavailregs);
		if (plist->colored_vars[i].regno
			== plist->colored_vars[newcvarno].regno)
		    {
		    nsameregs++;
		    ifcvarno = i;
		    }
		}
	    }
	}

    if (plist->colored_vars[newcvarno].regno >= 0)  /* already colored */
	{
	if (nsameregs == 0)	/* current reg is available -- use it */
	    {
	    goto can_color;
	    }
	else    /* interfering vars in current register */
	    {
	    if ((newregno = (short)nextel(-1, newavailregs)) >= 0)
		{		 /* try to move this var to another register */
		plist->colored_vars[newcvarno].regno = newregno;
		goto can_color;
		}
	    else if (nsameregs == 1)	/* no free regs, but only one
					 * interfering var in reg -- try to
					 * move it */
		{
		/* remove regs used by interfering vars from consideration */
	        addsetrange(0, nregs-1, newavailregs);
		i = 0;
		while ((i = nextel(i, plist->colored_vars[ifcvarno].varifset)) > 0)
		    {
		    delelement(plist->colored_vars[i].regno, newavailregs,
							     newavailregs);
		    }
	        /* the current register is not available, either */
		delelement(plist->colored_vars[ifcvarno].regno, newavailregs,
								newavailregs);

		if ((i = nextel(-1, newavailregs)) >= 0)
		    {
		    /* register available -- move var to it */
		    plist->colored_vars[ifcvarno].regno = i;

		    /* this web/loop can now be added -- in same register */
		    goto can_color;
		    }
		else
		    {
		    goto cant_color;	/* couldn't move interfering var */
		    }
		}
	    else  /* more than one interfering var in same reg -- too bad */
		goto cant_color;
	    }
	}
    else	/* not colored yet */
	{
	if ((newregno = (short)nextel(-1, newavailregs)) >= 0)
	    {		 /* register is available */
	    plist->colored_vars[newcvarno].regno = newregno;
	    plist->ncolored_vars++;
	    goto can_color;
	    }
	else
	    {
	    goto cant_color;
	    }
	}

can_color:

#ifdef COUNTING
    _n_colored_webloops++;
#endif

    /* update interfering vars set, stmtset */
    if (plist->colored_vars[newcvarno].varifset == (SET *)NIL)
        plist->colored_vars[newcvarno].varifset = new_set(plist->nlist_items+1);
    setassign(newifvarset, plist->colored_vars[newcvarno].varifset);
    if (plist->colored_vars[newcvarno].stmtset == (SET *)NIL)
        plist->colored_vars[newcvarno].stmtset = new_set(laststmtno + 1);
    setassign(newstmtset, plist->colored_vars[newcvarno].stmtset);

    if (sp_var->an.dbra_index)
	rewrite_dbra(sp_var, newstmtset);

    /* update all neighbors' interfering var sets */
    i = 0;
    while ((i = nextel(i, plist->colored_vars[newcvarno].varifset)) > 0)
	{
	adelement(newcvarno, plist->colored_vars[i].varifset,
			     plist->colored_vars[i].varifset);
	}

    /* find and remove all subsumed loops */
    emptyslot = plist->ncolored_items;
    if ((!wb->isweb) || wb->isinterblock)  /* loop or interblock webs only */
	{
	for (i = 0; i < plist->ncolored_items; ++i)
	    {
	    register WEB *cloop;
	    if ((cloop = plist->colored_items[i]) == (WEB *)NIL)
		{
		emptyslot = i;
		}
	    else if (! cloop->isweb)
		{
		if ((cloop->var == wb->var)
		 && (((wb->isweb) && (cloop->webloop.loop.web == wb))
		  || (! wb->isweb
		   && xin(num_to_region[cloop->webloop.loop.regionno]->containedin,
			 wb->webloop.loop.regionno))))
		    /* if old item is loop associated with new web OR
		     * old item is loop contained in new loop, THEN discard old
		     * item.
		     */
		    {
		    discard_web(cloop);
		    plist->colored_items[i] = (WEB *)NIL;
		    emptyslot = i;

#ifdef COUNTING
		    _n_subsumed_webloops++;
#endif
		    }
		}
	    }
	}

    plist->colored_items[emptyslot] = wb;
    if (emptyslot >= plist->ncolored_items)
	plist->ncolored_items++;
    return;

cant_color:
    discard_web(wb);
    return;
}  /* color_web */

/*****************************************************************************
 *
 *  MAKE_REGMAP()
 *
 *  Description:	Allocate and fill the register statement map for the
 *			given register class.
 *
 *  Called by:		allocate_reg_class()
 *
 *  Input Parameters:	plist -- pointer to structure containing colored items
 *				list
 *			highreg -- highest numbered register in class
 *			nregs -- max # registers allocated in class
 *
 *  Output Parameters:	plist -- structure will have regmap pointer added
 *
 *  Globals Referenced:	dfo[]
 *			num_to_region[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	clralloc()
 *			set_elem_vector()
 *
 *****************************************************************************
 */
LOCAL void make_regmap( plist, highreg, nregs )
REGCLASSLIST *plist;
register long highreg;
register long nregs;
{
    register ushort *regmap;
    register long i;

    /* allocate the regmap */
    regmap = (ushort *) clralloc(laststmtno * nregs * sizeof (ushort));

    /*	Register statement map layout:
     *
     *  Stmt  +-----------------------------------+  Stmt +-------------
     *	 #1   | highreg | . . . | highreg-nregs+1 |   #2  | highreg | . . .
     *	      +-----------------------------------+       +-------------
     */

    plist->regmap = regmap;
    regmap -= nregs;		/* adjust so don't have to subtract 1 from
				 * stmtno during indexing calculation
				 */

    /* fill in the regmap for each colored var */
    for (i = 1; i < plist->ncolored_vars; ++i)
	{
        register struct colored_var *cv;
        register long regoffset;
        register long *curr_stmt;

	cv = &(plist->colored_vars[i]);
	if (cv->deleted == NO)
	    {
	    regoffset = highreg - cv->regno;
	    set_elem_vector(cv->stmtset, mr_stmt_vector);
	    curr_stmt = mr_stmt_vector;
	    while (*curr_stmt > 0)
	        {
	        regmap[*(curr_stmt++) * nregs + regoffset] = i;
	        }
	    }
	}

    /* remove memory-type defuses from the statement map */
    for (i = 0; i < plist->ncolored_items; ++i)
	{
	WEB *wb;

	if ((wb = plist->colored_items[i]) != (WEB *)NIL)
	    {
	    if ((wb->regno = wb->var->an.regno) == 0)	/* pruned register */
		{
		discard_web(wb);
		plist->colored_items[i] = (WEB *)NIL;
		}
	    else
		{
	        if (wb->isweb)
	            {
		    /* Go thru the DU blocks.  If it's a memory-type, remove
 		     * that stmt from the register map
		     */
		    register DUBLOCK *du;
		    register DUBLOCK *duend;
		    duend = wb->webloop.inter.du->next;
		    du = duend;
		    do  {
		        if ((du->ismemory) && (du->stmtno > 0))
			    regmap[du->stmtno * nregs + highreg - wb->regno]= 0;
		        du = du->next;
		        } while (du != duend);
		    }
		}
	    }
	}
}  /* make_regmap */

/*****************************************************************************
 *
 *  PRUNE_REGISTERS()
 *
 *  Description:	Prune register allocations by "deleting" registers
 *			with savings <= load/store costs
 *
 *  Called by:		allocate_reg_class()
 *
 *  Input Parameters:	plist -- reg class descriptor
 *
 *  Output Parameters:	plist -- modified colored vars list
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void prune_registers(plist)
register REGCLASSLIST *plist;
{
    long regsavings[16];	/* vector to accumulate total savings per reg */
    register long i;
    long nregs;
    long highreg;
    long threshold;
    register long regno;

    nregs = plist->nregs;
    highreg = plist->high_regno;

    /* zero out savings vector */
    for (i = 0; i < nregs; ++i)
	{
	regsavings[i] = 0;
	}

    /* set actual register number in symtab entry */
    for (i = 1; i < plist->ncolored_vars; ++i)
	{
        register struct colored_var *cv;

	cv = &(plist->colored_vars[i]);
	cv->var->an.regno = cv->regno = highreg - cv->regno;
	}

    /* accumulate total savings per register */
    for (i = 0; i < plist->ncolored_items; ++i)
	{
	register WEB *wb;

	if ((wb = plist->colored_items[i]) != (WEB *)NIL)
	    {
	    regno = highreg - wb->var->an.regno;
	    regsavings[regno] += wb->tot_savings;
	    if (regsavings[regno] < 0)		/* must have overflowed */
		regsavings[regno] = MAXINT;
	    }
	}

    threshold = (ncalls > 0) ? (plist->storecost << 1) : plist->storecost;

    /* prune vars in registers */
    for (i = 1; i < plist->ncolored_vars; ++i)
	{
        register struct colored_var *cv;

	cv = &(plist->colored_vars[i]);
	if (regsavings[highreg - cv->regno] <= threshold)
	    {
	    cv->deleted = YES;
	    cv->var->an.regno = 0;
#ifdef COUNTING
	    _n_pruned_register_vars++;
#endif
	    }
	}
    
}  /* prune_registers */

/*****************************************************************************
 *
 *  REWRITE_DBRA()
 *
 *  Description:
 *
 *  Called by:
 *
 *  Input Parameters:
 *
 *  Output Parameters:
 *
 *  Globals Referenced:
 *
 *  Globals Modified:
 *
 *  External Calls:
 *
 *****************************************************************************
 */

LOCAL void rewrite_dbra(index_var, stmtset)
HASHU *index_var;
SET *stmtset;
{
    register long i;
    register REGIONINFO *rp;
    register NODE *np;
    register NODE *np1;

    for (i = 0; i <= lastregioninfo; ++i)
	{
	rp = regioninfo + i;

	if ((rp->type == DBRA_LOOP) && (rp->index_var == index_var)
	 && (!rp->dbra_instruction_inserted) && region[i])
	    {

	    /* find the statement # of the test */
	    for (np = dfo[rp->test_block]->bb.treep;
		 (np->in.op == SEMICOLONOP || np->in.op == UNARY SEMICOLONOP);
		 np = np->in.left)
	        {
	        if (np->in.op == SEMICOLONOP)
		    {
		    if (np->in.right == rp->test)
		        goto found;
		    }
	        else if (np->in.op == UNARY SEMICOLONOP)
		    {
		    if (np->in.left == rp->test)
		        goto found;
		    }
	        }
	    cerror("test not found in rewrite_dbra()");
found:
	    if (xin(stmtset, np->nn.stmtno))
	        {
	        /* free the increment tree */
	        np = rp->increment;
	        tfree(np->in.left);
	        tfree(np->in.right);
	        np->in.op = NOCODEOP;

	        /* rewrite the CBRANCH */
	        np = rp->test;
	        np->in.op = DBRA;
	        tfree(np->in.left->in.right);
	        np1 = np->in.left->in.left;
	        *(np->in.left) = *np1;
	        tfree(np1);

	        np->in.index = rp->is_short_dbra;
	        rp->dbra_instruction_inserted = YES;
	        }
	    }
	}
}  /* rewrite_dbra */
