/* file regpass2.c */
/* @(#) $Revision: 70.2 $ */
/* KLEENIX_ID @(#)regpass2.c	20.4 92/03/24 */


/*****************************************************************************
 *
 *  File regpass2.c contains routines for replacing memory references in the
 *  statement trees with register references; inserting register loads
 *  and stores; and removing formal argument moves in the prolog.
 *
 *****************************************************************************
 */


#include "c1.h"

LOCAL long currstmtno;
LOCAL long oldnumblocks;

struct between_blocks {
	long block1;
	long block2;
	long newblock;
	};
LOCAL struct between_blocks *added_blocks;
LOCAL long maxaddedblock = 100;
LOCAL long lastaddedblock;

/* procedures declared in this file */
LOCAL long add_block_between_blocks();
LOCAL void add_load_or_store();
LOCAL void add_load_store_between_blocks();
LOCAL void add_load_store_to_preheader();
LOCAL void add_load_store_within_block();
LOCAL flag add_load_store_within_block_1();
LOCAL void add_register_to_masks();
LOCAL void call_walk_2();
LOCAL void fix_register_masks();
      void insert_loads_and_stores();
LOCAL NODE *make_load_store_stmt();
LOCAL long offset_from_ls_subtree();
LOCAL long offset_from_minus_subtree();
LOCAL long offset_from_mul_subtree();
LOCAL long offset_from_plus_subtree();
      long offset_from_subtree();
      void reg_alloc_second_pass();
LOCAL void reg_check_array_form();
LOCAL void reg_check_farg();
LOCAL void reg_check_leaf();
LOCAL void reg_check_plus_array_form();
      void rm_farg_moves_from_prolog();
LOCAL void tree_walk_2();

/*****************************************************************************
 *
 *  ADD_BLOCK_BETWEEN_BLOCKS()
 *
 *  Description:	Create a new block between two blocks.  Update the
 *			predset and succset relationships.  Recalculate
 *			doms[].
 *
 *  Called by:		add_load_store_between_blocks()
 *
 *  Input Parameters:	block1 -- dfo number of first block
 *			block2 -- dfo number of second block
 *
 *  Output Parameters:	return value -- dfo number of added block 
 *
 *  Globals Referenced:	dfo[]
 *			numblocks
 *			predset[]
 *			pseudolbl
 *			succset[]
 *
 *  Globals Modified:	dfo[]
 *			lastblock
 *			numblocks
 *			predset[]
 *			pseudolbl
 *			succset[]
 *
 *  External Calls:	addgoto()
 *			adelement()
 *			cerror()
 *			delelement()
 *			find_doms()
 *			mkblock()
 *			new_set()
 *
 *****************************************************************************
 */
LOCAL long add_block_between_blocks(block1, block2)
register long block1;
register long block2;
{
    register BBLOCK *bp;		/* new block */
    register BBLOCK *bp1;
    register BBLOCK *bp2;
    long onumblocks;
    register long i;
    REGION *rp1;
    REGION *rp2;
    NODE *laststmt;

#ifdef COUNTING
    _n_blocks_between_blocks++;
#endif COUNTING

    bp1 = dfo[block1];
    laststmt = stmttab[bp1->bb.firststmt + bp1->bb.nstmts - 1].np;

    onumblocks = numblocks;
    bp = mkblock();

    /* mkblock() may change the contents of dfo[]. */
    bp1 = dfo[block1];
    bp2 = dfo[block2];
    bp->bb.l.val = pseudolbl;
    bp->bb.l.bp = bp;		/* self-referential */
    bp->bb.l.in = NULL;
    bp->bb.l.out = NULL;
    bp->bb.breakop = LGOTO;
    bp->bb.lbp = bp2;
    addgoto(bp, bp2->bb.l.val);

    /* set register mask in added GOTO */
    bp->bb.treep->nn.intmask = laststmt->nn.intmask;
    bp->bb.treep->nn.f881mask = laststmt->nn.f881mask;
    bp->bb.treep->nn.fpamask = laststmt->nn.fpamask;

    dfo[numblocks] = bp;
    bp->bb.node_position = numblocks;

    delelement(block2, succset[block1], succset[block1]);
    adelement(numblocks, succset[block1], succset[block1]);

    delelement(block1, predset[block2], predset[block2]);
    adelement(numblocks, predset[block2], predset[block2]);

    adelement(block1, predset[numblocks], predset[numblocks]);
    adelement(block2, succset[numblocks], succset[numblocks]);

    /* fix block 1's exits to account for the new block */
    switch(bp1->bb.breakop)
	{
	case CBRANCH:
	    if (bp1->bb.lbp == bp2)
		bp1->bb.lbp = bp;
	    if (bp1->bb.rbp == bp2)
		{
	        register NODE *np;

		bp1->bb.rbp = bp;
		np = bp1->bb.treep;
		np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
		np = np->in.right;
		np->tn.lval = pseudolbl;
		}
	    break;

	case FREE:
	    /* change to a LGOTO */
	    if (bp1->bb.lbp == bp2)
		{
		bp1->bb.breakop = LGOTO;
		bp1->bb.lbp = bp;
		addgoto(bp1, pseudolbl);

		/* set register mask in added GOTO */
		bp1->bb.treep->nn.intmask = laststmt->nn.intmask;
		bp1->bb.treep->nn.f881mask = laststmt->nn.f881mask;
		bp1->bb.treep->nn.fpamask = laststmt->nn.fpamask;
		}
	    else
		cerror("FREE next block incorrect in add_block_between_blocks()");
	    break;

	case LGOTO:	/* everyday GOTO */
	    if (bp1->bb.lbp == bp2)
		{
		register NODE *np;

		bp1->bb.lbp = bp;
		np = bp1->bb.treep;
		np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
		np->bn.label = pseudolbl;
		}
	    else
		cerror("GOTO next block incorrect in add_block_between_blocks()");
	    break;

	case FCOMPGOTO:
	case GOTO:	/* ASSIGNED GOTO */
	    {
	    register CGU *cg;

	    cg = bp1->cg.ll;
	    for (i = bp1->cg.nlabs-1; i >= 0; i--)
		{
		if (cg[i].lp->bp == bp2)
		    {
		    cg[i].lp = &(bp->b.l);
		    cg[i].val = pseudolbl;
		    }
		}
	    }
	    break;

	case EXITBRANCH:
#pragma BBA_IGNORE
	    cerror("EXITBRANCH in add_block_between_blocks()");
	    break;
	}

    /* add the new block to least common region and all regions containing it */
    rp1 = bp1->bb.region;
    rp2 = bp2->bb.region;
    if ((rp1 != rp2) && !xin(rp2->containedin, rp1->regionno))
	{
	if (xin(rp1->containedin, rp2->regionno))
	    {
	    rp1 = rp2;
	    }
	else
	    {
	    while ((rp1 = rp1->parent) && !xin(rp1->blocks, block2))
	        ;
	    }
	}
    adelement(numblocks, rp1->onlyblocks, rp1->onlyblocks);
    do {
        adelement(numblocks, rp1->blocks, rp1->blocks);
	} while (rp1 = rp1->parent);
    

    pseudolbl++;
    numblocks++;
    lastblock++;

    find_doms();

    return(onumblocks);
}  /* add_block_between_blocks */

/*****************************************************************************
 *
 *  ADD_LOAD_OR_STORE()
 *
 *  Description:	Add a single load/store to the statement trees as 
 *			specified.
 *			The "blockno" field is valid only if one of the
 *			statement numbers is zero and it's not a PREHEADER
 *			load/store.
 *
 *  Called by:		insert_loads_and_stores()
 *
 *  Input Parameters:	wb -- web for which load/store is added
 *			lp -- LOADST block specifying where load/store is
 *				to be inserted
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	stmttab[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_load_store_between_blocks()
 *			add_load_store_within_block()
 *			add_load_store_to_preheader()
 *			cerror()
 *
 *****************************************************************************
 */
