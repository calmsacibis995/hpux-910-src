/* file regweb.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)regweb.c	16.1 90/11/05 */


/*****************************************************************************
 *
 *  File regweb.c contains routines for creating web and loop descriptors
 *  from the def-use info collected in the first pass over the statement
 *  trees.
 *  Also, utility routines for allocating and freeing WEB, LOADST, DUBLOCK,
 *  and PLINK structures are in this file.
 *
 *****************************************************************************
 */


#include "c1.h"

LOCAL long *cgitem;
LOCAL long ncgitems;
LOCAL long *cwsair_elems;
LOCAL SET *mkw_temp_s;
LOCAL SET *mkw_temp_t;
LOCAL SET *fwr_defblocks;
LOCAL SET *fgl_t;
LOCAL SET *fgl_badloops;
LOCAL SET *fgl_loopsseen;
LOCAL SET *flr_defset;
LOCAL SET *flr_reachset;
LOCAL SET *flr_t;
LOCAL SET *flr_blockset;
LOCAL SET *flr_reachlive;
LOCAL SET *flr_leftovers;
LOCAL SET *defsinfpa881loop;
LOCAL SET *fpa881blocktset;
LOCAL SET *fpa881deftset;
LOCAL flag *loop_entry_store_needed;
LOCAL flag *loop_exit_load_needed;
LOCAL SET *fpa881livedefblockset;

/*  Storage allocation globals */
# ifdef DEBUGGING
char** first_dublock_bank;
char** curr_dublock_bank;
char* next_dublock;
char* last_dublock;
char* free_dublock_list;
long  max_dublock;
long  size_dublock;

char** first_loadst_bank;
char** curr_loadst_bank;
char* next_loadst;
char* last_loadst;
char* free_loadst_list;
long  max_loadst;
long  size_loadst;

char** first_plink_bank;
char** curr_plink_bank;
char* next_plink;
char* last_plink;
char* free_plink_list;
long  max_plink;
long  size_plink;

char** first_web_bank;
char** curr_web_bank;
char* next_web;
char* last_web;
char* free_web_list;
long  max_web;
long  size_web;

# else

LOCAL char** first_dublock_bank;
LOCAL char** curr_dublock_bank;
LOCAL char* next_dublock;
LOCAL char* last_dublock;
LOCAL char* free_dublock_list;
LOCAL long  max_dublock;
LOCAL long  size_dublock;

LOCAL char** first_loadst_bank;
LOCAL char** curr_loadst_bank;
LOCAL char* next_loadst;
LOCAL char* last_loadst;
LOCAL char* free_loadst_list;
LOCAL long  max_loadst;
LOCAL long  size_loadst;

LOCAL char** first_plink_bank;
LOCAL char** curr_plink_bank;
LOCAL char* next_plink;
LOCAL char* last_plink;
LOCAL char* free_plink_list;
LOCAL long  max_plink;
LOCAL long  size_plink;

LOCAL char** first_web_bank;
LOCAL char** curr_web_bank;
LOCAL char* next_web;
LOCAL char* last_web;
LOCAL char* free_web_list;
LOCAL long  max_web;
LOCAL long  size_web;

# endif DEBUGGING

/*  Procedures defined in this file: */

LOCAL void add_end_of_block_load();
LOCAL void add_load_store_to_web();
  DUBLOCK* alloc_dublock();
  LOADST*  alloc_loadst();
  PLINK*   alloc_plink();
  WEB*     alloc_web();
LOCAL void collapse_webs();
LOCAL void compute_loop_savings();
LOCAL void compute_use_only_web_if_regions();
LOCAL void compute_web_load_store_pts();
LOCAL void compute_web_savings_and_if_regions();
LOCAL void def_in_fpa881loop();
      void discard_web();
LOCAL void do_loop_allocation();
      void file_init_storage();
LOCAL void find_good_loops();
LOCAL void find_loop_range();
LOCAL long find_web_range();
LOCAL void fix_memory_def_use();
      void free_dublock();
      void free_loadst();
      void free_plink();
      void free_web();
LOCAL void generate_fpa881loop_loads_and_stores();
      void init_du_storage();
LOCAL void init_reg_classes();
LOCAL void insert_loop_in_looplist();
LOCAL void make_webs();
      void make_webs_and_loops();
LOCAL void merge_webs();
      void proc_init_storage();
LOCAL void prune_looplist();
LOCAL void regclass_list_insert();
LOCAL void setup_for_fpa881loops();
LOCAL void use_in_fpa881loop();

/*****************************************************************************
 *
 *  ADD_END_OF_BLOCK_LOAD()
 *
 *  Description:	Add loads after an end-of-block reference.
 *
 *  Called by:		compute_web_load_store_pts()
 *
 *  Input Parameters:	du -- DUblock we're doing this for
 *			wb -- web containing the DUblock
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	dfo[]
 *			inlive[]
 *			stmttab[]
 *
 *  Globals Modified:	ncgitems
 *			cgitem
 *
 *  External Calls:	alloc_loadst()
 *			cerror()
 *			xin()
 *
 *****************************************************************************
 */
LOCAL void add_end_of_block_load(du, wb)
register DUBLOCK *du;
register WEB *wb;
{
    BBLOCK *bp;
    register LOADST *lp;
    register CGU *cg;
    register long i;
    register long j;
    register long stmt2;

    bp = dfo[stmttab[du->stmtno].block];

    switch (bp->bb.breakop)
	{
	case LGOTO:	/* everyday GOTO */
	default:
#pragma BBA_IGNORE
            cerror("impossible block breakop in add_end_of_block_load()");
	    break;

	case EXITBRANCH:
	case FREE:
	    /* create load just after this statement */
	    lp = alloc_loadst();
	    lp->isload = YES;
	    lp->blockno = bp->bb.node_position;
	    lp->stmt1 = du->stmtno;
	    lp->stmt2 = 0;
	    lp->next = wb->load_store;
	    wb->load_store = lp;
	    break;

	case CBRANCH:
	    if (xin(inlive[bp->bb.lbp->bb.node_position], wb->var->an.varno))
		{
		/* create load between this block and each next block */
		lp = alloc_loadst();
		lp->isload = YES;
		lp->blockno = 0;
		lp->stmt1 = du->stmtno;
		lp->stmt2 = bp->bb.lbp->bb.firststmt;
		lp->next = wb->load_store;
		wb->load_store = lp;
		}

	    if ((bp->bb.lbp != bp->bb.rbp)
	     && xin(inlive[bp->bb.rbp->bb.node_position], wb->var->an.varno))
	        {
		lp = alloc_loadst();
		lp->isload = YES;
		lp->blockno = 0;
		lp->stmt1 = du->stmtno;
		lp->stmt2 = bp->bb.rbp->bb.firststmt;
		lp->next = wb->load_store;
		wb->load_store = lp;
		}
	    break;

	case FCOMPGOTO:
	case GOTO:	/* ASSIGNED GOTO */
	    /* add only one load per unique block */
	    cg = bp->cg.ll;
	    ncgitems = 0;
	    for (i = bp->cg.nlabs - 1; i >= 0; --i)
		{

		/* have we done this block already ? */
		stmt2 = cg[i].lp->bp->bb.firststmt;
		for (j = 0; j < ncgitems; ++j)
		    {
		    if (cgitem[j] == stmt2)
			goto next_cg_label;	/* yes*/
		    }

		/* new block -- add load */
	        if (xin(inlive[cg[i].lp->bp->bb.node_position],
		        wb->var->an.varno))
		    {
		    lp = alloc_loadst();
		    lp->isload = YES;
		    lp->blockno = 0;
		    lp->stmt1 = du->stmtno;
		    lp->stmt2 = stmt2;
		    lp->next = wb->load_store;
		    wb->load_store = lp;
		    }
		cgitem[ncgitems++] = stmt2;
next_cg_label:
		continue;
		}
	    break;
	}
}  /* add_end_of_block_load */

/*****************************************************************************
 *
 *  ADD_LOAD_STORE_TO_WEB()
 *
 *  Description:	Add a load-store block to a web.
 *
 *  Called by:		compute_web_load_store_pts()
 *
 *  Input Parameters:	isload -- YES if load, NO if store
 *			stmt1  -- statement preceding load
 *			stmt2  -- statement following load
 *			wb     -- web descriptor
 *
 *  Output Parameters:	wb
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_loadst()
 *
 *****************************************************************************
 */

LOCAL void add_load_store_to_web(isload, blockno, stmt1, stmt2, regionno, wb)
ushort isload;
ushort blockno;
long stmt1;
long stmt2;
ushort regionno;
WEB *wb;
{
    register LOADST *lp;

    lp = alloc_loadst();
    lp->isload = isload;
    lp->blockno = blockno;
    lp->stmt1 = stmt1;
    lp->stmt2 = stmt2;
    lp->regionno = regionno;
    lp->next = wb->load_store;
    wb->load_store = lp;
}  /* add_load_store_to_web */

/*****************************************************************************
 *
 *  ALLOC_DUBLOCK()
 *
 *  Description:	Allocate and return a DUBLOCK structure
 *
 *  Called by:		add_def_block() -- in regdefuse.c
 *			add_use_block() -- in regdefuse.c
 *
 *  Input Parameters:	None
 *
 *  Output Parameters:	Returns a pointer to the allocated DUBLOCK structure
 *
 *  Globals Referenced:	curr_dublock_bank
 *			free_dublock_list
 *			last_dublock
 *			max_dublock
 *			next_dublock
 *			size_dublock
 *
 *  Globals Modified:	curr_dublock_bank
 *			free_dublock_list
 *			last_dublock
 *			next_dublock
 *
 *  External Calls:	ckalloc()
 *
 *****************************************************************************
 */

DUBLOCK *alloc_dublock()
{
    char *ret;		/* allocated block */

    /* Order of allocation:
     *     1. remaining items in current bank
     *     2. items in following bank
     *     3. items on free list
     *     4. malloc a new bank
     */

    if (next_dublock < last_dublock)
	{
	ret = next_dublock;
	next_dublock += size_dublock;
	}
    else if (*curr_dublock_bank != NULL)   /* there is a next bank */
	{
	curr_dublock_bank = *((char ***)curr_dublock_bank);
	next_dublock = ((char *)curr_dublock_bank) + 4;
	last_dublock = next_dublock + (max_dublock * size_dublock);
	ret = next_dublock;
	next_dublock += size_dublock;
	}
    else if (free_dublock_list != NULL)		/* there are free blocks */
	{
	ret = free_dublock_list;
	free_dublock_list = *((char **)free_dublock_list);
	}
    else					/* nothing available -- malloc*/
	{
	*curr_dublock_bank = (char *)ckalloc(4 + (max_dublock * size_dublock));
	curr_dublock_bank = *((char ***)curr_dublock_bank);
	*curr_dublock_bank = NULL;		/* no following bank */
	next_dublock = ((char *)curr_dublock_bank) + 4;
	last_dublock = next_dublock + (max_dublock * size_dublock);
	ret = next_dublock;
	next_dublock += size_dublock;
	}

    return((DUBLOCK *) ret);
}  /* alloc_dublock */

/*****************************************************************************
 *
 *  ALLOC_LOADST()
 *
 *  Description:	Allocate and return a LOADST structure
 *
 *  Called by:		add_end_of_block_load()
 *			add_load_store_to_web()
 *			find_loop_range()
 *
 *  Input Parameters:	None
 *
 *  Output Parameters:	Returns a pointer to the allocated LOADST structure
 *
 *  Globals Referenced:	curr_loadst_bank
 *			free_loadst_list
 *			last_loadst
 *			max_loadst
 *			next_loadst
 *			size_loadst
 *
 *  Globals Modified:	curr_loadst_bank
 *			free_loadst_list
 *			last_loadst
 *			next_loadst
 *
 *  External Calls:	ckalloc()
 *
 *****************************************************************************
 */

