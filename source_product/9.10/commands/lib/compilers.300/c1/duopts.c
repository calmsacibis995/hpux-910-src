/* file duopts.c */
/* @(#) $Revision: 70.2 $ */
/* KLEENIX_ID @(#)duopts.c	20.3 92/03/24 */


/*****************************************************************************
 *
 *  File duopts.c contains:
 *
 *****************************************************************************
 */


#include "c1.h"

/* functions defined in this file */

LOCAL void cbranch_fold();
LOCAL flag check_const_prop();
LOCAL flag check_copy_prop();
LOCAL flag check_dead_store();
LOCAL void cleanup_def_use_opts();
      void constant_fold();
LOCAL void constant_propagation();
      void delete_unreached_block();
LOCAL void do_def_use_opts();
LOCAL void fold_constant_cbranches();
      void global_def_use_opts();
LOCAL void initialize_definetab();
LOCAL flag isvalueuse();
LOCAL void lcp_check_use();
LOCAL void local_copy_propagation();
LOCAL void move_def_error();
LOCAL void move_use();
      long pop_unreached_block_from_stack();
      void push_unreached_block_on_stack();
      void remove_dead_store();
LOCAL void remove_def();
LOCAL void remove_use();
LOCAL void delete_region();

/*****************************************************************************
 *
 * DEFINITIONS
 *
 *****************************************************************************
 */

flag	deadstore_disable;
flag	copyprop_disable;
flag	constprop_disable;
flag	disable_unreached_loop_deletion;
flag	disable_empty_loop_deletion;
flag	dbra_disable;
flag	loop_unroll_disable;
LOCAL	long	rm_stmtno;
SET	*remove_use_tmp_set;
long	*remove_use_def_vector;
flag	lcp_OK;
long	lcp_source_stmtno;
long	lcp_target_stmtno;
NODE	*lcp_parent_node;
long	*unreached_block_stack;
long	last_unreached_block;
long	max_unreached_block = 20;
flag	needregioninfo;
REGIONINFO *regioninfo;
long	lastregioninfo;

/*****************************************************************************
 *
 *  CBRANCH_FOLD()
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

LOCAL void cbranch_fold(stmtno, cbranch_node)
long stmtno;
NODE *cbranch_node;
{

    NODE *np;
    register long bn;
    register BBLOCK *bp;
    register long bn_target;
    register BBLOCK *bp_target;
    register REGIONINFO *rp;
    register REGIONINFO *rpend;

    np = cbranch_node->in.left;
    bn = stmttab[stmtno].block;
    bp = dfo[bn];
    if ((np->tn.op == ICON) && (np->atn.name == NULL))
	{

#ifdef COUNTING
	_n_cbranches_folded++;
#endif

	if (np->tn.lval == 0)	/* branch not taken */
	    {
	    bp_target = bp->bb.lbp;
	    bn_target = bp_target->bb.node_position;
	    if (bp_target != bp->bb.rbp)
		{
		delelement(bn, predset[bn_target], predset[bn_target]);
		delelement(bn_target, succset[bn], succset[bn]);
		if ((isemptyset(predset[bn_target])  /* empty or only pred is self */
		  || ((nextel(-1,predset[bn_target]) == bn_target)
                   && (nextel(bn_target,predset[bn_target]) == -1)))
		 && !bp_target->bb.deleted
		 && (bp_target->bb.entries == NULL))
		    push_unreached_block_on_stack(bn_target);
		}
	    tfree(cbranch_node->in.left);
	    tfree(cbranch_node->in.right);
	    /* If a backward branch is eliminated, remove associated region */
	    if (bn_target <= bn)
	      delete_region(bn);
	    /* new use for bp_target */
	    bp_target = bp->bb.rbp;
	    }
	else			/* branch taken */
	    {
	    bp_target = bp->bb.rbp;
	    bn_target = bp_target->bb.node_position;
	    if (bp_target != bp->bb.lbp)
		{
		delelement(bn, predset[bn_target], predset[bn_target]);
		delelement(bn_target, succset[bn], succset[bn]);
		if ((isemptyset(predset[bn_target])  /* empty or only pred is self */
		  || ((nextel(-1,predset[bn_target]) == bn_target)
                   && (nextel(bn_target,predset[bn_target]) == -1)))
		 && !bp_target->bb.deleted
		 && (bp_target->bb.entries == NULL))
		    push_unreached_block_on_stack(bn_target);
		}
	    tfree(cbranch_node->in.left);
	    tfree(cbranch_node->in.right);
	    /* If a backward branch is eliminated, remove associated region */
	    if (bn_target <= bn)
	      delete_region(bn);
	    /* new use for bp_target */
	    bp_target = bp->bb.lbp;
	    }

	if (bp_target == (bp + 1))
	    {
	    cbranch_node->in.op = NOCODEOP;
	    bp->bb.breakop = FREE;
	    bp->bb.lbp = bp_target;
	    bp->bb.rbp = NULL;
	    }	
	else
	    {
	    bp->bb.breakop = LGOTO;
	    bp->bb.lbp = bp_target;
	    bp->bb.rbp = NULL;
	    cbranch_node->in.op = LGOTO;
	    cbranch_node->bn.label = bp_target->bb.l.val;
	    }

	if (! needregioninfo)
	    {

	    for (rp = regioninfo, rpend = regioninfo + lastregioninfo;
		rp <= rpend;
		++rp)
		{
		if (rp->initial_test == cbranch_node)
		    rp->initial_test = NULL;
		if (rp->test == cbranch_node)
		    rp->test = NULL;
		}
	    }

	stmttab[stmtno].deleted = YES;

	while ((bn = pop_unreached_block_from_stack()) >= 0)
	    {
	    delete_unreached_block(bn);
	    }
	}
}  /* cbranch_fold */

/*****************************************************************************
 *
 *  CHECK_CONST_PROP()
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

LOCAL flag check_const_prop(defno)
long defno;
{
    HASHU *sp;
    SET *defset;
    long currblock;
    long lblock;
    register long tdef;
    register flag badblock;
    flag didprop;
    register DUBLOCK *du;
    register DUBLOCK *duend;
    NODE *np;
    long isleft;
    NODE *parent;
    flag founddef;

    sp = definetab[defno].sym;
    defset = new_set(maxdefine);
    didprop = NO;

    /* find definite uses reached *only* by this def */
    du = sp->an.du;
    if (du == NULL)
