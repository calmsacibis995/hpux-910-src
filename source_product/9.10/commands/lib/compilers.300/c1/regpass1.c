/* file regpass1.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)regpass1.c	16.1 90/11/05 */

/*****************************************************************************
 *
 *  File regpass1.c contains routines for the first pass over the
 *  statement trees.  This pass does the following:
 *	1. Allocate and fill the definetab[] with definition info.
 *	2. Number the statements and fill the stmttab[].
 *	3. Allocate and fill gen and kill sets for reaching defs and
 *		live variables data flow analyses.
 *	4. Find variable defs and uses -- allocate and fill DU-blocks and
 *		attach to symtab entry.
 *
 *****************************************************************************
 */


#include "c1.h"

LOCAL long currblockno;		/* current block number (dfo ordering) */

/* Routines defined in this file */

LOCAL void add_definetab_entry();
LOCAL void add_def_block();
LOCAL void add_entry_defs();
LOCAL void add_exit_uses();
LOCAL void add_use_block();
      long calculate_savings();
LOCAL void expand_definetab();
LOCAL void expand_stmttab();
LOCAL void first_pass_init();
LOCAL flag first_pass_tree_walk();
LOCAL void make_reaching_kill_sets();
LOCAL void make_var_def_sets();
LOCAL void process_simple_def();
LOCAL void process_simple_use();
LOCAL void realloc_definetab();
      void reg_alloc_first_pass();	/* called by main() in register.c */

/*****************************************************************************
 *
 *  ADD_DEFINETAB_ENTRY()
 *
 *  Description:	Add an entry to the definetab.
 *
 *  Called by:		add_entry_defs()
 *			process_simple_def()
 *
 *  Input Parameters:	sym -- symtab pointer of item being defined
 *			np -- tree node containing definition operator
 *			stmt -- statement number
 *			isdefinite -- is this a "definite" define?
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			lastdefine
 *			maxdefine
 *
 *  Globals Modified:	definetab[] -- entry added to it
 *			lastdefine -- incremented
 *
 *  External Calls:	expand_definetab()
 *
 *****************************************************************************
 */

LOCAL void add_definetab_entry(sym, np, stmt, isdefinite)
HASHU *sym;
NODE *np;
long stmt;
long isdefinite;
{
    register DEFTAB *dp;

    if (++lastdefine >= maxdefine)
	expand_definetab();
    dp = definetab + lastdefine;
    dp->sym = sym;
    dp->np = np;
    dp->stmtno = stmt;
    dp->isdefinite = isdefinite;
    dp->nuses = 0;
}  /* add_definetab_entry */

/*****************************************************************************
 *
 *  ADD_DEF_BLOCK()
 *
 *  Description:	Allocate a DU block, fill it with the info supplied,
 *			and attach it to the variable's DU chain
 *
 *  Called by:		add_entry_defs()
 *			process_simple_def()
 *
 *  Input Parameters:	sym -- pointer to variables symbol table entry
 *			type -- REGISTER or MEMORY-type def
 *			stmt -- stmt def occurred in
 *			savings -- expected savings if assigned to register
 *			defno -- index of this def in the definetab
 *			isdefinite -- is this a "definite" def?
 *			parent -- tree pointer of parent node
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_dublock()
 *
 *****************************************************************************
 */

LOCAL void add_def_block(sym, type, stmt, savings, defno, isdefinite, parent)
HASHU *sym;
long type;
long stmt;
long savings;
long defno;
unsigned isdefinite;
NODE *parent;
{
    register DUBLOCK *dp;
    DUBLOCK *sp;

    dp = alloc_dublock();
    dp->isdef = YES;
    dp->ismemory = (type == MEMORY);
    dp->isdefinite = isdefinite;
    dp->stmtno = stmt;
    dp->savings = savings;
    dp->defsetno = defno;
    dp->d.parent = parent;
    dp->inFPA881loop = NO;
    if (parent)
	{
	switch (parent->in.op)
	    {
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
			dp->parent_is_asgop = YES;
			dp->savings = 0;	/* already counted in USE */
			break;
	    default:
			dp->parent_is_asgop = NO;
			break;
	    }
	}
    else
	{
	dp->parent_is_asgop = NO;
	}

    /* attach the DU block to the symtab entry.  The DU chain is a circular
     * linked list with the HASHU field pointing to the end of the chain
     * (i.e., the most recently added entry).  Entries' next fields point
     * to the chronologically following DU block.
     */
    if (sp = sym->a6n.du)
	{
	dp->next = sp->next;
	sp->next = dp;
	sym->a6n.du = dp;
	}
    else
	{
	sym->a6n.du = dp;
	dp->next = dp;
	}
}  /* add_def_block */

/*****************************************************************************
 *
 *  ADD_ENTRY_DEFS()
 *
 *  Description:	Create memory-type defs for all register candidates in
 *			COMMON, static vars, and array base
 *			addresses at each entry point to the procedure.
 *			Also find candidates for EXIT uses.
 *
 *  Called by:		reg_alloc_first_pass()
 *
 *  Input Parameters:	pexitsyms -- where to store address of exitsyms array
 *			plastexitsym -- where to store index of last entry in
 *					exitsyms array
 *
 *  Output Parameters:	pexitsyms -- base address of vector of possible symtab
 *					items with uses at EXIT.
 *			plastexitsym -- index of last entry in exitsyms array
 *
 *  Globals Referenced:	in_reg_alloc_flag
 *			lastdefine
 *			lastep
 *			maxsymtsz
 *			outreachep[]
 *			symtab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_def_block()
 *			add_definetab_entry()
 *			adelement()
 *			ckalloc()
 *
 *****************************************************************************
 */
