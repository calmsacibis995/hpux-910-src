/* file regdefuse.c */
/* @(#) $Revision: 70.4 $ */
/* KLEENIX_ID @(#)regdefuse.c	20.9 92/03/24 */


/*****************************************************************************
 *
 *  File regdefuse.c contains routines for traversing the statement trees
 *  to collect def-use information.
 *
 *****************************************************************************
 */


#include "c1.h"

/* Routines defined in this file */

LOCAL void add_array_element_memory_defuses();
LOCAL void add_array_elements_to_callvars();
LOCAL void add_common_to_callvars();
LOCAL void add_fargs_to_callvars();
LOCAL void add_item_to_callvars();
LOCAL void add_ptr_targets_to_callvars();
LOCAL void add_single_common_to_callvars();
LOCAL void process_array_def();
LOCAL void process_array_use();
LOCAL void process_call();
LOCAL void process_call_arg();
LOCAL void process_call_args();
LOCAL void process_call_array_arg();
LOCAL void process_call_hidden_vars();
LOCAL void process_call_ptrindir_arg();
LOCAL void process_call_side_effects();
LOCAL void process_common_def();
LOCAL void process_def();
LOCAL void process_farg_addr_def();
LOCAL void process_farg_def();
LOCAL void process_ptrindir_def();
LOCAL void process_ptrindir_use();
LOCAL void process_use();
LOCAL void recognize_array();
      flag traverse_stmt_1();


/*****************************************************************************
 *
 *  ADD_ARRAY_ELEMENT_MEMORY_DEFUSES()
 *
 *  Description:	add a memory-type use for all specific array elements
 *			associated with the input symtab entry.  Also add
 *			memory uses for 2nd halves of complex items.
 *
 *  Called by:		process_array_use()
 *			process_array_def()
 *
 *  Input Parameters:	sp -- X6N or XN symtab entry
 *			parent -- parent tree node
 *			dodefs -- YES iff uses AND defs
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	(*simple_use_proc)()
 *			(*simple_def_proc)()
 *
 *****************************************************************************
 */

LOCAL void add_array_element_memory_defuses(sp, parent, dodefs, douses)
HASHU *sp;
NODE *parent;
long dodefs;
long douses;
{
    register HASHU *ep;
    register CLINK *c;

    for (c = sp->xn.member; c; c = c->next)
	{
	ep = symtab[c->val];
	if (douses == YES)
	    (*simple_use_proc)(ep, parent, MEMORY, NO, NO);
	if (dodefs == YES)
	    (*simple_def_proc)(ep, parent, MEMORY, NO);
	}

}  /* add_array_element_memory_defuses */

/*****************************************************************************
 *
 *  ADD_ARRAY_ELEMENTS_TO_CALLVARS()
 *
 *  Description:	Add array elements to the vector of items possibly
 *			read or modified by a CALL.
 *
 *  Called by:		process_call_array_arg()
 *			process_call_hidden_vars()
 *
 *  Input Parameters:	callvars -- vector of items affected by CALL
 *			sp -- XN or X6N symtab entry
 *			adddefs -- YES iff adding uses AND defs
 *
 *  Output Parameters:	callvars -- may have items added to vector
 *
 *  Globals Referenced:	symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_item_to_callvars()
 *
 *****************************************************************************
 */

LOCAL void add_array_elements_to_callvars(callvars, sp, adddefs)
CALLDUITEM *callvars;
register HASHU *sp;
long adddefs;
{
    register CLINK *cp;

    for (cp = sp->xn.member; cp; cp = cp->next)
	{
	sp = symtab[cp->val];
	add_item_to_callvars(callvars, sp, (adddefs ? YES : NO), YES,
				 MEMORY, NO, NO);
	}
}  /* add_array_elements_to_callvars */

/*****************************************************************************
 *
 *  ADD_COMMON_TO_CALLVARS()
 *
 *  Description:	Add all items in all COMMON blocks to the list of
 *			items possibly read/written by a CALL.
 *
 *  Called by:		process_call_arg()
 *			process_call_array_arg()
 *			process_call_hidden_vars()
 *			process_call_side_effects()
 *
 *  Input Parameters:	callvars -- vector of items possibly read/written by
 *					this call.
 *			adddefs -- YES iff adding defs
 *			adduses -- YES iff adding uses
 *
 *  Output Parameters:	callvars -- items may be added to vector
 *
 *  Globals Referenced:	comtab[]
 *			in_reg_alloc_flag
 *			lastcom
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_item_to_callvars()
 *
 *****************************************************************************
 */

LOCAL void add_common_to_callvars(callvars, adddefs, adduses)
CALLDUITEM *callvars;
register long adddefs;
register long adduses;
{
    register HASHU *sp;
    register long i;
    register CLINK *cp;

    for (i = 0; i <= lastcom; ++i)
	{
	for (cp = symtab[comtab[i]]->cn.member; cp; cp = cp->next)
	    {
	    sp = symtab[cp->val];
	    if (sp->xn.array && in_reg_alloc_flag)
	        {		/* generate refs for all specific elements */
	        register CLINK *ap;
	        for (ap = sp->xn.member; ap; ap = ap->next)
		    {
		    sp = symtab[ap->val];
	            add_item_to_callvars(callvars, sp,
				             adddefs ? YES : NO,
				             adduses ? YES : NO,
					     MEMORY, NO, NO);
		    }
	        }
	    else
	        {
	        add_item_to_callvars(callvars, sp,
				         adddefs ? YES : NO,
				         adduses ? YES : NO,
					 MEMORY, NO, NO);
	        }
	    }
	}
}  /* add_common_to_callvars */

/*****************************************************************************
 *
 *  ADD_FARGS_TO_CALLVARS()
 *
 *  Description:	All formal arguments are implicitly used/defined by a 
 *			call.  Add each one to the callvars descriptor vector.
 *
 *  Called by:		process_call_arg()
 *			process_call_array_arg()
 *			process_call_hidden_vars()
 *			process_call_side_effects()
 *
 *  Input Parameters:	callvars -- vector of items affected by call
 *			adddefs -- flag indicating whether defs are to be added
 *			adduses -- flag indicating whether uses are to be added
 *
 *  Output Parameters:	callvars -- items may be added
 *
 *  Globals Referenced:	fargtab[]
 *			in_reg_alloc_flag
 *			lastfarg
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_item_to_callvars()
 *
 *****************************************************************************
 */

LOCAL void add_fargs_to_callvars(callvars, adddefs, adduses)
CALLDUITEM *callvars;
register long adddefs;
register long adduses;
{
    register HASHU *sp;
    register long i;

    for (i = 0; i <= lastfarg; ++i)
	{
	sp = symtab[fargtab[i]];
	if (!sp->a6n.array || !in_reg_alloc_flag)
	    add_item_to_callvars(callvars, sp, adddefs ? YES : NO,
				 adduses ? YES : NO, MEMORY, NO, NO);
	}
}  /* add_fargs_to_callvars */

/*****************************************************************************
 *
 *  ADD_ITEM_TO_CALLVARS()
 *
 *  Description:	add variable to call vars list with def/use/reg/memory
 *			attributes as specified.  Only one entry for each
 *			variable should be in the list.
 *
 *  Called by:		add_array_element_to_callvars()
 *			add_common_to_callvars()
 *			add_fargs_to_callvars()
 *			process_call_arg()
 *			process_call_array_arg()
 *			process_call_hidden_vars()
 *
 *  Input Parameters:	callvars -- vector holding items
 *			sp -- pointer to symtab entry of item to be added
 *			def -- 1 iff def of var to be added
 *			use -- 1 iff use of var to be added
 *			reftype -- register or memory-type reference
 *			isdefinitedef -- is it a "definite" def?
 *			isdefiniteuse -- is it a "definite" use?
 *
 *  Output Parameters:	callvars -- items may be added
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void add_item_to_callvars(callvars, sp, def, use, reftype, isdefinitedef,
				isdefiniteuse)
register CALLDUITEM *callvars;
register HASHU *sp;
unsigned def;
unsigned use;
unsigned reftype;
unsigned isdefinitedef;
unsigned isdefiniteuse;
{
    register long i;
    register long lastcallvar;
    register struct call_def_use_item *pcdu;

    /* Search vector to see if it's already there.  If not, add it */
    lastcallvar = callvars->lastitem;
    pcdu = &(callvars->items[0]);
    for (i = 0; i <= lastcallvar; ++i, ++pcdu)
	{
	if (pcdu->sym == sp)
	    break;			 /* found it -- exit loop */
	}
    if (i > lastcallvar)		/* not found -- add it */
	{
	++(callvars->lastitem);
	pcdu->sym = sp;
	pcdu->def = def;
	pcdu->use = use;
	pcdu->ismem = (reftype == MEMORY);
	pcdu->isdefinitedef = isdefinitedef;
	pcdu->isdefiniteuse = isdefiniteuse;
	if ( use && !isdefiniteuse)
	  pcdu->has_non_definite_use = YES;
	else
	  pcdu->has_non_definite_use = NO;
	pcdu->ndefdefs = 0;
	pcdu->ndefuses = 0;
	}
    else				/* found -- modify existing entry */
	{
	pcdu->def |= def;
	pcdu->use |= use;
	pcdu->ismem |= (reftype == MEMORY);
	pcdu->isdefinitedef |= isdefinitedef;
	pcdu->isdefiniteuse |= isdefiniteuse;
	if ( use && !isdefiniteuse)
	  pcdu->has_non_definite_use = YES;
	}
    if (isdefinitedef)
	pcdu->ndefdefs++;
    if (isdefiniteuse)
	pcdu->ndefuses++;
}  /* add_item_to_callvars */

/*****************************************************************************
 *
 *  ADD_PTR_TARGETS_TO_CALLVARS()
 *
 *  Description:	Add all items in the ptrtab to the callvars vector.
 *
 *  Called by:		process_call_side_effects()
 *
 *  Input Parameters:	callvars -- vector of items possibly read/written by
 *					this call
 *			adddefs -- YES iff adding defs
 *			adduses -- YES iff adding uses
 *
 *  Output Parameters:	callvars -- items may be added to vector
 *
 *  Globals Referenced:	lastptr
 *			ptrtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_item_to_callvars()
 *
 *****************************************************************************
 */

