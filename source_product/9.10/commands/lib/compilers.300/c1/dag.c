/* file dag.c */
/* @(#) $Revision: 70.2 $ */
/* KLEENIX_ID @(#)dag.c	16.1.1.1 91/03/14 */

/* This file implements the common-subexpression algorithms described in
   "The Theory and Practice of Compiler Writing", by Tremblay and Sorenson,
   McGraw-Hill, 1985. See Section 12-4.
*/

# include "c1.h"

/* defines for constant propagation */
# define SONLY(p) (!(p->allo.flagfiller & ~SREF))
# define ISSIMPLE_LHS(p) (optype(p->in.op)==LTYPE && SONLY(p) && p->in.type.base==BTYPE(p->in.type)) /* no flags set */
# define CLEARCA	0	/* ambiguous write clears all const asgmts */
# define FIRSTCA	1	/* direct assignment of a constant */
# define FOLLOWINGCA	2	/* use of a constant-assigned var on rhs */
# define CP_CHEAPER(c)	(-128<=c && c<=127)	/* machine dependent */
# define INCRDECR(p) (p->in.right->in.op==ASSIGN && ISCONST(p->in.left) \
	&& ISSIMPLE_LHS(p->in.right->in.left) \
	&& ISCONST(p->in.right->in.right))
# define SEQABLE(op)	(!(dope[op] & NOSEQFLG))
# define CCSZ	3		/* common_child stack default size, increment */


NODE **seqtab; 		/* sequence table. seqtab[0] not used yet. */
CHILD *common_child;	/* stack for remembering common child nodes */

# ifdef DEBUGGING
    CHILD *max_common_child;
    unsigned topseq;
# else
LOCAL unsigned topseq;
# endif DEBUGGING
LOCAL CHILD *dup_child;	/* TOS for the common_child stack */
LOCAL NODE **replaced;	/* synonym for seqtab */
LOCAL int asg_count;	/* count of assignments for a particular lhs var. */
LOCAL NODE *asg_target; /* proposed lhs of a cse assignment */
LOCAL NODE *asg_csetemp;/* temp created for a cse */
LOCAL NODE *asg_tot;	/* top of current tree for cse */
LOCAL ushort blocknumber;
LOCAL flag call_seen;	/* YES iff a (U)CALL is present within the block */
LOCAL int maxccsz;

LOCAL void replacecommons();




/******************************************************************************
*
*	update_fargs_or_ptrs()
*
*       Description:            Update all formal arguments or all ptr targets
*				depending on the call.  It is not
*				correct to update only those in the same
*				argument list because called functions
*				may be nested.  All we know about formal
*				arguments is that they reside outside
*				local data space.  For all we know,
*				every formal argument could be an alias
*				for the same datum.
*
*	Called by:		assign()
*				redundant()
*
*	Input parameters:	newindex - the new index number to be given to
*					each formal argument in tab.
*				tab - the table of interest (fargtab or ptrtab)
*				lastindex - the index number of the last filled
*					entry in the tab
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*				(tab[])
*				(lastindex)
*
*	Globals modified:	symtab
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void update_fargs_or_ptrs(newindex, tab, lastindex)
	unsigned newindex, tab[], lastindex;
{
	register unsigned *up;

	for (up = &tab[lastindex]; up >= tab; up--)
		symtab[*up]->a6n.index = newindex;
}


/******************************************************************************
*
*	update_ces()
*
*       Description:            update_ces() updates the index number of
*				every available common element whenever
*				the index number of a formal argument is
*				updated because they may be aliases.
*
*	Called by:		assign()
*				redundant()
*
*	Input parameters:	u - the new index number to give to each
*				    common element.
*
*	Output parameters:	none
*
*	Globals referenced:	comtab
*				lastcom
*				symtab
*
*	Globals modified:	symtab
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void update_ces(u)	unsigned u;
{
	register unsigned *up;
	register CLINK *kp;

	for (up = &comtab[lastcom]; up >= comtab; up--)
		{
		symtab[*up]->a6n.index = u;
		for (kp = symtab[*up]->cn.member; kp; kp = kp->next)
			symtab[kp->val]->cn.index = u;
		}
}


/******************************************************************************
*
*	update_externs()
*
*	Description:		resets the index of each external in the extern
*				table.
*
*	Called by:		redundant()
*
*	Input parameters:	u - the new index number
*
*	Output parameters:	none
*
*	Globals referenced:	lastfilledsym
*				symtab
*
*	Globals modified:	symtab
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void update_externs(u)	unsigned u;
{
    register long i;
    register HASHU *sp;

    for (i = 0; i <= lastfilledsym; ++i)
	{
	sp = symtab[i];
	if (sp->an.seenininput		/* use seen in code */
	 && ((i <= lastexternsym)	/* declared outside of proc */
	  || (sp->an.isexternal)) )	/* or with "extern" attribute */
	    sp->an.index = u;
	}
}


