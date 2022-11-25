/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/pdi.c,v $
 * $Revision: 1.5.84.4 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/05 10:03:31 $
 */
/* HPUX_ID: @(#)pdi.c	55.1		88/12/23 */

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

/*
 hp 98628 driver modified from the 98626 driver 
 */

#include "../h/param.h"		/* various types and macros */
#include "../h/file.h"
#include "../h/tty.h"		/* tty structure and parameters */
#include "../h/conf.h"		/* linesw stuff */
#include "../wsio/intrpt.h"
#include "../wsio/hpibio.h"
#include "../s200io/pdi.h"
#define _INCLUDE_TERMIO
#include "../h/termios.h"
#include "../h/modem.h"
#include "../h/signal.h"
#include "../h/errno.h"
#include "../h/ld_dvr.h"

/* Modem Control masks */
#define	PDI_RTS		0x01
#define	PDI_DTR		0x02
#define	PDI_DRS		0x04

/* Modem Status masks */
#define	PDI_DSR		0x01
#define	PDI_DCD		0x02
#define	PDI_CTS		0x04
#define	PDI_RI		0x08
#define PDI_CCITT_IN	PDI_DSR | PDI_DCD | PDI_CTS | PDI_RI

#define CCITT_IN	(MDCD | MCTS | MDSR | MRI)
#define CCITT_OUT	(MDTR | MRTS | MDRS)

int pdi_errors = 0;	/* number of 98628 card crashes */

/* Temporary until it can be defined in tty.h */
#define	T_VHANGUP	-1

#define signal gsignal
#define PDISIZE sizeof(struct pdi_desc_type)
#define PDIHWSIZE sizeof(struct pdi_hw_data)
#define pdi_tp  (struct tty *)(isc_table[m_selcode(dev)]->ppoll_f)

extern struct tty *cons_tty;
extern dev_t cons_dev;
extern char remote_console;
extern short baudmap[];
int	pdiproc();
int	ttrstrt();
int	pdixmit();
int	pdi_intr();
int	pdi_intr_0();

/* table that maps unix symbolic baud values to uart baud values */

short pdibaudmap[] = {	/* For reading unix config */
		0x0000,	/* hangup */
		0x0001,	/* 50 baud */
		0x0002,	/* 75 baud */
		0x0003,	/* 110 baud */
		0x0004,	/* 134.5 baud */
		0x0005,	/* 150 baud */
		0x0006,	/* 200 baud */
		0x0007,	/* 300 baud */
		0x0008,	/* 600 baud */
		0x0008,	/* 900 baud ???????????????????????? */
		0x0009,	/* 1200 baud */
		0x000a,	/* 1800 baud */
		0x000b,	/* 2400 baud */
		0x000c, /* 3600 baud */
		0x000d,	/* 4800 baud */
		0x000d,	/* 7200 baud */
		0x000e, /* 9600 baud */
		0x000f,	/* 19200 baud */
		0x000f,	/* 38400 baud */
};

ushort pdibaudmap2[] = {	/* For reading switches */
		0,	/* for external */
		B50,	/* 50 baud */
		B75,	/* 75 baud */
		B110,	/* 110 baud */
		B134,	/* 134.5 baud */
		B150,	/* 150 baud */
		B200,	/* 200 baud */
		B300,	/* 300 baud */
		B600,	/* 600 baud */
		B1200,	/* 1200 baud */
		B1800,	/* 1800 baud */
		B2400,	/* 2400 baud */
		B3600,	/* 3600 baud */
		B4800,	/* 4800 baud */
		B9600,  /* 9600 baud */
		B19200,	/* 19200 baud */
};

pdi_open(dev, flag)

   dev_t  dev;
   int   flag;

{
/*---------------------------------------------------------------------------
 | FUNCTION -		Perform open processing.
 |
 | PARAMETERS -
 |	dev	Describes which serial card will be opened.
 |	flag	Specifies 'open' access modes.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 May 92	dmurray	Added call to clear vhangup on new open.
 |
 --------------------------------------------------------------------------*/

	/* Clear the vhangup flag. */
	pdiproc( pdi_tp, T_VHANGUP, 0 );
	return(ttycomn_open(dev, pdi_tp, flag));
}

/****************************************************************************
 * pdi_close (dev)
 *
 *   dev_t dev - Describes which serial card is to be closed.
 *
 *   pdi_close is called when the last close command has been issues for
 *   a specific serial card.  This procedure calls the generic TTY close
 *   routine.
 ****************************************************************************/

pdi_close(dev)

   dev_t  dev;
{
	return(ttycomn_close(dev, pdi_tp));
}