LOCAL void add_ptr_targets_to_callvars(callvars, adddefs, adduses)
CALLDUITEM *callvars;
register long adddefs;
register long adduses;
{
    register HASHU *sp;
    register long i;

    for (i = 0; i <= lastptr; ++i)
	{
	sp = symtab[ptrtab[i]];
	if (sp->xn.array && in_reg_alloc_flag)
	    {		/* generate refs for all specific elements */
	    register CLINK *cp;
	    for (cp = sp->xn.member; cp; cp = cp->next)
		{
		sp = symtab[cp->val];
		add_item_to_callvars(callvars, sp,
				     adddefs ? YES : NO,
				     adduses ? YES : NO,
				     MEMORY, NO, NO);
		}
	    }
	else
	    {
	    /* Avoid the "bogus" ptrtab entry */
	    if ((sp->a6n.tag != A6N) || (sp->a6n.offset != 0))
	    add_item_to_callvars(callvars, sp,
				 adddefs ? YES : NO,
				 adduses ? YES : NO,
				 MEMORY, NO, NO);
	    }
	}
}  /* add_ptr_targets_to_callvars */

/*****************************************************************************
 *
 *  ADD_SINGLE_COMMON_TO_CALLVARS()
 *
 *  Description:	Add all items in a single COMMON block to the list of
 *			items possibly read/written by a CALL.
 *
 *  Called by:		process_call_arg()
 *			process_call_array_arg()
 *
 *  Input Parameters:   common_name -- name of COMMON block
 *			callvars -- vector of items possibly read/written by
 *					this call.
 *			adddefs -- YES iff adding defs
 *			adduses -- YES iff adding uses
 *
 *  Output Parameters:	callvars -- items may be added to vector
 *
 *  Globals Referenced:	comtab[]
 *			in_reg_alloc_flag
 *			lastcom
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_item_to_callvars()
 *
 *****************************************************************************
 */

LOCAL void add_single_common_to_callvars(common_name, callvars, adddefs, adduses)
char *common_name;
CALLDUITEM *callvars;
register long adddefs;
register long adduses;
{
    register HASHU *sp;
    register long i;
    register CLINK *cp, *xcp;

    for (i = 0; i <= lastcom; ++i)
	{
	sp = symtab[comtab[i]];
	if (strcmp(sp->an.ap, common_name))
	    continue;
	for (cp = sp->cn.member; cp; cp = cp->next)
	    {
	    sp = symtab[cp->val];
	    if (sp->xn.array && in_reg_alloc_flag)
	        {		/* generate refs for all specific elements */
	        for (xcp = sp->xn.member; xcp; xcp = xcp->next)
		    {
		    sp = symtab[xcp->val];
	            add_item_to_callvars(callvars, sp,
				         adddefs ? YES : NO,
				         adduses ? YES : NO, MEMORY, NO, NO);
		    }
	        }
	    else
	        {
	        add_item_to_callvars(callvars, sp,
				     adddefs ? YES : NO,
				     adduses ? YES : NO, MEMORY, NO, NO);
	        }
	    }
	}
}  /* add_single_common_to_callvars */

/*****************************************************************************
 *
 *  PROCESS_ARRAY_DEF()
 *
 *  Description:	process an array definition.
 *
 *  Called by:		process_def()
 *
 *  Input Parameters:	np -- top of array tree
 *			parent -- defining operator node
 *			dosubtree -- YES iff subtrees haven't been processed
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	assumptions
 *			fargtab[]
 *			in_reg_alloc_flag
 *			lastfarg
 *			laststmtno
 *			pass1_collect_attributes
 *			simple_def_proc
 *			simple_use_proc
 *			symtab
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	add_array_element_memory_defuses()
 *			process_farg_def()
 *			(*simple_def_proc)()
 *			(*simple_use_proc)()
 *			recognize_array()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_array_def(np, parent, dosubtree)
NODE *np;
NODE *parent;
long dosubtree;
{
    HASHU *sp;
    long ref_val;
    long type;
    NODE *subscript_tree;
    NODE *subscript_parent;
    NODE *const_tree;
    HASHU *cse_sp;
    sp = symtab[np->in.arrayrefno];
    
    if ((sp->an.tag == AN) || (sp->an.tag == XN) ||
        (sp->an.tag == CN) || (sp->an.tag == SN))
      saw_global_access = YES;

    if (!fortran && np->tn.arrayelem)
      {
      if (!in_reg_alloc_flag) /* emit a def for the array as a whole */
	{
	sp = symtab[np->in.arrayrefno];
	(*simple_def_proc)(sp, parent, MEMORY, NO);
	}
      goto exit;
      }

    recognize_array(np, &type, &sp, &subscript_tree, &subscript_parent,
			&ref_val, &const_tree, &cse_sp);

    if (sp->an.farg)	/* formal argument array */
	{
	if (cse_sp == NULL)
	    {
	    process_farg_def(sp, parent, REGISTER, dosubtree);
							/* base address use */
	    }
	else
	    {
	    process_farg_def((in_reg_alloc_flag ? NULL : sp), parent, MEMORY,
				dosubtree);
						 /* do stuff for all fargs*/
	    if (dosubtree == TREE_NOT_PROCESSED)
	        (*simple_use_proc)(cse_sp, np, REGISTER, YES, NO);
	    }

   	if ((dosubtree == TREE_NOT_PROCESSED) && (subscript_tree != NULL))
	    traverse_stmt_1(subscript_tree, subscript_parent);
	}
    else 		/* not a formal argument array */
	{
	/* if def of common var and fargs may overlap common and there
 	 * are fargs, then generate a memory-type def for all fargs
	 */
	if (sp->an.common && (!NO_PARM_OVERLAPS) && (lastfarg >= 0))
	    {
	    register long i;
	    register HASHU *fp;

	    for (i = 0; i <= lastfarg; ++i)
		{
		fp = symtab[fargtab[i]];
		if (!in_reg_alloc_flag || !fp->a6n.array)
		    (*simple_def_proc)(fp, parent, MEMORY, NO);
		}
	    }

	if (type == ARB_ELEMENT)
	    {
	    if (!in_reg_alloc_flag)
	        (*simple_def_proc)(sp, parent, MEMORY, NO);

	    if (dosubtree == TREE_NOT_PROCESSED)
		{
	        if (cse_sp == NULL)
		    {
		    if (in_reg_alloc_flag)
		        /* add use of array base address */
	                (*simple_use_proc)(sp, parent, REGISTER, YES, NO);
		    }
	        else
		    {
	            (*simple_use_proc)(cse_sp, np, REGISTER, YES, NO);
		    }
		}

	    if (in_reg_alloc_flag)
		{
	        /* add memory uses and defs for all specific elements */
	        add_array_element_memory_defuses(sp, parent, YES,
			(dosubtree == TREE_NOT_PROCESSED) ? YES : NO);
		}

	    /* process subscript subtree */
	    if ((subscript_tree != NULL) && (dosubtree == TREE_NOT_PROCESSED))
		traverse_stmt_1(subscript_tree, subscript_parent);
	    }
	else if (type == CONST_ELEMENT)
	    {
	    if (in_reg_alloc_flag)
		{
	        if (sp->an.common && (!NO_PARM_OVERLAPS) && (lastfarg >= 0))
		    (*simple_def_proc)(sp, parent, MEMORY, YES);
		else
		    (*simple_def_proc)(sp, parent, REGISTER, YES);
		}
	    else
		{
    		if (!sp->a6n.equiv) 
      		  /* This avoids using a bad value for arrayrefno. */
		  sp = symtab[sp->an.wholearrayno];
		  (*simple_def_proc)(sp, parent, MEMORY, NO);
		}
	    }
	else			/* whole array */
	    {
	    if (in_reg_alloc_flag)
		{
		if (dosubtree == TREE_NOT_PROCESSED)
		    {
	    	    /* add use of array base address */
	            (*simple_use_proc)(sp, parent, REGISTER, YES, NO);
		    }

	        /* add memory uses and defs for all specific elements */
	        add_array_element_memory_defuses(sp, parent, YES,
			(dosubtree == TREE_NOT_PROCESSED) ? YES : NO);
		}
	    else
		{
	        (*simple_def_proc)(sp, parent, MEMORY, NO);
		}
	    }
	}
exit: ;
}  /* process_array_def */

