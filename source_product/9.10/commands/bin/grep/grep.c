static char *HPUX_ID = "@(#) $Revision: 72.2 $";
/*
 * grep/egrep/fgrep -- print lines matching (or not matching) a pattern
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 *
 *	June  1991:	Because of a defect in multibyte -i AND a 
 *			redefinition of POSIX case insensitivity,
 *			gen_d(), search16() and the macro csame were
 *			simplified.  The algorithm for case insensitivity
 *			is now:  if p is a pattern char and s is a string
 *			char, they match if p==_tolower(s)||p==_toupper(s).
 *			Randy Campbell.
 *	June  1990: 	More major revision: Randy Campbell
 *			The old fgrep algorithm is replaced with one
 *			based on Boyer-Moore string matching.  In 
 * 			connection with this, the basic data structure
 *			is changed to an array of structures, the
 *			reading of patterns from a -f file is changed to
 *			use the fgetl routine, getpat() is reduced in
 *			size, AND, a switchover mechanism is installed
 *			so that grep and egrep patterns which happen to
 *			contain no metacharacters are diverted to the
 *			faster B-M based fgrep.
 *
 *	October 1989:  	Major revision for POSIX.2 conformance
 *			Randy Campbell, UDL Fort Collins
 *
 *	Grep, egrep, fgrep now share one file, with fgrep and egrep
 *	installed as links.  Egrep and fgrep functionality are also
 *	accessed by -E and -F options to grep.
 *
 *	Because there are more options that affect the treatment of
 *	the expression(s) or string(s) input, I have added a routine
 *	getpat() that except in the simplest cases, gets the expression(s)
 *	and delivers them in suitable form to getcomp.  I have tried to
 *	maintain performance by minimizing conditionals in loops, therefore
 *   	getpat() is rather long.
 *
 *	The fgrep algorithm was imported essentially intact, but shortened
 *	to use the same I/O routine that grep/egrep use. The fgrep code now 
 *	lives in fexecute().
 *	The original (e)grep routine execute() was renamed egexecute() to
 *	differentiate it.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include	<locale.h>
#include	<nl_ctype.h>
#include	<nl_types.h>
nl_catd		catd;
#define		NL_SETN		1
#ifndef NLS
#define	catgets(i, j, k, s)	(s)
#define open_catalog()
#endif /* NLS */

#ifdef NLS16
#include <nl_ctype.h>
#include <langinfo.h>
#endif /* NLS16 */

#include <regex.h>
#include <limits.h>

char *errstr[] = {
	"Unknown regexp error code!!",				/*			catgets 101 */
	"Invalid collation element referenced.",		/* REG_ECOLLATE		catgets 102 */
	"Trailing \\ in pattern.",				/* REG_EESCAPE		catgets 103 */
	"\\n found in pattern.",				/* REG_ENEWLINE		catgets 104 */
	"Too many \\( \\) pairs or ( ) nested too deep.",	/* REG_ENSUB		catgets 105 */
	"Number in \\digit invalid or out of range.",		/* REG_ESUBREG		catgets 106 */
	"[ ] imbalance.",					/* REG_EBRACK		catgets 107 */
	"\\( \\) or ( ) imbalance.",				/* REG_EPAREN		catgets 108 */
	"\\{ \\} imbalance.",					/* REG_EBRACE		catgets 109 */
	"Invalid collation endpoint in [ - ] expression.",	/* REG_ERANGE		catgets 110 */
	"Regular expression overflow.",				/* REG_ESPACE		catgets 111 */
	"Number too large in \\{ \\} construct.",		/* REG_EABRACE		catgets 112 */
	"Invalid number in \\{ \\} construct.",			/* REG_EBBRACE		catgets 113 */
	"More than 2 numbers in \\{ \\} construct.",		/* REG_ECBRACE		catgets 114 */
	"First number exceeds second in \\{ \\}.",		/* REG_EDBRACE		catgets 115 */
	"Invalid character class named.",			/* REG_ECTYPE		catgets 116 */
	"No remembered search string.",				/* REG_ENOSEARCH	catgets 117 */
	"Duplication operator in illegal position.",		/* REG_EDUPOPER		catgets 118 */
	"Empty expression specified.",				/* REG_ENOEXPR		catgets 119 */
};

short	err_map[] = {	0,
			REG_ECOLLATE, REG_EESCAPE, REG_ENEWLINE, REG_ENSUB,
			REG_ESUBREG, REG_EBRACK, REG_EPAREN, REG_EBRACE,
			REG_ERANGE, REG_ESPACE, -1, REG_BADBR,
			-1, -1, REG_ECTYPE, REG_ENOSEARCH,
			REG_EDUPOPER, REG_ENOEXPR
};

#define	BLKSIZE	512
/* ARGNUM is used to dimension malloc'd space in getpat(), and also
 * for reading a -f file 
 */
#define ARGNUM  512	
#define CHARSET_SIZE	256  /* dimensions one of delta tables for B-M */

/* macros for some commonly used memory management, helps clean up
 * the code.
 */
#define Malloc(buf,size) \
    if ((buf = (u_char *)malloc(size)) == 0) \
	memerr()
#define Get_pat_list(size) \
    if((pat_list = (struct patt *)malloc(size * sizeof(struct patt))) == NULL) \
	memerr()
#define	Grow_pat_list(size) \
    if((pat_list = (struct patt *)realloc(pat_list, (size) * sizeof(struct patt))) == NULL)  \
	memerr()

regex_t	preg;


