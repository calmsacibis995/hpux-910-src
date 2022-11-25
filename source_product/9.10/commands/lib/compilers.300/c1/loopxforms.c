/* file loopxforms.c */
/* @(#) $Revision: 70.3 $ */
/* KLEENIX_ID @(#)loopxforms.c	20.3 91/08/07 */


/*****************************************************************************
 *
 *  File loopxforms.c contains:
 *
 *****************************************************************************
 */


#include "c1.h"

/* functions defined in this file */

      flag delete_empty_unreached_loops();
      void do_region_transforms();
      void coalesce_blocks();
      void analyze_regions();
LOCAL long well_formed_entrance();
LOCAL void check_index_mods();
LOCAL void analyze_increment_stmt();
LOCAL flag good_boundvar();
LOCAL flag analyze_do_loop();
LOCAL flag analyze_for_loop();
LOCAL flag recognize_until_loop();
LOCAL flag set_nits();
LOCAL void clear_regioninfo();
LOCAL flag delete_empty_do_loop();
LOCAL flag delete_empty_for_loop();
LOCAL flag has_assigns();
LOCAL flag delete_empty_until_loop();
LOCAL flag isemptyloop();
LOCAL void loop_unroll();
LOCAL flag ok_to_unroll_loop();
LOCAL flag ok_unroll_loop_index_var_refs();
LOCAL void replace_index_var();
LOCAL void replace_initial_load();
LOCAL flag reaching_uses_outside_of_block();
LOCAL void update_loop_iterations();
LOCAL void xform_do_loop_to_dbra();
LOCAL void xform_for_loop_to_dbra();
LOCAL void xform_forwhile_loop_to_until();
LOCAL void xform_loop_to_straight_line();

/* Local definitions */

LOCAL SET *analyze_loop_def_tset;
LOCAL HASHU *index_var_sp;
LOCAL NODE *index_var_node;
LOCAL NODE *index_replace;

/******************************************************************************
*
*	found_init()
*
*	Description:		Looks through the preheader of the loop to find
*				the loop variable initialization statement. It
*				returns YES iff if finds it.
*
*	Called by:		analyze_do_loop()
*
*	Input parameters:	loc - a symtab index for the var being searched for
*				np - a NODE ptr to the top of a block's tree
*
*	Output parameters:	locp - a ptr to a NODE ptr locating the pertinent
*					assignment, if any.
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		locate()
*
*******************************************************************************/
LOCAL flag found_init(loc, np, locp)	int loc; NODE *np, **locp;
{
	NODE *rnp;
	HASHU *sp;

	while (np->in.op == SEMICOLONOP)
		{
		rnp = np->in.right;
		if (rnp->in.op == ASSIGN && locate(rnp->in.left, &sp))
			{
			if (loc == sp->an.symtabindex)
				{
				*locp = rnp;
				return(YES);
				}
			}
		np = np->in.left;
		}
	if (np->in.op == UNARY SEMICOLONOP && np->in.left->in.op == ASSIGN
		&& locate(np->in.left->in.left, &sp))
		{
		if  (loc == sp->an.symtabindex) 
			{
			*locp = np->in.left;
			return(YES);
			}
		}
	return(NO);
}

/******************************************************************************
*
*	well_formed_entrance()
*
*	Description:		Checks to see if the loop entrance is the appropriate
*				shape.
*
*	Called by:		qq
*
*	Input parameters:	qq
*
*	Output parameters:	qq
*
*	Globals referenced:	qq
*
*	Globals modified:	qq
*
*	External calls:		qq
*
*******************************************************************************/
LOCAL long well_formed_entrance(currinfo)
	register REGIONINFO *currinfo;
{
	register long preheadnum;
	register long i;

    /* back branch destination block should have exactly two predecessors --
     * (1) the test/increment block and (2) a branch from outside the loop.
     */
    i = -1;
    preheadnum = nextel(i,predset[currinfo->backbranch.destblock]);
    if (preheadnum == currinfo->backbranch.srcblock)
	{
        preheadnum = nextel(preheadnum,predset[currinfo->backbranch.destblock]);
	if (preheadnum == -1)
	    return(-1);
	i = preheadnum;
	}
    else
	{
        i = nextel(preheadnum,predset[currinfo->backbranch.destblock]);
        if (i != currinfo->backbranch.srcblock)
	    return(-1);		/* something wierd -- bail out */
	}
    if ((i = nextel(i, predset[currinfo->backbranch.destblock])) != -1)
	return(-1);		/* more than 2 predecessors -- bail out */

    return(preheadnum);
}

/******************************************************************************
*
*	good_boundvar()
*
*	Description:		qq
*
*	Called by:		qq
*
*	Input parameters:	qq
*
*	Output parameters:	qq
*
*	Globals referenced:	qq
*
*	Globals modified:	qq
*
*	External calls:		qq
*
*******************************************************************************/
LOCAL flag good_boundvar(np)	register NODE *np;
{
long find_loc;

	/* bound variable must be simple -- not STRUCTREF or ARRAYREF */
	if (((np->in.op != NAME) && (np->in.op != OREG))
	 || np->in.structref || np->in.arrayref 
         || ((find_loc = find(np)) == (unsigned) (-1))
	 || (symtab[find_loc]->an.equiv))	/* can't be equiv or volatile */
	    return(NO);
	return(YES);
}

/******************************************************************************
*
*	set_nits
*
*	Description:		Sets number of loop iterations in the currinfo.
*
*	Called by:		qq
*
*	Input parameters:	qq
*
*	Output parameters:	qq
*
*	Globals referenced:	qq
*
*	Globals modified:	qq
*
*	External calls:		qq
*
*******************************************************************************/
LOCAL flag set_nits(boundval, initval, currinfo, compare_op)
	long boundval, initval;
	REGIONINFO *currinfo;
	uchar compare_op;
{
    long increment_value, overflow_check;
    unsigned long init, bound, difference, increment, niterations;

    increment_value = currinfo->increment_value;

    /*
    ** Check for 0 increment
    */
    if (!increment_value) {
	werror("Loop increment of 0 detected");
	return(NO);
    }

    /*
    ** Because we may be dealing with signed or unsigned comparisons,
    ** both initval and boundval are converted to unsigned longs.  If
    ** the comparison is signed, do this by adding MAXINT+1 to each.
    ** In this routine, the relative difference between initval and
    ** boundval is what matters and not their actual values.
    */
    switch (compare_op) {
	case EQ:
	case NE:
	case LE:
	case LT:
	case GE:
	case GT:
	    init = ~0;
	    init >>= 1;
	    init++;
	    bound = init;
	    init += (unsigned long) initval;
	    bound += (unsigned long) boundval;
	    break;
	case ULE:
	case ULT:
	case UGE:
	case UGT:
            init = (unsigned long) initval;
	    bound = (unsigned long) boundval;
	    break;
    }

    /*
    ** Check for special cases
    */
    if (init < bound) {
	switch (compare_op) {
	    case EQ:                         /* for ( i = 1;  i == 2;  i++) */
	    case GT:                         /* for ( i = 1;  i >  2;  i++) */
	    case GE:                         /* for ( i = 1;  i >= 2;  i++) */
	    case UGT:                        /* for (ui = 1; ui >  2;  i++) */
	    case UGE:                        /* for (ui = 1; ui >= 2;  i++) */
		currinfo->niterations = 0;
		return(YES);
	}
	if (increment_value < 0)             /* for ( i = 1;  i <  2;  i--) */
	    return(NO);
    }
    else if (init > bound) {
	switch (compare_op) {
	    case EQ:                         /* for ( i = 2;  i == 1;  i++) */
	    case LT:                         /* for ( i = 2;  i <  1;  i++) */
	    case LE:                         /* for ( i = 2;  i <= 1;  i++) */
	    case ULT:                        /* for (ui = 2; ui <  1; ui++) */
	    case ULE:                        /* for (ui = 2; ui <= 1; ui++) */
		currinfo->niterations = 0;
		return(YES);
	}
	if (increment_value > 0)             /* for ( i = 2;  i >  1;  i++) */
	    return(NO);
    }
    else {
	switch (compare_op) {
	    case NE:                         /* for ( i = 1;  i != 1;  i++) */
	    case LT:                         /* for ( i = 1;  i <  1;  i++) */
	    case GT:                         /* for ( i = 1;  i >  1;  i++) */
	    case ULT:                        /* for (ui = 1; ui <  1; ui++) */
	    case UGT:                        /* for (ui = 1; ui >  1; ui++) */
		currinfo->niterations = 0;
		return(YES);
	    case EQ:                         /* for ( i = 1;  i == 1;  i++) */
		currinfo->niterations = 1;
		return(YES);
	    case LE:
	    case ULE:
                if (increment_value < 0)     /* for ( i = 1;  i <= 1;  i--) */
		    return(NO);
		currinfo->niterations = 1;   /* for ( i = 1;  i <= 1;  i++) */
		return(YES);
	    case GE:
	    case UGE:
                if (increment_value > 0)     /* for ( i = 1;  i >= 1;  i++) */
		    return(NO);
		currinfo->niterations = 1;   /* for ( i = 1;  i >= 1;  i--) */
		return(YES);
	}
    }

    /*
    ** Calculate the number of iterations for the general case.
    */
    difference = bound > init ? bound - init : init - bound;
    increment = increment_value > 0 ? increment_value : -increment_value;
    niterations = 0;
    switch (compare_op) {
	case EQ:  /* can't happen, but just to be safe... */
	case LE:
	case GE:
	case UGE:
	case ULE:
	    /*
	    ** Because the comparison includes equality, we need to
	    ** check to see if the final value will be hit exactly.
	    */
	    if (!(difference % increment))
		niterations = 1;
	    break;
	case NE:
	    /*
	    ** If the final value won't be hit exactly, forget
	    ** trying to calculate the number of iterations.
	    */
	    if (difference % increment)
		return(NO);
	    break;
    }
    niterations += (difference - 1) / increment + 1;

    /*
    ** Avoid possible overflow
    */
    overflow_check = (long) niterations + 1;
    if (overflow_check <= 0)
	return(NO);

    currinfo->niterations = (long) niterations;
    return(YES);
}

