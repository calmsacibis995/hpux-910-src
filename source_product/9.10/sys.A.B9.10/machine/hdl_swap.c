/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_swap.c,v $
 * $Revision: 1.6.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:27 $
 */

/*
 * This file contains the hardware dependent swap-related routines.  They are
 * responsible for clearing out and setting up of HDL structures describing 
 * the process' address space when swapping the process out or back in.
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/vas.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../mach.300/atl.h"


/*
 * Hardware dependent swap in routine
 *
 * The hdl_swapi routine is responsible for:
 *	- Allocating any vas-oriented resources which are needed to resume
 *		a process after it has swapped out; it should be the inverse
 *		of hdl_swapo().
 *
 * For 300:
 *	- Get a segment table for the vas
 *	- setup vas HDL fields va_seg, va_segpfn for same.
 */
hdl_swapi(vas)
	register struct vas *vas;
{
	register preg_t *prp;

	/* Allocate the segment table for this vas. */
	if (vas->va_hdl.va_seg == NULL)
		vas->va_hdl.va_seg = (struct ste *)alloc_segtab();

	/* 
	 * restore the attach list if necessary.  
	 * We must do the uarea here and might as well do the rest 
	 * too instead of in hdl_procswapi
	 */
	for (prp = vas->va_next; prp != (preg_t *)vas; 
	     prp = prp->p_next) 
#ifdef PFDAT32
		if (prp->p_hdl.p_hdlflags & PHDL_ATLS)
#else	
		if ((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0)
#endif	
			atl_swapi(prp);
}

/*
 * Hardware dependent swapout routine
 *
 * The HDL swapout routine is responsible for:
 *	- Freeing any vas-oriented resources which won't be needed while
 *		the process is swapped out.
 *
 * For 300:
 *     	- Free page tables for vas's segment table
 *	- Free vas's segment table
 *	- Clear vas's ptr to (now non-existent) segment table
 */
hdl_swapo(vas)
	register struct vas *vas;
{
	register int i, j;
	register preg_t *prp;

	/* release any allocated mapping tables */
	vm_dealloc_tables(vas);

	/* For each pregion in the vas, clear any remaining HDL translations */
	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		/*
		 * Remove any existing atl entries, the translations
		 * are removed in hdl_detach by calling vm_dealloc.
		 */
#ifdef PFDAT32
		if (prp->p_hdl.p_hdlflags & PHDL_ATLS)
#else	
		if ((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0)
#endif	
			atl_swapo(prp);

		VASSERT(prp->p_hdl.p_ntran == 0);
	}
}

/*
 * Hardware dependent process swapin routine
 *
 * The HDL process swapin routine is responsible for:
 *	- Allocating any process-oriented VM resources which may have been
 *		freed by hdl_procswapo().
 *
 * For 300:
 *	- fill in proc fields relating to segment table:
 *	-	p_segptr, physical segment table ptr
 *		p_addr, address of u area ptes
 */
hdl_procswapi(pp, vas)
	register struct proc *pp;
	register struct vas *vas;
{
	register preg_t *prp;
	pp->p_segptr = vas->va_hdl.va_seg;
	pp->p_addr = vtoipte(pp->p_upreg, ptob(KSTACK_PAGES));
	VASSERT(pp->p_addr);

}


/*
 * Hardware dependent process swapout routine
 *
 * The HDL process swapout routine is responsible for:
 *	- Freeing any process-oriented VM resources.
 *
 * For 300:
 *	- Free segment table
 *	- Clear out proc fields relating to segment table:
 *		p_segptr, physical segment table pointer
 *		p_addr, address of u area ptes
 */
/*ARGSUSED*/
hdl_procswapo(pp, vas)
	register struct proc *pp;
	register struct vas *vas;
{
	pp->p_segptr = NULL;
	pp->p_addr = NULL;

}

/*
 * Hardware dependent process swapin resource count
 *
 * The HDL process swapin resource count tells the HIL how many
 * pages the HDL will need in order to swap in the named process.
 * This is necessary to prevent deadlocks.
 *
 * For the 300:
 *
 *    We return absolute worst case numbers such that the process is
 *    guarenteed to be able to execute one instruction, including
 *		hld page tables 
 *              minimum number pages they would map to assure 1 instruction
 *		uarea
 */
/*ARGSUSED*/
hdl_swapicnt(pp, vas)
    struct proc *pp;
    vas_t *vas;
{
    register int npages;
    register int pcount;
    register preg_t *prp;

    npages = 0;

    /* Account for pages taken up by attach lists */

    for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
#ifdef PFDAT32
	if (prp->p_hdl.p_hdlflags & PHDL_ATLS) {
#else	
	if ((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0) {
#endif	

	    pcount = prp->p_count;
	    if (prp->p_type != PT_STACK)
		pcount += ATL_PRP_EXTRA;

	    npages += btorp(pcount * sizeof(struct pregion_atl_entry));
	}
    }

    /* Account for pages taken up by page tables, segment tables, etc. */
    /* Also add a slop factor, to allow the process to get at least    */
    /* enough pages to actually execute one instruction.               */

    if (three_level_tables) {
	if (pp->p_flag&SSYS)
	    npages += 1;
	else
	    npages += 11;
    }
    else {
	if (pp->p_flag&SSYS)
	    npages += 2;
	else
	    npages += 14;
    }

    return(npages);
}
