static char *HPUX_ID = "@(#) $Revision: 70.12 $";
/*
 * newfs: friendly front end to mkfs
 * Notes: 
 *	1.  This program no longer contains subroutine getdiskbyname();
 *	    It is expected that getdiskbyname and its related routines
 *	    will be a part of libc.
 *
 *	2.  This program assumes the new naming convention for disks
 *	    i.e.: /dev/{r}dsk/(r)(c#d)#s#
 *	
 * nm	850716
 *	
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <sys/dir.h>

#include <stdio.h>
#include <disktab.h>
#ifdef LONGFILENAMES
#include <fcntl.h>
#include <mntent.h>
#include <sys/vfs.h>
#endif /* LONGFILENAMES */
#include <unistd.h>
#include <sys/diskio.h>

#define	BOOTDIR	"/etc"		/* directory for boot blocks */

int	verbose;		/* show mkfs line before exec */
int	forced = 0;		/* continue to process even the target device
				   is mounted */
#ifdef hp9000s800
int	noboot = 1;		/* don't fill boot blocks */
#else
int	noboot = 0;		/* default:  fill boot blocks */
#endif
#ifdef LONGFILENAMES
char	long_or_short;		/* Set to 'S' or 'L' */
#endif /* LONGFILENAMES */
int	fssize;			/* file system size */
int	fsize;			/* fragment size */
int	bsize;			/* block size */
int	ntracks;		/* # tracks/cylinder */
int	nsectors;		/* # sectors/track */
int	sectorsize;		/* bytes/sector */
int	cpg;			/* cylinders/cylinder group */
int	minfree = -1;		/* free space threshold */
int	rpm;			/* revolutions/minute of drive */
int	density;		/* number of bytes per inode */
int	errflg = 0;		/* errors in parsing input */
int	devfd;			/* use for s300 boot partition */

char	*av[20];		/* argv array and buffers for exec mkfs*/
char	a2[20];			/* fssize argv for exec mkfs	*/
char	a3[20];		        /* nsectors argv                */	
char	a4[20];			/* ntracks argv			*/
char	a5[20];			/* bsize argv			*/
char	a6[20];			/* fsize argv			*/
char	a7[20];			/* cpg argv			*/
char	a8[20];			/* minfree argv			*/
char	a9[20];			/* rpm argv			*/
char	a10[20];		/* density argv			*/

char	device[MAXPATHLEN];
char	cmd[BUFSIZ];		/* command line of mkfs		*/
char	*prog_name;

char	*strchr();
char	*strrchr();

extern char *optarg;
extern int optind, opterr;

