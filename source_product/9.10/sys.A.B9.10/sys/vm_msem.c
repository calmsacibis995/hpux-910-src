/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_msem.c,v $
 * $Revision: 1.2.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:17:52 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/mman.h"
#include "../h/vas.h"
#include "../h/buf.h"
#include "../h/malloc.h"

unsigned long msem_global_lock_id = 1;
struct msem_procinfo *msem_proc_list = (struct msem_procinfo *)0;
struct msem_info *msem_hlist[MSEM_HASH_SIZE];

/* Forward Declarations */

void msem_release();
void msem_locklist_remove();
struct msem_info *msem_find();
struct msem_procinfo *msem_pfind();
unsigned long msem_proc_init();

/*
 * Note that this msem_init() does not return a msemaphore pointer,
 * but we declare it this way, so it will not conflict with the
 * declaration for the user land stub in mman.h
 */

#ifndef LINT
msemaphore *
#endif
msem_init()
{
    register struct a {
	caddr_t    msem;
	int        initial_value;
    } *uap = (struct a *)u.u_ap;
    msemaphore sem_cp;

    /*
     * Verify that a valid initial_value was specified.
     */

    if (   uap->initial_value != MSEM_LOCKED
	&& uap->initial_value != MSEM_UNLOCKED) {

	u.u_error = EINVAL;
	return;
    }

    /*
     * Ensure that the semaphore is 16 byte aligned. This is necessary
     * to guarantee that the semaphore does not cross a page boundary.
     */

    if (((int)uap->msem & 0x0f) != 0) {
	u.u_error = EFAULT;
	return;
    }

    /*
     * Use msem_getaddr to check to make sure the semaphore exists in
     * a memory mapped region.
     */

    if (msem_getaddr(uap->msem,(reg_t *)0,(unsigned long *)0) == -1) {
	u.u_error = EINVAL;
	return;
    }

    /* Verify that the address is read/write */

    if (!useracc(uap->msem, sizeof(msemaphore), B_WRITE)) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * If this proc has not already allocated its msem_procinfo
     * structure, do so now.
     */

    if (u.u_procp->p_msem_info == (struct msem_procinfo *)0)
	(void)msem_proc_init();

    /* Create semaphore image */

    sem_cp.magic = __MSEM_MAGIC;
    if (uap->initial_value == MSEM_LOCKED)
	sem_cp.locker = u.u_procp->p_msem_info->lockid;
    else
	sem_cp.locker = 0;

    sem_cp.wanted = 0;

#ifdef __hp9000s800
    sem_cp.mcas_lock = 1;
#else
    sem_cp.spare = 0;
#endif

    /* Copy semaphore image out to semaphore address */

    u.u_error = copyout((caddr_t)&sem_cp, uap->msem, sizeof(msemaphore));
    if (u.u_error == 0)
	u.u_rval1 = (int)uap->msem;

    return;
}

msem_remove()
{
    register struct a {
	caddr_t msem;
    } *uap = (struct a *)u.u_ap;
    register struct msem_info *msem_iptr;
    reg_t *rp;
    unsigned long msem_offset;
    msemaphore sem_cp;

    /*
     * Get the region and region offset for this semaphore. msem_getaddr()
     * also makes sure that the address resides within a memory mapped
     * pregion.
     */

    if (msem_getaddr(uap->msem,&rp,&msem_offset) == -1) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Read the semaphore contents to make sure that it is a valid
     * semaphore (by checking the magic number).
     */

    u.u_error = copyin(uap->msem,(caddr_t)&sem_cp,sizeof(msemaphore));
    if (u.u_error != 0)
	return;

    if (sem_cp.magic != __MSEM_MAGIC) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Blow the semaphore away. We don't need any of the information
     * in the semaphore, since, if anyone is waiting on it, we have
     * the information we need stored in the kernel.
     */

    bzero((caddr_t)&sem_cp, sizeof(msemaphore));
    u.u_error = copyout((caddr_t)&sem_cp, uap->msem, sizeof(msemaphore));
    if (u.u_error != 0)
	return;

    /*
     * Now there should be no more race conditions, since we won't
     * be sleeping/faulting from here on.
     */

    msem_iptr = msem_find(rp, msem_offset);
    if (msem_iptr == (struct msem_info *)0) {

	/* There is no one waiting on this semaphore, so just return */

	return;
    }

    /*
     * Wakeup the waiters. The last waiter will free everything up
     * in msleep.
     */

    wakeup(msem_iptr);
    return;
}

