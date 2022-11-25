/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_pregion.c,v $
 * $Revision: 1.11.83.4 $        $Author: rpc $
 * $State: Exp $        $Locker:  $
 * $Date: 93/10/18 13:45:51 $
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/vm.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/var.h"
#include "../h/pfdat.h"
#include "../h/malloc.h"
#include "../h/sema.h"
#include "../h/vnode.h"
#ifdef FSD_KI
#include "../h/ki_calls.h"
#endif /* FSD_KI */

/*
 * CRIT() and UNCRIT() exist on the s300 and s700.  However, the s800
 * does not have these functions.
 */
#if defined(__hp9000s800) && !defined(__hp9000s700)
#define CRIT()		spl6()
#define UNCRIT(x)	splx(x)
#endif

preg_t *agehand;		/* Hand position for ageing */
preg_t *stealhand;		/* Active pregion list and hand for stealing */
vm_lock_t plistlock;		/* Lock of active list of pregions */

extern int handlaps;

static preg_t bcpreg;
preg_t *bufcache_preg = &bcpreg;  /* represents the buffer cache for vhand */

/*
 * Perform pregion initialization here.
 */
void
preginit()
{
	agehand = bufcache_preg;
	stealhand = bufcache_preg;
	handlaps = 0;
	bufcache_preg->p_forw = bufcache_preg;
	bufcache_preg->p_back = bufcache_preg;
	bufcache_preg->p_agescan = 0;
	bufcache_preg->p_ageremain = 1;
	bufcache_preg->p_stealscan = 0;
	bufcache_preg->p_vas = NULL;
	bufcache_preg->p_reg = NULL;

	vm_initlock(plistlock, PLISTLOCK_ORDER, "pregion list lock");
}

/*
 * Allocate a new pregion.
 */
preg_t *
allocpreg()
{
	register preg_t *prp;

	/*
	 * Allocate a pregion.
	 */
	MALLOC(prp, preg_t *, sizeof(preg_t), M_PREG, M_WAITOK);

	/*
	 * Initialize some fields.
	 */
	prp->p_flags = PF_ALLOC;
	prp->p_type = 0;
	prp->p_prot = -1;
	prp->p_space = -1;
	prp->p_vaddr = 0;
	prp->p_prpnext = (preg_t *)NULL;
	prp->p_prpprev = (preg_t *)NULL;
	prp->p_lastfault = 0;
	prp->p_lastpagein = 0;
	prp->p_trend_diff = 0;
	prp->p_trend_strength = 0;

	/*
	 * Note: freevas depends on the pregion having this initialized
	 * to quickly delete the pregion from a vas.  It avoids the search
	 * by assuming that the pregion prev, next pointers are either
	 * linked into the vas or pointing back at the prp.
	 */
	prp->p_prev = prp;
	prp->p_next = prp;
	hdl_allocpreg(prp);

	/*
	 * Will be added to active list if need be by
	 * activate_preg().
	 */
	prp->p_forw = prp;
	prp->p_back = prp;

	return(prp);
}


/*
 * freepreg() - free the indicated pregion from the indicated process.
 * This routine will deallocate prp even if it does not find it in the
 * given process' address space.
 */
void
freepreg(vas, prp)
	register vas_t *vas;
	register preg_t *prp;
{

	VASSERT(prp);
	VASSERT(prp->p_flags&PF_ALLOC);

	/*
	 * Find the pregion if it is in our list.
	 */
	vaslock(vas);

	/*
	 * Remove the pregion from the vas structure and put 
	 * the hint back to the beginning.
	 */
	prp->p_prev->p_next = prp->p_next;
	prp->p_next->p_prev = prp->p_prev;
	VA_KILL_CACHE(vas);
	vasunlock(vas);

	/*
	 * Should not be linked into active list.
	 */
	VASSERT(prp->p_forw == prp);
#ifdef FSD_KI
	KI_pregion(prp, KI_FREEPREG);
#endif /* FSD_KI */

	/*
	 * Tell HDL
	 */
	hdl_freepreg(prp);

	/*
	 * Free up the pregion
	 */
	FREE(prp, M_PREG);
}

/*
 * Add pregion to the active chain.
 */
