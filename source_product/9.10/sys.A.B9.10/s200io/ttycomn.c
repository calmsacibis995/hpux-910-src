/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ttycomn.c,v $
 * $Revision: 1.8.84.5 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/08/26 11:58:19 $
 */

/* HPUX_ID: @(#)ttycomn.c	52.4		88/07/26 */

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

#include "../h/param.h"
#include "../h/errno.h"
#include "../h/file.h"
#include "../h/tty.h"
#include "../h/conf.h"
#include "../wsio/hpibio.h"
#include "../s200io/mux.h"
#include "../h/termios.h"
#include "../h/termiox.h"
#include "../h/modem.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"

/* This table maps unix symbolic baud values to 98626 uart baud values.
 * 153600/baudmap[BAUD] yields the baud rate.  This table is also used
 * for various timing calculations.
 */
 
short baudmap[] = {	/* For reading unix config */
		0xFFFF,	/* hangup */
		0x0C00,	/* 50 baud */
		0x0800,	/* 75 baud */
		0x0574,	/* 110 baud */
		0x0476,	/* 134.5 baud */
		0x0400,	/* 150 baud */
		0x0300,	/* 200 baud */
		0x0200,	/* 300 baud */
		0x0100,	/* 600 baud */
		0x00AA,	/* 900 baud */
		0x0080,	/* 1200 baud */
		0x0055,	/* 1800 baud */
		0x0040,	/* 2400 baud */
		0x002B, /* 3600 baud */
		0x0020,	/* 4800 baud */
		0x0015,	/* 7200 baud */
		0x0010, /* 9600 baud */
		0x0008,	/* 19200 baud */
		0x0004,	/* 38400 baud */
};

#define signal gsignal
#undef	timeout			/* use timeout() rather than Ktimeout */

extern struct tty *cons_tty;
extern int lbolt;

#define CCITT_IN	(MDCD | MCTS | MDSR | MRI)
#define CCITT_OUT	(MDTR | MRTS | MDRS)
#define CCITT_MSL	(MDCD | MCTS | MDSR)
#define SIMPLE_MSL	(MDCD)
#define CCITT_MCL	(MDTR | MRTS)
#define SIMPLE_MCL	(MDTR)

#define	NA_PERIOD	6
#define NA_SECONDS	tp->modem_status	/* kludge: re-use tp space */
#define NA_MINUTES	tp->t_time		/* kludge: re-use tp space */

#define CTS_PACING	0x08			/* minor number bit */

/* Temporary until it can be defined in tty.h */
#define	T_VHANGUP	-1

/* Temporary until data structures can be restructured. */
#define	HW_VHANGUP	*((u_int *)(tp->utility) + 1)

/* Macros for semaphore usage */

#define OPEN_SEMA	(tp->t_open_sema)

#undef  GET_OPEN_SEMA
#undef  RELEASE_OPEN_SEMA

#define GET_OPEN_SEMA\
	err	= 0;\
	while (OPEN_SEMA) {\
		if (sleep( (caddr_t)&OPEN_SEMA, TTIPRI|PCATCH )){\
			err	= EINTR;\
			break;\
		}\
		if (HW_VHANGUP) {\
			err	= EBADF;\
			break;\
		}\
	}\
	if (!err)\
  		OPEN_SEMA	= 1

#define RELEASE_OPEN_SEMA\
	if (OPEN_SEMA) {\
		OPEN_SEMA	= 0;\
		wakeup( (caddr_t)&OPEN_SEMA );\
	}


/* common-usage tty routines */

/* return bits/char, including start and stop bits */

tty_csize(tp)
struct tty *tp;
{
	register cflag = tp->t_cflag;
	register size;

	cflag &= CSIZE;
	if (cflag == CS8)	size = 8;
	else if (cflag == CS7)	size = 7;
	else if (cflag == CS6)	size = 6;
	else if (cflag == CS5)	size = 5;
	if (cflag & CSTOPB)     size += 1;	/* extra STOP bit */
	size += 2;				/* standard START+STOP bits */
	return(size);
}

/****************************************************************************
 tty_modem_lines - get the value of the modem lines
 ****************************************************************************/
tty_modem_lines(tp)
register struct tty *tp;
{
	if (tp == cons_tty) {
		if (tp->t_state & CCITT)
			return(CCITT_MSL);
		else
			return(SIMPLE_MSL);
	}
	else {
		tp->t_proc(tp, T_MODEM_STAT);
		if (tp->t_state & CCITT)
			return(tp->t_smcnl & CCITT_MSL);
		else
			return(tp->t_smcnl & SIMPLE_MSL);
	}
}


/****************************************************************************
 ttymodem -	modem control
 ****************************************************************************/
ttymodem(tp, flag)
register struct tty *tp;
{

	if (flag == ON) {
		if (tp->t_state & CCITT)
			tp->t_smcnl |= CCITT_MCL;
		else
			tp->t_smcnl |= SIMPLE_MCL;
	}
	else {
		if (tp->t_state & CCITT)
			tp->t_smcnl &= ~CCITT_MCL;
		else
			tp->t_smcnl &= ~SIMPLE_MCL;
	}

	tp->t_proc(tp, T_MODEM_CNTL);	/* modem control */

	return(tty_modem_lines(tp));	/* return modem status */
}

