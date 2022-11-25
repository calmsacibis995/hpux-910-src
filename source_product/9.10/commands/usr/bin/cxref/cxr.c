#ifdef __hp9000s300
static char *HPUX_ID = "@(#) B.08.00 LANGUAGE TOOLS Internal $Revision: 70.1.1.2 $";
#else
static char *HPUX_ID = "@(#) $Revision: 70.1.1.2 $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#ifdef DEBUGGING
#   define OPTSTR "p:cdibTexhvYA:D:U:I:W:w:tsgOZzSGE#o:"
#else    
#   define OPTSTR "p:chvYA:D:U:I:W:w:tsgOZzSGE#o:"
#endif
#ifdef OSF
#ifdef PAXDEV
#define OWNERBIN "/paXdev/lbin/"
#define LIB      "/paXdev/lbin/"
#else
#define OWNERBIN "/usr/ccs/lib/"
#define LIB      "/usr/ccs/lib/"
#endif /* PAXDEV */
#else
#define OWNERBIN "/usr/lib/"
#define LIB      "/lib/"    
#endif /* OSF */

#define MAXWORD (BUFSIZ / 2)
#define MAXRFW MAXWORD
#define MAXRFL MAXWORD

#define MAX	BUFSIZ	/* maximum length of line */
#define BASIZ	50	/* funny */
#define LPROUT	70	/* output */
#define TTYOUT	30	/* numbers */
#define YES	1
#define NO	0
#define LETTER	'a'
#define DIGIT	'0'
#ifdef OSF
#define XCPP	"cpp.trad"
#else
#define XCPP	"cpp"
#endif /* OSF */
#define XCPPANSI "cpp.ansi"
#define XREF	"xpass"
#define XREFANSI "xpass.ansi"	
#define SRT	"/bin/sort"

#define Xcp     0
#define Xc0     1       

/* Extra arg space to save as a fixed overhead.  This is for "overhead"
 * args used in the call, and for -W args which could expand to several
 * args.
 * For xcpp, the current number of overhead args is 4: ("xcpp", "-F",
 * tmp2, tmp1).
 * For xpass, the current number of overhead args is 6: ("xpass", :-f",
 * "-i", tmp1, "-o", tmp2).
 * Note that because of -W args, it is still possible that the arg arrays
 * could overflow.
 */
#define NARG0	128
#define NPASS   2
#ifdef OSF
#ifdef PAXDEV
char	*nameps[NPASS] = {XCPP,XREF};
#else
char	*nameps[NPASS] = {XCPPANSI,XREFANSI};
#endif /* PAXDEV */
#else
char	*nameps[NPASS] = {XCPP,XREF};
#endif /* OSF */
/*
 * define a structure which records all the data for a pass and
 * allocate one for each pass
 */
struct PASS {
 char *path;		/* the path to the piece */
 char *name;		/* the name of the piece */
 unsigned alt      :1;	/* 1 -> user alternate pass in /usr/lib, /usr/bin */
 unsigned usepath :1;	/* 1 -> use the path for creating full pathname */
 unsigned nameset :1;	/* 1 -> set by user with -t, 0 -> use default */
 unsigned pathset :1;	/* 1 -> new path was provide by -t, 0 -> use default */
} passes[NPASS];

extern char
	*calloc(),
	*strcat(),
	*strcpy(),
	*mktemp(),
	*strtok(),
	*strchr();

char	*xref,	/* path to the cross-referencer */
	*xcpp,	/* path to cxref pre-processor */
	*cfile,	/* current file name */
	*tmp1,	/* contains preprocessor result */
	*tmp2,	/* contains xcpp and xpass cross-reference */
	*tmp3,	/* contains transformed tmp2 */
	*tmp4;	/* contains sorted output */

char	*clfunc = "   --   ";	/* clears function name for ext. variables */

char	*myalloc(),
        *copy();

char	xflnm[MAXWORD],	/* saves current filename */
	funcname[MAXWORD],	/* saves current function name */
	sv1[MAXRFL],	/* save areas used for output printing */
	sv2[MAXRFL],
	sv3[MAXRFL],
	sv4[MAXRFL],
	buf[BUFSIZ];

char	**plist,	/* argv pointer for xcpp call */
	**clist,	/* argv pointer for xpass call */
        **srclist;	/* list of source files */

int	narg;		/* number of args to cxref */
int	cnarg,		/* argcount for clist */
	pnarg;		/* argcount for plist */
int     srcarg;		/* number of src files to process */
int	cmaxarg,	/* max argcount for clist */
	pmaxarg;	/* max argcount for plist */


