/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)sys_bsd.c	1.16 (Berkeley) 11/29/88";
static char rcsid[] = "$Header: sys_bsd.c,v 1.11.109.2 92/03/19 17:17:50 seshadri Exp $";
#endif /* not lint */

/*
 * The following routines try to encapsulate what is system dependent
 * (at least between 4.x and dos) which is used in telnet.c.
 */

#if	defined(unix)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#ifdef hpux
#include <sys/termio.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/bsdtty.h>
#endif
#include <sys/time.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>

#include "ring.h"

#include "fdset.h"

#include "defines.h"
#include "externs.h"
#include "types.h"

int
	tout,			/* Output file descriptor */
	tin,			/* Input file descriptor */
	net,
	xon = 0;

#ifdef hpux
static struct termio oterm, nterm;
static struct	ltchars oltc = { 0 }, nltc = { 0 };
#else
static struct	tchars otc = { 0 }, ntc = { 0 };
static struct	ltchars oltc = { 0 }, nltc = { 0 };
static struct	sgttyb ottyb = { 0 }, nttyb = { 0 };
#endif

static fd_set ibits, obits, xbits;


init_sys()
{
    tout = fileno(stdout);
    tin = fileno(stdin);
    FD_ZERO(&ibits);
    FD_ZERO(&obits);
    FD_ZERO(&xbits);

    errno = 0;
}


TerminalWrite(buf, n)
char	*buf;
int	n;
{
	int i;

    i = write(tout, buf, n);
	if (i < 0)
	    return 0;
	else
	  return i;
}

TerminalRead(buf, n)
char	*buf;
int	n;
{
    return read(tin, buf, n);
}

/*
 *
 */

int
TerminalAutoFlush()
{
#if	defined(LNOFLSH)
    int flush;

    ioctl(0, TIOCLGET, (char *)&flush);
    return !(flush&LNOFLSH);	/* if LNOFLSH, no autoflush */
#else	/* LNOFLSH */
    return 1;
#endif	/* LNOFLSH */
}

/*
 * TerminalSpecialChars()
 *
 * Look at an input character to see if it is a special character
 * and decide what to do.
 *
 * Output:
 *
 *	0	Don't add this character.
 *	1	Do add this character
 */

int
TerminalSpecialChars(c)
int	c;
{
    void xmitAO(), xmitEL(), xmitEC(), intp(), sendbrk();

#ifdef hpux
    if (c == termIntChar) {
	    intp();
	    return 0;
    } else if (c == termQuitChar) {
	sendbrk();
	return 0;
    } else if (c == termFlushChar) {
	xmitAO();		/* Transmit Abort Output */
	return 0;
    } else if (!MODE_LOCAL_CHARS(globalmode)) {
	if (c == termKillChar) {
	    xmitEL();
	    return 0;
	} else if (c == termEraseChar) {
	    xmitEC();		/* Transmit Erase Character */
	    return 0;
	}
    }
    return 1;
#else
    if (c == ntc.t_intrc) {
	intp();
	return 0;
    } else if (c == ntc.t_quitc) {
	sendbrk();
	return 0;
    } else if (c == nltc.t_flushc) {
	xmitAO();		/* Transmit Abort Output */
	return 0;
    } else if (!MODE_LOCAL_CHARS(globalmode)) {
	if (c == nttyb.sg_kill) {
	    xmitEL();
	    return 0;
	} else if (c == nttyb.sg_erase) {
	    xmitEC();		/* Transmit Erase Character */
	    return 0;
	}
    }
    return 1;
#endif
}


/*
 * Flush output to the terminal
 */
 
void
TerminalFlushOutput()
{
#ifdef hpux
    (void) ioctl(fileno(stdout), TCFLSH, 1);
#else
    (void) ioctl(fileno(stdout), TIOCFLUSH, (char *) 0);
#endif
}

