/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/pte.c,v $
 * $Revision: 1.10.84.5 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/12 17:02:41 $
 */

/*
 * pte.c--support routines for managing PTEs, BTEs, and STEs
 *
 *  This file is intended to abstract the actual parameters of a 680x0-type
 *	VM system.  Its intent is to encapsulate the differences between
 *	various Series 300 MMUs.  In particular translation table tree depth,
 *	so that all other code can operate against one uniform MMU model.
 */
#include "../h/debug.h"
#include "../h/param.h"
#include "../machine/vmparam.h"
#include "../h/user.h"
#include "../h/types.h"
#include "../machine/pte.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/pfdat.h"
#include "../h/vm.h"

#define WAITOK 1
extern int mainentered;
extern caddr_t firstfit();
#ifdef OSDEBUG
extern int check_iptes;
extern int (*kdb_printf)();
#endif

/*
 * Allocate a range of virtual addresses for a region of the given type.
 */
vm_alloc(vas, prp)
	register vas_t *vas;
	register preg_t *prp;
{
	register preg_t *pr, *hintprp;

	/*
	 * If they asked for an exact virtual address, accomodate them.
	 */
	if (prp->p_flags & PF_EXACT) {
		/* Exact attaches must be page aligned */
		if ((unsigned int)prp->p_vaddr & (NBPG - 1)) {
			VASSERT(vas != KERNVAS);
			u.u_error = EINVAL;
			return(-1);
		}
		if (chkattach(vas, prp, 0))
			return(-1);
		return(0);
	}

	/*
	 * Handle case where vaddr wasn't specified.  For a first try use
	 * the virtual address just past the pregion pointed to by the hint.
	 */
	hintprp = vas->va_hdl.va_attach_hint;
	if (hintprp != (preg_t *) NULL)
		prp->p_vaddr = (caddr_t)((u_int)hintprp->p_vaddr + 
							ptob(hintprp->p_count));
	else
		prp->p_vaddr = (caddr_t)INIT_ATTCHPT;

	/*
	 * Check the atach point and if it is acceptable update the attach
	 * point hint in the vas and return the virtual address of the attach
	 * point.
	 */
	if (!chkattach(vas, prp, 0)) {
		vas->va_hdl.va_attach_hint = prp;
		return(0);
	}
	u.u_error = 0;

	/*
	 * The above algirithm failed to produce an acceptable attach point.
	 * Do a first fit search starting at INIT_ATTCHPT and ending at the
	 * end of the virtual address space.
	 */
	prp->p_vaddr = firstfit(vas, INIT_ATTCHPT, 0xffffffff, prp->p_count);

	/*
	 * Check the atach point and if it is acceptable update the attach
	 * point hint in the vas and return the virtual address of the attach
	 * point.
	 */
	if (!chkattach(vas, prp, 0)) {
		vas->va_hdl.va_attach_hint = prp;
		return(0);
	}
	u.u_error = 0;

	/*
	 * The above algirithm failed to produce an acceptable attach point.
	 * Do a first fit search starting at the beginning and ending at the
	 * end of the virtual address space.
	 */
	prp->p_vaddr = firstfit(vas, 0, 0xffffffff, prp->p_count);

	/*
	 * Check the atach point and if it is acceptable update the attach
	 * point hint in the vas and return the virtual address of the attach
	 * point.
	 */
	if (!chkattach(vas, prp, 0)) {
		vas->va_hdl.va_attach_hint = prp;
		return(0);
	}
	u.u_error = 0;

	/* Can't fit in the VM space */
	return(-1);
}

/*
 * Allocate virtual memory translation mapping tables.
 *
 * Given a vas and a virtual address, this routine makes sure that the
 * page table mapping this address exists.  If the page table does not
 * exist, then it is allocated and attached to the translation table
 * tree for this vas.  For MMUs using a 3 level table depth this may
 * mean allocating a block table.  This routine returns a pointer to
 * the page table entry mapping vaddr.  Note that the calling vas is
 * assumed to have a valid segment table.
 */
