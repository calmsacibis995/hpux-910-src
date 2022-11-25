/* file loops.c */
/* @(#) $Revision: 70.4 $ */
/* KLEENIX_ID @(#)loops.c       16.1.1.1 91/02/26 */

# include "c1.h"

/* NOTE: FLOAT_SR is defined if strength reduction is to be allowed on floating
   point aivs as functions of the scalar bivs. Unless more constant folding
   is available on floating point constant expressions, the result of such
   strength reduction is usually slower to execute than the original trees.
   If optim() is fully enhanced in the future this optimization may become
   worthwhile.

   FLOAT_SR is also used in misc.c.
*/

# define ISSIMPLE(p) ((optype((p)->in.op)==LTYPE)\
	&& (BTYPE((p)->in.type)==(p)->in.type.base)\
	&& !((p)->allo.flagfiller&CXM))
# define OK_SYM(sp) (((sp)->allo.attributes & SEENININPUT) && \
	!((sp)->allo.attributes & (ISEQUIV|ISFUNC|ISSTRUCT|ISCOMPLEX)) \
	&& ((sp)->allo.type.base != UNDEF) ) /* Not quite as in oglobal.c! */
			/* Fargs are automatically not SIMPLE because they are
			   OREGS that always have a UNARY MUL above them.
			*/
# define MIN(a,b)	(a<b? a : b)
# define OG_FLAGS	(~(IV_CHECKED|INVARIANT))
# define IV_FOUND	DAGOBSOLETE	/* reuse of this bit */
# define IV_FLAGS	(~(IV_CHECKED|IV_FOUND))

BBLOCK *top_preheader;	/* NULL unless dfo[0] is in a region that generates a
			   preheader
			*/
SET *looppreheaders;
SET *restricted_rblocks;	/* Contents same as currentregion except that
				   it does not contain elements for blocks
				   within inner subregions.
				*/
struct iv *ivtab;	/* induction variable table ... allocated in misc.c */
int maxivsz = IVINC;	/* max # of induction variables */
flag changing; 		/* communication between lisearch23() and detect_li().
			   Also used for global_constant_propagation.
			*/

LOCAL int topiv;
LOCAL SET *tempset;
LOCAL SET *loopdefs;		/* all definitions made within the region */
LOCAL SET *searchdefset;	/* used as temporary holding set during
				   searches by by lisearch1() and lisearch23()
				*/
LOCAL SET *exitdominators;	/* i in the set iff i dominates all exits of
				   the current region.
				*/
LOCAL BBLOCK *preheader;/* ptr to new preheader made during code_motion */
LOCAL BBLOCK *currentbp;/* ptr to BBLOCK under active consideration in
			   code_motion() and detect_indvars().
			*/
# ifdef COUNTING
LOCAL int loopcount;
# endif COUNTING;

LOCAL flag loopchanging; /* YES whenever loop is updated */
LOCAL flag region_dom_block; /* true iff current block is dom over all region
			       exits.
			*/
LOCAL flag anonymous_asgmt;	/* true iff a lhs for an assignment cannot be
				   positively determined. The usual cause is
				   that the assignment is thru a nonspecific
				   pointer.
				*/
LOCAL TTYPE tmatch();
LOCAL flag def_interferences();



/* Loop optimization routines */


/* The chief difficulty in performing code motion is that by moving
   statements from one block to another the descriptive sets such as
   deftab[], gen[], kill[], dom[], defs[], and region[] are invalidated
   unless care is taken to update them (time consuming).  One case in
   particular is that reaching information is required by loop induction
   variable reduction but the reaching information is obsolete once code
   motion is performed.  It would seem reasonable to perform loop
   induction variable reduction before code motion.
*/



/******************************************************************************
*
*	fargdef()
*
*	Description:		fargdef() returns YES iff there are definitions
*				of any formal argument within the loop. It also
*				initializes the useflag of all fargs.
*
*	Called by:		lisearch1()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	fargtab
*				lastfarg
*				symtab
*
*	Globals modified:	symtab
*
*	External calls:		setunion()
*				intersect()
*				isemptyset()
*
*******************************************************************************/
LOCAL flag fargdef()
{
	register unsigned *up;

	for (up = &fargtab[lastfarg]; up>=fargtab; up--)
		{
		if (symtab[*up]->an.defset)
			setunion(symtab[*up]->an.defset, searchdefset);
		symtab[*up]->an.useflag = YES;
		}
	intersect(searchdefset, loopdefs, searchdefset);
	return(!isemptyset(searchdefset));
	/* If it returns NO, then searchdefset must be empty */
}



/******************************************************************************
*
*	comdef
*
*	Description:		comdef() returns YES iff there are definitions
*				of any member of common (other than equivalenced
*				ones which are never considered for any
*				optimization) within the loop. It
*				also initializes the useflag of all common
*				members.
*
*	Called by:		lisearch1()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	comtab
*				lastcom
*				symtab
*
*	Globals modified:	symtab
*
*	External calls:		setunion()
*				intersect()
*				isemptyset()
*
*******************************************************************************/
LOCAL flag comdef()
{
	register unsigned *up;
	register CLINK *kp;

	for (up = &comtab[lastcom]; up >= comtab; up--)
		for (kp = symtab[*up]->cn.member; kp; kp = kp->next)
			{
			if (symtab[kp->val]->an.defset)
				setunion(symtab[kp->val]->an.defset,
					searchdefset);
			symtab[kp->val]->an.useflag = YES;
			}
	intersect(searchdefset, loopdefs, searchdefset);
	return(!isemptyset(searchdefset));
	/* If it returns NO, then searchdefset must be empty */
}


/******************************************************************************
*
*	lisearch1()
*
*	Description:		lisearch1() is a postfix tree traversal helper
*				routine called from detect_li(). It's basically
*				the implementation of step 1 in the Loop
*				invariant detection algorithm described in
*				A,S, & U, pages 638-639.  It searches a tree
*				node to see if it's invariant wrt the region
*				being currently analyzed.  A nonterminal node
*				is invariant if its children are invariant.
*				If a node is found to be invariant, its
*				invariant flag is set.  Otherwise it is cleared.
*				Note that the invariant flag is set to 1 here
*				only for LTYPE nodes.
*
*	Called by:		detect_li()
*
*	Input parameters:	np	NODE pointer
*
*	Output parameters:	none
*				(node pointed at by np is modified)
*
*	Globals referenced:	symtab
*				deftab
*
*	Globals modified:	symtab
*
*	External calls:		locate()
*				intersect()
*				isemptyset()
*				location()
*				clearset()
*
*******************************************************************************/
LOCAL void lisearch1(np)	register NODE *np;
{
	HASHU *sp;

	np->allo.flagfiller &= OG_FLAGS;	/* Initialization */

	switch( optype(np->in.op) )
		{
		case LTYPE:
			if ( locate(np, &sp) )
				{
				sp->an.useflag = YES;	/* initialization */
				if (sp->an.defset)
					intersect(sp->an.defset,loopdefs, searchdefset);
				else clearset(searchdefset);
				/* If the set's not empty there's a def within
				   the loop.
				*/
				if (sp->an.common)
					{
					if (fargdef())
						break;
					}
				else if (sp->an.farg)
					{
					if (comdef())
						break;
					if ( (!NO_PARM_OVERLAPS) && fargdef() )
						break;
					}
				if (isemptyset(searchdefset) && OK_SYM(sp))
					np->in.invariant = YES;
				}
			else if (!noinvariantop(np->in.op))
				np->in.invariant = ISSIMPLE(np) || nncon(np);
			break;
		case UTYPE:
			if (np->in.arrayref)
				{
				sp = location(np);
				sp->an.useflag = YES;	/* initialization */
				}
			break;
		case BITYPE:
			if (np->in.arrayref)
				{
				sp = location(np);
				sp->an.useflag = YES;	/* initialization */
				}
			if (asgop(np->in.op) && !locate(np->in.left, &sp))
					anonymous_asgmt = YES;
			break;
		}
}







/* sp->an.useflag will be set according to the following truth table:

	--
	|
	|	(A)	(Outside the region)
	|
	|
	--


    ------	(the region boundary)
    |
    |
    |	--
    |	|
    |	|	(B)	(within the region but outside C, the currentbp)
    |	|
    |	|
    |	--
    |
    |	--
    |	|
    |	|	(Cabove)
    |	|	X
    |	|	(Cbelow)
    |	|
    |	--
    |
    |
    -----

    At X, assuming reaching definitions from

		A	B	Cabove	Cbelow		useflag
case 1:		1+	0	0	0		YES
case 2:		1+	0	1	0		YES*
case 3:		0	1	0	0		YES
case 4:		1+	1	0	0		NO
case 5:		0	0	0	0		YES
case 6:		0	0	1	0		YES
case 7:		1+	0	0	1		NO
case 8:		0	0	0	1		NO
case 9: B+Cabove+Cbelow > 1				NO
case 10: # defs in region > 1				NO
	(case 9 is a subset of case 10)

* NOTE: (A) is computed up to the beginning of the current block. The def
  in C above is assumed to be nonambiguous, thereby hiding the def in (A).
*/

/******************************************************************************
*
*	checkltype()
*
*	Description:		checkltype() Checks an LTYPE node or any other
*				type that's an arrayref to see if it's
*				invariant. It operates only on those nodes that
*				can be found in the symbol table (i.e. not
*				anonymous). It's responsibilities are to set
*				the invariant flag for the NODE and the
*				sp->an.useflag for the pertinent symbol.
*
*	Called by:		check_ltypes()
*				check_ptr_targets()
*
*	Input parameters:	sp - a ptr into the symtab
*				np - a NODE ptr to the controlling subexpression
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*				deftab
*				currentbp
*				blockdefs
*
*	Globals modified:	symtab
*
*	External calls:		nextel()
*				intersect()
*				difference()
*				isemptyset()
*
*******************************************************************************/
LOCAL flag checkltype(sp, np)	register HASHU *sp; NODE *np;
{
	register int i;
	register uchar changecount;
	HASHU *lhs_sp;
	flag def_rhs_is_invariant;


	if (!sp->an.defset)
		{
		/* A kind of case 5 */
		/* sp->an.useflag = YES; ... already initialized this way */
		return ( (BTYPE(np->in.type)==np->in.type.base)
			&& !(np->allo.flagfiller&((EREF)|(LHSIREF))) 
			&& OK_SYM(sp) );
			/* Almost ISSIMPLE */
		}

	intersect(sp->an.defset, loopdefs, searchdefset);
	if (np->in.op == ASSIGN)
		delelement(np->in.defnumber, searchdefset, searchdefset);
	/* Searchdefset now has defs of sp within the region except this one. */

	def_rhs_is_invariant = NO;
	changecount = 0;
	i = -1;
	while ( (i = nextel(i, searchdefset)) >= 0 )
		{
		if (changecount++)
			{
			/* More than 1 def in the region */
			sp->an.useflag = NO; /* case 10 */
			return(NO);
			}
		def_rhs_is_invariant = asgop(deftab[i].np->in.op) &&
				deftab[i].np->in.right->in.invariant;
		lhs_sp = symtab[deftab[i].sindex];
		}
	if (changecount == 0)	/* cases 1 and 5 */
		{
		/* sp->an.useflag = YES; ... already initialized this way. */
		return(YES);
		}

	difference(loopdefs, currentbp->bb.l.in, tempset);
	intersect(tempset, sp->an.defset, tempset);
	/* Tempset now points to a set of reaching defines for sp
	   not within the current region. (A). Common members and fargs
	   are assumed to have initial defs that reach the region
	   (reconsider this assumption later).
	*/

	intersect(searchdefset, currentbp->bb.l.in, searchdefset);
	difference(blockdefs[currentbp->bb.node_position], searchdefset,
		searchdefset);
	/* Searchdefs now points to a set of reaching defines for
	   sp outside the current block but within the region. (B)
	*/
	if (!sp->an.common && !sp->an.farg && isemptyset(tempset))
		{
		if (isemptyset(searchdefset))	/* cases 6 and 8 */
			{
			sp->an.useflag &=
				(sp->an.blds==currentbp->bb.node_position)
				|| (sp == lhs_sp);
			return(sp->an.useflag && def_rhs_is_invariant
				&& changecount == 1);
			}
		/* Else case 3. */
		return(def_rhs_is_invariant && changecount == 1);
		}

	else
		{
		/* cases 2, 4 and 7 */
		if( sp != lhs_sp )
#pragma BBA_IGNORE
			cerror( "induction var elim: chcekltype:  sp != lhs_sp" );
		sp->an.useflag &= ( isemptyset(searchdefset) &&
				(sp->an.blds==currentbp->bb.node_position) );
		return(sp->an.useflag && def_rhs_is_invariant &&
			changecount == 1);
		}

	/* Invariant YES iff 1 def reached from outside this block or
	   sp is actually the lhs of the only definition or the
	   definition is in this block but already seen.
	*/
}


/******************************************************************************
*
*	check_ltypes()
*
*	Description:		check_ltypes() sets the invariant flag on a
*				node by calling checkltype(). For simple
*				auto vars it devolves to calling checkltype().
*				For COMMON members or fargs it must check
*				each alias.
*
*	Called by:		lisearch23()
*
*	Input parameters:	np - NODE ptr to either a simple var NODE
*				     or to a NODE that's in fact an array
*				     ref.
*
*	Output parameters:	none
*
*	Globals referenced:	fargtab
*				lastfarg
*				symtab
*				comtab
*				lastcom
*				changing
*
*	Globals modified:	none
*				(np->in.invariant may be changed)
*
*	External calls:		locate()
*
*******************************************************************************/
LOCAL void check_ltypes(np)	NODE *np;
{
	register unsigned *up;
	register CLINK *kp;
	register flag inv;
	HASHU *sp;

	if (!locate(np, &sp)) return;

	if (sp->an.farg)
		{
		inv = YES;
		for (up = &fargtab[lastfarg]; inv && (up >= fargtab); up--)
			inv = checkltype(symtab[*up], np);
		for (up = &comtab[lastcom]; inv && (up >= comtab); up--)
			for (kp = symtab[*up]->cn.member; inv && kp; kp = kp->next)
				inv = checkltype(symtab[kp->val], np);
		}
	else if (sp->an.common)
		{
		inv = checkltype(sp, np);
		for (up = &fargtab[lastfarg]; inv && (up >= fargtab); up--)
			inv = checkltype(symtab[*up], np);
		}
	else 
		inv = checkltype(sp, np);
	np->in.invariant = inv;
}


/******************************************************************************
*
*	set_blds()
*
*	Description:		set_blds() updates sp->blds for each symbol that
*				may be (ambiguously) defined by the
*				subexpression pointed to by np. blds is the
*				"block of last definition seen", which is used
*				by checkltype() above.
*
*	Called by:		lisearch23()
*
*	Input parameters:	np - a NODE ptr
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*				anonymous_asgmt
*				deftab
*
*	Globals modified:	symtab
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void set_blds(np)	NODE *np;
{
	register int d, dd, s;

	if (anonymous_asgmt)
		return;
	if ( d = np->in.defnumber )
		for (dd = deftab[d].children; dd >= 0; dd--, d++)
			{
			s = deftab[d].sindex;
			if (s >= 0)
				symtab[s]->an.blds =
					currentbp->bb.node_position;
			}
}


/******************************************************************************
*
*	structinvariant()
*
*	Description:		checks structured valued nodes to ensure that
*				each and every field is checked for invariance
*				(when appropriate) whenever the whole struct
*				is referenced.
*
*	Called by:		lisearch23()
*
*	Input parameters:	np - a NODE ptr to a subexpression tree.
*
*	Output parameters:	none
8				returns 1 iff entire struct is invariant.
*
*	Globals referenced:	symtab
*
*	Globals modified:	none
*
*	External calls:		cerror()
*
*******************************************************************************/
# if 0
LOCAL flag structinvariant(np)	register NODE *np;
{
	flag inv;
	CLINK *cp;

	switch(np->in.op)
		{
		case CM:
		case UCM:
		case RETURN:
		case CAST:
		case STASG:
		case ASSIGN:
		case INCR:
		case DECR:
			if (np->in.defnumber <= 0)
#pragma BBA_IGNORE
				cerror("structinvariant() cannot handle anonymous structref");
			cp = symtab[np->in.left->in.arrayrefno]->xn.member;
			inv = YES;
			while (inv && cp)
				{
				inv = checkltype(symtab[cp->val], np);
				cp = cp->next;
				}
			return(inv);
		default:
#pragma BBA_IGNORE
			cerror("unimplemented structref in structinvariant()");
			return(NO);
		}
}
# endif 0


