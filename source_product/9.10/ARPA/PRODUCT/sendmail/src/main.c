/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# ifndef lint
char copyright[] =
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
# endif /* not lint */

# ifndef lint
static char rcsid[] = "$Header: main.c,v 1.41.109.11 95/03/22 17:30:32 mike Exp $";
# 	ifndef hpux
static char sccsid[] = "@(#)main.c	5.31 (Berkeley) 7/20/90";
# 	endif	/* not hpux */
# endif /* not lint */
# ifdef PATCH_STRING
static char *patch_5384="@(#) PATCH_9.X: main.o $Revision: 1.41.109.11 $ 94/03/24 PHNE_5384";
# endif	/* PATCH_STRING */

# define	_DEFINE

# include <sys/file.h>
# include <signal.h>
# include <sys/ioctl.h>
# include <sys/errno.h>
# include <sys/stat.h>
# include <fcntl.h>
# include "sendmail.h"
# include <arpa/nameser.h>
# include <resolv.h>

# ifdef lint
char	edata, end;
# endif /* lint */

static void initmacros();
static void freeze();

/*
**  SENDMAIL -- Post mail to a set of destinations.
**
**	This is the basic mail router.  All user mail programs should
**	call this routine to actually deliver mail.  Sendmail in
**	turn calls a bunch of mail servers that do the real work of
**	delivering the mail.
**
**	Sendmail is driven by tables read in from /usr/lib/sendmail.cf
**	(read by readcf.c).  Some more static configuration info,
**	including some code that you may want to tailor for your
**	installation, is in conf.c.  You may also want to touch
**	daemon.c (if you have some other IPC mechanism), acct.c
**	(to change your accounting), names.c (to adjust the name
**	server mechanism).
**
**	Usage:
**		/usr/lib/sendmail [flags] addr ...
**
**		See the associated documentation for details.
**
**	Author:
**		Eric Allman, UCB/INGRES (until 10/81)
**			     Britton-Lee, Inc., purveyors of fine
**				database computers (from 11/81)
**		The support of the INGRES Project and Britton-Lee is
**			gratefully acknowledged.  Britton-Lee in
**			particular had absolutely nothing to gain from
**			my involvement in this project.
*/


int		NextMailer;	/* "free" index into Mailer struct */
char		*FullName;	/* sender's full name */
ENVELOPE	BlankEnvelope;	/* a "blank" envelope */
ENVELOPE	MainEnvelope;	/* the envelope around the basic letter */
ADDRESS		NullAddress =	/* a null address */
		{ "", "", NULL, "" };

/*
**  Pointers for setproctitle.
**	This allows "ps" listings to give more useful information.
**	These must be kept out of BSS for frozen configuration files
**		to work.
*/

# ifdef SETPROCTITLE
# 	ifdef PSTAT
char		*Args;	/* original argument vector as string */
# 	else	/* ! PSTAT */
char		**Argv = NULL;		/* pointer to argument vector */
char		*LastArgv = NULL;	/* end of argv */
# 	endif /* PSTAT */
# endif /* SETPROCTITLE */

# ifdef DAEMON
# 	ifndef SMTP
ERROR %%%%   Cannot have daemon mode without SMTP   %%%% ERROR
# 	endif /* SMTP */
# endif /* DAEMON */