/*****************************************************************************
 *
 *  PROCESS_ARRAY_USE()
 *
 *  Description:	process uses of a whole array, an arbitrary array
 *			element or a specific array element
 *
 *  Called by:		process_use()
 *
 *  Input Parameters:	np -- top of array tree
 *			parent -- node of "use" operator
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	in_reg_alloc_flag
 *			laststmtno
 *			pass1_collect_attributes
 *			simple_def_proc
 *			simple_use_proc
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	add_array_element_memory_defuses()
 *			recognize_array()
 *			(*simple_def_proc)()
 *			(*simple_use_proc)()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_array_use(np, parent)
register NODE *np;
NODE *parent;
{
    HASHU *sp;		/* symbol table entry for array */
    long ref_val;
    long type;
    NODE *subscript_tree;
    NODE *subscript_parent;
    NODE *const_tree;
    HASHU *cse_sp;

    if (!fortran && np->tn.arrayelem)
      {
      if (!in_reg_alloc_flag) /* emit a use for the array as a whole */
        {
	sp = symtab[np->in.arrayrefno];
	(*simple_use_proc)(sp, parent, MEMORY, NO, NO);
        }
      goto exit;
      }

    recognize_array(np, &type, &sp, &subscript_tree, &subscript_parent,
			&ref_val, &const_tree, &cse_sp);

    if (sp->a6n.farg)	/* formal argument array -- don't worry about specific
			 * elements because they can't be allocated to a
			 * register anyway.
			 */
	{
	if (!in_reg_alloc_flag)
	    (*simple_use_proc)(sp, parent, MEMORY, NO, NO);

	if (cse_sp == NULL)
	    {
	    if (in_reg_alloc_flag)
	        (*simple_use_proc)(sp, parent, REGISTER, YES, NO);
	    }
	else
	    {
	    (*simple_use_proc)(cse_sp, np, REGISTER, YES, NO);
	    }

	if (subscript_tree != NULL)
	    traverse_stmt_1(subscript_tree, subscript_parent);
	}
    else		/* not formal argument array */
	{
	if (type == ARB_ELEMENT)
	    {
	    if (! in_reg_alloc_flag)
	        (*simple_use_proc)(sp, parent, MEMORY, NO, NO);

	    if (cse_sp == NULL)
		{
		/* arbitrary array element -- add use for base address */
		if (in_reg_alloc_flag)
	            (*simple_use_proc)(sp, parent, REGISTER, YES, NO);
		}
	    else
		{
	        (*simple_use_proc)(cse_sp, np, REGISTER, YES, NO);
		}

	    /* add memory uses for all specific elements */
	    if (in_reg_alloc_flag)
	        add_array_element_memory_defuses(sp, parent, NO, YES);

	    /* process subscript tree */
	    if (subscript_tree != NULL)
	        traverse_stmt_1(subscript_tree, subscript_parent);
	    }
	else if (type == CONST_ELEMENT)
	    {
	    if (! in_reg_alloc_flag)
		{
		sp = symtab[sp->an.wholearrayno];
		(*simple_use_proc)(sp, parent, MEMORY, NO, NO);
		}
	    else
		{
	        /* add use to array element */
	        (*simple_use_proc)(sp, parent, REGISTER, YES, NO);
		}
	    }
	else			/* whole array */
	    {
	    if (in_reg_alloc_flag)
		{
	        /* add use for array base address */
	        (*simple_use_proc)(sp, parent, REGISTER, YES, NO);

	        /* add memory uses for all specific elements */
	        add_array_element_memory_defuses(sp, parent, NO, YES);
		}
	    else
		{
	        (*simple_use_proc)(sp, parent, MEMORY, NO, NO);
		}
	    }
	}
exit: ;
}  /* process_array_use */

/*****************************************************************************
 *
 *  PROCESS_CALL()
 *
 *  Description:	Process a CALL or UNARY CALL node.  Create DUBLOCKS
 *			for all variables affected by the call.
 *
 *  Called by:		traverse_stmt_1()
 *
 *  Input Parameters:	np -- pointer to CALL node in statement tree
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastfilledsym
 *			simple_def_proc
 *			simple_use_proc
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *			ckalloc()
 *			process_call_args()
 *			process_call_hidden_vars()
 *			process_call_side_effects()
 *			(*simple_def_proc)()
 *			(*simple_use_proc)()
 *
 *****************************************************************************
 */

LOCAL void process_call(np)
NODE *np;
{
    CALLDUITEM *callvars;	/* vector holding vars affected by call */
    register long i;
    register long j;
    register long ntimes;
    register struct call_def_use_item *pcdu;

    /* use the procedure name -- it may be a function var */
    traverse_stmt_1(np->in.left, np);

    /* first allocate the callvars vector */
    callvars = (CALLDUITEM *) ckalloc((lastfilledsym + 1)
					* sizeof(struct call_def_use_item)
				      + sizeof(long));
    callvars->lastitem = -1;

    /* process any hidden vars */
    if (np->in.hiddenvars)
	process_call_hidden_vars(callvars, np);

    /* process call args */
    if (np->in.op != UNARY CALL)
	process_call_args(callvars, np);

    /* process side-effects of CALL -- COMMON or fargs read/written */
    process_call_side_effects(callvars, np);

    /* goto thru callvars vector and generate def/uses for all items */
    for (i = callvars->lastitem, pcdu = &(callvars->items[i]);
	 i >= 0;
	 --i, --pcdu)
	{
	if (pcdu->use)
	    {
	    ntimes = pcdu->ndefuses;
	    if (ntimes == 0)
		ntimes = 1;
	    for (j = 1; j <= ntimes; ++j)
		{
	        (*simple_use_proc)(pcdu->sym, np,
			       pcdu->ismem ? MEMORY : REGISTER,
			       pcdu->isdefiniteuse,
			       pcdu->has_non_definite_use);
		pcdu->has_non_definite_use = NO;
		}
	    }
	if (pcdu->def)
	    {
	    ntimes = pcdu->ndefdefs;
	    if (ntimes == 0)
		ntimes = 1;
	    for (j = 1; j <= ntimes; ++j)
	        (*simple_def_proc)(pcdu->sym, np,
			       pcdu->ismem ? MEMORY : REGISTER,
			       pcdu->isdefinitedef);
	    }
	}

    /* free the callvars vector */
    FREEIT(callvars);

}  /* process_call */

/*****************************************************************************
 *
 *  PROCESS_CALL_ARG()
 *
 *  Description:	Process a single argument to a call, creating DUBLOCKS
 *			for the variable.
 *
 *  Called by:		process_call_args()
 *
 *  Input Parameters:	callvars -- vector of items affected by call
 *			np -- top of argument subtree
 *			call_np -- CALL node
 *
 *  Output Parameters:	callvars -- items may be added to the vector
 *
 *  Globals Referenced:	assumptions
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_common_to_callvars()
 *			add_fargs_to_callvars()
 *			add_item_to_callvars()
 *			add_single_common_to_callvars()
 *			find()
 *			process_call_array_arg()
 *			tfree()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_call_arg(callvars, np, call_np)
CALLDUITEM *callvars;
register NODE *np;
NODE *call_np;
{
    register HASHU *sp;
    register NODE *t;
    HASHU *csp;		/* ptr to symtab entry for 2nd half of complex */
    long find_loc;

again:

    if (np->in.arrayref || np->in.arraybaseaddr || np->in.structref)
	{
	/* This may eliminate the array designation */
	fix_array_form(np);
	}

    if (np->in.arrayref || np->in.arraybaseaddr || np->in.structref)
	{
	process_call_array_arg(callvars, np, call_np);
	return;
	}

    if (np->in.isptrindir)
	{
	process_call_ptrindir_arg(callvars, np);
	return;
	}

    switch (np->in.op)
	{
	case FOREG:
	case OREG:
		find_loc = find(np);
		if (find_loc != (unsigned) (-1))
		  {
		  sp = symtab[find_loc];
		  if ((np->in.op == OREG) && (!sp->a6n.farg || !fortran))
		      /* call-by-value */
		      add_item_to_callvars(callvars, sp, NO, YES, REGISTER,
					       NO, YES);
		  else
		      {
		      add_item_to_callvars(callvars, sp,
				       (call_np->in.no_arg_defs ? NO : YES),
				       (call_np->in.no_arg_uses ? NO : YES),
					  MEMORY, NO, NO);
		      if (sp->a6n.complex1)
			  {
			  csp = symtab[sp->on.back_half];
			  add_item_to_callvars(callvars, csp,
					  (call_np->in.no_arg_defs? NO : YES),
					  (call_np->in.no_arg_uses? NO : YES),
					  MEMORY, NO, NO);
			  }
		      }
		  if (fortran && sp->a6n.farg && !NO_PARM_OVERLAPS)
		      {
		      add_fargs_to_callvars(callvars,
				  (call_np->in.no_arg_defs ? NO : YES),
				  (call_np->in.no_arg_uses ? NO : YES));
		      add_common_to_callvars(callvars,
				  (call_np->in.no_arg_defs ? NO : YES),
				  (call_np->in.no_arg_uses ? NO : YES));
		      }
# ifdef FTN_POINTERS
		  if (fortran && sp->a6n.ptr)
		      add_ptr_targets_to_callvars(callvars,NO,YES);
# endif FTN_POINTERS
		  }
		break;

	case ICON:
	case NAME:
		if (np->atn.name == NULL)
		    break;

		saw_global_access = YES;
		find_loc = find(np);
		if (find_loc != (unsigned) (-1))
		  {
		  sp = symtab[find_loc];
		  add_item_to_callvars(callvars, sp,
				     ((call_np->in.no_arg_defs ||
                                      (np->in.op == NAME)) ? NO : YES),
				     (call_np->in.no_arg_uses ? NO : YES),
				       (np->in.op == NAME) ? REGISTER : MEMORY,
					NO, (np->in.op == NAME) ? YES : NO);
				/* NAME == call-by-value */
		  if ((np->in.op == ICON) && sp->an.complex1)
		      {
		      csp = symtab[sp->on.back_half];
		      add_item_to_callvars(callvars, csp,
				 (call_np->in.no_arg_defs? NO : YES),
				 (call_np->in.no_arg_uses? NO : YES),
						     MEMORY, NO, NO);
		      }

		  if (sp->an.common
		   && ((call_np->fn.side_effects & NO_GLOBAL_REFS)
				  != NO_GLOBAL_REFS))
		      if (!NO_PARM_OVERLAPS)
			  {
			  add_fargs_to_callvars(callvars,
				  (call_np->in.no_arg_defs ? NO : YES),
				  (call_np->in.no_arg_uses ? NO : YES));
			  add_common_to_callvars(callvars,
				  (call_np->in.no_arg_defs ? NO : YES),
				  (call_np->in.no_arg_uses ? NO : YES));
			  }
		      else if (!call_np->in.parm_types_matched)
			  {
			  add_single_common_to_callvars(sp->an.ap, callvars,
				  (call_np->in.no_arg_defs ? NO : YES),
				  (call_np->in.no_arg_uses ? NO : YES));
			  }
# ifdef FTN_POINTERS
		  if (fortran && sp->a6n.ptr)
		      add_ptr_targets_to_callvars(callvars,NO,YES);
# endif FTN_POINTERS
		  }
		break;

	case PLUS:
		if ((np->in.left->in.op != REG)
		 || (np->in.right->in.op != ICON))
		    {
		    traverse_stmt_1(np, NULL);
		    }
		else
		    {
		    t = talloc();
		    t->in.op = OREG;
		    t->tn.lval = np->in.right->tn.lval;
		    find_loc = find(t);
		    if (find_loc != (unsigned) (-1))
		      {
		      sp = symtab[find_loc];
		      add_item_to_callvars(callvars, sp,
			       (call_np->in.no_arg_defs? NO : YES),
			       (call_np->in.no_arg_uses? NO : YES),
					       MEMORY, NO, NO);
		      if (sp->a6n.complex1)
			  {
			  csp = symtab[sp->on.back_half];
			  add_item_to_callvars(callvars, csp,
				   (call_np->in.no_arg_defs? NO : YES),
			           (call_np->in.no_arg_uses? NO : YES),
						       MEMORY, NO, NO);
			  }
		      tfree(t);
		      }
		    }
		break;

	case UNARY MUL:
		/* call-by-value */
		if (np->in.left->in.op == OREG)
		    {
		    find_loc = find(np->in.left);
		    if (find_loc != (unsigned) (-1))
		      {
		      sp = symtab[find_loc];
		      add_item_to_callvars(callvars, sp, NO,
				  (call_np->in.no_arg_uses? NO : YES),
				  REGISTER, NO, YES);
		      }
		    }
		else
		    traverse_stmt_1(np->in.left, np);
		break;

	case COMOP:
		traverse_stmt_1(np->in.left, np);
		np = np->in.right;
		goto again;

	default:
		traverse_stmt_1(np, NULL);
	}
}  /* process_call_arg */

