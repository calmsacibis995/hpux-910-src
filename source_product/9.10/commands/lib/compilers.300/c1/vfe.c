/* file vfe.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)vfe.c	16.1 90/11/05 */

# include "c1.h"

/* procedure defined in this file */
LOCAL void	add_thunk_vars_to_call();
LOCAL void	add_thunk_vars_to_composite();
      void	adjust_vfe_block_pointers();
      void	adjust_vfe_label_pointers();
      NODE	*delete_stmt();
      long	find_or_add_vfe();
LOCAL void	make_thunk_vars_list();
LOCAL void	process_thunk_var();
      void	process_vfes();
LOCAL NODE	*replace_stmt();

/* definitions */

flag	vfes_to_emit;   /* 1 == non-empty vfes to emit */
struct vfethunk *vfe_thunks;
VFEREF	*vfe_anon_refs;         /* refs to vfes in assigned FMTS */
long    last_vfe_thunk;         /* index into vfe_thunks table */
long    curr_vfe_thunk;         /* index into vfe_thunks table */
long	max_vfes;		/* size of vfe_thunks table */
flag	non_empty_vfe_seen;	/* 1 == non-empty vfe seen */

LOCAL ushort *composite_thunk_vars;
LOCAL long max_composite_thunk_var;
LOCAL long last_composite_thunk_var;
LOCAL ushort *thunk_vars_list;
LOCAL long max_thunk_var;
LOCAL long last_thunk_var;

/*****************************************************************************
 *
 *  ADD_THUNK_VARS_TO_CALL()
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
LOCAL void add_thunk_vars_to_call(np, thunkvars, nthunkvars)
NODE *np;
ushort *thunkvars;
long nthunkvars;
{
    HIDDENVARS *hiddenvars;
    long totalitems;

    if (np->in.hiddenvars)
	totalitems = np->in.hiddenvars->nitems + nthunkvars;
    else
	totalitems = nthunkvars;

    hiddenvars = (HIDDENVARS *) ckalloc((totalitems + 1) * sizeof(ushort));
    hiddenvars->nitems = totalitems;
    if (np->in.hiddenvars)
	{
	memcpy(&(hiddenvars->var_index[0]), &(np->in.hiddenvars->var_index[0]),
		np->in.hiddenvars->nitems * sizeof(ushort));
	memcpy(&(hiddenvars->var_index[np->in.hiddenvars->nitems]), thunkvars,
		nthunkvars * sizeof(ushort));
        FREEIT(np->in.hiddenvars);
	}
    else
	{
	memcpy(&(hiddenvars->var_index[0]), thunkvars,
		nthunkvars * sizeof(ushort));
	}
    np->in.hiddenvars = hiddenvars;
}  /* add_thunk_vars_to_call */

/*****************************************************************************
 *
 *  ADD_THUNK_VARS_TO_COMPOSITE()
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

LOCAL void add_thunk_vars_to_composite(vp)
struct vfethunk *vp;
{
    register long ci;
    register long vi;
    register long index;

    if (!vp->thunkvars)
	return;

    for (vi = vp->nthunkvars - 1; vi >= 0; --vi)
	{
	index = vp->thunkvars[vi];
	for (ci = 0; ci <= last_composite_thunk_var; ++ci)
	    {
	    if (composite_thunk_vars[ci] == index)
		goto nextthunkvar;
	    }
	if (++last_composite_thunk_var >= max_composite_thunk_var)
	    {
	    max_composite_thunk_var <<= 2;	/* double size */
	    composite_thunk_vars = (ushort *) ckrealloc(composite_thunk_vars,
			max_composite_thunk_var * sizeof(ushort));
	    }
	composite_thunk_vars[last_composite_thunk_var] = index;
nextthunkvar:
	;
	}
}  /* add_thunk_vars_to_composite */

/*****************************************************************************
 *
 *  ADJUST_VFE_BLOCK_POINTERS() 
 * 
 *  Description:	The basic block table has been reallocated.  The
 *			variable format expression list contains absolute
 *			basic block pointers that must be updated.
 *
 *  Called by:		mkblock
 *
 *  Input Parameters:	delta -- (newtop - oldtop)
 *
 *  Output Parameters:
 *
 *  Globals Referenced: last_vfe_thunk
 *			vfe_thunks[]
 *
 *  Globals Modified: 	vfe_thunks[]
 *
 *  External Calls:
 *
 *****************************************************************************
 */