void 	open_catalog();
char	*strrchr(), *strchr();
u_char	*getpat();
FILE	*wordf, *fptr;
long	lnum;
/* buffer for line read from file */
u_char	prntbuf[LINE_MAX];
u_char	*command; 	    /* points to the name of grep/egrep/fgrep */
/*  The following flags affect output */
int	nflag;
int	bflag;
int	lflag;
int	cflag;
int 	qflag;
int	vflag;
int	sflag;
/* These flags affect interpretation of expressions */
int	iflag;
int	eflag;
int	fflag;
int 	xflag;
int	Eflag;
int 	Fflag;
/*  altflag signals special treatment of '|' in RE's to regcomp */
int	altflag;
int 	reg_flags = REG_NOSUB;	/*  Used to combine all flags for regcomp */
int 	nulflag;		/*  Set if a NULL expression is found */
int	nfile;
long	tln;
int	nsucc = 0;
int	nchars;
int	nlflag;

int	tot_size = 0;	/* used to track total size of patterns */
    /*
     * pat_cnt is incremented as soon as each pattern is read from
     * whatever source, so its value is the number of patterns
     * that we have.
     */
int	pat_cnt = 0;   

u_char *p;

long	blkno;
extern int errno;

/*
 *  The next few items depend on variables set by setlocale().  If the
 *  underlying mechanism ever changes, these may need to be redefined.
 */
extern int _nl_mb_collate; 	/* tells whether locale has M-B chars */

#ifdef NLS
#    define MULTIBYTE  (_nl_mb_collate)
#else
#    define MULTIBYTE 	0 
#endif /* NLS */


#ifdef NLS
u_char	usagebuf[400];
#endif /* NLS */

#define same(p, s)	(_toupper(p) == (s))  /* used when straight case tr */
#define csame(p, s)	((p)==_tolower(s)||(p)==_toupper(s)) 

/* variables and flags used to control switchover from grep to fgrep */
char *Gmetas = "^*[.$\\";
char *Emetas = "^*[|().{+$?\\";
int  metaflag = 0; 	    /* set if metacharacters are found */
int  allmetaflag = 1;       /* unset if one or more patterns have no metas */
#define	METAS	(short *)01  /* put in d_1 in [e]grep if no metas in pattern */

/*  Data structures used for tracking patterns and for B-M tables */
struct patt{
	u_char	*pattern;   /* pointer to the pattern */
	short	*d_1;	    /* pointer to delta(1) table */
	short 	*d_2;	    /*    "    "  delta(2) table */
	int 	patlen;	    /* length of pattern */
	};

struct patt *pat_list; 	    /* pointer to the array of structs */
/*
 *  See comments in gen_d() and search() for descriptions of how these
 *  tables are generated and used.
 */
short d_1[CHARSET_SIZE];    /* preallocate tables to be used in the common, */
short d_2[LINE_MAX];	    /* important case of a single pattern  */

extern int optind;
extern char *optarg;

