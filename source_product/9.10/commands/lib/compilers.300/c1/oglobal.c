/* file oglobal.c */
/* @(#) $Revision: 70.2 $ */
/* KLEENIX_ID @(#)oglobal.c     16.2 90/11/15 */

# include "c1.h"

# define MAXLPSTACK 100
# define TESTEL(a,b) if ((a)>(b)) cerror("set bounds violation") 
# define ISSIMPLE(p) (optype(p->in.op)==LTYPE \
	&& BTYPE(p->in.type)==p->in.type.base && !(p->allo.flagfiller&CXM))
# define DEFTABSZ 1000

/* Used in gosf() */
# ifndef GODEFCUTOFF
# 	define GOBLOCKCUTOFF	200
# 	define GODEFCUTOFF	1400
# 	define GOREGIONCUTOFF	200
# endif GODEFCUTOFF

/* Used in init_defs() */
# define OK_SYM(sp) (((sp)->allo.attributes & SEENININPUT) && \
	!((sp)->allo.attributes & (ISEQUIV|ISFUNC|ISSTRUCT|SCONST|ISCOMPLEX)) \
	&& ((sp)->allo.type.base != UNDEF) )
# define NOCOMDEF_CALL(np) (((np)->allo.flagfiller&(NOCOMUSES|NOCOMDEFS))==(NOCOMUSES|NOCOMDEFS))
# define NOARGDEF_CALL(np) (((np)->allo.flagfiller&(NOARGUSES|NOARGDEFS))==(NOARGUSES|NOARGDEFS))
/* Only the lhs of an assignment (not counting possible COMMON updates) is
   unambiguous.
*/
# define NOT_AMBIGUOUS(np,d,dd) ( ASSIGNMENT(np->in.op)\
	&& ((d==dd && !deftab[d].children) || (np->in.left->in.structref)) )


extern SET *restricted_rblocks;	/* loops.c */
extern flag assigned_goto_seen;

DFT *deftab;		/* array of ptrs to asgop nodes. 1 element per def */
BBLOCK **dfo;		/* array of BBLOCK ptrs malloced by inittaz */
SET **region;		/* indexed by dfo order (blocks) */
SET **blockdefs;	/* array of SET ptrs identifying defs per block */
			/* blockdefs[k] has element i = YES iff definition i
			   (from the deftab) is in block[k] (dfo ordering).
			*/
int lastdef;		/* index # of last var asgop per function */
			/* deftab[0] is not used */
int maxdefs;		/* max # of defines (including those to temps) */
int *preheader_index;	/* array of region preheader indices (using dfo
			   ordering). */
int maxnumblocks;	/* limit to numblocks expansion during code_motion */
int blockcount;		/* counter 0..numblocks-1 */
int nodecount;		/* walkf ordering of interior nodes. Used during 
			   strength reduction only. Relative position is all
			   that's important.
			*/

/* Used in gosf() */
int goblockcutoff = GOBLOCKCUTOFF;
int godefcutoff = GODEFCUTOFF;
int goregioncutoff = GOREGIONCUTOFF;

SET **predset;		/* array of predecessor sets. 1 elem per block */
SET **lastregion;	/* ptr to last active region */
SET **currentregion;	/* ptr to region being interrogated now */
SET **succset;		/* array of successor sets. 1 elem per block */
SET **dom;		/* array of dominator sets. 1 elem per block */
flag reducible;		/* t1_t2 reducible */


LOCAL int maxdftsz;		/* Max size of deftab. Expandable. */
LOCAL int oldmaxdftsz;
LOCAL long maxlpstack;		/* number of items in lpstack */
LOCAL BBLOCK **lpstack;		/* a stack */
LOCAL BBLOCK **toplpstk;	/* ptr to last active node in lpstack */
				/* dom[i] contains element j iff block j
				   dominates block i.
				*/
LOCAL SET *gent, *killt;	/* working sets for gen_and_kill() */
LOCAL NODE *stopnp;		/* for global constant propagation */
LOCAL NODE *creplacenp;		/* for global constant propagation */
LOCAL NODE *csubnp;		/* for global constant propagation */
LOCAL int simple_def_count;	/* counts defs that can be moved/duplicated */
LOCAL flag deftab_initted;	/* YES iff init_defs() has completed */


LOCAL void free_reaching_defs_data();
LOCAL void global_init();

# ifdef DEBUGGING
	LOCAL void print_regions();
# endif DEBUGGING




/* set_dfst() and its helper function search() are an implementation of 
   Algorithm 10.14 of Aho, Sethi, & Ullman. See the book for a description.
   The array dfo[] is set to contain the BBLOCK addresses of a dfst ordering.
   At this point there is a one-to-one pairing of active LBLOCK-BBLOCK
   pairs. Once the dfo ordering is complete, an attempt is made to always
   use this ordering (or its reverse) when traversing the blocks. Furthermore,
   all sets whose elements are blocks use the dfo number for element addressing
   from this point onward.
*/
/******************************************************************************
*
*	search()
*
*	Description:		see above
*
*	Called by:		search()
*				set_dfst()
*
*	Input parameters:	bp - a basic BBLOCK pointer.
*
*	Output parameters:	none
*
*	Globals referenced:	dfo
*
*	Globals modified:	dfo
*
*	External calls:		cerror()
*
*******************************************************************************/
LOCAL void search(bp) 	register BBLOCK *bp;
{
	CGU *cgup;
	int i;

# ifdef DEBUGGING
	if (!bp)
		/* cerror("impossible BBLOCK pointer in search()") */ ;
# endif DEBUGGING
	bp->bb.l.visited = YES;

	switch (bp->bb.breakop)
		{
		case CBRANCH:
			if (bp->bb.rbp && (bp->bb.rbp->bb.l.visited == NO))
				search(bp->bb.rbp);
			/* fall thru */
		case FREE:
		case LGOTO:	/* everyday GOTO */
			if (bp->bb.lbp && (bp->bb.lbp->bb.l.visited == NO))
				search(bp->bb.lbp);
			break;
		case FCOMPGOTO:
		case GOTO:	/* ASSIGNED GOTO */
			cgup = bp->cg.ll;
			for (i = 0; i < bp->cg.nlabs; i++)
				if (!cgup[i].lp->visited)
					search(cgup[i].lp->bp);
			break;
		case EXITBRANCH:
			break;
		}
	
# ifdef DEBUGGING
	if (blockcount <= 0)
		cerror("bad blockcount in search()");
# endif DEBUGGING
	bp->bb.node_position = --blockcount;
	dfo[blockcount] = bp;	/* actually a reversal of (6) in Fig. 10.47
				   of A, S & U. More useful this way.
				*/
}


/******************************************************************************
*
*	set_dfst()
*
*	Description:		see above
*
*	Called by:		setup_global_data()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	topblock
*				lastblock
*				numblocks
*				ep
*				lastep
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void set_dfst()
{
	register BBLOCK *bp;
	register int i;

	for (bp = topblock; bp <= lastblock; bp++)
		bp->bb.l.visited = NO;


	blockcount = numblocks;

	for (i = 0; i <= lastep; i++)
		{
		bp = ep[i].b.bp;
		if ( !bp->bb.l.visited )
			search(bp);
		}
	if (!dfo[0])
		{
		werror("Possible infinite loop detected.");
		for (i = 0; i < numblocks-1;i++)
			{
			dfo[i] = dfo[i+1];
			dfo[i]->bb.node_position = i;
			}
		for (bp = lastblock; bp >= topblock; bp--)
			if (bp->bb.breakop == EXITBRANCH)
				{
				dfo[numblocks-1] = bp;
				bp->bb.node_position = numblocks-1;
				break;
				}
		}
}


/* NOTE: I think this routine should be made generic so that we don't have
   more than one version of push and pop.
*/
/******************************************************************************
*
*	pushlp()
*
*	Description:		pushlp() pushes an unentered BBLOCK * onto the
*				stack. 
*
*	Called by:		regionize()
*
*	Input parameters:	bp - a ptr to a basic block header
*
*	Output parameters:	none
*
*	Globals referenced:	toplpstk
*
*	Globals modified:	toplpstk
*
*	External calls:		ckrealloc()
*
*******************************************************************************/
LOCAL void pushlp(bp)	BBLOCK *bp;
{
	BBLOCK **newlpstack;

	/* toplpstk points to top active member of the lpstack stack */
	if (++toplpstk >= &lpstack[maxlpstack])
		{
		maxlpstack += MAXLPSTACK;
		newlpstack = (BBLOCK **) ckrealloc(lpstack,
						maxlpstack * sizeof(BBLOCK *));
		toplpstk = newlpstack + (toplpstk - lpstack);
		lpstack = newlpstack;
		}
	*toplpstk = bp;
}


