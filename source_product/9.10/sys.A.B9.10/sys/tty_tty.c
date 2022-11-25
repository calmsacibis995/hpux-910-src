/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/tty_tty.c,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:22:33 $
 */
/* HPUX_ID: @(#)tty_tty.c	52.1		88/04/19 */

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
 * Indirect driver for controlling tty.
 *
 * THIS IS GARBAGE: MUST SOON BE DONE WITH struct inode * IN PROC TABLE.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/tty.h"
#include "../h/ioctl.h"
#include "../h/ttold.h"
#include "../h/proc.h"
#include "../h/uio.h"

/*ARGSUSED*/
sy_open(dev, flag)
	dev_t dev;
	int flag;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Open for tty interface.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 19 May 92	marcel	changed to conform to POSIX, and not call the driver,
 |			since that isn't nescessary with the new clear_ctty
 |			stuff
 ---------------------------------------------------------------------------*/


	if (u.u_procp->p_ttyp == NULL)
		return (ENXIO);
	return (0);
}

/*ARGSUSED*/
sy_close(dev, uio)
	dev_t dev;
	struct uio *uio;
{
  /*---------------------------------------------------------------------------
 | FUNCTION -		Open for tty interface.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 | 29 May 92	marcel	we no longer need to call the driver, since we don't
 |			call it on open, either.
 ---------------------------------------------------------------------------*/
  	return (0);
  
}


/*ARGSUSED*/
sy_read(dev, uio)
	dev_t dev;
	struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Read for tty interface.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 29 May 92	marcel	changed to conform to POSIX, and return EOF on 
 |			change of controlling terminal
 |
 ---------------------------------------------------------------------------*/
if (u.u_procp->p_ttyp == NULL)
	return (0);/* no controlling tty, so return EOF */
return ((*cdevsw[major(u.u_procp->p_ttyd)].d_read)(u.u_procp->p_ttyd, uio));
}

/*ARGSUSED*/
sy_write(dev, uio)
	dev_t dev;
	struct uio *uio;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Write for tty interface.
 |
 | MODIFICATION HISTORY
 |
 | Date		User	Changes
 | 19 May 92	marcel	changed to conform to POSIX, and return EIO on 
 |			no ctty
 |
 ---------------------------------------------------------------------------*/
if (u.u_procp->p_ttyp == NULL)
	return (EIO);

return ((*cdevsw[major(u.u_procp->p_ttyd)].d_write)(u.u_procp->p_ttyd, uio));
}

/*ARGSUSED*/
sy_ioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Ioctl for tty interface.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 |
 | 29 May 92	marcel	changed to conform to POSIX. If p_ttyp == NULL
 |			there is no ctty
 |
 ---------------------------------------------------------------------------*/

#ifdef	TIOCNOTTY
	if (cmd == TIOCNOTTY) {
		u.u_procp->p_ttyp = 0;
		u.u_procp->p_ttyd = NODEV;
		u.u_procp->p_pgrp = 0;
		return (0);
	}
#endif
if (u.u_procp->p_ttyp == NULL)
	return (ENXIO);
return ((*cdevsw[major(u.u_procp->p_ttyd)].d_ioctl)(u.u_procp->p_ttyd,
						    cmd, addr, flag));
}

/*ARGSUSED*/
sy_select(dev, flag)
	dev_t dev;
	int flag;
{
/*---------------------------------------------------------------------------
 | FUNCTION -		Select for tty interface.
 |
 | MODIFICATION HISTORY
 |
 | Date	      User	Changes
 |
 | 19 May 92	marcel	changed to not fool with u.u_error, only to return
 |			0 (false) if the controlling terminal is gone
 |
 ---------------------------------------------------------------------------*/

if (u.u_procp->p_ttyp == NULL) {
	return (0);
	}
return ((*cdevsw[major(u.u_procp->p_ttyd)].d_select)(u.u_procp->p_ttyd, flag));
}
