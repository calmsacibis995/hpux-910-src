#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.4.1.1 $";
#endif

/*
 * SYNOPSIS:
 * mount special mountdir [-r] [-f]
 * mount -a
 * Level:  HFS implementation
 * Origin: Sun
 * Modified to reflect HP release 5.0 and Spectrum release.
 *
 * mount returns 1 on error
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <ustat.h>
#include <stdio.h>
#include <sys/param.h>
#include <signal.h>
#include <mntent.h>
#include <checklist.h>
#include <fcntl.h>
#include <sys/fs.h>
#include <string.h>
#include <errno.h>


#include <rpc/rpc.h>
#include <time.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/mount.h>
#ifdef CDROM
#include <sys/vfs.h>
#include <sys/cdfsdir.h>
#include <sys/cdnode.h>
#include <sys/cdfs.h>
#endif /* CDROM */
#ifdef TRUX
#include <sys/security.h>
#include <sys/audit.h>
#endif
#ifdef QUOTA
#include <sys/quota.h>
#endif /* QUOTA */
#ifdef LOCAL_DISK
#include <cluster.h>
#endif /* LOCAL_DISK */
#include <unistd.h>
#include <sys/sysmacros.h>

char *flg[]={			/* used in printing mounted fs */
	"read/write",
	"read only"
	};

char *validopts[] = {		/* opts specified with "-o" must be in this
				    table. */
	MNTOPT_DEFAULTS,
	MNTOPT_RO,
	MNTOPT_RW,
	MNTOPT_SUID,
	MNTOPT_NOSUID,
	MNTOPT_NOAUTO,
	MNTOPT_BG,
	MNTOPT_FG,
	MNTOPT_RETRY,
	MNTOPT_RSIZE,
	MNTOPT_WSIZE,
	MNTOPT_TIMEO,
	MNTOPT_RETRANS,
	MNTOPT_PORT,
	MNTOPT_SOFT,
	MNTOPT_HARD,
	MNTOPT_INTR,
	MNTOPT_NOINTR,
#ifdef NFS3_2
	MNTOPT_DEVS,
	MNTOPT_NODEVS,
	MNTOPT_NOAC,
	MNTOPT_NOCTO,
	MNTOPT_ACTIMEO,
	MNTOPT_ACREGMIN,
	MNTOPT_ACREGMAX,
	MNTOPT_ACDIRMIN,
	MNTOPT_ACDIRMAX,
#endif /* NFS3_2 */
#ifdef QUOTA
        MNTOPT_QUOTA,
        MNTOPT_NOQUOTA,
#endif /* QUOTA */
#ifdef CDCASE
	MNTOPT_CDCASE,
#endif	
	NULL
	};

#if defined(LOCAL_DISK) && defined(SecureWare) && defined(B1)
#  define OPTIONS	"aflLprsuvA:S:o:t:"
#else
#  if !defined(LOCAL_DISK) && defined(SecureWare) && defined(B1)
#    define OPTIONS	"afprsuvA:S:o:t:"
#  else
#    if defined(LOCAL_DISK)
#      define OPTIONS 	"aflLprsuvo:t:"
#    else
#      define OPTIONS	"afprsuvo:t:"
#    endif
#  endif
#endif

int	ro = 0;        		/* ro = 0: mount read/write
			        	1: mount readonly     */
int	force = 0;		/* forcing mount even when fs is not clean */
int	freq = 1;
int	passno = 2;
int	all = 0;
int	verbose = 0;
int	fs_type = 0;
int	fs_options = 0;
int	fsname_and_dir = 0;	/* fsname and directory specified ? */
int	printed = 0;
int	fsclean_fd = 0;		/* global flag between mountfs and fsclean */
#if defined(GETMOUNT) || defined(LOCAL_DISK)
int	print = 0;
#endif /* GETMOUNT OR LOCAL_DISK */
#ifdef GETMOUNT
int	save_mnttab = 0;
int 	update = 0;
int	update_flag = 0;
#endif /* GETMOUNT */
#ifdef LOCAL_DISK
int	local_only = 0;		/* -l flag */
int	local_plus_nfs = 0;	/* -L flag */
#endif /* LOCAL_DISK */

#ifdef GETMOUNT
/*
 * For checking time stamp on /etc/mnttab, allow a four second lee way
 * for kernels within a cluster in which the kernel mount table time
 * stamps may be slightly out of sync.  Kernels should not be more than
 * one or two seconds out of sync.
 */
#define TIME_PAD 2
#endif /* GETMOUNT */

#define	NRETRY	1	/* number of times to retry a mount request */
#define	BGSLEEP	5	/* initial sleep time for background mount in seconds */
#define MAXSLEEP 120	/* max sleep time for background mount in seconds */
#define OPENRETRY 4	/* number of times to retry setmnt(/etc/mnttab) */
#define NFS_MNT_CNODEID 0 /* cnodeid() for nfs mounts */
/*
 * Fake errno for hfs and nfs file systems that are already mounted
 */
#define EMOUNTED	9999
/*
 * Fake errno for RPC failures that don't map to real errno's
 */
#define ERPC	(10000)

extern int errno;		/* return error code from system calls */
extern char *sys_errlist[];	/* for perror messages indexed by errno */

#if defined(SecureWare) && defined(B1)
char    auditbuf[80];
#endif

#ifdef LOCAL_DISK
#ifdef __STDC__
extern char *getcdf(char *, char *, int);	/* ansi c declaration */
#else /* not __STDC__ */
extern char *getcdf();				/* old c declaration */
#endif /* else not __STDC__ */
#endif /* LOCAL_DISK */
#ifdef GETMOUNT
extern int update_mnttab();	/* no params, so no special ansi c extern */
#endif /* GETMOUNT */

char	host[MNTMAXSTR];
char	name[MNTMAXSTR];	/* Name of file system */
char	dir[MNTMAXSTR];		/* directory to mount on */
char	type[MNTMAXSTR];	/* Type of file system (hfs,nfs,...) */
char	opts[MNTMAXSTR];	/* options */

/*
 * Structure used to build a mount tree.  The tree is traversed to do
 * the mounts and catch dependancies
 */
struct mnttree {
	struct mntent *mt_mnt;
	struct mnttree *mt_sib;
	struct mnttree *mt_kid;
};
struct mnttree *maketree();
struct mnttree *slink;
int do_slink = 0;
#if defined(SecureWare) && defined(B1)
FILE *mount_setmntent();
#endif /* B1 */

/* mnt_setmntent declaration; this is an internal routine that loops
 * until setmntent returns success.
 */
FILE *mnt_setmntent();

/*
 * the mount command has some items which, on the surface, appear to be poor
 * programming practices.  The fact that there appear to be lots of opens
 * with no regard to the corresponding closes is in evidence throughout
 * the command.  At one time, there were an equal number of opens and
 * closes, but this had to change to accomodate the autochanger.  It seems
 * that if a device is opens and closed multiple times, the autochanger
 * spends lots of time loading and unloading platters causing the command
 * take a *very* long time to execute.  Hence, there is an open of the
 * device at the beginning of main, and a close at the end.  This keeps
 * the autochanger open throughout the mount execution.  Unfortunately,
 * the s800 supports the concept of a mounted mag tape file system in a
 * read-only fashion.  That item alone is not unfortunate.  What makes it
 * unfortunate is that the tape driver does not support multiple opens
 * on the tape device.  The method chosen to resolve this issue is to
 * fstat the open device, pick off the major number and match it against
 * the tape driver major number.  If there is a match, the assumption is
 * that the device is a mag tape, and the device file will execute the correct
 * number of closes at the correct time.
 */

#ifdef __hp9000s800
int magtapefs;			/* global flag for mag tape file systems */
#endif /* __hp9000s800 */