LOCAL void add_load_or_store(wb, lp)
register WEB *wb;
register LOADST *lp;
{
    long block1;
    long block2;

    if (lp->stmt1 == PREHEADNUM)
	{
	add_load_store_to_preheader(lp->regionno, lp->isload, wb->var,
					wb->regno);
	}
    else
	{
	if (lp->stmt1 == 0)		/* l/s BEFORE stmt */
	    {
	    add_load_store_within_block(lp->blockno, 0, lp->stmt2, lp->isload,
					    wb->var, wb->regno);
	    }
	else if (lp->stmt2 == 0)	/* l/s AFTER stmt */
					/* last stmt of block iff it's safe */
	    {
	    add_load_store_within_block(lp->blockno, lp->stmt1, 0,
					lp->isload, wb->var, wb->regno);
	    }
	else				/* for stmts spec'd */
	    {
	    block1 = stmttab[lp->stmt1].block;
	    block2 = stmttab[lp->stmt2].block;
	    if (block1 == block2)
		{
		if ((lp->stmt1 + 1) != lp->stmt2)
		  /* This block branches back to the beginning of itself.
		   * It is in the "middle" of that branch that a load or
		   * store must be inserted.
		   */
		  add_load_store_between_blocks(block1, block2,
				lp->isload, wb->var, wb->regno);
		else
		add_load_store_within_block(block1, lp->stmt1, lp->stmt2,
					    lp->isload, wb->var, wb->regno);
		}
	    else		/* different blocks */
		{
		add_load_store_between_blocks(block1, block2,
				lp->isload, wb->var, wb->regno);
		}
	    }
	}
}  /* add_load_or_store */ 

/*****************************************************************************
 *
 *  ADD_LOAD_STORE_BETWEEN_BLOCKS()
 *
 *  Description:	Add a load or store between two different blocks.
 *			Create a new block between the two blocks.  Add the
 *			load/store to the new block.  If a new block has
 *			been created before, simply use it without creating
 *			a new block.
 *
 *  Called by:		add_load_or_store()
 *
 *  Input Parameters:	block1 -- block number of first block
 *			block2 -- block number of second block
 *			isload -- YES iff this is a load; NO ==> store
 *			sp     -- pointer to symtab entry for variable
 *			regno  -- register to be loaded/stored
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	dfo[]
 *			predset[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_block_between_blocks()
 *			add_load_store_within_block()
 *			cerror()
 *			nextel()
 *
 *****************************************************************************
 */
LOCAL void add_load_store_between_blocks(block1, block2, isload, sp, regno)
register long block1;
register long block2;
long isload;
HASHU *sp;
long regno;
{
    long newblock;
    register long i;
    register BBLOCK *bp1;
    register BBLOCK *bp2;
    long stmt2 = 0;

#ifdef COUNTING
    _n_loadstores_between_blocks++;
#endif COUNTING
    newblock = -1;
#if 0
/**************************************************************************  
    This is the code that used to add the load/store to block 2 if its the
    one and only predecessor is block 1.  This created a problem, thats why 
    it is no longer active code.  The problem occured when a load/store was
    moved into block 2 by this code and then a different load/store of the
    same variable was found to be needed at the beginning of block 2.  The
    order of these loads and stores is critical and they went out in the
    wrong order.
***************************************************************************/
    i = -1;
    while ((i = nextel(i, predset[block2])) >= 0)
	{
	if (i != block1)
	    goto multi_preds;
	}
    newblock = block2;
    stmt2 = dfo[block2]->bb.firststmt;
    goto add_load_store;
#endif

multi_preds:

    /* If block has already been added between the two blocks, use it */
    for (i = 0; i <= lastaddedblock; ++i)
	{
	if ((added_blocks[i].block1 == block1)
	 && (added_blocks[i].block2 == block2))
	    {
	    newblock = added_blocks[i].newblock;
	    goto add_load_store;
	    }
	}

    /* Go through all successors of block 1.  Add a new block between block1
     * and block 2 iff they are directly linked.
     */

    bp1 = dfo[block1];
    bp2 = dfo[block2];
    switch (bp1->bb.breakop)
	{
	case FCOMPGOTO:
	case GOTO:
	    {
	    register BBLOCK *bp;
	    register CGU *cg;

	    cg = bp1->cg.ll;
	    for (i = bp1->cg.nlabs-1; i >= 0; --i)
		{
		bp = cg[i].lp->bp;
		if (bp == bp2)
		    {
		    newblock = add_block_between_blocks(block1, block2);
		    break;
		    }
	        else if ((bp->bb.node_position >= oldnumblocks)
		      && (bp->bb.breakop == LGOTO)
		      && (bp->bb.lbp == bp2))
		    {
		    newblock = add_block_between_blocks(block1,
							bp->bb.node_position);
		    break;
		    }
		}
	    if (newblock < 0)
		cerror("FCOMPGOTO in add_load_store_between_blocks()");
	    }
	    break;

	case LGOTO:
	    if (bp1->bb.lbp == bp2)
		newblock = add_block_between_blocks(block1, block2);
/************************************************************************
 * This code appears to be unnecessary.  Code above after the multi_pred
 * label should handle the case of a block that has already been inserted.
 * In addition, if this code is necessary (?) it would seem to be incorrect
 * because attempting to add a second block would fail because:
 * bp1->bb.lbp->bb.lbp would not equal bp2.  The first block added would
 * be in the way.
 ************************************************************************/
	    else if ((bp1->bb.lbp->bb.node_position >= oldnumblocks)
	          && (bp1->bb.lbp->bb.breakop == LGOTO)
		  && (bp1->bb.lbp->bb.lbp == bp2))
		newblock = add_block_between_blocks(block1,
						bp1->bb.lbp->bb.node_position);
	    else
		cerror("GOTO in add_load_store_between_blocks()");
	    break;

	case CBRANCH:
	    if (bp1->bb.lbp == bp2)
		newblock = add_block_between_blocks(block1, block2);
/************************************************************************
 * This code appears to be unnecessary.  Code above after the multi_pred
 * label should handle the case of a block that has already been inserted.
 * In addition, if this code is necessary (?) it would seem to be incorrect
 * because attempting to add a second block would fail because:
 * bp1->bb.lbp->bb.lbp would not equal bp2.  The first block added would
 * be in the way.
 ************************************************************************/
	    else if ((bp1->bb.lbp->bb.node_position >= oldnumblocks)
	    	  && (bp1->bb.lbp->bb.breakop == LGOTO)
		  && (bp1->bb.lbp->bb.lbp == bp2))
		newblock = add_block_between_blocks(block1,
						bp1->bb.lbp->bb.node_position);

	    else if (bp1->bb.rbp == bp2)
		newblock = add_block_between_blocks(block1, block2);
/************************************************************************
 * This code appears to be unnecessary.  Code above after the multi_pred
 * label should handle the case of a block that has already been inserted.
 * In addition, if this code is necessary (?) it would seem to be incorrect
 * because attempting to add a second block would fail because:
 * bp1->bb.lbp->bb.lbp would not equal bp2.  The first block added would
 * be in the way.
 ************************************************************************/
	    else if ((bp1->bb.rbp->bb.node_position >= oldnumblocks)
	          && (bp1->bb.rbp->bb.breakop == LGOTO)
		  && (bp1->bb.rbp->bb.lbp == bp2))
		newblock = add_block_between_blocks(block1,
						bp1->bb.rbp->bb.node_position);

	    if (newblock < 0)
		cerror("CBRANCH in add_load_store_between_blocks()");
	    break;

	case FREE:
	    if (bp1->bb.lbp == bp2)
		newblock = add_block_between_blocks(block1, block2);
/************************************************************************
 * This code appears to be unnecessary.  Code above after the multi_pred
 * label should handle the case of a block that has already been inserted.
 * In addition, if this code is necessary (?) it would seem to be incorrect
 * because attempting to add a second block would fail because:
 * bp1->bb.lbp->bb.lbp would not equal bp2.  The first block added would
 * be in the way.
 ************************************************************************/
	    else if ((bp1->bb.lbp->bb.node_position >= oldnumblocks)
	          && (bp1->bb.lbp->bb.breakop == LGOTO)
		  && (bp1->bb.lbp->bb.lbp == bp2))
		newblock = add_block_between_blocks(block1,
						bp1->bb.lbp->bb.node_position);
	    else
		cerror("FREE in add_load_store_between_blocks()");
	    break;

	case EXITBRANCH:
#pragma BBA_IGNORE
	    cerror("has EXITBRANCH in add_load_store_between_blocks()");
	    break;
	}
    if (++lastaddedblock >= maxaddedblock)
	{
	maxaddedblock <<= 1;
	added_blocks = (struct between_blocks *) ckrealloc(added_blocks,
			maxaddedblock * sizeof(struct between_blocks));
	}
    added_blocks[lastaddedblock].block1 = block1;
    added_blocks[lastaddedblock].block2 = block2;
    added_blocks[lastaddedblock].newblock = newblock;

add_load_store:
    add_load_store_within_block(newblock, 0, stmt2, isload, sp, regno);

}  /* add_load_store_between_blocks */