LOADST *alloc_loadst()
{
    char *ret;		/* allocated block */

    /* Order of allocation:
     *     1. remaining items in current bank
     *     2. items in following bank
     *     3. items on free list
     *     4. malloc a new bank
     */

    if (next_loadst < last_loadst)
	{
	ret = next_loadst;
	next_loadst += size_loadst;
	}
    else if (*curr_loadst_bank != NULL)	/* there is a next bank */
	{
	curr_loadst_bank = *((char ***)curr_loadst_bank);
	next_loadst = ((char *)curr_loadst_bank) + 4;
	last_loadst = next_loadst + (max_loadst * size_loadst);
	ret = next_loadst;
	next_loadst += size_loadst;
	}
    else if (free_loadst_list != NULL)		/* there are free blocks */
	{
	ret = free_loadst_list;
	free_loadst_list = *((char **)free_loadst_list);
	}
    else					/* nothing available -- malloc*/
	{
	*curr_loadst_bank = (char *) ckalloc( 4 + (max_loadst * size_loadst));
	curr_loadst_bank = *((char ***)curr_loadst_bank);
	*curr_loadst_bank = NULL;		/* no following bank */
	next_loadst = ((char *)curr_loadst_bank) + 4;
	last_loadst = next_loadst + (max_loadst * size_loadst);
	ret = next_loadst;
	next_loadst += size_loadst;
	}

    return((LOADST *) ret);
}  /* alloc_loadst */

/*****************************************************************************
 *
 *  ALLOC_PLINK()
 *
 *  Description:	Allocate and return a PLINK structure
 *
 *  Called by:
 *
 *  Input Parameters:	None
 *
 *  Output Parameters:	Returns a pointer to the allocated PLINK structure
 *
 *  Globals Referenced:	curr_plink_bank
 *			free_plink_list
 *			last_plink
 *			max_plink
 *			next_plink
 *			size_plink
 *
 *  Globals Modified:	curr_plink_bank
 *			free_plink_list
 *			last_plink
 *			next_plink
 *
 *  External Calls:	ckalloc()
 *
 *****************************************************************************
 */

PLINK *alloc_plink()
{
    char *ret;		/* allocated block */

    /* Order of allocation:
     *     1. remaining items in current bank
     *     2. items in following bank
     *     3. items on free list
     *     4. malloc a new bank
     */

    if (next_plink < last_plink)
	{
	ret = next_plink;
	next_plink += size_plink;
	}
    else if (*curr_plink_bank != NULL)    /* there is a next bank */
	{
	curr_plink_bank = *((char ***)curr_plink_bank);
	next_plink = ((char *)curr_plink_bank) + 4;
	last_plink = next_plink + (max_plink * size_plink);
	ret = next_plink;
	next_plink += size_plink;
	}
    else if (free_plink_list != NULL)		/* there are free blocks */
	{
	ret = free_plink_list;
	free_plink_list = *((char **)free_plink_list);
	}
    else					/* nothing available -- malloc*/
	{
	*curr_plink_bank = (char *) ckalloc( 4 + (max_plink * size_plink));
	curr_plink_bank = *((char ***)curr_plink_bank);
	*curr_plink_bank = NULL;		/* no following bank */
	next_plink = ((char *)curr_plink_bank) + 4;
	last_plink = next_plink + (max_plink * size_plink);
	ret = next_plink;
	next_plink += size_plink;
	}

    return((PLINK *) ret);
}  /* alloc_plink */

/*****************************************************************************
 *
 *  ALLOC_WEB()
 *
 *  Description:	Allocate and return a WEB structure -- zeroed out
 *
 *  Called by:		do_loop_allocation()
 *			make_webs()
 *
 *  Input Parameters:	None
 *
 *  Output Parameters:	Returns a pointer to the allocated WEB structure
 *
 *  Globals Referenced:	curr_web_bank
 *			free_web_list
 *			last_web
 *			max_web
 *			next_web
 *			size_web
 *
 *  Globals Modified:	curr_web_bank
 *			free_web_list
 *			last_web
 *			next_web
 *
 *  External Calls:	ckalloc()
 *			memset()
 *
 *****************************************************************************
 */

WEB *alloc_web()
{
    char *ret;		/* allocated block */

    /* Order of allocation:
     *     1. remaining items in current bank
     *     2. items in following bank
     *     3. items on free list
     *     4. malloc a new bank
     */

    if (next_web < last_web)
	{
	ret = next_web;
	next_web += size_web;
	}
    else if (*curr_web_bank != NULL)	/* there is a next bank */
	{
	curr_web_bank = *((char ***)curr_web_bank);
	next_web = ((char *)curr_web_bank) + 4;
	last_web = next_web + (max_web * size_web);
	ret = next_web;
	next_web += size_web;
	}
    else if (free_web_list != NULL)		/* there are free blocks */
	{
	ret = free_web_list;
	free_web_list = *((char **)free_web_list);
	}
    else					/* nothing available -- malloc*/
	{
	*curr_web_bank = (char *) ckalloc( 4 + (max_web * size_web));
	curr_web_bank = *((char ***)curr_web_bank);
	*curr_web_bank = NULL;		/* no following bank */
	next_web = ((char *)curr_web_bank) + 4;
	last_web = next_web + (max_web * size_web);
	ret = next_web;
	next_web += size_web;
	}

    memset(ret, 0, size_web);		/* zero it out */

    return((WEB *) ret);
}  /* alloc_web */

/*****************************************************************************
 *
 *  COLLAPSE_WEBS()
 *
 *  Description:	Merge webs with common blocks
 *
 *  Called by:		make_webs()
 *
 *  Input Parameters:	pweblist -- pointer to list of webs
 *
 *  Output Parameters:  pweblist -- pointer to list of merged webs
 *
 *  Globals Referenced: maxdefine
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *			discard_web()
 *			intersect()
 *			isemptyset()
 *			merge_webs()
 *			new_set()
 *
 *****************************************************************************
 */

LOCAL void collapse_webs(pweblist)
WEB **pweblist;
{
    flag changing;
    register WEB *wb, *fwb, *wb1, *wbnext;
    SET *t;

    t = new_set(maxdefine);
    changing = YES;
    while (changing == YES)
	{
	changing = NO;
	
	wb = *pweblist;
	fwb = NULL;
	while (wb)
	    {
	    wbnext = wb->next;
	    for (wb1 = *pweblist; wb1 != wb; wb1 = wb1->next)
		{
		intersect(wb1->webloop.inter.reachset,
		          wb->webloop.inter.reachset, t);
		if (! isemptyset(t))
		    {
		    merge_webs(wb1, wb);
		    discard_web(wb);		/* discard web */
		    fwb->next = wbnext;		/* fwb cannot be NULL */
		    changing = YES;
		    goto nextweb;
		    }
		}
	    /* this web not merged with previous one -- update follow ptr */
	    fwb = wb;
nextweb:
	    wb = wbnext;
	    }
	}
    FREESET(t);
}  /* collapse_webs */

/*****************************************************************************
 *
 *  COMPUTE_LOOP_SAVINGS()
 *
 *  Description:	Compute total & adjusted savings for the given loop.
 *
 *  Called by:		find_loop_range()
 *
 *  Input Parameters:	loop -- loop descriptor
 *			duchain -- DUblock chain for loop
 *			regtype -- type of register for allocation
 *
 *  Output Parameters:  loop -- modified loop descriptor
 *
 *  Globals Referenced: reg_store_cost[]
 *			dfo[]
 *			num_to_region[]
 *			stmttab[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void compute_loop_savings(loop, duchain, regtype)
WEB *loop;
DUBLOCK *duchain;
long regtype;
{
    register long totalsave;

    totalsave = 0;

    /* for each load/store, subtract cost from savings */
    {
    register LOADST *lp;
    REGION *rp;
    long blockno;
    long mult1;
    long mult2;

    rp = num_to_region[loop->webloop.loop.regionno];
    for (lp = loop->load_store; lp; lp = lp->next)
	{
	if (lp->stmt1)
	    {
	    if (lp->stmt1 == PREHEADNUM)	/* preheader */
		{
		mult1 = rp->parent->niterations;
		}
	    else
		{
	        blockno = stmttab[lp->stmt1].block;
		mult1 = dfo[blockno]->bb.region->niterations;
		}
	    }
	else
	    {
	    mult1 = MAXINT;		/* ridiculously large */
	    }

	if (lp->stmt2)
	    {
	    blockno = stmttab[lp->stmt2].block;
	    mult2 = dfo[blockno]->bb.region->niterations;
	    }
	else
	    {
	    mult2 = MAXINT;
	    }
	mult1 = (mult1 < mult2) ? mult1 : mult2;
	totalsave -= reg_store_cost[regtype] * mult1;

	if (mc68040 && fortran && loop->var->an.farg)
	  totalsave -= mult1;
	}
    }

    /* for each register-type DU block, add the savings */
    {
    register DUBLOCK *du;
    for (du = duchain; du; du = du->d.nextloop)
	{
	long mult;
        long oldtotalsave = totalsave;

	mult = dfo[stmttab[du->stmtno].block]->bb.region->niterations;
	totalsave += du->savings * mult;
	if ((totalsave < 0) && (oldtotalsave > 0))
	    totalsave = MAXINT;
        }
    }

	/* Compiling for position independant code? */
	if (pic_flag)
	  if ((loop->var->an.tag == AN) || (loop->var->an.tag == XN))
	    if (totalsave >= (MAXINT / 2.3))
	      totalsave = MAXINT;
	    else
	      totalsave = totalsave * 2.3; /* Global vars are more expensive */ 

    /* stuff it all away in the loop descriptor */
    {
    long tsave;

    loop->tot_savings = totalsave;
    tsave = totalsave << SAVINGSSHIFT;
    if (tsave < totalsave)
	tsave = MAXINT;
    loop->adj_savings = tsave
			/ num_to_region[loop->webloop.loop.regionno]->nstmts;
    }
}  /* compute_loop_savings */

/*****************************************************************************
 *
 *  COMPUTE_USE_ONLY_WEB_IF_REGIONS()
 *
 *  Description:	Compute the interfering regions set for a web
 *			containing only uses
 *
 *  Called by:		make_webs_and_loops()
 *
 *  Input Parameters:	wb -- web with only uses
 *
 *  Output Parameters:	wb -- modified
 *
 *  Globals Referenced:	dfo
 *			lastregionno
 *			stmttab
 *
 *  Globals Modified:	none
 *
 *  External Calls:	new_set()
 *			setunion()
 *
 *****************************************************************************
 */
LOCAL void compute_use_only_web_if_regions(wb)
WEB *wb;
{
    register DUBLOCK *du;
    register DUBLOCK *duend;
    SET *ifregions;
    long blockno;

    ifregions = new_set(lastregionno + 1);

    du = wb->webloop.inter.du->next;
    duend = du;
    do {
	if (du->stmtno > 0)
	    {
	    blockno = stmttab[du->stmtno].block;
	    setunion(dfo[blockno]->bb.region->containedin, ifregions);
	    }
	du = du->next;
	} while (du != duend);

    wb->webloop.inter.if_regions = ifregions;
    wb->isinterblock = NO;
    wb->webloop.intra.first_stmt = wb->webloop.inter.du->stmtno;
    wb->webloop.intra.last_stmt = wb->webloop.inter.du->stmtno;
}  /* compute_use_only_web_if_regions */

/*****************************************************************************
 *
 *  COMPUTE_WEB_LOAD_STORE_PTS()
 *
 *  Description:	Determine where registers must be loaded and stored for
 *			a given web.
 *
 *			The philosophy is: the variable is in a register at
 *			entry to each basic block.  Within each basic block,
 *			keep track of the location of the variable value:
 *			memory, register, or both.  If the variable is live
 *			on block exit, it must be in a register.  Constants
 *			always have a current value in memory and may be in
 *			register, too.
 *
 *			The only exception to the above is the case of "maybe"
 *			memory uses with no reaching defs.  The only known
 *			cause of these are actual arguments in FORTRAN calls.
 *			It is impossible to know whether a FORTRAN actual
 *			argument is an input to a subroutine, output, or both.
 *			We assume it's both if there is a reaching def, and
 *			output only if no reaching def.
 *
 *			Register loads and stores are inserted to ensure the
 *			above philosophy is respected.
 *
 *  Called by:		find_web_range()
 *
 *  Input Parameters:	wb -- web descriptor
 *
 *  Output Parameters:  wb -- web descriptor + load-store chain
 *
 *  Globals Referenced: dfo[]
 *			ep[]
 *			outlive[]
 *			stmttab[]
 *			topregion
 *
 *  Globals Modified:	none
 *
 *  External Calls:	add_end_of_block_load()
 *			add_load_store_to_web()
 *			calculate_savings()
 *			def_in_fpa881loop()
 *			difference()
 *			generate_fpa881loop_loads_and_stores()
 *			intersect()
 *			isemptyset()
 *			setup_for_fpa881loops()
 *			use_in_fpa881loop()
 *			xin()
 *
 *****************************************************************************
 */

