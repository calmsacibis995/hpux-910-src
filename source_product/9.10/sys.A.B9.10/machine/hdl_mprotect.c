/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_mprotect.c,v $
 * $Revision: 1.2.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:40 $
 */

#include "../h/types.h"
#include "../h/debug.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../machine/vmparam.h"
#include "../machine/reg.h"
#include "../h/debug.h"
#include "../h/vas.h"
#include "../h/map.h"
#include "../h/sema.h"
#include "../h/pfdat.h"
#include "../h/pregion.h"
#include "../machine/param.h"
#include "../machine/pte.h"
#include "../machine/atl.h"
#include "../h/malloc.h"


/*
 * hdl_split_subpreg() --
 *    Split bucket "i" of a hdl_subpregion at page number "start_idx".
 *    Grows the subpregion if necessary, returning the address of the
 *    new data structure.
 */
hdl_subpreg_t *
hdl_split_subpreg(p_spreg, i, start_idx)
register hdl_subpreg_t *p_spreg;
int i;
int start_idx;
{
    int j;

    p_spreg->nused++;
    if (p_spreg->nused <= p_spreg->nelements) {
	/*
	 * We have room to grow within the current space.  Simply
	 * shift the array up one element (duplicating range[i] at
	 * range[i+1]) and then fixup range[i+1].
	 */
	for (j = p_spreg->nused-1; j > i; j--) {
	    p_spreg->range[j] = p_spreg->range[j-1];
	}
	p_spreg->range[i+1].idx = start_idx;
    }
    else {
	hdl_subpreg_t *old_spreg = p_spreg;
	int size;

	/*
	 * There is not sufficient room within the current space.
	 * Allocate a new structure that is 2X the current size and
	 * copy the old data to the new structure.
	 *
	 * NOTE:  we add 3 to the number of elements for our size
	 *        calculation because there are three words of space
	 *        before the range[] array.
	 */
	size = 2 * (p_spreg->nelements + 3) * sizeof p_spreg->range[0];
	MALLOC(p_spreg, hdl_subpreg_t *, size, M_PREG, M_WAITOK);
	p_spreg->nused = old_spreg->nused;
	p_spreg->nelements = (size / sizeof p_spreg->range[0]) - 3;
	p_spreg->hint = old_spreg->hint;

	/*
	 * Now copy the range[] array, duplicating the old range[i] at
	 * range[i] and range[i+1] and shifting the ones above i up by
	 * one element.
	 */
	for (j = 0; j <= i; j++)
	    p_spreg->range[j] = old_spreg->range[j];
	for (; j < p_spreg->nused; j++)
	    p_spreg->range[j] = old_spreg->range[j-1];
	p_spreg->range[i+1].idx = start_idx;
	FREE(old_spreg, M_PREG);
    }

    return p_spreg;
}

/*
 * hdl_setrange() --
 *    Change the access for a range of pages in the given pregion
 *    to the specified MPROT_* mode.
 */
void
hdl_setrange(prp, idx, count, mode)
preg_t *prp;
int idx;
int count;
u_long mode;
{
    register caddr_t vaddr = prp->p_vaddr + ptob(idx);
    register unsigned int *pte;
    register struct pte *pt;
    struct ste *va_seg = prp->p_vas->va_hdl.va_seg;
    int do_purge = 0;
    int do_selective = (count < 8);
    
    VASSERT(prp->p_vas != KERNVAS);

    for (; count--; vaddr += NBPG) {
	pte = (unsigned int *)itablewalk(va_seg, vaddr);
	if (pte == (unsigned int *)0)
	    continue;
	    
	/*
	 * Indirect PTE case...
	 */
	if (*pte & PG_IV) {
	    switch (mode) {
	    case MPROT_UNMAPPED:
	    case MPROT_NONE:
		*pte &= ~PG_IV;
		if (do_selective)
		    purge_tlb_select_user(vaddr);
		do_purge = 1;
		break;

	    case MPROT_RO:
		pt = (struct pte *)(*pte & 0xfffffffc);
		if (!pt->pg_ropte) {
		    *pte = (unsigned int)(pt - 1) | PG_IV;
		    if (do_selective)
			purge_tlb_select_user(vaddr);
		    do_purge = 1;
		}
		break;

	    case MPROT_RW:
		pt = (struct pte *)(*pte & 0xfffffffc);
		if (pt->pg_ropte) {
		    *pte = (unsigned int)(pt + 1) | PG_IV;
		    if (do_selective)
			purge_tlb_select_user(vaddr);
		    do_purge = 1;
		}
		break;
	    }
	    continue;
	}

	/*
	 * Non-indirect PTE case...
	 */
	if (*pte & PG_V) {
	    switch (mode) {
	    case MPROT_UNMAPPED:
	    case MPROT_NONE:
#ifdef PFDAT32
	    	if (prp->p_hdl.p_hdlflags & PHDL_ATLS)
		    atl_deletetrans(prp, prp->p_space, vaddr, 0);
		else
		    panic("hdl_setrange called for DPTE!");
#else	
		atl_deletetrans(prp, prp->p_space, vaddr, 0);
#endif	
		VASSERT((*pte & PG_V) == 0);
		break;
	    
	    case MPROT_RO:
		*pte |= PG_PROT;	/* set write protect bit */
		break;
	    
	    case MPROT_RW:
		*pte &= ~PG_PROT;	/* clear write protect bit */
		break;
	    }
	    if (do_selective)
		purge_tlb_select_user(vaddr);
	    do_purge = 1;
	}
    }

    if (do_purge && !do_selective)
	purge_tlb_user();
}

