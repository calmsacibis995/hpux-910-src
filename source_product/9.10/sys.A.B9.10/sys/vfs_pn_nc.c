/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_pn_nc.c,v $
 * $Revision: 1.3.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:16:14 $
 */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../h/pathname.h"

pn_getcomponent_n_computer(pnp, component)
	register struct pathname *pnp;
	register char *component;
{
	register char *cp;
	register int l;
	register int n;

	cp = pnp->pn_path;
	l = pnp->pn_pathlen;
	n = MAXNAMLEN;
	while ((l > 0) && (*cp != '/')) {
		if (--n < 0)
			return(ENAMETOOLONG);
		*component++ = *cp++;
		--l;
	}
	pnp->pn_path = cp;
	pnp->pn_pathlen = l;
	*component = 0;
	return (0);
}