/****************************************************************************
 * pdi_read (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be read from.
 *
 *   pdi_read is called each time a read request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY read routine.
 ****************************************************************************/

pdi_read(dev, uio)

{
	/* Call the generic TTY read routine */
	ttread (pdi_tp, uio);
}

/****************************************************************************
 * pdi_write (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be written to.
 *
 *   pdi_write is called each time a write request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY write routine.
 ****************************************************************************/

pdi_write(dev, uio)

   dev_t  dev;
{
	/* Call the generic TTY write routine */
	ttwrite (pdi_tp, uio);
}

/****************************************************************************
* pdi_ioctl (dev, cmd, arg, mode)
*
*   dev_t dev - Describes which serial card is to be affected.
*   int  cmd - Specifies the ioctl command being requested.
*   char *arg - Points to the passed-in parameter block.
*   int  mode - Specifies the current access modes.
*
*   pdi_ioctl is called each time an ioctl request is made for a
*   specific serial card.  This entry point is used to perform
*   TTY specific functions.  If one of the special SERIAL commands
*   is given, then it will be handled here.  If one of the standard
*   TTY commands is given, then the generic TTY ioctl routine will
*   be called upon to handle it.
****************************************************************************/

pdi_ioctl(dev, cmd, arg, mode)

   dev_t  dev;
   int   cmd;
   int   *arg;
   int   mode;

{
   register struct tty *tp;                  /* Ptr to tty structure */
   register int s;                           /* Current intr level */
   register err = 0;

   tp = pdi_tp;	/* get the tty pointer */
   /*
    * Check the requested ioctl command, and if it is one of the
    * new TTY commands, then process it here; else, call the
    * generic TTY ioctl routine.
    */
   switch(cmd) {

      case MCGETA:
	 /* Return the current state of the modem lines */
	 pdiproc(tp, T_MODEM_STAT);
	 *arg = tp->t_smcnl;
	 return(err);

      case MCSETAW:
      case MCSETAF:
	 /* Wait for output to drain */
	 err = ttywait (tp);
	 if (err != 0)
	    return (err) ;
	 if (cmd == MCSETAF)
	    ttyflush (tp, (FREAD | FWRITE));

      case MCSETA:
	 /* Set the modem control lines */
	 s = splx (tp->t_int_lvl);
	 if (tp->t_state & SIMPLE) {
		tp->t_smcnl &= ~(CCITT_OUT); /* clear shadow control bits */
		tp->t_smcnl |= (*arg & CCITT_OUT); /* set to bits in arg */
		pdiproc(tp, T_MODEM_CNTL);	/* set control lines */
	 }
	 splx (s);
	 return(err);

      case MCGETT:
	 /* Return the current timer settings */
	 for(s=0; s < NMTIMER; s++)
	 	((unsigned short *)arg)[s] = tp->timers[s];
	 return(err);

      case MCSETT:
	 /* Set the timers */
	 for(s=0; s < NMTIMER; s++)
	 	tp->timers[s] = ((unsigned short *)arg)[s];
	 return(err);

      case TCSETA:
      case TCSETAW:
      case TCSETAF:
      case TCSETATTR:
      case TCSETATTRD:
      case TCSETATTRF:
	if ((err=ttiocom(tp, cmd, arg, mode)) == -1) {
		err = 0;
		pdiparam(tp);
	}
	return(err);

      default:
	 /*
	  * Pass the ioctl command on through to the generic TTY ioctl
	  * handler.  If a non-zero value is returned, then call pdiparam
	  * to force the configuration to be updated to the newly
	  * specified values.
	  */
	if ((err=ttiocom(tp, cmd, arg, mode)) == -1) {
		err = 0;
	    	pdiparam(tp);
	}
	return(err);
   }
}

/* this is to get the tty pointer then call ttselect */
pdi_select(dev, rw)
dev_t dev;
{
	ttselect(pdi_tp, rw);
}

