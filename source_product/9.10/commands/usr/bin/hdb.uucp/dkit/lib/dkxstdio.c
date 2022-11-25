/*	@(#) $Revision: 64.1 $	*/
/*
 *	Perform Standard I/O functions for
 * 	a remote process using the dkxqt protocol
 */
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include <sys/types.h>
#include <sys/remfio.h>
#include <errno.h>
#include <dk.h>
#include <signal.h>
#ifdef	SVR3
#	define	SIGRTN	void
#else
#	define	SIGRTN	int
#endif
#include <termio.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ttold.h>
#include <sysexits.h>
#include <setjmp.h>
#include <stdio.h>

#define	F_SGTTYB	"bbbbs"
#define	F_TERMIO	"ssssbbbbbbbbb"
#define	FL_SGTTYB	6
#define	FL_TERMIO	17

	static char		sb[REMSIZE], rb[REMSIZE];

	static struct rem_req	r;
	static struct rem_reply	s;

	extern int		dk_verbose;

#define BSIZE 1024

	static char		buf[BSIZE];

	static union {
		struct termio ioctl_termio ;
		struct sgttyb ioctl_sgttyb ;
	} ioctl_buf ;

	static struct {
		short	signo;
		short	size;
	} sigpcb = {SIGUSR1, REMSIZE};

	static struct {
		short	resid ;
		short	reason ;
		short	ctlchar ;
	} qqabo ;

	static short		sig;
	static short		uSIG;
	static int		exitcode;
	static short		readmode[3] = {0, 0, 0}; /* demand full length */

	static jmp_buf		wayout;
	static struct termio	origterm;
	static int		otfd;		/* Terminal fd, if any */
	static int		otrestore;	/* stty modes changed flag */
	static int		eofmark;
	static int		origfmode[3];	/* original F_GETFL modes */
	static int		fmode[3];	/* current F_SETFL modes */
	
	static SIGRTN		catchsignal(), catchcancel();


/*
** This is the local server for remote execution (login) standard I/O.
*/

dkxstdio(fd)
{
	register int 	n;
	SIGRTN		(*intwas)(), (*quitwas)(), (*usr1was)();
#if hpux
	SIGRTN		(*tstpwas)();
#endif

	exitcode = -EX_IOERR;	/* in case other side hangs up unexpectedly */

	eofmark = 0;
	otrestore = 0;

	if((intwas = signal(SIGINT, SIG_IGN)) != SIG_IGN)
		(void) signal(SIGINT, catchsignal);
	if((quitwas = signal(SIGQUIT, SIG_IGN)) != SIG_IGN)
		(void) signal(SIGQUIT, catchsignal);
	if((usr1was = signal(SIGUSR1, SIG_IGN)) != SIG_IGN)
		(void) signal(SIGUSR1, catchcancel);
#if hpux
	if((tstpwas = signal(SIGTSTP, SIG_IGN)) != SIG_IGN)
		(void) signal(SIGTSTP, catchsignal);
#endif

	otfd = -1;

	ioctl(fd, DIOCRMODE, readmode);

	for(n = 0; n < 3; n++)
		fmode[n] = origfmode[n] = fcntl(n, F_GETFL, 0);

	if (intwas != SIG_IGN)
		for(n = 0; n < 3; n++)
			if(ioctl(n, TCGETA, &origterm) == 0){
				otfd = n;
				break;
			}

	if(setjmp(wayout)){
		if ((otfd == 0)  ||  (exitcode && otrestore)) {
			ioctl(otfd, TCSETA, &origterm);
			for(n = 0; n < 3; n++)
				fcntl(n, F_SETFL, origfmode[n]);
		}

		(void) signal(SIGUSR1, usr1was);
		(void) signal(SIGQUIT, quitwas);
		(void) signal(SIGINT, intwas);
#if hpux
		(void) signal(SIGTSTP, tstpwas);
#endif
		return(exitcode);
	}

	while (1) {
		int	rdcnt;

		errno = 0;
		uSIG = 0;
		if (((rdcnt = read(fd, rb, REMSIZE)) != REMSIZE) && (errno == 0)) {
			if (sig == 0) {
				ioctl(fd, DIOCQQABO, &qqabo);
				if (qqabo.ctlchar)	
					continue;
				if (rdcnt) {
					fprintf(stderr,"remreq count WRONG!\n");
					exitcode = -EX_PROTOCOL;
				}
				longjmp(wayout, 1);
			} else {
				errno = EINTR;
			}
		}
		if (errno == 0) {
			dkfcanon(F_REMREQ, rb, &r);

			/*
			** This first switch determines if additional data
			** is to be read in, and when necessary, reads it in.
			*/

			switch(r.r_type) {
			case RWRITE:
			case RIOCTL:
				break;
			default:
				if (r.r_length  &&  r.r_length <= sizeof(buf)) {
					n = read(fd, buf, r.r_length);
					if (n != r.r_length) {
						if (dk_verbose)
							perror("dkxstdio: dk read error (1)");
						exitcode = -EX_PROTOCOL;
						longjmp(wayout, 1);
					}
				}
			}

			/*
			** This second switch calls the appropriate
			** handler routine to provide the requested 
			** service.
			*/

			s.s_length = 0;
			s.s_type = r.r_type;
			errno = 0;

			switch (r.r_type) {
			case RREAD:
				rfread(fd);
				break;
			case RWRITE:
				rfwrite(fd);
				break;
			case RIOCTL:
				rfioctl(fd);
				if (eofmark)
					longjmp(wayout, 1);
				break;
			case RCANCEL:
				break;
			default:
				errno = EINVAL;
			}

			/*
			** This final switch sends a response message
			** back to the requestor, along with any
			** additional required data.
			*/

			switch (r.r_type) {
			case RIOCTL:
			case RCANCEL:
				break;
			default:
				s.s_error = errno;
				dktcanon(F_REMREPLY, &s, sb);
				msgsend(fd, sb, REMSIZE);
				if (s.s_length)
					msgsend(fd, buf, s.s_length);
			}
		} else if (sig) {
			sendsignal(fd);
		} else {
			if(dk_verbose)
				perror("dkxstdio: dk read error (2)") ;
			exitcode = -EX_PROTOCOL;
			longjmp(wayout, 1);
		}
	}
}