main(argc, argv)
	int argc;
	char **argv;
{
	int sargc;
	char **sargv;
	int ret;
	struct mntent mnt;
	struct mntent *mntp;
	FILE *mnttab;
	char *colon;
	struct mnttree *mtree;
#ifdef __hp9000s800
	int isamagtape();
#endif /* __hp9000s800 */
#ifdef GETMOUNT
	struct stat	statbuf;
	time_t mnttab_time = 0;
	char *fs_cdf;
	char *dir_cdf;
	char *sargv_cdf;
#endif /* GETMOUNT */
#ifdef LOCAL_DISK
	int not_nfs;			/* flag */
	char *path;
#endif /* LOCAL_DISK */
#ifdef CDROM
	struct statfs	fsbuf;
	int 		fsdev_fd;
#endif /* CDROM */

	int c;
	extern char *optarg;
	extern int optind;
	extern int opterr;

#if defined(SecureWare) && defined(B1)
	if (ISB1)
		mount_init(argc, argv);
#endif /* B1 */
#if !defined(LOCAL_DISK) && !defined(GETMOUNT)
	/* mount with no argument prints mount table */
	if (argc == 1) {
		int i;

		for (i = 0; ((i < OPENRETRY) &&
		    ((mnttab = mnt_setmntent(MNT_MNTTAB, "r")) == NULL)); i++);

		if (mnttab == NULL) {
			fprintf(stderr, "mount: ");
			perror(MNT_MNTTAB);
			exit(1);
		}
		while ((mntp = getmntent(mnttab)) != NULL) {
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
		            (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ||
		            (strcmp(mntp->mnt_type, MNTTYPE_SWAPFS) == 0)) {
				continue;
			}
			printent(mntp);
		}
		endmntent(mnttab);
		exit(0);
	}
#endif /* not LOCAL_DISK and not GETMOUNT */

	opts[0] = '\0';
	type[0] = '\0';

	/*
	 *   For backwards compatibility with earlier mount commands,
	 *   accepted the syntax:
	 *
	 *   		mount special directory [options]
	 */

	sargv = argv;
	sargc = 0;
	while ((sargc < argc) && (argv[1] != NULL) && (argv[1][0] != '-')) {
		++argv; ++sargc;
	}

	if ( sargc > 2 )
		usage();
#ifdef GETMOUNT
	if (sargc)
	{
	    update++;	/* set flag to update mnttab */
	    fsname_and_dir++;
	}
#endif /* GETMOUNT */

	/*
	 * Set options
	 */

	opterr = 1;
	while ((c = getopt(argc, argv, OPTIONS)) != EOF) {
		switch (c) {
		case 'a':
			if (print | ro | local_only | local_plus_nfs |
			            fs_options | fsname_and_dir)
			    usage();
			all++;
#ifdef GETMOUNT
			update++;
#endif /* GETMOUNT */
			break;
		case 'f':
			if (print)
			    usage();
			force++;
			break;
#ifdef LOCAL_DISK
		case 'l':
			if (local_plus_nfs) {
			    (void) fprintf(stderr,
			    "mount: conflicting options: -l, -L\n");
			    usage();
			}
			if (all | fsname_and_dir | force | verbose | fs_type |
				  fs_options | ro)
			    usage();
			local_only++;
			break;
		case 'L':
			local_plus_nfs++;
			if (local_only) {
			    (void) fprintf(stderr,
			    "mount: conflicting options: -l, -L\n");
			    usage();
			}
			if (all | fsname_and_dir | force | verbose | fs_type |
				  fs_options | ro)
			    usage();
			break;
#endif /* LOCAL_DISK */
		case 'o':
		/* ?????????? */
			if (all | print | local_only | local_plus_nfs)
			    usage();
			fs_options++;
			strcpy(opts, optarg);
			if (checkopts(opts) == 0)
				usage();
			break;
		case 'p':
#if !defined(GETMOUNT) && !defined(LOCAL_DISK)
		/* ?????????? */
			if (all | force | ro | fs_type | verbose | fs_options |
				  fsname_and_dir)
			    usage();
			printmtab(stdout);
			exit(0);
#else /* not GETMOUNT and not LOCAL_DISK */
			if (all | force | ro | fs_type | verbose | fs_options |
				  fsname_and_dir)
			    usage();
			print++;
			break;
#endif /* not GETMOUNT and not LOCAL_DISK */
		case 'r':
			if (all | print | local_only | local_plus_nfs)
			    usage();
			ro++;
			strcpy(opts, "ro");
			if (checkopts(opts) == 0)
			    usage();
			break;
#ifdef GETMOUNT
		case 's':
			save_mnttab++;
			break;
#endif /* GETMOUNT */
		case 't':
			/* ?????????????? */
			if (print  | local_only | local_plus_nfs)
			    usage();
			strcpy(type, optarg);
			if ((strcmp(type, MNTTYPE_HFS) == 0) ||
			    (strcmp(type, MNTTYPE_NFS) == 0)  ||
			    (strcmp(type, MNTTYPE_CDFS) == 0))
			{
			    fs_type++;
			}
			else
			{
		            (void) fprintf(stderr,
			      "mount: unknown file system type %s\n",
			       type);
			    usage();
			}
			break;
#ifdef GETMOUNT
		case 'u':
			update++;
			update_flag++;
			break;
#endif /* GETMOUNT */
		case 'v':
			if (print | local_only | local_plus_nfs)
			    usage();
			verbose++;
			break;
#if defined(SecureWare) && defined(B1)
#ifdef TRUX
		case 'A':
		case 'S':
			if (ISB1)
				mount_specflags(c,optarg);
			else
				usage();
			break;
#endif TRUX
#endif /* B1 */
		default:
			/*
			fprintf(stderr, "mount: unknown option: %c\n",
					argv[optind - 1][optchar]);
			 */
			/*
			 * OLD error message; now let getopt take care
			 * of it.
			 */
			usage();
		}
	}
#if defined(LOCAL_DISK) || defined(GETMOUNT)
#ifdef GETMOUNT
	/*
	 * if argv[optind] is not NULL (meaning that we have arguments that
	 *  haven't been processed yet), then we're
	 *  of the format:
	 *
	 *  mount <options> <dir>, or
	 *  mount <options> <spec>, or
	 *  mount <options> <spec> <dir>
	 */

	if (sargc > 0 && argv[optind] != NULL) /* we can't have em both */
	{
		usage();
	}
	else if (sargc <= 0)
	{
		sargv = &argv[optind - 1]; /* sargv[0] is not used */
		sargc = argc - optind;

	}

	if (sargc > 2)
		usage();

	if (sargc)
	{
	    	update++;	/* set flag to update mnttab */
	}

        /* if save_mnttab, don't update */
	if (save_mnttab)
	{
	    if (update_flag)
	    {
		(void) fprintf(stderr,"mount: conflicting options: -s, -u\n");
		usage();
	    }
	    update = 0;
	}

	/* get kernel last mount change time */
	(void)getmount_cnt(&mnttab_time);
	/*
	 * Do the update if
	 *     update_flag was set, or
	 *     stat of mnttab fails, or
	 *	   mnttab file is out of date (more than 2*TIME_PAD seconds)
	 */
	if (update_flag || (stat(MNT_MNTTAB, &statbuf) != 0) 	||
	    ((mnttab_time < statbuf.st_mtime - TIME_PAD) 	||
		(mnttab_time > statbuf.st_mtime + TIME_PAD)))
	{
	    if (update)		/* update on "write" operations only */
	    {
		/* update mnttab & reset its times */
#ifdef TRUX
		if (ISB1) {
			forcepriv(SEC_ALLOWDACACCESS);
			forcepriv(SEC_ALLOWMACACCESS);
		}
#endif TRUX
		if (update_mnttab())
		{
		    /* error */
		    fprintf(stderr, "mount: ");
		    perror(MNT_MNTTAB);
		    /* clear flag to indicate failure */
		    update = 0;
		}
#ifdef TRUX
		if (ISB1) {
			disablepriv(SEC_ALLOWDACACCESS);
			disablepriv(SEC_ALLOWMACACCESS);
		}
#endif TRUX
	    }
	    /*
	     * If update is not set, either mnttab is out of date, and
	     * we can't write to it (because this is not a write operation
	     * or because we're not root), or else update_mnttab() failed.
	     */
	    if (!update)
	    {
		if (update_flag)
		{
		    /*
		     * -u option was specified, but we couldn't update
		     * so bail right now, give message if we aren't root
		     */
		    fprintf(stderr, "mount:  Unable to update /etc/mnttab\n");
		    if (!(geteuid() == 0))
			fprintf(stderr, "Must be root to use mount -u\n");
		    exit(1);
		}
		else
		{
		    /*
		     * It's still out of date, but can't do anything about it
		     */
		    fprintf(stderr,"mount: /etc/mnttab is out of date with kernel data structures.\n");
		    fprintf(stderr,"Update with mount -u.\n");
		}
	    }
	}
#endif /* GETMOUNT */
	/*
	 * mount with no arguments (-u and -s are considered to be the same
	 * as choosing no arguments for displaying purposes), or with -l | -L
	 * prints mount table
	 */
	if ((argc == 1) || (argc == 2 && (update_flag || save_mnttab)) ||
	    ((local_only || local_plus_nfs) && !print))
	{
	    int i;

	    for (i = 0; ((i < OPENRETRY) &&
		((mnttab = mnt_setmntent(MNT_MNTTAB, "r")) == NULL)); i++);

	    if (mnttab == NULL) {
		    fprintf(stderr, "mount: ");
		    perror(MNT_MNTTAB);
		    exit(1);
	    }
	    while ((mntp = getmntent(mnttab)) != NULL)
	    {
		if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
	            (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0)   ||
	            (strcmp(mntp->mnt_type, MNTTYPE_SWAPFS) == 0)) {
			continue;
		}
#ifdef LOCAL_DISK
		/*
		 * Check for -l or -L flags:  Print entry
		 *  - If neither -l nor -L was specified, OR
		 *  - If -l was specified and the mount is
		 *	 on my cnode and it's NOT NFS, OR
		 *  - If -L was specified and it is NFS
		 */
		if ((!local_only && !local_plus_nfs) 			||
		    ((not_nfs = strcmp(mntp->mnt_type, MNTTYPE_NFS)) &&
			    mntp->mnt_cnode == cnodeid())  		||
		    (local_plus_nfs && !not_nfs))
		{
#endif /* LOCAL_DISK */
		    printent(mntp);
#ifdef LOCAL_DISK
		}
#endif /* LOCAL_DISK */
	    }
	    endmntent(mnttab);
	    exit(0);
	}

	if (print)
	{
#ifdef LOCAL_DISK
	    /*
	     * print option (-p) must appear alone, or with either
	     * local_only (-l) or local_plus_nfs (-L), but not with both
	     */
#endif /* LOCAL_DISK */
	    printmtab(stdout);
	    exit(0);
	}

	/* all proper uses of -l and -L options exit prior to this point */
	/* Note sure why this is here, but I'll leave it for now.
	 *	raf - 6/19/90
	 */
	if (local_only)
	    (void) fprintf(stderr,"mount: incorrect use of -l option\n");
	if (local_plus_nfs)
	    (void) fprintf(stderr,"mount: incorrect use of -L option\n");
