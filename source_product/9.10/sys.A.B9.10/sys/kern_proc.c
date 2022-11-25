/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_proc.c,v $
 * $Revision: 1.24.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:06:32 $
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

#include "../machine/reg.h"
#ifdef __hp9000s300
#include "../machine/pte.h"
#endif
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/acct.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/mbuf.h"

#ifdef BSDJOBCTL
#include "../h/tty.h"
#endif

/* This is like spgrp() except it assumes npgrp == -1 (job control) */
/* It also uses PERFORM_MEM speedups. */

spgrp2(top)
	register struct proc *top;
{
	register struct proc *p;
	int f = 0;
	int s;

	for (p = top; ; ) {
#define	bit(a)	(1<<(a-1))
		/* Avoid hazard; p_sig changed in powerfail */
		s = spl7();
		p->p_sig &= ~(bit(SIGTSTP)|bit(SIGTTIN)|bit(SIGTTOU));
		splx(s);
		f++;
		/*
		 * Search for children.
		 */
		if ((p->p_cptr) && (p->p_cptr->p_pptr == p)) {
			p = p->p_cptr;
			goto cont;
		}
		/*
		 * Search for siblings.
		 */
		for (; p != top; p = p->p_pptr)
			if (p->p_osptr) {
				if (p->p_osptr->p_pptr != p->p_pptr)
					panic("spgrp2: sibling chain botch");
				p = p->p_osptr;
				goto cont;
			}
		break;
	cont:
		;
	}
	return (f);
}

/*
 * Is p an inferior of the current process?
 */
inferior(p)
	register struct proc *p;
{

#ifdef MP
	/*
	 * Although sched_lock does not officially lock p_pptr and
	 * 	p_ppid, these fields cannot change without locking
	 *	sched_lock along the way.
	 */
#endif
	SPINLOCK(sched_lock);
	for (; p != u.u_procp; p = p->p_pptr)
		if (p->p_ppid == 0){
			SPINUNLOCK(sched_lock);
			return (0);
		}

	SPINUNLOCK(sched_lock);
	return (1);
}

struct proc *
pfind(pid)
	int pid;
{
	register struct proc *p;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	/* Since PIDHASH() is a modulo operation, it retains the sign */
	/* of the pid.  Thus we must filter out the negative pids     */
	SPINLOCKX(sched_lock,&nsp);
	if (pid > 0){
		for (p = &proc[pidhash[PIDHASH(pid)]]; p != &proc[0]; p = &proc[p->p_idhash])
			if (p->p_pid == pid){
				/* Don't unspinlock the sched_lock */
				return (p);
			}
	}
	else if (pid == 0) {
		/* The above algorithm does not work for proc 0.  If the
		 * pid was 0, return &proc[0].
		 */
		VASSERT(proc[0].p_pid == 0);
		return (&proc[0]);
	}
	SPINUNLOCKX(sched_lock,&nsp);
	return ((struct proc *)0);
}


struct proc *		/* find a process that is in pgrp pgid */
pgrpfind(pgid)
	int pgid;
{
	register struct proc *p;

	if (pgid > 0)
		for (p = &proc[pgrphash[PGRPHASH(pgid)]]; p != &proc[0]; p = &proc[p->p_pgrphx])
			if (p->p_pgrp == pgid)
				return (p);
	return ((struct proc *)0);
}

/*
 * Routines to search through pid, pgrp, sid, uid hash chains,
 * as well as to unlink them.
 */
premove(tp)
	register struct proc *tp;
{
	register struct proc *p, *lp;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	SPINLOCKX(sched_lock,&nsp);

	for ( lp = p = &proc[pidhash[PIDHASH(tp->p_pid)]]; p != &proc[0];
	      p = &proc[p->p_idhash]){
		if (tp == p){
			if (lp == p)
				pidhash[PIDHASH(tp->p_pid)] = p->p_idhash;
			else
				lp->p_idhash = p->p_idhash;
			SPINUNLOCKX(sched_lock,&nsp);
			return (1);
		}
		lp = p;
	}
	SPINUNLOCKX(sched_lock,&nsp);
	return (0);
}