/******************************************************************************
*
*	check_ptr_targets()
*
*	Description:		A UNARY MUL subexpression is invariant iff
*				each potential dereference is also invariant.
*				This routine checks all dereferences. In the
*				case that np is atop a STASG, take the 
*				STASG node's word for it.
*
*	NOTE: Our decision to not create deftab entries for structs implies
*		that FORTRAN formal args, which are usually pointers underneath
*		UNARY MULs. will never be marked as invariant except in those
*		very simple cases in which no I/O is performed anywhere in
*		the function.  Anytime a struct (which is a normal argument
*		of an I/O call in FORTRAN) is used as an actual argument it
*		is necessarily on the ptrtab.  Since there's no way to know 
*		if the definition is pertinent (i.e., within the loop) because
*		no defset exists for the symbol, it must be assumed that a
*		definition may occur within the loop and hence the UNARY
*		MUL node is not marked invariant.  The consequences are that
*		neither code motion nor strength reduction can occur on any
*		subexpression involving a UNARY MUL atop a formal argument.
*		See dts bug FSDdt03817 for a simple example.
*
*		In order to enable strength reduction or code motion on such
*		subexpressions we will have to have defsets on struct type
*		symbols (but not necessarily deftab entries).
*
*	Called by:		lisearch23()
*
*	Input parameters:	np - a NODE ptr to a UNARY MUL op.
*
*	Output parameters:	none
*
*	Globals referenced:	ptrtab
*				lastptr
*				symtab
*
*	Globals modified:	invariant flag on the np node
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void check_ptr_targets(np)	NODE *np;
{
	register unsigned *pp;
	flag inv;

	if(np->in.left->in.invariant && ptrtab && (np->in.left->in.op != STASG))
		{
		inv = YES;
		if (lastptr == 0 && symtab[*ptrtab]->an.type.base == UNDEF)
			/* The bogus ptrtab entry ... do not analyze it */
			;
		else
			for (pp = &ptrtab[lastptr]; (pp >= ptrtab) && inv; pp--)
				inv = checkltype(symtab[*pp], np);
		np->in.invariant = inv;
		}
	else
		np->in.invariant = np->in.left->in.invariant;
}


/******************************************************************************
*
*	lisearch23()
*
*	Description:		lisearch23() implements steps 2 and 3 of the
*				algorithm in A,S & U, page 639..
*
*	Called by:		detect_li()
*
*	Input parameters:	np - a NODE ptr
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
LOCAL void lisearch23(np)	register NODE *np;
{

	/* quit if already marked */
	if (np->in.invariant) return;

	switch( optype(np->in.op) )
		{
		case LTYPE:
			check_ltypes(np);
			break;

		case UTYPE:
			if ( callop(np->in.op) )
				/* leave np->in.invariant FALSE */
				set_blds(np);
			else if (np->in.op == UNARY MUL && !np->in.arrayref)
				check_ptr_targets(np);
			else if (np->in.arrayref && np->in.left->in.invariant)
				check_ltypes(np);
			else np->in.invariant = np->in.left->in.invariant;
			if (np->in.invariant && np->in.op == UCM
				&& np->in.left->in.structref)
				np->in.invariant = NO;
# if 0	/* Take the simpler, safer approach. */
				np->in.invariant = structinvariant(np);
# endif 0
			break;

		case BITYPE:
			if (np->in.op == SEMICOLONOP)
				np->in.invariant = np->in.right->in.invariant;
			else
				{
				if (asgop(np->in.op) || (callop(np->in.op) && !np->in.safe_call) )
					set_blds(np);
				if ( callop(np->in.op) && !np->in.safe_call )
					; /* leave np->in.invariant FALSE */
				else if (np->in.arrayref
					&& np->in.left->in.invariant
					&& np->in.right->in.invariant)
						check_ltypes(np);
				else if ((np->in.op == DIV || np->in.op == MOD)
					 && !nncon(np->in.right))
					np->in.invariant = NO;
				else
					np->in.invariant =
						np->in.left->in.invariant
						&& np->in.right->in.invariant
						&& !noinvariantop(np->in.op);
				if (np->in.invariant && asgop(np->in.op)
					&& np->in.left->in.structref)
					np->in.invariant = NO;
# if 0	/* Take the simpler, safe approach. */
					np->in.invariant = structinvariant(np);
# endif 0
				}
			break;
		}

	changing |= np->in.invariant;
}


/******************************************************************************
*
*	detect_li()
*
*	Description:		detect_li() implements the loop-invariant
*				detection algorithm in A,S & U, page 639.
*
*	Called by:		loop_optimization()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	dfo
*				anonymous_asgmt
*				maxdefs
*				tempset
*
*	Globals modified:	tempset
*				changing
*				currentbp
*
*	External calls:		walkf()
*				new_set()
*				free()	{from FREESET}
*
*******************************************************************************/
LOCAL void detect_li(cvector)	long *cvector;
{
	register i;
	register long *pelem;



	/* Step 1 of the algorithm on page 639. */
	pelem = cvector;
	while ((i = *pelem++) >= 0)
		{
		/* Individual basic blocks will be searched
		   more than once if they are within more
		   than 1 region. This is necessary because
		   definitions can be invariant to one loop
		   while not invariant to another.
		*/
		if (dfo[i]->b.treep)
			walkf(dfo[i]->bb.treep, lisearch1);
		if (anonymous_asgmt)
			/* No sense doing any more with this region */
			return;
		}
	/* Steps 2 and 3 of the algorithm */
	tempset = new_set(maxdefs);
	do
		{
		changing = NO;
		pelem = cvector;
		while ((i = *pelem++) >= 0)
			{
			currentbp = dfo[i];
			if (currentbp->bb.treep)
				walkf(currentbp->bb.treep, lisearch23);
			}
		}
	while (changing);
	FREESET(tempset);
}





/******************************************************************************
*
*	addgoto()
*
*	Description:		addgoto() adds an LGOTO expression to a basic
*				block. The tree for the basic block may be
*				vacuous.
*
*	Called by:		mknewpreheader()
*				addgoto()
*
*	Input parameters:	bp - a basic block ptr
*				labelval - an assembly code L label number
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		talloc()
*				block()
*
*******************************************************************************/
void addgoto(bp, labelval)	BBLOCK *bp; unsigned labelval;
{
	register NODE *np;

	np = talloc();
	np->in.op = LGOTO;
	np->bn.label = labelval;

	if (bp->bb.treep)
		bp->bb.treep = block(SEMICOLONOP, bp->bb.treep, np, INT);
	else
		bp->bb.treep = block(UNARY SEMICOLONOP, np, NULL, INT);
	bp->bb.treep->nn.stmtno = 0;	/* flag as added stmt to reg alloc */
}


/******************************************************************************
*
*	mknewpreheader()
*
*	Description:		mknewpreheader() makes a new loop preheader
*				suitable for taking loop invariant expressions
*				from within the main loop.
*
*	Called by:		addnode()
*				add_load_to_preheader()		
*
*	Input parameters:	cvector - a vector containing block #s of blocks
*					within the currentregion.
*
*	Output parameters:	none
*
*	Globals referenced:	maxnumblocks
*				innerdom
*				dfo
*				pseudolbl
*				maxdefs
*				preheader_index
*				region
*				looppreheaders
*				succset
*				predset
*				cgu
*				lastregion
*				lastblock
*				top_preheader
*
*	Globals modified:	innerdom
*				dfo
*				pseudolbl
*				looppreheaders
*				cgu
*				lastblock
*				top_preheader
*				numblocks
*
*	External calls:		new_set()
*				intersect()
*				nextel()
*				cerror()
*				free()	{FREESET}
*				mkblock()
*				adelement()
*				xin()
*				delelement()
*				setassign()
*				setcompare()
*				set_elem_vector()
*
*******************************************************************************/
BBLOCK *mknewpreheader(cvector)	long *cvector;
{
	register int i;
	register j;
	register BBLOCK *lpreheader;
	register BBLOCK *bpj;
	register SET *s;
	BBLOCK *oldbp;
	int k;
	NODE *cbp;
	CGU *cgup;
	int onumblocks;

	/* Assume in this routine that numblocks counts LBLOCKS, not BBLOCKS */
	{
	/* First, find out which block is the inner dominator of the
	   region (i.e.  the uppermost block within the region that
	   dominates all other region blocks).
	*/

	if (numblocks >= maxnumblocks)
#pragma BBA_IGNORE
		uerror("Too many new blocks made in mknewpreheader()");

	s = new_set(maxnumblocks);
	if (*cvector >= 0)	/* there are blocks in region ? */
		{
		setassign(dom[*cvector++], s);
		while (*cvector >= 0)
			intersect(dom[*cvector++], s, s);
		intersect(s, *currentregion, s);
		}

	if ( (i=nextel(-1, s)) < 0)
#pragma BBA_IGNORE
		cerror("Region has no dominator");

	FREESET(s);
	}

	/* If the last block "falls thru" to epilogue code, force an explicit
	   jump around the preheader by initiating a new block and a goto.
	*/
	onumblocks = numblocks;

	lpreheader = mkblock();
	oldbp = dfo[i];		/* oldbp points to the existing BBLOCK.  */
				/* must do after mkblock() in case expand
					block table */
	lpreheader->bb.l.val = pseudolbl;
	lpreheader->bb.l.bp = lpreheader;	/* self referential */
	lpreheader->bb.l.in = new_set(maxdefs);
	lpreheader->bb.l.out = new_set(maxdefs);
	lpreheader->bb.breakop = LGOTO;
	lpreheader->bb.lbp = oldbp;
	addgoto(lpreheader, oldbp->bb.l.val);

	/* dfo[] ordering for the new node isn't strictly correct
	   but it shouldn't change anything of significance because
	   regions have already been established.
	*/
	dfo[numblocks] = lpreheader;
	preheader_index[currentregion - region] = numblocks;
	adelement(numblocks, looppreheaders, looppreheaders);
	lpreheader->b.node_position = numblocks;
	currentbp = lpreheader;

	if (dfo[i]->bb.entries) /* The old block was an entry point */
	  {
	  int epn;

	  lpreheader->bb.entries = dfo[i]->bb.entries;
	  dfo[i]->bb.entries = NULL;
	  for (epn = 0; epn <= lastep; epn++)
	     if (ep[epn].b.bp == dfo[i])
		ep[epn].b.bp = lpreheader;
	  }

	/* The dom set for the new block is the same as for block i except
	   that it includes the new block itself (all blocks dom themselves).
	*/
	s = dom[numblocks];	/* New use for s */
	setassign(dom[i], s);
	adelement(numblocks, s, s);
	delelement(i, s, s);


	for (j = onumblocks-1; j >= 0; j--)
		{
		/* Patch up other dom blocks for the new preheader. */
		if (xin(s = dom[j], i))		/* New use for s */
			adelement(numblocks, s, s);

		/* Fix up branches and FREES to point instead to the new
		   BBLOCK (and label).
		*/

		/* If the branch is from within the region, don't change
		   but if it's a branch from outside, change it to point
		   instead to the preheader.
		*/
		if ( ! xin(*currentregion, j) )
			{
			/* predset && succset are assumed to have
			   already been created sufficiently large to
			   absorb a new active set.
			*/
			s = succset[j];	/* New use for s */
			if (xin(s, i))
				{
				delelement(i, s, s);
				adelement (numblocks, s, s);
				adelement (j, predset[numblocks],
					      predset[numblocks]);
				bpj = dfo[j];

				switch(bpj->bb.breakop)
				{
				case CBRANCH:
					/* Do nothing now except to modify
					the bblock pointer. When code
					is to be generated, check the "fall
					thru" lbp for equality
					in CBRANCH node. If not equal
					cause a goto to be generated.
					*/
					if (bpj->bb.lbp == oldbp)
						bpj->bb.lbp = lpreheader;
					if (bpj->bb.rbp == oldbp)
						{
						bpj->bb.rbp = lpreheader;
						cbp = bpj->bb.treep;
						if (cbp->in.op == SEMICOLONOP)
							cbp = cbp->in.right;
						else cbp = cbp->in.left;
						cbp = cbp->in.right;
						cbp->tn.lval = pseudolbl;
						}
					break;
				case FREE:
					/* change to a goto */
					if (bpj->bb.lbp == oldbp)
						{
						bpj->bb.breakop = LGOTO;
						bpj->bb.lbp = lpreheader;
						addgoto(bpj, pseudolbl);
						}
					else
#pragma BBA_IGNORE
						/*consistency check */
						cerror("FREE successor mismatch in mknewpre.()");
					break;
				case LGOTO:	/* everyday GOTO */
					if (bpj->bb.lbp == oldbp)
						{
						bpj->bb.lbp = lpreheader;
						cbp = bpj->bb.treep;
						cbp = (cbp->in.op==SEMICOLONOP)?
							cbp->in.right :
							cbp->in.left;
						cbp->bn.label = pseudolbl;
						}
					else
#pragma BBA_IGNORE
						/*consistency check */
						cerror("GOTO successor mismatch in mknewpre.()");
					break;
				case FCOMPGOTO:
				case GOTO:	/* ASSIGNED GOTO */
					cgup = bpj->cg.ll;
					for (k = bpj->cg.nlabs-1; k >= 0; k--)
						if (cgup[k].lp->bp == oldbp)
							{
							cgup[k].lp = &(lpreheader->b.l);
							cgup[k].val = pseudolbl;
							}
					break;
				case EXITBRANCH:
					break;
				}
				}
			/* blocks outside the loop which jumped to the
			 * inner dom must be removed from the inner dom's
			 * predset.
			 */
			s = predset[i];
			if (xin(s, j))
				{
				delelement(j, s, s);
				}
			}
		}


	/* If the loop dominator exists in some other region, too, then
	   so does the new preheader. Since we are dealing with strongly
	   connected regions (a subset of regions) the preheader does not
	   become part of its own region.
	*/
	{
	register SET **r;

	for (r = region; r <= lastregion; r++)
		{
		if (r == currentregion)
			continue;
		if (*r)
			switch(setcompare(*r, *currentregion))
				{
				case 0:	/* disjoint */
					/* fall thru */
				case 1:
					/* *r is wholely within *currentregion */
					break;
				case 2:
					/* *currentregion is wholely within *r */
					adelement(numblocks, *r, *r);
					break;
				case 3:
#pragma BBA_IGNORE
					/* overlapping regions ... impossible */
					cerror("ivalid region overlap in mknewpreheader()");
					break;
				}
		}

	}

	adelement(i, succset[numblocks], succset[numblocks]);
	adelement(numblocks, predset[i], predset[i]);

	numblocks++;
	lastblock++;
	pseudolbl++;
	
	/* if dfo[0] is in the region, then there's a preheader to the top
	   block.
	*/
	if ( xin(*currentregion, 0) )
		top_preheader = lpreheader;

#ifdef COUNTING
	_n_preheaders++;
#endif COUNTING

	return(lpreheader);
}

# ifdef COUNTING

LOCAL void sc_count(np)	register NODE *np;
/* Counts safe call code motions. */
{
top:
	switch (optype(np->in.op))
		{
		case BITYPE:
			sc_count(np->in.right);
			if (np->in.op == FMONADIC || np->in.op == CALL)
				{
				if (np->in.invariant)
					++_n_cm_safecalls;
				break;
				}
			/* fall thru */
		case UTYPE:
			np = np->in.left;
			goto top;
		}
}

# endif COUNTING