LOCAL void add_entry_defs(pexitsyms, plastexitsym)
HASHU ***pexitsyms;
long *plastexitsym;
{
    register HASHU	**exitsyms;
    register HASHU	*sp;
    register long	lastexitsym;
    long	epn;
    register long i;

    /* allocate vectors for holding exit sym pointers */
    exitsyms = (HASHU **) ckalloc((lastfilledsym + 1) * sizeof(HASHU *));
    lastexitsym = -1;

    /* go though symbol table and find vars needing entry/exit def/uses 
     * and put pointers to the syms in the exitsyms array.
     */
    for (i = lastfilledsym; i >= 0; --i)
	{
	sp = symtab[i];
	    switch (sp->an.tag)
		{
		default:
		case CN:
		case ON:
			continue;	/* ignore COMMON block and REG entries*/

		case SN:
		case S6N:
		case XN:
		case X6N:
			/* array base address */
			if (sp->x6n.farg && fortran && in_reg_alloc_flag)
			    {
			    if (lastep == 0)
				{
				/* add a MEMORY-type def for formal arguments
				   that have been revised by rm_farg_aliases()*/
			        add_definetab_entry(sp, NULL, 0, NO);
			        adelement(lastdefine, outreachep[0],
						  outreachep[0]);
			        add_def_block(sp, MEMORY, 0, 0,
						  lastdefine, NO, NULL);
			        }
			    else
			    	sp->x6n.cv = NULL;
			    }
			else if (sp->an.varno > 0)
			    {
			    /* add a MEMORY-type def for each entry point */
			    for (epn = lastep; epn >= 0; --epn)
				{
			        add_definetab_entry(sp, NULL, -epn, NO);
			        adelement(lastdefine, outreachep[epn],
						  outreachep[epn]);
			        add_def_block(sp, MEMORY, -epn, 0,
						  lastdefine, NO, NULL);
			        }
			    }

			if ((!in_reg_alloc_flag) && (sp->an.varno > 0)
			 && ((sp->an.tag == XN) || sp->x6n.farg))
			    exitsyms[++lastexitsym] = sp;

			break;

		case AN:			/* static var */
			if (sp->an.varno > 0)
			    {
			    if (! sp->an.isconst)
			        exitsyms[++lastexitsym] = sp;

			    /* add a MEMORY-type def for each entry point */
			    for (epn = 0; epn <= lastep; ++epn)
				{
			        add_definetab_entry(sp, NULL, -epn, NO);
			        adelement(lastdefine, outreachep[epn],
						  outreachep[epn]);
			        add_def_block(sp, MEMORY, -epn, 0, lastdefine,
					      NO, NULL);
				}
			    }
			break;

		case A6N:			/* farg or stack var */
			if (in_reg_alloc_flag)
			    {
			    if (sp->a6n.farg || sp->a6n.hiddenfarg)
			        {
			        if (fortran)
				    {
				    /* if there are no FORTRAN entry points
				     * the parameter copy code will be gone.
				     * This would have provided an entry def.
				     * The entry def must be explicitly done.
				     */
			            if ((sp->a6n.varno > 0) && (lastep == 0))
				        {
			                add_definetab_entry(sp, NULL, 0, NO);
			                adelement(lastdefine, outreachep[0],
						      outreachep[0]);
			                add_def_block(sp, MEMORY,0,0,lastdefine,
							    NO, NULL);
				        }
			            sp->a6n.cv = NULL;
			            if ( !sp->a6n.array && (sp->a6n.varno > 0)
					 && !sp->a6n.hiddenfarg)
				        {	/* non-array fargs only */
				        exitsyms[++lastexitsym] = sp;
				        }
				    }
			        else if (sp->a6n.varno > 0)
				    {
			            add_definetab_entry(sp, NULL, 0, NO);
			            adelement(lastdefine, outreachep[0],
						  outreachep[0]);
			            add_def_block(sp, MEMORY, 0, 0, lastdefine,
							NO, NULL);
				    }
			        }
			    else if (sp->a6n.hiddenfarg)  /* fortran only */
			        sp->a6n.cv = NULL;
			    }
			else	/* not in_reg_alloc_flag */
			    {
			    if (sp->a6n.farg || sp->a6n.hiddenfarg)
				{
				/* add entry defs */
			        if (sp->a6n.varno > 0)
				    {
			            add_definetab_entry(sp, NULL, 0, NO);
			            adelement(lastdefine, outreachep[0],
						  outreachep[0]);
			            add_def_block(sp, MEMORY, 0, 0, lastdefine,
							NO, NULL);
				    }
			        if (fortran && sp->a6n.farg)
				    exitsyms[++lastexitsym] = sp;
				}
			    }
		}
	}

    /* EXIT processing -- return exitsyms array for processing AFTER all
     * trees have been processed.
     */

    *pexitsyms = exitsyms;
    *plastexitsym = lastexitsym;
    
}    /* add_entry_defs */