void
activate_preg(prp)
	register preg_t *prp;
{
	VASSERT((prp->p_flags & PF_ACTIVE) == 0);

	/*
	 * If this pregion is marked PF_NOPAGE then someone has already
	 * put the region in the active list via another pregion, or we
	 * want to keep vhand away.  In this case just don't insert
	 * him in the active chain.
	 */
	if (prp->p_flags & PF_NOPAGE) {
		update_active_nice(prp, 1 /* add */);
		return;
	}

	VASSERT(prp->p_type != PT_UAREA);
	VASSERT(prp->p_type != PT_NULLDREF);

	prp->p_agescan = 0;
	prp->p_ageremain = 0;
	prp->p_stealscan = 0;
	prp->p_trend_diff = get_pregionnice(prp); /* see update_active_nice() */

	plstlock();
	prp->p_forw = stealhand;
	prp->p_back = stealhand->p_back;
	prp->p_back->p_forw = prp;
	prp->p_forw->p_back = prp;
	prp->p_flags |= PF_ACTIVE;
	plstunlock();

	update_active_nice(prp, 3 /* init */);
}

/* 
 * Remove pregion from active chain.
 */
void
deactivate_preg(prp)
	register preg_t *prp;
{
	if ((prp->p_flags & PF_ACTIVE) == 0)
		return;
	plstlock();

	/* If both the agehand and stealhand point to the region being 
	 * deactivated, both hands can be moved forward.  If the agehand
	 * points to the region being deleted, and the next region is pointed
	 * to by the stealhand, and we are at the limit on laps, then we can't
	 * move the agehand forward (so we move it back).  We don't have to
	 * adjust handlaps when moving the agehand back one region even when
         * it lands on the stealhand.
	*/
	if (prp == agehand) {
	    if (handlaps < 15 || agehand->p_forw != stealhand)
		bump_agehand();
	    else
		agehand = prp->p_back;
	}
	if (prp == stealhand)
	    bump_stealhand();

	prp->p_back->p_forw = prp->p_forw;
	prp->p_forw->p_back = prp->p_back;
	prp->p_forw = prp;
	prp->p_back = prp;
	prp->p_flags &= ~PF_ACTIVE;

	plstunlock();
}

/*
 * add_pregion_to_region() -
 *    Each region maintains a list of all pregions that are using the
 *    region.  This list is used by the s300 hardware dependent layer,
 *    memory mapped files (mprotect()) and possibly other areas (vhand
 *    may find this data useful).
 *
 *    This routine adds a pregion to that list.
 *
 *    The list is kept sorted by 'p_off', for the benefit of
 *    hdl_attach().
 */
void
add_pregion_to_region(prp)
register preg_t *prp;
{
	register reg_t *rp = prp->p_reg;
	register int x;

	/*
	 * Insert this prp into the list off the region.  Because an
	 * interrupt routine may need to traverse the pregion list, we
	 * must protect this section.
	 */
	x = CRIT();
	if (rp->r_pregs == (preg_t *)NULL) {
		/* The list is empty */
		rp->r_pregs = prp;
		prp->p_prpprev = (preg_t *)0;
		prp->p_prpnext = (preg_t *)0;
	} else {
		size_t p_off = prp->p_off;
		preg_t *prp2 = rp->r_pregs;
		preg_t *last_prp;

		/*
		 * Search the list and stop at the first element
		 * with a p_off greater than or equal to our p_off.
		 * We then insert *before* this element.
		 */
		while (prp2->p_off < p_off || 
		       prp2->p_off == p_off && (prp2->p_flags & PF_ACTIVE)) {
			last_prp = prp2;
			if ((prp2 = prp2->p_prpnext) == (preg_t *)0)
				break;
		}
		if ((prp->p_prpnext = prp2) == (preg_t *)0) {
			/*
			 * We ran off the end of the list.  last_prp
			 * points to the last element, and we add our
			 * new one after it.
			 */
			last_prp->p_prpnext = prp;
			prp->p_prpprev = last_prp;
		}
		else {
			/*
			 * We insert before 'prp2'.  If the previous
			 * pointer on prp2 is null, that means it is the
			 * first element of the list, so we must update
			 * rp->r_pregs.  Otherwise, we make the element
			 * before prp2 point to the new element.
			 */
			if ((prp->p_prpprev = prp2->p_prpprev) == (preg_t *)0)
				rp->r_pregs = prp;
			else
				prp->p_prpprev->p_prpnext = prp;
			prp2->p_prpprev = prp;
		}
	}
	UNCRIT(x);

	/*
	 * If we are attaching a writable pregion to a shared memory
	 * mapped file, we must increment the reference count on the
	 * file table entry for this vnode so that the DUX i_writesites
	 * maps are kept in sync.
	 *
	 * Of course, we only do this for MAP_FILE mmf pregions, not
	 * MAP_ANONYMOUS mmf pregions.
	 */
	if (prp->p_type == PT_MMAP && (prp->p_flags & PF_WRITABLE) &&
	    rp->r_type == RT_SHARED && rp->r_fstore != (struct vnode *)0) {
	    vas_t *vas = rp->r_fstore->v_vas;

	    VASSERT(vas != (vas_t *)0);
	    VASSERT(vas->va_fp != (struct file *)0);
	    SPINLOCK(file_table_lock);
	    vas->va_wcount++;
	    SPINUNLOCK(file_table_lock);
	}
}

