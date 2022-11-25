/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_misc.c,v $
 * $Revision: 1.8.83.9 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/07/20 10:57:31 $
 */
#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/vm.h"
#include "../h/vas.h"
#include "../h/vfd.h"
#include "../h/uio.h"
#include "../h/proc.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/pfdat.h"
#include "../h/vmmac.h"
#include "../h/systm.h"
#ifdef OSDEBUG
#include "../h/malloc.h"
#endif

extern preg_t   *stealhand;        /* list of active pregions */

/*
 * Temporarily lock a set of pages in memory.  Note: this routine may NOT
 * be called twice on the same set of pages without calling vsunlock.  
 * Overlapping calls will deadlock.
 */
vslock(off, cnt, pprp)
	register caddr_t off;
	register int cnt;
	preg_t	 **pprp;
{
	register preg_t *prp;
	space_t space;
	int length = cnt;	/* vaslockpages() may want to change it */
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	if ((prp = searchprp(u.u_procp->p_vas, -1, off)) == 0)
		panic("vslock: lock pages not in process space!");
	/* 
	 * Set pregion pointer so we don't have to search later (avoid
	 * deadlock with pfdat lock & region/vas locks)
	 */
	space = prp->p_space;

	if (vaslockpages(prp->p_vas, &space, off, &length, PROT_WRITE, 1)) {
		vmemp_unlockx();
		return(0);
	}

	/*
	 * If couldn't get the whole thing, fail the request
	 */
	if (cnt != length) {
		vasunlockpages(prp->p_vas, space, off, length);
		vmemp_unlockx();
		return(0);
	}
	/*
	 * If the space doesn't match we crossed a boundary,
	 * make the user break the request up.
	 */
	if (space != prp->p_space) {
		vasunlockpages(prp->p_vas, space, off, length);
		vmemp_unlockx();
		return(0);
	}
	*pprp = prp;	
	vmemp_unlockx();
	return(1);
}

/*
 * Unlock pages locked by vslock.  Turn on reference bit for the pages
 * unlocked.  In addition, turn on modification bit in the case of a
 * read operation.
 */
/*ARGSUSED*/
vsunlock(off, cnt, rw, prp)
	register caddr_t off;
	register int cnt;
	register int rw;
	register preg_t *prp;
{
        int     ret, i, pgcnt, bits, pfn;
	space_t space;
        caddr_t vaddr2;

        pgcnt = pagesinrange(off, cnt);
        vaddr2 = (caddr_t)((u_int)off &~ (NBPG-1));
	space = prp->p_space;

        /* Turn on reference bit.  For read operations,
         * turn on modification bit as well.
         */
        bits = (rw == B_READ) ? (VPG_REF | VPG_MOD) : VPG_REF;
        for (i = 0; i < pgcnt; i++, vaddr2 += ptob(1)) {
                pfn = hdl_vpfn(space, vaddr2);
                hdl_setbits(pfn, bits);
        }

	ret = vasunlockpages(prp->p_vas, space, off, cnt);
	return ret;
}

#ifdef MMF
vasuiomove(vas, n, rw, uio)
	vas_t *vas;
	int n;
	enum uio_rw rw;
	struct uio *uio;
{
	register struct iovec *iov;
	u_int cnt;
	int error = 0;
	preg_t *prp = vas->va_next;
	caddr_t vaddr = (prp->p_vaddr + uio->uio_offset);
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		switch (uio->uio_seg) {

		case 0:
		case 2:
			if (rw == UIO_READ) 
				error = vascopy(vas, -1, vaddr, 
						u.u_procp->p_vas, -1, 
						iov->iov_base, 
						(int)cnt, 0);
			else
				error = vascopy(u.u_procp->p_vas, -1, 
						iov->iov_base, 
						vas, -1, vaddr, (int)cnt, 0);
			if (error)
				return (error);
			break;

		case 1:
		case 3:
			/*
			 * For now assume kernel pages are locked 
			 * in memory.  Just do a bcopy.
			 */
			if (rw == UIO_READ) 
				error = vascopy(vas, -1, vaddr, KERNVAS, -1, 
						iov->iov_base, (int)cnt, 0);
			else
				error = vascopy(KERNVAS, -1, iov->iov_base, 
						vas, -1, vaddr, (int)cnt, 0);
			if (error)
				return (error);
			break;
		}
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		vaddr += cnt;
		n -= cnt;
	}
	vmemp_unlockx();	/* free up VM empire */
	return (error);
}
#endif /* MMF */

