#ifndef lint
static  char rcsid[] = "@(#)automount:	$Revision: 1.6.109.9 $	$Date: 93/12/13 16:35:55 $";
#endif

#ifndef lint
static char sccsid[] = 	"(#)auto_main.c	1.6 90/07/24 4.1NFSSRC Copyr 1990 Sun Micro";
#endif

#ifdef PATCH_STRING
static char *patch_2967="@(#) PATCH_9.0: auto_main.o $Revision: 1.6.109.9 $ 93/08/04 PHNE_2967"
;
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/* HP Native Language Support Declarations */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
nl_catd catd;
#endif NLS

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <rpc/rpc.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#ifndef hpux /* HP calls setpgrp instead of ioctl - pdb */
#include <sys/ioctl.h>
#endif
#include <sys/stat.h>
#include "nfs_prot.h"
#include <netinet/in.h>
#include <mntent.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <nfs/nfs_clnt.h>

#define NFSCLIENT
typedef nfs_fh fhandle_t;
/* HPNFS - pdb */
#ifdef hpux
#define _NFS_AUTOMOUNT
#include <nfs/nfs.h>
#endif
#include <sys/mount.h>

#include "automount.h"

extern errno;

#ifdef hpux
/* HPNFS mod. by hv 6/15/93 - for DTS INDaa14769 */
extern havechild;
#endif

void catch();
void reap();
void set_timeout();
void domntent();

/* HPNFS - dynamically turn on/off trace 11/16/93 hv */
void toggle_trace();

/* HPNFS 11/15/93 hv
 * Fix bug found in included flat file map
 * loadmaster_file returns -1 if fail; otherwise, return 0.
 */
#ifndef hpux
void loadmaster_file();
#endif

void loadmaster_yp();

#define	MAXDIRS	10

#define	MASTER_MAPNAME	"auto.master"

int maxwait = 60;
/* these are already declared in automount.h - pdb */
int mount_timeout = 30;
int max_link_time = 5*60;
int verbose, syntaxok;
dev_t tmpdev;
int yp;

u_short myport;

main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *transp;
	extern void nfs_program_2();
	extern void read_mtab();
	static struct sockaddr_in sin;	/* static causes zero init */
	struct nfs_args args;
	struct autodir *dir, *dir_next;
	int pid;
	int bad;
        int j; /* rb 2-4-92 temp variable */
	int err; /* This is for a bug fix to the original code - pdb */
	int master_yp = 1;
	char *master_file;
	struct hostent *hp;
	struct stat stbuf;
	extern int trace;
	char pidbuf[64];
#ifdef hpux
	/* fsname is an extra field in the nfs_args structure passed    */
	/* to vfsmount.  It is not documented on 8.0 man page - pdb     */
	char fsname[64];
#endif

	/* Open the message catalog for HP Native Language Support */

#ifdef NLS
	nl_init(getenv("LANG"));
	catd = catopen("automount",0);
