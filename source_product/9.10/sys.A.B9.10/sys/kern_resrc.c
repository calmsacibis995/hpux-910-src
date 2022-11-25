/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_resrc.c,v $
 * $Revision: 1.36.83.6 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/06 10:18:32 $
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


#include "../h/types.h"
#include "../h/param.h"
#ifdef hp9000s800
#include "../h/sysmacros.h"
#endif
#include "../h/systm.h"
#include "../h/user.h"
#ifdef hp9000s800
#include "../h/vnode.h"
#endif
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/rtprio.h"
#include "../h/pregion.h"

/*
 * Resource controls and accounting.
 */

getpriority()
{
	register struct a {
		int	which;
		int	who;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	u.u_r.r_val1 = NZERO+20;
	u.u_error = ESRCH;
	switch (uap->which) {
	case PRIO_PROCESS:
		if (uap->who == 0)
			p = u.u_procp;
		else
			p = pfind(uap->who);
		if (p == 0)
			return;
		u.u_r.r_val1 = p->p_nice;
		u.u_error = 0;
		break;
	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		for (p = proc; p < procNPROC; p++) {
			if (p->p_stat == NULL)
				continue;
			if (p->p_pgrp == uap->who &&
			    p->p_nice < u.u_r.r_val1) {
				u.u_r.r_val1 = p->p_nice;
				u.u_error = 0;
			}
		}
		break;
	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		for (p = proc; p < procNPROC; p++) {
			if (p->p_stat == NULL)
				continue;
			if (p->p_uid == uap->who &&
			    p->p_nice < u.u_r.r_val1) {
				u.u_r.r_val1 = p->p_nice;
				u.u_error = 0;
			}
		}
		break;
	default:
		u.u_error = EINVAL;
		break;
	}
	u.u_r.r_val1 -= NZERO;
}

setpriority()
{
	register struct a {
		int	which;
		int	who;
		int	prio;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	u.u_error = ESRCH;
	switch (uap->which) {
	case PRIO_PROCESS:
		if (uap->who == 0)
			p = u.u_procp;
		else
			p = pfind(uap->who);
		if (p == 0)
			return;
		donice(p, uap->prio);
		break;
	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		for (p = proc; p < procNPROC; p++) {
			if (p->p_stat == NULL)
				continue;
			if (p->p_pgrp == uap->who)
				donice(p, uap->prio);
		}
		break;
	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		for (p = proc; p < procNPROC; p++) {
			if (p->p_stat == NULL)
				continue;
			if (p->p_uid == uap->who)
				donice(p, uap->prio);
		}
		break;
	default:
		u.u_error = EINVAL;
		break;
	}
}

donice(p, n)
	register struct proc *p;
	register int n;
{

	if (u.u_uid && u.u_ruid &&
	    u.u_uid != p->p_uid && u.u_ruid != p->p_uid) {
		u.u_error = EACCES;
		return;
	}
	n += NZERO;
	if (n >= 2*NZERO)
		n = 2*NZERO - 1;
	if (n < 0)
		n = 0;
	if (n < p->p_nice && !suser()) {
		/*changed from EACCES for Bell compatibility*/
		u.u_error = EPERM;
		return;
	}
	p->p_nice = n;
	(void) setpri(p);
	if (u.u_error == ESRCH)
		u.u_error = 0;
	proc_update_active_nice(p);
}

setrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *lim;
	} *uap = (struct a *)u.u_ap;
	struct rlimit alim;
	register struct rlimit *alimp;
#ifdef __hp9000s300
	extern int maxdsiz, maxssiz;
#endif

	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	alimp = &u.u_rlimit[uap->which];
	u.u_error = copyin((caddr_t)uap->lim, (caddr_t)&alim,
		sizeof (struct rlimit));
	if (u.u_error)
		return;
	if (alim.rlim_cur > alimp->rlim_max || alim.rlim_max > alimp->rlim_max)
		if (!suser())
			return;
	switch (uap->which) {
	case RLIMIT_DATA:
		if (alim.rlim_cur > ptob(maxdsiz))
			alim.rlim_cur = ptob(maxdsiz);
		break;

	case RLIMIT_STACK:
		if (alim.rlim_cur > ptob(maxssiz))
			alim.rlim_cur = ptob(maxssiz);
		break;
        case RLIMIT_NOFILE: {
	      
	    int oldnfdchunks;
	    struct ofile_t **tmp_ofilep;

	    /* If they are changing the hard limit, make sure they aren't 
	       increasing it beyond the system wide limit MAXFUPLIM, or 
	       decreasing it below the soft limit.  Note: only the super
	       user may increase the hard limit, this is checked ealier.*/
	    if (alim.rlim_max > MAXFUPLIM) {
		u.u_error = EINVAL;
		return;
	    }
	    if (alim.rlim_max < alim.rlim_cur) {
		u.u_error = EINVAL;
		return;
	    }

	    if (alim.rlim_cur != alimp->rlim_cur) {
		/* It looks like they're changing the soft limit. */

		/* See if they are telling us to increase their max
		   number of open files to a number smaller than the
		   soft limit already is.  If none of the file descriptors
		   they are trying to get rid of are allocated, we will 
		   bail on the extra file descriptor table pointers that 
		   they have allocated.  No biggie, but it is possible to 
		   decrease your soft limit. */
	      
		if (u.u_maxof > alim.rlim_cur) {
		    /* They are asking to decrease their soft limit. */
		    if ((u.u_highestfd + 1) > alim.rlim_cur) {

			/* They have a file descriptor allocated which 
			   is higher then the limit they are trying to 
			   decrease to.  Return an error. */
			u.u_error = EINVAL;
			return;
		    }
		    
		    oldnfdchunks = NFDCHUNKS(u.u_maxof);
		    u.u_maxof = alim.rlim_cur;
		    /* Save the old structure to copy its values in our new 
		       smaller structure. */
		    tmp_ofilep = u.u_ofilep;
		    /* Now allocate a new structure which has enough space 
		       for the new number of file descriptors.   */
		    u.u_ofilep = 
		      (struct ofile_t **) 
			kmem_alloc((sizeof(struct ofile_t *)) 
				   * NFDCHUNKS(u.u_maxof));
		    bzero((caddr_t) u.u_ofilep, (sizeof(struct ofile_t *)) 
			  * NFDCHUNKS(u.u_maxof));
		    /* Copy the old junk into the new one. */
		    bcopy((caddr_t)tmp_ofilep, (caddr_t)u.u_ofilep, 
			  (sizeof(struct ofile_t *)) * NFDCHUNKS(u.u_maxof));
		
		    /* Bail on the temporary pointer to the old structure. */
		    kmem_free((caddr_t)tmp_ofilep, 
			      (sizeof(struct ofile_t *) * oldnfdchunks));
		}
		else 
		  if (u.u_maxof < alim.rlim_cur) {
		      
		      /* They are trying to increase their soft limit. 
			 Make sure they ( super and non-super-user processes) 
			 aren't trying to increase the number beyond the 
			 system-wide absolute upper limit of maximum open 
			 files (MAXFUPLIM) */
		      
		      
		      if (alim.rlim_cur > MAXFUPLIM) {
			  /* If they are, set errno to EINVAL and return. */
			  u.u_error = EINVAL;
			  return;
		      }
		      
		      oldnfdchunks = NFDCHUNKS(u.u_maxof);
		      u.u_maxof = alim.rlim_cur;
		      /* Save the old structure to copy its values in our new 
			 larger structure. */
		      tmp_ofilep = u.u_ofilep;
		      /* Now allocate a new structure which has enough space 
			 for the new number of file descriptors.   */
		      u.u_ofilep = 
			(struct ofile_t **) 
			  kmem_alloc((sizeof(struct ofile_t *)) 
				     * NFDCHUNKS(u.u_maxof));
		      bzero((caddr_t) u.u_ofilep, 
			    (sizeof(struct ofile_t *)) * NFDCHUNKS(u.u_maxof));
		      /* Copy the old junk into the new one. */
		      bcopy((caddr_t)tmp_ofilep, (caddr_t)u.u_ofilep, 
			    (sizeof(struct ofile_t *)) * oldnfdchunks);
		      /* Bail on the temporary pointer to the old structure. */
		      kmem_free((caddr_t)tmp_ofilep, (sizeof(struct ofile_t *) 
						      * oldnfdchunks));
		  }
	    }
	}
        }
	*alimp = alim;
	if (uap->which == RLIMIT_RSS)
		u.u_procp->p_maxrss = alim.rlim_cur/NBPG;
}

getrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *rlp;
	} *uap = (struct a *)u.u_ap;

	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyout((caddr_t)&u.u_rlimit[uap->which], (caddr_t)uap->rlp,
	    sizeof (struct rlimit));
}

/* System V - compatible ulimit system call */
ulimit()
{
	register struct a {
		int	cmd;
		long	arg;
	} *uap;

	uap = (struct a *)u.u_ap;
	switch(uap->cmd) {
	case 2:
		if (uap->arg > u.u_rlimit[RLIMIT_FSIZE].rlim_max && !suser())
			return;		/* u.u_error already set to EPERM */
		if (uap->arg < 0) {
			u.u_error = EINVAL;
			return;
		}
		u.u_rlimit[RLIMIT_FSIZE].rlim_cur = uap->arg;
		u.u_rlimit[RLIMIT_FSIZE].rlim_max = uap->arg;
	case 1:
		u.u_r.r_off = u.u_rlimit[RLIMIT_FSIZE].rlim_max;
		break;
	case 3:
		{
		preg_t *prp;

		if ((prp = findpregtype(u.u_procp->p_vas, PT_DATA)) == NULL)
			u.u_r.r_off = 0;
		else
			u.u_r.r_off = (off_t)(prp->p_vaddr +
					u.u_rlimit[RLIMIT_DATA].rlim_max);
		}
		break;
#if defined(HPNSE) && defined(hp9000s800)
	case 4:
		u.u_r.r_off = u.u_maxof;
		break;
#endif
	default:
		u.u_error = EINVAL;
		break;
	}
}