tty_MSL_check (tp, ms, OP, msl)

   register struct tty *tp;
   register unsigned long ms;
   int OP;
   register unsigned long msl;
{
	if (OP == LINE_SET) {
		/* If all the msl are set, then the connection is made */
		if ((ms & msl) == msl) {
			/* Wakeup anyone waiting for the open to complete */
			if ((tp->t_state & CARR_ON) == 0) {
				if (tp->t_state & CUL)
					wakeup (&tp->t_canq);
				else if (tp->t_state & TTYD)
					wakeup (&tp->t_slcnl);
				tp->t_state |= CARR_ON;
			}
		}
	}
	else {
		/* If one of the MSL has dropped, then break the connection */
		if (tp == cons_tty)	/* allways have connection */
			return ;	/* don't try to drop it */
		if ((tp->t_smcnl & (MCTS | MDSR)) == 0)	/* if modem clear */
			wakeup(&tp->timers[MTHANGUP]); /* wake up CCITT open */
		if ((ms & msl) != msl) {
				if ((tp->t_state & WOPEN) == 0) {
				    if (tp->t_cproc != NULL)
					psignal (tp->t_cproc, SIGHUP);
					/* modem disconnect - SIGHUP 
					   controlling process */
					ttymodem(tp, OFF);
				}
				tp->t_state &= ~CARR_ON;
				ttyflush (tp, (FREAD | FWRITE));
		}
	}
}

/*
 * If the user disabled CLOCAL mode, and the MSL lines
 * or the MCL lines are inactive, then drop the MCL lines,
 * and send the Hangup signal.
 */
tty_delta_CLOCAL(tp)
register struct tty *tp;
{

	if (tp->t_state & DELTA_CLOCAL) {
		if ((tp != cons_tty) && !(tp->t_state & TTY)
		   	&&
			((!(tp->t_state & CCITT) &&
			((tty_modem_lines(tp) != SIMPLE_MSL) ||
			((tp->t_smcnl & SIMPLE_MCL) != SIMPLE_MCL)))
	           		||
			((tp->t_state & CCITT) &&
			((tty_modem_lines(tp) != CCITT_MSL) ||
			((tp->t_smcnl & CCITT_MCL) != CCITT_MCL))))) {

			if (tp->t_state & CCITT)
				tty_MSL_check (tp, 0, LINE_CLEARED, CCITT_MSL);
			else
				tty_MSL_check (tp, 0, LINE_CLEARED, SIMPLE_MSL);
			}
		tp->t_state &= ~DELTA_CLOCAL;
	}
}

/****************************************************************************
 tty_na_TO - process NA t/o tick
 ****************************************************************************/
tty_na_TO(tp)
register struct tty *tp;
{
/*
 *	NA_SECONDS is the # of seconds at the NEXT timeout
 */
	untimeout(tty_na_TO, tp);	/* get rid of any extra timeouts */
	if (NA_SECONDS == 0)
		NA_MINUTES = 0;
	NA_SECONDS += NA_PERIOD;
	if (NA_SECONDS > 60) {
		NA_SECONDS = NA_PERIOD;
		NA_MINUTES++;
		if (	(tp->t_state & CCITT)
		   &&	(NA_MINUTES >= tp->timers[MTNOACTIVITY])
		   &&	(tp->timers[MTNOACTIVITY] != 0)	) {
			ttymodem(tp, OFF);	/* timer popped: close link */
			return;
		}
	}
	timeout(tty_na_TO, tp, HZ*NA_PERIOD);
}

/****************************************************************************
 * tty_open_TO (tp)
 *
 *   This procedure is responsible for handling any open timeouts which
 *   may occur during the operation of this driver.  When the timeout
 *   occurs, if the device is not set for CLOCAL mode and if the timer
 *   has not been set for 0 (zero), then all pending opens will be
 *   awakened and told that their open request has timed out.
 *
 ***************************************************************************/

tty_open_TO (tp)

   register struct tty *tp;

{
   /* Determine whether or not to process the timeout */
   tp->t_state &= ~OPEN_TO_PENDING;

   if (!(tp->t_cflag & CLOCAL) && (tp->timers[MTCONNECT] != 0)) {
      /* Wakeup all pending opens */
      tp->t_state |= OPEN_TIMED_OUT;
      wakeup (&tp->t_canq);
      wakeup (&tp->t_slcnl);
   }
}



/****************************************************************************
 * tty_CD_TO (tp)
 *
 *   This procedure is responsible for handling the loss of carrier timer.
 *   When carrier loss is detected, a timer is started.  When the timer
 *   completes, this procedure is invoked.  If carrier comes back, the
 *   timeout is cancelled.  Therefore, if this routine is ever invoked,
 *   carrier is gone, and the connection is broken.
 *
 ***************************************************************************/

tty_CD_TO (tp)

   register struct tty *tp;
{
	/* Determine if the timeout should be processed */
	tp->t_state &= ~CARRIER_TO_PENDING;

	if (!(tp->t_cflag & CLOCAL)) {	/* process the timeout */
		tty_MSL_check (tp, tty_modem_lines(tp), LINE_CLEARED,
					CCITT_MSL);
   	}
}



/****************************************************************************
 hangup_delay - delay here to allow modem to settle down
 ****************************************************************************/
hangup_delay(tp)	/* delay here to allow modem to settle down */
register struct tty *tp;
{
	if (tp->timers[MTHANGUP] != 0)
		delay ((HZ * tp->timers[MTHANGUP]) / 1000);
}

/***********************************************************************
 tty_modem_intr(tp) - process a modem interrupt
		this procedure runs at the interrupt level of the card
 ***********************************************************************/
