/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/apollo_pci.c,v $
 * $Revision: 1.6.84.6 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/11/16 11:17:31 $
 */

/* HPUX_ID: @(#)apollo_pci.c	55.1		88/12/23 */

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
 Apollo Utility Chip RS-232 UART driver 
 */

#include "../h/param.h"		/* various types and macros */
#include "../h/file.h"
#include "../h/tty.h"		/* tty structure and parameters */
#include "../h/conf.h"		/* linesw stuff */
#include "../wsio/intrpt.h"
#include "../wsio/hpibio.h"
#define _INCLUDE_TERMIO
#include "../h/termios.h"
#include "../h/modem.h"
#include "../h/errno.h"
#include "../h/ld_dvr.h"

#ifdef DDB
extern int ddb_boot;
#endif DDB

extern int lbolt;
extern struct tty *cons_tty;
extern dev_t cons_dev;
extern char remote_console;
char apollo_console;

int	apollo_pciproc();
int	ttrstrt();
int	apollo_pcixmit();
int	apollo_pcircv();

/* this gets the tp pointer from the isc table */
#define apollo_pci_tp  (struct tty *)(isc_table[m_selcode(dev)]->ppoll_f)

/* description of the UART */

#define PORTADR  ((struct apollo_pci *)(tp->t_card_ptr))

/* uart bit assignments */
#define TXRDY	0x20
#define RXRDY	0x01
#define DLAB	0x80
#define OVERR	0x02
#define FRMERR	0x08
#define PARERR	0x04
#define BRKBIT	0x10
#define THRE	0x20
#define TEMT	0x40
#define TXINTEN 0x02
#define RXINTEN 0x05  /* receiver and line status ints */
#define MSINTEN 0x08  /* modem status ints */
#define PENABLE 0x08
#define EPAR	0x10
#define TWOSB	0x04
#define SETBRK	0x40
#define	RLSD	0x80

/* Modem Control masks */
#define	PCI_RTS		0x02
#define	PCI_DTR		0x01
#define	PCI_DRS		0x04

/* Modem Status masks */
#define	PCI_RI		0x40
#define	PCI_DCD		0x80
#define	PCI_DSR		0x20
#define	PCI_CTS		0x10

#define CCITT_IN	(MDCD | MCTS | MDSR | MRI)
#define CCITT_OUT	(MDTR | MRTS | MDRS)

struct apollo_pci			/* device struct for hardware device */
{
	unsigned char	apollo_pci_data;
	unsigned char	pcif1[3];
	unsigned char	apollo_pciicntl;	/* interrupt control */
	unsigned char	pcif2[3];
	unsigned char	apollo_pciistat;	/* interrupt status */
	unsigned char	pcif3[3];
	unsigned char	apollo_pcilcntl;	/* line control */
	unsigned char	pcif4[3];
	unsigned char	apollo_pcimcntl;	/* modem control */
	unsigned char	pcif5[3];
	unsigned char	apollo_pcilstat;	/* line status */
	unsigned char	pcif6[3];
	unsigned char	apollo_pcimstat;	/* modem status */
};
#define baudlo apollo_pci_data
#define baudhi apollo_pciicntl

#define FIFO_ENABLE 	0x01

#define ON	1
#define OFF	0

/* Temporary until it can be defined in tty.h */
#define	T_VHANGUP	-1

/* This table maps unix symbolic baud values to Apollo uart baud values.
 * 153600/baudmap[BAUD] yields the baud rate.  This table is also used
 * for various timing calculations.
 */
 
short apollo_baudmap[] = {
		0xFFFF,	/* hangup */
		0x2710,	/* 50 baud */
		0x1A06,	/* 75 baud */
		0x11C1,	/* 110 baud */
		0x0E85,	/* 134.5 baud */
		0x0D05,	/* 150 baud */
		0x09C4,	/* 200 baud */
		0x0683,	/* 300 baud */
		0x0341,	/* 600 baud */
		0x022C,	/* 900 baud */
		0x01A1,	/* 1200 baud */
		0x0115,	/* 1800 baud */
		0x00D0,	/* 2400 baud */
		0x008B, /* 3600 baud */
		0x0068,	/* 4800 baud */
		0x0045,	/* 7200 baud */
		0x0034, /* 9600 baud */
		0x001A,	/* 19200 baud */
		0x000D,	/* 38400 baud */
};

