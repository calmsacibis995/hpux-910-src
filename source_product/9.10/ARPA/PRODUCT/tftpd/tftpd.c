/*
 * Copyright (c) 1983 Regents of the University of California.
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
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)tftpd.c	5.8 (Berkeley) 6/18/88";
static char rcsid[] = "@(#)$Header: tftpd.c,v 1.13.109.4 94/11/16 14:18:32 mike Exp $";
#endif /* not lint */

/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton <guyton@rand-unix>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <arpa/tftp.h>

#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <setjmp.h>
#include <syslog.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/param.h>

#define	TIMEOUT		5

extern	int errno;
struct	sockaddr_in sin = { AF_INET };
int	peer;
int	rexmtval = TIMEOUT;
int	maxtimeout = 5*TIMEOUT;

#define	PKTSIZE	SEGSIZE+4
char	buf[PKTSIZE];
char	ackbuf[PKTSIZE];
struct	sockaddr_in from;
int	fromlen;

static char	**dirs;		      /* list of non-option arguments */
static int	ndirs;	      	      /* # of items in "dirs" */
static char 	*home;		      /* home directory of user "tftp" */
static int	home_is_slash;        /* home is "/" (until proven otherwise) */

main(argc, argv)
	int argc;
	char **argv;
{
	register struct tftphdr *tp;
	register int n, k;
	int on = 1, c;
	struct passwd *pw;
	extern int opterr;
	extern int optind;
	extern char *optarg;

	openlog("tftpd", LOG_PID, LOG_DAEMON);
	if (ioctl(0, FIONBIO, &on) < 0) {
		syslog(LOG_ERR, "ioctl(FIONBIO): %m\n");
		exit(1);
	}
	fromlen = sizeof (from);
	n = recvfrom(0, buf, sizeof (buf), 0,
	    (caddr_t)&from, &fromlen);
	if (n < 0) {
		syslog(LOG_ERR, "recvfrom: %m\n");
		exit(1);
	}
	/*
	 * Now that we have read the message out of the UDP
	 * socket, we fork and exit.  Thus, inetd will go back
	 * to listening to the tftp port, and the next request
	 * to come in will start up a new instance of tftpd.
	 *
	 * We do this so that inetd can run tftpd in "wait" mode.
	 * The problem with tftpd running in "nowait" mode is that
	 * inetd may get one or more successful "selects" on the
	 * tftp port before we do our receive, so more than one
	 * instance of tftpd may be started up.  Worse, if tftpd
	 * break before doing the above "recvfrom", inetd would
	 * spawn endless instances, clogging the system.
	 */
	{
		int pid;
		int i, j;

		for (i = 1; i < 20; i++) {
		    pid = fork();
		    if (pid < 0) {
				sleep(i);
				/*
				 * flush out to most recently sent request.
				 *
				 * This may drop some request, but those
				 * will be resent by the clients when
				 * they timeout.  The positive effect of
				 * this flush is to (try to) prevent more
				 * than one tftpd being started up to service
				 * a single request from a single client.
				 */
				j = sizeof from;
				i = recvfrom(0, buf, sizeof (buf), 0,
				    (caddr_t)&from, &j);
				if (i > 0) {
					n = i;
					fromlen = j;
				}
		    } else {
				break;
		    }
		}
		if (pid < 0) {
			syslog(LOG_ERR, "fork: %m\n");
			exit(1);
		} else if (pid != 0) {
			exit(0);
		}
	}

	/*
	 * Now that we're on our own, process any arguments.
	 */
	opterr = 0;
	while ((c = getopt(argc, argv, "T:R:")) != EOF)
		switch (c) {
			
		case 'T':
			k = atoi(optarg);
			if (k <= 0)
				syslog(LOG_ERR, "Invalid total timeout %s",
				       optarg);
			else
				maxtimeout = k;
			break;
		case 'R':
			k = atoi(optarg);
			if (k <= 0)
				syslog(LOG_ERR, 
				       "Invalid retransmission timeout %s",
				       optarg);
			else
				rexmtval = k;
			break;
		default:
			syslog(LOG_ERR, "Unknown option -%c ignored", c);
		}

	/* skip arguments */
	argc -= optind;
	argv += optind;

	if (argc > 0) 
	{ 	
		/* 
		   Tftpd was invoked with non-option args.
		   Make a list ("dirs") of paths passed as arguments. 
		   Each path in the list will start with a slash (/), 
		   and no path will contain multiple consecutive slashes or
		   a trailing slash.
		*/

		if ((ndirs = get_list(argc, argv, &dirs)) < 0) 
		{
			syslog(LOG_ERR, "get_list() memory allocation error");
			exit(1);
		}
	}

	if ((pw = getpwnam("tftp")) == NULL) 
	{	
		/* No user "tftp" */

		if (ndirs == 0) 
		{ 
			syslog(LOG_ERR, 
				"No security mechanism exists; see tftpd(1M)");
			exit(1);
		}

		home_is_slash = 1;

	}
	else 
	{

		if (trim_path(pw->pw_dir, &home) < 0) 
		{
			syslog(LOG_ERR, "trim_path() memory allocation error");
			exit(1);
		}

		home_is_slash = (home == NULL) || (home[1] == '\0');

		if (home_is_slash && (ndirs == 0))


			/* 
			  ~tftp is "/" and there is no restriction list.
			  The whole file system is open to clients.
			  Log a warning, but continue. It's probably
			  not by accident.
			*/

			syslog(LOG_WARNING, "\"/\" is open for tftp access!");

	}

	/* 
	   Make "/" the working directory.
	   Do a "chroot" first, if ~tftp is not "/" 
	   and there are no non-option arguments;
	 */

	if (!home_is_slash && (ndirs == 0)) 
	{ 
	    if (chroot(home) < 0) 
	    {
		syslog(LOG_ERR, "chroot: %m");
		exit(1);
	    }
	}

	if (chdir("/") < 0) 
	{
		syslog(LOG_ERR, "chdir: %m");
		exit(1);
	}

	if (pw != NULL) 
	{

	    /* Leave our effective uid & gid alone */

	    if (setresuid(pw->pw_uid, -1, -1) < 0) 
	    {
		syslog(LOG_ERR, "setresuid: set real uid %d: %m", pw->pw_uid);
		exit(1);
	    }

	    if (setresgid(pw->pw_gid, -1, -1) < 0) 
	    {
		syslog(LOG_ERR, "setresuid: set real gid %d: %m", pw->pw_uid);
		exit(1);
	    }
	}

	from.sin_family = AF_INET;
	alarm(0);
	close(0);
	close(1);
	peer = socket(AF_INET, SOCK_DGRAM, 0);
	if (peer < 0) {
		syslog(LOG_ERR, "socket: %m\n");
		exit(1);
	}
	if (bind(peer, (caddr_t)&sin, sizeof (sin)) < 0) {
		syslog(LOG_ERR, "bind: %m\n");
		exit(1);
	}
	if (connect(peer, (caddr_t)&from, sizeof(from)) < 0) {
		syslog(LOG_ERR, "connect: %m\n");
		exit(1);
	}
	tp = (struct tftphdr *)buf;
	tp->th_opcode = ntohs(tp->th_opcode);
	if (tp->th_opcode == RRQ || tp->th_opcode == WRQ)
		tftp(tp, n);
	exit(1);
}

