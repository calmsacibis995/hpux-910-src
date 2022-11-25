/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/sys_gen.c,v $
 * $Revision: 1.40.83.9 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/03/09 15:44:22 $
 */

/* HPUX_ID: @(#)sys_gen.c	55.1		88/12/23 */

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
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/stat.h"
#ifdef SAR
#include "../h/sar.h"
#endif 
#ifdef MP
#include "../h/kern_sem.h"
#include "../h/sleep.h"
extern	lock_t *file_table_lock;
#endif	/* MP */
#include "../h/malloc.h"
#include "../h/poll.h"

/*
 * Read system call.
 */
read()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;
#ifdef SAR
	sysread++;
#endif /* hp9000s800 */

	aiov.iov_base = (caddr_t)uap->cbuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	rwuio(&auio, UIO_READ);
}

readv()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[MAXIOV];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > MAXIOV) {
		u.u_error = EINVAL;
		return;
	}

#ifdef SAR
	sysread++;
#endif /* hp9000s800 */
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_READ);
}

/*
 * Write system call
 */
write()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		int	count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

#ifdef SAR
	syswrite++;
#endif /* hp9000s800 */
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = uap->cbuf;
	aiov.iov_len = uap->count;
	rwuio(&auio, UIO_WRITE);
}

writev()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[MAXIOV];		/* XXX */

	if (uap->iovcnt <= 0 || uap->iovcnt > MAXIOV) {
		u.u_error = EINVAL;
		return;
	}

#ifdef SAR
	syswrite++;
#endif /* hp9000s800 */

	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_WRITE);
}

rwuio(uio, rw)
	register struct uio *uio;
	enum uio_rw rw;
{
	struct a {
		int	fdes;
	};
	register struct file *fp;
	register struct iovec *iov;
	int i, count;

