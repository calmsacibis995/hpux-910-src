/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_bigio.c,v $
 * $Revision: 1.3.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:40:15 $
 */

/* HPUX_ID: @(#)dux_bigio.c	55.1		88/12/23 */

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
#include "../h/errno.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../dux/dux_dev.h"
#include "../dux/dm.h"

/*  This system call  is  designed  to  do  I/O  directly  to  a  specified
 *  character  device.  The  user must be a UNSP and the device must be the
 *  special DUX remote device.  The function sets up a uio and either reads
 *  or  writes  the  appropriate  data.  The  return value is the number of
 *  characters read/written, just as with read/write.
 */

extern int nchrdev;

bigio()
  { register struct a
      { int     write;
	dev_t   dev;
	int     offset;
	caddr_t buf;
	u_int   size;
	int     fpflags;
      } *uap = (struct a *)u.u_ap;

    register dev_t rdev;
    register int   maj;
    struct uio uio;
    struct iovec iovec;

    if((u.u_duxflags & DUX_UNSP) == 0 || devsite(uap->dev) != my_site)
      { u.u_error = EINVAL;
	return;
      }

    if((rdev = localdev(uap->dev)) == NODEV || (maj = major(rdev)) >= nchrdev)
      { u.u_error = ENXIO;
	return;
      }

    /*  Set up uio and iovec    */
    iovec.iov_base = uap->buf;
    iovec.iov_len  = uap->size;

    uio.uio_iov    = &iovec;
    uio.uio_iovcnt = 1;
    uio.uio_offset = uap->offset;
    uio.uio_seg = UIOSEG_USER;
    uio.uio_resid  = uap->size;
    uio.uio_fpflags = uap->fpflags;

    if (((u.u_procp->p_flag & SOUSIG) == 0) && setjmp (&u.u_qsave))
    {
	if (uio.uio_resid == uap->size)
		u.u_eosys = RESTARTSYS;
    }
    else if(uap->write)
      u.u_error = (*cdevsw[maj].d_write)(rdev, &uio);
    else
      u.u_error = (*cdevsw[maj].d_read)(rdev, &uio);

    u.u_r.r_val1 = uap->size - uio.uio_resid;
  }
