/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sysV_shm.c,v $
 * $Revision: 1.52.83.3 $       $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 20:11:58 $
 */


#include "../h/types.h"
#include "../h/param.h"
#include "../h/vmmac.h"
#include "../h/sysmacros.h"
#include "../h/dir.h"
#include "../h/errno.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/vas.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/debug.h"
#include "../h/kernel.h"
#include "../machine/param.h"


/*
 * Shared memory list lock
 */
vm_sema_t shm_lock;

#define shmlock()		vm_psema(&shm_lock, PZERO);

#define shmunlock()		{ \
				VASSERT(vm_valusema(&shm_lock) <= 0); \
				vm_vsema(&shm_lock, 0); \
				}
				

extern	struct	shmid_ds	shmem[]; /* shared memory headers */
extern	struct	shminfo		shminfo; /* shared memory info structure */

struct	shmid_ds	*ipcgetperm(),
			*shmconv();

#define PROTMAP(x) ((x)?(PROT_USER|PROT_READ|PROT_EXECUTE):(PROT_USER|PROT_READ|PROT_WRITE|PROT_EXECUTE))

/* flags indicating whether old or new shmid_ds is being used */
#define OLD_SHMID_DS    1
#define NEW_SHMID_DS    2

union shmunion {
        struct  shmid_ds        ds;     /* shmid_ds */
        struct  oshmid_ds       ods;    /* old shmid_ds structure
                                         * supported for object code
                                         * compatibility
                                         */
};

/*
 * Shmat (attach shared segment) system call.
 */
shmat()
{
	register struct a {
		int	shmid;
		uint	addr;
		int	flag;
	}	*uap = (struct a *)u.u_ap;
	register struct shmid_ds	*sp;
	register reg_t			*rp;
	register preg_t			*prp;
	register int	shmn;
	register vas_t	*vas;

	/* Check that the number of shared memory segments attached
	 * to this process does not exceed shminfo.shmseg.
	 */
	shmn = 0;
	vas = u.u_procp->p_vas;
	vaslock(vas);
	for (prp = vas->va_next; prp != (preg_t *) vas; prp = prp->p_next)
		if (prp->p_type == PT_SHMEM)
			++shmn;
	vasunlock(vas);
	if (shmn >= shminfo.shmseg) {
		u.u_error = EMFILE;
		return;
	}

	shmlock();		/* lock down shmem list */

	/*
	 * Check that the segment actually exists and that this user
	 * can access it.
	 */
	if ((sp = shmconv(uap->shmid, SHM_DEST)) == NULL) {
		shmunlock();
		return;
	}

	if (ipcaccess(&sp->shm_perm, SHM_R)) {
		shmunlock();
		return;
	}

	if ((uap->flag & SHM_RDONLY) == 0) {
		if (ipcaccess(&sp->shm_perm, SHM_W)) {
			shmunlock();
			return;
		}
	}

	prp = sp->shm_vas->va_next;

	/*
	 * The psuedo-pregion must be the one that is in the active
	 * list.  All other pregions are added with PF_NOPAGE set.
	 */
	VASSERT(prp->p_flags | PF_ACTIVE);

	rp = prp->p_reg;
	reglock(rp);

	/*
	 * Attach to the region.  We set PF_NOPAGE since the
	 * psuedo-pregion is already in the active list.
	 */
	if ((prp = attachreg(u.u_procp->p_vas, rp, 
		     PROTMAP(uap->flag & SHM_RDONLY), PT_SHMEM,
		     (uap->addr ? PF_EXACT : 0) | PF_NOPAGE,
		     (uap->flag & SHM_RND) ?
		     (caddr_t)(uap->addr & ~(SHMLBA -1)):(caddr_t)uap->addr,
		     0, (size_t)prp->p_count)) == NULL) {
		regrele(rp);
		shmunlock();
		return;
	}
	if ((sp->shm_perm.mode & SHM_NOSWAP) &&
		!mlockpreg(prp)) {
		detachreg(u.u_procp->p_vas, prp);
		u.u_error = ENOMEM;
		shmunlock();
		return;
	}
	hdl_procattach(&u, u.u_procp->p_vas, prp);
	regrele(rp);
	sp->shm_nattch++;
	sp->shm_atime = time.tv_sec;
	sp->shm_lpid = u.u_procp->p_pid;
	sp->shm_perm.mode &= ~SHM_CLEAR;	/* was set when seg created */
	shmunlock();

	u.u_rval1 = (int) prp->p_vaddr;
	u.u_error = 0;
}