int	validate_access();
int	sendfile(), recvfile();

struct formats {
	char	*f_mode;
	int	(*f_validate)();
	int	(*f_send)();
	int	(*f_recv)();
	int	f_convert;
} formats[] = {
	{ "netascii",	validate_access,	sendfile,	recvfile, 1 },
	{ "octet",	validate_access,	sendfile,	recvfile, 0 },
#ifdef notdef
	{ "mail",	validate_user,		sendmail,	recvmail, 1 },
#endif
	{ 0 }
};

/*
 * Handle initial connection protocol.
 */
tftp(tp, size)
	struct tftphdr *tp;
	int size;
{
	register char *cp;
	int first = 1, ecode;
	register struct formats *pf;
	char *filename, *mode;

	filename = cp = tp->th_stuff;
again:
	while (cp < buf + size) {
		if (*cp == '\0')
			break;
		cp++;
	}
	if (*cp != '\0') {
		nak(EBADOP);
		exit(1);
	}
	if (first) {
		mode = ++cp;
		first = 0;
		goto again;
	}
	for (cp = mode; *cp; cp++)
		if (isupper(*cp))
			*cp = tolower(*cp);
	for (pf = formats; pf->f_mode; pf++)
		if (strcmp(pf->f_mode, mode) == 0)
			break;
	if (pf->f_mode == 0) {
		nak(EBADOP);
		exit(1);
	}
	ecode = (*pf->f_validate)(filename, tp->th_opcode);
	if (ecode) {
		nak(ecode);
		exit(1);
	}
	if (tp->th_opcode == WRQ)
		(*pf->f_recv)(pf);
	else
		(*pf->f_send)(pf);
	exit(0);
}