msleep()
{
    register struct a {
	caddr_t msem;
	int condition;
    } *uap = (struct a *)u.u_ap;
    register struct msem_info *msem_iptr;
    register struct msem_procinfo *msem_pinfo; /* This process */
    register struct msem_procinfo *tmp_pinfo;
    struct msem_procinfo *lock_pinfo;          /* process holding lock */
    struct msem_info *msem_prev_iptr;
    int hash = -1;
    reg_t *rp;
    unsigned long msem_offset;
    msemaphore sem_cp;
    int waiting;

    /*
     * If this proc has not already allocated its msem_procinfo
     * structure, do so now.
     */

    msem_pinfo = u.u_procp->p_msem_info;
    if (msem_pinfo == (struct msem_procinfo *)0) {
	(void)msem_proc_init();
	msem_pinfo = u.u_procp->p_msem_info;
    }

    /*
     * We use the waiting flag to indicate whether or not
     * we have set msem_waitptr for this proc yet. We start
     * out false (0), and set it to true (1) when we sleep. Once
     * we start "waiting", we don't stop when we get woken up
     * from the sleep, unless we were interrupted. We don't stop
     * "waiting" until we actually get the semaphore. This is done
     * to prevent the following scenario:
     *
     *      1) We sleep on the semaphore, and then get woken up
     *         due to the locking process releasing the semaphore.
     *      2) Some other process beats us to getting the semaphore
     *      3) The process that got the semaphore then tries to
     *         get a semaphore that we own.
     *
     * If we clear msem_waitptr, then there will be a window where the
     * other process will be able to wait on the lock that we own, In
     * which case we will have to return EDEADLK when we try to wait for
     * the semaphore again.  If we don't clear msem_waitptr, the other
     * process will get the EDEADLK, which is more appropriate (i.e. it
     * gets the EDEADLK immediately, rather than us returning EDEADLK
     * after having slept for an unknown amount of time).
     *
     * Also, if waiting is true (1), we don't have to redo the
     * deadlock check.
     */

    waiting = 0;

    /*
     * Get the region and region offset for this semaphore. msem_getaddr()
     * also makes sure that the address resides within a memory mapped
     * pregion.
     */

    if (msem_getaddr(uap->msem,&rp,&msem_offset) == -1) {
	u.u_error = EINVAL;
	return;
    }

    /*
     * Check to see if this semaphore already has waiters. If it
     * does not, we need to allocate memory up front, because we
     * can't afford to sleep for it later on.
     */

    msem_iptr = msem_find(rp, msem_offset);
    if (msem_iptr == (struct msem_info *)0) {
	MALLOC(msem_iptr, struct msem_info *,
	       sizeof(struct msem_info), M_DYNAMIC, M_WAITOK);

	if (msem_iptr == (struct msem_info *)0)
	    panic("msleep: MALLOC");

	/*
	 * We will use the fact that msem_iptr->msem_reg will be
	 * a null pointer if we have allocated the memory, so that
	 * we can free it if we don't use it.
	 */

	bzero(msem_iptr, sizeof(struct msem_info));
    }

    for (;;) {

	/*
	 * Call msem_validate to validate semaphore. This will
	 * guarantee that the remaining steps will not fault on
	 * a copyout. msem_validate will read the contents of the
	 * semaphore into sem_cp.
	 */

	u.u_error = msem_validate(&sem_cp,uap->msem);
	if (u.u_error) {

	    /* If the waiting flag is set, this means that we
	     * already passed the first validation. In this case,
	     * we return EINVAL, since the only cause for this
	     * error should be MMF truncation.
	     */

	    if (waiting)
		u.u_error = EINVAL;
	    break;
	}

	/*
	 * The semaphore might have become unlocked between the time
	 * that msem_lock() checked and we got here (or we have gone
	 * around the loop). We also consider the semaphore unlocked
	 * if the msem_info structure was not just allocated above,
	 * and the msem_locker field is null. The msem_locker field
	 * is only cleared by mwakeup and msem_release (called by exit).
	 * If it was done in mwakeup, then the locker field in the
	 * semaphore should have already been cleared; however, in
	 * the case of msem_release, we don't bother clearing the
	 * locker field in the semaphore.
	 */

	if (   sem_cp.locker == 0
	    || (   msem_iptr->msem_reg != (reg_t *)0
		&& msem_iptr->msem_locker == (struct msem_procinfo *)0) ) {

	    sem_cp.locker = msem_pinfo->lockid;
	    u.u_error = copyout((caddr_t)&sem_cp, uap->msem, sizeof(msemaphore));
	    break;
	}

	/*
	 * make sure that current locker is still alive.  If not, grant the
	 * semaphore to the calling process.  Note that we know that the
	 * list is not empty, since we are on it.
	 *
	 * If this semaphore already has waiters, it should be impossible
	 * for the locker field in the semaphore to be out of sync with the
	 * locker in the sem_info structure (if the locker field in the
	 * semaphore is not 0).  This is impossible, because we don't allow
	 * the lock to be obtained in user land if the wanted field is non
	 * zero.  If they are out of sync, it must be due to someone
	 * trashing the semaphore, so we will go with the kernels view.
	 *
	 * If this semaphore does not have waiters, we have to do a full
	 * search of all procs that have allocated an msemaphore lock id.
	 *
	 * Note that it is possible for a proc to be "waiting" on a
	 * semaphore that does not have a locker. This happens when a
	 * process releases the lock via mwakeup or exit. As mentioned
	 * above, we don't clear the msem_waitptr field until the
	 * process actually obtains the lock.
	 */

	/* Check to see if semaphore has waiters. */

	if (msem_iptr->msem_reg != (reg_t *)0) {

	   /*
	    * Yes, use msem_locker field.
	    */

	    lock_pinfo = msem_iptr->msem_locker;
	}
	else {

	    /* No it does not, so we call msem_pfind to find locker */

	    lock_pinfo = msem_pfind(sem_cp.locker);
	    if (lock_pinfo == (struct msem_procinfo *)0) {

		/* Locker not found, give semaphore to calling process */

		sem_cp.locker = msem_pinfo->lockid;
		u.u_error = copyout((caddr_t)&sem_cp, uap->msem, sizeof(msemaphore));
		break;
	    }
	}

	/*
	 * We now know that the lock is valid. If
	 * condition == MSEM_IF_NOWAIT, return with
	 * errno set to EAGAIN.
	 */

	if (uap->condition == MSEM_IF_NOWAIT) {
	    u.u_error = EAGAIN;
	    break;
	}

	/*
	 * We only need to check for deadlock the first time
	 * through the loop.
	 */

	if (!waiting) {

	    /*
	     * Check for deadlock. A deadlock will occur if
	     * we sleep waiting for a semaphore that will
	     * never be unlocked, due to the fact that we
	     * hold the lock on a semaphore that must be unlocked
	     * before the semaphore we want is unlocked.
	     */

	    tmp_pinfo = lock_pinfo;
	    while (   tmp_pinfo != (struct msem_procinfo *)0
		   && tmp_pinfo->msem_waitptr != (struct msem_info *)0) {

		/*
		 * There is a chance that the msem_locker field of an
		 * msem_info structure for a semaphore that has waiters,
		 * may be null. This happens when a process releases the
		 * lock. The msem_info structure is not released until
		 * the waitcount goes to zero. This is why we check
		 * to make sure that tmp_pinfo is not null at the top
		 * of the while loop.
		 */

		if (tmp_pinfo->msem_waitptr->msem_locker == msem_pinfo) {
		    u.u_error = EDEADLK;
		    break; /* This break only breaks the while loop */
		}

		tmp_pinfo = tmp_pinfo->msem_waitptr->msem_locker;
	    }

	    if (u.u_error)
		break;
	}


	/* Set wanted flag on semaphore, if it has not already been set */

	if (sem_cp.wanted == 0) {
	    sem_cp.wanted = 1;
	    u.u_error = copyout((caddr_t)&sem_cp, uap->msem, sizeof(msemaphore));
	    if (u.u_error)
		break;
	}

	/* If we get this far, it is OK to sleep on this semaphore. */

	/*
	 * First, if we allocated a new msem_info cell, initialize
	 * it and link it in.
	 */

	if (msem_iptr->msem_reg == (reg_t *)0) {
	    msem_iptr->msem_locker = lock_pinfo;
	    msem_iptr->msem_reg = rp;
	    msem_iptr->msem_offset = msem_offset;
	    msem_iptr->wait_count = 0;
	    msem_iptr->lock_next = lock_pinfo->msem_locklist;
	    lock_pinfo->msem_locklist = msem_iptr;
	    hash = msem_hash(rp, msem_offset);
	    msem_iptr->hash_next = msem_hlist[hash];
	    msem_hlist[hash] = msem_iptr;
	}

	/*
	 * Next, if we weren't already waiting on this semaphore,
	 * increment wait count, and set waiting flag and msem_waitptr
	 * field.
	 */

	if (!waiting) {
	    msem_iptr->wait_count++;
	    waiting = 1;
	    msem_pinfo->msem_waitptr = msem_iptr;
	}

	/*
	 * Now, sleep, at an interruptable priority, on the semaphore.
	 * Note that we set PCATCH when calling sleep() so that we
	 * can clean up if we get interrupted.
	 */

	if ( sleep(msem_iptr, PZERO+1 | PCATCH) ) {
	    u.u_error = EINTR;
	    break;
	}

	/*
	 * We have been woken up, so go back to the top of the loop
	 * and start over.
	 */

    } /* Infinite for loop */

    /*
     * We have broken out of the loop.
     * Clean up and return.
     */

    /* If we didn't use the allocated msem_info structure, just free it */

    if (msem_iptr->msem_reg == (reg_t *)0) {
	FREE(msem_iptr, M_DYNAMIC);
    }
    else {

	/*
	 * If we waited on the semaphore, decrement the waitcount and clear
	 * the msem_waitptr field.  If the waitcount goes to zero, unlink
	 * and free up the msem_info structure.
	 */

	if (waiting) {
	    msem_iptr->wait_count--;
	    msem_pinfo->msem_waitptr = (struct msem_info *)0;
	    if (msem_iptr->wait_count == 0) {

		/* Remove this cell from the hash list */

		if (hash == -1)
		    hash = msem_hash(rp, msem_offset);

		if (msem_hlist[hash] == msem_iptr)
		    msem_hlist[hash] = msem_iptr->hash_next;
		else {
		    msem_prev_iptr = msem_hlist[hash];
		    while (msem_prev_iptr->hash_next != msem_iptr)
			msem_prev_iptr = msem_prev_iptr->hash_next;

		    msem_prev_iptr->hash_next = msem_iptr->hash_next;
		}

		/* Remove this cell from the lock list */

		if (msem_iptr->msem_locker != (struct msem_procinfo *)0)
		    msem_locklist_remove(msem_iptr->msem_locker,msem_iptr);

		/* Now we can free the cell */

		FREE(msem_iptr, M_DYNAMIC);
		msem_iptr = (struct msem_info *)0;

		/* Clear the wanted field in the semaphore */

		sem_cp.wanted = 0;
		(void) copyout((caddr_t)&sem_cp, uap->msem, sizeof(msemaphore));
	    }
	}

	/*
	 * If we were given the lock, and there are still other waiters,
	 * we need to update the lock list.
	 */

	if (   sem_cp.locker == msem_pinfo->lockid
	    && msem_iptr != (struct msem_info *)0 ) {

	    if (msem_iptr->msem_locker != msem_pinfo) {

		/* Remove from old owners lock list */

		if (msem_iptr->msem_locker != (struct msem_procinfo *)0)
		    msem_locklist_remove(msem_iptr->msem_locker,msem_iptr);

		/* Add to new owners lock list */

		msem_iptr->lock_next = msem_pinfo->msem_locklist;
		msem_pinfo->msem_locklist = msem_iptr;

		/* Update msem_locker field */

		msem_iptr->msem_locker = msem_pinfo;
	    }
	}
    }

    /* Return to user land */

    return;
}

