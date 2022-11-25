/* file register.c */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)register.c	16.1 90/11/05 */


/*****************************************************************************
 *
 *  File register.c contains:
 *	1. The register allocation driver routine, register_allocation()
 *	2. initialization and utility routines
 *	3. reaching defs and live variables data flow info routines
 *	4. region descriptor construction
 *	5. all defines of register allocation data items
 *
 *****************************************************************************
 */


#include "c1.h"

/* functions defined in this file */

LOCAL void compute_live_vars();
      void compute_reaching_defs();
LOCAL void def_var_block_sets();

#ifdef DEBUGGING 
LOCAL void dump_colored_webs_and_regmaps();
LOCAL void dump_def_var_block_sets();
      void dump_definetab();
LOCAL void dump_gen_and_kill();
LOCAL void dump_live_vars();
LOCAL void dump_reaching_defs();
LOCAL void dump_region();
LOCAL void dump_regions();
LOCAL void dump_stmttab();
      void dump_varnos();
      void dump_webloop();
      void dump_webs_and_loops();
      void dump_work_loop_list();
#endif

      void final_reg_cleanup();
LOCAL void finish_region_descriptors();
LOCAL void free_live_and_reaching();
LOCAL void free_reg_data();
LOCAL void make_region();
LOCAL void make_region_descriptors();
LOCAL void make_region_entry_exit();
LOCAL void make_topregion();
LOCAL void make_topregion_entry_exit();
LOCAL void mark_defs_in_fpa881loops();
      void number_vars();
LOCAL void postfix_region_traverse();
      void register_allocation();	/* called by main() in c1.c */

/*****************************************************************************
 *
 * DEFINITIONS
 *
 *****************************************************************************
 */

HASHU	**varno_to_symtab;	/* varno to symtab vector */
ushort	lastvarno;		/* # items for reg alloc purposes */
				/* also, # items in varno_to_symtab */
STMT	*stmttab;		/* statement descriptor array */
long	laststmtno;		/* last entry in stmttab */
long	maxstmtno;		/* allocated size of stmttab */
DEFTAB	*definetab;		/* definition structure */
long	lastdefine;		/* last entry in definetab */
long	maxdefine;		/* allocated size of definetab */
SET	**genreach;		/* reaching defs "gen" sets for each block */
SET	**killreach;		/* reaching defs "kill" sets for each block */
SET	**inreach;		/* reaching defs "in" set for each block */
SET	**outreach;		/* reaching defs "out" set for each block */
SET	**outreachep;		/* reaching defs "out" sets for each ep */
SET	**genlive;		/* live vars "gen" sets for each block */
SET	**killlive;		/* live vars "kill" sets for each block */
SET	**inlive;		/* live vars "in" sets for each block */
SET	**outlive;		/* live vars "out" sets for each block */
SET	*inliveexit;		/* live vars "in" set for EXIT */
SET	**blockreach;		/* blocks for each reaching def in
					which def is in "in" set */
SET	**blocklive;		/* blocks for each live var in which
					var is in "in" set */
REGION	*topregion;		/* top of REGION tree -- whole pgm region */
REGION 	**num_to_region;	/* regionno to REGION map vector */
short	lastregionno;		/* last entry in num_to_region table */
REGCLASSLIST regclasslist[NREGCLASSES];	/* 3 element array of list blocks --
				          1 element each for int, addr, float */
struct reg_type_info reg_types[] =  /* info about D, A, 881, and FPA regs*/
	{	{13,  4 },	/* A regs -- w/o FPA */
		{ 7,  6 },	/* D regs */
		{23,  6 },	/* 881 regs */
		{39, 13 },	/* FPA regs */
	};

long	ncalls;
long	nexprs;
flag	in_reg_alloc_flag;
long	exitblockno;		/* dfo block # of exit block */
flag	pass1_collect_attributes;
void	(*simple_use_proc)();
void	(*simple_def_proc)();
flag	fpa881loopsexist;
REGION  *currregiondesc;
flag	array_forms_fixed;

SET	*fpa881loopblocks;
long	block_not_on_main_path;

long	max_farg_slots;
long	*farg_slots;
flag	saw_dragon_access;	/* Are there FPA ops in this proc? */
flag	saw_global_access;

/*****************************************************************************
 *
 *  COMPUTE_LIVE_VARS()
 *
 *  Description:	Allocate inlive[] and outlive[] arrays
 *			Compute live variables data flow information
 *			Free genlive[] and killlive[] sets
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	dfo[]
 *			genlive[]
 *			inliveexit
 *			killlive[]
 *			lastvarno
 *			numblocks
 *			succset[]
 *
 *  Globals Modified:	genlive[]	-- free'd
 *			inlive[]	-- allocated and filled
 *			killlive[]	-- free'd
 *			outlive[]	-- allocated and filled
 *
 *  External Calls:	FREEIT()
 *			ckalloc()
 *			clearset()
 *			difference()
 *			nextel()
 *			new_set()
 *			new_set_n()
 *			setassign()
 *			setcompare()
 *			setunion()
 *
 *****************************************************************************
 */

LOCAL void compute_live_vars()
{
    long nvars = lastvarno+1;
    register long i;
    flag changing;
    register BBLOCK *bp;	/* current block */
    SET *oldin;
    register long bn;

    /* allocate inlive[] and outlive[] sets */
    inlive = new_set_n(nvars, numblocks);
    outlive = new_set_n(nvars, numblocks);

    oldin = new_set(nvars);

    /* perform the data flow propagation */
    changing = YES;	/* to get us started */
    while (changing)
	{
	changing = NO;
	for (i = numblocks - 1; i >= 0; --i)      /* reverse dfo order */
	    {
	    bp = dfo[i];

	    /* oldin <-- previous value of inlive[] */
	    setassign(inlive[i], oldin);

	    /* if block falls into exit, use inliveexit[] */
	    if (bp->bb.breakop == EXITBRANCH)
		setassign(inliveexit, outlive[i]);
	    else
	        clearset(outlive[i]);

	    /* for all successors of current block, add their inlive[]
	     * sets to the outlive[] set.
	     */
	    bn = -1;
	    while ((bn = nextel(bn, succset[i])) >= 0)
		{
		setunion(inlive[bn], outlive[i]);
		}

	    /* in = gen U (out - kill) */
	    difference(killlive[i], outlive[i], inlive[i]);
	    setunion(genlive[i], inlive[i]);

	    if ( (changing == NO) && differentsets(inlive[i], oldin) )
		changing = YES;
	    }
	}

    FREESET(oldin);

    /* free genlive[] and killlive[] sets */
    FREESET_N(genlive);
    FREESET_N(killlive);
    
}  /* compute_live_vars */

/*****************************************************************************
 *
 *  COMPUTE_REACHING_DEFS()
 *
 *  Description:	Allocate inreach[] and outreach[] arrays
 *			Compute reaching defs data flow information
 *			Free genreach[] and killreach[] sets
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	dfo[]
 *			genreach[]
 *			killreach[]
 *			maxdefine
 *			numblocks
 *			outreachep[]
 *			predset[]
 *
 *  Globals Modified:	genreach[]	-- free'd
 *			inreach[]	-- allocated and filled
 *			killreach[]	-- free'd
 *			outreach[]	-- allocated and filled
 *
 *  External Calls:	ckalloc()
 *			clearset()
 *			difference()
 *			nextel()
 *			new_set()
 *			new_set_n()
 *			setcompare()
 *			setunion()
 *
 *****************************************************************************
 */

