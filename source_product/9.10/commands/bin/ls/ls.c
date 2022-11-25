static char *HPUX_ID = "@(#) $Revision: 70.2 $";

/*
* 	list file or directory;
* 	define DOTSUP to suppress listing of files beginning with dot
*/

#include <nl_ctype.h>

#ifdef NLS
#define MAXLNAME 14	/* maximum length of a language name */
#define NL_SETN 1	/* message set number */
#include <nl_types.h>
#include <locale.h>
nl_catd nlmsg_fd;
nl_catd nlmsg_tfd;
#else
#define catgets(i, sn,mn,s) (s)
#endif /* NLS */

#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>

#if defined(SecureWare) && defined(B1)
#include <sys/errno.h>
#include <sys/security.h>
#endif

char 	*strrchr();


#include	<sys/stat.h>
#include	<ndir.h>
#include	<stdio.h>
#include	<pwd.h>
#include	<grp.h>
#if defined(DUX) || defined(DISKLESS)
#ifndef SYMLINKS
#   define SYMLINKS		/* Make sure SYMLINKS is defined. */
#endif /* SYMLINKS */
#include	<cluster.h>
#endif /* defined(DUX) || defined(DISKLESS) */

#if u3b
#include	<sys/macro.h>
#endif

#ifndef STANDALONE
#define TERMINFO
#endif

/* -DNOTERMINFO can be defined on the cc command line to prevent
 * the use of terminfo.  This should be done on systems not having
 * the terminfo feature (pre 6.0 sytems ?).
 * As a result, columnar listings assume 80 columns for output,
 * unless told otherwise via the COLUMNS environment variable.
 */
#ifdef NOTERMINFO
#undef TERMINFO
#endif

#ifdef TERMINFO
#include	<curses.h>
#include	<term.h>
#endif

/*  #define	DOTSUP	1 */
#define ISARG   0100000 /* this bit equals 1 in lflags of structure lbuf
                        *  if the file the lbuf describes was a command
			*  line arguement.
                        */
#define DIRECT	10	/* Number of direct blocks */


/*
 *  The conditional compilation variable RFA is used to control the
 *  addition of code needed to have ls work with a hp-ux system that
 *  contains RFA.
 *  The code added does three things:
 *     (1)  Prints a 'n' in the mode field of a ls -l listing
 *     (2)  Allows the listing of the remote directory when a ls
 *          /net/remote is done with having to add a /. on the end of
 *          the network special file
 *     (3)  Prevents ls from decending more than one level is a remote
 *          file system when a ls -R is done.
 *  It has been added by Mike Shipley at CNO.
 */


#ifdef RFA
/* Number of levels allowed of network file recursion  */
#define 	MAX_R_LIMIT	1
#endif /* RFA */

#ifdef ACLS
#define OPACL	'+'	/* optional ACL entries on file */
#define NOACL	' '	/* NO optional entries on file  */
#endif /* ACLS */

struct	lbuf	{
	char	*lnamep;        /* file name */
#ifdef SYMLINKS
	char	*llinkto;	/* Symbolic link value. */
#endif /* SYMLINKS */
	char	ltype;  	/* filetype */
  	ino_t	lnum;           /* inode number of file */
	short	lflags; 	/* 0777 bits used as r,w,x permissions */
#ifdef ACLS
	char	lacl; 		/* indicator of optional ACL entries on file */
	short	allmodes;	/* optional ACL entries -result of getaccess */
				/* as root.    No optional entries - 0  */
#endif /* ACLS */
	short	lnl;    	/* number of links to file */
	unsigned short	luid;
	unsigned short	lgid;
	long	lsize;  	/* filesize or major/minor dev numbers */
#if defined(DUX) || defined(CNODE_DEV)
	cnode_t lcnode;		/* Cnode of the device (Generic/Specific). */
#endif /* defined(DUX) || defined(CNODE_DEV) */
#ifdef HFS
	long	lblocks;	/* real number of blocks (from stat struct) */
#endif
	long	lmtime;

#ifdef RFA
	int	r_level;        /* Network recursion level             */
#endif /* RFA */

};

struct dchain {
	char *dc_name;		/* path name */
	struct dchain *dc_next;	/* next directory in the chain */
	char	ltype;		/* file type code              */
#ifdef RFA
	int	r_level;	/* the network resursion level */
#endif /* RFA */

};


/* The following is used for passwd/group cache routines.  See
   the routines initcaches(), pwname() and grname(). */

#define PWCACHESIZE	128	/* Must be power of 2 */
#define PWCACHEKEY(n)	((n)&(PWCACHESIZE-1))
#define GRCACHESIZE	32	/* Must be power of 2 */
#define GRCACHEKEY(n)	((n)&(GRCACHESIZE-1))
#if defined(DUX) || defined(DISKLESS)
#define CNCACHESIZE	32	/* Must be power of 2 */
#define CNCACHEKEY(n)	((n)&(CNCACHESIZE-1))
#endif /* defined(DUX) || defined(DISKLESS) */

struct idcache
{
    int id;
    char *name;
} ;


struct idcache pwcache[PWCACHESIZE];
struct idcache grcache[GRCACHESIZE];
#if defined(DUX) || defined(DISKLESS)
struct idcache cncache[CNCACHESIZE];
#endif /* defined(DUX) || defined(DISKLESS) */

/* End of stuff for passswd/group caches. */


struct dchain *dfirst;	/* start of the dir chain */
struct dchain *cdfirst;	/* start of the durrent dir chain */
struct dchain *dtemp;	/* temporary - used for linking */
char *curdir;		/* the current directory */

int	nfiles = 0;	/* number of flist entries in current use */
int	nargs = 0;	/* number of flist entries used for arguments */
int	maxfils = 0;	/* number of flist/lbuf entries allocated */
int	maxn = 0;	/* number of flist entries with lbufs assigned */
int	quantn = 1024;	/* allocation growth quantum */

struct	lbuf	*nxtlbf;	/* pointer to next lbuf to be assigned */
struct	lbuf	**flist;	/* pointer to list of lbuf pointers */
struct	lbuf	*gstat();

DIR	*dirp;

int	aflg, bflg, cflg, dflg, fflg, gflg, iflg, lflg, mflg;
int	nflg, oflg, pflg, qflg, sflg, tflg, uflg, xflg;
int	Aflg, Cflg, Fflg, Rflg;
#if defined(DUX) || defined(DISKLESS)
int	Hflg;
#endif /* defined(DUX) || defined(DISKLESS) */
#ifdef SYMLINKS
int	Lflg;
#endif /* SYMLINKS */
int	rflg = 1;   /* initialized to 1 for special use in compar() */
int	flags;
int	err = 0;	/* Contains return code */