/*
 * duppregion() - duplicate in another pregion the same
 * view of a region provided by the passed in
 * pregion.  The new pregion will be attached to
 * vas on return.  This routine is similiar to
 * a dupreg followed by an attachreg.
 */
preg_t *
duppregion(vas, prp)
	register vas_t *vas;
	register preg_t *prp;
{
	register preg_t *prp2;

	VASSERT(prp->p_reg);
	VASSERT(vm_valusema(&prp->p_reg->r_lock) <= 0);

	/*
	 * Allocate a pregion.
	 */
	prp2 =  allocpreg();
	if (prp2 == NULL) {
		u.u_error = ENOMEM;
		return(NULL);
	}
	
	/*
	 * Copy the all of the fields of the  pregion.
	 * We must do this one at a time because not all of the fields
	 * will be copied.
	 */
	prp2->p_flags = (prp->p_flags & ~(PF_ACTIVE|PF_MLOCK));
	prp2->p_type = prp->p_type;
	prp2->p_reg = prp->p_reg;
	prp2->p_space = prp->p_space;
	prp2->p_vaddr = prp->p_vaddr;
	prp2->p_off = prp->p_off;
	prp2->p_count = prp->p_count;
	prp2->p_prot = prp->p_prot;

	hdl_duppregion(vas, prp2, prp);

	/*
	 * We must now make the next and previous pointer point back
	 * at this same pregion.
	 */
	prp2->p_next = prp2;
	prp2->p_prev = prp2;
	
	/*
	 * Call a hardware dependent routine for attaching this pregion.
	 * The routine will determine the attach address and protections
	 * from information in the pregion.
	 */
	if (hdl_attach(vas, prp2)) {
		/* 
		 * Pregion could not be attached. Clean up and back out.
		 */
		freepreg(vas, prp2);
		u.u_error = EINVAL;
		return(NULL);
	}

	/*
	 * Link this pregion into the list of pregions associated
	 * with its region.
	 */
	add_pregion_to_region(prp2);

	/*
	 * Now attach the pregion to the vas.
	 */
	insertpreg(vas, prp2);
	prp2->p_vas = vas;

	/*
	 * Now add the pregion to the list of active pregions.
	 */
	activate_preg(prp2);

	/*
	 * Bump misc. region attach counts.
	 */
	if (prp->p_reg->r_dbd)
		vfdswapi(prp->p_reg);
	prp2->p_reg->r_incore++;
	prp2->p_reg->r_refcnt++;

#ifdef FSD_KI
	KI_pregion(prp2, KI_DUPPREGION);
#endif /* FSD_KI */

	return(prp2);
}

/*
 * Check that attach of a pregion is legal
 * 	(assuming the pregion size was changed by "change" pages).
 *
 * Attach is illegal if it overlaps an existing pregion or extends
 * beyond the end of process virtual memory.
 */
int
chkattach(vas, newprp, change)
	vas_t *vas;
	preg_t *newprp;
	int change;
{
	register preg_t *prp;
	register caddr_t bvaddr;	/* beginning virtual addr */
	register caddr_t evaddr;	/* ending virtual addr */


	bvaddr = newprp->p_vaddr;
	evaddr = bvaddr + ptob(newprp->p_count + change);

	if (evaddr < bvaddr) {
		u.u_error = EINVAL;
		return(-1); /* new pregion extends past end of addr space */
	}

	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		if (prp == newprp)
			continue;
		if (evaddr <= prp->p_vaddr ||
		    bvaddr >= (prp->p_vaddr + ptob(prp->p_count)))
			continue;
		u.u_error = EINVAL;
		return(-1);	/* new pregion overlaps another pregion */
	}
	return(0);	/* attach is ok */
}

/*
 * do_findstartidxc()
 *
 * Because the 300 stack grow backwards from double stuff perspecitive,
 * we must find the first highest page in the stack that has been 
 * allocated.
 */
