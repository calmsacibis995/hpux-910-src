#ifndef lint
static char rcsid[] = "@(#)rpc.mountd:	$Revision: 1.77.109.19 $	$Date: 94/07/11 15:41:36 $  ";
#endif

/*#ifndef lint
/*static char sccsid[] = 	"@(#)rpc.mountd.c	1.4 90/07/23 4.1NFSSRC Copyr 1990 Sun Micro";
/*#endif
*/

#ifdef PATCH_STRING
static char *patch_2953="@(#) PATCH_9.X: rpc.mountd.o $Revision: 1.77.109.19 $ 94/06/16 PHNE_2953 PHNE_4402"
;
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc. 
 */
#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <exportent.h>
/*#include "exportent.h"*/
#include <string.h>
#include <sys/unistd.h>
#include <sys/signal.h>

/* in.h, nameser.h, and resolv.h are used for domain name info */
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#define MAXRMTABLINELEN		(MAXPATHLEN + MAXHOSTNAMELEN + 2)

/* HPNFS
**	Declare Globals for check_exit functionality.
*/
char	LogFile[64];		/* log file: use it instead of console	*/
long	Exitopt=3600l;		/* # of seconds to wait between checks	*/
long	Logopt=1;		/* Level of tracing - 1 is normal logging */
                                /* if Logopt is 3, then XPRINTF msgs appear */
long	Debug   = 0;		/* run in foreground ? */
int	My_prog=MOUNTPROG,		/* the program number	*/
	My_vers=MOUNTVERS,		/* the version number	*/
	My_prot=IPPROTO_UDP,		/* the protocol number	*/
	My_port;		/* the port number, filled in later	*/

extern alarmer();  /* HPNFS this is in check_exit.c */
extern int errno;

char XTAB[]  = "/etc/xtab";
char RMTAB[] = "/etc/rmtab";

int mnt();
char *exmalloc();
struct groups **newgroup();
struct exports **newexport();
int mount();
int check_exports();
int xent_free();
int log_cant_reply();

/*
 * mountd's version of a "struct mountlist". It is the same except
 * for the added ml_pos field.
 */
struct mountdlist {
/* same as XDR mountlist */
	char *ml_name;
	char *ml_path;
	struct mountdlist *ml_nxt;
/* private to mountd */
	long ml_pos;		/* position of mount entry in RMTAB */
};

struct mountdlist *mountlist;

struct xent_list {		/* cached export list */
	struct xent_list *x_next;
	struct exportent  x_xent;
} *xent_list;

/* HPNFS Sun's default is 1, HP's default is 0 */
int nfs_portmon = 0;
char *domain;
struct sigvec oldsigvec;

int rmtab_load();
int rmtab_delete();
long rmtab_insert();


/* HPNFS Internal tracing macro definitiions */
/* HPNFS define ILOG to call logmsg only if logopt is set */
#define ILOG(s)           if (Logopt >= 2) logmsg(s)
#define ILOG1(s,p1)       if (Logopt >= 2) logmsg(s,p1)
#define ILOG2(s,p1,p2)    if (Logopt >= 2) logmsg(s,p1,p2)
#define ILOG3(s,p1,p2,p3) if (Logopt >= 2) logmsg(s,p1,p2,p3)

/* HPNFS XPRINTF uses printf, useful for tracing before logging is on */
/*       Logopt must be initted to 3 for this to work              */
#define XPRINTF(s)           if (Logopt > 2) printf(s)
#define XPRINTF1(s,p1)       if (Logopt > 2) printf(s,p1)
#define XPRINTF2(s,p1,p2)    if (Logopt > 2) printf(s,p1,p2)
#define XPRINTF3(s,p1,p2,p3) if (Logopt > 2) printf(s,p1,p2,p3)

#ifdef DEBUG
exit(error)
int error;

{
  _exit (error);
}
#endif

#ifdef	BFA
/*	HPNFS	jad	87.07.02
**	handle	--	added to get BFA coverage after killing server;
**		the explicit exit() will be modified by BFA to write the
**		BFA data and close the BFA database file.
*/
handle(sig)
int sig;
{
	XPRINTF1 ("In BFA signal handler with signal %d. Exiting.\n", sig);
	exit(sig);
}

write_BFAdbase()
{
       pfa_dump();
}
#endif	BFA

/* HPNFS - only for debugging */
#ifdef DEBUG
abort_handle(sig)
int sig;
{
	XPRINTF1 ("rpc.mountd received signal (%d). ABORTING!\n", sig);
	abort(sig);
}
#endif

/* HPNFS Signal handler for SIGSYS.  SIGSYS is sent when a system call*/
/* HPNFS is not configured into the kernel.			      */
void
not_installed(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
	fprintf(stderr, "mountd: NFS system call is not available.\n        Please configure NFS into your kernel.\n");
	exit(1);
}