/******************************************************************************
*
*	assign()
*
*	Description:		assign() is responsible for updating the index
*				numbers for the symtab entry corresponding to
*				the node pointed at by np if it's
*				simple, or the appropriate child of np, and all
*				related symtab entries.
*
*	Called by:		redundant()
*				addseq()
*				assign()
*
*	Input parameters:	np - a pointer into the syntax tree
*				u  - the new index number
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*
*	Globals modified:	symtab
*
*	External calls:		location()
*				find()
*
*******************************************************************************/
LOCAL void assign (np, u, cmplx_upd)	NODE *np; unsigned u; flag cmplx_upd;
{
	HASHU *sp; 
	int incr;

	/*
	 * DEW
	 * find the symbol table pointer for the correct node.
	 */
top:
	if (np->in.arrayref)
		sp = location(np);
	else
		switch (np->in.op)
		{
		case ICON:
		case FCON:
			if (!np->atn.name) return;
			/* fall thru */

		case STRING:
		case NAME:
		case REG:
		case OREG:
		case FOREG:
			/* lookup */
			sp = symtab[find(np)];
			break;

		case CM:
			np = np->in.right;
			goto top;

		case FLD:
		case UCM:
			np = np->in.left;
			goto top;

		case UNARY MUL:
			/* Working with an address rather than a value */
			if (np->in.isptrindir)
				update_fargs_or_ptrs(u, ptrtab, lastptr);
			np = np->in.left;
			goto top;

		default:
			/* do nothing */
			return; 
		}

	/* DEW
	 * update the symbols index #
	 */
	sp->a6n.index = u;

	/* DEW
	 * additional checking needs to take place for formals, pointers,
	 * and complexes when the index # has a real value.
	 */ 
	if (u != 0)
		{
#if 0 /* DEW 3/19/91 */
		if (sp->a6n.farg && (fortran || ISPTR(sp->a6n.type)))
			{
			/* An assignment to a formal argument could be an
			   assignment to any of the formal arguments or to
			   data in common. We must assume the worst case.
			*/
			if (fortran) update_ces(u);
			update_fargs_or_ptrs(u, fargtab, lastfarg);
			}
#endif 0
		/* DEW
		 * if this is a FORTRAN formal argument and !NO_PARM_OVERLAPS,
		 * all common and all formal arguments get a def.
		 * NO_PARM_OVERLAPS means that formal parameters do not refer
		 * to the same memory space as anything in common.
		 */
		if (fortran && sp->a6n.farg && !NO_PARM_OVERLAPS)
			{
			update_ces(u);
			update_fargs_or_ptrs(u, fargtab, lastfarg);
			}
		else if (sp->a6n.common && !NO_PARM_OVERLAPS)
			{
			/* An assignment to an element of common could be an
			   assignment to any of the formal arguments. However,
			   since common elements are guaranteed to be distinct,
			   it is not correct to assume that all common elements
			   are also updated in the worst case.
			*/
			update_fargs_or_ptrs(u, fargtab, lastfarg);
			}

		if (cmplx_upd && ISFTP(sp->a6n.type))
			{
			/* On a CALL or a (U)CM, the actual arguments may
			   contain complexes masquerading as floats or doubles.
			   The other half of the complex or doubles must be 
			   updated wrt to the index number as well.
			*/
			if (sp->a6n.complex1)
				{
				incr = (BTYPE(sp->a6n.type) == FLOAT)? 4 : 8;
				if (np->in.arrayref)
					/* Arrays will be invalidated for
					   other reasons.
					*/
					return;
				np->tn.lval += incr;
				assign(np, u, NO);
				np->tn.lval -= incr;
				}
			else if (sp->a6n.complex2)
				{
				incr = (BTYPE(sp->a6n.type) == FLOAT)? 4 : 8;
				if (np->in.arrayref)
					/* Arrays will be invalidated for
					   other reasons.
					*/
					return;
				np->tn.lval -= incr;
				assign(np, u, NO);
				np->tn.lval += incr;
				}
			}
		}
}



# if 0
/******************************************************************************
*
*	assign_struct()
*
*	Description:		A special purpose version of assign() to
*				update the index number of each member of
*				a struct in the event of a whole struct
*				assignment.
*
*	Called by:		redundant()
*
*	Input parameters:	np - a NODE ptr pointing to the lhs of the
*				STASG node.
*				ii - the new index number
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*
*	Globals modified:	symtab
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void assign_struct(np, ii)	NODE *np; unsigned ii;
{
	register CLINK *cp;

	cp = symtab[np->in.arrayrefno]->xn.member;
	while (cp)
		{
		symtab[cp->val]->a6n.index = ii;
		cp = cp->next;
		}
}
# endif 0


/******************************************************************************
*
*	operand_index()
*
*	Description:		retrieves the index number for a node in the
*				tree.
*
*	Called by:		redundant()
*				operand_index()	{recursively}
*
*	Input parameters:	np - a NODE pointer into the syntax tree
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*				ptrtab
*
*	Globals modified:	none
*
*	External calls:		find()
*
*******************************************************************************/
LOCAL operand_index (np) 	register NODE *np;
{
	register unsigned *pi;
	register rindex;
	HASHU *sp;
	int rr;
	long find_loc;

	if (np->in.arrayref)
		{
		/* array reference ... case 3 */
		rindex = 0;
		switch (optype(np->in.op))
			{
			case BITYPE:
				rindex = operand_index(np->in.right);
				if (rindex < 0)
					return(-1);
				/* fall thru */
			case UTYPE:
				rr = operand_index(np->in.left);
				if (rr < 0)
					return(-1);
				rindex = max(rindex, rr);
				rr = max(np->in.index, symtab[np->in.arrayrefno]->a6n.index);
				return ( max(rr, rindex) );
			case LTYPE:
				return(symtab[np->in.arrayrefno]->a6n.index);
			}
		}
	if (np->in.isptrindir)
		{
		/* All ptr targets are updated to the same index whenever
		   any of them is updated thru pointers. However, they may
		   be updated in other ways as well.
		*/
		rindex = 0;
		if (optype(np->in.op) == BITYPE)
			rindex = operand_index(np->in.right);
		rr = operand_index(np->in.left);
		if (rindex < 0 || rr < 0)
			return(-1);
		rindex = max(rindex, rr);

		/* DEW
		 * Now, if index # from any of the possible ptr targets
		 * is larger than this one, then use it instead.
		 */
		for (pi = &ptrtab[lastptr]; pi >= ptrtab; pi--)
			{
			rr = symtab[*pi]->a6n.index;
			rindex = max(rindex, rr);
			}
		return(rindex);
		}

	if (optype(np->in.op) == LTYPE)
		{
		/* const or var */
		switch (np->tn.op)
			{
			case ICON:
			case FCON:
				if (!np->atn.name) return (0);
				/* fall thru */

			case STRING:
			case NAME:
			case REG:
			case OREG:
			case FOREG:
				/* lookup */
				find_loc = find(np);
				if (find_loc != (unsigned) (-1))
				  {
				  sp = symtab[find_loc];
				  return ( sp->a6n.index );
				  }
				else
				  if (np->tn.arrayrefno != NO_ARRAY)
				    {
				    sp = symtab[np->tn.arrayrefno];
				    return ( sp->a6n.index );
				    }
				  else
				    return(-1);

			case CCODES:
			case LGOTO:
			case FREE:
					return (0);
			}
		}
	
	/* DEW
	 * its not an array reference, pointer, or a leaf.
	 * return the previously computed index # for this node.
	 */
	return ( np->tn.index );	/* retrieve */
}


