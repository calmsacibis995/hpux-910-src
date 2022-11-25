/* @(#) $Revision: 70.2 $ */

#ifdef _NAMESPACE_CLEAN
#   define sysconf _sysconf
#endif /* _NAMESPACE_CLEAN */

#include <errno.h>
#include <sys/mman.h>
#include <sys/unistd.h>

#ifdef __hp9000s800
#include <machine/cpu.h>

unsigned long _mcas_util_addr; /* Cache for mcas_util() address */

#endif /* __hp9000s800 */

/*
 * Memory mapped files semaphore lock/unlock routines.
 */
#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _msem_lock msem_lock
#   define msem_lock _msem_lock
#endif /* _NAMESPACE_CLEAN */

int
msem_lock(sem, condition)
    msemaphore *sem;
    int condition;
{
    register int mcas_status;

#ifdef __hp9000s800

    /* Check to see if we have got the mcas_util address yet */

    if (_mcas_util_addr == 0) {

	/*
	 * cache the mcas_util address. The PA-RISC version of mcas
	 * uses a special light weight call which resides on the
	 * gateway page. The mcas libc stub expects the address of this
	 * routine to be stored in _mcas_util_addr.
	 */

	_mcas_util_addr = GW_PAGE_VADDR + sysconf(_SC_MCAS_OFFSET);
    }
#endif /* __hp9000s800 */

    /*
     * Check condition for a legal value, and make sure that sem
     * is pointing at a valid semaphore.
     */

    if (   (condition != 0 && condition != MSEM_IF_NOWAIT)
	|| sem->magic != __MSEM_MAGIC ) {

	errno = EINVAL;
	return -1;
    }

    /* Call lightweight kernel call to attempt to lock the semaphore. */

    if ((mcas_status = _mcas(sem)) == __MCAS_OK)
	return 0;

    /*
     * msem_init() will allocate the per process msemaphore lock id for a
     * process when it is called for the first time. This only needs to be
     * done once for the life of the process. However, if the semaphore was
     * created by another process, this process may not have a lock id
     * allocated yet. In this case, _mcas returns the __MCAS_NOLOCKID
     * status, since it cannot allocate the lock id itself. We use
     * sysconf() to allocate the lock id, and then retry the _mcas call.
     */

    if (mcas_status == __MCAS_NOLOCKID) {
	(void) sysconf(_SC_MSEM_LOCKID);
	if ((mcas_status = _mcas(sem)) == __MCAS_OK)
	    return 0;
    }

    /*
     * Check to see if someone else has the lock, or there was some
     * other error.
     */

    if (mcas_status != __MCAS_BUSY) {
	errno = EFAULT;
	return -1;
    }

    /*
     * Did not get the lock. It is possible that the lock is stale
     * (i.e. held by a process that has exited), so we can't just
     * return here if condition == MSEM_IF_NOWAIT. We pass the condition
     * variable into msleep(). If the lock is stale, we will give
     * the lock to the calling process, and return. If the lock is not stale,
     * msleep() will return immediately if condition == MSEM_IF_NOWAIT.
     * If condition is not MSEM_IF_NOWAIT, we will sleep until this
     * semaphore becomes available. msleep() will set sem->wanted
     * to 1 to indicate that we are sleeping.
     */

    return _msleep(sem, condition);
}

#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF _msem_unlock msem_unlock
#   define msem_unlock _msem_unlock
#endif /* _NAMESPACE_CLEAN */

int
msem_unlock(sem, condition)
    msemaphore *sem;
    int condition;
{

    /*
     * Check condition for a legal value, and make sure that sem
     * is pointing at a valid semaphore.
     */

    if (   (condition != 0 && condition != MSEM_IF_WAITERS)
	|| sem->magic != __MSEM_MAGIC ) {

	errno = EINVAL;
	return -1;
    }

    /*
     * Note the order of execution here. Setting the locker field
     * to 0 and/or checking the wanted field is not atomic. However,
     * by executing the statements in this order, we are safe.
     * Two "bad" things could happen. We could wind up not giving up
     * the lock, if condition == MSEM_IF_WAITERS, even if there are
     * waiters. Or we could wind up calling mwakeup() when it is not
     * necessary or while the semaphore is locked. These are all
     * race conditions, but there are no ill effects, and they will
     * occur rarely. It is assumed that a process that calls msem_unlock
     * with condition == MSEM_IF_WAITERS will periodically check, so
     * the lock will be released the next time around.
     */

    if (condition == MSEM_IF_WAITERS && sem->wanted == 0) {
	errno = EAGAIN;
	return -1;
    }

    /* Give up lock */

    sem->locker = 0;

    /* If there are waiters, call mwakeup() */

    if (sem->wanted)
	return _mwakeup(sem); /* mwakeup() clears sem->wanted */

    return 0;
}
