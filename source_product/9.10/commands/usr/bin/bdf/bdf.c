static char *HPUX_ID = "@(#) $Revision: 66.11 $";

#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>    
#ifdef SWFS
#include <sys/swap.h>    
#endif /* SWFS */
#include <errno.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/vfs.h>

#include <stdio.h>
#include <checklist.h>
#include <mntent.h>
#include <sys/signal.h>
#ifdef SecureWare
#include <sys/security.h>
#endif


/*
 * df (Berkeley style)
 */

struct mntent *getmntpt(), *mntdup();
char	root[32];
char	*mpath();
#ifdef SWFS
int	bflag;
#endif /* SWFS */
int	iflag;
int	type;
char	*typestr;

union {
	struct fs iu_fs;
	char dummy[SBSIZE];
} sb;
#define sblock sb.iu_fs

daddr_t	alloc();
char	*strcpy();

void myalrm();
int alarmoff = 0;		/* assume no alarm */
int armnfs;
#ifdef LOCAL_DISK
int local_only = 0;		/* local disk flags */
int local_plus_nfs = 0;
cnode_t mysite;			/* what cnode am I operating on */
#endif /* LOCAL_DISK */

main(argc, argv, environ)
	int argc;
	char **argv;
	char **environ;
{
	int i;
	struct stat statb;
	char tmpname[1024];

#ifdef LOCAL_DISK
	mysite = cnodeid();	/* what cnode am I on */
#endif /* LOCAL_DISK */

	signal(SIGALRM, myalrm);
	cleanenv( &environ, "LANG", "LANGOPTS", "NLSPATH", 0 );
#ifdef SecureWare
	if( ISSECURE )
		df_init(argc, argv);
#endif

	while (argc > 1 && argv[1][0]=='-') {
		switch (argv[1][1]) {

#ifdef SWFS
		case 'b':
		        bflag++;
			break;
#endif /* SWFS */
		case 'i':
			iflag++;
			break;
#ifdef LOCAL_DISK
		case 'l':
			/* print local mounted file system info only */
			if (local_plus_nfs) {
			    (void) fprintf(stderr, 
				"bdf: conflicting options: -l, -L\n");
			    usage();
			}
			local_only++;
			break;

		case 'L':
			/* print local mounted & NFS file system info only */
			local_plus_nfs++;
			if (local_only) {
			    (void) fprintf(stderr,
				"bdf: conflicting options: -l, -L\n");
			    usage();
			}
			break;
#endif /* LOCAL_DISK */

		case 't':
			type++;
			typestr = argv[2];
			argv++;
			argc--;
			break;

		default:
			usage();
		}
		argc--, argv++;
	}
	if (argc > 1 && type)
	    usage();
	sync();
	fputs("Filesystem           kbytes    used   avail ", stdout);
	if (iflag)
		fputs("%cap iused   ifree iused", stdout);
        else
		fputs("capacity", stdout);
	fputs(" Mounted on\n", stdout);
	/*
	 * For the degenerate case of no disks specified, 
	 * search /etc/mnttab for all currently mounted disks
	 */
	if (argc <= 1) {
		FILE *mnttabp;
		struct mntent *mnt;

		if ((mnttabp = setmntent(MNT_MNTTAB, "r")) == 0)
		{
			perror(MNT_MNTTAB);
			exit(1);
		}

		/* For each entry in mnttab */
		while (mnt = getmntent(mnttabp)) {
		        armnfs = 0;		/* assume not NFS */

			/* Ignore all ignore and swap entries in mnttab */
			if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
			    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0   ||
			    strcmp(mnt->mnt_type, MNTTYPE_SWAPFS) == 0)
				continue;

			if(strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0)
			    armnfs = 1;

			/* If type spec'd, skip non-conforming types */
			if (type && strcmp(typestr, mnt->mnt_type))
				continue;
#ifdef LOCAL_DISK
			/* 
			 * if local_only or local_plus_nfs specified,
			 * skip inappropriate entries
			 */
			 if (mnt->mnt_cnode != mysite && 
			    (local_only || (local_plus_nfs && !armnfs)))
				continue;
#endif /* LOCAL_DISK */

			dfreemnt(mnt->mnt_dir, mnt);
#ifdef SWFS
			if (bflag && strcmp(MNTTYPE_HFS, mnt->mnt_type) == 0)
			{
			        dswfsmnt(mnt->mnt_dir);
			}
#endif /* SWFS */

		}

		endmntent(mnttabp);
		exit(0);
	}
	for (i=1; i<argc; i++) {
		struct mntent *mnt;

		if (stat(argv[i], &statb) < 0) {
			perror(argv[i]);
		} else {
			/*
			 * if argv[i] is a device, get info via dev routine
			 */
			if (((statb.st_mode & S_IFBLK) == S_IFBLK) ||
			     ((statb.st_mode & S_IFCHR) == S_IFCHR)) {
				dfreedev(argv[i]);
			} else {
				/* 
				 * argv[i] is a file name, get its mount point 
				 * and get the info via the dfreemnt routine
				 */
				if ((mnt = getmntpt(argv[i])) != NULL) {
				    /* 
				     * if type is set, but this entry 
				     * is not the right one, bail out
				     */
				    if (type && strcmp(typestr, mnt->mnt_type))
					    continue;
#ifdef LOCAL_DISK
				    /* 
				     * if local_only or local_plus_nfs spec'd,
				     * skip inappropriate entries
				     */
				     if (mnt->mnt_cnode != mysite && 
					(local_only || (local_plus_nfs && 
				        (strcmp(MNTTYPE_NFS, mnt->mnt_type)))))
					    continue;
#endif /* LOCAL_DISK */
				    if(strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0)
				            armnfs = 1;
				    dfreemnt(argv[i], mnt);  
				}
				else {
				    /* 
				     * We were unable to get the mount point
				     */
				    continue; 
				}
			}
#ifdef SWFS
			if (bflag)
			    dswfsmnt(argv[1]);
#endif /* SWFS */
		}
	}
	exit(0);
}