mwakeup()
{
    register struct a {
	caddr_t msem;
    } *uap = (struct a *)u.u_ap;
    reg_t *rp;
    unsigned long msem_offset;
    msemaphore sem_cp;
    register struct msem_procinfo *msem_pinfo;
    register struct msem_info *msem_iptr;

    /*
     * Call msem_validate to validate semaphore. This will
     * guarantee that the remaining steps will not fault on
     * a copyout. msem_validate will read the contents of the
     * semaphore into sem_cp.
     */

    u.u_error = msem_validate(&sem_cp,uap->msem);
    if (u.u_error)
	return;

    /*
     * Make sure wanted flag is set, if not, just return.
     */

    if (sem_cp.wanted == 0)
	return;

    /*
     * Get the region and region offset for this semaphore. msem_getaddr()
     * also makes sure that the address resides within a memory mapped
     * pregion.
     */

    if (msem_getaddr(uap->msem,&rp,&msem_offset) == -1) {
	u.u_error = EINVAL;
	return;
    }

    /* Get pointer to semaphore */

    msem_iptr = msem_find(rp, msem_offset);
    if (msem_iptr != (struct msem_info *)0) {

	/* Is locker field clear? */

	if (sem_cp.locker != 0) {

	    /*
	     * No, this means that someone else grabbed the lock before
	     * we got into the kernel. So we don't need to do anything,
	     * since the new locker has already removed this semaphore
	     * from our lock list.
	     */

	    return;
	}

	msem_pinfo = u.u_procp->p_msem_info;
	if (   msem_pinfo != (struct msem_procinfo *)0
	    && msem_iptr->msem_locker == msem_pinfo) {

	    /* Remove this semaphore from our lock list */

	    msem_locklist_remove(msem_pinfo,msem_iptr);
	    msem_iptr->msem_locker = (struct msem_procinfo *)0;
	}

	/*
	 * Do wakeup on semaphore. Note that msleep clears wanted field
	 * in semaphore, once there are no more waiters.
	 */

	wakeup(msem_iptr);
    }

    return;
}

