/* @(#) $Revision: 38.1 $ */      
static char *HPUX_ID = "@(#) $Revision: 38.1 $";
/*
 *	fsck.c
 *
 *	Rewritten for the HP structured directory format.
 *	Last edit:  20 Jun 83, mlc
 */

#include <stdio.h>
#include <ctype.h>
#include "s500defs.h"
#include <sys/stat.h>
#include <ustat.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
/*
#include <psinfo.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/filsys.h>
#include <sys/ino.h>
#include <sys/dir.h>
*/

/*
 *	Global Defines.  Defines local to individual routines appear just
 *	before the routine declaration.
 */

#define	NO	0
#define YES	1

#define	BLDTREE	0			/* defines for processing inodes */
#define	CKUNCLM	1
#define	RECLAIM	2

#define	TNORM	0			/* File types of interest */
#define	TDIR	1

#define	UNSET	0
#define	SET	1

#define	DELETED_ENTRY	187

#define	BLKSZ		vol_hdr.s_blksz		/* volume block size */
#define	MAXBLK		vol_hdr.s_maxblk	/* last addressable block */
#define	NUMBLKS		((int) (MAXBLK + 1))	/* # of blocks on the dev */
#define	BITSPERWORD	32
#define	MOD8(x)		((x) & 7)
#define	MODWS(x)	((x) & 31)		/* x % BITSPERWORD */
#define	FMAPSZ		((int) (NUMBLKS / 8 + (MOD8(NUMBLKS) > 0)))	/* size of free map area */
#define	FARECSZ		((int) sizeof(union fa_entry))		/* FA record size */
			/* offset of the 1st FA record after the free map */
#define	AFTERFMAP	((ino_t) (3 + ((NUMBLKS + ((8 * FARECSZ) - 1)) / (8 * FARECSZ))))
#define	LASTFAREC	((ino_t) (farec.e1.di_size / FARECSZ - 1))	/* last FA record number */
#define	NUMFARECS	((int) (LASTFAREC + 1))
#define	FABITMAPSZ	(NUMFARECS / 8 + (MOD8(NUMFARECS) > 0))	/* size in bytes */
#define	MAXBLKSZ	4096		/* max block size of any volume */
#define HIBIT		0x80000000
#define	VALIDPTR(x)	((x) >= AFTERFMAP && (x) <= LASTFAREC)
#define CORRUPTPTR(x)	(!VALIDPTR(x) && ((x) & HIBIT) == NULL)
#define MAXINT		0x7fffffff
#define	BIT(x)		(1 << (BITSPERWORD - 1 - (x)))	/* bit 0 is leftmost bit */
#define	GLBLSECSZ	1024	/* set BLKSZ to this prior to the first read of */
				/* any volume.  "bread" reads in "BLKSZ" blocks */

#define	FREE	(ino_t) -1		/* some pseudo inode numbers */
#define HDR	(ino_t) -2
#define	UNCLM	(ino_t) -3

#define	MIN(a,b)	((a) < (b) ? (a) : (b))

/* Modcal's TRY ... RECOVER (no nesting) */
#define	TRY		if (!(escapecode = setjmp(err))) {
#define	RECOVER		} else
#define	ESCAPE(x)	longjmp(err, (x))

/*
 * Function "x" is required to succeed (return YES) in order for
 * the calling routine to continue.  Three flavors.
 * NOTE: a syntax error will result if you use one of these with
 * a trailing semicolon immediately before an "else".
 */
#define REQE(x)		{if ((x) == NO) ESCAPE(2);}	/* give up */
#define REQC(x)		{if ((x) == NO) continue;}	/* skip this instance */
#define	REQN(x)		{if ((x) == NO) return(NO);}	/* pass the buck */

#define	BOOL	int;		/* only used as an aid */

typedef	long	dbaddr; 	/* a bytewise device address mode */

extern	end;
extern	int errno;
#ifdef	hp9000s500
extern int errinfo;
#endif	hp9000s500
extern	sbrk(), brk();

int	sflag;			/* reconstruct the free map (unconditionally) */
int	nflag;			/* assume a no response */
int	yflag;			/* assume a yes response */
int	dflag;			/* debug flag */
int	fixFM;			/* rebuild free map flag */
int	lfwrittento;		/* "lost+found" written to? */
int	goodrun;		/* everything fixed? */
int	totalfree;		/* number of free blocks */
int	lffound;		/* flag indicating existance of "lost+found" */
int	lfentries;		/* # of entries currently in "lost+found" */
int	lfmax;			/* max # of entries in "lost+found" */
int	nfiles;			/* # of files on the device */
int	nfiles2;		/* used in an internal consistency test */
int	maxdepth;		/* for testing purposes */
int	treecnt;		/* for testing purposes */
int	ndirents;		/* number of directory entries */
int	escapecode = 0;		/* ESCAPE argument */

long	lseek();

ino_t	lfinode;		/* inode of "lfname" */

jmp_buf	err;

dbaddr	get_FArec_off();	/* function contained later on in this file */
dbaddr	fa_offset;		/* FA file offset in bytes */
dbaddr	lfoffset;		/* location of "lost+found" inode */

char	*lfname		= "lost+found";
char	*freemap;			/* ptr to start of free map copy */
char	*famap;				/* ptr to map of encountered inodes and EM's */
char	*imap;				/* ptr to map of all inodes in FA file */
char	*dirmap;			/* ptr to directory inode map */
char	*myname;
char	*blanks		= "                ";	/* 16 blanks */
char	*nofix		= "NO FIX IS PRESENTLY IMPLEMENTED.\n";
char	*lalloc();

struct filecntl {
	int	rfdes;		/* read descriptor */
	int	wfdes;		/* write descriptor */
};

struct enode {
	ino_t	parent;		/* inode number of extent's parent */
	ino_t	cprnt;		/* in "overlap", the conflicting parent */
	daddr_t	start;		/* starting block number */
	int	size;		/* number of blocks in extent */
	struct enode	*next;	/* pointer to next extent in list */
};

struct etnode {			/* tree version of enode */
	ino_t	parent;		/* inode number of extent's parent */
	daddr_t	start;		/* starting block number */
	int	size;		/* number of blocks in extent */
	struct	etnode	*father,
			*lc,	/* left child */
			*rc;	/* right child */
};

struct FAenode {		/* node for FA file extent */
	ino_t	startrec;	/* first record in extent */
	ino_t	endrec;		/* last record in extent */
	dbaddr	offset;		/* bytes from start of disc */
	struct	FAenode *next;	/* points to next node */
};

struct EMrecno {		/* nodes for EM's not encountered in dir traverse */
	ino_t	no;		/* FA file record number */
	ino_t	parent;		/* number of parent inode */
	struct	EMrecno *next;	/* ptr to next node */
};

struct link_node {		/* node for inodes w other than 1 link */
	ino_t	node;		/* inode number */
	int	r_links;	/* recorded number of links */
	int	e_links;	/* encountered number of links */
	struct	link_node *next;
};

struct dir_info {		/* directory related information */
	ino_t	inode;		/* inode of directory */
	int	startentry;	/* first full entry contained in buf */
	int	numentries;	/* number of entries in directory */
	int	piece;		/* fragment of entry in front of buf */
	char	buf[MAXBLKSZ];	/* buffer for entries */
	struct	dinode dirinode;	/* contents of dir's inode */
};

struct filecntl	dfile;		/* file descriptors for filesys */
struct etnode	*root;		/* root of the extent tree */
struct enode	*unclaimed;	/* start of unclaimed extent list */
struct enode	*overlap;	/* start of overlapped extent list */
struct FAenode	*firstFAenode;	/* points to start of FA extents linked list */
struct filsys	vol_hdr;	/* the volume header */
struct EMrecno	*EMlist;	/* ptr to linked list of abnormal extent maps */
struct link_node *linkptr;	/* ptr to linked list of link nodes */
struct etnode	*get_next_node();
struct etnode	*node0();

union	fa_entry farec;		/* save inode info for FA file */
union	fa_entry glblrec;	/* for misc purposes */
union	fa_entry lfrec;		/* save inode info for "lost+found" */

#define	SDF_LOCK	"/tmp/SDF..LCK"

/*ARGSUSED*/
rmlock(sig)
int sig;
{
	unlink(SDF_LOCK);
	exit(-1);
}