int	cflg = NO,	/* prints all given files as one cxref listing */
	silent = NO,	/* print filename before cxref? */
	tmpmade = NO,	/* indicates if temp file has been made */
	inafunc = NO,	/* in a function? */
	fprnt = YES,	/* first printed line? */
	addlino = NO,	/* add line number to line?? */
	hflag = YES,	/* print header? */
	verbose = NO,	/* verbose flag : print call strings to stderr */
	nlsflg = NO,	/* enable NLS support */
        debug   = NO,	/* cxr debug flag */
	ddbgflg = NO,	/* debugging? */
	idbgflg = NO,
	bdbgflg = NO,
	tdbgflg = NO,
	edbgflg = NO,
	xdbgflg = NO,
#ifdef OSF
#ifdef PAXDEV
        ansiflg = NO,   /* true is -Aa is used, false for -Ac, default */
#else
        ansiflg = YES,  /* true is -Aa is used, false for -Ac, default */
#endif /* PAXDEV */
#else
        ansiflg = NO,   /* true is -Aa is used, false for -Ac, default */
#endif /* OSF */
	LSIZE = LPROUT,	/* default size */
	lsize = 0,	/* scanline: keeps track of line length */
	sblk = 0,	/* saves first block number of each new function */
	fsp = 0,	/* format determined by prntflnm */
	defs = 0,	/* number of defines */
	undefs = 0,	/* number of undefines */
	incs = 0,	/* number of '-I' directories */
	nerrs = 0,	/* errors encountered along the way */
	np = 0,		/* number of arguments to be passed to xcpp */
	nc = 0;		/* number of arguments to be passed to xpass */

int	nword,	/* prntlino: new word? */
	nflnm,	/* new filename? */
	nfnc,	/* new function name? */
	nlino;	/* new line number? */

