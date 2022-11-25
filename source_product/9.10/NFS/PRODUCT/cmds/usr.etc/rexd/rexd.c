#ifndef lint
static  char rcsid[] = "@(#)rpc.rexd:	$Revision: 1.47.109.3 $	$Date: 93/09/02 09:09:41 $ ";
#endif

/* rexd.c 1.1 87/03/16 NFSSRC */

/*
 * rexd - a remote execution daemon baed on SUN Remote Procedure Calls
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#ifdef PATCH_STRING
static char *patch_3066="@(#) PATCH_9.0: rexd.o $Revision: 1.47.109.3 $ 93/09/01 PHNE_3066";
#endif

/* NOTE: rexd.c, mount_nfs.c and unix_login.c share a single message	*/
/* catalog (rexd.cat).  For that reason we have allocated messages 	*/
/* 1 through 40 for rexd.c, 41 through 80 for mount_nfs.c and from 81   */
/* on for unix_login.c.  If we need more than 40 messages in this file  */
/* we will need to take into account the message numbers that are 	*/
/* already used by the other files.					*/
/* Rexd now also uses messages 201 and beyond *mjk 12/22/88*            */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <sys/param.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <mntent.h>
#include <errno.h>
#include <sys/pstat.h>

#ifdef hpux
#include <sgtty.h>
#include <bsdterm.h>
#define SIGCHLD SIGCLD
#include <fcntl.h>
#define dup2(f1, f2) if ((f2) != (f1)) { close((f2)); fcntl((f1),F_DUPFD,(f2));}
#include <sys/termio.h>
#endif hpux
#include <rpcsvc/rex.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef DUX        /* added 8/31/88 to support rex on diskless: mjk */
#include <cluster.h>
#endif DUX

#ifdef hpux
#include <audnetd.h>
struct sockaddr_in sin, myaddr;
struct authunix_parms savecred;
char   audit_dir[1300];		/* Room for hostname[256]:path[1024] + some */
#endif hpux

/*
**	include the TRACE macros
*/
#include <arpa/trace.h>
# define	TRACEFILE	"/tmp/rexd.trace"

#ifdef hpux
/*
**	maximum number of characters in a pty path ...
*/
# define	MAX_PTY_LEN	32
#endif 

# define ListnerTimeout 300	/* seconds listner stays alive */
# define WaitLimit 10		/* seconds to wait after io is closed */

# define ISDIR    0040000      /* indicates file is directory mknod(2) */
# define CHLD_STOPPED 00177    /* used to determine if child process has
				  stopped or terminated */

SVCXPRT *ListnerTransp;		/* non-null means still a listner */
static int Argc;		/* saved argument count (for setproctitle) */
static char **Argv;		/* saved argument vector (for ps) */
fd_set HelperMask;		/* files for interactive mode */
int ptysize_set = FALSE;        /* have we set the pty size yet */

int child_sig;                  /* This flag indicates that we received a
				   SIGCHLD. It is needed because we can 
				   get a context swith at a bad time in 
				   the infinate loop in main causing select
				   to fail. See the coments in the loop */

/* globals used by rex_argparse */
int check_rhosts = FALSE;        /* flag indicating if daemon was started
				   with option to check client hosts against
				   hosts listed in hosts.equiv(4) and .rhosts.
				   Added security. Set by the -r option */
char LogFile[64];               /* Log file, for logging erros and warnings */

/* globals used for setting up a pty when the command is interactive */
int Master, Slave;		/* sides of the pty */
char *sname[MAX_PTY_LEN];       /* name of the slave pty */
int pipe1[2];                   /* pipe for parent to tell child it has 
				 * received and set the modes for the pty
				 */

#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

#ifdef BFA

write_BFAdbase()
{
        _UpdateBFA();
}
#endif /* BFA */

/* procedure to parse options to rexd */
void rex_argparse();

        /* The following structures are required to set up signal handlers
	 * with sigvector. Sigvector must be used instead of signal to 
	 * allow rexd to detect a stopped child. This allows us to 
	 * effectively ignore job control generated SIGTSTP signals. The 
	 * alternative is to be like Sun and hang.
	 */
	struct   sigvec    vec_dfl,
	                   vec_ign,
#ifdef BFA
	                   vec_usr2,
#endif	/* BFA */
	                   vec_sigchld;

#ifndef hpux
        struct   sigvec    vec_sigalrm;
#endif