unsigned int
vm_alloc_tables(vas, vaddr)
	vas_t *vas;
	register caddr_t vaddr;
{
	register unsigned int segment_table = (unsigned int)vas->va_hdl.va_seg;
	register unsigned int pt, pte;
	register unsigned int ste;

	VASSERT(segment_table);

	/* If we already have the page table entry, just return it. */
	if (pte = (unsigned int)itablewalk(segment_table, vaddr))
		return(pte);

	/* If we got here we didn't have a page table */
	if (three_level_tables) {
		register struct bte *bte;

		/* Get a pointer to the segment table entry for vaddr */
		ste = segment_table + MC68040_SEGOFF(vaddr);

		/* If its NULL then we need to allocate a block table */
		if (*(int *)ste == NULL) {
			unsigned int taddr;

			/* Keep track of valid segments for fast management */
			if (new_vste(vas, MC68040_SEGOFF(vaddr)) == 0)
				return 0;

			/* Allocate a block table and make it valid */
			taddr = ttw_alloc(blocktablemap, NBBT, ZERO_MEM);

			/* if we couldn't get memory the backout and fail */
			if (taddr == 0) {
				free_vste(vas, vaddr);
				return 0;
			}

			/* make the ste valid and store it in the segtab */
			*(int *)ste = taddr | SG_V; 

		}

		/* allocate the page table */
		pt = ttw_alloc(pagetablemap, MC68040PTSIZE, ZERO_MEM);
		if (pt == 0)
			return 0;

		/*
		 * Now that we are sure that we have a block table, pick
		 * up the appropriate block table entry for vaddr.
		 */
		bte = (struct bte *)((*(int *)ste & SG3_FRAME) + 
							MC68040_BTOFF(vaddr));

		/* Attach the page table to the block table and make it valid */
		*(int *)bte = pt | BT_V;

		/* Get a pointer to the page table entry for return value */
		pte = pt + MC68040_PTOFF(vaddr);

	} else {	/* Two Level Tables */

		/* Keep track of valid segments for fast management */
		if (new_vste(vas, MC68030_SEGOFF(vaddr)) == 0)
			return 0;

		/* try to allocate the page table */
		if ((pt = alloc_page(ZERO_MEM)) == 0) {
			free_vste(vas, vaddr);
			return 0;
		}

		/* Get a pointer to the segment table entry for vaddr */
		ste = segment_table + MC68030_SEGOFF(vaddr);

		/* Attach page table to the segment table and make it valid */
		*(int *)ste = pt | SG_V;

		/* Get a pointer to the page table entry for return value */
		pte = pt + MC68030_PTOFF(vaddr);
	}

	/*
	 * Since the page table did not exist there could not have been a 
	 * translation for this address in the tlb so we don't need to purge it.
	 * This assumes that the tlb is purged when translations are destroyed
	 * and tables are deallocated.  This is taken care of by vm_dealloc().
	 */

	/* return a pointer to the pte mapping vaddr */
	return(pte);
}

/*
 * Remove any valid translations for the npages starting at vaddr for the
 * given pregion.  This routine is used for both removing entire pregions
 * and for trimming back the size (shrinking) of pregions.
 */
