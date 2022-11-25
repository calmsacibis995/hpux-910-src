#ifndef lint
    static char *HPUX_ID = "@(#) $Revision: 72.9 $";
#endif

/*
 *	Mail to a remote machine will normally use the uux command.
 *	If available, it may be better to send mail via nusend
 *	or usend, although delivery is not as reliable as uux.
 *	Mail may be compiled to take advantage
 *	of these other networks by adding:
 *		#define USE_NUSEND  for nusend
 *	and
 *		#define USE_USEND   for usend.
 *
 *	NOTE:  If either or both defines are specified, that network
 *	will be tried before uux.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<strings.h>
#include	<sys/types.h>
#include	<signal.h>
#include	<pwd.h>
#include	<time.h>
#include	<utmp.h>
#include	<sys/stat.h>
#include	<setjmp.h>
#include	<sys/utsname.h>
#include        <fcntl.h>
#include        <unistd.h>
#include 	<sys/param.h>
#include 	<sys/wait.h>
#include	<syslog.h>

#ifdef NLS
#   include <locale.h>
#   include <nl_ctype.h>
#   include <nl_types.h>
#   define NL_SETN	1
#endif

#if defined(DUX) || defined(DISKLESS)
#   include <cluster.h>
#endif

#ifdef V4FS
#	define DELIVERMAIL	"/usr/sbin/sendmail"
#else
#	define DELIVERMAIL	"/usr/lib/sendmail"
#endif

#define LOCKON	 0x2	/* lockf(2) on /usr/mail/<username> done */
#define LOCKMADE 0x1	/* /usr/mail/<username>.lock made */
#define LOCKOFF	 0x0	/* lock(s) released */

#define C_NOACCESS	0	/* no access to file */
#define CHILD	0
#define SAME	0

#define CERROR		-1
#define CSUCCESS	0

#define TRUE	1
#define FALSE	0

#define E_FLGE	1	/* exit flge error */
#define E_FILE	2	/* exit file error */
#define E_SPACE	3	/* exit no space */
#define E_FRWD	3	/* exit cannot forward */
/*
 * copylet flags
 */
#define	REMOTE		1		/* remote mail, add rmtmsg */
#define ORDINARY	2
#define ZAP		3		/* zap header and trailing empty line */
#define FORWARD		4

#define LFNAME		1024		/* maximum size of a filename */
#define LSIZE           2*BUFSIZ        /* maximum size of a line */
#define	MAXLET		300		/* maximum number of letters */
#define FROMLEVELS	20		/* maximum number of forwards */
#define MAXFILENAME	128
#define MAX_FRWD_ADDR_LEN	256	/* maximum length of forwarding addr */
#define MAX_LOCK_TRIES	10		/* maximum times to lockf() or .lock */
#define PLENGTH		4096		/* the maximum length of an X400 or
					   Openmail address is 3054 bytes.  this
					   value allows for such an address plus
					   some extra for machine name */

#ifndef	MFMODE
#   define MFMODE	0660		/* create mode for `/usr/mail' files */
#endif

#define MAILGRP		6

#define A_OK		0		/* return value for access */
#define A_EXIST		0		/* access check for existence */
#define A_EXEC          1               /* access check for execute permission */
#define A_WRITE		2		/* access check for write permission */
#define A_READ		4		/* access check for read permission */

#define MAXHDR          100             /* maximum number of added header lines */

/*
 * This is the contents of sysexits.h. If sysexits.h is supported,  
 * delete the following and include the next line instead.
 * 	#include <sysexits.h>
 */

# define EX_OK		0	/* successful termination */

# define EX__BASE	64	/* base value for error messages */

# define EX_USAGE	64	/* command line usage error */
# define EX_DATAERR	65	/* data format error */
# define EX_NOINPUT	66	/* cannot open input */
# define EX_NOUSER	67	/* addressee unknown */
# define EX_NOHOST	68	/* host name unknown */
# define EX_UNAVAILABLE	69	/* service unavailable */
# define EX_SOFTWARE	70	/* internal software error */
# define EX_OSERR	71	/* system error (e.g., can't fork) */
# define EX_OSFILE	72	/* critical OS file missing */
# define EX_CANTCREAT	73	/* can't create (user) output file */
# define EX_IOERR	74	/* input/output error */
# define EX_TEMPFAIL	75	/* temp failure; user is invited to retry */
# define EX_PROTOCOL	76	/* remote error in protocol */
# define EX_NOPERM	77	/* permission denied */
# define EX_CONFIG	78	/* fatal configuration file error */

/* 
 * 	End of sysexits.h
 */

struct	let	{
	long	adr;
	char	change;
} let[MAXLET];

struct  passwd  *getpwuid(), *getpwent();

FILE	*tmpf=NULL, *malf=NULL;
char	*progname;

char	lettmp[LFNAME];
char	from[] = "From ";
char	TO[] = "To: ";

#ifdef V4FS
	char	maildir[] = "/var/mail/";
#else
	char	maildir[] = "/usr/mail/";
#endif

static char	sendto[MAX_FRWD_ADDR_LEN];

/*
 * Because of a security hole, we are disallowing the use of backquotes
 * in forwarding addresses.  If some customer *absolutely* has to have this
 * functionality, we can tell them to change the following variable with
 * adb(1) -w and give them the appropriate warning about the security
 * implications.
 */
int	allow_backquotes = 0;

/*
 * fowrding address
 */
char	*mailfile;
char	maillock[] = ".lock";
char	dead[] = "/dead.letter";
char	rmtbuf[LFNAME];
#define RMTMSG	" remote from %s\n"
#define FORWMSG	" forwarded by %s\n"
char	frwrd[] = "Forward to ";
char	nospace[] = "mail: no space for temp file\n";
char	*thissys;
char	mbox[] = "/mbox";
char	curlock[LFNAME];
char	line[PLENGTH];
char	resp[LSIZE];
char	*hmbox;
char	*hmdead;
char	*home;
char	*my_name;
char	*tmpdir;
char	*getlogin();
char	lfil[LFNAME];
char    *getsysname();

int	error;
int	fromlevel = 0;
int	nlet	= 0;
void	lock(), lock2();
int	locked = LOCKOFF;
int	changed;
int	forward;
void	delete();
int	flgf = 0;
int	flgp = 0;
int	flge = 0;
int	delflg = 1;
int	toflg = 0;	/* set if -t option is given */
int	rmail = 0;	/* set if invoked as "rmail" */
int	dflag = 0;	/* set if -d option is given */
int	sendmailflag = 0;	/* set if this system has "/usr/lib/sendmail" */
void	(*saveint)();
void	(*setsig())();
void	savdead(), done(), printmail(), copyback(), copymt(), sendmail(),
	cat();
size_t	strln();
uid_t	suid;		/* effective uid on entry to mail.  this will become
			   the saved uid */
uid_t	ruid;		/* real uid */

long	iop;

jmp_buf	sjbuf;

char **hdr_lines;           /* header lines generated by collapse() */

#ifdef USE_NUSEND
    jmp_buf nusendfail;
    int	nusendjmp = FALSE;
#endif

#ifdef USE_USEND
    jmp_buf usendfail;
    int	usendjmp = FALSE;
#endif

mode_t umsave;

#ifdef NLS
    nl_catd catd;
#endif

/*
 *	mail [ + ] [ -irpqe ]  [ -f file ]
 *	mail [ -dt ] persons 
 *      rmail [ -dt ] persons
 */
