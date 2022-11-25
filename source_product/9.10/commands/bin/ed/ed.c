/* @(#) $Revision: 70.10 $ */    
/*
**	Editor
*/

#ifdef NLS
#include <locale.h>
#include <nl_types.h>
nl_catd	catd;
#define	NL_SETN	1	/* set number */
#else NLS
#define catgets(i, j, k, s)	(s)
#endif NLS

char	*msgtab[] =
{
	"write or open on pipe failed",			/*  0 *//* catgets 1 */
	"warning: expecting `w'",			/*  1 *//* catgets 2 */
	"mark not lower case",				/*  2 *//* catgets 3 */
	"cannot open input file",			/*  3 *//* catgets 4 */
	"PWB spec problem",				/*  4 *//* catgets 5 */
	"nothing to undo",				/*  5 *//* catgets 6 */
	"restricted shell",				/*  6 *//* catgets 7 */
	"cannot create output file",			/*  7 *//* catgets 8 */
	"filesystem out of space!",			/*  8 *//* catgets 9 */
	"cannot open file",				/*  9 *//* catgets 10 */
	"cannot link",					/* 10 *//* catgets 11 */
	"Range endpoint too large",			/* 11 *//* catgets 12 */
	"unknown command",				/* 12 *//* catgets 13 */
	"search string not found",			/* 13 *//* catgets 14 */
	"-",						/* 14 *//* catgets 15 */
	"line out of range",				/* 15 *//* catgets 16 */
	"bad number",					/* 16 *//* catgets 17 */
	"bad range",					/* 17 *//* catgets 18 */
	"Illegal address count",			/* 18 *//* catgets 19 */
	"incomplete global expression",			/* 19 *//* catgets 20 */
	"illegal suffix",				/* 20 *//* catgets 21 */
	"illegal or missing filename",			/* 21 *//* catgets 22 */
	"no space after command",			/* 22 *//* catgets 23 */
	"fork failed - try again",			/* 23 *//* catgets 24 */
	"maximum of 64 characters in file names",	/* 24 *//* catgets 25 */
	"`\\digit' out of range",			/* 25 *//* catgets 26 */
	"interrupt",					/* 26 *//* catgets 27 */
	"line too long",				/* 27 *//* catgets 28 */
	"illegal character in input file",		/* 28 *//* catgets 29 */
	"write error",					/* 29 *//* catgets 30 */
	"out of memory for append",			/* 30 *//* catgets 31 */
	"temp file too big",				/* 31 *//* catgets 32 */
	"I/O error on temp file",			/* 32 *//* catgets 33 */
	"multiple globals not allowed",			/* 33 *//* catgets 34 */
	"global too long",				/* 34 *//* catgets 35 */
	"no match",					/* 35 *//* catgets 36 */
	"illegal or missing delimiter",			/* 36 *//* catgets 37 */
	"-",						/* 37 *//* catgets 38 */
	"replacement string too long",			/* 38 *//* catgets 39 */
	"illegal move destination",			/* 39 *//* catgets 40 */
	"-",						/* 40 *//* catgets 41 */
	"no remembered search string",			/* 41 *//* catgets 42 */
	"'\\( \\)' imbalance",				/* 42 *//* catgets 43 */
	"Too many `\\(' s",				/* 43 *//* catgets 44 */
	"more than 2 numbers given",			/* 44 *//* catgets 45 */
	"'\\}' expected",				/* 45 *//* catgets 46 */
	"first number exceeds second",			/* 46 *//* catgets 47 */
	"incomplete substitute",			/* 47 *//* catgets 48 */
	"newline unexpected",				/* 48 *//* catgets 49 */
	"'[ ]' imbalance",				/* 49 *//* catgets 50 */
	"regular expression overflow",			/* 50 *//* catgets 51 */
	"regular expression error",			/* 51 *//* catgets 52 */
	"command expected",				/* 52 *//* catgets 53 */
	"a, i, or c not allowed in G",			/* 53 *//* catgets 54 */
	"end of line expected",				/* 54 *//* catgets 55 */
	"no remembered replacement string",		/* 55 *//* catgets 56 */
	"no remembered command",			/* 56 *//* catgets 57 */
	"illegal redirection",				/* 57 *//* catgets 58 */
	"possible concurrent update",			/* 58 *//* catgets 59 */
	"that command confuses yed",			/* 59 *//* catgets 60 */
	"the x command has become X (upper case)",	/* 60 *//* catgets 61 */
	"Warning: 'w' may destroy input file (due to `illegal char' read earlier)", /* 61 *//* catgets 62 */
	"Caution: 'q' may lose data in buffer; 'w' may destroy input file", /* 62 *//* catgets 63 */
	0
};

#ifndef NONLS
#include <nl_ctype.h>
#endif NONLS

/*
 * Define some macros for the regular expression 
 * routines to use for input and error stuff.
 */

#define INIT		extern int peekc;
#define GETC()		getchr()
#define UNGETC(c)	(peekc = c)
#define PEEKC()		(peekc = getchr())
#define RETURN(c)	return
#define ERROR(c)	error1(c)

#include <regexp.h>

#include <stdio.h>

#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termio.h>

#ifndef RESEARCH
#ifdef	HP_NFS
#include <sys/vfs.h>
#ifdef	RFA
#include <errnet.h>
#endif /* RFA */
#else not HP_NFS
#include <ustat.h>
#endif not HP_NFS
#endif RESEARCH

#include <errno.h>
#include <setjmp.h>
#define	PUTM()	if(xcode >= 0) puts(catgets(catd, NL_SETN, xcode+1, msgtab[xcode]))
#define	FNSIZE	64			/* file name size */
#ifndef LINE_MAX 
#define LINE_MAX 2048
#endif
/*  POSIX.2 (P1003.2/D11 section 4.20.10):
 *  ed shall process lines of LINE_MAX length 
 */
#define LBSIZE  LINE_MAX		/* line buffer size */
#define GOCCUR  LINE_MAX+1		/* global occurance, see gsubf */
#define ESIZE	256			/* exp buffer size */
#define	GBSIZE	256			/* global buffer size */
#ifdef MAXPATHLEN
#undef MAXPATHLEN
#endif
#define MAXPATHLEN	1023		/* maximum length of the path	*/
					/* name without trailing null	*/


#define	KSIZE	9
#define	READ	0
#define	WRITE	1
#define PRNT	02

/* the following values are used for the calculation of the	*/
/* address of the line in the temporary file			*/
#define LBTMSK	0774	/* mask to calculate the offset	*/
#define BLKMSK	0177777	/* mask to calculate the block number	*/
			/* 2**16-1				*/
#define SHFT	1	/* shift value to calculate the offset	*/
#define OFFBITS	8	/* shift value to calculate the block number	*/
#define	BLKSIZ	512	/* the size of the block by byte	*/
#define HLFBLK	256	/* the half size of the block		*/
#define MAXBLK	65535	/* the maximum number of blocks		*/
			/* 2**16 -1				*/

#ifndef NONLS	/* 8 bit integrity */
int	crflag;		/* if necessary to crypt, crflag is set */
char	lastchar;	/* last char on decryption */
#endif NONLS


int	Xqt = 0;
int	lastc;
char	savedfile[FNSIZE];
char	file[FNSIZE];
char	funny[LBSIZE];
char	linebuf[LBSIZE];

char	expbuf[ESIZE];
char	rhsbuf[LBSIZE];

char	tmpfilename[MAXPATHLEN+1] ;	/* contains the path of the	*/
					/* temporary file		*/

/* This data is used to point to the line in the temporary file          */
/* _____________________________________________________________________ */
/* |               |                      |               |            | */
/* | unused(8bits) | block number(16bits) | offset(7bits) | flag(1bit) | */
/* |_______________|______________________|_______________|____________| */
/*                                                                       */
/* The line pointers are lined from the first break value and pointed by */
/* the pointers LINE.  If a LINE type data is incremented by one, the    */
/* LINE type data points to the next line pointer and vice versa.  So    */
/* the access to the line in the temporary file is done indirectly with  */
/* LINE type data.                                                       */

struct	lin	{
	int cur;	/* the line pointer	*/
	int sav;	/* the saved line pointer for undo operation	*/
};
typedef struct lin *LINE;

LINE	zero;	/* the starting point of the lines			*/
LINE	dot;	/* the current line					*/
LINE	dol;	/* the last line					*/
LINE	endcore;/* the limit of the space for lin			*/
LINE	fendcore; /* the first limit of the memory			*/
LINE	addr1;	/* the starting point of the range affected by the commands */
LINE	addr2;	/* the ending point of the range affected by the commands   */
LINE	savdol, savdot;	/* used to save dol or dot			*/
int	globflg;
int	initflg;

char	genbuf[LBSIZE];

long	count;

int	numpass;	/* Number of passes thru dosub(). */
int	gsubf;		/* Occurrence value. GOCCUR=all. */
int	ocerr1;		/* Allows lines NOT changed by dosub() to NOT be put
			out. Retains last line changed as current line. */
int	ocerr2;		/* Flags if ANY line changed by substitute(). 0=nc. */

char	*nextip;
char	*linebp;
int	ninbuf;
int	peekc;

int	io;
int	(*oldhup)();
int	(*oldquit)(), (*oldpipe)();
int	vflag = 1;	/* verbose flag */
			/* 1 = print byte counts */
			/* 0 =  suppress byte counts for e, E, r, w and ! */
int	yflag;

/* ========================================================================= */
/*
** CRYPT block 1
*/
#ifdef CRYPT
int	xflag;
int	xtflag;
int	kflag;
#endif CRYPT
/* ========================================================================= */

int	hflag;
int	xcode = -1;

/* ========================================================================= */
/*
** CRYPT block 2
*/
#ifdef CRYPT
char	key[KSIZE + 1];
char	crbuf[512];
char	perm[768];
char	tperm[768];
#endif CRYPT
/* ========================================================================= */