dfreedev(file)
	char *file;
{
#ifdef CDROM
	long totalblks, reserved, avail, free, used;
	struct statfs	fs;
	int autofd;

	autofd = open(file, O_RDONLY);
	if(autofd == -1) {
	    perror(file);
	    return;
	}
	if (fstatfsdev(autofd,&fs) < 0) {
		perror(file);
		close(autofd);
		return;
	}
#ifdef LOCAL_DISK
	if ((local_only || local_plus_nfs) && mysite != fs.f_cnode) {
		/*  if device isn't local, bailout w/o printing info */
		 return;
	} 
	else {
		if (mysite == fs.f_cnode) {
			(void) hidecdf(file, file, strlen(file)+1);
		}
	}
#endif /* LOCAL_DISK */
	close(autofd);
	if (strlen(file) > 19) {
		fputs(file, stdout);
		fputs("\n                   ", stdout);
	} else {
		printf("%-19.19s", file);
	}

	totalblks = fs.f_blocks;
	free = fs.f_bfree;
	used = totalblks - free;
	avail = fs.f_bavail;
	reserved = free - avail;
	printf("%8d%8d%8d", totalblks * (fs.f_bsize / 1024),
	    used * (fs.f_bsize / 1024), avail * (fs.f_bsize / 1024));
	totalblks -= reserved;

	if (iflag) {
		int files, tused;

		printf("%4ld%%", totalblks == 0 ? 0L : (long)
		    ((double) used / (double) totalblks * 100.0 + 0.5));
		files = fs.f_files;
		tused = files - fs.f_ffree;
		printf("%6ld%8ld%4ld%% ", tused, fs.f_ffree,
		    files == 0 ? 0L : (long)
			((double)tused / (double)files * 100.0 + 0.5));
	} else {
		printf("%6ld%%", totalblks == 0 ? 0L : (long)
		    ((double) used / (double) totalblks * 100.0 + 0.5));
		fputs("  ", stdout);
	}
	fputc(' ', stdout);
	fputs(mpath(file), stdout);
	fputc('\n', stdout);
#else /* CDROM */
	long totalblks, availblks, avail, free, used;
	int fi;

	fi = open(file, 0);
	if (fi < 0) {
		perror(file);
		return;
	}
	if (bread(fi, SBLOCK, (char *)&sblock, SBSIZE) == 0) {
		(void) close(fi);
		return;
	}
#ifdef LOCAL_DISK
	if (fstat(fi,&statbuf)) {
		perror(file);	/* shouldn't happen */
		/* 
		 * Don't exit, go ahead and print info from statfs, but
		 */
	}
	else {
		if ((local_only || local_plus_nfs) 
					&& mysite != statbuf.st_rcnode) {
			/*  if device isn't local, bailout w/o printing info */
			 return;
		} 
		else {
			if (mysite == statbuf.st_rcnode) {
				(void) hidecdf(file, file, strlen(file)+1);
			}
		}
	}
#endif /* LOCAL_DISK */
	printf("%-19.19s", file);

	totalblks = sblock.fs_dsize;
	free = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
	    sblock.fs_cstotal.cs_nffree;
	used = totalblks - free;
	availblks = totalblks * (100 - sblock.fs_minfree) / 100;
	avail = availblks > used ? availblks - used : 0;
	printf("%8d%8d%8d", totalblks * sblock.fs_fsize / 1024,
	    used * sblock.fs_fsize / 1024, avail * sblock.fs_fsize / 1024);
	if (iflag) {
		int inodes;
#ifdef NO_FLOAT
		printf("%4ld%%", availblks == 0 ? 0 : used * 100 / availblks );
		inodes = sblock.fs_ncg * sblock.fs_ipg;
		used = inodes - sblock.fs_cstotal.cs_nifree;
		printf("%6ld%8ld%4ld%% ", used, sblock.fs_cstotal.cs_nifree,
		    inodes == 0 ? 0 : used * 100 / inodes );
#else
		printf("%4ld%%", availblks == 0 ? 0L : (long)
		    ((double) used / (double) availblks * 100.0 + 0.5));
		inodes = sblock.fs_ncg * sblock.fs_ipg;
		used = inodes - sblock.fs_cstotal.cs_nifree;
		printf("%6ld%8ld%4ld%% ", used, sblock.fs_cstotal.cs_nifree,
		    inodes == 0 ? 0L : (long)
			((double)used / (double)inodes * 100.0 + 0.5));
#endif
	} else {
#ifdef NO_FLOAT
		printf("%6ld%%", availblks == 0 ? 0 : used * 100 / availblks );
#else
		printf("%6ld%%", availblks == 0 ? 0L : (long)
		    ((double) used / (double) availblks * 100.0 + 0.5));
#endif
		fputs("  ", stdout);
	}
	fputc(' ', stdout);
	fputs(mpath(file), stdout);
	fputc('\n', stdout);
	(void) close(fi);
#endif /* CDROM */
}