#endif /* LOCAL_DISK or GETMOUNT */

#ifdef SecureWare
	if ((!ISSECURE) && (geteuid() != 0)) {
		fprintf(stderr, "Must be root to use mount\n");
		exit(1);
	}
#else
	if (geteuid() != 0) {
		fprintf(stderr, "Must be root to use mount\n");
		exit(1);
	}
#endif /* SecureWare */

	(void) signal(SIGHUP, SIG_IGN);

	/* mount -a  */
	if (all) {
		mnttab = mnt_setmntent(MNT_CHECKLIST, "r");
		if (mnttab == NULL) {
			fprintf(stderr, "mount: ");
			perror(MNT_CHECKLIST);
			exit(1);
		}
		mtree = NULL;
		while ((mntp = getmntent(mnttab)) != NULL) {

			/*
			** Skip "ignore", "swap", and "noauto" entries.
			** Also skip "/".
			*/

			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAPFS) == 0) ||
			    hasmntopt(mntp, MNTOPT_NOAUTO) ||
			    (strcmp(mntp->mnt_dir, "/") == 0) ) {
				continue;
			}
			if (type[0] != '\0' &&
			    strcmp(mntp->mnt_type, type) != 0) {
				continue;
			}

			/*
			** Check that all required fields exist.
			*/

			if ( mntp->mnt_dir[0] == '\0'  ||
			     mntp->mnt_type[0] == '\0' ||
			     mntp->mnt_opts[0] == '\0' ||
			     mntp->mnt_freq == -1 ||
			     mntp->mnt_passno == -1 ) {
				fprintf(stderr, "mount: entry for %s has missing fields in %s\n", mntp->mnt_fsname, MNT_CHECKLIST);
				exit(1);
			}
#ifdef LOCAL_DISK
			/*
			** Add cnode field to mntent structure
			*/
			if (strcmp(mntp->mnt_type, MNTTYPE_NFS) == 0)
				mntp->mnt_cnode = NFS_MNT_CNODEID;
			else
				mntp->mnt_cnode = cnodeid();

			/*
			** Change dir and fsname into cdfs
			*/
			if (dir_cdf = getcdf(mntp->mnt_dir, NULL, MNTMAXSTR-1))
				mntp->mnt_dir = dir_cdf;

			if (fs_cdf = getcdf(mntp->mnt_fsname, NULL, MNTMAXSTR-1))
				mntp->mnt_fsname = fs_cdf;
#endif /* LOCAL_DISK*/
			mtree = maketree(mtree, mntp);
		}
		endmntent(mnttab);
		ret = mounttree(mtree);
		do_slink = 1;
		ret += mounttree(slink);
#if defined(SecureWare) && defined(B1)
                if (ISB1 && ret) {
                        sprintf(auditbuf, "mount: mounttree returned code: %d",
                               ret);
                        audit_subsystem(auditbuf, "'mount' aborted",
                                ET_SUBSYSTEM);
                }
#endif
		exit(ret);
	}

	/*
	 * Command looks like: mount <fsname>|<dir>
	 * we walk through /etc/checklist til we match either fsname or dir.
	 */
	if (sargc == 1) {
		mnttab = mnt_setmntent(MNT_CHECKLIST, "r");
		if (mnttab == NULL) {
			fprintf(stderr, "mount: ");
			perror(MNT_CHECKLIST);
			exit(1);
		}
		while ((mntp = getmntent(mnttab)) != NULL) {
			if ((strcmp(mntp->mnt_type, MNTTYPE_IGNORE) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAP) == 0) ||
			    (strcmp(mntp->mnt_type, MNTTYPE_SWAPFS) == 0)) {
				continue;
			}
			fs_cdf = getcdf(mntp->mnt_fsname, NULL, MNTMAXSTR - 1);
			dir_cdf = getcdf(mntp->mnt_dir, NULL, MNTMAXSTR - 1);
			sargv_cdf = getcdf(sargv[1], NULL, MNTMAXSTR - 1);
			/*
			 * If all pointers are non-null, then they point to
			 * valid paths (expanded to cdf's); otherwise, just
			 * compare the original paths like we used to.
			 */
			if (fs_cdf && dir_cdf && sargv_cdf) {
			    ; /* do nothing */
			}
			else {
			    if (sargv_cdf)
			        free(sargv_cdf);
			    if (fs_cdf)
			        free(fs_cdf);
			    if (dir_cdf)
			        free(dir_cdf);
			    fs_cdf = mntp->mnt_fsname;
			    dir_cdf = mntp->mnt_dir;
			    sargv_cdf = sargv[1];
			}
			if ((strcmp(fs_cdf, sargv_cdf)== 0) ||
			    (strcmp(dir_cdf, sargv_cdf) == 0)) {
#ifdef LOCAL_DISK
				/*
				** Add cnode field to mntent structure
				*/
				if (strcmp(mntp->mnt_type, MNTTYPE_NFS) == 0)
					mntp->mnt_cnode = NFS_MNT_CNODEID;
				else
					mntp->mnt_cnode = cnodeid();

				/*
				** Change dir and fsname into cdfs
				*/
				mntp->mnt_dir = dir_cdf;
				mntp->mnt_fsname = fs_cdf;
#endif /* LOCAL_DISK*/
				if (opts[0] != '\0') {
					char *topts;
					/*
					 * override checklist with command line
					 * options
					 */
					topts = mntp->mnt_opts;
					mntp->mnt_opts = opts;
					ret = mounttree(maketree(NULL, mntp));
					mntp->mnt_opts = topts;
				} else {
					ret = mounttree(maketree(NULL, mntp));
				}
#if defined(SecureWare) && defined(B1)
                                if (ISB1 && ret) {
                                        sprintf(auditbuf,
				 "mount: mounttree returned code: %d", ret);
                                        audit_subsystem(auditbuf,
				 "'mount' aborted", ET_SUBSYSTEM);
                                }
#endif
				exit(ret);
			}
		}
		fprintf(stderr, "mount: %s not found in %s\n", sargv[1], MNT_CHECKLIST);
		exit(1);
	}

	if (sargc != 2) {
		usage();
	}
#if defined(LOCAL_DISK)
	/* expand dir and name into full CDFness */
	if ((getcdf(sargv[2], dir, MNTMAXSTR-1)) == NULL)
	    strncpy(dir, sargv[2], MNTMAXSTR-1);
	dir[MNTMAXSTR-1] = '\0';
	if ((getcdf(sargv[1], name, MNTMAXSTR-1)) == NULL)
	    strncpy(name, sargv[1], MNTMAXSTR-1);
	name[MNTMAXSTR-1] = '\0';
#else /* LOCAL_DISK */
	strcpy(dir, sargv[2]);
	strcpy(name, sargv[1]);
#endif /* LOCAL_DISK */

	/* Set the file system type */

	/*
	 * Check for file system names of the form
	 *     host:path
	 * make these type nfs
	 */

	colon = strchr(name, ':');
	if (colon) {
		if (type[0] != '\0' && strcmp(type, MNTTYPE_NFS) != 0) {
			fprintf(stderr,"%s: %s; must use type nfs\n",
			    "mount: remote file system", name);
			usage();
		}
		strcpy(type, MNTTYPE_NFS);
	}

