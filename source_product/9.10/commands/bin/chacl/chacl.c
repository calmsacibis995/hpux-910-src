static char *HPUX_ID = "@(#) $Revision: 64.2 $";
/*
 * Change access control list (ACL) on files(s).
 * See the manual entry (chacl(1)) for details.
 * POINTER to design documents, technical rationales, etc:  24 documents were
 * placed in shared sources (/hpux/shared/INFO/release_docs/6.5/ACLs/) in 8902.
 *
 * NLS notes:  This program supports message catalogs if compiled with -DNLS.
 * Initialized string arrays begin at macro-ized message numbers and some space
 * has been left for them to grow.  Some unfortunate caveats:
 *
 * - findmsg(1) requires "catgets(" instead of the more readable "catgets ("
 *
 * - findmsg requires constant strings, not more-efficient global pointers to
 *   constant strings, or even just more-maintainable macros for strings
 *
 * - findmsg requires all parts of each catgets() call, or comment plus
 *   quoted string, to be on a single line, which makes the source less
 *   readable when it exceeds column 80
 *
 * - catopen(3) requires embedding the name of the command in the source
 *
 * This program has no other needs for NLS-smartness because ACL strings
 * contain only 7-bit data, including user and group names.
 */

#include <sys/types.h>		/* for stat.h		*/
#include <sys/stat.h>		/* for struct stat	*/
#include <unistd.h>		/* supports MODEMASK	*/
#include <stdio.h>
#include <acllib.h>		/* ACL_DELOPT comes from acl.h via this file */

extern	int	errno;		/* system error number */

void	exit();
char	*strerror();

/*********************************************************************
 * NLS OVERHEAD:
 */

#ifndef NLS

#define catgets(i, sn, mn, s) (s)		/* make it a no-op */

#else

#include <nl_types.h>

char *catgets();
char *catgetmsg();

nl_catd	nlmsg_fd;		/* file descriptor */
#define NL_SETN 1		/* set number	   */

#endif

/*
 * Some messages which appear more than once:
 */

char	*msg54 = /* catgets 54 */ "file \"%s\": can't apply ACL: %s: \"%s\"";
char	*msg55 = /* catgets 55 */ "file \"%s\"";


/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define MYNAME	"chacl"			/* bad practice but NLS requires it */

#define	PROC				/* null; easy to find procs */
#define	FALSE	0
#define	TRUE	1
#define	CHNULL	('\0')
#define	CPNULL	((char *) NULL)
#define	REG	register

#define	STDIN	(fileno (stdin))
#define	NO_OPT	'#'			/* option if none given by user	*/

char	*myname;			/* how program was invoked	*/
int	perfile;			/* flag:  reparse ACL per file?	*/

/*
 * These are globals for simplicity and efficiency.  The two different kinds of
 * ACL structure pointers are kept separate rather than passing and casting a
 * (void *) through common code.
 */

struct	acl_entry	acl	[NACLENTRIES];	/* exact (file) ACL */
struct	acl_entry_patt	aclpatt	[NACLENTRIES];	/* pattern ACL	    */

uid_t	fromuid;			/* for -f option */
gid_t	fromgid;

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

