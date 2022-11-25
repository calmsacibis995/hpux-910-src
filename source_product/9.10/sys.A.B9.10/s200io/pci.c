/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/pci.c,v $
 * $Revision: 1.7.84.11 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/12/16 09:59:14 $
 */


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
 hp 98626/98644 driver 
 */


#include "../h/param.h"		/* various types and macros */
#include "../h/errno.h"
#include "../h/file.h"
#include "../h/tty.h"		/* tty structure and parameters */
#include "../h/ld_dvr.h"
#include "../h/conf.h"		/* linesw stuff */
#include "../wsio/intrpt.h"
#include "../wsio/hpibio.h"
#define _INCLUDE_TERMIO
#include "../h/termios.h"
#include "../h/termiox.h"
#include "../h/modem.h"

#define signal gsignal

#ifdef DDB
extern int ddb_boot;
#endif /* DDB */

extern struct tty *cons_tty;
extern dev_t cons_dev;
extern char remote_console;
extern int lbolt;
extern short baudmap[];

extern int monitor_on;
extern void dbg_addr();

int	pciproc();
int	ttrstrt();
int	pcixmit();
int	pcircv();
static void pci_reset_tx_fifo() ;
static void pci_reset_rx_fifo() ;

/* Bappe made me do it! */
#ifdef CALL_KDB_ON_BREAK
unsigned call_kdb_on_break = 0xDEADBEEF;
#else /* not CALL_KDB_ON_BREAK */
unsigned call_kdb_on_break = 0xBEEF;
#endif /* else not CALL_KDB_ON_BREAK */

/* this gets the tp pointer from the isc table */
#define pci_tp  (struct tty *)(isc_table[m_selcode(dev)]->ppoll_f)

#define OUTLINE	0x80			/* set if priveleged line */

/* description of the UART used on i/o boards */

#define PORTADR  ((struct pci *)(tp->t_card_ptr + 0x10))
#define INTRENAB    0x80

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
#define RX_FIFO_RESET           0x2
#define TX_FIFO_RESET           0x4


/* Modem Control masks */
#define	PCI_RTS		0x02
#define	PCI_DTR		0x01
#define	PCI_DRS		0x04
#define	PCI_OUT2	0x08	/* Used to enable RTS hardware handshaking. */

/* Modem Status masks */
#define	PCI_RI		0x40
#define	PCI_DCD		0x80
#define	PCI_DSR		0x20
#define	PCI_CTS		0x10

#define CCITT_IN	(MDCD | MCTS | MDSR | MRI)
#define CCITT_OUT	(MDTR | MRTS | MDRS)

struct pci			/* device struct for hardware device */
{
	char	
		pcif1,
	pci_data,
		pcif2,
	pciicntl,	/* interrupt control */
		pcif3,
	pciistat,	/* interrupt status */
		pcif4,
	pcilcntl,	/* line control */
		pcif5,
	pcimcntl,	/* modem control */
		pcif6,
	pcilstat,	/* line status */
		pcif7,
	pcimstat;	/* modem status */
};

/*
 * 15 may 92	jlau
 *
 * Added a new field (i.e. open_count) to the pcihwdata structure to keep
 * track of the successful opens.
 *
 * WARNING: The common code (i.e. ttycomn.c) shared by the the pdi.c
 *          the pci.c, the apollo_pci.c and the mux.c drivers assumes
 *          that the tp->utility points to the open_count. Therefore,
 *          do NOT move the location of the open_count when new fields
 *          need to be added to the HW data structure.
 */
struct pcihwdata
{
	u_int	open_count;	/* this field must be 1st one in structure */
	u_int	vhangup;
	u_char	fileopen;	/* boolean flag for open state */
};

/* Temporary until it can be defined in tty.h */
#define	T_VHANGUP	-1

#define baudlo pci_data
#define baudhi pciicntl
#define pci_config pci_data		/* hidden mode register */

/* pci_config bit defines */
#define PCI_REM_LOCL	0x80	/* remote/local */
#define PCI_RUPT_LVL	0x30	/* interrupt level */
#define PCI_HSHK	0x04	/* hardware handshake present */
#define PCI_HSCLK	0x02	/* high speed clock */
#define PCI_ME		0x01	/* modem enable */

#define FIFO_ENABLE 	0x09

/* This table maps unix symbolic baud values to 98626 uart baud values.
 * 153600/baudmap[BAUD] yields the baud rate.  This table is also used
 * for various timing calculations.
 */
 
short highspeed_baudmap[] = {	/* For reading unix config */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000, /* unused place holder */
		0x0000,	/* unused place holder */
		0x0000,	/* unused place holder */
		0x0000, /* unused place holder */
		0x0000,	/* unused place holder */
		0x000c,	/* 38400 baud */
		0x0008,	/* 57600 baud */
		0x0004,	/* 115200 baud */
		0x0002,	/* 230400 baud */
		0x0001,	/* 460800 baud */
};

ushort baudmap2[] = {	/* For reading switches */
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
		B7200,	/* 7200 baud */
		B9600,  /* 9600 baud */
		B19200,	/* 19200 baud */
};

#define ON	1
#define OFF	0

/* HANGUP(modem file) or NOHANGUP(direct file) if a B0 baud rate is received */

#define HANGUP          1
#define NOHANGUP        0