/*****************************************************************************
 *
 *  ANALYZE_DO_LOOP()
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
LOCAL flag analyze_do_loop(currregion, currinfo)
SET **currregion;
register REGIONINFO *currinfo;
{
    register BBLOCK *bp;
    register long i;
    long preheadnum;
    register NODE *np;
    register NODE *np1;
    HASHU *hp;
    long bound;
    long init;
    long find_loc;

	if ( (preheadnum = well_formed_entrance(currinfo)) < 0)
		return(NO);

    /* predecessor of back branch source block should be the test stmt */
    bp = dfo[nextel(-1, predset[currinfo->backbranch.srcblock])];
    if (bp->bb.breakop != CBRANCH)
	return(NO);		/* not the test stmt */

    /* get to the CBRANCH statement */
    np = bp->bb.treep;
    np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
    np1 = np->in.left;			/* comparison operator */
    if (optype(np1->in.op) != BITYPE)
	return(NO);
    if ((optype(np1->in.left->in.op) == LTYPE)	/* should be index var */
      && ((np1->in.left->in.op != ICON) || (np1->in.left->atn.name))
      && ((find_loc = find(np1->in.left)) != (unsigned) (-1))
      && (symtab[find_loc] == currinfo->index_var))
	{
        if ((np1->in.right->in.op == ICON) && (np1->in.right->atn.name == NULL))
	    {
	    currinfo->const_bound_value = YES;
	    bound = np1->in.right->atn.lval;
	    }
	}
    else if ((optype(np1->in.right->in.op) == LTYPE)	/* should be index var */
      && ((np1->in.right->in.op != ICON) || (np1->in.right->atn.name))
      && ((find_loc = find(np1->in.right)) != (unsigned) (-1))
      && (symtab[find_loc] == currinfo->index_var))
	{
        if ((np1->in.left->in.op == ICON) && (np1->in.left->atn.name == NULL))
	    {
	    currinfo->const_bound_value = YES;
	    bound = np1->in.left->atn.lval;
	    }
	}
    else
	return(NO);

    currinfo->test = np;
    currinfo->test_block = bp->bb.node_position;

    /* check for the epilog increment */
    bp = bp->bb.lbp;
    np = bp->bb.treep;
    while (np->in.op == SEMICOLONOP)
	np = np->in.left;
    np = np->in.left;
    if ((np->in.op == ASSIGN) && ((np1 = np->in.right)->in.op == PLUS)
     && (optype(np->in.left->in.op) == LTYPE)
     && ((np->in.left->in.op != ICON) || (np->in.left->atn.name != NULL))
     && ((find_loc = find(np->in.left)) != (unsigned) (-1))
     && (symtab[find_loc] == currinfo->index_var))
	{
        if ((np1->in.right->in.op == ICON)
         && (np1->in.right->atn.name == NULL)
         && (optype(np1->in.left->in.op) == LTYPE)
         && ((np1->in.left->in.op != ICON) || (np1->in.left->atn.name != NULL))
         && ((find_loc = find(np1->in.left)) != (unsigned) (-1))
         && (symtab[find_loc] == currinfo->index_var))
	    {
	    currinfo->epilog_increment = np;
            currinfo->epilog_increment_block = bp->bb.node_position;
	    }
	else if ((np1->in.left->in.op == ICON)
         && (np1->in.left->atn.name == NULL)
         && (optype(np1->in.right->in.op) == LTYPE)
         && ((np1->in.right->in.op != ICON) || (np1->in.right->atn.name != NULL))
         && ((find_loc = find(np1->in.right)) != (unsigned) (-1))
         && (symtab[find_loc] == currinfo->index_var))
	    {
	    currinfo->epilog_increment = np;
            currinfo->epilog_increment_block = bp->bb.node_position;
	    }
	}

    /* unwind the prolog */
    if ((preheadnum > 0) && (preheader_index[currregion - region] == preheadnum))
	{
	int prehead_temp = preheadnum;
	preheadnum = nextel(-1, predset[prehead_temp]);
	if (nextel(preheadnum, predset[prehead_temp]) != -1)
	  return(NO);
	}

    /* this prehead block has several forms:
     *	(1) only an LGOTO (constant upper bound and either
     *          (a) $onetrip semantics or (b) constant initial value)
     *  (2) only a CBRANCH comparing the index with the upper bound
     *         (variable initial value and constant upper bound)
     *  (3) an assignment statement loading the upper bound variable
     *		(variable upper bound), followed by either:
     *      (a) LGOTO ($onetrip semantics)
     *      (b) CBRANCH
     */
	
    bp = dfo[preheadnum];

    /* The bound value could be on either side of the comparison */
    find_loc = locate(currinfo->test->in.left->in.left, &hp)?
	    hp->an.symtabindex : -1;
    if (find_loc == currinfo->index_var->an.symtabindex)
	    /* Found test var initial load, not bound var. Try the other
	       side.
	    */
	    {
	    find_loc = locate(currinfo->test->in.left->in.right, &hp)?
		    hp->an.symtabindex : -1;
	    }
    if ( !currinfo->const_bound_value 
	&& !found_init(find_loc, bp->bb.treep, &(currinfo->bound_load)) )
	    return(NO);
	
    currinfo->bound_load_block = preheadnum;

    if (bp->bb.breakop == CBRANCH)
	{
	np = bp->bb.treep;
	currinfo->initial_test = (np->in.op == SEMICOLONOP) ? np->in.right :
					np->in.left;
        currinfo->initial_test_block = preheadnum;
	}
    else if (bp->bb.breakop != LGOTO)
	return(NO);		/* wrong, wrong */

    /* Almost done.  Predecessor of the preheader is the block that
     * initially loads the index variable with the initial value.
     */
    if ( found_init(currinfo->index_var->an.symtabindex, bp->bb.treep,
	&(currinfo->initial_load)) )
	currinfo->initial_load_block = preheadnum;
    else
    	{
    	i = nextel(-1, predset[preheadnum]);
    	if (i == -1)
		return(NO);
    	bp = dfo[i];
    	if ((bp->bb.breakop != LGOTO) || (bp->bb.nstmts == 1))
		return(NO);
    	np = bp->bb.treep->in.left;
    	np1 = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
    	if ((np1->in.op != ASSIGN)
    	|| (optype(np1->in.left->in.op) != LTYPE)
    	|| ((np1->in.left->in.op == ICON) && (np1->in.left->atn.name == NULL))
    	|| ((find_loc = find(np1->in.left)) == (unsigned) (-1))
    	|| (symtab[find_loc] != currinfo->index_var))
		return(NO);
    	currinfo->initial_load = np1;
    	currinfo->initial_load_block = i;
    	}

    if ((currinfo->initial_load->in.right->in.op == ICON)
     && (currinfo->initial_load->in.right->atn.name == NULL))
	{
	currinfo->const_init_value = YES;
	init = currinfo->initial_load->in.right->tn.lval;
	}
    if (currinfo->const_bound_value && currinfo->const_init_value)
	{
	if (!currinfo->increment_value)
		{
		werror("Loop increment of 0 detected");
		return(NO);
		}
	currinfo->niterations = (bound - init) / currinfo->increment_value + 1;
	if (currinfo->niterations <= 0)
		currinfo->niterations = 1;
	}

    currinfo->type = DO_LOOP;

#ifdef COUNTING
    _n_do_loops++;
#endif

    return(YES);
}  /* analyze_do_loop */

/*****************************************************************************
 *
 *  ANALYZE_FOR_LOOP()
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

LOCAL flag analyze_for_loop(currregion, currinfo)
SET **currregion;
register REGIONINFO *currinfo;
{
    register BBLOCK *bp;
    register long i;
    long preheadnum;
    register NODE *np;
    register NODE *np1;
    long bound;
    long init;
    short compare_op;
    NODE *boundvar;
    long boundvarconst;
    NODE *initvar;
    long initvarconst;
    HASHU *boundsp;
    long find_loc;

	if ( (preheadnum = well_formed_entrance(currinfo)) < 0)
		return(NO);

    /* back branch destination block should be the test stmt */
    bp = dfo[currinfo->backbranch.destblock];
    if ((bp->bb.breakop != CBRANCH) || (bp->bb.nstmts != 1)
     || xin(*currregion, bp->bb.rbp->bb.node_position))	/* out branch in region*/
	return(NO);		/* not the test stmt */

    /* get to the CBRANCH statement */
    np = bp->bb.treep;
    np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
    np1 = np->in.left;			/* comparison operator */
    compare_op = np1->in.op;
    if ((optype(compare_op) != BITYPE)
     || (compare_op < EQ) || (compare_op > UGT))    /* not comparison op */
	return(NO);
    if ((optype(np1->in.left->in.op) == LTYPE)	      /* should be index var */
     && ((np1->in.left->in.op != ICON) || (np1->in.left->atn.name != NULL))
     && ((find_loc = find(np1->in.left)) != (unsigned) (-1))
     && (symtab[find_loc] == currinfo->index_var))
	{
        currinfo->test = np;
        currinfo->test_block = bp->bb.node_position;
	np1 = np1->in.right;		/* test value */
	}
    else if ((optype(np1->in.right->in.op) == LTYPE)  /* should be index var */
     && ((np1->in.right->in.op != ICON) || (np1->in.right->atn.name != NULL))
     && ((find_loc = find(np1->in.right)) != (unsigned) (-1))
     && (symtab[find(np1->in.right)] == currinfo->index_var))
	{
        currinfo->test = np;
        currinfo->test_block = bp->bb.node_position;
	/* swap */
	np = np1->in.left;
	np1->in.left = np1->in.right;
	np1->in.right = np;
	np1->in.op = revrel[np1->in.op - EQ];
	np1 = np1->in.right;
	}
    else
	return(NO);
    if ((np1->in.op == ICON) && (np1->atn.name == NULL))
	{
	currinfo->const_bound_value = YES;
	bound = np1->atn.lval;
	boundvar = NULL;
	}
    else if (optype(np1->in.op) == LTYPE)
	{
	boundvar = np1;
	boundvarconst = 0;
	}
    else if ((np1->in.op == PLUS) || (np1->in.op == MINUS))
	{
	if ((optype((boundvar = np1->in.left)->in.op) == LTYPE)
		&& (boundvar->in.op != ICON))
	    {
	    np = np1->in.right;
	    if ((np->in.op == ICON) && (np->atn.name == NULL))
		boundvarconst =
			(np1->in.op == PLUS) ? np->tn.lval : - np->tn.lval;
	    else
		boundvar = NULL;
	    }
	else if ((optype((boundvar = np1->in.right)->in.op) == LTYPE)
		&& (boundvar->in.op != ICON))
	    {
	    if (np1->in.op == MINUS)
		/* e.g. j < 5 - i */
		return(NO);
	    np = np1->in.left;
	    if ((np->in.op==ICON) && (np->atn.name==NULL))
		{
		boundvarconst = np->tn.lval;
		/* SWAP side of a commutative op - canonicalization */
		np1->in.left = np1->in.right;
		np1->in.right = np;
		}
	    else
		boundvar = NULL;
	    }
	}
    else
	boundvar = NULL;
	

    /* check if any index defs reach from inside loop, other than increment */
    intersect(inreach[currinfo->backbranch.srcblock],
		currinfo->index_var->an.defset, analyze_loop_def_tset);
    i = 0;
    while ((i = nextel(i, analyze_loop_def_tset)) > 0)
	{
	if ((definetab[i].stmtno > 0)
	 && xin(*currregion, stmttab[definetab[i].stmtno].block)
	 && (definetab[i].np != currinfo->increment))
	    {
	    currinfo->index_modified_within_loop = YES;
	    break;
	    }
	}

    if (boundvar)
	{
	if ( !good_boundvar(boundvar) )
		return(NO);

        /* check if any bounds defs are inside loop */
	boundsp = symtab[find(boundvar)];
        i = 0;
        while ((i = nextel(i, boundsp->an.defset)) > 0)
	    {
	    if ((definetab[i].stmtno > 0)
	     && xin(*currregion, stmttab[definetab[i].stmtno].block))
	        {
	        currinfo->bound_modified_within_loop = YES;
	        break;
	        }
	    }
	}

    /* unwind the prolog */
    if ((preheadnum > 0) && (preheader_index[currregion - region] == preheadnum))
	{
	int prehead_temp = preheadnum;
	preheadnum = nextel(-1, predset[prehead_temp]);
	if (nextel(preheadnum, predset[prehead_temp]) != -1)
	  return(NO);
	}

    /* this prehead block has several forms:
     *	(1) only a FREE fall through into the loop (no initial assignment)
     *  (2) only an LGOTO to the c1-added preheader (no initial assignment)
     *  (3) possibly several statements preceding either a FREE or an LGOTO
     */
	
    bp = dfo[preheadnum];
    if (((bp->bb.breakop == FREE) && (bp->bb.nstmts >= 1))
     || ((bp->bb.breakop == LGOTO) && (bp->bb.nstmts >= 2)))
	;
    else
	return(NO);		/* no index variable assignment possible */

    /* check if last statement (before LGOTO) is assignment to index var */
    np = bp->bb.treep;
    if (bp->bb.breakop == LGOTO)
	{
	np = np->in.left;
	}
    np1 = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
    if ((np1->in.op != ASSIGN)
     || (optype(np1->in.left->in.op) != LTYPE)
     || nncon(np1->in.left)
     || ((find_loc = find(np1->in.left)) == (unsigned) (-1))
     || (symtab[find_loc] != currinfo->index_var))
	return(NO);
    currinfo->initial_load = np1;
    currinfo->initial_load_block = preheadnum;
    np1 = np1->in.right;
    if (nncon(np1))
	{
	currinfo->const_init_value = YES;
	init = np1->tn.lval;
	initvar = NULL;
	}
    else if (optype(np1->in.op) == LTYPE)
	{
	initvar = np1;
	initvarconst = 0;
	}
    else if ((np1->in.op == PLUS) || (np1->in.op == MINUS))
	{
	np = np1->in.left;
	if ((optype(np->in.op) == LTYPE) && (np->in.op != ICON))
	    {
	    initvar = np;
	    np = np1->in.right;
	    if ((np->in.op == ICON) && (np->atn.name == NULL))
		initvarconst =
			(np1->in.op == PLUS) ? np->tn.lval : - np->tn.lval;
	    else
		initvar = NULL;
	    }
	else if ((optype((np = np1->in.right)->in.op) == LTYPE)
		&& (np->in.op != ICON))
	    {
	    if (np1->in.op == MINUS)
		/* e.g. i = 5 - j */
		return(NO);
	    initvar = np;
	    np = np1->in.left;
	    if ((np->in.op == ICON) && (np->atn.name == NULL))
		{
		initvarconst = np->tn.lval;
		/* SWAP sides of commutative op for canonicalization */
		np1->in.left = np1->in.right;
		np1->in.right = np;
		}
	    else
		initvar = NULL;
	    }
	}
    else
	initvar = NULL;

    /* init variable must be simple -- not STRUCTREF or ARRAYREF */
    if (initvar
     && (((initvar->in.op != NAME) && (initvar->in.op != OREG))
      || initvar->in.structref || initvar->in.arrayref
      || ((find_loc = find(initvar)) == (unsigned) (-1))
      || (symtab[find_loc]->an.equiv)))		/* can't be equiv or volatile */
	return(NO);

	if (currinfo->const_bound_value && currinfo->const_init_value
		&& !currinfo->index_modified_within_loop)
		{
		if ( !set_nits(bound, init, currinfo, compare_op ) )
			return(NO);
		}
    	else if (boundvar && initvar
	     && (find(boundvar) != (unsigned) (-1)) 
	     && (find(boundvar) == find(initvar)))
	{
		if ( !set_nits(boundvarconst, initvarconst, currinfo, compare_op ) )
			return(NO);
	}

    currinfo->type = FOR_LOOP;