main(argc, argv)
char	**argv;
{
	register i;
	char *tmpstr, *cp;

	/* mail is a setuid program owned by user daemon.  this is done
	   so that the system mail file will not be owned by the sending
	   user when it is first created.  we only want to run as daemon
	   when we create the mail file.  at all other times, we want the
	   effective user id to be the real user id. */

	progname = argv[0];

	suid = geteuid();
	ruid = getuid();
	if (setresuid(-1, ruid, suid) == CERROR) {
		fprintf(stderr, "mail: can't change user id\n");
		error = EX_SOFTWARE;
		exit(error);
	}
	umsave = umask(7);
	setbuf(stdout, (char *) malloc(BUFSIZ));

	thissys = getsysname();

	/* 
	 * Use environment variables
	 * logname is used for from
	 */
	if(((home = getenv("HOME")) == NULL) || (strlen(home) == 0))
		home = ".";
	if (((my_name = getenv("LOGNAME")) == NULL) || (strlen(my_name) == 0))
		my_name = getlogin();
	if ((my_name == NULL) || (strlen(my_name) == 0))
		my_name = getpwuid(geteuid())->pw_name;
	if (((tmpdir = getenv("TMPDIR")) == NULL) || (strlen(tmpdir) == 0))
		tmpdir = "/tmp";
	(void) sprintf( lettmp, "%s/maXXXXXX", tmpdir );
	(void) sprintf( rmtbuf, "%s/marXXXXXX", tmpdir );

	/*
	 * Catch signals for cleanup
	 */
	if(setjmp(sjbuf)) 
		done();
	for (i=SIGINT; i<SIGCLD; i++)
		setsig(i, delete);
	setsig(SIGHUP, done);
	/*
	 * Make temporary file for letter
	 */
	(void) mktemp(lettmp);
	unlink(lettmp);
	catd = catopen("mail", 0);
	if((tmpf = fopen(lettmp, "w")) == NULL){
		fprintf(stderr, catgets(catd,NL_SETN,1,"mail: can't open %s for writing\n"), lettmp);
		if(dflag)
			error = EX_CANTCREAT;
		else
			error = E_FILE;
		done();
	}
	/*
	 * Rmail always invoked to send mail
	 */
	/* strip preceeding path from argv[0] */
	tmpstr = &(argv[0][0]);
	if ((cp = strrchr(tmpstr,'/')) != (char *) NULL) {
		cp++;
		argv[0]= cp;	
	}
	
	/* do we have "/usr/lib/sendmail" ? */
	sendmailflag = isthere(DELIVERMAIL);

	if (*progname != 'r' &&	
		(argc == 1 || argv[1][0] == '-' || argv[1][0] == '+'))
		printmail(argc, argv);
	else {
		rmail = ( *progname == 'r'); /* invoked as "rmail" ? */
		sendmail(argc, argv);
	}

	done();
#ifdef lint
	return(0);
#endif
}

/*
 * isthere - return TRUE if:
 *              - file is there
 *              - any of its execute bits are set
 *              - it's size is non-zero,
 *           otherwise return FALSE.
 */

isthere(filename)
    char *filename;
{
    struct stat stbuf;

    if ( stat(filename, &stbuf) != CERROR &&	/* file exists */
	 ( stbuf.st_mode & 0111 )         &&	/* file is executable */
	 stbuf.st_size > 0 )			/* non-zero file size */
	return TRUE;
    else
	return FALSE;
}

/*
 * Signal reset
 * signals that are not being ignored will be 
 * caught by function f
 *	i	-> signal number
 *	f	-> signal routine
 * return
 *	rc	-> former signal
 */
void (*setsig(i, f))()
int	i;
void	(*f)();
{
	register void (*rc)();

	if((rc = signal(i, SIG_IGN))!=SIG_IGN)
		signal(i, f);
	return(rc);
}

off_t	save_st_size;
/*
 * Print mail entries
 *	argc	-> argument count
 *	argv	-> arguments
 */
