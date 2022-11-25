#ifndef lint
static  char rcsid[] = "@(#)on.c:	$Revision: 1.27.109.2 $	$Date: 93/09/02 09:13:36 $ ";
#endif

/* on.c 1.2 87/03/16 NFSSRC */

/*
 * on - user interface program for remote execution service
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */
#ifdef PATCH_STRING
static char *patch_3066="@(#) PATCH_9.0: on.o $Revision: 1.27.109.2 $ 93/09/01 PHNE_3066"
;
#endif

/* NOTE: on.c and where.c share a single message catalog.  	*/
/* For that reason we have allocated messages 1 through 40	*/
/* for the on.c file and messages 41 on for the where.c file.	*/
/* If we need more messages than 40 for this file we will need  */
/* to look at the numbers already used by where.c		*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#ifdef hpux
#include <termio.h>
/*
 * Undefine values in both termio.h and sgtty.h
 */
#include "bsdundef.h"

#include <sgtty.h>
#include <bsdterm.h>
#endif hpux
#include <rpcsvc/rex.h>

# define CommandName "on"	/* given as argv[0] */
# define AltCommandName "dbon"

#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

extern int errno;
char *getenv(), *catgets();
void usage();

/*
 * Note - the following must be long enough for at least two portmap
 * timeouts on the other side.
 */
struct timeval LongTimeout = { 123, 0 };

int Debug = 0;			/* print extra debugging information */
int Only2 = 0;			/* stdout and stderr are the same */
int Interactive = 0;		/* use a pty on server */
int NoInput = 0;		/* don't read standard input */

struct sgttyb OldFlags;		/* saved tty flags */
CLIENT *Client;			/* RPC client handle */
struct ttysize WindowSize;	/* saved window size */

#ifdef hpux
struct termio OldTermio;	/* Saved tty state -- more accurate for HP-UX*/
#endif hpux

#define LINES_DEFAULT 24
#define COLS_DEFAULT  80

#ifdef SIGWINCH
/*
 * window change handler - propagate to remote server 
 */
sigwinch()
{
     struct ttysize size;
     enum clnt_stat clstat;

     ioctl(0, TIOCGSIZE, &size);
     if (bcmp(&size,&WindowSize,sizeof size)==0) return;
     WindowSize = size;
     if (clstat = clnt_call(Client, REXPROC_WINCH,
    	xdr_rex_ttysize, &size, xdr_void, NULL, LongTimeout)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "on (size): ")));
		clnt_perrno(clstat);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "\r\n")));
     }
}
#endif SIGWINCH

/*
 * signal handler - propagate select signals to remote server.
 *                  Currently SIGINT, SIGQUIT and SIGTERM are propagated 
 *                  to the remote server.
 */
void
sendsig(sig)
     int sig;
{
     enum clnt_stat clstat;

     /* send sig to the server */
     if (clstat = clnt_call(Client, REXPROC_SIGNAL,
    	xdr_int, &sig, xdr_void, NULL, LongTimeout)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "on (signal): ")));
		clnt_perrno(clstat);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "\r\n")));
     }
}


main(argc, argv)
	int argc;
	char **argv;
{
	char *rhost;                    /* remote (server) host */
	char **cmdp;                    /* command to execute and args */
	char curdir[MAXPATHLEN];        /* user's current working directory */
	char wdhost[MAXPATHLEN];        /* host which has file system 
					 *  containing user's cwd
					 */
	char fsname[MAXPATHLEN];        /* file system containg user's cwd */
	char dirwithin[MAXPATHLEN];     /* sudir in fsname that is the cwd */
	struct rex_start rst;           /* parameters to start remote cmd */
	struct rex_result result;       /* result of call to rex_start */
	extern char **environ, *rindex(); 
	enum clnt_stat clstat;
	struct hostent *hp;
	struct sockaddr_in server_addr;
	int sock = RPC_ANYSOCK;
	int fd0, fd2;
	int selmask, zmask, remmask;
	int nfds, cc;
	static char buf[4096];
	struct rex_ttymode mode;        /* modes of the user's tty */
	int fm_result;                  /* result of call to findmount */
	int lines, cols;
	struct sigvec vec_sendsig;      /* signal handler structure for 
					 * signals that are propogated to
					 * the server 
					 */
#ifdef SIGWINCH
	struct sigvec  vec_sigwinch;    /* signal handler structure for 
					 * SIGWINCH
					 */
#endif SIGWINCH

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("on",0);
#endif NLS

	/* initialize signal handler structures */
	vec_sendsig.sv_handler = sendsig;
	vec_sendsig.sv_mask    = 0;
	vec_sendsig.sv_flags   = 0;

#ifdef SIGWINCH
	vec_sigwinch.sv_handler = sigwinch;
	vec_sigwinch.sv_mask    = 0;
	vec_sigwinch.sv_flags   = 0;
#endif SIGWINCH

	    /*
	     * we check the invoked command name to see if it should
	     * really be a host name.
	     */
	if ( (rhost = rindex(argv[0],'/')) == NULL) {
		rhost = argv[0];
	}
	else {
		rhost++;
	}

	while (argc > 1 && argv[1][0] == '-') {
	    switch (argv[1][1]) {
	      case 'd': Debug = 1;
	      		break;
	      case 'i': Interactive = 1;
	      		break;
	      case 'n': NoInput = 1;
	      		break;
	      default:
	      		printf((catgets(nlmsg_fd,NL_SETN,5, "Unknown option %s\n")),argv[1]);
	    }
	    argv++;
	    argc--;
	}
	if (strcmp(rhost,CommandName) && strcmp(rhost,AltCommandName)) {
	    cmdp = &argv[1];
	    Interactive = 1;
	} else {
	    if (argc < 2) {
	      usage();
	      exit(1);
	    }
	    rhost = argv[1];
	    cmdp = &argv[2];
	}

        /*
         * Can only have one of these options. 
         */
        if (Interactive && NoInput)
	  {
	    usage();
	    exit(1);
	  }

	if ((hp = gethostbyname(rhost)) == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "on: unknown host %s\n")), rhost);
		exit(1);
	}