/*****************************************************************************
 *
 *  ADD_LOAD_STORE_TO_PREHEADER()
 *
 *  Description:	Add a register load/store to a possibly new preheader
 *			for the specified region/web.
 *
 *  Called by:		add_load_or_store()
 *
 *  Input Parameters:	regionno -- which region
 *			isload   -- is this a load?
 *			sp	 -- pointer to symtab entry for variable
 *			regno	 -- register number
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastregion
 *			num_to_region[]
 *			preheader_index[]
 *			region
 *
 *  Globals Modified:	currentregion
 *
 *  External Calls:	add_load_store_within_block()
 *			mknewpreheader()
 *			find_doms()
 *			ckalloc()
 *			set_elem_vector()
 *
 *****************************************************************************
 */
LOCAL void add_load_store_to_preheader(regionno, isload, sp, regno)
long regionno;
long isload;
HASHU *sp;
long regno;
{
    long preheadblock;
    register REGION *rp;
    register SET **rpp;
    BBLOCK *preheadbp;
    long *cvector;

#ifdef COUNTING
    _n_loadstores_in_preheaders++;
#endif COUNTING

    rp = num_to_region[regionno];
    cvector = (long *)ckalloc(maxnumblocks * sizeof(long));
    for (rpp = region; rpp <= lastregion; ++rpp)
	{
	if (*rpp == rp->blocks)
	    {
	    preheadblock = preheader_index[rpp - region];
	    if (preheadblock == 0)
		{
		currentregion = rpp;
		set_elem_vector(*rpp, cvector);
		preheadbp = mknewpreheader(cvector);
		preheadblock = preheadbp->bb.node_position;
		if (preheadbp->bb.l.in != preheadbp->bb.l.out)
			FREESET(preheadbp->bb.l.in); /* Only used by global opts */
		FREESET(preheadbp->bb.l.out);

		/* add the new block to its region */
		rp = rp->parent;
		preheadbp->bb.region = rp;
		adelement(preheadblock, rp->onlyblocks, rp->onlyblocks);

		preheadbp->bb.treep->nn.stmtno = 0;
		}
	    else
		{
		preheadbp = dfo[preheadblock];
		}
	    break;	/* get out of loop */
	    }
	}

    FREEIT(cvector);
    add_load_store_within_block(preheadblock, 0, preheadbp->bb.treep->nn.stmtno,
				isload, sp, regno);
}  /* add_load_store_to_preheader */

/*****************************************************************************
 *
 *  ADD_LOAD_STORE_WITHIN_BLOCK()
 *
 *  Description:	Insert a load/store within a basic block
 *
 *  Called by:		add_load_or_store()
 *
 *  Input Parameters:	block1 -- block number containing the load/store
 *			stmt1  -- number of statement immediately preceding
 *					the load or store; 0 implies beginning
					of block
 *			stmt2  -- number of statement immediately following
 *					the load/store; 0 implies end of block
 *			isload -- YES iff this is load, NO ==> store
 *			sp     -- pointer to symbol table entry for variable
 *			regno  -- register to be loaded/stored
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	dfo[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_register_to_masks()
 *			block()
 *			make_load_store_stmt()
 *
 *****************************************************************************
 */
LOCAL void add_load_store_within_block(block1, stmt1, stmt2, isload, sp, regno)
long block1;
long stmt1;
long stmt2;
long isload;
HASHU *sp;
long regno;
{
    register NODE *np;
    NODE *load_store;
    BBLOCK *bp;
    flag add_load;
    flag add_store;
    flag done;

#ifdef COUNTING
    _n_loadstores_within_blocks++;
#endif COUNTING

    load_store = make_load_store_stmt(isload, sp, regno);

    bp = dfo[block1];

    /* find where the load/store is supposed to go */
    add_load = (stmt1 == 0) && isload;
    add_store = (stmt1 == 0) && !isload;

    np = bp->bb.treep;

    if (np == NULL)
	done = NO;
    else
	done = add_load_store_within_block_1(np, load_store, &add_load,
				&add_store, stmt1, stmt2, isload, regno);
    if (!done)
	{
	/* add the load/store to the end of the block */
	if (bp->bb.treep == NULL)
            {
	    bp->bb.treep = np = block(UNARY SEMICOLONOP, load_store, 0, INT);
	    }
	else
	    {
	    bp->bb.treep = np = block(SEMICOLONOP, bp->bb.treep, load_store, INT);
	    }
	np->nn.stmtno = LOADSTORE;      /* special statement # */
	np->in.isload = isload;
	np->in.isfpa881stmt = (fpaflag == FFPA)  
                                && (regno >= F0) && (regno <= F7);
	if (np->in.isfpa881stmt && !saw_dragon_access)
#pragma BBA_IGNORE
	  cerror("Unexpected FPA access");

	if (np->in.op == SEMICOLONOP)
	    {
	    /* copy register masks from preceding item */
	    np->nn.intmask = np->in.left->nn.intmask;
	    np->nn.f881mask = np->in.left->nn.f881mask;
	    np->nn.fpamask = np->in.left->nn.fpamask;
	    add_register_to_masks(regno, &(np->nn.intmask), &(np->nn.f881mask),
					&(np->nn.fpamask));
	    }
	else
	    {
	    np->nn.intmask = 0;
	    np->nn.f881mask = 0;
	    np->nn.fpamask = 0;
	    add_register_to_masks(regno, &(np->nn.intmask), &(np->nn.f881mask),
					&(np->nn.fpamask));
	    }
	}
}  /* add_load_store_within_block */

/*****************************************************************************
 *
 *  ADD_LOAD_STORE_WITHIN_BLOCK_1()
 *
 *  Description:	Recursively descend the statement tree, visiting the
 *			statements in order.  Insert the load/store statement
 *			in the appropriate place.
 *
 *  Called by:		add_load_store_within_block()
 *
 *  Input Parameters:	np -- current SEMICOLONOP or UNARY SEMICOLONOP node
 *			load_store -- statement tree describing load/store
 *				(without topping SEMICOLONOP)
 *			padd_load -- pointer to flag whether to insert load
 *				now.
 *			padd_store -- pointer to flag whether to insert store
 *				now.
 *			stmt1 -- first stmt of statement specifier pair
 *			stmt2 -- second stmt of statement specifier pair
 *			isload -- YES iff this is a load
 *			regno -- register number of load/store
 *
 *  Output Parameters:	padd_load -- flag may be set by this routine
 *			padd_store -- flag may be set by this routine
 *			return value -- YES iff load_store was inserted in tree.
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_load_store_within_block_1()
 *			add_register_to_masks()
 *			block()
 *
 *****************************************************************************
 */