pdiparam(tp)
register struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Setup serial port according to ioctl args. 
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 26 Feb 90  garth	Added code to ignore baud rate changes that are
 |			not supported.  This behavior is currently required
 |			by POSIX.  
 |
 | 19 Sep 90  jph	Fix for DTS INDaa08284. Added code to ignore baud 
 |			rate B0 when set on a non-modem port. 
 --------------------------------------------------------------------------*/
	register ushort flags;
	register int x, status, temp;

	flags = tp->t_cflag;
	if (((flags & CBAUD) == 0) && (tp->t_open_type != TTY_OPEN)) {
		/* hang up line */
		ttymodem(tp, OFF);
		tp->t_state &= ~CARR_ON;
	}

	x = splx(tp->t_int_lvl);
	if (flags & CREAD)
		tp->t_sicnl |= RXINTEN;
	else
		tp->t_sicnl &= ~RXINTEN;
	pdi_control( PASS_PDI, INTR_COND_REG, tp->t_sicnl);

	tty_delta_CLOCAL(tp);

	temp = flags & CBAUD;
	/* Eliminate unsupported speeds */
	if ((temp != B900) && (temp != B0) && (temp != B7200) && (temp <= B19200)) {
		/* set both transmit and recieve baud registers */
		pdi_control( PASS_PDI, BAUD_REG, pdibaudmap[flags & CBAUD] );
		pdi_control( PASS_PDI, BAUD_REG+1, pdibaudmap[flags & CBAUD] );
	} else {
		/* Make sure HW and SW settings match
		 * Reset flags with correct control settings.
		 */
		tp->t_cflag &= ~CBAUD;
		tp->t_cflag |= pdibaudmap2[pdi_status( PASS_PDI, BAUD_REG)];
		flags = tp->t_cflag;
	}
	pdi_control( PASS_PDI, CHAR_SIZE_REG, (flags&CSIZE)/CS6 );
	
	if (flags & PARENB) {
		if (flags & PARODD) 
			pdi_control( PASS_PDI, PARITY_REG, PARITY_ODD);
		else
			pdi_control( PASS_PDI, PARITY_REG, PARITY_EVEN);
	}
	else
		pdi_control( PASS_PDI, PARITY_REG, PARITY_NONE);

	if (tp->t_iflag & IXON) {
		/* turn d1/d3 on as host if not on yet */
		if (pdi_status( PASS_PDI, HANDSHAKE_REG) != 5)
			pdi_control( PASS_PDI, HANDSHAKE_REG, 5);
	/*	if (pdi_status( PASS_PDI, HANDSHAKE_REG) != D1D3)
			pdi_control( PASS_PDI, HANDSHAKE_REG, D1D3); */
	} else
		/* turn d1/d3 off */
		pdi_control( PASS_PDI, HANDSHAKE_REG, 0);

	if (flags & CSTOPB)
	      status = pdi_control( PASS_PDI, STOP_BIT_REG, TWO_STOP_BITS);
	else
	      status = pdi_control( PASS_PDI, STOP_BIT_REG, ONE_STOP_BIT);
	splx(x);
	if (status < 0)
		pdidown(tp);

	/* The 98628 sends <BREAK> for a specified number of character
	 * times (kinda dumb, huh).  This next bit computes the number of
	 * characters needed to make the time work out to approx 1/4 sec,
	 * as per rs232 convention.  Unsupported baud rates have already
	 * been eliminated.
	 */
	x = 153600 / (baudmap[flags & CBAUD] * tty_csize(tp) * 4); 
	x++;					/* adjust for truncation */
 	pdi_control( PASS_PDI, 39, x);		/* set break time */
}

/****************************************************************************
 pdi_intr -	interrupt service routine
 ****************************************************************************/

pdi_intr(inf)
struct interrupt *inf;
{
	register struct tty *tp = (struct tty *) inf->temp;
	register unsigned char status;

	tp->modem_status = 0;	/* for MTNOACTIVITY timeout */
	/* read interrupt status */
	status = *((unsigned char *)(tp->t_card_ptr + 0x4001));
	if (( ! (tp->t_state & (ISOPEN|WOPEN))) && (tp != cons_tty)) {
	/* assume modem interrupt.  Otherwise, tty_modem_intr() is a no-op */
		tty_modem_intr(tp);
		/* keep the dump card clear */
		if (pdi_control( PASS_PDI, CLEAR_REG, 0) < 0)
			pdidown(tp);
		return;
	}

	if (status & TXINTEN) {
		tp->t_sicnl &= ~TXINTEN;	/* we got the intr */
		if (pdi_control(PASS_PDI, INTR_COND_REG, tp->t_sicnl) < 0)
			pdidown(tp);
	}

	/* must see if the modem status has changed */
	if (status & MODEM_STATUS) {
		tty_modem_intr(tp);
		return;
	}

	if (! (tp->t_state & CARR_ON))
		/* if not up, don't process data */
		return;

	/* Save status in the queue */
	/* Extra data causes earlier data to be lost when buffer
	   overflows (this was pdi related but could happen to us) */
	if (tp->t_in_count >= TTYBUF_SIZE) {
		if (++tp->t_in_head >= tp->t_inbuf+TTYBUF_SIZE)
			tp->t_in_head = tp->t_inbuf;
	} else {
		tp->t_in_count++;
	}
	*tp->t_in_tail++ = status;
	if (tp->t_in_tail >= tp->t_inbuf+TTYBUF_SIZE)
		tp->t_in_tail = tp->t_inbuf;

