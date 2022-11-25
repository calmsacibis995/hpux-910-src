/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/iomap.c,v $
 * $Revision: 1.6.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:13:57 $
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
#include "../h/debug.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/vas.h"
#include "../h/pregion.h"
#include "../h/proc.h"
#include "../h/iomap.h"
#include "../h/vmmac.h"
#include "../h/mman.h"

extern preg_t *io_map();

iomap_open(dev, mode)
dev_t dev;
int mode;
{
	register int sc;

	/*
	** Check the validity of the size and select code
	** fields of the minor number.  If there is a problem
	** then return EINVAL.
	** The minor number is of the form: SSSSXX
	** where SS is the select code and XX is the number of 
	** select codes to map (the size).
	*/
	sc = IOMAP_SC(dev);
	if (IOMAP_SIZE(dev) > (0x10000 - sc))
		return(EINVAL);

	/*
	** Return success.
	*/
	return(0);
}

/* Hunt down the mapping by device #, return pointer to the pregion */
static preg_t *
get_io_prp(dev)
	dev_t dev;
{
	vas_t *vas = u.u_procp->p_vas;
	int pfn = btop(IOMAP_SC(dev) * 65536);
	register preg_t *prp;

	vaslock(vas);
	for (prp = vas->va_next; prp != (preg_t *)vas; prp = prp->p_next) {
		if ((prp->p_type == PT_IO) &&
				(pfn >= prp->p_hdl.p_physpfn) &&
				(pfn < prp->p_hdl.p_physpfn + prp->p_count))
			break;
	}

	/* If we found it, return it locked */
	if (prp != (preg_t *)vas) {
		reglock(prp->p_reg);
	} else {
		prp = 0;
	}
	vasunlock(vas);
	return(prp);
}

iomap_close(dev)
dev_t dev;
{
	preg_t *prp;

	u.u_error = 0;
	if ((prp = get_io_prp(dev)) != 0)
		io_unmap(prp);

	return(u.u_error);
}

iomap_read(dev)
dev_t dev;
{
	return(ENODEV);
}

iomap_write(dev)
dev_t dev;
{
	return(ENODEV);
}

iomap_ioctl(dev, cmd, arg, mode)
dev_t dev;
int cmd, *arg, mode;
{
	preg_t *prp;
	caddr_t physaddr;

	switch (cmd) {
	case IOMAPMAP:
		physaddr = (caddr_t)(IOMAP_SC(dev) * 65536);

		/* create io mapping */
		prp = io_map(u.u_procp->p_vas, *arg, physaddr, 
			     ((((IOMAP_SIZE(dev) * 65536) - 1) / NBPG) + 1),
			     PROT_URW);
		if (!prp) {
			u.u_error = ENOMEM;
			break;
		}

		/* save the virtual address */
		*arg = (int)prp->p_vaddr;
		break;

	case IOMAPUNMAP:
		/* discard any previous mapping */
		if ((prp = get_io_prp(dev)) == 0)
			u.u_error = ENXIO;
		else {
			u.u_error = 0;
			io_unmap(prp);
		}
		break;
	default:
		u.u_error = EINVAL;
		break;
	}
	return(u.u_error);
}