/*
** This routine handles remote read requests.
*/

	static
rfread(fd)
{
	register int		rfd, temp;
	register unsigned	len, rlen;
	int			rsig;

	len = r.r_var.rread.r_count;
	if (len > BSIZE)
		len = BSIZE;
	rfd = (r.r_file < 3) ? r.r_file : 0;
	temp = r.r_mode + FOPEN;
	if(fmode[rfd] != temp);
		fcntl(rfd, F_SETFL, fmode[rfd] = temp);

	ioctl(fd, DIOCSIG, &sigpcb);	/* watch for cancels while we read */
	rlen = read(rfd, buf, len);
	if (rsig = sig) {
		errno = EINTR;
	}
	temp = errno;
	if ((int) rlen == -1)
		rlen = 0;
	if (sig)
		sendsignal(fd);
	s.s_length = rlen;
	if (rlen == len && len != r.r_var.rread.r_count && temp == 0)
		s.s_resid = r.r_var.rread.r_count - len;
	else
		s.s_resid = 0;
	errno = temp;
}

/*
** This routine handles remote write requests
*/

	static
rfwrite(fd)
{
	register int		rfd, temp;
	register unsigned	len, rlen;

	s.s_count = 0;
	rfd = (r.r_file < 3) ? r.r_file : 1;
	temp = r.r_mode + FOPEN;
	if(fmode[rfd] != temp);
		fcntl(rfd, F_SETFL, fmode[rfd] = temp);

	while (len = r.r_length) {
		errno = 0;
		if (len > BSIZE)
			len = BSIZE;
		rlen = read(fd, buf, len);
		ioctl(fd, DIOCQQABO, &qqabo);
		if (qqabo.resid  ||  qqabo.ctlchar  ||  rlen != len) {
			if (sig)
				sendsignal(fd);
			if (!qqabo.ctlchar)
				discard(fd, qqabo.resid);
			if (sig)
				sendsignal(fd);
			errno = EINTR;
			return;
		}
		r.r_length -= len;
		temp = write(rfd, buf, len);
		if (temp > 0)
			s.s_count += temp;
		if (sig) {
			sendsignal(fd);
			discard(fd, r.r_length);
			r.r_length = 0;
		}
	}
	errno = 0;
}

/*
** This routine handles remote ioctl requests.
*/

	static