main(argc,argv)
	int argc;
	char *argv[];
{
	char *t, *sub;
	char *ccopts;			/* options, before cmd line */
	char *ccopts1;
	char *ccopts2;
	char *ccoptsAfter;		/* options, after cmd line */
	int  i,j;
	extern int optind;
	extern char *optarg;
	extern int optopt;	/* (undocumented) value of the illegal option when getopt returns a '?' */
	int c;
	char ic;
	char *npassname;

	mktmpfl();	/* make temp files */

# ifdef PARSE_CCOPTS	/* currently turned off */
	/* check for environment variable CCOPTS  */
	ccopts = (char *) getenv("CCOPTS");
	ccopts2 = ccopts1 = NULL;
	if (ccopts && (strlen(ccopts) > 0)) {
	    ccoptsAfter = ccopts;
	    while ((ccoptsAfter = strchr(ccoptsAfter, '|')) != NULL) {
	       if ((*(ccoptsAfter - 1)) == '\\') {
		  /* step past the escaped | */
		  ++ccoptsAfter;
	       } else {
	          /* found an unescaped |, add all options after */
	          *ccoptsAfter++ = '\0';
		  ccopts2 = copy(ccoptsAfter);
		  t = strtok(ccoptsAfter, " ");
		  j = argc;
		  while (t ){
 		    argc ++;
		    i = argc * sizeof(char *);
		    argv = (char **) realloc(argv, (unsigned) i);
		    argv[j++] = t;
		    t = strtok(NULL, " ");
	          }
	          break;
	       }
	    }
	    ccopts1 = copy(ccopts);
	    t = strtok(ccopts, " ");
	    j = 1;
	    while (t ){
		argc ++;
		i = argc * sizeof(char *);
		argv = (char **) realloc(argv, (unsigned) i);
		for (i = argc - 1; i > j; i--)
		     argv[i] = argv[i-1];
		argv[j++] = t;
		t = strtok(NULL, " ");
	    }
	}
# endif /* PARSE_CCOPTS */


	/* allocate space for argv arrays  and initialize variables */
	narg = argc + 1;	/* +1 for the terminating NULL */
	srclist = (char **)myalloc(argc, sizeof (char *));
	init_argvs();

	while (optind < argc)
	 {
	     if ((pnarg>=pmaxarg) || (cnarg>=cmaxarg))
	      {
		  fprintf(stderr, "Too many arguments\n");
		  dexit(1);
	      }
	     switch (c = getopt(argc, argv, OPTSTR))
	      {
		  /* select the piece to invoke for either cpp, xpass or both */
	       case 'p':
		  if ( ((t = strtok(optarg, ",")) != optarg)
		      || ((npassname = strtok(NULL, ",")) == NULL)
		      || strtok(NULL, ",") )
		   {
		       fprintf(stderr,"cxref: invalid subargument: -p %s",optarg);
		   }
		  if (strlen(t) == 1)
		   {
		       if((i = getXpass((ic = *t), "-p")) != -1)
			{
			    if (passes[i].nameset || passes[i].pathset)
				fprintf(stderr,
					"cxref: -p %s overwrites previous option\n",t);
			    passes[i].nameset = 1;
			    passes[i].pathset = 0;
			    mkpname(i, npassname);
			}
		   }
		  else /* more than one pass to be substituted */
		   {
		       while(ic = *t++)
			{
			    if((i = getXpass(ic, "-p")) == -1)
				break;
			    if(passes[i].nameset || passes[i].pathset)
				fprintf(stderr,
					"cxref: -p %c overwrites previous option\n", ic);
			    passes[i].nameset = 0;
			    passes[i].pathset = 1;
			    mkpname(i, npassname);
			}
		   }
		  continue;
		  
		  /* prints cross reference for all files combined */
	       case 'c':
		  cflg = YES;
		  continue;
# ifdef DEBUGGING
		  /* debug flags to pass on to ccom (xpass) */
	       case 'd':
		  clist[cnarg++] = "-d";
		  ddbgflg = YES;
		  continue;
	       case 'i':
		  clist[cnarg++] = "-i";
		  idbgflg = YES;
		  continue;
	       case 'b':
		  clist[cnarg++] = "-b";
		  bdbgflg = YES;
		  continue;
	       case 'T':
		  clist[cnarg++] = "-t";
		  tdbgflg = YES;
		  continue;
	       case 'e':
		  clist[cnarg++] = "-e";
		  edbgflg = YES;
		  continue;
	       case 'x':
		  clist[cnarg++] = "-x";
		  xdbgflg = YES;
		  continue;
# endif /* DEBUGGING */
	       case 'h':
		  hflag = NO;
		  continue;
	       case 'v':
		  verbose = YES;
		  continue;
	       case 'Y':
		  if (nlsflg==YES) continue; /* don't duplicate, even if user did */
		  clist[cnarg++] = "-n";
		  plist[pnarg++] = "-n";
		  nlsflg = YES;
		  continue;
	       case 'A': /* select compiler mode, compatability or ansi */
		  
		  if(*optarg == 'a')
		   {
		       /* select ansi mode by invoking the ansi compiler */
		       ansiflg = YES;
		       nameps[Xcp] = XCPPANSI;
		       nameps[Xc0] = XREFANSI;
		   }
		  else if (*optarg == 'c')
		   {
		       /* compatability mode compiler */
		       ansiflg = NO;
		       nameps[Xcp] = XCPP;
		       nameps[Xc0] = XREF;
		   }
		  else
		   {
		       /* unrecognized option */
		       fprintf("cxref: '-A %s': unrecognized option",optarg);
		   }
		  continue;
	       case 'D':
	       case 'U':
	       case 'I':
		  plist[pnarg] = myalloc(strlen(optarg)+2, sizeof (char));
		  plist[pnarg][0] = '-';
		  plist[pnarg][1] = c;
		  plist[pnarg][2] = '\0';
		  strcat(plist[pnarg++],optarg);
		  continue ;
	       case 'W' :
		   if ((optarg[1] != ',')
		       || ((t = strtok(optarg, ",")) != optarg))
		    {
			fprintf(stderr, "cxref: invalid subargument -W%s\n", optarg);
			continue;
		    }
		  if((i = getXpass((ic = *t), "-W")) == -1)
		      continue;
		  if (i == Xcp)
		       while((t = strtok(NULL, ",")) != NULL)
			{
			    if (pnarg>=pmaxarg)
			     {
				 fprintf(stderr, "cxref: too many arguments\n");
				 dexit(1);
			     }
			    plist[pnarg++] = t;
			}
		  else if(i == Xc0)
		      while((t = strtok(NULL, ",")) != NULL)
		       {
			   if (cnarg>=cmaxarg)
			    {
				fprintf(stderr, "cxref: too many arguments\n");
				dexit(1);
			    }
			   clist[cnarg++] = t;
		       }
		  continue;
	       case 't':
	       case 'w':
		  /* length option when printing on terminal */
		  LSIZE = getnumb(optarg) - BASIZ;
		  if (LSIZE <= 0)
		      LSIZE = TTYOUT;
		  continue;
	       case 's':
		  silent = YES;
		  continue;
	       case 'o':
		  /* output file */
		  if (freopen(optarg, "w", stdout) == NULL)
		   {
		       perror(*argv);
		       dexit(1);
		   }
		  continue;
	       case 'g':
	       case 'O':
		  /* Let 'g' and 'O' pass -- they show up when
		   * CFLAGS are used in a makefile's cxref. */
#ifdef hp9000s300
		  /* additional args to the s300 C compiler.
		   * Just ignore them here.
		   */
	       case 'Z':  case 'z':
	       case 'S':  
	       case 'G':
	       case 'E':
		  continue;
					
	       case '#':
		  debug = YES;
# endif
		  continue;
	       case '?':
		  if (strchr(OPTSTR, (char) optopt))
		   {
		       if (optopt !='l')
			   fprintf(stderr,"cxref:missing argument after -%c option",optopt);
		       continue;
		   }
		  dexit(1);
	       case EOF:
		  t = argv[optind];
		  optind++;

# ifdef hp9000s200
		/* handle "+" options. +N gets passed onto the xpass as
		 * a -N option.  All the others are ignored.
		 */
		if (*t == '+')
		 {
		     ic = *++t;
		     switch (ic)
		      {
		       case 'N':
			  if (cnarg >=cmaxarg)
			   {
			       fprintf(stderr, "cxref: too many arguments\n");
			       dexit(1);
			   }
			  clist[cnarg] = copy(--t);
			  clist[cnarg++][0] = '-';
			  continue;

		       default:
			  continue;
		      }
		 }
		else
# endif
		if (strcmp(t + strlen(t) - 2, ".c") != 0)
			continue;		/* not a .c file, skip */
		else
		     srclist[srcarg++] = t;
	      }
	 }

	/* finished examining all options */

	for (i = 0; i < NPASS; i++)
	 {
	     if(!passes[i].pathset && !passes[i].nameset)
		  mkpname(i, i == Xc0 ? OWNERBIN : LIB);
	     else if(passes[i].pathset)
	      {
		  passes[i].usepath = 1;
		  mkpname(i, NULL);
		  passes[i].alt = 1;
	      }
	 }
			
	xref = passes[Xc0].name;
	xcpp = passes[Xcp].name;
	
	if(ansiflg)
	 {
	     clist[0]       = XREFANSI;
	     plist[0]       = XCPPANSI;
	     
	     /* support generic ansi cpp */
# ifdef hp9000s300
	     plist[pnarg++] = "-I/usr/include";
	     
	     /* clean name space predefines */
	     plist[pnarg++] = "-D__hp9000s300";
	     plist[pnarg++] = "-D__unix";
	     plist[pnarg++] = "-D__hpux";
# endif
# ifdef hp9000s800
	     plist[pnarg++] = "-I/usr/include";
	     
	     /* clean name space predefines */
	     plist[pnarg++] = "-D__hp9000s800";
	     plist[pnarg++] = "-D__unix";
	     plist[pnarg++] = "-D__hpux";
	     plist[pnarg++] = "-D__hppa";
	     plist[pnarg++] = "-A";	/* obsolete ?? */
# endif
	     
	 }
	else
	 {
	     clist[0]       = XREF;
	     plist[0]       = XCPP;
	 }
 	     

	for(i = 0; i < srcarg; i++)
	 {
	     sblk = 0;
	     cfile = srclist[i];
	     if (silent == NO)
		 printf("%s:\n\n", cfile);
	     
	     doxref();	/* find variables in the input files */
	     if (cflg == NO)
	      {
		  sortfile();	/* sorts temp file */
		  prtsort();	/* print sorted temp file when -c option is not used */
		  tunlink();	/* forget everything */
		  mktmpfl();	/* and start over */
	      }
	 }
	
	
	if (cflg == YES)
	 {
	     sortfile();	/* sorts temp file */
	     prtsort();	/* print sorted temp file when -c option is used */
	 }
	tunlink();	/* unlink temp files */
	exit (0);
}


	/*
	 * This routine calls the program "xpass" which parses
	 * the input files, breaking the file down into
	 * a list with the variables, their definitions and references,
	 * the beginning and ending function block numbers,
	 * and the names of the functions---The output is stored on a
	 * temporary file and then read.  The file is then scanned
	 * by tmpscan(), which handles the processing of the variables
	 */