vascopy(svas, sspace, svaddr, dvas, dspace, dvaddr, bytes, priv)
	vas_t *svas, *dvas;
	space_t sspace, dspace;
	caddr_t svaddr, dvaddr;
	int bytes, priv;
{
	int error = 0;
	int scount, dcount;
	space_t nsspace, ndspace;
	
	/*
	 * Since the length could be contained in several
	 * pregions, we will loop through the destination
	 * vas finding each pregion and checking permissions
	 * as we go.
	 */
	while (bytes) {
		int n;
		scount = bytes;
		nsspace = sspace;
		error = vaslockpages(svas, &nsspace, svaddr, &scount, 
				     PROT_READ, priv);
		if (error)
			goto bad;

		dcount = bytes;
		ndspace = dspace;
		error = vaslockpages(dvas, &ndspace, dvaddr, &dcount, 
				     PROT_WRITE, priv);
		if (error) {
			vasunlockpages(svas, nsspace, svaddr, scount);
			goto bad;
		}
		
		n = MIN(scount, dcount);
		hdl_copy_page(svas, nsspace, svaddr, dvas, ndspace, dvaddr, n);
		error = vasunlockpages(svas, nsspace, svaddr, scount);
		error += vasunlockpages(dvas, ndspace, dvaddr, dcount);
		if (error)
			goto bad;
		svaddr += n;
		dvaddr += n;
		bytes -= n;
	}
	return(0);
bad:	
	printf("vascopy: got an error\n");
	return(error);
}

vaslockpages(vas, nspace, vaddr, ncount, prot, priv)
        vas_t *vas;
        space_t *nspace;
        caddr_t vaddr;
        int *ncount;
        int prot, priv;
{
        int error = 0;
        preg_t *prp;
        reg_t *rp = NULL;
        int pgcnt;
        int i;
        size_t index;
        vfd_t *vfd;
        unsigned int pfn;

        /*
         * Ensure valid addresses and appropriate permissions.
         */
        if (vas == KERNVAS) {   
                (*nspace) = KERNELSPACE;
                return(0);
        }
        if ((prp = findprp(vas, *nspace, vaddr)) == (preg_t *)NULL) {
                error = EFAULT;
                goto bad;
        }

        rp = prp->p_reg;
        if (!priv && ((prp->p_prot&prot) == 0)) {
                error = EFAULT;
                goto bad;
        }

        if (rp && (rp->r_zomb)) {
                error = EIO;
                goto bad;
        }

        /*
         * ncount gets set to the smaller of what was
         * asked for or the amount left in the pregion.
         */
        (*ncount) = MIN(*ncount, 
                        (ptob(prp->p_count) - 
                         ((u_int)vaddr - (u_int)prp->p_vaddr)));
        (*nspace) = prp->p_space;
        pgcnt = pagesinrange(vaddr, *ncount);

        /* Round down the vaddr */
        vaddr = (caddr_t) ((unsigned) vaddr & ~(NBPG-1));

        make_unswappable();
        /*
         * bring_in_pages() loops through setting the lock bits on the pages
         * and faulting in the invalid pages.
         */
        if ((error = bring_in_pages(prp,vaddr,pgcnt,
                        (prot == PROT_WRITE)?1:0,1))) {
                goto bad2;
        }

        index = regindx(prp, vaddr);
        for (i = 0; i < pgcnt; i++,index++) {

                VASSERT(index <rp->r_pgsz);
                if ((vfd = FINDVFD(rp, (int)index)) == (vfd_t *)NULL)
                        panic("vaslockpages: vfd not found");
                pfnlock((pfn = vfd->pgm.pg_pfn));
                hdl_addtrans(prp, prp->p_space,
                                (caddr_t) ptob(btop(vaddr + ptob(i))),
                                        vfd->pgm.pg_cw, (int) vfd->pgm.pg_pfn);
        }
        if (rp) {
                regrele(rp);
        }
        return(error);
bad2:
        make_swappable();

bad:
        
        if (rp) {
                regrele(rp);
        }
        return(error);
}