/*	card id values this driver supports */
#define ID626	0x2
#define ID644	0x42

extern ttcontrol();
#define	LD_CONTROL	ttcontrol

#define CTS_PACING	0x000008	/* minor number field */
#define TXLIMIT 	0x000030        /* minor number field */
#define RECEIVE_TRIGGER 0x0000C0        /* minor number field */

u_char pci_tx_limit[4] = {1,4,8,12};    /* max chars sent after getting XOFF */


pci_open(dev, flag)

dev_t		dev;
int		flag;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		This routine processes the open system call for the
 |			98626, 98644 serial ports.  pci_open first verifies
 |			that the card requested is installed, and then checks
 |			to see if the access mode requested can be granted.
 |			If it can be granted, then the proper open routine
 |			is invoked.
 |
 | PARAMETERS -
 |	dev		Contains the major/minor pair numbers for this device.
 |	flag		Contains the open flag information passed to the open
 |			system call.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 30 Jan 92	dmurray	+Added use of minor number pointer so that open
 |			 counts are kept correctly.
 |			+Call ioctl/TCSETX to enable RTS/CTS flow control.
 |
 | 17 Jul 92	dmurray	Deleted write to minor number pointer since we are
 |			keeping our own open counts.
 |
 | 24 Aug 94    rpc     Moved code from pciparam that resets the FIFO and
 |		        sets the receive trigger level. pciparam was
 |		        flushing the receive/xmit FIFO on every ioctl.
 |		        Also don't need to reset the receive trigger level
 |		        on each ioctl call. Once at open is OK.
 |
 --------------------------------------------------------------------------*/

	struct tty		*	tp		= pci_tp;
	struct termiox			termiox;
	struct pcihwdata	*	hw_p;
	register struct pci     *pci  ;

	hw_p = (struct pcihwdata *)tp->utility;

	/* CCITT mode and hardware flow control is invalid */
	if ((dev &(CCITT_MODE | CTS_PACING)) == (CCITT_MODE | CTS_PACING))
		return(ENOTTY);

	/* Clear vhangup flag. */
	pciproc( tp, T_VHANGUP, 0 );

	flag = ttycomn_open(dev, tp, flag);

	/*
	 * If this is the first successful open of this type, then set up
	 * the driver specific characteristics.  If the connection is being
	 * inherited, still set hardware flow control if the bit in this
	 * dev is set.
	 */
	if ((flag == 0) && (hw_p->fileopen == FALSE)) {
		hw_p->fileopen = TRUE;
 		if (tp->t_hardware & FIFO_MODE) {
 			/*
 			 *   Reset fifo pointers on first open (rpc: 8/17/94)
 			 */
 			pci_reset_tx_fifo(tp) ;
 			pci_reset_rx_fifo(tp) ;
 
 			/*
 			** Turn on the FIFOs
 			** Receiver trigger level is
 			** specified in minor number field
 			*/
			pci = PORTADR;
 			pci->pciistat = (tp->t_dev & RECEIVE_TRIGGER) |
 					FIFO_ENABLE;
 			tp->t_hardware |= FIFOS_ENABLED;
 		}
		tp->t_tx_limit = pci_tx_limit[((dev & TXLIMIT) >> 4)];
		tp->t_tx_count = 0;
		if (dev & CTS_PACING)
			termiox.x_hflag = (tp->t_hardware & HARDWARE_HANDSHAKE)
						 ? (RTSXOFF | CTSXON) : CTSXON;
		else
			termiox.x_hflag = tp->t_hw_flow_ctl & HWFC_MASK;
		termiox.x_cflag = 0;
		termiox.x_sflag = 0;
		pci_ioctl(dev, TCSETX, &termiox, 0);
	}
	
	return(flag);
}

pci_close(dev)

dev_t	dev;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		This routine processes the close system call for the
 |			98644 and 98626 ports.  This happens on last close of
 |			the device.  This procedure calls the generic TTY close
 |			routine.
 |
 | PARAMETERS
 |	dev		Contains the major/minor pair numbers for this device.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 30 Jan 92	dmurray	Added code to turn off hardware handshaking when
 |			the device is closed.
 |
 | 15 May 92	jlau	Changed code to check the newly added open_count
 |			for last close.  The other three counts (i.e.
 |			cul_count, tty_count, and ttyd_count are now used
 |			as "pending" open counts and should not be used for
 |			last close indication.  See correspnding changes
 |			in the ttycomn_close() routine.
 --------------------------------------------------------------------------*/

	/* Ptr to tty structure */
	register struct tty	*	tp	= pci_tp;
	struct pcihwdata	*	hw_p;
	int	err;

	hw_p = (struct pcihwdata *)tp->utility;

	err = ttycomn_close(dev, tp);

	/* Check for last close before closing the port */ 
	if (hw_p->open_count > 0)
		return (err);

	/* The following steps are taken only for the last close */

	hw_p->fileopen = FALSE;

	if ((tp->t_cflag & HUPCL) || (dev & DEV_TTY)) {
		if (tp->t_hw_flow_ctl & RTSXOFF) {
			tp->t_hardware &= ~HARDWARE_HNDSHK_ENABLED;
			pciproc(tp, T_MODEM_CNTL);
		}
		tp->t_hw_flow_ctl = 0;
	}
	return(0);
}