void
TerminalSaveState()
{
#ifdef hpux
    ioctl(0, TCGETA, &oterm);
    ioctl(0, TIOCGLTC, (char *)&oltc);

    nterm = oterm;
    nltc = oltc;

    termEofChar = nterm.c_cc[VEOF];
    termEraseChar = nterm.c_cc[VERASE];
    termFlushChar = (char)017;
    termIntChar = nterm.c_cc[VINTR];
    termKillChar = nterm.c_cc[VKILL];
    termQuitChar = nterm.c_cc[VQUIT];
#else
    ioctl(0, TIOCGETP, (char *)&ottyb);
    ioctl(0, TIOCGETC, (char *)&otc);
    ioctl(0, TIOCGLTC, (char *)&oltc);

    ntc = otc;
    nltc = oltc;
    nttyb = ottyb;

    termEofChar = ntc.t_eofc;
    termEraseChar = nttyb.sg_erase;
    termFlushChar = nltc.t_flushc;
    termIntChar = ntc.t_intrc;
    termKillChar = nttyb.sg_kill;
    termQuitChar = ntc.t_quitc;
#endif
}

void
TerminalRestoreState()
{
}

/*
 * TerminalNewMode - set up terminal to a specific mode.
 */


#ifndef hpux
void
TerminalNewMode(f)
register int f;
{
    static int prevmode = 0;
    struct tchars *tc;
    struct tchars tc3;
    struct ltchars *ltc;
    struct sgttyb sb;
    int onoff;
    int old;
    struct	tchars notc2;
    struct	ltchars noltc2;
    static struct	tchars notc =	{ -1, -1, -1, -1, -1, -1 };
    static struct	ltchars noltc =	{ -1, -1, -1, -1, -1, -1 };

    globalmode = f;
    if (prevmode == f)
	return;
    old = prevmode;
    prevmode = f;
    sb = nttyb;

    switch (f) {

    case 0:
	onoff = 0;
	tc = &otc;
	ltc = &oltc;
	break;

    case 1:		/* remote character processing, remote echo */
    case 2:		/* remote character processing, local echo */
    case 6:		/* 3270 mode - like 1, but with xon/xoff local */
		    /* (might be nice to have "6" in telnet also...) */
	    sb.sg_flags |= CBREAK;
	    if ((f == 1) || (f == 6)) {
		sb.sg_flags &= ~(ECHO|CRMOD);
	    } else {
		sb.sg_flags |= ECHO|CRMOD;
	    }
	    sb.sg_erase = sb.sg_kill = -1;
	    if (f == 6) {
		tc = &tc3;
		tc3 = notc;
		    /* get XON, XOFF characters */
		tc3.t_startc = otc.t_startc;
		tc3.t_stopc = otc.t_stopc;
	    } else {
		/*
		 * If user hasn't specified one way or the other,
		 * then default to not trapping signals.
		 */
		if (!donelclchars) {
		    localchars = 0;
		}
		if (localchars) {
		    notc2 = notc;
		    notc2.t_intrc = ntc.t_intrc;
		    notc2.t_quitc = ntc.t_quitc;
		    tc = &notc2;
		} else {
		    tc = &notc;
		}
	    }
	    ltc = &noltc;
	    onoff = 1;
	    break;
    case 3:		/* local character processing, remote echo */
    case 4:		/* local character processing, local echo */
    case 5:		/* local character processing, no echo */
	    sb.sg_flags &= ~CBREAK;
	    sb.sg_flags |= CRMOD;
	    if (f == 4)
		sb.sg_flags |= ECHO;
	    else
		sb.sg_flags &= ~ECHO;
	    notc2 = ntc;
	    tc = &notc2;
	    noltc2 = oltc;
	    ltc = &noltc2;
	    /*
	     * If user hasn't specified one way or the other,
	     * then default to trapping signals.
	     */
	    if (!donelclchars) {
		localchars = 1;
	    }
	    if (localchars) {
		notc2.t_brkc = nltc.t_flushc;
		noltc2.t_flushc = -1;
	    } else {
		notc2.t_intrc = notc2.t_quitc = -1;
	    }
	    noltc2.t_suspc = escape;
	    noltc2.t_dsuspc = -1;
	    onoff = 1;
	    break;

    default:
	    return;
    }
    ioctl(tin, TIOCSLTC, (char *)ltc);
    ioctl(tin, TIOCSETC, (char *)tc);
    ioctl(tin, TIOCSETP, (char *)&sb);
#if	(!defined(TN3270)) || ((!defined(NOT43)) || defined(PUTCHAR))
    ioctl(tin, FIONBIO, (char *)&onoff);
    ioctl(tout, FIONBIO, (char *)&onoff);
#endif	/* (!defined(TN3270)) || ((!defined(NOT43)) || defined(PUTCHAR)) */
#if	defined(TN3270)
    if (noasynchtty == 0) {
	ioctl(tin, FIOASYNC, (char *)&onoff);
    }
#endif	/* defined(TN3270) */

    if (MODE_LINE(f)) {
	void doescape();

	(void) signal(SIGTSTP, (int (*)())doescape);
    } else if (MODE_LINE(old)) {
	(void) signal(SIGTSTP, SIG_DFL);
	sigsetmask(sigblock(0) & ~(1<<(SIGTSTP-1)));
    }
}