/* catgets  1 */  "usage:",
/* catgets  2 */  "%s acl file ...           modify existing ACLs (add entries or modify modes)",
/* catgets  3 */  "%s -r acl file ...        replace old ACLs with given ACL on files",
/* catgets  4 */  "%s -d aclpatt file ...    delete specified entries, if present, from ACLs",
/* catgets  5 */  "%s -f fromfile tofile ... copy ACL from fromfile to tofiles; implies -r",
/* catgets  6 */  "%s -[z|Z|F] file ...      -z zap (delete) optional entries in ACLs",
/* catgets  7 */  "                             -Z zap (delete) all entries (zero access)",
/* catgets  8 */  "                             -F fold optional entries to base entries",
/* catgets  9 */  "",
/* catgets 10 */  "Operator form ACL input example:",
/* catgets 11 */  "",
/* catgets 12 */  "    %s 'ajs.adm=7, @.%%=rw, ggd.%%+w-x, %%.@-rwx, %%.adm+x, %%.%%=0' files...",
/* catgets 13 */  "",
/* catgets 14 */  "Short form ACL input example:",
/* catgets 15 */  "",
/* catgets 16 */  "    %s '(ajs.adm,7)(@.%%,rw)(ggd.%%,-w-),(%%.@,0)(%%.adm,rx),(%%.%%,0)' files...",
/* catgets 17 */  "",
/* catgets 18 */  "%% means \"no specific user or group\"",
/* catgets 19 */  "@ means \"current file owner or group\"",
/* catgets 20 */  "* means \"any user or group, including %%\" (patterns only)",
/* catgets 21 */  "In patterns \"*\" can be used for wildcard mode values, or they can be absent.",

CPNULL,
};


/************************************************************************
 * M A I N
 *
 * For options where an ACL string is given, set either global acl[] or
 * aclpatt[] to hold the initially parsed ACL or ACL pattern.  In these cases,
 * reparsing, or just file user and group IDs, may be needed (see below); set
 * global int perfile accordingly.
 */

PROC main (argc, argv)
REG	int	argc;
REG	char	**argv;
{
extern	int	optind;				/* from getopt()   */
extern	char	*optarg;			/* from getopt()   */
	int	newopt;				/* from getopt()   */
	int	option = NO_OPT;		/* option "letter" */

	char	*aclstring;			/* given ACL string */
	int	nentries;			/* number in ACL    */

	int	isstdin;			/* flag: is standard input? */

#ifdef NLS
	nlmsg_fd = catopen (MYNAME, 0);		/* ignore failure */
#endif

/*
 * PARSE OPTIONS:
 */

	myname = *argv;

	while ((newopt = getopt (argc, argv, "rdfzZF")) != EOF)
	{

	/*
	 * If it later becomes necessary to add one or more non-exclusive
	 * options, here's how.  Be sure to indent the existing code.
	 *
	 *  switch (newopt)
	 *  {
	 *  case 'X':	Xflag = TRUE; break;
	 *
	 *  default:	<code below>  break; }
	 */

/*
 * HANDLE EXCLUSIVE OPTIONS:
 */

	    if (newopt == '?')			/* unrecognized */
		Usage();

	    if (option != NO_OPT)		/* only one is allowed */
	    {
		Error (ERREXIT, NOERRNO, 50,
			/* catgets 50 */ "only one of -rdfzZF is allowed at a time");
	    }

	    option = newopt;

	} /* while */

	argc -= optind;			/* skip options	*/
	argv += optind;

/*
 * CHECK ARGUMENTS FOR SIMPLE OPTIONS:
 */

	switch (option)
	{
	case 'z':
	case 'Z':
	case 'F':

	    if (argc < 1)		/* must be at least one filename */
		Usage();

	    break;

/*
 * CHECK ARGUMENTS FOR -f OPTION; GET ACL FROM fromfile:
 *
 * Local nentries and globals acl[], fromuid, and fromgid are set here for
 * later use.
 */

	case 'f':

	    if (argc-- < 2)		/* must be at least two filenames */
		Usage();

	    if (isstdin = ((argv [0] [0] == '-') && (argv [0] [1] == CHNULL)))
		*argv = "<stdin>";

	    if (((nentries = GetACL (isstdin, *argv)) < 0)
	     || (GetIDs (isstdin, *argv++, & fromuid, & fromgid) < 0))
	    {
		exit (ERREXIT);		/* error was already printed */
	    }

	    break;

/*
 * CHECK ARGUMENTS FOR OTHER OPTIONS, INCLUDING SYNTAX OF ACL STRING:
 *
 * The converted value in acl[] may or may not be reused later.  fuid and fgid
 * are set to special values to detect if "@" symbols are used, so later
 * reparsing or getting of user and group ID values may be needed for each
 * file.
 *
 * Note:  This code rejects ACLs with more than NACLENTRIES unique entries,
 * counting "@" symbols as different than any other user or group names, even
 * though such ACLs might "fold" to fewer entries for certain files.
 */

	case 'r':
	case 'd':
	default:			/* i.e., NO_OPT */

	    if (argc-- < 2)		/* need ACL string and filename */
		Usage();

	    if ((nentries = StrToACL (aclstring  = *argv++,
				     /* nentries = */ 0,
				     /* pattern	 = */ (option == 'd'),
				     ACL_FILEOWNER, ACL_FILEGROUP,
				     /* getids   = */ FALSE,
				     /* isstdin  = */ FALSE,	/* don't care */
				     /* filename = */ "",	/* not needed */
				     51,
				     /* catgets 51 */ "invalid ACL: %s%s: \"%s\""
				    )) < 0)
	    {
		exit (ERREXIT);
	    }

	    perfile = CheckPerFile (/* pattern = */ (option == 'd'), nentries);
	    break;

	} /* switch */

/*
 * HANDLE EACH FILENAME ARGUMENT:
 *
 * Note that aclstring is undefined and unused for some options.
 */

	while (argc-- > 0)
	    OneFile (option, *argv++, aclstring, nentries);

#ifdef NLS
	catclose (nlmsg_fd);			/* just to be nice */
#endif

	exit (errcount ? WARNEXIT : OKEXIT);	/* set by OneFile() */

} /* main */


