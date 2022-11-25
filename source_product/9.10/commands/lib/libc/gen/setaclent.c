/* @(#) $Revision: 64.6 $ */    
/*
 * Library routines to add, modify, or delete one entry in a file's access
 * control list (ACL).
 * 
 * To get fsetaclentry() instead of setaclentry(), compile with -DFVERSION.
 * The two routines are so similar, it was best to build them as ifdef'd
 * variants.
 *
 * POINTER to design documents, technical rationales, etc:  24 documents were
 * placed in shared sources (/hpux/shared/INFO/release_docs/6.5/ACLs/) in 8902.
 */

#ifdef _NAMESPACE_CLEAN
#define getacl _getacl
#define fgetacl _fgetacl
#define setacl _setacl
#define fsetacl _fsetacl
#define stat _stat
#define fstat _fstat
#define setaclentry _setaclentry
#define fsetaclentry _fsetaclentry
#endif

#include <sys/types.h>		/* for stat() */
#include <sys/stat.h>		/* for stat() */
#include <acllib.h>

/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define	PROC				/* null; easy to find procs */
#define	FALSE	0
#define	TRUE	1
#define	REG	register

/*
 * ERROR RETURN VALUES (must be negative):
 */

#define	ERR_GETACL	(-1)		/* unable to perform getacl()	*/
#define	ERR_STAT	(-2)		/* unable to perform stat()	*/
#define	ERR_MAXENTRIES	(-3)		/* can't add a new entry	*/
#define	ERR_DELETE	(-4)		/* can't del non-existing entry	*/
#define	ERR_SETACL	(-5)		/* unable to perform setacl()	*/


/************************************************************************
 * [F]   S E T   E N T R Y
 *
 * See the manual entry (setaclentry(3)) for details.
 */

#ifndef FVERSION

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef setaclentry
#pragma _HP_SECONDARY_DEF _setaclentry setaclentry
#define setaclentry _setaclentry
#endif

PROC int setaclentry (path, uid, gid, mode)
	char	*path;			/* name of file whose ACL to alter */
#else

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fsetaclentry
#pragma _HP_SECONDARY_DEF _fsetaclentry fsetaclentry
#define fsetaclentry _fsetaclentry
#endif

PROC int fsetaclentry (fd,  uid, gid, mode)
	int	fd;			/* file descriptor whose ACL to alter */
#endif
	uid_t	uid;			/* which entry		*/
	gid_t	gid;
	int	mode;			/* new mode value	*/
{
REG	int	nentries;		/* number in acl[]	*/
REG	int	entry;			/* current acl[] index	*/
	struct	acl_entry acl [NACLENTRIES];

/*
 * GET FILE'S EXISTING ACL:
 */

#ifndef FVERSION
	if ((nentries = getacl (path, NACLENTRIES, acl)) < 0)
#else
	if ((nentries = fgetacl (fd,  NACLENTRIES, acl)) < 0)
#endif
	    return (ERR_GETACL);

/*
 * EVALUATE SPECIAL UID AND GID VALUES:
 */

	if ((uid == ACL_FILEOWNER)
#ifndef FVERSION
	 && ((uid = GetFileID (path, /* user = */ TRUE)) < 0))
#else
	 && ((uid = GetFileID (fd,   /* user = */ TRUE)) < 0))
#endif
	    return (uid);

	if ((gid == ACL_FILEGROUP)
#ifndef FVERSION
	 && ((gid = GetFileID (path, /* user = */ FALSE)) < 0))
#else
	 && ((gid = GetFileID (fd,   /* user = */ FALSE)) < 0))
#endif
	    return (gid);

/*
 * FIND ENTRY IN ACL:
 * 
 * If entry is found, (0 <= entry < nentries), else (entry == nentries).
 */

	for (entry = 0; entry < nentries; entry++)
	    if ((uid == acl [entry] . uid) && (gid == acl [entry] . gid))
		break;

/*
 * DELETE ENTRY:
 *
 * Note that this code doesn't care if the deleted entry is base or optional.
 * If it's a base entry, setacl() will not actually delete it, just zero its
 * mode, as documented.
 */

	if (mode == MODE_DEL)
	{
	    if (entry >= nentries)		/* not found */
		return (ERR_DELETE);

	    while (++entry < nentries)		/* start at next, if any */
		acl [entry - 1] = acl [entry];	/* copy each to previous */

	    nentries--;				/* get rid of one */
	}

/*
 * MODIFY ENTRY'S MODE, OR ADD ENTRY:
 *
 * Note that setacl() will catch an invalid mode value later.
 * Also, it's OK to add a new entry to the end of the ACL because the kernel
 * will order them as necessary.
 */

	else if (entry < nentries)		/* existing entry */
	    (acl [entry] . mode) = mode;

	else if (++nentries > NACLENTRIES)	/* too many, can't add one */
	    return (ERR_MAXENTRIES);

	else					/* add at end of list */
	{
	    (acl [entry] . uid)  = (aclid_t) uid;  /* entry was == nentries */
	    (acl [entry] . gid)  = (aclid_t) gid;
	    (acl [entry] . mode) = mode;
	}

/*
 * SET REVISED ACL:
 */

#ifndef FVERSION
	return ((setacl (path, nentries, acl) < 0) ? ERR_SETACL : 0);
#else
	return ((fsetacl (fd,  nentries, acl) < 0) ? ERR_SETACL : 0);
#endif

} /* [f]setaclentry */


/************************************************************************
 * G E T   F I L E   I D
 *
 * Given a filename and a type of ID value (user or group), return the file's
 * owner or group ID after calling stat() on the file.  Return ERR_STAT if
 * stat() fails.  (Any negative return looks like an error.)
 * 
 * This routine could be more efficient by only calling stat() once for any
 * file.  However, the caller would have to somehow inform it of being called
 * again on the same file.  Without recording filenames, this routine can't
 * safely assume that a call to get a group ID is for the same file as a
 * previous call to get a user ID).  Besides, needing both the user and group
 * IDs for one file should be rare (the combination is not a base entry), and
 * the stat buffer for the file is cached for a while by the kernel anyway.
 * 
 * For FVERSION, replace "filename" with "file descriptor" and "stat" with
 * "fstat" in the above text.
 */

#ifndef FVERSION

PROC static int GetFileID (path, isuser)
	char	*path;		/* file to stat()	*/
#else

PROC static int GetFileID (fd, isuser)
	int	fd;		/* file to fstat()	*/
#endif
	int	isuser;		/* flag: user/group ID	*/
{
static	struct	stat statbuf;	/* only allocate once	*/

#ifndef FVERSION
	if (stat (path, & statbuf) < 0)
#else
	if (fstat (fd,  & statbuf) < 0)
#endif
	    return (ERR_STAT);

	return (isuser ? statbuf.st_uid : statbuf.st_gid);

} /* GetFileID */