main(argc, argv, envp)
	int argc;
	char **argv;
	char **envp;
{
	register char *p;
	char **av;
	extern char Version[];
	char *from;
	typedef int (*fnptr)();
	STAB *st;
	uid_t euid;			/* effective uid for semaphore */
	gid_t egid = 0;			/* effective gid for semaphore */
	register int i;
	bool readconfig = TRUE;
	bool queuemode = FALSE;		/* process queue requests */
	bool nothaw;
	static bool reenter = FALSE;
	char jbuf[MAXDNAME];			/* holds MyHostName */
	int limit =0;
	extern bool safefile();
	extern time_t convtime();
	extern void intsig();
	extern char **myhostname();
	extern char *arpadate();
	extern char **environ;

	/*
	**  Check to see if we reentered.
	**	This would normally happen if e_putheader or e_putbody
	**	were NULL when invoked.
	*/

	if (reenter)
	{
		syserr("main: reentered!");
		abort();
	}
	reenter = TRUE;

# ifdef hpux
	/*
	** Clean environment, clear timers, close open files
	** Removing TZ (ala BSD) doesn't enforce use of local time on HP-UX.
	*/
	(void) cleanenv(&envp,
		"TZ", "LANG", "LANGOPTS", "NLSPATH",
		"LOCALDOMAIN", "HOSTALIASES",
		"HOME", 0);
# else	/* ! hpux */
	/* Enforce use of local time */
	putenv("TZ=");
# endif	/* hpux */

	/*
	**  Be sure we have enough file descriptors.
	**	But also be sure that 0, 1, & 2 are open.
	*/

	i = open("/dev/null", O_RDWR);
	while (i >= 0 && i < 2)
		i = dup(i);
	for (i = getnumfds(); i > 2; --i)
		(void) close(i);
	errno = 0;

# ifdef LOG_MAIL
	openlog("sendmail", LOG_PID, LOG_MAIL);
# else 	/* ! LOG_MAIL */
	openlog("sendmail", LOG_PID);
# endif 	/* LOG_MAIL */

	/* set up the blank envelope */
	BlankEnvelope.e_puthdr = putheader;
	BlankEnvelope.e_putbody = putbody;
	BlankEnvelope.e_xfp = NULL;
	STRUCTCOPY(NullAddress, BlankEnvelope.e_from);
	CurEnv = &BlankEnvelope;
	STRUCTCOPY(NullAddress, MainEnvelope.e_from);

	/*
	**  Set default values for variables.
	**	These cannot be in initialized data space.
	*/

	setdefaults(&BlankEnvelope);

	/*
	** Set the alias file variables to NULL.
	*/

	AliasFile = NULL;
	ReverseAliasFile = NULL;
	AliasDB = NULL;
	ReverseAliasDB = NULL;
	
	/*
	** Effective uid for semaphore ownership
	*/
	euid = geteuid();
	RealUid = getuid();
	RealGid = getgid();

	/*
	**  Do a quick prescan of the argument list.
	**	We do this to find out if we can potentially thaw the
	**	configuration file.  If not, we do the thaw now so that
	**	the argument processing applies to this run rather than
	**	to the run that froze the configuration.
	*/

	argv[argc] = NULL;
	av = argv;
	nothaw = FALSE;
	while ((p = *++av) != NULL)
	{
		if (strncmp(p, "-C", 2) == 0)
		{
			ConfFile = &p[2];
			if (ConfFile[0] == '\0')
				ConfFile = "sendmail.cf";
			(void) setgid(getrgid());
			(void) setuid(getruid());
			nothaw = TRUE;
		}
		else if (strncmp(p, "-bz", 3) == 0) {
			nothaw = TRUE;
		}
                else if (strncmp(p, "-Z", 2) == 0)
		{
			AltFreezeFile = TRUE;
			FreezeFile = &p[2];
			if (FreezeFile[0] == '\0')
				FreezeFile = "sendmail.fc";
			(void) setgid(getgid());
			(void) setuid(getuid());
		}
# ifdef DEBUG
		else if (strncmp(p, "-d", 2) == 0)
		{
			(void) setgid(getgid());
			(void) setuid(getuid());
			tTsetup(tTdvect, sizeof tTdvect, "0-99.1");
			tTflag(&p[2]);
			setbuf(stdout, (char *) NULL);
			printf("Version %s\n", Version);
		}
# endif /* DEBUG */
	}

	InChannel = stdin;
	OutChannel = stdout;

	/*
	**  WARNING:
	**  
	**  Do no mallocs before the following call to thaw.
	**  Malloc's data structures are part of the BSS.
	**  Beware of functions, macros, and library routines
	**  that call malloc!
	*/

	if (!nothaw)
		readconfig = !thaw(FreezeFile);

	/* reset the environment after the thaw */
	for (i = 0; i < MAXUSERENVIRON && envp[i] != NULL; i++)
		UserEnviron[i] = newstr(envp[i]);
	UserEnviron[i] = NULL;
	environ = UserEnviron;

# ifdef SETPROCTITLE
# 	ifdef PSTAT

	/*
	**  Save initial argument list for setproctitle.
	*/
	Args = (char *)xalloc(BUFSIZ);

	/* Only keep the basename of argv[0] */
	p = strrchr(*argv, '/');
	if (p++ == NULL)
		p = *argv;
	strcpy(Args, p);
	limit += strlen(p);
	strcat(Args, " ");
	limit++;
	/* Copy the rest of the args only up to BUFSIZ */
	for (i = 1;  i < argc; i++) {
	        limit += strlen(argv[i]);
		if (limit > (BUFSIZ -1))
		   break;
	 	strcat(Args, argv[i]);
		strcat(Args, " ");
		limit++;
	
	}
# 	else /* ! PSTAT */
	/*
	**  Save start and extent of argv for setproctitle.
	*/

	Argv = argv;
	if (i > 0)
		LastArgv = envp[i - 1] + strlen(envp[i - 1]);
	else
		LastArgv = argv[argc - 1] + strlen(argv[argc - 1]);
# 	endif /* PSTAT */
# endif /* SETPROCTITLE */

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, intsig);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, intsig);
	(void) signal(SIGTERM, intsig);
	(void) signal(SIGPIPE, SIG_IGN);
	OldUmask = umask(0);
	OpMode = MD_DELIVER;
	MotherPid = getpid();
	FullName = getenv("NAME");

	errno = 0;
	from = NULL;

	if (readconfig)
	{
		/* initialize some macros, etc. */
		initmacros();

		/* set default user for initgroups */
		setdefuser();

# if !defined(hpux)
		/* hostname */
		av = myhostname(jbuf, sizeof jbuf);
		if (jbuf[0] != '\0')
		{
# 	ifdef DEBUG
			if (tTd(0, 4))
				printf("canonical name: %s\n", jbuf);
# 	endif /* DEBUG */
			p = newstr(jbuf);
			define('w', p, CurEnv);
			setclass('w', p);
		}
		while (av != NULL && *av != NULL)
		{
# 	ifdef DEBUG
			if (tTd(0, 4))
				printf("\ta.k.a.: %s\n", *av);
# 	endif /* DEBUG */
			setclass('w', *av++);
		}
# endif /* !defined(hpux) */

		/* 
		**  Define hostname.
		**      If macro 'w' has already been defined in the frozen
		**      configuration or on the command line, use that
		**      definition.  Otherwise myhostname() will call
		**      gethostname().
		*/

		bzero(jbuf, sizeof jbuf);
		expand("\001w", jbuf, &jbuf[sizeof jbuf - 1], CurEnv);
		if (jbuf[0] == '\0')
		    definehostname(jbuf, CurEnv);

		/* UUCP hostname */
		bzero(jbuf, sizeof jbuf);
		(void) uucpname(jbuf);
		if (jbuf[0] != '\0')
		{
			p = newstr(jbuf);
			define('k', p, CurEnv);
		}
		bzero(jbuf, sizeof jbuf);

		/* version */
		define('v', Version, CurEnv);
	}

	/* current time */
	define('b', arpadate((char *) NULL), CurEnv);

	/*
	** Crack argv.
	*/

	av = argv;
	p = strrchr(*av, '/');
	if (p++ == NULL)
		p = *av;
	if (strcmp(p, "newaliases") == 0)
		OpMode = MD_INITALIAS;
	else if (strcmp(p, "mailq") == 0)
		OpMode = MD_PRINT;
	else if (strcmp(p, "smtpd") == 0)