LOCAL flag add_load_store_within_block_1(np, load_store, padd_load, padd_store,
					 stmt1, stmt2, isload, regno)
register NODE *np;
NODE *load_store;
flag *padd_load;
flag *padd_store;
long stmt1;
long stmt2;
long isload;
long regno;
{
    flag done;
    register NODE *next;

    if (np->in.op == SEMICOLONOP)
	done = add_load_store_within_block_1(np->in.left, load_store,
		     padd_load, padd_store, stmt1, stmt2, isload, regno);
    else
	done = NO;

    if (done == YES)
	return(YES);

    if (((np->nn.stmtno == LOADSTORE)
      && (*padd_store || ((np->in.isload) && *padd_load)))
     || (np->nn.stmtno == stmt2)
     || (np->nn.stmtno == 0))	/* new stmt added -- GOTO */
	{

	next = np;

	/* add the load/store */
	if (next->in.op == UNARY SEMICOLONOP)
	    {
	    next->in.op = SEMICOLONOP;
	    next->in.right = next->in.left;
	    np = block(UNARY SEMICOLONOP, load_store, 0, INT);
	    next->in.left = np;
	    }
	else
	    {
	    np = block(SEMICOLONOP, next->in.left, load_store, INT);
	    next->in.left = np;
	    }
	np->nn.stmtno = LOADSTORE;      /* special statement # */
	np->nn.isload = isload;
	np->in.isfpa881stmt = (fpaflag == FFPA)  
                                && (regno >= F0) && (regno <= F7);
	if (np->in.isfpa881stmt && !saw_dragon_access)
#pragma BBA_IGNORE
	  cerror("Unexpected FPA access");

	/* copy register masks from following item */
	np->nn.intmask = next->nn.intmask;
	np->nn.f881mask = next->nn.f881mask;
	np->nn.fpamask = next->nn.fpamask;
	add_register_to_masks(regno, &(np->nn.intmask), &(np->nn.f881mask),
				&(np->nn.fpamask));
	return(YES);
	}
    else if (np->nn.stmtno == stmt1)
	{
	if (isload)
	    *padd_load = YES;
	else
	    *padd_store = YES;
	}
    return(NO);
}  /* add_load_store_within_block_1 */

/*****************************************************************************
 *
 *  ADD_REGISTER_TO_MASKS()
 *
 *  Description:	Add a given register to either the 68020 or FLOAT
 *			masks.
 *
 *  Called by:		add_load_store_within_block()
 *
 *  Input Parameters:	regno	-- register number
 *			pintmask -- 68020 mask (pointer to ushort)
 *			pf881mask-- 68881 mask (pointer to uchar)
 *			pfpamask-- FPA mask (pointer to ushort)
 *
 *  Output Parameters:	pintmask
 *			pf881mask
 *			pfpamask
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */
LOCAL void add_register_to_masks(regno, pintmask, pf881mask, pfpamask)
long regno;
ushort *pintmask;
uchar *pf881mask;
ushort *pfpamask;
{
    long position;
    if (regno < A6)
	{
	position = SP - regno;
	*pintmask |= (1 << position);
	}
    else if (regno <= F7)
	{
	position = F7 - regno;
	*pf881mask |= (1 << position);
	}
    else
	{
	position = FP15 - regno;
	*pfpamask |= (1 << position);
	}
}  /* add_register_to_mask */

/*****************************************************************************
 *
 *  CALL_WALK_2()
 *
 *  Description:	Process a CALL node and arguments.  Replace memory
 *			references with REG nodes where possible in the
 *			argument list.
 *
 *  Called by:		tree_walk_2()
 *
 *  Input Parameters:	np -- pointer to CALL node
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	tree_walk_2
 *
 *****************************************************************************
 */
LOCAL void call_walk_2(np)
register NODE *np;
{
    np = np->in.right;		/* point to top of argument tree */
    while (np)
	{
	if (np->in.op == CM)
	    {
	    tree_walk_2(np->in.right);
	    np = np->in.left;
	    }
	if (np->in.op == UCM)
	    np = np->in.left;
	if (np->in.op != CM)
	    {
	    tree_walk_2(np);
	    np = NULL;
	    }
	}
}  /* call_walk_2 */

/*****************************************************************************
 *
 *  FIX_REGISTER_MASKS()
 *
 *  Description:	Recursively descend the statement tree, creating
 *			composite register masks for the block as we go.
 *			On the way back, set the register masks for each
 *			statement to the composites.
 *
 *  Called by:		insert_loads_and_stores()
 *
 *  Input Parameters:	np	-- current statement (SEMICOLON node)
 *			pintmask -- pointer to composite integer mask
 *			pf881mask -- pointer to composite 68881 mask
 *			pfpamask -- pointer to composite FPA mask
 *
 *  Output Parameters:	pintmask -- composite integer mask changed
 *			pf881mask -- composite 68881 mask changed
 *			pfpamask -- composite FPA mask changed
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	fix_register_masks()
 *
 *****************************************************************************
 */
LOCAL void fix_register_masks(np, pintmask, pf881mask, pfpamask)
NODE *np;
ushort *pintmask;
uchar *pf881mask;
ushort *pfpamask;
{
    *pintmask |= np->nn.intmask;
    *pf881mask |= np->nn.f881mask;
    *pfpamask |= np->nn.fpamask;

    if (np->in.op == SEMICOLONOP)
	fix_register_masks(np->in.left, pintmask, pf881mask, pfpamask);

    np->nn.intmask = *pintmask;
    np->nn.f881mask = *pf881mask;
    np->nn.fpamask = *pfpamask;
}

/*****************************************************************************
 *
 *  INSERT_LOADS_AND_STORES()
 *
 *  Description:	Insert register loads and stores into the statement
 *			trees.
 *
 *  Called by:		register_allocation()  -- in register.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	maxaddedblock
 *			regclasslist
 *
 *  Globals Modified:	added_blocks
 *			lastaddedblock
 *			oldnumblocks
 *
 *  External Calls:	FREEIT()
 *			add_load_or_store()
 *			ckalloc()
 *			discard_web()
 *			fix_register_masks()
 *
 *****************************************************************************
 */
void insert_loads_and_stores()
{
    register long regclass;
    register long item;
    register LOADST *lp;
    register LOADST *lpp;
    register REGCLASSLIST *plist;
    register long lastitem;
    register long i;

    /* record old high-water mark so we can know which blocks are added during
     * this procedure
     */
    oldnumblocks = numblocks;

    /* allocate array for recording new blocks added */
    added_blocks = (struct between_blocks *)
		ckalloc(maxaddedblock * sizeof(struct between_blocks));
    lastaddedblock = -1;

    /* process all colored webs/loops in each register class */
    for (regclass = 0; regclass < NREGCLASSES; ++regclass)
	{
	if (flibflag && ((regclass == F881CLASS) || (regclass == FPACLASS)))
	    continue;
	plist = regclasslist + regclass;
	if (plist->nregs == 0)
	    continue;

	lastitem = plist->ncolored_items;
	for (item = 0; item < lastitem; ++item)
	    {
	    register WEB *wb;
	    if ((wb = plist->colored_items[item]) != NULL)
		{
		lp = wb->load_store;
		while (lp)
		    {
		    lpp = lp->next;
		    add_load_or_store(wb, lp);
		    free_loadst(lp);
		    lp = lpp;
		    }
		wb->load_store = NULL;
		discard_web(wb);
		plist->colored_items[item] = NULL;
		}
	    }
	}

    FREEIT(added_blocks);

    /* goto through all blocks that were added during this procedure and fix
     * the register masks.  Fix them by or'ing together the masks for all
     * statements within the block.  Put the composite masks in each statement
     * in the block.
     */
    for (i = oldnumblocks; i < numblocks; ++i)
	{
	ushort intmask;
	ushort fpamask;
	uchar f881mask;
	register BBLOCK *bp;

	intmask = 0;
	fpamask = 0;
	f881mask = 0;
	bp = dfo[i];
	fix_register_masks(bp->bb.treep, &intmask, &f881mask, &fpamask);
	}
}  /* insert_loads_and_stores */