main(argc, argv)
int	argc;
char	**argv;
{
	FILE	*fp;
	char	filename[50];
	int	fd, pid, rmlock();

	sync();				/* dump all disc output */
	setbuf(stdout, (char *) NULL);	/* stderr is already unbuffered */

	myname = argv[0];
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {
			case 't':
			case 'T':
				while (**argv) (*argv)++;
				error("-t option not supported.\n");
				break;
			case 's':	/* unconditional salvage flag */
				stype(++*argv);
				sflag++;
				break;
			case 'S':	/* conditional salvage flag */
				error("-S option not supported.\n");
				break;
			case 'd':	/* debug */
				dflag++;
				while (*++*argv == 'd') dflag++;
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
			default:
				errexit("Bad option: %c.\n",**argv);
		}
	}	/* end of while */

	if (nflag && sflag) {
		error("Incompatible options: -n and -s.  -s ignored.\n");
		sflag = 0;
	}

	if (argc) {		/* arg list has names */
		if (access(SDF_LOCK, 0) == 0) {
cant:			error("%s: SDF lock file %s exists; can't proceed\n",
			    myname, SDF_LOCK);
			exit(1);
		}
		signal(SIGHUP, rmlock);
		signal(SIGQUIT, rmlock);
		signal(SIGINT, rmlock);
		signal(SIGTERM, rmlock);
		signal(SIGPIPE, rmlock);
		fd = open(SDF_LOCK, O_WRONLY | O_EXCL | O_CREAT, 0666);
		if (fd < 0)
			goto cant;
		pid = getpid();
		write(fd, (char *) &pid, sizeof(int));
		close(fd);
		while (argc-- > 0) {
			TRY
				check(*argv++);
			RECOVER
				recover();
			cleanup();
		}
		unlink(SDF_LOCK);
		exit(0);
	}
	else {
		fprintf(stderr, "Usage: %s [-y] [-n] [-s] [-d] device\n",
			myname);
		exit(1);
	}
}


stype(p)
char *p;
{
	if (*p)			/* p should point to a null char */
		error("Free-list organization formats are not supported.  Format ignored.\n");
}