# ifdef DAEMON
	{
		if (getuid() != 0) {
			usrerr("Permission denied");
			exit (EX_USAGE);
		}
		OpMode = MD_DAEMON;
		(void) putenv("HOSTALIASES=");
	}
# else /* not DAEMON */
	{
		usrerr("Daemon mode not implemented");
		exit(EX_USAGE);
	}
# endif /* not DAEMON */
	
	while ((p = *++av) != NULL && p[0] == '-')
	{
		switch (p[1])
		{
		  case 'b':	/* operations mode */
			switch (p[2])
			{
			  case MD_DAEMON:
# ifdef DAEMON
				if (getuid() != 0) {
					usrerr("Permission denied");
					exit (EX_USAGE);
				}
				(void) putenv("HOSTALIASES=");
# else	/* ! DAEMON */
				usrerr("Daemon mode not implemented");
				ExitStat = EX_USAGE;
				break;
# endif /* DAEMON */
			  case MD_SMTP:
# ifndef SMTP
				usrerr("I don't speak SMTP");
				ExitStat = EX_USAGE;
				break;
# endif /* SMTP */
				/* fall through */
			  case MD_KILL:
			  case MD_FREEZE:
			  case MD_INITALIAS:
			  case MD_ARPAFTP:
			  case MD_DELIVER:
			  case MD_VERIFY:
			  case MD_TEST:
			  case MD_PRINT:
				OpMode = p[2];
				break;

			  default:
				usrerr("Invalid operation mode %c", p[2]);
				ExitStat = EX_USAGE;
				break;
			}
			break;

		  case 'C':	/* select configuration file (already done) */
		  case 'Z':	/* select frozen config. file (already done) */
			break;

# ifdef DEBUG
		  case 'd':	/* debugging -- redo in case frozen */
			tTsetup(tTdvect, sizeof tTdvect, "0-99.1");
			tTflag(&p[2]);
			setbuf(stdout, (char *) NULL);
# 	ifdef NAMED_BIND
			_res.options |= RES_DEBUG;
# 	endif	/* NAMED_BIND */
			break;
# endif /* DEBUG */

		  case 'f':	/* from address */
		  case 'r':	/* obsolete -f flag */
			p += 2;
			if (*p == '\0' && ((p = *++av) == NULL || *p == '-'))
			{
				p = *++av;
				if (p == NULL || *p == '-')
				{
					usrerr("No \"from\" address");
					ExitStat = EX_USAGE;
					av--;
					break;
				}
			}
			if (from != NULL)
			{
				usrerr("More than one \"from\" address");
				ExitStat = EX_USAGE;
				break;
			}
			{
			char *cp = denlstring(p, TRUE, TRUE);
			from = newstr(cp);
			}
			break;

		  case 'F':	/* set full name */
			p += 2;
			if (*p == '\0' && ((p = *++av) == NULL || *p == '-'))
			{
				usrerr("Bad -F flag");
				ExitStat = EX_USAGE;
				av--;
				break;
			}
			{
			char *cp = denlstring(p, TRUE, TRUE);
			FullName = newstr(cp);
			}
			break;

		  case 'h':	/* hop count */
			p += 2;
			if (*p == '\0' && ((p = *++av) == NULL || !isdigit(*p)))
			{
				usrerr("Bad hop count (%s)", p);
				ExitStat = EX_USAGE;
				av--;
				break;
			}
			CurEnv->e_hopcount = atoi(p);
			break;
		
		  case 'n':	/* don't alias */
			NoAlias = TRUE;
			break;

		  case 'o':	/* set option */
			setoption(p[2], &p[3], FALSE, TRUE, CurEnv);
			break;

		  case 'q':	/* run queue files at intervals */
# ifdef QUEUE
			if (getuid() != 0) {
				usrerr("Permission denied");
				exit (EX_USAGE);
			}
			(void) putenv("HOSTALIASES=");
			queuemode = TRUE;
			QueueIntvl = convtime(&p[2]);
# else /* QUEUE */
			usrerr("I don't know about queues");
			ExitStat = EX_USAGE;
# endif /* QUEUE */
			break;

		  case 't':	/* read recipients from message */
			GrabTo = TRUE;
			break;

			/* compatibility flags */
		  case 'c':	/* connect to non-local mailers */
		  case 'e':	/* error message disposition */
		  case 'i':	/* don't let dot stop me */
		  case 'm':	/* send to me too */
		  case 'T':	/* set timeout interval */
		  case 'v':	/* give blow-by-blow description */
			setoption(p[1], &p[2], FALSE, TRUE, CurEnv);
			break;

		  case 's':	/* save From lines in headers */
			setoption('f', &p[2], FALSE, TRUE, CurEnv);
			break;

# ifdef NDBM
		  case 'I':	/* initialize alias DBM file */
			OpMode = MD_INITALIAS;
			break;
# endif /* NDBM */
		}
	}

	/*
	**  Do basic initialization.
	**	Read system control file.
	**	Extract special fields for local use.
	*/

	if (OpMode == MD_FREEZE || readconfig)
		readcf(ConfFile, CurEnv);