unsigned long
msem_proc_init()
{
    register struct msem_procinfo *msem_pinfo = u.u_procp->p_msem_info;

    if (msem_pinfo == (struct msem_procinfo *)0) {

	/*
	 * Since we set M_WAITOK, this malloc should not fail.
	 */

	MALLOC(msem_pinfo, struct msem_procinfo *,
	       sizeof(struct msem_procinfo), M_DYNAMIC, M_WAITOK);

	if (msem_pinfo == (struct msem_procinfo *)0)
	    panic("msem_proc_init: MALLOC");

	bzero(msem_pinfo, sizeof(struct msem_procinfo));

	/* Allocate msem lock id */

	msem_pinfo->lockid = msem_global_lock_id++;
	if (msem_global_lock_id == 0xffffffff)
	    msem_global_lock_id = 1;

	/* Link at front of doubly linked list */

	if (msem_proc_list != (struct msem_procinfo *)0)
	    msem_proc_list->msemp_prev = msem_pinfo;

	msem_pinfo->msemp_next = msem_proc_list;
	msem_proc_list = msem_pinfo;
	msem_pinfo->msemp_prev = (struct msem_procinfo *)0;

	/* Store in pointer in proc structure */

	u.u_procp->p_msem_info = msem_pinfo;
    }
    return msem_pinfo->lockid;
}