LOCAL void compute_web_load_store_pts(wb)
WEB *wb;
{
    register DUBLOCK *du;
    DUBLOCK *duend;
    register DUBLOCK *ndu;
    long currblock;
    BBLOCK *bp;
    long checkforfpa881loops;
    long register_type;
    register flag in_memory;
    register flag in_register;
    register flag is_const;
    flag def_in_currblock;
    long newblock;
    
    /* Set constant flag.  Constants are always in memory, may be in register
       too.
     */
    is_const = wb->var->an.isconst;

    if (fpa881loopsexist)
	{
	setup_for_fpa881loops(&checkforfpa881loops, &register_type, wb);
	}
    else
	{
	checkforfpa881loops = NO;
	}

    wb->isdefsonly = YES;		/* turned off iff USE seen */

    /* Var contents assumed to be in register at block entry */
    in_memory = is_const;	/* constants always in memory, assume vars
				 * aren't */
    in_register = YES;		/* all things in registers */
    currblock = -1;		/* no current block yet */

    du = wb->webloop.inter.du->next;
    duend = du;

    do {
	ndu = du->next;
	if (du->ismemory)
	    {
	    if (du->stmtno <= 0)	/* entry or exit defuse */
		{
		if (du->isdef)		/* entry def */
		    {
		    bp = ep[-(du->stmtno)].b.bp;

		    currblock = bp->bb.node_position;
		    def_in_currblock = YES;

		    if (checkforfpa881loops
		     && xin(fpa881loopblocks, currblock))
			;	/* ignore */
		    else if (bp->bb.nestdepth == 0)
			{
			/* if entry block has memory ref, don't do load */
			if ((ndu->stmtno > 0)
			 && (stmttab[ndu->stmtno].block == currblock)
			 && ndu->ismemory)
			    {
			    /* don't do initial load */
			    in_memory = YES;
			    in_register = NO;
			    }
			else
			    /* add load at entry block */
		            add_load_store_to_web(YES, currblock, 0,
                                                   bp->bb.firststmt, 0, wb);
			}
		    else		/* entry block in loop */
		        add_load_store_to_web(YES, 0, PREHEADNUM, 0,
					     bp->bb.region->regionno, wb);
		    }
		else			/* exit use */
		    {
		    wb->isdefsonly = NO;

		    /* add store at exit point */
		    if (topregion->exitstmts)
		        add_load_store_to_web(NO,
				stmttab[topregion->exitstmts->stmt1].block,
				0, topregion->exitstmts->stmt1, 0, wb);
		    }
		}
	    else	/* normal stmt */
		{
		newblock = stmttab[du->stmtno].block;
		if (newblock != currblock)	/* new block ?? */
		    {
		    def_in_currblock = NO;	/* no def seen yet */
		    currblock = newblock;
		    }
		bp = dfo[currblock];
		if (du->isdef)		/* memory def */
		    {
		    def_in_currblock = YES;
		    if (checkforfpa881loops
		     && definetab[du->defsetno].inFPA881loop)
			def_in_fpa881loop(du, wb, register_type);
		    else	/* delay load as long as possible */
			{
		        if ((ndu->stmtno > 0)
			 && (currblock == stmttab[ndu->stmtno].block)
		         && (ndu->stmtno >= du->stmtno))
		            {
			    /* Next DU block in same basic block.  Defer
			     * all load-store decisions until processing
			     * that reference.  Update variable location
			     * flags.
			     */
			    in_memory = YES;
			    in_register = NO;
		            }
		        else
			    {
			    /* This is last reference in this basic block. */
			    if (xin(outlive[currblock], wb->var->an.varno))
		                {
				/* variable is live on block exit -- need to
				 * load contents into register to keep
				 * consistent with assumptions.
				 */
		                if (du->stmtno
				 != (bp->bb.firststmt+ bp->bb.nstmts - 1))
			            {
				    /* This is not last statement in block.
			             * Create load just after this statement.
				     */
			            add_load_store_to_web(YES, currblock,
					du->stmtno, du->stmtno + 1, 0, wb);
			            }
		                else
			            {
				    /* This is last stmt in block */
			            add_end_of_block_load(du, wb);
		                    }
			        }
			    /* If variable is not live on block exit, then this
			     * is a dead definition, don't do anything with
			     * it.
			     */
			    in_register = YES;	/* set up for next block */
			    in_memory = is_const;
			    }
		        }
		    }
	        else	/* memory use */
		    {
		    wb->isdefsonly = NO;

		    if (checkforfpa881loops
		     && xin(fpa881loopblocks, stmttab[du->stmtno].block))
			use_in_fpa881loop(du, wb, register_type);

		    else
			{
			if (!in_memory)
			    {
			    /* Check to see if there are any defs reaching
			     * this memory use.  Some memory uses are
			     * not definite, i.e., "maybe" uses.  Generating
			     * a store for these may cause an uninitialized
			     * 881 register error.  So only generate stores
			     * when we know there's a def getting passed in
			     * a register.
			     */
			    if (def_in_currblock
			     || (wb->var->an.defset
			      && (intersect(wb->var->an.defset,
					   inreach[currblock], flr_defset),
				  !isemptyset(flr_defset))))
				{
			        /* create store immediately before this stmt */
			        add_load_store_to_web(NO, currblock,
				    (bp->bb.firststmt == du->stmtno) ?
					    0 : (du->stmtno - 1),
				    du->stmtno, 0, wb);
				}
			    else	/* no reaching defs -- do nothing */
				{
				in_register = NO;
				}
			    in_memory = YES;
			    }

			if ((ndu->stmtno <= 0)
			 || (currblock != stmttab[ndu->stmtno].block)
			 || (ndu->stmtno < du->stmtno))
			    {
			    /* Last reference in this block */

			    if (!in_register
			     && xin(outlive[currblock], wb->var->an.varno))
		                {
				/* It's live on exit and not in register --
				 * generate a load after this statement
				 */
		                if (du->stmtno
				      != (bp->bb.firststmt+ bp->bb.nstmts - 1))
			            {
			            /* create load just after this statement */
			            add_load_store_to_web(YES, currblock,
					du->stmtno, du->stmtno + 1, 0, wb);
			            }
		                else
			            {
			 	    /* This is last stmt in block */
			            add_end_of_block_load(du, wb);
		                    }
				}

			    /* Set flags for next basic block */
			    in_register = YES;
			    in_memory = is_const;
			    }
			}
		    }
	        }
	    }
	else	/* REGISTER-type def/use */
	    {
	    newblock = stmttab[du->stmtno].block;
	    if (newblock != currblock)	/* new block ?? */
		{
		def_in_currblock = NO;	/* no def seen yet */
		currblock = newblock;
		}

	    if (du->isdef)
		{
		if (checkforfpa881loops
		 && definetab[du->defsetno].inFPA881loop)
		    def_in_fpa881loop(du, wb, register_type);
		else if (wb->inFPA881loop)
		    {
		    du->savings = calculate_savings(register_type,
						du->d.parent, REGISTER);
		    if (mc68040 && fortran && wb->var->an.farg)
		      du->savings += 1;
		    }
		def_in_currblock = YES;
		in_register = YES;
		in_memory = is_const;
		}
	    else  /* use */
		{
	        wb->isdefsonly = NO;

		if (checkforfpa881loops
		 && xin(fpa881loopblocks, stmttab[du->stmtno].block))
			use_in_fpa881loop(du, wb, register_type);
		else
		    {
		    if (wb->inFPA881loop)
		        {
		        du->savings = calculate_savings(register_type,
						du->d.parent, REGISTER);
		        if (mc68040 && fortran && wb->var->an.farg)
		          du->savings += 1;
		        }
		    if (!in_register)
			/* create load just before this DU block's
			 * stmt */
			add_load_store_to_web(YES, currblock,
				(bp->bb.firststmt == du->stmtno) ?
					0 : (du->stmtno - 1),
				du->stmtno, 0, wb);
		    in_register = YES;

		    if ((ndu->stmtno <= 0)
		     || (currblock != stmttab[ndu->stmtno].block)
		     || (ndu->stmtno < du->stmtno))
			/* this is last ref in this basic block */
			in_memory = is_const;		/* set for next block */
		    }
		}
	    }

        du = du->next;
    }  while (du != duend);

    if (checkforfpa881loops)
	{
	/* take out 881-domain from live-range */
	if (wb->isinterblock)
	    difference(fpa881loopblocks, wb->webloop.inter.live_range,
			wb->webloop.inter.live_range);

	/* generate loads and stores at 881-domain boundaries */
	generate_fpa881loop_loads_and_stores(wb);
	}
    
}  /* compute_web_load_store_pts */

/*****************************************************************************
 *
 *  COMPUTE_WEB_SAVINGS_AND_IF_REGIONS()
 *
 *  Description:	Compute total & adjusted savings for the given web.
 *			Also, find the set of interfering regions.
 *
 *  Called by:		find_web_range()
 *
 *  Input Parameters:	wb -- web descriptor
 *			regtype -- type of register for allocation
 *
 *  Output Parameters:  wb -- modified web descriptor
 *
 *  Globals Referenced: reg_store_cost[]
 *			dfo[]
 *			stmttab[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	nextel()
 *			new_set()
 *			setassign()
 *			setunion()
 *
 *****************************************************************************
 */

LOCAL void compute_web_savings_and_if_regions(wb, regtype)
WEB *wb;
long regtype;
{
    register long totalsave;
    register long nstmts;
    SET * ifregions;

    totalsave = 0;

    /* for each load/store, subtract cost from savings */
    {
    register LOADST *lp;
    for (lp = wb->load_store; lp; lp = lp->next)
	{
	long blockno;
	register long mult;

	if (lp->stmt1)
	    {
	    if (lp->stmt1 == PREHEADNUM)
		{
		mult = num_to_region[lp->regionno]->parent->niterations;
		}
	    else
		{
		blockno = stmttab[lp->stmt1].block;
	        mult = dfo[blockno]->bb.region->niterations;
		}
	    }
	else
	    {
	    blockno = stmttab[lp->stmt2].block;
	    mult = dfo[blockno]->bb.region->niterations;
	    }
	totalsave -= reg_store_cost[regtype] * mult;
	if (mc68040 && fortran && wb->var->an.farg)
	  totalsave -= mult;
	}
    }

    /* for each register-type DU block, add the savings */
    {
    register DUBLOCK *du;
    register DUBLOCK *duend;
    register long mult;
    register long oldtotalsave;

    du = wb->webloop.inter.du->next;
    duend = du;
    do {
	if ((du->ismemory == NO) && !du->inFPA881loop)
	    {
	    mult = dfo[stmttab[du->stmtno].block]->bb.region->niterations;
	    if (du->isdef && (du->savings <= 0))  /* memory assignment */
		{
		if ((du->next != duend)		/* there is a next du block */
	         && (du->next->isdef == NO)	/* next block is a use */
	         && (du->next->ismemory == NO)	/* next is a register use */
		 && (du->next->savings > 0)	/* not a memory assignment */
	         && (stmttab[du->stmtno].block ==
			 stmttab[du->next->stmtno].block))  /* in same block */
		    {
		    du->savings = reg_store_cost[wb->var->an.register_type];
		    }
		}
	    oldtotalsave = totalsave;
	    totalsave += du->savings * mult;
	    if ((totalsave < 0) && (oldtotalsave > 0) && (du->savings > 0))
		totalsave = MAXINT;
	    }
	du = du->next;
	} while (du != duend);
	/* Compiling for position independant code? */
	if (pic_flag)
	  if ((wb->var->an.tag == AN) || (wb->var->an.tag == XN))
	    if (totalsave >= (MAXINT / 2.3))
	      totalsave = MAXINT;
	    else
	      totalsave = totalsave * 2.3; /* Global vars are more expensive */ 

    }

    /* compute interfering regions sets */
    {
    register long i;
    register long *pelem;

    ifregions = new_set(lastregionno + 1);

    if (wb->isinterblock)
	{
	set_elem_vector(wb->webloop.inter.live_range, cwsair_elems);
	pelem = cwsair_elems;
	nstmts = 0;
	/* process each block in the live range set */
	while ((i = *(pelem++)) >= 0)
	    {
	    nstmts += dfo[i]->bb.nstmts;
	    setunion(dfo[i]->bb.region->containedin, ifregions);
	    }
	wb->webloop.inter.nstmts = nstmts;
	}
    else if (wb->isdefsonly)
	{
	nstmts = 1;
	/* don't worry about interfering regions */
	}
    else
	{
	long blockno;
	nstmts = wb->webloop.intra.last_stmt - wb->webloop.intra.first_stmt + 1;
	blockno = stmttab[wb->webloop.intra.first_stmt].block;
	setassign(dfo[blockno]->bb.region->containedin, ifregions);
	}   
    }

    {
    long tsave;

    /* stuff it all away in the web descriptor */
    wb->tot_savings = totalsave;
    tsave = totalsave << SAVINGSSHIFT;
    if (tsave < totalsave)
	tsave = MAXINT;
    wb->adj_savings = tsave / nstmts;
    wb->webloop.inter.if_regions = ifregions;
    }
}  /* compute_web_savings_and_if_regions */

