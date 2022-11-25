/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/mux.c,v $
 * $Revision: 1.7.84.6 $	$Author: rpc $
 * $State: Exp $   	$Locker: rpc $
 * $Date: 93/12/16 09:35:45 $
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
	MUX driver
*/

#include "../h/param.h"		/* various types and macros */
#include "../h/errno.h"
#include "../h/file.h"
#include "../h/tty.h"		/* tty structure and parameters */
#include "../h/conf.h"		/* linesw stuff */
#include "../wsio/intrpt.h"
#include "../wsio/hpibio.h"
#define _INCLUDE_TERMIO
#include "../s200io/mux.h"
#include "../h/termios.h"
#include "../h/modem.h"
#include "../h/ld_dvr.h"

/* MUX shared memory definitions  See 98642-90001 for details */

/* per-card shared memory definition */

struct mux_io_0k {		/* 0..0x7FFF I/O space */
	u_char	x0;
	u_char	reset;		/* 0x0001 ... write = reset, read = id */
	u_char	x2;
	u_char	interrupt;	/* 0x0003 */
	u_char	x4;
	u_char	semaphore;	/* 0x0005 */
	u_char	xa[0x8000 - 0x0005 -1];
};

struct modem {			/* modem control structure */
	u_char	x0;
	u_char	in;
	u_char	x2;
	u_char	out;
	u_char	x4;
	u_char	mask;
};

struct mux_io_8k {		/* 0x8000..0xFFFF I/O space */
	u_char	x8000;
	u_char	int_cond;	/* 0x8001 */
	u_char	x8002;
	u_char	command;	/* 0x8003 */
	u_char	xa[0x8c01 - 0x8003 - 1];
	u_char	bitmap[256*2];	/* 0x8c01 */
	u_char	xb[0x8e30 - 0x8c01 - 256*2];
	struct	modem modem0;	/* 0x8e30 - 0x8e35 */
	u_char	xc[0x8e47 - 0x8e35 - 1];
	u_char	st_cond;	/* 0x8e47 */
	struct	modem modem1;	/* 0x8e48 - 0x8e4d */
	struct	modem modem2;	/* 0x8e4e - 0x8e53 */
	struct	modem modem3;	/* 0x8e54 - 0x8e59 */
	u_char	x8e5a;
	u_char	mcntl_reg;	/* 0x8e5b */
	u_char	x8e5c;
	u_char	mstat_reg;	/* 0x8e5d */
	u_char	xd[0xffff - 0x8e5d];
};

/*
 * 15 May 92    jlau
 *
 * We needed to add a new field (i.e. open_count for successful open)
 * in the hardware structure which tp->utility points to.  However,
 * we could not simply add it to the hardware structure (i.e. mux_io_8k) 
 * directly because that will cause a mismatch between the SW structue
 * and IO hardware layout.  Instead, we created a new structure (called
 * mux_hw_data) to store the open_count and a pointer to the the mux_io_8k 
 * structure.  Now, the tp->utility will point to the this new structure 
 * and any reference to the mux_io_8K structure will be accessed
 * via tp->utility->mux_io_ptr istead. 
 *
 * WARNING: the common code (i.e. ttycomn.c) shares by the the pdi.c
 *          the pci.c, the apollo_pci.c and the mux.c drivers assumes
 *          that the tp->utility points to the open_count. Therefore,
 *          do NOT move the location of the open_count when new fields
 *          need to be added to the HW data structure.
 */
struct mux_hw_data {
	u_int				open_count;	/* this field must be */
							/* the 1st one	      */
	u_int				vhangup;
	struct mux_io_8k	*	mux_io_ptr;
};

/* Temporary until a function code can be pt into tty.h */
#define	T_VHANGUP	-1

struct mux_port_io {		/* per-port I/O space:
					base_addr = 0x8e01 + (port*2) */
	u_char	rhead;		/* 0x8e01 */
	u_char	xa[0x8e09 - 0x8e01 - 1];
	u_char	rtail;		/* 0x8e09 */
	u_char	xb[0x8e11 - 0x8e09 - 1];
	u_char	thead;		/* 0x8e11 */
	u_char	xc[0x8e19 - 0x8e11 - 1];
	u_char	ttail;		/* 0x8e19 */
	u_char	xd[0x8e37 - 0x8e19 - 1];
	u_char	cmnd_tab;	/* 0x8e37 */
	u_char	xe[0x8e3f - 0x8e37 - 1];
	u_char	icr_tab;	/* 0x8e3f */
};

/* per-port macros */

#define TBUFF(port)	(0x8f41 - (port<<5))
#define BD(port)	(0x8e23 + (port<<2))

#undef	M_PARITY_ODD
#undef	M_PARITY_EVEN
#undef	M_TWO_STOP_BITS
/* addr0k + CONFG(port)		Configuration masks */
#define CONFG(port)	(0x8e21 + (port<<2))
#define	M_PARITY_ODD	0x01
#define	M_PARITY_EVEN	0x02
#define	M_TWO_STOP_BITS	0x08
#define	M_6_BITS	0x10
#define	M_7_BITS	0x20
#define	M_8_BITS	0x30
#define HWCSIZE		0x30

/* addr0k + RBUFF(port)		Character status masks */
#define RBUFF(port)	(0x8a01 - (port<<9))
#define	STATUS_MASK	0xf8
#define	FRAME_ERROR	0x80
#define	OVERRUN_ERROR	0x40
#define	PARITY_ERROR	0x20
#define	BREAK_DETECT	0x10
#define	BUFFER_OVERFLOW	0x08

/* addr->modm_out   - Modem Control masks */
#define	MUX_RTS		0x01
#define	MUX_DTR		0x02
#define	MUX_DRS		0x04

/* addr->modm_in    - Modem Status masks */
#define	MUX_RI		0x01
#define	MUX_DCD		0x02
#define	MUX_DSR		0x04
#define	MUX_CTS		0x08

