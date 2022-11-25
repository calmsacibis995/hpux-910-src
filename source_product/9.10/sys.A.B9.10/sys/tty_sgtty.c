/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/tty_sgtty.c,v $
 * $Revision: 1.9.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:13:18 $
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

#include "../h/param.h"
#ifdef hp9000s800
#include "../h/dir.h"
#endif
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/ttold.h"

/*
 * old stty and gtty
 */
ostty()
{
	register struct a {
		int	fdes;
		int	arg;
		int	narg;
	} *uap;

	uap = (struct a *)u.u_ap;
	uap->narg = uap->arg;
	uap->arg = TIOCSETP;
	ioctl();
}

ogtty()
{
	register struct a {
		int	fdes;
		int	arg;
		int	narg;
	} *uap;

	uap = (struct a *)u.u_ap;
	uap->narg = uap->arg;
	uap->arg = TIOCGETP;
	ioctl();
}
