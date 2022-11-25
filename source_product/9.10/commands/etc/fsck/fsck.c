/* @(#) $Revision: 70.14.2.7 $ */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/pstat.h>

/*
 * NOTE: This needs to be cleaned up once the header files
 *	 get straightened out.
 */
#ifdef HP_NFS
#include <time.h>
#define  d_ino d_fileno
#ifdef LONGFILENAMES
#define  SHORT_DIRSIZ 14
#define  DIRSIZ_MACRO
#else /* not LONGFILENAMES */
#define  DIRSIZ 14
#endif /* not LONGFILENAMES */
#endif /* HP_NFS */
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/fs.h>
#include <sys/stat.h>
#include <sys/ustat.h>
#include <mntent.h>
#include <signal.h>
#include "wait.h"
#define KERNEL
#define _KERNEL
#include <sys/dir.h>
#undef KERNEL
#undef _KERNEL
#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#include <sys/audit.h>
#endif /* secureWare && B1 */

#include <sys/errno.h>

/* RECONSTRUCT ONLY BAD CG IN PASS 6 */

typedef	int	(*SIG_TYP)();

#ifdef OLD_RFA
/*
 * Define IFNWK if it isn't defined.  This is so that we can recognize
 * network special files even after we obsolete them.
 */
#ifndef IFNWK
#define IFNWK 0110000 /* network special files */
#endif
int n_rfa_files = 0;  /* for warning message */
#endif /* OLD_RFA */

#ifdef TRUX
#define OLD_ACLS
#endif

#ifdef OLD_ACLS
#define ACLS
#ifndef IFCONT
#   define IFCONT    0070000            /* continuation inode */
#endif /* IFCONT */
#define di_contin di_ic.ic_spare[2]  /* continuation inode */
#endif /* OLD_ACLS */

#define READRETRIES	5	/* Number of times to retry reads */
#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))
#if TRUX || !defined(SecureWare) || !defined(B1)
#define	MAXINOPB	(MAXBSIZE / sizeof (struct dinode))
#endif
#define	SPERB		(MAXBSIZE / sizeof(short))
#define MINDIRSIZE	(sizeof (struct dirtemplate))
#ifdef LONGFILENAMES
#define MINDIRSIZE_LFN	(sizeof (struct dirtemplate_lfn))
#endif /* LONGFILENAMES */

#define	MAXDUP	10		/* limit on dup blks (per inode) */
#define	MAXBAD	10		/* limit on bad blks (per inode) */
#ifdef CACHE
#define CG_MAXBAD 50		/* limit on bad blocks in bitmaps */
#define CACHE_BSIZE 16384
#define CACHE_INOPB (CACHE_BSIZE/sizeof(struct dinode))
#endif

#define	USTATE	0		/* inode not allocated */
#define	FSTATE	01		/* inode is file */
#define	DSTATE	02		/* inode is directory */
#define	CLEAR	03		/* inode is to be cleared */
#ifdef ACLS
#define	CSTATE	04		/* inode is a continuation inode */
#define	CRSTATE	010		/* continuation inode has been referenced */
#define	HASCINODE	020	/* has continuation inode associated with it */
#define	STATE	(USTATE|FSTATE|DSTATE|CLEAR|CSTATE|CRSTATE)
#endif /* ACLS */

struct	dinode zino;		/* empty inode used to clear inodes */
typedef struct dinode	DINODE;
typedef struct direct	DIRECT;

#define	ALLOC	((dp->di_mode & IFMT) != 0)
#define	DIRCT	((dp->di_mode & IFMT) == IFDIR)
#define	REG	((dp->di_mode & IFMT) == IFREG)
#define	BLK	((dp->di_mode & IFMT) == IFBLK)
#define	CHR	((dp->di_mode & IFMT) == IFCHR)
#define	LNK	((dp->di_mode & IFMT) == IFLNK)
#ifdef IC_FASTLINK
#define FASTLNK (LNK && (dp->di_flags & IC_FASTLINK))
#else
#define FASTLNK (0)
#endif
#ifdef ACLS
#define	CONT	((dp->di_mode & IFMT) == IFCONT)  /* continuation inode */
#endif /* ACLS */
#define	SOCK	((dp->di_mode & IFMT) == IFSOCK)
#define	BADBLK	((dp->di_mode & IFMT) == IFMT)
#ifdef ACLS
#define	SPECIAL	(BLK || CHR || CONT)
#else /* no ACLS */
#define	SPECIAL	(BLK || CHR)
#endif /* ACLS */

struct bufarea {
	struct bufarea	*b_next;		/* must be first */
	daddr_t	b_bno;
	int	b_size;
	union {
		char	b_buf[MAXBSIZE];	/* buffer space */
		short	b_lnks[SPERB];		/* link counts */
		daddr_t	b_indir[MAXNINDIR];	/* indirect block */
		struct	fs b_fs;		/* super block */
		struct	cg b_cg;		/* cylinder group */
#if defined(TRUX) || !defined(SecureWare) || !defined(B1)
		struct dinode b_dinode[MAXINOPB]; /* inode block */
#endif
	} b_un;
	char	b_dirty;
};

typedef struct bufarea BUFAREA;

BUFAREA	inoblk;			/* inode blocks */
BUFAREA	fileblk;		/* other blks in filesys */
BUFAREA	sblk;			/* file system superblock */
BUFAREA	cgblk;			/* cylinder group blocks */

#ifdef CACHE
/*
 *	fsck performance tuning for 68K 9.03 release -- DKM 12-7-93
 *
 *	3 changes yielded a 2x performance increase:
 *		- a small generic "buffer cache" of blocks <= 8k
 *		- a table of directory inodes (filled during pass 1, 
 *		  used in pass 2)
 *		- reading inodes (in ginode) CACHE_BSIZE at a time instead of 
 *		  fs_bsize at a time
 */
 
int cache_useful = 0; 	  /*  we don't need to search di cache in pass 1  */
int dc_srches = 0;
int dc_hits = 0;
int dc_taken = 0;
ino_t first_inode, last_inode;		/*  currently in inoblk (ginode())  */

#define CACHE_SIZE 100
#define CB_SIZE 8192		/*  don't cache blocks bigger than this  */

int cache_size = CACHE_SIZE/2;		/*  default = 50*8k = 400k  */
int cache_hits = 0, cache_misses = 0;

struct cache {
	int b_blkno; 
	char b_buf[CB_SIZE];		/*  should this be MAXBSIZE?  */
} *bc[CACHE_SIZE];

#endif


#define	initbarea(x)	(x)->b_dirty = 0;(x)->b_bno = (daddr_t)-1
#define	dirty(x)	(x)->b_dirty = 1

#ifdef CACHE

struct di_cache {	/*  each dir inode will be put in one of these  */
	DINODE di;	    /*  must be first in struct!  */
	char dc_dirty;	    /*  inode needs to be written back out  */
	char hits;
	short rc;
	int key;	    /*  i-number  */
};

struct di_cache *di_tbl, *di_tbl_end;
int di_tbl_size = 0;

/* 
 *	when an entry in our directory inode cache is dirty, we set its
 *	dirty bit - if it's a regular inode or a DI that didn't make it
 *	into the cache (out of memory?), set the dirty bit for the whole blk
 */
int dirty_count = 0;

ino_dirty(dp)
struct di_cache *dp;
{
	if (dp >= di_tbl && dp < di_tbl_end)
		dp->dc_dirty = 1; 
	else					
		inoblk.b_dirty = 1; 		
	dirty_count++;
}			

#define	inodirty()	ino_dirty((struct di_cache *) dp)
#else
#define	inodirty()	inoblk.b_dirty = 1
#endif

#define	sbdirty()	sblk.b_dirty = 1
#define	cgdirty()	cgblk.b_dirty = 1

#define	dirblk		fileblk.b_un
#define	sblock		sblk.b_un.b_fs
#define	cgrp		cgblk.b_un.b_cg

struct filecntl {
	int	rfdes;
	int	wfdes;
	int	mod;
} dfile;			/* file descriptors for filesys */

struct inodesc {
	char id_type;		/* type of descriptor, DATA or ADDR */
	int (*id_func)();	/* function to be applied to blocks of inode */
	ino_t id_number;	/* inode number described */
	ino_t id_parent;	/* for DATA nodes, their parent */
	daddr_t id_blkno;	/* current block number being examined */
	int id_numfrags;	/* number of frags contained in block */
	long id_filesize;	/* for DATA nodes, the size of the directory */
	int id_loc;		/* for DATA nodes, current location in dir */
	int id_entryno;		/* for DATA nodes, current entry number */
	DIRECT *id_dirp;	/* for data nodes, ptr to current entry */
	enum {DONTKNOW, NOFIX, FIX} id_fix; /* policy on fixing errors */
};
/* file types */
#define	DATA	1
#define	ADDR	2


#define	DUPTBLSIZE	100	/* num of dup blocks to remember */
daddr_t	duplist[DUPTBLSIZE];	/* dup block table */
daddr_t	*enddup;		/* next entry in dup table */
daddr_t	*muldup;		/* multiple dups part of table */

#define	MAXLNCNT	50000	/* num zero link cnts to remember */
ino_t	badlncnt[MAXLNCNT];	/* table of inos with zero link cnts */
ino_t	*badlnp;		/* next entry in table */

#if defined(SecureWare) && defined(B1)
char auditbuf[80];
#endif

char	rawflg;
char	nflag;			/* assume a no response */
char	yflag;			/* assume a yes response */
char	fflag;			/* force fsck to check a mounted fs */
int	bflag;			/* location of alternate super block */
int	qflag;			/* less verbose flag */
int	debug;			/* output debugging info */
int	pclean;			/* preen plus only check if !FS_CLEAN */
char	preen;			/* just fix normal inconsistencies */
char	rplyflag;		/* any questions asked? */
char	hotroot;		/* checking root device */

daddr_t cached_block;

#ifdef QFS_CMDS 
char	run_fsck_on_qfsck_fail;

struct preen_list_entry {
	int pid;
	char *dev_name;
	char retry;
	struct preen_list_entry *next_dev;
};

struct preen_list_entry *preen_list;
#endif /*  QFS_CMDS */

#ifdef LONGFILENAMES
int	longfilenames;		/* 0 if current fs is short files (<14)
				   and 1 if long files are allowed (<=255) */
#endif /* LONGFILENAMES */
int	fixed;			/* set to zero if a 'no' reply is given */
char	fixcg;			/* corrupted free list bit maps */
#ifdef OLD_ACLS
int	nukeacls;		/* if true, remove old acls */
#endif /* OLD_ACLS */

char	*blockmap;		/* ptr to primary blk allocation map */
char	*freemap;		/* ptr to secondary blk allocation map */
char	*statemap;		/* ptr to inode state table */
short	*lncntp;		/* ptr to link count table */

char	*srchname;		/* name being searched for in dir */
char	pathname[BUFSIZ];	/* current pathname */
char	*pathp;			/* pointer to pathname position */
char	*endpathname = &pathname[BUFSIZ - 2];

char	*lfname = "lost+found";

ino_t	imax;			/* number of inodes */
ino_t	lastino;		/* hiwater mark of inodes */
ino_t	lfdir;			/* lost & found directory */

off_t	maxblk;			/* largest logical blk in file */
off_t	bmapsz;			/* num chars in blockmap */

daddr_t	n_ffree;		/* number of free fragments */
daddr_t	n_bfree;		/* number of free blocks */
daddr_t	n_blks;			/* number of blocks used */
daddr_t	n_files;		/* number of files seen */
#ifdef ACLS
daddr_t	n_cont;			/* number of continuation inodes seen */
#endif /* ACLS */
daddr_t	n_index;
daddr_t	n_bad;
daddr_t	fmax;			/* number of blocks in the volume */

daddr_t	badblk;
daddr_t	dupblk;

int	inosumbad;
int	offsumbad;
int	frsumbad;
int	sbsumbad;

#define	setbmap(x)	setbit(blockmap, x)
#define	getbmap(x)	isset(blockmap, x)
#define	clrbmap(x)	clrbit(blockmap, x)

#define	setfmap(x)	setbit(freemap, x)
#define	getfmap(x)	isset(freemap, x)
#define	clrfmap(x)	clrbit(freemap, x)

#define	ALTERED	010
#define	KEEPON	04
#define	SKIP	02
#define	STOP	01

long	lseek();
time_t	time();
DINODE	*ginode();
DIRECT	*fsck_readdir();
BUFAREA	*getblk();
int	catch();
int	findino(), mkentry(), chgdd();
int	pass1check(), pass1bcheck(), pass2check(), pass4check();
char	*rawname(), *unrawname();
char	*calloc(), *strcpy(), *strcat(), *strrchr();
extern int inside[], around[];
extern unsigned char *fragtbl[];

extern char *optarg;
extern int optind, opterr;

char	*devname;

extern int errno;
extern char *sys_errlist[];

/*
 * 	main - read command-line input.  Ensure they're compatible.
 *	Read checklist entries and check file systems in proper
 *	order (in parallel if using -P or -p options.
 */
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int c;
	FILE *mntfp;
	struct mntent *mnt;
	int pid, passno, anygtr, sumstatus, exit8;

#ifdef QFS_CMDS
	struct preen_list_entry *new_dev;
	struct preen_list_entry *next_dev;
	struct preen_list_entry *next_entry;
	char pid_found;
#endif /* QFS_CMDS */

#if defined(SecureWare) && !defined(STANDALONE)
	if (ISSECURE) {
	    set_auth_parameters(argc, argv);
	    if (!authorized_user("sysadmin")) {
                fprintf (stderr,
                  "fsck: you must have the 'sysadmin' authorization\n");
		exit(1);
	    }
	}
#ifdef B1
	if (ISB1) {
	    initprivs();
	    (void) forcepriv(SEC_ALLOWMACACCESS);
	}
#endif /* B1 */
#endif /* SecureWare && !STANDALONE */
	sync();
	fflag = 0;
#ifdef CACHE	
	while ((c = getopt(argc, argv, "PpdqnFNyYsStDfb:c:") ) != EOF) {
#else
	while ((c = getopt(argc, argv, "PpdqnFNyYsStDfb:") ) != EOF) {		
#endif
		switch (c) {

		case 'P':
			pclean++;
			preen++;
			break;


		case 'p':
			preen++;
			break;

		case 'b':
			bflag = atoi(optarg);
			if (bflag == 0) {
				fprintf(stderr, "invalid alternate super block\n");
				exit(8);
			}
			printf("Alternate super block location: %d\n", bflag);
			break;

#ifdef CACHE
		case 'c': 
			cache_size = atoi(optarg);
			break;
#endif
			
		case 'd':
			debug++;
			break;

		case 'q':
			qflag++;
			break;
		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		case 'F':	/* default yes to answer force to check */
			fflag++;
			break;
		case 's':
		case 'S':
		case 't':
		case 'D':
		case 'f':
			printf("Warning: -%c option is not supported for this filesystem\n", **argv);
			break;

		default:
			usage();
			exit(8);
		}
	}

	if (nflag && preen)
		errexit("Incompatible options: -n and -p\n");
	if (nflag && qflag)
		errexit("Incompatible options: -n and -q\n");

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catch);
	if (argc > optind) {
		exit8 = 0;
		while (argc > optind) {
#ifdef QFS_CMDS
			run_fsck_on_qfsck_fail = 1;
#endif /* QFS_CMDS */
			if (checkfilesys(argv[optind++])==0)
				exit8 = 1;
		}
		if (exit8)
			exit(8);
		else
			exit(0);
	}
	sumstatus = 0;
	passno = 1;
	/* process file system entries which have optional fields */
	do {
		anygtr = 0;
		if ((mntfp = setmntent(MNT_CHECKLIST, "r")) == 0)
			errexit("Can't open checklist file: %s\n", MNT_CHECKLIST);
		/* read file system entry from /etc/checklist
		 * skip non-HFS file systems (i.e. swap, ignore, nfs)
  		 * skip file system which does not have optional fields
		 */
		while ((mnt = getmntent(mntfp)) != 0) {
			if (strcmp(mnt->mnt_type, MNTTYPE_HFS) != 0 ||
 			    mnt->mnt_passno == -1)
				continue;
			if (preen == 0 ||
			    passno == 1 && mnt->mnt_passno == passno) {
#ifdef QFS_CMDS
				run_fsck_on_qfsck_fail = 1;
#endif /* QFS_CMDS */
				if (blockcheck(mnt->mnt_fsname) == 0 && preen)
					exit(8);
			} else if (mnt->mnt_passno > passno)
				anygtr = 1;
			else if (mnt->mnt_passno == passno) {
#ifdef QFS_CMDS
				/* build pid/dev file/retry list, fill in dev */
				new_dev = (struct preen_list_entry *)malloc(sizeof(struct preen_list_entry));
				if (new_dev == NULL)
					exit(8);
				new_dev->dev_name = mnt->mnt_fsname;
				new_dev->retry = 0;
				new_dev->next_dev = NULL;
				if (preen_list == NULL)
					preen_list = new_dev;
				else {
					next_entry = preen_list;
					while (next_entry->next_dev != NULL)
						next_entry = next_entry->next_dev;
					next_entry->next_dev = new_dev;
				}
#endif /* QFS_CMDS */
				pid = fork();
				if (pid < 0) {
					perror("fork");
					exit(8);
				}
				if (pid == 0) {
#ifdef QFS_CMDS
					run_fsck_on_qfsck_fail = 0;
#endif /* QFS_CMDS */
				/* 
				 * close the stdin of the child that 
				 * just forked to avoid too many children
				 * processes competing the terminal. 
				 */
					close (0);
					if (blockcheck(mnt->mnt_fsname)==0)
						exit(8);
#ifdef QFS_CMDS
					else if (blockcheck(mnt->mnt_fsname)==-1)
						exit(4);
#endif /* QFS_CMDS */
					else
						exit(0);
				}
#ifdef QFS_CMDS
				/* if pid > 0, add pid to list entry */
				if (pid > 0) {
					new_dev->pid = pid;
				}
#endif /* QFS_CMDS */
			}
		}
		if (preen) {
			union wait status;
#ifdef QFS_CMDS
			while ((pid = wait(&status)) != -1) {
				/* if 4 is returned from child, set retry bit for pid */
				if (WEXITSTATUS(status.w_status) == 4) {
					next_dev = preen_list;
					pid_found = 0;
					while ((next_dev != NULL) && (!pid_found)) {
						if (next_dev->pid == pid)
							pid_found = 1;
						else next_dev = next_dev->next_dev;
					}
					next_dev->retry = 1;
					/* clear 4 return value from status */
					status.w_retcode = 0;
				}
#else /* QFS_CMDS */
			while (wait(&status) != -1) {
#endif /* QFS_CMDS */
				sumstatus |= status.w_retcode;
			}
		}
		passno++;
	} while (anygtr);

	/* process minimal file system entries (i.e. entries which don't
	 * have any optional fields
	 */

	endmntent(mntfp);			/* reset checklist file */
	if ((mntfp = setmntent(MNT_CHECKLIST, "r")) == 0)
		errexit("Can't open checklist file: %s\n", MNT_CHECKLIST);
	while ((mnt = getmntent(mntfp)) != 0)
	{
		if (strcmp(mnt->mnt_type, MNTTYPE_HFS) != 0 ||
 		    mnt->mnt_passno != -1)
			continue;
		if (blockcheck(mnt->mnt_fsname) == 0)
			exit(8);
	}
#ifdef QFS_CMDS
	next_dev = preen_list;
	while (next_dev != NULL) {
		if (next_dev->retry) {
			run_fsck_on_qfsck_fail = 1;
			/* do we need to check if this dev is root? */
			if (blockcheck(next_dev->dev_name) == 0)
				exit(8);         
		}
		next_dev = next_dev->next_dev;
	}
	/* will these malloc'ed structures be free'd on exit? */
#endif /* QFS_CMDS */

	if (sumstatus)
		exit(8);
	(void)endfsent();
	exit(0);
}