/****************************************************************************
 * pci_read (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be read from.
 *
 *   pci_read is called each time a read request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY read routine.
 ****************************************************************************/

pci_read(dev, uio)

   dev_t  dev;
{
	/* Call the generic TTY read routine */
	return (ttread (pci_tp, uio));
}

/****************************************************************************
 * pci_write (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be written to.
 *
 *   pci_write is called each time a write request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY write routine.
 ****************************************************************************/

pci_write(dev, uio)

   dev_t  dev;
{
	/* Call the generic TTY write routine */
	return (ttwrite (pci_tp, uio));
}

pci_ioctl(dev, cmd, arg, mode)

dev_t		dev;
int		cmd;
int	*	arg;
int		mode;
{
/*---------------------------------------------------------------------------
 | FUNCTION -         Perform an ioctl request.  Pci_ioctl is called each
 |                    time an ioctl request is made for a specific serial
 |                    card.  This entry point is used to perform TTY specific
 |                    functions.  If one of the special SERIAL commands is
 |                    given, then it will be handled here.  If one of the
 |                    standard TTY commands is given, then the generic TTY
 |                    ioctl routine will be called upon to handle it.
 |
 | PARAMETERS PASSED -
 |    long dev        Describes which serial card is to be affected.
 |    int  cmd        Specifies the ioctl command being requested.
 |    char *arg       Points to the passed-in parameter block.
 |    int  mode       Specifies the current access modes.
 |
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 19 Sep 90	jph	Fix for DTS INDaa08284. Added code to ignore baud
 |			rate changes that are not supported (e.g. B0 when
 |			set on a non-modem port.)  This behavior is currently
 |			required by POSIX.
 |
 | 30 Jan 92	dmurray	Added termiox(7) ioctls for RTS/CTS enhancement.
 |
 --------------------------------------------------------------------------*/

   register struct tty	*	tp;	/* Ptr to tty structure */
   register int			s;	/* Current intr level */
   register int			hangup;	/* Flag indicating B0 baud rate */
   register			err	= 0;
   u_long			old_cflag;
   struct termiox	*	termiox	= (struct termiox *)arg;

   tp = pci_tp;	       		/* get the tty pointer */
   old_cflag = tp->t_cflag;     /* save a copy of cflag for reference later */
   hangup = NOHANGUP;           /* hangup will be set if baud is set to B0 */
   /*
    * Check the requested ioctl command, and if it is one of the
    * new TTY commands, then process it here; else, call the
    * generic TTY ioctl routine.
    */
   switch(cmd) {

      case MCGETA:
	 /* Return the current state of the modem lines */
	 pciproc(tp, T_MODEM_STAT);
	 *arg = tp->t_smcnl;
	 return(err);

      case MCSETAW:
      case MCSETAF:
	 /* Wait for output to drain */
	 err = ttywait (tp);
	 if (err != 0)
	    return(err);
	 if (cmd == MCSETAF)
	    ttyflush (tp, (FREAD | FWRITE));

      case MCSETA:
	 /* Set the modem control lines */
	 s = splx (tp->t_int_lvl);
	 if (tp->t_state & SIMPLE) {
	    tp->t_smcnl &= ~(CCITT_OUT);	/* clear shadow control bits */
	    tp->t_smcnl |= (*arg & CCITT_OUT);	/* set according to user */
	    pciproc(tp, T_MODEM_CNTL);
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
		/*
                 * Even if the user set baud to B0, the hardware is not
                 * affected since we don't pass down the B0 baud to the card.
                 * If B0 was set, we set cflag back to the old baud rate
                 * to match the hardware.
                 */
		if ((tp->t_cflag & CBAUD) == 0) {
			tp->t_cflag &= ~CBAUD;
			tp->t_cflag |= old_cflag & CBAUD;
			hangup = HANGUP;
			}
		pciparam(tp,hangup);
	}
	return(err);

	case TCGETX:
		termiox->x_hflag = tp->t_hw_flow_ctl;
		termiox->x_cflag = 0;
/*		termiox->x_rflag = ;*/		/* Reserved field */
		termiox->x_sflag = 0;
		return(0);

	case TCSETXF:
	case TCSETXW:
	case TCSETX:
		if (tp->t_state & CCITT)
			return(ENOTTY);
		if (cmd != TCSETX) {
			err = ttywait (tp);
			if (err != 0)
			   return(err);
		}
		if (cmd == TCSETXF)
	    		ttyflush (tp, (FREAD | FWRITE));

		s = splx(tp->t_int_lvl);
		if (termiox->x_hflag & RTSXOFF) {
			if (tp->t_hardware & HARDWARE_HANDSHAKE)
				tp->t_hardware |= HARDWARE_HNDSHK_ENABLED;
			else {
				splx(s);
				return(EINVAL);
			}
		} else {
			tp->t_hardware &= ~HARDWARE_HNDSHK_ENABLED;
		}
		tp->t_hw_flow_ctl = (termiox->x_hflag & HWFC_MASK);

		LD_CONTROL(tp, LDC_DFLOW, (termiox->x_hflag & RTSXOFF));
		pciproc(tp, T_MODEM_CNTL);

		/* If we are doing CTS pacing and CTS is low, call SUSPEND */
		if ((tp->t_hw_flow_ctl & CTSXON) && !(tp->t_smcnl & MCTS))
			pciproc(tp, T_SUSPEND);

		splx(s);

		return(0);

      default:
	 /*
	  * Pass the ioctl command on through to the generic TTY ioctl
	  * handler.  If a non-zero value is returned, then call pciparam
	  * to force the configuration to be updated to the newly
	  * specified values.
	  */
	if ((err=ttiocom(tp, cmd, arg, mode)) == -1) {
		err = 0;
	    	pciparam(tp,NOHANGUP);
	}
	return(err);
   }
}