/************************************************************************
 * C H E C K   P E R   F I L E
 *
 * Given a flag to check an exact or pattern ACL and the number of entries in
 * global acl[] or aclpatt[], check if reparsing or getting of file user and
 * group IDs is needed per file (true if any special user or group name symbols
 * were given), and return TRUE or FALSE accordingly.  If ACL_FILEOWNER and
 * ACL_FILEGROUP don't appear in the array, no reparsing is needed, hence
 * getting of file user and group IDs can also be avoided.  This is a
 * performance feature.
 */

PROC int CheckPerFile (pattern, nentries)
	int	pattern;
REG	int	nentries;
{
REG	int	entry;				/* current element */

	if (! pattern)
	{
	    for (entry = 0; entry < nentries; entry++)
	    {
		if ((acl [entry] . uid == ACL_FILEOWNER)
		 || (acl [entry] . gid == ACL_FILEGROUP))
		{
		    return (TRUE);
		}
	    }
	}
	else					/* dealing with a pattern */
	{
	    for (entry = 0; entry < nentries; entry++)
	    {
		if ((aclpatt [entry] . uid == ACL_FILEOWNER)
		 || (aclpatt [entry] . gid == ACL_FILEGROUP))
		{
		    return (TRUE);
		}
	    }
	}

	return (FALSE);

} /* CheckPerFile */


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
	    fprintf (stderr, catgets(nlmsg_fd, NL_SETN, 52, " (errno = %d)"),
		     errno);
	}

	putc ('\n', stderr);

	if (exitvalue)
	    exit (exitvalue);

	errcount++;

} /* Error */


/************************************************************************
 * O N E   F I L E
 *
 * Given the option being processed, a filename (may be "-" for standard
 * input), an ACL string (for options where it's provided; syntax was already
 * checked), and the number of entries in global acl[] or aclpatt[], apply the
 * ACL (or ACL pattern) or the desired operation to one file.
 */