#ifdef CACHE

/*
 *	initialize block cache - must do this before DI cache because
 *	we'll use this one to read in the superblock, which the DI cache
 *	uses to see how big it should be...
 */
b_cache_init()
{
	int i;
	

	if (cache_size > CACHE_SIZE)		/*  necessary?  */
		cache_size = CACHE_SIZE;
	for (i = 0; i < cache_size; i++) {
		bc[i] = (struct cache *) malloc(sizeof(struct cache));
		if (bc[i] == NULL) {
			cache_size = i;
			break;
		}
		bc[i]->b_blkno = 0;
	}
}


/*
 *	initialize dir inode cache - every directory inode in the fs will
 *	need a slot here, and it's a hash table, so it needs to be bigger
 *	than what will actually be used
 */
di_cache_init()
{
	int i;
	

	/*  XXX keep array of primes?  */
	di_tbl_size = 2*sblock.fs_cstotal.cs_ndir + 1;	 /*  good enough?  */
	di_tbl = (struct di_cache *) calloc(di_tbl_size, sizeof(struct di_cache));
	if (di_tbl == NULL)
		di_tbl_size = 0;
	else if (debug)  
		printf("calloced %d bytes for %d dir inodes\n", 
			di_tbl_size*sizeof(struct di_cache),
			sblock.fs_cstotal.cs_ndir);
	di_tbl_end = di_tbl + di_tbl_size;

	dc_hits = dc_srches = dc_taken = 0;
	for (i = 0; i < di_tbl_size; i++)
		di_tbl[i].key = 0;
}



cache_free()
{
	int i;
	

	for (i = 0; i < cache_size; i++) 
		if (bc[i])
			free(bc[i]);
		else if (debug)
			printf("cache_sz = %d; bc[%d] = NULL\n", cache_size, i);
			
	if (di_tbl && (di_tbl_size > 0)) {
		for (i = 0; i < di_tbl_size; i++)
			di_tbl[i].key = 0;	/*  paranoia!  */
		free(di_tbl);		
	}
}

#endif


/* 
 * is_hotroot -  returns 1 for the following conditions:
 *          - we are checking the root file system, and
 *          - we are not in pre_init_rc
 * 	in pre_init_rc means:
 *    	  root is mounted read-only (LVM and non-LVM)
 *    	  st_rdev is -1 (non-LVM only)
 *
 * Keep in mind that the st_dev of "/" should be the same as the st_rdev of 
 * the root block device.
 *
 * The general algorithm is as follow:
 *    stat the "/" 
 *    stat the fs_device that passed in
 *    if st_rdev is -1, return 0
 *    if the fs_device is not a block device then convert it to block mode
 *    stat the block version of fs_device 
 *    if it is not block device, return 0
 *    if st_rdev doesn't match /'s st_dev, return 0
 *    if "/" is read only, return 0 
 *    return 1
 */
is_hotroot(fs_device)
	char *fs_device;
{
	struct stat stslash, stblock, stchar;
	char blockdev[BUFSIZ];

        strcpy(blockdev,fs_device);
	if (stat("/", &stslash) < 0) {
		error("Can't stat root\n");
		return (0);
	}

	if (stat(blockdev, &stblock) < 0) {
		error("Can't stat %s\n", blockdev);
		return (0);
	}

	if (is_pre_init(stblock.st_rdev))
		return(0);

	/* convert the device name to be block device name */
	if ((stblock.st_mode & S_IFMT) == S_IFCHR) {
		unrawname(blockdev);
		if (stat(blockdev, &stblock) < 0) 
			return (0);
	}

	if ((stblock.st_mode & S_IFMT) != S_IFBLK) 
		return(0);

	if (stslash.st_dev != stblock.st_rdev) 
		return(0);

 	/* is root file system mounted read only? */
	if (is_roroot())
		return(0);

	return(1);
}

/*
 * Are we in pre_init_rc ?
 * rdev == -1 means we are in pre_init_rc
 */
is_pre_init(rdevnum)
dev_t rdevnum;
{
	if (rdevnum == -1)
		return (1);
	return (0);
}

/* 
 * is root file system read only? 
 */
is_roroot()
{
	if ( chown("/",UID_NO_CHANGE,GID_NO_CHANGE) == 0 )
		return(0);
	else if ( errno != EROFS ) {
		   printf ("fsck: chown failed: %s\n",sys_errlist[errno]);
		   return (0);
	}
	return(1);
}

/*
 *	blockcheck - Find out if we're checking the root file system and
 *	set the hotroot flag (so we know to reboot if its modified).  If
 *	it is the root file system, check the block device.  If not,
 *	change it to be raw and check the raw device.
 */
blockcheck(name)
	char *name;
{
	struct stat stslash, stblock, stchar;
	char *raw;
	int looped = 0;

	if (stat("/", &stslash) < 0) {
		error("Can't stat root\n");
		return (0);
	}
retry:
	if (stat(name, &stblock) < 0) {
		error("Can't stat %s\n", name);
		return (0);
	}
	if (stslash.st_dev == stblock.st_rdev) {
	/* This is the root device, check it as named */
#ifdef QFS_CMDS
		if (checkfilesys(name) == -1)
			return(-1);
		else return (1);
#else /* QFS_CMDS */
		if (checkfilesys(name)==0)
			return (0);
		else
			return (1);
#endif /* QFS_CMDS */

	} else if ((stblock.st_mode & S_IFMT) == S_IFBLK) {
	/* a non-root block device, check the raw version */
		raw = rawname(name);
		if (stat(raw, &stchar) < 0) {
			error("Can't stat %s\n", raw);
			return (0);
		}
		if ((stchar.st_mode & S_IFMT) == S_IFCHR) {
#ifdef QFS_CMDS
			if (checkfilesys(raw) == -1)
				return(-1);
			else return (1);
#else /* QFS_CMDS */
			if (checkfilesys(raw)==0)
				return (0);
			else
				return (1);
#endif /* QFS_CMDS */

		} else {
			error("%s is not a character device\n", raw);
			return (0);
		}
	} else if (stblock.st_mode & S_IFCHR) {
	/* a raw device name was passed */
		if (looped) {
			error("Can't make sense out of name %s\n", name);
			return (0);
		}
		name = unrawname(name);
		looped++;
		goto retry;
	}
	error("Can't make sense out of name %s\n", name);
	return (0);
}

/*
 *	checkfilesys - this is the driver of the other routines.
 *	First, call setup, then call pass1-pass5.  Print out the
 *	summary statistics which have been gathered.  Mark the
 *	fs_clean flag to be clean.  If we've modified the file
 *	system, print message and if we were checking root, sync
 *	and ask for a reboot.
 */
