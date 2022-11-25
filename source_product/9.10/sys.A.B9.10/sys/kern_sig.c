/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_sig.c,v $
 * $Revision: 1.62.83.6 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/03/09 15:44:14 $
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

#include "../h/debug.h"
#include "../machine/reg.h"
#ifdef __hp9000s300
#include "../machine/pte.h"
#endif
#include "../machine/psl.h"
#ifdef hp9000s800
#include "../h/types.h"
#endif
#include "../h/param.h"
#ifdef hp9000s800
#include "../h/sysmacros.h"
#endif
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/conf.h"
#include "../h/buf.h"
#ifdef hp9000s800
#include "../h/mnttab.h"
#endif
#include "../h/vm.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/ptrace.h"
#ifdef hp9000s800
#include "../h/fcntl.h"
#include "../h/vas.h"
#endif

#include "../h/kern_sem.h"

#include "../h/file.h"
#include "../ufs/inode.h"
#include "../dux/lookupops.h"

#define mask(sig)	(1<<(sig-1))
#define	cantmask	(mask(SIGKILL)|mask(SIGSTOP))
#define	stops	(mask(SIGSTOP)|mask(SIGTSTP)|mask(SIGTTIN)|mask(SIGTTOU))

/* for DIL */
#ifdef _WSIO
#define	discards	(mask(SIGCONT)|mask(SIGIO)|mask(SIGURG)|mask(SIGCLD)|mask(SIGPWR)|mask(SIGWINCH)|mask(SIGDIL))
#else
#define	discards	(mask(SIGCONT)|mask(SIGIO)|mask(SIGURG)|mask(SIGCLD)|mask(SIGPWR)|mask(SIGWINCH))
#endif