void compute_reaching_defs()
{
    register long i;
    flag changing;
    register BBLOCK *bp;	/* current block */
    SET *oldout;
    register long bn;

    /* allocate inreach[] and outreach[] sets */
    inreach = new_set_n(maxdefine, numblocks);
    outreach = new_set_n(maxdefine, numblocks);

    oldout = new_set(maxdefine);

    /* perform the data flow propagation */
    changing = YES;	/* to get us started */
    while (changing)
	{
	changing = NO;
	for (i = 0; i < numblocks; ++i)     /* forward dfo order */
	    {
	    bp = dfo[i];

	    /* oldout <-- previous value of outreach[] */
	    setassign(outreach[i], oldout);

	    clearset(inreach[i]);

	    /* if any entry pts lead to current block, inreach[] is the union
	     * of their outreach[] sets.
	     */
	    if (bp->bb.entries)
		{
		register PLINK *pp;
		for (pp = bp->bb.entries; pp; pp = pp->next)
		    {
		    setunion(outreachep[pp->val], inreach[i]);    
		    }
		}

	    /* for all predecessors of current block, add their outreach[]
	     * sets to the inreach[] set.
	     */
	    bn = -1;
	    while ((bn = nextel(bn, predset[i])) >= 0)
		{
		setunion(outreach[bn], inreach[i]);
		}

	    /* out = gen U (in - kill) */
	    difference(killreach[i], inreach[i], outreach[i]);
	    setunion(genreach[i], outreach[i]);

	    if ( (changing == NO) && differentsets(outreach[i], oldout) )
		changing = YES;
	    }
	}

    FREESET(oldout);

    /* free genreach[] and killreach[] sets */
    FREESET_N(genreach);
    FREESET_N(killreach);
    
}  /* compute_reaching_defs */

/*****************************************************************************
 *
 *  DEF_VAR_BLOCK_SETS()
 *
 *  Description:	Create sets for each def and var showing for which
 *			blocks the defs and vars are in the inreach[] and
 *			inlive[] sets.
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	inlive[]
 *			inreach[]
 *			lastdefine
 *			lastvarno
 *			numblocks
 *
 *  Globals Modified:	blockreach[]	-- allocated and filled
 *			blocklive[]	-- allocated and filled
 *
 *  External Calls:	FREEIT()
 *			adelement()
 *			ckalloc()
 *			new_set_n()
 *			set_elem_vector()
 *
 *****************************************************************************
 */

LOCAL void def_var_block_sets()
{
    register long bn;		/* block number */
    register long dn;		/* define number */
    register long vn;		/* variable number */
    register SET **bl;		/* blocklive vector */
    register SET **br;		/* blockreach vector */
    long *elem_vector;
    register long *curr_elem;

    long ndefs = lastdefine + 1;
    long nvars = lastvarno + 1;

    /* allocate blockreach[] and blocklive[] sets */
    blockreach = br = new_set_n(maxnumblocks, ndefs);
    blocklive = bl = new_set_n(maxnumblocks, nvars);

    /* allocate element number vector */
    bn = (ndefs > nvars) ? ndefs : nvars;
    bn += 2;
    elem_vector = (long *) ckalloc(bn * sizeof(long));

    /* go thru each block's inreach[] and inlive[] set and fill blockreach[]
     * and blocklive[].
     */
    for (bn = numblocks - 1; bn >= 0; --bn)
	{
	set_elem_vector(inreach[bn], elem_vector);
	curr_elem = elem_vector;
	while ((dn = *(curr_elem++)) >= 0)
	    {
	    adelement(bn, *(br + dn), *(br + dn));
	    }
	set_elem_vector(inlive[bn], elem_vector);
	curr_elem = elem_vector;
	while ((vn = *(curr_elem++)) >= 0)
	    {
	    adelement(bn, *(bl + vn), *(bl + vn));
	    }
	}

    FREEIT(elem_vector);
}  /* def_var_block_sets */

#ifdef DEBUGGING

/*****************************************************************************
 *
 *  DUMP_COLORED_WEBS_AND_REGMAPS()
 *  DUMP_DEF_VAR_BLOCK_SETS()
 *  DUMP_DEFINETAB()
 *  DUMP_GEN_AND_KILL()
 *  DUMP_LIVE_VARS()
 *  DUMP_REACHING_DEFS()
 *  DUMP_REGION()
 *  DUMP_REGIONS()
 *  DUMP_REGISTER_MAPS()
 *  DUMP_STMTTAB()
 *  DUMP_VARNOS()
 *  DUMP_WEBS_AND_LOOPS()
 *
 *  Description:  Routines to print debugging information.
 *
 *****************************************************************************
 */

/*******************  DUMP_COLORED_WEBS_AND_REGMAPS  *************************/
LOCAL void dump_colored_webs_and_regmaps()
{
    register long regclass;
    register long i;
    register long j;
    register REGCLASSLIST *plist;
    register WEB *wb;
    register ushort *rg;
    struct colored_var *cv;
    register HASHU *sp;

    fprintf(debugp,"\ndump_colored_webs_and_regmaps() called\n");
    for (regclass = 0; regclass < NREGCLASSES; ++regclass)
	{
	fprintf(debugp, "\n  %s:\n", (regclass == INTCLASS) ? "INTCLASS" :
					(regclass == ADDRCLASS) ? "ADDRCLASS" :
					(regclass == F881CLASS) ? "F881CLASS" :
								  "FPACLASS");
	if ((flibflag == YES)
	 && ((regclass == F881CLASS) || (regclass == FPACLASS)))
	    {
	    fprintf(debugp, "  +M specified\n");
	    }
	plist = regclasslist + regclass;
	if (plist->nregs == 0)
	    continue;
	fprintf(debugp, "\n   WEBS:\n");
	for (i = 0; i < plist->ncolored_items; ++i)
	    {
	    fprintf(debugp, "    [%d]", i);
	    if ((wb = plist->colored_items[i]) != NULL)
		dump_webloop(wb, YES);
	    else
		fprintf(debugp, "    NULL\n");
	    }

	fprintf(debugp, "\n   VARS:\n");
	for (i = 1; i < plist->ncolored_vars; ++i)
	    {
	    fprintf(debugp, "\n    [%d]", i);
	    cv = &(plist->colored_vars[i]);
	    sp = cv->var;
	    switch (sp->an.tag)
		{
		case XN:
		    fprintf(debugp, " var=0x%x XN %s+%d", sp, sp->xn.ap, sp->xn.offset);
		    break;
		case X6N:
		    fprintf(debugp, " var=0x%x X6N %d(%%a6)", sp, sp->x6n.offset);
		    break;
		case AN:
		    fprintf(debugp, " var=0x%x AN %s+%d", sp, sp->an.ap, sp->an.offset);
		    break;
		case A6N:
		    fprintf(debugp, " var=0x%x A6N %d(%%a6)", sp, sp->a6n.offset);
		    break;
		default:
		    cerror("bad symtab tag in dump_colored_webs_and_regmaps()");
		}
	    fprintf(debugp, "  regno=%d\n",cv->regno);
	    fprintf(debugp, "\tvarifset=");
	    printset(cv->varifset);
	    fprintf(debugp, "\tstmtset=");
	    printset(cv->stmtset);
	    }

	/* dump the reg map */
	fprintf(debugp, "\n\t\tReg #'s\n");
	fprintf(debugp, "  Stmt   ");
	for (i = plist->high_regno; i > (plist->high_regno - plist->nregs); --i)
	    {
	    fprintf(debugp, "   %2d", i);
	    }

	rg = plist->regmap;
	for (i = 1; i <= laststmtno; ++i)
	    {
	    fprintf(debugp,"\n  %4d   ", i);
	    for (j = plist->nregs; j > 0; --j, ++rg)
		{
		fprintf(debugp, " %4d", *rg);
		}
	    }
	fprintf(debugp, "\n");
	}
}  /* dump_colored_webs_and_regmaps */