int	col;
char	*globp;
int	tfile = -1;
int	tline;
char	*tfname = 0;

extern char	*locs;

char	ibuff[512];
int	iblock = -1;
char	obuff[512];
int	oblock = -1;
int	ichanged;
int	nleft;
int	names[26];
int	anymarks;
int	subnewa;
int	fchange;
int	nline;
int	fflg;				/* file flag */
int	shflg;				/* alternate shell prompt flag */

char	prompt[16] = "*";		/* alternate shell prompt string */

#ifdef NLS
char	*msgbuf;
#endif NLS


int	rflg;				/* restricted shell flag */
int	readflg;
int	eflg;
int	ncflg;
int	listn;
int	listf;
int	pflag;
int	tmperror ;	/* if the temporary file name is too long, this	*/
			/* flag will be set.				*/

int	flag28 = 0; /* Prevents write after a partial read */
int	save28 = 0; /* Flag whether buffer empty at start of read */

long 	savtime;
char	*name = "SHELL";
char	*rshell = "/bin/rsh";
char	*val;

char	*home;
char	*calloc();

char	*getenv();
LINE	address();
char	*getline();
char	*getblock();
char	*place();
char	*mktemp();
char	*sbrk();

struct	stat	Fl, Tf, stat_buf;

#ifndef RESEARCH
#ifdef	HP_NFS
struct	statfs  Statfsbuf;
#else not HP_NFS
struct	ustat	U;
#endif not HP_NFS
int	Short = 0;
int	oldmask; /* No umask while writing */
#endif RESEARCH

jmp_buf	savej;

#ifdef NULLS
int	nulls;	/* Null count */
#endif NULLS

long	ccount;

int 	firstcmd;	/* flag set on startup and cleared after first command
			   that keeps the buffer from being saved, so if user 
			   types undo, his initial read will not be undone */

struct	Fspec	{
	char	Ftabs[22];
	char	Fdel;
	unsigned char	Flim;
	char	Fmov;
	char	Ffill;
};
struct	Fspec	fss;

int	errcnt = 0;

extern  char    *optarg;                /* for getopt */
extern  int     optind, opterr;         /* for getopt */
	int     c;                      /* for getopt, option character */

onpipe()
{
	if (tfname && *tfname)		/* ensure we clean up /tmp file */
		unlink(tfname);
	error(0);
}

/******************************************************************************/
/*                     main routine                                           */
/******************************************************************************/

main(argc, argv)
int	argc;
char	**argv;
{
	register	char *p1, *p2;
	extern	int onintr(), quit(), onhup();
	int	(*oldintr)();

#ifdef NLS
	nl_catd		catd;

	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale("ed"), stderr);
		putenv("LANG=");
		catd = (nl_catd)-1;
	} else
		catd = catopen("ed", 0);
#endif NLS

	oldquit = signal(SIGQUIT, SIG_IGN);
	oldhup = signal(SIGHUP, SIG_IGN);
	oldintr = signal(SIGINT, SIG_IGN);
	oldpipe = signal(SIGPIPE, onpipe);
	if (((int)signal(SIGTERM, SIG_IGN)&01) == 0)
		signal(SIGTERM, quit);

	p1 = *argv;
	while(*p1++);
	while(--p1 >= *argv)
		if(*p1 == '/')
			break;
	/* *argv = p1 + 1; /**/

	/* if SHELL set in environment and is /bin/rsh, set rflg */

	if((val = getenv(name)) != NULL)
		if (strcmp(val, rshell) == 0)
			rflg++;

     /* if (**argv == 'r') /**/
	if (*(p1+1) == 'r')
		rflg++;

	home = getenv("HOME");

	/*opterr = 0;			/* getopt local error processing */
	do {				/* getopt with '+' options */
	    while ((c=getopt(argc,argv, "p:qsxy"))!=EOF) {
		switch (c) {

		case 'p':
			strncpy(prompt, optarg, 16);
			shflg = 1;
			break;

		case 'q':		/* falls thru to 's' */
		case 's':
			vflag = 0;
			break;

		case 'x':

/* ========================================================================= */
/*
** CRYPT block 3
*/
#ifdef CRYPT
			xflag = 1;
#else CRYPT
			puts(catgets(catd, NL_SETN, 65,"-x option is not available"));
#endif CRYPT
/* ========================================================================= */

			break;

		case 'y':
			yflag = 03;
			break;

		case '?': /* -(numeric) */
			/* error message? */
			exit (2);
		    	break;

		} /* switch (c) */
	    } /* while (c=getopt(argc,argv,)) */

	    if (optind > 0 && strcmp(argv[optind-1], "--") == 0)
		c = EOF;
	    else
		{
		if (argv[optind][0] == '-' &&	/* select - (same as -s) */
		    argv[optind][1] == '\0') {
			vflag = 0;
		        c='0';		/* ensure not EOF */
		        ++optind;
		    }
		}

	} while (c != EOF && optind < argc);	/* do getopt */

/* ========================================================================= */
/*
** CRYPT block 4
*/
#ifdef CRYPT
	if(xflag){
		getkey();
		kflag = crinit(key, perm);
	}
#endif CRYPT
/* ========================================================================= */

	if (optind < argc) {
		p1 = argv[optind];
		if(strlen(p1) >= FNSIZE) {
			puts(catgets(catd, NL_SETN, 66,"file name too long"));
			exit(2);
		}
		p2 = savedfile;
		while (*p2++ = *p1++);

#ifndef hp9000s500
		globp = "r";
#else hp9000s500
		globp = "e";
#endif hp9000s500

		fflg++;
	}
	else 	/* editing with no file so set savtime to 0 */
		savtime = 0;
	eflg++;
	fendcore = (LINE )sbrk(0);
	/* get the path of the temporary file from TMPDIR	*/
	tmperror = 0 ;
	tfname = getenv("TMPDIR") ;
	if( ( tfname != NULL ) && *tfname ) {
		/* We need the space for "/eXXXXX"		*/

		if( strlen( tfname ) > ( MAXPATHLEN - 7 ) ) {
			/* This will cause the temp file error later	*/
			/* We substitute the temporary value to tfname	*/
			tmperror++ ;
			tfname = mktemp("/tmp/eXXXXX") ;
		}
		strcpy( tmpfilename, tfname ) ;
		strcat( tmpfilename, mktemp("/eXXXXX") ) ;
		tfname = tmpfilename ;
	}else{
		tfname = mktemp("/tmp/eXXXXX");
	}
	init();
	if (((int)oldintr&01) == 0)
		signal(SIGINT, onintr);
	if (((int)oldhup&01) == 0)
		signal(SIGHUP, onhup);
	setjmp(savej);
	commands();
	quit();
}