main(argc, argv)
	int argc;
	char **argv;
{
	  /*
	   * the server is a typical RPC daemon, except that we only
	   * accept TCP connections.
	   */
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	int dorex();
	void CatchChild();
#ifndef hpux
	void ListnerTimer;
#endif

	fd_set	readfds, read_mask;
	fd_mask	*readfds_ptr, *read_mask_ptr, *HelperMask_ptr, 
		*svc_fds_ptr, or_mask;
	int	fd_set_len, fd_set_len_copy, num_fds;
	SVCXPRT *svcfd_create();

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("rexd",0);
#endif NLS
	STARTTRACE(TRACEFILE);
	TRACE("main: start");

	/*
	 * Remember the start and extent of argv for setproctitle().
	 * Open the console for error printouts, but don't let it be
	 * our controlling terminal.
	 */
	Argv = argv;
	Argc = argc;

	/* initialize the sigvec structures */
	vec_dfl.sv_handler = SIG_DFL;
	vec_dfl.sv_mask    = 0;
	vec_dfl.sv_flags   = 0;

	vec_ign.sv_handler = SIG_IGN;
	vec_ign.sv_mask    = 0;
	vec_ign.sv_flags   = 0;
#ifdef BFA
	vec_usr2.sv_handler = write_BFAdbase;
	vec_usr2.sv_mask    = 0;
	vec_usr2.sv_flags   = 0;
#endif /* BFA */
#ifndef hpux
	vec_sigalrm.sv_handler = ListnerTimer;
	vec_sigalrm.sv_mask    = 0;
	vec_sigalrm.sv_flags   = 0;
#endif

	vec_sigchld.sv_handler = CatchChild;
	vec_sigchld.sv_mask    = 0;
	vec_sigchld.sv_flags   = SV_BSDSIG; 

	TRACE("main: setting stdout/stderr to /dev/console");
	close(1);
	close(2);
	(void) open("/dev/console", 1);
	dup(1);
	NoControl();
	sigvector(SIGCHLD, &vec_sigchld, NULL);

#ifdef BFA
	sigvector(SIGUSR2, &vec_usr2, NULL);
#endif /* BFA */

#ifndef hpux
	sigvector(SIGALRM, &vec_sigalrm, NULL);
#endif

# ifdef NOINETD
	if ((ListnerTransp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL) {
		logmsg( (catgets(nlmsg_fd,NL_SETN,1, "rexd: svctcp_create: error\n")));
		exit(1);
	}
	pmap_unset(REXPROG, REXVERS);
	if (!svc_register(ListnerTransp, REXPROG, REXVERS, 
			dorex, IPPROTO_TCP)) {
		logmsg( (catgets(nlmsg_fd,NL_SETN,2, "rexd: service rpc register: error\n")));
		exit(1);
	}
# else NOINETD
#ifdef hpux
	TRACE("main: calling getpeername with 0");
	if ( getpeername(0, &sin, &len)) {
	  TRACE("rexd: getpeername failed ");
	  logmsg((catgets(nlmsg_fd,NL_SETN,201, "rexd: getpeername failed with errno = %d\n")), errno);
	  exit(1);
	}
	get_myaddress(&myaddr);
	TRACE("main: calling svcfd_create");
	if ((ListnerTransp = svcfd_create(0, 0, 0)) == NULL) {
	TRACE("main: svcfd_create failed");	 
#else hpux
	if ((ListnerTransp = svctcp_create(0, 0, 0)) == NULL) {
#endif hpux
		logmsg( (catgets(nlmsg_fd,NL_SETN,3, "rexd: svctcp_create: error\n")));
		exit(1);
	}
	TRACE("main: calling svc_register");
	if (!svc_register(ListnerTransp, REXPROG, REXVERS, dorex, 0)) {	
	        TRACE("main: svc_register failed");
        	logmsg( (catgets(nlmsg_fd,NL_SETN,4, "rexd: service rpc register: error\n")));
		exit(1);
	}
#ifndef hpux
	alarm(ListnerTimeout);
#else hpux
	ListnerTransp = (SVCXPRT *)NULL; /* Not really a listener */
#endif hpux
# endif NOINETD

/*
**  parse arguments and set appropriate global values based on arguments 
*/
	LogFile[0] = '\0';
	TRACE("main: main calling rex_argparse ...");
	rex_argparse(argc,argv);
	TRACE2("main: LogFile = %s", LogFile);
	(void) startlog(*argv,LogFile);
/*
 * normally we would call svc_run() at this point, but we need to be informed
 * of when the RPC connection is broken, in case the other side crashes.
 */
	/*
	 *	selecting on FD_SETSIZE bits can be fairly expensive. So
	 *	we will select only on 32 fds for the time being. We can
	 *	easily increase this if future changes make it necessary
	 *	by setting num_fds to appropriate values as follows. (The
	 *	mask will be left at full size, fd_set).
	 *
	 *   	num_fds = xxx;
	 *
	 *	- prabha
	 */

	num_fds = 32;
	FD_ZERO(&readfds);
	fd_set_len_copy = howmany(num_fds, NFDBITS);

	TRACE("main: entering loop that does async event processing");
	while (TRUE) {
	    extern int errno;

	    /* Note: If the Child dies and we experience a context 
	    **       switch to handle the signal between the statement
	    **       which sets readfds and the select call it is 
	    **       possible to get a -1 and EBADF from the select call
	    **       since the signal handler closes some of the files.
	    **       In this case we ignore the EBADF and reset readfds
	    **       and try again. child_sig tells us if had a context 
	    **       switch to the signal handler so we don't ignore all
	    **       EBADFs.
	    */
	    child_sig = FALSE;
	    TRACE2("main: the value of readfds is %d",readfds);

	    svc_fds_ptr = svc_fdset.fds_bits;
	    or_mask = 0;

	    fd_set_len = fd_set_len_copy;
	    do {
		or_mask |= *(svc_fds_ptr++);
	    } while (--fd_set_len);

	    if (or_mask == 0) {
        	TRACE("main: lost socket connection, cleanup rex");
		rex_cleanup();		
        	TRACE("main: rex cleanup complete");
		exit(1);
	    }

	    svc_fds_ptr = svc_fdset.fds_bits;
	    HelperMask_ptr = HelperMask.fds_bits;
	    readfds_ptr = readfds.fds_bits;

	    fd_set_len = fd_set_len_copy;
	    do {
		*(readfds_ptr++) = *(svc_fds_ptr++) | *(HelperMask_ptr++);
	    } while (--fd_set_len);
	    TRACE("main: readfds changed to (svc_fdset | HelperMask)");

	    TRACE("main: calling select");
	    switch (select(num_fds, &readfds, (int *)0, (int *)0, 0)) {
	      case -1:  TRACE("main: select returned -1");
		        if (errno == EINTR) continue;
		        if ( (errno == EBADF) && (child_sig) ) continue;
		        TRACE2("main: select failed errno= %d", errno);
	      		log_perror((catgets(nlmsg_fd,NL_SETN,5,"rexd: select failed, errno = %d")), errno);
			exit(1);
	      case 0:   
		        TRACE("main: select retruned 0");
	      		logmsg((catgets(nlmsg_fd,NL_SETN,6, "rexd: Select returned zero\n")));
			continue;
	      default:  
	    		HelperMask_ptr = HelperMask.fds_bits;
			readfds_ptr = readfds.fds_bits;
	    		or_mask = 0;
	    		fd_set_len = fd_set_len_copy;
	    		do {
				or_mask |= (*(HelperMask_ptr++) & 
					    *(readfds_ptr++));
	    		} while (--fd_set_len);

	      		if (or_mask) { /* read from Master Pty/InputSocket */
			    TRACE("main: calling HelperRead");
			    HelperRead(&readfds);
			}

			/* now if the rpc socket is set, read from it too */
	        	TRACE("main: calling svc_getreq");
                        readfds_ptr = readfds.fds_bits;
                        svc_fds_ptr = svc_fdset.fds_bits;
                        read_mask_ptr = read_mask.fds_bits;
	    		fd_set_len = fd_set_len_copy;
                        do {
                            *(read_mask_ptr++) = *(readfds_ptr++) &
                                                 *(svc_fds_ptr++);
                        } while (--fd_set_len);

#ifdef NEW_SVC_RUN
			svc_getreqset_ms(&read_mask, num_fds);
	        	TRACE2("main: returned from svc_getreq_ms, num_fds = %d",num_fds);
#else /* NEW_SVC_RUN */
			svc_getreqset(&read_mask);
	        	TRACE("main: returned from svc_getreq");
#endif /* NEW_SVC_RUN */
	    }
	}
}


#ifndef hpux
/*
 * This function gets called after the listner has timed out waiting
 * for any new connections coming in.
 */
void
ListnerTimer()
{
  TRACE("ListenerTimer: calling svc_destroy");
  svc_destroy(ListnerTransp);
  TRACE("ListenerTimer: returned from svc_destroy");
  exit();
}
#endif

/*
 * dorex - handle one of the rex procedure calls, dispatching to the 
 *	correct function.
 */
dorex(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	struct rex_start *rst;
	struct rex_result result;
	int max_fds;
	
	TRACE("dorex: SOP");

#ifndef hpux
/* can only hit if inetd supports a tcp wait service */
	if (ListnerTransp) {
		  /*
		   * First call - fork a server for this connection
		   */
		int fd, pid;

		TRACE("dorex: received listen socket for a server");
		pid = fork();
		if (pid > 0) {
		    /*
		     * Parent - return to service loop to accept further
		     * connections.
		     */
		        TRACE("dorex: start of parent");
			alarm(ListnerTimeout);
			xprt_unregister(transp);
		     	close(transp->xp_sock);
			TRACE("dorex: parent returning to main loop");
			return;
		}
		  /*
		   * child - close listner transport to avoid confusion
		   * Also need to close all other service transports
		   * besides the one we are interested in.
		   */
		alarm(0);
		TRACE("dorex: child closing listen socket");
		if (transp != ListnerTransp) {
			close(ListnerTransp->xp_sock);
			xprt_unregister(ListnerTransp);
		}
		ListnerTransp = NULL;
		TRACE("dorex: close daemon's file descriptors");
        	max_fds = getnumfds();
		for (fd=0; fd<max_fds; fd++)
		  if (fd != transp->xp_sock && FD_ISSET(fd, &svc_fds) ) {
			close(fd);
			FD_CLR(fd, &svc_fds);
		  }
	}
#endif hpux

	TRACE("dorex: entering loop to determine which proc was called");
	switch (rqstp->rq_proc) {
	case NULLPROC:
	        TRACE("dorex: NULLPROC  called");
		if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
		        TRACE("dorex: sendreply failed in NULLPROC");
			logmsg( (catgets(nlmsg_fd,NL_SETN,7, "rexd: nullproc err")));
			exit(1);
		}
		return;

	case REXPROC_START:
		TRACE("dorex: start of REXPROC_START");
		rst = (struct rex_start *)malloc(sizeof (struct rex_start));
		bzero((char *)rst, sizeof *rst);
		if (svc_getargs(transp, xdr_rex_start, rst) == FALSE) {
		        TRACE("dorex: svc_getargs failed in REXSTART_PROC");
			svcerr_decode(transp);
			exit(1);
		}
		if (rqstp->rq_cred.oa_flavor != AUTH_UNIX) {
			svcerr_auth(transp);
			exit(1);
		}
		TRACE("dorex: calling rex_start");
		result.rlt_stat = rex_start(rst,
			(struct authunix_parms *)rqstp->rq_clntcred,
			&result.rlt_message, transp->xp_sock);
		TRACE3("dorex: rex_start returned status %d, %s", 
		                          result.rlt_stat,result.rlt_message);
		if (svc_sendreply(transp, xdr_rex_result, &result) == FALSE) {
			logmsg( (catgets(nlmsg_fd,NL_SETN,8, "rexd: reply failed\n")));
                        TRACE("dorex: svc_sendreply failed in REXPROC_START");
			rex_cleanup();
                        TRACE("dorex: rex_cleanup retruned");
                        exit(1);
		}
		if (result.rlt_stat) {
                        TRACE("dorex: rex_start failed calling rex_cleanup");
			rex_cleanup();
                        TRACE("dorex: rex_cleanup returned");
			exit(0);
		}
		TRACE("dorex: REXPROC_START normall return");
		return;

	case REXPROC_MODES:
		{
		    struct rex_ttymode mode;		    
		    TRACE("dorex: start of REXPROC_MODES");
		    if (svc_getargs(transp, xdr_rex_ttymode, &mode)==FALSE) {
		        TRACE("dorex: svc_getargs failed");
			svcerr_decode(transp);
			exit(1);
		    }
		    TRACE("dorex: calling SetPtyMode");
		    SetPtyMode(&mode);
		    TRACE("dorex: returned from SetPtyMode");

		    if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
		        TRACE("dorex: sendreply failed");
			logmsg( (catgets(nlmsg_fd,NL_SETN,9, "rexd: mode reply failed")));
			exit(1);
		    }
		}
		TRACE("dorex: REXPROC_MODES normal retrun");
		return;

	case REXPROC_WINCH:
		{
		    struct ttysize size;

		    TRACE("dorex: start of REXPROC_WINCH");
		    if (svc_getargs(transp, xdr_rex_ttysize, &size)==FALSE) {
		        TRACE("dorex: svc_getargs failed");
			svcerr_decode(transp);
			exit(1);
		    }
		    TRACE("dorex: calling SetPtySize");
		    SetPtySize(&size);
		    TRACE("dorex: retruned form SetPtySize");
		    if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
		        TRACE("dorex: sendreply failed");
			logmsg( (catgets(nlmsg_fd,NL_SETN,10, "rexd: window change reply failed")));
			exit(1);
		    }
		}
		TRACE("dorex: REXPROC_WINCH normal return");
		return;

	case REXPROC_SIGNAL:
		{
		    int sigNumber;

		    TRACE("dorex: start of REXPROC_SIGNAL");
		    if (svc_getargs(transp, xdr_int, &sigNumber)==FALSE) {
		        TRACE("dorex: svc_getargs failed");
			svcerr_decode(transp);
			exit(1);
		    }
		    TRACE("dorex: calling SendSignal");
		    SendSignal(sigNumber);
		    TRACE("dorex: returned from SendSignal");
		    if (svc_sendreply(transp, xdr_void, 0) == FALSE) {
		        TRACE("dorex: svc_sendreply failed");
			logmsg( (catgets(nlmsg_fd,NL_SETN,11, "rexd: signal reply failed")));
			exit(1);
		    }
		}
		TRACE("dorex: REXPORC_SIGNAL normall return");
		return;

	case REXPROC_WAIT:
		TRACE("dorex: start of REXPROC_WAIT");
		result.rlt_stat = rex_wait(&result.rlt_message);
		TRACE2("dorex: rex_wait returned with status %d",
		                            result.rlt_stat);
		if (svc_sendreply(transp, xdr_rex_result, &result) == FALSE) {
		        TRACE("dorex: svc_sendreply failed");
			logmsg( (catgets(nlmsg_fd,NL_SETN,12, "rexd: reply failed\n")));
			exit(1);
		}
		TRACE("dorex: REXPROC_WAIT normal return");
		exit(0);
		/* NOTREACHED */

	default:
		TRACE("dorex: default case hit");
		svcerr_noproc(transp);
		exit(1);
	}
}