main(argc, argv)
	int argc;
	char **argv;
{
	SVCXPRT *transp;
	int pid;
	register int i;
	int sock;
	int proto;
	struct  sigvec  newsigvec;


	/* make sure the user is root first */
	if (getuid()) { /* This command allowed only to root */
		fprintf(stderr, "Must be root to start rpc.mountd.\n");
		exit(1) ;
	}

#ifdef	BFA
	/*	HPNFS	jad	87.07.02
	**	catch all fatal signals and exit explicitly, so the BFA
	**	numbers get updated.  Otherwise we get no BFA coverage!
	*/
	(void) signal(SIGHUP, handle);
	(void) signal(SIGINT, handle);
	(void) signal(SIGQUIT, handle);
	(void) signal(SIGTERM, handle);
	XPRINTF ("main signals set up for HUP, INT, QUIT, TERM.\n");

        /* added to get BFA data even as the daemon is running */
        (void) signal(SIGUSR2, write_BFAdbase);
#endif	BFA


#ifdef DEBUG 
	(void) signal(SIGHUP, abort_handle);
	(void) signal(SIGINT, abort_handle);
	(void) signal(SIGQUIT, abort_handle);
	(void) signal(SIGTERM, abort_handle);
	XPRINTF ("main abort signals set up for HUP, INT, QUIT, TERM.\n");
#endif

/* HPNFS On the s300 if getfh is not configured into the kernel  */
/* HPNFS we do not return a SIGSYS, we return a ENOPROTOOPT.  On */
/* HPNFS the s800 we get a SIGSYS			         */

	newsigvec.sv_handler = not_installed;
	newsigvec.sv_mask = 0;
	newsigvec.sv_flags = 0;

	sigvector(SIGSYS, &newsigvec, &oldsigvec);

        /* init logfile to nothing */
	LogFile[0] = '\0';

        /* parse arguments */
	if (Logopt > 2) {
	   printf ("about to call mountd_argparse: argv: ");
	   for (i=0;i<=argc;i++) printf ("%s ",argv[i]);
	   printf ("\n");
        }

	mountd_argparse (argc, argv);

        /* start logging, if logfile is not null */
        (void) startlog(*argv,LogFile);
	if (Logopt > 1)
           logmsg ("rpc.mountd started.");

	if (issock(0)) {
		/*
		 * Started from inetd 
		 */
		ILOG ("rpc.mountd was started from inetd.");
		sock = 0;
		proto = IPPROTO_UDP;	/* HPNFS - RE-register with portmapper */
	} else {
		/*
		 * Started from shell, background.
		 */
                /* HPNFS check for debug mode first */
	        if (!Debug) {
   		   ILOG ("rpc.mountd was started from a shell, running in backround.\n");
#ifdef BFA
		   write_BFAdbase();
#endif
  		   pid = fork();
		   if (pid < 0) {
			perror("mountd: can't fork");
			exit(2);
		   }
		   if (pid) {
			exit(0);
		   }

		  /*
		   * Close existing file descriptors, open "/dev/null" as
		   * standard input, output, and error, and detach from
		   * controlling terminal.
		   */

#ifndef hpux
		  /* HPNFS 
                   * I don't think we need this.
                   * setpgrp should take care of it.
                   */
                  i = getdtablesize();
		  while (--i >= 0)
		  	  (void) close(i);
		  (void) open("/dev/null", O_RDONLY);
		  (void) open("/dev/null", O_WRONLY);
		  (void) dup(1);
		  i = open("/dev/tty", O_RDWR);
		  if (i >= 0) {
			  (void) ioctl(i, TIOCNOTTY, (char *)0);
			  (void) close(i);
		  }
#else
		 /* HPNFS
		  * set a new process group with the child
		  * as the group leader.
		  */
		 setpgrp();
#endif
	       } /* if not debug */
	       else {
   		 ILOG ("rpc.mountd was started from a shell, running in Foreground.\n");
	       }
#ifndef hpux
	       /* HPNFS do this later on */
	       (void) pmap_unset(MOUNTPROG, MOUNTVERS);

	       /* HPNFS - we don't support this, yet */
	       (void) pmap_unset(MOUNTPROG, MOUNTVERS_POSIX);
#endif
	       sock = RPC_ANYSOCK;
	       proto = IPPROTO_UDP;
	}

	/* HPNFS - always unset - both UDP and TCP     */
        /* will be re-registered. This fixes a problem */
	/* where tcp can't register after mountd is    */
        /* is restarted from inetd.                    */
	(void) pmap_unset(MOUNTPROG, MOUNTVERS);

	/*
	 * Create UDP service
	 */
	ILOG ("Creating UDP service.");
	if ((transp = svcudp_create(sock)) == NULL) {
		logmsg ("couldn't create UDP transport");
		exit(3);
	}
	ILOG ("Registering UDP service.");
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, proto)) {
		logmsg ("couldn't register UDP MOUNTPROG_ORIG");
		exit(4);
	}

	/* HPNFS save off udp portnum for alarmer routine */
	My_port = transp->xp_port;
	ILOG1 ("registered portnum %d for UDP with portmapper",My_port);

#ifndef hpux /* HPNFS we don't support posix pathconf  */
	if (!svc_register(transp, MOUNTPROG, MOUNTVERS_POSIX, mnt, proto)) {
		logmsg ("couldn't register UDP MOUNTPROG");
		exit(5);
	}
#endif /* posix */
	/* if started with inetd -e, don't register the tcp port  HPNFS */
	/* Sun has dropped the -e option so it is necessary to    HPNFS */
	/* make the tcp port registration dependant on whether    HPNFS */
	/* or not mountd is started with inetd -e.                HPNFS */
	if (!(issock(0) && (Exitopt == 0))) {
	    /*
	     * Create TCP service
	     */
	    ILOG ("Creating TCP service.");
	    if ((transp = svctcp_create(RPC_ANYSOCK, 0, 0)) == NULL) {
		logmsg ("couldn't create TCP transport");
		exit(6);
	    }
	    ILOG ("Registering TCP service.");
	    if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mnt, 
			  IPPROTO_TCP)) {
		logmsg ("couldn't register TCP MOUNTPROG");
		exit(7);
	    }
	    ILOG1 ("registered portnum %d for TCP with portmapper",transp->xp_port);
#ifndef hpux /* no posix support */
	    if (!svc_register(transp, MOUNTPROG, MOUNTVERS_POSIX, mnt,
			      IPPROTO_TCP)) {
		logmsg ("couldn't register TCP MOUNTPROG");
		exit(8);
	    }