commands()
{

#ifndef NONLS	/* 8 bit integrity */
	long	current;	/* current file pointer position */
#endif NONLS

	int getfile(), gettty();
	register LINE a1;
	register c;
	register char *p1, *p2;
#if defined(HP_NFS) && defined(RFA)
	int rfa_file_sys = 0;
#endif

	int fsave, m, n;

	for (;;) {
	nodelim = 0;
	if ( pflag ) {
		pflag = 0;
		addr1 = addr2 = dot;
		goto print;
	}
	if (shflg && globp==0)
		write(1, prompt, strlen(prompt));

	addr1 = 0;
	addr2 = 0;
	if((c=getchr()) == ',') {
		addr1 = zero + 1;
		addr2 = dol;
		c = getchr();
		goto swch;
	}
	else if(c == ';') {
		addr1 = dot;
		addr2 = dol;
		c = getchr();
		goto swch;
	}
	else
		peekc = c;
	do {
		addr1 = addr2;
		if ((a1 = address())==0) {
			c = getchr();
			break;
		}
		addr2 = a1;
		if ((c=getchr()) == ';') {
			c = ',';
			dot = a1;
		}
	} while (c==',');
	if (addr1==0)
		addr1 = addr2;
swch:
	switch(c) {

	case 'a':
		setdot();
		newline();
		if (!globflg) save();
		append(gettty, addr2);
		continue;

	case 'c':
		delete();
		append(gettty, addr1-1);
		continue;

	case 'd':
		delete();
		continue;

	case 'E':
		fchange = 0;
		c = 'e';
	case 'e':
		fflg++;
		setnoaddr();
		if (vflag && fchange) {
			fchange = 0;
			error(1);
		}
		filename(c);
		eflg++;
		init();
		addr2 = zero;
		goto caseread;

	case 'f':
		setnoaddr();
		filename(c);
		if (!ncflg)  /* there is a filename */
			getime();
		else
			ncflg--;
		puts(savedfile);
		continue;

	case 'g':
		global(1);
		continue;
	case 'G':
		globaln(1);
		continue;

	case 'h':
		newline();
		setnoaddr();
		PUTM();
		continue;

	case 'H':
		newline();
		setnoaddr();
		if(!hflag) {
			hflag = 1;
			PUTM();
		}
		else
			hflag = 0;
		continue;

	case 'i':
		setdot();
		nonzero();
		newline();
		if (!globflg) save();
		append(gettty, addr2-1);
		if (dot == addr2-1)
			dot += 1;
		continue;


	case 'j':
		if (addr2==0) {
			addr1 = dot;
			addr2 = dot+1;
		}
		setdot();
		newline();
		nonzero();
		if (!globflg) save();
		join();
		continue;

	case 'k':
		if ((c = getchr()) < 'a' || c > 'z')
			error(2);
		newline();
		setdot();
		nonzero();
		names[c-'a'] = addr2->cur & ~01;
		anymarks |= 01;
		continue;

	case 'm':
		move(0);
		continue;

	case '\n':
		if (addr2==0)
			addr2 = dot+1;
		addr1 = addr2;
		goto print;

	case 'n':
		listn++;
		newline();
		goto print;

	case 'l':
		listf++;
	case 'p':
		newline();
	print:
		setdot();
		nonzero();
		a1 = addr1;
		do {
			if (listn) {
				count = a1 - zero;
				putd();
				putchr('\t');
			}
			puts(getline(a1++->cur));
		}
		while (a1 <= addr2);
		dot = addr2;

		pflag = 0;

		listn = 0;
		listf = 0;
		continue;

	case 'Q':
		fchange = 0;
	case 'q':
		setnoaddr();
		newline();
		quit();

	case 'r':
		filename(c);
	caseread:
		readflg = 1;

		save28 = (dol != fendcore);

		if ((io = eopen(file, 0)) < 0) {
			lastc = '\n';
			/* if first entering editor and file does not exist */
			/* set saved access time to 0 */
			if (eflg) {
				savtime = 0;
				eflg  = 0;
			}
			error(3);
		}
		/* get last mod time of file */
		/* eflg - entered editor with ed or e  */
		if (eflg) {
			eflg = 0;
			getime();
		}
		setall();
		ninbuf = 0;
		n = zero != dol;

#ifdef NULLS
		nulls = 0;
#endif NULLS

 		if (!firstcmd && !globflg && (c == 'r')) save();
 		firstcmd = 0;	/* clear flag that stops initial buffer save */

#ifndef NONLS	/* 8 bit integrity */
		crflag = 0;
		lastchar = '\n';	/* for null input */
		if (Xqt)	{	/* shell command output will be read */
			while ((current = read(io, genbuf, LBSIZE)) > 0)
				lastchar = genbuf[current - 1];
			close(io);
			wait((int *) 0);
			if ((io = eopen(file, 0)) < 0)	{
				if (eflg) {
					savtime = 0;
					eflg  = 0;
				}
				error(3);
			}
		}
		else	{	/* normal file will be read */
			current = lseek(io, 0L, 1);  /* save current position */
			lseek(io, -1L, 2);	/* move pointer to last char */
			read(io, &lastchar, 1);	/* get last char */
			lseek(io, current, 0);	/* return current position */
		}
		if (lastchar != '\n')
			crflag = 1;	/* this is not a ordinary file */
#endif NONLS

		append(getfile, addr2);

#ifndef NONLS	/* 8 bit integrity */
		if ((crflag == 1) && (lastchar != '\n'))
			error(28);	/* decryption is failed */
#endif NONLS

		exfile();
		readflg = 0;
		fchange = n;
		continue;

	case 's':
		setdot();
		nonzero();
		if (!globflg) save();
		substitute(globp!=0);
		continue;

	case 't':
		move(1);
		continue;

	case 'u':
		setdot();
		newline();
		if (!initflg) undo();
		else error(5);
		fchange = 1;
		continue;

	case 'v':
		global(0);
		continue;
	case 'V':
		globaln(0);
		continue;

	case 'w':

		if(flag28) {
			flag28 = 0;
			fchange = 0;
			error(61);
		}

		setall();

		if((zero != dol) && (addr1 <= zero || addr2 > dol))
			error(15);

		filename(c);
		if(Xqt) {
			io = eopen(file, 1);
			n = 1;	/* set n so newtime will not execute */
		}
		else {

			fstat(tfile,&Tf);

			if(stat(file, &Fl) < 0) {
				if((io = creat(file, 0666)) < 0)
					error(7);

				fstat(io, &Fl);

				Fl.st_mtime = 0;
				close(io);
			}
			else {

#ifndef RESEARCH
				oldmask = umask(0);
#endif RESEARCH

			}

#ifndef RESEARCH

#ifdef	HP_NFS
#ifdef	RFA
			if (statfs(file, &Statfsbuf))
				/* failed because statfs doesn't support RFA? */
				rfa_file_sys = (errno == ENET) && (errnet == NE_NOSERV);
#else /* not RFA */
			statfs(file, &Statfsbuf);
#endif /* not RFA */
#else not HP_NFS
			ustat(Fl.st_dev, &U);
#endif not HP_NFS

/*
**  ESO -- sww --- the following line modified to use UKE block size
**  for free space computation.  6/4/82
*/
/*  DTS: UCSqm00410
 *  To prevent multiplication overflow (f_bfree or f_bavail can be as big as 
 *  2^31 on 2G byte file systems), the following comparisons are done via
 *  division instead of multiplication. 
 */
#ifndef hp9000s500
#ifdef	HP_NFS
#ifdef	RFA
			if(!rfa_file_sys && !Short &&
			    (Statfsbuf.f_bfree < (Tf.st_size + 100)/Statfsbuf.f_bsize))
#else /* not RFA */
			if(!Short &&
			    (Statfsbuf.f_bfree < (Tf.st_size + 100)/Statfsbuf.f_bsize))
#endif /* not RFA */
#else not HP_NFS
			if(!Short && U.f_tfree < ((Tf.st_size>>9) + 100))
#endif not HP_NFS
#else hp9000s500
			if(!Short && (U.f_tfree * U.f_blksize) < (Tf.st_size + 100))
#endif hp9000s500
			{

				Short = 1;
				error(8);
			}
			Short = 0;
#endif RESEARCH

			p1 = savedfile;		/* The current filename */
			p2 = file;
			m = strcmp(p1, p2);

			if (Fl.st_nlink == 1 && (Fl.st_mode & S_IFMT) == S_IFREG) {
				if (close(open(file, 1)) < 0)
					error(9);

				if (!(n=m))

					chktime();
				mkfunny();

				if ((io = creat(funny, Fl.st_mode)) >= 0) {
#ifndef ACLS
					chown(funny, Fl.st_uid, Fl.st_gid);
					chmod(funny, Fl.st_mode);
#else /* ACLS */
					/*
					 * If file has optional ACL entries,
					 * get them and set them on new file.
					 * This skips remote files (st_acl not
					 * set).
					 */
					if (Fl.st_acl) {
						cpacl(file, funny, Fl.st_mode, 
						  Fl.st_uid, Fl.st_gid, 
						  geteuid(), getegid());
					}
					chown(funny, Fl.st_uid, Fl.st_gid);
					if (!Fl.st_acl) 
						chmod(funny, Fl.st_mode);
#endif /* ACLS */
					putfile();
					exfile();
					unlink(file);
					if (link(funny, file))
						error(10);
					unlink(funny);
					/* if filenames are the same */
					if (!n)
						newtime();
					/* check if entire buffer was written */

					fsave = fchange;
					fchange = ((addr1==zero || addr1==zero+1) && addr2==dol)?0:1;
					if(fchange == 1 && m != 0) fchange = fsave;

					continue;
				}
			}
			else   n = 1;	/* set n so newtime will not execute*/
			if((io = creat(file, 0666)) < 0)
				error(7);
		}
		putfile();
		exfile();
		if (!n) newtime();

		fsave = fchange;
		fchange = ((addr1==zero||addr1==zero+1)&&addr2==dol)?0:1;
	/* Leave fchange alone if partial write was to another file */
		if(fchange == 1 && m != 0) fchange = fsave;

		continue;

	case 'X':

/* ========================================================================= */
/*
** CRYPT block 5
*/
#ifdef CRYPT
		setnoaddr();
		newline();
		xflag = 1;
		getkey();
		kflag = crinit(key, perm);
#else CRYPT
		puts(catgets(catd, NL_SETN, 67,"this option is not available"));
#endif CRYPT
/* ========================================================================= */

		continue;


	case '=':
		setall();
		newline();
		count = (addr2-zero);
		putd();
		putchr('\n');
		continue;

	case '!':
		unixcom();
		continue;

	case EOF:
		return;

	case 'P':
		if (yflag)
			error(59);
		setnoaddr();
		newline();
		if (shflg)
			shflg = 0;
		else
			shflg++;
		continue;
	}

/* ========================================================================= */
/*
** CRYPT block 6
*/
#ifdef CRYPT
	if (c == 'x')
		error(60);
	else
		error(12);
#endif CRYPT
/* ========================================================================= */

	error(12);
	}
 	firstcmd = 0;  /* clear flag that prevents inappropriate buffer save */
}

LINE 
address()
{
	register minus, c;
	register LINE a1;
	int n, relerr;

	minus = 0;
	a1 = 0;
	for (;;) {
		c = getchr();
		if ('0'<=c && c<='9') {
			n = 0;
			do {
				n *= 10;
				n += c - '0';
			} while ((c = getchr())>='0' && c<='9');
			peekc = c;
			if (a1==0)
				a1 = zero;
			if (minus<0)
				n = -n;
			a1 += n;
			minus = 0;
			continue;
		}
		relerr = 0;
		if (a1 || minus)
			relerr++;
		switch(c) {
		case ' ':
		case '\t':
			continue;
	
		case '+':
			minus++;
			if (a1==0)
				a1 = dot;
			continue;

		case '-':
		case '^':
			minus--;
			if (a1==0)
				a1 = dot;
			continue;
	
		case '?':
		case '/':
			compile((char *) 0, expbuf, &expbuf[ESIZE], c);
			a1 = dot;
			for (;;) {
				if (c=='/') {
					a1++;
					if (a1 > dol)
						a1 = zero;
				}
				else {
					a1--;
					if (a1 < zero)
						a1 = dol;
				}
				if (execute(0, a1))
					break;
				if (a1==dot)
					error(13);
			}
			break;
	
		case '$':
			a1 = dol;
			break;
	
		case '.':
			a1 = dot;
			break;

		case '\'':
			if ((c = getchr()) < 'a' || c > 'z')
				error(2);
			for (a1=zero; a1<=dol; a1++)
				if (names[c-'a'] == (a1->cur & ~01))
					break;
			break;
	
		case 'y' & 037:
			if(yflag) {
				newline();
				setnoaddr();
				yflag ^= 01;
				continue;
			}

		default:
			peekc = c;
			if (a1==0)
				return(0);
			a1 += minus;
			if (a1<zero || a1>dol)
				error(15);
			return(a1);
		}
		if (relerr)
			error(16);
	}
}