#ifdef COUNTING
    _n_for_loops++;
#endif

    return(YES);
}  /* analyze_for_loop */

/*****************************************************************************
 *
 *  ANALYZE_REGIONS()
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
void analyze_regions(callno)
long callno;
{
    SET **currregion;
    register REGIONINFO *currinfo;
    register BBLOCK *bp1;
    register NODE *np;

    /* allocate defs test set used by analyze routines */
    analyze_loop_def_tset = new_set(maxdefine);

    for (currinfo = regioninfo, currregion = region; currregion <= lastregion;
		++currinfo, ++currregion)
	{
	if (callno == 1)
	    clear_regioninfo(currinfo);
	else if (currinfo->type != DBRA_LOOP)
	    clear_regioninfo(currinfo);
	else
	    continue;

	if (*currregion == NULL)
	    continue;

	bp1 = dfo[currinfo->backbranch.srcblock];

	/* what kind of loop is it ??? */
	if ( fortran && (bp1->bb.breakop == FREE) && (bp1->bb.nstmts >= 1) )
	    /* looks like a FORTRAN DO-loop so far */
	    {
	    currinfo->type = DO_LOOP;
	    np = bp1->bb.treep;
	    while(np->in.op == SEMICOLONOP)
		np = np->in.left;
	    np = np->in.left;		/* child of UNARY SEMICOLONOP */
	    currinfo->increment = np;
            currinfo->increment_block = currinfo->backbranch.srcblock;
	    analyze_increment_stmt(np, currinfo, currregion);
	    }
	else if (!fortran && (bp1->bb.breakop == LGOTO) && (bp1->bb.nstmts >= 2))
	    /* looks like a C for-loop so far */
	    {
	    currinfo->type = FOR_LOOP;
	    /* set np to last statement in block before GOTO */
	    np = bp1->bb.treep;
	    np = np->in.left;
	    np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
	    currinfo->increment = np;
            currinfo->increment_block = currinfo->backbranch.srcblock;
	    analyze_increment_stmt(np, currinfo, currregion);
	    }
	else if (!fortran && bp1->bb.breakop == CBRANCH)
	    {
	    /* DO-WHILE loop so far */
	    currinfo->increment_block = currinfo->backbranch.destblock;
	    bp1 = dfo[currinfo->increment_block];
	    np = bp1->bb.treep;
	    if (np)
		{
		np = (np->in.op == SEMICOLONOP)? np->in.right :
			np->in.left;
		currinfo->increment = np;
	    	currinfo->type = UNTIL_LOOP;
	    	analyze_increment_stmt(np, currinfo, currregion);
		}
	    else
	    	currinfo->type = UNKNOWN;
	
	    }
	}
    needregioninfo = NO;

    FREESET(analyze_loop_def_tset);
}  /* analyze_regions */




LOCAL void analyze_increment_stmt(np, currinfo, currregion)
	register NODE *np;
	register REGIONINFO *currinfo;
	SET **currregion;
{
register NODE *nr;
NODE *nt;
long index_type;
long find_loc;

	if ( (np->in.op == ASSIGN) 
		&& ((nr = np->in.right)->in.op == PLUS || nr->in.op == MINUS) )
		{
		/* index variable must be simple -- not STRUCTREF or ARRAYREF */
		if (((np->in.left->in.op != NAME) && (np->in.left->in.op != OREG))
 			|| np->in.left->in.structref || np->in.left->in.arrayref
 			|| ((find_loc = find(np->in.left)) == (unsigned) (-1))
 			|| (symtab[find_loc]->an.equiv)
 			|| (symtab[find_loc]->an.func))
			/* can't be equiv or volatile or func result */
    			{
    			currinfo->type = UNKNOWN;
    			return;
    			}
    
		/* increment must be simple constant */
		if ( nncon(nr->in.right)
			&& (optype(nr->in.left->in.op) == LTYPE)
			&& same(nr->in.left, np->in.left, NO, NO))
			;
		else if ( nncon(nr->in.left)
 			&& (optype(nr->in.right->in.op) == LTYPE)
 			&& same(nr->in.right, np->in.left, NO, NO))
		    {
		    if (nr->in.op == MINUS) /* Can't swap operands on MINUS */
			{
    			currinfo->type = UNKNOWN;
    			return;
			}
		    else
    			{
    			nt = nr->in.right;
    			nr->in.right = nr->in.left;
    			nr->in.left = nt;
    			}
		    }
		else
    			{
    			currinfo->type = UNKNOWN;
    			return;
    			}

		currinfo->increment_value = nr->in.right->tn.lval;
		if (nr->in.op == MINUS)
			currinfo->increment_value = -currinfo->increment_value;
		currinfo->index_var = symtab[find(np->in.left)];
		index_type = currinfo->index_var->an.type.base;
		switch (index_type)
    			{
    			case CHAR:
    			case SHORT:
    			case INT:
    			case LONG:
    			case UCHAR:
    			case USHORT:
    			case UNSIGNED:
    			case ULONG:
				break;	/* OK -- do nothing */
			
    			default:	/* all other types are no good */
				currinfo->type = UNKNOWN;
				return;
			}

		switch (currinfo->type)
			{
			case DO_LOOP:
				if (!analyze_do_loop(currregion, currinfo))
					currinfo->type = UNKNOWN;
				break;
			case FOR_LOOP:
				if (!analyze_for_loop(currregion, currinfo))
					currinfo->type = UNKNOWN;
				break;
			case UNTIL_LOOP:
				if (!recognize_until_loop(currregion, currinfo))
					currinfo->type = UNKNOWN;
				break;
			default:
    				currinfo->type = UNKNOWN;
			}
		}
	else currinfo->type = UNKNOWN;
}

/******************************************************************************
*
*	check_index_mods()
*
*	Description:		Check if any index defs reach from inside loop,
*				other than increment.
*
*	Called by:		qq
*
*	Input parameters:	qq
*
*	Output parameters:	qq
*
*	Globals referenced:	qq
*
*	Globals modified:	qq
*
*	External calls:		qq
*
*******************************************************************************/
LOCAL void check_index_mods(currinfo, currregion)
	REGIONINFO *currinfo;
	SET **currregion;
{
    register long i;

    /* check if any index defs reach from inside loop, other than increment */
    intersect(inreach[currinfo->backbranch.srcblock],
		currinfo->index_var->an.defset, analyze_loop_def_tset);
    i = 0;
    while ((i = nextel(i, analyze_loop_def_tset)) > 0)
	{
	if ((definetab[i].stmtno > 0)
	 && xin(*currregion, stmttab[definetab[i].stmtno].block)
	 && (definetab[i].np != currinfo->increment))
	    {
	    currinfo->index_modified_within_loop = YES;
	    break;
	    }
	}
}




/******************************************************************************
*
*	recognize_until_loop()
*
*	Description:		Checks for a well-formed DO-UNTIL(condition)
*				loop in C.
*
*	Called by:		qq
*
*	Input parameters:	qq
*
*	Output parameters:	qq
*
*	Globals referenced:	qq
*
*	Globals modified:	qq
*
*	External calls:		qq
*
*******************************************************************************/
/* Used for reversing sense of conditional branches */
LOCAL uchar negrel[]  = {NE, EQ, GT, GE, LT, LE, UGT, UGE, ULT, ULE};
LOCAL uchar fnegrel[] = {FNEQ, FEQ, FNGT, FGT, FNGE, FGE, FNLT, FLT, FNLE, FLE};

LOCAL flag recognize_until_loop(currregion, currinfo)
	register REGIONINFO *currinfo;
	SET **currregion;
{
	register NODE *np;
	register NODE *nl, *nr;
	BBLOCK *bp;
	BBLOCK *bpreheadp;
	NODE *boundvar;
	HASHU *hp;
	HASHU *boundsp;
	long preheadnum;
	long find_loc;
	long initval, boundval;
	uchar compareop;

	if ( (preheadnum = well_formed_entrance(currinfo)) < 0)
		return(NO);

	bpreheadp = dfo[preheadnum];

	bp = dfo[currinfo->backbranch.srcblock];
	np = bp->b.treep;
	np = (np->in.op == SEMICOLONOP)? np->in.right : np->in.left;
	currinfo->test = np;	/* Points to the CBRANCH node */
	np = np->in.left;
	if (np->in.op == NOT)
	{
	        if ( (compareop = np->in.left->in.op) >= EQ &&
	             compareop <= UGT)
		{
			/* Do a little canonicalization on the test subtree */
			nl = np->in.left;
			*np = *nl;
			nl->in.op = FREE;
			np->in.op = (np->in.op <= UGT) ?
				    negrel[np->in.op - EQ] :
				    fnegrel[np->in.op - FEQ];
		}
		else if (np->in.left->in.op == OREG)
		{
			/* Do a little canonicalization on the test subtree */
			np->in.op = EQ;
			np->in.right = bcon(0, np->in.left->tn.type.base);
		}
	}

	compareop = np->in.op;	/* Points to the comparison operator node */

	if ( (optype(compareop) != BITYPE) || (compareop < EQ)
		|| (compareop > UGT) )
		return(NO);
	
	nl = np->in.left;
	nr = np->in.right;
	if ( (optype(nl->in.op) == LTYPE)
		&& (!nncon(nl))
		&& ((find_loc = find(nl)) != -1)
		&& (symtab[find_loc] == currinfo->index_var))
		{
		/* Index var is on the left of the comparison */
		if (nncon(nr))
			{
			currinfo->const_bound_value = YES;
			boundval = nr->atn.lval;
			boundvar = NULL;
			}
		else
			{
			find_loc = locate(nr, &hp)?
				hp->an.symtabindex : -1;
			if ( !found_init(find_loc, bpreheadp->bb.treep,
				&(currinfo->bound_load)) )
				return(NO);
			currinfo->bound_load_block = preheadnum;
			boundvar = (optype(nr->in.op) == LTYPE)? nr : NULL;
			}
		}
	else if ( (optype(nr->in.op) == LTYPE)
		&& (!nncon(nr))
		&& ((find_loc = find(nr)) != -1)
		&& (symtab[find_loc] == currinfo->index_var))
		{
		/* Index var is on the right of the comparison ... for now. */
		if (nncon(nl))
			{
			currinfo->const_bound_value = YES;
			boundval = nl->atn.lval;
			boundvar = NULL;
			}
		else
			{
			find_loc = locate(nl, &hp)?
				hp->an.symtabindex : -1;
			if ( !found_init(find_loc, bpreheadp->bb.treep,
				&(currinfo->bound_load)) )
				return(NO);
			currinfo->bound_load_block = preheadnum;
			boundvar = (optype(nl->in.op) == LTYPE)? nl : NULL;
			}

		/* Canonicalize */
		np->in.left = nr;
		nr = np->in.right = nl;
		nl = np->in.left;
		np->in.op = revrel[compareop - EQ];
		}
	else return(NO);

	if (boundvar)
		{
		if ( !good_boundvar(boundvar) )
			return(NO);
        	/* check if any bounds defs are inside loop */
		boundsp = symtab[find(boundvar)];
        	find_loc = 0;
        	while ((find_loc = nextel(find_loc, boundsp->an.defset)) > 0)
			{
			if ((definetab[find_loc].stmtno > 0)
			&& xin(*currregion, stmttab[definetab[find_loc].stmtno].block))
				{
				currinfo->bound_modified_within_loop = YES;
				break;
				}
			}
		}

	currinfo->test_block = bp->bb.node_position;

	if ( found_init(currinfo->index_var->an.symtabindex,
		bpreheadp->bb.treep, &(currinfo->initial_load)) )
		currinfo->initial_load_block = preheadnum;
	else
		{
		/* Maybe it's initted in the block before the preheader */
		find_loc = nextel(-1, predset[preheadnum]);
		if (find_loc == -1)
			return(NO);
		bp = dfo[find_loc];	/* New use */
		if ( !found_init(currinfo->index_var->an.symtabindex,
			bp->bb.treep, &(currinfo->initial_load)) )
			return(NO);	/* Give up */
		currinfo->initial_load_block = find_loc;
		}

	nr = currinfo->initial_load->in.right;
	if (nncon(nr))
		{
		currinfo->const_init_value = YES;
		initval = nr->tn.lval;
		}

	check_index_mods(currinfo, currregion);

	if ( currinfo->const_bound_value && currinfo->const_init_value
		&& !currinfo->index_modified_within_loop
		&& set_nits(boundval, initval, currinfo, compareop) )
			currinfo->niterations -= currinfo->increment_value;
		/* Test-at-the-end loop has last value one increment less */
    	else return(NO);

	return(YES);
}	/* recognize_until_loop */