/******************************************************************************
*
*	addnode()
*
*	Description:		addnode() takes a statement in subtree form and
*				attaches it to the preheader.
*
*	Called by:		test_assignments()
*				deletenode()
*				crunch_invariants
*				add_sinit()
*
*	Input parameters:	np - a NODE ptr to a subexpression that's to
*				     be added to the preheader
*
*	Output parameters:	none
*
*	Globals referenced:	preheader
*				debugp
*				deftab
*				numblocks
*				loopchanging
*
*	Globals modified:	deftab
*				loopchanging
*				nodecount
*
*	External calls:		block()
*				fprintf()
*				fwalk()
*				delelement()
*				cerror()
*				update_flow_of_control()
*				optim()
*
*******************************************************************************/
LOCAL void addnode(np)	register NODE *np;
{
	register NODE *lnp;
	NODE *temp;
	ushort d, bno;
	int dd, phi;

	/* First, throw away excess baggage, if any. */
	if (np->in.op == UNARY SEMICOLONOP)
		{
		np->in.op = FREE;
		np = np->in.left;
		}
	else if (np->in.op == SEMICOLONOP)	
		{
		np->in.op = FREE;
		np = np->in.right;
		}

	lnp = preheader->b.treep;
	if (lnp->in.op == UNARY SEMICOLONOP)
		{
		temp = lnp->in.left;
		lnp->in.left = np;
		}
	else
		{
		temp = lnp->in.right;
		lnp->in.right = np;
		}

	np->in.index = nodecount++;
	lnp->in.index = nodecount++;
	preheader->b.treep = block(SEMICOLONOP,  optim(lnp), temp, INT);
	preheader->b.treep->in.index = nodecount++;

# ifdef DEBUGGING
	if (gdebug > 1)
		{
		fprintf(debugp, "addnode() called. Preheader tree:\n");
		fwalk(preheader->b.treep, eprint, 0);
		}
# endif DEBUGGING

# ifdef COUNTING
	sc_count(preheader->b.treep);
# endif COUNTING

	if (ASSIGNMENT(np->in.op))
		{
		d = np->in.defnumber; 	/* only simple lhs possible */
		delelement(d, loopdefs, loopdefs);
		bno = deftab[d].bn;
		phi = preheader->b.node_position;/* block number of preheader */
		if (bno != phi)
			for (dd = deftab[d].children; dd >= 0; dd--, d++)
				{
				deftab[d].bn = phi;
				}
		}
	else
#pragma BBA_IGNORE
		cerror("Strange loop invariant found in addnode()");

	/* Keeping in[] and out[] sets up to date is just too hard. Better
	   to recalculate them.
	*/
	update_flow_of_control();
	loopchanging = YES;
}


/******************************************************************************
*
*	cm_candidate()
*
*	Description:		cm_candidate() returns YES iff
*					1) the object is invariant, and
*					2) the three conditions specified in
*					A, S & U on page 639 are satisfied.
*
*	Called by:		deletenode()
*
*	Input parameters:	np - a NODE ptr to a subexpression candidate
*
*	Output parameters:	none
*
*	Globals referenced:	region_dom_block
*				deftab
*				symtab
*
*	Globals modified:	none
*
*	External calls:		def_interferences()
*
*******************************************************************************/
LOCAL flag cm_candidate(np)	register NODE *np;
{
	int d, dd;
	flag use;
	NODE *rp;

	if (!np->in.invariant)	return (NO);

	/* np points to a node immediately below the UNARY SEMICOLONOP or the
	   SEMICOLONOP
	*/
	if (np->in.op == ASSIGN)
		{
		d = np->in.defnumber;
		rp = np->in.right;
		np = np->in.left;
		}
	else return(NO);

	if (def_interferences(rp))
		return(NO);

	/* Now np points to the lhs */

	/* If the node np is is a temporary, there cannot be any use of it
	   after the loop. It is also guaranteed that there is only 1
	   definition. Therefore it can be moved into the preheader.
	*/
	if ( ISTEMP(np) )
		return (YES);

	/* If it's not a temporary, then it must satisfy all the conditions
	   stated in A,S&U, page 639:
		1) "The block containing 's: x=y op z;' dominates all exit
		    nodes of the loop..."
		2) "There is no other statement in the loop that assigns to x."
		3) "No use of x in the loop is reached by any definition of x
		    other than s."
		In addition, it should be cost-effective. It will be if it's
		an entire assignment statement.
	*/
	if ( region_dom_block && d )
		{
		use = YES;
		for (dd = deftab[d].children; use && dd >= 0; dd--, d++)
			use = symtab[deftab[d].sindex]->an.useflag;
		return(use); 
		}

	/* If the assignment is never again used in the function, also return
	   YES. - not yet implemented because it requires live variable
	   analysis.
	*/

	return (NO);
}


/******************************************************************************
*
*	ldfi()
*
*	Description:		helper routine for def_interferences(). Checks
*				a single node.
*
*	Called by:		def_interferences()
*
*	Input parameters:	np	a NODE ptr to the candidate node.
*
*	Output parameters:	none
*				(returns YES iff there is a definition of the
*				variable in question within the loop - hence
*				an interference)
*
*	Globals referenced:	loopdefs
*				searchdefset
*
*	Globals modified:	searchdefset
*
*	External calls:		locate()
*				intersect()
*				isemptyset()
*				nextel()
*
*******************************************************************************/
LOCAL flag ldfi(np)	NODE *np;
{
	HASHU *sp;
	register int i;

	if (!locate(np, &sp))
		return(NO);	/* Constants, etc. */
	else if (sp->an.useflag)
		{
		if (sp->an.defset)
			intersect(loopdefs, sp->an.defset, searchdefset);
		else
			return(NO);
		if (isemptyset(searchdefset))
			return(NO);
		else if (np->in.op == ASSIGN)
			{
			/* At most 1 def within the region because of defset */
			i = -1;
			while ( (i = nextel(i, searchdefset)) >= 0)
				return (i != np->in.defnumber);
			}
		}

	return(YES);
}


/******************************************************************************
*
*	def_interferences()
*
*	Description:		Checks for the presence of a reaching definition
*				within the current region that would interfere
*				with the use of a variable within a
*				subexpression to be moved out of the loop to a
*				preheader.
*
*	Called by:		minor_cm_candidate()
*				cm_candidate()
*				test_cmnode()
*
*	Input parameters:	np	a NODE ptr to the candidate
*					subexpression.
*
*	Output parameters:	none
*				(returns YES iff there is an interfering def
*				within the subtree)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		def_interferences()	{RECURSIVE}
*				ldfi()
*
*******************************************************************************/
LOCAL flag def_interferences(np)	register NODE *np;
{
top:
	switch (optype(np->in.op))
		{
		case BITYPE:
			if (np->in.arrayref && ldfi(np))
				{
# ifdef COUNTING
				_n_cm_dis++;
# endif COUNTING
				return(YES);
				}
			if (def_interferences(np->in.right))
				return(YES);
			/* fall thru */
		case UTYPE:
			if (np->in.arrayref && ldfi(np))
				{
# ifdef COUNTING
				_n_cm_dis++;
# endif COUNTING
				return(YES);
				}
			np = np->in.left;
			goto top;
			
		case LTYPE:
# ifdef COUNTING
			if (ldfi(np))
				{
				_n_cm_dis++;
				return(YES);
				}
			else return(NO);
# endif COUNTING
			return(ldfi(np));
		}
}



/******************************************************************************
*
*	minor_cm_candidate()
*
*	Description:		minor_cm_candidate() tests a subexpression to
*				see if it's worthwhile to use in code motion.
*				(Some sort of code savings should be realized).
*				If it is worthwhile a temp will eventually be
*				created for the subexpression.
*
*	Called by:		test_minors()
*				crunch_invariants()
*				test_assignments()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	region_dom_block
*
*	Globals modified:	none
*
*	External calls:		hardconv()
*				def_interferences()
*				worthit()
*
*******************************************************************************/
LOCAL flag minor_cm_candidate(np)	register NODE *np;
{

top:
	switch( np->in.op )
		{
		case CALL:
			if (np->in.safe_call)
				/* Just like FMONADIC */
				break;
			/* fall thru */
		/*case LTYPE:*/
		case NAME:
		case REG:
		case OREG:
		case FOREG:
		case ICON:
		case FCON:
		case CCODES:
		case NOCODEOP:
		case FENTRY:
		case LGOTO:
		case FCOMPGOTO:
		case STRING:
		case EXITBRANCH:
		case FREE:
			/* If the type is LTYPE there's no savings to be
			   seen.  This is a crude cost function.
			*/

		/*callops*/
		case STCALL:
		case UNARY CALL:
		case UNARY STCALL:

		/*nodelayops*/
		case QUEST:
		case COLON:
		case ANDAND:
		case OROR:
		case CM:
		case UCM:
		case COMOP:
			return (NO);

		case SCONV:
			if (!np->in.invariant)
				return(NO);
			if (hardconv(np))
				break;
			else
				np = np->in.left;
			goto top;

		case ASSIGN:
			/* Avoid situations like
				temp1 = temp2 = ...
			   moved into a preheader.
			*/
			if (ISTEMP(np->in.left))
				return(NO);
			break;

		/*divops*/
		case DIV:
		case ASG DIV:
		case DIVP2:
		case ASG DIVP2:
		case MOD:
		case ASG MOD:
		case UNARY MUL:	/* Dereferencing an invalid address may cause a
					segmentation violation. FSDdt04273.
				*/
			if (!region_dom_block)
				return(NO);
			break;

		default:
			break;
		}
	/* np is a candidate iff invariant and no operand has a reaching
	   definition from within the loop.
	*/
	return ( np->in.invariant && !def_interferences(np) && worthit(np) );
}


/******************************************************************************
*
*	mktempassign()
*
*	Description:		mktempassign() makes an appropriately shaped
*				assignment subtree to a new temporary. This
*				subtree is suitable for code motion up into a
*				region preheader.
*
*	Called by:		test_minors()
*				crunch_invariants()
*
*	Input parameters:	np - a NODE ptr pointing at a subexpression to
*				     be replaced by a temp.
*
*	Output parameters:	none
*
*	Globals referenced: 	numblocks
*				deftab
*				symtab
*				lastdef
*				blockdefs
*				preheader
*
*	Globals modified:	blockcount
*				blockdefs
*
*	External calls:		block()
*				mktemp()
*				init_defs()
*				adelement()
*
*******************************************************************************/
LOCAL NODE *mktempassign(np)	register NODE *np;
{
	HASHU *sp;

	np->in.temptarget = YES;
	np = block(ASSIGN, mktemp(0, np->in.type), np, 0, np->in.type);

	blockcount = preheader->bb.node_position;

	/* Reset deftab[] and lastdef */
	init_defs(np);

	/* Update pertinent sets for next iteration of lisearch1() */
	sp = symtab[deftab[np->in.defnumber].sindex];
# ifdef DEBUGGING
	if (!sp->an.defset)
#pragma BBA_IGNORE
		cerror("Null defset in mktempassign()");
# endif DEBUGGING
	adelement(lastdef, sp->an.defset, sp->an.defset);

	return ( np );
}



/******************************************************************************
*
*	test_minors()
*
*	Description:		Tests a subexpression for invariance.
*
*	Called by:		test_assignments()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		minor_cm_candidate()
*				mktempassign()
*
*******************************************************************************/
LOCAL NODE *test_minors(np)	NODE *np;
{
	return(minor_cm_candidate(np)? mktempassign(np) : NULL);
}


/******************************************************************************
*
*	test_assignments()
*
*	Description:		Even if the whole statement isn't a true
*				candidate for code motion (i.e.  it doesn't
*				satisfy all the requirements), it may be
*				invariant and there may be some subpart of it
*				that can be moved out of the loop.  For example,
*				if the statement is invariant but it occurs in
*				a block within the region that does not
*				dominate all exits of the region it is not
*				correct to move it into a preheader.  However,
*				if the expression has nontrivial subexpressions,
*				it is possible to assign temporaries to these
*				subexpressions and then move the definitions of
*				these temporaries out of the loop.  The original
*				assignment remains within the loop but it's much
* 				simpler (and quicker, hopefully).
*
*	Called by:		deletenode()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	n_code_motions
*
*	External calls:		cptemp()
*				tcopy()
*				tfree()
*				test_minors()
*				minor_cm_candidate()
*				addnode()
*				test_minors()
*
*******************************************************************************/
LOCAL flag test_assignments(np)		register NODE *np;
{
	NODE *childp;
	flag success = NO;


	/* First check the subscripts of the lhs, if any. */
	if (np->in.left->in.arrayref && optype(np->in.left->in.op) != LTYPE
		&& (childp = test_minors(np->in.left->in.left)))
		{
		/* Looking for a nontrivial lhs describing an array ref */
		np->in.left->in.left = cptemp(childp->in.left);
		addnode(childp);

#	ifdef COUNTING
		_n_code_motions++;
# 	endif COUNTING
		success = YES;
		}
	/* Now check the rhs */
	if (np->in.right->in.temptarget)
		{
		if (minor_cm_candidate(np->in.right))
			{
			/* Situation:  temp = subexpr. Move wholesale. */
			addnode(tcopy(np));
#	ifdef COUNTING
			_n_code_motions++;
# 	endif COUNTING
			tfree(np);
			np->in.op = NOCODEOP; 	/* There's no handle on the
						   parent.
						*/
			success = YES;
			}
		}
	else if (childp = test_minors(np->in.right))
		{
		np->in.right = cptemp(childp->in.left);
		addnode(childp);
#	ifdef COUNTING
		_n_code_motions++;
# 	endif COUNTING
		success = YES;
		}
	return(success);
}


/******************************************************************************
*
*	test_cmnode()
*
*	Description:		Subexpressions topped by a COMOP with an
*				assignment on the lhs and a temp on the rhs
*				are frequently generated by local optimizations.
*				If the rhs of the assignment is otherwise
*				invariant we can move the assignment out of
*				the loop and leave only the temp in place of
*				the COMOP subtree. This function detects an
*				appropriate occurrence of this situation.
*
*	Called by:		crunch_invariants()
*
*	Input parameters:	np - a NODE ptr to a subexpression
*
*	Output parameters:	none
*				(return value is TRUE iff it's a replaceable
*				assignment)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		same()
*				def_interferences()
*
*******************************************************************************/
LOCAL flag test_cmnode(np)	NODE *np;
{
	return( np->in.op == COMOP && np->in.invariant
		&& np->in.left->in.op == ASSIGN
		&& same(np->in.right, np->in.left->in.left, NO, NO)
		&& !def_interferences(np->in.left->in.right) );
}


/******************************************************************************
*
*	deletenode()
*
*	Description:		deletenode() checks a statement subtree to see
*				if it's invariant and therefore a candidate for
*				code motion. If it is, it deletes it from its
*				parent tree and calls addnode() to add it to a
*				(possibly new) preheader block. It returns YES
*				iff it moved something to the preheader.
*
*	Called by:		crunch_invariants()
*
*	Input parameters:	np - a NODE ptr to either a SEMICOLONOP or a
*				     UNARY SEMICOLONOP.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		block()
*
*******************************************************************************/
LOCAL flag deletenode(np)	register NODE *np;
{
	register NODE *lnp;

	lnp = (np->in.op==SEMICOLONOP)? np->in.right : np->in.left;
	if (cm_candidate(lnp))
		{
		np->in.invariant = NO;
		if (np->in.op == SEMICOLONOP)
			np->in.right = block(NOCODEOP, 0, 0, INT);
		else
			np->in.left = block(NOCODEOP, 0, 0, INT);
		addnode(lnp);
#	ifdef COUNTING
		_n_code_motions++;
# 	endif COUNTING
		return(YES);
		}
	else if (np->in.invariant)
		return(lnp->in.op == ASSIGN && test_assignments(lnp));
	else
		return (NO);
}