tty_modem_intr(tp)
register struct tty *tp;
{
	register int modem_status, change, operation;

	operation = tp->t_smcnl;	/* previous state */
	tp->t_proc(tp, T_MODEM_STAT);	/* get current state, update t_smcnl */
	modem_status = tp->t_smcnl;
	change = modem_status ^ operation;

	/* Clear To Send line changed state */
	if (change & MCTS) {
		if (tp->t_state & CCITT) {
	   		if ((modem_status & MCTS) || (tp->t_cflag & CLOCAL))
	      			operation = LINE_SET;
	   		else
	      			operation = LINE_CLEARED;
	   		tty_MSL_check (tp, modem_status, operation, CCITT_MSL);
		}
		/* CTS handshake enabled ? */
		if ((tp->t_dev & CTS_PACING) || (tp->t_hw_flow_ctl & CTSXON)) {
			if (modem_status & MCTS)
				tp->t_proc(tp, T_RESUME);
			else
				tp->t_proc(tp, T_SUSPEND);
		}
	}

	/* Data Set Ready line changed state */
	if (change & MDSR) {
		if (tp->t_state & CCITT) {
			if ((modem_status & MDSR) || (tp->t_cflag & CLOCAL))
				operation = LINE_SET;
			else
				operation = LINE_CLEARED;
			tty_MSL_check (tp, modem_status, operation, CCITT_MSL);
		}
	}

	    /* Ring Indicator line changed state */
	if (change & MRI) {
		/*
	 	* If this is the beginning of a ring, then read
	 	* the clock, and store the time.
	 	*/
		if (modem_status & MRI) {
			tp->t_time = lbolt;	/* ticks since boot */
			tp->t_int_flag |= START_OF_RING;
	/* if WOPEN (Waiting-for-OPEN) set, might have a call-in collision */
			if (tp->t_state & WOPEN)
				wakeup(&tp->t_canq);
		}
	/*
	 * Otherwise, the ring is ending, so read the clock
	 * again, and compare the two readings; if they
	 * differ by at least 500, then the ring was valid.
	 */
		else {
			if (tp->t_int_flag & START_OF_RING) {
			   tp->t_int_flag &= ~START_OF_RING;
			   if (lbolt - tp->t_time >= HZ/2)  /* .5 sec */
			      if (tp->t_state & WAIT_FOR_RING) {
				 if (((tp->t_state & ISOPEN) &&
				   ((tp->t_state&(CUL|TTY))==0))
				   || !(tp->t_state & ISOPEN))
					wakeup (&tp->t_int_lvl);
				 if ((tp->t_state & ISOPEN)
				  && (tp->t_state & CCITT)
				  && (tp->t_state & TTYD_NDELAY)
			   	  && (ttymodem(tp, ON) == CCITT_MSL)) {
					tp->t_state |= CARR_ON;
					tp->t_state &= ~WAIT_FOR_RING;
				 }
			      }
			}
		}
	}

	/* Data Carrier Detect line changed state */
	if (change & MDCD) {
	if (tp->t_state & (SIMPLE | TTY)) {
	   if ((tp->t_cflag & CLOCAL) || (tp->t_state & TTY)
	      || (modem_status & MDCD))
		operation = LINE_SET;
	   else
		operation = LINE_CLEARED;
	   tty_MSL_check (tp, modem_status, operation, SIMPLE_MSL);
	}
	else {
	    if ((modem_status & MDCD) || (tp->t_cflag & CLOCAL)) {
		tty_MSL_check (tp, modem_status, LINE_SET, CCITT_MSL);
		untimeout(tty_CD_TO, tp);
		tp->t_state &= ~CARRIER_TO_PENDING;
		ttyflush(tp, FREAD);
	   	tp->t_state |= CARR_ON;		/* turn on rx/tx */
	   	(*tp->t_proc)(tp, T_RESUME);	/* restart output */
	    }
	    else {
		tp->t_state &= ~CARR_ON;	/* turn off rx/tx */
	   	(*tp->t_proc)(tp, T_SUSPEND);	/* stop output */

	      /* Start a loss of carrier timeout */
	      if (tp->timers[MTCARRIER] != 0) {
		 if (!(tp->t_state & CARRIER_TO_PENDING)) {
		    tp->t_state |= CARRIER_TO_PENDING;
		    timeout(tty_CD_TO, tp,
			     (HZ*tp->timers[MTCARRIER])/1000);
		 }
	      }
	      else
	         tty_MSL_check (tp, modem_status, LINE_CLEARED, CCITT_MSL);
	   }
        }
     }
}

/****************************************************************************
 tty_re_init -	re-initialize tty after close.  If CCITT connection was left
		active, then do not re-initialize, as per MODEM(4).
 ****************************************************************************/
tty_re_init(tp)
register struct tty *tp;
{
	register int	mask;

	if (tp != cons_tty) {
		if ( ! (tp->t_smcnl & MDTR) ) {
			/* reset hardware flow control states */
			tp->t_hw_flow_ctl = 0;
			/* reset terminal settings */
			ttinit(tp);
			tp->t_proc(tp, T_PARM);
		}
	}
}

/****************************************************************************
 ttycomn_open - called after device-dependent driver gets *tp from dev
 ****************************************************************************/
