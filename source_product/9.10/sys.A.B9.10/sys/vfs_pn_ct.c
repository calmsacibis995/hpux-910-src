/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_pn_ct.c,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:16:08 $
 */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../h/pathname.h"


/*	Not supported yet */
#ifdef NOTDEF
static char firstof2 [] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

pn_getcomponent_chinese_t(pnp, component)
	register struct pathname *pnp;
	register char *component;
{
	register char *cp;
	register int l;
	register int n;
	register char c;
	/*	flag whether next byte will be 2nd of 2-byte char */
	register int secondof2 = 0;

	cp = pnp->pn_path;
	l = pnp->pn_pathlen;
	n = MAXNAMLEN;
	/*	Ignore '/' byte if it is 2nd byte of 2-byte char */
	while ((l > 0) && (secondof2 || (*cp != '/'))) {
		if (--n < 0)
			return(ENAMETOOLONG);
		*component++ = c = *cp++;
		if (secondof2)
			secondof2 = 0;
		else if (firstof2[(unsigned char) c])
			secondof2 = 1;
		--l;
	}
	pnp->pn_path = cp;
	pnp->pn_pathlen = l;
	*component = 0;
	return (0);
}
#endif