vm_dealloc(prp, vaddr, npages)
	preg_t *prp;
	register unsigned int vaddr;
	int npages;
{
	register int nptables;
	register unsigned int lastvaddr = vaddr + ptob(npages) - 1;
	register int x;
	vas_t *vas;
	unsigned int segtab;
	unsigned int ste;
	int segoff, firstptoff, lastptoff, leftref, rightref;


	/*
	 * We should always be freeing at least one page.
	 * We must have a pregion pointer.
	 * We can't clear more page references than there are pages
	 * in the pregion.
	 */
	VASSERT( prp );
	VASSERT( npages > 0 );
	VASSERT( (vaddr & 0xfff) == 0 );
	VASSERT( vaddr >= (unsigned int)prp->p_vaddr );
	VASSERT( lastvaddr <= (unsigned int)prp->p_vaddr 
			      + (unsigned int)prp->p_count*NBPG - 1 );

	/*
	 * Get pregion's vas and segment table.
	 */
	vas = (vas_t *)prp->p_vas;
	segtab = (unsigned int)vas->va_hdl.va_seg;

	/* 
	 * The following code is a performance enhancement.  If we know that
	 * this pregion belongs to the current running process and that the
	 * process is doing an exit() then we can skip removing the translations
	 * here.  The process is entering the zombie state and will not be run
	 * again so we don't have to worry about hits in the TLB.  The page
	 * tables will be cleaned up in a much more efficient manner by
	 * vm_dealloc_tables() which is called when the process is finally
	 * collected by kissofdeath().  Note that this has the side effect of
	 * holding on to the page table resources longer than otherwise.
	 *
	 * The test for mainentered is needed before we can look at the uarea
	 * because it isn't setup until after mainentered.
	 */
	if (mainentered && (u.u_procp->p_vas == vas)
			&& (u.u_procp->p_flag & SWEXIT)) {
#ifdef OSDEBUG
		/*
		 * If the check_iptes code is on we have to remove the 
		 * translations because the ipte debug code walks the tables
		 * counting iptes.
		 */
		if (check_iptes == 0)
#endif /* OSDEBUG */
		return;
	}

	/* 
	 * If the segment table isn't allocated then there can't be any
	 * translations to remove or page tables to clean up.  If this is
	 * a uarea then we know we are being called by hdl_procdetach() 
	 * via kissofdeath() and that this is the last pregion.  We skip
	 * the page table cleanup here as vm_dealloc_tables() will be called
	 * on our return to hdl_procdetach().
	 */
	
	if ((segtab == NULL) || (prp->p_type  == PT_UAREA))
		return;

	if (three_level_tables) {
		register unsigned int bte;
		int nbtables;
		int btoff;

		/* 
		 * Compute the number of block tables referenced 
		 */
		nbtables = MC68040_SEGIDX(lastvaddr)-MC68040_SEGIDX(vaddr)+1;

		/*
		 * Compute the number of page tables we're going to examine.
		 * There are one or more block tables.
		 * The one block table collapses when (nbtables-1)==0
		 * Remember, idxs start at 0.
		 *
		 * Those used in the first block table are:
		 * 	NBTEBT - MC68040_BTIDX(vaddr);
		 *
		 * Those used in the next (nbtables - 2) are:
		 *	(nbtables - 2) * NBTEBT
		 *
		 * Those used in the last block table are:
		 *	MC68040_BTIDX(lastvaddr) + 1
		 *
		 * Adding them all up we have...
		 */
		nptables=(nbtables - 1) * NBTEBT	
			 + MC68040_BTIDX(lastvaddr) 	
			 - MC68040_BTIDX(vaddr) + 1;

		/*
		 * Get address of first block table for this pregion
		 */
		segoff = MC68040_SEGOFF(vaddr);
		btoff  = MC68040_BTOFF(vaddr);
		ste = segtab + segoff;
		if (*(int *)ste) {
			bte = stetobt(ste) + btoff;
		} else
			bte = NULL;

		/* 
		 * Determine if we are sharing a PAGE table with the
		 * previous (left) or next (right) pregion. Boolean.
		 */
		leftref  = MC68040_LREF(prp, segoff, btoff);
		rightref = MC68040_RREF(prp, MC68040_SEGOFF(lastvaddr),
					MC68040_BTOFF(lastvaddr));

		/*
		 * Determine offsets into page tables
		 */
		firstptoff = MC68040_PTOFF(vaddr);
		lastptoff  = MC68040_PTOFF(lastvaddr);

		/* 
		 * One page table special case.
		 */
		if (nptables == 1) {
			/* 
			 * If the block table isn't allocated then there 
			 * can't be a page table so just return.
			 */
			if (bte == NULL)
				return;

			/*
			 * If the block table entry is NULL then the page 
			 * table isn't allocated so just return.
			 */
			if (*(int *)bte == NULL)
				return;

			/*
			 * If we are sharing the page table, clear only those
			 * references that we were using, otherwise free the
			 * whole page table. If any changes are made, flush
			 * the TLB appropriately.
			 */
                        if (leftref || rightref || (npages < prp->p_count)) {
				bzero(btetop(bte) + firstptoff,
				      lastptoff - firstptoff + 4);
                        } else {
                                ttw_free(pagetablemap, btetop(bte),
					 MC68040PTSIZE);
				*(int *)bte = 0;
			}

			/* 
			 * Flush TLB and return
			 */
			if (prp->p_space == KERNELSPACE)
				purge_tlb_super();
			else
				purge_tlb_user();

			return;
		}
         
		/*
		 * More than one page table to touch.
		 * First page table is a special case as it may be shared.
		 */
		if (bte == NULL) {
			/*
			 * No block table!
			 * We count as "cleared" the number of PAGE tables
			 * to which this BLOCK table would have pointed.
			 */
			nptables -= (NBTEBT - MC68040_BTIDX(vaddr));
		} else {
			/*
			 * Does this block table entry point to a page table?
			 */
			if (*(int *)bte) {
				/*
				 * Is it shared?
				 */
				if (leftref) {
					bzero(btetop(bte)+firstptoff, 
					      NBPT3-firstptoff);
				} else {
					ttw_free(pagetablemap, btetop(bte), 
						 MC68040PTSIZE);
					*(int *)bte = 0;
				}
			}

			/*
			 * We've "cleared" one PAGE table; the one
			 * to which the block table entry pointed.
			 */
			nptables--;

			/*
		 	 * Advance the block table pointer.
			 */
			bte += 4;
		}

		/* 
		 * If we had no block table entry or if after 
		 * incrementing bte it walked off the end of the current
		 * block table, use the next segment table entry to find
		 * the next bte; it may be NULL.
		 */
		if ( (bte == NULL) || ((bte & (NBBT - 1)) == 0) ) {
			ste += 4;
			if (*(int *)ste) {
				bte = stetobt(ste);
			} else {	
				bte = NULL;
			}
		}

		/*
		 * Free whole PAGE tables in the "middle" of the address range.
		 * These page tables are NOT shared, so we simply free them.
		 * Loop until only one page table or less remains; we might
		 * "skip" the last page table if an empty segment table
		 * entries (bte == NULL) exist.
		 */
		while (nptables > 1) {
			if (bte == NULL) {
				/*
			 	 * No block table!
			 	 * We "count" the number of page tables to
			 	 * which this block table would have pointed.
				 * This could make nptables <= 0.
			 	 */
				nptables -= NBTEBT;
			} else {
				/*
				 * Do we point to anything?
				 */
                        	if (*(int *)bte) {
                                	ttw_free(pagetablemap, btetop(bte),
						 MC68040PTSIZE);
                                	*(int *)bte = 0;
				}

				/*
				 * We've "cleared" one PAGE table; the one
				 * to which the block table entry pointed.
				 */
                                nptables--;

				/*
			 	 * Advance the block table pointer.
				 */
				bte += 4;
			}

			/* 
		 	 * If we had no block table entry or if after 
		 	 * incrementing bte it walked off the end of the current
		 	 * block table, use the next segment table entry to find
		 	 * the next bte; it may be NULL.
		 	 */
			if ( (bte == NULL) || ((bte & (NBBT - 1)) == 0) ) {
				ste += 4;
				if (*(int *)ste) {
					bte = stetobt(ste);
				} else {
					bte = NULL;
				}
			}
		}

		/* 
		 * Last PAGE table is a special case - it may be shared.
		 * At this point, if we have any page tables left
		 * to clear, there should only be ONE.
		 */
		VASSERT( nptables <= 1 );
		if ((nptables == 1)  && bte && *(int *)bte) {
			/*
			 * Is it shared?
			 */
			if (rightref) {
				bzero(btetop(bte), lastptoff + 4);
			} else {
				ttw_free(pagetablemap, btetop(bte), 
					 MC68040PTSIZE);
				*(int *)bte = 0;
			}
		}

	} else { /* two level tables */

		nptables = MC68030_SEGIDX(lastvaddr)-MC68030_SEGIDX(vaddr)+1;
		segoff = MC68030_SEGOFF(vaddr);
		ste = segtab + segoff;
		leftref = MC68030_LREF(prp, segoff);
		rightref = MC68030_RREF(prp, MC68030_SEGOFF(lastvaddr));

		/* Are we clearing out translations in just one page table? */
		if (nptables == 1) {
			/* If no page table to clean up just return */
			if (*(int *)ste == NULL) 
				return;

			firstptoff = MC68030_PTOFF(vaddr);
			lastptoff = MC68030_PTOFF(lastvaddr);

			if ((npages < prp->p_count) || leftref || rightref){
				bzero(stetop(ste) + firstptoff,
					lastptoff - firstptoff + 4);
			} else {
				free_page(stetop(ste));
				*(int *)ste = 0;
				free_vste(vas, vaddr);
			}

			/* purge the appropriate TLB */
			if (prp->p_space == KERNELSPACE)
				purge_tlb_super();
			else
				purge_tlb_user();
			return;
		}

		/*
		 * We now have more than one table's worth to deal with!
		 * Clear any translations from the first table
		 */
		if (*(int *)ste) {
			if (leftref) {
				firstptoff = MC68030_PTOFF(vaddr);
				bzero(stetop(ste)+firstptoff, NBPT-firstptoff);
			} else {
				free_page(stetop(ste));
				*(int *)ste = 0;
				free_vste(vas, vaddr);
			}
		}

		/* 
		 * We have cleaned out one of the page tables.  
		 * Assert that there are more to clean up.
		 */
		nptables--;
		VASSERT(nptables >= 1);

		/* round vaddr up to the next page table */
		vaddr = (vaddr + (NPTEPT * NBPG)) & ~((NPTEPT * NBPG) - 1);

		/* advance the segment table pointer */
		ste += 4;

		/* Free whole page tables in the middle of the address range */
		for (x = 1; x < nptables; x++) {
			if (*(int *)ste) {
				free_page(stetop(ste));
				*(int *)ste = 0;
				free_vste(vas, vaddr);
			}
			vaddr += ptob(NPTEPT);
			ste += 4;
		}

		/* now clear out the last page table */
		VASSERT(x == nptables);
		if (*(int *)ste) {
			if (rightref) {
				lastptoff = MC68030_PTOFF(lastvaddr);
				bzero(stetop(ste), lastptoff + 4);
			} else {
				free_page(stetop(ste));
				*(int *)ste = 0;
				free_vste(vas, vaddr);
			}
		}

	}

	/* purge the appropriate TLB */
	if (prp->p_space == KERNELSPACE)
		purge_tlb_super();
	else
		purge_tlb_user();
}