/*ARGSUSED*/
int
do_findstartidxc(rp, idx, vd, count, startidx)
	reg_t *rp;
	int idx;
	struct vfddbd *vd;
	int count;
	int *startidx;
{
	int tidx = idx;

	for (; count--; vd++, tidx++) {
		vfd_t *vfd = &(vd->c_vfd);
		dbd_t *dbd = &(vd->c_dbd);

               /*
                * If the page is valid, or has been paged out,
                * we've found the starting point for the stack
                */
               if (vfd->pgm.pg_v || (dbd->dbd_type == DBD_BSTORE)) {
                       *startidx = tidx;
                       return(1);
               }
       }

       /*
        * Didn't find a valid or paged-out stack page in this chunk.
        * Return the index of the last page in this chunk, but indicate to
        * foreach_chunk to keep trying.
        */
       *startidx = tidx;
       return(0);
}

/*
 * get_startidx() -- find starting index of the area to be locked
 */
/*ARGSUSED*/
int
get_startidx(prp)
register preg_t *prp;
{
	int start_idx;

#ifdef __hp9000s800
	start_idx = prp->p_off;
#endif /* __hp9000s800 */

#ifdef __hp9000s300
	if (prp->p_type != PT_STACK) {
		start_idx = prp->p_off;
	} else {
		start_idx = prp->p_off + prp->p_count - 1;
		foreach_chunk(prp->p_reg, prp->p_off, prp->p_count,
			do_findstartidxc, (caddr_t) &start_idx);
	}
#endif /* __hp9000s300 */

	return(start_idx);
}

/* mlockpreg(prp) - lock the region associated with this pregion in memory
 *	Bring all pages from swap device and lock this region in
 *	memory.  A non-zero r_mlockcnt indicates that the region cannot
 *	be swapped or paged out.  The RF_MLOCKING  flag indicates that
 *	the region is being locked in memory.
 *
 *	Return failure if insufficient lockable memory left in system.
 */
int
mlockpreg(prp)
	register preg_t *prp;
{
	register reg_t *rp = prp->p_reg;
	register int start_idx; /* starting idx of area to be locked */
	register int pgcnt;	/* number of pages to be locked */

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(prp->p_type != PT_UAREA);
	VASSERT(prp->p_type != PT_NULLDREF);

	/* Set PF_MLOCK flag.  Note that this should
	 * be done before entering the loop which calls virtual_fault.
	 * Otherwise, the pages which have been faulted in may be stolen 
	 * again when the process goes to sleep in virtaul_fault.
	 */
	prp->p_flags |= PF_MLOCK;

	if (rp->r_mlockcnt == 0) {
		VASSERT(!(rp->r_flags & RF_MLOCKING));
		
		/* find starting region index of the area to be locked */
		start_idx = get_startidx(prp);

		/* calculate how many pages are to be locked */
		pgcnt = prp->p_count + prp->p_off - start_idx;

		/* allocate system lockable memory; fail if not enough*/
		if (!clockmemreserve((unsigned int)pgcnt)) {
			goto bad2;
		}

		/* We are the first one to lock this region in memory.
		 * Set a flag and acquire r_mlock resource.
		 */
		rp->r_mlockcnt = 1;
		rp->r_flags |= RF_MLOCKING;
		if (!vm_cpsema(&rp->r_mlock))
			panic("mlockpreg: r_mlock already locked");

		/* fault in invalid pages; break copy-on-write association */

		if (rp->r_dbd) {
			vfdswapi(rp);   /* swapin vfds if necessary */
		}

		if (bring_in_pages(prp,
				   prp->p_vaddr+ptob(start_idx - prp->p_off),
				   pgcnt,1,0)) {
			goto bad1;
		}

		/* wake up any one waiting for this region to be locked
	 	 * in memory.
	 	 */
		vm_vsema(&rp->r_mlock, 0);
		rp->r_flags &= ~RF_MLOCKING;

		VASSERT(rp->r_nvalid == pgcnt);

	}
	else {	/*  region is already locked in memory */
		rp->r_mlockcnt++;
		if (rp->r_flags & RF_MLOCKING) {
			/* wait for region to finish paging in */
			regrele(rp);
			vm_psema(&rp->r_mlock, PZERO);
			vm_vsema(&rp->r_mlock, 0);
			reglock(rp);
		}
	}

	VASSERT(!(rp->r_flags & RF_MLOCKING));
	VASSERT(rp->r_mlockcnt != 0);
	return(1);	/* The region is locked in memory as requested */


bad1:
	/* wake up any one waiting for this region to be locked
	 * in memory.
	 */
	vm_vsema(&rp->r_mlock, 0);
	rp->r_flags &= ~RF_MLOCKING;
	rp->r_mlockcnt--;

	/* unreserve lockable memory */
	lockmemunreserve((unsigned int)pgcnt);

bad2:
	/* clear PF_MLOCK flags */
	prp->p_flags &= ~PF_MLOCK;

	return(0);
}