ttycomn_open(dev, tp, flag)
   register dev_t dev;
   register struct tty *tp;
   int flag;
{
/*---------------------------------------------------------------------------
 | FUNCTION -	ttycomn_open
 |
 | ASSUMPTION - The tp->utility pointer has already been intialized by the 
 |		serial drivers to point to the successful open count stored 
 |		somewhere in the driver-dependent data structure.
 |
 | MODIFICATION HISTORY
 |
 | Date       User	Changes
 | 21 Apr 92  dmurray	Added macros GET_OPEN_SEMA and RELEASE_OPEN_SEMA to
 |			perform semaphoring for ttycomn opens and closes.
 |			The change was prompted by a hotsite and was a
 |			fix to keep an open from happening before the close
 |			was able to fully complete.  The change lets a close
 |			keep the semaphore until it has had a chance to
 |			fully complete.  Opens however release the semaphore
 |			before sleeping and must re-acquire it upon awakening.
 |
 | 28 Apr 92  jlau	Fix for INDaa09938.
 |			Added check on error code returned by tty_CCITT_open()
 |			prior to calling tp->t_proc() near the end of the
 |			routine.
 |
 | 11 May 92  marcel	added PCATCH to all sleeps. Also initalized err to 0,
 |			to match with PCATCH code in GET_OPEN_SEMA macro. If
 |			you EINTR out of a sleep, you now go through
 |			ttycomn_close, to fix up the lines and open counts.
 |
 | 15 May 92  jlau	Restructured the code to track the open count
 |			and pending open count separately.  Added new varaible
 |			open_count to track successful open's and the existing
 |			cul_count, ttyd_count and tty_count will be used as
 |			pending open counts. With this change, the following 
 |			scenario will be prevented from	happending: 
 |
 |			    sleep 999999 < /dev/ttyd09&	(casue a pending open)
 |			    stty -a < /dev/tty09  	(stty worked fine)
 |			    fd = open("/dev/ttyd09", O_RDWR | O_NDELAY);
 |			    close(fd);
 |			    stty -a < /dev/tty09 (stty failed to open tty )
 |
 |			The problem was caused by the fact that the close
 |			after the no-delay open did not actually close the 
 |			port because it was not considered as the last open
 |			due to the confusion from the pending open (i.e. 
 |			open count = 2, one for pending and one for O_NDELAY).
 |		
 |			In addtion, we no longer call the ttycomn_close()
 |			(see 11 May 92 marcel above) here.  Instead, we will
 |			call a newly created routine ttycomn_cleanup() which
 |			is subset of the the ttycomn_close() routine.  This
 |			effort is done to make the S300 serial driver code 
 |			more in line with the S800 MUX driver code in the
 |			open/close handling.
 |
 --------------------------------------------------------------------------*/
	register int x, err;
	register struct proc *pp = u.u_procp;
	register u_int *open_count;

	open_count = (u_int *)tp->utility;

	/* Make sure the minor dev number is valid */
	if ((dev & DEV_CUL) && (dev & DEV_TTY))
		return(ENXIO);	/* cannot have direct and callout */
	
	if (tp == NULL)		/* bad minor number ? */
		return (ENXIO);

/*
 * since we're going to be mucking about with the *tp for quite
 * some time, we might as well go critical here.
 */
	x = spl6();

	GET_OPEN_SEMA;
	if (err)
		goto end;
	
	/* If the open request is for a mode different from the
	 * one currently in effect, then return an error, since
	 * the driver can only handle one mode at any time.
	 */
	if (  ((dev & CCITT_MODE) && (tp->t_state & SIMPLE)) ||
	     (((dev & CCITT_MODE) == 0) && (tp->t_state & CCITT)) )
		{
		err = ENXIO;
		goto end;
		}

	if ((tp->t_state & (ISOPEN | TTYD_PENDING)) == 0) {
		/* Initialize the device */
		if (dev & CCITT_MODE)
			tp->t_state |= CCITT;	/* for tp->t_proc(tp,T_PARM) */
		else
			tp->t_state |= SIMPLE;	/* for tp->t_proc(tp,T_PARM) */
		tty_re_init(tp);
	}
	else if (((dev & DEV_CUL) && (flag & (FNDELAY|FNBLOCK)) && (tp->t_state & TTY)) ||
		     (((dev & (DEV_TTY | DEV_CUL)) == 0) &&
		     (flag & (FNDELAY|FNBLOCK)) && ( ! (tp->t_open_type & TTYD_OPEN)))) {
		if (flag & FNBLOCK)
			err = EAGAIN;
		else
			err = EBUSY;
		goto end;
	}
	/*
	 * Priority of access:	Direct (DEV_TTY, TTY_OPEN), then
	 *			Call-Out (DEV_CUL, CUL_OPEN), then
	 *			Call-In (DEV_TTYD, TTYD_OPEN)
	 * If a higher priority access-type collides with an existing lower
	 * priority open, return EBUSY.  If a lower priority access-type
	 * collides with an existing higher priority open, the open will block
	 * until the existing open clears
	 */

	else if (((dev & DEV_TTY) && ( ! (tp->t_open_type & TTY_OPEN))) ||
		      ((dev & DEV_CUL) && (tp->t_open_type & TTYD_OPEN))) {
		/* 
		 * Return an error (EBUSY), because a lower priority
		 * open has already completed.
		 */
		err = EBUSY;
		goto end;
	}

	if (tp->t_state & CCITT)
		err = tty_CCITT_open (dev, tp, flag);
	else
		err = tty_simple_open (dev, tp, flag);

	if (err == 0)
		err = (*linesw[tp->t_line].l_open)(dev, tp);

	/*
 	 * Always decrement the pending count which was incremented by the
	 * tty_XXXX_open() routine.  If there was no open error, then set 
	 * tp->t_open_type else clean up the HW state appropriately if
	 * there is no more pending open or successful open.
 	 */
	if (dev & DEV_CUL) {
		tp->cul_count--;	
		if (err == 0)
			tp->t_open_type = CUL_OPEN;
		else {
			if ((tp->cul_count == 0) && (*open_count == 0)) {
				ttycomn_cleanup(dev, tp);
				goto end;
			}
		}
	} else if (dev & DEV_TTY) {
		tp->tty_count--;
		if (err == 0)
			tp->t_open_type = TTY_OPEN;
		else {
			if ((tp->tty_count == 0) && (*open_count == 0)) {
				ttycomn_cleanup(dev, tp);
				goto end;
			}
		}
	} else {	/* dev & DEV_TTYD */
		tp->ttyd_count--;
		if (err == 0)
			tp->t_open_type = TTYD_OPEN;
		else {
			if ((tp->ttyd_count == 0) && (*open_count == 0)) {
				ttycomn_cleanup(dev, tp);
				goto end;
			}
		}
	}

	/*------------------------------------------------------
	 |     Check to see if we need to set up this terminal
	 | as a controlling terminal for the process.
	 -----------------------------------------------------*/
	if (	(err == 0)			&&
		!(flag&FNOCTTY) 		&&
		(pp->p_pid	== pp->p_pgrp)	&&
		(pp->p_pid	== pp->p_sid)	&&
		(u.u_procp->p_ttyp == NULL)	&&
		(tp->t_cproc	== NULL)	){
		u.u_procp->p_ttyp	= tp;
		u.u_procp->p_ttyd	= dev;
		tp->t_pgrp	= pp->p_pgrp;
		tp->t_cproc	= pp;
		pp->p_ttyd = u.u_procp->p_ttyd;
		}

	/*------------------------------------------------------
	 |     Check error code returned by tty_CCITT_open()  
	 | before calling tp->t_proc().
	 -----------------------------------------------------*/
	if (err == 0) {
		if ((tp->t_dev & CTS_PACING) &&	/* CTS handshake enabled ? */
		    ((tp->t_smcnl & MCTS) == 0))	/* CTS line off ? */
			tp->t_proc(tp, T_SUSPEND);	/* ...then don't send */
		/*
		 * Currently, the (tp->t_state & ISOPEN) flag is not
		 * checked prior to initializing the HW.  Will investigate
		 * this issue in the when we rewrite this driver.
		 */

		tp->t_proc(tp, T_PARM);		/* initialize card */

		/* Finally, increment the successful open count */
		(*open_count)++;
	}

end:	RELEASE_OPEN_SEMA;
	splx(x);
	return(err);
}