	/* if data available then do this at lower level */
	if ( ! (tp->t_rcv_flag & TRUE)) {
		tp->t_rcv_flag |= TRUE;
		sw_trigger(&tp->rcv_intloc,pdi_intr_0,tp,0,tp->t_int_lvl);
	}
}

pdi_intr_0(tp)
register struct tty *tp;
{
	register ushort status;
	register int transerr, out;
	register int x;
	ushort c, report;

	/* may want to clear this when we are ready to leave */
	/* this way the interrupt routine will know that we are in here */

	transerr = 0;
	while (1) {
		/* Fetch Status that caused the interrupt */
		x = splx(tp->t_int_lvl);
		if (tp->t_in_count == 0) {
			tp->t_rcv_flag &= ~TRUE;	/* now clear flag */
			splx(x);
			return;
		}
		tp->t_in_count--;
		status = *tp->t_in_head++;
		if (tp->t_in_head >= tp->t_inbuf+TTYBUF_SIZE)
			tp->t_in_head = tp->t_inbuf;
		splx(x);

		if (status & 0x01) {
			/* reset the card because it lost its mind */
			*((char *)(tp->t_card_ptr + 0x0001)) = 0x80;
			snooze(200000);	/* wait for 200ms to settle card */
			/* clear out the error reporting byte */
			*((char *)(tp->t_card_ptr + 0x400d)) = 0x00;
			pdi_setup(tp);	/* re-initialize the card */
			pdiparam(tp, 0);	/* set to current states */
			pdi_errors++;		/* keep track of errors */
			if (tp->t_state & CARR_ON) {
				if ((tp->t_state & WOPEN) == 0) {
					signal (tp->t_pgrp, SIGHUP);
					ttymodem(tp, OFF);
				}
				tp->t_state &= ~CARR_ON;
				ttyflush (tp, (FREAD | FWRITE));
			}
		}

		if (status & DATA_AVAILABLE) {
			/* received data available */
			while (1) {
				c = 0;
				out = pdi_enter(PASS_PDI, ((int)&c)+1,
						 1, &report);
				/* see if this is the byte with the error */
				if (transerr) {
					transerr = 0;
					c |= PERROR;
				}
				/* See how many more are left to do */
				x = pdi_inqueue(PASS_PDI);
				if (x == -1)
					x = 0;
				if (out && (tp->t_cflag & CREAD))/* send byte */
					(*linesw[tp->t_line].l_input)(tp, c, 0, x);
				switch (report & REPORT_MASK) {
	 			case BREAK:
					/* break recieved */
					c = FRERROR;
					if (tp->t_cflag & CREAD) /* send byte */
						(*linesw[tp->t_line].l_input)(tp, c, 0, x);
					break;
				case TRANS_ERR:	
					/* framing and/or parity error on the
					** byte that we haven't got yet.
					** (don't ask why) */
					transerr = 1;
					break;
				default:
					/* report was 0 to get here
					** but we must get the byte with
					** with the error also */
					if ((out) || (transerr))
						/* loop again */
						break;
					else
						/* no character, leave */
						goto exit;
				}
			}
	exit: ;
		}
		if (status & WRITE_AVAILABLE) {
			pdixmit(tp);	/* transfer the bytes */
		}
	}
}

/****************************************************************************
 pdisend -	
 ****************************************************************************/

int
pdisend(tp)
struct tty *tp;
{
	char data;

	if (tp->t_state & (TTXON | TTXOFF)) {
		if (tp->t_state & TTXON) {
			data = CSTART;
			tp->t_state &= ~TTXON;
		}
		else {
			data = CSTOP;
			tp->t_state &= ~TTXOFF;
		}
		if (pdi_out(tp, data))
			return 0;	/* yes it got out */

		if (data == CSTART)	/* restore states */
			tp->t_state |= TTXON;
		else
			tp->t_state |= TTXOFF;
		return 0;
	}
	if (tp->t_state & BUSY) {
		if (pdi_out(tp, (char)tp->t_buf)) {
			tp->t_state &= ~BUSY;
			return 1;
		}
		else
			return 0;
	}
	return 0;
}

int
pdi_out(tp, byte)
register struct tty *tp;
unsigned char byte;
{
	register int x;

	if (pdi_output( PASS_PDI, &byte, 1)) {	/* there is room */
		return 1;	/* yes we got the byte out */
	}
	else {
		x = splx(tp->t_int_lvl);
		tp->t_sicnl |= TXINTEN;	    /* set write available interrupt */
		if (pdi_control( PASS_PDI, INTR_COND_REG, tp->t_sicnl) < 0)
			pdidown(tp);
		if (pdi_output( PASS_PDI, &byte, 1)) {
			/* byte got out, clear write available interrupt */
			tp->t_sicnl &= ~TXINTEN;
			if (pdi_control( PASS_PDI, INTR_COND_REG,
				tp->t_sicnl) < 0)
				pdidown(tp);
			splx(x);
			return 1;
		}
		tp->t_int_flag |= PEND_TXINT;	/* we have to wait */
		splx(x);
		return 0;	/* no the byte will be sent on interrupt */
	}
}