sigvec()
{
	register struct a {
		int	signo;
		struct	sigvec *nsv;
		struct	sigvec *osv;
	} *uap = (struct a  *)u.u_ap;
	struct sigvec vec;
	struct sigvec ovec;
	register int sig;
	register int bit;
	register void (*sig_handler)();
	register int sig_flags;
	int sig_mask;

	sig = uap->signo;
	bit = mask(sig);
#ifdef	SIGRESERVE
	if (sig <= 0 || sig >= NUSERSIG || sig >= NSIG || sig == SIGRESERVE) {
#else
	if (sig <= 0 || sig >= NUSERSIG || sig >= NSIG) {
#endif
		u.u_error = EINVAL;
		return;
	}
	/* read new vector first, so it isn't overwritten by old vector */
	if (uap->nsv) {
		u.u_error =
		    copyin((caddr_t)uap->nsv, (caddr_t)&vec, sizeof (vec));
		if (u.u_error)
			return;
		sig_handler = vec.sv_handler;
		sig_mask = vec.sv_mask;
		sig_flags = vec.sv_flags;

		if (sig_handler!=SIG_DFL && (sig==SIGKILL || sig==SIGSTOP)) {
			u.u_error = EINVAL;
			return;
		}
	}
	if (uap->osv) {
		ovec.sv_handler = u.u_signal[sig-1];
		ovec.sv_mask = u.u_sigmask[sig-1];
		ovec.sv_flags = 0;
		if ((sig == SIGCLD) && (u.u_procp->p_flag2 & S2CLDSTOP))
			ovec.sv_flags |= SV_BSDSIG;
		if (u.u_sigonstack & bit)
			ovec.sv_flags |= SV_ONSTACK;
 		if ((u.u_sigreset & bit) != 0)
			ovec.sv_flags |= SV_RESETHAND;
		u.u_error =
		    copyout((caddr_t)&ovec, (caddr_t)uap->osv, sizeof (ovec));
		if (u.u_error)
			return;
	}
	if (uap->nsv) {
		/* Don't allow any unknown bits to be set */

		if ((sig_flags & ~(SV_ONSTACK|SV_BSDSIG|SV_RESETHAND)) != 0) {
			u.u_error = EINVAL;
			return;
		}
		if (sig_flags & SV_BSDSIG)
			sig_flags &= ~SV_BSDSIG;
		else
			sig_flags |= SA_NOCLDSTOP;
		setsigvec(sig, sig_handler, sig_mask, sig_flags);
	}
}

sigaction()
{
	register struct a {
		int	signo;
		struct  sigaction *nsv;
		struct  sigaction *osv;
	} *uap = (struct a  *)u.u_ap;
	struct sigaction vec;
	struct sigaction ovec;
	register int sig;
	register int bit;
	register void (*sig_handler)();
	register int sig_flags;
	int sig_mask;

	sig = uap->signo;
	bit = mask(sig);
#ifdef SIGRESERVE
	if (sig <= 0 || sig >= NUSERSIG || sig >= NSIG || sig == SIGRESERVE) {
#else
	if (sig <= 0 || sig >= NUSERSIG || sig >= NSIG) {
#endif
		u.u_error = EINVAL;
		return;
	}

	/* read new vector first, so it isn't overwritten by old vector */
	if (uap->nsv) {
		u.u_error =
		    copyin((caddr_t)uap->nsv, (caddr_t)&vec, sizeof (vec));
		if (u.u_error)
			return;
		sig_handler = vec.sa_handler;
		sig_mask = vec.sa_mask.sigset[0];
		sig_flags = vec.sa_flags;

		if (sig_handler!=SIG_DFL && (sig==SIGKILL || sig==SIGSTOP)) {
			u.u_error = EINVAL;
			return;
		}
	}

	if (uap->osv) {
		ovec.sa_handler = u.u_signal[sig-1];
		bzero((caddr_t)&ovec.sa_mask,sizeof(sigset_t));
		ovec.sa_mask.sigset[0] = u.u_sigmask[sig-1];
		ovec.sa_flags = 0;

		if ((sig == SIGCLD) && (u.u_procp->p_flag2 & S2CLDSTOP) == 0)
			ovec.sa_flags |= SA_NOCLDSTOP;

		if ((u.u_sigonstack & bit) != 0)
			ovec.sa_flags |= SA_ONSTACK;

		if ((u.u_sigreset & bit) != 0)
			ovec.sa_flags |= SA_RESETHAND;

		u.u_error =
		    copyout((caddr_t)&ovec, (caddr_t)uap->osv, sizeof(ovec));
		if (u.u_error)
			return;
	}

	if (uap->nsv) {
		/* Don't allow any unknown bits to be set */
		if ((sig_flags & ~(SA_ONSTACK|SA_RESETHAND|SA_NOCLDSTOP))!=0) {
			u.u_error = EINVAL;
			return;
		}
		setsigvec(sig, sig_handler,sig_mask,sig_flags);
	}
}

setsigvec(sig, sig_handler, sig_mask, sig_flags)
	int sig;
	void (*sig_handler)();
	int sig_mask, sig_flags;
{
	register struct proc *p;
	register struct proc *q;
	register int bit;
	register int x;

	bit = mask(sig);
	p = u.u_procp;
	/*
	 * Change setting atomically.
	 */
	x = UP_SPL7();
	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(p));

	u.u_signal[sig-1] = sig_handler;
	u.u_sigmask[sig-1] = sig_mask & ~cantmask;
	if (sig == SIGCLD) {
		if (sig_flags & SA_NOCLDSTOP)
			p->p_flag2 &= ~S2CLDSTOP;
		else
			p->p_flag2 |= S2CLDSTOP;
	}

	if (sig_flags & SA_ONSTACK)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	if (sig_flags & SA_RESETHAND)
		u.u_sigreset |= bit;
	else
		u.u_sigreset &= ~bit;
	if (sig_handler == SIG_IGN) {
		p->p_sig &= ~bit;		/* never to be seen again */
		p->p_sigignore |= bit;
		p->p_sigcatch &= ~bit;
	}
	else {
		p->p_sigignore &= ~bit;
		if (sig_handler == SIG_DFL) {
			p->p_sigcatch &= ~bit;
			if ((discards & bit) != 0)
				p->p_sig &= ~bit; /* POSIX 1003.1, 3.3.1.3(c) */
		}
		else
			p->p_sigcatch |= bit;
	}

	UP_SPLX(x);

	if (sig == SIGCLD) {
		for (q = p->p_cptr; q != NULL; q = q->p_osptr) {

			/* Send SIGCLD if there is a zombie child that  */
			/* has not been adopted via ptrace() by another */
			/* process.					*/

			if (q->p_stat == SZOMB
			  && (!(q->p_dptr) || q->p_dptr == q->p_pptr)) {
				pm_psignal(p, SIGCLD);
				SPINUNLOCK(PROCESS_LOCK(p));
				SPINUNLOCK(sched_lock);
				return; /* once is enough */
			}
		}
	}
	SPINUNLOCK(PROCESS_LOCK(p));
	SPINUNLOCK(sched_lock);
}

sigprocmask()
{
	struct a {
		int how;
		sigset_t *set;
		sigset_t *oset;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	register int nmaskbits;
	sigset_t nmask;
	sigset_t omask;

	/* read new mask first, so it isn't overwritten by old mask */

	if (uap->set) {
		u.u_error =
		   copyin((caddr_t)uap->set, (caddr_t)&nmask, sizeof(sigset_t));
		if (u.u_error)
			return;
		nmaskbits = nmask.sigset[0] & ~cantmask;
	}

	if (uap->oset) {
		bzero((caddr_t)&omask, sizeof(sigset_t));
		omask.sigset[0] = p->p_sigmask;
		u.u_error =
		 copyout((caddr_t)&omask, (caddr_t)uap->oset, sizeof(sigset_t));
		if (u.u_error)
			return;
	}

	if (uap->set) {
		switch (uap->how) {
		case SIG_BLOCK:
			p->p_sigmask |= nmaskbits;
			break;
		case SIG_UNBLOCK:
			p->p_sigmask &= ~nmaskbits;
			break;
		case SIG_SETMASK:
			p->p_sigmask = nmaskbits;
			break;
		default:
			u.u_error = EINVAL;
			return;
		}
	}
}

sigsuspend()
{
	struct a {
		sigset_t *usigmask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	sigset_t nmask;

	if (uap->usigmask == (sigset_t *)0) {
		u.u_error = EFAULT;
		return;
	}

	u.u_error =
	    copyin((caddr_t)uap->usigmask, (caddr_t)&nmask, sizeof(sigset_t));
	if (u.u_error)
		return;

	/*
	 * When returning from sigsuspend, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in u.).
	 */
	u.u_oldmask = p->p_sigmask;
	p->p_flag |= SOMASK;
	p->p_sigmask = nmask.sigset[0] & ~cantmask;
	for (;;)
		sleep((caddr_t)&u, PSLEP);
	/*NOTREACHED*/
}

sigpending()
{
	struct a {
		sigset_t *set;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	sigset_t pset;

	bzero((caddr_t)&pset, sizeof(sigset_t));

	/* Due to SIGCLD semantics when the action for SIGCLD */
	/* is SIG_IGN, we also mask with ~p_sigignore.        */
	pset.sigset[0] = (p->p_sig & ~p->p_sigignore & p->p_sigmask);

	u.u_error =
		copyout((caddr_t)&pset, (caddr_t)uap->set, sizeof(sigset_t));
}

sigblock()
{
	struct a {
		int	usigmask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	register int x;

	SPL_REPLACEMENT(PROCESS_LOCK(p), spl6, x);

	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask |= uap->usigmask & ~cantmask;

	SPLX_REPLACEMENT(PROCESS_LOCK(p), x);
}

sigsetmask()
{
	struct a {
		int	usigmask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	register int x;

	SPL_REPLACEMENT(PROCESS_LOCK(p), spl6, x);

	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask = uap->usigmask & ~cantmask;

	SPLX_REPLACEMENT(PROCESS_LOCK(p), x);
}

sigpause()
{
	struct a {
		int	usigmask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in u.).
	 */
	SPINLOCK(sched_lock);
	SPINLOCK(proc_lock);
	u.u_oldmask = p->p_sigmask;
	p->p_flag |= SOMASK;
	p->p_sigmask = uap->usigmask & ~cantmask;
	SPINUNLOCK(proc_lock);
	SPINUNLOCK(sched_lock);
	for (;;)
		sleep((caddr_t)&u, PSLEP);
	/*NOTREACHED*/
}

sigstack()
{
	register struct a {
		struct	sigstack *nss;
		struct	sigstack *oss;
	} *uap = (struct a *)u.u_ap;
	struct sigstack ss;

	if (uap->oss) {
		u.u_error = copyout((caddr_t)&u.u_sigstack, (caddr_t)uap->oss,
						sizeof (struct sigstack));
		if (u.u_error)
			return;
	}
	if (uap->nss) {
		u.u_error =
		    copyin((caddr_t)uap->nss, (caddr_t)&ss, sizeof (ss));
		if (u.u_error == 0)
			u.u_sigstack = ss;
	}
}

/* KILL SHOULD BE UPDATED */

kill()
{
	register struct a {
		pid_t	pid;
		int	signo;
	} *uap = (struct a *)u.u_ap;
	register int who;
	int ispgrp = 0;
	int killall = 0;

	/* Users can't send certain signals (reserved for system).  */
	/* Prevent proc 1 (init) from being SIGKILLed or SIGSTOPed. */
	if ((uap->signo >= NUSERSIG) ||
	    ((uap->pid == PID_INIT) &&
	    ((uap->signo == SIGKILL) || (uap->signo == SIGSTOP)))) {
		u.u_error = EINVAL;
		return;
	}

	/* kill changed to Bell semantics */
	who = uap->pid;
	if (who == KILL_ALL_OTHERS) {
		ispgrp = 1;
		killall = 1;
	}
	else if (who <= 0) {
		ispgrp = 1;
		if (who == -1)
			killall = -1;
		else
			who = -who;
	}
	u.u_error = kill1(ispgrp, uap->signo, who, killall);
}

#ifdef __hp9000s300
killpg()
{
	register struct a {
		int pgrp;
		int signo;
	} *uap = (struct a *)u.u_ap;

	u.u_error = kill1(1, uap->signo, uap->pgrp,0);
}
#endif /* __hp9000s300 */

/* KILL CODE SHOULDNT KNOW ABOUT PROCESS INTERNALS !?! */

kill1(ispgrp, signo, who, priv)
	int priv;
	int ispgrp, signo, who;
{
	register struct proc *p;
	register short thx;
	register int endchain;
	int f;

#ifdef	SIGRESERVE
	if (signo < 0 || signo >= NSIG || signo == SIGRESERVE)
#else
	if (signo < 0 || signo >= NSIG)
#endif
		return EINVAL;
	if (who > 0 && !ispgrp) {
		p = pfind(who);	/* locks sched_lock if successful */
		if (p == 0)
			return ESRCH;
		/* kill1 changed to Bell semantics */
		if (u.u_uid && u.u_uid != p->p_uid && u.u_ruid != p->p_uid &&
		    (signo != SIGCONT || p->p_sid != u.u_procp->p_sid) &&
		    u.u_uid != p->p_suid && u.u_ruid != p->p_suid){
			SPINUNLOCK(sched_lock); 
			return EPERM;
		}
		if (signo)
			psignal(p, signo);
		else
			SPINUNLOCK(sched_lock); /* psignal unlocks sched_lock*/
		return 0;
	}
	if (who == 0) {
		/*
		 * Zero process id means send to my process group.
		 */
		ispgrp = 1;
		who = u.u_procp->p_pgrp;
		if (who == 0)
			return EINVAL;
	}
	/*
	 * If the priv flag is not set then we are killing all processes in
	 * our process group. We do this by chasing down the pgrp hash chain
	 * killing those who match the group (who).
	 * I believe that ispgrp is always one at this point. -joh
	 */
	if (!priv) {
		f = 0;

		/* Don't bother looking for any negative numbers */
		/* or any whose number is greater then the MAXPID*/
		if ((who < 0) || (who > MAXPID))
			return ESRCH;

		for (thx = pgrphash[ PGRPHASH(who) ]; thx != 0; thx =
		    proc[thx].p_pgrphx) {
			p = &proc[thx];
			if (p->p_pgrp != who || p->p_ppid == 0 ||
		    	    (p->p_flag&SSYS))
				continue;

			if (u.u_uid != 0 && u.u_uid != p->p_uid &&
			    u.u_ruid != p->p_uid &&
		    	    u.u_uid != p->p_suid && u.u_ruid != p->p_suid &&
		    	    (signo != SIGCONT || p->p_sid != u.u_procp->p_sid))
				continue;
			f++;
			if (signo)
				psignal(p, signo);
			/* this does not work, add token scheme -joh */
			/* KPREEMPTPOINT(); */
		}
		return (f == 0) ? ESRCH : 0;
	}

	SPINLOCK(activeproc_lock);
	SPINLOCK(sched_lock);

	endchain = 0;
	/* scan over the active proc table entries */
	for (f = 0, p = proc ; endchain == 0; p = &proc[p->p_fandx]) {
		if (p->p_fandx == 0)
			endchain = 1;
		if (!ispgrp) {
			if (p->p_pid != who)
				continue;
		}
		else if (p->p_pgrp != who && priv == 0 || p->p_ppid == 0 ||
		         (p->p_flag&SSYS) || (priv > 0 && p == u.u_procp))
			continue;
		if (u.u_uid != 0 && u.u_uid != p->p_uid &&
		    u.u_ruid != p->p_uid && u.u_uid != p->p_suid &&
		    u.u_ruid != p->p_suid &&
		    (signo != SIGCONT || p->p_sid != u.u_procp->p_sid))
			continue;
		f++;
		if (signo) {
			SPINLOCK(PROCESS_LOCK(p));
			pm_psignal(p, signo);
			SPINUNLOCK(PROCESS_LOCK(p));
		}
	}
	SPINUNLOCK(sched_lock);
	SPINUNLOCK(activeproc_lock);
	return (f == 0) ? ESRCH : 0;
}

/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 */
gsignal(pgrp, sig)
	register pid_t pgrp;
	register int sig;
{
	register struct proc *p;
	register short ghx;
#ifdef MP
	sv_lock_t nsp;
#endif MP

	if ((pgrp == 0) || (pgrp == PGID_NOT_SET))
		return;
	SPINLOCKX(sched_lock,&nsp);

	for (ghx = pgrphash[PGRPHASH(pgrp)]; ghx!=0; ghx = proc[ghx].p_pgrphx) {
		p = &proc[ghx];
		SPINLOCK(PROCESS_LOCK(p));
		if (p->p_pgrp == pgrp)
			pm_psignal(p, sig);
		SPINUNLOCK(PROCESS_LOCK(p));
	}
	SPINUNLOCKX(sched_lock,&nsp);
}

#ifdef NEVER_CALLED
struct proc *
valid_pidprc(pp)
	struct pidprc *pp;
{
	register struct proc *p;

	if (pp->pp_pid == 0)
		return (struct proc *)0;
	p = pfind(pp->pp_pid);	/* locks sched_lock if successful */
	if (p == 0)
		return (struct proc *)0;
#ifdef notdef
	if (p->p_prc != pp->p_prc) {
		SPINUNLOCK(sched_lock);
		return (struct proc *)0;
	}
#endif
	SPINUNLOCK(sched_lock);
	return p;
}

/*
 * Signal the process associated with the given pid and prc.
 */

signal_pidprc(pp, sig)
	struct pidprc *pp;
	int sig;
{
	register struct proc *p;

	p = (struct proc *)valid_pidprc(pp);
	if (p)
		psignal(p, sig);
}
#endif /* NEVER_CALLED */

#ifdef MP
/*
 * Send the specified signal to the specified process.
 *
 * sched_lock and proclock for this process must be locked before
 * entering here.
 */
#endif MP
#define PSIGNAL_RETURN	((void (*) ())-1) 	/* Return from psignal() action */

void
(*psignal_set_action(p, sig, smask))()
	register struct proc *p;
	register int sig;
	register int smask;
{
	register void (*action)();

#ifdef	SIGRESERVE
	if ((unsigned)sig >= NSIG || sig == SIGRESERVE)
#else
	if ((unsigned)sig >= NSIG)
#endif
		return((void (*) ())NULL);

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & STRC)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 * unless the signal is SIGCLD or SIGCONT.
		 */
		if (p->p_sigignore & smask)
			if (sig == SIGCLD || sig == SIGCONT)
				action = SIG_IGN;
			else
				action = PSIGNAL_RETURN;
		else if (p->p_sigmask & smask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & smask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}
	return(action);
}

/*
 * Sets the value of p_sig depending on which signal is being sent
 * 	and what action psignal wants to take.
 * Also sets p_nice if process is killed just so that it will run
 *	and get out of the way.
 */
#ifdef MP
/*
 * sched_lock and proclock for this process must be locked before
 * entering here.
 */
#endif MP
psignal_set_p_sig(p,sig,smask,action)
	register struct proc *p;
	register int sig;
	register int smask;
	register void (*action)();
{
	if (sig) {
		p->p_sig |= smask;
		switch (sig) {

		case SIGTERM:
			if ((p->p_flag&STRC) || action != SIG_DFL)
				break;
			/* fall into ... */

		case SIGKILL:
			if (p->p_nice > NZERO)
				p->p_nice = NZERO;
			break;

		case SIGCONT:
			p->p_sig &= ~stops;
			if (action == SIG_IGN)
				p->p_sig &= ~smask;
			break;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			p->p_sig &= ~mask(SIGCONT);
			break;
		}
	}
}

psignal_SSLEEP_action(p,sig,smask,action)
	register struct proc *p;
	register int sig;
	register int smask;
	register void (*action)();
{
		/*
		 * If process is sleeping at negative priority
		 * or with SSIGABL unset for RTPRIO
		 * we can't interrupt the sleep... the signal will
		 * be noticed when the process returns through
		 * trap() or syscall().
		 */
  		if ((SSIGABL & p->p_flag)==0)
			goto out;

		/*
		 * Process is sleeping and traced... make it runnable
		 * so it can discover the signal in issig() and stop
		 * for the parent.
		 */
		if (p->p_flag&STRC)
			goto run;
		switch (sig) {

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * These are the signals which by default
			 * stop a process.
			 */
			if (action != SIG_DFL)
				goto run;
			/*
			 * If process is in an orphaned process group,
			 * keyboard signals should not stop it.
			 */
			if (sig!=SIGSTOP && orphaned(p->p_pgrp,p->p_sid,
							(struct proc *)0)) {
				p->p_sig &= ~smask; /* discard stop signal */
				pm_psignal(p, SIGHUP); /* send SIGHUP instead */
				goto out;
			}
			p->p_sig &= ~smask;
			p->p_cursig = sig;
			stop(p);
			goto out;

		case SIGIO:
		case SIGCONT:
		case SIGURG:
		case SIGCLD:
		case SIGPWR:
		case SIGWINCH:
/* for DIL */
#ifdef	_WSIO
		case SIGDIL:
#endif
			/*
			 * These signals are special in that they
			 * don't get propogated... if the process
			 * isn't interested, forget it.
			 */
			if (action != SIG_DFL)
				goto run;
			p->p_sig &= ~smask;		/* take it away */
			goto out;

		default:
			/*
			 * All other signals cause the process to run
			 */
			goto run;
		}
out:
	return(1);
run:
	return(0);
}

psignal_SSTOP_action(p,sig,smask,action)
	register struct proc *p;
	register int sig;
	register int smask;
	register void (*action)();
{
		/*
		 * If traced process is already stopped,
		 * then no further action is necessary.
		 */
		if (p->p_flag&STRC)
			goto out;
		switch (sig) {

		case SIGKILL:
			/*
			 * Kill signal always sets processes running.
			 */
#ifdef	MP
			if (p->p_sleep_sema) {
				p->p_stat = SSLEEP;
				goto run;
			}
			else {
				/********************
				*** Should eliminate force_run
				*** in future
				*********************/
				/* MP guys look out for dbunsleep() */
				force_run(p);
				goto out;
			}
#else /* MP */
			goto run;
#endif /* ! MP */

		case SIGCONT:
			/*
			 * If the process catches SIGCONT, let it handle
			 * the signal itself.  If it isn't waiting on
			 * an event, then it goes back to run state.
			 * Otherwise, process goes back to sleep state.
			 */
#ifdef	MP
			if (action != SIG_DFL) {
				if (p->p_wchan) {
					p->p_stat = SSLEEP;
					goto run;
				}
				else if (p->p_sleep_sema == 0) {
					force_run(p);
					goto out;
				}
				else {
					p->p_stat = SSLEEP;
					goto out;
				}
			}
			else if (p->p_sleep_sema == 0) {
				force_run(p);
				goto out;
			}
			else {
				p->p_stat = SSLEEP;
				goto out;
			}
#else /* MP */
			if (action != SIG_DFL || p->p_wchan == 0)
				goto run;
			p->p_stat = SSLEEP;
			goto out;

#endif /* ! MP */
		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			p->p_sig &= ~smask;		/* take it away */
			goto out;

		default:
			/*
			 * If process is sleeping interruptibly, then
			 * unstick it so that when it is continued
			 * it can look at the signal.
			 * But don't setrun the process as its not to
			 * be unstopped by the signal alone.
			 */
#ifdef	MP
			/* DB_SENDRECV */
			dbunsleep(p);
			wsignal(p);
#else /* MP */
	  		if (p->p_wchan && (p->p_flag & SSIGABL))
				unsleep(p);
#endif /* ! MP */
			goto out;
		}
out:
	return(1);
run:
	return(0);
}


#ifdef STRACE
int (*log_signal)() = NULL;   /*  will be set by strace_link if configed in  */
#endif


/*
 * pm_psignal() expects that sched_lock and process_lock
 * are already locked.  It will not release them.
 */

pm_psignal(p,sig)
	register struct proc *p;
	register int sig;
{
	register int s;
	register void (*action)();
	int smask;
        register u_int reg_temp;

	if ((unsigned)sig >= NSIG
#ifdef	SIGRESERVE
	  || sig == SIGRESERVE
#endif
#ifndef HPUXBOOT
#ifndef	BSDJOBCTL
	  || IS_JOBCTL_SIG(sig)
#endif
#endif /* HPUXBOOT */
	  )
		return;
	smask = mask(sig);

#ifdef STRACE
	if (log_signal && (p->p_flag & SSCT))
		(*log_signal)(p->p_pid, sig);
#endif

	s = UP_SPL7();

	if((action = psignal_set_action(p,sig,smask))==PSIGNAL_RETURN)
		goto out;

	psignal_set_p_sig(p,sig,smask,action);

	/*
	 * Defer further processing for signals which are held.
	 */
	if ((action == SIG_HOLD) && !((sig == SIGCONT) && (p->p_stat == SSTOP)))
		goto out;

	switch (p->p_stat) {

	case SSLEEP:
		if(psignal_SSLEEP_action(p,sig,smask,action))
			goto out;
		else
			goto run;
		/*NOTREACHED*/

	case SSTOP:
		if(psignal_SSTOP_action(p,sig,smask,action))
			goto out;
		else
			goto run;
		/*NOTREACHED*/

	default:
		/*
		 * SRUN, SIDL, SZOMB do nothing with the signal,
		 * other than kicking ourselves if we are running.
		 * It will either never be noticed, or noticed very soon.
		 */
                if (!GETNOPROC(reg_temp) && (p == u.u_procp))
			aston();
		goto out;
	}
	/*NOTREACHED*/
run:
	/*
 	 * Raise priority to at least PUSER.
 	 */
	if (p->p_pri > PUSER)
                if ((p != u.u_procp || GETNOPROC(reg_temp)) &&
                        p->p_stat == SRUN && (p->p_flag & SLOAD)) {
			remrq(p);
			p->p_pri = PUSER;
			setrq(p);
		}
		else
			p->p_pri = PUSER;
#ifdef	MP
	/* DB_SENDRECV */
	dbunsleep(p);
	wsignal(p);
#else
	setrun(p);
#endif	/* ! MP */

out:
	UP_SPLX(s);

	return;
}

/*
 * Send the specified signal to
 * the specified process.
 */

psignal(p, sig)
	register struct proc *p;
	register int sig;
{
	register int s;
	sv_lock_t nsp_sched,nsp_proc;


	/*
	 * Acquire sched_lock and process lock if
	 * we don't have them already
	 */

	SPINLOCKX(sched_lock,&nsp_sched);
	SPINLOCKX(PROCESS_LOCK(p),&nsp_proc);

	/* XXX Avoid powerfail hazards, e.g. p_sig. */
	s = UP_SPL7();

	pm_psignal(p,sig);

	UP_SPLX(s);

	/*
	 * always unlock sched_lock and process lock
	 */

	SPINUNLOCK(sched_lock);
	SPINUNLOCK(PROCESS_LOCK(p));

	return;
}

#ifdef	MP
/********************************************
********** Force_run - Forces a stopped but
********** not blocked or sleeping process to run.
**********************************************/

static
force_run(p)
	register struct proc *p;
{
	register u_char	curpri;
        register u_int reg_temp;

        curpri = GETCURPRI(reg_temp);

/*************************************************
************* This is really bad... The following code
************* forces a process to be runnable if it is stopped
************* but not sleeping.
************* This should all be eliminated if the
************* BSD job control is implemented through a semaphore
************* approach
****************************************************/
	DO_ASSERT( !p->p_sleep_sema,"force_setrun: Process blocked");
	/* MP guys watch out for dbunsleep() */
	switch (p->p_stat) {

	case SRUN:
			printf("force_setrun: Process already runnable\n");
			return;
	case SSTOP:
			break;
	default:
			printf("force_setrun: Process in state %d\n",
				p->p_stat);
			break;
	};
	p->p_wchan = NULL;
	p->p_sleep_sema = NULL;
	p->p_stat = SRUN;
	p->p_pri = p->p_wakeup_pri;
	if (p->p_flag & SLOAD)
		setrq(p);
	if ((p->p_pri < curpri)&&(p->p_pri < CURPRI)) {
		runrun++;
		aston();
	}

	if ((p->p_flag&SLOAD) == 0) {
#ifdef	MP
		cvsync(&runout);
#else	/* ! MP */
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
#endif	/* ! MP */
		wantin++;
		/*
		 * Raise priority of swapper to priority of rt process
		 * we have to swap in.
		 */

		if ((p->p_flag & SRTPROC) &&
		    (proc[S_SWAPPER].p_pri > p->p_rtpri)) {
			(void)changepri(&proc[S_SWAPPER], p->p_rtpri);
		}
	}

}
#endif /* MP */

/*
 *  Called from issig() for traced processes:
 *    retval <	0	means:	return(retval) (procxmt() action==DOEXIT)
 *    retval ==	0	means:	loop back and look for signal to take
 *    retval >	0	means:	continue through issig, retval = signal
 *
 *  We enter and exit this routine with
 *	sched_lock taken
 *	PROCESS_LOCK(p) taken
 */
int
issig_STRC(p, sig, smask, action, savep_sig)
	struct	proc	*p;
	int		sig;
	int		*smask;
	int		*action;
	int		savep_sig;
{
	int	s1;

	/*
	 * If traced, always stop, and stay
	 * stopped until released by the parent.
	 */
	SPINUNLOCK(PROCESS_LOCK(p));
	SPINUNLOCK(sched_lock);
	do {
#ifdef	MP
		SPINLOCK(sched_lock);
		SPINLOCK(PROCESS_LOCK(p));
		if ((ipc.ip_req<=0)||(ipc.ip_lock!=p->p_pid)) {
			sv_sema_t	sv_sema;

			s1 = UP_SPL6();
			stop(p);
			UP_SPLX(s1);
			SPINUNLOCK(PROCESS_LOCK(p));
			release_semas(&sv_sema);
			swtch();  /* releases sched_lock */
			reaquire_semas(&sv_sema);
		} else {
			SPINUNLOCK(sched_lock);
			SPINUNLOCK(PROCESS_LOCK(p));
		}
#else	/* ! MP */
		if ((ipc.ip_req<=0)||(ipc.ip_lock!=p->p_pid)) {
			s1 = UP_SPL6();
			stop(p);
			UP_SPLX(s1);
			swtch();
		}
#endif	/* ! MP */
	} while (!(*action=procxmt()) && p->p_flag&STRC);

	SPINLOCK(sched_lock);
	SPINLOCK(PROCESS_LOCK(p));

	s1 = UP_SPL7();
	/*
	 * If procxmt() returned DOEXIT we will return -sig
	 * indicating that an exit() should be performed
	 * by the caller.  This allows the caller to cleanup
	 * instead of having procxmt() itself do the exit().
	 */
	if (*action == DOEXIT) {
		p->p_sig = savep_sig;
		p->p_cursig = 0;
		UP_SPLX(s1);
		VASSERT(sig > 0);
		return -sig;
	}
	/*
	 * If procxmt() returned DODETACH, the tracing
	 * process has detached the traced process.  A
	 * signal may have been designated to deliver.
	 */
	if (*action == DODETACH) {
		if (p->p_cursig)
			p->p_sig |= mask(p->p_cursig);
		goto return_for_next_signal;
	}
	/*
	 * If the traced bit got turned off,
	 * then put the signal taken above back into p_sig
	 * and go back up to the top to rescan signals.
	 * This ensures that p_sig* and u_signal are consistent.
	 */
	if ((p->p_flag&STRC) == 0) {
		p->p_sig |= *smask;
		goto return_for_next_signal;
	}

	/*
	 * If parent wants us to take the signal,
	 * then it will leave it in p->p_cursig;
	 * otherwise we just look for signals again.
	 */
	sig = p->p_cursig;
	if (sig == 0)
		goto return_for_next_signal;

	/*
	 * If signal is being masked, put it back
	 * into p_sig and look for other signals.
	 */
	*smask = mask(sig);
	if (p->p_sigmask & *smask) {
		p->p_sig |= *smask;
		goto return_for_next_signal;
	}

	UP_SPLX(s1);
	VASSERT(sig > 0);
	return sig;	/* do action for this signal */

return_for_next_signal:
	UP_SPLX(s1);
	return 0;	/* look for another signal */

}

/*
 * Returns true if the current
 * process has a signal to process.
 * The signal to process is put in p_cursig.
 * This is asked at least once each time a process enters the
 * system (though this can usually be done without actually
 * calling issig by checking the pending signal masks.)
 * A signal does not do anything
 * directly to a process; it sets
 * a flag that asks the process to
 * do something to itself.
 */

issig()
{
	register struct proc *p;
	register struct proc *this_child, *next_child;
	register int sig;
	int sigbits, smask;
	int action, savep_sig;
	int s;
	extern int (*graphics_sigptr)();
#ifdef MP
	sv_lock_t nsp;
#endif MP
	p = u.u_procp;

	/*
	 *  Protection issues in this code
	 *	We need 2 types of protection:
	 *		mutual exclusion (multiple top halves)
	 *		interrupt activity protection
	 *  S800/MP
	 *	mutual exclusion:	SPINLOCK code
	 *	interrupt activity:	SPINLOCK code
	 *  S800/UP
	 *	mutual exclusion:	N/A
	 *	interrupt activity:	UP_SPL code
	 *  S300/UP
	 *	mutual exclusion:	N/A
	 *	interrupt activity:	spl code
	 */
	S800(s = splcur());
	S300(s = spl6(); splx(s));
	SPINLOCKX(sched_lock, &nsp);
	SPINLOCK(PROCESS_LOCK(p));

	for (;;) {
		/*
		 * Here we check to see if the process has the graphics lock.
		 * If so, we must block the caught signals and others. The
		 *  graphics_get_sigmask() routine is in graf.300/graf.c for
		 *  the 300 and graf.800/framebuf.c for the 800.
		 *
		 *  We do not have to waste time checking this function pointer
                 *  against NULL.  There is no way for the SGRAPHICS flag to be
                 *  set if this function pointer is not valid.
		 */
		if (p->p_flag2 & SGRAPHICS)
		     sigbits = p->p_sig & ~((*graphics_sigptr)(p));
		else sigbits = p->p_sig & ~(p->p_sigmask);

		if ((p->p_flag&STRC) == 0)
			sigbits &= ~(p->p_sigignore & ~mask(SIGCLD));
		if (p->p_flag&SVFORK)
			sigbits &= ~stops;
		if (sigbits == 0) {
			break;
		}
		sig = ffs((long)sigbits);
		smask = mask(sig);

		/* While we fiddle with p_sig */
		(void) UP_SPL7();
		savep_sig = p->p_sig;
		p->p_sig &= ~smask;		/* take the signal! */
		p->p_cursig = sig;
		UP_SPLX(s);

		if (p->p_flag&STRC && (p->p_flag&SVFORK) == 0) {
			int	retval;

			/*
			 *  See comment on issig_STRC() for what the
			 *  return values mean
			 */
			retval = issig_STRC(p, sig, &smask, &action, savep_sig);
			if (retval == 0)	/*  look for sig to take */
				continue;
			if (retval < 0)	{	/*  DOEXIT from procxmt() */
				SPINUNLOCKX(sched_lock, &nsp);
				SPINUNLOCK(PROCESS_LOCK(p));
				return retval;
			}
			sig = retval;		/* continue with this sig val */
		}

		T_SPL(s);		/* Here we should be @ entry spl */
		T_SPINLOCK(sched_lock);
		T_SPINLOCK(PROCESS_LOCK(p));
		switch ((int)u.u_signal[sig-1]) {

		case (int)SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_ppid == 0)
				break;
			switch (sig) {

			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				/*
				 * A process in an orphaned process group is
				 * not allowed to stop on signals from the
				 * keyboard.
				 */
				if (orphaned(p->p_pgrp,p->p_sid,
							(struct proc *)0)) {
					/* discard stop sig */
					p->p_cursig = 0;
					/* let process know something's wrong */
					pm_psignal(p,SIGHUP);
					continue;
				}
				/* fall into ... */

			case SIGSTOP:
				if (p->p_flag&STRC)
					continue;
				(void) UP_SPL6();
				stop(p);
				UP_SPLX(s);
#ifdef	MP
				SPINUNLOCK(PROCESS_LOCK(p));

				{
					sv_sema_t	sv_sema;

					release_semas(&sv_sema);
					swtch();  /* releases sched_lock */
					reaquire_semas(&sv_sema);
				}
				SPINLOCK(sched_lock);
				SPINLOCK(PROCESS_LOCK(p));
#else	/* ! MP */
				swtch();
#endif	/* ! MP */
				continue;

			case SIGPWR:
			case SIGWINCH:
			case SIGCONT:
			case SIGCLD:
			case SIGURG:
			case SIGIO:
/* for DIL */
#ifdef	_WSIO
			case SIGDIL:
#endif
				/*
				 * These signals are normally not
				 * sent if the action is the default.
				 */
				continue;		/* == ignore */

			default:
				goto send;
			}
			/*NOTREACHED*/

		case (int)SIG_IGN:
			VASSERT(p->p_sigignore&mask(sig));
			if (sig == SIGCLD) {
				for (this_child = p->p_cptr; this_child != NULL;
				     this_child = next_child) {
					next_child = this_child->p_osptr;
					if (this_child->p_stat == SZOMB){
						SPINUNLOCK(sched_lock);
						SPINUNLOCK(PROCESS_LOCK(p));
						proc_unhash(this_child);
						freeproc(this_child);
						SPINLOCK(sched_lock);
						SPINLOCK(PROCESS_LOCK(p));
					}
				}
				continue;
			}
			/* else fall through */

		case (int)SIG_HOLD:
			/*
			 * Masking above should prevent us
			 * ever trying to take action on a held
			 * or ignored signal, unless process is traced.
			 */
			continue;

		default:
			/*
			 * This signal has an action, let
			 * psig process it.
			 */
			goto send;
		}
		/*NOTREACHED*/
	}
	T_SPINLOCK(sched_lock);
	T_SPINLOCK(PROCESS_LOCK(p));
	/*
	 * Didn't find a signal to send.
	 */
	p->p_cursig = 0;
	SPINUNLOCKX(sched_lock, &nsp);
	SPINUNLOCK(PROCESS_LOCK(p));
	T_SPL(s);
	return 0;

send:
	T_SPINLOCK(sched_lock);
	T_SPINLOCK(PROCESS_LOCK(p));
	/*
	 * Let psig process the signal.
	 */
	SPINUNLOCKX(sched_lock, &nsp);
	SPINUNLOCK(PROCESS_LOCK(p));
	T_SPL(s);
	return sig;
}

/*
 * Put the argument process into the stopped
 * state and notify the parent via wakeup and/or signal.
 */
stop(p)
	register struct proc *p;
{

	T_SPINLOCK(sched_lock);		/* test to see if we own this lock */
	T_SPINLOCK(PROCESS_LOCK(p));	/* test to see if we own this lock */
	T_UPSPL(SPL6);			/* assure that we are at least spl6 */

	p->p_stat = SSTOP;
	p->p_flag &= ~SWTED;
	if (p->p_flag & STRC && p->p_dptr)
		wakeup((caddr_t)p->p_dptr);	/* wake up step-parent */
	else
		wakeup((caddr_t)p->p_pptr);	/* wake up parent */
	/*
	 * Avoid sending signal to parent if process is traced
	 */
#ifdef MP
/*
 * XXX Assumption here that all process locks are the same lock,
 * otherwise we would have to lock p->pptr
 */
#endif MP
	if ((p->p_flag&STRC) != 0 || (p->p_pptr->p_flag2 & S2CLDSTOP) == 0)
		return;
	pm_psignal(p->p_pptr, SIGCLD);
}


/*
 * Perform the action specified by
 * the current signal.
 * The usual sequence is:
 *	if (issig())
 *		psig();
 * The signal bit has already been cleared by issig,
 * and the current signal number stored in p->p_cursig.
 */
psig()
{
	register struct proc *p = u.u_procp;
	register int sig = p->p_cursig;
	int smask = mask(sig), returnmask;
	register void (*action)();
	register int x;

	MP_ASSERT(! spinlocks_held(), "Spinlocks held on entry to psig()" );
	KPREEMPTPOINT();	/* allow KERNEL PREEMPTION right here */
	if (sig <= 0)
		panic("psig");
	action = u.u_signal[sig-1];
	if (action != SIG_DFL) {
		if (action == SIG_IGN || (p->p_sigmask & smask))
			panic("psig action");
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal (unless we're simulating
		 * the old signal facilities).
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want restored
		 * after the signal processing is completed.
		 */
		SPINLOCK(sched_lock);
		SPL_REPLACEMENT(PROCESS_LOCK(p), spl6, x);
 		if ((u.u_sigreset & smask) || (p->p_flag & SOUSIG)) {
 			if (sig != SIGILL && sig != SIGTRAP && sig != SIGPWR) {
 				u.u_signal[sig-1] = SIG_DFL;
 				p->p_sigcatch &= ~smask;
 			}
 			smask = 0;
 		}
		if (p->p_flag & SOMASK) {
			returnmask = u.u_oldmask;
			p->p_flag &= ~SOMASK;
		}
		else
			returnmask = p->p_sigmask;
		p->p_sigmask |= u.u_sigmask[sig-1] | smask;


		SPLX_REPLACEMENT(PROCESS_LOCK(p), x);
		SPINUNLOCK(sched_lock);
		u.u_ru.ru_nsignals++;
		sendsig(action, sig, returnmask);
		p->p_cursig = 0;
		return;
	}


	switch (sig) {

	case SIGILL:
	case SIGIOT:
	case SIGBUS:
	case SIGQUIT:
	case SIGTRAP:
	case SIGEMT:
	case SIGFPE:
	case SIGSEGV:
	case SIGSYS:
		u.u_arg[0] = sig;
		if (core())
			sig += 0200;
	}
	KPREEMPTPOINT();	/* allow KERNEL PREEMPTION right here */
	exit(sig);
}

#ifdef hp9000s800
sigsetreturn()
{
	struct a {
		void	(*func)();
		unsigned magic;
		int context_size;
	} *uap = (struct a *)u.u_ap;

	if (uap->magic == 0x06211988) {	 /* we have a valid magic cookie */
		switch (uap->context_size) {
			case sizeof(struct pa83_sigcontext):
				u.u_sigcontexttype = PA83_CONTEXT;
				break;
			case sizeof(struct sigcontext):
				u.u_sigcontexttype = PA89_CONTEXT;
				break;
			default:
				u.u_error = EINVAL;
				return;
		}
	}
	else {	/* assume old PA83 context if magic cookie doesn't match */
		u.u_sigcontexttype = PA83_CONTEXT;
	}
	u.u_sigreturn = uap->func;
}
#endif /* hp9000s800 */
