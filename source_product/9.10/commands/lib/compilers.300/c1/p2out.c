/* file p2out.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)p2out.c	16.1 90/11/05 */

# include "c1.h"

# define P2BUFFMAX 128
# define FOUR 4

#ifdef COUNTING
long _n_FPA881_loops;
long _n_procs_passed;
long _n_procs_optzd;
long _n_cbranches_folded;
long _n_int_const_prop;
long _n_real_const_prop;
long _n_unreached_blocks_deleted;
long _n_copy_prop;
long _n_dead_stores_removed;
long _n_do_loops;
long _n_for_loops;
long _n_block_merges;
long _n_empty_do_loops;
long _n_dbra_loops;
long _n_loops_unrolled;
long _n_consts_folded;
long _n_nodes_constant_collapsed;
long _n_comops_rewritten;
long _n_vfes_emitted;
long _n_real_consts_emitted;
long _n_colored_webloops;
long _n_colored_vars;
long _n_subsumed_webloops;
long _n_pruned_register_vars;
long _n_regions;
long _n_blocks_between_blocks;
long _n_loads;
long _n_stores;
long _n_preheaders;
long _n_loadstores_between_blocks;
long _n_loadstores_in_preheaders;
long _n_loadstores_within_blocks;
long _n_farg_translations;
long _n_mem_to_reg_rewrites;
long _n_farg_prolog_moves_removed;
long _n_FPA881_loads;
long _n_FPA881_stores;
long _n_aryelems;
long _n_common_regions;
long _n_comelems;
long _n_fargs;
long _n_ptr_targets;
long _n_structelems;
long _n_symtab_entries;
long _n_symtab_infos;
long _n_vfe_calls_deleted;
long _n_vfe_calls_replaced;
long _n_anon_vfes;
long _n_empty_vfes;
long _n_oneblock_vfes;
long _n_multiblock_vfes;
long _n_vfe_vars;
long _n_overlapping_regions;
long _n_nonreducible_procs;
long _n_uninitialized_vars;
long _n_useful_cses;
long _n_cses;
long _n_code_motions;
long _n_strength_reductions;
long _n_gcps;
long _n_cm_deletenodes;
long _n_cm_minors;
long _n_cm_cmnodes;
long _n_cm_safecalls;
long _n_cm_dis;
long _n_cm_combines;
long _n_bivs;
long _n_aivs;
long _n_minor_aivs;
long _n_constructed_aivs;
#endif COUNTING

long maxtempintreg;
long maxtempfloatreg;

extern short maxregs[];
extern flag haseffects();

LOCAL int p2buff[P2BUFFMAX];
LOCAL int *p2bufp		= &p2buff[0];
LOCAL int *p2bufend	= &p2buff[P2BUFFMAX];
LOCAL BBLOCK *currbp;
LOCAL ushort previntmask;
LOCAL ushort prevf881mask;
LOCAL ushort prevfpamask;
LOCAL ushort totalintmask;
LOCAL ushort totalf881mask;
LOCAL ushort totalfpamask;
LOCAL flag infpa881mode;

LOCAL void stmtout();
LOCAL void p2str();
LOCAL void fix_arrayforms_and_nodeout();
LOCAL void nodeout();
LOCAL void idiomcheck();
LOCAL void p2entry();
LOCAL void put_type();
LOCAL void p2text();

#ifdef DEBUGGING
	LOCAL NODE *prevstmt;
	LOCAL void p2stmtregscomment();
	LOCAL void p2blockcomment();
#endif DEBUGGING



/******************************************************************************
*
*	p2flush()
*
*	Description:		p2flush() writes the p2buff buffer onto the
*				output file.
*
*	Called by:		p2word()
*				p2pass()
*				prmaxlabblank()
*				prmaxlab()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	p2buff
*				p2bufp
*				outfile
*
*	Globals modified:	p2bufp
*
*	External calls:		fwrite()
*				cerror()
*
*******************************************************************************/
void p2flush()
{
	register  n = p2bufp - p2buff;

	if(n > 0)
		if (fwrite(p2buff, sizeof(int), n, outfile) != n)
#pragma BBA_IGNORE
			cerror("p2flush() error");
	p2bufp = p2buff;
}






/******************************************************************************
*
*	p2word()
*
*	Description:		p2word puts out a LONG onto the intermediate
*				code file and checks for a full buffer. 
*
*	Called by:		p2triple()
*				p2pass()
*				nodeout()
*				prmaxlabblank()
*				prmaxlab()
*				p2emitvfes()
*				p2str()
*
*	Input parameters:	w - a 4-byte int (anything) to be output.
*
*	Output parameters:	none
*
*	Globals referenced:	debugp
*				p2bufp
*				pdebug
*				p2bufend
*
*	Globals modified:	p2bufp
*
*	External calls:		fprintf()
*
*******************************************************************************/
void p2word(w)
int w;
{
# ifdef DEBUGGING 
	if (pdebug > 1)
		fprintf(debugp, "\t\t\t\t\t>>>>>p2word(0x%x)\n", w);
# endif DEBUGGING
*p2bufp++ = w;
if(p2bufp >= p2bufend)
	p2flush();
}




