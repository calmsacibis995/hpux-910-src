/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/tt0.c,v $
 * $Revision: 1.8.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:21:54 $
 */
/* HPUX_ID: @(#)tt0.c+timeout fix	52.2		88/06/10 */

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
 * Line discipline 0
 */

#include "../h/param.h"
#include "../h/file.h"
#include "../h/tty.h"
#include "../h/unistd.h"        /* for _POSIX_VDISABLE */
#include "../h/termios.h"
#include "../h/dk.h"

#define signal gsignal

extern char partab[];
extern tttimeo_sw();

/*
 * Place character(s) on raw TTY input queue, putting in delimiters
 * and waking up top half as needed.
 * Also echo if required.
 *
 * 	unprocessed is the number of characters received but not yet
 *		processed by this line discipline
 */
ttin(tp, ucp, ncode, unprocessed)
register struct tty *tp;
union {
	long ch;
	struct cblock *ptr;
} ucp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Place character(s) on raw TTY input queue, putting
 |			in delimiters and waking up top half as needed.
 |			Also echo if required.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 31 Aug 89  garth	Fix for DTS INDaa05924.
 |			Removed check for IEXTEN when a SUSP character is
 |			received.  The check for IEXTEN is not required by
 |			POSIX and is causes a POSIX test to fail. 
 |			
 | 07 Sep 89  murgia    Fix for SR 4700-664300 (aka, DTS INDaa02923).
 |                      (SR's correspond to 800 failures)
 |                      Removed conditional call to tttimeo.  Before fix,
 |                      if there was a timer currently active it would
 |                      complete before a new timer value could be set.
 |                      This caused read timeouts, in noncanonical mode,
 |                      to behave incorrectly.
 |
 |  16 Jan 90  garth	Added support for the iostat command.  Update
 |			tk_nin with the number of bytes being processed.
 |
 |  11 Dec 90  dmurray	Fix for DTS INDaa08438.
 |			Moved checks for special characters (QUIT, SUSP,
 |			and INTR) to happen before the character(s) were put
 |			onto the raw queue.  Previously special characters
 |			were making their way onto the raw queue before the
 |			code was processing them.  This caused a problem when
 |			NOFLSH was set since the raw queue would not be
 |			flushed and the special character would not be
 |			discarded.  This defect was discovered by XPG3
 |			standards testing.  A similar fix was made in the
 |			series 800 line discipline. (DTS INDaa07994)
 |
 --------------------------------------------------------------------------*/
	register c = ucp.ch;
	register iflg, lflg;
	register unsigned char *cp, *loop_cp;
	register int  num_chars;
	unsigned char ch;

	iflg = tp->t_iflag;
	switch (ncode) {
	case 0:
		/* This assignment is necessary for the iostat command.
		 * Do not remove the following line!
		 * NOTE: If a framing, parity or overrun error occurs,
		 * tk_nin will only be updated once with the character
		 * in error.
	 	 */
		tk_nin += 1;

		ncode++;
		if (c&PERROR && !(iflg&INPCK))
			c &= ~PERROR;
		if (c&(FRERROR|PERROR|OVERRUN)) {
			if ((c&0377) == 0) {
				if (iflg&IGNBRK)
					return;
				if (iflg&BRKINT) {
					signal(tp->t_pgrp, SIGINT);
					ttyflush(tp, (FREAD|FWRITE));
					return;
				}
			} else {
				if (iflg&IGNPAR)
					return;
			}
			if (iflg&PARMRK) {
				ttin(tp, 0377, 1, unprocessed+2);
				ttin(tp, 0, 1, unprocessed+1);
			} else
				c = 0;
			c |= 0400;
		} else {
			if (iflg&ISTRIP)
				c &= 0177;
			else {
				c &= 0377;
				if (c == 0377 && iflg&PARMRK)
					if (putc(0377, &tp->t_rawq))
						return;
			}
		}
		if (iflg&IXON) {
			if (tp->t_state&TTSTOP) {
				if (c == CSTART || (iflg&IXANY && tp->t_lflag&IEXTEN))
					(*tp->t_proc)(tp, T_RESUME);
			} else {
				if (c == CSTOP)
					(*tp->t_proc)(tp, T_SUSPEND);
			}
			if (c == CSTART || c == CSTOP)
				return;
		}
		if (c == '\n' && iflg&INLCR)
			c = '\r';
		else if (c == '\r')
			if (iflg&IGNCR)
				return;
			else if (iflg&ICRNL)
				c = '\n';
		if (iflg&IUCLC && tp->t_lflag&IEXTEN && 'A' <= c && c <= 'Z')
			c += 'a' - 'A';
	case 1:
		/*
		 * Don't put the character onto the raw queue if it
		 * is a special character and ISIG is set.
		 */
		lflg = tp->t_lflag;
		if (lflg&ISIG) {
			if (_T_cc_enabled(c, VINTR)) {
				signal(tp->t_pgrp, SIGINT);
				if (!(lflg&NOFLSH))
					ttyflush(tp, (FREAD|FWRITE));
				return;
			}
			if (_T_cc_enabled(c, VQUIT)) {
				signal(tp->t_pgrp, SIGQUIT);
				if (!(lflg&NOFLSH))
					ttyflush(tp, (FREAD|FWRITE));
				return;
			}
#ifdef	BSDJOBCTL
			if (_T_cc_enabled(c, VSUSP)) {
				if (!(lflg&NOFLSH))
					(void) ttyflush(tp, (FREAD|FWRITE));
				gsignal(tp->t_pgrp, SIGTSTP);
				return;
			}
#endif	/* BSDJOBCTL */
		}
		ch = c;
		if (putc(c, &tp->t_rawq))
			return;
		cp = &ch;

		break;
	
	default:
		cp = (unsigned char *)&ucp.ptr->c_data[ucp.ptr->c_first];
		loop_cp = cp;
		num_chars = ucp.ptr->c_last - ucp.ptr->c_first;
		lflg = tp->t_lflag;
		if (lflg&ISIG)
			while (num_chars --) {
				c = *loop_cp++;
				if (_T_cc_enabled(c, VINTR)) {
					signal(tp->t_pgrp, SIGINT);
					if (!(lflg&NOFLSH))
						ttyflush(tp, (FREAD|FWRITE));
					continue;
				}
				if (_T_cc_enabled(c, VQUIT)) {
					signal(tp->t_pgrp, SIGQUIT);
					if (!(lflg&NOFLSH))
						ttyflush(tp, (FREAD|FWRITE));
					continue;
				}
#ifdef	BSDJOBCTL
				if (_T_cc_enabled(c, VSUSP)) {
					if (!(lflg&NOFLSH))
						(void) ttyflush(tp, (FREAD|FWRITE));
					gsignal(tp->t_pgrp, SIGTSTP);
					continue;
				}
#endif	/* BSDJOBCTL */
				if (putc(c, &tp->t_rawq))
					return;
				tk_nin ++;
			}
		else {
			putcb(ucp.ptr, &tp->t_rawq);
			/* This assignment is necessary for the iostat command.
		 	 * Do not remove the following line!
	 		 */
			tk_nin += num_chars;
		}
		break;
	}
	if ((tp->t_rawq.c_cc+unprocessed) > TTXOHI) {
		if (((iflg&IXOFF) || (tp->t_dvr_fc&DFLOW)) && !(tp->t_state&TBLOCK))
/*
 * don't send XOFF unless a read call can be completed.  Otherwise, the raw
 * queue will never decrease in size, and XON will never be sent.
 */
			if (tp->t_delct > 0)
				(*tp->t_proc)(tp, T_BLOCK, 0);
		if (tp->t_rawq.c_cc > TTYHOG) {
			ttyflush(tp, FREAD);
			return;
		}
	}
	lflg = tp->t_lflag;
	if (lflg) while (ncode--) {
		c = *cp++;
		lflg = tp->t_lflag;
		if (lflg&ICANON) {
			if (c == '\n') {
				if (lflg&ECHONL)
					lflg |= ECHO;
				tty_sigio_check(tp);
				tp->t_delct++;
			} else if (_T_cc_enabled(c, VEOL)) {
				tty_sigio_check(tp);
				tp->t_delct++;
			}
			if (!(tp->t_state&ESC)) {
				if (c == '\\')
					tp->t_state |= ESC;
				if (_T_cc_enabled(c, VERASE) && lflg&ECHOE) {
					if (lflg&ECHO)
						ttxput(tp, '\b', 0);
					lflg |= ECHO;
					ttxput(tp, ' ', 0);
					c = '\b';
				} else if (_T_cc_enabled(c, VKILL) && lflg&ECHOK) {
					if (lflg&ECHO)
						ttxput(tp, c, 0);
					lflg |= ECHO;
					c = '\n';
				} else if (_T_cc_enabled(c, VEOF)) {
					lflg &= ~ECHO;
					tty_sigio_check(tp);
					tp->t_delct++;
				}
			} else {
				if (c != '\\' ||
				    ((lflg&(XCASE|IEXTEN))==(XCASE|IEXTEN)))
					tp->t_state &= ~ESC;
			}
		}
		if (lflg&ECHO) {
			ttxput(tp, c, 0);
			(*tp->t_proc)(tp, T_OUTPUT);
		}
	}
	if (!(lflg&ICANON)) {
		tp->t_state &= ~RTO;
		if (tp->t_rawq.c_cc >= tp->t_cc[VMIN]) {
			tty_sigio_check(tp);
			tp->t_delct = 1;
			/*
			 * Since the read has been satisfied
			 * remove the currently active timer
			 * (if there is one).
		    	 */
		    	if (tp->t_state&TACT) {
		    		untimeout(tttimeo_sw,tp);
			    	tp->t_state &= ~TACT;
		    	}
		} else if (tp->t_cc[VTIME]) {
/*			if (!(tp->t_state&TACT))
*/
			tttimeo(tp);
		}
	}
	if (tp->t_delct) { /* have read buffer to pass back to user */
		if (tp->t_state&IASLP) {
			tp->t_state &= ~IASLP;
			wakeup((caddr_t)&tp->t_rawq);
		}
		if (tp->t_rsel) {
			selwakeup(tp->t_rsel, tp->t_state & RCOLL);
			tp->t_state &= ~RCOLL;
			tp->t_rsel = 0;
		}
	}
}