/* Interrupt Service Routine structures */

struct mux_interrupt {			/* linked via sc_int(isc_num) */
	struct tty *tp[4];		/* all the tty structures */
	long	last_rxint;
};

/* other nifty macro's */

#define get_semaphore(addr)	while ((addr)->semaphore & 0x80)
#define clear_semaphore(addr)	(addr)->semaphore = 0
#define tx_size(port)		(((port)->ttail - (port)->thead) & 0x0f)
#define rx_size(port)		(((port)->rtail - (port)->rhead) & 0xff)
#define sc_int(sc)		((struct mux_interrupt *)isc_table[sc]->b_actf)

/* these macro's re-define unused areas of the tty structure */
/* these all assume sizeof(*char) == sizeof(*anything_else)  */

#define tp_addr0(tp)		((struct mux_io_0k *)	(tp)->t_card_ptr)
#define tp_addrp(tp)		((struct mux_port_io *)	(tp)->t_inbuf)

/* 
 * 15 May 92	jlau
 *
 * Changed the tp_addr8(tp) macro to refect the additon of open_count.
 * See comments above regarding the way  struct mux_io_8k is now accessed 
 * via the tp->utlity->mux_io_ptr indirection.
 */
#define tp_addr8(tp)	(((struct mux_hw_data *) (tp->utility))->mux_io_ptr)

#define tp_tx_base(tp)		((u_char *)		(tp)->t_in_head)
#define tp_rx_base(tp)		((u_char *)		(tp)->t_in_tail)
#define tp_port_number(tp)	(			(tp)->t_sicnl)
#define tp_tx_limit(tp)		(			(tp)->t_in_count)

extern struct tty *cons_tty;
extern char remote_console;
extern dev_t cons_dev;
extern struct linesw linesw[];
extern lbolt;

/* table that maps unix symbolic baud values to uart baud values */

short muxbaudmap[] = {	/* For reading unix config */
		0x0000,	/* hangup */
		0x0001,	/* 50 baud */
		0x0002,	/* 75 baud */
		0x0003,	/* 110 baud */
		0x0004,	/* 134.5 baud */
		0x0005,	/* 150 baud */
		0x0005,	/* 200 baud - not available, use 150 baud */
		0x0006,	/* 300 baud */
		0x0007,	/* 600 baud */
		0x0008,	/* 900 baud */
		0x0009,	/* 1200 baud */
		0x000a,	/* 1800 baud */
		0x000b,	/* 2400 baud */
		0x000c, /* 3600 baud */
		0x000d,	/* 4800 baud */
		0x000e,	/* 7200 baud */
		0x000f, /* 9600 baud */
		0x0010,	/* 19200 baud */
		0x0011,	/* 38400 baud */
};
/* table that maps uart baud values to unix symbolic baud values */

short unixbaudmap[] = {	/* For reading mux config */
		B0,	/* hangup */
		B50,	/* 50 baud */
		B75,	/* 75 baud */
		B110,	/* 110 baud */
		B134.5,	/* 134.5 baud */
		B150,	/* 150 baud */
		B300,	/* 300 baud */
		B600,	/* 600 baud */
		B900,	/* 900 baud */
		B1200,	/* 1200 baud */
		B1800,	/* 1800 baud */
		B2400,	/* 2400 baud */
		B3600,  /* 3600 baud */
		B4800,	/* 4800 baud */
		B7200,	/* 7200 baud */
		B9600,  /* 9600 baud */
		B19200,	/* 19200 baud */
		B38400,	/* 38400 baud */
};

#define CCITT_IN	(MDCD | MCTS | MDSR | MRI)
#define CCITT_OUT	(MDTR | MRTS | MDRS)

#define MUX_INT_LVL	3		/* mux card interrupt level */

/****************************************************************************
 * routine to get the tty pointer
 ****************************************************************************/

struct tty *
mux_get_tp(dev)

dev_t	dev;
{
	register unsigned sc = m_selcode(dev);
	register unsigned port = m_port(dev);

	if (port > 3) {			/* for 8-port MUX */
		sc++;
		port -= 4;
	}
	if (port > 3)
		return((struct tty *) 0);
	if (sc > 31)
		return((struct tty *) 0);
	if (isc_table[sc] == (struct isc_table_type *) 0)
		return((struct tty *) 0);
	if (isc_table[sc]->card_type != HP98642)
		return((struct tty *) 0);
	return( sc_int(sc)->tp[port] );
}

/****************************************************************************
 * mux_open (dev, flag)   
 *
 *   dev_t dev - Describes which serial card will be opened.
 *   int flag - Specifies 'open' access modes.
 ****************************************************************************/

#define TXLIMIT	0x000030	/* minor number field */

u_char mux_tx_limit[4] = {16,8,4,1};	/* max chars sent after getting XOFF */

mux_open(dev, flag)

   dev_t  dev;
   int   flag;

{
	register struct tty *tp;
	register x;

	tp = mux_get_tp(dev);
	if (tp == (struct tty *) 0)
		return (ENXIO) ;

	x = splx(MUX_INT_LVL);
	/* Clear vhangup flag */
	muxproc( tp, T_VHANGUP, 0 );
	flag = ttycomn_open(dev, tp, flag);
	splx(x);
	if (flag == 0)
		tp_tx_limit(tp) = mux_tx_limit[((dev & TXLIMIT) >> 4)];
	return(flag);
}

/****************************************************************************
 * mux_close (dev)
 *
 *   dev_t dev - Describes which serial card and port is to be closed.
 *
 *   mux_close is called on each close call.
 ****************************************************************************/

mux_close(dev)

   dev_t  dev;
{
	int error;
	int x = splx(MUX_INT_LVL);

	error = ttycomn_close(dev, mux_get_tp(dev));
	splx(x);
	return(error);
}

/****************************************************************************
 * mux_read (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be read from.
 *
 *   mux_read is called each time a read request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY read routine.
 ****************************************************************************/