main(argc, argv)
register argc;
u_char **argv;
{
	register	c;
	u_char 		*usage;
	int 		errflg = 0;
	int		errnum;
	int 		bufsize, num_pats, diff;
	u_char		*buf, *startbuf; 
	u_char		*pattern, *prevbuf;


#ifdef NLS
	if (!setlocale(LC_ALL,"")){
		fprintf(stderr, _errlocale(command));
		putenv("LANG=");
		catd = (nl_catd)-1;
	} 
#endif /* NLS */

	command = p = *argv;
	/* find part following last '/' */
	for (; *p; p++){
		if (*p == '/')
			command = p+1;
	}
	Eflag = (*command == 'e') ? REG_EXTENDED : 0;
	Fflag = (*command == 'f') ? 1 : 0;

/*  Usage message reflects change in possible invocations from POSIX */
#ifdef NLS
	strcpy(usagebuf,
	    catgets(catd,NL_SETN,1, "grep  [-E|-F] [-cbilnqsvx] [-e expression [-e expression] ... | -f file]\n [expression] [file ...]\n egrep [-cbilnqsvx] [-e expression [-e expression] ... | -f file]\n [expression] [file ...]\n fgrep [-cbilnqsvx] [-e expression [-e expression] ... | -f file]\n [expression] [file ...]"));
	usage = usagebuf;
#else
	usage = (u_char *)"grep  [-E|-F] [-cbilnqsvx] [-e expression [-e expression] ... | -f file]\n [expression] [file ...]\n egrep [-cbilnqsvx] [-e expression [-e expression] ... | -f file]\n [expression] [file ...]\n fgrep [-cbilnqsvx] [-e expression [-e expression] ... | -f file]\n [expression] [file ...]";
#endif /* NLS */

	/* Get space for the array of structs that will track all the
	 * patterns.  If all patterns are on command line, argc is an
	 * outer limit on how many there can be.  Patterns from -f file
	 * violate this, handled differently in option parsing 
	 */
	num_pats = argc;
	Get_pat_list(num_pats);   /* macro, call malloc, then zero fill */
	(void) bzero((char *)pat_list, num_pats*sizeof(struct patt));
	while ((c=getopt(argc, argv, "EFbcilnqsvxe:f:")) != EOF)
		switch (c){
		case 'E':
			Eflag++;  /* signals egrep behavior */
			break;
		case 'F':
			Fflag++;  /* signals fgrep behavior */
			break;
		case 'b':
			bflag++;
			break;
		case 'c':
			cflag++;
			break;
		case 'e':
			/* save pointer to each argument.  Total size and
			 * number to malloc expression space later 
			 */
			if (fflag) errflg++;
			eflag++;
			if(*optarg == NULL || *optarg == '\n')
			    nulflag++;
			/* if NULL pattern, we won't keep it around */
			else{
		    	    pat_list[pat_cnt].pattern = optarg;
			    pat_list[pat_cnt].patlen = strlen(optarg);
			    tot_size += pat_list[pat_cnt].patlen;
			    pat_cnt++;
			}
			break;
		case 'f':
			/* Open file once, read contents into pat_list. 
			 * handled later in getpat(). 
			 */
			if (eflag || fflag) errflg++;
			fflag++;

			/*  Assume argc dimension of pat_list won't be enough */
			free(pat_list);
			num_pats = ARGNUM;
			Get_pat_list(num_pats);
			(void) bzero((char *)pat_list,
				     num_pats*sizeof(struct patt));
			/* get some string space */
			Malloc(buf,_DBUFSIZ/2);
			startbuf = buf;
			if((fptr = fopen(optarg, "r")) == NULL){;
				open_catalog();
				fprintf(stderr,
				    catgets(catd, NL_SETN, 2, "%s: can't open %s\n"),
				    command, optarg);
				exit(2);
			}
			bufsize = _DBUFSIZ/2;
			while((nchars = fgetl(buf,LINE_MAX,fptr)) != 0){
			    if(nchars == 1)  	/* only a <nl> */
				nulflag++;
			    else{
				--nchars;
				buf[nchars] = '\0';  /* NULL-terminate it */
				pat_list[pat_cnt].pattern = buf;
				pat_list[pat_cnt].patlen = nchars;
				tot_size += nchars;
				buf += nchars + 1;   /* beginning of next pat */
				/* any pattern COULD be LINE_MAX long, so if
				 * we're less than LINE_MAX from the end of the
				 * buffer, get more 
				 */
				if(bufsize-(diff = buf - startbuf) < LINE_MAX){
				    bufsize += _DBUFSIZ/2;
				    prevbuf = startbuf;
				    if((startbuf = (u_char *)realloc(startbuf,bufsize)) == NULL)
					memerr();
				    buf = startbuf + diff;
				    /*
				     * If the string buffer was moved by the
				     * realloc, the pointers in pat_list need
				     * to move, too.
				     */
				    if(prevbuf != startbuf){
					long offset = startbuf - prevbuf;
					int i;
					
					for(i = 0; i <= pat_cnt; i++)
					    pat_list[i].pattern += offset;
				    }
			   	} 
				++pat_cnt;
				if(pat_cnt == num_pats){
				    num_pats += ARGNUM;
				    Grow_pat_list(num_pats);
				    (void) bzero((char *)&pat_list[pat_cnt],
						ARGNUM*sizeof(struct patt));
				}
			    }
			}
			/*
			 * Check did we read anything at all or was it
			 * an empty file?!
			 */
			if(tot_size == 0)
			    ++nulflag;
			fclose(fptr);
			/* 
			 * Shrink the allocated space back down to what was 
			 * actually used.  This is fairly cheap and 
			 * saves some memory.
			 * pat_cnt is added to the size for the character
			 * buffer because tot_size doesn't count the '\0's.
			 */
			startbuf = (u_char *)realloc(startbuf,(tot_size + pat_cnt));
			Grow_pat_list(pat_cnt + 1);   /* actually shrinks it */
						      /* no need zero fill   */	
			break;
		case 'i':
			iflag++;
			reg_flags |= REG_ICASE;
			break;
		case 'l':
			lflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'q':
			qflag++;  /* "quiet" flag, only report exit status */
			break;
		case 's':
			sflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		case '?':
			errflg++;
		}


	argc -= optind;
	if (errflg || ((argc <= 0) && !fflag && !eflag)){
		open_catalog();
		fprintf(stderr,
		    catgets(catd, NL_SETN, 3, "usage:\n %s\n"), usage);
		exit(2);
	}

	/* If no -e's or a -f, we have a single pattern as an argument */
	if ( !eflag  && !fflag){
    	    pat_list[pat_cnt].pattern = argv[optind++];
	    if( pat_list[pat_cnt].pattern == NULL)
	    	nulflag++;
	    else{

		/*  Check for embedded <nl>'s, break up into sep. patterns.
		 *  This can happen with either fgrep or egrep 
		 */
		while((p = (u_char *)strchr(pat_list[pat_cnt].pattern, '\n')) != NULL){

		    /*  Assign pattern length; check for NULL */
		    if((pat_list[pat_cnt].patlen = p - pat_list[pat_cnt].pattern) == 0){
			nulflag++;
			if(!xflag)
			    break;
		    }
		    tot_size += pat_list[pat_cnt].patlen;
		    *p++ = '\0';
		    pat_list[++pat_cnt].pattern = p;

		    /*  This can cause the argc number of slots to
		     *  be exceeded.  Have to realloc at 1 less than
		     *  num_pats because the above line can write past
		     *  the end of allocated space otherwise.
		     */
		    if(pat_cnt >= num_pats-1){
			num_pats += ARGNUM;
			Grow_pat_list(num_pats);
			(void)bzero((char *)&pat_list[pat_cnt+1],
					    ARGNUM*sizeof(struct patt));
		    }
		}
		if((pat_list[pat_cnt].patlen = strlen(pat_list[pat_cnt].pattern)) == 0)
		    nulflag++;
		tot_size += pat_list[pat_cnt++].patlen;
	    }
	    argc--;
    	}

	argv = &argv[optind];
	nfile = argc;


	if(!Fflag){
	    if((pattern = getpat()) != NULL){
	        if (Eflag)
	    	    reg_flags |= REG_EXTENDED;
	        if (altflag)
	    	    reg_flags |= _REG_ALT;
	    	    
	        if (errnum = regcomp(&preg, pattern, reg_flags))
	    	    regerr(errnum);
	    }
	}


	/* Execute this path if fgrep or some patterns didn't contain
	 * metacharacters 
	 */
	if (Fflag || !allmetaflag){
	    bm_setup();
	}

	/* Execute decides whether to call bm_exec() or regexec(), 
	 * depending on what's in the patterns 
	 */
	if (argc<=0)
		/*  Read from stdin */
		execute((u_char *)NULL);
	else{
		while ( --argc >= 0 ){
			execute(*argv);
			argv++;
		}
	}

	exit(nsucc == 2 ? 2 : nsucc == 0);
}