gremove(tp)
	register struct proc *tp;
{
	register struct proc *p, *lp;
	register int s;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	s = UP_SPL6();	/* mux driver can try to change pgrp's on the ICS ... */

	SPINLOCKX(sched_lock,&nsp);

	for ( lp = p = &proc[pgrphash[PGRPHASH(tp->p_pgrp)]]; p != &proc[0];
	      p = &proc[p->p_pgrphx]){
		if (tp == p){
			if (lp == p)
				pgrphash[PGRPHASH(tp->p_pgrp)] = p->p_pgrphx;
			else
				lp->p_pgrphx = p->p_pgrphx;
			
			SPINUNLOCKX(sched_lock,&nsp);
			UP_SPLX(s);
			return (1);
		}
		lp = p;
	}
	SPINUNLOCKX(sched_lock,&nsp);
	UP_SPLX(s);
	return (0);
}

/*
 * gchange(): Atomically changes the process group of a process.  The
 *            process is moved from its current pgrp hash chain to the
 *            proper one for the new group.
 */
gchange(p, pgrp)
	register struct proc *p;
	register short pgrp;
{
	register int s;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	s = UP_SPL6();
	SPINLOCKX(sched_lock,&nsp);
	if (!gremove(p)) {
		SPINUNLOCKX(sched_lock,&nsp);
		UP_SPLX(s);
		return 0;
	}
	p->p_pgrp = pgrp;
	glink(p, pgrp);
	SPINUNLOCKX(sched_lock,&nsp);
	UP_SPLX(s);
	return 1;
}


sremove(tp)		/* remove a process from its session hash chain */
	register struct proc *tp;
{
	register struct proc *p, *lp;
	register int s;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	s = UP_SPL6();	/* mux driver can try to change pgrp's on the ICS ... */
	SPINLOCKX(sched_lock, &nsp);
	for ( lp = p = &proc[sidhash[SIDHASH(tp->p_sid)]]; p != &proc[0];
	      p = &proc[p->p_sidhx]){
		if (tp == p){
			if (lp == p)
				sidhash[SIDHASH(tp->p_sid)] = p->p_sidhx;
			else
				lp->p_sidhx = p->p_sidhx;
			
				
			SPINUNLOCKX(sched_lock, &nsp);
			UP_SPLX(s);
			return (1);
		}
		lp = p;
	}
	SPINUNLOCKX(sched_lock, &nsp);
	UP_SPLX(s);
	return (0);
}

/*
 * schange(): Atomically changes the session ID of a process.  The
 *            process is moved from its current session hash chain to the
 *            proper one for the new session.
 */
schange(p, sid)
	register struct proc *p;
	register sid_t sid;
{
	register int s;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	s = UP_SPL6();
	SPINLOCKX(sched_lock, &nsp);
	if (!sremove(p)) {
		SPINUNLOCKX(sched_lock, &nsp);
		UP_SPLX(s);
		return 0;
	}
	p->p_sid = sid;
	slink(p, sid);
	SPINUNLOCKX(sched_lock, &nsp);
	UP_SPLX(s);
	return 1;
}


uremove(tp)
	register struct proc *tp;
{
	register struct proc *p, *lp;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	SPINLOCKX(sched_lock,&nsp);

	for ( lp = p = &proc[uidhash[UIDHASH(tp->p_uid)]]; p != &proc[0];
	      p = &proc[p->p_uidhx]){
		if (tp == p){
			if (lp == p)
				uidhash[UIDHASH(tp->p_uid)] = p->p_uidhx;
			else
				lp->p_uidhx = p->p_uidhx;
			SPINUNLOCKX(sched_lock,&nsp);
			return (1);
		}
		lp = p;
	}

	SPINUNLOCKX(sched_lock,&nsp);
	return (0);
}
