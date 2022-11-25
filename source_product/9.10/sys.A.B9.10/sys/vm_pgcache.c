/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_pgcache.c,v $
 * $Revision: 1.7.83.6 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:29:13 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/debug.h"
#include "../h/pfdat.h"
#include "../h/pregion.h"
#include "../h/time.h"
#include "../h/vnode.h"
#include "../h/swap.h"
#include "../h/sysinfo.h"
#include "../h/vmmeter.h"
#include "../h/vmmac.h"
#include "../h/vmparam.h"


/*
 * This file contains all of the routines that handle the pagecache.
 * The pagecache is maintained on a (vnode, data) basis.
 */

#ifdef OSDEBUG
/*
 * pageinhash return true if a given page is in the hash
 * and false if it is not.
 */
pageinhash(pfd)
	register pfd_t *pfd;
{
#ifdef PFDAT32
	VASSERT(pfd_is_locked(pfd));
#else	
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	

	return(pfd->pf_flags&P_HASH);
}
#endif OSDEBUG

/*
 * Insert a page into the page cache.
 * The caller is responsible for makeing sure that no such
 * page is currently in the cache.
 */

pageinsert(pfd, vp, data)
	register pfd_t *pfd;
	register struct vnode *vp;
	u_int data;
{
	register pfd_t *pfd2;

	VASSERT(vm_valulock(pfdat_hash) <= 0);
#ifdef PFDAT32
	VASSERT(pfd_is_locked(pfd));
#else	
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	

	/*
	 * Perform a quick sanity check that the caller has
	 * not created a duplicate entry.
	 */
	pfd2 = phash[PF_HASH(data)&phashmask];
	for(; pfd2 != NULL; pfd2 = pfd2->pf_hchain) {
		if ((pfd2->pf_data == data) && (pfd2->pf_devvp == vp)) {
			panic("pageinsert: dup");
		}
	}

	/*
	 * insert newcomers at head of bucket
	 */
	pfd->pf_hchain = phash[PF_HASH(data)&phashmask];
	phash[PF_HASH(data)&phashmask] = pfd;

	/*	Set up the pfdat.
	 */
	pfd->pf_devvp = vp;
	pfd->pf_data = data;
	pfd->pf_flags |= P_HASH;
}


/*
 * remove page from hash chain
 *	pfd	-> page frame pointer
 * returns:
 *	0	Entry not found.
 *	1	Entry found and removed.
 */
pageremove(pfd)
	register pfd_t *pfd;
{
	register pfd_t **pfd2;
	register int found = 0;
	register u_int data = pfd->pf_data;

	VASSERT(pfd);
#ifdef PFDAT32
	VASSERT(pfd_is_locked(pfd));
#else	
	VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	
	VASSERT(!ON_ISTACK);
	
	hashlock();
	/*
	 * See if it is on the list.
	 */
	pfd2 = &phash[PF_HASH(data)&phashmask];
	for(; *pfd2 != NULL; pfd2 = &((*pfd2)->pf_hchain)) {
		if (*pfd2 == pfd)
			break;
	}

	/*
	 * Disassociate page from disk and
	 * remove from hash table
	 */
	if (*pfd2 == pfd) {
		*pfd2 = pfd->pf_hchain;
		found = 1;
	}
	hashunlock();
	if (found && (pfd->pf_devvp != swapdev_vp))  /* XXX HP_REVISIT XXX */
		VN_RELE(pfd->pf_devvp);
	pfd->pf_data = BLKNULL;
	pfd->pf_hchain = NULL;
	pfd->pf_flags &= ~P_HASH;
	pfd->pf_devvp = 0;
	return(found);
}


/*
 * hash_insert searches for a given page in the hash.
 * If the page is found it will return the page locked.  If the page
 * is not found and pfd is non 0, it will insert pfd in the hash.
 *
 * This routine is an atomic check and set.
 */