#pragma BBA_IGNORE
	cerror("null DU chain in check_const_prop()");
    du = du->next;
    duend = du;
    currblock = -1;

    do {
	if ((du->stmtno > 0)	/* normal stmt */
	 && !du->deleted)
	    {
	    lblock = stmttab[du->stmtno].block;

	    if (du->isdef)
		{
		currblock = lblock;
		if (du->defsetno == defno)
		    badblock = NO;
		else
		    badblock = YES;
		}
	    else	/* use */
		{
		if (du->isdefinite)
		    {
		    if (lblock != currblock)
			{
			currblock = lblock;
			intersect(sp->an.defset, inreach[lblock], defset);
			tdef = 0;
			badblock = NO;
			founddef = NO;
			while ((tdef = nextel(tdef, defset)) > 0)
			    {
			    founddef = YES;
			    if ((tdef != defno) && !definetab[tdef].deleted)
				{
				badblock = YES;
				break;
				}
			    }
			if (!founddef)	/* uninitialized var */
			    {
			    badblock = YES;
			    uninitialized_var(definetab[defno].sym);
			    }
			}
		    if (!badblock && !du->parent_is_asgop
		     && isvalueuse(sp, du, &np, &parent, &isleft))
			{
			constant_propagation(defno, du, np, parent);
			constant_fold(du->stmtno);
			didprop = YES;
			}
		    }
		}
	    }
	du = du->next;
	} while (du != duend);

    FREESET(defset);
    return (didprop);
}  /* check_const_prop */

/*****************************************************************************
 *
 *  CHECK_COPY_PROP()
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

LOCAL flag check_copy_prop(defno)
long defno;
{
    struct deftab *dp;
    register DUBLOCK *du;
    register DUBLOCK *def_du;
    NODE *np;
    long blockno;
    long isleft;
    NODE *parent;

    dp = definetab + defno;
    blockno = stmttab[dp->stmtno].block;

    /* Check that it's a "value" use following in the same
     * block
     */
    for (du = dp->sym->an.du->next;
	 (!du->isdef || (du->defsetno != defno));
	 du = du->next)
	;

    def_du = du;

    du = du->next;	/* target du ?? */
    while (du->deleted)
	du = du->next;
    if (!du->isdef && du->isdefinite && !du->parent_is_asgop
     && !du->hides_maybe_use
     && (stmttab[du->stmtno].block == blockno)
     && isvalueuse(dp->sym, du, &np, &parent, &isleft))
	{
	/* Now check that all uses on the right-hand side don't have any
	 * defs between here and there.
	 */
	lcp_source_stmtno = dp->stmtno;
	lcp_target_stmtno = du->stmtno;
	pass1_collect_attributes = YES;
	lcp_OK = YES;
	/* This change accompanies a change to initialize_definetab.
	 * Copy propagation is no longer restricted if the source stmt
	 * has side effects.  Instead, make sure that no two stmts with
	 * side effects are changing order.
	 */
	if (stmttab[lcp_source_stmtno].hassideeffects)
	  {
	  int i;
	  for (i = lcp_source_stmtno + 1; i <= lcp_target_stmtno; i++)
	    if (stmttab[i].hassideeffects)
	      lcp_OK = NO;
	  if (lcp_OK)
	    { /*  Make sure this is not an embedded assignment stmt. */
	    if (stmttab[lcp_source_stmtno].np->in.op == UNARY SEMICOLONOP)
	      {
	      if (stmttab[lcp_source_stmtno].np->in.left != dp->np)
		lcp_OK = NO;
	      }
	    else if (stmttab[lcp_source_stmtno].np->in.op == SEMICOLONOP)
	      {
	      if (stmttab[lcp_source_stmtno].np->in.right != dp->np)
		lcp_OK = NO;
	      }
	    }
	  }
        if (lcp_OK)
	  {
	  simple_def_proc = move_def_error;
	  simple_use_proc = lcp_check_use;
	  traverse_stmt_1(dp->np->in.right, dp->np);
	  }
	if (lcp_OK)
	    {
	    local_copy_propagation(def_du, du, parent, isleft);
	    constant_fold(lcp_target_stmtno);
	    return YES;
	    }
	}

    return NO;
}  /* check_copy_prop */

/*****************************************************************************
 *
 *  CHECK_DEAD_STORE()
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

LOCAL flag check_dead_store(defno)
long defno;
{
    register DUBLOCK *du;
    register DUBLOCK *du_next;
    register DUBLOCK *du_follow;
    struct deftab *dp;

    dp = definetab + defno;

    if (dp->nuses == 0)
	{
	remove_dead_store(defno);
	return(YES);
	}
    else if (dp->nuses == 1)
	{
	/* find def in DU chain */

	du_follow = NULL;
        for (du = dp->sym->an.du->next;
	     (!du->isdef || (du->defsetno != defno));
	     du_follow = du, du = du->next)
	    ;

	/* Is the use in the same statement */
	du_next = du->next;
	if ((!du_next->isdef && (du_next->stmtno == du->stmtno)
	  && (du_next->defsetno == defno))
         || (du_follow && !du_follow->isdef && (du_follow->stmtno == du->stmtno)
	  && (du_follow->defsetno == 0)
	  && xin(inreach[stmttab[du->stmtno].block], defno)))
	    {
	    remove_dead_store(defno);
	    return(YES);
	    }
	}

    return NO;
}  /* check_dead_store */

/*****************************************************************************
 *
 *  CLEANUP_DEF_USE_OPTS()
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

LOCAL void cleanup_def_use_opts()
{
    register long i;

    FREEIT(stmttab);
    FREEIT(definetab);
    FREEIT(varno_to_symtab);

    FREESET_N(inreach);
    FREESET_N(outreach);
    FREESET_N(outreachep);

    for (i = 0; i <= lastfilledsym; ++i)
	{
	if (symtab[i]->an.defset)
	    {
	    FREESET(symtab[i]->an.defset);
	    symtab[i]->an.defset = NULL;
	    }
	symtab[i]->an.cv = NULL;
	symtab[i]->an.du = NULL;
	}

    init_du_storage();
}  /* cleanup_def_use_opts */

/*****************************************************************************
 *
 *  CONSTANT_FOLD()
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
void constant_fold(stmtno)
long stmtno;
{
    NODE *topnode;

    topnode = stmttab[stmtno].np;
    if (topnode->in.op == SEMICOLONOP)
	topnode = topnode->in.right;
    else
	topnode = topnode->in.left;
    walkf(topnode, node_constant_fold);

    /* Do constant branch folding */
    if (topnode->in.op == CBRANCH)
	{
	cbranch_fold(stmtno, topnode);
	}
#if 0
    else if (topnode->in.op == FCOMPGOTO)
	{
        long bn;

	while ((bn = pop_unreached_block_from_stack()) >= 0)
	    {
	    delete_unreached_block(bn);
	    }
	}
#endif
}  /* constant_fold */

/*****************************************************************************
 *
 *  NEED_CONST_CONV()
 *
 *  Description:
 *     A constant is about to be 'propagated'.  Check to see if the constant
 *     needs to be converted first.  This would be the case in instances
 *     such as:
 *   	 shortvar = 32768;
 *
 *  Called by:
 *    constant_propagation
 *
 *  Input Parameters:
 *    np points to an ASSIGN node with an ICON right hand operand
 *
 *  Output Parameters:
 *    The content of the ICON node is truncated if necessary. 
 *
 *  Globals Referenced:
 *
 *  Globals Modified:
 *
 *  External Calls:
 *
 *****************************************************************************
 */
