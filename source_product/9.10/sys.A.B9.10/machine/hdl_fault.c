/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_fault.c,v $
 * $Revision: 1.8.84.4 $       $Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:16 $
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/signal.h"
#include "../h/sema.h"
#include "../h/vfd.h"
#include "../h/vas.h"
#include "../h/pfdat.h"
#include "../h/vmmac.h"		/* Page macros */
#include "../h/vmmeter.h" /* For statistics */
#include "../h/vnode.h"
#include "../machine/vmparam.h"
#include "../machine/atl.h"


int xkillcnt = 0;
extern int processor;
extern pfd_t *allocpfd(); /* HPREVISIT - XXX move to pfdat.h drew 12/12/90 */

/*
 * This file contains the hardware dependent fault handling routines. Each 
 * routine in this file is called by a hardware independent routine to
 * handle a fault.  These routines provide hooks; so that implementations
 * can play with protections and virtual addresses in addition to the manner
 * in which the HIL layer uses them.
 */

/*
 * Hardware dependent protection fault routine.
 *
 * The HDL pfault routine is responsible for resolving hardware
 * protection faults which occur on the machine and are
 * not caused by a copy-on-write fault.  This routine must:
 *	- resolve the protection problem and return a 0
 *	  or return a value which will be returned to
 *	  trap if the HIL layer can not resolve the fault.
 *
 * For the 300:
 *	- Do nothing
 */
/*ARGSUSED*/
int
hdl_pfault(prp, wrt, space, vaddr)
	preg_t *prp;
	int wrt;
	int space;
	caddr_t vaddr;
{
	extern unsigned int physpfdat;
	int pfn;

	VASSERT(prp != (preg_t *)0);

        /*
         * If we took a protection violation on an I/O pregion,
         * we must send the process a SIGBUS and stop.
         */
        if (prp->p_type == PT_IO)
                return(-SIGBUS);
	if (space == KERNELSPACE)
		return(-SIGBUS);

	/* If it is a shared memory segment and is not read-only,
	 * turn on the modification bit.  Next, for each pregion 
	 * on the attach list that allows write access, clear
	 * the write protect bit in the associated pte.
	 */
	if (wrt && prp->p_type == PT_SHMEM && (prp->p_prot & PROT_WRITE)) {
		struct pte *pte;
		unsigned int *ipte;
		pfd_t *pfd;

		ipte = (unsigned int *)vastoipte((vas_t *)space, vaddr);
		VASSERT(ipte != (unsigned int *)NULL);

		/* get the real pte if appropriate */
		if (prp->p_hdl.p_hdlflags & PHDL_IPTES) {
			if ((*ipte) & PG_IV)
				pte = (struct pte *)(*ipte & 0xfffffffc);
			else {
				/*
				 * page became invalid due to vhand or sched
				 * while we acquired region lock just return
				 * and we will re-enter the system in vfault.
				 */
				return(0);
			}
		} else {
			pte = (struct pte *)ipte;
		}

		/*
		 * page became invalid due to vhand or sched while we
		 * acquired region lock just return and we will re-enter
		 * the system in vfault.
		 */
		if (!pte->pg_v)
			return(0);

		/* get the page frame number and lock it */
		pfn = pfntohil(pte->pg_pfnum);
		pfd = pfdat+pfn;
		pfnlock(pfn);

		/* set the modify bit */
		pfd->pf_hdl.pf_bits |= VPG_MOD;

		if (prp->p_hdl.p_hdlflags & PHDL_IPTES) {
			VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
			VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);


			/* make the indirect pte point to the real rw pte */
			pte = (struct pte *)&pfd->pf_hdl.pf_rw_pte;
			*ipte = ((int)pte - (int)pfdat + physpfdat) | PG_IV;

			/* clear the write protect bit and make it valid */
			pfd->pf_hdl.pf_rw_pte.pg_prot = 0;
			pfd->pf_hdl.pf_rw_pte.pg_v = 1;
#ifdef PFDAT32
		} else if (prp->p_hdl.p_hdlflags & PHDL_ATLS) 
#else	
		} else