check(dev)			/* This is the routine that coordinates */
char *dev;			/* all the actual checking & repair. */
{
	struct	etnode	*eptr, *enptr, *e2ptr;
	struct	EMrecno	*fptr;
	struct	enode	*gptr;
	struct	link_node *lptr;
	struct	direct	dir;
	union	fa_entry tmprec;
	char	nm[20];
	char	istring[2][16];
	ino_t	itemp;
	int	i;
	int	firstencounter;
	int	orphEM;			/* used to look for orphaned EM's */
	int	diff;			/* used in checking extent list */
	int	unclmsum = 0;		/* number of unclaimed blocks */

			/*************/
			/*  Phase 1  */
			/*************/
	printf("\nChecking %s.\n",dev);
	goodrun = YES;
	lfwrittento = NO;
	EMlist = NULL;
	linkptr = NULL;
	maxdepth = 0;
	treecnt = 0;
	nfiles = 1;		/* assume the root is present */
	nfiles2 = -1;		/* the root and the FA file will be added in */
	ndirents = 1;		/* justified by . in / */
	setup(dev);


			/*************/
			/*  Phase 2  */
			/*************/
	printf("\n**Checking Directories.\n");

	/* Note that I've cheated and know about inode 0 by now */
	famap = lalloc(FABITMAPSZ);	/* get inode bit map */

	/* We know that inodes 0 to end of free map are used */
	set_bits_in_map(famap, 0, (int) AFTERFMAP);

	/* Get the bit maps for inodes and directories */
	imap   = lalloc(FABITMAPSZ);
	dirmap = lalloc(FABITMAPSZ);

	if (dflag)
		printf("\tFinding inodes.\n");
	for (i = AFTERFMAP; i <= LASTFAREC; i++) {
		bread_FAbuf((ino_t) i, (char *) &glblrec);
		if (glblrec.e1.di_type == R_INODE) {
			set_bits_in_map(imap, i, 1);
			if (glblrec.e1.di_ftype == TDIR)
				set_bits_in_map(dirmap, i, 1);
			FArec_validity_ck((ino_t) i, &glblrec);
		}
		if (glblrec.e1.di_type == R_EM)
			FArec_validity_ck((ino_t) i, &glblrec);
	}

	/* Traverse all directories and fill in "famap" */
	if (dflag)
		printf("\tTraversing directories.\n");
	traverse("", (ino_t) 1);
	/*
	 *  I would like to "dalloc" imap and dirmap here, but I've probably done some
	 *  "lallocs" for link count records during the directory traversal.
	 */

	if (dflag)
		printf("\tBuilding extent tree.\n");
	startll();		/* start the extent tree */

	/* traverse all the inodes set in "famap" and build the rest of the */
	/* extent tree */
	for (i = get_next_fa_rec((int) (AFTERFMAP - 1), SET); i <= LASTFAREC; i = get_next_fa_rec(i, SET)) {
		REQE(pros_inode((ino_t) i, BLDTREE, NULL));
	}
	if (nfiles != nfiles2) {
		error("%s: internal inconsistency.\n", myname);
		error("\tNumber of files encountered: %d, number of files checked: %d\n", nfiles, nfiles2);
		if (reply("Quit") == YES)
			ESCAPE(1);
	}
	if (dflag)
		printf("\nMax tree depth = %d\n", maxdepth);
	if (dflag > 1)
		printf("\nNumber of nodes in tree = %d\n", treecnt);
	if (dflag > 2) {
		printf("\nExtent tree:\n");
		for (eptr = node0(); eptr != NULL; eptr = get_next_node(eptr))
			printf("\t\tstart=%d, size=%d, parent=%d\n",
				eptr->start, eptr->size, eptr->parent);
	}

			/*************/
			/*  Phase 3  */
			/*************/
	printf("\n**Checking Blocks.\n");
/*
 *	Build the unclaimed and multiply claimed block lists from the
 *	extent tree.
 */
	unclaimed = overlap = NULL;
	treecnt = 1;
	for (eptr = node0(), enptr = get_next_node(eptr); enptr != NULL; eptr = enptr, enptr = get_next_node(enptr)) {
		treecnt++;
		if (enptr->start <= MAXBLK) {
			if (diff = (enptr->start - (eptr->start + eptr->size))) {
				if (diff > 0) {
					bld_ext_ll(&unclaimed, eptr->start + eptr->size,
						diff, UNCLM);
					unclmsum += diff;
				}
				else {	/* diff < 0 */
					bld_ext_ll(&overlap, enptr->start,
						MIN(-diff, enptr->size), eptr->parent,
						enptr->parent);
					/* look at nodes beyond the adjacent one */
					for (e2ptr = get_next_node(enptr);
					    e2ptr != NULL && e2ptr->start < (eptr->start + eptr->size);
					    e2ptr = get_next_node(e2ptr)) {
						diff = eptr->start + eptr->size - e2ptr->start;
						bld_ext_ll(&overlap, e2ptr->start, MIN(diff, e2ptr->size),
							eptr->parent, e2ptr->parent);
					}
				}
			}
		}
		else {
			if (diff = (MAXBLK - (eptr->start + eptr->size - 1))) {
				if (diff > 0) {
					bld_ext_ll(&unclaimed, eptr->start + eptr->size,
						diff, UNCLM);
					unclmsum += diff;
				}
				else {		/* diff < 0 */
					/* over the end of the disc */
					error("Inode %d claims an extent beyond the end of the disc.\n", eptr->parent);
					error("\tStart = %d, size = %d.\n", eptr->start, eptr->size);
					error(nofix);
					goodrun = NO;
				}
			}
		}
	}	/* end of the for */
	eptr = root;
	while (eptr->rc != NULL)
		eptr = eptr->rc;
		/* eptr points to the last node */
	if (eptr->start + eptr->size - 1 < MAXBLK) {	/* does the list have the end of the disc? */
		bld_ext_ll(&unclaimed, eptr->start + eptr->size, (int)
			(MAXBLK - (eptr->start + eptr->size - 1)), UNCLM);
		unclmsum += MAXBLK - (eptr->start + eptr->size - 1);
	}
	if (dflag > 1)
		printf("\nNumber of tree nodes checked: %d\n", treecnt);
	if (unclmsum) printf("Number of unclaimed blocks: %d.\n", unclmsum);
	if (dflag) {
		if (unclaimed != NULL) printf("\nUnclaimed extents:\n");
		for (gptr = unclaimed; gptr != NULL; gptr = gptr->next)
			printf("\t%d to %d\n", gptr->start, gptr->start + gptr->size - 1);
	}
	if (overlap != NULL) {
		error("\nMULTIPLE FILES CLAIM THE SAME SPACE ON THIS DISC!!\n");
		if (!sflag)
			error("The -s option will correct this problem if one of the claimants is the free map.\n");
		else
			error("The free map may not be restored correctly.\n");
			/* eg, if one extent lies entirely within another and */
			/* the borders don't align */
		error(nofix);
		printf("Multiply claimed extents:\n");
		for (gptr = overlap; gptr != NULL; gptr = gptr->next) {
			for(i = 0; i < 2; i++) {
				itemp = i ? gptr->parent : gptr->cprnt;
				if (itemp == -1 )
					sprintf(istring[i], "%s", "the free map");
				else if (itemp == -2)
					sprintf(istring[i], "%s", gptr->start ? "the boot area" : "the superblock");
				else
					sprintf(istring[i], "inode %d", itemp);
			}
			printf("\t%d to %d, claimed by %s and %s\n", gptr->start,
				gptr->start + gptr->size - 1, istring[0], istring[1]);
		}
		goodrun = NO;
	}

			/*************/
			/*  Phase 4  */
			/*************/
	printf("\n**Checking The File Attribute File.\n");
	fixFM = YES;
	/* check all FA records that follow the free map */
	/* and that haven't been looked at yet */
	for (i = get_next_fa_rec((int)(AFTERFMAP - 1), UNSET); i <= LASTFAREC; i = get_next_fa_rec(i, UNSET)) {
		bread_FAbuf((ino_t) i, (char *) &glblrec);
		if (glblrec.e1.di_type == R_UNUSED)
			continue;
		if (glblrec.e1.di_type == R_INODE) {
			/* check to see if its an intact orphan */
		    	if (unclaimed != NULL && pros_inode((ino_t) i, CKUNCLM, NULL) == YES) {
				error("Inode %d is orphaned.", i);
				if (glblrec.e1.di_ftype == TDIR) {
					error("  It is also a directory and cannot be restored.\n");
				}
				else if (glblrec.e1.di_size == 0) {
					error("  Its size is zero.");
					/* continue on to "addtoFM()" */
				}
				else if (!lffound) {
					error("  %s not found.", lfname);
				}
				else {
					if (reply("Restore it") == YES) {
						sprintf(nm, "%d", i);
						sprintf(dir.d_name, "%.*s%.*s", strlen(nm),
							nm, DIRSIZ+2-strlen(nm), blanks);
						dir.d_object_type = dir.d_file_code = 0;
						dir.d_ino = i;
						mk_lf_entry(&dir);
						continue;
					}
				}
			}
			else	/* it's not intact */
				error("Inode %d is an orphan but its space is claimed by others.", i);
			REQC(addtoFM((ino_t) i, &glblrec, YES));
		}	/* end if R_INODE */
		/*
		 * We can't yet add corrupted EM's to the FM since they
		 * are probably part of an orphan described by an inode.
		 */
		else if (glblrec.e1.di_type == R_EM) {
			bld_EM_ll((ino_t) i, glblrec.e2.e_inode);
		}
		/*
		 * Currently, password, label, and free list records are
		 * being ignored.
		 */
		else if (glblrec.e1.di_type < R_INODE || glblrec.e1.di_type > R_FREEL) {
			error("Record type %d encountered in FA record %d\n", glblrec.e1.di_type, i);
			REQC(addtoFM((ino_t) i, &glblrec, YES));
		}
	}	/* end for loop */
	if (lfwrittento) {
		lfrec.e1.di_size = lfentries * sizeof(struct direct);
		REQE(bwrite(&dfile, (char *) &lfrec, lfoffset, sizeof(union fa_entry)));
	}

			/*************/
			/*  Phase 5  */
			/*************/

	/*
	 * Check for orphaned EM's.  This check requires that all normal
	 * orphans were restored in the last check.
	 */

	printf("\n**Checking Extent Maps.\n");
	for (fptr = EMlist; fptr != NULL; fptr = fptr->next) {
		/*
		 * Check if any EM still claims UNCLM space.  What
		 * we have here is an orphaned EM.
		 */
		if (dflag > 2)
			printf("Checking EM %d\n", fptr->no);
		read_FArec(fptr->no, (char *) &glblrec);
		if (glblrec.e2.e_type == R_UNUSED)	/* it may already have been thrown away */
			continue;
		orphEM = YES;
		/* invalidate the "next" ptr so we don't throw away any random records */
		glblrec.e2.e_next = -1;
		for (i=0; i<glblrec.e2.e_exnum; i++) {
			/* restored orphans are no longer unclaimed */
			if (ckunclm(glblrec.e2.e_extent[i].e_startblk,
				glblrec.e2.e_extent[i].e_numblk) == NO)
				orphEM = NO;
		}	/* end for loop */
		if (orphEM) {
			error("Orphaned extent map (%d) encountered.\n", fptr->no);
			if (goodrun) {
				REQC(addtoFM(fptr->no, &glblrec, YES));
			}
			else
				error("\tUnable to fix due to corrective actions not being accomplished.\n");
		}
		else {		/* check its parent */
			tmprec = glblrec;
			read_FArec(glblrec.e2.e_inode, (char *) &glblrec);
			/*
			 *  This next test will allow some cases to slip
			 *  by but a more thorough test isn't worth the
			 *  effort.  I should check the whole EM chain
			 *  but this happens seldom & the worst we can
			 *  do is lose this FA record till things change.
			 */
			if (glblrec.e1.di_type != R_INODE || !VALIDPTR(glblrec.e1.di_exmap)) {
				error ("Corrupt orphaned extent map (%d) encountered.\n", fptr->no);
				REQC(addtoFM(fptr->no, &tmprec, YES));
			}
		}
	}	/* end for */

			/*************/
			/*  Phase 6  */
			/*************/
	printf("\n**Checking Link Counts.\n");
	for (lptr = linkptr; lptr != NULL; lptr = lptr->next) {
		if (lptr->r_links != lptr->e_links) {
			error("Wrong link count for inode %d.\n", lptr->node);
			error("\tIt's currently %d but should be %d.\n", lptr->r_links, lptr->e_links);
			if (reply("Shall I fix it") == YES) {
				read_FArec(lptr->node, (char *) &glblrec);
				glblrec.e1.di_count = lptr->e_links;
				REQC(bwrite(&dfile, (char *) &glblrec, get_FArec_off(lptr->node), FARECSZ));
			}
		}
	}

			/*************/
			/*  Phase 7  */
			/*************/
	if (unclaimed != NULL && fixFM == YES) {
		printf("\n**Checking The Free Map.\n");
		firstencounter = YES;
		for (gptr = unclaimed; gptr != NULL; gptr = gptr->next) {
			if (gptr->parent == UNCLM) {
				if (firstencounter) {
					if (!sflag || nflag) {
						if (reply("Shall I return lost blocks to the free map") == NO) {
							goodrun = NO;
							break;
						}
					}
					else
						error("Updating the free map.\n");
					firstencounter = NO;
				}
				set_bits_in_map(freemap, (int) gptr->start, gptr->size);
			}
		}	/* end of for loop */
		if (!firstencounter) {		/* we've updated the free map */
			REQE(bwrite(&dfile, freemap, fa_offset + 3 * FARECSZ, FMAPSZ));
		}
	}
	else {
		if (unclaimed == NULL) {
			printf("\n**No Need To Check Free Map.\n");
			printf("\tThere are no unclaimed blocks.\n");
		}
		else {
			printf("\n**Unable To Check Free Map.\n");
			printf("\tThis device still has some uncorrected problems.\n");
			goodrun = NO;
		}
	}

			/*************/
			/*  Phase 8  */
			/*************/
	if (goodrun == YES) {
		if (dfile.wfdes != -1 && vol_hdr.s_corrupt) {
			vol_hdr.s_corrupt = 0;
			if (bwrite(&dfile, (char *) &vol_hdr, (dbaddr) 0, sizeof(vol_hdr)) == NO)
				error("Failed to update corrupt bit.\n");
		}
	}
	printf("\n%s statistics:\n", dev);
	printf("\ttotal number of files:\t%d\n", nfiles);
	printf("\ttotal number of blocks:\t%d\n", NUMBLKS);
	printf("\tnumber of user blocks:\t%d\n", NUMBLKS - 1 - totalfree - vol_hdr.s_bootsz - farec.e1.di_blksz);
	printf("\tnumber of free blocks:\t%d\n", totalfree);
	printf("\tpercent of disc unused:\t%d\n", totalfree*100/NUMBLKS);
	if (!goodrun) {
		printf("\nTHIS FILE SYSTEM IS NOT COMPLETELY RESTORED.\n\n");
		if (overlap == NULL)
			printf("\tBut no critical problems were encountered.\n\n");
		else
			printf("\tPlease fix the multiply-claimed extents before using this device.\n\n");
	}
}	/* end check */