#endif /* no posix */
        }

	/*
	 * Initalize the world 
	 */
	(void) yp_get_default_domain(&domain);
	ILOG1 ("Called yp_get_default_domain, domain is %s",domain); 

	/*
	 * Start serving 
	 */
	ILOG ("Calling rmtab_load");
	rmtab_load();
	ILOG ("Calling svc_run");
	svc_run();
	logmsg ("Error: svc_run shouldn't have returned");
	abort();
	/* NOTREACHED */
}


/*
 * Determine if a descriptor belongs to a socket or not 
 */
issock(fd)
	int fd;
{
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return (0);
	}
	/*
	 * SunOS returns S_IFIFO for sockets, while 4.3 returns 0 and does not
	 * even have an S_IFIFO mode.  Since there is confusion about what the
	 * mode is, we check for what it is not instead of what it is. 
	 */
	switch (st.st_mode & S_IFMT) {
	case S_IFCHR:
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFBLK:
		return (0);
	default:
		return (1);
	}
}

/*
 * Server procedure switch routine 
 */
mnt(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	
	ILOG ("mnt: SOP");
	switch (rqstp->rq_proc) {
	case NULLPROC:
		ILOG ("mnt: NULLPROC called");
		errno = 0;
		if (!svc_sendreply(transp, xdr_void, (char *)0))
			log_cant_reply(transp);
		break;
	case MOUNTPROC_MNT:
		ILOG ("mnt: MOUNTPROC_MNT called");
		mount(rqstp);
		break;
	case MOUNTPROC_DUMP:
		ILOG ("mnt: MOUNTPROC_DUMP called");
		errno = 0;
		if (!svc_sendreply(transp, xdr_mountlist, (char *)&mountlist))
			log_cant_reply(transp);
		break;
	case MOUNTPROC_UMNT:
		ILOG ("mnt: MOUNTPROC_UMNT called");
		umount(rqstp);
		break;
	case MOUNTPROC_UMNTALL:
		ILOG ("mnt: MOUNTPROC_UMNTALL called");
		umountall(rqstp);
		break;
	case MOUNTPROC_EXPORT:
	case MOUNTPROC_EXPORTALL:
		ILOG ("mnt: MOUNTPROC_EXPORT called");
		export(rqstp);
		break;
	default:
		ILOG ("mnt: NO PROC hit");
		svcerr_noproc(transp);
		break;
	}
	/*
        **      see if I should exit now, or hang around until remapped
        */
        check_exit();
        ILOG("mnt check_exit returned");
}

struct hostent *
getclientsname(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct sockaddr_in actual;
	struct hostent *hp;
	static struct in_addr prev;
	static char *null_alias;
	char *machine = (char *) 0; /* HPNFS */
	struct in_addr rq_claim; /* HPNFS */
	static struct hostent h;

	actual = *svc_getcaller(transp);
	if (nfs_portmon) {
		if (ntohs(actual.sin_port) >= IPPORT_RESERVED) {
			return (NULL);
		}
	}
	/*
	 * Don't use the unix credentials to get the machine name,
	 * instead use the source IP address. 
	 * Used cached hostent if previous call was for the
	 * same client.
	 */

	if (bcmp(&actual.sin_addr, &prev, sizeof(struct in_addr)) == 0) {
		return (&h);
	}

	/* HPNFS reset cached info */
	prev = actual.sin_addr;

	hp = gethostbyaddr((char *) &actual.sin_addr, sizeof(actual.sin_addr), 
			   AF_INET);
	if (hp == NULL) {			/* dummy one up */
		h.h_name = inet_ntoa(actual.sin_addr);
		h.h_aliases = &null_alias;
		h.h_addrtype = AF_INET;
		h.h_length = sizeof (u_long);
		hp = &h;
		/* HPNFS - if this lookup doesn't work, we can't */
                /* expect another others to either               */
		return (hp); /* HPNFS */
	} else {
		bcopy(hp, &h, sizeof(struct hostent));
	}

	/*return (hp); - deleted - HPNFS */


	/* HPNFS if the remote system is using an alias for it's */
	/* hostname and we are using DNS then check the ip addr  */
        /* and remember this name for future compares            */

	/* HPNFS get machine name from rpc request info */
	machine = ((struct authunix_parms *)
		rqstp->rq_clntcred)->aup_machname;


	/* is machine already in list */
	if ( (host_compare (machine, hp)) != 0) {
           /* get the ip addr of machine */
	   hp = gethostbyname(machine);

           /* save off this info  */
	   bcopy(hp, &h, sizeof(struct hostent));

	   /* if we didn't get anything, stick with what we've got */
	   if (hp == NULL) {
              hp = gethostbyaddr((char *) &actual.sin_addr, 
		   sizeof(actual.sin_addr), AF_INET);

	      /* save off this info  */
	      bcopy(hp, &h, sizeof(struct hostent));
              return (hp);
	   }
           
	   /* compare the list of ip addrs from gethostbyname   */
	   /* with the addr that the request came in on         */
	   /*  if they aren't the same then ignore this name    */
           /*  if they match then keep the results of the       */
           /*  last call to gethostbyname which has the real    */
           /*  name and this alias in it                        */
next_ipaddr:
	   bcopy(hp->h_addr, &rq_claim, sizeof(struct in_addr));

	   /* compare actual.sin_addr with hp */
	   if (actual.sin_addr.s_addr != rq_claim.s_addr) {
              if (hp && hp->h_addr_list[1]) { /* BIND changes */
		 hp->h_addr_list++;
		 goto next_ipaddr;
	       }
	      /* This should be a really deviant case.            */
              /* The hostname, on the remote, is set to a name    */
              /* which is not an alias known by our host          */
              /* resolution system.                               */
	      /* Too Bad. He is going to take the hit of another  */ 
	      /* gethostbyaddr call                               */

	      /* hostent has been munged by the above gethostbyname */
	      /* call gethostbyaddr for now, it would be nice to    */
              /* be able to really copy the hostent info            */
	      hp = gethostbyaddr((char *) &actual.sin_addr, 
				 sizeof(actual.sin_addr), 
				 AF_INET);
	     
              /* save off this info  */
	      bcopy(hp, &h, sizeof(struct hostent));

	      return (hp);
           }
        }

	return (hp);
}