LOCAL void need_const_conv(np)
NODE *np;
  {
  if ((np->in.right->in.op == ICON) && (np->in.op == ASSIGN))
    switch ( np->in.left->in.type.base )
      {
      case SHORT:
        np->in.right->tn.lval = (short) np->in.right->tn.lval;
        break;

      case CHAR:
        np->in.right->tn.lval = (char) np->in.right->tn.lval;
        break;

      case UCHAR:
        np->in.right->tn.lval = (uchar) np->in.right->tn.lval;
        break;

      case USHORT:
        np->in.right->tn.lval = (ushort) np->in.right->tn.lval;
        break;

      default:
        /* No other types require conversion */ ;
      }
  }

/*****************************************************************************
 *
 *  CONSTANT_PROPAGATION()
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

LOCAL void constant_propagation(defno, usedu, usenp, useparent)
long defno;
DUBLOCK *usedu;
NODE *usenp;
NODE *useparent;
{
    HASHU *sp;
    register DUBLOCK *du;
    register DUBLOCK *dunext;
    register DUBLOCK *constdu;
    TTYPE type_temp;

    /* There is (as of 10/31) a case where a constant is propagated to
     * a UNARY MUL node.  FORTRAN parameters are the case that causes
     * this.  Check for this case and free the node below the UNARY MUL
     * node. (rel6.5 will be defined for the 6.5 release so this change
     * will not be included until 7.0.  There are companion changes to
     * 'isvalueuse' and 'local_copy_propagation' (#$%).)
     */
    if (usenp->in.op == UNARY MUL)
      usenp->in.left->in.op = FREE;

    need_const_conv(definetab[defno].np);

    type_temp = usenp->in.type;
    *usenp = *(definetab[defno].np->in.right);
    usenp->in.type = type_temp;

    if (usedu->hides_maybe_use)
      {
      /* A definite use is to be removed but it hides a non-definite use.
       * simply change the definite use into a non definite use.
       */
      usedu->isdefinite = NO;
      usedu->hides_maybe_use = NO;
      }
    else
      {
      usedu->deleted = YES;
      definetab[defno].nuses--;
      }

    if ((useparent->in.op == SCONV) && ISFTP(useparent->in.type)
     && (usenp->in.op == ICON))
	{
	char buff[10];

#ifdef COUNTING
	_n_int_const_prop++;
#endif

	emit_real_const(pseudolbl, useparent);

	usenp->in.op = FREE;
	useparent->in.op = NAME;
	sprintf(buff, "L%d", pseudolbl);
	useparent->atn.name = addtreeasciz(buff);
	useparent->tn.lval = 0;
	useparent->tn.rval = 0;
	sp = symtab[find(useparent)];
	sp->an.isconst = YES;
	sp->an.type.base = BTYPE(useparent->tn.type);
	sp->an.ptr = NO;
	sp->an.func = NO;
	sp->an.varno = 0;	/* no def-use info */
	pseudolbl++;
	}
    else
	{
        /* If the constant is a static NAME kind, then we must create a DUBLOCK
         * and insert it in the appropriate place.
         */
        if (usenp->in.op == NAME)
	    {

#ifdef COUNTING
	    _n_real_const_prop++;
#endif
            sp = symtab[find(usenp)];
	    constdu = alloc_dublock();
	    *constdu = *usedu;
	    constdu->deleted = NO;

	    du = sp->an.du;
	    do {
	        dunext = du->next;
	        if ((dunext->stmtno > constdu->stmtno)
	         || (dunext->stmtno == -(lastep + 1)))	/* exit use */
		    {
		    du->next = constdu;
		    constdu->next = dunext;
		    goto inserted;
		    }
	        du = dunext;
	        } while (du != sp->an.du);
	    constdu->next = du->next;
	    du->next = constdu;
inserted:
	    if ((sp->an.du->stmtno != -(lastep + 1))
	     && (sp->an.du->stmtno <= constdu->stmtno))
	        sp->an.du = constdu;		/* always point to last item */
	    }

#ifdef COUNTING
	else
	    {
	    _n_int_const_prop++;
	    }
#endif
	}

}  /* constant_propagation */

/*****************************************************************************
 *
 *  DELETE_REGION()
 *
 *  Description:	A conditional branch has been folded.  A backward
 *			branch is eliminated as a result.  Eliminate the
 *			region that has indicated block as its backward
 *			branch.
 *
 *  Called by:	cbranch_fold()
 *
 *  Input Parameters:	bn - The block number of the backward branch.
 *
 *  Output Parameters:  none
 *
 *  Globals Referenced: lastregioninfo
 *			region
 *			regioninfo
 *
 *  Globals Modified:
 *			region
 *			regioninfo
 *
 *  External Calls:
 *
 *****************************************************************************
 */
LOCAL void delete_region(bn)
int bn;
{
  int i;

  for (i = lastregioninfo; i >= 0; --i)
    if (region[i] && regioninfo[i].backbranch.srcblock == bn)
      {
      FREESET(region[i]);
      region[i] = NULL;
      break; 
      }
} /* delete_region */

/*****************************************************************************
 *
 *  DELETE_UNREACHED_BLOCK()
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

void delete_unreached_block(bn)
long bn;
{
    BBLOCK *bp;
    BBLOCK *bp_target;
    CGU *cgup;
    register long i;
    long last_stmt;
    register NODE *np;
    long bn_target;

#ifdef COUNTING
    _n_unreached_blocks_deleted++;
#endif

    bp = dfo[bn];
    bp->bb.deleted = YES;

    /* remove this block from other's predecessor sets */
    switch (bp->bb.breakop)
	{
	case CBRANCH:
		bp_target = bp->bb.rbp;
		bn_target = bp_target->bb.node_position;
		delelement(bn, predset[bn_target], predset[bn_target]);
		delelement(bn_target, succset[bn], succset[bn]);
		if ((isemptyset(predset[bn_target])  /* empty or only pred is self */
		  || ((nextel(-1,predset[bn_target]) == bn_target)
                   && (nextel(bn_target,predset[bn_target]) == -1)))
		 && !bp_target->bb.deleted
		 && (bp_target->bb.entries == NULL))
		    push_unreached_block_on_stack(bn_target);
		bp->bb.rbp = NULL;
		/* fall through */

	case FREE:
	case LGOTO:
		bp_target = bp->bb.lbp;
		bn_target = bp_target->bb.node_position;
		delelement(bn, predset[bn_target], predset[bn_target]);
		delelement(bn_target, succset[bn], succset[bn]);
		if ((isemptyset(predset[bn_target])  /* empty or only pred is self */
		  || ((nextel(-1,predset[bn_target]) == bn_target)
                   && (nextel(bn_target,predset[bn_target]) == -1)))
		 && !bp_target->bb.deleted
		 && (bp_target->bb.entries == NULL))
		    push_unreached_block_on_stack(bn_target);
		bp->bb.lbp = NULL;
		break;

	case FCOMPGOTO:
	case GOTO:
		cgup = bp->cg.ll;
		for (i = 0; i < bp->cg.nlabs; ++i)
		    {
		    bp_target = cgup[i].lp->bp;
		    bn_target = bp_target->bb.node_position;
		    delelement(bn, predset[bn_target], predset[bn_target]);
		    delelement(bn_target, succset[bn], succset[bn]);
		    if ((isemptyset(predset[bn_target])  /* empty or only pred is self */
		      || ((nextel(-1,predset[bn_target]) == bn_target)
                       && (nextel(bn_target,predset[bn_target]) == -1)))
		     && !bp_target->bb.deleted
		     && (bp_target->bb.entries == NULL))
		        push_unreached_block_on_stack(bn_target);
		    }
		FREEIT(cgup);
		break;

	case EXITBRANCH:
		werror("Possible infinite loop detected.");
		break;
	}

    /* Fix up def-use info for all statements in this block */
    pass1_collect_attributes = NO;
    simple_def_proc = remove_def;
    simple_use_proc = remove_use;
    remove_use_tmp_set = new_set(maxdefine);	/* used only by remove_use() */
    remove_use_def_vector = (long *)ckalloc((maxdefine + 1) * sizeof(long));
						/* used only by remove_use() */
    i = bp->bb.firststmt;
    for (last_stmt = i + bp->bb.nstmts - 1; i <= last_stmt; ++i)
	{
	rm_stmtno = i;
	np = stmttab[i].np;
	if (np->in.op == SEMICOLONOP)
	    np = np->in.right;
	else
	    np = np->in.left;
	traverse_stmt_1(np, NULL);
	stmttab[i].deleted = YES;
	}
    FREEIT(remove_use_def_vector);
    FREESET(remove_use_tmp_set);

    /* Free the tree */
    if (bp->bb.treep)
	{
        tfree(bp->bb.treep);
        bp->bb.treep = NULL;
	}
    bp->bb.breakop = FREE;
    bp->bb.lbp = bp + 1;

    /* If this was the origination of a region backbranch, the region no
	longer exists
     */
    for (i = lastregioninfo; i >= 0; --i)
	{
	if (region[i] && regioninfo[i].backbranch.srcblock == bn)
	    {
	    FREESET(region[i]);
	    region[i] = NULL;
	    break;
	    }
	}
    
}  /* delete_unreached_block */