/*****************************************************************************
 *
 *  DEF_IN_FPA881LOOP()
 *
 *  Description:	Process a DUBLOCK "def" which occurs in an 
 *			FPA881loop.  Part of the web is outside the loop.
 *			Check if the def is used in FPA code.  If so,
 *			determine which loop the load should be for and
 *			set the bit.
 *
 *  Called by:		compute_web_load_store_pts()
 *
 *  Input Parameters:	du -- DUBLOCK pointer
 *			wb -- WEB pointer
 *			register_type -- 881 register type
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

LOCAL void def_in_fpa881loop(du, wb, register_type)
DUBLOCK *du;
WEB *wb;
long register_type;
{
    long defsetno;
    REGION *rp;
    REGION *rptst;

    defsetno = du->defsetno;

    /* is this def used outside the fpa881loop ??? */
    intersect(blockreach[defsetno], blocklive[wb->var->an.varno],
		fpa881blocktset);
    difference(fpa881loopblocks, fpa881blocktset, fpa881blocktset);
    if (! isemptyset(fpa881blocktset))		/* used outside */
	{
	/* find largest fpa881loop enclosing def, completely inside web */
	rp = dfo[stmttab[du->stmtno].block]->bb.region;
	while (rp != topregion)
	    {
	    rptst = rp->parent;
	    switch(setcompare(rptst->blocks, wb->webloop.inter.live_range))
		{
		case ONE_IN_TWO:
		case ONE_IS_TWO:
			;

		case TWO_IN_ONE:
		case OVERLAP:
			goto exitwhile;

		default:
#pragma BBA_IGNORE
			cerror("bad setcompare result in def_in_fpa881loop()");
		}
	    rp = rptst;
	    }
exitwhile:

	loop_exit_load_needed[rp->regionno] = YES;
	adelement(defsetno, defsinfpa881loop, defsinfpa881loop);
	}

    du->savings = calculate_savings(register_type, du->d.parent,
				du->ismemory);

    if (mc68040 && fortran && wb->var->an.farg)
      du->savings += 1;

    du->inFPA881loop = YES;
}  /* def_in_fpa881loops */

/*****************************************************************************
 *
 *  DISCARD_WEB()
 *
 *  Description:	free all data structures associated with a web
 *
 *  Called by:		collapse_webs()
 *			color_web()	-- in regallo.c
 *			do_loop_allocation()
 *			insert_loads_and_stores() -- in regpass2.c
 *			make_webs_and_loops()
 *			prune_looplist()
 *
 *  Input Parameters:	web -- web to be free'd
 *
 *  Output Parameters:  none
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *			free_dublock()
 *			free_loadst()
 *			free_web()
 *
 *****************************************************************************
 */

void discard_web(web)
register WEB *web;
{
    if (web->s.loops != NULL)
	FREESET(web->s.loops);
    if (web->load_store != NULL)
	{
	register LOADST *lp;
	register LOADST *np;
	for (lp = web->load_store; lp; lp = np)
	    {
	    np = lp->next;
	    free_loadst(lp);
	    }
	}
    if (web->isweb)
	{
	if (web->webloop.inter.du)
	    {
	    register DUBLOCK *du;
	    register DUBLOCK *ndu;
	    register DUBLOCK *duend;
	    duend = web->webloop.inter.du;
	    du = duend;
	    do {
		ndu = du->next;
		free_dublock(du);
		du = ndu;
	       } while (du != duend);
	    }
	if (web->webloop.inter.if_regions)
	    FREESET(web->webloop.inter.if_regions);
	if (web->webloop.inter.reachset)
	    FREESET(web->webloop.inter.reachset);

	if (web->isinterblock)
	    {
	    if (web->webloop.inter.live_range)
		FREESET(web->webloop.inter.live_range);
	    }
	}
    free_web(web);
}  /* discard_web */

/*****************************************************************************
 *
 *  DO_LOOP_ALLOCATION()
 *
 *  Description:	Find loop candidates for a given symtab entry.
 *			Insert into register allocation list.
 *
 *  Called by:		make_webs_and_loops()
 *
 *  Input Parameters:	weblist -- list of webs for symtab entry
 *			regtype -- register type
 *			regclass -- register class list structure
 *
 *  Output Parameters:  regclass -- modified register class list structure
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_web()
 *			discard_web()
 *			find_good_loops()
 *			find_loop_range()
 *			insert_loop_in_looplist()
 *			nextel()
 *			prune_looplist()
 *			regclass_list_insert()
 *			xin()
 *
 *****************************************************************************
 */

LOCAL void do_loop_allocation(weblist, regtype, regclass)
WEB *weblist;
long regtype;
REGCLASSLIST *regclass;
{
    SET *goodloops;
    register WEB *web;
    WEB *webnext;
    WEB *worklist;
    register WEB *loop;
    WEB *loopnext;
    register long loopno;
    WEB *fpa881worklist;
    long fpa881regtype;
    flag isfpa881loop;

    fpa881regtype = (regtype == REG_FPA_FLOAT_SAVINGS) ? REG_881_FLOAT_SAVINGS :
				REG_881_DOUBLE_SAVINGS;

    /* find eligible loops */
    goodloops = new_set(lastregionno + 1);
    find_good_loops(weblist, goodloops);

    /* find loops for each web */

    for (web = weblist; web; web = webnext)
	{
	webnext = web->next;
	if (web->isinterblock)
	    {
	    web->next = NULL;
	    web->prev = NULL;
	    worklist = NULL;
	    fpa881worklist = NULL;
	    if (web->tot_savings > 0)
		{
		if ((web->inFPA881loop)
    		 && ((regtype == REG_FPA_FLOAT_SAVINGS)
    		  || (regtype == REG_FPA_DOUBLE_SAVINGS)))
		    fpa881worklist = web;
		else
		    worklist = web;
		}

	    /* process all potential loops for this web */
	    loopno = -1;
	    while ((loopno = nextel(loopno,web->s.loops)) >= 0)
		{
		if (xin(goodloops, loopno))
		    {
		    loop = alloc_web();
		    loop->isweb = NO;
		    loop->var = web->var;
		    loop->webloop.loop.web = web;
		    loop->webloop.loop.regionno = loopno;

		    isfpa881loop = num_to_region[loopno]->isFPA881loop
    			&& ((regtype == REG_FPA_FLOAT_SAVINGS)
    			 || (regtype == REG_FPA_DOUBLE_SAVINGS));

		    /* find loop range, savings, load & store pts */
		    find_loop_range(web, loop,
					isfpa881loop ? fpa881regtype : regtype);

		    /* throw away unpromising loops */
		    if (loop->tot_savings <= 0)
		        discard_web(loop);
		    else
		        /* insert loop in list ordered by # statements */
		        insert_loop_in_looplist(
			    isfpa881loop ? &fpa881worklist :&worklist, loop);
		    }
		}

	    if (web->tot_savings <= 0)
		discard_web(web);
#			ifdef DEBUGGING
			if (rwdebug > 2)
			    {
			    fprintf(debugp, "\nBefore prune_looplist()\n");
			    dump_work_loop_list(web, worklist);
			    }
#			endif

	    /* prune the list */
	    if (worklist != NULL)
	        prune_looplist(&worklist);
	    if (fpa881worklist != NULL)
		prune_looplist(&fpa881worklist);

#			ifdef DEBUGGING
			if (rwdebug > 1)
			    {
			    fprintf(debugp, "\nAfter prune_looplist()\n");
			    dump_work_loop_list(0, worklist);
			    }
#			endif

	    /* insert promising webs and loops into register class list */
	    for (loop = worklist; loop; loop = loopnext)
		{
		loopnext = loop->next;
		regclass_list_insert(regclass, loop);
		}
	    for (loop = fpa881worklist; loop; loop = loopnext)
		{
		loopnext = loop->next;
		regclass_list_insert(regclasslist + F881CLASS, loop);
		}
	    }
	else	/* intrablock web */
	    {
	    if ((web->tot_savings > 0) && ! web->isdefsonly)
		{
		if (web->inFPA881loop
    		 && ((regtype == REG_FPA_FLOAT_SAVINGS)
    		  || (regtype == REG_FPA_DOUBLE_SAVINGS)))
		    regclass_list_insert(regclasslist + F881CLASS, web);
		else
		    regclass_list_insert(regclass, web);
		}
	    else
		discard_web(web);
	    }
	}
    FREESET(goodloops);

}  /* do_loop_allocation */

/*****************************************************************************
 *
 *  FILE_INIT_STORAGE()
 *
 *  Description:	Initialize the DUBLOCK, LOADST, PLINK, and WEB
 *			storage management structures.  Allocate initial
 *			banks and set global variables.
 *
 *  Called by:		main() -- c1.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	curr_dublock_bank
 *			curr_loadst_bank
 *			curr_plink_bank
 *			curr_web_bank
 *			first_dublock_bank
 *			first_loadst_bank
 *			first_plink_bank
 *			first_web_bank
 *			free_dublock_list
 *			free_loadst_list
 *			free_plink_list
 *			free_web_list
 *			last_dublock
 *			last_loadst
 *			last_plink
 *			last_web
 *			max_dublock
 *			max_loadst
 *			max_plink
 *			max_web
 *			next_dublock
 *			next_loadst
 *			next_plink
 *			next_web
 *			size_dublock
 *			size_loadst
 *			size_plink
 *			size_web
 *
 *  External Calls:	ckmalloc()
 *
 *****************************************************************************
 */