doxref()
{

	register int	i;
	FILE *fp, *tfp;
	char line[MAXRFL], s1[MAXRFW], s2[MAXRFW];

	/* setup for running xcpp.  Go back and set the "cfile" argument,
	 * and put a NULL after the last argument.
	 */
	plist[1] = cfile;
	if (pnarg >= pmaxarg) {
		fprintf(stderr, "cxref: too many arguments for pass -Wp\n");
		dexit(1);
		}
	plist[pnarg] = 0 ;
	if ( callsys(xcpp, plist) > 0) {
		fprintf(stderr, "xcpp failed on %s!\n", cfile);
		dexit(1);
	};

	/* setup for running xpass.  Set the "cfile" argument, and put
	 * a NULL after the last argument.
	 */
	clist[2] = cfile;
	if (cnarg >= cmaxarg) {
		fprintf(stderr, "cxref: too many arguments for pass -Wc\n");
		dexit(1);
	}
	clist[cnarg] = 0;
	if ( callsys(xref, clist) > 0) {
		fprintf(stderr, "xpass failed on %s! cxref may be incomplete\n", cfile);
		/* This used to be a fatal error, but we should be able
		 * to continue and print at least a pratial cxref from
		 * the info ccom was able to generate.
		/*dexit(1);*/
	};

	/* open temp file produced by "xpass" for reading */
	if ((fp = fopen(tmp2, "r")) == NULL) {
		perror(tmp2);
		dexit(1);
	}
	else {
		setbuf(fp, buf);
		/*
		 * open a new temp file for writing
		 * the output from tmpscan()
		 */
		if ((tfp = fopen(tmp3, "a")) == NULL) {
			perror(tmp3);
			dexit(1);
		}
		else {

			/*
			 * read a line from tempfile 2,
			 * break it down and scan the
			 * line in tmpscan()
			 */

			while (fgets(line, MAX, fp) != NULL) {
				if (line[0] == '"') {
					/* removes quotes */
					for (i=0; line[i + 1] != '"'; i++) {
						xflnm[i] = line[i + 1];
					};
					xflnm[i] = '\0';
					continue;
				};
				sscanf(line, "%s%s", s1, s2);
				tmpscan(s1, s2, tfp);
			};
			if (tfp && (feof(tfp) || ferror(tfp)))
			{
				perror("cxref.tmpscan");
				dexit(1);
			}
			fclose(tfp);
		};
		if (fp && ferror(fp))
		{
			perror("cxref.tmpscan");
			dexit(1);
		}
		fclose(fp);
	};
}

	/*
	 * general purpose routine which does a fork
	 * and executes what was passed to it--
	 */