/*
 * msem_release is called when a process exits
 */

void
msem_release(p)
    register struct proc *p;
{
    register struct msem_procinfo *msem_pinfo = p->p_msem_info;
    register struct msem_info *msem_iptr;
    register struct msem_info *save_msem_iptr;

    /* We already know that p_msem_info is not a null pointer */

    /* Remove this cell from linked list */

    if (msem_pinfo->msemp_prev == (struct msem_procinfo *)0)
	msem_proc_list = msem_pinfo->msemp_next;
    else
	msem_pinfo->msemp_prev->msemp_next = msem_pinfo->msemp_next;

    if (msem_pinfo->msemp_next != (struct msem_procinfo *)0)
	msem_pinfo->msemp_next->msemp_prev = msem_pinfo->msemp_prev;

    /*
     * Unlock any semaphores that we have locked, and that there are
     * waiters for. Note that we don't clear the locker field in the
     * actual semaphore. We just clear the msem_locker field in the
     * associate msem_info structure, and then do a wakeup on the
     * semaphore. msleep is smart enough to see that we have released
     * the semaphore, and assign it to the first waiter.
     */

    msem_iptr = msem_pinfo->msem_locklist;
    while (msem_iptr != (struct msem_info *)0) {

	/* Since we are zeroing the lock_next pointers as we go,
	 * it would be very bad if we were to sleep here.
	 */

	msem_iptr->msem_locker = (struct msem_procinfo *)0;
	save_msem_iptr = msem_iptr->lock_next;
	msem_iptr->lock_next = (struct msem_info *)0;

	wakeup(msem_iptr);
	msem_iptr = save_msem_iptr;
    }

    FREE(msem_pinfo,M_DYNAMIC);
    p->p_msem_info = (struct msem_procinfo *)0;
    return;
}

/*
 * When the underlying file of a memory mapped region is truncated,
 * we need to release all of the semaphores in the truncated area, that
 * have waiters. Processes that are sleeping on these semaphores will
 * wake up, and attempt to validate the semaphore. The validation should
 * fail, and EINVAL will be returned.
 */