/******************************************************************************
*
*	regionize()
*
*	Description:		regionize() is the region maker. Regions are
*				the subgraph structures commonly referred to as
*				"loops". See Tremblay & Sorenson, pages 676-677.
*
*	Called by:		setup_global_data()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	lastregion
*				toplpstk
*				numblocks
*				dfo
*				region
*				maxregion
*				preheader_index
*
*	Globals modified:	lastregion
*				toplpstk
*				preheader_index
*
*	External calls:		cerror()
*				new_set()
*				adelelement()
*				xin()
*				free()	{FREEIT}
*
*******************************************************************************/
LOCAL void regionize()
{
	register LINK *pred, *pred2;
	register BBLOCK *bp, *bp2;
	register int head, tail;

	exitblockno = -1;
	lastregioninfo = -1;
	lastregion = region - 1;
	maxlpstack = MAXLPSTACK;
	lpstack = (BBLOCK **) ckalloc(maxlpstack * sizeof(BBLOCK *));
	toplpstk = lpstack - 1;
	for (head = numblocks-1; head >= 0; head--)
		{	/* for all edges in the flow graph */
		bp = dfo[head];
		if (bp->bb.breakop == EXITBRANCH)
			exitblockno = head;
		for (pred = bp->bb.l.pred; pred; pred = pred->next)
			{
			tail = pred->bp->bb.node_position;
			/* each predecessor relationship denotes an edge t->h */
			if (tail >= head)
				{
				/* a back edge has been found */
#ifdef COUNTING
				_n_regions++;
#endif COUNTING
				if ((++lastregion - region) >= maxregion)
					{
					SET **newregion;

					maxregion += REGIONSZ;
					newregion = (SET **)ckrealloc(region,
						     maxregion * sizeof(SET *));
					lastregion = newregion
							+ (lastregion - region);
					region = newregion;
					regioninfo =
					   (REGIONINFO *)ckrealloc(regioninfo,
						maxregion * sizeof(REGIONINFO));
					}
				/* regions are bit vectors addressed by
					dfo[] indices
				*/
				*lastregion = new_set(maxnumblocks);

				lastregioninfo++;
				memset(&(regioninfo[lastregioninfo]), 0,
					sizeof(REGIONINFO));
				regioninfo[lastregioninfo].backbranch.srcblock
					= tail;
				regioninfo[lastregioninfo].backbranch.destblock
					= head;
				needregioninfo = YES;

				/* add BBLOCK[h] to the region */
# ifdef DEBUGGING
				TESTEL(head, maxnumblocks);
# endif DEBUGGING
				adelement(head, *lastregion, *lastregion);
				if ( !xin(*lastregion, tail) )
					{
					/* add BBLOCK[t] to the region */
# ifdef DEBUGGING
					TESTEL(tail,maxnumblocks);
# endif DEBUGGING
					adelement(tail, *lastregion, *lastregion);
					pushlp(pred->bp);
					}
				for (bp2 = *toplpstk; toplpstk >= lpstack;
					bp2 = *toplpstk--)
					{
					for (pred2 = bp2->b.l.pred; pred2;
						pred2 = pred2->next)
						{
						/* new use for tail */
						tail = pred2->bp->bb.node_position;
						if ( !xin(*lastregion, tail) )
							{
# ifdef DEBUGGING
							TESTEL(tail, maxnumblocks);
# endif DEBUGGING
							adelement( tail, *lastregion, *lastregion);
							pushlp(pred2->bp);
							}
						}
					}
				}
			}
		}

	/* Now destroy the linked list version to cut down confusion
	   later.
	*/
	for (head = numblocks-1; head >= 0; head--)
		{	/* for all edges in the flow graph */
		bp = dfo[head];
		for (pred = bp->bb.l.pred; pred; pred = pred2)
			{
			pred2 = pred->next;
			free_plink(pred);
			}
		bp->bb.l.pred = (LINK *)NULL;
		}

    FREEIT(lpstack);
}


/******************************************************************************
*
*	FIX_OVERLAPPING_REGIONS()
*
*	Description:		Two regions have been found to overlap.  If
*				they have the same backbranch destination
*				block, create one region out of the two.
*
*	Called by:		check_region_overlap()
*
*	Input parameters:	r1, r2 - ** SET for the overlapping regions
*
*	Output parameters:	YES if regions were fixed, NO otherwise
*
*	Globals referenced:	region
*				lastregion
*
*	Globals modified:	region
*
*	External calls:		newset()
*				delelement()
*				adelement()
*				setassign()
*				setunion()
*				mkblock()
*				addgoto()
*
*******************************************************************************/
LOCAL flag fix_overlapping_regions(r1, r2)
	SET **r1, **r2;
{
	REGIONINFO	*region1, *region2;
	int		low_src, high_src, destblock;
	int		i;
	SET		**low_region, **high_region;
	SET 		**r;
        BBLOCK 		*bp;
        BBLOCK 		*bp1;
        BBLOCK 		*bp2;
        SET		*oldset;
      
	region1 = regioninfo + (r1-region);
	region2 = regioninfo + (r2-region);

	destblock = region1->backbranch.destblock;

	/* Only regions with a common destination block can resolve overlaps */
	if (destblock != region2->backbranch.destblock)
	  return (NO);

	oldset = new_set(maxnumblocks);

	/* Does the region with the highest numbered srcblock contain only
	 * an unconditional branch?
	 */
	if (region1->backbranch.srcblock > region2->backbranch.srcblock)
	  {
	  high_src = region1->backbranch.srcblock;
	  high_region = r1;
	  low_src = region2->backbranch.srcblock;
	  low_region = r2;
	  }
	else
	  {
	  high_src = region2->backbranch.srcblock;
	  high_region = r2;
	  low_src = region1->backbranch.srcblock;
	  low_region = r1;
	  }

	if ((dfo[high_src]->l.treep->tn.op == UNARY SEMICOLONOP) &&
	    (dfo[high_src]->l.treep->in.left->tn.op == LGOTO))
	  {
	  /* Have the low_src block jump to the high_src block */
	  bp = dfo[low_src];
	  bp1 = dfo[high_src];
          bp2 = dfo[destblock];
      
          if (bp->bb.breakop == CBRANCH)
	      {
	      if (bp->bb.lbp == bp2)
	        bp->bb.lbp = bp1;
              if (bp->bb.rbp == bp2)
	        {
                register NODE *nnp;
      
                bp->bb.rbp = bp1;
	        nnp = bp->bb.treep;
	        nnp = (nnp->in.op == SEMICOLONOP) ? nnp->in.right : nnp->in.left;
	        nnp = nnp->in.right;
	        nnp->tn.lval = bp1->l.l.val;
	        }
	      }
          else if (bp->bb.breakop == LGOTO)
	      {
	      if (bp->bb.lbp == bp2)
	        {
	        register NODE *nnp;
      
	        bp->bb.lbp = bp1;
	        nnp = bp->bb.treep;
	        nnp = (nnp->in.op == SEMICOLONOP) ? nnp->in.right : nnp->in.left;
	        nnp->bn.label = bp1->l.l.val;
	        }
	      else
#pragma BBA_IGNORE
	        cerror("GOTO next block incorrect in fix_overlapping_regions()");
	      }
	  else if (bp->bb.breakop == FREE)
	      {
	      /* change to a LGOTO */
	      if (bp->bb.lbp == bp2)
		{
		bp->bb.breakop = LGOTO;
		bp->bb.lbp = bp1;
		addgoto(bp, bp1->l.l.val);
		}
	      else
#pragma BBA_IGNORE
		cerror("FREE next block incorrect in fix_overlapping_regions()");
	      }
	  else if ((bp->bb.breakop == FCOMPGOTO) ||
		   (bp->bb.breakop == GOTO))
	      {
	      register CGU *cg;
	      int j;

	      cg = bp->cg.ll;
	      for (j = bp->cg.nlabs-1; j >= 0; j--)
		{
		if (cg[j].lp->bp == bp2)
		  {
		  cg[j].lp = &(bp1->b.l);
		  cg[j].val = bp1->l.l.val;
		  }
		} 
	      }
          else 
#pragma BBA_IGNORE
            cerror("Unexpected breakop in fix_overlapping_regions()");
      
	  /* Fix up pred and succ sets.  
	   *   - A succ of low_src is no longer destblock
	   *   - A succ of low_src is now high_src.
	   *   - An additional pred of high_src is now low_src.  
           *   - Low_src is no longer a pred of destblock.
	   */
          delelement(destblock, succset[low_src], succset[low_src]);
	  adelement(high_src, succset[low_src], succset[low_src]);
	  adelement(low_src, predset[high_src], predset[high_src]);
	  delelement(low_src, predset[destblock], predset[destblock]);

	  /* Update the region sets.
	   *   - Add all elements of low_src region to high_src region.
	   *   - Delete low_src region all together.
	   *   - Any region that contains low_src now contains all of high_src
	   *     region.
	   *   - Any region that contains high_src now contains all of high_src
	   *     region.
	   */
	   setassign(*high_region,oldset);
	   setunion(*low_region,*high_region);
	   FREESET(*low_region);
	   *low_region = NULL;/* indicates an invalid region */
	   for (r = region; r <= lastregion; r++)
	      if (*r && (*r != *high_region) &&
		 (xin(*r,low_src) || xin(*r,high_src)))
		if ((i = (setcompare(oldset,*r) == ONE_IN_TWO)) ||
		    (i == ONE_IS_TWO))
                  setunion(*high_region,*r);
	  FREESET(oldset);
	  return (YES);
	  }
	else
	  {
	  /* Create a new block and have the low_src and high_src blocks
	   * jump to it.  This new block will jump back to the destblock.
	   */
          bp = mkblock();
      
          bp1 = dfo[high_src];
          bp2 = dfo[destblock];

          bp->bb.l.val = pseudolbl;
	  bp->bb.l.bp = bp;
          bp->bb.breakop = LGOTO;
          bp->bb.lbp = bp2;
          addgoto(bp, bp2->bb.l.val);
      
          dfo[numblocks] = bp;
          bp->bb.node_position = numblocks;
      
	  /* Fix up pred and succ sets.  
	   *   - A succ of low_src is the new block.
	   *   - A succ of high_src is the new block.
	   *   - Destblock is no longer a succ of low_src.
	   *   - Destblock is no longer a succ of high_src.
           *
	   *   - An additional pred of destblock is the new block.
           *   - Low_src is no longer a pred of destblock.
           *   - High_src is no longer a pred of destblock.
           *
	   *   - An additional pred of the new block is now low_src.  
	   *   - An additional pred of the new block is now high_src.  
           *   - The succ of the new block is destblock.
	   */

          adelement(numblocks, succset[low_src], succset[low_src]);
          adelement(numblocks, succset[high_src], succset[high_src]);
          delelement(destblock, succset[low_src], succset[low_src]);
          delelement(destblock, succset[high_src], succset[high_src]);

          adelement(numblocks, predset[destblock], predset[destblock]);
          delelement(high_src, predset[destblock], predset[destblock]);
          delelement(low_src, predset[destblock], predset[destblock]);
      
          adelement(high_src, predset[numblocks], predset[numblocks]);
          adelement(low_src, predset[numblocks], predset[numblocks]);
          adelement(destblock, succset[numblocks], succset[numblocks]);
      
          /* fix hish_src's and low_src's exits to account for the new block */
          for (i=0; i<=1; i++)
            {
            if (i == 0) 
	      bp1 = dfo[high_src];
            else
	      bp1 = dfo[low_src];
      
            if (bp1->bb.breakop == CBRANCH)
	        {
	        if (bp1->bb.lbp == bp2)
	          bp1->bb.lbp = bp;
                if (bp1->bb.rbp == bp2)
	          {
                  register NODE *nnp;
        
	          bp1->bb.rbp = bp;
	          nnp = bp1->bb.treep;
	          nnp = (nnp->in.op == SEMICOLONOP) ? nnp->in.right : nnp->in.left;
	          nnp = nnp->in.right;
	          nnp->tn.lval = pseudolbl;
	          }
	        }
            else if (bp1->bb.breakop == LGOTO)
	        {
	        if (bp1->bb.lbp == bp2)
	          {
	          register NODE *nnp;
        
	          bp1->bb.lbp = bp;
	          nnp = bp1->bb.treep;
	          nnp = (nnp->in.op == SEMICOLONOP) ? nnp->in.right : nnp->in.left;
	          nnp->bn.label = pseudolbl;
	          }
	        else
#pragma BBA_IGNORE
	          cerror("GOTO next block incorrect in fix_overlapping_regions()");
	        }
	    else if (bp1->bb.breakop == FREE)
	        {
	        /* change to a LGOTO */
	        if (bp1->bb.lbp == bp2)
		  {
		  bp1->bb.breakop = LGOTO;
		  bp1->bb.lbp = bp;
		  addgoto(bp1, pseudolbl);
		  }
	        else
#pragma BBA_IGNORE
		  cerror("FREE next block incorrect in fix_overlapping_regions()");
	        }
	    else if ((bp1->bb.breakop == FCOMPGOTO) ||
		     (bp1->bb.breakop == GOTO))
	        {
	        register CGU *cg;
	        int j;
  
	        cg = bp1->cg.ll;
	        for (j = bp1->cg.nlabs-1; j >= 0; j--)
		  {
		  if (cg[j].lp->bp == bp2)
		    {
		    cg[j].lp = &(bp->b.l);
		    cg[j].val = pseudolbl;
		    }
		  } 
	        }
            else 
              cerror("Unexpected breakop in fix_overlapping_regions()");
            }
	  /* One of the regions is being eliminated.  Since we don't know
	   * which one it is change both.
	   */
	  region1->backbranch.srcblock = numblocks; 
	  region2->backbranch.srcblock = numblocks;
      
	  /* Update the region sets.
	   *   - Add all elements of low_src region to high_src region.
	   *   - Add the new block to the high_src region.
	   *   - Delete low_src region all together.
	   *   - Any region that contains low_src now contains all of high_src
	   *     region.
	   *   - Any region that contains high_src now contains all of high_src
	   *     region.
	   */
	   setassign(*high_region,oldset);
	   setunion(*low_region,*high_region);
	   adelement(numblocks,*high_region,*high_region);
	   FREESET(*low_region);
	   *low_region = NULL;/* indicates an invalid region */
	   for (r = region; r <= lastregion; r++)
	      if (*r && (*r != *high_region) &&
		 (xin(*r,low_src) || xin(*r,high_src)))
		if ((i = (setcompare(oldset,*r) == ONE_IN_TWO)) ||
		    (i == ONE_IS_TWO))
                  setunion(*high_region,*r);

	  FREESET(oldset);
          pseudolbl++;
	  if (++numblocks >= maxnumblocks)
		{
		numblocks--;
		global_disable = YES;
		}
          else lastblock++;

	  return (YES);
	  } 

} /* fix_overlapping_regions */