void adjust_vfe_block_pointers(delta) int delta;
{
    long vi;

    for (vi = 0; vi <= last_vfe_thunk; vi++)
	vfe_thunks[vi].startblock = (BBLOCK*)((int)vfe_thunks[vi].startblock 
								       + delta);
}


/*****************************************************************************
 *
 *  ADJUST_VFE_LABEL_POINTERS() 
 * 
 *  Description:	The label block table has been reallocated.  The
 *			variable format expression list contains absolute
 *			label block pointers that must be updated.
 *
 *  Called by:		enterlblock
 *
 *  Input Parameters:	delta -- (newtop - oldtop)
 *
 *  Output Parameters:
 *
 *  Globals Referenced: last_vfe_thunk
 *			vfe_thunks[]
 *
 *  Globals Modified: 	vfe_thunks[]
 *
 *  External Calls:
 *
 *****************************************************************************
 */

void adjust_vfe_label_pointers(delta) int delta;
{
    long vi;

    for (vi = 0; vi <= last_vfe_thunk; vi++)
	vfe_thunks[vi].startlabel = (LBLOCK*)((int)vfe_thunks[vi].startlabel 
								       + delta);
}

/*****************************************************************************
 *
 *  DELETE_STMT()
 *
 *  Description:	delete the statement tree with top node "np" from the
 *			tree with head pointer "headp".  Return the new headp.
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

NODE *delete_stmt(headp, np)
NODE *headp;
register NODE *np;
{
    register NODE *currp;
    register NODE *prevp;
    NODE *newheadp;

    newheadp = headp;

    for (prevp = (NODE *) NULL, currp = headp;
	 currp;
	 prevp = currp, currp = currp->in.left)
	{
	if ((currp->in.op == SEMICOLONOP) && (currp->in.right == np))
	    {
	    if (currp == headp)
		{
		newheadp = currp->in.left;
		}
	    else
		{
		prevp->in.left = currp->in.left;
		}
	    currp->in.op = UNARY SEMICOLONOP;
	    currp->in.left = currp->in.right;
	    tfree(currp);
	    break;
	    }
	else if ((currp->in.op == UNARY SEMICOLONOP) && (currp->in.left == np))
	    {
	    if (currp == headp)
		{
		newheadp = (NODE *) NULL;
		}
	    else
		{
		prevp->in.op = UNARY SEMICOLONOP;
		prevp->in.left = prevp->in.right;
		}
	    tfree(currp);
	    break;
	    }
	}

    return (newheadp);
}

/*****************************************************************************
 *
 *  FIND_OR_ADD_VFE()
 *
 *  Description:	look for variable expression format name in thunk
 *			table.  add it if not found.  return index of entry.
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

long find_or_add_vfe(name)
register char *name;
{
	register struct vfethunk *end;
	register struct vfethunk *v;

	for (v = vfe_thunks, end = &(vfe_thunks[last_vfe_thunk]);
		v <= end; v++)
	    {
	    if (!strcmp(v->label, name))
		/* match -- break and do nothing */
		break;
	    }
	if (v > end)
	    {		/* not found -- add it */
	    ++last_vfe_thunk;
	    if (last_vfe_thunk >= max_vfes)
		/* expand table */
		{
		max_vfes += VFE_TABLE_SIZE;
		vfe_thunks = (struct vfethunk *) ckrealloc(vfe_thunks,
					max_vfes * sizeof(struct vfethunk));
		}
	    vfe_thunks[last_vfe_thunk].label = addtreeasciz(name);
	    vfe_thunks[last_vfe_thunk].refs = (VFEREF *) NULL;
	    curr_vfe_thunk = last_vfe_thunk;
	    }
	else
	    {
	    curr_vfe_thunk = v - vfe_thunks;
	    }
	return(curr_vfe_thunk);
}