char	*dmark;	/* Used if -p option active. Contains "/" or NULL. */

unsigned	lastuid	= -1, lastgid = -1;
int	statreq;    /* is > 0 if any of sflg, (n)lflg, tflg are on */

char	*dotp = ".";
char	*makename();
char	*savestr();
char	*grbuf, *pwbuf;
#if defined(DUX) || defined(DISKLESS)
char	*cnbuf;
#endif /* defined(DUX) || defined(DISKLESS) */
char	*ctime();
char	*malloc();

int	stat();

#ifdef SYMLINKS
int	lstat();
#endif /* SYMLINKS */

long	nblock();
long	tblocks;  /* total number of blocks of files in a directory */
long	sixmonthsago, onehourahead;

int	num_cols = 80;
int	colwidth;
int	filewidth;
int	fixedwidth;
int	curcol;
int	compar();

main(argc, argv)
int argc;
char *argv[];
{
#define	EQ(x,y)		!strcmp(x,y)

	extern char	*optarg;
	extern int	optind;
	int		amino, opterr=0;
	int		c;
	register struct lbuf *ep;
	struct	lbuf	lb;
	int		i, width;
	long		time();
	void 		qsort(), exit();
	char		*cmd;
	char		*getenv();
#if defined NLS16 && defined EUC
	int		ch, count;
	char		*p;
#endif

#ifdef NLS || NLS16			/* initialize to the current locale */
	unsigned char lctime[5+4*MAXLNAME+4], *pc;
	unsigned char savelang[5+MAXLNAME+1];

	if (!setlocale(LC_ALL, "")) {		/* setlocale fails */
		fputs(_errlocale("ls"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)-1;		/* use default messages */
		nlmsg_tfd = (nl_catd)-1;
	} else {				/* setlocale succeeds */
		char *s;

		if (((s=getenv("LANG")) && *s)||((s=getenv("NLSPATH")) && *s))
			nlmsg_fd = catopen("ls", 0);    /* use $LANG messages */
		else
			nlmsg_fd = (nl_catd)-1;
		strcpy(lctime, "LANG=");	/* $LC_TIME affects some msgs */
		strcat(lctime, getenv("LC_TIME"));
		if (lctime[5] != '\0') {	/* if $LC_TIME is set */
			extern char *strchr();

			strcpy(savelang, "LANG=");	/* save $LANG */
			strcat(savelang, getenv("LANG"));
			if ((pc = (unsigned char *)strchr(lctime, '@')) != NULL) /*if modifier*/
				*pc = '\0';	/* remove modifer part */
			putenv(lctime);		/* use $LC_TIME for some msgs */
			nlmsg_tfd = catopen("ls", 0);
			putenv(savelang);	/* reset $LANG */
		} else				/* $LC_TIME is not set */
			nlmsg_tfd = nlmsg_fd;	/* use $LANG messages */
	}
#endif /* NLS || NLS16 */

#ifdef STANDALONE
	if (argv[0][0] == '\0')
		argc = getargv("ls", &argv, 0);
#endif

	lb.lmtime = time((long *) NULL);
	sixmonthsago = lb.lmtime - 6L*30L*24L*60L*60L;	/* 6 months ago */
	onehourahead = lb.lmtime + 60L*60L;		/* 1 hour ahead. */
	Aflg = getuid() == 0;
	if	(isatty(1)) {
		Cflg = 1;
	}
	else {
		Cflg = 0;
	}
	mflg = 0;

	if (cmd = strrchr(argv[0], '/'))
		++cmd;
	else
		cmd = argv[0];
	if (EQ(cmd,"ls"))
		; /* skip over the unnecessary checks */
	else if (EQ(cmd,"l"))  {
		Cflg = 0;
		mflg = 1;
	}
	else if (EQ(cmd,"ll")) {
		lflg++;
		statreq++;
	}
	else if (EQ(cmd,"lsf")) {
		Fflg++;
		statreq++;
	}
	else if (EQ(cmd,"lsr")) {
		Rflg++;
		statreq++;
	}
	else if (EQ(cmd,"lsx")) {
		xflg = 1;
		Cflg = 1;
		mflg = 0;
	}
#if defined(DUX) || defined(DISKLESS)
	statreq++;
#endif /* defined(DUX) || defined(DISKLESS) */

	while ((c=getopt(argc, argv,
#if defined(DUX) || defined(DISKLESS)
			"1ARadCxmnlogrtucpFHLbqisf")) != EOF) switch(c)
#else /* not (defined(DUX) || defined(DISKLESS)) */
#ifdef SYMLINKS
			"1ARadCxmnlogrtucpFLbqisf")) != EOF) switch(c)
#else
			"1ARadCxmnlogrtucpFbqisf")) != EOF) switch(c)