log_cant_reply(transp)
	SVCXPRT *transp;
{
	int saverrno;
	struct sockaddr_in actual;
	register struct hostent *hp;
	register char *name;

	saverrno = errno;	/* save error code */
	actual = *svc_getcaller(transp);
	/*
	 * Don't use the unix credentials to get the machine name, instead use
	 * the source IP address. 
	 */
	if ((hp = gethostbyaddr(&actual.sin_addr, sizeof(actual.sin_addr),
	   AF_INET)) != NULL)
		name = hp->h_name;
	else
		name = inet_ntoa(actual.sin_addr);

	errno = saverrno;
	if (errno == 0)
		logmsg ("couldn't send reply to %s", name);
	else
		logmsg ("couldn't send reply to %s: %s", name,strerror(errno));
}


/*
 * Check mount requests, add to mounted list if ok 
 */
mount(rqstp)
	struct svc_req *rqstp;
{
	SVCXPRT *transp;
	fhandle_t fh;
	struct fhstatus fhs;
	char *path, rpath[MAXPATHLEN];
	struct mountdlist *ml;
	char *gr, *grl;
	struct exportent *xent;
	struct exportent *findentry();
	struct stat st;
	char **aliases;
	struct hostent *client;
        int saveerror = 0;

	transp = rqstp->rq_xprt;
	path = NULL;
	fhs.fhs_status = 0;
	client = getclientsname(rqstp, transp);
	if (client == NULL) {
		fhs.fhs_status = EACCES;
		goto done;	
	}
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}

	if (Logopt > 1)
           logmsg ("mount: mount request from %s, mounting %s.\n",
                    client->h_name, path);

	if (lstat(path, &st) < 0) {
		fhs.fhs_status = EACCES;
		goto done;	
	}

	/*
	 * Get a path without symbolic links.
	 */
	if (realpath(path, rpath) == NULL) {
		logmsg	("mount request: realpath failed on %s: %s",
			 path, strerror(errno));
		fhs.fhs_status = EACCES;
		goto done;
	}

	/* HPNFS just call getfh, instead of do_getfh */
	if (getfh(rpath, &fh) < 0) {
		saveerror=errno;
		fhs.fhs_status = errno == EINVAL ? EACCES : errno;
		if (saveerror = EINVAL)
                   logmsg ("mount request: getfh failed on %s: %s.\n     This could be caused by %s not being exported.\n",
		    path, strerror(saveerror), path );
		else
                   logmsg ("mount request: getfh failed on %s: %s.\n",
		    path, strerror(saveerror), path );
		goto done;
	}

	xent = findentry(rpath);
	if (xent == NULL) {
		fhs.fhs_status = EACCES;
		goto done;
	}

	/* Check access list - hostnames first */

	grl = getexportopt(xent, ACCESS_OPT);
	if (grl == NULL)
		goto done;

	while ((gr = strtok(grl, ":")) != NULL) {
		grl = NULL;
		if (dn_cmp(gr, client->h_name) == 0)
			goto done;
		for (aliases = client->h_aliases; *aliases != NULL;
		     aliases++) {
			if (dn_cmp(gr, *aliases) == 0)
				goto done;
		}
	}
	
	/* no hostname match - try netgroups */

	grl = getexportopt(xent, ACCESS_OPT);
	if (grl == NULL)
		goto done;

	while ((gr = strtok(grl, ":")) != NULL) {
		grl = NULL;
		if (in_netgroup(gr, client->h_name, domain))
			goto done;
		for (aliases = client->h_aliases; *aliases != NULL;
		     aliases++) {
			if (in_netgroup(gr, *aliases, domain))
				goto done;
		}
	}

	/* Check root and rw lists */

	grl = getexportopt(xent, ROOT_OPT);
	if (grl != NULL) {
		while ((gr = strtok(grl, ":")) != NULL) {
			grl = NULL;
			if (dn_cmp(gr, client->h_name) == 0)
				goto done;
		        for (aliases = client->h_aliases; 
                             *aliases != NULL; aliases++) {
			       if (dn_cmp(gr, *aliases) == 0)
				   goto done;
		        }
		}
	}
	grl = getexportopt(xent, RW_OPT);
	if (grl != NULL) {
		while ((gr = strtok(grl, ":")) != NULL) {
			grl = NULL;
			if (dn_cmp(gr, client->h_name) == 0)
				goto done;
		        for (aliases = client->h_aliases; 
                             *aliases != NULL; aliases++) {
			       if (dn_cmp(gr, *aliases) == 0)
				   goto done;
		        }
		}
	}
	fhs.fhs_status = EACCES;

done:
	if (fhs.fhs_status == 0)
		fhs.fhs_fh = fh;
	errno = 0;
	if (!svc_sendreply(transp, xdr_fhstatus, (char *)&fhs))
		log_cant_reply(transp);
	if (path != NULL)
		svc_freeargs(transp, xdr_path, &path);
	if (fhs.fhs_status) {
           if (Logopt >= 1) 
	      logmsg ("mount: mount request from %s denied: %s\n", 
                      client->h_name, strerror(fhs.fhs_status) );
		return;
	}

	/*
	 *  Add an entry for this mount to the mount list.
	 *  First check whether it's there already - the client
	 *  may have crashed and be rebooting.
	 */
	for (ml = mountlist; ml != NULL ; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_path, rpath) == 0) {
			if (strcmp(ml->ml_name, client->h_name) == 0) {
				return;
			}
			for (aliases = client->h_aliases; *aliases != NULL;
			     aliases++) {
				if (strcmp(ml->ml_name, *aliases) == 0) {
					return;
				}
			}
		}
	}

	/*
	 * Add this mount to the mount list.
	 */
	ml = (struct mountdlist *) exmalloc(sizeof(struct mountdlist));
	ml->ml_path = (char *) exmalloc(strlen(rpath) + 1);
	(void) strcpy(ml->ml_path, rpath);
	ml->ml_name = (char *) exmalloc(strlen(client->h_name) + 1);
	(void) strcpy(ml->ml_name, client->h_name);
	ml->ml_nxt = mountlist;
	ml->ml_pos = rmtab_insert(client->h_name, rpath);
	mountlist = ml;

	if (Logopt > 1) 
           logmsg ("mount: mount request from %s granted.\n",
                    client->h_name);
	return;
}