/******************************************************************************
*
*	crunch_invariants()
*
*	Description:		crunch_invariants() analyzes and manipulates a
*				node of the tree for code motion.
*
*	Called by:		code_motion()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	lastdef
*				maxdefs
*
*	Globals modified:	_n_cm_deletenodes
*				_n_cm_code_motions
*				_n_cm_cm_minors
*				_n_cm_cmnodes
*
*	External calls:		cptemp()
*
*******************************************************************************/
LOCAL void crunch_invariants(np)	register NODE *np;
{
	NODE *dp;

	if (np == NULL) return;
	if (lastdef >= maxdefs - 2)
		return;
	if (np->in.op == SEMICOLONOP || np->in.op == UNARY SEMICOLONOP)
		{
		if ( deletenode(np) ) 
# ifdef COUNTING
			_n_cm_deletenodes++
# endif COUNTING
			;
		}
	else if ( !np->in.invariant )
		{
		/* Even in an expression that's not invariant there may
		   be invariant subexpressions that, if sufficiently
		   complex, may profitably be moved out of the region
		   by means of first assigning the subexpression to a
		   temporary, then moving the temporary into the
		   preheader.
		*/
		switch ( optype(np->in.op) )
		{
		case UTYPE:
			if (minor_cm_candidate(np->in.left))
				{
				dp = mktempassign(np->in.left);
				np->in.left = cptemp(dp->in.left);
				np->in.left->in.arrayrefno =
					dp->in.right->in.arrayrefno;
				np->in.left->allo.flagfiller =
					dp->in.right->allo.flagfiller&(~ISPTRINDIR);
				if (dp->in.right->in.op == UNARY MUL)
					np->in.left->allo.flagfiller &=
						~(AREF|ARRAYELEM|ARRAYFORM);
				np->in.left->in.invariant = YES;
				addnode (dp);
#	ifdef COUNTING
				_n_code_motions++;
				_n_cm_minors++;
# 	endif COUNTING
				}
			else if (test_cmnode(np->in.left))
				{
				/* CM op made by assigning to a temp */
				dp = np->in.left;
				np->in.left = dp->in.right;
				np->in.left->in.invariant = YES;
				addnode(dp->in.left);
#	ifdef COUNTING
				_n_code_motions++;
				_n_cm_cmnodes++;
# 	endif COUNTING
				dp->in.op = FREE;
				}
			break;
		case BITYPE:
			if ( noinvariantop(np->in.op) ) /* maybe others too? */
				break;
			/* Since np->in.invariant is false, only 1 side
			   can be invariant.
			*/
			if (minor_cm_candidate(np->in.left))
				{
				dp = mktempassign(np->in.left);
				np->in.left = cptemp(dp->in.left);
				np->in.left->in.arrayrefno =
					dp->in.right->in.arrayrefno;
				np->in.left->allo.flagfiller =
					dp->in.right->allo.flagfiller&(~ISPTRINDIR);
				if (dp->in.right->in.op == UNARY MUL)
					np->in.left->allo.flagfiller &=
						~(AREF|ARRAYELEM|ARRAYFORM);
				np->in.left->in.invariant = YES;
				addnode (dp);
#	ifdef COUNTING
				_n_code_motions++;
				_n_cm_minors++;
# 	endif COUNTING
				}
			else if (test_cmnode(np->in.left))
				{
				/* CM op made by assigning to a temp */
				dp = np->in.left;
				np->in.left = dp->in.right;
				np->in.left->in.invariant = YES;
				addnode(dp->in.left);
#	ifdef COUNTING
				_n_code_motions++;
				_n_cm_cmnodes++;
# 	endif COUNTING
				dp->in.op = FREE;
				}
			else if (minor_cm_candidate(np->in.right))
				{
				dp = mktempassign(np->in.right);
				np->in.right = cptemp(dp->in.left);
				np->in.right->in.arrayrefno =
					dp->in.right->in.arrayrefno;
				np->in.right->allo.flagfiller =
					dp->in.right->allo.flagfiller&(~ISPTRINDIR);
				if (dp->in.right->in.op == UNARY MUL)
					np->in.right->allo.flagfiller &=
						~(AREF|ARRAYELEM|ARRAYFORM);
				np->in.right->in.invariant = YES;
				addnode (dp);
#	ifdef COUNTING
				_n_code_motions++;
				_n_cm_minors++;
# 	endif COUNTING
				}
			else if (test_cmnode(np->in.right))
				{
				/* CM op made by assigning to a temp */
				dp = np->in.right;
				np->in.right = dp->in.right;
				np->in.right->in.invariant = YES;
				addnode(dp->in.left);
#	ifdef COUNTING
				_n_code_motions++;
				_n_cm_cmnodes++;
# 	endif COUNTING
				dp->in.op = FREE;
				}
			break;
		}
		}
}

/******************************************************************************
*
*	replace_temp()
*
*	Description:		A recursive routine to replace all offsets of
*				one temporary by the offset of another.
*
*	Called by:		combine_cmtemps()
*
*	Input parameters:	np - a NODE ptr to the top of a subtree.
*				replacee - the NODE to be replaced.
*				replacer - the ASSIGN NODE atop the replacing
*					NODE to be copied.
*
*	Output parameters:	none
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		replace_temp()	{RECURSIVE}
*
*******************************************************************************/
LOCAL void replace_temp(np, replacee, replacer)
	register NODE *np; NODE *replacee, *replacer;
{
top:
	switch (optype(np->in.op))
		{
		case BITYPE:
			if (np == replacer)
				/* Don't want to throw away the base assignment
				   itself!
				*/
				break;
			replace_temp(np->in.right, replacee, replacer);
			/* fall thru */
		case UTYPE:
			np = np->in.left;
			goto top;
		case LTYPE:
			if (np->tn.op==OREG && np->tn.lval == replacee->tn.lval)
				*np = *(replacer->in.left);
		}
}

/******************************************************************************
*
*	check_cmtemps()
*
*	Description:		returns TRUE iff only 1 definition of a
*				temporary occurs in the subprogram.
*
*	Called by:		combine_cmtemps()
*
*	Input parameters:	tset - a SET ptr to a working set.
*				np - a NODE ptr to a temporary assignment.
*
*	Output parameters:	none
*				returns TRUE as stated above.
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		location()
*				setassign()
*				delelement()
*				isemptyset()
*
*******************************************************************************/
LOCAL flag check_cmtemps(tset, np)	register SET *tset; NODE *np;
{
	HASHU *sp;

	sp = location(np->in.left);
	setassign(sp->an.defset, tset);
	delelement(np->in.defnumber, tset, tset);
	return(isemptyset(tset));
}

/******************************************************************************
*
*	combine_cmtemps()
*
*	Description:		Traverses the preheader looking for duplicate
*				but distinct temporaries constructed during
*				code motion. Drives replace_temp() to combine
*				these temps.
*
*	Called by:		code_motion()
*
*	Input parameters:	lastnp - a NODE ptr to the preheader's tree
*					before code motion began adding temp
*					assignments. It now points to the first
*					of any preexisting temp assignments.
*				cvector - a vector containing block #s of blocks
*					within currentregion.
*
*	Output parameters:	none
*				(the preheader block tree and the loop trees
*				may be revised)
*
*	Globals referenced:	udisable
*				preheader
*				gdebug
*				debugp
*
*	Globals modified:	none
*
*	External calls:		same()
*				tfree()
*				tcopy()
*				replace_temp()
*				new_set()
*				check_cmtemps()
*				free()	{FREESET}
*
*******************************************************************************/
LOCAL void combine_cmtemps(lastnp, cvector) NODE *lastnp; long *cvector;
{
	register NODE *lnp;
	register NODE *rnp;
	register NODE *rnp2;
	register int i;
	NODE *np;
	NODE *currentnp;
	NODE *useless;
	SET *tset;
	long *pelem;

	if (udisable || !(np=preheader->b.treep))
		return;

# ifdef DEBUGGING
	useless = NULL;
# endif DEBUGGING

	tset = new_set(maxdefs);
	for (np = np->in.left; (np != lastnp) && np->in.op == SEMICOLONOP;
		np = np->in.left)
		{
		rnp = np->in.right;
		if ( (rnp->in.op != ASSIGN) || !check_cmtemps(tset, rnp) )
			continue;

		for (lnp = np->in.left; lnp != lastnp; lnp = lnp->in.left)
			{
			if (lnp->in.op != SEMICOLONOP && lnp->in.op != UNARY SEMICOLONOP)
				break;

			rnp2 = (lnp->in.op == SEMICOLONOP)? lnp->in.right :
				lnp->in.left;

			if (rnp2->in.op != ASSIGN)
				continue;

			if (same(rnp->in.right, rnp2->in.right, NO, NO))
				{
				if ( !check_cmtemps(tset, rnp2) )
				    continue;

# ifdef COUNTING
				_n_cm_combines++;
# endif COUNTING
				/* Combine the two temps */
				useless = tcopy(rnp->in.left);
				if (ISTEMP(useless))
					{
					/* The temp has no uses outside the loop
					   other than possibly in the preheader.
					*/
					tfree(rnp);
					rnp->in.op = NOCODEOP;
					}
				else
					{
					/* It's not a temp. Assume that other
					   uses for rnp's lhs exist outside the
					   loop (where the invariancy may not
					   hold). Useless is something
					   of a misnomer in this case (it's
					   really only relatively useless).
					*/
					tfree(rnp->in.right);
					rnp->in.right = tcopy(rnp2->in.left);
					}
				pelem = cvector;
				while ((i = *pelem++) >= 0)
					if (currentnp=dfo[i]->bb.treep)
						replace_temp(currentnp,
							useless, rnp2);
				/* The preheader itself may use the extraneous
				   lhs.
				*/
				replace_temp(preheader->b.treep, useless, rnp2);
				tfree(useless);
				break;
				}
			}
		}
	FREESET(tset);

# ifdef DEBUGGING
	if ((gdebug > 1) && useless)
		{
		fprintf(debugp, "after combine_cmtemps():\n");
		dumpblockpool(1, 1);
		}
# endif DEBUGGING
}


/******************************************************************************
*
*	code_motion()
*
*	Description:		code_motion() is the basic driver for code
*				motion optimization. For each block in the
*				region it does a postfix traversal of the
*				associated tree to detect and move loop
*				invariant statements.
*
*	Called by:		loop_optimization()
*
*	Input parameters:	rvector - a vector of block #s in
*						restricted_rblocks
*
*	Output parameters:	none
*
*	Globals referenced:	maxnumblocks
*				numblocks
*				dfo
*				region_dom_block
*				exitdominators
*				currentbp
*
*	Globals modified:	currentbp
*				region_dom_block
*
*	External calls:		xin()
*				walkf()
*				crunch_invariants()
*				combine_cmtemps()
*
*******************************************************************************/
LOCAL void code_motion(cvector, rvector) long *cvector, *rvector;
{
	register i;
	NODE *cleanup_np;

	/* The region traversal must go in this order because it follows the
	   dfo[]. Otherwise we may add nodes to the preheader before they're
	   defined.
	*/

	/* Remove blocks from all inner subregions from consideration. Only
	   headers (which aren't part of the subregions they control) and
	   nonintersecting blocks should be considered. Preheader blocks have
	   already been added to the outer region by mknewpreheader().
	*/

	cleanup_np = (preheader->bb.treep->in.op == UNARY SEMICOLONOP)?
			NULL : preheader->bb.treep;

	while ((i = *rvector++) >= 0)
		if ( dfo[i]->bb.treep )
			{
			currentbp = dfo[i];
			region_dom_block = xin(exitdominators, i);
			walkf(currentbp->bb.treep, crunch_invariants);
			}
	combine_cmtemps(cleanup_np, cvector);
}


/******************************************************************************
*
*	free_ivtab()
*
*	Description:		Each ivtab entry has associated expression trees
*				headed by mc and ac. These must be freed once
*				iv processing is complete for each region. It
*				also resets the ivtab to default size.
*
*	Called by:		loop_optimization()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	ivtab
*				topiv
*				maxivsz
*
*	Globals modified:	ivtab
*				maxivsz
*
*	External calls:		tfree()
*
*******************************************************************************/
LOCAL void free_ivtab()
{
	register struct iv *ivp;

	for (ivp = &ivtab[topiv]; ivp >= ivtab; ivp--)
		{
		tfree(ivp->mc);
		tfree(ivp->ac);
		}

	/* Reset ivtab to default size */
	if (maxivsz != IVINC)
		{
		maxivsz = IVINC;
		ivtab = (struct iv *) ckrealloc(ivtab,
					maxivsz * sizeof(struct iv) );
		}
}


/******************************************************************************
*
*	loop_optimization()
*
*	Description:		loop_optimization() is the basic driver for all
*				optimizations directed specifically at loops.
*				The precise definition of a "loop" to c1 is
*				that of a "region" described in Tremblay and
*				Sorenson, and A, S, & U.  Optimizations are
*				performed only if the flowgraph is found to be
* 				reducible via t1_t2 analysis.
*
*	Called by:		global_optimize()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	loopdefs
*				searchdefset
*				exitdominators
*				maxnumblocks
*				maxdefs
*				lastdef
*				loopchanging
*				currentregion
*				region
*				lastregion
*				preheader_index
*				preheader
*				numblocks
*				blockdefs
*				succset
*				dom
*				anonymous_asgmt
*				gdebug
*				gdebugp
*				lastblock
*				blockpool
*				sr_disable
*				tempset
*				topiv
*				dfo
*
*	Globals modified:	loopdefs
*				searchdefset
*				exitdominators
*				loopchanging
*				preheader
*				anonymous_asgmt
*				tempset
*				topiv
*				restricted_rblocks
*
*	External calls:		new_set()
*				clearset()
*				optim()
*				xin()
*				setunion()
*				adelement()
*				setassign()
*				intersect()
*				fprintf()
*				printset()
*				walkf()
*				printinvs()
*				free()		{FREEIT}
*				freeset()	{FREESET}
*				difference()
*				nextel()
*				set_elem_vector()
*				ckalloc()
*
*******************************************************************************/
LOCAL void detect_indvars();
LOCAL void strength_reduction();