/*****************************************************************************
 *
 *  CLEAR_REGIONINFO()
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

LOCAL void clear_regioninfo(regioninfo)
    register REGIONINFO *regioninfo;
{
    ushort srcblock;
    ushort destblock;

    srcblock = regioninfo->backbranch.srcblock;
    destblock = regioninfo->backbranch.destblock;
    memset(regioninfo, 0, sizeof(REGIONINFO));
    regioninfo->backbranch.srcblock = srcblock;
    regioninfo->backbranch.destblock = destblock;
}  /* clear_regioninfo */

/*****************************************************************************
 *
 *  COALESCE_BLOCKS()
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

void coalesce_blocks()
{
    register long b;
    register BBLOCK *bp;
    register long s;
    register BBLOCK *sp;
    register long i;
    register NODE *np;
    register NODE *tp;
    ushort *blockmap;
    flag isentry;

    blockmap = (ushort *) ckalloc(numblocks * sizeof(ushort));
    for (b = 0; b < numblocks; ++b)
	{
	blockmap[b] = (ushort)b;
	}

    for (b = 0; b < numblocks; ++b)
	{
	bp = dfo[b];
	isentry = bp->bb.treep && bp->bb.treep->in.entry;

again:
	/* Consider only active blocks ending in FREE or GOTO */
	if (bp->bb.deleted
	 || ((bp->bb.breakop != FREE) && (bp->bb.breakop != LGOTO)))
	    continue;

	/* Only one successor */
	s = nextel(-1, succset[b]);
	if ((s < 0) || (nextel(s, succset[b]) >= 0))
	    continue;

	/* Successor is not an entry point and has only one successor */
	sp = dfo[s];
	if (sp->bb.entries)
	    continue;
	i = nextel(-1, predset[s]);
	if (i != b)
	    continue;
	i = nextel(i, predset[s]);
	if (i >= 0)
	    continue;

	/* Ignore empty blocks */
	if (!bp->bb.treep || !sp->bb.treep)
	    continue;

	/* Don't fold the exit block in */
	if (sp->bb.breakop == EXITBRANCH)
	    continue;

	/* Turn off entry flag */
	tp = bp->bb.treep;
	if (tp)
	    tp->in.entry = NO;

	/* Has the label for the successor block been used in an ASSIGN
	   statement.  If so, it must be kept, do not coalesce. */
        {
	CGU	*tabend, *cp;
	flag	assign_label;

	tabend = &asgtp[ficonlabsz-1];
	assign_label = NO;
	for (cp = tabend ; cp >= asgtp; cp--)
	  if (sp->l.l.val == cp->val)
	    {
	    assign_label = YES;
	    continue;
	    }
	if (assign_label)
	  continue;
	}

	/* Strip off GOTO */
	if (bp->bb.breakop == LGOTO)
	    {
	    np = (tp->in.op == SEMICOLONOP) ? tp->in.right : tp->in.left;
	    tfree(np);
	    np = (tp->in.op == SEMICOLONOP) ? tp->in.left : NULL;
	    tp->in.op = FREE;
	    tp = np;
	    }

	/* concatenate blocks */
	if (tp == NULL)
	    {
	    tp = sp->bb.treep;
	    }
	else
	    {
	    for (np = sp->bb.treep; np->in.op != UNARY SEMICOLONOP;
		 np = np->in.left)
		;
	    np->in.right = np->in.left;
	    np->in.left = tp;
	    np->in.op = SEMICOLONOP;
	    }

	bp->bb.breakop = sp->bb.breakop;

	/* if successor fell through to next block, need to add an LGOTO */
	if ((sp->bb.breakop == FREE) && (sp->bb.lbp != (bp + 1)))
	    {
	    np = block(LGOTO, sp->bb.lbp->bb.l.val, 0, INT, 0);
	    sp->bb.treep = block(SEMICOLONOP, sp->bb.treep, np, INT, 0);
	    bp->bb.breakop = LGOTO;
	    }

	bp->bb.treep = sp->bb.treep;
	sp->bb.treep = NULL;

	/* set new breakop, next block fields */
	switch(bp->bb.breakop)
	    {
	    case FCOMPGOTO:
	    case GOTO:
			bp->cg.nlabs = sp->cg.nlabs;
			bp->cg.ll = sp->cg.ll;
			break;

	    case LGOTO:
	    case FREE:
			bp->bb.lbp = sp->bb.lbp;
			bp->bb.rbp = NULL;
			break;

	    case CBRANCH:
			bp->bb.lbp = sp->bb.lbp;
			bp->bb.rbp = sp->bb.rbp;
			break;

	    case EXITBRANCH:
			break;
	    }

	/* remove old predset[], succset[] relationships */
	delelement(s, succset[b], succset[b]);
	delelement(b, predset[s], predset[s]);

	/* mark successor block as deleted */
	sp->bb.deleted = YES;
	sp->bb.breakop = FREE;
	sp->bb.rbp = NULL;
	sp->bb.lbp = sp + 1;

	/* update new successor, predecessor relationships */
	i = -1;
	while ((i = nextel(i, succset[s])) >= 0)
	    {
	    delelement(s, predset[i], predset[i]);
	    adelement(b, predset[i], predset[i]);
	    delelement(i, succset[s], succset[s]);
	    adelement(i, succset[b], succset[b]);
	    }

	blockmap[s] = (ushort)b;

	/* fix up ENTRY info */
	if (isentry)
	    {
	    PLINK *pp;

	    bp->bb.treep->in.entry = YES;
	    for (pp = bp->bb.entries; pp; pp = pp->next)
		{
		ep[pp->val].b.nodep = bp->bb.treep;
		}
	    }

#ifdef COUNTING
	_n_block_merges++;
#endif

	/* try it again with this same (new) block */
	goto again;

	}

    /* fix up regioninfo block numbers */
    if (! needregioninfo)
	{
	REGIONINFO *rp, *rpend;
	for (rp = regioninfo, rpend = regioninfo + lastregioninfo;
		rp <= rpend; ++rp)
	    {
	    rp->initial_load_block = blockmap[rp->initial_load_block];
	    rp->initial_test_block = blockmap[rp->initial_test_block];
	    rp->bound_load_block = blockmap[rp->bound_load_block];
	    rp->increment_block = blockmap[rp->increment_block];
	    rp->test_block = blockmap[rp->test_block];
	    rp->epilog_increment_block = blockmap[rp->epilog_increment_block];
	    rp->backbranch.srcblock = blockmap[rp->backbranch.srcblock];
	    rp->backbranch.destblock = blockmap[rp->backbranch.destblock];
	    }
	}

    FREEIT(blockmap);
}  /* coalesce_blocks */

/******************************************************************************
*
*	delete_empty_until_loop()
*
*	Description:		Removes a DO-UNTIL(condition) C loop.
*
*	Called by:		qq
*
*	Input parameters:	qq
*
*	Output parameters:	qq
*
*	Globals referenced:	qq
*
*	Globals modified:	qq
*
*	External calls:		qq
*
*******************************************************************************/
LOCAL flag delete_empty_until_loop(regioninfo, curregion)
register REGIONINFO *regioninfo;
SET **curregion;
{
	register int i;
	register SET *iset;
	register NODE *np;
	register BBLOCK *ibp, *obp;
	BBLOCK *bp;
	flag lefthanded;

	ibp = dfo[regioninfo->initial_load_block];
	obp = dfo[regioninfo->test_block];
	lefthanded = (obp->bb.rbp == dfo[regioninfo->backbranch.destblock]);

	if (ibp->bb.deleted || obp->bb.deleted)
		return(NO);

	if (ibp->bb.breakop == FREE)
		{
		/* It cannot have a following preheader block; otherwise
		   the breakop would be an LGOTO.
		*/
		addgoto(ibp, (lefthanded)?
			obp->bb.lbp->bb.l.val : obp->bb.rbp->bb.l.val);
		ibp->bb.breakop = LGOTO;
		}
	if (ibp->bb.breakop != LGOTO)
#pragma BBA_IGNORE
		cerror("strange loop seen in delete_empty_until_loop()");

	obp = lefthanded? obp->bb.lbp : obp->bb.rbp;
	ibp->bb.lbp = obp;	/* The loop exit block */

	/* Update the labels in the LGOTO. */
	np = ibp->bb.treep;
	np = (np->in.op == SEMICOLONOP)? np->in.right : np->in.left;
	np->bn.label = obp->bb.l.val;

	/* Update succsets and predsets for the new path. */
	i = obp->bb.node_position;

	iset = succset[ibp->bb.node_position];
	delelement(regioninfo->backbranch.destblock, iset, iset);
	adelement(i, iset, iset);

	iset = predset[i];
	delelement(regioninfo->test_block, iset, iset);
	adelement(ibp->bb.node_position, iset, iset);

	i = regioninfo->test_block;
	bp = dfo[i];
	bp->bb.breakop = LGOTO;	/* Avoid looking at the rhs for cleanup*/
	if (lefthanded)
		{
		/* Swap to fool cleanup */
		bp->bb.lbp = bp->bb.rbp;
		bp->bb.rbp = obp;
		}

	i = -1;
	while ((i = nextel(i, predset[regioninfo->test_block])) >= 0)
		delelement(regioninfo->test_block, succset[i], succset[i]);
	clearset(predset[regioninfo->test_block]);

	if (reaching_uses_outside_of_block(regioninfo, curregion))
		replace_initial_load(regioninfo);
	else
    		{
		/* delete initial load */
    		np = regioninfo->initial_load;
    		for (i = 1; i <= lastdefine; ++i)
			if (definetab[i].np == np)
	    			{
	    			remove_dead_store(i);
	    			break;
				}
		}

	/* Delete obsolete blocks */
	clearset(predset[regioninfo->backbranch.destblock]);
    	delete_unreached_block(regioninfo->test_block);
    	while ((i = pop_unreached_block_from_stack()) >= 0)
        	delete_unreached_block(i);

#ifdef COUNTING
#	if 0
    	_n_empty_until_loops++;
#	endif 
#endif

	return(YES);
}  /* delete_empty_until_loop */

/*****************************************************************************
 *
 *  DELETE_EMPTY_DO_LOOP()
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

LOCAL flag delete_empty_do_loop(regioninfo)
register REGIONINFO *regioninfo;
{
    register BBLOCK *indexloadbp;
    register SET *iset;
    BBLOCK *preheadbp;
    BBLOCK *testbp;
    register BBLOCK *outbranchbp;
    NODE *np;
    register long i;

    /* Check for volatile index or bound */
    if (regioninfo->epilog_increment 
	&& regioninfo->epilog_increment->in.op != NOCODEOP)
	/* index used after loop */
	return(NO);

    /* Change LGOTO after index initial load to go to final test branch out */
    indexloadbp = dfo[regioninfo->initial_load_block];
    np = indexloadbp->bb.treep;
    np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
    preheadbp = indexloadbp->bb.lbp;
    testbp = dfo[regioninfo->test_block];
    outbranchbp = testbp->bb.lbp;
    np->bn.label = outbranchbp->bb.l.val;
    indexloadbp->bb.lbp = outbranchbp;

    iset = predset[outbranchbp->bb.node_position];
    adelement(indexloadbp->bb.node_position, iset, iset);

    iset = succset[indexloadbp->bb.node_position];
    adelement(outbranchbp->bb.node_position, iset, iset);
    delelement(preheadbp->bb.node_position, iset, iset);

    iset = predset[preheadbp->bb.node_position];
    delelement(indexloadbp->bb.node_position, iset, iset);
    if (!isemptyset(iset))
#pragma BBA_IGNORE
	cerror("nonempty predset in delete_unreached_do_loop()");
    delete_unreached_block(preheadbp->bb.node_position);
    while ((i = pop_unreached_block_from_stack()) >= 0)
        delete_unreached_block(i);

    iset = predset[regioninfo->backbranch.destblock];
    delelement(regioninfo->backbranch.srcblock, iset, iset);

    iset = succset[regioninfo->backbranch.srcblock];
    delelement(regioninfo->backbranch.destblock, iset, iset);

    delete_unreached_block(regioninfo->backbranch.destblock);
    while ((i = pop_unreached_block_from_stack()) >= 0)
        delete_unreached_block(i);

    /* delete initial load */
    np = regioninfo->initial_load;
    for (i = 1; i <= lastdefine; ++i)
	{
	if (definetab[i].np == np)
	    {
	    remove_dead_store(i);
	    break;
	    }
	}