#ifdef CDROM
	if (type[0] == '\0')
	{
	        struct stat sbuf;
		if ((fsdev_fd = open(name, O_RDONLY|O_NDELAY)) < 0) {
			perror(name);
			exit(1);
		}
		if (fstat(fsdev_fd, &sbuf) < 0) {
		    perror(name);
		    exit(1);
		}
		if ((sbuf.st_mode & S_IFMT) != S_IFBLK) {
		    fprintf(stderr, "%s: must be a block device\n",name);
		    usage();
		}
		if (fstatfsdev(fsdev_fd,&fsbuf) < 0) {
		        if (errno == EINVAL) {
			    fprintf(stderr, "%s: unrecognized file system\n",
				    name);
			}
			else {
			    perror(name);
			}
			exit(1);
		}
#ifdef __hp9000s800
		if (sysconf(_SC_IO_TYPE) == IO_TYPE_SIO) {
			magtapefs = isamagtape(fsdev_fd);
			if (magtapefs)
				close(fsdev_fd);
		} else {                /* _WSIO ==> s700 for now */
			magtapefs = 0;
		}
#endif /* __hp9000s800 */
		switch (fsbuf.f_fsid[1]) {
		case MOUNT_CDFS:
			strcpy(type,MNTTYPE_CDFS);
			break;
		case MOUNT_UFS:
			strcpy(type,MNTTYPE_HFS);
			break;
		default:
			fprintf(stderr,"%s: invalid file system type\n");
			exit(1);
		}
	}
#else /* CDROM */
	if (type[0] == '\0')
	{
		strcpy(type, MNTTYPE_HFS);	/* default type = HFS */
	}
#endif /* CDROM */


	if (strcmp(type, MNTTYPE_NFS) == 0) {
		passno = 0;
		freq = 0;
	}

	mnt.mnt_fsname = name;
	mnt.mnt_dir = dir;
	mnt.mnt_type = type;
	mnt.mnt_opts = opts;
	mnt.mnt_freq = freq;
	mnt.mnt_passno = passno;
#ifdef LOCAL_DISK
	/*
	** Add cnode field to mntent structure
	*/
	if (strcmp(mnt.mnt_type, MNTTYPE_NFS) == 0)
		mnt.mnt_cnode = NFS_MNT_CNODEID;
	else
		mnt.mnt_cnode = cnodeid();

#endif /* LOCAL_DISK */

	/*
	** Set either "rw" or "ro".  Have to do this after setting
	** up mnt struct because of hasmntopt().
	*/

	if ( mnt.mnt_opts[0] == '\0' ) {
		strcpy(mnt.mnt_opts, ro ? MNTOPT_RO : MNTOPT_DEFAULTS);
	}
	else if ( ro && (hasmntopt(&mnt, MNTOPT_RO) == 0) ) {
		strcat(mnt.mnt_opts, ",");
		strcat(mnt.mnt_opts, MNTOPT_RO);
	}
#if defined(SecureWare) && defined(B1)
	if (ISB1)
		mount_setoptions(mnt.mnt_opts);
#endif /* B1 */

	ret = mounttree(maketree(NULL, &mnt));

#if defined(SecureWare) && defined(B1)
        if (ISB1 && ret) {
                sprintf(auditbuf, "mount: mounttree returned code: %d",
                        ret);
                audit_subsystem(auditbuf, "'mount' aborted",
                        ET_SUBSYSTEM);
                }
#endif

#ifdef CDROM
	close(fsdev_fd);
#endif /* CDROM */

	return(ret);
}


/*
	Update the timestamp on the mnttab file so that it
	matches the kernel timestamp returned by getmount_cnt().
	This is done for "mount -a" and "mount fs dir" invocations.

	NOTE: if the -s option was specified, we do not want to
	set the timestamp.  Otherwise, a latter mount invocation
	will think that mnttab is already up to date.
*/
void
update_mnttab_time()
{
	time_t	mnttab_time;
	struct  utimbuf mnt_times;

	if (!save_mnttab) {
		(void)getmount_cnt(&mnttab_time);
		mnt_times.actime = mnttab_time;
		mnt_times.modtime = mnttab_time;
		(void)utime(MNT_MNTTAB,&mnt_times);
	}
}


int
checkopts(o)
	char *o;
{
	int result = 1;
	int comma = 0;
	int optlen;
	int i;
	char buf[MNTMAXSTR];
	char *optr;
	char *nptr;
	char *optlist;

	nptr = buf;
	*nptr = '\0';

	for (optr = strtok(o, ","); *optr != '\0'; optr = strtok(NULL, ","))
	{
	    for (optlist = validopts[0], i = 0; optlist != NULL;
		optlist = validopts[i], i++)
	    {
		optlen = strlen(optlist);
		if ((strncmp(optr, optlist, optlen) == 0) &&
		    ((optr[optlen] == '\0') || (optr[optlen] == '=')))
			break;
	    }

	    if (optlist == NULL)
	    {
		fprintf(stderr, "mount: illegal option \"%s\"\n", optr);
		result = 0;
	    }
	    else
	    {
		if (comma == 0)
		    comma++;
		else
		    strcat(nptr,",");

		strcat(nptr, optr);
	    }
	}

	strcpy(o, buf);

	return(result);
}


int
mountfs(print, mnt)
	int print;
	struct mntent *mnt;
{
	int error;
	extern int errno;
	int t = -1;
	int flags = 0;
	union data {
		struct ufs_args	ufs_args;
		struct nfs_args nfs_args;
#ifdef CDROM
                struct cdfs_args cdfs_args;
#endif /* CDROM */
	} data;
	struct stat statval;
#ifdef LOCAL_DISK
	struct cct_entry *cct_ent;
	int nfs_mount;
#endif /* LOCAL_DISK */
#ifdef CDCASE
	int fd;
#endif
	

	void addtomtab();

	if (mnt->mnt_dir[0] != '/') {
		fprintf(stderr, "mount: precede argument with / such as: /%s for mounting %s\n", mnt->mnt_dir, mnt->mnt_fsname);
		return(1);
	}

	/* check the directory */
	if (stat(mnt->mnt_dir, &statval)) {
                fprintf(stderr, "mount: ");
		perror(mnt->mnt_dir);
		return(1);
#ifdef LOCAL_DISK
	}
	else
	{
		if (!(statval.st_mode & S_IFDIR)) {
		    fprintf(stderr, "mount: %s must be a directory\n", mnt->mnt_dir);
		    return(1);
		}
		/*
		 * Ensure that mount dir is either local or is on rootserver
		 * to enforce restrictions against distributed cascaded mounts
		 *
		 * Mount is LEGAL if:
		 *  - mount pt cnodeid == my cnodeid && new mount is NOT NFS,
		 *	or
		 *  - mount point cnode id == 0 (NFS) && new mount is NFS,
		 *	or
		 *  - mount point cnode id == root server's cnode id
		 */
		nfs_mount = !(strcmp(mnt->mnt_type, MNTTYPE_NFS));
		if (!((statval.st_cnode == cnodeid() && !nfs_mount)
			|| (statval.st_cnode == 0 &&
				!strcmp(mnt->mnt_type, MNTTYPE_NFS)))) {
		    /*
		     * If it's not local, get the cnode id of the rootserver
		     */
		    setccent();
		    while (cct_ent = getccent()) {
			if (cct_ent->cnode_type == 'r')
				break; 	/* found rootserver */
		    }
		    /*
		     * If we found a root server entry and it doesn't
		     * match, or if we didn't find a rootserver entry and
		     * we're not a localroot (rootserver or standalone)
		     * then complain about a distributed cascaded mount!
		     */
		    if (cct_ent && (statval.st_cnode != cct_ent->cnode_id) ||
						(!cct_ent && !localroot())) {
			if (!nfs_mount)
			    fprintf(stderr, "mount: directory %s must be local or must be on the root server\n", mnt->mnt_dir);
			else
			    fprintf(stderr, "mount: directory %s must be on the root server or must be NFS\n", mnt->mnt_dir);
			return(1);
		    }
		}
#else /* LOCAL_DISK */
	} else if (!(statval.st_mode & S_IFDIR)) {
		fprintf(stderr, "mount: %s must be a directory\n", mnt->mnt_dir);
		return(1);
#endif /* LOCAL_DISK */
	}
	if (strcmp(mnt->mnt_type, MNTTYPE_HFS) == 0) {
		t = MOUNT_UFS;
		error = mount_hfs(mnt, &data.ufs_args);
	} else if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0) {
		if (mounted(mnt)) {
			if (print) {
				fprintf(stderr, "mount: %s already mounted\n",
			    	mnt->mnt_fsname);
			}
			return (EMOUNTED);
		}
		t = MOUNT_NFS;
		error = mount_nfs(mnt, &data.nfs_args);
#ifdef CDROM
        } else  if (strcmp(mnt->mnt_type, MNTTYPE_CDFS) == 0) {
		if (mounted(mnt)) {
			if (print) {
				fprintf(stderr, "file system for device %s is already mounted\n",
			    	mnt->mnt_fsname);
			}
			return (EMOUNTED);
		}
#ifdef LOCAL_DISK
		/*
		 * Don't allow non-rootserver cnodes to mount CDFS
		 * because kernel crash recover code is not in place
		 */
		if (!localroot()) {
			fprintf(stderr, "mount: CD file systems may only be mounted on the cluster rootserver cnode\n");
			return(1);
		}
#endif /* LOCAL_DISK */
                t = MOUNT_CDFS;
                error = mount_cdfs(mnt, &data.cdfs_args);
#endif /* CDROM */
	} else {
		fprintf(stderr,
		    "mount: unknown file system type %s\n",
		    mnt->mnt_type);
		error = EINVAL;
	}

	if (error) {
		return (error);
	}

	flags |= (hasmntopt(mnt, MNTOPT_RO) == NULL) ? 0 : M_RDONLY;
	flags |= (hasmntopt(mnt, MNTOPT_NOSUID) == NULL) ? 0 : M_NOSUID;