/*****************************************************************************
 *
 *  MAKE_THUNK_VARS_LIST()
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

LOCAL void make_thunk_vars_list(vp)
struct vfethunk *vp;
{
    register VFECODE *vcp;

    pass1_collect_attributes = NO;
    in_reg_alloc_flag = NO;
    simple_def_proc = process_thunk_var;
    simple_use_proc = process_thunk_var;

    last_thunk_var = -1;

    for (vcp = vp->thunk; vcp; vcp = vcp->next)
	{
	traverse_stmt_1(vcp->np, NULL);
	}

    if (last_thunk_var > -1)
	{
	vp->thunkvars = (ushort*)ckalloc((last_thunk_var + 1) * sizeof(ushort));
	vp->nthunkvars = last_thunk_var + 1;
	memcpy(vp->thunkvars, thunk_vars_list, vp->nthunkvars * sizeof(ushort));
#ifdef COUNTING
	_n_vfe_vars += vp->nthunkvars;
#endif
	}

}  /* make_thunk_vars_list */

/*****************************************************************************
 *
 *  PROCESS_THUNK_VAR()
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

LOCAL void process_thunk_var(sp, parent, deftype, isdefinite, hidden_use)
HASHU *sp;
NODE *parent;
long deftype;
long isdefinite;
long hidden_use;
{
    register long index;
    register long i;

    index = sp->an.symtabindex;

    for (i = 0; i <= last_thunk_var; ++i)
	{
	if (thunk_vars_list[i] == index)
	    return;				/* already in list */
	}

    /* not in list -- add to it */
    if (++last_thunk_var >= max_thunk_var)	/* must expand table first */
	{
	max_thunk_var <<= 2;
	thunk_vars_list = (ushort *) ckrealloc(thunk_vars_list,
				max_thunk_var * sizeof(ushort));
	}

    thunk_vars_list[last_thunk_var] = index;
}  /* process_thunk_var */

/*****************************************************************************
 *
 *  PROCESS_VFES()
 *
 *  Description:	inline vfes where possible
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

void process_vfes()
{
    register struct vfethunk *vp;
    register VFEREF *rp;
    VFEREF *drp;
    register BBLOCK *bp;

    vfes_to_emit = NO;
    thunk_vars_list = (ushort *) NIL;

    if (non_empty_vfe_seen & (vfe_anon_refs != (VFEREF *) NULL))
	{
	/* go through assign table, looking for non-empty vfes */
	CGU *cp;
	char vfelabel[10];
	long lablength;

	max_composite_thunk_var = 100;
	max_thunk_var = 100;
	last_composite_thunk_var = -1;
	composite_thunk_vars =
		(ushort *) ckalloc(max_composite_thunk_var *sizeof(ushort));
	thunk_vars_list = (ushort *) ckalloc(max_thunk_var * sizeof(ushort));

	for (cp = &(asgtp[ficonlabsz - 1]); cp >= asgtp; --cp)
	    {
	    if (cp->nonexec)	/* FORMAT label */
		{
		/* find corresponding vfe -- is it empty?? */
		lablength = sprintf(vfelabel, "_F%d", cp->val);

		for (vp = &(vfe_thunks[last_vfe_thunk]); vp >= vfe_thunks; --vp)
		    {
		    if (!strncmp(vp->label, vfelabel, lablength))
			{
			vp->asg_fmt_target = YES;
			if (vp->thunk)
			    {
			    /* make thunk vars list */
			    make_thunk_vars_list(vp);
			    add_thunk_vars_to_composite(vp);
			    vfes_to_emit = YES;
#ifdef COUNTING
			    _n_anon_vfes++;
#endif
			    }
			break;
			}
		    }
		}
	    }
	}
    else
	composite_thunk_vars = NULL;

    /* Process all anonymous refs */
    for (rp = vfe_anon_refs; rp; drp = rp, rp=rp->next, FREEIT(drp))
	{
	if (vfes_to_emit)
	    {
	    /* Add composite thunk vars list to call node */
	    add_thunk_vars_to_call(rp->np, composite_thunk_vars,
					last_composite_thunk_var + 1);
	    }
	else
	    {
	    bp = topblock + rp->bp;
	    bp->l.treep = delete_stmt(bp->l.treep, rp->np);
#ifdef COUNTING
	    _n_vfe_calls_deleted++;
#endif
	    }
	}

    /* free composite thunk vars list, if any */
    if (composite_thunk_vars)
	{
	FREEIT(composite_thunk_vars);
	composite_thunk_vars = NULL;
	}

    for (vp = &(vfe_thunks[last_vfe_thunk]); vp >= vfe_thunks; vp--)
	{
	if (vp->thunk == (VFECODE *) NULL)	/* empty vfe */
	    {
#ifdef COUNTING
	    _n_empty_vfes++;
#endif
	    for (rp = vp->refs; rp; drp = rp, rp=rp->next, FREEIT(drp))
		{
		bp = topblock + rp->bp;
		bp->l.treep = delete_stmt(bp->l.treep, rp->np);
#ifdef COUNTING
		_n_vfe_calls_deleted++;
#endif
		}
	    }
	else if (vp->nblocks == 1)	/* non-empty vfe */
	    {
#ifdef COUNTING
	    _n_oneblock_vfes++;
#endif
	    for (rp = vp->refs; rp; drp = rp, rp=rp->next, FREEIT(drp))
		{
		bp = topblock + rp->bp;
		bp->l.treep = replace_stmt(bp->l.treep, rp->np, vp->thunk->np);
#ifdef COUNTING
		_n_vfe_calls_replaced++;
#endif
		}
	    if (!vfes_to_emit || !vp->asg_fmt_target)
		{
		tfree(vp->thunk->np);
		FREEIT(vp->thunk);
		}
	    if (vp->thunkvars)
	        FREEIT(vp->thunkvars);
	    }
	else	/* multi-block thunk */
	    {
#ifdef COUNTING
	    _n_multiblock_vfes++;
#endif
	    if (thunk_vars_list == NULL)
		{
		max_thunk_var = 100;
	        thunk_vars_list = (ushort *) ckalloc(max_thunk_var
							* sizeof(ushort));
		}

	    if (!vp->thunkvars)
	        make_thunk_vars_list(vp);

	    /* add thunkvars as hidden vars to all calls */
	    for (rp = vp->refs; rp; drp = rp, rp=rp->next, FREEIT(drp))
		{
	        /* Add thunk vars list to call node */
	        add_thunk_vars_to_call(rp->np, vp->thunkvars, vp->nthunkvars);
		}

	    vfes_to_emit = YES;
	    if (vp->thunkvars)
	        FREEIT(vp->thunkvars);
	    }
	}

    if (thunk_vars_list != NULL)
	FREEIT(thunk_vars_list);
}  /* process_vfes */