/*****************************************************************************
 *
 *  ADD_EXIT_USES()
 *
 *  Description:	Check which variables need to have memory-type
 *			uses at routine exit.  The condition:  static, COMMON
 *			or fargs (not farg array) with a definition within
 *			the routine (don't count memory-type defs at routine
 *			entry points.  Add uses at the exit for variables
 *			meeting these criteria.
 *
 *  Called by:		reg_alloc_first_pass()
 *
 *  Input Parameters:	exitsyms    -- list of candidate static, COMMON, fargs
 *			lastexitsym -- index of last entry in exitsyms vector
 *
 *  Output Parameters:	exitsyms -- free'd by this routine
 *
 *  Globals Referenced: definetab[]
 *			in_reg_alloc_flag
 *			inliveexit
 *			lastep
 *
 *  Globals Modified:	inliveexit
 *
 *  External Calls:	FREEIT()
 *			add_use_block()
 *			adelement()
 *			nextel()
 *
 *****************************************************************************
 */
LOCAL void add_exit_uses(exitsyms, lastexitsym)
register HASHU **exitsyms;
register long lastexitsym;
{
    register long i;
    register long defno;
    register HASHU *sp;

    for (i = 0; i <= lastexitsym; ++i)
	{
	sp = exitsyms[i];

	if (sp->an.defset)
	    {
	    defno = 0;
	    while ((defno = nextel(defno, sp->an.defset)) > 0)
		{
		if (definetab[defno].stmtno > 0)
		    {
		    if (in_reg_alloc_flag)
		        adelement(sp->an.varno, inliveexit, inliveexit);
		    add_use_block(sp, MEMORY, -(lastep+1), 0, 0, NO, NO, NULL);
		    break;
		    }
		}
	    }
	}

    FREEIT(exitsyms);

}  /* add_exit_uses */

/*****************************************************************************
 *
 *  ADD_USE_BLOCK()
 *
 *  Description:	Allocate and fill a DUBLOCK describing a "use".
 *			Attach to the variable's symtab entry.
 *
 *  Called by:		add_exit_uses()
 *			process_simple_use()
 *
 *  Input Parameters:	sym -- symtab entry of item with use.
 *			type -- REGISTER- or MEMORY-type use.
 *			stmt -- statement number of use.
 *			savings -- estimated savings if item was allocated to
 *					register (machine cycles).
 *			reachdef -- index of single definition reaching this
 *				use (if def occurred prior in same block), else
 *				0 denoting all defs reaching this block.
 *			isdefinite -- is this a "definite" use?
 *			hidden_use -- does a "definite" use hide a "maybe" use?
 *			parent -- parent node of this use
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_dublock()
 *
 *****************************************************************************
 */

LOCAL void add_use_block(sym, type, stmt, savings, reachdef, isdefinite, 
			 hidden_use, parent)
HASHU *sym;
long type;
long stmt;
long savings;
long reachdef;
long isdefinite;
long hidden_use;
NODE *parent;
{
    register DUBLOCK *dp;
    DUBLOCK *sp;

    dp = alloc_dublock();
    dp->isdef = NO;
    dp->ismemory = (type == MEMORY);
    dp->isdefinite = isdefinite;
    dp->hides_maybe_use = hidden_use;
    dp->stmtno = stmt;
    dp->savings = savings;
    dp->defsetno = reachdef; /* actually, this is supposed to be a SET *,
				   * but, it's either 0 or a single def number
				   * so we use the defsetno field.  -- it's
				   * converted to a SET during web construction.
				   */
    dp->d.parent = parent;
    dp->inFPA881loop = NO;

    if (parent)
	{
	switch (parent->in.op)
	    {
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
			dp->parent_is_asgop = YES;
			break;
	    default:
			dp->parent_is_asgop = NO;
			break;
	    }
	}
    else
	{
	dp->parent_is_asgop = NO;
	}

    /* attach the DU block to the symtab entry.  The DU chain is a circular
     * linked list with the HASHU field pointing to the end of the chain
     * (i.e., the most recently added entry).  Entries' next fields point
     * to the chronologically following DU block.
     */
    if (sp = sym->a6n.du)
	{
	dp->next = sp->next;
	sp->next = dp;
	sym->a6n.du = dp;
	}
    else
	{
	sym->a6n.du = dp;
	dp->next = dp;
	}
}  /* add_use_block */

