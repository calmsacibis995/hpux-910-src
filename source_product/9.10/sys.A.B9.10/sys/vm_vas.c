/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_vas.c,v $
 * $Revision: 1.13.83.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/18 13:46:45 $
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/vas.h"
#include "../h/var.h"
#include "../h/malloc.h"
#include "../dux/sitemap.h"

/*
 * Initialize the vas module (only called once).
 */
void
vasinit()
{
	/* Nothing to do */
}

vas_t *
allocvas()
{
	register vas_t *vas;
	vm_sema_state;		/* semaphore save state */
	extern void hdl_allocvas();

	/*
	 * Allocate a vas.
	 */
	MALLOC(vas, vas_t *, sizeof(vas_t), M_VAS, M_WAITOK);

	vmemp_lockx();		/* lock down VM empire */
	/*
	 * Initialize the vas fields (created locked)
	 */
	vm_initsema(&vas->va_lock, 0, VAS_VA_LOCK_ORDER, "vas sema");
	vas->va_refcnt = 1;
	VA_KILL_CACHE(vas);
	vas->va_next = vas->va_prev = (preg_t *)vas;
	vas->va_proc = (proc_t *)NULL;
	vas->va_flags = 0;	/* no flags set initially */
	vas->va_fp = (struct file *)0;
	vas->va_wcount = 0;

	/*
	 * Initialize approximations of resident set size
	 */
	vas->va_rss = 0;
	vas->va_prss = 0;
	vas->va_swprss = 0;

	/* Tell HDL */
	hdl_allocvas(vas);

	vmemp_unlockx();	/* free up VM empire */
	return(vas);
}

void
freevas(vas)
	register vas_t *vas;
{
	extern void hdl_freevas();
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	VASSERT(vm_valusema(&vas->va_lock) <= 0);

	if (vas->va_refcnt != 0) {
		vasunlock(vas);
		vmemp_returnx();
	}

	if (vas->va_next != (preg_t *)vas)
		panic("freevas: vas cnt 0, but still pointing to pregions");

	/* Tell HDL */
	hdl_freevas(vas);

	vasunlock(vas);
	vm_termsema(&vas->va_lock);
	vmemp_unlockx();	/* free up VM empire */
	/*
	 * Free it
	 */
	FREE((caddr_t)vas, M_VAS);
}

/*
 * This routine will copy one address space to another.  It will also
 * copy partial pieces based on the type of copy (fork) being done.
 */