/*
 * 15 May 92	jlau
 *
 * Added a new field (i.e. open_count) to the util_stuff structure to keep 
 * track of the successful opens. 
 *
 * WARNING: The common code (i.e. ttycomn.c) shared by the the pdi.c
 *          the pci.c, the apollo_pci.c and the mux.c drivers assumes
 *          that the tp->utility points to the open_count. Therefore,
 *          do NOT move the location of the open_count when new fields
 *          need to be added to the HW data structure.
 */

/* the following is for the break madness/NID chip problem */
struct util_stuff{
	u_int	open_count;  /* this field must be the 1st one in structure */
	u_int	vhangup;
	int	break_end;
	int	break_madness
	};
#define tp_utility(tp) ( (struct util_stuff *) (tp)->utility )

#define RECEIVE_TRIGGER 0x0000C0        /* minor number field */


apollo_pci_open(dev, flag)

dev_t	dev;
int	flag;

{
/*---------------------------------------------------------------------------
 | FUNCTION -	Perform open processing.
 |
 | PARAMETERS PASSED -
 |
 | dev	Describes which serial card will be opened.
 | flag	Specifies 'open' access modes.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 May 92	dmurray	Added call to clear vhangup flag on new open.
 |
 | 08 Jul 92	marcel	added check for ccitt mode, which we don't support
 |			in this hardware.
 ---------------------------------------------------------------------------*/

	int err ;
	register struct tty *tp;
	register struct apollo_pci *apollo_pci ;


	if (dev & CCITT_MODE) /* OOPS, we don't support CCITT */
		return (ENXIO);

	tp = apollo_pci_tp ;
	apollo_pciproc( tp, T_VHANGUP, 0 );
	err = ttycomn_open(dev, apollo_pci_tp, flag) ;
	if (err != 0)
		return (err) ;

	if (tp->t_hardware & FIFO_MODE) {
		/* flush the fifo */
		apollo_pci = PORTADR;
		apollo_pci->apollo_pciistat = 0x87;

		/*
		** Turn on the FIFO
		** Receiver trigger level is 
		** specified in minor number field
		*/
/*
		apollo_pci->apollo_pciistat =
			(tp->t_dev & RECEIVE_TRIGGER) | FIFO_ENABLE;
*/
		/* 
		** Set the reciever trigger level to 1 since the UART
		** doesn't implement the reciever trigger timeout.
		*/
		apollo_pci->apollo_pciistat = FIFO_ENABLE;
		tp->t_hardware |= FIFOS_ENABLED;
	}
	return (0) ;
}


/****************************************************************************
 * apollo_pci_close (dev)
 *
 *   dev_t dev - Describes which serial card is to be closed.
 *
 *   apollo_pci_close is called when the last close command has been issues for
 *   a specific serial card.  This procedure calls the generic TTY close
 *   routine.
 ****************************************************************************/

apollo_pci_close(dev)

   dev_t  dev;
{
	return(ttycomn_close(dev, apollo_pci_tp));
}


/****************************************************************************
 * apollo_pci_read (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be read from.
 *
 *   apollo_pci_read is called each time a read request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY read routine.
 ****************************************************************************/

apollo_pci_read(dev, uio)

   dev_t  dev;
{
	/* Call the generic TTY read routine */
	return (ttread (apollo_pci_tp, uio));
}


/****************************************************************************
 * apollo_pci_write (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be written to.
 *
 *   apollo_pci_write is called each time a write request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY write routine.
 ****************************************************************************/

apollo_pci_write(dev, uio)

   dev_t  dev;
{
	/* Call the generic TTY write routine */
	return (ttwrite (apollo_pci_tp, uio));
}


/****************************************************************************
* apollo_pci_ioctl (dev, cmd, arg, mode)
*
*   long dev - Describes which serial card is to be affected.
*   int  cmd - Specifies the ioctl command being requested.
*   char *arg - Points to the passed-in parameter block.
*   int  mode - Specifies the current access modes.
*
*   apollo_pci_ioctl is called each time an ioctl request is made for a
*   specific serial card.  This entry point is used to perform
*   TTY specific functions.  If one of the special SERIAL commands
*   is given, then it will be handled here.  If one of the standard
*   TTY commands is given, then the generic TTY ioctl routine will
*   be called upon to handle it.
****************************************************************************/