int
getnumfds()
{
        u.u_r.r_val1 = u.u_highestfd + 1;
}

getrusage()
{
	register struct a {
		int	who;
		struct	rusage *rusage;
	} *uap = (struct a *)u.u_ap;
	register struct rusage *rup;
	struct rusage ru;

	switch (uap->who) {
	case RUSAGE_SELF:
		rup = &u.u_ru;
		ru = *rup;
		ru.ru_stime = u.u_procp->p_stime;
		ru.ru_utime = u.u_procp->p_utime;
		break;
	case RUSAGE_CHILDREN:
		rup = &u.u_cru;
		ru = *rup;
		break;
	default:
		u.u_error = EINVAL;
		return;
	}
#ifdef __hp9000s300
	ru_ticks_to_timeval(ru.ru_stime);
	ru_ticks_to_timeval(ru.ru_utime);
#endif

	u.u_error = copyout((caddr_t)&ru, (caddr_t)uap->rusage,
	    sizeof (struct rusage));
}

ruadd(ru, ru2)
	register struct rusage *ru, *ru2;
{
	register long *ip, *ip2;
	register int i;

#ifdef __hp9000s300
	ru->ru_uticks += ru2->ru_uticks;
	ru->ru_sticks += ru2->ru_sticks;
#endif
#ifdef hp9000s800
	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
#endif
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i > 0; i--)
		*ip++ += *ip2++;
}