vm_dealloc_tables(vas)
	register vas_t *vas;
{
	register unsigned int segment_table = (unsigned int)vas->va_hdl.va_seg;
	register struct vste *vste = vas->va_hdl.va_vsegs;
	register unsigned int ste;
	register int j, x;

	if (segment_table == NULL)
		return;

	x = CRIT();
	while (vste != NULL) {
		ste = segment_table + vste->vste_offset;
		VASSERT(*(int *)ste);
		if (three_level_tables) {
			unsigned int bte;
			bte = stetobt(ste);
			for (j = 0; j < NBTEBT; j++, bte+=4) {
				if (*(int *)bte)	
					ttw_free(pagetablemap, btetop(bte), 
								MC68040PTSIZE);
			}
			ttw_free(blocktablemap, ste3top(ste), NBBT);
		} else
			free_page(stetop(ste));
		ttw_free(validsegmap, vste, sizeof(struct vste));
		vste = vste->vste_next;

	}
	free_segtab(segment_table);
	vas->va_hdl.va_vsegs = NULL;
	vas->va_hdl.va_seg = NULL;
	vas->va_hdl.va_attach_hint = NULL;
	purge_tlb_user();
	UNCRIT(x);
}

vm_dup_tables(svas, dvas)
	register vas_t *dvas, *svas;
{
	unsigned int ssegtab = (unsigned int)svas->va_hdl.va_seg;
	register struct vste *vste = svas->va_hdl.va_vsegs;
	unsigned int sste, dste;
	unsigned int dsegtab;
	int x;

	x = CRIT();

        /*
         * Assert that the parent has page tables and the child does NOT!
         */
        VASSERT(svas->va_hdl.va_seg);
        if (dvas->va_hdl.va_seg == NULL) {
		if (!ON_ISTACK) procmemreserve(1, WAITOK, (reg_t *)0);
		dvas->va_hdl.va_seg = (struct ste *)alloc_segtab();
		if (!ON_ISTACK) procmemunreserve();
	}

	dsegtab = (unsigned int)dvas->va_hdl.va_seg;

	while (vste != NULL) {
		sste = ssegtab + vste->vste_offset;
		dste = dsegtab + vste->vste_offset;
		VASSERT(*(int *)sste);
		if (three_level_tables) {
			register int j;
			unsigned int sbte;
			unsigned int dbte;

			/* reserve worst case number of pages we might need */
			if (!ON_ISTACK) procmemreserve(10, WAITOK, (reg_t *)0);

			sbte = stetobt(sste);
			if (*(int *)dste == NULL) {
				*(int *)dste = 
				       ttw_alloc(blocktablemap, NBBT, ZERO_MEM)
									| SG_V;
				new_vste(dvas, vste->vste_offset);
			}
			dbte = stetobt(dste);
			for (j = 0; j < NBTEBT; j++, sbte+=4, dbte+=4) {
				if (*(int *)sbte) {
				      int table = ttw_alloc(pagetablemap, 
						MC68040PTSIZE, DONT_ZERO_MEM);
				      *(int *)dbte = table | BT_V;
				      pg_copy256((*(int *)sbte)&BLK_FRAME,table);
				}
			}
		} else {
			int table;

			/* reserve worst case number of pages we might need */
			if (!ON_ISTACK) procmemreserve(2, WAITOK, (reg_t *)0);

			table = alloc_page(DONT_ZERO_MEM);
			*(int *)dste = table | SG_V;
			pg_copy4096(stetop(sste), table);
			new_vste(dvas, vste->vste_offset);
		}

		/* Release whatever memory is no longer needed */
		if (!ON_ISTACK) procmemunreserve();

		vste = vste->vste_next;
	}
	UNCRIT(x);
}