#endif	
			atl_pfdat_do(pfn, atl_promote_write, (caddr_t)NULL);

		pfnunlock(pfn);
		purge_tlb_user();
		return(0);
	}
	return(SIGBUS);
}


/*
 * Hardware dependent virtual fault routine.
 *
 * The HDL vfault routine is responsible for resolving hardware
 * dependent virtual faults that do not relate to bringing a page
 * into memory.  One example of where this is usefull is for stack
 * growth.  The direction and location of the stack and its growth is
 * a hardware dependent concept.
 *
 * 300 HDL vfault routine:
 *
 *	- Detect illegal kernel references and panic.
 *	- Demand-create HDL translations.  Mostly HDL is coherent with HIL,
 *	  but cases like growth can result in incoherence.
 *	- Request copy-on-write when we discover a need for it.
 */
int
hdl_vfault(prp, wrt, space, vaddr)
	preg_t *prp;
	int wrt;
	int space;
	caddr_t vaddr;
{
	register reg_t	*rp;
	register vfd_t	*vfd;
	register int	pgindx;

        /* If no segment table associated with this vas (ie shared mem), then
         * just return.
         */
	if (!(((vas_t *)(space))->va_hdl.va_seg))
		return(SIGSEGV);


	if (!prp) {	/* we were passed a null pregion */
		/* This is fatal from the kernel */
		if (space == KERNELSPACE)
			return(-SIGSEGV);
		return(SIGSEGV);
	}

	/*
	 * vfaults to inactive kernel pregions should be impossible
	 */
	if ((space == KERNELSPACE) && !(prp->p_flags & PF_ACTIVE))
		return(-SIGSEGV);

	/*
	 * If it's a fault in an I/O pregion, either we need to demand-load
	 * the I/O translation (after an hdl_swapout) or we have a true bus
	 * error from the I/O mapping.
	 */
	if (prp->p_type == PT_IO) {
		struct pte *pt;

		pt = vastopte((vas_t *)prp->p_space, vaddr);
		if (pt && pt->pg_v)
			return(-SIGBUS);

		io_addtrans(prp);
		return(0);
	}

	/*	Find vfd as mapped by region
	 */
	pgindx = regindx(prp, vaddr);
	rp = prp->p_reg;
	vfd = FINDVFD(rp, pgindx);

	/*
	 * If the page is now valid (but not copy-on-write) it either
	 * means that the page came in while waiting for the region
	 * lock or this is a demand-fault translation from a fork.
	 */
	if (vfd->pgm.pg_v) {
		if (!wrt || !(vfd->pgm.pg_cw)) {
			/* Translate the page */
			pfnlock(vfd->pgm.pg_pfn);
			if (rp->r_zomb) {
                        	pfnunlock(vfd->pgm.pg_pfn);
                        	printf("Pid %d killed due to text modification or page I/O error\n",
				u.u_procp->p_pid);
                        	xkillcnt++;
                        	return(-SIGKILL);
                	}
			cnt.v_intrans++;
			hdl_addtrans(prp, prp->p_space, vaddr, vfd->pgm.pg_cw,
				     vfd->pgm.pg_pfn);
			pfnunlock(vfd->pgm.pg_pfn);
			return(0);
		}
	}

	/*	
	 * Do copy on write, when page is still in core.  This is really
	 * copy-on-access.
	 */
	if (vfd->pgm.pg_v) {
		pfnlock(vfd->pgm.pg_pfn);
		(void)hdl_cwfault(wrt, prp, pgindx);
		return(0);
	}

	/*
	 * Page needs to be read into memory or it is a 
	 * segmentation violation.
	 */
	return(SIGSEGV);
}

/*
 * hdl_kmap()--given a physical page number, map it somewhere in the kernel
 *  and return the place.  This is useful for kernel access to user data.
 */
caddr_t
hdl_kmap(pfn)
	int pfn;
{
	extern int hil_pbase;

	/* XXX paranoid for now */
	purge_dcache();

	/* XXX use Sols macro when its in a header */
	/* return transparent translation address */
	return((caddr_t)(pctopfn(pfn+hil_pbase) << PGSHIFT));
}

/*
 * hdl_kunmap()--delete translation created with hdl_kmap().
 */