apollo_pci_ioctl(dev, cmd, arg, mode)

   dev_t  dev;
   int   cmd;
   int   *arg;
   int   mode;

{
   register struct tty *tp;                  /* Ptr to tty structure */
   register int s;                           /* Current intr level */
   register err = 0;

   tp = apollo_pci_tp;		/* get the tty pointer */
   /*
    * Check the requested ioctl command, and if it is one of the
    * new TTY commands, then process it here; else, call the
    * generic TTY ioctl routine.
    */
   switch(cmd) {

      case MCGETA:
	 /* Return the current state of the modem lines */
	 apollo_pciproc(tp, T_MODEM_STAT);
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
	    tp->t_smcnl &= ~(CCITT_OUT);	/* clear shadow control bits */
	    tp->t_smcnl |= (*arg & CCITT_OUT);	/* set according to user */
	    apollo_pciproc(tp, T_MODEM_CNTL);
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

/*    case TIOCSETP:	*/
      case TCSETA:
      case TCSETAW:
      case TCSETAF:
      case TCSETATTR:
      case TCSETATTRD:
      case TCSETATTRF:
	if ((err=ttiocom(tp, cmd, arg, mode)) == -1) {
		err = 0;
		apollo_pciparam(tp);
	}
	return(err);

      default:
	 /*
	  * Pass the ioctl command on through to the generic TTY ioctl
	  * handler.  If a non-zero value is returned, then call apollo_pciparam
	  * to force the configuration to be updated to the newly
	  * specified values.
	  */
	if ((err=ttiocom(tp, cmd, arg, mode)) == -1) {
		err = 0;
	    	apollo_pciparam(tp);
	}
	return(err);
   }
}


/* this is to get the tty pointer then call ttselect */
apollo_pci_select(dev, rw)
dev_t dev;
{
	return (ttselect(apollo_pci_tp, rw));
}


/****************************************************************************
  apollo_pciparam -	Set up apollo_pci according to ioctl args.
 ****************************************************************************/

apollo_pciparam(tp)
register struct tty *tp;
{
	register flags, lcl;
	register struct apollo_pci *apollo_pci = PORTADR;

	flags = tp->t_cflag;
	if ((flags&CBAUD) == 0) {
		/* hang up line */
		ttymodem(tp, OFF);
		tp->t_state &= ~CARR_ON;
		return;
	}
	lcl = 0;
	if (flags&CREAD)
		tp->t_sicnl |= RXINTEN;
	else
		tp->t_sicnl &= ~RXINTEN;
	lcl |= (flags&CSIZE)/CS6;
	if (flags&PARENB) {
		lcl |= PENABLE;
		if ((flags&PARODD) == 0)
			lcl |= EPAR;
	}
	if (flags&CSTOPB)
		lcl |= TWOSB;
	tp->t_slcnl = lcl;
	tp->t_sicnl |= TXINTEN;
	tp->t_int_flag |= PEND_TXINT;	/* we must wait for every byte */
	apollo_pciinit(tp);
}


