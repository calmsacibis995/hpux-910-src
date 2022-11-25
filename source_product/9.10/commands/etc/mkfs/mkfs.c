static char *HPUX_ID = "@(#) $Revision: 70.18 $";

/*
 * make file system for cylinder-group style file systems
 * add fs_clean        4/12/85
 * SYNOPSIS: either specify all optional arguments or none
 */

/*
 * The following constants set the options used for the number
 * of sectors (fs_nsect), and number of tracks (fs_ntrak).
 */
#define DFLNSECT	32
#define DFLNTRAK	16

/*
 * The following two constants set the option block and fragment sizes.
 * Both constants must be a power of 2 and meet the following constraints:
 *	MINBSIZE <= DESBLKSIZE <= MAXBSIZE
 *	DEV_BSIZE <= DESFRAGSIZE <= DESBLKSIZE
 *	DESBLKSIZE / DESFRAGSIZE <= 8
 */
#define DESBLKSIZE	8192
#define DESFRAGSIZE	1024

/*
 * Cylinder groups may have up to MAXCPG cylinders. The actual
 * number used depends upon how much information can be stored
 * on a single cylinder. The option is to used 16 cylinders
 * per group.
 */
#define	DESCPG		16	/* desired fs_cpg */

/*
 * MINFREE gives the minimum acceptable percentage of file system
 * blocks which may be free. If the freelist drops below this level
 * only the superuser may continue to allocate blocks. This may
 * be set to 0 if no reserve of free blocks is deemed necessary,
 * however throughput drops by fifty percent if the file system
 * is run at between 90% and 100% full; thus the option value of
 * fs_minfree is 10%.
 */
#define MINFREE		10

/*
 * ROTDELAY gives the minimum number of milliseconds to initiate
 * another disk transfer on the same cylinder. It is used in
 * determining the rotationally optimal layout for disk blocks
 * within a file.
 */

/* For S300 and S700: All disks have a rotational delay of 1ms. */
/* FSDlj09629 */
#define ROTDELAY_DEFAULT 1

#ifdef __hp9000s800
/* For spectrum, the rotational delay depends on the interface.
 * For HP-IB the value for ROTDELAY is 6ms, for ALINK it is 4ms.
 */
#define ROTDELAY_HPIB	6
#define ROTDELAY_ALINK	4
#define ROTDELAY_NIO_HPIB	6

/* Add scsi, autochanger support */
/* FSDlj09629 */
#define ROTDELAY_SCSI_IR	0
#define ROTDELAY_SCSI	1
#define ROTDELAY_AUTO	0

#define MAJOR_B_HPIB	0
#define MAJOR_C_HPIB	4
#define MAJOR_B_HPFL	10
#define MAJOR_C_HPFL	12
#define MAJOR_B_NIO_HPIB	8
#define MAJOR_C_NIO_HPIB	7

/* Scsi, autochanger support */
#define MAJOR_B_SCSI	7
#define MAJOR_C_SCSI	13
#define MAJOR_B_AUTOCH	12
#define MAJOR_C_AUTOCH	19
#endif /* __hp9000s800 */

/*
 * MAXCONTIG sets the option for the maximum number of blocks
 * that may be allocated sequentially. Since UNIX drivers are
 * not capable of scheduling multi-block transfers, this options
 * to 1 (ie no contiguous blocks are allocated).
 */
#define MAXCONTIG	1

/*
 * MAXBLKPG determines the maximum number of data blocks which are
 * placed in a single cylinder group. This is currently set to one
 * fourth of the number of blocks in a cylinder group.
 */
#define MAXBLKPG(fs)	((fs)->fs_fpg / (fs)->fs_frag / 4)

/*
 * Each file system has a number of inodes statically allocated.
 * We allocate one inode slot per NBPI bytes, expecting this
 * to be far more than we will ever need.
 */
#define	NBPI		2048

/*
 * Disks are assumed to rotate at 60HZ, unless otherwise specified.
 */
#define	DEFHZ		60

#ifdef LONGFILENAMES
#define DIRSIZ_MACRO
#endif

#ifndef STANDALONE
#include <stdio.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/fs.h>
#include <sys/sysmacros.h>
#define KERNEL
#define _KERNEL
#include <sys/dir.h>
#undef KERNEL
#undef _KERNEL
#include <sys/stat.h>
#include <ustat.h>
#include <sys/pstat.h>
#include <ctype.h>
#include <mntent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/scsi.h>
#include <sys/diskio.h>

#ifdef __hp9000s800
#include <sys/libIO.h>

#define VMUNIX "/hp-ux"		/* the kernel */
#define VERSION -3		/* I/O tree version */
#endif /* __hp9000s800 */

#define UMASK		0755
#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

#ifdef LONGFILENAMES
#include <sys/vfs.h>
#endif
#ifdef TRUX
#include <sys/security.h>
#endif

union {
	struct fs fs;
	char pad[MAXBSIZE];
} fsun;
#define	sblock	fsun.fs
struct fs *fs;

struct	csum *fscs;

union {
	struct cg cg;
	char pad[MAXBSIZE];
} cgun;
#define	acg	cgun.cg

/* One block of inodes. */
struct  dinode zino[MAXBSIZE/sizeof(struct dinode)];
char	*fsys, *proto;
char token[MAXPATHLEN];
char name_token[MAXPATHLEN];

time_t	utime_val;
FILE *fin;
int	fsi,
	fso,
	ino = 3;
short   errs;
#ifdef LONGFILENAMES
int fsmagic;	/* FS_MAGIC or FS_MAGIC_LFN */
int fsfeatures;
int maxnamlen;	/* DIRSIZ_CONSTANT for orig HFS and MAXNAMLEN for LFN */
#endif /* LONGFILENAMES */

FILE *logfile;
#define	LOGFILE	"/etc/sbtab"
void log();
char message[160];

unsigned long getnum();
int nlink_adjust();

daddr_t	alloc();
daddr_t proto_alloc();

#ifdef __hp9000s800
/*Table converting a driver into the appropriate rotational delay*/
struct rotdelay_info
{
	char *name;
	int delay;
} rotdelay_info[] = {
	"disc0", ROTDELAY_HPIB,
	"disc1", ROTDELAY_NIO_HPIB,
	"disc2", ROTDELAY_ALINK,
	"disc3", ROTDELAY_SCSI,
	"autoch", ROTDELAY_AUTO,
	NULL, 0
};
#endif /* __hp9000s800 */

/*
 * Stuff to lookup the uid and gid in proto files from /etc/passwd
 * and /etc/group.
 */
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>

#define UID  1
#define GID  2

long getugid();

/*
 * The following variables are used to keep track of the current
 * path name while recursing through a prototype file in descend().
 * entry() uses this path to enter a path into a table that is used
 * by path_to_inum().  path_to_inum() is used to lookup inode numbers
 * when creating hard links.
 */
char proto_path[MAXPATHLEN];
char *end_pp = &proto_path[0];

void enter_path();
int path_to_inum();

int lfn_flag = 0;
int sfn_flag = 0;
int fflag = 0;

extern char *optarg;
extern int optind, opterr;

/* use find_min_fsize to figure out the minimun fragment size */
long find_min_fsize();