pfd_t *
hash_insert(vp, data, pfd)
	register struct vnode *vp;
	u_int data;
	register pfd_t *pfd;
{
	register pfd_t *pfd2;
        register u_int context;

	/*
	 * Inserting a page in the hash requires first 
	 * holding the vnode.
	 */
	/*
	 * HACK to avoid vnode refcnt overruns on swap vnode
	 * for LARGE memory systems.  We are doing this because
	 * 8.0 CRT probably prefers this to our chaning vnode.h
	 * to re-size or move the count field in the vnode.
	 * 
	 * XXX HP_REVISIT XXX
	 */
	if (vp != swapdev_vp)
		VN_HOLD(vp);
retry:
        SPINLOCK_USAV(pfdat_hash, context);
	pfd2 = phash[PF_HASH(data)&phashmask];
	for( ; pfd2 != NULL ; pfd2 = pfd2->pf_hchain) {
		if ((pfd2->pf_data == data) && (pfd2->pf_devvp == vp)) {
			if (pfd2->pf_flags & P_BAD)
				continue;
			if (!cpfdatlock(pfd2)) {
                                SPINUNLOCK_USAV(pfdat_hash, context);
				pfdatlock(pfd2);
				if (!PAGEINHASH(pfd2) ||
				    (pfd2->pf_data != data) ||
				    (pfd2->pf_devvp != vp)) {
					pfdatunlock(pfd2);
					goto retry;
				} 
				if (vp != swapdev_vp) /* XXX HP_REVISIT XXX */
					VN_RELE(vp);
				return(pfd2);
			}
                        SPINUNLOCK_USAV(pfdat_hash, context);
			if (vp != swapdev_vp)
				VN_RELE(vp);	      /* XXX HP_REVISIT XXX */
			return(pfd2);
		}
	}
	if (pfd) {
#ifdef PFDAT32
		VASSERT(pfd_is_locked(pfd));
#else	
		VASSERT(vm_valusema(&pfd->pf_lock) <= 0);
#endif	
		pageinsert(pfd, vp, data);
                SPINUNLOCK_USAV(pfdat_hash, context);
		return(pfd);
	}
        SPINUNLOCK_USAV(pfdat_hash, context);
	if (vp != swapdev_vp)		/* XXX HP_REVISIT XXX */
		VN_RELE(vp);
	return((pfd_t *)0);
}

/*
 * See if a page for (vp, data) is currently in the hash.
 *
 * Since the page is not locked on return no guarantees exist that
 * the page will remain on the hash.
 */
pfd_t *
hash_peek(vp, data)
	register struct vnode *vp;
	register u_int data;
{
	register pfd_t *pfd;
        register u_int context;

        SPINLOCK_USAV(pfdat_hash, context);
	pfd = phash[PF_HASH(data)&phashmask];
	for( ; pfd != NULL ; pfd = pfd->pf_hchain) {
		if ((pfd->pf_data == data) && (pfd->pf_devvp == vp)) {
			if (pfd->pf_flags & P_BAD)
				continue;
                        SPINUNLOCK_USAV(pfdat_hash, context);
			return(pfd);
		}
	}
        SPINUNLOCK_USAV(pfdat_hash, context);
	return((pfd_t *)0);
}

/*
 * Check if the desired page in in the cache.  If it is
 * return it.  If it is on the free list, remove it if
 * we can; otherwise return 0.
 */
pfd_t *
pageincache(vp, data)
	struct vnode *vp;
   	u_int data;
{
   	register pfd_t *pfd;

	/*
	 * Look in page cache
	 */
	if (pfd = hash_insert(vp, data, (pfd_t *)0)) {
		/*
		 * If the page is on the free list 
		 * remove it.  If we can't get the 
		 * memory than just return without
		 * the page a new one will be assigned.
		 */
		if (!PAGEONFREE(pfd)) {
			pfd->pf_use++;
		} else {
			if (removepfn(pfd) == 0)
				return((pfd_t *)0);
			VASSERT(pfd->pf_use == 1);
		}
		return(pfd);
	}
	return((pfd_t*)0);
}

/*
 * This routine attempts to add the specified page to the page cache as
 * the one and only page associated with the specified vp and data.
 */
pfd_t *
addtocache(pfn, vp, data)
	int pfn;
	struct vnode *vp;
	u_int data;
{

	pfd_t *newpfd;
	pfd_t *pfd = &pfdat[pfn];
retry:
	/*
	 * If our page is not the one in the hash then we must
	 * overwrite the one already in the hash.  The procedure is 
	 * complicated because the page we want might be on the freelist
	 * and memory could be so tight that we could not reserve the 
	 * page.  If removepfn (which takes the page off the free list) can
	 * not reserve the memory then we will try to insert our page into
	 * the hash list.  Note: removepfn removes the page passed in from 
	 * the hash list if it can not get memory.
	 */
	newpfd = hash_insert(vp, data, pfd);
	if (newpfd != pfd) {
		if (!PAGEONFREE(newpfd)) {
			newpfd->pf_use++;
		} else {
			if (removepfn(newpfd) == 0)
				goto retry;
		}
	}
	return(newpfd);
}

/*
 * Remove the page associated with this vnode and
 * data from the cache.
 */
removefromcache(vp, data)
	struct vnode *vp;
	daddr_t data;
{
	register pfd_t *pfd;
	
	if ((pfd = hash_insert(vp, (u_int)data, (pfd_t *)0)) != NULL) {
		pageremove(pfd);
		pfdatunlock(pfd);
	}
}	