/******************************************************************************
*
*	check_region_overlap()
*
*	Description:		updates region table if it finds two regions
*				that intersect but one is not a subset of the
*				other. These regions do not behave well for
*				global optimization (when a preheader is 
*				introduced) or in register allocation. Such
*				regions are marked by freeing the sets and
*				the region pointers are made NULL.
*
*	Called by:		setup_global_data()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	region
*				lastregion
*
*	Globals modified:	region
*
*	External calls:		setcompare()
*
*******************************************************************************/
LOCAL void check_region_overlap()
{
	register SET **r1, **r2;

	for (r1 = region; r1 < lastregion; r1++)
	    {
	    if (*r1)
		{
		for (r2 = r1 + 1; r2 <= lastregion; r2++)
		    {
		    if (*r2)
			{
			switch(setcompare(*r1, *r2))
			    {
			    case DISJOINT:
			    case ONE_IN_TWO:
			    case TWO_IN_ONE:
				break;		/* OK */

			    case ONE_IS_TWO:	/* no duplicates, please */
				FREESET(*r2);
				*r2 = NULL;/* indicates an invalid region */
#ifdef COUNTING
				_n_overlapping_regions++;
#endif COUNTING
				break;

			    case OVERLAP:
				if (fix_overlapping_regions(r1,r2))
				  { /* This means the fix worked */
				  if (!*r1) /* *r1 was eliminated, skip */
				    r2 = lastregion;
				  break;
				  }
				/* regions which overlap cause bad things to
				   happen when preheaders are made or when
				   register allocation begins to count
				   usages.
				*/
				FREESET(*r1);
				FREESET(*r2);
				*r1 = NULL;/* indicates an invalid region */
				*r2 = NULL;/* indicates an invalid region */
#ifdef COUNTING
				_n_overlapping_regions += 2;
#endif
				goto next_r1;
			    }
			}
		    }
		}
next_r1:
	    ;
	    }
}


/******************************************************************************
*
*	ELIMINATE_BAD_REGIONS();
*
*	Description:		Updates region table if it finds a region
*				whos head does not dominate its tail.
*
*	Called by:		find_doms()        
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	region
*				lastregion
*				regioninfo
*				dom[]     
*
*	Globals modified:	region
*
*	External calls:		xin()
*
*******************************************************************************/
LOCAL void eliminate_bad_regions()
{
	SET **r;
	
	REGIONINFO *l_region;

	for (r = region; r <= lastregion; r++)
	    {
	    if (*r)
		{
		l_region = regioninfo + (r - region);
		if (!(xin(dom[l_region->backbranch.srcblock],
			  l_region->backbranch.destblock)))
		  { /* bad region */
		  FREESET(*r);
		  *r = NULL;/* indicates an invalid region */
# ifdef DEBUGGING
		  fprintf(stderr,"%s: Bad region eliminated.\n",procname);
# endif DEBUGGING
		  }
		}
	    }
}


/******************************************************************************
*
*	ELIMINATE_ASSIGNED_GOTO_REGIONS();
*
*	Description:		Removes any region which could be the
*				target of an assigned goto
*
*	Called by:		setup_global_data()        
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	region
*				lastregion
*				regioninfo
*				asgtb     
*				ficonlabsz     
*				dfo     
*
*	Globals modified:	region
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void eliminate_assigned_goto_regions()
{
	SET **r;
	
	REGIONINFO *l_region;
	CGU        *cp, *tabend;

	if (!assigned_goto_seen)
	{
	    return;
	}

        tabend = &asgtp[ficonlabsz];

	for (r = region; r <= lastregion; r++)
	{
	    if (*r)
	    {
		l_region = regioninfo + (r - region);

		for (cp = asgtp; cp < tabend; cp++)
		{
		    if (cp->val == dfo[l_region->backbranch.destblock]->l.l.val)
		    {
		        FREESET(*r);
		        *r = NULL;/* indicates an invalid region */
			break;
		    }
	        }
	    }

        }
}



/******************************************************************************
*
*	t1_t2()
*
*	Description:		t1_t2() does a T1-T2 analysis of the flowgraph
*				to determine if it's reducible. It uses the
*				algorithm in Aho, Sethi, & Ullman, page 667.
* 				This routine returned a "0" in the past for 
*				non reducible procedures.  Another 
*				transformation now allows optimization of 
*				non reducible routines.  Hense, this routine 
*				always returns 1.  This routine has always
*				done more than a T1-T2 analysis.  It computes
*				predecessor and successor sets.
*
*	Called by:		setup_global_data()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	maxnumblocks
*				dfo
*				predset
*				succset
*				cgu
*				lastep
*				ep
*				gdebug
*
*	Globals modified:	predset
*				succset
*
*	External calls:		ckalloc()
*				clralloc()
*				adelement()
*				new_set()
*				isemptyset()
*				setassign()
*				fprintf()
*				print_settab()
*				delelement()
*				nextel()
*				setunion()
*				xin()
*				free()	{FREEIT}
*				free_set_n()	{FREESET_N}
*
*******************************************************************************/
LOCAL flag t1_t2()
{
	register unsigned i, lnumblocks;
	register j;
	register SET *new;
	register LINK *pred;
	register SET **lpredset, **lsuccset;
	BBLOCK *bp;
	CGU *cgup;
	flag *deleted;		/* array of "deleted" flags, 1 per block */
	flag changing;

	lnumblocks = numblocks;
	lpredset = new_set_n(maxnumblocks, lnumblocks+1);
	lsuccset = new_set_n(maxnumblocks, lnumblocks+1);
	deleted = (flag *) clralloc((lnumblocks+1) * sizeof(flag) );

	/* for each block make a predecessor set and a successor set */
	for (i= 0; i < lnumblocks; i++)
		{
		bp = dfo[i];	/* use the dfo[] for indexing */

		new = predset[i];
		for (pred = bp->bb.l.pred; pred; pred = pred->next)
			{
# ifdef DEBUGGING
			TESTEL(pred->bp->bb.node_position, lnumblocks);
# endif DEBUGGING
			adelement(pred->bp->bb.node_position, new, new);
			}
		setassign(new, lpredset[i]);

		new = succset[i];
		switch(bp->bb.breakop)
			{
			case CBRANCH:
# ifdef DEBUGGING
				TESTEL(bp->bb.rbp->bb.node_position,lnumblocks);
# endif DEBUGGING
				adelement(bp->bb.rbp->bb.node_position, new, new);
				/* fall thru */
			case FREE:
			case LGOTO:
# ifdef DEBUGGING
				TESTEL(bp->bb.lbp->bb.node_position, lnumblocks);
# endif DEBUGGING
				adelement(bp->bb.lbp->bb.node_position, new, new);
				break;
			case FCOMPGOTO:
			case GOTO:
				cgup = bp->cg.ll;
				for (j = bp->cg.nlabs-1; j >= 0; j--)
					{
# ifdef DEBUGGING
					TESTEL(cgup[j].lp->bp->bb.node_position,
						lnumblocks);
# endif DEBUGGING
					adelement(cgup[j].lp->bp->bb.node_position,
						new, new);
					}
				break;
			case EXITBRANCH:
				break;
			}
		setassign(new, lsuccset[i]);
		}
	
	/* Tail recursion elimination can create unreachable blocks. Since 
	   these cannot interact with other blocks in the flow graph, throw
	   them out of any consideration before we start.
	*/
	for (j = lnumblocks-1; j >= 0; j--)
		if ( isemptyset(lpredset[j]) && isemptyset(lsuccset[j]) )
			deleted[j] = YES;

# ifdef DEBUGGING
	if (gdebug > 5)
		{
		fprintf(debugp, "In t1_t2(), after computing sets:\n");
		print_settab(lpredset, "lpredset");
		print_settab(lsuccset, "lsuccset");
		}
# endif	DEBUGGING

	/* Create a pseudo
	   block on top (actually create only the lpredset and lsuccset
	   for it). Fill the lpredsets and lsuccsets for this new pseudo
	   block as though it was the parent of all entry point LBLOCKS.
	   Then the t1-t2 analysis will have a common ancestor, which is
	   required for the algorithm to succeed. This pseudo ancestor
	   will not exist beyond this routine because it's not needed in
	   any future processing.

	   The pseudo block is needed even if there is only one entry point,
	   in order to correctly handle the case where the first block is
	   in a loop.
	*/
	for (j = lastep; j >= 0; j--)
		{
		i = ep[j].b.bp->bb.node_position;
		adelement(lnumblocks, lpredset[i], lpredset[i]);
		adelement(i, lsuccset[lnumblocks], lsuccset[lnumblocks]);
		}
	lnumblocks++;

# ifdef DEBUGGING
	if (gdebug > 6)
		{
		fprintf(debugp, "In t1_t2(), after combining sets:\n");
		for (i=0; i < lnumblocks; i++)
			{
			fprintf(debugp, "lpredset[%3d]=", i);
			for (j=0; j < lnumblocks; j++)
				{
				if (j % 5 == 0) fprintf(debugp, " ");
				fprintf(debugp, xin(lpredset[i], j)? "1" : "0");
				}
			fprintf(debugp, deleted[i]? "  deleted\n" : "\n");
			}
		for (i=0; i < lnumblocks; i++)
			{
			fprintf(debugp, "lsuccset[%3d]=", i);
			for (j=0; j < lnumblocks; j++)
				{
				if (j % 5 == 0) fprintf(debugp, " ");
				fprintf(debugp, xin(lsuccset[i], j)? "1" : "0");
				}
			fprintf(debugp, deleted[i]? "  deleted\n" : "\n");
			}
		}
# endif	DEBUGGING

	/* Cleanup by freeing all the sets */
	FREESET_N(lpredset);
	FREESET_N(lsuccset);
	FREEIT(deleted);

	/* This routine returned a "0" in the past for non reducible
	 * procedures.  Other transformations now allow optimization of
	 * non reducible routines.  Hense, this routine always returns 1.
	 */
	return (1);
}


