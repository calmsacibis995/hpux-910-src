/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/tty.c,v $
 * $Revision: 1.8.84.5 $	$Author: rpc $
 * $State: Exp $   	$Locker: rpc $
 * $Date: 94/09/19 12:44:24 $
 */
/* HPUX_ID: @(#)tty.c+timeout fix	52.5cmm		88/07/18 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
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
 * general TTY subroutines
 */
#include "../h/param.h"
#include "../h/user.h"
#include "../h/systm.h"
#include "../h/tty.h"
#include "../h/ld_dvr.h"
#include "../h/unistd.h"	/* for _POSIX_VDISABLE */
#define _INCLUDE_TERMIO
#include "../h/termios.h"
#ifdef	BSDJOBCTL
#include "../h/bsdtty.h"
#endif	BSDJOBCTL
#include "../h/ttold.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/modem.h"
#include "../h/dk.h"

extern char partab[];

/*
 * tty low and high water marks
 * high < TTYHOG
 */
int	tthiwat[32] = {
	  0,  60,  60,  60,	/*      0,    50,    75,   110 */
	 60,  60,  60, 120,	/*    134,   150,   200,   300 */
	120, 120, 180, 180,	/*    600,   900,  1200,  1800 */
	240, 240, 240, 240,	/*   2400,  3600,  4800,  7200 */
	240, 240, 300, 300,	/*   9600, 19200, 38400, 57600 */
	300, 300, 300,   0,	/* 115200,230400,460800,  (27) */
	  0,   0,   0,   0,	/*   (30),  (31),  (32),  (33) */
	  0,   0, 100, 100,	/*   (34),  (35),  EXTA,  EXTB */
};
int	ttlowat[32] = {
	  0,  20,  20,  20,	/*      0,    50,    75,   110 */
	 20,  20,  20,  40,	/*    134,   150,   200,   300 */
	 40,  40,  60,  60,	/*    600,   900,  1200,  1800 */
	 80,  80,  80,  80,	/*   2400,  3600,  4800,  7200 */
	 80,  80,  80, 100,	/*   9600, 19200, 38400, 57600 */
	100, 100, 100,   0,	/* 115200,230400,460800,  (27) */
	  0,   0,   0,   0,	/*   (30),  (31),  (32),  (33) */
	  0,   0,  50,  50,	/*   (34),  (35),  EXTA,  EXTB */
};

#ifdef BSDJOBCTL
char    ttcchar[NLDCC] = {
	CINTR,		/* VINTR */
	CQUIT,		/* VQUIT */
	CERASE,		/* VERASE */
	CKILL,		/* VKILL */
	CEOF,		/* VEOF */
	0,		/* VEOL */
	0,		/* Reserved - AT&T */
	0,		/* Reserved - AT&T */
	0377,		/* Reserved - HP */
	0377,		/* Reserved - HP */
	0377,		/* Reserved - HP */
	CEOF,		/* VMIN  (POSIX) */
	0,		/* VTIME (POSIX) */
	0377,		/* VSUSP */
	CSTART,		/* VSTART (POSIX) */
	CSTOP,		/* VSTOP  (POSIX) */
	0377		/* VDSUSP */
};
#else  ! BSDJOBCTL
char	ttcchar[NCCS] = {
	CINTR,
	CQUIT,
	CERASE,
	CKILL,
	CEOF,
	0,		/* VEOL */
	0,		/* Reserved - AT&T */
	0,		/* Reserved - AT&T */
	0,		/* Reserved - HP */
	0,		/* Reserved - HP */
	0,		/* Reserved - HP */
	0,		/* Reserved - HP */
	CEOF,		/* VMIN  (POSIX) */
	0,		/* VTIME (POSIX) */
	CSTART,		/* VSTART (POSIX) */
	CSTART		/* VSTOP  (POSIX) */
};
#endif ! BSDJOBCTL

/* null clist header */
struct clist ttnulq;

/* Temporary until an appropriate entry can be made in tty.h */
/* Currently this is the same as HIDDENMODE_ENABLED which is not used. */
#define	VHANGUP	0x80

/* canon buffer */
char	canonb[CANBSIZ];
/*
 * Input mapping table-- if an entry is non-zero, when the
 * corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */
char	maptab[] = {
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,'|',000,000,000,000,000,'`',
	'{','}',000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,'~',000,
	000,'A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W',
	'X','Y','Z',000,000,000,000,000,
};

/*
 * routine called on first teletype open.
 * establishes a process group for distribution
 * of quits and interrupts from the tty.
 */
ttopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	tp->t_state &= ~(ASYNC | NBIO);
	tp->t_pidprc = u.u_procp;
	tp->t_dev = dev;
	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;
	return (0);
}

ttclose(tp)
register struct tty *tp;
/*---------------------------------------------------------------------------
 | FUNCTION -		Called on last close. 
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 |
 | 29 May 92	marcel  Added call to clear_ctty_from_session to clear any
 |			dangling pointers to this devices as a ctty. Fix for
 |			DTS INDaa08508 and part of DTS INDaa11774
 ---------------------------------------------------------------------------*/
{
	register x;

	new_cons_close(tp);			/* close possible console? */
	x = splsx(tp->t_int_lvl);
	(tp->t_proc == NULL) ? 0 :(*tp->t_proc)(tp, T_RESUME);
	splsx(x);
	(void) ttywait(tp);
	ttyflush(tp, FREAD);
	tp->t_state &= ~(ISOPEN|WOPEN);
	
	if (CONTROL_PROC(tp)) { /* really get rid of this as ctty*/
		clear_ctty_from_session(CONTROL_PROC(tp));
	}

	tp->t_pgrp = 0; /* added to 4.2 */
	tp->t_cproc = NULL;	/* no controlling process */
	tp->t_dev = 0;
}

/*
 * common ioctl tty code
 */