#ifdef COUNTING
    _n_empty_do_loops++;
#endif

    return(YES);
}  /* delete_empty_do_loop */

/******************************************************************************
*
*	replace_initial_load()
*
*	Description:		The initial load statement in a C for-loop
*				sometimes has to be updated so that the index
*				variable has the correct final value. Of course,
*				If the loop is never executed, then leave
*				the initial assignment alone, since it will
*				be deleted eventually anyway.
*
*	Called by:		delete_empty_for_loop()
*				delete_empty_until_loop()
*
*	Input parameters:	currinfo - a struct containing all pertinent
*					info on the region being removed.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		cerror()
*
*******************************************************************************/
LOCAL void replace_initial_load(currinfo) REGIONINFO *currinfo;
{
	register NODE *np;
	register int i;
	register short increment;
	if (currinfo->niterations == 0)
		/* Leave the initial assignment alone */
		return;
	np = currinfo->initial_load->in.right;
	if (np->in.op != ICON)
#pragma BBA_IGNORE
		cerror("Non-constant initial load in replace_initial_load()");
	increment = currinfo->increment_value;
	for (i = currinfo->niterations; i > 0; i--)
		np->tn.lval += increment;
}

/******************************************************************************
*
*	reaching_uses_outside_of_block()
*
*	Description:		searches DU chain to see if the index var is
*				used outside of the current block. If it is,
*				then it will have to be set to the correct value
*				upon loop exit.
*
*	Called by:		delete_empty_for_loop()
*
*	Input parameters:	currinfo - a struct containing pertinent
*					information about the current region.
*				curregion - a struct containing other region
*					information.
*
*	Output parameters:	none
*				(function returns YES iff the region has uses
*				outside of the current region.)
*
*	Globals referenced:	definetab[]
*				stmttab[]
*				inreach[]
*				exitblockno
*
*	Globals modified:	none
*
*	External calls:		cerror()
*				xin()
*
*******************************************************************************/
LOCAL flag reaching_uses_outside_of_block(currinfo, curregion)
SET **curregion;
register REGIONINFO *currinfo;
{
    HASHU *sp;
    register long loaddefno;
    register long i;
    register DUBLOCK *du;
    register DUBLOCK *duend;
    register long xblock;

    sp = currinfo->index_var;

    /* find define #'s for initial load and increment */
    loaddefno = 0;
    for (i = 1; i <= lastdefine; ++i)
	{
	if (definetab[i].np == currinfo->initial_load)
	    {
	    loaddefno = i;
	    break;
	    }
	}
    if (loaddefno == 0)
#pragma BBA_IGNORE
	cerror("define for initial load not found in reaching_uses_outside_of_block()");

    /* Go through all DU blocks for index var.  */
    duend = sp->an.du->next;
    du = duend;
    do {
	xblock = stmttab[du->stmtno].block;
	if (du->deleted)
	    goto nextdu;
	if (!xin(*curregion, xblock)) /* not in region */
	    {
	    if (!du->isdef && (du->defsetno == 0))
						 /* use w/o def in block*/
		{
	        if (du->stmtno > 0)		/* regular stmt */
		    {
	            if (xin(inreach[xblock], loaddefno))	/* load reaches */
			return(YES);
		    }
		else if ((exitblockno >= 0) 
		      && (xin(inreach[exitblockno], loaddefno)))/* exit reaches */
		    	return(YES);
		}
	    }
nextdu:
	du = du->next;
        } while (du != duend);
    return(NO);
}  /* reaching_uses_outside_of_block */

/*****************************************************************************
 *
 *  DELETE_EMPTY_FOR_LOOP()
 *
 *  Description:		Removes blocks associated with an empty
 *				for loop.
 *
 *  Called by:			delete_empty_unreached_loops()
 *
 *  Input Parameters:		regioninfo - a ptr to a REGIONINFO struct
 *				giving all pertinent data about a region
 *				that describes a loop.
 *
 *  Output Parameters:		none
 *
 *  Globals Referenced:		succset[]
 *				predset[]
 *				dfo[]
 *				lastdefine
 *				definetab[]
 *				_n_empty_do_loops
 *
 *  Globals Modified:		succset[]
 *				predset[]
 *				_n_empty_do_loops
 *
 *  External Calls:		addgoto()
 *				cerror()
 *				clearset()
 *				adelement()
 *				delelement()
 *				isemptyset()
 *				delete_unreached_block()
 *				pop_unreached_block_from_stack()
 *				remove_dead_store()
 *				xin()
 *
 *****************************************************************************
 */

LOCAL flag delete_empty_for_loop(regioninfo, curregion)
register REGIONINFO *regioninfo;
SET **curregion;
{
	register int i;
	register SET *iset;
	register NODE *np;
	register BBLOCK *ibp, *obp;
	NODE          *incr_node;
	struct deftab *incr_def;

        /*
	 * find the def for the loop increment
	 */

        incr_node = regioninfo->increment;

	for (incr_def = definetab; incr_def->np != incr_node; incr_def++);

	/*
	 * Only two allowable uses are in
	 * the test and the increment, so
	 * more than two indicates a use
	 * after the loop
	 */
	if (incr_def->nuses > 2)
	{
	    return(NO);
	}

	/*
	 * if the test contains any assignment
	 * operators then punt
	 */
	if (has_assigns(regioninfo->test))
	{
	    return(NO);
	}

	/*
	 * OK to remove the loop
	 */
	ibp = dfo[regioninfo->initial_load_block];
	obp = dfo[regioninfo->test_block];

	if (ibp->bb.deleted || obp->bb.deleted)
		return(NO);

	if (ibp->bb.breakop == FREE)
		{
		/* It cannot have a following preheader block; otherwise
		   the breakop would be an LGOTO.
		*/
		addgoto(ibp, obp->bb.rbp->bb.l.val);
		ibp->bb.breakop = LGOTO;
		}
	if (ibp->bb.breakop != LGOTO)
#pragma BBA_IGNORE
		cerror("strange FOR-loop seen in delete_empty_for_loop()");

	if (!xin(succset[regioninfo->initial_load_block], regioninfo->test_block))
		/* An intervening preheader node has been inserted prior. */
		ibp = ibp->bb.lbp;

	obp = obp->bb.rbp;	/* The loop exit block */
	ibp->bb.lbp = obp;

	/* Update succsets and predsets for the new path. */
	i = obp->bb.node_position;

	iset = succset[ibp->bb.node_position];
	delelement(regioninfo->test_block, iset, iset);
	adelement(i, iset, iset);

	iset = predset[i];
	delelement(regioninfo->test_block, iset, iset);
	adelement(ibp->bb.node_position, iset, iset);

	/* Update the labels in the LGOTO. */
	np = ibp->bb.treep;
	np = (np->in.op == SEMICOLONOP)? np->in.right : np->in.left;
	np->bn.label = obp->bb.l.val;

	i = regioninfo->test_block;
	clearset(predset[i]);
	clearset(succset[regioninfo->increment_block]);

	iset = succset[regioninfo->increment_block];
	delelement(regioninfo->test_block, iset, iset);

	if (reaching_uses_outside_of_block(regioninfo, curregion))
		replace_initial_load(regioninfo);
	else
    		{
		/* delete initial load */
    		np = regioninfo->initial_load;
    		for (i = 1; i <= lastdefine; ++i)
			if (definetab[i].np == np)
	    			{
	    			remove_dead_store(i);
	    			break;
				}
		}

	/* Delete obsolete blocks */
    	delete_unreached_block(regioninfo->test_block);
    	while ((i = pop_unreached_block_from_stack()) >= 0)
        	delete_unreached_block(i);

#ifdef COUNTING
    	_n_empty_do_loops++;
#endif

	return(YES);
}  /* delete_empty_for_loop */

/*****************************************************************************
 *
 *  HAS_ASSIGNS()
 *
 *  Description:		Returns YES if the tree contains any
 *				assignment operators
 *
 *  Called by:			delete_empty_for_loop()
 *
 *  Input Parameters:		np - the tree
 *
 *  Output Parameters:		none
 *
 *  Globals Referenced:		none
 *
 *  Globals Modified:		none
 *
 *  External Calls:		has_assigns()
 *
 *****************************************************************************
 */

LOCAL flag has_assigns(np)
register NODE *np;
{

more:
     if (asgop(np->in.op))
     {
	 return(YES);
     }

     switch(optype(np->in.op))
     {

     case BITYPE:
	     if (has_assigns(np->in.right))
	     {
		 return(YES);
	     }
	     /*
	      * fall thru to handle left subtree
	      */

     case UTYPE:
             np = np->in.left;
             goto more;

     default:
             return(NO);
     }
}

/*****************************************************************************
 *
 *  DELETE_EMPTY_UNREACHED_LOOPS()
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

flag delete_empty_unreached_loops()
{

    SET **currregion;
    register REGIONINFO *currinfo;
    flag changing;
    long i;

    changing = NO;

    for (currinfo = regioninfo, currregion = region; currregion <= lastregion;
		++currinfo, ++currregion)
	{

	if (*currregion == NULL)
	    continue;

	/* check for unreachable loop */
	if (((i = nextel(-1, predset[currinfo->backbranch.destblock]))
		== currinfo->backbranch.srcblock)
	 && ((i = nextel(i, predset[currinfo->backbranch.destblock])) == -1)
	 && (dfo[currinfo->backbranch.destblock]->bb.entries == NULL)
	 && xin(dom[currinfo->backbranch.srcblock],
		currinfo->backbranch.destblock))
	    {
	    if (disable_unreached_loop_deletion || in_reg_alloc_flag)
		{
		currinfo->unreachable = YES;
		}
	    else
		{
		delelement(currinfo->backbranch.srcblock,
				predset[currinfo->backbranch.destblock],
				predset[currinfo->backbranch.destblock]);
		delelement(currinfo->backbranch.destblock,
				succset[currinfo->backbranch.srcblock],
				succset[currinfo->backbranch.srcblock]);
		i = -1;
		while ((i = nextel(i,*currregion)) >= 0)
                    push_unreached_block_on_stack(i);
		while ((i = pop_unreached_block_from_stack()) >= 0)
		    delete_unreached_block(i);
		changing = YES;
		}
	    continue;	/* delete_unreached_block() will also check for
			   empty loops.
			*/
	    }

	/* check for empty loop */
	if (isemptyloop(*currregion, currinfo))
	    {
	    if (disable_empty_loop_deletion || in_reg_alloc_flag)
		{
		currinfo->empty = YES;
		}
	    else
		{
		switch (currinfo->type)
		    {
		    case DO_LOOP:
			changing |= delete_empty_do_loop(currinfo);
			break;

		    case FOR_LOOP:
			changing |= delete_empty_for_loop(currinfo, currregion);
			break;

		    case UNTIL_LOOP:
			changing |= delete_empty_until_loop(currinfo);
			break;
		    }
		continue;
		}
	    }
	}

    return(changing);

}  /* delete_empty_unreached_loops */

/*****************************************************************************
 *
 *  DO_REGION_TRANSFORMS()
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

void do_region_transforms(callno)
long callno;
{
    register REGIONINFO *currinfo;
    SET **currregion;

    for (currinfo = regioninfo, currregion = region; currregion <= lastregion;
		++currinfo, ++currregion)
	{

	if (*currregion == NULL)
	    continue;

	update_loop_iterations(currinfo);

	if (! loop_unroll_disable
         && (currinfo->type == FOR_LOOP)
	 && (callno == 1)	/* only unroll loops before dag'ing */
	 && currinfo->niterations
	 && (currinfo->niterations <= LOOPUNROLLMAXSTMTS))
	    loop_unroll(currregion, currinfo);

	if (*currregion == NULL)
	    continue;

	if (!dbra_disable)
	    {
	    if ((currinfo->type == DO_LOOP))
	    	xform_do_loop_to_dbra(currregion, currinfo);
	    else if (currinfo->type == FOR_LOOP)
	    	xform_for_loop_to_dbra(currregion, currinfo);
	    }

#if 0
	if ((currinfo->type == FOR_LOOP)
		/* || (currinfo->type == WHILE_LOOP) */)
	    xform_forwhile_loop_to_until(currinfo);
#endif
	}
}  /* do_region_transforms */