void
printmail(argc, argv)
char	**argv;
{
	register int c;
	int	flg, i, j, print, aret, stret, goerr = 0;
	char	*p, *getarg();
	char	frwrdbuf[MAX_FRWD_ADDR_LEN];
	struct	stat stbuf;
	extern	char *optarg;
	char	fbuf[MAX_FRWD_ADDR_LEN];

	/*
	 * Print in reverse order
	 */
	if ((argc > 1) && (argv[1][0] == '+')) {
		forward = 1;
		argc--;
		argv++;
	}
	while((c = getopt(argc, argv, "f:rpqietd")) != EOF)
	    switch(c) {

		/*
		 * use file input for mail
		 */
		case 'f':
			flgf = 1;
			mailfile = optarg;
			break;
		/* 
		 * print without prompting
		 */
		case 'p':
			flgp++;
		/* 
		 * terminate on deletes
		 */
		case 'q':
			delflg = 0;
			break;
		case 'i':
			break;

		/* 
		 * print by first in, first out order
		 */
		case 'r':
			forward = 1;
			break;
		/*
		 * do  not print mail
 		 */
		case 'e':
			flge = 1;
			break;

		case 't':
			toflg = 1;
			break;

		/*
		 * direct local delivery
		 */

		case 'd':
			dflag = 1;
			break;

		/*
		 * bad option
		 */
		case '*':	/*  '*' is valid help command also */
		case '?':
			goerr++;
	    }

	if ( toflg || dflag )
	    if ( argc > 2 ) {
		argv++;
		sendmail(--argc, argv);
		done();
	    } else
		goerr++;

	if(goerr) {
		fprintf(stderr, catgets(catd,NL_SETN,2,"usage: mail [+] [-epqr] [-f file]\n"));
		fprintf(stderr, catgets(catd,NL_SETN,3,"       mail [-dt] persons\n"));
		if (dflag)
			error = EX_USAGE;
		else
			error = E_FILE;
		done();
	}

	/*
	 * create working directory mbox name
	 */
	if((hmbox = (char *) malloc(strlen(home) + strlen(mbox) + 1)) == NULL){
		fprintf(stderr, catgets(catd,NL_SETN,4,"mail: can't allocate hmbox"));
		error = E_FILE;
		return;
	}
	cat(hmbox, home, mbox);
	if(!flgf) {
		if(((mailfile = getenv("MAIL")) == NULL) || (strlen(mailfile) == 0)) {
			if((mailfile = (char *) malloc(strlen(maildir) + strlen(my_name) + 1)) == NULL){
				fprintf(stderr, catgets(catd,NL_SETN,5,"mail: can't allocate mailfile"));
				error = E_FILE;
				return;
			}
			cat(mailfile, maildir, my_name);
		}
	}
	/*
	 * Check accessibility of mail file
	 */
	lock(mailfile);

	stret = stat(mailfile, &stbuf);
	save_st_size = stbuf.st_size;

	if((aret=access(mailfile, A_READ)) == A_OK)
		malf = fopen(mailfile, "r+");
	if (stret == CSUCCESS && aret == CERROR) {
		fprintf(stderr, catgets(catd,NL_SETN,6,"mail: permission denied!\n"));
		error = E_FILE;
		(void) unlock();
		return;
	}else 
	if (flgf && (aret == CERROR || (malf == NULL))) {
		fprintf(stderr, catgets(catd,NL_SETN,7,"mail: cannot open %s\n"), mailfile);
		error = E_FILE;
		(void) unlock();
		return;
	}else 
	if(aret == CERROR || (malf == NULL) || (stbuf.st_size == 0)) {
		if(!flge) printf(catgets(catd,NL_SETN,8,"No mail.\n"));
		error = E_FLGE;
		(void) unlock();
		return;
	}

	lock2(fileno(malf));

	/*
	 * See if mail is to be forwarded to another system.  We can't use the
	 * areforwarding routine here because it opens and closes the file
	 * which releases the lockf() lock we just set with lock2().
	 */
	fbuf[0] = '\0';
	if ((fread((void *)fbuf, sizeof(frwrd) - 1, 1, malf) == 1) &&
	    (strncmp(fbuf, frwrd, sizeof(frwrd) - 1) == SAME)) {
		if(flge) {
			(void) unlock_all(fileno(malf));
			error = E_FLGE;
			return;
		}
		if (!((stbuf.st_gid == MAILGRP) && ((stbuf.st_mode & 0777)== MFMODE))) {
			printf(catgets(catd,NL_SETN,9,"Check mode and group id of mail file.\n"));
			printf(catgets(catd,NL_SETN,10,"Mode should be \"660\" with group id \"mail\".\n"));
			(void) unlock_all(fileno(malf));
			error = E_FRWD;
			return;
		}
		printf(catgets(catd,NL_SETN,10,"Your mail is being forwarded to "));
		fseek(malf, (long)(sizeof(frwrd) - 1), 0);
		fgets(frwrdbuf, sizeof(frwrdbuf), malf);
		printf(catgets(catd,NL_SETN,11,"%s"), frwrdbuf);
		if(getc(malf) != EOF)
			printf(catgets(catd,NL_SETN,12,"and your mailbox contains extra stuff\n"));
		(void) unlock_all(fileno(malf));
		return;
	}
	if(flge) {
		(void) unlock_all(fileno(malf));
		return;
	}
	/*
	 * copy mail to temp file and mark each
	 * letter in the let array
	 */
	rewind(malf);
	copymt(malf, tmpf);
	(void) unlock2(fileno(malf));
	fclose(malf);
	fclose(tmpf);
	(void) unlock();
	if((tmpf = fopen(lettmp, "r")) == NULL) {
		fprintf(stderr,catgets(catd,NL_SETN,13,"mail: can't open %s\n"), lettmp);
		error = E_FILE;
		return;
	}
	changed = 0;
	print = 1;
	for (i = 0; i < nlet; ) {

		/*
		 * reverse order ?
		 */
		j = forward ? i : nlet - i - 1;
		if( setjmp(sjbuf) == 0 && print != 0 )
				copylet(j, stdout, ORDINARY);
		
		/*
		 * print only
		 */
		if(flgp) {
			i++;
			continue;
		}
		/*
		 * Interactive
		 */
		setjmp(sjbuf);
		printf("? ");
		(void) fflush(stdout);
		if (fgets(resp, sizeof(resp), stdin) == NULL)
			break;
		print = 1;
		switch (resp[0]) {

		default:
			printf(catgets(catd,NL_SETN,14,"usage\n"));

		/*
		 * help
		 */
		case '?':
	print = 0;
    	printf(catgets(catd,NL_SETN,15,"q\t\tquit\n"));
    	printf(catgets(catd,NL_SETN,16,"x\t\texit without changing mail\n"));
    	printf(catgets(catd,NL_SETN,17,"p\t\tprint\n"));
    	printf(catgets(catd,NL_SETN,18,"s [file]\tsave (default mbox)\n"));
    	printf(catgets(catd,NL_SETN,19,"w [file]\tsame without header\n"));
    	printf(catgets(catd,NL_SETN,20,"-\t\tprint previous\n"));
    	printf(catgets(catd,NL_SETN,21,"d\t\tdelete\n"));
    	printf(catgets(catd,NL_SETN,22,"+\t\tnext (no delete)\n"));
    	printf(catgets(catd,NL_SETN,23,"m user\t\tmail to user\n"));
    	printf(catgets(catd,NL_SETN,24,"! cmd\t\texecute cmd\n"));
	print = 0;
		break;

		/*
		 * skip entry
		 */
		case '+':

		case 'n':
		case '\n':
			i++;
			break;
		case 'p':
			break;
		case 'x':
			changed = 0;
			/*FALLTHROUGH*/
		case 'q':
			goto donep;
		/*
		 * Previous entry
		 */
		case '^':
		case '-':
			if (--i < 0)
				i = 0;
			break;
		/*
		 * Save in file without header
		 */
		case 'y':
		case 'w':
		/*
		 * Save mail with header
		 */
		case 's':
			if (resp[1] == '\n' || resp[1] == '\0')
				cat(resp+1, hmbox, "");
			else if(resp[1] != ' ') {
				printf(catgets(catd,NL_SETN,25,"invalid command\n"));
				print = 0;
				continue;
			}
			(void) umask(umsave);
			flg = 0;
			p = resp+1;
			if (getarg(lfil, p) == NULL)
				cat(resp+1, hmbox, "");
			for (p = resp+1; (p = getarg(lfil, p)) != NULL; ) {
				if((aret=legal(lfil)))
					malf = fopen(lfil, "a");
				if ((malf == NULL) || (aret == 0)) {
					fprintf(stderr, catgets(catd,NL_SETN,26,"mail: cannot append to %s\n"), lfil);
					flg++;
					continue;
				}
				if(aret==2)
					(void) chown(lfil, geteuid(), getgid());
				if (copylet(j, malf, resp[0]=='w'? ZAP: ORDINARY) == FALSE) {
					fprintf(stderr, catgets(catd,NL_SETN,27,"mail: cannot save mail\n"));
					flg++;
				}
				fclose(malf);
			}
			(void) umask(7);
			if (flg)
				print = 0;
			else {
				let[j].change = 'd';
				changed++;
				i++;
			}
			break;
		/*
		 * Mail letter to someone else
		 */
		case 'm':
			if (resp[1] == '\n' || resp[1] == '\0') {
				i++;
				continue;
			}
			if (resp[1] != ' ') {
				printf(catgets(catd,NL_SETN,28,"invalid command\n"));
				print = 0;
				continue;
			}
			flg = 0;
			p = resp+1;
			if (getarg(lfil, p) == NULL) {
				i++;
				continue;
			}
			for (p = resp+1; (p = getarg(lfil, p)) != NULL; ) {
				if (lfil[0] == '$' && !(getenv(&lfil[1]))) {
					fprintf(stderr,catgets(catd,NL_SETN,29,"%s has no value or is \
not exported.\n"),lfil);
					flg++;
				}
				else if (sendrmt(j, lfil) == FALSE)
 					flg++;
			}
			if (flg)
				print = 0;
			else {
				let[j].change = 'd';
				changed++;
				i++;
			}
			break;
		/*
		 * Escape to shell
		 */
		case '!':
			system(resp+1);
			printf(catgets(catd,NL_SETN,30,"!\n"));
			print = 0;
			break;
		/*
		 * Delete an entry
		 */
		case 'd':
			let[j].change = 'd';
			changed++;
			i++;
			if (resp[1] == 'q')
				goto donep;
			break;
		}
	}
	/*
	 * Copy updated mail file back
	 */
   donep:
	if (changed)
		copyback();
}

/*
 * copy temp or whatever back to /usr/mail
 */
void
copyback()
{
	register i, n, c;
	int new = 0;
	struct stat stbuf;

	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	lock(mailfile);
	if ((malf = fopen(mailfile, "r+")) == NULL) {
		fprintf(stderr, catgets(catd,NL_SETN,31,"mail: can't update %s\n"), mailfile);
		error = E_FILE;
		done();
	}
	lock2(fileno(malf));
	stat(mailfile, &stbuf);
	/*
	 * Has new mail arrived?
	 */
	if (stbuf.st_size != save_st_size) {
		fseek(malf, let[nlet].adr, 0);
		fclose(tmpf);
		if((tmpf = fopen(lettmp, "a")) == NULL) {
			fprintf(stderr,catgets(catd,NL_SETN,31,"mail: can't re-write %s\n"), mailfile);
			error = E_FILE;
			done();
		}

		/*
		 * append new mail
		 * assume only one
		 * new letter
		 */
		while ((c = fgetc(malf)) != EOF)
			if (fputc(c, tmpf) == EOF) {
				fclose(malf);
				fclose(tmpf);
				fprintf(stderr, catgets(catd,NL_SETN,32,nospace));
				error = E_SPACE;
				done();
			}
		fclose(tmpf);
		if((tmpf = fopen(lettmp, "r")) == NULL) {
			fprintf(stderr, catgets(catd,NL_SETN,33,"mail: can't re-read %s\n"), lettmp);
			error = E_FILE;
			done();
		}
		if(nlet == (MAXLET-2)){
			fprintf(stderr, catgets(catd,NL_SETN,34,"copyback:Too many letters\n"));
			error = E_SPACE;
			done();
		}
		let[++nlet].adr = stbuf.st_size;
		new = 1;
	}
	/*
	 * Copy mail back to mail file
	 */
	if (fflush(malf) != 0) {
		fprintf(stderr, catgets(catd,NL_SETN,35,"mail: can't update %s\n"), mailfile);
		error = E_FILE;
		done();
	}
	if (ftruncate(fileno(malf), 0) != 0) {
		fprintf(stderr, catgets(catd,NL_SETN,36,"mail: can't update %s\n"), mailfile);
		error = E_FILE;
		done();
	}
	if (fseek(malf, 0L, SEEK_SET) != 0) {
		fprintf(stderr, catgets(catd,NL_SETN,37,"mail: can't update %s\n"), mailfile);
		error = E_FILE;
		done();
	}
	n = 0;
	for (i = 0; i < nlet; i++)
		if (let[i].change != 'd') {
			if (copylet(i, malf, ORDINARY) == FALSE) {
				fprintf(stderr, catgets(catd,NL_SETN,38,"mail: cannot copy mail back\n"));
			}
			n++;
		}
	(void) unlock2(fileno(malf));
	fclose(malf);
	/*
	 * Empty mailbox?
	 */
	if ((n == 0) && ((stbuf.st_mode & 0777)== MFMODE))
		unlink(mailfile);
	if (new && !flgf)
		printf(catgets(catd,NL_SETN,39,"new mail arrived\n"));
	(void) unlock();
}