int HasHelper = 0;		/* must kill helpers (interactive mode) */
int child = 0;			/* pid of the executed process */
int ChildStatus = 0;		/* saved return status of child */
int ChildDied = 0;		/* true when above is valid */
char nfsdir[MAXPATHLEN];	/* file system we mounted */
char *tmpdir = NULL;		/* where above is mounted, NULL if none */
                                /* directory where all mounts are done */
char mntdir[MAXPATHLEN] =
           "/usr/spool/rexd";
                                /* default pathname of the mount point */
char mntpnt[MAXPATHLEN] =      
           "/usr/spool/rexd/rexdXXXXXX";  
           

/*
 * signal handler for SIGCHLD - called when user process dies
 */
void
CatchChild()
{
  int pid, status;

  TRACE("CatchChild:  SOP");  
  /* we set the flag child_sig to let the loop in main know that we received
  **  a SIGCHLD.The loop in main must figure out is this occured at a bad time 
  */
  child_sig = TRUE;

  /* check to see if the child has stopped or terminated */
  TRACE("Calling wait3 to see if child has terminated");
  pid = wait3(&status, (WUNTRACED | WNOHANG), NULL);
  /* reset the signal handler after calling wait3 */
  TRACE("Resetting the signal handler after calling wait3");
  sigvector(SIGCHLD, &vec_sigchld, NULL);

  if (pid==child) {
      TRACE("Flushing characters child has written to the PTY");
      /* Flush any characters that the child has written to the Pty 
      **   to the OutputSocket if this is an interactive command.
      */
      if (HasHelper)
	FlushPty();

      /* Check to see if the child has terminated or stopped. If the 
       * child has stopped then we restart it. This sounds a little 
       * silly but there is a reason we do this. If the child stopped
       * it was causeed by the fact that the associated PTY processed
       * a Ctrl Z and sent the process a SIGTSTP (this is used for job
       * control). Since there is no way for the client to restart the 
       * stopped process (except to login to the server and send a 
       * SIGCONT we just send it a SIGCONT, this effectivelly ignoring
       * the SIGTSTP.
       */
      
      /* determine if the child is stopped due to a SIGTSTP, lower order 
       * 8 bits are 1s if stopped and 0s if terminated.
       */
      if ( (status & CHLD_STOPPED) == CHLD_STOPPED )
	{
	  TRACE2("CatchChild: The child process stopped sig=%d, start it"
		 , (status >> 8));
	  /* start the process back up */
	  SendSignal(SIGCONT);
	}
      else 
	{
	  TRACE("CatchChild: The child process has died");
	  ChildStatus = status;
	  ChildDied = 1;
	  if (HasHelper) { 
	      TRACE("CatchChild: calling KillHelper");
	      KillHelper(child, sname);
	      TRACE("CatchChild: returned from KillHelper");
	      HasHelper = 0;
	  }
	}
    }
  TRACE("CatchChild: EOP");
}

