static char *HPUX_ID = "@(#) $Revision: 64.2 $";
/*
 * List access control list (ACL) on files(s).
 * See the manual entry (lsacl(1)) for details.
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
 * This program has no other needs for NLS-smartness because ACL strings
 * contain only 7-bit data, including user and group names.
 */

#include <stdio.h>
#include <acllib.h>

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

nl_catd	nlmsg_fd;		/* file descriptor */
#define NL_SETN 1		/* set number	   */

#endif


/*********************************************************************
 * MISCELLANEOUS GLOBAL VALUES:
 */

#define MYNAME	"lsacl"			/* bad practice but NLS requires it */

#define	PROC				/* null; easy to find procs */
#define	FALSE	0
#define	TRUE	1
#define	CHNULL	('\0')
#define	CPNULL	((char *) NULL)
#define	REG	register

#define	STDIN	(fileno (stdin))

char	*myname;			/* how program was invoked	*/
int	lflag = FALSE;			/* -l (long form) option	*/

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

/* catgets 1 */  "usage: %s [-l] file ...",
/* catgets 2 */  "-l long form output",

    CPNULL,
};


/************************************************************************
 * M A I N
 */

PROC main (argc, argv)
REG	int	argc;
REG	char	**argv;
{
extern	int	optind;				/* from getopt()	*/
	int	option;				/* option "letter"	*/
extern	int	errno;				/* system error number	*/
	int	errcount = 0;			/* error count		*/

	int	nentries;			/* number in acl[]	*/
	struct	acl_entry acl [NACLENTRIES];	/* value to print	*/
	char	*aclstring;			/* string form		*/
	int	notfirst = FALSE;		/* flag: not first file	*/

#ifdef NLS
	nlmsg_fd = catopen (MYNAME, 0);	/* ignore failure */
#endif

/*
 * CHECK ARGUMENTS:
 */

	myname = *argv;

	while ((option = getopt (argc, argv, "l")) != EOF)
	{
	    switch (option)
	    {
	    case 'l':	lflag = TRUE; break;
	    default:	Usage();
	    }
	}

	argc -= optind;			/* skip options	*/
	argv += optind;

	if (argc < 1)			/* must be at least one filename */
	    Usage();

/*
 * GET FILE'S ACL:
 */

	while (argc-- > 0)
	{
	    if (((*argv) [0] != '-') || ((*argv) [1] != CHNULL))
		nentries = getacl (*argv, NACLENTRIES, acl);	/* not stdin */
	    else
	    {
		nentries = fgetacl (STDIN, NACLENTRIES, acl);
		*argv = lflag ? "<stdin>" : "-";
	    }

	    if (nentries < 0)
	    {
		int saveno = errno;

		fprintf (stderr,
			 catgets(nlmsg_fd, NL_SETN, 10, "%s: file \"%s\""),
			 myname, *argv);

		fprintf (stderr, ": %s", strerror (saveno));
		fprintf (stderr,
			 catgets(nlmsg_fd, NL_SETN, 11, " (errno = %d)\n"),
			 saveno);

		errcount++;
	    }

/*
 * CONVERT ACL TO STRING AND PRINT IT:
 *
 * Assume acltostr() always works.
 */

	    else
	    {
		aclstring = acltostr (nentries, acl,
				      lflag ? FORM_LONG : FORM_SHORT);

		if (! lflag)			/* short form */
		{
		    fputs(aclstring, stdout);
		    fputc(' ', stdout);
		    fputs(*argv, stdout);
		    fputc('\n', stdout);
		}
		else				/* long form */
		{
		    if (notfirst)
			putchar ('\n');		/* leading blank line */
		    else
			notfirst = TRUE;

		    fputs(*argv, stdout);
		    fputs(":\n", stdout);
		    fputs(aclstring, stdout);
		}
	    } /* else */

	    argv++;

	} /* while */

/*
 * FINISH UP:
 */

#ifdef NLS
	catclose (nlmsg_fd);			/* just to be nice */
#endif

	exit (errcount ? WARNEXIT : OKEXIT);

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
