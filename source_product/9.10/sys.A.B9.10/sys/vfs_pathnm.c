/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_pathnm.c,v $
 * $Revision: 1.7.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:16:00 $
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

/*	@(#)vfs_pathnm.c 1.1 86/02/03 SMI	*/
/*      NFSSRC @(#)vfs_pathnm.c	2.1 86/04/15 */
/*      USED TO BE CALLED vfs_pathname.c, DLP	*/


#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/uio.h"
#include "../h/errno.h"
#include "../h/pathname.h"

/*
 * Pathname utilities.
 *
 * In translating file names we copy each argument file
 * name into a pathname structure where we operate on it.
 * Each pathname structure can hold MAXPATHLEN characters
 * including a terminating null, and operations here support
 * allocating and freeing pathname structures, fetching
 * strings from user space, getting the next character from
 * a pathname, combining two pathnames (used in symbolic
 * link processing), and peeling off the first component
 * of a pathname.
 */

/*
 * Allocate contents of pathname structure.
 * Structure itself is typically automatic
 * variable in calling routine for convenience.
 */
pn_alloc(pnp)
	register struct pathname *pnp;
{
	pnp->pn_buf = (char *)kmem_alloc((u_int)MAXPATHLEN);
	pnp->pn_path = (char *)pnp->pn_buf;
	pnp->pn_pathlen = 0;
}

/*
 * Pull a pathname from user user or kernel space
 */
int
pn_get(str, seg, pnp)
	register struct pathname *pnp;
	int seg;
	register char *str;
{
	register char *cp;
	extern char *nfs_strncpy();

	pn_alloc(pnp);
	if (seg == UIOSEG_USER) {
		int l;   /* path length, including null byte */
		register int error;

#ifndef HPUXBOOT
		error = copyinstr(str, pnp->pn_path, MAXPATHLEN, &l);
#else HPUXBOOT
		/* HPUXBOOT is kernel space to kernel space */
		error = copystr(str, pnp->pn_path, MAXPATHLEN, &l);
#endif HPUXBOOT
		if (!error) {
			if (l <= 1) {
				pn_free(pnp);
				return(ENOENT);
			} else {
				pnp->pn_pathlen = l-1;
				return(0);
			}
		} else if (error == EFAULT) {
			pn_free(pnp);
			return(EFAULT);
		} /* else error is ENOENT, fall through */
	} else {
		/*
		 * cp will point to last null in pn_path
		 */
		cp = nfs_strncpy(pnp->pn_path, str, MAXPATHLEN);

		pnp->pn_pathlen = cp - pnp->pn_path;
		if (pnp->pn_pathlen < MAXPATHLEN) {
			return(0);
		}
	}
	pn_free(pnp);
	return (ENAMETOOLONG);
}

/*
 * Set pathname to argument string.
 */
pn_set(pnp, path)
	register struct pathname *pnp;
	register char *path;
{
	register char *cp;
	extern char *nfs_strncpy();

	pnp->pn_path = pnp->pn_buf;
	cp = nfs_strncpy(pnp->pn_path, path, MAXPATHLEN);
	pnp->pn_pathlen = cp - pnp->pn_path;
	if (pnp->pn_pathlen >= MAXPATHLEN)
		return (ENAMETOOLONG);
	return (0);
}

#ifdef SUN_NFS
/*
 * Combine two argument pathnames by putting
 * second argument before first in first's buffer,
 * and freeing second argument.
 * This isn't very general: it is designed specifically
 * for symbolic link processing.
 */
/*Bug! Bug! Bug! If pnp->pn_buf + sympnp -> pn_pathlen is between pnp->path 
  and pnp->path+pnp->pathlen, the pnp->pn_path will not be copy correctly.*/
pn_combine(pnp, sympnp)
	register struct pathname *pnp;
	register struct pathname *sympnp;
{

	if (pnp->pn_pathlen + sympnp->pn_pathlen >= MAXPATHLEN)
		return (ENAMETOOLONG);
	bcopy(pnp->pn_path, pnp->pn_buf + sympnp->pn_pathlen,
	    (u_int)pnp->pn_pathlen);
	bcopy(sympnp->pn_path, pnp->pn_buf, (u_int)sympnp->pn_pathlen);
	pnp->pn_pathlen += sympnp->pn_pathlen;
	pnp->pn_path = pnp->pn_buf;
	return (0);
}
#else /* HPUX_NFS */
/*
 * Combine two argument pathnames by putting
 * second argument before first in first's buffer,
 * and freeing second argument by the caller.
 */