/******************************************************************************
*
*	p2triple()
*
*	Description:		p2triple puts out 3 quantities onto the
*				intermediate code buffer.
*			order:
*				first byte		  	  last byte
*				------------------------------------------
*				|  btype (word) | var (byte) | op (byte) |
*				------------------------------------------
*
*	Called by:		p2pass()
*				stmtout()
*				nodeout()
*				prmaxlabblank()
*				prmaxlab()
*				p2entry()
*				p2emitvfes()
*				p2blockcomment()
*				p2stmtregscomment()
*
*	Input parameters:	op - a uchar corresponding to FOP(x) in f77pass1
*				     and  c1.c.
*				var - a uchar corresponding to VAL(x) in
*				     f77pass1 and c1.c.
*				btype - a uchar corresponding to REST(x) in
*				     f77pass1 and c1.c.
*
*	Output parameters:	none
*
*	Globals referenced:	pdebug
*				debugp
*
*	Globals modified:	none
*
*	External calls:		fprintf()
*
*******************************************************************************/
LOCAL void p2triple(op, var, btype)
uchar op, var;
ushort btype;
{
	register word;

# ifdef DEBUGGING
	if (pdebug) 
		fprintf(debugp, "\t\t\t\t\t>>>>>p2triple(op=%s (0x%x),var=0x%x,btype=0x%x)\n",
		xfop(op), op, var, btype);
# endif DEBUGGING

	word = ((int)op) | (((int)var)<<8);
	word |= ( (int) btype) <<16;
	p2word(word);
}


/******************************************************************************
*
*	p2pass()
*
*	Description:		p2pass() is the overall driver to output
*				functions, their headers, their basic blocks,
*				and their epilogue information back onto the
*				output file.
*
*	Called by:		main()
*
*	Input parameters:	x - a copy of the original FRBRAC record
*				xbuf - an array containing structured valued
*				       function information (if any). It is
*				       usually not interesting.
*
*	Output parameters:	none
*
*	Globals referenced:	previntmask
*				prevf881mask
*				prevfpamask
*				totalintmask
*				totalf881mask
*				totalfpamask
*				prevstmt
*				top_preheader
*				fortran
*				last_vfe_thunk
*				blockpool
*				lastblock
*				fdebug
*				global_disable
*				reg_disable
*				pseudolbl
*				maxtempintreg
*				maxtempfloatreg
*				fpaflag
*
*	Globals modified:	previntmask
*				prevf881mask
*				prevfpamask
*				totalintmask
*				totalf881mask
*				totalfpamask
*				prevstmt
*				pseudolbl
*				maxtempintreg
*				maxtempfloatreg
*
*	External calls:		walkf()
*
*******************************************************************************/
void p2pass(x, xbuf)	int x, xbuf[];
{
	register BBLOCK *bp;
	NODE *np, *gp;
	LCRACKER ll;
	unsigned exitlabel;
	PLINK *pl;

	previntmask = 0;
	prevf881mask = 0;
	prevfpamask = 0;
	totalintmask = 0;
	totalf881mask = 0;
	totalfpamask = 0;
	infpa881mode = NO;

#ifdef DEBUGGING
	prevstmt = NULL;
#endif

	p2triple(FEXPR, (strlen(filename) + 3) / 4, 0);
	p2str(filename);

	if (top_preheader)
		{
		/* if the top block belongs to a region that has a preheader,
		   the preheader must be executed first, even before block[0].
		   Hence an immediate jump to the preheader is mandated.
		*/
		p2triple(GOTO, YES, 0);
		p2word(top_preheader->b.l.val);
		}

	if (fortran && (last_vfe_thunk > -1))  /* there are FORMAT stmts ?? */
	    p2emitvfes();

	ll.s.op = LABEL;
	exitlabel = 0;

	for (bp = blockpool; bp <= lastblock; bp++)
		{
		if (bp->bb.deleted)
			continue;

		if (bp->bb.entries)
			p2entry(bp->bb.entries);

		currbp = bp;		/* set for use by nodeout() */

		np = bp->bb.treep;

		/* Put out the block's label(s). */
		ll.s.l = bp->bb.l.val;
		p2word(ll.x);
		pl = bp->bb.l.pref_llistp;
		while (pl)
			{
			ll.s.l = pl->val;
			p2word(ll.x);
			pl = pl->next;
			}

		if ( np )
			{
# ifdef DEBUGGING 
			if (fdebug)
				{
				p2blockcomment((global_disable && reg_disable) ?
					bp - blockpool : bp->bb.node_position,
					bp->bb.treep);
				}
# endif DEBUGGING
			/* Check for machine dependent improvements */
			walkf(np, idiomcheck);

			/* Check for branches that go to the next block and
			   discard them.
			*/
			if (bp->bb.breakop==LGOTO && bp->bb.lbp==bp+1
			  && !((bp+1)->bb.treep && (bp+1)->bb.treep->in.entry))
				{
				gp = (np->in.op == SEMICOLONOP)?
					np->in.right : np->in.left;
				gp->in.op = FREE;
				}

			/* Postfix output the block's tree */
			stmtout(np);
			}

		/* If the "fall-thru" block on a CBRANCH is no longer
		   consecutive, actively jump to it.
		*/
		if (bp->bb.breakop==CBRANCH && bp->bb.lbp != bp+1 )
			{
			p2triple(GOTO, YES, 0);
			p2word(bp->bb.lbp->bb.l.val);
			}
		if ((bp->bb.breakop == EXITBRANCH)
		 && (bp != lastblock))	     /* redundant GOTO if last block */
			{
			if (!exitlabel) exitlabel = pseudolbl++;
			p2triple(GOTO, YES, 0);
			p2word(exitlabel);
			}
		}

	if (infpa881mode)
		{
		p2triple(C1881CODEGEN, NO, 0);	/* turn off */
		infpa881mode = NO;
		}

	if (exitlabel)
		{
		ll.s.l = exitlabel;
		p2word(ll.x);
		}
	p2word(x);		/* the original FRBRAC long word */
	if (VAL(x))
		{
		p2word(xbuf[0]);/* the original struct valued function stuff */
		p2word(xbuf[1]);/* ditto */
		}
	p2flush();
}



