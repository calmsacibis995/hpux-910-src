static char *HPUX_ID = "@(#) $Revision: 70.3 $";

/*
 * ncheck -- obtain file names from reading filesystem
 *
 *   no argument:     generates a path name vs. i-number list of all files
 *   -i               report only those files whose i-number follow
 *   -a               allows printing of the names . and ..
 *   -s               reduces to report to special files and files with 
 *                    set-user-ID mode.
 * This is UCB 4.2 code but supports S5 arguments.  The original 4.2 code
 * has -m option which is not documented in the 4.2 ncheck(1M) manual page
 * Since S5 ncheck(1M) does not support this -m option either, so I took out 
 * the code which relates to this option.
 * 
 */

#define	NB		300         /* s5.2 constants */
#ifdef small
#define HSIZE		1740
#else
#define HSIZE		65537	/* max number of directories per fs */
#endif
#define MAXDIRSTR	(30 * HSIZE) /* space for directory names */

#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))

#include <sys/types.h>
#include <sys/param.h>
#ifdef HP_NFS
#include <time.h>
#include <sys/vnode.h>
#endif HP_NFS
#include <sys/inode.h>
#include <sys/ino.h>
#include <sys/fs.h>
#include <sys/dir.h>
#include <stdio.h>
#include <checklist.h>
#ifdef TRUX
#include <sys/security.h>
#endif
#if defined(DUX) || defined(DISKLESS)
#include <sys/stat.h>
#endif /* defined(DUX) || defined(DISKLESS) */

char *strchr();
char line[BUFSIZ];

struct	fs  *sblock;
char sbuf[SBSIZE];           

#if defined(TRUX) || !defined(SecureWare) || !defined(B1)
struct  dinode  itab[MAXIPG];
#endif
struct 	dinode	*gip;
typedef struct ilist {
	ino_t	ino;
	u_short	mode;
	short	uid;
	short	gid;
	struct 	ilist *next;
} ILIST;
ILIST	*ilist;
struct	htab
{
	ino_t	h_ino;
	ino_t	h_pino;
#if defined(DISKLESS) || defined(DUX)
	u_short	h_mode;
#endif /* defined(DISKLESS) || defined(DUX) */
	char	*h_name;
} htab[HSIZE];
char strngtab[MAXDIRSTR];
int strngloc;

struct dirstuff {
	int loc;
	struct dinode *ip;
	char dbuf[MAXBSIZE];
};

int	aflg;
int	sflg;
int	iflg; /* number of inodes being searched for */
int	fi;
ino_t	ino;
int	nhent;
ILIST	*nxfile;
ILIST	*cmdline;
ILIST	*NewIlist();

int	nerror;
daddr_t	bmap();
long	atol();
struct htab *lookup();
FILE *mntf;
struct mntent *mntptr;
long super;

main(argc, argv)
	int argc;
	char *argv[];
{
	register i;
	long n;

#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
            if (!authorized_user("backup"))  {
		fprintf(stderr,
			 "ncheck: you must have the 'backup' authorization\n");
                exit(1);
            }
	}
#endif
	super = SBLOCK;
	ilist   = NewIlist();
	cmdline = ilist;
	while (--argc) {
		argv++;
		if (**argv=='-')
		switch ((*argv)[1]) {

		case 'a':
			aflg++;
			continue;

		case 'i':
			while (argc > 1)
			{
				n = (argc > 1) ? atol(argv[1]): 0;
				if(n == 0)
					break;
				cmdline->ino  = n;
				cmdline->next = NewIlist();
				cmdline = cmdline->next;
				iflg++;
				argv++;
				argc--;
			}
			continue;

		case 's':
			sflg++;
			continue;

		default:
			fprintf(stderr, "ncheck: bad flag %c\n", (*argv)[1]);
			nerror++;
		}
		else   
			break;
	}
	nxfile = cmdline;
	if (argc)
		while(argc--)
			check(*argv++);
	else {
	        if ((mntf = setmntent(MNT_CHECKLIST, "r")) == NULL)
		{
	            fprintf(stderr, "ncheck: Couldn't open %s\n", MNT_CHECKLIST);
		    nerror++;
		}

		while ( (mntptr = getmntent(mntf)) != (struct mntent *)NULL )
		    if (strcmp(mntptr->mnt_type, MNTTYPE_HFS) == 0)
			check( mntptr->mnt_fsname );

		endmntent(mntf);
	}
	return(nerror);
}

