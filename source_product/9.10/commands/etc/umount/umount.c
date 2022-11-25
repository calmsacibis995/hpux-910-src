#ifndef lint
    static char *HPUX_ID = "@(#) $Revision: 72.3 $";
#endif

/*
 * umount
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/file.h>
#include <stdio.h>
#include <mntent.h>
#include <errno.h>
#include <time.h>
#include <rpc/rpc.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>				/* for PATH_MAX */

#if (defined(DISKLESS) || defined(DUX) || defined(LOCAL_DISK))
#   include <cluster.h>
#endif

#ifdef TRUX
#   include <sys/security.h>
#   include <sys/audit.h>
#endif

#ifndef PATH_MAX
#   define PATH_MAX	1023
#endif

/*
 * This structure is used to build a list of mntent structures
 * in reverse order from /etc/mtab.
 */
struct mntlist {
	struct mntent *mntl_mnt;
	struct mntlist *mntl_next;
};

int	exit_status = 0;
int	all = 0;
int	nfsmounted = 0;		/* Set if any nfs filesys is mounted for -a */
int	verbose = 0;
int	host = 0;
int	type = 0;

#ifdef GETMOUNT
#  define OPTIONS "asvh:t:"
#else
#  define OPTIONS "avh:t:"
#endif

#if defined(SecureWare) && defined(B1)
    char auditbuf[PATH_MAX];
#endif

#ifdef GETMOUNT
int 	save_mnttab=0;	/* Global flag indicating if old mnttab should be
			    preserved instead of updating from kernel */
#endif /* GETMOUNT */

char	*typestr;
char	*hoststr;

void	*xmalloc();
struct mntlist *mkmntlist();
struct mntent *mntdup();
FILE *mnt_setmntent();		/* retries setmntent if ERRNO = EACCES */
int eachreply();

char errstr[PATH_MAX];

#if defined(SecureWare) && defined(B1)
    extern FILE *umount_setmntent();
#endif

#ifdef GETMOUNT
    extern int update_mnttab();	/* no params, so no special ANSI C extern */
#endif

extern char *optarg;
extern int optind;

main(argc, argv)
	int argc;
	char **argv;
{
	int c;

#if defined(SecureWare) && defined(B1)
	if(ISB1){
		set_auth_parameters(argc, argv);
                initprivs();
		forcepriv(SEC_ALLOWDACACCESS);
		forcepriv(SEC_ALLOWMACACCESS);
#if SEC_ILB
		forcepriv(SEC_ILNOFLOAT);
#endif
		enablepriv(SEC_MULTILEVELDIR);
		if (!authorized_user("mount")) {
		    (void) fprintf (stderr,
				    "umount: you must have the 'mount' authorization\n");
		    exit(1);
		}
	}
#endif
	sync();
	(void) umask(0);
	opterr = 1;
	if (argc == 1) /* no arguments */
	    usage();
	while ((c = getopt(argc, argv, OPTIONS)) != EOF) {
		switch (c) {
		case 'a':
			all++;
			break;
		case 'h':
			all++;
			host++;
			hoststr = optarg;
			break;
		case 't':
			all++;
			type++;
			typestr = optarg;
			break;
#ifdef GETMOUNT
		case 's':
			save_mnttab++;
			break;
#endif /* GETMOUNT */
		case 'v':
			verbose++;
			break;
		default:
			/*
			(void) fprintf(stderr, "umount: unknown option '%c'\n",
				       *options);
			 */
			usage();
		}
	}

	if (all && argv[optind] != NULL) {
		usage();
	}

	umountlist((argc - optind), &argv[optind]);
	exit(exit_status);
	/*NOTREACHED*/
}

