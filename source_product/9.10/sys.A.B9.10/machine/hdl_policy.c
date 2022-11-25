/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_policy.c,v $
 * $Revision: 1.8.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:28:59 $
 */

/*
 * This file contains the hardware dependent policy routines.  Each routine 
 * in this file is called by hardware independent routine in the course
 * of creating, growing, and deleting a region.  These routines are
 * policy decisions about the location of a pregion and a pregion's
 * ability to expand.
 */
#include "../h/debug.h"
#include "../h/types.h"
#include "../h/pregion.h"
#include "../h/vas.h"

#include "../h/sema.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/errno.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/pfdat.h"
#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/atl.h"

int do_addtransc();
int do_copypagec();
int do_checkvfds();

/*
 * Hardware dependent virtual address attach routine.
 *
 * The HDL attach routine is responsible for several items:
 *	- verify that the attach address is acceptable.
 *	- if the pregion is of type PT_MMAP or PT_SHMEM,
 *	  then choose an attach address for the pregion
 *	  and fill in p_vaddr.
 *	- provide a value for p_space.  For machine which have a 32 bit per 
 *	  process address space, p_space will be something like the process
 *	  id.  For machines which have a full 64 bit address space, p_space
 *	  will be the actuall value for the space register.
 *
 *	- note: it is possible that vas is not a true process vas, but is
 *	  part of a shared memory segment or part of a vnode's list of 
 *	  pregions.
 *	- return 0 if the attach was successful, and 1 otherwise.
 *
 * 300 HDL attach routine.
 *
 * For the 300 we:
 *	- Make the space number the address of the vas structure.
 *	- Precalculate the protection value from the pregion.
 *	- Allocate a chunk of virtual address space.  If the virtual address
 *	  isn't specified, we will choose one in an appropriate area based
 *	  on the pregion's type.
 */
int
hdl_attach(vas, prp)
	register vas_t *vas;
	register preg_t *prp;
{
	VASSERT((prp->p_hdl.p_hdlflags & PHDL_ATTACH) == 0);

	/*
	 * Fill in space up front, needed for chkattach.
	 */
	prp->p_space = (space_t)vas;

	/*
	 * Allocate a segment for this type of memory, bomb if can't find
	 *  a fit.
	 */
	if ((prp->p_prot & PROT_KERNEL) && !(prp->p_type == PT_UAREA)) {
		/* 
		 * This is a psuedo address space, nothing to do.
		 */
		VASSERT(prp->p_vaddr == ptob(prp->p_off));
	} else {
		/* 
		 * Regular process address space
		 */
		if (vm_alloc(vas, prp) == -1)
			return(1);
	}

	/*
	 * We keep a list of all pregions associated with each
	 * region; so that we can find the pte's for the associated
	 * with a pfdat (for this region) and delete them.
	 */
#ifdef PFDAT32
	if (((prp->p_space != KERNELSPACE) && (prp->p_type != PT_IO)) || 
	    prp->p_type == PT_GRAFLOCKPG)
		if (indirect_ptes)		
			prp->p_hdl.p_hdlflags |= PHDL_IPTES;
		else
			prp->p_hdl.p_hdlflags |= PHDL_ATLS;

#else	
	if ((prp->p_space != KERNELSPACE) && indirect_ptes && 
	    					(prp->p_type != PT_IO)) {
		prp->p_hdl.p_hdlflags |= PHDL_IPTES;
	}
#endif	
#ifdef OSDEBUG
	prp->p_hdl.p_hdlflags |= PHDL_ATTACH;
#endif /* OSDEBUG */

	return(0);
}

/*
 * Hardware dependent process attach routine.
 *
 * The HDL procattach routine is responsible for:
 *	- setting up any registers that are needed for the given
 *	  process to access the new pregion.
 *
 * 300 HDL procattach routine.
 *
 * For 300:
 *	- Set proc field for segment table physical address
 */