/* munlockpreg(prp) - unlock region asscoiated with this pregion from memory
 *	Unlock the locked region from memory.  The r_mlockcnt field
 *	contains the number of processes that have locked this region
 *	in memory.  When this count reaches 0, unlock this region and
 *	free up the lockable memory.
 */
void
munlockpreg(prp)
	register preg_t *prp;
{
	register reg_t *rp = prp->p_reg;
	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(rp->r_mlockcnt > 0);
	VASSERT(prp->p_type != PT_UAREA);
	VASSERT(prp->p_type != PT_NULLDREF);
#ifdef __hp9000s800
        VASSERT(rp->r_nvalid == prp->p_count);
#endif /* __hp9000s800 */


	if (--rp->r_mlockcnt == 0) {
		lockmemunreserve(rp->r_nvalid);
	}
	prp->p_flags &= ~PF_MLOCK;
}


/*
 * Attach a region to a process' address space.  Allocate a pregion, let
 * the HDL (along with the "vaddr", "pregflag", and "count" parameters)
 * choose an attach address.  Verify that there aren't any collisions, and
 * return a pregion to the caller.
 */
preg_t *
attachreg(vas, rp, prot, pregtype, pregflag, vaddr, off, count)
	register vas_t *vas;
	register reg_t *rp;
	short prot, pregtype;
	int pregflag;
	caddr_t vaddr;
	size_t off, count;
{
	register preg_t *prp;
	vm_sema_state;		/* semaphore save state */

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

 	if ((int)vaddr & PGOFSET) {	/* ensure page-aligned attach */
		u.u_error = EINVAL;
		return(0);
	}
		
	/*
	 * If this is a PT_TEXT region, set PF_VTEXT so that we know to
	 * perform a dectext() when we free this pregion.
	 */
	if (pregtype == PT_TEXT)
	    pregflag |= PF_VTEXT;

	vmemp_lockx();		/* lock down VM empire */
	/*
	 * Allocate a pregion.
	 */
	prp =  allocpreg();

	/*	
	 * init pregion
	 */
	prp->p_reg = rp;
	prp->p_prot = prot;
	prp->p_type = pregtype;
	prp->p_flags |= pregflag;
	prp->p_vaddr = vaddr;
   	prp->p_off = off;
	prp->p_count = count;
	prp->p_vas = vas;

	/*
	 * Call a hardware dependent routine for attaching this pregion.
	 * The routine will determine the attach address and protections
	 * from information in the pregion.
	 */
	if (hdl_attach(vas, prp)) {
		/* 
		 * Pregion could not be attached. Clean up and back out.
		 */
		freepreg(vas, prp);
		u.u_error = EINVAL;
		vmemp_returnx(NULL);
	}

	/*
	 * Call chkattach to see if the virtual address of the new
	 * pregion overlaps with any of the current pregions.
	 */
	if (chkattach(vas, prp, 0)) {
		hdl_detach(vas, prp);
		freepreg(vas, prp);
		vmemp_returnx(NULL);
	}

	/*
	 * Link this pregion into the list of pregions associated
	 * with its region.
	 */
	add_pregion_to_region(prp);

	/*
	 * Now attach the pregion to our vas.
	 */
	insertpreg(vas, prp);

	/*
	 * Now add the pregion to the list of active pregions.
	 */
	activate_preg(prp);

	/*
	 * Bump misc. region attach counts.
	 */
	if (rp->r_dbd)
		vfdswapi(rp);
	++rp->r_incore;
	++rp->r_refcnt;

#ifdef FSD_KI
	KI_pregion(prp, KI_ATTACHREG);
#endif /* FSD_KI */

	vmemp_unlockx();	/* free up VM empire */
	return(prp);
}

#ifdef NEVER_CALLED
/*
 * Local function invoked on valid vfds to add their translations to
 *	the HDL
 */
/*ARGSUSED*/
int
do_addtrans(idx, vfd, dbd, prp)
	int idx;
	register vfd_t *vfd;
	dbd_t *dbd;
	register preg_t *prp;
{
	pfnlock(vfd->pgm.pg_pfnum);
	hdl_addtrans(prp, prp->p_space, prp->p_vaddr+ptob(idx-prp->p_off),
			vfd->pgm.pg_cw, (int) vfd->pgm.pg_pfnum);
	pfnunlock(vfd->pgm.pg_pfnum);
	return(0);
}
#endif /* NEVER_CALLED */