#ifdef QUOTA
	flags |= (hasmntopt(mnt, MNTOPT_QUOTA) == NULL) ? 0 : M_QUOTA;
#endif /* QUOTA */

#if defined(SecureWare) && defined(B1)
	if (ISB1){
	    mount_setattrs(mnt);
	    mount_privilege("mount", strcmp(mnt->mnt_type,MNTTYPE_NFS) == 0);
	}
	if ((ISB1) ? (mount_fsmount(t, mnt->mnt_dir, flags, &data) < 0) :
	             (vfsmount(t, mnt->mnt_dir, flags, &data) < 0))
#else
	if (vfsmount(t, mnt->mnt_dir, flags, &data) < 0)
#endif /* B1 */
	{
		if (print) {
		        switch(errno) {
			    case ENODEV:
				fprintf(stderr, "mount: ");
				switch(t) {
				case MOUNT_CDFS:
					fprintf(stderr, "CDFS");
					break;
				case MOUNT_NFS:
					fprintf(stderr, "NFS");
					break;
				}
				fprintf(stderr, " not configured\n");
				break;
			    default:
				fprintf(stderr, "mount: %s on ", mnt->mnt_fsname);
				perror(mnt->mnt_dir);
				break;
			}
		}
		return (errno);
	}
#ifdef QUOTA
/* make sure the quotas got enabled */
	if ((flags & M_QUOTA) &&
		(quotactl(Q_SYNC, data.ufs_args.fspec, 0, 0) == -1)) {
	    fprintf(stderr,
		    "mount: WARNING: could not enable quotas on %s (%s):\n\t",
		    mnt->mnt_dir, data.ufs_args.fspec);
	    switch (errno) {
	    case ENOSYS:
		fputs("kernel not configured for quotas",stderr);
		break;
	    case ENODEV:
		fputs("not an HFS file system",stderr);
		break;
	    case ESRCH:
		fputs("check quotas configuration",stderr);
		break;
	    default:
		fputs(sys_errlist[errno], stderr);
	    }
	    fprintf(stderr, "\n\t%s incorrectly indicates quotas enabled!",
		    MNT_MNTTAB);
#ifdef GETMOUNT
	    fputs("  Run \"mount -u\" to correct.\n", stderr);
#else /* GETMOUNT */
	    fputs("\n", stderr);
#endif /* GETMOUNT */
	}
#endif /* QUOTA */

#ifdef CDCASE
	if (hasmntopt(mnt, MNTOPT_CDCASE)) {
	    fd = open(mnt->mnt_dir, O_RDONLY);
	    if (fd > 0) {
		error = fsctl(fd, CDFS_CONV_CASE, NULL, 0);
		    if (error == 0)		
			error = fsctl(fd, CDFS_ZAP_VERS, NULL, 0);
		close(fd);
    	    }
	    if (fd < 0 || error)
	    	fprintf(stderr, "mount: could not enable case conversion on %s\n", 
	    		mnt->mnt_dir);
	}
#endif	

	addtomtab(mnt);		/* Update /etc/mnttab */
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
        	sprintf(auditbuf, "successful mount of %s on %s",
			 mnt->mnt_fsname, mnt->mnt_dir);
        	audit_subsystem(auditbuf, "mount successful",
		 ET_OBJECT_AVAIL);
	}
#endif
	if (t == MOUNT_UFS)
		close(fsclean_fd);

	if (verbose) {
		fprintf(stdout, "%s mounted on %s%s\n",
		    mnt->mnt_fsname, mnt->mnt_dir,
#ifdef QUOTA
		    flags & M_QUOTA ? " (with quotas)" : ""
#else
		    ""
#endif /* QUOTA */
		    );
	}
	return (0);
}

int
mount_hfs(mnt, args)
	struct mntent *mnt;
	struct ufs_args *args;
{
	static char fsname[MNTMAXSTR];
	int Ret;

	/* return <> 0 denotes error encounter in fsclean */

#ifdef __hp9000s800
	if (magtapefs)
	{
	    if (hasmntopt(mnt,MNTOPT_RW) && hasmntopt(mnt,MNTOPT_RO))
	    {
		(void) fprintf(stderr,"mount: conflicting options: ro,rw\n");
		return(1);
	    }
	    else if (strcmp(mnt->mnt_opts,MNTOPT_DEFAULTS) == 0)
	    {
		strcpy(mnt->mnt_opts,MNTOPT_RO);
	    }
	    else if (!hasmntopt(mnt,MNTOPT_RW) && !hasmntopt(mnt,MNTOPT_RO))
	    {
		strcpy(opts,mnt->mnt_opts);
		mnt->mnt_opts = opts;
		strcat(mnt->mnt_opts,",");
		strcat(mnt->mnt_opts,MNTOPT_RO);
	    }
	}
#endif /* __hp9000s800 */

	if ((Ret = fsclean(mnt->mnt_fsname)) && !force )
		return(Ret);
#ifdef QFS_CMDS
	if (Ret == 2)
		return(1);
#endif QFS_CMDS

	strcpy(fsname, mnt->mnt_fsname);
	args->fspec = fsname;
	return (0);
}

#ifdef CDROM
int
mount_cdfs(mnt, args)
	struct mntent *mnt;
	struct cdfs_args *args;
{
	static char	fsname[MNTMAXSTR];
	struct statfs	fsbuf;
        struct stat sbuf;

	if (hasmntopt(mnt,MNTOPT_RW) && hasmntopt(mnt,MNTOPT_RO)) {
		(void) fprintf(stderr,"mount: conflicting options: ro,rw\n");
		return(1);
#ifdef TRUX
			/* B1 may have more options */
	} else if (strncmp(mnt->mnt_opts,MNTOPT_DEFAULTS,strlen(MNTOPT_DEFAULTS)) == 0){
		strcpy(mnt->mnt_opts,MNTOPT_RO);
				/* re-copy B1 options */
        	if (ISB1)
                	mount_setoptions(mnt->mnt_opts);

#else
	} else if (strcmp(mnt->mnt_opts,MNTOPT_DEFAULTS) == 0) {
		strcpy(mnt->mnt_opts,MNTOPT_RO);
#endif
	} else if (!hasmntopt(mnt,MNTOPT_RW) && !hasmntopt(mnt,MNTOPT_RO)) {
		strcpy(opts,mnt->mnt_opts);
		mnt->mnt_opts = opts;
		strcat(mnt->mnt_opts,",");
		strcat(mnt->mnt_opts,MNTOPT_RO);
	}
	if (stat(mnt->mnt_fsname, &sbuf) < 0) {
	    perror(name);
	    exit(1);
	}
	if ((sbuf.st_mode & S_IFMT) != S_IFBLK) {
	    fprintf(stderr, "%s: must be a block device\n",name);
	    usage();
	}
	if (statfsdev(mnt->mnt_fsname,&fsbuf) < 0) {
	    if (errno == EINVAL) {
		fprintf(stderr, "%s: unrecognized file system\n",
			name);
	    }
	    else
            {
		perror(mnt->mnt_fsname);
	    }
	    exit(1);
	}
	if (fsbuf.f_fsid[1] != MOUNT_CDFS) {
		(void) fprintf(stderr, "mount: bad \"standard id\" on %s\n",
				mnt->mnt_fsname);
		return(1);
	}

	strcpy(fsname, mnt->mnt_fsname);
	args->fspec = fsname;
	return (0);
}
#endif /* CDROM */

int
mount_nfs(mnt, args)
	struct mntent *mnt;
	struct nfs_args *args;
{
	static struct sockaddr_in sin;
	struct hostent *hp;
	static struct fhstatus fhs;
	char *cp;
	char *hostp = host;
	char *path;
	int s;
	struct timeval timeout;
	CLIENT *client;
	enum clnt_stat rpc_stat;
	u_short port;