/*
 * Attempt to purge the pages associated with this vnode from
 * the page cache.  If a page can not be removed don't worry
 * about it.  We make no guarantees that new pages are not
 * inserted behind our search.
 */
mpurge(vp)
	register struct vnode *vp;
{
	int i;
	pfd_t *pfd;
	int total;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */

	/*
	 * Check each hash queue, only holding the hash lock for the 
	 * duration of that queue.
	 * Remember phashmask is 1 less than the size of the hash table!
	 */
	for (i = 0; i < (phashmask+1); i++) {
		hashlock();
		/*
		 * Compute total number of potential page with this
		 * vnode on this chain.
		 */
		total = 0;
		for (pfd = phash[i]; pfd != NULL; pfd = pfd->pf_hchain) {
			if (pfd->pf_devvp == vp)
				total++;
		}
retry:
		/* 
		 * Walk the chain removing pages until total is 0
		 * or we don't find any more.
		 */
		for (pfd = phash[i]; pfd != NULL; pfd = pfd->pf_hchain) {
			if (pfd->pf_devvp == vp) {
				/*
				 * Exceeded original amount, vnode must still
				 * be active.
				 */
				if (--total < 0) {
					hashunlock();
					vmemp_unlockx();	
					return;
				}

				/*
				 * Remove the page.
				 */
				if (cpfdatlock(pfd)) {
					VASSERT(pfd->pf_devvp == vp);
					hashunlock();
					pageremove(pfd);
					pfdatunlock(pfd);
					hashlock();
					goto retry;
				}
			}
		}
		hashunlock();
	}
	vmemp_unlockx();	/* free up VM empire */
}

/*
 * Attempt to purge the pages associated with this file sys from
 * the page cache.  If a page can not be removed don't worry
 * about it.  We make no guarantees that new pages are not
 * inserted behind our search.
 */
mpurgevfs(vfsp)
	register struct vfs *vfsp;
{
	int i;
	pfd_t *pfd;
	int total;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */

	/*
	 * Check each hash queue, only holding the hash lock for the 
	 * duration of that queue.
	 * Remember phashmask is 1 less than the size of the hash table!
	 */
	for (i = 0; i < (phashmask+1); i++) {
		hashlock();
		/*
		 * Compute total number of potential page with this
		 * vnode on this chain.
		 */
		total = 0;
		for (pfd = phash[i]; pfd != NULL; pfd = pfd->pf_hchain) {
			if (pfd->pf_devvp->v_vfsp == vfsp)
				total++;
		}
retry:
		/* 
		 * Walk the chain removing pages until total is 0
		 * or we don't find any more.
		 */
		for (pfd = phash[i]; pfd != NULL; pfd = pfd->pf_hchain) {
			if (pfd->pf_devvp->v_vfsp == vfsp) {
				/*
				 * Exceeded original amount, vnode must still
				 * be active.
				 */
				if (--total < 0) {
					hashunlock();
					vmemp_unlockx();	
					return;
				}

				/*
				 * Remove the page.
				 */
				if (cpfdatlock(pfd)) {
					VASSERT(pfd->pf_devvp->v_vfsp == vfsp);
					hashunlock();
					pageremove(pfd);
					pfdatunlock(pfd);
					hashlock();
					goto retry;
				}
			}
		}
		hashunlock();
	}
	vmemp_unlockx();	/* free up VM empire */
}

/*
 * Interface routines for the buffer cache and page cache.
 * These routine are not needed with true MMFs.
 */

#ifdef notdef
/*
 * Look for block bn of device dev in the free pool.
 *	dev	-> device of our interest
 *	data	-> block of our interest
 * returns:
 *	0	-> can't find it
 *	pfd	-> ptr to pfdat entry
 * NOT NEEDED WITH FULL MMF's XXX.
 */

pfd_t *
mfind(vp, bn)
	struct vnode *vp;
	daddr_t	bn;
{
	pfd_t	*ret;
	vm_sema_state;		/* semaphore save state */

	vmemp_lockx();		/* lock down VM empire */
	ret = hash_peek(vp, (u_int)bn);
	vmemp_unlockx();	/* free up VM empire */
	return(ret);
}
#endif /* notdef */

/*
 * Pull the clist entry of <dev,bn> off the hash chains.
 * NOT NEEDED WITH FULL MMF's XXX.
 */
munhash(vp, bn)
	struct vnode *vp;
	daddr_t bn;
{
	register pfd_t *pfd;
	vm_sema_state;		/* semaphore save state */
	
	vmemp_lockx();		/* lock down VM empire */
	if ((pfd = hash_insert(vp, (u_int)bn, (pfd_t *)0)) != NULL) {
		pageremove(pfd);
		pfdatunlock(pfd);
	}
	vmemp_unlockx();	/* free up VM empire */
}