/******************************************************************************
*
*	check()
*
*	Description:		check() implements the CHECK function
*				described in Tremblay & Sorenson, page 627.
*				It "... tests A, the current node under
*				examination, for formal identity against the
*				DAG nodes 1,2,...,j-1 and returns in k [kp]
*				either the sequence number of a matching node
*				with the matched node's index number in i [ip]
*				or the value zero if no match is found (with i
*				also set to 0)."
*
*	Called by:		redundant()
*
*	Input parameters:	np - a NODE pointer to a subtree
*				j
*
*	Output parameters: 	kp
*				ip
*
*	Globals referenced:	seqtab
*
*	Globals modified:	none
*
*	External calls:		same()
*
*******************************************************************************/
LOCAL void check(np, j, kp, ip)	register NODE *np; unsigned j;
	unsigned *kp, *ip;
{
	register unsigned q;
	register NODE *testp;

	/* DEW
	 * NOTE: book algorithm calls for a forward search of the
	 * dag nodes, which makes the most sense to me.  Here, however
	 * we are doing a reverse search.  Original reason for change is 
	 * that the for loop code generated on a reverse search is more
	 * efficient.  In my opinion, the time taken to execute the same
	 * routine (does a tree search) far outweighs the miniscule
	 * savings generated from the for loop reverse search.
	 */
	for (q = j - 1; q > 0; q--)
		{
		testp = seqtab[q];

		/* DEW
		 * if dagobsolete is set in testp, then this node has
		 * already been made into a CSE, can't use it.
		 * if !same, then different tree structure, can't use this
		 * one.
		 * if lhsiref is set, then have a lhs array ref. can't use.
		 */
		if ( testp->in.dagobsolete || !same(np, testp, YES, YES)
			|| testp->in.lhsiref || np->in.lhsiref)
			continue;

		/* DEW
		 * found a match.  return the current sequence number, and
		 * the index number of the matched node.
		 *
		 * NOTE:  Really should check index # first before walking
		 * down the tree (in same).  This could easily be accomplished
		 * by passing in the text index number.
		 */
		*kp = q;
		*ip = testp->in.index;
		return;
		}
	*kp = 0;
}



/* CALLs present sequencing problems for cses. In particular, it is not
   generally possible to ensure that temps will be filled before their
   use. kill_cses() prefix traverses the tree to turn off
   (by setting the callref bits) cse detection for any children.

   In the same manner, an lhs of an ASSIGN that is not LTYPE should not be
   a candidate for cse unless it is an arrayref in masquerade. Otherwise
   we get the impossible situation
			=
		      /  \
		    =     ...
		   / \

    after the redundant cse is eliminated.
*/
/******************************************************************************
*
*	kill_cses()
*
*	Description:		kill_cses() sets the callref flag on a NODE 
*				pointer and recursively on all subnodes from
*				that pointer to prevent detecting cses from
*				that point downward in the subtree.
*
*	Called by:		redundant()
*
*	Input parameters:	np - a NODE pointer that's the top of a subtree.
*				in - a flag to set np->tn.callref
*
*	Output parameters:	outl - a ptr to an int flag that will become 
*					the "in" flag on subsequent calls to
*					kill_cses() for the left child.
*				outr - a ptr to an int flag that will become
*					the "in" flag on subsequent calls to
*					kill_cses() for the right child.
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		hardops()
*
*******************************************************************************/
LOCAL void kill_cses(np, in, outl, outr)	register NODE *np;
							int in;
							int *outl, *outr;
{
	flag assumes;

	if ( optype(np->in.op) == LTYPE )
		np->tn.callref |= (in != 0);
	else
		switch (np->in.op)
		{
		case STCALL:
		case CALL:
			if (np->in.safe_call)
				/* Treat the call just like FMONADIC */
				goto def;
			assumes = !(np->in.no_arg_defs && np->in.no_com_defs);
			*outl = assumes;
			*outr = assumes;
			break;
		case STASG:
		case ASSIGN:
			np = np->in.left;
			if ( !(np->allo.flagfiller & (AREF|SREF|ISPTRINDIR))
				&& optype(np->in.op) != LTYPE )
				*outl = YES;
			*outr = in;
			break;
		default:
def:
			*outl = in;
			*outr = in;
			break;
		}
}


/******************************************************************************
*
*	symtabinit()
*
*	Description:		symtabinit() traverses the symtab to ensure
*				that every member's index is 0.
*
*	Called by:		redundant()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*				maxsymtsz
*				lastfilledsym
*
*	Globals modified:	symtab
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void symtabinit()
{
	register HASHU **sp;
	register HASHU **spstop;

	/* symtab initialization */
	for (spstop = symtab, sp = &symtab[lastfilledsym]; sp >= spstop; sp--)
		{
		(*sp)->a6n.index = 0;
		}
}

/******************************************************************************
*
*	reg_estimates()
*
*	Description:		Traverses the symtab looking for things that
*				are likely to end up in registers. If there
*				are many such items, we should be a little
*				more reluctant to add CSEs.
*
*	Called by:		local_optimizations()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*				lastfilledsym
*				acount
*				dcount
*
*	Globals modified:	acount
*				dcount
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void reg_estimates()
{
	register HASHU **sp;
	register HASHU **spstop;
	register HRTAG ltag;

	/* If CSE isn't done, then these will stay high (conservative). */
	acount = 0;
	dcount = 0;

	/* symtab initialization */
	for (spstop = symtab, sp = &symtab[lastfilledsym]; --sp >= spstop; )
		{
		/* Count things likely to be put into registers later */
		if ( (*sp)->a6n.seenininput )
			{
			ltag = (*sp)->a6n.tag;
			if ( ltag == XN || ltag == X6N )
				acount++;
			else if (ltag == A6N && !(*sp)->a6n.ptr)
				dcount++;
			}
		}
}