	cp = mnt->mnt_fsname;
	while ((*hostp = *cp) != ':') {
		if (*cp == '\0') {
			fprintf(stderr,
			    "mount: nfs file system; use host:path\n");
			return (1);
		}
		hostp++;
		cp++;
	}
	*hostp = '\0';
	path = ++cp;
	/*
	 * Get server's address
	 */
	if ((hp = gethostbyname(host)) == NULL) {
		/*
		 * XXX
		 * Failure may be due to yellow pages, try again
		 */
		if ((hp = gethostbyname(host)) == NULL) {
			fprintf(stderr,
			    "mount: %s not in hosts database\n", host);
			return (1);
		}
	}

	args->flags = 0;
	if (hasmntopt(mnt, MNTOPT_SOFT) != NULL) {
		args->flags |= NFSMNT_SOFT;
	}
	if (hasmntopt(mnt, MNTOPT_NOINTR) == NULL) {
		args->flags |= NFSMNT_INT;
	}
#ifdef NFS3_2
	if (hasmntopt(mnt, MNTOPT_NODEVS) != NULL) {
		args->flags |= NFSMNT_NODEVS;
	}
	if (hasmntopt(mnt, MNTOPT_NOAC) != NULL) {
		args->flags |= NFSMNT_NOAC;
	}
	if (hasmntopt(mnt, MNTOPT_NOCTO) != NULL) {
		args->flags |= NFSMNT_NOCTO;
	}
#endif /* NFS3_2 */

next_ipaddr:
	/*
	 * get fhandle of remote path from server's mountd
	 */
	memset(&sin, 0, sizeof(sin));
	memcpy((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
	sin.sin_family = AF_INET;
	timeout.tv_usec = 0;
	timeout.tv_sec = 20;
	s = RPC_ANYSOCK;
	if ((client = clntudp_create(&sin, MOUNTPROG, MOUNTVERS,
	    timeout, &s)) == NULL) {
		if (rpc_createerr.cf_stat == RPC_PMAPFAILURE)
		    if (hp && hp->h_addr_list[1]) {
			hp->h_addr_list++;
			goto next_ipaddr;
		    }

		if (!printed) {
			fprintf(stderr, "mount: %s server not responding", host);
			clnt_pcreateerror("");
			printed = 1;
		}
		return (ETIMEDOUT);
	}

#ifndef NFS3_2   /* As of NFS3_2 the above clntupd_create */
	         /* does its own bindresvport */
	if (! bindresvport(s)) {
		fprintf(stderr,"Warning: mount: cannot do local bind\n");
	}
#endif /* NFS3_2 */

	client->cl_auth = authunix_create_default();
	timeout.tv_usec = 0;
	timeout.tv_sec = 20;
	rpc_stat = clnt_call(client, MOUNTPROC_MNT, xdr_path, &path,
	    xdr_fhstatus, &fhs, timeout);
	errno = 0;
	if (rpc_stat != RPC_SUCCESS) {
		if (!printed) {
			fprintf(stderr, "mount: %s server not responding", host);
			clnt_perror(client, "");
			printed = 1;
		}
		switch (rpc_stat) {
		case RPC_TIMEDOUT:
		case RPC_PMAPFAILURE:
		case RPC_PROGNOTREGISTERED:
			errno = ETIMEDOUT;
			break;
		case RPC_AUTHERROR:
			errno = EACCES;
			break;
		default:
			errno = ERPC;
			break;
		}
	}
#ifndef NFS3_2
	/* As of NFS3_2 the clnt_destroy does its own close */
	close(s);
#endif /* NFS3_2 */

	clnt_destroy(client);
	if (errno) {
		return(errno);
	}

	if (errno = fhs.fhs_status) {
		if (errno == EACCES) {
			fprintf(stderr, "mount: access denied for %s:%s\n",
			    host, path);
		} else {
			fprintf(stderr, "mount: ");
			perror(mnt->mnt_fsname);
		}
		return (errno);
	}
	if (printed) {
		fprintf(stderr, "mount: %s server ok\n", host);
		printed = 0;
	}

	/*
	 * set mount args
	 */
	args->fh = &fhs.fhs_fh;
	args->hostname = host;
#ifdef GETMOUNT
	args->fsname = mnt->mnt_fsname;
	args->flags |= NFSMNT_HOSTNAME|NFSMNT_FSNAME;
#else
	args->flags |= NFSMNT_HOSTNAME;
#endif /* GETMOUNT */
	if (args->rsize = nopt(mnt, MNTOPT_RSIZE)) {
		args->flags |= NFSMNT_RSIZE;
	}
	if (args->wsize = nopt(mnt, MNTOPT_WSIZE)) {
		args->flags |= NFSMNT_WSIZE;
	}
	if (args->timeo = nopt(mnt, MNTOPT_TIMEO)) {
		args->flags |= NFSMNT_TIMEO;
	}
	if (args->retrans = nopt(mnt, MNTOPT_RETRANS)) {
		args->flags |= NFSMNT_RETRANS;
	}
	if (port = nopt(mnt, MNTOPT_PORT)) {
		sin.sin_port = htons(port);
	} else {
		sin.sin_port = htons(NFS_PORT);	/* XXX should use portmapper */
	}

	/* new mount options in 9.0 */
        if (args->acregmax = nopt(mnt, MNTOPT_ACTIMEO)) {
                args->acdirmax = args->acregmax;
                args->acdirmin = args->acregmax;
                args->acregmin = args->acregmax;
                args->flags |= NFSMNT_ACREGMIN;
                args->flags |= NFSMNT_ACREGMAX;
                args->flags |= NFSMNT_ACDIRMIN;
                args->flags |= NFSMNT_ACDIRMAX;
        } else {
                if (args->acregmin = nopt(mnt, MNTOPT_ACREGMIN)) {
                        args->flags |= NFSMNT_ACREGMIN;
                }
                if (args->acregmax = nopt(mnt, MNTOPT_ACREGMAX)) {
                        args->flags |= NFSMNT_ACREGMAX;
                }
                if (args->acdirmin = nopt(mnt, MNTOPT_ACDIRMIN)) {
                        args->flags |= NFSMNT_ACDIRMIN;
                }
                if (args->acdirmax = nopt(mnt, MNTOPT_ACDIRMAX)) {
                        args->flags |= NFSMNT_ACDIRMAX;
                }
        }

	args->addr = &sin;

	/*
	 * should clean up mnt ops to not contain defaults
	 */
	return (0);
}

printent(mnt)
	struct mntent *mnt;
{
	char info[MNTMAXSTR+1];
#ifdef LOCAL_DISK
	struct cct_entry *cct_ent;
	char *ctime_str;
#endif /* LOCAL_DISK */

	(void) strcpy(info, (hasmntopt(mnt, MNTOPT_RO)?flg[1]:flg[0]));
#ifdef QUOTA
	if (hasmntopt(mnt, MNTOPT_QUOTA))
		(void) strcat(info, " (with quotas)");
#endif
#ifdef LOCAL_DISK
	/* Get clusterconf entry of mounting cnode so we can print name */
	if (cct_ent = getcccid(mnt->mnt_cnode)) {

	    /* Get rid of \n on end of ctime string */
	    ctime_str = ctime(&mnt->mnt_time);
	    ctime_str[strlen(ctime_str) - 1] = '\0';

	    fprintf(stdout, "%s on %s %s on %s (%s)\n",
		mnt->mnt_dir, mnt->mnt_fsname, info, ctime_str,
						cct_ent->cnode_name);
	}
	else
#endif /* LOCAL_DISK */
	    fprintf(stdout, "%s on %s %s on %s",
		mnt->mnt_dir, mnt->mnt_fsname, info, ctime(&mnt->mnt_time));
}

printmtab(outp)
	FILE *outp;
{
	FILE *mnttab;
	struct mntent *mntp;
	int maxfsname = 0;
	int maxdir = 0;
	int maxtype = 0;
	int maxopts = 0;
#ifdef LOCAL_DISK
	int not_nfs;
#endif /* LOCAL_DISK */

	/*
	 * first go through and find the max width of each field
	 */
	mnttab = mnt_setmntent(MNT_MNTTAB, "r");
	if (mnttab == NULL) {
		fprintf(stderr, "mount: ");
		perror(MNT_MNTTAB);
		exit(1);
	}
	while ((mntp = getmntent(mnttab)) != NULL) {
		if (strlen(mntp->mnt_fsname) > maxfsname) {
			maxfsname = strlen(mntp->mnt_fsname);
		}
		if (strlen(mntp->mnt_dir) > maxdir) {
			maxdir = strlen(mntp->mnt_dir);
		}
		if (strlen(mntp->mnt_type) > maxtype) {
			maxtype = strlen(mntp->mnt_type);
		}
		if (strlen(mntp->mnt_opts) > maxopts) {
			maxopts = strlen(mntp->mnt_opts);
		}
	}
	endmntent(mnttab);

	/*
	 * now print them out in pretty format
	 */
	mnttab = mnt_setmntent(MNT_MNTTAB, "r");
	if (mnttab == NULL) {
		fprintf(stderr, "mount: ");
		perror(MNT_MNTTAB);
		exit(1);
	}
	while ((mntp = getmntent(mnttab)) != NULL) {
#ifdef LOCAL_DISK
	    /*
	     * Check for -l or -L flags:  Print entry
	     *  - If neither -l nor -L was specified, OR
	     *  - If -l was specified and the mount is
	     *	 on my cnode and it's NOT NFS, OR
	     *  - If -L was specified and it is NFS
	     */
	    if ((!local_only && !local_plus_nfs) 			||
		((not_nfs = strcmp(mntp->mnt_type, MNTTYPE_NFS)) &&
			mntp->mnt_cnode == cnodeid())  		||
		(local_plus_nfs && !not_nfs))
	    {
#endif /* LOCAL_DISK */
		fprintf(outp, "%-*s", maxfsname+1, mntp->mnt_fsname);
		fprintf(outp, "%-*s", maxdir+1, mntp->mnt_dir);
		fprintf(outp, "%-*s", maxtype+1, mntp->mnt_type);
		fprintf(outp, "%-*s", maxopts+1, mntp->mnt_opts);
		fprintf(outp, " %d %d\n", mntp->mnt_freq, mntp->mnt_passno);
#ifdef LOCAL_DISK
	    }
#endif /* LOCAL_DISK */
	}
	endmntent(mnttab);
	return (0);
}

/*
 * Check to see if mntck is already mounted.
 * We have to be careful because getmntent modifies its static struct.
 */
mounted(mntck)
	struct mntent *mntck;
{
	int found = 0;
	struct mntent *mnt, mntsave;
	FILE *mnttab;

	mnttab = mnt_setmntent(MNT_MNTTAB, "r");
	if (mnttab == NULL) {
		fprintf(stderr, "mount: ");
		perror(MNT_MNTTAB);
		exit(1);
	}
	mntcp(mntck, &mntsave);
	while ((mnt = getmntent(mnttab)) != NULL) {
		if ((strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) ||
	            (strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0) ||
	            (strcmp(mnt->mnt_type, MNTTYPE_SWAPFS) == 0) ){
			continue;
		}
		if ((strcmp(mntsave.mnt_fsname, mnt->mnt_fsname) == 0) &&
		    (strcmp(mntsave.mnt_dir, mnt->mnt_dir) == 0) ) {
			found = 1;
			break;
		}
	}
	endmntent(mnttab);
	*mntck = mntsave;
	return (found);
}

mntcp(mnt1, mnt2)
	struct mntent *mnt1, *mnt2;
{
	static char fsname[128], dir[128], type[128], opts[128];

	mnt2->mnt_fsname = fsname;
	strcpy(fsname, mnt1->mnt_fsname);
	mnt2->mnt_dir = dir;
	strcpy(dir, mnt1->mnt_dir);
	mnt2->mnt_type = type;
	strcpy(type, mnt1->mnt_type);
	mnt2->mnt_opts = opts;
	strcpy(opts, mnt1->mnt_opts);
	mnt2->mnt_freq = mnt1->mnt_freq;
	mnt2->mnt_passno = mnt1->mnt_passno;
#ifdef LOCAL_DISK
	mnt2->mnt_cnode = mnt1->mnt_cnode;
#endif /* LOCAL_DISK */
}

/*
 * Return the value of a numeric option of the form foo=x, if
 * option is not found or is malformed, return 0.
 */
int
nopt(mnt, opt)
	struct mntent *mnt;
	char *opt;
{
	int val = 0;
	char *equal;
	char *str;

	if (str = hasmntopt(mnt, opt)) {
		if (equal = strchr(str, '=')) {
			val = atoi(&equal[1]);
		} else {
			fprintf(stderr, "mount: bad numeric option '%s'\n",
			    str);
		}
	}
	return (val);
}

/*
 * update /etc/mnttab
 */
void
addtomtab(mnt)
	struct mntent *mnt;
{
	FILE *mnted;
	int i;

	for (i = 0; ((i < OPENRETRY) &&
#if defined(SecureWare) && defined(B1)
	    ((ISB1) ? ((mnted = mount_setmntent(MNT_MNTTAB, "r+")) == NULL) :
	              ((mnted = mnt_setmntent(MNT_MNTTAB, "r+")) == NULL)));
									i++)
#else
	    ((mnted = mnt_setmntent(MNT_MNTTAB, "r+")) == NULL)); i++)
#endif /* B1 */
		sleep(1);