/****************************************************************************
 pdixmit -	Transmitter interrupt, most of the work is done by pdisend.
 ****************************************************************************/

pdixmit(tp)
register struct tty *tp;
{
	tp->t_int_flag &= ~PEND_TXINT;	/* tell tty - not waiting */

	if (pdisend(tp) == 0)
		return;
	pdiproc(tp, T_OUTPUT);
}

pdiproc(tp, cmd, data)

register struct tty	*	tp;
int				cmd;
int				data;

{
/*---------------------------------------------------------------------------
 | FUNCTION -		Perform various functions.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 May 92	dmurray	Added T_VHANGUP.
 |
 --------------------------------------------------------------------------*/

	register c;
	register int x;

	switch(cmd) {
	case T_TIME:
		tp->t_slcnl &= ~SETBRK;
		tp->t_state &= ~TIMEOUT;
		goto start;

	case T_WFLUSH:
		/* clear card buffers and any pending interrupt */
		x = splx(tp->t_int_lvl);
		if (pdi_control( PASS_PDI, CLEAR_REG, 0) < 0)
			pdidown(tp);
		if (tp->t_int_flag & PEND_TXINT) {
			/* tell tty - not waiting */
			tp->t_int_flag &= ~PEND_TXINT;
			tp->t_sicnl &= ~TXINTEN;	/* we got the intr */
			if (pdi_control(PASS_PDI, INTR_COND_REG,
				tp->t_sicnl) < 0)
				pdidown(tp);
			/* this will hopefully clear the BUSY flag */
			splx(x);
			pdisend(tp);
		}
		splx(x);

	case T_RESUME:
		x = splx(tp->t_int_lvl);
		/* this will tell the card to start transmitting */
		/* see T_SUSPEND for explanation */
		*((unsigned char *)(tp->t_card_ptr + 0x4079)) = 0;
		if (tp->t_int_flag&PEND_TXINT || tp->t_state&BUSY) {
			/* this will hopefully clear the BUSY flag */
			if (pdisend(tp)) {
				/* tell tty - not waiting */
				tp->t_int_flag &= ~PEND_TXINT;
				tp->t_sicnl &= ~TXINTEN;	/* we got the intr */
				if (pdi_control(PASS_PDI, INTR_COND_REG,
					tp->t_sicnl) < 0)
					pdidown(tp);
			}
		}
		splx(x);

	case T_OUTPUT:
	start:
		if (tp->t_state&(TIMEOUT|TTSTOP|BUSY))
			break;
		while ((c=getc(&tp->t_outq)) >= 0) {
#ifdef NO_TX_DELAYS
			if ((tp->t_oflag&OPOST) && c == 0200) {
				if ((c = getc(&tp->t_outq)) < 0)
					break;
				if (c > 0200) {
					tp->t_state |= TIMEOUT;
					timeout(ttrstrt, tp, (c&0177)+6,NULL);
					break;
				}
			}
#endif
			tp->t_buf = (struct cblock *)c; /* use buf as temp */
			tp->t_state |= BUSY;
			if (!pdisend(tp)) break;
		}
		ttoutwakeup(tp); /* check for sleepers to wakeup */
		break;

	case T_SUSPEND:
		/* this little trick will tell the 98628 to stop transmitting
		** immediately.  This is need since the outbound buffer has n
		** bytes and when the line discipline orders a stop. 
		** This is done only for cases not done by the card itself.
		** XON/XOFF is handled exclusively by the card for causing a
		** stop.
		*/
		if (!(tp->t_iflag&IXON))
			*((unsigned char *)(tp->t_card_ptr + 0x4079)) = 1;
		break;

	case T_BLOCK:
		/* turn off handshake as terminal */
		if (pdi_control( PASS_PDI, 22, 3) < 0)
			pdidown(tp);
		tp->t_state &= ~TTXON;
		tp->t_state |= TTXOFF|TBLOCK;
		x = splx(tp->t_int_lvl);
		tp->t_sicnl &= ~TXINTEN;
		if (pdi_control( PASS_PDI, INTR_COND_REG, tp->t_sicnl) < 0)
			pdidown(tp);
		splx(x);
		pdisend(tp);
		break;

	case T_RFLUSH:
		if (!(tp->t_state&TBLOCK))
			break;
	case T_UNBLOCK:
		/* turn on handshake as terminal */
		if (pdi_control( PASS_PDI, 22, 5) < 0)
			pdidown(tp);
		tp->t_state &= ~(TTXOFF|TBLOCK);
		tp->t_state |= TTXON;
		x = splx(tp->t_int_lvl);
		tp->t_sicnl |= TXINTEN;
		if (pdi_control( PASS_PDI, INTR_COND_REG, tp->t_sicnl) < 0)
			pdidown(tp);
		splx(x);
		pdisend(tp);
		break;

	case T_BREAK:
		tp->t_slcnl |= SETBRK;
		if (pdi_control( PASS_PDI, BREAK_REG, 0) < 0)
			pdidown(tp);
		tp->t_state |= TIMEOUT;
		timeout(ttrstrt, tp, HZ/4, NULL);
		break;
	case T_PARM:
		pdiparam(tp);
		break;
	case T_MODEM_CNTL:
		{
			register int smcnl = tp->t_smcnl;
			register int modem = 0;

			if (smcnl & MDTR)	modem |= PDI_DTR;
			if (smcnl & MRTS)	modem |= PDI_RTS;
			if (smcnl & MDRS)	modem |= PDI_DRS;
			if (pdi_control( PASS_PDI, MODEM_REG_8, modem) < 0)
				pdidown();
		}
		break;
	case T_MODEM_STAT:
		{
			/* read modem status */
			register int modem = pdi_status(PASS_PDI, MODEM_REG_7);
			register int smcnl = tp->t_smcnl;

			smcnl &= ~(CCITT_IN);
			if (modem & PDI_CTS)	smcnl |= MCTS;
			if (modem & PDI_DSR)	smcnl |= MDSR;
			if (modem & PDI_DCD)	smcnl |= MDCD;
			if (modem & PDI_RI )	smcnl |= MRI ;
			tp->t_smcnl = smcnl;
		}
		break;

	case T_VHANGUP:
		((struct pdi_hw_data *)(tp->utility))->vhangup	= data;
		ttcontrol( tp, LDC_VHANGUP, data );
		ttycomn_control( tp, cmd, data );
		if (data == 0)
			break;
		wakeup( (caddr_t)PASS_PDI );
		break;
	}
}

