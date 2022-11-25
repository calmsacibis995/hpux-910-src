/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/kern_xxx.c,v $
 * $Revision: 1.50.83.7 $	$Author: marshall $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/09 13:34:06 $
 */

/*	kern_xxx.c	6.1	83/07/29	*/
/* HPUX_ID: @(#)kern_xxx.c	55.1		88/12/23 */

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

#include "../h/param.h"
#include "../h/systm.h"
#ifdef __hp9000s800
#include "../h/dir.h"
#endif
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/reboot.h"
#include "../h/uio.h"
#include "../h/vnode.h"
#ifdef __hp9000s300
#include "../ufs/inode.h" /* needed for s300 version of reboot() */
#endif /* hp9000s300 */
#include "../h/pathname.h"
#ifdef __hp9000s800
#include "../h/buf.h"
#include "../h/vm.h"
#endif /* hp9000s800 */
#ifdef __hp9000s300
#include "../s200io/bootrom.h"
#endif /* hp9000s300 */

#include "../dux/lookupops.h"
#include "../dux/cct.h"
#include "../dux/dux_hooks.h"
#include "../h/kern_sem.h"

long hostid;

gethostid()
{
	u.u_r.r_val1 = hostid;
}

sethostid()
{
	struct a {
		int	hostid;
	} *uap = (struct a *)u.u_ap;

	if (suser())
		hostid = uap->hostid;
}

char  	domainname[DOMAINNAMELENGTH];
int     domainnamelen; 

getdomainname()
{
	register struct a {
		char	*domainname;
		int	len;
	} *uap = (struct a *)u.u_ap;
	register u_int len;

	len = uap->len;
	if (len > domainnamelen + 1)
		len = domainnamelen + 1;
	u.u_error = copyout((caddr_t)domainname,(caddr_t)uap->domainname,len);
}

#ifdef __hp9000s300
struct msus no_msus(blocked, dev)
char blocked;
dev_t dev;
{
	struct msus msus;
	int maj = major(dev);

	/* Special case kludge for no LAN driver */
	if (!blocked && ((maj == 18) || (maj == 19))) {
		msus.dir_format = 7;		/* Special */
		msus.device_type = 2;		/* LAN */
		msus.sc = m_selcode(dev);
		msus.ba = m_busaddr(dev);	/* don't care for LAN */
		msus.unit = m_unit(dev);	/* don't care for LAN */
		msus.vol = m_volume(dev);	/* don't care for LAN */
	}
	else {
		msus.device_type = 31;
	}
	return(msus);
}

struct msus (*msus_for_boot)() = no_msus;
extern caddr_t dio2_map_page();

reboot()
{
	register struct a {
		int	flags;
		char	*device;
		char	*file;
		char	*linkaddr;  /* link address of new server */
	} *uap = (struct a *)u.u_ap;
	struct vnode *vp;
	struct pathname pn;
	register char *p;
	register int cnt = 0, c;
	struct msus new_msus;
	extern struct msus msus;
	extern char sysname[];
	extern caddr_t *farea;
	char *farea_ptr;
	dev_t dev;
	int s;

	if (suser()) {
		uap->flags |= RB_BOOT;
		if (uap->flags & RB_NEWDEVICE) {
			uap = (struct a *) u.u_ap;
			u.u_error = pn_get(uap->device, UIOSEG_USER, &pn);
			if (u.u_error)
				return;
			u.u_error = lookuppn(&pn, FOLLOW_LINK, (struct vnode **)0, &vp, LKUP_NORECORD);
			if (u.u_error) {
				pn_free(&pn);
				return;
			}
			if ((vp -> v_type != VBLK) && (vp -> v_type != VCHR)) {
				u.u_error = EINVAL;
				VN_RELE(vp);
				return;
			}
			if (vp->v_fstype == VDUX) {
				/* v_rdev field not filled in if the inode */
				/* for the device file is type VDUX and */
				/* if the device has not been opened. */
				dev = (VTOI(vp))->i_device;
			} else {
				dev = vp->v_rdev;
			}
			new_msus = msus_for_boot((vp->v_type == VBLK)?1:0, dev);

			if (new_msus.device_type == 31) {
				u.u_error = ENXIO;
				return;
			} else
				msus = new_msus;

			uap->flags &= ~RB_NEWDEVICE;
			VN_RELE(vp);
		}
			
		if (uap->flags & RB_NEWSERVER) {
			/* want to boot to a new LAN boot server */
			char tmp[3];	/* tmp string to hold 1 "byte" */
			tmp[2] = NULL;  /* last char always null to end str */
			p = uap->linkaddr;	/* p is lkaddr in user space */

			/* Get rid of 0x if prefixed on linkaddr */
		        if ((fubyte(p)) == '0' && (((c=fubyte(p+1)) == 'x') 
							   || (c == 'X'))) {
			   p = &p[2];
			}
			/* map in farea address space so we can write to it */
			s=spl6();  /* don't let intrp steal mapped page */
			farea_ptr = (char *)dio2_map_page(farea);
			
			/* for each byte in addr string */
			for (cnt = 0; cnt < 6; cnt++) {
			    /* copy 1 "byte" string and write to farea */
			    tmp[0] = fubyte(p++);
			    tmp[1] = fubyte(p++);
			    /* translate to ascii, but only write 1 byte */
			    *(farea_ptr++) = (char)strtol(tmp,(char **)NULL,16);
			} /* end for each byte */

			splx(s);   /* don't care about interrupts now */
			uap->flags &= ~RB_NEWSERVER;
		}  /* end if new server */
		if (uap->flags & RB_NEWFILE) {
			/* build a pascal string */
			p = sysname;
			cnt = 0;
			while((c=fubyte(uap->file++)) != -1) {
				if (c == 0) break;
				*p++ = c;
				if (++cnt >=10) break;
			}
			for (; ++cnt < 10; )
				*p++ = (char) 0x00;
			uap->flags &= ~RB_NEWFILE;
		}

		boot(uap->flags, 0);
	} else
		u.u_error = EPERM;
}