/**********************************************************************
 * tty_simple_open (dev, tp, flag)
 *
 *  This is the open routine used each time a serial card is opened
 *  for use in SIMPLE mode.
 *
 *********************************************************************/

tty_simple_open (dev, tp, flag)

   register dev_t dev;
   register struct tty *tp;
   int flag;

{
/*---------------------------------------------------------------------------
 | FUNCTION -           tty_simple_open
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 Apr 92	dmurray	Added macros GET_OPEN_SEMA and RELEASE_OPEN_SEMA to
 |			perform semaphoring for ttycomn opens and closes.
 |			The change was prompted by a hotsite and was a
 |			fix to keep an open from happening before the close
 |			was able to fully complete.  The change lets a close
 |			keep the semaphore until it has had a chance to
 |			fully complete.  Opens however release the semaphore
 |			before sleeping and must re-acquire it upon awakening.
 |			tty_simple_open() keeps the semaphore when returning
 |			to ttycomn_open(), and ttycomn_open() releases it
 |			before returning.
 |			NOTE: This may need to be changed if tty_simple_open()
 |			      is called by another routine.
 |
 | 11 May 92	marcel	Added PCATCH to all sleeps.  Also, added reg int err,
 |			and initialized it to 0, to go with modified SEMA
 |			macros.
 |
 | 22 May 92	dmurray	Added vhangup checks to sleeps.
 |
 --------------------------------------------------------------------------*/

	int	err	= 0;
  
	/*
	 * Keep track of who's waiting to complete an open.
	 */
	if (dev & DEV_CUL) {
		tp->t_state |= (CUL | SIMPLE);
		tp->cul_count++;
		if (tp->t_state & TTY) {
			RELEASE_OPEN_SEMA;
			if (sleep (&tp->t_canq, TTIPRI|PCATCH))
				return (EINTR);
			if (HW_VHANGUP)
				return (EBADF);
			GET_OPEN_SEMA;
		}
	}
	else if (dev & DEV_TTY) {
		tp->t_state |= (TTY | SIMPLE);
		tp->tty_count++;
	}
	else {
		tp->t_state |= (TTYD | SIMPLE);
		tp->ttyd_count++;
		if ((tp->t_state & ISOPEN) && (tp->t_state & (CUL | TTY))) {
			RELEASE_OPEN_SEMA;
			if (sleep (&tp->t_slcnl, TTIPRI|PCATCH))
				return (EINTR);
			if (HW_VHANGUP)
				return (EBADF);
			GET_OPEN_SEMA;
		}
	}
	if (err) /* this is set in GET_OPEN_SEMA */
		return (err);
	
check_carrier:
	/*
	 * If the user has specified that we are 'talking' by means
	 * of a direct connect line, or if modem control has been
	 * disabled (CLOCAL set), then don't bother activating the
	 * DTR line, and don't worry about the DCD line.  However, if
	 * modem control is enabled, then activate the DTR line,
	 * and look at the current DCD state.
	 */
	if ((dev & DEV_TTY) || (tp->t_cflag & CLOCAL) ||
				(ttymodem (tp, ON) == SIMPLE_MSL))
		tp->t_state |= CARR_ON;
	else
		tp->t_state &= ~CARR_ON;

	/*
	 * If the serial card is being opened for non-blocking 
	 * operation or as a direct connect device (tty), then
	 * don't wait for the DCD line to go active - complete
	 * the open IMMEDIATELY! (or return an error, if necessary).
	 * If we are open for blocking mode, or if a higher priority
	 * open has completed earlier, then put the calling process
	 * to sleep, until the DCD line goes active, or until the
	 * higher priority open closes.
	 */

	if (flag & (FNDELAY|FNBLOCK)) {
	   /* Complete the open */
		if ((dev & (DEV_TTY | DEV_CUL)) == 0) {
			/* Force any pending CUL opens to return EBUSY */
			tp->t_state |= TTYD_NDELAY;
			wakeup (&tp->t_canq);
		}
		
	}
	else {
		/* Wait for the device to become ready */
		if (( ! (tp->t_state & CARR_ON)) || 
			((tp->t_state & TTY) && ( !(dev & DEV_TTY))) ||
			((tp->t_state & CUL)&&( !(dev & (DEV_TTY|DEV_CUL))))) {

			tp->t_state |= WOPEN;
			/* Start sleeping */
			if (dev & DEV_CUL) {
				RELEASE_OPEN_SEMA;
				if (sleep (&tp->t_canq, TTIPRI|PCATCH))
					return (EINTR);
				if (HW_VHANGUP)
					return (EBADF);
				GET_OPEN_SEMA;
				if (tp->t_state & TTYD_NDELAY)
				    return(EBUSY);
				
			}
			else if ((dev & (DEV_TTY | DEV_CUL)) == 0) {
				RELEASE_OPEN_SEMA;
				if (sleep (&tp->t_slcnl, TTIPRI|PCATCH))
					return (EINTR);
				if (HW_VHANGUP)
					return (EBADF);
				GET_OPEN_SEMA;
			}
			if (err)
			    return (err);
				
				
			tty_re_init(tp);
			goto check_carrier;
		}
	}
	RELEASE_OPEN_SEMA;
	return(0);	/* successful open */
}

