/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sys_plock.c,v $
 * $Revision: 1.33.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:12:29 $
 */


/*	sys_plock.c	1.0	84/05/18	*/


#include "../h/types.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/sema.h"
#include "../h/proc.h"
#include "../ufs/fs.h"
#include "../h/uio.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/lock.h"
#include "../h/vmmac.h"
#include "../h/mbuf.h"
#include "../h/signal.h"
#include "../h/vas.h"
#include "../h/pfdat.h"
#include "../h/debug.h"

extern int nproc; /* defined in param.c */


 /* 
  * Lock() locks memory for user processes.  We use the name
  *   lock instead of plock, because plock is already taken
  *   for a pipe locking routine.
  * The process may have to sleep while memory
  *  is being locked or brought in from disc.
  * The process has to have appropriate privileges
  *   (PRIV_MLOCK).
  */
lock()
{
    register struct a {
	int     operation;
    }  *uap = (struct a    *) u.u_ap;

    register preg_t *prp;
    register struct proc *p;
    register long 
	    op,
	    locking,
	    dotext,
	    dodata,
	    dostack,
	    didtext,
	    diddata,
	    didstack;

/* Test for privlege */
    if(!( in_privgrp(PRIV_MLOCK, (struct ucred *)0) || suser() )){
	u.u_error = EPERM;
	return ;
    };

    p = u.u_procp;
    op = uap -> operation;

 /*
  *  Input error testing
  */

    locking = dotext = dodata = dostack = 0;

    switch(op) {

	case PROCLOCK:
		/* if anything is locked, cannot lock again */

		if ((prp = findpregtype(p->p_vas, PT_TEXT)) != NULL) {
			if (prp->p_flags & PF_MLOCK) {
				goto bad;	/* already locked */
			}
			dotext = 1;
		}
		if ((prp = findpregtype(p->p_vas, PT_DATA)) != NULL) {
			if (prp->p_flags & PF_MLOCK) {
				goto bad;	/* already locked */
			}
			dodata = 1;
		}
		if ((prp = findpregtype(p->p_vas, PT_STACK)) != NULL) {
			if (prp->p_flags & PF_MLOCK) {
				goto bad;	/* already locked */
			}
			dostack = 1;
		}
		if (!(dotext || dostack || dodata)) {
			/* nothing to lock */
			goto bad;
		}
		locking = 1;
		break;

	case TXTLOCK:
		/* No text or already locked means error */
		/* Impure text will show up as data region XXX Andy */

		if (((prp = findpregtype(p->p_vas, PT_TEXT)) == NULL) ||
		    (prp->p_flags & PF_MLOCK)) {
			goto bad;
		}
		locking = dotext = 1;
		break;

	case DATLOCK:
		/* No data/stack or already locked means error */

		if ((prp = findpregtype(p->p_vas, PT_DATA)) != NULL) {
			if (prp->p_flags & PF_MLOCK) {
				goto bad;	/* already locked */
			}
			dodata = 1;
		}
		if ((prp = findpregtype(p->p_vas, PT_STACK)) != NULL) {
			if (prp->p_flags & PF_MLOCK) {
				goto bad;	/* already locked */
			}
			dostack = 1;
		}
		if (!(dostack || dodata)) {
			/* nothing to lock */
			goto bad;
		}
		locking = 1;
		break;

	case UNLOCK:
		/* find out what exists that is locked */

		if (((prp = findpregtype(p->p_vas, PT_TEXT)) != NULL) &&
		    (prp->p_flags & PF_MLOCK)) {
			dotext = 1;
		}
		if (((prp = findpregtype(p->p_vas, PT_DATA)) != NULL) &&
		    (prp->p_flags & PF_MLOCK)) {
			dodata = 1;
		}
		if (((prp = findpregtype(p->p_vas, PT_STACK)) != NULL) &&
		    (prp->p_flags & PF_MLOCK)) {
			dostack = 1;
		}

		/* If nothing is locked already, this is an error */

		if (!(dotext || dostack || dodata)) {
			goto bad;	/* nothing to unlock */
		}
		locking = 0;
		break;

	default:
bad:
		u.u_error = EINVAL;
		return;
    }

 /* 
  *  Do Lock / Unlock
  */

    if ( locking ) {

    /* 
     *   Now lock each part (text, data, stack) of memory in turn.
     *   If any part fails, backout the other parts.
     */

	didtext = diddata = didstack = 0;

	if ((dotext  &&((didtext=mlockpregtype(p->p_vas, PT_TEXT)) == 0)) ||
	    (dodata  &&((diddata=mlockpregtype(p->p_vas, PT_DATA)) == 0)) ||
	    (dostack &&((didstack=mlockpregtype(p->p_vas, PT_STACK)) == 0))) {

		/* one lock failed; backout other successful locks */

		if (didtext)  
			munlockpregtype(p->p_vas, PT_TEXT);

		if (diddata)  
			munlockpregtype(p->p_vas, PT_DATA);

		if (didstack)
			munlockpregtype(p->p_vas, PT_STACK);

		u.u_error = ENOMEM;
		return;
	}

    } else {

	/* Unlock each part of memory to be unlocked */	

	if (dotext)  
		munlockpregtype(p->p_vas, PT_TEXT);

	if (dodata)  
		munlockpregtype(p->p_vas, PT_DATA);

	if (dostack)
		munlockpregtype(p->p_vas, PT_STACK);
    }
}