main(argc, argv)
	int argc;
	char *argv[];
{
	int c;
	char *cp, *special;
	register struct disktab *dp;
	register struct partition *pp;

	struct stat st;
	register int i;
	int status;
#ifdef QFS_CMDS
	int create_qfs = 0;
	int create_ufs = 0;
	int qfs_log_size = 0;
	char qfs_or_ufs = '\0';
#endif /* QFS_CMDS */
	int	dens_spc;
	int	cpg_spc;
	int fd;
	capacity_type cap;

	prog_name = argv[0];

#ifdef LONGFILENAMES
	long_or_short = '\0';

#ifdef QFS_CMDS
	while ((c = getopt(argc, argv, "vnLSQURF:s:b:f:t:c:m:r:i:")) != EOF)
#else /* not QFS_CMDS */
	while ((c = getopt(argc, argv, "vnLSFs:b:f:t:c:m:r:i:")) != EOF)
#endif /* QFS_CMDS */
#else /* not LONGFILENAMES */
	while ((c = getopt(argc, argv, "vns:b:f:t:c:m:r:i:")) != EOF)
#endif /* not LONGFILENAMES */
	{
		switch(c) {
			case 'v':
				verbose++;
				break;

			case 'n':
				noboot++;
				break;

			case 'F':
				forced++;
				break;

#ifdef LONGFILENAMES
			case 'L':
			case 'S':
				long_or_short = c;
				break;
#endif /* LONGFILENAMES */
#ifdef QFS_CMDS
			case 'Q':
				create_qfs = 1;
				break;
			
			case 'U':
				create_ufs = 1;
				break;

			case 'R':
				qfs_log_size = atoi(optarg);
				if (qfs_log_size < 0)
					fatal("%s: bad recovery log size",
						optarg);
				break;
#endif /* QFS_CMDS */
	

			case 's':
				fssize = atoi(optarg);
				if (fssize < 0)
					fatal("%s: bad file system size",
						optarg);
				break;

			case 't':
				ntracks = atoi(optarg);
				if (ntracks < 0)
					fatal("%s: bad total tracks", optarg);
				break;

			case 'b':
				bsize = atoi(optarg);
				if (bsize < 0 || bsize < MINBSIZE)
					fatal("%s: bad block size", optarg);
				break;

			case 'f':
				fsize = atoi(optarg);
				if (fsize < 0)
					fatal("%s: bad frag size", optarg);
				break;

			case 'c':
				cpg = atoi(optarg);
				if (cpg < 0)
					fatal("%s: bad cylinders/group", optarg);
				break;

			case 'm':
				minfree = atoi(optarg);
				if (minfree < 0 || minfree > 99)
					fatal("%s: bad free space %%",
						optarg);
				break;

			case 'r':
				rpm = atoi(optarg);
				if (rpm < 0)
					fatal("%s: bad revs/minute", optarg);
				break;

			case 'i':
				density = atoi(optarg);
				if (density < 0)
					fatal("%s: bad bytes per inode",
						optarg);
				break;

			default:
				errflg++;
				break;
			}
	}

	if ( argc < 2 || errflg )
		usage();
	special = argv[optind++];

#ifdef QFS_CMDS
	if (create_qfs && create_ufs)
		usage();
#endif /* QFS_CMDS */

	/* this part of code is not necessary for HP-UX system since
         * we follow Bell naming convention: user will have to specify
	 * /dev/rxxx  for character special device file
         * 
  	 * cp = strrchr(special, '/');
	 * if (cp != 0)
	 *  	special = cp + 1;
	 * if (*special == 'r' && special[1] != '0' && special[1] != '1')
 	 *	special++;
	 * sprintf(device, "/dev/r%s", special);
	 * special = device;
	 */

	/* check the specified special file to see if it's  character
         * special device file
         */
	if (stat(special, &st) < 0) {
		fprintf(stderr, "%s: ", prog_name);
		perror(special);
		exit(2);
	}
	if ((st.st_mode & S_IFMT) != S_IFCHR)
		fatal("%s: not a character device", special);

#ifndef DISC_LABEL
	dp = getdiskbyname(argv[optind]);
#else
	dp = getdiskbynum(argv[optind], special);
#endif
	if (dp == 0)
		fatal("%s: unknown disk type", argv[optind]);

/* For series 600 and 800, we derive the section number from the special
 * file name.  For series 300, 400, and 700 (which will all support
 * partitioning thru software disk striping), we derive the section number
 * from the special file minor number.
 */
        /* Series 600/800 */
	if (sysconf(_SC_IO_TYPE) == IO_TYPE_SIO) {
	    if ((fd=open(special, O_RDONLY)) < 0) {
		fatal("Couldn't open %s",special);
	    }
	    if (ioctl(fd, DIOC_CAPACITY, &cap) < 0) {
		cp = strchr(special, '\0') - 2;
		if ( (*cp >= '0') && (*cp <= '9') )
		    i = atoi(cp);
		else if ( (*++cp >= '0') && (*cp <= '9') )
		    i = atoi(cp);
		else
		    fatal("error: can't figure out file system section");
		
		/*  getdiskbyname on retrieves information for sections 0-NSECTI
		    ONS */
		if ( i >= NSECTIONS )
		    fatal("error: can't figure out file system section");
		pp = &dp->d_partitions[i];
	    } else {
		if (cap.lba == 0)
			fatal("error: no space allocated to device");
                pp = &dp->d_partitions[2];
                pp->p_size = cap.lba;
	    }
            close(fd);

        /* Series 300/400/700 (IO_TYPE_WSIO) */
	} else {
                if (major(st.st_rdev) == 55) {	/* 55 is the major for autoch */
                  i = 0;
                } else if (major(st.st_rdev) == 104) {	/* 104 is the major for opal */
                  i = 1;
                } else {
/*
  This line was originally:
                  i = m_volume(st.st_rdev);
  but m_volume macro, as defined in sys/sysmacros.h, is defined only for
  300/400/700 systems. Since S800 code should run on S700 machines, the
  following kludge makes the command run-time smart so that if the S800 version
  of the command is run on an S700 system, it will work. The catch is to make
  sure to change the mask in the following line if its definition in sysmacros
  file changes.  AL 9/80/91
*/
                  i = (int)((unsigned)(st.st_rdev)&0xf);
	        }
                cp = ltoa((long)i);
		if (dp != 0)
			pp = &dp->d_partitions[i];
	}

	if (fssize == 0) {
		fssize = pp->p_size;
		if (fssize < 0)
			fatal("error: no size for section '%s' in disktab", cp);
	}
	if (nsectors == 0) {
		nsectors = dp->d_nsectors;
		if (nsectors < 0)
			fatal("error: no #sectors/track in disktab");
	}
	if (ntracks == 0) {
		ntracks = dp->d_ntracks;
		if (ntracks < 0)
			fatal("error: no #tracks in disktab");
	}
	if (sectorsize == 0) {
		sectorsize = dp->d_secsize;
		if (sectorsize < 0)
			fatal("error: no sector size in disktab");
	}
	if (bsize == 0) {
		bsize = pp->p_bsize;
		if (bsize < 0)
			fatal("error: no block size for section '%s' in disktab",
				cp);
	}
	if (fsize == 0) {
		fsize = pp->p_fsize;
		if (fsize < 0)
			fatal("error: no frag size for section '%s' in disktab",
				cp);
	}
	if (rpm == 0) {
		rpm = dp->d_rpm;
		if (rpm < 0)
			fatal("error: no revolutions/minute in disktab");
	}

	/*
	 * It may not be possible for mkfs to satisfy both the inode density
	 * and cylinder group size requested.  In order to let mkfs know
	 * which of these parameters was specified by the user, and should
	 * therefore be given priority, these two parameters will be passed
	 * as a negative number if they are user specified.
	 */

	dens_spc = 0;
	if (density <= 0)
		density = 2048;
	else
		dens_spc = 1;
	if (minfree < 0)
		minfree = 10;
	cpg_spc = 0;
	if (cpg == 0)
		cpg = 16;
	else
		cpg_spc = 1;
	i = 0;
	sprintf(a2, "%d", fssize);
	av[i++] = a2;
	sprintf(a3, "%d", nsectors);
	av[i++] = a3;
	sprintf(a4, "%d", ntracks);
	av[i++] = a4;
	sprintf(a5, "%d", bsize);
	av[i++] = a5;
	sprintf(a6, "%d", fsize);
	av[i++] = a6;
	if (cpg_spc)
		sprintf(a7, "%d", -1*cpg);
	else
		sprintf(a7, "%d", cpg);
	av[i++] = a7;
	sprintf(a8, "%d", minfree);
	av[i++] = a8;
	sprintf(a9, "%d", rpm / 60);
	av[i++] = a9;
	if (dens_spc)
		sprintf(a10, "%d", -1*density);
	else
		sprintf(a10, "%d", density);
	av[i++] = a10;
	av[i++] = 0;

#ifdef LONGFILENAMES
	if (long_or_short == '\0' ) {
#if defined(FD_FSMAGIC)
		if (is_root_lfn())
			long_or_short = 'L';
		else long_or_short = 'S';
#else /* not new magic number */
		long_or_short = default_magic()==FS_MAGIC_LFN?'L':'S';
#endif /* not new magic number */
	}
#ifdef QFS_CMDS
	if (!create_qfs && !create_ufs) {
		if (is_root_qfs()) {
			qfs_or_ufs = 'Q';
		} else {
			 qfs_or_ufs = 'U';
		}
	} else {
		if (create_qfs) {
			qfs_or_ufs = 'Q';
		} else { 
			qfs_or_ufs = 'U';
		}
	}

	if (qfs_log_size) { 
		if (forced)
		  sprintf(cmd, "/etc/mkfs -%c -%c -R -F %d  %s", long_or_short, 
			qfs_or_ufs, qfs_log_size, special);
		else
		  sprintf(cmd, "/etc/mkfs -%c -%c -R %d  %s", long_or_short, 
			qfs_or_ufs, qfs_log_size, special);
	}
	else {
		if (forced)
			sprintf(cmd, "/etc/mkfs -%c -%c -F %s", long_or_short, qfs_or_ufs, special);
		else
			sprintf(cmd, "/etc/mkfs -%c -%c %s", long_or_short, qfs_or_ufs, special);

#else /* not QFS_CMDS */
	if (forced)
		sprintf(cmd, "/etc/mkfs -%c -F %s", long_or_short, special);
	else
		sprintf(cmd, "/etc/mkfs -%c %s", long_or_short, special);
#endif /* QFS_CMDS */
#else /* not LONGFILENAMES */
	sprintf(cmd, "/etc/mkfs %s", special);
#endif /* not LONGFILENAMES */

	for (i = 0; av[i] != 0; i++) {
		strcat(cmd, " ");
		strcat(cmd, av[i]);
	}

/* This segment is included here for the autochanger.  Opening the
 * device before the call to mkfs will prevent removing the media from
 * the platter.
 */
#ifndef hp9000s800
	if(*cp == '0' && !noboot) {
	     	devfd = open(special, 1);
		if (devfd < 0) {
		        fprintf(stderr, "%s: ", prog_name);
			perror(special);
			exit(1);
		}
	}
#endif
	
	if (verbose)
		printf("%s\n", cmd);
	if (status = system(cmd))
		exit(status>>8);		/* DSDrs01374: exit code not
						 * correctly propagated 
					     	 */

#ifndef hp9000s800
	/* only install boot program in root partition */
	if (*cp == '0' && !noboot) 
		installboot(special);
#endif
	
	exit(0);
}

#ifndef hp9000s800
installboot(dev)
	char *dev;
{
	int fd;
	char bootblock[MAXPATHLEN];
	char bootimage[BBSIZE];

	sprintf(bootblock, "%s/%s", BOOTDIR, "boot");
	if (verbose) {
		printf("installing boot code\n");
		printf("sector 0 boot = %s\n", bootblock);
	}
	fd = open(bootblock, 0);
	if (fd < 0) {
		fprintf(stderr, "%s: ", prog_name);
		perror(bootblock);
		exit(1);
	}
	if (read(fd, bootimage, BBSIZE) < 0) {
		fprintf(stderr, "%s: ", prog_name); 
		perror(bootblock);
		exit(2);
	}
	close(fd);
	if (write(devfd, bootimage, BBSIZE) != BBSIZE) {
		fprintf(stderr, "%s: ", prog_name);
		perror(dev);
		exit(2);
	}
	close(devfd);
}
#endif

/*VARARGS*/
fatal(fmt, arg1, arg2)
	char *fmt;
{

	fprintf(stderr, "%s: ", prog_name);
	fprintf(stderr, fmt, arg1, arg2);
	putc('\n', stderr);
	exit(10);
}

usage()
{
#ifdef QFS_CMDS
/* put in after 8.0 
	fprintf(stderr, "usage: %s [ -L | -S ] [ -Q | -U ] [ -R recovery_log_size ] [ -v ] [ -F ]
		[ mkfs-options ] %s\n", prog_name, "special-device device-type");
*/ 
	fprintf(stderr, "usage: %s [ -L | -S ] [ -v ] [ -F ] [ mkfs-options ] %s\n", prog_name,
		"special-device device-type");
#else /* not QFS_CMDS */
#ifdef LONGFILENAMES
	fprintf(stderr, "usage: %s [ -L | -S ] [ -v ] [ -F ] [ mkfs-options ] %s\n", prog_name,
		"special-device device-type");
#else /* not LONGFILENAMES */
	fprintf(stderr, "usage: %s [ -v ] [ -F ] [ mkfs-options ] %s\n", prog_name,
		"special-device device-type");
#endif /* not LONGFILENAMES */
#endif /* not QFS_CMDS */
	fprintf(stderr, "where mkfs-options are:\n");
	fprintf(stderr, "\t-s file system size (sectors)\n");
	fprintf(stderr, "\t-t tracks/cylinder\n");
	fprintf(stderr, "\t-b block size\n");
	fprintf(stderr, "\t-f frag size\n");
	fprintf(stderr, "\t-c cylinders/group\n");
	fprintf(stderr, "\t-m minimum free space %%\n");
	fprintf(stderr, "\t-r revolutions/minute\n");
	fprintf(stderr, "\t-i number of bytes per inode\n");
	exit(1);
}

#if defined(FD_FSMAGIC)
is_root_lfn()
{
	struct statfs fs;

	if (statfs("/",&fs) < 0) {
		printf("cannot statfs on /\n");
		exit(1);
	}

	if ((fs.f_magic == FS_MAGIC_LFN) || (fs.f_featurebits & FSF_LFN))
		return 1;
	else return 0;
}

#ifdef QFS_CMDS
is_root_qfs()
{
	struct statfs fs;

	if (statfs("/",&fs) < 0) {
		printf("cannot statfs on /\n");
		exit(1);
	}

	if (fs.f_featurebits & FSF_QFS)
		return 1;
	else return 0;
}
#endif /* QFS_CMDS */
#else /* not new magic number */

#ifdef LONGFILENAMES
/*
 * Return the file system magic number of the root file system.
 */

default_magic()
{
#if defined DUX || defined DISKLESS
	struct statfs fs;

	if (statfs("/",&fs) < 0) {
		printf("cannot statfs on /\n");
		exit(1);
	}
	if (fs.f_spare[1] == 14)
		return(FS_MAGIC);
	if (fs.f_spare[1] == 255)
		return(FS_MAGIC_LFN);
	printf("mkfs: bad super block magic number on /\n");
	exit(1);
#else
	FILE *fp;
	struct mntent *mnt;
	struct stat statroot, statdev;
	char pad[MAXBSIZE];
	struct fs *super;
	int fd;
	int found = 0;

	/*
	 * Read /etc/mnttab to find the which device the root file
	 * system is on.
	 */

	if ( (fp=setmntent(MNT_MNTTAB, "r")) == NULL ) {
		perror(MNT_MNTTAB);
		exit(1);
	}

	while( (mnt=getmntent(fp)) != NULL ) {
		/* skip non-root and swap, ignore, nfs, ... entries */
		if ( (strcmp(mnt->mnt_dir, "/")==0)  &&
		     (strcmp(mnt->mnt_type, MNTTYPE_HFS)==0) ) {
			++found;
			break;
		}
	}

	endmntent(fp);

	if ( !found ) {
		printf("Can't find / in %s\n", MNT_MNTTAB);
		exit(1);
	}
	
	/*
	 * Sanity check.  Make sure the /etc/mnttab is not lying to
	 * us about where / is located.
	 */

	if ( stat(mnt->mnt_dir, &statroot) < 0 ) {
		perror(mnt->mnt_dir);
		exit(1);
	}

	if ( stat(mnt->mnt_fsname, &statdev) < 0 ) {
		perror(mnt->mnt_fsname);
		exit(1);
	}

	if ( statroot.st_dev != statdev.st_rdev ) {
		printf("Can't find root file system. Check %s entry\n",
			MNT_MNTTAB);
		exit(1);
	}
 
	/*
	 * Get the magic number of the root file system.
	 */

	if ((fd = open(mnt->mnt_fsname, O_RDONLY)) == -1)
	{
		printf("newfs: cannot open %s\n", mnt->mnt_fsname);
		exit(1);
	}
	if (lseek(fd, (long)dbtob(SBLOCK), 0) == -1)
	{
		printf("newfs: lseek error on %s\n", mnt->mnt_fsname);
		exit(1);
	}
	super =  (struct fs *)pad;
	if (read(fd, super, SBSIZE) != SBSIZE)
	{
		printf("newfs: read error on %s\n", mnt->mnt_fsname);
		exit(1);
	}
	close(fd);
	if ((super->fs_magic != FS_MAGIC) && (super->fs_magic != FS_MAGIC_LFN))
	{
		printf("newfs: bad super block magic number on %s\n", mnt->mnt_fsname);
		exit(1);
	}
	return(super->fs_magic);
#endif /* defined DUX || defined DISKLESS */
}
#endif /* LONGFILENAMES */
#endif /* not new magic number */