/***************************************************************************
 pdi_setup -	set up the card (done only once for each card)
 ***************************************************************************/
pdi_setup(tp)
register struct tty *tp;
{
	unsigned char status;
	char byte = ' ';

	/* set up the interrupt masks for the modem lines */
	/* set condition for which to interrupt */

	/* clear interrupt */
	status = *((unsigned char *)(tp->t_card_ptr + 0x4001));
	pdi_control( PASS_PDI, 0, 1);	/* reset the card */

	pdi_control( PASS_PDI, MODEM_INTMASK_REG, PDI_CCITT_IN);
	pdi_control( PASS_PDI, MODEM_INTCOND_REG, MODEM_INTEN);

	tp->t_sicnl = MODEM_INTEN | 0x01;
	pdi_control( PASS_PDI, INTR_COND_REG, tp->t_sicnl);

	/* want to report break and parity/framing errors in control blocks */
	pdi_control( PASS_PDI, CBLOCK_MASK_REG, REPORT_ERRORS_BREAK);

	/* clear inbound seperator */
	pdi_control( PASS_PDI, INBOUND_MASK_REG, 0);

	/* allow all characters through except handshake */
	pdi_control( PASS_PDI, PROTOCOL_REG, 62);

	/* we don't have any prompt characters */
	pdi_control( PASS_PDI, 31, 0);

	/* turn off hardware handshake */
	pdi_control( PASS_PDI, 23, 0);

	/* turn off all dumb timeouts */
	pdi_control( PASS_PDI, 16, 0);
	pdi_control( PASS_PDI, 17, 0);
	pdi_control( PASS_PDI, 18, 0);
	pdi_control( PASS_PDI, 19, 0);

	/* Kludge -- poke the card to allow it to connect */
	pdi_output( PASS_PDI, &byte, 1);
}

pdi_wait(tp)

register struct tty *tp;

