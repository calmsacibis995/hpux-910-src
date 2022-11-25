/* @(#) $Revision: 66.1 $ */    
/*
 * Library routine to convert access control list (ACL) from array of
 * structures to string (symbolic form).
 *
 * POINTER to design documents, technical rationales, etc:  24 documents were
 * placed in shared sources (/hpux/shared/INFO/release_docs/6.5/ACLs/) in 8902.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define acltostr _acltostr
#define getgrgid _getgrgid
#define strcpy _strcpy
#define strncpy _strncpy
#define ltoa _ltoa
#define getpwuid _getpwuid
#endif


#include <unistd.h>		/* for *_OK values */
#include <pwd.h>		/* for getpwuid()  */
#include <grp.h>		/* for getgrgid()  */
#include <acllib.h>

/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define	USERNAMEMAX	 8	/* truncation sizes */
#define	GROUPNAMEMAX	 8
#define	NAMEMAX		20	/* allow name or number, with margin */

#define	NONSPECIFIC "%"		/* special user name / group name */

#define	PROC			/* null; easy to find procs */
#define	NULL	0
#define	CHNULL	('\0')
#define	CPNULL	((char *) NULL)
#define	REG	register

static char *UIDtoStr();
static char *GIDtoStr();
static char *ModetoStr();
static char *StrCpy();

extern char *ltoa();

/*********************************************************************
 * SIZE OF STATIC RETURN STRING:
 *
 * Short form entry:	(username.groupname,mode)
 * Long form entry:	mode<space>username.groupname<newline>
 */

#define	SHORTENTRY (1 + USERNAMEMAX + 1 + GROUPNAMEMAX + 1 + 3 + 1)
#define	LONGENTRY  (3 + 1 + USERNAMEMAX + 1 + GROUPNAMEMAX + 1)

/* STRMAX uses SHORTENTRY since, by inspection, it's bigger than LONGENTRY */

#define	STRMAX ((NACLENTRIES * SHORTENTRY) + 1)  /* plus trailing null */


/************************************************************************
 * A C L   T O   S T R
 *
 * See the manual entry (acltostr(3)) for details.
 */


/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef acltostr
#pragma _HP_SECONDARY_DEF _acltostr acltostr
#define acltostr _acltostr
#endif

PROC char * acltostr (nentries, acl, form)
	int	nentries;		/* number in acl[]	*/
	struct	acl_entry acl[];	/* to convert		*/
	int	form;			/* of desired output	*/
{
static	char	string [STRMAX];	/* result to return	*/
REG	char	*cp;			/* place in string[]	*/
REG	int	entry;			/* place in acl[]	*/

/*
 * CHECK PARAMETERS:
 */

	if ((nentries > NACLENTRIES)
	 || ((form != FORM_SHORT) && (form != FORM_LONG)))
	{
	    return (CPNULL);
	}

/*
 * BUILD SHORT-FORM ACL STRING:
 */

	*(cp = string) = CHNULL;	/* default/initial null string */

	if (form == FORM_SHORT)
	{
	    for (entry = 0; entry < nentries; entry++)
	    {
		*cp++ = '(';
		cp    = StrCpy (cp, UIDtoStr (acl [entry] . uid));
		*cp++ = '.';
		cp    = StrCpy (cp, GIDtoStr (acl [entry] . gid));
		*cp++ = ',';
		cp    = StrCpy (cp, ModetoStr (acl [entry] . mode));
		*cp++ = ')';
	    }
	}

/*
 * BUILD LONG-FORM ACL STRING:
 */

	else
	{
	    for (entry = 0; entry < nentries; entry++)
	    {
		cp    = StrCpy (cp, ModetoStr (acl [entry] . mode));
		*cp++ = ' ';
		cp    = StrCpy (cp, UIDtoStr (acl [entry] . uid));
		*cp++ = '.';
		cp    = StrCpy (cp, GIDtoStr (acl [entry] . gid));
		*cp++ = '\n';
	    }
	}

/*
 * FINISH UP:
 */

	*cp = CHNULL;
	return (string);

} /* acltostr */