/*****************************************************************************
 *
 *  PROCESS_CALL_ARGS()
 *
 *  Description:	Process argument list in a call, generating defs and
 *			uses for the variables in the list
 *
 *  Called by:		process_call()
 *
 *  Input Parameters:	callvars -- vector of items affected by a call
 *			call_np -- CALL node
 *
 *  Output Parameters:	callvars -- items may be added to the vector
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	process_call_arg()
 *
 *****************************************************************************
 */

LOCAL void process_call_args(callvars, call_np)
CALLDUITEM *callvars;
NODE *call_np;
{
    register NODE *np = call_np->in.right;

    /* process each argument in turn */

    while (np)
	{
        if (np->in.op == CM)
	    {
	    process_call_arg(callvars, np->in.right, call_np);
	    np = np->in.left;
	    }
	if (np->in.op == UCM)
	    np = np->in.left;
	if (np->in.op != CM)
	    {
	    process_call_arg(callvars, np, call_np);
	    np = NULL;
	    }
	}
}  /* process_call_args */

/*****************************************************************************
 *
 *  PROCESS_CALL_ARRAY_ARG()
 *
 *  Description:	Process an array reference in a call argument list.
 *			Generate defs/uses as appropriate.
 *
 *  Called by:		process_call_arg()
 *
 *  Input Parameters:	callvars -- vector of items def'd/use'd by the call
 *			np -- top node of array reference
 *			call_np -- CALL node
 *
 *  Output Parameters:	callvars -- may have items added to vector
 *
 *  Globals Referenced:	assumptions
 *			in_reg_alloc_flag
 *			simple_def_proc
 *			simple_use_proc
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_array_elements_to_callvars()
 *			add_common_to_callvars()
 *			add_fargs_to_callvars()
 *			add_item_to_callvars()
 *			add_single_common_to_callvars()
 *			find()
 *			recognize_array()
 *			(*simple_def_proc)()
 *			(*simple_use_proc)()
 *			talloc()
 *			tfree()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_call_array_arg(callvars, np, call_np)
CALLDUITEM *callvars;
NODE *np;
NODE *call_np;
{
    HASHU *sp;		/* symbol table entry for array */
    long ref_val;
    long type;
    NODE *subscript_tree;
    NODE *subscript_parent;
    NODE *const_tree;
    HASHU *cse_sp;

    sp = symtab[np->in.arrayrefno];
    
    if ((sp->an.tag == AN) || (sp->an.tag == XN) ||
        (sp->an.tag == CN) || (sp->an.tag == SN))
      saw_global_access = YES;

    if (!fortran && np->tn.arrayelem)
      {
      if (!in_reg_alloc_flag) 
	{
	sp = symtab[np->in.arrayrefno];
	add_item_to_callvars(callvars, sp,
		(call_np->in.no_arg_defs ? NO : YES),
		(call_np->in.no_arg_uses ? NO : YES),
		MEMORY, NO, NO);
	}
      return;
      }

    recognize_array(np, &type, &sp, &subscript_tree, &subscript_parent,
			&ref_val, &const_tree, &cse_sp);

    /* if it's a farg, we simply gen a use for the array base address,
     * evaluate the subscripts, and do any side-effect processing.
     */
    if (sp->a6n.farg)
	{

	if (! in_reg_alloc_flag)
	    /* implicit use and def of array */
	    add_item_to_callvars(callvars, sp,
			(call_np->in.no_arg_defs ? NO : YES),
			(call_np->in.no_arg_uses ? NO : YES),
			MEMORY, NO, NO);

	if (cse_sp == NULL)
	    {
	    /* add REGISTER use of base address */
	    if (in_reg_alloc_flag)
	        add_item_to_callvars(callvars, sp, NO, YES, REGISTER, NO, YES);
	    }
	else
	    {
	    add_item_to_callvars(callvars, cse_sp, NO, YES, REGISTER, NO, YES);
	    }
	
	/* evaluate the subscripts */
	if (subscript_tree != NULL)
	    traverse_stmt_1(subscript_tree, subscript_parent);

	/* process any side effects */
	if (!NO_PARM_OVERLAPS)
	    {
	    add_fargs_to_callvars(callvars,
				(call_np->in.no_arg_defs? NO : YES),
				(call_np->in.no_arg_uses? NO : YES));
	    add_common_to_callvars(callvars,
				(call_np->in.no_arg_defs? NO : YES),
				(call_np->in.no_arg_uses? NO : YES));
	    }
	}
    else	/* not an farg */
	{
	if (sp->x6n.common	   /* it's in a common block */
	 && !NO_PARM_OVERLAPS)
	    add_fargs_to_callvars(callvars,
				(call_np->in.no_arg_defs? NO : YES),
				(call_np->in.no_arg_uses? NO : YES));
		
	if (type == CONST_ELEMENT)
	    {
	    if (in_reg_alloc_flag)
		{
	    	add_item_to_callvars(callvars, sp,
				  (call_np->in.no_arg_defs ? NO : YES),
				  (call_np->in.no_arg_uses ? NO : YES),
				     (ref_val == VALUE ? REGISTER : MEMORY),
				     NO, (ref_val == VALUE ? YES : NO));
		if ((sp->a6n.complex1) && (ref_val == REFERENCE))
		    {
		    HASHU *ssp;
		    ssp = symtab[sp->on.back_half];
	    	    add_item_to_callvars(callvars, ssp,
				 (call_np->in.no_arg_defs ? NO : YES),
				 (call_np->in.no_arg_uses ? NO : YES),
				MEMORY, NO, NO);
		    }
		}
	    else	/* dead store gen/kill */
		{
		sp = symtab[sp->a6n.wholearrayno];
	    	add_item_to_callvars(callvars, sp,
				(call_np->in.no_arg_defs ? NO : YES),
				(call_np->in.no_arg_uses ? NO : YES),
				MEMORY, NO, NO);
		}
	    if (!call_np->in.parm_types_matched)
		{
		if (sp->an.common)
		    add_single_common_to_callvars(sp->an.ap, callvars,
				(call_np->in.no_arg_defs ? NO : YES),
				(call_np->in.no_arg_uses ? NO : YES));
		else if (in_reg_alloc_flag)
	    	    add_array_elements_to_callvars(callvars,
					symtab[sp->a6n.wholearrayno],
					(call_np->in.no_arg_defs? NO : YES));
		}
	    }
	else if (type == WHOLE_ARRAY)
	    {
	    if (in_reg_alloc_flag)
	        /* generate REG use for base addr and memory def/use for
	         * specific elements
	         */
		add_item_to_callvars(callvars, sp, NO, YES, REGISTER, NO, YES);
	    else
		add_item_to_callvars(callvars, sp,
					(call_np->in.no_arg_defs? NO : YES),
					(call_np->in.no_arg_uses? NO : YES),
					MEMORY, NO, NO);
	    if (!call_np->in.parm_types_matched && sp->an.common)
	        add_single_common_to_callvars(sp->an.ap, callvars,
				(call_np->in.no_arg_defs ? NO : YES),
				(call_np->in.no_arg_uses ? NO : YES));
	    else if (in_reg_alloc_flag)
	        add_array_elements_to_callvars(callvars, sp,
					(call_np->in.no_arg_defs? NO : YES));
	    }
	else 			/* arbitrary element */
	    {
	    if (! in_reg_alloc_flag)
		add_item_to_callvars(callvars, sp, 
		          ((call_np->in.no_arg_defs || (ref_val == VALUE))?
				NO : YES),
			  (call_np->in.no_arg_uses ? NO : YES),
			  MEMORY, NO, NO);

	    if (cse_sp == NULL)
		{
	        /* add register use for base address */
	        if (in_reg_alloc_flag)
		    add_item_to_callvars(callvars, sp, NO, YES, REGISTER, NO,
					 YES);
		}
	    else
		{
		add_item_to_callvars(callvars, cse_sp, NO, YES, REGISTER,
					 NO, YES);
		}

	    /* add memory def-uses for all specific elements */
	    if (!call_np->in.parm_types_matched && sp->an.common
	     && (ref_val == REFERENCE))
	        add_single_common_to_callvars(sp->an.ap, callvars,
				(call_np->in.no_arg_defs ? NO : YES),
				(call_np->in.no_arg_uses ? NO : YES));
	    else if (in_reg_alloc_flag)
	        add_array_elements_to_callvars(callvars, sp,
		          ((call_np->in.no_arg_defs || (ref_val == VALUE))?
				NO : YES));

	    /* process subscript tree */
	    if (subscript_tree != NULL)
		traverse_stmt_1(subscript_tree, subscript_parent);
	    }
	}
}  /* process_call_array_arg */