mux_read(dev, uio)

    dev_t dev;
{
	/* Call the generic TTY read routine */
	int error;
	int x = splx(MUX_INT_LVL);

	error = ttread (mux_get_tp(dev), uio);
	splx(x);
	return(error);
}

/****************************************************************************
 * mux_write (dev, uio)
 *
 *   dev_t dev - Describes which serial card is to be written to.
 *
 *   mux_write is called each time a write request is made for a specific
 *   serial card.  It, in turn, calls the generic TTY write routine.
 ****************************************************************************/

mux_write(dev, uio)

   dev_t  dev;
{
	/* Call the generic TTY write routine */
	int error;
	int x = splx(MUX_INT_LVL);

	error = ttwrite (mux_get_tp(dev), uio);
	splx(x);
	return(error);
}

/****************************************************************************
* mux_ioctl (dev, cmd, arg, mode)
*
*   long dev - Describes which serial card is to be affected.
*   int  cmd - Specifies the ioctl command being requested.
*   char *arg - Points to the passed-in parameter block.
*   int  mode - Specifies the current access modes.
*
*   mux_ioctl is called each time an ioctl request is made for a
*   specific serial card.  This entry point is used to perform
*   TTY specific functions.  If one of the special SERIAL commands
*   is given, then it will be handled here.  If one of the standard
*   TTY commands is given, then the generic TTY ioctl routine will
*   be called upon to handle it.
****************************************************************************/

mux_ioctl(dev, cmd, arg, mode)

   register dev_t  dev;
   register int   cmd;
   register int   *arg;
   int   mode;

{
/*---------------------------------------------------------------------------
 | FUNCTION -		Setup mux according to ioctl args. 
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 19 Sep 89  garth	Added code to return EINVAL if user attempts to
 |			set unsupported character sizes.  This behavior
 |			is currently required by POSIX.
 |			
 --------------------------------------------------------------------------*/

   register struct tty *tp;		/* Ptr to tty structure */
   register int x;
   register err = 0;

   tp = mux_get_tp(dev);		/* get the tty pointer */
   x = splx(MUX_INT_LVL);
   /*
    * Check the requested ioctl command, and if it is one of the
    * new TTY commands, then process it here; else, call the
    * generic TTY ioctl routine.
    */
   switch(cmd) {

      case MCGETA:
	 /* Return the current state of the modem lines */
	 muxproc(tp, T_MODEM_STAT);
	 *arg = tp->t_smcnl;
	 break;

      case MCSETAW:
      case MCSETAF:
	 /* Wait for output to drain */
	 err = ttywait (tp);
	 if (err != 0)
		break ;
	 if (cmd == MCSETAF)
	    ttyflush (tp, (FREAD | FWRITE));

      case MCSETA:
	 /* Set the modem control lines */
	 if (tp->t_state & SIMPLE) {
		tp->t_smcnl &= ~(CCITT_OUT);	/* clear shadow control bits */
		tp->t_smcnl |= (*arg & CCITT_OUT); /* set according to user */
		muxproc(tp, T_MODEM_CNTL);	/* set mux control lines */
	 }
	 break;

      case MCGETT:
	 /* Return the current timer settings */
	 bcopy(tp->timers, arg, sizeof(struct mtimer));
	 break;

      case MCSETT:
	 /* Set the timers */
	 bcopy(arg, tp->timers, sizeof(struct mtimer));
	 break;

      case TCSETA:
      case TCSETAW:
      case TCSETAF:
      case TCSETATTR:
      case TCSETATTRD:
      case TCSETATTRF:
	if ((err=ttiocom(tp, cmd, arg, mode)) == -1) {
		err=muxparam(tp);
	}
	break;

      default:
	 /*
	  * Pass the ioctl command on through to the generic TTY ioctl
	  * handler.  If a non-zero value is returned, then call muxparam
	  * to force the configuration to be updated to the newly
	  * specified values.
	  */
	if ((err=ttiocom(tp, cmd, arg, mode)) == -1) {
	    	err=muxparam(tp);
	}
	break;
   }
   splx(x);
   return(err);
}

/****************************************************************************
 * this gets the tty pointer then calls ttselect
 ****************************************************************************/

mux_select(dev, rw)
dev_t dev;
{
	int error;
	int x = splx(MUX_INT_LVL);

	error = ttselect(mux_get_tp(dev), rw);
	splx(x);
	return(error);
}

/****************************************************************************
  muxparam -	Set up mux according to ioctl args.
 ****************************************************************************/