/*****************************************************************************
 *
 *  CALCULATE_SAVINGS()
 *
 *  Description:	Estimate the savings/cost of the operator if the
 *			variable was allocated to a register.
 *
 *  Called by:		process_simple_def()
 *			process_simple_use()
 *
 *  Input Parameters:	sp -- symtab pointer of item
 *			parent -- tree node of operator
 *			usetype -- REGISTER- or MEMORY-type def/use.
 *
 *  Output Parameters:	return value -- estimated savings/cost
 *
 *  Globals Referenced:	reg_savings[]
 *			reg_store_cost[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

long calculate_savings(register_type, parent, usetype)
long register_type;
NODE *parent;
long usetype;
{
    long savings;
    register long rop;
    register long lop;
    register long srcreg;
    register long destreg;
    NODE *sp;
	   
    if (usetype == MEMORY)
        {
        savings = -reg_store_cost[register_type];
        }
    else
        {
        if (parent->in.op == ASSIGN)
	    {
	    /* simple memory assignments are faster than 881 registers */
	    rop = parent->in.right->in.op;
	    lop = parent->in.left->in.op;

	    srcreg = ((rop == OREG) || (rop == NAME) || (rop == ICON));
	    destreg = ((lop == OREG) || (lop == NAME) || (lop == ICON));

	    switch (register_type)
	        {
	        case REG_A_SAVINGS:
	        case REG_D_SAVINGS:
		    if (mc68040)
                        {
                        if (srcreg || destreg)
                                {
                                /* If this value for savings exceeds the value
                                 * or reg_store_cost[REG_A/D_SAVINGS] you will
                                 * get inefficient code for FORTRAN integer
                                 * parameters.
                                 */
                                savings = reg_store_cost[register_type];
                                /* Check for the op= */
                                switch (rop)
                                  {
                                  case PLUS:
                                  case MUL:
                                  case AND:
                                  case OR:
                                  case ER:
					/* commutative ops */
                                        sp = parent->in.right->in.right;
                                        if (same(parent->in.left, sp, NO, NO)
                                                && !haseffects(sp))
                                          {
					  if (register_type == REG_A_SAVINGS)
                                            savings = 3 * reg_A_savings_factor;
					  else
                                            savings = 3 * reg_D_savings_factor;
                                          break;
                                          }
                                        /* fall thru */
                                  case MINUS:
                                  case DIV:
                                  case MOD:
                                  case LS:
                                  case RS:
                                        sp = parent->in.right->in.left;
                                        if (same(parent->in.left, sp, NO, NO)
                                                && !haseffects(sp))
					  if (register_type == REG_A_SAVINGS)
                                            savings = 3 * reg_A_savings_factor;
					  else
                                            savings = 3 * reg_D_savings_factor;
                                        break;
                                  }
                                }
                        else
				savings = 0;
                        }
                    else
                        {
                        if (srcreg && destreg)
                                savings = 6;
                        else if (destreg)
                                savings = 1;
                        else if (srcreg)
                                savings = 3;
                        else
                                savings = 0;
                        }
                    break;
	        case REG_FPA_FLOAT_SAVINGS:
		    savings = 0;	/* it's a draw */
		    break;
	        case REG_FPA_DOUBLE_SAVINGS:
		    /* if both could be regs or src is not array -- might
		     * be expression calcalated on FPA
		     */
		    if (destreg && (srcreg || (rop != UNARY MUL)))
		        savings = 14;
		    else
		        savings = 0;	/* a draw -- memory mapped */
		    break;
	        case REG_881_FLOAT_SAVINGS:
	        case REG_881_DOUBLE_SAVINGS:
		    /* if src is not array -- might
		     * be expression calcalated on 881
		     */
		    if (mc68040)
                        {
                        savings = 2 * reg_F_savings_factor;
                        if (destreg && (rop != UNARY MUL))
                                /* Check for the op= */
                                {
                                switch (rop)
                                  {
                                  case PLUS:
                                  case MUL:
                                  case AND:
                                  case OR:
                                  case ER:
					/* commutative ops */
                                        sp = parent->in.right->in.right;
                                        if (same(parent->in.left, sp, NO, NO)
                                                && !haseffects(sp))
                                          {
                                          savings = 6 * reg_F_savings_factor;
                                          break;
                                          }
                                        /* fall thru */
                                  case MINUS:
                                  case DIV:
                                  case MOD:
                                  case LS:
                                  case RS:
                                        sp = parent->in.right->in.left;
                                        if (same(parent->in.left, sp, NO, NO)
                                                && !haseffects(sp))
                                          savings = 6 * reg_F_savings_factor;
                                        break;
                                  }
                                }
                        }
		    else
                        {
                        if (destreg && srcreg)
                                savings = -25;
                        else if (destreg && (rop != UNARY MUL))
                                savings = reg_store_cost[register_type];
                        else
                                savings = -reg_store_cost[register_type];
                        }
                    break;
	        }
	    }
        else
	  if (mc68040)
            switch (register_type)
                {
                case REG_881_FLOAT_SAVINGS:
                case REG_881_DOUBLE_SAVINGS:
                  savings = reg_F_savings_factor;
                  break;
                case REG_A_SAVINGS:
                  savings = reg_A_savings_factor;
                  break;
                case REG_D_SAVINGS:
                  savings = reg_D_savings_factor;
                  break;
	        case REG_FPA_FLOAT_SAVINGS:
	        case REG_FPA_DOUBLE_SAVINGS:
                  /* do the table lookup to find savings */
                  savings = reg_savings[parent->in.op][register_type];
                  break;
                default:
                  cerror("Bad reg type in calculate_savings()");
                }
          else
            /* do the table lookup to find savings */
            savings = reg_savings[parent->in.op][register_type];
        }

    return(savings);

}  /* calculate_savings */

/*****************************************************************************
 *
 *  EXPAND_DEFINETAB()
 *
 *  Description:	Increment maxdefine; expand definetab and genreach[]
 *			sets appropriately
 *
 *  Called by:		add_definetab_entry()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			genreach[]
 *			lastep
 *			maxdefine
 *			numblocks
 *			outreachep[]
 *
 *  Globals Modified:	definetab
 *			genreach[]
 *			maxdefine
 *			maxdefs
 *			outreachep[]
 *
 *  External Calls:	FREEIT()
 *			ckrealloc()
 *			new_set_n()
 *
 *****************************************************************************
 */