/***************
 *  execute() works with a single input file at a time.  It reads a line,
 *  then tries to match it with either regexec() or bm_exec(), or both,
 *  depending on how the program was called, what options were given,
 *  and what was in the patterns.
 *
 *  If the program was called as an fgrep, only bm_exec() is tried, no
 *  matter what else.  If grep or egrep, and some of the patterns
 *  have no RE/ERE metacharacters in them, bm_exec() is tried first 
 *  against the string patterns (preprocessed elsewhere).
 *  If grep or egrep and NONE of the patterns have metachars in them,
 *  again, only bm_exec() is used.
 ***************/
execute(file)
register u_char *file;
{
	register u_char *lbuf;
	u_char *filename;
	register i;

	if (file == NULL){
		wordf = stdin;
		filename = (u_char *)"-";
	}
	else if ( (wordf = fopen(file, "r")) == NULL){
		if (!sflag){
			open_catalog();
			fprintf(stderr,
			    catgets(catd, NL_SETN, 4, "%s: can't open %s\n"),
			    command, file);
		}
		nsucc = 2;
		return;
	}
	filename = file;

	lnum = 0;
	tln = 0;
	nlflag = 1;

	while ((nchars = fgetl(prntbuf, LINE_MAX, wordf)) != 0){
		if (nlflag)	/* only count a line when newline processed */
			lnum++;

		if (prntbuf[nchars-1] == '\n'){
			nlflag = 1;
			prntbuf[nchars-1] = '\0';
		} else
			nlflag = 0;

		lbuf = prntbuf;

		/*
		 *  Note bm_exec() and regexec() both return 0 for match 
		 */
		if(Fflag || !metaflag){
		    /* 
		     * we subtract 1 from nchars because fgetl counts the 
		     * newline, and bm_exec doesn't.  We leave nchars
		     * unchanged for use in fwrites, however.
		     */
		    if(((!bm_exec(prntbuf,nchars-1)) ^ vflag) && succeed(filename) == 1)
			break;
		} 	/* (Fflag || !metaflag) */

		else{	
		    if(allmetaflag){
		        if (((!regexec(&preg, lbuf,0,NULL,0)) ^ vflag) && succeed(filename) == 1)
			break;	/* lflag only once */
		    }
		    else{	/* Mix of meta, non-meta patterns */
			if((((!bm_exec(prntbuf,nchars-1)) || ((!regexec(&preg, lbuf,0,NULL,0)))) ^ vflag) && succeed(filename) == 1)
			    break;
		    }
		}

	}
	fclose(wordf);

	if (cflag){
		if (nfile>1)
			fprintf(stdout, "%s:", file);
		fprintf(stdout, "%ld\n", tln);
	}
	return;
}

succeed(f)
register u_char *f;
{
	nsucc = (nsucc == 2) ? 2 : 1;
	/*  qflag:  output nothing */
	if (qflag) return(1);
	if (cflag){
		tln++;
		return(0);
	}
	if (lflag){
		fprintf(stdout, "%s\n", f);
		return(1);
	}

	if (nfile > 1)	/* print filename */
		fprintf(stdout, "%s:", f);

	if (bflag)	/* print block number */
		fprintf(stdout, "%ld:", (ftell(wordf)-1)/BLKSIZE);

	if (nflag)	/* print line number */
		fprintf(stdout, "%ld:", lnum);

	if (nlflag)
		prntbuf[nchars-1] = '\n';

	fwrite(prntbuf, 1, nchars, stdout);

	if (!nlflag){
		/* long line--get and print remainder of it */
		while ((nchars = fgetl(prntbuf, LINE_MAX, wordf)) != 0){
			fwrite(prntbuf, 1, nchars, stdout);
			if (prntbuf[nchars-1] == '\n')
				break;	/* stop when we find the end */
		}
		nlflag++;	/* signal we've done the whole line */
	}

	return(0);
}

regerr(err)
register err;
{
	int	i;

	open_catalog();
	fprintf(stderr,
	    catgets(catd, NL_SETN, 5, "%s: RE error %d: "), command, err);
	for (i=sizeof(err_map)/sizeof(short)-1; i && err_map[i] != err; i--)
		continue;
	fprintf(stderr, "%s\n", catgets(catd, NL_SETN, i+101, errstr[i]));
	exit(2);
}

/*
 * The following code is a modified version of the fgets() stdio
 * routine.  The reason why it is used instead of fgets() is that
 * we need to know how many characters we read into the buffer.
 * Thus that value is returned here instead of the value of s1.
 */
#define MINIMUM(x, y)	(x < y ? x : y)
#define _BUFSYNC(iop)	if (_bufend(iop) - iop->_ptr <   \
				( iop->_cnt < 0 ? 0 : iop->_cnt ) )  \
					_bufsync(iop)

extern int _filbuf();
extern char *memccpy();