/******************************************************************************
*
*	redundant_cleanup()
*
*	Description:		cleans up the seqtab after completion of 
*				redundant().
*
*	Called by:		redundant()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	topseq
*				seqtab
*
*	Globals modified:	none
*
*	External calls:		tfree2()
*
*******************************************************************************/
LOCAL void redundant_cleanup()
{
	register unsigned ltopseq;

	/* DEW - 5/91 repair of defect FSDdt07343
	 * Not all dagobsolete nodes are freeable.  When the
	 * node can be freed, the freeable flag now is set.
	 * Only free nodes that are obsolete AND freeable.
	 */
	for (ltopseq = topseq; ltopseq >= 1; ltopseq--)
		if (seqtab[ltopseq]->in.dagobsolete &&
		    seqtab[ltopseq]->in.cse_node_freeable) /* DEW FSDdt07343 */
			tfree2(seqtab[ltopseq]);
}


/******************************************************************************
*
*	redundant()
*
*	Description:		redundant() is the basic driver for detecting
*				local CSEs.
*
*	Called by:		local_optimizations()
*
*	Input parameters:	np - a NODE ptr to the top of a basic block's 
*					tree.
*
*	Output parameters:	none
*
*	Globals referenced:	common_child
*				seqtab
*				ddebug
*				topseq
*
*	Globals modified:	dup_child
*				max_common_child
*
*	External calls:		fwalk()
*				walkf()
*				fprintf()
*				cerror()
*				eprint()
*				werror()
*				symtabinit()
*				addseq()
*
*******************************************************************************/
LOCAL void addseq();

