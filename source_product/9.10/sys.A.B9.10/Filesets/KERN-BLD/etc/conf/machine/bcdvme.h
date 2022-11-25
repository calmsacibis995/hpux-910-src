/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/bcdvme.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:10:07 $
 */
/* HPUX_ID: @(#)bcdvme.h	52.2		88/04/26 */
#ifndef _MACHINE_BCDVME_INCLUDED /* allow multiple inclusions */
#define _MACHINE_BCDVME_INCLUDED

/* 
(c) Copyright 1983, 1984, 1985, 1986 Hewlett-Packard Company.
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

#ifndef _KERNEL_BUILD
#include <sys/ioctl.h>
#endif /* ! _KERNEL_BUILD */

/*
 * VME driver header file
 * @(#)vme.h	1.5 - 11/28/85
 */

/* ioctl commands */
#define VMESET		_IOW('V',1,struct vmeio)
#define VMEGET		_IOR('V',2,struct vmeio)
#define VMEWANTI	_IOW('V',3,struct vmeint)
#define VMEREADI	_IOWR('V',4,struct vmeint)
#define VMEENDI		_IOW('V',5,struct vmeint)

/* control io */
struct vmeio {
	unsigned short timeout;
	char addmod;
	char ioflags;
	char strategy;
};

/* timeout is in .01s of seconds */

/* addmod is up to the user */

/* ioflags */
#define VBYTE		01		/* read/write byte by byte */
#define VFIXADDR	02		/* r/w fixed address on VME */

/* strategy for releasing bus */
#define VROR		1		/* release on request */
#define VRWD		2		/* release when done */
#define VROCLOSE	3		/* release on close */

/* control interrupts */
struct vmeint {
	char signal;
	int statusid;
};

/* statusid that matches all devices */
#define ALLDEVICES	-1 	

#endif /* _MACHINE_BCDVME_INCLUDED */