#endif /* not SYMLINKS */
#endif /* not (defined(DUX) || defined(DISKLESS)) */
	 {
		/*
		 * 1 - force 1/line in output
		 */
		case '1':
			Cflg = 0;
			xflg = 0;	/* POSIX.2 */
			mflg = 0;	/* POSIX.2 */
			continue;

		/* STANDARD FLAGS */
		case 'A':
			Aflg = !Aflg;
			continue;
		case 'R':
			Rflg++;
			statreq++;
			continue;
		case 'a':
			aflg++;
			continue;
		case 'd':
			dflg++;
			continue;
		case 'C':
			Cflg = 1;
			lflg = 0; 	/* POSIX.2 */
			mflg = 0;
			continue;
		case 'x':
			xflg = 1;
			Cflg = 1;
			lflg = 0; 	/* POSIX.2 */
			mflg = 0;
			continue;
		case 'm':
			Cflg = 0;
			mflg = 1;
			lflg = 0; 	/* POSIX.2 */
			continue;
		case 'n':
			nflg++;
		case 'l':
			lflg++;
			Cflg = 0; 	/* POSIX.2 */
			xflg = 0;	/* POSIX.2 */
			mflg = 0;	/* POSIX.2 */
			statreq++;
			continue;
		case 'o':
			oflg++;
			lflg++;
			statreq++;
			continue;
		case 'g':
			gflg++;
			lflg++;
			statreq++;
			continue;
		case 'r':
			rflg = -1;
			continue;
		case 't':
			tflg++;
			statreq++;
			continue;
		case 'u':
			uflg++;
			cflg = 0; /* POSIX.2 */
			continue;
		case 'c':
			cflg++;
			uflg = 0; /* POSIX.2 */
			continue;
		case 'p':
			pflg++;
			statreq++;
			continue;
		case 'F':
			Fflg++;
			statreq++;
			continue;
#if defined(DUX) || defined(DISKLESS)
		case 'H':
			Hflg++;
			Fflg++;
			statreq++;
			continue;
#endif /* defined(DUX) || defined(DISKLESS) */
#ifdef SYMLINKS
		case 'L':
			Lflg++;
			continue;
#endif /* SYMLINKS */
		case 'b':
			bflg = 1;
			qflg = 0;
			continue;
		case 'q':
			qflg = 1;
			bflg = 0;
			continue;
		case 'i':
			iflg++;
#if defined(DUX) || defined(DISKLESS)
			statreq++;
#endif /* defined(DUX) || defined(DISKLESS) */
			continue;
		case 's':
			sflg++;
			statreq++;
			continue;
		case 'f':
			fflg++;
			continue;
		case '?':
			opterr++;
			continue;
		}
	if(opterr) {
#if defined(DUX) || defined(DISKLESS)
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1, "usage: ls -1ARadCxmnlogrtucpFHLbqisf [files]\n")));
#else /* not (defined(DUX) || defined(DISKLESS)) */
#ifdef SYMLINKS
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,2, "usage: ls -1ARadCxmnlogrtucpFLbqisf [files]\n")));
#else /* not SYMLINKS */
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,3, "usage: ls -1ARadCxmnlogrtucpFbqisf [files]\n")));
#endif /* not SYMLINKS */
#endif /* not (defined(DUX) || defined(DISKLESS)) */
		exit(2);
	}

	if (fflg) {
		aflg++;
		lflg = 0;
		sflg = 0;
		tflg = 0;
		statreq = 0;
	}

	fixedwidth = 2;
	if (pflg || Fflg)
		fixedwidth++;
	if (iflg)
		fixedwidth += 7;
	if (sflg)
		fixedwidth += 5;

	if (lflg) {				/* This is the way  */
		if (!gflg && !oflg)		/* 5.0 behaved, but */
			gflg = oflg = 1;	/* it may be open   */
		else				/* to interpretation*/
		if (gflg && oflg)
			gflg = oflg = 0;
		Cflg = mflg = 0;
		if ((oflg > 0) || (gflg > 0))
		{
		    initcaches();
		}
	}

	if (Cflg || mflg) {
		char *clptr;
		if ((clptr = getenv("COLUMNS")) != NULL)
			num_cols = atoi(clptr);
#ifdef TERMINFO
		else {
			setupterm(0,1,&i); /* get term description */
			resetterm();	/* undo what setupterm changed */
			if (i == 1)
				num_cols = columns;
		}
#endif
		if (num_cols < 20 || num_cols > 160)
			/* assume it is an error */
			num_cols = 80;
	}

	/* allocate space for flist and the associated	*/
	/* data structures (lbufs)			*/
	maxfils = quantn;
	if((flist=(struct lbuf **)malloc((unsigned)(maxfils * sizeof(struct lbuf *)))) == NULL
	|| (nxtlbf = (struct lbuf *)malloc((unsigned)(quantn * sizeof(struct lbuf)))) == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "ls: out of memory\n")));
		exit(2);
	}
	if ((amino=(argc-optind))==0) { /* case when no names are given
					* in ls-command and current
					* directory is to be used
 					*/
		argv[optind] = dotp;
	}
	for (i=0; i < (amino ? amino : 1); i++) {
		if (Cflg || mflg) {
			width = strlen(argv[optind]);
#if defined NLS16 && defined EUC
			p = argv[optind];
			for (count = width; count > 0; count--) {
				ch = *p++;
				ch = ch & 0377;
				if (FIRSTof2(ch) && (C_COLWIDTH(ch) == 1))
					width--;
			}
#endif
			if (width > filewidth)
				filewidth = width;
		}
		if ((ep = gstat((*argv[optind] ? argv[optind] : dotp), 1))==NULL)
		{
			err = 2;
			optind++;
			continue;
		}
		ep->lnamep = (*argv[optind] ? argv[optind] : dotp);
		ep->lflags |= ISARG;
		optind++;
		nargs++;	/* count good arguments stored in flist */
	}
	colwidth = fixedwidth + filewidth;
	qsort(flist, (unsigned)nargs, sizeof(struct lbuf *), compar);

	for (i=0; i<nargs; i++) {
#if defined(DUX) || defined(DISKLESS)
		if ((flist[i]->ltype=='d' || flist[i]->ltype=='n' ||
		     flist[i]->ltype=='H') && dflg==0 || fflg)
			break;
#else /* not defined(DUX) || defined(DISKLESS) */
#ifdef RFA
		if ((flist[i]->ltype=='d' || flist[i]->ltype=='n')
		     && dflg==0 || fflg)
			break;
#else
		if (flist[i]->ltype=='d' && dflg==0 || fflg)
			break;
#endif /* RFA */
#endif /* not defined(DUX) || defined(DISKLESS) */
	}

	pem(&flist[0],&flist[i], 0);
	for (; i<nargs; i++) {

#ifdef RFA
		pdirectory(flist[i]->lnamep, (amino>1), nargs,
			   flist[i]->ltype, flist[i]->r_level);
#else
		pdirectory(flist[i]->lnamep, (amino>1), nargs,
			   flist[i]->ltype);

#endif /* RFA */

		/* -R: print subdirectories found */
		while (dfirst || cdfirst) {
			/* Place direct subdirs on front in right order */
			while (cdfirst) {
				/* reverse cdfirst onto front of dfirst */
				dtemp = cdfirst;
				cdfirst = cdfirst -> dc_next;
				dtemp -> dc_next = dfirst;
				dfirst = dtemp;
			}
			/* take off first dir on dfirst & print it */
			dtemp = dfirst;
			dfirst = dfirst->dc_next;

#ifdef RFA
			pdirectory (dtemp->dc_name, 1, nargs,
		                    dtemp->ltype, dtemp->r_level);
#else
			pdirectory (dtemp->dc_name, 1, nargs,
		                    dtemp->ltype);

#endif /* RFA */
			free (dtemp->dc_name);
			free ((char *)dtemp);
		}
	}
	exit(err);
}

/*
 * pdirectory: print the directory name, labelling it if title is
 * nonzero, using lp as the place to start reading in the dir.
 */

