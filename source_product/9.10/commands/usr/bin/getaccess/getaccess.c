static char *HPUX_ID = "@(#) $Revision: 64.1 $";
/*
 * List access rights to file(s).
 * See the manual entry (getaccess(1)) for details.
 * POINTER to design documents, technical rationales, etc:  24 documents were
 * placed in shared sources (/hpux/shared/INFO/release_docs/6.5/ACLs/) in 8902.
 *
 * NLS notes:  This program supports message catalogs if compiled with -DNLS.
 * Initialized string arrays begin at macro-ized message numbers and some space
 * has been left for them to grow.  Some unfortunate caveats:
 *
 * - findmsg(1) requires "catgets(" instead of the more readable "catgets ("
 *
 * - findmsg requires all parts of each catgets() call, or comment plus
 *   quoted string, to be on a single line, which makes the source less
 *   readable when it exceeds column 80
 *
 * - catopen(3) requires embedding the name of the command in the source
 *
 * This program has no other needs for NLS-smartness because it only deals
 * with 7-bit data, including user and group names.
 */

#include <sys/types.h>		/* for stat.h	*/
#include <sys/stat.h>		/* for statbuf	*/
#include <sys/getaccess.h>
#include <unistd.h>		/* for *_OK values */
#include <limits.h>		/* for NGROUPS_MAX */
#include <stdio.h>
#include <pwd.h>		/* for getpwnam()  */
#include <grp.h>		/* for getgrnam()  */

void	exit();
char	*strerror();

extern	int	errno;		/* system error number */

/*********************************************************************
 * NLS OVERHEAD:
 */

#ifndef NLS

#define catgets(i, sn, mn, s) (s)		/* make it a no-op */

#else

#include <nl_types.h>

char *catgets();

nl_catd	nlmsg_fd;		/* file descriptor */
#define NL_SETN 1		/* set number	   */

#endif


/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define MYNAME	"getaccess"		/* bad practice but NLS requires it */

#define	PROC				/* null; easy to find procs */
#define	FALSE	0
#define	TRUE	1
#define	CHNULL	('\0')
#define	CPNULL	((char *) NULL)
#define	REG	register

char	*myname;			/* how program was invoked	*/
int	uflag = FALSE;			/* -u (username) option		*/
int	gflag = FALSE;			/* -g (groups list) option	*/
int	rflag = FALSE;			/* -r (real IDs) option		*/
int	nflag = FALSE;			/* -n (numeric output) option	*/

/*
 * Special uid/gid value meaning to use file's ID value:
 *
 * FILEIDVALUE is chosen to not collide with special values in supported
 * include files.
 */

#define	FILEIDCHAR  '@'
#define	FILEIDVALUE 65520

int	errcount = 0;			/* error count			*/

#define	WARNING  0			/* non-error value for Error()	*/
#define	NOERRNO  0			/* non-errno value for Error()	*/
#define	OKEXIT   0			/* in case of no error		*/
#define	ERREXIT  1			/* in case of startup error	*/
#define	WARNEXIT 2			/* in case of warnings		*/


/*********************************************************************
 * USAGE MESSAGE:
 *
 * NLMN_USAGE is the base message number for Usage().  It must match the
 * catgets comments used by findmsg(1).
 */

#define	NLMN_USAGE 1

char *usage[] = {

/* catgets 1 */  "usage: %s [-u user] [-g group[,group]...] [-n] file ...",
/* catgets 2 */  "       %s -r [-n] file ...",
/* catgets 3 */  "-u determine access for given user instead of caller's effective ID",
/* catgets 4 */  "-g determine access for given groups instead of caller's groups",
/* catgets 5 */  "-r do access checks using caller's real IDs instead of effective",
/* catgets 6 */  "-n print numeric value of access rights (0..7) instead of string",
/* catgets 7 */  "user or group can be \"@\" for each file's group ID in turn",

    CPNULL,
};


/************************************************************************
 * M A I N
 */

