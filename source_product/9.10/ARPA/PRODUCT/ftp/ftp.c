/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
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

#ifndef lint
static char rcsid[] = "$Header: ftp.c,v 1.17.109.3 93/10/26 17:44:34 donnad Exp $";
static char sccsid[] = "@(#)ftp.c	5.28 (Berkeley) 4/20/89";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <arpa/ftp.h>
#include <arpa/telnet.h>

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <pwd.h>
#include <varargs.h>

#include "ftp_var.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#ifdef NOVFPRINTF
#define vfprintf(a,b,c) _doprnt(b,c,a)
#endif

#if defined(sun) || defined(hpux)
/* FD_SET wasn't defined until 4.0. its a cheap test for uid_t  presence */
#ifndef FD_SET
#define	NBBY	8		/* number of bits in a byte */
/*
 * Select uses bit masks of file descriptors in longs.
 * These macros manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))

typedef int uid_t;
typedef int gid_t;
#endif
#endif

struct	sockaddr_in hisctladdr;
struct	sockaddr_in data_addr;
int	data = -1;
int	abrtflag = 0;
int	ptflag = 0;
int	connected;
int	allbinary;
struct	sockaddr_in myctladdr;
uid_t	getuid();
sig_t	lostpeer();
off_t	restart_point = 0;

FILE	*cin, *cout;
FILE	*dataconn();

/* 
   For better performance the buffer size is 
   a page multiple, aligned on a page boundary 
*/

#define BIGBUFSIZ 8*NBPG	/* 32K */
#define ALIGN(addr) \
	((unsigned int) ((unsigned int) (addr) + (NBPG - 1)) & ~(NBPG - 1))

char *
hookup(host, port)
	char *host;
	int port;
{
	register struct hostent *hp = 0;
	int s,len;
	static char hostnamebuf[80];

	bzero((char *)&hisctladdr, sizeof (hisctladdr));
	hisctladdr.sin_addr.s_addr = inet_addr(host);
	if (hisctladdr.sin_addr.s_addr != -1) {
		hisctladdr.sin_family = AF_INET;
		(void) strncpy(hostnamebuf, host, sizeof(hostnamebuf));
	} else {
		hp = gethostbyname(host);
		if (hp == NULL) {
			fprintf(stderr, "ftp: %s: ", host);
			herror((char *)NULL);
			code = -1;
			return((char *) 0);
		}
		hisctladdr.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0],
		    (caddr_t)&hisctladdr.sin_addr, hp->h_length);
		(void) strncpy(hostnamebuf, hp->h_name, sizeof(hostnamebuf));
	}
	hostname = hostnamebuf;
	s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
	if (s < 0) {
		perror("ftp: socket");
		code = -1;
		return (0);
	}
	hisctladdr.sin_port = port;
	while (connect(s, (struct sockaddr *)&hisctladdr, sizeof (hisctladdr)) < 0) {
		if (hp && hp->h_addr_list[1]) {
			int oerrno = errno;
			extern char *inet_ntoa();

			fprintf(stderr, "ftp: connect to address %s: ",
				inet_ntoa(hisctladdr.sin_addr));
			errno = oerrno;
			perror((char *) 0);
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0],
			     (caddr_t)&hisctladdr.sin_addr, hp->h_length);
			fprintf(stdout, "Trying %s...\n",
				inet_ntoa(hisctladdr.sin_addr));
			(void) close(s);
			s = socket(hisctladdr.sin_family, SOCK_STREAM, 0);
			if (s < 0) {
				perror("ftp: socket");
				code = -1;
				return (0);
			}
			continue;
		}
		perror("ftp: connect");
		code = -1;
		goto bad;
	}
	len = sizeof (myctladdr);
	if (getsockname(s, (struct sockaddr *)&myctladdr, &len) < 0) {
		perror("ftp: getsockname");
		code = -1;
		goto bad;
	}
	cin = fdopen(s, "r");
	cout = fdopen(s, "w");
	if (cin == NULL || cout == NULL) {
		fprintf(stderr, "ftp: fdopen failed.\n");
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
	if (verbose)
		printf("Connected to %s.\n", hostname);
	if (getreply(0) > 2) { 	/* read startup message from server */
		if (cin)
			(void) fclose(cin);
		if (cout)
			(void) fclose(cout);
		code = -1;
		goto bad;
	}
#ifdef SO_OOBINLINE
	{
	int on = 1;

	if (setsockopt(s, SOL_SOCKET, SO_OOBINLINE, &on, sizeof(on))
		< 0 && debug) {
			perror("ftp: setsockopt");
		}
	}
#endif SO_OOBINLINE

	return (hostname);
bad:
	(void) close(s);
	return ((char *)0);
}