/*
 * Local function invoked to delete translations of valid vfds within a chunk
 */
/*ARGSUSED*/
int
do_deltransc(rp, idx, vd, count, prp)
	reg_t *rp;
	int idx;
	struct vfddbd *vd;
	register int count;
	register preg_t *prp;
{
	for (; count--; idx++, vd++) {
		register vfd_t *vfd = &(vd->c_vfd);

		if (vfd->pgm.pg_v) {
			register int pfn = vfd->pgm.pg_pfnum;
			pfnlock(pfn);
			hdl_deletetrans(prp, prp->p_space,
					prp->p_vaddr+ptob(idx-prp->p_off),
					pfn);
			pfnunlock(pfn);
		}
	}
	return(0);
}

/*
 * remove_pregion_from_region() -
 *    Each region maintains a list of all pregions that are using the
 *    region.  This list is used by the s300 hardware dependent layer,
 *    memory mapped files (mprotect()) and possibly other areas (vhand
 *    may find this data useful).
 *
 *    This routine removes a pregion from that list.
 */
void
remove_pregion_from_region(prp)
register preg_t *prp;
{
	register reg_t *rp = prp->p_reg;
	int x;

	/*
	 * Remove this prp from the list off the region.  Because an
	 * interrupt routine may need to traverse the pregion list, we
	 * must protect this section.
	 */
	x = CRIT();
	if (rp->r_pregs == prp) {
		/* this pregion is first on the list */
		rp->r_pregs = prp->p_prpnext;
		if (rp->r_pregs != (preg_t *)0)
			rp->r_pregs->p_prpprev = (preg_t *)0;
	} else {
		prp->p_prpprev->p_prpnext = prp->p_prpnext;
		if (prp->p_prpnext != (preg_t *)0)
			prp->p_prpnext->p_prpprev = prp->p_prpprev;
	}
	UNCRIT(x);

	/*
	 * If we are detaching a writable pregion from a shared memory
	 * mapped file, we must decrement the reference count on the
	 * file table entry for this vnode so that the DUX i_writesites
	 * maps are kept in sync.
	 *
	 * Of course, we only do this for MAP_FILE mmf pregions, not
	 * MAP_ANONYMOUS mmf pregions.
	 */
	if (prp->p_type == PT_MMAP && (prp->p_flags & PF_WRITABLE) &&
	    rp->r_type == RT_SHARED && rp->r_fstore != (struct vnode *)0) {
	    vas_t *vas = rp->r_fstore->v_vas;

	    /*
	     * unmapvnode() may have taken care of things for us,
	     * in which case the psuedo-vas for this region is no
	     * longer with us.
	     */
	    if (vas != (vas_t *)0) {

		SPINLOCK(file_table_lock);
		if (--vas->va_wcount == 0) {
		    struct file *fp = vas->va_fp;

		    VASSERT(fp != (struct file *)0);
		    vas->va_fp = (struct file *)0;
		    SPINUNLOCK(file_table_lock);
		    closef(fp);
		}
		else {
		    SPINUNLOCK(file_table_lock);
		}
	    }
	}
#ifdef OSDEBUG
	prp->p_prpnext = (preg_t *)0;
	prp->p_prpprev = (preg_t *)0;
#endif /* OSDEBUG */
}

/*
 * Detach a region from the current process' address space
 */
void
detachreg(vas, prp)
	register vas_t *vas;
	register preg_t *prp;
{
	register reg_t *rp = prp->p_reg;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(rp->r_mlockcnt <= rp->r_refcnt);

	/* If pregion is memory locked, inform region
	 * that we no longer need it locked.
	 */
	if (PREGMLOCKED(prp))
		munlockpreg(prp);

	/*
	 * Deactive pregion from list of active pregions
	 */
	deactivate_preg(prp);
	
	/*
	 * Call hardware dependant detach routine.
	 */
	hdl_detach(vas, prp);

	/*
	 * Remove this pregion from the list of pregions associated
	 * with its region.
	 */
	remove_pregion_from_region(prp);

	update_active_nice(prp, 0 /* remove */);

	/*
	 * Decrement use count and free region if zero
	 * and RF_NOFREE is not set, otherwise unlock
	 * the region.
	 */
	--rp->r_incore;
	if (--rp->r_refcnt == 0 && !(rp->r_flags & RF_NOFREE))
		freereg(rp);
	else {
		regrele(rp);
	}
	freepreg(vas, prp);
}