vasunlockpages(vas, space, vaddr, count)
        vas_t *vas;
        space_t space;
        caddr_t vaddr;
        int count;
{
        preg_t *prp;
        reg_t *rp = NULL;
        int error = 0;
        int i, pgcnt;
        caddr_t vaddr2;
        size_t index;

        if (vas == KERNVAS)
                return(0);
        pgcnt = pagesinrange(vaddr, count);
        /*
         * Free up the pfdat locks before going for vas or region locks
         */
        vaddr2 = (caddr_t)((u_int)vaddr &~ (NBPG-1));
        for (i = 0; i < pgcnt; i++, vaddr2 += ptob(1)) {
                struct pfdat    *pfd;
                /*
                 * Get the page number to free.
                 */
                pfd = &pfdat[hdl_vpfn(space, vaddr2)];
#ifdef PFDAT32
                VASSERT(pfd_is_locked(pfd));
#else	
                VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	
                pfdatunlock(pfd);
        }

        /*
         * Find the pregions.
         */
        if ((prp = findprp(vas, space, vaddr)) == (preg_t *)NULL) {
                error = EFAULT;
                goto bad;
        }
        rp = prp->p_reg;
        /*
         * unwire pages loops through and unsets the pg_lock bit.
         */
        unwire_pages(prp,vaddr,pgcnt);
        regrele(rp);
bad:
        make_swappable();

        return(error);
}

/*
 *  bring_in_pages(prp,vaddr,count,break_cow,wire_down)
 *
 *  1. faults in a given range of pages in a pregion, resolving copy on write,
 *  if specified.
 *  2.If specified, pages are wired down in memory so that vhand/swap cannot 
 *  steal *  them. The kernel client must unfreeze these pages when it is done 
 *  by calling unwire_pages() or by setting the vfd.pgm.pg_lock bit to 0.
 *
 *      must hold reglock on entry.
 *
 *      returns:
 *         0 : on success
 *         non zero : on failure
 *
 * If  break_cow is non-zero, copy on write is broken. 
 *
 * The wire_down option ensures that vhand does not steal the page away.
 * But it does NOT ensure persistent translation. For this you must hold
 * the pfdatlock(for eg: see vaslockpages()). There are code paths where the 
 * translation of a page may change even though the page will not be stolen 
 * from you.
 */
bring_in_pages(prp,vaddr,count,break_cow,wire_down)
        preg_t *prp;
        caddr_t vaddr;
        int count;
        int break_cow;
        int wire_down;
{
        int error = 0;
        reg_t *rp = prp->p_reg;
        size_t index;
        vfd_t *vfd;
        int i;
        unsigned int spl_level;



        VASSERT(vm_valusema(&rp->r_lock) <= 0);
        index = regindx(prp, vaddr);
        for (i = 0; i < count; i++,index++) {
                VASSERT(index <rp->r_pgsz);
                if ((vfd = FINDVFD(rp, (int)index)) == (vfd_t *)NULL)
                        panic("bring_in_pages: vfd not found");

                if (wire_down) {
                        /*
                         * Check if someone else already set the lock bit
                         * if so back out and wait for them to clear it up.
                         */
                        while (vfd->pgm.pg_lock) {
                                /*
                                 * want to get soundly asleep before someone 
                                 * else can wake us up. (MP issue). Note that 
                                 * the sleep_lock gets undone by the time we 
                                 * wake up from sleep().
                                 */
                                spl_level = SLEEP_LOCK();
                                rp->r_flags |= RF_WANTLOCK;
                                regrele(rp);
                                SLEEP_THEN_UNLOCK(rp, PSLEP, spl_level);
                                reglock(rp);
                                /*
                                 * The vfd pointer could have become stale when
                                 * we slept without the region lock.
                                 */
                                vfd = FINDVFD(rp, (int)index);
                        }
                        vfd->pgm.pg_lock = 1;
                }
                if (!vfd->pgm.pg_v) {

                        if (virtual_fault(prp, break_cow,
                                          prp->p_space, vaddr+ptob(i))) {
                                error = ENOSPC;
                                goto bad2;
                        }
                        /*
                         * The vfd pointer could have become stale when
                         * we slept without the region lock in
                         * virtual_fault()
                         */
                        vfd = FINDVFD(rp, (int)index);
                
                }
                VASSERT(vfd->pgm.pg_v);

                /*
                 * break copy-on-write if they want to write to it.
                 *
                 * If we did a vfault (above), it would have called
                 * hdl_cwfault() and done this for us; in which case,
                 * pg_cw will no longer be set; but if the page was
                 * valid and cw, vfault() won't be called and we must
                 * call pfault() here, directly. Anyhow, it's risky to
                 * rely on the pfault()ing-side effect of vfault().
                 */
                if (break_cow && vfd->pgm.pg_cw) {
                        if (prot_fault(prp, 1, prp->p_space, vaddr+ptob(i))) {
                                error = ENOSPC;
                                goto bad2;
                        }
#ifdef OSDEBUG
                        /*
                         * The vfd pointer could have become stale when
                         * we slept without the region lock in
                         * prot_fault()
                         */
                        vfd = FINDVFD(rp, (int)index);
                        VASSERT(vfd->pgm.pg_cw == 0);
#endif
                }
        }
        return(0);
bad2:
        /* backing out after faulting in some pages */
        if (wire_down) {
                int j;
                pfd_t *pfd;

                index = regindx(prp, vaddr );
                for (j = 0; j <= i; j++,index++) {
                        VASSERT(index <rp->r_pgsz);
                        if ((vfd = FINDVFD(rp, (int)index)) == (vfd_t *)NULL)
                                panic("bring_in_pages: vfd not found");
                        vfd->pgm.pg_lock = 0;
                }
        }
        return(error);
}

