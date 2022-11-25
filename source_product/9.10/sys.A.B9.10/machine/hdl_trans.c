/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_trans.c,v $
 * $Revision: 1.10.84.8 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/10 15:08:16 $
 */

/*
 * This file contains all of the hardware dependent translation routines.
 * The routines in this file establish and maintain actual mapping and
 * protections for physical pages in the system.
 */

/*
 * hdl_trans.c--translation-oriented HDL routines
 */
#include "../h/types.h"
#include "../h/debug.h"
#include "../h/vmparam.h"
#include "../h/param.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/vfd.h"
#include "../h/pfdat.h"
#include "../h/vmmac.h"
#include "../h/map.h"
#include "../h/vdma.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../machine/param.h"
#include "../machine/pte.h"
#include "../machine/atl.h"
#include "../machine/vmparam.h"

#define WAITOK 1


#define min(x,y) ( ((x) < (y)) ? (x) : (y) )
unsigned int *vm_alloc_tables();
#ifdef OSDEBUG
extern int check_iptes;
#endif /* OSDEBUG */

#ifdef BNR_HPSOS
void default_trans_hook(pte, vaddr)
struct pte *pte;
u_int vaddr;
{
	/* Do nothing */
}

void (*hdl_trans_hook)() = default_trans_hook;

#endif /* BNR_HPSOS */

io_addtrans(prp)
	register preg_t *prp;
{
	register unsigned int new_pte;
	register unsigned int *pt;
	register int cachemode = prp->p_hdl.p_hdlflags & PHDL_CACHE_MASK;
	register int i;
	extern unsigned int *itablewalk();


	VASSERT(prp->p_type == PT_IO);

	if (prp->p_vas->va_hdl.va_seg == NULL)
		return 1;

	/*
	 * Allocate new page table if necessary.
	 * Reserve worst case number of pages we might need.
	 */
	if (mainentered)
		procmemreserve(3, WAITOK, (reg_t *)0);
	pt = vm_alloc_tables(prp->p_vas, prp->p_vaddr);
	if (mainentered)
		procmemunreserve();

	/* build a proto type pte */
	new_pte = (prp->p_hdl.p_physpfn << PGSHIFT) | PG_V;

	/* set the cache mode for this pregion */
	switch(cachemode) {
		/* cacheable, writethrough */
		case PHDL_CACHE_WT:				break;

		/* cacheable, copyback (68040 only) */
		case PHDL_CACHE_CB: new_pte |= PG_CB;		break;

		/* cache inhibited, serialized */
		case PHDL_CACHE_CI: new_pte |= PG_CI;		break;

		/* cache inhibited nonserialized (68040 only) */
		case PHDL_CACHE_NS: new_pte |= PG_CI | PG_NS;	break;
		default: new_pte |= PG_CI;
	}

	/* fill in all of the ptes for this pregion */
	for (i = 0; i < prp->p_count; i++, new_pte += NBPG) {
		*pt++ = new_pte;
		if (END_PT_PAGE(pt)) {
			if (mainentered)
				procmemreserve(3, WAITOK, (reg_t *)0);
			pt = vm_alloc_tables(prp->p_vas, (unsigned int)
					prp->p_vaddr + ((i+1) * NBPG));
			if (mainentered)
				procmemunreserve();
		}
	}

	/* Flush the TLB */
	if (prp->p_space == KERNELSPACE)
		purge_tlb_super();
	else
		purge_tlb_user();

	return 0;
}

io_deletetrans(prp)
	register preg_t *prp;
{
	register int i;
	register struct pte *pte;

	VASSERT(prp->p_type == PT_IO);

	if (prp->p_vas->va_hdl.va_seg == NULL)
		return;

	pte = (struct pte *)vastopte(prp->p_vas, prp->p_vaddr);
	if ((pte == (struct pte *)NULL) || (pte->pg_v == 0))
		return;

	for (i = 0; i < prp->p_count; i++) {
		*(int *)pte++ = 0;
		if (END_PT_PAGE(pte))
			pte = (struct pte *)vastopte(prp->p_vas,
			   (unsigned int)prp->p_vaddr + ((i+1) * NBPG));
	}

	if (prp->p_space == KERNELSPACE)
		purge_tlb_super();
	else
		purge_tlb_user();
}

/*
 * Map a page into the kernel.
 *
 *  Maps a page at the specified index, returns a pointer to it.
 *   Assumes page is in memory.
 *  When mapping from kernel space, we just return the vaddr, adjusted
 *   to a page boundry.
 */
caddr_t
kern_map(vas, vaddr, wrt)
	vas_t *vas;
	caddr_t vaddr;
	int wrt;
{
	preg_t *prp;
	vfd_t *vfd;
	int pfn, bits;
	register struct pte *pt;
	unsigned int *ipte;

	/*
	 * Kernel space is a trivial case
	 */
	if (vas == &kernvas)
		return((caddr_t)(((unsigned long)vaddr) & ~(NBPG-1)));

	/*
	 * If the page is currently mapped, get the page number and
	 * construct a virtual address from that.
	 */
	ipte = (unsigned int *)vastoipte(vas, vaddr);
	if (ipte != NULL) {
		if ((*ipte) & PG_IV) {
			pt = (struct pte *)(*ipte & 0xfffffffc);
			if ((pt != NULL) && pt->pg_v) {
				/*
				 * Mark users pte as referenced.  If write
				 * mark the users modified bit.  Don't set the
				 * modify bit on a read-only ipte, use the
				 * read-write version.
				 */
				pt->pg_ref = 1;
				if (wrt) {
					/* point at the read/write pte */
					if (pt->pg_ropte)
						pt++;
					pt->pg_m = 1;
				}
				purge_icache();
				purge_dcache();
				return((caddr_t) (pt->pg_pfnum << PGSHIFT));
			}

		} else {
			pt = (struct pte *)ipte;
			if ((pt != NULL) && pt->pg_v) {
				/*
				 * Mark users pte as referenced.  If write
				 * mark the users modified bit.
				 */
				pt->pg_ref = 1;
				if (wrt) {
					pt->pg_m = 1;
				}
				purge_icache();
				purge_dcache();
				return((caddr_t) (pt->pg_pfnum << PGSHIFT));
			}
		}
	}

	/*
	 * Get User's page frame number from pregion and construct a virtual
	 * address from that.
	 *
	 * get the HI version of the page number
	 */
	if ((prp = searchprp(vas, -1, vaddr)) == (preg_t *)NULL)
		panic("hdl_copy_page: sprp");
	vfd = FINDVFD(prp->p_reg, regindx(prp, vaddr));
	VASSERT(vfd->pgm.pg_v);
	pfn = vfd->pgm.pg_pfn;

	/*
	 * Update ref/mod bits in pfdat[]
	 * when we don't have the PTE
	 */
	bits = VPG_REF | (wrt ? VPG_MOD : 0);
	pfdat[pfn].pf_hdl.pf_bits |= bits;

	/*
	 * Return the transparent tlb virtual address.
	 * NOTE: Clear out the ttlbva old values when the ttl is no
	 * longer cache inihibited. 2/23/90 sfk XXX
	 */
	purge_icache();
	purge_dcache();
	return(ttlbva(pfn));
}

