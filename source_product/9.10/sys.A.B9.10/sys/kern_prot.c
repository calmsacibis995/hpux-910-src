/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_prot.c,v $
 * $Revision: 1.30.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:06:40 $
 */

/*	kern_prot.c	6.1	83/07/29	*/

/* HPUX_ID: @(#)kern_prot.c	55.1		88/12/23 */

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
 * System calls related to processes and protection
 */

#include "../machine/reg.h"

#include "../h/types.h"
#include "../h/param.h"
#include "../h/systm.h"
#ifdef hp9000s800
#include "../h/dir.h"
#endif hp9000s800
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#ifdef hp9000s800
#include "../ufs/inode.h"
#endif hp9000s800
#include "../h/proc.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/acct.h"
#ifdef hp9000s800
#include "../h/mount.h"
#endif hp9000s800
#include "../ufs/quota.h"
#ifdef AUDIT
#include "../h/audit.h"
#endif AUDIT

getpid()
{

	u.u_r.r_val1 = u.u_procp->p_pid;
	u.u_r.r_val2 = u.u_procp->p_ppid;

}

#ifdef	BSDJOBCTL
getpgrp2()
#else	not BSDJOBCTL
getpgrp()
#endif	not BSDJOBCTL
{
	register struct a {
		pid_t	pid;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	if (uap->pid == 0)
		uap->pid = u.u_procp->p_pid;
	p = pfind((int)uap->pid);	/* locks sched_lock, if successful */
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
#ifdef	BSDJOBCTL
	/* check for permission to ask about this pid */
	if (p->p_sid != u.u_procp->p_sid) {
		SPINUNLOCK(sched_lock);
		u.u_error = EPERM;
		return;
	}
#endif	BSDJOBCTL
	u.u_r.r_val1 = p->p_pgrp;
	SPINUNLOCK(sched_lock);
}

getuid()
{

	u.u_r.r_val1 = u.u_ruid;
	u.u_r.r_val2 = u.u_uid;
}

getgid()
{

	u.u_r.r_val1 = u.u_rgid;
	u.u_r.r_val2 = u.u_gid;
}

getgroups()
{
	register struct	a {
		int	gidsetsize;
		gid_t	*gidset;
	} *uap = (struct a *)u.u_ap;
	register gid_t *gp;

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--)
		if (gp[-1] != NOGROUP)
			break;
	if (uap->gidsetsize == 0) {
		u.u_r.r_val1 = gp-u.u_groups;
		return;
	}
	if (uap->gidsetsize < gp - u.u_groups) {
		u.u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u.u_groups;
	u.u_error = copyout((caddr_t)u.u_groups, (caddr_t)uap->gidset,
	    uap->gidsetsize * sizeof (u.u_groups[0]));
	if (u.u_error)
		return;
	u.u_r.r_val1 = uap->gidsetsize;
}

#ifdef	BSDJOBCTL
setpgrp2()	/* the Posix function setpgid() also uses the same routine */
#else
setpgrp()
#endif
{
	register struct proc *p,*q;
	register struct a {
		pid_t	pid;
		pid_t	pgrp;
	} *uap = (struct a *)u.u_ap;

	if (uap->pid == 0)	/* pid == 0, use pid of calling process */
		uap->pid = u.u_procp->p_pid;
	if (uap->pgrp == 0)	/* pgrp == 0, use pid of the indicated process*/
		uap->pgrp = uap->pid;

	if (uap->pgrp <= 0 || uap->pgrp >= MAXPID) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->pid == u.u_procp->p_pid) {
		p = u.u_procp;
		SPINLOCK(sched_lock);
	} else {
		p = pfind((int)uap->pid); /* aquires sched_lock if successful */
		if ((p == 0) || (p->p_ppid != u.u_procp->p_pid)){
			if(p)
				SPINUNLOCK(sched_lock);
			u.u_error = ESRCH;
			return;
		}
		else { /* trying to change the pgrp of a child process */
			if (p->p_flag2 & S2EXEC) {	/* child has exec'ed */
				SPINUNLOCK(sched_lock);
				u.u_error = EACCES;
				return;
			}
			if (p->p_sid != u.u_procp->p_sid){  /* child in 
							    different session */
				SPINUNLOCK(sched_lock);
				u.u_error = EPERM;
				return;
			}
		}
	}
/*
 * S300 used to have this #ifdef starting after cont_setpgid: label.
 * I am resolving in favor of S800.
 */
#ifdef	BSDJOBCTL
	if (p->p_sid == p->p_pid) {	/* the process is a session leader */
		SPINUNLOCK(sched_lock);
		u.u_error = EPERM;
		return;
	}

	if (p->p_pid != uap->pgrp) {
		for (q = &proc[sidhash[SIDHASH(u.u_procp->p_sid)]]; 
		     q != &proc[0]; q = &proc[q->p_sidhx])
			if ((q->p_pgrp == uap->pgrp) && 
			    (q->p_sid == u.u_procp->p_sid))
				goto cont_setpgid;
		SPINUNLOCK(sched_lock);
		u.u_error = EPERM;
		return;
	}

cont_setpgid:

#else	not BSDJOBCTL
/* need better control mechanisms for process groups */
	if (p->p_uid != u.u_uid && u.u_uid && !inferior(p)) {
		SPINUNLOCK(sched_lock);
		u.u_error = EPERM;
		return;
	}
#endif	not BSDJOBCTL
	/* unlink old pgrp, and insert new into chain. */

	/* spl6 not necessary here because gchange() does it */
	if (!gchange(p, (short)uap->pgrp)) {
		SPINUNLOCK(sched_lock); 
		panic("setpgrp2: lost pgrphx");
	}
	SPINUNLOCK(sched_lock); 
}