/*****************************************************************************
 *
 *  PROCESS_CALL_HIDDEN_VARS()
 *
 *  Description:	Process hidden vars in call, generate defs/uses for
 *			them
 *
 *  Called by:		process_call()
 *
 *  Input Parameters:	callvars -- vector of items affected by the call
 *			call_np -- CALL node
 *
 *  Output Parameters:	callvars -- items may be added to the vector
 *
 *  Globals Referenced:	assumptions
 *			in_reg_alloc_flag
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_array_elements_to_callvars()
 *			add_common_to_callvars()
 *			add_fargs_to_callvars()
 *			add_item_to_callvars()
 *
 *****************************************************************************
 */

LOCAL void process_call_hidden_vars(callvars, call_np)
CALLDUITEM *callvars;
NODE *call_np;
{
    register long i;
    register long nitems;
    register HASHU *sp;
    register ushort *array;
    HASHU *csp;

    nitems = call_np->in.hiddenvars->nitems;
    array = call_np->in.hiddenvars->var_index;
    for (i = 0; i < nitems; ++i)
	{
	sp = symtab[*(array++)];
	if ((sp->an.tag == XN) || (sp->a6n.tag == X6N))
	    {
	    if (in_reg_alloc_flag)
	        /* add memory def/uses for all specific elements */
	        add_array_elements_to_callvars(callvars, sp, YES);
	    else
	        add_item_to_callvars(callvars, sp, YES, YES, MEMORY, NO, NO);
	    }
	else
	    {
	    add_item_to_callvars(callvars, sp, YES, YES, MEMORY, NO, NO);
	    if (sp->a6n.complex1)
		{
		csp = symtab[sp->on.back_half];
		add_item_to_callvars(callvars, csp, YES, YES, MEMORY, NO, NO);
		}
	    }
	if (!NO_PARM_OVERLAPS)
	    {
	    if (sp->a6n.farg)
		{
		add_fargs_to_callvars(callvars, YES, YES);
		add_common_to_callvars(callvars, YES, YES);
		}
	    else if (sp->an.common)
		{
		add_fargs_to_callvars(callvars, YES, YES);
		}
	    }
	}
}  /* process_call_hidden_vars */

/*****************************************************************************
 *
 *  PROCESS_CALL_PTRINDIR_ARG()
 *
 *  Description:	Process call argument which is a pointer indirection.
 *
 *  Called by:		process_call_arg()
 *
 *  Input Parameters:	callvars -- vector of items affected by call
 *			np -- pointer to CALL tree node
 *
 *  Output Parameters:	callvars -- items may be added to the vector
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_ptr_targets_to_callvars()
 *			cerror()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_call_ptrindir_arg(callvars, np)
CALLDUITEM *callvars;
register NODE *np;
{
    if (np->in.op != UNARY MUL)
	cerror("op != UNARY MUL in process_call_ptrindir_arg");
    if (np->in.isarrayform && ! array_forms_fixed)
	fix_array_form(np);
    traverse_stmt_1(np->in.left, np);
    add_ptr_targets_to_callvars(callvars, NO, YES);
}  /* process_call_ptrindir_arg */

/*****************************************************************************
 *
 *  PROCESS_CALL_SIDE_EFFECTS()
 *
 *  Description:	Process any side-effects associated with a call.
 *			This may mean generating defs and/or uses for fargs
 *			and COMMON variables.
 *
 *  Called by:		process_call()
 *
 *  Input Parameters:	callvars -- vector of items affected by call
 *			np -- pointer to CALL tree node
 *
 *  Output Parameters:	callvars -- items may be added to the vector
 *
 *  Globals Referenced:	assumptions
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_common_to_callvars()
 *			add_fargs_to_callvars()
 *
 *****************************************************************************
 */

LOCAL void process_call_side_effects(callvars, np)
CALLDUITEM *callvars;
NODE *np;
{
    if (fortran)
	{
        if (!np->in.no_com_uses || !np->in.no_com_defs)
	    {
	    add_common_to_callvars(callvars, (np->in.no_com_defs ? NO : YES),
				(np->in.no_com_uses ? NO : YES));
	    if (!NO_PARM_OVERLAPS)
	        add_fargs_to_callvars(callvars, (np->in.no_com_defs ? NO : YES),
				(np->in.no_com_uses ? NO : YES));
            /* Add all static non-constants 
               (non-COMMON for FORTRAN) to the list */
            add_ptr_targets_to_callvars(callvars,
                                        (np->in.no_com_defs ? NO : YES),
				        (np->in.no_com_uses ? NO : YES));
	    }
	}
    else /* !fortran */
        /* Add all static non-constants (non-COMMON for FORTRAN) to the list */
        add_ptr_targets_to_callvars(callvars, YES, YES);
}  /* process_call_side_effects */

/*****************************************************************************
 *
 *  PROCESS_COMMON_DEF()
 *
 *  Description:	Process the definition of a variable in COMMON
 *
 *  Called by:		process_def()
 *
 *  Input Parameters:	sp -- symtab entry of COMMON var
 *			parent -- tree node containing defining operator
 *			deftype -- REGISTER- or MEMORY-type def
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: assumptions
 *			fargtab[]
 *			in_reg_alloc_flag
 *			lastfarg
 *			laststmtno
 *			pass1_collect_attributes
 *			simple_def_proc
 *			simple_use_proc
 *			symtab
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	(*simple_def_proc)()
 *			(*simple_use_proc)()
 *
 *****************************************************************************
 */

LOCAL void process_common_def(sp, parent, deftype, dosubtree)
HASHU *sp;
NODE *parent;
long deftype;
long dosubtree;
{
    register long i;
    register HASHU *fp;

    if (!NO_PARM_OVERLAPS && (lastfarg >= 0))
	{
	/* with COMMON-farg overlap, we always keep memory consistent with
	 * register contents.  Hence, we treat this def as a memory def.
	 */
	(*simple_def_proc)(sp, parent, MEMORY, YES);

	for (i = lastfarg; i >= 0; --i)
	    {
	    fp = symtab[fargtab[i]];
	    if (!in_reg_alloc_flag)
	        (*simple_def_proc)(fp, parent, MEMORY, NO);
	    else if (!fp->a6n.array)
		{
		if (dosubtree == TREE_NOT_PROCESSED)
	            (*simple_use_proc)(fp, parent, MEMORY, NO, NO);
	        (*simple_def_proc)(fp, parent, MEMORY, NO);
		}
	    }
	}
    else	/* no COMMON-farg overlap -- treat like any simple variable */
	{
	(*simple_def_proc)(sp, parent, deftype, YES);
	}
}  /* process_common_def */

/*****************************************************************************
 *
 *  PROCESS_DEF()
 *
 *  Description:	process a "def" of a variable
 *
 *  Called by:		traverse_stmt_1()
 *
 *  Input Parameters:	np -- tree node of variable/array being def'd
 *			parent -- tree node containing defining operator
 *			dosubtree -- should subtrees be descended, too?
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: in_reg_alloc_flag
 *			laststmtno
 *			pass1_collect_attributes
 *			simple_def_proc
 *			simple_use_proc
 *			symtab
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	find()
 *			process_array_def()
 *			process_common_def()
 *			process_farg_addr_def()
 *			process_farg_def()
 *			(*simple_def_proc)()
 *			(*simple_use_proc)()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_def(np, parent, dosubtree)
register NODE *np;
NODE *parent;
long dosubtree;
{
    register HASHU *sp;
    long find_loc;

    if (np->in.arrayref || np->in.arraybaseaddr
     || (np->in.isarrayform && !np->in.isptrindir))
	{
	/* This may eliminate the array designation */
	fix_array_form(np);
	}

    if (np->in.arrayref || np->in.arraybaseaddr
     || (np->in.isarrayform && !np->in.isptrindir))
	{
	process_array_def(np, parent, dosubtree);
	}
    else if (np->in.isptrindir)
	{
	process_ptrindir_def(np, parent, dosubtree);
	}
    else if (fortran && (np->in.op == UNARY MUL) && (np->in.left->in.op == OREG))
	{
	sp = symtab[find(np->in.left)];
	if (sp->an.farg)
	    process_farg_def(sp, parent, REGISTER, dosubtree);
	else if (dosubtree == TREE_NOT_PROCESSED)
	    (*simple_use_proc)(sp,np,REGISTER,YES,NO); /* indirection thru ptr*/
	}
    else
	{
        switch (optype(np->in.op))
	    {
	    case BITYPE:
		if (dosubtree == TREE_NOT_PROCESSED)
		    {
		    traverse_stmt_1(np->in.left, np);
		    traverse_stmt_1(np->in.right, np);
		    }
		break;

	    case UTYPE:
		if (dosubtree == TREE_NOT_PROCESSED)
		    traverse_stmt_1(np->in.left, np);
		break;

	    case LTYPE:
		find_loc = find(np);
		if (find_loc != (unsigned) (-1))
		  {
		  sp = symtab[find_loc];
		  if (fortran && (sp->a6n.farg || sp->a6n.hiddenfarg
		    || (sp->a6n.array && symtab[sp->a6n.wholearrayno]->a6n.farg)
		    || (sp->a6n.isstruct
		     && symtab[sp->a6n.wholearrayno]->a6n.farg)))
		      {
		      if (in_reg_alloc_flag)
		          process_farg_addr_def(sp, np, parent);
		      }
		  else if (sp->a6n.common)
		      process_common_def(sp, parent, REGISTER, dosubtree);
		  else
		      (*simple_def_proc)(sp, parent, REGISTER, YES);
		  }
		if ((np->in.op == NAME) || (np->in.op == ICON))
		  saw_global_access = YES;
		break;
	    }
	}

    return;
}  /* process_def */

/*****************************************************************************
 *
 *  PROCESS_FARG_ADDR_DEF()
 *
 *  Description:	Process the move of a formal argument from calling
 *			routine space to local variable space.
 *
 *  Called by:		process_def()
 *
 *  Input Parameters:	sp -- symtab entry of local var entry for var.
 *			np -- tree node of variable
 *			parent -- tree node containing defining operator
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	laststmtno
 *			simple_def_proc
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_plink()
 *			(*simple_def_proc)()
 *
 *****************************************************************************
 */