/*****************************************************************************
 *
 *  ISEMPTYLOOP()
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

LOCAL flag isemptyloop(regionset, regioninfo)
SET *regionset;
register REGIONINFO *regioninfo;
{
    register long xblock;
    register long stmt;
    register BBLOCK *bp;
    register NODE *np;

    /* are there any other statements in loop ?? */
    xblock = -1;
    while ((xblock = nextel(xblock, regionset)) >= 0)
	{
	bp = dfo[xblock];
	if (bp->bb.deleted)
	    continue;
	for (stmt = (bp->bb.firststmt + bp->bb.nstmts - 1);
		stmt >= bp->bb.firststmt; --stmt)
	    {
	    if (!stmttab[stmt].deleted)
		{
		np = stmttab[stmt].np;
		np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
		if ((np != regioninfo->increment) && (np != regioninfo->test)
		 && ((np->in.op != LGOTO) /* GOTO's within region don't count */
	          || !xin(regionset, bp->bb.lbp->bb.node_position)))
		    return(NO);
		}
	    }
	}

    return (YES);
}  /* isemptyloop */

/*****************************************************************************
 *
 *  LOOP_UNROLL()
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

LOCAL void loop_unroll(currregion, currinfo)
SET **currregion;
REGIONINFO *currinfo;
{
    long codeblock;

    if (!ok_to_unroll_loop(currregion, currinfo, &codeblock))
	return;

    xform_loop_to_straight_line(currinfo, codeblock);

}  /* loop_unroll */

/*****************************************************************************
 *
 *  OK_TO_UNROLL_LOOP()
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

LOCAL flag ok_to_unroll_loop(currregion, currinfo, pcodeblock)
SET **currregion;
register REGIONINFO *currinfo;
long *pcodeblock;
{
    long maxstmts;
    long nstmts;
    long lastcodeblock;
    long nblocks;
    long blockno;
    BBLOCK *bp;
    NODE *np;
    NODE *nextnp;
    NODE *code;

    /* verify that there are no memory-type or indirect refs to index var
	within loop.  And that the index var assignments within loop are
	not referenced outside the loop.
     */
    if (! ok_unroll_loop_index_var_refs(currregion, currinfo))
	return NO;
    
    maxstmts = LOOPUNROLLMAXSTMTS / currinfo->niterations;
    nstmts = 0;
    lastcodeblock = 0;
    nblocks = 0;
    blockno = 0;
    while ((blockno = nextel(blockno, *currregion)) > 0)
	{
	bp = dfo[blockno];
	if (bp->bb.deleted)
	    continue;

	/* branches out of the region, except for the single exit branch,
	 * are no good.
	 */
	switch(bp->bb.breakop)
	    {
	    case GOTO:
	    case FCOMPGOTO:
		    return(NO);
	    case CBRANCH:
		    if (blockno != currinfo->test_block)
		        {
		        if (!xin(*currregion, bp->bb.lbp->bb.node_position))
			    return(NO);
		        if (!xin(*currregion, bp->bb.rbp->bb.node_position))
			    return(NO);
		        }
		    break;
	    case LGOTO:
	    case FREE:
		    if (!xin(*currregion, bp->bb.lbp->bb.node_position))
			return(NO);
		    break;
	    }

	/* Since all the code has to be in a single block, there are exactly
	 * three blocks in the loop.
	 */
	if (++nblocks > 3)
	    return(NO);

	/* Count the statements -- bomb out if over the threshold */
	if (np = bp->bb.treep)
	    {
	    while (np)
		{
		if (np->in.op == SEMICOLONOP)
		    {
		    nextnp = np->in.left;
		    code = np->in.right;
		    }
		else
		    {
		    nextnp = NULL;
		    code = np->in.left;
		    }
		if ((code->in.op != NOCODEOP) && (code != currinfo->test)
		 && (code != currinfo->increment) && (code->in.op != LGOTO)
		 && (code->in.op != CBRANCH))
		    {
		    if (lastcodeblock && (lastcodeblock != blockno))
			return(NO);
		    if (++nstmts > maxstmts)
			return(NO);
		    lastcodeblock = blockno;
		    }
		np = nextnp;
		}
	    }
	}

    	if ((lastcodeblock == 0) || (nstmts == 0))
		return(NO);
	if (dfo[lastcodeblock]->bb.breakop != FREE)
		/* The body isn't the correct shape for unrolling */
		return(NO);

	*pcodeblock = lastcodeblock;
	return(YES);
}  /* ok_to_unroll_loop */

/*****************************************************************************
 *
 *  OK_UNROLL_LOOP_INDEX_VAR_REFS()
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
LOCAL flag ok_unroll_loop_index_var_refs(currregion, currinfo)
SET **currregion;
register REGIONINFO *currinfo;
{
    register long loaddefno;
    register long incrdefno;
    register long i;
    register DUBLOCK *du;
    register DUBLOCK *duend;
    register long xblock;
    HASHU *sp;
    SET *regionblockset;

    sp = currinfo->index_var;
    regionblockset = *currregion;

    /* find define #'s for initial load and increment */
    if (! currinfo->initial_load)
#pragma BBA_IGNORE
	cerror("no initial load for region in ok_unroll_loop_index_var_refs()");
    if (! currinfo->increment)
#pragma BBA_IGNORE
	cerror("no increment for region in ok_unroll_loop_index_var_refs()");
    loaddefno = 0;
    incrdefno = 0;
    for (i = 1; i <= lastdefine; ++i)
	{
	if (definetab[i].np == currinfo->initial_load)
	    loaddefno = i;
	else if (definetab[i].np == currinfo->increment)
	    incrdefno = i;
	if (loaddefno && incrdefno)
		break;
	}
    if (loaddefno == 0)
#pragma BBA_IGNORE
	cerror("define for initial load not found in ok_unroll_loop_index_var_refs()");
    if (incrdefno == 0)
#pragma BBA_IGNORE
	cerror("define for increment not found in ok_unroll_loop_index_var_refs()");

    /* go through all DU blocks for index var.  Not OK to unroll loop if
     * any memory-type refs to index var inside loop, or if index value used
     * outside loop.
     */
    duend = sp->an.du->next;
    du = duend;
    do {
	xblock = stmttab[du->stmtno].block;
	if (du->deleted)
	    goto nextdu;
	if ((du->stmtno > 0) && xin(regionblockset, xblock))
	    {
	    if (du->ismemory || !du->isdefinite)
		return(NO);
	    }
	else	/* not in region */
	    {
	    if (!du->isdef && (du->defsetno == 0))
						 /* use w/o def in block*/
		{
	        if (du->stmtno > 0)		/* regular stmt */
		    {
	            if (xin(inreach[xblock], loaddefno)	/* load or incr */
	             || xin(inreach[xblock], incrdefno))	/* reaches */
			return(NO);
		    }
		else if ((exitblockno >= 0) 
		      && (xin(inreach[exitblockno], loaddefno)	/* exit use, */
	               || xin(inreach[exitblockno], incrdefno))) /* load or incr */
								/* reaches */
		    return(NO);
		}
	    }
nextdu:
	du = du->next;
        } while (du != duend);
    return(YES);
}  /* ok_unroll_loop_index_var_refs */

/*****************************************************************************
 *
 *  REPLACE_INDEX_VAR()
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

LOCAL void replace_index_var(sp, parent, usetype, isdefinite, hidden_use)
HASHU *sp;
register NODE *parent;
long usetype;
long isdefinite;
long hidden_use;
{
    register NODE *np;
    register NODE *pp;

    if (sp != index_var_sp)
	return;

    if (parent == NULL)
#pragma BBA_IGNORE
	cerror("NULL parent in replace_index_var()");

    if (parent->in.op == CALL)
	{
	pp = parent;
	np = parent->in.right;
	if (np->in.op == CM)
	    {
nextarg:
	    pp = np;
	    np = pp->in.right;
	    if ((np->in.op == index_var_node->in.op)
	     && (np->tn.lval == index_var_node->tn.lval)
	     && (np->tn.rval == index_var_node->tn.rval)
	     && (!strcmp(np->atn.name, index_var_node->atn.name)))
		{
		np->in.op = FREE;
		pp->in.right = tcopy(index_replace);
		return;
		}
	    np = pp->in.left;
	    if (np->in.op == CM)
		goto nextarg;
	    }
	if (np->in.op == UCM)
	    {
	    pp = np;
	    np = pp->in.left;
	    if ((np->in.op == index_var_node->in.op)
	     && (np->tn.lval == index_var_node->tn.lval)
	     && (np->tn.rval == index_var_node->tn.rval)
	     && (!strcmp(np->atn.name, index_var_node->atn.name)))
	        {
	        np->in.op = FREE;
	        pp->in.left = tcopy(index_replace);
	        return;
	        }
	    }
	else
	    {
	    if ((np->in.op == index_var_node->in.op)
	     && (np->tn.lval == index_var_node->tn.lval)
	     && (np->tn.rval == index_var_node->tn.rval)
	     && (!strcmp(np->atn.name, index_var_node->atn.name)))
	        {
	        np->in.op = FREE;
	        pp->in.right = tcopy(index_replace);
	        return;
	        }
	    }
        cerror("index var not found in replace_index_var()");
	}

    if (optype(parent->in.op) == LTYPE)
#pragma BBA_IGNORE
	cerror("parent op == LTYPE in replace_index_var()");

    np = parent->in.left;
    if ((np->in.op == index_var_node->in.op)
     && (np->tn.lval == index_var_node->tn.lval)
     && (np->tn.rval == index_var_node->tn.rval)
     && (!strcmp(np->atn.name, index_var_node->atn.name)))
	{
	np->in.op = FREE;
	parent->in.left = tcopy(index_replace);
	return;
	}

    if (optype(parent->in.op) == UTYPE)
#pragma BBA_IGNORE
	cerror("parent op == UTYPE in replace_index_var()");

    np = parent->in.right;
    if ((np->in.op == index_var_node->in.op)
     && (np->tn.lval == index_var_node->tn.lval)
     && (np->tn.rval == index_var_node->tn.rval)
     && (!strcmp(np->atn.name, index_var_node->atn.name)))
	{
	np->in.op = FREE;
	parent->in.right = tcopy(index_replace);
	return;
	}

    cerror("index var not found in replace_index_var()");
}  /* replace_index_var */

/*****************************************************************************
 *
 *  UPDATE_LOOP_ITERATIONS()
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

LOCAL void update_loop_iterations(currinfo)
register REGIONINFO *currinfo;
{
    register NODE *np;
    register NODE *np1;
    long find_loc;
    long bound = 0;
    long init = 0;

    if (currinfo->type == DO_LOOP)
	{

	if (np = currinfo->initial_load)
	    {
	    if ((np->in.right->in.op == ICON)
	     && (np->in.right->atn.name == NULL))
		{
		currinfo->const_init_value = YES;
		init = np->in.right->tn.lval;
		}
	    }

	if (np = currinfo->test)
	    {
	    np1 = np->in.left;		/* comparison operator */
	    if ((optype(np1->in.left->in.op) == LTYPE)	/* should be index var */
	      && ((np1->in.left->in.op != ICON) || (np1->in.left->atn.name))
	      && ((find_loc = find(np1->in.left)) != (unsigned) (-1))
	      && (symtab[find_loc] == currinfo->index_var))
		{
		if ((np1->in.right->in.op == ICON)
		 && (np1->in.right->atn.name == NULL))
		    {
		    currinfo->const_bound_value = YES;
		    bound = np1->in.right->atn.lval;
		    }
		}
	    else if ((optype(np1->in.right->in.op) == LTYPE)  /* should be index var */
	      && ((np1->in.right->in.op != ICON) || (np1->in.right->atn.name))
	      && ((find_loc = find(np1->in.right)) != (unsigned) (-1))
	      && (symtab[find_loc] == currinfo->index_var))
		{
		if ((np1->in.left->in.op == ICON)
		 && (np1->in.left->atn.name == NULL))
		    {
		    currinfo->const_bound_value = YES;
		    bound = np1->in.left->atn.lval;
		    }
		}
	    }

        if (currinfo->const_bound_value && currinfo->const_init_value)
	    {
	    if (currinfo->increment_value == 1)
	        {
	        if (bound < init)
		    currinfo->niterations = 1;
	        else
		    currinfo->niterations = bound - init + 1;
	        }
	    else if (currinfo->increment_value == -1)
	        {
	        if (init < bound)
		    currinfo->niterations = 1;
	        else
		    currinfo->niterations = init - bound + 1;
	        }
	    }
	}
    else if (currinfo->type == FOR_LOOP)
	{
	}
}  /* update_loop_iterations */