/*
 * hdl_range_mapped_or_unmapped() --
 *    Looking at the hardware dependent information for the given
 *    pregion, determine if the given range of pages (from off to
 *    off + count, inclusive) is currently all mapped or all unmapped.
 *
 *    The range off..off+count must be in the overall range for the
 *    pregion.
 */
int
hdl_range_mapped_or_unmapped(prp, idx, count, mapped)
preg_t *prp;
int idx;
int count;
int mapped; /* 1 if all mapped, 0 if all unmapped */
{
    register hdl_subpreg_t *p_spreg = prp->p_hdl.p_spreg;
    int i;

    VASSERT(count > 0);

    /*
     * If there is no per-page data, the answer is simple.
     */
    if (p_spreg == (hdl_subpreg_t *)0)
	return mapped;

    /*
     * Find the bucket containing "idx".
     */
    i = p_spreg->hint;
    if (p_spreg->range[i].idx != idx) {
	if (p_spreg->range[i].idx < idx) {
	    /*
	     * Search forwards.
	     */
	    i++;
	    while (p_spreg->range[i].idx <= idx)
		i++;
	    i--;
	}
	else {
	    /*
	     * Search backwards
	     */
	    i--;
	    while (p_spreg->range[i].idx > idx)
		i--;
	}
    }

    /*
     * Now scan the ranges from idx to idx+count and see
     * if any of them are unmapped.
     */
    while (count > 0) {
	int new_idx;

	if (mapped) {
	    if (p_spreg->range[i].mode == MPROT_UNMAPPED)
		return 0; /* part of the range is unmapped */
	}
	else {
	    if (p_spreg->range[i].mode != MPROT_UNMAPPED)
		return 0; /* part of the range is mapped */
	}

	/*
	 * Advance to the next bucket.
	 */
	new_idx = p_spreg->range[i+1].idx;
	count -= new_idx - idx;
	idx = new_idx;
	i++;
    }

    return 1;
}

/*
 * hdl_range_mapped() --
 *    Looking at the hardware dependent information for the given
 *    pregion, determine if the given range of pages (from off to
 *    off + count, inclusive is really part of the pregion.
 *
 *    The range off..off+count must be in the overall range for the
 *    pregion.
 *
 *    If there are no mprotect data structures associated with this
 *    region, we always return 1.
 *
 *    Returns 1 if so, 0 if not.
 */
int
hdl_range_mapped(prp, idx, count)
preg_t *prp;
int idx;
int count;
{
    return hdl_range_mapped_or_unmapped(prp, idx, count, 1);
}

/*
 * hdl_range_unmapped() --
 *    Looking at the hardware dependent information for the given
 *    pregion, determine if the given range of pages (from off to
 *    off + count, inclusive) is all unmapped.
 *
 *    The range off..off+count must be in the overall range for the
 *    pregion.
 *
 *    If there are no mprotect data structures associated with this
 *    region, we always return 0.
 *
 *    Returns 1 if so, 0 if not.
 */