LOCAL void process_farg_addr_def(sp, np, parent)
register HASHU *sp;
NODE *np;
NODE *parent;
{
    register PLINK *pp;
    long slot;
    long old_max_slot;

    if (sp->a6n.array)
	{
	/* only record farg addr definition in symtab entry */
	sp = symtab[sp->a6n.wholearrayno];
	pp = alloc_plink();
	pp->val = laststmtno;
	pp->next = (PLINK *) (sp->x6n.cv);
	((PLINK *) (sp->x6n.cv)) = pp;

    	np->in.arrayref = YES;
	np->in.arrayrefno = sp->x6n.symtabindex;

	/* record register def if appropriate */
	(*simple_def_proc)(sp, parent, REGISTER, YES);
	}
    else
	{
	/* record farg addr definition in symtab */
	pp = alloc_plink();
	pp->val = laststmtno;
	pp->next = (PLINK *) (sp->x6n.cv);
	((PLINK *) (sp->x6n.cv)) = pp;

	/* record memory def if appropriate.  This is the entry def. */
	(*simple_def_proc)(sp, parent, MEMORY, YES);
	}

    if (parent && (parent->in.op == ASSIGN) && (parent->in.left->in.op == OREG)
     && (parent->in.left->tn.lval < 0) && ((parent->in.left->tn.lval % 4) == 0)
     && (parent->in.right->in.op == OREG) && (parent->in.right->tn.rval > 0)
     && ((parent->in.right->tn.lval % 4) == 0))
	{
	slot = (-parent->in.left->tn.lval) / 4;
	if (slot >= max_farg_slots)
	    {
	    old_max_slot = max_farg_slots;
	    do {
		max_farg_slots += MAX_FARG_SLOTS;
		} while (max_farg_slots <= slot);
	    farg_slots = (long *)ckrealloc(farg_slots, max_farg_slots * sizeof(long));
	    memset(farg_slots + old_max_slot, 0,
		   (max_farg_slots - old_max_slot) * sizeof(long));
	    }
	farg_slots[slot] = parent->in.right->tn.lval;
	}

}  /* process_farg_addr_def */

/*****************************************************************************
 *
 *  PROCESS_FARG_DEF()
 *
 *  Description:	Process a non-array formal argument definition
 *
 *  Called by:		process_array_def()
 *			process_def()
 *
 *  Input Parameters:	sp -- symbol table entry of farg
 *			parent -- tree node of defining operator
 *			deftype -- REGISTER- or MEMORY-type def
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: assumptions
 *			comtab[]
 *			fargtab[]
 *			in_reg_alloc_flag
 *			lastcom
 *			lastfarg
 *			laststmtno
 *			pass1_collect_attributes
 *			simple_def_proc
 *			simple_use_proc
 *			symtab
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	(*simple_def_proc)()
 *			(*simple_use_proc)()
 *
 *****************************************************************************
 */

LOCAL void process_farg_def(sp, parent, deftype, dosubtree)
register HASHU *sp;
NODE *parent;
long deftype;
long dosubtree;
{
    register long i;
    register HASHU *p;
    register CLINK *ccp;

    if (!NO_PARM_OVERLAPS)
	{
	/* fargs may overlap COMMON elements or other fargs.  In this case
	 * we always keep memory consistent with register contents.  Generate
	 * memory-type defs for all COMMON and fargs.
	 */
	if (sp != NULL)
	    {
	    if (sp->a6n.array)
		{
		if (in_reg_alloc_flag)
		    {
		    if (dosubtree == TREE_NOT_PROCESSED)
	                (*simple_use_proc)(sp, parent, REGISTER, YES, NO);
		    }
		else
	            (*simple_def_proc)(sp, parent, MEMORY, NO);
		}
	    else
	        (*simple_def_proc)(sp, parent, MEMORY, YES);
	    }

	for (i = 0; i <= lastfarg; ++i)
	    {
	    p = symtab[fargtab[i]];
	    if (p != sp)	/* don't do the main farg again */
		{
		if (in_reg_alloc_flag && (dosubtree == TREE_NOT_PROCESSED))
	            (*simple_use_proc)(p, parent, MEMORY, NO, NO);
		(*simple_def_proc)(p, parent, MEMORY, NO);
		}
	    }
	
	for (i = 0; i <= lastcom; ++i)
	    {
	    for (ccp = symtab[comtab[i]]->cn.member; ccp; ccp = ccp->next)
		{
	        p = symtab[ccp->val];
	        if (in_reg_alloc_flag && p->an.array)
				/* process only the specific array elements */
		    {
		    register CLINK *cp;
		    for (cp = p->xn.member; cp; cp = cp->next)
		        {
		        p = symtab[cp->val];
			if (dosubtree == TREE_NOT_PROCESSED)
			    (*simple_use_proc)(p, parent, MEMORY, NO, NO);
			(*simple_def_proc)(p, parent, MEMORY, NO);
		        }
		    }
	        else
		    {
		    if (in_reg_alloc_flag && (dosubtree == TREE_NOT_PROCESSED))
			(*simple_use_proc)(p, parent, MEMORY, NO, NO);
		    (*simple_def_proc)(p, parent, MEMORY, NO);
		    }
	        }
	    }
	}
    else	/* parameters don't overlap */
	{
	if (sp != NULL)
	    {
	    if (sp->a6n.array)
		{
		if (in_reg_alloc_flag)
		    {
		    if (dosubtree == TREE_NOT_PROCESSED)
	                (*simple_use_proc)(sp, parent, REGISTER, YES, NO);
		    }
		else
	            (*simple_def_proc)(sp, parent, MEMORY, NO);
		}
	    else
	        (*simple_def_proc)(sp, parent, deftype, YES);
	    }
	}
}  /* process_farg_def */

/*****************************************************************************
 *
 *  PROCESS_PTRINDIR_DEF()
 *
 *  Description:	Process def of a pointer indirection.
 *
 *  Called by:		process_def()
 *
 *  Input Parameters:	np -- UNARY MUL of pointer indirection
 *			parent -- tree node of definition operator
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: in_reg_alloc_flag
 *			lastptr
 *			laststmtno
 *			pass1_collect_attributes
 *			ptrtab
 *			simple_def_proc
 *			simple_use_proc
 *			symtab
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	cerror()
 *			(*simple_def_proc)()
 *			(*simple_use_proc)()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_ptrindir_def(np, parent, dosubtree)
NODE *np;
NODE *parent;
long dosubtree;
{
    register long i;
    register HASHU *sp;
    register CLINK *cp;

    if (np->in.op != UNARY MUL)
#pragma BBA_IGNORE
	cerror("op != UNARY MUL in process_ptrindir_def()");
    if (np->in.isarrayform && ! array_forms_fixed)
	fix_array_form(np);
    if (dosubtree == TREE_NOT_PROCESSED)
        traverse_stmt_1(np->in.left, np);

    for (i = 0; i <= lastptr; ++i)
	{
	sp = symtab[ptrtab[i]];
	if (in_reg_alloc_flag && sp->an.array)
			/* process only the specific array elements */
	    {
	    for (cp = sp->xn.member; cp; cp = cp->next)
		{
		sp = symtab[cp->val];
	        if (dosubtree == TREE_NOT_PROCESSED)
		    (*simple_use_proc)(sp, parent, MEMORY, NO, NO);
		(*simple_def_proc)(sp, parent, MEMORY, NO);
		}
	    }
	else
	    {
	    if (in_reg_alloc_flag && (dosubtree == TREE_NOT_PROCESSED))
		(*simple_use_proc)(sp, parent, MEMORY, NO, NO);
	    (*simple_def_proc)(sp, parent, MEMORY, NO);
	    }

	if (pass1_collect_attributes)
	    stmttab[laststmtno].hassideeffects = YES;
	}
}  /* process_ptrindir_def */

/*****************************************************************************
 *
 *  PROCESS_PTRINDIR_USE()
 *
 *  Description:	Process use of a pointer indirection.
 *
 *  Called by:		process_use()
 *
 *  Input Parameters:	np -- UNARY MUL of pointer indirection
 *			parent -- tree node of definition operator
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastptr
 *			laststmtno
 *			pass1_collect_attributes
 *			ptrtab
 *			symtab
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	cerror()
 *			(*simple_use_proc)()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_ptrindir_use(np, parent)
NODE *np;
NODE *parent;
{
    register long i;
    register HASHU *sp;
    register CLINK *cp;

    if (np->in.op != UNARY MUL)
#pragma BBA_IGNORE
	cerror("op != UNARY MUL in process_ptrindir_use()");
    if (np->in.isarrayform && ! array_forms_fixed)
	fix_array_form(np);
    traverse_stmt_1(np->in.left, np);

    for (i = 0; i <= lastptr; ++i)
	{
	sp = symtab[ptrtab[i]];
	if (in_reg_alloc_flag && sp->an.array)
			/* process only the specific array elements */
	    {
	    for (cp = sp->xn.member; cp; cp = cp->next)
		{
		sp = symtab[cp->val];
		(*simple_use_proc)(sp, parent, MEMORY, NO, NO);
		}
	    }
	else
	    {
	    (*simple_use_proc)(sp, parent, MEMORY, NO, NO);
	    }
	}
}  /* process_ptrindir_use */