/*
 * Convert user supplied shmid into a ptr to the associated
 * shared memory header.
 *
 * IMPORTANT:
 * The caller of shmconv() must hold the shmem list lock.
 */
struct shmid_ds *
shmconv(s,flg)
register int	s;	/* shmid */
int		flg;
{
	register struct shmid_ds	*sp;	/* ptr to associated header */

	VASSERT(vm_valusema(&shm_lock) <= 0);

	if (s < 0)
	{
		u.u_error = EINVAL;
		return(NULL);
	}
	sp = &shmem[s % shminfo.shmmni];
	if (!(sp->shm_perm.mode & IPC_ALLOC) || sp->shm_perm.mode & flg
                || s / shminfo.shmmni != sp->shm_perm.seq) {
		u.u_error = EINVAL;
		return(NULL);
	}
	return(sp);
}

/*
 * BEGIN_DESC
 * 
 * shminit()
 *
 * Input Parameters:
 *	None
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	shm_lock
 *
 * Description:
 *	Initialize the shmem list lock semaphore
 *
 * END_DESC
 */
void
shminit()
{
	vm_initsema(&shm_lock, 1, SHMID_LOCK_ORDER, "shmem list sema");
}

/*
 * Convert a region pointer into a pointer to the shared memory segment
 * that has that region hanging from it.
 *
 * IMPORTANT:
 * The caller of regtoshm() must hold the shmem list lock.
 */
struct shmid_ds *
regtoshm(rp)
reg_t *rp;
{
	struct shmid_ds *sp;
	vas_t *vas;
	preg_t *prp;

	VASSERT(rp != (reg_t *)NULL);
	VASSERT(vm_valusema(&shm_lock) <= 0);

	for (sp = shmem; sp < &shmem[shminfo.shmmni]; sp++) {
		vas = sp->shm_vas;
		if (vas == (vas_t *)NULL)
			continue;	/* no shared memory segment here */
		prp = vas->va_next;
		VASSERT(prp != (preg_t *)NULL);
		VASSERT(prp != (preg_t *)sp->shm_vas);
		VASSERT(prp->p_next == (preg_t *)vas);
		VASSERT(prp->p_reg != (reg_t *)NULL);
		if (prp->p_reg == rp)
			return sp;
	}

	return (struct shmid_ds *)NULL;		/* didn't find it */
}

/*
 * Do all IPC_RMID processing on a given shared memory segment.  Called
 * from shmctl() for the actual removal request, shmdt() for disposal at
 * last detach, and dispreg() (through detachshm()) to handle exit() and
 * exec() processing.
 *
 * If there is only one pregion (the one attached to the shmem pseudo-vas)
 * connected to the region and the shared memory segment has been destroyed
 * (SHM_DEST), we're ready to clean up this segment.
 *
 * IMPORTANT:
 * The caller of shmrmseg() must hold the shmem list lock.
 *
 */
#define	SHM_REMOVED	1
#define	SHM_NOTREMOVED	(!(SHM_REMOVED))

int
shmrmseg(sp)
struct shmid_ds *sp;
{
	vas_t *vas;
	preg_t *prp;
	reg_t *rp;

	VASSERT(sp != (struct shmid_ds *)NULL);
	VASSERT(sp >= shmem);
	VASSERT(sp < &shmem[shminfo.shmmni]);
	VASSERT(vm_valusema(&shm_lock) <= 0);

	if (!(sp->shm_perm.mode & SHM_DEST))
		return SHM_NOTREMOVED;	/* never removed, don't touch it */

	vas = sp->shm_vas;
	VASSERT(vas != (vas_t *)NULL);
	prp = vas->va_next;
	VASSERT(prp != (preg_t *)NULL);
	VASSERT(prp != (preg_t *)sp->shm_vas);
	VASSERT(prp->p_next == (preg_t *)vas);
	VASSERT(prp->p_reg != (reg_t *)NULL);
	rp = prp->p_reg;

	/* if anyone (else) is still attached, we can't do anything yet */
	reglock(rp);
	VASSERT(rp->r_refcnt >= 1);
	if (rp->r_refcnt != 1) {
		regrele(rp);
		return SHM_NOTREMOVED;
	}

	sp->shm_vas = (vas_t *)0;
	sp->shm_segsz = 0;

	detachreg(vas, prp);
	VASSERT(vas->va_next == (preg_t *)vas);
	vaslock(vas);
	vas->va_refcnt--;
	freevas(vas);