int 
trim_path(path_in, path_out)
register char *path_in; 
char **path_out;
{

	char 		buf[MAXPATHLEN + 1]; 	/* space to copy "path_in" in */
	register char 	*bufp = buf;
	register char 	*end;			/* last char in "path_in" */
	char		save;			/* used to restore "path_in" */


	/* Can't do anything with NULL or empty path */

	if (path_in == NULL || *path_in == '\0') 
	{
		*path_out = NULL;
		return 0;
	}

	/* Skip leading white space (very unlikely to have any) */

	while (*path_in != '\0' && isspace(*path_in))
		++path_in;

	if (*path_in == '\0') 
	{
		*path_out = NULL;
		return 0;
	}

	/* Trim trailing white space (very unlikely to have any) */

	end = path_in + strlen(path_in) - 1;
	while (isspace(*end))
		--end;

	/* Set new string end, after saving the char. in that position */

	save = *++end;
	*end = '\0';

	/* 
	  Make a new copy of "path_in," replacing 
	  multiple consecutive slashes with one slash; 
	  the new path will always start with a slash.
	*/

	*bufp = '/';
	while (*path_in != '\0') 
	{
		if (*path_in == '/')
			if (*bufp == '/') 
			{
				++path_in;
				continue;
			}
		*++bufp = *path_in++;
	}

	/* End the string; remove trailing '/' (if there is one) */

	if (*bufp == '/' && bufp != buf)
		*bufp = '\0';
	else
		*++bufp = '\0';

	/* Restore "path_in" to original */

	*end = save;

	/* Return trimmed copy of "path_in" in "path_out" */

	*path_out = strdup(buf);
	if (*path_out == NULL)
		return -1;

	return 0;
}

int 
get_list(argc, argv, dirs)
int argc;
char **argv;
char ***dirs;
{
/*
  Save the paths in "argv" to "dirs."
  The saved paths have the following characteristics:
    - start with a slash;
    - don't end with a slash;
    - don't contain multiple consecutive slashes.
*/

  
	register int n = 0;
	char *path;

	*dirs = (char **) malloc(argc * sizeof(char *));
	if (*dirs == NULL)
		return -1;

	while (argc-- > 0) 
	{

		if (trim_path(*argv, &path) < 0)
			return -1;

		if (path != NULL)
			(*dirs)[n++] = path;

		++argv;
	}

	return n;
}

int 
access_denied(fn)
register char *fn;
{

/* 
  Returns 1 if none of the paths in the "dirs" list
  is at the base of the "fn" path; otherwise returns 0. 
*/

    register int 	dlen;
    register char 	**dirp = dirs;
    register int 	i;
    register int 	flen;

    flen = strlen(fn);
    for (i = 0; i < ndirs; i++) 
    {

    	dlen = strlen(*dirp);
	if (flen >= dlen && strncmp(fn, *dirp, dlen) == 0) 
	{
		/* The two paths must be either identical,
		   or the first character in the path "fn"
		   follwing the matched part must be a slash.
		*/

		if (dlen == flen || fn[dlen] == '/')
			break;
	}
	++dirp;

    }
    return i >= ndirs;
}


FILE *file;


int 
open_file(filename, request)
register char *filename;
register int request;
{

/* 
  Check "filename" access and status, open it,
  and associate the file descriptor with a stream.
*/
	register int	fd;
	register int	mode; 
	struct stat 	stbuf;

	mode = request == RRQ ? R_OK : W_OK;
	if (access(filename, mode) < 0)
		return (errno == ENOENT ? ENOTFOUND : EACCESS);

	mode = request == RRQ ? O_RDONLY : (O_WRONLY | O_TRUNC);
	if ((fd = open(filename, mode)) < 0)
		return errno+100;

	/* Only allow transfer of regular files */

	if (fstat(fd, &stbuf) < 0)	/* shouldn't happen */
		return  EACCESS;
	if ((stbuf.st_mode & S_IFMT) != S_IFREG)
		return EACCESS;

	if ((file = fdopen(fd, (request == RRQ)? "r" : "w")) == NULL)
		return errno+100;

	/* Succeeded */
	return (0);
}