/*
 * Hardware dependent copy page routine.
 *
 * The HDL copy_page routine is responsible for:
 *	- copying from the source virtual address (sspace, svaddr)
 *	  to the destination virtual address (dspace, dvaddr)
 *	  for bytes (bytes).
 *	- Some key assumptions made by this routine:
 *		- Pages are aligned on a HIL page boundry
 *		- Physical memory is present for the pages
 *		- Always copied in page-sized quantities
 *
 * 300 HDL copy page routine.
 *
 * For 300 we need to:
 *	- If pages aren't in memory, fault in manually with vfault().
 *	- Map them into the kernel through kern_map().
 *	- Call bcopy() to do the actual copy in assembly language.
 *	- This routine assumes that a pregion boundary is never crossed.
 */
/*ARGSUSED*/
hdl_copy_page(svas, sspace, svaddr, dvas, dspace, dvaddr, bytes )
	vas_t *svas, *dvas;
	space_t sspace, dspace;
	caddr_t svaddr, dvaddr;
	register int bytes;
{
	register char *src, *dest;
	register int scount, dcount, count;

	/*
	 * Get the pregions for the source & destination.
	 * Map the initial pages & figure out how many bytes lie
	 * on that page.
	 */
	scount = BYTES_ON_PAGE(svaddr);
	src = kern_map(svas, svaddr, 0) + (NBPG - scount);

	dcount = BYTES_ON_PAGE(dvaddr);
	dest = kern_map(dvas, dvaddr, 1)+(NBPG-dcount);

	/* Copy data */
	while (bytes > 0) {

		/* See how many bytes before end of someone's page */
		count = min(bytes, scount);
		count = min(count, dcount);

		/* Copy that, flush out cache */
		if (count == NBPG) {
			pg_copy(src, dest);
		} else {
			bcopy(src, dest, count);
		}
		/* do we need to do this? */
		purge_dcache_s();

		/* Decrement count remaining */
		bytes -= count;

		/* Advance source location */
		svaddr += count;
		src += count;

		/* If crossed page boundry, map next page */
		if (((scount -= count) == 0) && bytes) {
			scount = NBPG;
			src = kern_map(svas, svaddr, 0);
		}

		/* Advance destination location */
		dvaddr += count;
		dest += count;

		/* Similar for destination */
		if (((dcount -= count) == 0) && bytes) {
			dcount = NBPG;
			dest = kern_map(dvas, dvaddr, 1);
		}
	}
}



/*
 * Hardware dependent unvirtualize page routine.
 *
 * The HDL unvirtualize routine is responsible for:
 *	- deleting all translations for the given page (pfn).
 *
 * For 300:
 *  Walk attach list, invalidate PTE for each attached pregion.
 *  Assumes that kernel maps are not changed here--only flushes
 *   the user TLBs.
 *  Clear the mod and ref info in the pfdat.
 */
