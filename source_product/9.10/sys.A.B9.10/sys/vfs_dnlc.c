/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_dnlc.c,v $
 * $Revision: 1.13.83.5 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/17 19:51:56 $
 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

/*      @(#)vfs_dnlc.c 1.1 86/02/03 Copyr 1985 Sun Micro     */
/*      NFSSRC @(#)vfs_dnlc.c	2.2 86/05/14 */

/*
 * Copyright (c) 1985 by Sun Microsystems.
 */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../h/vnode.h"
#include "../h/dnlc.h"
#include "../rpc/types.h"
#include "../nfs/nfs.h"
#include "../nfs/rnode.h"
#include "../ufs/inode.h"
#include "../h/kern_sem.h"
extern lock_t *dnlc_lock;

/*
 * Directory name lookup cache.
 * Based on code originally done by Robert Els at Melbourne.
 *
 * Names found by directory scans are retained in a cache
 * for future referene.  It is managed LRU, so frequently
 * used names will hang around.  Cache is indexed by hash value
 * obtained from (vp, name) where the vp refers to the
 * directory containing the name.
 *
 * For simplicity (and economy of storage), names longer than
 * some (small) maximum length are not cached, they occur
 * infrequently in any case, and are almost never of interest.
 */

 /*
  *	MP design notes -
  *	Invariant: when "dnlc_lock" is available, every node is
  *	linked into its proper place in the LRU queue, and any
  *	valid entry is linked into the correct hash chain. Any
  *	entry which is not null-hashed has both vp and dvp fields
  *	set and VN_HOLDs have been already done. If the cred field
  *	is not to be NOCRED, it is set and a crhold has already
  *	been done.
  *
  *	VN_HOLD and crhold grab spinlocks, and VN_RELE
  *	can sleep, so we rearrange code so that none of these
  *	are called from inside the spinlocked area, while preserving
  *	the above invariant.
  */

 /*
  * The size of the name cache is set here to the number of inodes in
  * the default system.  This should *not* be hardcoded, it should be
  * redone so that it's an option which may be changed via uxgen - ghs
  */
#define NC_SIZE			356	/* size of name cache */

#define	NC_HASH(namep, namlen, vp)	\
	((namep[0] + namep[namlen-1] + namlen + (int) vp) & (NC_HASH_SIZE-1))

/*
 * Macros to insert, remove cache entries from hash, LRU lists.
 */
#define	INS_HASH(ncp,nch)	insque(ncp, nch)
#define	RM_HASH(ncp)		remque(ncp)

#define	INS_LRU(ncp1,ncp2)	insque2((struct ncache *) ncp1, (struct ncache *) ncp2)
#define	RM_LRU(ncp)		remque2((struct ncache *) ncp)

#define	NULL_HASH(ncp)		(ncp)->hash_next = (ncp)->hash_prev = (ncp)

/*
 * Hash list of name cache entries for fast lookup.
 */
struct nc_hash nc_hash[NC_HASH_SIZE];

/*
 * LRU list of cache entries for aging.
 */
struct nc_lru nc_lru;

struct	ncstats ncstats;		/* cache effectiveness statistics */

static struct ncache *dnlc_search();
int	doingcache = 1;

extern int ncsize;
extern struct ncache ncache[];

/*
 * Initialize the directory cache.
 * Put all the entries on the LRU chain and clear out the hash links.
 */
dnlc_init()
{
	register struct ncache *ncp;
	register int i;

	nc_lru.lru_next = (struct ncache *) &nc_lru;
	nc_lru.lru_prev = (struct ncache *) &nc_lru;
	for (i = 0; i < ncsize; i++) {
		ncp = &ncache[i];
		INS_LRU(ncp, &nc_lru);
		NULL_HASH(ncp);
		ncp->dp = ncp->vp = (struct vnode *) 0;
		ncp->cred = NOCRED;
	}
	for (i = 0; i < NC_HASH_SIZE; i++) {
		ncp = (struct ncache *) &nc_hash[i];
		NULL_HASH(ncp);
	}
}