PROC OneFile (option, filename, aclstring, nentries)
	int	option;
	char	*filename;
	char	*aclstring;
	int	nentries;
{
	int	isstdin;	/* flag: is standard input? */

/*
 * CHECK FOR STANDARD INPUT:
 */

	if (isstdin = ((filename [0] == '-') && (filename [1] == CHNULL)))
	    filename = "<stdin>";

/*
 * PERFORM REQUESTED OPERATION:
 *
 * Since st_basemode is not available for remote files, option "z" must use
 * setacl(), not stat() and chmod() (it shouldn't chmod (st_mode)).  So it is
 * limited as to the domain of files it can affect.  Option "Z" could use chmod
 * (file, 0) to do the job, so it would work over LAN, but it purposely uses
 * setacl() for consistency with "z" (it also only works on local files).
 *
 * Note:  subroutines handle warning messages.
 */

	switch (option)
	{
	case NO_OPT:	Modify  (isstdin, filename, aclstring);		  break;
	case 'r':	Replace (isstdin, filename, aclstring, nentries); break;
	case 'd':	Delete	(isstdin, filename, nentries);		  break;
	case 'f':	Copy	(isstdin, filename, nentries);		  break;
	case 'z':	SetACL	(isstdin, filename, ACL_DELOPT,	acl);	  break;
	case 'Z':	SetACL	(isstdin, filename, 0,		acl);	  break;
	case 'F':	Fold	(isstdin, filename);			  break;

	default:	Error (ERREXIT, NOERRNO, 53,
				/* catgets 53 */ "defect in OneFile (%c)\n",
				option);
	}

} /* OneFile */


/************************************************************************
 * M O D I F Y
 *
 * Given a flag whether to use STDIN or a named file, a filename, an ACL
 * string, and globals acl[] and int perfile, modify the ACL on one file.  If
 * anything goes wrong, a subroutine handles the warning message.
 *
 * aclstring must be reparsed for application to (modification of) each file's
 * existing ACL, even if (perfile == FALSE).  Use StrToACL() rather than
 * calling strtoacl() directly, even though aclstring was already parsed
 * earlier, in case the reparsing results in too many entries this time when it
 * didn't before (since the file already has non-zero entries).
 */

PROC Modify (isstdin, filename, aclstring)
	int	isstdin;
	char	*filename;
	char	*aclstring;
{
	int	nentries;			/* number in acl[] */

	if (((nentries = GetACL (isstdin, filename)) >= 0)	/* initially */

	 && ((nentries = StrToACL (aclstring, nentries,		/* new number */
				  /* pattern	= */ FALSE,
				  /* fuid, fgid	= */ 0, 0,	/* don't care */
				  /* getids	= */ perfile,	/* if needed  */
				  isstdin, filename,
				  54, msg54)) >= 0))
	{
	    SetACL (isstdin, filename, nentries, acl);
	}

} /* Modify */


/************************************************************************
 * R E P L A C E
 *
 * Given a flag whether to use STDIN or a named file, a filename, an ACL
 * string, the number of entries in global acl[], and global int perfile,
 * replace the ACL on one file.  If anything goes wrong, a subroutine handles
 * the warning message.
 *
 * aclstring must be reparsed for application to each file if (perfile ==
 * TRUE).  Use StrToACL() rather than calling strtoacl() directly, even though
 * aclstring was already parsed earlier, just in case any (unexpected) error
 * occurs this time.
 */

PROC Replace (isstdin, filename, aclstring, nentries)
	int	isstdin;
	char	*filename;
	char	*aclstring;
	int	nentries;
{
	if (perfile

	 && ((nentries = StrToACL (aclstring,
				  /* nentries	= */ 0,
				  /* pattern	= */ FALSE,
				  /* fuid, fgid	= */ 0, 0,	/* don't care */
				  /* getids	= */ TRUE,
				  isstdin, filename,
				  54, msg54)) < 0))
	{
	    return;
	}

	SetACL (isstdin, filename, nentries, acl);

} /* Replace */


/************************************************************************
 * D E L E T E
 *
 * Given a flag whether to use STDIN or a named file, a filename, the number of
 * entries in global aclpatt[] (to delete), and globals acl[], aclpatt[],
 * perfile, and aclentrystart[], delete entries from the ACL on one file.  If
 * anything goes wrong, print a warning message (possibly multiple times, once
 * per entry in the delete pattern which was not found in the file's ACL).
 *
 * Because it's easy, do not bother to set a new ACL on a file unless the old
 * one was modified.  This reduces auditing.
 */