{
/*---------------------------------------------------------------------------
 | FUNCTION -	This is called from ttywait in tty.c.  It returns
 |		when the 98628 output buffer is empty.
 |
 | PARAMETERS -
 |	tp	Pointer to tty structure.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 May 92	dmurray	Added check for vhangup and added PCATCH to sleep.
 |
 | 21 Jul 92	marcel	Added back in splsx (sublevel) that was left out
 |			when PCATCH was done.
 |
 --------------------------------------------------------------------------*/
	int x,y;

	y = splsx(tp->t_int_lvl);
	x = spl6();
	if (pdi_try_queue(tp)) {
		/* the queue is not empty so let's sleep */
		splsx(y);
		if (sleep( (caddr_t)PASS_PDI, (TTOPRI | PCATCH) )) {
			splx(x);
			return(EINTR);
		}
		if (((struct pdi_hw_data *)(tp->utility))->vhangup) {
			splx(x);
			return(EBADF);
		}
		delay(HZ/15);	/* wait for UART (@ 150 baud ??) to empty */
		splx(x);
	} else 	{
		  
		splsx(y);
		splx(x);
	}
	return(0);
	

}

pdi_try_queue(tp)
register struct tty *tp;
{
	if ((*((unsigned char *)(tp->t_card_ptr + 0x4227)) ==
	     *((unsigned char *)(tp->t_card_ptr + 0x422b))) &&
	    (*((unsigned char *)(tp->t_card_ptr + 0x4229)) ==
	     *((unsigned char *)(tp->t_card_ptr + 0x422d))))  {
	/* yes the buffer is empty */
		wakeup((caddr_t) PASS_PDI);
		return(0);
	}
	else /* lets try a little later (17 msecs) */
		timeout(pdi_try_queue, tp, 2, NULL);
	return(1);
}

/*************************************************************************
** Console Interface Routines:						**
**									**
** pdiputchar(c)		Print a char - used by Kernel printf.	**
** pdioutnow(c)			Print the character now.		**
** pdi_cons_init(n)		Set-up Line characteristics for console.**
*************************************************************************/

pdiputchar(c)
register ushort c;
{
	if (c == 0) return;
	if (c == '\n') pdioutnow('\r');
	pdioutnow(c);
}

pdioutnow(c)
ushort c;
{
	register struct tty *tp = cons_tty;
	register x;
	unsigned char buf = (char) c;

	/* Don't let ANYONE in until I output this character! */
	/* I don't know if I need spl around this */
	x = spl6();
	while (pdi_output( PASS_PDI, &buf, 1) == 0)
		;
	splx(x);
}

/* Set up Line characteristics for console. */
pdi_who_init(tp, tty_num, sc)
register struct tty *tp;
{
	extern char ttcchar[];
	register int x;

	if (tty_num!=-1)
		return;

/* Default values.
** Some of these defaults can be changed by 
** the user through an ioctl system call.
*/
	if (tp == NULL)	{	/* we must use port 1 */
		tp = (struct tty *)(isc_table[sc]->ppoll_f);
		cons_tty = tp;
		cons_dev = (dev_t)((sc << 16) + 0x04);
	}

	tp->t_state = CARR_ON;
	tp->t_iflag = ICRNL;
	tp->t_oflag = OPOST|ONLCR|TAB3;
	tp->t_lflag = ISIG|ICANON|ECHO|ECHOK;
	tp->t_cflag = CLOCAL|CREAD;

	/* setup default control characters */
	bcopy(ttcchar, tp->t_cc, NLDCC);

	x = splx(tp->t_int_lvl);
	tp->t_sicnl &= ~RXINTEN; /* no interrupts until open */
	if (pdi_control( PASS_PDI, INTR_REG, tp->t_sicnl) < 0)
		pdidown(tp);
	splx(x);

	/* reset the card, this will not be done */
	/* read and set the baud rate in 'tp->cflag' */
	/* read and set the parity and character size */

	/* Set Baud Rate */
	tp->t_cflag |= pdibaudmap2[pdi_status( PASS_PDI, BAUD_REG)];

	/* Set Line Characteristics */
	/* Set two CSIZE bits */
	tp->t_cflag |= (pdi_status( PASS_PDI, CHAR_SIZE_REG) << 5);
	if (pdi_status( PASS_PDI, PARITY_REG)) {	/* If Parity Enabled */
		tp->t_cflag |= PARENB;
		if (pdi_status( PASS_PDI, PARITY_REG) == PARITY_ODD)
			tp->t_cflag |= PARODD;
	} else
		tp->t_cflag &= ~PARENB;
	if (pdi_status( PASS_PDI, STOP_BIT_REG) == TWOSB) {
		tp->t_cflag |= CSTOPB;		/* If Two Stop Bits */
	}

	pdiparam(tp);
	 printf("\n    System Console is 98628 at select code %d\n",sc);
}

pdidown(tp)
register struct tty *tp;
{
	printf("98628 at sc %d failed\n",
		(((int) (tp->t_card_ptr)) >> 16) & 0x1F);
	panic("98628: card down");
}