/*****************************************************************************
 *
 *  PROCESS_USE()
 *
 *  Description:	Process a variable "use".  Add DU blocks and elements
 *			to live vars gen sets
 *
 *  Called by:		traverse_stmt_1()
 *
 *  Input Parameters:	np -- tree node for use'd variable
 *			parent -- tree node for operator
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: laststmtno
 *			pass1_collect_attributes
 *			simple_use_proc
 *			symtab
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	find()
 *			process_array_use()
 *			(*simple_use_proc)()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL void process_use(np, parent)
register NODE *np;
NODE *parent;
{
    HASHU *sp;
    long find_loc;

    if (np->in.arrayref || np->in.arraybaseaddr
     || (np->in.isarrayform && !np->in.isptrindir))
	{
	/* This may eliminate the array designation */
	fix_array_form(np);
	}

    if (np->in.arrayref || np->in.arraybaseaddr
     || (np->in.isarrayform && !np->in.isptrindir))
	{
	process_array_use(np,parent);
	return;
	}

    if (np->in.isptrindir)
	{
	process_ptrindir_use(np, parent);
	return;
	}

    if (fortran && (np->in.op == UNARY MUL) && (np->in.left->in.op == OREG))
	{
	sp = symtab[find(np->in.left)];
	if (sp->a6n.farg)
	    (*simple_use_proc)(sp,parent,REGISTER,YES,NO);  /* use parent op */
	else
	    (*simple_use_proc)(sp,np,REGISTER,YES,NO);	  /* use UNARY MUL op */
	return;
	}

    switch (optype(np->in.op))
	{
	case BITYPE:
		traverse_stmt_1(np->in.left, np);
		traverse_stmt_1(np->in.right, np);
		break;
	case UTYPE:
		traverse_stmt_1(np->in.left, np);
		break;
	case LTYPE:
		find_loc = find(np);
		if (find_loc != (unsigned) (-1))
		  {
		  sp = symtab[find_loc];
		  if (((np->in.op == ICON) && (np->atn.name != NULL)
		    && sp->an.isconst)
		   || (np->in.op == FOREG))
		      break;	/* don't enter address uses */
		  if (fortran && (np->in.op == OREG) && (sp->a6n.farg))
		      break;	/* don't enter address uses */
		  /* kludge for mixed auto and adjustable arrays w/ ENTRY */
		  if (fortran && (np->in.op == OREG)
			&& (sp->a6n.wholearrayno != NO_ARRAY))
		      {
		      sp=symtab[sp->a6n.wholearrayno];
		      if (!sp->a6n.farg)
			  break;
		      }
		  (*simple_use_proc)(sp,parent,REGISTER,YES,NO);
		  }
		break;
	}
    return;
}  /* process_use */

/*****************************************************************************
 *
 *  RECOGNIZE_ARRAY()
 *
 *  Description:	Given a array reference tree, parse it.  Return whether
 *			it's a whole array, arbitrary element or constant
 *			element.  Return a pointer to the symtab entry.  Return
 *			a pointer to the subscript tree (but do not walk it).
 *			Return whether this is a "value" or a "reference" use.
 *
 *  Called by:		process_array_def()
 *			process_array_use()
 *			process_call_array_arg()
 *
 *  Input Parameters:	np -- top of array reference tree with in->arrayref set
 *
 *  Output Parameters:	ptype -- pointer to flag whether this refers to a whole
 *				array (WHOLE_ARRAY),
 *				arbitrary element(ARB_ELEMENT) or specific
 *				element(CONST_ELEMENT).
 *			psp -- pointer to symbol table entry.  XN or X6N record
 *				for whole array & arb element types; AN or A6N
 *				record for specific array elements.
 *			psubscript_tree -- pointer to subscript tree.  NULL if
 *				whole array or specific element.
 *			psubscript_parent -- pointer to parent node of
 *				subscript tree.  NULL if *psubscript_tree NULL.
 *			pref_val -- pointer to flag whether this reference was
 *				a value (VALUE) or reference (REFERENCE) type.
 *			pconst_tree -- pointer to constant pulled out of
 *				array/struct/ptr subscript reference.
 *			pcse_sp -- pointer to symbol table entry for CSE OREG
 *				which is the array reference.  NULL if not a
 *				CSE tree or a C0 temporary describing an array
 *				element.
 *
 *  Globals Referenced:	baseoff
 *			in_reg_alloc_flag
 *			symtab
 *
 *  Globals Modified:	saw_global_access
 *
 *  External Calls:	cerror()
 *			find()
 *			fix_array_form()
 *
 *****************************************************************************
 */

LOCAL void recognize_array(np, ptype, psp, psubscript_tree, psubscript_parent,
			pref_val, pconst_tree, pcse_sp)
register NODE *np;
long *ptype;
HASHU **psp;
NODE **psubscript_tree;
NODE **psubscript_parent;
long *pref_val;
NODE **pconst_tree;
HASHU **pcse_sp;
{
    HASHU *sp;
    NODE *nr;
    NODE *topnode = np;
    HASHU *c0sp;

    if (! array_forms_fixed)
	fix_array_form(np);

    *pconst_tree = NULL;
    *pcse_sp = NULL;
    *psubscript_parent = NULL;
    
    sp = symtab[np->in.arrayrefno];

    if (sp->an.farg)	/* formal argument array */
	{
	*psp = sp;		/* don't distinguish farg specific elements */
	if (np->in.op == UNARY MUL)
	    {
	    *pref_val = VALUE;
	    np = np->in.left;
	    if (np->in.op == OREG)
		{
		if (np->tn.lval < -baseoff)	/* CSE OREG */
		    {
		    *ptype = ARB_ELEMENT;
		    *psubscript_tree = NULL;
		    *pcse_sp = symtab[find(np)];
		    }
		else	
		    {
		    if (fortran && (c0sp = symtab[find(np)], c0sp->a6n.isc0temp))
			{
		        *ptype = ARB_ELEMENT;
		        *psubscript_tree = NULL;
		        *pcse_sp = c0sp;
			}	    
		    else			/* FARG */
			{
/*		        np->in.arraybaseaddr = NO; /* prevent confusion later */
		        *ptype = CONST_ELEMENT;
		        *psubscript_tree = NULL;
			}
		    }
		}
	    else if (np->in.op == PLUS)
		goto fargplus;
	    else
#pragma BBA_IGNORE
		cerror("strange-shaped farg array in recognize_array() - 1");
	    }
	else if (np->in.op == PLUS)
	    {
	    *pref_val = REFERENCE;
fargplus:
	    if (np->in.left->in.op == PLUS)
		{
		if (np->in.right->in.op != ICON)
#pragma BBA_IGNORE
		    cerror("strange-shaped farg array in recognize_array() - 2");
		*pconst_tree = np->in.right;
		np = np->in.left;
		}

	    if (np->in.left->in.op == OREG)
		{
/*		np->in.left->in.arraybaseaddr = NO;	/* prevent confusion */
		if (np->in.left->tn.lval < -baseoff)
		    *pcse_sp = symtab[find(np->in.left)];
		else if (fortran
		      && (c0sp = symtab[find(np->in.left)], c0sp->a6n.isc0temp))
		    *pcse_sp = c0sp;
		nr = np->in.right;
		if ((*pcse_sp == NULL) && (nr->in.op == ICON)
		 && (nr->atn.name == NULL))
		    {
		    *ptype = CONST_ELEMENT;
		    *psubscript_tree = NULL;
		    }
		else
		    {
		    *ptype = ARB_ELEMENT;
		    *psubscript_tree = nr;
		    *psubscript_parent = np;
		    }
		}
	    else
		{
	        if ((np->in.left->in.op == FOREG) && !fortran)
		    {
		    /* C farg struct arrays may look more like non-farg arrays.
		     * Try again.
		     */
		    np = topnode;
		    goto non_farg_array;
		    }
		else
#pragma BBA_IGNORE
		    cerror("strange-shaped farg array in recognize_array() - 3");
		}
	    }
	else if (np->in.op == OREG)
	    {
	    c0sp = symtab[find(np)];
	    if ((np->tn.lval < -baseoff)		/* CSE-created temp */
	     || (fortran && c0sp->a6n.isc0temp))
		{
	        *pref_val = REFERENCE;
		*ptype = ARB_ELEMENT;
		*psubscript_tree = NULL;
		*pcse_sp = c0sp;
		}
	    else
		{
	        *pref_val = REFERENCE;
		*ptype = CONST_ELEMENT;
		*psubscript_tree = NULL;
		}
	    }
	else if (np->in.op == FOREG)	/* structure-valued function result */
		{
	        *pref_val = REFERENCE;
		*ptype = WHOLE_ARRAY;
		*psubscript_tree = NULL;
		}
	else
	    cerror("strange-shaped farg array in recognize_array() - 4");
	}
    else 		/* not a formal argument array */
	{
non_farg_array:
	if (np->in.op == UNARY MUL)
	    {
	    *pref_val = VALUE;
	    np = np->in.left;
	    if (np->in.op == PLUS)
		{
		goto plus;
		}
	    else if (np->in.op == OREG)
		{
		*psp = sp;
		*ptype = ARB_ELEMENT;
		*psubscript_tree = NULL;

		if (np->tn.lval < -baseoff)
		    *pcse_sp = symtab[find(np)];
		else if (fortran && (c0sp = symtab[find(np)], c0sp->a6n.isc0temp))
		    *pcse_sp = c0sp;
		else if (np->in.comma_ss)
		    *pcse_sp = symtab[find(np)];
		else
#pragma BBA_IGNORE
		    cerror("invalid OREG in recognize_array()");
		}
	    else if (((np->in.op == ICON) && (np->atn.name != NULL))
		  || (np->in.op == FOREG))
		{
		*psp = sp;
		*ptype = ARB_ELEMENT;
		*psubscript_tree = NULL;
		if (np->in.op == ICON)
		  saw_global_access = YES;
		}
	    else
#pragma BBA_IGNORE
	        cerror("strange-shaped array in recognize_array() - 1");
	    }
	else if (np->in.op == PLUS)
	    {
	    *pref_val = REFERENCE;
plus:
	    if (np->in.left->in.op == PLUS)
		{
		if ((np->in.right->in.op != ICON) || np->in.right->atn.name)
#pragma BBA_IGNORE
		    cerror("strange-shaped array in recognize_array() - 2");
		*pconst_tree = np->in.right;
		np = np->in.left;
		}

	    if ((np->in.left->in.op == FOREG)
	     || ((np->in.left->in.op == ICON)
	      && (np->in.left->atn.name != NULL)))
		{
/*		np->in.left->in.arraybaseaddr = NO; /* prevent confusion */
		*psp = sp;
		*ptype = ARB_ELEMENT;
		*psubscript_tree = np->in.right;
		*psubscript_parent = np;
		if (np->in.left->in.op == ICON)
		  saw_global_access = YES;
		}
	    else if (np->in.left->in.op == OREG)
		{
		*psp = sp;
		*ptype = ARB_ELEMENT;
		*psubscript_tree = np->in.right;
		*psubscript_parent = np;

		if ((np->in.left->tn.lval < -baseoff) ||
		    (np->in.left->tn.comma_ss))  /* CSE temp */
		    *pcse_sp = symtab[find(np->in.left)];
		else if (fortran
		      && (c0sp = symtab[find(np->in.left)], c0sp->a6n.isc0temp))
		    *pcse_sp = c0sp;
		else
#pragma BBA_IGNORE
		    cerror("invalid OREG in recognize_array()");
		}
	    else if (np->in.left->in.op == REG)
		{
		*psp = sp;
		*ptype = ARB_ELEMENT;
		*psubscript_tree = np->in.right;
		*psubscript_parent = np;
		}
	    else
	        cerror("strange-shaped array in recognize_array() - 3");
	    }
	else if (np->in.op == OREG)
	    {
	    if (np->tn.lval < -baseoff)		/* c1-created */
		{
	        *pref_val = REFERENCE;
		*pcse_sp = symtab[find(np)];
		*psp = sp;
		*ptype = ARB_ELEMENT;
	        *psubscript_tree = NULL;
		}
	    else if (fortran && (c0sp = symtab[find(np)], c0sp->a6n.isc0temp))
		{
	        *pref_val = REFERENCE;
		*pcse_sp = c0sp;
		*psp = sp;
		*ptype = ARB_ELEMENT;
	        *psubscript_tree = NULL;
		}
	    else
		{
	        *pref_val = VALUE;
	        goto leaf;
		}
	    }
	else if (np->in.op == NAME)
	    {
	    *pref_val = VALUE;
	    goto leaf;
	    }
	else if ((np->in.op == FOREG) || (np->in.op == ICON))
	    {
	    *pref_val = REFERENCE;
leaf:
	    *psubscript_tree = NULL;
	    if (np->in.arrayelem)	/* constant array element */
		{
		if (fortran)
		  sp = symtab[find(np)];
		*psp = sp;
		*ptype = CONST_ELEMENT;
		}
	    else			/* whole array */
		{
		*psp = sp;
		*ptype = WHOLE_ARRAY;
		}
	    if ((np->in.op == ICON) || (np->in.op == NAME))
	      saw_global_access = YES;
	    }
	else
#pragma BBA_IGNORE
	    cerror("strange-shaped array in recognize_array() - 4");
	}
}  /* recognize_array */