# ifdef QUEUE
	if (queuemode && RealUid != 0 && bitset(PRIV_RESTRICTQRUN, PrivacyFlags))
	{
		struct stat stbuf;

		/* check to see if we own the queue directory */
		if (stat(QueueDir, &stbuf) < 0)
			syserr("main: cannot stat %s", QueueDir);
		if (stbuf.st_uid != RealUid)
		{
			/* nope, really a botch */
			usrerr("You do not have permission to process the queue");
			exit (EX_NOPERM);
		}
	}
# endif /* QUEUE */

	switch (OpMode)
	{
	  case MD_FREEZE:
		/*
		**  this is critical to avoid forgeries
		**  of the frozen configuration
		*/
		(void) setgid(getgid());
		(void) setuid(getuid());

		/* freeze the configuration */
		freeze(FreezeFile);
		exit(EX_OK);

	  case MD_INITALIAS:
		Verbose = TRUE;
		break;
	}

	/* do heuristic mode adjustment */
	if (Verbose)
	{
		/* turn off noconnect option */
		setoption('c', "F", TRUE, FALSE, CurEnv);

		/* turn on interactive delivery */
		setoption('d', "", TRUE, FALSE, CurEnv);
	}

	/* our name for SMTP codes */
	expand("\001j", jbuf, &jbuf[sizeof jbuf - 1], CurEnv);
	MyHostName = jbuf;
# ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "local hostname %s", MyHostName);
# endif /* LOG */


	/* the indices of local and program mailers */
	st = stab("local", ST_MAILER, ST_FIND);
	if (st == NULL)
		syserr("No local mailer defined");
	else
		LocalMailer = st->s_mailer;
	st = stab("prog", ST_MAILER, ST_FIND);
	if (st == NULL)
		syserr("No prog mailer defined");
	else
		ProgMailer = st->s_mailer;

	/* enable remote delivery mode if requested */
	if (RemoteServer && RemoteServer[0]=='\0')
	{
		RemoteDefault();	
	} 
	if (RemoteServer && OpMode == MD_DELIVER && !queuemode && !GrabTo
	    && av[0] != NULL)
	{
		int r;
		r = RemoteMode(from, av, NULL, CurEnv);
		exit(r);
	}

	/* operate in queue directory */
	if (chdir(QueueDir) < 0)
	{
		syserr("Cannot chdir(%s)", QueueDir);
		exit(EX_SOFTWARE);
	}

	/*
	**  Do operation-mode-dependent initialization.
	*/

	switch (OpMode)
	{
	  case MD_PRINT:
		/* print the queue */
# ifdef QUEUE
		dropenvelope(CurEnv);
		printqueue();
		exit(EX_OK);
# else /* QUEUE */
		usrerr("No queue to print");
		finis();
# endif /* QUEUE */

	  case MD_INITALIAS:
		/* initialize alias databases */
		ExitStat = EX_OK;
		AliasDB = initaliases(AliasFile, TRUE, CurEnv);
		if (ReverseAliasFile)
			ReverseAliasDB = initaliases(ReverseAliasFile, TRUE, CurEnv);
		exit(ExitStat);

	  case MD_KILL:
	  case MD_SMTP:
	  case MD_DAEMON:
		/* don't open alias database -- done in srvrsmtp */
		break;

	  default:
		/* open the alias databases */
		AliasDB = initaliases(AliasFile, FALSE, CurEnv);
		if (ReverseAliasFile)
			ReverseAliasDB = initaliases(ReverseAliasFile, FALSE, CurEnv);
		break;
	}