rtprio()
{
	register struct a {
		pid_t	pid;
	        int     prio;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;
 	register int pri;
	int s;

#ifdef MP_SDTA
#include "../machine/sdta.h"
	if(uap->pid == SDTA_PID){
		u.u_r.r_val1 = sdta_initialize(uap->prio);
		return;
	}
#endif MP_SDTA

	/* Rtprio has the purpose of setting the
	 * priority of process pid to real time
	 * priority prio.  That means setting
	 * p_rtpri, p_usrpri and the RT bit in p_flag
	 * of the process and any shadow children.
	 */

	/* Finding proc entry from pid */
	if (uap->pid == 0){
		p = u.u_procp;
		SPINLOCK(sched_lock);
	}else
		/* pfind locks sched_lock if successful */
		if((p = pfind((int)uap->pid))==0){  /* No such pid */
			u.u_error = ESRCH;
			return;
		}

	MP_ASSERT(owns_spinlock(sched_lock),"rtprio: sched_lock not owned!");

	if(p->p_flag & SRTPROC)
		u.u_r.r_val1 = rtunconvert(p->p_rtpri);
        else
		u.u_r.r_val1 = RTPRIO_RTOFF;

	pri = rtconvert(uap->prio);
	if(pri == RTPRIO_NOCHG){
		SPINUNLOCK(sched_lock);
		return;
	}

	/* Test for matching correct user */

	if (u.u_uid && u.u_uid != p->p_uid && u.u_ruid != p->p_uid
	    && u.u_uid != p->p_suid && u.u_ruid != p->p_suid){
		u.u_error = EPERM;
		SPINUNLOCK(sched_lock);
		return;
	}

	/* Test for capability */

	if(!( (pri == RTPRIO_RTOFF && uap->pid == 0) ||
	      (in_privgrp(PRIV_RTPRIO, (struct ucred *)0) || suser()) )) {

		/* if not super user or realtime capability then
		 * return EPERM ... except for reading priorities
		 * or setting one's own priority back to timesharing.
		 */
		 u.u_error = EPERM;
		 SPINUNLOCK(sched_lock);
		 return;
	};

	/* Turn off real time priorities */
	if(pri == RTPRIO_RTOFF){
		p->p_flag &= ~SRTPROC; /* turn off RT flag */
		setpri(p);
	}
	/* Test for non-priorities */
	else if((pri < RTPRIO_MIN) || (pri > RTPRIO_MAX)){
		u.u_error = EINVAL;
		SPINUNLOCK(sched_lock);
		return;
        }
	/* Set an RT priority */
	else {
		p->p_flag |= SRTPROC;/* set real time flag */
		p->p_rtpri  = pri;   /* set real time priority */
		p->p_usrpri = pri;   /* set user priority */
	}
	proc_update_active_nice(p);

	/*
	 * Discover if this process is in the run
	 * queue and if so pull it out before changing priority
	 * and then put it back in. Otherwise just change p_pri.
	 *
	 * Must be protected in case of interrupt that changes
	 * our priority or the run queues.
	 */

	/* XXX when this spl7 is removed, fix this unlock,lock pair */
	SPINUNLOCK(sched_lock);
	/* p_pri and run queue must be consistent for powerfail */
	s = spl7();
	SPINLOCK(sched_lock);
#ifdef MP
	if ((!(p->p_mpflag & SRUNPROC)) &&
#else
	if ((p != u.u_procp) &&
#endif
	    p->p_stat == SRUN && (p->p_flag & SLOAD)) {
		remrq(p);
		p->p_pri = (p->p_usrpri < PTIMESHARE)?p->p_usrpri:PTIMESHARE;
		setrq(p);
	} else
		p->p_pri = (p->p_usrpri < PTIMESHARE)?p->p_usrpri:PTIMESHARE;
	SPINUNLOCK(sched_lock);
	splx(s);
	runrun++;
	return;
}