setup(dev)			/* check the volume and do the initial setup */
char *dev;
{
	dev_t		rootdev;
	struct	stat	statarea;
	struct	ustat	ustatarea;
	struct	psblock	*psbp, *psp, *psinfo();
	int		i;

	if (stat("/", &statarea) < 0)
		errexit("Can't stat root.\n");
	rootdev = statarea.st_dev;
	if (stat(dev, &statarea) < 0) {
		error("Can't stat %s.\n", dev);
		ESCAPE(2);
	}
/*
	if ((statarea.st_mode & S_IFMT) != S_IFCHR &&
	    (statarea.st_mode & S_IFMT) != S_IFBLK) {
		error("%s is not a block or character device.\n", dev);
		ESCAPE(2);
	}
*/
	if (rootdev == statarea.st_rdev) {
		fprintf(stderr, "This is the root device.\n");
		fprintf(stderr, "Use \"fsck\" instead of \"sdffsck\"\n");
		exit(1);
	}
	if (ustat(statarea.st_rdev, &ustatarea) == 0) {
		fprintf(stderr, "This is device is mounted.\n");
		fprintf(stderr, "Use \"fsck\" instead of \"sdffsck\"\n");
		exit(1);
	}
	if ((dfile.rfdes = open(dev,0)) < 0) {		/* open for reading */
		error("Can't open %s.\n", dev);
#ifdef	hp9000s500
		error("\tErrno = %d, errinfo = %d\n", errno, errinfo);
#else
		error("\tErrno = %d\n", errno);
#endif	hp9000s500
		ESCAPE(2);
	}
	if (nflag || (dfile.wfdes = open(dev,1)) < 0) {	/* open for writing */
		dfile.wfdes = -1;
		error("\tCan't write to %s, continuing for diagnostics only.\n", dev);
	}

		/* Get the volume header info, which is in block zero */

	BLKSZ = GLBLSECSZ;	/* BLKSZ is presently not set for this volume */
	REQE(bread(&dfile, (char *) &vol_hdr, (dbaddr) 0, sizeof(vol_hdr)));
	if (vol_hdr.s_format != 0x700) {
		error("%s has an unknown format (%#x).\n", dev, vol_hdr.s_format);
		ESCAPE(2);
	}
	fa_offset = vol_hdr.s_fa * BLKSZ;
	if (BLKSZ > MAXBLKSZ) {
		error("\nThe blocksize (%d) for %s is too large for %s.\n",
			BLKSZ, dev, myname);
		ESCAPE(2);
	}

	if (dflag) {
		printf("Volume Header:\n");
		printf("\tformat = %#x\n\tcorrupt = %d\n", vol_hdr.s_format, vol_hdr.s_corrupt);
		printf("\tblock size = %d\n\tmax block = %d\n", BLKSZ, MAXBLK);
		printf("\tFA file starts at block %d\n", vol_hdr.s_fa);
		printf("\n");
	}

	/* build linked list of extents for the FA file */
	bld_FA_ll();

	/* Check that /<device>/lost+found is around */
	checklf();
}


checklf()		/* check the "lost+found" directory */
{
	int	i = 0;
	struct	direct	dir;
	struct	dir_info dinfo;

	dinfo.inode = (ino_t) 1;
	dinfo.startentry = -1;		/* to indicate a new directory */
	while (dirread(i++, &dir, &dinfo) == YES) {
		if (strncmp(dir.d_name, lfname, strlen(lfname)) == 0) {
			read_FArec(dir.d_ino, (char *) &glblrec);
			lfrec = glblrec;
			lfoffset = get_FArec_off(dir.d_ino);
			lfinode = dir.d_ino;
			lfentries = lfrec.e1.di_size / (int) sizeof(struct direct);
			lfmax = lfrec.e1.di_blksz * (BLKSZ / (int) sizeof(struct direct));
			if (lfmax == 0) {	/* remove when kernel bug fixed */
				error("Warning: %s has no blocks allocated to it.\n", lfname);
				error("\tNo new entries may be made.\n");
			}
			lffound = YES;
			return;
		}
	}
	error("%s directory not found.\n", lfname);
	if (reply("Quit") == YES)
		ESCAPE(1);
	lfentries = lfmax = 0;
	lffound = NO;
}


#define	WRDRM	(BITSPERWORD <= MAXBLK-fbit+1 ? BITSPERWORD : MAXBLK-fbit+1)
#define	ALLZEROS	0
#define	ALLONES	0xffffffff

startll()		/* start the extent linked list */
{
	int	fsize = 0;		/* size of free block */
	int	*fm;			/* pointer into free map */
	int	s;			/* temp buffer for word of free map */
	int	i, j;
	daddr_t	fbit = 0;		/* current free bit/block */
	daddr_t	fstart = 0;		/* start of free block */

	root = NULL;			/* cause it's used by bld_ext_tree */
	REQE(pros_inode((ino_t) 0, BLDTREE, NULL));	/* inode for FA file */
	REQE(pros_inode((ino_t) 1, BLDTREE, NULL)); 	/* inode for root dir */
	/* No need to process inode 2, the start of the free list */

	bld_ext_tree((daddr_t) 0, 1, HDR);		/* add the vol hdr to the list */
	if (vol_hdr.s_bootsz > 0)			/* add boot area */
		bld_ext_tree(vol_hdr.s_boot, vol_hdr.s_bootsz, HDR);

	/*
	 * Process the free map.  Charlie assures me that the free
	 * map will always be in the first extent of the FA file.
	 * Note that if a block is free, its corresponding bit is
	 * set to 1.  Zero, therefore, means the block is being used.
	 */

	freemap = lalloc(FMAPSZ);

	totalfree = 0;
	if (sflag)		/* force free map rebuilding, */
		return;		/* no free blocks in map at this point */
	REQE(bread(&dfile, freemap, fa_offset + 3 * FARECSZ, FMAPSZ));
	fm = (int *) freemap;

	while (fbit < MAXBLK) {		/* go through the bit map */
		s = *fm++;
		if (s == ALLZEROS) {			/* an easy case */
			if (fsize) {
				bld_ext_tree(fstart, fsize, FREE);
				fsize = 0;
			}
			fbit += WRDRM;
		}
		else if (s == ALLONES) {		/* another easy case */
			if (!fsize) fstart = fbit;
			fsize += WRDRM;
			fbit += WRDRM;
			totalfree += BITSPERWORD;
		}
		else {				/* process one bit at a time */
			for (i=0, j=WRDRM; i<j; i++) {
				if (s & HIBIT) {
					if (!fsize) fstart = fbit;
					fsize++;
					totalfree++;
				}
				else {
					if (fsize) bld_ext_tree(fstart, fsize, FREE);
					fsize = 0;
				}
				s <<= 1;
				fbit++;
			}	/* end for */
		}	/* end else */
		if (dflag > 4) printf("\t\t\t\t%d bits into the freemap.\n", fbit);
	}	/* end while */
	if (fsize) bld_ext_tree(fstart, fsize, FREE);
	if (dflag) printf("Free map built.  Number of free blocks = %d.  Last block = %d.\n",
		totalfree, fbit-1);
}


#define	ISDIRECT(x)	((*((int *) dirmap + (x) / BITSPERWORD)) & BIT(MODWS(x)))
#define	ENCOUNTERED(x)	((*((int *) famap + (x) / BITSPERWORD)) & BIT(MODWS(x)))
#define	NOTINODE(x)	(!((*((int *) imap + (x) / BITSPERWORD)) & BIT(MODWS(x))))