hdl_procattach(up, vas, prp)
	struct user *up;
	register vas_t *vas;
	preg_t *prp;
{
	register int notkspace = prp->p_space != KERNELSPACE;

	VASSERT((prp->p_hdl.p_hdlflags & PHDL_PROCATTACH) == 0);
	VASSERT(prp->p_hdl.p_hdlflags & PHDL_ATTACH);

	/*
	 * Allocate the segment table on first attach to a process.
	 * We assume the kernel has a segment table.
	 */
        if (notkspace && (vas->va_hdl.va_seg == NULL)) {
                vas->va_hdl.va_seg = (struct ste *)alloc_segtab();
		up->u_procp->p_segptr = vas->va_hdl.va_seg;
	}

	if ((prp->p_type != PT_IO) && (prp != kernpreg) &&
#ifdef PFDAT32
	    (prp->p_hdl.p_hdlflags & PHDL_ATLS))
#else	
	    (prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0)
#endif	
		 atl_procattach(prp);

	/* Precalculate protections for pregion */
	if (prp->p_prot & PROT_WRITE)
		prp->p_hdl.p_hdlflags &= ~PHDL_RO;
	else
		prp->p_hdl.p_hdlflags |= PHDL_RO;

	if (prp->p_type == PT_UAREA)
		prp->p_hdl.p_hdlflags |= PHDL_RO;

	/*
         * For shared memory, set read only flag if it is
	 * not in kernel space.
         * The pfault handler will catch the first write,
         * turn on the modification bit and promote write
         * permission if appropriate.
	 */
	if ((prp->p_type == PT_SHMEM) && notkspace)
		prp->p_hdl.p_hdlflags |= PHDL_RO;

	/*
	 * For some types of pregions we prefill translations
	 * and copy pages.
	 */
	switch (prp->p_type) {
	case PT_IO:
		io_addtrans(prp);
		break;
	case PT_UAREA:
		/*
		 * Add HDL-level translations for all 
		 * existing upper-level translations.
		 */
		foreach_chunk(prp->p_reg, prp->p_off, prp->p_count, 
							     do_addtransc, prp);
		break;
	case PT_STACK:		
	case PT_DATA:
		/*
		 * Copy the page instead of copy-on-writing the page.
		 */
		foreach_chunk(prp->p_reg, prp->p_off, prp->p_count, 
							     do_copypagec, prp);
		break;
	case PT_MMAP:
	case PT_TEXT:
		if ((prp->p_prot&PROT_EXECUTE) && (up->u_syscall != SYS_VFORK)) {
			/*
			 * Add HDL-level translations for all 
			 * existing upper-level translations.
			 */
			foreach_chunk(prp->p_reg, prp->p_off, prp->p_count, 
							     do_addtransc, prp);
			break;
		}
		/* Fall through */
	default:
		if (notkspace && !(prp->p_hdl.p_hdlflags & PHDL_IPTES)) {
			foreach_chunk(prp->p_reg, prp->p_off, prp->p_count, 
						              do_addatlsc, prp);
		} else {
			foreach_chunk(prp->p_reg, prp->p_off, prp->p_count, 
							     do_checkvfds, prp);
		}
		purge_dcache();
		purge_tlb();
	}

#ifdef OSDEBUG
	prp->p_hdl.p_hdlflags |= PHDL_PROCATTACH;
#endif
}

/*
 * Hardware dependent virtual address detach routine.
 *
 * The HDL detach routine is responsible for:
 *	- detaching this pregion from this vas. This will
 *	  include items like deallocating global resources.
 *
 * 300 HDL detach routine
 *
 * For 300:
 *	- Decrement reference counts on the STE
 *	- Free page table resources when no more references exist
 */
hdl_detach(vas, prp)
	register vas_t *vas;
	register preg_t *prp;
{
	VASSERT(prp->p_hdl.p_hdlflags & PHDL_ATTACH);

	/* If this pregion was the attach point hint then reset it */
	if (vas->va_hdl.va_attach_hint == prp) {
		if (prp->p_prev != (preg_t *)vas) {
			if ((unsigned int)prp->p_prev->p_vaddr < INIT_ATTCHPT)
				vas->va_hdl.va_attach_hint = (preg_t *)NULL;
			else
				vas->va_hdl.va_attach_hint = prp->p_prev;
		} else {
			vas->va_hdl.va_attach_hint = (preg_t *)NULL;
		}
	}

#ifdef OSDEBUG
	prp->p_hdl.p_hdlflags &= ~PHDL_ATTACH;
#endif
}

/*
 * Hardware dependent process detach routine.
 *
 * The HDL procdeattach routine is responsible for:
 *	- Removing access to the given pregion from the given process.
 *
 * 300 HDL procdetach routine
 *
 * For 300:
 *	- Get rid of I/O translations
 *
 * XXX the user pointer is useless; do NOT use it!
 */