/*******************  DUMP_DEF_VAR_BLOCK_SETS  *******************************/
LOCAL void dump_def_var_block_sets()
{
    /* dump contents of blockreach[] and blocklive[] sets */
    register long dn;		/* def number */
    register long vn;		/* var number */

    fprintf(debugp, "\ndump_def_var_block_sets called\n");
    if (rddebug)		/* reaching defs */
	{
	for (dn = 1; dn <= lastdefine; ++dn)
	    {
	    fprintf(debugp, "    blockreach[%d] = ", dn);
	    printset(blockreach[dn]);
	    }
	}
    if (rldebug)		/* live vars */
	{
	for (vn = 1; vn <= lastvarno; ++vn)
	    {
	    fprintf(debugp, "    blocklive[%d] = ", vn);
	    printset(blocklive[vn]);
	    }
	}
}  /* dump_def_var_block_sets */



/*******************  DUMP_DEFINETAB  ****************************************/
LOCAL void dump_definetab()
{
    /* dump contents of the definetab */
    register long d;
    register HASHU *sp;

    fprintf(debugp, "\ndump_definetab called\n");
    for (d = 1; d <= lastdefine; ++d)
	{
	sp = definetab[d].sym;
	fprintf(debugp,
    "    [%3d] node=0x%.8x stmt=%d sym=0x%.8x[%d] nu=%d del=%d def=%d se=%d\n",
		d, definetab[d].np, definetab[d].stmtno, sp, sp->an.symtabindex,
		definetab[d].nuses, definetab[d].deleted,
		definetab[d].isdefinite, definetab[d].hassideeffects);
	}
}  /* dump_definetab */



/*******************  DUMP_GEN_AND_KILL  *************************************/
LOCAL void dump_gen_and_kill()
{
    /* dump contents of the gen and kill sets */
    register long bn;		/* block number */

    fprintf(debugp, "\ndump_gen_and_kill called\n");
    
    if (rddebug)		/* reaching defs */
	{
	for (bn = 0; bn <= lastep; ++bn)
	    {
	    fprintf(debugp, "    outreachep[%d]  = ", bn);
	    printset(outreachep[bn]);
	    }
	for (bn = 0; bn < numblocks; ++bn)
	    {
	    fprintf(debugp, "    genreach[%d]    = ", bn);
	    printset(genreach[bn]);
	    fprintf(debugp, "    killreach[%d]   = ", bn);
	    printset(killreach[bn]);
	    }
	}
    
    if (rldebug)		/* live vars */
	{
	for (bn = 0; bn < numblocks; ++bn)
	    {
	    fprintf(debugp, "    genlive[%d]    = ", bn);
	    printset(genlive[bn]);
	    fprintf(debugp, "    killlive[%d]   = ", bn);
	    printset(killlive[bn]);
	    }
	fprintf(debugp, "    inliveexit = ");
	printset(inliveexit);
	}
}  /* dump_gen_and_kill */



/*******************  DUMP_LIVE_VARS  ****************************************/
LOCAL void dump_live_vars()
{
    /* dump live variable DFA inlive[] and outlive[] sets */
    register long bn;

    fprintf(debugp, "\ndump_live_vars called\n");
    for (bn = 0; bn < numblocks; ++bn)
	{
	fprintf(debugp, "inlive[%d]  = ", bn);
	printset(inlive[bn]);
	fprintf(debugp, "outlive[%d] = ", bn);
	printset(outlive[bn]);
	}
}  /* dump_live_vars */



/*******************  DUMP_REACHING_DEFS  ************************************/
LOCAL void dump_reaching_defs()
{
    /* dump reaching defs DFA inreach[] and outreach[] sets */
    register long bn;

    fprintf(debugp, "\ndump_reaching_defs called\n");
    for (bn = 0; bn < numblocks; ++bn)
	{
	fprintf(debugp, "inreach[%d]  = ", bn);
	printset(inreach[bn]);
	fprintf(debugp, "outreach[%d] = ", bn);
	printset(outreach[bn]);
	}
}  /* dump_reaching_defs */



/*******************  DUMP_REGION  *******************************************/
LOCAL void dump_region(rp)
register REGION *rp;
{
    /* dump contents of region descriptor */

    register PLINK *pp;
    register LOADST *sp;

    fprintf(debugp,"\t%d)  0x%.8x  p=%d  s=%d  c=%d  nstmts=%d  nest=%d\n",
	    rp->regionno,
	    rp,
	    rp->parent ? rp->parent->regionno : -1,
	    rp->sibling ? rp->sibling->regionno : -1,
	    rp->child ? rp->child->regionno : -1,
	    rp->nstmts,
	    rp->nestdepth);
    fprintf(debugp,"\t    n881ops=%d  nFPAops=%d  isFPA881loop=%s\n",
		rp->n881ops, rp->nFPAops, (rp->isFPA881loop ? "YES" : "NO"));
    fprintf(debugp,"\t    entry=");
    for (pp = rp->entryblocks; pp; pp = pp->next)
	fprintf(debugp, " %d", pp->val);
    fprintf(debugp,";  exit=");
    for (sp = rp->exitstmts; sp; sp = sp->next)
	fprintf(debugp, " (%d,%d)", sp->stmt1, sp->stmt2);
    fprintf(debugp,"\n");
    fprintf(debugp,"\t    blocks      = ");
    printset(rp->blocks);
    fprintf(debugp,"\t    onlyblocks  = ");
    printset(rp->onlyblocks);
    fprintf(debugp,"\t    containedin = ");
    printset(rp->containedin);
    fprintf(debugp,"\t    ifset       = ");
    printset(rp->ifset);
}  /* dump region */



/*******************  DUMP_REGIONS  ******************************************/
LOCAL void dump_regions()
{
    /* dump region descriptor tree */
    register long rn;

    fprintf(debugp, "\ndump_regions called\n");
    for (rn = 0; rn <= lastregionno; ++rn)
	{
	fprintf(debugp, "    num_to_region[%d] = 0x%.8x\n", rn,
		num_to_region[rn]);
	}
    fprintf(debugp, "    topregion = 0x%.8x\n", topregion);
    fprintf(debugp, "    Regions:\n");
    for (rn = 0; rn <= lastregionno; ++rn)
	{
	dump_region(num_to_region[rn]);
	}

}  /* dump_regions */



/*******************  DUMP_STMTTAB  ******************************************/
LOCAL void dump_stmttab()
{
    /* dump contents of the stmttab array */
    
    register long i;
    register STMT *sp;

    fprintf(debugp, "\ndump_stmttab called\n");
    for (i = 0; i <= laststmtno; ++i)
	{
	sp = stmttab + i;
	fprintf(debugp,"    [%d]  node=0x%.8x  block=%d  sideeffect=%d\n",
		i, sp->np, sp->block, sp->hassideeffects);
	}
}  /* dump_stmttab */


/*******************  DUMP_VARNOS  *******************************************/
LOCAL void dump_varnos()
{
    /* dump contents of the varno_to_symtab array */
    
    register long v;
    register HASHU *sp;

    fprintf(debugp, "\ndump_varnos called\n");
    for (v = 1; v <= lastvarno; ++v)
	{
	sp = varno_to_symtab[v];
	fprintf(debugp,"    varno_to_symtab[%d] = 0x%.8x (%d)",
		v, sp, sp->an.symtabindex);
	if (sp->an.pass1name)
	    fprintf(debugp," \"%s\"\n", sp->an.pass1name);
	else
	    fprintf(debugp,"\n");
	}
}  /* dump_varnos */