hdl_unvirtualize(pfn)
	int pfn;
{
	pfd_t *pfd = pfdat+pfn;
	

#ifdef PFDAT32
	VASSERT((pfd - pfdat) == pfn);
	VASSERT(pfd_is_locked(&pfdat[pfn]));
	if (pfd->pf_hdl.pf_bits & PFHDL_ATLS)
		atl_unvirtualize(pfn);
	else if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
#else
	VASSERT(pfd->pf_pfn == pfn);
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
	if ((pfd->pf_hdl.pf_bits & PFHDL_IPTES) == 0)
		atl_unvirtualize(pfn);
	else {
#endif
		*(int *)&pfd->pf_hdl.pf_rw_pte &= ~(PG_V | PG_M | PG_REF);
		*(int *)&pfd->pf_hdl.pf_ro_pte &= ~(PG_V | PG_M | PG_REF);
#ifdef PFDAT32
		pfd->pf_hdl.pf_bits &= ~(VPG_MOD | VPG_REF | PFHDL_COPYFRONT |
#else
		pfd->pf_hdl.pf_bits &= ~(VPG_MOD | VPG_REF | PF_COPYFRONT |
#endif
						PFHDL_UAREA | PFHDL_IPTES);
#ifndef	PFDAT32
		purge_tlb();
#endif
#ifdef OSDEBUG
		if (check_iptes)
			vl_find_iptes(pfn, 1);
#endif 
#ifdef PFDAT32
	} else if (pfd->pf_hdl.pf_bits & PFHDL_DPTES && pfd->pf_hdl.pf_kpp) {
		if (pfd->pf_hdl.pf_kpp->pg_pfnum && indirect_ptes &&
		    pfd->pf_hdl.pf_kpp->pg_pfnum != pfd->pf_hdl.pf_rw_pte.pg_pfnum) 
			panic("hdl_unvirtualize");
		else
			*(unsigned int *)pfd->pf_hdl.pf_kpp &= ~(PG_V | PG_M | PG_REF);
#endif
	}

#ifdef PFDAT32
	pfd->pf_hdl.pf_bits &= PFHDL_ZERO_BITS;
	purge_tlb();
	
	VASSERT(pfd_is_locked(pfd));
#else
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
	VASSERT((hdl_getbits(pfn) & (VPG_MOD|VPG_REF))==0);
#endif
}



/*
 * Hardware dependent add translation routine.
 *
 * The HDL addtrans routine is responsible for:
 *	- creating a translation for the given page (pfn), with
 *	  the given pregion (prp) at the specified virtual address
 *	  (space, vaddr) with the specified protection (cw = copy on write).
 *
 * For 300 we need to:
 *	- update the user's PTE, which hangs off of the pregion
 *	- create an "attach list" element and add it to the list
 *		hanging off the pfdat entry.
 */
hdl_addtrans(prp, space, vaddr, cw, pfn)
	register preg_t *prp;
	space_t space;
	register caddr_t vaddr;
	int cw;
	register int pfn;
{
	register unsigned int new_pte;
	register unsigned int *pte;
	register unsigned int *pt;
	register pfd_t *pfd = pfdat+pfn;
	extern int processor;
	int notkspace = (space != KERNELSPACE);
	int ro;

	VASSERT(prp->p_type != PT_IO);
#ifdef PFDAT32
	VASSERT(pfd_is_locked(&pfdat[pfn]));
#else
	VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif

	if (((vas_t *)space)->va_hdl.va_seg == NULL)
		return 1;

	/* Allocate new page table if necessary */
	if ((pt  = vm_alloc_tables((vas_t *)space, vaddr)) == NULL)
		return 1;

	/*
	 * If this pregion has been protected with mprotect(), get this
	 * pages real access rights from the per-range data.
	 */
	ro = cw;
	if (IS_MPROTECTED(prp)) {
	    u_long mprot = mprot = hdl_page_mprot(prp, vaddr);

	    /*
	     * If the page is marked MPROT_NONE or MPROT_UNMAPPED, do
	     * not really add a translation for the page.
	     */
	    if (mprot <= MPROT_NONE)
		return 1;
	    if (mprot == MPROT_RO)
		ro = 1;		/* force read-only access */
	}
	else
	    ro |= (prp->p_hdl.p_hdlflags & PHDL_RO);

	if (!ro && prp->p_type == PT_MMAP) {
		/*
		 * For memory mapped files we constrain pages that
		 * represent holes in a file to be read-only.  A write
		 * fault on such a page will cause space to be allocated
		 * for the hole on disk.
		 */
		int pgindx = regindx(prp, vaddr);
		dbd_t *dbd = FINDDBD(prp->p_reg, pgindx);

		if (dbd->dbd_type == DBD_HOLE)
			ro = 1;		/* force read-only access */
	}

#ifdef PFDAT32
	if (prp->p_hdl.p_hdlflags & PHDL_IPTES) {
#else
	if (notkspace && (prp->p_hdl.p_hdlflags & PHDL_IPTES)) {
#endif
		pfd->pf_hdl.pf_bits |= PFHDL_IPTES;

		/* pick up correct ipte based on protections */
		if (ro)
			pte = (unsigned int *)&pfd->pf_hdl.pf_ro_pte;
		else {
			pte = (unsigned int *)&pfd->pf_hdl.pf_rw_pte;
			*pte &= ~PG_PROT;
		}
		*pt = ((int)pte - (int)pfdat + physpfdat) | PG_IV;

		/* Clear old PG_CB and PG_CI bits */

		*pte &= ~(PG_CB|PG_CI);

		/* Set page mode bits */
/*   XXX  set valid bit once  */
/*  XXX  why special flag U area?  it's not paged...  */
		switch(prp->p_type) {
			case PT_UAREA:
				/*
				 * Set the mod bit so hdl_getbits will always
				 * return modified for uareas.
				 */
				pfd->pf_hdl.pf_bits |= PFHDL_UAREA;
				*pte |= PG_CB | PG_V;
				break;
			case PT_STACK:
				if ((u.u_procp->p_flag2 & S2STACK_WT) == 0)
					*pte |= PG_CB | PG_V;
				else
					*pte |= PG_V;
				break;
			case PT_SHMEM:
			case PT_GRAFLOCKPG:
				if (processor != M68040)
					*pte |= PG_CI | PG_V;
				else
					*pte |= PG_V;
				break;
			case PT_DATA:
				if ((u.u_procp->p_flag2 & S2DATA_WT) == 0)
					*pte |= PG_CB | PG_V;
				else
					*pte |= PG_V;
				break;
			default:
				*pte |= PG_V;
				break;
		}
#ifdef BNR_HPSOS
		(*hdl_trans_hook) (pt, vaddr);
#endif /* BNR_HPSOS */

		/* Flush this entry from the TLB */
		purge_tlb_select_user(vaddr);

		return 0;
	}

	VASSERT(!notkspace || ((pfd->pf_hdl.pf_bits & PFHDL_IPTES) == 0));
	VASSERT((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0);

	if ((*pt & PG_V) && (hiltopfn(pfn) << PGSHIFT) == (*pt & PG_FRAME)) {

	    /* Preserve PG_NOTIFY and caching modes */

	    if (ro)
		*pt |= PG_PROT;
	    else
		*pt &= ~PG_PROT;
	}
	else {

	    /* Build a proto type pte */

	    if (ro)
		    new_pte = (hiltopfn(pfn) << PGSHIFT) | PG_PROT | PG_V;
	    else
		    new_pte = (hiltopfn(pfn) << PGSHIFT) | PG_V;

	    /* Set cache mode */

	    switch(prp->p_type) {
		    case PT_STACK:
			    if (notkspace &&
				((u.u_procp->p_flag2 & S2STACK_WT) == 0))
				    new_pte |= PG_CB;
			    break;
		    case PT_SHMEM:
		    case PT_GRAFLOCKPG:
			    new_pte |= PG_CI;
			    break;
		    case PT_DATA:
			    if (notkspace &&
				((u.u_procp->p_flag2 & S2DATA_WT) == 0))
				    new_pte |= PG_CB;
			    break;
		    default:
			    break;
	    }

	    /* Store the new pte */
	    *pt = new_pte;
	}

#ifdef BNR_HPSOS
	if (notkspace)
		(*hdl_trans_hook) (pt, vaddr);
#endif /* BNR_HPSOS */

	/* Flush this entry from the TLB */
	if (notkspace)
		purge_tlb_select_user(vaddr);
	else
		purge_tlb_select_super(vaddr);

#ifdef PFDAT32
	/* add an attach list entry if appropriate */
	if (prp->p_hdl.p_hdlflags & PHDL_ATLS) 
		atl_addtrans(prp, pfn, pt, vaddr);
	else {
		pfd->pf_hdl.pf_kpp = (struct pte *) pt;
		pfd->pf_hdl.pf_bits |= PFHDL_DPTES;
	}
#else
	/* add an attach list entry */
	atl_addtrans(prp, pfn, pt, vaddr);
#endif
	return 0;
}

/*
 * Hardware dependent chunk add translation routine.
 *
 *      Same as hdl_addtrans() for chunks.
 */

hdl_addtransc(prp, space, vaddr, count, vd)
    register preg_t *prp;
    space_t space;
    register caddr_t vaddr;
    register int count;
    struct vfddbd *vd;
{
    register unsigned int *pt;
    register pfd_t *pfd;
    register unsigned int *pte;
    register int pfn;
    register int pfhdlbits;
    register int ptebits;
    register int ro;
    register int indirect;
    int retval;
    int save_sr;
    int notkspace = (space != KERNELSPACE);
    vfd_t *vfd = &(vd->c_vfd);
    dbd_t *dbd = &(vd->c_dbd);
    extern int processor;
    int mprotected = IS_MPROTECTED(prp);

    VASSERT(prp->p_type != PT_IO);

    if (((vas_t *)space)->va_hdl.va_seg == NULL)
	    return 1;

    /* Allocate new page table if necessary */

    if ((pt  = vm_alloc_tables((vas_t *)space, vaddr)) == NULL)
	return 1;

    /* Copy on write is guaranteed to be the same for the entire
     * chunk. We set the read only flag based on the copy on write
     * bit for the first vfd and the PHDL_RO flag for the pregion
     */

     /*
      * Set read only flag initially from pregion PHDL_RO flag, unless
      * the pregion has been mprotected, in which case we find the
      * correct mode for each page as we go.
      */
    if (mprotected)
	ro = 0;
    else
	ro = (prp->p_hdl.p_hdlflags & PHDL_RO);

    /* Set indirect flag */
    if (notkspace && (prp->p_hdl.p_hdlflags & PHDL_IPTES)) {
	indirect = 1;

	/*
	 * If pregion is type PT_UREA set PFHDL_UAREA
	 * bit in pfdat so that hdl_getbits() will always
	 * return modified for uareas.
	 */
	if (prp->p_type == PT_UAREA)
	    pfhdlbits = PFHDL_UAREA | PFHDL_IPTES;
	else
	    pfhdlbits = PFHDL_IPTES;
    }
    else {
	VASSERT((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0);
	indirect = 0;
    }

    switch(prp->p_type) {

    case PT_UAREA:
	    ptebits = PG_CB | PG_V;
	    break;

    case PT_STACK:
	    if (notkspace && ((u.u_procp->p_flag2 & S2STACK_WT) == 0))
		    ptebits = PG_CB | PG_V;
	    else
		    ptebits = PG_V;
	    break;

    case PT_SHMEM:
    case PT_GRAFLOCKPG:
	    ptebits =  PG_CI | PG_V;
	    break;

    case PT_DATA:
	    if (notkspace && ((u.u_procp->p_flag2 & S2DATA_WT) == 0))
		    ptebits = PG_CB | PG_V;
	    else
		    ptebits = PG_V;
	    break;

    default:
	    ptebits = PG_V;
	    break;
    }

    /*
     * If indirect ptes, we just go to spl 6 and don't bother with any
     * pages that are locked.
     */
    if (indirect)
	save_sr = CRIT();

    retval = 0;
    while (count-- > 0) {
	int valid = vfd->pgm.pg_v;
	int mprot_ro = 0;

	if (mprotected && valid) {
	    u_long mprot = hdl_page_mprot(prp, vaddr);

	    if (mprot <= MPROT_NONE)
		valid = 0;
	    else if (mprot == MPROT_RO)
		mprot_ro = 1;
	}

	if (valid) {
	    pfn = vfd->pgm.pg_pfn;
	    pfd = pfdat + pfn;

	    if (indirect) {
#ifdef PFDAT32
		if (!pfd_is_locked(pfd)) {
#else
		if ((pfd->pf_lock.b_lock & SEM_LOCKED) == 0) {
#endif
		    pfd->pf_hdl.pf_bits |= pfhdlbits;

		    /* pick up correct ipte based on ro flag */

		    if (ro || mprot_ro || vfd->pgm.pg_cw ||
			dbd->dbd_type == DBD_HOLE) {
			    pte = (unsigned int *)&pfd->pf_hdl.pf_ro_pte;
		    }
		    else {
			    pte = (unsigned int *)&pfd->pf_hdl.pf_rw_pte;
			    *pte &= ~PG_PROT;
		    }

		    *pt = ((int)pte - (int)pfdat + physpfdat) | PG_IV;

		    /* Clear old PG_CB and PG_CI bits */

		    *(int *)pte &= ~(PG_CB|PG_CI);

		    /* Set PG_CB, PG_CI and PG_V bits appropriately */

		    *(int *)pte |= ptebits;
#ifdef BNR_HPSOS
		    if (notkspace)
			(*hdl_trans_hook) (pt, vaddr);
#endif /* BNR_HPSOS */
		}
	    }
	    else {
		if (cpfdatlock(pfd)) {
		    VASSERT(!notkspace || ((pfd->pf_hdl.pf_bits & PFHDL_IPTES) == 0));

		    /* Store pte */

		    if ((*pt & PG_V)
			&& (hiltopfn(pfn) << PGSHIFT) == (*pt & PG_FRAME)) {

			/* Preserve PG_NOTIFY and caching modes */

			if (ro || mprot_ro || vfd->pgm.pg_cw ||
			    dbd->dbd_type == DBD_HOLE) {
			    *pt |= PG_PROT;
			}
			else {
			    *pt &= ~PG_PROT;
			}
		    }
		    else {
			if (ro || mprot_ro || vfd->pgm.pg_cw ||
			    dbd->dbd_type == DBD_HOLE) {
			    *pt = (hiltopfn(pfn) << PGSHIFT) | PG_PROT | ptebits;
			}
			else {
			    *pt = (hiltopfn(pfn) << PGSHIFT) | ptebits;
			}
		    }
#ifdef BNR_HPSOS
		    if (notkspace)
			(*hdl_trans_hook) (pt, vaddr);
#endif /* BNR_HPSOS */

		    /* add an attach list entry */
#ifdef PFDAT32
		    if (prp->p_hdl.p_hdlflags & PHDL_ATLS) 
			atl_addtrans(prp, pfn, pt, vaddr);
		    else {
  		    	pfd->pf_hdl.pf_kpp = (struct pte *) pt;
		    	pfd->pf_hdl.pf_bits |= PFHDL_DPTES;
	 	    }
#else
		    atl_addtrans(prp, pfn, pt, vaddr);
#endif
		    pfdatunlock(pfd);
		}
	    }
	}
	else {
	    struct pte *pt;

	    pt = vastoipte((vas_t *)space, vaddr);
	    if (pt)
		*(int *)pt = 0;
	}

	vaddr += NBPG;
	vfd = (vfd_t *)(((struct vfddbd *)vfd) + 1);
	dbd = (dbd_t *)(((struct vfddbd *)dbd) + 1);
	pt++;
	if (END_PT_PAGE(pt) && count > 0) {
	    pt = vm_alloc_tables((vas_t *)space, vaddr);
	    if (pt == NULL) {
		retval = 1;

		/* Since we have made at least one
		 * translation, we break out of loop
		 * to purge the tlb.
		 */

		break;
	    }
	}
    }

    if (indirect)
	UNCRIT(save_sr);

    /* Flush the TLB */

    if (notkspace)
	    purge_tlb_user();
    else
	    purge_tlb_super();

    return retval;
}

/*
 * Hardware dependent copy on write routine.
 *
 * The HDL cw routine is responsible for:
 *	- making sure that the given page is not writable
 *	  by the user.
 *	- making sure *all* translations are read only
 *
 * 300 HDL cw routine.
 *
 * For 300 we need to:
 *	- disable write access to the page for all user processes.
 *	  this means going through the attach list.
 */
hdl_cw(pfn)
	u_int pfn;
{
	pfd_t *pfd = pfdat+pfn;

#ifdef PFDAT32
	VASSERT(pfd_is_locked(&pfdat[pfn]));
#else
	VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif

#ifdef PFDAT32
	if (pfd->pf_hdl.pf_bits&PFHDL_COPYFRONT)
#else
	if (pfd->pf_hdl.pf_bits&PF_COPYFRONT)
#endif
		return(0);

	if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
		/* Disable write access */
		pfd->pf_hdl.pf_rw_pte.pg_prot = 1;
		VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
		VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
		purge_tlb_user();
#ifdef PFDAT32
	} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
		atl_cw(pfn);
	else
		panic("cw called for DPTE");
#else
	} else
		atl_cw(pfn);
#endif
	return(1);
}

/*
 * Hardware dependent zero page routine.
 *
 * The HDL zero_page routine is responsible for:
 *	- zeroing out the bytes (count) beginning at the specified virtual
 *	  address (space, vaddr).
 *
 * 300 HDL zero page routine.
 *
 * For 300 we need to:
 *	- Map each page into our address space.
 *	- Zero it out
 */
hdl_zero_page(space, vaddr, bytes)
space_t space;
caddr_t vaddr;
register int bytes;
{
	register char *p;
	register int count;

	/*
	 * Trivial for KERNELSPACE
	 */
	if (space == KERNELSPACE)
	{
		if ((bytes == NBPG) && (((int)vaddr & (16-1)) == 0))
		{
			pg_zero(vaddr);
		} else
		{
			bzero(vaddr, bytes);
		}
		return;
	}

	/*
	 * Map in the initial page
	 */
	count = BYTES_ON_PAGE(vaddr);
	p = kern_map((vas_t *)space, vaddr, 1);

	/* Adjust for partial page fill */
	p += NBPG-count;

	/* Zero a page at a time */
	while (bytes > 0) {

		/* Zero current amount, flush out */
		if (count == NBPG)
		{
			pg_zero(p);
		} else
		{
			bzero(p, count);
		}
		purge_dcache_s();

		/* Advance source location, update total byte counter */
		vaddr += count;
		bytes -= count;

		/* Will always end up page aligned except for first copy */
		count = NBPG;

		/* Map in next page */
		if (bytes > 0)
			p = kern_map((vas_t *)space, vaddr, 1);
	}
}

/* Return page frame number for a virtual address */
int
hdl_vpfn(space, vaddr)
space_t space;
unsigned int vaddr;
{
	register struct pte *pt;

	/* the tt window is equivalently mapped */
	if (space == KERNELSPACE) {
		if (vaddr >= tt_region_addr) {
			if (vaddr >= physpfdat)
				return(pfntohil(vaddr >> PGSHIFT));
			else
				return(-1);
		}
	}

	/* Look up PTE, gets its physical page */
	pt = vastopte((vas_t *)space, (caddr_t)vaddr);
	if (pt == NULL) {
		return(-1);
	}
	return(pfntohil(pt->pg_pfnum));
}


/*
 * Hardware dependent delete translation routine.
 *
 * The HDL deletetrans routine is responsible for:
 *	- deleting a translation for the given page (pfn), with
 *	  the given pregion (prp) at the specified virtual address
 *	  (space, vaddr).
 *
 * 300 HDL deletetrans routine.
 *
 * For 300 we need to:
 *	- Clear the user's PTE to close access
 *	- Remove his "attach list" entry from the pfdat attach chain
 */
hdl_deletetrans(prp, space, vaddr, pfn)
register preg_t *prp;
space_t space;
caddr_t vaddr;
int pfn;
{
	int iskern = (space == KERNELSPACE);
	struct pte opt;
#ifdef PFDAT32
	unsigned int *pp;
	struct pfdat *pfd;
		
#endif

	if (prp->p_type == PT_IO)
		return;

#ifdef PFDAT32
	VASSERT(pfd_is_locked(&pfdat[pfn]));
#else
	VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif

	if (prp->p_hdl.p_hdlflags & PHDL_IPTES) {
		int x, idx, pgindx;
		preg_t *prp2;
		register struct pte *pte;

		pgindx = regindx(prp, vaddr);
		x = CRIT();
		prp2 = prp->p_reg->r_pregs;
		while (prp2 != (preg_t *)NULL) {
			/* pick up the correct region index for this pregion */
			idx = pgindx - prp2->p_off;

			if ((idx >= 0) && (idx < prp2->p_count)) {
				pte = vtoipte(prp2, ptob(idx));
				if (pte)
					*(int *)pte = 0;
			}
			prp2 = prp2->p_prpnext;
		}
		UNCRIT(x);
		purge_tlb();
		purge_dcache();
#ifdef PFDAT32
	} else if (prp->p_hdl.p_hdlflags & PHDL_ATLS) {
#else
	} else
#endif
	      if (((vas_t *)space)->va_hdl.va_seg != NULL)
		      atl_deletetrans(prp, space, vaddr, 1);
#ifdef PFDAT32
	/*
	 *	check stuff out; if it looks good, zap both the PTE
	 *	(which pf_kpp is pointing at) and pf_kpp itself, since
	 *	there is no longer a good PTE to point at (the DBC is
	 *	managing its own "free" page frames; if we don't do this,
	 *	the DBC will associate the PTE with some other page and
	 *	hdl_unvirtualize will have problems
	 */
	} else if (prp->p_hdl.p_hdlflags & PHDL_DPTES) {
		pfd = pfdat + pfn;
		if (pfd->pf_hdl.pf_kpp == NULL ||
		    (pfd->pf_hdl.pf_kpp->pg_pfnum && indirect_ptes &&
		     pfd->pf_hdl.pf_kpp->pg_pfnum != pfd->pf_hdl.pf_rw_pte.pg_pfnum)) 
			printf("deltrans pg_pfn is confused; pfd = %x\n", pfd); 
		if (pfd->pf_hdl.pf_bits & PFHDL_DPTES) {
			*(unsigned int *)pfd->pf_hdl.pf_kpp = 0;
			pfd->pf_hdl.pf_kpp = 0;
		} else
			printf("deltrans pf_bits confused; pfd = %x\n", pfd); 
	}
#endif
}



kern_addtrans(vaddr, pfn)
	register caddr_t vaddr;
	register int pfn;
{
	register unsigned int *pt;

#ifdef PFDAT32
	VASSERT(pfd_is_locked(&pfdat[pfn]));
#else
	VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif

	/* Allocate new page table if necessary */
	pt = vm_alloc_tables((vas_t *)KERNVAS, vaddr);
	VASSERT(pt);

	/* Store the new pte */
	*pt = (hiltopfn(pfn) << PGSHIFT) | PG_V;

	purge_dcache();			 /*   XXX  overkill?  */
	purge_tlb_select_super(vaddr);
	return 0;
}

kern_deletetrans(vaddr, pfn)
caddr_t vaddr;
int pfn;
{
	register struct pte *pte;

#ifdef PFDAT32
	VASSERT(pfd_is_locked(&pfdat[pfn]));
#else
	VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif

	pte = (struct pte *)itablewalk(Syssegtab, vaddr);
	VASSERT(pte && pte->pg_v);

	/* blow away the page table entry */
	*(int *)pte = 0;

	purge_dcache();			 /*   XXX  overkill?  */
	purge_tlb_select_super(vaddr);
}


/*
 * Hardware dependent user protect routine.
 *
 * The HDL user_protect routine is responsible for:
 *	- taking the given page (pfn), with the given pregion (prp)
 *	  at the specified virtual address (space, vaddr) and making sure
 *	  that NO user can access the page.
 *	- returning in nspace/nvaddr a 64-bit address at which the
 *	  kernel may reference the protected data
 *	- if "steal" is false, it is intended that the translation continue
 *	  to exist, and that the user be able to reference through it when
 *	  the hdl_user_unprotect is later applied.
 *	- if "steal" is true, it is not intended that the user ever be able
 *	  to reference through the translation again.
 *
 * For 300 we:
 *	XXX NEW DOC NEEDED
 *	- Set mapping R/W so kernel can movs through it XXX for now,should
 *	create a private kernel mapping to protect from user.
 */
/*ARGSUSED*/
hdl_user_protect(prp, space, vaddr, count, nspace, nvaddr, steal)
preg_t *prp;
space_t space;
caddr_t vaddr;
int count;
space_t *nspace;
caddr_t *nvaddr;
int steal;
{
	caddr_t kva;
	int pfn;
	pfd_t *pfd;
	int pgindx, i, j;
#ifdef PFDAT32
	unsigned int *pp;
#endif

	/*
	 * Get the starting page index.
	 */
	pgindx = regindx(prp, vaddr);

	/*
	 * If count is one page, use the ttlb window instead of allocating
	 * kernel virtual address space.
	 */
	if (count == 1) {
		vfd_t *vfdp;
		int pfn;

		/* Get the page number from HIL */
		vfdp = FINDVFD(prp->p_reg, pgindx);
		pfn = vfdp->pgm.pg_pfn;
		pfd = pfdat+pfn;

		/* If appropriate, take all user translations */
		if (steal) {
			struct pte *pt = vtopte(prp, vaddr - prp->p_vaddr);
			if (pt)
				pt->pg_v = 0;
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				int x, idx;
				preg_t *prp2;
				struct pte *pte;
				VASSERT(pfd->pf_hdl.pf_bits & PFHDL_IPTES);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);

				x = CRIT();
				prp2 = prp->p_reg->r_pregs;
				while (prp2 != (preg_t *)NULL) {
					idx = pgindx - prp2->p_off;
					if ((idx >= 0) && (idx < prp2->p_count)) {
						pte = vtoipte(prp2, ptob(idx));
						if (pte)
							*(int *)pte = 0;
					}
					prp2 = prp2->p_prpnext;
				}
				UNCRIT(x);
				*(int *)&pfd->pf_hdl.pf_rw_pte &=
							~(PG_V | PG_M | PG_REF);
				*(int *)&pfd->pf_hdl.pf_ro_pte &=
							~(PG_V | PG_M | PG_REF);
				pfd->pf_hdl.pf_bits &=
#ifdef PFDAT32
					~(VPG_MOD | VPG_REF | PFHDL_COPYFRONT);
#else
					~(VPG_MOD | VPG_REF | PF_COPYFRONT);
#endif
				purge_tlb();
#ifdef OSDEBUG
				if ((check_iptes) && (pfd->pf_use <= 1))
					vl_find_iptes(pfn, 1);
#endif /* OSDEBUG */
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
#else
			} else
#endif
				atl_steal(pfn);
#ifdef PFDAT32
			else {
				if (pp = (uint *) tablewalk(Syssegtab, vaddr))
					*pp &= ~(PG_V | PG_M | PG_REF);
				else
					printf("tablewalk failed: pfd is %x\n", pfd);
				pfd->pf_hdl.pf_bits &= ~(VPG_MOD | VPG_REF);
				purge_tlb();
			}
#endif
		} else {
			/*
			 * Note: we must add in a translation for this
			 * vas; so that unprotect can enable it.
			 */
			hdl_addtrans(prp, space, vaddr, vfdp->pgm.pg_cw, pfn);

			/*
			 * Remove all of the translations.
			 */
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
				if (pfd->pf_hdl.pf_rw_pte.pg_v) {
					pfd->pf_hdl.pf_bits |= PFHDL_RWPTE_V;
					pfd->pf_hdl.pf_rw_pte.pg_v = 0;
				}
				if (pfd->pf_hdl.pf_ro_pte.pg_v) {
					pfd->pf_hdl.pf_bits |= PFHDL_ROPTE_V;
					pfd->pf_hdl.pf_ro_pte.pg_v = 0;
				}
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) {
				atl_pfdat_do(pfn, atl_prot, 0);
				VASSERT((pfd->pf_hdl.pf_bits & PFHDL_PROTECTED) == 0);
				pfd->pf_hdl.pf_bits |= PFHDL_PROTECTED;
				pfd->pf_hdl.pf_bits &= ~(PFHDL_UNSET_REF|PFHDL_UNSET_MOD);
			} else if (pfd->pf_hdl.pf_bits & PFHDL_DPTES) 
				if (pfd->pf_hdl.pf_kpp && pfd->pf_hdl.pf_kpp->pg_v) {
					pfd->pf_hdl.pf_bits |= PFHDL_RWPTE_V;
					pfd->pf_hdl.pf_kpp->pg_v = 0;
				}
#else
			} else
				atl_pfdat_do(pfn, atl_prot, 0);
#endif
		}