#ifdef RFA
pdirectory (name, title, lp, ltype, r_level)
char *name;
int title;
int lp;
char ltype;
int  r_level;
#else /* not RFA */
pdirectory (name, title, lp, ltype)
char *name;
int title;
int lp;
char ltype;
#endif /* RFA */
{
	register struct dchain *dp;
	register struct lbuf *ap;
	register int j;

#if defined(DUX) || defined(DISKLESS)
	static char hbuf[MAXPATHLEN+2];

	/* Append + to hidden directories. */
	if (ltype == 'H')
	{
	    (void) strcpy(hbuf, name);
	    (void) strcat(hbuf, "+");
	    name = hbuf;
	}
#endif /* defined(DUX) || defined(DISKLESS) */

	filewidth = 0;
	curdir = name;
	if (title) {
		putc('\n', stdout);
		pprintf(name, ":");
		new_line();
	}
	nfiles = lp;

	readDIR(name, ltype);

	if (fflg==0)
		qsort(&flist[lp],(unsigned)(nfiles - lp),sizeof(struct lbuf *),compar);
	if (Rflg) for (j = nfiles - 1; j >= lp; j--) {
		ap = flist[j];


#if defined(DUX) || defined(DISKLESS)
#ifdef RFA
		if ((ap->ltype == 'd' || ap->ltype == 'n' || ap->ltype == 'H')
		      && strcmp(ap->lnamep, ".") != 0
		      && strcmp(ap->lnamep, "..") != 0
		      && r_level + ap->r_level <= MAX_R_LIMIT)
#else
		if ((ap->ltype == 'd' || ap->ltype == 'H')
		      && strcmp(ap->lnamep, ".") != 0
		      && strcmp(ap->lnamep, "..") != 0)
#endif /* RFA */
#else /* not defined(DUX) || defined(DISKLESS) */
#ifdef RFA
		if ((ap->ltype == 'd' || ap->ltype == 'n')
		      && strcmp(ap->lnamep, ".") != 0
		      && strcmp(ap->lnamep, "..") != 0
		      && r_level + ap->r_level <= MAX_R_LIMIT)
#else
		if (ap->ltype == 'd'
                      && strcmp(ap->lnamep, ".") != 0
		      && strcmp(ap->lnamep, "..") != 0)
#endif /* RFA */
#endif /* not defined(DUX) || defined(DISKLESS) */

		     {
			dp = (struct dchain *)calloc(1,sizeof(struct dchain));
			if (dp == NULL)
				fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,4, "ls: out of memory\n")));
			dp->dc_name = savestr(makename(curdir, ap->lnamep));

			dp-> ltype = ap->ltype;
#ifdef RFA
			dp->r_level = ap->r_level + r_level;
#endif /* RFA */

			dp -> dc_next = dfirst;
			dfirst = dp;
		    }
	}
	if (lflg || sflg)
		curcol += printf((catgets(nlmsg_fd,NL_SETN,7, "total %ld")), tblocks);
	pem(&flist[lp],&flist[nfiles],lflg||sflg);
}

/*
 * pem: print 'em.  Print a list of files (e.g. a directory) bounded
 * by slp and lp.
 */
pem(slp, lp, tot_flag)
	register struct lbuf **slp, **lp;
	int tot_flag;
{
	int ncols, nrows, row, col;
	register struct lbuf **ep;

	if (Cflg || mflg) {
		if (num_cols >= colwidth) {
			ncols = num_cols / colwidth;
		}
		else {
			ncols = 1;
		}
	}

	if (ncols == 1 || mflg || xflg || !Cflg) {
		for (ep = slp; ep < lp; ep++)
			pentry(*ep);
		new_line();
		return;
	}
	/* otherwise print -C columns */
	if (tot_flag)
		slp--;
	nrows = (lp - slp - 1) / ncols + 1;
	for (row = 0; row < nrows; row++) {
		col = (row == 0 && tot_flag);
		for (; col < ncols; col++) {
			ep = slp + (nrows * col) + row;
			if (ep < lp)
				pentry(*ep);
		}
		new_line();
	}
}

pentry(ap)  /* print one output entry;
            *  if uid/gid is not found in the appropriate
            *  file (passwd/group), then print uid/gid instead of
            *  user/group name;
            */
struct lbuf *ap;
{
	char *pwname(), *grname();
#if defined(DUX) || defined(CNODE_DEV)
	char *cnname();
#endif /* defined(DUX) || defined(CNODE_DEV) */
	struct	{
		char	dminor,
			dmajor;
	};
	register struct lbuf *p;
	register char *cp;
#ifdef NLS
	extern int __nl_langid[];
#endif
#if defined NLS16 && defined EUC
	unsigned char *s;
	int   c, cc, i;
#endif
	p = ap;
	column();
	if (iflg)
		if (mflg && !lflg)
			curcol += printf("%lu ", p->lnum);
		else
			curcol += printf("%6lu ", p->lnum);
	if (sflg)
		curcol += printf( (mflg && !lflg) ? "%ld " : "%4ld " ,
			(p->ltype != 'b' && p->ltype != 'c') ?
#ifdef HFS
				nblock(p->lblocks) : 0L );
#else
				nblock(p->lsize) : 0L );
#endif
	if (lflg) {
		putchar(p->ltype);
		curcol++;
		pmode(p->lflags);
#ifdef ACLS
		/*
		 * Print a char after mode to indicate optional ACL entries:
		 */
		curcol += printf("%1c %2d ", p->lacl, p->lnl);
#else /* ACLS */
		curcol += printf(" %3d ", p->lnl);
#endif /* ACLS */
		if (oflg)
			if(!nflg && ((pwbuf = pwname((int)p->luid)) != NULL))
				curcol += printf("%-9.9s", pwbuf);
			else
				curcol += printf("%-9u", p->luid);
		if (gflg)
			if(!nflg && ((grbuf = grname((int)p->lgid)) != NULL))
				curcol += printf("%-9.9s", grbuf);
			else
				curcol += printf("%-9u", p->lgid);
		if (p->ltype=='b' || p->ltype=='c')
#if defined(DUX) || defined(CNODE_DEV)
		    /*
		     * With the Hflag, print the cnode-specific device file
		     * information.
		     */
		    if (Hflg) {
			if ((nflg == 0) &&
				((cnbuf = cnname((int)p->lcnode)) != NULL))
				curcol += printf("%3d 0x%6.6x %-9.9s", major((int)p->lsize), minor((int)p->lsize), cnbuf);
			else
				curcol += printf("%3d 0x%6.6x %-9d", major((int)p->lsize), minor((int)p->lsize), (int)p->lcnode);
		    }
		    else
		    {
				curcol += printf("%3d 0x%6.6x", major((int)p->lsize), minor((int)p->lsize));
		    }
#else /* not CNODE_DEV */
				curcol += printf("%3d 0x%6.6x", major((int)p->lsize), minor((int)p->lsize));
#endif /* not CNODE_DEV */
		else
			curcol += printf("%7ld", p->lsize);
#ifndef NLS
		cp = ctime(&p->lmtime);
#endif
		if((p->lmtime < sixmonthsago) || (p->lmtime > onehourahead)) {
#ifdef NLS
		    if (__nl_langid[LC_TIME] == 0 || __nl_langid[LC_TIME] == 99)
			/* n-computer  or "C" */
			cp = nl_cxtime(&p->lmtime, "%3h %2d  %Y");
		    else
			cp = nl_cxtime(&p->lmtime, (catgets(nlmsg_tfd,NL_SETN,11, "%4h %2d  %Y")));
			curcol += printf(" %s ", cp);
#else
			curcol += printf(" %-7.7s %-4.4s ", cp+4, cp+20);
#endif
		}
		else {
#ifdef NLS
		    if (__nl_langid[LC_TIME] == 0 || __nl_langid[LC_TIME] == 99)
			/* n-computer  or "C" */
			cp = nl_cxtime(&p->lmtime, "%3h %2d %H:%M");
		    else
			cp = nl_cxtime(&p->lmtime, (catgets(nlmsg_tfd,NL_SETN,12, "%4h %2d %H:%M")));
			curcol += printf(" %s ", cp);
#else
			curcol += printf(" %-12.12s ", cp+4);
#endif
		}
	}