/* 
 *
 * unwire_pages(prp,vaddr,count)
 *
 * - loops through clearing up the lock bits on the source pages
 * - Wakes up anyone sleeping on those bits. i.e sleeping on region lock
 *   with the RF_WANTLOCK flag set.
 */

unwire_pages(prp,vaddr,count)
        preg_t *prp;
        caddr_t vaddr;
        int count;
{
        reg_t *rp = prp->p_reg;
        size_t index;
        vfd_t *vfd;
        int i;
        unsigned int spl_level;

        VASSERT(vm_valusema(&rp->r_lock) <= 0);
        /*
         * Loop through clearing the lock bits on the source pages.
         */ 
        index = regindx(prp, vaddr);
        for (i = 0; i < count; i++,index++) {
                if ((vfd = FINDVFD(rp, (int)index)) == (vfd_t *)NULL)
                        panic("unwire_pages: vfd not found");
                vfd->pgm.pg_lock = 0;
        }
        /*
         * we must make sure that any other (MP) process that might
         * be setting this flags gets done and soundly asleep before
         * we try to wake them up.
         */
        spl_level = SLEEP_LOCK();
        if (rp->r_flags&RF_WANTLOCK) {
                rp->r_flags &= ~RF_WANTLOCK;
                wakeup((caddr_t)rp);
        }
        /*
         * Since we come out of wakeup() still holding the sleep_lock,
         * this would be a good time to ditch it.
         */
        SLEEP_UNLOCK(spl_level);        
}

/*
 * foreach_pregion() --
 *    Loop through all pregions spanning [addr, addr+len), calling
 *    fn() as fn(prp, idx, count, arg).  If fn() returns a non-zero
 *    value, the looping terminates and the value returned is passed
 *    back to the caller.  If some address within [addr, addr+len)
 *    is not valid, we set u.u_error to EFAULT and return 1.
 *
 * NOTE: when 'fn' is called, the region referenced by prp will be
 *       locked.  It is the responsibility of 'fn' to regrele() the
 *       region.  foreach_pregion() will not reference the 'prp' or
 *       the region after it has called 'fn' (so that 'fn' can free
 *       the prp and/or the region).
 */