PROC main (argc, argv)
REG	int	argc;
REG	char	**argv;
{
extern	int	optind;		/* from getopt()   */
extern	char	*optarg;	/* from getopt()   */
REG	int	option;		/* option "letter" */

REG	uid_t	uid	= UID_EUID;		/* uid value to use	*/
REG	int	ngroups	= NGROUPS_EGID_SUPP;	/* number in gidset[]	*/
	gid_t	*gidset;			/* gid values, if given	*/

#ifdef NLS
	nlmsg_fd = catopen (MYNAME, 0);		/* ignore failure */
#endif

/*
 * PARSE OPTIONS:
 */

	myname = *argv;

	while ((option = getopt (argc, argv, "u:g:rn")) != EOF)
	{
	    switch (option)
	    {
	    case 'u':	uid   = StrToID (& optarg, /* isuser = */ TRUE);
			uflag = TRUE;
			break;

	    case 'g':	ngroups	= StrToGIDs (optarg, & gidset);
			gflag	= TRUE;
			break;
	    
	    case 'r':	uid	= UID_RUID;
			ngroups	= NGROUPS_RGID_SUPP;
			rflag	= TRUE;
			break;
	    
	    case 'n':	nflag = TRUE;
			break;
	  
	    default:	Usage();
	    }
	} /* while */

	argc -= optind;			/* skip options	*/
	argv += optind;

/*
 * MORE ARGUMENT CHECKS:
 */

	if ((uflag || gflag) && rflag)
	{
	    Error (ERREXIT, NOERRNO, 15,
		    /* catgets 15 */ "can't give -u or -g along with -r");
	}

	if (argc < 1)			/* require at least one filename */
	    Usage();

/*
 * HANDLE EACH FILENAME ARGUMENT:
 */

	while (argc-- > 0)
	    OneFile (*argv++, uid, ngroups, gidset);
	    /* if any errors, sets errcount */

#ifdef NLS
	catclose (nlmsg_fd);			/* just to be nice */
#endif

	exit (errcount ? WARNEXIT : OKEXIT);	/* set by OneFile() */

} /* main */


/************************************************************************
 * U S A G E
 *
 * Convert usage messages (char *usage[]) to native language, print them to
 * stderr, and exit with ERREXIT.  Each message is followed by a newline.
 * Array indices start at 0, but NLS message numbers start at NLMN_USAGE.
 * Note that this call of catgets() is not picked up by findmsg(1).
 */

PROC Usage()
{
REG	int	which = 0;		/* current line */

	while (usage [which] != CPNULL)
	{
	    fprintf (stderr,
		     catgets (nlmsg_fd, NL_SETN, (NLMN_USAGE + which),
			      usage [which]),
		     myname);

	    putc ('\n', stderr);
	    which++;
	}

	exit (ERREXIT);

} /* Usage */


/************************************************************************
 * E R R O R
 *
 * Convert an error message to native language, print it to stderr and, if
 * given a non-zero exit value, exit with that value, else increment global
 * errcount and return.  The converted message is preceded by "<myname>:  "
 * using global char *myname, and followed by a newline.  If errno (system
 * error number passed in, or NOERRNO) is not NOERRNO, a relevant message is
 * appended before the newline.  Note that having the caller pass in a copy of
 * errno also means it is immune to being changed as a side effect before being
 * printed.
 */

/* VARARGS4 */
PROC Error (exitvalue, errno, msgnum, message, arg1, arg2, arg3, arg4)
	int	exitvalue;	/* or zero for warning	    */
	int	errno;		/* system errno if relevant */
	int	msgnum;		/* NLS message number	    */
	char	*message;
	long	arg1, arg2, arg3, arg4;
{
	fprintf (stderr, "%s: ", myname);

	fprintf (stderr, catgets(nlmsg_fd, NL_SETN, msgnum, message),
		 arg1, arg2, arg3, arg4);

	if (errno != NOERRNO)
	{
	    fprintf (stderr, ": %s", strerror (errno));
	    fprintf (stderr, catgets(nlmsg_fd, NL_SETN, 30, " (errno = %d)"),
		     errno);
	}

	putc ('\n', stderr);

	if (exitvalue)
	    exit (exitvalue);

	errcount++;

} /* Error */


