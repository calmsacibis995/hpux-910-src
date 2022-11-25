/*
 * Copyright (c) 1983 The Regents of the University of California.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)$Header: rlogin.c,v 1.31.109.3 92/09/21 11:51:29 ash Exp $";
#endif

/*
 * rlogin - remote login
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ptyio.h>

#include <netinet/in.h>

#include <stdio.h>
#include <termio.h>
#include <termios.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>

#ifdef SecureWare
#include <sys/security.h>
#endif /* SecureWare */

#ifdef hpux
/*
 * A common usr1 signal handler for both the reader() and writer() 
 * processes will avoid a race condition.
 */
#if defined(SIGTSTP) && defined(SIGWINCH)
#define COMMONUSR1
#endif /* SIGTSTP && SIGWINCH */
#endif /* hpux */


#include <netdb.h>
#include <fcntl.h>
#ifdef	SIGTSTP
#include <unistd.h>
#include <sys/bsdtty.h>
#endif	SIGTSTP
#ifdef NLS
#include <nl_ctype.h>
#endif NLS


#ifndef TIOCPKT_WINDOW
#define TIOCPKT_WINDOW 0x80
#endif /* TIOCPKT_WINDOW */

#ifdef SecureWare
#include <audnetd.h>
char *rhost = (char *)0, *rusr = (char *)0, *lusr = (char *)0;
uid_t	ruid = (uid_t) -1, luid = (uid_t) -1;
u_long	raddr = (u_long) -1, laddr = (u_long) -1;

/*
**    Auditing disabled because audit_daemon routine not yet available.
**    To re-enable auditing, remove the #ifdef notdef and the code
**    from the #else to the #endif.
**
**    To make auditing work, either give this program the potential
**    privileges enabled immediately below, or else make it part of
**    a protected subsystem, and remove the enablepriv() calls.
*/

#ifdef notdef
#ifdef B1
#define enable_priv_for_audit()	\
		if (ISB1) { \
			enablepriv(SEC_ALLOWDACACCESS); \
			enablepriv(SEC_ALLOWMACACCESS); \
			enablepriv(SEC_WRITE_AUDIT); \
		}
#define disable_priv_for_audit()	\
		if (ISB1) { \
			disablepriv(SEC_ALLOWDACACCESS); \
			disablepriv(SEC_ALLOWMACACCESS); \
			disablepriv(SEC_WRITE_AUDIT); \
		}
#else /* not B1 */
#define enable_priv_for_audit()	
#define disable_priv_for_audit()	
#endif /* not B1 */

#define AUDIT_SUCCESS(s)	\
	if (ISSECURE) { \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_SUCCESS, NA_VALI_RUSEROK, \
			raddr, rhost, ruid, rusr, luid, lusr, NA_EVNT_IDENT, \
			NA_MASK_RADDR|NA_MASK_RUID|NA_MASK_LUID, (s)); \
		disable_priv_for_audit(); \
	}

#else /* not notdef */
#define AUDIT_SUCCESS(s)
#endif /* not notdef */

#else /* not SecureWare */
#define AUDIT_SUCCESS(s)
#endif /* not SecureWare */

#undef BUFSIZ
#define BUFSIZ 1024

char	*malloc(), *getenv(), *strrchr(); 
struct	passwd *getpwuid();
char	*name = (char *) 0;
int	rem;
int ppid;
char	cmdchar = '~';
int	eight = 1;
char	*speeds[] =
	{ "0", "50", "75", "110", "134", "150", "200", "300", "600",
	"900", "1200", "1800", "2400", "3600", "4800", "7200", "9600",
	"19200", "38400", "EXTA", "EXTB" };

char	term[64] = "network";
extern	int errno;
extern	char *sys_errlist[];
extern	char **environ;
char	**vp;
void	lostpeer();
int	segv();
void	catchild(), writer(), shell_escape();
void	echo(), mode();
#ifdef SIGTSTP
void	suspend();
#endif SIGTSTP

/* global so that can get defmodes before rcmd call */
struct	termio defmodes;
char	deferase, defkill;

long sigblock(), sigsetmask();
#define TRUE 1
#define FALSE 0
char oob = '\0';	/* OOB byte; set by recv_oob; used by do_oob. */
int flushing = FALSE;	/* true if a FLUSHWRITE packet was sent by rlogind: */
			/* set by get_oob and urg; */
			/* used by do_oob; reset by reader. */
int stop = TIOCPKT_DOSTOP;
			/* flow control state: */
			/* set by get_oob and urg; */
			/* used by do_oob and mode; */
			/* reset by do_oob. */
int urgent = FALSE;	/* true if there is OOB data blocked by a */
			/* full socket input buffer: */
			/* set and reset by urg and recv_oob; */
			/* used by reader. */
#ifdef SIGTSTP
int suspended = FALSE;	/* set by usr1; reset by usr2; used by do_oob. */
#endif SIGTSTP

#ifdef SIGWINCH
int dosigwinch = 0;
int sigwinch();
#endif /* SIGWINCH */

#ifdef TIOCSWINSZ
#define get_window_size(fd,wp) ioctl(fd,TIOCGWINSZ,wp)
struct winsize winsize;
#endif /* TIOCSWINSZ */