int 
validate_access(filename, mode)
char *filename;
int mode;
{
	char 	tftp_path[MAXPATHLEN + 1];

	char 	*fn;	/* trimmed copy of "filename" (starts with /, has no 
			   trailing / and no multiple consecutive slashes */
	int 	ret1 = -1,
		ret2; 	/* open_file() return values */

	/* Make a trimmed copy of "filename" */

	if (trim_path(filename, &fn) < 0 || fn == NULL)
		return EACCESS;

	if (ndirs == 0)
	{
		/* 
		  Access is limited to ~tftp. (Before reaching 
		  this point, tftpd did a "chroot" to ~tftp.) 
		  Open "filename" for the operation specified by "mode"
		  (provided that the access permissions of the file allow it)
		*/

		ret1 = open_file(fn, mode);
		free(fn);
		return ret1;
	}

	/* 
	   Don't allow back door access. Note that the "/../" 
	   pattern is harmless at the beginning of the path when ~tftp 
	   doesn't exist or is "/," but we will reject it anyway 
	   to make the point that it should not be used. 
	   (A path ending with "/.." is a directory, and it will
	   be caught later--the HP version of tftp rejects such 
	   a path on the client side)
	 */

	if (strstr(fn, "/../") != NULL) 
	{
		free(fn);
		return EACCESS;
	}

	if (!home_is_slash) {

		/* 
		  A tftp home directory exists, but tftpd 
		  didn't do a "chroot" to it. Look for "filename"
		  relative to ~tftp.
		*/

		/* Prepend ~tftp to "filename" */

		strcpy(tftp_path, home);
		strcat(tftp_path, fn);

		/*
		  Open "filename" for the operation specified by "mode"
		  (provided that the access permissions of the file allow it)
		*/

		if ((ret1 = open_file(tftp_path, mode)) == 0) 
		{
			free(fn);
			return 0;
		}

		/* else 
			try to find it in the places listed 
		   	on the command line 
		*/
	}

	/* 
	  Check that "filename" is listed on the command line,
	  or that it could be in one of the directories given
	  on the command line.
	*/

	if (access_denied(fn)) 
	{
		free(fn);
		return EACCESS;
	}

	/*
	  Open "filename" for the operation specified by "mode"
	 (provided that the access permissions of the file allow it)
	*/

	ret2 = open_file(fn, mode);
	free(fn);

	if (ret2 == 0)
		return 0;

	return (ret2 > ret1 ? ret2 : ret1);
}


int	timeout;
jmp_buf	timeoutbuf;

timer()
{

	timeout += rexmtval;
	if (timeout >= maxtimeout)
		exit(1);
	longjmp(timeoutbuf, 1);
}

/*
 * Send the requested file.
 */
sendfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *r_init();
	register struct tftphdr *ap;    /* ack packet */
	register short int block = 1, size, n;

	signal(SIGALRM, timer);
	dp = r_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		size = readit(file, &dp, pf->f_convert);
		if (size < 0) {
			nak(errno + 100);
			goto abort;
		}
		dp->th_opcode = htons((u_short)DATA);
		dp->th_block = htons((u_short)block);
		timeout = 0;
		(void) setjmp(timeoutbuf);

send_data:
		if (send(peer, dp, size + 4, 0) != size + 4) {
			syslog(LOG_ERR, "write: %m\n");
			goto abort;
		}
		read_ahead(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);        /* read the ack */
			n = recv(peer, ackbuf, sizeof (ackbuf), 0);
			alarm(0);
			if (n < 0) {
				syslog(LOG_ERR, "read: %m\n");
				goto abort;
			}
			ap->th_opcode = ntohs((u_short)ap->th_opcode);
			ap->th_block = ntohs((u_short)ap->th_block);

			if (ap->th_opcode == ERROR)
				goto abort;
			
			if (ap->th_opcode == ACK) {
				if (ap->th_block == block) {
					break;
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (ap->th_block == (block -1)) {
					goto send_data;
				}
			}

		}
		block++;
	} while (size == SEGSIZE);