#ifdef BSDJOBCTL

/* Verify whether the specified process group pgrp is in the same session
 * as the current process.  pgrpcheck is used by the tty driver to determine
 * if the tty fg process group should be set to pgrp.
 *
 * Returns zero if authorized, else it returns an error code suitable
 * for returning in u.u_error.
 */

int
pgrpcheck(pgrp)
pid_t pgrp;
{
#ifdef MP
	sv_lock_t nsp;
#endif MP
	register struct proc *p;

	/* range check for valid pgrp */
	if (pgrp <= 0 || pgrp >= MAXPID || pgrp == PGID_NOT_SET)
		return (EINVAL);
	if (u.u_procp->p_sid == SID_NOT_SET){	/* if system process, let it */
		return (0);			/* set the tty */
	}

	SPINLOCKX(sched_lock,&nsp);
	for (p = &proc[pgrphash[PGRPHASH(pgrp)]]; p != &proc[0]; p = &proc[p->p_pgrphx]) {
		if (p->p_pgrp != pgrp)
			continue;
		if (p->p_sid != u.u_procp->p_sid) {
			SPINUNLOCKX(sched_lock,&nsp);
			return (EPERM);
		} else {
			SPINUNLOCKX(sched_lock,&nsp);
			return (0);
		}
	}
	SPINUNLOCKX(sched_lock,&nsp);
	return (EPERM);		/* pre 7.0 csh needs this to be zero */
				/* however, Posix requires  EPERM */
}

#endif BSDJOBCTL

#ifdef notdef			/* BSD call not in HP-UX */
#ifdef MP
	Does not work with MP
#endif MP
setreuid()
{
	struct a {
		uid_t	ruid;
		uid_t	euid;
	} *uap;
	register uid_t ruid, euid;
#ifdef AUDIT
	register struct proc *p = u.u_procp;
#endif

	uap = (struct a *)u.u_ap;
	ruid = uap->ruid;
	if (ruid == -1)
		ruid = u.u_ruid;
	if (u.u_ruid != ruid && u.u_uid != ruid && !suser())
		return;
	euid = uap->euid;
	if (euid == -1)
		euid = u.u_uid;
	if (u.u_ruid != euid && u.u_uid != euid && !suser())
		return;
	/*
	 * Everything's okay, do it.
	 */
#ifdef AUDIT
	p->p_idwrite = 0;		/* clear flag */
#endif
	u.u_cred = crcopy(u.u_cred);
	u.u_procp->p_uid = ruid;
	u.u_ruid = ruid;
	u.u_uid = euid;
}

