#ifndef lint
static	char rcsid[] = "@(#)nfsd:	$Revision: 1.37.109.3 $	$Date: 93/11/23 09:20:00 $";
#endif
#ifndef lint
/* (#)nfsd.c	2.1 86/04/17 NFSSRC */ 
/* static char sccsid[] = "nfsd.c 1.1 86/02/03 Copyr 1985 Sun Micro"; */
#endif

/*
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

/* NFS server */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <nfs/nfs.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/lock.h>

#ifdef SecureWare
#include <sys/types.h>
#include <sys/security.h>
#include <prot.h>
#endif /* SecureWare */

#if defined(SecureWare) && defined(B1)


#define ENABLEPRIV(priv) \
	 { \
          if (ISB1)  \
             if (nfs_enablepriv(priv)) { \
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7,"nfsd: needs to be executed with SEC_CHROOT privilege"))); \
		exit(EACCES); \
	     } \
	 }

#define DISABLEPRIV(priv)       if (ISB1)  nfs_disablepriv(priv);

#else

#define ENABLEPRIV(priv)     {}
#define DISABLEPRIV(priv)    {}

#endif /* SecureWare && B1 */


extern errno;
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

struct sigvec oldsigvec;

void
catch()
{
}

main(argc, argv)
char	*argv[];
{
	register int sock;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	char *dir = "/";
	int nservers = 1;
	int pid, t;
	/* Used when checking if file descriptors 0, 1 and 2 are open */

	struct  stat  statbuf;
	int	fdes, s;   		/* file descriptor */
	struct  sigvec  newsigvec;



#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("nfsd",0);
#endif NLS

#if defined(SecureWare) && defined(B1)
/*
 * This program assumes it will be started by the "epa" command and that it
 * will have the default privileges plus SEC_CHROOT.
 * It will then be disabled until it is needed later where it
 *  will be raised and lowered.
 */

      if (ISB1) {
		/*
                 * set_auth_parameters() will change the umask, so keep
                 * a copy and restore it.
                 * 
		 * Only allow a user who is a member of the "network"
		 * protected subsystem to execute and utilize the potential
		 * privileges associated with this program.
		 */
                mode_t cmask = umask(0);
		set_auth_parameters(argc, argv);
                umask(cmask);
		if (authorized_user("") == 0) {
			fprintf(stderr, catgets(nlmsg_fd,NL_SETN,8, "nfsd: Not authorized for network subsystem:  Permission denied\n"));
			exit(1);
		}
		nfs_initpriv();
		DISABLEPRIV(SEC_CHROOT);
      }
#endif /* SecureWare && B1 */


/* HPNFS Only the superuser can run nfsd.  If you were not the     */
/* HPNFS supersuser it would fail during the chroot with the error */
/* HPNFS "/: not owner".					   */
/* This only needs to be checked in the non-B1 environment.  If    */
/* are in a B1 environment, being root does not have any special   */
/* meaning.                                                        */
/*
 * I am not bothering to check for the SEC_CHROOT since it will soon
 * be enabled and the program will exit if it is not available.
 */

#if defined(SecureWare) && defined(B1)
	if (! ISB1) 
#endif /* SecureWare && B1 */
	   {  /* These braces are here just to avoid another ifdef */
		      if (geteuid()) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "You must be root to run %s\n")), argv[0]);
			exit(EACCES);
		      } 
	   } 


	if (argc > 2) {
	  	fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,2, "usage: %s [servers] (servers >= 1)\n")), argv[0]);
	 	exit(1);
	}
	if (argc == 2) {
		nservers = atoi(argv[1]);

/* HPNFS If you use a negative, zero or non-numeric argument with SUN's nfsd */
/* HPNFS it will fork processes until it fills the process table.  This fix  */
/* HPNFS should avoid the problem.					     */

		if (nservers <= 0)
		{
		   	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "usage: %s [servers] (servers >= 1)\n")),
							 	      argv[0]);
			exit(1);
		}
	}

	/*
	 * Set current and root dir to server root
	 */
	ENABLEPRIV(SEC_CHROOT);
	if (chroot(dir) < 0) {
		DISABLEPRIV(SEC_CHROOT);
		perror(dir);
		exit(1);
	}
	DISABLEPRIV(SEC_CHROOT);
	if (chdir(dir) < 0) {
		perror(dir);
		exit(1);
	} 

#ifndef DEBUG

/* HPNFS If there are already other nfs daemons running we would like to */
/* HPNFS an error message about it.  For that reason we have moved  the  */
/* HPNFS check before the setpgrp and reopen file descriptors 0, 1 and 2 */
/* HPNFS only if they were not already open.				 */

	fdes = open("/",O_RDONLY);
	for (s = 0; s < 3; s++)
		if ((fstat(s,&statbuf)== -1) && (errno == EBADF))
			dup2(fdes,s);
	for (s = 3; s < 10; s++)
		(void) close(s);
#endif DEBUG

	addr.sin_addr.s_addr = 0;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NFS_PORT);
	if ( ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	    || (bind(sock, &addr, len) != 0)
	    || (getsockname(sock, &addr, &len) != 0) ) {
		(void)close(sock);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "nfsd: server already active\n")));
		exit(1);
	}

#ifndef DEBUG
	/*
	 * Background 
	 */
        pid = fork();
	if (pid < 0) {
		perror((catgets(nlmsg_fd,NL_SETN,5, "nfsd: fork")));
		exit(1);
	}
	if (pid != 0)
		exit(0);
#endif DEBUG

	setpgrp();

	if (plock(PROCLOCK) < 0) { /* Make it NON-SWAPPABLE  tjs 7/93 */
		perror("plock");
	}

	/* register with the portmapper */
	/* if the registering fails it should not be a problem since */
	/* the port for nfsd is a well-known port.		     */

	pmap_unset(NFS_PROGRAM, NFS_VERSION);
	pmap_set(NFS_PROGRAM, NFS_VERSION, IPPROTO_UDP, NFS_PORT);
	while (--nservers) {
		if (!fork()) {
			break;
		}
	}
	newsigvec.sv_handler = catch;
	newsigvec.sv_mask = 0;
	newsigvec.sv_flags = 0;

	sigvector(SIGTERM, &newsigvec, &oldsigvec);

/* HPNFS On the s300 if nfssvc is not configured into the kernel */
/* HPNFS we do not return a SIGSYS, we return a ENOPROTOOPT.  On */
/* HPNFS the s800 we get a SIGSYS, which when ignored produces a */
/* HPNFS EINVAL.						 */

	newsigvec.sv_handler = SIG_IGN;
	newsigvec.sv_mask = 0;
	newsigvec.sv_flags = 0;

	sigvector(SIGSYS, &newsigvec, &oldsigvec);

#ifdef NLS
	catclose(nlmsg_fd);
#endif NLS

#ifdef BFA
	_UpdateBFA();
#endif BFA
	nfssvc(sock); 

/* HPNFS This is the error the s300 returns if the system call is not */
/* HPNFS configured in.						      */

	if ((nservers == 1) && ((errno == ENOPROTOOPT) || (errno == EINVAL)))
	{	
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN, 6, "nfsd: NFS system call is not available.\n      Please configure NFS into your kernel.\n")));
	}
	pmap_unset(NFS_PROGRAM, NFS_VERSION);
	exit(1);
}