/*
 * Change the size of a pregion
 *  change == 0  -> no-op
 *  change  < 0  -> shrink
 *  change  > 0  -> expand
 *
 * For expansion, you get (fill) real pages (change-fill) demand zero pages
 * unless the region is memory locked in which case you get (change) 
 * real pages.
 *
 * Returns 0 on no-op, -1 on failure, and 1 on success.
 */
int
growpreg(prp, change, fill, type, flags)
	preg_t *prp;
	int change;
	int fill, type, flags;
{
	register reg_t *rp = prp->p_reg;
	vfd_t *vfd;
	dbd_t *dbd;
	register int k;
	int reg_change, reg_changed = 0;
	int base_new, base_prp;

	VASSERT(rp != NULL);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(change >= 0 || (-change <= (int)rp->r_pgsz));
	VASSERT(rp->r_refcnt == 1);

	if (change == 0)
		return(0);

	/*
	 * Bounce it if it wouldn't fit in its vas any more
	 */
	if ((change > 0) && (chkattach(prp->p_vas, prp, change) < 0))
		return(-1);

	/* Record where we started growing from */
	base_new = prp->p_count + prp->p_off;
	base_prp = prp->p_count;

	/*
	 * If we're auto-adjusting the region, see if it needs to grow
	 * or shrink.
	 */
	if (flags & ADJ_REG) {
		reg_change = (prp->p_off + prp->p_count + change) - rp->r_pgsz;

		/*
		 * The only case where we inhibit action is where we are
		 * growing and we end up smaller than the region.  This is
		 * because this case usually represents someone else having
		 * a larger view.
		 */
		if (!((change > 0) && (reg_change < 0)))
			reg_changed = 1;
	}

	if (change < 0) {
		/*
		 * The pregion is being shrunk; tell the HDL and recalculate
		 * the size of the pregion.
		 */
		if (hdl_grow(prp, change))
			panic("growpreg: hdl_grow shrink failed");

		if (reg_changed) {
			if (growreg(rp, reg_change, type) < 0)
				panic("growpreg: growreg shrink failed");
		}
		prp->p_count += change;
	} else {
		if (reg_changed) {
			if (growreg(rp, reg_change, type) < 0) {
                                /*
                                 * u.u_error set to ENOMEM by growreg.
                                 */
				return(-1);
			}
		}

		/*
		 * See if HDL will allow us to grow.
		 */
		if (hdl_grow(prp, change)) {
			if (reg_changed) {
				if (growreg(rp, -reg_change, type) < 0)
				        panic("growpreg: ungrow failed");
			}
			u.u_error = ENOMEM;
			return(-1);
		}
		prp->p_count += change;
		VASSERT(change >= fill);

		/* 
		 * if the region is memory locked make sure that all new
		 * pages are actually paged in.
		 */
		if (prp->p_reg->r_mlockcnt != 0) { 
			fill = change;
		}

		if (fill > 0) {
			if (vfdmemall(prp, base_new, (unsigned int)fill)) {
				panic("growreg - vfdmemall failed");
			}
			if (type == DBD_DZERO) {
				space_t	nspace;
				caddr_t	nvaddr;

				hdl_user_protect(prp, prp->p_space, 
					prp->p_vaddr + ptob(base_prp), 
					fill, &nspace, &nvaddr, 
					0 /* steal */);
				hdl_zero_page(nspace, nvaddr, (int)ptob(fill));
				hdl_user_unprotect(nspace, nvaddr, fill, 0);
			} 
			
			/*
			 * Growing a pregion with auto fill from a front
			 * store is currently not supported.  MMFs will 
			 * need more work to accomplish this.
			 */
			for (k = 0; k < fill; k++) {
				FINDENTRY(rp, base_new + k, &vfd, &dbd);
				VASSERT((dbd->dbd_type == DBD_DFILL) ||
				        (dbd->dbd_type == DBD_DZERO));

				if (dbd->dbd_type == DBD_DFILL) {
					hdl_addtrans(prp, prp->p_space,
					    prp->p_vaddr + ptob(base_prp+k),
					    vfd->pgm.pg_cw, 
					    (int) vfd->pgm.pg_pfn);
				}
				dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff0e;
				pfnunlock(vfd->pgm.pg_pfn);
			}
		}
	}
#ifdef FSD_KI
	KI_pregion(prp, KI_GROWPREG);
#endif /* FSD_KI */

	return(1);
}

#ifdef	__hp9000s300
/*
 * Fill in the memory under a slot in a pregion
 */