void loop_optimization()
{
	register i, j;
	register SET **r;
# ifndef COUNTING	/* It needs to be global for counting. */
	int loopcount;	
# endif COUNTING
	long *cvector, *rvector, *pelem;
	SET *exitnodes;		/* i is in exitnodes iff dfo[i] is an exit from
				   the currentregion (i.e. it has a successor
				   not in the region).
				*/

	loopdefs = new_set(maxdefs);
	searchdefset = new_set(maxdefs);
	exitnodes = new_set(maxnumblocks);
	exitdominators = new_set(maxnumblocks);
	restricted_rblocks = new_set(maxnumblocks);
	cvector = (long *)ckalloc(maxnumblocks*sizeof(long));
	rvector = (long *)ckalloc(maxnumblocks*sizeof(long));
	loopcount = 0;
	do
		{
		loopchanging = NO;
		for (currentregion = region; currentregion <= lastregion; currentregion++)
			{
			if (lastdef >= maxdefs - 2)
				/* No more room at the inn. Punt on further
				   opportunities.
				*/
				goto exit;
			if (! *currentregion)
				/* currentregion points to a region that
				   is invalid according to
				   check_region_overlap() due to overlapping
				   regions.
				*/
				continue;

			set_elem_vector(*currentregion, cvector);
			setassign(*currentregion, restricted_rblocks);
			for (r = currentregion - 1; r >= region; r--)
				if (*r)
					difference(*r, restricted_rblocks,
						restricted_rblocks);

			preheader = dfo[preheader_index[currentregion-region]];

			clearset(loopdefs);
			clearset(exitnodes);
			pelem = cvector;
			while ((i = *pelem++) >= 0)
				{
				if (dfo[i]->bb.treep )
					{
					/* canonicalize for sure */
					dfo[i]->bb.treep = optim(dfo[i]->bb.treep);
					setunion(blockdefs[i], loopdefs);
	
					j = -1;
					while ( (j = nextel(j, succset[i])) >= 0 )
						if ( !xin(*currentregion, j) )
							{
							adelement(i, exitnodes, exitnodes);
							break;
							}
					}
				}

			/* Clear out blds, since this block number is assumed to be
			 * outside of the loop before we call detect_li().  This is
			 * fine the first time through a loop, but wrong the second
			 * time, since any previous defs will still be present.
			 */
			for( j = -1; (j = nextel(j,loopdefs)) >= 0; )
				if( (i = deftab[j].sindex) >= 0 )
					symtab[ i ]->an.blds = 0;
	
			setassign(*currentregion, exitdominators);
			i = -1;
			while ( (i = nextel(i, exitnodes)) >= 0 )
				intersect(dom[i], exitdominators, exitdominators);

			/* The order in which regions are manipulated is probably
		   	important due to region nesting. The forward ordering
		   	used here seems appropriate so far.
			*/
			anonymous_asgmt = NO;
			detect_li(cvector);
# ifdef DEBUGGING
			if (gdebug > 1)
				{
				fprintf(debugp, "exitnodes for the region: ");
				printset(exitnodes);
				fprintf(debugp, "exitdominators for the region: ");
				printset(exitdominators);
				fprintf(debugp, "loopdefs for the region: ");
				printset(loopdefs);
				fprintf(debugp, "Loop invariant stmts in region[%3d]:\n", 
					currentregion - region);
				for (i = numblocks-1; i >= 0; i--)
					if ( xin(*currentregion, i)
						&& dfo[i]->bb.treep)
						walkf(dfo[i]->bb.treep, printinvs);
				for (i=0; i <= lastblock - blockpool; i++)
					fprintf(debugp, "\tdfo[%3d] = 0x%x\n", i, dfo[i]);
				}
# endif DEBUGGING
			/* For now, don't do code motion if anonymous assignments are
		   	present within the region. This decision may be revised later
		   	when a better method is implemented to locate these anonymous
		   	assignments precisely.
			*/
			if (!anonymous_asgmt)
				{
				set_elem_vector(restricted_rblocks, rvector);
				if (lastdef < maxdefs - 1)
					code_motion(cvector, rvector);
				if (!sr_disable && lastdef < maxdefs - 2)
					{
					tempset = new_set(maxdefs);
					topiv = -1;
					detect_indvars(rvector);
					strength_reduction(cvector);
					FREESET(tempset);
					if (topiv > -1)
						free_ivtab();
					}
				}
# ifdef DEBUGGING
			else
				{
				if (gdebug)
					fprintf(debugp, "region %d has an anonymous assignment\n",
						currentregion - region);
				}
# endif DEBUGGING
			}
		loopcount++;
		}
	while (loopchanging && loopcount < LITNO);

exit:
	FREEIT(cvector);
	FREEIT(rvector);
	FREESET(exitdominators);
	FREESET(exitnodes);
	FREESET(searchdefset);
	FREESET(loopdefs);
	FREESET(restricted_rblocks);
	restricted_rblocks = NULL;
}


/******************************************************************************
*
*	avoid_aivs()
*
*	Description:		avoid_aivs() traverses the tree from the point
*				of discovery of an induction variable. It turns
*				off the iv_checked flag for all children at
*				that point downward to prevent further
*				minor_aivs being detected within a subtree that
*				is to be removed/altered anyway.
*
*	Called by:		avoid_aivs()
*				add_indvar()
*				detect_indvars()
*
*	Input parameters:	np - a NODE ptr to the top of a subexpression
*				     to be disallowed as search material.
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
LOCAL void avoid_aivs(np)	register NODE *np;
{
top:
	np->in.iv_checked = YES;
	switch( optype(np->in.op) )
		{
		case BITYPE:
			avoid_aivs(np->in.right);
			/* fall thru */
		case UTYPE:
			np = np->in.left;
			goto top;
		}
}



/******************************************************************************
*
*	add_indvar()
*
*	Description:		Adds an induction variable triple to the ivtab
*					array.
*
*	Called by:		associated_indvar()
*				minor_aiv()
*				detect_indvars()
*
*	Input parameters:	index - deftab index for the assignment
*				jsp - ptr to symtab for "j" in the algorithm
*				fsp - ptr to symtab for "family"
*				scale - multiplicative factor
*				offset - additive factor
*				minor_aivnp - ptr to subexpression for an
*					implied temporary (not in the symtab)
*
*	Output parameters:	none
*
*	Globals referenced:	maxivsz
*				ivtab
*				topiv
*				gdebug
*				debugp
*				deftab
*				loopcount
*
*	Globals modified:	ivtab
*				maxivsz
*				topiv
*				_n_bivs
*				_n_aivs
*
*	External calls:		ckrealloc()
*				bcon()
*				fprintf()
*
*******************************************************************************/
LOCAL void add_indvar(index, ksp, fsp, scale, offset, minor_aivnp, bn)
		int index;
		HASHU *ksp, *fsp;
		NODE *scale;
		NODE *offset;
		NODE *minor_aivnp;
		int bn;
{
	register struct iv *ivp;

# ifdef COUNTING
	if (loopcount == 0)
		if (ksp == fsp)
			_n_bivs++;
		else
			_n_aivs++;
# endif COUNTING

	if (++topiv >= maxivsz)
		{
		maxivsz += IVINC;
		ivtab = (struct iv *) ckrealloc(ivtab,
					maxivsz * sizeof(struct iv) );
		}
	ivp = &ivtab[topiv];
	ivp->family_sp = fsp;
	ivp->temp_sp = NULL;
	ivp->sibling = NULL;
	ivp->j_sp = ksp;
	ivp->dti = index;
	ivp->mc = scale? scale : bcon(1, INT);
	ivp->ac = offset? offset : bcon(0, INT);
	ivp->tnp = minor_aivnp;
	ivp->bn = bn;
	ivp->initted = NO;
	if ( index >= 0 )
		avoid_aivs(deftab[index].np);
	else 
		minor_aivnp->in.iv_checked = YES;
		/* Don't look at it next iteration */
# ifdef DEBUGGING
	if (gdebug > 2)
		{
		fprintf(debugp, "induction var (%d) added:	", topiv);
		fprintf(debugp,"j_sp = 0x%x, with triple (0x%x, 0x%x, 0x%x)",
			ivp->j_sp, ivp->family_sp, ivp->mc, ivp->ac);
		if (index >= 0)
			fprintf(debugp,"\n\t\t(from deftab[%d] = 0x%x)\n",
				ivp->dti, deftab[ivp->dti]);
		else
			fprintf(debugp,", tnp = 0x%x\n", ivp->tnp);
		fwalk(ivp->mc, eprint,0);
		fwalk(ivp->ac, eprint,0);
		fprintf(debugp,"\n");
		}
# endif DEBUGGING
}


/******************************************************************************
*
*	biv_shape()
*
*	Description:		biv_shape() simply checks for the
*				correct form of a basic iv.  Other criteria are
*				checked back in detect_indvars().
*
*	Called by:		detect_indvars()
*
*	Input parameters:	i - deftab index of the pertinent ASSIGN stmt.
*
*	Output parameters:	none
*
*	Globals referenced:	deftab
*				searchdefset
*
*	Globals modified:	searchdefset - set of defs that have the 
*					appropriate shape for basic iv's.
*
*	External calls:		same()
*				adelement()
*
*******************************************************************************/
LOCAL void biv_shape(i) int i;
{
	register NODE *npr;
	NODE *npl;

	/* Assume canonicalization has been done. Note also that
	   biv_shape() is called only for deftab[] entries that
	   describe ASSIGNs (BITYPE).
	*/
	/* Basic ivs can have more than one definition within the loop so
	   long as they all have the correct form. Associated iv's, on the
	   other hand, must be defined only once within the loop.
	*/
	npr = deftab[i].np->in.right;
	if ( npr->in.op == PLUS )
		{
		npl = deftab[i].np->in.left;
		if (ISFTP(npl->in.type))
			/* We don't want biv's to be anything other than scalars. */
			return;
		if ( nncon(npr->in.left) || npr->in.left->in.invariant )
			{
			if ( same(npl, npr->in.right, NO, NO) )
				adelement(i, searchdefset, searchdefset);
			}
		/* I'm extending the definition of a basic indvar to include
		   those assignments whose increment is relatively constant
		   (i.e. invariant).
		*/
		else if ( npr->in.right->in.invariant &&
			same(npl, npr->in.left, NO, NO) )
				adelement(i, searchdefset, searchdefset);
		}
}


/******************************************************************************
*
*	set_cd()
*
*	Description:		Sets the "c" and "d" constant values specified
*				in A, S & U.
*
*	Called by:		associated_indvar()
*				minor_aiv()
*
*	Input parameters:	jvp - a ptr to an ivtab entry of interest
*				np - a NODE ptr to a constant subexpression
*				constval - the (relatively) constant value.
*					It may alternatively be loop invariant.
*
*	Output parameters:	cp - a ptr to the multiplicative constant
*				dp - a ptr to the additive constant
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		tcopy()
*				block()
*				makety()
*				bcon()
*				tmatch()
*
*******************************************************************************/
LOCAL void set_cd(cp, dp, jvp, np, constval)
	register struct iv *jvp;
	register NODE **cp, **dp;
	NODE *np;
	NODE *constval;	/* Do not alter original contents of constval */
{
	register TTYPE t;

# ifndef FLOAT_SR 
	if (ISFTP(constval->in.type))
#pragma BBA_IGNORE
		cerror("floating point constval type in set_cd()");
# endif FLOAT_SR

	switch(np->in.op)
		{
		case OREG:
		case FOREG:
		case NAME:
		case REG:
			*cp = tcopy(jvp->mc);
			*dp = tcopy(jvp->ac);
			break;
		case PLUS:
		case MINUS:
			*cp = tcopy(jvp->mc);
			t = tmatch(&(jvp->ac->in.type), &(constval->in.type));
			*dp = block(np->in.op, makety(tcopy(jvp->ac), t),
				makety(tcopy(constval), t), 0, t);
			break;
		case LS:
			t = tmatch(&(jvp->mc->in.type), &(constval->in.type));
			constval = block(LS, bcon(1,INT), tcopy(constval), INT,
					0);
			*cp = block(MUL, makety(tcopy(jvp->mc), t),
				makety(tcopy(constval), t), 0, t);
			t = tmatch(&(jvp->ac->in.type), &(constval->in.type));
			*dp = block(MUL, makety(tcopy(jvp->ac), t),
				makety(constval, t), 0, t);
			break;
		case MUL:
			t = tmatch(&(jvp->mc->in.type), &(constval->in.type));
			*cp = block(MUL, makety(tcopy(jvp->mc), t),
				makety(tcopy(constval), t), 0, t);
			t = tmatch(&(jvp->ac->in.type), &(constval->in.type));
			*dp = block(MUL, makety(tcopy(jvp->ac), t),
				makety(tcopy(constval), t), 0, t);
			break;
		}

	/* jvp->mc and jvp->ac probably contain constant expressions by now.
	   No problem. These expressions will be simplified en masse later
	   in detect_indvars() once all aivs have been detected.
	*/
}

/******************************************************************************
*
*	const_operand()
*
*	Description:		Checks an operand for relative constancy
*				for use in detecting induction variables.
*				It returns YES iff the operand is relatively
*				constant within the region.
*
*	Called by:		associated_indvar()
*				minor_aiv()
*				minor_aiv2()
*
*	Input parameters:	np - a NODE ptr to the operand being tested.
*
*	Output parameters:	none
*
*	Globals referenced:	loopdefs
*				lastfarg
*
*	Globals modified:	searchdefset
*
*	External calls:		locate()
*				intersect()
*				isemptyset()
*
*******************************************************************************/
LOCAL flag const_operand(np)	register NODE *np;
{
	HASHU *sp;

	if (!np->in.invariant)
		return(NO);
	if (np->in.op == SCONV)
		np = np->in.left;
	if (nncon(np))
		return(YES);
	if (np->in.common_base && lastfarg != -1)
		return(NO);
	if (!locate(np, &sp))
		return(NO);
	if (!sp->an.defset)
		return(YES);
	intersect(sp->an.defset, loopdefs, searchdefset);
	return (isemptyset(searchdefset));
}

/******************************************************************************
*
*	associated_indvar()
*
*	Description:		Finds associated induction variables based on
*				the existing basic induction variables within
*				the ivtab. Once found they are added to the 
*				ivtab.
*
*	Called by:		detect_indvars()
*
*	Input parameters:	kindex - a deftab index for a loopdef.
*
*	Output parameters:	none
*
*	Globals referenced:	deftab
*				lastfarg
*				ivtab
*				loopdefs
*				searchdefset
*				currentbp
*				blockdefs
*
*	Globals modified:	searchdefset
*
*	External calls:		locate()
*				difference()
*				delelement()
*				intersect()
*				isemptyset()
*				nextel()
*				const_operand()
*
*******************************************************************************/
LOCAL void associated_indvar(kindex)	int kindex;
{
	register int dd;
	register struct iv *ivp;
	NODE *rnp;	/* points to the binary op */
	NODE *mfactor;
	NODE *c, *d;
	HASHU *rhs_sp;	/* points to HASHU of "j" in the algorithm */
	HASHU *ksp;	/* points to HASHU of "k" in the algorithm */
	NODE *constval;
	int jindex;	/* deftab index for j within currentbp */

	rnp = deftab[kindex].np->in.right;

	/* assumes canonicalization of the trees */
	switch(rnp->in.op)
		{
		case FOREG:
			if ( (DECREF(rnp->tn.type) != BTYPE(rnp->tn.type)) 
				|| (rnp->allo.flagfiller & CXM) )
					goto notindvar;
			else goto simple;
		case OREG:
		case NAME:
		case REG:
			/* Simple assignments can be just a degenerate case
			   of k = j*1+0.
			*/
			if (!ISSIMPLE(rnp) && !ISPTR(rnp->in.type))
				goto notindvar;
simple:
			if (rnp->in.common_base && lastfarg != -1)
				goto notindvar;
			if ( !locate(rnp, &rhs_sp) )
				goto notindvar;
			ksp = symtab[deftab[kindex].sindex];
			constval = NULL;
			break;
		case MINUS:	/* Like PLUS but no SWAPping allowed. */
		case LS:	/* a form of MUL */
			constval = rnp->in.right;
			if (!const_operand(constval))
				goto notindvar;
			mfactor = rnp->in.left;
# ifdef FLOAT_SR
			if (mfactor->in.op == SCONV)
				mfactor = mfactor->in.left;
# endif FLOAT_SR
			if (!ISSIMPLE(mfactor)
				&& !ISPTR(mfactor->in.type))
				goto notindvar;
			if (mfactor->in.common_base && lastfarg != -1)
				goto notindvar;
			if ( !locate(mfactor, &rhs_sp) )
				goto notindvar;
			ksp = symtab[deftab[kindex].sindex];
			break;
		case PLUS:
		case MUL:
			if (!nncon(rnp->in.left) && rnp->in.right->in.invariant)
				{
				/* A minor canonicalization for invariancy */
				NODE *sp;
				SWAP(rnp->in.left, rnp->in.right);
				}
			constval = rnp->in.left;
			if (!const_operand(constval))
				goto notindvar;
			mfactor = rnp->in.right;
# ifdef FLOAT_SR
			if (mfactor->in.op == SCONV)
				mfactor = mfactor->in.left;
# endif FLOAT_SR
			if (!ISSIMPLE(mfactor) &&
				!ISPTR(mfactor->in.type))
				goto notindvar;
			if (mfactor->in.common_base && lastfarg != -1)
				goto notindvar;
			if ( !locate(mfactor, &rhs_sp) )
				goto notindvar;
			ksp = symtab[deftab[kindex].sindex];
			break;
		/* DIV cannot be a candidate because for integers it can
		   cause an mc == 0 which causes an increment/init value to
		   be 0. This is regardless of the possible divide by 0 
		   problem.
		*/
		default:
			goto notindvar;
		}

	/* Associated induction vars must be defined only 1 time in the loop. */
	if (!ksp->an.defset)
#pragma BBA_IGNORE
		cerror("Null ksp->an.defset in associated_indvar()");
	intersect(loopdefs, ksp->an.defset, searchdefset);
	delelement(kindex, searchdefset, searchdefset);
	if ( !isemptyset(searchdefset) )
		goto notindvar;

	for (ivp = ivtab; ivp <= &ivtab[topiv]; ivp++)
		{
		/* Looking for an induction var match for "j" */
		if (ivp->j_sp == rhs_sp)
			{
			/* Found one already in the table */
			if (ivp->j_sp == ivp->family_sp)
				{
				/* ivp points to a basic induction var */
				/* same family as basic indvar*/
				set_cd(&c, &d, ivp, rnp, constval);
				add_indvar(kindex, ksp, ivp->j_sp, c, d, 0,
					deftab[kindex].bn);
				}
			else
				{
				if (!rhs_sp->an.defset || !ivp->family_sp->an.defset)
#pragma BBA_IGNORE
					cerror("Null defset in associated_indvar()");
				/* ivp points to a nonbasic induction var */
				/* Check condition (b) from A S & U first */
				difference(loopdefs, rhs_sp->an.defset,
					searchdefset);
				intersect(currentbp->bb.l.in, searchdefset,
					searchdefset);
				if (!isemptyset(searchdefset))
					goto notindvar;

				/* Condition (a) ... This is the tough one! */
				/* For now, check only for induction vars that
				   are within the same block as j.
				*/
				intersect(rhs_sp->an.defset,
					blockdefs[currentbp->bb.node_position],
					searchdefset);
				if (!isemptyset(searchdefset))
					{
					/* k and j are defined in the same block */
					/* In this case the deftab is guaranteed
					   to be in order because of the way it
					   was filled.
					*/
					dd = 0;
					while ( (dd=nextel(dd, searchdefset)) >= 0 )
						/* Must be exactly 1 element */
						break;
					jindex = dd;
					if (jindex < kindex)
						/* j immediately before k */
						{
						intersect(ivp->family_sp->an.defset,
							blockdefs[currentbp->bb.node_position],
							searchdefset);
						if (!isemptyset(searchdefset))
							{
							/* 1+ defs of i within
							   this block.
							*/
							dd = jindex;
							if ( ((dd = nextel(dd, searchdefset)) >= 0)
								&& (dd < kindex) )
									goto notindvar;
							}
						set_cd(&c, &d, ivp, rnp, constval);
						add_indvar(kindex, ksp,
							ivp->family_sp, c, d, 0,
							deftab[kindex].bn);
						}
					}
				goto notindvar;
				}
			break;
			}
		}
notindvar:
	return;
}