abort:
	(void) fclose(file);
}

justquit()
{
	exit(0);
}


/*
 * Receive a file.
 */
recvfile(pf)
	struct formats *pf;
{
	struct tftphdr *dp, *w_init();
	register struct tftphdr *ap;    /* ack buffer */
	register short int block = 0, n, size;

	signal(SIGALRM, timer);
	dp = w_init();
	ap = (struct tftphdr *)ackbuf;
	do {
		timeout = 0;
		ap->th_opcode = htons((u_short)ACK);
		ap->th_block = htons((u_short)block);
		block++;
		(void) setjmp(timeoutbuf);
send_ack:
		if (send(peer, ackbuf, 4, 0) != 4) {
			syslog(LOG_ERR, "write: %m\n");
			goto abort;
		}
		write_behind(file, pf->f_convert);
		for ( ; ; ) {
			alarm(rexmtval);
			n = recv(peer, dp, PKTSIZE, 0);
			alarm(0);
			if (n < 0) {            /* really? */
				syslog(LOG_ERR, "read: %m\n");
				goto abort;
			}
			dp->th_opcode = ntohs((u_short)dp->th_opcode);
			dp->th_block = ntohs((u_short)dp->th_block);
			if (dp->th_opcode == ERROR)
				goto abort;
			if (dp->th_opcode == DATA) {
				if (dp->th_block == block) {
					break;   /* normal */
				}
				/* Re-synchronize with the other side */
				(void) synchnet(peer);
				if (dp->th_block == (block-1))
					goto send_ack;          /* rexmit */
			}
		}
		/*  size = write(file, dp->th_data, n - 4); */
		size = writeit(file, &dp, n - 4, pf->f_convert);
		if (size != (n-4)) {                    /* ahem */
			if (size < 0) nak(errno + 100);
			else nak(ENOSPACE);
			goto abort;
		}
	} while (size == SEGSIZE);
	write_behind(file, pf->f_convert);
	(void) fclose(file);            /* close data file */

	ap->th_opcode = htons((u_short)ACK);    /* send the "final" ack */
	ap->th_block = htons((u_short)(block));
	(void) send(peer, ackbuf, 4, 0);

	signal(SIGALRM, justquit);      /* just quit on timeout */
	alarm(rexmtval);
	n = recv(peer, buf, sizeof (buf), 0); /* normally times out and quits */
	alarm(0);
	if (n >= 4 &&                   /* if read some data */
	    dp->th_opcode == DATA &&    /* and got a data block */
	    block == dp->th_block) {	/* then my last ack was lost */
		(void) send(peer, ackbuf, 4, 0);     /* resend final ack */
	}
abort:
	return;
}

struct errmsg {
	int	e_code;
	char	*e_msg;
} errmsgs[] = {
	{ EUNDEF,	"Undefined error code" },
	{ ENOTFOUND,	"File not found" },
	{ EACCESS,	"Access violation" },
	{ ENOSPACE,	"Disk full or allocation exceeded" },
	{ EBADOP,	"Illegal TFTP operation" },
	{ EBADID,	"Unknown transfer ID" },
	{ EEXISTS,	"File already exists" },
	{ ENOUSER,	"No such user" },
	{ -1,		0 }
};

/*
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
nak(error)
	int error;
{
	register struct tftphdr *tp;
	int length;
	register struct errmsg *pe;
	extern char *sys_errlist[];

	tp = (struct tftphdr *)buf;
	tp->th_opcode = htons((u_short)ERROR);
	tp->th_code = htons((u_short)error);
	for (pe = errmsgs; pe->e_code >= 0; pe++)
		if (pe->e_code == error)
			break;
	if (pe->e_code < 0) {
		pe->e_msg = sys_errlist[error - 100];
		tp->th_code = EUNDEF;   /* set 'undef' errorcode */
	}
	strcpy(tp->th_msg, pe->e_msg);
	length = strlen(pe->e_msg);
	tp->th_msg[length] = '\0';
	length += 5;
	if (send(peer, buf, length, 0) != length) {
		syslog(LOG_ERR, "nak: %m\n");
	}
}