#else

void
TerminalNewMode(f)
register int f;
{
    static int prevmode = 0;
    struct termio *tc;
    struct ltchars *ltc;
    struct termio tc3;
    struct termio sb;
    int onoff;
    int old;
    struct termio notc2;
    struct	ltchars noltc2;
    static struct termio notc = { 0,  0,  0,  0,  0, 
				 -1, -1, -1, -1, -1, -1, -1, -1};
    static struct	ltchars noltc =	{ -1, -1, -1, -1, -1, -1 };
    register int i;

    globalmode = f;
    if (prevmode == f)
	return;
    old = prevmode;
    prevmode = f;
    sb = nterm;

    switch (f) {

    case 0:
	onoff = 0;
	tc = &oterm;
	ltc = &oltc;
	break;

    case 1:		/* remote character processing, remote echo */
    case 2:		/* remote character processing, local echo */
    case 6:		/* 3270 mode - like 1, but with xon/xoff local */
		    /* (might be nice to have "6" in telnet also...) */
	    sb.c_iflag &= ~INLCR;		/* no LF -> CR on input */
	    sb.c_iflag &= ~BRKINT;		/* ignore BREAK */
	    if( !xon )				/* if xon not set */
		sb.c_iflag &= ~(IXANY|IXOFF|IXON);  /* no local XON/XOFF */
	    sb.c_lflag &= ~(ISIG|ICANON);	/* raw mode */
	    sb.c_oflag &= ~OCRNL;		/* no CR -> NL on output */
	    /* preserve tab delays, but turn off tab-to-space expansion */
	    if ((sb.c_oflag & TABDLY) == TAB3)
		sb.c_oflag &= ~TAB3;
	    sb.c_cc[VMIN] = 1;
	    sb.c_cc[VTIME] = 0;
	    if ((f == 1) || (f == 6)) {
		sb.c_iflag &= ~ICRNL;		/* CRMOD off on input */
	        sb.c_oflag &= ~ONLCR;		/* CRMOD off on output */
		sb.c_lflag &= ~ECHO; 		/* ECHO off */
	    } else {
		sb.c_iflag |= ICRNL;		/* CRMOD on on input */
		sb.c_oflag |= ONLCR;		/* CRMOD on on output */
		sb.c_lflag |= ECHO;		/* ECHO on */
	    }
	    if (f == 6) {
		tc = &tc3;
		tc3.c_cc[VINTR] = notc.c_cc[VINTR];
		tc3.c_cc[VQUIT] = notc.c_cc[VQUIT];
	    } else {
		/*
		 * If user hasn't specified one way or the other,
		 * then default to not trapping signals.
		 */
		if (!donelclchars) {
		    localchars = 0;
		}
		if (localchars) {
		    notc2 = notc;
		    notc2.c_cc[VINTR] = nterm.c_cc[VINTR];
		    notc2.c_cc[VQUIT] = nterm.c_cc[VQUIT];
		    tc = &notc2;
		} else {
		    tc = &notc;
		}
	    }
	    ltc = &noltc;
	    onoff = 1;
	    break;
    case 3:		/* local character processing, remote echo */
    case 4:		/* local character processing, local echo */
    case 5:		/* local character processing, no echo */
	    sb.c_iflag |= ICRNL;	    /* CRMOD on on input */
	    sb.c_iflag |= IXANY|IXOFF|IXON; /* local XON/XOFF */
	    sb.c_iflag &= ~INLCR;	    /* no LF to CR on input */
	    sb.c_iflag &= ~BRKINT;	    /* ignore BREAK */
	    sb.c_oflag |= ONLCR;	    /* CRMOD on on output */
	    sb.c_oflag &= ~OCRNL;	    /* no CR to NL on output */
	    sb.c_lflag |= ICANON;    	    /* cooked mode */
	    sb.c_lflag &= ~ISIG;            /* no local signals */
	    /* preserve tab delays, but turn off tab-to-space expansion */
	    if ((sb.c_oflag & TABDLY) == TAB3)
		sb.c_oflag &= ~TAB3;
	    if (f == 4)
		sb.c_lflag |= ECHO;
	    else
		sb.c_lflag &= ~ECHO;
	    notc2 = nterm;
	    tc = &notc2;
	    noltc2 = oltc;
	    /*
	     * If user hasn't specified one way or the other,
	     * then default to trapping signals.
	     */
	    if (!donelclchars) {
		localchars = 1;
	    }
	    if (localchars) {
		;
	    }
	    sb.c_cc[VEOL] = escape;
	    ltc = &noltc;
	    onoff = 1;
	    break;

    default:
	    return;
    }
    sb.c_cc[VINTR] = tc->c_cc[VINTR];
    sb.c_cc[VQUIT] = tc->c_cc[VQUIT];
    ioctl(tin, TCSETAW, (char *)&sb);
    ioctl(tin, TIOCSLTC, (char *)ltc);
#if	(!defined(TN3270)) || ((!defined(NOT43)) || defined(PUTCHAR))
    ioctl(tin, FIONBIO, (char *)&onoff);
    ioctl(tout, FIONBIO, (char *)&onoff);
#endif	/* (!defined(TN3270)) || ((!defined(NOT43)) || defined(PUTCHAR)) */
#if	defined(TN3270)
    if (noasynch == 0) {
	ioctl(tin, FIOASYNC, (char *)&onoff);
    }
#endif	/* defined(TN3270) */

    if (MODE_LINE(f)) {
	void doescape();

	(void) signal(SIGTSTP, (int (*)())doescape);
    } else if (MODE_LINE(old)) {
	(void) signal(SIGTSTP, SIG_DFL);
	sigsetmask(sigblock(0) & ~(1<<(SIGTSTP-1)));
    }
}
#endif