/******************************************************************************
*
*	find_doms()
*
*	Description:		find_doms() builds bitvector representations of
*				dom sets. It uses the algorithm described in
*				A, S & U, pp. 670-671.
*
*	Called by:		add_block_between_blocks()
*				add_load_store_to_preheader()
*				update_flow_of_control()
*				main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	numblocks
*				maxnumblocks
*				dom
*				predset
*
*	Globals modified:	dom
*
*	External calls:		new_set()
*				addsetrange()
*				setassign()
*				intersect()
*				adelement()
*				differentsets()
*				free()
*				free_set_n()	{FREESET_N}
*				new_set_n()
*
*******************************************************************************/
void find_doms()
{
	register unsigned i, lnumblocks;
	register SET *currentset, *iset;
	flag changing;
	long *elem_vector;
	register long *pelem;

	if (dom)
		FREESET_N(dom);
	dom = new_set_n(maxnumblocks, numblocks + (lastregion - region + 1));
	lnumblocks = numblocks;
	iset = new_set(maxnumblocks);
	elem_vector = (long *) ckalloc((numblocks+1) * sizeof(long));

	adelement(0, dom[0], dom[0]);

	for (i = 1; i < lnumblocks; i++)
		addsetrange(0, lnumblocks-1, dom[i]);

	do
		{
		changing = NO;

		for (i = 1; i < lnumblocks; i++)
			{
			currentset = dom[i];
			set_elem_vector(predset[i], elem_vector);
			pelem = elem_vector;
			if (*pelem != -1)	/* if there are pred's */
				{
				setassign(dom[*(pelem++)], iset);

				while (*pelem >= 0)
					intersect(dom[*(pelem++)], iset, iset);

				adelement(i, iset, iset);
				if ( differentsets(iset, currentset) )
					{
					changing = YES;
					setassign(iset, currentset);
					}
				}

# ifdef DEBUGGING
			TESTEL(i, maxnumblocks);
# endif DEBUGGING
			}
		}
	while (changing);

	/* cleanup */
	FREEIT(elem_vector);
	FREESET(iset);
# ifdef DEBUGGING
	if (gdebug>3)
		print_settab(dom, "dom");
# endif	DEBUGGING

	/* Now that dominators are available, use this to 
	   detect and eliminate invalid regions: 
	   (regions who's head doesn't dominate its tail) */
	eliminate_bad_regions();
}


/******************************************************************************
*
*	gen_and_kill()
*
*	Description:		gen_and_kill() generates the gen[] and kill[]
*				sets for each non-terminal node in each tree.
*
*				The basic strategy is to treat each block as a
*				sequence of statements, some of which are
*				assignments (we have to account for ambiguous
*				assignments).  By traversing the tree in
*				postfix the statements are visited in order.
*				If the nodes are not LTYPE, gen and kill sets
*				are generated.  In the case of assignment
*				statements the algorithms in A,S & U,
*				pages 612-614 are used.  If the statement is
*				not assignment, then just percolate the gen and
*				kill sets upward because nothing is altered by
*				the stmt itself.
*
*	Called by:		update_flow_of_control()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	deftab
*				symtab
*
*	Globals modified:	gent
*				killt
*				Gen	for block tree head
*				Kill	for block tree head
*
*	External calls:		clearset()
*				adelement()
*				setunion()
*				delelement()
*				difference()
*				setassign()
*				xin()
*
*******************************************************************************/
LOCAL void gen_and_kill(dfp)	register DFT *dfp;
{
	register ushort dd;
	register int i;
	register SET *Gen = dfo[dfp->bn]->b.gen;
	register SET *Kill = dfo[dfp->bn]->b.kill;
	NODE *np = dfp->np;
	int sx;
	ushort d;
	flag new_gen;	/* YES iff want to compute Gen set too */

	d = dd = np->in.defnumber; 
	if (!dd)
		/* All deftab entries must have a non-zero defnumber. Otherwise
		   things will go bad in a hurry.
		*/
#pragma BBA_IGNORE
		cerror("deftab entry without a defnumber in gen_and_kill()");

	/* If the block is not part of the region being accessed (if any) nor
	   part of the new preheader (if any), then its Gen set cannot have
	   changed since the last time it was calculated. Don't bother
	   recalculating it.
	*/
	new_gen = ( !restricted_rblocks || dfp->bn == *preheader_index
			|| xin(restricted_rblocks, dfp->bn) );

	/* gen[Si] and kill[Si] computation */
	clearset(gent);
	clearset(killt);
	for (i = deftab[d].children; i >= 0; i--, dd++)
		{
		/* May have a def. of more than just
		   the lhs if it's a formal arg.
		*/
		adelement(dd, gent, gent);
		sx = deftab[dd].sindex;
		if (sx>=0 && NOT_AMBIGUOUS(np, d, dd))
			{
			/* sx < 0 ==> anonymous */
			setunion(symtab[sx]->an.defset, killt);
			delelement(dd, killt, killt);
			}
		/* else do nothing; */
		/* Like an ambiguous assignment;
		   i.e. It is an anonymous assignment;
		   it doesn't necessarily
		   kill other defs. gen[] isn't
		   updated here, either, because
		   anonymous_asgmt is set to YES
		   in lisearch1() shortly anyway.
		*/
		}

	/* Now compute gen[S] and kill[S]. The values
	   will bubble up the tree.
	*/
	if (dfo[dfp->bn]->b.lastasgnode)
		{
		if (new_gen)
			{
			difference( killt, Gen, Gen );
			setunion(gent, Gen);
			/* Gen now has gen[S] (partially
		   	   complete to this level in the tree).
			*/
			}
		difference( gent, Kill, Kill );
		setunion(killt, Kill);
		/* Kill now has kill[S] (partially
		   complete to this level in the tree).
		*/
		}
	else
		{
		if (new_gen)
			setassign(gent, Gen);
		setassign(killt, Kill);
		}
	dfo[dfp->bn]->b.lastasgnode = np;
}


/******************************************************************************
*
*	fill_defsets()
*
*	Description:		fill_defsets() fills the def[] sets for each
*				declared variable. It also corrects the deftab[]
*				array to use dfo numbering vice absolute
*				numbering for the blocks.
*
*	Called by:		global_optimize()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	deftab
*				lastdef
*				blockpool
*				symtab
*				gdebug
*				debugp
*
*	Globals modified:	symtab
*				deftab
*
*	External calls:		adelement()
*				fprintf()
*
*******************************************************************************/
LOCAL void fill_defsets()
{
	register DFT *dfp;
	register i;
	register SET *dsp;

	for (i = lastdef; i >= 1;--i)
		{
		/* Each deftab entry points to an asgop, a CALL, a CM or a
		   UCM node.
		*/
		dfp = &deftab[i];
		if (dfp->sindex >= 0)
			{
			dsp = symtab[dfp->sindex]->an.defset;
			if (!dsp)
				symtab[dfp->sindex]->an.defset = dsp = new_set(maxdefs);
			adelement(i, dsp, dsp);
			}
		}
# ifdef DEBUGGING
	if (gdebug > 1)
		{
		fprintf(debugp, "\nDeftab before new temps added: \n");
		for (i = 1 ; i <= lastdef; i++)
			{
			dfp = &deftab[i];
			fprintf(debugp,"deftab[%d]: np=0x%x", i,dfp->np);
			fprintf(debugp,", sindex=%d, children=%d, bn=%d, constrhs=%d\n",
				dfp->sindex, dfp->children, dfp->bn, dfp->constrhs);
			if ( ((i+1)%5)==0 )
				fprintf(debugp, "\n");
			}
		}
# endif DEBUGGING
}