login(host)
	char *host;
{
	char tmp[80];
	char *user, *pass, *acct, *getlogin(), *getlongpass();
	int n, aflag = 0;

	user = pass = acct = 0;
	if (ruserpass(host, &user, &pass, &acct) < 0) {
		code = -1;
		return(0);
	}
	while (user == NULL) {
		char *myname = getlogin();

		if (myname == NULL) {
			struct passwd *pp = getpwuid(getuid());

			if (pp != NULL)
				myname = pp->pw_name;
		}
		if (myname)
			printf("Name (%s:%s): ", host, myname);
		else
			printf("Name (%s): ", host);
		(void) fgets(tmp, sizeof(tmp) - 1, stdin);
		tmp[strlen(tmp) - 1] = '\0';
		if (*tmp == '\0')
			user = myname;
		else
			user = tmp;
	}
	n = command("USER %s", user);
	if (n == CONTINUE) {
		if (pass == NULL)
			pass = getlongpass("Password:");
		n = command("PASS %s", pass);
	}
	if (n == CONTINUE) {
		aflag++;
		if (acct == NULL)
			acct = getlongpass("Account:");
		n = command("ACCT %s", acct);
	}
	if (n != COMPLETE) {
		fprintf(stderr, "Login failed.\n");
		return (0);
	}
	if (!aflag && acct != NULL)
		(void) command("ACCT %s", acct);
	if (proxy)
		return(1);
	for (n = 0; n < macnum; ++n) {
		if (!strcmp("init", macros[n].mac_name)) {
			(void) strcpy(line, "$init");
			makeargv();
			domacro(margc, margv);
			break;
		}
	}
	return (1);
}

sig_t
cmdabort()
{
	extern jmp_buf ptabort;

	printf("\n");
	(void) fflush(stdout);
	abrtflag++;
	if (ptflag)
		longjmp(ptabort,1);
}

/*VARARGS1*/
command(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;
	int r;
	sig_t (*oldintr)(), cmdabort();

	va_start(ap);
	fmt = va_arg(ap, char *);

	abrtflag = 0;
	if (debug) {
		printf("---> ");
		va_start(ap);
		fmt = va_arg(ap, char *);
		vfprintf(stdout, fmt, ap);
		va_end(ap);
		printf("\n");
		(void) fflush(stdout);
	}
	if (cout == NULL) {
		perror ("No control connection for command");
		code = -1;
		va_end(ap);
		return (0);
	}
	oldintr = signal(SIGINT,cmdabort);
	va_start(ap);
	fmt = va_arg(ap, char *);
	vfprintf(cout, fmt, ap);
	va_end(ap);
	fprintf(cout, "\r\n");
	(void) fflush(cout);
	cpend = 1;
	r = getreply(!strcmp(fmt, "QUIT"));
	if (abrtflag && oldintr != SIG_IGN)
		(*oldintr)();
	(void) signal(SIGINT, oldintr);
	return(r);
}

char reply_string[BUFSIZ];		/* last line of previous reply */

#include <ctype.h>

getreply(expecteof)
	int expecteof;
{
	register int c, n;
	register int dig;
	register char *cp;
	int originalcode = 0, continuation = 0;
	sig_t (*oldintr)(), cmdabort();
	int pflag = 0;
	char *pt = pasv;

	oldintr = signal(SIGINT,cmdabort);
	for (;;) {
		dig = n = code = 0;
		cp = reply_string;
		while ((c = getc(cin)) != '\n') {
			if (c == IAC) {     /* handle telnet commands */
				switch (c = getc(cin)) {
				case WILL:
				case WONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,DONT,c);
					(void) fflush(cout);
					break;
				case DO:
				case DONT:
					c = getc(cin);
					fprintf(cout, "%c%c%c",IAC,WONT,c);
					(void) fflush(cout);
					break;
				default:
					break;
				}
				continue;
			}
			dig++;
			if (c == EOF) {
				if (expecteof) {
					(void) signal(SIGINT,oldintr);
					code = 221;
					return (0);
				}
				lostpeer();
				if (verbose) {
					printf("421 Service not available, remote server has closed connection\n");
					(void) fflush(stdout);
				}
				code = 421;
				return(4);
			}
			if (c != '\r' && (verbose > 0 ||
			    (verbose > -1 && n == '5' && dig > 4))) {
				if (proxflag &&
				   (dig == 1 || dig == 5 && verbose == 0))
					printf("%s:",hostname);
				(void) putchar(c);
			}
			if (dig < 4 && isdigit(c))
				code = code * 10 + (c - '0');
			if (!pflag && code == 227)
				pflag = 1;
			if (dig > 4 && pflag == 1 && isdigit(c))
				pflag = 2;
			if (pflag == 2) {
				if (c != '\r' && c != ')')
					*pt++ = c;
				else {
					*pt = '\0';
					pflag = 3;
				}
			}
			if (dig == 4 && c == '-') {
				if (continuation)
					code = 0;
				continuation++;
			}
			if (n == 0)
				n = c;
			if (cp < &reply_string[sizeof(reply_string) - 1])
				*cp++ = c;
		}
		if (verbose > 0 || verbose > -1 && n == '5') {
			(void) putchar(c);
			(void) fflush (stdout);
		}
		if (continuation && code != originalcode) {
			if (originalcode == 0)
				originalcode = code;
			continue;
		}
		*cp = '\0';
		if (n != '1')
			cpend = 0;
		(void) signal(SIGINT,oldintr);
		if (code == 421 || originalcode == 421)
			lostpeer();
		if (abrtflag && oldintr != cmdabort && oldintr != SIG_IGN)
			(*oldintr)();
		return (n - '0');
	}
}