LOCAL void redundant (np)	register NODE *np;
{
	register unsigned ltopseq;
	register i, ii;
	unsigned j, k, s;
	NODE *topnp = np;
	flag csefound = NO;
	flag opty;

	topseq = 0;
	dup_child = common_child - 1;	/* initialize the stack */

# ifdef DEBUGGING
	max_common_child = dup_child;
# endif DEBUGGING

	/* DEW
	 * initialize the index # of all variables in the symbol table
	 */
	symtabinit();

	/* DEW
	 * mark all expressions that are part of a call or pointer operation
	 */
	fwalk(np, kill_cses, 0);

	/* DEW
	 * build a sequence table.  Each entry is a node pointer in the
	 * post order traversal required by this algorithm.
	 */
	walkf(np, addseq);

# ifdef DEBUGGING
	if( ddebug>=3 )
		{
		fprintf(debugp, "in redundant(0x%x), after initial seq # assignment:\n",
			np);
		for (i = 1; i <= topseq; i++)
			fprintf(debugp,"\tseqtab[%d] = 0x%x\n", i, seqtab[i]);
		fprintf(debugp, "\n\n");
		}
# endif	DEBUGGING

	/* DEW
	 * j will hold the new sequence #s determined as we build the dag.
	 */
	j = 1;


	/* DEW
	 * as index #s start with 0, the sequence #s must start with 1.
	 */
	for (ltopseq = 1; ltopseq <= topseq; ltopseq++)
		{
		np = seqtab[ltopseq];
		opty = optype(np->in.op);

		/* DEW
		 * First check to see if last time around a cse was found.
		 * if so, then this node needs to be updated to point to 
		 * the common child.
		 */
		if (dup_child >= common_child )	/* a child was common */
			{
			/* this switch could conceivably be combined with
			   the following one but the purposes are distinct.
			*/
			/* I think it's correct to check rhs first here because
			   in the postfix traversal of the tree a common lhs
			   would be pushed first then the common rhs, if there
			   is one. At pop time the rhs would be on top. Confirm
			   this proposition later.
			*/
# ifdef DEBUGGING
			if (ddebug)
				fprintf(debugp,
					"redundant() found common sub = 0x%x, with parent = 0x%x\n",
					dup_child->np, seqtab[dup_child->k]);
# endif DEBUGGING
			csefound = YES;
			switch(opty)
				{
# ifdef DEBUGGING
				case LTYPE:
					cerror("impossible seqtab entry in redundant()");
					break;
# endif DEBUGGING
				case BITYPE:
					if (np->in.right == dup_child->np)
						{
						/* Attach common subexp tree */
						np->in.right = seqtab[dup_child->k];
						/* Mark the tree as common so
						that it isn't overfreed
						later.
						*/
						np->in.right->in.common_node = YES;
						np->in.right->ind.usage++;

						/* DEW - 5/91 FSDdt07343
						 * The dup_child node can now
						 * be freed, set the freeable
						 * flag.
						 */
						dup_child->np->in.cse_node_freeable = YES;
						dup_child--;
						if (dup_child < common_child)
							break;
						}
					/* fall thru */
				case UTYPE:
					if (np->in.left == dup_child->np)
						{
						/* Attach common subexp tree */
						np->in.left = seqtab[dup_child->k];
						/* Mark the tree as common so
						that it isn't overfreed
						later.
						*/
						np->in.left->in.common_node = YES;
						np->in.left->ind.usage++;

						/* DEW - 5/91 FSDdt07343
						 * The dup_child node can now
						 * be freed, set the freeable
						 * flag.
						 */
						dup_child->np->in.cse_node_freeable = YES;
						dup_child--;
						}
				}
			}

		/* DEW
		 * compute the index of this node.  The index
		 * value is stored in i.
		 */
		i = 0;
		switch ( opty )
			{
# ifdef DEBUGGING
			case LTYPE:
				/* Seqtab should have only non-LTYPE nodes */
				cerror("Impossible LTYPE seqtab entry in redundant().");
				break;
# endif DEBUGGING
			case BITYPE:
				i = operand_index(np->in.right);
				if (i < 0)
					/* Cease any further CSE checks because
					   an anonymous node has been seen.
					*/
					goto anonymous_operand;
				/* fall thru */
			case UTYPE:
				ii = operand_index(np->in.left);
				i = max(i, ii);
				if (np->allo.flagfiller & (AREF|ISPTRINDIR))
					{
					ii = operand_index(np);
					i = max(i, ii);
					}
				if (ii < 0)
					/* Cease any further CSE checks because
					   an anonymous node has been seen.
					*/
					/* Structure assignments will never
					   pass the COMMON_OP test below. 
					*/
					goto anonymous_operand;
				break;
			}
		i++;

		/*
		 * DEW
		 * check to see if this node looks the same as any of
		 * the nodes allready processed (determined by ltopseq).
		 * If a node is the same, k is the sequence number, and
		 * s is the index number.
		 */
		if (COMMON_OP(np->in.op))
			check(np, ltopseq, &k, &s); 	/* sets k and s */
		else k = 0;

		/* DEW
		 * If k is non-zero, then found a node that looks the same.
		 * if i and s are equal, then the index #s are the same,
		 * thus these nodes are common.
		 */
		if (k && i == s)
			{
			/* Set dup_child so that on next iteration of np we
			   can remember which child was common and relink
			   accordingly. Something akin to mark().
			*/
			push_common_child(np, k);
			np->in.dagobsolete = YES;
			/* seqtab[] is never updated so it will now point to
			   a dagobsolete node in np. We'll just never consider
			   this seqtab[] entry for CSE in check() hereafter.
			*/
			/* DEW - 5/91 FSDdt07343
			 * Make sure the freeable flag is off.  The node
			 * can not be freed until after its parent points
			 * to the common node.
			 */
			np->in.cse_node_freeable = NO;
			}
		else
			{
			/* DEW
			 * nodes are not the same, so save off the
			 * computed index # and the latest sequence
			 * number.
			 */
			np->in.dagobsolete = NO;/* insurance */
			np->ind.index = i;	/* index number */
			np->ind.seq = j;	/* new sequence number */

			/*
			 * DEW
			 * if this is an assignment operator, update
			 * the index # of the variable on the lhs with
			 * the current sequence #
			 */
# if 0
			if (np->in.op == STASG)
				assign_struct(np->in.left, j);
			else
# endif 0
			if ( asgop(np->in.op) )
				assign(np->in.left, j, NO);
			else if (np->in.op == CM || np->in.op == UCM)
				/* to prevent intermediate cses on formals,
				   etc. before the CALL node is traversed.
				*/
				{
#if 0 /* DEW 3/19/91 */
				if (!np->in.no_arg_defs)
					assign(np, j, YES);
#endif 0
				/* DEW
				 * If the argument is a pass by reference
				 * then def the argument.
				 * If the argument is a pointer, and it is
				 * not dereferenced, then def all ptr targets.
				 * NOTE: not necessary to def all ptr targets
				 * because CALL statements do that anyway.
				 */
				if (fortran && !np->in.no_com_defs)
					assign(np, j, YES);
				}
			else if (callop(np->in.op) && !np->in.safe_call) 
				/* DEW
				 * CALLs can update common (FORTRAN),
				 *       externs (C), formal args targets
				 *       (FORTRAN), or pointer targets (C).
				 */
				{

				/* DEW
				 * Handle special case missed above of just
				 * one call paramter, and its value is
				 * modified.
				 */
				if (np->in.op == CALL
					&& fortran /* DEW 3/19/91 */
					&& !np->in.no_com_defs
					&& np->in.right->in.op != CM
					&& np->in.right->in.op != UCM )
						assign(np->in.right, j, YES);

				/* A CALL causes a potential update of
				   all formals and all common elements,
				   not just the one immediately below
				   the CALL node.
				*/
				if (fortran)
					{
					if (!np->in.no_com_defs)
						update_ces(j);
					if (!np->in.no_arg_defs &&
					    !np->in.no_com_defs &&
					    !NO_PARM_OVERLAPS)
						update_fargs_or_ptrs(j, fargtab,
							lastfarg);
					}
				else if (!np->in.no_com_defs)
					/* DEW 3/19/91
					 * C supports no_com_defs too!
					 * #program NO_SIDE_EFFECTS
					 */
					update_externs(j);
				
				/* DEW 3/13/91
				 * A call can def ptr targets, therefore
				 * need to also update all ptr targets.
				 */
#ifdef FTN_POINTERS
				/* DEW
				 * Fortran pointers are handled differently.
				 * the no_com_defs flag in fortran means
				 * that fortran pointer target are not def'ed
				 * in the call, whereas in C this is not
				 * true.
				 */
				if (!fortran || !np->in.no_com_defs)
#endif
				update_fargs_or_ptrs(j, ptrtab, lastptr);

				}

			/* DEW
			 * increment the sequence number
			 */
			j++;
			}
		}


	/* cleanup stuff */
	if (dup_child >= common_child)	/* stack not empty */
#pragma BBA_IGNORE
		cerror("common_child stack left unempty in redundant()");

	/* DEW
	 * throw away all of the dagobsolete nodes in this blcok
	 */
	redundant_cleanup();
	topnp->in.csefound = csefound;
	return;

anonymous_operand:
# ifdef DEBUGGING
	if (verbose && np->in.op != STASG)
		werror("an anonymous operand was detected during CSE.");
# endif DEBUGGING
	redundant_cleanup();
	topnp->in.csefound = csefound;
	return;
}


/******************************************************************************
*
*	addseq()
*
*       Description:            addseq() initializes the tree nodes with
*				the sequence numbers and index numbers.
*				It also sets up the seqtab[].
*				Corresponding counters are updated as
*				well.
*
*	Called by:		redundant()
*
*	Input parameters:	np - a subtree NODE pointer
*
*	Output parameters:	none
*
*	Globals referenced:	maxseqsz
*				seqtab
*				dope[]	( SEQABLE() )
*
*	Globals modified:	seqtab
*				topseq
*
*	External calls:		ckrealloc()
*
*******************************************************************************/
LOCAL void addseq (np) register NODE *np;
{

	/* DEW
	 * initialize the node (interior/terminal) index # to 0
	 */
	np->tn.index = 0;

	if ( !SEQABLE(np->in.op) )	/* Includes all LTYPE nodes */
		{
		/* Take this opportunity to zero out indexes for terminals */
		/* DEW
		 * index number for leafs that have symbols are the symbol
		 * entries index #
		 */
		return;
		}
	if (np->in.op == SEMICOLONOP)
		{
		if (!SEQABLE(np->in.right->in.op))
			{
			return;
			}
		}

	else if (np->in.op == UNARY SEMICOLONOP)
		{
		if (!SEQABLE(np->in.left->in.op))
			{
			return;
			}
		}

	/*
	 * as index #s begin with 1, sequence #s must begin with 1.
	 * therefore, preincrement.
	 */
	if (++topseq >= maxseqsz)
		{
		maxseqsz += DAGSZ/4;
		seqtab = (NODE **)ckrealloc(seqtab, maxseqsz * sizeof(NODE *));
		}
	/* DEW
	 * if this is an array reference, initialize the index to 0 for
	 * the entire array.
	 */
	if (np->in.op == UNARY MUL && np->in.arrayref) assign(np, 0, NO);

	/*
	 * add this node to the sequence table
	 */
	seqtab[topseq] = np;


	if (callop(np->in.op) && !np->in.safe_call)
		/* Take this opportunity to set the call_seen flag. */
		call_seen = YES;
}