next_ipaddr:
	bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = 0;	/* use pmapper */
	sock = RPC_ANYSOCK;
	if (Debug) nl_printf((catgets(nlmsg_fd,NL_SETN,8, "Got the host named %1$s (%2$s)\n")),
			rhost,inet_ntoa(server_addr.sin_addr));
	
	/* create a tcpip based client */
	if ((Client = clnttcp_create(&server_addr, REXPROG, REXVERS, &sock,
	    0, 0)) == NULL) {
		/*
		 * if ((rpc_createerr.cf_stat == RPC_SYSTEMERROR ) && 
		 *     ((rpc_createerr.cf_error.re_errno == ENETUNREACH) ||
		 *      (rpc_createerr.cf_error.re_errno == EHOSTUNREACH))) {
		 */

		/* when calling clnttcp_create with sin_port = 0 and a bad ip-address, it is
		 * more likely that RPC_PMAPFAILURE be returned. This is because pmap_getport
		 * gets called if sin_port == 0. The test commented out above is appropriate
		 * only if sin_port != 0.
		 */

		 if ((rpc_createerr.cf_stat == RPC_PMAPFAILURE) || (rpc_createerr.cf_stat == RPC_TIMEDOUT)) {
		        fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,34, "on: failed to reach %1$s at address %2$s\n")),rhost, inet_ntoa(*(u_long *)hp->h_addr));
		        if (hp && hp->h_addr_list[1]) {
			    hp->h_addr_list++;
		            fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,35, "on: trying %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
			    goto next_ipaddr;
		        } else {
			    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "on: cannot connect to server on %s\n")),
			    rhost);
			    exit(1);
		        }
		} else {
		    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "on: cannot connect to server on %s\n")),
			rhost);
			exit(1);
		}
	}
	if (Debug) printf((catgets(nlmsg_fd,NL_SETN,10, "TCP RPC connection created\n")));
	Client->cl_auth = authunix_create_default();
	  /*
	   * Now that we have created the TCP connection, we do some
	   * work while the server daemon is being swapped in.
	   */
	if (getwd(curdir) == 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,11, "on: can't find . (%s)\n")),curdir);
		exit(1);
	}
	/* Attempt to find the mount point of the user's current working
	 *  directory. It may be either local, remote via NFS or Remote via
	 *  RFA. The on command does not support a current directory accessed
         *  via rfa, so we print an error and exit.
         */
	fm_result = findmount(curdir,wdhost,fsname,dirwithin);
	if (fm_result == 0 ) {
		nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "on: can't locate mount point for %1$s (%2$s)\n")),
				curdir, dirwithin);
		exit(1);
	}
	else if (fm_result == -1) 
         {
	  fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "on: current directory (%s) is remote via RFA.\n"))
				,curdir);
	  fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "    RFA directories not supported by on \n")));
          exit(1);
	 } 
	if (Debug) nl_printf((catgets(nlmsg_fd,NL_SETN,15, "wd host %1$s, fs %2$s, dir within %3$s\n")),
		wdhost, fsname, dirwithin);

	Only2 = samefd(1,2);

	/* initialize rex start info */
	rst.rst_cmd = cmdp;
	rst.rst_env = environ;
	rst.rst_host = wdhost;
	rst.rst_fsname = fsname;
	rst.rst_dirwithin = dirwithin;
	rst.rst_port0 = makeport(&fd0);
	rst.rst_port1 =  rst.rst_port0;		/* same port as stdin */
	rst.rst_flags = 0;
	if (Interactive) {
		struct sgttyb newFlags;

		/* do not allow an interactive command if stdin is not a tty */
		if (isatty(0) == 0  )
		  {
		    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,33, "on: standard input (stdin) is not a tty\n")));
		    exit(1);
		  }

		rst.rst_flags |= REX_INTERACTIVE;
		bsd_gtty(0, &OldFlags);