main(argc, argv)
	int argc;
	char *argv[];
{
	long cylno, rpos, blk, i, j, fssize, n, warn = 0;
	long inode_blks;
	int f, c;
	struct stat statarea;
	struct ustat ustatarea;
	short option;
	short	cpg_spec;		/* cylinders/group was specified */
	short	bpi_spec;		/* inode density was specified */
	short	cpg_mod;		/* cylinder group size modified */
	int	bpi;			/* bytes per inode */
	int	cg;			/* total number of cylinder groups */
	int	tot_i;			/* total number of inodes required */
	int	cg_req;			/* cylinder groups required */
	int	cg_size;		/* cylinder group size */
	int	maxipg;			/* maximum number of inodes that will
					   fit in cylinder group */
	long    min_fsize;              /* minimun fragment size */

#if defined(SecureWare) && defined(B1) && !defined(SEC_STANDALONE) && !defined(STANDALONE)
	if(ISB1){
	    set_auth_parameters(argc, argv);
	    if (!authorized_user("sysadmin")) {
		fprintf(stderr,
			"mkfs: you must have the 'sysadmin' authorization\n");
		exit(1);
	    }
	    initprivs();
	    forcepriv(SEC_ALLOWMACACCESS);
	    forcepriv(SEC_ALLOWDACACCESS);
	}
#endif
	option = 0;
#ifdef LONGFILENAMES
	fsmagic = 0;
	fsfeatures = 0;
#endif
#ifndef STANDALONE
	time(&utime_val);

#ifdef LONGFILENAMES
	while ((c = getopt(argc, argv, "LSF")) != EOF)
	{
		switch(c) {
			case 'L':
				lfn_flag = 1;
				break;

			case 'S':
				sfn_flag = 1;
				break;
			case 'F':
				fflag = 1;
				break;

			default:
				printf("usage: mkfs [ -L | -S ] [ -F ] special size \n");
				printf("            [nsect ntrak bsize fsize cpg minfree rps nbpi]\n");
				printf("   or  mkfs [ -L | -S ] [ -F ] special proto \n");
				printf("            [nsect ntrak bsize fsize cpg minfree rps nbpi]\n");
				exit(1);
		}
	}

	if (sfn_flag)
		fsmagic = FS_MAGIC;
	else if (lfn_flag)
		fsmagic = FS_MAGIC_LFN;

	if (fsmagic == 0)
	    fsmagic = default_magic();

#if defined(FD_FSMAGIC)
	if (fsmagic == FS_MAGIC_LFN)
		fsfeatures |= FSF_LFN;

	if ((fsmagic==FS_MAGIC_LFN) || (fsfeatures & FSF_LFN))
		maxnamlen = MAXNAMLEN;
	else maxnamlen = DIRSIZ_CONSTANT;
#else /* not new magic number */
	maxnamlen = fsmagic==FS_MAGIC_LFN?MAXNAMLEN:DIRSIZ_CONSTANT;
#endif /* new magic number */

	init_proto_dirs();
#endif /* LONGFILENAMES */

	/* check invocation */
	if (((optind+2) != argc) && ((optind+10) != argc)) {
#ifdef LONGFILENAMES
		printf("usage: mkfs [ -L | -S ] [ -F ] special size \n");
		printf("            [nsect ntrak bsize fsize cpg minfree rps nbpi]\n");
		printf("   or  mkfs [ -L | -S ] [ -F ] special proto \n");
		printf("            [nsect ntrak bsize fsize cpg minfree rps nbpi]\n");
#else /* not LONGFILENAMES */
		printf("usage: mkfs special size \n");
		printf("            [nsect ntrak bsize fsize cpg minfree rps nbpi]\n");
		printf("   or  mkfs special proto \n");
		printf("            [nsect ntrak bsize fsize cpg minfree rps nbpi]\n");
#endif /* LONGFILENAMES */
		exit(1);
	}
	if ((optind+10) == argc)
		option = 1;

	/* check validity of the specified special file; this file must exist */
	fsys = argv[optind++];
	if (stat(fsys, &statarea) < 0)
	{
		printf(" %s: cannot stat\n", fsys);
		exit(1);
	}
	if (!fflag) {
 		if (is_mounted(fsys,&statarea)) {
			printf("%s: mounted file system\n",fsys);
			exit(1);
 		}
 		if (is_swap(statarea.st_rdev)) {
			printf("%s: swap device\n",fsys);
			exit(1);
 		}
	}
	fso = creat(fsys, 0666); /*create new file or rewrite old file*/
	if(fso < 0) {
		printf("%s: cannot create\n", fsys);
		exit(1);
	}

	fsi = open(fsys, O_RDONLY);
	if(fsi < 0) {
		printf("%s: cannot open\n", fsys);
		exit(1);
	}

	/*
	 * Make sure that this disk is not part of an SDS array.  If so,
	 * print an error message and exit.  If the user really wants
	 * to destroy the array, he must run "/etc/sdsadmin -d <fsys>"
	 * We only do this if they gave the non-partitioned block or
	 * character special file.
	 */
	if ((S_ISCHR(statarea.st_mode) || S_ISBLK(statarea.st_mode)) &&
	    (statarea.st_rdev & 0x0f) == 0)
	{
	    static char cmd_str[] =
		"/etc/sdsadmin -l %s > /dev/null 2>&1";
	    char *cmd = malloc(strlen(fsys) + strlen(cmd_str) + 1);

	    if (cmd == (char *)0) {
		fputs("mkfs: out of memory\n", stderr);
		exit(1);
	    }
	    sprintf(cmd, cmd_str, fsys);
	    if (system(cmd) == 0) {
		fprintf(stderr, "mkfs: %s is part of an SDS array\n",
		    fsys);
		fprintf(stderr, "mkfs: use \"/etc/sdsadmin -d %s\" if you",
		    fsys);
		fprintf(stderr, " wish to destroy the array\n");
		exit(1);
	    }
	    free(cmd);
	}
		
	/* open the log file, if it exists */
	if (!access(LOGFILE, W_OK))
		logfile=fopen(LOGFILE, "a+");

	/* check which form of mkfs command is used */
	proto = argv[optind++];
	fin = fopen(proto, "r");
	if (fin == NULL)     /* first form is specified */
	{
		fssize = n = 0;
		for (f = 0; c = proto[f]; f++)
			if (isdigit(c))
				n = n*10 + (c- '0');
			else
				printf("%s:  cannot open\n", proto), exit(1);
		fssize = n;
		n = 0;
	}
	else    	/* second form is used: proto */
	{
	       /* get name of boot program and copy it to
	        * the boot area. Boot program "" is ignored.
                */
		getstr();
		if (strcmp(token, "\"\"") != 0 &&
		    sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO)
		{
		    int fd;
		    char buf[BBSIZE];

		    if ((fd = open(token, O_RDONLY)) < 0)
			printf("%s: cannot open\n", token);
		    else
		    {
			int size = lseek(fd, 0L, SEEK_END);

			if (size > BBSIZE)
			    printf("%s: too big\n", token);
			else
			{
			    /* read boot program */
			    lseek(fd, 0L, SEEK_SET);
			    read(fd, buf, size);

			    /* write boot-block to file system */
			    lseek(fso, 0L, SEEK_SET);
			    if (write(fso, buf, BBSIZE) != BBSIZE)
			    {
				fputs("write error: boot-block\n",
				    stderr);
				exit(1);
			    }
			}
			close(fd);
		    }
		}
		fssize = getnum();
	}  /* end of else proto */
#else /* STANDALONE */
	{
		static char protos[60];
		char fsbuf[100];

		printf("file sys size: ");
		gets(protos);
		fssize = atoi(protos);
		do {
			printf("file system: ");
			gets(fsbuf);
			fso = open(fsbuf, O_WRONLY);
			fsi = open(fsbuf, O_RDONLY);
		} while (fso < 0 || fsi < 0);
	}
	argc = 0;
#endif /* STANDALONE */

	/* check the specified size of file system and verify that
	 * the special device file is big enough to contain the
	 * new file system
   	 */
	if (fssize <= 0)
		printf("preposterous size %d\n", fssize), exit(1);

	/*
	 * Maximum file system size is 4GB - DEV_BSIZE, since this is the
         * maximum value which can be lseek'd to without wrapping around to
         * the beginning of the device.  Since fssize is in DEV_BSIZE units,
         * this means fssize cannot exceed 4MB - 1, or 4194303.
         */
#define FSSIZE_MAX     4194303
        if (fssize > FSSIZE_MAX) {
                printf("file system size %d greater than 4GB\n", fssize);
                exit(1);
        }

#if defined(SecureWare) && defined(B1)
	if(ISB1)
	    mkfs_set_type(&sblock);
#endif

	/* make sure that the fssize is not too large */
	{
		int k, unused;
		min_fsize=find_min_fsize(fsi);
		k=min_fsize / DEV_BSIZE;
		unused=(fssize - k ) % k;
		if (unused) {
			fssize-=unused;
		}
		wtfs(fssize - k, k * DEV_BSIZE, (char *)&sblock);
	}


	/*
	 * collect and verify the sector and track info...
	 */
	if (!option)
	{
		sblock.fs_nsect = DFLNSECT;
		sblock.fs_ntrak = DFLNTRAK;
	}
	else
	{
		sblock.fs_nsect = atoi(argv[optind++]);
		sblock.fs_ntrak = atoi(argv[optind++]);
	}
	if (sblock.fs_nsect <= 0)
	     printf("preposterous nsect %d\n",sblock.fs_nsect),exit(1);
	if (sblock.fs_ntrak <= 0)
		printf("preposterous ntrak %d\n",sblock.fs_ntrak),exit(1);
	sblock.fs_spc = sblock.fs_ntrak * sblock.fs_nsect;

	/*
	 * collect and verify the block and fragment sizes
	 */
	if (!option)
	{
		sblock.fs_bsize=DESBLKSIZE;
		sblock.fs_fsize=DESFRAGSIZE;
	}
	else
	{
		sblock.fs_bsize= atoi(argv[optind++]);
		sblock.fs_fsize= atoi(argv[optind++]);
	}
	if (!POWEROF2(sblock.fs_bsize)) {
		printf("block size must be a power of 2, not %d\n",
		    sblock.fs_bsize);
		exit(1);
	}
	if (!POWEROF2(sblock.fs_fsize)) {
		printf("fragment size must be a power of 2, not %d\n",
		    sblock.fs_fsize);
		exit(1);
	}
	if (sblock.fs_fsize < min_fsize) {
		printf("fragment size %d is too small, minimum is %d\n",
		sblock.fs_fsize, min_fsize);
		exit(1);
	}
	if (sblock.fs_bsize < MINBSIZE) {
		printf("block size %d is too small, minimum is %d\n",
		    sblock.fs_bsize, MINBSIZE);
		exit(1);
	}
	if (sblock.fs_fsize > SBSIZE){
		printf("fragment size %d is too big, maximum is %d\n",
		    sblock.fs_fsize, SBSIZE);
		exit(1);
	}
	if (sblock.fs_bsize >MAXBSIZE){
		printf("block size %d is too big, maximum is %d\n",
		    sblock.fs_bsize, MAXBSIZE);
		exit(1);
	}
	if (sblock.fs_bsize < sblock.fs_fsize) {
		printf("block size (%d) cannot be smaller than fragment size (%d)\n",
		    sblock.fs_bsize, sblock.fs_fsize);
		exit(1);
	}

	sblock.fs_bmask = ~(sblock.fs_bsize - 1);
	sblock.fs_fmask = ~(sblock.fs_fsize - 1);
	for (sblock.fs_bshift = 0, i = sblock.fs_bsize; i > 1; i >>= 1)
		sblock.fs_bshift++;
	for (sblock.fs_fshift = 0, i = sblock.fs_fsize; i > 1; i >>= 1)
		sblock.fs_fshift++;
	sblock.fs_frag = numfrags(&sblock, sblock.fs_bsize);
	if (sblock.fs_frag > MAXFRAG) {
		printf("fragment size %d is too small, minimum with block size %d is %d\n",
		    sblock.fs_fsize, sblock.fs_bsize,
		    sblock.fs_bsize / MAXFRAG);
		exit(1);
	}
	for (sblock.fs_fragshift = 0, i = sblock.fs_frag; i > 1; i >>= 1)
		sblock.fs_fragshift++;
	sblock.fs_nindir = sblock.fs_bsize / sizeof(daddr_t);
#if defined(SecureWare) && defined(B1)
	if(ISB1)
		sblock.fs_inopb = sblock.fs_bsize / disk_dinode_size();
	else
		sblock.fs_inopb = sblock.fs_bsize / sizeof(struct dinode);
#else
	sblock.fs_inopb = sblock.fs_bsize / sizeof(struct dinode);
#endif
	sblock.fs_nspf = sblock.fs_fsize / DEV_BSIZE;
	for (sblock.fs_fsbtodb = 0, i = sblock.fs_nspf; i > 1; i >>= 1)
		sblock.fs_fsbtodb++;
	sblock.fs_sblkno =
	    roundup(howmany(BBSIZE + SBSIZE, sblock.fs_fsize), sblock.fs_frag);
	sblock.fs_cblkno = (daddr_t)(sblock.fs_sblkno +
	    roundup(howmany(SBSIZE, sblock.fs_fsize), sblock.fs_frag));
	sblock.fs_iblkno = sblock.fs_cblkno + sblock.fs_frag;
	sblock.fs_cgoffset = roundup(
	    howmany(sblock.fs_nsect, sblock.fs_fsize / DEV_BSIZE),
	    sblock.fs_frag);
	sblock.fs_cpc = NSPB(&sblock);
	if((sblock.fs_cpc == 64)  &&	   /* 64K block size requested */
	   ((sblock.fs_spc % 2) == 1)) {   /* ns and nt are odd */
	    sblock.fs_ntrak = sblock.fs_ntrak + 1;
	    sblock.fs_spc = sblock.fs_ntrak * sblock.fs_nsect;
	}
	for (sblock.fs_cgmask = 0xffffffff, i = sblock.fs_ntrak; i > 1; i >>= 1)
		sblock.fs_cgmask <<= 1;
	if (!POWEROF2(sblock.fs_ntrak))
		sblock.fs_cgmask <<= 1;
	for (sblock.fs_cpc = NSPB(&sblock), i = sblock.fs_spc;
	     sblock.fs_cpc > 1 && (i & 1) == 0;
	     sblock.fs_cpc >>= 1, i >>= 1)
		/* void */;
	if (sblock.fs_cpc > MAXCPG) {
		printf("maximum block size with nsect %d and ntrak %d is %d\n",
		    sblock.fs_nsect, sblock.fs_ntrak,
		    sblock.fs_bsize / (sblock.fs_cpc / MAXCPG));
		exit(1);
	}
	/*
	 * Now have size for file system and nsect and ntrak.
	 * Determine number of cylinders and blocks in the file system.
	 */
	sblock.fs_size = fssize = dbtofsb(&sblock, fssize);
	sblock.fs_ncyl = fssize * NSPF(&sblock) / sblock.fs_spc;
	if (fssize * NSPF(&sblock) > sblock.fs_ncyl * sblock.fs_spc) {
		sblock.fs_ncyl++;
		warn = 1;
	}
	if (sblock.fs_ncyl < 1) {
		printf("file systems must have at least one cylinder\n");
		exit(1);
	}
	/*
	 * collect and verify the number of cylinders per group
	 */
	cpg_spec = 0;
	bpi_spec = 0;
	cpg_mod = 0;
	if (!option)   /* default value */
	{
		sblock.fs_cpg = MAX(sblock.fs_cpc, DESCPG);
		/*
		 * if fs_cpg is not a multiple of cpc, we can change it.
		 * (Since the user has not specifically requested a value.)
		 * This may happen using either a 64k or 32k file system.
		 */
		if (sblock.fs_cpg % sblock.fs_cpc != 0)
		    sblock.fs_cpg = sblock.fs_cpc;
		sblock.fs_fpg = (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
		while (sblock.fs_fpg / sblock.fs_frag > MAXBPG(&sblock) &&
		    sblock.fs_cpg > sblock.fs_cpc) {
			sblock.fs_cpg -= sblock.fs_cpc;
			sblock.fs_fpg =
			    (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);
		}
	}
	else  /* take the given value */
	{
		/*
		 * it may not be possible for mkfs to create a file system that
		 * satisfies both the cylinder group size and inode density
		 * specified.  in order for mkfs to know which of these two
		 * parameters should be given priority in any necessary
		 * trade-off, an internal convention between newfs and mkfs has
		 * been established.  if either of these parameters is negative,
		 * it means that the parameter was specified by the user in
		 * newfs and thus should not be modified in mkfs.
		 */

		sblock.fs_cpg = atoi(argv[optind++]);
		if (sblock.fs_cpg < 0) {
			sblock.fs_cpg *= -1;
			cpg_spec = 1;
		}
		if (sblock.fs_cpg == 0) {
			printf("0 is an invalid value for cylinders/group\n");
			exit(1);
		}
		/*
		 * If the user did not specify a value for cpg (default value
		 * from mkfs), we can still change it to accommodate the
		 * 64k and 32k file system
		 */
		if ((sblock.fs_cpg % sblock.fs_cpc != 0) && (!cpg_spec))
		    sblock.fs_cpg = sblock.fs_cpc;
		sblock.fs_fpg = (sblock.fs_cpg * sblock.fs_spc) / NSPF(&sblock);

		/*
		 * see if we will be able to accommodate the requested number
		 * of inodes with the specified cylinder group size.  if not,
		 * and if the user specified the inode density and not the
		 * cylinder group size, adjust the cylinder group size to
		 * allow more inodes.
		 */

		bpi = atoi(argv[optind+2]);
		if (bpi < 0) {
			bpi *= -1;
			bpi_spec = 1;
		}
		if ((bpi_spec == 1) && (cpg_spec == 0)) {

			/* we calculate inodes on a per cylinder group basis.
			 * for very small file systems, we must use the file
			 * system size to avoid allocating too many inodes.
			 */
			inode_blks = (sblock.fs_fpg < fssize) ? sblock.fs_fpg : fssize;
			inode_blks = (inode_blks - sblock.fs_iblkno) /
					sblock.fs_frag;
			inode_blks /= 1 +
				((bpi * INOPB(&sblock)) / sblock.fs_bsize);
			if (inode_blks == 0)
				inode_blks = 1;
			cg = sblock.fs_ncyl / sblock.fs_cpg;
			tot_i = inode_blks * INOPB(&sblock) * cg;
			maxipg = ((sblock.fs_fpg - sblock.fs_iblkno) /
				sblock.fs_frag) * INOPB(&sblock);
			if (maxipg > MAXIPG)
				maxipg = MAXIPG;
			cg_req = tot_i / maxipg;

			/* only make change if off by at least 10 percent */
			if (cg_req > cg + cg/10) {
				cg_size = sblock.fs_ncyl / cg_req;
				if ((cg_size % sblock.fs_cpc) || (cg_size == 0))
					if (cg_size < sblock.fs_cpc)
						cg_size = sblock.fs_cpc;
					else
						cg_size -= cg_size % sblock.fs_cpc;
				if (cg_size < sblock.fs_cpg) {
					cpg_mod = 1;
					sblock.fs_cpg = cg_size;
					sblock.fs_fpg = (sblock.fs_cpg *
						sblock.fs_spc)/NSPF(&sblock);
				}
			}
		}
	}
	if (sblock.fs_cpg < 1) {
		printf("cylinder groups must have at least 1 cylinder\n");
		exit(1);
	}
	if (sblock.fs_cpg > MAXCPG) {
		printf("cylinder groups are limited to %d cylinders\n", MAXCPG);
		exit(1);
	}
	if (sblock.fs_cpg % sblock.fs_cpc != 0) {
		printf("cylinder groups must have a multiple of %d cylinders\n",
		    sblock.fs_cpc);
		exit(1);
	}
	/*
	 * determine feasability/values of rotational layout tables
	 */
	if (sblock.fs_ntrak == 1) {
		sblock.fs_cpc = 0;
		goto next;
	}
	if (sblock.fs_spc * sblock.fs_cpc > MAXBPC * NSPB(&sblock) ||
	    sblock.fs_nsect > (1 << NBBY) * NSPB(&sblock)) {
		printf("%s %s %d %s %d.%s",
		    "Warning: insufficient space in super block for\n",
		    "rotational layout tables with nsect", sblock.fs_nsect,
		    "and ntrak", sblock.fs_ntrak,
		    "\nFile system performance may be impaired.\n");
		sblock.fs_cpc = 0;
		goto next;
	}
	/*
	 * calculate the available blocks for each rotational position
	 */
	for (cylno = 0; cylno < MAXCPG; cylno++)
		for (rpos = 0; rpos < NRPOS; rpos++)
			sblock.fs_postbl[cylno][rpos] = -1;
	blk = sblock.fs_spc * sblock.fs_cpc / NSPF(&sblock);
	for (i = 0; i < blk; i += sblock.fs_frag)
		/* void */;
	for (i -= sblock.fs_frag; i >= 0; i -= sblock.fs_frag) {
		cylno = cbtocylno(&sblock, i);
		rpos = cbtorpos(&sblock, i);
		blk = i / sblock.fs_frag;
		if (sblock.fs_postbl[cylno][rpos] == -1)
			sblock.fs_rotbl[blk] = 0;
		else
			sblock.fs_rotbl[blk] =
			    sblock.fs_postbl[cylno][rpos] - blk;
		sblock.fs_postbl[cylno][rpos] = blk;
	}
next:
	/*
	 * Validate specified/determined cpg.
	 */
	if (sblock.fs_spc > MAXBPG(&sblock) * NSPB(&sblock)) {
		printf("too many sectors per cylinder (%d sectors)\n",
		    sblock.fs_spc);
		while(sblock.fs_spc > MAXBPG(&sblock) * NSPB(&sblock)) {
			sblock.fs_bsize <<= 1;
			if (sblock.fs_frag < MAXFRAG)
				sblock.fs_frag <<= 1;
			else
				sblock.fs_fsize <<= 1;
		}
		printf("nsect %d, and ntrak %d, requires block size of %d,\n",
		    sblock.fs_nsect, sblock.fs_ntrak, sblock.fs_bsize);
		printf("\tand fragment size of %d\n", sblock.fs_fsize);
		exit(1);
	}
	if (sblock.fs_fpg > MAXBPG(&sblock) * sblock.fs_frag) {
		printf("cylinder group too large (%d cylinders); ",
		    sblock.fs_cpg);
		printf("max: %d cylinders per group\n",
		    MAXBPG(&sblock) * sblock.fs_frag /
		    (sblock.fs_fpg / sblock.fs_cpg));
		exit(1);
	}
	sblock.fs_cgsize = fragroundup(&sblock,
	    sizeof(struct cg) + howmany(sblock.fs_fpg, NBBY));
	/*
	 * Compute/validate number of cylinder groups.
	 */
	sblock.fs_ncg = sblock.fs_ncyl / sblock.fs_cpg;
	if (sblock.fs_ncyl % sblock.fs_cpg)
		sblock.fs_ncg++;
	if ((sblock.fs_spc * sblock.fs_cpg) % NSPF(&sblock)) {
		printf("mkfs: nsect %d, ntrak %d, cpg %d is not tolerable\n",
		    sblock.fs_nsect, sblock.fs_ntrak, sblock.fs_cpg);
		printf("as this would have cyl groups whose size\n");
		printf("is not a multiple of %d; choke!\n", sblock.fs_fsize);
		exit(1);
	}

	if (!option)
		sblock.fs_minfree = MINFREE;
	else
	{
		sblock.fs_minfree = atoi(argv[optind++]);
		if (sblock.fs_minfree < 0 || sblock.fs_minfree > 99) {
			printf("%s: bogus minfree reset to %d%%\n", sblock.fs_minfree,
				MINFREE);
			sblock.fs_minfree = MINFREE;
		}
	}

	if (!option)
		sblock.fs_rps = DEFHZ;
	else
		sblock.fs_rps = atoi(argv[optind++]);

	/*
	 * Compute number of inode blocks per cylinder group.
	 * Start with one inode per NBPI bytes; adjust as necessary.
	 */
	bpi = MAX(NBPI, sblock.fs_fsize);
	if (option)   /*get the given value*/
	{
		i = atoi(argv[optind]);
		if (i < 0)
			bpi = -i;
		else if (i == 0)
			printf("%s: bogus nbpi reset to %d\n", argv[optind], bpi);
		else
			bpi = i;
		optind++;
	}

	inode_blks = (sblock.fs_fpg < fssize) ? sblock.fs_fpg : fssize;
	inode_blks = (inode_blks - sblock.fs_iblkno) / sblock.fs_frag;
	inode_blks /= 1 + ((bpi * INOPB(&sblock)) / sblock.fs_bsize);
	if (inode_blks == 0)
		inode_blks = 1;
	sblock.fs_ipg = inode_blks * INOPB(&sblock);
	if (sblock.fs_ipg > MAXIPG) {
		sblock.fs_ipg = MAXIPG;

		/* tell user that not enough inodes could be supplied */

		if ((bpi_spec == 1) && (cpg_spec == 0)) {
			printf("could not provide as many inodes as requested due to fundamental file\n");
			printf("system restrictions.  the maximum possible number of inodes has been provided\n");
		} else if (bpi_spec == 1) {
			printf("could not provide the specified number of inodes with the current cylinder\n");
			printf("group size (%d).  it may be possible to get more inodes by specifying\n", sblock.fs_cpg);
			printf("smaller cylinder groups.\n\n");
		}
	}
#if defined(SecureWare) && defined(B1)
        /* secure inodes do not fit exactly into a fragment. */
	if(ISB1)
            sblock.fs_dblkno = sblock.fs_iblkno +
	        (howmany(sblock.fs_ipg, INOPB(&sblock)) * sblock.fs_frag);
	else
	    sblock.fs_dblkno = sblock.fs_iblkno+sblock.fs_ipg / INOPF(&sblock);
#else
	sblock.fs_dblkno = sblock.fs_iblkno + sblock.fs_ipg / INOPF(&sblock);
#endif
	i = MIN(~sblock.fs_cgmask, sblock.fs_ncg - 1);
	if (cgdmin(&sblock, i) - cgbase(&sblock, i) >= sblock.fs_fpg) {
		printf("inode blocks/cyl group (%d) >= data blocks (%d)\n",
		    cgdmin(&sblock, i) - cgbase(&sblock, i) / sblock.fs_frag,
		    sblock.fs_fpg / sblock.fs_frag);
		printf("number of cylinders per cylinder group must be increased\n");
		if (cpg_mod)
			printf("or number of inodes requested must be reduced\n");
		exit(1);
	}
	j = sblock.fs_ncg - 1;
	if ((i = fssize - j * sblock.fs_fpg) < sblock.fs_fpg &&
	    cgdmin(&sblock, j) - cgbase(&sblock, j) > i) {
		printf("Warning: inode blocks/cyl group (%d) >= data blocks (%d) in last\n",
		    (cgdmin(&sblock, j) - cgbase(&sblock, j)) / sblock.fs_frag,
		    i / sblock.fs_frag);
		printf("    cylinder group. This implies %d sector(s) cannot be allocated.\n",
		    i * NSPF(&sblock));
		sblock.fs_ncg--;
		sblock.fs_ncyl -= sblock.fs_ncyl % sblock.fs_cpg;
		sblock.fs_size = fssize = sblock.fs_ncyl * sblock.fs_spc /
		    NSPF(&sblock);
		warn = 0;
	}
	if (warn) {
		printf("Warning: %d sector(s) in last cylinder unallocated\n",
		    sblock.fs_spc -
		    (fssize * NSPF(&sblock) - (sblock.fs_ncyl - 1)
		    * sblock.fs_spc));
	}
	/*
	 * fill in remaining fields of the super block
	 */
	sblock.fs_csaddr = cgdmin(&sblock, 0);
	sblock.fs_cssize =
	    fragroundup(&sblock, sblock.fs_ncg * sizeof(struct csum));

#ifdef __hp9000s800
	if (sysconf(_SC_IO_TYPE) == IO_TYPE_SIO) 
	/* 
	 * This feature is only supported for S800s. 
	 * DSDe406581
	 */
	{
/* 
 * this block is for extendfs support
 */
		int	size4G;
		int	ncg4G;
		int	cssize4G;

			/* number of frags for 4GB */
		size4G = 1 << (32 - sblock.fs_fshift);

		ncg4G = size4G / sblock.fs_fpg;

		if (size4G != ncg4G * sblock.fs_fpg)
			++ncg4G;

		cssize4G = fragroundup(&sblock, ncg4G * sizeof(struct csum));
#ifdef	DEBUG
		printf("** change cssize from %d to %d for 4GB expansion.\n",
			sblock.fs_cssize, cssize4G);
#endif	
		sblock.fs_cssize = cssize4G;
	}
#endif /* __hp9000s800 */

	i = sblock.fs_bsize / sizeof(struct csum);
	sblock.fs_csmask = ~(i - 1);
	for (sblock.fs_csshift = 0; i > 1; i >>= 1)
		sblock.fs_csshift++;
	i = sizeof(struct fs) +
		howmany(sblock.fs_spc * sblock.fs_cpc, NSPB(&sblock));
	sblock.fs_sbsize = fragroundup(&sblock, i);
	fscs = (struct csum *)calloc(1, sblock.fs_cssize);
#ifdef LONGFILENAMES
	sblock.fs_magic = fsmagic;
#if defined(FD_FSMAGIC)
	sblock.fs_featurebits = fsfeatures;
#endif /* new magic number */
#else
	sblock.fs_magic = FS_MAGIC;
#endif /* LONGFILENAMES */

	sblock.fs_rotdelay = get_rot_delay(statarea.st_rdev,
					   statarea.st_mode & S_IFMT);
	sblock.fs_maxcontig = MAXCONTIG;
	sblock.fs_maxbpg = MAXBLKPG(&sblock);
	sblock.fs_cgrotor = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_fmod = 0;
	sblock.fs_ronly = 0;

	/* mark file system as bad and write out super block.  this will
	 * prevent file system from being mounted if mkfs is aborted before
	 * file system is completely built.  super block will be rewritten
	 * later with fs_clean bit indicating file system is correct.
	 */
	sblock.fs_clean = FS_NOTOK;
	wtfs(SBLOCK, SBSIZE, (char *)&sblock);

	/*
	 * Dump out summary information about file system.
	 */
	printf("%s:\t%d sectors in %d cylinders of %d tracks, %d sectors\n",
	    fsys, sblock.fs_size * NSPF(&sblock), sblock.fs_ncyl,
	    sblock.fs_ntrak, sblock.fs_nsect);
	printf("\t%.1fMb in %d cyl groups (%d c/g, %.2fMb/g, %d i/g)\n",
	    (float)sblock.fs_size * sblock.fs_fsize * 1e-6, sblock.fs_ncg,
	    sblock.fs_cpg, (float)sblock.fs_fpg * sblock.fs_fsize * 1e-6,
	    sblock.fs_ipg);
	/*
	 * Now build the cylinders group blocks and
	 * then print out indices of cylinder groups.
	 */
	printf("super-block backups (for fsck -b#) at:");
	sprintf(message, "\n%s: super-block backups (for fsck -b#) at:", fsys);
	log(message);
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++) {
		initcg(cylno);
		if (cylno % 10 == 0) {
			printf("\n");
			log("\n");
		}
		sprintf(message, " %d,", fsbtodb(&sblock, cgsblock(&sblock, cylno)));
		printf(message);
		log(message);
	}
	printf("\n");
	log("\n");
	/*
	 * Construct the initial file system with lost+found directory;
	 * Now if mkfs proto, construct the file system according
	 * to the information contained in the proto file;
         * then write out the super block
	 */
	fsinit();
	if (fin)
	{
		fs = &sblock;
		/*
		 * Call getstr() before calling descend().  descend()
		 * expects the 'mode' field to be read in already.
		 */
		getstr();
		descend((struct inode *)0);
	}
	sblock.fs_time = utime_val;
	sblock.fs_clean = FS_CLEAN;
	wtfs(SBLOCK, SBSIZE, (char *)&sblock);
	for (i = 0; i < sblock.fs_cssize; i += sblock.fs_bsize)
		wtfs(fsbtodb(&sblock, sblock.fs_csaddr + numfrags(&sblock, i)),
			sblock.fs_cssize - i < sblock.fs_bsize ?
			    sblock.fs_cssize - i : sblock.fs_bsize,
			((char *)fscs) + i);
	/*
	 * Write out the duplicate super blocks
	 */
	for (cylno = 0; cylno < sblock.fs_ncg; cylno++)
		wtfs(fsbtodb(&sblock, cgsblock(&sblock, cylno)),
		    SBSIZE, (char *)&sblock);
#ifndef STANDALONE
	exit(0);
#endif
}