dfreemnt(file, mnt)
	char *file;
	struct mntent *mnt;
{
	long totalblks, avail, free, used, reserved;
	struct statfs fs;
	int autofd;

	/*
	 * There have been complaints about bdf hanging on a remote (NFS)
	 * file system.  The fix to that problem is to set an alarm
	 * of 10 seconds prior to the statfs() system call.  The hooker
	 * is that statfs is uninterruptable until the rpc layer in
	 * the kernel.  After several retries, the kernel recognizes
	 * that the remote system is unavailable, and starts to field
	 * signals.  It is at this point that the alarm will be noticed
	 * and honored.  At that point myalrm() is called, a global flag
	 * is set, the signal handler is reset, and program execution
	 * continues.
	 */
	
        if(armnfs)
	    alarm(10L);		/* set the alarm */
	autofd = open(file, O_RDONLY);
        if(alarmoff) {
	    fprintf(stderr, "file system %s not responding\n", file);
	    alarmoff = 0;
	    return;
	}
	if(autofd == -1) {
	    if(statfs(file, &fs) < 0) {
		perror(file);
		return;
	    }
	}
	else {
	    if (fstatfs(autofd, &fs) < 0) {
		perror(file);
		return;
	    }
	}
	close(autofd);
	if(armnfs)
	    alarm(0);		/* cancel the alarm */

#ifdef LOCAL_DISK
	/* Hide the cdf components if this file system is locally mounted */
	if (mysite == mnt->mnt_cnode) {
		(void) hidecdf(mnt->mnt_fsname, mnt->mnt_fsname, 
						strlen((mnt->mnt_fsname)+1));
		(void) hidecdf(mnt->mnt_dir, mnt->mnt_dir, 
						strlen((mnt->mnt_dir)+1));
	}
#endif /* LOCAL_DISK */