/* 
 * Converts a kernel virtual address to a physical address
 */
caddr_t	
vtop(log_addr)
	unsigned int log_addr;	
{
	struct pte *pte;

	/* the tt window is equivalently mapped */
	if (log_addr >= tt_region_addr)
		return((caddr_t)log_addr);

	/* get the pte out of the tables */
	pte = vastopte(&kernvas, log_addr);
	VASSERT(pte != (struct pte *)NULL);

	/*
	 * If the page is valid reutn its physical address.
	 */
	if (pte->pg_v)
		return(caddr_t)((*(int *)pte & PG_FRAME)|(log_addr & PGOFSET));
	return((caddr_t) -1);
}

new_vste(vas, offset) 
	vas_t *vas;
	unsigned int offset;
{
	register struct vste *vste;
	register int x;

	vste = (struct vste *)
		ttw_alloc(validsegmap, sizeof(struct vste), ZERO_MEM);

	if (vste == NULL)
		return 0;

	vste->vste_offset = offset;

	x = CRIT();

	/* now insert into the list */
	vste->vste_next = vas->va_hdl.va_vsegs;
	vste->vste_prev = NULL;
	if (vas->va_hdl.va_vsegs)
		vste->vste_next->vste_prev = vste;
	vas->va_hdl.va_vsegs = vste;
	UNCRIT(x);
	return 1;
}