# ifdef DEBUG
	if (tTd(0, 15))
	{
		/* print configuration table (or at least part of it) */
		printrules();
		for (i = 0; i < MAXMAILERS; i++)
		{
			register struct mailer *m = Mailer[i];
			int j;

			if (m == NULL)
				continue;
			printf("mailer %d (%s): P=%s S=%d R=%d M=%ld F=", i,
				m->m_name, m->m_mailer, m->m_s_rwset,
				m->m_r_rwset, m->m_maxsize);
			for (j = '\0'; j <= '\177'; j++)
				if (bitnset(j, m->m_flags))
					(void) putchar(j);
			printf(" E=");
			xputs(m->m_eol);
			printf("\n");
		}
	}
# endif /* DEBUG */

	/*
	**  Switch to the main envelope.
	*/

	BlankEnvelope.e_flags &= ~EF_FATALERRS;
	CurEnv = newenvelope(&MainEnvelope, CurEnv);
	MainEnvelope.e_flags = BlankEnvelope.e_flags;

	/*
	**  If test mode, read addresses from stdin and process.
	*/

	if (OpMode == MD_TEST)
	{
		char buf[MAXLINE];

		printf("ADDRESS TEST MODE\nEnter <ruleset> <address>\n");
		for (;;)
		{
			register char **pvp;
			char *q;
			extern char *DelimChar;

			printf("> ");
			(void) fflush(stdout);
			if (fgets(buf, sizeof buf, stdin) == NULL)
				finis();
			for (p = buf; isspace(*p); p++)
				continue;
			q = p;
			while (*p != '\0' && !isspace(*p))
				p++;
			if (*p == '\0')
				continue;
			*p = '\0';
			do
			{
				char pvpbuf[PSBUFSIZE];

				pvp = prescan(++p, ',', pvpbuf);
				if (pvp == NULL)
					continue;
				rewrite(pvp, 3, CurEnv);
				p = q;
				while (*p != '\0')
				{
					rewrite(pvp, atoi(p), CurEnv);
					while (*p != '\0' && *p++ != ',')
						continue;
				}
			} while (*(p = DelimChar) != '\0');
		}
	}

# ifdef QUEUE
	/*
	**  If collecting stuff from the queue, go start doing that.
	*/

	if (queuemode && OpMode != MD_DAEMON && QueueIntvl == 0)
	{
		/* Set real uid & gid to defaults */
		(void) setresgid(DefGid, -1, -1);
		(void) setresuid(DefUid, -1, -1);
		runqueue(FALSE);
		finis();
	}