traverse(path, thisdir)		/* Recursive traversal of directories. */
char	*path;
ino_t	thisdir;
{
	struct	direct dir;
	int	i=0, j;
	char	nm[100];
	struct	dir_info dinfo;

	dinfo.inode = thisdir;
	dinfo.startentry = -1;		/* to indicate a new directory */
	while (dirread(i++, &dir, &dinfo) == YES) {
		ndirents++;
		for (j=DIRSIZ-1; j>0; j--)	/* our sdf blank pads--put on an ending null */
			if (dir.d_name[j] != ' ') break;
		if (dir.d_name[j+1] == ' ')
			dir.d_name[j+1] = '\0';
		sprintf(nm, "%s/%.*s", path, DIRSIZ, dir.d_name);
		if (dflag > 2) printf("\t\tProcessing %s\n", nm);
		/*
		 * The following checks for invalid directory entries.
		 */
		if ((dir.d_object_type == DELETED_ENTRY) && (dir.d_ino == 0)) {
			if (!nflag) {
				REQC(rm_dir_entry(i-1, &dinfo));
				i--;
				ndirents--;
			}
		}
		else {
			if ((dir.d_ino != 1 && dir.d_ino < AFTERFMAP) ||
				dir.d_ino > LASTFAREC ||
				NOTINODE((int) dir.d_ino) ||
				dir.d_object_type || dir.d_file_code) {
				/*
			 	*  Don't add one of these to "famap"
				*/
				error("Bad directory entry encountered for %s\n", nm);
				if (dflag)
					printf("inode = %d, obj type = %d, file code = %d\n",
						dir.d_ino, dir.d_object_type, dir.d_file_code);
				if (reply("Shall I remove this entry") == YES) {
					REQC(rm_dir_entry(i-1, &dinfo));
					i--;
					ndirents--;
				}
				else if (reply("Continue") == NO)
					ESCAPE(1);
				continue;
			}
	
			if (ENCOUNTERED((int) dir.d_ino)) {
				bld_linkcnt_ll(dir.d_ino, 1, 2);
				if (ISDIRECT((int) dir.d_ino)) {
					error("WARNING:  %s is a multiply linked directory.\n", nm);
					continue;	/* don't want to traverse it again */
				}
			}
			else {
				nfiles++;
				set_bits_in_map(famap, (int) dir.d_ino, 1);
			}
	
			if (ISDIRECT((int) dir.d_ino)) {
				if (dflag > 1)
					printf("\tProcessing directory %s\n", nm);
				/*
			 	* Check backward link.
			 	*/
				read_FArec(dir.d_ino, (char *) &glblrec);
				if (glblrec.e1.di_parent != thisdir) {
					error("%s's .. inode is wrong.  \n\tIt's currently %d but should be %d.\n",
						nm, glblrec.e1.di_parent, thisdir);
					if (reply("Shall I fix it") == YES) {
						glblrec.e1.di_parent = thisdir;
						if (bwrite(&dfile, (char *) &glblrec, get_FArec_off(dir.d_ino), FARECSZ) == NO)
							error("Fix failed.\n");
					}
				}
				traverse(nm, dir.d_ino);
			}
		}
	}	/* end while */
}


BOOL
pros_inode(fa_node, flag, parent)		/* process a FA record */
ino_t	fa_node, parent;
int	flag;
{
		union	fa_entry rec;
		int	i;

		if (dflag > 3) printf("\t\t\tProcessing inode %d.\n", fa_node);

		bread_FAbuf(fa_node, (char *) &rec);

		if (rec.e1.di_type == R_INODE) {
			if (flag == BLDTREE) {
				nfiles2++;
				if (rec.e1.di_count != 1)
					bld_linkcnt_ll(fa_node, (int) rec.e1.di_count, 1);
				for (i=0; i<rec.e1.di_exnum; i++) {
					bld_ext_tree(rec.e1.di_extent[i].di_startblk,
						rec.e1.di_extent[i].di_numblk,
						fa_node);
				}
			}
			else if (flag == CKUNCLM) {
				if (rec.e1.di_exnum == 0) return(YES);
				for (i=0; i<rec.e1.di_exnum; i++) {
					REQN(ckunclm(rec.e1.di_extent[i].di_startblk,
						rec.e1.di_extent[i].di_numblk));
				}
			}
			else if (flag == RECLAIM) {
				for (i=0; i<rec.e1.di_exnum; i++) {
					REQN(fixunclm(rec.e1.di_extent[i].di_startblk,
						rec.e1.di_extent[i].di_numblk, fa_node));
				}
				rec.e1.di_count = 1;
				REQN(bwrite(&dfile, (char *) &rec, get_FArec_off(fa_node), FARECSZ));
			}
			if (VALIDPTR(rec.e1.di_exmap))
				return(pros_inode(rec.e1.di_exmap, flag, fa_node));
		}
		else if (rec.e1.di_type == R_EM ) {
			/* Have we seen this one before? */
			if (flag == BLDTREE && ENCOUNTERED((int) fa_node))
				return(YES);	/* no need to follow EM chain */
			if (parent != rec.e2.e_inode) {
				error("Owner conflict for extent map %d;\n\t claimants are inodes %d and %d.\n",
					fa_node, parent, rec.e2.e_inode);
				error(nofix);
				goodrun = NO;
				return(NO);
			}
			if (flag == BLDTREE) {
				set_bits_in_map(famap, (int) fa_node, 1);
				for (i=0; i<rec.e2.e_exnum; i++) {
					bld_ext_tree(rec.e2.e_extent[i].e_startblk,
						rec.e2.e_extent[i].e_numblk,
						rec.e2.e_inode);
				}
			}
			else if (flag == CKUNCLM) {
				for (i=0; i<rec.e2.e_exnum; i++) {
					REQN(ckunclm(rec.e2.e_extent[i].e_startblk,
						rec.e2.e_extent[i].e_numblk));
				}
			}
			else if (flag == RECLAIM) {
				for (i=0; i<rec.e2.e_exnum; i++) {
					REQN(fixunclm(rec.e2.e_extent[i].e_startblk,
						rec.e2.e_extent[i].e_numblk, parent));
					}
			}
			if (VALIDPTR(rec.e2.e_next))
				return(pros_inode(rec.e2.e_next, flag, parent));
		}
		else {
			error("While processing a file, an FA record ");
			error("type %d was encountered in FA record %d\n",
				rec.e1.di_type, fa_node);
			error("\t(parent = %d, flag = %d)\n", parent, flag);
			if (reply("Continue") == NO)
				ESCAPE(1);
		}
	return(YES);
}


			/************************/
			/*			*/
			/*     I/O routines	*/
			/*			*/
			/************************/


int
getline(fp, loc, maxlen)	/* Read a line from file fp and put it in */
FILE *fp;			/* loc which has maxlen characters. */
char *loc;
{
	int n;
	char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF) return(EOF);
		if (!isspace(n) && p < lastloc) *p++ = n;
	}
	*p = 0;			/* terminate the string */
	return(p - loc);	/* return the length */
}


BOOL
bread(fcp, buf, off, size)		/* read an arbitrary sized block of fcp */
struct filecntl *fcp;			/* starting on any byte boundary */
char	*buf;
dbaddr	off;
int	size;
{
	dbaddr	r;			/* offset from block boundary */
	int	i;
	int	blksz = BLKSZ;		/* so it won't change out from under us */
	char	readbuf[MAXBLKSZ];

	if (dflag > 3) printf("\t\t\tReading offset %d for %d.\n", off, size);

	if (r = off % blksz) {	/* read a partial block */
		if (lseek(fcp->rfdes, off-r, 0) < 0) {
			rwerr("seek", off-r);
			return(NO);
		}
		if (read(fcp->rfdes, readbuf, blksz) != blksz) {
			rwerr("read", off-r);
			return(NO);
		}
		for (i = 0; i < MIN(blksz-r, size); i++)
			*buf++ = *(readbuf + r + i);
		off += (dbaddr)(blksz-r);
		if ((size -= (blksz - r)) <= 0) return(YES);
	}

	/* "off" should be block aligned at this point */

	if (lseek(fcp->rfdes, off, 0) < 0) {
		rwerr("seek", off);
		return(NO);
	}
	else if (size % blksz == 0) {
		if (read(fcp->rfdes, buf, size) == size)
			return(YES);
		else {
			rwerr("read", off);
			return(NO);
		}
	}
	else {
		while (size > 0) {		/* need to read in block increments */
			/* This next line could cause trouble if we're reading the end of the disc */
			if (read(fcp->rfdes, readbuf, blksz) != blksz) {
				rwerr("read", off);
				return(NO);
			}
			for (i=0; i<MIN(size, blksz); i++)
				*buf++ = *(readbuf + i);
			off += (dbaddr) blksz;
			size -= blksz;
		}
	}
	return(YES);
}


read_FArec(node, buf)		/* read a random FA record */
ino_t	node;
char	*buf;
{
	if (bread(&dfile, buf, get_FArec_off(node), FARECSZ) == NO) {
		error("Can't read FA record %d\n", node);
		ESCAPE(2);
	}
}


/*
 *	Use the following routine when doing sequential reads of the FA file.
 *	Reads are done by blocks and the information is buffered to minimize
 *	actual reads.  The buffer is only one block in size so I don't have 
 *	to worry about FA files with multiple extents.
 */

