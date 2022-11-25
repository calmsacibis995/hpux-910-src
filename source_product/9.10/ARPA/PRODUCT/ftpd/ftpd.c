/*
 * Copyright (c) 1985, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

# ifndef lint
char copyright[] =
"@(#) Copyright (c) 1985, 1988 Regents of the University of California.\n\
 All rights reserved.\n";
# endif /* not lint */

# ifndef lint
static char rcsid[] = "$Header: ftpd.c,v 1.25.109.8 95/01/10 09:55:49 craig Exp $";
static char sccsid[] = "@(#)ftpd.c	based on 5.28	(Berkeley) 4/20/89";
# endif /* not lint */

#ifdef PATCH_STRING
static char *patch_PHNE_4896 =
	"@(#) PATCH_9.03:        ftpd.o   $Revision: 1.25.109.8 $ 94/10/14 PHNE_4896";
#endif

/*
 * FTP server.
 */
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/socket.h>
# include <sys/file.h>
# include <sys/wait.h>

# ifdef hpux
# 	include <dirent.h> 
# else	/* ! hpux */
# 	include <sys/dir.h>
# endif	/* hpux */

# include <netinet/in.h>

# define	FTP_NAMES
# include <arpa/ftp.h>
# ifndef hpux
# 	include <arpa/inet.h>
# endif	/* not hpux */
# include <arpa/telnet.h>

# include <ctype.h>
# include <stdio.h>
# include <signal.h>
# include <pwd.h>
# include <setjmp.h>
# include <netdb.h>
# include <errno.h>
# include <unistd.h>
# ifdef hpux
# 	include <string.h>
# else	/* ! hpux */
# 	include <strings.h>
# endif	/* hpux */
# include <syslog.h>
# include <varargs.h>
# include "pathnames.h"

# ifdef SecureWare
# 	include <sys/security.h>
# 	include <sys/audit.h>
# 	include <prot.h>
# endif /* SecureWare */

# ifndef MAXHOSTNAMELEN
# 	define MAXHOSTNAMELEN 64
# endif	/* not MAXHOSTNAMELEN */

# if (defined(sun) || defined(hpux)) && !defined(FD_SET)
/* FD_SET being defined is a cheap way of determining if we are on 4.0 or not */
typedef int uid_t;
typedef int gid_t;
# endif	/* defined (sun) || defined(hpux)) && !defined(FD_SET) */

# ifdef hpux
# 	define seteuid(uid)	setresuid(-1, (uid), -1)
# 	define setegid(gid)	setresgid(-1, (gid), -1)
extern char *index(), *rindex();
# endif	/* hpux */


/*
 * AUDIT and SecureWare are mutually exclusively defined.
 */

# if defined(AUDIT) || defined(SecureWare)
# 	include <audnetd.h>
u_short	ruid = (uid_t)-1, aud_luid = (uid_t)-1;
u_long	raddr = (u_long)-1, laddr = (u_long)-1;
# 	ifdef AUDIT
extern void audit_daemon();
# 		define AUDIT_SUCCESS(stat,s) { audit_daemon(NA_SERV_FTP, NA_RSLT_SUCCESS, NA_VALI_PASSWD, \
					     raddr, ruid, laddr, aud_luid, \
					     stat, NA_MASK_RUSR, (s)) ; \
				    }

# 		define AUDIT_FAILURE(s)	{ audit_daemon(NA_SERV_FTP, NA_RSLT_FAILURE, NA_VALI_PASSWD, \
					   raddr, ruid, laddr, aud_luid, \
					   0, NA_MASK_RUSR|NA_MASK_EVNT, (s));\
				}
/* added for ftp daemon system call auditing 03/02/92 */
# 		define SECUREPASS 		"/.secure/etc/passwd"
# 		include <sys/audit.h>
struct passwd *s_pwd;
struct stat s_pfile;

# 	endif /* AUDIT */

# 	ifdef SecureWare
char *rhost = (char *) 0, *rusr = (char *)0, *lusr = (char *) 0;


# 		ifdef B1
/*
 * In the AUDIT_SUCCESS case, I want to preserve any privileges that may
 * already be enabled, so make a copy of the privileges, enable privileges
 * and then just restore the privileges to what they were before.
 */
# 			define ENABLE_PRIV_FOR_AUDIT() \
		if (ISB1) { \
		     getpriv(SEC_EFFECTIVE_PRIV, orig_effprivs); \
		     enablepriv(SEC_ALLOWDACACCESS); \
		     enablepriv(SEC_ALLOWMACACCESS); \
		     enablepriv(SEC_WRITE_AUDIT); \
		}
# 			define DISABLE_PRIV_FOR_AUDIT() \
		if (ISB1) { \
		     setpriv(SEC_EFFECTIVE_PRIV, orig_effprivs); \
		}
# 		else /* not B1 */
# 			define ENABLE_PRIV_FOR_AUDIT()   {}
# 			define DISABLE_PRIV_FOR_AUDIT()  {}
# 		endif /* not B1 */


# 		define AUDIT_SUCCESS(event,s)  \
        if (ISSECURE) { \
		 ENABLE_PRIV_FOR_AUDIT();     \
                 audit_daemon(NA_RSLT_SUCCESS, NA_VALI_PASSWD, \
                        raddr, rhost, ruid, rusr, aud_luid, lusr, event, \
                        NA_MASK_RUID|\
			(aud_luid == (uid_t)-1 ? NA_MASK_LUID : 0)|\
			(rhost == (char *)0 ? NA_MASK_RHOST : 0)|\
			(rusr == (char *)0 ? NA_MASK_RUSRNAME : 0), (s)); \
		 DISABLE_PRIV_FOR_AUDIT();     \
	 }
# 		define AUDIT_FAILURE(s)        \
        if (ISSECURE) {  \
		ENABLE_PRIV_FOR_AUDIT();     \
                audit_daemon(NA_RSLT_FAILURE, NA_VALI_PASSWD, \
                        raddr, rhost, ruid, rusr, aud_luid, lusr, NA_EVNT_START, \
                        NA_MASK_RUID|\
			(aud_luid == (uid_t)-1 ? NA_MASK_LUID : 0)|\
			(raddr == (u_long)-1 ? NA_MASK_RADDR : 0)|\
                        (rhost == (char *)0 ? NA_MASK_RHOST : 0)|\
                        (rusr == (char *)0 ? NA_MASK_RUSRNAME : 0)|\
                        (lusr == (char *)0 ? NA_MASK_LUSR : 0), (s));  \
		DISABLE_PRIV_FOR_AUDIT();     \
	 }
# 	endif   /* SecureWare */

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 * This is here because the audit subsystem is hosed for now.
 * This cannot be released as is or there will be NO auditing!!!
 *
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 */
# 	if ( defined(NO_AUDIT) && (!defined(AUDIT)) )
# 		undef  AUDIT_SUCCESS
# 		define AUDIT_SUCCESS(w,e) {}
# 		undef  AUDIT_FAILURE
# 		define AUDIT_FAILURE(s)   {}
# 	endif	/* defined (NO_AUDIT) && (!defined(AUDIT)) ) */

# else  /* AUDIT || SecureWare */
# 	define AUDIT_SUCCESS(stat,s)   {}
# 	define AUDIT_FAILURE(s)        {}
# endif  /* AUDIT || SecureWare */

# if defined(SecureWare) && defined(B1)
# 	define ENABLEPRIV(priv)	if (ISB1) enablepriv(priv);
# 	define DISABLEPRIV(priv)	if (ISB1) disablepriv(priv);
extern  mask_t   effprivs[SEC_SPRIVVEC_SIZE];
        mask_t   orig_effprivs[SEC_SPRIVVEC_SIZE];
# else	/* ! defined (SecureWare) && defined(B1) */
# 	define ENABLEPRIV(priv)
# 	define DISABLEPRIV(priv)

# endif /* SecureWare && B1 */




extern	int errno;
extern	char *sys_errlist[];
extern	int sys_nerr;
extern	char *crypt();
extern	char version[];
extern	char *home;		/* pointer to home directory for glob */
extern	FILE *ftpd_popen(), *fopen(), *freopen();
extern	int  ftpd_pclose(), fclose();
extern	char *getline();
extern	char cbuf[];
extern	off_t restart_point;

struct	sockaddr_in ctrl_addr;
struct	sockaddr_in data_source;
struct	sockaddr_in data_dest;
struct	sockaddr_in his_addr;
struct	sockaddr_in pasv_addr;

int	data;
jmp_buf	errcatch, urgcatch;
int	logged_in;
struct	passwd *pw;
int	debug;
int	timeout = 900;     /* timeout after 15 minutes of inactivity */
int	maxtimeout = 7200; /* don't allow idle time to be set beyond 2 hours */
int	logging;
int	verbose;
int	guest;
int	type;
int	form;
int	stru;			/* avoid C keyword */
int	mode;
int	usedefault = 1;		/* for data transfers */
int	pdata = -1;		/* for passive mode */
int	transflag;
off_t	file_size;
off_t	byte_count;
# if !defined(CMASK) || CMASK == 0
# 	undef CMASK
# 	define CMASK 027
# endif	/* !defined (CMASK) || CMASK == 0 */
int	defumask = CMASK;		/* default umask value */
char	tmpline[7];
char	hostname[MAXHOSTNAMELEN];
char	remotehost[MAXHOSTNAMELEN];

