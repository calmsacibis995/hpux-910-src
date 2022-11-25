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
static char sccsid[] = "@(#)rsh.c	5.7 (Berkeley) 9/20/88";
static char rcsid[] = "@(#)$Header: remsh.c,v 1.26.109.2 92/03/04 17:29:35 byrond Exp $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>

#ifdef SecureWare
#include <sys/security.h>
#endif /* SecureWare */

#include <netinet/in.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <netdb.h>
#include <fcntl.h>

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
/*
 * remsh - remote shell with hosts.equiv(4)
 * rexec - remote shell with password
 */
/* VARARGS */
int	error();
char	*index(), *rindex(), *malloc(), *getpass(), *strcpy();

struct	passwd *getpwuid();

int	errno;
int	options;
int	rfd2;
int	nflag;
void	sendsig(), catchpipe();
int	rem, pid;

extern char **environ;
char **vp;
int userexec = 0;	/* set if we are invoked as rexec */

#define _PATH_RLOGIN	"/usr/bin/rlogin"

main(argc, argv0)
	int argc;
	char **argv0;
{
	char *host, *cp, **ap, buf[BUFSIZ], *args, **argv = argv0, *user = 0;
	register int cc;
	int asrsh = 0;
	struct passwd *pwd;
	int	uid;
	fd_set readfrom, ready;
	int one = 1;
	struct servent *sp;
	int omask;
	struct sigvec	vec, ovec;
	struct stat stat_buf;
	int fd;
	char *clientname;

#ifdef SecureWare
	if (ISSECURE)
		set_auth_parameters(argc, argv);
#endif /* SecureWare */
	host = strrchr(argv[0], '/');
	if (host)
		host++;
	else
		host = argv[0];
	argv++, --argc;

	/*
	**	if invoked as something other than remsh or rsh, use the 
	**	invocation name as the host name to connect to (clever).
	*/
	if (!strcmp(host, "remsh") || !strcmp(host, "rsh") ||
	    (userexec = !strcmp(host, "rexec"))) {
		clientname = host;
		host = *argv++, --argc;
		asrsh = 1;
	} else {
		clientname = "remsh";
	}
#ifdef SecureWare
	rhost = host;
#endif /* SecureWare */
	    
	if (!userexec) { 
		/*
		**  if not running privileged, no reason to go on,
		**  since rcmd will fail in rresvport anyway.
		*/
#if defined(SecureWare) && defined(B1)
		if (ISB1) {
			initprivs();
			if (!forcepriv(SEC_REMOTE)) {
				fprintf(stderr,
		    "%s: This program requires remote potential privilege\n",
					clientname);
			       exit(1);
			}
			disablepriv(SEC_REMOTE);
		} else
#endif /* SecureWare && B1 */
		if (geteuid() != 0) {
			fprintf(stderr,
			    "%s: This program requires super user privileges\n",
			    clientname);
			exit(1);
		}
	} else {
		/*
		**  If invoked as rexec, reset uid.
		*/
		setuid(getuid());
	}
	
	/*
	**	make sure file descriptors 0, 1, and 2 are open
	*/
	for (fd = 0; fd <= 2 ; fd++)
	{
		if (fstat(fd, &stat_buf) != 0)
		{
			if (open("/dev/null", O_RDWR) < 0)
			{
				fprintf(stderr, "%s: ", clientname);
				perror("open:");
				exit(1);
			}
		}
	}

	/*
	**	save a copy of original environment in case we exec rlogin
	*/
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
	
	/*
	**	clear timers, close open files, and wipe out environment
	*/
	cleanenv(&environ,
		"LANG", "LANGOPTS", "NLSPATH",
		"LOCALDOMAIN", "HOSTALIASES", 0);

another:
	if (argc > 0 && !strcmp(*argv, "-l")) {
		argv++, argc--;
		if (argc > 0)
			user = *argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strcmp(*argv, "-n")) {
		argv++, argc--;
		nflag++;
		goto another;
	}
	if (argc > 0 && !strcmp(*argv, "-d")) {
		argv++, argc--;
		options |= SO_DEBUG;
		goto another;
	}
	/*
	 * Ignore the -L, -w, -e and -8 flags to allow aliases with rlogin
	 * to work
	 *
	 * There must be a better way to do this! -jmb
	 */
	if (argc > 0 && !strncmp(*argv, "-L", 2)) {
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-w", 2)) {
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-e", 2)) {
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-8", 2)) {
		argv++, argc--;
		goto another;
	}
	if (argc > 0 && !strncmp(*argv, "-7", 2)) {
		argv++, argc--;
		goto another;
	}
	if (host == 0)
		goto usage;
	if (argv[0] == 0) {
		if (userexec)
			goto usage;
		if (asrsh)
			*argv0 = "rlogin";
		/*
		**	exec rlogin, restoring original environment;
		**	let rlogin do its own cleanenv().
		*/
		execve(_PATH_RLOGIN, argv0, vp);
		fprintf(stderr, "%s: ", clientname);
		perror(_PATH_RLOGIN);
		exit(1);
	}

	uid = getuid();
	pwd = getpwuid(uid);
	if (pwd == (struct passwd *) NULL) 
	{ 
		fprintf(stderr,
			"%s: There is no passwd entry for you (user id %d)\n",
			clientname, uid);
		exit(1);
	}
#ifdef SecureWare
	lusr = pwd->pw_name;
#endif /* SecureWare */
	cc = 0;
	for (ap = argv; *ap; ap++)
		cc += strlen(*ap) + 1;
	cp = args = malloc(cc);
	for (ap = argv; *ap; ap++) {
		(void) strcpy(cp, *ap);
		while (*cp)
			cp++;
		if (ap[1])
			*cp++ = ' ';
	}
	if (userexec) {
		sp = getservbyname("exec", "tcp");
		if (sp == (struct servent *) NULL) {
			fprintf(stderr, "%s: exec/tcp: Unknown service\n",
				clientname);
			exit(1);
		}
        	rem = rexec(&host, sp->s_port, user ? user : pwd->pw_name, 
	    	            0, args, &rfd2);
	} else {
		sp = getservbyname("shell", "tcp");
		if (sp == (struct servent *) NULL) {
			fprintf(stderr, "%s: shell/tcp: Unknown service\n",
				clientname);
			exit(1);
		}
#if defined(SecureWare) && defined(B1)
		if (ISB1) {
			/*
			**  rcmd binds to a reserved port, which requires REMOTE
			*/
			forcepriv(SEC_REMOTE);
		}
#endif /* SecureWare && B1 */
		rem = rcmd(&host, sp->s_port, pwd->pw_name,
			user ? user : pwd->pw_name, args, &rfd2);
#if defined(SecureWare) && defined(B1)
		if (ISB1) {
			disablepriv(SEC_REMOTE);
		}
#endif /* SecureWare && B1 */
	}
        if (rem < 0) {
		/*
		**	rcmd and rexec print their own error messages
		*/
                exit(1);
	}
	if (rfd2 < 0) {
		fprintf(stderr, "%s: Can't establish rfd2 (stderr)\n",
			clientname);
		exit(2);
	}
#ifdef SecureWare
	rusr = user ? user : pwd->pw_name;
	AUDIT_SUCCESS("Certified identity of local user");
#endif /* SecureWare */
	if (options & SO_DEBUG) {
		if (setsockopt(rem, SOL_SOCKET, SO_DEBUG,
		    &one, sizeof(one)) < 0) {
			fprintf(stderr, "%s: ", clientname);
			perror("setsockopt (stdin)");
		}
		if (setsockopt(rfd2, SOL_SOCKET, SO_DEBUG,
		    &one, sizeof(one)) < 0) {
			fprintf(stderr, "%s: ", clientname);
			perror("setsockopt (stderr)");
		}
	}
	/*
	**	remove my effective userid permissions ...
	*/
	(void) setuid(getuid());

	/*
	**	set up signal handlers, unless already SIG_IGN (/bin/sh process
	**	running in the background).  my signal handler will propagate
	**	the signals to the remote host as a byte written to the
	**	secondary socket (remshd/rexecd protocol)
	*/
	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|
	                 sigmask(SIGTERM)|sigmask(SIGHUP));

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, sendsig);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, sendsig);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, sendsig);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, sendsig);
	/*
    **  ignore the death of my child -- banish zombies!
    */
    signal(SIGCLD, SIG_IGN);

	if (nflag == 0) {
		pid = fork();
		if (pid < 0) {
			perror("fork");
			exit(1);
		}
	} else {
		/* if no stdin, shutdown (remote) reads on socket */
		shutdown(rem, 1);
	}

	/*
	**	set the sockets to be non-blocking ...
	*/
	ioctl(rfd2, FIONBIO, &one);
	ioctl(rem, FIONBIO, &one);
        if (nflag == 0 && pid == 0) {
		char *bp;
		int wc;
		fd_set rembits;

		(void) close(rfd2);
		FD_ZERO(&rembits);
	reread:
		errno = 0;
		cc = read(0, buf, sizeof buf);
		if (cc <= 0)
			goto done;
		bp = buf;
	rewrite:
		FD_SET(rem, &rembits);
		if (select(FD_SETSIZE, 0, &rembits, 0, 0) < 0) {
			if (errno != EINTR) {
				perror("select");
				exit(1);
			}
			goto rewrite;
		}
		if (!FD_ISSET(rem, &rembits))
			goto rewrite;
		wc = write(rem, bp, cc);
		if (wc < 0) {
			if (errno == EWOULDBLOCK)
				goto rewrite;
			goto done;
		}
		cc -= wc; bp += wc;
		if (cc == 0)
			goto reread;
		goto rewrite;
	done:
		(void) shutdown(rem, 1);
		exit(0);
		/*NOTREACHED*/
	}

	/*
	**	this is the parent only, since the child (above)
	**	has exited ... child cannot get here
	*/

	/*
	**	on SIGPIPE, pass the signal on to remsh/rexec child
	**	and remshd/rexecd.
	*/
	if (signal(SIGPIPE, SIG_IGN) != SIG_IGN)
		signal(SIGPIPE, catchpipe);
	sigsetmask(omask);

	/*
	**	the parent's job is to select between the two sockets
	**	and either write to stdout or stderr, depending on which
	**	socket we got activity upon ...
	*/
	FD_ZERO(&readfrom);
	FD_SET(rfd2, &readfrom);
	FD_SET(rem, &readfrom);
	do {
		ready = readfrom;
		if (select(FD_SETSIZE, &ready, 0, 0, 0) < 0) {
			if (errno != EINTR) {
				fprintf(stderr, "%s: ", clientname);
				perror("select");
				exit(1);
			}
			continue;
		}
		if (FD_ISSET(rfd2, &ready)) {
			errno = 0;
			cc = read(rfd2, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK)
					FD_CLR(rfd2, &readfrom);
			} else
				(void) write(2, buf, cc);
		}
		if (FD_ISSET(rem, &ready)) {
			errno = 0;
			cc = read(rem, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK)
					FD_CLR(rem, &readfrom);
			} else
				(void) write(1, buf, cc);
		}
        } while (FD_ISSET(rem, &readfrom) || 
		 FD_ISSET(rfd2, &readfrom));

	/*
	** send a hard kill to the child; OK if it's already exited
	*/
	if (nflag == 0)
		(void) kill(pid, SIGKILL);
	exit(0);
usage:
	fprintf(stderr, "usage: %s host [ -l login ] [ -n ] command\n",
		clientname);
	exit(1);
}


/*
**	send a signal to the remote process as a signal number (single byte)
**	written to the secondary socket (remshd/rexecd protocol)
*/
void
sendsig(signo)
	char signo;
{
	(void) write(rfd2, (char *)&signo, 1);
}

/*
**	pass SIGPIPE on to remsh/rexec child and remshd/rexecd.
*/
void
catchpipe(signo)
char signo;
{
	if (nflag == 0)
		kill(pid, signo);
	(void) write(rfd2, (char *)&signo, 1);
	exit(0);
}