#pragma	OPT_LEVEL 	1
int lockablemem_cnt = 0;
int lockablemem_stolen = 0;

/*
 * Take size bytes from lockable_mem.  If this is more than is available
 * deal with that.
 */

steal_lockmem(size)
register unsigned size;
{
	int over_usage=0;

	VASSERT(lockable_mem >= 0);

	pfdatlstlock();

	lockable_mem -= size;

	if (lockable_mem < 0) {
		over_usage = -lockable_mem;
		lockablemem_stolen += over_usage;
		lockable_mem = 0;
	}

	VASSERT(lockable_mem >= 0);

	pfdatlstunlock(); 

	return(1);
}

/*
 * Conditionally reserve size memory pages of memory lockable pages.
 * Does not block waiting for memory lockable pages to become available.
 * Return values:
 *	0 - Insufficient lockable memory available.
 *	1 - Lockable memory available immediately.
 */

int
clockmemreserve(size)
register unsigned size;
{
	VASSERT(lockable_mem >= 0);

	/*
	 * since the memory can be considered "swap" space,
	 * let the swapper know that some memory is being used.
	 */
	if (!steal_swap((reg_t *)0, (int)size, 1))
                return(0);

	pfdatlstlock();
	if (lockable_mem >= size) {
		lockable_mem -= size;
		pfdatlstunlock();
		return(1);
	}
	pfdatlstunlock();
	return_swap((int)size);
	return(0);
}

/*
 * Reserve size lockable memory pages.
 * Blocks waiting for lockable memory to become available, if necessary.
 * Return values:
 *	0 - Lockable memory available immediately
 *	1 - Had to sleep to get lockable memory
 */
int
lockmemreserve(size)
   register unsigned size;
{
	int have_slept = 0;
	unsigned origsize;
	unsigned lock_state;

	VASSERT(lockable_mem >= 0);

	/*
         * since the memory can be considered "swap" space,
         * let the swapper know that some memory is being used.
         */
	if (!steal_swap((reg_t *)0, (int)size, 1)) {

		/* steal_swap() with no_wait clear */
		have_slept = 1;

		if (!steal_swap((reg_t *)0, (int)size, 0))
			return(-1);
	}

	/* in-line expansion of clockmemreserve(): */

	pfdatlstlock();
	if (lockable_mem >= size) {
		lockable_mem -= size;
		pfdatlstunlock();
		if (have_slept)
			return(1);
		else
			return(0);
	}

	origsize = size;
	while (lockable_mem < size) {

		/* take what we can get so this swtch was not wasted */
		VASSERT(lockable_mem >= 0);
		size -= lockable_mem;
		lockable_mem = 0;

		pfdatlstunlock();

		lock_state = sleep_lock();
		/* State that someone intends to sleep */
		lockablemem_cnt++;

		if (sleep_then_unlock((caddr_t)&lockable_mem, PSLEP|PCATCH,
							lock_state) == 1) {
			/* back out amount locked so far */
			pfdatlstlock();
			lockable_mem += origsize - size;
			pfdatlstunlock();

			/* back out stolen swap space */
			return_swap((int)origsize);

			return(-1);
		}
		pfdatlstlock();
	}
	lockable_mem -= size;
	VASSERT(lockable_mem >= 0);
	pfdatlstunlock();
	return(1);
}