/*
 * Timeout intervals for retrying connections
 * to hosts that don't accept PORT cmds.  This
 * is a kludge, but given the problems with TCP...
 */
# define	SWAITMAX	90	/* wait at most 90 seconds */
# define	SWAITINT	5	/* interval between retries */

int	swaitmax = SWAITMAX;
int	swaitint = SWAITINT;

void	lostconn();
void	myoob();
FILE	*getdatasock(), *dataconn();


# define KBYTE   1024
# define BIGBUFSIZ 56*KBYTE      /* 56K */
# define ALIGN(addr, pgsz) \
	((char *) (((unsigned int) (addr) + (pgsz - 1)) & ~(pgsz - 1)))

int bufSize = BIGBUFSIZ;
int pageSize;
char *bigBuf;           /* Allocated space of size 2*bufSize + pageSize */

# ifdef SETPROCTITLE
# 	ifdef hpux
char	*Argv[3];
# 	else	/* ! hpux */
char	**Argv = NULL;		/* pointer to argument vector */
char	*LastArgv = NULL;	/* end of argv */
# 	endif	/* hpux */
char	proctitle[BUFSIZ];	/* initial part of title */
# endif /* SETPROCTITLE */

main(argc, argv, envp)
	int argc;
	char *argv[];
	char **envp;
{
	int addrlen, on = 1;
	char *cp, *sgetsave();

# ifdef SecureWare
        if (ISSECURE) {
		set_auth_parameters(argc, argv);
# 	ifdef B1
		if (ISB1)
  			initprivs();  
# 	 endif /* B1 */
	}
# endif /* SecureWare */


	addrlen = sizeof (his_addr);
	if (getpeername(0, (struct sockaddr *)&his_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getpeername (%s): %m",argv[0]);
		exit(1);
	}
# if defined(AUDIT) || defined(SecureWare)
	raddr = his_addr.sin_addr.s_addr;
# endif /* AUDIT	|| SecureWare */
	addrlen = sizeof (ctrl_addr);
	if (getsockname(0, (struct sockaddr *)&ctrl_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getsockname (%s): %m",argv[0]);
		exit(1);
	}
	data_source.sin_port = htons(ntohs(ctrl_addr.sin_port) - 1);
# ifdef AUDIT
	laddr = ctrl_addr.sin_addr.s_addr;
#  endif /* AUDIT */
	debug = 0;
# ifdef LOG_DAEMON
	openlog("ftpd", LOG_PID, LOG_DAEMON);
# else	/* ! LOG_DAEMON */
	openlog("ftpd", LOG_PID);
# endif	/* LOG_DAEMON */
# ifdef SETPROCTITLE
	/*
	 *  Save start and extent of argv for setproctitle.
	 */
# 	ifdef hpux
	Argv[0] = sgetsave(argv[0]);
	Argv[1] = Argv[2] = NULL;
# 	else	/* ! hpux */
	Argv = argv;
	while (*envp)
		envp++;
	LastArgv = envp[-1] + strlen(envp[-1]);
# 	endif	/* hpux */
# endif /* SETPROCTITLE */

	argc--, argv++;
	while (argc > 0 && *argv[0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++) switch (*cp) {

		case 'v': /* Log the info normally logged only for anon. ftp */
			verbose = 1;
			break;

		case 'd':
			debug = 1;
			break;

		case 'l':
			logging = 1;
			break;

		case 't':
			timeout = atoi(++cp);
			if (maxtimeout < timeout)
				maxtimeout = timeout;
			goto nextopt;

		case 'T':
			maxtimeout = atoi(++cp);
			if (timeout > maxtimeout)
				timeout = maxtimeout;
			goto nextopt;

		case 'u':
		    {
			int val = 0;

			if (*++cp == '\0') { /* allow space after -u */
				++argv, --argc;
				cp = argv[0];
			}
			while (*cp && *cp >= '0' && *cp <= '9') {
				val = val*8 + *cp - '0';
				++cp;
			}
			if (*cp)
				fprintf(stderr, "ftpd: Bad value for -u\r\n");
			else
				defumask = val;
			goto nextopt;
		    }

		case 'B':
		    {
		    int nK; /* number of 1024 blocks */

		    if (*++cp == '\0') { /* allow space after -B */
			    ++argv, --argc;
			    cp = argv[0];
		    }

		    nK = atoi(cp);
		    if (nK > 0 && nK < 56)
			    bufSize = nK * KBYTE;
		    goto nextopt;
		    }

		default:
			fprintf(stderr, "ftpd: Unknown flag -%c ignored.\r\n",
			     *cp);
			break;
		}
nextopt:
		argc--, argv++;
	}
	(void) freopen(_PATH_DEVNULL, "w", stderr);
	(void) signal(SIGPIPE, lostconn);
# ifndef hpux
	(void) signal(SIGCHLD, SIG_IGN);
# endif	/* not hpux */
	if ((int)signal(SIGURG, myoob) < 0)
		syslog(LOG_ERR, "signal: %m");

	/* handle urgent data inline */
	/* Sequent defines this, but it doesn't work */
# ifdef SO_OOBINLINE
	if (setsockopt(0, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on)) < 0)
		syslog(LOG_ERR, "setsockopt: %m");
# endif	/* SO_OOBINLINE */
# ifdef	F_SETOWN
	if (fcntl(fileno(stdin), F_SETOWN, getpid()) == -1)
		syslog(LOG_ERR, "fcntl F_SETOWN: %m");
# else	/* ! F_SETOWN */
	on = getpid();
	if (ioctl(fileno(stdin), SIOCSPGRP, &on) < 0)
		syslog(LOG_ERR, "ioctl (SIOCSPGRP)");
# endif	/* F_SETOWN */
	dolog(&his_addr);
# ifdef SecureWare
	rhost = remotehost;
# endif /* SecureWare*/
	
	/*
	  Allocate space for the transfer buffer
	  (it will be used as two buffers of size
          "bufSize," aligned on a page boundary)
        */

	pageSize = sysconf(_SC_PAGE_SIZE);
	bigBuf = (char *) malloc(2 * bufSize + pageSize);
	if ((pageSize < 0) || (bigBuf == NULL))
	      {
	      syslog(LOG_ERR, "Memory allocation failure");
	      exit(1);
	      }
	bigBuf = ALIGN(bigBuf, pageSize);

	/*
	 * Set up default state
	 */
	data = -1;
	type = TYPE_A;
	form = FORM_N;
	stru = STRU_F;
	mode = MODE_S;
	tmpline[0] = '\0';
	(void) gethostname(hostname, sizeof (hostname));
	reply(220, "%s FTP server (%s) ready.", hostname, version);
	(void) setjmp(errcatch);
	for (;;)
		(void) yyparse();
	/* NOTREACHED */
}

void
lostconn()
{

	if (debug)
		syslog(LOG_DEBUG, "lost connection");
	dologout(-1);
}

static char ttyline[20];

/*
 * Helper function for sgetpwnam().
 */
char *
sgetsave(s)
	char *s;
{
	char *new = (char *) malloc((unsigned) strlen(s) + 1);

	if (new == NULL) {
		perror_reply(421, "Local resource failure: malloc");
		dologout(1);
		/* NOTREACHED */
	}
	(void) strcpy(new, s);
	return (new);
}

/*
 * Save the result of a getpwnam.  Used for USER command, since
 * the data returned must not be clobbered by any other command
 * (e.g., globbing).
 */
struct passwd *
sgetpwnam(name)
	char *name;
{
	static struct passwd save;
	register struct passwd *p;
	char *sgetsave();

	if ((p = getpwnam(name)) == NULL)
		return (p);
	if (save.pw_name) {
		free(save.pw_name);
		free(save.pw_passwd);
		free(save.pw_gecos);
		free(save.pw_dir);
		free(save.pw_shell);
	}
	save = *p;
	save.pw_name = sgetsave(p->pw_name);
	save.pw_passwd = sgetsave(p->pw_passwd);
	save.pw_gecos = sgetsave(p->pw_gecos);
	save.pw_dir = sgetsave(p->pw_dir);
	save.pw_shell = sgetsave(p->pw_shell);
# if defined(AUDIT) || defined(SecureWare)
	aud_luid = (uid_t) p->pw_uid;
# 	if defined(SecureWare)
	lusr = save.pw_name;
# 	endif /* defined(SecureWare) */
# endif /* (AUDIT) || (SecureWare) */
	return (&save);
}

int login_attempts;		/* number of failed login attempts */
int askpasswd;			/* had user command, ask for passwd
				 * If askpasswd==0, then waiting for user name
				 * If askpasswd==1, then waiting for password
				 * If askpasswd==2, then in secure mode and
				 * change of user is not allow, so the state is
				 * just to eat any incoming password
				 */

/*
 * USER command.
 * Sets global passwd pointer pw if named account exists
 * and is acceptable; sets askpasswd if a PASS command is
 * expected. If logged in previously, need to reset state.
 * If name is "ftp" or "anonymous" and ftp account exists,
 * set guest and pw, then just return.
 * If account doesn't exist, ask for passwd anyway.
 * Otherwise, check user requesting login privileges.
 * Disallow anyone who does not have a standard
 * shell as returned by getusershell().
 * Disallow anyone mentioned in the file _PATH_FTPUSERS
 * to allow people such as root and uucp to be avoided.
 */
# ifndef hpux
/* 
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 */
user(name)
	char *name;
{
	register char *cp;
	FILE *fd;
	char *shell;
	char line[BUFSIZ], *getusershell();

	if (logged_in) {
		if (guest) {
			reply(530, "Can't change user from guest login.");
			return;
		}
		end_login();
	}

	guest = 0;
	if (strcmp(name, "ftp") == 0 || strcmp(name, "anonymous") == 0) {
		if ((pw = sgetpwnam("ftp")) != NULL) {
			guest = 1;
			askpasswd = 1;
			reply(331, "Guest login ok, send ident as password.");
		} else
			reply(530, "User %s unknown.", name);
		return;
	}
	if (pw = sgetpwnam(name)) {
		if ((shell = pw->pw_shell) == NULL || *shell == 0)
			shell = _PATH_BSHELL;
		while ((cp = getusershell()) != NULL)
			if (strcmp(cp, shell) == 0)
				break;
		endusershell();
		if (cp == NULL) {
			reply(530, "User %s access denied.", name);
			if (logging)
				syslog(LOG_NOTICE,
				    "FTP LOGIN REFUSED FROM %s, %s",
				    remotehost, name);
			pw = (struct passwd *) NULL;
			return;
		}
		if ((fd = fopen(_PATH_FTPUSERS, "r")) != NULL) {
		    while (fgets(line, sizeof (line), fd) != NULL) {
			if ((cp = index(line, '\n')) != NULL)
				*cp = '\0';
			if (strcmp(line, name) == 0) {
				reply(530, "User %s access denied.", name);
				if (logging)
					syslog(LOG_NOTICE,
					    "FTP LOGIN REFUSED FROM %s, %s",
					    remotehost, name);
				pw = (struct passwd *) NULL;
				return;
			}
		    }
		}
		(void) fclose(fd);
	}
	reply(331, "Password required for %s.", name);
	askpasswd = 1;
	/*
	 * Delay before reading passwd after first failed
	 * attempt to slow down passwd-guessing programs.
	 */
	if (login_attempts)
		sleep((unsigned) login_attempts);
}
# else /* hpux */

char username[20] = "";  /* For logging use in pass() */

user(name)
	char *name;
{
	register char *cp;
	FILE *fd;
	char *shell;
	char line[BUFSIZ];

	if (logged_in) {
# 	if defined(SecureWare) && defined(B1) 
		if (ISSECURE) {
			/*
			 * Since the audit id is immutable and 
			 * privileges/clearances can not be reset properly
			 * (if B1 was being run),
			 * a change of user is not permitted in the 
			 * secure version of ftp
			 */
			reply(530, "Second USER command not permitted.");
			AUDIT_FAILURE("Attempt to change user id with secure ftp");
			askpasswd = 2; /* To skip over the incoming password */
			return;
		} else
# 	endif /* defined(SecureWare) && defined(B1) */

		end_login(); /* Executed if !ISB1 or SecureWare&B1 not defined*/
	}

	strncpy(username, name, sizeof(username)-1);
	username[sizeof(username)-1] = '\0';

	if (strlen(name) == 0) {
		reply(530, "Login incorrect.");
		AUDIT_FAILURE("Missing user name");
		return;
	}
	guest = 0;
	if (strcmp(name, "ftp") == 0 || strcmp(name, "anonymous") == 0) {
		if ((pw = sgetpwnam("ftp")) != NULL) {
# 	if defined(SecureWare)
			if (ISSECURE) {
				ENABLEPRIV(SEC_ALLOWDACACCESS);
				ENABLEPRIV(SEC_ALLOWMACACCESS);
				/*
				 * Check the environment of the host to
				 * determine if it is safe to allow
				 * anonymous ftp.
				 */
				if (check_ftp_env(pw) == -1) {
					DISABLEPRIV(SEC_ALLOWDACACCESS);
					DISABLEPRIV(SEC_ALLOWMACACCESS);
					reply(530, 
				 	"Guest login not permitted.");
					return;
				}
				DISABLEPRIV(SEC_ALLOWDACACCESS);
				DISABLEPRIV(SEC_ALLOWMACACCESS);
			}
# 	endif /* defined(SecureWare) */
			guest = 1;
			askpasswd = 1;
			reply(331, "Guest login ok, send ident as password.");
		} else {
# 	ifdef hpux
			reply(530, "Guest login not permitted.");
# 	else /* hpux */
			reply(530, "User %s unknown.", name);
# 	endif /* hpux */
			AUDIT_FAILURE("Guest login attempt");
		}
		return;
	}
	pw = sgetpwnam(name);
	reply(331, "Password required for %s.", name);
	askpasswd = 1;
	/*
	 * Delay before reading passwd after first failed
	 * attempt to slow down passwd-guessing programs.
	 */
	if (login_attempts)
		sleep((unsigned) login_attempts);
}
# endif /* hpux */
/*
 * Terminate login as previous user, if any, resetting state;
 * used when USER command is given or login fails.
 */
end_login()
{
# if defined(SecureWare) && defined(B1)
	if (! ISB1)
# endif  /* defined(SecureWare) && defined(B1) */
		(void) seteuid((uid_t)0);
	if (logged_in) {
	        if (logging)
		    syslog(LOG_INFO, "User %s logged out", pw->pw_name);
		ENABLEPRIV(SEC_ALLOWDACACCESS);
		ENABLEPRIV(SEC_ALLOWMACACCESS);
		logwtmp(ttyline, "", "");
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		DISABLEPRIV(SEC_ALLOWMACACCESS);
# if defined(SecureWare) 
		if (ISSECURE) {
			/*
			 * Since a Secure FTP does not  allow a change of
			 * users, don't bother resetting the state with
			 * hopes of starting over.  Just blow out of here.
			 */
			dologout(0);
		}
# endif  /* defined(SecureWare) && defined(B1) */
	}
	pw = NULL;
	logged_in = 0;
	guest = 0;
}

# ifndef hpux
/* 
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 * THIS IS NOT THE HP VERSION !!!!!
 */
pass(passwd)
	char *passwd;
{
	char *xpasswd, *salt;

	if (logged_in || askpasswd == 0) {
		reply(503, "Login with USER first.");
		return;
	}
	askpasswd = 0;
	if (!guest) {		/* "ftp" is only account allowed no password */
		if (pw == NULL)
			salt = "xx";
		else
			salt = pw->pw_passwd;
		xpasswd = crypt(passwd, salt);
		/* The strcmp does not catch null passwords! */
		if (pw == NULL || *pw->pw_passwd == '\0' ||
		    strcmp(xpasswd, pw->pw_passwd)) {
			reply(530, "Login incorrect.");
			pw = NULL;
			if (login_attempts++ >= 5) {
				syslog(LOG_NOTICE,
				    "repeated login failures from %s",
				    remotehost);
				exit(0);
			}
			return;
		}
	}
	login_attempts = 0;		/* this time successful */
	(void) setegid((gid_t)pw->pw_gid);
	(void) initgroups(pw->pw_name, pw->pw_gid);

	/* open wtmp before chroot */
	(void)sprintf(ttyline, "ftp%d", getpid());
	logwtmp(ttyline, pw->pw_name, remotehost);
	logged_in = 1;

	if (guest) {
		/*
		 * We MUST do a chdir() after the chroot. Otherwise
		 * the old current directory will be accessible as "."
		 * outside the new root!
		 */
		if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
			reply(550, "Can't set guest privileges.");
			goto bad;
		}
	} else if (chdir(pw->pw_dir) < 0) {
		if (chdir("/") < 0) {
			reply(530, "User %s: can't change directory to %s.",
			    pw->pw_name, pw->pw_dir);
			goto bad;
		} else
			lreply(230, "No directory! Logging in with home=/");
	}
	if (seteuid((uid_t)pw->pw_uid) < 0) {
		reply(550, "Can't set uid.");
		goto bad;
	}
	if (guest) {
		reply(230, "Guest login ok, access restrictions apply.");
# 	ifdef SETPROCTITLE
		sprintf(proctitle, "%s: anonymous/%.*s", remotehost,
		    sizeof(proctitle) - sizeof(remotehost) -
		    sizeof(": anonymous/"), passwd);
		setproctitle(proctitle);
# 	endif /* SETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, "ANONYMOUS FTP LOGIN FROM %s, %s",
			    remotehost, passwd);
	} else {
		reply(230, "User %s logged in.", pw->pw_name);
# 	ifdef SETPROCTITLE
		sprintf(proctitle, "%s: %s", remotehost, pw->pw_name);
		setproctitle(proctitle);
# 	endif /* SETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, "FTP LOGIN FROM %s, %s",
			    remotehost, pw->pw_name);
	}
	home = pw->pw_dir;		/* home dir for globbing */
	(void) umask(defumask);
	return;