/*
 * rex_wait - wait for command to finish, unmount the file system,
 * and return the exit status.
 * message gets an optional string error message.
 */
rex_wait(message)
	char **message;
{
	static char error[1024];
	int count;

	TRACE("rex_wait: SOP");
	*message = error;
	/* initialize error to a null string */
	error[0]='\0';
	if (child == 0) {
		errprintf(error,(catgets(nlmsg_fd,NL_SETN,13, "No process to wait for!\n")));
		return (1);
	}
	TRACE("rex_wait: calling kill");
	kill(child, SIGHUP);
	for (count=0;!ChildDied && count<WaitLimit;count++)
		sleep(1);
	TRACE("rex_wait: calling rex_cleanup");
	rex_cleanup();
	TRACE("rex_wait: return child status to caller");
	if (ChildStatus & 0xFF)
		return (ChildStatus);
	return (ChildStatus >> 8);
}


/*
 * cleanup - unmount and remove our temporary directory
 */
rex_cleanup()
{
    TRACE("rex_cleanup: SOP");
    if (HasHelper) {
	TRACE("rex_cleanup: calling KillHelper");
	KillHelper(child);
    	HasHelper = 0;
    } else if (child) {
	killpg(child,SIGTERM);
    }

    if (tmpdir) {
	chdir("/");
	TRACE("rex_cleanup: calling umount");
	if (umount_nfs(nfsdir, tmpdir)) {
	    TRACE2("rex_cleanup: could not umount %s",nfsdir);
	    logmsg((catgets(nlmsg_fd,NL_SETN,14, "rexd: couldn't umount %s\n")),
			 nfsdir);
  	}
	if (rmdir(tmpdir) < 0) 
	    if (errno != EBUSY) log_perror((catgets(nlmsg_fd,NL_SETN,202, "rmdir")));
	tmpdir = NULL;
    }

    /*
     * Audit the termination of the users access, giving the information
     * We logged when we started this process....
     */
    audit_daemon(  NA_SERV_REXD, NA_RSLT_SUCCESS,
		   check_rhosts ? NA_VALI_RUSEROK : NA_VALI_UID,
		   sin.sin_addr.s_addr, savecred.aup_uid, 
		   myaddr.sin_addr.s_addr, savecred.aup_uid,
		   NA_STAT_STOP, NA_MASK_NONE , audit_dir);
}