/*ARGSUSED*/
hdl_kunmap(vaddr, pfn)
	caddr_t vaddr;
	int pfn;
{
	/* XXX paranoid for now */
	purge_dcache();
}

/*
 * hdl_remap()--reset translation which was created for kernel
 *
 * For 300: delete the temporary kernel translation.  Don't need
 *  to do anything about the user translation as it was never
 *  deleted.
 */
/*ARGSUSED*/
hdl_remap(ospace, ovaddr, pfn)
	space_t ospace;
	caddr_t ovaddr;
	int pfn;
{
	/* XXX paranoid for now */
	purge_dcache();
}

/*
 * HDL copy-on-write fault:
 *
 * This routine embodies the hardware-dependent techniques needed to
 * implement copy-on-write.
 *
 * 300 copy-on-write:
 *
 * For the 300, we:
 *	- Get a new page, create a kernel translation, and copy from the
 *	  user's page.  We then free resources, break disk associations,
 *	  and so forth.
 */

int
hdl_cwfault(wrt, prp, idx)
	int wrt;
	register preg_t *prp;
	int idx;
{
	pfd_t	*pfd;
	reg_t	*rp = prp->p_reg;
	unsigned space = prp->p_space;
	caddr_t	vaddr = (prp->p_vaddr + (ptob(idx - prp->p_off)));
	vfd_t origvfd;
	register vfd_t *vfd;
	vfd_t *vfd2;
	register dbd_t *dbd;
	dbd_t *dbd2;
	unsigned pn;
	register int cw;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	FINDENTRY(rp, idx, &vfd2, &dbd2);
	vfd = vfd2; dbd = dbd2;
	pn = vfd->pgm.pg_pfn;
	cw = vfd->pgm.pg_cw;
	VASSERT(vfd->pgm.pg_v);

	pfd = &pfdat[pn];
#ifdef PFDAT32
   	VASSERT(pfd_is_locked(pfd));
#else	
   	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	
	VASSERT(pfd->pf_use > 0);

	/*
	 * If this page is in an MMF region, we just add the translation
	 * and return.
	 */
	if (rp->r_fstore == rp->r_bstore) {
		hdl_addtrans(prp, space, vaddr, cw, (int)pn);
		pfdatunlock(pfd);
		return(1);
	}

	/* 
	 * If writing to a page, copy it if others are using it or it is
	 *  associated with a file.
	 */
        if (wrt &&
	    ((pfd->pf_use > 1) ||(cw && dbd->dbd_type == DBD_FSTORE))) {
		caddr_t oldvaddr, newvaddr;
		/*
                 * If this is a data page shared by parent and child and
                 * the child hasn't exec'd yet then add this page to the
                 * list of pages to copy up front on fork
                 */
		switch (prp->p_type) {
		case PT_DATA:
		case PT_STACK:
			if (((u.u_procp->p_flag2 & S2EXEC) == 0) &&
			    (dbd->dbd_type != DBD_FSTORE))
#ifdef PFDAT32
				pfd->pf_hdl.pf_bits |= PFHDL_COPYFRONT;
#else
				pfd->pf_hdl.pf_bits |= PF_COPYFRONT;
#endif
                }

		/*
		 * Reserve memory for the page copy.  If we need to sleep
		 * to get memory, the region will be unlocked and the
		 * state of this page could change (e.g., stolen by
		 * getpages()).  To allow for this, we compare the
		 * page state (vfd value) before and after we reserve
		 * memory.  If the state changes we backout of our
		 * copy and let the user retry the access.
		 *
		 * One gotcha in doing this is that we can be called by
		 * vfault() to possibly copy pages reclaimed via pageincache();
		 * in this case our vfd for the page has been marked valid,
		 * but we need to copy it before it is accessed.  We mark
		 * the vfd as copy on write to ensure that any access to it
		 * while we sleep waiting for memory forces a copy (i.e.,
		 * we will reenter here).  Note that in this case the page
		 * is not currently translated through the virtual address
		 * represented by this region, so any user access will fault
		 * and we will wind up here again.
		 */ 
		vfd->pgm.pg_cw = 1;
		
		/*
		 * We will try and reserve a single page to copy to.  If we
		 * can not get a page then we must unlock the region and the
		 * source page and then try again later. 
		 * To prevent thrashing, we try and obtain a single page after
		 * failure.  We will sleep until the page is reserved.
		 * We then release the page and return.
		 */
			
		if (!cmemreserve(1)) {
			vfd_t origvfd;

			origvfd = *vfd;
			pfdatunlock(pfd);
			memreserve(rp, 1);
			VASSERT(vm_valusema(&rp->r_lock) <= 0);
			FINDENTRY(rp, idx, &vfd2, &dbd2);
			vfd = vfd2;
			if (vfd->pgi.pg_vfd != origvfd.pgi.pg_vfd) {
				/*
				 * Vfd state changed while we slept getting
				 * memory with region unlocked.  Most
				 * probably the page was stolen by vhand()
				 * and marked invalid.  Backout and retry.
				 */
				memunreserve(1);
				return(0);
			}
			pfdatlock(pfd);
		}

		/* 
		 * At this point the page had better not be on the free list.
		 */
		VASSERT((pfd->pf_flags&P_QUEUE) == 0);
# ifdef LATER
		minfo.cw++;
# endif	LATER

		/* 
		 * detach from the source pfd and make sure it is not
		 * mapped into the current region.
		 */
		hdl_deletetrans(prp, space, vaddr, pn);

		/* 
		 * clear all but the lock bit 
		 */
		if (vfd->pgm.pg_lock) {
			vfd->pgi.pg_vfd = 0;	
			vfd->pgm.pg_lock = 1;	
		}
		else
			vfd->pgi.pg_vfd = 0;	
			
		rp->r_nvalid--;

		/* 
		 * get the destination page (already reserved above)
		 */
		VFDFILL_ONE(prp, rp, space, vaddr, vfd);

		/* 
		 * Get a translation for each of the old
		 * page and the new page.  Since the source pages translation
		 * could have stale entires we must purge the source pages
		 * translations from the cache.
		 */
		oldvaddr = ttlbva(pn);
		purge_dcache();
		newvaddr = ttlbva(vfd->pgm.pg_pfn);

		/* do the copy */
		pg_copy4096(oldvaddr, newvaddr);

		/*
		 * If we are on a machine with seperate I and D caches
		 * that are operate in copy back mode, we must push the
		 * D cache; so that the I cache will see the
		 * change.
		 */
		if (prp->p_prot&PROT_EXECUTE) {
			if (processor == M68040)
				purge_dcache_physical();
		}

		/*
		 * Make sure that destination page is mapped into the
		 * destination address space.
		 */
		hdl_addtrans(prp, space, vaddr, 0 /* cw */, vfd->pgm.pg_pfn);

		/* 
		 * unlock destination page 
		 */
		pfnunlock(vfd->pgm.pg_pfn); 

		/* 
		 * Now free the source page 
		 */
		freepfd(pfd);

		/*
		 * Tell the vnode we are no longer using this dbd.
		 */
		switch (dbd->dbd_type) {
			case DBD_FSTORE:
				VOP_DBDDEALLOC(rp->r_fstore, dbd);
				break;
			case DBD_BSTORE:
				VOP_DBDDEALLOC(rp->r_bstore, dbd);
				break;
		}
		dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff01;
	} else {
# ifdef LATER
		minfo.steal++;
# endif	LATER

		/* If writing to a c.w. page, free any disc association.
		 */
		if (wrt && cw) {
			if (PAGEINHASH(pfd))
				pageremove(pfd);
			/*
			 * Tell the vnode we are no longer using this dbd.
			 * the current pfn can remain locked because
			 * it will NOT be found by removefromcache().
			 */
			switch (dbd->dbd_type) {
				case DBD_FSTORE:
					VOP_DBDDEALLOC(rp->r_fstore, dbd);
					break;
				case DBD_BSTORE:
					VOP_DBDDEALLOC(rp->r_bstore, dbd);
					break;
			}
			dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff02;
			vfd->pgm.pg_cw = 0;
			cw = 0;
		}
		hdl_addtrans(prp, space, vaddr, cw, (int)pn);
		pfdatunlock(pfd);
	}
	return(1);
}