struct tty *
pdi_pwr(isc_num,addr,id,il,cs,pci_total)
register char *addr;
int *cs;
int *pci_total;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Initialize serial driver upon power-up.  
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 15 May 92  jlau	The tp->utility will no longer point to the 
 |			pdi_desc_type structure directly.  Instead it will
 |			point to a new structure called struct pdihwdata
 |			which consists of a newly added open_count (for 
 |			keeping track of successful opens) and a pointer 
 |			to the pdi_desc_type structure.  See pdi.h for more
 |			detail on the new pdi_hw_data structure.
 |
 --------------------------------------------------------------------------*/
 
	register struct tty *tp;
	register struct isc_table_type *isc;
	extern struct tty_driver sio628_driver;
	extern int pdi_intr();

	/* Id must be 20+32. Interupt level must be 3 or less. */
	if ((id!=(20+32)) || il>3)
		return((struct tty *)0);
	if (!testr(addr+0x400D,1))	/* ram is there */
		return((struct tty *)0);
	if (*(addr+0x400D) == 0x06)
		return((struct tty *)0);	/* self test failed */
	if (*(addr+16431) != 1)
		return((struct tty *)0);	/* 1 = Async 628 */

	isc = (struct isc_table_type *)calloc(sizeof(struct isc_table_type));
	isc_table[isc_num] = isc;
	isc->my_isc = isc_num;
	isc->card_type = HP98628;
	isc->card_ptr = (int *)addr;
	isc->int_lvl = il;
	/* for indirect calls */
	isc->tty_routine = (struct tty_drivercp *)&sio628_driver;

	/* make tty structure now */
	tp = (struct tty *)calloc(sizeof(struct tty));
	(struct tty *)(isc->ppoll_f) = tp;	/* and save it */

	tp->t_proc = pdiproc;			/* for ttyflush */
	/* Set type field */
	tp->t_drvtype = &sio628_driver;
	isrlink(pdi_intr, il, 3 + (int)addr, 0xc0, 0xc0, 0, tp);

	/* Allocate space for struct pdi_hw_data and init pointer to it */
	tp->utility = (int *)calloc(PDIHWSIZE);
	/* Init open_count */
	((struct pdi_hw_data *)(tp->utility))->open_count = 0;
	/* Allocate space for struct pdi_desc_type and init pointer to it */
	((struct pdi_hw_data *)(tp->utility))->pdi_desc_ptr = 
				(struct pdi_desc_type *)calloc(PDISIZE);

	/* let pdi drivers init there variables */
	pdi_init( PASS_PDI, addr);
	pdi_setup(tp);
	/* Set card address field */
	tp->t_card_ptr = addr;

	/* Set up input queue for it */
	tp->t_in_head = tp->t_in_tail = tp->t_inbuf = 
		(ushort *)calloc(TTYBUF_SIZE*2);
	tp->t_in_count = 0;
	tp->t_int_flag = 0;
	tp->t_rcv_flag = FALSE;
	tp->t_xmt_flag = FALSE;
	tp->t_int_lvl = il;
	tp->t_pgrp = 0;
	tp->t_hw_flow_ctl = 0;
	tp->t_open_sema = 0;
	tp->t_hardware = 0;
	tp->t_cproc = NULL;
	/* Check for remote console */
	if (!remote_console) {
		/* Check for remote bit. */
		if (*(addr+1)&0x80) {
			*cs = 1;
			cons_tty = tp;
			remote_console = TRUE;
			cons_dev = (dev_t)((isc_num << 16) + 0x04);
		}
	}
	/* Increment to next location */
	(*pci_total)++;
	return(tp);
}

pdi_nop()
{
	return(0);
}

extern int (*make_entry)();
int (*sio628_saved_make_entry)();

struct tty_driver sio628_driver = {
SIO628, pdi_open, pdi_close, pdi_read, pdi_write, pdi_ioctl, pdi_select,
	pdiputchar, pdi_wait, pdi_pwr, pdi_who_init, 0
};

/*
** linking and inititialization routines
*/

sio628_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	if (id == 20+32) 
		if(data_com_type(isc) == 1) {
			if (isc->int_lvl > 3) {
				io_inform("HP98628", isc, -2, 
					" ignored; interrupt level at %d must be less than 4"
					,isc->int_lvl);
			} else {
				io_inform("HP98628 RS-232C Datacomm Interface", isc, 0);
			}
			return FALSE; /* isc entry not needed yet! */
		}
	return(*sio628_saved_make_entry)(id, isc);
}

sio628_link()
{
	extern struct tty_driver *tty_list;

	sio628_saved_make_entry = make_entry;
	make_entry = sio628_make_entry;

	sio628_driver.next = tty_list;
	tty_list = &sio628_driver;

}