# endif /* QUEUE */

	/*
	**  If a daemon or killing the daemon, initialize the 
	**  semaphore.
	*/

	if (OpMode == MD_DAEMON || OpMode == MD_KILL)
	{
		if (seminit(euid, egid, FALSE) < 0)
		{
			syserr("Cannot initialize semaphore set");
			exit(EX_OSERR);
		}

		/*
		**  On the normal system,
		**	Only the real owner may kill the daemon.
		**	Only root may start the daemon, unless
		**	we listen on a non-reserved port.
		*/
		(void) setgid(getgid());
		(void) setuid(getuid());
	}

	if (OpMode == MD_KILL)
	{
		int old_daemon_pid;

		/* return value and errno from semctl() call */
		if ((old_daemon_pid = daemonpid()) < 0)
		{
			syserr("Cannot get daemon PID");
			exit(EX_OSERR);
		}
		else if (old_daemon_pid == 0 || old_daemon_pid == getpid())
		{
			errno = 0;
			usrerr("I see no daemon here.");
			exit(EX_USAGE);
		}
		else
		{
			int killreturn;

			killreturn = kill(old_daemon_pid, SIGTERM);
			if (killreturn == 0)
				exit(EX_OK);
			if (errno == ESRCH)
			{
				errno = 0;
				usrerr("I see no daemon here.");
				exit(EX_USAGE);
			}
			else if (errno == EPERM)
			{
				usrerr("Cannot kill daemon");
				exit(EX_USAGE);
			}
			else
			{
				syserr("Cannot kill daemon");
				exit(EX_OSERR);
			}
		}
	}

	/*
	**  If a daemon, wait for a request.
	**	getrequests will always return in a child.
	**	If we should also be processing the queue, start
	**		doing it in background.
	**	We check for any errors that might have happened
	**		during startup.
	*/

	if (OpMode == MD_DAEMON || QueueIntvl != 0)
	{
		/* Set real uid & gid to defaults */
		(void) setresgid(DefGid, -1, -1);
		(void) setresuid(DefUid, -1, -1);

		if (!tTd(0, 1))
		{
			/* put us in background */
			i = dofork();
			if (i < 0)
				syserr("daemon: Cannot fork");
			if (i != 0)
				exit(0);

			/* get our pid right */
			MotherPid = getpid();

			if (OpMode == MD_DAEMON && acquire() < 0)
			{
				errno = 0;
				usrerr("A daemon is already running");
				exit(EX_USAGE);
			}

			/* disconnect from our controlling tty */
			disconnect(TRUE, CurEnv);
		}

# ifdef QUEUE
		if (queuemode)
		{
# 	ifdef LOG
			if (QueueIntvl > 0 && LogLevel > 8)
			{
				if (LogLevel > 11)
					syslog(LOG_DEBUG,
					    "queue run started, interval=%d, pid=%d",
					    QueueIntvl, getpid());
				else 
					syslog(LOG_INFO,
					    "queue run started, interval=%d",
					    QueueIntvl);
			}
# 	endif /* LOG */
			runqueue(TRUE);
			if (OpMode != MD_DAEMON)
				for (;;)
					pause();
		}
# endif /* QUEUE */
		dropenvelope(CurEnv);

# ifdef DAEMON
		getrequests();

		/* at this point we are in a child: reset state */
		OpMode = MD_SMTP;
		(void) newenvelope(CurEnv, CurEnv);
		openxscript(CurEnv);
# endif /* DAEMON */
	}
	
# ifdef SMTP
	/*
	**  If running SMTP protocol, start collecting and executing
	**  commands.  This will never return.
	*/

	if (OpMode == MD_SMTP)
		smtp(CurEnv);
# endif /* SMTP */

	/*
	**  Do basic system initialization and set the sender
	*/

	initsys(CurEnv);
	setsender(from, CurEnv);

	if (OpMode != MD_ARPAFTP && *av == NULL && !GrabTo)
	{
		usrerr("Recipient names must be specified");

		/* collect body for UUCP return */
		if (OpMode != MD_VERIFY)
			collect(FALSE, FALSE, CurEnv);
		finis();
	}
	if (GrabTo && RemoteServer)
		NoAlias = TRUE;

	if (OpMode == MD_VERIFY)
		CurEnv->e_sendmode = SM_VERIFY;

	/*
	**  Scan argv and deliver the message to everyone.
	*/

	sendtoargv(av, CurEnv);

	/* if we have had errors sofar, arrange a meaningful exit stat */
	if (Errors > 0 && ExitStat == EX_OK)
		ExitStat = EX_USAGE;

	/*
	**  Read the input mail.
	*/

	CurEnv->e_to = NULL;
	if (OpMode != MD_VERIFY || GrabTo){
		Saw8Bits = FALSE;   
		collect(FALSE, FALSE, CurEnv);
	}
	if (GrabTo && RemoteServer && OpMode == MD_DELIVER)
	{
		ExitStat = RemoteMode(from, NULL, CurEnv->e_sendqueue, CurEnv);
		finis();
	}
	errno = 0;

	/* collect statistics */
	if (OpMode != MD_VERIFY)
		markstats(CurEnv, (ADDRESS *) NULL);

# ifdef DEBUG
	if (tTd(1, 1))
		printf("From person = \"%s\"\n", CurEnv->e_from.q_paddr);
# endif /* DEBUG */

	/*
	**  Actually send everything.
	**	If verifying, just ack.
	*/

	CurEnv->e_from.q_flags |= QDONTSEND;
	CurEnv->e_to = NULL;
	sendall(CurEnv, SM_DEFAULT);

	/*
	**  All done.
	*/

	finis();
}
/*
**  FINIS -- Clean up and exit.
**
**	Parameters:
**		none
**
**	Returns:
**		never
**
**	Side Effects:
**		exits sendmail
*/

void finis()
{
	extern void poststats();
# ifdef DEBUG
	if (tTd(2, 1))
		printf("\n====finis: stat %d e_flags 0x%x\n", ExitStat, CurEnv->e_flags);
# endif /* DEBUG */

	/* clean up temp files */
	CurEnv->e_to = NULL;
	dropenvelope(CurEnv);

	/* post statistics */
	poststats(StatFile);

	/* and exit */
# ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "finis, pid=%d", getpid());
# endif /* LOG */
	if (ExitStat == EX_TEMPFAIL)
		ExitStat = EX_OK;
	exit(ExitStat);
}
/*
**  INTSIG -- clean up on interrupt
**
**	This just arranges to exit.  It pessimises in that it
**	may resend a message.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Unlocks the current job.
*/