/*
 * Put character(s) on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * It is called both from the top half for output, and from
 * interrupt level for echoing.
 */
ttxput(tp, ucp, ncode)
register struct tty *tp;
struct cblock *ucp;
register ncode;
{
	register int cs;
	register flg;
	register unsigned char *cp;
	register char *colp;
	register int ctype;
	register int c;
	unsigned char ch = (unsigned char)ucp;

	flg = tp->t_oflag;
	if (ncode == 0) {
		if (tp->t_outq.c_cc >= TTYHOG)
			return;
		ncode++;
		if (!(flg&OPOST)) {
			putc(ch, &tp->t_outq);
			return;
		}
		ucp = NULL;
		cp = &ch;
	} else {
		if (!(flg&OPOST)) {
			putcb(ucp, &tp->t_outq);
			return;
		}
		cp = (unsigned char *)&ucp->c_data[ucp->c_first];
	}
	colp = &tp->t_col;

	while (ncode--) {
		c = 0;
		switch (partab[cs = *cp++] & 077) {
	
		case 0:	/* ordinary */
			(*colp)++;
	
		case 1:	/* non-printing */
			break;
	
		case 2:	/* backspace */
			if (flg&BSDLY)
				c = 1;
			if (*colp)
				(*colp)--;
			break;
	
		case 3:	/* line feed */
			if (flg&ONLRET)
				goto cr;
			if (flg&ONLCR) {
				if (!(flg&ONOCR && *colp==0)) {
					putc('\r', &tp->t_outq);
				}
				goto cr;
			}
		nl:
			if (flg&NLDLY)
				c = 5;
			break;
	
		case 4:	/* tab */
			c = 8 - ((*colp)&07);
			*colp += c;
			ctype = flg&TABDLY;
			if (ctype == TAB0) {
				c = 0;
			} else if (ctype == TAB1) {
				c = 2;
			} else if (ctype == TAB2) {
				c = 4;
			} else if (ctype == TAB3) {
				do
					putc(' ', &tp->t_outq);
				while (--c);
				continue;
			}
			break;
	
		case 5:	/* vertical tab */
			if (flg&VTDLY)
				cs = '\n';
			break;
	
		case 6:	/* carriage return */
			if (flg&OCRNL) {
				cs = '\n';
				goto nl;
			}
			if (flg&ONOCR && *colp == 0)
				continue;
		cr:
 			if (ctype = flg&CRDLY) {
 				if (ctype == CR1) {
 					if (*colp) {
 						c = (*colp>>4) +3;
 						if (c<6)
 							c = 6;
 					}
 				} else if (ctype == CR2) {
 					c = 2;
 				} else if (ctype == CR3) {
 					c = 4;
 				}
			}
			*colp = 0;
			break;
	
		case 7:	/* form feed */
			if (flg&FFDLY)
				cs = '\n';
			break;
		}
		if (flg&OLCUC && 'a' <= cs && cs <= 'z')
			cs += 'A' - 'a';
		putc(cs, &tp->t_outq);

/*  The following nonsence is an attempt to get rid of timed delays
**  that seem to be necessary for support of asr35 type of machines.
**  Modern buffered rs232 cards seem to leave out the ability to tell
**  when the last char has been output to the device, makeing timed
**  delays almost impossible to do relayably.  Therefore this approach
**  is being tried.  It will spool the forms use of a 35 type of tty.
**  Well this is modern times where forms are done on proticall type 
**  of devices -- YEA?;
*/
		if (c) {
			if (flg&OFDEL)
				cs = 0177;
			else
				cs = 0;
			if (c > 9)
				c = 9;
			do
				putc(cs, &tp->t_outq);
			while(--c);
		}
	}
	if (ucp != NULL)
		putcf(ucp);
}