callsys(f, v)
	char f[], *v[];
{
	register int pid, w;
	char **p;
	int status;
	int t;

	if (debug) {
		printf("callsys %s:", f);
		for (p=v; *p != NULL; p++)
			printf(" '%s'", *p);
		printf(" .\n");
		return(0);
	}

	if (verbose) {
		fprintf(stderr, "%s ", f);
		t = 0;
		while( v[++t]  ) fprintf(stderr, " %s", v[t]);
		fprintf(stderr, "\n");
	}

	if ((pid = fork()) == 0) {
		/* only the child gets here */
		execvp(f, v);
		perror(f);
		dexit(1);
	}
	else
		if (pid == -1) {
			/* fork failed - tell user */
			perror("cxref");
			dexit(1);
		};
	/*
	 * loop while calling "wait" to wait for the child.
	 * "wait" returns the pid of the child when it returns or
	 * -1 if the child can not be found.
	 */
	while (((w = wait(&status)) != pid) && (w != -1));
	if (w == -1) {
		/* child was lost - inform user */
		perror(f);
		dexit(1);
	}
	else {
		/* check system return status */
		if (((w = status & 0x7f) != 0) && (w != SIGALRM)) {
			/* don't complain if the user interrupted us */
			if (w != SIGINT) {
				fprintf(stderr, "Fatal error in %s", f);
				perror(" ");
			};
			/* remove temporary files */
			dexit(1);
		};
	};
	/* return child status only */
	return((status >> 8) & 0xff);
}



/* Allocate space for the arg arrays for the calls to xcpp and xpass,
 * initialize variables, and fill in the array's fixed items.
 */
init_argvs()
{

	pmaxarg = narg + NARG0;
	plist = (char **)myalloc(pmaxarg * sizeof(*plist), 1);
	plist[0] = "cpp";
	plist[1] = 0;	/* cfile will go here */
	plist[2] = tmp1;
	plist[3] = "-F";
	plist[4] = tmp2;
	pnarg = 5;

	cmaxarg = narg + NARG0;
	clist = (char **)myalloc(cmaxarg * sizeof(*clist), 1);
	clist[0] = "xpass";
	clist[1] = "-f";
	clist[2] = 0;	/* cfile will go here */
	clist[3] = "-i";
	clist[4] = tmp1;
	clist[5] = "-o";
	clist[6] = tmp2;
	cnarg = 7;


}


	/*
	 * general purpose routine which returns
	 * the numerical value found in a string
	 */