/******************************************************************************
*
*	minor_aiv()
*
*	Description:		Searches for subexpressions that are not
*				assignments that would be candidates for strength
*				reduction if they were replaced by an assignment
*				to a temporary.
*
*	Called by:		detect_indvars()
*
*	Input parameters:	rnp - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	ivtab
*				topiv
*				blockdefs
*				searchdefset
*				currentbp
*				deftab
*
*	Globals modified:	searchdefset
*
*	External calls:		locate()
*				intersect()
*				isemptyset()
*				nextel()
*				const_operand()
*
*******************************************************************************/
LOCAL void minor_aiv(rnp)	register NODE *rnp;
{
	register struct iv *ivp;
	NODE *topnp;
	NODE *c,*d, *constval;
	NODE *operand;
	HASHU *jsp;	/* points to HASHU of "j" in the algorithm */
	int idef, jdef;
	int iindex, jindex, kindex;
	int locate_result;

	topnp = rnp;
	if (rnp->in.iv_checked)
		goto not_iv;
# ifndef FLOAT_SR 
	if (ISFTP(rnp->in.type))
		goto not_iv;
# endif FLOAT_SR

	/* Assumes canonicalization of the trees */
	switch(rnp->in.op)
		{
		case MINUS:	/* like PLUS but no SWAPping allowed. */
		case LS:	/* a form of MUL */
			constval = rnp->in.right;
			if (!const_operand(constval))
				goto not_iv;
			operand = rnp->in.left;
			if (operand->in.op == SCONV)
				operand = operand->in.left;
			if (!ISSIMPLE(operand)) goto not_iv;
			if (operand->in.common_base && lastfarg != -1)
				goto not_iv;
			locate_result = locate(operand, &jsp);
			if ( !nncon(operand) && !locate_result)
#pragma BBA_IGNORE
				goto weird;
			break;
		case PLUS:
		case MUL:
			if (!nncon(rnp->in.left) && rnp->in.right->in.invariant)
				{
				/* A minor canonicalization for invariancy */
				NODE *sp;
				SWAP(rnp->in.left, rnp->in.right);
				}
			constval = rnp->in.left;
			if (!const_operand(constval))
				goto not_iv;
			operand = rnp->in.right;
			if (operand->in.op == SCONV)
				operand = operand->in.left;
			if (!ISSIMPLE(operand)) goto not_iv;
			if (operand->in.common_base && lastfarg != -1)
				goto not_iv;
			if ( !locate(operand, &jsp) )
#pragma BBA_IGNORE
				goto weird;
			break;
		default:
			goto not_iv;
		}
	for (ivp = ivtab; ivp <= &ivtab[topiv]; ivp++)
		{
		/* Looking for an induction var match for "j" */
		if (ivp->j_sp == jsp)
			{
			/* Found one already in the table */
			if (ivp->j_sp == ivp->family_sp)
				{
				/* ivp points to a basic induction var */
				/* same family as basic indvar*/
				set_cd(&c, &d, ivp, topnp, constval);
# ifdef COUNTING
				_n_minor_aivs++;
# endif COUNTING
				add_indvar(-1, NULL, ivp->family_sp, c, d,
					topnp, currentbp->bb.node_position);
				currentbp->bb.treep->nn.iv_found = YES;
				return;
				}
			else
				{
				/* ivp points to a nonbasic induction var */
				/* Check condition (b) from A S & U first */
# ifdef DEBUGGING
				if (!jsp->an.defset || !ivp->family_sp->an.defset)
#pragma BBA_IGNORE
					cerror("Null defset in minor_aiv()");
# endif DEBUGGING
				difference(loopdefs, jsp->an.defset,
					searchdefset);
				intersect(currentbp->bb.l.in, searchdefset,
					searchdefset);
				if (!isemptyset(searchdefset))
					goto not_iv;

				/* Condition (a) ... This is the tough one! */
				/* For now, check only for induction vars that
				   are within the same block as j.
				*/
				intersect(jsp->an.defset,
					blockdefs[currentbp->bb.node_position],
					searchdefset);
				if (!isemptyset(searchdefset))
					{
					/* k and j are defined in the same block */
					jdef = 0;
					while ( (jdef = nextel(jdef,searchdefset)) >= 0 )
						/* Must be exactly 1 element */
						break;
					jindex = deftab[jdef].np->in.index;
					kindex = rnp->in.index;
					intersect(ivp->family_sp->an.defset,
						blockdefs[currentbp->bb.node_position],
						searchdefset);
					idef = -1;
					while ( (idef = nextel(idef,searchdefset)) >= 0 )
						{
						/* i, being basic,  may be
						   defined more than once within
						   the loop.
						*/
						iindex = deftab[idef].np->in.index;
						if (jindex < kindex)
							{
							/* j seen before k */
							if ( (jindex < iindex)
							   &&(iindex < kindex) )
								goto not_iv;
							}
						else
							/* betweenness difficult
							   to determine here.
							*/
							goto not_iv;
						}
					set_cd(&c, &d, ivp, topnp, constval);
# ifdef COUNTING
					_n_minor_aivs++;
# endif COUNTING
					add_indvar(-1, NULL, ivp->family_sp, c,
						d, topnp,
						currentbp->bb.node_position);
					currentbp->bb.treep->nn.iv_found = YES;
					return;
					}
				}
			return;
			}
		}
	return;
weird:
	cerror("Anonymous assignment in minor_aiv()");
	return;
not_iv:
	return;
}


/******************************************************************************
*
*	aiv_loc()
*
*	Description:		aiv_loc() returns the address of the ivtab
*				element for the node pointer np.  If np does
*				not correspond to an ivtab element, it returns
*				NULL.
*
*	Called by:		minor_aiv2()
*
*	Input parameters:	np - a NODE ptr.
*
*	Output parameters:	none
*
*	Globals referenced:	ivtab
*				topiv
*				deftab
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
LOCAL struct iv * aiv_loc(np)	register NODE *np;
{
	register struct iv *ivp;

	if (np->in.op != COMOP)
		for (ivp = &ivtab[topiv]; ivp >= ivtab; ivp--)
			{
			if (ivp->dti != -1)
				break;	/* No more minor aivs in the table */
			if (ivp->tnp == np)
			return(ivp);
			}
	return(NULL);
}

/******************************************************************************
*
*	tmatch()
*
*	Description:		a simple minded routine that returns the type
*				of a + or - node given its operands. Its
*				use is restricted to creating "c" and "d" for
*				strength reduction.
*
*	Called by:		minor_aiv2()
*				add_sinit()
*				set_cd()
*
*	Input parameters:	t1, t2 - ptrs to the types of the two operands
*
*	Output parameters:	none
*				(returns the appropriate type of the BINARY OP)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
LOCAL TTYPE tmatch(t1, t2)	register TTYPE *t1, *t2;
{
	static TTYPE tret = {0,0,0};
	short b1,b2;
	flag uflag;

	if (SAME_TYPE((*t1), (*t2)))
		return(*t1);
	if (ISPTR(*t1))
		return(*t1);
	if (ISPTR(*t2))
		return(*t2);
	if (ISFTP(*t1))
		return(*t1);
	if (ISFTP(*t2))
		return(*t2);

	/* If we reach here, we're basically arbitrating between scalars. */
	uflag = ISUNSIGNED(*t1) || ISUNSIGNED(*t2);
	b1 = t1->base;
	b2 = t2->base;
	if (b1 >= UCHAR)
		b1 += CHAR - UCHAR;
	if (b2 >= UCHAR)
		b2 += CHAR - UCHAR;
	b1 = max(b1, b2);
	if (uflag)
		b1 += UCHAR - CHAR;
	tret.base = b1;
	return(tret);
}


/******************************************************************************
*
*	minor_aiv2()
*
*       Description:            minor_aiv2() attempts to find associated
*				induction variables of the form m*(x +
*				b) where (x + b) is a previously
*				identified associated induction variable
*				that, because it involves a PLUS or
*				MINUS, would not ordinarily be replaced
*				by strength reduction.  The overall
*				induction variable is cost effective,
*				however.
*
*	Called by:		detect_indvars()
*
*	Input parameters:	rnp - a NODE ptr to a candidate subexpression
*
*	Output parameters:	none
*
*	Globals referenced:	debugp
*				ivtab
*
*	Globals modified:	ivtab
*				_n_constructed_aivs
*
*	External calls:		fprintf()
*				bcon()
*				aiv_loc()
*				block()
*				tcopy()
*				locate()
*				intersect()
*				delelement()
*				isemptyset()
*				tmatch()
*				makety()
*				fwalk()
*				const_operand()
*
*******************************************************************************/
LOCAL void minor_aiv2(rnp)	register NODE *rnp;
{
	NODE *lnp;
	struct iv *ivp;
	NODE * constval;
	HASHU *ksp;
	TTYPE t;

	if (rnp->in.iv_checked) return;

	/* Assumes canonicalization of the trees */
	lnp = rnp->in.left;
	switch(rnp->in.op)
		{
		case MINUS:
		case LS:	/* a form of MUL */
			if (!const_operand(rnp->in.right))
				return;
			if ( (ivp = aiv_loc(lnp)) == NULL )
				return;
			constval = rnp->in.op == LS?
				block(LS, bcon(1,INT), tcopy(rnp->in.right),
					0, lnp->in.type)
				: tcopy(rnp->in.right);
			break;
		case PLUS:
		case MUL:
			if (!nncon(lnp) && rnp->in.right->in.invariant)
				{
				/* A minor canonicalization for invariancy */
				NODE *sp;
				SWAP(rnp->in.left, rnp->in.right);
				lnp = rnp->in.left;
				}
			if (!const_operand(lnp))
				return;
			lnp = rnp->in.right;
			if ( (ivp = aiv_loc(lnp)) == NULL )
				return;
			constval = tcopy(rnp->in.left);
			break;
		case ASSIGN:
			if ( (ivp = aiv_loc(rnp->in.right)) == NULL )
				return;
			if ( !locate(lnp, &ksp) )
				return;

			/* Aivs must be defined once per loop.*/
			if (!ksp->an.defset)
				/* No defset implies that this is a variable
				   of some improper class (e.g. equiv) that
				   was never added to the defset. Don't attempt
				   to optimize it in any way.
				*/
				return;
			intersect(loopdefs, ksp->an.defset, searchdefset);
			delelement(rnp->in.defnumber, searchdefset, searchdefset);
			if ( !isemptyset(searchdefset) )
				return;

			/* If j is basic, then simply revise the ivp entry. If
			   it's not, then the conditions of A,S&U must be
			   checked. However, these conditions were already
			   checked for the subexpression on the rhs in
			   minor_aiv(). Again, only modify the ivp entry.
			*/
			if (!ivp->j_sp)
				ivp->j_sp = ksp;
			ivp->dti = rnp->in.defnumber;
			ivp->tnp = NULL;
			break;
		default:
			return;
		}

	/* Revise the existing ivtab entry to consume the larger subtree. */
	switch (rnp->in.op)
		{
		case LS:
		case MUL:
			t = tmatch(&(ivp->mc->in.type), &(constval->in.type));
			ivp->mc = block(MUL, makety(ivp->mc, t),
					makety(constval, t), 0, t);
			ivp->ac = block(MUL, makety(ivp->ac, t),
					makety(tcopy(constval), t) , 0, t);
			ivp->tnp = rnp;
			break;
		case MINUS:
		case PLUS:
			t = tmatch(&(ivp->mc->in.type), &(constval->in.type));
			ivp->ac = block(rnp->in.op, makety(ivp->ac, t),
					makety(constval, t), 0, t);
			ivp->tnp = rnp;
			break;
		/* Do nothing.
		case ASSIGN:
			break;
		*/
		}
# ifdef COUNTING
	_n_constructed_aivs++;
# endif COUNTING
	changing = YES;
# ifdef DEBUGGING
	if (gdebug > 2)
		{
		fprintf(debugp,"Induction var %d modified.", ivp - ivtab);
		fprintf(debugp,"j_sp = 0x%x, with triple (0x%x, 0x%x, 0x%x)",
			ivp->j_sp, ivp->family_sp, ivp->mc, ivp->ac);
		fprintf(debugp,", tnp = 0x%x\n", ivp->tnp);
		fwalk(ivp->mc,eprint,0);
		fwalk(ivp->ac,eprint,0);
		fprintf(debugp,"\n");
		}
# endif DEBUGGING
}