bad:
	/* Forget all about it... */
	end_login();
}
# else /* hpux */
int
checkuser(name)
	register char *name;
{
	register char *cp;
	FILE *fd;
	char *shell;
	char line[BUFSIZ], *getusershell();


	if ((shell = pw->pw_shell) == NULL || *shell == 0)
		shell = _PATH_BSHELL;
	
	ENABLEPRIV(SEC_ALLOWDACACCESS);
	ENABLEPRIV(SEC_ALLOWMACACCESS);
	while ((cp = getusershell()) != NULL)
		if (strcmp(cp, shell) == 0)
			break;
	endusershell();
	if (cp == NULL) {
		AUDIT_FAILURE("Invalid user shell");
		if (logging)
			syslog(LOG_NOTICE,
		    "FTP LOGIN REFUSED FROM %s, %s: Invalid user shell",
			    remotehost, name);
		DISABLEPRIV(SEC_ALLOWDACACCESS);
		DISABLEPRIV(SEC_ALLOWMACACCESS);
		return(0);
	}
	if ((fd = fopen(_PATH_FTPUSERS, "r")) != NULL) {
		while (fgets(line, sizeof (line), fd) != NULL) {
			if ((cp = index(line, '\n')) != NULL)
				*cp = '\0';
			if (strcmp(line, name) == 0) {
				AUDIT_FAILURE("User listed in ftpusers");
				if (logging)
					syslog(LOG_NOTICE,
		    "FTP LOGIN REFUSED FROM %s, %s: User listed in ftpusers",
					    remotehost, name);
				DISABLEPRIV(SEC_ALLOWDACACCESS);
				DISABLEPRIV(SEC_ALLOWMACACCESS);
				return(0);
			}
		}
	}
	(void) fclose(fd);
	DISABLEPRIV(SEC_ALLOWDACACCESS);
	DISABLEPRIV(SEC_ALLOWMACACCESS);
	return(1);
}