/*****************************************************************************
 *
 *  DO_DEF_USE_OPTS()
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

LOCAL void do_def_use_opts(callno)
long callno;
{
    register long defno;
    register struct deftab *dp;
    flag changing;
    register NODE *np;
    long niterations;
    HASHU *sp;
    int find_loc;

    unreached_block_stack = (long *)ckalloc(max_unreached_block * sizeof(long));
    last_unreached_block = -1;

    initialize_definetab();

    fold_constant_cbranches();	/* transform any cbranches with constant
				 * condition to GOTO's.
				 */

    if (needregioninfo || (callno == 2))
	analyze_regions(callno);

    niterations = 0;
    changing = YES;

    while ((changing == YES) && (niterations < 5))
	{
	niterations++;
	changing = NO;

#		ifdef DEBUGGING
		if (qddebug)
		    dump_definetab();
#		endif DEBUGGING

        /* go through all definite defs with = op */
        for (defno = 1; defno <= lastdefine; ++defno)
	    {
	    dp = definetab + defno;
	    if (!dp->deleted)
	        {

		/* check for constant propagation */
		if (dp->OKforconstprop && !dp->deleted
		 && ((((np = dp->np->in.right)->in.op == ICON)
		   && (np->atn.name == NULL))
		  || ((np->in.op == NAME)
		   && ((find_loc = find(np)) != (unsigned) (-1))
		   && ((sp = symtab[find_loc])->an.isconst)
		   && (sp->an.varno > 0))))
		    {
		    changing |= check_const_prop(defno);
		    }

		/* check for dead store removal */
		if (dp->OKfordeadstore && !dp->deleted)
		    {
		    changing |= check_dead_store(defno);
		    }

		/* check for local copy propagation */
		if (dp->OKforcopyprop && !dp->deleted && (dp->nuses == 1)
		 && ((((np = dp->np->in.right)->in.op != ICON)
		   || (np->atn.name != NULL))
		  && ((np->in.op != NAME)
		   || ((find_loc = find(np)) == (unsigned) (-1)) 
		   || (!symtab[find_loc]->an.isconst))))
		    {
		    changing |= check_copy_prop(defno);
		    }  /* if */
	        }  /* if */
	    }  /* for defno */

/*	
 *	Delete loops only during the first call to do_def_use_opts().
 *	Coelescing blocks can merge the contents of blocks that make
 *	up a loop.  This makes it unrecognizable to the code that
 *	deletes a loop.
 */
	if ( (callno == 1) && delete_empty_unreached_loops() )
		changing = YES;

	}  /* while */

    do_region_transforms(callno);

    FREEIT(unreached_block_stack);

}  /* do_def_use_opts */

/*****************************************************************************
 *
 *  FOLD_CONSTANT_CBRANCHES()
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

LOCAL void fold_constant_cbranches()
{
    register long i;
    register BBLOCK *bp;
    register NODE *np;

    for (i = 0; i < numblocks; ++i)
	{
	bp = dfo[i];
	if (!bp->bb.deleted && bp->bb.breakop == CBRANCH)
	    {
	    np = bp->bb.treep;
	    np = (np->in.op == SEMICOLONOP) ? np->in.right : np->in.left;
	    cbranch_fold(bp->bb.treep->nn.stmtno, np);
	    }
	}
}  /* fold_constant_cbranches */

/*****************************************************************************
 *
 *  GLOBAL_DEF_USE_OPTS()
 *
 *  Description:
 *
 *  Called by:
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:
 *
 *  Globals Modified:
 *
 *  External Calls:
 *
 *****************************************************************************
 */

void global_def_use_opts(callno)
long callno;
{
    in_reg_alloc_flag = NO;	/* read by number_vars() and lots of routines
				 * called by reg_alloc_first_pass()
				 */

    number_vars();		/* (register.c)  Assign numbers to candidates
				 * for copy propagation and dead store removal
				 */

#		ifdef DEBUGGING
		if ((qddebug > 1) || (qudebug > 1))
		    dump_varnos();
#		endif DEBUGGING

    reg_alloc_first_pass();	/* (regdefuse.c)  Collect def-use info, build
				 * reaching defs gen and kill sets
				 */

#		ifdef DEBUGGING
		if (qtdebug)
		    dumpblockpool(1,1);
		if ((qddebug > 1) | (qudebug > 1))
		    dumpsymtab();
		if (qddebug > 1)
		    dump_definetab();
		if (qtdebug)
		    dump_stmttab();
		if (qddebug > 2)
		    dump_gen_and_kill();
#		endif DEBUGGING

    array_forms_fixed = YES;

    compute_reaching_defs();	/* (register.c)  Reaching defs DFA */

#		ifdef DEBUGGING
		if (qddebug > 2)
		    dump_reaching_defs();
		if (qddebug | qudebug)
		    dumpsymtab();
#		endif DEBUGGING

    do_def_use_opts(callno);  /* Do the def-use opts -- global constant
				 * propagation, local copy propagation, and
				 * dead store removal
				 */

    cleanup_def_use_opts();	/* Free data structures */

#		ifdef DEBUGGING
		if (qtdebug)
		    {
		    fprintf(debugp, "\nBlocks after global_def_use_opts()\n");
		    dumpblockpool(1,1);
		    }
#		endif DEBUGGING

/*    if (callno == 1)
*/
	array_forms_fixed = NO;

}  /* global_def_use_opts */