ttiocom(tp, cmd, arg, mode)
register struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		called to process IOCTLs
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 |  1 May 89	garth	Added check to prevent returning EIO or sending
 |			SIGTTOU if a process is trying to set the foreground
 |			process group (via TIOCSPGRP) and the controlling
 |			process of the terminal is associated with a 
 |			different session.  Also, added additional
 |			checks when doing a TIOCSPGRP.
 |			
 | 11 Sep 89	garth	Fix for SR 4700-496984 (aka, DTS INDaa05951).
 |			Added code to move data from canq back to rawq
 |			when an FIONREAD is done.  This ensures that 
 |			accurate byte counts will be returned on successive
 |			calls to FIONREAD.
 |
 | 27 Feb 92	dmurray	Added TIOCGWINSZ and TIOCSWINSZ for SIGWINCH
 |			support.
 |
 | 26 May 92	dmurray	Added pcatch to sleep and check for vhangup condition.
 |
 | 08 Jul 92	marcel	now we check for error returns from ttywait.  Fix
 |			for DTS INDaa12309
 |
 --------------------------------------------------------------------------*/

	extern int nldisp;
	register flag;
	register x;
	struct termio cb;
	struct termios pb;
	struct sgttyb tb;
	int retval;
	extern nodev() ;
	
#ifdef	BSDJOBCTL
	struct ltchars lb;
#endif	BSDJOBCTL

#ifdef	BSDJOBCTL

	if (cmd == TIOCSPGRP && (u.u_procp->p_ttyp == NULL ||
				 tp != u.u_procp->p_ttyp || 
	    (tp->t_cproc == NULL) || (tp->t_cproc->p_sid != u.u_procp->p_sid)))
		return (ENOTTY);

        /*
         * If the ioctl involves modification,
         * insist on being able to write the device,
         * and hang if in the background.
         */

	switch(cmd) {
	case TIOCCONS:
	case TCSETAW:
	case TCSETAF:
	case TCSETA:
	case TCSBRK:
	case TCXONC:
	case TCFLSH:
	case TIOCSETP:
	case FIOSSAIOSTAT:
	case FIOSSAIOOWN:
	case FIOSNBIO:
	case TIOCSLTC:
	case TIOCLBIS:
	case TIOCLBIC:
	case TIOCLSET:
	case TIOCSPGRP:
	case TCSETATTRD:
	case TCSETATTRF:
	case TCSETATTR:
	case TIOCSWINSZ:

#define bit(a) (1<<(a-1))
		while (u.u_procp->p_pgrp != tp->t_pgrp && tp->t_pgrp &&
		   tp == u.u_procp->p_ttyp &&
                   (u.u_procp->p_flag&SVFORK) == 0 &&
                   !(u.u_procp->p_sigignore & bit(SIGTTOU)) &&
                   !(u.u_procp->p_sigmask & bit(SIGTTOU))) {
			/* Don't return EIO or send SIGTTOU if we are trying
			   to set the foreground process group and the
			   controlling process of the terminal is associated
			   with a different session. POSIX requires that
			   ENOTTY be returned in this case.
			*/
			if ((cmd == TIOCSPGRP) && ((tp->t_cproc == NULL) ||
			    	    (u.u_procp->p_sid != tp->t_cproc->p_sid)))
				return(ENOTTY);
			if (orphaned(u.u_procp->p_pgrp, u.u_procp->p_sid,NULL))
				return (EIO);
			gsignal(u.u_procp->p_pgrp, SIGTTOU);
			if (sleep( (caddr_t)&lbolt, (TTOPRI | PCATCH) ))
				return(EINTR);
			if (tp->t_hardware & VHANGUP)
				return(EBADF);
                }
                break;
#undef  bit
	}