#ifdef RFA
	if ((pflg || Fflg) && (p->ltype == 'd' || p->ltype == 'n'))
#else
	if ((pflg || Fflg) && p->ltype == 'd')
#endif /* RFA */
		dmark = "/";
#if defined(DUX) || defined(DISKLESS)
	else if (Fflg && (p->ltype == 'H'))
		dmark = "+";
#endif /* defined(DUX) || defined(DISKLESS) */
	else
#ifdef SYMLINKS
	if (Fflg && (p->ltype == 'l'))
		dmark = "@";
	else
#endif /* SYMLINKS */
#ifdef ACLS
	/*
	 * Print a "*" if execute permission is on in
	 * any of the ACL entries.  Try the base modes and
	 * then the getaccess value for root.
	 */
	if (Fflg && ((p->lflags & 0111) || (p->allmodes & 1)))
#else /* no ACLS */
	if (Fflg && (p->lflags & 0111))
#endif /* ACLS */
		dmark = "*";
	else
	if (Fflg && (p->ltype == 'p'))   /* POSIX.2 */
		dmark = "|";		 /* POSIX.2 */
	else
		dmark = "";

	if (qflg || bflg)
		pprintf(p->lnamep,dmark);
	else
#if defined NLS16 && defined EUC
	for (s = p->lnamep, i = 0; i < 2; i++, s = dmark)
		while(c = *s++) {
			if (FIRSTof2(c) && SECof2((int)*s)) {
				cc = *s++;
				putc(c, stdout);
				putc(cc, stdout);
				curcol++;
				if (C_COLWIDTH(c) - 1)
					curcol++;
			}
			else {
				putc(c, stdout);
				curcol++;
			}
		}
#else
		curcol += printf("%s%s",p->lnamep,dmark);
#endif

#ifdef SYMLINKS
	if (lflg && p->llinkto)
	{
		if (qflg || bflg)
			pprintf(" -> ", p->llinkto);
		else
			curcol += printf(" -> %s", p->llinkto);
	}
#endif /* SYMLINKS */


}

/* print various r,w,x permissions
 */
pmode(aflag)
{
        /* these arrays are declared static to allow initializations */
	static int	m0[] = { 1, S_IREAD>>0, 'r', '-' };
	static int	m1[] = { 1, S_IWRITE>>0, 'w', '-' };
	static int	m2[] = { 3, S_ISUID|S_IEXEC, 's', S_IEXEC, 'x', S_ISUID, 'S', '-' };
	static int	m3[] = { 1, S_IREAD>>3, 'r', '-' };
	static int	m4[] = { 1, S_IWRITE>>3, 'w', '-' };
	static int	m5[] = { 3, S_ISGID|(S_IEXEC>>3),'s', S_IEXEC>>3,'x', S_ISGID,'S', '-'};
	static int	m6[] = { 1, S_IREAD>>6, 'r', '-' };
	static int	m7[] = { 1, S_IWRITE>>6, 'w', '-' };
	static int	m8[] = { 3, S_ISVTX|(S_IEXEC>>6),'t', S_IEXEC>>6,'x', S_ISVTX,'T', '-'};

        static int  *m[] = { m0, m1, m2, m3, m4, m5, m6, m7, m8};

	register int **mp;

	flags = aflag;
	for (mp = &m[0]; mp < &m[sizeof(m)/sizeof(m[0])];)
		selectmode(*mp++);
}

selectmode(pairp)
register int *pairp;
{
	register int n;

	n = *pairp++;
	while (n-->0) {
		if((flags & *pairp) == *pairp) {
			pairp++;
			break;
		}else {
			pairp += 2;
		}
	}
	putchar(*pairp);
	curcol++;
}

/*
 * column: get to the beginning of the next column.
 */
column()
{

	if (curcol == 0)
		return;
	if (mflg) {
		putc(',', stdout);
		curcol++;
		if (curcol + colwidth + 2 > num_cols) {
			putc('\n', stdout);
			curcol = 0;
			return;
		}
		putc(' ', stdout);
		curcol++;
		return;
	}
	if (Cflg == 0) {
		putc('\n', stdout);
		curcol = 0;
		return;
	}
	if ((curcol / colwidth + 2) * colwidth > num_cols) {
		putc('\n', stdout);
		curcol = 0;
		return;
	}
	do {
		putc(' ', stdout);
		curcol++;
	} while (curcol % colwidth);
}

new_line()
{
	if (curcol) {
		putc('\n',stdout);
		curcol = 0;
	}
}


/* read each filename in directory dir and store its
 *  status in flist[nfiles]
 *  use makename() to form pathname dir/filename;
 */