/* this is to get the tty pointer then call ttselect */
pci_select(dev, rw)
dev_t dev;
{
	return (ttselect(pci_tp, rw));
}

/*---------------------------------------------------------------------------
 | FUNCTION -           Set up pci according to ioctl args.
 |
 | MODIFICATION HISTORY
 |
 | Date       User      Changes
 | 19 Sep 90  jph       Fix for DTS INDaa08284. Added code to check for B0
 |                      on a non-modem port.  B0 causes a hangup on a modem
 |                      port, but is ignored on a non-modem port.  The new
 |                      'hangup' parameter is set if the ioctl set baud to B0.
 --------------------------------------------------------------------------*/

pciparam(tp,hangup)
register struct tty *tp;
register int hangup;
{
	register flags, lcl;

	flags = tp->t_cflag;
	if ((hangup) && (tp->t_open_type != TTY_OPEN)) {
		/* hang up line */
		ttymodem(tp, OFF);
		tp->t_state &= ~CARR_ON;
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
	pciinit(tp);
}


pci_intr(inf)
struct interrupt *inf;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Interrupt service routine.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 09 Oct 91	dmurray	Fix for INDaa09178.
 |			When there is a break condition, read data out of
 |			the input register and then OR it with FRERROR.
 |			This is how it is handled by the 98642 driver.
 |			Previously the data was not being read out until
 |			later causing an extra NULL byte in break sequences.
 |
 --------------------------------------------------------------------------*/

	register struct tty *tp = (struct tty *) inf->temp;
	register struct pci *pci = PORTADR;
	register unsigned char stat;
	register unsigned short val;

	tp->modem_status = 0;	/* for MTNOACTIVITY timeout */
	if (!(tp->t_state&(ISOPEN|WOPEN))) {
		tp->t_sicnl &= ~(TXINTEN|RXINTEN); /* no ints until open */
		pciinit(tp);
	}
	while (1) {
		stat = (unsigned char)pci->pciistat;
		if ((stat&0x1)!=0) return;
		switch ((stat>>1)&0x3) {
		case 1: /* transmitter register empty */
			sw_trigger(&tp->xmit_intloc,pcixmit,tp,0,tp->t_int_lvl);
			continue;
		case 0: /* modem status */
			if (pci->pcimstat & 0x04) { /* trailing edge of RI ? */
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
			stat = (unsigned short)((unsigned char)pci->pcilstat);	/* read and clear */
			val = (unsigned short)((unsigned char)pci->pci_data);
			if (stat&BRKBIT) 
			{
				if (call_kdb_on_break == 0xDEADBEEF)
					call_kdb();
				val |= FRERROR;
			} else {
				if (stat&FRMERR) 
					val |= FRERROR;
				if (stat&OVERR) 
					val |= OVERRUN;
				if (stat&PARERR) 
					val |= PERROR;
			}
			/* Fetch data and status */
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
	        	while (pci->pcilstat & 0x1) {
				val = (unsigned short)((unsigned char)pci->pci_data);
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
			sw_trigger(&tp->rcv_intloc,pcircv,tp,0,tp->t_int_lvl);
		}
	}
}

pcircv(tp)
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

int
pcisend(tp)
register struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		This routine loads the transmit buffer or fifo
 |			with outbound data.
 |
 | PARAMETERS -
 |	tp		Pointer to tty structure.
 |  
 | NOTES -		Originally CTS was checked as the fifo was being
 |			filled when CTS pacing was being done.  This was
 |			an attempt to stop transmission as quickly as
 |			possible when CTS was de-asserted.  Actually this
 |			was only partly successful because this only covered
 |			the window of time when the fifo was being filled.
 |			If CTS was de-asserted after the fifo was filled,
 |			it would still all to transmitted.  As a result
 |			doing CTS checking here did not help with the
 |			maximum sent after CTS goes down.  Since it was
 |			judged to not be worth the significant added
 |			overhead, the check was removed.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 19 Feb 92	dmurray	Removed check for CTS state.
 |
 --------------------------------------------------------------------------*/
	register struct pci *pci = PORTADR;

	if (((tp->t_hardware & FIFOS_ENABLED) == 0) || 
            (tp->t_tx_count >= tp->t_tx_limit)) {
		if ((pci->pcilstat & THRE) == 0)
			return (0) ;
		else
			tp->t_tx_count = 0;
	}

	if (tp->t_state & (TTXON|TTXOFF|TTSTOP)) {
		if (tp->t_state & TTXON) {
			pci->pci_data = CSTART;
			tp->t_tx_count++;
			tp->t_state &= ~TTXON;
			return (0) ;
		}
		if (tp->t_state & TTXOFF) {
			pci->pci_data = CSTOP;
			tp->t_tx_count++;
			tp->t_state &= ~TTXOFF;
			return (0) ;
		}
		if (tp->t_state & TTSTOP)  /* stop sending ? */
			return (0) ;
	}
	if (tp->t_state & BUSY) {
		pci->pci_data = (char)tp->t_buf;
		tp->t_tx_count++;
		tp->t_state &= ~BUSY;
		wakeup((caddr_t)(&tp->t_state));
		return (1) ;
	}
}


pcixmit(tp)
register struct tty *tp;
{
	int	s;

	s = splx(tp->t_int_lvl);
	/* Indicate that sw trigger has been disarmed */
	if (pcisend(tp) == 0) {
		splx(s);
		return;
	}

	splx(s);
	pciproc(tp, T_OUTPUT);
}

pciproc(tp, cmd, data)

register struct tty	*tp;
int			cmd;
int			data;

{
/*----------------------------------------------------------------------
 | FUNCTION -		This routine is called by the line discipline
 |			et.al. to do various tasks determined by "cmd".
 |
 | PARAMETERS PASSED -
 |      tp		tty structure pointer.
 |
 |	cmd		function for pciproc to perform.
 |
 | GLOBALS REFERENCED -
 |
 | WARNINGS -
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 11 Sep 91	dmurray	Fix for DTS INDaa10025.
 |			Deleted code in T_BLOCK and T_UNBLOCK which disables
 |			and enables THRE interrupts.
 |
 | 30 Jan 92	dmurray	+ Changed T_BLOCK and T_UNBLOCK to do RTS flow control
 |			  also.
 |			+ Changed T_MODEM_CNTL to use RTS and RTS handshaking
 |			  under the new design.
 |
 | 20 Feb 92	dmurray	Fix for INDaa11446.
 |			spl so that a software trigger cannot interrupt
 |			us here.
 |
 | 21 May 92	dmurray	Added T_VHANGUP.
 |
 ---------------------------------------------------------------------*/
	register c;
	struct pci *pci;
	int	x;

	x = splx(tp->t_int_lvl);

	switch(cmd) {
	case T_TIME:
		tp->t_state &= ~TIMEOUT;
		tp->t_slcnl &= ~SETBRK;
		pciinit(tp);
		goto start;

	case T_WFLUSH:
	case T_RESUME:
		tp->t_state &= ~TTSTOP;
		pcisend(tp);
		goto start;

	case T_OUTPUT:
	start:
		if (tp->t_state&(TIMEOUT|TTSTOP|BUSY))
			break;
		while ((c=getc(&tp->t_outq)) >= 0) {
			tp->t_buf = (struct cblock *)c; /* use buf as temp */
			tp->t_state |= BUSY;
			if (!pcisend(tp)) break;
		}
		ttoutwakeup(tp); /* check for sleepers to wakeup */
		break;

	case T_SUSPEND:
		tp->t_state |= TTSTOP;
		break;

	case T_BLOCK:
		/* XON/XOFF flow control */
		if ((tp->t_iflag & IXOFF) || data) {
			tp->t_state &= ~TTXON;
			tp->t_state |= (TTXOFF|TBLOCK);
			pcisend(tp);
		}
		/*
		 * If we are doing RTS flow control and the hardware
		 * currently has control of RTS, then take control
		 * of RTS and set it low.
		 */
		if (tp->t_hw_flow_ctl & RTSXOFF) {
			tp->t_state |= TBLOCK;
			tp->t_hardware &= ~HARDWARE_HNDSHK_ENABLED;
			pciproc(tp, T_MODEM_CNTL);
		}
		break;

	case T_RFLUSH:
		pci_reset_rx_fifo(tp) ;
		if (!(tp->t_state&TBLOCK))
			break;
	case T_UNBLOCK:
		/* XON/XOFF flow control */
		if ((tp->t_iflag & IXOFF) || data) {
			tp->t_state &= ~(TTXOFF|TBLOCK);
			tp->t_state |= TTXON;
			pcisend(tp);
		}
		/*
		 * If we are doing RTS flow control and the driver
		 * currently has control of RTS, then return control
		 * of RTS to the hardware.
		 */
		if (tp->t_hw_flow_ctl & RTSXOFF) {
			tp->t_state &= ~TBLOCK;
			tp->t_hardware |= HARDWARE_HNDSHK_ENABLED;
			pciproc(tp, T_MODEM_CNTL);
		}
		break;

	case T_BREAK:
		tp->t_slcnl |= SETBRK;
		pciinit(tp);
		tp->t_state |= TIMEOUT;
		timeout(ttrstrt, tp, HZ/4, NULL);
		break;

	case T_PARM:
		pciparam(tp,NOHANGUP);
		break;

	case T_MODEM_CNTL:
		{
			register int smcnl = tp->t_smcnl;
			register int modem = 0;

			if (tp->t_hardware & HARDWARE_HNDSHK_ENABLED) {
				/* Enable HW RTS flow control */
				modem |= (PCI_OUT2 | PCI_RTS);
			} else {
				if ( !(tp->t_hw_flow_ctl & RTSXOFF) &&
				      (smcnl & MRTS)) {
					 modem |= PCI_RTS;
				}
			}
			if (smcnl & MDTR)	modem |= PCI_DTR;
			if (smcnl & MDRS)	modem |= PCI_DRS;
			PORTADR->pcimcntl = modem;
		}
		break;

	case T_MODEM_STAT:
		{
			/* read modem status */
			int modem = PORTADR->pcimstat;
			int smcnl = tp->t_smcnl;

			smcnl &= ~(CCITT_IN);
			if (modem & PCI_CTS)	smcnl |= MCTS;
			if (modem & PCI_DSR)	smcnl |= MDSR;
			if (modem & PCI_DCD)	smcnl |= MDCD;
			if (modem & PCI_RI )	smcnl |= MRI ;
			tp->t_smcnl = smcnl;
		}
		break;

	case T_VHANGUP:
		((struct pcihwdata *)tp->utility)->vhangup	= data;
		ttcontrol( tp, LDC_VHANGUP, data );
		ttycomn_control( tp, cmd, data );
		if (data == 0)
			break;
		wakeup( (caddr_t)(&tp->t_state) );
		break;
	}
	splx(x);
}

pciinit(tp)
register struct tty *tp;
{
/*----------------------------------------------------------------------
 | FUNCTION -		PCI initialization sequence from Signetics
 |			Bipolar/MOS Microprocessor Data Manual page 178.
 |			This routine should be called anytime one of the
 |			register is modified.
 |
 | PARAMETERS PASSED -
 |      tp		tty structure pointer.
 |
 | GLOBALS REFERENCED -
 |
 | WARNINGS -
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 30 Jan 92	dmurray	Removed code to turn on hardware handshaking.
 |			This is done elsewhere now.
 |
 ---------------------------------------------------------------------*/
	short s;
	register i;
	register struct pci *addr = PORTADR;

	s = splx(tp->t_int_lvl);		/* if lower, init won't work! */
	/*
	** If we have a NS16550A UART then enable its alternate high
	** speed clock for BAUD rate above 19200 and disable otherwise.
	**
	**	RPC, Aug 4, 1994: Use the high speed clock above 38400
	**	baud. At 38400 baud, the divisor for the slow speed
	**	is exactly 4. There is some error in the divisor for the
	**	high speed clock at 38400 baud resulting in a higher
	**	error rate.
	*/
	if (tp->t_hardware & HIGHSPEED_CLOCK) {
		enable_hidden_mode(tp->t_isc_num);
		if ((tp->t_cflag & CBAUD) > B38400) {
			*((char *)(tp->t_card_ptr+0x01)) |= PCI_HSCLK;
			tp->t_hardware |= HIGHSPEED_CLOCK_ENABLED;
			i = highspeed_baudmap[tp->t_cflag&CBAUD];
		} else {
			*((char *)(tp->t_card_ptr+0x01)) &= ~PCI_HSCLK;
			tp->t_hardware &= ~HIGHSPEED_CLOCK_ENABLED;
			i = baudmap[tp->t_cflag&CBAUD];
		}
		disable_hidden_mode(tp->t_isc_num);
	} else
		i = baudmap[tp->t_cflag&CBAUD];
	addr->pcilcntl = DLAB | tp->t_slcnl;	/* access the baud registers */
	addr->baudhi = i>>8;
	addr->baudlo = i & 0xff;
	addr->pcilcntl = tp->t_slcnl;	/* line control */
	addr->pciicntl = tp->t_sicnl;	/* interrupt control */

	tty_delta_CLOCAL(tp);

	*((short *)(tp->t_card_ptr+0x02)) = INTRENAB;	/* interupt enab */
	splx(s);
}

/*************************************************************************
** Console Interface Routines:						**
**									**
** pciputchar(c)		Print a char - used by Kernel printf.	**
** pcioutnow(c)			Print the character now.		**
** pci_who_init(n)		Set-up Line characteristics for console.**
*************************************************************************/

pciputchar(c)
register ushort c;
{
	if (c == 0) return;
	if (c == '\n') pcioutnow('\r');
	pcioutnow(c);
}


pcioutnow(c)
register ushort c;
{
	register struct tty *tp = cons_tty;
	register struct pci *pci = PORTADR;
	register x;

	/* Don't let ANYONE in until I output this character! */
	x = spl6();
	/* Wait for Transmitter Holding Register to Empty. */
	while ((pci->pcilstat&THRE)==0)
		;
	pci->pci_data = (char)c;
	splx(x);	
}

/* Set up Line characteristics for console. */
pci_who_init(tp, tty_num, sc)
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

	/* Get Card Start address */
	addr = tp->t_card_ptr;
	/* reset intfc, clear UART */
	*((char *) addr+1) = 0xFF;
	snooze(50);		/* Wait at least 50 microseconds. */
	*((char *) addr+1) = 0x00;
	snooze(50);		/* Wait at least 50 microseconds. */
	if (((*(addr+1)) & 0x7f) == ID626) {	/* this is a 98626 card */
		/* Set Baud Rate */
		i = (int)(*(addr+5)&0xF);/* Read 4 bit baud selector at ID+4 */
		tp->t_cflag |= baudmap2[i];

		/* Set Line Characteristics */
		i = (int)(*(addr+7)&0x3F);/* Read 6 bit baud selector at ID+6 */
		tp->t_cflag |= ((i&0x3)<<5);	/* Set two CSIZE bits */
		if (i&0x8) {	/* If Parity Enabled */
			tp->t_cflag |= PARENB;
			if (i&0x10)	/* If Even Parity */
				tp->t_cflag |= PARODD;
		} else
			tp->t_cflag &= ~PARENB;
		if (i&0x4) {	/* If Two Stop Bits */
			tp->t_cflag |= CSTOPB;
		}

		pciparam(tp,NOHANGUP);
		printf("\n    System Console is 98626 at select code %d\n",sc);
	}
	else {	/* 98644 */
		/* Set Baud Rate to 9600 */
		tp->t_cflag |= B9600;
	
		/* Set Line Characteristics */
		tp->t_cflag |= CS8;	/* set character to 8 bits */
		tp->t_cflag &= ~PARENB;	/* no parity */

		pciparam(tp,NOHANGUP);
		printf("\n    System Console is 98644 at select code %d\n",sc);
	}
}