struct exportent *
findentry(path)
	char *path;
{
	struct exportent *xent;
	struct xent_list *xp;
	register char *p1, *p2;

	check_exports();

	for (xp = xent_list ; xp ; xp = xp->x_next) {
		xent = &xp->x_xent;
		for (p1 = xent->xent_dirname, p2 = path ; *p1 == *p2 ; p1++, p2++)
			if (*p1 == '\0')
				return xent;	/* exact match */

		if ((*p1 == '\0' && *p2 == '/' ) ||
		    (*p1 == '\0' && *(p1-1) == '/') ||
		    (*p2 == '\0' && *p1 == '/' && *(p1+1) == '\0')) {
			if (issubdir(path, xent->xent_dirname))
				return xent;
		}
	}
	return ((struct exportent *) NULL);
}

#define MAXGRPLIST 256
/*
 * Use cached netgroup info if the previous call was
 * from the same client.  Two lists are maintained
 * here: groups the client is a member of, and groups
 * the client is not a member of.
 */
int
in_netgroup(group, in_hostname, domain)
	char *group, *in_hostname, *domain;
{
	static char prev_hostname[MAXHOSTNAMELEN+1];
	static char grplist[MAXGRPLIST+1], nogrplist[MAXGRPLIST+1];
	char hostname[MAXHOSTNAMELEN+1];
	char key[256];
	char *ypline = NULL;
	int yplen;
	register char *gr, *p;
	static time_t last;
	time_t time();
	time_t time_now;
	static int cache_time = 30; /* sec */
	
	/* HPNFS
	 * Copy over the given hostname to a temp variable so we can truncate
	 * from a fully qualified name to just the hostname if necessary.
	 */
	strcpy (hostname, in_hostname);

	time_now = time((long *)0);
	if (time_now > last + cache_time ||
	    strcmp(hostname, prev_hostname) != 0) {
		last = time_now;
		(void) strcpy(key, hostname);
		(void) strcat(key, ".");
		(void) strcat(key, domain);
		bzero(grplist, sizeof(grplist)); /* HPNFS patch from sun */
		if (yp_match(domain, "netgroup.byhost", key,
		    strlen(key), &ypline, &yplen) == 0) {
			(void) strncpy(grplist, ypline, MIN(yplen, MAXGRPLIST));
			free(ypline);
		} else {
			grplist[0] = '\0';
		}
		nogrplist[0] = '\0';
		(void) strcpy(prev_hostname, hostname);
	}

	for (gr = grplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
                if (strlen(group) == p - gr && strncmp(group, gr, p - gr) == 0)
			return 1;
		if (*p == ',')
			p++;
	}
	for (gr = nogrplist ; *gr ; gr = p ) {
		for (p = gr ; *p && *p != ',' ; p++)
			;
                if (strlen(group) == p - gr && strncmp(group, gr, p - gr) == 0)
			return 0;
		if (*p == ',')
			p++;
	}

retry:
	if (innetgr(group, hostname, (char *)NULL, domain)) {
		if (strlen(grplist)+1+strlen(group) > MAXGRPLIST)
			return 1;
		if (*grplist)
			(void) strcat(grplist, ",");
		(void) strcat(grplist, group);
		return 1;
	} else {
		/* See if we can match without a fully qualified name.
		 * Truncate to just the hostname and retry if we really
		 * did truncate.
		 */
		if ((p = strchr(hostname, '.')) != NULL) {
           			*p = '\0';
				goto retry;
        	}

		/* HPNFS not in netgroup - add to nogrplist */
		if (strlen(nogrplist)+1+strlen(group) > MAXGRPLIST)
			return 0; /* HPNFS used to be 1 which is wrong*/
		if (*nogrplist)
			(void) strcat(nogrplist, ",");
		(void) strcat(nogrplist, group);
		return 0;
	}
}

check_exports()
{
	FILE *f;
	struct stat st;
	static long last_xtab_time;
	struct exportent *xent;
	struct xent_list *xp, *xp_prev;
	char rpath[MAXPATHLEN];


	ILOG ("check_exports: SOP");

	/*
	 *  read in /etc/xtab if it has changed 
	 */

	ILOG ("check_exports: check date of /etc/xtab");
	if (stat(XTAB, &st) != 0) {
		logmsg ("Cannot stat %s: %s", XTAB, strerror(errno));
		return;
	}
	if (st.st_mtime == last_xtab_time)
		return;				/* no change */

	ILOG ("check_exports: /etc/xtab changed free list and read new file");
	xent_free(xent_list);			/* free old list */
	xent_list = NULL;
	
	f = setexportent();
	if (f == NULL) {
    	   /* HPNFS */
           /* exportfs might have been running, */
	   /* try again in a few seconds        */
	   sleep(3);


	   /* HPNFS they only get one more try */
	   f = setexportent();
	   if (f == NULL)
              return;
	}

	while (xent = getexportent(f)) {
		/*
		 * Get a path without symbolic links.
		 */
		if (realpath(xent->xent_dirname, rpath) == NULL) {
			logmsg ("check_exports: realpath failed on %s: %s",
				xent->xent_dirname, strerror(errno));
			continue;
		}

		xp = (struct xent_list *)malloc(sizeof(struct xent_list));
		if (xp == NULL)
			goto alloc_failed;
		if (xent_list == NULL)
			xent_list = xp;
		else
			xp_prev->x_next = xp;
		xp_prev = xp;
		bzero((char *)xp, sizeof(struct xent_list));
		xp->x_xent.xent_dirname = strdup(rpath);
		if (xp->x_xent.xent_dirname == NULL)
			goto alloc_failed;
		if (xent->xent_options) {
			xp->x_xent.xent_options = strdup(xent->xent_options);
			if (xp->x_xent.xent_options == NULL)
				goto alloc_failed;
		}
	}
	endexportent(f);
	last_xtab_time = st.st_mtime;
	return;

alloc_failed:
	logmsg ("Memory allocation failed: %s", strerror(errno));
	xent_free(xent_list);
	xent_list = NULL;
	endexportent(f);
	return;
}