fillpreg(prp, prp_indx)
	register preg_t *prp;
	int prp_indx;
{
	vfd_t *vfd;
	dbd_t *dbd;
	int regidx = prp->p_off + prp_indx;
	reg_t *rp = prp->p_reg;

	if (vfdmemall(prp, regidx, 1)) {
		panic("fillpreg - vfdmemall failed");
	}
	FINDENTRY(rp, regidx, &vfd, &dbd);
	VASSERT((dbd->dbd_type == DBD_DZERO) || (dbd->dbd_type == DBD_DFILL));
	if (dbd->dbd_type == DBD_DZERO) {
		space_t	nspace;
		caddr_t	nvaddr;

		hdl_user_protect(prp, prp->p_space, 
			prp->p_vaddr + ptob(prp_indx), 
			1, &nspace, &nvaddr, 
			0 /* steal */);
		hdl_zero_page(nspace, nvaddr, ptob(1));
		hdl_user_unprotect(nspace, nvaddr, 1, 0);
	}
	pfnunlock(vfd->pgm.pg_pfn);
	dbd->dbd_type = DBD_NONE; dbd->dbd_data = 0xfffff0f;
}
#endif	/* __hp9000s300 */

/*
 * This function solves a design conflict in 9.0.  vhand() thinks that
 * regions can be fully represented by multiple non-overlapping
 * pregions, while memory mapped files do not.  This leads to a region
 * having only one full pregion on the active list (often one not
 * associated with a process), which leads to loss of information
 * about the priorities of processes using subparts of regions.
 * Because vhand uses priorities to bias paging for interactive
 * response, it will need the active pregion for a region to know the
 * lowest valued priority of all processes using the region.
 *
 * This function will keep the lowest valued priority for a region in
 * an active list pregion field that is intended for something else,
 * but not yet used, due to schedule constraints.  In order for this
 * to work, this function must be called for each addition or removal
 * of a pregion to a region, and for each priority change of a
 * process for the pregion.
 *
 * Calling Constraints:
 *  - must be called with region lock
 *  - for a removal, the pregion must already be removed from the region
 *	pregion list
 *  - for addition, the pregion must already be added to the region pregion 
 * 	list
 */
update_active_nice(prp, type)
    preg_t *prp;
    int type;	/* 0 => remove;  1 => add;  2 => change; 3 => init */
{
    int prpnice, curnice, minnice;
    preg_t *actprp;

    if (prp->p_reg == NULL)
	return;

    /* 
     * Find the pregion in the active list.  It should normally be found
     * right away, because if it's shared it's a pseudo region with
     * offest zero, and the insert into the region pregion list is done
     * sorted on offset.
     */
    for (actprp = prp->p_reg->r_pregs; actprp; actprp = actprp->p_prpnext) {
	if (actprp->p_flags & PF_ACTIVE)
	    break;
    }
    if (actprp == NULL)
	return;

    /* use logic to see if there is a fast way out */
    curnice = type == 3 /* init */ ? 0x7fffffff : actprp->p_trend_diff;
    prpnice = get_pregionnice(prp);
    if (prp == actprp && type != 0 /* remove */ &&
	    prp == prp->p_reg->r_pregs && prp->p_prpnext == NULL) {
	/* typical case: only one pregion */
	actprp->p_trend_diff = prpnice;
	return;
    }
    if (type == 0 /* remove */) {
	if (prpnice > curnice)
	    return;
    } else if (type == 1 /* add */) {
	if (prpnice < curnice)
	    actprp->p_trend_diff = prpnice;
	return;
    } else if (type == 2 /* change */ && prpnice <= curnice) {
	actprp->p_trend_diff = prpnice;
	return;
    }

    /*
     * Search the pregions off the region to find the minimum nice value,
     * kicking out as early as possible.
     *
     * Handles the following cases:
     *  - type is init
     *  - type is remove and prpnice == curnice
     *  - type is change and prpnice > curnice
     */
    minnice = 0x7fffffff;
    for (prp = prp->p_reg->r_pregs; prp; prp = prp->p_prpnext) {
	prpnice = get_pregionnice(prp);
	if (minnice > prpnice) {
	    minnice = prpnice;
	    if (minnice < 0 || minnice == curnice)
		break;
	}
    }
    actprp->p_trend_diff = minnice;
}

/*
 * Change process priority as seen by vhand by calling update_active_nice() 
 * for each pregion in the process.  See the comment there.
 */
proc_update_active_nice(p)
    struct proc *p;
{
    vas_t *vas = p->p_vas;
    preg_t *prp;

    vaslock(vas);

    for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
	reglock(prp->p_reg);
	update_active_nice(prp, 2 /* change */);
	regrele(prp->p_reg);
    }

    vasunlock(vas);
}