/*
 * copy mail (f1) to temp (f2)
 */
void
copymt(f1, f2)	
register FILE *f1, *f2;
{
	long nextadr;
	size_t n;
	int mesg = 0;

	nlet = nextadr = 0;
	let[0].adr = 0;
	while (fgets(line, sizeof(line), f1) != NULL) {

		/*
		 * bug nlet should be checked
		 */
		if(line[0] == from[0])
			if (isfrom(line)){
				if(nlet >= (MAXLET-2)) {
					if (!mesg) {
						fprintf(stderr,catgets(catd,NL_SETN,40,"Warning: Too \
many letters, overflowing letters concatenated\n\n"));
						mesg++;
					}
				}
				else
					let[nlet++].adr = nextadr;
			}
		n = strln(line);
		nextadr += n;
		if (write(fileno(f2), (void *)line, n) != n) {
			fclose(f1);
			fclose(f2);
			fprintf(stderr, catgets(catd, NL_SETN, 32, nospace));
			error = E_SPACE;
			done();
		}
	}

	/*
	 * last plus 1
	 */
	let[nlet].adr = nextadr;
}

/*
 * check to see if mail is being forwarded
 *	s	-> mail file
 *	quotes_p-> pointer to int in which is returned an indication
 *		   of whether or not the forwarding address contains
 *		   any backquotes (see comment for "allow_backquotes").
 *
 * returns
 *	TRUE	-> forwarding
 *	FALSE	-> local
 */
areforwarding(s, quotes_p)
char *s;
int  *quotes_p;
{
	register char *p;
	register int c;
	int nquotes=0, retval;
	FILE *fd;
	char fbuf[MAX_FRWD_ADDR_LEN];

	if((fd = fopen(s, "r")) == NULL) {
	        *quotes_p = 0;
		return(FALSE);
	}
	fbuf[0] = '\0';
	if ( (fread((void *)fbuf, sizeof(frwrd) - 1, 1, fd) == 1) &&
	    (strncmp(fbuf, frwrd, sizeof(frwrd) - 1) == SAME) ) {
		for(p = sendto; (c = getc(fd)) != EOF && c != '\n';) {
		        if (c == '`')
				nquotes++;
			if (c != ' ') 
				*p++ = c;
		}
		*p = '\0';
		retval = TRUE;
		goto cleanup;
	}
	retval = FALSE;

cleanup:
	fclose(fd);
	*quotes_p = nquotes;
	return(retval);
}

/*
 * copy letter 
 *	ln	-> index into: letter table
 *	f	-> file descrptor to copy file to
 *	type	-> copy type
 */
copylet(ln, f, type) 
register FILE *f;
{
	register int i;
	register char *s;
	char	buf[512], buf2[512], lastc, ch;
	size_t	n, j, num;
	long	k;
	int	retval;

	fseek(tmpf, let[ln].adr, 0);
	k = let[ln+1].adr - let[ln].adr;
	if(k)
		k--;
	for(i=0;i<k;){
		s = buf;
		num = ((k-i) > sizeof(buf))?sizeof(buf):(k-i);
		if((n = read(fileno(tmpf), (void *)buf, num)) <= 0){
			return(FALSE);
		}
		lastc = buf[n-1];
		if(i == 0){
			for(j=0;j<n;j++,s++){
				if(*s == '\n')
					break;
			}
			if(type != ZAP)
				if(write(fileno(f),(void *)buf,j) == CERROR){
				return(FALSE);
				}
			i += j+1;
			n -= j+1;
			ch = *s++;
			switch(type){
			case REMOTE:
				(void) sprintf(buf2, RMTMSG, thissys);
				retval = write(fileno(f), (void *)buf2, strlen(buf2));
				break;
			case ORDINARY: {
				buf2[0] = ch;
				retval = write(fileno(f), (void *)buf2, 1);
				break;
			    }
			case FORWARD:
				(void) sprintf(buf2, FORWMSG, my_name);
				retval = write(fileno(f), (void *)buf2, strlen(buf2));
				break;
			case ZAP:
				break;
			default:
				retval = CERROR;
				break;
			}
			if (retval == CERROR)
				return(FALSE);
		}
		if(write(fileno(f), (void *)s, n) != n){
			return(FALSE);
		}
		i += n;
	}
	if(type != ZAP || lastc != '\n'){
		read (fileno(tmpf), (void *)buf, 1);
		write(fileno(f),    (void *)buf, 1);
	}
	return(TRUE);
}

/*
 * check for "from" string
 *	lp	-> string to be checked
 * returns
 *	TRUE	-> match
 *	FALSE	-> no match
 */
isfrom(lp)
register char *lp;
{
	register char *p;

	for (p = from; *p; )
		if (*lp++ != *p++)
			return(FALSE);
	return(TRUE);
}

/*
 * Send mail
 *	argc	-> argument count
 *	argv	-> argument list
 */