xent_free(xp)
	struct xent_list *xp;
{
	register struct xent_list *next;

	while (xp) {
		if (xp->x_xent.xent_dirname)
			free(xp->x_xent.xent_dirname);
		if (xp->x_xent.xent_options)
			free(xp->x_xent.xent_options);
		next = xp->x_next;
		free((char *)xp);
		xp = next;
	}
}


/*
 * Remove an entry from mounted list 
 */
umount(rqstp)
	struct svc_req *rqstp;
{
	char *path;
	struct mountdlist *ml, *oldml;
	struct hostent *client;
	SVCXPRT *transp;

	ILOG ("umount: SOP");

	transp = rqstp->rq_xprt;
	path = NULL;
	if (!svc_getargs(transp, xdr_path, &path)) {
		svcerr_decode(transp);
		return;
	}

	ILOG ("umount: decoded arguments");

	errno = 0;
	ILOG ("umount: sending reply to remote");
	if (!svc_sendreply(transp, xdr_void, (char *)NULL))
		log_cant_reply(transp);

	client = getclientsname(rqstp, transp);

	ILOG1 ("umount:  remove %s from mountlist", client->h_name);
	if (client != NULL) {
		oldml = mountlist;
		for (ml = mountlist; ml != NULL;
		     oldml = ml, ml = ml->ml_nxt) {
			if (strcmp(ml->ml_path, path) == 0 &&
			    strcmp(ml->ml_name, client->h_name) == 0) {
				if (ml == mountlist) {
					mountlist = ml->ml_nxt;
				} else {
					oldml->ml_nxt = ml->ml_nxt;
				}
				rmtab_delete(ml->ml_pos);
				free(ml->ml_path);
				free(ml->ml_name);
				free((char *)ml);
				break;
			}
		}
	}
	svc_freeargs(transp, xdr_path, &path);
}

/*
 * Remove all entries for one machine from mounted list 
 */
umountall(rqstp)
	struct svc_req *rqstp;
{
	struct mountdlist *ml, *oldml;
	struct hostent *client;
	SVCXPRT *transp;

	ILOG ("umountall: SOP");

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}
	ILOG ("umountall: decoded arguments");

	/* 
	 * We assume that this call is asynchronous and made via the portmapper
	 * callit routine.  Therefore return control immediately. The error
	 * causes the portmapper to remain silent, as apposed to every machine
	 * on the net blasting the requester with a response. 
	 */
	svcerr_systemerr(transp);
	client = getclientsname(rqstp, transp);
	if (client == NULL) {
		return;
	}
	oldml = mountlist;
	for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
		if (strcmp(ml->ml_name, client->h_name) == 0) {
			if (ml == mountlist) {
				mountlist = ml->ml_nxt;
				oldml = mountlist;
			} else {
				oldml->ml_nxt = ml->ml_nxt;
			}
			rmtab_delete(ml->ml_pos);
			free(ml->ml_path);
			free(ml->ml_name);
			free((char *)ml);
		} else {
			oldml = ml;
		}
	}
}

static char *export_opts[] = { ACCESS_OPT, ROOT_OPT, RW_OPT };

/*
 * send current export list 
 */
export(rqstp)
	struct svc_req *rqstp;
{
	struct exportent *xent;
	struct exports *ex;
	struct exports **tail;
	char *grl;
	char *gr;
	struct groups *groups;
	struct groups **grtail;
	struct groups **gr_search_to;
	SVCXPRT *transp;
	struct xent_list *xp;
	int i;

	transp = rqstp->rq_xprt;
	if (!svc_getargs(transp, xdr_void, NULL)) {
		svcerr_decode(transp);
		return;
	}

	ILOG ("export: SOP");

	check_exports();

	ex = NULL;
	tail = &ex;
	for (xp = xent_list ; xp ; xp = xp->x_next) {
		xent = &xp->x_xent;

		groups = NULL;
		grtail = &groups;

		/* Check the access, root, and rw lists.  rpc.mountd will
		 * allow clients who are on the root and rw lists to also
		 * mount a file system so send that information.  But, an empty
		 * access list implies anyone can mount it so don't add root
		 * and rw entries if access list is empty.  The automounter
		 * is the biggest user of this functionality.
		 * See SR 1653072678.
		 */
		for (i = 0; i < sizeof(export_opts)/sizeof(char *); i++) {

			/* This is point to which we will search when seeing if
			 * we have already added this system to the group list.
			 */
			gr_search_to = grtail;

			/* Get the next list of machines that have access. */
			grl = getexportopt(xent, export_opts[i]);
			if (grl != NULL) {
				while ((gr = strtok(grl, ":")) != NULL) {
					struct groups **check;

					grl = NULL;

					/* See if we already have added this
					 * name. This loop looks strange because
					 * its using a **groups in order to
					 * only search the parts of the list
					 * that were added before this
					 * exportopt.  For example, if we're
					 * processing the access list, it will
					 * only search things added for the
					 * root and rw list.
					 */
					for (check = &groups;
						check != gr_search_to;
						check = &((*check)->g_next)) {
						if (!strcmp(gr,
							(*check)->g_name))
							break;
					}

					/* If its new, add it */
					if (check == gr_search_to)
						grtail = newgroup(gr, grtail);
				}
			}
			/* This is kind of tricky, but if the access list
			 * is empty, then anyone can mount it, so we return
			 * an empty group list.  Otherwise, the correct list
			 * is the concatenation of access, rw and root lists.
			 */
			if ((i == 0) && (groups == NULL)) {
				break;
			}
		}
		tail = newexport(xent->xent_dirname, groups, tail);
	}