int
foreach_pregion(addr, len, fn, arg)
caddr_t addr;
u_long len;	/* in bytes */
int (*fn)();
void *arg;
{
    vas_t *vas;		/* our virtual address space */

    VASSERT((caddr_t)pagedown(addr) == addr);

    /*
     * Now go through the requested address range.  This may span
     * several pregions, so we loop through until we exhaust all of
     * the range, fn returns non-zero, or we encounter an address
     * that is not valid for the proces.
     *
     * As a special case, we validate the starting addr if len is 0.
     */
    len = btorp(len);
    vas = u.u_procp->p_vas;
    for (;;) {
	preg_t *prp;	/* current pregion */
	long off;	/* offset into pregion (pages) */
	long count;	/* number of pages to operate on */
	int ret;

	prp = findprp(vas, -1, addr);
	if (prp == (preg_t *)0) {
	    u.u_error = EFAULT;
	    return 1;
	}

	/*
	 * Special check so that we validate 'addr', even
	 * when count is 0.
	 */
	if (len == 0) {
	    regrele(prp->p_reg);
	    return 0;
	}

	/*
	 * Compute how many pages from this pregion we are to
	 * operate on.
	 */
	off = btop(addr - prp->p_vaddr);
	count = prp->p_count - off;
	count = MIN(len, count);

	/*
	 * Part of this address range could really be unmapped.
	 * Verify the range by looking at the hardware dependent
	 * data.
	 */
	if (!hdl_range_mapped(prp, off, count)) {
	    regrele(prp->p_reg);
	    u.u_error = EFAULT;
	    return 1;
	}

	if ((ret = (*fn)(prp, off, count, arg)) != 0)
	    return ret;

	if ((len -= count) == 0)
	    return 0;
	addr += ptob(count);
    }
    /* NOTREACHED */
}


/*
 *	XXX should we get rid of this in favor of mem_sum() below?  
 */
sum_pfdat()
{
	struct pfdat *pfd;
	int kernel, free, hash, user;
	extern int bufpages, physmem, pfdatnumentries;
	

	kernel = free = hash = user = 0;
	printf("There are %d %dK pages, %d of which are in pfdat[]\n", 
		physmem, NBPG/1024, pfdatnumentries);
	for (pfd = pfdat; pfd < (pfdat + pfdatnumentries); pfd++) {
		if (pfd->pf_flags & P_SYS)
			kernel++;
		else if (pfd->pf_flags & P_QUEUE)
			free++;
		else if (pfd->pf_flags & P_HASH)
			hash++;			
		else 
			user++;			
	}
	
	printf("Kernel: %d (%d BC)  Free: %d   Hash: %d  User: %d  Total: %d\n",
		kernel+(physmem-pfdatnumentries), bufpages, free, hash, user,
		kernel+(physmem-pfdatnumentries) + free + hash + user);
}

	

#ifdef OSDEBUG

/*
 * Local function invoked to print out the page frame number of each vfd
 */
/*ARGSUSED*/
print_vfd_info(idx, vfd, dbd, arg)
	int idx;
	register vfd_t *vfd;
	dbd_t *dbd;
	int arg;
{
	printf("    0x%x\n",vfd->pgm.pg_pfn);
	return(0);
}

print_vfds(prp)
	preg_t *prp;
{
	extern void foreach_valid();
	foreach_valid(prp->p_reg,(int)prp->p_off,(int)prp->p_count,
						print_vfd_info,(caddr_t)0);
}

/* Tacky little routine to display memory usage overview */
extern int firstfree, maxfree;
int mem_lev = 0;
size_t preglow = 0;
size_t reglow = 0;
size_t preghigh = 0xfffffff;
size_t reghigh = 0xfffffff;