void
sendmail(argc, argv)
char **argv;
{
	int	aret;
	char	**args;
	int	fromflg = 0;
	char    *collapse();
	struct	tm *bp;
	char    *tp, *zp;
	int     c;
	size_t	n;
	char buf[PLENGTH];
	char *path;     /* accumulated path of sender (collapse()) */

	if ( rmail ) {
	    while ( (c=getopt(argc, argv, "dt")) != EOF )
		switch (c) {
		    case 'd':
			dflag = 1; /* direct local delivery */
			break;
		    case 't':
			toflg = 1; /* add "To:" line to message */
			break;
		    case '?':
			/* skip over unknown options;
			 * this isn't really a nice way of handling
			 * this problem but since "rmail" may have
			 * been invoked by uucp we can't easily tell
			 * the user an unknown option has been
			 * specified, hence we ignore unknown options.
			 */
			break;
		}
	}

	/* collapse ">From " lines to something reasonable:
	 *     ("path" will contain the path of all collapsed lines;
	 *      global variable "hdr_lines" will contain new header
	 *      lines generated by collapse)
	 */
	path = collapse();

	/*
	 * Format time
	 */
	time(&iop);
	bp = localtime(&iop);
	tp = asctime(bp);
	zp = tzname[bp->tm_isdst];

	if ( path[0] == '\0' )
	    (void) strcpy(path, my_name);

	/* only add "From " line on final delivery: */
	/* include HH:MM:SS in timestamp - %.19s   */
	if ( !sendmailflag || dflag ) {
	    (void) sprintf(buf, "%s%s %.19s %.3s %.5s",
			   from, path, tp, zp, tp+20);
	    write(fileno(tmpf), (void *)buf, strlen(buf));
	}

	/* put on header lines generated by collapse() */
	for (;*hdr_lines != (char *)0; hdr_lines++)
	    write(fileno(tmpf), (void *)(*hdr_lines), strlen(*hdr_lines));

	/*
	 * Copy to list in mail entry?
	 */
	if (toflg == 1 && argc > 1) {
		if (rmail) {
		/* Set up argc and argv so To: line is correct. */
		/* See printmail() handling of dflag and toflg. */
			argc--;
			argv++;
		}
		aret = argc;
		args = argv;
		write(fileno(tmpf), (void *)TO, strlen(TO));
		while(--aret > 0) {
			++args;
			write(fileno(tmpf), (void *)(*args), strlen(*args));
			write(fileno(tmpf), (void *)" ", 1);
		}
		write(fileno(tmpf), (void *)"\n", 1);
	}
	iop = ftell(tmpf);
	flgf = 1;
	/*
	 * Read mail message
	 */
	saveint = setsig(SIGINT, savdead);
	fromlevel = 0;

	/* first process last line read by "collapse" but not written yet */

	do {
		if (line[0] == '.' && line[1] == '\n')
			if ( !dflag || !rmail )
				break;
		/*
		 * If "from" string is present prepend with
		 * a ">" so it is no longer interpreted as
		 * the last system fowarding the mail.
		 */
		if(line[0] == from[0])
			if (isfrom(line))
				write(fileno(tmpf), (void *)">", 1);
		/*
		 * Find out how many "from" lines
		 */
		if (fromflg == 0 && (strncmp(line, "From", 4) == SAME || strncmp(line, ">From", 5) == SAME))
			fromlevel++;
		else
			fromflg = 1;
		n = strln(line);
		if (write(fileno(tmpf), (void *)line, n) != n) {
			fclose(tmpf);
			fprintf(stderr, catgets(catd, NL_SETN, 32, nospace));
			if (dflag)
				error = EX_IOERR;
			else
				error = E_SPACE;
			return;
		}
		flgf = 0;
	} while ( fgets(line, sizeof(line), stdin) != NULL );

	setsig(SIGINT, saveint);
	write(fileno(tmpf), (void *)"\n", 1);
	/*
	 * In order to use some of the subroutines that
	 * are used to read mail, the let array must be set up
	 */
	nlet = 1;
	let[0].adr = 0;
	let[1].adr = ftell(tmpf);
	if (fclose(tmpf) == EOF) {
	    fclose(tmpf);
	    fprintf(stderr, catgets(catd, NL_SETN, 32, nospace));
	    if (dflag)
			error = EX_OSERR;
            else	
             		error = E_SPACE;
	    return;
	}
	if (flgf)
		return;
	if((tmpf = fopen(lettmp, "r")) == NULL) {
		fprintf(stderr, catgets(catd,NL_SETN,42,"mail: cannot reopen %s for reading\n"), lettmp);
		if (dflag)
			error = EX_NOINPUT;
		else
			error = E_FILE;
		return;
	}
	/*
	 * Send a copy of the letter to the specified users
	 */

	/* advance argv, argc beyond options */
	for (; argv[1][0] == '-'; argv++, argc--)
	    ;

	if (error == 0)
		while (--argc > 0)
			if ( sendlet(0, *++argv, 0, path) == FALSE && !dflag)
				error++;
	/*
	 * Save a copy of the letter in dead.letter if
	 * any letters can't be sent
	 */

	if (error) {
		/*
		 * Try to create dead letter in current directory
		 * or in home directory
		 */
		(void) umask(umsave);
		if((aret=legal(&dead[1])))
			malf = fopen(&dead[1], "w");
		if ((malf == NULL) || (aret == 0)) {

			/*
			 * try to create in $HOME
			 */
			if((hmdead = (char *) malloc(strlen(home) + strlen(dead) + 1)) == NULL) {
				fprintf(stderr, catgets(catd,NL_SETN,43,"mail: can't malloc\n"));
				goto out;
			}
			cat(hmdead, home, dead);
			if((aret=legal(hmdead)))
				malf = fopen(hmdead, "w");
			if ((malf == NULL) || (aret == 0)) {
				fprintf(stderr, catgets(catd,NL_SETN,44,"mail: cannot create %s\n"), &dead[1]);
			out:
				if (!dflag)
					error = E_FILE;
				fclose(tmpf);
				(void) umask(7);
				return;
			}  else {
				(void) chmod(hmdead, 0600);
				printf(catgets(catd,NL_SETN,45,"Mail saved in %s\n"), hmdead);
			}
		} else {
			(void) chmod(&dead[1], 0600);
			printf(catgets(catd,NL_SETN,46,"Mail saved in %s\n"), &dead[1]);
		}
		/*
		 * Copy letter into dead letter box
		 */
		(void) umask(7);
		if (copylet(0, malf, ORDINARY) == FALSE) {
			fprintf(stderr,catgets(catd,NL_SETN,47,"mail: cannot save in dead letter\n"));
		}
		fclose(malf);
	}
	fclose(tmpf);
}


/*
 * collapse -
 *      This routine reads the >From ... remote from ... lines that
 *      UUCP is so fond of and turns them into something reasonable.
 *
 *      Later on if sendmail is called, it is given with the -f option
 *      an argument which is built from these lines.
 */

