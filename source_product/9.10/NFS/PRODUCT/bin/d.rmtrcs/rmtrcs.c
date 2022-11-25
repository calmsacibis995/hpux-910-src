/* $Source: /source/hpux_source/networking/rcs/nfs90_800/bin/d.rmtrcs/RCS/rmtrcs.c,v $
 * $Revision: 1.4.109.1 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 91/11/19 14:00:31 $
 *
 * Revision 1.3  91/11/01  15:28:18  15:28:18  kcs (Kernel Control System)
 * The rcs commands don't reside on /usr/local/bin any longer.  They now reside
 * in /usr/bin.
 * rev 3.3 author: chm;
 * 
 * Revision 3.3  88/05/23  11:14:44  11:14:44  chm
 * The rcs commands don't reside on /usr/local/bin any longer.  They now reside
 * in /usr/bin.
 * 
 * Revision 3.2  87/07/10  16:16:05  16:16:05  dds (Darren Smith)
 * Made rmtrcs always route thru the same account instead of just using the same name
 * 
 * Revision 1.13  86/08/06  15:08:33  15:08:33  jcm (J. C. Mercier)
 * Ifdef'ed out code to check for -x or -y.
 * We no longer wan to use aci with kernel files.
 * 
 * Revision 1.12  86/07/22  15:46:20  15:46:20  lam (Susanna Lam)
 * rcp is in /usr/bin.
 * 
 * Revision 1.11  86/07/22  10:32:20  10:32:20  lam (Susanna Lam)
 * The remote shell to use is at /usr/bin/remsh, /usr/local/bin/rsh is
 * a restricted shell & therefore does not work for us.
 * 
 * Revision 1.10  86/06/25  11:21:47  11:21:47  jcm (J. C. Mercier)
 * Rrlog is having problems with remote shell.
 * Ifdef'ed out with LANBUG the closing of stdin
 * so it will work better.
 * 
 * Revision 1.9  86/06/24  10:43:19  10:43:19  jcm (J. C. Mercier)
 * KDIR is now the official sys.rcs directory.
 * 
 * Revision 1.8  86/06/14  15:09:09  jcm (J. C. Mercier)
 * Added support for rlocks under HPUX>
 * This means that you need a new locks program on the HPUX
 * machine containing the ,v files.  Also, the argument should
 * just be ".".
 * 
 * Revision 1.7  86/06/13  17:13:30  jcm (J. C. Mercier)
 * Added support for rcscheck when the ,v files are on an HPUX
 * system which does not have symbolic links.  Next, we will
 * do the same for markrev, markbr and locks.  Note that these
 * server programs on the HPUX system also have to be modified
 * accordingly.
 * 
 * Revision 1.6  86/06/13  11:18:46  jcm (J. C. Mercier)
 * Fixed compilation of ACI without KCI.
 * Also, KHOST will now be hpisoa1 under HPUX
 * and hpdsa under VAX.
 * 
 * Revision 1.5  86/06/13  10:46:33  jcm (J. C. Mercier)
 * This version supports kci.
 * Kci is, potentially, another version of ci.
 * For us, it will be /mnt/azure/bin/ci and it
 * will look a lot like aci since it will also
 * require -x or -y options.  Note that if
 * the RMTRCS link matches KHOST and KDIR
 * we force kci even if the user typed ci or aci.
 * 
 * Revision 1.4  86/06/13  09:26:13  jcm (J. C. Mercier)
 * Fixed a dumb typo in the previous revision.
 * This rev can handle ,v files on Indigos as
 * well as Vaxen.  Recursive commands (e.g.
 * rcscheck, locks) are NOT supported when
 * ,v files are on Indigos... yet.
 * 
 * Revision 1.3  86/06/12  13:25:43  jcm (J. C. Mercier)
 * We will now allow rmtrcs to run on a vax/Indigo and
 * with the target ,v files on a vax/Indigo.  I fixed
 * around the command paths using ifdefs.  Also, we
 * now allow aci as a separate independent command
 * instead of forcing it to also be ci.
 * 
 * Revision 1.2  86/03/31  12:25:26  gburns (Greg Burns)
 * On hp-ux, look for rsh in /usr/local/bin instead of /usr/ucb.
 * We still look for rcp in /usr/ucb, since that side is executed
 * on a vax.  Once the vaxen go, this should also change to
 * /usr/local/bin.
 * 
 * Revision 1.1  86/03/20  16:11:10  gburns (Greg Burns)
 * Initial revision
 * 
 * Revision 1.18  86/01/27  10:39:38  jcm (J. C. Mercier)
 * Fixed bug in the way we tried to fix the bug of rev1.16.
 * Hopefully I got it right this time.  The problem was
 * that the -x option was still being passed to the co.
 * 
 * Revision 1.17  86/01/22  16:45:13  jcm (J. C. Mercier)
 * Added abort condition when ci/aci is specified and neither
 * -x nor -y is specified.
 * 
 * Revision 1.16  86/01/21  12:18:50  jcm (J. C. Mercier)
 * Changed HPUX to hpux.  Added ACI for aci operation.
 * Also, fixed bug under aci.  We were passing
 * the -x and/or -y to the ensuing co upon a
 * ci [-u | -l].  That caused the co to croak.
 * 
 * Revision 1.15  86/01/20  17:18:23  gburns (Greg Burns)
 * co core dumped on hp-ux when invoked with no arguments.  Fixed.
 * 
 * Revision 1.14  86/01/20  16:58:10  gburns (Greg Burns)
 * Ported rmtrcs to Indigo hp-ux.  Note: Since arpa services are currently
 * not reliable from Indigo->Vax, testing has only been done from Vax to
 * Indigo (i.e.: co, rcscheck, etc.).  Do not use ci until Indigo to Vax
 * arpa services are debugged.
 * 
 * Revision 1.13  86/01/17  21:44:31  jcm (J. C. Mercier)
 * CI is now /mnt/azure/bin/aci.
 * 
 * Revision 1.12  86/01/16  20:30:32  jcm (J. C. Mercier)
 * Added capability for aci.
 * Need to remember to change the define when we
 * install it.  Next: promtp_user and fnames.
 * 
 * Revision 1.11  85/10/18  14:04:55  wallace (Kevin G. Wallace)
 * Increased size of argument list arrays.
 * 
 * Revision 1.10  85/05/21  10:23:39  wallace (Kevin G. Wallace)
 * Added extension numbers to the .RBK back-up files.
 * 
 * Revision 1.9  85/02/20  18:00:41  wallace (Kevin G. Wallace)
 * Made rmtrcs ci -f [-l | -u] drop the -f on the ensuing co.
 * 
 * Revision 1.8  85/01/08  15:49:41  wallace (Kevin G. Wallace)
 * Increased size of log message buffer.
 * 
 * Revision 1.7  84/09/13  13:46:22  wallace (Kevin G. Wallace)
 * Fixed -m option to ci, such that rmtrcs does not prompt for a
 * log message and it sends files to the remote system via standard
 * input.  Placed single quotes are -c and -m options.  Made
 * execlocally() use the real argv, not the out produced by
 * parseargs().  Added RCSid.
 * 
 * Revision 1.6  84/09/04  22:14:41  wallace (Kevin G. Wallace)
 * Made some changes to the few comments.
 * 
 * Revision 1.5  84/09/04  09:47:49  wallace (Kevin G. Wallace)
 * Added support for locks, markbr, and markrev.
 * 
 * Revision 1.4  84/09/03  10:16:09  wallace (Kevin G. Wallace)
 * Added remote version of rcscheck.
 * 
 * Revision 1.3  84/09/01  15:49:55  wallace (Kevin G. Wallace)
 * Added ability to deal with multiple files.  Made ci
 * move the orignal to a .RBK file.  Added support for
 * ci -u and -l.  Added support for re-using log messages.
 * 
 * Revision 1.2  84/08/31  21:09:39  wallace (Kevin G. Wallace)
 * Header added.  
 * 
 * $Endlog$
 */