/*---------------------------------------------------------------------------
 | FUNCTION -		Perform initialization at bootup.
 |
 | PARAMETERS PASSED -
 |
 | MODIFICATION HISTORY
 |
 | Date        	User	Changes
 | 12 April 91	dmurray	Fix for DTS INDaa09206.
 |			Changed code to not configure card into system if
 |			its interrupt level is not set to 5.  Before this
 |			causing panics during configuration.  This is
 |			reverting back to 7.0 behavior.
 |
 | 17 May   91	dmurray	Fix for DTS INDaa09526.
 |			Added LOG_IO_OFFSET to address for apollo utility
 |			chip when we are turning off modem outputs to free
 |			SC9 DTR.  This was needed in 8.0 since logical
 |			addressing != physical.
 |
 | 08 Apr   92	dmurray	Added initialization of isc->my_isc for ioscan.
 |
 | 18 May   92  jlau	Added code to intialize the newly added open_count
 |			field in the pcihwdata structure.
 --------------------------------------------------------------------------*/
struct tty *pci_pwr(isc_num, addr, id, il, cs, pci_total)
char *addr;
int *cs;
int *pci_total;
{
	register struct tty *tp;
	register struct isc_table_type *isc;
	extern struct tty_driver sio626_driver;

	/* id must equal ID626 or ID644 */
	/* Only allow cards at level 5 */
/*
	if (addr == (char *)0x690000 && (port9 == 0))
		return; / temp !!!! for debugger!!!!! /
*/
	if (il != 5)
		return;

	if ((id != ID626) && (id != ID644))
		return;

	/* allocate isc and tp stuff */
	isc = (struct isc_table_type *)calloc(sizeof(struct isc_table_type));
	isc_table[isc_num] = isc;
	isc->my_isc = isc_num;
	if (id == ID644)
		isc->card_type = HP98644;
	else
		isc->card_type = HP98626;
	isc->card_ptr = (int *)addr;
	isc->int_lvl = il;
	/* for indirect calls */
	isc->tty_routine = (struct tty_drivercp *)&sio626_driver;

	/* make tty structure now */
	tp = (struct tty *)calloc(sizeof(struct tty));
	(struct tty *)(isc->ppoll_f) = tp;	/* and save it */
	(struct pcihwdata *)tp->utility =
			 (struct pcihwdata *)calloc(sizeof(struct pcihwdata));
	((struct pcihwdata *)(tp->utility))->fileopen = FALSE;

	((struct pcihwdata *)(tp->utility))->open_count = 0;

	tp->t_proc = pciproc;			/* for ttyflush */
	/* Set type field */
	tp->t_drvtype = &sio626_driver;
	/* Set up interrupt service routine for it. */
	isrlink(pci_intr, il, 3 + (int)addr, 0xc0, 0xc0, 0, tp);
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
	tp->t_cproc = NULL;
	tp->t_hw_flow_ctl = 0;
	tp->t_open_sema	= 0;

	/* Check for remote console */
	if (!remote_console) {
		/* Check for remote bit. */
		if (*(addr+1)&0x80) {
			*cs = 1;
			cons_tty = tp;
			remote_console = TRUE;
			/* make console dev number */
			cons_dev = (dev_t)((isc_num << 16) + 0x04);
		}
	}
	/* Increment to next location */
	(*pci_total)++;
/*
 * Turn on modem interrupts.  Since *tp may be sc 9 (KDB/KCDB port), set
 * baud rate and character size to what KDB/KCDB expect.
 */
	tp->t_cflag |= B9600;		/* initialize baud rate divisor */
	tp->t_cflag |= CS8;		/* set character to 8 bits */
	tp->t_sicnl = MSINTEN;		/* enable modem interrupts forever */

	/* save the select code for possible hidden mode operations */
	tp->t_isc_num = isc_num;

	/*
	** If we have an NS16550A UART then enable its fifos
	** and check for the presence of fancy hardware.
	*/
	tp->t_hardware = 0;
	if (hidden_mode_exists(isc_num)) {
		tp->t_hardware = HIDDENMODE | FIFO_MODE | 
					HIGHSPEED_CLOCK | HARDWARE_HANDSHAKE;
	}

	/* turn off modem outputs on port 1 if apollo utility chip */
	if ( testr( ((char *)(0x41c000 + LOG_IO_OFFSET)), 1) == TRUE)
		*((char *)(0x41c030 + LOG_IO_OFFSET)) = 0;

	pciparam(tp,NOHANGUP);
	pci_reset_rx_fifo(tp) ;
	pci_reset_tx_fifo(tp) ;
}