#endif	BSDJOBCTL

	switch(cmd) {

	case TIOCCONS:			/* make me the console */
		if (!suser())		/* Must be root to steal console. */
			return(EPERM);	/* You aren't. */
		new_cons_open(tp);
		return(-1);		/* success? */

	case TCSETAW:
	case TCSETAF:
	case TCSETATTRD:
	case TCSETATTRF:
		if ( (retval=ttywait(tp)) != 0)
			return (retval);

		if ((cmd == TCSETAF) || (cmd == TCSETATTRF))
			ttyflush(tp, FREAD);
	case TCSETA:
	case TCSETATTR:
		if ((cmd == TCSETA) || (cmd == TCSETAW) || (cmd == TCSETAF)) {
			bcopy(arg, &cb, sizeof cb);
			pb.c_iflag = cb.c_iflag; /* copy relevant info   */
			pb.c_oflag = cb.c_oflag; /*   from struct termio */
			pb.c_cflag = cb.c_cflag; /*   to struct termios  */
			pb.c_lflag = cb.c_lflag;
			bcopy(ttcchar, pb.c_cc, NCCS);  /* initialize */
			bcopy(cb.c_cc, pb.c_cc, NCC);   /* copy in TERMIO cc */
			pb.c_cc[VSUSP] = tp->t_cc[VSUSP];  /* maintain VSUSP */
			pb.c_cc[VMIN ] = pb.c_cc[_V2_VMIN];
			pb.c_cc[VTIME] = pb.c_cc[_V2_VTIME];
			pb.c_lflag |= IEXTEN;
			if (tp->t_lmode & BSDTOSTOP)  /* set POSIX if BSD set */
				pb.c_lflag |= TOSTOP;
		} else {	/* TCSETATTR || TCSETATTRD || TCSETATTRF */
			bcopy(arg, &pb, sizeof pb);
			pb.c_cc[VSTART ] = CSTART;  /* can't be changed */
			pb.c_cc[VSTOP ] = CSTOP;    /* can't be changed */
			cb.c_line = tp->t_line;		/* no LD change */
			if (pb.c_lflag & TOSTOP) /*set BSD TOSTOP given POSIX*/
				tp->t_lmode |= BSDTOSTOP;
			else
				tp->t_lmode &= ~BSDTOSTOP;
		}

		if (tp->t_line != cb.c_line) {
			if (cb.c_line < 0 || cb.c_line >= nldisp) {
				return(EINVAL);
				break;
			}
			/*
			 *	Fix for japaneese customer who could not
			 *	live with the workaround of not setting
			 *	invalid line disciline. This change prevents
			 *	user from shooting themseves in the foot.
			 *	rpc: 9/19/94
			 */
			else if (linesw[cb.c_line].l_open == nodev)
				cb.c_line = tp->t_line;		/* bad LD */
			else
				(*linesw[tp->t_line].l_ioctl)
					(tp, LDCLOSE, 0, mode);
		}
		flag = tp->t_lflag;
		tp->t_iflag = pb.c_iflag;
		tp->t_oflag = pb.c_oflag;

		/* we must find out when CLOCAL DROPS to we can check lines */
		if ((tp->t_cflag & CLOCAL) && ( ! (pb.c_cflag & CLOCAL)))
			tp->t_state |= DELTA_CLOCAL;

		tp->t_cflag = pb.c_cflag;
		tp->t_lflag = pb.c_lflag;
		bcopy(pb.c_cc, tp->t_cc, NCCS);
		if (tp->t_line != cb.c_line) {
			tp->t_line = cb.c_line;
			(*linesw[tp->t_line].l_ioctl)(tp, LDOPEN, 0, mode);
		} else if (tp->t_lflag != flag) {
			(*linesw[tp->t_line].l_ioctl)(tp, LDCHG, flag, mode);
		}
		if (tp->t_cflag & CLOCAL) {
			/* WARNING: DO NOT REMOVE THIS TEST FOR PTYS!!!!
			 * This test exists because of problems in 
			 * the pty driver.  These problems will be
			 * resolved in 8.0.
			 */
			if (major(tp->t_dev) != 17) /* not for ptys */
				tp->t_state |= CARR_ON;	/* for ttwrite() */
		}
		return(-1);



	case TCGETATTR:				/* tcgetattr */
		pb.c_iflag = tp->t_iflag;
		pb.c_oflag = tp->t_oflag;
		pb.c_cflag = tp->t_cflag;
		pb.c_lflag = tp->t_lflag;
		bcopy(tp->t_cc, pb.c_cc, NCCS);
		bcopy(&pb, arg, sizeof pb);
		break;

	case TCGETA:
		cb.c_iflag = tp->t_iflag;
		cb.c_oflag = tp->t_oflag;
		cb.c_cflag = tp->t_cflag;
		cb.c_lflag = tp->t_lflag;
		cb.c_line = tp->t_line;
		bcopy(tp->t_cc, cb.c_cc, NCC);
		if ((tp->t_lflag & ICANON) == 0) {
			cb.c_cc[_V2_VMIN ] = tp->t_cc[VMIN ];
			cb.c_cc[_V2_VTIME] = tp->t_cc[VTIME];
		}
		bcopy(&cb, arg, sizeof cb);
		break;

	case TCSBRK:
		if ( (retval=ttywait(tp)) != 0)
			return (retval);

		if (*(int *)arg == 0)
			(*tp->t_proc)(tp, T_BREAK);
		break;

	case TCXONC:
		switch (*(int *)arg) {
		case 0:
			(*tp->t_proc)(tp, T_SUSPEND);
			break;
		case 1:
			(*tp->t_proc)(tp, T_RESUME);
			break;
		case 2:
			(*tp->t_proc)(tp, T_BLOCK, 1);
			break;
		case 3:
			(*tp->t_proc)(tp, T_UNBLOCK, 1);
			break;
		default:
			return(EINVAL);
		}
		break;

	case TCFLSH:
		switch (*(int *)arg) {
		case 0:
		case 1:
		case 2:
			ttyflush(tp, (*(int*)arg - FOPEN)&(FREAD|FWRITE));
			break;

		default:
			return(EINVAL);
		}
		break;

/* conversion aide only */
	case TIOCSETP:
		if ( (retval = ttywait(tp)) != 0)
			return (retval);
		ttyflush(tp, (FREAD|FWRITE));
		bcopy(arg, &tb, sizeof(tb));
		tp->t_iflag = 0;
		tp->t_oflag = 0;
		tp->t_lflag = 0;
		tp->t_cflag = (tb.sg_ispeed&CBAUD)|CREAD;
		if ((tb.sg_ispeed&CBAUD)==B110)
			tp->t_cflag |= CSTOPB;
		tp->t_cc[VERASE] = tb.sg_erase;
		tp->t_cc[VKILL] = tb.sg_kill;
		flag = tb.sg_flags;
		if (flag&O_HUPCL)
			tp->t_cflag |= HUPCL;
		if (flag&O_XTABS)
			tp->t_oflag |= TAB3;
		else if (flag&O_TBDELAY)
			tp->t_oflag |= TAB1;
		if (flag&O_LCASE) {
			tp->t_iflag |= IUCLC;
			tp->t_oflag |= OLCUC;
			tp->t_lflag |= XCASE|IEXTEN;
		}
		if (flag&O_ECHO)
			tp->t_lflag |= ECHO;
		if (!(flag&O_NOAL))
			tp->t_lflag |= ECHOK;
		if (flag&O_CRMOD) {
			tp->t_iflag |= ICRNL;
			tp->t_oflag |= ONLCR;
			if (flag&O_CR1)
				tp->t_oflag |= CR1;
			if (flag&O_CR2)
				tp->t_oflag |= ONOCR|CR2;
		} else {
			tp->t_oflag |= ONLRET;
			if (flag&O_NL1)
				tp->t_oflag |= CR1;
			if (flag&O_NL2)
				tp->t_oflag |= CR2;
		}
		if (flag&O_RAW) {
			tp->t_cc[VTIME] = 1;
			tp->t_cc[VMIN] = 6;
			tp->t_iflag &= ~(ICRNL|IUCLC);
			tp->t_cflag |= CS8;
		} else {
			tp->t_cc[VEOF] = CEOF;
			tp->t_cc[VEOL] = 0;
			tp->t_iflag |= BRKINT|IGNPAR|ISTRIP|IXON|IXANY;
			tp->t_oflag |= OPOST;
			tp->t_cflag |= CS7|PARENB;
			tp->t_lflag |= ICANON|ISIG|IEXTEN;
		}
		tp->t_iflag |= INPCK;
		if (flag&O_ODDP)
			if (flag&O_EVENP)
				tp->t_iflag &= ~INPCK;
			else
				tp->t_cflag |= PARODD;
		if (flag&O_VTDELAY)
			tp->t_oflag |= FFDLY;
		if (flag&O_BSDELAY)
			tp->t_oflag |= BSDLY;
		return(-1);

	case TIOCGETP:
		tb.sg_ispeed = tp->t_cflag&CBAUD;
		tb.sg_ospeed = tb.sg_ispeed;
		tb.sg_erase = tp->t_cc[VERASE];
		tb.sg_kill = tp->t_cc[VKILL];
		flag = 0;
		if (tp->t_cflag&HUPCL)
			flag |= O_HUPCL;
		if (!(tp->t_lflag&ICANON))
			flag |= O_RAW;
		if (tp->t_lflag&XCASE)
			flag |= O_LCASE;
		if (tp->t_lflag&ECHO)
			flag |= O_ECHO;
		if (!(tp->t_lflag&ECHOK))
			flag |= O_NOAL;
		if (tp->t_cflag&PARODD)
			flag |= O_ODDP;
		else if (tp->t_iflag&INPCK)
			flag |= O_EVENP;
		else
			flag |= O_ODDP|O_EVENP;
		if (tp->t_oflag&ONLCR) {
			flag |= O_CRMOD;
			if (tp->t_oflag&CR1)
				flag |= O_CR1;
			if (tp->t_oflag&CR2)
				flag |= O_CR2;
		} else {
			if (tp->t_oflag&CR1)
				flag |= O_NL1;
			if (tp->t_oflag&CR2)
				flag |= O_NL2;
		}
		if ((tp->t_oflag&TABDLY)==TAB3)
			flag |= O_XTABS;
		else if (tp->t_oflag&TAB1)
			flag |= O_TBDELAY;
		if (tp->t_oflag&FFDLY)
			flag |= O_VTDELAY;
		if (tp->t_oflag&BSDLY)
			flag |= O_BSDELAY;
		tb.sg_flags = flag;
		bcopy(&tb, arg, sizeof(tb));
		break;

	case FIONREAD:
		if (tp->t_canq.c_cc  == 0)
			(void) canon(tp, mode, 1);
		* (long *) arg = tp->t_canq.c_cc;
		/* If in raw mode, return data in canq back to rawq.
		 * This is done so that on successive calls to FIONREAD
		 * accurate byte counts are returned.
		 * NOTE: There is no need to do this in canonical
		 * mode. because only data up to a newline character
		 * can be read.  Thus, accurate byte counts should
		 * be returned on successive calls to FIONREAD.
		 */
		if (!(tp->t_lflag & ICANON) && (tp->t_canq.c_cc)) {
			x = splx(tp->t_int_lvl);
			/* 
			 * Move all canonical input to front of raw queue to
			 * be reprocessed.
			 */
			if (tp->t_rawq.c_cc) {
				tp->t_canq.c_cc += tp->t_rawq.c_cc;
				tp->t_canq.c_cl->c_next = tp->t_rawq.c_cf;
				tp->t_canq.c_cl = tp->t_rawq.c_cl;
			}
			tp->t_rawq = tp->t_canq;
			tp->t_canq = ttnulq;
			/* 
			 * Possibly add a check for TTYHOG to see if rawq
			 * should be flushed.
			 */
			if (tp->t_rawq.c_cc >= tp->t_cc[VMIN]) 
				tp->t_delct = 1;
			else if (tp->t_cc[VTIME]) 
				tttimeo(tp);
			splx(x);
		}
		break;

	case FIOGSAIOSTAT:
		if (tp->t_state & ASYNC)
			* (int *) arg = 1;
		else
			* (int *) arg = 0;
		break;

	case FIOSSAIOSTAT:
		if (* (int *) arg)
			tp->t_state |= ASYNC;
		else
			tp->t_state &= ~ASYNC;
		break;
	case FIOGSAIOOWN:
		* (int *) arg = (int) tp->t_pidprc->p_pid;
		break;
	case FIOSSAIOOWN:
		{
			struct pidprc tpidprc;
			register struct proc *tproc;
			int tpid;

			tpid = * (int *) arg;
			if ((tpid < 0) || (tpid >= MAXPID))
				return(ESRCH);
			tproc = pfind(tpid);
			if (tproc == (struct proc *) 0)
				return(ESRCH);
			if (tproc->p_uid != u.u_ruid
				&& tproc->p_uid != u.u_uid
				&& tproc->p_suid != u.u_ruid
				&& tproc->p_suid != u.u_uid
				&& u.u_uid != 0
				&& !inferior(tproc)) {
					return(EPERM);
			}
			tp->t_pidprc = tproc;
		}
		break;

	case FIOGNBIO:
		if (tp->t_state & NBIO)
			* (int *) arg = 1;
		else
			* (int *) arg = 0;
		break;

	case FIOSNBIO:
		if (* (int *) arg)
			tp->t_state |= NBIO;
		else
			tp->t_state &= ~NBIO;
		break;
#ifdef	BSDJOBCTL

	/*
	 * set/get local special characters
	 */

	case TIOCSLTC:
		bcopy(arg, &lb, sizeof(lb));
		if (lb.t_rprntc != 0377)
			return(EINVAL);
		if (lb.t_flushc != 0377)
			return(EINVAL);
		if (lb.t_werasc != 0377)
			return(EINVAL);
		if (lb.t_lnextc != 0377)
			return(EINVAL);
		tp->t_cc[VSUSP]  = lb.t_suspc;
		tp->t_cc[VDSUSP] = lb.t_dsuspc;
		break;

	case TIOCGLTC:
		lb.t_suspc  = tp->t_cc[VSUSP];
		lb.t_dsuspc = tp->t_cc[VDSUSP];
		lb.t_rprntc = 0377;
		lb.t_flushc = 0377;
		lb.t_werasc = 0377;
		lb.t_lnextc = 0377;

		bcopy((caddr_t)&lb, arg, sizeof (struct ltchars));
		break;

	/*
	 * Modify local mode word.
	 */

	case TIOCLBIS:
		tp->t_lmode |= *(int *)arg;
		SET_TOSTOP
		break;

	case TIOCLBIC:
		tp->t_lmode &= ~(*(int *)arg);
		SET_TOSTOP
		break;

	case TIOCLSET:
		tp->t_lmode = *(int *)arg;
		SET_TOSTOP
		break;

	case TIOCLGET:
		*(int *)arg = tp->t_lmode;
		break;

	/*
	 * should allow SPGRP and GPGRP only if tty open for reading
	 */

	case TIOCSPGRP:
	{
		int tmp1;
		
		/* Must have a controlling terminal and terminal referenced
		   must be a controlling terminal and the controlling
		   terminal must be assicated with the session of the
		   calling process */
		if (!(u.u_procp->p_ttyp) || (tp != u.u_procp->p_ttyp) ||
		    ((tp->t_cproc == NULL) || 
		     (u.u_procp->p_sid != tp->t_cproc->p_sid)))
			return(ENOTTY);
		/* check for permission to assume this pgrp */
		if (tmp1 = pgrpcheck(*(int *)arg))
			return(tmp1);

		tp->t_pgrp = *(int *)arg;

		break;
	}

	case TIOCGPGRP:
		if (u.u_procp->p_ttyp == NULL || tp != u.u_procp->p_ttyp)
		    return (ENOTTY);
		if (tp->t_pgrp == PGID_NOT_SET)
			return (EACCES);
		*(int *)arg = tp->t_pgrp;
		break;

#endif	BSDJOBCTL

	case TIOCGWINSZ:
		/*
		 * Return the current window size values.
		 */
		((struct winsize *)arg)->ws_row		= tp->t_ws_row;
		((struct winsize *)arg)->ws_col         = tp->t_ws_col;
		((struct winsize *)arg)->ws_xpixel      = tp->t_ws_xpixel;
		((struct winsize *)arg)->ws_ypixel      = tp->t_ws_ypixel;
		break;

	case TIOCSWINSZ:
		/*
		 * If any of the window size values has changed, save the
		 * new values and then send a SIGWINCH to the foreground
		 * process group.
		 */
		if ( tp->t_ws_row    != ((struct winsize *)arg)->ws_row    ||
		     tp->t_ws_col    != ((struct winsize *)arg)->ws_col    ||
		     tp->t_ws_xpixel != ((struct winsize *)arg)->ws_xpixel ||
		     tp->t_ws_ypixel != ((struct winsize *)arg)->ws_ypixel ) {
			tp->t_ws_row	= ((struct winsize *)arg)->ws_row;
			tp->t_ws_col	= ((struct winsize *)arg)->ws_col;
			tp->t_ws_xpixel	= ((struct winsize *)arg)->ws_xpixel;
			tp->t_ws_ypixel	= ((struct winsize *)arg)->ws_ypixel;
			gsignal(tp->t_pgrp, SIGWINCH);
		}
		break;

	default:
		if ((cmd&0xff) == LDIOC)
			(*linesw[tp->t_line].l_ioctl)(tp, cmd, *(int *)arg, mode);
		else
			return(EINVAL);
		break;
	}
	return(0);
}