getnumb(ap)
	register char *ap;
{

	register int n, c;

	n = 0;
	while ((c = *ap++) >= '0' && c <= '9')
		n = n * 10 + c - '0';
	return(n);
}

tmpscan(s, ns, tfp)
	register char s[];
	char ns[];
	FILE *tfp;
{

	/* this routine parses the output of "xpass"*/

	/*
	 * D--variable defined; R--variable referenced;
	 * F--function name; B--block(function ) begins;
	 * E--block(function) ends
	 */

	register int lino = 0;
	char star;

	/*
	 * look for definitions and references of external variables;
	 * look for function names and beginning block numbers
	 */

	if (inafunc == NO) {
		switch (*s++) {
			case 'D':
				star = '*';
				goto ahead1;
			case 'R':
				star = ' ';
ahead1:				lino = getnumb(ns);
				fprintf(tfp, "%s\t%s\t%s\t%5d\t%c\n", s, xflnm, clfunc, lino, star);
				break;
			case 'F':
				strcpy(funcname, s);
				star = '$';
				fprintf(tfp, "%s\t%c\n", s, star);
				break;
			case 'B':
				inafunc = YES;
				sblk = getnumb(s);
				break;
			default:
				fprintf(stderr, "SWITCH ERROR IN TMPSCAN: inafunc = no\n");
				dexit(1);
		};
	}
	else {
		/*
		 * in a function:  handles local variables
		 * and looks for the end of the function
		 */

		switch (*s++) {
			case 'R':
				star = ' ';
				goto ahead2;
				/* No Break Needed */
			case 'D':
				star = '*';
ahead2:				lino = getnumb(ns);
				fprintf(tfp, "%s\t%s\t%s\t%5d\t%c\n", s, xflnm, funcname, lino, star);
				break;
			case 'B':
				break;
			case 'E':
				lino = getnumb(s);
				/*
				 * lino is used to hold the ending block
				 * number at this point
				 *
				 * if the ending block number is the
				 * same as the beginning block number
				 * of the function, indicate that the
				 * next variable found will be external.
				 */

				if (sblk == lino) {
					inafunc = NO;
				}
				break;
			case 'F':
				star = '$';
				fprintf(tfp, "%s\t%c\n", s, star);
				break;
			default:
				fprintf(stderr, "SWITCH ERROR IN TMPSCAN: inafunc = yes\n");
				dexit(1);
		};
	};
}

mktmpfl()
{

	/* make temporary files */

	tmp1 = mktemp("/tmp/xr1XXXXXX");
	tmp2 = mktemp("/tmp/xr2XXXXXX");	/* holds output of "xpass" */
	tmp3 = mktemp("/tmp/xr3XXXXXX");	/* holds output of tmpscan() routine */
	tmp4 = mktemp("/tmp/xr4XXXXXX");	/* holds output of tempfile 3 */
	tmpmade = YES;	/* indicates temporary files have been made */
	setsig();
}

tunlink()
{

	/* unlink temporary files */

	if (tmpmade == YES) {	/* if tempfiles exist */
		unlink(tmp1);
		unlink(tmp2);
		unlink(tmp3);
		unlink(tmp4);
	};
}

dexit(n)
	int n;
{
	/* remove temporary files and exit with error status */

	tunlink();
	exit(n);
}

setsig()
{

	/* set up check on signals */

	int sigout();

	if (isatty(1)) {
		if (signal(SIGHUP, SIG_IGN) == SIG_DFL)
			signal(SIGHUP, sigout);
		if (signal(SIGINT, SIG_IGN) == SIG_DFL)
			signal(SIGINT, sigout);
	}
	else {
		signal(SIGHUP, SIG_IGN);
		signal(SIGINT, SIG_IGN);
	};

	signal(SIGQUIT, sigout);
	signal(SIGTERM, sigout);
}

sigout()
{

	/* signal caught; unlink tmp files */

	tunlink();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	dexit(1);
}


char	*xarv[5];

sortfile()
{
	/* sorts temp file 3 --- stores on 4 */

	register int status;

	xarv[0] = "sort";
	xarv[1] = "-o";
	xarv[2] = tmp4;
	xarv[3] = tmp3;
	xarv[4] = 0;
	/* execute sort */
	if ((status = callsys(SRT, xarv)) > 0) {
		fprintf(stderr, "Sort failed with status %d\n", status);
		dexit(1);
	};
}