main(argc, argv)
	int argc;
	char **argv;
{
	char *host, *cp; 
	struct termio ttyb;
	struct passwd *pwd;
	struct servent *sp;
	int uid;
	struct sigvec vec,ovec;
	

#ifdef SecureWare
	if (ISSECURE)
		set_auth_parameters(argc, argv);
#endif /* SecureWare */
	{
		int vecsize = 0, envsize = 0, i = 0;

		vp = environ;
		while (vp != (char **) NULL && *vp != (char *) NULL ) 
		{
			vecsize++;
			envsize += strlen(*vp) + 1;
			vp++;
		}
		vp = (char **)malloc((vecsize + 1) * sizeof (char *));
		cp = malloc(envsize);
		
		while (i < vecsize)
		{
			vp[i] = cp;
			strcpy(vp[i], environ[i]);
			while (*cp != (char) NULL)
				cp++;
			cp++;
			i++;
		}
	}
	
	cleanenv(&environ,
		"LANG", "LANGOPTS", "NLSPATH",
		"LOCALDOMAIN", "HOSTALIASES", 
		"TERM", "SHELL", 0);

#ifdef NLS
	nl_init(getenv("LANG"));
#endif NLS

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		initprivs();
		if (!forcepriv(SEC_REMOTE)) {
			fprintf(stderr,
		"rlogin: This program requires remote potential privilege\n");
			exit(1);
		}
		disablepriv(SEC_REMOTE);
	} else
#endif /* SecureWare && B1 */
	{
		/*
		**	if not running as superuser, no reason to go any farther
		**	since rlogin will fail anyway when rresvport is called
		*/
		if (geteuid() != 0) {
			fprintf(stderr,
		       "rlogin: This program requires super user privileges\n");
			exit(1);
		}
	}

	host = strrchr(argv[0], '/');
	if (host)
		host++;
	else
		host = argv[0];
	argv++, --argc;
	/*
	**	if rlogin is invoked as anything but "rlogin", it will use
	**	the invocation name as the host name to log in to (clever)
	*/
	if (!strcmp(host, "rlogin"))
		host = *argv++, --argc;
another:
	/*
	**	different login name on the remote host 
	*/
	if (argc > 0 && !strcmp(*argv, "-l")) {
		argv++, argc--;
		if (argc == 0)
			goto usage;
		name = *argv++; argc--;
		goto another;
	}
	/*
	**	change the rlogin escape character from '~' to whatever
	*/
	if (argc > 0 && !strncmp(*argv, "-e", 2)) {
		cmdchar = argv[0][2];
		argv++, argc--;
		goto another;
	}
	/*
	**	force 7-bit data path -- default is 8-bits
	*/
	if (argc > 0 && !strcmp(*argv, "-7")) {
		eight = 0;
		argv++, argc--;
		goto another;
	}
	/*
	**	this is a no-op for compatibility -- rlogin uses 8-bit
	**	data by default, so this option will do nothing.
	*/
	if (argc > 0 && !strcmp(*argv, "-8")) {
		eight = 1;
		argv++, argc--;
		goto another;
	}

	if (host == 0)
		goto usage;
#ifdef SecureWare
	rhost = host;
#endif /* SecureWare */
	if (argc > 0)
		goto usage;
	/*
	**	get the userid of the user invoking rlogin
	*/
	pwd = getpwuid(getuid());
	if (pwd == (struct passwd *) NULL) {
		fprintf(stderr,
		    "rlogin: There is no passwd entry for you (user id %d)\n",
		    getuid());
		exit(1);
	}
#ifdef SecureWare
	lusr = pwd->pw_name;
#endif /* SecureWare */
	sp = getservbyname("login", "tcp");
	if (sp == 0) {
		fprintf(stderr, "rlogin: login/tcp: Unknown service\n");
		exit(2);
	}
	/*
	**	find the user's TERM info, if possible; if not, use "network"
	*/
	cp = getenv("TERM");
	if (cp)
		strcpy(term, cp);
	if (ioctl(fileno(stdin), TCGETA, &ttyb) == 0)
	{
		(void)strcat(term, "/");
		(void)strcat(term, speeds[ttyb.c_cflag & CBAUD]);
	}
	
	/**** moved before rcmd call so that if get a SIGPIPE in rcmd **/
	/**** we will have the defmodes set already. ***/
	(void)ioctl(fileno(stdin), TCGETA, &defmodes);

#ifdef TIOCSWINSZ
	get_window_size(0,&winsize);
#endif /* TIOCSWINSZ */
	
	/*** reset signal mask inherited from parent ***/
	sigsetmask(0);
	vec.sv_handler = lostpeer;
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	(void) sigvector( SIGPIPE, &vec, (struct sigvec *) 0);

#if defined(SecureWare) && defined(B1)
	/*
	**  rcmd requires REMOTE privilege
	**  because it binds to a privileged port 
	*/
	if (ISB1) {
		forcepriv(SEC_REMOTE);
	}
#endif /* SecureWare && B1 */
	/*
	**	set up the connection to the remote host -- uses a single
	**	socket for stdin/stdout/stderr, so no secondary is requested
	*/
        rem = rcmd(&host, sp->s_port, pwd->pw_name,
	    name ? name : pwd->pw_name, term, 0);
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		disablepriv(SEC_REMOTE);
	}
#endif /* SecureWare && B1 */

        if (rem < 0)
                exit(1);
#ifdef SecureWare
	rusr = name ? name : pwd->pw_name;
	AUDIT_SUCCESS("Certified identity of local user");
#endif /* SecureWare */
	/*
	**	setuid to the calling user -- we've done all we need to as root
	*/
	uid = getuid();
	if (setuid(uid) < 0) {
		perror("rlogin: setuid");
		exit(1);
	}
	doit();
	/*NOTREACHED*/