setregid()
{
	register struct a {
		gid_t	rgid;
		gid_t	egid;
	} *uap;
	register gid_t rgid, egid;
	int s;
#ifdef AUDIT
	register struct proc *p = u.u_procp;
#endif

	uap = (struct a *)u.u_ap;
	rgid = uap->rgid;
	if (rgid == -1)
		rgid = u.u_rgid;
	if (u.u_rgid != rgid && u.u_gid != rgid && !suser())
		return;
	egid = uap->egid;
	if (egid == -1)
		egid = u.u_gid;
	if (u.u_rgid != egid && u.u_gid != egid && !suser())
		return;
#ifdef AUDIT
	p->p_idwrite = 0;	/* clear flag */
#endif
	u.u_cred = crcopy(u.u_cred);
	if (u.u_rgid != rgid) {
		leavegroup(u.u_rgid);
		(void) entergroup(rgid);
		u.u_rgid = rgid;
	}

	/*
	 * For privileged groups need to re-compute privilege mask.
	 */
	SPL_REPLACEMENT(sched_lock,spl6,s);
	u.u_procp->p_flag |= SPRIV;
	SPLX_REPLACEMENT(sched_lock, s);

	u.u_gid = egid;
}
#endif notdef			/* BSD syscall not in HPUX */


setresuid()
{
	struct a {
		uid_t	ruid;
		uid_t	euid;
		uid_t	suid;
	} *uap;
	register uid_t ruid, euid, suid;
	register struct proc *p = u.u_procp;

	uap = (struct a *)u.u_ap;
	ruid = uap->ruid;
	euid = uap->euid;
	suid = uap->suid;
	if ((ruid < 0 && ruid != UID_NO_CHANGE) || (ruid >= MAXUID) ||
	    (euid < 0 && euid != UID_NO_CHANGE) || (euid >= MAXUID) ||
	    (suid < 0 && suid != UID_NO_CHANGE) || (suid >= MAXUID)) {
		u.u_error = EINVAL;
		return;
	}
	if (ruid == UID_NO_CHANGE)
		ruid = u.u_ruid;
	if (u.u_ruid != ruid && u.u_uid != ruid && p->p_suid != ruid 
		&& !resuser())
		return;
	if (euid == UID_NO_CHANGE)
		euid = u.u_uid;
	if (u.u_ruid != euid && u.u_uid != euid && p->p_suid != euid
		&& !resuser())
		return;
	if (suid == UID_NO_CHANGE)
		suid = p->p_suid;
	if (u.u_ruid != suid && u.u_uid != suid && p->p_suid != suid
		&& !resuser())
		return;
	/*
	 * Everything's okay, do it.
	 */
#ifdef AUDIT
	p->p_idwrite = 0;		/* clear flag */
#endif AUDIT
	u.u_cred = crcopy(u.u_cred);
	/* unlink old uid, and add new into the chain */
	if (!(uremove(p)))
		panic("setresuid: lost uidhx");
	SPINLOCK(sched_lock);
	ulink(p,ruid);
	p->p_uid = ruid;
	u.u_ruid = ruid;
	u.u_uid = euid;
	p->p_suid = suid;
	SPINUNLOCK(sched_lock);
}