	if (strlen(mnt->mnt_fsname) > 19) {
		fputs(mnt->mnt_fsname, stdout);
		fputs("\n                   ", stdout);
	} else {
		printf("%-19.19s", mnt->mnt_fsname);
	}

	totalblks = fs.f_blocks;
	free = fs.f_bfree;
	used = totalblks - free;
	avail = fs.f_bavail;
	reserved = free - avail;
#ifdef CDROM
	printf("%8d%8d%8d", totalblks * (fs.f_bsize / 1024),
	    used * (fs.f_bsize / 1024), avail * (fs.f_bsize / 1024));
#else /* CDROM */
	printf("%8d%8d%8d", totalblks * fs.f_bsize / 1024,
	    used * fs.f_bsize / 1024, avail * fs.f_bsize / 1024);
#endif /* CDROM */
	totalblks -= reserved;

	if (iflag) {
		long files, tused;

	        printf("%4ld%%",
		    totalblks == 0 ? 0L : (long)
		    ((double) used / (double) totalblks * 100.0 + 0.5));
		files = fs.f_files;
		tused = files - fs.f_ffree;
		printf("%6ld%8ld%4ld%% ", tused, fs.f_ffree,
		       files == 0? 0L : (long)
			 ((double)tused / (double)files * 100.0 + 0.5));
	} else {
	   	printf("%6ld%%", totalblks == 0 ? 0L : (long)
		    ((double) used / (double) totalblks * 100.0 + 0.5));
		fputs("  ", stdout);
	}
	fputc(' ', stdout);
	fputs(mnt->mnt_dir, stdout);
	fputc('\n', stdout);
}


long lseek();

bread(fi, bno, buf, cnt)
	int fi;
	daddr_t bno;
	char *buf;
{
	int n;
	extern errno;

	(void) lseek(fi, (long)(bno * DEV_BSIZE), 0);
	if ((n=read(fi, buf, cnt)) != cnt) {
		printf("\nread error bno = %ld\n", bno);
		printf("count = %d; errno = %d\n", n, errno);
		exit(0);
	}
}


/*
 * Given a name like /usr/src/etc/foo.c returns the mntent
 * structure for the file system it lives in.
 */
struct mntent *
getmntpt(file)
	char *file;
{
	FILE *mntp;
	struct mntent *mnt, *mntsave;
	struct stat filestat, dirstat;

	if (stat(file, &filestat) < 0) {
		perror(file);
		return(NULL);
	}

	if ((mntp = setmntent(MNT_MNTTAB, "r")) == 0) {
		perror(MNT_MNTTAB);
		exit(1);
	}

	mntsave = NULL;
	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0   ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAPFS) == 0)
			continue;
		if ((stat(mnt->mnt_dir, &dirstat) >= 0) &&
		   (filestat.st_dev == dirstat.st_dev)) {
			endmntent(mntp);
			return mnt;
		}
	}
	endmntent(mntp);
	fputs("Couldn't find mount point for ", stderr);
	fputs(file, stderr);
	fputc('\n', stderr);
	return NULL;
}