#ifndef PFDAT32		
		VASSERT((pfd->pf_hdl.pf_bits & PFHDL_PROTECTED) == 0);
		pfd->pf_hdl.pf_bits |= PFHDL_PROTECTED;
		pfd->pf_hdl.pf_bits &= ~(PFHDL_UNSET_REF|PFHDL_UNSET_MOD);
#endif
		*nspace = KERNELSPACE;
		*nvaddr = ttlbva(pfn);
		purge_dcache();
		purge_tlb_user();
		return;
	}

	/*
	 * get some kernel addresses for this chunk
	 */
	if ((kva = (caddr_t)ptob(rmalloc(sysmap, count))) == 0) {
		panic("hdl_user_protect: Out of system Virtual space.\n");
	}
	kva -= ptob(1);

	/*
	 * Starting at pgindx, for count pages, verify the
	 * translation and disable user access rights.
	 */
	for (i = 0, j = pgindx; i < count; i++, j++) {
		vfd_t *vfdp;

		/* Get the page number from HIL */
		vfdp = FINDVFD(prp->p_reg, j);
		pfn = vfdp->pgm.pg_pfn;
		pfd = pfdat+pfn;
#ifdef PFDAT32
		if (indirect_ptes == 0) {		
			VASSERT((pfd->pf_hdl.pf_bits & PFHDL_PROTECTED) == 0);
			pfd->pf_hdl.pf_bits |= PFHDL_PROTECTED;
			pfd->pf_hdl.pf_bits &= ~(PFHDL_UNSET_REF|PFHDL_UNSET_MOD);
		}
#else
		VASSERT((pfd->pf_hdl.pf_bits & PFHDL_PROTECTED) == 0);
		pfd->pf_hdl.pf_bits |= PFHDL_PROTECTED;
		pfd->pf_hdl.pf_bits &= ~(PFHDL_UNSET_REF|PFHDL_UNSET_MOD);
#endif
		/* If appropriate, take all user translations */
		if (steal) {
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				int x, idx;
				preg_t *prp2;
				struct pte *pte;
				VASSERT(pfd->pf_hdl.pf_bits & PFHDL_IPTES);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);

				x = CRIT();
				prp2 = prp->p_reg->r_pregs;
				while (prp2 != (preg_t *)NULL) {
					idx = j - prp2->p_off;
					if ((idx >= 0) && (idx < prp2->p_count)) {
						pte = vtoipte(prp2, ptob(idx));
						if (pte)
							*(int *)pte = 0;
					}
					prp2 = prp2->p_prpnext;
				}
				UNCRIT(x);
				*(int *)&pfd->pf_hdl.pf_rw_pte &=
							~(PG_V | PG_M | PG_REF);
				*(int *)&pfd->pf_hdl.pf_ro_pte &=
							~(PG_V | PG_M | PG_REF);
				pfd->pf_hdl.pf_bits &=
#ifdef PFDAT32
					~(VPG_MOD | VPG_REF | PFHDL_COPYFRONT);
#else
					~(VPG_MOD | VPG_REF | PF_COPYFRONT);
#endif
				purge_tlb();
#ifdef OSDEBUG
				if ((check_iptes) && (pfd->pf_use <= 1))
					vl_find_iptes(pfn, 1);
#endif /* OSDEBUG */
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
#else
			} else