setresgid()
{
	register struct a {
		gid_t	rgid;
		gid_t	egid;
		gid_t 	sgid;
	} *uap;
	register gid_t rgid, egid, sgid;
#ifdef AUDIT
	register struct proc *p = u.u_procp;
#endif AUDIT
	int s;

	uap = (struct a *)u.u_ap;
	rgid = uap->rgid;
	egid = uap->egid;
	sgid = uap->sgid;
	if ((rgid < 0 && rgid != GID_NO_CHANGE) || (rgid >= MAXUID) ||
	    (egid < 0 && egid != GID_NO_CHANGE) || (egid >= MAXUID) ||
	    (sgid < 0 && sgid != GID_NO_CHANGE) || (sgid >= MAXUID)) {
		u.u_error = EINVAL;
		return;
	}
	if (rgid == GID_NO_CHANGE)
		rgid = u.u_rgid;
	if (u.u_rgid != rgid && u.u_gid != rgid && u.u_sgid != rgid
		&& !resuser())
		return;
	if (egid == GID_NO_CHANGE)
		egid = u.u_gid;
	if (u.u_rgid != egid && u.u_gid != egid && u.u_sgid != egid
		&& !resuser())
		return;
	if (sgid == GID_NO_CHANGE)
		sgid = u.u_sgid;
	if (u.u_rgid != sgid && u.u_gid != sgid && u.u_sgid != sgid
		&& !resuser())
		return;
	u.u_cred = crcopy(u.u_cred);
	u.u_gid = egid;
	u.u_sgid = sgid;
	u.u_rgid = rgid;

	/*
	 * For privileged groups need to re-compute privilege mask.
	 */
	SPL_REPLACEMENT(sched_lock,spl6,s);
	u.u_procp->p_flag |= SPRIV;
	SPLX_REPLACEMENT(sched_lock, s);

#ifdef AUDIT
	p->p_idwrite = 0;	/* clear flag */
#endif AUDIT
}
/******************************************/

setgroups()
{
	register struct	a {
		int	gidsetsize;
		gid_t	*gidset;
	} *uap = (struct a *)u.u_ap;
	register gid_t *gp;
	register gid_t *ngp;
	struct ucred *newcr, *tmpcr;
	int s;


	if (uap->gidsetsize > sizeof (u.u_groups) / sizeof (u.u_groups[0])) {
		u.u_error = EINVAL;
		return;
	}
	newcr = crdup(u.u_cred);
	u.u_error = copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    uap->gidsetsize * sizeof (newcr->cr_groups[0]));
#ifdef AUDIT
	if (AUDITEVERON())
		save_iarray((char *)newcr->cr_groups,
				uap->gidsetsize * sizeof(newcr->cr_groups[0]));
#endif AUDIT
	if (u.u_error){
		crfree(newcr);
		return;
	}
	
	/*
	 * Check to see if user is adding new groups, if so, he must
	 * be super user.
	 */
	    for (  ngp = &newcr -> cr_groups[0]
		 ; ngp < &newcr -> cr_groups[uap->gidsetsize]
		 ; ngp++)
	    {
		if(*ngp >= MAXUID){
			u.u_error = EINVAL;
			crfree(newcr);
			return;
		}	
		if(!groupmember(*ngp) && !suser()) {
			crfree(newcr);
		    	return;
		}
	    }

	/* Don't do a bcopy here as it will wreck file system access semantics
	 * over NFS.  Since an "open" file over NFS is not really open, but 
	 * the credential structure holds the access rights for the file.  To
	 * hold UNIX file system semantics a read after an open must work if 
	 * the open succeded.  Therefore, the groups in the credential structure
	 * must remain the same for all old file "opens".  So we must copy the
	 * credential structure to a new one, and change that one, leaving the
	 * original be.
	 */
	tmpcr = u.u_cred;
	u.u_cred = newcr;
	crfree(tmpcr);

	for (gp = &u.u_groups[uap->gidsetsize]; gp < &u.u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;

	/*
	 * For privileged groups need to re-compute privilege mask.
	 */
	SPL_REPLACEMENT(sched_lock, spl6, s);
	u.u_procp->p_flag |= SPRIV;
	SPLX_REPLACEMENT(sched_lock, s);
}

/*
 * Group utility functions.
 */


/*
 * Check if gid is a member of the group set.
 * NOTE: Previous calls to in_u_groups() have been redirected here and
 *       in_u_groups() deleted.
 */
groupmember(gid)
	gid_t gid;
{
	register gid_t *gp;

	if (u.u_gid == gid)
		return (1);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

#ifdef notdef		/* BSD code to support setre[ug]id */
/*
 * Delete gid from the group set.
 */
leavegroup(gid)
	gid_t gid;
{
	register gid_t *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			goto found;
	return;
found:
	for (; gp < &u.u_groups[NGROUPS-1]; gp++)
		*gp = *(gp+1);
	*gp = NOGROUP;
}

/*
 * Add gid to the group set.
 */
entergroup(gid)
	gid_t gid;
{
	register gid_t *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			return (0);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp < 0) {
			*gp = gid;
			return (0);
		}
	return (-1);
}
#endif notdef		/* BSD code to support setre[ug]id */