PROC Delete (isstdin, filename, nentriespatt)
	int	isstdin;
	char	*filename;
REG	int	nentriespatt;
{
REG	int	nentries;			/* number in acl[]	*/
	uid_t	fuid;				/* fast file values	*/
	gid_t	fgid;
REG	int	entrypatt, entry, enttemp;	/* indices in arrays	*/
	int	changed = FALSE;		/* flag:  ACL changed?	*/
REG	int	matched;			/* flag:  match entry?	*/

extern	char	*aclentrystart[];		/* for error reports	*/
	char	*cpsave;			/* char to save		*/
	char	chsave;				/* saved char		*/

/*
 * GET FILE'S ACL (into global acl[]) AND FILE IDS:
 */

	if (((nentries = GetACL (isstdin, filename)) < 0)
	 || (perfile && (GetIDs (isstdin, filename, & fuid, & fgid) < 0)))
	{
	    return;				/* error was already printed */
	}

/*
 * DELETE MATCHING ENTRIES IN FILE'S ACL:
 *
 * Compare aclpatt[] with acl[] and delete entries from the latter.  For each
 * match, start at the next entry, if any, and copy (shift) each to the
 * previous slot.  This code happily deletes entries without caring if they are
 * base or optional.  Deleted base entries later get their modes set to zero by
 * setacl(), as documented.
 */

	for (entrypatt = 0; entrypatt < nentriespatt; entrypatt++)
	{					/* check each pattern entry */
	    matched = FALSE;
	    entry   = 0;

	    while (entry < nentries)		/* check against file entries */
	    {
		if (MatchEntry (aclpatt + entrypatt, acl + entry, fuid, fgid))
		{
		    changed = matched = TRUE;

		    for (enttemp = (entry + 1); enttemp < nentries; enttemp++)
			acl [enttemp - 1] = acl [enttemp];

		    nentries--;			/* deleted one */
		}
		else
		{
		    entry++;			/* go to next slot */
		}
	    }

/*
 * CHECK IF PATTERN ENTRY WAS MATCHED:
 */

	    if (! matched)
	    {
		cpsave  = aclentrystart [entrypatt + 1];
		chsave  = *cpsave;
		*cpsave = CHNULL;	/* temporarily end string */

		Error (WARNING, NOERRNO, 56,
			/* catgets 56 */ "file \"%s\": no matching ACL entry to delete for \"%s\"",
			filename, aclentrystart [entrypatt]);

		*cpsave = chsave;	/* repair ACL string */
	    }
	} /* for */

/*
 * SET REVISED ACL:
 */

	if (changed)
	    SetACL (isstdin, filename, nentries, acl);

} /* Delete */


/************************************************************************
 * M A T C H   E N T R Y
 *
 * Given pointers to a pattern entry and a file entry, and file user and group
 * IDs (may be random values if the caller knows they're not needed), return
 * TRUE if the entry pattern matches the file entry, FALSE otherwise.
 */

PROC int MatchEntry (pp, fp, fuid, fgid)
	struct	acl_entry_patt	*pp;		/* pattern entry */
	struct	acl_entry	*fp;		/* file entry	 */
	uid_t	fuid;
	gid_t	fgid;
{
/*
 * CHECK USER ID:
 *
 * If the pattern uid is not the wildcard value, then if it's the file owner
 * symbol the file entry's uid must match the file's uid, else it must match
 * the pattern uid.
 */

	if (((pp -> uid) != ACL_ANYUSER)
	 && ((fp -> uid) != ((pp -> uid == ACL_FILEOWNER) ? fuid : pp -> uid)))
	{
	     return (FALSE);
	}

/*
 * CHECK GROUP ID:
 */

	if (((pp -> gid) != ACL_ANYGROUP)
	 && ((fp -> gid) != ((pp -> gid == ACL_FILEGROUP) ? fgid : pp -> gid)))
	{
	     return (FALSE);
	}

/*
 * CHECK MODE:
 *
 * The bits set in both onmode and offmode must appear in mode.
 */

	if (((  (fp -> mode)  & MODEMASK & (pp -> onmode )) != (pp -> onmode))
	 || (((~(fp -> mode)) & MODEMASK & (pp -> offmode)) != (pp -> offmode)))
	{
	     return (FALSE);
	}

	return (TRUE);

} /* MatchEntry */