/******************************************************************************
*
*	p2name()
*
*	Description:		puts a char string (i.e. a name) onto the output
*				buffer. It should usually follow a p2triple()
*				call with a NAME or FENTRY op.
*
*	Called by:		nodeout()
*				p2entry()
*				main()
*
*	Input parameters:	cp - a ptr to a char string (null terminated)
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
void p2name(cp)	register char *cp;
{
	register int i;
	union
		{
		int word;
		char s[sizeof(int)];
		} w;
	
	w.word = 0;
	i = 0;
	while( w.s[i++] = *cp++ )
		if ( i >= sizeof(int) )
			{
			p2word(w.word);
			w.word = 0;
			i = 0;
			}
	if (i > 0)
		p2word(w.word);
}


/******************************************************************************
*
*	incrdecr()
*
*	Description:		Rewrites subtrees of the appropriate shape to
*				use the ++ or -- operators. Makes for faster
*				assembly code.
*
*	Called by:		idiomcheck()
*
*	Input parameters:	np - A node ptr to a MINUS or a PLUS node with
*					at least something of the appropriate
*					shape underneath.
*
*	Output parameters:	none
*				returns a ptr to a revised subtree.
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		same()
*				nncon()	{actually a macro}
*
*******************************************************************************/
LOCAL NODE *incrdecr(np, newop)	register NODE *np; uchar newop;
{
	register NODE *testp;

	testp = np->in.left->in.right;
	if (nncon(testp) && (same(np->in.right, testp, NO, NO)))
		{
		testp->in.op = FREE;
		testp = np->in.left->in.left;
		np->in.op = newop;
		LO(np) = FREE;
		np->in.left = testp;
		}
	return(np);
}

/******************************************************************************
*
*	idiomcheck()
*
*	Description:		idiomcheck() checks for simple patterns in the
*				trees that may be rewritten to be more efficient
*				for the S300 in particular.
*
*	Called by:		p2pass()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		same()
*				tfree()
*
*******************************************************************************/
LOCAL void idiomcheck(np)	NODE *np;
{
	NODE *rp, *sp;
	int i;

	switch (np->in.op)
		{
		case ASSIGN:
			/* Check for the op= one destroyed by oreg2plus().  */
			switch (RO(np))
				{
				case PLUS:
				case MUL:
				case AND:
				case OR:
				case ER:
					/* commutative ops */
					sp = np->in.right->in.right;
					if (same(np->in.left, sp, NO, NO)
				 		&& !haseffects(sp))
						{
						rp = np->in.right;
						np->in.op = ASG rp->in.op;
						np->in.right = rp->in.left;
						rp->in.op = FREE;
						tfree(sp);
						break;
						}
					/* fall thru */
				case MINUS:
				case DIV:
				case MOD:
				case LS:
				case RS:
					sp = np->in.right->in.left;
					if (same(np->in.left, sp, NO, NO)
				 		&& !haseffects(sp))
						{
						rp = np->in.right;
						np->in.op = ASG rp->in.op;
						np->in.right = rp->in.right;
						rp->in.op = FREE;
						tfree(sp);
						}
					break;
				}
			break;

		case PLUS:
			if (LO(np) == ASG MINUS)
				np = incrdecr(np, DECR);
			/* Subq is faster than add.l */
			else
				{
				if (nncon(np->in.left) && LV(np) <= -1 &&
				 LV(np) >= -8)
					{
					LV(np) = - LV(np);
					np->in.op = MINUS;
					SWAP(np->in.left, np->in.right);
					}
				else if (nncon(np->in.right) && RV(np) <= -1 &&
				 RV(np) >= -8)
					{
					RV(np) = - RV(np);
					np->in.op = MINUS;
					}
				if (np->in.op == MINUS && LO(np) == ASG PLUS)
					np = incrdecr(np, INCR);
				}
			break;

		case MINUS:
			if (LO(np) == ASG PLUS)
				np = incrdecr(np, INCR);
			break;
		case LE:
		case LT:
		case GE:
		case GT:
		case ULE:
		case ULT:
		case UGE:
		case UGT:
			if ( (( LO(np) == ICON) && (np->in.left->in.name == 0)) ||
				((RO(np) == REG) && (LO(np) != REG)) )
                    	{ 
                    	/* Better code is generated when ICON is right operand
			   or register is left operand.
			*/
	
			SWAP(np->in.left, np->in.right);
			np->in.op = revrel[np->in.op - EQ];
		    	}
			break;
		
		case MUL:
			if (nncon(np->in.left))
				SWAP(np->in.left, np->in.right);
			if (nncon(np->in.right))
				{
				i = RV(np);
				if ((i == (i & -i)) && (i != 0)) 
					/* an exact power of 2 */
					{
					np->in.op = LS;
					np->in.right->in.type.base = INT;
					RV(np) = ispow2(i);
					}
				}
			break;
		}
}

/*****************************************************************************
 *
 *  STMTOUT()
 *
 *  Description:	Walk a block's tree in statement order.  Output the
 *			register masks if they are different from the
 *			previous statement's.  Output the tree in postfix
 *			order.
 *
 *  Called by:		p2pass()
 *
 *  Input Parameters:	np -- current statement node (SEMICOLONOP)
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	previntmask
 *			prevf881mask
 *			prevfpamask
 *			fpaflag
 *
 *  Globals Modified:	previntmask
 *			prevf881mask
 *			prevfpamask
 *			totalintmask
 *			totalrealmask
 *
 *  External Calls:	p2triple()
 *			stmtout()
 *			walkf()
 *
 *****************************************************************************
 */