readDIR(dir, ltype)
char *dir;
char ltype;
{
	struct direct *dentry;
	register struct lbuf *ep;
	register int width;
	char	*dirptr;
#if defined NLS16 && defined EUC
	int	c, count;
	char	*p;
#endif

#ifdef RFA
	/* Append /. to get access remote root directory if remote file */
	if (ltype == 'n')
	     dirptr = makename(dir, ".");
	else dirptr = dir;
#else
	dirptr = dir;
#endif /* RFA */

	if ((dirp = opendir(dirptr)) == NULL) {
		fflush(stdout);
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,8, "%s unreadable\n")), dirptr);
		err = 2;
		return;
	}
        else {
          	tblocks = 0;
          	for(;;) {
			if ((dentry = readdir(dirp)) == NULL)
          			break;  /* end of directory */
          		if (dentry->d_ino==0
          			|| (aflg==0 && dentry->d_name[0]=='.'
#ifndef DOTSUP
          			&& (!Aflg || dentry->d_name[1]=='\0'
			 	    || (dentry->d_name[1]=='.'
          			    && dentry->d_name[2]=='\0'))
#endif
          			))  /* check for directory items '.', '..',
                                   *  and items without valid inode-number;
                                   */
          			continue;

			if (Cflg || mflg) {
				width = strlen(dentry->d_name);
#if defined NLS16 && defined EUC
			p = dentry->d_name;
			for (count = width; count > 0; count--) {
				c = *p++;
				c = c & 0377;
				if (FIRSTof2(c) && (C_COLWIDTH(c) == 1))
					width--;
			}
#endif
				if (width > filewidth)
					filewidth = width;
			}
          		ep = gstat(makename(dir, dentry->d_name), 0);
          		if (ep==NULL)
          			continue;
                        else {

#if defined(DUX) || defined(DISKLESS)
			    if (ep->lnum == 0)
				ep->lnum = dentry->d_ino;
#else /* not(defined(DUX) || defined(DISKLESS)) */
          		     ep->lnum = dentry->d_ino;
#endif /* not(defined(DUX) || defined(DISKLESS)) */
			     ep->lnamep = savestr(dentry->d_name);
                        }
          	}
		closedir (dirp);
		colwidth = fixedwidth + filewidth;
	}
}

/* get status of file and recomputes tblocks;
 * argfl = 1 if file is a name in ls-command and  = 0
 * for filename in a directory whose name is an
 * argument in the command;
 * stores a pointer in flist[nfiles] and
 * returns that pointer;
 * returns NULL if failed;
 */

struct lbuf *
gstat(file, argfl)
char *file;
{
	struct stat statb;
#ifdef SYMLINKS
	struct stat statb1;
	int (*statf)() = Lflg ? stat : lstat;
	char buf[MAXPATHLEN+1];
	int cc;
#endif /* SYMLINKS */
#if defined(DUX) || defined(DISKLESS)
	char *plusptr = NULL;
	char hbuf[MAXPATHLEN+1];
	int statres = -1;
#endif /* defined(DUX) || defined(DISKLESS) */
	register struct lbuf *rep;
	static int nomocore;
	char *realloc();
#ifdef ACLS
	int rootgid = 0;
#endif /* ACLS */

	if (nomocore)
		return(NULL);
	else if (nfiles >= maxfils) {
/* all flist/lbuf pair assigned files time to get some more space */
		maxfils += quantn;
		if((flist=(struct lbuf **)realloc((char *)flist, (unsigned)(maxfils * sizeof(struct lbuf *)))) == NULL
		|| (nxtlbf = (struct lbuf *)malloc((unsigned)(quantn * sizeof(struct lbuf)))) == NULL) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "ls: out of memory\n")));
			nomocore = 1;
			return(NULL);
		}
	}

/* nfiles is reset to nargs for each directory
 * that is given as an argument maxn is checked
 * to prevent the assignment of an lbuf to a flist entry
 * that already has one assigned.
 */
	if(nfiles >= maxn) {
		rep = nxtlbf++;
		flist[nfiles++] = rep;
		maxn = nfiles;
	}else {
		rep = flist[nfiles++];
	}
	rep->lflags = 0;

#ifdef RFA
	rep->r_level = 0;
#endif /* RFA */

	if (argfl || statreq) {
#if defined(DUX) || defined(DISKLESS)
		if (Hflg)
		{
		    (void) strcpy(hbuf, file);
		    file = hbuf;
		    if ((statres = checkhd(file, statf, &statb, argfl==0)) == 1)
			plusptr = strrchr(file, '+');
		}
		else
		    statres = (*statf)(file, &statb);

		if ((statres < 0) &&
			((statf == lstat || lstat(file, &statb) < 0)))
		{
		    if  (argfl || (checkhd(file,stat,&statb,1) != 1))
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "%s not found\n")), file);
		    nfiles--;
		    return(NULL);
		}
#else /* not (defined(DUX) || defined(DISKLESS)) */
#ifdef SYMLINKS
		if ((*statf)(file, &statb)<0)
		{
		    if (statf == lstat || lstat(file, &statb) < 0)
		    {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "%s not found\n")), file);
			nfiles--;
			return(NULL);
		    }
		}
#else /* not SYMLINKS */
		if (stat(file, &statb)<0)
		{
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "%s not found\n")), file);
			nfiles--;
			return(NULL);
		}
#endif /* not SYMLINKS */
#endif /* not (defined(DUX) || defined(DISKLESS)) */
                else {
	            	rep->lnum = statb.st_ino;
	            	rep->lsize = statb.st_size;
#ifdef HFS
			rep->lblocks = statb.st_blocks;
#endif
#ifdef SYMLINKS
	            	rep->llinkto = NULL;
#endif /* SYMLINKS */
#ifdef ACLS
			/*
			 * If the file has optional ACL entries, get the
			 * access permissions for root to find out if there are
			 * any execute permissions.
			 */
			if (statb.st_acl)
			{
			    rep->allmodes = getaccess(file, 0, 1, &rootgid,
				(void *)0, (void *)0);
			    if (rep->allmodes < 0)
				rep->allmodes = 0;
			}
			else
			    rep->allmodes = 0;
#endif /* ACLS */
	            	switch(statb.st_mode&S_IFMT) {

	            	case S_IFDIR:
#if defined(DISKLESS) || defined(DUX)
			    /*
			     *  did not change to S_ISCDF() macro as the
			     *  switch control expression has already done
			     *  some of the test
			     */
			    if (Hflg && (statb.st_mode & S_CDF)
				    && (!argfl || plusptr != NULL))
			    {
				/* Get rid of the plus used to stat with. */
				if (plusptr != NULL)
				    *plusptr = '\0';
	            		rep->ltype = 'H';
			    }
			    else
#endif /* defined(DISKLESS) || defined(DUX) */
	            		rep->ltype = 'd';
	            		break;

	            	case S_IFBLK:
	            		rep->ltype = 'b';
	            		rep->lsize = statb.st_rdev;
#if defined(DUX) || defined(CNODE_DEV)
				rep->lcnode = statb.st_rcnode;
#endif /* defined(DUX) || defined(CNODE_DEV) */
	            		break;

	            	case S_IFCHR:
	            		rep->ltype = 'c';
	            		rep->lsize = statb.st_rdev;
#if defined(DUX) || defined(CNODE_DEV)
				rep->lcnode = statb.st_rcnode;
#endif /* defined(DUX) || defined(CNODE_DEV) */
	            		break;

#ifdef hpux
	            	case S_IFIFO:
                 		rep->ltype = 'p';
                 		break;

			case S_IFSOCK:
				rep->ltype = 's';
				break;
#endif

#ifdef RFA
			case S_IFNWK:
				rep->ltype = 'n';
				rep->r_level = 1;
				break;
#endif /* RFA */

#ifdef SYMLINKS
			case S_IFLNK:
				rep->ltype = 'l';
				if (lflg) {
				    cc = readlink(file, buf, MAXPATHLEN);
				    if (cc >= 0) {
					    buf[cc] = 0;
					    rep->llinkto = savestr(buf);
				    }
				    break;
				}
				if (stat(file, &statb1) < 0)
				    break;
				if ((statb1.st_mode & S_IFMT) == S_IFDIR) {
				    statb = statb1;
				    rep->ltype = 'd';
				    rep->lsize = statb.st_size;
				}
			break;
#endif /* SYMLINKS */
                        default:
                                rep->ltype = '-';
                 	}
	          	rep->lflags = statb.st_mode & ~S_IFMT;
                                    /* mask ISARG and other file-type bits */
#ifdef ACLS
			/*
			 * Set flag indicating optional ACL entries:
			 */
	          	rep->lacl = (statb.st_acl ? OPACL : NOACL);
#endif /* ACLS */
	          	rep->luid = statb.st_uid;
	          	rep->lgid = statb.st_gid;
	          	rep->lnl = statb.st_nlink;
	          	if(uflg)
	          		rep->lmtime = statb.st_atime;
	          	else if (cflg)
	          		rep->lmtime = statb.st_ctime;
	          	else
	          		rep->lmtime = statb.st_mtime;
                        if (rep->ltype != 'b' && rep->ltype != 'c')
#ifdef HFS
			   tblocks += nblock(statb.st_blocks);
#else
	          	   tblocks += nblock(statb.st_size);
#endif
                }
	}
        return(rep);
}

