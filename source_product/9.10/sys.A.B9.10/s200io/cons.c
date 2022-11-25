/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/cons.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:10:45 $
 */
/* HPUX_ID: @(#)cons.c	55.1		88/12/23 */

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
 Console Driver & TTY device switch
 */

#include "../h/param.h"
#include "../h/user.h"		/* user structure */
#include "../h/tty.h"		/* user structure */
#include "../wsio/hpibio.h"	/* isc_table_type structure */
#include "../h/errno.h"	

extern struct isc_table_type *isc_table[];
extern struct tty *cons_tty;
extern int cons_dev;
extern short npci;

dev_t  	cons_out_dev = (dev_t) NULL;
struct tty *cons_out_tty = (struct tty *) NULL;

undo_new_console()
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Restore console output to physical console device.
 |			This function is called before panic is called so 
 |			that the information printed out by panic goes to the 
 |			physical console device.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 29 Mar 89  garth	Added this function to restore console output
 |			to physical console device. 
 |
 --------------------------------------------------------------------------*/

	cons_out_dev = (dev_t) NULL;
	cons_out_tty = (struct tty *) NULL;
}

new_cons_open(tp)
struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Send console output to another tty not physical
 |			console tty.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 29 Mar 89  garth	Added this function so that the console output
 |			could be sent to a another tty.
 |
 --------------------------------------------------------------------------*/
	
	cons_out_dev = tp->t_dev;
	cons_out_tty = tp;
}

new_cons_close(tp)
struct tty *tp;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Restore console output to original console tty.  
 |			This is done only if tp is currently the console.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 29 Mar 89  garth	Added this function so that the console can
 |			be restored to its original value when a 
 |			tty wants to release it.
 |
 --------------------------------------------------------------------------*/
	/* Are we the console? */
	if (tp->t_dev == cons_out_dev)
		undo_new_console();
}


/* Open the console device.
*/
cons_open(dev, flag)
{

	u_int	fsminor;

	return((*cons_tty->t_drvtype->open)(cons_dev, flag, &fsminor));
}

/* Open the tty device.
*/
tty_open(dev, flag, fsminorp)

dev_t		dev;
int		flag;
int	*	fsminorp;
{
	register struct isc_table_type	*	isc;
	register int				unit;

	unit = m_selcode(dev);
	if ((unit < 0) || (unit > 31))		/* bad minor number */
		return(ENXIO);

	if ((isc = isc_table[unit]) == NULL)	/* no device at selcode */
		return(ENXIO);

	if (isc->tty_routine == NULL)		/* not a tty device here */
		return(ENXIO);

	return((*isc->tty_routine->open)(dev, flag, fsminorp));
}

/* Close the console device.
*/
cons_close(dev)
{
	(*cons_tty->t_drvtype->close)(cons_dev);
}

/* Close the tty device.
*/
tty_close(dev)
{
	register struct isc_table_type *isc;

	isc = isc_table[m_selcode(dev)];

	return((*isc->tty_routine->close)(dev));
}

/* This is where the read system call comes
** if the console device is specified.
*/
cons_read(dev, uio)
{
	return((*cons_tty->t_drvtype->read)(cons_dev, uio));
}

/* System read call.
*/
tty_read(dev, uio)
{
	register struct isc_table_type *isc;

	isc = isc_table[m_selcode(dev)];

	return((*isc->tty_routine->read)(dev, uio));
}

/* This is where the write system call comes
** if the console device is specified.
*/
cons_write(dev, uio)
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Console driver write routine.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 29 Mar 89  garth	Added check to see if console output should be
 |			sent to another tty.
 |
 --------------------------------------------------------------------------*/

	if (cons_out_dev)
		return((*cons_out_tty->t_drvtype->write)(cons_out_dev, uio));
	return((*cons_tty->t_drvtype->write)(cons_dev, uio));
}

/* System write call.
*/
tty_write(dev, uio)
{
	register struct isc_table_type *isc;

	isc = isc_table[m_selcode(dev)];

	return((*isc->tty_routine->write)(dev, uio));
}

cons_ioctl(dev, cmd, arg, mode)
{
	return((*cons_tty->t_drvtype->ioctl)(cons_dev, cmd, arg, mode));
}

tty_ioctl(dev, cmd, arg, mode)
{
	register struct isc_table_type *isc;

	isc = isc_table[m_selcode(dev)];

	return((*isc->tty_routine->ioctl)(dev, cmd, arg, mode));
}

/* This is where the select system call comes
** if the console device is specified.
*/
cons_select(dev, rw)
{
	return((*cons_tty->t_drvtype->select)(cons_dev, rw));
}

/* System read call.
*/
tty_select(dev, rw)
{
	register struct isc_table_type *isc;

	isc = isc_table[m_selcode(dev)];

	return((*isc->tty_routine->select)(dev, rw));
}

/*************************************************************************
** Console Interface Routines:						**
**									**
** putchar(c, touser)		Print a char - used by Kernel printf.	**
*************************************************************************/
putchar(c, touser)
register int c;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Print a char - used by kernel printf.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 12 Jul 89  garth	Fix for defects INDaa05503 and INDaa05504.
 |			Added returns around calls to driver putchar
 |			routine.
 |
 --------------------------------------------------------------------------*/

	extern char cons_up;

	if (touser) {
		register struct tty *tp = u.u_procp->p_ttyp;

		if (tp && (tp->t_state&CARR_ON)) {
			register s = splsx(tp->t_int_lvl);
			ttxput(tp, c, 0);
			(*tp->t_proc)(tp, T_OUTPUT);
			splsx(s);
		}
		return;
	}
	else  {
		extern char *panicstr;
		if (panicstr && cons_out_dev!=NULL) { 	/* if panicking */
			undo_new_console();
		}

		if (cons_up) {
			if (cons_out_dev)
				/* NOTE: Only the putchar routine for the
				   pty driver has the necessary changes
				   for this to work!!!!
				 */
				return((*cons_out_tty->t_drvtype->kputchar)(c));
			return((*cons_tty->t_drvtype->kputchar)(c));
		}
	}
}