mem_sum()
{
	register preg_t *prp;
	register reg_t *rp;
	register int nreg, nzero, npages, nlow, nswap, nfree, psys, phdl;
	extern struct pfdat phead;
	register struct pfdat *pfd;
	extern int freemem;
	int i, numlocked;
	int locked_reg, locked_reg_pages;
	int nused, nunused;
	
	nreg = nzero = npages = nlow = nswap = nfree = psys = phdl = 0;

	locked_reg = locked_reg_pages = 0;

	nused = nunused = 0;


	/* 
	 * Count # of pages locked but not P_SYS 
	 * also count the number of P_SYS pages.
	 */
	numlocked = 0;
	for (i = firstfree, pfd = pfdat + i; i < maxfree; i++, pfd++) {
		if (pfd->pf_use > 0)
			nused++;
		else
			nunused++;

		if (pfd->pf_flags & P_SYS) {
			++psys;
			if (pfd->pf_flags & P_HDL)
				++phdl;
			continue;
		}
#ifdef PFDAT32
		if (pfd->pf_locked) {
#else	
		if (vm_valusema(&pfd->pf_lock) <= 0) {
#endif	
			++numlocked;
			/* Level 1: print the page frame number */
			if (mem_lev > 0)
			   printf("Page (0x%x) locked and not P_SYS\n",
#ifdef PFDAT32
				   pfd - pfdat);
#else
				   pfd->pf_pfn);
#endif
		}
	}

	/* Count # pages on free list */
	for (pfd = phead.pf_next; pfd != &phead; pfd = pfd->pf_next)
		++nfree;

	/* Scan active pregions, summarizing usage */
	prp = stealhand;
	do {
		++nreg;
		rp = prp->p_reg;

		if (mem_lev > 0) {
			if ((prp->p_count >= preglow) && (prp->p_count <= preghigh))
				print_preg(prp, (char *)0);
		}
		/* Level 2: print region summary too */
		if (mem_lev > 1) {
			if ((rp->r_pgsz >= reglow) && (rp->r_pgsz <= reghigh))
				print_reg(rp, (char *)0);
		}
		/* Level 3: print page frame numbers attached to region */
		if (mem_lev > 2) {
			printf("page frame numbers:\n");
			foreach_valid(prp->p_reg, (int)prp->p_off, 
				(int)prp->p_count, print_vfd_info, (caddr_t)0);
		}

		/* Calculate statistics */
		if (rp->r_nvalid == 0) 
			++nzero;
		else {
	    		npages += rp->r_nvalid;
	    		if (rp->r_nvalid < 5)
				++nlow;
		}
		nswap += rp->r_swnvalid;
		if (rp->r_mlockcnt) {
			locked_reg++;
			locked_reg_pages += rp->r_nvalid;
		}
		prp = prp->p_forw;
	} while (prp != stealhand);
	printf("physical memory in system: %d pages.\n", physmem);
	printf("freemem at time of boot: %d pages.\n", maxmem);
    	printf("%d P_SYS pages and %d P_HDL pages.\n", psys, phdl);
    	printf("%d pages locked and not P_SYS.\n", numlocked);
	printf("%d regions locked, using %d pages\n", locked_reg, 
		locked_reg_pages);
    	printf("%d pages of free memory, %d reserved.\n", 
		nfree, nfree - freemem);
	printf("%d pages of memory mapped by a region(s).\n", npages);
	printf("%d pages swapped by a region(s).\n", nswap);
	printf("%d pregions, %d with no pages, %d with less than 5.\n",
						nreg, nzero, nlow);
	printf("%d pages with zero use count, %d pages with use count > 0\n", 
						nunused, nused);
}

check_swapped()
{
	preg_t *prp;
	int found = 0;

	/* Scan active pregions, summarizing usage */
	prp = stealhand;
	do {
		reg_t *rp = prp->p_reg;
		if (rp->r_incore == 0) {
			found++;
			if (rp->r_nvalid != 0)
				print_reg(rp, "r_nvalid not 0");
			if (rp->r_dbd == 0)
				print_reg(rp, "r_dbd is 0");
		}
		prp = prp->p_forw;
	} while (prp != stealhand);
	if (found == 0)
		printf("check_swapped: none found\n");
}

print_preg(prp, str)
	preg_t *prp;
	char *str;
{
	if (str)
		printf("%s:\n", str);
	printf("preg 0x%x type 0x%x vaddr 0x%x count %d nval %d\n", 
		prp, prp->p_type, prp->p_vaddr, 
		prp->p_count, prp->p_reg->r_nvalid);
	
}
print_reg(rp, str)
	reg_t *rp;
	char *str;
{
	if (str)
		printf("%s:\n", str);
	printf(" reg 0x%x swapnval %d pgsz %d byte/len %d/%d\n",
	       rp, rp->r_swnvalid, rp->r_pgsz, rp->r_byte, 
	       rp->r_bytelen);
	
}

#endif /* OSDEBUG */

/*
 * bvtospace - given a buffer pointer and an offset, tell me what
 *             space the offset is in.
 */
space_t
bvtospace(b, v)
register struct buf *b;
register caddr_t v;
{
	VASSERT(v == b->b_un.b_addr);   /* why pass in v at all? */
	/* correct space always stuffed into buffer header */
	return(b->b_spaddr);
}