/*
 * Remote Revision Control System (RCS) Front-End
 *	   (I admit it; this is a crock!)
 *   (What do you expect for a few days work?)
 *
 * Kevin G. Wallace
 * Data Systems Division
 * Hewlett-Packard Company
 * Cupertino, CA 95014
 */

#include <stdio.h>
#ifdef	hpux 
#include <string.h>
#include <sys/types.h>
#ifdef hp9000s200
#include <fcntl.h>
#else 
#include <sys/fcntl.h>
#endif
#define index		strchr
#define rindex		strrchr
#define dup2(old,new)	close(new); dup(old)
#else  
#include <strings.h>
#endif 
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/dir.h>

#define	PGMNAM		"rmtrcs"
#define	RMTDIRPREFIX	"/tmp/rmtdir"
#define	LINKDIR		"RMTRCS"

#define	CAT	"/bin/cat"
#define	CD	"cd"
#define	CHMOD	"/bin/chmod"
#ifdef	ACI
/*
 * Both, VAX and HPUX systems will have it in the same place.
 */
#undef	ACI
#define	ACI	"/mnt/azure/bin/aci"
#endif

#ifdef	KCI
/*
 * The following ci is used for kernel sources only so that
 * we can control them a bit better.  If you are checking in
 * a kernel source file, this executable is expected to be on
 * the machine containing the ,v files whether it is a VAX
 * or an HPUX system.
 */
#undef	KCI
#define	KCI	"/mnt/azure/bin/ci"
#define	KDIR	"/mnt/azure/root.port/sys.rcs"
#ifdef	HPUX
#define	KHOST	"hpisoa1"
#endif
#ifdef	VAX
#define	KHOST	"hpdsa"
#endif
#endif

#ifdef	HPUX
#define	CI	"/usr/bin/ci"
#define	CO	"/usr/bin/co"
#endif
#ifdef	VAX
#define	CI	"/usr/new/ci"
#define	CO	"/usr/new/co"
#endif

#ifdef	HPUX
#define	ECHO	"/bin/echo"
#endif

#ifdef	VAX
#define	LN	"/bin/ln"
#endif

#define	LOCKS	"/mnt/azure/bin/locks"
#define	MARKBR	"/mnt/azure/bin/markbr"
#define	MARKREV	"/mnt/azure/bin/markrev"
#define	MKDIR	"/bin/mkdir"
#define	RM	"/bin/rm"

#ifdef	HPUX
#define	RCS	"/usr/bin/rcs"
#endif
#ifdef	VAX
#define	RCS	"/usr/new/rcs"
#endif

#define	RCSCHECK	"/mnt/azure/bin/rcscheck"

#ifdef	HPUX
#define	RCSDIFF	"/usr/bin/rcsdiff"
#define	RLOG	"/usr/bin/rlog"
#endif
#ifdef	VAX
#define	RCSDIFF	"/usr/new/rcsdiff"
#define	RLOG	"/usr/new/rlog"
#endif

#ifdef	HPUX
#define	RCP	"/usr/bin/rcp"
#endif
#ifdef	VAX
#define	RCP	"/usr/ucb/rcp"
#endif

#ifdef hpux
#define	RSH	"/usr/bin/remsh"
#endif
#ifdef	vax
#define	RSH	"/usr/ucb/rsh"
#endif

/*
 * The following ifdef is used for projects at CND.  To avoid having
 * to set up an account for everyone at CND, simply route all accounts through
 * the account below, which happens to be named after the engineer that 
 * originally started all this for the NFS/300 and NFS/800 code sharing.
 */