LOCAL void stmtout(np)
register NODE *np;
{
    /* walk the statements in left-to-right order */
    if (np->in.op == SEMICOLONOP)
	stmtout(np->in.left);

#ifdef DEBUGGING
    if (rodebug && !reg_disable)
	p2stmtregscomment(np);
#endif

    if (np->nn.isfpa881stmt && ! infpa881mode)
	{
	p2triple(C1881CODEGEN, YES, 0);
	infpa881mode = YES;
	}
    else if (infpa881mode && ! np->nn.isfpa881stmt)
	{
	p2triple(C1881CODEGEN, NO, 0);
	infpa881mode = NO;
	}
    if (! reg_disable) /* Don't put out SETREGS if not registers allocated */
      /* print the register masks, if different */
      if ((np->nn.intmask != previntmask) || (np->nn.f881mask != prevf881mask)
       || (np->nn.fpamask != prevfpamask))
	  {
	  p2triple(SETREGS, 0, np->nn.intmask);
	  if (fpaflag && saw_dragon_access)
	      p2triple(np->nn.f881mask, 0, np->nn.fpamask);
	  else
	      p2triple(np->nn.f881mask, 0, 0);
	  previntmask = np->nn.intmask;
	  prevf881mask = np->nn.f881mask;
	  prevfpamask = np->nn.fpamask;
	  }

    /* print the tree */
    if (np->in.op == SEMICOLONOP)
	{
	/* Remove useless self-assignments */
	if (np->in.right->in.op == ASSIGN
	 && same(np->in.right->in.left, np->in.right->in.right, NO, NO) )
	    {
	    tfree(np->in.right);
	    }
	else
	    {
	    fix_arrayforms_and_nodeout(np->in.right);

	    /* top it with FEXPR (for the UNARY SEMICOLONOP or SEMICOLONOP) */
	    p2triple(FEXPR, 0, np->nn.source_lineno);
	    }
	}
    else
	{
	/* Remove useless self-assignments */
	if (np->in.left->in.op == ASSIGN
	 && same(np->in.left->in.left, np->in.left->in.right, NO, NO) 
	 && (!np->in.left->in.left->tn.equivref))
	    {
	    tfree(np->in.left);
	    }
	else
	    {
	    fix_arrayforms_and_nodeout(np->in.left);

	    /* top it with FEXPR (for the UNARY SEMICOLONOP or SEMICOLONOP) */
	    p2triple(FEXPR, 0, np->nn.source_lineno);
	    }
	}

    np->in.op = FREE;
}  /* stmtout */

/******************************************************************************
*
*	fix_arrayforms_and_nodeout()
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
*	External calls:		fix_arrayforms_and_nodeout() {RECURSIVE}
*				fix_array_form()
*				nodeout()
*
*******************************************************************************/
LOCAL void fix_arrayforms_and_nodeout(np)
register NODE *np;
{
    short opty;
    register NODE *l;
    register NODE *base;
    NODE *t;

    opty = optype(np->in.op);
    if (opty == UTYPE)
	{
	if (np->in.isarrayform)
	    goto arrayform;
unary:
	fix_arrayforms_and_nodeout(np->in.left);
	}
    else if (opty == BITYPE)
	{
	if (np->in.isarrayform)
	    {
arrayform:
	    if (np->in.op == UNARY MUL)
		l = np->in.left;
	    else
		l = np;
	    if ((l->in.op == MINUS) && (l->in.left->in.op == PLUS) &&
		(l->in.right->in.op == ICON))
		{
		l->in.op = PLUS;
		l->in.right->tn.lval = - l->in.right->tn.lval;
		}
	    if ((l->in.op == PLUS) && (l->in.left->in.op == PLUS))
		{
		if ((l->in.right->in.op != ICON) || l->in.right->atn.name)
		    {
		    fix_array_form(l);
		    if ((l->in.op != PLUS) || (l->in.left->in.op != PLUS))
			{
			switch(optype(np->in.op))
			    {
			    case LTYPE:  goto leaf;
			    case UTYPE:  goto unary;
			    case BITYPE: goto binary;
			    }
			}
		    }
		t = l->in.left;
		base = t->in.left;
		if ((base->in.op == FOREG) || (base->in.op == ICON))
		    {
		    base->tn.lval += l->in.right->tn.lval;
		    l->in.right->in.op = FREE;
		    l->in.right = t->in.right;
		    l->in.left = base;
		    t->in.op = FREE;
		    }
		else
		    {
		    t->in.left = l->in.right;
		    l->in.right = t;
		    l->in.left = base;
		    }
		}
	    }
	if (optype(np->in.op) == UTYPE)
	    goto unary;
binary:
	fix_arrayforms_and_nodeout(np->in.left);
	fix_arrayforms_and_nodeout(np->in.right);
	}
leaf:
    nodeout(np);
}  /* fix_arrayforms_and_nodeout */