/*****************************************************************************
 *
 *  INITIALIZE_DEFINETAB()
 *
 *  Description:	Walk through all entries in the definetab, setting
 *			the "nuses", "deleted" and "hassideeffects" fields
 *			to appropriate values.
 *
 *  Called by:		global_def_use_opts()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			lastdefine
 *			stmttab
 *
 *  Globals Modified:	definetab
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void initialize_definetab()
{
    {
    register DEFTAB *dp;
    register DEFTAB *dpend;
    register flag isassignstmt;
    register flag ismultidef;
    
    for (dp = definetab + 1, dpend = definetab + lastdefine; dp <= dpend; ++dp)
	{
	dp->nuses = 0;
	dp->deleted = NO;
	dp->hassideeffects = stmttab[dp->stmtno].hassideeffects;
	dp->allusesindefstmt = YES;

	isassignstmt = (dp->np->in.op == ASSIGN);
	if (dp > definetab+1)
	  ismultidef = ((dp-1)->stmtno == dp->stmtno);
	else
	  ismultidef = NO;
	if (dp < dpend)
	  ismultidef = ismultidef || ((dp+1)->stmtno == dp->stmtno);

	dp->OKforconstprop = !constprop_disable && dp->isdefinite
			  && isassignstmt;
	dp->OKfordeadstore = !deadstore_disable && !dp->hassideeffects
			  && isassignstmt && !ismultidef;
	dp->OKforcopyprop = !copyprop_disable && dp->isdefinite
			  && isassignstmt && !ismultidef;
	}
    }

    /* Go through all defs and uses and increment the nuses count for each def.
     * In each use, set the firstdefduinblock field to point to the first
     * def-type DU block in the current block.  It is used to reconstruct the
     * set of defs reaching the use.
     */
    {
    long i;
    register SET *defset;
    HASHU *sp;
    register DUBLOCK *du;
    register DUBLOCK *duend;
    long currblock;
    register long defno;
    long *elem_vector;
    register long *curr_elem;

    defset = new_set(maxdefine);
    elem_vector = (long *) ckalloc((maxdefine + 1) * sizeof(long));

    for (i = 0; i <= lastfilledsym; ++i)
	{
	if ((du = (sp = symtab[i])->an.du) != NULL)
	    {
	    if (sp->an.defset == NULL)
		sp->an.defset = new_set(maxdefine);

	    du = du->next;
	    duend = du;
	    
	    currblock = -1;
	    do {
		du->deleted = NO;

		if ((du->stmtno > 0)		/* normal statement */
		 && !du->deleted)
		    {
		    if (stmttab[du->stmtno].block != currblock)
			{
			currblock = stmttab[du->stmtno].block;
			intersect(sp->an.defset, inreach[currblock], defset);
			}
		    if (du->isdef)
			{
			if (du->isdefinite)
			    clearset(defset);
			adelement(du->defsetno, defset, defset);
			}
		    else	/* use */
			{
			set_elem_vector(defset, elem_vector);
			curr_elem = elem_vector;
			while ((defno = *curr_elem++) != -1)
			    {
			    definetab[defno].nuses++;
			    if (du->stmtno != definetab[defno].stmtno)
				definetab[defno].allusesindefstmt = NO;
			    }
			}
		    }
		else if (du->stmtno == -(lastep + 1) && (exitblockno >= 0))
			/* exit use */
		    {
		    intersect(outreach[exitblockno], sp->an.defset, defset);
		    set_elem_vector(defset, elem_vector);
		    curr_elem = elem_vector;
		    while ((defno = *(curr_elem++)) != -1)
			{
			definetab[defno].nuses++;
			definetab[defno].allusesindefstmt = NO;
			}
		    }
		else		/* entry def -- ignore */
		    ;
		du = du->next;
		} while (du != duend);
	    }
	}
    FREESET(defset);
    FREEIT(elem_vector);
    }
    
}  /* initialize_definetab */

/*****************************************************************************
 *
 *  ISVALUEINSTANCE()
 *
 *  Description: A function returning true if 'np' is a use of the symbol
 *		 table entry 'sp'.
 *
 *  Called by:	isvalueuse
 *
 *  Input Parameters:	sp -  Symbol table entry thought to be in use in 'np'.
 *			np -  Tree node thought to be a use of the symbol
 *			      table entry 'sp'.
 *
 *  Output Parameters: none
 *
 *  Globals Referenced: none
 *
 *  Globals Modified: none
 *
 *  External Calls: none
 *
 *****************************************************************************
 */

LOCAL flag isvalueinstance(sp, np)
HASHU *sp;
NODE *np;
{
    if (((sp->an.tag == A6N) && (np->in.op == OREG)
          && (np->tn.lval == sp->a6n.offset))
     || ((sp->an.tag == AN) && (np->in.op == NAME)
         && (np->atn.lval == sp->an.offset)
         && !strcmp(np->atn.name, sp->an.ap))
/* This change will go into effect after the 6.5 release.  This change
 * allows FORTRAN formal parameter to participate in constant propagation
 * and local copy propagation.  There are companion changes in 
 * 'constant_propagation' and 'local_copy_propagation' (#$%).
 */
     || ((sp->an.farg && fortran) && 
         ((np->in.op == UNARY MUL) && 
          (np->in.left->in.op == OREG) &&
          (np->in.left->tn.lval == sp->a6n.offset)))
                                                    )
	return YES;
    else
	return NO;
}

/*****************************************************************************
 *
 *  ISVALUEUSE()
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

LOCAL flag isvalueuse(sp, du, pnp, pparent, pisleft)
HASHU *sp;
DUBLOCK *du;
NODE **pnp;
NODE **pparent;
long *pisleft;
{
    register NODE *np;
    register NODE *nr;
    flag isleft;

    *pparent = np = du->d.parent;
    switch(np->in.op)
        {
        case CM:
	    nr = np->in.right;
	    if (isvalueinstance(sp,nr))
		{
		*pisleft = NO;
		*pnp = nr;
		return YES;
	        }
		/* fall through */
        case UCM:
	    nr = np->in.left;
	    if (isvalueinstance(sp,nr))
		{
		*pisleft = YES;
		*pnp = nr;
		return YES;
		}
	    else
		return NO;

        case CALL:		/* search for it on args chain */
	    nr = np->in.right;
	    isleft = NO;
	    if (nr->in.op == CM)
		{
nextarg:
		np = nr;
	        nr = np->in.right;
		isleft = NO;
	        if (isvalueinstance(sp,nr))
		    {
		    *pparent = np;
		    *pisleft = isleft;
		    *pnp = nr;
		    return YES;
	            }
		nr = np->in.left;
		isleft = YES;
		if (nr->in.op == CM)
		    goto nextarg;
		}

	    if (nr->in.op == UCM)
		{
		np = nr;
	        nr = np->in.left;
		isleft = YES;
		}

	    if (isvalueinstance(sp,nr))
		{
		*pparent = np;
		*pisleft = isleft;
		*pnp = nr;
		return YES;
	        }
	    else
		return NO;
		
        case UNARY CALL:
	    return NO;		/* too hard, do nothing */

	case ASSIGN:
	    nr = np->in.right;
	    if (isvalueinstance(sp,nr))
		{
		*pisleft = NO;
		*pnp = nr;
		return YES;
		}
	    else
		return NO;

        default:
	    nr = np->in.left;
	    if (isvalueinstance(sp,nr))
		{
		*pisleft = YES;
		*pnp = nr;
		return YES;
		}

	    if (optype(np->in.op) != BITYPE)
		return NO;

	    nr = np->in.right;
	    if (isvalueinstance(sp,nr))
		{
		*pisleft = NO;
		*pnp = nr;
		return YES;
		}
	    else
		return NO;
        }  /* switch */
}  /* isvalueuse */