#ifdef CND
#define CNDACCOUNT "cmahon"
#endif CND

#define	FILENAMESIZE	2048
#define	HOSTNAMESIZE	256
#define	LOGMSGSIZE	2048
#define	MAXFILES	1024
#define	AVSIZE		(MAXFILES+100)

#define	error(str)	fprintf(stderr, "%s: %s", PGMNAM, str)
#define	errorn(str)	fprintf(stderr, "%s: %s\n", PGMNAM, str)
#define	eprintf(fmt, str)	fprintf(stderr, "%s: ", PGMNAM), \
	fprintf(stderr, fmt, str)
#define	eprintfn(fmt, str)	fprintf(stderr, "%s: ", PGMNAM), \
	fprintf(stderr, fmt, str), fprintf(stderr, "\n")


#ifndef	lint
#endif

char *rmtav[AVSIZE];		/* Null Terminated Copy of Argv */
char **rmtargv;			/* New Argv for Remote Command */
char rmtdir[FILENAMESIZE]	/* Temporary Remote Directory */
	= RMTDIRPREFIX;
char rmtdestfile[FILENAMESIZE];	/* Temporary Remote File */
char rmtsrcfile[FILENAMESIZE];	/* Source File */
char rmtvfile[FILENAMESIZE];	/* ,v File */
char backfile[FILENAMESIZE];	/* Backup File */
char uid[10];			/* Unique Id */	
char wd[FILENAMESIZE];		/* Current Directory */
char *cwd;			/* Pointer to Current Directory */
char hostname[HOSTNAMESIZE];	/* Current Host */
char rhost[HOSTNAMESIZE];	/* Remote Host */
char rdir[FILENAMESIZE];
char *rcd = rdir;		/* Remote Connect Directory */
char *rcmd;			/* Remote RCS Command */
char **localargv;			/* Pointer to Remote RCS Command */
char *filenames[MAXFILES];	/* List of filenames */
char logmsg[LOGMSGSIZE];	/* RCI Log Message */

char *stdinp, *stdoutp;		/* Filenames or strings for */
int  stdinsize;			/*  Standard Input and Output */
char **filenameptr;		/* Pointer to first Filename Pointer */
int  files;			/* Number of filenames */
int  logmsgsize;		/* RCI Log Message Size */


/* Flags */
char cflag[80];			/* RCS c Flag */
int  lflag;			/* RCS l Flag */
char mflag[LOGMSGSIZE];		/* RCS m Flag */
int  nflag;			/* RCS n Flag */
int  pflag;			/* RCS p Flag */
int  qflag;			/* RCS q Flag */
int  rflag;			/* RCS r Flag */
int  tflag;			/* RCS t Flag */
int  uflag;			/* RCS u Flag */
int  vflag;			/* RCS v Flag */
#if	defined(ACI) || defined(KCI)
char xflag[FILENAMESIZE] = "";	/* oldlogidflag for aci */
int  yflag;			/* newlogidflag for aci */
#endif
int  rmtrcsnflag;		/* Verbose But Don't Execute Flag */
int  rmtrcsvflag;		/* Verbose Flag */

/* Internal Functions */
#if	defined(ACI) || defined(KCI)
int raci();
#endif

int rci(), rco(), rrcs(), rrcsdiff(), rrlog(), rrcscheck(), rlocks();
int rmarkbr(), rmarkrev();

char *gets();

struct {
	char *r_name;
	int (*r_func)();
} rcmds[] = {
#ifdef	ACI
	"aci", raci,
#endif
#ifdef	KCI
	"kci", raci,
#endif
	"ci", rci,
	"co", rco,
	"locks", rlocks,
	"markbr", rmarkbr,
	"markrev", rmarkrev,
	"rcs", rrcs,
	"rcscheck", rrcscheck,
	"rcsdiff", rrcsdiff,
	"rlog", rrlog,
};

#define	NRCMDS	(sizeof(rcmds)/sizeof(rcmds[0]))

#define	STDIN	0
#define	STDOUT	1
#define	STDERR	2

#define	READ	0
#define	WRITE	1

#define	F_REDIRECT_STDOUT	0x1
#define	F_REDIRECT_STDIN	0x2
#define	F_CLOSE_STDIN		0x4
#define	F_PIPE_TO_STDIN		0x8

main(argc, argv)
	char *argv[];
{
	int i;

	parseargs(argc, argv);
	execlocally();
	getstrings();
	for (i = 0; i < NRCMDS; i++)
		if (!strcmp(rcmd, rcmds[i].r_name))
			exit((*rcmds[i].r_func)());
	eprintfn("%s: Command not found.", rcmd);
	exit(1);
}

/*
 * Remote command is the first non-hyphen argument after argv[0].
 * Filenames are all non-hyphen arguments thereafter.
 */