/******************************************************************************
*
*	nodeout()
*
*	Description:		Directs the emission all records to the output
*				file for a NODE. Then it frees the node.
*
*	Called by:		fix_arrayforms_and_nodeout()
*				p2emitvfes()
*
*	Input parameters:	np - a NODE ptr.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		p2triple()
*				put_type()
*				p2word()
*				p2name()
*				free()	{FREEIT}
*
*******************************************************************************/
LOCAL void nodeout(np)	register NODE *np;
{
	int i;
	int nodefaultlabs;	/* count of explicit labels for computed goto */

	switch (np->in.op)
		{
		case NAME:
			p2triple(NAME, (np->tn.lval != 0), np->tn.type.base);
			put_type(np);
			if (np->tn.lval)
				p2word(np->tn.lval);
			p2name (np->atn.name);
			break;
		case ICON:
			p2triple(ICON, np->atn.name != NULL, np->tn.type.base);
			put_type(np);
			p2word(np->tn.lval);
			if (np->atn.name)
				p2name(np->atn.name);
			break;
		case LGOTO:
			/* if it's a GOTO to the following block, suppress it */
			if ((currbp->bb.lbp != currbp+1)
			 || ((currbp+1)->bb.treep
			  && (currbp+1)->bb.treep->in.entry))
			    {
			    p2triple(GOTO, YES, 0);
			    p2word(np->bn.label);
			    }
			break;
		case UCM:
			/* UCM not recognized by f1. By discarding it we
			   implicitly reattach its child to the CM above it.
			*/
		case NOCODEOP:
		case FREE:
			break;
		case OREG:
			p2triple(OREG, np->tn.rval,np->in.type.base );
			put_type(np);
			p2word(np->tn.lval);
			break;
		case FOREG:
			/* reconstruct the original subtree */
			p2triple(REG, np->tn.rval, np->tn.type.base);
			put_type(np);
			p2triple(ICON, 0, INT);
			p2word(np->tn.lval);
			p2triple(PLUS, np->tn.rval, np->tn.type.base);
			put_type(np);
			break;
		case SEMICOLONOP:
		case UNARY SEMICOLONOP:
			p2triple(FEXPR, 0, 0);
			break;
		case FCOMPGOTO:
			nodefaultlabs = np->cbn.nlabs - 1;
			if (fortran)
				{
				p2triple(FCOMPGOTO, 0, nodefaultlabs);
				}
			else	/* SWITCH */
				{
				nodefaultlabs++;	/* all labels for C */
				p2triple(SWTCH, np->cbn.switch_flags,
					nodefaultlabs);
				}
			for (i = 0; i < nodefaultlabs; i++)
				{
				if (! fortran)
					p2word(np->cbn.ll[i].caseval);
				p2word(np->cbn.ll[i].val);
				}

			/* emit goto to fall-through computed-goto label if
				it's not the very next block
			 */
			if (fortran &&
			    (currbp->cg.ll[nodefaultlabs].lp
				!= &(currbp+1)->bb.l))
			    {
			    p2triple(GOTO, YES, 0);
			    p2word(currbp->cg.ll[nodefaultlabs].val);
			    }

			FREEIT(np->cbn.ll);
			break;
		case GOTO:	/* ASSIGNED GOTO */
			FREEIT(np->cbn.ll);
			p2triple(GOTO, NO, 0);
			break;

		case DBRA:
			p2triple(DBRA, np->in.index, 0);
			break;

		case STASG:
		case STARG:
		case FLD:
		case STCALL:
		case UNARY STCALL:
			p2triple(np->sin.op, np->sin.stalign,np->sin.type.base);
			put_type(np);
			p2word(np->sin.stsize);
			break;

		default:
			p2triple(np->in.op, np->tn.rval,np->in.type.base );
			put_type(np);
			break;
		}
	if (np->in.hiddenvars && callop(np->in.op))
		FREEIT(np->in.hiddenvars);
	np->in.op = FREE;
}	/* nodeout */


/******************************************************************************
*
*	put_type()
*
*	Description:		emits a properly formed TWORD struct to pass2.
*
*	Called by:		nodeout()
*
*	Input parameters:	np - a NODE ptr to a subtree.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		p2word()
*
*******************************************************************************/
LOCAL void put_type(np)	NODE *np;
{
	union
		{
		struct 
			{
			long mods2:14;
			long fill :18;
			}x;
		long m;
		} mod2type;

	if ( TMODS1(np->tn.type.base) )
		{
		p2word(np->in.type.mods1);
		if ( TMODS2(np->tn.type.base) )
			{
			mod2type.x.mods2 = np->tn.type.mods2;
			p2word(mod2type.m);
			}
		}
}


/******************************************************************************
*
*	prmaxlabblank()
*
*	Description:		emit blank template for max-label-used info
*					-- filled in at EOF.
*
*	Called by:		main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		p2triple()
*				p2word()
*				p2flush()
*
*******************************************************************************/
void prmaxlabblank()
{
	p2triple(FMAXLAB,0,0);
	p2word(0);
	p2flush();
}


/******************************************************************************
*
*	prmaxlab()
*
*	Description:		Fills in max-label-used template emitted at
*				start.
*
*	Called by:		main()
*
*	Input parameters:	maxlab - maximum label value used in c1.
*
*	Output parameters:	none
*
*	Globals referenced:	outfile
*
*	Globals modified:	none
*
*	External calls:		p2flush()
*				fseek()
*				p2triple()
*				p2word()
*
*******************************************************************************/
void prmaxlab(maxlab)
int maxlab;
{
	p2flush();
	fseek(outfile,0,0);
	p2triple(FMAXLAB,0,0);
	p2word(maxlab);
	p2flush();
	fseek(outfile,2,0);
}

/******************************************************************************
*
*	p2entry()
*
*	Description:		p2entry() regurgitates the ENTRY name
*				information back into the intermediate file at
*				the proper file position for correct sequencing
*				in pass 2.
*
*	Called by:		p2pass()
*
*	Input parameters:	pp - a ptr to a linked list of ENTRY info.
*
*	Output parameters:	none
*
*	Globals referenced:	ep
*
*	Globals modified:	none
*
*	External calls:		p2triple()
*				p2name()
*
*******************************************************************************/
LOCAL void p2entry(pp)
register PLINK *pp;
{
    while (pp)
	{
	if (pp->val != 0)	/* ignore main subprogram entry point */
	    {
	    p2triple(FENTRY, 0, 0);
	    p2name(ep[pp->val].b.name);
	    }
	pp = pp->next;
	}
}

/*****************************************************************************
 *
 *  P2EMITVFES()
 *
 *  Description:	emit the variable expression FORMAT "thunks"
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
 *  External Calls:	p2str()
 *
 *****************************************************************************
 */