/*******************  DUMP_WEBLOOP  ******************************************/
void dump_webloop(wb, dodublocks)
register WEB *wb;
long dodublocks;
{
    HASHU *sp;

    fprintf(debugp, "    0x%x) %s", wb,
			wb->isweb ? (wb->isinterblock ? "WE" : "WA") : "L");
    sp = wb->var;
    switch (sp->an.tag)
	{
	case XN:
	    fprintf(debugp, " var=0x%x XN %s+%d", sp, sp->xn.ap, sp->xn.offset);
	    break;
	case X6N:
	    fprintf(debugp, " var=0x%x X6N %d(%%a6)", sp, sp->x6n.offset);
	    break;
	case AN:
	    fprintf(debugp, " var=0x%x AN %s+%d", sp, sp->an.ap, sp->an.offset);
	    break;
	case A6N:
	    fprintf(debugp, " var=0x%x A6N %d(%%a6)", sp, sp->a6n.offset);
	    break;
	default:
	    cerror("bad symtab tag in dump_webloop()");
	}
    fprintf(debugp, " totsav=%d adjsav=%d\n", wb->tot_savings, wb->adj_savings);
    fprintf(debugp, "\tnext=0x%x prev=0x%x regno=%d\n",
		    wb->next, wb->prev, wb->regno);
    if (wb->s.loops)
	{
        fprintf(debugp, "\tloops/if_neighbors= ");
	printset(wb->s.loops);
	}
    if (dodublocks)
	{
        register LOADST *lp;
        fprintf(debugp, "\tload_store=");
        for (lp = wb->load_store; lp; lp = lp->next)
	    {
	    fprintf(debugp, " (%c %d %d)", (lp->isload? 'L':'S'), lp->stmt1,
		    lp->stmt2);
	    }
	}
    fprintf(debugp, "\n");
    if (wb->isweb)
	{
	if (dodublocks)
	    {
	    register DUBLOCK *dp;
	    register DUBLOCK *dpend;
	    fprintf(debugp, "\tdublocks:\n");
	    dp = dpend = wb->webloop.inter.du->next;
	    do  {
		dumpdublock(dp);
		dp = dp->next;
		} while (dp != dpend);
	    }
	if (wb->webloop.inter.if_regions)
	    {
	    fprintf(debugp, "\tif_regions=");
	    printset(wb->webloop.inter.if_regions);
	    }
	if (wb->webloop.inter.reachset)
	    {
	    fprintf(debugp, "\treachset=");
	    printset(wb->webloop.inter.reachset);
	    }
	if (wb->isinterblock)
	    {
	    fprintf(debugp, "\tnstmts=%d live_range=",
			    wb->webloop.inter.nstmts);
	    if (wb->webloop.inter.live_range)
		printset(wb->webloop.inter.live_range);
	    else
		fprintf(debugp, "\n");
	    }
	else
	    {
	    fprintf(debugp, "\tfirst_stmt=%d last_stmt=%d\n",
		    wb->webloop.intra.first_stmt, wb->webloop.intra.last_stmt);
	    }
	}
    else
	{
	fprintf(debugp, "\tweb=0x%x regionno=%d\n", wb->webloop.loop.web,
		wb->webloop.loop.regionno);
	}
}  /* dump_webloop */



/*******************  DUMP_WEBS_AND_LOOPS  ***********************************/
void dump_webs_and_loops()
{
    long i;
    register WEB *wb;

    fprintf(debugp,"\ndump_webs_and_loops() called\n");
    /* for each register class, dump the web/loops in priority order */
    for (i = 0; i < NREGCLASSES; ++i)
	{
	fprintf(debugp, "%s\n", (i == INTCLASS ? "INTCLASS" :
			        (i == ADDRCLASS ? "ADDRCLASS" :
			        (i == F881CLASS ? "F881CLASS" : "FPACLASS"))));
	for (wb = regclasslist[i].list; wb; wb = wb->next)
	    {
	    dump_webloop(wb, YES);
	    }
	}
}  /* dump_webs_and_loops */



/*******************  DUMP_WORK_LOOP_LIST  ***********************************/
void dump_work_loop_list(web, looplist)
WEB *web;
WEB *looplist;
{
    register WEB *wb;

    fprintf(debugp,"\ndump_work_loop_list() called\n");
    if (web != (WEB*) 0)
	{
	fprintf(debugp,"WEB=\n");
	dump_webloop(web, NO);
	}
    for (wb = looplist; wb; wb = wb->next)
	{
	dump_webloop(wb, YES);
	}
}  /* dump_webs_and_loops */
#endif

/*****************************************************************************
 *
 *  FINAL_REG_CLEANUP()
 *
 *  Description:	Final cleanup of all register allocation data
 *			structures.  Free them.
 *
 *  Called by:		main()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	regclasslist
 *
 *  Globals Modified:	none
 *
 *  External Calls:	FREEIT()
 *
 *****************************************************************************
 */
void final_reg_cleanup()
{
    register long regclass;
    register long i;
    register struct colored_var *pcv;

    for (regclass = 0; regclass < NREGCLASSES; ++regclass)
	{
	if ((flibflag && ((regclass == F881CLASS) || (regclass == FPACLASS)))
	 || (regclasslist[regclass].nregs == 0))
	    continue;
	FREEIT(regclasslist[regclass].colored_items);
	for (i = regclasslist[regclass].ncolored_vars - 1,
		pcv = &(regclasslist[regclass].colored_vars[i]);
	     i > 0; --i, --pcv)
	    {
	    if (pcv->varifset)
		FREESET(pcv->varifset);
	    if (pcv->stmtset)
		FREESET(pcv->stmtset);
	    }
	FREEIT(regclasslist[regclass].colored_vars);
	FREEIT(regclasslist[regclass].regmap);
	}
}  /* final_reg_cleanup */

/*****************************************************************************
 *
 *  FINISH_REGION_DESCRIPTORS()
 *
 *  Description:	For each region, find which regions it interferes with
 *			and create a region set showing this relationship.
 *			Also show which regions a region is contained within.
 *
 *  Called by:		register_allocation()
 *			finish_region_descriptors()  -- recursive call
 *
 *  Input Parameters:	rp -- region descriptor
 *
 *  Output Parameters:	rp -- modified region descriptor
 *
 *  Globals Referenced:	lastregionno
 *
 *  Globals Modified:	none
 *
 *  External Calls:	adelement()
 *			finish_region_descriptors()
 *			make_region_entry_exit()
 *			make_topregion_entry_exit()
 *			new_set()
 *			nextel()
 *			setassign()
 *			setunion()
 *
 *****************************************************************************
 */

LOCAL void finish_region_descriptors(rp)
REGION *rp;			/* current region */
{
    register REGION *cp;	/* child region */
    SET *ifset;
    SET *containedin;
    long nchildstmts;
    long bn;			/* block number */
    flag fpaloopsinchildren = NO;

    if (rp == topregion)
	{
	make_topregion_entry_exit(topregion);
	}
    else
	{
	make_region_entry_exit(rp);
	}

    ifset = new_set(lastregionno + 1);
    containedin = new_set(lastregionno + 1);
	
    /* find containedin set */
    if (rp->parent)
	setassign(rp->parent->containedin, containedin);
    adelement(rp->regionno, containedin, containedin);
    rp->containedin = containedin;

    nchildstmts = 0;

    /* create interference set */
    if (rp->child)
	{
	for (cp = rp->child; cp; cp = cp->sibling)
	    {
	    finish_region_descriptors(cp);
	    setunion(cp->ifset, ifset);
	    nchildstmts += cp->nstmts;
	    if (! cp->isFPA881loop)
		fpaloopsinchildren = YES;
	    }
	}
    else	/* no children */
	{
	setassign(containedin, ifset);
	}
    rp->ifset = ifset;

    rp->isFPA881loop = ((fpaflag == FFPA) && saw_dragon_access) 
                && (rp->n881ops > 0) && (rp->nFPAops < (rp->n881ops << 1))
		&& ((rp->child == NULL) || (fpaloopsinchildren == NO));

    if (rp->isFPA881loop)
	{
	setunion(rp->blocks, fpa881loopblocks);
	fpa881loopsexist = YES;
#ifdef COUNTING
	_n_FPA881_loops++;
#endif
	}

    /* process each block in only this loop */
    bn = -1;
    while ((bn = nextel(bn, rp->onlyblocks)) >= 0)
	{
	register BBLOCK *bp;

	bp = dfo[bn];
	bp->bb.region = rp;		/* map block to this region */
	nchildstmts += bp->bb.nstmts;	/* add statements */
	}

    rp->nstmts = nchildstmts;		/* set nstmts field */

}  /* finish_region_descriptors */