parseargs(argc, argv)
	char *argv[];
{
	int i;

	for (i = 1; i < argc; i++) {
		rmtav[i] = argv[i];
		if (argv[i][0] == '-')
			switch (argv[i][1]) {
			case 'c':
				quote(cflag, argv[i]);
				rmtav[i] = cflag;
				break;

			case 'm':
				quote(mflag, argv[i]);
				rmtav[i] = mflag;
				break;

			case 'l':
				lflag++;
				break;
			case 'p':
				pflag++;
				break;
			case 'n':
				if (rcmd)
					nflag++;
				else
					rmtrcsnflag++;
				break;
			case 'q':
				qflag++;
				break;
			case 'r':
				rflag++;
				break;
			case 't':
				tflag++;
				break;
			case 'u':
				uflag++;
				break;
			case 'v':
				if (rcmd)
					vflag++;
				else
					rmtrcsvflag++;
				break;
#if	defined(ACI) || defined(KCI)
			case 'x':
				quote(xflag, argv[i]);
				rmtav[i] = xflag;
				break;
			case 'y':
				yflag++;
				break;
#endif
			}
		else if (!rcmd) {
			rcmd = argv[i];
			localargv = &argv[i];
			rmtargv = &rmtav[i];
		} else {
			filenames[files++] = argv[i];
			if (files == 1)
				filenameptr = &rmtav[i];
		}
	}
	if (!rcmd) {
		fprintf(stderr, "usage: %s [-n] [-v] rcscmd [rcsoptions] file ...\n",
			PGMNAM);
		exit(1);
	}
#ifdef	ACI
	if (!yflag &&
	    xflag[0] == '\0' &&
	    (!strcmp(rcmd, "aci"))) {
		fprintf(stderr, "%s: aci needs -x or -y\n", PGMNAM);
		exit(1);
	}
#endif
#ifdef	KCI
#ifdef	notdef
	if (!yflag &&
	    xflag[0] == '\0' &&
	    (!strcmp(rcmd, "kci"))) {
		fprintf(stderr, "%s: kci needs -x or -y\n", PGMNAM);
		exit(1);
	}
#endif
#endif


	/* Remote Argv Must Terminate With a Null */
	rmtav[++i] = (char *)0;
}

quote(to, from)
	char *to, *from;
{
	*to++ = '\'';
	while (*to++ = *from++)
		;
	*(to-1) = '\'';
	*to = '\0';
}

/*
 * Execute the command locally if
 *   (1) there is no LINKDIR file
 *   (2) it is not a symbolic link (normal file on )
 *   (3) symbolic link cannot be read (normal file on )
 *   (4) link is not to a remote host
 * Otherwise return rhost and rdir
 * NOTE: on hpux a normal file is READ for the remote host directory.
 * Format is host:path-name in the first line.
 */