/*
 * This function does the server work to get a command executed
 * Returns 0 if OK, nonzero if error
 */
rex_start(rst, ucred, message, sock)
	struct rex_start *rst;
	struct authunix_parms *ucred;
	char **message;
	int sock;
{
	char *index(), *mktemp();
	char hostname[255];
	char *p, *wdhost, *fsname, *subdir;
	char dirbuf[1024];
	static char error[1024];
	char defaultShell[1024];
	int len, int_t;
	int fd0, fd1, fd2;
#ifdef TRACEON
	int tfd;
#endif /* TRACEON */
	extern char **environ;
	struct stat tsb;   
	int    estat;         /* status returned by execcp in child process */
	void   execerr();     /* function to print error when exec fails    */
	char   modes_set;     /* buffer used by child to read from pipe     */
	int    max_fds;

	TRACE("rex_start: SOP");
	if (child) {	/* already started */
	        TRACE("rex_start: Child already started");
		killpg(child, SIGKILL);
		return (1);
	}
	*message = error;
	/* initialize error to a null string */
	error[0]='\0';

	sigvector(SIGCHLD, &vec_sigchld, NULL);

	/*
	 * Save credentials for later use in auditing this connection.
	 */
	savecred = *ucred;

	TRACE("rex_start: calling ValidUser");
	if (ValidUser(ucred->aup_machname,ucred->aup_uid,error,defaultShell))
	  {
	    /* create an audit event indicating that access failed */
	    audit_daemon(NA_SERV_REXD,NA_RSLT_FAILURE,
		     check_rhosts ? NA_VALI_RUSEROK : NA_VALI_UID,
		     sin.sin_addr.s_addr, savecred.aup_uid, 
                     myaddr.sin_addr.s_addr, savecred.aup_uid,
                     NA_STAT_START, NA_MASK_NONE,"");
	    return(1);
	  }

	TRACE("rex_start: user is valid");
	wdhost = rst->rst_host;
	fsname = rst->rst_fsname;
	subdir = rst->rst_dirwithin;
	gethostname(hostname, 255);
#ifdef DUX     /* added 8/31/88 to support rex on diskless: mjk */
	if (cnodeid() != 0) 
	  { 
	    /* we are in a diskless cluster, see if wdhost is in the cluster */
	    if (getccnam(wdhost) != NULL )
	      {
		/* The directory is on one of the cluster members, so we see
		** it in the same place as wdhost. Therefore, we set wdhost
		**  to ourself and save a mount 
		*/
		strcpy(wdhost,hostname);
	      }
	  }
#endif DUX 
	if (strcmp(wdhost, hostname) != 0) {
	        TRACE("rex_start: requested directory is remote");
		strcpy(dirbuf,wdhost);
		strcat(dirbuf,":");
		strcat(dirbuf,fsname);
		if (! AlreadyMounted(dirbuf)) {
		  /*
		   * The requested directory is not mounted anywhere,
		   * so try to mount our own copy of it.  We set nfsdir
		   * so that it gets unmounted later, and tmpdir so that
		   * it also gets removed when we are done.
		   */
		    TRACE("rex_start: need to mount requested directory");
		    /* check to see that mntdir exists and is a directory */
		    if (stat(mntdir, &tsb) == -1) {
			/* mntdir does not exist */
			TRACE2("rex_start: could not stat mntdir %s", mntdir);
			errprintf(error,
			   (catgets(nlmsg_fd,NL_SETN,203, "rexd: mountdir (%s) is not a directory\n")),mntdir);
			return(1);
		    } else if ( !(tsb.st_mode & ISDIR)) {
			TRACE4("st_mode = %x ISDIR = %x  AND = %x",
			       tsb.st_mode, ISDIR, (tsb.st_mode & ISDIR) );
			/* mntdir exists but is not a directory */
			TRACE2("rex_start: %s is not a directory", mntdir);
			errprintf(error,
			   (catgets(nlmsg_fd,NL_SETN,204, "rexd: mountdir (%s) is not a directory\n")),mntdir);
			return(1);
		    }
		    tmpdir = mktemp(mntpnt);
		    if (mkdir(tmpdir, 0777)) {
			TRACE("rex_start: could not make tmpdir");
			errprintf(error,
		   		(catgets(nlmsg_fd,NL_SETN,220, "rexd: attempt to  make temporary dir %s for mounting failed\n")),tmpdir);
			log_perror(tmpdir);
			return(1);
		    }
		    if (mount_nfs(dirbuf, tmpdir, error)) {
		        TRACE("rex_start; can't mount requested directory ");
			if (error == NULL ) { /* no error string from mount_nfs */
			    errprintf(error,
				  (catgets(nlmsg_fd,NL_SETN,205, "rexd: can't mount file system %1$s from host %2$s\n")),dirbuf,wdhost);
			} else { /* mount_nfs returned a better error */
				    errprintf(error,"%s",error);
			}
			return(1);
		    }
		    strcpy(nfsdir, dirbuf);
		    strcpy(dirbuf, tmpdir);
		    strcat(dirbuf, subdir);
		} else {
		  /*
		   * The requested directory is already mounted, so
		   * just change to it. It might be mounted in a
		   * different place, so be careful.
		   * (dirbuf is modified in place!)
		   */
			strcat(dirbuf, subdir);
		}
	} else {
		  /*
		   * The requested directory is local to our machine,
		   * so just change to it.
		   */
		strcpy(dirbuf,fsname);
		strcat(dirbuf,subdir);
	}

	TRACE("rex_start: set up socket for stdin");
	fd0 = socket(AF_INET, SOCK_STREAM, 0);
        TRACE2("rex_start: opened TCP socket (%d) for stdin, doconnect to remote", fd0);
	fd0 = doconnect(&sin, rst->rst_port0,fd0);

	if (rst->rst_port0 == rst->rst_port1) {
	  /*
	   * use the same connection for both stdin and stdout
	   */
	        TRACE("rex_start: stdout will use same socket as stdin");
		fd1 = fd0;
	}

	if (rst->rst_flags & REX_INTERACTIVE) {
	  /*
	   * allocate a pseudo-terminal pair if necessary
	   *   - allocate pty was changed to return the name of the slave
	   *     pty instead of opening it. The slave must be opened in 
	   *     the child in order for things to work with HP job control.
	   *     see comments in getpty() for more details.
	   */
	   TRACE("rex_start: command is interactive, calling AllocatePty");
	   if (AllocatePty(fd0,fd1, sname)) {
	        TRACE("rex_start: can't allocate a pty");
	   	errprintf(error,(catgets(nlmsg_fd,NL_SETN,19, "rexd: cannot allocate a pty\n")));
		return (1);
	   }
	   TRACE2("rex_start: Allocate pty returned sname = %s", sname);
	   HasHelper = 1;

	   /*
	    * Set the default modes for the pty so we start consistent
	    * modes for the pty. The client changes the modes 
	    * by calling REXPROC_MODES 
	    */
	   FixPty(Master);

	  /* The child process must wait until the parent has received and set
	   * the tty modes sent by the client before it execs the requested 
	   * command. Otherwise a timing condition can cause the command to 
	   * hang. We need a pipe in order to tell the child when the parent
	   * has set the modes.
	   */
	  if ( pipe(pipe1) == -1 )
	    {
	      errprintf(error,(catgets(nlmsg_fd,NL_SETN,206, "rexd: can't create pipe\n")));
	      TRACE2("rexd: could not create pipe, errno = %d",errno);
	      return(1);
	   }
           TRACE3("rex_start: pipe call returned p0 = %d, p1 = %d\n\n\trex_start: calling fork\n\n", pipe1[0], pipe1[1]);
	}

	TRACE("rex_start: fork ");
	sighold(SIGCHLD);
	child = fork();
	sigrelse(SIGCHLD);
	if (child < 0) {
	        TRACE("rex_start: can't fork");
		errprintf(error, (catgets(nlmsg_fd,NL_SETN,20, "rexd: can't fork\n")));
		return (1);
	}
	if (child) {
	    /*
	     * parent rexd: close network connections if needed,
	     * then return to the main loop.
	     */
		if ((rst->rst_flags & REX_INTERACTIVE)==0) {
		  TRACE3("rex_start: not interactive, close fd0 = %d fd1 = %d",
			 fd0, fd1);
			close(fd0);
			close(fd1);
		}
	        TRACE("rex_start: parent: retrun to the main loop");
		return (0);
	}
	TRACE("rex_start: child: do the dirty work and exec command");

	/* set a new process group with the child as the group leader */
	TRACE("rex_start: set a new process group with child as leader");
	setpgrp();

	if (rst->rst_flags & REX_INTERACTIVE) {
	  TRACE2("rex_start: open slave pty %s", sname);
	  Slave = open(sname, O_RDWR);
	  TRACE2("rex_start: child opened Slave = %d", Slave);
	  if (Slave < 0) {
	    TRACE2("rex_start: can't open slave for, %s",sname);
	    log_perror(sname);
	    close(Master);
	    exit(1);
	  }
	  int_t = Slave;

	  /* close all connections to controlling terminal */
	  /* so that when we reopen, it is ours exclusivly. */

	  if (vhangup() < 0) {
	     TRACE2("rex_start: vhangup() failed for, %s",sname);
	     log_perror(sname);
	     close(Master);
	     close(int_t);
	     exit(1);
	     }

         /* reopen the slave pseudo-terminal */
	  Slave = open(sname, O_RDWR);
	       if (Slave < 0) {
		TRACE2("rex_start: can't re-open slave for, %s",sname);
	       log_perror(sname);
	       close(Master);
	       close(int_t);
	       exit(1);
	       }
	 close(int_t);
      }

	if (rst->rst_port0 != rst->rst_port1) {
	        TRACE("rex_start: stdin/stdout not the same, create socket");
		fd1 = socket(AF_INET, SOCK_STREAM, 0);
		/* The following shutdowns were removed due to an apparent
		 *  error in the sockets code that was causing and error 
		 * message to be printed on the console. It's removal is a
		 *  workaround and should not effect functionality. *mjk 11/13*
		 */
/*		shutdown(fd0, 1);  */
		fd1 = doconnect(&sin, rst->rst_port1,fd1);
/*		shutdown(fd1, 0); */
	}
	if (rst->rst_port1 == rst->rst_port2) {
	  /*
	   * Use the same connection for both stdout and stderr
	   */
	        TRACE("rex_start: stdout and stderr are the same");
		fd2 = fd1;
	} else {
	        TRACE("rex_start: stdout/stderr not the same, create socket");
		fd2 = socket(AF_INET, SOCK_STREAM, 0);
		fd2 = doconnect(&sin, rst->rst_port2,fd2);

		/* The following shutdown was removed due to an apparent
		 *  error in the sockets code that was causing and error 
		 * message to be printed on the console. It's removal is a
		 *  workaround and should not effect functionality. *mjk 11/13*
		 */
/*		shutdown(fd2, 0); */
	}

	if (rst->rst_flags & REX_INTERACTIVE) {
	  /*
	   * use ptys instead of sockets in interactive mode
	   */
	   TRACE("rex_start: interactive command, calling DoHelper");
	   DoHelper(&fd0, &fd1, &fd2);
	   TRACE("rex_start: returned from DoHelper");
	}

	sprintf(audit_dir,
		catgets(nlmsg_fd,NL_SETN,219, "Initial directory is %s:%s%s"),
		rst->rst_host, rst->rst_fsname, rst->rst_dirwithin);
	audit_daemon(NA_SERV_REXD,NA_RSLT_SUCCESS,
		     check_rhosts ? NA_VALI_RUSEROK : NA_VALI_UID,
		     sin.sin_addr.s_addr, savecred.aup_uid, 
		     myaddr.sin_addr.s_addr, savecred.aup_uid,
		     NA_STAT_START, NA_MASK_NONE, audit_dir);

        TRACE("rex_start: setup environment variables, groups, gid and uid");
	environ = rst->rst_env;
	setgroups(ucred->aup_len,ucred->aup_gids);
	setgid(ucred->aup_gid);
	setuid(ucred->aup_uid);
	sigvector( SIGINT, &vec_dfl, NULL);
	sigvector( SIGHUP, &vec_dfl, NULL);
	sigvector( SIGQUIT, &vec_dfl, NULL);
	
        TRACE("rex_start: chdir to the requested directory");
	if (chdir(dirbuf)) {
              TRACE("rex_start: can't chdir to requested directory");
 	      errprintf(error, 
	          (catgets(nlmsg_fd,NL_SETN,17, "rexd: can't chdir to %s\n")), dirbuf);
		      return (1);
	}

	if (rst->rst_flags & REX_INTERACTIVE) {
	  /* When the read on pipe1 succeeds then we know the parent has 
	   * set the modes and tty size so we can continue.
	   */
	  
	  /* variables used to set the pty size */
	  int lines;    /* number of lines for the pty */
	  int cols;     /* number of columns for the pty */
	  int num;      /* pipe byte for signal from SIGWINCH change */
	  char linestr[16];   /* LINES= + places for upto MAXINT and NULL */
	  char colstr[18];    /* COLUMNS= + places for upto MAXINT and NULL */
#ifndef TIOCSSIZE

	  TRACE("rex_start: reading pipe to get the pty size from the parent");
	  if (read(pipe1[0],&lines,sizeof(lines)) != sizeof(lines) )
	    {
	      log_perror((catgets(nlmsg_fd,NL_SETN,207, "rexd: could not read from pipe")));
	      fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,208, "rexd: could not read from pipe\n")));
	      TRACE("rex_start: could not read lines from pipe");
	      exit(1);
	    }
	  TRACE2("rex_start: lines = %d",lines);
	  if (read(pipe1[0],&cols,sizeof(cols)) != sizeof(cols) )
	    {
	      log_perror((catgets(nlmsg_fd,NL_SETN,209, "rexd: could not read from pipe")));
	      fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,210, "rexd: could not read from pipe\n")));
	      TRACE("rex_start: could not read cols from pipe");
	      exit(1);
	    }
	  TRACE2("rex_start: cols = %d",cols);
	  /* Set the values for environment variables LINES and COLUMNS */
	  sprintf(linestr,"LINES=%d",lines);
	  sprintf(colstr,"COLUMNS=%d",cols);
	  TRACE2("rex_start: setting LINES to %s",linestr);
	  putenv(linestr);
	  TRACE2("rex_start: setting COLUMNS to %s",colstr);
	  putenv(colstr);

	  TRACE("rex_start: received pty size from parent");