check(file)
	char *file;
{
	register int i, j, c;
	int nfiles;
	long dblk;
	ILIST *p, *p1;
	
	fi = open(file, 0);
	if(fi < 0) {
		fprintf(stderr, "ncheck: cannot open %s\n", file);
		nerror++;
		return;
	}
	nhent = 0;
	sync();
	bread(super,sbuf, SBSIZE);
	sblock = ((struct fs *)sbuf);
#if defined(SecureWare) && defined(B1)
	if(ISB1)
            disk_set_file_system (sblock, sblock->fs_bsize);
#endif	

#if defined(FD_FSMAGIC)
	if ((sblock->fs_magic!=FS_MAGIC) && (sblock->fs_magic!=FS_MAGIC_LFN)
		&& (sblock->fs_magic != FD_FSMAGIC)) {
#else /* not new magic number */
	if ((sblock->fs_magic!=FS_MAGIC) && (sblock->fs_magic!=FS_MAGIC_LFN)) {
#endif /* new magic number */
		printf("%s: not a file system\n", file);
		nerror++;
		return;
	}
	ino = 0;
	for (c = 0; c < sblock->fs_ncg; c++) {
#if defined(SecureWare) && defined(B1)
		if(ISB1){
                    int pass1();
                    ncheck_cyl_group (sblock, c, pass1);
		}
		else{
		    dblk = fsbtodb(sblock, cgimin(sblock,c));
		    bread(dblk, (char *)itab, (sblock->fs_ipg* sizeof(struct dinode)));
		    for(j = 0; j < sblock->fs_ipg; j++) {
			if (itab[j].di_mode != 0)
				pass1(&itab[j]);
			ino++;
		    }
		}
#else
		dblk = fsbtodb(sblock, cgimin(sblock,c));
		bread(dblk, (char *)itab, (sblock->fs_ipg* sizeof(struct dinode)));
		for(j = 0; j < sblock->fs_ipg; j++) {
			if (itab[j].di_mode != 0)
				pass1(&itab[j]);
			ino++;
		}
#endif  	
	}
	ino = 0;
	for (c = 0; c < sblock->fs_ncg; c++) {
#if defined(SecureWare) && defined(B1)
	    if(ISB1){
                int pass2();
                ncheck_cyl_group (sblock, c, pass2);
	    }
	    else{
		bread(fsbtodb(sblock, cgimin(sblock, c)), (char *)itab,
		    sblock->fs_ipg * sizeof (struct dinode));
		for(j = 0; j < sblock->fs_ipg; j++) {
			if (itab[j].di_mode != 0)
				pass2(&itab[j]);
			ino++;
		}
	    }
#else
		bread(fsbtodb(sblock, cgimin(sblock, c)), (char *)itab,
		    sblock->fs_ipg * sizeof (struct dinode));
		for(j = 0; j < sblock->fs_ipg; j++) {
			if (itab[j].di_mode != 0)
				pass2(&itab[j]);
			ino++;
		}
#endif
	}
	ino = 0;
	for (c = 0; c < sblock->fs_ncg; c++) {
#if defined(SecureWare) && defined(B1)
	    if(ISB1){
                int pass3();
                ncheck_cyl_group (sblock, c, pass3);
	    }
	    else{
		bread(fsbtodb(sblock, cgimin(sblock, c)), (char *)itab,
		    sblock->fs_ipg * sizeof (struct dinode));
		for(j = 0; j < sblock->fs_ipg; j++) {
			if (itab[j].di_mode != 0)
				pass3(&itab[j]);
			ino++;
		}
	   }
#else
		bread(fsbtodb(sblock, cgimin(sblock, c)), (char *)itab,
		    sblock->fs_ipg * sizeof (struct dinode));
		for(j = 0; j < sblock->fs_ipg; j++) {
			if (itab[j].di_mode != 0)
				pass3(&itab[j]);
			ino++;
		}
#endif
	}
	close(fi);
	for (i = 0; i < HSIZE; i++)
		htab[i].h_ino = 0;
	p = cmdline->next;
	while (p)
	{
		p1 = p->next;
		free(p);
		p = p1;
	}
	nxfile = cmdline;
}

pass1(ip)
	register struct dinode *ip;
{
	int i;
#if defined(DISKLESS) || defined(DUX)
	register struct htab *hp;
#endif /* defined(DISKLESS) || defined(DUX) */

	if ((ip->di_mode & IFMT) != IFDIR) {
		if (sflg==0)
			return;
		if ((ip->di_mode&IFMT)==IFBLK || (ip->di_mode&IFMT)==IFCHR
		  || ip->di_mode&(ISUID|ISGID)) {
			nxfile->ino  = ino;
			nxfile->mode = ip->di_mode;
			nxfile->uid  = ip->di_uid;
			nxfile->gid  = ip->di_gid;
			nxfile->next = NewIlist();
			nxfile = nxfile->next;
			return;
		}
	}
/*
 * In a DUX environment, we need to save the mode so that pname()
 * can decide later whether or not to append a '+' to a given path
 * component.
 */
#if defined(DISKLESS) || defined(DUX)
	if ((hp = lookup(ino, 1)))
	    hp->h_mode = ip->di_mode;
#else	
	lookup(ino, 1);
#endif /* defined(DISKLESS) || defined(DUX) */
}

pass2(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	struct htab *hp;

	if((ip->di_mode&IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for (dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(dp->d_ino == 0)
			continue;
		hp = lookup(dp->d_ino, 0);
		if(hp == 0)
			continue;

		if(dotname(dp))
			continue;

		hp->h_pino = ino;
		hp->h_name = &strngtab[strngloc];

		strngloc += strlen(dp->d_name) + 1;
		if (strngloc>=MAXDIRSTR) {
			fprintf(stderr,
			  "ncheck: out of core-- increase MAXDIRSTR\n");
			exit(1);
		}
		strcpy(hp->h_name, dp->d_name);
	}
}

pass3(ip)
	register struct dinode *ip;
{
	register struct direct *dp;
	struct dirstuff dirp;
	int k;
	ILIST *p;

	if((ip->di_mode&IFMT) != IFDIR)
		return;
	dirp.loc = 0;
	dirp.ip = ip;
	gip = ip;
	for(dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		if(aflg==0 && dotname(dp))
			continue;
		if(sflg == 0 && iflg == 0)
			goto pr;
		for(p = ilist; p->next != (ILIST *)0; p = p->next)
			if(p->ino == dp->d_ino)
				break;
		if (p->ino == 0)
			continue;
	pr:
		printf("%-5u\t", dp->d_ino);
		pname(ino, 0);
		printf("/%s", dp->d_name);
		if (lookup(dp->d_ino, 0))
			printf("/.");
		printf("\n");
	}
}

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register struct dirstuff *dirp;
{
	register struct direct *dp;
	daddr_t lbn, d;

	for(;;) {
		if (dirp->loc >= dirp->ip->di_size)
			return NULL;
		if ((lbn = lblkno(sblock, dirp->loc)) == 0) {
			d = bmap(lbn);
			if(d == 0)
				return NULL;
			bread(fsbtodb(sblock, d), dirp->dbuf,
			    dblksize(sblock, dirp->ip, lbn));
		}
		dp = (struct direct *)
		    (dirp->dbuf + blkoff(sblock, dirp->loc));
		dirp->loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

dotname(dp)
	register struct direct *dp;
{

	if (dp->d_name[0]=='.')
		if (dp->d_name[1]==0 ||
		   (dp->d_name[1]=='.' && dp->d_name[2]==0))
			return(1);
	return(0);
}

pname(i, lev)
	ino_t i;
	int lev;
{
	register struct htab *hp;

	if (i==ROOTINO)
		return;
	if ((hp = lookup(i, 0)) == 0) {
		printf("???");
		return;
	}
	if (lev > 10) {
		printf("...");
		return;
	}
	pname(hp->h_pino, ++lev);
#if defined(DISKLESS) || defined(DUX)
	if (hp->h_mode & S_CDF)
	    printf("/%s+", hp->h_name);
	else
#endif /* defined(DISKLESS) || defined(DUX) */
	printf("/%s", hp->h_name);
}

struct htab *
lookup(i, ef)
	ino_t i;
	int ef;
{
	register struct htab *hp;

	for (hp = &htab[i%HSIZE]; hp->h_ino;) {
		if (hp->h_ino==i)
			return(hp);
		if (++hp >= &htab[HSIZE])
			hp = htab;
	}
	if (ef==0)
		return(0);
	if (++nhent >= HSIZE) {
		fprintf(stderr, "ncheck: out of core-- increase HSIZE\n");
		exit(1);
	}
	hp->h_ino = i;
	return(hp);
}

bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
	int cnt;
{
	register i;
	lseek(fi, (long)dbtob(bno), 0);
	if (read(fi, buf, cnt) != cnt) {
		fprintf(stderr, "ncheck: read error %d\n", bno);
		for(i=0; i < cnt; i++)
			buf[i] = 0;
	}
}

daddr_t
bmap(i)
	int i;
{
	daddr_t ibuf[MAXNINDIR];

	if(i < NDADDR)
		return(gip->di_db[i]);
	i -= NDADDR;
	if(i > NINDIR(sblock)) {
		fprintf(stderr, "ncheck: %u - huge directory\n", ino);
		return((daddr_t)0);
	}
	bread(fsbtodb(sblock, gip->di_ib[i]), (char *)ibuf, sizeof(ibuf));
	return(ibuf[i]);
}


/* Allocate a new ILIST element and exit if out of memory
 */

ILIST *
NewIlist()
{
	ILIST	*p;

	if ((p = (ILIST *) malloc (sizeof (ILIST))) == (ILIST *)0)
	{
		fprintf (stderr, "ncheck: can't allocate space to hold inode entry\n");
		exit(1);
	}
	
	p->ino  = 0;
	p->mode = 0;
	p->uid  = 0;
	p->gid  = 0;
	p->next = (ILIST *)0;
	
	return p;
}