	sp->shm_perm.mode = 0;

	if (((int)(++(sp->shm_perm.seq) * shminfo.shmmni + (sp - shmem))) < 0)
		sp->shm_perm.seq = 0;

	return SHM_REMOVED;
}

/*
 * BEGIN_DESC
 * 
 * detachshm()
 *
 * Input Parameters:
 *	vas - pointer to the vas list of the process
 *	prp - pointer to a shared memory pregion
 *
 * Return Value:
 *	None.
 *
 * Globals Referenced:
 *	shm_lock
 *
 * Description:
 *	This routine is called by dispreg() to detach the running
 *	process from a shared memory segment.  It may also remove
 *	the segment if appropriate.
 *
 * Algorithm:
 *	Detachshm() first detaches the current process from a shared
 *	memory region.  It then calls shmrmseg().  Shmrmseg() checks
 *	whether the shared memory segement has been marked for removal
 *	by an IPC_RMID operation and whether there is no longer any
 *	process attached to the segment.  If so, it then removes the
 *	shared memory segment.
 *
 *	The calls to detachreg() and shmrmseg() are protected by the
 *	same shmem list lock to avoid race conditions.  The process
 *	should not hold the region lock while it is trying to obtain
 *	the shmem list lock.  The region lock should be reacquired
 *	after the process locks the shmem list.
 *
 * In/Out conditions:
 *	The region lock is held on entry.  It is released before
 *	locking the shmem list and is reacquired afterwards.  On
 *	exit, both the region lock and the shmem list lock are
 *	released.
 *
 * END_DESC
 */
void
detachshm(vas, prp)
vas_t *vas;
preg_t *prp;
{
	reg_t *rp;
	struct shmid_ds *sp;

	rp = prp->p_reg;

	/*
	 * Release the region lock and reacquire it after the
	 * shmem list is locked.
	 */
	regrele(rp);
	shmlock();
	reglock(rp);
	sp = regtoshm(rp);
	VASSERT(sp != NULL);
	detachreg(vas, prp);
	if (shmrmseg(sp) == SHM_NOTREMOVED) {

		sp->shm_nattch--;
	}

	shmunlock();
}

/*
 * oshmctl - Old shmctl system call.
 *           Retained for object code compatibility.
 */
oshmctl()
{
        shmctl1(OLD_SHMID_DS);
}

/*
 * shmctl - Shmctl system call.
 */
shmctl()
{
        shmctl1(NEW_SHMID_DS);
}

/*
 * Shmctl1 - Main body of shmctl system call.
 */