#else
       if (read(pipe1[0],&num,1) != 1)
               {
                 log_perror((catgets(nlmsg_fd,NL_SETN,209, "rexd: could not read from TIOC pipe")));
                 fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,210, "rexd: could not read from TIOC pipe\n")));
                 TRACE("rex_start: could not read from TIOC pipe");
                 exit(1);
               }
#endif
	}

	TRACE("rex_start: dup fd0,fd1 and fd2 to stdin, stdout and stderr");
	dup2(fd0, 0);
	dup2(fd1, 1);
	dup2(fd2, 2);

	TRACE("rex_start: close all file descriptors > 2");
#ifdef hpux
        max_fds = getnumfds();
#ifdef TRACEON
	tfd = fileno(trace_fd);
#endif /* TRACEON */
#else hpux
	max_fd = getdtablesize();
#endif hpux
	for (fd0 = 3; fd0 < max_fds; fd0++) {

#ifdef TRACEON
		/* we don't want to close the trace file */
		if (fd0 == tfd)
			continue;
#endif /* TRACEON */
		close(fd0);
	}

	if (rst->rst_cmd[0]==NULL) {
	  /*
	   * Null command means execute the default shell for this user
	   */
	    char *args[2];

            TRACE2("rex_start: starting shell, %s",defaultShell);
	    args[0] = defaultShell;
	    args[1] = NULL;
	    estat = execvp(defaultShell, args);

	    /* should only get here if execvp call failed */
	    TRACE("rex_start: can't exec shell");
	    /* generate an error message based on the value of errno */
	    execerr(defaultShell, errno);

	    exit(1);
	}

	TRACE2("rex_start: starting command %s", rst->rst_cmd[0]);
	estat = execvp(rst->rst_cmd[0], rst->rst_cmd);

	/* should only get here if exec call failed */
	TRACE2("rex_start: can't exec %s", rst->rst_cmd[0]);
        /* generate an error message based on the value of errno */
	execerr(rst->rst_cmd[0], errno);

	exit(1);
}