#ifdef hpux
		/*
		 * Using our termio is more accurate than using [gs]tty()
		 * Also, get the rest of the modes, etc. now, since our
		 * emulations could change some of the values.
		 */
		(void) ioctl( 0, TCGETA, &OldTermio);
		bsd_tty_ioctl(0, TIOCGETC, &mode.more);
		bsd_tty_ioctl(0, TIOCGLTC, &mode.yetmore);
		bsd_tty_ioctl(0, TIOCLGET, &mode.andmore);
#endif hpux
		bsd_gtty(0, &newFlags);
		newFlags.sg_flags |= BSD_RAW;
		newFlags.sg_flags &= ~BSD_ECHO;
		bsd_stty(0, &newFlags);
	}

	/* use same port if stdout and stderr are the same file */
	if (Only2) {
		rst.rst_port2 = rst.rst_port1;
	} else {
		rst.rst_port2 = makeport(&fd2);
	}
	if (clstat = clnt_call(Client, REXPROC_START,
	    xdr_rex_start, &rst, xdr_rex_result, &result, LongTimeout)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,16, "on %s: ")), rhost);
		clnt_perrno(clstat);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,17, "\n")));
		Die(1);
	}
	if (result.rlt_stat != 0) {
		nl_fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,18, "on %1$s: %2$s")),rhost,result.rlt_message);
		Die(1);
	}
	if (Debug) printf((catgets(nlmsg_fd,NL_SETN,19, "Client call was made\r\n")));
	if (Interactive) {
	  /*
	   * Pass the tty modes along to the server 
	   */

	     mode.basic = OldFlags;
#ifndef hpux
	     ioctl(0, TIOCGETC, &mode.more);
	     ioctl(0, TIOCGLTC, &mode.yetmore);
	     ioctl(0, TIOCLGET, &mode.andmore);
#endif hpux
	     if (clstat = clnt_call(Client, REXPROC_MODES,
	    	xdr_rex_ttymode, &mode, xdr_void, NULL, LongTimeout)) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,20, "on (modes) %s: ")), rhost);
			clnt_perrno(clstat);
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,21, "\r\n")));
	     }
#ifdef TIOCGSIZE
         {
	     WindowSize.ts_lines = 0;
	     WindowSize.ts_cols = 0;
	     ioctl(0, TIOCGSIZE, &WindowSize);
	 } 
             if (WindowSize.ts_lines <= 0 && WindowSize.ts_cols <= 0)
	    {
		char *ttype, tbuf[BUFSIZ];
		int lines = -1, cols = -1;

		/* The tgetnum routine does a getenv("LINES") for "li" and 
		 * a getenv("COLUMNS") for "co" beofre checking the default 
		 * entry for the terminal so we do not need to do this.
		 * However, in the case that TERM is not defined or tgetent
		 * fails we need to check to see if LINES or COLUMNS is 
		 * set. *mjk 11/28/88*
		 */

		if ((ttype = getenv("TERM")) && tgetent(tbuf, ttype) == 1) {
			/* tgetnum returns -1 if value not present */
			lines = tgetnum("li");
			cols = tgetnum("co");
		}
		else
		  { 
		    /* check the environment variables for LINES and COLUMNS */
		    char *linestr;          /* value of env. var. LINES */
		    char *colstr;           /* value of env. var. COLUMNS */
		    linestr = getenv("LINES");
		    colstr  = getenv("COLUMNS");

		    /* atoi returns 0 if no integer can be fromed */
		    if (linestr != NULL)
		      lines = atoi(linestr);
		    if (colstr != NULL)
		      cols = atoi(colstr);
		  }

		if (lines <= 0 )
		  lines = LINES_DEFAULT;
		if (cols <= 0 )
		  cols = COLS_DEFAULT;

		WindowSize.ts_lines = lines;
		WindowSize.ts_cols = cols;
	    }
#endif TIOCGSIZE
	     if (clstat = clnt_call(Client, REXPROC_WINCH,
	    	xdr_rex_ttysize, &WindowSize, xdr_void, NULL, LongTimeout)) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,22, "on (size) %s: ")), rhost);
			clnt_perrno(clstat);
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,23, "\r\n")));
	     }
