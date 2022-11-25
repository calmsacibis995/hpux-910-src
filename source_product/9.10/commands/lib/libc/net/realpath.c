/*   @(#)realpath.c	$Revision: 12.1 $	$Date: 92/01/22 17:57:36 $  */
/*
#ifndef lint
static	char sccsid[] = "@(#)realpath.c	1.3 90/07/20 4.1NFSSRC; from 1.3 88/02/08 SMI";
#endif
*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifdef _NAMESPACE_CLEAN
/*
 * The following defines were added as part of the name space cleanup
 * for ANSI-C / POSIX.  The names of these routines were changed in libc
 * and we need to be sure we get the libc version.  Rather than change
 * all the places we reference these functions, we do these defines here
 * which should catch any references in header files also.
 */

#define strlen 			_strlen
#define strncmp 		_strncmp
#define strncpy 		_strncpy
#define strcpy 			_strcpy
#define getcwd			_getcwd
#define readlink		_readlink
#define pathcanon		_pathconon /* In this file */
#define realpath 		_realpath  /* In this file */

#ifdef _ANSIC_CLEAN
#define free 			_free
#define malloc 			_malloc
#endif /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */


#include <string.h>
#include <sys/param.h>
#include <errno.h>

/* HPNFS
 * changed getwd to getcwd. Same functionality but already in hp-ux
 * getcwd has one additional parm which is maxpathlen
 */
extern char	*getcwd();

/* LINTLIBRARY */

/* define secondary definition for libc name space cleanup */
#ifdef _NAMESPACE_CLEAN
#undef pathcanon
#pragma _HP_SECONDARY_DEF _pathcanon pathcanon
#define pathcanon _pathcanon

#undef realpath
#pragma _HP_SECONDARY_DEF _realpath realpath
#define realpath _realpath
#endif

/*
 * Input name in raw, canonicalized pathname output to canon.  If dosymlinks
 * is nonzero, resolves all symbolic links encountered during canonicalization
 * into an equivalent symlink-free form.  Returns 0 on success, -1 on failure.
 * The routine fails if the current working directory can't be obtained or if
 * either of the arguments is NULL.
 *
 * Sets errno on failure.
 */