pci_wait(tp)
struct tty	*	tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Wait for output to drain from the harwdare.
 |
 | MODIFICATION HISTORY
 |
 | Date        	User	Changes
 | 23 Apr 92	dmurray	Created routine to fix INDaa11823.
 |
 | 21 May 92	dmurray	Added check for vhangup and PCATCH to sleep.
 |
 --------------------------------------------------------------------------*/

	int	s;

	s = splx (tp->t_int_lvl);
	/* If there is something in the tbuf, wait for it to get to the chip. */
	if (tp->t_state & BUSY) {
		if (sleep( (caddr_t)(&tp->t_state), TTOPRI| PCATCH )) {
			splx(s);
			return(EINTR);
		}
		if (((struct pcihwdata *)tp->utility)->vhangup) {
			splx(s);
			return(EBADF);
		}
	}

	/* Wait until the shift register is emptied. */
	while ((PORTADR->pcilstat & TEMT) == 0)
		delay(HZ/20);

	splx(s);
	return(0);
}

extern ttwrite(),ttread();

struct tty_driver sio626_driver = {
SIO626, pci_open, pci_close, pci_read, pci_write, pci_ioctl, pci_select,
	pciputchar, pci_wait, pci_pwr, pci_who_init, 0
};

/*
** linking and inititialization routines
*/