#endif NLS

	if (geteuid() != 0) {
		fprintf(stderr, (catgets(catd,NL_SETN,301, "Must be root to use automount\n")));
		exit(1);
	}

	argc--;
	argv++;

	openlog("automount", LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_DAEMON);

	(void) setbuf(stdout, (char *)NULL);
	(void) gethostname(self, sizeof self);
	hp = gethostbyname(self);
	if (hp == NULL) {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,302, "Can't get my address")));
		exit(1);
	}
	bcopy(hp->h_addr, (char *)&my_addr, hp->h_length);
	(void) getdomainname(mydomain, sizeof mydomain);
	(void) strcpy(tmpdir, "/tmp_mnt");
	master_file = NULL;
	syntaxok = 1;	/* OK to log map syntax errs to console */

	while (argc && argv[0][0] == '-') switch (argv[0][1]) {
	case 'n':
        case 'm':
        case 'T':
        case 'v':
                for(j =1; argv[0][j] != 0 ; j++)
                {
                 switch (argv[0][j]) {
                  case 'n':
                     nomounts++;
                     break;

                  case 'v':
                    verbose++;
                    break;

                  case 'T':
                    trace++;
                    break;
 
                  case 'm':
                   master_yp = 0;
                   break;

                  default:
                    break;

                  }
                }

                  argc--;
                  argv++;
                  break;

	case 'f':
		master_file = argv[1];
		argc -= 2;
		argv += 2;
		break;
	case 'M':
		(void) strcpy(tmpdir, argv[1]);
		argc -= 2;
		argv += 2;
		break;
	case 't':			/* timeouts */
		if (argc < 2) {
			(void) fprintf(stderr, (catgets(catd,NL_SETN,303, "Bad timeout value\n")));
			usage();
		}
		if (argv[0][2]) {
			set_timeout(argv[0][2], atoi(argv[1]));
		} else {
			char *s;

			for (s = strtok(argv[1], ","); s ;
				s = strtok(NULL, ",")) {
				if (*(s+1) != '=') {
					(void) fprintf(stderr,
						(catgets(catd,NL_SETN,304, "Bad timeout value\n")));
					usage();
				}
				set_timeout(*s, atoi(s+2));
			}
		}
		argc -= 2;
		argv += 2;
		break;

	case 'D':
		if (argv[0][2])
			(void) putenv(&argv[0][2]);
		else {
			(void) putenv(argv[1]);
			argc--;
			argv++;
		}
		argc--;
		argv++;
		break;

	default:
		usage();
	}

	if (verbose && argc == 0 && master_yp == 0 && master_file == NULL) {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,305, "no mount maps specified")));
		usage();
	}

	yp = 1;
	err = yp_bind(mydomain);
	if (err) {
		if (verbose)
			syslog(LOG_ERR, (catgets(catd,NL_SETN,306, "NIS bind failed: %s")),
				yperr_string(err));
		yp = 0;
	}

	read_mtab();
	/*
	 * Get mountpoints and maps off the command line
	 */
	while (argc >= 2) {
		if (argc >= 3 && argv[2][0] == '-') {
			dirinit(argv[0], argv[1], argv[2]+1, 0);
			argc -= 3;
			argv += 3;
		} else {
			dirinit(argv[0], argv[1], "rw", 0);
			argc -= 2;
			argv += 2;
		}
	}
	if (argc)
		usage();

/* HP does not have the environment variable ARCH - pdb */
#ifndef hpux
	if (getenv("ARCH") == NULL) {
		char buf[16], str[32];
		int len;
		FILE *f;

		f = popen("arch", "r");
		(void) fgets(buf, 16, f);
		(void) pclose(f);
		if (len = strlen(buf))
			buf[len - 1] = '\0';
		(void) sprintf(str, "ARCH=%s", buf);
		(void) putenv(str);
	}
#endif
	
	if (master_file) {
		loadmaster_file(master_file);
	}
	if (master_yp) {
		loadmaster_yp(MASTER_MAPNAME);
	}

	/*
	 * Remove -null map entries
	 */
	for (dir = HEAD(struct autodir, dir_q); dir; dir = dir_next) {
	    	dir_next = NEXT(struct autodir, dir);
		if (strcmp(dir->dir_map, "-null") == 0) {
			REMQUE(dir_q, dir);
		}
	}
	if (HEAD(struct autodir, dir_q) == NULL)   /* any maps ? */
		exit(3);   /* rb 1/31/92 */

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,307, "Cannot create UDP service")));
		exit(1);
	}
	if (!svc_register(transp, NFS_PROGRAM, NFS_VERSION, nfs_program_2, 0)) {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,308, "svc_register failed")));
		exit(1);
	}
	if (mkdir_r(tmpdir) < 0) {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,309, "couldn't create %s: %m")), tmpdir);
		exit(1);
	}
	if (stat(tmpdir, &stbuf) < 0) {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,310, "couldn't stat %s: %m")), tmpdir);
		exit(1);
	}
	tmpdev = stbuf.st_dev;

#ifdef DEBUG
	pid = getpid();
	if (fork()) {
		/* parent */
		signal(SIGTERM, catch);
		signal(SIGHUP,  read_mtab);
		signal(SIGCHLD, reap);  
		auto_run();
		syslog(LOG_ERR, (catgets(catd,NL_SETN,311, "svc_run returned")));
		exit(1);
	}
#else NODEBUG
	switch (pid = fork()) {
	case -1:
		syslog(LOG_ERR, (catgets(catd,NL_SETN,312, "Cannot fork: %m")));
		exit(1);
	case 0:
		/* child */
		{ int tt = open("/dev/tty", O_RDWR);
		  if (tt > 0) {
/* Use setpgrp for HP - pdb */
#ifdef hpux
			setpgrp();
#else
			(void) ioctl(tt, TIOCNOTTY, (char *)0);
#endif
			(void) close(tt);
		  }
		}
		signal(SIGTERM, catch);
		signal(SIGHUP, read_mtab);
         	signal(SIGCHLD, reap);      
                signal(SIGUSR2,toggle_trace);
		auto_run();
		syslog(LOG_ERR, (catgets(catd,NL_SETN,313, "svc_run returned")));
		exit(1);
	}
