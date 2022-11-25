/* @(#) $Revision: 64.3 $ */    
/*
 * Library routine to change file owner and/or group represented in access
 * control list (ACL) (i.e. perform ownership translation).
 *
 * POINTER to design documents, technical rationales, etc:  24 documents were
 * placed in shared sources (/hpux/shared/INFO/release_docs/6.5/ACLs/) in 8902.
 */

#ifdef _NAMESPACE_CLEAN
#define chownacl _chownacl
#endif /* _NAMESPACE_CLEAN */

#include <sys/acl.h>

/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define	PROC				/* null; easy to find procs */
#define	FALSE	0
#define	TRUE	1
#define	REG	register

#define	NOENTRY	(-1)


/************************************************************************
 * C H O W N   A C L
 *
 * See the manual entry (chownacl(3)) for details.
 */

#ifdef _NAMESPACE_CLEAN
#undef chownacl
#pragma _HP_SECONDARY_DEF _chownacl chownacl
#define chownacl _chownacl
#endif /* _NAMESPACE_CLEAN */

PROC void chownacl (nentries, acl, olduid, oldgid, newuid, newgid)
	int	nentries;		/* number in ACL */
	struct	acl_entry acl[];
	uid_t	olduid;			/* previous IDs	*/
	gid_t	oldgid;
	uid_t	newuid;			/* new IDs */
	gid_t	newgid;
{
	int	fixuid = (olduid != newuid);	/* flag: fixes needed?	*/
	int	fixgid = (oldgid != newgid);
REG	int	entry;				/* index in acl[]	*/
	uid_t	uidentry = NOENTRY;		/* entries to change	*/
	gid_t	gidentry = NOENTRY;
REG	uid_t	uid;				/* fast values		*/
REG	gid_t	gid;

/*
 * CHECK IF NO CHANGES NEEDED:
 *
 * This is just to save time, since no changes would result anyway.
 */

	if (! (fixuid || fixgid))		/* no changes needed */
	    return;

/*
 * EXAMINE ACL FOR CHANGES NEEDED:
 */

	for (entry = 0; entry < nentries; entry++)
	{
	    uid = (uid_t) (acl [entry] . uid);
	    gid = (gid_t) (acl [entry] . gid);

/*
 * CHECK FOR OLD OR NEW USER ENTRY:
 */

	    if (fixuid && (gid == ACL_NSGROUP))	/* maybe entry of interest */
	    {
		if (uid == newuid)		/* no need to change users */
		{
		    if (! fixgid)		/* might as well quit */
			return;

		    fixuid = FALSE;
		}

		if (uid == olduid)		/* entry to maybe change */
		    uidentry = entry;		/* remember its index	 */
	    }

/*
 * CHECK FOR OLD OR NEW GROUP ENTRY:
 *
 * This code parallels the previous.
 */

	    if (fixgid && (uid == ACL_NSUSER))	/* maybe entry of interest */
	    {
		if (gid == newgid)		/* no need to change groups */
		{
		    if (! fixuid)		/* might as well quit */
			return;

		    fixgid = FALSE;
		}

		if (gid == oldgid)		/* entry to maybe change */
		    gidentry = entry;		/* remember its index	 */
	    }
	} /* for */

/*
 * MAKE ANY CHANGES NEEDED:
 */

	if (fixuid && (uidentry != NOENTRY))
	    (acl [uidentry] . uid) = (aclid_t) newuid;

	if (fixgid && (gidentry != NOENTRY))
	    (acl [gidentry] . gid) = (aclid_t) newgid;

} /* chownacl */