	errno = 0;
	if (!svc_sendreply(transp, xdr_exports, (char *)&ex))
		log_cant_reply(transp);
	freeexports(ex);
}


freeexports(ex)
	struct exports *ex;
{
	struct groups *groups, *tmpgroups;
	struct exports *tmpex;

	ILOG ("freeexports: SOP");

	while (ex) {
		groups = ex->ex_groups;
		while (groups) {
			tmpgroups = groups->g_next;
			free(groups->g_name);
			free((char *)groups);
			groups = tmpgroups;
		}
		tmpex = ex->ex_next;
		free(ex->ex_name);
		free((char *)ex);
		ex = tmpex;
	}
}


struct groups **
newgroup(name, tail)
	char *name;
	struct groups **tail;
{
	struct groups *new;
	char *newname;

	new = (struct groups *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->g_name = newname;
	new->g_next = NULL;
	*tail = new;
	return (&new->g_next);
}


struct exports **
newexport(name, groups, tail)
	char *name;
	struct groups *groups;
	struct exports **tail;
{
	struct exports *new;
	char *newname;

	new = (struct exports *) exmalloc(sizeof(*new));
	newname = (char *) exmalloc(strlen(name) + 1);
	(void) strcpy(newname, name);

	new->ex_name = newname;
	new->ex_groups = groups;
	new->ex_next = NULL;
	*tail = new;
	return (&new->ex_next);
}

char *
exmalloc(size)
	int size;
{
	char *ret;

	if ((ret = (char *) malloc((u_int)size)) == NULL) {
		logmsg ("Out of memory");
		exit(9);
	}
	return (ret);
}

usage()
{
	fprintf(stderr, "usage: /usr/etc/rpc.mountd [-l log_file] [-t #] [-e | -n]\n");
	fprintf(stderr, "       -t 1   Only log errors (default)\n");
	fprintf(stderr, "       -t 2   Log errors and mount requests\n");
	fprintf(stderr, "       -t 3   Internal tracing\n");

	exit(10);
}

/*
 * Old geth() took a file descriptor. New getfh() takes a pathname.
 * So the the mount daemon can run on both old and new kernels, we try
 * the old version of getfh() if the new one fails.
 */
do_getfh(path, fh)
	char *path;
	fhandle_t *fh;
{
	int fd;
	int res;
	int save;

	res = getfh(path, fh);

/* HPNFS we only have one getfh and its the right one :) */
#ifndef hpux
	if (res < 0 && errno == EBADF) {	
		/*
		 * This kernel does not have the new-style getfh()
		 */
		fd = open(path, 0, 0);
		if (fd >= 0) {
			res = getfh((char *)fd, fh);
			save = errno;
			(void) close(fd);
			errno = save;
		}
	}
#endif
	return (res);
}


FILE *f;

rmtab_load()
{
	char *path;
	char *name;
	char *end;
	struct mountdlist *ml;
	char line[MAXRMTABLINELEN];

	f = fopen(RMTAB, "r");
	if (f != NULL) {
		while (fgets(line, sizeof(line), f) != NULL) {
			name = line;
			path = strchr(name, ':');
			if (*name != '#' && path != NULL) {
				*path = 0;
				path++;
				end = strchr(path, '\n');
				if (end != NULL) {
					*end = 0;
				}
				ml = (struct mountdlist *) 
					exmalloc(sizeof(struct mountdlist));
				ml->ml_path = (char *)
					exmalloc(strlen(path) + 1);
				(void) strcpy(ml->ml_path, path);
				ml->ml_name = (char *)
					exmalloc(strlen(name) + 1);
				(void) strcpy(ml->ml_name, name);
				ml->ml_nxt = mountlist;
				mountlist = ml;
			}
		}
		(void) fclose(f);
		(void) truncate(RMTAB, (off_t)0);
	} 
	f = fopen(RMTAB, "w+");
	if (f != NULL) {
#ifndef hpux
		setlinebuf(f);
#endif
		for (ml = mountlist; ml != NULL; ml = ml->ml_nxt) {
			ml->ml_pos = rmtab_insert(ml->ml_name, ml->ml_path);
		}
	}
}


long
rmtab_insert(name, path)
	char *name;
	char *path;
{
	long pos;

	if (f == NULL || fseek(f, 0L, 2) == -1) {
		return (-1);
	}
	pos = ftell(f);
	if (fprintf(f, "%s:%s\n", name, path) == EOF) {
		return (-1);
	}
	(void) fflush(f);
	return (pos);
}

rmtab_delete(pos)
	long pos;
{
	if (f != NULL && pos != -1 && fseek(f, pos, 0) == 0) {
		(void) fprintf(f, "#");
		(void) fflush(f);
	}
}


#ifndef hpux
char *strdupx(s1)
char *s1;
{
    char *s2;
    extern char *malloc(), *strcpy();

    s2 = malloc(strlen(s1)+1);
    if (s2 != NULL)
        s2 = strcpy(s2, s1);
    return(s2);
}
#endif

/*
**	argparse()	--	parse command line options for daemon
*/

/*
**	we will always sleep (at least) this long between alarms
*/
#define	MIN_INTERVAL	600

/*
**	getopt options
*/
extern	char	*optarg;
extern  int	opterr; /* prevent getopt from printing a message */

mountd_argparse(argc, argv)
int	argc;
char	**argv;
{
    int c;
    char *invo_name = *argv;
    char temp[20];
    int  logoptset = 0;

    /*
    **	parse all arguments passed in to the function and set the
    **	Exitopt and LogFile variables as required
    */

    opterr = 0; /* prevent getopt from printing a message */

    while ((c=getopt(argc, argv, "endt:l:")) != EOF) {
        XPRINTF1 ("argparse getopt returned option %c\n", c);
	switch(c) {
	    case 'e':
		    /*
		    **	exit after running once -- Exitopt == 0
		    */
		    Exitopt = 0l;
		    XPRINTF2 ("argparse option %c, set Exitopt = %d\n", c, Exitopt);
		    break;
	    case 'n':
		    /*
		    **	default is to check with portmapper every hour
		    */
		    Exitopt = MIN_INTERVAL;
		    XPRINTF2 ("argparse option %c, set Exitopt = %d\n", c, Exitopt);
		    break;
	    case 'l':
		    /*
		    **	log file name ...
		    */
		    strcpy(LogFile, optarg);
		    XPRINTF2 ("argparse option %c, set Logfile to %s\n", c, LogFile);
		    break;
	    case 'd':
                    /* debug on */
		    XPRINTF1 ("argparse option %c, set debug on\n", c);
		    Debug = 1;
		    break;

	    case 't':
		    /* trace option */
		    logoptset++;
		    if ( strlen(optarg) != 0) /* not just a -t */
		      {
		        strcpy(temp,optarg);
		        Logopt = atol (temp);
		      }
	            XPRINTF2 ("argparse option %c, Tracing level %d on\n", c, Logopt);
		    break;

	    default:
		    XPRINTF1 ("argparse option %c is illegal\n", c);
		    usage ();
		    fprintf(stderr,"%s: illegal option\n", invo_name);
		    break;
	}
    }

    /* Make sure logging is on if a trace opt was used */
    if (logoptset && (LogFile[0] == '\0')) {
       strcpy (LogFile, "/tmp/mountd.log");
    }

    /*
    **	set up the signal handler and set an alarm; note that if
    **	Exitopt is zero, this will just turn off the alarm ...
    */
    (void) signal(SIGALRM, alarmer);
    (void) alarm(Exitopt);
    XPRINTF1 ("argparse set alarm for %d seconds\n", Exitopt);
}



/* dn_cmp
 * domain name compare
 * compare two hostnames. hostname might be a fully 
 * qualified domain name. 
 *
 * If exportname ends with a dot then remove the dot and do
 * a straight compare. This is a domain name rule.
 * If hostname has a full domain name and exportname doesn't
 * then append THIS hosts domain name to export name before
 * comparing them.
 *
*/

dn_cmp (exportname, hostname)
char *exportname,*hostname;

{

  char  *p;
  int	exportnamelen;
  int   baselen = -1;

 /* is hostname in domain syntax? */
  if ((p = strchr(hostname, '.')) != NULL) {
     baselen = p - hostname;
  }

  exportnamelen=strlen(exportname);

  /* check for dot in last char of exportname */
  if (exportname[exportnamelen-1] == '.') { 
     /* remove the dot and do a straight compare */
     exportname[--exportnamelen] = '\0';
     return (strcasecmp (hostname, exportname));
  }
  else 
     return(checkhost(hostname,exportname,baselen));


} /* dn_cmp */


/*
 * checkhost() is a private function (ie. not exported) 
 * from libc:bsdipc/ruserok.c
 *
 * compare two hostnames which might be in domain name syntax
 *  rhost - the remote hostname
 *  lhost - the name from /etc/exports
 *  len - length of basename of rhost
 */
static int
checkhost(rhost, lhost, len)
	char *rhost, *lhost;
	int len;
{
	static char *domainp = NULL;
	static int nodomain = 0;
	extern struct state _res;

	if (len == -1)
		/* no domain on rhost, must match lhost exactly */
		return(strcasecmp(rhost, lhost));
	if (strncasecmp(rhost, lhost, len))
		/* host name local parts don't match, why go on? */
		return(1);
	if (!strcasecmp(rhost, lhost))
		/* whole domain-extended names match, succeed */
		return(0);
	if (*(lhost + len) != '\0')
		/* lhost has a domain but it didn't match, fail */
		return(1);
	if (nodomain)
		/* already determined there is no local domain, fail */
		return(1);
	if (!domainp) {
		/*
		**  local domain not returned in hostname(),
		**  get it from _res.defdname
		*/
		if (!(_res.options & RES_INIT))
			if (res_init() == -1) {
				/* there is no local domain */
				nodomain = 1;
				return(1);
			}
		domainp = _res.defdname;
	}
	/* rhost's domain must match local domain */
	return(strcasecmp(domainp, rhost + len + 1));
}




/* host_compare
 *  - compare a hostname with a hostent data structure
 *    search the hostname and alias list of the hostent
 *    use dn_cmp because the hostent might be in DNS format
 *
 */
int
host_compare (host, client)
char *host;
struct hostent *client;

{
   char **aliases;

   /* make sure there is something to compare */
   if ( (client == NULL) || (host == NULL) )
      return (1);

   /* check the hostname */
   if (dn_cmp(host, client->h_name) == 0) /* match */
      return (0);

   /* check the alias list */
   for (aliases = client->h_aliases; *aliases != NULL;
	aliases++) {
	  if (dn_cmp(host, *aliases) == 0)
	      return (0);
  }

  /* nothing matches */
  return (1);

}