long nblock(size)
long size;
{
	long blocks, tot;

	tot = howmany(dbtob(size), 512);
	return(tot);
}

/* returns pathname of the form dir/file;
 *  dir is a null-terminated string;
 */
char *
makename(dir, file)
char *dir, *file;
{
	static char dfile[MAXPATHLEN+1];/* MAXPATHLEN is the maximum
					 * length of a file/dir name in
					 * ls-command; dfile is static
					 * as this is returned by makename();
					 */
	int dirlen = strlen(dir),
	    filelen = strlen(file);

	if (dirlen + 1 + filelen + 1 > MAXPATHLEN) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,9, "ls: filename too long\n")));
		exit(1);
	}
	if (!strcmp(dir, "")) {
		(void) strcpy(dfile, file);
	}
	else {
		(void) strcpy(dfile, dir);
		if (dir[dirlen - 1] != '/' && *file != '/')
			(void) strcat(dfile, "/");
		(void) strcat(dfile, file);
	}

	return(dfile);
}


/* initcaches() - This routine initializes the passwd,group,cnode
   name caches. */
initcaches()
{
    int i;

    for (i = 0; i < PWCACHESIZE; i++)
    {
	pwcache[i].id = -1;
	pwcache[i].name = NULL;
    }

    for (i = 0; i < GRCACHESIZE; i++)
    {
	grcache[i].id = -1;
	grcache[i].name = NULL;
    }

#if defined(DUX) || defined(CNODE_DEV)
    for (i = 0; i < CNCACHESIZE; i++)
    {
	cncache[i].id = -1;
	cncache[i].name = NULL;
    }
#endif /* defined(DUX) || defined(CNODE_DEV) */
}


/* grname(gid) - This routine gets the name that corresponds to gid
 *  from the group file.  It caches using the key GRCACHEKEY.  The
 *  cache is GRCACHESIZE big.  If the gid is the same as the last
 *  gid this routine was called with, then the last value is returned.
 */
char *
grname(gid)
{
    struct group *getgrgid();
    struct group *group;
    static lastgid = -1;
    static char *result;
    struct idcache *entry;
    int key = GRCACHEKEY(gid);

    if (lastgid == gid)		/* Quick check for last gid. */
	return (result);

    entry = &(grcache[key]);	/* Get a pointer to this cache entry. */

    if (entry->id != gid)	/* Do we have a cache hit or miss. */
    {				/*  MISS! Replace entry from group file. */
	group = getgrgid(gid);

	if (entry->name != NULL)	/* Free last entries name. */
	    free(entry->name);

	if (group != NULL)	/* Did we find the gid? */
	{			/*  Yes: */
				/* Malloc/copy new name. */
	    entry->name = malloc(strlen(group->gr_name) + 1);
	    (void) strcpy(entry->name, group->gr_name);
	}
	else
	    entry->name = NULL;	/* No: then just return NULL. */

	entry->id = gid;	/* Be sure to put the gid in the cache. */
    }

    lastgid = gid;
    result = entry->name;

    return (result);
}


/* pwname(uid) - This routine gets the name that corresponds to uid
 *  from the passwd file in the same way that grname() does.  This
 *   routine also has a cache of size PWCACHESIZE.
 */
char
*pwname(uid)
int uid;
{
    struct passwd *getpwuid();
    struct passwd *pwd;
    static int lastuid = -1;
    static char *result;
    struct idcache *entry;
    int key;
#ifdef HP_NFS
    uid = ((uid == UID_NOBODY) ? -2 : uid);
#endif /* HP_NFS */
    key = PWCACHEKEY(uid);

    if (lastuid == uid)		/* Quick check for last uid. */
	return (result);

    entry = &(pwcache[key]);	/* Get a pointer to this cache entry. */

    if (entry->id != uid)	/* Do we have a cache hit or miss. */
    {				/*  MISS! Replace entry from passwd file. */
	pwd = getpwuid(uid);

	if (entry->name != NULL)	/* Free last entries name. */
	    free(entry->name);

	if (pwd != NULL)	/* Did we find the uid? */
	{			/*  Yes: */
				/* Malloc/copy new name. */
	    entry->name = malloc(strlen(pwd->pw_name) + 1);
	    (void) strcpy(entry->name, pwd->pw_name);
	}
	else
	    entry->name = NULL;	/* No: then just return NULL. */

	entry->id = uid;	/* Be sure to put the uid in the cache. */
    }

    lastuid = uid;
    result = entry->name;

    return (result);
}

#ifdef CNODE_DEV
/* cnname(cid) - This routine gets the name that corresponds to cid
 *  from the clusterconf file in the same way as grname() and pwname().
 *  It caches using the key CNCACHEKEY.  The cache is CNCACHESIZE big.
 */