/*****************************************************************************
 *
 *  FREE_LIVE_AND_REACHING()
 *
 *  Description:	Free live vars and reaching defs data structures
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastdefine
 *			lastep
 *			lastvarno
 *			numblocks
 *
 *  Globals Modified:	blocklive[]
 *			blockreach[]
 *			inlive[]
 *			inliveexit
 *			inreach[]
 *			outlive[]
 *			outreach[]
 *			outreachep[]
 *
 *  External Calls:	FREEIT()
 *
 *****************************************************************************
 */

LOCAL void free_live_and_reaching()
{
    /* free inreach, outreach, inlive and outlive sets */
    FREESET_N(inreach);
    FREESET_N(outreach);
    FREESET_N(inlive);
    FREESET_N(outlive);

    /* free inliveexit and outreachep[] sets */
    FREESET(inliveexit);
    FREESET_N(outreachep);

    /* free blockreach sets */
    FREESET_N(blockreach);

    /* free blocklive sets */
    FREESET_N(blocklive);
}  /* free_live_and_reaching */

/*****************************************************************************
 *
 *  FREE_REG_DATA()
 *
 *  Description:	Free all data structures used by the register
 *			allocation phase.  Free global optimization data
 *			structures.  Cleanup.  Go home.
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastfilledsym
 *			lastregionno
 *			num_to_region[]
 *			symtab
 *			topregion
 *
 *  Globals Modified:	definetab[]		-- free'd
 *			fpa881loopblocks	-- free'd
 *			num_to_region[]		-- free'd
 *			stmttab[]		-- free'd
 *			topregion		-- free'd
 *			varno_to_symtab[]	-- free'd
 *
 *  External Calls:	FREEIT()
 *
 *****************************************************************************
 */

LOCAL void free_reg_data()
{
    register long i;
    register REGION *rp;

    /* free stmttab, definetab, varno_to_symtab */
    FREEIT(stmttab);
    FREEIT(definetab);
    FREEIT(varno_to_symtab);

    /* free region descriptor tree */
    for (i = 0; i <= lastregionno; ++i)
	{
	rp = num_to_region[i];
	
	/* don't free rp->blocks because it is in "region" and was free'd in
	 * free_global_data()
	 */

	FREESET(rp->onlyblocks);
	FREESET(rp->containedin);
	FREESET(rp->ifset);

	/* next procedure initialization will clean up the plinks and loadsts */

	}
    /* Topregion isn't considered in free_global_data(). */
    FREESET(topregion->blocks);
    FREEIT(topregion);		/* free all region descriptors */
    FREEIT(num_to_region);

    for (i = 0; i <= lastfilledsym; ++i)
	{
	if (symtab[i]->an.defset != NULL)
	    {
	    FREESET(symtab[i]->an.defset);
	    symtab[i]->an.defset = NULL;
	    symtab[i]->an.du = NULL;
	    }
	}

    FREESET(fpa881loopblocks);

}  /* free_reg_data */

/*****************************************************************************
 *
 *  MAKE_REGION()
 *
 *  Description:	Fill a region descriptor with entry and exit block
 *			information about a region.  Place the region
 *			descriptor in the proper place in the region tree.
 *
 *  Called by:		make_region_descriptors()
 *
 *  Input Parameters:
 *			bset -- set of blocks in region
 *			rp   -- region descriptor
 *
 *  Output Parameters:
 *			rp   -- modified region descriptor
 *
 *  Globals Referenced:	topregion
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *			setcompare()
 *
 *****************************************************************************
 */

LOCAL void make_region(bset, regionnum, rp)
SET *bset;
long regionnum;
register REGION *rp;		/* current region descriptor */
{
    register REGION *cp;	/* child pointer */
    register REGION *fp;	/* "follow" pointer */
    register REGION *tp;	/* parent pointer */
    REGION *ccp;

    /* put the blockset in the descriptor */
    rp->blocks = bset;


    if (needregioninfo)
	rp->regioninfo = NULL;
    else
	rp->regioninfo = &(regioninfo[regionnum]);

    /* put the descriptor in the proper place in the region tree.  Compare
     * blocksets with regions already in the tree.
     */
    tp = topregion;
    rp->parent = NULL;
    while (rp->parent == NULL)
	{

	for (cp = tp->child, fp = NULL; cp && (rp->parent == NULL);
	     fp = cp, cp = cp->sibling)
	    {
	    switch (setcompare(rp->blocks, cp->blocks))
		{
		case DISJOINT:
				break;
		case ONE_IN_TWO:
				tp = cp;
				goto newparent;

		case TWO_IN_ONE:
				rp->sibling = cp->sibling;
				cp->sibling = NULL;
				if (fp)
				    fp->sibling = rp;
				else
				    tp->child = rp;
				rp->child = cp;
				cp->parent = rp;
				rp->parent = tp;
			
				/* check remaining children of parent to
				 * see if they're in this loop, too
				 */
				for (ccp = rp->sibling, fp = rp; ccp; )
				    {
				    if (setcompare(rp->blocks, ccp->blocks) 
					  == TWO_IN_ONE)
					{
					fp->sibling = ccp->sibling;
					ccp->sibling = rp->child;
					rp->child = ccp;
					ccp->parent = rp;
					ccp = fp->sibling;
					}
				    else
					{
					fp = ccp;
					ccp = ccp->sibling;
					}
				    }
				break;

		default:
#pragma BBA_IGNORE
				cerror("invalid regions in make_region()");
		}
	    }
	if (rp->parent == NULL)
	    {
	    if (fp == NULL)
		tp->child = rp;
	    else
		fp->sibling = rp;
	    rp->parent = tp;
	    rp->sibling = NULL;
	    rp->child = NULL;
	    }
newparent:
	continue;
	}

    rp->n881ops = 0;
    rp->nFPAops = 0;
}  /* make_region */

/*****************************************************************************
 *
 *  MAKE_REGION_DESCRIPTORS()
 *
 *  Description:	Create region descriptor tree which illustrates:
 *			  1. Which loops are contained in other loops
 *			  2. Which blocks in a loop are not in any nested loops
 *			  3. Region entry and exit blocks
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	lastregion
 *			maxnumblocks
 *			region
 *
 *  Globals Modified:	fpa881loopblocks
 *			fpa881loopsexist
 *			lastregionno
 *			num_to_region	-- allocated and filled
 *			topregion
 *
 *  External Calls:	ckalloc()
 *			make_region()
 *			make_topregion()
 *			new_set()
 *			postfix_region_traverse()
 *
 *****************************************************************************
 */

LOCAL void make_region_descriptors()
{
    register SET **rp;		/* current region pointer */
    REGION *region_desc;	/* region descriptors (allocated all at once) */
    short region_number;
    ushort nregions;		/* # of regions (include entire pgm) */
    long rinfonumber;

    /* Count regions and allocate descriptors and map vector */
    nregions = lastregion - region + 2;	   /* regions + whole program */
    region_desc = (REGION *) ckalloc(nregions * sizeof(REGION));
    num_to_region = (REGION **) ckalloc(nregions * sizeof(REGION *));

    /* Create region descriptor for entire program */
    topregion = region_desc++;
    make_topregion(topregion);

    /* Create region descriptor tree for loops in outer->inner order */
    for (rp = lastregion, rinfonumber = lastregioninfo; rp >= region;
		--rp, --rinfonumber)
	{
	if (*rp)
		make_region(*rp, rinfonumber, region_desc++);
	}

    /* Traverse the region tree in postfix order, numbering the regions and
     * filling num_to_region[]
     */
    region_number = -1;
    postfix_region_traverse(topregion, 0, &region_number);
    lastregionno = region_number;

    fpa881loopsexist = NO;			/* none seen yet */
    fpa881loopblocks = new_set(maxnumblocks);	/* filled in by
						 * finish_region_descriptors()
						 */

}  /* make_region_descriptors */