/******************************************************************************
*
*	reaching()
*
*	Description:		reaching() computes the in[] and out[] sets for
*				each block and attaches them to the appropriate
*				BBLOCK header nodes. The algorithm is taken
*				from A, S & U, page 625.
*
*	Called by:		update_flow_of_control()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	maxdefs
*				numblocks
*				dfo
*				predset
*				lastblock
*				topblock
*
*	Globals modified:	none
*
*	External calls:		new_set()
*				setassign()
*				setunion()
*				set_elem_vector()
*				difference()
*				differentsets()
*				free		{FREEIT}
*				ckalloc()
*				freeset()	{FREESET}
*
*******************************************************************************/
LOCAL void reaching()
{
	register SET *oldout;
	register BBLOCK *bp;
	register i;
	register long *pelem;
	register flag *modified;	/* modified[i] YES iff the out set
					   for this block is changed during
					   this iteration.
					*/
	register flag changing;
	register flag sum;		/* YES iff any predecessor has changed
					   an out set.
					*/
	flag first_time;
	long *vector;

	oldout = new_set(maxdefs);
	modified = (flag *)ckalloc(numblocks);
	vector = (long *)ckalloc( numblocks * sizeof(long) );
	for (i = numblocks-1; i >= 0; i--)
		{
		bp = dfo[i];
		modified[i] = YES;	/* Prime the pump for the next loop. */
		/* NOTE: make sure these sets are freed eventually */
		if (!bp->bb.l.out)
			bp->bb.l.out = new_set(maxdefs);
		if (bp->bb.treep)
			{
			if (!bp->bb.l.in)
				bp->bb.l.in = new_set(maxdefs);
# ifdef DEBUGGING
			if (!bp->bb.kill || !bp->bb.gen)
				cerror("Missing gen or kill set in reaching()");
# endif DEBUGGING
			setassign(bp->bb.gen, bp->bb.l.out);
			}
		else
			/* If there are no statements in the block then
			   the in set must be identical to the out set! 
			*/
			{
			if (bp->bb.l.in && bp->bb.l.in != bp->bb.l.out)
				FREESET(bp->bb.l.in);
			bp->bb.l.in = bp->bb.l.out;
			}
		}

	first_time = YES;
	do
		{
		changing = NO;
		for (i = 0; i < numblocks; i++)
			{
			bp = dfo[i];
			sum = NO;
			set_elem_vector(predset[i], vector);
			pelem = vector;
			while (*pelem >= 0)
				{
				if (modified[*pelem])
					setunion(dfo[*pelem]->bb.l.out, bp->bb.l.in);
				sum |= modified[*pelem++];
				}
			if (sum && bp->bb.treep)
				{
				setassign(bp->bb.l.out, oldout);
				difference(bp->bb.kill, bp->bb.l.in,
					bp->bb.l.out);
				setunion(bp->bb.gen, bp->bb.l.out);
				if (differentsets(oldout, bp->bb.l.out))
					{
					modified[i] = YES;
					changing = YES;
					}
				else modified[i] = first_time;
				}
			else modified[i] = first_time;
			}
		first_time = NO;
		}
	while (changing);

	FREESET(oldout);
	FREEIT(vector);
	FREEIT(modified);
	for (bp = lastblock; bp >= topblock; bp--)
		/* Keep the gen sets for use in the next iteration. */
		if (bp->bb.kill)
			{
			FREESET(bp->bb.kill);
			bp->bb.kill = NULL;
			}
}


/* Global constant propagation routines */

/******************************************************************************
*
*	constwalk()
*
*	Description:		constwalk() is a tree traversal helper routine
*				for gcp(). Its purpose is
*				to replace LTYPE nodes with the appropriate ICON
*				node but stopping once the stop point is
*				reached.
*
*	Called by:		constwalk()
*				gcp()
*
*	Input parameters:	np - a NODE ptr to a candidate subexpression
*				pstopflag - a means to turn off further searches
*					at some point in the tree.
*
*	Output parameters:	none
				(the subexpression may be rewritten)
*
*	Globals referenced:	stopnp
*				csubnp
*				creplacenp
*				changing
*
*	Globals modified:	changing
*
*	External calls:		none
*
*******************************************************************************/
LOCAL void constwalk(np, pstopflag)	register NODE *np; flag *pstopflag;
{
top:	if (np == stopnp)
		*pstopflag = YES;
	if (*pstopflag)
		return;

	switch(optype(np->in.op))
		{
		case LTYPE:
			if (same(np, csubnp, YES, NO))
				{
				*np = *creplacenp;
				changing = YES;
				}
			return;
		case BITYPE:
			constwalk(np->in.left, pstopflag);
			np = np->in.right;
			goto top;
		case UTYPE:
			np = np->in.left;
			goto top;
		}
}




/******************************************************************************
*
*	gcp()
*
*	Description:		Gcp() uses the deftab
*				and in[] and out[] (reaching) information to
*				check for constant assignments that reach a
*				block.  Once such an assignment has been
*				discovered, it's a simple matter to replace all
*				refs to the lhs variable with the constant
*				value instead a la constprop() in dag.c.
*				i.e., it does global constant propagation.
*
*	Called by:		update_flow_of_control()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	maxdefs
*				numblocks
*				dfo
*				deftab
*				csubnp
*				symtab
*				blockdefs
*				stopnp
*				creplacenp
*				changing
*				gdebug
*				debugp
*				region
*
*	Globals modified:	csubnp
*				stopnp
*				creplacenp
*				changing
*				currentregion
*
*	External calls:		new_set()
*				nextel()
*				set_elem_vector()
*				intersect()
*				delelement()
*				isemptyset()
*				optim()
*				fprintf()
*				fwalk()
*				eprint()
*				fputs()
*
*******************************************************************************/
LOCAL void gcp()
{
	register int i, j;
	register long *pelem;
	register DFT *dfp;
	register BBLOCK *dp;
	register SET *tset;
	SET *varset;
	SET *sdefset;
	long *vector;
	flag stopflag;
	uchar op;


	if (currentregion && *currentregion)
		{
		/* If this is not the first time gcp() is called, then it
		   has been called because a global optimization has taken
		   place. Only the affected blocks need be checked in this
		   case.
		*/
		/* For the duration of gcp() only, add the preheader block
		   into the current region. Most gcp() opportunities will
		   be in the preheader.
		*/
		adelement(preheader_index[currentregion - region],
			*currentregion, *currentregion);
		}
	else return;	/* duopts.c has a global const prop routine of its own.*/

	tset = new_set(maxdefs);
	sdefset = new_set(maxdefs);
	vector = (long *)ckalloc((lastdef + 1) * sizeof(long));

	for (i = lastdef; i > 0; i--)
		{
		HASHU *sp;
		dfp = &deftab[i];
		if ( (!dfp->constrhs) || (dfp->sindex < 0) )
			continue;
		sp = symtab[dfp->sindex];
		if ((BTYPE(sp->a6n.type) != sp->a6n.type.base) || !OK_SYM(sp))
			continue;
		if (sp->a6n.tag!=A6N || sp->a6n.offset > 0)
			/* Only local stack items can be propagated in this
			   case.
			*/
			continue;
		adelement(i, sdefset, sdefset);
		}

	for (i = 0; i < numblocks; i++)
		{
		dp = dfo[i];
		if (!dp->b.treep)
			continue;
		if (!xin(*currentregion, i))
			continue;

		intersect(sdefset, dp->b.l.in, tset);
		set_elem_vector(tset, vector);
		for (pelem = vector; *pelem >= 0; pelem++)
			{
			dfp = &deftab[*pelem];

			/* If in the same block as the *pelem, forget it. We've
			   already done local constant propagation.
			*/
			if (dfp->bn == i)
				continue;

			csubnp = dfp->np->in.left;

			/* Next, determine that only 1 definition of this var
			   comes in.
			*/
			varset = symtab[dfp->sindex]->an.defset;
			intersect(varset, dp->b.l.in, tset); /* New use for tset*/
			delelement(*pelem, tset, tset);
			if (!isemptyset(tset))
				continue;
			
			/* The var may also have one or more definitions within
			   the block.  We should attempt constant propagation 
			   only up to the first such definition.
			*/
			intersect(varset, blockdefs[i], tset); /* New use for tset */
			stopnp = NULL;
			j = 0;
			if ((j = nextel(j, tset)) > 0)
				{
				op = deftab[j].np->in.op;
				if (op != ASSIGN && op != INCR && op != DECR)
					/* Some node that disturbs the L-R-P
					   postfix traversal order, like a CALL.
					*/
					goto nextdef;
				stopnp = deftab[j].np->in.left;
				}
			creplacenp = dfp->np->in.right;
			changing = NO;
			stopflag = NO;
			constwalk(dp->b.treep, &stopflag);
			if (changing)	/* Do some constant folding */
				{
				dp->b.treep = optim(dp->b.treep);
# 	ifdef COUNTING
				_n_gcps++;
# 	endif COUNTING
# ifdef DEBUGGING
				if (gdebug > 1)
					{
					fprintf(debugp,
						"gcp found a constant in dfo[%d]:\n",
						i);
					fprintf(debugp,"\tcsubnp = 0x%x, ",
						csubnp);
					fprintf(debugp,"creplacenp = 0x%x, ",
						creplacenp);
					fprintf(debugp,"stopnp = 0x%x\n",stopnp);
					fwalk(dp->b.treep,eprint,0);
					fputs("\n\0", debugp);
					}
# endif DEBUGGING
				}
nextdef:
			continue;
			}
		}
	FREEIT(vector);
	FREESET(sdefset);
	FREESET(tset);

	/* Now put the current region back into its original shape */
	delelement(preheader_index[currentregion - region],
		*currentregion, *currentregion);
}




/******************************************************************************
*
*	fill_blockdefs()
*
*	Description:		Fills the blockdefs sets.
*
*	Called by:		update_flow_of_control()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	maxdefs
*				numblocks
*				lastdef
*				debugp
*
*	Globals modified:	blockdefs[]
*
*	External calls:		new_set_n()
*				addsetrange()
*				fprintf()
*				printset()
*				dumpsymtab()
*
*******************************************************************************/
LOCAL void fill_blockdefs()
{
	register unsigned i, j;

	blockdefs = new_set_n(maxdefs, numblocks);
	for (i = 1; i <= lastdef; i++)
		{
		j = deftab[i].children;
		addsetrange(i, i+j, blockdefs[deftab[i].bn]);
		i += j;
		}

# ifdef DEBUGGING
	if (gdebug>3)
		{
		fprintf(debugp, "after fill_blockdefs():\n");
		for (i = 0; i <numblocks; i++)
			{
			fprintf(debugp, "\tblockdefs[%3d] = ", i);
			printset(blockdefs[i]);
			}
		fprintf(debugp, "\n");
		dumpsymtab();
		}
# endif	DEBUGGING
}