pass(passwd)
	char *passwd;
{
	char *xpasswd, *salt;

# 	ifdef SecureWare
	int luid;

# 		ifdef B1
/*
 * No need to check for  if (ISB1) because askpasswd would only
 * have been set to 2 IF ISB1 is TRUE.
 */
	if (askpasswd == 2) {
		askpasswd = 0;
		reply(503, "Password ignored.");
		return;
	}
# 		endif /* B1 */
# 	endif /* SecureWare */


	if (logged_in || askpasswd == 0) {
		reply(503, "Login with USER first.");
		AUDIT_FAILURE("Attempted PASS before USER");
		return;
	}
	askpasswd = 0;
	if (!guest) {		/* "ftp" is only account allowed no password */
# 	ifdef SecureWare
		if (ISSECURE) {
			if ( (luid = ftpd_check_prpw(pw, passwd, 0)) == -1 ) {
                                if (logging)
                                        syslog(LOG_INFO,
                                               "User %s login incorrect",
                                               pw->pw_name);
				if (login_attempts++ >= 5) {
					AUDIT_FAILURE("Repeated login failures");
					syslog(LOG_NOTICE,
				    	"REPEATED LOGIN FAILURES FROM %s",
				    	remotehost);
					exit(0);
				}
				return; 
			}
		} else 
# 	endif  /* SecureWare */
		       {
			int badpasswd = 0;
			if (pw == NULL)
				salt = "xx";
			else
				salt = pw->pw_passwd;

			xpasswd = crypt(passwd, salt);
			if (pw != NULL)
				badpasswd = (*pw->pw_passwd == '\0') ||
			    	    	strcmp(xpasswd, pw->pw_passwd);
			/* The strcmp does not catch null passwords! */
			if (pw == NULL || badpasswd || 
		    	     !checkuser(pw->pw_name)) {
				reply(530, "Login incorrect.");
				if (pw == NULL || badpasswd) {
				    if (logging)
					syslog(LOG_INFO,
					       "User %s: Login incorrect", username);
				    AUDIT_FAILURE(pw == NULL ? 
						  "No entry in passwd file" :
						  "Password verification failed");
				}
				pw = NULL;
				if (login_attempts++ >= 5) {
					AUDIT_FAILURE("Repeated login failures");
					syslog(LOG_NOTICE,
				    	"REPEATED LOGIN FAILURES FROM %s",
				    	remotehost);
					exit(0);
				}
				return;
			}
		}  /* Here to match the bracket after the else for */
		   /* if (ISSECURE).  It can be to match the else  */
		   /* or just to put the unsecure part in a {}     */
# 	ifdef SecureWare
	/* IF anonymous ftp is required, then the else only needs  */
	/* to be here to do something in the secure code.          */
	} else {   
		/*
		 * Here for anonymous ftp
		 */
		 if (ISSECURE)
			if ( (luid = ftpd_check_prpw(pw, passwd, 1)) == -1)
				return;
# 	endif /* SecureWare */
	}
	login_attempts = 0;		/* this time successful */

# 	ifdef SecureWare
/* 
 * The ENABLEPRIVS and DISABLEPRIVS are OK here because they are done
 * before the change of privileges for the user.
 */
	if (ISSECURE) {
		ENABLEPRIV(SEC_SETPROCIDENT);
		if (setluid(luid) == -1) {
			char auditbuf[BUFSIZ];
                        
			sprintf(auditbuf, "setluid(%d): %s",
                                luid, sys_errlist[errno]);
                        AUDIT_FAILURE(auditbuf)
			DISABLEPRIV(SEC_SETPROCIDENT);
			exit(0);
		}
	}
# 		ifdef B1
	if (ISB1 && ftpd_setclrnce() == -1)
		return;
# 		endif /* B1 */
# 	endif /* SecureWare */

	(void) setegid((gid_t)pw->pw_gid);
	(void) initgroups(pw->pw_name, pw->pw_gid);

	ENABLEPRIV(SEC_ALLOWDACACCESS);
	ENABLEPRIV(SEC_ALLOWMACACCESS);
	/* open wtmp before chroot */
	(void)sprintf(ttyline, "ftp%d", getpid());
	logwtmp(ttyline, pw->pw_name, remotehost);
	logged_in = 1;


# 	ifdef AUDIT
/* audit successfull login before setting eid to user */
    AUDIT_SUCCESS(NA_EVNT_START, NULL);

/* Set up FTP daemon for system call auditing if a .secure/etc/passwd */
/* file exists       												  */
    if (stat(SECUREPASS, &s_pfile) >= 0) {
        s_pwd = getpwuid(pw->pw_uid);
        if (s_pwd->pw_audflg > 1 || s_pwd->pw_audflg < 0) {
                fputs("Bad audit flag\n", stdout);
                exit(1);
        }
        /* Set the audit id */
        if (setaudid(s_pwd->pw_audid) == -1) {
                fputs("Bad audit id\n", stdout);
                exit(1);
        }
        setaudproc(s_pwd->pw_audflg);
    }
# 	endif /* AUDIT */

	if (guest) {
		/*
		 * We MUST do a chdir() after the chroot. Otherwise
		 * the old current directory will be accessible as "."
		 * outside the new root!
		 */
		ENABLEPRIV(SEC_CHROOT);
		if (chroot(pw->pw_dir) < 0 || chdir("/") < 0) {
			syslog(LOG_ERR, "chroot %s: %m", pw->pw_dir);
			DISABLEPRIV(SEC_CHROOT);
			reply(550, "Can't set guest privileges.");
			AUDIT_FAILURE("Failed chroot to guest directory");
			goto bad;
		}
		DISABLEPRIV(SEC_CHROOT);
	} else if (chdir(pw->pw_dir) < 0) {
		syslog(LOG_ERR, "chdir %s: %m", pw->pw_dir);
		reply(530, "User %s: can't change directory to %s.",
		    pw->pw_name, pw->pw_dir);
		AUDIT_FAILURE("Failed chdir to home directory");
		goto bad;
	}
	if (seteuid((uid_t)pw->pw_uid) < 0) {
		syslog(LOG_ERR, "seteuid (%d): %m", pw->pw_uid);
		reply(550, "Can't set uid.");
		AUDIT_FAILURE("Invalid user id");
		goto bad;
	}

# 	ifdef SecureWare
	/*
	 * Do the audit before setting up the privileges of the
	 * ftpuser which may not have the necessary privileges.
	 */
	
  	AUDIT_SUCCESS(NA_EVNT_START, NULL); 

	DISABLEPRIV(SEC_ALLOWDACACCESS);
	DISABLEPRIV(SEC_ALLOWMACACCESS);
	DISABLEPRIV(SEC_SETPROCIDENT);
       
	/*
	 * Now set this process up with the privileges that the user is allowed
	 */

	if (ISSECURE)
		ftpd_setup_privs();
# 	endif /* SecureWare */

	if (guest) {
		reply(230, "Guest login ok, access restrictions apply.");
# 	ifdef SETPROCTITLE
		sprintf(proctitle, "%s: anonymous/%.*s", remotehost,
		    sizeof(proctitle) - sizeof(remotehost) -
		    sizeof(": anonymous/"), passwd);
		setproctitle(proctitle);
# 	endif /* SETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, "ANONYMOUS FTP LOGIN FROM %s, %s",
			    remotehost, passwd);
		home = "/";		/* home dir for globbing */
	} else {
		reply(230, "User %s logged in.", pw->pw_name);
# 	ifdef SETPROCTITLE
		sprintf(proctitle, "%s: %s", remotehost, pw->pw_name);
		setproctitle(proctitle);
# 	endif /* SETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, "FTP LOGIN FROM %s, %s",
			    remotehost, pw->pw_name);
		home = pw->pw_dir;	/* home dir for globbing */
	}
	(void) umask(defumask);
	return;