prtsort()
{

	/* prints sorted files and formats output for cxref listing */

	FILE *fp;
	char line[MAX];

	/* open tempfile of sorted data */
	if ((fp = fopen(tmp4, "r")) == NULL) {
		fprintf(stderr, "CAN'T OPEN %s\n", tmp4);
		dexit(1);
	}
	else {
		if (hflag) {
			fprintf(stdout, "SYMBOL		  FILE		      FUNCTION	   LINE\n");
		};
		while (fgets(line, MAX, fp) != NULL) {
			scanline(line);	/* routine to format output */
		};
		fprnt = YES;	/* reinitialize for next file */
		if (fp && ferror(fp))
		{
			perror("cxref.prtsort");
			dexit(1);
		}
		fclose(fp);
		putc('\n', stdout);
	};
}

scanline(line)
	char *line;
{

	/* formats what is to be printed on the output */

	register char *sptr1;
	register int och, nch;
	char s1[MAXRFL], s2[MAXRFL], s3[MAXRFL], s4[MAXRFL], s5[MAXRFL];
	
	/*
	 * break down line into variable name, filename,
	 * function name, and line number
	 */
	sscanf(line, "%s%s%s%s%s", s1, s2, s3, s4, s5);
	if (strcmp(s2, "$") == 0) {
		/* function name */
		if (strcmp(sv1, s1) != 0) {
			strcpy(sv1, s1);
			printf("\n%s()", s1);	/* append '()' to name */
			*sv2 = *sv3 = *sv4 = '\0';
			fprnt = NO;
		};
		return;
	};
	if (strcmp(s5, "*") == 0) {
		/* variable defined at this line number */
		*s5 = '\0';
		sptr1 = s4;
		och = '*';
		/* prepend a star '*' */
		for ( nch = *sptr1; *sptr1 = och; nch = *++sptr1)
			och = nch;
	}
	if (fprnt == YES) {
		/* if first line--copy the line to a save area */
		prntwrd( strcpy(sv1, s1) );
		prntflnm( strcpy(sv2, s2) );
		prntfnc( strcpy(sv3, s3) );
		prntlino( strcpy(sv4, s4) );
		fprnt = NO;
		return;
	}
	else {
		/*
		 * this part checks to see what variables have changed
		 */
		if (strcmp(sv1, s1) != 0) {
			nword = nflnm = nfnc = nlino = YES;
		}
		else {
			nword = NO;
			if (strcmp(sv2, s2) != 0) {
				nflnm = nfnc = nlino = YES;
			}
			else {
				nflnm = NO;
				if (strcmp(sv3, s3) != 0) {
					nfnc = nlino = YES;
				}
				else {
					nfnc = NO;
					nlino = (strcmp(sv4, s4) != 0) ? YES : NO;
					if (nlino == YES) {
						/*
						 * everything is the same
						 * except line number
						 * add new line number
						 */
						addlino = YES;
						prntlino( strcpy(sv4, s4) );
					};
					/*
					 * Want to return if we get to
					 * this point. Case 1: nlino
					 * is NO, then entire line is
					 * same as previous one.
					 * Case 2: only line number is
					 * different, add new line number
					 */
					return;
				};
			};
		};
	};

	/*
	 * either the word, filename or function name
	 * are different; this part of the routine handles  
	 * what has changed...
	 */

	addlino = NO;
	lsize = 0;
	if (nword == YES) {
		/* word different--print line */
		prntwrd( strcpy(sv1, s1) );
		prntflnm( strcpy(sv2, s2) );
		prntfnc( strcpy(sv3, s3) );
		prntlino( strcpy(sv4, s4) );
		return;
	}
	else {
		fputs("\n\t\t", stdout);
		if (nflnm == YES) {
			/*
			 * different filename---new name,
			 * function name and line number are
			 * printed and saved
			 */
			prntflnm( strcpy(sv2, s2) );
			prntfnc( strcpy(sv3, s3) );
			prntlino( strcpy(sv4, s4) );
			return;
		}
		else {
			/* prints filename as formatted by prntflnm()*/
			switch (fsp) {
			case 1:
				printf("%s\t\t\t", s2);
				break;
			case 2:
				printf("%s\t\t", s2);
				break;
			case 3:
				printf("%s\t", s2);
				break;
			case 4:
				printf("%s  ", s2);
			};
			if (nfnc == YES) {
				/*
				 * different function name---new name
				 * is printed with line number;
				 * name and line are saved
				 */
				prntfnc( strcpy(sv3, s3) );
				prntlino( strcpy(sv4, s4) );
			};
		};
	};
}