usage:
	fprintf(stderr,
	    "Usage: rlogin host [ -ex ] [ -l username ] [ -8 ]\n");
	exit(1);
}

#ifdef SIGTSTP

/*
**  P -- wait on mutual exclusion semaphore
**
**	Parameters:
**		pid -- process id of the calling program; this byte of the
**			file associated with stdout is locked, providing
**			an advisory lock for this process and its children
**			only.
**
**	Returns:
**		if successful, 0;
**		if unsuccessful, -1.
**
**	Side Effects:
**		the calling process will sleep until it can lock the byte;
**		one byte of the file associated with stdout is then locked.
**		
*/

int
P(pid)
int pid;
{
    int save_errno = errno;
    struct flock lockdes;

    lockdes.l_type = F_WRLCK;
    lockdes.l_whence = 0;
    lockdes.l_start = pid;
    lockdes.l_len = 1;
    lockdes.l_pid = 0;


    if (fcntl(fileno(stdout), F_SETLKW, &lockdes) == -1) {
        perror("rlogin: P: fcntl(F_SETLKW)");
	errno = save_errno;
	return(-1);
    } else {
        errno = save_errno;
        return(0);
    }
}

/*
**  V -- release mutual exclusion semaphore
**
**	Parameter:
**		pid --  process id of the calling program; the offset of the
**			byte to be unlocked.
**
**	Returns:
**		if successful, 0;
**		if unsuccessful, -1.
**
**	Side Effects:
**		the requested byte of the file associated with stdout will
**		be unlocked.
**		
*/

V(pid)
int pid;
{
    int save_errno = errno;
    struct flock lockdes;

    lockdes.l_type = F_UNLCK;
    lockdes.l_whence = 0;
    lockdes.l_start = pid;
    lockdes.l_len = 1;
    lockdes.l_pid = 0;


    if (fcntl(fileno(stdout), F_SETLK, &lockdes) == -1) {
        perror("rlogin: V: fcntl(F_SETLK)");
	errno = save_errno;
	return(-1);
    } else {
        errno = save_errno;
        return(0);
    }
}
#endif SIGTSTP

#define CRLF "\r\n"

int	child;
void	done();
#ifdef SIGWINCH
void writeroob();
#endif /* SIGWINCH */
#ifdef COMMONUSR1
void usr1handler();
void usr1();
#endif /* COMMONUSR1 */
#ifdef	SIGTSTP
struct	ltchars	defltc;
struct	ltchars noltc =	{ -1, -1, -1, -1, -1, -1 };
#endif	SIGTSTP

doit()
{
	struct sigvec vec;

#ifdef	SIGTSTP
	(void)ioctl(fileno(stdin), TIOCGLTC, &defltc);
#endif	SIGTSTP
	deferase = defmodes.c_cc[VERASE];
	defkill = defmodes.c_cc[VKILL];

	/*
	**	ignore INT; and QUIT; default handling for HUP
	*/
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	vec.sv_handler = SIG_IGN;
	(void) sigvector(SIGINT, &vec, (struct sigvec *) 0);
	(void) sigvector(SIGQUIT, &vec, (struct sigvec *) 0);

	vec.sv_handler = SIG_DFL;
	(void) sigvector(SIGHUP, &vec, (struct sigvec *) 0);
#ifdef hpux
#ifdef COMMONUSR1
	vec.sv_handler = usr1handler;
	(void) sigvector(SIGUSR1, &vec, (struct sigvec *) 0);
#endif /* COMMONUSR1 */
#endif /* hpux */

#if defined(SecureWare) && defined(B1)
	/*
	**  lest least privilege hamper the privileged user's fork()
	*/
	if (ISB1) {
		enablepriv(SEC_LIMIT);
	}
#endif /* SecureWare && B1 */
	child = fork();
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		disablepriv(SEC_LIMIT);
	}
#endif /* SecureWare && B1 */
	if (child == -1) {
		perror("rlogin: fork");
		done();
	}
	mode(1);
	if (child == 0) {
		reader();
		sleep(1);
		prf("\007Connection closed.");
		exit(3);
	}
	vec.sv_handler = done;
	(void) sigvector(SIGCLD, &vec, (struct sigvec *) 0);
#ifdef SIGWINCH
#ifndef COMMONUSR1
	vec.sv_handler = writeroob;
	(void) sigvector(SIGUSR1, &vec, (struct sigvec *) 0);
#endif /* !COMMONUSR1 */
#endif /* SIGWINCH */
	writer();
	prf("Closed connection.");
	done();
}

int interrupted = FALSE;

void
interrupt(sig)
int sig;
{
	interrupted = TRUE;
}

void
done(sig)
int sig;
{
	int status;

	mode(0);
	if (child > 0 && kill(child, SIGKILL) >= 0)
		wait(&status);
	exit(0);
}

#ifdef hpux
#ifdef COMMONUSR1
/*  
 *  one usr1 handler for both reader and writer process. This avoids
 *  a race condition when one process's sends a usr1 before the other
 *  one has a chance to set its usr1 signal handler
 */
void 
usr1handler()
{
    if (child == 0)
	usr1();	 	/*  Calls the reader's handler  */
    else
	writeroob();	/*  Calls the writer's handler  */
}
#endif /* COMMONUSR1 */
#endif /* hpux */

#ifdef SIGWINCH
void
 writeroob()
{

    if (dosigwinch == 0) {
        sendwindow();
        (void) signal(SIGWINCH, sigwinch);
    }
    dosigwinch = 1;
}