empty(mask, sec)
	struct fd_set *mask;
	int sec;
{
	struct timeval t;

	t.tv_sec = (long) sec;
	t.tv_usec = 0;
	return(select(FD_SETSIZE, mask, (struct fd_set *) 0, (struct fd_set *) 0, &t));
}

jmp_buf	sendabort;

abortsend()
{

	mflag = 0;
	abrtflag = 0;
	printf("\nsend aborted\n");
	(void) fflush(stdout);
	longjmp(sendabort, 1);
}

#define HASHBYTES 1024

sendrequest(cmd, local, remote, printnames)
	char *cmd, *local, *remote;
	int printnames;
{
	FILE *fin, *dout = 0, *popen();
	int (*closefunc)(), pclose(), fclose();
	sig_t (*oldintr)(), (*oldintp)();
	int abortsend();


	long bytes = 0, hashbytes = HASHBYTES;
	register int c, d;
	struct stat st;
	struct timeval start, stop;
	char *mode;

	if (verbose && printnames) {
		if (local && *local != '-')
			printf("local: %s ", local);
		if (remote)
			printf("remote: %s\n", remote);
	}
	if (proxy) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	mode = "w";
	if (setjmp(sendabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) signal(SIGINT,oldintr);
		if (oldintp)
			(void) signal(SIGPIPE,oldintp);
		code = -1;
		return;
	}
	oldintr = signal(SIGINT, abortsend);
	if (strcmp(local, "-") == 0)
		fin = stdin;
	else if (*local == '|') {
		oldintp = signal(SIGPIPE,SIG_IGN);
		fin = popen(local + 1, "r");
		if (fin == NULL) {
			perror(local + 1);
			(void) signal(SIGINT, oldintr);
			(void) signal(SIGPIPE, oldintp);
			code = -1;
			return;
		}
		closefunc = pclose;
	} else {
		fin = fopen(local, "r");
		if (fin == NULL) {
			perror(local);
			(void) signal(SIGINT, oldintr);
			code = -1;
			return;
		}
		closefunc = fclose;
		if (fstat(fileno(fin), &st) < 0 ||
		    (st.st_mode&S_IFMT) != S_IFREG) {
			fprintf(stdout, "%s: not a plain file.\n", local);
			(void) signal(SIGINT, oldintr);
			fclose(fin);
			code = -1;
			return;
		}
	}
	if (initconn("w")) {
		(void) signal(SIGINT, oldintr);
		if (oldintp)
			(void) signal(SIGPIPE, oldintp);
		code = -1;
		if (closefunc != NULL)
			(*closefunc)(fin);
		return;
	}
	if (setjmp(sendabort))
		goto abort;

	if (restart_point &&
	    (strcmp(cmd, "STOR") == 0 || strcmp(cmd, "APPE") == 0)) {
		if (fseek(fin, (long) restart_point, 0) < 0) {
			perror(local);
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
		if (command("REST %ld", (long) restart_point)
			!= CONTINUE) {
			restart_point = 0;
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
		restart_point = 0;
		mode = "r+w";
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			if (oldintp)
				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
	} else
		if (command("%s", cmd) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			if (oldintp)
				(void) signal(SIGPIPE, oldintp);
			if (closefunc != NULL)
				(*closefunc)(fin);
			return;
		}
	dout = dataconn(mode);
	if (dout == NULL)
		goto abort;
	(void) gettimeofday(&start, (struct timezone *)0);
	oldintp = signal(SIGPIPE, SIG_IGN);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
	    {
		/* 64K, page aligned buffer (used as two 32K buffers) */

		char buf[2 * BIGBUFSIZ + NBPG]; 
		register char *bufp;
		register char *aligned_buf = ALIGN(buf);
		register char *end_buf = aligned_buf + 2 * BIGBUFSIZ;
		int bufsize = BIGBUFSIZ;

		errno = d = 0;
		bufp = aligned_buf;
		while ((c = read(fileno(fin), bufp, bufsize)) > 0) {
			bytes += c;
			if ((d = send(fileno(dout), bufp, c, 0)) != c)
				break;
			if (hash) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}

			bufp += bufsize;
			if (bufp == end_buf)
				bufp = aligned_buf;
		}
		if (hash && bytes > 0) {
			if (bytes < HASHBYTES)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0)
			perror(local);
		if (d < 0) {
			if (errno != EPIPE) 
				perror("netout");
			bytes = -1;
		}
		break;
	    }

	case TYPE_A:
		while ((c = getc(fin)) != EOF) {
			if (c == '\n') {
				while (hash && (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += HASHBYTES;
				}
				if (ferror(dout))
					break;
				(void) putc('\r', dout);
				bytes++;
			}
			(void) putc(c, dout);
			bytes++;
	/*		if (c == '\r') {			  	*/
	/*		(void)	putc('\0', dout);  /* this violates rfc */
	/*			bytes++;				*/
	/*		}                          			*/	
		}
		if (hash) {
			if (bytes < hashbytes)
				(void) putchar('#');
			while (bytes >= hashbytes) {
				(void) putchar('#');
				hashbytes += HASHBYTES;
			}
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(fin))
			perror(local);
		if (ferror(dout)) {
			if (errno != EPIPE)
				perror("netout");
			bytes = -1;
		}
		break;
	}
	(void) gettimeofday(&stop, (struct timezone *)0);
	if (closefunc != NULL)
		(*closefunc)(fin);
	(void) fclose(dout);
	(void) getreply(0);
	(void) signal(SIGINT, oldintr);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	if (bytes > 0)
		ptransfer("sent", bytes, &start, &stop);
	return;
abort:
	(void) gettimeofday(&stop, (struct timezone *)0);
	(void) signal(SIGINT, oldintr);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	if (!cpend) {
		code = -1;
		return;
	}
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (dout)
		(void) fclose(dout);
	(void) getreply(0);
	code = -1;
	if (closefunc != NULL && fin != NULL)
		(*closefunc)(fin);
	if (bytes > 0)
		ptransfer("sent", bytes, &start, &stop);
}

jmp_buf	recvabort;

sig_t
abortrecv()
{

	mflag = 0;
	abrtflag = 0;
	printf("\n");
	(void) fflush(stdout);
	longjmp(recvabort, 1);
}

recvrequest(cmd, local, remote, mode, printnames)
	char *cmd, *local, *remote, *mode;
{
	FILE *fout, *din = 0, *popen();
	int (*closefunc)(), pclose(), fclose();
	sig_t (*oldintr)(), (*oldintp)(), abortrecv();
	int oldverbose, oldtype = 0, is_retr, tcrflag, nfnd, bare_lfs = 0;
	char *gunique(), msg;

	/* 32K buffer, page aligned */

	char buf[BIGBUFSIZ + NBPG];
	char *aligned_buf = ALIGN(buf);
	int bufsize = BIGBUFSIZ;

	long bytes = 0, hashbytes = HASHBYTES;
	struct fd_set mask;
	register int c, d;
	struct timeval start, stop;
	struct stat st;

	is_retr = strcmp(cmd, "RETR") == 0;
	if (is_retr && verbose && printnames) {
		if (local && *local != '-')
			printf("local: %s ", local);
		if (remote)
			printf("remote: %s\n", remote);
	}
	if (proxy && is_retr) {
		proxtrans(cmd, local, remote);
		return;
	}
	closefunc = NULL;
	oldintr = NULL;
	oldintp = NULL;
	tcrflag = !crflag && is_retr;
	if (setjmp(recvabort)) {
		while (cpend) {
			(void) getreply(0);
		}
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		if (oldintr)
			(void) signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	oldintr = signal(SIGINT, abortrecv);
	if (strcmp(local, "-") && *local != '|') {
		if (access(local, 2) < 0) {
			char *dir = rindex(local, '/');

			if (errno != ENOENT && errno != EACCES) {
				perror(local);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (dir != NULL)
				*dir = 0;
			d = access(dir && *local ? local : ".", 2);
			if (dir != NULL)
				*dir = '/';
			if (d < 0) {
				perror(local);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (!runique && errno == EACCES &&
			    chmod(local, 0600) < 0) {
				perror(local);
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
			if (runique && errno == EACCES &&
			   (local = gunique(local)) == NULL) {
				(void) signal(SIGINT, oldintr);
				code = -1;
				return;
			}
		}
		else if (runique && (local = gunique(local)) == NULL) {
			(void) signal(SIGINT, oldintr);
			code = -1;
			return;
		}
	}
	if (initconn("r")) {
		(void) signal(SIGINT, oldintr);
		code = -1;
		return;
	}
	if (setjmp(recvabort))
		goto abort;
	if (!is_retr) {
		if (type != TYPE_A && (allbinary == 0 || type != TYPE_I)) {
			oldtype = type;
			oldverbose = verbose;
			if (!debug)
				verbose = 0;
			setascii();
			verbose = oldverbose;
		}
	} else if (restart_point) {
		if (command("REST %ld", (long) restart_point) != CONTINUE)
			return;
	}
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						setbinary();
						break;
					case TYPE_E:
						setebcdic();
						break;
					case TYPE_L:
						settenex();
						break;
				}
				verbose = oldverbose;
			}
			return;
		}
	} else {
		if (command("%s", cmd) != PRELIM) {
			(void) signal(SIGINT, oldintr);
			if (oldtype) {
				if (!debug)
					verbose = 0;
				switch (oldtype) {
					case TYPE_I:
						setbinary();
						break;
					case TYPE_E:
						setebcdic();
						break;
					case TYPE_L:
						settenex();
						break;
				}
				verbose = oldverbose;
			}
			return;
		}
	}
	din = dataconn("r");
	if (din == NULL)
		goto abort;
	if (strcmp(local, "-") == 0)
		fout = stdout;
	else if (*local == '|') {
		oldintp = signal(SIGPIPE, SIG_IGN);
		fout = popen(local + 1, "w");
		if (fout == NULL) {
			perror(local+1);
			goto abort;
		}
		closefunc = pclose;
	} else {
		fout = fopen(local, mode);
		if (fout == NULL) {
			perror(local);
			goto abort;
		}
		closefunc = fclose;
	}
	(void) gettimeofday(&start, (struct timezone *)0);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		if (restart_point &&
		    lseek(fileno(fout), (long) restart_point, L_SET) < 0) {
			perror(local);
			if (closefunc != NULL)
				(*closefunc)(fout);
			return;
		}
		errno = d = 0;
		while ((c = read(fileno(din), aligned_buf, bufsize)) > 0) {
			if ((d = write(fileno(fout), aligned_buf, c)) != c)
				break;
			bytes += c;
			if (hash) {
				while (bytes >= hashbytes) {
					(void) putchar('#');
					hashbytes += HASHBYTES;
				}
				(void) fflush(stdout);
			}
		}
		if (hash && bytes > 0) {
			if (bytes < HASHBYTES)
				(void) putchar('#');
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (c < 0) {
			if (errno != EPIPE)
				perror("netin");
			bytes = -1;
		}
		if (d < c) {
			if (d < 0)
				perror(local);
			else
				fprintf(stderr, "%s: short write\n", local);
		}
		break;

	case TYPE_A:
		if (restart_point) {
			register int i, n, c;

			if (fseek(fout, 0L, L_SET) < 0)
				goto done;
			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fout)) == EOF)
					goto done;
				if (c == '\n')
					i++;
			}
			if (fseek(fout, 0L, L_INCR) < 0) {
done:
				perror(local);
				if (closefunc != NULL)
					(*closefunc)(fout);
				return;
			}
		}
		while ((c = getc(din)) != EOF) {
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				while (hash && (bytes >= hashbytes)) {
					(void) putchar('#');
					(void) fflush(stdout);
					hashbytes += HASHBYTES;
				}
				bytes++;
				if ((c = getc(din)) != '\n' || tcrflag) {
					if (ferror(fout))
						goto break2;
					(void) putc('\r', fout);
					if (c == '\0') {
						bytes++;
						goto contin2;
					}
					if (c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, fout);
			bytes++;
	contin2:	;
		}
break2:
		if (bare_lfs) {
			printf("WARNING! %d bare linefeeds received in ASCII mode\n", bare_lfs);
			printf("File may not have transferred correctly.\n");
		}
		if (hash) {
			if (bytes < hashbytes)
				(void) putchar('#');
			while (bytes >= hashbytes) {
				(void) putchar('#');
				hashbytes += HASHBYTES;
			}
			(void) putchar('\n');
			(void) fflush(stdout);
		}
		if (ferror(din)) {
			if (errno != EPIPE)
				perror("netin");
			bytes = -1;
		}
		if (ferror(fout))
			perror(local);
		break;
	}
	if (closefunc != NULL)
		(*closefunc)(fout);
	(void) signal(SIGINT, oldintr);
	if (oldintp)
		(void) signal(SIGPIPE, oldintp);
	(void) gettimeofday(&stop, (struct timezone *)0);
	(void) fclose(din);
	(void) getreply(0);
	if (bytes > 0 && is_retr)
		ptransfer("received", bytes, &start, &stop);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		verbose = oldverbose;
	}
	return;