fgetl(ptr, size, iop)
u_char *ptr;
register int size;
register FILE *iop;
{
	u_char *p, *ptr0 = ptr;
	register int n;

	for (size--; size > 0; size -= n){
		if (iop->_cnt <= 0){ /* empty buffer */
			if (_filbuf(iop) == EOF){
				if (ptr0 == ptr)
					return (NULL);
				break; /* no more data */
			}
			iop->_ptr--;
			iop->_cnt++;
		}
		n = MINIMUM(size, iop->_cnt);
		if ((p = (u_char *)memccpy(ptr, (u_char *) iop->_ptr, '\n', n)) != NULL)
			n = p - ptr;
		ptr += n;
		iop->_cnt -= n;
		iop->_ptr += n;
		_BUFSYNC(iop);
		if (p != NULL)
			break; /* found '\n' in buffer */
	}
	*ptr = '\0';
	return (ptr-ptr0);
}

/***********************************
 *  getpat() is used with regular grep or -E (egrep).
 *  It gets the pattern(s) from either the command line
 *  or a file and puts all of them together in a buffer
 *
 *	Takes no arguments, operates on global data structure, pat_list.
 *	Returns either NULL or a pointer to malloc'd space.
 *
 *	This routine contains a lot of segments of very similar, but
 *	not identical code.  This was done to avoid degrading performance
 *	by asking questions inside loops.  The intent is to figure out
 *	with only 2 or 3 conditionals what processing is required of
 *	the pattern(s) received, then go straight through and do it.
 *
 *	Since -x can be used with any other options, anchoring symbols,
 *      '^' and '$' must be affixed to the beginning and end of each
 *	expression if the xflag is set.
 *
 *	All patterns are scanned for RE metacharacters.  Those containing
 * 	any are so marked (by setting METAS in d_1), and processed
 *	here.  The others will be preprocess by bm_setup() and run as fgrep
 *	strings.
 *
 **********************************/
u_char *
getpat()
{

	u_char	c;
	int	nl = 0;
	char	*metas, *m;  /* used in scanning for meta chars */
	int 	index, tmpmeta;
	u_char	*pattern, *linebuf, *inpbuf;
	int 	non_bm = 0;
	int 	bars = 0;

	if(Eflag)
	    metas = Emetas;
	else
	    metas = Gmetas;

	/* check patterns for metacharacters */
	for(index = 0; index < pat_cnt; index++){
	    m = metas;
	    tmpmeta = 0;
	    do{
		if(strchr(pat_list[index].pattern,*m) != NULL){
		    tmpmeta = metaflag = 1;
		    pat_list[index].d_1 = METAS;
		    ++non_bm;
		    break;
		}
	    } while(*++m != '\0');
	    /*
	     * On regular grep, we are going to escape literal '|'s
	     * in the pattern.  We count them in order to allow space
	     * for the '\' characters.
	     */
	    if (tmpmeta && !Eflag) {
		inpbuf = pat_list[index].pattern;
		while ((c = *inpbuf++) != '\0')
			if (c == (u_char) '|') bars++;    /* count bars */
	    }
	    /* See if we still think all the patterns have metachars 
	     * If this one didn't, kill allmetaflag 
	     */
	    if(allmetaflag && !tmpmeta)
		allmetaflag = 0;
	}

	/*
	 * If none of the patterns had RE metacharacters, no need
	 * to process any of them here.  Return NULL and regcomp()
	 * will be bypassed.
	 */
	if(!metaflag)	  	/* no patterns had metacharacters */
	    return(NULL);	/* They will all go to fgrep */
	/* 
	 * A simple, likely case - a single pattern w/o anchoring 
	 * No further processing needed for regcomp() 
	 */
	if(!xflag && pat_cnt == 1) 
	    return(pat_list[0].pattern);
	  

	/*
	 * malloc space for the pattern to be passed to regcomp.
	 * We use tot_size (cumulative pattern size).
	 * Plus up to 3 per pattern, 5 if xflag, for extra chars
	 * to be inserted ("$|^",etc).
	 * Plus up to 2 used at the very beginning.
	 * Plus 1 for the null at the very end.
	 * Plus 1 for each literal '|' to be escaped, counted above as "bars". 
	 * (Note this last will always be zero for egrep).
	 */
	Malloc(linebuf,tot_size + (non_bm * (xflag ? 5 : 3)) + 3 + bars);
	pattern = linebuf;
	/*
	 * Sometimes the same space has been used before.  Starting out
	 * with a null character helps strcpy work right.
	 */
	*linebuf = '\0';
	if (Eflag){		/* expect ERE's */
    	    if (!xflag){	/*  no anchoring  */
   		for(index = 0; index < pat_cnt; index++){
    		    if(pat_list[index].d_1 == METAS){
			strcpy(pattern, pat_list[index].pattern);
			*(pattern += pat_list[index].patlen) = '|';
			*++pattern = '\0';	/* so strcpy works right */
		     }
		 }
		 *--pattern = '\0';
	    }
	    /* (Eflag, xflag */
	    else {		/* xflag, need to anchor */
	        *pattern++ = '^';
		*pattern++ = '(';
	        for(index = 0; index < pat_cnt; index++){
	            if(pat_list[index].d_1 == METAS){
			strcpy(pattern, pat_list[index].pattern);
			strcpy((pattern += pat_list[index].patlen),")$|^(");
			pattern += 5;
		    }
		 }
		 *(pattern -= 3) = '\0';
	    }
	}
	/* (!Eflag) */
	else{		/* regular grep, expect RE's */
	    altflag++;
	    if (!xflag){	/*  no anchoring  */
	        for(index = 0; index < pat_cnt; index++){
	            if(pat_list[index].d_1 == METAS){
			inpbuf = pat_list[index].pattern;
			while ((c = *inpbuf++) != '\0'){
			    if (c == '|')    /* literal bar, escape it */
			        *pattern++ = '\\';
			    *pattern++ = c;
			}
			*pattern++ = '|';
		     }
		 }
		 *--pattern = '\0';
	    }
	    /* (!Eflag, xflag) */
	    else {		/* xflag, need to anchor */
		*pattern++ = '^';
	        for(index = 0; index < pat_cnt; index++){
	            if(pat_list[index].d_1 == METAS){
			inpbuf = pat_list[index].pattern;
			while ((c = *inpbuf++) != '\0'){
			    if (c == '|')    /* literal bar, escape it */
			        *pattern++ = '\\';
			    *pattern++ = c;
			}
			*pattern++ = '$';
			*pattern++ = '|';
			*pattern++ = '^';
		     }
		}
		*(pattern -= 2) = '\0';
 	    }
	}

	return(linebuf);
}