/*****************************************************************************
 *
 *  MAKE_REGION_ENTRY_EXIT()
 *
 *  Description:	Add entry and exit block lists to the region descriptor
 *
 *  Called by:		make_region()
 *
 *  Input Parameters:	rp -- region descriptor
 *
 *  Output Parameters:	rp -- region descriptor with entry, exit blocks/stmts
 *				added
 *
 *  Globals Referenced:	dfo[]
 *			dom[]
 *			maxnumblocks
 *
 *  Globals Modified:	none
 *
 *  External Calls:	cerror()
 *			ckalloc()
 *			intersect()
 *			nextel()
 *			new_set()
 *			setassign()
 *			xin()
 *
 *****************************************************************************
 */

LOCAL void make_region_entry_exit(rp)
REGION *rp;
{
    register long i;
    long bn;			/* block number */
    SET *loopdoms;
    register LOADST *lp;
    register LOADST *llp;
    long stmt1;
    long stmt2;

    /* Find the inner dominator of the region -- this is the sole entry block.
     * While we're searching all the region's blocks, look for jumps outside
     * the loop -- these blocks are exit blocks.
     */
    loopdoms = new_set(maxnumblocks);
    setassign(rp->blocks,loopdoms);	/* loopdoms = all blocks in loop */
    rp->exitstmts = NULL;

    bn = -1;
    llp = NULL;
    while ((bn = nextel(bn, rp->blocks)) >= 0)
	{
        register BBLOCK *bp;

	bp = dfo[bn];
	if (bp->bb.deleted)
	    continue;

	intersect(dom[bn], loopdoms, loopdoms);  /* find least common dominator
						  * of all blocks in loop */
	stmt1 = bp->bb.firststmt + bp->bb.nstmts - 1;
	switch (bp->bb.breakop)			/* how does block end? */
	    {
	    case LGOTO:	/* check single target block */
			if (!xin(rp->blocks,bp->bb.lbp->bb.node_position))
			    {
			    stmt2 = bp->bb.lbp->bb.firststmt;
			    goto add_exit;
			    }
			break;
	    case CBRANCH:	/* check both branch and fall-thru blocks */
			if (!xin(rp->blocks,bp->bb.lbp->bb.node_position))
			    {
			    stmt2 = bp->bb.lbp->bb.firststmt;
			    lp = alloc_loadst();
			    lp->stmt1 = stmt1;
			    lp->stmt2 = stmt2;
			    if (llp)
				llp->next = lp;
			    else
				rp->exitstmts = lp;
			    llp = lp;
			    }
			if (!xin(rp->blocks,bp->bb.rbp->bb.node_position))
			    {
			    stmt2 = bp->bb.rbp->bb.firststmt;
			    goto add_exit;
			    }
			break;
	    case FCOMPGOTO:	/* check every target block */
	    case GOTO:		/* ASSIGNED GOTO */
			{
		        register CGU *cgup;
			cgup = bp->cg.ll;
			for (i = bp->cg.nlabs - 1; i >= 0; --i)
			    {
			    if (!xin(rp->blocks,
				     cgup[i].lp->bp->bb.node_position))
				{
				stmt2 = cgup[i].lp->bp->bb.firststmt;
			        lp = alloc_loadst();
				lp->stmt1 = stmt1;
				lp->stmt2 = stmt2;
				if (llp)
				    llp->next = lp;
				else
				    rp->exitstmts = lp;
				llp = lp;
				}
			    }
			}
			break;
	    case FREE:		/* check single successor block */
			i = nextel(-1, succset[bn]);
			bp = dfo[i];
			if (!xin(rp->blocks,i))
			    {
			    stmt2 = bp->bb.firststmt;
			    goto add_exit;
			    }
			break;
	    case EXITBRANCH:		/* always jumps outside loop */
			stmt2 = 0;
add_exit:
			lp = alloc_loadst();
			lp->stmt1 = stmt1;
			lp->stmt2 = stmt2;
			if (llp)
			    llp->next = lp;
			else
			    rp->exitstmts = lp;
			llp = lp;
			break;
	    }
	}
    if (llp)
        llp->next = NULL;

    if ((bn = nextel(-1, loopdoms)) < 0)
#pragma BBA_IGNORE
	cerror("region has no dominator in make_region()");

    {
    register PLINK *pp;

    pp = alloc_plink();
    pp->val = bn;
    pp->next = NULL;
    rp->entryblocks = pp;
    }

    FREESET(loopdoms);
}  /* make_region_entry_exit */

/*****************************************************************************
 *
 *  MAKE_TOPREGION()
 *
 *  Description:	Create the region descriptor for the entire program.
 *
 *  Called by:		make_region_descriptors()
 *
 *  Input Parameters:	topregion -- region descriptor for topregion
 *
 *  Output Parameters:	topregion -- filled topregion descriptor
 *
 *  Globals Referenced:	dfo[]
 *			ep[]
 *			lastep
 *			maxnumblocks
 *			numblocks
 *
 *  Globals Modified:	none
 *
 *  External Calls:	addsetrange()
 *			ckalloc()
 *			new_set()
 *
 *****************************************************************************
 */

LOCAL void make_topregion(topregion)
register REGION *topregion;
{

    /* whole program contains all blocks */
    topregion->blocks = new_set(maxnumblocks);
    addsetrange(0, numblocks-1, topregion->blocks);

    topregion->regioninfo = NULL;
    topregion->niterations = 1;
    topregion->parent = NULL;
    topregion->child = NULL;
    topregion->sibling = NULL;
    topregion->n881ops = 0;
    topregion->nFPAops = 0;
}  /* make_topregion */

/*****************************************************************************
 *
 *  MAKE_TOPREGION_ENTRY_EXIT()
 *
 *  Description:	Create entry/exit point lists for the whole program
 *			region descriptor.
 *
 *  Called by:		finish_region_descriptors()
 *
 *  Input Parameters:	topregion -- region descriptor for topregion
 *
 *  Output Parameters:	topregion -- filled topregion descriptor
 *
 *  Globals Referenced:	dfo[]
 *			ep[]
 *			lastep
 *			maxnumblocks
 *			numblocks
 *
 *  Globals Modified:	none
 *
 *  External Calls:	alloc_plink()
 *			alloc_loadst()
 *
 *****************************************************************************
 */

LOCAL void make_topregion_entry_exit(topregion)
register REGION *topregion;
{
    register long i;		/* index */

    /* add all entry point blocks to entry list for whole program */
    {
    register PLINK *pp;		/* entry list item pointer */
    register PLINK *ppl;	/* entry list item pointer */
    for (i = 0, ppl = NULL; i <= lastep; i++)
	{
	pp = alloc_plink();
	pp->val = ep[i].b.bp->bb.node_position;
	if (ppl)
	    ppl->next = pp;
	else
	    topregion->entryblocks = pp;
	ppl = pp;
	} 
    ppl->next = NULL;
    }

    /* add all exit point blocks to exit list for whole program */
    {
    register LOADST *pp;		/* exit list item pointer */
    register LOADST *ppl;		/* exit list item pointer */
    for (i = 0, ppl = NULL; i < numblocks; i++)
	{
	if (dfo[i]->bb.breakop == EXITBRANCH)
	    {
	    pp = alloc_loadst();
	    pp->stmt1 = dfo[i]->bb.firststmt + dfo[i]->bb.nstmts - 1;
	    pp->stmt2 = 0;
	    if (ppl)
	        ppl->next = pp;
	    else
	        topregion->exitstmts = pp;
	    ppl = pp;
	    }
	} 
    if (ppl)
        ppl->next = NULL;
    else
	topregion->exitstmts = NULL;
    }
}  /* make_topregion_entry_exit */