char *
cnname(cid)
{
    struct cct_entry *getcccid();
    struct cct_entry *cct;
    static lastcid = -1;
    static char *result;
    struct idcache *entry;
    int key = CNCACHEKEY(cid);

    if (lastcid == cid)		/* Quick check for last cid. */
	return (result);

    entry = &(cncache[key]);	/* Get a pointer to this cache entry. */

    if (entry->id != cid)	/* Do we have a cache hit or miss. */
    {				/*  MISS! Replc entry from /etc/clusterconf. */
	cct = getcccid(cid);

	if (entry->name != NULL)	/* Free last entries name. */
	    free(entry->name);

	if (cct != NULL)	/* Did we find the cid? */
	{			/*  Yes: */
				/* Malloc/copy new name. */
	    entry->name = malloc(strlen(cct->cnode_name) + 1);
	    (void) strcpy(entry->name, cct->cnode_name);
	}
	else
	    entry->name = NULL;	/* No: then just return NULL. */

	entry->id = cid;	/* Be sure to put the cid in the cache. */
    }

    lastcid = cid;
    result = entry->name;

    return (result);
}
#endif /* CNODE_DEV */

compar(pp1, pp2)  /* return >0 if item pointed by pp2 should appear first */
struct lbuf **pp1, **pp2;
{
	register struct lbuf *p1, *p2;

	p1 = *pp1;
	p2 = *pp2;
	if (dflg==0) {
		/* compare two names in ls-command one of which is file
		 *  and the other is a directory;
		 *  this portion is not used for comparing files within
		 *  a directory name of ls-command;
		 */

#if defined(DUX) || defined(DISKLESS)

		if (p1->lflags&ISARG && (p1->ltype=='d' || p1->ltype=='H')) {
			if (!(p2->lflags&ISARG && (p2->ltype=='d' || p2->ltype=='H')))
				return(1);
		}
		else {
			if (p2->lflags&ISARG && (p2->ltype=='d' || p2->ltype=='H'))
				return(-1);
		}
#else /* not (defined(DUX) || defined(DISKLESS)) */
		if (p1->lflags&ISARG && p1->ltype=='d') {
			if (!(p2->lflags&ISARG && p2->ltype=='d'))
				return(1);
                }
		else {
			if (p2->lflags&ISARG && p2->ltype=='d')
				return(-1);
		}
#endif /* not (defined(DUX) || defined(DISKLESS)) */
	}
	if (tflg) {
		if(p2->lmtime == p1->lmtime)
			return(0);
		else if(p2->lmtime > p1->lmtime)
			     return(rflg);
		else return(-rflg);
	}
        else
#ifdef NLS || NLS16
	{
	  return(rflg * strcoll(p1->lnamep, p2->lnamep));
	}
#else
	  return(rflg * strcmp(p1->lnamep, p2->lnamep));
#endif
}

pprintf(s1,s2)

	unsigned char *s1, *s2;   /* NLS - unsigned so not neg. value      */
{

	register unsigned char *s; /* NLS - unsigned so not neg. value     */

	register int   c;
	register int  cc;
	int i;

	for (s = s1, i = 0; i < 2; i++, s = s2)
#ifdef NLS16
#ifdef EUC
		while(c = *s++) {
			if (FIRSTof2(c) && SECof2((int)*s)) {
				putc(c, stdout);
				if (C_COLWIDTH(c) -1)
					curcol++;
				c = *s++;
			}
#else /* EUC */
		while(c = CHARADV(s)) {
			if (c > 0377) {		/* 2-byte character */
				putc((c>>8), stdout);
				c = c & 0377;
				curcol++;
			}
#endif /* EUC */
                        /* NLS - determine if char. is printable, which   */
                        /*       depends upon LANG & LC_CTYPE env. var. */
			else if (!isprint(c)) {

				if (qflg)
					c = '?';
				else if (bflg) {
					curcol += 3;
					putc ('\\', stdout);
					cc = '0' + (c>>6 & 07);
					putc (cc, stdout);
					cc = '0' + (c>>3 & 07);
					putc (cc, stdout);
					c = '0' + (c & 07);
				}
			}
			curcol++;
			putc(c, stdout);
		}
#else /* not NLS16 */
		while(c = *s++) {

                        /* NLS - determine if char. is printable, which   */
                        /*       depends upon LANG & LC_CTYPE env. var. */
			if (!isprint(c)) {

				if (qflg)
					c = '?';
				else if (bflg) {
					curcol += 3;
					putc ('\\', stdout);
					cc = '0' + (c>>6 & 07);
					putc (cc, stdout);
					cc = '0' + (c>>3 & 07);
					putc (cc, stdout);
					c = '0' + (c & 07);
				}
			}
			curcol++;
			putc(c, stdout);
		}
#endif /* NLS16 */
}


#if defined(DUX) || defined(DISKLESS)

/* Checkhd() - This routine is used to determine if a file is a hidden
   directory or not.  The arguements include the name of the file, the
   stat function to use, a stat structure pointer to return the stat info
   in, and a flag that if 1 means check for hidden for all files and if
   0 means check only files that don't have a last component of "." or "/..".
   The return value is 1 iff name is hidden, 0 iff name is statable but
   not hidden and -1 iff not stat'able. */

int
checkhd(name, statf, statb, dotflag)
register char *name;
int (*statf)();
register struct stat *statb;
int dotflag;
{
	char *ptr, *slash;
	int len;

	if (name == NULL)	/* Garbage in ... */
	    return (0);

	    /* The following FOR loop does the same as:
	     * len = strlen(name);
	     * slash = strrchr(name, '/');
	     * They are combined together for performance.
	     */
	for (ptr = name, len = 0, slash = NULL; *ptr != '\0'; ptr++, len++)
	{
	    if (*ptr == '/')
		slash = ptr;
	}

	if (slash != NULL)	/* If we found '/', point to the next char. */
	    slash++;
	else if (len == 2)	/* If we didn't find '/' and the len <= 2, */
	    slash = name;	/* The name could be "." or "..". */

				/* The following IF does this:
				    IF the dotflag is false OR
					the dot flag is true and the name ends
					in "." or "..",
				    THEN try stating with a plus appended. */
	if ((dotflag) ||
		(slash == NULL) || (*slash++ != '.') ||
		(*slash++ != '.') || (*slash != '\0'))
	{
	    *ptr++ = '+';
	    *ptr = '\0';
	}

				/* Stat the file and check for hidden. */
	if (((*statf)(name, statb) == 0) && (S_ISCDF(statb->st_mode)))
	    return(1);
	else			/* Else return the stat of the original file. */
	{
	    name[len] = '\0';

	    return ((*statf)(name, statb));
	}
}
#endif /* defined(DUX) || defined(DISKLESS) */


char *
savestr(str)
char * str;
{
    char *cp = malloc(strlen(str) + 1);

    if (cp == NULL)
    {
	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "ls: out of memory\n")));
	exit(1);
    }

    (void) strcpy(cp, str);

    return (cp);
}