shmctl1(shmid_ds_type)
int shmid_ds_type;      /* Is old or new shmid_ds being used? */
{
	struct a {
		int		shmid,
				cmd;
                union shmunion *        arg;
	}	*uap = (struct a *)u.u_ap;
	struct shmid_ds	*sp;	/* shared memory header ptr */
	register reg_t *rp;
        union shmunion shmtmp;

	shmlock();		/* lock down shmem list */
	if ((sp = shmconv(uap->shmid, (uap->cmd == IPC_STAT) ? 0 : SHM_DEST)) ==
                NULL) {
		shmunlock();
		return;
	}

	switch(uap->cmd) {

	/* Remove shared memory identifier. */
	case IPC_RMID:
		if (u.u_uid != sp->shm_perm.uid && u.u_uid != sp->shm_perm.cuid
		   		&& !suser()) {
					/* suser() sets u_error = EPERM */
			shmunlock();
			return;
		}

		sp->shm_perm.mode |= SHM_DEST;
		sp->shm_ctime = time.tv_sec;
		/*
		 * Change to a private key so the old key can be reused
		 * without waiting for our last detach.  The only accesses
		 * allowed to this segment now are shmdt() and
		 * shmctl(IPC_STAT).  All others will give bad shmid.
		 */
		sp->shm_perm.key = IPC_PRIVATE;

		(void)shmrmseg(sp);

		break;

	/* Set ownership and permissions. */
	case IPC_SET:
		if (u.u_uid != sp->shm_perm.uid && u.u_uid != sp->shm_perm.cuid
			 && !suser()) {
					/* suser() sets u_error = EPERM */
			shmunlock();
			return;
		}

                if (shmid_ds_type == NEW_SHMID_DS) {
                        if (copyin((caddr_t) uap->arg, (caddr_t) &shmtmp.ds,
                                sizeof(shmtmp.ds))) {
                                u.u_error = EFAULT;
				shmunlock();
				return;
                        }

                        /*
                         * We only need to copy the fields the user can set.
                         */
                        sp->shm_perm.uid = shmtmp.ds.shm_perm.uid;
                        sp->shm_perm.gid = shmtmp.ds.shm_perm.gid;
                        sp->shm_perm.mode = (shmtmp.ds.shm_perm.mode & 0777) |
                                (sp->shm_perm.mode & ~0777);
                }
                else {
                        if (copyin((caddr_t) uap->arg, (caddr_t) &shmtmp.ods,
                                sizeof(shmtmp.ods))) {
                                u.u_error = EFAULT;
				shmunlock();
				return;
                        }

                        /*
                         * We only need to copy the fields the user can set.
                         */
                        sp->shm_perm.uid = (uid_t) shmtmp.ods.shm_perm.uid;
                        sp->shm_perm.gid = (gid_t) shmtmp.ods.shm_perm.gid;
                        sp->shm_perm.mode = (shmtmp.ods.shm_perm.mode & 0777)|
                                (sp->shm_perm.mode & ~0777);
                }
		sp->shm_ctime = time.tv_sec;
		break;

	/* Get shared memory data structure. */
	case IPC_STAT:
		if (ipcaccess(&sp->shm_perm, SHM_R)) {
			shmunlock();
			return;
		}

		VASSERT(sp->shm_vas != 0);
		VASSERT(sp->shm_vas->va_next != (preg_t *)sp->shm_vas);
                /*
                 * We have not locked the region "rp".  Since the incore
                 * count is used only for statistical purposes, it is
                 * okay if it is not perfect
                 */
                rp = sp->shm_vas->va_next->p_reg;
                sp->shm_cnattch = rp->r_incore - 1;

                if (shmid_ds_type == NEW_SHMID_DS) {
                        if (copyout((caddr_t) sp, (caddr_t) uap->arg,
                                sizeof(*sp))) {
                                u.u_error = EFAULT;
                        }
                }
                else {
                        /*
                         * Copy fields in the new shmid_ds structure to
                         * the old shmid_ds structure.  Note that there are
                         * old and new versions of struct ipc_perm as well.
                         */
                        shmtmp.ods.shm_perm.uid = (__ushort) sp->shm_perm.uid;
                        shmtmp.ods.shm_perm.gid = (__ushort) sp->shm_perm.gid;
                        shmtmp.ods.shm_perm.cuid =
                                (__ushort) sp->shm_perm.cuid;
                        shmtmp.ods.shm_perm.cgid =
                                (__ushort) sp->shm_perm.cgid;
                        shmtmp.ods.shm_perm.mode = sp->shm_perm.mode;
                        shmtmp.ods.shm_perm.seq = sp->shm_perm.seq;
                        shmtmp.ods.shm_perm.key = sp->shm_perm.key;
#ifdef __hp9000s800
                        shmtmp.ods.shm_perm.ndx = sp->shm_perm.ndx;
                        shmtmp.ods.shm_perm.wait = sp->shm_perm.wait;
#endif /* __hp9000s800 */
                        shmtmp.ods.shm_segsz = sp->shm_segsz;
                        shmtmp.ods.shm_vas = sp->shm_vas;
                        shmtmp.ods.shm_lpid = (__ushort) sp->shm_lpid;
                        shmtmp.ods.shm_cpid = (__ushort) sp->shm_cpid;
                        shmtmp.ods.shm_nattch = sp->shm_nattch;
                        shmtmp.ods.shm_cnattch = sp->shm_cnattch;
                        shmtmp.ods.shm_atime = sp->shm_atime;
                        shmtmp.ods.shm_dtime = sp->shm_dtime;
                        shmtmp.ods.shm_ctime = sp->shm_ctime;

                        if (copyout((caddr_t) &shmtmp.ods, (caddr_t) uap->arg,
                                sizeof(shmtmp.ods))) {
                                u.u_error = EFAULT;
                        }
                }
		break;

	/* Lock segment in memory */
	case SHM_LOCK:
		if (!suser()) {
			u.u_error = 0;
		 	if (!in_privgrp(PRIV_MLOCK, (struct ucred *)0)) {
				u.u_error = EPERM;
				shmunlock();
				return;
			}

			if (u.u_uid != sp->shm_perm.uid &&
				u.u_uid != sp->shm_perm.cuid) {
				u.u_error = EPERM;
				shmunlock();
				return;
			}
		}
		if (!(sp->shm_perm.mode & SHM_NOSWAP)) {
			rp = sp->shm_vas->va_next->p_reg;
			reglock(rp);
			if (!mlockpreg(sp->shm_vas->va_next)) {
				regrele(rp);
				u.u_error = ENOMEM;
				shmunlock();
				return;
			}
			regrele(rp);
			sp->shm_perm.mode |= SHM_NOSWAP;
		}
		break;

	/* Unlock segment */
	case SHM_UNLOCK:
		if (!suser()) {
			u.u_error = 0;
		 	if (!in_privgrp(PRIV_MLOCK, (struct ucred *)0)) {
				u.u_error = EPERM;
				shmunlock();
				return;
			}

			if (u.u_uid != sp->shm_perm.uid &&
					u.u_uid != sp->shm_perm.cuid) {
				u.u_error = EPERM;
				shmunlock();
				return;
			}
		}
		if (sp->shm_perm.mode & SHM_NOSWAP) {
			rp = sp->shm_vas->va_next->p_reg;
			reglock(rp);
			munlockpreg(sp->shm_vas->va_next);
			regrele(rp);
			sp->shm_perm.mode &= ~SHM_NOSWAP;
		} else {
			u.u_error = EINVAL;
			shmunlock();
			return;
		}
		break;

	default:
		u.u_error = EINVAL;
		break;
	}

	shmunlock();
}