int
pathcanon(raw, canon, dosymlinks)
    char	*raw,
		*canon;
    int		dosymlinks;
{
    register char	*s,
			*d;
    register char	*limit = canon + MAXPATHLEN;
    char		*modcanon;
    int			nlink = 0;

    /*
     * Do a bit of sanity checking.
     */
    if (raw == NULL || canon == NULL) {
	errno = EINVAL;
	return (-1);
    }

    /*
     * If the path in raw is not already absolute, convert it to that form.
     * In any case, initialize canon with the absolute form of raw.  Make
     * sure that none of the operations overflow the corresponding buffers.
     * The code below does the copy operations by hand so that it can easily
     * keep track of whether overflow is about to occur.
     */
    s = raw;
    d = canon;
    if (*s != '/') {
	/* Relative; prepend the working directory. */
	if (getcwd(d,MAXPATHLEN) == NULL) {
	    /* Use whatever errno value getwd may have left around. */
	    return (-1);
	}
	d += strlen(d);
	/* Add slash to separate working directory from relative part. */
	if (d < limit)
	    *d++ = '/';
	modcanon = d;
    } else
	modcanon = canon;
    while (d < limit && *s)
	*d++ = *s++;

    /* Add a trailing slash to simplify the code below. */
    s = "/";
    while (d < limit && (*d++ = *s++))
	continue;
	

    /*
     * Canonicalize the path.  The strategy is to update in place, with
     * d pointing to the end of the canonicalized portion and s to the
     * current spot from which we're copying.  This works because
     * canonicalization doesn't increase path length, except as discussed
     * below.  Note also that the path has had a slash added at its end.
     * This greatly simplifies the treatment of boundary conditions.
     */
    d = s = modcanon;
    while (d < limit && *s) {
	if ((*d++ = *s++) == '/' && d > canon + 1) {
	    register char  *t = d - 2;

	    switch (*t) {
	    case '/':
		/* Found // in the name. */
		d--;
		continue;
	    case '.': 
		switch (*--t) {
		case '/':
		    /* Found /./ in the name. */
		    d -= 2;
		    continue;
		case '.': 
		    if (*--t == '/') {
			/* Found /../ in the name. */
			while (t > canon && *--t != '/')
			    continue;
			d = t + 1;
		    }
		    continue;
		default:
		    break;
		}
		break;
	    default:
		break;
	    }
	    /*
	     * We're at the end of a component.  If dosymlinks is set
	     * see whether the component is a symbolic link.  If so,
	     * replace it by its contents.
	     */
	    if (dosymlinks) {
		char		link[MAXPATHLEN + 1];
		register int	llen;

		/*
		 * See whether it's a symlink by trying to read it.
		 *
		 * Start by isolating it.
		 */
		*(d - 1) = '\0';
		if ((llen = readlink(canon, link, sizeof link)) >= 0) {
		    /* Make sure that there are no circular links. */
		    nlink++;
		    if (nlink > MAXSYMLINKS) {
			errno = ELOOP;
			return (-1);
		    }
		    /*
		     * The component is a symlink.  Since its value can be
		     * of arbitrary size, we can't continue copying in place.
		     * Instead, form the new path suffix in the link buffer
		     * and then copy it back to its proper spot in canon.
		     */
		    t = link + llen;
		    *t++ = '/';
		    /*
		     * Copy the remaining unresolved portion to the end
		     * of the symlink. If the sum of the unresolved part and
		     * the readlink exceeds MAXPATHLEN, the extra bytes
		     * will be dropped off. Too bad!
		     */
		    (void) strncpy(t, s, sizeof link - llen - 1);
		    link[sizeof link - 1] = '\0';
		    /*
		     * If the link's contents are absolute, copy it back
		     * to the start of canon, otherwise to the beginning of
		     * the link's position in the path.
		     */
		    if (link[0] == '/') {
			/* Absolute. */
			(void) strcpy(canon, link);
			d = s = canon;
		    }
		    else {
			/*
			 * Relative: find beginning of component and copy.
			 */
			--d;
			while (d > canon && *--d != '/')
			    continue;
			s = ++d;
			/*
			 * If the sum of the resolved part, the readlink
			 * and the remaining unresolved part exceeds
			 * MAXPATHLEN, the extra bytes will be dropped off.
			*/
			if (strlen(link) >= (limit - s)) {
				(void) strncpy(s, link, limit - s);
				*(limit - 1) = '\0';
			} else {
				(void) strcpy(s, link);
			}
		    }
		    continue;
		} else {
		   /*
		    * readlink call failed. It can be because it was
		    * not a link (i.e. a file, dir etc.) or because the
		    * the call actually failed.
		    */
		    if (errno != EINVAL)
			return (-1);
		    *(d - 1) = '/';	/* Restore it */
		}
	    } /* if (dosymlinks) */
	}
    } /* while */

    /* Remove the trailing slash that was added above. */
    if (*(d - 1) == '/' && d > canon + 1)
	    d--;
    *d = '\0';
    return (0);
}

/*
 * Canonicalize the path given in raw, resolving away all symbolic link
 * components.  Store the result into the buffer named by canon, which
 * must be long enough (MAXPATHLEN bytes will suffice).  Returns NULL
 * on failure and canon on success.
 *
 * The routine indirectly invokes the readlink() system call and getwd()
 * so it inherits the possibility of hanging due to inaccessible file 
 * system resources.
 */
char *
realpath(raw, canon)
    char	*raw;
    char	*canon;
{
    return (pathcanon(raw, canon, 1) < 0 ? NULL : canon);
}