bad:
	DISABLEPRIV(SEC_ALLOWDACACCESS);
	DISABLEPRIV(SEC_ALLOWMACACCESS);
	DISABLEPRIV(SEC_SETPROCIDENT);

	/* Forget all about it... */
	end_login();
}
# endif  /* hpux */

retrieve(cmd, name)
	char *cmd, *name;
{
	FILE *fin, *dout;
	struct stat st;
	int (*closefunc)();

	if (cmd == 0) {
		fin = fopen(name, "r"), closefunc = fclose;
		st.st_size = 0;
	} else {
		char line[BUFSIZ];

		(void) sprintf(line, cmd, name), name = line;
		fin = ftpd_popen(line, "r"), closefunc = ftpd_pclose;
		st.st_size = -1;
		st.st_blksize = BIGBUFSIZ;
	}
	if (fin == NULL) {
		if (errno != 0)
			perror_reply(550, name);
		return;
	}
	if (cmd == 0 &&
	    (fstat(fileno(fin), &st) < 0 || (st.st_mode&S_IFMT) != S_IFREG)) {
		reply(550, "%s: not a plain file.", name);
		goto done;
	}
# ifdef NEW_LOGGING
	if (cmd == 0 && logging) {
		if (guest)
			syslog(LOG_INFO, "ANONYMOUS FTP: retrieve %s", name);
		else if (verbose)
			syslog(LOG_INFO, "FTP: retrieve %s", name);
	}
# endif	/* NEW_LOGGING */
	if (restart_point) {
		if (type == TYPE_A) {
			register int i, n, c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fin)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}	
		} else if (lseek(fileno(fin), restart_point, L_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	dout = dataconn(name, st.st_size, "w");
	if (dout == NULL)
		goto done;
	send_data(fin, dout);
	(void) fclose(dout);
	data = -1;
	pdata = -1;
done:
	(*closefunc)(fin);
}

store(name, mode, unique)
	char *name, *mode;
	int unique;
{
	FILE *fout, *din;
	struct stat st;
	int (*closefunc)();
	char *gunique();

	if (unique && stat(name, &st) == 0 &&
	    (name = gunique(name)) == NULL)
		return;

	if (restart_point)
		mode = "r+w";
	fout = fopen(name, mode);
	closefunc = fclose;
	if (fout == NULL) {
		perror_reply(553, name);
		return;
	}
# ifdef NEW_LOGGING
	if (logging) { 
		if (guest)
			syslog(LOG_INFO, "ANONYMOUS FTP: store %s", name);
		else if (verbose)
			syslog(LOG_INFO, "FTP: store %s", name);
	}
# endif	/* NEW_LOGGING */
	if (restart_point) {
		if (type == TYPE_A) {
			register int i, n, c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fout)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}	
			/*
			 * We must do this seek to "current" position
			 * because we are changing from reading to
			 * writing.
			 */
			if (fseek(fout, 0L, L_INCR) < 0) {
				perror_reply(550, name);
				goto done;
			}
		} else if (lseek(fileno(fout), restart_point, L_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	din = dataconn(name, (off_t)-1, "r");
	if (din == NULL)
		goto done;
	if (receive_data(din, fout) == 0) {
		if (unique)
			reply(226, "Transfer complete (unique file name:%s).",
			    name);
		else
			reply(226, "Transfer complete.");
	}
	(void) fclose(din);
	data = -1;
	pdata = -1;
done:
	(*closefunc)(fout);
}

FILE *
getdatasock(mode)
	char *mode;
{
	int s, on = 1, tries;
	int bufsize = bufSize;
	FILE *datastream;

	if (data >= 0)
		return (fdopen(data, mode));
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (NULL);

# if defined(SecureWare) && defined(B1)
	if (ISB1) {
		if (ftpd_forcepriv(SEC_REMOTE) == 0)
			syslog(LOG_ERR, 
			      "ftpd: requires remote potential privilege");
	} else
# endif /* defined(SecureWare) && defined(B1) */
	(void) seteuid((uid_t)0); /* Executed if !ISB1            */
				  /* or SecureWare&B1 not defined */

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	    (char *) &on, sizeof (on)) < 0)
	{
		syslog(LOG_ERR, "setsockopt (SO_REUSEADDR)");
		goto bad;
	}
	if (*mode == 'w') {
		if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, 
			       &bufsize, sizeof(bufsize)) < 0)
		{
			syslog(LOG_ERR, "setsockopt (SO_SNDBUF)");
			goto bad;
		}

		/* Set copy avoidance on send */
		if (setsockopt(
			s, SOL_SOCKET, SO_SND_COPYAVOID, &on, sizeof(on)) < 0)
		{
			syslog(LOG_ERR, "setsockopt (SO_SND_COPYAVOID)");
			goto bad;
		}

	}

	if (setsockopt(s, SOL_SOCKET, SO_RCVBUF,
				&bufsize, sizeof(bufsize)) < 0)
	{
		syslog(LOG_ERR, "setsockopt (SO_RCVBUF)");
		goto bad;
	}

	/* Set copy avoidance on receive */
	if (setsockopt(s, SOL_SOCKET, SO_RCV_COPYAVOID, &on, sizeof(on)) < 0) 
	{
		syslog(LOG_ERR, "setsockopt (SO_RCV_COPYAVOID)");
		goto bad;
	}

	/* anchor socket to avoid multi-homing problems */
	data_source.sin_family = AF_INET;
	data_source.sin_addr = ctrl_addr.sin_addr;
	for (tries = 1; ; tries++) {
		if (bind(s, (struct sockaddr *)&data_source,
		    sizeof (data_source)) >= 0)
			break;
		if (errno != EADDRINUSE || tries > 10)
			goto bad;
		sleep(tries);
	}
# if defined(SecureWare) && defined(B1)
	if (ISB1) {
		(int) ftpd_restorepriv();
	} else 
# endif /* defined(SecureWare) && defined(B1) */
	(void) seteuid((uid_t)pw->pw_uid); /* Executed if !ISB1 or      */
					   /* SecureWare&B1 not defined */

	datastream = fdopen(s, mode);
	if (setvbuf(datastream, (char *)NULL, _IOFBF, BIGBUFSIZ) != 0)
		goto bad;
	return (datastream);
bad:
# if defined(SecureWare) && defined(B1)
	if (ISB1) {
	 	(int) ftpd_restorepriv();
	}
# endif /* defined(SecureWare) && defined(B1) */
	(void) seteuid((uid_t)pw->pw_uid); /* Executed if !ISB1 or      */
					   /* SecureWare&B1 not defined */
	(void) close(s);
	return (NULL);
}