/************************************************************************
 * S T R   T O   G I D S
 *
 * Given a string alleged to contain a comma-separated list of group IDs
 * (names, numbers, or FILEIDCHAR), and a pointer to an array of gid_t's,
 * parse the list into a local, static array (maximum of NGROUPS_MAX elements),
 * set the pointer to it, and return the number of elements (one or more).  If
 * anything goes wrong, error out.
 *
 * Note that each call to this routine replaces the list saved from previous
 * calls, if any.  Also, StrToID() catches a null string as a null ID.
 */

PROC int StrToGIDs (string, gidsetp)
	char	*string;		/* to parse  */
	gid_t	**gidsetp;		/* to return */
{
static	gid_t	gidset [NGROUPS_MAX];	/* actual data		*/
	int	ngroups = 0;		/* number in gidset[]	*/

	do {
	    if (ngroups >= NGROUPS_MAX)
	    {
		Error (ERREXIT, NOERRNO, 17,
			/* catgets 17 */ "more than %d groups not allowed",
			NGROUPS_MAX);
	    }

	    gidset [ngroups++] = StrToID (& string, /* isuser = */ FALSE);

	} while (*string++ != CHNULL);		/* till done, else skip "," */

	*gidsetp = gidset;
	return (ngroups);

} /* StrToGIDs */


/************************************************************************
 * S T R   T O   I D
 *
 * Given a pointer to a string alleged to contain a user or group ID (name,
 * number, or FILEIDCHAR) ending in comma (for group ID only) or CHNULL, and a
 * type flag (user or group), convert the ID to a numeric value, update the
 * string pointer to the character past the consumed part, and return the
 * value.  Error out if anything goes wrong.
 *
 * For an ID of FILEIDCHAR, return FILEIDVALUE.  There's a small chance the ID
 * string might be a number with the same value -- too bad -- it should be an
 * invalid user/group ID value anyway.
 */

PROC int StrToID (stringp, isuser)
	char	**stringp;		/* to parse and update	*/
	int	isuser;			/* flag: user or group?	*/
{
REG	char	*cp	= *stringp;	/* place in string	*/
REG	char	*cpnext	= *stringp;	/* past end of name	*/
	int	iscomma	= FALSE;	/* name ends in comma?	*/
	struct	passwd *pwd;		/* from getpwnam()	*/
	struct	group  *grp;		/* from getgrnam()	*/
	int	result;			/* to return		*/

/*
 * FIND CHARACTER PAST NAME:
 */

	while (*cpnext != CHNULL)
	{
	    if ((! isuser) && (*cpnext == ','))	 /* alternate terminator */
	    {
		iscomma = TRUE;
		*cpnext = CHNULL;		/* fix before returning! */
		break;
	    }

	    cpnext++;
	}

	if (cp == cpnext)		/* null name */
	{
	    Error (ERREXIT, NOERRNO, isuser ? 18 : 19,
		    isuser ? /* catgets 18 */ "null user ID not allowed" :
			     /* catgets 19 */ "null group ID not allowed");
	}

/*
 * CHECK AND CONVERT NAME:
 */

	if ((cp [0] == FILEIDCHAR) && (cp [1] == CHNULL))
	{
	    result = FILEIDVALUE;
	}
	else if (isuser ? ((pwd = getpwnam (cp)) != (struct passwd *) NULL)
			: ((grp = getgrnam (cp)) != (struct group  *) NULL))
	{
	    /*
	     * It appears getpwnam() and getgrnam() never leave a file open,
	     * so there is no need to call endpwent() or endgrent().
	     */

	    result = isuser ? (pwd -> pw_uid) : (grp -> gr_gid);

	    /* a negative value would appear to be an error; that's OK */
	}
	else					/* try it as a number	*/
	if ((result = StrToNum (cp)) < 0)	/* not a number, either	*/
	{
	    Error (ERREXIT, NOERRNO, isuser ? 20 : 21,
		    isuser ? /* catgets 20 */ "invalid user ID \"%s\"" :
			     /* catgets 21 */ "invalid group ID \"%s\"",
		    cp);
	}

/*
 * FINISH UP:
 */

	if (iscomma)
	    *cpnext = ',';		/* restore value */

	*stringp = cpnext;		/* advance to terminator */
	return (result);

} /* StrToID */