/******************************************************************************
*
*	detect_indvars()
*
*       Description:            detect_indvars() finds a list of
*				induction variables for a given region.
*				Structs defining these vars are added to
*				the iv list.  This is an implementation
*				of algorithm 10.9 in A,S & U.
*
*	Called by:		loop_optimization()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	searchdefset
*				lastdef
*				deftab
*				loopdefs
*				dfo
*				symtab
*				maxdefs
*				tempset
*				topiv
*				currentbp
*				changing
*				ivtab
*				topiv
*				restricted_rblocks
*				debugp
*
*	Globals modified:	searchdefset
*				deftab
*				tempset
*				currentbp
*				changing
*				ivtab
*
*	External calls:		clearset()
*				nextel()
*				set_elem_vector()
*				new_set()
*				intersect()
*				difference()
*				xin()
*				delelement()
*				isemptyset()
*				bcon()
*				free()		{FREESET}
*				walkf()
*				optim()
*				biv_shape()
*				avoid_aivs()
*				associated_indvar()
*				minor_aiv()
*				minor_aiv2()
*				fprintf()
*				fwalk()
*
*******************************************************************************/
LOCAL void detect_indvars(rvector)	long *rvector;
{
	register int i;
	register long *pelem;
	register struct iv *ivp;
	register SET *splset;
	HASHU *sp;
	NODE *np;
	long *vector;
	int ivsave;
	int blockno;

	i = max(numblocks, (lastdef+1));
	vector = pelem = (long *)ckalloc(i * sizeof(long));

	/* Step 1 ... Identify basic induction variable assignments */
	clearset(searchdefset);
	set_elem_vector(loopdefs, vector);
	while (*pelem >= 0)
		deftab[*pelem++].np->allo.flagfiller &= IV_FLAGS;
	pelem = vector;
	while (*pelem >= 0)
		{
		/* interested only in defs within the current region (loop) */
		np = deftab[*pelem].np;
		if (callop(np->in.op) && !np->in.safe_call)
			{
			/* Don't find any iv's in blocks containing
			   CALL ops  that generate any definitions
			   (or, in fact, any op that alters the
			   L-R-Postfix traversal) because "between" is
			   indeterminate in these blocks. If basic blocks
			   are not added for these blocks, neither can
			   associated iv's be added for them.
			*/
			if ( NO_SIDE_EFFECTS &&
				(np->in.op == UNARY CALL || np->in.op == UNARY STCALL) )
				/* Very simple cases */
				continue;
			blockno = deftab[*pelem].bn;
			avoid_aivs(dfo[blockno]->bb.treep);
			while ( (*pelem<lastdef) && deftab[*pelem+1].bn == blockno) 
				pelem++;
			goto next;
			}
		if ( (np->in.op != ASSIGN) || (deftab[*pelem].children)
			|| (!ISSIMPLE(np->in.left) && !ISPTR(np->in.left->in.type)) )
			{
			/* Turn off all further searches. (Top plus children
			   all have np point to the same node.)
			*/
			deftab[*pelem].np->in.iv_checked = YES;
			pelem += deftab[*pelem].children;
			goto next;
			}

		biv_shape(*pelem);	/* manipulates searchdefset */
next:		pelem++;
		}

	/* Remove those defs which are within inner subregions. */
	set_elem_vector(searchdefset, vector);
	pelem = vector;
	while (*pelem >= 0)
		{
		if (!xin(restricted_rblocks, deftab[*pelem].bn))
			delelement(*pelem, searchdefset, searchdefset);
		pelem++;
		}

	/* At this point, searchdefset contains all pertinent assignments
	   within the loop of the form a := a +/- <const>. It remains to be 
   	   determined which vars have ONLY this form within the loop.
	*/
	
	splset = new_set(maxdefs);
	set_elem_vector(searchdefset, vector);
	pelem = vector;
	while (*pelem >= 0)
		{
		int sp_def;
		sp = symtab[deftab[*pelem].sindex];
		if (sp->an.defset)
			intersect(loopdefs, sp->an.defset, splset);
		else
			clearset(splset);
		/* splset should now contain only those defs of sp
		   within the loop.
		*/

		difference(searchdefset, splset, tempset);	/* C = B - A */

		if ( isemptyset(tempset) )
			add_indvar(*pelem, sp, sp, bcon(1, INT), NULL, NULL,
				deftab[*pelem].bn);
		
		/* Now remove all defs of sp from further consideration
		   so that multiple instances of the same basic iv are
		   not added to the list.  Later, add_sincr() will still
		   correctly insert temporary incrementation code for
		   each and every basic iv definition, regardless of
		   whether or not it's in the ivtab.
		*/
		sp_def = 0;
		while ( (sp_def = nextel(sp_def, splset)) >= 0 )
			avoid_aivs(deftab[sp_def].np);
		difference(splset, searchdefset, searchdefset);
		pelem++;
		}
	FREESET(splset);
	/* Searchdefset is now released to be used for another purpose. */
			

		 

	/* Step 2 ... Identify associated induction variables */
	set_elem_vector(loopdefs, vector);
	do
		{
		ivsave = topiv;
		pelem = vector;
		while (*pelem >= 0)
			{
			if (xin(restricted_rblocks, deftab[*pelem].bn))
				{
				currentbp = dfo[deftab[*pelem].bn];
				if (!deftab[*pelem].np->in.iv_checked)
					associated_indvar(*pelem);
				}
			pelem++;
			}
		}
	while (ivsave != topiv);

	/* Step 2a. Now look for smaller fragments that may be suitable as
	   associated induction variables.
	*/
	do
		{
		pelem = rvector;
		ivsave = topiv;
		while (*pelem >= 0)
			{
			NODE *treep;
			if (treep = dfo[*pelem]->bb.treep)
				{
				currentbp = dfo[*pelem];
				walkf(treep, minor_aiv);
				}
			pelem++;
			}
		}
	while (ivsave != topiv);

	/* Step 2b. Now check for larger minor aivs that contain these
	   previously identified associated induction variables.
	*/
	do
		{
		changing = NO;
		pelem = rvector;
		while (*pelem >= 0)
			{
			NODE *treep;
			if ((treep = dfo[*pelem]->bb.treep) && treep->nn.iv_found)
				walkf(treep, minor_aiv2);
			pelem++;
			}
		}
	while (changing);

	FREEIT(vector);

	for (ivp = &ivtab[topiv]; ivp >= ivtab; ivp--)
		{
		ivp->ac = optim(ivp->ac);
		ivp->mc = optim(ivp->mc);
		}
# ifdef DEBUGGING
	if (gdebug >= 2)
		{
		fprintf(debugp,"Induction table after detect_indvars()\n");
		for (ivp = ivtab; ivp <= &ivtab[topiv]; ivp++)
			{
			fprintf(debugp,"j_sp = 0x%x, with triple (0x%x, 0x%x, 0x%x)",
				ivp->j_sp, ivp->family_sp, ivp->mc, ivp->ac);
			fprintf(debugp,", tnp = 0x%x\n", ivp->tnp);
			if ( ivp->dti >= 0 )
				fprintf(debugp,"\t\t(from deftab[%d] = 0x%x)\n",
					ivp->dti, deftab[ivp->dti]);
			fwalk(ivp->mc,eprint,0);
			fwalk(ivp->ac,eprint,0);
			fprintf(debugp,"\n");
			}
		fprintf(debugp,"\n");
		}
# endif DEBUGGING
}

/******************************************************************************
*
*	set_dt()
*
*	Description:		set_dt() is a tree traversal helper (fwalk)
*				that associates a controlling ";" node with
*				each pertinent element of the deftab[].
*				After it is complete it will be possible to
*				find the "top" of each statement for each
*				deftab[] entry pointing within a loop.  This
*				capability is required in order that new
*				statements be inserted into a tree during
*				strength reduction optimization.
*
*	Called by:		strength_reduction()
*
*	Input parameters:	np - a NODE ptr to a subtree during traversal
*
*	Output parameters:	none
*
*	Globals referenced:	deftab
*
*	Globals modified:	deftab
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void set_dt(np)
register NODE *np;
{
static NODE *last_semi;

top:
	/* Parameter decs may cause LINT to complain. */
	switch(np->in.op)
		{
		case SEMICOLONOP:
		case UNARY SEMICOLONOP:
			last_semi = np;
			break;
		case CM:
		case UCM:
		case ASSIGN:
		case RETURN:
		case CAST:
		case STASG:
		case STARG:
		case INCR:
		case DECR:
			deftab[np->in.defnumber].stmt = last_semi;
			break;
		case CALL:
			if (np->in.safe_call)
				break;
			/* fall thru */
		case UNARY CALL:
		case STCALL:
		case UNARY STCALL:
			if (np->in.defnumber)
				{
				int dn;
				DFT *dp;

				dp = &deftab[np->in.defnumber];
				for (dn = dp->children; dn >= 0; dp++, dn--)
					dp->stmt = last_semi;
				}
			break;
		}

	switch (optype(np->in.op))
		{
		case LTYPE:
			return;
		case BITYPE:
			set_dt(np->in.right);
			/* fall thru */
		case UTYPE:
			np = np->in.left;
			goto top;
		}
}

/******************************************************************************
*
*	in_subtree()
*
*	Description:		a boolean function that returns YES iff the
*				operand search has a twin in the subexpression.
*				It accomplishes this check by means of a
*				recursive search. Only LTYPE nodes are checked.
*
*	Called by:		add_sincr()
*				in_subtree()	(recursively)
*
*	Input parameters:	search - a NODE ptr to a subtree being searched
*				match - a NODE ptr to an LTYPE node for which
*					we are searching for a twin.
*				interim - a NODE ptr to a subtree that will eventually
*					be replaced by a copy of *match but that hasn't
*					occurred yet due to the ordering of
*					strength_reduction().
*
*	Output parameters:	none
*				returns YES or NO as described above.
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		same()
*
*******************************************************************************/
LOCAL flag in_subtree(match, search, interim) NODE *match, *search, *interim;
{

top:
	/* If the new temp is to replace more than one aiv it is possible that
	   not all the replacements are complete. In this case check instead
	   to see if the aiv itself (rather than the temp) is within the search
	   tree.
	*/
	if (interim == search)
		return(YES);

	switch(optype(search->in.op))
		{
		case BITYPE:
			if (in_subtree(match, search->in.right, interim))
				return(YES);
			/* fall thru */
		case UTYPE:
			search = search->in.left;
			goto top;	/* Avoid tail recursion */
		case LTYPE:
			return(same(match, search, NO, NO));
		}
}

/******************************************************************************
*
*	find_increment()
*
*	Description:		Given the basic induction variable increment
*				assignment subtree, find the constant increment
*				and return it. Since the tree may not always
*				be canonicalized, ensure that you get the
*			        correct operand.
*
*	Called by:		add_sincr()
*
*	Input parameters:	xnp - a NODE ptr pointing to the ASSIGN op
*					in the increment subexpression.
*
*	Output parameters:	none
*				(returns a NODE ptr to the constant operand.)
*
*	Globals referenced:	none
*
*	Globals modified:	none
*
*	External calls:		same()
*
*******************************************************************************/
LOCAL NODE *find_increment(xnp)	NODE *xnp;
{
	register NODE *increment;

	increment = xnp->in.right->in.left;
	if (same(increment, xnp->in.left, NO, NO))
		/* Oops, selected wrong side of PLUS */
		increment = xnp->in.right->in.right;
	return(increment);
}

/******************************************************************************
*
*	add_sincr()
*
*       Description:            add_sincr() implements step 3 of
*				algorithm 10.10 of A, S & U.
*				"Immediately after each assignment i:= i
*				+ n in L [i is a basic induction
*				variable and L is the loop (region)],
*				where n is a constant, append:  s := s +
*				c*n where the expression c*n is
*				evaluated to a constant because c and n
*				are constants.  Place s in the family of
*				i, with triple (i,c,d)."
*
*	Called by:		strength_reduction()
*
*	Input parameters:	jivp - a ptr to an ivtab entry describing "j"
*				tnp - a ptr to the NODE describing the new temp.
*
*	Output parameters:	none
*
*	Globals referenced:	loopdefs
*				tempset
*				lastdef
*				deftab
*				blockcount
*				blockdefs
*				lastregion
*				region
*				currentregion
*
*	Globals modified:	tempset
*				blockcount
*				deftab
*
*	External calls:		intersect()
*				xin()
*				block()
*				tcopy()
*				adelement()
*				tfree()
*				nextel()
*				in_subtree()
*
*******************************************************************************/
LOCAL NODE *add_sincr(jivp, tnp)
	register struct iv *jivp; 
	NODE *tnp;
{
	static TTYPE inttype = {INT, 0, 0};
	register int i;
	register NODE *xnp;
	register NODE *newnp;
	register NODE *increment;
	int first_incr, first_pred;
	int incr_bn;
	NODE *revised_tnp;	/* Useful copy of the original tnp */
				/* THIS one, at least, will not be freed */
	NODE *cnp, *lnp;
	SET **lregion;
	struct iv *ivp3;
	TTYPE type, type2;
	flag more_than_one_incr;
	flag more_than_one_pred;
	flag aiv_is_a_temp;
	flag no_uses_in_biv_block;
	flag simple_predecessor;

	if (jivp->initted)	/* Do not duplicate the incr operations */
		return(tnp);

	type = tnp->in.type;
	simple_predecessor = NO;	/* Initialization */

	/* Make the increment  a simple INT if type is PTR. Otherwise,
	   the register allocator may incorrectly put the constant into a
	   register.
	*/
	type2 = ISPTR(type)? inttype : type;

	revised_tnp = NULL;
	intersect(loopdefs, jivp->family_sp->an.defset, tempset);

	/* For simple regions where the biv increment occurs only once
	   and the block in which it occurs has a single predecessor, we
	   can do some adjustments to the basic algorithm that might
	   make for better code. In particular, this sometimes allows c2
	   to change A register references into postincrements vice
	   reference followed by a separate add instruction.
	*/
	if (aiv_is_a_temp = ISTEMP(tnp))
		{
		first_incr = nextel(-1, tempset);
		more_than_one_incr =  (nextel(first_incr, tempset) > 0);
		incr_bn = deftab[first_incr].bn;
		first_pred = nextel(-1, predset[incr_bn]);
		more_than_one_pred = (nextel(first_pred, predset[incr_bn]) > 0);

		no_uses_in_biv_block = (jivp->bn != incr_bn);
		for (ivp3 = jivp + 1; no_uses_in_biv_block && ivp3 <= &ivtab[topiv]; ivp3++)
			no_uses_in_biv_block = (ivp3->sibling != jivp)
						|| (ivp3->bn != incr_bn);

		/* Check for a conveniently shaped predecessor to the biv
		   increment block. Some are too hard (and too rare) to be
		   worthwhile for this stuff.
		*/
		switch (dfo[first_pred]->bb.breakop)
			{
			case FREE:
			case LGOTO:
				/* Adding a stmt will be easy. */
				simple_predecessor = YES;
				break;
			case CBRANCH:
				/* Unless the "other" path of the branch is outside
				   the loop, forget this optimization.
				*/
				i = -1;
				if ((i = nextel(i, succset[first_pred])) == incr_bn)
					if ((i = nextel(i, succset[first_pred])) == -1) break;
				if (xin(*currentregion, i))
					break;

				/* Check to see that the branch condition
				   doesn't involve the biv.
				*/
				simple_predecessor = YES;
				xnp = dfo[first_pred]->bb.treep;
				xnp = (xnp->in.op == SEMICOLONOP)?
					xnp->in.right : xnp->in.left;
				for (ivp3 = jivp; simple_predecessor && ivp3 <= &ivtab[topiv]; ivp3++)
					if ((ivp3==jivp||ivp3->sibling == jivp)
						&& ivp3->bn == first_pred)
						simple_predecessor = 
							!in_subtree(jivp->tnp, xnp->in.left, ivp3->tnp);
				
				break;
			case FCOMPGOTO:
			case GOTO:
				/* Too weird to be important. Maybe someday. */
				break;
# ifdef DEBUGGING
			case EXITBRANCH:
				/* How can a predecessor be an EXIT node ? */
				cerror("Weird predecessor in add_sincr()");
				break;
# endif DEBUGGING
			}
		if (simple_predecessor)
			for (lregion = region;
			  simple_predecessor && lregion<=lastregion;
			  lregion++)
			  if (*lregion && lregion != currentregion)
			    simple_predecessor = !xin(*lregion, first_pred);
		}

	/* Assuming all the necessary conditions are met, let's now move
	   the increment up to the previous block.  Doing this will make
	   it easier for c2 to (possibly) combine the increment with a
	   use of the aiv to produce a postincrement, which is a free
	   increment. c2 can never combine instructions on either side
	   of a label.
	*/

	if (aiv_is_a_temp && !more_than_one_incr && !more_than_one_pred
		&& no_uses_in_biv_block && simple_predecessor)
		{
			increment = find_increment(deftab[first_incr].np);
			increment = block(MUL, makety(tcopy(increment), type2), 
					makety(tcopy(jivp->mc),type2),0, type2);
			newnp = block(PLUS, increment, tcopy(tnp), 0, type);
			newnp = optim(block(ASSIGN, revised_tnp = tcopy(tnp),
					newnp, 0, type));

			xnp = dfo[first_pred]->bb.treep;
			switch(dfo[first_pred]->bb.breakop)
				{
				case FREE:
					dfo[first_pred]->bb.treep = (xnp)?
							block(SEMICOLONOP, xnp, newnp,
								0, NULL) :
							block(UNARY SEMICOLONOP, newnp,
								NULL, 0, NULL);
					break;
				case LGOTO:
				case CBRANCH:
					if (xnp->in.op == SEMICOLONOP)
						xnp->in.left = block(SEMICOLONOP,
							xnp->in.left, newnp, 0, NULL);
					else
						{
						xnp->in.right = xnp->in.left;
						xnp->in.left = 
							block(UNARY SEMICOLONOP,
								newnp, NULL, 0, NULL);
						xnp->in.op = SEMICOLONOP;
						}
					break;
				}

			/* Now to update some bookkeeping. */
			blockcount = first_pred;
		}

	else
	   for (i = lastdef; i >= 1; i--)
		/* Should go in this order because lastdef is reset within. */
		if (xin(tempset, i))
			{
			xnp = deftab[i].np;	/* Shorthand */
			increment = find_increment(xnp);
			increment = block(MUL, makety(tcopy(increment), type2), 
					makety(tcopy(jivp->mc),type2),0, type2);

			/* First make a new assignment subtree & bookkeeping */
			newnp = block(PLUS, increment, tcopy(tnp), 0, type);
			newnp = optim(block(ASSIGN, revised_tnp = tcopy(tnp),
					newnp, 0, type));

			/* Now put it into the appropriate spot in the tree */
			cnp = tcopy(xnp);
			lnp = tcopy(cnp->in.left);
			tfree(xnp);
			xnp->in.op = COMOP;
			xnp->in.left = cnp;
			xnp->in.right = block(COMOP, newnp, lnp, 0, type);


			/* deftab[i] has moved down the tree */
			deftab[i].np = cnp;

			/* Now to update some bookkeeping. */
			blockcount = deftab[i].bn;
			}

	init_defs(newnp);
	adelement(lastdef, jivp->temp_sp->an.defset, jivp->temp_sp->an.defset);
	return(revised_tnp);
}

