/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sysV_sem.c,v $
 * $Revision: 1.27.83.4 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 93/10/31 12:00:59 $
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

/*
**	Inter-Process Communication Semaphore Facility.
*/

#include "../h/param.h"
#ifdef hp9000s800
#include "../h/dir.h"
#endif
#include "../h/errno.h"
#include "../h/signal.h"
#include "../h/ipc.h"
#include "../h/sem.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#ifdef SAR
#include "../h/sar.h"
#endif
#ifdef __hp9000s300
#define	STACK_300_OVERFLOW_FIX
#endif

#ifdef	STACK_300_OVERFLOW_FIX
#include "../h/malloc.h"
#endif	/* STACK_300_OVERFLOW_FIX */

extern struct semid_ds	sema[];		/* semaphore data structures */
extern struct sem	sem[];		/* semaphores */
extern struct ipcmap	semmap[];	/* sem allocation map */
extern struct sem_undo	*sem_undo[];	/* undo table pointers */
extern struct sem_undo	semu[];		/* operation adjust on exit table */
extern struct seminfo seminfo;		/* param information structure */
struct sem_undo	*semunp;		/* ptr to head of undo chain */
struct sem_undo	*semfup;		/* ptr to head of free undo chain */

extern time_t	time;			/* system idea of date */

struct semid_ds	*ipcgetperm(),
		*semconv();

/* flags indicating whether old or new semid_ds is being used */
#define OLD_SEMID_DS	1
#define NEW_SEMID_DS	2

union semunion {
	ushort	semvals[SEMMSL];	/* set semaphore values */
	struct	semid_ds	ds;	/* set permission values */
	struct	osemid_ds	ods;	/* old semid_ds structure
					 * supported for object code
					 * compatibility
					 */
};

/*
**	semaoe - Create or update adjust on exit entry.
*/

#define SEMOP_PROTECTED


semaoe(val, id, num)
short	val,	/* operation value to be adjusted on exit */
	num;	/* semaphore # */
int	id;	/* semid */
{
	register struct undo		*uup,	/* ptr to entry to update */
					*uup2;	/* ptr to move entry */
	register struct sem_undo	*up,	/* ptr to process undo struct */
					*up2;	/* ptr to undo list */
	register int			i,	/* loop control */
					found;	/* matching entry found flag */
	register long			tmp_aoe;/* used for range checking */

	if(val == 0)
		return(0);
	if(val > seminfo.semaem || val < -seminfo.semaem) {
		u.u_error = ERANGE;
		return(1);
	}
	if((up = sem_undo[pindx(u.u_procp)]) == NULL)
		if (up = semfup) {
			semfup = up->un_np;
			up->un_np = NULL;
			sem_undo[pindx(u.u_procp)] = up;
		} else {
			u.u_error = ENOSPC;
			return(1);
		}
	for(uup = up->un_ent, found = i = 0;i < up->un_cnt;i++) {
		if(uup->un_id < id || (uup->un_id == id && uup->un_num < num)) {
			uup++;
			continue;
		}
		if(uup->un_id == id && uup->un_num == num)
			found = 1;
		break;
	}
	if(!found) {
		if(up->un_cnt >= seminfo.semume) {
			u.u_error = EINVAL;
			return(1);
		}
		if(up->un_cnt == 0) {
			up->un_np = semunp;
			semunp = up;
		}
		uup2 = &up->un_ent[up->un_cnt++];
		while(uup2-- > uup)
			*(uup2 + 1) = *uup2;
		uup->un_id = id;
		uup->un_num = num;
		uup->un_aoe = -val;
		return(0);
	}
	tmp_aoe = (long)uup->un_aoe - (long)val;
	if(tmp_aoe > seminfo.semaem || tmp_aoe < -seminfo.semaem) {
		u.u_error = ERANGE;
		return(1);
	}
	uup->un_aoe = (short) tmp_aoe;
	if(uup->un_aoe == 0) {
		uup2 = &up->un_ent[--(up->un_cnt)];
		while(uup++ < uup2)
			*(uup - 1) = *uup;
		if(up->un_cnt == 0) {

			/* Remove process from undo list. */
			if(semunp == up)
				semunp = up->un_np;
			else
				for(up2 = semunp;up2 != NULL;up2 = up2->un_np)
					if(up2->un_np == up) {
						up2->un_np = up->un_np;
						break;
					}
			up->un_np = NULL;
		}
	}
	return(0);
}

