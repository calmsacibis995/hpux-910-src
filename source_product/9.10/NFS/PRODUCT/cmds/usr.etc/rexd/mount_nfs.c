/* 	@(#)mount_nfs.c	$Revision: 1.21.109.1 $	$Date: 91/11/19 14:15:28 $  */

/* mount_nfs.c 1.1 87/03/16 NFSSRC */

/*
 *  mount_nfs.c - procedural interface to the NFS mount operation
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

/* NOTE: rexd.c, mount_nfs.c and unix_login.c share a single message	*/
/* catalog (rexd.cat).  For that reason we have allocated messages 	*/
/* 1 through 40 for rexd.c, 41 through 80 for mount_nfs.c and from 81   */
/* on for unix_login.c.  If we need more than 40 messages in this file  */
/* we will need to take into account the message numbers that are 	*/
/* already used by the other files.					*/

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

/** # define OLDMOUNT /** for 2.0 systems only **/

#include <sys/param.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <nfs/nfs.h>
# ifndef OLDMOUNT
#include <sys/mount.h>
# endif OLDMOUNT
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <mntent.h>
#include <sys/vfs.h>

/* include macros for arpa tracing */
#ifdef TRACEON 
#undef TRACEON
#define LIBTRACE
#endif

#include <arpa/trace.h>


#ifdef NLS
extern nl_catd nlmsg_fd;
#endif NLS
#define MAXSLEEP 1  /* in seconds */


/*
 * mount_nfs - mount a file system using NFS
 *
 * Returns: 0 if OK, 1 if error.  
 * 	The "error" string returns the error message.
 */