/******************
 * bm_setup() compiles the delta(1) and delta(2) arrays for all the
 * patterns that have been found.  An exception is if this was called
 * as grep/egrep and some of the patterns have metacharacters so they
 * need to be handled by regcomp and regexec.
 ******************/
bm_setup()
{
	int 	index;
	short	*d_1_space, *d_2_space;
	int 	d_2_size = tot_size * sizeof(short);
	
	/* 
	 * Likely common case of single pattern, we use the 
	 * preallocated global d_1 
	 * Otherwise, malloc space, using information available 
	 * This grabs memory for the d_1 table. As patterns are 
	 * processed, we just advance through the allocated space 
	 */
	if(pat_cnt == 1 && pat_list[0].d_1 != METAS)
	    d_1_space = d_1;
	else
	    if((d_1_space = (short *)malloc(pat_cnt * CHARSET_SIZE * sizeof(short)))== NULL)
		memerr();

	/*
	 *  If the total pattern size is less than the preallocated
	 *  array d_2, we can use it, even for multiple patterns.
	 *  Otherwise malloc space based on the known total size of
	 *  all patterns.  We advance through it, same as the d_1 space.
	 */
	if(d_2_size <= sizeof(d_2))
		d_2_space = d_2;
	else
	    if((d_2_space = (short *)malloc(tot_size * sizeof(short))) == NULL)
		memerr();
	

	/* Run through all of patterns and compile tables */
	for(index = 0; index < pat_cnt; index++){
	    /* skip patterns reserved for regexec() */
	    if(pat_list[index].d_1 == METAS)
		continue;
	    else{ 	/* We'll process it */
	        pat_list[index].d_1 = d_1_space;
	        pat_list[index].d_2 = d_2_space;

		/* gen_d() fills in the tables */
		gen_d(pat_list[index].pattern, pat_list[index].d_1,
		      pat_list[index].d_2, pat_list[index].patlen);
		d_1_space = pat_list[index].d_1 + CHARSET_SIZE;
		d_2_space = pat_list[index].d_2 + pat_list[index].patlen;
	    }
	}

}


/******************
 * bm_exec() runs through all the patterns against a line of text
 * until it finds a match or runs out of patterns.
 * It takes for arguments the line to be matched "string", and the
 * length of the string "slen"; the length is needed by the algorithm,
 * and we have it from fgetl, so why recalculate it?
 * Before calling the actual match routines, search() (or search16()),
 * A couple of special cases are checked, NULL patterns and NULL
 * patterns with anchoring set (-x).
 *******************/
bm_exec(string,slen)
u_char *string;
int    slen;
{
	int	index;

	/*
	 * If one or more patterns were NULL (empty), all lines match
	 * unless -x given, in which case empty lines match.
	 */
	if(nulflag && (!xflag || slen == 0))
	    return(0);

	/* Now try all the compiled patterns ...  */
	for(index = 0; index < pat_cnt; index++){
	    /* ...except those with metacharacters for grep/egrep */
	    if(pat_list[index].d_1 == METAS)
		continue;
	    /* if xflag on, length of pattern must match that of string */
	    if(xflag && pat_list[index].patlen != slen)
		continue;
	    
	    if(MULTIBYTE){
		if(search16(&pat_list[index],string,slen))
		    return(0);
	    }
	    else	/* not M-B */
		if(search(&pat_list[index],string,slen))
		    return(0);
	}

	/* If we fell out here, there was no match */
	return(1);
}		    
		

/*****************
 *  gen_d() is  the code that compiles the delta tables used by Boyer-
 *  Moore.  They are shift tables that tell how far the pattern can
 *  be shifted to the right when a match fails.
 *  d_1 is 256 bytes, and is based on the rightmost reocurrence of
 *  a character in the pattern.
 *  d_2 is the length of the pattern, and uses knowledge of substrings
 *  that have already been matched.
 *  The one added strangeness to this is the handling of the -i ignore
 *  case.  All pattern characters AND some of their case counterparts 
 *  get filled in with the shift distance. 
 *  The POSIX.2 definition for case insensitivity says that, give pattern
 *  character, p, and string character, s, they match if:
 *
 *	p == toupper(s) || p == tolower(s)
 *
 *   So, given a character like <o-umlaut>, the d_1 table needs the same 
 *  shift value for the character itself, and <O-umlaut> because 
 *  tolower(<O-umlaut>) == <o-umlaut>, but not for 'O' because 
 *  tolower('O') != <o-umlaut> and toupper('O') != <o-umlaut>.
 *  Since the shift tables don't provide an easy way to to find out from
 *  <o-umlaut> that it is the lowercase of <O-umlaut> a table is constructed
 *  giving the "reverse" case mappings.  This is pretty strongly predicated
 *  on the assumption that there are many-to-one lower-to-upper mappings
 *  (i.e., toupper('o') == 'O' and toupper(<o-umlaut>) == 'O') but only 
 *  one-to-one upper-to-lower mappings.  If a locale turns up for which this
 *  assumption does not hold, this code will have to be changed.
 *
 ****************/