#endif

	/* parent */
	sin.sin_family = AF_INET;
	sin.sin_port = htons(transp->xp_port);
	myport = transp->xp_port;

	/* The following is needed for cluster nodes to be able to access */
	/* file systems mounted by automount running on another cluster   */
	/* node.  The system with automount running must be contacted -   */
	/* not the local system. - pdb                                    */
#ifdef hpux
	sin.sin_addr.s_addr = my_addr.s_addr;
#else
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	args.addr = &sin;
	args.flags = NFSMNT_INT + NFSMNT_TIMEO +
#ifdef hpux
		     /* An HP only flag ... */
		     NFSMNT_FSNAME +

		     /* This flag was added to vfsmount on 8.07 to allow  */
		     /* automount mount points to appear as "ignore"      */
		     /* entries in /etc/mnttab while still being type NFS */
		     /* in the kernel - pdb                               */
		     NFSMNT_IGNORE +
#endif
                     NFSMNT_HOSTNAME + NFSMNT_RETRANS;
	args.timeo = (mount_timeout + 5) * 10;
	args.retrans = 5;
	bad = 1;

#ifdef hpux
	/* Extra field in nfs_args structure on HP machines - pdb */
	/* Want to make this look like what happens in addmntent! */
	(void) sprintf(fsname,"%s:(pid%d)", self, pid);
	args.fsname = fsname;
#endif

	/*
	 * Mount the daemon at its mount points.
	 * Start at the end of the list because that's
	 * the first on the command line.
	 */
	for (dir = TAIL(struct autodir, dir_q); dir;
	    dir = PREV(struct autodir, dir)) {
		(void) sprintf(pidbuf, "(pid%d@%s)", pid, dir->dir_name);
		if (strlen(pidbuf) >= HOSTNAMESZ-1)
			(void) strcpy(&pidbuf[HOSTNAMESZ-3], "..");
		args.hostname = pidbuf;
		args.fh = (caddr_t)&dir->dir_vnode.vn_fh; 
/* HP uses vfsmount instead! - pdb */
#ifdef hpux
                if (vfsmount(MOUNT_NFS, dir->dir_name, M_RDONLY, &args)) {
#else
#ifdef OLDMOUNT
#define MOUNT_NFS 1
		if (mount(MOUNT_NFS, dir->dir_name, M_RDONLY, &args)) {
#else
		if (mount("nfs", dir->dir_name, M_RDONLY|M_NEWTYPE, &args)) {
#endif /* end of if OLDMOUNT */
#endif /* end of if hpux */
			syslog(LOG_ERR, (catgets(catd,NL_SETN,314, "Can't mount %s: %m")), dir->dir_name);
			bad++;
                        break;   /*rb 4/1/92 */
		} else {
			domntent(pid, dir);
			bad = 0;
		}
	}
	if (bad)
		(void) kill(pid, SIGTERM);
	exit(bad);
	/*NOTREACHED*/
}

void
set_timeout(letter, t)
	char letter ; int t;
{
	if (t <= 1) {
		(void) fprintf(stderr, (catgets(catd,NL_SETN,315, "Bad timeout value\n")));
		usage();
	}
	switch (letter) {
	case 'm':
		mount_timeout = t;
		break;
	case 'l':
		max_link_time = t;
		break;
	case 'w':
		maxwait = t;
		break;
	default:
		(void) fprintf(stderr, (catgets(catd,NL_SETN,316, "automount: bad timeout switch\n")));
		usage();
		break;
	}
}

void
domntent(pid, dir)
	int pid;
	struct autodir *dir;
{
	FILE *f;
	struct mntent mnt;
	struct stat st;
	char fsname[64];
	char mntopts[100];
	char buf[16];

	f = setmntent(MOUNTED, "a");
	if (f == NULL) {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,317, "Can't update %s")), MOUNTED);
		return;
	}
	(void) sprintf(fsname, "%s:(pid%d)", self, pid);
	mnt.mnt_fsname = fsname;
	mnt.mnt_dir = dir->dir_name;
	mnt.mnt_type = MNTTYPE_IGNORE;
	(void) sprintf(mntopts, "ro,intr,port=%d,map=%s,%s", myport,
		dir->dir_map, 
		dir->dir_vnode.vn_type == VN_LINK ? "direct" : "indirect");
	if (dir->dir_vnode.vn_type == VN_DIR) {
		if (stat(dir->dir_name, &st) == 0) {
			(void) sprintf(buf, ",%s=%04x",
/* MNTINFO_DEV is the same as "dev" - pdb */
#ifdef hpux
				"dev", (st.st_dev & 0xFFFF));
#else
				MNTINFO_DEV, (st.st_dev & 0xFFFF));
#endif
			(void) strcat(mntopts, buf);
		}
	}
	mnt.mnt_opts = mntopts;
	mnt.mnt_freq = 0;
	mnt.mnt_passno = 0;	
	/* Sun does not have the following - pdb */