umountlist(argc, argv)
	int argc;
	char *argv[];
{
	int i, pid;
	struct mntent *mnt;
	struct mntlist *mntl, *mntcur;
	struct mntlist *mntrev = NULL;
	int tmpfd;
	char *colon;
	FILE *tmpmnt;
	char *tmpname = "/etc/umountXXXXXX";

#ifdef TRUX
	int trusted_nfs;
#endif

#ifdef GETMOUNT
	if (save_mnttab) {		/* for backward compatibility */
#endif /* GETMOUNT */

#if defined(SecureWare) && defined(B1)
		if(ISB1){
			umount_new_mnttab(tmpname);
			tmpmnt = umount_setmntent(tmpname, "w");
		}
		else {
			(void) mktemp(tmpname);
			if ((tmpfd = open(tmpname, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0) {
				(void) sprintf(errstr,
					       "umount: temporary file %s",
					       tmpname);
				perror(errstr);
				exit(2);
			}
			close(tmpfd);
			tmpmnt = setmntent(tmpname, "w");
		}
#else
		(void) mktemp(tmpname);
		if ((tmpfd = open(tmpname, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0) {
			(void) sprintf(errstr,
				       "umount: temporary file %s", tmpname);
			perror(errstr);
			exit(2);
		}
		close(tmpfd);
		tmpmnt = setmntent(tmpname, "w");
#endif
		if (tmpmnt == NULL) {
			(void) sprintf(errstr, "umount: temporary file %s",
				       tmpname);
			perror(errstr);
			exit(2);
		}
#ifdef GETMOUNT
	}
#endif /* GETMOUNT */
	if (all) {
		if (!host &&
#ifdef TRUX
			(!type || (type && (strcmp(typestr, MNTTYPE_NFS) == 0) ||
					(trusted_nfs = (strcmp(typestr, MNTTYPE_TNFS) == 0))))) {
#else
		    (!type || (type && strcmp(typestr, MNTTYPE_NFS) == 0))) {
#endif
			struct mntent *mntp;
			FILE *mntfp = mnt_setmntent(MNT_MNTTAB, "r+");

			if (mntfp == NULL) {
			    (void) sprintf(errstr, "umount: %s", MNT_MNTTAB);
			    perror(errstr);
			    exit(2);
			}

			while (!nfsmounted &&
				((mntp = getmntent(mntfp)) != NULL))
			    if (strncmp(mntp->mnt_type, "nfs", 3) == 0)
				nfsmounted = 1;

			endmntent(mntfp);

			if (nfsmounted) {
			    pid = fork();
			    if (pid < 0)
				    perror("umount: fork");
			    if (pid == 0) {
#ifdef GETMOUNT
				if (save_mnttab) endmntent(tmpmnt);
#else
				endmntent(tmpmnt);
#endif
				clnt_broadcast(MOUNTPROG,

#ifdef TRUX
					(trusted_nfs ? MOUNTVERS_TNFS : MOUNTVERS),
#else
					MOUNTVERS,
#endif
				    MOUNTPROC_UMNTALL,
				/* SR#: 4701-197855
				   The following parameters should be removed
				   to avoid the failure of clnt_broadcast().
				    MOUNTVERS, MOUNTPROC_UMNTALL,
				 */
				    xdr_void, NULL, xdr_void, NULL, eachreply);
				exit(0);
			    }
			}
		}
	}
	/*
	 * get a last first list of mounted stuff, reverse list and
	 * null out entries that get unmounted.
	 */
	for (mntl = mkmntlist(MNT_MNTTAB); mntl != NULL;
	    mntcur = mntl, mntl = mntl->mntl_next,
	    mntcur->mntl_next = mntrev, mntrev = mntcur) {
		mnt = mntl->mntl_mnt;
		if (strcmp(mnt->mnt_dir, "/") == 0) {
			continue;
		}
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}
		if (all) {
			if (type && strcmp(typestr, mnt->mnt_type)) {
				continue;
			}
			if (host) {
#ifdef TRUX
				if (strcmp(MNTTYPE_NFS, mnt->mnt_type) &&
					strcmp(MNTTYPE_TNFS, mnt->mnt_type)) {
#else
				if (strcmp(MNTTYPE_NFS, mnt->mnt_type)) {
#endif
					continue;
				}
				colon = strchr(mnt->mnt_fsname, ':');
				if (colon && *(colon+1)!=':') {
					*colon = '\0';
					if (strcmp(hoststr, mnt->mnt_fsname)) {
						*colon = ':';
						continue;
					}
					*colon = ':';
				} else {
					continue;
				}
			}
#ifdef LOCAL_DISK
			/*
			 * Don't try to unmount a non-local mount 
			 * w/ umount -a unless it is an NFS mount
			 */
			if (mnt->mnt_cnode != 0 && 
				mnt->mnt_cnode != cnodeid() &&
				    strcmp(MNTTYPE_NFS, mnt->mnt_type)) {
				continue;
			}
#endif /* LOCAL_DISK */
			if (umountmnt(mnt)) {
				mntl->mntl_mnt = NULL;
#if defined(SecureWare) && defined(B1)
#ifdef TRUX
				if (ISB1) {
				    (void) sprintf(auditbuf,
						   "successful umount of %s",
						   mnt->mnt_dir);
				    audit_subsystem(auditbuf,
						    "umount successful",
						    ET_OBJECT_UNAV);
				}
#endif TRUX
			}
                        else if (ISB1) {
                                 (void) sprintf(auditbuf, "unmount %s",
						mnt->mnt_dir);
                                 audit_subsystem(auditbuf, "not successful",
						 ET_SUBSYSTEM);
#endif
			}
			continue;
		}

		for (i=0; i<argc; i++) {
			if ((strcmp(mnt->mnt_dir, argv[i]) == 0) ||
			    (strcmp(mnt->mnt_fsname, argv[i]) == 0)) {
				if (umountmnt(mnt)) {
					mntl->mntl_mnt = NULL;
				}
#if defined(SecureWare) && defined(B1)
                                else {
					if (ISB1) {
					    (void) sprintf(auditbuf,
							   "unmount %s",
							   mnt->mnt_dir);
					    audit_subsystem(auditbuf,
							    "not successful"
							    ET_SUBSYSTEM);
					}
                                }
#endif
				*argv[i] = '\0';
				break;
			}
		}
	}

	for (i=0; i<argc; i++) {
		if (*argv[i] && *argv[i] != '/') {
		    (void) fprintf(stderr,
				   "umount: must use full path, %s not unmounted\n",
				   argv[i]);
		    continue;
		}
		if (*argv[i]) {
			struct mntent tmpmnt2;

			tmpmnt2.mnt_fsname = NULL;
			tmpmnt2.mnt_dir = argv[i];
			tmpmnt2.mnt_type = MNTTYPE_HFS;
			umountmnt(&tmpmnt2);
		}
	}
#ifdef TRUX
	if (ISB1) {
		forcepriv(SEC_ALLOWDACACCESS);
		forcepriv(SEC_ALLOWMACACCESS);
	}
#endif TRUX
#ifdef GETMOUNT
	if (save_mnttab) {	/* for backward compatibility */
#endif /* GETMOUNT */
		/*
		 * Build new temporary mtab by walking mnt list
		 */
		for (; mntcur != NULL; mntcur = mntcur->mntl_next) {
			if (mntcur->mntl_mnt) {
				addmntent(tmpmnt, mntcur->mntl_mnt);
			}
		}
		endmntent(tmpmnt);

		/*
		 * Move tmp mtab to real mtab
		 */

#if defined(SecureWare) && defined(B1)
		if ((ISB1) ? (umount_rename(tmpname, MNT_MNTTAB) < 0) :
		    (rename(tmpname, MNT_MNTTAB) < 0))
#else
		if (rename(tmpname, MNT_MNTTAB) < 0)
#endif
		{
			(void) sprintf(errstr,"umount: rename of %s to %s",
				       tmpname, MNT_MNTTAB);
			perror(errstr);
			exit(2);
		}
#ifdef GETMOUNT
	} else {
		/* update mnttab from kernel */
		if (update_mnttab()) {
			(void) fprintf(stderr, "mount: ");
			perror(MNT_MNTTAB);
			exit(2);
		}
	}
#endif /* GETMOUNT */
#ifdef TRUX
	if (ISB1) {
		disablepriv(SEC_ALLOWDACACCESS);
		disablepriv(SEC_ALLOWMACACCESS);
	}
#endif TRUX
}

umountmnt(mnt)
	struct mntent *mnt;
{
	int pid;

#if defined(SecureWare) && defined(B1)
	if ((ISB1) ? (umount_do_umount(mnt->mnt_dir) < 0) :
	    (umount(mnt->mnt_dir) < 0))
#else
	if (umount(mnt->mnt_dir) < 0)
#endif
	{
		exit_status = 2;
		if (errno != EINVAL) {
			(void) sprintf(errstr, "umount: umount(1M) of %s",
				       mnt->mnt_dir);
			perror(errstr);

			if (errno == ENOTBLK)
				return(1);
			else
				return(0);
		}
#if (defined(DISKLESS) || defined(DUX)) && !defined(LOCAL_DISK)
	/* don't clean up mnttab if this is a client */
		if (cnodes(NULL) > 0) {
			struct cct_entry *ccentry;
			if ((ccentry = getcccid(cnodeid())) == NULL) {
				(void) fprintf(stderr,
				"umount: unexpected cluster error: no clusterconf entry found\n");
				return 0;
			}
			if ((ccentry->cnode_type == 'c') ||
				(ccentry->cnode_type == 'w')) {
				/* this is a client, don't mess with mnttab */
				return(0);
			}
		} /* otherwise, let the normal code clean up mnttab */
#else  /* (DISKLESS || DUX) && !LOCAL_DISK */
		/*
		 * EINVAL -- May have attempted to umount a disk
		 * 	which is local to another cnode in the 
		 *	cluster.  Issue a good warning if so...
		 */
		if (mnt->mnt_cnode != cnodeid() && mnt->mnt_cnode != 0 &&
#ifdef TRUX
					strcmp(MNTTYPE_NFS, mnt->mnt_type) &&
					strcmp(MNTTYPE_TNFS, mnt->mnt_type))
#else
					strcmp(MNTTYPE_NFS, mnt->mnt_type)) 
#endif
			(void) fprintf(stderr, "%s: Not mounted locally -- must be unmounted from cnode %d\n", mnt->mnt_fsname, mnt->mnt_cnode);
		else
#endif /* (DISKLESS || DUX) && !LOCAL_DISK */
		    (void) fprintf(stderr, "%s: Not mounted\n", mnt->mnt_dir);
		return(1);
	} else {
#ifdef TRUX
		if ((strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) ||
			(strcmp(mnt->mnt_type, MNTTYPE_TNFS) == 0)) {
#else
		if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) {
#endif
				/* Tell the server in the background. */
			pid = fork();
			if (pid < 0)
				perror("umount: fork");
			if (pid == 0) {
				rpctoserver(mnt);
				exit(0);
			}
		}
		if (verbose) {
			(void) fprintf(stderr, "%s: Unmounted\n",
				       mnt->mnt_dir);
		}
		return(1);
	}
}

usage()
{
#ifdef GETMOUNT
	(void) fprintf(stderr,
		       "usage: umount -a [-vs] [-t <type>] [-h <host>]\n");
	(void) fprintf(stderr,
		       "       umount [-vs] <path>|<dev> ...\n");
#else
	(void) fprintf(stderr,
		       "usage: umount -a [-v] [-t <type>] [-h <host>]\n");
	(void) fprintf(stderr,
		       "       umount [-v] <path>|<dev> ...\n");
#endif
	exit(2);
}

rpctoserver(mnt)
	struct mntent *mnt;
{
	char *p;
	char hostname[BUFSIZ];
	char *errmsg="Warning: Couldn't notify server of umount of %s on %s.\n";
	struct sockaddr_in sin;
	struct hostent *hp;
	int s;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
#ifdef TRUX
	int trusted_nfs = (strcmp(mnt->mnt_type, MNTTYPE_TNFS) == 0);
#endif
		
	if ((p = strchr(mnt->mnt_fsname, ':')) == NULL)
		return(1);

	(void) strncpy(hostname, mnt->mnt_fsname,
		       (size_t)(p - mnt->mnt_fsname));
	hostname[p - mnt->mnt_fsname] = '\0';

	p++;			/* p now points to remote path. */

	if ((hp = gethostbyname(hostname)) == NULL) {
	    if (verbose) {
		(void) fprintf(stderr, errmsg, mnt->mnt_fsname, mnt->mnt_dir);
		(void) fprintf(stderr, "%s not in hosts database\n", hostname);
	    }
	    return(1);
	}
next_ipaddr:
	(void) memset((void *)&sin, 0, sizeof(sin));
	(void) memcpy((void *) &sin.sin_addr, (void *)(hp->h_addr),
		      (size_t) hp->h_length);
	sin.sin_family = AF_INET;
	s = RPC_ANYSOCK;
	timeout.tv_usec = 0;
	timeout.tv_sec = 10;
#ifdef TRUX
	if ((client = clntudp_create(&sin, MOUNTPROG,
		(trusted_nfs ? MOUNTVERS_TNFS : MOUNTVERS),
#else
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
#endif
	    timeout, &s)) == NULL) {
	    if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
		if (hp && hp->h_addr_list[1]) {
		    hp->h_addr_list++;
		    goto next_ipaddr;
		}
	    }
	    if (verbose) {
		(void) fprintf(stderr, errmsg, mnt->mnt_fsname, mnt->mnt_dir);
		clnt_pcreateerror("umount");
	    }
	    return(1);
	}
#ifndef NFS3_2   /* As of NFS3_2 the above clntupd_create */
	         /* does its own bindresvport */
	if (! bindresvport(s)) {
	    if (verbose) {
		(void) fprintf(stderr, errmsg, mnt->mnt_fsname, mnt->mnt_dir);
		(void) fprintf(stderr,
			       "Warning: umount: cannot do local bind.\n");
	    }
	}
#endif /* NFS3_2 */	
	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 25;
	rpc_stat = clnt_call(client, MOUNTPROC_UMNT, xdr_path, &p,
	    xdr_void, NULL, timeout);
	if (rpc_stat != RPC_SUCCESS) {
	    if (verbose) {
		(void) fprintf(stderr, errmsg, mnt->mnt_fsname, mnt->mnt_dir);
		clnt_perror(client, "Warning: umount:");
	    }
	    return(1);
	}
	return(0);
}

/*ARGSUSED*/
eachreply(resultsp, addrp)
	char *resultsp;
	struct sockaddr_in *addrp;
{
	int done = 1;

	return (done);
}

void *
xmalloc(size)
	size_t size;
{
	char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		(void) fprintf(stderr, "umount: ran out of memory!\n");
		exit(2);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	struct mntent *mnt;
{
	struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	(void) strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	(void) strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	(void) strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	(void) strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;
	new->mnt_time = mnt->mnt_time;

#ifdef LOCAL_DISK
	new->mnt_cnode = mnt->mnt_cnode;
#endif

	return (new);
}

struct mntlist *
mkmntlist(file)
	char *file;
{
	FILE *mounted;
	struct mntlist *mntl;
	struct mntlist *mntst = NULL;
	struct mntent *mnt;

	/*
	 *  Open with "r+" to ensure advisory locking within setmntent().
	 */
	 if ((mounted = mnt_setmntent(file, "r+")) == NULL)
		return(NULL);

	while ((mnt = getmntent(mounted)) != NULL) {
		mntl = (struct mntlist *)xmalloc(sizeof(*mntl));
		mntl->mntl_mnt = mntdup(mnt);
		mntl->mntl_next = mntst;
		mntst = mntl;
	}
	return(mntst);
}

#ifndef NFS3_2
bindresvport(sd)
	int sd;
{
	u_short port;
	struct sockaddr_in sin;
	int err = -1;

#	define MAX_PRIV (IPPORT_RESERVED-1)
#	define MIN_PRIV (IPPORT_RESERVED/2)

	get_myaddress(&sin);
	sin.sin_family = AF_INET;
#ifdef TRUX
	(void) forcepriv(SEC_NETPRIVADRR);
#endif
	for (port = (u_short)MAX_PRIV; err && ((u_int)(port >= MIN_PRIV)); port--) {
		sin.sin_port = htons(port);
		err = bind(sd, (void *)&sin, sizeof(sin));
	}
#ifdef TRUX
		(void) disablepriv(SEC_NETPRIVADDR);
#endif
	return (err == 0);
}
#endif /* NFS3_2 */

/*
 * mnt_setmntent():
 * This routine retries setmntent if errno == EACCES, and a test of lockf
 * on the setmntent file reveals that a lock exists.  This is done because
 * setmntent may have failed due to a lockf (which will cause it to
 * return EACCES) in which case we may want to retry, or setmntent may
 * have failed on fopen due to a legitimate access error in which case
 * we want to return NULL.
 *
 * NOTE -- this exact same routine is also in mount.c.  If you have a fix
 * to make here, it's likely that you'll also have to duplicate it there.
 */
FILE *
mnt_setmntent(filename, mode)
char *filename, *mode;
{
    FILE *fp;
    unsigned counter=1;
    unsigned tries=0;
    int fd;

#if defined(SecureWare) && defined(B1)
    while ((fp = (ISB1 ? umount_setmntent(filename, mode) :
			setmntent(filename, mode))) == NULL) {
#else /* B1 */
    while((fp = setmntent(filename, mode)) == NULL) {
#endif /* B1 */
        if (errno!=EACCES && errno!=EAGAIN) /* real failure, return NULL */
            return NULL;

	/* check to see if failure due to lockf */
	if ((fd=open(filename,O_RDONLY))<0)
	    return NULL;

	if (lockf(fd,F_TEST,0)==0) {	/* Hmm, file is not locked now... */
	    close(fd);			/* so we try again, but will only */
	    if (++tries == 10) {	/* put up with this for 10 tries. */
		errno=EACCES;
		return NULL;
	    }
	}
	else {
	    close(fd);
	    if (((counter++) % 4) == 0) {	/* emit msg every ~4 secs */
		(void) fprintf(stderr,"mount: cannot lock %s; ", filename);
		(void) fprintf(stderr,"still trying ...\n");
	    }
	    /*
	     * Sleep in case we are running at a realtime priority greater
	     * than the process that has the lock and is probably trying
	     * to give it up.  This breaks up an otherwise potentially
	     * unbounded, tight loop in the code.
	     */
	    (void) sleep(1);
        }
    }
    return(fp);
}