int
NetClose(fd)
int	fd;
{
    return close(fd);
}


void
NetNonblockingIO(fd, onoff)
int
	fd,
	onoff;
{
    ioctl(fd, FIONBIO, (char *)&onoff);
}

#if	defined(TN3270)
void
NetSigIO(fd, onoff)
int
	fd,
	onoff;
{
    ioctl(fd, FIOASYNC, (char *)&onoff);	/* hear about input */
}

void
NetSetPgrp(fd)
int fd;
{
    int myPid;

    myPid = getpid();
    fcntl(fd, F_SETOWN, myPid);
}
#endif	/*defined(TN3270)*/

/*
 * Various signal handling routines.
 */

static void
deadpeer()
{
	setcommandmode();
	longjmp(peerdied, -1);
}

static void
intr()
{
    if (localchars) {
	intp();
	return;
    }
    setcommandmode();
    longjmp(toplevel, -1);
}

static void
intr2()
{
    if (localchars) {
	sendbrk();
	return;
    }
}

#ifdef  SIGWINCH
static void
sendwin(sig)
    int sig;
{
    if (connected) {
    sendnaws();
    }
}
#endif /* SIGWINCH */

static void
doescape()
{
    command(0);
}

void
sys_telnet_init()
{
    (void) signal(SIGINT, (int (*)())intr);
    (void) signal(SIGQUIT, (int (*)())intr2);
    (void) signal(SIGPIPE, (int (*)())deadpeer);

#ifdef SIGWINCH
    (void) signal(SIGWINCH, (int (*)())sendwin);
#endif /* SIGWINCH */

    setconnmode();

    NetNonblockingIO(net, 1);

#if	defined(TN3270)
    if (noasynchnet == 0) {			/* DBX can't handle! */
	NetSigIO(net, 1);
	NetSetPgrp(net);
    }
#endif	/* defined(TN3270) */

#if	defined(SO_OOBINLINE)
    if (SetSockOpt(net, SOL_SOCKET, SO_OOBINLINE, 1) == -1) {
	perror("SetSockOpt");
    }
#endif	/* defined(SO_OOBINLINE) */
}