/*
 * Add a name to the directory cache.
 */
dnlc_enter(dp, name, vp, cred)
	register struct vnode *dp;
	register char *name;
	struct vnode *vp;
	struct ucred *cred;
{
	register int namlen;
	register struct ncache *ncp;
	register int hash;
	int release_cred;
	struct vnode *odvp = (struct vnode *)0;
	struct vnode *ovp = (struct vnode *)0;
	struct ucred *ocredp = NOCRED;

	if (!doingcache) {
		return;
	}
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		ncstats.long_enter++;
		return;
	}

	/*
	 * The cr_ref field is only a signed short in the 300/9.0 release.
	 * This means that it can be overflowed by having a large dnlc and
	 * accessing lots of files across NFS.  To avoid overflowing the
	 * field, we will not perform the dnlc_enter if the count is
	 * already half as large as it can get.  --jcm 11/08/93
	 * 	actually added by cwb
	 *
	 * P. S.  This is not an issue on 800/9.0 or in 10.0 because the
	 *        cr_ref field is 32 bits wide in those releases.
	 *
	 */
	if ((cred != NOCRED) && (cred->cr_ref >= 0x4000)) return;

	hash = NC_HASH(name, namlen, dp);

	/*
	 *	MP reordering - grab now so they won't disappear
	 */
	VN_HOLD(dp);
	VN_HOLD(vp);

	/* Count number of times in name cache, for debug only. */
	DNLC_INCR(dp);
	DNLC_INCR(vp);

	if (cred != NOCRED){
		crhold(cred);
	}

	SPINLOCK(dnlc_lock);
	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp != (struct ncache *) 0) {
		goto backout;
	}

	/*
	 * Take least recently used cache struct.
	 */
	ncp = nc_lru.lru_next;
	if (ncp == (struct ncache *) &nc_lru) {	/* LRU queue empty */
		ncstats.lru_empty++;
		goto backout;
	}
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	RM_HASH(ncp);
	release_cred = 0;
	if (ncp->dp != (struct vnode *) 0) {

 	/*
         * PHKL_2836 
         *
         * Split rnode entry r_cred into r_rcred and r_wcred to
         * avoid EACCES errors in the delayed buffer write case.
         *
         */
		if (ncp->cred != NULL && ncp->dp->v_fstype == VNFS
		    && ncp->cred != vtor(ncp->dp)->r_rcred) {
			release_cred = 1;
		}
		if (ncp->cred != NULL && ncp->dp->v_fstype == VNFS
		    && ncp->cred != vtor(ncp->dp)->r_wcred) {
			release_cred = 1;
		}
		odvp = ncp->dp;
	}
	
	if (ncp->vp != (struct vnode *) 0) {

 	/*
         * PHKL_2836 
         *
         * Split rnode entry r_cred into r_rcred and r_wcred to
         * avoid EACCES errors in the delayed buffer write case.
         *
         */
		if (ncp->cred != NULL && ncp->vp->v_fstype == VNFS
		    && ncp->cred != vtor(ncp->vp)->r_rcred) {
			release_cred = 1;
		}
		if (ncp->cred != NULL && ncp->vp->v_fstype == VNFS
		    && ncp->cred != vtor(ncp->vp)->r_wcred) {
			release_cred = 1;
		}
		ovp = ncp->vp;
	}
	ocredp = ncp->cred;

	ncp->dp = dp;
	ncp->vp = vp;
	ncp->namlen = namlen;
	bcopy(name, ncp->name, (unsigned)namlen);
	ncp->cred = cred;

	/*
	 * Insert in LRU, hash chains.
	 */
	INS_LRU(ncp, nc_lru.lru_prev);
	INS_HASH(ncp, &nc_hash[hash]);
	SPINUNLOCK(dnlc_lock);

	/*
	 * Drop old vnodes and credentials (if we had any).
	 */
	if (release_cred) {
		crfree(ocredp);
	}
	if (odvp) {
		DNLC_DECR(odvp);
		VN_RELE(odvp);
	}
	if (ovp) {
		DNLC_DECR(ovp);
		VN_RELE(ovp);
	}
	return;