apollo_pci_intr(inf)
struct interrupt *inf;
/*---------------------------------------------------------------------------
 | FUNCTION - apollo_pci_intr -	interrupt service routine.
 |
 | PARAMETERS PASSED -
 |	inf          interrupt information structure
 |
 | MODIFICATION HISTORY
 |
 | Date       User      Changes
 | 3 Mar 91   marcel    Fix for Visionary Design Systems Hotsite
 |			 (SR# 5003047639 aka DTS XXXXXXXXX)
 |			+ changed while(data ready bit) loop into an if
 |			  to prevent hang on stuck DR bit.
 |			+ hoisted read of fifo in case :reciever line status
 |			  so that an extra null wouldn't be put out
 |			+ added code to ignore extra break interupts for 250 
 |			  milliseconds after receipt of the first one, using
 |			  a new field in the utility area of the tty structure
 |			  and lbolt for timing.
 |			+ if a null or an 0xFF character comes out of the fifo
 |			  during break_madness mode, we ignore it, and
 |			  continue, because it is a bogus character.  If the 
 |			  character is not a null or 0xFF, then we assume that
 |			  break madness is over, and pass the character up.
 ---------------------------------------------------------------------------*/
{
	register struct tty *tp = (struct tty *) inf->temp;
	register struct apollo_pci *apollo_pci = PORTADR;
	register unsigned char stat;
	register unsigned short val;

	tp->modem_status = 0;	/* for MTNOACTIVITY timeout */
	if (!(tp->t_state&(ISOPEN|WOPEN))) {
		tp->t_sicnl &= ~(TXINTEN|RXINTEN); /* no ints until open */
		apollo_pciinit(tp);
	}
	while (1) {
		stat = (unsigned char)apollo_pci->apollo_pciistat;
		if ((stat&0x1)!=0) return;
		switch ((stat>>1)&0x3) {
		case 1: /* transmitter register empty */
			sw_trigger(&tp->xmit_intloc,apollo_pcixmit,tp,0,tp->t_int_lvl);
			continue;
		case 0: /* modem status */
			if (apollo_pci->apollo_pcimstat & 0x04) { /* trailing edge of RI ? */
		/*
		 * the 98626/98644 card interrupts only on the trailing edge
		 * of Ring Indicate (pin22).  Therefore, set up the tty
		 * structure as if we've already received the leading edge.
		 * This code should match what happens for leading edge of RI.
		 */
				tp->t_smcnl |= MRI;
				tp->t_time = lbolt - (HZ/2);
				tp->t_int_flag |= START_OF_RING;
				if (tp->t_state & WOPEN)
					wakeup(&tp->t_canq);
			}
			tty_modem_intr(tp);
			continue;
		case 3: /* receiver line status */
			/* read and clear */
			stat = (unsigned short)
			  ((unsigned char)apollo_pci->apollo_pcilstat);
			val = (unsigned short)
			  ((unsigned char)apollo_pci->apollo_pci_data);

			if (stat&BRKBIT){
				val |= FRERROR;
 				if (tp_utility(tp)->break_end < lbolt )
				  /*begin break madness mode */
				  tp_utility(tp)->break_end=lbolt+(HZ/4);
				else  
				  return(0);
				/*end of if break_madness */
			 } else {
				if (stat&FRMERR) 
					val |= FRERROR;
				if (stat&OVERR) 
					val |= OVERRUN;
				if (stat&PARERR) 
					val |= PERROR;

				/*
				 * if we get a null or FF character on the
				 * heels of a break_madness condition, get rid
				 *  of it, as it is probably bogus. Otherwise,
				 *  end the break madness and go on processing.
				 */
				if (tp_utility(tp)->break_end >= lbolt ) 
				    if( (val == 0xFF) || (val == 0x00))
				      return(0);
				    else
				      tp_utility(tp)->break_end = 0;

			}
			
			/* Save data and status in the queue */
			/* Earlier data will be lost when buffer overflows */
			if (tp->t_in_count >= TTYBUF_SIZE) {
				if (++tp->t_in_head >= tp->t_inbuf+TTYBUF_SIZE)
					tp->t_in_head = tp->t_inbuf;
			} else {
				tp->t_in_count++;
			}
			*tp->t_in_tail++ = val;
			if (tp->t_in_tail >= tp->t_inbuf+TTYBUF_SIZE)
				tp->t_in_tail = tp->t_inbuf;
			break;
		case 2: /* received data available */
	        	/* loop while DR bit is set */
	        	if (apollo_pci->apollo_pcilstat & 0x1) {
				val = (unsigned short)
				  ((unsigned char)apollo_pci->apollo_pci_data);

				/*
				 * if we get a null or FF character on the
				 * heels of a break_madness condition, get rid
				 *  of it, as it is probably bogus. Otherwise,
				 *  end the break madness and go on processing.
				 */
				if (tp_utility(tp)->break_end >= lbolt ) 
				    if( (val == 0xFF) || (val == 0x00))
				      return(0);
				    else
				      tp_utility(tp)->break_end = 0;


				/* if not up, don't process data */
				if (!(tp->t_state & CARR_ON))
					continue;
				/*
				** Save data and status in the queue.
				** Earlier data will be lost when buffer 
				** overflows.
				*/
				if (tp->t_in_count >= TTYBUF_SIZE) {
					if (++tp->t_in_head >= tp->t_inbuf+TTYBUF_SIZE)
						tp->t_in_head = tp->t_inbuf;
				} else {
					tp->t_in_count++;
				}
				*tp->t_in_tail++ = val;
				if (tp->t_in_tail >= tp->t_inbuf+TTYBUF_SIZE)
					tp->t_in_tail = tp->t_inbuf;
			}
			break;
		}

		if (! (tp->t_state & CARR_ON))
			/* if not up, don't process data */
			continue;

		/* Signal level zero service to be done by ite_service */
		if (!tp->t_rcv_flag) {
			tp->t_rcv_flag = TRUE;
			sw_trigger(&tp->rcv_intloc,apollo_pcircv,tp,0,tp->t_int_lvl);
		}
	}
}