/*****************************************************************************
 *
 *  LCP_CHECK_USE()
 *
 *  Description:	lcp_check_use() is called by the routines in
 *			regdefuse.c through the function variable "simple_use".
 *			lcp_check_use() checks that the variable with the
 *			use doesn't have any definitions between
 *			lcp_source_stmtno and lcp_target_stmtno.
 *
 *  Called by:
 *
 *  Input Parameters:	sp -- symtab pointer of item being used
 *			parent -- parent tree node
 *			usetype -- MEMORY or REGISTER
 *			isdefinite -- is this a "definite" use?
 *			hidden_use -- a definite hides a "maybe" use?
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			lcp_OK
 *			lcp_source_stmtno
 *			lcp_target_stmtno
 *			stmttab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *
 *****************************************************************************
 */

LOCAL void lcp_check_use(sp, parent, usetype, isdefinite, hidden_use)
HASHU *sp;
NODE *parent;
long usetype;
long isdefinite;
long hidden_use;
{
    register DUBLOCK *du;
    register DUBLOCK *duend;
    long targetblock;
    DUBLOCK *first;
    NODE *np;

    if (!lcp_OK)
	return;		/* problem already found */

    if (ISPTR(sp->an.type)	/* may be pointer indirect -- bail out */
     || (sp->an.varno == 0))	/* unknown var -- bail out */
	{
	lcp_OK = NO;
	return;
	}

    /* ignore memory uses of constants */
    if (sp->an.isconst && (usetype == MEMORY))
	return;

    targetblock = stmttab[lcp_target_stmtno].block;

    if ((du = sp->an.du) == NULL)
#pragma BBA_IGNORE
	cerror ("empty DU block chain in lcp_check_use()");
    du = du->next;
    duend = du;

    /* look through DU blocks for source variable.  This is complicated
       because the "parent" field in the DU block may not be correct due
       to constant propagation and array form re-writing.  So, we look for
       an exact match.  If we don't find it, we take the first DU block
       for the statement satisfying all other criteria.
     */
    do {
	if ((du->stmtno == lcp_source_stmtno) && (!du->isdef))
	    {
	    first = du;
	    do {
	        if ((parent == du->d.parent) && !du->isdef)
		    goto found;
	        else if (du->stmtno != lcp_source_stmtno)
		    {
		    du = first;
		    goto found;
		    }
		du = du->next;
		} while (du != duend);
	    du = first;

found:
	    do {
	        du = du->next;
	        if (du == duend)  /* cycled through rest of blocks, OK */
		    return;
		else if (!du->deleted)	/* skip deleted blocks */
		    {
		    if (stmttab[du->stmtno].block == targetblock)
			{
			if (du->stmtno <= lcp_target_stmtno)
			    {
			    if (du->isdef)
				{
				if (du->stmtno == lcp_target_stmtno)
				  {
				  /* OK if you have: x = y+1; y = x */
				  np = stmttab[lcp_target_stmtno].np;
				  np = (np->in.op == SEMICOLONOP) ? np->in.right
								  : np->in.left;
				  if (du->d.parent ==  np)
				    return;
				  }
				lcp_OK = NO;	/* bad news, set the flag */
				return;
				}
			    }
			else	/* after target stmt ==> no defs */
			    return;
			}
		    else	/* out of block ==> no defs */
			return;
		    }
	        } while (1);
	    }
	du = du->next;
	} while (du != duend);
    cerror ("DU block not found in lcp_check_use()");
}  /* lcp_check_use */

/*****************************************************************************
 *
 *  LOCAL_COPY_PROPAGATION()
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

LOCAL void local_copy_propagation(defdu, usedu, useparent, targetisleftchild)
DUBLOCK *defdu;
DUBLOCK *usedu;
NODE *useparent;
long targetisleftchild;
{
    long defno;
    NODE *np;
    NODE *definenp;
    NODE *npold;
    NODE *makety();

#ifdef COUNTING
    _n_copy_prop++;
#endif

    defno = defdu->defsetno;
    definenp = definetab[defno].np;
    npold = definenp->in.right;
    if (targetisleftchild)
        np = makety(npold, useparent->in.left->tn.type);
    else
        np = makety(npold, useparent->in.right->tn.type);

    /* move DU USE blocks from def statement to target statement */
    lcp_source_stmtno = defdu->stmtno;
    lcp_target_stmtno = usedu->stmtno;
    lcp_parent_node = (np != npold) ? np : usedu->d.parent;
    pass1_collect_attributes = NO;
    simple_def_proc = move_def_error;
    simple_use_proc = move_use;

#		ifdef DEBUGGING
		if (qpdebug)
		    fprintf(debugp,
		     "\nlocal_copy_propagation called -- defno=%d usestmt=%d\n",			defno, lcp_target_stmtno);
#		endif DEBUGGING

    traverse_stmt_1(npold, definenp);

    /* move tree from def statement to target statement */
    if (targetisleftchild)
	{
    /* There is (as of 10/31) a case where a copy is propagated to
     * a UNARY MUL node.  FORTRAN parameters are the case that causes
     * this.  Check for this case and free the node below the UNARY MUL
     * node. (rel6.5 will be defined for the 6.5 release so this change
     * will not be included until 7.0.  There are companion changes to
     * 'isvalueuse' and 'local_copy_propagation' (#$%).)
     */
	if (useparent->in.left->in.op == UNARY MUL)
	  useparent->in.left->in.left->in.op = FREE;
	useparent->in.left->in.op = FREE;
	useparent->in.left = np;
	}
    else
	{
	if (useparent->in.right->in.op == UNARY MUL)
	  useparent->in.right->in.left->in.op = FREE;
	useparent->in.right->in.op = FREE;
	useparent->in.right = np;
	}

    if ((useparent->in.op == UNARY MUL) && 
        (np->in.arrayref || np->in.structref) )
      {
      useparent->in.arrayref = np->in.arrayref;
      useparent->in.arrayrefno = np->in.arrayrefno; 
      useparent->in.structref = np->in.structref;
      useparent->in.isarrayform = NO;
      useparent->in.isptrindir = NO;
      }

    /* delete source statement */
    if (definenp->in.left->in.op == UNARY MUL)
	  definenp->in.left->in.left->in.op = FREE;
    definenp->in.left->in.op = FREE;
    definenp->in.op = NOCODEOP;

    defdu->deleted = YES;
    usedu->deleted = YES;
    definetab[defno].deleted = YES;

    stmttab[lcp_source_stmtno].deleted = YES;
}  /* local_copy_propagation */