/*
 * Detach shared memory segment
 */
shmdt()
{
	register struct a {
		caddr_t	addr;
	} *uap = (struct a *)u.u_ap;
	register preg_t		*prp;
	register reg_t		*rp;
	register struct shmid_ds *sp;
	vas_t *vas = u.u_procp->p_vas;

	/*
	 * Find matching shmem address in process vas.
	 */
	if ((prp = searchprp(vas, -1, uap->addr)) == NULL) {
		u.u_error = EINVAL;
		return;
	}
	if (prp->p_type != PT_SHMEM) {
		u.u_error = EINVAL;
		return;
	}
	/* address provided must be the starting address -- standards */
	if (uap->addr != prp->p_vaddr) {
		u.u_error = EINVAL;
		return;
	}

	rp = prp->p_reg;

	/*
	 * The following calls to detachreg() and shmrmseg() must be
	 * protected by the same shmem list lock to avoid race conditions.
	 * The shmem list lock should be acquired before calling
	 * reglock to lock the shared memory region.
	 */
	shmlock();		/* lock down shmem list */
	reglock(rp);
	hdl_procdetach(&u, vas, prp);
	sp = regtoshm(rp);
	VASSERT (sp != NULL);
	detachreg(vas, prp);

	if (shmrmseg(sp) == SHM_NOTREMOVED) {
		/*
		 * This wasn't the last detach of a SHM_DEST'd
		 * segment, so update the detach time and pid.
		 */
		sp->shm_dtime = time.tv_sec;
		sp->shm_lpid = u.u_procp->p_pid;
		sp->shm_nattch--;
	}

	shmunlock();
}

/*
 * Shmget (create new shmem) system call.
 */