/************************************************************************
 * C O P Y
 *
 * Given a flag whether to use STDIN or a named file, a filename, the number of
 * entries in global acl[], and globals fromuid and fromgid, replace the ACL on
 * one file with acl[] after doing ownership translation if necessary.  If
 * anything goes wrong, a subroutine handles the warning message.
 */

PROC Copy (isstdin, filename, nentries)
	int	isstdin;
	char	*filename;
REG	int	nentries;
{
static	struct	acl_entry aclnew [NACLENTRIES];	/* only allocate once	*/
	uid_t	touid;				/* new user ID		*/
	gid_t	togid;				/* new group ID		*/
REG	int	entry;				/* index in acl[]	*/

/*
 * GET TOFILE'S IDS:
 */

	if (GetIDs (isstdin, filename, & touid, & togid) < 0)
	    return;				/* error was already printed */

/*
 * IDS MATCH FROMFILE'S; JUST SET ACL WITHOUT TRANSLATION:
 */

	if ((fromuid == touid) && (fromgid == togid))
	    SetACL (isstdin, filename, nentries, acl);

/*
 * OWNER AND/OR GROUP DIFFERS; TRANSLATE OWNERSHIP AND SET ACL:
 *
 * Create, modify, and use a temporary copy of acl[] so as not to change the
 * global value, which is used for more than one file.
 */

	else
	{
	    for (entry = 0; entry < nentries; entry++)
		aclnew [entry] = acl [entry];

	    chownacl (nentries, aclnew, fromuid, fromgid, touid, togid);
	    SetACL (isstdin, filename, nentries, aclnew);
	}

} /* Copy */


/************************************************************************
 * F O L D
 *
 * Given a flag whether to use STDIN or a named file, and a filename, fold the
 * optional entries in one file's ACL into effective access rights in the base
 * mode bits.  If anything goes wrong, print a warning message.
 *
 * The only reasonable way to do this is with stat() (st_mode already returns
 * the folded bits in st_mode) and chmod().  It depends on side-effect
 * behaviors of stat() and chmod(), but it also works over LAN.
 */

PROC Fold (isstdin, filename)
	int	isstdin;
	char	*filename;
{
static	struct	stat statbuf;	/* only allocate once */

	if ((GetStat (isstdin, filename, & statbuf) >= 0)

	 && ((isstdin ? fchmod (STDIN,	  statbuf.st_mode)
		      :  chmod (filename, statbuf.st_mode)) < 0))
	{
	    Error (WARNING, errno, 55, msg55, filename);
	}

} /* Fold */


/************************************************************************
 * G E T   A C L
 *
 * Given a flag whether to use STDIN or a named file, a filename, and global
 * acl[] to modify (which has at least NACLENTRIES elements), get the ACL on
 * STDIN or the named file and return the number of entries in it.  If anything
 * goes wrong, print a warning message and return the (negative) error value
 * from getacl().
 */

PROC int GetACL (isstdin, filename)
	int	isstdin;
	char	*filename;
{
	int	nentries;

	if ((nentries = isstdin ? fgetacl (STDIN,    NACLENTRIES, acl)
			        :  getacl (filename, NACLENTRIES, acl)) < 0)
	{
	    Error (WARNING, errno, 55, msg55, filename);
	}

	return (nentries);

} /* GetACL */