/*
**	semconv - Convert user supplied semid into a ptr to the associated
**		semaphore header.
*/

struct semid_ds *
semconv(s)
register int	s;	/* semid */
{
	register struct semid_ds	*sp;	/* ptr to associated header */

	sp = &sema[s % seminfo.semmni];
	if((sp->sem_perm.mode & IPC_ALLOC) == 0 ||
#ifdef __hp9000s300
		(s<0) ||
#endif
		s / seminfo.semmni != sp->sem_perm.seq) {
		u.u_error = EINVAL;
		return(NULL);
	}
	return(sp);
}

/*
**	osemctl - Old semctl system call.
**		Retained for object code compatibility.
*/
osemctl()
{
	semctl1(OLD_SEMID_DS);
}

/*
**	semctl - Semctl system call.
*/
semctl()
{
	semctl1(NEW_SEMID_DS);
}


#ifdef SEMOP_PROTECTED
semctl1(semid_ds_type)
int semid_ds_type;	/* Is old or new semid_ds being used? */
{
	PSEMA(&pm_sema);
	__semctl1(semid_ds_type);
	VSEMA(&pm_sema);
}

__semctl1(semid_ds_type)
int semid_ds_type;	/* Is old or new semid_ds being used? */
#else /* Not SEMOP_PROTECTED */
/*
**	semctl1 - Main body of semctl system call.
*/

semctl1(semid_ds_type)
int semid_ds_type;	/* Is old or new semid_ds being used? */
#endif /* SEMOP_PROTECTED */