rfioctl(fd)
{
	short		need, give, fmtl;
	int		narg;
	register char	*fmt;

	fmtl = need = give = 0;
	narg = r.r_var.rioctl.r_arg;
	switch (r.r_var.rioctl.r_cmd) {
	case TCSETAF:
	case TCSETAW:
	case TCSETA:
		need = sizeof (struct termio);
		fmt  = F_TERMIO;
		fmtl = FL_TERMIO;
		otrestore = 1;		/* Mark stty changed */
		break;
	case TIOCSETP:
		need = sizeof(struct sgttyb);
		fmt  = F_SGTTYB;
		fmtl = FL_SGTTYB;
		otrestore = 1;		/* Mark stty changed */
		break;
	case TCGETA:
		give = sizeof(struct termio);
		fmt  = F_TERMIO;
		fmtl = FL_TERMIO;
		break;
	case TIOCGETP:
		give = sizeof(struct sgttyb);
		fmt  = F_SGTTYB;
		fmtl = FL_SGTTYB;
		break;
	case DXIOEXIT:
		exitcode = narg;
		eofmark++;
#if hpux
		need = 4;
		break;
	default:
		{
		int cmd;

		cmd = r.r_var.rioctl.r_cmd;
		need = (cmd & IOC_IN)?((cmd & IOCSIZE_MASK)>> 16) : 0;
		give = (cmd & IOC_OUT)?((cmd & IOCSIZE_MASK)>> 16) : 0;
		fmt = "bbbbbbbbbb";
		fmtl = 10;
		}
#endif
	}
	if (need || give)
		narg = (int)&ioctl_buf;
	if (need > r.r_length) {
		discard(fd, r.r_length);
		s.s_length = fmtl;
		s.s_error = 0;
		s.s_count = need;
		s.s_resid = fmtl;
		dktcanon(F_REMREPLY, &s, sb);
		msgsend(fd, sb, REMSIZE);
		if (fmtl)
			msgsend(fd, fmt, fmtl);
		return;
	}
	if (need) {
		read(fd, &ioctl_buf, need);
#ifdef hpux
		/*
		 * for HP to HP we do not do conversion.
		 */
#else
		dkfcanon(fmt, &ioctl_buf, &ioctl_buf);
#endif
	}
	discard(fd, r.r_length - need);
	errno = 0;
	if(!eofmark) {
		ioctl((r.r_file < 3) ? r.r_file : 0, r.r_var.rioctl.r_cmd,narg);
	}
#if hpux
	/*
	 * try to get the exit status.
	 */
	else {
		dkfcanon("l", &ioctl_buf, &exitcode);
	}
#endif
	s.s_length = give + fmtl;
	s.s_error = errno;
	s.s_count = 0;
	s.s_resid = fmtl;
	dktcanon(F_REMREPLY, &s, sb);
	msgsend(fd, sb, REMSIZE);
	if (fmtl)
		msgsend(fd, fmt, fmtl);
	if (give) {
#ifdef hpux
		/*
		 * No conversion is needed for HP to HP.
		 */
#else
		dktcanon(fmt, &ioctl_buf, &ioctl_buf);
#endif
		msgsend(fd, &ioctl_buf, give);
	}
}

/*
** This routine is called to discard residual data left in the channel
** by signals, etc.
*/

	static
discard(fd, len)
{
	register unsigned rlen = 1;	/* fake */

	ioctl(fd, DIOCQQABO, &qqabo);
	while (len  &&  rlen > 0  &&  !qqabo.ctlchar) {
		rlen = (len > BSIZE) ? BSIZE : len;
		errno = 0;
		rlen = read(fd, buf, rlen);
		len -= rlen;
		ioctl(fd, DIOCQQABO, &qqabo);
	}
}

/*
** This routine catches signals.  Quit signals are timed so that a double
** quit within two seconds can cause termination.
*/

	static SIGRTN
catchsignal(signo)
{
	static long	lasttime;
	long		now, time();

	sig = signo;
	(void) signal(signo, catchsignal);
	if(signo == SIGQUIT){
		time(&now);
		if((now - lasttime) < 2){
			exitcode = -EX_NOINPUT;
			longjmp(wayout, 1);
		}
		lasttime = now;
	}
}



/*
** This routine is used to send signal messages to the remote side.
*/

	static
sendsignal(fd)
{
	if (sig) {
		s.s_length = 0;
		s.s_type = RSIGNAL;
		s.s_error = sig;
		sig = 0;
		dktcanon(F_REMREPLY, &s, sb);
		msgsend(fd, sb, REMSIZE);
		s.s_type = r.r_type;
	}
}

/*
** This routine is use to catch the 'cancel' signals caused by the DIOCSIG
** ioctl.
*/

	static SIGRTN
catchcancel(signo)
{
	uSIG++;
	(void) signal(signo, catchcancel);
}

/*
** This routine is used to write a message to the remote end.  The write
** is repeated until either it succeeds or a hard (non-EINTR) error occurs.
*/

	static
msgsend(fd, addr, count)
	char	*addr;
{
	int	ret;

	while  ((ret = write(fd, addr, count)) < 0  &&  errno == EINTR)
		/* Do nothing */
		;
	
	return(ret);
}