/*****************************************************************************
 *
 *  MAKE_LOAD_STORE_STMT()
 *
 *  Description:	Create the statement tree for generating the register
 *			load/store.  Return a pointer to the ASSIGN node.
 *
 *  Called by:		add_load_store_within_stmt()
 *
 *  Input Parameters:	isload -- YES iff this is a load; NO ==> store
 *			sp     -- pointer to symtab entry for variable
 *			regno  -- register number
 *
 *  Output Parameters:	return value -- pointer to load/store tree
 *
 *  Globals Referenced:	translate_fargs_flag
 *
 *  Globals Modified:	none
 *
 *  External Calls:	block()
 *			farg_translate()
 *			incref()
 *			talloc()
 *
 *****************************************************************************
 */
LOCAL NODE *make_load_store_stmt(isload, sp, regno)
long isload;
register HASHU *sp;
long regno;
{
    register NODE *var;
    register NODE *reg;
    register NODE *assign;

    /* first make the variable */
    if (sp->an.farg && fortran)
	{
	var = talloc();
	var->in.op = OREG;
	var->tn.rval = A6;
	var->tn.lval = sp->x6n.offset;
	var->tn.type = incref(sp->x6n.type, PTR, 0);
	if ((sp->an.tag != X6N) && (sp->an.tag != S6N))
	    {
	    var = block(UNARY MUL, var, 0, 0, sp->a6n.type);
	    }
	}
    else if ((sp->an.tag == AN) || (sp->an.tag == XN) || (sp->an.tag == SN))
	{
	var = talloc();
	var->in.op = (sp->an.tag == AN) ? NAME : ICON;
	var->atn.name = sp->an.ap;
	var->atn.lval = sp->an.offset;
	var->atn.type = sp->x6n.type;
	if ((sp->an.tag == XN) || (sp->an.tag == SN))
	    var->atn.type = incref(var->atn.type, PTR, 0);
	}
    else if ((sp->an.tag == A6N) || (sp->an.tag == X6N) || (sp->an.tag == S6N))
	{
	var = talloc();
	if (isload && (regno >= A0) && (regno <= A5) && !sp->a6n.ptr)
	    var->in.op = FOREG;
	else
	    var->in.op = OREG;
	var->tn.rval = A6;
	    var->tn.lval = sp->a6n.offset;
	var->atn.type = sp->x6n.type;
	if ((sp->an.tag == X6N) || (sp->an.tag == S6N))
	    var->atn.type = incref(var->atn.type, PTR, 0);
	}

    /* now the register descriptor */
    reg = talloc();
    reg->in.op = REG;
    reg->tn.rval = regno;
    reg->tn.type = var->tn.type;

    /* put it all together */
    if (isload)
	{
	assign = block(ASSIGN, reg, var, 0, reg->tn.type);
#ifdef COUNTING
	_n_loads++;
#endif COUNTING
	}
    else
	{
	assign = block(ASSIGN, var, reg, 0, reg->tn.type);
#ifdef COUNTING
	_n_stores++;
#endif COUNTING
	}

    return(assign);
}  /* make_load_store_stmt */

/*****************************************************************************
 *
 *  OFFSET_FROM_LS_SUBTREE()
 *
 *  Description:	Find constant offset in tree topped with LS operator.
 *			The shift factor has already been verified to be an
 *			integer constant.
 *			Re-write tree by removing constant.
 *
 *  Called by:		offset_from_subtree()
 *
 *  Input Parameters:	subtree -- pointer to node at top of tree
 *
 *  Output Parameters:	pnewsubtree -- modified tree (constant removed)
 *			return value == constant offset
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	offset_from_subtree()
 *
 *****************************************************************************
 */
LOCAL long offset_from_ls_subtree(subtree, pnewsubtree)
register NODE *subtree;
NODE **pnewsubtree;	
{
    register long offset;
    NODE *lsubtree;

    offset = offset_from_subtree(subtree->in.left, &lsubtree);

    if (offset != 0)
	offset <<= subtree->in.right->tn.lval;

    if (lsubtree == NULL)
	{
	subtree->in.right->in.op = FREE;
	subtree->in.op = FREE;
	*pnewsubtree = NULL;
	}
    else
	{
	subtree->in.left = lsubtree;
	*pnewsubtree = subtree;
	}
    return(offset);
}  /* offset_from_ls_subtree */

/*****************************************************************************
 *
 *  OFFSET_FROM_MINUS_SUBTREE()
 *
 *  Description:	Find constant offset in tree topped with MINUS operator.
 *			Re-write tree removing constant.
 *
 *  Called by:		offset_from_subtree()
 *
 *  Input Parameters:	subtree -- pointer to node at top of tree
 *
 *  Output Parameters:	pnewsubtree -- modified tree (constant removed)
 *			return value == constant offset
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	offset_from_subtree()
 *
 *****************************************************************************
 */
LOCAL long offset_from_minus_subtree(subtree, pnewsubtree)
register NODE *subtree;
NODE **pnewsubtree;	
{
    long roffset;
    NODE *rsubtree;
    NODE *lsubtree;

    roffset = - offset_from_subtree(subtree->in.right, &rsubtree);

    if (rsubtree == NULL)
	{
	lsubtree= subtree->in.left;
	subtree->in.op = FREE;
	*pnewsubtree = lsubtree;
	}
    else
	{
	subtree->in.right = rsubtree;
	*pnewsubtree = subtree;
	}
    return(roffset);
}  /* offset_from_minus_subtree */

/*****************************************************************************
 *
 *  OFFSET_FROM_MUL_SUBTREE()
 *
 *  Description:	Find constant offset in tree topped with MUL operator.
 *			Re-write tree by removing constant.
 *
 *  Called by:		offset_from_subtree()
 *
 *  Input Parameters:	subtree -- pointer to node at top of tree
 *
 *  Output Parameters:	pnewsubtree -- modified tree (constant removed)
 *			return value == constant offset
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	offset_from_subtree()
 *
 *****************************************************************************
 */
LOCAL long offset_from_mul_subtree(subtree, pnewsubtree)
register NODE *subtree;
NODE **pnewsubtree;	
{
    register long offset;
    NODE *lsubtree;

    if ((subtree->in.left->in.op == ICON)
     && (subtree->in.left->atn.name == NULL))
	{
        offset = offset_from_subtree(subtree->in.right, &lsubtree);

	if (offset != 0)
	    offset *= subtree->in.left->tn.lval;

	if (lsubtree == NULL)
	    {
	    subtree->in.left->in.op = FREE;
	    subtree->in.op = FREE;
	    *pnewsubtree = NULL;
	    }
        else
	    {
	    subtree->in.right = lsubtree;
	    *pnewsubtree = subtree;
	    }
	}
    else if ((subtree->in.right->in.op == ICON)
     && (subtree->in.right->atn.name == NULL))
	{
        offset = offset_from_subtree(subtree->in.left, &lsubtree);

	if (offset != 0)
	    offset *= subtree->in.right->tn.lval;

	if (lsubtree == NULL)
	    {
	    subtree->in.right->in.op = FREE;
	    subtree->in.op = FREE;
	    *pnewsubtree = NULL;
	    }
        else
	    {
	    subtree->in.left = lsubtree;
	    *pnewsubtree = subtree;
	    }
	}
    else
	{
	offset = 0;
	*pnewsubtree = subtree;
	}
    return(offset);
}  /* offset_from_mul_subtree */