int
hdl_range_unmapped(prp, idx, count)
preg_t *prp;
int idx;
int count;
{
    return hdl_range_mapped_or_unmapped(prp, idx, count, 0);
}


/*
 * hdl_page_mprot() --
 *    Return the mprotect page mode (MPROT_*) for the given page
 *    within the given pregion.
 */
u_long
hdl_page_mprot(prp, vaddr)
preg_t *prp;
caddr_t vaddr;
{
    register hdl_subpreg_t *p_spreg = prp->p_hdl.p_spreg;
    register int idx = btop(vaddr - prp->p_vaddr);
    int i;

    /*
     * If there is no per-page data, just return the protection
     * mode for the entire pregion.
     */
    if (p_spreg == (hdl_subpreg_t *)0)
	return (prp->p_prot & PROT_WRITE) ? MPROT_RW : MPROT_RO;

    /*
     * Find the bucket containing "idx".
     */
    i = p_spreg->hint;
    if (p_spreg->range[i].idx != idx) {
	if (p_spreg->range[i].idx < idx) {
	    /*
	     * Search forwards.
	     */
	    i++;
	    while (p_spreg->range[i].idx <= idx)
		i++;
	    i--;
	}
	else {
	    /*
	     * Search backwards
	     */
	    i--;
	    while (p_spreg->range[i].idx > idx)
		i--;
	}
    }

    p_spreg->hint = i;
    return p_spreg->range[i].mode;
}

/*
 * hdl_mprot_hole() --
 *    Find the first unmapped range within an mprotected pregion
 *    that is at least 'count' pages in size.  Returns -1 if
 *    none, otherwise the page index of the start of the hole.
 */
int
hdl_mprot_hole(prp, count)
preg_t *prp;
int count;
{
    hdl_subpreg_t *p_spreg = prp->p_hdl.p_spreg;
    register struct hdl_protrange *rptr = &p_spreg->range[0];
    int nused = p_spreg->nused;
    int i;
    int spot = -1;
    int need = count;

    for (i = 0; i < nused-1; i++) {
	if (rptr[i].mode == MPROT_UNMAPPED) {
	    int rcount = rptr[i+1].idx - rptr[i].idx;

	    /*
	     * If we do not have an adjacent run yet, start it here.
	     */
	    if (spot == -1)
		spot = i;

	    /*
	     * If this bucket is big enough, return the start of our
	     * current run.
	     */
	    if (need <= rcount)
		return rptr[spot].idx;
	    
	    /*
	     * This bucket is not big enough, but the next bucket may
	     * also be unmapped, so reduce 'need' by the size of this
	     * bucket and keep searching.
	     */
	    need -= rcount;
	}
	else {
	    /*
	     * This bucket is mapped, reset our run back
	     * to the starting condition.
	     */
	    spot = -1;
	    need = count;
	}
    }
    return -1;
}

/*
 * hdl_mprotect() --
 *    Set the protection on "count" pages of a pregion starting at
 *    start_idx to "mode".
 */