struct mtimer tttimers = {25, 400, 0, 250, 0, 0};

ttinit(tp)
register struct tty *tp;
{
	tp->t_line	= 0;
	tp->t_iflag	= 0;
	tp->t_oflag	= 0;
	tp->t_cflag	= SSPEED|CS8|CREAD|HUPCL;
	tp->t_lflag	= 0;
	tp->t_ws_row	= 0;
	tp->t_ws_col	= 0;
	tp->t_ws_xpixel	= 0;
	tp->t_ws_ypixel	= 0;
	tp->t_dvr_fc	= 0;
#ifdef BSDJOBCTL
	bcopy(ttcchar, tp->t_cc, NLDCC);
#else  ! BSDJOBCTL
	bcopy(ttcchar, tp->t_cc, NCC);
#endif ! BSDJOBCTL
	bcopy(&tttimers, tp->timers, sizeof(struct mtimer));
}

ttywait(tp)

register struct tty	*	tp;

{
/*---------------------------------------------------------------------------
 | FUNCTION -		Wait for output to be drained.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 26 May 92	dmurray	Added pcatch to sleep and check for vhangup condition.
 |
 ---------------------------------------------------------------------------*/

	register x;
	int t;
	int rval  ;
/*
 * WARNING !!!  This routine works only as long as the processor interrupt
 * level is at least tp->t_int_lvl.  Otherwise, a receive interrupt which
 * echoes to t_outq nullifies this routine.  Set the interrupt level back
 * to zero only when transmission can be resumed.
 *
 * Secondly, there is a critical section between the outq.cc test and the
 * subsequent sleep.  There is also a critical section at the RS-232 driver
 * wait routine (again, just before the sleep).
 *
 * Since this routine could have been called at level (0,0), these two
 * critical sections are forced to tp->t_int_lvl as a quick fix.  However,
 * since ttywait() doesn't work below tp->t_int_lvl, all callers of this
 * routine need to be re-evaluated.
 */
	do {
		delay(HZ/15);		/* let data sleaze out of UART */
		t = lbolt;		/* get current time */
		x = splx(tp->t_int_lvl);
		if (tp->t_outq.c_cc || (tp->t_state&(BUSY|TIMEOUT))) {
			tp->t_state |= TTIOW;
			if (sleep( (caddr_t)&tp->t_oflag, (TTOPRI | PCATCH) )) {
				tp->t_state &= ~TTIOW;
				splx(x);
				return(EINTR);
			}
			if (tp->t_hardware & VHANGUP) {
				tp->t_state &= ~TTIOW;
				splx(x);
				return(EBADF);
			}
			t--;		/* make us check again */
		}
		rval = (*tp->t_drvtype->wait)(tp);   /* buffered card empty */
		if (rval != 0) {
			splx(x);
			return (rval) ;
		}
		splx(x);
	} while (t != lbolt);	/* if we slept, re-check all queues */
	return (0);

}
/*
 * wakeup any sleepers when driver ready to output more chars
 */