/*
 * Initialize a cylinder group.
 */
initcg(cylno)
int cylno;
{
	daddr_t cbase, d, dlower, dupper, dmax;
	long i, j, s;
	register struct csum *cs;

	/*
	 * Determine block bounds for cylinder group.
	 * Allow space for super block summary information in first
	 * cylinder group.
	 */
	cbase = cgbase(&sblock, cylno);
	dmax = cbase + sblock.fs_fpg;
	if (dmax > sblock.fs_size)
		dmax = sblock.fs_size;
	dlower = cgsblock(&sblock, cylno) - cbase;
	dupper = cgdmin(&sblock, cylno) - cbase;
	cs = fscs + cylno;
	acg.cg_time = utime_val;
	acg.cg_magic = CG_MAGIC;
	acg.cg_cgx = cylno;
	if (cylno == sblock.fs_ncg - 1)
		acg.cg_ncyl = sblock.fs_ncyl % sblock.fs_cpg;
	else
		acg.cg_ncyl = sblock.fs_cpg;
	acg.cg_niblk = sblock.fs_ipg;
	acg.cg_ndblk = dmax - cbase;
	acg.cg_cs.cs_ndir = 0;
	acg.cg_cs.cs_nffree = 0;
	acg.cg_cs.cs_nbfree = 0;
	acg.cg_cs.cs_nifree = 0;
	acg.cg_rotor = 0;
	acg.cg_frotor = 0;
	acg.cg_irotor = 0;
	for (i = 0; i < sblock.fs_frag; i++) {
		acg.cg_frsum[i] = 0;
	}
#ifdef TRUX
	/* Re-coding this algorithm because for secure inodes
	 * fs_ipg is not an even multiple of INOPB
	 */
	for (i = 0; i < sblock.fs_ipg; i++ ) {
		clrbit(acg.cg_iused, i);
		acg.cg_cs.cs_nifree ++;
	}
#else
	for (i = 0; i < sblock.fs_ipg; ) {
		for (j = INOPB(&sblock); j > 0; j--) {
			clrbit(acg.cg_iused, i);
			i++;
		}
		acg.cg_cs.cs_nifree += INOPB(&sblock);
	}
#endif /* TRUX */
	if (cylno == 0)
		for (i = 0; i < ROOTINO; i++) {
			setbit(acg.cg_iused, i);
			acg.cg_cs.cs_nifree--;
		}
	while (i < MAXIPG) {
		clrbit(acg.cg_iused, i);
		i++;
	}
	lseek(fso, fsbtodb(&sblock, cgimin(&sblock, cylno)) * DEV_BSIZE, SEEK_SET);
#if defined(SecureWare) && defined(B1)
	{
	    int  write_error = 0;

	    if(ISB1){
	    	if (mkfs_write_inodes(fso, &sblock))
	    	    write_error = 1;
	    }
	    else {
		int blocks_to_write, bytes_remaining;

		blocks_to_write = (sblock.fs_ipg * sizeof (struct dinode)) / MAXBSIZE;
		bytes_remaining = (sblock.fs_ipg * sizeof (struct dinode)) % MAXBSIZE;

		for (i = 0; i < blocks_to_write; i++)
		    if (write(fso, (char *)zino, MAXBSIZE) != MAXBSIZE)
		    {
		    	write_error = 1;
		    	break;
		    }

	    	if ((write_error == 0) && (bytes_remaining != 0))
		    if (write(fso, (char *)zino, bytes_remaining) != bytes_remaining)
		    	write_error = 1;
	    }
	    if (write_error)
		printf("write error %ld\n",
		    numfrags(&sblock, lseek(fso, 0L, SEEK_CUR)));
	}
#else /* defined(SecureWare) && defined(B1) */
	{
	    int blocks_to_write, bytes_remaining, write_error = 0;

	    blocks_to_write = (sblock.fs_ipg * sizeof (struct dinode)) / MAXBSIZE;
	    bytes_remaining = (sblock.fs_ipg * sizeof (struct dinode)) % MAXBSIZE;

	    for (i = 0; i < blocks_to_write; i++)
		if (write(fso, (char *)zino, MAXBSIZE) != MAXBSIZE)
		{
		    write_error = 1;
		    break;
		}

	    if ((write_error == 0) && (bytes_remaining != 0))
		if (write(fso, (char *)zino, bytes_remaining) != bytes_remaining)
		    write_error = 1;

	    if (write_error)
		printf("write error %ld\n",
		    numfrags(&sblock, lseek(fso, 0L, SEEK_CUR)));
	}

#endif /* defined(SecureWare) && defined(B1) */

	for (i = 0; i < MAXCPG; i++) {
		acg.cg_btot[i] = 0;
		for (j = 0; j < NRPOS; j++)
			acg.cg_b[i][j] = 0;
	}
	if (cylno == 0) {
		/*
		 * reserve space for summary info and Boot block
		 */
		dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
		for (d = 0; d < dlower; d += sblock.fs_frag)
			clrblock(&sblock, acg.cg_free, d/sblock.fs_frag);
	} else {
		for (d = 0; d < dlower; d += sblock.fs_frag) {
			setblock(&sblock, acg.cg_free, d/sblock.fs_frag);
			acg.cg_cs.cs_nbfree++;
			acg.cg_btot[cbtocylno(&sblock, d)]++;
			acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]++;
		}
		sblock.fs_dsize += dlower;
	}
	sblock.fs_dsize += acg.cg_ndblk - dupper;
	for (; d < dupper; d += sblock.fs_frag)
		clrblock(&sblock, acg.cg_free, d/sblock.fs_frag);
	if (d > dupper) {
		acg.cg_frsum[d - dupper]++;
		for (i = d - 1; i >= dupper; i--) {
			setbit(acg.cg_free, i);
			acg.cg_cs.cs_nffree++;
		}
	}
	while ((d + sblock.fs_frag) <= dmax - cbase) {
		setblock(&sblock, acg.cg_free, d/sblock.fs_frag);
		acg.cg_cs.cs_nbfree++;
		acg.cg_btot[cbtocylno(&sblock, d)]++;
		acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]++;
		d += sblock.fs_frag;
	}
	if (d < dmax - cbase) {
		acg.cg_frsum[dmax - cbase - d]++;
		for (; d < dmax - cbase; d++) {
			setbit(acg.cg_free, d);
			acg.cg_cs.cs_nffree++;
		}
		for (; d % sblock.fs_frag != 0; d++)
			clrbit(acg.cg_free, d);
	}
	for (d /= sblock.fs_frag; d < MAXBPG(&sblock); d ++)
		clrblock(&sblock, acg.cg_free, d);
	sblock.fs_cstotal.cs_ndir += acg.cg_cs.cs_ndir;
	sblock.fs_cstotal.cs_nffree += acg.cg_cs.cs_nffree;
	sblock.fs_cstotal.cs_nbfree += acg.cg_cs.cs_nbfree;
	sblock.fs_cstotal.cs_nifree += acg.cg_cs.cs_nifree;
	*cs = acg.cg_cs;
	wtfs(fsbtodb(&sblock, cgtod(&sblock, cylno)),
		sblock.fs_bsize, (char *)&acg);
}