muxparam(tp)
register struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Setup mux according to ioctl args. 
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 11 Sep 89  garth	Added code to prevent user from setting character
 |			sizes of 5 or 6 bits.  The 98642 does not support
 |			character sizes of 5 or 6 bits.
 |
 | 19 Sep 89  garth	Added code to return EINVAL if user attempts to
 |			set unsupported character sizes.  This behavior
 |			is currently required by POSIX.
 |
 | 22 Feb 90  garth	Added code to ignore baud rate changes that are
 |			not supported.  This behavior is currently required
 |			by POSIX.
 |			
 | 19 Sep 90  jph	FIX for DTS INDaa08284. Added code to ignore baud 
 |			rate B0 when set on a non-modem port.
 --------------------------------------------------------------------------*/
	register int x, i, flags, hwcsize;
	int	err = 0, temp;
	u_char *addr;

	if ((((flags = tp->t_cflag) & CBAUD) == 0) && 
		(tp->t_open_type != TTY_OPEN)) {
		ttymodem(tp, OFF);		/* hang up line */
		tp->t_state &= ~CARR_ON;
	}

	i = tp_port_number(tp);

	x = 0;				/* -PARENB, -CSTOPB, CS5 */
	if (flags & PARENB)		/* parity */
		if (flags & PARODD)
			x |= M_PARITY_ODD;
		else
			x |= M_PARITY_EVEN;
	if (flags & CSTOPB)		/* stop bits */
		x |= M_TWO_STOP_BITS;
	flags &= CSIZE;			/* bits per character */
	addr = (u_char *) tp->t_card_ptr + CONFG(i);
	if (flags == CS5 || flags == CS6) {
		/* Don't try to set character sizes of 5
 		 * or 6 bits.  The 98642 mux does not support
		 * these character sizes.
		 * Make sure that Hardware CSIZE matches
		 * line discipline CSIZE.
		 * Set error code to EINVAL.  This is currently
		 * required by POSIX.
		 */
		hwcsize = (*addr & HWCSIZE);
		flags = (hwcsize == M_7_BITS ? CS7 : CS8);
		tp->t_cflag &= ~CSIZE;
		tp->t_cflag |= flags;
		err = EINVAL;
	}
	if (flags == CS7)	x |= M_7_BITS;
	if (flags == CS8)	x |= M_8_BITS;

	*addr = x;			/* set configuration */
	addr += 2;			/* point to BD(i) */
	temp = tp->t_cflag & CBAUD;
	if ((temp != B200) && (temp != B0) && (temp <= B19200)) {
		*addr = muxbaudmap[tp->t_cflag & CBAUD];/* set baud rate */
	} else {
		/* Make sure HW and SW settings match. */
		tp->t_cflag &= ~CBAUD;
		tp->t_cflag |= unixbaudmap[*addr];
	}
	mux_interrupt(tp, i, 0x01);
	muxproc(tp, T_MODEM_CNTL);
	tty_delta_CLOCAL(tp);
	return(err);
}

mux_interrupt(tp, cmd, mask)
register struct tty *tp;
register cmd;
{
/*----------------------------------------------------------------------
 | FUNCTION -	Tell the MUX to do something, depending on "cmd" and "mask"
 |
 | PARAMETERS PASSED -
 |	tp	mux or port to send cmd, which interrupts the MUX
 |	cmd	command to send to the mux.  If cmd <= 3, it is port-specific.
 |	mask	port-specific command, valid only if cmd <= 3.
 |
 | MODIFICATION HISTORY
 |
 |	Date		User		Changes
 |	20 Aug 90	dmurray		Fix for INDaa10000.
 |					Added snooze after aquiring semaphore
 |					so as not to write to the command
 |					register until after the RETI
 |					instruction on the Z-80.
 ---------------------------------------------------------------------*/

	get_semaphore(tp_addr0(tp));
	/*
	 * We need to dalay here so that we can avoid a timing window
	 * on the card.  We must not write to the command register
	 * during an RETI for a host ISR running on the card's Z-80.
	 * The time between giving up the semaphore and the RETI instruction
	 * is approx. 6 microsecands.  We must therefore spin for 10 usecs.
	 * since snooze is only accurate to within 4 usecs.
	 */
	snooze(10);
	if (cmd <= 3) {			/* port-specific command */
		tp_addrp(tp)->cmnd_tab |= mask;
		cmd = 1<<cmd;
	}
	tp_addr8(tp)->command |= cmd;	/* generates interrupt on MUX */
	clear_semaphore(tp_addr0(tp));
}

/****************************************************************************
 * (carefully) Remove characters from the MUX tx FIFO
 ****************************************************************************/

mux_tx_flush(tp)
struct tty *tp;
{
	register struct mux_port_io *addrp;
	register int tail_index;
	register x;

	addrp = tp_addrp(tp);
/*
 * Update MUX pointers to make MUX think it's empty.  Leave two bytes on the
 * MUX to avoid race with MUX generation of the TX BUFFER EMPTY interrupt.
 */
	x = spl6();			/* don't want to be slower than MUX */
	while (tx_size(addrp) > 2) {
		tail_index = addrp->ttail;
		tail_index--;
		tail_index &= 0xf;
		addrp->ttail = tail_index;
	}
	splx(x);
}

/****************************************************************************
 * Process a MUX interrupt.
 * If the receiver mode is non-optimal, change it.
 * Call mux_tx(), mux_rx(), or tty_modem_intr() as appropriate.
 ****************************************************************************/