ttoutwakeup(tp)
register struct tty *tp;
{
	if ((tp->t_state&TTIOW) && tp->t_outq.c_cc==0) {
		tp->t_state &= ~TTIOW;
		wakeup((caddr_t)&tp->t_oflag);
	}
	if (tp->t_outq.c_cc<=ttlowat[tp->t_cflag&CBAUD]) {
		if (tp->t_state&OASLP) {
			tp->t_state &= ~OASLP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~WCOLL;
		}
	}
}
/*
 * flush TTY queues
 */
ttyflush(tp, cmd)
register struct tty *tp;
{
	register struct cblock *cp;
	register s;

	if (cmd&FWRITE) {
		while ((cp = getcb(&tp->t_outq)) != NULL)
			putcf(cp);
		(*tp->t_proc)(tp, T_WFLUSH);
		ttoutwakeup(tp);
	}
	if (cmd&FREAD) {
		while ((cp = getcb(&tp->t_canq)) != NULL)
			putcf(cp);
		s = splsx(tp->t_int_lvl);
		while ((cp = getcb(&tp->t_rawq)) != NULL)
			putcf(cp);
		tp->t_delct = 0;
		splsx(s);
		(*tp->t_proc)(tp, T_RFLUSH);
		if (tp->t_state&IASLP) {
			tp->t_state &= ~IASLP;
			wakeup((caddr_t)&tp->t_rawq);
		}
	}
}