/*
 * initialize the file system
 */
struct inode node;
#define PREDEFDIR 3
#ifdef LONGFILENAMES
struct direct root_dir[3];
struct direct lost_found_dir[3];

int
init_proto_dirs()
{
	root_dir[0].d_ino = ROOTINO;
#if defined(FD_FSMAGIC)
	if ((fsmagic==FS_MAGIC_LFN ) || (fsfeatures & FSF_LFN))
		root_dir[0].d_reclen = 12;
	else
		root_dir[0].d_reclen = DIRSTRCTSIZ;
#else /* not new magic number */
	root_dir[0].d_reclen = fsmagic==FS_MAGIC_LFN?12:DIRSTRCTSIZ;
#endif /* new magic number */
	root_dir[0].d_namlen = 1;
	strcpy(root_dir[0].d_name,".");

	root_dir[1].d_ino = ROOTINO;
#if defined(FD_FSMAGIC)
	if ((fsmagic==FS_MAGIC_LFN ) || (fsfeatures & FSF_LFN))
		root_dir[0].d_reclen = 12;
	else
		root_dir[0].d_reclen = DIRSTRCTSIZ;
#else /* not new magic number */
	root_dir[0].d_reclen = fsmagic==FS_MAGIC_LFN?12:DIRSTRCTSIZ;
#endif /* new magic number */
	root_dir[1].d_namlen = 2;
	strcpy(root_dir[1].d_name,"..");

	root_dir[2].d_ino = LOSTFOUNDINO;
#if defined(FD_FSMAGIC)
	if ((fsmagic==FS_MAGIC_LFN ) || (fsfeatures & FSF_LFN))
		root_dir[0].d_reclen = 12;
	else
		root_dir[0].d_reclen = DIRSTRCTSIZ;
#else /* not new magic number */
	root_dir[0].d_reclen = fsmagic==FS_MAGIC_LFN?12:DIRSTRCTSIZ;
#endif /* new magic number */
	root_dir[2].d_namlen = 10;
	strcpy(root_dir[2].d_name,"lost+found");

	lost_found_dir[0].d_ino = LOSTFOUNDINO;
#if defined(FD_FSMAGIC)
	if ((fsmagic==FS_MAGIC_LFN ) || (fsfeatures & FSF_LFN))
		root_dir[0].d_reclen = 12;
	else
		root_dir[0].d_reclen = DIRSTRCTSIZ;
#else /* not new magic number */
	root_dir[0].d_reclen = fsmagic==FS_MAGIC_LFN?12:DIRSTRCTSIZ;
#endif /* new magic number */
	lost_found_dir[0].d_namlen = 1;
	strcpy(lost_found_dir[0].d_name,".");

	lost_found_dir[1].d_ino = ROOTINO;
#if defined(FD_FSMAGIC)
	if ((fsmagic==FS_MAGIC_LFN ) || (fsfeatures & FSF_LFN))
		root_dir[0].d_reclen = 12;
	else
		root_dir[0].d_reclen = DIRSTRCTSIZ;
#else /* not new magic number */
	root_dir[0].d_reclen = fsmagic==FS_MAGIC_LFN?12:DIRSTRCTSIZ;
#endif /* new magic number */
	lost_found_dir[1].d_namlen = 2;
	strcpy(lost_found_dir[1].d_name,"..");

	lost_found_dir[2].d_ino = 0;
	lost_found_dir[2].d_reclen = DIRBLKSIZ;
	lost_found_dir[2].d_namlen = 0;
	lost_found_dir[2].d_name[0] = '\0';
}
#else /* not LONGFILENAMES */
struct direct root_dir[] = {
	{ ROOTINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" }
};
struct direct lost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), 1, "." },
	{ ROOTINO, sizeof(struct direct), 2, ".." },
	{ 0, DIRBLKSIZ, 0, 0 }
};
#endif /* LONGFILENAMES */
char dbuf[MAXBSIZE];