#ifdef SIGWINCH
	     sigvector(SIGWINCH, &vec_sigwinch, NULL);
#endif SIGWINCH
	     sigvector(SIGINT, &vec_sendsig, NULL);
             sigvector(SIGQUIT, &vec_sendsig, NULL);
             sigvector(SIGTERM, &vec_sendsig, NULL);
	}
	doaccept(&fd0);
	remmask = (1 << fd0);
	if (Debug) printf((catgets(nlmsg_fd,NL_SETN,24, "accept on stdout\r\n")));
	if (!Only2) {
		doaccept(&fd2);
		shutdown(fd2, 1);
		if (Debug) printf((catgets(nlmsg_fd,NL_SETN,25, "accept on stderr\r\n")));
		remmask |= (1 << fd2);
	}
	if (NoInput) {
		  /*
		   * no input - simulate end-of-file instead
		   */
		zmask = 0;
		shutdown(fd0, 1);
	}
	else {
		  /*
		   * set up to read standard input, send to remote
		   */
		zmask = 1;
	}
	while (remmask) {
		selmask = remmask | zmask;
		nfds = select(32, &selmask, 0, 0, 0);
		if (nfds <= 0) {
			if (errno == EINTR) continue;
			perror((catgets(nlmsg_fd,NL_SETN,26, "on: select")));
			Die(1);
		}
		if (selmask & (1<<fd0)) {
			cc = read(fd0, buf, sizeof buf);
			if (cc > 0)
				write(1, buf, cc);
			else
				remmask &= ~(1<<fd0);
		}
		if (!Only2 && selmask & (1<<fd2)) {
			cc = read(fd2, buf, sizeof buf);
			if (cc > 0)
				write(2, buf, cc);
			else
				remmask &= ~(1<<fd2);
		}
		if (!NoInput && selmask & (1<<0)) {
			cc = read(0, buf, sizeof buf);
			if (cc > 0) 
				write(fd0, buf, cc);
			else {
				/*
				 * End of standard input - shutdown outgoing
				 * direction of the TCP connection.
				 */
				zmask = 0;
				shutdown(fd0, 1);
			}
		}
	}
	close(fd0);
	if (!Only2)
	    close(fd2);
	if (clstat = clnt_call(Client, REXPROC_WAIT,
	    xdr_void, 0, xdr_rex_result, &result, LongTimeout)) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,27, "on: ")));
		clnt_perrno(clstat);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,28, "\r\n")));
		Die(1);
	}
	Die(result.rlt_stat);
}

/*
 * like exit, but resets the terminal state first 
 */
Die(stat)
{
  if (Interactive) {
#ifdef hpux
      if ( ioctl(0, TCSETAW, &OldTermio) < 0 )
	perror((catgets(nlmsg_fd,NL_SETN,29, "on: ioctl(TCSETAW)")));
#else
      stty(0,&OldFlags);
#endif hpux
      printf((catgets(nlmsg_fd,NL_SETN,30, "\r\n")));
  }
  exit(stat);
}

#ifndef hpux
remstop()
{
	exit(23);
}
#endif 

  /*
   * returns true if we can safely say that the two file descriptors
   * are the "same" (both are same file).
   */
samefd(a,b)
{
    struct stat astat, bstat;
    if (fstat(a,&astat) || fstat(b,&bstat)) return(0);
    if (astat.st_ino == 0 || bstat.st_ino == 0) return(0);
    return( !bcmp( &astat, &bstat, sizeof(astat)) );
}


/*
 * accept the incoming connection on the given
 * file descriptor, and return the new file descritpor
 */
doaccept(fdp)
	int *fdp;
{
	int fd;

	fd = accept(*fdp, 0, 0);
	if (fd < 0) {
		perror((catgets(nlmsg_fd,NL_SETN,31, "on: accept")));
#ifndef hpux		
		remstop();
		exit(1);
#else
		exit(23);
#endif
	}
	close(*fdp);
	*fdp = fd;
}

/*
 * create a socket, and return its the port number.
 */
makeport(fdp)
	int *fdp;
{
	struct sockaddr_in sin;
	int fd, len;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror((catgets(nlmsg_fd,NL_SETN,32, "on: socket")));
		exit(1);
	}
	bzero((char *)&sin, sizeof sin);
	sin.sin_family = AF_INET;
	bind(fd, &sin, sizeof sin);
	len = sizeof sin;
	getsockname(fd, &sin, &len);
	listen(fd, 1);
	*fdp = fd;
	return (htons(sin.sin_port));
}

void
usage()
{
  fprintf(stderr, 
	  (catgets(nlmsg_fd,NL_SETN,6,"Usage: on [-i|-n] [-d] host [command [argument] ... ]\n")));
}