/*
 * Routines to allocate and free credentials structures
 */

int cractive = 0;

struct credlist {
	union {
		struct ucred cru_cred;
		struct credlist *cru_next;
	} cl_U;
#define		cl_cred cl_U.cru_cred
#define		cl_next cl_U.cru_next
};

struct credlist *crfreelist = NULL;


/*
 * Allocate a zeroed cred structure and crhold it.
 */
#ifdef MP
/*
 * Changes here were needed to protect crfreelist,etc.
 *
 * Future: Get rid of the spl6().
 */
#endif MP
struct ucred *
crget()
{
	register struct ucred *cr;
	register s;

	s = UP_SPL6();
	SPINLOCK(cred_lock);
	if (crfreelist) {
		cr = &crfreelist->cl_cred;
		crfreelist = ((struct credlist *)cr)->cl_next;
	} else {
		SPINUNLOCK(cred_lock);
		cr = (struct ucred *)kmem_alloc((u_int)sizeof(*cr));
		SPINLOCK(cred_lock);
	}
	(void) UP_SPLX(s);
	bzero((caddr_t)cr, (u_int)sizeof(*cr));
	/* crhold(cr);*/
	cr->cr_ref++;
	cractive++;
	SPINUNLOCK(cred_lock);
	return(cr);
}


/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
#ifdef MP
/*
 * Uses cred_lock to lock manipulations of the credentials structure, and
 * the credentials free list and active count.
 */
#endif MP
crfree(cr)
	struct ucred *cr;
{
	register s;
	
	s = UP_SPL6();
	SPINLOCK(cred_lock);

	/*
	 * check to see if we are freeing a structure that's
	 * already been freed.  If we allow this to happen, we will
	 * corrupt the free list
	 */
	if (cr->cr_ref <= 0) {
		printf("crfree(0x%08x) : bad cr_ref %d\n", cr, cr->cr_ref);

		if (cr->cr_ref >= -1)
			panic("crfree: freeing free credential struct");
		else
			panic("crfree: credential reference overflow");
	}

	if (--cr->cr_ref != 0) {
		(void) UP_SPLX(s);
		SPINUNLOCK(cred_lock);
		return;
	}
	((struct credlist *)cr)->cl_next = crfreelist;
	crfreelist = (struct credlist *)cr;
	cractive--;
	SPINUNLOCK(cred_lock);
	(void) UP_SPLX(s);
}


/*
 * Copy cred structure to a new one and free the old one.
 */
#ifdef MP
/*
 * No changes are required to crcopy() for MP.  The only possible
 * problem would be if someone else were changing the cred structure
 * while we were copying it.  This should never happen.  No one else
 * has access to the credentials structure until we return a pointer
 * to it.
 */
#endif MP
struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	crfree(cr);
	newcr->cr_ref = 1;
	return(newcr);
}

/*
 * Dup cred struct to a new held one.
 */
#ifdef MP
/* 
 * No changes required for MP. See crcopy() explanation.
 */
#endif MP
struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
	return(newcr);
}

/*
 * Test if the current user is the
 * super user.
 */
suser()
{

	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}
resuser()
{
        register struct proc *p = u.u_procp;
        if (u.u_uid == 0 || u.u_ruid == 0 || p->p_suid == 0) {
                u.u_acflag |= ASU;
                return (1);
        }
        u.u_error = EPERM;
        return (0);
}
 
#if defined(TRUX) && defined(B1)
/*
 * Stub routines for B1 privilege checking routines until the regions/B1
 * merge.  marden.
 */
privileged(priv, error)
int priv, error;
{
	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	if (error != 0)
		u.u_error = EPERM;
	return(0);
}

privileged_uid(priv, uid)
int priv, uid;
{
	if (uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	return(0);
}
#endif /* TRUX && B1 */