void file_init_storage()
{
    max_farg_slots = MAX_FARG_SLOTS;
    farg_slots = (long *)ckalloc(max_farg_slots * sizeof(long));

    max_dublock = DUBLOCKS_IN_BANK;
    size_dublock = sizeof(DUBLOCK);
    if ((size_dublock % 4) != 0)
	size_dublock += 4 - (size_dublock % 4);
    first_dublock_bank = (char **) ckalloc(4 + (max_dublock * size_dublock));
    curr_dublock_bank = first_dublock_bank;
    *curr_dublock_bank = NULL;		/* no following banks */
    next_dublock = ((char *)curr_dublock_bank) + 4;
    last_dublock = next_dublock + (max_dublock * size_dublock);
    free_dublock_list = NULL;			/* no free blocks yet */

    max_loadst = LOADSTS_IN_BANK;
    size_loadst = sizeof(LOADST);
    if ((size_loadst % 4) != 0)
	size_loadst += 4 - (size_loadst % 4);
    first_loadst_bank = (char **) ckalloc(4 + (max_loadst * size_loadst));
    curr_loadst_bank = first_loadst_bank;
    *curr_loadst_bank = NULL;		/* no following banks */
    next_loadst = ((char *)curr_loadst_bank) + 4;
    last_loadst = next_loadst + (max_loadst * size_loadst);
    free_loadst_list = NULL;			/* no free blocks yet */

    max_plink = PLINKS_IN_BANK;
    size_plink = sizeof(PLINK);
    if ((size_plink % 4) != 0)
	size_plink += 4 - (size_plink % 4);
    first_plink_bank = (char **) ckalloc(4 + (max_plink * size_plink));
    curr_plink_bank = first_plink_bank;
    *curr_plink_bank = NULL;	/* no following banks */
    next_plink = ((char *)curr_plink_bank) + 4;
    last_plink = next_plink + (max_plink * size_plink);
    free_plink_list = NULL;			/* no free blocks yet */

    max_web = WEBS_IN_BANK;
    size_web = sizeof(WEB);
    if ((size_web % 4) != 0)
	size_web += 4 - (size_web % 4);
    first_web_bank = (char **) ckalloc(4 + (max_web * size_web));
    curr_web_bank = first_web_bank;
    *curr_web_bank = NULL;	/* no following banks */
    next_web = ((char *)curr_web_bank) + 4;
    last_web = next_web + (max_web * size_web);
    free_web_list = NULL;			/* no free blocks yet */
}  /* file_init_storage */

/*****************************************************************************
 *
 *  FIND_GOOD_LOOPS()
 *
 *  Description:	Find loops which are eligible for loop allocation for 
 *			a given set of webs.  Ineligible loops are in the live
 *			range of more than one web, or have memory-type defuses
 *			in them.
 *
 *  Called by:		do_loop_allocation()
 *
 *  Input Parameters:	weblist -- list of webs
 *			goodloops -- pointer to SET (good loop set)
 *
 *  Output Parameters:  goodloops -- set filled by this routine
 *
 *  Globals Referenced:	dfo[]
 *			lastregionno
 *			num_to_region[]
 *			stmttab[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *			difference()
 *			intersect()
 *			isemptyset()
 *			new_set()
 *			nextel()
 *			setcompare()
 *			setunion()
 *
 *****************************************************************************
 */

	LOCAL void find_good_loops(weblist, goodloops)
	WEB *weblist;
	SET *goodloops;
{
    register WEB *web;
    SET *loops;
    register DUBLOCK *dp;
    register DUBLOCK *dpend;
    REGION *rp;
    long regionno;

    clearset(fgl_badloops);
    clearset(fgl_loopsseen);

    for (web = weblist; web; web = web->next)
	{

	/* any loops affected by more than one web are "bad" */
	intersect(web->webloop.inter.if_regions, fgl_loopsseen, fgl_t);
	setunion(web->webloop.inter.if_regions, fgl_loopsseen);
	if (!isemptyset(fgl_t))
	    {
	    setunion(fgl_t, fgl_badloops);
	    }

	/* any loops completely containing a web are bad */
	regionno = -1;
	while ((regionno = nextel(regionno, web->webloop.inter.if_regions))
		>= 0)
	    {
	    if (web->isinterblock)
		{
	        long result;

	        result = setcompare(num_to_region[regionno]->blocks,
					web->webloop.inter.live_range);
	        if ((result == TWO_IN_ONE) || (result == ONE_IS_TWO))
		    {
		    setunion(num_to_region[regionno]->containedin, fgl_badloops);
		    break;
		    }
		}
	    else	/* intrablock */
		{
		long blockno;

		blockno = stmttab[web->webloop.intra.first_stmt].block;
		if (xin(num_to_region[regionno]->blocks, blockno))
		    {
		    setunion(num_to_region[regionno]->containedin, fgl_badloops);
		    break;
		    }
		}
	    }

	/* all loops with memory-type DUblocks are "bad", too */
	/* register-type DUblocks imply potential loops */

	loops = new_set(lastregionno + 1);
	dp = web->webloop.inter.du->next;
	dpend = dp;
	do {
	    if (dp->ismemory)
		{
		if (dp->stmtno > 0)
		    {
		    rp = dfo[stmttab[dp->stmtno].block]->bb.region;
		    setunion(rp->containedin, fgl_badloops);
		    }
		else
		    {
		    adelement(topregion->regionno, fgl_badloops, fgl_badloops);
		    }
		}
	    else
		{
		rp = dfo[stmttab[dp->stmtno].block]->bb.region;
		setunion(rp->containedin, loops);
		}
	    dp = dp->next;
	    } while (dp != dpend);

	web->s.loops = loops;
	setunion(loops, goodloops);
	}

    difference(fgl_badloops, goodloops, goodloops);

}  /* find_good_loops */

/*****************************************************************************
 *
 *  FIND_LOOP_RANGE()
 *
 *  Description:	Compute register loads and stores for loop allocation.
 *			Compute savings.
 *
 *  Called by:		do_loop_allocation()
 *
 *  Input Parameters:	web -- web associated with this loop
 *			loop -- WEB structure describing this loop
 *			regtype -- register type for calculating savings
 *
 *  Output Parameters:	loop -- modified loop descriptor
 *
 *  Globals Referenced:	blocklive[]
 *			blockreach[]
 *			maxdefine
 *			num_to_region[]
 *			numblocks
 *			stmttab[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *			add_load_store_to_web()
 *			adelement()
 *			compute_loop_savings()
 *			difference()
 *			intersect()
 *			isemptyset()
 *			new_set()
 *			nextel()
 *			setunion()
 *			xin()
 *
 *****************************************************************************
 */

LOCAL void find_loop_range(web, loop, regtype)
WEB *web;
WEB *loop;
long regtype;
{
    register DUBLOCK *ldu;	/* loop DU chain */
    register REGION *rp;	/* this region */
    register long i;
    flag need_load;


    clearset(flr_defset);	/* definitions within this loop */
    clearset(flr_reachset);	/* definitions used by uses within this loop */
    rp = num_to_region[loop->webloop.loop.regionno];

    /* Collect DU blocks for this loop.  Find defs within this loop and
     * defs used by references within this loop.
     */
    ldu = NULL;			/* no DU blocks yet */

    {
    register DUBLOCK *du;
    DUBLOCK *duend;
    long blockno;

    du = web->webloop.inter.du->next;
    duend = du;

    do {
	if (du->stmtno > 0)
	    {
	    blockno = stmttab[du->stmtno].block;
	    if (xin(rp->blocks, blockno))
		{
	        du->d.nextloop = ldu;
	        ldu = du;		/* add DUblock to chain */
	        if (du->isdef)
		    {
		    adelement(du->defsetno, flr_defset, flr_defset);
		    }
	        else	/* is use */
		    {
		    if (du->defsetno != 0)
		        {
		        adelement(du->defsetno, flr_reachset,
							flr_reachset);
		        }
		    else
		        {
		        intersect(inreach[blockno], web->var->an.defset, flr_t);
		        setunion(flr_t, flr_reachset);
		        }
		    }
		}
	    }
	du = du->next;
	} while (du != duend);
    }

    /* Add a load at loop entry (preheader) if defs reach from outside */
    difference(flr_defset, flr_reachset, flr_t);    /* t = reachset - defset */
    need_load = !isemptyset(flr_t);

    if (! need_load)
	{
        register LOADST *lrp;

	/* get all defs from outside reaching to loop */
	difference(flr_defset, inreach[rp->entryblocks->val], flr_reachset);
	intersect(web->var->an.defset, flr_reachset, flr_reachset);

	/* find all blocks reached by these defs */
	clearset(flr_blockset);
	i = 0;
	while ((i = nextel(i, flr_reachset)) >= 0)
	    {
	    setunion(blockreach[i], flr_blockset);
	    }

	/* find all reached blocks which are live */
	intersect(flr_blockset, blocklive[web->var->an.varno], flr_reachlive);
	
	/* if any loop exits are a reached, live block -- need a load */
        for (lrp = rp->exitstmts; lrp; lrp = lrp->next)
	    {
	    if (xin(flr_reachlive, stmttab[lrp->stmt2].block))
	        need_load = YES;
	    }
	}

    if (need_load)
	add_load_store_to_web(YES, 0, PREHEADNUM, 0, rp->regionno, loop);

    /* Add stores for loop exits carrying defs from inside the loop to a
     * use outside.
     */
    if ( !isemptyset(flr_defset) )
	{
	clearset(flr_blockset);

	/* find all blocks reached by defs within the loop */
	i = 0;
	while ((i = nextel(i, flr_defset)) >= 0)
	    {
	    setunion(blockreach[i], flr_blockset);
	    }

	/* find range of blocks with defs meeting actual uses */
	intersect(flr_blockset, blocklive[loop->var->an.varno], flr_reachlive);

	/* we're only interested in those blocks outside this region */
	difference(rp->blocks, flr_reachlive, flr_leftovers);

	/* add stores iff there are blocks outside the loop reached by defs
	 * inside the loop.
	 */
	if ( !isemptyset(flr_leftovers) )
	    {
	    /* go thru region exits, looking for branches into leftovers blocks
	     */
	    register LOADST *lrp;
	    for (lrp = rp->exitstmts; lrp; lrp = lrp->next)
		{
		if (xin(flr_leftovers, stmttab[lrp->stmt2].block))
		    add_load_store_to_web(NO, 0, lrp->stmt1, lrp->stmt2, 0, 
                                          loop);
		}
	    }
	}

    /* compute the loop savings */
    compute_loop_savings(loop, ldu, regtype);

}  /* find_loop_range */

/*****************************************************************************
 *
 *  FIND_WEB_RANGE()
 *
 *  Description:	Determine live range of a web.  Calculate the load and
 *			store points.  Calculate the total and adjusted savings.
 *			Find the set of interfering regions.
 *
 *  Called by:		make_webs_and_loops()
 *
 *  Input Parameters:	wb -- web
 *	 		regtype -- register type for allocation
 *
 *  Output Parameters:  wb -- modified web descriptor
 *			return value -- NO iff bad web, YES iff OK
 *
 *  Globals Referenced:	blocklive[]
 *			blockreach[]
 *			definetab[]
 *			numblocks
 *			stmttab[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	adelement()
 *			cerror()
 *			compute_use_only_web_if_regions()
 *			compute_web_load_store_pts()
 *			compute_web_savings_and_if_regions()
 *			intersect()
 *			isemptyset()
 *			new_set()
 *			nextel()
 *			setunion()
 *
 *****************************************************************************
 */

LOCAL long find_web_range(wb, regtype)
register WEB *wb;
long regtype;
{
    SET *liverange;
    register long i;
    register long ndefs;

    /* find all blocks reached by defs in this web */
    clearset(fwr_defblocks);
    ndefs = 0;
    i = 0;
    while ((i = nextel(i, wb->webloop.inter.reachset)) >= 0)
	{
	ndefs++;
	setunion(blockreach[i], fwr_defblocks);
	}

    liverange = new_set(maxnumblocks);
    intersect(fwr_defblocks, blocklive[wb->var->a6n.varno], liverange);

    if (ndefs == 0)
	{
	if (wb->hasdefiniteuse)
#pragma BBA_IGNORE
	    cerror("No defs for web in find_web_range()");
	else
	    {
	    compute_use_only_web_if_regions(wb);
	    FREESET(liverange);
	    return NO; 		/* bad web */
	    }
	}

    if (isemptyset(liverange))		/* intrablock web */
	{
	wb->isinterblock = NO;
	wb->webloop.intra.first_stmt = wb->webloop.inter.du->next->stmtno;
	wb->webloop.intra.last_stmt = wb->webloop.inter.du->stmtno;
	if (stmttab[wb->webloop.intra.first_stmt].block
		!= stmttab[wb->webloop.intra.last_stmt].block)
#pragma BBA_IGNORE
	    cerror("intrablock web spans blocks in find_web_range()");
        FREESET(liverange);
	}
    else				/* interblock web */
	{
	wb->isinterblock = YES;

	/* add all blocks containing defs to the liverange */
        i = 0;
        while ((i = nextel(i, wb->webloop.inter.reachset)) >= 0)
	    {
	    if (definetab[i].stmtno > 0)
	        adelement(stmttab[definetab[i].stmtno].block, liverange,
							      liverange);
    	    }
	wb->webloop.inter.live_range = liverange;
	}

    /* determine the register load and store points */
    compute_web_load_store_pts(wb);

    /* compute the total and adjusted savings and interfering regions */
    compute_web_savings_and_if_regions(wb, regtype);

    return YES;

}  /* find_web_range */

