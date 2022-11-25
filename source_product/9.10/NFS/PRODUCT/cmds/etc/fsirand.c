#ifndef lint
static  char rcsid[] = "@(#)fsirand:	$Revision: 1.29.109.1 $	$Date: 91/11/19 14:01:31 $";
#endif
/*static  char sccsid[] = "fsirand.c 1.1 85/05/30 Copyr 1984 Sun Micro";*/

/*
 * fsirand - Copyright (c) 1984 Sun Microsystems.
 */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <stdio.h>
#include <sys/param.h>
#include <sys/fs.h>

#include <sys/sysmacros.h>	 
#include <sys/vnode.h>
#include <sys/inode.h>
#include <sys/ino.h>

char fsbuf[SBSIZE];

/*   
 *     SecureWare  && B1
 * A secure file system's disk inode is larger than a normal file system's
 * inode.  So this array cannot just be an array of dinode struct because
 * later on in the code there is a completion check in a for loop that
 * is computed using the &dibuf[number_of_inode_read].  Because the disk inode
 * are different size, this does not work.  What needs to be done is to
 * make the array a char array and then use the run time computed size
 * of the disk inode for the file system in question to terminate the
 * for loop (i.e. &dibuf[disk_dinode_size() * number_of_inodes_read])
 */
#if defined(SecureWare) && defined(B1)
char dibuf[MAXBSIZE];
#else
struct dinode dibuf[MAXBSIZE/sizeof (struct dinode)];
#endif /* defined(SecureWare) && defined(B1) */

extern int errno;
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

main(argc, argv)
int	argc;
char	*argv[];
{
	struct fs *fs;
	int fd;
	char *dev;
	int bno;
	struct dinode *dip;
	int inum, imax;
	int n;
	int seekaddr, bsize;
	int pflag = 0;
	struct timeval timeval;
#if defined(SecureWare) && defined(B1)
	int real_dinode_size;
#endif


#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("fsirand",0);
#endif NLS

	argv++;
	argc--;
	if (argc > 0 && strcmp(*argv, "-p") == 0) {
		pflag++;
		argv++;
		argc--;
	}
	if (argc <= 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "Usage: fsirand [-p] special\n")));
		exit(1);
	}
	dev = *argv;
	fd = open(dev, pflag ? 0 : 2);
	if (fd == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "fsirand: Cannot open %s\n")), dev);
		exit(1);
	}
	if (lseek(fd, (unsigned)SBLOCK * DEV_BSIZE, 0) == -1) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "fsirand: Seek to superblock failed\n")));
		exit(1);
	}
	fs = (struct fs *) fsbuf;
	if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
	     fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "fsirand: Read of superblock failed %d\n")), errno);
		exit(1);
	}
#ifdef hpux
	if ((fs->fs_magic != FS_MAGIC) && (fs->fs_magic != FS_MAGIC_LFN)) {
#else
	if (fs->fs_magic != FS_MAGIC) {
#endif hpux
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "fsirand: Not a superblock\n")));
		exit(1);
	}
	if (pflag) {
		printf((catgets(nlmsg_fd,NL_SETN,6, "fsid: %x %x\n")), fs->fs_id[0], fs->fs_id[1]);
	} else {
		n = getpid();
		srand(timeval.tv_sec + timeval.tv_usec + n);
		while (n--) {
			rand();
		}
	}

#if defined(SecureWare) && defined(B1)
	/*
	 * This is a call to a SecureWare library that will initialize
	 * the size of the disk inodes found on a file system
	 *
	 * The only reason that this command will work with a secure
	 * disk inode is that the normal disk inode is overlayed at
	 * the front of the secure disk inode.
	 */
	disk_set_file_system(fs);
	real_dinode_size = disk_dinode_size();
	bsize = INOPB(fs) * real_dinode_size;
#else
	bsize = INOPB(fs) * sizeof (struct dinode);
#endif /* SecureWare && B1 */

	inum = 0;
	imax = fs->fs_ipg * fs->fs_ncg;
	while (inum < imax) {
		bno = itod(fs, inum);
		seekaddr = (unsigned)fsbtodb(fs, bno) * DEV_BSIZE;
		if (lseek(fd, seekaddr, 0) == -1) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,7, "fsirand: lseek to %d failed\n")), seekaddr);
			exit(1);
		}
		n = read(fd, (char *) dibuf, bsize);
		if (n != bsize) {
			printf((catgets(nlmsg_fd,NL_SETN,8, "fsirand: premature EOF\n")));
			exit(1);
		}

#if defined(SecureWare) && defined(B1)
		/*
		 * When the command might be used on a secure file system,
		 * use disk_inode_incr() to increment the disk inode pointer
		 * by the appropriate size (either normal disk inode or
		 * secure disk inode)
		 *
		 * As mentioned earlier, the terminating condition of the
		 * for loop needs to be a bit different when dealing
		 * with secure file system disk inodes.
		 * Also, you can't just do a dip++ to increment the pointer
		 * since it can be of a different size.
		 */
		for (dip = (struct dinode *) dibuf; 
		     dip <(struct dinode *)&dibuf[real_dinode_size * INOPB(fs)];
		     disk_inode_incr(&dip, 1) ) {
#else
		for (dip = dibuf; dip < &dibuf[INOPB(fs)]; dip++) {
#endif /* SecureWare && B1 */
			if (pflag) {
     				nl_printf((catgets(nlmsg_fd,NL_SETN,9, "ino %1$d gen %2$x\n")), inum, dip->di_gen);  
			} else {
				dip->di_gen = rand();
			}
			inum++;
		}
		if (!pflag) {
			if (lseek(fd, seekaddr, 0) == -1) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "fsirand: lseek to %d failed\n")),
				    seekaddr);
				exit(1);
			}
			n = write(fd, (char *) dibuf, bsize);
			if (n != bsize) {
				printf((catgets(nlmsg_fd,NL_SETN,11, "fsirand: premature EOF\n")));
				exit(1);
			}
		}
	}
	if (!pflag) {
		gettimeofday(&timeval, 0);
		fs->fs_id[0] = timeval.tv_sec;
		fs->fs_id[1] = timeval.tv_usec + getpid();
		if (lseek(fd, (unsigned)SBLOCK * DEV_BSIZE, 0) == -1) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,12, "fsirand: Seek to superblock failed\n")));
			exit(1);
		}
		if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "fsirand: Write of superblock failed %d\n")),
			    errno);
			exit(1);
		}
	}
	for (n = 0; n < fs->fs_ncg; n++ ) {
		seekaddr = (unsigned)fsbtodb(fs, cgsblock(fs, n)) * DEV_BSIZE;
		if (lseek(fd,  seekaddr, 0) == -1) {
		     fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,14, "fsirand: Seek to alternative superblock failed\n")));
		     exit(1);
		}
		if (pflag) {
			if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
				nl_fprintf(stderr,
		(catgets(nlmsg_fd,NL_SETN,15, "fsirand: Read of alternative superblock failed %1$d %2$d\n")),
				    errno, seekaddr);
				exit(1);
			}
#ifdef hpux
			if ((fs->fs_magic != FS_MAGIC) && (fs->fs_magic != FS_MAGIC_LFN)) {
#else
			if (fs->fs_magic != FS_MAGIC) {
#endif hpux
			    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,16, "fsirand: Not an alternative superblock\n")));
			    exit(1);
			}
		} else {
			if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
				fprintf(stderr,
			(catgets(nlmsg_fd,NL_SETN,17, "fsirand: Write of alternative superblock failed\n")));
				exit(1);
			}
		}
	}
}