/*****************************************************************************
 *
 *  MARK_DEFS_IN_FPA881LOOPS()
 *
 *  Description:	Go through definetab, setting the "inFPA881loop" bit
 *			for each define in an fpa881loop.
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	definetab[]
 *			fpa881loopblocks
 *			lastdefine
 *			stmttab[]
 *
 *  Globals Modified:	definetab[]
 *
 *  External Calls:	none
 *
 *****************************************************************************
 */

LOCAL void mark_defs_in_fpa881loops()
{
    register long i;
    register long stmtno;
    for (i = 1; i <= lastdefine; ++i)
	{
	stmtno = definetab[i].stmtno;
	if ((stmtno > 0) && xin(fpa881loopblocks, stmttab[stmtno].block))
	    definetab[i].inFPA881loop = YES;
	else
	    definetab[i].inFPA881loop = NO;
	}
}  /* mark_defs_in_fpa881loops */

/*****************************************************************************
 *
 *  NUMBER_VARS()
 *
 *  Description:	Assign unique numbers to register allocation
 *			candidates.  Create varno_to_symtab mapping.
 *			Compute register type and store in symtab entry.
 *
 *  Called by:		register_allocation()
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	flibflag
 *			in_reg_alloc_flag
 *			lastfilledsym
 *			maxsymtsz
 *			symtab
 *
 *  Globals Modified:	lastvarno
 *			varno_to_symtab[]	-- allocated and filled
 *
 *  External Calls:	ckalloc()
 *
 *****************************************************************************
 */