/************************************************************************
 * G E T   I D S
 *
 * Given a flag whether to use STDIN or a named file, a filename, and pointers
 * to user and group IDs, get and return the ID values on the file and return
 * zero.  If anything goes wrong, return -1; a subroutine gives the message.
 */

PROC int GetIDs (isstdin, filename, uidp, gidp)
	int	isstdin;
	char	*filename;
	uid_t	*uidp;
	gid_t	*gidp;
{
static	struct	stat statbuf;		/* only allocate once */

	if (GetStat (isstdin, filename, & statbuf) < 0)
	    return (-1);

	*uidp = statbuf . st_uid;
	*gidp = statbuf . st_gid;
	return (0);

} /* GetIDs */


/************************************************************************
 * G E T   S T A T
 *
 * Given a flag whether to use STDIN or a named file, a filename, and a pointer
 * to a struct stat, stat() STDIN or the named file and return zero.  If
 * anything goes wrong, print a warning message and return -1.
 */

PROC int GetStat (isstdin, filename, statbufp)
	int	isstdin;
	char	*filename;
	struct	stat *statbufp;
{
	if ((isstdin ? fstat (STDIN,    statbufp)
		     :  stat (filename, statbufp)) < 0)
	{
	    Error (WARNING, errno, 55, msg55, filename);

	    return (-1);
	}

	return (0);

} /* GetStat */


/************************************************************************
 * S T R   T O   A C L
 *
 * This procedure takes many parameters because it encapsulates the entire
 * operation of parsing an ACL, including getting file ID values.  Given:
 *
 * - an ACL (or ACL pattern) string to convert;
 *
 * - the number of entries already in global acl[] (if applicable);
 *
 * - a flag whether to parse an exact ACL or a pattern (to global acl[] or
 *   aclpatt[]);
 *
 * - file user and group ID values to substitute for "@" symbols;
 *
 * - a flag to get file ID values or use the given ones (which may be
 *   don't-care values if the caller so chooses); if TRUE, isstdin must be
 *   valid and if it in turn is FALSE, filename must be non-null;
 *
 * - a flag to use STDIN or the named file for getting file ID values;
 *
 * - a filename for use in error messages;
 *
 * - an error message NLS number and string;
 *
 * get file IDs if needed, then call strtoacl() or strtoaclpatt() and return
 * the number of resulting entries.  If anything goes wrong, print a warning
 * message and return a negative value.
 *
 * errmsg must contain one "%s" where the filename parameter goes, and a second
 * "%s" to be used to print the meaning of the error, and a third "%s" inside
 * quotes (\") where the bad-entry string goes.
 */

PROC int StrToACL (string, nentries, pattern, fuid, fgid, getids,
		   isstdin, filename, errmsgnum, errmsg)

	char	*string;
	int	nentries;
	int	pattern;
	uid_t	fuid;
	gid_t	fgid;
	int	getids;
	int	isstdin;
	char	*filename;
	int	errmsgnum;
	char	*errmsg;
{
/*
 * GET FILE IDS:
 */

	if (getids && (GetIDs (isstdin, filename, & fuid, & fgid) < 0))
	    return (-1);			/* error was already printed */

/*
 * PARSE ACL STRING:
 */

	if ((nentries = (pattern ?
		    strtoaclpatt (string, NACLENTRIES, aclpatt) :
		    strtoacl (string, nentries, NACLENTRIES, acl, fuid, fgid)))
	    < 0)
	{		/* report an error  with positive error number */
	    ReportSTAError (pattern, (-nentries), errmsgnum, errmsg, filename);
	}

	return (nentries);

} /* StrToACL */


/*********************************************************************
 * MESSAGES FOR ERRORS FROM strtoacl() AND strtoaclpatt():
 *
 * These are variable parts of the complete error messages.  NLMN_ERROR is the
 * base message number for ReportSTAError().  It must match the catgets
 * comments used by findmsg(1).
 */

#define	NLMN_ERROR 30