LOCAL void expand_definetab()
{
    register long newmax;
    register long i;
    register long incr;

    incr = maxdefine >> 2;		/* bump by 25% */
    if (incr < 10)
	incr = 10;
    maxdefine += incr;
    maxdefs = maxdefine;		/* for use by mknewpreheader */
    newmax = maxdefine;

    /* expand the definetab */
    definetab = (DEFTAB *) ckrealloc(definetab, newmax * sizeof(DEFTAB));

    /* expand all the genreach[] and outreachep[] sets */
    for (i = 0; i < numblocks; ++i)
	{
		change_set_size( genreach[i], newmax );
	}

    for (i = 0; i <= lastep; ++i)
	{
		change_set_size( outreachep[i], newmax );
	}
}  /* expand_definetab */

/*****************************************************************************
 *
 *  EXPAND_STMTTAB()
 *
 *  Description:	Expand the stmttab because it's full.
 *
 *  Called by:		first_pass_tree_walk()
 *			reg_alloc_first_pass()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	maxstmtno
 *			stmttab[]
 *
 *  Globals Modified:	maxstmtno
 *			stmttab[]
 *
 *  External Calls:	ckrealloc()
 *
 *****************************************************************************
 */

LOCAL void expand_stmttab()
{
    long incr;
    incr = maxstmtno >> 2;	/* bump by 25% */
    if (incr < 10)
	incr = 10;
    maxstmtno += incr;

    /* expand the definetab */
    stmttab = (STMT *) ckrealloc(stmttab, maxstmtno * sizeof(STMT));
}  /* expand_stmttab */

/*****************************************************************************
 *
 *  FIRST_PASS_INIT()
 *
 *  Description:	Allocate stmttab, definetab, reaching defs gen sets
 *			and live vars gen and kill sets
 *
 *  Called by:		reg_alloc_first_pass()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: in_reg_alloc_flag
 *			lastcom
 *			lastep
 *			lastfarg
 *			lastfilledsym
 *			lastvarno
 *			ncalls
 *			nexprs
 *			numblocks
 *
 *  Globals Modified:	definetab
 *			genlive
 *			genreach
 *			killlive
 *			laststmtno
 *			maxdefine
 *			maxdefs
 *			maxstmtno
 *			outreachep
 *			stmttab
 *
 *  External Calls:	ckalloc()
 *			new_set()
 *			new_set_n()
 *
 *****************************************************************************
 */

LOCAL void first_pass_init()
{
    register long nvars = lastvarno + 1;

    /* estimate the number of statements and allocate the stmttab */
    maxstmtno = nexprs + (nexprs >> 2);	  /* original # stmts + 25% */
    if (maxstmtno < 10)
	maxstmtno = 10;
    laststmtno = 0;			  /* entry 0 is blank -- start at 1 */
    stmttab = (STMT *) ckalloc(maxstmtno * sizeof(STMT));

    /* Estimate the number of defines and allocate the definetab.
     * Assume half of all statements are ASSIGNs, and each call defines all
     * COMMON and formal argument variable, and every variable is a formal
     * argument with an implicit def at each entry point (upper bound calc).
     * For C, assume each call defines all items in the ptrtab.
     */
    if (fortran)
	maxdefine = (maxstmtno>>1) + (lastcom + lastfarg + 2) * ncalls
			  + (lastfilledsym + 1) * (lastep + 1);
    else
	maxdefine = (maxstmtno>>1) + (lastptr + 1) * ncalls
			  + (lastfilledsym + 1) * (lastep + 1);

    if (maxdefine < 10)
	maxdefine = 10;
    if (in_reg_alloc_flag)
	maxdefs = maxdefine;		/* for use by mknewpreheader */
    lastdefine = 0;			/* entry 0 is blank -- start at 1 */
    definetab = (DEFTAB *) ckalloc(maxdefine * sizeof(DEFTAB));

    /* allocate genreach[] and outreachep[] */
    genreach = new_set_n(maxdefine, numblocks);
    outreachep = new_set_n(maxdefine, (lastep+1));

    /* allocate genlive[], killlive[], and inliveexit[] */
    if (in_reg_alloc_flag)
	{
	genlive = new_set_n(nvars, numblocks);
	killlive = new_set_n(nvars, numblocks);
	inliveexit = new_set(nvars);
	}

}  /* first_pass_init */

/*****************************************************************************
 *
 *  FIRST_PASS_TREE_WALK()
 *
 *  Description:	Walk each statement in a block tree in turn.  Assign
 *			statement numbers to each tree.  Remove null trees.
 *			Add entries to the stmttab[] vector.
 *
 *  Called by:		first_pass_tree_walk()  -- recursively
 *			reg_alloc_first_pass()
 *
 *  Input Parameters:	tree -- top of tree to be walked
 *
 *  Output Parameters:	return value -- 1 iff current tree is NULL
 *
 *  Globals Referenced:	laststmtno
 *			maxstmtno
 *
 *  Globals Modified:	laststmtno
 *			stmttab[] -- entry may be added
 *
 *  External Calls:	cerror()
 *			expand_stmttab()
 *			tfree()
 *			traverse_stmt_1()
 *
 *****************************************************************************
 */