fsinit()
{
	int i;

	/*
	 * initialize the node
	 */
	node.i_atime = utime_val;
	node.i_mtime = utime_val;
	node.i_ctime = utime_val;
	/*
	 * create the lost+found directory
	 */
	(void)makedir(lost_found_dir, 2);
	for (i = DIRBLKSIZ; i < sblock.fs_bsize; i += DIRBLKSIZ)
#ifdef LONGFILENAMES
#if defined(FD_FSMAGIC)
		if ((fsmagic == FS_MAGIC_LFN) || (fsfeatures & FSF_LFN))
#else /* not new magic number */
		if (fsmagic == FS_MAGIC_LFN)
#endif /* new magic number */
			memcpy(&dbuf[i], &lost_found_dir[2], DIRSIZ(&lost_found_dir[2]));
		else
			memcpy(&dbuf[i], &lost_found_dir[2], DIRSTRCTSIZ);
#else /* not LONGFILENAMES */
		memcpy(&dbuf[i], &lost_found_dir[2], sizeof(struct direct));
#endif /* LONGFILENAMES */
	node.i_number = LOSTFOUNDINO;
	node.i_mode = IFDIR | UMASK;
	node.i_nlink = 2;
	node.i_size = sblock.fs_bsize;
	node.i_db[0] = alloc(node.i_size, node.i_mode);
#ifdef ACLS
	node.i_contin = 0;
#endif /* ACLS */
	node.i_blocks = btodb(fragroundup(&sblock, node.i_size));
	wtfs(fsbtodb(&sblock, node.i_db[0]), node.i_size, dbuf);
	iput(&node);

	/*
	 * create the root directory
	 */
	node.i_number = ROOTINO;
	node.i_mode = IFDIR | UMASK;
	node.i_nlink = PREDEFDIR;
	node.i_size = makedir(root_dir, PREDEFDIR);
	node.i_db[0] = alloc(sblock.fs_fsize, node.i_mode);
#ifdef ACLS
	node.i_contin = 0;
#endif /* ACLS */
	node.i_blocks = btodb(fragroundup(&sblock, node.i_size));
	wtfs(fsbtodb(&sblock, node.i_db[0]), sblock.fs_fsize, dbuf);
	iput(&node);

	/*
	 * Enter the fixed paths into the (path,inode) table used
	 * by path_to_inum().
	 */
	enter_path(ROOTINO, ".");
	enter_path(ROOTINO, "..");
	enter_path(LOSTFOUNDINO, "lost+found");
	enter_path(LOSTFOUNDINO, "lost+found/.");
	enter_path(ROOTINO, "lost+found/..");
}

/*
 * construct a set of directory entries in "dbuf".
 * return size of directory.
 */
makedir(protodir, entries)
	register struct direct *protodir;
	int entries;
{
	char *cp;
	int i, spcleft;

	spcleft = DIRBLKSIZ;
	for (cp = dbuf, i = 0; i < entries - 1; i++) {
#ifdef LONGFILENAMES
#if defined(FD_FSMAGIC)
		if ((fsmagic == FS_MAGIC_LFN) || (fsfeatures & FSF_LFN))
#else /* not new magic number */
		if (fsmagic == FS_MAGIC_LFN)
#endif /* new magic number */
			protodir[i].d_reclen = DIRSIZ(&protodir[i]);
		else
			protodir[i].d_reclen = DIRSTRCTSIZ;
#else /* not LONGFILENAMES */
		protodir[i].d_reclen = sizeof(struct direct);
#endif /* LONGFILENAMES */
		memcpy(cp, &protodir[i],  protodir[i].d_reclen);
		cp += protodir[i].d_reclen;
		spcleft -= protodir[i].d_reclen;
	}
	protodir[i].d_reclen = spcleft;
#ifdef LONGFILENAMES
#if defined(FD_FSMAGIC)
	if ((fsmagic == FS_MAGIC_LFN) || (fsfeatures & FSF_LFN))
#else /* not new magic number */
	if (fsmagic == FS_MAGIC_LFN)
#endif /* new magic number */
	{
		memcpy(cp, &protodir[i], DIRSIZ(&protodir[i]));
		cp += DIRSIZ(&protodir[i]);
	}
	else
	{
		memcpy(cp, &protodir[i], DIRSTRCTSIZ);
		cp += DIRSTRCTSIZ;
	}
#else /* not LONGFILENAMES */
	memcpy(cp, &protodir[i], sizeof(struct direct));
	cp += sizeof(struct direct);
#endif /* LONGFILENAMES */
	return (cp - dbuf);
}

/*
 * allocate a block or frag
 */
daddr_t
alloc(size, mode)
	int size;
	int mode;
{
	int i, frag;
	daddr_t d;

	rdfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		return (0);
	}
	if (acg.cg_cs.cs_nbfree == 0) {
		printf("first cylinder group ran out of space\n");
		return (0);
	}
	for (d = 0; d < acg.cg_ndblk; d += sblock.fs_frag)
		if (isblock(&sblock, acg.cg_free, d / sblock.fs_frag))
			goto goth;
	printf("internal error: can't find block in cyl 0\n");
	return (0);
goth:
	clrblock(&sblock, acg.cg_free, d / sblock.fs_frag);
	acg.cg_cs.cs_nbfree--;
	sblock.fs_cstotal.cs_nbfree--;
	fscs[0].cs_nbfree--;
	if (mode & IFDIR) {
		acg.cg_cs.cs_ndir++;
		sblock.fs_cstotal.cs_ndir++;
		fscs[0].cs_ndir++;
	}
	acg.cg_btot[cbtocylno(&sblock, d)]--;
	acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]--;
	if (size != sblock.fs_bsize) {
		frag = howmany(size, sblock.fs_fsize);
		fscs[0].cs_nffree += sblock.fs_frag - frag;
		sblock.fs_cstotal.cs_nffree += sblock.fs_frag - frag;
		acg.cg_cs.cs_nffree += sblock.fs_frag - frag;
		acg.cg_frsum[sblock.fs_frag - frag]++;
		for (i = frag; i < sblock.fs_frag; i++)
			setbit(acg.cg_free, d + i);
	}
	wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	return (d);
}

/*
 * Allocate an inode on the disk
 */
iput(ip)
	register struct inode *ip;
{
#if defined(SecureWare) && defined(B1)
	char b1buf[MAXBSIZE];
	struct dinode *buf = (struct dinode *) b1buf;
#else
	struct dinode buf[MAXINOPB];
#endif
	daddr_t d;
	int c;

	if (ip->i_number >= sblock.fs_ipg * sblock.fs_ncg) {
		printf("iput: inode value out of range (%d).\n",
		    ip->i_number);
		exit(1);
	}

	c = itog(&sblock, ip->i_number);
	rdfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg 0: bad magic number\n");
		exit(1);
	}
	acg.cg_cs.cs_nifree--;

	/*
	 * The following accounts for the fact that this inode may
	 * be in a cylinder other than the first cylinder.
	 *
	 * This used to say: "setbit(acg.cg_iused, ip->i_number)",
	 * which blows up when the inode number gets large enough
	 * to move into the second cylinder group.  The index into
	 * the acg.cg_iused array must be MODed by fs_ipg first.
	 *
	 * A local is used since setbit() is a macro which uses
	 * its second argument multiple times.
	 */
	{
	    daddr_t ipref = ip->i_number % sblock.fs_ipg;
	    setbit(acg.cg_iused, ipref);
	}

	wtfs(fsbtodb(&sblock, cgtod(&sblock, 0)), sblock.fs_cgsize,
	    (char *)&acg);
	sblock.fs_cstotal.cs_nifree--;
	fscs[0].cs_nifree--;
	d = fsbtodb(&sblock, itod(&sblock, ip->i_number));
	rdfs(d, sblock.fs_bsize, buf);
#if defined(SecureWare) && defined(B1)
	if(ISB1)
		mkfs_pack_inode(&sblock, buf, ip);
	else{
#ifdef ACLS
		buf[itoo(&sblock, ip->i_number)].di_ic = ip->i_icun.i_ic;
#else /* not ACLS */
		buf[itoo(&sblock, ip->i_number)].di_ic = ip->i_ic;
#endif /* ACLS */
	}
#else
#ifdef ACLS
	buf[itoo(&sblock, ip->i_number)].di_ic = ip->i_icun.i_ic;
#else
	buf[itoo(&sblock, ip->i_number)].di_ic = ip->i_ic;
#endif /* ACLS */
#endif
	wtfs(d, sblock.fs_bsize, buf);
}

/*
 * read a block from the file system
 */