void
intsig()
{
	FileName = NULL;
	if (CurEnv->e_id != (char *) NULL)
		unlockqueue(CurEnv);
	exit(EX_OK);
}
/*
**  INITMACROS -- initialize the macro system
**
**	This just involves defining some macros that are actually
**	used internally as metasymbols to be themselves.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		initializes several macros to be themselves.
*/

struct metamac
{
	char	metaname;
	char	metaval;
};

struct metamac	MetaMacros[] =
{
	/* LHS pattern matching characters */
	'*', MATCHZANY,	'+', MATCHANY,	'-', MATCHONE,	'=', MATCHCLASS,
	'~', MATCHNCLASS,

	/* these are RHS metasymbols */
	'#', CANONNET,	'@', CANONHOST,	':', CANONUSER,	'>', CALLSUBR,

	/* HP-only RHS metasymbol */
	'<', CALLNAME,

	/* the conditional operations */
	'?', CONDIF,	'|', CONDELSE,	'.', CONDFI,

        /* and finally the hostname lookup characters */
	'[', HOSTBEGIN, ']', HOSTEND,

	'\0'
};

static void initmacros()
{
	register struct metamac *m;
	char buf[5];
	register int c;

	for (m = MetaMacros; m->metaname != '\0'; m++)
	{
		buf[0] = m->metaval;
		buf[1] = '\0';
		define(m->metaname, newstr(buf), CurEnv);
	}
	buf[0] = MATCHREPL;
	buf[2] = '\0';
	for (c = '0'; c <= '9'; c++)
	{
		buf[1] = c;
		define(c, newstr(buf), CurEnv);
	}
}
/*
**  FREEZE -- freeze BSS & allocated memory
**
**	This will be used to efficiently load the configuration file.
**
**	Parameters:
**		freezefile -- the name of the file to freeze to.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Writes BSS and malloc'ed memory to freezefile
*/

union frz
{
	char		frzpad[BUFSIZ];	/* insure we are on a BUFSIZ boundary */
	struct
	{
		time_t	frzstamp;	/* timestamp on this freeze */
		char	*frzbrk;	/* the current break */
		char	*frzedata;	/* address of edata */
		char	*frzend;	/* address of end */
		char	frzver[252];	/* sendmail version */
	} frzinfo;
};

static void freeze(freezefile)
	char *freezefile;
{
	int f;
	union frz fhdr;
	char *startup_dir = NULL;
	extern char edata, end;
	extern void *sbrk();
	extern char Version[];
  	extern char *getcwd();

	if (freezefile == NULL)
		return;

	/* try to open the freeze file */
	f = creat(freezefile, FileMode);
	if (f < 0)
	{
		syserr("Cannot freeze %s", freezefile);
		errno = 0;
		return;
	}

	/* build the freeze header */
	fhdr.frzinfo.frzstamp = curtime();
	fhdr.frzinfo.frzbrk = sbrk(0);
	fhdr.frzinfo.frzedata = &edata;
	fhdr.frzinfo.frzend = &end;
	(void) strcpy(fhdr.frzinfo.frzver, Version);
# ifdef hp9000s300
	strcat(fhdr.frzinfo.frzver, "-300");
# endif	/* hp9000s300 */
# ifdef hp9000s800
	strcat(fhdr.frzinfo.frzver, "-800");
# endif	/* hp9000s800 */

	/* write out the freeze header */
	if (write(f, (char *) &fhdr, sizeof fhdr) != sizeof fhdr ||
	    write(f, (char *) &edata, (int) (fhdr.frzinfo.frzbrk - &edata)) !=
					(int) (fhdr.frzinfo.frzbrk - &edata))
	{
		syserr("Cannot freeze %s", freezefile);
	}

# ifdef LOG
	if (LogLevel > 8)
	{
		if (ConfFile[0] != '/')
		{
			startup_dir = getcwd((char *)NULL, BUFSIZ);
			strcat(startup_dir, "/");
		}
		if (LogLevel > 11)
		{
			syslog(LOG_DEBUG,
			       "freezing configuration file %s%s, start=0x%08x end=0x%08x",
			       startup_dir, ConfFile, (char *) &edata, fhdr.frzinfo.frzbrk);
		}
		else
		{
			syslog(LOG_INFO,
			       "freezing configuration file %s%s",
			       startup_dir, ConfFile);
		}
	}
# endif /* LOG */

	/* fine, clean up */
	(void) close(f);
}
/*
**  THAW -- read in the frozen configuration file.
**
**	Parameters:
**		freezefile -- the name of the file to thaw from.
**
**	Returns:
**		TRUE if it successfully read the freeze file.
**		FALSE otherwise.
**
**	Side Effects:
**		reads freezefile in to BSS area.
*/