/*****************************************************************************
 *
 *  XFORM_DO_LOOP_TO_DBRA()
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

LOCAL void xform_do_loop_to_dbra(currregion, currinfo)
SET **currregion;
register REGIONINFO *currinfo;
{
    register long i;
    long stmtno;
    long init_uses;
    long incr_uses;
    long n;
    NODE *bound;
    register NODE *np;
    register NODE *np1;
    long type;
    register BBLOCK *bp;

    /* increment must be +-1 */
    if ((currinfo->increment_value != 1) && (currinfo->increment_value != -1))
	return;

    /* increment is the only def of index variable within loop */
    i = 0;
    while ((i = nextel(i, currinfo->index_var->an.defset)) > 0)
	{
	stmtno = definetab[i].stmtno;
	if ((stmtno > 0) && !stmttab[stmtno].deleted
	 && xin(*currregion,stmttab[stmtno].block)
	 && (definetab[i].np != currinfo->increment))
	    return;
	}

    /* Has the initial test been removed??? */
    if ((np = currinfo->initial_test) && (np->in.op != CBRANCH))
	currinfo->initial_test = NULL;


    /* increment and test are the only uses of index variable defs from
     * initialization and increment statements.
     */
    init_uses = 2;	/* increment and test */
    incr_uses = 2;	/* increment and test */
    /* increment for initial test if not constant propagated */
    if (currinfo->initial_test && !currinfo->const_init_value)
	init_uses++;

    np = currinfo->initial_load;
    for (i = 1; i <= lastdefine; ++i)
	{
	if (np == definetab[i].np)
	    break;
	}
    if (i > lastdefine)
#pragma BBA_IGNORE
	cerror("initial load not found in xform_do_loop_to_dbra()");
    if (definetab[i].nuses != init_uses)
	return;

    np = currinfo->increment;
    for (++i; i <= lastdefine; ++i)
	{
	if (np == definetab[i].np)
	    break;
	}
    if (i > lastdefine)
#pragma BBA_IGNORE
	cerror("increment not found in xform_do_loop_to_dbra()");
    if (definetab[i].nuses != incr_uses)
	return;

    /* this is not a $ONETRIP loop that may be executed only once */
    if (!currinfo->initial_test)
	{
	if (!currinfo->const_init_value || !currinfo->const_bound_value
	 || (currinfo->niterations == 1))
	    return;
	}

    /* Are there less than 1000 statements in the loop ??? */
    n = 0;
    i = -1;
    while ((i = nextel(i, *currregion)) >= 0)
	{
	n += dfo[i]->bb.nstmts;
	}

    if (n > 1000)
	return;

    /* OK -- DO THE TRANSFORMATION!!!! */
    if (currinfo->bound_load)
	{
	np = currinfo->bound_load;
	if (np->in.op == NOCODEOP)
	    goto nobound;
	bound = np->in.right;
	tfree(np->in.left);
	np->in.op = NOCODEOP;
	currinfo->bound_load = NULL;
	}
    else
	{
nobound:
	np = currinfo->test;
	bound = tcopy(np->in.left->in.right);
	}

    type = currinfo->index_var->an.type.base;


    /* load the index value */
    np = currinfo->initial_load;

    if (currinfo->increment_value > 0)
	np->in.right = block(MINUS, bound, np->in.right, type, 0);
    else
	np->in.right = block(MINUS, np->in.right, bound, type, 0);
    (void) node_constant_fold(np->in.right);

    /* now, the initial test */
    if (currinfo->initial_test)
	{
        np1 = currinfo->initial_test;
	np1 = np1->in.left;	/* the comparison operator */
	np1->in.op = LT;
	tfree(np1->in.left);
	np1->in.left = tcopy(np->in.left);	/* index var */
	tfree(np1->in.right);
	np1->in.right = bcon(0, type);
	}

    /* the final test */
    np = currinfo->test;
    np = np->in.left;	/* the comparison operator */
    np->in.op = EQ;
    tfree(np->in.right);
    np->in.right = bcon(-1, type);

    /* finally, move the increment to before the final test */
    bp = dfo[currinfo->increment_block];
    np1 = bp->bb.treep;
    for (np = np1;
         ((np->in.op == SEMICOLONOP) || (np->in.op == UNARY SEMICOLONOP));
	 np = np->in.left)
	{
	if (((np->in.op == SEMICOLONOP)
	  && (np->in.right == currinfo->increment))
	 || ((np->in.op == UNARY SEMICOLONOP)
	  && (np->in.left == currinfo->increment)))
	    goto found;
	}
    cerror("increment not found in xform_do_loop_to_dbra()");
found:
    if (np1 == np)
	{
	if (np->in.op == SEMICOLONOP)
	    bp->bb.treep = np->in.left;
	else
	    {
	    bp->bb.treep = NULL;
	    np->in.op = SEMICOLONOP;
	    np->in.right = np->in.left;
	    }
	}
    else
	{
	while (np1->in.left != np)
	    np1 = np1->in.left;
	if (np->in.op == SEMICOLONOP)
	    np1->in.left = np->in.left;
	else
	    {
	    np1->in.op = UNARY SEMICOLONOP;
	    np1->in.left = np1->in.right;
	    np1->in.right = NULL;
	    np->in.op = SEMICOLONOP;
	    np->in.right = np->in.left;
	    }
	}
    bp->bb.nstmts--;

    bp = dfo[currinfo->test_block];
    for (np1 = bp->bb.treep;
         ((np1->in.op == SEMICOLONOP) || (np1->in.op == UNARY SEMICOLONOP));
	 np1 = np1->in.left)
	{
	if (((np1->in.op == SEMICOLONOP)
	  && (np1->in.right == currinfo->test))
	 || ((np1->in.op == UNARY SEMICOLONOP)
	  && (np1->in.left == currinfo->test)))
	    goto found1;
	}
    cerror("test not found in xform_do_loop_to_dbra()");

found1:
    if (np1->in.op == SEMICOLONOP)
	{
	np->in.left = np1->in.left;
	np1->in.left = np;
	}
    else
	{
	np1->in.op = SEMICOLONOP;
	np1->in.right = np1->in.left;
	np1->in.left = np;
	np->in.op = UNARY SEMICOLONOP;
	np->in.left = np->in.right;
	np->in.right = NULL;
	}

    if (currinfo->increment_value == 1)
	{
	np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
	np = np->in.right;
	if (np->in.op == PLUS)
	    {
	    if ((np->in.left->in.op == ICON) && (np->in.left->atn.name == NULL))
	        np->in.left->tn.lval = -1;
	    else if ((np->in.right->in.op == ICON)
	          && (np->in.right->atn.name == NULL))
	        np->in.right->tn.lval = -1;
	    }
	else
	    np->in.op = PLUS;
	}

    currinfo->type = DBRA_LOOP;
    currinfo->index_var->an.dbra_index = YES;
    currinfo->is_short_dbra = (currinfo->niterations > 0)
				&& (currinfo->niterations < 32767);

#ifdef COUNTING
    _n_dbra_loops++;
#endif
    
}  /* xform_do_loop_to_dbra */

/*****************************************************************************
 *
 *  XFORM_FOR_LOOP_TO_DBRA()
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

LOCAL void xform_for_loop_to_dbra(currregion, currinfo)
SET **currregion;
register REGIONINFO *currinfo;
{
    register NODE *np;
    register NODE *np1;
    register BBLOCK *bp;
    register long i;
    long stmtno;
    long init_uses;
    long incr_uses;
    long n;
    NODE *bound;
    NODE *final_test;
    long type;
    BBLOCK *test_block;
    long iter_offset;

    /* increment must be +-1 */
    if ((currinfo->increment_value != 1) && (currinfo->increment_value != -1))
	return;

    /* The limit must not be modified within the body of the loop. */
    if (currinfo->bound_modified_within_loop)
	return;

    /* Check that the test is either signed or within range of dbra */
    np = currinfo->test;
    if ( ISUNSIGNED(np->in.type) )
	{
	if (!ISCONST(np->in.left->in.right))
		return;
	np = np->in.left->in.right;	/* The quantity being tested against */
	switch(np->in.type.base)
		{
		case USHORT:
			if (np->tn.lval & 0x8000)
				return;
			break;
		case UCHAR:
			if (np->tn.lval &0x80)
				return;
			break;
		default:
			if (np->tn.lval & 0x80000000)
				return;
			break;
		}
	}

    /* increment is the only def of index variable within loop */
    i = 0;
    while ((i = nextel(i, currinfo->index_var->an.defset)) > 0)
	{
	stmtno = definetab[i].stmtno;
	if ((stmtno > 0) && !stmttab[stmtno].deleted
	 && xin(*currregion,stmttab[stmtno].block)
	 && (definetab[i].np != currinfo->increment))
	    return;
	}

    /* increment and test are the only uses of index variable defs from
     * initialization and increment statements.
     */
    init_uses = 2;	/* increment and test */
    incr_uses = 2;	/* increment and test */

    np = currinfo->initial_load;
    for (i = 1; i <= lastdefine; ++i)
	{
	if (np == definetab[i].np)
	    break;
	}
    if (i > lastdefine)
#pragma BBA_IGNORE
	cerror("initial load not found in xform_for_loop_to_dbra()");
    if (definetab[i].nuses != init_uses)
	return;

    np = currinfo->increment;
    for (++i; i <= lastdefine; ++i)
	{
	if (np == definetab[i].np)
	    break;
	}
    if (i > lastdefine)
#pragma BBA_IGNORE
	cerror("increment not found in xform_for_loop_to_dbra()");
    if (definetab[i].nuses != incr_uses)
	return;

    /* Are there less than 1000 statements in the loop ??? */
    n = 0;
    i = -1;
    while ((i = nextel(i, *currregion)) >= 0)
	{
	n += dfo[i]->bb.nstmts;
	}

    if (n > 1000)
	return;

    /* OK -- DO THE TRANSFORMATION!!!! */
    np = currinfo->test;
    bound = tcopy(np->in.left->in.right);

    type = currinfo->index_var->an.type.base;


    /* load the index value */
    np = currinfo->initial_load;

    switch (currinfo->test->in.left->in.op)  /* comparison operator */
	{
	case LT:
	case GT:
	case ULT:
	case UGT:
	case NE:
		/* decrement # iterations by 1 */
		iter_offset = -1;
		break;
	default:
		iter_offset = 0;
		break;
	}

    if (currinfo->increment_value > 0)
	{
	if (iter_offset)
	    {
	    if ((bound->tn.op == ICON) && (bound->atn.name == NULL))
		{
		bound->tn.lval += iter_offset;
		iter_offset = 0;
		}
	    else if ((np->in.right->tn.op == ICON)
		  && (np->in.right->atn.name == NULL))
		{
		np->in.right->tn.lval -= iter_offset;
		iter_offset = 0;
		}
	    }
	if ((np->in.right->in.op == ICON) && (np->in.right->atn.name == NULL)
	 && (np->in.right->tn.lval == 0))
	    {
	    np->in.right->in.op = FREE;	  /* no need to subtract zero at r.t. */
	    np->in.right = bound;
	    }
	else
	    {
	    np->in.right = block(MINUS, bound, np->in.right, type, 0);
	    }
	}
    else
	{
	if (iter_offset)
	    {
	    if ((bound->tn.op == ICON) && (bound->atn.name == NULL))
		{
		bound->tn.lval -= iter_offset;
		iter_offset = 0;
		}
	    else if ((np->in.right->tn.op == ICON)
		  && (np->in.right->atn.name == NULL))
		{
		np->in.right->tn.lval += iter_offset;
		iter_offset = 0;
		}
	    }
	if ((bound->in.op == ICON) && (bound->atn.name == NULL)
	 && (bound->tn.lval == 0))
	    bound->in.op = FREE;	  /* no need to subtract zero at r.t. */
	else
	    np->in.right = block(MINUS, np->in.right, bound, type, 0);
	}
    (void) node_constant_fold(np->in.right);

    if (iter_offset)
	np->in.right = block(PLUS, np->in.right, bcon(iter_offset), type, 0);

    /* If this region has a preheader, move the index load to be the last
     * statement in the preheader.
     */
    if ((n = preheader_index[currinfo - regioninfo]) > 0)
	{
	NODE *preheadtree;
	NODE *follow;

	bp = dfo[n];
	if (preheadtree = bp->bb.treep)
	    {
	    np1 = (preheadtree->in.op == SEMICOLONOP) ?
			preheadtree->in.right : preheadtree->in.left;
	    if (np1->in.op == LGOTO)
		{

		/* add load to preheader block */
		if (preheadtree->in.op == SEMICOLONOP)
		    {
		    np1 = talloc();
		    np1->in.op = SEMICOLONOP;
		    np1->in.left = preheadtree->in.left;
		    np1->in.right = np;
		    preheadtree->in.left = np1;
		    }
		else
		    {
		    preheadtree->in.op = SEMICOLONOP;
		    preheadtree->in.right = preheadtree->in.left;
		    np1 = talloc();
		    preheadtree->in.left = np1;
		    np1->in.op = UNARY SEMICOLONOP;
		    np1->in.left = np;
		    }

		/* remove load from initial load block */
		bp = dfo[currinfo->initial_load_block];
		np1 = bp->bb.treep;
		follow = NULL;
again:
		if (np1->in.op == SEMICOLONOP)
		    {
		    if (np1->in.right == np)
			{
			if (follow)
			    follow->in.left = np1->in.left;
			else
			    bp->bb.treep = np1->in.left;
			np1->in.op = FREE;
			}
		    else
			{
			follow = np1;
			np1 = np1->in.left;
			goto again;
			}
		    }
		else  /* UNARY SEMICOLONOP */
		    {
		    if (np1->in.left == np)
			{
			np1->in.op = FREE;
			if (follow)
			    {
			    follow->in.left = follow->in.right;
			    follow->in.op = UNARY SEMICOLONOP;
			    }
			else
			    {
#pragma BBA_IGNORE
			    cerror("missing LGOTO in initial load block in xform_for_loop_to_dbra()");
			    }
			}
		    else
			{
#pragma BBA_IGNORE			   
			cerror("initial load not found in xform_for_loop_to_dbra()");
			}
		    }

		/* remove new location of load */
		currinfo->initial_load_block = n;
		}
	    }
	}

    /* construct the final test */
    final_test = tcopy(currinfo->test);
    test_block = dfo[currinfo->test_block];
    np1 = final_test->in.left;	/* the comparison operator */

    np1->in.op = EQ;
    tfree(np1->in.left);
    np1->in.left = tcopy(np->in.left);	/* index var */
    tfree(np1->in.right);
    np1->in.right = bcon(-1, type);
    final_test->in.right->tn.lval = test_block->bb.lbp->bb.l.val;

    /* replace the back branch with the final test */
    bp = dfo[currinfo->increment_block];
    np = bp->bb.treep;
    if ((np->in.op != SEMICOLONOP) || (np->in.right->in.op != LGOTO))