/*
 * Get next function from output queue.
 * Called from xmit interrupt complete.
 */

ttout(tp) 
register struct tty *tp;
{
}

ttttimeo(tp)
register struct tty *tp;
{
	tttimeo(tp);
}

tttimeo_sw(tp)
register struct tty *tp;
{
	if (tp->tttimeo_intloc.proc == 0)
		sw_trigger(&tp->tttimeo_intloc, ttttimeo, tp, 0, tp->t_int_lvl);
}

tttimeo(tp)
register struct tty *tp;
/*---------------------------------------------------------------------------
 | FUNCTION -	begin a raw timer, or else stop a raw timer that had
 |		had already been started.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 |
 | 07 Sep 89  murgia    Fix for SR 4700-664300 (aka, DTS INDaa02923).
 |			(SR's correspond to 800 failures)
 |                      Added conditional call to untimeout.  Before fix,
 |                      if there was a timer currently active it would
 |                      complete before a new timer value could be set.
 |                      This caused read timeouts, in noncanonical mode,
 |                      to behave incorrectly.
 |
 --------------------------------------------------------------------------*/
{
	if (tp->t_state&TACT) {
		/*
		 * Stop a previous raw timer
		 */
		 untimeout(tttimeo_sw,tp);
		 tp->t_state &= ~TACT;
	}
	if (tp->t_lflag&ICANON || tp->t_cc[VTIME] == 0)
		return;
	if (tp->t_rawq.c_cc == 0 && tp->t_cc[VMIN])
		return;
	if (tp->t_state&RTO) {
		tty_sigio_check(tp);
		tp->t_delct = 1;
		if (tp->t_state&IASLP) {
			tp->t_state &= ~IASLP;
			wakeup((caddr_t)&tp->t_rawq);
		}
		if (tp->t_rsel) {
			selwakeup(tp->t_rsel, tp->t_state & RCOLL);
			tp->t_state &= ~RCOLL;
			tp->t_rsel = 0;
		}
	} else {
		tp->t_state |= RTO|TACT;
		timeout(tttimeo_sw, tp, tp->t_cc[VTIME]*(HZ/10), NULL);
	}
}

