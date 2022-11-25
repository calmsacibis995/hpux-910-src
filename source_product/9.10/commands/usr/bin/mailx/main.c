/* @(#) $Revision: 70.1 $ */      
#

#include "rcv.h"
#include <sys/stat.h>
#include <stdio.h>

#ifndef NLS
#  define catgets(i,sn,mn,s) (s)
#else NLS
#  define NL_SETN 12	/* set number */
#  include <locale.h>
#  include <nl_types.h>
   nl_catd nl_fn;
   nl_catd nl_tfn;
   struct locale_data *ld;
#endif NLS

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Startup -- interface with user.
 */


jmp_buf	hdrjmp;

/*
 * Find out who the user is, copy his mail file (if exists) into
 * /tmp/Rxxxxx and set up the message pointers.  Then, print out the
 * message headers and read user commands.
 *
 * Command line syntax:
 *	mailx [ -i ] [ -r address ] [ -h number ] [ -f [ name ] ]
 * or:
 *	mailx [ -i ] [ -r address ] [ -h number ] people ...
 *
 * and a bunch of other options.
 */

extern int optind;
extern char *optarg;

main(argc, argv)
	char **argv;
{
	register char *ef;
	register int i, argp;
	int mustsend, uflag, hdrstop(),  f; 
	void (*prevint)(); 
	int loaded = 0;
	FILE *ibuf, *ftat;
	struct termio tbuf;
	char *shellp, *replytop;

	int fflag = 0;				/* 1 = -f option specified */
	char optstring [ 24 ];		/* option string for getopt */
	int x;
	int fopt = 0;				/* 1 = f option is followed by a filename */

#if defined NLS || defined NLS16	/* initialize to correct language */
	char tmplang[SL_NAME_SIZE];
	if (!setlocale(LC_ALL,"")){
		fputs(_errlocale(),stderr);
		nl_fn = (nl_catd)-1;
	}
	else  {
		nl_fn = catopen("mailx",0);
		ld=getlocale(LOCALE_STATUS);
		if (strcmp(ld->LC_ALL_D,ld->LC_TIME_D)) {
			sprintf(tmplang,"LANG=%s",ld->LC_TIME_D);
			putenv(tmplang);
			nl_tfn=catopen("ar",0);
			sprintf(tmplang,"LANG=%s",ld->LC_ALL_D);
			putenv(tmplang);
		}
		else
			nl_tfn=nl_fn;
	}
#endif NLS || NLS16

#ifdef signal
	Siginit();
#endif

	/*
	 * Set up a reasonable environment.  We clobber the last
	 * element of argument list for compatibility with version 6,
	 * figure out whether we are being run interactively, set up
	 * all the temporary files, buffer standard output, and so forth.
	 */

	uflag = 0;
	argv[argc] = (char *) -1;
	inithost();
	mypid = getpid();
	intty = isatty(0);
	if (ioctl(1, TCGETA, &tbuf)==0) {
		outtty = 1;
		baud = tbuf.c_cflag & CBAUD;
	}
	else
		baud = B9600;
	image = -1;

	/*
	 * Now, determine how we are being used.
	 * We successively pick off instances of -r, -h, -f, and -i.
	 * If called as "rmail" we note this fact for letter sending.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */

	ef = NOSTR;
	argp = -1;
	mustsend = 0;
	if (argc > 0 && **argv == 'r') {
		rmail++;
		mustsend++;
	}

	/* if the old syntax ( -f filename ) is used put make the filename
	   a required parameter, else make it optional.  If in a future version
	   only the posix functionality is used this loop can be dropped and
	   optstring can specify f without a colon.
    */
	for ( x = 1; x < argc - 1 ; x++ )
		if ( ( strcmp ( argv [ x ], "-f" ) == 0 ) && 
           ( *argv [ x + 1 ] != '-' ) ) {     
			/* require f have an argument */
			strcpy ( optstring, "UT:r:h:FdeHinNs:u:f:" );
			fopt = 1;
			break;
			}
	if ( ! fopt ) /* f will not have a parameter, the filename 
                     may possibly be at the end of cmd */
		strcpy ( optstring, "UT:r:h:FdeHinNs:u:f" );
			


	/* have each option handle missing parameters */
	while ( (i = getopt ( argc, argv, optstring ) ) != EOF ) {
		/*
		 * If current argument is not a flag, then the
		 * rest of the arguments must be recipients.
		 */

		switch (i) {
		case 'e':
			/*
			 * exit status only
			 */
			exitflg++;
			break;

		case 'r':
			/*
			 * Next argument is address to be sent along
			 * to the mailer.
			 */
			mustsend++;
			rflag = optarg;
			break;

		case 'T':
			/*
			 * Next argument is temp file to write which
			 * articles have been read/deleted for netnews.
			 */
			Tflag = optarg;
			if ((f = creat(Tflag, 0600)) < 0) {
				perror(Tflag);
				exit(1);
			}
			close(f);
			/* fall through for -I too */

		case 'I':
			/*
			 * print newsgroup in header summary
			 */
			newsflg++;
			break;

		case 'u':
			/*
			 * Next argument is person to pretend to be.
			 */
			uflag++;
			strcpy( myname, optarg );
			break;

		case 'i':
			/*
			 * User wants to ignore interrupts.
			 * Set the variable "ignore"
			 */
			assign("ignore", "");
			break;
		
		case 'U':
			UnUUCP++;
			break;

		case 'd':
			assign("debug", "");
			break;

		case 'h':
			/*
			 * Specified sequence number for network.
			 * This is the number of "hops" made so
			 * far (count of times message has been
			 * forwarded) to help avoid infinite mail loops.
			 */
			mustsend++;
			hflag = atoi( optarg );
			if (hflag == 0) {
				fprintf(stderr, (catgets(nl_fn,NL_SETN,1, "-h needs non-zero number\n")));
				exit(1);
			}
			break;

		case 's':
			/*
			 * Give a subject field for sending from
			 * non terminal
			 */
			sflag = optarg;
			mustsend++;
			break;

		case 'f':
			/*
			 * User is specifying file to "edit" with mailx,
			 * as opposed to reading system mailbox.
			 * If no argument is given after -f, we read his
			 * mbox file in his home directory.  This is handled below.
			 */
			fflag++;	
			/* The filename may not be supplied or it may be at the end of 
			 * the command line.
			*/
			if ( fopt )		/* if filename is supplied */
				ef = optarg;	
			break;

		case 'F':
			Fflag++;
			mustsend++;
			break;

		case 'n':
			/*
			 * User doesn't want to source
			 *	/usr/lib/mailx/mailx.rc
			 */
			nosrc++;
			break;

		case 'N':
			/*
			 * Avoid initial header printing.
			 */
			noheader++;
			break;

		case 'H':
			/*
			 * Print headers and exit
			 */
			Hflag++;
			break;

		default:
			fprintf(stderr, (catgets(nl_fn,NL_SETN,2, "usage: mailx [-FU] [-s subject] [-r address] [-h number] address ...\n" )) );
			fprintf(stderr, (catgets(nl_fn,NL_SETN,3, "       mailx -e\n" )) );
			fprintf(stderr, (catgets(nl_fn,NL_SETN,4, "       mailx [-UHiNn] [-u user]\n" )) );
			fprintf(stderr, (catgets(nl_fn,NL_SETN,5, "       mailx -f [-UHiNn] [filename]\n" )) );
			exit(1);
		}
	}

	if ( fflag && ! fopt ) {		/* handle -f option */
		if ( optind < argc )		/* if filename at end of cmd line */
			ef = argv[ optind ];
		else {						/* use default file */
			ef = (char *) calloc(1, strlen(Getf("MBOX"))+1);
			strcpy(ef, Getf("MBOX"));
			}
		}

	if ( optind < argc ) 		/* remaining args must be recipents */
		argp = optind;

	/*
	 * Check for inconsistent arguments.
	 */

	if (newsflg && ef==NOSTR) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,6, "Need -f with -I flag\n")));
		exit(1);
	}
	if (ef != NOSTR && argp != -1) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,7, "Cannot give -f and people to send to.\n")));
		exit(1);
	}
	if (exitflg && (mustsend || argp != -1))
		exit(1);	/* nonsense flags involving -e simply exit */
	if (mustsend && argp == -1) {
		fprintf(stderr, (catgets(nl_fn,NL_SETN,8, "The flags you gave are used only when sending mail.\n")));
		exit(1);
	}
	/* get environment variables of interest */
	if ( (shellp=getenv("SHELL")) != NULL )
	    assign("SHELL", shellp);
	if ( (replytop=getenv("REPLYTO")) != NULL )
	    assign("replyto", replytop);
	tinit();

	/*
	 * Insure that the handler for SIGCLD is set to SIG_DFL
	 * so that zombie processes WILL be generated. This
	 * prevents code from getting stuck in loops like:
	 *
	 *      while ( wait(&status) != pid )
	 *          ;
	 *
	 * If the handler is set to SIG_IGN this loop will never
	 * terminate because no zombie process exists to wait on.
	 */

	(void ) sigset(SIGCLD, SIG_DFL);


	input = stdin;
	rcvmode = argp == -1;
	if (!nosrc)
		load(MASTER);

	if (argp != -1) {
		load(Getf("MAILRC"));
		mail(&argv[argp]);

		/*
		 * why wait?
		 */

		exit(senderr);
	}

	/*
	 * Ok, we are reading mail.
	 * Decide whether we are editing a mailbox or reading
	 * the system mailbox, and open up the right stuff.
	 *
	 * Do this before sourcing the MAILRC, because there might be
	 * a 'chdir' there that breaks the -f option. But if the
	 * file specified with -f is a folder name, go ahead and
	 * source the MAILRC anyway so that "folder" will be defined.
	 */

	if (ef != NOSTR) {
		char *ename;

		edit++;
		if ( *ef == '+' ) {
			load(Getf("MAILRC"));
			loaded++;
		}
		ename = expand(ef);
		if (ename != ef) {
			ef = (char *) calloc(1, strlen(ename) + 1);
			strcpy(ef, ename);
		}
		editfile = ef;
		strcpy(mailname, ef);
	}

	if (setfile(mailname, edit) < 0)
		exit(1);

	if ( !loaded )
		load(Getf("MAILRC"));
	if (msgCount > 0 && !noheader && value("header") != NOSTR) {
		if (setjmp(hdrjmp) == 0) {
			if ((prevint = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
				sigset(SIGINT, hdrstop);
			announce();
			fflush(stdout);
			sigset(SIGINT, prevint);
		}
	}
	if (Hflag || (!edit && msgCount == 0)) {
		if (!Hflag)
			fprintf(stderr, (catgets(nl_fn,NL_SETN,9, "No mail for %s\n")), myname);
		fflush(stdout);
		exit(0);
	}
	commands();
	if (!edit) {
		sigset(SIGHUP, SIG_IGN);
		sigset(SIGINT, SIG_IGN);
		sigset(SIGQUIT, SIG_IGN);
		quit();
	}
	exit(0);
}

/*
 * Interrupt printing of the headers.
 */
hdrstop()
{

	clrbuf(stdout);
	printf((catgets(nl_fn,NL_SETN,10, "\nInterrupt\n")));
	fflush(stdout);
	sigrelse(SIGINT);
	longjmp(hdrjmp, 1);
}