#ifdef SWFS
dswfsmnt(mnt)
char *mnt;
{
	struct swapfs_info swapfs_buf;
	struct statfs statfs_buf;
	int autofd;
	int available;

	if (swapfs(mnt, &swapfs_buf) == 0)
	{
	        autofd = open(mnt, O_RDONLY);
		if(autofd == -1)
		        return;
	        if (fstatfs(autofd, &statfs_buf) < 0) {
		        close(autofd);
			return;
		}
#ifdef LOCAL_DISK
		if (mysite == statfs_buf.f_cnode) {
			(void) hidecdf(swapfs_buf.sw_mntpoint, 
				    swapfs_buf.sw_mntpoint, 
				    strlen(swapfs_buf.sw_mntpoint+1)); 
		}
#endif /* LOCAL_DISK */
		close(autofd);

		/*
		 * compute the swap space currently available
		 * by computing the actual the actual space available
		 * (used + file system space available to generic users) -
		 * swap reserve.  Then reduce the value to the swapon
		 * limit if it's lower
		 */
		available = swapfs_buf.sw_binuse +	/* blocks in use */
		            statfs_buf.f_bavail -	/* free blocks */
			    swapfs_buf.sw_breserve;	/* reserved */
		if (available < 0)
		    available = 0;
		if ((available > swapfs_buf.sw_bavail) &&
		    (swapfs_buf.sw_bavail > 0)) {
		    available = swapfs_buf.sw_bavail;
		}
		swapfs_buf.sw_bavail = available;

		printf("Swapping           %8d%8d%8d",
		       (statfs_buf.f_blocks * statfs_buf.f_bsize) / 1024,
		       (swapfs_buf.sw_binuse * statfs_buf.f_bsize) / 1024,
		       ((swapfs_buf.sw_bavail - swapfs_buf.sw_binuse) *
			statfs_buf.f_bsize) / 1024);

		if (!iflag)
		    printf("  ");

		printf("%4ld%%", (long) ((double) swapfs_buf.sw_binuse /
				       (double) swapfs_buf.sw_bavail *
				       100.0 + 0.5));

		if (iflag)
		    printf("                     %s\n",
			   swapfs_buf.sw_mntpoint);
		else
		    printf("   %s\n", swapfs_buf.sw_mntpoint);
	}
}
#endif /* SWFS */


/*
 * Given a name like /dev/dsk/c0d0s2, returns the mounted path, like /usr.
 * If the volume is locally mounted, then the CDFs are collapsed in the
 * mounted path name.
 */
char *
mpath(file)
	char *file;
{
	FILE *mntp;
	register struct mntent *mnt;

	if ((mntp = setmntent(MNT_MNTTAB, "r")) == 0) {
		perror(MNT_MNTTAB);
		exit(1);
	}

	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(file, mnt->mnt_fsname) == 0) {
			endmntent(mntp);
#ifdef LOCAL_DISK
			/*  If local disk, hide the CDF components */
			if (mysite == mnt->mnt_cnode) {
				(void) hidecdf(mnt->mnt_dir, mnt->mnt_dir,
						strlen(mnt->mnt_dir)+1); 
			}
#endif /* LOCAL_DISK */
			return (mnt->mnt_dir);
		}
	}
	endmntent(mntp);
	return "";
}


usage()
{
#ifdef LOCAL_DISK
#ifdef SWFS
	fputs("usage: bdf [ -b ] [ -i ] [ -l | -L ] [-t type | file... ]\n", 
								stderr);
#else /* not SWFS */
	fputs("usage: bdf [ -i ] [ -l | -L ] [-t type | file... ]\n", stderr);
#endif /* not SWFS */
#else /* not LOCAL_DISK */
#ifdef SWFS
	fputs("usage: bdf [ -b ] [ -i ] [-t type | file... ]\n", stderr);
#else /* not SWFS */
	fputs("usage: bdf [ -i ] [-t type | file... ]\n", stderr);
#endif /* not SWFS */
#endif /* not LOCAL_DISK */
	exit(0);
}


void
myalrm()
{
    alarmoff = 1;
    signal(SIGALRM, myalrm);
}