setdot()
{
	if (addr2 == 0)
		addr1 = addr2 = dot;
	if (addr1 > addr2)
		error(17);
}

setall()
{
	if (addr2==0) {
		addr1 = zero+1;
		addr2 = dol;
		if (dol==zero)
			addr1 = zero;
	}
	setdot();
}

setnoaddr()
{
	if (addr2)
		error(18);
}

nonzero()
{
	if (addr1<=zero || addr2>dol)
		error(15);
}

newline()
{
	register c;

	c = getchr();
	if ( c == 'p' || c == 'l' || c == 'n' ) {
		pflag++;
		if ( c == 'l') listf++;
		if ( c == 'n') listn++;
		c = getchr();
	}
	if ( c != '\n')
		error(20);
}

filename(comm)
{
	register char *p1, *p2;
	register c;
	register i = 0;

	count = 0;
	c = getchr();
	if (c=='\n' || c==EOF) {
		p1 = savedfile;
		if (*p1==0 && comm!='f')
			error(21);
		/* ncflg set means do not get mod time of file */
		/* since no filename followed f */
		if (comm == 'f')
			ncflg++;
		p2 = file;
		while (*p2++ = *p1++);
		red(savedfile);
		return;
	}
	if (c!=' ')
		error(22);
	while ((c = getchr()) == ' ');
	if(c == '!')
		++Xqt, c = getchr();
	if (c=='\n')
		error(21);
	p1 = file;
	do {
		if(++i >= FNSIZE)
			error(24);
		*p1++ = c;
		if(c==EOF || (c==' ' && !Xqt))
			error(21);
	} while ((c = getchr()) != '\n');
	*p1++ = 0;
	if(Xqt)
		if (comm=='f') {
			--Xqt;
			error(57);
		}
		else
			return;
	if (savedfile[0]==0 || comm=='e' || comm=='f') {
		p1 = savedfile;
		p2 = file;
		while (*p1++ = *p2++);
	}
	red(file);
}

exfile()
{

#ifdef NULLS
	register c;
#endif NULLS

#ifndef RESEARCH
	if(oldmask) {
		umask(oldmask);
		oldmask = 0;
	}
#endif RESEARCH

	eclose(io);
	io = -1;
	if (vflag) {
		putd();
		putchr('\n');

#ifdef NULLS
		if(nulls) {
			c = count;
			count = nulls;
			nulls = 0;
			putd();
			puts(catgets(catd, NL_SETN, 68," nulls replaced by '\\0'"));
			count = c;
		}
#endif NULLS

	}
}

onintr()
{
	signal(SIGINT, onintr);
	putchr('\n');
	lastc = '\n';

	globflg = 0;		/* For MR # bl82-03954 */

	if (*funny) unlink(funny); /* remove tmp file */
	/* if interruped a read, only part of file may be in buffer */
	if ( readflg ) {
		puts (catgets(catd, NL_SETN, 69,"\007read may be incomplete - beware!\007"));
		fchange = 0;
	}
	error(26);
}

onhup()
{
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	/* if there are lines in file and file was */

	/* not written since last update, save in ed.hup, or $HOME/ed.hup */

	if (dol > zero && fchange == 1) {
		addr1 = zero+1;
		addr2 = dol;
		io = creat("ed.hup", 0666);

		if(io < 0 && home) {
			char	*fn;

			fn = calloc(strlen(home) + 8, sizeof(char));
			if(fn) {
				strcpy(fn, home);
				strcat(fn, "/ed.hup");
				io = creat(fn, 0666);
				free(fn);
			}
		}

		if (io > 0) {
			putfile();
			eclose(io);
			io = -1;
		}
	}
	fchange = 0;
	quit();
}

error(code)
register code;
{
	register c;

	if(code == 28 && save28 == 0) {
		fchange = 0;
		flag28++;
	}

	readflg = 0;
	++errcnt;
	listf = listn = 0;
	pflag = 0;

#ifndef RESEARCH
	if(oldmask) {
		umask(oldmask);
		oldmask = 0;
	}
#endif RESEARCH

#ifdef NULLS	/* Not really nulls, but close enough */
	/* This is a bug because of buffering */
	if(code == 28) /* illegal char. */
		putd();
#endif NULLS

	putchr('?');
	if(code == 3)	/* Cant open file */
		puts(file);
	else
		putchr('\n');
	count = 0;
	fstat(0, &stat_buf);
	if (stat_buf.st_mode & 0100000) /* if a standard file, move to EOF */
		lseek(0, (long)0, 2);
	else if (stat_buf.st_mode & 010000) /* if pipe, read to EOF */
		while(getchr() != EOF);
	if (globp)
		lastc = '\n';
	globp = 0;
	peekc = lastc;
	if(lastc)
		while ((c = getchr()) != '\n' && c != EOF);
	if (io > 0) {
		eclose(io);
		io = -1;
	}
	/* 
	 * remove the temp (funny) file if write is failed
	 */
	if (code == 8 || code == 29 ) 
		unlink(funny);
	xcode = code;
	if(hflag)
		PUTM();

	if(code == 4)				/* Non-fatal error. */
		return(0);

	/*  POSIX.2 (P1003.3.2/D6 section 4.20.5.2)
	 *  if error happened in non-interactive mode, exit right away 
	 */ 
	if (S_ISREG(stat_buf.st_mode) || S_ISFIFO(stat_buf.st_mode)) {
		unlink(tfname);
		exit(2);
	}

	longjmp(savej, 1);
}

getchr()
{
	char c;

	if (lastc=peekc) {
		peekc = 0;
		return(lastc);
	}

	if (globp) {
		if ((lastc = *globp++ & 0377) != 0)
			return(lastc);
		globp = 0;
		return(EOF);
	}
	if (read(0, &c, 1) <= 0)
		return(lastc = EOF);

#ifdef NONLS	/* 8 bit integrity */
	lastc = c&0177;
#else NONLS
	lastc = c&0377;
#endif NONLS

	return(lastc);
}

gettty()
{
	register c;
	register char *gf;
	register char *p;

	p = linebuf;
	gf = globp;
	while ((c = getchr()) != '\n') {
		if (c==EOF) {
			if (gf)
				peekc = c;
			return(c);
		}

#ifdef NONLS	/* 8 bit integrity */
		if ((c &= 0177) == 0)
#else NONLS
		if (c == 0)
#endif NONLS

			continue;
		*p++ = c;
		if (p >= &linebuf[LBSIZE-2])
			error(27);
	}
	*p++ = 0;
	if (linebuf[0]=='.' && linebuf[1]==0)
		return(EOF);
	/*  POSIX.2 (P1003.2/D11, section 4.20.10):
	 *  No escaping of any characters in input mode allowed
	 */
	/*
	if (linebuf[0]=='\\' && linebuf[1]=='.' && linebuf[2]==0) {
		linebuf[0] = '.';
		linebuf[1] = 0;
	}
	*/
	return(0);
}