free_vste(vas, vaddr)
	vas_t *vas;
	unsigned int vaddr;
{
	register struct vste *vste = vas->va_hdl.va_vsegs;
	register int off = SEGOFF(vaddr);
	register int x;
	
	x = CRIT();

	/* find the entry with the offset for vaddr */
	while ((vste != NULL) && (vste->vste_offset != off)) 
		vste = vste->vste_next;
	VASSERT(vste);

	/* remove it from the list */
	if (vste->vste_next == NULL) {
		/* since next was null we are at the end of the list */
		if (vste->vste_prev == NULL)
			/* this was the only entry on the list */
			vas->va_hdl.va_vsegs = NULL;
		else
			/* this was the last entry on the list */
			vste->vste_prev->vste_next = NULL;

		/* release the memory */
		ttw_free(validsegmap, vste, sizeof(struct vste));
		UNCRIT(x);
		return;
	}

	/* 
	 * The next entry existed so set it's prev pointer to 
	 * the prev pointer of the entry we are removing.
	 */
	vste->vste_next->vste_prev = vste->vste_prev;

	if (vste->vste_prev == NULL)
		/* we are removing the first entry in the list */
		/* 
		 * We are removing the first entry in the list so
		 * set the list header pointer to the next pointer 
		 * of the entry we are removing.
		 */
		vas->va_hdl.va_vsegs = vste->vste_next;
	else
		/* 
		 * The prev entry existed so set it's next pointer to 
		 * the next pointer of the entry we are removing.
		 */
		vste->vste_prev->vste_next = vste->vste_next;

	/* release the memory */
	ttw_free(validsegmap, vste, sizeof(struct vste));
	UNCRIT(x);
}