sigwinch()
{
    struct winsize ws;

    if (dosigwinch && get_window_size(0, &ws) == 0 &&
        bcmp(&ws, &winsize, sizeof (ws))) {
        winsize = ws;
        sendwindow();
    }
}

/*
 * Send the window size to the server via the magic escape
 */
sendwindow()
{
    char obuf[4 + sizeof (struct winsize)];
    struct winsize *wp = (struct winsize *)(obuf+4);

    obuf[0] = 0377;
    obuf[1] = 0377;
    obuf[2] = 's';
    obuf[3] = 's';
    wp->ws_row = htons(winsize.ws_row);
    wp->ws_col = htons(winsize.ws_col);
    wp->ws_xpixel = htons(winsize.ws_xpixel);
    wp->ws_ypixel = htons(winsize.ws_ypixel);
    (void) write(rem, obuf, sizeof(obuf));
}
#endif /* SIGWINCH */


/*
 * writer: write to remote: 0 -> line.
 * ~.	terminate
 * ~^Z	suspend rlogin process.
 * ~^Y  suspend rlogin process, but leave reader alone.
 */

char *backspace = "\010\040\010";	/* backspace, space, backspace */

void
writer()
{
	char c;
	char b[600]; /* buffer for command for shell escape, beginning with "~!";
		     ** shell_escape() should be passed &b[2]
		     */
	int bss;		/* number of characters to output for backspace */
	register n;
	register bol = TRUE;	/* at beginning of line ? */
	register local = -1;	/* index of last character put in b;
				** if local >= 0, echoing locally
				*/
	int wrotecmdchar = FALSE;
	struct sigvec vec;
	void interrupt();

	memset(b, '\0', sizeof(b));
	bss = defmodes.c_lflag & ECHOE ? 3 : 1;
	for (;;) {
		n = read(0, &c, 1);
		if (n <= 0) {
			if (n < 0 && errno == EINTR)
				continue;
			break;
		}
		/*
		 * If we're at the beginning of the line
		 * and recognize a command character, then
		 * we echo locally.  Otherwise, characters
		 * are echo'd remotely.  If the command
		 * character is doubled, this acts as a 
		 * force and local echo is suppressed.
		 */
		if (bol) {
			wrotecmdchar = bol = FALSE;
			if (c == cmdchar) {
				b[++local] = c;
				continue;
			}
		} else if (local == 0) {
			/*
			**	have seen cmdchar;
			**	looking for the rest of the escape sequence
			*/
			if (c == '.' || c == defmodes.c_cc[VEOF]) {
				/*
				**	exit writer, terminate rlogin
				*/
				echo(c, wrotecmdchar);
				break;
#ifdef SIGTSTP
			} else if (c == defltc.t_suspc || c == defltc.t_dsuspc) {
				/*
				**	suspend
				*/
				local = -1;
				bol = TRUE;
				echo(c, wrotecmdchar);
				suspend(c);
				bss = defmodes.c_lflag & ECHOE ? 3 : 1;
				continue;
#endif SIGTSTP
			} else if (c == '!') {
#ifdef SIGTSTP
				register char suspc = defltc.t_suspc;
				register char dsuspc = defltc.t_dsuspc;
#endif SIGTSTP

				/*
				**	collect command for shell escape
				*/
				b[++local] = c;
				write(1, b, 2);
				wrotecmdchar = TRUE;
#ifdef SIGTSTP
				/*
				**	save and restore the suspend characters
				**	so that suspend is not active while
				**	collecting the shell command
				*/
				defltc.t_suspc = defltc.t_dsuspc = 0377;
#endif SIGTSTP
				/*
				**	temporarily catch INT and QUIT
				**	so user may abort shell escape input
				*/
				vec.sv_mask = 0;
				vec.sv_onstack = 0;
				vec.sv_handler = interrupt;
				(void) sigvector(SIGINT, &vec, (struct sigvec *) 0);
				(void) sigvector(SIGQUIT, &vec, (struct sigvec *) 0);

				/*
				**	restore default tty modes so that tty driver
				**	or NLIO handles user input
				*/
				mode(0);

				/*
				**	collect the shell escape command, if any.
				*/
				read(0, &b[2], sizeof(b) - 2); 

#ifdef SIGTSTP
				defltc.t_suspc = suspc;
				defltc.t_dsuspc = dsuspc;
#endif SIGTSTP
				vec.sv_handler = SIG_IGN;
				(void) sigvector(SIGINT, &vec, (struct sigvec *) 0);
				(void) sigvector(SIGQUIT, &vec, (struct sigvec *) 0);
				if (!interrupted) {
				    mode(0);
				    shell_escape(&b[2]);
				}
				else {
				    interrupted = FALSE;
				}
				mode(1);
				memset(b, '\0', sizeof(b));
				local = -1;
				bol = TRUE;
				continue;
			} else if (c == deferase || c == defkill || c == defmodes.c_cc[VINTR]) {
				/*
				**	kill escape sequence
				*/
				b[local--] = '\0';
				bol = TRUE;
				if (wrotecmdchar)
					write(1, backspace, bss);
				continue;
			} else {
				/*
				**	if not escaping a cmdchar,
				**	send cmdchar to remote
				*/
				local = -1;
				wrotecmdchar = FALSE;
				if (c != cmdchar) {
					char cmdchar7 = eight ? cmdchar : cmdchar & 0177;

					write(rem, &cmdchar7, 1);
				}
			}
		}
		if (eight == 0)
			c &= 0177;
		if (write(rem, &c, 1) == 0) {
			prf("line gone");
			break;
		}
		bol = c == defkill || c == defmodes.c_cc[VEOF] ||
		    c == defmodes.c_cc[VINTR] ||
#ifdef SIGTSTP
		    c == defltc.t_suspc || c == defltc.t_dsuspc ||
#endif SIGTSTP
		    c == '\r' || c == '\n';
	}
}