/*****************************************************************************
 *
 *  MOVE_DEF_ERROR()
 *
 *  Description:	move_def_error() is an error routine called by routines
 *			in regdefuse.c through the function variable
 *			"simple_def_proc".  No defs should be seen while 
 *			traversing for local_copy_propagation.  If any are,
 *			this routine is called.
 *
 *  Called by:
 *
 *  Input Parameters:	sp -- symtab pointer of item being defined
 *			parent -- tree node of defining op
 *			deftype -- MEMORY or REGISTER
 *			isdefinite -- is this a "definite" def???
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *
 *****************************************************************************
 */

LOCAL void move_def_error(sp, parent, deftype, isdefinite)
register HASHU *sp;
NODE *parent;
long deftype;
long isdefinite;
{
if (sp->an.varno == 0 )
    return;
else
#pragma BBA_IGNORE
    cerror ("move_def_error() called");
}  /* move_def_error */

/*****************************************************************************
 *
 *  MOVE_USE()
 *
 *  Description:	move_use() is called by the routines in regdefuse.c
 *			through the function variable "simple_use".
 *			move_use() finds the DUBLOCK and moves it from
 *			lcp_source_stmtno to lcp_target_stmtno.
 *
 *  Called by:
 *
 *  Input Parameters:	sp -- symtab pointer of item being used
 *			parent -- parent tree node
 *			usetype -- MEMORY or REGISTER
 *			isdefinite -- is this a "definite" use?
 *			hidden_use -- a definite hides a "maybe" use?
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			lcp_parent_node
 *			lcp_source_stmtno
 *			lcp_target_stmtno
 *			stmttab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *
 *****************************************************************************
 */

LOCAL void move_use(sp, parent, usetype, isdefinite, hidden_use)
HASHU *sp;
NODE *parent;
long usetype;
long isdefinite;
long hidden_use;
{
    register DUBLOCK *du;
    register DUBLOCK *duend;
    register DUBLOCK *dunext;
    register DUBLOCK *dufollow;
    DUBLOCK *first;
    DUBLOCK *firstfollow;
    long targetblock = stmttab[lcp_target_stmtno].block;

    /* varno == 0 means no def-use info -- ignore */
    if (sp->an.varno == 0)
	return;

    /* ignore memory uses of constants */
    if (sp->an.isconst && (usetype == MEMORY))
	return;

    if ((du = sp->an.du) == NULL)
#pragma BBA_IGNORE
	cerror ("empty DU block chain in move_use()");
    dufollow = du;
    du = du->next;
    duend = du;

     /* look through DU blocks for source variable.  This is complicated
       because the "parent" field in the DU block may not be correct due
       to constant propagation and array form re-writing.  So, we look for
       an exact match.  If we don't find it, we take the first DU block
       for the statement satisfying all other criteria.
     */

    do {
	if ((du->stmtno == lcp_source_stmtno) && (!du->isdef))
	    {
	    first = du;
	    firstfollow = dufollow;
	    do {
	        if ((parent == du->d.parent) && !du->isdef)
		    goto found;
	        else if (du->stmtno != lcp_source_stmtno)
		    {
		    du = first;
		    dufollow = firstfollow;
		    goto found;
		    }
		dufollow = du;
		du = du->next;
		} while (du != duend);
	    du = first;
	    dufollow = firstfollow;

found:
	    dunext = du->next;
	    if ((dunext == duend) || (dunext->stmtno >= lcp_target_stmtno)
	     || (stmttab[dunext->stmtno].block != targetblock))
		/* Don't move block */
		;
	    else if (dufollow == dunext)	/* special 2-block case */
		{
		sp->an.du = du;
		}
	    else  /* block needs to be moved */
		{
		if (duend == du)
		  duend = dunext;
		dufollow->next = dunext;
		do {
		    dufollow = dunext;
		    dunext = dunext->next;
		  } while ((dunext != duend)
			 && (dunext->stmtno < lcp_target_stmtno)
			 && (stmttab[dunext->stmtno].block == targetblock));
		/* insert du block here */
		du->next = dunext;
		dufollow->next = du;
		if (dunext == duend)
		    sp->an.du = du;
		}

	    du->stmtno = lcp_target_stmtno;
	    if (parent->in.op == ASSIGN)
		du->d.parent = lcp_parent_node;
	    return;
	    }
	dufollow = du;
	du = du->next;
	} while (du != duend);
    cerror ("DU block not found in move_use()");
}  /* move_use */

/*****************************************************************************
 *
 *  POP_UNREACHED_BLOCK_FROM_STACK()
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

long pop_unreached_block_from_stack()
{
    if (last_unreached_block >= 0)
	return(unreached_block_stack[last_unreached_block--]);
    else
	return(-1);
}  /* pop_unreached_block_from_stack */

/*****************************************************************************
 *
 *  PUSH_UNREACHED_BLOCK_ON_STACK()
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

void push_unreached_block_on_stack(bn)
long bn;
{
    register long i;

    /* Don't add if already in the stack */
    for (i = 0; i <= last_unreached_block; ++i)
	{
	if (unreached_block_stack[i] == bn)
	    return;
	}

    /* Expand stack if necessary */
    if (++last_unreached_block >= max_unreached_block)
	{
	max_unreached_block <<= 1;	/* double size */
	unreached_block_stack = (long *)ckrealloc(unreached_block_stack,
		max_unreached_block * sizeof(long));
	}

    /* Add the block */
    unreached_block_stack[last_unreached_block] = bn;

}  /* push_unreached_block_on_stack */

/*****************************************************************************
 *
 *  REMOVE_DEAD_STORE()
 *
 *  Description:	Remove the dead store corresponding to define # "defno".
 *			Update def-use info accordingly.
 *			Delete statement.
 *
 *  Called by:		do_def_use_opts()
 *
 *  Input Parameters:	defno -- definetab index of corresponding definition
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			remove_def
 *			remove_use
 *			rm_stmtno
 *			stmttab
 *
 *  Globals Modified:	pass1_collect_attributes
 *			remove_use_tmp_set
 *			rm_stmtno
 *			simple_def_proc
 *			simple_use_proc
 *
 *  External Calls:	FREEIT()
 *			new_set()
 *			tfree()
 *			traverse_stmt_1()	-- regdefuse.c
 *
 *****************************************************************************
 */