#endif /* hp9000s300 */

#ifdef __hp9000s800
reboot()
{
	register struct a {
		int	opt;
		/*
		 * The following two arguments come from the hp9000s300
		 * implementation of this system call.  They allow a
		 * device file name and a file within that device to
		 * be specified as the boot device and the file to be
		 * booted.  The hp9000s800 implementation does not do that
		 * as of 11/85; it is purely a 4.2BSD implemenatation.
		 * (The fields are here for the future.)
		 */
		char	*device;
		char	*file;
	};

	if (suser())
		boot(RB_BOOT, ((struct a *)u.u_ap)->opt);
}
#endif /* hp9000s800 */

osetuid()
{
	unsigned register uid;
	register struct a {
		int	uid;
	} *uap;

	register struct proc *p;
	uap = (struct a *)u.u_ap;
	uid = uap->uid;

	if (uid < 0 || uid >= MAXUID) {
		u.u_error = EINVAL;
		return;
	}

	p = u.u_procp;

#ifdef AUDIT
	p->p_idwrite = 0;	/* clear flag */
#endif AUDIT
	if (u.u_uid && (uid == u.u_ruid))
		{
			u.u_cred = crcopy(u.u_cred);
			u.u_uid = uid;
		}
	else if (u.u_uid && (uid == u.u_uid)) {
		if (!in_privgrp(PRIV_SETRUGID, (struct ucred *)0)) {
			if (uid == p->p_suid) {
				u.u_cred = crcopy(u.u_cred);
				u.u_uid = uid;
			}
			else {
				u.u_error = EPERM;
				return;
			}
		}
		else {
			u.u_cred = crcopy(u.u_cred);
			/* unlink old u.u_ruid and insert new into chain */
			if (!(uremove(u.u_procp)))
				panic("osetuid1: lost uidhx");
			SPINLOCK(sched_lock);
			ulink(u.u_procp,uid);
			u.u_procp->p_uid = uid;
			u.u_ruid = uid;
			SPINUNLOCK(sched_lock);
		}
	}
	else if (u.u_uid && (uid == p->p_suid))
		{
			u.u_cred = crcopy(u.u_cred);
			u.u_uid = uid;
		}
	else if (suser()) {
		u.u_cred = crcopy(u.u_cred);
		/* unlink old u.u_ruid and insert new into chain */
		if (!(uremove(u.u_procp)))
			panic("osetuid2: lost uidhx");
		SPINLOCK(sched_lock);
		ulink(u.u_procp,uid);
		u.u_uid = uid;
		u.u_procp->p_uid = uid;
		u.u_procp->p_suid = uid;
		u.u_ruid = uid;
		SPINUNLOCK(sched_lock);
	} else 
		return;

	/* DB_SENDRECV */
	dbsetuid();

}