/*****************************************************************************
 *
 *  OFFSET_FROM_PLUS_SUBTREE()
 *
 *  Description:	Find constant offset in tree topped with PLUS operator.
 *			Re-write tree removing constant.
 *
 *  Called by:		offset_from_subtree()
 *
 *  Input Parameters:	subtree -- pointer to node at top of tree
 *
 *  Output Parameters:	pnewsubtree -- modified tree (constant removed)
 *			return value == constant offset
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	offset_from_subtree()
 *
 *****************************************************************************
 */
LOCAL long offset_from_plus_subtree(subtree, pnewsubtree)
register NODE *subtree;
NODE **pnewsubtree;	
{
    long loffset;
    long roffset;
    NODE *lsubtree;
    NODE *rsubtree;

    loffset = offset_from_subtree(subtree->in.left, &lsubtree);
    roffset = offset_from_subtree(subtree->in.right, &rsubtree);

    if (lsubtree == NULL)
	{
	subtree->in.op = FREE;
	if (rsubtree == NULL)
	    {
	    *pnewsubtree = NULL;
	    }
	else
	    {
	    *pnewsubtree = rsubtree;
	    }
	}
    else if (rsubtree == NULL)
	{
	subtree->in.op = FREE;
	*pnewsubtree = lsubtree;
	}
    else
	{
	subtree->in.left = lsubtree;
	subtree->in.right = rsubtree;
	*pnewsubtree = subtree;
	}
    return(loffset + roffset);
}  /* offset_from_plus_subtree */

/*****************************************************************************
 *
 *  OFFSET_FROM_SUBTREE()
 *
 *  Description:	Find the constant offset in a subscript tree.
 *			Re-write the subscript tree by removing the constant
 *			offset.
 *
 *  Called by:		offset_from_ls_subtree()
 *			offset_from_minus_subtree()
 *			offset_from_mul_subtree()
 *			offset_from_plus_subtree()
 *			reg_check_array()
 *
 *  Input Parameters:	subtree	-- subscript tree to be processed
 *			pnewsubtree -- pointer thru which to store address of
 *					modified subscript tree
 *
 *  Output Parameters:	pnewsubtree
 *			return value -- constant offset
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	offset_from_ls_subtree()
 *			offset_from_minus_subtree()
 *			offset_from_mul_subtree()
 *			offset_from_plus_subtree()
 *
 *****************************************************************************
 */

long offset_from_subtree(subtree, pnewsubtree)
NODE *subtree;
NODE **pnewsubtree;
{
    long offset;

    if (subtree->in.op == PLUS)
	{
	offset = offset_from_plus_subtree(subtree, pnewsubtree);
	}
    else if (subtree->in.op == MINUS)
	{
	offset = offset_from_minus_subtree(subtree, pnewsubtree);
	}
    else if (subtree->in.op == MUL)
	{
	offset = offset_from_mul_subtree(subtree, pnewsubtree);
	}
    else if ((subtree->in.op == LS) && (subtree->in.right->in.op == ICON)
	  && (subtree->in.right->atn.name == NULL))
	{
	offset = offset_from_ls_subtree(subtree, pnewsubtree);
	}
    else if ((subtree->in.op == ICON) && (subtree->atn.name == NULL))
	{
	offset = subtree->tn.lval;
	subtree->in.op = FREE;
	*pnewsubtree = NULL;
	}
    else
	{
	offset = 0;
	*pnewsubtree = subtree;
	}

    return(offset);
}  /* offset_from_subtree */

/*****************************************************************************
 *
 *  REG_ALLOC_SECOND_PASS()
 *
 *  Description:	Make the second pass over the statement trees and
 *			replace memory references with register references
 *			for allocated items.  For each statement, create
 *			a register usage mask that p2out can use to emit
 *			SETREGS records where necessary.
 *
 *  Called by:		register_allocation() -- in register.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	regclasslist[]
 *			stmttab[]
 *
 *  Globals Modified:	currstmtno
 *
 *  External Calls:	tree_walk_2()
 *
 *****************************************************************************
 */
void reg_alloc_second_pass()
{
    register long stmt;

    for (stmt = 1; stmt <= laststmtno; ++stmt)
	{
        register ushort mask;
        long regclass;
	ushort ADmask;
	ushort F881mask;
	ushort FPAmask;
	flag infpa881check;
	flag fpa881codeseen = NO;

	currstmtno = stmt;	/* set global for later reference */

	for (regclass = 0; regclass < NREGCLASSES; ++regclass)
	    {
	    register ushort *rp;
	    register ushort *rpend;
	    register REGCLASSLIST *plist;
	    register struct colored_var *cvp;
	    register ushort bit;
	    register long nregs;

	    plist = regclasslist + regclass;
	    nregs = plist->nregs;

	    /* set up register mask data */
	    switch (regclass)
		{
		case INTCLASS:		/* 0 */
				mask = 0;
				bit = 0x100;	/* first reg == D7, skip A's */
				infpa881check = NO;
				break;
		case ADDRCLASS:		/* 1 */
				bit = 0x4;	/* first reg == A5 */
				infpa881check = NO;
				break;
		case F881CLASS:		/* 2 */
				mask = 0;
				bit = 0x1;	/* first reg is highest float */
				infpa881check = ((fpaflag == FFPA)
                                                  && saw_dragon_access);
				break;
		case FPACLASS:		/* 3 */
				mask = 0;
				if (fpaflag == FFPA)
				  bit = 0x1;	/* first reg is highest float */
				else /* BFPA */
				  bit = 0x1 << 8; /* first reg is highest -8 */
				infpa881check = NO;
				break;
		}

	    /* check each regmap entry for this class & this statement */
	    rp = plist->regmap + ((stmt - 1) * nregs);
	    rpend = rp + nregs;
	    for ( ; rp < rpend; ++rp)
		{
		if (*rp)	/* occupied */
		    {
	            register HASHU *sp;

		    /* set info in symtab entry for variable */
		    cvp = &(plist->colored_vars[*rp]);
		    sp = cvp->var;
		    sp->an.stmtno = stmt;
		    sp->an.regno = cvp->regno;

		    /* add this entry to the register mask */
		    mask |= bit;

		    if (infpa881check)
			fpa881codeseen = YES;
		    }
		bit <<= 1;
		}

	    /* remember 68020 mask and 881/FPA mask separately */
	    switch (regclass)
		{
		case INTCLASS:		/* 0 */
				break;
		case ADDRCLASS:		/* 1 */
				ADmask = mask;
				break;
		case F881CLASS:		/* 2 */
				F881mask = mask;
				break;
		case FPACLASS:		/* 3 */
				FPAmask = mask;
				break;
		}
	    }

	{
	register NODE *np;
	np = stmttab[stmt].np;

	/* store register masks in SEMICOLON node */
	np->nn.intmask = ADmask;
	np->nn.f881mask = F881mask;
	np->nn.fpamask = FPAmask;
	if (fpa881codeseen)
	    np->nn.isfpa881stmt = YES;

	/* walk statement tree and replace memory refs with regs */
	if (np->in.op == SEMICOLONOP)
	    tree_walk_2(np->in.right);
	else
	    tree_walk_2(np->in.left);
	}

	}
}  /* reg_alloc_second_pass */