{
	register struct a {
		int		semid;
		uint		semnum;
		int		cmd;
		union semunion	*arg;
	}	*uap = (struct a *)u.u_ap;
	register struct	semid_ds	*sp;	/* ptr to semaphore header */
	register struct sem		*p;	/* ptr to semaphore */
	register int			i;	/* loop control */
	union semunion semtmp;

	if((sp = semconv(uap->semid)) == NULL)
		return;
	u.u_rval1 = 0;
	switch(uap->cmd) {

	/* Remove semaphore set. */
	case IPC_RMID:
		if(u.u_uid != sp->sem_perm.uid && u.u_uid != sp->sem_perm.cuid
			&& !suser())
			return;
		semunrm(uap->semid, 0, sp->sem_nsems);
		for(i = sp->sem_nsems, p = sp->sem_base;i--;p++) {
			p->semval = p->sempid = 0;
			if(p->semncnt) {
				wakeup((caddr_t)&p->semncnt);
				p->semncnt = 0;
			}
			if(p->semzcnt) {
				wakeup((caddr_t)&p->semzcnt);
				p->semzcnt = 0;
			}
		}
		ipcmfree(semmap, sp->sem_nsems, 
				(unsigned int)(sp->sem_base - sem) + 1);
		if(uap->semid + seminfo.semmni < 0)
			sp->sem_perm.seq = 0;
		else
			sp->sem_perm.seq++;
		sp->sem_perm.mode = 0;
		return;

	/* Set ownership and permissions. */
	case IPC_SET:
		if (semid_ds_type == NEW_SEMID_DS) {
			if(copyin((caddr_t) uap->arg, (caddr_t) &semtmp.ds,
				sizeof(semtmp.ds))) {
				u.u_error = EFAULT;
				return;
			}
		}
		else {
			if(copyin((caddr_t) uap->arg, (caddr_t) &semtmp.ods,
				sizeof(semtmp.ods))) {
				u.u_error = EFAULT;
				return;
			}
		}
		/* Check if the id was removed during copyin.     */
		/* If it was, leave u_error from semconv (EINVAL) */
		if(semconv(uap->semid) == NULL)
			return;
		/* test permission after copyin, in case permissions changed */
		if(u.u_uid != sp->sem_perm.uid && u.u_uid != sp->sem_perm.cuid
			 && !suser())
			return;

		if (semid_ds_type == NEW_SEMID_DS) {
			sp->sem_perm.uid = semtmp.ds.sem_perm.uid;
			sp->sem_perm.gid = semtmp.ds.sem_perm.gid;
			sp->sem_perm.mode = semtmp.ds.sem_perm.mode & 0777 |
				IPC_ALLOC;
		}
		else {
			sp->sem_perm.uid = (uid_t) semtmp.ods.sem_perm.uid;
			sp->sem_perm.gid = (gid_t) semtmp.ods.sem_perm.gid;
			sp->sem_perm.mode = semtmp.ods.sem_perm.mode & 0777 |
				IPC_ALLOC;
		}
		sp->sem_ctime = time;
		return;

	/* Get semaphore data structure. */
	case IPC_STAT:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			return;

		/* get a consistent snapshot before (virtual) copyout */
		if (semid_ds_type == NEW_SEMID_DS) {
			if (copyout((caddr_t) sp, (caddr_t) uap->arg,
				sizeof(*sp))) {
				u.u_error = EFAULT;
				return;
			}
		}
		else {
			/* Copy fields in the new semid_ds structure to
			 * the old semid_ds structure.  Note that there are
			 * old and new versions of struct ipc_perm.
			 * Therefore, we cannot simply do a structure to
			 * structure assignment.
			 */
			semtmp.ods.sem_perm.uid = (__ushort) sp->sem_perm.uid;
			semtmp.ods.sem_perm.gid = (__ushort) sp->sem_perm.gid;
			semtmp.ods.sem_perm.cuid = 
				(__ushort) sp->sem_perm.cuid;
			semtmp.ods.sem_perm.cgid = 
				(__ushort) sp->sem_perm.cgid;
			semtmp.ods.sem_perm.mode = sp->sem_perm.mode;
			semtmp.ods.sem_perm.seq = sp->sem_perm.seq;
			semtmp.ods.sem_perm.key = sp->sem_perm.key;
#ifdef __hp9000s800
			semtmp.ods.sem_perm.ndx = sp->sem_perm.ndx;
			semtmp.ods.sem_perm.wait = sp->sem_perm.wait;
#endif /* __hp9000s800 */
			semtmp.ods.sem_base = sp->sem_base;
			semtmp.ods.sem_nsems = sp->sem_nsems;
			semtmp.ods.sem_otime = sp->sem_otime;
			semtmp.ods.sem_ctime = sp->sem_ctime;

			if (copyout((caddr_t)&semtmp.ods, (caddr_t) uap->arg,
				sizeof(semtmp.ods))) {
				u.u_error = EFAULT;
				return;
			}
		}
		return;

	/* Get # of processes sleeping for greater semval. */
	case GETNCNT:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			return;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			return;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->semncnt;
		return;

	/* Get pid of last process to operate on semaphore. */
	case GETPID:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			return;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			return;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->sempid;
		return;

	/* Get semval of one semaphore. */
	case GETVAL:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			return;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			return;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->semval;
		return;

	/* Get all semvals in set. */
	case GETALL:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			return;
		/* get a consistent snapshot before (virtual) copyout */
		for(i = 0, p = sp->sem_base; i < sp->sem_nsems; i++, p++)
			semtmp.semvals[i] = p->semval;
		u.u_error = copyout((caddr_t)semtmp.semvals,
				    (caddr_t)uap->arg,
				    sizeof(semtmp.semvals[0]) * sp->sem_nsems);
		return;

	/* Get # of processes sleeping for semval to become zero. */
	case GETZCNT:
		if(ipcaccess(&sp->sem_perm, SEM_R))
			return;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			return;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->semzcnt;
		return;

	/* Set semval of one semaphore. */
	case SETVAL:
		if(ipcaccess(&sp->sem_perm, SEM_A))
			return;
		if(uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			return;
		}
		if((unsigned)uap->arg > seminfo.semvmx) {
			u.u_error = ERANGE;
			return;
		}
		if((p = sp->sem_base + uap->semnum)->semval = 
						(unsigned short) uap->arg) {
			if(p->semncnt) {
				p->semncnt = 0;
				wakeup((caddr_t)&p->semncnt);
			}
		} else
			if(p->semzcnt) {
				p->semzcnt = 0;
				wakeup((caddr_t)&p->semzcnt);
			}
		p->sempid = (unsigned short ) u.u_procp->p_pid;
		semunrm(uap->semid, (ushort)uap->semnum, (ushort)uap->semnum);
		return;

	/* Set semvals of all semaphores in set. */
	case SETALL:
		if (u.u_error = copyin((caddr_t)uap->arg,
				     (caddr_t)semtmp.semvals,
				     sizeof(semtmp.semvals[0]) * sp->sem_nsems))
			return;
		/* Check if the id was removed during copyin.     */
		/* If it was, leave u_error from semconv (EINVAL) */
		if(semconv(uap->semid) == NULL)
			return;
		/* test permission after copyin, in case permissions changed */
		if(ipcaccess(&sp->sem_perm, SEM_A))
			return;
		for(i = 0;i < sp->sem_nsems;)
			if(semtmp.semvals[i++] > seminfo.semvmx) {
				u.u_error = ERANGE;
				return;
			}
		semunrm(uap->semid, 0, sp->sem_nsems);
		for(i = 0, p = sp->sem_base;i < sp->sem_nsems;
			(p++)->sempid = (unsigned short) u.u_procp->p_pid) {
			if(p->semval = semtmp.semvals[i++]) {
				if(p->semncnt) {
					p->semncnt = 0;
					wakeup((caddr_t)&p->semncnt);
				}
			} else
				if(p->semzcnt) {
					p->semzcnt = 0;
					wakeup((caddr_t)&p->semzcnt);
				}
		}
		return;
	default:
		u.u_error = EINVAL;
		return;
	}
}