osetgid()
{
	unsigned register gid;
	register struct a {
		int	gid;
	} *uap;

	register change_egid = 0;
	register change_rgid = 0;
	register change_sgid = 0;
#ifdef AUDIT
	register struct proc *p;
#endif AUDIT
	int s;

	uap = (struct a *)u.u_ap;
#ifdef AUDIT
	p = u.u_procp;
	p->p_idwrite = 0;	/* clear flag */
#endif AUDIT
	gid = uap->gid;

	if (gid < 0 || gid >= MAXUID) {
		u.u_error = EINVAL;
		return;
	}

	if (u.u_uid && (u.u_rgid == gid))
		change_egid++;
	else if (u.u_uid && (u.u_gid == gid)) {
		if (!in_privgrp(PRIV_SETRUGID, (struct ucred *)0)) {
			if (u.u_sgid == gid) {
				change_egid++;
			}
			else {
				u.u_error = EPERM;
				return;
			}
		}
		else {
			change_rgid++;
		}
	}
	else if (u.u_uid && (u.u_sgid == gid))
		change_egid++;
	else if (suser()) {
		change_rgid++;
		change_egid++;
		change_sgid++;
	} else
		return;

	if (change_rgid || change_egid || change_sgid)
		u.u_cred = crcopy(u.u_cred);
	if (change_rgid) {
#ifdef	notdef	/* Leave this out because it is NOT Bell compatible. */
		leavegroup(u.u_rgid);
		(void) entergroup(gid);
#endif
		u.u_rgid = gid;
	}
	if (change_egid)
		u.u_gid = gid;
	if (change_sgid)
		u.u_sgid = gid;
	/*
 	* For privileged groups need to re-compute privilege mask.
 	*/
	s = spl6();
	u.u_procp->p_flag |= SPRIV;
	splx(s);
}

/*
 * Replaced 4.1-compatible version with SysIII-compatible
 * Flag indicates getpgrp, setpgrp or setsid call:
 *	0 - getpgrp(getpgid)
 *	1 - setpgrp
 *	2 - setsid
 */

osetpgrp()
{
	register struct a {
		int	flag;
	} *uap;
	register struct proc *p;
	register int s;

	uap = (struct a *)u.u_ap;
	p = u.u_procp;
	SPINLOCK(sched_lock);
	if (uap->flag) {		/* setpgrp() or setsid() */
		if (p->p_pgrp != p->p_pid) {
#ifdef	BSDJOBCTL	
		    if (pgrpfind((int)p->p_pid)) {
			if (uap->flag == 2) {	/* setsid() is called */
				u.u_error = EPERM;
				goto finished;
			}
		    }
		    else {
			s = UP_SPL6();
			u.u_procp->p_ttyp = NULL;  /* lose controlling tty */
			u.u_procp->p_ttyd = NODEV; /* and 'clear' dev_t */

			/* create a new session */
			if (!schange(p, p->p_pid))
				panic("osetpgrp: lost sidhx");
			/* unlink old pgrp from chain and insert new */
			if (!gchange(p, (short)p->p_pid))
				panic("osetpgrp: lost pgrphx");
			UP_SPLX(s);
		    }
#endif
		}
		else {
			if (uap->flag == 2) {	/* setsid() is called */
				u.u_error = EPERM;
				goto finished;
			}
		}
	}
	u.u_rval1 = p->p_pgrp;

finished:		
	SPINUNLOCK(sched_lock);
	return;

}


otime()
{
	register struct a {
		caddr_t	time_ptr;
	} *uap = (struct a *)u.u_ap;
#ifdef __hp9000s300
	struct timeval atv;
#endif

	/*
	 * You can't just use time.tv_sec, because we might have ticked
	 * past the second mark.  We need to get the real time of day,
	 * accurate to the microsecond, because that can change the seconds.
	 *
	 * It seems that only s300 has this problem.
	 */
#ifdef __hp9000s800
	u.u_r.r_time = time.tv_sec;
#endif
#ifdef __hp9000s300
	get_precise_time(&atv);
	u.u_r.r_time = atv.tv_sec;
#endif
	if (uap->time_ptr != NULL)
	    u.u_error = copyout ((caddr_t )&u.u_r.r_time, uap->time_ptr,
			         sizeof (u.u_r.r_time));
}