/*
 * Process rings -
 *
 *	This routine tries to fill up/empty our various rings.
 *
 *	The parameter specifies whether this is a poll operation,
 *	or a block-until-something-happens operation.
 *
 *	The return value is 1 if something happened, 0 if not.
 */

int
process_rings(netin, netout, netex, ttyin, ttyout, poll)
int poll;		/* If 0, then block until something to do */
{
    register int c;
		/* One wants to be a bit careful about setting returnValue
		 * to one, since a one implies we did some useful work,
		 * and therefore probably won't be called to block next
		 * time (TN3270 mode only).
		 */
    int returnValue = 0;
    static struct timeval TimeValue = { 0 };

    if (netout) {
	FD_SET(net, &obits);
    } 
    if (ttyout) {
	FD_SET(tout, &obits);
    }
#if	defined(TN3270)
    if (ttyin) {
	FD_SET(tin, &ibits);
    }
#else	/* defined(TN3270) */
    if (ttyin) {
	FD_SET(tin, &ibits);
    }
#endif	/* defined(TN3270) */
#if	defined(TN3270)
    if (netin) {
	FD_SET(net, &ibits);
    }
#   else /* !defined(TN3270) */
    if (netin) {
	FD_SET(net, &ibits);
    }
#   endif /* !defined(TN3270) */
    if (netex) {
	FD_SET(net, &xbits);
    }
    if ((c = select(FD_SETSIZE, &ibits, &obits, &xbits,
			(poll == 0)? (struct timeval *)0 : &TimeValue)) < 0) {
	if (c == -1) {
		    /*
		     * we can get EINTR if we are in line mode,
		     * and the user does an escape (TSTP), or
		     * some other signal generator.
		     */
	    if (errno == EINTR) {
		return 0;
	    }
#	    if defined(TN3270)
		    /*
		     * we can get EBADF if we were in transparent
		     * mode, and the transcom process died.
		    */
	    if (errno == EBADF) {
			/*
			 * zero the bits (even though kernel does it)
			 * to make sure we are selecting on the right
			 * ones.
			*/
		FD_ZERO(&ibits);
		FD_ZERO(&obits);
		FD_ZERO(&xbits);
		return 0;
	    }
#	    endif /* defined(TN3270) */
		    /* I don't like this, does it ever happen? */
	    printf("sleep(5) from telnet, after select\r\n");
	    sleep(5);
	}
	return 0;
    }

    /*
     * Any urgent data?
     */
    if (FD_ISSET(net, &xbits)) {
	FD_CLR(net, &xbits);
	SYNCHing = 1;
	ttyflush(1);	/* flush already enqueued data */
    }

    /*
     * Something to read from the network...
     */
    if (FD_ISSET(net, &ibits)) {
	int canread;

	FD_CLR(net, &ibits);
	canread = ring_empty_consecutive(&netiring);
#if	!defined(SO_OOBINLINE)
	    /*
	     * In 4.2 (and some early 4.3) systems, the
	     * OOB indication and data handling in the kernel
	     * is such that if two separate TCP Urgent requests
	     * come in, one byte of TCP data will be overlaid.
	     * This is fatal for Telnet, but we try to live
	     * with it.
	     *
	     * In addition, in 4.2 (and...), a special protocol
	     * is needed to pick up the TCP Urgent data in
	     * the correct sequence.
	     *
	     * What we do is:  if we think we are in urgent
	     * mode, we look to see if we are "at the mark".
	     * If we are, we do an OOB receive.  If we run
	     * this twice, we will do the OOB receive twice,
	     * but the second will fail, since the second
	     * time we were "at the mark", but there wasn't
	     * any data there (the kernel doesn't reset
	     * "at the mark" until we do a normal read).
	     * Once we've read the OOB data, we go ahead
	     * and do normal reads.
	     *
	     * There is also another problem, which is that
	     * since the OOB byte we read doesn't put us
	     * out of OOB state, and since that byte is most
	     * likely the TELNET DM (data mark), we would
	     * stay in the TELNET SYNCH (SYNCHing) state.
	     * So, clocks to the rescue.  If we've "just"
	     * received a DM, then we test for the
	     * presence of OOB data when the receive OOB
	     * fails (and AFTER we did the normal mode read
	     * to clear "at the mark").
	     */
	if (SYNCHing) {
	    int atmark;

	    ioctl(net, SIOCATMARK, (char *)&atmark);
	    if (atmark) {
		c = recv(net, netiring.supply, canread, MSG_OOB);
		if ((c == -1) && (errno == EINVAL)) {
		    c = recv(net, netiring.supply, canread, 0);
		    if (clocks.didnetreceive < clocks.gotDM) {
			SYNCHing = stilloob(net);
		    }
		}
	    } else {
		c = recv(net, netiring.supply, canread, 0);
	    }
	} else {
	    c = recv(net, netiring.supply, canread, 0);
	}
	settimer(didnetreceive);
#else	/* !defined(SO_OOBINLINE) */
	c = recv(net, netiring.supply, canread, 0);
#endif	/* !defined(SO_OOBINLINE) */
	if (c < 0 && errno == EWOULDBLOCK) {
	    c = 0;
	} else if (c <= 0) {
	    return -1;
	}
	if (netdata) {
	    Dump('<', netiring.supply, c);
	}
	if (c)
	    ring_supplied(&netiring, c);
	returnValue = 1;
    }

    /*
     * Something to read from the tty...
     */
    if (FD_ISSET(tin, &ibits)) {
	FD_CLR(tin, &ibits);
	c = TerminalRead(ttyiring.supply, ring_empty_consecutive(&ttyiring));
	if ((c < 0 && errno == EWOULDBLOCK) || (c == 0 && !isatty(tin))) {
	    c = 0;
	} else {
	    /* EOF detection for line mode!!!! */
	    if ((c == 0) && MODE_LOCAL_CHARS(globalmode) && isatty(tin)) {
			/* must be an EOF... */
		*ttyiring.supply = termEofChar;
		c = 1;
	    }
	    if (c <= 0) {
		return -1;
	    }
	    ring_supplied(&ttyiring, c);
	}
	returnValue = 1;		/* did something useful */
    }

    if (FD_ISSET(net, &obits)) {
	FD_CLR(net, &obits);
	returnValue |= netflush();
    }
    if (FD_ISSET(tout, &obits)) {
	FD_CLR(tout, &obits);
	returnValue |= ttyflush(SYNCHing|flushout);
    }

    return returnValue;
}
#endif	/* defined(unix) */

int
TerminalWindowSize(rows, cols)
    long *rows, *cols;
{
#ifdef  TIOCGWINSZ
    struct winsize ws;

    if (ioctl(fileno(stdin), TIOCGWINSZ, (char *)&ws) >= 0) {
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
    }
#endif  /* TIOCGWINSZ */
    return 1;
}