/*****************************************************************************
 *
 *  REG_CHECK_ARRAY_FORM()
 *
 *  Description:	Check an array reference to see if the base address
 *			should be replaced by a register.  Do the replacement.
 *
 *  Called by:		tree_walk_2()
 *
 *  Input Parameters:	np	-- top of array reference tree
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: currstmtno
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *			find()
 *			reg_check_farg_array()
 *			reg_check_stack_array()
 *			reg_check_static_array()
 *
 *****************************************************************************
 */
LOCAL void reg_check_array_form(np)
register NODE *np;
{
    register HASHU *sp;
    long isptrindir;
    long find_loc;

    isptrindir = np->in.isptrindir;
    if (! isptrindir)
	{
	find_loc = np->in.arrayrefno;
	sp = symtab[np->in.arrayrefno];
	}

    if (np->in.op == UNARY MUL)
	{
	np = np->in.left;
	if (np->in.op == PLUS)
	    goto plus;
	else if ((np->in.op == OREG) || (np->in.op == NAME))
	    {
	    if (isptrindir)
		{
		find_loc = find(np);
		if (find_loc != (unsigned) (-1))
		  sp = symtab[find_loc];
		}

	    if ((np->in.op == OREG) && (np->tn.lval >= -baseoff)
	     && !np->in.comma_ss && !np->tn.isc0temp)
		{
	        if ((find_loc != (unsigned) (-1)) &&
		    (sp->x6n.stmtno == currstmtno))
	            {
	            np->in.op = REG;
	            np->tn.rval = sp->x6n.regno;
	            np->tn.lval = 0;
#ifdef COUNTING
		    _n_mem_to_reg_rewrites++;
#endif COUNTING
	            }
		}
	    else
		{
		find_loc = find(np);
		if (find_loc != (unsigned) (-1))
		  sp = symtab[find_loc];
	        if ((find_loc != (unsigned) (-1)) &&
		    (sp->x6n.stmtno == currstmtno))
	            {
	            np->in.op = REG;
	            np->tn.rval = sp->a6n.regno;
	            np->tn.lval = 0;
#ifdef COUNTING
		    _n_mem_to_reg_rewrites++;
#endif COUNTING
	            }
		}
	    }
	else if (((np->in.op == ICON) && (np->atn.name != NULL))
	      || (np->in.op == FOREG))
	    {
	    if (isptrindir)
		cerror("pointer ICON/FOREG in reg_check_array_form()");
	    if ((find_loc != (unsigned) (-1)) &&
		(sp->x6n.stmtno == currstmtno))
	        {
	        np->in.op = REG;
	        np->tn.rval = sp->x6n.regno;
	        np->tn.lval = 0;
#ifdef COUNTING
		_n_mem_to_reg_rewrites++;
#endif COUNTING
	        }
	    }
	else if ((np->in.op != ICON) || (np->atn.name != NULL))
	    cerror("strange-shaped array in reg_check_array_form() - 1");
	}
    else if (np->in.op == PLUS)
	{
plus:
	reg_check_plus_array_form(np, isptrindir, sp);
	}
    else if (np->in.op == OREG)
	{
	if ((np->tn.lval >= -baseoff) && !np->tn.isc0temp)
	    {
	    if (np->in.arrayelem && !sp->x6n.farg)
		if (!fortran)
		  return;
		else
	          sp = symtab[find(np)];
	    if (sp->x6n.stmtno == currstmtno)
	        {
	        np->in.op = REG;
	        np->tn.rval = sp->x6n.regno;
	        np->tn.lval = 0;
#ifdef COUNTING
		_n_mem_to_reg_rewrites++;
#endif COUNTING
	        }
	    }
        else
	    {
	    /* Don't do any register substitution of C array elements.
	     * This avoids problems associated with constant array
	     * elements outside the bounds of the array.
	     */
	    if (!fortran && np->in.arrayelem)
	      return;
	    sp = symtab[find(np)];
	    if (sp->x6n.stmtno == currstmtno)
	        {
	        np->in.op = REG;
	        np->tn.rval = sp->a6n.regno;
	        np->tn.lval = 0;
#ifdef COUNTING
		_n_mem_to_reg_rewrites++;
#endif COUNTING
	        }
	    }
	}
    else if ((np->in.op == NAME) || (np->in.op == FOREG) || (np->in.op == ICON))
	{
	if (np->in.arrayelem || isptrindir || (np->tn.lval != sp->an.offset))
	    {
	    /* Don't do any register substitution of C array elements.
	     * This avoids problems associated with constant array
	     * elements outside the bounds of the array.
	     */
	    if (!fortran && np->in.arrayelem)
	      return;
	    sp = symtab[find(np)];
	    if (sp->an.stmtno == currstmtno)
		{
		np->in.op = REG;
		np->tn.rval = sp->an.regno;
		np->tn.lval = 0;
#ifdef COUNTING
		_n_mem_to_reg_rewrites++;
#endif COUNTING
		}
	    }
	else			/* whole array or farg array ref */
	    {
	    if (sp->an.stmtno == currstmtno)
		{
		np->in.op = REG;
		np->tn.rval = sp->an.regno;
		np->tn.lval = 0;
#ifdef COUNTING
		_n_mem_to_reg_rewrites++;
#endif COUNTING
		}
	    }
	}
    else
#pragma BBA_IGNORE
	cerror("strange-shaped array in reg_check_array() - 2");
}  /* reg_check_array_form */

/*****************************************************************************
 *
 *  REG_CHECK_FARG()
 *
 *  Description:	Check a UNARY* -> OREG tree for replacing memory refs
 *			with registers.  Do the replacement, if warranted.
 *			Translate OREG's for farg, if necessary.
 *
 *  Called by:		tree_walk_2()
 *
 *  Input Parameters:	np	-- top of tree to be checked
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	currstmtno
 *			farg_high
 *			farg_low
 *			symtab
 *			translate_fargs_flag
 *
 *  Globals Modified:	none
 *
 *  External Calls:	farg_translate()
 *			find()
 *
 *****************************************************************************
 */
LOCAL void reg_check_farg(np)
register NODE *np;
{
    register HASHU *sp;
    register NODE *l;
    long find_loc;

    l = np->in.left;
    if ((find_loc = find(np->in.left)) == (unsigned) (-1))
      return;
    
    sp = symtab[find_loc];
    if (sp->a6n.farg)
	{
	if (sp->a6n.stmtno == currstmtno)
	    {
	    tfree(l);
	    np->in.op = REG;
	    np->tn.rval = sp->a6n.regno;
	    np->tn.lval = 0;
#ifdef COUNTING
	    _n_mem_to_reg_rewrites++;
#endif COUNTING
	    }
	}
   else		/* not an farg -- process OREG separately */
	{
	if (sp->a6n.stmtno == currstmtno)
	    {
	    l->in.op == REG;
	    l->tn.rval = sp->a6n.regno;
	    l->tn.lval = 0;
#ifdef COUNTING
	    _n_mem_to_reg_rewrites++;
#endif COUNTING
	    }
	}
}  /* reg_check_farg */

/*****************************************************************************
 *
 *  REG_CHECK_LEAF()
 *
 *  Description:	Check a leaf node reference to see if it should be
 *			changed to a register ref.  Perform the change.
 *			Translate farg OREG's if necessary.
 *
 *  Called by:		tree_walk_2()
 *
 *  Input Parameters:	np 	-- leaf node
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	currstmtno
 *			farg_high
 *			farg_low
 *			translate_fargs_flag
 *
 *  Globals Modified:	none
 *
 *  External Calls:	farg_translate()
 *			find()
 *
 *****************************************************************************
 */