/*****************************************************************************
 *
 *  REPLACE_STMT()
 *
 *  Description:	replace the statement tree "np" (in block tree "headp")
 *			with the block tree "tp".  Return the new headp.
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

LOCAL NODE *replace_stmt(headp, np, tp)
NODE *headp;
register NODE *np;
NODE *tp;
{
    register NODE *currp;
    register NODE *prevp;
    NODE *newheadp;
    NODE *newtree;
    NODE *leftstmt;		/* left-most statement in inserted tree */

    newheadp = headp;

    newtree = tcopy(tp);	/* make copy for insertion */

    for (currp = newtree; currp->in.op != UNARY SEMICOLONOP;
	 currp = currp->in.left)
		;
    leftstmt = currp;

    /* find statement to be replaced */
    for (prevp = (NODE *) NULL, currp = headp;
	 currp;
	 prevp = currp, currp = currp->in.left)
	{
	if ((currp->in.op == SEMICOLONOP) && (currp->in.right == np))
	    {
	    if (currp == headp)
		{
		newheadp = newtree;
		}
	    else
		{
		prevp->in.left = newtree;
		}
	    leftstmt->in.op = SEMICOLONOP;
	    leftstmt->in.right = leftstmt->in.left;
	    leftstmt->in.left = currp->in.left;
	    currp->in.op = UNARY SEMICOLONOP;
	    currp->in.left = currp->in.right;
	    tfree(currp);
	    break;
	    }
	else if ((currp->in.op == UNARY SEMICOLONOP) && (currp->in.left == np))
	    {
	    if (currp == headp)
		{
		newheadp = newtree;
		}
	    else
		{
		prevp->in.left = newtree;
		}
	    tfree(currp);
	    break;
	    }
	}

    return (newheadp);
}  /* replace_stmt */