ostime()
{
	register struct a {
#ifdef __hp9000s300
		int	time;
#endif
#ifdef __hp9000s800
		long	*time;
#endif
	} *uap = (struct a *)u.u_ap;
	struct timeval tv;
	sv_sema_t SS;

	if (!suser()) {
		u.u_error = EPERM;
		return;
	}
	/* must sync out the inodes due to the inode compare update */
	/* algorithm for discless */
	if (my_site_status & CCT_CLUSTERED){
		PXSEMA(&filesys_sema, &SS);
		sync();
		VXSEMA(&filesys_sema, &SS);
	}
#ifdef __hp9000s300
	tv.tv_sec = uap->time;
#endif /* hp9000s300 */
#ifdef __hp9000s800
	u.u_error = 
		copyin((caddr_t)uap->time, (caddr_t)&tv, sizeof(tv.tv_sec));
	if (u.u_error) return;
#endif /* hp9000s800 */
	tv.tv_usec = 0;
	setthetime(&tv);
        if (my_site_status & CCT_CLUSTERED)
                DUXCALL(REQ_SETTIMEOFDAY)();
}

#ifdef __hp9000s800
#include "../h/timeb.h"
#endif hp9000s800
#ifdef __hp9000s300
/* from old timeb.h */
struct timeb {
	time_t	time;
	u_short	millitm;
	short	timezone;
	short	dstflag;
};
#endif /* hp9000s300 */

oftime()
{
	register struct a {
		struct	timeb	*tp;
	} *uap = (struct a *)u.u_ap;
	struct timeb tb;
	struct timeval atv;
#ifdef __hp9000s800
	struct timeval ms_gettimeofday();
#endif

#ifdef __hp9000s800
	atv = ms_gettimeofday();
#endif
#ifdef __hp9000s300
	get_precise_time(&atv);
#endif
	tb.time = atv.tv_sec;
	tb.millitm = (u_short) (atv.tv_usec / 1000);
	tb.timezone = tz.tz_minuteswest;
	tb.dstflag = tz.tz_dsttime;
	u.u_error = copyout((caddr_t)&tb, (caddr_t)uap->tp, sizeof (tb));
}

