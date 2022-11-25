/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/mem.c,v $
 * $Revision: 1.6.84.5 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/09/22 14:50:50 $
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
 * Memory special file
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/uio.h"

mm_read(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (mmrw(dev, uio, UIO_READ));
}

mm_write(dev, uio)
dev_t dev;
struct uio *uio;
{
	return (mmrw(dev, uio, UIO_WRITE));
}

mmrw(dev, uio, rw)
dev_t dev;
struct uio *uio;
enum uio_rw rw;
{
	register u_int c;
	register struct iovec *iov;
	register caddr_t vaddr;
	int error = 0;
	int access;

	/* use tt window XXX */
	if (rw == UIO_READ)
		purge_dcache_u();
	else
		purge_dcache_s();

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}

		switch (minor(dev)) {

		/* minor device 0 is physical memory */
		case 0: {
			if (uio->uio_offset < ptob(physmembase))
				goto fault;

			c = min((u_int)iov->iov_len,(u_int)(0-uio->uio_offset));
			error = uiomove((caddr_t)uio->uio_offset, c, rw, uio);
			continue;
		}

		/* minor device 1 is kernel memory */
		case 1:
			/* should check for io space access */
			c = iov->iov_len;
			access = kernacc((caddr_t)uio->uio_offset, c, 
				     rw == UIO_READ ? B_READ : B_WRITE);
			if (access == UIO_NOACCESS)
				goto fault;
			error = uiomove((caddr_t)uio->uio_offset, c, rw, uio);
			if (access == UIO_KERNWRITE)
				kernprot((caddr_t)uio->uio_offset, c);
			continue;

		/* minor device 2 is EOF/RATHOLE */
		case 2:
			if (rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			break;

		/* minor device 3 is always full */
		case 3:
			if (rw == UIO_WRITE)
				return (ENOSPC);
			c = iov->iov_len;
			break;

#ifdef UMEM_DRIVER
		/* minor device 4 is user memory driver */
		case 4:
			error = umem_mmrw(dev, uio, rw, iov);
			c = iov->iov_len;
			break;		/*   XXX was   continue ??  */
#endif 

		default:
			error = EINVAL;
			break;
		}

		if (error)
			break;
		iov->iov_base += c;
		iov->iov_len -= c;
		uio->uio_offset += c;
		uio->uio_resid -= c;
	}
	return (error);
fault:
	return (EFAULT);
}