AlreadyMounted(fsname)
    char *fsname;
{
  /*
   * Search the mount table to see if the given file system is already
   * mounted.  If so, return the place that it is mounted on.
   */
   FILE *table;
   register struct mntent *mt;

   TRACE("AlreadyMounted: SOP");
#ifdef hpux
   /* 
    * changed from "r" to "r+" so we can lock the 
    * table and be sure to chdir into the file system
    * before we release the lock on the mount table
    */
   table = setmntent(MNT_MNTTAB,"r+");
#else not hpux
   table = setmntent(MOUNTED,"r");
#endif hpux
   while ( (mt = getmntent(table)) != NULL) {
   	if (strcmp(mt->mnt_fsname,fsname) == 0) {
	    TRACE3("AlreadyMounted: %s mounted on %s",fsname, mt->mnt_dir);
	    strcpy(fsname,mt->mnt_dir);
	    /* 
	     * this will hold the cd into the file system
	     * to hold it (not allow it to be unmounted
	     */
	    chdir(fsname);
	    endmntent(table);
	    return(1);
	}
   }
   endmntent(table);
   TRACE2("AlreadyMounted: %s not mounted", fsname);
   return(0);
}


/*
 * connect to the indicated IP address/port, and return the 
 * resulting file descriptor.  
 */