/**********************************************************************
 * tty_CCITT_open (dev, tp, flag)
 *
 *  This is the open routine used each time a serial card is opened
 *  for use in CCITT mode.
 *
 *********************************************************************/

tty_CCITT_open (dev, tp, flag)

   register dev_t dev;
   register struct tty *tp;
   int flag;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		tty_CCITT_open
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 Apr 92	dmurray	Added macros GET_OPEN_SEMA and RELEASE_OPEN_SEMA to
 |			perform semaphoring for ttycomn opens and closes.
 |			The change was prompted by a hotsite and was a
 |			fix to keep an open from happening before the close
 |			was able to fully complete.  The change lets a close
 |			keep the semaphore until it has had a chance to
 |			fully complete.  Opens however release the semaphore
 |			before sleeping and must re-acquire it upon awakening.
 |			tty_CCITT_open() keeps the semaphore when returning
 |			to ttycomn_open(), and ttycomn_open() releases it
 |			before returning.
 |			NOTE: This may need to be changed if tty_simple_open()
 |			      is called by another routine.
 |
 | 28 Apr 92	jlau	Fix for Defect Number INDaa09938.
 |			Added check on FNBLOCK|FNDELAY flags before going
 |			to sleep. The change was made to fix problem where
 |			opening in non-block mode of a tty port will hang.
 |			Notice that the check must be done after the
 |			open count (i.e. cul_count, tty_count, or ttyd_count)
 |			has been incremented and before the sleep() is 
 |			called.  This is due to the fact that ttycomn_open()
 |			always assumes that the open count has always been
 |			incremented after tty_XXXX_open() returns.
 |
 |			NOTE: This is only a "partial" fix to this defect
 |			in that only the "hanging" has been eliminated. 
 |			However, the final fix should be smart enough to
 |			detect the case in which CTS and DSR signals are
 |			not left high from a previous connection.
 |
 | 11 May 92	marcel	Added int err, and intialized it to 0, to go with
 |			changed GET_OPEN_SEMA macro.  Also, PCATCHed all
 |			sleeps.
 |
 | 22 May 92	dmurray	Added vhangup checks to sleeps.
 |
 --------------------------------------------------------------------------*/

	unsigned char	wait;
	int		err	= 0;

	/*
	 * Keep track of who's waiting to complete an open.
	 */
wait_for_ring:
	wait = (((tp->t_state & (TTY | CUL | TTYD)) == 0) /* first open */
	     && (tp->t_cflag & HUPCL)
	     &&	(tp->t_smcnl & (MCTS | MDSR)));		  /* modem still up */
	if (dev & DEV_CUL) {
		tp->t_state |= (CUL | CCITT);
		tp->cul_count++;
		if (wait) {
			/* 
 	 		 * Check FNDELAY|FNBLOCK flags before sleeping.
	 		 */
			if (flag & FNBLOCK)
				return EAGAIN;
			else if (flag & FNDELAY) 
				return EBUSY;
			
			RELEASE_OPEN_SEMA;
			if (sleep(&tp->timers[MTHANGUP], TTIPRI|PCATCH))
				return (EINTR);
			if (HW_VHANGUP)
				return (EBADF);
			GET_OPEN_SEMA;
			if (err)
				return (err);

			
		}
		if (tp->t_state & TTY) {
			RELEASE_OPEN_SEMA;
			if (sleep (&tp->t_canq, TTIPRI|PCATCH))
				return (EINTR);
			if (HW_VHANGUP)
				return (EBADF);
			GET_OPEN_SEMA;
			if (err)
				return (err);
		}
	}
	else if (dev & DEV_TTY) {
		tp->t_state |= (TTY | CCITT);
		tp->tty_count++;
		if (wait) {
			/* 
 	 		 * Check FNDELAY|FNBLOCK flags before sleeping.
	 		 */
			if (flag & FNBLOCK)
				return EAGAIN;
			else if (flag & FNDELAY) 
				return EBUSY;	
			RELEASE_OPEN_SEMA;
			if (sleep(&tp->timers[MTHANGUP], TTIPRI|PCATCH))
				return (EINTR);
			if (HW_VHANGUP)
				return (EBADF);
			GET_OPEN_SEMA;
			if (err)
				return (err);
		}
	}
	else {
		tp->t_state |= (TTYD | CCITT);
		tp->ttyd_count++;
		if (wait) {
			/* 
 	 		 * Check FNDELAY|FNBLOCK flags before sleeping.
	 		 */
			if (flag & FNBLOCK)
				return EAGAIN;
			else if (flag & FNDELAY) 
				return EBUSY;	
			RELEASE_OPEN_SEMA;
			if (sleep(&tp->timers[MTHANGUP], TTIPRI|PCATCH))
				return (EINTR);
			if (HW_VHANGUP)
				return (EBADF);
			GET_OPEN_SEMA;
		}
		if (err)
			return (err);

		/* See if we must wait for an incoming call */
		if (flag & (FNDELAY|FNBLOCK)) {
		   if ((tp->t_cflag & CLOCAL) ||
				 (tty_modem_lines(tp) == CCITT_MSL))
		   	tp->t_state |= CARR_ON;
		   else
		   	tp->t_state &= ~CARR_ON;
		   goto no_delay_open;
		}
		else if ((tp->t_state & (ISOPEN | TTYD_PENDING)) &&
			((tp->t_state & CARR_ON) || (tp->t_cflag & CLOCAL)) &&
			!(tp->t_state & (CUL | TTY)))
				goto check_carrier;
		else {
			if (tty_modem_lines(tp) == CCITT_MSL)
				goto check_carrier;
			tp->t_state |= (WAIT_FOR_RING | WOPEN);
			RELEASE_OPEN_SEMA;
			if (sleep (&tp->t_int_lvl, TTIPRI|PCATCH))
				return (EINTR);
			if (HW_VHANGUP)
				return (EBADF);
			GET_OPEN_SEMA;
			if (err)
				return (err);
			tp->t_state &= ~WAIT_FOR_RING;
			tp->t_state |= TTYD_PENDING;
			wakeup (&tp->t_canq);
		}
	}