thaw(freezefile)
	char *freezefile;
{
	int f;
	union frz fhdr;
	extern char edata, end;
	extern char Version[];
	extern int brk();
	char version[255];

	if (freezefile == NULL)
		return (FALSE);

	/* open the freeze file */
	f = open(freezefile, 0);
	if (f < 0)
	{
		if (AltFreezeFile)
		{
			/* Haven't thawed; LogLevel is still 0 */
			LogLevel++;
			syserr("Cannot open frozen configuration file %s",
				freezefile);
			exit(ExitStat);
		}
		else
		{
			/* If the freeze file does not exist (ENOENT), this
			 * is OK.  The customer for some reason probably does
			 * not want to use it.  For any other error, log it.
			 */
			if (errno != ENOENT)
				syslog(LOG_WARNING,
					"Cannot open frozen configuration file %s",
					freezefile);
			errno = 0;
			return(FALSE);
		}
	}

	strcpy(version, Version);
# ifdef hp9000s300
	strcat(version, "-300");
# endif	/* hp9000s300 */
# ifdef hp9000s800
	strcat(version, "-800");
# endif	/* hp9000s800 */

	/* read in the header */
	if (read(f, (char *) &fhdr, sizeof fhdr) < sizeof fhdr)
	{
		syserr("Cannot read frozen configuration file");
		(void) close(f);
		return (FALSE);
	}
	if ( fhdr.frzinfo.frzedata != &edata ||
	    fhdr.frzinfo.frzend != &end ||
	    strcmp(fhdr.frzinfo.frzver, version) != 0)
	{
		syslog(LOG_WARNING, "Wrong version of frozen configuration file");
		(void) close(f);
		return (FALSE);
	}

	/* arrange to have enough space */
	if (brk(fhdr.frzinfo.frzbrk) == -1)
	{
		syserr("Cannot break to %x", fhdr.frzinfo.frzbrk);
		(void) close(f);
		return (FALSE);
	}

	/* now read in the freeze file */
	if (read(f, (char *) &edata, (int) (fhdr.frzinfo.frzbrk - &edata)) !=
					(int) (fhdr.frzinfo.frzbrk - &edata))
	{
		syserr("Cannot read frozen configuration file");
		/* oops!  we have trashed memory..... */
		(void) write(2, "Cannot read freeze file\n", 24);
		_exit(EX_SOFTWARE);
	}

	(void) close(f);
	return (TRUE);
}
/*
**  DISCONNECT -- remove our connection with any foreground process
**
**	Parameters:
**		fulldrop -- if set, we should also drop the controlling
**			TTY if possible -- this should only be done when
**			setting up the daemon since otherwise UUCP can
**			leave us trying to open a dialin, and we will
**			wait for the carrier.
**		e -- the current envelope.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Trys to insure that we are immune to vagaries of
**		the controlling tty.
*/

void disconnect(fulldrop, e)
	bool fulldrop;
	register ENVELOPE *e;
{
	int fd;

# ifdef DEBUG
	if (tTd(52, 1))
		printf("disconnect: In %d Out %d\n", fileno(InChannel),
						fileno(OutChannel));
	if (tTd(52, 5))
	{
		printf("don't\n");
		return;
	}
# endif /* DEBUG */

	/* be sure we don't get nasty signals */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);

	/* we can't communicate with our caller, so.... */
	HoldErrs = TRUE;
	if (e->e_errormode != EM_QUIET)
	    e->e_errormode = EM_MAIL;
	Verbose = FALSE;

	/* all input from /dev/null */
	if (InChannel != stdin)
	{
		(void) fclose(InChannel);
		InChannel = stdin;
	}
	(void) freopen("/dev/null", "r", stdin);

	/* output to the transcript */
	if (OutChannel != stdout)
	{
		(void) fclose(OutChannel);
		OutChannel = stdout;
	}
	if (e->e_xfp == NULL)
		e->e_xfp = fopen("/dev/null", "w");
	(void) fflush(stdout);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
# ifdef notdef
	(void) close(1);
	(void) close(2);
	/*
	** I believe this to be unnecessary.  And besides, it 
	** makes for ugly transcripts during background deliveries.
	*/
	while ((fd = dup(fileno(e->e_xfp))) < 2 && fd > 0)
		continue;
# endif	/* notdef */
	/* drop our controlling TTY completely if possible */
	if (fulldrop)
	{
# if BSD > 43
		daemon(1, 1);
# else	/* ! BSD > 43 */
# 	ifdef TIOCNOTTY
		fd = open("/dev/tty", 2);
		if (fd >= 0)
		{
			(void) ioctl(fd, (int) TIOCNOTTY, (char *) 0);
			(void) close(fd);
		}
		(void) setpgrp(0, 0);
# 	else /* ! TIOCNOTTY */
		setpgrp();
# 	endif /* TIOCNOTTY */
# endif /* BSD */
		errno = 0;
	}

# ifdef LOG
	if (LogLevel > 11)
		syslog(LOG_DEBUG, "in background, pid=%d", getpid());
# endif /* LOG */

	errno = 0;
}