/*****************************************************************************
 *
 *  FIX_MEMORY_DEF_USE()
 *
 *  Description:	Go thru DU blocks for a symtab entry.  If
 *			there is a memory-type reference for a statement,
 *			change all other references for that statement to be
 *			memory-type.
 *
 *  Called by:		make_web_and_loops()
 *
 *  Input Parameters:	sp -- pointer to symtab entry
 *
 *  Output Parameters:  none
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void fix_memory_def_use(sp)
register HASHU *sp;
{
    register long stmtno;
    register flag ismemory;
    DUBLOCK *first_stmt_du;
    register DUBLOCK *du;
    register DUBLOCK *duend;

    du = sp->a6n.du->next;
    duend = du;
    stmtno = -100000000;	/* impossible stmtno -- to get us started */
    do {
	if (du->stmtno == stmtno)
	    {
	    if (ismemory && !du->ismemory)	/* change to memory-type */
		{
		du->ismemory = YES;
		du->savings = 0;
		}
	    else if (du->ismemory && !ismemory)	    /* register-types seen */
		{
		register DUBLOCK *eu;

		for (eu = first_stmt_du; eu != du; eu = eu->next)
		    {
		    eu->ismemory = YES;
		    eu->savings = 0;
		    }
		ismemory = YES;
		}
	    }
	else		/* new statement */
	    {
	    stmtno = du->stmtno;
	    ismemory = du->ismemory;
	    first_stmt_du = du;
	    }
        du = du->next;
    } while (du != duend);
}  /* fix_memory_def_use */

/*****************************************************************************
 *
 *  FREE_DUBLOCK()
 *
 *  Description:	Free a DUBLOCK structure
 *
 *  Called by:		discard_web()
 *
 *  Input Parameters:	dp -- pointer to DUBLOCK structure
 *
 *  Output Parameters:	None
 *
 *  Globals Referenced: free_dublock_list
 *
 *  Globals Modified:	free_dublock_list
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

void free_dublock(dp)
DUBLOCK *dp;
{
    *((char **)dp) = free_dublock_list;
    free_dublock_list = (char *) dp;
}  /* free_dublock */

/*****************************************************************************
 *
 *  FREE_LINKED_LIST()
 *
 *  Description:	Free a linked list of memory blocks. The parameter
 *			'lp' points to the first block of memory in the list.
 *
 *  Called by:		proc_init_storage()
 *			init_du_storage()
 *
 *  Input Parameters:	lp -- pointer to a linked list of heap blocks.
 *
 *  Output Parameters:  none
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

void free_linked_list(lp)
char **lp;
{
if (lp != NULL)
  {
  free_linked_list(*lp);
  FREEIT(lp);
  } 
}  /* free_linked_list */

/*****************************************************************************
 *
 *  FREE_LOADST()
 *
 *  Description:	Free a LOADST structure
 *
 *  Called by:		discard_web()
 *
 *  Input Parameters:	lp -- pointer to LOADST structure
 *
 *  Output Parameters:	None
 *
 *  Globals Referenced: free_loadst_list
 *
 *  Globals Modified:	free_loadst_list
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

void free_loadst(lp)
LOADST *lp;
{
    *((char **) lp) = free_loadst_list;
    free_loadst_list = (char *) lp;
}  /* free_loadst */

/*****************************************************************************
 *
 *  FREE_PLINK()
 *
 *  Description:	Free a PLINK structure
 *
 *  Called by:		
 *
 *  Input Parameters:	pp -- pointer to PLINK structure
 *
 *  Output Parameters:	None
 *
 *  Globals Referenced: free_plink_list
 *
 *  Globals Modified:	free_plink_list
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

void free_plink(pp)
PLINK *pp;
{
    *((char **) pp) = free_plink_list;
    free_plink_list = (char *) pp;
}  /* free_plink */

/*****************************************************************************
 *
 *  FREE_WEB()
 *
 *  Description:	Free a WEB structure
 *
 *  Called by:		discard_web()
 *
 *  Input Parameters:	wb -- pointer to WEB structure
 *
 *  Output Parameters:	None
 *
 *  Globals Referenced: free_web_list
 *
 *  Globals Modified:	free_web_list
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

void free_web(wb)
WEB *wb;
{
    *((char **) wb) = free_web_list;
    free_web_list = (char *) wb;
}  /* free_web */

/*****************************************************************************
 *
 *  GENERATE_FPA881LOOP_LOADS_AND_STORES()
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
LOCAL void generate_fpa881loop_loads_and_stores(wb)
WEB *wb;
{
    register long rn;
    long varno;
    register LOADST *lp;

    varno = wb->var->an.varno;

    for (rn = 0; rn <= lastregionno; ++rn)
	{
	if (loop_entry_store_needed[rn])
	    add_load_store_to_web(NO, 0, PREHEADNUM, 0, rn, wb);
#ifdef COUNTING
	    _n_FPA881_stores++;
#endif
	}

	clearset(fpa881livedefblockset);

    /* find set of blocks reached by defs in the loop AND with uses outside the
     * loop.
     */
    rn = 0;
    while ((rn = nextel(rn, defsinfpa881loop)) > 0)
	{
	intersect(blockreach[rn], blocklive[varno], fpa881blocktset);
	difference(fpa881loopblocks, fpa881blocktset, fpa881blocktset);
	setunion(fpa881blocktset, fpa881livedefblockset);
	}

    for (rn = 0; rn <= lastregionno; ++rn)
	{
	if (loop_exit_load_needed[rn])
	    {
	    for (lp = num_to_region[rn]->exitstmts; lp; lp = lp->next)
		{
		if (xin(fpa881livedefblockset, stmttab[lp->stmt2].block))
		    {
		    add_load_store_to_web(YES, 0, lp->stmt1, lp->stmt2, 0, wb);
#ifdef COUNTING
		    _n_FPA881_stores++;
#endif
		    }
		}
	    }
	}
}  /* generate_fpa881loop_loads_and_stores */

/*****************************************************************************
 *
 *  INIT_DU_STORAGE()
 *
 *  Description:	Initialize DUBLOCK storage
 *
 *  Called by:		cleanup_def_use_opts() -- duopts.c
 *			proc_init_storage()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	curr_dublock_bank
 *			first_dublock_bank;
 *			max_dublock
 *			next_dublock
 *			size_dublock
 *
 *  Globals Modified:	curr_dublock_bank
 *			free_dublock_list
 *			last_dublock
 *			next_dublock
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

void init_du_storage()
{
    curr_dublock_bank = first_dublock_bank;
    next_dublock = ((char *)curr_dublock_bank) + 4;
    last_dublock = next_dublock + size_dublock * max_dublock;
    free_dublock_list = NULL;
    free_linked_list(*first_dublock_bank);
    *first_dublock_bank = NULL;
}  /* init_du_storage */

/*****************************************************************************
 *
 *  INIT_REG_CLASSES()
 *
 *  Description:	Initialize register class candidate lists
 *
 *  Called by:		make_webs_and_loops()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	regclasslist[]
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void init_reg_classes()
{
    register long i;

    for (i = 0; i < NREGCLASSES; ++i)
	{
	regclasslist[i].list = NULL;
	regclasslist[i].nlist_items = 0;
	}

}  /* init_reg_classes */

/*****************************************************************************
 *
 *  INSERT_LOOP_IN_LOOPLIST()
 *
 *  Description:	Insert given loop in the working list.  The list is
 *			ordered by decreasing number of statements.
 *
 *  Called by:		do_loop_allocation()
 *
 *  Input Parameters:	pworklist -- pointer to head of list pointer.
 *			loop -- loop to be inserted
 *
 *  Output Parameters:	pworklist -- list is modified by this routine
 *
 *  Globals Referenced:	num_to_region[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */
LOCAL void insert_loop_in_looplist(pworklist, loop)
WEB **pworklist;
register WEB *loop;
{
    register WEB *web;
    register WEB *fweb;
    register long nstmts;

    nstmts = num_to_region[loop->webloop.loop.regionno]->nstmts;
    for (web = *pworklist, fweb = NULL; web; fweb = web, web = web->next)
	{
	if (web->isweb)
	    {
	    if (web->isinterblock)
		{
		if (nstmts > web->webloop.inter.nstmts)
		    goto add_loop;
		}
	    else
		{
		if (nstmts > (web->webloop.intra.last_stmt
				- web->webloop.intra.first_stmt + 1))
		    goto add_loop;
		}
	    }
	else
	    {
	    if (nstmts > num_to_region[web->webloop.loop.regionno]->nstmts)
		{
		goto add_loop;
		}
	    }
	}

add_loop:
    if (fweb)
	{
	loop->next = fweb->next;
	fweb->next = loop;
	loop->prev = fweb;
	}
    else
	{
	loop->next = *pworklist;
	*pworklist = loop;
	loop->prev = NULL;
	}

    if (loop->next)
	loop->next->prev = loop;

}  /* insert_loop_in_looplist */

/*****************************************************************************
 *
 *  MAKE_WEBS()
 *
 *  Description:	Collect DU blocks for given symtab entry into disjoint
 *			webs.
 *
 *  Called by:		make_webs_and_loops()
 *
 *  Input Parameters:	sp -- pointer to symtab entry
 *
 *  Output Parameters:  pweblist -- pointer to weblist which is filled
 *
 *  Globals Referenced:	inreach[]
 *			maxdefine
 *			stmttab[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *			adelement()
 *			alloc_web()
 *			cerror()
 *			collapse_webs()
 *			discard_web()
 *			fprintf()
 *			intersect()
 *			isemptyset()
 *			new_set()
 *			setunion()
 *			xin()
 *			
 *****************************************************************************
 */