bread_FAbuf(fa_node, buf)
ino_t	fa_node;
char	*buf;
{
	static	char sbuf[MAXBLKSZ];
	static	ino_t startrec = (ino_t) MAXINT;	/* as good as any other bad value */
	int	nrecs = BLKSZ/FARECSZ;			/* number of records in one block */
	int	i, j;

	/* is the record already in the buffer?  If not, read it in. */
	if (fa_node < startrec || fa_node >= startrec + nrecs) {
		/* fill the buffer */
		startrec = fa_node - ((int) fa_node % nrecs);
		if (bread(&dfile, sbuf, get_FArec_off(startrec), BLKSZ) == NO) {
			error("Can't read FA record %d\n", fa_node);
			ESCAPE(2);
		}
	}
	for (i = j = (fa_node - startrec) * FARECSZ; i < j + FARECSZ; i++)
		*buf++ = sbuf[i];
}


/*
 *	Read a directory entry.  The buffers and associated information
 *	must be passed to me.  The buffer size must be >= BLKSZ.
 *	The first time you call me for a directory, make sure
 *	"dinfo->startentry" is set to "-1".
 */
BOOL
dirread(entry, dp, dinfo)
int	entry;			/* the entry to read */
struct	direct	*dp;		/* pointer to directory descriptor */
struct	dir_info *dinfo;	/* pointer to a bunch of stuff */
{
	dbaddr	offset, get_dir_off();
	int	i, j;

	if (dinfo->startentry == -1 || entry < dinfo->startentry ||
	    (dinfo->piece + (entry - dinfo->startentry) * sizeof(struct direct) >= BLKSZ &&
	    entry < dinfo->numentries)  /* needed to prevent seek error */ ) {
		if ((offset = get_dir_off(entry, dinfo)) == NULL)
			return(NO);
		REQN(bread(&dfile, dinfo->buf, offset, BLKSZ));
	}
	if (entry >= dinfo->numentries)
		return(NO);
	if ((i = dinfo->piece + (entry - dinfo->startentry) * sizeof(struct direct)) +
	    sizeof(struct direct) <= BLKSZ) {
		*dp = *(struct direct*) &(dinfo->buf[i]);	/* structure assignment */
	}
	else {
		for (j = i; j < BLKSZ; j++)
			*((char *) dp)++ = dinfo->buf[j];
		if ((offset = get_dir_off(entry+1, dinfo)) == NULL)
			return(NO);
		REQN(bread(&dfile, dinfo->buf, offset, BLKSZ));
		for (j = 0; j < dinfo->piece; j++)
			*((char *) dp)++ = dinfo->buf[j];
	}
	return(YES);
}


BOOL
bwrite(fcp, buf, off, size)		/* write an arbitrary sized block to fcp */
struct	filecntl *fcp;			/* starting on any byte boundary */
char	*buf;
dbaddr	off;
int	size;
{
	dbaddr	r;			/* offset from block boundary */
	int	i, firstime = YES;
	char	writebuf[MAXBLKSZ];
	char	*c;

	if (dflag > 3) printf("\t\t\tWriting to offset %d for %d.\n", off, size);

	while (size > 0) {
		if ((r = off % BLKSZ) || size < BLKSZ) {
			REQN(bread(fcp, writebuf, off-r, BLKSZ));
			c = writebuf;
			for (i=0; i < MIN(BLKSZ-r, size); i++)
				*(c + r + i) = *buf++;
		}
		else {
			c = buf;
			buf += BLKSZ;
		}
		if (firstime) {
			if (lseek(fcp->wfdes, off-r, 0) < 0) {
				rwerr("seek", off-r);
				return(NO);
			}
			firstime = NO;
		}
		if (write(fcp->wfdes, c, BLKSZ) != BLKSZ) {
			rwerr("write", off-r);
			return(NO);
		}
		off += (dbaddr) (BLKSZ-r);
		size -= (BLKSZ - r);
	}	/* end of while loop */
	return(YES);
}


rwerr(s, off)
char *s;
dbaddr off;
{
	error("\nCan't %s: offset %d.", s, off);
#ifdef	hp9000s500
	error("\tErrno = %d, errinfo = %d\n", errno, errinfo);
#else
	error("\tErrno = %d\n", errno);
#endif	hp9000s500
	if (reply("Quit") == YES)
		ESCAPE(1);
}


BOOL
reply(s)
char *s;
{
	char line[80];

	error("\n%s? ", s);
	if (nflag || dfile.wfdes < 0) {
		error("no\n\n");
		return(NO);
	}
	if (yflag) {
		error("yes\n\n");
		return(YES);
	}
	for (;;) {
		if (getline(stdin, line, sizeof(line)) == EOF) {
			error("\nEOF encountered.\n");
			ESCAPE(2);
		}
		error("\n");
		if (line[0] == 'y' || line[0] == 'Y')
			return(YES);
		else if (line[0] == 'n' || line[0] == 'N')
			return(NO);
		error("Invalid response, please re-enter: ");
	}
}


			/*************************************/
			/*				     */
			/* Functions that build data objects */
			/*				     */
			/*************************************/

bld_ext_tree(st, sz, prnt)		/* add an extent to the all encompassing */
daddr_t	st;				/* tree structure */
int	sz;
ino_t	prnt;
{
	struct	etnode *thisnode, *t;
	int	depth = 1;

	/* I expect the heap to be zeroed */
	thisnode = (struct etnode *) lalloc(sizeof(struct etnode));
	thisnode->parent = prnt;
	thisnode->start = st;
	thisnode->size = sz;
	treecnt++;
	if (root == NULL) {
		root = thisnode;
		return;			/* father is NULL */
	}
	for (t = root;;) {
		if (t->start == st && t->size == sz && t->parent == prnt) {
			dalloc((char *) thisnode);
			return;
		}
		if (st < t->start) {
			if (t->lc != NULL) {
				depth++;
				t = t->lc;
			}
			else {
				t->lc = thisnode;
				thisnode->father = t;
				if (depth > maxdepth)
					maxdepth = depth;
				return;
			}
		}
		else {		/* st >= t->start */
			if (t->rc != NULL) {
				depth++;
				t = t->rc;
			}
			else {
				t->rc = thisnode;
				thisnode->father = t;
				if (depth > maxdepth)
					maxdepth = depth;
				return;
			}
		}
	}	/* end for */
}


/* VARARGS3 */
bld_ext_ll(head, st, sz, prnt, cnflt)	/* Add an extent to the ordered linked list */
struct	enode	**head;			/* Addr of ptr to start of list */
daddr_t	st;
int	sz;
ino_t	prnt, cnflt;			/* The last param is not usually passed. */
{					/* (Only when the list is "overlap") */
	struct enode *thisnode, *t;

	thisnode = (struct enode *) lalloc(sizeof(struct enode));
	thisnode->parent = prnt;
	if (head == &overlap)
		thisnode->cprnt = cnflt;
	thisnode->start  = st;
	thisnode->size   = sz;

	if ((t = *head) == NULL || thisnode->start < t->start) {
		thisnode->next = t;
		*head = thisnode;
	}
	else {
		while (t->next != NULL && t->next->start <= thisnode->start)
			t = t->next;
		if (t->start == st && t->size == sz && t->parent == prnt) {
			dalloc((char *) thisnode);
		}
		else {
			thisnode->next = t->next;
			t->next = thisnode;
		}
	}
}


bld_EM_ll(node, prnt)		/* Add a node to the list of extent maps */
ino_t	node;
ino_t	prnt;
{
	struct	EMrecno *thisnode, *t;

	thisnode = (struct EMrecno *) lalloc(sizeof(struct EMrecno));
	thisnode->no = node;
	thisnode->parent = prnt;
	if ((t = EMlist) == NULL || thisnode->no < EMlist->no) {
		thisnode->next = EMlist;
		EMlist = thisnode;
	}
	else {
		while (t->next != NULL && t->next->no <= thisnode->no)
			t = t->next;
		if (t->no != node) {
			thisnode->next = t->next;
			t->next = thisnode;
		}
		else {
			if (t->parent != prnt) {
				error("Inodes %d and %d (both corrupt) claim extent map %d\n",
					thisnode->parent, prnt, node);
				error(nofix);
			}
			dalloc((char *) thisnode);
		}
	}
}


/*
 *	Add a node to the list of inodes.  If the count == 1, it can only mean
 *	that I am being called to record an encounter.  Conversely, if
 *	encountered == 1, we have an inode whose link count is other than 1.
 */