LOCAL flag first_pass_tree_walk(tree)
NODE *tree;
{
    switch (tree->in.op)
	{
	case SEMICOLONOP:
		if (first_pass_tree_walk(tree->in.left))  /* empty tree */
		    {
		    register NODE *np;
		    np = tree->in.left;
		    --laststmtno;		/* changed our minds */
		    if (np->in.op == SEMICOLONOP)
			{
			tree->in.left = np->in.left;
			np->in.op = UNARY SEMICOLONOP;
			np->in.left = np->in.right;
			}
		    else
			{
			tree->in.left = tree->in.right;
			tree->in.op = UNARY SEMICOLONOP;
			}
		    tfree(np);
		    }
		/* fall into unary case */

	case UNARY SEMICOLONOP:
		tree->nn.stmtno = ++laststmtno;  /* assign stmtno */
		if (laststmtno >= maxstmtno)
		    expand_stmttab();
		stmttab[laststmtno].np = tree;
		stmttab[laststmtno].block = currblockno;
		stmttab[laststmtno].deleted = NO;
		stmttab[laststmtno].hassideeffects = tree->nn.haseffects;
		if (tree->in.op == SEMICOLONOP)
		    {
		    if (traverse_stmt_1(tree->in.right, tree) == YES)
			return (YES);
		    }
		else	/* UNARY SEMICOLONOP */
		    {
		    if (traverse_stmt_1(tree->in.left, tree) == YES)
			return (YES);
		    }
		break;

	default:
#pragma BBA_IGNORE
		cerror("bad tree in first_pass_tree_walk()");
	}
    return(NO);
}  /* first_pass_tree_walk */

/*****************************************************************************
 *
 *  MAKE_REACHING_KILL_SETS()
 *
 *  Description:	Allocate and fill the killreach[] sets
 *
 *  Called by:		reg_alloc_first_pass()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab[]
 *			genreach[]
 *			in_reg_alloc_flag
 *			maxdefine
 *			numblocks
 *
 *  Globals Modified:	killreach -- allocated and filled
 *
 *  External Calls:	ckalloc()
 *			delelement()
 *			nextel()
 *			new_set_n()
 *			setunion()
 *
 *****************************************************************************
 */

LOCAL void make_reaching_kill_sets()
{
    register long i;
    register long e;
    HASHU *sp;
    register SET *setp;

    /* allocate and fill the killreach[] sets */
    killreach = new_set_n(maxdefine, numblocks);
    for (i = 0; i < numblocks; ++i)
	{
	setp = killreach[i];

	/* for each def in the genreach[] set, find the corresponding symtab
	 * entry and defset.  For register allocation, all other defs for the
	 * variable go into the killreach[] set.  For dead store calculations,
	 * all other defs are killed iff a "definite" def is in the genreach
	 * set.
	 */

	e = -1;
	while ((e = nextel(e, genreach[i])) >= 0)
	    {
	    if (in_reg_alloc_flag || definetab[e].isdefinite)
		{
	        sp = definetab[e].sym;
	        setunion(sp->an.defset, setp);
	        delelement(e, setp, setp);
		}
	    }
	}

}  /* make_reaching_kill_sets */

/*****************************************************************************
 *
 *  MAKE_VAR_DEF_SETS()
 *
 *  Description:	For each variable with defs (varno > 0), create a
 *			defset showing which definition numbers correspond to
 *			the variable
 *
 *  Called by:		reg_alloc_first_pass()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab[]
 *			lastdefine
 *			maxdefine
 *
 *  Globals Modified:	none
 *
 *  External Calls:	adelement()
 *			new_set()
 *
 *****************************************************************************
 */

LOCAL void make_var_def_sets()
{
    register long i;
    register HASHU *sp;

    for (i = 1; i <= lastdefine; ++i)
	{
	sp = definetab[i].sym;
	if (sp->an.defset == NULL)
	    sp->an.defset = new_set(maxdefine);
	adelement(i, sp->an.defset, sp->an.defset);
	}
}  /* make_var_def_sets */

/*****************************************************************************
 *
 *  PROCESS_SIMPLE_DEF()
 *
 *  Description:	Process a "simple" variable definition.  Add def to 
 *			definetab and genreach[].  If no previous uses of var
 *			in block, add var to killlive[].  Add DU block to
 *			symbol table entry
 *
 *  Called by:		add_array_element_memory_defuses()
 *			process_array_def()
 *			process_call()
 *			process_common_def()
 *			process_def()
 *			process_farg_addr_def()
 *			process_farg_def()
 *
 *  Input Parameters:	sp -- symtab entry of item being def'd
 *			parent -- tree node containing defining operator
 *			deftype -- REGISTER- or MEMORY-type def
 *			isdefinite -- is the a "definite" def?
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	currblockno
 *			definetab[]
 *			genlive[]
 *			genreach[]
 *			in_reg_alloc_flag
 *			killlive[]
 *			lastdefine
 *			laststmtno
 *			stmttab[]
 *
 *  Globals Modified:	genreach[]
 *			killlive[]
 *
 *  External Calls:	add_def_block()
 *			add_definetab_entry()
 *			adelement()
 *			calculate_savings()
 *			delelement()
 *			xin()
 *
 *****************************************************************************
 */