#endif
				atl_steal(pfn);
#ifdef PFDAT32
			else {
				if (pp = (uint *) tablewalk(Syssegtab, vaddr+i*NBPG))
					*pp &= ~(PG_V | PG_M | PG_REF);
				else
					printf("tablewalk 2 failed: pfd is %x\n", pfd);
				pfd->pf_hdl.pf_bits &= ~(VPG_MOD | VPG_REF);
				purge_tlb();
			}
#endif
		} else {
			/*
			 * Note: we must add in a translation for this
			 * vas; so that unprotect can enable it.
			 */
			hdl_addtrans(prp, space, vaddr + ptob(i),
				vfdp->pgm.pg_cw, pfn);

			/*
			 * Remove all of the translations.
			 */
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
				if (pfd->pf_hdl.pf_rw_pte.pg_v) {
					pfd->pf_hdl.pf_bits |= PFHDL_RWPTE_V;
					pfd->pf_hdl.pf_rw_pte.pg_v = 0;
				}
				if (pfd->pf_hdl.pf_ro_pte.pg_v) {
					pfd->pf_hdl.pf_bits |= PFHDL_ROPTE_V;
					pfd->pf_hdl.pf_ro_pte.pg_v = 0;
				}
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_DPTES) {
				if (pfd->pf_hdl.pf_kpp && pfd->pf_hdl.pf_kpp->pg_v) {
					pfd->pf_hdl.pf_bits |= PFHDL_RWPTE_V;
					pfd->pf_hdl.pf_kpp->pg_v = 0;
				}
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
#else
			} else