FILE *
dataconn(name, size, mode)
	char *name;
	off_t size;
	char *mode;
{
	char sizebuf[32];
	FILE *file;
	int retry = 0;

	file_size = size;
	byte_count = 0;
	if (size != (off_t) -1)
		(void) sprintf (sizebuf, " (%ld bytes)", size);
	else
		(void) strcpy(sizebuf, "");
	if (pdata >= 0) {
		struct sockaddr_in from;
		int s, fromlen = sizeof(from);

		s = accept(pdata, (struct sockaddr *)&from, &fromlen);
		if (s < 0) {
			reply(425, "Can't open data connection.");
			(void) close(pdata);
			pdata = -1;
			return(NULL);
		}
		(void) close(pdata);
		pdata = s;
		reply(150, "Opening %s mode data connection for %s%s.",
		     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
		return(fdopen(pdata, mode));
	}
	if (data >= 0) {
		reply(125, "Using existing data connection for %s%s.",
		    name, sizebuf);
		usedefault = 1;
		return (fdopen(data, mode));
	}
	if (usedefault)
		data_dest = his_addr;
	usedefault = 1;
	file = getdatasock(mode);
	if (file == NULL) {
		reply(425, "Can't create data socket (%s,%d): %s.",
		    inet_ntoa(data_source.sin_addr),
		    ntohs(data_source.sin_port),
		    errno < sys_nerr ? sys_errlist[errno] : "unknown error");
		return (NULL);
	}
	data = fileno(file);
	while (connect(data, (struct sockaddr *)&data_dest,
	    sizeof (data_dest)) < 0) {
		if (errno == EADDRINUSE && retry < swaitmax) {
			sleep((unsigned) swaitint);
			retry += swaitint;
			continue;
		}
		perror_reply(425, "Can't build data connection");
		(void) fclose(file);
		data = -1;
		return (NULL);
	}
	reply(150, "Opening %s mode data connection for %s%s.",
	     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
	return (file);
}

/*
 * Tranfer the contents of "instr" to
 * "outstr" peer using the appropriate
 * encapsulation of the data subject
 * to Mode, Structure, and Type.
 *
 * NB: Form isn't handled.
 */
send_data(instr, outstr)
	FILE *instr, *outstr;
{
	register int c, cnt;

	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return;
	}
	switch (type) {

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n') {
				if (ferror(outstr))
					goto data_err;
				(void) putc('\r', outstr);
			}
			(void) putc(c, outstr);
		}
		fflush(outstr);
		transflag = 0;
		if (ferror(instr))
			goto file_err;
		if (ferror(outstr))
			goto data_err;
		reply(226, "Transfer complete.");
		return;

	case TYPE_I:
	case TYPE_L:
	    {      

		register char *bufp;
		register char *aligned_buf;
		register char *end_buf;
		register int bufsize = bufSize;

		int netfd = fileno(outstr);
		int filefd = fileno(instr);

		bufp = aligned_buf = bigBuf;
		end_buf = bufp + 2 * bufSize;

		while ((cnt = read(filefd, bufp, bufsize)) > 0) {
			if (send(netfd, bufp, cnt, 0) != cnt)
				break;
			byte_count += cnt;
			bufp += bufsize;
			if (bufp == end_buf)
				bufp = aligned_buf;
		}
		transflag = 0;
		if (cnt != 0) {
			if (cnt < 0)
				goto file_err;
			goto data_err;
		}
		reply(226, "Transfer complete.");
		return;
	    }

	default:
		transflag = 0;
		reply(550, "Unimplemented TYPE %d in send_data", type);
		return;
	}

data_err:
	transflag = 0;
	perror_reply(426, "Data connection");
	return;

file_err:
	transflag = 0;
	perror_reply(551, "Error on input file");
}

/*
 * Transfer data from peer to
 * "outstr" using the appropriate
 * encapulation of the data subject
 * to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled.
 */
receive_data(instr, outstr)
	FILE *instr, *outstr;
{
	register int c;
	int cnt, bare_lfs = 0;

	register char *aligned_buf = bigBuf;
	register int bufsize = bufSize;

	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return (-1);
	}
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		while ((cnt = read(fileno(instr), aligned_buf, bufsize)) > 0) {
                   /* write until all of buf is written or error is detected */
                   int bytes_written = 0, sum = 0;
                   do {
                      bytes_written = write(fileno(outstr), &aligned_buf[sum],
                                            (cnt - sum));
                      if (bytes_written < 0)  /* -1 returned and errno is set */
                         goto file_err;
                      sum += bytes_written;
                   } while (sum < cnt);
		   byte_count += cnt;
		}
		if (cnt < 0) /* read error */
			goto data_err;
		transflag = 0;
		return (0);

	case TYPE_E:
		reply(553, "TYPE E not implemented.");
		transflag = 0;
		return (-1);

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				if (ferror(outstr))
					goto data_err;
				if ((c = getc(instr)) != '\n') {
					(void) putc ('\r', outstr);
					if (c == '\0' || c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, outstr);
	contin2:	;
		}
		fflush(outstr);
		if (ferror(instr))
			goto data_err;
		if (ferror(outstr))
			goto file_err;
		transflag = 0;
		if (bare_lfs) {
			lreply(226, "WARNING! %d bare linefeeds received in ASCII mode",bare_lfs);
			printf("   File may not have transferred correctly.\r\n");
		}
		return (0);
	default:
		reply(550, "Unimplemented TYPE %d in receive_data", type);
		transflag = 0;
		return (-1);
	}

data_err:
	transflag = 0;
	perror_reply(426, "Data Connection");
	return (-1);

file_err:
	transflag = 0;
	perror_reply(452, "Error writing file");
	return (-1);
}

statfilecmd(filename)
	char *filename;
{
	char line[BUFSIZ];
	FILE *fin;
	int c;

	(void) sprintf(line, "/bin/ls -lA %s", filename);
	fin = ftpd_popen(line, "r");
	lreply(211, "status of %s:", filename);
	while ((c = getc(fin)) != EOF) {
		if (c == '\n') {
			if (ferror(stdout)){
				perror_reply(421, "control connection");
				(void) ftpd_pclose(fin);
				dologout(1);
				/* NOTREACHED */
			}
			if (ferror(fin)) {
				perror_reply(551, filename);
				(void) ftpd_pclose(fin);
				return;
			}
			(void) putc('\r', stdout);
		}
		(void) putc(c, stdout);
	}
	(void) ftpd_pclose(fin);
	reply(211, "End of Status");
}

statcmd()
{
	struct sockaddr_in *sin;
	u_char *a, *p;

	lreply(211, "%s FTP server status:", hostname, version);
	printf("     %s\r\n", version);
	printf("     Connected to %s", remotehost);
	if (isdigit(remotehost[0]))
		printf(" (%s)", inet_ntoa(his_addr.sin_addr));
	printf("\r\n");
	if (logged_in) {
		if (guest)
			printf("     Logged in anonymously\r\n");
		else
			printf("     Logged in as %s\r\n", pw->pw_name);
	} else if (askpasswd == 1)
		printf("     Waiting for password\r\n");
	else if (askpasswd == 0)
		printf("     Waiting for user name\r\n");
	printf("     TYPE: %s", typenames[type]);
	if (type == TYPE_A || type == TYPE_E)
		printf(", FORM: %s", formnames[form]);
	if (type == TYPE_L)
# if NBBY == 8
		printf(" %d", NBBY);
# else	/* ! NBBY == 8 */
		printf(" %d", bytesize);	/* need definition! */
# endif	/* NBBY == 8 */
	printf("; STRUcture: %s; transfer MODE: %s\r\n",
	    strunames[stru], modenames[mode]);
	if (data != -1)
		printf("     Data connection open\r\n");
	else if (pdata != -1) {
		printf("     in Passive mode");
		sin = &pasv_addr;
		goto printaddr;
	} else if (usedefault == 0) {
		printf("     PORT");
		sin = &data_dest;
printaddr:
		a = (u_char *) &sin->sin_addr;
		p = (u_char *) &sin->sin_port;
# define UC(b) (((int) b) & 0xff)
		printf(" (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
			UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
# undef UC
	} else
		printf("     No data connection\r\n");
	reply(211, "End of status");
}

fatal(s)
	char *s;
{
	reply(451, "Error in server: %s\n", s);
	reply(221, "Closing connection due to server error.");
	dologout(0);
	/* NOTREACHED */
}

/* VARARGS2 */
reply(n, fmt, p0, p1, p2, p3, p4, p5)
	int n;
	char *fmt;
{
	printf("%d ", n);
	printf(fmt, p0, p1, p2, p3, p4, p5);
	printf("\r\n");
	(void)fflush(stdout);
	if (debug) {
		syslog(LOG_DEBUG, "<--- %d ", n);
		syslog(LOG_DEBUG, fmt, p0, p1, p2, p3, p4, p5);
}
}

/* VARARGS2 */
lreply(n, fmt, p0, p1, p2, p3, p4, p5)
	int n;
	char *fmt;
{
	printf("%d- ", n);
	printf(fmt, p0, p1, p2, p3, p4, p5);
	printf("\r\n");
	(void)fflush(stdout);
	if (debug) {
		syslog(LOG_DEBUG, "<--- %d- ", n);
		syslog(LOG_DEBUG, fmt, p0, p1, p2, p3, p4, p5);
	}
}

ack(s)
	char *s;
{
	reply(250, "%s command successful.", s);
}

nack(s)
	char *s;
{
	reply(502, "%s command not implemented.", s);
}

/* ARGSUSED */
yyerror(s)
	char *s;
{
	char *cp;

	if (cp = index(cbuf,'\n'))
		*cp = '\0';
	reply(500, "'%s': command not understood.", cbuf);
	if (cp)
		*cp = '\n';
}

delete(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return;
	}
	if ((st.st_mode&S_IFMT) == S_IFDIR) {
		if (rmdir(name) < 0) {
			perror_reply(550, name);
			return;
		}
		goto done;
	}
	if (unlink(name) < 0) {
		perror_reply(550, name);
		return;
	}
done:
# ifdef NEW_LOGGING
	if (logging) {
		if (guest)
			syslog(LOG_INFO, "ANONYMOUS FTP: delete %s", name);
		else if (verbose)
			syslog(LOG_INFO, "FTP: delete %s", name);
	}
# endif	/* NEW_LOGGING */
	ack("DELE");
}

cwd(path)
	char *path;
{
	if (chdir(path) < 0)
		perror_reply(550, path);
	else {
# ifdef NEW_LOGGING
		if (logging) { 
			if (guest)
				syslog(LOG_INFO, "ANONYMOUS FTP: cwd %s", path);
			else if (verbose)
				syslog(LOG_INFO, "FTP: cwd %s", path);
		}
# endif	/* NEW_LOGGING */
		ack("CWD");
	}
}