/*ARGSUSED*/
hdl_procdetach(up, vas, prp)
	struct user *up;
	vas_t *vas;
	register preg_t *prp;
{
	VASSERT(prp->p_hdl.p_hdlflags&PHDL_PROCATTACH);
	VASSERT(prp->p_hdl.p_hdlflags & PHDL_ATTACH);

	/* Remove the translations for this pregion */
	vm_dealloc(prp, prp->p_vaddr, prp->p_count);

	/* If this is the last pregion attached to this vas then free
	 * up the segment table and any remaining block and page tables.
	 */
	if (prp->p_next == prp->p_prev)
		vm_dealloc_tables(vas);

	/*
	 * Special case code to delete iotranslations
	 */
	if (prp->p_type == PT_IO) {
		io_deletetrans(prp);
#ifdef OSDEBUG
		prp->p_hdl.p_hdlflags &= ~PHDL_PROCATTACH;
#endif
		return;
	}

#ifdef PFDAT32
	if (prp->p_hdl.p_hdlflags & PHDL_ATLS) {
#else	
	if ((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0) {
#endif	
		/*
		 * Remove any existing atl entries, the translations
		 * are removed in hdl_detach.
		 */
		prp_atl_destroy(prp);
		VASSERT(prp->p_hdl.p_ntran == 0);
		VASSERT(prp->p_hdl.p_atl == (struct pregion_atl *)NULL);
	}

#ifdef OSDEBUG
	prp->p_hdl.p_hdlflags &= ~PHDL_PROCATTACH;
#endif
}

/*
 * Hardware dependent pregion duplicate routine.
 *
 * The HDL duppregion routine is responsible for:
 *	- Setting a duplicate view of dupprp in newprp.  
 *	  All of the hardware dependent fields must be
 *	  filled in.  The vas is the virtual address space
 *	  where newprp will be placed.
 *
 * 300 HDL duppregion routine.
 *
 * For 300 duppregion:
 *	- Set space to address of vas where new pregion will live
 *	- Duplicate read-only information
 *	- Set up initial HDL-level translations
 */
hdl_duppregion(vas, newprp, dupprp)
	vas_t *vas;
	preg_t *newprp, *dupprp;
{
	newprp->p_space = (space_t)vas;
	newprp->p_hdl.p_hdlflags = dupprp->p_hdl.p_hdlflags & 
		~(PHDL_ATTACH|PHDL_PROCATTACH|PHDL_IPTES);
	newprp->p_hdl.p_physpfn = dupprp->p_hdl.p_physpfn;
	newprp->p_hdl.p_ntran = 0;
}

/*
 * Hardware dependent grow region routine.
 *
 * The HDL grow region routine is responsible for:
 *	- ensuring that a given region of a process can grow.  Some
 *	  architectures could have hardware dependent reasons why a
 *	  particular region could not grow.
 *	- return 1 to indicate the growth is not allowed;
 *	  return 0 to indicate the growth is allowed.
 *	- It is specifically not responsible for detecting overlapping
 *	  pregions (the HIL does it for you).
 *
 * 300 HDL grow region routine.
 *
 * For 300 hdl_grow:
 *	- Call tablewalk-specific code to update MMU data structures for
 *	  keeping track of reference counts.
 */
hdl_grow(prp, change)
	register preg_t *prp;
	register int change;
{
	register caddr_t vaddr;
	extern int do_deltransc();


	if ((prp->p_prot&PROT_KERNEL) && !(prp->p_type == PT_UAREA))
		return(0);

#ifdef PFDAT32
	if (prp->p_hdl.p_hdlflags & PHDL_ATLS)
#else	
	if ((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0) 
#endif	
		atl_grow(prp,change);
		
	/* Region shrinks--free appropriate resources */
	if (change < 0) {

		/* Easier to think about it as a positive number of change */
		change = -change;

		/* Figure out vaddr of first page not referenced any more */
		vaddr = prp->p_vaddr + ptob(prp->p_count - change);

		/* free resources for these pages */
		vm_dealloc(prp, vaddr, change);
	}
	return(0);
}

/*ARGSUSED*/
do_copypagec(rp, idx, vd, count, prp)
	reg_t *rp;
        int idx;
	struct vfddbd *vd;
	int count;
        preg_t *prp;
{
	int start;
	struct pte *pt = NULL;
	caddr_t vaddr;

	VASSERT(prp->p_off == 0);
	VASSERT(prp->p_count <= rp->r_pgsz);

	vaddr = prp->p_vaddr+ptob(idx);

	for (; count--; vd++,idx++,vaddr += NBPG) {
		vfd_t *vfd = &(vd->c_vfd);
		dbd_t *dbd = &(vd->c_dbd);
		int pfn = vfd->pgm.pg_pfn;
		pfd_t *pfd = pfdat+pfn;

		pt = vastopte((vas_t *)prp->p_space, vaddr);

		if (vfd->pgm.pg_v) {
#ifdef PFDAT32
			if (pfd->pf_hdl.pf_bits & PFHDL_COPYFRONT) {
#else
			if (pfd->pf_hdl.pf_bits & PF_COPYFRONT) {
#endif
				VASSERT(vfd->pgm.pg_cw);
				pfdatlock(pfd);
				if (hdl_cwfault(1, prp, idx))
				       VASSERT(dbd->dbd_type==DBD_NONE);
			} else {
				if (pt && pt->pg_v) {
	/*
	 * This page has not yet been copied.  Thus the child must 
	 * be prevented from writing to it (we must be able to get 
	 * the fault and break the association).  So, we make the child's
	 * view R/O. This can happen if hdl_user_protect is called on the 
	 * page during the fork.
	 */
					if (vfd->pgm.pg_cw)
						pt->pg_prot = 1;
#ifdef PFDAT32
					if (pfd->pf_hdl.pf_bits & PFHDL_ATLS)
#else	
					if ((pfd->pf_hdl.pf_bits & PFHDL_IPTES) == 0)
#endif	
					{
						pfdatlock(pfd);
						atl_addtrans(prp, pfn, pt, vaddr);
						pfdatunlock(pfd);
					}
				}
			}
		} else {
			pt = vastoipte((vas_t *)prp->p_space, vaddr);
			if (pt)
				*(int *)pt = 0;
		}
	}

	return(0);
}

/*ARGSUSED*/
do_addtransc(rp, idx, vd, count, prp)
	reg_t *rp;
        int idx;
	struct vfddbd *vd;
	int count;
        preg_t *prp;
{
	if (count == 1) {
	    vfd_t *vfd = &(vd->c_vfd);
	    int pfn = vfd->pgm.pg_pfn;

	    if (vfd->pgm.pg_v) {
		    pfnlock(pfn);
		    hdl_addtrans(prp, prp->p_space,
				 prp->p_vaddr + ptob(idx - prp->p_off),
				 vfd->pgm.pg_cw, pfn);
		    pfnunlock(pfn);
	    } else {
		    struct pte *pt;

		    pt = vastoipte((vas_t *)prp->p_space, 
					prp->p_vaddr + ptob(idx - prp->p_off));
		    if (pt)
			*(int *)pt = 0;
	    }

	} else {
		hdl_addtransc(prp, prp->p_space,
			      prp->p_vaddr + ptob(idx - prp->p_off),
			      count,vd);
	}
	return(0);
}

do_checkvfds(rp, idx, vd, count, prp)
	reg_t *rp;
        int idx;
	struct vfddbd *vd;
	int count;
        preg_t *prp;
{
	int start;
	struct pte *pt = NULL;
	caddr_t vaddr;

	VASSERT(prp->p_count <= rp->r_pgsz);

	vaddr = prp->p_vaddr+ptob(idx - prp->p_off);

	for (; count--; vd++, vaddr += NBPG) {
		vfd_t *vfd = &(vd->c_vfd);

		if (!vfd->pgm.pg_v) {
			pt = vastoipte((vas_t *)prp->p_space, vaddr);
			if (pt)
				*(int *)pt = 0;
		}
	}

	return(0);
}
/* Used to delete all translations for a pregion when a process is swapped
** out.  Not really sure why this is required.  HP Revisit.
*/
extern int do_deltransc();
hdl_alt_swapout (prp)
        register preg_t *prp;
{
        if ((prp->p_hdl.p_hdlflags & PHDL_IPTES) == 0) {
                foreach_chunk(prp->p_reg, prp->p_off, prp->p_count,
                                            do_deltransc, prp);
        }
}