char *
collapse()
{
    char ufrom[PLENGTH],            /* user on remote system */
	 sys[PLENGTH],              /* a system in path */
	 junk[64],              /* scratchpad */
	 hdr_line[PLENGTH];         /* for making a header line */
    static char *hdr[MAXHDR];   /* array with header lines */
    static char path[PLENGTH];    /* accumulated path of sender (collapse()) */
    register char *cp;
    register char *uf=(char *)0;/* ptr into ufrom */

    /* make sizes of character arrays much bigger than
     * we need to avoid overflow of them in the "scanf"
     * routine where these arrays are used.
     */
    char dayname[20],           /* Mon, Tue, Wed, ... */
	 month[20],             /* Jan, Feb, Mar, ... */
	 day[20],               /* 1 through 31 */
	 time_[20],              /* hh:mm[:ss] */
	 zone[20],              /* time zone; e.g. MST */
	 year[20];              /* e.g. 1986 */

    int fromlines = 0,          /* set if atleast 1 "From" line seen */
	leave = 0,              /* if set, we must return */
	remote_from,            /* set if line has "remote from" string */
	i;

    /* clean up array of header lines */

    for (i=0; i < MAXHDR; i++) {
	if ( hdr[i] != (char *)0 ) {
	    free((void *)hdr[i]); /* free memory allocated "last" time */
	    hdr[i] = (char *)0;
	}
    }

    ufrom[0] = '\0';
    path[0] = '\0';
    hdr_lines = hdr; /* initialize hdr_lines */

    while ( fgets(line, sizeof(line), stdin) != NULL ) {

	/* make sure we got a complete line.  if not, then header is
	   corrupt so punt. */

	char	*np;
	for(np = line; np < line + sizeof(line); np++)
		if (*np == '\n')
			break;
	if (np >= line + sizeof(line)) {
		fprintf(stderr,catgets(catd,NL_SETN,48, "header line too long\n"));
		error = EX_DATAERR;
		done();
	}

	/* quit if:
	 *    1. line does start with "From " or ">From " but
	 *       doesn't have the minimum number of items on it, or
	 *    2. line doesn't start with "From " or ">From "
	 */

	if ( strncmp(line,"From ",5) == 0 || strncmp(line,">From ",6) == 0 ) {
	    i = sscanf(line, "%s %s %s %s %s %s %s %s", junk, ufrom,
			      dayname, month, day, time_, zone, year);
	    /* we're expecting atleast 7 items */
	    if ( i < 7 )
		leave++;
	} else
	    leave++;

	if ( leave )
	    if ( fromlines == 0 ) {
		path[0] = '\0';
		return path; /* no "From " or ">From " lines found */
	    } else
		break;

	fromlines++; /* processed atleast one "From " or ">From " line */

	/* if "zone" has the "year" value, fix it */
	if ( strlen(zone) == 4 ) {
	    (void) strcpy(year, zone);
	    zone[0] = '\0';
	}

	cp = line;
	uf = ufrom;
	sys[0] = '\0';
	remote_from = 0; /* don't know if we have the "remote from" */
			 /* string in current "From " line.         */

	for (;;) {
	    cp = strchr(cp+1, 'r'); /* next occurrence of 'r' */
	    if ( cp != NULL ) {
		/* is it a "remote from" string? */
		if ( strncmp(cp, "remote from ", 12) == 0 )
		    break;
	    } else {
		/* No "remote from" string in this "From " line.
		 * Instead try to extract a system name from "ufrom" string
		 * by putting everything up until the last '!' into
		 * the "sys" string.
		 */
		register char *p = strrchr(uf, '!');

		if (p != NULL) {
		    *p = '\0';
		    (void) strcpy(sys, uf);
		    uf = p + 1;
		    break;
		}
		break;
	    }
	}

	if ( cp != NULL ) {
	    /* extract system name from "remote from" string */
	    (void) sscanf(cp, "remote from %s", sys);
	    remote_from++; /* got "remote from" string in this "From" line */
	}

	if ( sys[0] != '\0' ) {
	    (void) strcat(path, sys); /* tack system name onto end */
	    (void) strcat(path, "!"); /* and also the ! */

	    if ( remote_from ) {
		/* generate "Received: ...." line here;
		 *
		 * Note: "Received" lines may be changed to "Sent-By" lines
		 *       in the future (a la RFC-976; Mark Horton) when
		 *       the situation stabilizes and becomes clearer.
		 */
		if ( zone[0] != '\0' )
		    (void) sprintf(hdr_line,
				   "Received: from %s with uucp; %s, %s %s %s %s %s\n",
				   sys, dayname, day, month, year+2, time_, zone);
		else
		    (void) sprintf(hdr_line,
				   "Received: from %s with uucp; %s, %s %s %s %s\n",
				   sys, dayname, day, month, year+2, time_);
		*hdr_lines = (char *) malloc(strlen(hdr_line)+1);
		(void) strcpy(*hdr_lines++, hdr_line);
	    }
	}
    }

    if (uf)
	(void) strcat(path, uf);
    hdr_lines = hdr; /* reset hdr_lines */

    return path;
}

void
savdead()
{
	setsig(SIGINT, saveint);
	error++;
}

/*
 * send mail to remote system taking fowarding into account
 *	n	-> index into mail table
 *	name	-> mail destination
 * returns
 *	TRUE	-> sent mail
 *	FALSE	-> can't send mail
 */
sendrmt(n, name)
register char *name;
{
# define NSCCONS	"/usr/nsc/cons/"
	register char *p;
	register local = 0;
	FILE *rmf;
	char rsys[MAXHOSTNAMELEN], cmd[200];
#ifdef USE_NUSEND
	char remote[30];
#endif

	/*
	 * assume mail is for remote
	 * look for bang to confirm that
	 * assumption
	 */

	if ( sendmailflag ) {
	    (void) sprintf(cmd, "%s %s", DELIVERMAIL, name);
	    local++;
	} else {
	    local = 0;
	    while (*name=='!')
		    name++;
	    for(p=rsys; *name!='!'; *p++ = *name++)
		    if (*name=='\0') {
			    local++;
			    break;
		    }
	    *p = '\0';
	    if ((!local && *name=='\0') || (local && *rsys=='\0')) {
		    fprintf(stderr,catgets(catd,NL_SETN,49, "null name\n"));
		    return(FALSE);
	    }
	    if (local)
#ifdef V4FS
		    (void) sprintf(cmd, "/usr/bin/mail %s", rsys);
#else
		    (void) sprintf(cmd, "/bin/mail %s", rsys);
#endif
	    if (strcmp(thissys, rsys) == SAME) {
		    local++;
		    if ( *(name+1) == '\0' ) {
			fprintf(stderr,catgets(catd,NL_SETN,50, "null name\n"));
			return FALSE;
		    } else
#ifdef V4FS
			(void) sprintf(cmd, "/usr/bin/mail %s", name+1);
#else
			(void) sprintf(cmd, "/bin/mail %s", name+1);
#endif
	    }
	}

	/*
	 * send local mail or remote via uux
	 */
	if (!local) {
		if (fromlevel > FROMLEVELS)
			return(FALSE);

#ifdef USE_NUSEND
		/*
		 * If mail can't be sent over NSC network
		 * use uucp.
		 */
		if (setjmp(nusendfail) == 0) {
			nusendjmp = TRUE;
			(void) sprintf(remote, "%s%s", NSCCONS, rsys);
			if (access(remote, A_EXIST) != CERROR) {
				/*
				 * Send mail over NSC network
				 */
				(void) sprintf(cmd,
					       "nusend -d %s -s -e -!'rmail %s' - 2>/dev/null",
					       rsys, name);
#ifdef DEBUG
printf("%s\n", cmd);
#endif
				if ((rmf=popen(cmd, "w")) != NULL) {
					copylet(n, rmf, local? FORWARD: REMOTE);
					if (pclose(rmf) == 0) {
						nusendjmp = FALSE;
						return(TRUE);
					}
				}
			}
		}
		nusendjmp = FALSE;
#endif

#ifdef USE_USEND
		if (setjmp(usendfail) == 0) {
			usendjmp = TRUE;
			(void) sprintf(cmd,
				       "usend -s -d%s -uNoLogin -!'rmail %s' - 2>/dev/null",
				       rsys, name);
#ifdef DEBUG
printf("%s\n", cmd);
#endif
			if ((rmf=popen(cmd, "w")) != NULL) {
				copylet(n, rmf, local? FORWARD: REMOTE);
				if (pclose(rmf) == 0) {
					usendjmp = FALSE;
					return(TRUE);
				}
			}
		}
		usendjmp = FALSE;
#endif

		/*
		 * Use uux to send mail
		 */
		if (strchr(name+1, '!'))
			(void) sprintf(cmd,
				       "exec /usr/bin/uux - %s!rmail \\(%s\\)",
				       rsys, name+1);
		else
			(void) sprintf(cmd,
				       "exec /usr/bin/uux - %s!rmail %s",
				       rsys, name+1);
	}
#ifdef DEBUG
printf("%s\n", cmd);
#endif
	/*
	 * copy letter to pipe
	 */
	if ((rmf=popen(cmd, "w")) == NULL)
		return(FALSE);
	if (copylet(n, rmf, local? FORWARD: REMOTE) == FALSE) {
		fprintf(stderr, catgets(catd,NL_SETN,51,"mail: cannot pipe to mail command\n"));
		(void) pclose(rmf);
		return(FALSE);
	}

	/*
	 * check status
	 */
	return(pclose(rmf)==0 ? TRUE : FALSE);
}

/*
 * send letter n to name
 *	n	-> letter number
 *	name	-> mail destination
 *	level	-> depth of recursion for forwarding
 * returns
 *	TRUE	-> mail sent
 *	FALSE	-> can't send mail
 */