LOCAL void process_simple_def(sp, parent, deftype, isdefinite)
register HASHU *sp;
NODE *parent;
long deftype;
long isdefinite;
{
    long savings;
    long prevdef;

    /* is this a good variable or not ?? */
    if (sp->an.varno == 0)
	{
	if (pass1_collect_attributes)
	    stmttab[laststmtno].hassideeffects = YES;
	return;
	}

    /* if previous reaching def is in this block, remove it from genreach[] */
    if ((prevdef = sp->an.lastdefno) > 0)
	{
	if (stmttab[definetab[prevdef].stmtno].block == currblockno
	 && (in_reg_alloc_flag || isdefinite))
	    {
	    delelement(sp->an.lastdefno, genreach[currblockno],
		       genreach[currblockno]);
	    }
	}

    /* add this def to the definetab and to the genreach[] set */
    add_definetab_entry(sp, parent, laststmtno, isdefinite);
    adelement(lastdefine, genreach[currblockno], genreach[currblockno]);
    sp->an.lastdefno = lastdefine;

    /* if variable not yet live on block entry, add it to the kill set */
    if (in_reg_alloc_flag && !xin(genlive[currblockno], sp->an.varno))
	adelement(sp->an.varno, killlive[currblockno], killlive[currblockno]);

    /* create the du block and attach to symtab entry */
    if (in_reg_alloc_flag)
	{
	if (((sp->an.tag == XN) || (sp->an.tag == X6N)
	  || (sp->an.tag == SN) || (sp->an.tag == S6N))
	 && (deftype != MEMORY))
	  if (mc68040)
            savings = reg_A_savings_factor;
          else
	    savings = 4;
	else
	    {
            savings = calculate_savings(sp->an.register_type, parent, deftype);
	    if (mc68040 && fortran && sp->an.farg)
	      savings += 1;
	    }
	savings >>= block_not_on_main_path;
	}
    add_def_block(sp, deftype, laststmtno, savings, lastdefine, isdefinite,
		  parent);

}  /* process_simple_def */

/*****************************************************************************
 *
 *  PROCESS_SIMPLE_USE()
 *
 *  Description:	Add a DU block to a symbol table entry, add an element
 *			to the blocks live vars gen set
 *
 *  Called by:		add_array_element_memory_defuses()
 *			process_array_def()
 *			process_array_use()
 *			process_call()
 *			process_def()
 *			process_farg_def()
 *			process_use()
 *
 *  Input Parameters:	sp -- symtab entry of item being use'd
 *			parent -- tree node of operator
 *			usetype -- REGISTER- or MEMORY-type use
 *			isdefinite -- is this a "definite" use??
 *			hidden_use -- does a definite use "hide" a maybe use?
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	currblockno
 *			definetab[]
 *			genlive[]
 *			in_reg_alloc_flag
 *			killlive[]
 *			stmttab[]
 *
 *  Globals Modified:	genlive[]
 *
 *  External Calls:	add_use_block()
 *			adelement()
 *			calculate_savings()
 *			xin()
 *
 *****************************************************************************
 */

LOCAL void process_simple_use(sp, parent, usetype, isdefinite, hidden_use)
register HASHU *sp;
NODE *parent;
long usetype;
long isdefinite;
long hidden_use;
{
    long reachdef = 0;
    long savings;
    long lastdef;

    /* is this a good variable or not ?? */
    if (sp->an.varno == 0)
	{
	if (pass1_collect_attributes)
	    stmttab[laststmtno].hassideeffects = YES;
	return;
	}

    /* ignore memory uses of constants */
    if (sp->an.isconst && (usetype == MEMORY))
	return;

    if ((parent->in.op == FORCE) && (sp->a6n.register_type != REG_A_SAVINGS)
	&& (sp->a6n.register_type != REG_D_SAVINGS))
	usetype = MEMORY;

    /* add variable to this block's genlive set iff it's not already in
     * the killlive set
     */
    if (in_reg_alloc_flag && !xin(killlive[currblockno], sp->a6n.varno))
	adelement(sp->a6n.varno, genlive[currblockno], genlive[currblockno]);

    /* if previous def of this variable was in current block, then that's
     * the only one that reaches this use
     */
    lastdef = sp->a6n.lastdefno;
    if ((lastdef > 0)
     && (stmttab[definetab[lastdef].stmtno].block == currblockno))
	reachdef = lastdef;

    /* calculate savings */
    if (in_reg_alloc_flag)
	{
	if (((sp->an.tag == XN) || (sp->an.tag == X6N)
	  || (sp->an.tag == SN) || (sp->an.tag == S6N)) && (usetype != MEMORY))
	  if (mc68040)
            savings = reg_A_savings_factor;
          else
	    savings = 4;
	else
	    {
            savings = calculate_savings(sp->an.register_type, parent, usetype);
	    if (mc68040 && fortran && sp->an.farg)
	      savings += 1;
	    }
	savings >>= block_not_on_main_path;
	}
	
    /* add the DU block to the symtab entry */
    add_use_block(sp, usetype, laststmtno, savings, reachdef, isdefinite,
		hidden_use, parent);
}  /* process_simple_use */

/*****************************************************************************
 *
 *  REALLOC_DEFINETAB()
 *
 *  Description:	Reallocate the definetab, genreach[] and outreachep[]
 *			sets if grossly overallocated ( > 25% waste ).
 *
 *  Called by:		reg_alloc_first_pass()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab
 *			genreach[]
 *			lastdefine
 *			lastep
 *			maxdefine
 *			numblocks
 *			outreachep[]
 *
 *  Globals Modified:	definetab
 *			genreach[]
 *			maxdefine
 *			outreachep[]
 *
 *  External Calls:	FREEIT()
 *			ckrealloc()
 *			memcpy()
 *			new_set()
 *
 *****************************************************************************
 */