/*
**	semexit - Called by exit(kern_exit.c) to clean up on process exit.
*/

semexit()
{
	register struct sem_undo	*up,	/* process undo struct ptr */
					*p;	/* undo struct ptr */
	register struct semid_ds	*sp;	/* semid being undone ptr */
	register int			i;	/* loop control */
	register long			v;	/* adjusted value */
	register struct sem		*semp;	/* semaphore ptr */

	if((up = sem_undo[pindx(u.u_procp)]) == NULL)
		return;
	if(up->un_cnt == 0)
		goto cleanup;
	for(i = up->un_cnt;i--;) {
		if((sp = semconv(up->un_ent[i].un_id)) == NULL)
			continue;
		v = (long)(semp = sp->sem_base + up->un_ent[i].un_num)->semval +
			up->un_ent[i].un_aoe;
		if(v < 0 || v > seminfo.semvmx)
			continue;
		semp->semval = (unsigned short ) v;
		if(v == 0 && semp->semzcnt) {
			semp->semzcnt = 0;
			wakeup((caddr_t)&semp->semzcnt);
		}
		if(up->un_ent[i].un_aoe > 0 && semp->semncnt) {
			semp->semncnt = 0;
			wakeup((caddr_t)&semp->semncnt);
		}
	}
	up->un_cnt = 0;
	if(semunp == up)
		semunp = up->un_np;
	else
		for(p = semunp;p != NULL;p = p->un_np)
			if(p->un_np == up) {
				p->un_np = up->un_np;
				break;
			}
cleanup:
	up->un_np = semfup;
	semfup = up;
	sem_undo[pindx(u.u_procp)] = NULL;
}

/*
**	semget - Semget system call.
*/

#ifdef SEMOP_PROTECTED
semget()
{
	PSEMA(&pm_sema);
	__semget();
	VSEMA(&pm_sema);
}


__semget()	/* Takes the place of the original "semget" declaration */

#else /* Not SEMOP_PROTECTED */
semget()
#endif /* SEMOP_PROTECTED */
{
	register struct a {
		key_t	key;
		int	nsems;
		int	semflg;
	}	*uap = (struct a *)u.u_ap;
	register struct semid_ds	*sp;	/* semaphore header ptr */
	register int			i;	/* temp */
	int				s;	/* ipcgetperm status return */

	if((sp = ipcgetperm(uap->key, uap->semflg, (struct ipc_perm *)sema,
				seminfo.semmni, sizeof(*sp), &s)) == NULL)
		return;
	if(s) {

		/* This is a new semaphore set.  Finish initialization. */
		if(uap->nsems <= 0 || uap->nsems > seminfo.semmsl) {
			u.u_error = EINVAL;
			sp->sem_perm.mode = 0;
			return;
		}
		if((i = ipcmalloc(semmap, uap->nsems)) == NULL) {
			u.u_error = ENOSPC;
			sp->sem_perm.mode = 0;
			return;
		}
		sp->sem_base = sem + (i - 1);
		sp->sem_nsems = uap->nsems;
		sp->sem_ctime = time;

		/*
		 * Initialize semaphore values to zero.
		 */
		bzero(sp->sem_base, sp->sem_nsems * sizeof(*(sp->sem_base)));
		sp->sem_otime = 0;
	} else
		if(uap->nsems && sp->sem_nsems < uap->nsems) {
			u.u_error = EINVAL;
			return;
		}
	u.u_rval1 = sp->sem_perm.seq * seminfo.semmni + (sp - sema);
}

