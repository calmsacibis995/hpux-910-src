/*
** Copyright 1990 (C) Hewlett Packard Corporation
**
** Portions are:
**
** Copyright (c) 1983 Regents of the University of California.
** All rights reserved.  The Berkeley software License Agreement
** specifies the terms and conditions for redistribution.
**
** from 1.38 88/02/07 SMI; from UCB 5.14 (Berkeley) 1/23/89
**
** $Header: inetd.h,v 1.7.109.10 94/11/18 10:22:45 mike Exp $
*/

# ifdef _DEFINE
# define EXTERN
# ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
# endif lint
# else  _DEFINE
# define EXTERN extern
# endif _DEFINE

/*
 * Inetd - Internet super-server
 *
 * SYNOPSIS:
 *		inetd -l
 *		inetd -c
 *		inetd -k
 *		inetd -L
 *
 * This program invokes all internet services as needed.
 * connection-oriented services are invoked each time a
 * connection is made, by creating a process.  This process
 * is passed the connection as file descriptor 0 and is
 * expected to do a getpeername to find out the source host
 * and port.
 *
 * Datagram oriented services are invoked when a datagram
 * arrives; a process is created and passed a pending message
 * on file descriptor 0.  Datagram servers may either connect
 * to their peer, freeing up the original socket for inetd
 * to receive further messages on, or ``take over the socket'',
 * processing all arriving datagrams and, eventually, timing
 * out.	 The first type of server is said to be ``multi-threaded'';
 * the second type of server ``single-threaded''. 
 *
 * Inetd uses a configuration file which is read at startup
 * and, possibly, at some later time in response to a hangup signal.
 * The configuration file is ``free format'' with fields given in the
 * order shown below.  Continuation lines for an entry must being with
 * a space or tab.  All fields must be present in each entry.
 *
 *	service name			must be in /etc/services
 *	socket type			stream/dgram/raw/rdm/seqpacket
 *	protocol			must be in /etc/protocols
 *	wait/nowait			single-threaded/multi-threaded
 *	user				user to run daemon as
 *	server program			full path name
 *	server program arguments	including server program name
 *
 * for rpc services:
 *	rpc
 *	socket type			stream/dgram/raw/rdm/seqpacket
 *	protocol			of the form rpc/protocol
 *	wait/nowait			single-threaded/multi-threaded
 *	user				user to run daemon as
 *	server program			full path name
 *	port				for portmapper
 *	version				the version of version range
 *	server program arguments	including server program name
 *
 * Comment lines are indicated by a `#' in column 1.
 */


#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/termio.h>
#include <sys/wait.h>
#include <malloc.h>
#include <netinet/in.h>
#include <syslog.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <ctype.h>

#include <arpa/nameser.h>
#include <resolv.h>

#ifdef AUDIT
#include <sys/audit.h>
#endif

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif TRUE

#define NOSECURITY	0	/* No security file used */
#define SECURITY	1	/* Security file used */
#define BADSECURITY	-1	/* Prolem with security file */
#define OUTOFMEMORY     -3      /* Out of memory when mallocing */

#define MAXLINE		2048
#define MAXARGS		64
#define INTSIZE		32		/* size of integer */

#define INETD_SEM_KEY	17368354
#define SEM_CREATE 	(IPC_CREAT | IPC_EXCL | 0644)
#define	CONFIG_FILE	"/etc/inetd.conf"
#define	SECURITY_FILE	"/usr/adm/inetd.sec"
#define	SIGBLOCK	(sigmask(SIGCHLD) | sigmask(SIGHUP) |\
			 sigmask(SIGALRM) | sigmask(SIGQUIT))

#define	TOOMANY		40		/* don't start more than TOOMANY */
#define	CNT_INTVL	60		/* servers in CNT_INTVL sec. */
#define	RETRYTIME	(60*10)		/* retry after bind or server fail */

EXTERN int	initconf;	/* configuring or reconfiguring inetd */
EXTERN int	maxservers;	/* max. number of configurable services */
EXTERN int	nservers;	/* total number of services configured */
EXTERN FILE	*fsecure;	/* security file */
EXTERN FILE	*fconfig;	/* configuration file */
EXTERN fd_set	allsock;	/* list of active servers */
EXTERN int	nsock, maxsock;
EXTERN int	options;
EXTERN int	timingout;
EXTERN char     *remotehost;    /* name of client host */
EXTERN char     *remoteaddr;    /* address of client host in dot notation  */
EXTERN struct hostent *remotehp;
/*
 * Each service requires one file descriptor for the socket we listen on
 * for requests for that service.  We don't allow more services than
 * we can allocate file descriptors for.  OTHERDESCRIPTORS is the number
 * of descriptors needed for other purposes; this number was determined
 * experimentally.
 */