/******************************************************************************
*
*	push_common_child()
*
*	Description:		common_child stack push routine.
*
*	Called by:		redundant()
*
*	Input parameters:	np - a NODE pointer
*				k - a seqtab index
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	dup_child
*				maxccsz
*				common_child
*				max_common_child
*
*	External calls:		cerror()
*
*******************************************************************************/
LOCAL push_common_child ( np, k )	NODE *np; unsigned k;
{
	int i;

	if ( (++dup_child - common_child) >= maxccsz )
		{
		i = dup_child - common_child;
		maxccsz += CCSZ;
		common_child = (CHILD *)ckrealloc(common_child, 
				maxccsz * sizeof(CHILD));
		dup_child = common_child + i;
		}
	dup_child->np = np;
	dup_child->k = k;
# ifdef DEBUGGING
		if (dup_child > max_common_child)
		  max_common_child = dup_child;
# endif DEBUGGING
# ifdef COUNTING
	_n_cses++;
# endif COUNTING
}

/******************************************************************************
*
*	reduce_usage()
*
*       Description:            If a parent is found to be cost
*				effective for CSE, then children can
*				still be cost effective but their use in
*				computing the parent should not be
*				counted.
*
*	Called by:		reduce_usage()
*				rplc()
*
*	Input parameters:	np - a NODE pointer
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
LOCAL void reduce_usage(np)	NODE *np;
{
top:
	switch (optype(np->in.op))
		{
		case BITYPE:
			if (np->ind.usage)
				np->ind.usage--;
			reduce_usage(np->in.right);
			/* fall thru */
		case UTYPE:
			if (np->ind.usage)
				np->ind.usage--;
			np = np->in.left;
			goto top;	/* avoid one level of recursion */
		}
}

/******************************************************************************
*
*	rtcopy()
*
*	Description:		similar to tcopy() but does something special
*				when encountering a COMOP node.
*
*	Called by:		rplc()
*
*	Input parameters:	p - a NODE ptr to a subtree.
*
*	Output parameters:	none
*				(returns a copy of the tree, less any COMOPS)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		talloc()
*				rtcopy()	{recursive}
*
*******************************************************************************/
LOCAL NODE *rtcopy(p)	register NODE *p;
{
	register NODE *q;

	q = talloc();
	if (p->in.op == COMOP)
		*q = *(p->in.right);
	else
		{
		*q = *p;
		switch( optype(q->in.op) )
			{
			case BITYPE:
				q->in.right = rtcopy(q->in.right);
				/* fall thru */
			case UTYPE:
				q->in.left = rtcopy(q->in.left);
			}
		}
	q->in.temptarget = NO;
	return(q);
}

/******************************************************************************
*
*	rplc()
*
*	Description:		rplc() is a coroutine for replacecommons().
*
*	Called by:		replacecommons()
*
*	Input parameters:	np - NODE pointer.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		talloc()
*				rtcopy()
*				worthit()
*				mktemp()
*				block()
*				cptemp()
*				replacecommons()
*
*******************************************************************************/
LOCAL NODE *rplc(np, anp, CSEtmpflag) register NODE *np; NODE *anp;
                                      flag *CSEtmpflag;
{
	register NODE *rp;

	*CSEtmpflag = NO;
	if (np->ind.common_node)
		{
		rp = replaced[np->ind.seq];
		if ( !rp )
			{
			/* Previously identified as a temptarget but
			   not worth making it into a CSE.
			*/
			np = rtcopy(np);
			return(np);
			}

		if (rp->in.op == COMOP)
			{
			/* already replaced once */
# ifdef COUNTING
			_n_useful_cses++;
# endif COUNTING
			np = rtcopy(rp->in.right);
			*CSEtmpflag = YES;
			return(np);
			}

		if ( np->ind.temptarget )
			{
			flag w = NO;
			if ( np->in.op != COMOP && (anp != NULL) || (w = worthit(np)))
				{
				/* Either the cse replacement is "free" because
				a user's variable is used as the cse
				replacement (hence no store and load), or
				the cost is amortizable.
				 
				If anp != NULL, then the temp assignment
				will eventually be replaced by use of a
				user variable in fixnvlt().
				*/
# 	ifdef COUNTING
				_n_useful_cses++;
# 	endif COUNTING
				if (w)
					{
					if (ISPTR(np->in.type))
						acount++;
					else if (!ISFTP(np->in.type))
						dcount++;
					}
				rp = talloc();
				*rp = *np;
				np->ind.arrayref = NO;  /* so reg alloc doesnt*/
				np->ind.arrayelem = NO; /* get confused */
				np->in.op = COMOP;
				np->in.right = mktemp(0, np->in.type);
				np->in.left = block(ASSIGN, cptemp(np->in.right), 
						rp, 0, rp->in.type);
				reduce_usage(rp);
				if (optype(rp->in.op) != LTYPE)
					replacecommons(rp);
				*CSEtmpflag = YES;
				return(np);
				}
			else 
				{
				replaced[np->ind.seq] = 0;
				if (optype(np->in.op) != LTYPE)
					replacecommons(np);
				return(np);
				}
			}
		}

	if (optype(np->in.op) != LTYPE)
		replacecommons(np);
	return(np);
}