#ifdef OSDEBUG
/*
 *  kmem_bucket_info and kmem_type_info, print out statistical info about
 *  memory allocated through MALLOC and kmalloc, and freed through FREE 
 *  and kfree for each bucket size and for each type of memory.  They use
 *  kmem_print_buckets and kmem_print_types to format the data.
 */

char *mem_type[]={"    M_FREE", "    M_MBUF", " M_DYNAMIC", "  M_HEADER",
		  "  M_SOCKET", "     M_PCB", "  M_RTABLE", "  M_HTABLE",
		  "  M_ATABLE", "  M_SONAME", "  M_SOOPTS", "  M_FTABLE",
		  "  M_RIGHTS", "  M_IFADDR", "   M_NTIMO", "  M_DEVBUF",
		  "  M_ZOMBIE", "   M_NAMEI", "   M_GPROF", "M_IOCTLOPS",
                  "M_SUPERBLK", "    M_CRED", "    M_TEMP", "     M_VAS",
		  "    M_PREG", "     M_REG", "   M_IOSYS", "  M_NIPCCB",
		  "  M_NIPCSR", "  M_NIPCPR", "  M_DQUOTA", "     M_DMA",
		  "    M_GRAF", "     M_ITE", "     M_ATL", "M_LOCKLIST",
		  "     M_DBC", "M_LOSTPAGE", " M_COWARGS", " M_SWAPMAP",
		  "   M_TRACE", "     M_VFD", "M_KMEM_ALL", "   M_UMEMD",
		  /* 
		   * The following are placeholders that will appear if
		   * memory types are added in malloc.h but not here.
		   */
		  "NEED2ADDME", "NEED2ADDME", "NEED2ADDME", "NEED2ADDME"};

#define COLUMNS 5
#define KMEM_TYPE_FIELDS 9
#define KMEM_BUCKET_FIELDS 4
#define BUCKETSTART 0 /* could use MINBUCKET instead, but want to see if
                      * anything was written in the first MINBUCKET buckets */
#define MAX_ROW ((((MAXBUCKET-BUCKETSTART)/COLUMNS)+1) * (COLUMNS) + \
		 BUCKETSTART)

kmem_print_buckets(bucketsize, field, bucket)
	int bucketsize;
	int field;
	struct kmembuckets *bucket;
{
	int tempsize=bucketsize;

	switch (field) {
	      case 0:
		printf("\nBucket Number: ");
		break;
	      case 1:
		printf("\nkb_next      : ");
		break;
	      case 2:
		printf("\nkb_calls     : ");
		break;
	      case 3:
		printf("\nkb_total     : ");
		break;
	      case 4:
		printf("\nkb_totalfree : ");
		break;
	      default:
	        printf("\nNew fields have been added to struct");
		printf(" kmembuckets, update kmem_print_buckets!\n");
		return;
	}

	while ((tempsize - bucketsize < COLUMNS) && 
	       (tempsize <= MAXBUCKET)) {
		switch (field) {
		      case 0:
			printf("%10d ", tempsize);
			break;
		      case 1:
			printf("%10lx ", bucket[tempsize].kb_next);
			break;
		      case 2:
			printf("%10ld ", bucket[tempsize].kb_calls);
			break;
		      case 3:
			printf("%10ld ", bucket[tempsize].kb_total);
			break;
		      case 4:
			printf("%10ld ", bucket[tempsize].kb_totalfree); 
			break;
		}
		tempsize++;
	}
}