void
echo(c, wrotecmdchar)
register char c;
int wrotecmdchar;
{
	char buf[8];
	register char *p = buf;

	c &= 0177;
	if (!wrotecmdchar)
		*p++ = cmdchar;
	if (c < ' ') {
		*p++ = '^';
		*p++ = c + '@';
	} else if (c == 0177) {
		*p++ = '^';
		*p++ = '?';
	} else
		*p++ = c;
	*p++ = '\r';
	*p++ = '\n';
	write(1, buf, p - buf);
}

#ifdef SIGTSTP
void
suspend(cmdc)
char cmdc;
{
	struct sigvec vec;

	(void)write(fileno(stdout),CRLF,sizeof(CRLF));
	/*
	**  send child (reader process) SIGUSR1
	**  (and wait for it to respond)
	**  so that it won't catch a SIGURG and
	**  try to do ioctls on the terminal while
	**  it's supposed to be asleep
	*/
	kill(child, SIGUSR1);
	P(getpid());
	mode(0);
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	vec.sv_handler = SIG_DFL;
	(void) sigvector(SIGCLD, &vec, 0);
	/*
	**	send the SIGTSTP to either the whole
	**	process group (includes the reader),
	**	or just this pid (leaves reader alone)
	*/
	kill(cmdc == defltc.t_suspc ? 0 : getpid(), SIGTSTP);
	/*
	**	reget parameters in case they were
	**	changed; this allows us to suspend
	**	rlogin, do an stty, resume rlogin, and
	**	exit, retaining the modes we set while
	**	suspended; not such a big deal, but...
	*/
	(void)ioctl(fileno(stdin), TCGETA, (char *)&defmodes);
	(void)ioctl(fileno(stdin), TIOCGLTC, &defltc);
	vec.sv_handler = done;
	(void) sigvector(SIGCLD, &vec, 0);
	mode(1);
	/*
	**	send child (reader process) SIGUSR2
	**	so that it may start catching SIGURG
	**	and doing ioctls on the terminal again
	*/
	kill(child, SIGUSR2);
#ifdef SIGWINCH
	sigwinch();
#endif /* SIGWINCH */
}
#endif SIGTSTP

/*
**  recv_oob(oob_ptr):  receive OOB byte if possible.  recv_oob will try to
**			recv up to MAX_RECV_OOB times;  if it gets EWOULDBLOCK
**			every time, the assumption is that the socket input
**			buffer is full, and in-stream reads must be done to
**			drain it before the OOB byte may be recv'ed.
**
**	Parameters:
**		oob_ptr: pointer to character in which to put the out of band
**			 byte read.
**
**	Returns:
**		if successful, 0;
**		if unsuccessful, the errno resulting from an unsuccessful
**		    recv( ... MSG_OOB).
**
**	Side Effects:
**		recv's one byte out of band;
**		modifies the char pointed to by oob_ptr;
**		may change errno.
**		
*/
#define MAX_RECV_OOB 16		/* empirically enough */
int
recv_oob(oob_ptr)
char *oob_ptr;
{
    int i, rc, recv_errno;
    int atmark = 0;
    int save_errno = errno;

    for (i = 1; i <= MAX_RECV_OOB; i++) {
	errno = 0;
	rc = recv(rem, oob_ptr, 1, MSG_OOB);
	if (rc <= 0) {
	    if (errno == EWOULDBLOCK) {
		continue;
	    } else {
		if (errno != EINVAL) {
		    perror("rlogin: recv_oob: recv");
		}
		break;
	    }
	}
	break;
    }
    recv_errno = errno;
    errno = save_errno;
    return(recv_errno);
}

/*
**  get_oob: recv OOB data and translate to state information
**
**	Parameters:
**		none.
**
**	Returns:
**		nothing.
**
**	Side Effects:
**		calls recv_oob;  if successful,
**		    may set global variable flushing;
**		    may change global variables stop;
**		    will clear global variable urgent;
**                  will clear global variable oob;
**		    will call do_oob.
**		
*/
#define MASK(x) ( 1L << (x-1) )
get_oob()
{
    long omask;
    int rc;

    omask = sigblock(MASK(SIGURG));
    if ((rc = recv_oob(&oob)) == 0) {
	if (oob & TIOCPKT_FLUSHWRITE) {
	    flushing = TRUE;
	}
	if (oob & TIOCPKT_WINDOW) {
		(void) kill(ppid,SIGUSR1);
	}
	if (oob & TIOCPKT_NOSTOP) {
	    stop = TIOCPKT_NOSTOP;
	} else if (oob & TIOCPKT_DOSTOP) {
	    stop = TIOCPKT_DOSTOP;
	} 
	oob = 0;
	urgent = FALSE;
	do_oob();
    } else if (rc != EWOULDBLOCK) {
	urgent = FALSE;
    }
    sigsetmask(omask);
    return(rc);
}