/* Read one line from io into linebuf	*/
getfile()
{
	register c;
	register char *lp, *fp;

#ifdef NONLS	/* 8 bit integrity */
	int crflag;

	crflag = 0;
#endif NONLS

	lp = linebuf;
	fp = nextip;
	do {
		if (--ninbuf < 0) {
			if ((ninbuf = read(io, genbuf, LBSIZE)-1) < 0)
				return(EOF);

#ifdef NONLS	/* 8 bit integrity */
			fp = genbuf;
			while(fp < &genbuf[ninbuf])
				if(*fp++ & 0200)
					crflag = 1;
#endif NONLS

/* ========================================================================= */
/*
** CRYPT block 7
*/
#ifdef CRYPT
			if(crflag) {
				if(kflag) {
					crblock(perm, genbuf, ninbuf+1, count);
				}
				else 
					error(28);
			}
#else CRYPT
			if (crflag)
				error(28);
#endif CRYPT
/* ========================================================================= */

			fp = genbuf;

#ifdef NONLS	/* 8 bit integrity */
			while(fp < &genbuf[ninbuf]) {
				if(*fp++ & 0200)
					error(28);
			}
			fp = genbuf;
#endif NONLS

		}
		if (lp >= &linebuf[LBSIZE]) {
			lastc = '\n';

#ifndef NONLS	/* 8 bit integrity */
			if (crflag)	break;
#endif NLS

			error(27);
		}

#ifdef NONLS	/* 8 bit integrity */
		if ((*lp++ = c = *fp++ & 0177) == 0) {
#else NONLS
		if ((lastchar = *lp++ = c = *fp++ & 0377) == 0) {
#endif NONLS

#ifdef NULLS
			lp[-1] = '\\';
			*lp++ = '0';
			nulls++;
#else NULLS
			lp--;
			continue;
#endif NULLS

		}
		count++;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;

	if (fss.Ffill && fss.Flim && lenchk(linebuf,&fss) < 0) {
#ifndef NLS
		write(1,"line too long: lno = ",21);
#else NLS
	 	msgbuf = catgets(catd, NL_SETN, 70,"line too long: lno = ");
		write(1, msgbuf, strlen(msgbuf));
#endif NLS
		ccount = count;
		count = (++dot-zero);
		dot--;
		putd();
		count = ccount;
		putchr('\n');
	}

	return(0);
}

/* put lines from addr1 to addr2 into io	*/
putfile()
{
	int n;
	LINE a1;
	register char *fp, *lp;
	register nib;

	nib = 512;
	fp = genbuf;
	a1 = addr1;
	do {
		lp = getline(a1++->cur);

		if (fss.Ffill && fss.Flim && lenchk(linebuf,&fss) < 0) {
#ifndef NLS
			write(1,"line too long: lno = ",21);
#else NLS
	 		msgbuf = catgets(catd, NL_SETN, 71,"line too long: lno = ");
			write(1, msgbuf, strlen(msgbuf));
#endif NLS
			ccount = count;
			count = (++dot-zero);
			dot--;
			putd();
			count = ccount;
			putchr('\n');
		}

		for (;;) {	/* put each block	*/
			if (--nib < 0) {
				n = fp-genbuf;

/* ========================================================================= */
/*
** CRYPT block 8
*/
#ifdef CRYPT
				if(kflag)
					crblock(perm, genbuf, n, count-n);
#endif CRYPT
/* ========================================================================= */

				if(write(io, genbuf, n) != n)
					error(29);
				nib = 511;
				fp = genbuf;
			}

			if(dol->cur == 0)	/* Allow write of null file */
				break;

			count++;
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	/* put the remaining data	*/
	n = fp-genbuf;

/* ========================================================================= */
/*
** CRYPT block 9
*/
#ifdef CRYPT
	if(kflag)
		crblock(perm, genbuf, n, count-n);
#endif CRYPT
/* ========================================================================= */

	if(write(io, genbuf, n) != n)
		error(29);
}

/* append the lines resulting from the function f to LINE a.	*/
append(f, a)
LINE a;
int (*f)();
{
	register LINE a1, a2, rdot;
	int tl;

	nline = 0;
	dot = a;
	while ((*f)() == 0) {
		if (dol >= endcore) {
			if ((int)sbrk(512*sizeof(struct lin)) == -1) {
				lastc = '\n';
				error(30);
			}
			endcore += 512;
		}
		tl = putline();
		nline++;
		a1 = ++dol;
		a2 = a1+1;
		rdot = ++dot;
		/* when new lines are inserted in the lines.	*/
		while (a1 > rdot)
			(--a2)->cur = (--a1)->cur;
		rdot->cur = tl;
	}
}

unixcom()
{
	register (*savint)(), pid, rpid;
	int retcode;
	static char savcmd[LBSIZE];	/* last command */
	char curcmd[LBSIZE];		/* current command */
	char *psavcmd, *pcurcmd, *psavedfile;
	register c, endflg=1, shflg=0;
	char shell[10];			/* current shell */

#ifdef NLS16
	int secondbyte = 0;
#endif NLS16

	setnoaddr();
	if(rflg)
		error(6);
	pcurcmd = curcmd;
	/* read command til end */
	/* a '!' found in beginning of command is replaced with the saved command.
	   a '%' found in command is replaced with the current filename */
	c=getchr();
	if (c == '!') {
		if (savcmd[0]==0) 
			error(56);
		else {
			psavcmd = savcmd;
			while (*pcurcmd++ = *psavcmd++);
			--pcurcmd;
			shflg = 1;
		}
	}
	else UNGETC(c);  /* put c back */

#ifndef NLS16
	while (endflg==1) {
		while ((c=getchr()) != '\n' && c != '%' && c != '\\')
			*pcurcmd++ = c;
		if (c=='%') { 
			if (savedfile[0]==0)
				error(21);
			else {
				psavedfile = savedfile;
				while(*pcurcmd++ = *psavedfile++);
				--pcurcmd;
				shflg = 1;
			}
		}
		else if (c == '\\') {
			c = getchr();
			if (c != '%')
				*pcurcmd++ = '\\';
			*pcurcmd++ = c;
		}
		else
			/* end of command hit */
			endflg = 0;
	}
#else NLS16
	while (endflg==1) {
		c=getchr();
		if (c == '\n')
			/* end of command hit */
			endflg = 0;
		else if (secondbyte) {
			*pcurcmd++ = c;
			secondbyte = 0;
		}
		else if (FIRSTof2(c)) {
			*pcurcmd++ = c;
			secondbyte++;
		}
		else if (c=='%') { 
			if (savedfile[0]==0)
				error(21);
			else {
				psavedfile = savedfile;
				while(*pcurcmd++ = *psavedfile++);
				--pcurcmd;
				shflg = 1;
			}
		}
		else if (c == '\\') {
			c = getchr();
			if (c != '%')
				*pcurcmd++ = '\\';
			if (FIRSTof2(c))
				secondbyte++;
			*pcurcmd++ = c;
		}
		else
			*pcurcmd++ = c;
	}
#endif NLS16

	*pcurcmd++ = 0;
	if (shflg == 1)
		puts(curcmd);
	/* save command */
	strcpy(savcmd,curcmd);

	/* changed to use environmental SHELL as the escaped shell */
	strcpy (shell, getenv("SHELL"));
	if (!shell[0])
		strcpy(shell, "/bin/sh");	/* default shell */

	if ((pid = fork()) == 0) {
		signal(SIGHUP, oldhup);
		signal(SIGQUIT, oldquit);

		close(tfile);

		execlp(shell, "sh", "-c", curcmd, (char *) 0);
		exit(0100);
	}
	savint = signal(SIGINT, SIG_IGN);
	while ((rpid = wait(&retcode)) != pid && rpid != -1);
	signal(SIGINT, savint);
	if (vflag)
		puts("!");
}

quit()
{
	if (vflag && fchange) {
		fchange = 0;

		if(flag28) {
			flag28 = 0; 
			error(62);	/* For case where user reads
					in BOTH a good file & a bad file */
		}

		error(1);
	}
	unlink(tfname);
	exit(errcnt? 2: 0);
}

delete()
{
	setdot();
	newline();
	nonzero();
	if (!globflg) save();
	rdelete(addr1, addr2);
}

rdelete(ad1, ad2)
LINE ad1, ad2;
{
	register LINE a1, a2, a3;

	a1 = ad1;
	a2 = ad2+1;
	a3 = dol;
	dol -= a2 - a1;
	do
		a1++->cur = a2++->cur;
	while (a2 <= a3);
	a1 = ad1;
	if (a1 > dol)
		a1 = dol;
	dot = a1;
	fchange = 1;
}

gdelete()
{
	register LINE a1, a2, a3;

	a3 = dol;
	for (a1=zero+1; (a1->cur&01)==0; a1++)
		if (a1>=a3)
			return;
	for (a2=a1+1; a2<=a3;) {
		if (a2->cur&01) {
			a2++;
			dot = a1;
		}
		else
			a1++->cur = a2++->cur;
	}
	dol = a1-1;
	if (dot>dol)
		dot = dol;
	fchange = 1;
}

/* get the line pointed by tl from the temporary file into	*/
/* linebuf and return the address of linebuf.			*/
char *
getline(tl)
{
	register char *bp, *lp;
	register int nl;

	lp = linebuf;
	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~0377;	/* delete the offset value	*/
	while (*lp++ = *bp++)
		/* when there are no data in the block but the line	*/
		/* does not end, the next block is gotten.		*/
		/* tl is incremented by half a block size because tl 	*/
		/* will be shifted by one in getblock()			*/
		if (--nl == 0) {
			bp = getblock(tl+=HLFBLK, READ);
			nl = nleft;
		}
	return(linebuf);
}

/* put the current line into the temporary file and increment	*/
/* the current line ( tline ).					*/
putline()
{
	register char *bp, *lp;
	register nl;
	int tl;

	fchange = 1;
	lp = linebuf;
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~0377;	/* delete the offset value	*/
	while (*bp = *lp++) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		/* when there are no space in the block but the line	*/
		/* does not end, the next block is gotten.		*/
		/* tl is incremented by half a block size because tl 	*/
		/* will be shifted by one in getblock()			*/
		if (--nl == 0) {
			bp = getblock(tl+=HLFBLK, WRITE);
			nl = nleft;
		}
	}
	nl = tline;
	tline += (((lp-linebuf)+03)>>1)&077776;
	return(nl);
}

/* write data to the temporary file ( iof == WRITE ) or read	*/
/* data from the temporary file ( iof == READ ).		*/
char *
getblock(atl, iof)
{
	extern read(), write();
	register int bno, off;
	register char *p1, *p2;
	register int n;
	
	bno = (atl>>OFFBITS)&BLKMSK;	/* calculate the block number	*/
	off = (atl<<SHFT)&LBTMSK;	/* calculate the offset value	*/
	if (bno >= MAXBLK) {	/* if the block number exceeds the	*/
		lastc = '\n';	/* limit, the error occurs.		*/
		error(31);
	}
	nleft = 512 - off;
	if (bno==iblock) {
		ichanged |= iof;
		return(ibuff+off);
	}
	if (bno==oblock)
		return(obuff+off);
	if (iof==READ) {
		if (ichanged) {

/* ========================================================================= */
/*
** CRYPT block 10
*/
#ifdef CRYPT
			if(xtflag)
				crblock(tperm, ibuff, 512, (long)0);
#endif CRYPT
/* ========================================================================= */

			blkio(iblock, ibuff, write);
		}
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, read);

/* ========================================================================= */
/*
** CRYPT block 11
*/
#ifdef CRYPT
		if(xtflag)
			crblock(tperm, ibuff, 512, (long)0);
#endif CRYPT
/* ========================================================================= */

		return(ibuff+off);
	}
	if (oblock>=0) {

/* ========================================================================= */
/*
** CRYPT block 12
*/
#ifdef CRYPT
		if(xtflag) {
			p1 = obuff;
			p2 = crbuf;
			n = 512;
			while(n--)
				*p2++ = *p1++;
			crblock(tperm, crbuf, 512, (long)0);
			blkio(oblock, crbuf, write);
		}
		 else
			blkio(oblock, obuff, write);
#else CRYPT
		blkio(oblock, obuff, write);
#endif CRYPT
/* ========================================================================= */

	}
	oblock = bno;
	return(obuff+off);
}

/* do the block io 	*/
blkio(b, buf, iofcn)
int b ;	/* block number, 1 block = 512 bytes	*/
char *buf;
int (*iofcn)();
{
	if ( tmperror ) {	/* if the temporary file name is invalid	*/
		error(32) ;
	}
	lseek(tfile, (long)b<<9, 0);
	if ((*iofcn)(tfile, buf, 512) != 512) {

		if(dol != zero)		 /* Bypass this if writing null file */
			error(32);

	}
}

/* open the temporary file and initialize the pointes	*/
init()
{
	register *markp;
	int omask;

	close(tfile);
 	firstcmd = 1;	/* set flag so buffer wont be saved */
	tline = 2;
	for (markp = names; markp < &names[26]; )
		*markp++ = 0;
	subnewa = 0;
	anymarks = 0;
	iblock = -1;
	oblock = -1;
	ichanged = 0;
	initflg = 1;
	if( tmperror == 0 ) {	/* if the temporary file name is valid */
		omask = umask(0);
		close(creat(tfname, 0600));
		umask(omask);
		tfile = open(tfname, 2);
	}

/* ========================================================================= */
/*
** CRYPT block 13
*/
#ifdef CRYPT
	if(xflag) {
		xtflag = 1;
		makekey(key, tperm);
	}
#endif CRYPT
/* ========================================================================= */

	brk((char *)fendcore);
	dot = zero = dol = savdot = savdol = fendcore;

	flag28 = save28 = 0;

	endcore = fendcore - sizeof(struct lin);
}

global(k)
{
	register char *gp;
	register c;
	register LINE a1;
	char globuf[GBSIZE];

#ifdef NLS16
	int secondbyte = 0;
#endif NLS16

	if (globp)
		error(33);
	setall();
	nonzero();
	if ((c=getchr())=='\n')
		error(19);
	save();
	compile((char *) 0, expbuf, &expbuf[ESIZE], c);
	gp = globuf;

#ifndef NLS16
	while ((c = getchr()) != '\n') {
		if (c==EOF)
			error(19);
		if (c=='\\') {
			c = getchr();
			if (c!='\n')
				*gp++ = '\\';
		}
		*gp++ = c;
		if (gp >= &globuf[GBSIZE-2])
			error(34);
	}
#else NLS16
	while ((c = getchr()) != '\n') {
		if (c==EOF)
			error(19);
		if (secondbyte)
			secondbyte = 0;
		else if (FIRSTof2(c))
			secondbyte++;
		else if (c=='\\') {
			c = getchr();
			if (c!='\n')
				*gp++ = '\\';
			if (FIRSTof2(c))
				secondbyte++;
		}
		*gp++ = c;
		if (gp >= &globuf[GBSIZE-2])
			error(34);
	}
#endif NLS16
	if (gp == globuf)
		*gp++ = 'p';
	*gp++ = '\n';
	*gp++ = 0;
	for (a1=zero; a1<=dol; a1++) {
		a1->cur &= ~01;
		if (a1>=addr1 && a1<=addr2 && execute(0, a1)==k)
			a1->cur |= 01;
	}
	/*
	 * Special case: g/.../d (avoid n^2 algorithm)
	 */
	if (globuf[0]=='d' && globuf[1]=='\n' && globuf[2]=='\0') {
		gdelete();
		return;
	}
	for (a1=zero; a1<=dol; a1++) {
		if (a1->cur & 01) {
			a1->cur &= ~01;
			dot = a1;
			globp = globuf;
			globflg = 1;
			commands();
			globflg = 0;
			a1 = zero;
		}
	}
}

join()
{
	register char *gp, *lp;
	register LINE a1;

	if (addr1 == addr2) return;
	gp = genbuf;
	for (a1=addr1; a1<=addr2; a1++) {

		lp = getline(a1->cur);
		while (*gp = *lp++)
			if (gp++ >= &genbuf[LBSIZE-2])
				error(27);
	}

	lp = linebuf;
	gp = genbuf;
	while (*lp++ = *gp++);

	addr1->cur = putline();
	if (addr1<addr2)
		rdelete(addr1+1, addr2);
	dot = addr1;
}

substitute(inglob)
{

	register nl;

	register LINE a1;
	int *markp;
	int getsub();

	int ingsav;		/* for saving inglob */
	ingsav = inglob;
	ocerr2 = 0;

	gsubf = compsub();
	for (a1 = addr1; a1 <= addr2; a1++) {
		if (execute(0, a1)==0)
			continue;
		numpass = 0;
		ocerr1 = 0;

		inglob |= 01;
		dosub();
		if (gsubf) {
			while (*loc2) {
				if (execute(1, (LINE )0)==0)
					break;
				dosub();
			}
		}

		if(ocerr1 == 0)		/* Don't put out-not changed. */
			continue;

		subnewa = putline();
		a1->cur &= ~01;
		if (anymarks) {
			for (markp = names; markp < &names[26]; markp++)
				if (*markp == a1->cur)
					*markp = subnewa;
		}
		a1->cur = subnewa;
		append(getsub, a1);
		nl = nline;
		a1 += nl;
		addr2 += nl;
	}

	if(ingsav)	/* Was in global-no error msg allowed. */
		return;

	if (inglob==0)		/* Not in global, but not found. */
		error(35);

	if(ocerr2 == 0)		/* RE found, but occurrence match failed. */
		error(35);

}

compsub()
{
	register seof, c;
	register char *p;
	static char remem[LBSIZE]={-1};

	int i;

#ifndef NONLS
	int slashflag = 0;
#endif NONLS

	if ((seof = getchr()) == '\n' || seof == ' ')
		error(36);
	compile((char *) 0, expbuf, &expbuf[ESIZE], seof);

	p = rhsbuf;

#ifdef NONLS	/* 8-bit integrity */
	for (;;) {
		c = getchr();
		if (c=='\\')
			c = getchr() | 0200;
		if (c=='\n') {
			if (nodelim == 1) {
				nodelim = 0;
				error(36);
			}
			if (globp && globp[0])
				c |= 0200;	/* insert '\' */
			else {
				UNGETC(c);
				pflag++;
				break;
			}
		}
		if (c==seof) {
			break;
		}
		*p++ = c;
		if (p >= &rhsbuf[LBSIZE])
			error(38);
	}
#else NONLS
	for (;;) {
		c = getchr();
		if (slashflag)
			slashflag = 0;
		else {
#ifndef NLS16
			if (c=='\\')
				slashflag++;
#else NLS16
			if (c=='\\') {
				int d;
				if (!FIRSTof2(d = getchr()))
					slashflag++;
				UNGETC(d);
			}
			else if (FIRSTof2(c)) {
				int d;
				if ((d = getchr()) != '\n')
					slashflag++;
				UNGETC(d);
			}
#endif NLS16
			if (c=='\n') {
				if (nodelim == 1) {
					nodelim = 0;
					error(36);
				}
				if (globp && globp[0]) {
					UNGETC(c);
					c = '\\';
					slashflag++;
				} else {
					UNGETC(c);
					pflag++;
					break;
				}
			}
			if (c==seof) {
				break;
			}
		}
		*p++ = c;
		if (p >= &rhsbuf[LBSIZE])
			error(38);
	}
#endif NONLS

	*p++ = 0;
	if(rhsbuf[0] == '%' && rhsbuf[1] == 0) 
		if(remem[0]!=-1) {
			strcpy(rhsbuf, remem);
		}
		else {
			error(55);
		}
	else
		strcpy(remem, rhsbuf);
	c = 0;
	peekc = getchr();	/* Gets char after third delimiter. */
	if(peekc == 'g') {
		c = GOCCUR;
		peekc = 0;
	}
	if(peekc >= '1' && peekc <= '9') {
		c = peekc-'0';
		peekc = 0;	/* Allows getchr() to get next char. */
		while(1) {
			i = getchr();
			if(i < '0' || i > '9')
				break;
			c = c*10 + i-'0';
			/* POSIX.2 (P1003.2/D11 section 4.20.10):
			 * increase suffix size from 512 to LBSIZE 
			 */
			if(c > LBSIZE)	/* "Illegal suffix" */
				error(20);
			}
		peekc = i;	/* Effectively an unget. */
		}
	newline();
	return(c);	/* Returns occurrence value. 0 & 1 both do first
			occurrence only: c=0 if ordinary substitute; c=1
			if use 1 in global sub (s/a/b/1). 0 in global
			form is illegal. */

}

getsub()
{
	register char *p1, *p2;

	p1 = linebuf;
	if ((p2 = linebp) == 0)
		return(EOF);
	while (*p1++ = *p2++);
	linebp = 0;
	return(0);
}

dosub()
{
	register char *lp, *sp, *rp;
	int c;

#ifdef NLS16
	int secondbyte = 0;
#endif NLS16

	if(gsubf > 0 && gsubf < GOCCUR) {
		numpass++;
		if(gsubf != numpass)
			return;
	}
	ocerr1++;
	ocerr2++;

	lp = linebuf;
	sp = genbuf;
	rp = rhsbuf;
	while (lp < loc1)
		*sp++ = *lp++;

	while (c = *rp++ & 0377) {
#ifdef NLS16
		if (secondbyte)
			secondbyte = 0;
		else if (FIRSTof2(c))
			secondbyte++;
		else
#endif NLS16
		if (c=='&') {
			sp = place(sp, loc1, loc2);
			continue;
		}
#ifdef NONLS	/* 8-bit integrity */
		else if(c & 0200) {
			c &= 0177;
#else NONLS
		else if(c == '\\') {
			c = *rp++ & 0377;
#endif NONLS
			if(c >= '1' && c < nbra + '1') {
				sp = place(sp, braslist[c-'1'], braelist[c-'1']);
				continue;
			}
#ifdef NLS16
			else if (FIRSTof2(c))
				secondbyte++;
#endif NLS16
		}
		*sp++ = c;
		if (sp >= &genbuf[LBSIZE])
			error(27);
	}
	lp = loc2;
	loc2 = sp - genbuf + linebuf;
	while (*sp++ = *lp++)
		if (sp >= &genbuf[LBSIZE])
			error(27);

	lp = linebuf;
	sp = genbuf;
	while (*lp++ = *sp++);
}

char *
place(sp, l1, l2)
register char *sp, *l1, *l2;

{
	while (l1 < l2) {
		*sp++ = *l1++;
		if (sp >= &genbuf[LBSIZE])
			error(27);
	}
	return(sp);
}

move(cflag)
{
	register LINE adt, ad1, ad2;
	int getcopy();

	setdot();
	nonzero();
	if ((adt = address())==0)
		error(39);
	newline();
	if (!globflg) save();
	if (cflag) {
		ad1 = dol;
		append(getcopy, ad1++);
		ad2 = dol;
	}
	else {
		ad2 = addr2;
		for (ad1 = addr1; ad1 <= ad2;)
			ad1++->cur &= ~01;
		ad1 = addr1;
	}
	ad2++;
	if (adt<ad1) {
		dot = adt + (ad2-ad1);
		if ((++adt)==ad1)
			return;
		reverse(adt, ad1);
		reverse(ad1, ad2);
		reverse(adt, ad2);
	}
	else if (adt >= ad2) {
		dot = adt++;
		reverse(ad1, ad2);
		reverse(ad2, adt);
		reverse(ad1, adt);
	}
	else
		error(39);
	fchange = 1;
}

reverse(a1, a2)
register LINE a1, a2;
{
	register int t;

	for (;;) {
		t = (--a2)->cur;
		if (a2 <= a1)
			return;
		a2->cur = a1->cur;
		a1++->cur = t;
	}
}

getcopy()
{

	if (addr1 > addr2)
		return(EOF);
	getline(addr1++->cur);
	return(0);
}


error1(code)
{
	expbuf[0] = 0;
	nbra = 0;
	error(code);
}

execute(gf, addr)
LINE addr;
{

	register char *p1, *p2, c;

	for (c=0; c<NBRA; c++) {
		braslist[c] = 0;
		braelist[c] = 0;
	}
	if (gf) {
		if (circf)
			return(0);
		locs = p1 = loc2;
	}
	else {
		if (addr==zero)
			return(0);
		p1 = getline(addr->cur);
		locs = 0;
	}
	return(step(p1, expbuf));
}


putd()
{
	register r;

	r = (int)(count%10);
	count /= 10;
	if (count)
		putd();
	putchr(r + '0');
}

puts(sp)
register char *sp;
{
	int sz,i;
	if (fss.Ffill && (listf == 0)) {
		if ((i = expnd(sp,funny,&sz,&fss)) == -1) {
			write(1,funny,fss.Flim & 0377); putchr('\n');
#ifndef NLS
			write(1,"too long",8);
#else NLS
	 		msgbuf = catgets(catd, NL_SETN, 72, "too long");
			write(1, msgbuf, strlen(msgbuf));
#endif NLS
		}
		else
			write(1,funny,sz);
		putchr('\n');
#ifndef NLS
		if (i == -2) write(1,"tab count\n",10);
#else NLS
		if (i == -2) {
	 		msgbuf = catgets(catd, NL_SETN, 73,"tab count\n");
			write(1, msgbuf, strlen(msgbuf));
		}
#endif NLS
		return(0);
	}

	col = 0;

#ifndef NLS16
	while (*sp)
		putchr(*sp++);
#else NLS16
	while (*sp)
		if (listf)
			putchr(CHARADV(sp));
		else
			putchr(*sp++&0377);
#endif NLS16

	putchr('\n');
}

char	line[70];
char	*linp = line;

putchr(ac)
{
	register char *lp;
	register c;
	short len;

	lp = linp;
	c = ac;
	if ( listf ) {

/*	Because this position 'col++' causes the bug for the l command.
**	Therefore, this statement is moved to the position 'out:' label.
*/

		if (col >= 72 && c != '\n') {
			col = 0;
			*lp++ = '\\';
			*lp++ = '\n';
		}

#ifdef NLS16
		if (c > 0377) {		/* 16bit character */
			*lp++ = ((c >> 8) & 0377);
			*lp++ = c & 0377;
			col++;
			goto out;
		}
#endif NLS16

		/*  POSIX.2 (P1003.2/D11.2):
		 *  replaced the historical backspace-overstrike method,
		 *  standardize the following non-printable characters.
		 */
		switch (c) {
		case '\\': 		/* backslash */
			*lp++ = '\\';
			*lp++ = '\\';
			goto out;
		case '\a':		/* <alert> */
			*lp++ = '\\';
			*lp++ = 'a';
			goto out;
		case '\b':		/* <backspace> */
			*lp++ = '\\';
			*lp++ = 'b';
			goto out;
		case '\f':		/* <form-feed> */
			*lp++ = '\\';
			*lp++ = 'f';
			goto out;
		case '\n':		/* <newline> */
			/* note that <newline> is interpreted to a line
			 * break. Instead, a '$' to printed as an indication
		 	 * of end of line
			 */ 
			*lp++ = '$';
			*lp++ = c;
			goto out;
		case '\r':		/* <carriage-return> */
			*lp++ = '\\';
			*lp++ = 'r';
			goto out;
		case '\t':		/* <tab> */
			*lp++ = '\\';
			*lp++ = 't';
			goto out;
		case '\v':		/* <vertical-tab> */
			*lp++ = '\\';
			*lp++ = 'v';
			goto out;
		}

#ifdef NONLS	/* Character set features */
		if (c<' ' && c!= '\n') {
#else NONLS
		if (! nl_isprint(c & 0377, currlangid()) && c != '\n') {
#endif NONLS

			*lp++ = '\\';

#ifdef NONLS	/* 8 bit integrity */
			*lp++ = (c>>3)+'0';
#else NONLS
			*lp++ = ((c>>6) & 03)+'0';
			*lp++ = ((c>>3) & 07)+'0';
#endif NONLS

			*lp++ = (c&07)+'0';

#ifdef NONLS	/* 8 bit integrity */
			col += 2;
#else NONLS
			col += 3;
#endif NONLS

			goto out;
		}
	}
	*lp++ = c;
out:
	col++;
	if(c == '\n' || lp >= &line[64]) {
		linp = line;
		len = lp - line;
		if(yflag & 01)
			write(1, &len, sizeof(len));
		write(1, line, len);
		return;
	}
	linp = lp;
}

globaln(k)
{
	register char *gp;
	register c;
	register LINE a1;
	int  nfirst, pr;
	char globuf[GBSIZE];

	if (yflag)
		error(59);
	if (globp)
		error(33);
	setall();
	nonzero();
	if ((c=getchr())=='\n')
		error(19);
	save();
	compile((char *) 0, expbuf, &expbuf[ESIZE], c);
	for (a1=zero; a1<=dol; a1++) {
		a1->cur &= ~01;
		if (a1>=addr1 && a1<=addr2 && execute(0, a1)==k)
			a1->cur |= 01;
	}
	nfirst = 0;
	newline();
	for (a1=zero; a1<=dol; a1++) {
		if (a1->cur & 01) {
			a1->cur &= ~01;
			dot = a1;
			puts(getline(a1->cur));
			if ((c=getchr()) == EOF)
				error(52);
			if(c=='a' || c=='i' || c=='c')
				error(53);

#ifdef NONLS	/* 8 bit integrity */
			c &= 0177;
#else NONLS
			c &= 0377;
#endif NONLS

			if (c == '\n') {
				a1 = zero;
				continue;
			}
			if (c != '&') {
#ifdef NLS16
				int secondbyte = FIRSTof2(c);
#endif NLS16
				gp = globuf;
				*gp++ = c;
				while ((c = getchr()) != '\n') {
#ifdef NLS16
					if (secondbyte)
						secondbyte = 0;
					else if (FIRSTof2(c))
						secondbyte++;
					else
#endif NLS16
					if (c=='\\') {
						c = getchr();
						if (c!='\n')
							*gp++ = '\\';
#ifdef NLS16
						if (FIRSTof2(c))
							secondbyte++;
#endif NLS16
					}
					*gp++ = c;
					if (gp >= &globuf[GBSIZE-2])
						error(34);
				}
				*gp++ = '\n';
				*gp++ = 0;
				nfirst = 1;
			}
			else
				if ((c=getchr()) != '\n')
					error(54);
			globp = globuf;
			if (nfirst) {
				globflg = 1;
				commands();
				globflg = 0;
			}
			else error(56);
			globp = 0;
			a1 = zero;
		}
	}
}
eopen(string, rw)
char *string;
{
#define w_or_r(a,b) (rw?a:b)
	int pf[2];
	int i;
	int io;

	int chcount;	/* # of char read. */
	int crflag;
	char *fp;

	crflag = 0;	 /* Is file encrypted flag; 1=yes. */

	if (rflg) {	/* restricted shell */
		if (Xqt) {
			Xqt = 0;
			error(6);
		}
	}
	if(!Xqt) {
		if((io=open(string, rw)) >= 0) {

			if (fflg) {

				chcount = read(io,funny,LBSIZE);

/* ========================================================================= */
/*
** CRYPT block 15
*/
#ifdef CRYPT
#ifdef NONLS	/* 8 bit integrity */
/* Verify that line just read IS an encrypted file. */

				fp = funny; /* Set fp to start of buffer. */
				while(fp < &funny[chcount])
					if(*fp++ & 0200)
						crflag = 1;

/* If is is encrypted, & -x option was used, & key is not null, decode it. */

				if(crflag & xflag & kflag)
#else NONLS
				if(xflag & kflag)
#endif NONLS

					crblock(perm, funny, chcount, (long)0);

#endif CRYPT
/* ========================================================================= */

				if (fspec(funny, &fss,0) < 0) {
					fss.Ffill = 0;
					fflg = 0;
					error(4);
				}
				lseek(io,0L,0);
			}

		}
		else  /* open failed */
			if (errno == EOPNOTSUPP)
				puts(catgets(catd, NL_SETN, 75,"Operation not supported on socket"));	
		fflg = 0;
		return(io);
	}
	if(pipe(pf) < 0)
xerr:		error(0);
	if((i = fork()) == 0) {
		signal(SIGHUP, oldhup);
		signal(SIGQUIT, oldquit);
		signal(SIGPIPE, oldpipe);
		signal(SIGINT, (int (*)()) 0);
		close(w_or_r(pf[1], pf[0]));
		close(w_or_r(0, 1));
		dup(w_or_r(pf[0], pf[1]));
		close(w_or_r(pf[0], pf[1]));
		execlp("/bin/sh", "sh", "-c", string, (char *) 0);
		exit(1);
	}
	if(i == -1)
		goto xerr;
	close(w_or_r(pf[0], pf[1]));
	return w_or_r(pf[1], pf[0]);
}

eclose(f)
{
#ifdef hpux
	fsync(f);
/*  Fail soft on the fsync since fsync is not supported over RFA   */
#endif hpux
	/* FSDlj09532:
	 * The exit value of close(f) is checked to detect if the any
	 * previous write to a NFS file system is full.
	 */
	if (close(f) != 0) {
		io = -1;
		if (errno == ENOSPC) 
			error(8);
		else
			error(29);
	}
	if(Xqt)
		Xqt = 0, wait((int *) 0);
}

mkfunny()
{
	register char *p, *p1, *p2;
	int	tflen ;	/* the length of the temporary file name	*/

	p2 = p1 = funny;
	p = file;
	while(*p)
		p++;
	while(*--p  == '/')	/* delete trailing slashes */
		*p = '\0';
	p = file;
	while (*p1++ = *p)
		if (*p++ == '/') 
			p2 = p1;
	tflen = strlen( tfname ) ;
	p1 = &tfname[tflen - 5];	/* get the process number	*/
	*p2 = '\007';	/* add unprintable char to make funny a unique name */
	while (p1 <= &tfname[tflen])
		*++p2 = *p1++;
}

getime() /* get modified time of file and save */
{
	if (stat(file,&Fl) < 0)
		savtime = 0;
	else
		savtime = Fl.st_mtime;
}

chktime() /* check saved mod time against current mod time */
{
	if (savtime != 0 && Fl.st_mtime != 0) {
		if (savtime != Fl.st_mtime)
			error(58);
	}
}

newtime() /* get new mod time and save */
{
	stat(file,&Fl);
	savtime = Fl.st_mtime;
}

red(op) /* restricted - check for '/' in name */
        /* and delete trailing '/' */
char *op;
{
	register char *p;

	p = op;
	while(*p)
		if(*p++ == '/'&& rflg) {
			*op = 0;
			error(6);
		}
	/* delete trailing '/' */
	while(p > op) {
		if (*--p == '/')
			*p = '\0';
		else break;
	}
}

char *fsp, fsprtn;

fspec(line,f,up)
char line[];
struct Fspec *f;
int up;
{
	struct termio arg;
	register int havespec, n;

	if(!up) clear(f);

	havespec = fsprtn = 0;
	for(fsp=line; *fsp && *fsp != '\n'; fsp++) {

#ifdef NLS16
		if (FIRSTof2(*fsp)) {		/* skip 16-bit character */
			if(!havespec) {
				fsp++;
				if (!*fsp || *fsp == '\n')
					break;
				continue;
			}
			return(-1);
		}
#endif NLS16

		switch(*fsp) {

			case '<':       if(havespec) 
						return(-1);
					if(*(fsp+1) == ':') {
						havespec = 1;
						clear(f);
						if(!ioctl(1, TCGETA, &arg) &&
							((arg.c_oflag&TAB3) == TAB3))
						  f->Ffill = 1;
						fsp++;
						continue;
					}

			case ' ':       continue;

			case 's':       if(havespec && (n=numb()) >= 0)
						f->Flim = n;
					continue;

			case 't':       if(havespec) targ(f);
					continue;

			case 'd':       continue;

			case 'm':       if(havespec)  n = numb();
					continue;

			case 'e':       continue;
			case ':':       if(!havespec)
						continue;
					if(*(fsp+1) != '>')
						fsprtn = -1;
					return(fsprtn);

			default:	if(!havespec)
						continue;
					return(-1);
		}
	}
	return(1);
}


numb()
{
	register int n;

	n = 0;
	while(*++fsp >= '0' && *fsp <= '9')
		n = 10*n + *fsp-'0';
	fsp--;
	return(n);
}


targ(f)
struct Fspec *f;
{
	register int n;

	if(*++fsp == '-') {
		if(*(fsp+1) >= '0' && *(fsp+1) <= '9')
			tincr(numb(),f);
		else
			tstd(f);
		return;
	}
	if(*fsp >= '0' && *fsp <= '9') {
		tlist(f);
		return;
	}
	fsprtn = -1;
	fsp--;
	return;
}


tincr(n,f)
int n;
struct Fspec *f;
{
	register int l, i;

	l = 1;
	for(i=0; i<20; i++)
		f->Ftabs[i] = l += n;
	f->Ftabs[i] = 0;
}


tstd(f)
struct Fspec *f;
{
	char std[3];

	std[0] = *++fsp;
	if (*(fsp+1) >= '0' && *(fsp+1) <= '9')  {
						std[1] = *++fsp;
						std[2] = '\0';
	}
	else std[1] = '\0';
	fsprtn = stdtab(std, f->Ftabs);
	return;
}


tlist(f)
struct Fspec *f;
{
	register int n, last, i;

	fsp--;
	last = i = 0;

	do {
		if((n=numb()) <= last || i >= 20) {
			fsprtn = -1;
			return;
		}
		f->Ftabs[i++] = last = n;
	} while(*++fsp == ',');

	f->Ftabs[i] = 0;
	fsp--;
}


expnd(line,buf,sz,f)
char line[], buf[];
int *sz;
struct Fspec *f;
{
	register char *l, *t;
	register int b;

	l = line - 1;
	b = 1;
	t = f->Ftabs;
	fsprtn = 0;

	while(*++l && *l != '\n' && b < 511) {
		if(*l == '\t') {
			while(*t && b >= *t) t++;
			if (*t == 0) fsprtn = -2;
			do buf[b-1] = ' '; while(++b < *t);
		}
		else buf[b++ - 1] = *l;
	}

	buf[b] = '\0';
	*sz = b;
	if(*l != '\0' && *l != '\n') {
		buf[b-1] = '\n';
		return(-1);
	}
	buf[b-1] = *l;
	if(f->Flim && b-1 > f->Flim) return(-1);
	return(fsprtn);
}


clear(f)
struct Fspec *f;
{
	f->Ftabs[0] = f->Fdel = f->Fmov = f->Ffill = 0;
	f->Flim = 0;
}
lenchk(line,f)
char line[];
struct Fspec *f;
{
	register char *l, *t;
	register int b;

	l = line - 1;
	b = 1;
	t = f->Ftabs;

	while(*++l && *l != '\n' && b < 511) {
		if(*l == '\t') {
			while(*t && b >= *t) t++;
			while(++b < *t);
		}
		else b++;
	}

	if((*l!='\0'&&*l!='\n') || (f->Flim&&b-1>f->Flim))
		return(-1);
	return(0);
}
#define NTABS 21

/*      stdtabs: standard tabs table
	format: option code letter(s), null, tabs, null */
char stdtabs[] = {
'a',    0,1,10,16,36,72,0,      		/* IBM 370 Assembler */
'a','2',0,1,10,16,40,72,0,      		/* IBM Assembler alternative*/
'c',    0,1,8,12,16,20,55,0,    		/* COBOL, normal */
'c','2',0,1,6,10,14,49,0,       		/* COBOL, crunched*/
'c','3',0,1,6,10,14,18,22,26,30,34,38,42,46,50,54,58,62,67,0,
'f',    0,1,7,11,15,19,23,0,    		/* FORTRAN */
'p',    0,1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,0, /* PL/I */
's',    0,1,10,55,0,    			/* SNOBOL */
'u',    0,1,12,20,44,0, 			/* UNIVAC ASM */
0};
/*      stdtab: return tab list for any "canned" tab option.
	entry: option points to null-terminated option string
		tabvect points to vector to be filled in
	exit: return(0) if legal, tabvect filled, ending with zero
		return(-1) if unknown option
*/

stdtab(option,tabvect)
char option[], tabvect[NTABS];
{
	char *scan;
	tabvect[0] = 0;
	scan = stdtabs;
	while (*scan)
		{
		if (strequal(&scan,option)) {
			strcopy(scan,tabvect);
			break;
		}
		else while(*scan++);    /* skip over tab specs */
		}

/*      later: look up code in /etc/something */

	return(tabvect[0]?0:-1);
}

/*      strequal: checks strings for equality
	entry: scan1 points to scan pointer, str points to string
	exit: return(1) if equal, return(0) if not
		*scan1 is advanced to next nonzero byte after null
*/

strequal(scan1,str)
char **scan1, *str;
{
	char c, *scan;

	scan = *scan1;
	while ((c = *scan++) == *str && c)
		str++;
	*scan1 = scan;
	if (c == 0 && *str == 0)
		 return(1);
	if (c)
		while(*scan++);
	*scan1 = scan;
	return(0);
}

/*      strcopy: copy source to destination */

strcopy(source,dest)
char *source, *dest;
{
	while (*dest++ = *source++);
	return;
}

/* This is called before a buffer modifying command so that the */
/* current array of line ptrs is saved in sav and dot and dol are saved */
save() {
	LINE i;

	savdot = dot;
	savdol = dol;
	for (i=zero+1; i<=dol; i++)
		i->sav = i->cur;
	initflg = 0;
}

/* The undo command calls this to restore the previous ptr array sav */
/* and swap with cur - dot and dol are swapped also. This allows user to */
/* undo an undo */
int
undo() {
	int tmp;
	LINE i, tmpdot, tmpdol;

	tmpdot = dot; dot = savdot; savdot = tmpdot;
	tmpdol = dol; dol = savdol; savdol = tmpdol;
	/* swap arrays using the greater of dol or savdol as upper limit */
	for (i=zero+1; i<=((dol>savdol) ? dol : savdol); i++) {
		tmp = i->cur;
		i->cur = i->sav;
		i->sav = tmp;
	}
}