oalarm()
{
	register struct a {
		unsigned long	deltat;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	register int hzleft;
	int s = spl7();

	hzleft = untimeout(realitexpire, (caddr_t)p);
	timerclear(&p->p_realtimer.it_interval);
	u.u_r.r_val1 = 0;
	if (hzleft >= 0) {

	    /* Round to nearest second */

	    hzleft = (hzleft + HZ / 2) / HZ;

	    /* be sure never to return zero if the alarm has not gone off */

	    if (hzleft == 0)
		u.u_r.r_val1 = 1;
	    else
		u.u_r.r_val1 = hzleft;
	}
	if (uap->deltat == 0) {
		timerclear(&p->p_realtimer.it_value);
		splx(s);
		return;
	}
	p->p_realtimer.it_value = time;
	if (uap->deltat >= MAX_ALARM)
		p->p_realtimer.it_value.tv_sec += MAX_ALARM;
	else
		p->p_realtimer.it_value.tv_sec += uap->deltat;
	timeout(realitexpire, (caddr_t)p, hzto(&p->p_realtimer.it_value));
	splx(s);
}

onice()
{
	register struct a {
		int	niceness;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	register int n;

	n = uap->niceness;
	if((n<0 || n>2*NZERO) && !suser())
		n=0;
	donice(p, (p->p_nice-NZERO)+n);
	u.u_rval1 = p->p_nice-NZERO; /* ignored if u_error is set */
}

#include "../h/times.h"

#ifdef __hp9000s800
time_t
scalehz(tvp)
	register struct timeval *tvp;
{
	struct timeval tv;

	tvcopy(&tv, tvp);/* make a local copy (atomically) of the timeval */

	return (time_t)((tv.tv_sec * hz)+(tv.tv_usec / tick));
}
#endif

otimes()
{
	register struct a {
		struct	tms *tmsb;
	} *uap = (struct a *)u.u_ap;
	struct tms atms;

#ifdef __hp9000s300
	atms.tms_utime = u.u_procp->p_uticks;
	atms.tms_stime = u.u_procp->p_sticks;
	atms.tms_cutime = u.u_cru.ru_uticks;
	atms.tms_cstime = u.u_cru.ru_sticks;
#endif /* hp9000s300 */
#ifdef __hp9000s800
	atms.tms_utime = scalehz(&u.u_procp->p_utime);
	atms.tms_stime = scalehz(&u.u_procp->p_stime);
	atms.tms_cutime = scalehz(&u.u_cru.ru_utime);
	atms.tms_cstime = scalehz(&u.u_cru.ru_stime);
#endif /* hp9000s800 */
	u.u_error = copyout((caddr_t)&atms, (caddr_t)uap->tmsb, sizeof (atms));
	if (u.u_error == 0)
#ifdef __hp9000s800
		u.u_r.r_time = ticks_since_boot;
#endif /* hp9000s800 */
#ifdef __hp9000s300
		u.u_r.r_time = lbolt;
#endif /* hp9000s300 */
}


#ifdef __hp9000s300
ossig()
{
	struct a {
		int	signo;
		void    (*fun)();
	} *uap = (struct a *)u.u_ap;
	register int a;
	register void (*sig_handler)();
	int sig_mask;
	int sig_flags;
	struct proc *p = u.u_procp;

	a = uap->signo;
	sig_handler = uap->fun;
	if (a <= 0 || a >= NUSERSIG || a >= NSIG || a == SIGKILL ||
	    a == SIGRESERVE ||
#ifdef BSDJOBCTL
	    a == SIGSTOP ) {
#else  ! BSDJOBCTL
	    IS_JOBCTL_SIG(a)) {
#endif ! BSDJOBCTL
		u.u_error = EINVAL;
		return;
	}
	sig_mask  = 0;
	sig_flags = (SA_RESETHAND|SA_NOCLDSTOP);
	u.u_r.r_val1 = (int)u.u_signal[a-1];
	setsigvec(a, sig_handler, sig_mask, sig_flags);
}
#endif /* hp9000s300 */

#ifdef __hp9000s800

/*
 * For compare and swap operations, we first probe to make
 * sure user has write access and then turn off interrupts in
 * order to gain mutual exclusion of the specified address.
 * Note that we can take a page fault and/or a protection
 * fault on the address, but eventually we will execute the
 * LDW with interrupts turned off.  Then we simply compare and
 * swap the values.  Since this implementation turns off
 * interrupts for the mutual exclusion, it will only work on a
 * uni-processor.  We assume that the access rights of the
 * page containing the address will not change between the
 * call to useracc() and the references with interrupts turned
 * off.  The specified address must be word aligned and
 * for cds(2), the doubleint must not cross a page boundary.
 */

#define	unaligned(x)	(((unsigned)(x))&3)

/*
 * cs(2) - compare and swap
 */
cs()
{
	struct a {
		int	oldv;
		int	newv;
		int	*addr;
	} *uap = (struct a *)u.u_ap;
	int s;

	/*
	 * We must verify access, since there is no way to differentiate
	 * between an error and a valid -1 from fuword().
	 */
	if ((unaligned(uap->addr))
	    || (!useracc((caddr_t)uap->addr, sizeof(uap->newv), B_WRITE))) {
		u.u_error = EFAULT;
		return;
	}

	s = spl7();
	if (uap->oldv == fuword(uap->addr)) {
		suword(uap->addr, uap->newv);
		u.u_r.r_val1 = 1;
	} else
		u.u_r.r_val1 = 0;
	splx(s);
}


/*
 * cds(2) - compare and swap double
 */
cds()
{
	struct a {
		int	oldv[2];
		int	newv[2];
		int	*addr;
	} *uap = (struct a *)u.u_ap;
	int s;

	/*
	 * We must verify access, since there is no way to differentiate
	 * between an error and a valid -1 from fuword().
	 */
	if ((unaligned(uap->addr))
	    || (btop((caddr_t)&uap->addr[0]) != btop((caddr_t)&uap->addr[1]))
	    || (!useracc((caddr_t)uap->addr, sizeof(uap->newv), B_WRITE))) {
		u.u_error = EFAULT;
		return;
	}

	s = spl7();
	if (uap->oldv[0] == fuword(&uap->addr[1]) &&
	    uap->oldv[1] == fuword(&uap->addr[0])) {
		suword(&uap->addr[0], uap->newv[1]);
		suword(&uap->addr[1], uap->newv[0]);
		u.u_r.r_val1 = 1;
	} else
		u.u_r.r_val1 = 0;
	splx(s);
}
#endif hp9000s800