check_carrier:
	/*
	 * Activate the MCL lines, and look at the current MSL states.
	 */
	if ((dev & DEV_TTY) || (tp->t_cflag & CLOCAL) ||
	   (ttymodem(tp, ON) == CCITT_MSL)) {
		tp->t_state |= CARR_ON;
		if (dev & DEV_CUL)
			wakeup (&tp->t_canq);
		else if ((dev & (DEV_TTY | DEV_CUL)) == 0) {
			wakeup (&tp->t_slcnl);
			wakeup (&tp->t_int_lvl);
		}
	}
	else
		tp->t_state &= ~CARR_ON;

	/*
	 * If the serial card is being opened for non-blocking 
	 * operation  then
	 * don't wait for the MSL lines to go active - complete
	 * the open IMMEDIATELY! (or return an error, if necessary).
	 * If we are open for blocking mode, or if a higher priority
	 * open has completed earlier, then put the calling process
	 * to sleep, until the MCL lines go active, or until the
	 * higher priority open closes.
	 */

no_delay_open:
	if (flag & (FNDELAY|FNBLOCK)) {
	   /* Complete the open */
	   if ((dev & (DEV_TTY | DEV_CUL)) == 0) {
	      /* Force any pending CUL opens to return EBUSY */
	      tp->t_state |= (TTYD_NDELAY | WAIT_FOR_RING);
	      wakeup (&tp->t_canq);
	   }
	}
	else {
		/* Wait for the device to become ready */
		if (!(tp->t_state & CARR_ON) ||
                   ((dev & DEV_CUL) && (tp->t_state & TTY))) {
			tp->t_state |= WOPEN;
			/* Start a connect timeout */
			tp->t_state &= ~OPEN_TIMED_OUT;
			if (!(tp->t_state & OPEN_TO_PENDING) &&
			   (tp->timers[MTCONNECT])) {
				tp->t_state |= OPEN_TO_PENDING;
				timeout(tty_open_TO, tp,
					tp->timers[MTCONNECT] * HZ);
			}
			/* Start sleeping */
			if (dev & DEV_CUL) {
/* if RI received within last 10 seconds, return EBUSY (call collision) */
				if (lbolt  < tp->t_time + (HZ*10))
					return (EBUSY);
				RELEASE_OPEN_SEMA;
				if (sleep (&tp->t_canq, TTIPRI|PCATCH))
					return (EINTR);
				if (HW_VHANGUP)
					return (EBADF);
				GET_OPEN_SEMA;
				if (err)
				  return (err);
				if (!(tp->t_state & OPEN_TIMED_OUT)) {
					tp->t_state &= ~OPEN_TO_PENDING;
					untimeout(tty_open_TO, tp);
				}

				if (tp->t_state & (TTYD_NDELAY | TTYD_PENDING))
					return (EBUSY);
				else if (tp->t_state & OPEN_TIMED_OUT) {
					ttymodem(tp, OFF);
					return (EIO);
				}
			}
			else if ((dev & (DEV_TTY | DEV_CUL)) == 0) {
				wakeup (&tp->t_canq);
				RELEASE_OPEN_SEMA;
				if (sleep (&tp->t_slcnl, TTIPRI|PCATCH))
					return (EINTR);
				if (HW_VHANGUP)
					return (EBADF);
				GET_OPEN_SEMA;
				if (err)
					return (err);

				if (tp->t_state & OPEN_TIMED_OUT) {
					ttymodem(tp, OFF);
					tp->t_state &= ~TTYD_PENDING;
					goto wait_for_ring;
				}
				else {
					tp->t_state &= ~OPEN_TO_PENDING;
					untimeout(tty_open_TO, tp);
				}
			}
			tty_re_init(tp);
			goto check_carrier;
		}
	}

	/* Complete the open */
complete_open:
	tp->t_state &= ~TTYD_PENDING;
	if (tp->t_open_type != TTY_OPEN)
		tty_na_TO(tp);			/* start NA timer */
	return(0);	/* successful open */
}

ttycomn_close(dev, tp)

register dev_t			dev;
register struct tty	*	tp;