/*
**  do_oob: perform actions requested by rlogind via OOB byte
**
**	Parameters:
**		none.
**
**	Returns:
**		nothing.
**
**	Side Effects:
**		if not stopped (job control: see writer, usr1, and usr2):
**		    if flushing is set:
**			flushes the tty output buffer;
**			flushes pending socket input up to oob mark.
**		    if stop is set:
**			turns on or off ^S/^Q flow control in the tty driver;
**			resets stop.
**
*/
do_oob()
{
    int atmark, cc;
    char waste[BUFSIZ];
    struct termio modes;
    long omask;

#ifdef SIGTSTP
    if (!suspended) {
#endif SIGTSTP
	omask = sigblock(MASK(SIGURG));
	if (flushing) {
	    /*
	    **  flush tty output buffer
	    */
	    (void) ioctl(fileno(stdout), TCFLSH, 1);
	    /*
	    **  flush unread input to OOB mark
	    */
	    for (;;) {
		if (ioctl(rem, SIOCATMARK, &atmark) < 0) {
		    perror("rlogin: do_oob: ioctl(SIOCATMARK)");
		    break;
		}
		if (atmark)
		    break;
		if ((cc = read(rem, waste, sizeof (waste))) <= 0) {
		    perror("rlogin: do_oob: read(waste)");
		    break;
		}
	    }
	    /*
	    **  flushing is reset either after the call to do_oob following
	    **  the recv call in reader, or following the end of the write loop
	    **  in reader.
	    */
	}
	if (stop == TIOCPKT_NOSTOP) {
	    /*
	    **  turn off flow control; make ^S and ^Q ordinary characters
	    **  to rlogin's tty
	    */
	    (void) ioctl(fileno(stdout), TCGETA, &modes);
	    modes.c_iflag &= ~IXON;
	    (void) ioctl(fileno(stdout), TCSETAW, &modes);
	} else if (stop == TIOCPKT_DOSTOP) {
	    /*
	    **  turn on flow control; ^S and ^Q should be interpreted by
	    **  rlogin's tty
	    */
	    (void) ioctl(fileno(stdout), TCGETA, &modes);
	    modes.c_iflag |= IXON;
	    (void) ioctl(fileno(stdout), TCSETAW, &modes);
	}
	/*
	**  stop is reset so that we don't do the ioctl in do_oob 
	**  next time unless necessary.
	*/
	stop = 0;
	sigsetmask(omask);
#ifdef SIGTSTP
    }
#endif SIGTSTP
}
    
#ifdef SIGURG

struct sigcontext *scp;

/*
**  urg:  SIGURG handler.  If urg is able to recv the OOB byte, it sets
**	    state information (see recv_oob).  Otherwise, it sets urgent,
**	    so that reader will call get_oob, after doing an in-stream
**	    recv, in order to get the pending OOB byte.
**
**	Side Effects:
**	    if recv_oob is successful:
**		may set the global variables flushing or stop;
**		will call do_oob.
**	    otherwise:
**		may set the global variable urgent.
**		
*/
void
urg(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
    int rc;
    int save_errno = errno;

    /*
    **  by default, restart interrupted system calls;
    **  if recv was interrupted, we need to clear the socket buffer
    **  as much as possible;  if write was interrupted, we should 
    **  finish unless the OOB byte is a flush.
    */
    scp->sc_syscall_action = SIG_RESTART;
    urgent = FALSE;

    if ((rc = recv_oob(&oob)) == 0) {
	if (oob & TIOCPKT_FLUSHWRITE) {
	    flushing = TRUE;
	    if ((scp->sc_syscall == SYS_WRITE) ||
		(scp->sc_syscall == SYS_RECV)) {
		/*
		**  do not restart write or recv if flushing
		*/
		scp->sc_syscall_action = SIG_RETURN;
	    }
	}
	if (oob & TIOCPKT_WINDOW) {
		(void) kill(ppid,SIGUSR1);
	}
	if (oob & TIOCPKT_NOSTOP) {
	    stop = TIOCPKT_NOSTOP;
	} else if (oob & TIOCPKT_DOSTOP) {
	    stop = TIOCPKT_DOSTOP;
	}
	oob = 0;
	do_oob();
    } else if (rc == EWOULDBLOCK) {
	urgent = TRUE;
    }
    errno = save_errno;
}
#endif

#ifdef	SIGTSTP
/*
**  usr1: handler for SIGUSR1.  writer sends reader SIGUSR1 to indicate
**	    that writer (and maybe reader, too) is suspended and reader
**	    should not do the tty ioctls in do_oob.  usr1 calls V so
**	    that writer will not suspend itself until usr1 has completed.
**
**	Side Effects:
**		sets the global variable suspended;
**		releases the semaphore (see V). 
**		
*/
void
usr1()
{
	int save_errno;

	save_errno = errno;
	suspended = TRUE;
	V(getppid());
	errno = save_errno;
}

/*
**  usr2: handler for SIGUSR2.  writer sends reader SIGUSR2 to indicate
**	    that writer has restarted and restored "raw" terminal mode, so
**	    reader may do the tty ioctls in do_oob.
**
**	Side Effects:
**		clears the global variable suspended.
**
*/
void
usr2()
{
	suspended = FALSE;
}
#endif	SIGTSTP

/*
 * reader: read from remote: line -> 1
 */
