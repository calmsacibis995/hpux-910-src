/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/tty_bk.c,v $
 * $Revision: 1.8.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:12:54 $
 */

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

#ifdef __hp9000s300
#include "nbk.h"
#endif
#ifdef hp9000s800
#include "bk.h"
#endif

#if NBK > 0
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"

/*
 * Line discipline for Berkeley network.
 *
 * This supplies single lines to a user level program
 * with a minimum of fuss.  Lines are newline terminated.
 *
 * This discipline requires that tty device drivers call
 * the line specific l_ioctl routine from their ioctl routines,
 * assigning the result to cmd so that we can refuse most tty specific
 * ioctls which are unsafe because we have ambushed the
 * teletype input queues, overlaying them with other information.
 */

/*
 * Open as networked discipline.  Called when discipline changed
 * with ioctl, this assigns a buffer to the line for input, and
 * changing the interpretation of the information in the tty structure.
 */
/*ARGSUSED*/
bkopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct buf *bp;

	if (tp->t_line == NETLDISC)
		return (EBUSY);	/* sometimes the network opens /dev/tty */
	bp = geteblk(1024);
	ttyflush(tp, FREAD|FWRITE);
	tp->t_bufp = bp;
	tp->t_cp = (char *)bp->b_un.b_addr;
	tp->t_inbuf = 0;
	tp->t_rec = 0;
	return (0);
}

/*
 * Break down... called when discipline changed or from device
 * close routine.
 */
bkclose(tp)
	register struct tty *tp;
{
	register int s;

	s = spl5();
	wakeup((caddr_t)&tp->t_rawq);
	if (tp->t_bufp) {
		brelse(tp->t_bufp);
		tp->t_bufp = 0;
	} else
		printf("bkclose: no buf\n");
	tp->t_cp = 0;
	tp->t_inbuf = 0;
	tp->t_rec = 0;
	tp->t_line = 0;		/* paranoid: avoid races */
	splx(s);
}

/*
 * Read from a network line.
 * Characters have been buffered in a system buffer and are
 * now dumped back to the user in one fell swoop, and with a
 * minimum of fuss.  Note that no input is accepted when a record
 * is waiting.  Our clearing tp->t_rec here allows further input
 * to accumulate.
 */
bkread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	register int s;
	int error;

	if ((tp->t_state&TS_CARR_ON)==0)
		return (-1);
	s = spl5();
	while (tp->t_rec == 0 && tp->t_line == NETLDISC)
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	splx(s);
	if (tp->t_line != NETLDISC)
		return (-1);
	error = uiomove(tp->t_bufp->b_un.b_addr, tp->t_inbuf, UIO_READ, uio);
	tp->t_cp = (char *)tp->t_bufp->b_un.b_addr;
	tp->t_inbuf = 0;
	tp->t_rec = 0;
	return (error);
}

/*
 * Low level character input routine.
 * Stuff the character in the buffer, and wake up the top
 * half after setting t_rec if this completes the record
 * or if the buffer is (ick!) full.
 *
 * Thisis where the formatting should get done to allow
 * 8 character data paths through escapes.
 *
 * This rutine should be expanded in-line in the receiver
 * interrupt routine of the dh-11 to make it run as fast as possible.
 */
bkinput(c, tp)
register c;
register struct tty *tp;
{

	if (tp->t_rec)
		return;
	*tp->t_cp++ = c;
	if (++tp->t_inbuf == 1024 || c == '\n') {
		tp->t_rec = 1;
		wakeup((caddr_t)&tp->t_rawq);
	}
}

/*
 * This routine is called whenever a ioctl is about to be performed
 * and gets a chance to reject the ioctl.  We reject all teletype
 * oriented ioctl's except those which set the discipline, and
 * those which get parameters (gtty and get special characters).
 */
/*ARGSUSED*/
bkioctl(tp, cmd, data, flag)
	struct tty *tp;
	caddr_t data;
{

	if ((cmd>>8) != 't')
		return (-1);
	switch (cmd) {

	case TIOCSETD:
	case TIOCGETD:
	case TIOCGETP:
	case TIOCGETC:
		return (-1);
	}
	return (ENOTTY);
}
#endif