LOCAL void reg_check_leaf(np)
register NODE *np;
{
    register HASHU *sp;
    long find_loc;

    if ((find_loc = find(np)) == (unsigned) (-1))
      return;
    sp = symtab[find_loc];

    /* kludge for mixed automatic arrays and adjustable arrays w/ ENTRY points*/
    if (fortran && (np->in.op == OREG) && (sp->a6n.wholearrayno != NO_ARRAY)
      && symtab[sp->a6n.wholearrayno]->a6n.farg)
        sp = symtab[sp->a6n.wholearrayno];

    if (sp->an.stmtno == currstmtno)
	{
	np->in.op = REG;
	np->tn.rval = sp->an.regno;
	np->tn.lval = 0;
#ifdef COUNTING
	_n_mem_to_reg_rewrites++;
#endif COUNTING
	}
}  /* reg_check_leaf */

/*****************************************************************************
 *
 *  REG_CHECK_PLUS_ARRAY_FORM()
 *
 *  Description:	Check a stack array reference to see if the base
 *			address	should be replaced by a register.  Do the
 *			replacement.
 *
 *  Called by:		reg_check_array()
 *
 *  Input Parameters:	np	-- top PLUS node in array reference tree 
 *			sp	-- symtab entry pointer
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	currstmtno
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *			offset_from_subtree()
 *			tfree()
 *			tree_walk_2()
 *
 *****************************************************************************
 */
LOCAL void reg_check_plus_array_form(topnode, isptrindir, sp)
HASHU *sp;
register NODE *topnode;
long isptrindir;
{
    register NODE *basenode;
    register NODE *np;
    register NODE *constnode;
    NODE *subtree;

    np = topnode;

    if (np->in.left->in.op == PLUS)
	{
	constnode = np->in.right;
	if ((constnode->in.op != ICON) || (constnode->atn.name))
#pragma BBA_IGNORE
	    cerror("strange-shaped array form in reg_check_plus_array_form() - 1");
	np = np->in.left;
	}
    else
	constnode = NULL;

    if (np->in.op != PLUS)
#pragma BBA_IGNORE
        cerror("strange-shaped array form in reg_check_plus_array_form() - 2");

    subtree = np->in.right;
    basenode = np->in.left;

    if ((isptrindir && (basenode->in.op != ICON))
     || ((basenode->in.op == OREG)
	&& ((basenode->tn.lval < -baseoff) || basenode->tn.isc0temp)))
	sp = symtab[find(basenode)];

    if (!(isptrindir && (basenode->in.op == ICON)) &&
       (sp->x6n.stmtno == currstmtno))
	{

	basenode->in.op = REG;
	basenode->tn.lval = 0;
	basenode->tn.rval = sp->a6n.regno;
#ifdef COUNTING
	_n_mem_to_reg_rewrites++;
#endif COUNTING

	if (constnode != NULL)
	    {
	    topnode->in.right = np;
	    np->in.right = constnode;
	    np->in.left = subtree;
	    topnode->in.left = basenode;
	    }
	}
    else	/* base not in register */
	{
	if (constnode != NULL)
	    {
	    if (basenode->in.op == FOREG)
		{
		constnode->tn.lval += basenode->tn.lval;
		basenode->tn.lval = 0;
		basenode->in.op = REG;	/* %a6, of course */
		topnode->in.right = np;
		np->in.right = constnode;
		np->in.left = subtree;
		topnode->in.left = basenode;
		}
	    else if (basenode->in.op == ICON)
		{
		basenode->tn.lval += constnode->tn.lval;
		constnode->in.op = FREE;
		constnode = NULL;
		np->in.op = FREE;
		topnode->in.left = basenode;
		topnode->in.right = subtree;
		}
	    else
		{
		topnode->in.right = np;
		np->in.right = constnode;
		np->in.left = subtree;
		topnode->in.left = basenode;
		}
	    }
	else	/* constnode == NULL */
	    {
	    if (basenode->in.op == FOREG)
		{
		/* re-write to REG-ICON form */
		constnode = bcon(basenode->tn.lval, INT);
		basenode->in.op = REG;
		basenode->tn.lval = 0;
		topnode->in.right = block(PLUS, subtree, constnode, INT, 0);
		}
	    }
	}

    tree_walk_2(subtree);
}  /* reg_check_plus_array_form */

/*****************************************************************************
 *
 *  RM_FARG_MOVES_FROM_PROLOG()
 *
 *  Description:	Remove statement trees which copy formal argument
 *			addresses from calling routine space to local
 *			variable space.  Always free data structures
 *			which statements do the copies (whether the statements
 *			are removed or not).
 *
 *  Called by:		register_allocation()  -- in register.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: dfo[]
 *			fargtab[]
 *			hiddenfargchain
 *			lastfarg
 *			stmttab[]
 *			symtab
 *			translate_fargs_flag
 *
 *  Globals Modified:	hiddenfargchain -- set to -1 (empty)
 *
 *  External Calls:	delete_stmt()
 *			free_plink()
 *
 *****************************************************************************
 */
void rm_farg_moves_from_prolog()
{
    register long i;
    register PLINK *pp;
    register PLINK *ppp;
    HASHU *sp;

	{
	for (i = 0; i <= lastfarg; ++i)
	    {
	    sp = symtab[fargtab[i]];
	    if (pp = ((PLINK *) (sp->a6n.cv)))
	        {
	        while (pp)
		    {
		    ppp = pp->next;
		    free_plink(pp);
		    pp = ppp;
		    }
	        sp->a6n.cv = NULL;
	        }
	    }
	while (hiddenfargchain > 0)
	    {
	    sp = symtab[hiddenfargchain];
	    hiddenfargchain = sp->a6n.nexthiddenfarg;
	    if (pp = ((PLINK *) (sp->a6n.cv)))
	        {
	        while (pp)
		    {
		    ppp = pp->next;
		    free_plink(pp);
		    pp = ppp;
		    }
	        sp->a6n.cv = NULL;
	        }
	    }
	}
}  /* rm_farg_moves_from_prolog */

/*****************************************************************************
 *
 *  TREE_WALK_2()
 *
 *  Description:	Walk the given input tree, replacing memory refs
 *			with register refs, if necessary
 *
 *  Called by:		reg_alloc_second_pass()
 *
 *  Input Parameters:	np -- top of tree to be walked
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	optype()
 *			reg_check_array()
 *			reg_check_farg()
 *			reg_check_leaf()
 *			tree_walk_2()
 *
 *****************************************************************************
 */
LOCAL void tree_walk_2(np)
register NODE *np;
{

    if (np->in.arrayref || np->in.arraybaseaddr || np->in.isarrayform)
	{
	reg_check_array_form(np);
	return;
	}

    switch (np->in.op)
	{
	case ICON:
	case FOREG:	/* don't replace address uses of val-type vars */
#if 0
		    if (np->atn.name)
			reg_check_leaf(np);
#endif
		    /* else ignore */
		    break;

	case OREG:
	case NAME:
		    reg_check_leaf(np);
		    break;

	case UNARY MUL:
# ifdef FTN_POINTERS
		    {
		    int symloc;

		    if ((np->in.left->in.op == OREG) && fortran)
		        {
			symloc = find(np->in.left);
			if (symtab[symloc]->a6n.farg)
			  reg_check_farg(np);
			else
			  tree_walk_2(np->in.left);
			}
		    else
			tree_walk_2(np->in.left);
		    break;
		    }
# else
		    if ((np->in.left->in.op == OREG) && fortran)
			reg_check_farg(np);
		    else
			tree_walk_2(np->in.left);
		    break;
# endif FTN_POINTERS

	case CALL:
	case STCALL:
		    call_walk_2(np);
		    tree_walk_2(np->in.left);  /* call thru dummy arg ?? */
		    break;

	default:
		    switch (optype(np->in.op))
			{
			case LTYPE:
				    break;	/* do nothing */

			case UTYPE:
				    tree_walk_2(np->in.left);
				    break;

			case BITYPE:
				    tree_walk_2(np->in.left);
				    tree_walk_2(np->in.right);
				    break;
			}
		    break;
	}
}  /* tree_walk_2 */