/************************************************************************
 * U I D   T O   S T R
 *
 * Convert a user ID value to a string equivalent.  If a non-specific value,
 * return NONSPECIFIC, else the user name, if known (truncated to USERNAMEMAX),
 * else a string representation of the ID number.  The returned value must be
 * considered read-only by the caller, and is only valid until the next call.
 *
 * It might be more efficient if this routine cached values already seen, but
 * in ACLs repetition of the same uid should be the exception, not the norm,
 * and the whole password file is likely to be cached by the system anyway.
 */

PROC static char * UIDtoStr (uid)
	aclid_t	uid;
{
static	char	string [NAMEMAX];		/* return value	   */
	struct	passwd *pwd;			/* from getpwuid() */

	if (uid == ACL_NSUSER)
	    return (NONSPECIFIC);

	pwd = getpwuid ((int) uid);		/* get user name */

	/*
	 * It appears getpwuid() never leaves a file open, so there is
	 * no need to call endpwent().
	 */
	if (pwd == (struct passwd *) NULL)	/* unknown name	*/
	    strcpy (string, ltoa (uid));
	else					/* use known name */
	{
	    strncpy (string, pwd -> pw_name, USERNAMEMAX + 1);
	    string [USERNAMEMAX] = CHNULL; /* make sure of termination */
	}

	return (string);

} /* UIDtoStr */


/************************************************************************
 * G I D   T O   S T R
 *
 * Convert a group ID value to a string equivalent.  If a non-specific value,
 * return NONSPECIFIC, else the group name, if known (truncated to
 * GROUPNAMEMAX), else a string representation of the ID number.  The returned
 * value must be considered read-only by the caller, and is only valid until
 * the next call.
 *
 * This routine parallels UIDtoStr(); only the specific calls and value names
 * differ (but enough that it's best to use two routines).
 */

PROC static char * GIDtoStr (gid)
	aclid_t	gid;
{
static	char	string [NAMEMAX];		/* return value	   */
	struct	group *grp;			/* from getgrgid() */

	if (gid == ACL_NSGROUP)
	    return (NONSPECIFIC);

	grp = getgrgid ((int) gid);		/* get group name */

	/*
	 * It appears getgrgid() never leaves a file open, so there is
	 * no need to call endgrent().
	 */
	if (grp == (struct group *) NULL)	/* unknown name	*/
	    strcpy (string, ltoa (gid));
	else					/* use known name */
	{
	    strncpy (string, grp -> gr_name, GROUPNAMEMAX + 1);
	    string [GROUPNAMEMAX] = CHNULL; /* make sure of termination */
	}

	return (string);

} /* GIDtoStr */


/************************************************************************
 * M O D E   T O   S T R
 *
 * Convert a mode value to a string equivalent:  <r|-><w|-><x|-><CHNULL>.  The
 * returned value must be considered read-only by the caller, and is only valid
 * until the next call.
 */

PROC static char * ModetoStr (mode)
	int	mode;
{
static	char string [4];	/* return value */

	string [0] = (mode & R_OK) ? 'r' : '-';
	string [1] = (mode & W_OK) ? 'w' : '-';
	string [2] = (mode & X_OK) ? 'x' : '-';
	string [3] = CHNULL;

	return (string);

} /* ModetoStr */


/************************************************************************
 * S T R   C P Y
 *
 * Given destination and source string pointers, copy from the source to the
 * destination up to but not including CHNULL, and return a pointer to the next
 * unwritten location in the destination.  This is faster than the supported
 * strcat(3) for repeated calls because it helps the caller advance through
 * the string, not restart from the beginning with each call.
 */

PROC static char * StrCpy (dest, source)
REG	char	*dest, *source;
{
	while (*source != CHNULL)
	    *dest++ = *source++;

	return (dest);

} /* StrCpy */