/******************************************************************************
*
*	update_flow_of_control()
*
*	Description:		Update_flow_of_control() controls those routines
*				that set up gen[], kill[], and hence in[] and
*				out[]. As nodes are altered by code motion and 
*				strength reduction of induction variables, it
*				becomes necessary to recompute this information.
*				Therefore this routine is globally visible.
*
*	Called by:		global_optimize()
*				addnode()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	numblocks
*				dfo
*				maxdefs
*				gdebug
*				debugp
*				gcp_disable
*
*	Globals modified:	dfo
*
*	External calls:		new_set()
*				clearset()
*				walkf()
*				setassign()
*				printset()
*				fprintf()
*				dumpblockpool()
*				free_set_n()	{FREESET_N}
*
*******************************************************************************/
void update_flow_of_control()
{
	register BBLOCK *bp;
	register DFT *dfp;

	if (blockdefs)
		FREESET_N(blockdefs);
	blockdefs = NULL;

	for (bp = lastblock; bp >= blockpool; bp--)
		{
		if (bp->b.treep)
			{
			/* One gen, kill, in and out set per block */
			bp->b.lastasgnode = NULL;
			if (!bp->b.gen)
				bp->b.gen = new_set(maxdefs);
			if (!bp->b.kill)
				bp->b.kill = new_set(maxdefs);
			else
				clearset(bp->b.kill);
			}
		}

	/* Topological ordering is important here. */
	/* initialization of sets and tables */
	gent = new_set(maxdefs);
	killt = new_set(maxdefs);
	for (dfp = &deftab[1]; dfp <= &deftab[lastdef]; dfp++)
		{
		gen_and_kill(dfp);
		dfp += dfp->children;
		}
	FREESET(gent);
	FREESET(killt);




# ifdef DEBUGGING
	if (gdebug > 4)
		{
		register i;
		fprintf(debugp, "after gen_and_kill():\n");
		for (i=0; i < numblocks; i++)
			{
			bp = dfo[i];
			if (bp->b.treep)
				{
				fprintf(debugp, "\ttree[%3d].gen =  ", i);
				printset(bp->b.gen);
				fprintf(debugp, "\ttree[%3d].kill = ", i);
				printset(bp->b.kill);
				}
			}
		}
# endif DEBUGGING

	fill_blockdefs();
	reaching();
# ifdef DEBUGGING
	if (gdebug > 3)
		{
		register i;
		fprintf(debugp, "after reaching():\n");
		for (i = 0; i < numblocks; i++)
			{
			bp = dfo[i];
			if (bp->b.treep)
				{
				fprintf(debugp, "\tLBLOCK[0x%x]->in =  ", bp);
				printset(bp->b.l.in);
				fprintf(debugp, "\tLBLOCK[0x%x]->out = ", bp);
				printset(bp->b.l.out);
				fprintf(debugp, "\n");
				}
			else
				fprintf(debugp, "\tEMPTY BLOCK\n\n");
			}
		}
# endif DEBUGGING
	
	if (!gcp_disable)
		gcp();
# ifdef DEBUGGING
	if (gdebug > 1)
		{
		fprintf(debugp, "after gcp():\n");
		dumpblockpool(1, 1);
		}
# endif DEBUGGING
}




/******************************************************************************
*
*	global_optimize()
*
*	Description:		Global_optimize() does all inter-block
*				optimizations. Its first task is to set up
*				all the informational sets. Then it calls the
*				various optimization drivers if appropriate.
*
*	Called by:		main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	reducible
*				gdebug
*				debugp
*				dfo
*				godefcutoff
*
*	Globals modified:	deftab_initted
*				currentregion
*				maxdftsz
*				oldmaxdftsz
*
*	External calls:		loop_optimization()
*				fill_defsets()
*				fprintf()
*				global_init()
*				fill_defsets()
*				ckalloc()
*				gosf()
*
*******************************************************************************/
void global_optimize()
{
	long *cvector;


	maxdftsz = DEFTABSZ;
	if (maxdftsz > (2*godefcutoff))
		maxdftsz = 2*godefcutoff;
	oldmaxdftsz = maxdftsz;
	deftab = (DFT *)ckalloc(maxdftsz*sizeof(DFT));
	deftab_initted = NO;
	global_init();
	if (!gosf())
		{
		free_reaching_defs_data();
		return;
		}

	defsets_on = YES;
	fill_defsets();
# ifdef COMPLEXITY
	complexitize(SAMPLING);
	complexitize(REPORTING);
# endif COMPLEXITY

	if (!dom)
		find_doms();

	cvector = (long *)ckalloc((maxnumblocks+1)*sizeof(long));
	for (currentregion = region; currentregion <= lastregion; currentregion++)
		if (*currentregion)
			{
			set_elem_vector(*currentregion, cvector);
			(void)mknewpreheader(cvector);
			}

	currentregion = NULL;
	FREEIT(cvector);
	update_flow_of_control();

	/* Now work on finding induction variables and code motion. */
	loop_optimization();

# ifdef DEBUGGING
	if (gdebug > 1)
		{
		int i;

		fprintf(debugp, "after loop_optimization():\n");
		for (i=0; i <= numblocks; i++)
			fprintf(debugp, "\tdfo[%3d] = 0x%x\n", i, dfo[i]);
		}
# endif	DEBUGGING

	free_reaching_defs_data();
# ifdef DEBUGGING
	if (gdebug)
		{
		fprintf(debugp, "after global_optimize():\n");
		print_regions();
		}
# endif	DEBUGGING
}


/******************************************************************************
*
*	free_reaching_defs_data()
*
*	Description:		Does a graceful cleanup of those global sets
*				used during the discovery of reaching
*				definitions.
*
*	Called by:		global_optimize()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	Gen
*				Kill
*				lastfilledsym
*				symtab
*				blockdefs
*				defsets_on
*
*	Globals modified:	gent
*				symtab
*				blockdefs
*				defsets_on
*
*	External calls:		free()	{FREEIT} {FREESET}
*				free_set_n()	{FREESET_N}
*
*******************************************************************************/
LOCAL void free_reaching_defs_data()
{
	register i;
	register BBLOCK *bp;

	for (i = 0; i <= lastfilledsym; ++i)
		if (symtab[i]->a6n.defset)
			{
			FREESET(symtab[i]->a6n.defset);
			symtab[i]->a6n.defset = NULL;
			}
	if (blockdefs)
		FREESET_N(blockdefs);
	blockdefs = NULL;
	defsets_on = NO;
	FREEIT(deftab);
	deftab = NULL;
	for (i = numblocks-1; i >= 0; i--)
		{
		bp = dfo[i];
		if (bp->bb.l.in && bp->bb.l.in != bp->bb.l.out) 
			FREESET(bp->bb.l.in);
		if (bp->bb.l.out) 
			FREESET(bp->bb.l.out);
		if (bp->bb.gen)
			FREESET(bp->bb.gen);
		bp->bb.gen = NULL;
		/* Kill sets have been cleaned out in reaching() */
		}
}


/******************************************************************************
*
*	free_global_data()
*
*	Description:		frees sets created for global flow analysis.
*
*	Called by:		main()
*				free_reg_data()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	predset
*				succset
*				looppreheaders
*				preheader_index
*				region
*				lastregion
*
*	Globals modified:	dom
*				predset
*				succset
*				looppreheaders
*				preheader_index
*				region
*
*	External calls:		free()	{FREEIT}
*				free_set_n()	{FREESET_N}
*
*******************************************************************************/
void free_global_data()
{
	register SET **lregion;

	/* stuff done only if the flowgraph is reducible via t1_t2(). */
	for (lregion = region; lregion <= lastregion; lregion++)
		if (*lregion) 
			FREESET(*lregion);
	if (dom)
		FREESET_N(dom);
	dom = NULL;
	FREESET_N(predset);
	FREESET_N(succset);
	FREESET(looppreheaders);
	FREEIT(preheader_index);
}




/******************************************************************************
*
*	setup_global_data()
*
*	Description:		sets up sets and global variables for flow 
*				analysis.
*
*	Called by:		main()
*				register_allocation()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	lastblock
*				blockpool
*				gdebug
*				debugp
*				dfo
*				reducible
*				defsets_on
*				preheader_index
*				region
*				lastregion
*
*	Globals modified:	numblocks
*				maxnumblocks
*				predset
*				succset
*				defsets_on
*				preheader_index
*				looppreheaders
*
*	External calls:		fprintf()
*				dumpblockpool()
*				new_set()
*				free()	{FREEIT}
*				free_set_n()	{FREESET_N}
*				clralloc()
*				fputs()
*				print_settab()
*
*******************************************************************************/
void setup_global_data()
{
	numblocks = lastblock - blockpool + 1;
	maxnumblocks = numblocks + (numblocks >> 2) + GOREGIONCUTOFF;

# ifdef DEBUGGING
	if (gdebug > 4)
		{
		fprintf(debugp, "\nEntering setup_global_data():\n");
		dumpblockpool(1,1);
		}
# endif DEBUGGING

	/* set up dfo[], the dfst ordering. We don't really bother to make a
	   tree.
	*/
	set_dfst();

	/* From this point onward block addressing within sets is done using
	   dfst ordering.
	*/

# ifdef DEBUGGING
	if (gdebug > 1)
		{
		register ii;

		fprintf(debugp, "after set_dfst():\n");
		for (ii=0; ii <= lastblock - blockpool; ii++)
			fprintf(debugp, "\tdfo[%3d] = 0x%x\n", ii, dfo[ii]);
		/* Loopxforms will change the blocks anyway, but if we really
		   need to see them now ...
		*/
		if (gdebug > 4)
			dumpblockpool(1,1);
		}
# endif	DEBUGGING

	/* Partition into regions */
	predset = new_set_n(maxnumblocks, maxnumblocks);
	succset = new_set_n(maxnumblocks, maxnumblocks);
	if (! (reducible =t1_t2()) )
		{
		/* The flowgraph is not reducible. For now, punt. Later it
		   may be useful to split nodes and retry.
		*/
		
		werror(
"Procedure not reducible\n(no global optimizations or register allocation performed in this routine).");
# ifdef DEBUGGING
		if (gdebug)
			fprintf(debugp, "The flow graph is not reducible.\n");
# endif	DEBUGGING
# ifdef COUNTING
		_n_nonreducible_procs++;
# endif COUNTING
		FREESET_N(predset);
		FREESET_N(succset);
		defsets_on = NO;
		return;
		}

	
	regionize();
	eliminate_assigned_goto_regions();
	check_region_overlap();

	preheader_index = (int *)clralloc( (lastregion-region+1)*sizeof(int) );

	/* From this point onward predecessor and successor relationships
	   are detailed in predset[] and succset[] arrays. No attempt is
	   made to keep the linked lists of predecessors accurate. Perhaps
	   the linked lists could be freed here?
	*/
# ifdef DEBUGGING
	if (gdebug)
		{
		fprintf(debugp, "after regionize():\n");
		print_regions();
		}
# endif	DEBUGGING

	looppreheaders = new_set(maxnumblocks);
}  /* setup_global_data */