LOCAL void make_webs(sp, pweblist)
HASHU *sp;
WEB **pweblist;
{
    WEB *weblist;		/* web list */
    register long blockno;	/* current blockno */
    register long newblockno;	/* du's blockno */
    register WEB *blockweb;	/* web for blockno */
    register DUBLOCK *du;
    DUBLOCK *duend;
    DUBLOCK *dunext;
    register WEB *wb;
    flag isdefiniteuse;

    weblist = NULL;	/* no webs yet */
    du = sp->a6n.du->next;
    duend = du;
    blockno = -10000000;		/* impossible blockno -- to get us started */

    do {
	dunext = du->next;

	isdefiniteuse = NO;

	if (du->stmtno <= 0)
	    newblockno = du->stmtno - 1;
	else
	    newblockno = stmttab[du->stmtno].block;

	if (du->isdef)
	    {
	    if (newblockno == blockno)	/* all DU blocks for same block must be
					 * together
					 */
	        {
		du->next = blockweb->webloop.inter.du->next;
		blockweb->webloop.inter.du->next = du;
		blockweb->webloop.inter.du = du;
		adelement(du->defsetno, wb->webloop.inter.reachset,
					     wb->webloop.inter.reachset);
	        }
	    else
		{

		blockno = newblockno;

		/* find this defno in the reach set of another web */
		for (wb = weblist; wb; wb = wb->next)
		    {
		    if (xin(wb->webloop.inter.reachset, du->defsetno))
			{
			du->next = wb->webloop.inter.du->next;
			wb->webloop.inter.du->next = du;
			wb->webloop.inter.du = du;
			blockweb = wb;
			goto nextdu;
			}
		    }

		/* web not found -- make a new one */
		wb = alloc_web();
		wb->webloop.inter.reachset = new_set(maxdefine);
		adelement(du->defsetno, wb->webloop.inter.reachset,
					     wb->webloop.inter.reachset);
		du->next = du;
		wb->webloop.inter.du = du;
		wb->next = weblist;
		wb->isweb = YES;
		wb->var = sp;
		weblist = wb;
		blockweb = wb;
		}
	    }
	else		/* must be use */
	    {
	    isdefiniteuse = du->isdefinite;
	    if (newblockno == blockno)	/* all DU blocks for same block must be
					 * together
					 */
	        {
		du->next = blockweb->webloop.inter.du->next;
		blockweb->webloop.inter.du->next = du;
		blockweb->webloop.inter.du = du;
		if (du->defsetno == 0)	/* use inreach[] set */
		    {
		    intersect(inreach[newblockno], sp->an.defset,
			      mkw_temp_s);
		    if (isemptyset(mkw_temp_s) && isdefiniteuse)
			goto uninitialized;
		    else
		        setunion(mkw_temp_s, blockweb->webloop.inter.reachset);
		    }
		else
		    {
		    adelement(du->defsetno, blockweb->webloop.inter.reachset,
						 blockweb->webloop.inter.reachset);
		    }
		blockweb->hasdefiniteuse |= isdefiniteuse;
	        }
	    else	/* new block */
		{
		blockno = newblockno;

		if (du->defsetno == 0)	/* use inreach[] set */
		    {
		    if (du->stmtno < 0)		/* exit use */
			{
		        if (topregion->exitstmts)
			    {
		            long xblockno;
		            xblockno =
				stmttab[topregion->exitstmts->stmt1].block;
		            intersect(inreach[xblockno], sp->an.defset,
					mkw_temp_s);
			    }
		        else
			    clearset(mkw_temp_s);
			}
		    else			/* normal use */
			{
		        intersect(inreach[stmttab[du->stmtno].block],
				  sp->an.defset, mkw_temp_s);
			}

		    if (isemptyset(mkw_temp_s) && isdefiniteuse)
			goto uninitialized;

		    /* find this reachset in the reach set of another web */
		    for (wb = weblist; wb; wb = wb->next)
		        {
			intersect(mkw_temp_s, wb->webloop.inter.reachset,
				  mkw_temp_t);
			if (! isemptyset(mkw_temp_t))
			    {
			    setunion(mkw_temp_s,wb->webloop.inter.reachset);
			    du->next = wb->webloop.inter.du->next;
			    wb->webloop.inter.du->next = du;
			    wb->webloop.inter.du = du;
			    blockweb = wb;
			    wb->hasdefiniteuse |= isdefiniteuse;
			    goto nextdu;
			    }
		        }

		    /* web not found -- make a new one */
		    wb = alloc_web();
		    wb->webloop.inter.reachset = new_set(maxdefine);
		    setassign(mkw_temp_s, wb->webloop.inter.reachset);
		    du->next = du;
		    wb->webloop.inter.du = du;
		    wb->next = weblist;
		    wb->isweb = YES;
		    wb->var = sp;
		    wb->hasdefiniteuse = isdefiniteuse;
		    weblist = wb;
		    blockweb = wb;
		    }
		else		/* only single def reaches */
		    {
		    /* find this defno in the reach set of another web */
		    for (wb = weblist; wb; wb = wb->next)
		        {
		        if (xin(wb->webloop.inter.reachset, du->defsetno))
			    {
			    du->next = wb->webloop.inter.du->next;
			    wb->webloop.inter.du->next = du;
			    wb->webloop.inter.du = du;
			    wb->hasdefiniteuse |= isdefiniteuse;
			    blockweb = wb;
			    goto nextdu;
			    }
		        }

		    /* web not found -- make a new one */
		    wb = alloc_web();
		    wb->webloop.inter.reachset = new_set(maxdefine);
		    adelement(du->defsetno, wb->webloop.inter.reachset,
					         wb->webloop.inter.reachset);
		    du->next = du;
		    wb->webloop.inter.du = du;
		    wb->next = weblist;
		    wb->isweb = YES;
		    wb->var = sp;
		    wb->hasdefiniteuse = isdefiniteuse;
		    weblist = wb;
		    blockweb = wb;
		    }
		}
	    }
nextdu:
	du = dunext;
        } while (du != duend);

    *pweblist = weblist;

    collapse_webs(pweblist);	/* merge webs with common defs */
    return;

uninitialized:
    *pweblist = NULL;
    uninitialized_var(sp);	/* might not return */

    for (wb = weblist; wb; wb = blockweb)
	{
	blockweb = wb->next;
	discard_web(wb);
	}

}  /* make_webs */

/*****************************************************************************
 *
 *  MAKE_WEBS_AND_LOOPS()
 *
 *  Description:	Create web and loop descriptors for register allocation.
 *			Attach them to the prioritized list for each register
 *			class.
 *
 *  Called by:		register_allocation()  -- in register.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:  none
 *
 *  Globals Referenced:	fpaflag
 *			loop_allocation_flag
 *			maxsymtsz
 *			symtab
 *
 *  Globals Modified:	cgitem
 *
 *  External Calls:	ckalloc()
 *			discard_web()
 *			do_loop_allocation()
 *			find_web_range()
 *			fix_memory_def_use()
 *			init_reg_classes()
 *			make_webs()
 *			regclass_list_insert()
 *
 *****************************************************************************
 */

void make_webs_and_loops()
{
    WEB *weblist;
    REGCLASSLIST *regclass;
    register HASHU *sp;
    register WEB *wb;
    long regtype;
    WEB *wbnext;
    register long i;
    long good_web;

    /* initialize register class data structures */
    init_reg_classes();
    cgitem = (long *) ckalloc(numblocks * sizeof(long));

    mkw_temp_s = new_set(maxdefine);	/* used only by make_webs() */
    mkw_temp_t = new_set(maxdefine);    /* used only by make_webs() */
    fwr_defblocks = new_set(maxnumblocks);  /* used only by find_web_range() */
    fgl_t = new_set(lastregionno + 1);	/* used only by find_good_loops() */
    fgl_badloops = new_set(lastregionno + 1);	/* same */
    fgl_loopsseen = new_set(lastregionno + 1);	/* same */
    flr_defset = new_set(maxdefine);
    flr_reachset = new_set(maxdefine);
    flr_t = new_set(maxdefine);
    flr_blockset = new_set(maxnumblocks);
    flr_reachlive = new_set(maxnumblocks);
    flr_leftovers = new_set(maxnumblocks);
    cwsair_elems = (long *)ckalloc((numblocks + 2) * sizeof(long));

    if (fpa881loopsexist)
	{
	defsinfpa881loop = new_set(maxdefine);
	fpa881blocktset = new_set(maxnumblocks);
	fpa881deftset = new_set(maxdefine);
	fpa881livedefblockset = new_set(maxnumblocks);;
	loop_entry_store_needed =
			(flag *)ckalloc(sizeof(flag) * (lastregionno + 1));
	loop_exit_load_needed =
			(flag *)ckalloc(sizeof(flag) * (lastregionno + 1));
	}

    /* process each symtab entry with DU blocks */
    for (i = 0; i <= lastfilledsym; ++i)
	{
	sp = symtab[i];
	if (sp->a6n.du != NULL)
	    {

	    if (sp->a6n.defset == NULL)
		sp->a6n.defset = new_set(maxdefine);

	    /* determine register type & class */
	    regtype = sp->an.register_type;
	    switch(regtype)
		{
		case REG_A_SAVINGS:
			regclass = regclasslist + ADDRCLASS;
			break;
		case REG_D_SAVINGS:
			regclass = regclasslist + INTCLASS;
			break;
		case REG_881_FLOAT_SAVINGS:
		case REG_881_DOUBLE_SAVINGS:
			regclass = regclasslist + F881CLASS;
			break;
		case REG_FPA_FLOAT_SAVINGS:
		case REG_FPA_DOUBLE_SAVINGS:
			regclass = regclasslist + FPACLASS;
			break;
		}

	    /* ensure all defuses for single statement are all memory or
	     * all register-type
	     */
	    fix_memory_def_use(sp);

	    /* collect def-use blocks into webs */
	   make_webs(sp, &weblist);	    /* many webs per var */

#	ifdef DEBUGGING
	   if (rwdebug > 2)
	    {
		    register WEB *w;
		    fprintf(debugp, "\nAfter make_webs()\n");
		    for (w = weblist; w; w = w->next)
			dump_webloop(w, YES);
		    }
#	endif DEBUGGING

	    /* process each web -- find live range and
	     * insert into prioritized list for regclass.
	     */
	    for (wb = weblist; wb; wb = wbnext)
		{
		wbnext = wb->next;

	        /* find live range, load & store pts, total and adjusted
		 * savings, and interfering regions.
		 */
		good_web = find_web_range(wb,regtype);

#			ifdef DEBUGGING
			if (rwdebug > 2)
			    {
			    fprintf(debugp, "\nAfter find_web_range()\n");
			    dump_webloop(wb, YES);
			    }
#			endif

		if (loop_allocation_flag)
		    ;			/* processing handled in
					 * do_loop_allocation() below.
					 */
		else if ((wb->tot_savings > 0) && good_web && ! wb->isdefsonly)
		    regclass_list_insert(
		     (wb->inFPA881loop ? (regclasslist + F881CLASS) : regclass),
		     wb);
		else
		    discard_web(wb);
		}

	    if (loop_allocation_flag && (weblist != NULL))
		do_loop_allocation(weblist, regtype, regclass);
	     
	    }
	}

    FREEIT(cgitem);
    FREESET(mkw_temp_s);
    FREESET(mkw_temp_t);
    FREESET(fwr_defblocks);
    FREESET(fgl_t);
    FREESET(fgl_badloops);
    FREESET(fgl_loopsseen);
    FREESET(flr_defset);
    FREESET(flr_reachset);
    FREESET(flr_t);
    FREESET(flr_blockset);
    FREESET(flr_reachlive);
    FREESET(flr_leftovers);
    FREEIT(cwsair_elems);

    if (fpa881loopsexist)
	{
	FREESET(defsinfpa881loop);
	FREESET(fpa881deftset);
	FREESET(fpa881blocktset);
	FREESET(fpa881livedefblockset);
	FREEIT(loop_entry_store_needed);
	FREEIT(loop_exit_load_needed);
	}
}  /* make_webs_and_loops */

/*****************************************************************************
 *
 *  MERGE_WEBS()
 *
 *  Description:	subsume second web into first one
 *
 *  Called by:		collapse_webs()
 *
 *  Input Parameters:	wb1 -- first web
 *			wb2 -- second web (subsumed)
 *
 *  Output Parameters:  wb1 -- modified first web
 *
 *  Globals Referenced: none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	setunion()
 *
 *****************************************************************************
 */

LOCAL void merge_webs(wb1, wb2)
WEB *wb1;
WEB *wb2;
{
    register DUBLOCK *du1;	/* next block of DU chain 1 */
    register DUBLOCK *du2;	/* next block of DU chain 2 */
    DUBLOCK *duend1;		/* end of DU chain 1 */
    DUBLOCK *duend2;		/* end of DU chain 2 */
    register DUBLOCK *du;	/* last block of merged chain */
    DUBLOCK *firstdu;		/* first block of merged chain */
    register flag done1;	/* done with chain 1 ? */
    register flag done2;	/* done with chain 2 ? */

    /* merge reachsets */
    setunion(wb2->webloop.inter.reachset, wb1->webloop.inter.reachset);

    /* merge lists of DU blocks -- keeping them in order */
    du1 = wb1->webloop.inter.du->next;
    duend1 = wb1->webloop.inter.du;
    du2 = wb2->webloop.inter.du->next;
    duend2 = wb2->webloop.inter.du;
    du = NULL;
    done1 = done2 = NO;

    do {
	if (done1)
	    {
	    while ( !done2 )
		{
		du->next = du2;
		du = du2;
		if (du2 == duend2)
		    done2 = YES;
		else
		    du2 = du2->next;
		}
	    }
	else if (done2)
	    {
	    while ( !done1 )
		{
		du->next = du1;
		du = du1;
		if (du1 == duend1)
		    done1 = YES;
		else
		    du1 = du1->next;
		}
	    }
	else
	    {
	    if (du1->stmtno < du2->stmtno)
		{
		if (du == NULL)
		    firstdu = du1;
		else
		    du->next = du1;
		du = du1;
		if (du1 == duend1)
		    done1 = YES;
		else
		    du1 = du1->next;
		}
	    else
		{
		if (du == NULL)
		    firstdu = du2;
		else
		    du->next = du2;
		du = du2;
		if (du2 == duend2)
		    done2 = YES;
		else
		    du2 = du2->next;
		}
	    }
    }  while (( !done1 ) || ( !done2 ));
    du->next = firstdu;
    wb1->webloop.inter.du = du;
    wb2->webloop.inter.du = NULL;
}  /* merge_webs */