/*****************************************************************************
 *
 *  TRAVERSE_STMT_1()
 *
 *  Description:	Recursively descend the input tree, finding defs and
 *			uses
 *
 *  Called by:		first_pass_tree_walk()
 *			process_array_def()
 *			process_array_use()
 *			process_call_arg()
 *			process_call_array_arg()
 *			process_def()
 *			process_use()
 *			traverse_stmt_1()	-- recursive call
 *
 *  Input Parameters:	tree -- tree to be traversed
 *			parent -- parent node
 *
 *  Output Parameters:	return value -- 1 iff tree is NULL (NOCODEOP)
 *
 *  Globals Referenced: fortran
 *			laststmtno
 *			pass1_collect_attributes
 *
 *  Globals Modified:	stmttab
 *
 *  External Calls:	process_call()
 *			process_def()
 *			process_use()
 *			strcmp()
 *			traverse_stmt_1() -- recursively descend tree
 *
 *****************************************************************************
 */

flag traverse_stmt_1(tree,parent)
register NODE *tree;
NODE *parent;
{
    switch (tree->in.op)
	{
	case NOCODEOP:
		    return(YES);		/* flag to delete this tree */

	case ICON:	/* address uses -- don't count */
		/* Avoid considering a function call's ICON node 
		 * as a global variable reference.  It will look
		 * like an ICON with the type PTR FTN <type>.
		 * Accessing a functions location IS a global 
		 * variable reference.
		 */
		if (tree->in.name != NULL)
		  if ((((tree->in.type.base >> 6) & 0xf) != 9) ||
		      !callop(parent->in.op))
		    saw_global_access = YES;
		    break;

	case FOREG:
		    if (parent->in.op == STASG)
		      process_use(tree,parent);
		    break;

	case OREG:
	case NAME:
		    if (tree->in.op == NAME)
		      saw_global_access = YES;
		    process_use(tree,parent);
		    break;

	case UNARY MUL:
		    if (tree->in.arrayref || tree->in.isptrindir
		     || tree->in.structref
		     || ((tree->in.left->in.op == OREG) && fortran))
			process_use(tree,parent);  /* might be farg */
		    else
			traverse_stmt_1(tree->in.left, tree);
		    break;

	case PLUS:
		    if (tree->in.arrayref || tree->in.structref)
			{
			process_use(tree,parent);
			}
		    else
			{
			if (in_reg_alloc_flag
			    && ((tree->in.type.base == FLOAT)
			     || (tree->in.type.base == DOUBLE)))
			    {
			    currregiondesc->nFPAops++;
			    saw_dragon_access = YES;
			    }

			goto binary;
			}
		    break;

	case MINUS:
	case MUL:
	case DIV:
		    if (in_reg_alloc_flag
		     && ((tree->in.type.base == FLOAT)
		      || (tree->in.type.base == DOUBLE)))
			{
			currregiondesc->nFPAops++;
			saw_dragon_access = YES;
			}

		    goto binary;
		    break;

	case SCONV:
		    if (in_reg_alloc_flag
		     && (((tree->in.type.base == FLOAT)
		           || (tree->in.type.base == DOUBLE))
                       || ((tree->in.left->in.type.base == FLOAT)
			   || (tree->in.left->in.type.base == DOUBLE))))
			{
			currregiondesc->nFPAops++;
			saw_dragon_access = YES;
			}
		
		    goto unary;

	case FMONADIC:
		    if (in_reg_alloc_flag)
			{
			if (! strcmp(tree->in.left->in.name, "fabs"))
			    {
			    currregiondesc->nFPAops++;
			    saw_dragon_access = YES;
			    }
			else
			    currregiondesc->n881ops++;
			}

		    goto binary;

	case CALL:
	case UNARY CALL:
	case STCALL:
	case UNARY STCALL:
		    {
		    NODE *lop;
	 	    if (pass1_collect_attributes)
		        stmttab[laststmtno].hassideeffects = YES;
		    process_call(tree);

		    /* Inlined transcendentals need pic base reg */
		    lop = tree->in.left;
                    if ((lop->in.op == ICON) &&
                        (lop->in.name[0] == '_') &&
                        (lop->in.name[1] == '_') &&
                        (lop->in.name[2] == 'f') &&
                        (lop->in.name[3] == 'p') &&
                        (lop->in.name[4] == '0') &&
                        (lop->in.name[5] == '4') &&
                        (lop->in.name[6] == '0') &&
                        (lop->in.name[7] == '_'))
                      saw_global_access = YES;

		    break;
		    }

	case ASSIGN:
	case STASG:
		    if ((parent->in.op == SEMICOLONOP)
		     || (parent->in.op == UNARY SEMICOLONOP))
			{
			NODE *lp = tree->in.left;
			NODE *rp = tree->in.right;
			if ((optype(lp->in.op) == LTYPE)
			 && (optype(rp->in.op) == LTYPE)
			 && (lp->tn.lval == rp->tn.lval)
			 && (lp->tn.rval == rp->tn.rval)
			 && (lp->tn.op == rp->tn.op)
			 && (!strcmp(lp->atn.name, rp->atn.name))
			 && (!lp->tn.equivref))
			    {
			    tfree(lp);
			    tfree(rp);
			    tree->in.op = NOCODEOP;
			    return(YES);
			    }
			}
		    else if (pass1_collect_attributes)
			stmttab[laststmtno].hassideeffects = YES;
		    traverse_stmt_1(tree->in.right, tree);
		    process_def(tree->in.left, tree, TREE_NOT_PROCESSED);
		    break;

	case ASG PLUS:
	case ASG MINUS:
	case ASG MUL:
	case ASG DIV:
	case ASG MOD:
	case ASG AND:
	case ASG OR:
	case ASG ER:
	case ASG LS:
	case ASG RS:
	case ASG DIVP2:
	case INCR:
	case DECR:
		    if ((parent->in.op != SEMICOLONOP)
		     && (parent->in.op != UNARY SEMICOLONOP)
		     && pass1_collect_attributes)
			stmttab[laststmtno].hassideeffects = YES;

		    traverse_stmt_1(tree->in.right, tree);
	 	    process_use(tree->in.left, tree);
		    process_def(tree->in.left, tree, TREE_PROCESSED);
		    break;

	case COMOP:		/* same as default binary, except pass COMOP's
				 * parent to right subtree for accurate
				 * costing.
				 */
		    traverse_stmt_1(tree->in.left, tree);
		    traverse_stmt_1(tree->in.right, parent);
		    break;


	case EQ:
	case NE:
	case LE:
	case LT:
	case GE:
	case GT:
		    if (in_reg_alloc_flag
			&& ((tree->in.left->in.type.base == FLOAT)
                         || (tree->in.left->in.type.base == DOUBLE)))
			{
			currregiondesc->nFPAops++;
			saw_dragon_access = YES;
			}
		    /* drop throuch to default case */

	default:
		    if (asgop(tree->in.op))
#pragma BBA_IGNORE
			cerror("asgop seen in traverse_stmt_1()");

		    /* code gen for a SWITCH or FCOMPGOTO will use %a2 */
		    if (tree->in.op == FCOMPGOTO)
		      saw_global_access = YES;
		    switch (optype(tree->in.op))
			{
			case LTYPE:
			    break;
			case UTYPE:
unary:
			    traverse_stmt_1(tree->in.left, tree);

			    if (in_reg_alloc_flag
			        && ((tree->in.left->in.type.base == FLOAT)
			         || (tree->in.left->in.type.base == DOUBLE)))
			        {
			        currregiondesc->nFPAops++;
			        saw_dragon_access = YES;
			        }
			    break;
			case BITYPE:
binary:
			    traverse_stmt_1(tree->in.left, tree);
			    traverse_stmt_1(tree->in.right, tree);
			    break;
			}
	}
    return(NO);
}  /* traverse_stmt_1 */