/*
 * Transfer raw input list to canonical list,
 * doing erase-kill processing and handling escapes.
 */
canon(tp, flag, ncode)
register struct tty *tp;
int flag;
int ncode;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Transfer data from raw to canonical input list.
 |			Erase, Kill and Escape character processing is
 |			performed during transfer.
 |
 | PARAMETERS PASSED -
 |	tp		pointer to tty structure.
 |
 |      flag            file flag.
 |
 |      ncode           == 0: called from read().
 |                      == 1: called from ioctl(). (FIONREAD)
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 12 Apr 89	garth	Fix for SR 4700-XXXXXX (aka, DTS INDaa0xxxx).
 |			Changed splx to splsx so that the software
 |			sub-level is returned to its proper setting.
 |			Before fix, doing an FIONREAD would cause 
 |			software triggers not to be serviced.
 |
 | 07 Sep 89	murgia    Fix for SR 4700-664300 (aka, DTS INDaa02923).
 |                      (Above SR's are for the 800 series machines)
 |                      Removed conditional call to tttimeo.  Before fix,
 |                      if there was a timer currently active it would
 |                      complete before a new timer value could be set.
 |                      This caused read timeouts, in noncanonical mode,
 |                      to behave incorrectly.
 |
 | 06 Dec 91	dmurray	Fix for INDaa10655.  Protect clist manipulation
 |			with spl.
 |
 | 15 Apr 92	marcel	Fix for INDaa11758.
 |			Moved non-blocking checks after the VMIN checks, and
 |			return the appropriate error code for the non-block
 |			cases.  ttread will pass along any error returned.
 |
 | 26 May 92	dmurray	Added pcatch to sleep and check for vhangp condition.
 |
 ---------------------------------------------------------------------------*/
	register char *bp;
	register c, esc, x, y;

	x = splsx(tp->t_int_lvl);
	if (tp->t_rawq.c_cc == 0)
		tp->t_delct = 0;
	while (tp->t_delct == 0) {
		if (!(tp->t_lflag&ICANON) && tp->t_cc[VMIN]==0) {
			if (tp->t_cc[VTIME]==0)
				break;
			if (ncode != 0) {
				splsx(x);
				return(0);
			}
			tp->t_state &= ~RTO;
			tttimeo(tp);
		}
		if (ncode != 0) {
			splsx(x);
			return(0);
		}

		if (flag & FNBLOCK) {
			splsx(x);
			return(EAGAIN);
		}

		if (!(flag & FNDELAY) && (tp->t_state & NBIO)) {
			splsx(x);
			return(EWOULDBLOCK);
		}

		if (!(tp->t_state & CARR_ON) || (flag & FNDELAY)) {
			splsx(x);
			return(0);
		}
		
		tp->t_state |= IASLP;
		y = splx(tp->t_int_lvl);
		splsx(x);
		if (sleep( (caddr_t)&tp->t_rawq, (TTIPRI | PCATCH) )) {
			tp->t_state &= ~IASLP;
			splx(y);
			return(EINTR);
		}
		if (tp->t_hardware & VHANGUP) {
			tp->t_state &= ~IASLP;
			splx(y);
			return(EBADF);
		}
		x = splsx(tp->t_int_lvl);
		splx(y);
#ifdef	BSDJOBCTL
		/* If Job Control is enabled, then it is possible that
		 * we have been moved from foreground to background, so
		 * we must return back to ttread to re-check all the
		 * appropriate things, mostly the fg/bg check.
		 */
		if (ncode == 0 && tp == u.u_procp->p_ttyp &&
		    u.u_procp->p_pgrp != tp->t_pgrp && tp->t_pgrp) {
			splsx(x);
			return(-1);
		}
#endif
	}
	if (!(tp->t_lflag&ICANON)) {
		y = splx(tp->t_int_lvl);
		tp->t_canq = tp->t_rawq;
		tp->t_rawq = ttnulq;
		tp->t_delct = 0;
		splsx(x);
		splx(y);
		return(0);
	}
	splsx(x);
	bp = canonb;
	esc = 0;
	while ((c=getc(&tp->t_rawq)) >= 0) {
		if (!esc) {
			if (c == '\\') {
				esc++;
			} else if (_T_cc_enabled(c, VERASE)) {
				if (bp > canonb)
					bp--;
				continue;
			} else if (_T_cc_enabled(c, VKILL)) {
				bp = canonb;
				continue;
			} else if (_T_cc_enabled(c, VEOF)) {
				break;
			}
		} else {
			esc = 0;
			if (_T_cc_enabled(c, VERASE) ||
			    _T_cc_enabled(c, VKILL)  ||
			    _T_cc_enabled(c, VEOF))
				bp--;
			else if (tp->t_lflag&XCASE && tp->t_lflag&IEXTEN) {
				if ((c < 0200) && maptab[c]) {
					bp--;
					c = maptab[c];
				} else if (c == '\\')
					continue;
			} else if (c == '\\')
				esc++;
		}
		*bp++ = c;
		if (c == '\n' || _T_cc_enabled(c, VEOL))
			break;
		if (bp >= &canonb[CANBSIZ])
			bp--;
	}
	tp->t_delct--;
	c = bp - canonb;
	bp = canonb;
/* faster copy ? */
	while (c--)
		putc(*bp++, &tp->t_canq);
	return(0);
}