#pragma BBA_IGNORE
	cerror("bad backbranch in xform_for_loop_to_dbra()");
    tfree(np->in.right);	/* throw away the LGOTO */
    np->in.right = final_test;

    /* patch up the predecessor/successor sets */
    delelement(bp->bb.node_position,
		predset[bp->bb.lbp->bb.node_position],    
    		predset[bp->bb.lbp->bb.node_position]);
    delelement(bp->bb.lbp->bb.node_position,
		succset[bp->bb.node_position],    
    		succset[bp->bb.node_position]);
    bp->bb.lbp = test_block->bb.rbp;
    bp->bb.rbp = test_block->bb.lbp;
    bp->bb.breakop = CBRANCH;
    currinfo->backbranch.destblock = bp->bb.rbp->bb.node_position;
    adelement(bp->bb.node_position,
		predset[bp->bb.lbp->bb.node_position],
		predset[bp->bb.lbp->bb.node_position]);
    adelement(bp->bb.node_position,
		predset[bp->bb.rbp->bb.node_position],
		predset[bp->bb.rbp->bb.node_position]);
    adelement(bp->bb.lbp->bb.node_position,
		succset[bp->bb.node_position],
		succset[bp->bb.node_position]);
    adelement(bp->bb.rbp->bb.node_position,
		succset[bp->bb.node_position],
		succset[bp->bb.node_position]);

    /* now, the initial test */
    np = currinfo->initial_load;
    if ((np->in.right->in.op != ICON) || np->in.right->atn.name ||
        (np->in.right->tn.lval < 0))
	{
        np1 = currinfo->test;
	np1 = np1->in.left;	/* the comparison operator */
	np1->in.op = GE;
	tfree(np1->in.left);
	np1->in.left = tcopy(np->in.left);	/* index var */
	tfree(np1->in.right);
	np1->in.right = bcon(0, type);
	}
    else
	{
	/* delete the block */
	i = nextel(-1, predset[currinfo->test_block]);
	if (i < 0)
#pragma BBA_IGNORE
	    cerror("test block has no predecessors in xform_for_loop_to_dbra()");
	if (nextel(i, predset[currinfo->test_block]) >= 0)
#pragma BBA_IGNORE
	    cerror("test block has >1 predecessors in xform_for_loop_to_dbra()");
	test_block = dfo[currinfo->test_block];
	tfree(test_block->bb.treep);
	test_block->bb.treep = NULL;
	delelement(i, predset[currinfo->test_block],
			predset[currinfo->test_block]);
	delelement(currinfo->test_block, succset[i], succset[i]);
	n = test_block->bb.lbp->bb.node_position;
	adelement(i, predset[n], predset[n]);
	adelement(n, succset[i], succset[i]);
	delelement(currinfo->test_block,
			predset[test_block->bb.lbp->bb.node_position],
			predset[test_block->bb.lbp->bb.node_position]);
	delelement(currinfo->test_block,
			predset[test_block->bb.rbp->bb.node_position],
			predset[test_block->bb.rbp->bb.node_position]);
	delelement(test_block->bb.lbp->bb.node_position,
			succset[currinfo->test_block],
			succset[currinfo->test_block]);
	delelement(test_block->bb.rbp->bb.node_position,
			succset[currinfo->test_block],
			succset[currinfo->test_block]);
	test_block->bb.breakop = FREE;
	test_block->bb.lbp = test_block+1;
	test_block->bb.rbp = NULL;
	test_block->bb.deleted = YES;

	bp = dfo[i];
	switch(bp->bb.breakop)
	    {
	    case FREE:
		bp->bb.breakop = LGOTO;
		bp->bb.lbp = dfo[n];
		np = talloc();
		np->in.op = LGOTO;
		np->bn.label = bp->bb.lbp->bb.l.val;
		bp->bb.treep = block(SEMICOLONOP, bp->bb.treep, np, INT);
		break;
	    case LGOTO:
		bp->bb.lbp = dfo[n];
		np = bp->bb.treep;
		np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
		np->tn.lval = bp->bb.lbp->bb.l.val;
		break;
	    default:
#pragma BBA_IGNORE
		cerror("unexpected breakop in xform_for_loop_to_dbra()");
	    }
	}
    delelement(currinfo->test_block, *currregion, *currregion);

    if (currinfo->increment_value == 1)
	{
	np = currinfo->increment;
	np = np->in.right;
	if (np->in.op == PLUS)
	    {
	    if ((np->in.left->in.op == ICON) && (np->in.left->atn.name == NULL))
	        np->in.left->tn.lval = -1;
	    else if ((np->in.right->in.op == ICON)
	          && (np->in.right->atn.name == NULL))
	        np->in.right->tn.lval = -1;
	    }
	else
	    np->in.op = PLUS;
	}

    currinfo->type = DBRA_LOOP;
    currinfo->index_var->an.dbra_index = YES;
    currinfo->is_short_dbra = (currinfo->niterations > 0)
				&& (currinfo->niterations < 32767);
    currinfo->test_block = currinfo->increment_block;
    currinfo->test = dfo[currinfo->test_block]->bb.treep->in.right;

#ifdef COUNTING
    _n_dbra_loops++;
#endif
    
}  /* xform_for_loop_to_dbra */

/*****************************************************************************
 *
 *  XFORM_FORWHILE_LOOP_TO_UNTIL()
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

# if 0
LOCAL void xform_forwhile_loop_to_until(currinfo)
REGIONINFO *currinfo;
{
}  /* xform_forwhile_loop_to_dbra */
# endif 0

/*****************************************************************************
 *
 *  XFORM_LOOP_TO_STRAIGHT_LINE()
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

LOCAL void xform_loop_to_straight_line(currinfo, codeblock)
REGIONINFO *currinfo;
long codeblock;
{
    register NODE *code;
    register NODE *semi;
    register BBLOCK *bp;
    register BBLOCK *succ;
    register long i;
    NODE *outcode;
    NODE *initnp;
    NODE *newcode;
    
    /* find new successor block */
    succ = dfo[currinfo->test_block]->bb.rbp;

    /* extract code from body of loop */
    bp = dfo[codeblock];
    code = bp->bb.treep;

    while (code->in.op != UNARY SEMICOLONOP)
	{
	code = code->in.left;
	}

    if ((code->in.left->in.op == LGOTO) || (code->in.left->in.op == CBRANCH))
#pragma BBA_IGNORE
	cerror("branch in block in xform_loop_to_straight_line");

    code = bp->bb.treep;		/* the whole tree */

    /* expand the code block */
    outcode = NULL;
    initnp = currinfo->initial_load;
    index_var_sp = currinfo->index_var;
    index_var_node = initnp->in.left;
    initnp = initnp->in.right;

    pass1_collect_attributes = NO;
    simple_def_proc = replace_index_var;
    simple_use_proc = replace_index_var;

    for (i = 0; i < currinfo->niterations; ++i)
	{
	index_replace = block(PLUS, tcopy(initnp),
		  bcon(i * currinfo->increment_value, index_var_sp->an.type.base),
		  0, index_var_sp->an.type);
	(void) node_constant_fold(index_replace);
	if ((i + 1) == currinfo->niterations)
	    newcode = code;
	else
	    newcode = tcopy(code);

	/* replace all refs to index var */
	for (semi = newcode; semi; )
	    {
	    if (semi->in.op == SEMICOLONOP)
		{
	        traverse_stmt_1(semi->in.right, NULL);
		semi = semi->in.left;
		}
	    else
		{
	        traverse_stmt_1(semi->in.left, NULL);
		semi = NULL;
		}
	    }

	tfree(index_replace);

	if (outcode)
	    {
	    for (semi = newcode; semi->in.op == SEMICOLONOP;
				semi = semi->in.left)
		;
	    semi->in.op = SEMICOLONOP;
	    semi->in.right = semi->in.left;
	    semi->in.left = outcode;
	    }
	outcode = newcode;
	}

    walkf(outcode, node_constant_fold);

    bp->bb.treep = NULL;
    bp->bb.nstmts = 0;

    /* get rid of increment */
    delelement(bp->bb.node_position, predset[bp->bb.lbp->bb.node_position],
		predset[bp->bb.lbp->bb.node_position]);
    delelement(bp->bb.lbp->bb.node_position, succset[bp->bb.node_position],
		succset[bp->bb.node_position]);
    if (!isemptyset(predset[bp->bb.lbp->bb.node_position]))
#pragma BBA_IGNORE
	cerror("not empty set in xform_loop_to_straight_line()");
    delete_unreached_block(bp->bb.lbp->bb.node_position);

    while((i = pop_unreached_block_from_stack()) >= 0)
	delete_unreached_block(i);

    /* replace initial_load with the new tree */
    bp = dfo[currinfo->initial_load_block];
    
    code = bp->bb.treep;
    if (code == NULL)
#pragma BBA_IGNORE
	cerror("bad initial_load block in xform_loop_to_straight_line()");

    if (code->in.op == UNARY SEMICOLONOP)
	{
        if (code->in.left != currinfo->initial_load)
#pragma BBA_IGNORE
	    cerror("bad initial_load block in xform_loop_to_straight_line()");
	tfree(code);
	semi = NULL;
	}
    else
	{
        if (code->in.right != currinfo->initial_load)
#pragma BBA_IGNORE
	    cerror("bad initial_load block in xform_loop_to_straight_line()");
	semi = code->in.left;
	tfree(code->in.right);
	code->in.op = FREE;
	}

    bp->bb.treep = outcode;

    if (semi)
	{
        for (code = outcode; code->in.op == SEMICOLONOP; code = code->in.left)
	    ;
	code->in.op = SEMICOLONOP;
	code->in.right = code->in.left;
	code->in.left = semi;
	}

    /* fix up predecessor/successor stuff */

    adelement(bp->bb.node_position, predset[succ->bb.node_position],
    		predset[succ->bb.node_position]);
    adelement(succ->bb.node_position, succset[bp->bb.node_position],
    		succset[bp->bb.node_position]);

    delelement(bp->bb.node_position, predset[(bp+1)->bb.node_position],
		predset[(bp+1)->bb.node_position]);
    delelement((bp+1)->bb.node_position, succset[bp->bb.node_position],
		succset[bp->bb.node_position]);
    if (!isemptyset(predset[(bp+1)->bb.node_position]))
#pragma BBA_IGNORE
	cerror("not empty set in xform_loop_to_straight_line()");
    delete_unreached_block((bp+1)->bb.node_position);

    while((i = pop_unreached_block_from_stack()) >= 0)
	delete_unreached_block(i);

    bp->bb.breakop = FREE;
    bp->bb.lbp = succ;
    bp->bb.rbp = NULL;

#ifdef COUNTING
    _n_loops_unrolled++;
#endif

}  /* xform_loop_to_straight_line */