apollo_pcircv(tp)
register struct tty *tp;
{
	register ushort c;
	register int x;

	while (1) {
		/* Fetch Data and Status */
		x = splx(tp->t_int_lvl);
		if (tp->t_in_count == 0) {
			/* Indicate that sw trigger has been disarmed */
			tp->t_rcv_flag = FALSE;
			splx(x);
			return;
		}
		tp->t_in_count--;
		c = *tp->t_in_head++;
		if (tp->t_in_head >= tp->t_inbuf+TTYBUF_SIZE)
			tp->t_in_head = tp->t_inbuf;
		splx(x);

		/* receiver line status */
		/***** temp for 64000 detect of '626/'644 overruns *****/
		if (c & OVERRUN)
			c |= 0xFFFF;	/* no-op */
		/**********************************/

		/* received data available */
		(*linesw[tp->t_line].l_input)(tp, c, 0, tp->t_in_count);
	}
}


/****************************************************************************
 apollo_pcisend -	
 ****************************************************************************/

int
apollo_pcisend(tp)
register struct tty *tp;
{
	register struct apollo_pci *apollo_pci = PORTADR;

	if ((apollo_pci->apollo_pcilstat & THRE) == 0)
		return 0;

	if (tp->t_state & (TTXON|TTXOFF)) {
		if (tp->t_state & TTXON) {
			apollo_pci->apollo_pci_data = CSTART;
			tp->t_state &= ~TTXON;
			return 0;
		}
		if (tp->t_state & TTXOFF) {
			apollo_pci->apollo_pci_data = CSTOP;
			tp->t_state &= ~TTXOFF;
			return 0;
		}
	}
	if (tp->t_state & BUSY) {
		apollo_pci->apollo_pci_data = (char)tp->t_buf;
		tp->t_state &= ~BUSY;
		wakeup((caddr_t)(&tp->t_state));
		return 1;
	}

	return 0;
}




apollo_pcixmit(tp)
register struct tty *tp;
{
	/* Indicate that sw trigger has been disarmed */
	int s;

	s = splx(tp->t_int_lvl);
	if (apollo_pcisend(tp) == 0) {
		splx(s);
		return;
	}
	splx(s);
	apollo_pciproc(tp, T_OUTPUT);
}


apollo_pciproc(tp, cmd, data)

register struct tty	*	tp;
int				cmd;
int				data;