LOCAL p2emitvfes()
{
	register struct vfethunk *vp;
	register struct vfethunk *vend;
	char buff[P2BUFFMAX];
	long size;
	long label;
	LCRACKER ll;
	register VFECODE *vcp;
	VFECODE *vcpnext;
	

	/* emit branch around thunks */
	label = pseudolbl++;
	p2triple(GOTO, YES, 0);
	p2word(label);
	ll.s.op = LABEL;

	for (vp = vfe_thunks, vend = &(vfe_thunks[last_vfe_thunk]); 
	     vp <= vend;
	     vp++)
	    {
	    if (vfes_to_emit && (vp->asg_fmt_target || (vp->nblocks > 1)))
		{
		size = sprintf(buff, "%s:", vp->label);
		size = (size + 3) / 4;
		p2triple(FTEXT, size, 0);
		p2str(buff);
		for (vcp = vp->thunk; vcp;
		     vcpnext = vcp->next, FREEIT(vcp), vcp = vcpnext)
		    {
		    ll.s.l = vcp->labval;
		    p2word(ll.x);
		    walkf(vcp->np, nodeout);
		    tfree(vcp->np);
		    }
		p2triple(VAREXPRFMTEND,0,0);
#ifdef COUNTING
		_n_vfes_emitted++;
#endif
		}
	    else
		{
		size = sprintf(buff, "\tset\t%s,0", vp->label);
		size = (size + 3) / 4;
		p2triple(FTEXT, size, 0);
		p2str(buff);
		}
	    }

	ll.s.l = label;
	p2word(ll.x);
}	/* p2emitvfes */


/******************************************************************************
*
*	p2str()
*
*	Description:		Emit a string (suitably packaged) to the output
*				file.
*
*	Called by:		p2pass()
*				p2emitvfes()
*				p2text()
*
*	Input parameters:	s - char ptr to the string.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		p2word()
*
*******************************************************************************/
LOCAL void p2str(s)
register char *s;
{
register int i;
union { long int word; char str[FOUR]; } u;

i = 0;
u.word = 0;
while(*s)
	{
	u.str[i++] = *s++;
	if(i == FOUR)
		{
		p2word(u.word);
		u.word = 0;
		i = 0;
		}
	}
if(i > 0)
	p2word(u.word);
}




/******************************************************************************
*
*	fname
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
LOCAL void p2text(s)
register char *s;
{
    long length;

    length = (strlen(s) + 3) / 4;
    p2triple(FTEXT, length, 0);
    p2str(s);
}  /* p2text */

/*****************************************************************************
 *
 *  EMIT_REAL_CONST()
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

emit_real_const(label, tree)
long label;
NODE *tree;
{
    char buff[160];
    typedef struct quad QUAD;
    struct quad { int i1,i2,i3,i4; };
    extern QUAD _U_Qfcnvxf_sgl_to_quad();
    extern QUAD _U_Qfcnvxf_usgl_to_quad();
    union {
        float f;
        double d;
	QUAD dd;
	long i[4];
	long l;
	unsigned u;
	} cons, val;
    flag unsigned_val;

#ifdef COUNTING
    _n_real_consts_emitted++;
#endif

    p2text("\tlalign\t1");
    p2text("\tdata");
    sprintf(buff, "L%d:", label);
    p2text(buff);
    switch (tree->in.left->tn.type.base)
	{
	case UNSIGNED:
	case ULONG:
	case USHORT:
	case UCHAR:
		unsigned_val = YES;
		val.u = (unsigned) tree->in.left->tn.lval;
		break;
	default:
		unsigned_val = NO;
		val.l = tree->in.left->tn.lval;
		break;
	}
	switch(tree->tn.type.base)
		{
		case FLOAT:
			if (unsigned_val)	/* work around code-gen bug for ?: */
	    			cons.f = val.u;
			else
	    			cons.f = val.l;
			sprintf(buff, "\tlong\t0x%x", cons.i[0]);
			break;
		case DOUBLE:
			if (unsigned_val)
				cons.d = val.u;
			else
				cons.d = val.l;
			sprintf(buff,"\tlong\t0x%x,0x%x", cons.i[0], cons.i[1]);
			break;
		case LONGDOUBLE:
			if (unsigned_val)
				cons.dd = _U_Qfcnvxf_usgl_to_quad(val.u);
			else
				cons.dd = _U_Qfcnvxf_sgl_to_quad(val.l);
			sprintf(buff,"\tlong\t0x%x,0x%x,0x%x,0x%x",
				cons.i[0], cons.i[1], cons.i[2], cons.i[3]);
			break;
		default:
#pragma BBA_IGNORE
			cerror("bad type in emit_real_const()");
		}
		p2text(buff);
    p2text("\ttext");
}  /* emit_real_const */

#ifdef COUNTING

/*****************************************************************************
 *
 *  WRITE_COUNTS()
 *
 *  Description:
 *
 *  Called by:
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	countp
 *			all the count variables
 *
 *  Globals Modified:
 *
 *  External Calls:	fprintf()
 *
 *****************************************************************************
 */