bld_linkcnt_ll(lnode, count, encountered)
ino_t	lnode;
int	count;
int	encountered;
{
	struct	link_node *thisnode, *t;

	thisnode = (struct link_node *) lalloc(sizeof(struct link_node));
	thisnode->node = lnode;
	thisnode->r_links = count;
	if ((t = linkptr) == NULL || lnode < linkptr->node) {
		thisnode->next = linkptr;
		thisnode->e_links = encountered;
		linkptr = thisnode;
	}
	else {
		while (t->next != NULL && t->next->node <= thisnode->node)
			t = t->next;
		if (t->node != lnode) {
			thisnode->next = t->next;
			thisnode->e_links = encountered;
			t->next = thisnode;
		}
		else {		/* this happens with multiple links */
			dalloc((char *) thisnode);
			if (count != 1)
				t->r_links = count;
			else
				t->e_links++;
		}
	}
}


bld_FA_ll()		/* Build the linked list of extents */
{			/* that make up the FA file. */
			/* Used only by "get_FArec_off()". */
	int	i;
	ino_t	em;
	struct	FAenode *thisnode;

	/* build the first node */
	if (bread(&dfile, (char *) &glblrec, fa_offset, FARECSZ) == NO) {
		error("Can't read FA inode.\n");
		ESCAPE(2);
	}
	farec = glblrec;
	firstFAenode = thisnode = (struct FAenode *) lalloc(sizeof(struct FAenode));
	thisnode->startrec = 0;
	thisnode->endrec = glblrec.e1.di_extent[0].di_numblk * BLKSZ / FARECSZ - 1;
	thisnode->offset = fa_offset;
	thisnode->next = NULL;
	if (glblrec.e1.di_exnum == 1)
		return;

	/* build nodes 2 thru 4 */
	for (i=1; i<glblrec.e1.di_exnum; i++) {
		thisnode->next = (struct FAenode *) lalloc(sizeof(struct FAenode));
		thisnode->next->startrec = thisnode->endrec + 1;
		thisnode->next->endrec = thisnode->endrec + glblrec.e1.di_extent[i].di_numblk
			* BLKSZ / FARECSZ;
		thisnode = thisnode->next;
		thisnode->offset = glblrec.e1.di_extent[i].di_startblk * BLKSZ;
		thisnode->next = NULL;
	}	/* end for loop */
	if (!VALIDPTR(em = glblrec.e1.di_exmap))
		return;

	/* build all the rest */
	for(;;) {
		/* "em" has to be within the linked list built so far */
		read_FArec(em, (char *) &glblrec);
		for (i=0; i<glblrec.e2.e_exnum; i++) {
			thisnode->next = (struct FAenode *) lalloc(sizeof(struct FAenode));
			thisnode->next->startrec = thisnode->endrec + 1;
			thisnode->next->endrec = thisnode->endrec + glblrec.e2.e_extent[i].e_numblk
				* BLKSZ / FARECSZ;
			thisnode = thisnode->next;
			thisnode->offset = glblrec.e2.e_extent[i].e_startblk * BLKSZ;
			thisnode->next = NULL;
		}	/* end for loop */
		if (!VALIDPTR(em = glblrec.e2.e_next))
			return;
	}	/* end infinite loop */
}


			/***************************************/
			/*				       */
			/* Functions that Retrieve Information */
			/*				       */
			/***************************************/

/*
 *	This routine searches the FA file extents to find where a particular
 *	FA record (inode) is located.  The byte offset from the beginning of
 *	the disc is returned.
 */
dbaddr
get_FArec_off(rec)
ino_t	rec;
{
	struct	FAenode *p = firstFAenode;

	while (rec > p->endrec && p->next != NULL)
		p = p->next;
	if (rec > p->endrec) {
		error("Can't locate inode %d.\n", rec);
		ESCAPE(2);
	}
	return(p->offset + ((rec - p->startrec) * FARECSZ));
}


/*
 *	This routine returns the disc address for the block of the
 *	entry passed in.  It also returns the number of the first
 *	complete entry in the same block plus the size of any
 *	preceeding fragment plus the total number entries
 *	in the directory.
 *
 *	Note that, just like "dirread()", I expect "dinfo->startentry"
 *	to be set to -1 for a new directory.  Usually, this case will
 *	only come via "dirread()".
 */
dbaddr
get_dir_off(entry, dinfo)
int	entry;
struct	dir_info *dinfo;
{
	static	ino_t	lastEM	= 0;
	static	struct	em_rec	EMrec;
	int	i,
		rem,
		blk;

	if (dinfo->startentry == -1) {
		read_FArec(dinfo->inode, (char *) &glblrec);
		dinfo->dirinode = glblrec.e1;
	}
	dinfo->numentries = dinfo->dirinode.di_size / (int) sizeof(struct direct);
	if (dinfo->dirinode.di_exnum == 0)
		return(NULL);
	blk = entry * (int) sizeof(struct direct)/BLKSZ;
	dinfo->startentry = blk * BLKSZ / (int) sizeof(struct direct) + (blk * BLKSZ % (int) sizeof(struct direct) ? 1 : 0);
	dinfo->piece = dinfo->startentry * sizeof(struct direct) - blk * BLKSZ;
	for (i = 0; i < dinfo->dirinode.di_exnum; i++) {
		rem = blk;
		blk -= dinfo->dirinode.di_extent[i].di_numblk;
		if (blk < 0)
			return((dinfo->dirinode.di_extent[i].di_startblk + rem) * BLKSZ);
	}
	if (dinfo->dirinode.di_exmap != lastEM) {
		read_FArec(dinfo->dirinode.di_exmap, (char *) &glblrec);
		lastEM = dinfo->dirinode.di_exmap;
		EMrec = glblrec.e2;
	}
	for (;;) {
		for (i=0; i<EMrec.e_exnum; i++) {
			rem = blk;
			blk -= EMrec.e_extent[i].e_numblk;
			if (blk < 0)
				return((EMrec.e_extent[i].e_startblk + rem) *BLKSZ);
		}
		lastEM = EMrec.e_next;
		read_FArec(lastEM, (char *) &glblrec);
		EMrec = glblrec.e2;
	}
}


struct	etnode *
node0()
{
	struct	etnode	*t;

	t = root;
	while (t->lc != NULL)
		t = t->lc;
	return(t);
}


struct	etnode	*
get_next_node(t)				/* return the "next" (as sorted) */
struct	etnode	*t;				/* node after "t" in the tree */
{
	struct	etnode	*n;

	if (t->rc != NULL) {
		n = t->rc;
		while (n->lc != NULL)
			n = n->lc;
		return(n);
	}
	n = t->father;
	if (n == NULL || t == n->lc)
		return(n);
	while (n->father != NULL && n == n->father->rc)
		n = n->father;
	return(n->father);		/* NULL is returned if t points to the last node */
}


int
get_next_fa_rec(lastrec, ret_set)	/* returns the first set/unset bit */
int	lastrec;			/* following "lastrec" in "famap" */
int	ret_set;
{
	int	*mp, i, set, wordstart;

	lastrec++;
	mp = (int *) famap + lastrec / BITSPERWORD;
	for (i = MODWS(lastrec), wordstart = lastrec - i; (wordstart + i) <= LASTFAREC; i++) {
		set = *mp & BIT(MODWS(i));
		if ((set && ret_set) || (!set && !ret_set))
			return(wordstart + i);
		if (MODWS(i) == (BITSPERWORD - 1))
			mp++;
	}
	return(MAXINT);
}


			/***************************************************/
			/*						   */
			/* Functions that check or manipulate data objects */
			/*						   */
			/***************************************************/

#define	MEMBLK	2000
static char *lasta = NULL;
static char *lastr = NULL;
char *
lalloc(req)			/* local heap allocator */
int	req;
{
	char *p;
	int n;

	if (lastr == NULL) lasta = lastr = (char *) sbrk(0);
	n = req + sizeof(char *) - 1;		/* round to next multiple */
	n &= ~(sizeof(char *) - 1);		/* of the word size */
	while (lasta + n >= lastr) {
		if (sbrk(MEMBLK) == -1)
			errexit("Out of heap space.\n\n");
		lastr += MEMBLK;
	}
	p = lasta;
	lasta += n;
	return(p);
}


dalloc(ret)		/* deallocate some space.  Can only be used */
char	*ret;		/* for a return immediately after a "lalloc" */
{
	char	*p;

	for (p = ret; p < lasta; p++)
		*p = 0;
	lasta = ret;
}


BOOL
mk_dir_entry(entry, dir, dinfo)		/* make an entry in an arbitrary directory */
int	entry;
struct	direct *dir;
struct	dir_info *dinfo;
{
	dbaddr	offset;
	int	i;