{
/*---------------------------------------------------------------------------
 | FUNCTION -	Perform various functions.
 |
 | PARAMETERS PASSED -
 |	tp	Pointer to tty structure.
 |	cmd	Function code for action to perform.
 |	data	Function code dependent argument.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 May 92	dmurray	Added T_VHANGUP.
 |
 ---------------------------------------------------------------------------*/

	register c;
	register struct apollo_pci *apollo_pci;
	int s;

	s = splx(tp->t_int_lvl);

	switch(cmd) {
	case T_TIME:
		tp->t_state &= ~TIMEOUT;
		tp->t_slcnl &= ~SETBRK;
		apollo_pciinit(tp);
		goto start;

	case T_WFLUSH:
	case T_RESUME:
		tp->t_state &= ~TTSTOP;
		apollo_pcisend(tp);
		goto start;

	case T_OUTPUT:
	start:
		if (tp->t_state&(TIMEOUT|TTSTOP|BUSY))
			break;
		while ((c=getc(&tp->t_outq)) >= 0) {
			tp->t_buf = (struct cblock *)c; /* use buf as temp */
			tp->t_state |= BUSY;
			if (!apollo_pcisend(tp)) break;
		}
		ttoutwakeup(tp); /* check for sleepers to wakeup */
		break;

	case T_SUSPEND:
		tp->t_state |= TTSTOP;
		break;

	case T_BLOCK:
		tp->t_state &= ~TTXON;
		tp->t_state |= TTXOFF|TBLOCK;
		tp->t_sicnl &= ~TXINTEN;
		apollo_pciinit(tp);
		apollo_pcisend(tp);
		break;

	case T_RFLUSH:
		if (!(tp->t_state&TBLOCK))
			break;
	case T_UNBLOCK:
		tp->t_state &= ~(TTXOFF|TBLOCK);
		tp->t_state |= TTXON;
		tp->t_sicnl |= TXINTEN;
		apollo_pciinit(tp);
		apollo_pcisend(tp);
		break;

	case T_BREAK:
		tp->t_slcnl |= SETBRK;
		apollo_pciinit(tp);
		tp->t_state |= TIMEOUT;
		timeout(ttrstrt, tp, HZ/4, NULL);
		break;

	case T_PARM:
		apollo_pciparam(tp);
		break;

	case T_MODEM_CNTL:
		{
			register int smcnl = tp->t_smcnl;
			register int modem = 0;

			if (smcnl & MDTR)	modem |= PCI_DTR;
			if (smcnl & MRTS)	modem |= PCI_RTS;
			if (smcnl & MDRS)	modem |= PCI_DRS;
			PORTADR->apollo_pcimcntl = modem;
		}
		break;

	case T_MODEM_STAT:
		{
			/* read modem status */
			register int modem = PORTADR->apollo_pcimstat;
			register int smcnl = tp->t_smcnl;

			smcnl &= ~(CCITT_IN);
			if (modem & PCI_CTS)	smcnl |= MCTS;
			if (modem & PCI_DSR)	smcnl |= MDSR;
			if (modem & PCI_DCD)	smcnl |= MDCD;
			if (modem & PCI_RI )	smcnl |= MRI ;
			tp->t_smcnl = smcnl;
		}
                break;
        case T_VHANGUP:
		tp_utility(tp)->vhangup	= data;
                ttcontrol( tp, LDC_VHANGUP, data );
                ttycomn_control( tp, cmd, data );
                if (data == 0)
                        break;
                wakeup( (caddr_t)(&tp->t_state) );
		break;
	}
	splx(s);
}

/****************************************************************************
  apollo_pciinit -	PCI initialization sequence from Signetics Bipolar/MOS
		Microprocessor Data Manual page 178.  This routine should
		be called anytime one of the register is modified.
 ****************************************************************************/

apollo_pciinit(tp)
register struct tty *tp;
{
	short s;
	register i;
	register struct apollo_pci *addr = PORTADR;

	s = splx(tp->t_int_lvl);		/* if lower, init won't work! */
	i = apollo_baudmap[tp->t_cflag&CBAUD];
	addr->apollo_pcilcntl = DLAB | tp->t_slcnl; /* access baud registers */
	addr->baudhi = i>>8;
	addr->baudlo = i & 0xff;
	addr->apollo_pcilcntl = tp->t_slcnl;	/* line control */
	addr->apollo_pciicntl = tp->t_sicnl;	/* interrupt control */

	tty_delta_CLOCAL(tp);

	/* enable interrupts */
	switch (tp->t_isc_num) {
		case 5: /* UART2 */
			*((char *)(0x41C0E0 + LOG_IO_OFFSET)) |= 0x40;
			break;
		case 6: /* UART3 */
			*((char *)(0x41C0E0 + LOG_IO_OFFSET)) |= 0x80;
			break;
		case 9: /* UART1 */
			*((char *)(0x41C0E0 + LOG_IO_OFFSET)) |= 0x20;
			break;
		default:
			panic("apollo_pciinit: Internal RS-232 not at sc 5/6/9");
	}
	splx(s);
}


/*************************************************************************
** Console Interface Routines:						**
**									**
** apollo_pciputchar(c)		Print a char - used by Kernel printf.	**
** apollo_pcioutnow(c)		Print the character now.		**
** apollo_pci_who_init(n)	Set-up Line characteristics for console.**
*************************************************************************/

apollo_pciputchar(c)
register ushort c;
{
	if (c == 0) return;
	if (c == '\n') apollo_pcioutnow('\r');
	apollo_pcioutnow(c);
}


apollo_pcioutnow(c)
register ushort c;
{
	register struct tty *tp = cons_tty;
	register struct apollo_pci *pci = PORTADR;
	register x;

	/* Don't let ANYONE in until I output this character! */
	x = spl6();
	/* Wait for Transmitter Holding Register to Empty. */
	while ((pci->apollo_pcilstat&THRE)==0)
		;
	pci->apollo_pci_data = (char)c;
	splx(x);	
}