#define	min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))

gen_d(pattern,d_1, d_2, patlen)
u_char *pattern;
short 	*d_1, *d_2;
int	patlen;

{

	short f[LINE_MAX]; /* could get as large as a line */
	int c,i,j,k,l,m,t;
	int tp;
	u_char case_list[CHARSET_SIZE];  /* for unobvious case counterparts */

	/* patlen is the actual pattern length, m is the pattern length
	 * less one to help with 0-based indexing 
	 */
	m = patlen - 1;

	/*  First fill in d_1 table with patlen.  This is the default
	 *  shift for characters that are not in the pattern 
	 */
	for(j = 0; j < CHARSET_SIZE; j++){
	    d_1[j] = patlen;
	    case_list[j] = 0;
	}

	if(!iflag){
	    /*
	     * This loop fills in the jump distance in the d_1 table for
	     * the chars that are in the pattern, as their distance from
	     * the end of the pattern.
	     *
	     * Loop 1, d_2:
	     *
	     * The d_2 table is initially filled on the assumption
	     * that at any failure, we want to shift the pattern down
	     * completely past itself and shift the string index all the 
	     * way to the end.  This assumption is expressed as twice the
	     * pattern length - 1 - k  (k is the index into the pattern).
	     */
	    for(k = 0; k <= m; k++){
	        d_1[pattern[k]] = m - k;
		/*
		 *  Loop 1, d_2:
		 */
	        d_2[k] = 2 * m - k + 1; /* the "+ 1" is for zero-based index */
	    }

	    /*
	     *  Loop 2, d_2:
	     *
	     *  This loop backs through the pattern finding matching substrings
	     *  and recording the distances between them in the d_2 table.
	     *  For each substring, j and t point to the characters to the
	     *  left of the substrings:
	     *
	     *	pattern:   a b c d e b c
	     *                 ^       ^
	     *                 j       t
	     *
	     *  Shift distances are calculated to bring pattern[j] over 
	     *  pattern[t] if a failure occurs on t.
	     */
	    j = m;
	    t = m + 1;
	    while( j >= 0){
	    	f[j] = t;
	    	while(t <= m && pattern[j] != pattern[t]){
		    d_2[t] = min(d_2[t],m - j);
		    t = f[t];
	    	}
	    	t = t - 1;
	    	j = j - 1;
	    }
	}

	/* Here's where we do the case insensitive stuff.
	 * this is separated so the more usual stuff doesn't pay the
	 * price for this 
	 */
	else{ 	/* iflag */
	    /*
	     *  Each char and its case counterpart need to get the shift value.
	     *  Special arrangements need to be made for the corner case of
	     *  characters like <a-acute>, whose case counterpart is <A>,
	     *  but needs to match <A-acute> as well (because <A-acute> 
	     *  downshifts to <a-acute>.
	     */
	    for(i = 0; i < patlen; i++){
		for(j = 0; j < CHARSET_SIZE; j++){
		    if(_tolower(j) == pattern[i])
			case_list[pattern[i]] = j;
		}
	    }

	    for(k = 0; k <= m; k++){
	        d_1[_tolower(pattern[k])] = d_1[pattern[k]] = m - k;
		if(case_list[pattern[k]])
		    d_1[case_list[pattern[k]]] = m - k;
	        /*
	         *  Loop 1, d_2:
	         */
	        d_2[k] = 2 * m - k + 1; /* the "+ 1" is for zero-based index */
	    }

	    /*
	     *  Loop  2, d_2:
	     *
	     *  Nothing changes for case except using the macro, csame,
	     *  for character compares so we don't miss substrings
	     *  such as "cb<a-acute>" and "cb<A-acute>", which match 
	     *  in case insensitive context.
	     */
	    j = m;
	    t = m + 1;
	    while( j >= 0){
	    	f[j] = t;
	    	while(t <= m && ! csame(pattern[j],pattern[t])){
		    d_2[t] = min(d_2[t],m - j);
		    t = f[t];
	    	}
	    	t = t - 1;
	    	j = j - 1;
	    }
	}

	/*
	 *  Loop 3, d_2:
	 *
	 *  In the second loop above, no change is made to the default
	 *  values of d_2 between 0 and t.  
	 *  This loop adjusts those values, recognizing that if a match
	 *  fails to the left of t, the max we want to shift is far
	 *  enough to bring the first character of the pattern over the 
	 *  current position.  
	 */
	for(k = 0; k <= t; k++)
	    d_2[k] = min(d_2[k],m + t - k + 1 ); /* +1 for 0-based index */

	/*
	 *  Loop 4, d_2:
	 *
	 *  The three loops run so far in calculating d_2 were written 
	 *  directly from psuedo-code by Knuth.  However Knuth apparently
	 *  left a hole, in that with certain kinds of sub-sub-strings,
	 *  d_2 values to the right of t may not be recalculated in 
	 *  loop 2, and loop 3 only goes up to t.
	 *  In this example:
	 *
	 *	b z a b y a b z a b
	 *                1 1   1 1
	 *	2 2 2 2     2 2 2 2
	 *
	 *  there are overlapping substrings, ab and bzab.  In the loop 2
	 *  calculation, the shift value for b does not get changed from
	 *  the default because of where it falls in the pattern.
	 *
	 *  Loop 4 checks for such skips between the final value of t and
	 *  the end of the pattern.
	 */
	if((tp = f[t]) <= m){
	    while( t <= m){
		d_2[t] = min(d_2[t],max(tp - t + m + 1,m + 1));
		++t;
	    }
	}
}