	if ((offset = get_dir_off(entry, dinfo)) == NULL)
		return(NO);
	if ((i = dinfo->piece + (entry - dinfo->startentry) * sizeof(struct direct)) +
	    sizeof(struct direct) <= BLKSZ) {
		REQN(bwrite(&dfile, (char *) dir, offset + (dbaddr) i, sizeof(struct direct)));
	}
	else {		/* slow, but not really important */
		REQN(bwrite(&dfile, (char *) dir, offset + (dbaddr) i, BLKSZ - i));
		if ((offset = get_dir_off(entry+1, dinfo)) == NULL)
			return(NO);
		REQN(bwrite(&dfile, (char *) dir + (BLKSZ - i), offset, dinfo->piece));
	}
	return(YES);
}


BOOL
mk_lf_entry(dir)		/* Make an entry in "lost+found". */
struct	direct *dir;
{
	static	struct dir_info lfdinfo;

	if (lfentries >= lfmax) {
		error("%s is full.\n", lfname);
		goodrun = NO;
		/* remove extents from the unclaimed list */
		return(pros_inode(dir->d_ino, RECLAIM, NULL));
	}
	if (!lfwrittento) {
		lfdinfo.inode = lfinode;
		lfdinfo.startentry = -1;
	}
	REQN(mk_dir_entry(lfentries, dir, &lfdinfo));
	lfentries++;
	lfwrittento = YES;
	if (dflag)
		printf("%s written to.\n", lfname);
	/* Reset the link count */
	return(pros_inode(dir->d_ino, RECLAIM, NULL));
}


BOOL
rm_dir_entry(entry, dinfo)	/* remove an entry from a directory */
int	entry;
struct	dir_info *dinfo;
{
	struct	direct dir;

	if (entry < (dinfo->numentries - 1)) {
		/* read the last entry */
		REQN(dirread(dinfo->numentries - 1, &dir, dinfo));
		/* write it over the one to be deleted */
		REQN(mk_dir_entry(entry, &dir, dinfo));
	}
	dinfo->dirinode.di_size -= sizeof(struct direct);
	REQN(bwrite(&dfile, (char *) &dinfo->dirinode,
		get_FArec_off(dinfo->inode), sizeof(struct dinode)));
	dinfo->startentry = -1;		/* force re-read of dir entries */
	return(YES);
}


/*
 *	Check if an extent is in unclaimed space.
 */
BOOL
ckunclm(st, sz)
daddr_t	st;
int	sz;
{
	struct	enode *eptr;

	for (eptr = unclaimed; eptr != NULL && st >= eptr->start; eptr = eptr->next) {
		if ((st + sz) <= (eptr->start + eptr->size) && eptr->parent == UNCLM)
			return(YES);
	}
	return(NO);
}


BOOL
fixunclm(st, sz, prnt)		/* fix up the unclaimed list to show where */
daddr_t	st;			/* restored orphan is located. */
int	sz;
ino_t	prnt;
{
	struct	enode *eptr;

	for (eptr = unclaimed; eptr != NULL; eptr = eptr->next) {
		if (st >= eptr->start && st < eptr->start + eptr->size) {
			if (st + sz > eptr->start + eptr->size)
				break;
			if (st != eptr->start)
				bld_ext_ll(&unclaimed, eptr->start, (int) (st - eptr->start), UNCLM);
			if ((st + sz) != (eptr->start + eptr->size))
				bld_ext_ll(&unclaimed, st + sz, (int) ((eptr->start + eptr->size) - (st + sz)), UNCLM);
			eptr->parent = prnt;
			eptr->start = st;
			eptr->size = sz;
			return(YES);
		}
	}	/* end for */
	error("Unclaimed list is corrupt.  [fixunclm]\n");
	error(nofix);
	goodrun = NO;
	return(NO);
}


/*
 *	Change a FA record type to unused.  This is only used when some
 *	(or all) of the record's extents are in the unclaimed list.  The
 *	record should be changed to "unused" and the IOS will add it
 *	to its internal map of unused records later on.
 *	WARNING: I may destroy "glblrec".
 */
BOOL
addtoFM(fa_node, p, prompt)
ino_t	fa_node;
union	fa_entry *p;
int	prompt;
{
	ino_t	ptr;

	if (p->e1.di_type == R_INODE)		/* inode/FIR */
		ptr = p->e1.di_exmap;
	else if (p->e1.di_type == R_EM)		/* extent map */
		ptr = p->e2.e_next;
	else
		ptr = -1;
	if (!prompt || reply("Shall I add it to the free map") == YES) {
		p->e1.di_type = R_UNUSED;	/* unused */
		REQN(bwrite(&dfile, (char *) p, get_FArec_off(fa_node), FARECSZ));
		if (VALIDPTR(ptr)) {
			read_FArec(ptr, (char *) &glblrec);
			return(addtoFM(ptr, &glblrec, NO));
		}
	}
	else {			/* Can't update the free map because the area */
		fixFM = NO;	/* is still claimed by a corrupt FA record or */
		goodrun = NO;	/* by an unrestored orphan. */
	}
	return(YES);
}



set_bits_in_map(map, start, size)	/* set bits "start" to "start+size-1" */
char	* map;				/* in the bit map stipulated */
int	start;
int	size;
{
	int	*fm, i, j;

	fm = (int *) map + start / BITSPERWORD;
	for (i = j = MODWS(start); i < size + j; i++) {
		*fm |= BIT(MODWS(i));
		if (MODWS(i) == (BITSPERWORD - 1)) fm++;
	}
	if (map == freemap)
		totalfree += size;
}


FArec_validity_ck(recnum, p)
ino_t	recnum;
union	fa_entry *p;
{
	int	bad = NO;
	int	i;
	char	*s;

	if (p->e1.di_type == R_INODE) {
		s = "inode";
		/*
		 * If too many things are bad, maybe I should change this
		 * record to unused.
		 */
		if (p->e1.di_exnum < 0 || p->e1.di_exnum > 4) {
			p->e1.di_exnum = 4;
			bad = YES;
		}
		if (CORRUPTPTR(p->e1.di_exmap)) {
			p->e1.di_exmap = -1;
			bad = YES;
		}
		for (i = 0; i < p->e1.di_exnum; i++) {
			if (p->e1.di_extent[i].di_startblk < 1 ||
			    p->e1.di_extent[i].di_numblk   < 1 ||
			   (p->e1.di_extent[i].di_startblk +
			    p->e1.di_extent[i].di_numblk   > NUMBLKS + 1)) {
				p->e1.di_exnum = (ushort) i;
				bad = YES;
				break;
			}
		}
		/*
		 * A check of total blocks would be appropriate here,
		 * especially if there are extent maps involved.
		 */
	}
	else if (p->e1.di_type == R_EM) {
		s = "extent map";
		if (p->e2.e_exnum < 0 || p->e2.e_exnum > 13) {
			p->e2.e_exnum = 13;
			bad = YES;
		}
		if (CORRUPTPTR(p->e2.e_next)) {
			p->e2.e_next = -1;
			bad = YES;
		}
		if (CORRUPTPTR(p->e2.e_last)) {
			p->e2.e_last = -1;
			bad = YES;
		}
		for (i = 0; i < p->e2.e_exnum; i++) {
			if (p->e2.e_extent[i].e_startblk < 1 ||
			    p->e2.e_extent[i].e_numblk   < 1 ||
			   (p->e2.e_extent[i].e_startblk +
			    p->e2.e_extent[i].e_numblk   > NUMBLKS + 1)) {
				p->e2.e_exnum = (ushort) i;
				bad = YES;
				break;
			}
		}
	}
	if (bad) {
		error("Invalid %s encountered (%d).",s , recnum);
		if (reply("Shall I try to fix it") == NO ||
		    bwrite(&dfile, (char *) p, get_FArec_off(recnum), FARECSZ) == NO)
			ESCAPE(2);
	}
}


			/***************************/
			/*			   */
			/* Miscellaneous Functions */
			/*			   */
			/***************************/


cleanup()
{
	close(dfile.rfdes);
	if (dfile.wfdes != -1) close(dfile.wfdes);
	/*
	 *  WARNING:  the following is erroneous if stdio doesn't use its
	 *            own separate heap.
	 */
	lasta = lastr = NULL;
	if (brk(&end) == -1)
		errexit("failed to return the heap.\n");
}


/* VARARGS0 */
error(s1, s2, s3, s4)
{
	fprintf(stderr, s1, s2, s3, s4);
}


/* VARARGS0 */
errexit(s1, s2, s3, s4)
{
	error("%s: ", myname);
	error(s1, s2, s3, s4);
	cleanup();
	unlink(SDF_LOCK);
	exit(8);
}


recover()
{
	if (escapecode == 2)
		error("Unable to continue.\n\n");
}