/* Set up Line characteristics for console. */
apollo_pci_who_init(tp, tty_num, sc)
register struct tty *tp;
{
	extern char ttcchar[];
	register char *addr;
	register i;

	if (tty_num != -1)
		return;

/* Default values.
** Some of these defaults can be changed by 
** the user through an ioctl system call.
*/
	if (tp == NULL)	{	/* we must get tp */
		tp = (struct tty *)(isc_table[sc]->ppoll_f);
		cons_tty = tp;
		cons_dev = (dev_t)((sc << 16) + 0x04);
	}

	tp->t_state = CARR_ON;
	tp->t_iflag = ICRNL|IXON;
	tp->t_oflag = OPOST|ONLCR|TAB3;
	tp->t_lflag = ISIG|ICANON|ECHO|ECHOK;
	tp->t_cflag = CLOCAL|CREAD;

	/* setup default control characters */
	bcopy(ttcchar, tp->t_cc, NLDCC);

	tp->t_sicnl &= ~(TXINTEN|RXINTEN); /* no interrupts until open */

	/* Set Baud Rate to 9600 */
	tp->t_cflag |= B9600;
	
	/* Set Line Characteristics */
	tp->t_cflag |= CS8;	/* set character to 8 bits */
	tp->t_cflag &= ~PARENB;	/* no parity */

	apollo_pciparam(tp);
	printf("\n    System Console is Apollo RS-232 Port at select code %d\n", sc);
}

struct tty *
apollo_pci_pwr(isc_num, addr, id, il, cs, apollo_pci_total)

char *addr;
int *cs;
int *apollo_pci_total;

{
/*---------------------------------------------------------------------------
 | FUNCTION -		Power-on Initialization Routine.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 15 May 92	jlau	Added code to initialize the newly-added open_count
 |			field in the util_stuff structure.
 |
 | 28 May 92	dmurray	Fix for INDaa12062.
 |			Initialize RTS and DTR to low during initialization.
 |
 --------------------------------------------------------------------------*/

	register struct tty *tp;
	register struct isc_table_type *isc;
	extern struct tty_driver ap_sio_driver;

	if (id != APOLLOPCI)
		return;

	/* allocate isc and tp stuff */
	isc = (struct isc_table_type *)calloc(sizeof(struct isc_table_type));
	isc_table[isc_num] = isc;
	isc->card_type = APOLLOPCI;
	isc->int_lvl = il;
	/* for indirect calls */
	isc->tty_routine = (struct tty_drivercp *)&ap_sio_driver;

	/* make tty structure now */
	tp = (struct tty *)calloc(sizeof(struct tty));
	(struct tty *)(isc->ppoll_f) = tp;	/* and save it */

	tp->t_proc = apollo_pciproc;			/* for ttyflush */
	/* Set type field */
	tp->t_drvtype = &ap_sio_driver;

	/* allocate and initialize utility pointer for break madness */
	tp_utility(tp) =(struct util_stuff *)calloc(sizeof(struct util_stuff));
	tp_utility(tp)->open_count = 0;
	tp_utility(tp)->break_end = 0;

	switch (isc_num) {
		case 5: /* UART2 */
			/* Set card address field */
			isc->card_ptr = (int *)(addr + 0x40);
			tp->t_card_ptr = (char *)isc->card_ptr;
			/* Set up interrupt service routine for it. */
			isrlink(apollo_pci_intr, il, 0x41C0E8 + LOG_IO_OFFSET, 0x40, 0x40, 0, tp);
			break;	
		case 6: /* UART3 */
			/* Set card address field */
			isc->card_ptr = (int *)(addr + 0x60);
			tp->t_card_ptr = (char *)isc->card_ptr;
			/* Set up interrupt service routine for it. */
			isrlink(apollo_pci_intr, il, 0x41C0E8 + LOG_IO_OFFSET, 0x80, 0x80, 0, tp);
			break;	
		case 9: /* UART1 */
			/* Set card address field */
			isc->card_ptr = (int *)(addr + 0x20);
			tp->t_card_ptr = (char *)isc->card_ptr;
			/* Set up interrupt service routine for it. */
			isrlink(apollo_pci_intr, il, 0x41C0E8 + LOG_IO_OFFSET, 0x20, 0x20, 0, tp);
			break;	
		default:
			panic("Internal RS-232 not at sc 5/6/9");
	}
	/* Set up input queue for it */
	tp->t_in_head = tp->t_in_tail = tp->t_inbuf = 
		(ushort *)calloc(TTYBUF_SIZE*2);
	tp->t_in_count = 0;
	tp->t_int_flag = 0;
	tp->t_rcv_flag = FALSE;
	tp->t_xmt_flag = FALSE;
	tp->t_int_lvl = il;
	tp->t_pgrp = 0;
	tp->t_cproc = NULL;
	tp->t_hw_flow_ctl = 0;
	tp->t_open_sema = 0;

        /* Check for remote console */
        if (!remote_console) {
		eeprom_get_token(9, &apollo_console, 1);
                if (apollo_console == isc_num) {
                        *cs = 1;
                        cons_tty = tp;
                        remote_console = TRUE;
                        /* make console dev number */
                        cons_dev = (dev_t)((isc_num << 16) + 0x04);
                }
        }

	/* Increment to next location */
	(*apollo_pci_total)++;

	/*
 	* Turn on modem interrupts.  Since *tp may be sc 9 (KDB/KCDB port), set
 	* baud rate and character size to what KDB/KCDB expect.
 	*/
	tp->t_cflag |= B9600;		/* initialize baud rate divisor */
	tp->t_cflag |= CS8;		/* set character to 8 bits */
	tp->t_sicnl = MSINTEN;		/* enable modem interrupts forever */

	/* save the select code for possible hidden mode operations */
	tp->t_isc_num = isc_num;

	tp->t_hardware = FIFO_MODE;
	apollo_pciparam(tp);

	/* Initialize RTS and DTR to low.  Their power-on state is high. */
	tp->t_smcnl &= ~(CCITT_OUT);
	apollo_pciproc(tp, T_MODEM_CNTL);
}