msem_trunc(rp, new_length)
    register reg_t *rp;
    unsigned long new_length;
{
    register int bucket;
    register struct msem_info *msem_iptr;

    for (bucket = 0; bucket < MSEM_HASH_SIZE; bucket++) {

	msem_iptr = msem_hlist[bucket];
	while (msem_iptr != (struct msem_info *)0) {

	    if (   msem_iptr->msem_reg == rp
		&& msem_iptr->msem_offset >= new_length) {

		if (msem_iptr->msem_locker != (struct msem_procinfo *)0) {
		    msem_locklist_remove(msem_iptr->msem_locker,msem_iptr);
		    msem_iptr->msem_locker = (struct msem_procinfo *)0;
		}

		wakeup(msem_iptr);
	    }
	    msem_iptr = msem_iptr->hash_next;
	}
    }

    return;
}

void
msem_locklist_remove(msem_pinfo,msem_iptr)
    register struct msem_procinfo *msem_pinfo;
    register struct msem_info *msem_iptr;
{
    register struct msem_info *msem_prev_iptr;

    if (msem_pinfo->msem_locklist == msem_iptr)
	msem_pinfo->msem_locklist = msem_iptr->lock_next;
    else {
	msem_prev_iptr = msem_pinfo->msem_locklist;
	while (msem_prev_iptr->lock_next != msem_iptr)
	    msem_prev_iptr = msem_prev_iptr->lock_next;

	msem_prev_iptr->lock_next = msem_iptr->lock_next;
    }

    return;
}

msem_validate(sem_cpptr,usem)
    msemaphore *sem_cpptr;
    caddr_t usem;
{
    register int status;

    /*
     * First write to the first byte of the magic number of the
     * semaphore. This detects invalid pointers and ensures that
     * we have write access to the page that the semaphore is on,
     * so that the remaining steps will be "atomic". Note that
     * we only write the first byte, so that we don't mistakenly
     * validate an invalid semaphore.
     */

    sem_cpptr->magic = __MSEM_MAGIC;

    status = copyout((caddr_t)&sem_cpptr->magic,
		     (caddr_t)&((msemaphore *)usem)->magic,
		     1);

    if (status != 0)
	return status;

    /* Now read the semaphore contents back in */

    status = copyin(usem,(caddr_t)sem_cpptr,sizeof(msemaphore));

    /* The copyin shouldn't have failed, but just in case ... */

    if (status != 0)
	return status;

    /* Check for valid magic number */

    if (sem_cpptr->magic != __MSEM_MAGIC)
	return EINVAL;

    return 0;
}

struct msem_procinfo *
msem_pfind(lockid)
    register unsigned long lockid;
{
    register struct msem_procinfo *tmp_pinfo;

    tmp_pinfo = msem_proc_list;
    while (tmp_pinfo != (struct msem_procinfo *)0) {
	if (tmp_pinfo->lockid == lockid)
	    break;

	tmp_pinfo = tmp_pinfo->msemp_next;
    }

    return tmp_pinfo;
}

struct msem_info *
msem_find(rp, roffset)
    register reg_t *rp;
    register unsigned long roffset;
{
    register struct msem_info *miptr;
    register int hash;

    hash = msem_hash(rp, roffset);
    miptr = msem_hlist[hash];

    while (miptr != (struct msem_info *)0) {
	if (miptr->msem_reg == rp && miptr->msem_offset == roffset)
	    break;

	miptr = miptr->hash_next;
    }

    return miptr;
}

msem_getaddr(sem_addr,rp_ptr,roffset_ptr)
    caddr_t sem_addr;
    reg_t **rp_ptr;
    unsigned long *roffset_ptr;
{
    register preg_t *prp;

    prp = searchprp(u.u_procp->p_vas, -1, sem_addr);
    if (prp == (preg_t *)0 || prp->p_type != PT_MMAP)
	return -1;

    /* Get the region pointer and region offset for this semaphore */

    if (rp_ptr != (reg_t **)0)
	*rp_ptr = prp->p_reg;

    if (roffset_ptr != (unsigned long *)0)
	*roffset_ptr = ptob(prp->p_off) + (sem_addr - prp->p_vaddr);

    return 0;
}

msem_hash(rp, roffset)
    reg_t *rp;
    unsigned long roffset;
{
    register unsigned long hash;

    hash = (unsigned long)rp ^ roffset;
    hash = (hash >> 16) ^ (hash & 0xffff);
    hash = (hash >> 8) ^ (hash & 0xff);
    hash = (hash >> 4) ^( hash & 0xf);

    return (int)hash;
}