void
hdl_mprotect(prp, start_idx, count, mode)
preg_t *prp;
int start_idx;
int count;
u_long mode;
{
    register hdl_subpreg_t *p_spreg = prp->p_hdl.p_spreg;
    register struct hdl_protrange *rptr;
    int i;

    VASSERT(start_idx >= 0);
    VASSERT(count > 0);
    VASSERT(start_idx + count <= prp->p_count);

    if (p_spreg == (hdl_subpreg_t *)0) {
	/*
	 * Allocate a new structure to store the subpregion data.
	 */
	MALLOC(p_spreg, hdl_subpreg_t *, sizeof *p_spreg, M_PREG, M_WAITOK);
	prp->p_hdl.p_spreg = p_spreg;

	/*
	 * Initialize the new structure
	 */
	p_spreg->nused = 2;
	p_spreg->nelements = sizeof p_spreg->range / sizeof p_spreg->range[0];
	p_spreg->hint = 0;
	p_spreg->range[0].mode = (prp->p_prot & PROT_WRITE) ? MPROT_RW : MPROT_RO;
	p_spreg->range[0].idx = 0;
	p_spreg->range[1].mode = MPROT_UNMAPPED;
	p_spreg->range[1].idx = prp->p_count;
    }

    /*
     * Find the first bucket at or just before start_idx.
     */
    rptr = &p_spreg->range[0];
    i = p_spreg->hint;
    if (rptr[i].idx != start_idx) {
	if (rptr[i].idx < start_idx) {
	    /*
	     * Search forwards.
	     */
	    i++;
	    while (rptr[i].idx <= start_idx)
		i++;
	    i--;
	}
	else {
	    /*
	     * Search backwards
	     */
	    i--;
	    while (rptr[i].idx > start_idx)
		i--;
	}
    }

    /*
     * Okay, we have the starting bucket.  Before starting to loop
     * over the ranges, get things up to a range boundary (if not
     * already there).
     */
    if (rptr[i].idx != start_idx) {
	int rcount = rptr[i+1].idx - start_idx;

	if (rptr[i].mode == mode) {
	    /*
	     * This range is already the requested mode, if the range
	     * is larger than the count, then we can return.
	     * If not, we advance to the next range and start looking
	     * at the remaining ranges.
	     */
	    if (count < rcount) {
		p_spreg->hint = i;
		return;
	    }
	    count -= rcount;
	    i++;
	}
	else {
	    /*
	     * This range has a different mode.  If the next range
	     * has the same mode as the new mode, we may be able
	     * to move its start point back.  Otherwise, we will
	     * have to split this subrange in half.
	     */
	    if (count >= rcount && i < p_spreg->nused-2 &&
					    mode == rptr[i+1].mode) {
		i++;
		rptr[i].idx = start_idx;
		hdl_setrange(prp, start_idx, rcount, mode);
	    }
	    else {
		p_spreg = hdl_split_subpreg(p_spreg, i, start_idx);
		prp->p_hdl.p_spreg = p_spreg;
		rptr = &p_spreg->range[0];
		i++;
	    }
	    VASSERT(rptr[i].idx == start_idx);
	}
    }

    while (count > 0) {
	int ridx = rptr[i].idx;
	int rcount = rptr[i+1].idx - ridx;

	/*
	 * If the number of pages in this subrange is less than the
	 * number of pages we have left to change, we change the mode
	 * of the entire subrange.
	 */
	if (rcount <= count) {
	    if (rptr[i].mode != mode) {
		rptr[i].mode = mode;
		hdl_setrange(prp, ridx, rcount, mode);
	    }
	    count -= rcount;
	    i++;
	    continue;
	}

	/*
	 * If the current subrange already has the correct mode, we are
	 * done.  Just break out of this loop.
	 */
	if (rptr[i].mode == mode)
	    break;

	/*
	 * This subrange is larger than the subrange we want to change.
	 * We have two options:
	 *   1.  If the previous range has the same mode as the new
	 *       mode, we can simply move the start point of the current
	 *       range up to the end of the subrange we are changing.
	 *       NOTE: For this case, we must make sure that we are not
	 *             currently manipulating range[0].
	 *
	 *   2.  Otherwise, we must split this bucket into two, changing
	 *       the first half to the new mode.
	 */

	if (i > 0 && rptr[i-1].mode == mode) {
	    rptr[i].idx += count;				 /* Case 1 */
	}
	else {
	    p_spreg = hdl_split_subpreg(p_spreg, i, ridx+count); /* Case 2 */
	    prp->p_hdl.p_spreg = p_spreg;
	    p_spreg->range[i].mode = mode; /* not rptr, it may have changed */
	}

	hdl_setrange(prp, ridx, count, mode);
	break;
    }
    p_spreg->hint = i;
}

/*
 * hdl_mprot_dup()
 *    Duplicate the mprotect data from one pregion into another.
 */
void
hdl_mprot_dup(sprp, dprp)
preg_t *sprp;
preg_t *dprp;
{
    hdl_subpreg_t *sp_spreg, *dp_spreg;
    int size;

    sp_spreg = sprp->p_hdl.p_spreg;
    if (sp_spreg == (hdl_subpreg_t *)0) {
	dprp->p_hdl.p_spreg = (hdl_subpreg_t *)0;
	return;
    }

    size = (sp_spreg->nelements + 3) * sizeof sp_spreg->range[0];
    MALLOC(dp_spreg, hdl_subpreg_t *, size, M_PREG, M_WAITOK);
    bcopy(sp_spreg, dp_spreg, size);
    dprp->p_hdl.p_spreg = dp_spreg;
}