/************************
 *  search() and search16() carry out the Boyer-Moore search for the 
 *  pattern in the string.
 *  The basic idea of the search is to start at the end of the pattern
 *  and the matching position in the string and work backwards.  If we
 *  get all the way to the beginning of the pattern, it's a match.
 *  If the characters don't match at any point, the delta tables, d_1
 *  and d_2 tell us how far down the string we can shift the pattern.
 *  See the comments in gen_d() for a description of how these tables
 *  are generated.
 *
 *  The d_1 table is indexed by the value of the string character, and
 *  tells how far right to shift to bring the next occurrence of that
 *  character in the pattern over the current occurrence in the string.
 *   
 *  The d_2 table is indexed by the index of the current position in 
 *  the pattern.  It tells how far right to shift the pattern to bring
 *  over the next occurrence of the substring that has already been
 *  matched.
 * 
 *  The largest of d_1[string_character] or d_2[pattern_index] is selected
 *  for the next shift.
 *
 *  The algorithm is considered sub-linear because it is usually not
 *  necessary to compare every character in the string, and the longer
 *  the pattern is, the faster the search proceeds.
 ************************/
search(pat,string,slen)
struct patt 	*pat;
u_char 		*string;
int		slen;
{
	
	int 	i,pl,j;
	u_char	uc;
	u_char	*pattern = pat->pattern;
	short	*d_1 = pat->d_1;
	short	*d_2 = pat->d_2;


	/* make this smaller by one to compensate for zero-based indexing */
	pl = pat->patlen - 1;
	i = j = pl;

	/*  If case-insensitive, both the pattern character and all
	 *  of its case equivalents have been given shift values 
	 *  in d_1. 
	 *  Thus if the pattern character is 'A', both 'A' and 'a'
	 *  get the same shift values in the shift table.
	 */
	if(iflag){
	    while(i < slen){
	        while(csame(pattern[j],string[i])){
	    	    if(--j < 0)
	    	        return(1);
	            i--;
	        }
	        i += max(d_1[_tolower(string[i])],d_2[j]);
	        j = pl;
	    }
	}
	else{	/* no case shifting */
	    while(i < slen){
	        while(string[i] == pattern[j]){
	    	    if(--j < 0)
	    	        return(1);
	            i--;
	        }
	        i += max(d_1[string[i]],d_2[j]);
	        j = pl;
	    }
	}
	return(0);
}

/*******************
 *  In the search16() routine, we treat every string as if it were 
 *  single-byte chars until a match occurs.  Then we go back to the 
 *  last known character boundary and come forward to see if the pattern 
 *  is lined up on a byte boundary on the string.  If so, match, if not, 
 *  move pattern over one byte and restart the loop.
 *******************/
search16(pat, string, slen)
struct patt	*pat;
u_char 		*string;
int		slen;
{
	u_char 	*last_2byte = string;
	int 	i,j, pl;
	u_char 	uc;
	u_char	*pattern = pat->pattern;
	short	*d_1 = pat->d_1;
	short	*d_2 = pat->d_2;

	/* make this one smaller to compensate for zero-based indexing */
	pl = pat->patlen - 1;
	i = j = pl;

	if(iflag){	/* same stuff as in search */
	    while(i < slen){
	        while(csame(pattern[j],string[i])){
	     	    if(--j < 0){
	    	        /* possible match, but must verify */
	    	        while(last_2byte < &string[i])
	    		    ADVANCE(last_2byte);
	    	        if(last_2byte == &string[i])  /* lined up exactly */
	    		    return(1);
	    	        else{
	    		    /* 
			     * have to increment i to go past last place 
			     */
	    		    i += pl + 1;
	    		    if(i >= slen)
	    		        return(0);
	    		    j = pl;
	    		    continue;
	    	        }
	    	    }
		    else
		        i--;
	        }
	        i += max(d_1[string[i]],d_2[j]);
	        j = pl;
	    }
	}

	else{	/* Normal processing */
	    while(i < slen){
	        while(string[i] == pattern[j]){
	     	    if(--j < 0){
	    	        /* possible match, but must verify */
	    	        while(last_2byte < &string[i])
	    		    ADVANCE(last_2byte);
	    	        if(last_2byte == &string[i])  /* lined up exactly */
	    		    return(1);
	    	        else{
	    		    /* 
			     * have to increment i to go past last place 
			     */
	    		    i += pl + 1;
	    		    if(i >= slen)
	    		        return(0);
	    		    j = pl;
	    		    continue;
	    	        }
	    	    }
		    else
		        i--;
	        }
	        i += max(d_1[string[i]],d_2[j]);
	        j = pl;
	    }
	}
	return(0);
}


memerr()
{
	fprintf(stderr,"%s: not enough memory", command);
	exit(2);
}

#ifdef NLS
void
open_catalog()
{
    static int called = 0;
    int err;

    if (!called)
    {
	err = errno; /* Preserve errno value since we don't use this anyway */
	catd = catopen("grep", 0);
	errno = err;
	called = 1;
    }
}

#endif /* NLS */