vas_t *
dupvas(forktype, svas)
	int forktype;
	vas_t *svas;
{
	vas_t *dvas;
	preg_t *prp;
	preg_t *nulldrefprp = NULL;

	/*
	 * Allocate a new virtual address space structure for the
	 * child.  Unlock the vas; so that pregions can be added to it.
	 * Do this first, because low level code needs a vas once
	 * the p_flags are initialized.
	 */
	switch (forktype) {
	      case FORK_VFORK:
		vaslock(svas);
		svas->va_refcnt++;
		vasunlock(svas);
		return(svas);
		break;
	      case FORK_DAEMON:
		dvas = allocvas();
		vasunlock(dvas);
		return(dvas);
		break;
	      case FORK_PROCESS:
		dvas = allocvas();
		vasunlock(dvas);
		break;
	      default:
		panic("dupvas: unsupported fork type");
	}

	/*
	 * Duplicate the vas flags.
	 */
	dvas->va_flags = svas->va_flags;

	/*
	 * Duplicate all the pregions of the process.
	 * The backout code for this routine uses prp at the end; so
	 * if you change this loop be careful.
	 */
	for (prp = svas->va_next; prp != (preg_t *)svas; prp = prp->p_next) {
		/*
		 * Perform type specific actions.
		 */
		switch (prp->p_type) {
		      case PT_NULLDREF:
			/* Lets wait and do the nulldreference pregion last.
			 * See comment below as to why.
			*/
			nulldrefprp = prp;
			continue;
			break;
		      case PT_UAREA:
			/*
			 * Skip Uarea.
			 */
			continue;
			break;
		      case PT_TEXT: /* Fall through */
		      case PT_MMAP:
			/*
			 * Bump reference count on text vnode.
			 */
			{
				struct vnode *vp;
				vp = prp->p_reg->r_fstore;
				if (vp) {
					if (prp->p_flags & PF_VTEXT)
						inctext(vp, USERINITIATED);
					VASSERT(vp->v_vas != (vas_t *)0);
					if (mapvnode(vp, 0, 0)) {
						if (prp->p_flags & PF_VTEXT)
							dectext(vp, 1);
						prp = prp->p_prev;
						goto bad;
					}
				}
			}
			break;

#if defined(SHMEM) || defined(_WSIO)

		     /*
		      * This code is used to keep the shared memory
		      *  attach counts current for ipcs(1).
		      */

		     case PT_SHMEM:
			shm_fork(prp->p_reg);
			break;

#endif /* defined(SHMEM) || defined(_WSIO) */

		      default:
			break;

		}

		/*
		 * Now copy each pregion appropriately.
		 */
		if (prp->p_reg->r_type == RT_PRIVATE) {
			if (private_copy(dvas, prp))
				goto bad;
		} else {
			if (shared_copy(dvas, prp))
				goto bad;
		}
	}
	/* The null dereference region is RT_SHARED, while the PT_TEXT region
	** for this process could be RT_PRIVATE.  The hardware dependent layer  
	** is implemented in a way that requires the TEXT pregion to be attached
	** before the null dereference pregion.  Thats why we do it here.
	*/
	if (nulldrefprp != NULL) {
		if (add_nulldref (NULL, dvas) != 0) {
			prp = svas->va_prev;
			goto bad;
		}
	}
	return(dvas);

bad:
	/*
	 * Walk backwards through the ones that worked and decrement
	 * the text and mapping counts when appropriate.
	 */
	for (; prp != (preg_t *)svas; prp = prp->p_prev) {
		/*
		 * Perform type specific actions.
		 */
		switch (prp->p_type) {
		struct vnode *vp;
		case PT_TEXT: /* Fall through */
		case PT_MMAP:
			/*
			 * Unbump the mapping count and text image counts.
			 */
			vp = prp->p_reg->r_fstore;
			if (vp) {
				if (prp->p_flags & PF_VTEXT)
					dectext(vp, 1);
				VASSERT(vp->v_vas != (vas_t *)0);
				unmapvnode(vp);
			}
			break;

#if defined(SHMEM) || defined(_WSIO)
 	/*
	 * This code is used to keep the shared memory attach counts current
	 *  for ipcs(1).
	 */

		case PT_SHMEM:
		     shm_fork_backout(prp->p_reg);
		     break;

#endif

		}
	}

	/*
	 * Remove all of the allocated pregion(s)/region(s).
	 */
	while (dvas->va_next != (preg_t *)dvas) {
		prp = dvas->va_next;
		reglock(prp->p_reg);
		detachreg(dvas, prp);
	}

	/*
	 * Free the destination vas allocated above.
	 */
	vaslock(dvas);
	dvas->va_refcnt--;
	freevas(dvas);
	return((vas_t *)NULL);
}

int
private_copy(vas, prp)
	vas_t *vas;
	preg_t *prp;
{
	reg_t *rp;
	preg_t *prp2;

	VASSERT(prp->p_reg->r_type == RT_PRIVATE);

	reglock(prp->p_reg);
	rp = dupreg(prp->p_reg, prp->p_reg->r_type, 0, prp->p_reg->r_pgsz);
	if (rp == NULL)
		goto bad;
	prp2 = attachreg(vas, rp, prp->p_prot, prp->p_type,
			    (prp->p_flags & ~(PF_ACTIVE|PF_MLOCK|PF_NOPAGE)) |
			     PF_EXACT,
			     prp->p_vaddr, prp->p_off,
			     prp->p_count);
	if (prp2 == NULL)
		goto bad;
	hdl_mprot_dup(prp, prp2);
	regrele(rp);
	regrele(prp->p_reg);
	return(0);

bad:
	regrele(prp->p_reg);
	if (rp)
		freereg(rp);
	return(1);
}