/******************************************************************************
*
*	find_asgs()
*
*	Description:		counts assignments to a variable within a
*				basic block by traversing the tree.
*
*	Called by:		comma_subsumable() via walkf()
*
*	Input parameters:	np - a NODE ptr.
*
*	Output parameters:	none
*
*	Globals referenced:	asg_target
*
*	Globals modified:	asg_count
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void find_asgs(np)	register NODE *np;
{
	if (np->in.op == ASSIGN && same(np->in.left, asg_target, NO, YES))
		asg_count++;
	else if (asg_count == 1 && callop(np->in.op) && !np->in.safe_call )
		/* Any subsequent call could use the local var is an actual
		   param. If so, then it should be recomputed.
	   	*/
		asg_count++;
}


/******************************************************************************
*
*	comma_subsumable()
*
*	Description:		determines if a special form of CSE rewrite
*				will create a subtree in which either a 
*				previously existing temp or a user-defined
*				variable is the CSE variable. In this case
*				no new temp will be needed. Hence no additional
*				store and load will be required for the new
*				temp during its definition and subsequent uses.
*
*	Called by:		replacecommons()
*
*	Input parameters:	np - a NODE ptr to the lhs of an ASSIGN
*
*	Output parameters:	none
*				(return value is YES or NO)
*
*	Globals referenced:	call_seen
*
*	Globals modified:	asg_count
*				asg_target
*
*	External calls:		locate()
*				walkf()
*
*******************************************************************************/
LOCAL flag comma_subsumable(np)	register NODE *np;
{
	HASHU *sp;

	if (ISTEMP(np))
		{
		/* I don't really think this case can happen. What could
		   allocate temps before this?
		*/
		np->tn.comma_ss = YES;
		return(YES);
		}
	if (call_seen)
		{
		/* Anything other than a local in the presence of a call will
		   be considered not a candidate.
		*/
		if (!locate(np, &sp))
			return(NO);
		if (sp->allo.attributes & (ISFARG|ISCOMMON|ISHIDDENFARG|ISFUNC|ISEXTERNAL|ISEQUIV))
			return(NO);
		}

	/* Ensure that the lhs is sufficiently simple to have no side
	   effects and to be a savings if used instead of the temporary.
	*/
	if (np->in.op == UNARY MUL)
		np = np->in.left;
	if (optype(np->in.op) != LTYPE)
		return(NO);

	asg_count = 0;
	asg_target = np;
	walkf(asg_tot, find_asgs);
	if (asg_count == 1) 
		{
		np->tn.comma_ss = YES;
		return(YES);
		}
	return(NO);
}



/******************************************************************************
*
*	replacecommons()
*
*	Description:		replacecommons() is a tree traversal routine
*				to convert a subtree in a dag into a subtree
*				in a tree by replacing CSEs.
*
*	Called by:		rplc()
*				treeem()
*
*	Input parameters:	np - a NODE pointer to a DAG subtree.
*
*	Output parameters:	np points to a modified subtree.
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void replacecommons(np)	register NODE *np;
{
	flag CSEtmpflag;   /* 
			      FLLrt01313: if left child becomes a temp, make
                              sure the node is no longer considered an array
			      form.  Doing so messes up register allocation.
			   */

	if (np)
		{
		switch(optype(np->in.op))
			{
			case UTYPE:
				np->in.left = rplc(np->in.left, NULL,
                                                   &CSEtmpflag);
				if (CSEtmpflag) {
				    np->in.arrayref = NO;
				    np->in.isarrayform = NO;
				}
				break;
			case BITYPE:
				np->in.left = rplc(np->in.left, NULL,
                                                   &CSEtmpflag);
				if (CSEtmpflag) {
				    np->in.arrayref = NO;
				    np->in.isarrayform = NO;
				}
				if (np->in.op == ASSIGN && np->in.right->ind.temptarget
					&& comma_subsumable(np->in.left)
					&& (heavytree(np->in.left) <= heavytree(np->in.right)) )
					np->in.right = rplc(np->in.right, np,
					               &CSEtmpflag);
				else
					np->in.right = rplc(np->in.right, NULL,
					               &CSEtmpflag);
				break;
			}
		}
}


/******************************************************************************
*
*	tagcommons()
*
*	Description:		fills the replaced[] array which avoids looping
*				when traversing a dag, and at the same time
*				marks the node as dagobsolete (i.e. seen).
*
*	Called by:		treeem()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	replaced
*
*	Globals modified:	replaced
*
*	External calls:		cerror()
*
*******************************************************************************/
LOCAL void tagcommons(np)	register NODE *np;
{
	if (np->ind.common_node)
		{
		if (replaced[np->ind.seq] == 0)
			replaced[np->ind.seq] = np;
		else
			{
			switch( optype(np->in.op) )
				{
				case BITYPE:
					np->in.right->ind.dagobsolete = YES;
					/* fall thru */
				case UTYPE:
					np->in.left->ind.dagobsolete = YES;
					break;
# ifdef DEBUGGING
				default:
					cerror("leaf node in tagcommons()");
# endif	DEBUGGING
				}
			np->ind.temptarget = YES;
			}
		}
}


/******************************************************************************
*
*	invalidlvalue()
*
*	Description:		return NO if p an lvalue, YES otherwise.
*
*	Called by:		fixnvlt()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
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
LOCAL flag invalidlvalue(np) register NODE *np;
{
	NODE *lnp;


again:
	switch( np->in.op ){

	case PAREN:

	case FLD:
		np = np->in.left;
		goto again;

	case UNARY MUL:
		lnp = np->in.left;
		/* fix the &(a=b) bug, given that a and b are structures */
		if( lnp->in.op == STASG ) return( YES );
		/* and the f().a bug, given that f returns a structure */
		if( lnp->in.op == UNARY STCALL || lnp->in.op == STCALL )
			return( YES );
		if (asgop(lnp->in.op) && !ISPTR(lnp->in.type))
			return( YES );
		/* fall thru */

	case NAME:
	case OREG:
		if( ISARY(np->in.type) || ISFTN(np->in.type) ) return(YES);
		/* fall thru */

	case COMOP:
	case REG:
		return(NO);

	default:
		return(YES);
		}
}