char	*error_sta[] = {

/* catgets 30 */  /*  0 */  "(no error)",
/* catgets 31 */  /*  1 */  "ACL entry doesn't start with \"(\" in short form",
/* catgets 32 */  /*  2 */  "ACL entry doesn't end with \")\" in short form",
/* catgets 33 */  /*  3 */  "user name not terminated with dot in ACL entry",
/* catgets 34 */  /*  4 */  "group name not terminated correctly in ACL entry",
/* catgets 35 */  /*  5 */  "user name is null in ACL entry",
/* catgets 36 */  /*  6 */  "group name is null in ACL entry",
/* catgets 37 */  /*  7 */  "invalid user name in ACL entry",
/* catgets 38 */  /*  8 */  "invalid group name in ACL entry",
/* catgets 39 */  /*  9 */  "invalid mode in ACL entry",
/* catgets 40 */  /* 10 */  "more than 16 ACL entries at entry",

/* catgets 41 */  /* 11 */  "unknown error from strtoacl()",
/* catgets 42 */  /* 12 */  "unknown error from strtoaclpatt()",

};

#define	MAXERR_STA 10		/* highest value, except last two */


/************************************************************************
 * R E P O R T   S T A   E R R O R
 *
 * Given a flag if checking an exact or pattern ACL, an error number (positive
 * value), an error message number and string (see StrToACL()), a filename, and
 * global aclentrystart[], translate the message meaning part and report an
 * error from strtoacl() or strtoaclpatt() using global error_sta[].  NLS
 * support makes this sufficiently complex to deserve a separate subroutine.
 * Be careful not to stomp on errmsg or errmeaning by causing catgets() to be
 * called twice.
 */

PROC ReportSTAError (pattern, errnum, errmsgnum, errmsg, filename)
	int	pattern;
	int	errnum;
	int	errmsgnum;
	char	*errmsg;
	char	*filename;
{
extern	char	*aclentrystart[];		/* for error report	*/
REG	char	chsave;				/* saved character	*/
	char	*errmeaning;			/* part of message	*/

#ifdef NLS
#define	ERRSIZE 100				/* hope that's big enough */
	char	errbuf [ERRSIZE];
#endif

/*
 * TRANSLATE MEANING OF ERROR:
 *
 * Special messages should never be used, but are provided just in case of
 * an unexpected return from strtoacl[patt]().
 */

	if (errnum > MAXERR_STA)			/* unknown value */
	    errnum = MAXERR_STA + (pattern ? 2 : 1);	/* use special	 */

#ifdef NLS
	if (*(errmeaning = catgetmsg (nlmsg_fd, NL_SETN, (NLMN_ERROR + errnum),
				      errbuf, ERRSIZE)) == CHNULL)
#endif
	    errmeaning = error_sta [errnum];	/* didn't/couldn't translate */

/*
 * PREPARE ACL ENTRY STRING, GIVE MESSAGE:
 */

	chsave = aclentrystart [1] [0];
	aclentrystart [1] [0] = CHNULL;		/* temporarily end string */

	Error (WARNING, NOERRNO, errmsgnum, errmsg,
			   filename, errmeaning, aclentrystart [0]);

	aclentrystart [1] [0] = chsave;		/* repair ACL string */

} /* ReportSTAError */


/************************************************************************
 * S E T   A C L
 *
 * Given a flag whether to use STDIN or a named file, a filename, the number of
 * entries and acl[] to use (may differ from global acl[]), set the ACL on one
 * file.  If anything goes wrong, print a warning message.
 */

PROC SetACL (isstdin, filename, nentries, acl)
	int	isstdin;
	char	*filename;
	int	nentries;
	struct	acl_entry acl[];
{
	if ((isstdin ? fsetacl (STDIN,    nentries, acl)
		     :  setacl (filename, nentries, acl)) < 0)
	{
	    Error (WARNING, errno, 55, msg55, filename);
	}

} /* SetACL */