rdfs(bno, size, bf)
	daddr_t bno;
	int size;
	char *bf;
{
	int n;

	if (lseek(fsi, bno * DEV_BSIZE, SEEK_SET) == -1) {
		printf("seek error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
	n = read(fsi, bf, size);
	if(n != size) {
		printf("read error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
}

/*
 * write a block to the file system
 */
wtfs(bno, size, bf)
	daddr_t bno;
	int size;
	char *bf;
{
	int n;

	if (lseek(fso, bno * DEV_BSIZE, SEEK_SET) == -1) {
		printf("seek error: %ld\n", bno);
		perror("wtfs");
		exit(1);
	}
	n = write(fso, bf, size);
	if(n != size) {
		printf("write error: %ld\n", bno);
		perror("wtfs");
		exit(1);
	}
}

/*
 * check if a block is available
 */
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	unsigned char mask;

	switch (fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
#ifdef STANDALONE
		printf("isblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "isblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}

/*
 * take a block out of the map
 */
clrblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch ((fs)->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("clrblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "clrblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}

/*
 * put a block into the map
 */
setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	int h;
{
	switch (fs->fs_frag) {
	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
#ifdef STANDALONE
		printf("setblock bad fs_frag %d\n", fs->fs_frag);
#else
		fprintf(stderr, "setblock bad fs_frag %d\n", fs->fs_frag);
#endif
		return;
	}
}
descend(par)
	struct inode *par;
{
	struct inode in;
	int ibc = 0;
	int i, f, c;
	char *save_end;
	struct dinode *dip;
	int entries = NINDIR(fs);
#if defined(SecureWare) && defined(B1)
        char b1inos[MAXBSIZE];
	struct dinode *inos = (struct dinode *) b1inos;
#else
	struct dinode inos[MAXBSIZE / sizeof (struct dinode)];
#endif
	char buf[MAXBSIZE];
	daddr_t ib[MAXBSIZE / sizeof (daddr_t)];

	/*
	 * NOTE: This code will not work if a file specified in
	 *       the proto file needs more than the first indirect
	 *       block.  There is a check for this below, and a
	 *       message is printed.
	 *
	 * This should be fixed, but for now it doesn't seem like to
	 * bad of a restriction (the file has to be pretty big to go
	 * over this limit).
	 *
	 * Some comments on direct and indirect blocks...
	 *
	 * There are NDADDR (typically 12) pointers in the inode
	 * for the first "fs_bsize" blocks of a file.  These blocks
	 * are referred to as "direct blocks".
	 * If a file has more than "fs_bsize" blocks, there are NIADDR
	 * pointers (typically 3) that may be used to point to a block
	 * that contains more pointers.  These blocks are referred to
	 * as "indirect blocks".  These indirect blocks may contain
	 * upto NINDIR(fs) entries.  Typically, this is the same as
	 * "fs_bsize / sizeof (daddr_t)".
	 *
	 * The pointers in the first "indirect block" point to more
	 * data blocks for the file.  Thus, this "indirect" block
	 * points to the NDADDR+1..NDADDR+NINDIR(fs) data blocks.
	 *
	 * If the file is larger than NDADDR+NINDIR(fs) blocks, then
	 * the second indirect pointer will point to upto NINDIR(fs)
	 * "indirect blocks", each of which will point to upto
	 * NINDIR(fs) more data blocks.
	 *
	 * If the file is larger than
	 *         NDADDR+NINDIR(fs)+(NINDIR(fs)*NINDIR(fs)) blocks,
	 * the third indirect block will point to upto NINDIR(fs)
	 * "second indirect" blocks, each of which will point to upto
	 * NINDIR(fs) "indirect" blocks, each of which will point to
	 * upto NINDIR(fs) data blocks.
	 *
	 * And so the scheme would continue.  In HP-UX, we currently
	 * have 12 direct blocks and 3 indirect block pointers in the
	 * inode structure.
	 *
	 * Thus NDADDR is 12, NIADDR is 3 and NINDIR(fs) is usually
	 * the filesystem block size / 4 (i.e. 2048 on a filesystem
	 * that uses 8k blocks).
	 *
	 * If your filesystem used an 8k blocksize, you could store
	 * a file as large as:
	 *
	 * Using no indirect blocks:
	 *
	 *     12 + (0) + (0*2048) + (0*2048*2048) blocks
	 *
	 *   or a total of 12 8k blocks (96 Kbytes)
	 *
	 * Using only the first level of indirect blocks:
	 *
	 *     12 + (2048) + (0*2048) + (0*2048*2048)
	 *
	 *   or a total of 2,060 8k blocks (approx. 16.09 Mbytes)
	 *
	 * Using the first and second level of indirect blocks:
	 *
	 *     12 + (2048) + (2048*2048) + (0*2048*2048)
	 *
	 *   or a total of 4,196,364 8k blocks (approx. 256 Gbytes)
	 *
	 * Using all three levels:
	 *
	 *     12 + (2048) + (2048*2048) + (2048*2048*2048)
	 *
	 *   or a total of 8,594,130,956 8k blocks
	 *   (approx. 65,568 Gbytes, 65 Terabytes)
	 *
	 * While this may sound absurd, remember that block sizes
	 * used to be much smaller than 8k.  If you consider a 4k
	 * block size, the max file size is a little more realistic.
	 */
	memset(&in, 0, sizeof(struct inode));
	memset(buf, 0, fs->fs_bsize);
	memset(ib, (daddr_t)0, NINDIR(fs));
#ifdef SYMLINKS
	in.i_mode = gmode(token[0], "-bcdl",
	    IFREG, IFBLK, IFCHR, IFDIR, IFLNK);
#else
	in.i_mode = gmode(token[0], "-bcd",
	    IFREG, IFBLK, IFCHR, IFDIR, 0);
#endif /* SYMLINKS */
	in.i_mode |= gmode(token[1], "-u", 0, ISUID, 0, 0, 0);
	in.i_mode |= gmode(token[2], "-g", 0, ISGID, 0, 0, 0);
	for (i = 3; i < 6; i++) {
		c = token[i];
		if (c < '0' || c > '7') {
			printf("%c/%s: bad octal mode digit\n", c, token);
			errs++;
			c = 0;
		}
		in.i_mode |= (c-'0')<<(15-3*i);
	}

	in.i_uid = (u_short)getugid(UID);
	in.i_gid = (u_short)getugid(GID);
	in.i_nlink = 1;

	if (par != (struct inode *)0) {
		ialloc(&in);
	} else {
		par = &in;
		i = itod(fs, ROOTINO);
		rdfs(fsbtodb(fs, i), fs->fs_bsize, (char *)inos);
#if defined(SecureWare) && defined(B1)
		if( ISB1 )
		    disk_inode_in_block(fs, inos, &dip, ROOTINO);
		else
		    dip = &inos[ROOTINO % INOPB(fs)];
#else
		dip = &inos[ROOTINO % INOPB(fs)];
#endif
		in.i_number = ROOTINO;
		in.i_nlink = dip->di_nlink;
		in.i_size = dip->di_size;
		in.i_db[0] = dip->di_db[0];
		rdfs(fsbtodb(fs, in.i_db[0]), fs->fs_bsize, buf);
	}

	switch (in.i_mode&IFMT) {

	case IFREG:
		getstr();
		f = open(token, O_RDONLY);
		if (f < 0) {
			printf("%s: cannot open\n", token);
			errs++;
			break;
		}
		while ((i = read(f, buf, (int)fs->fs_bsize)) > 0) {
			if (ibc >= (entries + NDADDR)) {
			    printf("mkfs: file %s too big for processing by mkfs\n", token);
			    printf("      /%s truncated to %d bytes\n",
				proto_path, ibc * (int)fs->fs_bsize);
			    break;
			}
			in.i_size += i;
			newblk(buf, &ibc, ib, (int)blksize(fs, &in, ibc));
		}
		if (ibc <= NDADDR)
			in.i_blocks = btodb( fragroundup(fs,in.i_size) );
		else {
			/*
			 * Calculate how many extra blocks we will
			 * have to hold the indirect blocks.
			 */
			long indir_blocks = 1; /* 1st level indirect */
			long tmp = ibc - NDADDR;

			if ( tmp > entries ) {
				/* 2nd level of indirect blocks */
				tmp -= entries;
				indir_blocks += 
				    ((tmp + entries -1) / entries) + 1;
			}
			if ( tmp > entries * entries ) {
				/* 3rd level of indirect blocks */
				tmp -= entries*entries;
				i = (tmp+(entries*entries-1))/(entries*entries);
				indir_blocks += i + 1;
			}
			in.i_blocks = (ibc + indir_blocks) * NSPB(fs);
		}
		close(f);
		break;

#ifdef SYMLINKS
	case IFLNK:
		getstr();
		memset(buf, '\0', (int)fs->fs_bsize);
		strcpy(buf, token);
		in.i_size = strlen(buf);
		newblk(buf, &ibc, ib, (int)blksize(fs, &in, ibc));
		in.i_blocks = btodb( fragroundup(fs,in.i_size) );
		break;
#endif /* SYMLINKS */

	case IFBLK:
	case IFCHR:
		/*
		 * special file
		 * content is maj/min types
		 */
		i = getnum() & 0xffffff;
		f = getnum() & 0xffffff;
#if defined DUX || defined DISKLESS
		in.i_device = makedev(i,f);
#else
		in.i_rdev = makedev(i,f);
#endif
		in.i_blocks = 0;
		break;

	case IFDIR:
		/*
		 * directory
		 *     put in extra links
		 *     call recursively until name of "$" found
		 */
		save_end = end_pp;

		if (in.i_number != ROOTINO) {
			/*
			 * Put this directory into the global
			 * proto_path buffer and reset end_pp.
			 * (our name is in name_token[])
			 */
			strcpy(end_pp, name_token);
			end_pp += strlen(end_pp);
			*end_pp++ = '/';

			par->i_nlink++;
			in.i_nlink++;
			entry(&in, in.i_number, ".", buf);
			entry(&in, par->i_number, "..", buf);
		}
		else {
		    /*
		     * Set end_pp to the beginning of the proto_path
		     * buffer.
		     */
		    end_pp = &proto_path[0];
		}


		for (;;) {
			getstr();
			if (token[0]=='$' && token[1]=='\0')
				break;

			/*
			 * Read the mode string.  We must save the
			 * file name in another buffer.
			 */
			strcpy(name_token, token);
			getstr();
			if (token[0] == 'L') {
				int ino;

				/*
				 * Create a hard link, mode, owner
				 * and group are all ignored.
				 */
				getstr();  /* owner (ignored) */
				getstr();  /* group (ignored) */
				getstr();  /* path name to link to */
				ino = path_to_inum(token);
				if (ino == 0) {
					fprintf(stderr, "source of hard link (%s) not found\n",
					    token);
					exit(1);
				}
				if (nlink_adjust(ino, 1) == -1) {
					strcpy(end_pp, name_token);
					fprintf(stderr, "can't hardlink to a directory (ln %s /%s)\n",
					    token, proto_path);
					exit(1);
				}
				entry(&in, ino, name_token, buf);
			}
			else {
				entry(&in, (ino_t)(ino+1), name_token, buf);
				descend(&in);
			}
		}

		/*
		 * Restore end_pp to its previous value
		 */
		end_pp = save_end;

		in.i_blocks = btodb (fragroundup(fs, in.i_size));
		if (in.i_number != ROOTINO)
			newblk(buf, &ibc, ib, (int)blksize(fs, &in, 0));
		else
			wtfs(fsbtodb(fs, in.i_db[0]), (int)fs->fs_bsize, buf);
		break;
	}
	proto_iput(&in, &ibc, ib);
}

/*ARGSUSED*/
gmode(c, s, m0, m1, m2, m3, m4)
	char c, *s;
	int m1, m2, m3, m4;
{
	int i;

	if (c == s[0])
		return m0;

	if (s[1] == '\0')
		goto bad_mode;
	if (c == s[1])
		return m1;

	if (s[2] == '\0')
		goto bad_mode;
	if (c == s[2])
		return m2;

	if (s[3] == '\0')
		goto bad_mode;
	if (c == s[3])
		return m3;

	if (s[4] == '\0')
		goto bad_mode;
	if (c == s[4])
		return m4;

bad_mode:
	printf("%c/%s: bad mode\n", c, token);
	errs++;
	return 0;
}

entry(ip, inum, str, buf)
	struct inode *ip;
	ino_t inum;
	char *str;
	char *buf;
{
	register struct direct *dp, *odp;
	int i, oldsize, newsize, spacefree;

	odp = dp = (struct direct *)buf;
	while ((int)dp - (int)buf < ip->i_size) {
		odp = dp;
		dp = (struct direct *)((int)dp + dp->d_reclen);
	}
	if (odp != dp)
#ifdef LONGFILENAMES
#if defined(FD_FSMAGIC)
		if ((fsmagic == FS_MAGIC_LFN) || (fsfeatures & FSF_LFN))
			oldsize = DIRSIZ(odp);
		else oldsize = DIRSTRCTSIZ;
#else /* not new magic number */
		oldsize =  (fsmagic == FS_MAGIC_LFN)?DIRSIZ(odp):DIRSTRCTSIZ;
#endif /* new magic number */
#else /* not LONGFILENAMES */
		oldsize = sizeof(struct direct);
#endif /* LONGFILENAMES */
	else
		oldsize = 0;
	spacefree = odp->d_reclen - oldsize;
	dp = (struct direct *)((int)odp + oldsize);
	dp->d_ino = inum;
#ifdef LONGFILENAMES
 	dp->d_namlen = (strlen(str) > maxnamlen ? maxnamlen : strlen(str));
	newsize = (fsmagic == FS_MAGIC_LFN)?DIRSIZ(dp):DIRSTRCTSIZ;
#if defined(FD_FSMAGIC)
	if ((fsmagic == FS_MAGIC_LFN) || (fsfeatures & FSF_LFN))
		newsize = DIRSIZ(dp);
	else newsize = DIRSTRCTSIZ;
#else /* not new magic number */
	newsize = (fsmagic == FS_MAGIC_LFN)?DIRSIZ(dp):DIRSTRCTSIZ;
#endif /* new magic number */
#else /* not LONGFILENAMES */
 	dp->d_namlen = (strlen(str) > DIRSIZ ? DIRSIZ : strlen(str));
	newsize = sizeof(struct direct);
#endif /* LONGFILENAMES */
	if (spacefree >= newsize) {
		odp->d_reclen = oldsize;
		dp->d_reclen = spacefree;
	} else {
		dp = (struct direct *)((int)odp + odp->d_reclen);
		if ((int)dp - (int)buf >= fs->fs_bsize) {
			printf("directory too large\n");
			exit(1);
		}
		dp->d_ino = inum;
#ifdef LONGFILENAMES
 		dp->d_namlen = (strlen(str) > maxnamlen ? maxnamlen : strlen(str));
#else
 		dp->d_namlen = (strlen(str) > DIRSIZ ? DIRSIZ : strlen(str));
#endif /* LONGFILENAMES */
		dp->d_reclen = DIRBLKSIZ;
	}
#ifdef LONGFILENAMES
#if defined(FD_FSMAGIC)
	if ((fsmagic == FS_MAGIC_LFN) || (fsfeatures & FSF_LFN))
#else /* not new magic number */
	if (fsmagic == FS_MAGIC_LFN)
#endif /* new magic number */
	{
		strncpy(dp->d_name, str, dp->d_namlen) ;

		i = dp->d_namlen;
		dp->d_name[i++] = 0;

		for ( ; (i < MAXNAMLEN) && (i & 3); i++)
 			dp->d_name[i] = 0;
	}
	else if (strlen(str) > DIRSIZ_CONSTANT)
 		strncpy(dp->d_name, str, DIRSIZ_CONSTANT) ;
 	else
 	{
 		strncpy( dp->d_name, str, strlen(str)) ;
 		for (i=strlen(str)+1; i<DIRSIZ_CONSTANT; i++)
 			dp->d_name[i] = 0;
 	}
#else /* not LONGFILENAMES */
 	if (strlen(str) > DIRSIZ)
 		strncpy(dp->d_name, str, DIRSIZ) ;
 	else
 	{
 		strncpy( dp->d_name, str, strlen(str)) ;
 		for (i=strlen(str)+1; i<DIRSIZ; i++)
 			dp->d_name[i] = 0;
 	}
#endif /* LONGFILENAMES */
	ip->i_size = (int)dp - (int)buf + newsize;

	/*
	 * Now call enter_path() to enter this path into our
	 * (path,inode) table used by path_to_inum().
	 */
	enter_path(inum, str);
}

newblk(buf, aibc, ib, size)
	int *aibc;
	char *buf;
	daddr_t *ib;
	int size;
{
	daddr_t bno;

	/*
	 * There are checks elsewhere that should prevent this from
	 * ever occurring.  But, to be safe...
	 */
	if (*aibc > (NDADDR + NINDIR(fs))) {
		fprintf(stderr,
		    "mkfs: internal error, indirect block full\n");
		exit(1);
	}

	bno = proto_alloc(size);
	wtfs(fsbtodb(fs, bno), (int)fs->fs_bsize, buf);
	memset(buf, 0, fs->fs_bsize);
	ib[(*aibc)++] = bno;
}

proto_iput(ip, aibc, ib)
	struct inode *ip;
	int *aibc;
	daddr_t *ib;
{
	daddr_t d;
	int i;
#if defined(SecureWare) && defined(B1)
	char b1buf[MAXBSIZE];
	struct dinode *buf = (struct dinode *) b1buf;
#else
	struct dinode buf[MAXBSIZE / sizeof (struct dinode)];
#endif

	ip->i_atime = ip->i_mtime = ip->i_ctime = time((long *)0);
	switch (ip->i_mode&IFMT) {

	case IFDIR:
	case IFREG:
#ifdef SYMLINKS
	case IFLNK:
#endif
		/*
		 * Direct blocks
		 */
		for (i = 0; i < *aibc; i++) {
			if (i >= NDADDR)
				break;
			ip->i_db[i] = ib[i];
		}
		/*
		 * Indirect blocks.  This code only handles the
		 * first level of indirect blocks!
		 */
		if (*aibc > NDADDR) {
			int n_indir;
			int leftover;

			/*
			 * Allocate a block for our indirect block
			 * pointers.
			 */
			ip->i_ib[0] = proto_alloc((int)fs->fs_bsize);

			/*
			 * We've already put the first NDADDR direct
			 * block pointers from ib[] into the inode,
			 * now copy the rest of them into our indirect
			 * block, and make sure the rest of the block
			 * is initialized to 0.
			 */
			n_indir = (*aibc - NDADDR);
			leftover = fs->fs_bsize -
				   (n_indir * sizeof (daddr_t));

			ib += NDADDR;
			memset(ib + n_indir, 0, leftover);
			wtfs(fsbtodb(fs, ip->i_ib[0]),
			    (int)fs->fs_bsize, (char *)ib);
		}
		break;

	case IFBLK:
	case IFCHR:
		break;

	default:
		printf("bad mode %o\n", ip->i_mode);
		exit(1);
	}
	d = fsbtodb(fs, itod(fs, ip->i_number));
	rdfs(d, (int)fs->fs_bsize, (char *)buf);
#if defined(SecureWare) && defined(B1)
	if( ISB1 )
	    mkfs_pack_inode(fs, buf, ip);
	else{
#ifdef ACLS
	    buf[itoo(fs, ip->i_number)].di_ic = ip->i_icun.i_ic;
#else
	    buf[itoo(fs, ip->i_number)].di_ic = ip->i_ic;
#endif /* ACLS */
	}
#else
#ifdef ACLS
	buf[itoo(fs, ip->i_number)].di_ic = ip->i_icun.i_ic;
#else
	buf[itoo(fs, ip->i_number)].di_ic = ip->i_ic;
#endif /* ACLS */
#endif
	wtfs(d, (int)fs->fs_bsize, (char *)buf);
}

/*
 * nlink_adjust() --
 *     adjust the link count of an inode 'inum' by 'incr' (incr may be
 *     negative).
 *     We refuse to adjust the link count on a directory (returning -1).
 */
int
nlink_adjust(inum, incr)
	int inum;
	int incr;
{
	daddr_t d;
	int i;
	struct dinode buf[MAXBSIZE / sizeof (struct dinode)];

	/*
	 * Compute the block number where this inode is stored and
	 * read in the block of inodes.
	 */
	d = fsbtodb(fs, itod(fs, inum));
	rdfs(d, (int)fs->fs_bsize, (char *)buf);

	/*
	 * disallow hard links to directories
	 */
	i = buf[itoo(fs, inum)].di_ic.ic_mode & S_IFMT;
	if (i == 0 || i == S_IFDIR)
	    return -1;

	/*
	 * Change the link count of the inode we are interested in.
	 */
	if ((i = (buf[itoo(fs, inum)].di_ic.ic_nlink += incr)) <= 0 ||
	    i >= MAXLINK) {
		fprintf(stderr,
		    "nlink_adjust(): bad link count (%d) for ino %d\n",
		    i, inum);
		exit(1);
	}

	/*
	 * Now write the modified block of inodes back out to the disk.
	 */
	wtfs(d, (int)fs->fs_bsize, (char *)buf);
}

daddr_t
proto_alloc(size)
	int size;
{
	int i, frag;
	daddr_t d;
	static int cg = 0;

again:
#ifdef DEBUG
printf("proto_alloc: cg=%d, cgtod=%ld\n", cg, cgtod(&sblock, cg));
#endif
	rdfs(fsbtodb(&sblock, cgtod(&sblock, cg)), (int)sblock.fs_cgsize,
	    (char *)&acg);
#ifdef DEBUG
printf("proto_alloc: fsbtodb:cgtod=%ld\n", fsbtodb(&sblock, cgtod(&sblock, cg)));
#endif
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg %d: bad magic number\n", cg);
		return (0);
	}
	if (acg.cg_cs.cs_nbfree == 0) {
		cg++;
		if (cg >= fs->fs_ncg) {
			printf("ran out of space\n");
			return (0);
		}
		goto again;
	}
	for (d = 0; d < acg.cg_ndblk; d += sblock.fs_frag)
		if (isblock(&sblock, (u_char *)acg.cg_free, d / sblock.fs_frag))
			goto goth;
	printf("internal error: can't find block in cyl %d\n", cg);
	return (0);
goth:
	clrblock(&sblock, (u_char *)acg.cg_free, d / sblock.fs_frag);
	acg.cg_cs.cs_nbfree--;
	sblock.fs_cstotal.cs_nbfree--;
	fscs[cg].cs_nbfree--;
	acg.cg_btot[cbtocylno(&sblock, d)]--;
	acg.cg_b[cbtocylno(&sblock, d)][cbtorpos(&sblock, d)]--;
	if (size != sblock.fs_bsize) {
		frag = howmany(size, sblock.fs_fsize);
		fscs[cg].cs_nffree += sblock.fs_frag - frag;
		sblock.fs_cstotal.cs_nffree += sblock.fs_frag - frag;
		acg.cg_cs.cs_nffree += sblock.fs_frag - frag;
		acg.cg_frsum[sblock.fs_frag - frag]++;
		for (i = frag; i < sblock.fs_frag; i++)
			setbit(acg.cg_free, d + i);
	}
	wtfs(fsbtodb(&sblock, cgtod(&sblock, cg)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	return (acg.cg_cgx * fs->fs_fpg + d);
}

/*
 * Allocate an inode on the disk
 */
ialloc(ip)
	register struct inode *ip;
{
	struct dinode buf[MAXBSIZE / sizeof (struct dinode)];
	daddr_t d;
	int c;

	ip->i_number = ++ino;
	if (ip->i_number >= sblock.fs_ipg * sblock.fs_ncg) {
		printf("ialloc: inode value out of range (%d).\n",
		    ip->i_number);
		exit(1);
	}

	c = itog(&sblock, ip->i_number);
	rdfs(fsbtodb(&sblock, cgtod(&sblock, c)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	if (acg.cg_magic != CG_MAGIC) {
		printf("cg %d: bad magic number\n", c);
		exit(1);
	}
	if (ip->i_mode & IFDIR) {
		acg.cg_cs.cs_ndir++;
		sblock.fs_cstotal.cs_ndir++;
		fscs[c].cs_ndir++;
	}
	acg.cg_cs.cs_nifree--;

	/*
	 * The following accounts for the fact that this inode may
	 * be in a cylinder other than the first cylinder.
	 *
	 * This used to say: "setbit(acg.cg_iused, ip->i_number)",
	 * which blows up when the inode number gets large enough
	 * to move into the second cylinder group.  The index into
	 * the acg.cg_iused array must be MODed by fs_ipg first.
	 *
	 * A local is used since setbit() is a macro which uses
	 * its second argument multiple times.
	 */
	{
	    daddr_t ipref = ip->i_number % sblock.fs_ipg;
	    setbit(acg.cg_iused, ipref);
	}

	wtfs(fsbtodb(&sblock, cgtod(&sblock, c)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	sblock.fs_cstotal.cs_nifree--;
	fscs[c].cs_nifree--;
	return (ip->i_number);
}

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
	if (fs.f_magic == 0) {
		if (fs.f_featurebits == 14)
			return(FS_MAGIC);
		if (fs.f_featurebits == 255)
			return(FS_MAGIC_LFN);
	} else {
		if ((fs.f_magic == FS_MAGIC_LFN) || (fs.f_featurebits & FSF_LFN))
			return(FS_MAGIC_LFN);
		else if (fs.f_magic == FS_MAGIC)
			return(FS_MAGIC);
	}

	printf("mkfs: bad super block magic number on /\n");
	exit(1);
#else /* not DUX || DISKLESS */
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
		printf("mkfs: cannot open %s\n", mnt->mnt_fsname);
		exit(1);
	}
	if (lseek(fd, (long)dbtob(SBLOCK), SEEK_SET) == -1)
	{
		printf("mkfs: lseek error on %s\n", mnt->mnt_fsname);
		exit(1);
	}
	super =  (struct fs *)pad;
	if (read(fd, super, SBSIZE) != SBSIZE)
	{
		printf("mkfs: read error on %s\n", mnt->mnt_fsname);
		exit(1);
	}
	close(fd);
#if defined(FD_FSMAGIC)
	if ((super->fs_magic != FS_MAGIC) && (super->fs_magic != FS_MAGIC_LFN)
		&& (super->fs_magic != FD_FSMAGIC))
#else /* not new magic number */
	if ((super->fs_magic != FS_MAGIC) && (super->fs_magic != FS_MAGIC_LFN))
#endif /* new magic number */
	{
		printf("mkfs: bad super block magic number on %s\n", mnt->mnt_fsname);
		exit(1);
	}
	return(super->fs_magic);
#endif /* DUX || DISKLESS */
}
#endif /* LONGFILENAMES */

/*
 * Given the device number and type (S_IFBLK or S_IFCHR), return the
 * appropriate rotational delay
 */
int
get_rot_delay(dev, type)
dev_t dev;
int type;
{

#ifdef __hp9000s800
	int majornum;
	char *driver;
	register struct rotdelay_info *p;
	char *get_driver_name();
#endif

	/*
	 * On the s300 and s700, the partition number is used for
	 * Software Disk Striping.  Software disk striping always
	 * uses a rotational delay of 0 for multi-disk arrays.
	 *
	 * We do not want to force the rotational delay to 0 for
	 * single disk arrays, so we run /etc/sdsadmin -l and count
	 * how many "device" entries it has.  If it has more than
	 * 1, then we use a rotational delay of 0, otherwise we
	 * go through the normal logic to decide what rotational
	 * delay to use.
	 */
	if ((dev & 0x0f) != 0 && sysconf(_SC_IO_TYPE) == IO_TYPE_WSIO) {
	    static char cmd_str[] =
		"(/etc/sdsadmin -l %s | /bin/grep -c '^device') 2>/dev/null";
	    FILE *fp;
	    char *cmd = malloc(strlen(fsys) + strlen(cmd_str) + 1);

	    if (cmd == (char *)0) {
		fputs("mkfs: out of memory\n", stderr);
		exit(1);
	    }

	    sprintf(cmd, cmd_str, fsys);
	    if ((fp = popen(cmd, "r")) != (FILE *)0) {
		char buf[10];

		free(cmd);
		fgets(buf, sizeof buf, fp);
		pclose(fp);
		if (buf[0] >= '2' && buf[0] <= '8' && buf[1] == '\n')
		    return 0;
	    }
	    else
		free(cmd);
	}

#ifdef __hp9000s800

	majornum = major(dev);

	if (sysconf(_SC_IO_TYPE) != IO_TYPE_SIO) {

		int rot_delay, wrt_cach;
		disk_describe_type dsk_data;

		/* we are on a 700.  a rotational delay value of 0 is
		   preferable if the drive has read ahead buffering, else we
		   should use the default value of 4.  this code is klugy
		   and is intended only as a short-term solution for release
		   IF1.  it depends on the fact that, for the SCSI drives
		   supported in IF1, the presence of write caching is
		   equivalent to read caching.  this is true for all drives
		   except magneto-optical.  these will get the default value
		   even though a value of 0 would be better. */

		rot_delay = ROTDELAY_DEFAULT;
		if (ioctl(fsi, DIOC_DESCRIBE, &dsk_data) != -1)
			if (dsk_data.intf_type == SCSI_INTF)
				 if (ioctl(fsi, SIOC_GET_IR, &wrt_cach) != -1)
					if (wrt_cach)
						rot_delay = 0;
		return(rot_delay);
	}

	if (type != S_IFBLK && type != S_IFCHR) {
		fprintf(stderr,"mkfs: Unknown device type, using default rotational delay\n");
		return(ROTDELAY_DEFAULT);
	}

	if ((driver = get_driver_name(majornum,type)) == NULL) {
		/* Well, we couldn't get the driver name so
		   use the default major numbers for the
		   different drivers to get the rot_delay. */

		/* FSDlj09629 */
                /* The SCSI interface should be checked to decide
		   the rotdelay. Should be 0 if the SCSI is IRed. */

  		int rot_delay, wrt_cach;
  		disk_describe_type dsk_data;
  
  		if (majornum == MAJOR_B_SCSI || majornum == MAJOR_C_SCSI) {
  		rot_delay = ROTDELAY_SCSI;
  		if (ioctl(fsi, DIOC_DESCRIBE, &dsk_data) != -1)
  		   if (dsk_data.intf_type == SCSI_INTF)
  		      if (ioctl(fsi, SIOC_GET_IR, &wrt_cach) != -1)
  			 if (wrt_cach)
  			    rot_delay = ROTDELAY_SCSI_IR;
  			return(rot_delay);
  		}

		if (type == S_IFBLK) {
			switch(majornum) {
			case MAJOR_B_HPIB:	return(ROTDELAY_HPIB);
			case MAJOR_B_HPFL:	return(ROTDELAY_ALINK);
			case MAJOR_B_NIO_HPIB:	return(ROTDELAY_NIO_HPIB);
			/* FSDlj09629 */
			/* case MAJOR_B_SCSI:	return(ROTDELAY_SCSI); */
			/* autochanger should use 0 as the rotdelay. */
			case MAJOR_B_AUTOCH:	return(ROTDELAY_AUTO);
			default:		return(ROTDELAY_DEFAULT);
			} /* switch */
		} else if (type == S_IFCHR) {
			switch(majornum) {
			case MAJOR_C_HPIB:	return(ROTDELAY_HPIB);
			case MAJOR_C_HPFL:	return(ROTDELAY_ALINK);
			case MAJOR_C_NIO_HPIB:	return(ROTDELAY_NIO_HPIB);
			/* FSDlj09629 */
			/* case MAJOR_C_SCSI:	return(ROTDELAY_SCSI); */
			/* autochanger should use 0 as the rotdelay. */
			case MAJOR_C_AUTOCH:	return(ROTDELAY_AUTO);
			default:		return(ROTDELAY_DEFAULT);
			} /* switch */
		}
	}

	for (p = rotdelay_info; p->name != NULL; p++)
		if (strcmp(p->name,driver) == 0) {
	
		/* FSDlj09629 */
                /* The SCSI interface should be checked to decide
		   the rotdelay. Should be 0 if the SCSI is IRed. */

  		int rot_delay, wrt_cach;
  		disk_describe_type dsk_data;

  	        if (strcmp(driver,"disc3") == 0) {
  		   rot_delay = ROTDELAY_SCSI;
  		   if (ioctl(fsi, DIOC_DESCRIBE, &dsk_data) != -1)
  		      if (dsk_data.intf_type == SCSI_INTF)
  		         if (ioctl(fsi, SIOC_GET_IR, &wrt_cach) != -1)
  			    if (wrt_cach)
  			       rot_delay = ROTDELAY_SCSI_IR;
  		   return(rot_delay);
  		   }
		else
		   return(p->delay);
                }

	fprintf(stderr,
		"Unknown driver type %s, using default rotational delay\n",
		driver);
#endif /* __hp9000s800 */
	return(ROTDELAY_DEFAULT);
}

#ifdef __hp9000s800
/*
 * Given the device number and type (S_IFBLK or S_IFCHR), return the device
 * driver name.  Return NULL if unknown.
 * NOTE:  This code is not called for the s700.  It is used only to get
 * the SIO driver name.
 */
char *
get_driver_name(majornum, type)
int majornum, type;
{
	io_mgr_type	*mgr_table;
	int		 status, num_mgrs, i=0;

	if ((status = io_init(O_RDONLY)) != SUCCESS) {
	    (void)print_libIO_status("mkfs", _IO_INIT, status, O_RDONLY);
	    return(NULL);
	}

	if ((num_mgrs = io_get_table(T_IO_MGR_TABLE, (void **)&mgr_table))
	        < 0) {
	    (void)print_libIO_status("mkfs", _IO_GET_TABLE, status,
		     T_IO_MGR_TABLE, (void *)mgr_table);
	    io_end();
	    return(NULL);
	}

	for (i=0; i < num_mgrs; i++)
	    {
		if ((type == S_IFCHR ? mgr_table[i].c_major :
		     mgr_table[i].b_major) == majornum)
		{
		    static char driver_buf[MAX_ID];

		    strncpy(driver_buf, mgr_table[i].name, MAX_ID);
		    io_free_table(T_IO_MGR_TABLE, (void **)&mgr_table);
		    io_end();
		    return(driver_buf);
		}
	    }
	io_free_table(T_IO_MGR_TABLE, (void **)&mgr_table);
	io_end();
	return(NULL);
}
#endif /* __hp9000s800 */

void
log(string)
char *string;
{
	if (logfile != NULL)
		fprintf(logfile, "%s", string);
	return;
}

/*
 * The following structure is used to maintain linked lists that
 * are used to map arbitrary strings to arbitrary numbers.  These
 * lists are used by getugid() and path_to_inum().
 *
 * The string[] is really longer, but we dynamically allocate it to be
 * just the right size we need.
 */
struct str_num_node
{
    struct str_num_node *next;
    unsigned long number;
    char string[1];
};

/*
 * lookup_string() --
 *    look for a string in a list pointed to by 'ptr'.  If a match
 *    is found, *pnumber is set to the number associated with the
 *    string.
 * returns:
 *    0 -- a match was found
 *   -1 -- no match was found
 */
int
lookup_string(ptr, string, pnumber)
struct str_num_node *ptr;
char *string;
unsigned long *pnumber;
{
    while (ptr != (struct str_num_node *)0)
    {
	if (strcmp(ptr->string, string) == 0)
	{
	    *pnumber = ptr->number;
	    return 0;
	}
	ptr = ptr->next;
    }
    return -1;
}

/*
 * enter_string() --
 *    add a string to a list pointed to by 'ptr'.
 */
void
enter_string(pptr, string, number)
struct str_num_node **pptr;
char *string;
unsigned long number;
{
    struct str_num_node *ptr;
    int len = strlen(string);

    /*
     * Allocate enough memory for a str_num_node structure and the
     * string that we need to save.
     */
    ptr = (struct str_num_node *)
	malloc(sizeof (struct str_num_node) + len);
    if (ptr == (struct str_num_node *)0)
    {
	fputs("mkfs: out of memory\n", stderr);
	exit(1);
    }

    ptr->next = *pptr;
    ptr->number = number;
    strcpy(ptr->string, string);
    *pptr = ptr;
}

/*
 * getstr() --
 *    reads the next string from the proto file, placing it in
 *    the global string 'token'.
 */
getstr()
{
    int i;
    int c;

    /*
     * Skip blanks, tabs and newlines.  A ':' delimits a comment
     * that runs to the end of a line.
     */
    do
    {
	c = getc(fin);

	if (c == ':')
	    while ((c = getc(fin)) != EOF && c != '\n')
		continue;

	if (c == EOF)
	{
	    fprintf(stderr, "mkfs: unexpected EOF in proto file\n");
	    exit(1);
	}
    } while (c == ' ' || c == '\t' || c == '\n');

    /*
     * Now read the next token, placing it in the global
     * string 'token'.
     */
    i = 0;
    do
    {
	token[i++] = c;
	c = getc(fin);
    } while (c != EOF && c != ' ' && c != '\t' && c != '\n');
    token[i] = '\0';
}

/*
 * getnum() --
 *    read a numeric token from the proto file.  Numbers beginning
 *    with '0x' or '0X' are hex, numbers beginning with a leading 0
 *    are octal, all others are decimal.
 */
unsigned long
getnum()
{
    unsigned long n;
    char *p;

    getstr();
    n = strtoul(token, &p, 0);
    if (*p != '\0')
    {
	fprintf(stderr, "mkfs: %s: bad number in proto file\n",
		token);
	errs++;
	return 0;
    }
    return n;
}

/*
 * getugid() --
 *     read a symbolic or numeric user or group name from the proto
 *     file.  Symbolic names are translated to their numeric
 *     representation.
 *
 *     To avoid excessive accesses to the password or group databases,
 *     we keep track of uid and gid strings that we have already looked
 *     up in a linked list using lookup_string() and enter_string().
 */
static long
getugid(type)
int type;
{
    register long i;
    char *p;

    getstr();

    /*
     * First, try to convert to a number.  If that succeeds, simply
     * return the result.
     */
    i = strtoul(token, &p, 0);
    if (*p == '\0')
	return i;

    /*
     * The uid/gid field is not numeric, try looking it up in the
     * password or group database.
     */
    if (type == UID)
    {
	struct passwd *getpwnam();
	static struct str_num_node *uid_table = 0;
	struct passwd *pw;
	unsigned long num;

	if (lookup_string(uid_table, token, &num) == 0)
	    return num;

	if ((pw = getpwnam(token)) != (struct passwd *)0)
	{
	    enter_string(&uid_table, token, pw->pw_uid);
	    return pw->pw_uid;
	}
    }
    else
    {
	struct group *getgrnam();
	static struct str_num_node *gid_table = 0;
	struct group *gr;
	unsigned long num;

	if (lookup_string(gid_table, token, &num) == 0)
	    return num;

	if ((gr = getgrnam(token)) != (struct group *)0)
	{
	    enter_string(&gid_table, token, gr->gr_gid);
	    return gr->gr_gid;
	}
    }

    fprintf(stderr, "mkfs: %s: bad %s in proto file\n",
	token, type == UID ? "uid" : "gid");
    errs++;
    return 0;
}

/*
 * Routines used to translate path names to inode numbers without
 * having to seek all over the filesystem.  The lookup algorithm
 * uses a simple linear search through a linked list (it is not
 * expected that there will be many paths, so the performance of
 * this should be adequate).
 */
static struct str_num_node *path_ptr = (struct str_num_node *)0;

void
enter_path(inum, basename)
int inum;
char *basename;
{
    struct str_num_node *ptr;

    strcpy(end_pp, basename);
    enter_string(&path_ptr, proto_path, inum);
}

int
path_to_inum(path)
char *path;
{
    unsigned long num;

    /*
     * Ignore any leading '/'s, if present
     */
    while (*path == '/')
	path++;

    if (lookup_string(path_ptr, path, &num) == 0)
	return num;
    return 0;
}

static long find_min_fsize(fd)
int fd;
{
	disk_describe_type dsk_data;
	if (ioctl(fd, DIOC_DESCRIBE, &dsk_data) != -1 &&
	    dsk_data.lgblksz > DEV_BSIZE)
		return((long) dsk_data.lgblksz);
	return DEV_BSIZE;
}

/* 
 * is_mounted(): 
 *		check whether or not the specified device is a mounted
 *		file system. This function converts the device name to 
 *		a block mode name (e.g. /dev/dsk/...), and use ustat() 
 *		to figure out it's mounted or not.
 */
is_mounted(device,dev_st)
char *device;
struct stat *dev_st;
{
 	struct ustat ust;
 	char *dp;
	char buf[BUFSIZ];

 strcpy(buf,device);

 if ((dev_st->st_mode & S_IFMT) == S_IFCHR) {

	dp = strrchr(buf, '/');
	if (strncmp(dp, "/r", 2) != 0) 
		while (dp >= buf && *--dp != '/');
	if (*(dp+1) == 'r')
		(void)strcpy(dp+1, dp+2);
	if (stat(buf,dev_st)<0) 
		return(0);
 }

 if (ustat(dev_st->st_rdev, &ust) >= 0)
	return(1);

 return(0);
}

/*
 * is_swap():  checks whether or not the target device is a swap device
 *	       by scanning thru the swap_info, which is returned from 
 *	       pstat() calls.
 */
is_swap(devno)
dev_t devno;
{
#define PS_BURST 1


	struct pst_swapinfo pst[PS_BURST];
	register struct pst_swapinfo *psp = &pst[0];
	int idx = 0;
	int count;
	int match=0;

	while ((count= pstat(PSTAT_SWAPINFO, psp, sizeof(*psp),
			     PS_BURST, idx) != 0)) {
		idx = pst[count - 1].pss_idx + 1;
		if ((psp->pss_flags & SW_BLOCK) && 
		    (psp->pss_major == major(devno)) &&
		    (psp->pss_minor == minor(devno))) {
		    match = 1;
		    break;
		}
	}
	return match;
}