reader()
{
    long omask;
    char rbuf[BUFSIZ];
    char *rbp = rbuf;
    register int cnt, cc;
#ifdef	SIGURG
    struct sigvec vec;

    /*
    **	set up the SIGURG signal handler so that when urgent data
    **	arrives, the reader process reads it and does the "right thing"
    */
    omask = sigblock(MASK(SIGURG));
    vec.sv_handler = urg;
    vec.sv_mask = 0;
    vec.sv_onstack = 0;
    (void) sigvector(SIGURG, &vec, (struct sigvec *) 0);
#ifdef	SIGTSTP
#ifndef COMMONUSR1
    vec.sv_handler = usr1;
    (void) sigvector(SIGUSR1, &vec, (struct sigvec *) 0);
#endif /* !COMMONUSR1 */

    vec.sv_handler = usr2;
    (void) sigvector(SIGUSR2, &vec, (struct sigvec *) 0);
#endif	SIGTSTP
    /*
    **	this mumbo-jumbo sets up the socket so that when urgent data
    **	arrives, we are notified; this is only necessary because we
    **	are the child process.
    */
    {
	int pid = getpid();

	ioctl(rem, SIOCSPGRP, (char *)&pid);
    }
    sigsetmask(omask);
#endif SIGURG
	ppid = getppid();
    for (;;) {
	/*
	**	NOTE:	this is a blocking recv: we wait either
	**	for new data to arrive, or for a SIGURG
	*/
	cnt = recv(rem, rbuf, sizeof(rbuf), 0);
	if (cnt == 0) {
	    /*
	    **	zero byte recv means socket EOF.  go home
	    */
	    break;
	} else if (cnt < 0) {
	    if (errno != EINTR) {
		perror("rlogin: reader: recv");
		break;
	    }
	}
	if (urgent) {
	    int rc = get_oob();
	}
	/*
	**	now write the data to stdout; if the write is 
	**	interrupted by SIGURG, and the OOB byte was
	**	not TIOCPKT_FLUSHWRITE, the write will be
	**	restarted, cc will be less than cnt, and the
	**	rest of the buffer will be written on successive
	**	passes through the loop.  If the OOB byte was
	**	TIOCPKT_FLUSHWRITE, the write will not be
	**	restarted, flushing will be set, and the unwritten
	**	portion of the buffer will be thrown away.
	*/
	omask = sigblock(0L);			/* get current signal mask */
	rbp = rbuf;
	while ((rbp < (rbuf + cnt)) && !flushing) {
	    sigsetmask(omask);			/* SIGURG not blocked */ 
	    cc = write(fileno(stdout), rbp, cnt - (rbp - rbuf));
	    if (cc < 0) {
		/*
		** if the write caught SIGURG, and flushing was set,
		** errno will be EINTR;  otherwise:
		*/
		if (errno != EINTR) {
		    perror("rlogin: reader: write");
		    break;
		}
	    } else {
		rbp += cc;
	    }
	    /* It is necessary to block SIGURG before the test in
	    ** the while loop (and unblock it at the head of the loop
	    ** and after resetting flushing) in order to avoid the
	    ** following:
	    **   flushing is true;
	    **   we exit the loop;
	    **   we catch SIGURG and set flushing again;
	    **   we return to reader and reset flushing, incorrectly.
	    */
	    omask = sigblock(MASK(SIGURG));
	}
	flushing = FALSE;
	sigsetmask(omask);
    }
}

#define MIN     1
#define TIME    1

void
mode(f)
{
	static struct termio ixon_state;
	static int first = TRUE;
	struct termio sb;
#ifdef	SIGTSTP
	struct	ltchars	*ltc;
#endif	SIGTSTP

	/*
	**	store initial state of IXON bit
	*/
	if (first) {
	    (void) ioctl(fileno(stdin), TCGETA, &ixon_state);
	    first = FALSE;
	}

	switch (f) {

	case 0:
		/*
		**      remember whether IXON was set, so it can be restored
		**      when mode(1) is next done
		*/
		(void) ioctl(fileno(stdin), TCGETA, &ixon_state);
		/*
		**	copy the initial modes we saved into sb; this is
		**	for restoring to the initial state
		*/
		(void)memcpy(&sb, &defmodes, sizeof(defmodes));
#ifdef	SIGTSTP
		ltc = &defltc;
#endif	SIGTSTP
		break;

	case 1:
		/*
		**	get the current stty bits, and modify them to put
		**	us into HP's equivalent of Berkeley "raw" mode
		*/
		(void) ioctl(fileno(stdin), TCGETA, &sb);
		/*
		**	turn off input mappings
		*/
		sb.c_iflag &= ~(ICRNL|INLCR|BRKINT);
		/*
		**	turn off output mappings
		*/
		sb.c_oflag &= ~(ONLCR|OCRNL);
		/*
		**	turn off canonical processing and character echo;
		**	also turn off signal checking -- ICANON might be
		**	enough to do this, but we're being careful
		*/
		sb.c_lflag &= ~(ECHO|ICANON|ISIG);
		sb.c_cc[VTIME] = TIME;
		sb.c_cc[VMIN] = MIN;
		if (eight)
			sb.c_iflag &= ~(ISTRIP);
                /* preserve tab delays, but turn off tab-to-space expansion */
                if ((sb.c_oflag & TABDLY) == TAB3)
                        sb.c_oflag &= ~TAB3;
		/*
		**  restore current flow control state
		*/
		if (ixon_state.c_iflag & IXON) {
		    sb.c_iflag |= IXON;
		} else {
		    sb.c_iflag &= ~IXON;
		}
#ifdef	SIGTSTP
		ltc = &noltc;
#endif	SIGTSTP
		break;

	default:
		return;
	}
	(void)ioctl(fileno(stdin), TCSETA, &sb);
#ifdef	SIGTSTP
	(void)ioctl(fileno(stdin), TIOCSLTC, (char *)ltc);
#endif	SIGTSTP
}