backout:
	SPINUNLOCK(dnlc_lock);
	/*
	 *	We won't be using these guys after all
	 */
	DNLC_DECR(dp);
	VN_RELE(dp);
	DNLC_DECR(vp);
	VN_RELE(vp);
	if (cred != NOCRED){
		crfree(cred);
	}

}

/*
 * Look up a name in the directory name cache.
 */
struct vnode *
dnlc_lookup(dp, name, cred)
	struct vnode *dp;
	register char *name;
	struct ucred *cred;
{
	register int namlen;
	register int hash;
	register struct ncache *ncp;
	register struct ncache *prev;

	if (!doingcache) {
		return ((struct vnode *) 0);
	}
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		ncstats.long_look++;
		return ((struct vnode *) 0);
	}
	hash = NC_HASH(name, namlen, dp);
	SPINLOCK(dnlc_lock);
	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp == (struct ncache *) 0) {
		ncstats.misses++;
		SPINUNLOCK(dnlc_lock);
		return ((struct vnode *) 0);
	}
	ncstats.hits++;
	/*
	 * Move this slot to the end of LRU
	 * chain.
	 */
	RM_LRU(ncp);
	INS_LRU(ncp, nc_lru.lru_prev);
	/*
	 * If not at the head of the hash chain,
	 * move forward so will be found
	 * earlier if looked up again.
	 */
	if (ncp->hash_prev != (struct ncache *) &nc_hash[hash]) {
		/* don't assume that remque() preserves links! */
		prev = ncp->hash_prev->hash_prev;
		RM_HASH(ncp);
		INS_HASH(ncp, prev);
	}
	SPINUNLOCK(dnlc_lock);
	return (ncp->vp);
}

/*
 * Remove an entry in the directory name cache.
 */
dnlc_remove(dp, name)
	struct vnode *dp;
	register char *name;
{
	register int namlen;
	register struct ncache *ncp;
	int hash;

	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		return;
	}
	hash = NC_HASH(name, namlen, dp);
	SPINLOCK(dnlc_lock);
	while (ncp = dnlc_search(dp, name, namlen, hash, ANYCRED)) {
		dnlc_rm(ncp);
	}
	SPINUNLOCK(dnlc_lock);

}

/*
 * Purge the entire cache.
 */
dnlc_purge()
{
	register struct nc_hash *nch;
	register struct ncache *ncp;

	ncstats.purges++;
	SPINLOCK(dnlc_lock);
start:
	for (nch = nc_hash; nch < &nc_hash[NC_HASH_SIZE]; nch++) {
		ncp = nch->hash_next;
		while (ncp != (struct ncache *) nch) {
			if (ncp->dp == 0 || ncp->vp == 0) {
				panic("dnlc_purge: zero vp");
			}
			dnlc_rm(ncp);
			goto start;
		}
	}
	SPINUNLOCK(dnlc_lock);
}

/*
 * Purge any cache entries referencing a vnode.
 */
dnlc_purge_vp(vp)
	register struct vnode *vp;
{
	register int moretodo;
	register struct ncache *ncp;

	SPINLOCK(dnlc_lock);
	do {
		moretodo = 0;
		for (ncp = nc_lru.lru_next; ncp != (struct ncache *) &nc_lru;
		    ncp = ncp->lru_next) {
			if (ncp->dp == vp || ncp->vp == vp) {
				dnlc_rm(ncp);
				moretodo = 1;
				break;
			}
		}
	} while (moretodo);
	SPINUNLOCK(dnlc_lock);
}

/*
 * Purge any cache entry.
 * Called by iget when inode freelist is empty.
 */