makedir(name)
	char *name;
{
	if (mkdir(name, 0777) < 0)
		perror_reply(550, name);
	else {
# ifdef NEW_LOGGING
		if (logging) { 
			 if (guest)
			    syslog(LOG_INFO, "ANONYMOUS FTP: mkdir %s", name);
			 else if (verbose)
			    syslog(LOG_INFO, "FTP: mkdir %s", name);
		}
# endif	/* NEW_LOGGING */
		reply(257, "MKD command successful.");
	}
}

removedir(name)
	char *name;
{
	if (rmdir(name) < 0)
		perror_reply(550, name);
	else {
# ifdef NEW_LOGGING
	if (logging) {
		if (guest)
			syslog(LOG_INFO, "ANONYMOUS FTP: rmdir %s", name);
		else if (verbose)
			syslog(LOG_INFO, "FTP: rmdir %s", name);
	}
# endif	/* NEW_LOGGING */
		ack("RMD");
	}
}

pwd()
{
	char path[MAXPATHLEN + 1];
	extern char *getwd();

	if (getwd(path) == (char *)NULL)
                reply(550, "%s.", path);
	else
		reply(257, "\"%s\" is current directory.", path);
}

static struct stat from_stat, to_stat;

char *
renamefrom(name)
	char *name;
{

	if (stat(name, &from_stat) < 0) {
		perror_reply(550, name);
		return ((char *)0);
	}
	if (((from_stat.st_mode & S_IFMT) ==  S_IFREG) ||
	    ((from_stat.st_mode & S_IFMT) ==  S_IFDIR)) {
		reply(350, "File exists, ready for destination name.");
		return (name);
	}
	reply(550, "%s: Must be a regular file or a directory.", name);
	return ((char *)0);
}

void
renamecmd(from, to)
	char *from, *to;
{
        char line[BUFSIZ], *p = line;
	FILE *fin;
	int c, fd1, fd2, r;

	/* Check for and handle directory renames */
	if ((from_stat.st_mode & S_IFMT) == S_IFDIR) {
		mv_dir(from, to);
		return;
	}

	/*
	** If "to" exists, check to see that it is
	** a regular file and then delete it.
	*/
	if (stat(to, &to_stat) == 0) {
		if ((to_stat.st_mode & S_IFMT) != S_IFREG) {
			reply(550, "%s: Not a regular file.", to);
			return;
		}
		if ((from_stat.st_dev == to_stat.st_dev)
		   && (from_stat.st_ino == to_stat.st_ino)) {
			reply(550, "%s and %s are identical.", from, to);
			return;
		}
		if (unlink(to))	{
			perror_reply(550, to);
			return;
		}
	} else
		if (errno != ENOENT) {
			perror_reply(550, to);
			return;
		}

	/*
	** Try to rename it.
	*/
	if (rename(from,  to) == 0) {
# ifdef NEW_LOGGING
	if (logging) { 
		if (guest)
		    syslog(LOG_INFO, "ANONYMOUS FTP: rename %s %s", from, to);
		else if (verbose)
		    syslog(LOG_INFO, "FTP: rename %s %s", from, to);
	}
# endif	/* NEW_LOGGING */
		ack("RNTO");
		return;
	}
	if (errno != EXDEV) {
		perror_reply(550, from);
		return;
	}


	/*
	** Since the rename is accross devices, we must
	** copy all data in "from" to "to".
	*/
	if ((fd1 = open(from, O_RDONLY)) < 0) {
		perror_reply(550, from);
		return;
	}
	if ((fd2 = open(to, O_WRONLY | O_CREAT, from_stat.st_mode)) < 0) {
		perror_reply(550, to);
		close(fd1);
		return;
	}

	/* copy all data in "from" file to "to" file */
	{	char buf[BIGBUFSIZ];
		
		while ((r = read(fd1, buf, BIGBUFSIZ)) > 0)
			if (write(fd2, buf, r) < 0) {
				perror_reply(550, to);
				close(fd1);
				close(fd2);
				return;
			}
	}

	/* last return value should be 0, meaning EOF */
	if (r != 0) {
		perror_reply(550, from);
		close(fd1);
		close(fd2);
		return;
	}

	/* close the files and remove the "from" file */
	close(fd1);
	close(fd2);
	if (unlink(from) == 0) {
# ifdef NEW_LOGGING
	if (logging) { 
		if (guest)
		    syslog(LOG_INFO, "ANONYMOUS FTP: rename %s %s", from, to);
		else if (verbose)
		    syslog(LOG_INFO, "FTP: rename %s %s", from, to);
	}
# endif	/* NEW_LOGGING */
		ack("RNTO");
		return;
	}
	perror_reply(550, from);
	(void) unlink(to);
}

mv_dir(from, to)
char *from, *to;
{
	register char *p, *from_dir;
	char *from_parent, *to_parent, *parent_name();
	int savecode;

	if (stat(to, &to_stat) >= 0) {
		reply(550, "%s: Already exists.", to);
		return;
	}
	from_dir = p = from;
	while (*p)
		if (*p++ == '/' && *p)
			from_dir = p;
	if (!strcmp(from_dir, ".") || !strcmp(from_dir, "..") ||
	    from_dir[strlen(from_dir) - 1] == '/') {
		reply(550, "%s: Cannot rename.", from_dir);
		return;
	}
	from_parent = parent_name(from);
	if (stat(from_parent, &from_stat) < 0) {
		perror_reply(550, from_parent);
		return;
	}
	to_parent = parent_name(to);
	if (stat(to_parent, &to_stat) < 0) {
		perror_reply(550, to_parent);
		return;
	}
	if ((from_stat.st_dev != to_stat.st_dev) ||
	    (from_stat.st_ino != to_stat.st_ino)) {
		reply(550, "Cannot move directories; rename only.");
		return;
	}
	/*
	 * Not sure why this seteuid() is here as there should have
	 * been no change of the euid to require its resetting
	 */

# if defined(SecureWare) && defined(B1)
	if (! ISB1)
# endif /* defined(SecureWare) && defined(B1) */
		seteuid (pw->pw_uid);
	if (access(from_parent, 2) < 0) {
		reply(550, "%s: Cannot write.", from_parent);
		return;
	}
	if (rename(from, to) < 0) {
		perror_reply(550, "rename");
		return;
	}
# ifdef NEW_LOGGING
	if (logging) {
		if (guest)
		    syslog(LOG_INFO, "ANONYMOUS FTP: rename %s %s", from, to);
		else if (verbose)
		    syslog(LOG_INFO, "FTP: rename %s %s", from, to);
	}
# endif	/* NEW_LOGGING */
	ack("RNTO");
	return;
}

char *
parent_name(name)
register char *name;
{
	register int c;
	register char *p, *q;
	static char buf[128];

	p = q = buf;
	while (c = *p++ = *name++)
		if (c == '/')
			q = p-1;
	if (q == buf && *q == '/')
		q++;
	*q = 0;
	return buf[0] ? buf : ".";
}

dolog(sin)
	struct sockaddr_in *sin;
{
	struct hostent *hp = gethostbyaddr((char *)&sin->sin_addr,
		sizeof (struct in_addr), AF_INET);
	time_t t, time();
	extern char *ctime();

	if (hp)
		(void) strncpy(remotehost, hp->h_name, sizeof (remotehost));
	else
		(void) strncpy(remotehost, inet_ntoa(sin->sin_addr),
		    sizeof (remotehost));
# ifdef SETPROCTITLE
	sprintf(proctitle, "%s: connected", remotehost);
	setproctitle(proctitle);
# endif /* SETPROCTITLE */

	if (logging) {
		t = time((time_t *) 0);
		syslog(LOG_INFO, "connection from %s at %s",
		    remotehost, ctime(&t));
	}
}

/*
 * Record logout in wtmp file
 * and exit with supplied status.
 */
dologout(status)
	int status;
{
	if (logged_in) {
	        if (logging)
		    syslog(LOG_INFO, "User %s logged out", pw->pw_name);
# if defined(SecureWare) && defined(B1)
		if (ISB1) {
			if (ftpd_forcepriv(SEC_ALLOWDACACCESS) == 0)
				syslog(LOG_ERR, 
			  "ftpd: requires allowdacaccess potential privilege");
			if (ftpd_forcepriv(SEC_ALLOWMACACCESS) == 0)
				syslog(LOG_ERR, 
			  "ftpd: requires allowmacaccess potential privilege");
		} else
# endif /* defined(SecureWare) && defined(B1) */
		(void) seteuid((uid_t)0); /* Executed if !ISB1 or      */
					  /* SecureWare&B1 not defined */
		logwtmp(ttyline, "", "");
	}
	/* beware of flushing buffers after a SIGPIPE */
# if defined(SecureWare) && defined(B1) 
	if (ISB1) {
		(int) ftpd_restorepriv();
	}
# endif /* defined(SecureWare) && defined(B1) */

	_exit(status);
}