/*
 * Unreserve size lockable memory pages.
 * Wakeup any processes waiting for lockable memory.
 */
lockmemunreserve(size)
register unsigned size;
{
	unsigned lock_state;

	return_swap((int)size);

	pfdatlstlock();
	if (lockablemem_stolen) {
		lockablemem_stolen -= size;
		if (lockablemem_stolen < 0) {
			lockable_mem += -lockablemem_stolen;
			lockablemem_stolen = 0;
		}
	} else 
		lockable_mem += size;

	VASSERT(lockable_mem <= total_lockable_mem);
	VASSERT(lockable_mem >=0);

	if (lockable_mem) {
		/* if anyone is waiting on lockable memory then wake him up */
		lock_state = sleep_lock();
		if (lockablemem_cnt) {
			lockablemem_cnt = 0;
			wakeup((caddr_t)&lockable_mem);
		}
		sleep_unlock(lock_state);
	}
	pfdatlstunlock();
}


/*	
 * Check if process p has a pregion memory locked.
 *
 * This routine is desgined to be used by sched.  It ignores the
 * fact that the U area is always locked in memory.
 *
 * Return 1 if process has memory locked regions or vas is locked, 0 otherwise.
 * This routine can't sleep, so we have to use cvaslock() rather than
 * vaslock().
 */
mlockedpreg(p)
	register struct proc *p;
{
	register preg_t *prp;

	/*	Walk through process pregions, check lock flag
	 */
	if (! cvaslock(p->p_vas))
		return(1);
	for (prp = p->p_vas->va_next; prp != (preg_t *)p->p_vas; 
	     prp = prp->p_next) {
		if (prp->p_flags & PF_MLOCK) {
			vasunlock(p->p_vas);
			return(1);
		}
	}
	vasunlock(p->p_vas);
	return(0);
}


/*
 *	Return the total number of pages a process has memory locked.
 */
uint
numlockedpages(p)
	register struct proc *p;
{
	register preg_t *prp;
	register reg_t  *rp;
	register int	numpages = 0;

	/*	Walk through process regions, sum the size
	 *	of regions which are memory locked by this process.
	 *	A region is memory locked by this process if the
	 *	pregion is memory locked.
	 */
	vaslock(p->p_vas);
	for (prp = p->p_vas->va_next; prp != (preg_t *)p->p_vas; 
	     prp = prp->p_next) {
		if (prp->p_flags & PF_MLOCK) {
			rp = prp->p_reg;
			reglock(rp);
			VASSERT(rp->r_mlockcnt != 0);
			/* calculate how many pages are locked */
			numpages += prp->p_off+prp->p_count-get_startidx(prp);
			regrele(rp);
		}
	}
	vasunlock(p->p_vas);
	return(numpages);
}


/* 
 * Called when you are about to expand memory
 * and want to know if it is OK from the point of view
 * of maximum lockable memory & the locked memory
 * currently in use by the process.
 */
int
chkmaxlockmem(change)
	int change;
{
	register struct proc *p = u.u_procp;

	/* If there can never be enough lockable memory, a signal is
	 * sent to the process and failure is returned.
	 */
	if (numlockedpages(p) + change > total_lockable_mem) {
		return(0);
	}
	return(1);
}