kmem_print_types(m_type, field)
	int m_type;
	int field;
{
	int temp_m_type=m_type;

	switch (field) {
	      case 0:
		printf("\nMemory Type  : ");
		break;
	      case 1:
		printf("\nks_flags     : ");
		break;
	      case 2:
		printf("\nks_check     : ");
		break;
	      case 3:
		printf("\nks_sleep     : ");
		break;
	      case 4:
		printf("\nks_reslimit  : ");
		break;
	      case 5:
		printf("\nks_resinuse  : ");
		break;
	      case 6:
		printf("\nks_inuse     : ");
		break;
	      case 7:
		printf("\nks_calls     : ");
		break;
	      case 8:
		printf("\nks_memuse    : ");
		break;
	      case 9:
		printf("\nks_maxused   : ");
		break;
	      default:
	        printf("\nNew fields have been added to struct");
		printf(" kmemstats, update kmem_print_types!\n");
		return;
	}

	temp_m_type=m_type;
	while ((temp_m_type - m_type < COLUMNS) && 
	       (temp_m_type < M_LAST)) {
		switch (field) {
		      case 0:
			printf("%s ", mem_type[temp_m_type]);
			break;
		      case 1:
			printf("%10ld ", kmemstats[temp_m_type].ks_flags);
			break;
		      case 2:
			printf("%10ld ", kmemstats[temp_m_type].ks_check);
			break;
		      case 3:
			printf("%10ld ", kmemstats[temp_m_type].ks_sleep);
			break;
		      case 4:
			printf("%10ld ", kmemstats[temp_m_type].ks_reslimit);
			break;
		      case 5:
			printf("%10ld ", kmemstats[temp_m_type].ks_resinuse);
			break;
		      case 6:
			printf("%10ld ", kmemstats[temp_m_type].ks_inuse);
			break;
		      case 7:
			printf("%10ld ", kmemstats[temp_m_type].ks_calls);
			break;
		      case 8:
			printf("%10ld ", kmemstats[temp_m_type].ks_memuse);
			break;
		      case 9:
			printf("%10ld ", kmemstats[temp_m_type].ks_maxused);
			break;
		}
		temp_m_type++;
	}
}

kmem_bucket_info()
{

	int bucketsize, i;
        
	for (bucketsize=0; bucketsize<MAX_ROW; bucketsize+=COLUMNS) {
		for (i=0; i<=KMEM_BUCKET_FIELDS; i++) {
			kmem_print_buckets(bucketsize, i, bucket);
		}
		printf("\n");
	}
}

kmem_reserved_info()
{

	int bucketsize, i;
        
	for (bucketsize=0; bucketsize<MAX_ROW; bucketsize+=COLUMNS) {
		for (i=0; i<=KMEM_BUCKET_FIELDS; i++) {
			kmem_print_buckets(bucketsize, i, reserved_bucket);
		}
		printf("\n");
	}
}

kmem_type_info()
{
	int m_type, i;
	for (m_type=M_FREE; m_type<M_LAST; m_type+=COLUMNS) {
		for (i=0; i<=KMEM_TYPE_FIELDS; i++) {
			kmem_print_types(m_type, i);
		}
		printf("\n");
	}
}

dump_prp(prp)
register preg_t *prp;
{
	printf("Pregion: prp = 0x%x\n", prp);
	printf("   p_type = ");
	switch (prp->p_type) {
		case PT_UNUSED:
			printf("PT_UNUSED");
			break;
		case PT_UAREA:
			printf("PT_UAREA");
			break;
		case PT_TEXT:
			printf("PT_TEXT");
			break;
		case PT_DATA:
			printf("PT_DATA");
			break;
		case PT_STACK:
			printf("PT_STACK");
			break;
		case PT_SHMEM:
			printf("PT_SHMEM");
			break;
		case PT_NULLDREF:
			printf("PT_NULLDREF");
			break;
		case PT_LIBTXT:
			printf("PT_LIBTXT");
			break;
		case PT_LIBDAT:
			printf("PT_LIBDAT");
			break;
		case PT_SIGSTACK:
			printf("PT_SIGSTACK");
			break;
		case PT_IO:
			printf("PT_IO");
			break;
		case PT_MMAP:
			printf("PT_MMAP");
			break;
	}
	printf(" p_vaddr = 0x%x p_off = 0x%x\n",
			prp->p_vaddr, prp->p_off);
	printf("   p_count = 0x%x p_vas = 0x%x p_reg = 0x%x\n",
			prp->p_count, prp->p_vas, prp->p_reg);
	printf("   p_flags = 0x%x p_space = 0x%x\n",
			prp->p_flags, prp->p_space);
}

dump_vas(vas)
register vas_t *vas;
{
	register preg_t *prp;

	printf("Pregion list ******************************** :\n");
	prp = vas->va_next;
	while (prp != (preg_t *)vas) {
		dump_prp(prp);
		prp = prp->p_next;
	}
}
#endif /* OSDEBUG */