#ifdef hpux
	mnt.mnt_time = time((long *)0);
	mnt.mnt_cnode = 0;
#endif
	(void) addmntent(f, &mnt);	
	(void) endmntent(f);
}

void
catch()
{
	struct autodir *dir;
	int child;
	struct filsys *fs, *fs_next;
	struct stat stbuf;
	struct fattr *fa;
	/* HPNFS - bug fix */
	int still_busy = 0;  /* how many mount points are still busy */
	int log_time;        /* how long until next log message      */

	/*  HPNFS - bug fix
	 *
	 *  Disallow further SIGTERM signals while automount 
	 *  and it's child unmount all automount mount points
	 *  and NFS mounts made by automount.
	 */
	signal(SIGTERM, SIG_IGN);

	/*
	 *  The automounter has received a SIGTERM.
	 *  Here it forks a child to carry on servicing
	 *  its mount points in order that those
	 *  mount points can be unmounted.  The child
	 *  checks for symlink mount points and changes them
	 *  into directories to prevent the unmount system
	 *  call from following them.
	 */
	if ((child = fork()) == 0) {
		for (dir = HEAD(struct autodir, dir_q); dir;
		    dir = NEXT(struct autodir, dir)) {
			if (dir->dir_vnode.vn_type != VN_LINK)
				continue;

			dir->dir_vnode.vn_type = VN_DIR;
			fa = &dir->dir_vnode.vn_fattr;
			fa->type = NFDIR;
			fa->mode = NFSMODE_DIR + 0555;
		}
		return;
	}

	for (dir = HEAD(struct autodir, dir_q); dir;
	    dir = NEXT(struct autodir, dir)) {

		/*  This lstat is a kludge to force the kernel
		 *  to flush the attr cache.  If it was a direct
		 *  mount point (symlink) the kernel needs to
		 *  do a getattr to find out that it has changed
		 *  back into a directory.
		 */

		/*
		 * From thurlow@convex.com:
		 *
		 * The old code, a simple lstat, wasn't reliably convincing
		 * the kernel that the node had changed, so the kernel
		 * readlink requests would get ESTALE.  The kernel must
		 * eventually time out its attribute cache, so we'll wait
		 * until we know we can unmount properly.
		 */
		do {
			if (lstat(dir->dir_name, &stbuf) < 0) {
			syslog(LOG_ERR, (catgets(catd,NL_SETN,318, "lstat %s: %m")), dir->dir_name);
				break;
			}

			if ((stbuf.st_mode & S_IFMT) == S_IFLNK) {
				/*
				 * POSIX impliticly blocks signals in a handler
				 */
#ifdef __stdc__
				 sigunblock(sigmask(SIGALRM));
#endif
				 sleep(1);
				 continue;
			}
			else
				break;
		} while (1);

		dir->dir_busy = FALSE; /* HPNFS - bug fix */

                /* HP call is umount NOT unmount - pdb */
#ifdef hpux
		if (umount(dir->dir_name) < 0) {
#else
		if (unmount(dir->dir_name) < 0) {
#endif
			if (errno != EBUSY) {
				syslog(LOG_ERR, (catgets(catd,NL_SETN,319, "unmount %s: %m")), dir->dir_name);
			} else {
				/* HPNFS - bug fix */
				dir->dir_busy = TRUE;
				still_busy++;
			}

		} else {
			clean_mtab(dir->dir_name, 0);
			if (dir->dir_remove)
				(void) rmdir(dir->dir_name);
		}
	}

	/*  HPNFS - bug fix 
	 *         
	 *  All automount mount points must be unmounted before the
	 *  child process (currently the automount daemon) is killed.
	 *  If the mount points are left mounted with no automount
	 *  daemon, all processes that try to access them will hang.
	 *  Here we try (as many times as necessary) to unmount any 
	 *  mount points that failed to unmount earlier.
	 *
	 *  Try unmounting every 10 seconds, but only log every 60.
	 */
	log_time = 10;
	while (still_busy) {

		if (log_time == 0)
			log_time = 60;
		sleep(10);
		log_time = log_time - 10;

		for (dir = HEAD(struct autodir, dir_q); dir;
	    	     dir = NEXT(struct autodir, dir)) {

			if (!dir->dir_busy)
				continue;

			if (umount(dir->dir_name) < 0) {
				if (errno != EBUSY) {
					dir->dir_busy = FALSE;
					still_busy--;
					syslog(LOG_ERR, (catgets(catd,NL_SETN,319, "unmount %s: %m")), dir->dir_name);
				} else {
				     if (log_time != 0)
					continue;
				     syslog(LOG_ERR, (catgets(catd,NL_SETN,333, "unmount failed - processes still accessing %s - will retry")), dir->dir_name);
				}
			} else {
				dir->dir_busy = FALSE;
				still_busy--;
				clean_mtab(dir->dir_name, 0);
				if (dir->dir_remove)
					(void) rmdir(dir->dir_name);
			} /* else */

		} /* for */

	} /* while still_busy */

	(void) kill (child, SIGKILL);

	/*
	 *  Unmount any mounts done by the automounter
	 */
	for (fs = HEAD(struct filsys, fs_q); fs; fs = fs_next) {
		fs_next = NEXT(struct filsys, fs);
		if (fs->fs_mine && fs == fs->fs_rootfs) {
			if (do_unmount(fs))
				fs_next = HEAD(struct filsys, fs_q);
		}
	}

	syslog(LOG_ERR, (catgets(catd,NL_SETN,320, "exiting")));
	exit(0);
}

void
reap()
{

#ifndef hpux
	while (wait3((union wait *)0, WNOHANG, (struct rusage *)0) > 0)
		;
#else

/* HPNFS mod. by hv 6/15/93 - for DTS INDaa14769 */
      while (wait3((union wait *)0, WNOHANG, (struct rusage *)0) > 0)
      {
          havechild --;
      }

      signal(SIGCHLD, reap);  /* rb 5/93 - avoid ZOMBIEs */
#endif
}

/* HPNFS - Added to dynamically turn on/off trace 11/16/93 hv */
#ifdef hpux
void
toggle_trace()
{
    if (trace <= 0)
      trace = 2;
    else 
      trace = 0;
    signal(SIGUSR2,toggle_trace);
}
#endif

auto_run()
{
	int read_fds, n;
	time_t time();
	long last;
	struct timeval tv;

	last = time((long *)0);
	tv.tv_sec = maxwait;
	tv.tv_usec = 0;
	for (;;) {
		read_fds = svc_fds;
		n = select(32, &read_fds, (int *)0, (int *)0, &tv);
		time_now = time((long *)0);
		if (n)
			svc_getreq(read_fds);
		if (time_now >= last + maxwait) {
			last = time_now;
			do_timeouts();
		}
	}
}

usage()
{
	char *msg1[80];
	char *msg2[80];
	char *msg3[80];
	char *msg4[80];
	char *msg5[80];
	char *msg6[80];
	char *msg7[80];
	char *msg8[80];
	char *msg9[80];
	char *msg10[80];
	char *msg11[80];
	char *msg12[80];

	strcpy(msg1, catgets(catd,NL_SETN,321, "Usage: automount\n%s%s%s%s%s%s%s%s%s%s%s"));
	strcpy(msg2, catgets(catd,NL_SETN,322, "\t[-n]\t\t(no mounts)\n"));
	strcpy(msg3, catgets(catd,NL_SETN,323, "\t[-m]\t\t(ignore NIS auto.master)\n"));
	strcpy(msg4, catgets(catd,NL_SETN,324, "\t[-T]\t\t(trace nfs requests)\n"));
	strcpy(msg5, catgets(catd,NL_SETN,325, "\t[-v]\t\t(verbose error msgs)\n"));
	strcpy(msg6, catgets(catd,NL_SETN,326, "\t[-tl n]\t\t(mount duration)\n"));
	strcpy(msg7, catgets(catd,NL_SETN,327, "\t[-tm n]\t\t(attempt interval)\n"));
	strcpy(msg8, catgets(catd,NL_SETN,328, "\t[-tw n]\t\t(unmount interval)\n"));
	strcpy(msg9, catgets(catd,NL_SETN,329, "\t[-M dir]\t(temporary mount dir)\n"));
	strcpy(msg10, catgets(catd,NL_SETN,330, "\t[-D n=s]\t(define env variable)\n"));
	strcpy(msg11, catgets(catd,NL_SETN,331, "\t[-f file]\t(get mntpnts from file)\n"));
	strcpy(msg12, catgets(catd,NL_SETN,332, "\t[ dir map [-mntopts] ] ...\n"));

	fprintf(stderr, 
		msg1,
		msg2,
		msg3,
		msg4,
		msg5,
		msg6,
		msg7,
		msg8,
		msg9,
		msg10,
		msg11,
		msg12);
	exit(1);
}

void
loadmaster_yp(mapname)
	char *mapname;
{
	int first, err;
	char *key, *nkey, *val;
	int kl, nkl, vl;
	char dir[100], map[100];
	char *p, *opts;


	if (!yp)
		return;

	first = 1;
	key  = NULL; kl  = 0;
	nkey = NULL; nkl = 0;
	val  = NULL; vl  = 0;

	for (;;) {
		if (first) {
			first = 0;
			err = yp_first(mydomain, mapname, &nkey, &nkl, &val, &vl);
		} else {
			err = yp_next(mydomain, mapname, key, kl, &nkey, &nkl,
				&val, &vl);
		}
		if (err) {
			if (err != YPERR_NOMORE && err != YPERR_MAP)
				syslog(LOG_ERR, "%s: %s",
					mapname, yperr_string(err));
			return;
		}
		if (key)
			free(key);
		key = nkey;
		kl = nkl;

		if (kl >= 100 || vl >= 100)
			return;
		if (kl < 2 || vl < 1)
			return;
		if (isspace(*(u_char *)key) || *key == '#')
			return;
		(void) strncpy(dir, key, kl);
		dir[kl] = '\0';
		(void) strncpy(map, val, vl);
		map[vl] = '\0';
		p = map;
		while (*p && !isspace(*(u_char *)p))
			p++;
		opts = "rw";
		if (*p) {
			*p++ = '\0';
			while (*p && isspace(*(u_char *)p))
				p++;
			if (*p == '-')
				opts = p+1;
		}

		dirinit(dir, map, opts, 0);

		free(val);
	}
}

/* HPNFS 11/15/93 hv
 * Fig bug found in included flat file map
 * loadmaster_file returns -1 if fail; otherwise, return 0.
 */
#ifndef hpux
void
#endif
loadmaster_file(masterfile)
	char *masterfile;
{
	FILE *fp;
	char *line, *dir, *map, *opts;
	extern char *get_line();
	char linebuf[1024];

	if ((fp = fopen(masterfile, "r")) == NULL) {
		syslog(LOG_ERR, "%s:%m", masterfile);
		return (-1);  /* HPNFS */    
	}

	while ((line = get_line(fp, linebuf, sizeof linebuf)) != NULL) {
		dir = line;
		while (*dir && isspace(*(u_char *)dir)) dir++;
		if (*dir == '\0')
			continue;
		map = dir;
		while (*map && !isspace(*(u_char *)map)) map++;
		if (*map)
			*map++ = '\0';
		if (*dir == '+') 
                {
                    /* HPNFS 11/15/93 hv
                     * Fix bug found in included flat file map
                     * Also check included flat file map.
                     */
                    if (loadmaster_file(dir+1) < 0)
			loadmaster_yp(dir+1);
		} else {
			while (*map && isspace(*(u_char *)map)) map++;
			if (*map == '\0')
				continue;
			opts = map;
			while (*opts && !isspace(*(u_char *)opts)) opts++;
			if (*opts) {
				*opts++ = '\0';
				while (*opts && isspace(*(u_char *)opts)) opts++;
			}
			if (*opts != '-')
				opts = "-rw";
			
			dirinit(dir, map, opts+1, 0);
		}
	}
	(void) fclose(fp);
        return (0);  /* HPNFS */ 
}