LOCAL void realloc_definetab()
{
    register long newmax;
    register long i;
    long threshold;

    threshold = lastdefine + (lastdefine >> 2);

    if (threshold > maxdefine)
	return;			/* not grossly overallocated, do nothing */

    newmax = maxdefine = lastdefine + 1;   /* allocate exactly what is needed */

    /* reallocate the definetab, genreach[] and outreachep[]] sets */
    definetab = (DEFTAB *) ckrealloc(definetab, newmax * sizeof(DEFTAB));

    for (i = 0; i < numblocks; ++i)
	{
		change_set_size( genreach[i], newmax );
	}

    for (i = 0; i <= lastep; ++i)
	{
		change_set_size( outreachep[i], newmax );
	}
    
}  /* realloc_definetab */

/*****************************************************************************
 *
 *  REG_ALLOC_FIRST_PASS()
 *
 *  Description:	Allocate stmttab, definetab, gen and kill sets.
 *			Make first pass through the statement trees, filling
 *			the stmttab, definetab, gen and kill sets and finding
 *			variable defs and uses.
 *
 *  Called by:		register_allocation()   -- in register.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	dfo[]
 *			laststmtno
 *			maxstmtno
 *			numblocks
 *
 *  Globals Modified:	currblockno
 *			laststmtno
 *			pass1_collect_attributes
 *			simple_def_proc
 *			simple_use_proc
 *			stmttab -- may have entry added
 *
 *  External Calls:	add_entry_defs()
 *			add_exit_uses()
 *			block()
 *			expand_stmttab()
 *			first_pass_init()
 *			first_pass_tree_walk()
 *			make_reaching_kill_sets()
 *			make_var_def_sets()
 *			realloc_definetab()
 *			tfree()
 *
 *****************************************************************************
 */

void reg_alloc_first_pass()
{
    register long i;		/* blocknumber */
    register BBLOCK *bp;	/* blockpointer */
    long lastexitsym;
    HASHU **exitsyms;

    first_pass_init();		/* allocate tables and sets */

    pass1_collect_attributes = YES;
    simple_use_proc = process_simple_use;
    simple_def_proc = process_simple_def;
    block_not_on_main_path = 0;

    /* add defs for COMMON, static vars and array base addresses for each
     * procedure entry point.  Find list of candidates for exit uses.
     */
    add_entry_defs(&exitsyms, &lastexitsym);

    /* perform the tree traversal */
    for (i = 0; i < numblocks; ++i)
        {
	bp = dfo[i];
	bp->bb.firststmt = laststmtno+1;
	currblockno = bp->bb.node_position;
	if (in_reg_alloc_flag)
	    {
	    currregiondesc = bp->bb.region;
	    if (currregiondesc == topregion)
		{
		if ((exitblockno < 0) || xin(dom[exitblockno], i))
		    block_not_on_main_path = 0;
		else
		    block_not_on_main_path = 1;
		}
	    else
		{
		if (currregiondesc->regioninfo)
		    {
		    if (xin(dom[currregiondesc->regioninfo->backbranch.srcblock], i))
		        block_not_on_main_path = 0;
		    else
		        block_not_on_main_path = 1;
		    }
		}
	    }

	if (bp->bb.treep)		/* skip null blocks */
	    {
	    if (first_pass_tree_walk(bp->bb.treep))  /* last tree is nil */
	        {
	        /* remove the null tree */
	        register NODE *np;

	        np = bp->bb.treep;
	        if (np->in.op == SEMICOLONOP)
		    {
		    --laststmtno;
		    bp->bb.treep = np->in.left;
		    np->in.op = UNARY SEMICOLONOP;
		    np->in.left = np->in.right;
	            tfree(np);
		    }
		/* Don't remove the NOCODEOP if this would 
                 * create an empty basic block.  There is
		 * one empty basic block created for each
		 * routine. Don't create any empty basic
		 * blocks by removing all statements. Each 
		 * basic block must have at least one stmt 
		 * even if it is a NOCODEOP. 
		 */
	        }
	    }
	if (bp->bb.breakop == EXITBRANCH)
	    {
	    if (bp->bb.treep == NULL)	/* exit block must have stmt */
		{
	        register NODE *p;

	        p = block(NOCODEOP, 0, 0, INT);
	        bp->bb.treep = block(UNARY SEMICOLONOP, p, 0, INT);
	        bp->bb.treep->nn.stmtno = ++laststmtno;
	        if (laststmtno >= maxstmtno)
		    expand_stmttab();
	        stmttab[laststmtno].np = bp->bb.treep;
	        stmttab[laststmtno].block = currblockno;
	        stmttab[laststmtno].hassideeffects = NO;
	        stmttab[laststmtno].deleted = NO;
	        }
	    }

	bp->bb.nstmts = laststmtno - bp->bb.firststmt + 1;
  	}

    array_forms_fixed = YES;

    realloc_definetab();	/* realloc definetab size and defs sets if
				 * grossly overallocated.
				 */

    make_var_def_sets();	/* create define sets for each var */

    add_exit_uses(exitsyms, lastexitsym);

    make_reaching_kill_sets();	/* create killreach sets */

}  /* reg_alloc_first_pass */