/*****************************************************************************
 *
 *  PROC_INIT_STORAGE()
 *
 *  Description:	Re-initialize the DUBLOCK, LOADST, PLINK, and WEB
 *			storage allocation structures for the next procedure
 *
 *  Called by:		funcinit() -- c1.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	first_loadst_bank
 *			first_plink_bank
 *			first_web_bank
 *			max_loadst
 *			max_plink
 *			max_web
 *			size_loadst
 *			size_plink
 *			size_web
 *
 *  Globals Modified:	curr_loadst_bank
 *			curr_plink_bank
 *			curr_web_bank
 *			free_loadst_list
 *			free_plink_list
 *			free_web_list
 *			last_loadst
 *			last_plink
 *			last_web
 *			next_loadst
 *			next_plink
 *			next_web
 *
 *  External Calls:	free_linked_list
 *
 *****************************************************************************
 */

void proc_init_storage()
{
    init_du_storage();

    curr_loadst_bank = first_loadst_bank;
    next_loadst = ((char *)curr_loadst_bank) + 4;
    last_loadst = next_loadst + size_loadst * max_loadst;
    free_loadst_list = NULL;
    free_linked_list(*first_loadst_bank);
    *first_loadst_bank = NULL;

    curr_plink_bank = first_plink_bank;
    next_plink = ((char *)curr_plink_bank) + 4;
    last_plink = next_plink + size_plink * max_plink;
    free_plink_list = NULL;
    free_linked_list(*first_plink_bank);
    *first_plink_bank = NULL; 

    curr_web_bank = first_web_bank;
    next_web = ((char *)curr_web_bank) + 4;
    last_web = next_web + size_web * max_web;
    free_web_list = NULL;
    free_linked_list(*first_web_bank);
    *first_web_bank = NULL; 
}  /* proc_init_storage */

/*****************************************************************************
 *
 *  PRUNE_LOOPLIST()
 *
 *  Description:	Discard inappropriate candidates from a web's list
 *			containing itself and enclosed loops.
 *			The criteria is that smaller items (by # stmts)
 *			must have smaller tot_savings, else discard the
 *			larger item.  Also, smaller items must have greater
 *			adj_savings, else discard smaller item.
 *
 *  Called by:		do_loop_allocation()
 *
 *  Input Parameters:	plist -- pointer to list
 *
 *  Output Parameters:	plist -- modified list
 *
 *  Globals Referenced: num_to_region[]
 *
 *  Globals Modified:	none
 *
 *  External Calls:	discard_web()
 *			xin()
 *
 *****************************************************************************
 */

LOCAL void prune_looplist(plist)
WEB **plist;
{
    register WEB *wb;
    register WEB *fwb;		/* follow pointer */
    WEB *wbnext;

    /* list is ordered by decreasing number of statements */
    for (wb = *plist; wb; wb = wbnext)
	{
	wbnext = wb->next;

	/* find preceding item in list which conflicts with this loop */
	for (fwb = wb->prev; fwb; fwb = fwb->prev)
	    {
	    if (fwb->isweb	/* the web conflicts with everything */
	     ||  wb->isweb
	     || xin(num_to_region[fwb->webloop.loop.regionno]->ifset,
		    wb->webloop.loop.regionno))
		break;
	    }

	/* check if one of the items should be removed */
	if (fwb)
	    {
	    /* check second condition -- smaller has adj_savings > larger */
	    if (fwb->adj_savings >= wb->adj_savings)
		{
		wb->prev->next = wbnext;
		if (wbnext)
		    wbnext->prev = wb->prev;
		discard_web(wb);
		}
	    /* check first condition -- smaller has tot_savings < larger */
	    else
		{
		if (fwb->tot_savings <= wb->tot_savings)
		    {
		    /* must remove preceding entry and maybe others before it */
		    WEB *w;
		    flag changing = YES;
		    while (changing == YES)
			{
			changing = NO;

			/* remove entry from list */
			if (fwb->prev)
			    fwb->prev->next = fwb->next;
			else
			    *plist = fwb->next;
			fwb->next->prev = fwb->prev;
			w = fwb->prev;
			discard_web(fwb);

			/* find preceding one with conflict and test it, too */
		        for (fwb = w; fwb; fwb = fwb->prev)
			    {
			    if (fwb->isweb 
			     ||  wb->isweb
			     || xin(num_to_region[fwb->webloop.loop.regionno]->
									  ifset,
				    wb->webloop.loop.regionno))
				{
			        /* check if this one fails condition, too */
			        if (fwb->tot_savings <= wb->tot_savings)
				    changing = YES;
				break;
				}
			    }
			}
		    }
		}
	    }
	}
}  /* prune_looplist */

/*****************************************************************************
 *
 *  REGCLASS_LIST_INSERT()
 *
 *  Description:	Insert web or loop into priority-ordered list of
 *			candidates for given register class.
 *
 *  Called by:		do_loop_allocation()
 *			make_webs_and_loops()
 *
 *  Input Parameters:	pregclass -- register class descriptor
 *			wb -- web or loop descriptor
 *
 *  Output Parameters:	pregclass
 *
 *  Globals Referenced:	none
 *
 *  Globals Modified:	none
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void regclass_list_insert(pregclass, web)
REGCLASSLIST *pregclass;
register WEB *web;
{
    register WEB *fwb;
    register WEB *wb;

    (pregclass->nlist_items)++;		/* increment item count */

    /* list is ordered by decreasing adj_savings.  Find place to insert */
    for (wb = pregclass->list, fwb = NULL; wb; fwb = wb, wb = wb->next)
	{
	if (web->adj_savings > wb->adj_savings)
	    {
	    if (fwb == NULL)
		pregclass->list = web;
	    else
		fwb->next = web;
	    web->next = wb;
	    return;
	    }
	}

    /* tack it onto the end of the list */
    if (fwb == NULL)
	pregclass->list = web;
    else
        fwb->next = web;
    web->next = NULL;
}  /* regclass_list_insert */

/*****************************************************************************
 *
 *  SETUP_FOR_FPA881LOOPS()
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
LOCAL void setup_for_fpa881loops(pcheckforfpa881loops, pregister_type, wb)
long *pcheckforfpa881loops;
long *pregister_type;
WEB  *wb;
{
    long register_type;
    register long i;

    register_type = wb->var->an.register_type;
    *pcheckforfpa881loops = (fpaflag == FFPA)
		&& ((register_type == REG_FPA_FLOAT_SAVINGS)
		 || (register_type == REG_FPA_DOUBLE_SAVINGS));

    if (*pcheckforfpa881loops)
	{
	if (!saw_dragon_access)
#pragma BBA_IGNORE
	  cerror("Unexpected FPA access");
	if (wb->isinterblock)
	    {
	    switch (setcompare(wb->webloop.inter.live_range, fpa881loopblocks))
		{
		case ONE_IS_TWO:
		case ONE_IN_TWO:
			wb->inFPA881loop = YES;
			*pcheckforfpa881loops = NO;
			break;

		case DISJOINT:
			wb->inFPA881loop = NO;
			*pcheckforfpa881loops = NO;
			break;

		case TWO_IN_ONE:
		case OVERLAP:
			wb->inFPA881loop = NO;
			break;

		default:
#pragma BBA_IGNORE
			cerror("impossible setcompare() value in setup_for_fpa881loops()");
		}
	    }
	else  /* intrablock */
	    {
	    /* An intrablock web with a first statement of 0 is an
	     * entry def for a static variable.  It cannot be in a loop. 
             */
	    if ((wb->webloop.intra.first_stmt > 0) &&
                (xin(fpa881loopblocks,
				stmttab[wb->webloop.intra.first_stmt].block)))
		wb->inFPA881loop = YES;
	    else
		wb->inFPA881loop = NO;
	    *pcheckforfpa881loops = NO;
	    }
	}
    else  /* *pcheckforfpa881loops == NO */
	{
	wb->inFPA881loop = NO;
	}

    if (wb->inFPA881loop || *pcheckforfpa881loops)
	*pregister_type = (register_type == REG_FPA_FLOAT_SAVINGS) ?
				REG_881_FLOAT_SAVINGS : REG_881_DOUBLE_SAVINGS;

    /* zero out store_needed and load_needed arrays */
    if (*pcheckforfpa881loops)
	{
	for (i = 0; i <= lastregionno; i++)
	    {
	    loop_entry_store_needed[i] = NO;
	    loop_exit_load_needed[i] = NO;
	    }
	clearset(defsinfpa881loop);
	}
}  /* setup_for_fpa881loops */

/*****************************************************************************
 *
 *  UNINITIALIZED_VAR()
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

void uninitialized_var(sp)
HASHU *sp;
{
    if (sp->a6n.pass1name != NULL)
	{
	if (!sp->a6n.uninit_warned)
	    {
	    werror("Variable \"%s\" uninitialized before use.",
			sp->a6n.pass1name);
	    sp->a6n.uninit_warned = YES;
#ifdef COUNTING
	    _n_uninitialized_vars++;
#endif
	    }
	}
    else
	{
	if (!sp->a6n.uninit_warned)
	    {
	    werror("Location \"%d(%%a6)\", uninitialized before use.",
			sp->a6n.offset);
	    sp->a6n.uninit_warned = YES;
#ifdef COUNTING
	    _n_uninitialized_vars++;
#endif
	    }
	}

}  /* uninitialized_var */

/*****************************************************************************
 *
 *  USE_IN_FPA881LOOP()
 *
 *  Description:	Process a DUBLOCK "use" which occurs in an 
 *			FPA881loop.  Part of the web is outside the loop.
 *			Check if the use references defs in FPA code.  If so,
 *			determine which loop the store should be for and
 *			set the bit.
 *
 *  Called by:		compute_web_load_store_pts()
 *
 *  Input Parameters:	du -- DUBLOCK pointer
 *			wb -- WEB pointer
 *			register_type -- 881 register type
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

LOCAL void use_in_fpa881loop(du, wb, register_type)
DUBLOCK *du;
WEB *wb;
long register_type;
{
    REGION *rp;
    REGION *rptst;
    long blockno;
    long defno;

    du->savings = calculate_savings(register_type, du->d.parent,
				du->ismemory);

    if (mc68040 && fortran && wb->var->an.farg)
      du->savings += 1;

    du->inFPA881loop = YES;

    if (du->defsetno > 0)	/* only def from inside loop reaches */
	return;

    blockno = stmttab[du->stmtno].block;

    intersect(inreach[blockno], wb->var->an.defset, fpa881deftset);
    defno = 0;
    while ((defno = nextel(defno, fpa881deftset)) > 0)
	{
	if (! definetab[defno].inFPA881loop)
	    {
	    /* find largest 881 loop enclosing use, within web */
	    rp = dfo[blockno]->bb.region;
	    while (rp != topregion)
	        {
	        rptst = rp->parent;
	        switch(setcompare(rptst->blocks, wb->webloop.inter.live_range))
		    {
		    case ONE_IN_TWO:
		    case ONE_IS_TWO:
			    ;

		    case TWO_IN_ONE:
		    case OVERLAP:
			    goto exitwhile;

		    default:
#pragma BBA_IGNORE
			cerror("bad setcompare result in use_in_fpa881loop()");
		    }
	        rp = rptst;
	        }
exitwhile:

	    loop_entry_store_needed[rp->regionno] = YES;
	    break;
	    }
	}
}  /* use_in_fpa881loops */