abort:

/* abort using RFC959 recommended IP,SYNC sequence  */

	(void) gettimeofday(&stop, (struct timezone *)0);
	if (oldintp)
		(void) signal(SIGPIPE, oldintr);
	(void) signal(SIGINT,SIG_IGN);
	if (oldtype) {
		if (!debug)
			verbose = 0;
		switch (oldtype) {
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		verbose = oldverbose;
	}
	if (!cpend) {
		code = -1;
		(void) signal(SIGINT,oldintr);
		return;
	}

	fprintf(cout,"%c%c",IAC,IP);
	(void) fflush(cout); 
	msg = IAC;
/* send IAC in urgent mode instead of DM because UNIX places oob mark */
/* after urgent byte rather than before as now is protocol            */
	if (send(fileno(cout),&msg,1,MSG_OOB) != 1) {
		perror("abort");
	}
	fprintf(cout,"%cABOR\r\n",DM);
	(void) fflush(cout);
	FD_ZERO(&mask);
	FD_SET(fileno(cin), &mask);
	if (din) { 
		FD_SET(fileno(din), &mask);
	}
	if ((nfnd = empty(&mask,10)) <= 0) {
		if (nfnd < 0) {
			perror("abort");
		}
		code = -1;
		lostpeer();
	}
	if (din && FD_ISSET(fileno(din), &mask)) {
		while ((c = read(fileno(din), buf, bufsize)) > 0)
			;
	}
	if ((c = getreply(0)) == ERROR && code == 552) { /* needed for nic style abort */
		if (data >= 0) {
			(void) close(data);
			data = -1;
		}
		(void) getreply(0);
	}
	(void) getreply(0);
	code = -1;
	if (data >= 0) {
		(void) close(data);
		data = -1;
	}
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	if (din)
		(void) fclose(din);
	if (bytes > 0)
		ptransfer("received", bytes, &start, &stop);
	(void) signal(SIGINT,oldintr);
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */
int sendport = -1;

initconn(mode)
	char *mode;
{
	register char *p, *a;
	int result, len, tmpno = 0;
	int on = 1;
	int bufsize = BIGBUFSIZ;

noport:
	data_addr = myctladdr;
	if (sendport)
		data_addr.sin_port = 0;	/* let system pick one */ 
	if (data != -1)
		(void) close (data);
	data = socket(AF_INET, SOCK_STREAM, 0);
	if (data < 0) {
		perror("ftp: socket");
		if (tmpno)
			sendport = 1;
		return (1);
	}

	if (*mode == 'w') 
	{
		/* Set send socket buffer to a large value */
		if (setsockopt(data, SOL_SOCKET, SO_SNDBUF, 
			       &bufsize, sizeof(bufsize)) < 0) {
			perror("ftp: setsockopt (SO_SNDBUF)");
			goto bad;
		}

		/* Set copy avoidance on send */
		if (setsockopt(
			data, SOL_SOCKET,SO_SND_COPYAVOID, &on,sizeof(int)) < 0)
		{
			perror("ftp: setsockopt (SO_SND_COPYAVOID)");
			goto bad;
		}
	}

	/* Set receive buffer to a large value (regardless of "mode") */
	if (setsockopt(data, SOL_SOCKET, SO_RCVBUF,
			       &bufsize, sizeof(bufsize)) < 0) 
	{
		perror("ftp: setsockopt (SO_RCVBUF)");
		goto bad;
	}

	/* Set copy avoidance on receive */
	if (setsockopt(data, SOL_SOCKET,SO_RCV_COPYAVOID, &on,sizeof(int)) < 0)
	{
		perror("ftp: setsockopt (SO_RCV_COPYAVOID)");
		goto bad;
	}

	if (!sendport)
		if (setsockopt(data, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on)) < 0) {
			perror("ftp: setsockopt (reuse address)");
			goto bad;
		}
	if (bind(data, (struct sockaddr *)&data_addr, sizeof (data_addr)) < 0) {
		perror("ftp: bind");
		goto bad;
	}
	if (options & SO_DEBUG &&
	    setsockopt(data, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof (on)) < 0)
		perror("ftp: setsockopt (ignored)");
	len = sizeof (data_addr);
	if (getsockname(data, (struct sockaddr *)&data_addr, &len) < 0) {
		perror("ftp: getsockname");
		goto bad;
	}
	if (listen(data, 1) < 0)
		perror("ftp: listen");
	if (sendport) {
		a = (char *)&data_addr.sin_addr;
		p = (char *)&data_addr.sin_port;
#define	UC(b)	(((int)b)&0xff)
		result =
		    command("PORT %d,%d,%d,%d,%d,%d",
		      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
		      UC(p[0]), UC(p[1]));
		if (result == ERROR && sendport == -1) {
			sendport = 0;
			tmpno = 1;
			goto noport;
		}
		return (result != COMPLETE);
	}
	if (tmpno)
		sendport = 1;
	return (0);
bad:
	(void) close(data), data = -1;
	if (tmpno)
		sendport = 1;
	return (1);
}