mux_intr(inf)
struct interrupt *inf;
/*----------------------------------------------------------------------
 | FUNCTION -	Process a MUX interrupt.  If the receiver mode is non-optimal,
 |	 	change it. Call mux_tx(), mux_rx(), or tty_modem_intr() as
 |		appropriate.
 |
 | MODIFICATION HISTORY
 |
 |	Date		User		Changes
 |	11 Jun 92	jlau		Fix for INDaa12138.
 |					Added check for port open before
 |					processing interrupt status.
 ---------------------------------------------------------------------*/
{
	register struct mux_io_8k *addr8;	/* must be %a5 */
	register struct tty *tp;
	register struct mux_interrupt *mux;
	register icr_status, rx_status, modem_status, i;
	register u_char status, mask;

	mux = (struct mux_interrupt*) inf->temp;
	tp = mux->tp[0];
	addr8 = tp_addr8(tp);

	{
		register struct mux_io_0k *addr0 = tp_addr0(tp);

		get_semaphore(addr0);
		status = addr8->int_cond;	/* get mux status */
		addr8->int_cond = 0;		/* clear status */
		icr_status = mux_icr_status();	/* get & clear icr_tab's */
		rx_status = mux_rx_status();	/* get mux rx FIFO indices */
		modem_status = 0;		/* initialize for modem */
		if (status & 0x20) {		/* modem interrupt ? */
			if ((modem_status = addr8->mstat_reg) == 0)
				modem_status = 0x01;	/* 98642-compatible */
			addr8->mstat_reg = 0;	/* clear status */
		}
 		clear_semaphore(addr0);
	}

/*	if (status & 0x10)			/* self_test done ? */
/*		;				/* (not used) */

/****************************************************************************
 *   The MUX has two interrupt modes for receiving bytes.  For high volume,
 *   use the 60 Hz timer.  Otherwise, turn on the BITMAP to generate one
 *   interrupt per byte.  The general idea is to minimize the number of MUX
 *   interrupts on the 680x0 CPU.  The time required to switch modes is about
 *   five times longer than one non-productive timer interrupt.  We call
 *   these two modes "timer mode" and "byte mode".
 ****************************************************************************/

	i = lbolt; i -= mux->last_rxint;	/* i = time since last RX */
	if (rx_status)
		mux->last_rxint = lbolt;	/* got data, update */
    {
	register u_char *p;
	p = &addr8->bitmap[0];
	mask = *p;
	if (((status & 0x40) && (i >= 10))	/* too slow for timer mode ? */
	 || ((icr_status & (0x02 * 0x01010101)) && (i == 0)
		&& (mask != 0))) {		/* too fast for byte mode ? */

		mask ^= 0x0f;			/* toggle bitmap mask */
		for (i = 256; --i >= 0; ) {	/* unorthodox, but faster */
			*p = mask;
			p += 2;
		}
		if (mask == 0) {	/* DC1/DC3 always interrupts */
			mask = 0x0f;	/* for the compiler */
			addr8->bitmap[CSTART*2] = mask;
			addr8->bitmap[CSTOP *2] = mask;
			addr8->bitmap[(CSTART+128)*2] = mask;
			addr8->bitmap[(CSTOP+128) *2] = mask;
		}
/***************************************************************************
 * Performance note:
 *  
 * It may be better to use a UNIX timeout/sw_trigger for polling the MUX.
 * Additionally, the polling interval could be adjusted, depending on the
 * maximum baud rate of the MUX (e.g.  9600 baud requires a 100mS polling
 * interval, rather than the 16.7 mS you get by using the MUX driver).
 * Don't exceed 100mS, or echoing looks bad.
 ***************************************************************************/
		while (addr8->command & 0x20) /* wait for MUX to toggle timer */
			;
		mux_interrupt(mux->tp[0], 0x20, 0);	/* toggle MUX timer */
	}

    }
/* process the rx/tx queues.  To avoid a race condition, the rx		*/
/* queue MUST be examined after the bitmap is set (for byte mode).	*/

	rx_status = mux_rx_status();
	mask = 1<<(4-1);			/* mask = (1<<i) */
	for (i = 4; --i >= 0; ) {		/* because of icr/rx status */
		tp = mux->tp[i];

		/* Process receive & xmit status only if the port is open */ 
		if (tp->t_state & (ISOPEN|WOPEN)) {  
			if (rx_status & 0xff)	 /* receive FIFO not empty */
				mux_rx(tp);	 
						 /* xmit buffer empty */
			if ((status & mask) && (icr_status & 0x01)) { 
				mux_tx(tp);	 
				tp->t_int_flag &= ~PEND_TXINT;	
				ttoutwakeup(tp); /* wake up sleepers */
			}
		}

		/* Always process modem status regardless of open or not */ 
		if (modem_status & mask) 	 /* modem status changed */
			tty_modem_intr(tp);

		/* Look at status on the next port */
		rx_status >>= 8;
		icr_status >>= 8;
		mask >>= 1;
	}
}

/****************************************************************************
 * Get MUX interrupt status
 * These assembler routines assume that %a5 points to the top 8k of the
 * MUX card (the start of shared RAM).  They must be called while the
 * 680x0 has the MUX semaphore, or there will be a race between the MUX
 * firmware and the 680x0.
 ****************************************************************************/