/*
 * Allocate one HDL page and zero it out if requested.
 * Return the tt window physical address of the page.
 */
unsigned int
alloc_page(zero)
	int zero;
{
	register pfd_t *pfd;
	extern pfd_t *allocpfd();
	register unsigned int paddr;

	/* notify the swapper that we are stealing a page of memory */
	if (!steal_swap((reg_t *)0, 1, 0))
		panic("alloc_page: steal_swap failed");

        /* Set aside memory */
        memreserve((reg_t *)0, 1);

	/* since this is an HDL page, we cannot swap it */
	steal_lockmem(1);

	/* Get the space */
	pfd = allocpfd();

	/* Make it a wired down system page */
	pfd->pf_flags |= P_SYS|P_HDL;

	/* access through logical == physical tt window */
#ifdef PFDAT32
	paddr = pctopfn((pfd - pfdat)+hil_pbase) << PGSHIFT;
#else 
	paddr = pctopfn(pfd->pf_pfn+hil_pbase) << PGSHIFT;
#endif

	/* zero the page if requested */
	if (zero)
		pg_zero4096(paddr);

	return(paddr);
}

/*
 * Free one physical page
 */
free_page(paddr)
	unsigned int paddr;
{
	register pfd_t *pfd = pfdat + (pfntopc(paddr >> PGSHIFT) - hil_pbase);

	/* notify the swapper that we are returning a page of memory */
	return_swap(1);

	/* give back the lockable mem */
	lockmemunreserve(1);

	/* clean up the flags we set */
	pfd->pf_flags &= ~(P_SYS|P_HDL);

	/* give the page back to the system */
	freepfd(pfd);
}

unsigned int
ttw_alloc(map, size, zero)
	register struct map *map;
	register unsigned int size;
	int zero;
{
	register unsigned int addr;
	register int x;

	x = CRIT();
	
	/* round up to cache line boundry */
	size = (size + 0xF) & 0xFFFFFFF0;
	VASSERT(size && size <= NBPG);
	
	/* try to get the space from the resource map */
	addr = (unsigned int)rmalloc(map, size);

	/* if we didn't get it allocate some more space in the map */
	if (addr == 0) {
		/* try to get a page */
		if ((addr = alloc_page(DONT_ZERO_MEM)) == 0)
			return 0;

		/* free it into the map */
		rmfree(map, NBPG, addr); 

		/* now try and get the space again */ 
		addr = (unsigned int)rmalloc(map, size);
	}
	UNCRIT(x);
	VASSERT(addr);

	if (zero)
	{
		switch(size)
		{
		case 256:
			pg_zero256(addr);
			break;
		case 512:
			pg_zero512(addr);
			break;
		case 4096:
			pg_zero4096(addr);
			break;
		default:
			bzero(addr, size);
			break;
		}
	}
	return(addr);
}