execlocally()
{
	char *colon;
	char linkname[FILENAMESIZE];
	struct stat status;

#ifdef	hpux 
	if ((stat(LINKDIR, &status) == -1)
	    || ((status.st_mode&S_IFMT) != S_IFREG)
	    || (readrdir(LINKDIR, linkname, FILENAMESIZE) == -1)
#else  
	if ((lstat(LINKDIR, &status) == -1)
	    || ((status.st_mode&S_IFMT) != S_IFLNK)
	    || (readlink(LINKDIR, linkname, FILENAMESIZE) == -1)
#endif 
	    || ((colon = index(linkname, ':')) == (char *)0)) {
		execvp(localargv[0], localargv);
		errorn("execvp failed");
		exit(1);
	}
	*colon++ = '\0';
	(void)strcpy(rhost, linkname);
	(void)strcpy(rdir, colon);
#ifdef	KCI
	if ((!strcmp(&(rcmd[strlen(rcmd)-2]), "ci")) &&
	    (!strcmp(rhost, KHOST)) &&
	    (!strncmp(rdir, KDIR, strlen(KDIR)))) {
		rcmd = "kci";		/* force kci in this case */
		fprintf(stderr, "%s: forcing kci...\n", PGMNAM);
		/*
		 * We need to check again for the -x and -y flags
		 * because the user could have used vanilla ci as
		 * the rcmd, in which case, we would not have
		 * checked for it.  Oh well...
		 */
#ifdef	notdef
		if (!yflag && xflag[0] == '\0') {
			fprintf(stderr, "%s: kci needs -x or -y\n", PGMNAM);
			exit(1);
		}
#endif
	}
#endif
}

#ifdef	hpux 
int readrdir (file, s, n)
char	*file, *s;
int	n;
{
	FILE *fd;
	char *newln;

	if ((fd = fopen (file, "r")) == NULL) 
		return (-1);
	
	fgets (s, n, fd);
        if ((newln = index (s, '\n')) != (char *)0)
		*newln = '\0';

	fclose (fd);
	return (0);
}
#endif 

/*
 * Get All Strings We Could Possibly Need
 */
getstrings()
{
#ifdef	hpux 
	extern char	*getcwd();
#endif 

	/* Get Unique Id */
	(void)sprintf(uid, "%d", getuid());

	/* Get Temporary Directory */
	(void)strcpy(&rmtdir[sizeof(RMTDIRPREFIX)-1], uid);

	/* Get Current Directory and Hostname */
#ifdef	hpux 
	cwd = getcwd(wd, sizeof wd);
#else  
	(void)getwd(wd);
	cwd = wd;
#endif 
	(void)gethostname(hostname, sizeof(hostname));

	/* Get Default Filename Strings */
	getdynstrings(filenames[0]);
}

/*
 * Get All Strings That Depend on Current Filename
 */
getdynstrings(fp)
	char *fp;
{

	/* Get Temporary File */
	(void)strcpy(rmtsrcfile, hostname);
	(void)strcat(rmtsrcfile, ":");
	if (*fp == '/')
		(void)strcat(rmtsrcfile, fp);
	else {
		(void)strcat(rmtsrcfile, cwd);
		(void)strcat(rmtsrcfile, "/");
		(void)strcat(rmtsrcfile, fp);
	}

	/* Get Destination File */
	(void)strcpy(rmtdestfile, rmtdir);
	(void)strcat(rmtdestfile, "/");
	(void)strcat(rmtdestfile, fp);

	/* Get ,v File */
	(void)strcpy(rmtvfile, fp);
	(void)strcat(rmtvfile, ",v");

	/* Backup Filename */
	getbackupfilename(backfile, fp);
}

getbackupfilename(s, fp)
	char *s, *fp;
{
	int l, m, max = 0;
	struct direct *dp;
	DIR *dirp;

	(void)strcpy(s, fp);
#ifdef hpux
	s[13] = '\0';	/* Truncate to 12 characters, leave 2 chars
			   for '~<digit>' */
	(void)strcat(s, "~");
#else  hpux
	(void)strcat(s, ".RBK.");
#endif hpux
	l = strlen(s);
	dirp = opendir(".");
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
		if ((!strncmp(dp->d_name, s, l))
		    && ((m = atoi(&dp->d_name[l])) > max))
			max = m;
	closedir(dirp);
	sprintf(&s[l], "%d", max+1);
}

/*
 * Copy Argument List
 */
copyav(to, from)
	char *to[], *from[];
{
	while (*to++ = *from++)
		;
}

/*
 * Redirect Standard Input
 */
int
rstdin(fp)
	char *fp;
{
	if (rmtrcsnflag || rmtrcsvflag)
		eprintfn("redirecting standard input from \"%s\"", fp);
	if (!rmtrcsnflag) {
		(void)close(STDIN);
		if (open(fp, O_RDONLY, 0) != STDIN) {
			errorn("cannot redirect standard input");
			return(-1);
		}
	}
	return(0);
}

/*
 * Redirect Standard Output
 */
int
rstdout(fp)
	char *fp;
{
	if (rmtrcsnflag || rmtrcsvflag)
		eprintfn("redirecting standard output to \"%s\"", fp);
	if (!rmtrcsnflag) {
		(void)close(STDOUT);
		if (open(fp, O_RDWR|O_CREAT, 0666) != STDOUT) {
			errorn("cannot redirect standard output");
			return(-1);
		}
	}
	return(0);
}

/*
 * Close Standard Input
 */
cstdin()
{
	if (rmtrcsnflag || rmtrcsvflag)
		errorn("closing standard input");
	if (!rmtrcsnflag)
		(void)close(STDIN);
}

/*
 * Yes/No (Yes Default)
 */
int
yesno()
{
	char buf[80];

	(void)gets(buf);
	return(buf[0] == 'n' ? 0 : 1);
}

/*
 * No/Yes (No Default)
 */
int
noyes()
{
	char buf[80];

	(void)gets(buf);
	return(buf[0] == 'y' ? 1 : 0);
}

/*
 * Remote CO
 */
rco()
{
	struct stat status;
	int i, errs;

	if (pflag) {
		rmtargv[0] = CO;
		cstdin();
		(void)execrsh(rmtargv);
		return(1);
	}
	errs = 0;
	if (files == 0)
		return(errs);
	*(filenameptr+1) = (char *)0;
	for (i = 0; i < files; i++) {
		if ((stat(filenames[i], &status) == 0)
		    && (status.st_mode&0222)) {
			if (qflag) {
				eprintfn("co error: writable %s exists; checkout aborted.", filenames[i]);
				errs++;
				continue;
			}
			eprintf("writable %s exists; overwrite? [ny](n): ",
				filenames[i]);
			if (!noyes()) {
				errorn("co warning: checkout aborted.");
				errs++;
				continue;
			}
		}
		if (!rmtrcsnflag)
			(void)unlink(filenames[i]);
		stdoutp = filenames[i];
		rmtargv[-1] = CO;
		rmtargv[0] = "-p";
		*filenameptr = filenames[i];

		if ((runrsh(&rmtargv[-1], F_REDIRECT_STDOUT) == -1)
		    || (stat(filenames[i], &status) == -1)
		    || (status.st_size == 0)) {
			if (!rmtrcsnflag)
				(void)unlink(filenames[i]);
			errs++;
			continue;
		}
		if (!lflag)
			(void)chmod(filenames[i], 0444);
		fprintf(stderr, "done\n");
	}
	return(errs);
}

/*
 * Remote RCS
 */
rrcs()
{
	cstdin();
	rmtargv[0] = RCS;
	(void)execrsh(rmtargv);
	return(1);
}

/*
 * Remote RLOG
 */
rrlog()
{
#ifndef	LANBUG
	cstdin();
#endif
	rmtargv[0] = RLOG;
	(void)execrsh(rmtargv);
	return(1);
}

/*
 * Remote RCSDIFF
 */
rrcsdiff()
{
	char *av[AVSIZE], **avp;
	int i, j, errs;

	if (rflag > 1) {
		cstdin();
		rmtargv[0] = RCSDIFF;
		(void)execrsh(rmtargv);
		return(1);
	}
	if (files == 1) {
		if (rstdin(filenames[0]) == -1)
			return(1);
		avp = av;
		*avp++ = MKDIR;
		*avp++ = rmtdir;
		*avp++ = ";";
		*avp++ = CAT;
		*avp++ = ">";
		*avp++ = rmtdestfile;
		*avp++ = ";";
		*avp++ = RCSDIFF;
		for (i = 1; (rmtargv[i] != (char *)0)
		    && (rmtargv[i] != filenames[0]); i++){
			*avp++ = rmtargv[i];
		}
		*avp++ = rmtvfile;
		*avp++ = rmtdestfile;
		*avp++ = ";";
		*avp++ = RM;
		*avp++ = "-r";
		*avp++ = rmtdir;
		*avp++ = (char *)0;
		(void)execrsh(av);
		return(1);
	}
	/*
	 * Multiple Filenames, One or No Revsion Specified
	 */
	errs = 0;
	for (i = 0; i < files; i++) {
		getdynstrings(filenames[i]);
		avp = av;
		if (i == 0) {
			*avp++ = MKDIR;
			*avp++ = rmtdir;
			*avp++ = ";";
		}
		*avp++ = CAT;
		*avp++ = ">";
		*avp++ = rmtdestfile;
		*avp++ = ";";
		*avp++ = RCSDIFF;
		for (j = 1; (rmtargv[j] != (char *)0)
		    && (rmtargv[j] != filenames[0]); j++){
			*avp++ = rmtargv[j];
		}
		*avp++ = rmtvfile;
		*avp++ = rmtdestfile;
		if (i == files-1) {
			*avp++ = ";";
			*avp++ = RM;
			*avp++ = "-r";
			*avp++ = rmtdir;
		}
		*avp++ = (char *)0;
		stdinp = filenames[i];
		if (runrsh(av, F_REDIRECT_STDIN) == -1)
			errs++;
	}
	return(errs);
}

void getlogmsg(i)
{
	char *ep = logmsg;
	int newline = 1;

	if (i != 0) {
		fprintf(stderr, "reuse log message of previous file? [yn](y): ");
		if (yesno())
			return;
	}
	fprintf(stderr, "enter log message (for %s):\n(terminate with ^D or single '.')\n>> ",
		filenames[i]);
	while (gets(ep) != NULL) {
		if (*ep == '.') {
			*ep = '\0';
			newline = 0;
			break;
		}
		while (*ep)
			ep++;
		*ep++ = '\n';
		*ep = '\0';
		fprintf(stderr, ">> ");
	}
	logmsgsize = ep-logmsg;
	if (newline)
		(void)putc('\n', stderr);
}

#if	defined(ACI) || defined(KCI)
/* 
 * Remote ACI
 */

int
raci()
{
	char *av[AVSIZE], **avp, pflag[32];
	struct stat status;
	int i, j, errs, flags;

	if (tflag) {
		errorn("ci: cannot handle -t yet");
		return(1);
	}
	errs = 0;
	for (i = 0; i < files; i++) {
		getdynstrings(filenames[i]);
		avp = av;
		if (i == 0) {
			*avp++ = MKDIR;
			*avp++ = rmtdir;
			*avp++ = ";";
		}
		if (*mflag) {
			*avp++ = CAT;
			*avp++ = ">";
			*avp++ = rmtdestfile;
		} else {
			*avp++ = RCP;
			*avp++ = rmtsrcfile;
			*avp++ = rmtdestfile;
		}
		*avp++ = ";";
#ifdef	KCI
		if (!strcmp(rcmd, "kci"))
			*avp++ = KCI;
#ifdef	ACI
		else
#endif
#endif
#ifdef	ACI
			*avp++ = ACI;
#endif

		if ((xflag[0] != '\0') || (i == 0))
			for (j = 1; (rmtargv[j] != (char *)0)
			    && (rmtargv[j] != filenames[0]); j++){
				*avp++ = rmtargv[j];
			}
		else {	/* add -x, delete -y after first one */
			*avp++ = "-x";
			for (j = 1; (rmtargv[j] != (char *)0)
			    && (rmtargv[j] != filenames[0]); j++)
				if (rmtargv[j][1] != 'y')
					*avp++ = rmtargv[j];
		}

		*avp++ = rmtvfile;
		*avp++ = rmtdestfile;

		if (i == files-1) {
			*avp++ = ";";
			*avp++ = RM;
			*avp++ = "-r";
			*avp++ = rmtdir;
		}
		*avp++ = (char *)0;
		if (*mflag) {
			stdinp = filenames[i];
			flags = F_REDIRECT_STDIN;
		} else {
			getlogmsg(i);
			stdinp = logmsg;
			stdinsize = logmsgsize;
			flags = F_PIPE_TO_STDIN;
		}
		if (runrsh(av, flags) == -1) {
			errs++;
			continue;
		}
		/*
		 * Make backup-file because don't know
		 * if ci succeeded.
		 */
		if (!rmtrcsnflag && rename(filenames[i], backfile) == -1) {
			eprintfn("ci: cannot create back-up file \"%s\"",
				backfile);
			errs++;
			continue;
		}
		if (lflag || uflag) {
			avp = av;
			*avp++ = CO;
			(void)strcpy(pflag, "-q -p");
			for (j = 1; (rmtargv[j] != (char *)0)
			    && (rmtargv[j] != filenames[0]); j++) {
				if ((rmtargv[j][0] == '-')
				    && ((rmtargv[j][1] == 'u')
				     || (rmtargv[j][1] == 'l'))) {
					if (rmtargv[j][2] != '\0')
					(void)strcat(pflag, &rmtargv[j][2]);
					*avp++ = pflag;
				} else if ((rmtargv[j] != mflag)
				    && (rmtargv[j] != xflag)
				    && ((rmtargv[j][0] != '-')
					    || ((rmtargv[j][1] != 'f')
					        && (rmtargv[j][1] != 'y'))))
					    *avp++ = rmtargv[j];
			}
			*avp++ = filenames[i];
			*avp++ = (char *)0;
			stdoutp = filenames[i];
			if ((runrsh(av, F_REDIRECT_STDOUT) == -1)
			    || (stat(filenames[i], &status) == -1)
			    || (status.st_size == 0)) {
				if (!rmtrcsnflag)
					(void)unlink(filenames[i]);
				errs++;
				continue;
			}
			if (!lflag)
				(void)chmod(filenames[i], 0444);

		}
	}
	return(errs);
}
#endif

/* 
 * Remote CI
 */

int
rci()
{
	char *av[AVSIZE], **avp, pflag[32];
	struct stat status;
	int i, j, errs, flags;

	if (tflag) {
		errorn("ci: cannot handle -t yet");
		return(1);
	}
	errs = 0;
	for (i = 0; i < files; i++) {
		getdynstrings(filenames[i]);
		avp = av;
		if (i == 0) {
			*avp++ = MKDIR;
			*avp++ = rmtdir;
			*avp++ = ";";
		}
		if (*mflag) {
			*avp++ = CAT;
			*avp++ = ">";
			*avp++ = rmtdestfile;
		} else {
			*avp++ = RCP;
			*avp++ = rmtsrcfile;
			*avp++ = rmtdestfile;
		}
		*avp++ = ";";
		*avp++ = CI;

		for (j = 1; (rmtargv[j] != (char *)0)
		    && (rmtargv[j] != filenames[0]); j++) {
			*avp++ = rmtargv[j];
		}

		*avp++ = rmtvfile;
		*avp++ = rmtdestfile;

		if (i == files-1) {
			*avp++ = ";";
			*avp++ = RM;
			*avp++ = "-r";
			*avp++ = rmtdir;
		}
		*avp++ = (char *)0;
		if (*mflag) {
			stdinp = filenames[i];
			flags = F_REDIRECT_STDIN;
		} else {
			getlogmsg(i);
			stdinp = logmsg;
			stdinsize = logmsgsize;
			flags = F_PIPE_TO_STDIN;
		}
		if (runrsh(av, flags) == -1) {
			errs++;
			continue;
		}
		/*
		 * Make backup-file because don't know
		 * if ci succeeded.
		 */
		if (!rmtrcsnflag && rename(filenames[i], backfile) == -1) {
			eprintfn("ci: cannot create back-up file \"%s\"",
				backfile);
			errs++;
			continue;
		}
		if (lflag || uflag) {
			avp = av;
			*avp++ = CO;
			(void)strcpy(pflag, "-q -p");
			for (j = 1; (rmtargv[j] != (char *)0)
			    && (rmtargv[j] != filenames[0]); j++) {
				if ((rmtargv[j][0] == '-')
				    && ((rmtargv[j][1] == 'u')
				     || (rmtargv[j][1] == 'l'))) {
					if (rmtargv[j][2] != '\0')
					(void)strcat(pflag, &rmtargv[j][2]);
					*avp++ = pflag;
				} else if ((rmtargv[j] != mflag)
				    && ((rmtargv[j][0] != '-')
					|| (rmtargv[j][1] != 'f')))
					    *avp++ = rmtargv[j];
			}
			*avp++ = filenames[i];
			*avp++ = (char *)0;
			stdoutp = filenames[i];
			if ((runrsh(av, F_REDIRECT_STDOUT) == -1)
			    || (stat(filenames[i], &status) == -1)
			    || (status.st_size == 0)) {
				if (!rmtrcsnflag)
					(void)unlink(filenames[i]);
				errs++;
				continue;
			}
			if (!lflag)
				(void)chmod(filenames[i], 0444);

		}
	}
	return(errs);
}

/*
 * Remote RCSCHECK
 */
int
rrcscheck()
{
	char *av[AVSIZE], **avp;
	struct stat status;
	int i, j, errs;

	errs = 0;
	rcd = (char *)0;
	for (i = 0; i < files; i++) {
		getdynstrings(filenames[i]);
		avp = av;
		if (i == 0) {
			*avp++ = MKDIR;
			*avp++ = rmtdir;
			*avp++ = ";";
		}
		*avp++ = CD;
		*avp++ = rmtdir;
		*avp++ = ";";
		if (i == 0) {
#ifdef	HPUX
			*avp++ = ECHO;
			*avp++ = rdir;
			*avp++ = ">";
			*avp++ = "RCS";
			*avp++ = ";";
#endif
#ifdef	VAX
			*avp++ = LN;
			*avp++ = "-s";
			*avp++ = rdir;
			*avp++ = "RCS";
			*avp++ = ";";
#endif
		}
		*avp++ = CAT;
		*avp++ = ">";
		*avp++ = rmtdestfile;
		*avp++ = ";";
		if ((stat(filenames[i], &status) == 0)
		    && !(status.st_mode&0222)) {
			*avp++ = CHMOD;
			*avp++ = "-w";
			*avp++ = rmtdestfile;
			*avp++ = ";";
		}
		*avp++ = RCSCHECK;
		for (j = 1; (rmtargv[j] != (char *)0)
		    && (rmtargv[j] != filenames[0]); j++){
			*avp++ = rmtargv[j];
		}
		*avp++ = filenames[i];
		if (i == files-1) {
			*avp++ = ";";
			*avp++ = CD;
			*avp++ = ";";
			*avp++ = RM;
			*avp++ = "-r";
			*avp++ = rmtdir;
		}
		*avp++ = (char *)0;
		stdinp = filenames[i];
		if (runrsh(av, F_REDIRECT_STDIN) == -1)
			errs++;
	}
	return(errs);
}

/*
 * Remote LOCKS
 */
int
rlocks()
{
	char *av[AVSIZE], **avp;
	int i;
	rcd = (char *)0;
	avp = av;
	*avp++ = MKDIR;
	*avp++ = rmtdir;
	*avp++ = ";";
	*avp++ = CD;
	*avp++ = rmtdir;
	*avp++ = ";";
#ifdef	HPUX
	*avp++ = ECHO;
	*avp++ = rdir;
	*avp++ = ">";
	*avp++ = "RCS";
	*avp++ = ";";
#endif
#ifdef	VAX
	*avp++ = LN;
	*avp++ = "-s";
	*avp++ = rdir;
	*avp++ = "RCS";
	*avp++ = ";";
#endif
	*avp++ = LOCKS;
	for (i = 1; rmtargv[i]; i++)
		*avp++ = rmtargv[i]; 	/* you normally just say "." */
	*avp++ = ";";
	*avp++ = CD;
	*avp++ = ";";
	*avp++ = RM;
	*avp++ = "-r";
	*avp++ = rmtdir;
	*avp++ = (char *)0;
	(void)execrsh(av);
	return(1);
}

/*
 * Remote MARKBR
 */
int
rmarkbr()
{
	char *av[AVSIZE], **avp;
	int i, j, errs;

	errs = 0;
#ifndef	HPUX
	rcd = (char *)0;
	for (i = 0; i < files; i++) {
		getdynstrings(filenames[i]);
		avp = av;
		if (i == 0) {
			*avp++ = MKDIR;
			*avp++ = rmtdir;
			*avp++ = ";";
		}
		*avp++ = CD;
		*avp++ = rmtdir;
		*avp++ = ";";
		if (i == 0) {
			*avp++ = LN;
			*avp++ = "-s";
			*avp++ = rdir;
			*avp++ = "RCS";
			*avp++ = ";";
		}
		*avp++ = CAT;
		*avp++ = ">";
		*avp++ = rmtdestfile;
		*avp++ = ";";
		*avp++ = MARKBR;
		for (j = 1; (rmtargv[j] != (char *)0)
		    && (rmtargv[j] != filenames[0]); j++){
			*avp++ = rmtargv[j];
		}
		*avp++ = filenames[i];
		if (i == files-1) {
			*avp++ = ";";
			*avp++ = CD;
			*avp++ = ";";
			*avp++ = RM;
			*avp++ = "-r";
			*avp++ = rmtdir;
		}
		*avp++ = (char *)0;
		stdinp = filenames[i];
		if (runrsh(av, F_REDIRECT_STDIN) == -1)
			errs++;
	}
#else
	eprintfn("%s: not supported, yet....", rcmd);	
#endif
	return(errs);
}

/*
 * Remote MARKREV
 */
int
rmarkrev()
{
	char *av[AVSIZE], **avp;
	int i, j, errs;

	errs = 0;
#ifndef	HPUX
	rcd = (char *)0;
	for (i = 0; i < files; i++) {
		getdynstrings(filenames[i]);
		avp = av;
		if (i == 0) {
			*avp++ = MKDIR;
			*avp++ = rmtdir;
			*avp++ = ";";
		}
		*avp++ = CD;
		*avp++ = rmtdir;
		*avp++ = ";";
		if (i == 0) {
			*avp++ = LN;
			*avp++ = "-s";
			*avp++ = rdir;
			*avp++ = "RCS";
			*avp++ = ";";
		}
		*avp++ = CAT;
		*avp++ = ">";
		*avp++ = rmtdestfile;
		*avp++ = ";";
		*avp++ = MARKREV;
		for (j = 1; (rmtargv[j] != (char *)0)
		    && (rmtargv[j] != filenames[0]); j++){
			*avp++ = rmtargv[j];
		}
		*avp++ = filenames[i];
		if (i == files-1) {
			*avp++ = ";";
			*avp++ = CD;
			*avp++ = ";";
			*avp++ = RM;
			*avp++ = "-r";
			*avp++ = rmtdir;
		}
		*avp++ = (char *)0;
		stdinp = filenames[i];
		if (runrsh(av, F_REDIRECT_STDIN) == -1)
			errs++;
	}
#else
	eprintfn("%s: not supported, yet....", rcmd);	
#endif
	return(errs);
}

/*
 * Exec RSH
 *
 */
execrsh(argv)
	char *argv[];
{
	char *av[AVSIZE], **avp = av;

	*avp++ = RSH;
	*avp++ = rhost;
#ifdef CND
	*avp++ = "-l";
	*avp++ = CNDACCOUNT;
#endif CND
	if (rcd) {
		*avp++ = CD;
		*avp++ = rcd;
		*avp++ = ";";
	}
	copyav(avp, argv);
	if (rmtrcsnflag || rmtrcsvflag) {
		fprintf(stderr, "%s: execl(\"%s\"", PGMNAM, RSH);
		for (avp = av; *avp;)
			fprintf(stderr, ", \"%s\"", *avp++);
		fprintf(stderr, ");\n");
	}
	if (!rmtrcsnflag) {
		execv(RSH, av);
		errorn("execv failure");
		return(-1);
	} else
		return(0);
}

int
runrsh(argv, flags)
	char *argv[];
{
	union wait status;
	int fildes[2];

	if (rmtrcsnflag || rmtrcsvflag) {
		errorn("forking a child");
	}
	if (flags & F_PIPE_TO_STDIN) {
		if (rmtrcsnflag || rmtrcsvflag)
			eprintfn("piping \"\n%s\" to child's standard input",
				stdinp);

		if ((!rmtrcsnflag) && (pipe(fildes) == -1)) {
			errorn("cannot make pipe");
			return(-1);
		}
	}
	if (!fork()) {
		if ((flags & F_REDIRECT_STDIN) && (rstdin(stdinp) != 0))
			exit(1);
		if ((flags & F_REDIRECT_STDOUT) && (rstdout(stdoutp) != 0))
			exit(1);
		if (flags & F_CLOSE_STDIN)
			cstdin();
		if ((!rmtrcsnflag) && (flags & F_PIPE_TO_STDIN)) {
			(void)close(fildes[WRITE]);
			(void)dup2(fildes[READ],STDIN);
			(void)close(fildes[READ]);
		}
		(void)execrsh(argv);
		if (rmtrcsnflag)
			exit(0);
		else
			exit(1);
	}
	if ((!rmtrcsnflag) && (flags & F_PIPE_TO_STDIN)) {
		(void)close(fildes[READ]);
		(void)write(fildes[WRITE], stdinp, stdinsize);
		(void)close(fildes[WRITE]);
	}
	if ((wait(&status) == -1) || (status.w_termsig != 0)
	    || (status.w_retcode != 0))
		return(-1);
	else
		return(0);
}

#ifdef	notyet
handler(sig, code, scp)
	struct sigcontext *scp;
{
	char *av[AVSIZE], **avp;

	if (cleandir)

	errorn("cleaning up");
	rcd = (char *)0;
	*avp++ = RM;
	*avp++ = "-r";
	*avp++ = rmtdir;
	*avp++ = (char *)0;
}
#endif

#ifdef	hpux 
rename (from, to)
char 	*from, *to;
{
	unlink(to);
	if (link (from, to) != 0) return (-1);
	if (unlink(from) != 0) return (-1);
	return(0);
}
#endif 