/*
 * Restart typewriter output following a delay timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
ttrstrt_sw(tp)
register struct tty *tp;
{
	(*tp->t_proc)(tp, T_TIME);
}

ttrstrt(tp)
register struct tty *tp;
{
	if (tp->ttrstrt_intloc.proc == 0)
		sw_trigger(&tp->ttrstrt_intloc, ttrstrt_sw, tp, 0, tp->t_int_lvl);
}

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 */
ttread(tp, uio)
register struct tty *tp;
register struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Move inbound data from the raw queue to the user's
 |			buffer.  Canon() is called to move the data from raw
 |			queue to canonical buffer.  This routine then moves
 |			the data to the user's buffer.
 |
 |			NOTE:  The canonical buffer (and count) are currently
 |			globals, shared by all ports!  This has to be changed
 |			in order to support MP!
 |
 |			Call from device driver's read routine.
 |
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 15 Apr 92	marcel	Fix for INDaa11758.
 |			Removed non-block checks at end of procedure.  Now,
 |			canon will return error if non-blocking and ttread
 |			will return that error.  A read that returns from
 |			canon successfully with no bytes will return no bytes.
 |
 | 26 May 92	dmurray	Added pcatch to sleep and check for vhangup condition.
 |
 | 17 Jul 92	dmurray	Fix for INDaa12371.
 |			Added splsx call so that we are protected from
 |			sw_triggers until we are finished.  See defect for
 |			detailed failure scenario.
 |
 ---------------------------------------------------------------------------*/
	register struct clist *tq;
	register error = 0;
	register s;
	int cnt = uio->uio_resid;

	tq = &tp->t_canq;
#ifdef	BSDJOBCTL

loop:

	/*
	 * Hang process if it's in the background.
	 */

#define bit(a) (1<<(a-1))
	while (tp == u.u_procp->p_ttyp && u.u_procp->p_pgrp != tp->t_pgrp && tp->t_pgrp)
	{
		if ((u.u_procp->p_sigignore & bit(SIGTTIN)) ||
		   (u.u_procp->p_sigmask & bit(SIGTTIN)) ||
		    u.u_procp->p_flag&SVFORK ||
		    orphaned(u.u_procp->p_pgrp, u.u_procp->p_sid,NULL))
			return (EIO);
		gsignal(u.u_procp->p_pgrp, SIGTTIN);
		if (sleep( (caddr_t)&lbolt, (TTIPRI | PCATCH) ))
			return(EINTR);
		if (tp->t_hardware & VHANGUP)
			return(EBADF);
	}
#undef	bit

#endif	BSDJOBCTL

	if (tq->c_cc == 0) {
		error = canon(tp, uio->uio_fpflags, 0);
#ifdef	BSDJOBCTL
		/* For job control, if canon returns -1, then it has been
		 * awoken because of a possible bg/fg change.  We need to
		 * go back to see if we are now in the background.
		 */
		if (error == -1)
			goto loop;
#endif
	}

	while (uio->uio_resid!=0 && error==0) {
#ifdef	BSDJOBCTL
		if ((uio->uio_resid >= CBSIZE) &&
		   ((tp->t_cc[VDSUSP] == 0377) ||
		   !(tp->t_lflag & (IEXTEN|ISIG)))) {
#else	not BSDJOBCTL
		if (uio->uio_resid >= CBSIZE) {
#endif
			register n;
			register struct cblock *cp;
			register char *bufp;

			cp = getcb(tq);
			if (cp == NULL)
				break;
			n = min(uio->uio_resid, cp->c_last - cp->c_first);
			error = uiomove(&cp->c_data[cp->c_first], n, UIO_READ, uio);
			putcf(cp);
		} else {
			register c;

			if ((c = getc(tq)) < 0)
				break;
#ifdef  BSDJOBCTL
			if (_T_cc_enabled(c, VDSUSP) &&
			    ((tp->t_lflag & (IEXTEN|ISIG)) == (IEXTEN|ISIG)) ) {
				gsignal(tp->t_pgrp, SIGTSTP);
				if (sleep( (caddr_t)&lbolt, (TTIPRI | PCATCH) ))
					return(EINTR);
				if (tp->t_hardware & VHANGUP)
					return(EBADF);
				goto loop;
				}
#endif  BSDJOBCTL
			error = ureadc(c, uio); 
		}
	}
	if (tp->t_state&TBLOCK) {
/*
 * UNBLOCK input (i.e. send X-ON) if raw queue is below the threshold
 * OR IF THERE ARE NO COMPLETE RECORDS IN THE RAW QUEUE (Since the next
 * read won't complete, you'll never decrease the size of the raw queue,
 * and therefore will NEVER send XON).
 *
 * This bug inherited from Bell 5.2
 */
		if ((tp->t_rawq.c_cc<TTXOLO) || (tp->t_delct == 0)) {
			s = splsx(tp->t_int_lvl);
			(*tp->t_proc)(tp, T_UNBLOCK);
			splsx(s);
		}
	}
	return(error);
}

/*
 * Called from device's write routine after it has
 * calculated the tty-structure given as argument.
 */
ttwrite(tp, uio)
register struct tty *tp;
register struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Called from devices write routine.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 31 Aug 89	garth	Fix for SR 4700-819771 (aka, DTS INDaa05923).
 |			Changed return value to EIO instead of zero
 |			when carrier is lost.  This is required by 
 |			POSIX.
 | 16 Jan 90	garth	Added support for the iostat command.  Update
 |			tk_nout with the number of bytes being written
 |			out.
 |			
 | 26 May 92	dmurray	Added pcatch to sleeps and check for vhangup condition.
 |
 --------------------------------------------------------------------------*/
	unsigned char obuf[CBSIZE/2];
	int x, error = 0;
	register cc;
	register unsigned char *cbuf;
	int initial_resid = uio->uio_resid;

	if ((tp->t_state&CARR_ON)==0)
		return(EIO);

#ifdef	BSDJOBCTL

	/*
	 * Hang the process if it's in the background.
	 */