/******************************************************************************
*
*	new_definition()
*
*	Description:		Creates a new deftab entry for a definition
*				and returns its index.
*
*	Called by:		add_sinit()
*				farg_defs()
*				common_defs()
*				ptrindir_defs()
*				init_defs()
*
*	Input parameters:	np - a NODE ptr for the ASSIGN, CM, CALL, or
*					whatever node causing a definition.
*				index - symtab index of the symbol being defined
*
*	Output parameters:	none
*				(return value is deftab index of new def)
*
*	Globals referenced:	lastdef
*				oldmaxdftsz
*				maxdftsz
*				deftab
*				blockcount
*				symtab
*				deftab_initted
*				godefcutoff
*
*	Globals modified:	lastdef
*				maxdftsz
*				deftab
*
*	External calls:		ckrealloc()
*				cerror()
*
*******************************************************************************/
ushort new_definition(np, index)	NODE *np; int index;
{
	register DFT *dp;
	int i;
	int size;

	++lastdef;
	if (!deftab_initted && lastdef >= godefcutoff)
		return(0);

# ifdef DEBUGGING
	{
	HASHU *sp;
	sp = (index >= 0)? symtab[index] : NULL;
	if (sp && !OK_SYM(symtab[index]))
		cerror("Improper symbol definition in new_definition()");
	}
# endif DEBUGGING

	size = 2 * godefcutoff;
	if ((lastdef >= maxdftsz) && (maxdftsz < size))
		{
		/* Fibonnaci expansion */
		i = maxdftsz;
		maxdftsz += oldmaxdftsz;
		if (maxdftsz > size)
			maxdftsz = size;
		oldmaxdftsz = i;
		deftab = (DFT *)ckrealloc(deftab, maxdftsz * sizeof(DFT));
		}
	if (deftab_initted && lastdef >= maxdefs)
#pragma BBA_IGNORE
		cerror("Def set overflow in new_definition()");
	i = lastdef;
	dp = &deftab[lastdef];
	dp->np = np;
	dp->stmt = NULL;
	dp->bn = blockcount;
	dp->sindex = index;	/* symtab index */
	dp->constrhs = ( (np->in.op == ASSIGN) && (RO(np) == ICON) );
	dp->children = 0;
	return(i);
}

/******************************************************************************
*
*	present()
*
*	Description:		present() returns YES iff symbol with
*				symtax[index] is already within the deftab
*				entries for this (chained) CALL or ASSIGNMENT. 
*
*	Called by:		common_defs()
*				farg_defs()
*
*	Input parameters:	dn - deftab index
*				index - candidate symbol symtab index
*
*	Output parameters:	none
*				(returns YES iff symbol already present)
*
*	Globals referenced:	deftab
*				lastdef
*
*	Globals modified:	none
*
*	External calls:		none
*
*******************************************************************************/
LOCAL flag present(dn, index)	ushort dn; int index;
{
	register DFT *dfp;
	register DFT *dfpend;

	if (dn && lastdef)
		for(dfp = &deftab[dn], dfpend = &deftab[lastdef]; dfp <= dfpend;
			dfp++)
			if (dfp->sindex == index)
				return (YES);
	return (NO);
}



/******************************************************************************
*
*	common_defs()
*
*	Description:		Common_defs() creates deftab entries for each
*				member of common whenever a CALL or UCALL node
*				has been hit. This is done so that the gen()
*				entries can be updated later.
*
*	Called by:		init_defs()
*
*	Input parameters:	np - a NODE ptr to the ASSIGN, CALL, CM or 
*					whatever that causes the def.
*				ss - the symtab index to the var being directly
*					def'd
*
*	Output parameters:	none
*
*	Globals referenced:	fortran
*				NO_SHARED_COMMON_PARMS (actually a macro)
*				symtab[]
*				comtab[]
*				lastfilledsym
*				lastcom
*				ptrtab
*				lastptr
*				deftab[]
*				godefcutoff
*
*	Globals modified:	deftab[]
*
*	External calls:		new_definition()
*
*******************************************************************************/
LOCAL void common_defs(np, ss)	NODE *np; int ss;
{
	register ushort first_dn;
	register i;
	register unsigned *pp;
	register CLINK *kp;
	HASHU *sp;
	ushort dn;
	flag external_ptr_seen;

	/* All deftab entries for a given np will be consecutive. The first
	   may have already been linked.
	*/

	first_dn = np->in.defnumber;
	external_ptr_seen = NO;

	if (fortran)
		{
		/* Common regions first. Any member of any common region could
		   be updated when a CALL or UCALL is invoked so we assume the
		   worst unless ASSUME_NO_SIDE_EFFECTS is specified.
		*/
		/* ss == -1 if the op is CALL or STCALL */
		if (callop(np->in.op) && NOCOMDEF_CALL(np))
			;	/* do nothing */
		else if (NO_SIDE_EFFECTS && (callop(np->in.op) || symtab[ss]->a6n.farg))
			;	/* do nothing */
		else if (NO_PARM_OVERLAPS && ss >= 0 && symtab[ss]->a6n.farg)
			;	/* do nothing */
		else for (pp = &comtab[lastcom]; pp >= comtab; pp--)
			for (kp = symtab[*pp]->cn.member; kp; kp = kp->next)
				if (OK_SYM(symtab[kp->val]) &&
				    (!first_dn || !present(first_dn, kp->val)) )
					{
					/* i.e. Not already in the list */
					if (lastdef >= godefcutoff)
						break;
					dn = new_definition(np, kp->val);
					if (!first_dn)
						first_dn = dn;
					}
		}

	else
		for (i = lastfilledsym; i >= 0; i--)
			{
			if (lastdef >= godefcutoff)
				break;
			sp = symtab[i];
			if (sp->an.isexternal)
				{
				if (!external_ptr_seen && ISPTR(sp->an.type))
					external_ptr_seen = YES;
				if (OK_SYM(sp))
					{
					dn = new_definition(np, i);
					if (!first_dn)
						first_dn = dn;
					}
				}
			}

	/* C has externally defined vars that behave much like common
	   in the presence of a callop. They are also coincidental to
	   the pointer table. Extern globals may be pointers to local vars.
	   Even if there are other files which access this function, any
	   global vars which are ptrs to local vars must be declared.
	*/

	if ( external_ptr_seen && ptrtab )
		for (pp = &ptrtab[lastptr]; pp >= ptrtab; pp--)
			if (!first_dn || !present(first_dn, *pp))
				{
				if (lastdef >= godefcutoff)
					break;
				sp = symtab[*pp];
				if (!OK_SYM(sp))
					continue;
				if (sp->a6n.tag == A6N && sp->a6n.offset==0)
					/*Only the bogus entry. Don't use it. */
					continue;
				dn = new_definition(np, *pp);
				if (!first_dn)
					first_dn = dn;
				}

	if (first_dn)
		deftab[first_dn].children = lastdef - first_dn;
	np->in.defnumber = first_dn;
}



/******************************************************************************
*
*	farg_defs()
*
*	Description:		Farg_defs() does the same thing for formal
*				arguments.  It must be assumed that a CALL or
*				UCALL invocation can potentially update any of
*				the formal arguments of this routine. Called
*				only when fortran==TRUE.
*
*	Called by:		init_defs()
*
*	Input parameters:	np - a NODE ptr pointing to an ASSIGN, CM, CALL,*					or whatever that caused the def.
*
*	Output parameters:	none
*
*	Globals referenced:	assumptions	(NO_PARM_OVERLAPS)
*				fargtab
*				lastfarg
*				deftab
*				lastdef
*
*	Globals modified:	deftab
*
*	External calls:		new_definition()
*
*******************************************************************************/
LOCAL void farg_defs(np)	NODE *np;
{
	register unsigned *up;
	register ushort first_dn;
	ushort dn;

	if (NO_PARM_OVERLAPS)
		switch(np->in.op)
			{
			case STASG:
			case CM:
			case UCM:
			case ASSIGN:
			case RETURN:
			case CAST:
				return;
			}

	if (callop(np->in.op) && NOARGDEF_CALL(np))
		return;

	first_dn = np->in.defnumber;

	for (up = &fargtab[lastfarg]; up >= fargtab; up--)
		if (OK_SYM(symtab[*up]) && (!first_dn || !present(first_dn, *up)) )
				{
				/* i.e. Not already in the list */
				dn = new_definition(np, *up);
				if (!first_dn)
					first_dn = dn;
				}
	if (first_dn)
		deftab[first_dn].children = lastdef - first_dn;
	np->in.defnumber = first_dn;
}


/******************************************************************************
*
*	ptrindir_defs()
*
*	Description:		Assignments to subexpressions topped by a UNARY
*				MUL with the isptrindir flag set indicate that
*				the assignment is actually thru the ptr to any
*				one of the candidates in the ptrtab and,
*				therefore, not wholly anonymous. Ptrindir_defs()
*				adds all members of the ptrtab to the deftab
*				as children.
*
*	Called by:		init_defs()
*
*	Input parameters:	np - a NODE ptr to the assignment node.
*
*	Output parameters:	none
*
*	Globals referenced:	ptrtab
*				lastptr
*				deftab
*
*	Globals modified:	deftab
*
*	External calls:		present()
*
*******************************************************************************/
LOCAL void ptrindir_defs(np)	register NODE *np;
{
	register unsigned *pp;
	register ushort first_dn;
	HASHU *sp;
	ushort dn;

	first_dn = np->in.defnumber;

	if (lastptr==0)
		{
		sp = symtab[ptrtab[0]];
		if (sp->a6n.tag == A6N && sp->a6n.offset == 0)
			goto end;
		}
	for (pp = &ptrtab[lastptr]; pp >= ptrtab; pp--)
		{
		if (!OK_SYM(symtab[*pp]))
			continue;
		if (lastdef >= godefcutoff)
			break;
		if ( !first_dn || !present(first_dn, *pp) )
			{
			dn = new_definition(np, *pp);
			if (!first_dn)
				first_dn = dn;
			}
		}

end:
	if (first_dn)
		deftab[first_dn].children = lastdef - first_dn;
	np->in.defnumber = first_dn;
}


/******************************************************************************
*
*	defining_aparam()
*
*	Description:		Returns YES iff the anonymous actual parameter
*				is a possible definition. It cannot be a 
*				definition if it is an expression or if it's a
*				simple constant.
*
*	Called by:		init_defs()
*
*	Input parameters:	np - a NODE ptr to the node below the (U)CM.
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
LOCAL flag defining_aparam(np)	NODE *np;
{
	flag defining = YES;

	switch(np->in.op)
		{
		case FCON:
		case NAME:
		case STRING:
			defining = NO;
			break;
		case ICON:
			if (!ISPTR(np->in.type))
				/* The actual param is a simple constant */
				defining = NO;
			break;
		case UNARY MUL:
			break;
		default:
			if (!(np->allo.flagfiller &
				(ARRAYELEM|ABASEADDR|LHSIREF|ISPTRINDIR)))
				/* The actual param is an expression. No
				   definition is possible. 
				*/
				defining = NO;
			break;
		}
	return(defining);
}