shmget()
{
	struct a {
		key_t	key;
		uint	size,
			shmflg;
	}	*uap = (struct a *)u.u_ap;
	struct shmid_ds	*sp;	/* shared memory header ptr */
	int s;			/* ipcgetperm status */
	register size_t size;
	register reg_t *rp;
	register vas_t *vas;

	shmlock();		/* lock down shmem list */
	if ((sp = ipcgetperm(uap->key, (int)uap->shmflg, 
		(struct ipc_perm *)shmem, shminfo.shmmni, sizeof(*sp), &s))
								== NULL) {
		shmunlock();
		return;
	}

	if (s) {
		/*
		 * This is a new shared memory segment.
		 */
		if (uap->size < shminfo.shmmin || uap->size > shminfo.shmmax) {
			u.u_error = EINVAL;
			sp->shm_perm.mode = 0;
			shmunlock();
			return;
		}
		sp->shm_perm.mode &= ~SHM_NOSWAP;	/* don't lock */
		sp->shm_perm.mode |= SHM_CLEAR;	/* reset on 1st (any) attach */
		sp->shm_segsz = uap->size;
		vas = allocvas();
		vasunlock(vas);
		sp->shm_atime = sp->shm_dtime = 0;
		sp->shm_ctime = time.tv_sec;
		sp->shm_nattch = 0;
		sp->shm_lpid = 0;
		sp->shm_cpid = u.u_procp->p_pid;
		if ((rp = allocreg( (struct vnode *) 0 , swapdev_vp,
							RT_SHARED)) == NULL) {
			u.u_error = ENOMEM;
			goto bad;
		}
		size = btorp(uap->size);

		/* Check if we have enough swap space to cover growth */
		if (growreg(rp, (int)size, DBD_DZERO) < 0) {
			freereg(rp);
			goto bad;
		}

		/* Create shm view of entire shared memory segment */
		if (( attachreg(vas, rp, PROT_KRW, PT_SHMEM,
				     0, (caddr_t)0, 0, size)) == NULL) {
			freereg(rp);
			goto bad;
		}
		regrele(rp);
		sp->shm_vas = vas;
	} else {
		/*
		 * Found an existing segment.  Check size
		 */
		if (uap->size && uap->size > sp->shm_segsz) {
			u.u_error = EINVAL;
			shmunlock();
			return;
		}
	}

	u.u_rval1 = sp->shm_perm.seq * shminfo.shmmni + (sp - shmem);
	shmunlock();
	return;

bad:

	/*
	 * An error occured so free the vas
	 */
	vaslock(vas);
	VASSERT(vas->va_next == (preg_t *) vas);
	vas->va_refcnt--;
	freevas(vas);

	/*
	 * Now free the shared memory entry.
	 */
	if (((int)(++(sp->shm_perm.seq) * shminfo.shmmni
		   + (sp - shmem))) < 0)
		sp->shm_perm.seq = 0;
	sp->shm_perm.mode = 0;
	u.u_error = ENOMEM;
	shmunlock();

}

/*
 * BEGIN_DESC
 *
 * shm_fork()
 *
 * Input Parameters:
 *      rp-pointer to a region.  This region is a shared memory region
 *      that is associated to a process trying to fork.
 *
 * Return Value:
 *      None.
 *
 * Globals Referenced:
 *      shm_lock
 *
 * Description:
 *      shm_fork is called  as  a process attempts to fork.  It increments the
 *      shared memory reference and core counts.  The maintenance of the
 *      share memory reference count is for providing accurate information
 *	for ipcs(1).
 *
 * Algorithm:
 *      Upon entry, the shared memory region pointer is used to find a
 *      corresponding entry in the shared memory table.  The core count
 *      of that entry is then incremented.
 *      
 * In/Out conditions:
 *      rp is a valid shared memory region pointer.
 *
 *	Note that rp is used only to locate the corresponding shared memory
 *      segement, and is not used to dereference region data.  Therefore,
 *      it is not necessary to lock the region.
 *
 * END_DESC
 */

void
shm_fork(rp)
reg_t *rp;

{
	struct shmid_ds	*sp;

	VASSERT(rp != NULL);
	shmlock();		/* Lock shared memory list */
	sp = regtoshm(rp);
	VASSERT(sp != NULL);
	sp->shm_nattch++; 
	shmunlock();		/* Unlock shared memory list */

}

/*
 * BEGIN_DESC
 *
 * shm_fork_backout()
 *
 * Input Parameters:
 *      rp-pointer to a region.  This region is a shared memory region
 *      and is associated to a process whose creation failed.
 *     
 * Return Value:
 *      None.
 *
 * Globals Referenced:
 *      shm_lock
 *
 * Description:
 *      shm_fork_backout is called  when an attempt to fork a new
 *      process (dupvas()) fails.  shm_fork_backout decrements the shared 
 *      memory reference and core counts back to their original value. 
 *      The maintenance of the share memory reference count is for 
 *      providing accurate information for ipcs(1).
 *
 * Algorithm:
 *      Upon entry, the shared memory region pointer is used to find a
 *      corresponding entry in the shared memory table.  The core count
 *      of that entry is then decremented.
 *      
 * In/Out conditions:
 *      rp is a valid shared memory region pointer.
 *
 *	Note that rp is used only to locate the corresponding shared memory
 *      segement, and is not used to dereference region data.  Therefore,
 *      it is not necessary to lock the region.
 *
 * END_DESC
 */

void
shm_fork_backout(rp)
reg_t *rp;

{
	struct shmid_ds	*sp;

	VASSERT(rp != NULL);
	shmlock();		/* Lock shared memory list */
	sp = regtoshm(rp);
	VASSERT(sp != NULL);
	sp->shm_nattch--; 
	shmunlock();		/* Unlock shared memory list */

}