prntwrd(w)
	char *w;
{

	/* formats word(variable)*/
	register int wsize;	/*16 char max. word length */

	if ((wsize = strlen(w)) < 8) {
		printf("\n%s\t\t", w);
	}
	else
		if ((wsize >= 8) && (wsize < 16)) {
			printf("\n%s\t", w);
		}
		else {
			printf("\n%s  ", w);
		};
}

prntflnm(fn)
	char *fn;
{

	/* formats filename */
	register int fsize;	/*24 char max. fliename length */

	if ((fsize = strlen(fn)) < 8) {
		printf("%s\t\t\t", fn);
		fsp = 1;
	}
	else {
		if ((fsize >= 8) && (fsize < 16)) {
			printf("%s\t\t", fn);
			fsp = 2;
		}
		else
			if ((fsize >= 16) && (fsize < 24)) {
				printf("%s\t", fn);
				fsp = 3;
			}
			else {
				printf("%s  ", fn);
				fsp = 4;
			};
	};
}

prntfnc(fnc)
	char *fnc;
{

	/* formats function name */
	if (strlen(fnc) < 8) {
		printf("%s\t  ", fnc);
	}
	else {
		printf("%s  ", fnc);
	};
}
/* ----------------------------------------------------- */
char *
copy(s)
register char *s;
{
	register char *ns;

	ns = myalloc(strlen(s)+1,sizeof ns);
	return(strcpy(ns, s));
}


char *
myalloc(num, size)
	unsigned num, size;
{
	register char *ptr;

	if ((ptr = calloc(num, size)) == NULL) {
		perror("cxref");
		dexit(1);
	};
	return(ptr);
}

prntlino(ns)
	register char *ns;
{

	/* formats line numbers */

	register int lino, i;
	char star;

	i = lino = 0;

	if (*ns == '*') {
		star = '*';
		ns++;	/* get past star */
	}
	else {
		star = ' ';
	};
	lino = getnumb(ns);
	if (lino < 10)	/* keeps track of line width */
		lsize += (i = 3);
	else if ((lino >=10) && (lino < 100))
			lsize += (i = 4);
	else if ((lino >= 100) && (lino < 1000))
				lsize += (i = 5);
	else if ((lino >= 1000) && (lino < 10000))
					lsize += (i = 6);
	else /* lino > 10000 */
						lsize += (i = 7);
	if (addlino == YES) {
		if (lsize <= LSIZE) {
			/* line length not exceeded--print line number */
			fprintf(stdout, " %c%d", star, lino);
		}
		else {
			/* line to long---format lines overflow */
			fprintf(stdout, "\n\t\t\t\t\t\t   %c%d", star, lino);
			lsize = i;
		};
		addlino = NO;
	}
	else {
		fprintf(stdout, " %c%d", star, lino);
	};
}

/* ----------------------------------------------------- */
mkpname(n, pname)
int	n;
char	*pname;
{
	if(nameps[n])
	 {
	     if(!passes[n].usepath)
	      {
		  if(passes[n].nameset && ! passes[n].pathset)
		   {
		       /* a -p<pass>,<path name> was specfied */
		       passes[n].name = myalloc(strlen(pname) + 1, sizeof (char));
		       strcpy(passes[n].name, pname);
		   }
		  else if(!passes[n].nameset && passes[n].pathset)
		   {
		       /* a multipe substition was request by
			* -p<pass_list>,<path>
			* so save the new path name */
		       passes[n].path = myalloc(strlen(pname) + 1, sizeof (char));
		       strcpy(passes[n].path, pname);
		   }
		  else 
		   {
		       passes[n].name = myalloc(strlen(pname) + strlen(nameps[n]) + 1, sizeof (char));
		       strcpy(passes[n].name, pname);
		       strcat(passes[n].name,nameps[n]);
		   }
	      }
	     else
	      {
		  /* use the previously specified path to create the
		   * full path name to the piece
		   */
		  passes[n].name = myalloc(strlen(passes[n].path) + strlen(nameps[n]) + 1, sizeof (char));
		  strcpy(passes[n].name,passes[n].path );
		  strcat(passes[n].name,nameps[n]);
	      }
	 }
}

/* ----------------------------------------------------- */
getXpass(c, opt)
char	c, *opt;
{
    int pass = -1;
    
    switch (c)
     {
      case '0':
      case 'c':
	 pass = Xc0;
	 break;
	 
      case 'p':
	 pass = Xcp;
	 break;
	 
      default:
	 fprintf(stderr, "cxref: unrecognized pass name: '%s%c'\n", opt, c);
	 break;
     }
    return pass;
}