	GETF(fp, (((struct a *)u.u_ap)->fdes));
	if ((fp->f_flag&(rw==UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	uio->uio_fpflags = fp->f_flag;
	uio->uio_resid = 0;
	uio->uio_seg = 0;
	iov = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	if ((u.u_procp->p_flag&SOUSIG) == 0 && setjmp(&u.u_qsave)) {
		if (uio->uio_resid == count)
			u.u_eosys = RESTARTSYS;
	} else
		u.u_error = (*fp->f_ops->fo_rw)(fp, rw, uio);
/* #ifdef hp9000s800 */
	/*	The following non-Sun/Berkeley test was added
	**	because the series 800 line and pty drivers were
	**	modified for reasons of reliability to sleep with PCATCH,
	**	preventing a successful return from the above setjmp.
	*/
	if ((u.u_error == EINTR) && ((u.u_procp->p_flag&SOUSIG) == 0)){
		u.u_error = 0;
		if (uio->uio_resid == count)
			u.u_eosys = RESTARTSYS;
	}
/* #endif hp9000s800 */
	u.u_r.r_val1 = count - uio->uio_resid;
#ifdef POSIX
	if (u.u_error == ENOSPC && u.u_r.r_val1 != 0)
		u.u_error = 0;
#endif POSIX
	SPINLOCK(file_table_lock);
	fp->f_offset += u.u_r.r_val1;
	SPINUNLOCK(file_table_lock);
	u.u_ru.ru_ioch += u.u_r.r_val1;	/* for System V accounting */
}

/*
 * Ioctl system call
 */
#ifdef __hp9000s300
#define	STACK_300_OVERFLOW_FIX
#endif /* __hp9000s300 */

#define	DATA_ON_STACK_SIZE	128

ioctl()
{
	register struct file *fp;
	struct a {
		int	fdes;
		int	cmd;
		caddr_t	cmarg;
	} *uap;
	register int com;
	register u_int size;
	caddr_t	data;
	char data_on_stack[DATA_ON_STACK_SIZE];
#ifndef	STACK_300_OVERFLOW_FIX
	label_t lqsave;
#endif

	uap = (struct a *)u.u_ap;
	if ((fp = getf(uap->fdes)) == NULL)
		return;
	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	com = uap->cmd;

#ifdef __hp9000s300 /* Old ioctl's */
	/*
	 * Map old style ioctl's into new for the
	 * sake of backwards compatibility (sigh).
	 */
	if ((com&~0xffff) == 0) {
		com = mapioctl(com);
		if (com == 0) {
			u.u_error = EINVAL;
			return;
		}
	}
#endif /* __hp9000s300 */

	/*
	 * Interpret high order 16 bits to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
#ifdef __hp9000s300 /* Old ioctl's */
	/* this is a kludge for commands that have the old IOC_VOID */
	if (com & 0x20000000) {
		com |= IOC_VOID;
		com &= ~0x20000000;
	}
	if ((com & (IOC_INOUT|IOC_VOID)) == 0) {
		u.u_error = EINVAL;
		return;
	}
#endif /* __hp9000s300 */
	size = (com &~ (IOC_INOUT|IOC_VOID)) >> 16;
	if (size) {
		if (size <= DATA_ON_STACK_SIZE) {
			/* very fast for small buffers */
			data = (caddr_t)data_on_stack;
		}
#ifdef	STACK_300_OVERFLOW_FIX
		else if (size <= 1018) {
			/*
			 * Might sleep here.  Macro collapses when size known
			 * at compile.
			 */
			VAPOR_MALLOC(data, caddr_t, 1018, M_IOCTLOPS, M_WAITOK);
		}
		else {
			/*
			 * Might sleep here.  Subroutine call, -- macro does
			 * not collapse.
			 */
			data = (caddr_t)vapor_malloc(size, M_IOCTLOPS, M_WAITOK);
		}
#else	/* STACK_300_OVERFLOW_FIX */
		else {
			MALLOC(data, caddr_t, size, M_IOCTLOPS, M_WAITOK);
			/* set up to handle an interrupted system call */
			lqsave = u.u_qsave;
			if (setjmp(&u.u_qsave)) {
			    FREE(data, M_IOCTLOPS);
			    u.u_error = EINTR;
			    return;
			}
		}
#endif	/* STACK_300_OVERFLOW_FIX */
	} else
		data = (caddr_t)&uap->cmarg;
	if (com&IOC_IN) {
		if (size) {
			u.u_error = copyin(uap->cmarg, data, size);
			if (u.u_error) {
#ifndef	STACK_300_OVERFLOW_FIX
				if (size > DATA_ON_STACK_SIZE)
					FREE(data, M_IOCTLOPS);
#endif	/* STACK_300_OVERFLOW_FIX */
				return;
			}
		}
	} else if ((com&IOC_OUT) && size) {
		/*
		 * Zero the buffer on the stack so the user
		 * always gets back something deterministic.
		 */
		bzero(data, size);
	}

	u.u_error = (*fp->f_ops->fo_ioctl)(fp, com, data);
	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (u.u_error == 0 && (com&IOC_OUT) && size)
		u.u_error = copyout(data, uap->cmarg, size);

#ifndef	STACK_300_OVERFLOW_FIX
	if (size > DATA_ON_STACK_SIZE) {
		FREE(data, M_IOCTLOPS);
		u.u_qsave = lqsave;
	}
#endif	/* STACK_300_OVERFLOW_FIX */
}

int	unselect();
int	nselcoll;
/*
 * Select system call.
 */

#define SELECT_PERF_SIZE 2		/* two int's worth of bits */

select()
{
	register struct uap  {
		int	nd;
		fd_set	*in, *ou, *ex;
		struct	timeval *tv;
	} *uap = (struct uap *)u.u_ap;
	fd_set *ibits, *obits;
	struct {
		fd_mask fd_bits[SELECT_PERF_SIZE];
	} stack_ibits[3], stack_obits[3];

	struct timeval atv;
	int s, ncoll, ni;
	label_t lqsave;
	int timeout_hz;

	if (uap->nd > u.u_maxof)
		uap->nd = u.u_maxof;	/* forgiving, if slightly wrong */
	ni = howmany(uap->nd, NFDBITS);

	if (uap->nd < 0) {
		u.u_error = EINVAL;
		return;
	}

	/* If the bitmasks don't fit into the local arrays, allocate space */
	if (uap->nd > SELECT_PERF_SIZE*NFDBITS) {
		MALLOC(ibits, fd_set *, 6*sizeof(fd_set), M_IOSYS, M_WAITOK);
		bzero((caddr_t) ibits, 6*sizeof(fd_set));
		obits = &ibits[3];
	}
	else {
		bzero((caddr_t)stack_ibits, sizeof(stack_ibits));
		bzero((caddr_t)stack_obits, sizeof(stack_obits));
		ibits = stack_ibits;
		obits = stack_obits;
	}

#define	getbits(name, x) \
	if (uap->name) { \
		u.u_error = copyin( \
		    (caddr_t) uap->name, \
		    (caddr_t) (ibits==stack_ibits ? stack_ibits+x : ibits+x), \
		    (unsigned) (ni*sizeof(fd_mask))); \
		if (u.u_error) \
			goto done; \
	}
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits
	if (uap->tv) {
		u.u_error = copyin((caddr_t)uap->tv, (caddr_t)&atv,
			sizeof (atv));
		if (u.u_error)
			goto done;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			goto done;
		}

		/* Convert select timeout to hz. If too large, use */
		/* maximum hz value.                               */

		if (atv.tv_sec <= 0x7fffffff / HZ - HZ)
		    timeout_hz = atv.tv_sec * HZ + ((atv.tv_usec * HZ + 500000) / 1000000);
		else
		    timeout_hz = 0x7fffffff;
	}
retry:
	ncoll = nselcoll;
	spinlock(sched_lock);
	u.u_procp->p_flag |= SSEL;
	spinunlock(sched_lock);
	u.u_r.r_val1 = selscan(ibits, obits, uap->nd);
	if (u.u_error || u.u_r.r_val1)
		goto done;

	if (uap->tv && timeout_hz <= 0)
	    goto done;

	s = spl6();
	if ((u.u_procp->p_flag & SSEL) == 0 || nselcoll != ncoll) {
		SPINLOCK(sched_lock);
		u.u_procp->p_flag &= ~SSEL;
		SPINUNLOCK(sched_lock);
		splx(s);
		goto retry;
	}
	SPINLOCK(sched_lock);
	u.u_procp->p_flag &= ~SSEL;
	SPINUNLOCK(sched_lock);


	/*
	 *	PHKL_2414 - based on S800 PHKL_2258
	 *
	 *	In MRed 9.0, there is a leak here; if a
	 *	process does a select on a file descriptor >= 64
	 *	and the select is interrupted, the kernel does not
	 *	FREE the memory it MALLOCed - things like NCS & YP
	 *	do this stuff, and as a result have a tendency to 
	 *	cripple 9.0 systems rather quickly.  The fix is to
	 *	*always* setjmp here, whether a specific timeout was
	 *	specified or not.
	 */

	/*
	 * Note: if we get a signal but no timer
	 * was set then the masks will remain as
	 * they were on input.  The code will take
	 * a longjmp into syscall and syscall will
	 * return with EINTR.
	 */

	lqsave = u.u_qsave;
	if (setjmp(&u.u_qsave)) {
		if (uap->tv) 
			untimeout(unselect, (caddr_t)u.u_procp);
		u.u_error = EINTR;
		splx(s);
		goto free_the_bits;
	}
	if (uap->tv) 
		timeout(unselect, (caddr_t)u.u_procp, timeout_hz);

	sleep((caddr_t)&selwait, PZERO+1);
	u.u_qsave = lqsave;

	/*
	 *  untimeout returns the number of hz left until 
	 *  the timeout would have gone off, or -1 if it  
	 *  has already gone off.                         
	 */
	if (uap->tv) 
		timeout_hz = untimeout(unselect, (caddr_t)u.u_procp);


	splx(s);
	goto retry;
done:
#define	putbits(name, x) \
	if (uap->name) { \
		int error = copyout( \
		      (caddr_t) (ibits==stack_ibits ? stack_obits+x : obits+x),\
		      (caddr_t) uap->name, \
		      (unsigned) (ni*sizeof(fd_mask))); \
		if (error) \
			u.u_error = error; \
	}
	if (u.u_error==0) {
		putbits(in, 0);
		putbits(ou, 1);
		putbits(ex, 2);
	}
#undef putbits
free_the_bits:
	if (ibits != stack_ibits)
		FREE((caddr_t) ibits, M_IOSYS);
}

unselect(p)
	register struct proc *p;
{
	register int s = spl6();
#ifdef	MP
	register sema_t	*sleep_sema;

	sleep_sema = &slpsem[ HASH( &selwait ) ];
        /*
         * Wake the specific process
         * Assume its sleeping. If its stopped, that's ok.
         * wsync can handle stopped processes too.
         */

        MP_ASSERT( (p->p_sleep_sema == sleep_sema),
                   "Unselect: Unexpected sleep semaphore for process");

        wsync(sleep_sema, p);

#else	! MP
	switch (p->p_stat) {

	case SSLEEP:
		setrun(p);
		break;

	case SSTOP:
		unsleep(p);
		break;
	}
#endif	! MP
	splx(s);
}

selscan(ibits, obits, nfd)
register fd_mask *ibits, *obits;
register int nfd;
{
	register int seltype, fd, index, i;
	register fd_mask bits;
	register int s, ni, n = 0, mask;
	register struct file *fp, **fpp;
	static int types[] = {FREAD, FWRITE, 0 /* exception==0 */};

	s = spl6();

	/* How big are these arrays, anyway? */
	ni = nfd > SELECT_PERF_SIZE*NFDBITS
	    ? sizeof(fd_set)/sizeof(fd_mask)
	    : SELECT_PERF_SIZE;

	for (seltype = 0; seltype < 3; seltype++) {
		for (index=i=0; i<nfd; i+=SFDCHUNK, index++) {
			bits=ibits[index];
			if (bits==0)
				continue;

			/*
			 *    SWFfc00361 -- 8.0 & 9.0 blindly set fpp
			 *	just before the "for (index..."	loop above,
			 *	and then dereference it in the inner loop
			 *	below; the problem is that when all of the
			 *	descriptors for a chunk are freed (by 
			 *	close(2)), the chunk is given back, so the
			 *	pointer can be null ==> kernel bus error.
			 *	The fix is to check and make sure the 
			 *	chunk is really there.	I also eliminated
			 *	the calls to getf() (another fix would have
			 *	been to always call getf())   DKM 8-30-93
			 */
			if (u.u_ofilep[index] == NULL) {
				u.u_error = EBADF;
				break;
			}
			fpp = u.u_ofilep[index]->ofile;

			for (mask=1, fd=i; bits && fd<nfd; fd++, mask<<=1) {
				if ((bits & mask)==0)	/* anything wanted? */
					continue;	/* no, don't ask    */
				fp = fpp[fd % SFDCHUNK];/* get file ptr     */
				if (fp == NULL) {	/* is it good?      */
					u.u_error = EBADF;
					break;
				}
				u.u_fp = fp;
				if ((*fp->f_ops->fo_select)(fp,types[seltype])){
					obits[index] |= mask;
					n++;
				}
				bits &= ~mask;
			}
		}
		ibits += ni;
		obits += ni;
	}

	splx(s);
	return n;
}

/*ARGSUSED*/
seltrue(dev, flag)
dev_t dev;
int flag;
{
	switch (flag) {
	case FREAD:		/* For the traditional select() flags, */
	case FWRITE:		/* seltrue() must return success. */
	case 0:			/* Even when selecting for exceptions. */
		return 1;
	default:		/* This must be a new poll() condition, */
		return 0;	/* so we return false. */
	}
}

unsigned _nselwakeups, _ncolls, _nsetruns;

selwakeup(p, coll)
	register struct proc *p;
	int coll;
{
	_nselwakeups++;
	if (coll) {
		_ncolls++;
		nselcoll++;
		wakeup((caddr_t)&selwait);
	}
	if (p) {
		int s = spl6();
		_nsetruns++;
		if (p->p_wchan == (caddr_t)&selwait)
			unselect(p);
		else if (p->p_flag & SSEL){
			SPINLOCK(sched_lock);
			p->p_flag &= ~SSEL;
			SPINUNLOCK(sched_lock);
		}
		splx(s);
	}
}

/*
 * Poll system call
 *
 * poll(fdp, nfds, timo)
 * struct pollfd *fdp;		array of things to check
 * int nfds;			how many pollfd's there are
 * long timo;			-1=wait forever 0=scan once >0=that many ms
 *
 * The big picture:
 *
 * The user calls poll() with a bunch of records (struct pollfd).  Each pollfd
 * has the following fields:
 *
 *	int fd		file descriptor of interest
 *	short events	events of interest on fd (input)
 *	short revents	events that occurred on fd (output)
 *
 * The poll() code will look at the pollfds, one at a time.
 * Each bit set in events corresponds to a condition that the user is
 * interested in (for example, POLLIN refers to readable data).  For each
 * bit set in events, poll() will call the appropriate *_select() routine
 * with a flag indicating the condition of interest.
 *
 * The job of the *_select() routine is:
 *
 *	if the condition is true, return 1
 *	if the condition is false, return 0 and remember that who is
 *	interested in the condition.
 *
 * After poll() calls a *_select() routine, it sets the appropriate bit
 * in revents if the *_select() routine returned 1.
 *
 * After poll() has called all of the *_select() routines, it looks to see
 * how many routines succeeded.  If any succeeded, it returns to the user.
 * If none succeeded, it puts the process to sleep.  Later, when the
 * condition becomes true (e.g., readable data arrives), the driver will
 * wake up the process that is sleeping on the condition, and poll() will
 * call all the *_select() routines again to see who woke it up.
 * And the beat goes on...
 *
 * If, however, the condition never becomes true, the user's timeout will
 * wake up the process, and poll will return.
 */

/* Note that routine is not MP-tested, even thought I tried my best. */

/*
 * The supreme being gives instructions to temple priests:
 * (Ezekiel, chapter 44, verse 20)
 *
 *	Neither shall they shave their heads, nor suffer their locks to
 *	grow long; they shall only poll their heads.
 */

struct file *getf_no_ue();

poll()
{
	register struct uap  {
		struct	pollfd *fdp;	/* array of things to check */
		int nfds;		/* number of pollfd's */
		long timo;  /* -1=wait forever 0=scan once >0: that many ms */

	} *uap = (struct uap *)u.u_ap;
	register int timeout_hz, ncoll, s;
	label_t lqsave;

	if (uap->timo < -1 || uap->nfds < 0) {
		u.u_error = EINVAL;
		return;
	}

	if (uap->timo >= 0) {
		/*
		 * Convert select timeout to hz.
		 * 2^31 milli-seconds is 24.85+ days.
		 * Since MAX_ALARM is guaranteed to be at least 31 days,
		 * it will always fit.  The timeout must be signed, since
		 * a timeout of -1 means wait forever.
		 */
		timeout_hz = uap->timo / (1000 / HZ);

		/*
		 * The above code assumes that 1000/HZ doesn't lose any
		 * precision.  Let's enforce that here.
		 */
#if 1000 % HZ
		*** panic "HZ must go evenly into 1000" ***
#endif
	}


	/* Scan fds until an event is found or the timeout is reached. */

	for(;;) {
		/*
		 * Polling the fds is a relatively long process.  Set the SSEL
		 * flag so that we can see if something happened to an fd after
		 * we checked it but before we go to sleep.
		 *
		 * The SSEL flag is set here, and cleared if somebody calls
		 * selwakeup() (which they do if data arrives or in some other
		 * way a polled-for condition becomes true).  The selwakeup()
		 * routine clears SSEL, which signals the code below to try the
		 * loop again.
		 */
		ncoll = nselcoll;
		spinlock(sched_lock);
		u.u_procp->p_flag |= SSEL;
		spinunlock(sched_lock);

		u.u_r.r_val1 = pollscan(uap->fdp, uap->nfds);
		if (u.u_error || u.u_r.r_val1)
			break;

		if (uap->timo >= 0 && timeout_hz <= 0)	/* timeout expired? */
			break;

		s = spl6();
		SPINLOCK(sched_lock);
		if ((u.u_procp->p_flag & SSEL) == 0 || nselcoll != ncoll) {
			u.u_procp->p_flag &= ~SSEL;
			SPINUNLOCK(sched_lock);
			splx(s);
			continue;
		}
		u.u_procp->p_flag &= ~SSEL;
		SPINUNLOCK(sched_lock);
		if (uap->timo > 0) {
			/*
			 * Note: if we get a signal but no timer was set then
			 * the masks will remain as they were on input.
			 * The code will take a longjmp into syscall and
			 * syscall will return with EINTR.
			 */
			lqsave = u.u_qsave;
			if (setjmp(&u.u_qsave)) {
				untimeout(unselect, (caddr_t)u.u_procp);
				u.u_error = EINTR;
				splx(s);
				return;
			}
			timeout(unselect, (caddr_t)u.u_procp, timeout_hz);
		}
		sleep((caddr_t)&selwait, PZERO+1);
		if (uap->timo > 0) {
			u.u_qsave = lqsave;

			/*
			 * untimeout returns the number of hz left until
			 * the timeout would have gone off, or -1 if it
			 * has already gone off.
			 */

			timeout_hz = untimeout(unselect, (caddr_t)u.u_procp);
		}
		splx(s);
	}

	/*
         * Call polldone to allow streams and other drivers to clean
         * up internal data structures when the poll call completes
         */
        polldone(uap->fdp, uap->nfds);
}

#define NPOLLFILE 20

int
pollscan(fdp, nfds)
struct pollfd *fdp;
{
	int response_count, flag, size, i, j, x, sel_flag;
	struct pollfd pollfd[NPOLLFILE], *p;
	struct file *fp;

	/*
	 * Check fd's for specified events. 
	 * Read in pollfd records in blocks of NPOLLFILE.  Test each fd in the
	 * block and store the result of the test in the event field of the
	 * in-core record.  After a block of fds is finished, write the result
	 * out to the user.  Note that if no event is found, the whole
	 * procedure will be repeated after awakenening from the sleep
	 * (subject to timeout).
	 */

	response_count=0;

	for (i = 0; u.u_error==0 && i < nfds; i++) {
		j = i % NPOLLFILE;
		/* If we're just starting a block, read it in.  */
		if (j==0) {			/* Just starting a block */
			size = min(nfds - i, NPOLLFILE);
			if (copyin(fdp, pollfd, size*sizeof(*fdp))) {
				u.u_error = EFAULT;
				return 0;
			}
		}

		p = &pollfd[j];
		p->revents = 0;
		if (p->fd < 0)			/* Negative file descriptor? */
			goto skip;		/* Strange but documented. */

		fp = getf_no_ue(p->fd);

		if (fp == NULL) {		/* Is this a valid fd? */
			p->revents = POLLNVAL;	/* No, it's not. */
			goto skip;
		}

		/* Now, poll for a few special events: */
		/*
		 * They tell me that streams can return POLLNVAL for
		 * a valid file descriptor.  Don't ask me why...
		 */
		if ((*fp->f_ops->fo_select)(fp, POLLNVAL)) {
			p->revents = POLLNVAL;
			goto skip;
		}
		if ((*fp->f_ops->fo_select)(fp, POLLERR)) {
			p->revents = POLLERR;
			goto skip;
		}
		/*
		 * We always poll for POLLHUP, but it's not exclusive
		 * with respect to the other conditions, so we just include
		 * it with the requested flags.
		 */

		/* For each flag in events, call the select routine */
		for (x = p->events & ~(POLLNVAL|POLLERR) | POLLHUP; x; ) {

			flag = (x ^ x-1) + 1 >> 1;/* extract first set bit */
			x &= ~flag;		/* remove it from word */
			switch (flag) {
			case USER_POLLPRI: sel_flag = POLLPRI; break;
			case POLLIN:	sel_flag = FREAD;  break;
			case POLLOUT:	sel_flag = FWRITE; break;
			default:	sel_flag = flag;   break;
			}
			if ((*fp->f_ops->fo_select)(fp, sel_flag))
				p->revents |= flag;
		}

skip:		if (p->revents)
			response_count++;

		/* Copy stuff back out if we just finished a block */
		if (j==NPOLLFILE-1 || i==nfds-1) {
			if (copyout(pollfd, fdp, size*sizeof(*fdp)))
				u.u_error = EFAULT;
			fdp += size;
		}
	}

	return response_count;
}


int
polldone(fdp, nfds)
struct pollfd *fdp;
{
        int size, i, j;
        struct pollfd pollfd[NPOLLFILE], *p;
        struct file *fp;

        /*
         * Call fds with POLLDONE event to let them know
         * that poll is completing.  Streams and other drivers
         * can clean up their data structures this way.
         */

        for (i = 0; u.u_error==0 && i < nfds; i++) {
                j = i % NPOLLFILE;
                /* If we're just starting a block, read it in.  */
                if (j==0) {                     /* Just starting a block */
                        size = min(nfds - i, NPOLLFILE);
                        if (copyin(fdp, pollfd, size*sizeof(*fdp))) {
                                u.u_error = EFAULT;
                                return 0;
                        }
                }

                p = &pollfd[j];
                if (p->fd < 0)                  /* Negative file descriptor? */
                        continue;               /* Strange but documented. */

                fp = getf_no_ue(p->fd);

                if (fp == NULL)                 /* Is this a valid fd? */
                        continue;               /* No, it's not. */

                /* Call select with POLLDONE flag */
                (*fp->f_ops->fo_select)(fp, POLLDONE);

        }

        return 0;
}
