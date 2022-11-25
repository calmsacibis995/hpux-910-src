/* @(#) $Revision: 64.7 $ */    
/*
 * Library routines to copy access control list (ACL) and miscellaneous mode
 * bits from one file to another.
 *
 * To get fcpacl() instead of cpacl(), compile with -DFVERSION.  The two
 * routines are so similar, it was best to build them as ifdef'd variants.
 *
 * POINTER to design documents, technical rationales, etc:  24 documents were
 * placed in shared sources (/hpux/shared/INFO/release_docs/6.5/ACLs/) in 8902.
 */


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define chmod      _chmod
#define chownacl   _chownacl
#define cpacl      _cpacl
#define fcpacl     _fcpacl
#define fchmod     _fchmod
#define fgetacl    _fgetacl
#define fsetacl    _fsetacl
#define getacl     _getacl
#define setacl     _setacl
#endif

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/acl.h>

extern int errno;

void	chownacl();

/*
 * Miscellaneous mode bits, those other than access mode bits:
 * (Let the compiler optimize this to a constant value.)
 */

#define	MMBITS  (S_CDF | S_ISUID | S_ISGID | S_ENFMT | S_ISVTX)


/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define	PROC				/* null; easy to find procs */
#define	REG	register

/*
 * ERROR RETURN VALUES (must be negative):
 */

#define	ERR_GETACL	(-1)		/* unable to perform getacl()	*/
#define	ERR_CHMOD_MM	(-2)		/* unable to perform chmod()	*/
					/* to set misc mode bits	*/
#define	ERR_SETACL	(-3)		/* unable to perform setacl()	*/
#define	ERR_CHMOD	(-4)		/* unable to perform chmod()	*/
					/* to set all mode bits		*/


/************************************************************************
 * [F]   C P   A C L
 *
 * See the manual entry (cpacl(3)) for details.
 */

#ifndef FVERSION

#ifdef _NAMESPACE_CLEAN
#undef cpacl
#pragma _HP_SECONDARY_DEF _cpacl cpacl
#define cpacl _cpacl
#endif /* _NAMESPACE_CLEAN */

PROC int cpacl (fromfile, tofile, frommode, fromuid, fromgid, touid, togid)
	char	*fromfile, *tofile;	/* filenames */
#else

#ifdef _NAMESPACE_CLEAN
#undef fcpacl
#pragma _HP_SECONDARY_DEF _fcpacl fcpacl
#define fcpacl _fcpacl
#endif /* _NAMESPACE_CLEAN */

PROC int fcpacl (fromfd, tofd,    frommode, fromuid, fromgid, touid, togid)
	int	fromfd, tofd;		/* open file descriptors */
#endif
	int	frommode;		/* for chmod()		 */
	uid_t  	fromuid, touid;		/* for owner translation */
	gid_t	fromgid, togid;
{
REG	int	nentries;		/* number in acl[]	*/
	struct	acl_entry acl [NACLENTRIES];

/*
 * GET ACL ON FROMFILE:
 */

#ifndef FVERSION
	if ((nentries = getacl (fromfile, NACLENTRIES, acl)) < 0)  /* failed */
#else
	if ((nentries = fgetacl (fromfd,  NACLENTRIES, acl)) < 0)  /* failed */
#endif
	{
	    if (errno != EOPNOTSUPP)		/* not remote file */
		return (ERR_GETACL);

/*
 * REMOTE FROMFILE; SET ALL MODE BITS ON TOFILE:
 */

#ifndef FVERSION
	    else if (chmod (tofile, frommode) < 0)
#else
	    else if (fchmod (tofd,  frommode) < 0)
#endif
		return (ERR_CHMOD);

	} /* if */

/*
 * LOCAL FROMFILE; SET MISCELLANEOUS MODE BITS ON TOFILE:
 *
 * This also clears the base mode bits and deletes optional entries as a
 * side-effect.
 */

	else
	{
#ifndef FVERSION
	    if (chmod (tofile, frommode & MMBITS) < 0)
#else
	    if (fchmod (tofd,  frommode & MMBITS) < 0)
#endif
		return (ERR_CHMOD_MM);

/*
 * TRANSLATE AND SET ACL ON TOFILE:
 */

	    chownacl (nentries, acl, fromuid, fromgid, touid, togid);

#ifndef FVERSION
	    if (setacl (tofile, nentries, acl) < 0)	/* failed */
#else
	    if (fsetacl (tofd,  nentries, acl) < 0)	/* failed */
#endif
	    {
		if (errno != EOPNOTSUPP)	/* not remote file */
		    return (ERR_SETACL);

/*
 * REMOTE TOFILE; SET ALL MODE BITS:
 */

#ifndef FVERSION
		if (chmod (tofile, frommode) < 0)
#else
		if (fchmod (tofd,  frommode) < 0)
#endif
		    return (ERR_CHMOD);

	    } /* if */
	} /* else */

	return (0);

} /* [f]cpacl */