void write_counts()
{
    fprintf(countp, "_n_procs_passed = %d\n", _n_procs_passed);
    fprintf(countp, "_n_procs_optzd = %d\n", _n_procs_optzd);
    fprintf(countp, "_n_cbranches_folded = %d\n", _n_cbranches_folded);
    fprintf(countp, "_n_int_const_prop = %d\n", _n_int_const_prop);
    fprintf(countp, "_n_real_const_prop = %d\n", _n_real_const_prop);
    fprintf(countp, "_n_unreached_blocks_deleted = %d\n", _n_unreached_blocks_deleted);
    fprintf(countp, "_n_copy_prop = %d\n", _n_copy_prop);
    fprintf(countp, "_n_dead_stores_removed = %d\n", _n_dead_stores_removed);
    fprintf(countp, "_n_do_loops = %d\n", _n_do_loops);
    fprintf(countp, "_n_for_loops = %d\n", _n_for_loops);
    fprintf(countp, "_n_block_merges = %d\n", _n_block_merges);
    fprintf(countp, "_n_empty_do_loops = %d\n", _n_empty_do_loops);
    fprintf(countp, "_n_dbra_loops = %d\n", _n_dbra_loops);
    fprintf(countp, "_n_loops_unrolled = %d\n", _n_loops_unrolled);
    fprintf(countp, "_n_consts_folded = %d\n", _n_consts_folded);
    fprintf(countp, "_n_nodes_constant_collapsed = %d\n", _n_nodes_constant_collapsed);
    fprintf(countp, "_n_comops_rewritten = %d\n", _n_comops_rewritten);
    fprintf(countp, "_n_vfes_emitted = %d\n", _n_vfes_emitted);
    fprintf(countp, "_n_real_consts_emitted = %d\n", _n_real_consts_emitted);
    fprintf(countp, "_n_colored_webloops = %d\n", _n_colored_webloops);
    fprintf(countp, "_n_colored_vars = %d\n", _n_colored_vars);
    fprintf(countp, "_n_subsumed_webloops = %d\n", _n_subsumed_webloops);
    fprintf(countp, "_n_pruned_register_vars = %d\n", _n_pruned_register_vars);
    fprintf(countp, "_n_regions = %d\n", _n_regions);
    fprintf(countp, "_n_blocks_between_blocks = %d\n", _n_blocks_between_blocks);
    fprintf(countp, "_n_loads = %d\n", _n_loads);
    fprintf(countp, "_n_stores = %d\n", _n_stores);
    fprintf(countp, "_n_preheaders = %d\n", _n_preheaders);
    fprintf(countp, "_n_loadstores_between_blocks = %d\n", _n_loadstores_between_blocks);
    fprintf(countp, "_n_loadstores_in_preheaders = %d\n", _n_loadstores_in_preheaders);
    fprintf(countp, "_n_loadstores_within_blocks = %d\n", _n_loadstores_within_blocks);
    fprintf(countp, "_n_farg_translations = %d\n", _n_farg_translations);
    fprintf(countp, "_n_mem_to_reg_rewrites = %d\n", _n_mem_to_reg_rewrites);
    fprintf(countp, "_n_farg_prolog_moves_removed = %d\n", _n_farg_prolog_moves_removed);
    fprintf(countp, "_n_FPA881_loops = %d\n", _n_FPA881_loops);
    fprintf(countp, "_n_FPA881_loads = %d\n", _n_FPA881_loads);
    fprintf(countp, "_n_FPA881_stores = %d\n", _n_FPA881_stores);
    fprintf(countp, "_n_aryelems = %d\n", _n_aryelems);
    fprintf(countp, "_n_common_regions = %d\n", _n_common_regions);
    fprintf(countp, "_n_comelems = %d\n", _n_comelems);
    fprintf(countp, "_n_fargs = %d\n", _n_fargs);
    fprintf(countp, "_n_ptr_targets = %d\n", _n_ptr_targets);
    fprintf(countp, "_n_structelems = %d\n", _n_structelems);
    fprintf(countp, "_n_symtab_entries = %d\n", _n_symtab_entries);
    fprintf(countp, "_n_symtab_infos = %d\n", _n_symtab_infos);
    fprintf(countp, "_n_vfe_calls_deleted = %d\n", _n_vfe_calls_deleted);
    fprintf(countp, "_n_vfe_calls_replaced = %d\n", _n_vfe_calls_replaced);
    fprintf(countp, "_n_anon_vfes = %d\n", _n_anon_vfes);
    fprintf(countp, "_n_empty_vfes = %d\n", _n_empty_vfes);
    fprintf(countp, "_n_oneblock_vfes = %d\n", _n_oneblock_vfes);
    fprintf(countp, "_n_multiblock_vfes = %d\n", _n_multiblock_vfes);
    fprintf(countp, "_n_vfe_vars = %d\n", _n_vfe_vars);
    fprintf(countp, "_n_overlapping_regions = %d\n", _n_overlapping_regions);
    fprintf(countp, "_n_nonreducible_procs = %d\n", _n_nonreducible_procs);
    fprintf(countp, "_n_uninitialized_vars = %d\n", _n_uninitialized_vars);
    fprintf(countp, "_n_cses = %d\n", _n_cses);
    fprintf(countp, "_n_useful_cses = %d\n", _n_useful_cses);
    fprintf(countp, "_n_code_motions = %d\n", _n_code_motions);
    fprintf(countp, "_n_strength_reductions = %d\n", _n_strength_reductions);
    fprintf(countp, "_n_gcps = %d\n", _n_gcps);
    fprintf(countp, "_n_cm_deletenodes = %d\n", _n_cm_deletenodes);
    fprintf(countp, "_n_cm_minors = %d\n", _n_cm_minors);
    fprintf(countp, "_n_cm_cmnodes = %d\n", _n_cm_cmnodes);
    fprintf(countp, "_n_cm_safecalls = %d\n", _n_cm_safecalls);
    fprintf(countp, "_n_cm_dis = %d\n", _n_cm_dis);
    fprintf(countp, "_n_cm_combines = %d\n", _n_cm_combines);
    fprintf(countp, "_n_bivs = %d\n", _n_bivs);
    fprintf(countp, "_n_aivs = %d\n", _n_aivs);
    fprintf(countp, "_n_minor_aivs = %d\n", _n_minor_aivs);
    fprintf(countp, "_n_constructed_aivs = %d\n", _n_constructed_aivs);

}  /* write_counts */
#endif COUNTING

# ifdef DEBUGGING


LOCAL void p2blockcomment(dfonum, hex)	unsigned short dfonum;
{
	char lbuf[P2BUFFMAX];
	int lsize;

	lsize = sprintf(lbuf, "#\t\tblock[%d] (treep=0x%x)", dfonum, hex);
	lsize = ((unsigned)(lsize + 3))/4;
	p2triple(FTEXT, lsize, 0);
	p2str(lbuf);
}