checkfilesys(filesys)
char *filesys;
{
	int ret_val;
#ifdef QFS_CMDS
	char qfsck_command[BUFSIZ];	/* command line for qfsck call */
	int qfsck_return_code;
	struct stat stat_buf;
#endif /* QFS_CMDS */

	fixed = 1;		/* set to 0 by any 'no' reply */
#ifdef CACHE
	b_cache_init();
#endif
	
#ifdef QFS_CMDS
	if (is_QFS(filesys)) {

		/* check for existence of qfsck, if not there, run fsck */
		if (stat("/etc/qfsck", &stat_buf) == 0) {
			sprintf(qfsck_command, "/etc/qfsck %s", filesys);
			qfsck_return_code = system(qfsck_command);
			if (qfsck_return_code == 0)
				return;
			if ((qfsck_return_code == 1) && (!run_fsck_on_qfsck_fail)) {
				/* add to list of fs's to retry */
				return(-1);
			}
		}
	}
#endif /* QFS_CMDS */

	devname = filesys;
	ret_val = setup(filesys);
	if (ret_val == 0) {
#if defined(SecureWare) && defined(B1)
                sprintf(auditbuf, "check file system %s", filesys);
                audit_subsystem(auditbuf,
                        "can't check file system, error in setup",
                        ET_SYS_ADMIN);
#endif
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
#ifdef CACHE
		cache_free();
#endif					
		return(0);
	}  else if (ret_val == -1) {	/* pclean && FS_CLEAN */
#ifdef CACHE
		cache_free();
#endif					
		return(1);
	}


#ifdef CACHE
	di_cache_init();
	cache_useful = 0;
	first_inode = 1;
	last_inode = 0;
	cache_hits = cache_misses = 0;
#endif
	 
/* 1: scan inodes tallying blocks used */
	if (preen == 0) {
		printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
		if (hotroot)
			printf("** Root file system\n");
		printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

#ifdef CACHE
	if (di_tbl_size > 0)
		cache_useful = 1;
#endif	

/* 1b: locate first references to duplicates, if any */
	if (enddup != &duplist[0]) {
		if (preen)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

/* 2: traverse directories from root to mark all connected directories */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

/* 3: scan inodes looking for disconnected directories */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

/* 4: scan inodes looking for disconnected files; check reference counts */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();

/* 5: check resource counts in cylinder groups */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();

	if (fixcg) {
		if (preen == 0)
			printf("** Phase 6 - Salvage Cylinder Groups\n");
		makecg();
		n_ffree = sblock.fs_cstotal.cs_nffree;
		n_bfree = sblock.fs_cstotal.cs_nbfree;
	}

#ifdef CACHE
	if (debug) {
		printf("Cache hits = %d, misses = %d, hit rate = %d%%, inodirty = %d\n", 
			cache_hits, cache_misses, cache_hits*100/(cache_hits+cache_misses), dirty_count);
		printf("DC hits = %d, misses = %d\n", dc_hits, dc_srches-dc_hits);
	}		
#endif

#ifdef DEBUG
	printf ("used = %d\n",
	    n_blks - howmany(sblock.fs_cssize, sblock.fs_fsize));
	printf ("free = %d\n",
	    n_ffree + sblock.fs_frag * n_bfree);
#endif /* DEBUG */

#if defined(ACLS) && !defined(B1)
	pwarn("%d files, %d icont, %d used, %d free (%d frags, %d blocks)\n",
	    n_files, n_cont, n_blks - howmany(sblock.fs_cssize, sblock.fs_fsize),
#else /* no ACLS */
	pwarn("%d files, %d used, %d free (%d frags, %d blocks)\n",
	    n_files, n_blks - howmany(sblock.fs_cssize, sblock.fs_fsize),

#endif /* ACLS */
	    n_ffree + sblock.fs_frag * n_bfree, n_ffree, n_bfree);

#ifdef OLD_RFA
	if (n_rfa_files > 0) {
	    pwarn("***** Encountered %d obsolete network special file%s",
		n_rfa_files, n_rfa_files > 1 ? "s\n" : "\n");
	    pwarn("***** These must be removed before the next release of HP-UX\n");
	}
#endif /* OLD_RFA */

	/* if user's specification denotes that the file system block
         * is going to be modified (nflag == 0) then fsck store the
         * correct magic number in the super block if it is not already
 	 * there
	 */
	if (!nflag && !(dfile.wfdes < 0)) {
		if (!hotroot) {
			if (fixed && (sblock.fs_clean != FS_CLEAN)) {
				if (!preen && !qflag)
					printf("***** MARKING FILE SYSTEM CLEAN *****\n");
				sblock.fs_clean = FS_CLEAN;
				dfile.mod++;
			}
		}
		else {  	/* hotroot */
			/* fix FS_CLEAN if changes made and no 'no' replies */
			if (dfile.mod && fixed)
				sblock.fs_clean = FS_CLEAN;
/*
 *  Fix fs_clean if there were no 'no' replies.
 *  This is done for both the s300 and s800.  The s800 root will be
 *  guaranteed clean as of 7.0.
 */
			if (fixed && (sblock.fs_clean != FS_OK)) {
				if (!preen && !qflag)
					printf("***** MARKING FILE SYSTEM CLEAN *****\n");
				sblock.fs_clean = FS_CLEAN;
				dfile.mod++;
			}
		}
	}

	if (dfile.mod) {
		(void)time(&sblock.fs_time);
		sbdirty();
	}
	ckfini();
#ifdef CACHE
	cache_free();
#endif		
	free(blockmap);
	free(freemap);
	free(statemap);
	free((char *)lncntp);
	if (!dfile.mod)
		return;
	/* don't print if qflag */
	/* otherwise print only if root or not preening */
	if (!qflag && (!preen || hotroot))
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
	if (hotroot) {
		printf("\n***** REBOOT HP-UX; DO NOT SYNC (USE reboot -n) *****\n");
		/* Don't sync if the raw mode of root fs name is used. */
		if (!rawflg)
			sync();
		exit(4);
	}
}

#ifdef QFS_CMDS
is_QFS(filesys)
char *filesys;

{
	daddr_t super = bflag ? bflag : SBLOCK;

	if ((dfile.rfdes = open(filesys, 0)) < 0) {
		error("Can't open %s\n", filesys);
		return(0);
	}

	if (bread(&dfile, &sblock, super, (long)SBSIZE) == 0)
		return(0);

	close(dfile.rfdes);

	if (sblock.fs_featurebits & FSF_QFS)
		return(1);
	else return(0);
}
#endif /* QFS_CMDS */


/*  setup(dev)
 *	return values:
 *		-1 : don't need to check this file system because
 *			pclean > 0 and fs.fs_clean == FS_CLEAN
 *			or, on the 200/300,
 *			pclean > 0 and hotroot > 0 and fs.fs_clean == FS_OK
 *		 0 : couldn't complete setup; can't check file system
 *		 1 : all okay
 */
setup(dev)
	char *dev;
{
	struct stat statb;
	daddr_t super = bflag ? bflag : SBLOCK;
	int i, j, c, d, cgd;
	int mounted, swap;
	struct stat st_mounted;
	long size;
	BUFAREA asblk;
#	define altsblock asblk.b_un.b_fs
	BUFAREA asblk_1;
#	define altsblock_1 asblk_1.b_un.b_fs

	if (stat(dev, &statb) < 0) {
		error("Can't stat %s\n", dev);
		return (0);
	}
	rawflg = 0;
	if ((statb.st_mode & S_IFMT) == S_IFBLK)
		;
	else if ((statb.st_mode & S_IFMT) == S_IFCHR)
		rawflg++;
	else {
		printf("file is not a block or character device\n");
		if (preen || reply("OK") == 0)
			return (0);
	}

	hotroot = is_hotroot(dev);

{
	/*
	 * The following code is added to improve usability of fsck.
	 * we need to give user a warning if the device being checked is
	 * a hotroot, a mounted file system, or a swap device.
	 * The rules are:
	 *	1) if nflag is set, it's pretty safe to fsck the target dev
	 *	2) if the target device is a swap, exit
	 *	3) if hotroot is set, and "-F" is not specified prompt the 
	 *		user and wait for reply
	 *	4) if the target is a mounted file system, and "-F" is not
	 *		specified, prompt the user and wait for reply
	 *
	 * UCSqm00289, DSDe406844...
	 * Caveat: There is no way to tell the current run level, so we cannot
	 * tell whether or not we are in single user mode.
	 */
	 if (!nflag) {
		mounted = swap = 0;
		if (!hotroot) {
			mounted = is_mounted(dev,&st_mounted);
			swap = is_swap(st_mounted.st_rdev);
		}
		if (!fflag) {
			if (hotroot) {
				printf("fsck: %s: root file system",dev);
				if (!freply("continue (y/n)"))
					return (0);
			}
			else if (mounted) {
				printf("fsck: %s: mounted file system",dev);
				if (!freply("continue (y/n)"))
					return (0);
			}
			else if (swap) {
				printf("fsck: %s: swap device\n",dev);
				if (!freply("continue (y/n)"))
					return (0);
			}
		}
	}
}

#if defined(SecureWare) && defined(B1) && !defined(STANDALONE)
	if (ISB1)
	    (void) forcepriv(SEC_ALLOWDACACCESS);
#endif
	if ((dfile.rfdes = open(dev, 0)) < 0) {
		error("Can't open %s\n", dev);
#if defined(SecureWare) && defined(B1) && !defined(STANDALONE)
		if (ISB1)
		    (void) disablepriv(SEC_ALLOWDACACCESS);
#endif
		return (0);
	}
	if (preen == 0)
		printf("** %s", dev);
	if (nflag || (dfile.wfdes = open(dev, 1)) < 0) {
		dfile.wfdes = -1;
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf(" (NO WRITE)");
	}
#if defined(SecureWare) && defined(B1) && !defined(STANDALONE)
	if (ISB1)
	    (void) disablepriv(SEC_ALLOWDACACCESS);
#endif
	if (preen == 0)
		printf("\n");
	fixcg = 0; inosumbad = 0; offsumbad = 0; frsumbad = 0; sbsumbad = 0;
	dfile.mod = 0;
	cached_block = -1;
#ifdef ACLS
	n_files = n_blks = n_ffree = n_bfree = n_cont = 0;
#else /* no ACLS */
	n_files = n_blks = n_ffree = n_bfree = 0;
#endif /* ACLS */
#ifdef OLD_ACLS
	nukeacls = 0;
#endif /* OLD_ACLS */
	muldup = enddup = &duplist[0];
	badlnp = &badlncnt[0];
	lfdir = 0;
	rplyflag = 0;
	initbarea(&sblk);
	initbarea(&fileblk);
	initbarea(&inoblk);
	initbarea(&cgblk);
	initbarea(&asblk);
	initbarea(&asblk_1);
	/*
	 * Read in the super block and its summary info.
	 */
	if (bread(&dfile, (char *)&sblock, super, (long)SBSIZE) == 0)
		return (0);
	sblk.b_bno = super; /* SBLOCK or set by -b option */
	sblk.b_size = SBSIZE;
	/*
	 * Do we need to continue ?
	 */
	if (pclean && sblock.fs_clean == FS_CLEAN)
		return(-1);
	if (pclean && hotroot && sblock.fs_clean == FS_OK) 
		return(-1);
	/*
	 * run a few consistency checks of the super block
	 */
#if defined(FD_FSMAGIC)
	if ((sblock.fs_magic != FS_MAGIC) && (sblock.fs_magic != FS_MAGIC_LFN)
		&& (sblock.fs_magic != FD_FSMAGIC))
#else /* not new magic number */
	if ((sblock.fs_magic != FS_MAGIC) && (sblock.fs_magic != FS_MAGIC_LFN))
#endif /* new magic number */
		{ badsb("MAGIC NUMBER WRONG"); return (0); }
#ifdef LONGFILENAMES
#if defined(FD_FSMAGIC)
	if ((sblock.fs_magic == FS_MAGIC_LFN) || (sblock.fs_featurebits & FSF_LFN))
#else /* not new magic number */
	if (sblock.fs_magic == FS_MAGIC_LFN)
#endif /* new magic number */
		longfilenames = 1;
	else
		longfilenames = 0;
#endif /* LONGFILENAMES */
	if (sblock.fs_ncg < 1)
		{ badsb("NCG OUT OF RANGE"); return (0); }
	if (sblock.fs_cpg < 1 || sblock.fs_cpg > MAXCPG)
		{ badsb("CPG OUT OF RANGE"); return (0); }
	if (sblock.fs_ncg * sblock.fs_cpg < sblock.fs_ncyl ||
	    (sblock.fs_ncg - 1) * sblock.fs_cpg >= sblock.fs_ncyl)
		{ badsb("NCYL DOES NOT AGREE WITH NCG*CPG"); return (0); }
	if (sblock.fs_sbsize > SBSIZE)
		{ badsb("SIZE PREPOSTEROUSLY LARGE"); return (0); }
#if defined(SecureWare) && defined(B1)
	if (ISB1)
	    disk_set_file_system(&sblock);
#endif
	/*
	 * Set all possible fields that could differ, then do check
	 * of whole super block against an alternate super block.
	 * When an alternate super-block is specified this check is skipped.
	 */
	if (bflag)
		goto sbok;
	if (getblk(&asblk, cgsblock(&sblock, sblock.fs_ncg - 1),
	    sblock.fs_sbsize) == 0)
		return (0);
	altsblock.fs_link = sblock.fs_link;
	altsblock.fs_rlink = sblock.fs_rlink;
	altsblock.fs_time = sblock.fs_time;
	altsblock.fs_cstotal = sblock.fs_cstotal;
	altsblock.fs_cgrotor = sblock.fs_cgrotor;
	altsblock.fs_fmod = sblock.fs_fmod;
	altsblock.fs_clean = sblock.fs_clean;
	altsblock.fs_ronly = sblock.fs_ronly;
	altsblock.fs_flags = sblock.fs_flags;
	altsblock.fs_maxcontig = sblock.fs_maxcontig;
	altsblock.fs_minfree = sblock.fs_minfree;
	altsblock.fs_rotdelay = sblock.fs_rotdelay;
	altsblock.fs_maxbpg = sblock.fs_maxbpg;
	altsblock.fs_mirror = sblock.fs_mirror;
	memcpy((char *)altsblock.fs_csp, (char *)sblock.fs_csp,
		sizeof sblock.fs_csp);
	memcpy((char *)altsblock.fs_fsmnt, (char *)sblock.fs_fsmnt,
		sizeof sblock.fs_fsmnt);
	if (memcmp((char *)&sblock, (char *)&altsblock, (int)sblock.fs_sbsize))
        { /* Determine which super block was bad. first or last ? */
	  if (getblk(&asblk_1, cgsblock(&sblock, sblock.fs_ncg - 2),
	      sblock.fs_sbsize) == 0)
              { badsb("TRASHED VALUES IN SUPER BLOCK"); return (0); }

	altsblock_1.fs_link = sblock.fs_link;
	altsblock_1.fs_rlink = sblock.fs_rlink;
	altsblock_1.fs_time = sblock.fs_time;
	altsblock_1.fs_cstotal = sblock.fs_cstotal;
	altsblock_1.fs_cgrotor = sblock.fs_cgrotor;
	altsblock_1.fs_fmod = sblock.fs_fmod;
	altsblock_1.fs_clean = sblock.fs_clean;
	altsblock_1.fs_ronly = sblock.fs_ronly;
	altsblock_1.fs_flags = sblock.fs_flags;
	altsblock_1.fs_maxcontig = sblock.fs_maxcontig;
	altsblock_1.fs_minfree = sblock.fs_minfree;
	altsblock_1.fs_rotdelay = sblock.fs_rotdelay;
	altsblock_1.fs_maxbpg = sblock.fs_maxbpg;
	altsblock_1.fs_mirror = sblock.fs_mirror;
	memcpy((char *)altsblock_1.fs_csp, (char *)sblock.fs_csp,
		sizeof sblock.fs_csp);
	memcpy((char *)altsblock_1.fs_fsmnt, (char *)sblock.fs_fsmnt,
		sizeof sblock.fs_fsmnt);

	if (memcmp((char *)&sblock, (char *)&altsblock_1, (int)sblock.fs_sbsize) == 0)
         { /*  Fix last alternate super block */
              (void)bwrite(&dfile, (char *)&sblock,
                     fsbtodb(&sblock, cgsblock(&sblock, sblock.fs_ncg - 1)),
                           SBSIZE);
           }
	else
         {badsb("TRASHED VALUES IN SUPER BLOCK"); return (0); }

        } /* end of if 1st and last super block not equal */

sbok:
	fmax = sblock.fs_size;
	imax = sblock.fs_ncg * sblock.fs_ipg;
	n_bad = cgsblock(&sblock, 0); /* boot block plus dedicated sblock */
	/*
	 * read in the summary info.
	 */
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		size = sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize;
		sblock.fs_csp[j] = (struct csum *)calloc(1, (unsigned)size);
		if (bread(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    size) == 0)
			return (0);
	}
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsz = roundup(howmany(fmax, NBBY), sizeof(short));
	blockmap = calloc((unsigned)bmapsz, sizeof (char));
	if (blockmap == NULL) {
		printf("cannot alloc %d bytes for blockmap\n", bmapsz);
		goto badsb;
	}
	freemap = calloc((unsigned)bmapsz, sizeof (char));
	if (freemap == NULL) {
		printf("cannot alloc %d bytes for freemap\n", bmapsz);
		goto badsb;
	}
	statemap = calloc((unsigned)(imax + 1), sizeof(char));
	if (statemap == NULL) {
		printf("cannot alloc %d bytes for statemap\n", imax + 1);
		goto badsb;
	}
	lncntp = (short *)calloc((unsigned)(imax + 1), sizeof(short));
	if (lncntp == NULL) {
		printf("cannot alloc %d bytes for lncntp\n",
		    (imax + 1) * sizeof(short));
		goto badsb;
	}
	for (c = 0; c < sblock.fs_ncg; c++) {
		cgd = cgdmin(&sblock, c);
		if (c == 0) {
			d = cgbase(&sblock, c);
			cgd += howmany(sblock.fs_cssize, sblock.fs_fsize);
		} else
			d = cgsblock(&sblock, c);
		for (; d < cgd; d++)
			setbmap(d);
	}

	return (1);

badsb:
	ckfini();
	return (0);
#	undef altsblock
#	undef altsblock_1
}

/*
 *	pass1 - check blocks and sizes
 *	Walk through each inode and, if it is allocated:
 *	    Ensure that the cylinder group inode
 *              used information is correct.
 *	    From the inode's ic_size information, figure
 *              number of blocks.
 *	    Skip the number of blocks which should be used, and check
 *		that the direct and indirect addresses which should not
 *		be used are not used.
 *	    Calls ckinode which calls pass1check to check blocks
 *              which are used.
 *	    Put either DSTATE or FSTATE in the statemap
 *              if directory or not.
 *	    Keep track of link count info in lncntp[].
 *	    If it is a FIFO, make sure it is empty.
 *	    If it is a continuation inode, add one to n_cont counter and
 *		put CSTATE in the statemap.
 */
pass1()
{
	register int c, i, n, j;
	register DINODE *dp;
	int ndb, partial;
	struct inodesc idesc;
	ino_t inumber;
	int badpipe;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1check;
	inumber = 0;
	n_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
	/*
	 * walk through each cylinder group in the file system
	 */
	for (c = 0; c < sblock.fs_ncg; c++) {
		if (getblk(&cgblk, cgtod(&sblock, c), sblock.fs_cgsize) == 0)
			continue;
		if (cgrp.cg_magic != CG_MAGIC) {
			pfatal("CG %d: BAD MAGIC NUMBER\n", c);
			memset((char *)&cgrp, 0, (int)sblock.fs_cgsize);
		}
		n = 0;
		/*
		 * check each inode in the cylinder group
		 */
		for (i = 0; i < sblock.fs_ipg; i++, inumber++) {
			dp = ginode(inumber);
			if (dp == NULL)
				continue;
			n++;
			/*
			 *if inode is allocated
			 */
			if (ALLOC) {
				if (!isset(cgrp.cg_iused, i)) {
					if (debug)
						printf("%d bad, not used\n",
						    inumber);
					inosumbad++;
				}
				n--;
				lastino = inumber;
#ifdef ACLS
				/*
				 * Don't check blocks and sizes of
				 * continuation inodes
				 */
 				if (CONT) {
#ifdef OLD_ACLS
				   /*
				    * If this is the first continuation inode,
			   	    * ask the user to remove ACLs.
				    * Force the user to do it manually.
				    */
				    if (n_cont == 0) {
				       printf("WARNING: This filesystem contains old ACLs.\n");
				       printf("These ACLs are ignored and will not be");
				       printf(" compatible with future releases.\n");
				       if (preen || qflag || yflag)
				          printf("Run fsck manually to remove them.\n");
				       else
				          nukeacls = reply("REMOVE");
				       if (nukeacls)
				          printf("All continuation inodes will be removed.\n");
				    }
#endif /* OLD_ACLS */
 				    statemap[inumber] = CSTATE;
 				    lncntp[inumber] = dp->di_nlink;
				    n_cont++;
				    continue;
 				}
#endif /* ACLS */
#ifdef OLD_RFA
				if ((dp->di_mode & IFMT) == IFNWK)
				    n_rfa_files++;
#endif /* OLD_RFA */
				if (!preen && BADBLK &&
				    reply("HOLD BAD BLOCK") == 1) {
					dp->di_size = sblock.fs_fsize;
					dp->di_mode = IFREG|0600;
					inodirty();
				} else if (ftypeok(dp) == 0)
					goto unknown;
				if (dp->di_size < 0) {
					if (debug)
						printf("bad size %d:",
							dp->di_size);
					goto unknown;
				}

				/* initialize all R/W activities of FIFO file */
				/* make sure FIFO is empty (everything is 0) */
				if ((dp->di_mode & IFMT) == IFIFO &&
				    (dp->di_frcnt!=0 || dp->di_fwcnt!=0))
			        {
					if (!qflag)
						pwarn("NON-ZERO READER/WRITER COUNT(S) ON PIPE I=%u",inumber);
					if (preen && !qflag)
						printf(" (CORRECTED)\n");
					else if (!qflag)
					{
						if (reply("CORRECT") == 0)
							goto no_reset;
					}
					dp->di_size = 0;
					dp->di_frptr = 0;
					dp->di_fwptr = 0;
					dp->di_frcnt = 0;
					dp->di_fwcnt = 0;
					dp->di_fflag = 0;
  					dp->di_fifosize = 0;
					inodirty();
					ndb = 0;
					for (j = ndb; j < NDADDR; j++)
						dp->di_db[j] = 0;
				}  /* if frcnt && fwcnt */
#ifdef IC_FASTLINK
				else if (FASTLNK)
				{
				    /*
				     * Fast symlink -- verify that the
				     * size is valid and that the length
				     * of the path is correct.
				     */
				    if (dp->di_size >= MAX_FASTLINK_SIZE)
				    {
					if (debug)
					    printf("bad fastlink size %d:",
						   dp->di_size);
					goto unknown;
				    }

				    dp->di_symlink[MAX_FASTLINK_SIZE-1] = '\0';
				    if (strlen(dp->di_symlink) != dp->di_size)
				    {
					int len = strlen(dp->di_symlink);

					pwarn("BAD SYMLINK SIZE, SHOULD BE %d: size = %d",
					    len, dp->di_size);
					if (preen)
					    printf(" (CORRECTED)\n");
					else
					{
					    printf("\n");
					    pinode(inumber);
					    if (reply("CORRECT") == 0)
						    continue;
					}
					dp->di_size = len;
					inodirty();
				    }
				    goto no_block_checks;
				}
#endif /* IC_FASTLINK */
				else
				{
				    /*
				     * get the number of blocks used in inode.
				     * the calculation is done explicitly rather
				     * than using the 'howmany' macro because
				     * howmany can overflow for file sizes
				     * very near the largest possible integer.
				     */
				    ndb = (dp->di_size/sblock.fs_bsize) +
				        ((dp->di_size%sblock.fs_bsize) ? 1 : 0);
				    if (SPECIAL)  ndb++;

				    /*
				     * walk through all unused (should be)
				     * direct addresses and ensure they are 0
				     */
				    for (j=ndb; j<NDADDR; j++) {
#if defined(DUX) || defined(CNODE_DEV)
					/*
					 * DUX uses db[2] on cnode-specific
					 * device files, so skip 'em
					 */
					if (j == 2 && SPECIAL)
					    continue;
#endif /* DUX || CNODE_DEV */
					if (dp->di_db[j] != 0) {
					    if (debug)
						printf("bad direct addr: %d\n",
							dp->di_db[j]);
					    pwarn("BAD DIRECT ADDRESS, SHOULD BE ZERO: inode.di_db[%d] = %d",
						    j, dp->di_db[j]);
					    if (preen)
						printf(" (CORRECTED)\n");
					    else {
						printf("\n");
						pinode(inumber);
						if (reply("CORRECT") == 0)
							continue;
					    }
					    dp->di_db[j] = 0;
					    inodirty();
					}
				    }
				}
no_reset:
				if (ndb > NDADDR)
					if (indircheck(dp, ndb, inumber)) {
						pfatal("COULD NOT CHECK INDIRECT BLOCKS");
						pinode(inumber);
						if (reply("CLEAR") == 1) {
							zapino(dp);
							statemap[inumber] = USTATE;
							inodirty();
							inosumbad++;
							continue;
						}
					}
				for (j = 0, ndb -= NDADDR; ndb > 0; j++)
					ndb /= NINDIR(&sblock);

				/*
				 * Walk through unused (should be) indirect
				 * addresses and ensure they are 0
				 */
				for (; j < NIADDR; j++) {
					if (dp->di_ib[j] != 0) {
						if ((dp->di_mode & IFMT) != IFIFO) {
							if (debug)
								printf("bad indirect addr: %d\n",
								dp->di_ib[j]);
							goto unknown;
						}
					}
				}
no_block_checks:
				/*
				 * Count number of files.
				 */
				n_files++;
				/*
				 * remember link counts for later
				 */
				lncntp[inumber] = dp->di_nlink;
				if (dp->di_nlink <= 0) {
					if (badlnp < &badlncnt[MAXLNCNT])
						*badlnp++ = inumber;
					else {
						pfatal("LINK COUNT TABLE OVERFLOW");
						if (reply("CONTINUE") == 0)
							errexit("");
					}
				}
				/*
				 * keep track of whether a directory or not
				 */
				statemap[inumber] = DIRCT ? DSTATE : FSTATE;
#ifdef ACLS
				/*
				 * keep track of associated contin inodes
				 */
				if (dp->di_contin != 0)
				    statemap[inumber] |= HASCINODE;
#endif /* ACLS */
				badblk = dupblk = 0; maxblk = 0;
				idesc.id_number = inumber;
				idesc.id_filesize = 0;

				/*
				 * This will check the blocks by calling
				 * the routine, pass1check
				 */
				(void)ckinode(dp, &idesc);
				idesc.id_filesize *= btodb(sblock.fs_fsize);
				if (dp->di_blocks != idesc.id_filesize) {
					if (!qflag)
					pwarn("INCORRECT BLOCK COUNT I=%u (%ld should be %ld)",
					    inumber, dp->di_blocks,
					    idesc.id_filesize);
					if (preen)
						printf(" (CORRECTED)\n");
					else if (!qflag)
					{
						if (reply("CORRECT") == 0)
						continue;
					}
					dp->di_blocks = idesc.id_filesize;
					inodirty();
				}
				continue;
		unknown:
				pfatal("UNKNOWN FILE TYPE ");
				pinode(inumber);
				if (reply("CLEAR") == 1) {
					zapino(dp);
					inodirty();
					inosumbad++;
				}
			} else {   /* inode mode is 0: not used */
				/*
				 * check cylinder group info
				 */
				if (isset(cgrp.cg_iused, i)) {
					if (debug)
						printf("%d bad, marked used\n",
						    inumber);
					inosumbad++;
					n--;
				}
				partial = 0;
				badpipe = 0;
				for (j = 0; j < NDADDR; j++)
					if (dp->di_db[j] != 0)
						partial++;
				for (j = 0; j < NIADDR; j++)
					if (dp->di_ib[j] != 0) {
						badpipe++;
						partial++;
					}
				if (partial || dp->di_mode != 0 ||
#ifdef ACLS
				    dp->di_size != 0 || dp->di_contin != 0) {
#else
				    dp->di_size != 0) {
#endif /* ACLS */
					if (badpipe) 
						pwarn("UNREF PIPE ");
					else
#ifdef ACLS
					    if (dp->di_contin != 0)
						pwarn("UNALLOCATED INODE HAS BAD ic_contin VALUE ");
					    else
#endif /* ACLS */
						pfatal("PARTIALLY ALLOCATED INODE ");

					pinode(inumber);
					if (badpipe && preen)
						printf(" (CLEARED)\n");
					if((badpipe && preen)||(reply("CLEAR") == 1)) {
						zapino(dp);
						inodirty();
						inosumbad++;
					}
				}
			}
		}
		if (n != cgrp.cg_cs.cs_nifree) {
			if (debug)
				printf("cg[%d].cg_cs.cs_nifree is %d; calc %d\n",
				    c, cgrp.cg_cs.cs_nifree, n);
			inosumbad++;
		}

		if (cgrp.cg_cs.cs_nbfree != sblock.fs_cs(&sblock, c).cs_nbfree
		  || cgrp.cg_cs.cs_nffree != sblock.fs_cs(&sblock, c).cs_nffree
		  || cgrp.cg_cs.cs_nifree != sblock.fs_cs(&sblock, c).cs_nifree
		  || cgrp.cg_cs.cs_ndir != sblock.fs_cs(&sblock, c).cs_ndir)
			sbsumbad++;
	}
}

/*
 *	pass1check - Checks that blocks (actually fragments) are
 *	not out of range and are not referenced by more than one
 *	inode.
 */
pass1check(idesc)
	register struct inodesc *idesc;
{
	register daddr_t *dlp;
	int res = KEEPON;
	int anyout, nfrags;
	daddr_t blkno = idesc->id_blkno;

	anyout = outrange(blkno, idesc->id_numfrags);

	/*
	 * walk through all blocks (indicated by id_numfrags)
	 * id_numfrags is set by ckinode
	 */
	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		/*
		 * Make sure block is in range
		 */
		if (anyout && outrange(blkno, 1)) {
			blkerr(idesc->id_number, "BAD", blkno);
			if (++badblk >= MAXBAD) {
				pwarn("EXCESSIVE BAD BLKS I=%u",
					idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			res = SKIP;
		/*
		 * Make sure block has not been referenced already
		 */
		} else if (getbmap(blkno)) {
			blkerr(idesc->id_number, "DUP", blkno);
			if (++dupblk >= MAXDUP) {
				pwarn("EXCESSIVE DUP BLKS I=%u",
					idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			if (enddup >= &duplist[DUPTBLSIZE]) {
				pfatal("DUP TABLE OVERFLOW.");
				if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			for (dlp = duplist; dlp < muldup; dlp++)
				if (*dlp == blkno) {
					*enddup++ = blkno;
					break;
				}
			if (dlp >= muldup) {
				*enddup++ = *muldup;
				*muldup++ = blkno;
			}
		/*
		 * if all is well, add this block to blockmap
		 */
		} else {
			n_blks++;
			setbmap(blkno);
		}
		idesc->id_filesize++;
	}
	return (res);
}

/*
 *	pass1b - if there were duplicate blocks found in pass1check,
 *	this is called to make another check.  It walks through all
 *	of the inodes and calls ckinode to call pass1bcheck.
 */
pass1b()
{
	register int c, i;
	register DINODE *dp;
	struct inodesc idesc;
	ino_t inumber;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1bcheck;
	inumber = 0;
	/*
	 * walk through cylinder groups and then inodes in the cg
	 */
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0; i < sblock.fs_ipg; i++, inumber++) {
			dp = ginode(inumber);
			if (dp == NULL)
				continue;
			idesc.id_number = inumber;
#ifdef ACLS
			if (((statemap[inumber] & STATE) != USTATE) &&
#else /* not ACLS */
			if (statemap[inumber] != USTATE &&
#endif /* ACLS */
			    (ckinode(dp, &idesc) & STOP))
				goto out1b;
		}
	}
out1b:
	flush(&dfile, &inoblk);
}

/*
 *	pass1bcheck - checks again for duplicate blocks
 */
pass1bcheck(idesc)
	register struct inodesc *idesc;
{
	register daddr_t *dlp;
	int nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;

	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (outrange(blkno, 1))
			res = SKIP;
		for (dlp = duplist; dlp < muldup; dlp++)
			if (*dlp == blkno) {
				blkerr(idesc->id_number, "DUP", blkno);
				*dlp = *--muldup;
				*muldup = blkno;
				if (muldup == duplist)
					return (STOP);
			}
	}
	return (res);
}

/*
 *	pass2 - Check Pathnames
 *	Traverses the directory structure to ensure it is in tact.
 *	This starts with the root inode and calls descend which calls
 *	ckinode which calls dirscan which calls pass2check which
 *	recursively calls descend and so on, and so on.
 */
pass2()
{
	register DINODE *dp;
	struct inodesc rootdesc;

	memset((char *)&rootdesc, 0, sizeof(struct inodesc));
	rootdesc.id_type = ADDR;
	rootdesc.id_func = pass2check;
	rootdesc.id_number = ROOTINO;
	pathp = pathname;
#ifdef ACLS
	switch (statemap[ROOTINO] & STATE) {
#else /* no ACLS */
	switch (statemap[ROOTINO]) {
#endif /* ACLS */

	case USTATE:
		perrexit("ROOT INODE UNALLOCATED. TERMINATING.\n");

	case FSTATE:
		pfatal("ROOT INODE NOT DIRECTORY");
		if (reply("FIX") == 0 || (dp = ginode(ROOTINO)) == NULL)
			errexit("");
		dp->di_mode &= ~IFMT;
		dp->di_mode |= IFDIR;
		inodirty();
		inosumbad++;
#ifdef ACLS
		/*
		 * Keep any info on associated continuation inode
		 */
		if (statemap[ROOTINO] & HASCINODE)
		    statemap[ROOTINO] = DSTATE|HASCINODE;
		else
		    statemap[ROOTINO] = DSTATE;
#else /* no ACLS */
		statemap[ROOTINO] = DSTATE;
#endif /* ACLS */
		/* fall into ... */

	case DSTATE:
		descend(&rootdesc, ROOTINO);
		break;

	case CLEAR:
		pfatal("DUPS/BAD IN ROOT INODE");
		printf("\n");
		if (reply("CONTINUE") == 0)
			errexit("");
#ifdef ACLS
		/*
		 * Keep any info on associated continuation inode
		 */
		if (statemap[ROOTINO] & HASCINODE)
		    statemap[ROOTINO] = DSTATE|HASCINODE;
		else
		    statemap[ROOTINO] = DSTATE;
#else /* no ACLS */
		statemap[ROOTINO] = DSTATE;
#endif /* ACLS */
		descend(&rootdesc, ROOTINO);
	}
}

/*
 *	pass2check - recursively checks the directory structure.
 */
pass2check(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;
	char *curpathloc;
	int n, entrysize, ret = 0;
	DINODE *dp;
	DIRECT proto;
#ifdef ACLS
	int holdstate;
#endif /* ACLS */

	/*
	 * check for "."
	 */
	if (idesc->id_entryno != 0)
		goto chk1;
	if (dirp->d_ino != 0 && dirp->d_namlen == 1 && dirp->d_name[0] == '.') {
		if (dirp->d_ino != idesc->id_number) {
			direrr(idesc->id_number, "BAD INODE NUMBER FOR '.'");
			dirp->d_ino = idesc->id_number;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		goto chk1;
	}
	direrr(idesc->id_number, "MISSING '.'");
	proto.d_ino = idesc->id_number;
	proto.d_namlen = 1;
	(void)strcpy(proto.d_name, ".");
#ifdef LONGFILENAMES
	entrysize = longfilenames?DIRSIZ(&proto):DIRSTRCTSIZ;
#else
	entrysize = sizeof (struct direct);
#endif

	if (dirp->d_ino != 0) {
		pfatal("CANNOT FIX, FIRST ENTRY IN DIRECTORY CONTAINS %s\n",
			dirp->d_name);
	} else if (dirp->d_reclen < entrysize) {
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '.'\n");
	} else if (dirp->d_reclen < 2 * entrysize) {
		proto.d_reclen = dirp->d_reclen;
		memcpy((char *)dirp, (char *)&proto, entrysize);
		if (reply("FIX") == 1)
			ret |= ALTERED;
	} else {
		n = dirp->d_reclen - entrysize;
		proto.d_reclen = entrysize;
		memcpy((char *)dirp, (char *)&proto, entrysize);
		idesc->id_entryno++;
		lncntp[dirp->d_ino]--;
		dirp = (DIRECT *)((char *)(dirp) + entrysize);
		memset((char *)dirp,0, n);
		dirp->d_reclen = n;
		if (reply("FIX") == 1)
			ret |= ALTERED;
	}
chk1:
	if (idesc->id_entryno > 1)
		goto chk2;
	proto.d_ino = idesc->id_parent;
	proto.d_namlen = 2;
	(void)strcpy(proto.d_name, "..");
#ifdef LONGFILENAMES
	entrysize = longfilenames?DIRSIZ(&proto):DIRSTRCTSIZ;
#else
	entrysize = sizeof (struct direct);
#endif

	if (idesc->id_entryno == 0) {
#ifdef LONGFILENAMES
		n = longfilenames?DIRSIZ(dirp):DIRSTRCTSIZ;
#else
		n = sizeof (struct direct);
#endif

		if (dirp->d_reclen < n + entrysize)
			goto chk2;
		proto.d_reclen = dirp->d_reclen - n;
		dirp->d_reclen = n;
		idesc->id_entryno++;
		lncntp[dirp->d_ino]--;
		dirp = (DIRECT *)((char *)(dirp) + n);
		memset((char *)dirp, 0, n);
		dirp->d_reclen = proto.d_reclen;
	}
	if (dirp->d_ino != 0 && dirp->d_namlen == 2 &&
	    strcmp(dirp->d_name, "..") == 0) {
		if (dirp->d_ino != idesc->id_parent) {
			direrr(idesc->id_number, "BAD INODE NUMBER FOR '..'");
			dirp->d_ino = idesc->id_parent;
			if (reply("FIX") == 1)
				ret |= ALTERED;
		}
		goto chk2;
	}
	direrr(idesc->id_number, "MISSING '..'");
	if (dirp->d_ino != 0) {
		pfatal("CANNOT FIX, SECOND ENTRY IN DIRECTORY CONTAINS %s\n",
			dirp->d_name);
	} else if (dirp->d_reclen < entrysize) {
		pfatal("CANNOT FIX, INSUFFICIENT SPACE TO ADD '..'\n");
	} else {
		proto.d_reclen = dirp->d_reclen;
		memcpy((char *)dirp, (char *)&proto, entrysize);
		if (reply("FIX") == 1)
			ret |= ALTERED;
	}
chk2:
	if (dirp->d_ino == 0)
		return (ret|KEEPON);
	if (idesc->id_entryno >= 2 &&
	    dirp->d_namlen <= 2 &&
	    dirp->d_name[0] == '.') {
		if (dirp->d_namlen == 1) {
			direrr(idesc->id_number, "EXTRA '.' ENTRY");
			dirp->d_ino = 0;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			return (KEEPON | ret);
		}
		if (dirp->d_name[1] == '.') {
			direrr(idesc->id_number, "EXTRA '..' ENTRY");
			dirp->d_ino = 0;
			if (reply("FIX") == 1)
				ret |= ALTERED;
			return (KEEPON | ret);
		}
	}
	curpathloc = pathp;
	*pathp++ = '/';
	if (pathp + dirp->d_namlen >= endpathname) {
		*pathp = '\0';
		perrexit("NAME TOO LONG %s%s\n", pathname, dirp->d_name);
	}
	memcpy(pathp, dirp->d_name, dirp->d_namlen + 1);
	pathp += dirp->d_namlen;
	idesc->id_entryno++;
	n = 0;
	if (dirp->d_ino > imax || dirp->d_ino <= 0) {
		direrr(dirp->d_ino, "I OUT OF RANGE");
		n = reply("REMOVE");
	} else {
again:
#ifdef ACLS
		switch (statemap[dirp->d_ino] & STATE) {
#else /* no ACLS */
		switch (statemap[dirp->d_ino]) {
#endif /* ACLS */
		case USTATE:
			direrr(dirp->d_ino, "UNALLOCATED");
			n = reply("REMOVE");
			break;

		case CLEAR:
			direrr(dirp->d_ino, "DUP/BAD");
			if ((n = reply("REMOVE")) == 1)
				break;
			if ((dp = ginode(dirp->d_ino)) == NULL)
				break;
#ifdef ACLS
			holdstate = DIRCT ? DSTATE : FSTATE;
			if (statemap[dirp->d_ino] & HASCINODE)
		    		statemap[dirp->d_ino] = holdstate|HASCINODE;
			else
		    		statemap[dirp->d_ino] = holdstate;
#else /* NO ACLS */
			statemap[dirp->d_ino] = DIRCT ? DSTATE : FSTATE;
#endif /* ACLS */
			goto again;

		case FSTATE:
			lncntp[dirp->d_ino]--;
			break;

		case DSTATE:
			descend(idesc, dirp->d_ino);
#ifdef ACLS
			if ((statemap[dirp->d_ino] & STATE) != CLEAR) {
#else /* no ACLS */
			if (statemap[dirp->d_ino] != CLEAR) {
#endif /* ACLS */
				lncntp[dirp->d_ino]--;
			} else {
				dirp->d_ino = 0;
				ret |= ALTERED;
			}
			break;
		}
	}
	pathp = curpathloc;
	*pathp = '\0';
	if (n == 0)
		return (ret|KEEPON);
	dirp->d_ino = 0;
	return (ret|KEEPON|ALTERED);
}

/*
 *	pass3 - Check Connectivity
 *	For each inumber:
 *	    If it has an associated continuation inode, make sure
 *		that it is a continuation inode and has not already
 *		been referenced.
 *	    If it has a state of DSTATE, then it is a directory which
 *		was not referenced in pass2.  See if we can link it up.
 */
pass3()
{
	register DINODE *dp;
	struct inodesc idesc;
	ino_t inumber, orphan;
	int loopcnt;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = DATA;
	for (inumber = ROOTINO; inumber <= lastino; inumber++) {
#ifdef ACLS
		if (statemap[inumber] & HASCINODE) {
		    if ((dp = ginode(inumber)) == NULL)
			break;
		    /*
		     * Make sure di_contin is not out of range and then
		     * check and make sure that the inode #di_contin
		     * is a continuation inode that has not already been
		     * referenced.
		     */
#ifdef OLD_ACLS
		    /*
		     * If we are removing all continuation inodes, just
		     * remove each link to them and don't set the state of
		     * the cinodes to CRSTATE. This causes cinodes to end
		     * up in state=CSTATE (no refs) so they get removed.
		     */
		    if (nukeacls) {
		       dp->di_contin = 0;
		       inodirty();
		    }
		    else
#endif /* OLD_ACLS */
		    if ((dp->di_contin < ROOTINO || dp->di_contin > imax) ||
		    	((statemap[dp->di_contin] & STATE) != CSTATE))
		    {
			/*  this is an error which must be cleared by hand. */
			pfatal("BAD CONTINUATION INODE NUMBER ");
			printf(" I=%u ", inumber);
			if (reply("CLEAR") == 1) {
				dp->di_contin = 0;
				inodirty();
				inosumbad++;
			}
		    }
		    else
		    {
			statemap[dp->di_contin] = CRSTATE;
		    }
		}
		if ((statemap[inumber] & STATE) == DSTATE) {
#else /* no ACLS */
		if (statemap[inumber] == DSTATE) {
#endif /* ACLS */
			pathp = pathname;
			*pathp++ = '?';
			*pathp = '\0';
			idesc.id_func = findino;
			srchname = "..";
			idesc.id_parent = inumber;
			loopcnt = 0;
			do {
				orphan = idesc.id_parent;
				if ((dp = ginode(orphan)) == NULL)
					break;
				idesc.id_parent = 0;
				idesc.id_filesize = dp->di_size;
				idesc.id_number = orphan;
				(void)ckinode(dp, &idesc);
				if (idesc.id_parent == 0)
					break;
				if (loopcnt >= sblock.fs_cstotal.cs_ndir)
					break;
				loopcnt++;
#ifdef ACLS
			} while ((statemap[idesc.id_parent] & STATE) == DSTATE);
#else /* no ACLS */
			} while (statemap[idesc.id_parent] == DSTATE);
#endif /* ACLS */
			if (linkup(orphan, idesc.id_parent) == 1) {
				idesc.id_func = pass2check;
				idesc.id_number = lfdir;
				descend(&idesc, orphan);
			}
		}
	}
}

/*
 *	pass4 - Check Reference Counts
 *	Walk through the inumbers which are allocated and statemap again:
 *	    FSTATE - file(of sorts)- check the link count and correct if needed.
 *	    DSTATE - unreferenced directory (Should have been changed to
 *		FSTATE in pass2 or pass3) - Clear.
 *	    CLEAR - marked earlier to be cleared - Clear.
 *	    CSTATE - unreferenced continuation inode (Should have been changed
 *		to CRSTATE in pass3) - Clear.
 *	    CSTATE - referenced continuation inode - check the link count -
 *		should be 1.
 *	Check the free inode count in the super block.
 */
pass4()
{
	register ino_t inumber, *blp;
	int n;
	struct inodesc idesc;
#ifdef ACLS
	register DINODE *dp;
#endif /* ACLS */

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	for (inumber = ROOTINO; inumber <= lastino; inumber++) {
		idesc.id_number = inumber;
#ifdef ACLS
		switch (statemap[inumber] & STATE) {
#else /* no ACLS */
		switch (statemap[inumber]) {
#endif /* ACLS */

		/*
		 * file (or referenced directory)
		 */
		case FSTATE:
			n = lncntp[inumber];
			if (n)
				adjust(&idesc, (short)n);
			else {
				for (blp = badlncnt;blp < badlnp; blp++)
					if (*blp == inumber) {
						clri(&idesc, "UNREF", 1);
						break;
					}
			}
			break;

		/*
		 * UNreferenced directory
		 */
		case DSTATE:
			clri(&idesc, "UNREF", 1);
			break;

		/*
		 * Marked earlier to be cleared
		 */
		case CLEAR:
			clri(&idesc, "BAD/DUP", 1);
			break;

#ifdef ACLS
		/*
		 * UNreferenced continuation inode
		 */
		case CSTATE:
			clri(&idesc, "UNREF", 2);
			break;

		/*
		 * referenced continuation inode
		 */
		case CRSTATE:
			if ((dp = ginode(inumber)) == NULL)
			    break;
			if (dp->di_nlink != 1)
			    if (!qflag) {
			    	pwarn("BAD LINK COUNT IN CONTINUATION INODE ");
				pwarn("I=%u (%ld should be %ld)",
					    inumber, dp->di_nlink, 1);
				if (preen)
					printf(" (CORRECTED)\n");
				else
				{
					if (reply("CORRECT") == 0)
					continue;
				}
			    	dp->di_nlink = 1;
				inodirty();
			    }
			break;
#endif /* ACLS */
		}
	}
#ifdef ACLS
	if (imax - ROOTINO - n_files - n_cont != sblock.fs_cstotal.cs_nifree) {
#else /* no ACLS */
	if (imax - ROOTINO - n_files != sblock.fs_cstotal.cs_nifree) {
#endif /* ACLS */
		pwarn("FREE INODE COUNT WRONG IN SUPERBLK");
		if (preen || qflag)
			printf(" (FIXED)\n");
		if (preen || qflag|| reply("FIX") == 1) {
#ifdef ACLS
			sblock.fs_cstotal.cs_nifree = imax - ROOTINO - n_files -n_cont;
#else /* no ACLS */
			sblock.fs_cstotal.cs_nifree = imax - ROOTINO - n_files;
#endif /* ACLS */
			sbdirty();
		}
	}
	flush(&dfile, &fileblk);
}

/*
 *	pass4check - This is called by ckinode which is called by clri.
 *	Clears the bmap or duplist for blocks used by inode which is being
 *	cleared.  If the block is on the dup list, don't clear it from the
 *	bmap since it is referenced by another inode.
 */
pass4check(idesc)
	register struct inodesc *idesc;
{
	register daddr_t *dlp;
	int nfrags, res = KEEPON;
	daddr_t blkno = idesc->id_blkno;
	int found; /* DSDe406500, found a blkno in duplist */

	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (outrange(blkno, 1))
			res = SKIP;
		else if (getbmap(blkno)) {
			found = 0;
			for (dlp = duplist; dlp < enddup; dlp++)
				if (*dlp == blkno) {
					found = 1;
					*dlp = *--enddup;
					res = KEEPON;
				}
			if (!found) { 
				clrbmap(blkno);
				n_blks--;
            		}
		}
	}
	return (res);
}

/*
 *	pass5 - Check Cylinder Groups
 */
pass5()
{
	register int c, n, i, b, d;
	short bo[MAXCPG][NRPOS];
	long botot[MAXCPG];
	long frsum[MAXFRAG];
	int blk;
	daddr_t cbase;
	int blockbits = (1<<sblock.fs_frag)-1;

	memcpy(freemap, blockmap, (unsigned)bmapsz);
	dupblk = 0;
#ifdef CACHE
	badblk = 0;	/*  bugfix - not related to caching...  */
#endif		
	n_index = sblock.fs_ncg * (cgdmin(&sblock, 0) - cgtod(&sblock, 0));
	for (c = 0; c < sblock.fs_ncg; c++) {
		cbase = cgbase(&sblock, c);
		memset((char *)botot, 0, sizeof (botot));
		memset((char *)bo, 0, sizeof (bo));
		memset((char *)frsum, 0, sizeof (frsum));
		/*
		 * need to account for the super blocks
		 * which appear (inaccurately) bad
		 */
		n_bad += cgtod(&sblock, c) - cgsblock(&sblock, c);
		if (getblk(&cgblk, cgtod(&sblock, c), sblock.fs_cgsize) == 0)
			continue;
		if (cgrp.cg_magic != CG_MAGIC) {
			pfatal("CG %d: BAD MAGIC NUMBER\n", c);
			memset((char *)&cgrp, 0, (int)sblock.fs_cgsize);
		}
		for (b = 0; b < sblock.fs_fpg; b += sblock.fs_frag) {
			blk = blkmap(&sblock, cgrp.cg_free, b);
			if (blk == 0)
				continue;
			if (blk == blockbits) {
				if (pass5check(cbase+b, sblock.fs_frag) == STOP)
					goto out5;
				/* this is clumsy ... */
				n_ffree -= sblock.fs_frag;
				n_bfree++;
				botot[cbtocylno(&sblock, b)]++;
				bo[cbtocylno(&sblock, b)]
				    [cbtorpos(&sblock, b)]++;
				continue;
			}
	 		for (d = 0; d < sblock.fs_frag; d++)
				if ((blk & (1<<d)) &&
				    pass5check(cbase + b + d, (long)1) == STOP)
					goto out5;
			fragacct(&sblock, blk, frsum, 1);
		}
		if (memcmp((char *)cgrp.cg_frsum, (char *)frsum, sizeof(frsum))) {
			if (debug)
			for (i = 0; i < sblock.fs_frag; i++)
				if (cgrp.cg_frsum[i] != frsum[i])
				printf("cg[%d].cg_frsum[%d] have %d calc %d\n",
				    c, i, cgrp.cg_frsum[i], frsum[i]);
			frsumbad++;
		}
		if (memcmp((char *)cgrp.cg_btot, (char *)botot, sizeof (botot))) {
			if (debug)
			for (n = 0; n < sblock.fs_cpg; n++)
				if (botot[n] != cgrp.cg_btot[n])
				printf("cg[%d].cg_btot[%d] have %d calc %d\n",
				    c, n, cgrp.cg_btot[n], botot[n]);
			offsumbad++;
		}
		if (memcmp((char *)cgrp.cg_b, (char *)bo, sizeof (bo))) {
			if (debug)
			for (i = 0; i < NRPOS; i++)
				if (bo[n][i] != cgrp.cg_b[n][i])
				printf("cg[%d].cg_b[%d][%d] have %d calc %d\n",
				    c, n, i, cgrp.cg_b[n][i], bo[n][i]);
			offsumbad++;
		}
	}
out5:
	if (dupblk)
		pwarn("%d DUP BLKS IN BIT MAPS\n", dupblk);
	if (fixcg == 0) {
		if ((b = n_blks+n_ffree+sblock.fs_frag*n_bfree+n_index+n_bad) != fmax) {
			pwarn("%ld BLK(S) MISSING\n", fmax - b);
			fixcg = 1;
		} else if (inosumbad + offsumbad + frsumbad + sbsumbad) {
			pwarn("SUMMARY INFORMATION %s%s%s%sBAD\n",
			    inosumbad ? "(INODE FREE) " : "",
			    offsumbad ? "(BLOCK OFFSETS) " : "",
			    frsumbad ? "(FRAG SUMMARIES) " : "",
			    sbsumbad ? "(SUPER BLOCK SUMMARIES) " : "");
			fixcg = 1;
		} else if (n_ffree != sblock.fs_cstotal.cs_nffree ||
		    n_bfree != sblock.fs_cstotal.cs_nbfree) {
			pwarn("FREE BLK COUNT(S) WRONG IN SUPERBLK");
			if (preen || qflag)
				printf(" (FIXED)\n");
			if (preen || qflag|| reply("FIX") == 1) {
				sblock.fs_cstotal.cs_nffree = n_ffree;
				sblock.fs_cstotal.cs_nbfree = n_bfree;
				sbdirty();
			}
		}
	}
	if (fixcg) {
		pwarn("BAD CYLINDER GROUPS");
		if (preen)
			printf(" (FIXED)\n");
		else if (!qflag)
		{
			if (reply("FIX") == 0)
				fixcg = 0;
		}
	}
}

/*
 *	pass5check -
 */
pass5check(blk, size)
	daddr_t blk;
	long size;
{

	if (outrange(blk, (int)size)) {
		fixcg = 1;
		if (preen)
			pfatal("BAD BLOCKS IN BIT MAPS.");
#ifdef CACHE
		if (++badblk >= CG_MAXBAD) {
			printf("TOO MANY USED BLOCKS ARE MARKED FREE IN FREE-BLOCK LIST (BITMAP).");
#else			
		if (++badblk >= MAXBAD) {
			printf("EXCESSIVE BAD BLKS IN BIT MAPS.");
#endif
			if (reply("CONTINUE") == 0)
				errexit("");
			return (STOP);
		}
	}
	for (; size > 0; blk++, size--)
		if (getfmap(blk)) {
			fixcg = 1;
			++dupblk;
		} else {
			n_ffree++;
			setfmap(blk);
		}
	return (KEEPON);
}

/*
 *	ckinode - the beastiest routine in this program :-)
 *	Figures out the number of fragments associated with each block
 *	in db array and sends that info to the function it calls.  Also
 *	goes throught the indirect address array and calls iblock which
 *	gets the direct addresses of each block and the number of fragments
 *	of the block and calls a function.
 *
 *	This calls a function as follows:
 *		*idesc->id_type = ADDR
 *			calls what is passed as id_func (usually pass#check)
 *		*idesc->id_type = DATA
 *			calls dirscan which reads each directory entry and
 *			calls the function in id_func (usually pass#check)
 */
ckinode(dp, idesc)
	DINODE *dp;
	register struct inodesc *idesc;
{
	register daddr_t *ap;
	int ret, n, ndb, offset;
	DINODE dino;

	if (SPECIAL || FASTLNK)
		return (KEEPON);
	dino = *dp;
	idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	/*
	 * number of blocks calculation does not use the 'howmany' macro
	 * because this macro can overflow for very large file sizes
	 */
	ndb = (dino.di_size/sblock.fs_bsize) +
		((dino.di_size%sblock.fs_bsize) ? 1 : 0);
	/*
	 * Walk through each of the direct blocks and figure the number
	 * of fragments (may be = whole block).
	 */
	for (ap = &dino.di_db[0]; ap < &dino.di_db[NDADDR]; ap++) {
		if (--ndb == 0 && (offset = blkoff(&sblock, dino.di_size)) != 0)
			idesc->id_numfrags =
				numfrags(&sblock, fragroundup(&sblock, offset));
		else
			idesc->id_numfrags = sblock.fs_frag;
		if (*ap == 0)
			continue;
		idesc->id_blkno = *ap;
		if (idesc->id_type == ADDR)
			ret = (*idesc->id_func)(idesc);
		else
			ret = dirscan(idesc);
		if (ret & STOP)
			return (ret);
	}
	idesc->id_numfrags = sblock.fs_frag;
	/*
	 * Walk through the indirect block addresses and call iblock to
	 * get the direct address and call the function we need.
	 */
	if ((dino.di_mode & IFMT) != IFIFO) {
		for (ap = &dino.di_ib[0], n = 1; n <= 2; ap++, n++) {
			if (*ap) {
				idesc->id_blkno = *ap;
				ret = iblock(idesc, n,
					dino.di_size - sblock.fs_bsize * NDADDR);
				if (ret & STOP)
					return (ret);
			}
		}
	}
	return (KEEPON);
}

/*
 *	iblock - traverses the indirect address array to get the
 *	blocks and call the function in id_func.
 */
iblock(idesc, ilevel, isize)
	struct inodesc *idesc;
	register ilevel;
	long isize;
{
	register daddr_t *ap;
	register daddr_t *aplim;
	int i, n, (*func)(), nif;
	BUFAREA ib;

	if (idesc->id_type == ADDR) {
		func = idesc->id_func;
		if (((n = (*func)(idesc)) & KEEPON) == 0)
			return (n);
	} else
		func = dirscan;
	if (outrange(idesc->id_blkno, idesc->id_numfrags)) /* protect thyself */
		return (SKIP);
	initbarea(&ib);
	if (getblk(&ib, idesc->id_blkno, sblock.fs_bsize) == NULL)
		return (SKIP);
	ilevel--;
	if (ilevel == 0) {
		nif = lblkno(&sblock, isize) + 1;
	} else /* ilevel == 1 */ {
		nif = isize / (sblock.fs_bsize * NINDIR(&sblock)) + 1;
	}
	if (nif > NINDIR(&sblock))
		nif = NINDIR(&sblock);
	aplim = &ib.b_un.b_indir[nif];
	for (ap = ib.b_un.b_indir, i = 1; ap < aplim; ap++, i++)
		if (*ap) {
			idesc->id_blkno = *ap;
			if (ilevel > 0)
				n = iblock(idesc, ilevel,
				    isize - i*NINDIR(&sblock)*sblock.fs_bsize);
			else
				n = (*func)(idesc);
			if (n & STOP)
				return (n);
		}
	return (KEEPON);
}

/*
 *	outrange - returns
 *		1 if block is OUT of range
 *		0 if block is IN range
 */
outrange(blk, cnt)
	daddr_t blk;
	int cnt;
{
	register int c;

	if ((unsigned)(blk+cnt) > fmax)
		return (1);
	c = dtog(&sblock, blk);
	if (blk < cgdmin(&sblock, c)) {
		/* 
		 * DSDe406500
		 * Cylinder group 0 is a special case, due to the boot block
		 * and primary super block. So, no data block # should be less
		 * than the begining of data blocks.
		 */
		if (c==0) {
			if (debug) {
				printf("blk %d < cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				printf(" blk+cnt %d > cgsbase %d\n",
				    blk+cnt, cgsblock(&sblock, c));
			}
			return (1);
		}
		else 
		if ((blk+cnt) > cgsblock(&sblock, c)) {
			if (debug) {
				printf("blk %d < cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				printf(" blk+cnt %d > cgsbase %d\n",
				    blk+cnt, cgsblock(&sblock, c));
			}
			return (1);
		}
	} else {
		if ((blk+cnt) > cgbase(&sblock, c+1)) {
			if (debug)  {
				printf("blk %d >= cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				printf(" blk+cnt %d > sblock.fs_fpg %d\n",
				    blk+cnt, sblock.fs_fpg);
			}
			return (1);
		}
	}
	return (0);
}

/*
 *	blkerr - a block error occurred.  Set statemap to CLEAR and
 *	print an error.
 */
blkerr(ino, s, blk)
	ino_t ino;
	char *s;
	daddr_t blk;
{

	pfatal("%ld %s I=%u", blk, s, ino);
	printf("\n");
#ifdef ACLS
	/*
	 *  Keep the continuation inode info
	 */
	if (statemap[ino] & HASCINODE)
    		statemap[ino] = CLEAR|HASCINODE;
	else
    		statemap[ino] = CLEAR;
#else /* no ACLS */
	statemap[ino] = CLEAR;
#endif /* ACLS */
}

/*
 *	descend - traverse the directory structure finding the children
 *	and ensuring that things are correct.
 *	This is called by pass2 and recursively by pass2check.
 */
descend(parentino, inumber)
	struct inodesc *parentino;
	ino_t inumber;
{
	register DINODE *dp;
	struct inodesc curino;

	memset((char *)&curino, 0, sizeof(struct inodesc));
#ifdef ACLS
	/*
	 * keep any continuation inode information
	 */
	if (statemap[inumber] & HASCINODE)
	    statemap[inumber] = HASCINODE|FSTATE;
	else
	    statemap[inumber] = FSTATE;
#else /* no ACLS */
	statemap[inumber] = FSTATE;
#endif /* ACLS */

	if ((dp = ginode(inumber)) == NULL)
		return(0);
	if (dp->di_size == 0) {
		direrr(inumber, "ZERO LENGTH DIRECTORY");
		if (reply("REMOVE") == 1)
#ifdef ACLS
			/*
	 		 * keep any continuation inode information
	 		 */
			if (statemap[inumber] & HASCINODE)
	    		    statemap[inumber] = HASCINODE|CLEAR;
			else
	    		    statemap[inumber] = CLEAR;
#else /* no ACLS */
			statemap[inumber] = CLEAR;
#endif /* ACLS */
		return(0);
	}
#ifdef LONGFILENAMES
	if (dp->di_size < (longfilenames?MINDIRSIZE_LFN:MINDIRSIZE))
#else
	if (dp->di_size < MINDIRSIZE)
#endif
	{
		direrr(inumber, "DIRECTORY TOO SHORT");
#ifdef LONGFILENAMES
		dp->di_size = longfilenames?MINDIRSIZE_LFN:MINDIRSIZE;
#else
		dp->di_size = MINDIRSIZE;
#endif
		if (reply("FIX") == 1)
			inodirty();
	}
	curino.id_type = DATA;
	curino.id_func = parentino->id_func;
	curino.id_parent = parentino->id_number;
	curino.id_number = inumber;
	curino.id_filesize = dp->di_size;
	(void)ckinode(dp, &curino);
}

/*
 *	dirscan - This will read the directory structures from the
 *	blocks and do some sanity checks and call the function passed
 *	in id_func.  This calls fsck_readdir which calls dircheck
 *	to read the directory structures and do some checks.
 */
dirscan(idesc)
	register struct inodesc *idesc;
{
	register DIRECT *dp;
	int dsize, n;
	long blksiz;
	char dbuf[DIRBLKSIZ];

	if (idesc->id_type != DATA)
		perrexit("wrong type to dirscan %d\n", idesc->id_type);
	blksiz = idesc->id_numfrags * sblock.fs_fsize;
	if (outrange(idesc->id_blkno, idesc->id_numfrags)) {
		idesc->id_filesize -= blksiz;
		return (SKIP);
	}
	idesc->id_loc = 0;
	for (dp = fsck_readdir(idesc); dp != NULL; dp = fsck_readdir(idesc)) {
#ifdef LONGFILENAMES
		/*
		 * If we suspect that this long file name system was
		 * once a Bell-compatible HFS file system with 14-character
		 * file names, then we must be sure to compact the . and
		 * .. entries so that another entry can't sneak in between
		 * the two.
		 *
		 * If we detect a . entry with d_reclen the size of
		 * Bell-compatible HFS directory structure, shrink it
		 * and copy what we think is the  .. entry right next
		 * to the . entry.
		 */
		if (idesc->id_entryno == 0 && longfilenames &&
		    dp->d_reclen == DIRSTRCTSIZ && dp->d_namlen == 1 &&
		    dp->d_name[0] == '.') {

		      DIRECT *tmpdp;

			/*
			 * If -q not specified, make sure that this
			 * compaction is really desired.
			 */
			if (!qflag) {
				pwarn("UNUSED SPACE BETWEEN . AND ..");
				pinode(idesc->id_number);
				if (preen)
					printf(" (FIXED)\n");
				if (!preen && reply("FIX") == 0)
					goto nocompact;
			}
			tmpdp = (DIRECT *) ((char *)dp + dp->d_reclen);
			dsize = DIRSIZ(dp);
			/* This could potentially be an overlapping copy. */
			memcpy(((char *)dp + dsize), tmpdp, DIRSIZ(tmpdp));
			tmpdp = (DIRECT *) ((char *)dp + dsize);
			tmpdp->d_reclen += (DIRSTRCTSIZ - dsize);
			dp->d_reclen = dsize;
			dirty(&fileblk);
		}
nocompact:
#endif /* LONGFILENAMES */
		dsize = dp->d_reclen;
		memcpy(dbuf, (char *)dp, dsize);
		idesc->id_dirp = (DIRECT *)dbuf;
		if ((n = (*idesc->id_func)(idesc)) & ALTERED) {
			if (getblk(&fileblk, idesc->id_blkno, blksiz) != NULL) {
				memcpy((char *)dp, dbuf, dsize);
				dirty(&fileblk);
				sbdirty();
			} else
				n &= ~ALTERED;
		}
		if (n & STOP)
			return (n);
	}
	return (idesc->id_filesize > 0 ? KEEPON : STOP);
}

/*
 * 	fsck_readdir - get next struct direct and check namelen and name.
 *	this calls dircheck.
 */
DIRECT *
fsck_readdir(idesc)
	register struct inodesc *idesc;
{
	register DIRECT *dp, *ndp;
	long size, blksiz;

	blksiz = idesc->id_numfrags * sblock.fs_fsize;
	if (getblk(&fileblk, idesc->id_blkno, blksiz) == NULL) {
		idesc->id_filesize -= blksiz - idesc->id_loc;
		return NULL;
	}
	if (idesc->id_loc % DIRBLKSIZ == 0 && idesc->id_filesize > 0 &&
	    idesc->id_loc < blksiz) {
		dp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
		if (dircheck(idesc, dp))
			goto dpok;
		idesc->id_loc += DIRBLKSIZ;
		idesc->id_filesize -= DIRBLKSIZ;
		dp->d_reclen = DIRBLKSIZ;
		dp->d_ino = 0;
		dp->d_namlen = 0;
		dp->d_name[0] = '\0';
		if (dofix(idesc))
			dirty(&fileblk);
		return (dp);
	}
dpok:
	if (idesc->id_filesize <= 0 || idesc->id_loc >= blksiz)
		return NULL;
	dp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
	idesc->id_loc += dp->d_reclen;
	idesc->id_filesize -= dp->d_reclen;
	ndp = (DIRECT *)(dirblk.b_buf + idesc->id_loc);
	if ((idesc->id_filesize <= 0 && idesc->id_loc % DIRBLKSIZ != 0) ||
	    (idesc->id_loc < blksiz && idesc->id_filesize > 0 &&
	     dircheck(idesc, ndp) == 0)) {
		printf("filesize = %d  loc = %d  blksiz = %d \n",
		 idesc->id_filesize, idesc->id_loc, blksiz);
		size = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
		dp->d_reclen += size;
		idesc->id_loc += size;
		idesc->id_filesize -= size;
		printf("size = %d  reclen = %d  loc = %d  filesize = %d \n",
		 size, dp->d_reclen, idesc->id_loc, idesc->id_filesize);
		if (dofix(idesc))
			dirty(&fileblk);
	}
	return (dp);
}

/*
 * 	dircheck - Verify that a directory entry is valid.
 * 	This is a superset of the checks made in the kernel.
 */
dircheck(idesc, dp)
	struct inodesc *idesc;
	register DIRECT *dp;
{
	register int size;
	register char *cp;
	int spaceleft;

#ifdef LONGFILENAMES
	size = longfilenames?DIRSIZ(dp):DIRSTRCTSIZ;
#else
	size = sizeof (struct direct);
#endif

	spaceleft = DIRBLKSIZ - (idesc->id_loc % DIRBLKSIZ);
	if (dp->d_ino < imax &&
	    dp->d_reclen != 0 &&
	    dp->d_reclen <= spaceleft &&
	    (dp->d_reclen & 0x3) == 0 &&
	    dp->d_reclen >= size &&
	    idesc->id_filesize >= size &&
#ifdef LONGFILENAMES
	    dp->d_namlen <= (longfilenames?MAXNAMLEN:SHORT_DIRSIZ))
#else
	    dp->d_namlen <= DIRSIZ)
#endif
	{

		if (dp->d_ino == 0)
			return (1);
		for (cp = dp->d_name, size = 0; size < dp->d_namlen; size++)
			if (*cp++ == 0)
				return (0);
		if (*cp == 0)
			return (1);
	}
	return (0);
}

/*
 *	direrr - Directory Error
 */
direrr(ino, s)
	ino_t ino;
	char *s;
{
	register DINODE *dp;

	pwarn("%s ", s);
	pinode(ino);
	printf("\n");
	if ((dp = ginode(ino)) != NULL && ftypeok(dp))
		pfatal("%s=%s\n", DIRCT?"DIR":"FILE", pathname);
	else
		pfatal("NAME=%s\n", pathname);
}

/*
 *	adjust - Try and adjust a bad link count.
 *
 *      Called by pass4.  Note that linkup is called in
 *      pass3 to link lost directories, and in pass4
 *      (from adjust) to link lost files.  In pass3,
 *      we ignore dp->di_nlink, since pass4 fixes that.
 *      But this routine, called only for non-directories,
 *      must leave di_nlink correct, since there are no
 *      more opportunites to fix it.
 */
adjust(idesc, lcnt)
	register struct inodesc *idesc;
	short lcnt;
{
	register DINODE *dp;
	int orphan;

	if ((dp = ginode(idesc->id_number)) == NULL)
		return(0);
	orphan = idesc->id_number;

	if (dp->di_nlink == lcnt) {
		if (linkup(idesc->id_number, (ino_t)0) == 0)
			clri(idesc, "UNREF", 0);
		else {
			dp = ginode(orphan);
			if (dp && !DIRCT) {
				dp->di_nlink = 1;
				inodirty();
			}
                }
	}
	else {
		pwarn("LINK COUNT %s",
			(lfdir==idesc->id_number)?lfname:(DIRCT?"DIR":"FILE"));
		pinode(idesc->id_number);
		printf(" COUNT %d SHOULD BE %d",
			dp->di_nlink, dp->di_nlink-lcnt);
		if (preen) {
			if (lcnt < 0) {
				printf("\n");
				preendie();
			}
			printf(" (ADJUSTED)\n");
		}
		if (preen || reply("ADJUST") == 1) {
			dp->di_nlink -= lcnt;
			inodirty();
		}
	}
}

/*
 *	clri - Clear a bad inode (ask first).
 */
clri(idesc, s, flg)
	register struct inodesc *idesc;
	char *s;
	int flg;
{
	register DINODE *dp;
#ifdef ACLS
	struct inodesc cidesc;
#endif /* ACLS */

	if ((dp = ginode(idesc->id_number)) == NULL)
		return(0);
	if (flg == 1) {
		if ((dp->di_mode & IFMT) != IFIFO)
			pwarn("%s %s", s, DIRCT?"DIR":"FILE");
		else
		{
			if (!qflag)
				pwarn("%s PIPE", s);
		}
		pinode(idesc->id_number);
#ifdef ACLS
	}
	else if (flg == 2) {
		pwarn("%s %s", s, "CONTINUATION INODE ");
		printf(" I=%u ", idesc->id_number);
#ifdef OLD_ACLS
		if (nukeacls) preen++;
#endif /* OLD_ACLS */
#endif /* ACLS */
	}
	if (preen || qflag|| reply("CLEAR") == 1) {
		if (preen)
			printf(" (CLEARED)\n");
#ifdef ACLS
#ifdef OLD_ACLS
		if (nukeacls && flg == 2) preen--;
#endif /* OLD_ACLS */
		if (CONT)
			n_cont--;
		else
			n_files--;

		/*
		 * If there is a CI associated with this inode, we must
		 * clear it as well.
		 */
		if (statemap[idesc->id_number] & HASCINODE) {
		    if (!(dp->di_contin < ROOTINO || dp->di_contin > imax))
			cidesc.id_number = dp->di_contin;
			clri(&cidesc, "UNREF", 2);
		}
#else /* no ACLS */
		n_files--;
#endif /* ACLS */
		(void)ckinode(dp, idesc);
		zapino(dp);
		statemap[idesc->id_number] = USTATE;
		inodirty();
		inosumbad++;
	}
}

/*
 *	badsb - Bad Super Block
 */
badsb(s)
	char *s;
{
#ifdef FIND_SB
	printf("\nBAD SUPERBLOCK ON DEVICE %s: %s\n", devname, s);
	pwarn("SEARCHING FOR ALTERNATE SUPERBLOCK LOCATIONS...\n");

	if (findsb())
		pwarn("TRY  fsck -b <n>  WHERE <n> IS ONE OF THE NUMBERS ABOVE\n");
	else {
		pwarn("NO SUPERBLOCKS FOUND");
		if (bflag)
			printf(" AT OR AFTER %d...\n", bflag);
		else
			printf(".\n");
	}
	if (preen)
		preendie();	
#else
	if (preen)
		printf("%s: ", devname);
	printf("BAD SUPER BLOCK: %s\n", s);
	pwarn("USE -b OPTION TO FSCK TO SPECIFY LOCATION OF AN ALTERNATE\n");
	pfatal("SUPER-BLOCK TO SUPPLY NEEDED INFORMATION; SEE fsck(1M).\n");
#endif	
}


#ifdef FIND_SB


/*
 *	The following code tries to find an alternate superblock if the
 *	one we've tried didn't work out.  It brute-force searches for
 *	a good one past the one that was tried, and then uses what it 
 *	finds to list other possibilities
 */
findsb()
{
	int offset, magic;
	int i, j;
	BUFAREA block;	

#define bbb block.b_un.b_fs
#define frags_to_K(x) (x*bbb.fs_fsize/DEV_BSIZE)


	initbarea(&block);
	if (bflag)
		offset = bflag;
	else
		offset = SBLOCK;     	/*  skip past boot block  */

	while (offset % (MINBSIZE/DEV_BSIZE))
		offset++;		/*  align on MINBSIZE boundary  */
	if (debug)
		printf("starting search at block %d\n", offset);

	/*
	 *	find the first superblock and then use its values
	 *	along with the macros from fs.h to tell where the
	 *	extra superblocks will be
	 */
	if ((i = find_first_sb(offset, &block))) {
		if (debug)
			printf("found first good SB at %d (k)\n", i);
		pwarn("  ");

		j = i/frags_to_K(bbb.fs_fpg);
		for (i = 0; i < 10 && j < bbb.fs_ncg; i++, j++)
			printf("%d ", frags_to_K(cgsblock(&bbb, j))); 
		putchar('\n');
	} 
	return(i);
}




/*
 *	find first superblock - note that we look on MINBSIZE boundaries
 */
int find_first_sb(start, bp)
int start;
BUFAREA *bp;
{
	int i = start;
	BUFAREA sb;
#define sbp bp->b_un.b_fs	


	initbarea(&sb);
	sblock.fs_fsbtodb = 0;		/*  getblk uses this  */
	
	while (getblk(bp, i, SBSIZE) != NULL) {
		if (is_good(&sbp)) {
			getblk(&sb, cgsblock(&sbp, sbp.fs_ncg-1), SBSIZE);
			if (is_good(&sb.b_un.b_fs))
				return(i);
			getblk(&sb, cgsblock(&sbp, sbp.fs_ncg/2), SBSIZE);
			if (is_good(&sb.b_un.b_fs))
				return(i);
		}
		i += MINBSIZE/DEV_BSIZE;
	}
	return(0);
}


/*
 *	decide whether a superblock is good or not - no perfect way to
 *	do this, but a few sanity checks go a long way...
 */
is_good(sp)
struct fs *sp;
{
	if (((sp->fs_magic == FS_MAGIC) || (sp->fs_magic == FS_MAGIC_LFN) || (sp->fs_magic == FD_FSMAGIC)) &&
	    (sp->fs_bsize % MINBSIZE == 0) && (sp->fs_bsize > 0) &&
	    (sp->fs_bsize <= MAXBSIZE) && (sp->fs_fsize > 0) &&
	    (sp->fs_ipg <= MAXIPG) && (sp->fs_ipg > 0) &&
	    (sp->fs_size <= sp->fs_fpg*sp->fs_ncg) &&
	    (sp->fs_size > sp->fs_fpg*(sp->fs_ncg-1)))
		return(1);
	return(0);
}
#endif




/*
 *	ginode - Given inumber, return pointer to the inode.
 */
#if  defined(SecureWare) || defined(B1) || !defined(CACHE)
DINODE *
ginode(inumber)
	ino_t inumber;
{
#if defined(SecureWare) && defined(B1)
	DINODE *dp;
#endif
	daddr_t block_to_fetch; /* disk block number for specified inumber */

	if (inumber < ROOTINO || inumber > imax) {
		if (debug && inumber > imax)
			printf("inumber out of range (%d)\n", inumber);
		return (NULL);
	}

	/*
	 * BUG FIX
	 * 1) Deal sanely with cylinder group boundary crossings.
	 * 2) Don't reread first inode block ~45 times when startinum is 0
	 *              (2 is a performance tweak only.)
	 */

	block_to_fetch = itod(&sblock, inumber);
	if (block_to_fetch != cached_block) {
		if (getblk(&inoblk, block_to_fetch, sblock.fs_bsize) == NULL)
			return (NULL);
		cached_block = block_to_fetch;
	}

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
	    disk_inode_in_block (&sblock, inoblk.b_un.b_buf, &dp, inumber);
	    return dp;
	}
	else
	    return (&inoblk.b_un.b_dinode[inumber % INOPB(&sblock)]);
#else
	return (&inoblk.b_un.b_dinode[inumber % INOPB(&sblock)]);
#endif
}

#else

/*
 *	keep a simple hash table of directory inodes - at the moment, we
 *	just use (i-number mod table_size) as a hash function and do 
 *	linear probing for collisions.  Since the table size is 2x the
 *	number of directories the superblock says are in the fs, we
 *	should never fill up, but if the superblock was 100% correct, 
 *	we wouldn't be running, would we? :-)
 */

#define min(a, b)  ((a) < (b) ? (a) : (b))
#define max(a, b)  ((a) > (b) ? (a) : (b))


dc_insert(ip, inum)
DINODE *ip;
ino_t inum;
{
	int i;
	

	if (dc_taken >= di_tbl_size)		/*  we're full  */
		return(0);
	i = inum % di_tbl_size;
	while (di_tbl[i].key) {	 
		di_tbl[i].rc++;
		i = (i + 1) % di_tbl_size;
	}
	
	bcopy(ip, &di_tbl[i].di, sizeof(DINODE));
	di_tbl[i].key = inum;
	di_tbl[i].rc = 0;
	di_tbl[i].dc_dirty = 0;
	di_tbl[i].hits = 0;
	dc_taken++;
}




DINODE *dc_search(inum)
ino_t inum;
{
	int i, first_i;

	dc_srches++;	
	if ((di_tbl_size == 0) || (di_tbl == NULL))	
		return(NULL);
	i = first_i = inum % di_tbl_size;
		
	while (di_tbl[i].key && (di_tbl[i].key != inum)) {
		di_tbl[i].rc++;
		i = (i + 1) % di_tbl_size;
		if (i == first_i)	/*  table full, inode not here  */
			return(NULL);
	}
	
	if (di_tbl[i].key) {
		di_tbl[i].hits++;
		if (di_tbl[i].dc_dirty)
			dc_hits++;
		return(&di_tbl[i].di);
	} else
		return(NULL);
}


/*
 *	return pointer to the specified inode, assuming it's a legitimate
 *	number.  if the inode is in the DI cache, return it; if it's in
 *	the block of inodes we have, return it; if not, go get the block
 *	it's in.
 */
DINODE *
ginode(inumber)
	ino_t inumber;
{
	daddr_t block_to_fetch; /* disk block number for specified inumber */
	int size, first, first_cb, first_cg;
	int left, skipping;
	DINODE *ip;
		

	if (inumber < ROOTINO || inumber > imax) {
		if (debug && inumber > imax)
			printf("inumber out of range (%d)\n", inumber);
		return (NULL);
	}

	if (cache_useful && (ip = dc_search(inumber))) 
		return(ip);

	if ((inumber >= first_inode) && (inumber <= last_inode)) {
		ip = &inoblk.b_un.b_dinode[inumber - first_inode];
		if ((ip->di_mode & IFMT) == IFDIR)
			dc_insert(ip, inumber);
		return(ip);
	}

	/*
	 *	not in the cache, so we have to go to the disk - need
	 *	to figure out how much to read and where to start:
	 *
	 *	case 1: there are fewer than CACHE_INOPB ipg, so we just
	 *	        read the whole cylgrp's inodes at once
	 *
	 *	case 2: > CACHE_BSIZE worth of inodes per group, so we
	 *		must do it a piece at a time
	 */
	block_to_fetch = cgimin(&sblock, inumber/sblock.fs_ipg);
	if (sblock.fs_ipg <= CACHE_INOPB) 
		size = sblock.fs_ipg*sizeof(DINODE);	
	else {		
		/*
		 *	would like to read on a CACHE_BSIZE-worth-of-inodes 
		 *	boundary, but can't if that would take us back 
		 *	before the start of the inodes in this CG
		 */
		first_cb = inumber/CACHE_INOPB*CACHE_INOPB;
		first_cg = inumber/sblock.fs_ipg*sblock.fs_ipg;  
		first = max(first_cb, first_cg);   
		skipping = first - first_cg;
		left = sblock.fs_ipg - skipping;
		size = min(left*sizeof(DINODE), CACHE_BSIZE);
		block_to_fetch += skipping*sizeof(DINODE)/sblock.fs_fsize;	
	}

	if (getblk(&inoblk, block_to_fetch, size) == NULL) {
		first_inode = 1;
		last_inode = 0;
		return (NULL);
	}
	if (sblock.fs_ipg <= CACHE_INOPB) 
		first_inode = inumber/sblock.fs_ipg*sblock.fs_ipg;
	else
		first_inode = first; 
	last_inode = first_inode + size/sizeof(DINODE) - 1;

	ip = &inoblk.b_un.b_dinode[inumber - first_inode];
	if ((ip->di_mode & IFMT) == IFDIR)
		dc_insert(ip, inumber);
	return(ip);
}

#endif	/*  secureware, b1, or cache  */





/*
 *	ftypeok - Checks that the mode in the inode is okay.
 *		Returns - 0 - BAD
 *			  1 - GOOD
 */
ftypeok(dp)
	DINODE *dp;
{
	switch (dp->di_mode & IFMT) {

	case IFDIR:
	case IFREG:
	case IFBLK:
	case IFCHR:
	case IFLNK:
	case IFSOCK:
	case IFIFO:
#if defined(RFA) || defined(OLD_RFA)
	case IFNWK:
#endif
		return (1);

	default:
		if (debug)
			printf("bad file type 0%o\n", dp->di_mode);
		return (0);
	}
}

/*
 *	freply - prints the string, s, gets a reply fromterminal and
 *	returns:
 *		0 - no (don't continue)
 *		1 - yes (continue)
 */
freply(s)
	char *s;
{
	char line[80];

	if (!isatty(0))
		errexit("exiting\n");
	printf("\n%s? ", s);
	if (getline(stdin, line, sizeof(line)) == EOF)
		errexit("\n");
	if (line[0] == 'y' || line[0] == 'Y')
		return (1);
	return (0);
}

is_mounted(device,dev_st)
	char *device;
	struct stat *dev_st;
{
	char blockdev[BUFSIZ];
	struct ustat ust;
	char *dp;

	strcpy(blockdev,device);
	if (stat(blockdev,dev_st)<0)
		return (0);
	
	if (is_pre_init(dev_st->st_rdev))
		return (0);
	
	if ((dev_st->st_mode & S_IFMT) == S_IFCHR) {
		dp = strrchr(blockdev, '/');
		if (strncmp(dp, "/r", 2) != 0)
			while (dp >= blockdev && *--dp != '/');
		if (*(dp+1) == 'r')
			(void)strcpy(dp+1, dp+2);
		if (stat(blockdev,dev_st)<0)
			return(0);
	}

	if (ustat(dev_st->st_rdev, &ust) >= 0) {
		/*
		 * make sure we are not in pre_init_rc. If we are,
		 * 0 should be returned in here.
		 */
		if ((is_root(dev_st->st_rdev)) && is_roroot())
			return (0);
		return (1);
	}

	return (0);
}

/*
 * is_root(): test the target device is a root by checking the st_rdev with
 * the st_dev of "/"
 * if they are the same, this is a root
 */
is_root (rdev_num)
	dev_t rdev_num;
{
	struct stat stslash;

	if (stat("/",&stslash) < 0)
		return (0);

	if (stslash.st_dev != rdev_num)
		return (0);

	return (1);
}

/* 
 * is_swap(): scan through the device numbers of swapinfo
 * if they are the same as the given st_rdev, the given device is a swap device
 */
is_swap (devno)
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
	return (match);
}

/*
 *	reply - Prints the string, s, gets a reply from nflag,
 *	yflag, or terminal and returns:
 *		0 - no
 *		1 - yes
 */
reply(s)
	char *s;
{
	char line[80];

	if (preen)
		pfatal("INTERNAL ERROR: GOT TO reply()");
	rplyflag = 1;
	printf("\n%s? ", s);
	if (nflag || dfile.wfdes < 0) {
		printf(" no\n\n");
		return (0);
	}
	if (yflag) {
		printf(" yes\n\n");
		return (1);
	}
	if (getline(stdin, line, sizeof(line)) == EOF)
		errexit("\n");
	printf("\n");
	if (line[0] == 'y' || line[0] == 'Y')
		return (1);
	else {
		fixed = 0;
		return (0);
	}
}

/*
 *	getline - reads from fp, into loc, maxlen characters.
 */
getline(fp, loc, maxlen)
	FILE *fp;
	char *loc;
{
	register n;
	register char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF)
			return (EOF);
		if (!isspace(n) && p < lastloc)
			*p++ = n;
	}
	*p = 0;
	return (p - loc);
}

/*
 *	getblk
 */
BUFAREA *
getblk(bp, blk, size)
	register BUFAREA *bp;
	daddr_t blk;
	long size;
{
	register struct filecntl *fcp;
	daddr_t dblk;


	fcp = &dfile;
	dblk = fsbtodb(&sblock, blk);
	if (bp->b_bno == dblk)
		return (bp);
	flush(fcp, bp);
	if (bread(fcp, bp->b_un.b_buf, dblk, size) != 0) {
		bp->b_bno = dblk;
		bp->b_size = size;
		return (bp);
	}
	bp->b_bno = (daddr_t)-1;
	return (NULL);
}

/*
 *	flush - if b_dirty, write the block
 */
flush(fcp, bp)
	struct filecntl *fcp;
	register BUFAREA *bp;
{

	if (bp->b_dirty)
		(void)bwrite(fcp, bp->b_un.b_buf, bp->b_bno, (long)bp->b_size);
	bp->b_dirty = 0;
}

/*
 *	rwerr - Read/Write error.
 */
rwerr(s, blk)
	char *s;
	daddr_t blk;
{

	if (preen == 0)
		printf("\n");
	pfatal("CANNOT %s: BLK %ld", s, blk);
	if (reply("CONTINUE") == 0)
		errexit("Program terminated\n");
}

/*
 *	ckfini
 */
ckfini()
{
	flush(&dfile, &fileblk);
	flush(&dfile, &sblk);
	if (sblk.b_bno != SBLOCK) {
		sblk.b_bno = SBLOCK;
		sbdirty();
		flush(&dfile, &sblk);
	}
	flush(&dfile, &inoblk);
#ifdef CACHE
	di_flush_cache();
#endif
	
	(void)close(dfile.rfdes);
	(void)close(dfile.wfdes);
}



#ifdef CACHE

/*
 *	di_flush_cache - flush out any dirty directory inodes
 *
 *	if directory inodes were modified, we must flush them
 *	out to disk - we do this a CACHE_BSIZE chunk at a time
 *
 *	note that we're probably in raw mode, so we can't do
 *	individual inode (128 byte) writes
 */
di_flush_cache()
{
	short *map;	/*  1 row per CG  */
	int ncg;	/*  how many rows to look at  */
	int cg, col, k, first, last, addr;


	if (!is_good(&sblock))	 /*  don't do this if superblock was bad  */
		return(0);
	else if (debug)
		printf("flushing dir inode cache...\n");
		
	/*
	 *	"map" is an array of shorts - one per CG; each one is
	 *	further subdivided into bits that represent a CACHE_BSIZE
	 *	chunk worth of inodes in this CG, so this is really a
	 *	2D bit array 
	 */
	ncg = sblock.fs_ncg;
	if (map = (short *) malloc(ncg*sizeof(short))) {
		bzero(map, ncg*sizeof(short));
		for (k = 0; k < di_tbl_size; k++) 
			if (di_tbl[k].dc_dirty && di_tbl[k].key) {
				cg = di_tbl[k].key/sblock.fs_ipg;
				col = (di_tbl[k].key % sblock.fs_ipg)/CACHE_INOPB;
				map[cg] |= 1<<col;
			}
	}

	for (cg = 0; cg < ncg; cg++) {

		if (map && (map[cg] == 0))    /*  no dirty ones in this CG  */
			continue;

		for (col = 0; col < MAXIPG/CACHE_INOPB; col++) {

			if (map && (map[cg] & (1<<col)) == 0)	
				continue;
			if (debug) 
				printf("di_flush_cache: CG %d, chunk %d is dirty\n", cg, col);

			addr = cgimin(&sblock, cg) + col*CACHE_BSIZE/sblock.fs_fsize;
			first = cg*sblock.fs_ipg + col*CACHE_INOPB;
			last = CACHE_INOPB > sblock.fs_ipg ? 
				first + sblock.fs_ipg - 1 :
				first + CACHE_INOPB - 1;
			getblk(&inoblk, addr, (last-first+1)*sizeof(DINODE));

			/*
			 *	we now have the chunk worth of inodes; walk
			 *	through our cache, overlaying any dirty ones
			 *	into the chunk
			 *
			 *	for i = first to last if dc_search(i)...  XXX
			 */
			for (k = 0; k < di_tbl_size; k++)
				if (di_tbl[k].dc_dirty && 
				    (di_tbl[k].key >= first) &&
				    (di_tbl[k].key <= last))
					bcopy(&di_tbl[k].di, &inoblk.b_un.b_dinode[di_tbl[k].key - first], sizeof(DINODE));
			inoblk.b_dirty = 1;
			flush(&dfile, &inoblk);
		}
	}
}

#endif



/*
 *	pinode - Print inode
 */
pinode(ino)
	ino_t ino;
{
	register DINODE *dp;
	register char *p;
	char uidbuf[BUFSIZ];
	char *ctime();

	printf(" I=%u \n", ino);
	if ((dp = ginode(ino)) == NULL)
		return(0);
	printf(" OWNER=");
	if (getpw((int)dp->di_uid, uidbuf) == 0) {
		for (p = uidbuf; *p != ':'; p++);
		*p = 0;
		printf("%s ", uidbuf);
	}
	else {
		printf("%d ", dp->di_uid);
	}
	printf("MODE=%o\n", dp->di_mode);
	if (preen)
		printf("%s: ", devname);
	printf("SIZE=%ld ", dp->di_size);
	p = ctime(&dp->di_mtime);
	printf("MTIME=%12.12s %4.4s ", p+4, p+20);
}

/*
 *	makecg - Phase 6: Salvage Cylinder Groups
 */
makecg()
{
	int c, blk;
	daddr_t dbase, d, dlower, dupper, dmax;
	long i, j, s;
	ino_t inumber;
	register struct csum *cs;
	register DINODE *dp;

	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		dbase = cgbase(&sblock, c);
		dmax = dbase + sblock.fs_fpg;
		if (dmax > sblock.fs_size) {
			for ( ; dmax >= sblock.fs_size; dmax--)
				clrbit(cgrp.cg_free, dmax - dbase);
			dmax++;
		}
		dlower = cgsblock(&sblock, c) - dbase;
		dupper = cgdmin(&sblock, c) - dbase;
		cs = &sblock.fs_cs(&sblock, c);
		(void)time(&cgrp.cg_time);
		cgrp.cg_magic = CG_MAGIC;
		cgrp.cg_cgx = c;
		if (c == sblock.fs_ncg - 1)
			cgrp.cg_ncyl = sblock.fs_ncyl % sblock.fs_cpg;
		else
			cgrp.cg_ncyl = sblock.fs_cpg;
		cgrp.cg_niblk = sblock.fs_ipg;
		cgrp.cg_ndblk = dmax - dbase;
		cgrp.cg_cs.cs_ndir = 0;
		cgrp.cg_cs.cs_nffree = 0;
		cgrp.cg_cs.cs_nbfree = 0;
		cgrp.cg_cs.cs_nifree = 0;
		cgrp.cg_rotor = 0;
		cgrp.cg_frotor = 0;
		cgrp.cg_irotor = 0;
		for (i = 0; i < sblock.fs_frag; i++)
			cgrp.cg_frsum[i] = 0;
		inumber = sblock.fs_ipg * c;
		for (i = 0; i < sblock.fs_ipg; inumber++, i++) {
			cgrp.cg_cs.cs_nifree++;
			clrbit(cgrp.cg_iused, i);
			dp = ginode(inumber);
			if (dp == NULL)
				continue;
			if (ALLOC) {
				if (DIRCT)
					cgrp.cg_cs.cs_ndir++;
				cgrp.cg_cs.cs_nifree--;
				setbit(cgrp.cg_iused, i);
				continue;
			}
		}
		while (i < MAXIPG) {
			clrbit(cgrp.cg_iused, i);
			i++;
		}
		if (c == 0)
			for (i = 0; i < ROOTINO; i++) {
				setbit(cgrp.cg_iused, i);
				cgrp.cg_cs.cs_nifree--;
			}
		for (s = 0; s < MAXCPG; s++) {
			cgrp.cg_btot[s] = 0;
			for (i = 0; i < NRPOS; i++)
				cgrp.cg_b[s][i] = 0;
		}
		if (c == 0) {
			dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
		}
		for (d = dlower; d < dupper; d++)
			clrbit(cgrp.cg_free, d);
		for (d = 0; (d + sblock.fs_frag) <= dmax - dbase;
		    d += sblock.fs_frag) {
			j = 0;
			for (i = 0; i < sblock.fs_frag; i++) {
				if (!getbmap(dbase + d + i)) {
					setbit(cgrp.cg_free, d + i);
					j++;
				} else
					clrbit(cgrp.cg_free, d+i);
			}
			if (j == sblock.fs_frag) {
				cgrp.cg_cs.cs_nbfree++;
				cgrp.cg_btot[cbtocylno(&sblock, d)]++;
				cgrp.cg_b[cbtocylno(&sblock, d)]
				    [cbtorpos(&sblock, d)]++;
			} else if (j > 0) {
				cgrp.cg_cs.cs_nffree += j;
				blk = blkmap(&sblock, cgrp.cg_free, d);
				fragacct(&sblock, blk, cgrp.cg_frsum, 1);
			}
		}
		for (j = d; d < dmax - dbase; d++) {
			if (!getbmap(dbase + d)) {
				setbit(cgrp.cg_free, d);
				cgrp.cg_cs.cs_nffree++;
			} else
				clrbit(cgrp.cg_free, d);
		}
		for (; d % sblock.fs_frag != 0; d++)
			clrbit(cgrp.cg_free, d);
		if (j != d) {
			blk = blkmap(&sblock, cgrp.cg_free, j);
			fragacct(&sblock, blk, cgrp.cg_frsum, 1);
		}
		for (d /= sblock.fs_frag; d < MAXBPG(&sblock); d ++)
			clrblock(&sblock, cgrp.cg_free, d);
		sblock.fs_cstotal.cs_nffree += cgrp.cg_cs.cs_nffree;
		sblock.fs_cstotal.cs_nbfree += cgrp.cg_cs.cs_nbfree;
		sblock.fs_cstotal.cs_nifree += cgrp.cg_cs.cs_nifree;
		sblock.fs_cstotal.cs_ndir += cgrp.cg_cs.cs_ndir;
		*cs = cgrp.cg_cs;
		(void)bwrite(&dfile, (char *)&cgrp,
			fsbtodb(&sblock, cgtod(&sblock, c)), sblock.fs_cgsize);
	}
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		(void)bwrite(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize);
	}
	sblock.fs_ronly = 0;
	sblock.fs_fmod = 0;
	sbdirty();
}

/*
 *	findino - Try and find the parent of this inode.
 */
findino(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (dirp->d_ino == 0)
		return (KEEPON);
	if (!strcmp(dirp->d_name, srchname)) {
		if (dirp->d_ino >= ROOTINO && dirp->d_ino <= imax)
			idesc->id_parent = dirp->d_ino;
		return (STOP);
	}
	return (KEEPON);
}

/*
 *	mkentry - Make a directory entry
 */
mkentry(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;
	DIRECT newent;
	int newlen, oldlen;

	newent.d_namlen = 11;
#ifdef LONGFILENAMES
	newlen = longfilenames?DIRSIZ(&newent):DIRSTRCTSIZ;
#else
	newlen = sizeof (struct direct);
#endif

	if (dirp->d_ino != 0)
#ifdef LONGFILENAMES
		oldlen = longfilenames?DIRSIZ(dirp):DIRSTRCTSIZ;
#else
		oldlen = sizeof (struct direct);
#endif
	else
		oldlen = 0;
	if (dirp->d_reclen - oldlen < newlen)
		return (KEEPON);
	newent.d_reclen = dirp->d_reclen - oldlen;
	dirp->d_reclen = oldlen;
	dirp = (struct direct *)(((char *)dirp) + oldlen);
	dirp->d_ino = idesc->id_parent;	/* ino to be entered is in id_parent */
	dirp->d_reclen = newent.d_reclen;
	dirp->d_namlen = lftempname(dirp->d_name, idesc->id_parent);
	return (ALTERED|STOP);
}

/*
 *	chgdd - Change ".."
 */
chgdd(idesc)
	struct inodesc *idesc;
{
	register DIRECT *dirp = idesc->id_dirp;

	if (dirp->d_name[0] == '.' && dirp->d_name[1] == '.' &&
	dirp->d_name[2] == 0) {
		dirp->d_ino = lfdir;
		return (ALTERED|STOP);
	}
	return (KEEPON);
}

/*
 *	linkup - Link up an orphaned directory with its parent
 */
linkup(orphan, pdir)
	ino_t orphan;
	ino_t pdir;
{
	register DINODE *dp;
	int lostdir, len;
	struct inodesc idesc;

	memset((char *)&idesc, 0, sizeof(struct inodesc));
	if ((dp = ginode(orphan)) == NULL)
		return (0);
	lostdir = DIRCT;
	if ((dp->di_mode & IFMT) != IFIFO)
		pwarn("UNREF %s ", lostdir ? "DIR" : "FILE");
	else
		pwarn("UNREF PIPE");
	pinode(orphan);
	if (preen && dp->di_size == 0)
		return (0);
	if (DIRCT && (dp->di_nlink == 0))
		return 0;
	if (preen)
		printf(" (RECONNECTED)\n");
	else
		if (reply("RECONNECT") == 0)
			return (0);
#if defined(SecureWare) && defined(B1)
	if (ISB1)
		fsck_orphan_tag(dp);
#endif
	pathp = pathname;
	*pathp++ = '/';
	*pathp = '\0';
	if (lfdir == 0) {
		if ((dp = ginode(ROOTINO)) == NULL)
			return (0);
		srchname = lfname;
		idesc.id_type = DATA;
		idesc.id_func = findino;
		idesc.id_number = ROOTINO;
		idesc.id_filesize = dp->di_size;
		(void)ckinode(dp, &idesc);
		if ((lfdir = idesc.id_parent) == 0) {
			pfatal("SORRY. NO lost+found DIRECTORY");
			printf("\n\n");
			return (0);
		}
	}
	if ((dp = ginode(lfdir)) == NULL ||
#ifdef ACLS
	     !DIRCT || ((statemap[lfdir] & STATE) != FSTATE)) {
#else /* no ACLS */
	     !DIRCT || statemap[lfdir] != FSTATE) {
#endif /* ACLS */
		pfatal("SORRY. NO lost+found DIRECTORY");
		printf("\n\n");
		return (0);
	}
#if defined(SecureWare) && defined(B1)
	if(ISB1 && !fsck_reconnect(dp)) {
		pwarn("SORRY. I=%u DOES NOT DOMINATE lost+found\n\n", orphan);
		return(0);
	}
#endif
	if (fragoff(&sblock, dp->di_size)) {
		dp->di_size = fragroundup(&sblock, dp->di_size);
		inodirty();
	}
	len = strlen(lfname);
	memcpy(pathp, lfname, len + 1);
	pathp += len;
	idesc.id_type = DATA;
	idesc.id_func = mkentry;
	idesc.id_number = lfdir;
	idesc.id_filesize = dp->di_size;
	idesc.id_parent = orphan;	/* this is the inode to enter */
	idesc.id_fix = DONTKNOW;
	if ((ckinode(dp, &idesc) & ALTERED) == 0) {
		pfatal("SORRY. NO SPACE IN lost+found DIRECTORY");
		printf("\n\n");
		return (0);
	}
	lncntp[orphan]--;
	*pathp++ = '/';
	pathp += lftempname(pathp, orphan);
	if (lostdir) {
		dp = ginode(orphan);
#if defined(SecureWare) && defined(B1)
		if(ISB1 && fsck_clear_flags(dp))
			inodirty();
#endif
		idesc.id_type = DATA;
		idesc.id_func = chgdd;
		idesc.id_number = orphan;
		idesc.id_filesize = dp->di_size;
		idesc.id_fix = DONTKNOW;
		(void)ckinode(dp, &idesc);
		if ((dp = ginode(lfdir)) != NULL) {
			dp->di_nlink++;
			inodirty();
			lncntp[lfdir]++;
		}
		pwarn("DIR I=%u CONNECTED. ", orphan);
		printf("PARENT WAS I=%u\n", pdir);
		if (preen == 0)
			printf("\n");
	}
	return (1);
}

/*
 * 	lftempname - generate a temporary name for the lost+found directory.
 */
lftempname(bufp, ino)
	char *bufp;
	ino_t ino;
{
	register ino_t in;
	register char *cp;
	int namlen;

	cp = bufp + 2;
	for (in = imax; in > 0; in /= 10)
		cp++;
	*--cp = 0;
	namlen = cp - bufp;
	in = ino;
	while (cp > bufp) {
		*--cp = (in % 10) + '0';
		in /= 10;
	}
	*cp = '#';
	return (namlen);
}





/*
 *	bread - read a data block
 */
bread(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	long size;
{
	int i = 0;

#ifdef CACHE
	int j, k;
	struct cache *x;
	

	if (size <= CB_SIZE && bc[0])		/*  linear search in cache  */
	    for (j = 0; j < cache_size; j++)
		if (bc[j]->b_blkno == blk) {
			bcopy(bc[j]->b_buf, buf, size);
			x = bc[j];		/*  maintain LRU  */
			for (k = j; k > 0; k--)
				bc[k] = bc[k-1];
			bc[0] = x;
			cache_hits++;
			return(1);
		} 
	cache_misses++;
#endif		

	do
	{
		if (lseek(fcp->rfdes, (long)dbtob(blk), 0) == -1)
			rwerr("SEEK", blk);
		else if (read(fcp->rfdes, buf, (int)size) == size)
#ifndef CACHE	
			return (1);
#else			
		{		
		    if ((size <= CB_SIZE) && (cache_size > 0) && bc[0]) {
		    	/*
		    	 *   move everyone else down, and put in first slot
		    	 */
			j = cache_size - 1;
			x = bc[j];
			for (k = j; k > 0; k--)
				bc[k] = bc[k-1];
			bc[0] = x;
			bc[0]->b_blkno = blk;
			bcopy(buf, bc[0]->b_buf, size);
		    }
		    return (1);
		}
#endif				
		rwerr("READ", blk);
	} while (i++ < READRETRIES);
	error("FAILED READ OF BLOCK #%d, TRIED %d TIMES\n", blk, READRETRIES);
	return (0);
}

/*
 *	bwrite - write a data block
 */
bwrite(fcp, buf, blk, size)
	register struct filecntl *fcp;
	char *buf;
	daddr_t blk;
	long size;
{
#ifdef CACHE
	int j;
#endif
		
	if (fcp->wfdes < 0)
		return (0);
	if (lseek(fcp->wfdes, (long)dbtob(blk), 0) == -1)
		rwerr("SEEK", blk);
	else if (write(fcp->wfdes, buf, (int)size) == size) {
#ifdef CACHE
		/*
		 *	just maintain integrity here - don't mess with LRU
		 */
		if (size <= CB_SIZE && bc[0])
		    for (j = 0; j < cache_size; j++)
			if (bc[j]->b_blkno == blk) {
				bcopy(buf, bc[j]->b_buf, size);
				break;
			}
#endif
		fcp->mod = 1;
		return (1);
	}
	rwerr("WRITE", blk);
	return (0);
}

/*
 *	catch
 */
catch()
{

	ckfini();
	exit(12);
}

/*
 *	unrawname - If cp is a raw device file, try to figure out the
 *	corresponding block device name by removing the r if it is where
 *	we think it might be.
 */
char *
unrawname(cp)
	char *cp;
{
	char *dp = strrchr(cp, '/');
	struct stat stb;

	if (dp == 0)
		return (cp);
	if (stat(cp, &stb) < 0)
		return (cp);
	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);

	/* Check for LVM device naming:
	 * /dev/vg00/rlvol1 -> /dev/vg00/lvol1
	 *
	 * or for series 200/300 old naming
	 * convention (i.e. /dev/rhd).
	 */
	if (strncmp(dp, "/r", 2) != 0) 
		/*  Check for AT&T Sys V disk naming convention
		 *    (i.e. /dev/rdsk/c0d0s3).
		 */
		while (dp >= cp && *--dp != '/');

	/*  If it's neither, just return the name unchanged. */
	if (*(dp+1) == 'r')
		(void)strcpy(dp+1, dp+2);

	return (cp);
}

/*
 *	rawname - If cp is a block device file, try to figure out the
 *	corresponding character device name by adding an r where we think
 *	it might be.
 */
char *
rawname(cp)
	char *cp;
{
	static char rawbuf[MAXPATHLEN];
	char *dp = strrchr(cp, '/');

	if (dp == 0)
		return (0);
	while (dp >= cp && *--dp != '/');

	/*  Check for AT&T Sys V disk naming convention
	 *    (i.e. /dev/dsk/c0d0s3).
	 *  or for the auto changer convention
	 *    (i.e. /dev/ac/c0d1as2 or /dev/ac/1a)
	 *  If it doesn't fit either of these,
	 *    assume the LVM device naming convention
	 *    /dev/vg00/lvol1 -> /dev/vg00/rlvol1
	 *    or the old 200/300 naming
	 *    convention (i.e. /dev/hd).
	 */
	if ((strncmp(dp, "/dsk", 4) != 0) && (strncmp(dp, "/ac", 3) != 0)) {
		dp = strrchr(cp, '/');
	}

	*dp = 0;
	(void)strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);

	return (rawbuf);
}

/*
 * 	dofix - determine whether an inode should be fixed.
 */
dofix(idesc)
	register struct inodesc *idesc;
{

	switch (idesc->id_fix) {

	case DONTKNOW:
		direrr(idesc->id_number, "DIRECTORY CORRUPTED");
		if (reply("FIX") == 0) {
			idesc->id_fix = NOFIX;
			return (0);
		}
		idesc->id_fix = FIX;
		return (ALTERED);

	case FIX:
		return (ALTERED);

	case NOFIX:
		return (0);

	default:
		perrexit("UNKNOWN INODESC FIX MODE %d\n", idesc->id_fix);
	}
	/* NOTREACHED */
}

/*
 *	error - print an error message.
 */
/* VARARGS1 */
error(s1, s2, s3, s4)
	char *s1;
{

	printf(s1, s2, s3, s4);
}

/*
 *	errexit - print an error message and then exit.
 */
/* VARARGS1 */
errexit(s1, s2, s3, s4)
	char *s1;
{
	error(s1, s2, s3, s4);
	exit(8);
}

/*
 *	perrexit - An inconsistency occurred which fsck cannot fix.  If
 *	preening, prefix the message with the device number, otherwise
 *	just printf.  Exit with status=8.
 */
/* VARARGS1 */
perrexit(s1, s2, s3, s4)
	char *s1;
{
	if (preen)
		printf("%s: ", devname);
	error(s1, s2, s3, s4);
	exit(8);
}

/*
 * 	pfatal - An inconsistency occured which shouldn't during normal
 *	operations.  Die if preening, otherwise just printf.
 */
/* VARARGS1 */
pfatal(s, a1, a2, a3)
	char *s;
{

	if (preen) {
		printf("%s: ", devname);
		printf(s, a1, a2, a3);
		printf("\n");
		preendie();
	}
	printf(s, a1, a2, a3);
}

/*
 *	preendie - Print error message to run fsck manually and die.
 */
preendie()
{

	printf("%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n", devname);
	exit(8);
}

/*
 * 	pwarn - like printf when not preening, or a warning (preceded
 *	by filename) when preening.
 */
/* VARARGS1 */
pwarn(s, a1, a2, a3, a4, a5, a6)
	char *s;
{

	if (preen)
		printf("%s: ", devname);
	printf(s, a1, a2, a3, a4, a5, a6);
}

#ifndef lint
/*
 * 	panic - Stub for routines from kernel.
 */
panic(s)
	char *s;
{

	pfatal("INTERNAL INCONSISTENCY: %s\n", s);
	exit(12);
}
#endif

/*
 * 	indircheck - checks to make sure that all indirect block pointers
 *	beyond the expected end of the file to the end of the indirect
 *	pointer blocks have a value of zero.
 */
indircheck(dp, ndb, inumber)
register DINODE *dp;
int ndb, inumber;
{
	daddr_t addr;
	int x = 1;		/* size available in this level of indirection */
	int acctd;		/* block counter */
	int level;		/* indirection counter */
	int idchk();

	/*
	 * Determine how many levels of indirection.
	 * Count up blocks accounted for in lower levels
	 */
	for (acctd=NDADDR, level=0; level < NIADDR; level++) {
		x *= NINDIR(&sblock);
		if (ndb <= (acctd + x))
			break;
		acctd += x;
	}
	if (level == NIADDR) {
		pfatal("FILE SIZE TOO LARGE");
		return(1);
	}
	/*
	 * fetch the first indirect block
	 */
	addr = dp->di_ib[level];
	if (addr == 0)
	{ /* the indirect blocks expected have not been allocated */
		pfatal("ZERO INDIRECT BLOCK POINTER (inode.di_ib[%d])", level);
		return(1);
	}
	ndb -= acctd;

	if (idchk(ndb, level, addr, inumber))
		return(1);
	return(0);
}

/*
 *	idchk - Check indirect addresses
 */
idchk(nblk, level, addr, inumber)
int nblk, level, inumber;
daddr_t addr;
{
	daddr_t *bap;
	int index;
	BUFAREA ptrblk;
	int last;	/* last good ptr loc in this ptrblk */
	int tmp_nblk;
		

	if (level < 0) return(0);

        /* be sure we're not looking at garbage */
        if (outrange(addr, sblock.fs_frag)) {
                blkerr(inumber, "BAD", addr);
                return(1);
        }
	ptrblk.b_bno = -1;	/* initialize ptrblk for the getblk call */
	ptrblk.b_dirty = 0;
	if (getblk(&ptrblk, addr, sblock.fs_bsize) == NULL) {
		pfatal("CANNOT READ INDIRECT BLOCK");
		return(1);
	}
	bap = ptrblk.b_un.b_indir;
	switch (level) {
	case 0:
		last = (nblk % NINDIR(&sblock)-1);
		break;
	default:
		/*
		 *  if the block is full, be careful not to slip on to the
		 *  next one (which is what happened up through 68K 9.03)
		 */
		last = ((nblk-1) / NINDIR(&sblock));
		tmp_nblk = nblk;
		nblk %= NINDIR(&sblock);
		if (tmp_nblk && !nblk)
			nblk = NINDIR(&sblock);
	}

	if (last == -1)			/* block exactly full */
	         return(0);
	if (idchk(nblk, level-1, bap[last], inumber))
		return(1);
	for (index = last + 1; index < NINDIR(&sblock); index++)
		if (bap[index] != 0) {
			pwarn("BAD INDIRECT ADDRESS: IND BLOCK %d[%d] = %d",
				addr, index, bap[index]);
			if (preen)
				printf(" (CORRECTED)\n");
			else {
				printf("\n");
				pinode(inumber);
					if (reply("CORRECT") == 0)
						continue;
			}
			bap[index] = 0;
			dirty(&ptrblk);
		}
	flush(&dfile, &ptrblk);
	return(0);
}

/*
 *  zapino - clear the inode to free it.  We must also increment
 *	the generation number.
 */

zapino(ino)
	struct dinode *ino;
{
	zino.di_gen = ++(ino->di_gen);
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
	    memset((char *) ino, '\0', disk_dinode_size());
	    ino->di_gen = zino.di_gen;
	}
	else
	    *(ino) = zino;
#else
	*(ino) = zino;
#endif /* SecureWare && B1 */
}

usage()
{
	fprintf(stderr, "usage: /etc/fsck [ -p | -P ] [ -F ] [ file system ... ]\n");
	fprintf(stderr, "   or: /etc/fsck [ -b block # ] [ -F ] [ -y ] [ -n ] [ -q ] [ file system ... ]\n");
}