dnlc_purge1()
{
	register struct ncache *ncp;

	SPINLOCK(dnlc_lock);
	for (ncp = nc_lru.lru_next; ncp != (struct ncache *) &nc_lru;
	    ncp = ncp->lru_next) {
		if (ncp->dp) {
			dnlc_rm(ncp);
			SPINUNLOCK(dnlc_lock);
			return (1);
		}
	}
	SPINUNLOCK(dnlc_lock);
	return (0);
}

/*
 * Obliterate a cache entry.
 */
static
dnlc_rm(ncp)
	register struct ncache *ncp;
{
	struct vnode *dp = ncp->dp;
	struct vnode *vp = ncp->vp;
	struct ucred *credp = ncp->cred;

	ncp->dp = (struct vnode *) 0;
	ncp->vp = (struct vnode *) 0;
	ncp->cred = NOCRED;
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	RM_HASH(ncp);
	/*
	 * Insert at head of LRU list (first to grab).
	 */
	INS_LRU(ncp, &nc_lru);
	/*
	 * And make a dummy hash chain.
	 */
	NULL_HASH(ncp);
	SPINUNLOCK(dnlc_lock);	/* so we can sleep in VN_RELE */
	/*
	 * Release ref on vnodes.
	 */
	DNLC_DECR(dp);
	VN_RELE(dp);
	DNLC_DECR(vp);
	VN_RELE(vp);
	if (credp != NOCRED) {
		crfree(credp);
	}
	SPINLOCK(dnlc_lock);
}


#ifdef  AUTOCHANGER
/*
 * Purge any cache entries referencing a dev.
 */
dnlc_purge_dev(dev)
	dev_t dev;
{
	register int moretodo;
	register struct ncache *ncp;

	SPINLOCK(dnlc_lock);
	do {
		moretodo = 0;
		for (ncp = nc_lru.lru_next; ncp != (struct ncache *) &nc_lru;
		    ncp = ncp->lru_next) {
			if ((ncp->vp) && (ncp->vp->v_fstype ==  VUFS) && 
			    (VTOI(ncp->vp)->i_dev == dev)) {
				dnlc_rm(ncp);
				moretodo = 1;
				break;
			}
		}
	} while (moretodo);
	SPINUNLOCK(dnlc_lock);
}

#endif

/*
 * Utility routine to search for a cache entry.
 *	MP - must be called with dnlc_lock held
 */
static struct ncache *
dnlc_search(dp, name, namlen, hash, cred)
	register struct vnode *dp;
	register char *name;
	register int namlen;
	int hash;
	struct ucred *cred;
{
	register struct nc_hash *nhp;
	register struct ncache *ncp;

	nhp = &nc_hash[hash];
	for (ncp = nhp->hash_next; ncp != (struct ncache *) nhp;
	    ncp = ncp->hash_next) {
		if (ncp->dp == dp && ncp->namlen == namlen &&
		    (cred == ANYCRED || ncp->cred == cred) &&
		    *ncp->name == *name &&	/* fast chk 1st chr */
		    bcmp(ncp->name, name, namlen) == 0) {
			return (ncp);
		}
	}
	return ((struct ncache *) 0);
}

/*
 * Insert into queue, where the queue pointers are
 * in the second two longwords.
 * Should be in assembler like insque.
 */
static
insque2(ncp2, ncp1)
	register struct ncache *ncp2, *ncp1;
{
	register struct ncache *ncp3;

	ncp3 = ncp1->lru_next;
	ncp1->lru_next = ncp2;
	ncp2->lru_next = ncp3;
	ncp3->lru_prev = ncp2;
	ncp2->lru_prev = ncp1;
}

/*
 * Remove from queue, like insque2.
 */
static
remque2(ncp)
	register struct ncache *ncp;
{
	ncp->lru_prev->lru_next = ncp->lru_next;
	ncp->lru_next->lru_prev = ncp->lru_prev;
}