pn_combine(pnp, sympnp)
	register struct pathname *pnp;
	register struct pathname *sympnp;
{

	if (pnp->pn_pathlen + sympnp->pn_pathlen >= MAXPATHLEN)
		return (ENAMETOOLONG);
        bcopy(pnp->pn_path, sympnp->pn_buf + sympnp->pn_pathlen,
            (u_int)pnp->pn_pathlen);
	pnp->pn_pathlen += sympnp->pn_pathlen;
        bcopy(sympnp->pn_path, pnp->pn_buf, (u_int)pnp->pn_pathlen);
	pnp->pn_path = pnp->pn_buf;
	*(pnp->pn_path+pnp->pn_pathlen) = '\000';
	return (0);
}
#endif SUN_NFS

/*
 * Strip next component off a pathname and leave in
 * buffer comoponent which should have room for
 * MAXNAMLEN bytes and a null terminator character.
 *
 * Replaced by indirect call through procedure variable
 * defined in ../machine/space.h. This is to allow gen-time
 * selection of different parsers for Asian character sets.
 *
pn_getcomponent(pnp, component)
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
*/

/*
 * skip over consecutive slashes in the pathname
 */
void
pn_skipslash(pnp)
	register struct pathname *pnp;
{
	while ((pnp->pn_pathlen > 0) && (*pnp->pn_path == '/')) {
		pnp->pn_path++;
		pnp->pn_pathlen--;
	}
}

/*
 * Free pathname resources.
 */
void
pn_free(pnp)
	register struct pathname *pnp;
{

	kmem_free((caddr_t)pnp->pn_buf, (u_int)MAXPATHLEN);
	pnp->pn_buf = 0;
}

/*
 * Reset pathname structure.
 */
pn_reset(pnp)

register struct pathname *pnp;
{
	register char *cp;
	register int	i;
	cp = pnp->pn_path = pnp->pn_buf;
	for (i=0; *cp++; i++);
	pnp->pn_pathlen = i;
}


/*reset the last component of pn_buf to pn_buf*/
/*returns 0 if pnp is empty, 1 if it is non-empty*/
pn_setlastcomp(pnp)

register struct pathname *pnp;
{
	register char *cp1, *cp2;
	register char *pnpbase = pnp->pn_buf;
	cp2 = 0;
	for (cp1 = pnpbase; *cp1; cp1++) {
		if (*cp1 != '/') cp2=cp1; /*cp2 points to last non-null non-"/" 
					    character*/
	}
	if (cp2 == 0) {			/* Just in case, pnp is empty*/
		pnp -> pn_path = pnpbase;
		*pnpbase = '\000';
		pnp -> pn_pathlen = 0;
		return(0);
	}
	for(cp1 = cp2; cp1 >= pnpbase; cp1--) {
		if(*cp1 == '/') {
			break;
		}
	}
	cp1++;
	pnp -> pn_pathlen = cp2 - cp1 + 1;
	pnp -> pn_path = pnpbase;
	while(cp1 <= cp2) {
		*pnpbase++ = *cp1++;
	}
	*pnpbase = '\000';
	return(1);
}

/*back up one component, assumpt there is at least one component before current
  pn_path*/
pn_backup_comp(pnp)
register struct pathname *pnp;
{
	register char *cp1, *cp2;
	register int len;
	cp1 = pnp -> pn_path;
	cp2 = pnp -> pn_buf;
	len = pnp -> pn_pathlen;
	cp1--;
	len++;
	while ((cp1 >= cp2) && (*cp1 == '/')) {
		cp1--;
		len++;
	}
	while ((cp1 >= cp2) && (*cp1 != '/')) {
		cp1--;
		len++;
	}
	cp1++;
	len--;
	pnp -> pn_path = cp1;
	pnp -> pn_pathlen = len;
}