extern int (*make_entry)();

int (*sio626_saved_make_entry)();

/*---------------------------------------------------------------------------
 | FUNCTION -		Claim the card if it a 98626 or 98644.
 |
 | PARAMETERS PASSED -
 | int id			Card id.
 | struct isc_table_type *isc	Pointer to isc table.
 |
 |
 | MODIFICATION HISTORY
 |
 | Date        	User	Changes
 | 12 April 91	dmurray	Fix for DTS INDaa09206.
 |			Changed code to not configure card into system if
 |			its interrupt level is not set to 5.  Before this
 |			causing panics during configuration.  This is
 |			reverting back to 7.0 behavior.
 |
 | 17 Jan   92	dmurray	Added changes for DDB to do different io_inform
 |			so that DDB can be used on select code 9.
 |
 --------------------------------------------------------------------------*/

sio626_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	char *card_name;
	int advanced_hw = 0;
	switch (id) {
case ID626:	card_name = "HP98626 RS-232C Serial Interface";
		break;
case ID644:	
		if (hidden_mode_exists(isc->my_isc)) {
			card_name = "HP98644 Advanced RS-232C Serial Interface";
			advanced_hw = 1;
		} else
			card_name = "HP98644 RS-232C Serial Interface";
		break;
	default:
		return(*sio626_saved_make_entry)(id, isc);
	}