#define	OTHERDESCRIPTORS	8


/*
** services 
*/

struct	servtab {
    char	*se_service;	/* name of service */
    int		se_socktype;	/* type of socket to use */
    char	*se_proto;	/* protocol used */
    char	se_isrpc;	/* service is RPC-based */
    int		se_wait;	/* single threaded server */
    int		se_endwait;	/* running wait becomes nowait */
    int		se_checked;	/* looked at during merge */
    char	*se_user;	/* user name to run as */
    struct	biltin *se_bi;	/* if built-in, description */
    char	*se_server;	/* server program */
    char	**se_argv;	/* argument vector */
    int		se_fd;		/* open descriptor */
    union {
	struct	sockaddr_in ctrladdr; /* bound address */
	struct {
	    unsigned prog;	/* program number */
	    unsigned lowvers;	/* lowest version supported */
	    unsigned highvers;	/* highest version supported */
	    char *name;
	} rpcnum;
    } se_un;
    int		se_count;	/* number started since se_time */
    struct	timeval se_time;/* start of se_count */
    struct	servtab *se_next;
};

EXTERN struct servtab *servtab;

#define se_ctrladdr se_un.ctrladdr
#define se_rpc se_un.rpcnum

/*
** Internal routines
*/

int echo_stream(), discard_stream(), machtime_stream();
int daytime_stream(), chargen_stream();
int echo_dg(), discard_dg(), machtime_dg(), daytime_dg(), chargen_dg();

struct biltin {
    char	*bi_service;	/* internally provided service name */
    int		bi_socktype;	/* type of socket supported */
    int		bi_fork;	/* 1 if should fork before call */
    int		bi_wait;	/* 1 if should wait for child */
    int	(*bi_fn)();		/* function which performs it */
};

#ifdef _DEFINE
struct biltin biltins[] = {
    /* Echo received data */
    "echo",		SOCK_STREAM,	1, 0,	echo_stream,
    "echo",		SOCK_DGRAM,	0, 0,	echo_dg,

    /* Internet /dev/null */
    "discard",	SOCK_STREAM,	1, 0,	discard_stream,
    "discard",	SOCK_DGRAM,	0, 0,	discard_dg,

    /* Return 32 bit time since 1970 */
    "time",		SOCK_STREAM,	0, 0,	machtime_stream,
    "time",		SOCK_DGRAM,	0, 0,	machtime_dg,

    /* Return human-readable time */
    "daytime",	SOCK_STREAM,	0, 0,	daytime_stream,
    "daytime",	SOCK_DGRAM,	0, 0,	daytime_dg,

    /* Familiar character generator */
    "chargen",	SOCK_STREAM,	1, 0,	chargen_stream,
    "chargen",	SOCK_DGRAM,	0, 0,	chargen_dg,
    0
    };
#else
EXTERN struct biltin biltins[];
#endif

EXTERN int debug;	/* Flag for "-d" option			   */
EXTERN int logflg; 	/* Flag for the "-l" option		   */
EXTERN int rereadflg;	/* Flag set by SIGHUP handler ("-c" option)*/
EXTERN int badfile; 	/* There were no good entries		   */
                        /* in the configuration file.		   */
EXTERN int portmap_up;	/* Used to avoid calling the portmapper	   */
                        /* more than once if it is not running	   */
EXTERN int lines;	/* Used to count lines in conf. file	   */

void	reread(),	/* Signal handler declarations */
	togglelog(),
	killprocess(),
	reapchild(),
	retry();

extern errno;
extern char *sys_errlist[];
extern long sigsetmask();

char	*servicename(), *getenv();
long	time();
struct passwd	*getpwnam();
char	*inet_ntoa();
void	delserv(), unregister();

#ifdef SETPROCTITLE
EXTERN char *Argv[3];
#endif /* SETPROCTITLE */

#define AUDIT_SUCCESS(event,s)
#define AUDIT_FAILURE(s)
