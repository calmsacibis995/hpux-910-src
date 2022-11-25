/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/sys/RCS/tty_conf.c,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:22:09 $
 */
/* HPUX_ID: @(#)tty_conf.c	55.1		88/12/23 */

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


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/tty.h"
#include "../h/conf.h"

int	nodev();
int	nulldev();

int	ttopen(),ttclose(),ttread(),ttwrite(),ttioctl();
int	ttin(),ttout();

#include "../h/nbk.h"
#if NBK > 0
int	bkopen(),bkclose(),bkread(),bkinput(),bkioctl();
#endif

#include "../h/tb.h"
#if NTB > 0
int	tbopen(),tbclose(),tbread(),tbinput(),tbioctl();
#endif

struct	linesw linesw[] =
{
	ttopen, ttclose, ttread, ttwrite, ttioctl,
	ttin, ttout, nulldev, nulldev, nulldev,
#if NBK > 0
	bkopen, bkclose, bkread, ttwrite, bkioctl,
	bkinput, nodev, nulldev, ttstart, nulldev,
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
	ttopen, ttclose, ttread, ttwrite, ttioctl,
	ttin, ttout, nulldev, nulldev, nulldev,
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nulldev,		/* 3 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
#if NTB > 0
	tbopen, tbclose, tbread, nodev, tbioctl,
	tbinput, nodev, nulldev, ttstart, nulldev,		/* 4 */
#else
	nodev, nodev, nodev, nodev, nodev,
	nodev, nodev, nodev, nodev, nodev,
#endif
};

int	nldisp = sizeof (linesw) / sizeof (linesw[0]);

/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
nullioctl(tp, cmd, data, flags)
	struct tty *tp;
	char *data;
	int flags;
{

#ifdef lint
	tp = tp; data = data; flags = flags;
#endif
	return (-1);
}