/*****************************************************************************
 *
 *  P2STMTREGSCOMMENT()
 *
 *  Description:	Emit statement number and register usage comments into
 *			the assembly file.  The amount of information is
 *			controlled by the -Ro (rodebug) flag.
 *			  rodebug = 1		statement # only
 *			  rodebug = 2		statement # + register changes
 *			  rodebug = 3		stmt # + register usage every
 *						   statement + reg changes
 *
 *  Called by:		stmtout()
 *
 *  Input Parameters:	np -- pointer to current SEMICOLONOP node
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	flibflag
 *			fpaflag
 *			prevnode
 *			regclasslist
 *			rodebug
 *
 *  Globals Modified:	prevnode
 *
 *  External Calls:	p2str()
 *			p2triple()
 *			sprintf()
 *
 *****************************************************************************
 */
LOCAL void p2stmtregscomment(np)
NODE *np;
{
    char lbuf[P2BUFFMAX];
    int lsize;
    register HASHU *sp;
    register long reg;
    register long regclass;
    flag prevanon;
    flag curranon;
    ushort prevint;
    ushort prevf881;
    ushort prevfpa;
    register REGCLASSLIST *plist;
    long prevreg;
    long currreg;
    register ushort prevmask;
    register ushort currmask;

    /* emit the statement number */
    if (np->nn.stmtno == LOADSTORE)
        lsize = sprintf(lbuf, "#\t\tstmt #<ADDED LOAD/STORE>");
    else if (np->nn.stmtno > 0)
        lsize = sprintf(lbuf, "#\t\tstmt #%d", np->nn.stmtno);
    else
        lsize = sprintf(lbuf, "#\t\tstmt #<ADDED GOTO>");
    lsize = ((unsigned)(lsize + 3))/4;
    p2triple(FTEXT, lsize, 0);
    p2str(lbuf);

    /* emit the register contents */
    if (rodebug > 1)
	{
	if (prevstmt == NULL)
	    {
	    prevanon = YES;
	    prevint = 0;
	    prevf881 = 0;
	    prevfpa = 0;
	    }
	else if ((prevstmt->nn.stmtno ==0) || (prevstmt->nn.stmtno== LOADSTORE))
	    {
	    prevanon = YES;
	    prevint = prevstmt->nn.intmask;
	    prevf881 = prevstmt->nn.f881mask;
	    prevfpa = prevstmt->nn.fpamask;
	    }
	else
	    prevanon = NO;

	if ((np->nn.stmtno == 0) || (np->nn.stmtno == LOADSTORE))
	    curranon = YES;
	else
	    curranon = NO;

	for (regclass = 0; regclass < NREGCLASSES; regclass++)
	    {
	    switch (regclass)
		{
		case INTCLASS:
			prevmask = prevint >> 8;
			currmask = np->nn.intmask >> 8;
			break;
		case ADDRCLASS:
			prevmask = prevint;
			currmask = np->nn.intmask;
			break;
		case F881CLASS:
			prevmask = prevf881;
			currmask = np->nn.f881mask;
			break;
		case FPACLASS:
			prevmask = prevfpa;
			currmask = np->nn.fpamask;
			break;
		}
	    if (flibflag && ((regclass == F881CLASS) || (regclass == FPACLASS)))
		continue;

	    plist = regclasslist + regclass;

	    for (reg = 0; reg < plist->nregs; ++reg)
		{
		if (prevanon == NO)
		    prevreg = plist->regmap[(prevstmt->nn.stmtno - 1)
						* plist->nregs + reg];
		else
		    prevreg = (prevmask & 1) ? -1 : 0;

		if (curranon == NO)
		    currreg = plist->regmap[(np->nn.stmtno - 1)
						* plist->nregs + reg];
		else
		    currreg = (currmask & 1) ? -1 : 0;

		prevmask >>= 1;
		currmask >>= 1;

		if (((currreg == 0) && (prevreg == 0))
		 || ((rodebug < 3)
		  && (((currreg == -1) && (prevreg == -1))
		   || ((currreg > 0) && (prevreg > 0)
		    && (plist->colored_vars[currreg].var
				== plist->colored_vars[prevreg].var)))))
		    continue;	/* nothing to print */

		lsize = sprintf(lbuf, "#\t\t%s%d = ",
			(regclass == ADDRCLASS) ? "A" :
			(regclass == INTCLASS) ? "D" :
			(regclass == F881CLASS) ? "FPA" : "FP",
			(regclass == ADDRCLASS) ? (5 - reg):
			(regclass == INTCLASS) ? (7 - reg):
			(regclass == F881CLASS) ? (7 - reg): (15 - reg));
			
		if (currreg == 0)
		    lsize += sprintf(lbuf+lsize,"<unused>");
		else if (currreg == -1)
		    lsize += sprintf(lbuf+lsize,"<in use>");
		else 	/* known variable */
		    {
	            sp = plist->colored_vars[currreg].var;
		    if (sp->an.pass1name)
			lsize += sprintf(lbuf+lsize, "\"%s\"  ",
					 sp->an.pass1name);
		    if (sp->an.tag==A6N || sp->an.tag == X6N || sp->an.tag == S6N)
			lsize += sprintf(lbuf+lsize, "A6N/X6N/S6N %d(a6),%d",
					sp->a6n.offset, sp->a6n.index);
		    else if ( sp->an.tag==AN || sp->an.tag == XN || sp->an.tag == SN )
			lsize += sprintf(lbuf+lsize, "AN/XN/SN %s+%d", sp->an.ap,
						sp->an.offset);
		    }
		lsize = ((unsigned)(lsize + 3))/4;
		p2triple(FTEXT, lsize, 0);
		p2str(lbuf);
		}
	    }
	}
    prevstmt = np;
}  /* p2stmtregscomment */

# endif DEBUGGING