#endif
				atl_pfdat_do(pfn,  atl_prot, 0);
#ifdef PFDAT32
			else
				printf("tablewalk 3 failed: pfd is %x\n", pfd);
#endif
		}

		/* Add a kernel translation */
		kern_addtrans(kva+ptob(i), pfn);
	}

	/* Fill in our caller's variables */
	*nspace = KERNELSPACE;
	*nvaddr = kva;

	purge_dcache();
	purge_tlb_user();
}

/*
 * Hardware dependent user unprotect routine.
 *
 * NEED NEW DOCUMENTATION
 *
 * The HDL user_unprotect routine is responsible for:
 *	- taking the given page (pfn), with the given pregion (prp)
 *	  at the specified virtual address (space, vaddr) and making sure
 *	  that what ever protection was established by
 *	  user_protect was removed.
 *
 * For 300 we:
 *	- Restore the validity of user translations.
 *	- Delete the kernel translation for the page.
 *	- XXX an undocumented action of the HPPA procedure is that it
 *	  clears the "modified" bit; I'm going to mimic it for now.
 */
hdl_user_unprotect(space, vaddr, count, steal)
space_t space;
caddr_t vaddr;
register int count;
register int steal;
{
	register struct pte *pt;
	int pfn;
	register pfd_t * pfd;
	register int i;

	if ((count == 1) &&
			((unsigned int)vaddr >= tt_region_addr)) {
		pfn = pfntohil((unsigned int)vaddr >> PGSHIFT);
		pfd = pfdat + pfn;

		if (steal) {
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
				*(int *)&pfd->pf_hdl.pf_rw_pte &=
							~(PG_V | PG_M | PG_REF);
				*(int *)&pfd->pf_hdl.pf_ro_pte &=
							~(PG_V | PG_M | PG_REF);
				pfd->pf_hdl.pf_bits &=
#ifdef PFDAT32
					~(VPG_MOD | VPG_REF | PFHDL_COPYFRONT);
#else
					~(VPG_MOD | VPG_REF | PF_COPYFRONT);
#endif
				purge_tlb();
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
#else
			} else
#endif
				atl_steal(pfn);
#ifdef PFDAT32
			else	
				printf("tablewalk 4 failed: pfd is %x\n", pfd);			
#endif
		} else {
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
				if (pfd->pf_hdl.pf_bits & PFHDL_RWPTE_V)
					pfd->pf_hdl.pf_rw_pte.pg_v = 1;
				if (pfd->pf_hdl.pf_bits & PFHDL_ROPTE_V)
					pfd->pf_hdl.pf_ro_pte.pg_v = 1;
				pfd->pf_hdl.pf_bits &=
					~(PFHDL_RWPTE_V | PFHDL_ROPTE_V);
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_DPTES) {
				if (pfd->pf_hdl.pf_kpp && 
				    pfd->pf_hdl.pf_bits && PFHDL_RWPTE_V) {
					pfd->pf_hdl.pf_kpp->pg_v = 1;
				        pfd->pf_hdl.pf_bits &= ~PFHDL_RWPTE_V;
				}
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
#else
			} else
#endif
				atl_pfdat_do(pfn,  atl_prot, 1);
		}