apollo_pci_wait(tp)
struct tty	*	tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Wait for output to drain from the harwdare.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 May 92	dmurray	Added PCATCH to sleeps and check for vhangup.
 |
 --------------------------------------------------------------------------*/

	int	s;

	s = splx (tp->t_int_lvl);
	/* If there is something in the tbuf, wait for it to get to the chip. */
	if (tp->t_state & BUSY) {
		if (sleep( (caddr_t)(&tp->t_state), (TTOPRI | PCATCH) )) {
			splx(s);
			return(EINTR);
		}
		if (tp_utility(tp)->vhangup) {
			splx(s);
			return(EBADF);
		}
	}

	/* Wait until the shift register is emptied. */
	while ((PORTADR->apollo_pcilstat & TEMT) == 0)
		delay(HZ/20);

	splx(s);
	return(0);
}

extern ttwrite(),ttread();

struct tty_driver ap_sio_driver = {
	SIO626, apollo_pci_open, apollo_pci_close, apollo_pci_read, 
	apollo_pci_write, apollo_pci_ioctl, apollo_pci_select,
	apollo_pciputchar, apollo_pci_wait, apollo_pci_pwr,
	apollo_pci_who_init, 0
};

/*
** linking and inititialization routines
*/

extern int (*make_entry)();

int (*ap_sio_saved_make_entry)();


ap_sio_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	char *card_name;

	if (id != APOLLOPCI)
		return(*ap_sio_saved_make_entry)(id, isc);

	card_name = "Internal RS-232C Serial Interface";
#ifdef DDB
	if ((ddb_boot) && (isc->my_isc == 9))
		io_inform(card_name, isc, -3);
	else {
#endif DDB
		io_inform(card_name, isc, isc->int_lvl);
		msg_printf("    With 8 byte rcv fifo.\n");
		if (isc->int_lvl != 5) {
			printf("WARNING: Interface card interrupt level at %d instead of 5.\n", isc->int_lvl);
			printf("    This configuration is for HP internal usage only.\n");
		}
#ifdef DDB
	}
#endif DDB
	return FALSE;
}

ap_sio_link()
{
	extern struct tty_driver *tty_list;

	ap_sio_saved_make_entry = make_entry;
	make_entry = ap_sio_make_entry;

	ap_sio_driver.next = tty_list;
	tty_list = &ap_sio_driver;
}