int
shared_copy(vas, prp)
	vas_t *vas;
	preg_t *prp;
{
	preg_t *prp2;
	int noexact;

	VASSERT(prp->p_reg->r_type == RT_SHARED);

	if (noexact = ((prp->p_flags&PF_EXACT) == 0))
		prp->p_flags |= PF_EXACT;
	reglock(prp->p_reg);
	if ((prp2 = duppregion(vas, prp)) == (preg_t *)NULL)
		goto bad;
	hdl_mprot_dup(prp, prp2);
	regrele(prp->p_reg);
	if (noexact)
		prp->p_flags &= ~PF_EXACT;
	return(0);

bad:
	regrele(prp->p_reg);
	return(1);
}

/*
 * Insert the new pregion into the virtual address space list.
 * Pregions are inserted into the list in ascending order.
 * Note: both space and offset are conisdered.
 */
void
insertpreg(vas, prp)
	register vas_t *vas;
	register preg_t *prp;
{
	register preg_t *prp2;


	/*
	 * Keep others from walking this pregion list.
	 */
	vaslock(vas);
	prp2 = vas->va_next;
	while (prp2 != (preg_t *)vas &&
	       prpgreaterthan(prp, prp2->p_space, prp2->p_vaddr)) {
		prp2 = prp2->p_next;
	}
	prp->p_next = prp2;
	prp->p_prev = prp2->p_prev;
	prp->p_prev->p_next = prp;
	prp->p_next->p_prev = prp;
	VA_INS_CACHE(vas, prp);
	vasunlock(vas);
}

/* the "slow version" -- it walks the pregion chain.
 * if we are called, the fast version has already missed;
 * the vas, however, is locked.
 * looks much like searchprp().
 */
preg_t *
findpreg_slowcase(vas, space, vaddr, flags)
	vas_t	*vas;
	space_t	space;
	caddr_t	vaddr;
	int	flags;
{
	int greater;
	preg_t *prp;

	if (vas->va_next == (preg_t *)vas)
		goto notfound;

	if (space == (space_t)-1) {
		greater = 0;
		prp = vas->va_next;
	} else {
		prp = vas->va_cache[0];	/* know it isn't here. got a better starting place? */
		if (prpgreaterthan(prp, space, vaddr)) {
			prp = prp->p_prev;
			greater = 1;
		} else {
			prp = prp->p_next;
			greater = 0;
		}
	}

	while (prp != (preg_t *)vas) {
		if (prpcontains_exact(prp, space, vaddr,
					 (flags & FPRP_MINUS1)))
			goto found;
		if (greater)
			prp = prp->p_prev;
		else
			prp = prp->p_next;
	}
notfound:
	vasunlock(vas);
	return(NULL);
found:
	VA_INS_CACHE(vas, prp);
	vasunlock(vas);
	if (flags & FPRP_LOCK) {
		reglock(prp->p_reg);
		VASSERT(prpcontains_exact(prp, space, vaddr,
			(flags & FPRP_MINUS1)));
	}
	return(prp);
}


preg_t *
findpreg(vas,space,vaddr,flags)
	vas_t 	*vas;
	space_t	space;
	caddr_t vaddr;
	int	flags;
{
	preg_t *prp;

	if (!cvaslock(vas)) {
		if (flags & FPRP_COND)
			return((preg_t *)-1);
		else
			vaslock(vas);
	}
	/*
	 * Note that if we change VA_CACHE_SIZE to >1, must reinstall the loop
	 * here.  (Was removed for performance)
	 */

	if ((prp = vas->va_cache[0]) != (preg_t *)vas) {
		if (prpcontains_exact(prp, space, vaddr,
				      (flags & FPRP_MINUS1))) {
			vasunlock(vas);
			if (flags & FPRP_LOCK) {
				reglock(prp->p_reg);
				VASSERT(prpcontains_exact(prp, space, vaddr,
						  (flags & FPRP_MINUS1)));
			}
			return(prp);
		}
	}
	return(findpreg_slowcase(vas,space,vaddr,flags));
}


#ifdef NEVER_CALLED