#define bit(a) (1<<(a-1))
	while (u.u_procp->p_pgrp != tp->t_pgrp && 
	    tp->t_pgrp && tp == u.u_procp->p_ttyp && 
	    (tp->t_lflag&TOSTOP) && (u.u_procp->p_flag&SVFORK)==0 &&
	    !(u.u_procp->p_sigignore & bit(SIGTTOU)) &&
	    !(u.u_procp->p_sigmask & bit(SIGTTOU))
	    ) {
		if (orphaned(u.u_procp->p_pgrp, u.u_procp->p_sid,NULL))
			return (EIO);
		gsignal(u.u_procp->p_pgrp, SIGTTOU);
		if (sleep( (caddr_t)&lbolt, (TTIPRI | PCATCH) ))
			return(EINTR);
		if (tp->t_hardware & VHANGUP)
			return(EBADF);
	}
#undef	bit

#endif	BSDJOBCTL
	x = splx(tp->t_int_lvl);
	while (uio->uio_resid) {
		while (tp->t_outq.c_cc > tthiwat[tp->t_cflag&CBAUD]) {
			(*tp->t_proc)(tp, T_OUTPUT);
			/* If the device is going to use an interrupt
			** to say it is ready to output more characters
			** then go to sleep.  Or if state is TTSTOP
			** then go to sleep.
			*/
			if ((tp->t_int_flag&PEND_TXINT) ||
				(tp->t_state&(TTSTOP|TIMEOUT))) {

				if (uio->uio_fpflags&FNBLOCK) {
					splx(x);
					if (uio->uio_resid == initial_resid)
						return(EAGAIN);
					return(0);
				}
				if (tp->t_state & NBIO) {
					splx(x);
					if (uio->uio_resid == initial_resid)
						return(EWOULDBLOCK);
					return(0);
				}
				tp->t_state |= OASLP;
				if (sleep( (caddr_t)&tp->t_outq,
							 (TTOPRI | PCATCH) )) {
					tp->t_state &= ~OASLP;
					wakeup((caddr_t)&tp->t_outq);
					splx(x);
					return(EINTR);
				}
				if (tp->t_hardware & VHANGUP) {
					tp->t_state &= ~OASLP;
					splx(x);
					return(EBADF);
				}
			}
		}
		if ((tp->t_state&CARR_ON)==0) {
			splx(x);
			return(0);
		}
		/*
		 * Grab a hunk of data from the user.  This little piece of
		 * code is necessary to insure that the buffer is not zero
		 * length and is contigious.
		 */
		cc = uio->uio_iov->iov_len;
		/* This assignment is necessary for the iostat command.
	 	 * Do not remove the following line!
		 */
		tk_nout += cc;
		if (cc == 0) { 
			uio->uio_iovcnt--;
			uio->uio_iov++;
			if (uio->uio_iovcnt < 0)
				panic("ttwrite");
			continue;
		}
		if (cc >= (CBSIZE/2)) {
			register n;
			register struct cblock *cp;

			if ((cp = getcf()) == NULL)
				break;
			n = min(cc, CBSIZE);
			cbuf = (unsigned char *)cp->c_data;
			if ((error = uiomove(cbuf, n, UIO_WRITE, uio))) {
				putcf(cp);
				break;
			}
			cp->c_last = n;
			if (!((tp->t_lflag&XCASE && tp->t_lflag&IEXTEN) || tp->t_oflag&OLCUC))
				do 
					if ((unsigned char)partab[*cbuf++]&077)
						break;
				while (--n);
			if (n)
				ttxput(tp, cp, cp->c_last); /* some need line displn */
			else {
				tp->t_col += cp->c_last;
				putcb(cp, &tp->t_outq);/*none need line displn*/
			}
		} else {
			cbuf = obuf;
			if ((error = uiomove(obuf, cc, UIO_WRITE, uio))) 
				break;
			while (cc--)
				ttxput(tp, *cbuf++, 0);
		}
	}
	(*tp->t_proc)(tp, T_OUTPUT);
	splx(x);

	if (uio->uio_fpflags&FNBLOCK)
		if (uio->uio_resid == initial_resid)
			if (error == 0)
				return(EAGAIN);
	if ((tp->t_state & NBIO) && (uio->uio_resid == initial_resid) &&
	    (error == 0))
		return(EWOULDBLOCK);
	return(error);
}

ttselect(tp, rw)
register struct tty *tp;
int rw;
{
	int s = spl5();

	switch (rw) {

	case FREAD:
		if (tp->t_delct > 0 || tp->t_canq.c_cc > 0)
			goto win;
		if (tp->t_rsel && tp->t_rsel->p_wchan == (caddr_t)&selwait)
			tp->t_state |= RCOLL;
		else
			tp->t_rsel = u.u_procp;
		break;

	case FWRITE:
		if (tp->t_outq.c_cc <= tthiwat[tp->t_cflag&CBAUD])
			goto win;
		if (tp->t_wsel && tp->t_wsel->p_wchan == (caddr_t)&selwait)
			tp->t_state |= WCOLL;
		else
			tp->t_wsel = u.u_procp;
		break;
	}
	splx(s);
	return (0);
win:
	splx(s);
	return (1);
}

ttcontrol(tp, com, data)
struct tty	*	tp;
caddr_t			data;
enum ldc_func		com;
{
/*---------------------------------------------------------------------------
 | FUNCTION -           Perform miscellaneous line discipline functionality
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 26 May 92	dmurray	Added LDC_VHANGUP.
 |
 ---------------------------------------------------------------------------*/

	switch(com) {
	case LDC_DFLOW:
		if ((int) data == 0) {
			tp->t_dvr_fc &= ~DFLOW;
			break;
		}
		tp->t_dvr_fc |= DFLOW;
		if (tp->t_rawq.c_cc > TTXOHI) {
			(*tp->t_proc)(tp, T_BLOCK, 0);
		}
		break;

	case LDC_VHANGUP:
		if ((int)data == 0) {
			tp->t_hardware	&= ~VHANGUP;
			break;
		}
		tp->t_hardware	|= VHANGUP;
		wakeup( (caddr_t)&lbolt );
		wakeup( (caddr_t)&tp->t_oflag );
		wakeup( (caddr_t)&tp->t_rawq );
		wakeup( (caddr_t)&tp->t_outq );
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~WCOLL;
		}
		if (tp->t_rsel) {
			selwakeup(tp->t_rsel, tp->t_state & RCOLL);
			tp->t_state &= ~RCOLL;
			tp->t_rsel = 0;
		}
		break;
	}
	return(0);
}