/*
 * I/O control interface
 */
ttioctl(tp, cmd, arg, mode)
register struct tty *tp;
{
	ushort	chg;
	register x;

	switch(cmd) {
	case LDCHG:
		chg = tp->t_lflag ^ arg;
		if (!(chg&ICANON))
			break;
		x = splsx(tp->t_int_lvl);
		if (tp->t_canq.c_cc) {
			if (tp->t_rawq.c_cc) {
				tp->t_canq.c_cc += tp->t_rawq.c_cc;
				tp->t_canq.c_cl->c_next = tp->t_rawq.c_cf;
				tp->t_canq.c_cl = tp->t_rawq.c_cl;
			}
			tp->t_rawq = tp->t_canq;
			tp->t_canq = ttnulq;
		}
/*		tty_sigio_check(tp);	/* TEMP ?  not in s800 tty driver */
		tp->t_delct = tp->t_rawq.c_cc;
		splsx(x);
		break;

	default:
		break;
	}
}

/*
 * send SIGIO to user ?
 */
tty_sigio_check(tp)
register struct tty *tp;
{
	if (tp->t_state & ASYNC) {
		if ((tp->t_delct == 0) && (tp->t_canq.c_cc == 0))
				psignal(tp->t_pidprc, SIGIO);
	}
}