FILE *
dataconn(mode)
	char *mode;
{
	struct sockaddr_in from;
	int s, fromlen = sizeof (from);
	FILE *datastream;

	s = accept(data, (struct sockaddr *) &from, &fromlen);
	if (s < 0) {
		perror("ftp: accept");
		(void) close(data), data = -1;
		return (NULL);
	}
	(void) close(data);
	data = s;
	datastream = fdopen(data, mode);
	if (setvbuf(datastream, (char *)NULL, _IOFBF, BIGBUFSIZ) != 0) {
		perror("ftp: setvbuf");
		(void) close(data), data = -1;
		return (NULL);
	}
	return (datastream);
}

ptransfer(direction, bytes, t0, t1)
	char *direction;
	long bytes;
	struct timeval *t0, *t1;
{
	struct timeval td;
	float s, bs;

	if (verbose) {
		tvsub(&td, t1, t0);
		s = td.tv_sec + (td.tv_usec / 1000000.);
#define	nz(x)	((x) == 0 ? 1 : (x))
		bs = bytes / nz(s);
		printf("%ld bytes %s in %.2f seconds (%.2f Kbytes/s)\n",
		    bytes, direction, s, bs / 1024.);
	}
}

/*tvadd(tsum, t0)
	struct timeval *tsum, *t0;
{

	tsum->tv_sec += t0->tv_sec;
	tsum->tv_usec += t0->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
} */

tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

sig_t
psabort()
{
	extern int abrtflag;

	abrtflag++;
}

pswitch(flag)
	int flag;
{
	extern int proxy, abrtflag;
	sig_t (*oldintr)();
	static struct comvars {
		int connect;
		char name[MAXHOSTNAMELEN];
		struct sockaddr_in mctl;
		struct sockaddr_in hctl;
		FILE *in;
		FILE *out;
		int tpe;
		int cpnd;
		int sunqe;
		int runqe;
		int mcse;
		int ntflg;
		char nti[17];
		char nto[17];
		int mapflg;
		char mi[MAXPATHLEN];
		char mo[MAXPATHLEN];
		} proxstruct, tmpstruct;
	struct comvars *ip, *op;

	abrtflag = 0;
	oldintr = signal(SIGINT, psabort);
	if (flag) {
		if (proxy)
			return;
		ip = &tmpstruct;
		op = &proxstruct;
		proxy++;
	}
	else {
		if (!proxy)
			return;
		ip = &proxstruct;
		op = &tmpstruct;
		proxy = 0;
	}
	ip->connect = connected;
	connected = op->connect;
	if (hostname) {
		(void) strncpy(ip->name, hostname, sizeof(ip->name) - 1);
		ip->name[strlen(ip->name)] = '\0';
	} else
		ip->name[0] = 0;
	hostname = op->name;
	ip->hctl = hisctladdr;
	hisctladdr = op->hctl;
	ip->mctl = myctladdr;
	myctladdr = op->mctl;
	ip->in = cin;
	cin = op->in;
	ip->out = cout;
	cout = op->out;
	ip->tpe = type;
	type = op->tpe;
	if (!type)
		type = 1;
	ip->cpnd = cpend;
	cpend = op->cpnd;
	ip->sunqe = sunique;
	sunique = op->sunqe;
	ip->runqe = runique;
	runique = op->runqe;
	ip->mcse = mcase;
	mcase = op->mcse;
	ip->ntflg = ntflag;
	ntflag = op->ntflg;
	(void) strncpy(ip->nti, ntin, 16);
	(ip->nti)[strlen(ip->nti)] = '\0';
	(void) strcpy(ntin, op->nti);
	(void) strncpy(ip->nto, ntout, 16);
	(ip->nto)[strlen(ip->nto)] = '\0';
	(void) strcpy(ntout, op->nto);
	ip->mapflg = mapflag;
	mapflag = op->mapflg;
	(void) strncpy(ip->mi, mapin, MAXPATHLEN - 1);
	(ip->mi)[strlen(ip->mi)] = '\0';
	(void) strcpy(mapin, op->mi);
	(void) strncpy(ip->mo, mapout, MAXPATHLEN - 1);
	(ip->mo)[strlen(ip->mo)] = '\0';
	(void) strcpy(mapout, op->mo);
	(void) signal(SIGINT, oldintr);
	if (abrtflag) {
		abrtflag = 0;
		(*oldintr)();
	}
}