void remove_dead_store(defno)
long defno;
{
    NODE *np;		/* top node of actual code tree */

#ifdef COUNTING
    _n_dead_stores_removed++;
#endif

    rm_stmtno = definetab[defno].stmtno;

#		ifdef DEBUGGING
		if (qsdebug)
		    fprintf(debugp,
			"\nremove_dead_store() called.  defno=%d, stmtno=%d\n",
			defno, rm_stmtno);
#		endif DEBUGGING

    /* Set flags and routine names for tree traversal */
    pass1_collect_attributes = NO;
    simple_def_proc = remove_def;
    simple_use_proc = remove_use;
    remove_use_tmp_set = new_set(maxdefine);  /* used only by remove_use() */
    remove_use_def_vector = (long *)ckalloc((maxdefine + 1) * sizeof(long));

    np = definetab[defno].np;

    /* update all defuse info and decrement nuses counts of reaching defs */
    traverse_stmt_1(np, NULL);

    FREEIT(remove_use_def_vector);
    FREESET(remove_use_tmp_set);

#		ifdef DEBUGGING
		if (qsdebug && qddebug)
		    {
		    fprintf(debugp, "After remove_dead_store:\n");
		    dump_definetab();
		    }
#		endif DEBUGGING

    /* free statement tree */
    tfree(np->in.left);
    tfree(np->in.right);
    np->in.op = NOCODEOP;

#		ifdef DEBUGGING
		if ((qsdebug > 1) && qtdebug)
		    {
		    fprintf(debugp, "Block after remove_dead_store:\n");
		    fwalk(dfo[stmttab[rm_stmtno].block]->l.treep, eprint, 0);
		    }
#		endif DEBUGGING

    stmttab[rm_stmtno].deleted = YES;
}  /* remove_dead_store */

/*****************************************************************************
 *
 *  REMOVE_DEF()
 *
 *  Description:	remove_def() is called by the routines in regdefuse.c
 *			through the function variable "simple_def_proc".
 *			It flags the def as "deleted" in the definetab.
 *			remove_def() also finds the DUBLOCK and flags it
 *			as deleted, too.
 *
 *  Called by:
 *
 *  Input Parameters:	sp -- symtab pointer of item being defined
 *			parent -- tree node of defining op
 *			deftype -- MEMORY or REGISTER
 *			isdefinite -- is this a "definite" def???
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	rm_stmtno
 *
 *  Globals Modified:	definetab
 *
 *  External Calls:	cerror()
 *
 *****************************************************************************
 */

LOCAL void remove_def(sp, parent, deftype, isdefinite)
register HASHU *sp;
NODE *parent;
long deftype;
long isdefinite;
{
    register DUBLOCK *du;
    register DUBLOCK *duend;
    DUBLOCK *first;

    if (sp->an.varno == 0)    /* no def-use info */
	return;

    if ((du = sp->an.du) == NULL)
#pragma BBA_IGNORE
	cerror ("empty DU block chain in remove_def()");
    du = du->next;
    duend = du;

     /* look through DU blocks for def.  This is complicated
       because the "parent" field in the DU block may not be correct due
       to constant propagation and array form re-writing.  So, we look for
       an exact match.  If we don't find it, we take the first DU block
       for the statement satisfying all other criteria.
     */

    do {
	if (!du->deleted && du->isdef && (du->stmtno == rm_stmtno))
	    {
	    first = du;
	    do {
	        if ((parent == du->d.parent) && !du->deleted && du->isdef)
		    goto found;
	        else if (du->stmtno != rm_stmtno)
		    {
		    du = first;
		    goto found;
		    }
		du = du->next;
		} while (du != duend);
	    du = first;

found:
	    du->deleted = YES;
	    definetab[du->defsetno].deleted = YES;
	    return;
	    }
	du = du->next;
	} while (du != duend);

    if (isdefinite)
#pragma BBA_IGNORE
	cerror ("DU block not found in remove_def()");
}  /* remove_def */

/*****************************************************************************
 *
 *  REMOVE_USE()
 *
 *  Description:	remove_use() is called by the routines in regdefuse.c
 *			through the function variable "simple_use".
 *			remove_use() also finds the DUBLOCK and flags it
 *			as deleted, and decrements the nuses count of every
 *			reaching def.
 *
 *  Called by:
 *
 *  Input Parameters:	sp -- symtab pointer of item being used
 *			parent -- parent tree node
 *			usetype -- MEMORY or REGISTER
 *			isdefinite -- is this a "definite" use?
 *			hidden_use -- a definite hides a "maybe" use?
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			inreach[]
 *			remove_use_tmp_set
 *			rm_stmtno
 *			stmttab
 *
 *  Globals Modified:	definetab
 *			remove_use_tmp_set
 *
 *  External Calls:	FREEIT()
 *			adelement()
 *			cerror()
 *			clearset()
 *			intersect()
 *			new_set()
 *			nextel()
 *
 *****************************************************************************
 */

LOCAL void remove_use(sp, parent, usetype, isdefinite, hidden_use)
register HASHU *sp;
NODE *parent;
long usetype;
long isdefinite;
long hidden_use;
{
    register DUBLOCK *du;
    register DUBLOCK *duend;
    long blockno;
    register long defno;
    DUBLOCK *first;
    register long *pdef;

    if (sp->an.varno == 0)    /* no def-use info */
	return;

    /* ignore memory uses of constants */
    if (sp->an.isconst && (usetype == MEMORY))
	return;

    blockno = stmttab[rm_stmtno].block;

    /* by default, all var's defs reaching block reach the use */
    intersect(inreach[blockno], sp->an.defset, remove_use_tmp_set);

    if ((du = sp->an.du) == NULL)
#pragma BBA_IGNORE
	cerror ("empty DU block chain in remove_use()");
    du = du->next;
    duend = du;

    do {
	if ((du->stmtno > 0) && (stmttab[du->stmtno].block == blockno))
	    {
	    /* keep a running record of reaching defs within the block */
	    if (du->isdef)
		{
		if (du->isdefinite)
		    clearset(remove_use_tmp_set);

		adelement(du->defsetno, remove_use_tmp_set,
			  remove_use_tmp_set);
		}

	     /* look through DU blocks for use.  This is complicated
		because the "parent" field in the DU block may not be correct
		due to constant propagation and array form re-writing.  So, we 
		look for an exact match.  If we don't find it, we take the
		first DU block for the statement satisfying all other criteria.
	     */

	    /* process the use */
	    if (!du->isdef && (du->stmtno == rm_stmtno) && !du->deleted)
	        {
	        first = du;
	        do {
	            if ((parent == du->d.parent) && !du->deleted && !du->isdef)
		        goto found;
	            else if (du->stmtno != rm_stmtno)
		        {
		        du = first;
		        goto found;
		        }
		    du = du->next;
		    } while (du != duend);
	        du = first;

found:
	        du->deleted = YES;
		set_elem_vector(remove_use_tmp_set, remove_use_def_vector);
		pdef = remove_use_def_vector;

		/* decrement nuses count for all reaching defs */
		while ((defno = *(pdef++)) > 0)
		    {
		    if (!definetab[defno].deleted)
			definetab[defno].nuses--;
		    }

	        return;
		}
	    }
	du = du->next;
	} while (du != duend);
    if (isdefinite)
#pragma BBA_IGNORE
	cerror ("DU block not found in remove_use()");
}  /* remove_use */