mount_nfs(fsname, dir, error)
	char *fsname;
	char *dir;
	char *error;
{
	struct sockaddr_in sin;
	struct hostent *hp;
	struct fhstatus fhs;
	char host[256];
	char *path;
	int s = RPC_ANYSOCK, try_next_ipaddr = 0;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int printed1 = 0;
	int printed2 = 0;
	unsigned winks = 0;  /* seconds of sleep time */
	extern errno;
	struct mntent mnt;
	char *index();
	FILE *mnted;
# ifndef OLDMOUNT
	struct nfs_args args;
# endif OLDMOUNT
	TRACE("mount_nfs: SOP");
	path = index(fsname, ':');
	if (path==NULL) {
	        TRACE2("mount_nfs: no host name in %s", fsname);
		errprintf(error,(catgets(nlmsg_fd,NL_SETN,41, "rexd: no host name in %s\n")), fsname);
		return(1);
	}
	*path++ = '\0';
	strcpy(host,fsname);
	TRACE2("mount_nfs: file system is on host %s", host);
	path[-1] = ':';
	/*
	 * Get server's address
	 */
	if ((hp = gethostbyname(host)) == NULL) {
	        TRACE2("mount_nfs: host %s is not in hosts database", host);
		errprintf(error,
		    (catgets(nlmsg_fd,NL_SETN,42, "rexd: %s not in hosts database\n")), host);
		return (1);
	}

	/*
	 * get fhandle of remote path from server's mountd.
	 * We have to use short timeouts since otherwise our incoming
	 * RPC call will time out.
	 */
	do {
	        TRACE("mount_nfs: top of loop to get file handle from host");
		bzero(&sin, sizeof(sin));
		bcopy(hp->h_addr, (char *) & sin.sin_addr, hp->h_length);

		sin.sin_family = AF_INET;
		timeout.tv_usec = 0;
		timeout.tv_sec = 5;
		/* winks = 0; */
		s = RPC_ANYSOCK;
		do {
		    if (sin.sin_port==0) {
		    	/*
			 * Ask portmapper for port number since
			 * the standard pmap_getport has too long a timeout
			 */
/*		        TRACE("mount_nfs: attempting to get a port number for the MOUNTPROG");
*/
			int ps = RPC_ANYSOCK;
			struct pmap parms;
			short port;

			sin.sin_port = htons(PMAPPORT);
			TRACE("mount_nfs: creating rpc client for portmapper");
			/* make a client handle; we haven't contacted 
			 * the remote port mapper yet */
			client = clntudp_create(&sin, PMAPPROG, PMAPVERS,
				timeout, &ps);
			if (client==NULL) { /* may be there's no memory */
			    TRACE("mount_nfs: clntudp_create returned NULL");
			    if (winks < MAXSLEEP)
			      {
				TRACE("mount_nfs: incrementing winks");
				winks++;
			      }
			    else {
			      TRACE2("mount_nfs: can't map port for %s", host);
			      errprintf(error,
			          (catgets(nlmsg_fd,NL_SETN,43, "rexd: can not map port for %s\n")), host);
			      return(1);
			    }
			    TRACE2("mount_nfs: sleeping for %d seconds",winks);
			    sleep(winks);
			    sin.sin_port = 0;
			    TRACE("mount_nfs: try again ");
			    continue;
			}

			/* at this point we have a handle with all info
			 * to reach the remote machine if it is up */

			parms.pm_prog = MOUNTPROG;
			parms.pm_vers = MOUNTVERS;
			parms.pm_prot = IPPROTO_UDP;
			parms.pm_port = 0;
			TRACE("mount_nfs: call port mapper to get port number for mount");
			rpc_stat = clnt_call(client,
					     PMAPPROC_GETPORT, 
				             xdr_pmap, &parms,
					     xdr_u_short, &port,
					     timeout);

		        if (rpc_stat != RPC_SUCCESS) {
			    	if (winks < MAXSLEEP) {
					TRACE("mount_nfs: clnt_call failed, incrrementing winks");
					winks++;
			        }
			    	else {
            		    	 if ((rpc_stat == RPC_TIMEDOUT) || (rpc_stat == RPC_PMAPFAILURE)) {
			          logmsg((catgets(nlmsg_fd,NL_SETN,71, "rexd: failed to reach server at the address %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
				    if (hp && hp->h_addr_list[1]) {
				    	hp->h_addr_list++;
			      		clnt_destroy(client);
				    	try_next_ipaddr  = 1;
			                logmsg((catgets(nlmsg_fd,NL_SETN,72, "rexd: now trying %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
					break; /* out of the do loop */
				    }
            			}
			      		TRACE2("mount_nfs: %s not responding to prot map request. giving up", host);
			      		errprintf(error,
			          	(catgets(nlmsg_fd,NL_SETN,44, "rexd: %s not responding to port map request\n")), 
				   host);
			      		clnt_destroy(client);
			      		return(1);
			    	}
			    if (!printed1++) {
			        TRACE2("mount_nfs: %s not responding to portmap request",host);
				logmsg(
			    (catgets(nlmsg_fd,NL_SETN,45, "rexd: %s not responding to portmap request")), host);
				logclnt_perror(client,"");
			    }
			    
			    TRACE2("mount_nfs: sleeping for %d seconds",winks);
			    sleep(winks);
			    clnt_destroy(client);
			    client = NULL;
			    close(ps);
			    sin.sin_port = 0;
			    TRACE("mount_nfs: try again "); 
			    continue;
			}
			sin.sin_port = ntohs(port);
 			TRACE2("mount_nfs:port for MOUNTPROG is %d",sin.sin_port);
			clnt_destroy(client);
			close(ps);
		    }

		    TRACE("mount_nfs: creating client for MOUNTPROG");
		    if ((client = clntudp_create(&sin,
			    MOUNTPROG, MOUNTVERS, timeout, &s)) == NULL) {
			if (winks < MAXSLEEP)
			  {
				TRACE("mount_nfs: clnt_call failed, incrrementing winks");
				winks++;
			  }
			else {
			  if (rpc_createerr.cf_stat == RPC_PROGNOTREGISTERED)
			    {
			      TRACE2("mount_nfs: %s is not running a mount daemon", host);
			      errprintf(error,
			          (catgets(nlmsg_fd,NL_SETN,47, "rexd: %s is not running a mount daemon\n")), host);
			    }
			  else
			    {
			        TRACE2("mount_nfs: %s not responding to mount request, giving up", host);
			  	errprintf(error,
				  (catgets(nlmsg_fd,NL_SETN,48, "rexd: %s not responding to mount request\n")), 
				    host);
			      }
			  return(1);
			}
			TRACE2("mount_nfs: sleeping for %d seconds",winks);
			sleep(winks);
			if (!printed1++) {
			        TRACE2("mount_nfs: %s not responding to mount request",host);
				logmsg(
			    (catgets(nlmsg_fd,NL_SETN,49, "rexd: %s not responding to mount request")), host);
				logclnt_pcreateerror("");
			}
		    }
		} while (client == NULL);

            	if (try_next_ipaddr) {
			try_next_ipaddr = 0;
			continue;	/* with the outer do loop */
		}

		TRACE2("mount_nfs: we have contacted mountd daemon on %s",host);
		client->cl_auth = authunix_create_default();
		timeout.tv_usec = 0;
		timeout.tv_sec = 25;
                TRACE("mount_nfs: calling mount daemon to get file handle");
		rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &path,
		    xdr_fhstatus, &fhs, timeout);
		if (rpc_stat != RPC_SUCCESS) {
			if (!printed2++) {
                                TRACE2("mount_nfs: %s not responding to mount request", host);
				logmsg(
				    (catgets(nlmsg_fd,NL_SETN,51, "rexd: %s not responding to mount request")), host);
				logclnt_perror(client, "");
			}
		}
		close(s);
		clnt_destroy(client);
	} while (rpc_stat == RPC_TIMEDOUT || fhs.fhs_status == ETIMEDOUT);

	if (rpc_stat != RPC_SUCCESS || fhs.fhs_status) {
		errno = fhs.fhs_status;
                TRACE2("mount_nfs: clnt_call returned %d", errno);
		if (errno == EACCES) {
                       TRACE2("mount_nfs: client not on export list for %s", fsname);
			errprintf(error,
			  (catgets(nlmsg_fd,NL_SETN,53, "rexd: not in export list for %s\n")),
			    fsname);
		}
		return (1);
	}
	if (printed1 || printed2) {
                TRACE2("mount_nfs: %s finally responded", host);
		logmsg((catgets(nlmsg_fd,NL_SETN,54, "rexd: %s OK\n")), host);
	}

	/*
	 * remote mount the fhandle on the local path.
	 * We have to mount it soft otherwise the incoming RPC will time out.
	 */
        TRACE("mount_nfs: prepare to mount fhandle to local file system");
# ifdef OLDMOUNT
	if (nfsmount(&sin, &fhs.fhs_fh, dir, 0, 0, 0) <0) {
# else OLDMOUNT
	sin.sin_port = htons(NFS_PORT);
	memset(&args,0,sizeof(struct nfs_args));
	args.addr = &sin;
	args.fh = &fhs.fhs_fh;
	args.flags = NFSMNT_HOSTNAME | NFSMNT_FSNAME | NFSMNT_SOFT;
	args.hostname = host;
	args.fsname = fsname;
#ifdef hpux
        TRACE("mount_nfs: calling vfsmount");
	if (vfsmount(MOUNT_NFS, dir, M_NOSUID, &args) < 0) {
#else not hpux
	if (mount(MOUNT_NFS, dir, M_NOSUID, &args) < 0) {
#endif hpux
# endif OLDMOUNT
                TRACE2("mount_nfs: could not mount fhandle to local file system. errno = %d", errno);
		errprintf(error,(catgets(nlmsg_fd,NL_SETN,70, "rexd: unable to mount %1$s: %2$s\n")),
			fsname, strerror(errno));
		return (1);
	}

	/*
	 * update /etc/mtab
	 */
        TRACE("mount_nfs: update mnttab");
	mnt.mnt_fsname = fsname;
	mnt.mnt_dir = dir;
	mnt.mnt_type = MNTTYPE_NFS;
	mnt.mnt_opts = "rw,noquota,soft";
	mnt.mnt_freq = 0;
	mnt.mnt_passno = 0;
	newmtab(&mnt, (char *)NULL);
        TRACE("mount_nfs: EOP");
	return (0);
}

/* number of tries to umount the temporary mounted file system */
#define UNMOUNTTRIES 10

/*
 * umount_nfs - unmount a file system when finished
 */
umount_nfs(fsname, dir)
	char *fsname, *dir;
{
	char *p, *index();

	struct sockaddr_in sin;

	struct hostent *hp;
	int s = RPC_ANYSOCK;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	int  count = 0;             /* how many times have we tried umount */

	TRACE("umount_nfs: SOP");
        /*
         * Give the filesystem time to become un-busy when unmounting.
         * If child aborted and is takes a core dump, we may receive the
         * SIGCHLD before the core dump is completed.
         */
        while (umount(dir) == -1) {
                if (errno != EBUSY) 
		  {
		    TRACE3("umount_nfs: call to umount(%s) faild: %s", 
			   dir, strerror(errno));
		    log_perror(dir);
		    return (1);
		  }
                if (++count > UNMOUNTTRIES)
		  {
		    TRACE2("umount_nfs: %s busy could not umount", dir);
		    return (1);
		  }
                sleep (1);
	      } /*while */

	newmtab((struct mntent *)NULL, dir);
	if ((p = index(fsname, ':')) == NULL)
	  {
	    TRACE("umount_nfs: file system was local");
	    return(1);
	  }
	*p++ = 0;
	
	  if ((hp = gethostbyname(fsname)) == NULL) { 
	        TRACE2("umount_nfs: %s not in hosts database", fsname);
	 	logmsg((catgets(nlmsg_fd,NL_SETN,57, "rexd: %s not in hosts database\n")), fsname);
	  	return(1);
	  }
	 
next:
	bzero(&sin, sizeof(sin));
	bcopy(hp->h_addr, (char *) & sin.sin_addr, hp->h_length);
	sin.sin_family = AF_INET;
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
	s = RPC_ANYSOCK;
	TRACE("umount_nfs: calling clntudp_create, MOUNTPROG");
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS, timeout, &s)) == NULL) {
	        TRACE("umount_nfs: clntudp_create failed");
		logclnt_pcreateerror((catgets(nlmsg_fd,NL_SETN,58, "rexd: warning on umount create (did not umount):")));
              	if ((rpc_createerr.cf_stat == RPC_PMAPFAILURE) || (rpc_createerr.cf_stat == RPC_TIMEDOUT)) {
	     		logmsg((catgets(nlmsg_fd,NL_SETN,71, "rexd: failed to reach server at the address %s\n")), inet_ntoa(*(struct in_addr *)hp->h_addr));
	        	if (hp && hp->h_addr_list[1]) {
	     			hp->h_addr_list++;
	     			logmsg((catgets(nlmsg_fd,NL_SETN,72, "rexd: now trying %s\n")), inet_ntoa(*(u_long *)hp->h_addr));
	     			goto next;
	     		}
             	} else
	      
			return(1);
	}
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 25;
	TRACE("umount_nfs: calling clnt_call, MOUNTPROC_UMNT");
	rpc_stat = clnt_call(client, MOUNTPROC_UMNT, xdr_path, &p,
	    xdr_void, NULL, timeout);
	if (rpc_stat != RPC_SUCCESS) {
        	TRACE("umount_nfs: clnt_call, failed");
		logclnt_perror(client, (catgets(nlmsg_fd,NL_SETN,60, "rexd: warning umount (did not umount):")));
		return(1);
	}
	TRACE("umount_nfs: EOP");
	return(0);
}

#define MAXTRIES 30

newmtab(addmnt, deldir)
	struct mntent *addmnt;
	char *deldir;
{
	char *from = "/etc/omtabXXXXXX";
	char *to = "/etc/nmtabXXXXXX";
	FILE *fromp, *top;
	int count = 0;
	struct mntent *mnt;
	struct stat statbuf;    /* used to preserve the modes on MNT_MNTTAB */

	TRACE("newmtab: SOP");
	while ( (fromp = setmntent(MNT_MNTTAB, "r+")) == NULL) {
		if (++count > MAXTRIES) {
		        TRACE2("newmtab:can't get %s lock", MNT_MNTTAB);
			logmsg((catgets(nlmsg_fd,NL_SETN,62, "rexd: can't get %s lock\r\n")), MNT_MNTTAB);
			return;
		}
		sleep(1);
	}
	TRACE2("newmtab: locked %s", MNT_MNTTAB);
	mktemp(to);
	top = setmntent(to, "w");
	if (fromp == NULL || top == NULL) {
	        TRACE3("newmtab: can't open %s or %s", from, to);
		rename(from, MNT_MNTTAB);
		logmsg((catgets(nlmsg_fd,NL_SETN,63, "rexd: can't open %1$s or %2$s\r\n")), from, to);
		return;
	}
	TRACE2("newmtab: creating new %s", MNT_MNTTAB);
	while ((mnt = getmntent(fromp)) != NULL)
		if (deldir == NULL || strcmp(mnt->mnt_dir, deldir) != 0)
			addmntent(top, mnt);
	if (addmnt)
	  {
	        TRACE2("newmtab: adding new netry to %s", MNT_MNTTAB);
		addmntent(top, addmnt);
	  }
	/* keep the modes of MNT_MNTTAB the same as before */
        if (!fstat (fileno (fromp), &statbuf))
                (void) fchmod (fileno (top), statbuf.st_mode);
	endmntent(top);
	if (rename(to, MNT_MNTTAB)) {
	        TRACE3("newmtab: can not rename %s to %s", to, MNT_MNTTAB);
		logmsg((catgets(nlmsg_fd,NL_SETN,64, "rexd: can not rename %1$s to %2$s\r\n")), 
			to, MNT_MNTTAB);
		rename(from, MNT_MNTTAB);
	}
	endmntent(fromp);
	TRACE("newmtab: EOP");
}


/*
 * errprintf will print a message to the log file
 * and return the same string in the given message buffer.
 * We add an extra return character in case the console is in raw mode.
 */
errprintf(s,p1,p2,p3,p4,p5,p6,p7)
	char *s;
{
	nl_sprintf(s,p1,p2,p3,p4,p5,p6,p7);
	logmsg((catgets(nlmsg_fd,NL_SETN,69, "%s")),s);
}