static
sendlet(n, name, level, path)
int	n;
char	*name;
char    *path;
{
	register char *p;
	char	file[MAXFILENAME];
	struct  passwd  *pwd;
	int fwding=0, quotes;
	void (*istat)(), (*qstat)(), (*hstat)();

	if ( sendmailflag && !dflag ) {
	    int s, i;
	    pid_t pid, p;

	    /* If "sendmail" exists and we were invoked without
	     * the -d option, then pass to DELIVERMAIL:
	     */


	    rewind(tmpf);
	    pid = fork();
	    if ( pid == CERROR ) {
		perror("fork");
		exit(1);
	    }

	    if ( pid == 0 ) {
		int next = 0; /* count for next argument */
		char *ap[6];

		for (i = SIGHUP; i <= SIGQUIT; i++)
		    signal(i, SIG_IGN);

		s = fileno(tmpf);
		for (i=getnumfds()-1;i>=3;i--)
		    if (i != s)
			close(i);
		close(0);
		dup(s);
		close(s);

		ap[next++] = "sendmail";
		ap[next++] = "-oem";
		ap[next++] = "-oi";
		if ( path[0] ) {
		    ap[next] = (char *) malloc(strlen(path)+3);
		    (void) sprintf(ap[next++], "-f%s", path);
		}
		ap[next++] = name;
		ap[next] = (char *)0;

		execv(DELIVERMAIL, ap);
		perror(DELIVERMAIL);
		exit(1);

	    }

	    /* wait for child's completion */
	    while ( (p = wait(&s)) != pid && p != CERROR )
		;
	    return (s == 0) ? TRUE : FALSE;
	}

	if(level > FROMLEVELS) {
		fprintf(stderr, catgets(catd,NL_SETN,52,"unbounded forwarding\n"));
		return(FALSE);
	}
	if (strcmp(name, "-") == SAME)
		return(TRUE);
	/*
	 * See if mail is to be forwarded
	 */
	if ( !sendmailflag && !dflag ) {
	    for(p=name; *p!='!' &&*p!='\0'; p++)
		    ;
	    if (*p == '!')
		return(sendrmt(n, name));
	}

	/*
	 * See if user has specified that mail is to be fowarded
	 */
	cat(file, maildir, name);

	fwding = areforwarding(file, &quotes);
	if (quotes) {
	    openlog("rmail", LOG_CONS|LOG_PID, LOG_LOCAL1);
	    if (syslog(LOG_INFO,
		       "%sallowing forwarding from %s",
		       allow_backquotes ? " " : "dis", file) != 0)
	        fprintf(stderr, "syslog() failed\n");
	    closelog();

	    fprintf(stderr, "Cannot forward mail to address specified in %s\n",
		    file);
	    if (!allow_backquotes)
		return(FALSE);
	}

	if (fwding)
	    if ( !sendmailflag )
		return(sendlet(n, sendto, level+1, path));
	    else {
		int s, i;
		pid_t pid, p;

		/* pass it back to DELIVERMAIL */

		rewind(tmpf);
		pid = fork();

		if ( pid == CERROR ) {
		    perror("fork");
		    exit(1);
		}

		if ( pid == 0 ) {
		    int next = 0; /* count for next argument */
		    char *ap[6], c;

		    for (i = SIGHUP; i <= SIGQUIT; i++)
			signal(i, SIG_IGN);

		    s = fileno(tmpf);
		    for (i=getnumfds()-1;i>=3;i--)
			if (i != s)
			    close(i);
		    close(0);
		    dup(s);
		    close(s);

		    /*
		     * We must now get rid of the "From " line at
		     * the beginning of the letter because we're
		     * passing it back to "sendmail" which can't
		     * deal with "From " lines.
		     *
		     * So throw away the first line:
		     */
		    do
			read(0, (void *)&c, 1);
		    while ( c != '\n' );

		    ap[next++] = "sendmail";
		    ap[next++] = "-oem";
		    ap[next++] = "-oi";
		    if ( path[0] ) {
			ap[next] = (char *) malloc(strlen(path)+3);
			(void) sprintf(ap[next++], "-f%s", path);
		    }
		    ap[next++] = sendto; /* forwarding address */
		    ap[next] = (char *)0;

		    execv(DELIVERMAIL, ap);
		    perror(DELIVERMAIL);
		    exit(1);
		}

		/* wait for child's completion */
		while ( (p = wait(&s)) != pid && p != CERROR )
		    ;
		return (s == 0) ? TRUE : FALSE;
	    }


	/*
	 * see if user exists on this system
	 */
	setpwent();	

	if((pwd = getpwnam(name)) == NULL){
		fprintf(stderr, catgets(catd,NL_SETN,53,"mail: can't send to %s\n"), name);
		if (dflag)
			error = EX_NOUSER;
		return(FALSE);
	}

	cat(file, maildir, name);
	lock(file);

	/*
	 * If mail file does not exist create it
	 * with the correct uid and gid
	 */
	if(access(file, A_EXIST) == CERROR) {
		(void) umask(0);
		istat = signal(SIGINT, SIG_IGN);
		qstat = signal(SIGQUIT, SIG_IGN);
		hstat = signal(SIGHUP, SIG_IGN);

		/* change effective uid temporarily so file will not be
		   owned by sending user */
		(void) setuid(suid);
		close(creat(file, MFMODE));
		(void) umask(7);
		(void) chown(file, pwd->pw_uid, getegid());
		(void) setuid(ruid);
		signal(SIGINT, istat);
		signal(SIGQUIT, qstat);
		signal(SIGHUP, hstat);
	}

	/*
	 * Append letter to mail box.  We must have locked the file before
	 * associating an append stream with it to be assured that
	 * the stream's file pointer is correct.
	 */
	if ((malf = fopen(file, "a")) == NULL) {
		fprintf(stderr, catgets(catd,NL_SETN,54,"mail: cannot append to %s\n"), file);
		if (dflag)
			error = EX_NOPERM;
		(void) unlock();
		return(FALSE);
	}

	lock2(fileno(malf));

	if (copylet(n, malf, ORDINARY) == FALSE) {
		fprintf(stderr, catgets(catd,NL_SETN,56,"mail: cannot append to %s\n"), file);
		if (dflag)
			error = EX_IOERR;
		(void) unlock2(fileno(malf));
		fclose(malf);
		(void) unlock();
		return(FALSE);
	}
	(void) unlock2(fileno(malf));
	fclose(malf);
	(void) unlock();
	return(TRUE);
}


/*
 * signal catching routine
 * reset signals on quits and interupts
 * exit on other signals
 *	i	-> signal #
 */
void
delete(i)
register int i;
{
	setsig(i, delete);

#ifdef USE_NUSEND
	if (i == SIGPIPE && nusendjmp == TRUE)
		longjmp(nusendfail, 1);
#endif

#ifdef USE_USEND
	if (i == SIGPIPE && usendjmp == TRUE)
		longjmp(usendfail, 1);
#endif

	if(i>SIGQUIT){
		fprintf(stderr, catgets(catd,NL_SETN,57,"mail: error signal %d\n"), i);
	}else
		fprintf(stderr, "\n");
	if(delflg && (i==SIGINT || i==SIGQUIT))
		longjmp(sjbuf, 1);
	done();
}

/*
 * clean up lock files and exit
 *
 * Since we are going to bail anyway, do not bother doing an unlock2(),
 * since the lockf() goes away when the file is closed inside of exit().
 */
void
done()
{
	(void) unlock();
	(void) unlink(lettmp);
	(void) unlink(rmtbuf);
	exit(error);
}

/*
 * create mail lock file
 *	file	-> lock file name
 * returns:
 *	none
 *
 * Calling this routine is the first step in fully locking the incoming
 * mailbox.  This routine creates the /usr/mail/<username>.lock file.
 * A call to lockf() (via lock2()) needs to be done to repel accesses to
 * the incoming mailbox for mailers that use/prefer that method of locking.
 *
 * When this first phase has been completed, we half-set "locked" to "LOCKMADE"
 * to denote that the lockfile was made.  When lock2()/lockf() is complete
 * the locking level will be promoted to "LOCKON|LOCKMADE".
 *
 * If we were called with locked != LOCKOFF there is some sort of programming
 * error and we complain rather than silently return or exit without making
 * the lock file.
 */