/************************************************************************
 * S T R   T O   N U M
 *
 * Given a pointer to a null-terminated string, return the numeric equivalent,
 * base-10, of the digits in the string (>= 0) or -1 if the string contains
 * anything other than a digit.  Doesn't handle integer overflow; it might look
 * like an error return (but that's OK).
 * 
 * I was unable to find (exactly) this routine in standard libraries.
 */

PROC int StrToNum (string)
REG	char	*string;
{
REG	int	result = 0;		/* to return */

	while (*string != CHNULL)
	{
	    if ((*string < '0') || (*string > '9'))
		return (-1);

	    result = (result * 10) + ((*string++) - '0');
	}

	return (result);

} /* StrToNum */


/************************************************************************
 * O N E   F I L E
 *
 * Given a filename, user ID, number of groups, and set of group IDs, check
 * access to the named file and print a summary to standard output.  First
 * convert any user or group ID of FILEIDVALUE to the file's appropriate value
 * if needed (note, for groups, in a copy of gidset[], not the original).  In
 * case of error, print a warning message and return.
 */

PROC OneFile (filename, uid, ngroups, gidset)
REG	char	*filename;	/* to check		*/
	uid_t	uid;		/* value to use		*/
REG	int	ngroups;	/* number in gidset[]	*/
	gid_t	gidset[];	/* group IDs to use	*/
{
static	gid_t	gidset2 [NGROUPS_MAX];	/* local copy of gidset[]	*/
REG	int	group;			/* current element in gidset2[]	*/
	int	mode;			/* access modes available	*/

/*
 * SUBSTITUTE FILE OWNER ID FOR PLACEHOLDER:
 */

	if ((uid == FILEIDVALUE)
	 && ((uid = GetFileID (filename, /* isuser = */ TRUE)) < 0))
	{
	    return;			/* error message already given */
	}

/*
 * SUBSTITUTE FILE GROUP ID FOR PLACEHOLDERS:
 *
 * Note that if ngroups is a special value, it's negative, so the loop doesn't
 * run at all, which is fine because gidset[] is later ignored anyway.
 */

	for (group = 0; group < ngroups; group++)
	{
	    if (gidset [group] != FILEIDVALUE)
		gidset2 [group] = gidset [group];
	    else
	    if ((gidset2 [group] = GetFileID (filename, /* isuser = */ FALSE))
		< 0)
	    {
		return;			/* error message already given */
	    }
	}

/*
 * GET FILE ACCESS INFORMATION:
 */

	if ((mode = getaccess (filename, uid, ngroups, gidset2,
				 /* label = */ (void *) NULL,
				 /* label = */ (void *) NULL)) < 0)
	{
	    Error (WARNING, errno, 22, /* catgets 22 */ "file \"%s\"",
		   filename);

	    return;
	}

/*
 * PRINT INFORMATION:
 */

	if (nflag)
	    printf ("%o %s\n", mode, filename);
	else
	{
	    printf ("%c%c%c %s\n",
		    (mode & R_OK) ? 'r' : '-',
		    (mode & W_OK) ? 'w' : '-',
		    (mode & X_OK) ? 'x' : '-',
		    filename);
	}

} /* OneFile */


/************************************************************************
 * G E T   F I L E   I D
 *
 * Given a filename and a type flag (user or group), get and return the file's
 * user or group ID.  In case of error, print a warning message and return -1.
 * 
 * It might be more efficient if this routine cached values already seen, but
 * repetition of the same uid or gid should be the exception, not the norm (for
 * one file), and the whole password or group file is likely to be cached by
 * the system anyway.
 */

PROC int GetFileID (filename, isuser)
	char	*filename;	/* to get IDs for	*/
	int	isuser;		/* flag: user or group?	*/
{
static	struct	stat statbuf;	/* only allocate once */

	if (stat (filename, & statbuf) < 0)
	{
	    Error (WARNING, errno, 23, /* catgets 23 */ "file \"%s\"",
		   filename);

	    return (-1);
	}

	return (isuser ? statbuf.st_uid : statbuf.st_gid);

	/* a negative value would appear to be an error; that's OK */

} /* GetFileID */