{
/*---------------------------------------------------------------------------
 | FUNCTION -	Called to close the serial device.
 |
 | PARAMETERS PASSED -
 |	dev	Describes which serial card is to be closed.
 |	tp	Pointer to tty structure.
 |
 | ASSUMPTION - 	The tp->utility pointer has already been intialized 
 |			by the serial drivers to point to the successful 
 |			open count stored somewhere in the driver-dependent 
 |			data structure.
 |
 | MODIFICATION HISTORY
 |
 |
 | Date		User	Changes
 |  1 May 89	garth	Removed reinitialization of t_pgrp and t_cproc.
 |			They get reset in the line discipline (ttclose).
 |			
 | 21 Apr 92	dmurray	Added macros GET_OPEN_SEMA and RELEASE_OPEN_SEMA to
 |			perform semaphoring for ttycomn opens and closes.
 |			The change was prompted by a hotsite and was a
 |			fix to keep an open from happening before the close
 |			was able to fully complete.  The change lets a close
 |			keep the semaphore until it has had a chance to
 |			fully complete.  Opens however release the semaphore
 |			before sleeping and must re-acquire it upon awakening.
 |
 | 11 May 92	marcel	Added int err, initalized to 0, to go with modified
 | 			GET_OPEN_SEMA macro.
 | 
 | 15 May 92	jlau	Added code to check the newly-added successful open
 |			count for last close.  Removed code that check the 
 |			individual cul_count, tty_count and tty_count which 
 |			are now used as pending	open counts.
 |
 |			Also, a subset of the ttycomn_close() code has been
 |			pulled out and and put into a new ttycomn_cleanup() 
 |			routine which will be called if the open_count 
 |			reaches zero after decremented.
 |
 --------------------------------------------------------------------------*/

	register int		s;
	register int		err;
	register u_int	*	open_count	= (u_int *)tp->utility;

	s = splx (tp->t_int_lvl);

	GET_OPEN_SEMA;  /* this is the only place we won't check for */
			/* failure, because we can't do anything about it*/

	/* Close down the port if this is the last close */
	if (--*open_count == 0) 
		ttycomn_cleanup(dev, tp);

	RELEASE_OPEN_SEMA;
	splx (s);
	return(0);
}

ttycomn_cleanup(dev, tp)

   register dev_t  dev;
   register struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Perform all the necessary chores required to clean up
 |			the line. Called by ttycomn_close() on last close or
 |			by ttycomn_open() when EINTR out of a sleep.
 |
 | ASSUMPTION -		The open semaphore is held by the caller via the   
 |			GET_OPEN_SEMA macro.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 15 May 92  jlau	Routine created.
 |			It was originally part of the ttycomn_close() routine.
 |			The restructure was done in an attempt to make the
 |			S300 code more in line with the S800 code in the
 |			open/close handling.
 --------------------------------------------------------------------------*/

	if (dev & DEV_CUL) {
		/* last close of this type, Did this have it open? */
		if ((tp->t_open_type & CUL_OPEN) || (tp->t_open_type == 0)) {

			/* Call the generic TTY close routine */
			(*linesw[tp->t_line].l_close)(tp);

			tp->t_open_type = 0;
			/* wake up any pending requests */
			if (tp->t_cflag & HUPCL) {
				ttymodem(tp, OFF);
				/* Start the disconnect timer here */
				hangup_delay(tp);
			}
			if (tp->t_state & TTYD)
				wakeup (&tp->t_slcnl);
		}
		tp->t_state &= ~CUL;
	}
	else if (dev & DEV_TTY) {
		/* last close of this type, Did this have it open? */
		if ((tp->t_open_type & TTY_OPEN) || (tp->t_open_type == 0)) {

			/* Call the generic TTY close routine */
			(*linesw[tp->t_line].l_close)(tp);

			tp->t_open_type = 0;
			/* wake up any pending requests */
			if (tp->t_state & CUL)
	   			wakeup (&tp->t_canq);
			if (tp->t_state & TTYD)
				wakeup (&tp->t_slcnl);
		}
		tp->t_state &= ~TTY;
	}
	else {	/* TTYD type */
		/* last close of this type, Did this have it open? */
		if ((tp->t_open_type & TTYD_OPEN) || (tp->t_open_type == 0)) {

			/* Call the generic TTY close routine */
			(*linesw[tp->t_line].l_close)(tp);

			tp->t_open_type = 0;
			/* wake up any pending requests */
			if (tp->t_state & CUL)
	   			wakeup (&tp->t_canq);
			if (tp->t_cflag & HUPCL) {
				ttymodem(tp, OFF);
				/* Start the disconnect timer here */
				hangup_delay(tp);
			}
			tp->t_state &= ~(TTYD | TTYD_NDELAY | TTYD_PENDING);
		}
	}

	/* only close if no one else is alive */
	if ((tp->t_state & (TTY | CUL | TTYD)) == 0) {
		/* No one is waiting */
		/* turn off the card as much as possible */
		if (tp->t_state & OPEN_TO_PENDING)
			untimeout(tty_open_TO, tp);
		if (tp->t_state & CARRIER_TO_PENDING)
			untimeout(tty_CD_TO, tp);
		if (tp->t_state & CCITT)
			untimeout(tty_na_TO, tp);

		tp->t_state &= ~( SIMPLE | CCITT |
					OPEN_TO_PENDING | CARRIER_TO_PENDING);
		/*
		 *	Fix defect #SWFfc01066. If the line is not reset,
		 *	and the current line setting is bad, this tty
		 *	struct is hosed until a reboot.
		 */
                tp->t_line      = 0;
	}
}

ttycomn_control( tp, cmd, data )

struct tty	*	tp;
int			cmd;
int			data;

{
/*---------------------------------------------------------------------------
 | FUNCTION -		Do various tasks needed by the drivers concerning
 |			things which happen in this module.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 |
 --------------------------------------------------------------------------*/

	int	*	hw_p	= tp->utility;

	switch( cmd ) {
	case T_VHANGUP:
		/*
		 * One int offset to the driver hardware data structure is
		 * the vhangup flag.
		 */
		if (data == 0)
			break;
		wakeup( (caddr_t)&OPEN_SEMA );
		wakeup( (caddr_t)&tp->t_canq );
		wakeup( (caddr_t)&tp->t_slcnl );
		wakeup( (caddr_t)&tp->timers[MTHANGUP] );
		wakeup( (caddr_t)&tp->t_int_lvl );
		break;
	}
	return(0);
}