ttw_free(map, entry, size)
	register struct map *map;
	register unsigned int entry;
	register unsigned int size;
{
	register unsigned int addr;
	register int x;
	
	VASSERT(entry);

	x = CRIT();

	/* round up to cache line boundry */
	size = (size + 0xF) & 0xFFFFFFF0;
	VASSERT(size && size <= NBPG);
	
	/* give this one back to the resource map */
	rmfree(map, size, entry);

	/* try to get the whole page it was in */
	addr = (unsigned int)rmget(map, NBPG, pagedown(entry));

	/* 
	 * if we were able to remove the whole page from 
	 * the map then give it back to the system.
	 */
	if (addr != 0)
		free_page(addr);

	/* if this one crossed a page boundry, check the upper page */
	if (pagedown(entry) != pagedown(entry + size - 1)) {
		/* try to get the whole page it was in */
		addr = (unsigned int) rmget(map, NBPG, pagedown(entry+size-1));

		if (addr != 0)
			free_page(addr);
	}
	UNCRIT(x);
}

/*
 * See if the given vas has a pregion whose 32 bit address lies 
 * within the specified range and is not the pregion passed in.
 */
preg_t *
findrange(vas, start, end, oprp)
	vas_t  *vas;
	caddr_t start;
	caddr_t	end;
	preg_t *oprp;
{
	preg_t *prp;
	
	vaslock(vas);
	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		if ((prp->p_vaddr >= start) && (prp->p_vaddr < end) 
				&& (prp != oprp)) {
			vasunlock(vas);
			return(prp);
		}
	}
	vasunlock(vas);
	return((preg_t *)0);
}

/*
 * Firstfit finds a hole of count pages in the current vas.  It returns 
 * either the beginning address of the hole or NULL if no hole can be 
 * found.  It restricts its search to a hole that lies within start and end.
 */
caddr_t
firstfit(vas, start, end, count)
	vas_t  *vas;
	u_int start;
	u_int end;
	size_t count;
{
	preg_t *prp;
	u_int holestart, holeend;
	
	VASSERT((start + ptob(count)) <= end);

        prp = findrange(vas, start, end, (preg_t *)NULL);
        if (prp == NULL)
                return((caddr_t)start);

	if ((start + ptob(count)) <= (u_int)prp->p_vaddr)
		return((caddr_t)start);

	/*
	 * Loop through the vas checking for a fit in the "holes" between
	 * pregions. prp is set to -1 when we've hit the end of the list
	 * we want to check.
	 *
	 * The beginning of the hole is the end of the current pregion +1.
	 * 
	 * The end of the hole is the lesser of the end of our range and
	 * the beginning of the next pregion -1.  When we run out of 
	 * pregions, we use the end of our range and set the prp to -1.
	 *
	 * Note we can get negative holes, but the fit check will kick that
	 * out.
	 */

	vaslock(vas);
	while (prp != (preg_t *)-1) {
		holestart = (u_int)prp->p_vaddr + ptob(prp->p_count);
		if ((prp->p_next != (preg_t *)vas) &&
		    (prp->p_space == prp->p_next->p_space)) {
			holeend = min(end, prp->p_next->p_vaddr - 1);
			prp = prp->p_next;
		}
		else {
			holeend = end;
			prp = (preg_t *)-1;
		}
		if ((holestart + ptob(count) ) <= holeend) {
			vasunlock(vas);
			return((caddr_t)holestart);
		}
	}
	vasunlock(vas);
	return((caddr_t)NULL);
}