/*VARARGS*/
prf(f, a1, a2, a3)
	char *f;
{
	/*
	**	print an error message to stderr 
	*/
	fprintf(stderr, f, a1, a2, a3);
	fprintf(stderr, CRLF);
}

segv(sig,code,scp)
int sig,code;
struct sigcontext *scp;
{
    fprintf(stderr,"caught SIGSEGV: code = 0x%x; scp->sc_syscall = %d\r\n",code,scp->sc_syscall);
}
    
void
lostpeer()
{
	struct sigvec vec;
	/*
	**	handle the case of our socket going down -- if we try to
	**	write to a socket that has been closed (by the process on
	**	the other side, or by a network error), we get SIGPIPE.
	**	this routine handles that and calls done to exit nicely
	*/
        vec.sv_handler = SIG_IGN;
	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	(void) sigvector(SIGPIPE, &vec, (struct sigvec *) 0);
	prf("\007Connection closed.");
	done();
}


void
shell_escape(line)
	char *line;
{
	char *cmd, *argv[3], *ch;
	unsigned short c;
	register int shell, kid;
	long oldmask, sigblock();
	struct sigvec vec;
	int readerdied = 0;

	vec.sv_mask = 0;
	vec.sv_onstack = 0;
	vec.sv_handler = SIG_DFL;
	(void) sigvector(SIGCLD, &vec, 0);
#ifdef	SIGTSTP
	/*
	**      tell the child not to do terminal ioctls during the shell escape;
	**	this is possible because the child will handle SIGURG even while
	**	it is suspended.
	*/
	kill(child, SIGUSR1);
	/*
	**      wait until we are sure the child got the message
	*/
	P(getpid());
	/*
	**	If we're on a job control machine, we need to make sure
	**	that we STOP our peer (reader) from sending output to
	**	the tty; else if tostop is in our stty, the original csh
	**	wakes up, csh's run away, all hell breaks loose.  This
	**	fix is not guaranteed; there is still a possible race
	**	condition -- the reader MUST get the signal before it
	**	hits the next write syscall, and we hit the fork.
	*/
	(void) kill(child, SIGSTOP);
#endif	SIGTSTP

	/*
	**	if we are connected, we block SIGPIPE so that the shell escape
	**	can complete even if the socket goes down.
	*/
	oldmask = sigblock((1L << (SIGPIPE - 1)));

#if defined(SecureWare) && defined(B1)
	/*
	**  lest least privilege hamper the privileged user's fork()
	*/
	if (ISB1) {
		enablepriv(SEC_LIMIT);
	}
#endif /* SecureWare && B1 */
	shell = fork();
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		disablepriv(SEC_LIMIT);
	}
#endif /* SecureWare && B1 */
	if(shell == 0) {
		/*
		**	CHILD: set up default handlers for the user signals 
		*/
		vec.sv_handler = SIG_DFL;
		(void) sigvector(SIGINT, &vec, 0);
		(void) sigvector(SIGHUP, &vec, 0);
		(void) sigvector(SIGQUIT, &vec, 0);
		/*
		**	find the user's shell and set up the argument vector
		**	for the exec call; default to /bin/sh if not set
		*/
		argv[0] = getenv("SHELL");
		if (argv[0] == NULL)
			argv[0] = "/bin/sh";
		ch = strrchr(argv[0],'/');
		if (ch != NULL)
			argv[1] = ch + 1;
		else
			argv[1] = argv[0];

		/*
		**	search for a command.  Assumes that a "-c" will always
		**	tell $SHELL to execute one command.
		**	start searching just past the "~!", and search for
		**	a "nonspace" character (NOTE: '\0' is NOT a space)
		*/
		cmd = line;
		while ((c = CHARAT(cmd)) == ' ' || c == '\t') {
			ADVANCE(cmd);
		}
		argv[2] = cmd;
		while ((c = CHARAT(cmd)) != '\0' && c != '\r' && c != '\n') {
			ADVANCE(cmd);
		}
		*cmd = '\0';
		if (*argv[2] == '\0') {
			/*
			**	if no command is specified, just exec an
			**	interactive shell for the user 
			*/
			(void) execle(argv[0], argv[1], 0, vp);
		} else {
			/*
			**	otherwise exec the command given and return
			*/
			(void) execle(argv[0], argv[1], "-c", argv[2], 0, vp);
		}
		perror("rlogin: exec: shell");
		_exit(0);
	}

	/*
	**  PARENT:  if forked succesfully, have to be careful of reader
	**  dying.  If reader died, exit program after shell escape returns.
	*/
	if (shell < 0)
		perror("rlogin: fork");
	else
		while ((kid = wait((int *)0)) != shell)
			if (kid == child)
				readerdied = 1;
	if (readerdied) {
	       printf("Connection closed.\n");
	       done();
	}
	vec.sv_handler = done;
	(void) sigvector(SIGCLD, &vec, 0);
	(void) sigsetmask(oldmask);
	printf("[Returning to remote]\n");
#ifdef	SIGTSTP
	/*
	**	if the reader didn't die, then we have
	**	to wake it up from its sleep (SIGSTOP)
	*/
	(void) kill(child, SIGCONT);
	/*
	**      let the child know that it may do ioctls on the tty again
	*/
	kill(child, SIGUSR2);
#endif	SIGTSTP
#ifdef SIGWINCH
	sigwinch();
#endif /* SIGWINCH */
	return;
}