#ifdef DDB
	if ((ddb_boot) && (isc->my_isc == 9))
		io_inform(card_name, isc, -3);
	else {
#endif /* DDB */
		io_inform(card_name, isc, 5);

		if (advanced_hw) {
			msg_printf("        With 16 byte rcv fifo, 16 byte xmit fifo, hardware handshake\n");
			msg_printf("        and high speed clock.\n");
		}

#ifdef DDB
	}
#endif /* DDB */

	return FALSE;
}

sio626_link()
{
	extern struct tty_driver *tty_list;

	sio626_saved_make_entry = make_entry;
	make_entry = sio626_make_entry;

	sio626_driver.next = tty_list;
	tty_list = &sio626_driver;
}

/*---------------------------------------------------------------------------
 | FUNCTION -		Reset the receive FIFO
 |
 | MODIFICATION HISTORY
 |
 | Date        	User	Changes
 | 17 August 94 rpc	Created to provide method for reseting the receive
 |			FIFO. Reseting was being done in pciparam. This caused
 |			both receive and xmit FIFOs to be reset on any ioctl()
 |			call. This caused data loss. See defect #FSDdt09040
 |			for more info.
 |
 --------------------------------------------------------------------------*/

static void
pci_reset_rx_fifo(tp)
register struct tty *tp;
{
	register struct pci *pci = PORTADR;
	if (tp->t_hardware & FIFO_MODE) 
            pci->pciistat =
                (tp->t_dev & RECEIVE_TRIGGER) | FIFO_ENABLE | RX_FIFO_RESET;
}


/*---------------------------------------------------------------------------
 | FUNCTION -		Reset the xmit FIFO
 |
 | MODIFICATION HISTORY
 |
 | Date        	User	Changes
 | 17 August 94 rpc	Created to provide method for reseting the xmit
 |			FIFO. Reseting was being done in pciparam. This caused
 |			both receive and xmit FIFOs to be reset on any ioctl()
 |			call. This caused data loss. See defect #FSDdt09040
 |			for more info.
 |
 --------------------------------------------------------------------------*/

static void
pci_reset_tx_fifo(tp)
register struct tty *tp;
{
	register struct pci *pci = PORTADR;
	if (tp->t_hardware & FIFO_MODE) 
            pci->pciistat =
                (tp->t_dev & RECEIVE_TRIGGER) | FIFO_ENABLE | TX_FIFO_RESET;
}