mux_nop() {}			/* fool the compiler/optimizer */
asm("_mux_rx_status:		# xor the rx head & tail pointers
	movp.l	0xe01(%a5),%d0	# assumes addr8 in %a5 !!!!!
	movp.l	0xe09(%a5),%d1
	eor.l	%d1,%d0
	rts
_mux_icr_status:		# get the icr_tab status
	movp.l	0xe3f(%a5),%d0	# assumes addr8 in %a5 !!!!!
	movq	&0,%d1		# assumes we have the semaphore
	movp.l	%d1,0xe3f(%a5)	# clear status
	rts				");

#ifdef MUX_DEBUG
/***************************************************************************
 * dump MUX modem values to KDB tty
 ***************************************************************************/
mux_printf(addr)
register struct mux_io_8k *addr;
{
	extern int (*kdb_printf)();

	register i;
	register struct modem *mp;

	(*kdb_printf)("mux at %8x\n",addr);
	(*kdb_printf)("INT_COND (%8x) = %2x  MSTAT_REG (%8x) = %2x\n",
		&addr->int_cond, addr->int_cond,
		&addr->mstat_reg, addr->mstat_reg);
	(*kdb_printf)("COMMAND  (%8x) = %2x  MCNTL_REG (%8x) = %2x\n",
		&addr->command, addr->command,
		&addr->mcntl_reg , addr->mcntl_reg);
	for (i = 0; i < 4; i++) {
		switch (i) {
		case 0:	mp = &addr->modem0; break;
		case 1:	mp = &addr->modem1; break;
		case 2:	mp = &addr->modem2; break;
		case 3:	mp = &addr->modem3; break;
		}
		(*kdb_printf)("port %1d (%8x) IN OUT MASK = %2x %2x %2x\n",
			i, mp,
			mp->in, mp->out, mp->mask);
	}
}
#endif /* MUX_DEBUG */

/****************************************************************************
 * Drain the MUX receiver FIFO (up to 128 bytes), sending data to the
 * line discipline routine (e.g. ttin()).
 ****************************************************************************/

mux_rx(tp)
register struct tty *tp;
{
	register u_char *buff_ptr;
	register struct mux_port_io *addrp;
	register (*line_input)();
	register head_index, put_byte, status;
	register long   c;

	addrp = tp_addrp(tp);
	head_index = addrp->rhead;	/* get current index from MUX */
	buff_ptr = tp_rx_base(tp);
	buff_ptr += head_index; buff_ptr += head_index;	/* twice for DIO */
	put_byte = ((tp->t_cflag & CREAD) && (tp->t_state & CARR_ON));
	line_input = linesw[(short)tp->t_line].l_input;/* performance */

	while (head_index != addrp->rtail) {
		c = *(buff_ptr++);	/* get character off MUX */
		buff_ptr++;		/* next odd byte in I/O space */
		status = *(buff_ptr++);	/* get status off MUX */
		buff_ptr++;		/* next odd byte in I/O space */
		head_index +=2;		/* update head_index */
		if ((head_index &= 0xff) == 0)
			buff_ptr -= 0x200;/* wraparound: X2 for I/O */
		addrp->rhead = head_index;	/* update:  let MUX proceed */

		/* process the data */

		if (status) {		/* save some CPU cycles */
			if (status & FRAME_ERROR)	c |= FRERROR;
			if (status & OVERRUN_ERROR)	c |= OVERRUN;
			if (status & PARITY_ERROR)	c |= PERROR;
			if (status & BREAK_DETECT)	c |= FRERROR;
			if (status & BUFFER_OVERFLOW)	c |= OVERRUN;
		}
/*
 * let the number of unprocessed bytes be 0.  This is used for the XOFF
 * threshhold.  Since we have virtually unlimited cblocks anyway, it
 * really doesn't matter.  The ttin() programmer is apparently paranoid. :-)
 */
		if (put_byte)
			(*line_input)(tp, c, 0, 0);
	}
	tp->modem_status = 0;			/* for MTNOACTIVITY timeout */
}

/****************************************************************************
 * Fill the MUX transmit FIFO (up to 16 bytes), using data from tp->t_outq.
 ****************************************************************************/

mux_tx(tp)
register struct tty *tp;
{
	register struct mux_port_io *addrp;
	register u_char *buff_ptr;
	register tail_index, next_tail_index;
	register c, tx_limit;

	tx_limit = tp_tx_limit(tp);
	addrp = tp_addrp(tp);
	next_tail_index = tail_index = addrp->ttail;
	next_tail_index++;
	next_tail_index &= 0xf;
	buff_ptr = tp_tx_base(tp);
	buff_ptr += tail_index; buff_ptr += tail_index;	/* twice for DIO */

	while (next_tail_index != addrp->thead) {	/* while room on mux */
		if (tp->t_state & (TTXON|TTXOFF|TTSTOP)) {/* save CPU cycles */
			if (tp->t_state & TTXON) {	/* send XON ? */
				tp->t_state &= ~TTXON;
				c = CSTART;
				tx_limit++;
				goto mux_putc;
			}
			else if (tp->t_state & TTXOFF) {/* send XOFF ? */
				tp->t_state &= ~TTXOFF;
				c = CSTOP;
				tx_limit++;
				goto mux_putc;
			}
			else if (tp->t_state & TTSTOP)	/* stop sending ? */
				return;
		}
		if ((c = getc(&tp->t_outq)) < 0)
			break;				/* no data in outq */
mux_putc:
		*buff_ptr = c;				/* send data to MUX */
		buff_ptr += 2;				/* next odd byte */
		if ((tail_index = next_tail_index) == 0)/* update indexes */
			buff_ptr -= 0x20;		/* wraparound */
		next_tail_index++;
		next_tail_index &= 0xf;
		addrp->ttail = tail_index;		/* update MUX index */
		if (((tail_index - addrp->thead) & 0x0f) == 1)/* start mux ? */
			mux_interrupt(tp, tp_port_number(tp), 0x02);
		if (--tx_limit == 0)
			break;
	}
	tp->t_int_flag |= PEND_TXINT;
	tp->modem_status = 0;	/* for MTNOACTIVITY timeout */
/*	return(rvalue);		/* return value not used */
}


muxproc(tp, cmd, data)

register struct tty	*	tp;
int				cmd;
int				data;

{
/*----------------------------------------------------------------------
 | FUNCTION -	General functions called by line discipline, et.al.  This
 |		routine is pointed at by tp->t_proc, so it can be called
 |		by any of the tty routines.
 |
 | PARAMETERS PASSED -
 |	tp	tty structure pointer
 |	cmd	function code to perform
 |	data	additional data for function code
 |
 | FUNCTION CODES -
 | T_TIME:	end timeout
 | T_WFLUSH:	flush MUX write FIFO
 | T_RFLUSH:	flush MUX read FIFO
 | T_RESUME:	Got a "START" character, resume output
 | T_OUTPUT:	start output
 | T_SUSPEND:	Got a "STOP" character
 | T_BLOCK:	Send "STOP" to other side
 | T_UNBLOCK:	Send "START" to other side
 | T_BREAK:	Send "BREAK" to other side
 | T_PARM:	Set port's baud rate, character size, etc
 | T_MODEM_CNTL:	Set port's modem control lines
 | T_MODEM_STAT:	Get port's modem status lines
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 21 May 92	dmurray	Added T_VHANGUP.
 | 
 ---------------------------------------------------------------------*/

	switch(cmd) {
	case T_TIME:
		tp->t_state &= ~TIMEOUT;
		muxparam(tp);/* now configure the port */
		goto start;

	case T_WFLUSH:
		/* clear card transmit buffer */
		mux_tx_flush(tp);

	case T_RESUME:
		tp->t_state &= ~TTSTOP;
		goto start;

	case T_OUTPUT:
	start:
		if (tp->t_state&(TIMEOUT|TTSTOP|BUSY))
			break;
		mux_tx(tp);
		break;

	case T_SUSPEND:
		/* Don't send any more data, XOFF was received */
		tp->t_state |= TTSTOP;
		break;

	case T_BLOCK:	/* Tell the device to stop sending data */
		tp->t_state &= ~TTXON;
		tp->t_state |= TTXOFF|TBLOCK;
		mux_tx(tp);
		break;

	case T_RFLUSH:
		{
			register ushort cflag = tp->t_cflag;
			tp->t_cflag &= ~CREAD;	/* turn off receiver */
			mux_rx(tp);		/* bit-bucket MUX rx data */
			tp->t_cflag = cflag;	/* restore the bit */
		}
		if (!(tp->t_state&TBLOCK))
			break;

	case T_UNBLOCK: /* Tell the device to start sending data */
		tp->t_state &= ~(TTXOFF|TBLOCK);
		tp->t_state |= TTXON;
		mux_tx(tp);
		break;

	case T_BREAK:				/* called only from ttiocom */
		{
			register pn = tp_port_number(tp);

			mux_interrupt(tp, pn, 0x04);/* toggle break on */
			delay(HZ/4);		/* send break for .25 sec */
			mux_interrupt(tp, pn, 0x04);/* toggle break on */
		}
		break;

	case T_PARM:
		muxparam(tp);
		break;

	case T_MODEM_CNTL:
		{
			register struct mux_io_8k *addr8 = tp_addr8(tp);
			register int smcnl = tp->t_smcnl;
			register int modem = 0;

			if (smcnl & MDTR)	modem |= MUX_DTR;
			if (smcnl & MRTS)	modem |= MUX_RTS;
			if (smcnl & MDRS)	modem |= MUX_DRS;
			switch (tp_port_number(tp)) {
			case 0:	addr8->modem0.out = modem; break;
			case 1:	addr8->modem1.out = modem; break;
			case 2:	addr8->modem2.out = modem; break;
			case 3:	addr8->modem3.out = modem; break;
			default:	panic("MUX driver programming error");
			}
			get_semaphore(tp_addr0(tp));
			addr8->mcntl_reg |= 1<<tp_port_number(tp);
			clear_semaphore(tp_addr0(tp));
			mux_interrupt(tp, 0x10, 0);
		}
		break;

	case T_MODEM_STAT:
		{
			register struct mux_io_8k *addr8 = tp_addr8(tp);
			register int smcnl = tp->t_smcnl;
			register int modem;

			switch (tp_port_number(tp)) {
			case 0:	modem = addr8->modem0.in; break;
			case 1:	modem = addr8->modem1.in; break;
			case 2:	modem = addr8->modem2.in; break;
			case 3:	modem = addr8->modem3.in; break;
			default:	panic("MUX driver programming error");
			}
			smcnl &= ~(CCITT_IN);
			if (modem & MUX_CTS)	smcnl |= MCTS;
			if (modem & MUX_DSR)	smcnl |= MDSR;
			if (modem & MUX_DCD)	smcnl |= MDCD;
			if (modem & MUX_RI )	smcnl |= MRI ;
			tp->t_smcnl = smcnl;
		}
		break;

	case T_VHANGUP:
		((struct mux_hw_data *)(tp->utility))->vhangup	= data;
		ttcontrol( tp, LDC_VHANGUP, data );
		ttycomn_control( tp, cmd, data );
		if (data == 0)
			break;
		/* No wakeups to do since there are no sleeps in this module. */
		break;
	}
}

/* This is called from ttywait */

mux_wait(tp)
register struct tty *tp;
{
	register n;
	register struct mux_port_io *addrp = tp_addrp(tp);

	while ((n = tx_size(addrp)) > 0)
		tty_char_delay(tp, n);

	return (0) ;
}

extern short baudmap[];

tty_char_delay(tp, n)	/* delay user, depending on amt of data, baud, etc. */
struct tty *tp;
{
	register cflag = tp->t_cflag;
	register size;

	size = 2;				/* START/STOP bit */
	if (cflag & CSTOPB)	size += 1;	/* extra STOP bit */
	cflag &= CSIZE;
	if (cflag == CS5)	size += 5;
	if (cflag == CS6)	size += 6;
	if (cflag == CS7)	size += 7;
	if (cflag == CS8)	size += 8;

	size *= n;
	size *= baudmap[tp->t_cflag & CBAUD];
	size += (153600/HZ - 1);		/* round it up */
	size /= (153600/HZ);
	delay(size);	/* roundup(size*n*baudmap[speed]*HZ/153600) */
}

/*************************************************************************
** Console Interface Routines:						**
**									**
** mux_putchar(c)		Print a char - used by Kernel printf.	**
** mux_who_init(n)		Set-up Line characteristics for console.**
*************************************************************************/

mux_putchar(c)
register ushort c;
{
	register struct tty *tp = cons_tty;
	register struct mux_port_io *addrp = tp_addrp(tp);
	int x = spl6();		/* could be called from any spl level */

	if (cfreelist.c_next == NULL)
		cinit();
	if (c == 0)
		goto done;

	while(tx_size(addrp) > 0)
		;
/* this code should be the same as the echo processing code in ttin(). */
	ttxput(tp, (long) c, 0);
	(*tp->t_proc)(tp, T_OUTPUT);
done:
	splx(x);
}


/* Set up Line characteristics for console. */
mux_who_init(tp, tty_num, sc)
register struct tty *tp;
{
	extern char ttcchar[];

	if (tty_num != -1)
		return;
	if (tp == NULL)	{	/* we must use port 1 */
		cons_dev = (dev_t)makeminor(sc, 1, 0, 4);
		cons_tty = tp = mux_get_tp(cons_dev);
	}

	tp->t_state = CARR_ON;
	tp->t_iflag = ICRNL|IXON;
	tp->t_oflag = OPOST|ONLCR|TAB3;
	tp->t_lflag = ISIG|ICANON|ECHO|ECHOK;
	tp->t_cflag = CLOCAL|CREAD|B9600|CS8;

	bcopy(ttcchar, tp->t_cc, NLDCC);/* setup default control characters */
	muxparam(tp);
	printf("\n    System Console is 98642 at select code %d\n",sc);
}

/****************************************************************************
 * This routine is called at boot time to allocate and initialize kernel
 * data structures.  This routine also "turns on" the MUX.
 ****************************************************************************/

struct tty *
mux_pwr(isc_num, addr,id,il,cs,pci_total)
register char *addr;
int *cs;
int *pci_total;
{
/*----------------------------------------------------------------------
 | FUNCTION -	Power-on initialization routine.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 |
 | 15 May 92	jlau	Changed the tp->utility pointer initialization.  
 |			Tp->utility now points to a new structure
 |			which contains the open_count and a pointer to
 |			the mux_io_8k structure.  Subsequent access to
 |			the mux_io_8k structure will have to go through this
 |			indirection (see changes to macro tp_addr8(tp)).
 |
 ------------------------------------------------------------------------*/
	register struct mux_interrupt *mux;
	register struct tty *tp;
	register struct mux_io_8k *addr8;
	register i;

	extern struct tty_driver sio642_driver;

	tp = (struct tty *) 0;

	/* id 5 = mux, allow only interrupt level 3 */
	if ((id != 5) || (il != MUX_INT_LVL))
		return(tp);

	addr8 = (struct mux_io_8k *) addr;	(char *) addr8 += 0x8000;
	if (addr8->st_cond != 0xe0)
		return(tp);	/* Mux failed self test */

	/* set modem interupt mask */
	addr8->modem0.mask = (MUX_RI | MUX_DCD | MUX_DSR | MUX_CTS);
	addr8->modem1.mask = (MUX_RI | MUX_DCD | MUX_DSR | MUX_CTS);
	addr8->modem2.mask = (MUX_RI | MUX_DCD | MUX_DSR | MUX_CTS);
	addr8->modem3.mask = (MUX_RI | MUX_DCD | MUX_DSR | MUX_CTS);

	{
	/* allocate select code area */
	register struct isc_table_type *isc;

	isc = (struct isc_table_type *)calloc(sizeof(struct isc_table_type));
	isc_table[isc_num] = isc;
	isc->card_type = HP98642;
	isc->card_ptr = (int *)addr;
	isc->int_lvl = MUX_INT_LVL;
	/* for indirect calls */
	isc->tty_routine = (struct tty_drivercp *)&sio642_driver;
	}

	/* allocate isr info structure */
	mux = (struct mux_interrupt *)calloc(sizeof(struct mux_interrupt));
	sc_int(isc_num) = mux;

	for (i = 0; i < 4; i++) {
		tp = (struct tty *)calloc(sizeof(struct tty));
		tp->t_card_ptr = addr;		/* Set card address field */
		tp->t_drvtype = &sio642_driver;
		tp->t_proc = muxproc;			/* for ttyflush */
		tp->t_int_flag = 0;
		tp->t_rcv_flag = FALSE;
		tp->t_xmt_flag = FALSE;
		tp->t_int_lvl = MUX_INT_LVL;
		tp->t_pgrp = 0;
		tp->t_cproc = NULL;
		tp->t_hw_flow_ctl = 0;
		tp->t_open_sema = 0;
		tp->t_hardware = 0;
		tp->utility = (int *)calloc(sizeof(struct mux_hw_data));
		((struct mux_hw_data *)(tp->utility))->open_count = 0;
		((struct mux_hw_data *)(tp->utility))->mux_io_ptr = addr8;
		(char *)tp_addrp(tp) = addr + 0x8e01 + (i << 1);
		(char *)tp_tx_base(tp) = addr + TBUFF(i);
		(char *)tp_rx_base(tp) = addr + RBUFF(i);
		tp_port_number(tp) = i;

		mux->tp[i] = tp;
	}
	/* Check for remote console */
	if ((!remote_console)
	 /* Check remote bit */
	 && (((struct mux_io_0k *)addr)->reset & 0x80) ) {
		*cs = 1;
		cons_tty = mux->tp[1];
		remote_console = TRUE;
		/* make console dev number */
		cons_dev = (dev_t)makeminor(isc_num, 1, 0, 4);
	}
	isrlink(mux_intr, MUX_INT_LVL,
		&((struct mux_io_0k *)addr)->interrupt,
		0xc0, 0xc0, isc_num, mux);
/*
 * MUX timer is initially off.
 * It is easiest to turn on the MUX timer, then let the ISR decide the
 *   receiver mode (interupt-on-byte vs. interrupt-on-60hz-timer).
 */
	addr8->bitmap[0] = 0;	/* fake out the ISR */
	mux_interrupt(tp, 0x20, 0);	/* toggle MUX timer */
	((struct mux_io_0k *)addr)->interrupt = 0x80; /* enable card */
	(*pci_total) += 4;
	return(tp);
}


extern int (*make_entry)();
int (*sio642_saved_make_entry)();

struct tty_driver sio642_driver = {
SIO642, mux_open, mux_close, mux_read, mux_write, mux_ioctl, mux_select,
	mux_putchar, mux_wait, mux_pwr, mux_who_init, 0
};

/*
 * linking and inititialization routines
 */

sio642_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	unsigned char self_test;
	register struct mux_io_8k *addr = (struct mux_io_8k *)isc->card_ptr;

	if (id != 5)
		return(*sio642_saved_make_entry)(id, isc);

	(char *) addr += 0x8000;
	self_test = addr->st_cond;
	if (self_test != 0xe0) {   	/* Mux failed self test */
	      printf("HP98642 at Select Code %d ignored;",isc->my_isc);
	      printf("  Self Test Error = 0x%x\n", self_test);
	      return FALSE;
	}
	io_inform("HP98642 RS-232C Multiplexer", isc, MUX_INT_LVL);
	return FALSE;
}

sio642_link()
{
	extern struct tty_driver *tty_list;

	sio642_saved_make_entry = make_entry;
	make_entry = sio642_make_entry;

	sio642_driver.next = tty_list;
	tty_list = &sio642_driver;
}