	if (mnted == NULL) {
		fprintf(stderr, "mount: ");
		perror(MNT_MNTTAB);
		exit(1);
	}
	mnt->mnt_time = time((long *)0);
	if (addmntent(mnted, mnt)) {
		fprintf(stderr, "mount: ");
		perror(MNT_MNTTAB);
		exit(1);
	}
	endmntent(mnted);
}

char *
xmalloc(size)
	int size;
{
	char *ret;

	if ((ret = (char *)malloc(size)) == NULL) {
		fprintf(stderr, "mount: ran out of memory!\n");
		exit(1);
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
	strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;
	new->mnt_time = mnt->mnt_time;
#ifdef LOCAL_DISK
	new->mnt_cnode = mnt->mnt_cnode;
#endif /* LOCAL_DISK */

	return (new);
}

/*
 * Build the mount dependency tree
 */
struct mnttree *
maketree(mt, mnt)
	struct mnttree *mt;
	struct mntent *mnt;
{
	if (mt == NULL) {
		mt = (struct mnttree *)xmalloc(sizeof (struct mnttree));
		mt->mt_mnt = mntdup(mnt);
		mt->mt_sib = NULL;
		mt->mt_kid = NULL;
	} else {
		if (substr(mt->mt_mnt->mnt_dir, mnt->mnt_dir)) {
			mt->mt_kid = maketree(mt->mt_kid, mnt);
		} else {
			mt->mt_sib = maketree(mt->mt_sib, mnt);
		}
	}
	return (mt);
}

void
printtree(mt)
	struct mnttree *mt;
{
	if (mt) {
		printtree(mt->mt_sib);
		printf("   %s\n", mt->mt_mnt->mnt_dir);
		printtree(mt->mt_kid);
	}
}

int
mounttree(mt)
	struct mnttree *mt;
{
	int ret = 0;
	struct stat sbuf;
	int error;
	int forked;
	int slptime;
	int retry;
	int firsttry;

	if (mt) {
		ret += mounttree(mt->mt_sib);
		printed = 0;
		forked = 0;
		firsttry = 1;
		slptime = BGSLEEP;
		retry = nopt(mt->mt_mnt, MNTOPT_RETRY);
		if (retry <= 0) {
			retry = 0;
		}

		for (; retry >= 0; retry--) {
			if (all) {
		            if (stat(mt->mt_mnt->mnt_dir, &sbuf) && do_slink == 0) {  /* symlink? */
			        slink = maketree(slink, mt->mt_mnt);
			        return;
			    }
			}
			error = mountfs(!forked, mt->mt_mnt);
			if (error != ETIMEDOUT && error != ENETDOWN &&
			    error != ENETUNREACH && error != ENOBUFS &&
			    error != ECONNREFUSED && error != ECONNABORTED) {
				break;
			}
			if (retry > 0) {
			    if (!forked && hasmntopt(mt->mt_mnt, MNTOPT_BG)) {
				fprintf(stderr, "mount: backgrounding\n");
				fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
				printtree(mt->mt_kid);
				if (fork()) {
					return;
				} else {
					forked = 1;
				}
			    }
			    if (!forked && firsttry) {
				fprintf(stderr, "mount: retrying\n");
				fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
				printtree(mt->mt_kid);
				firsttry = 0;
			    }
			    sleep(slptime);
			    slptime = MIN(slptime << 1, MAXSLEEP);
			}
		}

		/* mount kid even if its parent is already mounted,
	 	   yet exit with 1 since an error has occurred */
		if (error == 0 || error == EMOUNTED) {
			if (error == EMOUNTED)
				ret = 1;
			ret += mounttree(mt->mt_kid);
		} else {
			ret = error;
			if (!firsttry) {
				fprintf(stderr, "mount: giving up on:\n");
				fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
				printtree(mt->mt_kid);
			}
		}
		if (forked) {
			exit(ret);
		}
	    }
	update_mnttab_time();
	return(ret);
}


/*
 * Returns true if s1 is a pathname substring of s2.
 */
int
substr(s1, s2)
	char *s1;
	char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
}