#ifdef PFDAT32
		if (indirect_ptes == 0)
			pfd->pf_hdl.pf_bits &= ~PFHDL_PROTECTED;
#else
		pfd->pf_hdl.pf_bits &= ~PFHDL_PROTECTED;
#endif
		purge_dcache();
		purge_tlb_user();
		return;
	}

	VASSERT(space == KERNELSPACE);
	for (i = 0; i < count; i++) {
		pt = vastopte((vas_t *)space, vaddr+ptob(i));
		VASSERT(pt != NULL);

		/* Get rid of the temporary kernel translation */
		pfn = pfntohil(pt->pg_pfnum);
		pfd = pfdat + pfn;
		kern_deletetrans(vaddr+ptob(i), pfn);

		if (steal) {
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
				*(int *)&pfd->pf_hdl.pf_rw_pte &=
							~(PG_V | PG_M | PG_REF);
				*(int *)&pfd->pf_hdl.pf_ro_pte &=
							~(PG_V | PG_M | PG_REF);
				pfd->pf_hdl.pf_bits &=
#ifdef PFDAT32
					~(VPG_MOD | VPG_REF | PFHDL_COPYFRONT);
#else
					~(VPG_MOD | VPG_REF | PF_COPYFRONT);
#endif
				purge_tlb();
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
#else
			} else
#endif
				atl_steal(pfn);
#ifdef PFDAT32
			else
				printf("tablewalk 6 failed: pfd is %x\n", pfd);			
#endif
			VASSERT((hdl_getbits(pfn) & (VPG_MOD|VPG_REF))==0);
		} else {
			if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
				VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
				if (pfd->pf_hdl.pf_bits & PFHDL_RWPTE_V)
					pfd->pf_hdl.pf_rw_pte.pg_v = 1;
				if (pfd->pf_hdl.pf_bits & PFHDL_ROPTE_V)
					pfd->pf_hdl.pf_ro_pte.pg_v = 1;
				pfd->pf_hdl.pf_bits &=
					~(PFHDL_RWPTE_V | PFHDL_ROPTE_V);
#ifdef PFDAT32
			} else if (pfd->pf_hdl.pf_bits & PFHDL_DPTES) {
				if (pfd->pf_hdl.pf_kpp && 
				    pfd->pf_hdl.pf_bits && PFHDL_RWPTE_V) {
					pfd->pf_hdl.pf_kpp->pg_v = 1;
				        pfd->pf_hdl.pf_bits &= ~PFHDL_RWPTE_V;
				}
			} else if (pfd->pf_hdl.pf_bits & PFHDL_ATLS) 
#else
			} else
#endif
				atl_pfdat_do(pfn,  atl_prot, 1);
		}
#ifdef PFDAT32
		if (indirect_ptes == 0)
			pfd->pf_hdl.pf_bits &= ~PFHDL_PROTECTED;
#else
		pfd->pf_hdl.pf_bits &= ~PFHDL_PROTECTED;
#endif
	}
	purge_tlb_user();

	/* Return kernel addrs to address pool */
	rmfree(sysmap, count, (caddr_t) btorp((unsigned)vaddr + ptob(1)));
}