/******************************************************************************
*
*	ha_defs()
*
*	Description:		Adds definitions to deftab due to hidden
*				arguments.
*
*	Called by:		init_defs()
*
*	Input parameters:	np - a NODE ptr to the CALL node.
*
*	Output parameters:	none
*
*	Globals referenced:	lastdef
*				godefcutoff
*
*	Globals modified:	deftab
*
*	External calls:		present()
*				new_definition()
*
*******************************************************************************/
LOCAL void ha_defs(np)	register NODE *np;
{
	register ushort first_dn;
	ushort *array, j;
	int i;
	HASHU *sp;
	ushort dn;

	first_dn = np->in.defnumber;
	array = np->in.hiddenvars->var_index;

	for (i = np->in.hiddenvars->nitems; i > 0; i--)
		{
		j = *array++;
		sp = symtab[j];
		if ( !OK_SYM(sp) )
			continue;
		if (lastdef >= godefcutoff)
			break;
		if (!first_dn || !present(first_dn, j))
			{
			dn = new_definition(np, j);
			if (!first_dn)
				first_dn = dn;
			}
		}

	if (first_dn)
		deftab[first_dn].children = lastdef - first_dn;
	np->in.defnumber = first_dn;
}


/******************************************************************************
*
*	init_defs()
*
*	Description:		init_defs() initializes the deftab[] array
*				element for each node for computing the gen and
*				kill sets for each node in the tree(s).
*				Corresponding counters are updated as well.
*
*	Called by:		global_init()
*				mktempassign()
*
*	Input parameters:	np - a NODE ptr to the top of a subexpression.
*
*	Output parameters:	none
*
*	Globals referenced:	nodecount
*				symtab
*
*	Globals modified:	none
*
*	External calls:		locate()
*				defining_aparam()
*				new_definition()
*				common_defs()
*				farg_defs()
*				ptrindir_defs()
*				ha_defs()
*
*******************************************************************************/
void init_defs (np) register NODE *np;
{

	HASHU *sp;
	int ss;

	if ( optype(np->in.op) != LTYPE )
		{
		np->in.index = nodecount++;	/* for strength reduction */

		switch (np->in.op)
			{
			default:
				return;
# if 0	/* Never put structure assignments into the deftab */
			case STASG:
				f_find_struct = YES;
				ss = locate(np->in.left, &sp) ?
					sp->an.symtabindex : -1;
				f_find_struct = NO;
				np->in.defnumber = new_definition(np, ss);
				break;
# endif 0
			case CM:
				if (NO_SIDE_EFFECTS)
					return;
				ss = locate( (RO(np) == UNARY MUL
						&& np->in.right->in.structref)?
						np->in.right->in.left :
						np->in.right, &sp )?
							sp->an.symtabindex : -1;
				if (ss < 0)
					if (defining_aparam(np->in.right))
						np->in.defnumber =
							new_definition(np, ss);
					else return;
				else
					{
					if (!fortran && RO(np) == OREG &&
						!sp->a6n.farg)
						/* Call by value */
						return;
					if (OK_SYM(sp))
						np->in.defnumber = new_definition(np, ss);
					}
				break;
			case UCM:
				if (NO_SIDE_EFFECTS)
					return;
				ss = locate( (LO(np) == UNARY MUL
						&& np->in.left->in.structref)?
						np->in.left->in.left :
						np->in.left, &sp )?
							sp->an.symtabindex : -1;
				if (ss < 0)
					if (defining_aparam(np->in.left))
						np->in.defnumber =
							new_definition(np, ss);
					else return;
				else
					{
					if (!fortran && LO(np) == OREG && !sp->a6n.farg)
						/* Call by value */
						return;
					if (OK_SYM(sp))
						np->in.defnumber = new_definition(np, ss);
					}
				break;
			case INCR:
			case DECR:
			/* all the asgops except +=, etc. */
			case ASSIGN:
			case RETURN:
			case CAST:
				ss = locate( (LO(np) == UNARY MUL
						&& np->in.left->in.structref)?
						np->in.left->in.left :
						np->in.left, &sp )?
							sp->an.symtabindex : -1;
				if (ss < 0 || OK_SYM(sp))
					{
					np->in.defnumber = new_definition(np, ss);
					if (ISSIMPLE(np->in.left))
						simple_def_count++;
					}
				break;
			case STCALL:
			case CALL:
				if (np->in.safe_call)
					return;
				if (NO_SIDE_EFFECTS)
					return;
				ss = locate(np->in.right, &sp)?
					sp->an.symtabindex : -1;
				if (RO(np) != CM && RO(np) != UCM
					&& (ss < 0 || OK_SYM(sp))
					&& (fortran || RO(np) != OREG
					  || sp->a6n.farg)) 
					/* Define the first parameter */
					np->in.defnumber = new_definition(np, ss);
				common_defs(np, -1);
				if (fortran)
					farg_defs(np);
				if (np->in.hiddenvars)
					ha_defs(np);
				return;
			case UNARY STCALL:
			case UNARY CALL:
				if (NO_SIDE_EFFECTS)
					return;
				ss = (locate(np->in.left, &sp))?
					sp->an.symtabindex : -1;
				/* No parameters to define */
				common_defs(np, ss);
				if (fortran)
					farg_defs(np);
				if (np->in.hiddenvars)
					ha_defs(np);
				return;
			}

		if (ss>=0)	
			{
			/* An assign to an farg could define a
			   common element or another farg.
			*/
			if (symtab[ss]->a6n.farg)
				{
				common_defs(np, ss);
				if (fortran) farg_defs(np);
				}
			/* An assign to a common element could
			   define a farg.
			*/
			else if (symtab[ss]->a6n.common)
				farg_defs(np);
			}
		else if (np->in.left->in.isptrindir)
			ptrindir_defs(np);
		return;
		}
}


/******************************************************************************
*
*	global_init()
*
*	Description:		global_init() does a subset of what dagem()
*				does but only for the purposes of 
*				global_optimization().
*
*	Called by:		global_optimize()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	nodecount
*				blockcount
*				blockpool
*				lastblock
*				maxdefs
*				lastdef
*				lastn
*				numblocks
*				debugp
*				godefcutoff
*
*	Globals modified:	nodecount
*				blockcount
*				maxdefs
*				deftab_initted
*
*	External calls:		fprintf()
*				walkf()
*				init_defs()
*				rewrite_comops_in_block()
*
*******************************************************************************/
LOCAL void global_init()
{
	register NODE *np;
	int i;

# ifdef DEBUGGING
	if (gdebug) fprintf(debugp, "global_init() called.\n");
# endif	DEBUGGING

	nodecount = 0;
	simple_def_count = 0;

	for (blockcount = 0; (blockcount<numblocks) && (lastdef<=godefcutoff);
		blockcount++)
		{
		if (np = dfo[blockcount]->bb.treep)
			{
			rewrite_comops_in_block(np);
			walkf(np, init_defs);
			}
		}

	/* Set maxdefs big enough to allow for some growth due to temp
	   assignments.
	*/
	i = lastdef + simple_def_count*3  + 20;
	maxdefs = max(i, lastn-numblocks);
	deftab_initted = YES;
}

/******************************************************************************
*
*	gosf()
*
*	Description:		Gosf() is a global optimization speed function.
*				global optimization is disabled when the source
*				function reaches a specified level of complexity
*				that would cause c1 to be unreasonably slow.
*
*	Called by:		main()
*
*	Input parameters:	none
*
*	Output parameters:	none
*
*	Globals referenced:	lastregion
*				region
*				lastdef
*				goblockcutoff
*				godefcutoff
*				goregioncutoff
*				verbose
*
*	Globals modified:	global_disable
*
*	External calls:		none
*
*******************************************************************************/
flag gosf()
{
	if ( (lastregion < region) || (numblocks > goblockcutoff)
		|| (lastdef >= godefcutoff) || ((lastregion-region+1)>goregioncutoff) )
		{
		global_disable = YES;
		if (verbose && !(lastregion < region))
			werror("Exceeded default complexity level for\n loop optimizations. Use -Wg,-All to override.");
		return(NO);	/* No regions */
		}
	return(YES);
}

# ifdef COMPLEXITY
void complexitize(mode)	char mode;
{
	static int maxnumblocks;	/* high water mark of numblocks before
						any preheaders made.
					*/
	static int maxdefs;		/* high water mark of deftab entries
						before any new defs added.
					*/
	static int maxnumregions;	/* high water mark of lastregion */
	static int maxregionlevel;	/* high water mark of region nesting
						level.
					*/
	int i;
	int j;

	switch(mode)
		{
		case INITIALIZATION:
			maxnumregions = 0;
			maxnumblocks = 0;
			maxdefs = 0;
			maxregionlevel = 0;
			return;

		case SAMPLING:
			{
			register SET **rx;
			SET **r;
			i = lastregion - region + 1;
			for (r = region; r <= lastregion; r++)
				if (!*r)
					i--;
			maxnumregions = max(i, maxnumregions);
			for (r = region; r <= lastregion-1; r++)
				if (*r)
					{
					i = 0;
					for (rx = r+1; rx <= lastregion; rx++)
						if (*rx)
							switch(setcompare(*r, *rx))
							{
							case DISJOINT:
								continue;
							case ONE_IN_TWO:
								i++;
								continue;
							case ONE_IS_TWO:
							case TWO_IN_ONE:
							case OVERLAP:
								cerror("impossible regions in complexitize()");
							}
						else continue;
					maxregionlevel = max(i, maxregionlevel);
					}
			}
			{
			register NODE *treep;
			i = numblocks;
			for (j = i; j > 0; j--)
				if ( !(treep = dfo[j-1]->bb.treep) 
					|| (treep->in.left->in.op == NOCODEOP) )
					i--;
			maxnumblocks = max(i, maxnumblocks);
			}
			maxdefs = max(lastdef, maxdefs);
			return;
		
		case REPORTING:
			if (print_counts)
				{
				fprintf(countp, "Proc is %s\n", procname);
    				fprintf(countp, "maxnumblocks = %d\n", maxnumblocks);
    				fprintf(countp, "maxdefs = %d\n", maxdefs);
    				fprintf(countp, "maxnumregions = %d\n", maxnumregions);
    				fprintf(countp, "maxregionlevel = %d\n\n", maxregionlevel);
				}
			return;
		}
}
# endif COMPLEXITY

# ifdef DEBUGGING
LOCAL void print_regions()
{
	register SET **lregion;

	for (lregion = region; lregion <= lastregion; lregion++)
		{
		fprintf(debugp, "\tregion[%3d]=", lregion - region);
		if (*lregion)
			printset(*lregion);
		else fputs("NULL (overlapping region)\n\0", debugp);
		}
}
# endif DEBUGGING