/******************************************************************************
*
*	returntemp()
*
*       Description:            returntemp() is a helper routine for
*				fixnvlt() below.  See that routine for a
*				description.
*
*	Called by:		fixnvlt()
*
*	Input parameters:	np - a NODE ptr to a subexpression that may
*				     be topped by a temporary of interest.
*
*	Output parameters:	none
*
*	Globals referenced:	asg_target
*				asg_csetemp
*
*	Globals modified:	none
*
*	External calls:		tcopy()
*
*******************************************************************************/
LOCAL void returntemp(np)	NODE *np;
{
	NODE *t;
	if (ISTEMP(np) && np->tn.lval == asg_csetemp->tn.lval)
		{
		t = tcopy(asg_target);
		*np = *t;
		t->in.op = FREE;
		}
}


/******************************************************************************
*
*	fixnvlt()
*
*       Description:            fixnvlt() fixes certain tree shapes
*				that, although syntactically ok, are
*				semantically meaningless and will croak
*				the code generator because it cannot
*				recognize them or rewrite them into
*				recognizable shapes.  It also replaces a
*				commonly seen inefficiency caused by an
*				injudicious use of temporaries.
*
*	Called by:		treeem()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	asg_csetemp
*				asg_target
*
*	External calls:		talloc()
*				walkf()
*
*******************************************************************************/
LOCAL fixnvlt(np)	register NODE *np;
{
	NODE *lp, *ncp;

	if (asgop(np->in.op) && invalidlvalue(np->in.left))
		{
		ncp = talloc();
		*ncp = *np;
		np->in.op = COMOP;
		np->in.left = np->in.left->in.left;
		np->in.right = ncp;
		lp = talloc();
		ncp->in.left->in.left = lp;
		*lp = *(np->in.left->in.left);
		}

	/* Find towering assignments that could be simplified by using user's
	   lhs in place of C1-defined temporaries.

	i.e.
			=
		       / \
		      /   \
		     a     ,
			  / \
			 /   \
			=     t
		       / \
		      /   \
		     t     foo

	becomes

			=
		       / \
		      /   \
		     a     foo

	where a is used elsewhere in the dag in place of t.  Only when a
	is a temp or a is defined only once within the block is this
	rewrite performed because otherwise a may be subsequently
	redefined by another ASSIGN in the tree. This condition is detected
	earlier in comma_subsumable() during the call to replacecommons().
	*/
	if ( np->in.op == ASSIGN && np->in.right->in.op == COMOP
		&& ISTEMP(np->in.right->in.right) 
		&& np->in.left->in.comma_ss )
		{
		/* asg_tot -- top of tree
		   asg_target -- user variable to be substituted for temp.
		   asg_csetemp -- temp to be eliminated
		*/
		lp = np->in.right->in.left;
		asg_target = np->in.left;
		asg_csetemp = lp->in.left;
		np->in.right->in.right->in.op = FREE;
		np->in.right->in.op = FREE;
		np->in.right = lp->in.right;
		walkf(asg_tot, returntemp);
		lp->in.left->in.op = FREE;
		lp->in.op = FREE;
		}
}


/******************************************************************************
*
*	treeem()
*
*       Description:            treeem() makes trees from dags for all
*				the blocks in the BBLOCK table.  It does
*				this by assigning temporary storage to
*				common subtree results.
*
*	Called by:		local_optimizations()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	replaced
*				seqtab
*
*	Globals modified:	replaced
*				seqtab
*
*	External calls:		fprintf()
*				fwalk()
*				eprint()
*
*******************************************************************************/

LOCAL void treeem(np)	NODE *np;
{
	register NODE **ip;

	if (!np->in.csefound) return;

	/* Use the seqtab space for a replaced table. First, initialize. */
	replaced = seqtab;
	for (ip = &replaced[topseq]; ip > replaced; )
		*ip-- = NULL;
	asg_tot = np;

	/* Replace common nodes with temp memory locations */
	FWALKC(np, tagcommons, DAGOBSOLETE);

	replacecommons(np);
# ifdef DEBUGGING
	if (ddebug > 2)
		{
		fprintf(debugp,"after replacecommons:\n");
		fwalk(np,eprint,0);
		}
# endif DEBUGGING

	/* asg_tot will be top-of-tree.  asg_target, if filled in fixnvlt(),
	   will be a user-defined variable which is a replacer for an
	   unnecessary temp, specified in asg_csetemp.
	*/
	fwalk(np, fixnvlt, 0);
}


/******************************************************************************
*
*	local_optimizations()
*
*	Description:		the basic driver for all local optimizations.
*
*	Called by:		main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	topblock
*				lastblock
*				ddebug
*				debugp
*
*	Globals modified:	blocknumber
*				maxccsz
*				common_child
*
*	External calls:		fprintf()
*				fwalk()
*				eprint()
*				reg_estimates()
*				redundant()
*				ckalloc()
*
*******************************************************************************/
void local_optimizations()
{
	register BBLOCK *bp;
	register NODE *np;

	reg_estimates();
	maxccsz = CCSZ;
	common_child = (CHILD *)ckalloc(maxccsz * sizeof(CHILD));

	for (bp = topblock; bp <= lastblock; bp++)
		if (np = bp->b.treep)
			{
			blocknumber = bp - topblock + 1;
			rewrite_comops_in_block(np);
#ifdef DEBUGGING
			if( ddebug )
				{
				fprintf(debugp, "after rewrite_comops_in_block(0x%x). Blocknumber: %d\n", np, blocknumber);
				fwalk( np, eprint, 0 );
				fprintf(debugp, "\n\n");
				}
#endif	DEBUGGING
			call_seen = NO;
			redundant(np);
#ifdef DEBUGGING
			if( ddebug )
				{
				fprintf(debugp, "after redundant(0x%x):\n", np);
				fwalk( np, eprint, 0 );
				fprintf(debugp, "\n\n");
				}
#endif	DEBUGGING
			treeem(np);
#ifdef DEBUGGING
			if( ddebug )
				{
				fprintf(debugp, "after treeem(0x%x):\n", np);
				fwalk( np, eprint, 0 );
				fprintf(debugp, "\n\n");
				}
#endif	DEBUGGING
			}
	FREEIT(common_child);
}