jmp_buf ptabort;
int ptabflg;

sig_t
abortpt()
{
	printf("\n");
	(void) fflush(stdout);
	ptabflg++;
	mflag = 0;
	abrtflag = 0;
	longjmp(ptabort, 1);
}

proxtrans(cmd, local, remote)
	char *cmd, *local, *remote;
{
	sig_t (*oldintr)(), abortpt();
	int tmptype, oldtype = 0, secndflag = 0, nfnd;
	extern jmp_buf ptabort;
	char *cmd2;
	struct fd_set mask;

	if (strcmp(cmd, "RETR"))
		cmd2 = "RETR";
	else
		cmd2 = runique ? "STOU" : "STOR";
	if (command("PASV") != COMPLETE) {
		printf("proxy server does not support third part transfers.\n");
		return;
	}
	tmptype = type;
	pswitch(0);
	if (!connected) {
		printf("No primary connection\n");
		pswitch(1);
		code = -1;
		return;
	}
	if (type != tmptype) {
		oldtype = type;
		switch (tmptype) {
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
	}
	if (command("PORT %s", pasv) != COMPLETE) {
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		pswitch(1);
		return;
	}
	if (setjmp(ptabort))
		goto abort;
	oldintr = signal(SIGINT, abortpt);
	if (command("%s %s", cmd, remote) != PRELIM) {
		(void) signal(SIGINT, oldintr);
		switch (oldtype) {
			case 0:
				break;
			case TYPE_A:
				setascii();
				break;
			case TYPE_I:
				setbinary();
				break;
			case TYPE_E:
				setebcdic();
				break;
			case TYPE_L:
				settenex();
				break;
		}
		pswitch(1);
		return;
	}
	sleep(2);
	pswitch(1);
	secndflag++;
	if (command("%s %s", cmd2, local) != PRELIM)
		goto abort;
	ptflag++;
	(void) getreply(0);
	pswitch(0);
	(void) getreply(0);
	(void) signal(SIGINT, oldintr);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			setascii();
			break;
		case TYPE_I:
			setbinary();
			break;
		case TYPE_E:
			setebcdic();
			break;
		case TYPE_L:
			settenex();
			break;
	}
	pswitch(1);
	ptflag = 0;
	printf("local: %s remote: %s\n", local, remote);
	return;
abort:
	(void) signal(SIGINT, SIG_IGN);
	ptflag = 0;
	if (strcmp(cmd, "RETR") && !proxy)
		pswitch(1);
	else if (!strcmp(cmd, "RETR") && proxy)
		pswitch(0);
	if (!cpend && !secndflag) {  /* only here if cmd = "STOR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					setascii();
					break;
				case TYPE_I:
					setbinary();
					break;
				case TYPE_E:
					setebcdic();
					break;
				case TYPE_L:
					settenex();
					break;
			}
			if (cpend) {
				char msg[2];

				fprintf(cout,"%c%c",IAC,IP);
				(void) fflush(cout); 
				*msg = IAC;
				*(msg+1) = DM;
				if (send(fileno(cout),msg,2,MSG_OOB) != 2)
					perror("abort");
				fprintf(cout,"ABOR\r\n");
				(void) fflush(cout);
				FD_ZERO(&mask);
				FD_SET(fileno(cin), &mask);
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						perror("abort");
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
		}
		pswitch(1);
		if (ptabflg)
			code = -1;
		(void) signal(SIGINT, oldintr);
		return;
	}
	if (cpend) {
		char msg[2];

		fprintf(cout,"%c%c",IAC,IP);
		(void) fflush(cout); 
		*msg = IAC;
		*(msg+1) = DM;
		if (send(fileno(cout),msg,2,MSG_OOB) != 2)
			perror("abort");
		fprintf(cout,"ABOR\r\n");
		(void) fflush(cout);
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				perror("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (!cpend && !secndflag) {  /* only if cmd = "RETR" (proxy=1) */
		if (command("%s %s", cmd2, local) != PRELIM) {
			pswitch(0);
			switch (oldtype) {
				case 0:
					break;
				case TYPE_A:
					setascii();
					break;
				case TYPE_I:
					setbinary();
					break;
				case TYPE_E:
					setebcdic();
					break;
				case TYPE_L:
					settenex();
					break;
			}
			if (cpend) {
				char msg[2];

				fprintf(cout,"%c%c",IAC,IP);
				(void) fflush(cout); 
				*msg = IAC;
				*(msg+1) = DM;
				if (send(fileno(cout),msg,2,MSG_OOB) != 2)
					perror("abort");
				fprintf(cout,"ABOR\r\n");
				(void) fflush(cout);
				FD_ZERO(&mask);
				FD_SET(fileno(cin), &mask);
				if ((nfnd = empty(&mask,10)) <= 0) {
					if (nfnd < 0) {
						perror("abort");
					}
					if (ptabflg)
						code = -1;
					lostpeer();
				}
				(void) getreply(0);
				(void) getreply(0);
			}
			pswitch(1);
			if (ptabflg)
				code = -1;
			(void) signal(SIGINT, oldintr);
			return;
		}
	}
	if (cpend) {
		char msg[2];

		fprintf(cout,"%c%c",IAC,IP);
		(void) fflush(cout); 
		*msg = IAC;
		*(msg+1) = DM;
		if (send(fileno(cout),msg,2,MSG_OOB) != 2)
			perror("abort");
		fprintf(cout,"ABOR\r\n");
		(void) fflush(cout);
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				perror("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	pswitch(!proxy);
	if (cpend) {
		FD_ZERO(&mask);
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,10)) <= 0) {
			if (nfnd < 0) {
				perror("abort");
			}
			if (ptabflg)
				code = -1;
			lostpeer();
		}
		(void) getreply(0);
		(void) getreply(0);
	}
	if (proxy)
		pswitch(0);
	switch (oldtype) {
		case 0:
			break;
		case TYPE_A:
			setascii();
			break;
		case TYPE_I:
			setbinary();
			break;
		case TYPE_E:
			setebcdic();
			break;
		case TYPE_L:
			settenex();
			break;
	}
	pswitch(1);
	if (ptabflg)
		code = -1;
	(void) signal(SIGINT, oldintr);
}

reset()
{
	struct fd_set mask;
	int nfnd = 1;

	FD_ZERO(&mask);
	while (nfnd > 0) {
		FD_SET(fileno(cin), &mask);
		if ((nfnd = empty(&mask,0)) < 0) {
			perror("reset");
			code = -1;
			lostpeer();
		}
		else if (nfnd) {
			(void) getreply(0);
		}
	}
}

char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	char *cp = rindex(local, '/');
	int d, count=0;
	char ext = '1';

	if (cp)
		*cp = '\0';
	d = access(cp && *local ? local : ".", 2);
	if (cp)
		*cp = '/';
	if (d < 0) {
		perror(local);
		return((char *) 0);
	}
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	while (!d) {
		if (++count == 100) {
			printf("runique: can't find unique file name.\n");
			return((char *) 0);
		}
		*cp++ = ext;
		*cp = '\0';
		if (ext == '9')
			ext = '0';
		else
			ext++;
		if ((d = access(new, 0)) < 0)
			break;
		if (ext != '0')
			cp--;
		else if (*(cp - 2) == '.')
			*(cp - 1) = '1';
		else {
			*(cp - 2) = *(cp - 2) + 1;
			cp--;
		}
	}
	return(new);
}