/*
**	seminit - Called by main(init_main.c) to initialize the semaphore map.
*/

seminit()
{
	register i;

	ipcmapinit(semmap, seminfo.semmap);
	ipcmfree(semmap, seminfo.semmns, 1);

	semfup = semu;
	for (i = 0; i < seminfo.semmnu - 1; i++) {
		semfup->un_np = (struct sem_undo *)((uint)semfup+seminfo.semusz);
		semfup = semfup->un_np;
	}
	semfup->un_np = NULL;
	semfup = semu;
}

/*
**	semop - Semop system call.
*/

#ifdef SEMOP_PROTECTED
semop()
{
	PSEMA(&pm_sema);
	__semop();
	VSEMA(&pm_sema);
}


__semop()	/* Takes the place of the original "semop" declaration */

#else /* Not SEMOP_PROTECTED */
semop()
#endif /* SEMOP_PROTECTED */
{
	register struct a {
		int		semid;
		struct sembuf	*sops;
		uint		nsops;
	}	*uap = (struct a *)u.u_ap;
	register struct sembuf		*op;	/* ptr to operation */
	register int			i;	/* loop control */
	register struct semid_ds	*sp;	/* ptr to associated header */
	register struct sem		*semp;	/* ptr to semaphore */
	int	again;
#ifdef	STACK_300_OVERFLOW_FIX
	struct sembuf			*semops;/* operation holding area pointer */
#else	/* STACK_300_OVERFLOW_FIX */
	struct sembuf		semops[SEMOPM];	/* operation holding area */
#endif	/* STACK_300_OVERFLOW_FIX */

#ifdef SAR
	semacnt++;	/* sar */
#endif
	if(uap->nsops > seminfo.semopm) {
		u.u_error = E2BIG;
		return;
	}
#ifdef	STACK_300_OVERFLOW_FIX
	/* get size of callers buffer */
	i =  uap->nsops * sizeof(*op);

	/* might sleep here */
	semops = (struct sembuf *)vapor_malloc(i, M_TEMP, M_WAITOK);
#endif	/* STACK_300_OVERFLOW_FIX */

	if (u.u_error = copyin((caddr_t)uap->sops, (caddr_t)semops,
			        uap->nsops * sizeof(*op)))
		return;

	/* semconv must follow copyin, as copyin is interruptable */
	if((sp = semconv(uap->semid)) == NULL)
		return;

	/* Verify that sem #s are in range and permissions are granted. */
	for(i = 0, op = semops;i++ < uap->nsops;op++) {
		if(ipcaccess(&sp->sem_perm, op->sem_op ? SEM_A : SEM_R))
			return;
		if(op->sem_num >= sp->sem_nsems) {
			u.u_error = EFBIG;
			return;
		}
	}
	again = 0;
check:
	/* Loop waiting for the operations to be satisified atomically. */
	/* Actually, do the operations and undo them if a wait is needed
		or an error is detected. */
	if (again) {
		/* Verify that the semaphores haven't been removed. */
		if(semconv(uap->semid) == NULL) {
			u.u_error = EIDRM;
			return;
		}
		/* no need to copy in user operation list after sleep */
		/* since it is kept in local variable.                */
	}
	again = 1;

	for(i = 0, op = semops;i < uap->nsops;i++, op++) {
		semp = sp->sem_base + op->sem_num;
		if(op->sem_op > 0) {
			if(op->sem_op + (long)semp->semval > seminfo.semvmx ||
				(op->sem_flg & SEM_UNDO &&
				semaoe(op->sem_op, uap->semid, op->sem_num))) {
				if(u.u_error == 0)
					u.u_error = ERANGE;
				if(i)
					semundo(semops, i, uap->semid, sp);
				return;
			}
			semp->semval += op->sem_op;
			if(semp->semncnt) {
				semp->semncnt = 0;
				wakeup((caddr_t)&semp->semncnt);
			}
			continue;
		}
		if(op->sem_op < 0) {
			if(semp->semval >= -(op->sem_op))
			{
				if(op->sem_flg & SEM_UNDO &&
					semaoe(op->sem_op, uap->semid, op->sem_num)) {
					if(i)
						semundo(semops, i, uap->semid, sp);
					return;
				}
				semp->semval += op->sem_op;
				if(semp->semval == 0 && semp->semzcnt) {
					semp->semzcnt = 0;
					wakeup((caddr_t)&semp->semzcnt);
				}
				continue;
			}
			if(i)
				semundo(semops, i, uap->semid, sp);
			if(op->sem_flg & IPC_NOWAIT) {
				u.u_error = EAGAIN;
				return;
			}
			semp->semncnt++;
			if(sleep(&semp->semncnt, PCATCH | PSEMN)) {
				if((semp->semncnt)-- <= 1) {
					semp->semncnt = 0;
					wakeup((caddr_t)&semp->semncnt);
				}
				u.u_error = EINTR;
				return;
			}
			goto check;
		}
		if(semp->semval) {
			if(i)
				semundo(semops, i, uap->semid, sp);
			if(op->sem_flg & IPC_NOWAIT) {
				u.u_error = EAGAIN;
				return;
			}
			semp->semzcnt++;
			if(sleep(&semp->semzcnt, PCATCH | PSEMZ)) {
				if((semp->semzcnt)-- <= 1) {
					semp->semzcnt = 0;
					wakeup((caddr_t)&semp->semzcnt);
				}
				u.u_error = EINTR;
				return;
			}
			goto check;
		}
	}

	/* All operations succeeded.  Update sempid for accessed semaphores. */
	for(i = 0, op = semops;i++ < uap->nsops;
		(sp->sem_base + (op++)->sem_num)->sempid = 
		(unsigned short ) u.u_procp->p_pid);
	sp->sem_otime = time;
	u.u_rval1 = 0;
}