void
lock(file)
char *file;
{
	register int fd, i=0;

	if (locked == LOCKOFF) {
	    cat(curlock, file, maillock);
	    for (i=0; i<MAX_LOCK_TRIES; i++) {
		if ((fd = open(curlock, O_CREAT | O_EXCL, C_NOACCESS)) 
			!= CERROR) {
				close(fd);
				locked = LOCKMADE;
				return;
		} else
			(void) sleep(2);
	    }
	}

	fprintf(stderr, catgets(catd,NL_SETN,59,"mail: %s not creatable after %d tries\n"), curlock, i);
	perror(curlock);
	if (dflag)
		error = EX_CANTCREAT;
	else
		error = E_FILE;
	curlock[0] = '\0';
	done();
}

/*
 * Lock the body of the file against accesses with lockf().  This is
 * undone by calling unlock2().
 *
 * This routine should probably print out error messages as well as
 * returning non-zero return values, but we cannot change the message
 * catalogs at this point in the release.
 */
void
lock2(fd)
{
	register int j=0;

	if (locked == LOCKMADE) {
	    for (j=0; j<MAX_LOCK_TRIES; j++) {
		if (lockf(fd, F_TLOCK, 0) == 0) {
			locked |= LOCKON;
			return;
		}
		(void) sleep(2);
	    }
	}

	fprintf(stderr, catgets(catd,NL_SETN,58,"mail: can not lock on %s after %d tries\n"), mailfile, j);
	perror(mailfile);
	if (dflag)
		error = EX_CANTCREAT;
	else
		error = E_FILE;
	done();
}

/*
 * Unlock both locks in the reverse order that we acquired them.
 */
unlock_all(fd)
{
	int err, err2;

	err2 = unlock2(fd);
	err  = unlock();

	return((err2 != 0) ? err2 : err);
}

unlock2(fd)
{
	if (!(locked & LOCKON)) {
		/*
		 * We should burp an error here because we would never expect
		 * this to happen, but that would involve changing those
		 * !@#$%& NLS message catalogs...
		 */
		return(-1);
	}
	if (lockf(fd, F_ULOCK, 0) != 0) {
		/* should complain, but ... */
		locked &= ~LOCKON;
		return(-2);
	}
	locked &= ~LOCKON;
	return(0);
}

/*
 * Undo the lockfile (/usr/mail/<username>.lock) made by lock().
 */
unlock()
{
	if (locked != LOCKMADE) {
		/* another place where we should complain... */
		return(-1);
	}
	if (unlink(curlock) < 0) {
		/* ditto */
		return(-2);
	}
	locked = LOCKOFF;
	return(0);
}

/*
 * concatenate from1 and from2 to to
 *	to	-> destination string
 *	from1	-> source string
 *	from2	-> source string
 * return:
 *	none
 */
void
cat(to, from1, from2)
register char *to, *from1, *from2;
{

	for (; *from1; )
		*to++ = *from1++;
	for (; *from2; )
		*to++ = *from2++;
	*to = '\0';
}

/*
 * get next token
 *	p	-> string to be searched
 *	s	-> area to return token
 * returns:
 *	p	-> updated string pointer
 *	s	-> token
 *	NULL	-> no token
 */
char *getarg(s, p)	
register char *s, *p;
{
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\n' || *p == '\0')
		return(NULL);
	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
		*s++ = *p++;
	*s = '\0';
	return(p);
}

/*
 * check existence of file
 *	file	-> file to check
 * returns:
 *	0	-> exists unwriteable
 *	1	-> exists writeable
 *	2	-> does not exist
 */
legal(file)
register char *file;
{
	register char *sp;
	char dfile[MAXFILENAME];

	/*
	 * If file does not exist then
	 * try "." if file name has no "/"
	 * For file names that have a "/", try check
	 * for existence of previous directory
	 */
	if(access(file, A_EXIST) == A_OK)
		if(access(file, A_WRITE) == A_OK)
			return(1);
		else	return(0);
	else {
		if((sp=strrchr(file, '/')) == NULL)
			cat(dfile, ".", "");
		else {
			(void) strncpy(dfile, file, (size_t)(sp - file));
			dfile[sp - file] = '\0';
		}
		if(access(dfile, A_WRITE) == CERROR) 
			return(0);
		return(2);
	}
}

/*
 * Invoke shell to execute command waiting for
 * command to terminate
 *	s	-> command string
 * return:
 *	status	-> command exit status
 */
system(s)
char *s;
{
	register pid_t pid, w;
	int status;
	void (*istat)(), (*qstat)();
	char *arg0;
	char *shell;
	char Shell[BUFSIZ];
	char *cmd = "-c";

	/*
	 * Spawn the shell to execute command, however,
	 * since the mail command runs setgid mode
	 * reset the effective group id to the real
	 * group id so that the command does not
	 * acquire any special privileges
	 */
	if (((shell = getenv("SHELL")) == NULL) || (*shell == '\0'))
#ifdef V4FS
		shell = "/usr/bin/sh";
#else
		shell = "/bin/sh";
#endif
	if (( arg0 = strrchr(shell, '/')) == NULL) {
		arg0 = shell;
#ifdef V4FS
		(void) strcpy(Shell, "/usr/bin/");
#else
		(void) strcpy(Shell, "/bin/");
#endif
		(void) strcat(Shell, arg0);
		shell = Shell;
	} else
		arg0++;
#ifdef V4FS
	if (!strcmp(shell, "/usr/bin/csh")) 
#else
	if (!strcmp(shell, "/bin/csh")) 
#endif
		cmd = "-cf";
#ifdef V4FS
	else if (strcmp(shell, "/usr/bin/ksh") == NULL || strcmp(shell,"/usr/bin/rksh") == NULL) 
#else
	else if (strcmp(shell, "/bin/ksh") == NULL || strcmp(shell,"/bin/rksh") == NULL) 
#endif
		cmd = "-cp";
	
	if ((pid = fork()) == CHILD) {
		(void) setuid(getuid());
		(void) setgid(getgid());
		execlp(shell, arg0, cmd, s, NULL);
		perror(shell);
		_exit(127);
	}

	/*
	 * Parent temporarily ignores signals so it 
	 * will remain around for command to finish
	 */
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while ((w = wait(&status)) != pid && w != CERROR)
		;
	if (w == CERROR)
		status = CERROR;
	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);
	return(status);
}

/*
*  strln - determine length of line (terminated by '\n')
*/
size_t
strln (s)
char *s;
{
	int i;

	for (i=0 ; i < LSIZE && s[i] != '\n' && s[i] != '\0'; i++);
	return(i+1);
}


char *
getsysname()
{
/* SEE BELOW:
 *    static struct utsname utsn;
 */
    static char nodename[MAXHOSTNAMELEN];
#if defined(DUX) || defined(DISKLESS)
    struct cct_entry *cctptr;

    /* Get root server name. */

    setccent();
    while ((cctptr = getccent()) != (struct cct_entry *)0) {
	if (cctptr->cnode_type == 'r') {

	    (void) strcpy(nodename,cctptr->cnode_name);
	    endccent();
	    return nodename;
	}
    }

    endccent();

    /* Fall through and get hostname from uname(2) call. */

#endif /* DUX || DISKLESS */

	gethostname(nodename, MAXHOSTNAMELEN);
	return (nodename);
/*
 * NOW: fall thru and get hostname from gethostname(2)
 * call, above...
 *
 *   uname(&utsn);
 *
 *   return utsn.nodename;
 */
}

#ifndef lint
/*
 * This function exists in order to keep the number of messages in the NLS
 * message catalogs constant with respect to a few versions ago.  Even though
 * removal of a message does/need not affect the content of the message
 * catalogs that have already been translated, there is a tool used in the
 * lab that counts the number of messages to make sure that they are constant
 * from when they were last translated for the release.
 *
 * This code can be removed for the next major release after 9.0.
 */
static int
dummy()
{
    char *s;

    s = catgets(catd,NL_SETN,55,"mail: cannot append to %s\n");
    s = catgets(catd,NL_SETN,41, nospace);
}
#endif