void number_vars()
{
    register HASHU *sp;		/* pointer to current symtab entry */
    register long i;

    /* allocate varno_to_symtab vector with <symtabsize> elements */
    varno_to_symtab = (HASHU **) ckalloc((lastfilledsym+2) * sizeof(HASHU*));

    lastvarno = 0;    /* start at 1; varno == 0 means not reg allocatable */

    /* go thru entire symtab, setting varno for items meeting criteria */
    for (i = 0; i <= lastfilledsym; ++i)
	{
	sp = symtab[i];
	switch (sp->an.tag)
	    {
	    case CN:	/* COMMON */
	    default:
			sp->cn.varno = 0;	/* not eligible */
			break;

	    case S6N:	/* dynamic structure */
	    case SN:	/* static structure */

			if (!sp->sn.seenininput		/* not seen in code */
			 || !ISARY(sp->sn.type)
			 || sp->sn.equiv		/* EQUIVALENCE'd var */
			 || sp->sn.isstruct		/* structure field */
			 || !in_reg_alloc_flag)  /* or in dead store */
			    {
			    sp->sn.varno = 0;
			    break;
			    }
			else
			    goto numbervar;

	    case AN:
			if ((sp->an.ap == NULL)		/* simple constant */
			 || !sp->an.seenininput		/* not seen in code */
			 || (fortran && (BTYPE(sp->an.type) == UCHAR))
							/* CHAR* var */
			 || sp->an.equiv		/* EQUIVALENCE'd var */
			 || sp->an.func			/* function var */
			 || sp->an.isstruct		/* structure field */
			    /* +M flag and real type */
			 || (flibflag && ((sp->an.type.base == FLOAT)
				       || (sp->an.type.base == DOUBLE)))
			 || (sp->an.arrayelem && (!fortran  /* C array elem */
				|| !in_reg_alloc_flag))  /* or in dead store */
			 || (sp->an.type.base == LONGDOUBLE)
			 || (sp->an.isconst
			  && !((sp->an.type.base == FLOAT)
			     || (sp->an.type.base == DOUBLE)))
			   )
			    {
			    sp->an.varno = 0;
			    break;
			    }
			else
			    goto numbervar;

	    case A6N:
			if (sp->a6n.equiv		/* EQUIVALENCE'd var */
			 || sp->a6n.func		/* function var */
			 || !sp->an.seenininput		/* not seen in code */
			 || (fortran && (BTYPE(sp->a6n.type) == UCHAR))
							/* CHAR* var */
			 || sp->a6n.isstruct		/* structure field */
			 || (fortran && (sp->a6n.offset >= 0) && lastep > 0)
					/* FORTRAN actual arg before copy */
			    /* +M flag & real type, but not a pointer */
			 || (flibflag && !sp->a6n.ptr
			  && ((BTYPE(sp->a6n.type) == FLOAT)
			   || (BTYPE(sp->a6n.type) == DOUBLE)))
			 || (sp->a6n.farg		/* COMPLEX formal arg */
			  && (sp->a6n.complex1 || sp->a6n.complex2))
			 || ((sp->a6n.farg || sp->a6n.hiddenfarg)
			  && (lastep > 0))	/* formal args when ENTRYs */
			 || (sp->an.arrayelem && (!fortran  /* C array elem */
				|| !in_reg_alloc_flag))	 /* or in dead store */
			 || (sp->a6n.type.base == LONGDOUBLE)
			 || sp->a6n.offset == 0	/* bogus symtab entry */
				)
			    {
			    sp->a6n.varno = 0;
			    break;		/* not allocatable */
			    }
			else
			    goto numbervar;

	    case XN:
	    case X6N:
			/*
			 * Don't allow allocation of an array base to an 
			 * A reg for local arrays within 32K of A6.
			 */
			if (!sp->xn.seenininput		/* not seen in code */
			 || (sp->a6n.array && !(sp->a6n.arrayelem) && 
			     (sp->an.tag == X6N) &&
			     (!sp->an.farg) &&
			     (in_reg_alloc_flag) &&
			     ((sp->a6n.offset < 32768) &&
			      (sp->a6n.offset > -32769)))
			 || (! in_reg_alloc_flag
			  && (sp->a6n.equiv		/* EQUIVALENCE'd var */
			   || sp->a6n.isstruct)))	/* STRUCT/UNION */
			    {
			    sp->a6n.varno = 0;
			    break;
			    }

			/* entire arrays can always put base addr in A reg */
numbervar:
			sp->an.varno = ++lastvarno;
			varno_to_symtab[lastvarno] = sp;
			sp->an.lastdefno = 0;
			sp->an.du = NULL;
			sp->an.defset = NULL;
			sp->an.stmtno = 0;
			sp->an.regno = 0;

			if (! in_reg_alloc_flag)
			    break;

			/* calculate and store the register type */
#ifdef FTN_POINTERS
		        if ((sp->an.ptr && (!sp->an.farg || !fortran || ISPTRPTR(sp->a6n.type)))
#else
		        if ((sp->an.ptr && (!sp->an.farg || !fortran))
							/* non-farg pointer */
#endif
		         || (sp->an.tag == XN) || (sp->an.tag == X6N)
		         || (sp->an.tag == SN) || (sp->an.tag == S6N))
							/* array base address */
			    sp->xn.register_type = REG_A_SAVINGS;
		        else if (BTYPE(sp->an.type) == FLOAT)
			    if (fpaflag && saw_dragon_access)
			        sp->an.register_type = REG_FPA_FLOAT_SAVINGS;
			    else
			        sp->an.register_type = REG_881_FLOAT_SAVINGS;
		        else if (BTYPE(sp->an.type) == DOUBLE)
			    if (fpaflag && saw_dragon_access)
			        sp->an.register_type = REG_FPA_DOUBLE_SAVINGS;
			    else
			        sp->an.register_type = REG_881_DOUBLE_SAVINGS;
		        else
			    sp->an.register_type = REG_D_SAVINGS;
			break;
	    }
	}

}  /* number_vars */

/*****************************************************************************
 *
 *  POSTFIX_REGION_TRAVERSE()
 *
 *  Description:	Traverse region descriptor tree in postfix order,
 *			setting nesting depth, region number
 *			and onlyblocks fields.
 *
 *  Called by:		make_region_descriptors()
 *			postfix_region_traverse() -- recursive call
 *
 *  Input Parameters:	rp -- current region pointer
 *			nestdepth -- current nesting depth
 *			pregion_number -- pointer to region number counter
 *
 *  Output Parameters:	rp -- modified region descriptor
 *			pregion_number -- incremented
 *
 *  Globals Referenced:	dfo[]
 *			maxnumblocks
 *
 *  Globals Modified:	num_to_region -- entry for rp added
 *
 *  External Calls:	difference()
 *			nextel()
 *			new_set()
 *			postfix_region_traverse()
 *			setunion()
 *
 *****************************************************************************
 */

LOCAL void postfix_region_traverse(rp, nestdepth, pregion_number)
REGION *rp;		/* current region pointer */
ushort nestdepth;		/* nesting depth */
short *pregion_number;	/* pointer to region number counter */
{
    register REGION *cp;		/* pointer to child region */
    register long bn;			/* block number */

    rp->nestdepth = nestdepth;		/* set nesting depth */

    rp->onlyblocks = new_set(maxnumblocks);  /* originally empty */

    /* set number of iterations */
    /* The philosophy is this:
     *   No level can contribute more than 100
     *   Multiply until reaching 10000, then add
     */
    if (rp != topregion)		/* already set in topregion to 1 */
	{
	long nits;

	rp->niterations = rp->parent->niterations;

	if (rp->regioninfo && (rp->regioninfo->niterations != 0))
	    nits = rp->regioninfo->niterations;
	else
	    nits = DEFAULTLOOPITERATIONS;

	if (nits > 100)
	    nits = 100;

	if (rp->niterations < 10000)
	    rp->niterations *= nits;
	else
	    rp->niterations += nits;
	}

    /* traverse each child */
    for (cp = rp->child; cp; cp = cp->sibling)
	{
	postfix_region_traverse(cp, nestdepth+1, pregion_number);
	setunion(cp->blocks, rp->onlyblocks);
	}

    difference(rp->onlyblocks, rp->blocks, rp->onlyblocks);
							/* blocks in this loop
						       * and not in nested */

    /* process each block in only this loop */
    bn = -1;
    while ((bn = nextel(bn, rp->onlyblocks)) >= 0)
	{
	register BBLOCK *bp;

	bp = dfo[bn];
	bp->bb.region = rp;		/* map block to this region */
	bp->bb.nestdepth = nestdepth;	/* set nesting depth of block */
	}

    rp->regionno = ++(*pregion_number);	/* set region number */
    num_to_region[rp->regionno] = rp;	/* map region number to this REGION */
    
}  /* postfix_region_traverse */

/*****************************************************************************
 *
 *  REGISTER_ALLOCATION()
 *
 *  Description:	This is the main driver routine for register
 *			allocation.  It makes two passes over the statement
 *			trees.  The first pass collects definition and use info
 *			about every candidate for register allocation.  Then,
 *			reaching definitions and live variables data flow
 *			calculations are performed.  The def-use info is used
 *			to construct "webs" (def-use chains with common defs
 *			and uses).  The data flow info is used to find the
 *			"live range" (set of blocks/stmts) for each web.
 *			Allocation on a loop basis is calculated, too.
 *			The webs/loops are ordered by CPU cycle savings and
 *			registers are allocated so as to produce the greatest
 *			savings.  The second pass over the trees replaces
 *			memory references with register references.  Finally,
 *			register loads and stores are inserted where needed.
 *
 *  Called by:		main()   -- in c1.c
 *
 *  Input Parameters:	none
 *
 *  Output Parameters:	none
 *
 *  Globals Referenced:	global_disable
 *			lastep
 *			lastfarg
 *			reducible
 *
 *  Globals Modified:	in_reg_alloc_flag
 *
 *  External Calls:	allocate_regs()
 *			compute_live_vars()
 *			compute_reaching_defs()
 *			def_var_block_sets()
 *			free_live_and_reaching()
 *			free_reg_data()
 *			insert_loads_and_stores()
 *			make_region_descriptors()
 *			make_webs_and_loops()
 *			number_vars()
 *			reg_alloc_first_pass()
 *			reg_alloc_second_pass()
 *			rm_farg_moves_from_prolog()
 *			setup_global_data()
 *			sym_insert_icons()
 *
 *****************************************************************************
 */

void register_allocation()
{
#		ifdef DEBUGGING
		if (rtdebug)
		    dumpblockpool(1,1);
#		endif

    in_reg_alloc_flag = YES;
    

    make_region_descriptors();	/* create region (DO-loop) descriptor structure
				 */


    number_vars();		/* assign numbers to each register allocation
				 * candidate in the symbol table, create
				 * varno_to_symtab[] array
				 */

#		ifdef DEBUGGING
		if (rsdebug)
		    dumpsymtab();
		    if (rsdebug > 1)
			dump_varnos();
#		endif

    saw_dragon_access = NO;     /* changed to YES when dragon op is seen */
    saw_global_access = NO;

    reg_alloc_first_pass();	/* (regdefuse.c) first pass over stmt trees.
				 * fill definetab; create gen and kill sets
				 * for reaching defs and live vars DFA; find
				 * defs and uses for all reg candidates and
				 * make DU-blocks
				 */

#		ifdef DEBUGGING
		if (rddebug)
		    dump_definetab();
		if (rddebug | rudebug)
		    dumpsymtab();
		if (rtdebug)
		    dump_stmttab();
		if (rddebug || rldebug)
		    dump_gen_and_kill();
#		endif

    compute_reaching_defs();	/* reaching definitions data flow analysis */

#		ifdef DEBUGGING
		if (rddebug)
		    dump_reaching_defs();
#		endif

    compute_live_vars();	/* live variables data flow analysis */

#		ifdef DEBUGGING
		if (rldebug)
		    dump_live_vars();
#		endif

    def_var_block_sets();	/* create blockreach and blocklive sets showing
				 * the range for each def and live var
				 */

#if 0
    analyze();
#endif

#		ifdef DEBUGGING
		if (rddebug || rldebug)
		    dump_def_var_block_sets();
#		endif

    finish_region_descriptors(topregion);  /* finish region descriptor
					    * contents.
					    */

#		ifdef DEBUGGING
		if (rrdebug)
		    dump_regions();
#		endif

    if (fpa881loopsexist)
	mark_defs_in_fpa881loops();

    make_webs_and_loops();	/* (regweb.c) makes web and loop descriptors
				 * from the def-use info, reaching defs and
				 * live vars info, and the region descriptors.
				 * Descriptors contain live range, savings
				 * and register load/store info.
				 */

    free_live_and_reaching();	/* free live vars and reaching defs data
				 * structures.
				 */

#		ifdef DEBUGGING
		if (rwdebug)
		    dump_webs_and_loops();
#		endif

    allocate_regs();		/* (regallo.c) allocates registers to most
				 * beneficial webs and loops.  Creates reg maps
				 */

#		ifdef DEBUGGING
		if (rRdebug)
		    dump_colored_webs_and_regmaps();
#		endif

    reg_alloc_second_pass();	/* (regpass2.c) second pass over the stmt trees.
				 * Replace memory references with register
				 * refs, according to the reg maps.
				 */

#		ifdef DEBUGGING
		if (rtdebug > 1)
		    dumpblockpool(1,1);
#		endif

    insert_loads_and_stores();	/* (regpass2.c) insert register loads and
				 * stores in the statement trees where needed.
				 */

    rm_farg_moves_from_prolog();  /* (regpass2.c) if no ENTRYs, remove the
				 * copies from calling space to local space.
				 * Always free all data structures.
				 */

    free_reg_data();		/* free all register allocation data structures.
				 * Free global optimization data, too.
				 */

#		ifdef DEBUGGING
		if (rtdebug)
		    dumpblockpool(1,1);
#		endif

    in_reg_alloc_flag = NO;

}  /* register_allocation */