/*
**	semundo - Undo work done up to finding an operation that can't be done.
*/

semundo(op, n, id, sp)
register struct sembuf		*op;	/* first operation that was done ptr */
register int			n,	/* # of operations that were done */
				id;	/* semaphore id */
register struct semid_ds	*sp;	/* semaphore data structure ptr */
{
	register struct sem	*semp;	/* semaphore ptr */

	for(op += n - 1;n--;op--) {
		if(op->sem_op == 0)
			continue;
		semp = sp->sem_base + op->sem_num;
		semp->semval -= op->sem_op;
		if(op->sem_flg & SEM_UNDO)
			semaoe(-(op->sem_op), id, op->sem_num);
	}
}

/*
**	semunrm - Undo entry remover.
**
**	This routine is called to clear all undo entries for a set of semaphores
**	that are being removed from the system or are being reset by SETVAL or
**	SETVALS commands to semctl.
*/

semunrm(id, low, high)
int	id;	/* semid */
ushort	low,	/* lowest semaphore being changed */
	high;	/* highest semaphore being changed */
{
	register struct sem_undo	*pp,	/* ptr to predecessor to p */
					*p;	/* ptr to current entry */
	register struct undo		*up;	/* ptr to undo entry */
	register int			i,	/* loop control */
					j;	/* loop control */

	pp = NULL;
	p = semunp;
	while(p != NULL) {

		/* Search through current structure for matching entries. */
		for(up = p->un_ent, i = 0;i < p->un_cnt;) {
			if(id < up->un_id)
				break;
			if(id > up->un_id || low > up->un_num) {
				up++;
				i++;
				continue;
			}
			if(high < up->un_num)
				break;
			for(j = i;++j < p->un_cnt;
				p->un_ent[j - 1] = p->un_ent[j]);
			p->un_cnt--;
		}

		/* Reset pointers for next round. */
		if(p->un_cnt == 0)

			/* Remove from linked list. */
			if(pp == NULL) {
				semunp = p->un_np;
				p->un_np = NULL;
				p = semunp;
			} else {
				pp->un_np = p->un_np;
				p->un_np = NULL;
				p = pp->un_np;
			}
		else {
			pp = p;
			p = p->un_np;
		}
	}
}