void
myoob()
{
	char *cp;

	/* only process if transfer occurring */
	if (!transflag)
		return;
	cp = tmpline;
	if (getline(cp, 7, stdin) == NULL) {
		reply(221, "You could at least say goodbye.");
		dologout(0);
	}
# ifndef SO_OOBINLINE
	cp++;
# endif	/* not SO_OOBINLINE */
	upper(cp);
	if (strcmp(cp, "ABOR\r\n") == 0) {
		tmpline[0] = '\0';
		reply(426, "Transfer aborted. Data connection closed.");
		reply(226, "Abort successful");
		longjmp(urgcatch, 1);
	}
	if (strcmp(cp, "STAT\r\n") == 0) {
		if (file_size != (off_t) -1)
			reply(213, "Status: %lu of %lu bytes transferred",
			    byte_count, file_size);
		else
			reply(213, "Status: %lu bytes transferred", byte_count);
	}
}

/*
 * Note: a response of 425 is not mentioned as a possible response to
 * 	the PASV command in RFC959. However, it has been blessed as
 * 	a legitimate response by Jon Postel in a telephone conversation
 *	with Rick Adams on 25 Jan 89.
 */
passive()
{
	int len;
	register char *p, *a;

	pdata = socket(AF_INET, SOCK_STREAM, 0);
	if (pdata < 0) {
		perror_reply(425, "Can't open passive connection");
		return;
	}
	pasv_addr = ctrl_addr;
	pasv_addr.sin_port = 0;

# if defined(SecureWare) && defined(B1)
	if (ISB1) {
		if (ftpd_forcepriv(SEC_REMOTE) == 0)
			syslog(LOG_ERR, 
			      "ftpd: requires remote potential privilege");
	} else
# endif /* defined(SecureWare) && defined(B1) */
	(void) seteuid((uid_t)0); /* Executed if !ISB1 or SecureWare&B1 */
				  /* not defined */

	if (bind(pdata, (struct sockaddr *)&pasv_addr, sizeof(pasv_addr)) < 0) {
# if defined(SecureWare) && defined(B1)
	if (ISB1) {
		(int) ftpd_restorepriv();
	} else
# endif /* defined(SecureWare) && defined(B1) */
		(void) seteuid((uid_t)pw->pw_uid); /*Executed if !ISB1 or     */
					 	  /*SecureWare&B1 not defined */
		goto pasv_error;
	}
# if defined(SecureWare) && defined(B1)
	if (ISB1) {
		(int) ftpd_restorepriv();
	} else
# endif /* defined(SecureWare) && defined(B1) */
	(void) seteuid((uid_t)pw->pw_uid); /* Executed if !ISB1 or      */
					   /* SecureWare&B1 not defined */
	len = sizeof(pasv_addr);
	if (getsockname(pdata, (struct sockaddr *) &pasv_addr, &len) < 0)
		goto pasv_error;
	if (listen(pdata, 1) < 0)
		goto pasv_error;
	a = (char *) &pasv_addr.sin_addr;
	p = (char *) &pasv_addr.sin_port;

# define UC(b) (((int) b) & 0xff)

	reply(227, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", UC(a[0]),
		UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
	return;

pasv_error:
	(void) close(pdata);
	pdata = -1;
	perror_reply(425, "Can't open passive connection");
	return;
}

/*
 * Generate unique name for file with basename "local".
 * The file named "local" is already known to exist.
 * Generates failure reply on error.
 */
char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	struct stat st;
	char *cp = rindex(local, '/');
	int count = 0;

	if (cp)
		*cp = '\0';
	if (stat(cp ? local : ".", &st) < 0) {
		perror_reply(553, cp ? local : ".");
		return((char *) 0);
	}
	if (cp)
		*cp = '/';
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	for (count = 1; count < 100; count++) {
		(void) sprintf(cp, "%d", count);
		if (stat(new, &st) < 0)
			return(new);
	}
	reply(452, "Unique file name cannot be created.");
	return((char *) 0);
}

/*
 * Format and send reply containing system error number.
 */
perror_reply(code, string)
	int code;
	char *string;
{
	if (errno < sys_nerr)
		reply(code, "%s: %s.", string, sys_errlist[errno]);
	else
		reply(code, "%s: unknown error %d.", string, errno);
}

static char *onefile[] = {
	"",
	0
};

send_file_list(whichfiles)
	char *whichfiles;
{
	struct stat st;
	DIR *dirp = NULL;
# ifdef hpux
	struct dirent *dir;
# else	/* ! hpux */
	struct direct *dir;
# endif /* hpux */
	FILE *dout = NULL;
	register char **dirlist, *dirname;
	int simple = 0;
	char *strpbrk();

	if (strpbrk(whichfiles, "~{[*?") != NULL) {
		extern char **glob(), *globerr;

		globerr = NULL;
		dirlist = glob(whichfiles);
		if (globerr != NULL) {
			reply(550, globerr);
			return;
		} else if (dirlist == NULL) {
			errno = ENOENT;
			perror_reply(550, whichfiles);
			return;
		}
	} else {
		onefile[0] = whichfiles;
		dirlist = onefile;
		simple = 1;
	}

	if (setjmp(urgcatch)) {
		transflag = 0;
		return;
	}
	while (dirname = *dirlist++) {
		if (stat(dirname, &st) < 0) {
			/*
			 * If user typed "ls -l", etc, and the client
			 * used NLST, do what the user meant.
			 */
			if (dirname[0] == '-' && *dirlist == NULL &&
			    transflag == 0) {
				retrieve("/bin/ls %s", dirname);
				return;
			}
			perror_reply(550, whichfiles);
			if (dout != NULL) {
				(void) fclose(dout);
				transflag = 0;
				data = -1;
				pdata = -1;
			}
			return;
		}

		if ((st.st_mode&S_IFMT) == S_IFREG) {
			if (dout == NULL) {
				dout = dataconn("file list", (off_t)-1, "w");
				if (dout == NULL)
					return;
				transflag++;
			}
			fprintf(dout, "%s\r\n", dirname);
			byte_count += strlen(dirname) + 1;
			continue;
		} else if ((st.st_mode&S_IFMT) != S_IFDIR)
			continue;

		if ((dirp = opendir(dirname)) == NULL)
			continue;

		while ((dir = readdir(dirp)) != NULL) {
			char nbuf[MAXPATHLEN];

			if (dir->d_name[0] == '.' && dir->d_namlen == 1)
				continue;
			if (dir->d_name[0] == '.' && dir->d_name[1] == '.' &&
			    dir->d_namlen == 2)
				continue;

			sprintf(nbuf, "%s/%s", dirname, dir->d_name);

			/*
			 * We have to do a stat to insure it's
			 * not a directory or special file.
			 */
			if (simple || (stat(nbuf, &st) == 0 &&
			    (st.st_mode&S_IFMT) == S_IFREG)) {
				if (dout == NULL) {
					dout = dataconn("file list", (off_t)-1,
						"w");
					if (dout == NULL)
						return;
					transflag++;
				}
				if (nbuf[0] == '.' && nbuf[1] == '/')
					fprintf(dout, "%s\r\n", &nbuf[2]);
				else
					fprintf(dout, "%s\r\n", nbuf);
				byte_count += strlen(nbuf) + 1;
			}
		}
		(void) closedir(dirp);
	}

	if (dout == NULL)
		reply(550, "No files found.");
	else if (ferror(dout) != 0)
		perror_reply(550, "Data connection");
	else
		reply(226, "Transfer complete.");

	transflag = 0;
	if (dout != NULL)
		(void) fclose(dout);
	data = -1;
	pdata = -1;
}

# ifdef SETPROCTITLE

# 	ifdef hpux
# 		include <sys/pstat.h>
# 	endif	/* hpux */

/*
 * clobber argv so ps will show what we're doing.
 * (stolen from sendmail)
 * warning, since this is usually started from inetd.conf, it
 * often doesn't have much of an environment or arglist to overwrite.
 */

/*VARARGS1*/
setproctitle(fmt, a, b, c)
char *fmt;
{
	register char *p, *bp, ch;
	register int i;
	char buf[BUFSIZ], pbuf[BUFSIZ];

	(void) sprintf(buf, fmt, a, b, c);

# 	ifdef hpux
	Argv[1] = buf;

	/* clean out CR's and LF's */
	bp = buf;
	while (*bp) {
		if (*bp == '\n' | *bp == '\r')
			*bp = ' ';
		bp++;
	}
# 		ifdef PSTAT_SETCMD
	sprintf(pbuf, "%s %s", Argv[0], Argv[1]);
	pstat(PSTAT_SETCMD, pbuf);
# 		else	/* ! PSTAT_SETCMD */
	/* pre-8.0 pstat */
	pstat_set_command(Argv);
# 		endif /* PSTAT_SETCMD */

# 	else	/* ! hpux */
	/* make ps print our process name */
	p = Argv[0];
	*p++ = '-';

	i = strlen(buf);
	if (i > LastArgv - p - 2) {
		i = LastArgv - p - 2;
		buf[i] = '\0';
	}
	bp = buf;
	while (ch = *bp++)
		if (ch != '\n' && ch != '\r')
			*p++ = ch;
	while (p < LastArgv)
		*p++ = ' ';
# 	endif	/* hpux */
}
# endif /* SETPROCTITLE */