doconnect(sin, port, fd)
	struct sockaddr_in *sin;
	short port;
	int fd;
{
        TRACE("doconnect: SOP");
	sin->sin_port = ntohs(port);
	if (connect(fd, sin, sizeof *sin)) {
	        TRACE("doconnect: connect call failed");
		log_perror((catgets(nlmsg_fd,NL_SETN,24, "rexd: connect")));
		exit(1);
	}
	TRACE2("doconnect: connect call returned file des. %d", fd);
	return (fd);
}

/*
*  SETPROCTITLE -- set the title of this process for "ps"
*
*       Does nothing if there were not enough arguments on the command
*       line for the information.
*
*	Side Effects:
*		Clobbers argv[] of our main procedure.
*/

setproctitle(user, host)
	char *user, host[];
{
	char	buf[BUFSIZ];
	char	hostname[BUFSIZ];
	int	i;
	
	TRACE("setproctitle: SOP")

	/*
	 * This has been changed as of 8.0.  Previously, we were doing a 
	 * lot of munging of Argv in order to get the user@host notation
	 * for pstat_set_command.  Now, with the pstat() call, we can
	 * pass a buffer (built in the following lines).  ps(1) will now
	 * display:
	 *		rpc.rexd <options> [user@host]
	 *
	 * dlr 08/17/90
	 */

	memset(buf,0,BUFSIZ);
	for (i=0; i < Argc; i++) {
		strcat(buf,Argv[i]);
		buf[strlen(buf)] = ' ';
	}

	/* 
	 * If the host in in our domain, don't bother printing the 
	 * fully qualified domainname.
         */  

	if ((gethostname(hostname,BUFSIZ) == 0) && (strchr(host,'.') != NULL))
		if (strcmp(strchr(host,'.'),strchr(hostname,'.')) == 0)
			host[(int)strchr(host,'.') - (int)host] = NULL;

	buf[strlen(buf)] = '[';
	strcat(buf,user);
	buf[strlen(buf)] = '@';
	strcat(buf,host);
	buf[strlen(buf)] = ']';

	TRACE("setproctitle: before calling pstat");
        pstat(PSTAT_SETCMD, buf, strlen(buf), 0, 0);

	TRACE("setproctitle: EOP");
}

/*
**	rex_argparse()	--	parse command line options for rexd.
**                              This is similar to argparse in check_exit.c
**                              used by all other rpc daemons. However, rexd
**                              has a different set of options and must 
**                              use a different version of the parser.
*/     

/*
**	getopt options
*/
extern	char	*optarg;
extern  int     opterr;

void
rex_argparse(argc, argv)
int	argc;
char	**argv;
{
    int c;
    char *invo_name = *argv;

    /*
    **	parse all arguments passed in to the function and set the
    **	Exitopt and LogFile variables as required
    */
    opterr = 0; /* keeps getopt from writing error messages to stderr 
                 * (the console). we log messages as appropriate */
    while ((c=getopt(argc, argv, "rl:m:")) != EOF) {
	TRACE2("rex_argparse: getopt returned option %c", c);
	switch(c) {
	    case 'r':
		    /*
		    **	Do extra authentication by checking looking at 
		    **  host.eqiuv(4) and .rhost for the uid to see if 
		    **  the request came from a host listed in one of 
		    **  these files.
		    */
	            check_rhosts = TRUE;
		    TRACE2("rex_argparse: option %c, set check_rhosts", c);
		    break;
	    case 'l':
		    /*
		    **	log file name ...
		    */
		    strcpy(LogFile, optarg);
		    TRACE3("rex_argparse: option %c, set Logfile = %s", c, optarg);
		    break;
	    case 'm':
		    /* 
		     * directory of mount points for remote directories
		     */
		    /* make sure that a mountdir was specified. if not 
		     * we use the default mountdir.
		     */
		    if ( strlen(optarg) == 0) 
		      {
			break;
		      }
		    /* make sure that optarg has room to append /rexdXXXXXX */
		    if (strlen(optarg) > (MAXPATHLEN - 11) )
			{
			  TRACE2("rex_argparse: option %c, argument to long",c);
			  /* get out of here */
			  break;
			}
		    strcpy(mntdir, optarg);
		    strcpy(mntpnt, mntdir);
		    /* append the template for naming a subdirectory */
		    strcat(mntpnt,"/rexdXXXXXX");
		    TRACE3("rex_argparse: option %c, set mntpnt = %s",c,mntpnt);
		    break;
	    default:
		    TRACE2("rex_argparse: option %c is illegal", c);
		    /* do nothing. we don't want to write to the console
		     * and logging is not started yet
		     */
		    break;
	}
      }
}


void
execerr(command, err)
     char *command;   /* command that failed to execute */
     int   err;       /* value of errno after failure   */

{
  /* This routine prints out a message for a failed execvp call. The 
   * messages are intended to be similar for to thoughs provided by 
   * a shell. Not all cases are covered. The original Sun code just 
   * returned on an execvp failure with no indication of an error 
   * *mjk*
   */
  TRACE("execerr: SOP");
  if (err == ENOENT || err == ENOTDIR )
    {
      fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,211, "rexd: %s: Command not found\n")),command);
      logmsg((catgets(nlmsg_fd,NL_SETN,212, "rexd: %s: Command not found")),command);
    }
  else if (err == EACCES) 
    {
      fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,213, "rexd: %s: Permission denied\n")), command);
      logmsg((catgets(nlmsg_fd,NL_SETN,214, "rexd: %s: Permission denied")), command);
    }
  else if (err == ETXTBSY)
    {
      fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,215, "rexd: %s: Text file busy\n")), command);
      logmsg((catgets(nlmsg_fd,NL_SETN,216, "rexd: %s: Text file busy")), command);
    }
  else 
    {  
      /* one of many possible but less likely conditions */
      fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,217, "rexd: %s: Can't execute\n")), command);
      logmsg((catgets(nlmsg_fd,NL_SETN,218, "rexd: %s: Can't execute")), command);
    }
  TRACE("execerr: EOP");

}