/*
 *	Find the region corresponding to a virtual address. -1 for
 *	the space will take the first region containing the virtual
 *	address.  This should always be unique, since we don't allow
 *	the virtual addresses for a process to overlap (same address
 *	in two different spaces.)  -1 is a safe number since space
 *	ID's are never allowed to be greater than a protection ID
 *	number, which is only 15 bits.
 */

reg_t *
searchreg(vas, space, vaddr)
	vas_t *vas;
	space_t space;
	caddr_t	vaddr;
{
	register preg_t *prp;
	if ((prp = searchprp(vas, space, vaddr)) == NULL)
		return(NULL);
	return(prp->p_reg);
}

/*
 * Find the region corresponding to a vritual address. -1 for the
 * space means don't care.  This routine returns the region locked.
 */

reg_t *
findreg(vas, space, vaddr)
	vas_t *vas;
	space_t space;
	caddr_t	vaddr;
{
	register preg_t *prp;
	reg_t	*ret;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	if ((prp = findprp(vas, space, vaddr)) == NULL)
		ret = NULL;
	else
		ret = prp->p_reg;
	vmemp_unlockx();	/* free up VM empire */
	return(ret);
}
#endif /* NEVER_CALLED */

/*
 * Summarize usage within a vas.  Return 0 usage if can't get the vas, and
 * have return value of -1.  Otherwise fill in ints and return 0.
 */
pregusage(vas, dsize, tsize, ssize)
    register vas_t *vas;
    int *dsize, *tsize, *ssize;
{
	register preg_t *prp;
	register int ds, ts, ss;

	/* Couldn't get it, just return 0s */
	if ((!vas) || (!cvaslock(vas))) {
		*dsize = *tsize = *ssize = 0;
		return(-1);
	}

	/*
	 * Scan across the vas's pregions, summing for the
	 * types we care about
	 */
	ds = ts = ss = 0;
	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		switch (prp->p_type) {
			case PT_TEXT:
			    ts += prp->p_count;
			    break;
			case PT_DATA:
			    ds += prp->p_count;
			    break;
			case PT_STACK:
#ifdef hp9000s800
			    ss += prp->p_count;
#endif /* hp9000s800 */
#ifdef __hp9000s300
			    /* Due to a design error regions cannot grow in
			     * both directions.  The s300 stacks grow down.
			     * To kludge around this the s300 allocates a
			     * stack of maxssize and demand faults it in.
			     * Thus the number of swap pages allocated is
			     * the actual size of the region.  ICK.
			     * We should fix regions to grow down and
			     * get rid of all this kludgy s300 stack code.
			     */
			    ss += prp->p_reg->r_swalloc;
#endif /* __hp9000s300 */
			    break;
			default:
			    break;
		}
	}
	*dsize = ds;
	*tsize = ts;
	*ssize = ss;
	vasunlock(vas);
	return(0);
}

#ifdef NEVER_CALLED
/*
 * Summarize memory usage.  This routine is much like pregusage() but
 * it operates in terms of PHYSICAL pages allocated under the pregion.
 */
regusage(vas, dsize, tsize, ssize)
	register vas_t *vas;
	int *dsize, *tsize, *ssize;
{
	register preg_t *prp;
	register int x;
	register int ds, ts, ss;

	/* Couldn't get it or vas is NULL, just return 0s */
	if (!vas || !cvaslock(vas)) {
		*dsize = *tsize = *ssize = 0;
		return(-1);
	}

	/*
	 * Scan across the vas's pregions, summing for
	 * the types we care about
	 */
	ds = ts = ss = 0;
	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		x = prp->p_reg->r_nvalid;
		switch (prp->p_type) {
		case PT_TEXT:
			ts += x;
			break;
		case PT_DATA:
			ds += x;
			break;
		case PT_STACK:
			ss += x;
			break;
		default:
			break;
		}
	}
	*dsize = ds;
	*tsize = ts;
	*ssize = ss;
	vasunlock(vas);
	return(0);
}
#endif /* NEVER_CALLED */

/*
 * Find the pregion of a particular type.
 */