/******************************************************************************
*
*	add_sinit()
*
*	Description:		Add_sinit() implements step 4 of algorithm
*				10.10 of A, S & U.  "It remains to ensure that
*				s is initialized to c*i + d on entry to the
*				loop. The initialization may be placed at the
*				end of the preheader.  The initialization
*				consists of 
*					s := c*i	{just s:= i if c is 1}
*					s := s+d	{omit if d is 0 } "
*
*	Called by:		strength_reduction()
*
*	Input parameters:	jivp - a ptr to the ivtab entry for "j"
*				tnp - a ptr to the NODE describing the new temp.
*
*	Output parameters:	none
*
*	Globals referenced:	deftab
*
*	Globals modified:	none
*				(the tree is modified)
*
*	External calls:		tcopy()
*				block()
*				new_definition()
*				locate()
*				cerror()
*				tmatch()
*				makety()
*
*******************************************************************************/
LOCAL void add_sinit(jivp, tnp)	register struct iv *jivp; NODE *tnp;
{
	register NODE *newnp;
	register HASHU *sp;
	register TTYPE t;
	HASHU *ssp;
	int i;

	sp = jivp->family_sp;
	switch(sp->an.tag)
		{
		case AN:
			newnp = block(NAME, sp->an.offset, 0, 0, sp->an.type);
			newnp->atn.name = sp->an.ap;
			break;
		case A6N:
			newnp = block(OREG, sp->a6n.offset, TMPREG, 0,
				sp->a6n.type);
			break;
		case ON:
			newnp = block(REG, sp->on.regval, 0, 0, sp->on.type);
			break;
		default:
			cerror("unknown biv node type used in add_sinit()");
		}

	if (!nncon(jivp->mc) || jivp->mc->tn.lval != 1)
		{
		t = tmatch(&(newnp->in.type), &(jivp->mc->in.type));
		newnp = block(MUL, makety(tcopy(jivp->mc), t),
				makety(newnp, t), 0, t);
		}

	if (!nncon(jivp->ac) || jivp->ac->tn.lval != 0)
		{
		t = tmatch(&(newnp->in.type), &(jivp->ac->in.type));
		newnp = block(PLUS, makety(tcopy(jivp->ac), t), 
				makety(newnp, t), 0, t);
		}

	newnp = block(ASSIGN, tcopy(tnp), makety(newnp, tnp->in.type), 0,
		tnp->in.type);
	i = locate(tnp, &ssp)? ssp->an.symtabindex : -1;
	newnp->in.defnumber = new_definition(newnp, i);
	addnode(newnp);
# 	ifdef COUNTING
	_n_strength_reductions++;
# 	endif COUNTING
}


/******************************************************************************
*
*	remove_obsolete_def()
*
*	Description:		Definitions that are destroyed by strength_
*				reduction() must be removed from the loopdefs
*				and sp->an.defset sets in order for the next
*				iteration of detect_indvars() to find new
*				basic induction variables.
*
*	Called by:		strength_reduction()
*
*	Input parameters:	dno - definition number
*
*	Output parameters:	none
*
*	Globals referenced:	symtab
*				loopdefs
*
*	Globals modified:	symtab
*				loopdefs
*
*	External calls:		delelement()
*
*******************************************************************************/
LOCAL void remove_obsolete_def(dno)	int dno;
{
	HASHU *sp;

	sp = symtab[deftab[dno].sindex];
	delelement(dno, loopdefs, loopdefs);
	delelement(dno, sp->an.defset, sp->an.defset);
}



/******************************************************************************
*
*	sr_worthit()
*
*	Description:		Checks to see if the associated induction
*				variable indicated by the (cp,dp) pair would
*				actually result in faster object code being
*				produced. It returns YES iff it does. Not
*				all valid strength reductions actually result
*				in faster code (e.g., some multiplies of powers
*				of 2 are done very quickly by a left shift;
*				hence no speedup is accomplished if they are
*				replaced by an add.)
*
*	Called by:		strength_reduction()
*
*	Input parameters:	ivp - a ptr to an associated iv.
				bivp - a ptr to its basic iv.
*
*	Output parameters:	none
*				(returns YES iff the strength reduction
*				would result in faster object code)
*
*	Globals referenced:	deftab
*
*	Globals modified:	none
*
*	External calls: 	heavytree()
*				new_set()
*				intersect()
*				nextel()
*
*******************************************************************************/
LOCAL flag sr_worthit(ivp, bivp, used_twice)	struct iv *ivp, *bivp;
{
	register NODE *cp, *dp;
	register int i;
	SET *tempset;
	int bdefcount;	/* # of defs of the biv within the loop */
	flag result;
	flag c_incr;	/* If the basic iv has a true constant
			   increment, then the resulting
			   code coming out of add_sincr()
			   will be quicker. If it's only
			   relatively constant (i.e. invariant)
			   the resulting add is more expensive.
			*/

	cp = ivp->mc;	/* multiplicative factor */
	if (!nncon(cp))
		return(YES);
	
	dp = ivp->ac;	/* additive factor */

# ifdef FLOAT_SR
	if (ISFTP(cp->in.type) || ISFTP(dp->in.type))
		return(YES);
# endif FLOAT_SR

	i = cp->tn.lval;
	if ( i != (i & -i) )	/* Not a power of 2 */
		return(YES);

	result = NO;
	c_incr = nncon(deftab[bivp->dti].np->in.right->in.left);

	/* Count definitions of the biv within the loop */
	tempset = new_set(maxdefs);
	intersect(loopdefs, bivp->family_sp->an.defset, tempset);
	bdefcount = 0;
	i = -1;
	while ((i = nextel(i, tempset)) >= 0)
		bdefcount++;
	FREESET(tempset);
	bdefcount--;	/* A single def is the minimum expected */

	/* It's advantageous to get array base addresses into temps regardless
	   of the apparent cost.
	*/
	if ( c_incr && !bdefcount )
		switch (optype(dp->in.op))
			{
			case LTYPE:
				if (dp->in.arraybaseaddr && (cp->tn.lval != 1))
					result = YES;
				break;
			case BITYPE:
				if (dp->in.right->allo.flagfiller &
				     (AREF|ABASEADDR|ARRAYFORM))
					result = YES;
				/* fall thru */
			case UTYPE:
				if (dp->in.left->allo.flagfiller &
				     (AREF|ABASEADDR|ARRAYFORM))
					result = YES;
				break;
			}

	return( result || (ISPTR(dp->in.type)? aheavyop(dp->in.op) : heavyop(dp->in.op))
		|| (heavytree(dp)+MIN(used_twice,2)) > COST_BREAKEVEN+bdefcount);
}

/******************************************************************************
*
*	strength_reduction()
*
*	Description:		strength_reduction() implements Algorithm 10.10
*				of A, S & U to reduce the strength of
*				expressions involving induction variables.
*
*	Called by:		loop_optimization()
*
*	Input parameters:	cvector - a vector containing block #s in the
*					currentregion.
*
*	Output parameters:	none
*
*	Globals referenced:	loopdefs
*				lastdef
*				maxdefs
*				deftab
*				dfo
*				ivtab
*				topiv
*				currentregion
*
*	Globals modified:	deftab
*				(block's tree is rewritten)
*
*	External calls:		location()
*				mktemp()
*				block()
*				tfree()
*				rewrite_comops_in_block()
*				remove_obsolete_def()
*
*******************************************************************************/
LOCAL void strength_reduction(cvector)	long *cvector;
{
	register struct iv *ivp, *ivp2, *ivp3;
	register NODE *tnp;
	register long *pelem, *vector;
	NODE *asg_np;
	NODE *testnp;
	HASHU *basic_sp;
	OFFSZ tspot;
	long flags;
	int m;
	int lastdblock;
	int used_twice;
	ushort arrayrefno;

# 	define ISNEWTEMP(np) (np->in.op==OREG && np->tn.lval < -tspot)

	/* Array and structref flags must be carried through to the new
	   temp. Otherwise the register allocator gets confused.
	*/
#	define MVFLAGS(np) (np)->allo.flagfiller = flags;\
				(np)->allo.arrayrefno = arrayrefno

	tspot = tmpoff;	/* Temps beyond tspot will necessarily be made for 
			   this invocation of strength_reduction().
			*/

	/* Finish initializing selected portions of the deftab[] */
	m = max((lastdef+1), numblocks);
	vector = pelem = (long *)ckalloc(m * sizeof(long));
	lastdblock = -1;
	set_elem_vector(loopdefs, vector);
	while (*pelem >= 0)
		{
		if (deftab[*pelem].bn != lastdblock)
			{
			lastdblock = deftab[*pelem].bn;
			set_dt(dfo[lastdblock]->b.treep);
			}
		pelem++;
		}

	/* Consider each basic iv in turn */
	for (ivp = ivtab; ivp <= &ivtab[topiv]; ivp++)
		{
		if (ivp->j_sp != ivp->family_sp)
			break;	/* All basic iv's have been seen. */
		basic_sp = ivp->family_sp;

		/* Look for other iv's within the same family */
		for (ivp2 = ivp+1; ivp2 <= &ivtab[topiv]; ivp2++)
			{
			if (lastdef >= maxdefs - 3)
				goto exit;	/* No room for new defs */

			if (ivp2->family_sp != basic_sp)
				continue;
			if (ivp2->initted && ivp2->temp_sp==NULL)
				/* Already considered but not worthit */
				continue;
			
			/* Check to see if the same iv is used more
			   than once.
			*/
			if (!ivp2->initted)
				{
				used_twice = 0;
				for (ivp3=ivp2+1; ivp3 <= &ivtab[topiv]; ivp3++)
					{
					if (ivp3->family_sp != basic_sp)
						continue;
					if (same(ivp3->ac, ivp2->ac, NO, NO)
						&& same(ivp3->mc, ivp2->mc, NO, NO))
						{
						/* temp_sp==NULL && initted==YES
					   	will be used as a flag later.
						*/
						ivp3->temp_sp = NULL;
						ivp3->initted = YES;
						ivp3->sibling = ivp2;
						used_twice++;
						}
					}
	
				if ( !sr_worthit(ivp2, ivp, used_twice) )
					{
					/* The strength reduction is valid but
				   	it results in slower code than the
				   	original!
					*/
					continue;
					}
				}

			asg_np = (ivp2->dti >= 0)? deftab[ivp2->dti].np :
						ivp2->tnp;
			flags = asg_np->allo.flagfiller;
			arrayrefno = asg_np->allo.arrayrefno;

			if (!ivp2->temp_sp)
				{
				/* No temp has yet been assigned for this "j" */
				if ( (ivp2->dti>=0) && ISNEWTEMP(asg_np->in.left) )
					{
					/* Reuse this temp. It cannot have been
					   used before this point.
					*/
					tnp = NULL;
					ivp2->temp_sp = location(asg_np->in.left);
					}
				else
					{
					tnp = mktemp(0, asg_np->in.type);
					ivp2->temp_sp = location(tnp);
					}

				/* Clean up duplicate ivtab entries */
				for (ivp3 = ivp2+1; ivp3 <= &ivtab[topiv]; ivp3++)
					if (ivp3->sibling == ivp2)
						{
						/* Found up above */
						ivp3->temp_sp = ivp2->temp_sp;
						}
				}
			else /* A temp var "s" already exists for the iv */
				tnp = block(OREG, ivp2->temp_sp->a6n.offset,
						TMPREG, 0, ivp2->temp_sp->a6n.type);

			/* Steps 2 and 3 */
			if (tnp == NULL)
				{
				/* Perhaps we can reuse this temp instead of
				   creating a "temp1 = temp2" situation.
				*/
				tnp = asg_np->in.left;
				tfree(asg_np->in.right);

				testnp = deftab[ivp2->dti].stmt;
				if (!testnp)
					cerror("stmt no. not set in strength_reduction()");
				testnp = (testnp->in.op == SEMICOLONOP)?
					testnp->in.right : testnp->in.left;
				remove_obsolete_def(ivp2->dti);
				if (testnp == asg_np)
					{
					/* This assignment is not itself an
					   operand of anything but ";".
					   Therefore, throw away the assignment
					   altogether.
					*/
					asg_np->in.op = NOCODEOP;
					tnp = add_sincr(ivp2, tnp); 
					tfree(asg_np->in.left);
					}
				else
					{
					/* Move the lhs "up" the tree, thereby
					   replacing an "=" operand with a
					   leaf.
					*/
					*asg_np = *tnp;
					MVFLAGS(asg_np);
					testnp = tnp;	/* Save the ptr value */
					tnp = add_sincr(ivp2, tnp); 
					testnp->in.op = FREE;
					}
				}
			else
				{
				if (ivp2->dti >= 0)
					{
					tfree(asg_np->in.right);
					asg_np->in.right = tnp;
					MVFLAGS(asg_np->in.right);
					tnp = add_sincr(ivp2, tnp); 
					}
				else	/* a minor aiv */
					{
					tfree(asg_np);
					*asg_np = *tnp;
					MVFLAGS(asg_np);
					testnp = tnp;
					tnp = add_sincr(ivp2, tnp);
					testnp->in.op = FREE;
					}
				}

			/* Step 4 */
			if (!ivp2->initted)
				add_sinit(ivp2, tnp); 
			}
		/* "s" is not added to the ivtab[] because its use in this
		   region is complete. It will be detected as an iv by 
		   detect_indvars() anyway if the next region supersets this
		   one.
		*/
		}
	/* Get rid of useless COMOPS now so that next iteration of 
	   detect_indvars() will see well-formed subtrees involving
	   newly induced temps.
	*/
exit:
	pelem = cvector;
	while (*pelem >= 0)
		{
		if (dfo[*pelem]->b.treep)
			rewrite_comops_in_block(dfo[*pelem]->b.treep);
		pelem++;
		}
	FREEIT(vector);
}