#ifndef NFS3_2   /* As of NFS3_2 the libc bindresvport is used */
int
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
	for (port = MAX_PRIV; err && port >= MIN_PRIV; port--) {
		sin.sin_port = htons(port);
		err = bind(sd,&sin,sizeof(sin));
	}
	return (err == 0);
}
#endif /* NFS3_2 */

int
fsclean(dev)
	char *dev;
{

	int ret;
	char pad[SBSIZE];
	struct fs *super;

	ret = 0;	/* ret is modified (= 1) if device is not clean */
#if defined(SecureWare) && defined(B1)
	if ((ISB1) ? ((fsclean_fd = mount_opendev(dev, O_RDONLY)) == -1) :
	             ((fsclean_fd = open(dev, O_RDONLY)) == -1))
#else
	if ((fsclean_fd = open(dev, O_RDONLY)) == -1)
#endif /* B1 */
	{
#ifdef DEBUG
		(void) fprintf(stderr, "mount: cannot open %s\n", dev);
#endif
		(void) fprintf(stderr, "mount: ");
		perror(dev);

		return(1);
	}
	if (lseek(fsclean_fd, (long)dbtob(SBLOCK), 0) == -1)
	{
		(void) fprintf(stderr, "mount: lseek error on %s\n", dev);
		close(fsclean_fd);
		return(1);
	}
	super =  (struct fs *)pad;
	if (read(fsclean_fd, super, SBSIZE) != SBSIZE)
	{
		(void) fprintf(stderr, "mount: read error on %s\n", dev);
		close(fsclean_fd);
		return(1);
	}
#if defined(FD_FSMAGIC)
	if ((super->fs_magic != FS_MAGIC) && (super->fs_magic != FS_MAGIC_LFN)
		&& (super->fs_magic != FD_FSMAGIC))
#else /* not new magic number */
#ifdef LONGFILENAMES
	if ((super->fs_magic != FS_MAGIC) && (super->fs_magic != FS_MAGIC_LFN))
#else /* not LONGFILENAMES */
	if (super->fs_magic != FS_MAGIC)
#endif /* not LONGFILENAMES */
#endif /* new magic number */
	{
		(void) fprintf(stderr, "mount: bad super block magic number on %s\n", dev);
		close(fsclean_fd);
		return(1);
	}
	/* gives error if file system is not clean and NOT mounted
	 * other possible error will be given by intrinsic mount(2)
	 */
	if (super->fs_clean != FS_CLEAN)
	{
		if (fsmounted(dev)) {
			(void) fprintf(stderr, "file system for device %s is already mounted\n", dev);
			ret = EMOUNTED;
		} else {
			(void) fprintf(stderr, "file system for device %s needs to be fsck'ed before mounting\n", dev);

#ifdef QFS_CMDS
			if (force) {
				if (super->fs_featurebits & FSF_QFS) {
	 				(void) fprintf(stderr, "-f option is not supported on Quickstart file systems\n");
					close(fsclean_fd);
					return(2);
				} else {
 					(void) fprintf(stderr, "forcing mount\n");
				}
			}
#else /* not QFS_CMDS */
			if (force)
				(void) fprintf(stderr, "forcing mount\n");
#endif /* QFS_CMDS */
			ret = 1;	/* device is not clean */
		}
	}
#ifdef __hp9000s800
	if (magtapefs)
	    close(fsclean_fd);
#endif /* __hp9000s800 */
	return(ret);

	/*
	 *if (strncmp(strrchr(name, '/') + 1, super.fs_fsmnt, MNTLEN))
	 *	(void) printf("WARNING!! - mounting: <%.6s> as <%.6s>\n",
	 *		super.fs_fsmnt, name);
	 */
}


fsmounted(dev)
	char *dev;
{
	struct stat statval;
	struct ustat ustatval;
#ifdef DEBUG
	char errmsg[80];
#endif /* DEBUG */

/* ustat the file, error indicates it is not mounted */
	if (stat(dev,&statval) || ustat(statval.st_rdev, &ustatval)) {
#ifdef DEBUG
		(void) sprintf(errmsg, "stat/ustat %s(0x%x)", dev, statval.st_rdev);
		perror(errmsg);
#endif /* DEBUG */
		return(0);
	}
	return(1);
}

usage()
{
#if defined(SecureWare) && defined(B1)
	if (ISB1)
	    fputs(mount_usage(0), stderr);
	else{
#ifdef LOCAL_DISK
	    (void) fprintf(stderr,
	    "usage: mount  [fsname  directory [-frv] [-l|L] [-u|s] [-o options] [-t type]]\n");
	    (void) fprintf(stderr, "   or  mount  -a [-fv] [-l|L] [-u|s] [-t type]\n");
	    (void) fprintf(stderr, "   or  mount  -p [-l|L] [-u|s]\n");
#else /* LOCAL_DISK */
	    (void) fprintf(stderr,
	    "usage: mount  [fsname  directory [-frv] [-o options] [-t type]]\n");
	    (void) fprintf(stderr, "   or  mount  -a [-fv] [-t type]\n");
	    (void) fprintf(stderr, "   or  mount  -p\n");
#endif /* LOCAL_DISK */
	}
#else
#ifdef LOCAL_DISK
	    (void) fprintf(stderr,
	    "usage: mount  [fsname  directory [-frv] [-l|L] [-u|s] [-o options] [-t type]]\n");
	    (void) fprintf(stderr, "   or  mount  -a [-fv] [-l|L] [-u|s] [-t type]\n");
	    (void) fprintf(stderr, "   or  mount  -p [-l|L] [-u|s]\n");
#else /* LOCAL_DISK */
	(void) fprintf(stderr, "usage: mount  [fsname  directory [-frv] [-o options] [-t type]]\n");
	(void) fprintf(stderr, "   or  mount  -a [-fv] [-t type]\n");
	(void) fprintf(stderr, "   or  mount  -p\n");
#endif /* LOCAL_DISK */
#endif /* B1 */
		exit(2);
}

/*
 * mnt_setmntent():
 * This routine retries setmntent if errno == EACCES, and a test of lockf
 * on the setmntent file reveals that a lock exists.  This is done because
 * setmntent may have failed due to a lockf (which will cause it to
 * return EACCES) in which case we may want to retry, or setmntent may
 * have failed on fopen due to a legitimate access error in which case
 * we want to return NULL.
 */
FILE *
mnt_setmntent(filename, type)
char *filename, *type;
{
    FILE *fp;
    int fd;
    int counter = 1;
    int tries = 0;

#if defined(SecureWare) && defined(B1)
    while ((fp = (ISB1 ? mount_setmntent(filename, type) :
                        setmntent(filename, type))) == NULL) {
#else /* B1 */
    while((fp = setmntent(filename, type)) == NULL) {
#endif /* B1 */
	if (errno!=EACCES && errno!=EAGAIN) /* real failure, return NULL */
	    return NULL;
	else {			/* check to see if failure due to lockf */
	    if ((fd=open(filename,O_RDONLY))<0)
		return NULL;
	    if (lockf(fd,F_TEST,0)==0) { /* Hmm, file is not locked now... */
		close(fd);		 /* so we try again, but will only */
		if (++tries == 10) {	 /* put up with this for 10 tries. */
			errno=EACCES;
			return NULL;
		}
	    }
	    else {
		close(fd);
		if (((counter++) % 4000) == 0) {  /* emit occasional msg */
			fprintf(stderr,"mount: cannot lock %s; ",filename);
			fprintf(stderr,"still trying ...\n");
		}
	    }
	}
    }
    return(fp);
}
#ifdef LOCAL_DISK
/*
 * localroot() -- function call to determine that we've got a local
 *		root disk. (i.e., we're standalone or a root server)
 *		Returns TRUE if localroot, FALSE otherwise.
 */
int
localroot()
{
    static int  is_localroot = -1;
    int     length;
    char   *contextbuf;
    char   *s;

    if (is_localroot != -1)
	return is_localroot;

    length = getcontext(NULL, 0);   /* get length of context str */
    contextbuf = (char *)malloc(length);    /* get space to put it in */
    (void)getcontext(contextbuf, length);   /* get the context */

    /* look for localroot in context -- (in standalone context also) */
    s = strstr(contextbuf, "localroot");
    free(contextbuf);
    return is_localroot = (s != NULL);
}
#endif /* LOCAL_DISK */

/*
 * Mag tape file systems are only supported on the s800 because block
 * tape drives are only supported on the s800.  So, this routine will
 * only be compiled on the s800.  NOTE:  This routine will be compiled for
 * the s700, but will not be called!
 */
#ifdef __hp9000s800
#define MAGTAPEMAJOR 5
int
isamagtape(fd)
int fd;
{
    struct stat stbuf;

    if (fstat(fd, &stbuf) == -1)
	return 0;
    return major(stbuf.st_rdev) == MAGTAPEMAJOR;
}
#endif /* __hp9000s800 */