preg_t *
findpregtype(vas, type)
	register vas_t *vas;
	register int type;
{
	register preg_t *prp;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	vaslock(vas);
	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		if (prp->p_type == type) {
			vasunlock(vas);
			vmemp_returnx(prp);
		}
	}
	vasunlock(vas);
	vmemp_unlockx();	/* free up VM empire */
	return(NULL);
}

/*
 * kissofdeath - wipe out the kernel stack and uarea for the named process.
 *   Never call for yourself!
 */
void
kissofdeath(p)
	struct proc *p;
{
	register preg_t *prp;
	register reg_t *rp;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	VASSERT(p != u.u_procp);
	if ((prp = findpregtype(p->p_vas, PT_UAREA)) == NULL) {
		panic("kissofdeath: findpreg");
	}
	rp = prp->p_reg;
	reglock(rp);

	/* XXX note the NULL pointer--we don't have the U area */
	hdl_procdetach((struct user *)0, p->p_vas, prp);
	detachreg(p->p_vas, prp);
	vmemp_unlockx();	/* free up VM empire */

	/* give back stolen lockable mem */

	if (p->p_flag & SSYS)
		lockmemunreserve(UPAGES);
}



/*
 *	Dispose of all regions associated with a process but the
 *	kernel stack (lives in the PT_UAREA pregion).
 */
int
dispreg(p, vp, procattached)
	register struct proc *p;
	struct vnode *vp;
	int procattached;  /* flag used to allow hdl_procdetach() call */
{
	reg_t *rp;
	register preg_t *prp;
	preg_t *tprp;
	extern void detachshm();
	struct vnode *curtext_vp; /* Process' current text vnode */
	struct vnode *curmmap_vp; /* Process' current mmf vnode */
	int execself = 0;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	/* assert that either p is the current process, or else procattached
	 * is zero (no u-area exists for it) and vp is NULL.
	 */
	VASSERT((u.u_procp == p) || (!procattached && !vp));
	VASSERT(p->p_memresv <= 0);

	/*	We must check for the special case of a process
	 *	exec'ing itself.  In this case, we skip
	 *	detaching the regions and then attaching them
	 *	again because it causes deadlock problems.
	 */
	if (vp) {
		prp = findpregtype(p->p_vas, PT_TEXT);
		if (prp  &&  prp->p_reg->r_fstore == vp) {
			execself = 1;
		}
	}

	/*
	 * get rid of memory we declared to be locked earlier if
	 * this is the process that really allocated some.
	 *
	 * (if p != u.u_procp, we must be in the backout code)
	 */
	if ((p == u.u_procp) && (p->p_flag & SSYS))
	{
		int lock_count=1; /* one page for misc tables */
		lock_count += hdl_swapicnt(p, p->p_vas);
		lockmemunreserve(lock_count);
	}

	/*
	 * Dispose of all pregions except the kernel stack region.  In case
	 * of execself, don't detach TEXT or NULLDREF.
	 */
	for (prp = p->p_vas->va_next; prp != (preg_t *)p->p_vas; prp = tprp) {
		tprp = prp->p_next;
		rp = prp->p_reg;
		VASSERT(rp);

		switch (prp->p_type) {
		case PT_UAREA:
			/* Keep the U area */
			continue;
		case PT_NULLDREF:
			if (execself)
				continue;
			break;
		case PT_TEXT:
			if (execself) {
				/*
				 * if memory locked, inform region
				 * that we no longer need it locked. This
				 * is done by detachreg, if not execself.
				 */
				if (PREGMLOCKED(prp)) {
					reglock(rp);
					munlockpreg(prp);
					regrele(rp);
				}
				continue;	/* don't detach it */
			}

			/*
			 * Now decrement the text usage information in the
			 * vnode and unmap the vnode.  We are
			 * destroying this text image and  no longer need
			 * it mapped.
			 */
			curtext_vp = prp->p_reg->r_fstore;
			VASSERT(prp->p_flags & PF_VTEXT);
			dectext(curtext_vp, 1);
			unmapvnode(curtext_vp);
			break;
		case PT_MMAP:
			/*
			 * If this mapping asked for execute permission
			 * decrement the text usage info in the vnode and
			 * unmap the vnode.
			 */
			curmmap_vp = prp->p_reg->r_fstore;
			if (curmmap_vp != (struct vnode *) NULL) {
				if (prp->p_flags & PF_VTEXT)
					dectext(curmmap_vp, 1);

				unmapvnode(curmmap_vp);
			}
			break;
		}
		reglock(rp);
		if (procattached)
			 hdl_procdetach(&u, p->p_vas, prp);

#if defined(SHMEM) || defined(_WSIO)
		if (prp->p_type != PT_SHMEM) {
			detachreg(p->p_vas, prp);
		}
		else {
			/*
			 * If this is the last attach to a shared memory
			 * region that's been removed (IPC_RMID), throw
			 * the shared memory segment away.
			 *
			 * Detachshm() releases the region lock and
			 * reacquires it after the shmem list is locked.
			 * Therefore, do not count on the calls to
			 * hdl_prodetach() and detachshm() to be protected
			 * by the same region lock.
			 */
			detachshm(p->p_vas, prp);
		}
#else
		detachreg(p->p_vas, prp);
#endif
	}
	vmemp_unlockx();	/* free up VM empire */
	return(execself);
}

/* mlockpregtype(vas, type) - lock a pregion in memory
 *	Bring all pages from swap device and lock the region associated
 *	with the pregion (of the specified type) in memory.
 */

int
mlockpregtype(vas, type)
	vas_t *vas;
	int	type;
{
	register reg_t	*rp;
	preg_t	*prp;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	VASSERT(type != PT_UNUSED);

	prp = findpregtype(vas, type);
	if (prp == NULL)
		panic("mlockpreg: can't find pregion");
	if (PREGMLOCKED(prp))
		panic("mlockpregtype: pregion already memory locked");
	rp = prp->p_reg;
	reglock(rp);
	if (mlockpreg(prp)) {
		regrele(rp);
		vmemp_returnx(1);
	}
	regrele(rp);

	vmemp_unlockx();	/* free up VM empire */
	return(0);
}

/* munlockpregtype(vas, type) - unlock a pregion from memory
 *	Unlock the locked region associated with the pregion from memory.
 */

void
munlockpregtype(vas, type)
	vas_t *vas;
	int	type;
{
	register reg_t	*rp;
	preg_t	*prp;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	VASSERT(type != PT_UNUSED);

	prp = findpregtype(vas, type);
	if (prp == NULL)
		panic("munlockpregtype: can't find pregion");
	if (PREGMLOCKED(prp) == 0)
		panic("munlockpregtype: pregion not memory locked");
	rp = prp->p_reg;
	reglock(rp);
	munlockpreg(prp);
	regrele(rp);
	vmemp_unlockx();	/* free up VM empire */
	return;
}

#if defined(hp9000s800) && (NBPG == 4096)
/* Return the current size of the following major segments/regions
 * of process memory.  These routines are called by macros in vmmac.h;
 * the macros have the same name, all in uppercase.
 */
int
textsize(p)
struct proc *p;
{
	register preg_t *prp;
	register int	ret;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	if ((prp = findpregtype(p->p_vas, PT_TEXT)) == NULL)
		ret = 0;
	else
		ret = prp->p_count;

	vmemp_unlockx();	/* free up VM empire */
	return(ret);
}
#endif /* defined(hp9000s800) && (NBPG == 4096) */

int
ustacksize(p)
struct proc *p;
{
	register preg_t *prp;
	register int	ret;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	if ((prp = findpregtype(p->p_vas, PT_STACK)) == NULL)
		ret = 0;
	else
		ret = prp->p_count;

	vmemp_unlockx();	/* free up VM empire */
	return(ret);
}

int
datasize(p)
struct proc *p;
{
	register preg_t *prp;
	register int	ret;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	if ((prp = findpregtype(p->p_vas, PT_DATA)) == NULL)
		ret = 0;
	else
		ret = prp->p_count;
	vmemp_unlockx();	/* free up VM empire */
	return(ret);
}
