static char *HPUX_ID = "@(#) B2371B.08.37  C/ANSI C  Internal $Revision: 72.3 $";
/* @(#) $Revision: 72.3 $ */
/*
 *              ****Series 300 only****
 *
 * cc command -- calls the appropriate passes of the
 *		c compiler, assembler, and linker.
 * 
 * This code is a modification of the Bell V.2 cc.c .
 *
 */

# include <errno.h>
# include <stdio.h>
# include <ctype.h>
# include <signal.h>
# include <unistd.h>
# include <string.h>


#ifdef TIMES
# include <sys/types.h>
# include <sys/times.h>
# include <sys/param.h>
 
        struct {
                time_t user;
                time_t sys;
                time_t childuser;
                time_t childsys;
        } buffer1, buffer2;        /* buffer1 and buffer2 has the starting
                                   /* and ending time of a child repectively*/
        long    before, after;
        extern  long times();

#endif TIMES

#define	SCPTR	sizeof(char *)

#define	Xdr	-2
#define	Xccom	0
#define Xcp1	1
#define Xcp2	2
#define	Xc0	3
#define	Xc1	4
#define	Xc2	5
#define	Xcp	6
#define	Xas	7
#define	Xbba	8
#define	Xld	9
#define	Xcrt0	10
#define	Xend	11
#define	NPASS	10
#define	NPARTS	2
#define Xdummy  NPASS
#define XNPASS  NPASS+1

#define	NAMEcp   "cpp"
#define	NAMEcpa  "cpp.ansi"		/* ansi version of cpp */
#define	NAMEcpbba  "bbacpp" 		/* bba  version of cpp */
#define	NAMEccom   "ccom"
#define	NAMEccoma  "ccom.ansi"		/* ansi version of ccom */
#define NAMEcp1  "cpass1"
#define NAMEcp1a "cpass1.ansi"		/* ansi version of cpass1 */
#define NAMEcp2  "cpass2"
#define NAMEc0   "c.c0"
#define	NAMEc1   "c.c1"
#define	NAMEc2   "c.c2"
#define	NAMEas   "as"
#define NAMEbbagen "bbagen"             /* bba generation utility */
#define	NAMEld   "ld"
#define NAMEcrt0  "crt0.o"
#define NAMEend   "end.o"

#define	TEMPDIR	"/tmp"
char *tmp_infile[ XNPASS ];              /* tmp (default) input file names */
char *infile[ XNPASS ];                  /* actual input file names */
char *user_outfile = NULL;               /* final outputfile (if not '.o') */
char *macrofile;			 /* Static Analysis macrofile for cpp */
char *bbafile;                           /* archive output file of bbagen */

typedef char flag;

char	*drivername;	/* name by which invoked, argv[0] */
char	*outfile;	/* name of output file (-o option) */
char	**clist;	/* list of source files (.s or .c) */
char	**list[NPASS];	/* list of arguments to be passed to each pass */
int	nlist[NPASS];	/* # of arguments for each pass */
char	**av;		/* argv for execv (corresponds to v[] of callsys) */

flag    bbaflag;        /* flag indicates ccbba driver; call bba pieces  */
flag    c89flag;        /* flag indicates c89 driver (ansi only) is specified */
flag    ansiflag;       /* flag -A[ac] option ( select compiler mode */
flag	ansiextensions; /* flag +e to enable extensions (asms) */
flag    multiSubstitute;/* true: a multiple pass -t spec seen.  Allow overwrite
			 * of pass name if -A[ca] is used, otherwise let use
			 * get what he or she asked for */
flag    divorced;       /* preprocessor & compiler passes are *NOT* merged 
			 *    for all source files */
flag    estranged;      /* preprocessor & compiler passes are *NOT* merged 
			 *    for this source file only ( .i file ) */
flag    separated;      /* divorced || estranged */
flag    child = 0;      /* set if child process */
flag	pflag;		/* flag -P option (preprocess -> .i file)*/
flag	sflag;		/* flag -S option (compile -> .s file)*/
flag	cflag;		/* flag -c option (don't link) */
int	eflag;		/* flag error */
flag	gflag;		/* flag -g option (debug) */
flag	vflag;		/* flag +OV option (objects derefenced by global pointers are treated as 'volatile') */
flag	wflag;		/* flag -w option (warning off) */
flag	exflag;		/* flag -E option (preprocess -> stdout)*/
flag	oflag;		/* flag -O option (optimize) */
flag	proflag;	/* flag -p option (profile) */
flag	gproflag;	/* flag -G option (gprof) */
flag	nlsflag;	/* flag -Y option (enable NLS support) */
flag	verbose;	/* flag -v option */
flag    picword;        /* flag +z option */
flag    piclong;        /* flag +Z option */
flag    saflag;		/* Static Analysis -y */
flag    plusyflag;	/* +y, Modifier for including all debug info */
			/* used only with -g or -y                   */
flag	debug;		/* flag -# option (undocumented feature) */

#ifdef TIMES
flag	timeflag;	/* flag +T  report compilation time (undocumented feature) */
#endif TIMES

flag	c0flag;		/* 1 iff c0 is to be called, if c0 is 1, c1 MUST be 1 */
flag	c1flag;		/* 1 iff c1 is to be called */
flag	c2flag;		/* 1 iff c2 is to be called */
flag	fphwflag;	/* set if +ffpa , +bfpa or +M are used */
			/* +ffpa +bfpa --> +1 */
			/* +M          --> +3 */
/*
 * define a structure which records all the data for a pass and
 * allocate one for each pass
 */
struct PASS {
 char *path;		/* the path to the piece */
 char *name;		/* the name of the piece */
 unsigned alt      :1;	/* 1 -> use alternate pass in /usr/lib, /usr/bin */
 unsigned usepath :1;	/* 1 -> use the path for creating full pathname */
 unsigned nameset :1;	/* 1 -> set by user with -t, 0 -> use default */
 unsigned pathset :1;	/* 1 -> new path was provide by -t, 0 -> use default */
} passes[NPASS+NPARTS];
 
char	*npassname ;		/* new pass name (-t option) */
char	libprefix[] = "/lib/";	/* prefix for p012 passes */
char	binprefix[] = "/bin/";  /* prefix for al passes */
char	usrlibprefix[] = "/usr/lib/";  /* prefix for end.o */
char	bbacppprefix[] = "/usr/hp64000/lib/";  /* prefix for bbacpp */
char	bbagenprefix[] = "/usr/hp64000/bin/";  /* prefix for bbagen */
char	*nameps[NPASS+NPARTS] = {NAMEccom, NAMEcp1, NAMEcp2, NAMEc0, NAMEc1,
				 NAMEc2, NAMEcp, NAMEas, NAMEbbagen, NAMEld,
				 NAMEcrt0, NAMEend};

/* getopt (3C) externs */
extern	int optind;	/* argv index of next argument to be processed */
extern	int opterr;	/* if set to 0, supresses error message from getopt */
extern	char *optarg;	/* points to start of option argument */
extern	int optopt;	/* (undocumented) value of the illegal option when
			   getopt returns a '?' */
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
static A_count = 0;     /* CLL1100030   Always use the last -A option */
#endif 

#ifdef SCC
#define OPTSTR  "a:B:cCD:EI:l:L:nNo:PqQsSt:U:wW:YzZ"
#else
#define OPTSTR  "A:a:B:cCD:EgGI:l:L:nNo:OpPqQsSt:U:vwW:yYzZ#"
#endif

/* pipe-specific data structures */
#include <fcntl.h>
struct pipeinfo { char *f;
		  char **v;
		  char *altf;
		  int pid;              /* process id */
		  unsigned alt:1;
		  unsigned term:1;      /* indicates termination */
		  unsigned short pass;
	     } pipephase[ XNPASS ];

int pipenum = 0;        /* # of processes to be piped */
flag pipeflag = 1;      /* pipe option on; use +s for off */

int pipe_killer;             /* last pipephase or first with fatal error */
int pipe_error;              /* fatal error - e.g., don't link */
int pcpipe[ NPASS-1 ][ 2 ];  /* pipes for passes */
int std_err[ NPASS ][ 2 ];   /* pipes between passes' stderr & driver */

flag pass_in_pipe[ XNPASS ];
int nextpass[ XNPASS ];
/* end pipe-specific data structures */

#ifdef INSTRUMENTED
/* compiler instrumentation for beta releases. */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
/* udp packet definition */
struct udp_packet {
    int size;			/* size of the source file in bytes */
    char name[14];		/* basename of the file */
    unsigned long inode;	/* inode */
    int flag_debugging:1;	/* -g used */
    int flag_optlevel1:1;	/* +O1 used */
    int flag_optlevel2:1;	/* +O2 used */
    int flag_prof:1;		/* -p used */
    int flag_gprof:1;		/* -G used */
    int flag_ansi:1;		/* -Aa used */
    int flag_ffpa:1;		/* +ffpa used */
    int flag_m:1;		/* -M used */
    int flag_warning:1;		/* -w used */
    int unused:2;		/* saved for future use (zero'd) */
    int version:5;		/* compiler version id */
};
/* The following defines should be passed in, the examples below were used
 * in the prototype: -DINST_UDP_PORT=49963 -DINST_UDP_ADDR=0x0f01780d */
/* pick a udp port that is unlikely to be used elsewhere
#define INST_UDP_PORT		42963 */
/* internet address of hpcomet 15.1.120.13
#define INST_UDP_ADDR		0x0f01780d */
/* compiler version id 
#define INST_VERSION		1 */
#endif /* INSTRUMENTED */

/* ----------------------------------------------- */

char	*copy(), *setsuf(), *stralloc(), *strchar();
char	*strcat(), *strncat(), *strtok();
char	*strncpy(), *strcpy(), *strchr();
char	*mktemp(), *malloc(), *tempnam();
char	*realloc(), *getenv();

void	ino_clash();

/* ----------------------------------------------------- */

main(argc, argv)
char *argv[]; 
{
	char *t;
	char *ccopts;			/* options, before cmd line */
	char *ccopts1;
	char *ccopts2;
	char *ccoptsAfter;		/* options, after cmd line */
	char *ccoptsCount;		/* count options, resize once */
	char **newargv;			/* merge of CCOPTS and argv */
	int howmany_ccopts;		/* how many new args from PCOPTS */
	int nc, nargs, nxo;
	char ic;
	int i, j, c, na;
	int idexit();

	/* check for environment variable CCOPTS  */
	ccopts = (char *) getenv("CCOPTS");
	ccopts2 = ccopts1 = NULL;
	if (ccopts && (strlen(ccopts) > 0)) {

	    /*
	     * look for trailing cc opts and record them.
	     */
	    ccoptsAfter = ccopts;
	    while ((ccoptsAfter = strchr(ccoptsAfter, '|')) != NULL) 
	       if ((*(ccoptsAfter - 1)) == '\\') {
		  /* step past the escaped | */
		  ++ccoptsAfter;
	       } else {
		
	          /*
		   * found an unescaped |, terminate leading options
		   * move After to start of trailing options and copy
		   * them for echoing later as needed.
		   */
	          *ccoptsAfter++ = '\0';
		  ccopts2 = copy(ccoptsAfter);
		  break;
	      }

	    /* count leading options to add */
	    ccoptsCount = copy(ccopts);
	    howmany_ccopts = 0;
	    if ((t = strtok(ccoptsCount, " ")) != NULL)
		howmany_ccopts++;
	    while((t= strtok(NULL, " ")) != NULL)
		howmany_ccopts++;

	    /* count trailing options */
	    if (ccoptsAfter) {
	        ccoptsCount = copy(ccoptsAfter);
		if ((t = strtok(ccoptsCount, " ")) != NULL)
		    howmany_ccopts++;
		while((t= strtok(NULL, " ")) != NULL)
		    howmany_ccopts++;
	    }

	    /* get new hunk of memory big enough for all the options */
	    i = (argc + howmany_ccopts + 1) * sizeof (char *);
	    if((newargv = (char **) malloc((unsigned) i)) == NULL) {
	        perror("%s:",argv[0]);
		dexit(300);
	    }

	    /* copy program name */
	    newargv[0] = argv[0];

	    /*
	     * Copy leading options into newargv, then the existing argv
	     * contents and then the trailing cc opts.
	     */
	    i = 1;			/* 1st slot after program name */
	    
	    if (ccopts && strlen(ccopts)) {
	        ccopts1 = copy(ccopts);	/* save for later display */
		if ((t = strtok(ccopts, " ")) != NULL) {
	            do {
		        newargv[i++] = t;
		    } while((t = strtok(NULL, " ")) != NULL);
		}
	    }

	    /* copy original argv into new argv */
	    for (j = 1; j  < argc; i++,j++)
		 newargv[i] = argv[j];

	    /* copy trailing ccopts into newargv */
	    if (ccoptsAfter && ((t = strtok(ccoptsAfter, " ")) != NULL)) {
	        do {
		    newargv[i++] = t;
		} while((t = strtok(NULL, " ")) != NULL);
	    }
	    argc += howmany_ccopts;
	    newargv[argc] = NULL;
	    argv = newargv;
	}
   

	/* check which driver was specified */
	if (t=strrchr(argv[0],'/'))
          drivername = t + 1;
	else
          drivername = argv[0];
	if ((strlen(drivername)==3) && (strcmp(drivername,"c89") == 0))
	  {
	  c89flag = 1;
	  ansiflag = 1;
	  nameps[Xccom]  = NAMEccoma;
	  nameps[Xcp1] = NAMEcp1a;
	  nameps[Xcp]  = NAMEcpa;
	  }
	if ((strlen(drivername)==5) && (strcmp(drivername,"ccbba") == 0))
	  {
	  bbaflag = 1;
	  divorced = 1;
	  pipeflag = 0;
	  nameps[Xcp]  = NAMEcpbba;
	  }
	opterr = 0;
	i = nc = nxo = 0;
	nargs = argc + 100;
	j = SCPTR * nargs - 1;		/*stralloc allocates extra byte.*/
	for (i=NPASS; i-- > 0; ) {
		nlist[i] = 0;
		list[i] = (char **) stralloc(j);
	}
	clist = (char **)stralloc(j);
	av = (char **)stralloc(j + 10 * SCPTR);
	setbuf(stdout, (char *)NULL);  /* makes i/o completely unbuffered */

	/* process options : note that getopt returns EOF when it 
	   encounters the first non-option argument */

	while (optind<argc)
	  switch (c = getopt(argc, argv, OPTSTR)) {

#ifndef SCC
	case '#':
		debug = 1;
		continue;

	case 'v':
		verbose = 1;
		continue;
#endif /* not SCC */
	case 'w':
		wflag++;
		continue;

	case 'z':
		warn("option '-z' is not supported");
		continue;

	case 'Z': /* default is to allow null ptr dereferencing */
		continue;

#ifndef SCC
	case 'A': /* select compiler mode, compatability or ansi */
		
		if (c89flag)
		  {
		  warn("-A not allowed with c89 ansi driver, option ignored");
		  continue;
		  }
		if(*optarg == 'a')
		 {
		     /* select ansi mode by invoking the ansi compiler */
		     ansiflag = 1;
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
		     if( A_count++ )
			  warn( "-A %s overwrites previous option", optarg );
#endif
		     nameps[Xccom]  = NAMEccoma;
		     nameps[Xcp1] = NAMEcp1a;
		     if ( !bbaflag )
			  nameps[Xcp]  = NAMEcpa;
		 }
		else if (*optarg == 'c')
		 {
		     /* compatability mode compiler */
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
		     if( A_count++ )
			  warn( "-A %s overwrites previous option", optarg );
#endif
		     ansiflag = 0;
		     nameps[Xccom]  = NAMEccom;
		     nameps[Xcp1] = NAMEcp1;
		     if ( !bbaflag )
			  nameps[Xcp]  = NAMEcp;
		 }
		else
		 {
		     /* unrecognized option */
		     error("'-A %s': unrecognized option",optarg);
		 }
		continue;
#endif /* not SCC */
		
	case 'S':
		sflag++;
		cflag++;
		continue;

	case 'o':
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
		if (outfile)
		warn("- o %s overwrites earlier option -o %s",optarg, outfile);
#endif
		outfile = optarg;
		if ((c=getsuf(outfile))=='c' || outfile[0] == '-' || outfile[0] == '+')
			error("'-o %s': Illegal name for output file", outfile);
		continue;

#ifndef SCC
	case 'O':
		if (oflag)
		  {
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
		  warn("-O overwrites previous optimization option");
#endif
		  /* reset optimizer state to start  */
		  c0flag = c1flag = c2flag = vflag = oflag = 0;
		  }
		oflag++;
		c1flag++;
		c2flag++;
		continue;

	case 'p':
		proflag++;
		continue;

	case 'G':
		gproflag++;
		proflag++;
		continue;
#endif /* not SCC */

	case 'Y':
		nlsflag++;
		continue;

	case 'c':
		cflag++;
		continue;

	case 'E':
		exflag++;
	case 'P':
		pflag++;
		cflag++;
		divorced = 1;
	case 'D':
	case 'I':
	case 'U':
	case 'C':
		if (nlist[Xcp] >= nargs) {
			error("Too many EPDIUC options", (char *)NULL);
			continue;
		}

		if(c != 'E') {
		   list[Xcp][nlist[Xcp]] = stralloc(strlen(optarg)+2);
		   list[Xcp][nlist[Xcp]][0] = '-';
		   list[Xcp][nlist[Xcp]][1] = c;
		   list[Xcp][nlist[Xcp]][2] = '\0';
		   strcat(list[Xcp][nlist[Xcp]], optarg);
		   nlist[Xcp]++;
	        }
		continue;

	case 't':
		if ( ((t = strtok(optarg, ",")) != optarg)
			|| ((npassname = strtok(NULL, ",")) == NULL)
			|| strtok(NULL, ",") ){
			error("invalid subargument: -t %s",optarg);
			continue;
		}
		if (strlen(t) == 1){
			if((i = getXpass((ic = *t), "-t")) == -1)
				continue;
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
			if (passes[i].nameset || passes[i].pathset)
				warn("-t %s overwrites previous option", t);
#endif
			passes[i].nameset = 1;
			passes[i].pathset = 0;
			mkpname(i, npassname);
			if( i==Xcp ) divorced = 1;
			continue;
		}
		else /* more than one pass to be substituted */
		{
		        multiSubstitute = 1;
			while(ic= *t++){
			  if((i = getXpass(ic, "-t")) == -1)
				  continue;
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
			  if(passes[i].nameset || passes[i].pathset)
				  warn("-t %c overwrites previous option", ic);
#endif
			  passes[i].nameset = 0;
			  passes[i].pathset = 1;
			  mkpname(i, npassname);
			}
			continue;
		}
				
        case 'B':
                if (nlist[Xld] >= nargs)
                  {
                  error("Too many ld options", (char *)NULL);
                  continue;
                  }
                list[Xld][nlist[Xld]] = stralloc(strlen(optarg)+2);
                list[Xld][nlist[Xld]][0] = '-';
                list[Xld][nlist[Xld]][1] = 'B';
                list[Xld][nlist[Xld]][2] = '\0';
                strcat(list[Xld][nlist[Xld]], optarg);
                nlist[Xld]++;
                continue;
	case 'a':
                if (nlist[Xld] >= nargs)
                  {
                  error("Too many ld options", (char *)NULL);
                  continue;
                  }
                list[Xld][nlist[Xld]] = stralloc(strlen(optarg)+2);
                list[Xld][nlist[Xld]][0] = '-';
                list[Xld][nlist[Xld]][1] = 'a';
                list[Xld][nlist[Xld]][2] = '\0';
                strcat(list[Xld][nlist[Xld]], optarg);
                nlist[Xld]++;
                continue;

#ifndef SCC
	case 'g':
		gflag++;
		continue;
#endif /* not SCC */

	case 'W':
		if((optarg[1] != ',')
			|| ((t = strtok(optarg, ",")) != optarg)){
			error("invalid subargument: -W%s", optarg);
			continue;
		}
		if((i = getXpass((ic = *t), "-W")) == -1)
			continue;
		if (i==Xdr) {
			parse_driver_options();
			continue;
		}
		while((t = strtok(NULL, ",")) != NULL) {
			if(nlist[i] >= nargs){
				error("too many arguments for pass -W%c", ic);
				continue;
			}
			list[i][nlist[i]++] = t;
		}
		continue;

	case 'L': /* the -L and the directory must go into separate args */
		t = stralloc(2);
		t[0] = '-';
		t[1] = c;
		t[2] = '\0';
		list[Xld][nlist[Xld]++] = t;
		t = stralloc(strlen(optarg));
		strcpy(t, optarg);
		goto checknl;

	case 'l':
	case 'n':
	case 'N':
	case 'q':
	case 's':
	case 'Q':
		t = stralloc(strlen(optarg)+2);
		t[0] = '-';
		t[1] = c;
		t[2] = '\0';
		strcat(t, optarg);
		goto checknl;

#ifndef SCC
	case 'y':
		saflag++;
		continue;
#endif /* not SCC */

	case '?':
		if (strchr(OPTSTR, (char) optopt)){
			if (optopt !='l')
			   error("missing argument after -%c option",optopt);
			continue;
		}
#ifdef SCC
		if (strchr("AgGOpvy#", (char) optopt))
			warn("The -%c option is not a feature of the bundled C compiler",optopt);
		else
			warn("unrecognized option -%c", optopt);
		continue;
#else
		warn("unrecognized option -%c", optopt);
		continue;
#endif /* SCC */

checknl:
		if(nlist[Xld] >= nargs){
			free(t);
			error("Too many ld options", (char *)NULL);
			continue;
		}
		list[Xld][nlist[Xld]++] = t;
		continue;

	case EOF:
		t = argv[optind];
		optind++;

		/* handle "+" options */
		if (*t=='+')
		{
			ic = *++t;

			switch (ic) {

			case 'b':
			case 'f':
				if (fphwflag) error("only one +ffpa, +bfpa or +M can be used");
				if (*++t==NULL)		/* +f or +b */
				 {
				     error("+%s: option no longer supported",
					   --t);
				     continue;
				 }
				else if (strcmp(t,"fpa")==0)
				{
				  /* +ffpa or +bfpa */ 
				  fphwflag = 1;
				  list[Xc2][nlist[Xc2]] = "-D";
				  nlist[Xc2]++;
				}
				else 
				{
				  error ("unrecognized option +%s", --t);
				  continue;
				}

				list[Xccom][nlist[Xccom]] = "--";
				list[Xccom][nlist[Xccom]][1] = ic;
				nlist[Xccom]++;
				list[Xcp2][nlist[Xcp2]] = "--";
				list[Xcp2][nlist[Xcp2]][1] = ic;
				nlist[Xcp2]++;
				if (ic == 'f')
					list[Xc1][nlist[Xc1]] = "-f";
				else
					list[Xc1][nlist[Xc1]] = "-h";
				nlist[Xc1]++;
				
				break;

#ifndef SCC
			case 'e':
				ansiextensions++;
				break;
#endif /* not SCC */

			case 'M':
				if (fphwflag) error("only one of +ffpa, +bfpa or +M can be used");
				fphwflag = 3;
				list[Xccom][nlist[Xccom]] = "-M";
				nlist[Xccom]++;
				list[Xcp2][nlist[Xcp2]] = "-M";
				nlist[Xcp2]++;
				list[Xc1][nlist[Xc1]] = "-l";
				nlist[Xc1]++;
				break;
			case 'N':
				list[Xccom][nlist[Xccom]] = copy(--t);
				list[Xccom][nlist[Xccom]][0] = '-';
				nlist[Xccom]++;
				list[Xcp2][nlist[Xcp2]] = copy(t);
				list[Xcp2][nlist[Xcp2]][0] = '-';
				nlist[Xcp2]++;
				break;
#ifndef SCC
			case 'O':
				if (oflag)
				 {
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
				     warn("+O%c overwrites previous optimization option", *(t+1));
				     /* reset optimizer state to start  */
#endif
				     c0flag = c1flag = c2flag = vflag = oflag = 0;
				 }
				switch (*++t)
				 {
				  case '1':
				     c2flag++;
				     oflag++;
				     break;
				  case 'V':
				     /* since +OV should call same
					pieces as +O2, fall thru */
				     vflag++; 
				  case '2':
				     c1flag++;
				     c2flag++;
				     oflag++;
				     break;
				  case '3':
				     c1flag++;
				     c2flag++;
				     oflag++;
				     c0flag++;
				     break;
				  default:
				     error("unrecognized +Ox option");
				 }
				break;
#endif /* not SCC */

			case 's':
				/* sequential execution of passes */
                                pipeflag = 0; 
				break;
#ifdef TIMES
			case 'T':
                                timeflag = 1;
				break;
#endif TIMES
			case 'x':		/* 20 on, default  Rel. 7.0 */
				break;
			case 'X':		/* 68010 code gen. */
				error("+%c: option no longer supported",ic);
				break;
#ifndef SCC
			case 'y':
				plusyflag++;
				continue;
			case 'z': /* pic word offset */
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
				if (piclong)
				  {
				  piclong = 0;
				  warn("'+z' overides earlier '+Z' option");
				  }
#endif
				++picword;
				break;

			case 'Z': /* pic long offset */
/*  CLL1100030   pkwan  920507   remove unnecessary warnings */
#if 0
				if (picword)
				  {	 
				  picword = 0;
				  warn("'+Z' overides earlier '+z' option");
				  }
#endif
				++piclong;
				break;
#endif /* not SCC */
			default:
#ifdef SCC
				if (strchr("eOyzZ", (char) ic))
					warn("The +%c option is not a feature of the bundled C compiler",ic);
				else
					warn("unrecognized option +%c", ic);
#else
				warn("unrecognized option +%c", ic);
#endif /* SCC */
			}
			continue;
		}

		if((c=getsuf(t))=='c' || c=='s'|| c=='i' || exflag) {
			clist[nc++] = t;
			t = setsuf(t, 'o');
		}
		if(bbaflag)
			list[Xbba][nlist[Xbba]++] = t;
		if (nodup(list[Xld], t)) {
			if(nlist[Xld] >= nargs){
				error("Too many ld arguments", (char *)NULL);
				continue;
			}
			list[Xld][nlist[Xld]++] = t;
			if (getsuf(t)=='o')
				nxo++;
		   }
	   }

/* --------- finished examining options -------- */

	if( !pipeflag ) divorced = 1;
	if( !ansiflag ) {
	     divorced = 1;
	     if (ansiextensions) {
		  warn("+e ignored without -Aa");
		  ansiextensions = 0;
	     }
	}

	if (ccopts1 && verbose)
	    fprintf(stderr, "CCOPTS is : %s\n", ccopts1);
	if (ccopts2 && verbose)
	    fprintf(stderr, "CCOPTS at end of line : %s\n", ccopts2);

	if ((gflag || saflag) && (oflag||c1flag||c2flag||c0flag))
		{
		oflag = 0;
		c0flag = 0;
		c1flag = 0;
		c2flag = 0;
		warn("Optimization options are ignored with '-g' and '-y'");
		}
        if ((picword||piclong) && (proflag||gproflag))
          {
          proflag = gproflag = 0;
          warn("'-p' and '-G' are ignored with '+z' or '+Z'");
          }
        if (plusyflag && !(gflag||saflag))
          {
          plusyflag = 0;
          warn("'+y' is ignored, unless used with '-y' or '-g'");
          }
	
	if (gproflag)
	  nameps[Xcrt0] = "gcrt0.o";
	else if (proflag)
	  nameps[Xcrt0] = "mcrt0.o";

	if (c0flag)
	  {
	  list[Xcp1][nlist[Xcp1]] = "-F";
	  nlist[Xcp1]++;
	  list[Xcp1][nlist[Xcp1]] = "-O";
	  nlist[Xcp1]++;
	  list[Xc0][nlist[Xc0]] = "-U";
	  nlist[Xc0]++;
	  }

	for (i = 0; i < NPASS + NPARTS; i++)
	  {
	     if(!passes[i].pathset && !passes[i].nameset)
		  {
		  char *prefix;
		  if((i==Xas) || (i==Xld)) prefix = binprefix;
		  else if((i==Xcp) && bbaflag) prefix = bbacppprefix;
		  else if((i==Xbba) && bbaflag) prefix = bbagenprefix;
		  else if(i==Xend) prefix = usrlibprefix;
		  else prefix = libprefix;
		  mkpname(i, prefix);
		  }
	     else if(passes[i].pathset)
	      {
		  passes[i].usepath = 1;
		  mkpname(i, NULL);
		  passes[i].alt = 1;
	      }
	  }

	if(nc==0) 
		goto nocom;
   
	if (pflag==0) {
		tmp_infile[ Xcp ]  = NULL;
		tmp_infile[ Xccom ] = 
		     tmp_infile[ Xcp1 ] = tempnam(TEMPDIR, "ctm4");
		tmp_infile[ Xc0 ]  = c0flag ? tempnam(TEMPDIR, "ctm1") : NULL;
		tmp_infile[ Xc1 ]  = c1flag ? tempnam(TEMPDIR, "ctm7") : NULL;
		tmp_infile[ Xcp2 ] = c1flag ? tempnam(TEMPDIR, "ctm2") : NULL;
		tmp_infile[ Xc2 ]  = c2flag ? tempnam(TEMPDIR, "ctm5") : NULL;
		tmp_infile[ Xas ]  = sflag  ? NULL : tempnam(TEMPDIR, "ctm3");
		tmp_infile[ Xld ] = NULL;
		tmp_infile[ Xdummy ] = NULL;

		if ( pipeflag ) initialize_pipe_vars();
	}

	if(eflag)
		dexit();
	signal(SIGCLD, SIG_DFL); 
	signal( SIGPIPE, SIG_DFL );
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, idexit);

/* compile each source file */
	for (i=0; i<nc; i++) {
	     int pass;
	     int j;

#ifdef INSTRUMENTED
		{
		    /* code to send out a UDP packet with information about
		     * the current compiler.  Requires a server that is 
		     * listening for packets.
		     */
		    int s = socket(AF_INET,SOCK_DGRAM,0);
		    char *ptr;
		    struct udp_packet packet;
		    struct sockaddr_in address,myaddress;
		    struct stat statbuf;
		    /* initialize the data */
		    stat(clist[i],&statbuf);
		    packet.size = statbuf.st_size;
		    packet.inode = statbuf.st_ino;
		    strncpy(packet.name,((ptr=strrchr(clist[i],'/'))?ptr+1:clist[i]),14);
		    packet.flag_debugging = (gflag != 0);
		    packet.flag_optlevel1 = (c2flag != 0);
		    packet.flag_optlevel2 = (c1flag != 0);
		    packet.flag_prof = (proflag != 0);
		    packet.flag_gprof = (gproflag != 0);
		    packet.flag_ansi = (ansiflag != 0);
		    packet.flag_ffpa = (fphwflag == 1);
		    packet.flag_m = (fphwflag == 3);
		    packet.flag_warning = (wflag != 0);
		    packet.unused = 0;
		    packet.version = INST_VERSION;
		    /* set up the addresses */
		    address.sin_family = AF_INET;
		    address.sin_port = INST_UDP_PORT;
		    address.sin_addr.s_addr = INST_UDP_ADDR;
		    myaddress.sin_family = AF_INET;
		    myaddress.sin_port = 0;
		    myaddress.sin_addr.s_addr = INADDR_ANY;
		    /* try blasting a packet out, no error checking here */
		    bind(s,&myaddress,sizeof(myaddress));
		    sendto(s,&packet,sizeof(packet),0,&address,
			   sizeof(address));
		    close( s );
		}
#endif /* INSTRUMENTED */

	     /* set up default input file names */
	     for( j=0; j<XNPASS; j++ )
		  infile[ j ] = tmp_infile[ j ];

	     if( pflag ) user_outfile = setsuf( clist[ i ], 'i' );
	     else if( sflag ) user_outfile = setsuf( clist[ i ], 's' );

	     if (nc>1 || verbose)
		  fprintf(stderr, "%s:\n", clist[i]);
	     if ( estranged = ( getsuf(clist[i])=='i' )) {
		  if (pflag) continue;
		  infile[ Xccom ] = infile[ Xcp1 ] = clist[i];
		  goto compile;
	     } 
	     if (getsuf(clist[i])=='s'&&!pflag) {
		  infile[ Xas ] = clist[i];
		  goto assemble;
	     } 

		/* **** PREPROCESS **** */

	     infile[ Xcp ] = clist[ i ];

	     if (pflag) {
		  if(!exflag) ino_clash( clist[i], user_outfile );
	     }
	     av[0] = nameps[Xcp];
	     av[1] = clist[i];
	     if( divorced ) {
		  av[2] = exflag ? "-" : ( pflag ? user_outfile : infile[ Xccom] );
		  na = 3;
	     }
	     else {
		  na = 2;
	     }
	     for(j=0; j < nlist[Xcp]; j++)
		  av[na++] = list[Xcp][j];
#ifdef xcomp300_800
	     av[na++] = "-J";
	     av[na++] = "-I/usr/local/300comp/usr/include";
#endif xcomp300_800

	     if(ansiflag) {
		     /* support generic ansi cpp */
		     av[na++] = "-I/usr/include";

		     /* clean name space predefines */
		     av[na++] = "-D__hp9000s300";
		     av[na++] = "-D__unix";
		     av[na++] = "-D__hpux";
		 }
		    
		if(bbaflag)
			{
			av[na++] = "-DBBA_OPTA";
			if(ansiflag)
				av[na++] = "-DBBA_OPTa";
			}
		if (saflag)
		  {
		  if (ansiflag)
		    {
		    av[na++] = "-G";
		    }
		  else
		    {
		    macrofile = tempnam(TEMPDIR, "ctm6");
		    av[na++] = "-X";
		    av[na++] = macrofile;
		    }
		  }
		if (nlsflag) av[na++] = "-Y" ;
		if (!ansiflag || (ansiflag && ansiextensions))
			av[na++] = "-$";
		av[na]=0;
		if( divorced ) {
		     if( dispatch_pass( Xcp, av, na )) 
         	 	  continue;
		     if (pflag)
			  continue;
		}

		/* **** COMPILE **** */
compile:
		if( separated = ( divorced || estranged )) na = 0;
		if (c0flag || c1flag)
			{
			av[na++] = nameps[Xcp1];
			pass = Xcp1;
			}
		else 
		     {
			av[na++] = nameps[Xccom];
			pass = Xccom;
		   }

		if (picword)
			av[na++] = "-i";
		if (piclong)
			av[na++] = "-I";
		if (proflag) {
			av[na++] = "-Yp";
		} 
		if (vflag)
		        av[na++] = "-V";
		if (wflag)
			av[na++] = "-Yw";
		if (gflag)
			av[na++] = "-Yg";
#ifdef POST80
		else {
		     if( c1flag )
			  av[na++] = "-YO";
		     else if (oflag)
			  av[na++] = "-Yo";
		}
#else /* not POST80 */
		else if (oflag)
			av[na++] = "-Yo";
#endif /* not POST80 */
		if (saflag)
			{
			av[na++] = "-YX";
			if (!gflag)
			  av[na++] = "-Yg";
			}
		if (plusyflag)
			av[na++] = "-YA";
		if (ansiextensions)
		{
        		av[na++] = "-We";
        		av[na++] = "-Y$";
		}

		for (j=0; j < nlist[Xccom]; j++)
			av[na++] = list[Xccom][j];

		if (c0flag || c1flag)
		{
			for (j=0; j < nlist[Xcp1]; j++)
				av[na++] = list[Xcp1][j];
		}

	     if (nlsflag) av[na++] = "-n" ;
	     if( separated )
		  av[na++] = infile[ Xccom];
	     else
		  infile[ Xccom ] = infile[ Xcp1 ] = NULL;
	     if (sflag) {
		  av[na++] = "-Yk";
		  infile[ Xas ] = setsuf(clist[i], 's');
		  ino_clash(clist[i], infile[ Xas ] );
	     }

		if (c0flag || c1flag)
			av[na++] = infile[ (c0flag ? Xc0 : Xc1) ];
		else
		{
			if (oflag)
			     av[na++] = infile[ Xc2 ];
			else 
			     av[na++] = infile[ Xas ];
		}
	     if( !divorced || !pipeflag || estranged ) 
		  /* 'stderr' for 'ccom' will *NOT* be piped */
		  av[na++] ="-YS";

	     av[na] = 0;
	     if( dispatch_pass( pass, av, na ))
		  continue;
	
		if (c0flag || c1flag)
			{
		/* **** PROCEDURE INTEGRATOR  (c.c0) **** */
			if (c0flag)
				{
				/* call c0 */
				na = 0;
				av[na++] = nameps[Xc0];
				for (j = 0; j < nlist[Xc0]; j++)
					av[na++] = list[Xc0][j];
				if (wflag)
					av[na++] = "-w";
				av[na++] = infile[ Xc0 ];
				av[na++] = infile[ Xc1 ];

				av[na] = 0;
				if( dispatch_pass( Xc0, av, na ))
			     		continue;
				}
		/* **** GLOBAL OPTIMIZER (c.c1) **** */
			if (c1flag)
				{
				/* call c1 */
				na = 0;
				av[na++] = nameps[Xc1];
				for (j = 0; j < nlist[Xc1]; j++)
					av[na++] = list[Xc1][j];
				if (verbose)
			  	av[na++] = "-v";
				if (picword || piclong)
			  		av[na++] = "-Z";
				if (wflag)
					av[na++] = "-w";
				av[na++] = infile[ Xc1 ];
				av[na++] = infile[ Xcp2 ];

				av[na] = 0;
				if( dispatch_pass( Xc1, av, na ))
			     		continue;
				}
		/* **** CODE GEN (cpass2) **** */
			/* call code generator */
			na = 0;
			av[na++] = nameps[Xcp2];
			if (proflag)
				av[na++] = "-p";
			if (picword)
			  av[na++] = "-i";
			if (piclong)
			  av[na++] = "-I";
			for (j = 0; j < nlist[Xcp2]; j++)
				av[na++] = list[Xcp2][j];
			av[na++] = infile[ Xcp2 ];
			av[na++] = infile[ Xc2 ];
			av[na] = 0;
		   
			if( dispatch_pass( Xcp2, av, na ))
			     continue;
			}

		/* **** PEEPHOLE OPTIMIZER (c.c2) **** */

		if (oflag) {
			na = 0;
			av[na++] = nameps[Xc2];
			if (picword)
			  av[na++] = "-q";
			if (piclong)
			  av[na++] = "-Q";
			for (j=0; j < nlist[Xc2]; j++)
				av[na++] = list[Xc2][j];
			av[na++] = infile[ Xc2 ];
			av[na++] = infile[ Xas ];
			av[na] = 0;
			if( dispatch_pass( Xc2, av, na ))
			     continue;
		   }
		/* **** ASSEMBLE **** */

assemble:
		if (sflag)
			continue;
		
		na = 0;
		av[na++] = nameps[Xas];
		for (j=0; j < nlist[Xas]; j++)
			av[na++] = list[Xas][j];

		if (proflag)
			av[na++] = "-l";
		if (picword)
		  av[na++] = "+z";
		if (piclong)
		  av[na++] = "+Z";
		av[na++] = "-o";
		if (cflag && nc == 1 && outfile)
			av[na++] = outfile;
		else
			av[na++] = setsuf(clist[i], 'o');
		ino_clash(clist[i], av[na-1]);
		av[na++] = infile[ Xas ];
		av[na] = 0;
	     if( dispatch_pass( Xas, av, na ))
		  continue;

	} /* end for loop for each source file */

nocom:
		/* **** BBAGEN **** */

	if (bbaflag && (cflag==0)) {
		na = i = 0;
		av[na++] = nameps[Xbba];
		av[na++] = "-o";
		av[na++] = (bbafile=tempnam(TEMPDIR,"ctm8"));
		while(i<nlist[Xbba])
		    av[na++] = list[Xbba][i++];
		av[na] = 0;
		callsys(Xbba, av);
	   }

		/* **** LINK **** */

	if (cflag==0 && nlist[Xld]!=0) {
		na = i = 0;
		av[na++] = nameps[Xld];
		av[na++] = passes[Xcrt0].name;

		if (outfile)
		  {
		  av[na++] = "-o";
		  av[na++] = outfile;
		  }

		while(i<nlist[Xld])
		  {
		  /* don't pass along -s if -g or -y was specified */
		  if ((gflag||saflag) && (strcmp(list[Xld][i],"-s")==0))
		    i++;
		  else
		    av[na++] = list[Xld][i++];
		  }
		if (bbaflag)
			av[na++] = bbafile;
		if (!gflag && !saflag && !proflag)
			av[na++] = "-x";
		if (gflag || saflag)
                  av[na++] = passes[Xend].name;
		if (proflag)
		 {
		     av[na++] = "-L";
		     av[na++] = "/lib/libp";
		 }

		av[na++] = "-lc";
		av[na] = 0;

		eflag |= callsys(Xld, av);
		if (nc==1 && nxo==1 && eflag==0)
			cunlink(setsuf(clist[0], 'o'));
		if (bbaflag)
			cunlink(bbafile);
	   }
	dexit();
	}

/* ----------------------------------------------------- */
idexit()
{
	eflag += 100;
	dexit();
}

/* ----------------------------------------------------- */
dexit() {
    int i;

    if( child ) exit( 0377 );  /* children shouldn't be unlinking */

    if ( !pflag ) {
	 for( i = 0; i < NPASS; i++ ){
	      if( tmp_infile[ i ] != NULL ) {
		   unlink( tmp_infile[ i ] );
	      }
	 }
    }
    exit(eflag);
}

/* ----------------------------------------------------- */
/*VARARGS*/
error(s, a, b)
char *s, *a, *b;
{
	fprintf(stderr, "%s: ",drivername);
	fprintf(stderr, s, a, b);
	putc('\n', stderr);
	cflag++;
	eflag++;
}

/* ----------------------------------------------------- */
/*VARARGS*/
warn(s, a, b)
char *s, *a, *b;
{
	if (wflag) return; 
	fprintf(stderr, "%s: (warning) ",drivername);
	fprintf(stderr, s, a, b);
	putc('\n', stderr);
}


/* ----------------------------------------------------- */
getsuf(as)
char as[];
{
	register int c;
	register char *s;
	register int t;

	s = as;
	c = 0;
	while(t = *s++)
		if (t=='/')
			c = 0;
		else
			c++;
	s -= 3;
	if (c>2 && *s++=='.')
		return(*s);

	return(0);
}

/* ----------------------------------------------------- */
char *
setsuf(as, ch)
char *as;
{
	register char *s, *s1;

	s = s1 = copy(as);
	while(*s)
		if (*s++ == '/')
			s1 = s;
	s[-1] = ch;
	return(s1);
}

/* ----------------------------------------------------- */
char *
formatoption(opt)
char *opt;
{
register char *p;

for(p = opt; *p != '\0'; ++p)
  if (*p == ' ')
    break;
if(*p == '\0') /* no spaces, just return string */
  return(opt);
/* otherwise, surround string in ' */
p = stralloc(strlen(opt)+2);
return(strcat(strcat(strcpy(p,"\'"),opt),"\'"));
}

/* ----------------------------------------------------- */
callsys( pass, v )
    int pass;			/* the index into the passes array */
    char *v[]; 
{
	int t, status;
	char **p;
	char *altpass;

	if (debug) {
		fprintf(stderr, "callsys %s:", passes[pass].name);
		for (p=v; *p != NULL; p++)
			fprintf(stderr, " %s", *p);
		fprintf(stderr, " .\n");
		return(0);
	}

	if (verbose) {
		if ( v[0] != nameps[Xld]) fprintf(stderr, "\t");
		fprintf(stderr, "%s ", passes[pass].name);
		t = 0;
		while( v[++t]  )
		  fprintf(stderr, " %s", formatoption(v[t]));
		fprintf(stderr, "\n");
	}

#ifdef TIMES
	if (timeflag)
		before = times(&buffer1);
#endif TIMES
	if ((t=fork())==0) {
		execv(passes[pass].name, v);
		if(passes[pass].alt != 0){
			altpass = stralloc(4 + strlen(passes[pass].name));
			strcpy(altpass, "/usr");
			strcat(altpass, passes[pass].name);
			execv(altpass, v);
			error("Can't find %s, %s", passes[pass].name, altpass);
			dexit(300);
		}
		error("Can't find %s", passes[pass].name);
		dexit(300);
	} else
		if (t == -1) {
			error("Can't fork; Try again");
			return(1);
		}
	while(t!=wait(&status))
		;
	if (t = status&0377) {
		if (t!=SIGINT)
			error("Fatal error in %s", passes[pass].name);
		eflag += 200;
		dexit();
	}

#ifdef TIMES
	if (timeflag)
	{
		after = times(&buffer2);
		/* make sure 'after' was successfully and correctly updated */
		if (after < before)
		{
			fprintf(stderr,"time: command terminated abnormally.\n");
			dexit(2);
		}

		if (!verbose) fprintf(stderr,"%s:\t", nameps[pass]);
		fprintf(stderr,"\t(secs) \treal %5.2f", (float)(after-before)/HZ);
		fprintf(stderr,"\tuser %5.2f", (float)(buffer2.childuser - buffer1.childuser)/HZ);
		fprintf(stderr,"\tsys  %5.2f", (float)(buffer2.childsys  - buffer1.childsys)/HZ);
		/*
		 * fprintf(stderr,"\tU+S  %5.2f", (float)((buffer2.childuser - buffer1.childuser)
			   + (buffer2.childsys  - buffer1.childsys))/HZ);
		 */
		fprintf(stderr,"\n");
		fprintf(stderr,"\n");
	}
#endif TIMES

	return((status>>8) & 0377);
}

/* ----------------------------------------------------- */
char *
copy(s)
register char *s;
{
	register char *ns;

	ns = stralloc(strlen(s));
	return(strcpy(ns, s));
}

/* ----------------------------------------------------- */
char *
stralloc(n)
int	n;
{
	char *malloc();
	register char *s;

	s = malloc(n+1);
	if (s==NULL) {
		error("out of space", (char *)NULL);
		dexit();
	}
	return(s);
}


/* ----------------------------------------------------- */
nodup(l, os)
char **l, *os;
{
	register char *t, *s;
	register int c;

	s = os;
	if (getsuf(s) != 'o')
		return(1);
	while(t = *l++) {
		while(c = *s++)
			if (c != *t++)
				break;
		if (*t=='\0' && c=='\0')
			return(0);
		s = os;
	}
	return(1);
}

/* ----------------------------------------------------- */
cunlink(f)
char *f;
{
	if (f==NULL)
		return;
	unlink(f); 
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
		       /* a -t<pass>,<path name> was specfied */
		       passes[n].name = stralloc(strlen(pname)+1);
		       strcpy(passes[n].name, pname);
		   }
		  else if(!passes[n].nameset && passes[n].pathset)
		   {
		       /* a multipe substition was request by
			* -t<pass_list>,<path>
			* so save the new path name */
		       passes[n].path = stralloc(strlen(pname)+1);
		       strcpy(passes[n].path, pname);
		   }
		  else 
		   {
		       passes[n].name = stralloc(strlen(pname) +
						 strlen(nameps[n]));
		       strcpy(passes[n].name, pname);
		       strcat(passes[n].name,nameps[n]);
		   }
	      }
	     else
	      {
		  /* use the previously specified path to create the
		   * full path name to the piece
		   */
		  passes[n].name = stralloc(strlen(passes[n].path) +
					    strlen(nameps[n]));
		  strcpy(passes[n].name,passes[n].path );
		  strcat(passes[n].name,nameps[n]);
	      }
	 }
}


/* ----------------------------------------------------- */
getXpass(c, opt)
char	c, *opt;
{
	switch (c) {
	default:
		warn("unrecognized pass name: '%s%c'", opt, c);
		return(-1);
	case 'd':
		if (opt[1] == 't')
			error("cannot use -t with pass d");
		else
		return(Xdr);

	case '0':
		return(Xcp1);
	case '1':
		return(Xcp2);
	case '2':
		return(Xc2);

	case 'c':
		return(Xccom);
	case 'p':
		return(Xcp);
	case 'a':
		return(Xas);
	case 'l':
		return(Xld);
	case 'g':
		return(Xc1);
	case 'b':
		if (bbaflag)
		  {
		  return(Xbba);
		  }
		else
		  {
		  warn("unrecognized pass name: '%s%c'", opt, c);
		  return(-1);
		  }
	case 's':
		return(Xcrt0);
	case 'e':
		return(Xend);
	case 'i':
		return(Xc0);
	}
}

/* ----------------------------------------------------- */
parse_driver_options()
{
	char *t;
	char ic;

	/* initial call to strtok is case 'W' of main() */
	while((t = strtok(NULL, ",")) != NULL) {
		if (*t=='-'&& strlen(t)==2)
		{
			ic = *++t;
			switch (ic) {
				case 'x':		/* 20 on */
					break;
				case 'X':		/* 20 off */
					error("-Wd,-%c: option no longer supported", ic);
					break;
				default:
					warn("unrecognized driver option -Wd,-%c", ic);
			}
		}
		else warn("unrecognized driver option -Wd,%s", t);
	}
}

/* ----------------------------------------------------- */
#include <sys/types.h>
#include <sys/stat.h>

void ino_clash(src,obj)
char *src;
char *obj;
{

    ino_t        src_ino, obj_ino;
    struct stat  r,s;

    if (stat (src, &r)) return;
    src_ino = r.st_ino;
    if (stat (obj, &s)) return;
    obj_ino = s.st_ino;
    if (src_ino == obj_ino)
    {
	/*
	 * the inode number are the same, but are they on the
	 * same device?
	 */
	if (r.st_dev == s.st_dev)
	 {
	     /*
	      * hmm, the device seems to be the same, but are the
	      * files on the same node?
	      */
	     if (!(s.st_remote ^ r.st_remote))
	      {
		  /*
		   * either both remote or both local.
		   * if local look at the cluster ID of the files
		   * machine in case this is Diskless
		   * if remote and network special files
		   * are the same return error
		   */
		  if(r.st_remote)
		   {
		       if ( (s.st_netdev == r.st_netdev) &&
			   (s.st_netino && r.st_netino))
			{
			    error("%s will over-write existing %s",obj,src);
			    dexit();
			}
		   }
		  else
		   {
		       /* maybe we're running DISKLESS ? */
		       if (r.st_cnode == s.st_cnode &&
			   r.st_realdev == s.st_realdev)
			{
			    error("%s will over-write existing %s",obj,src);
			    dexit();
			}
		   }
	      }
	 }
    }
}

/* ----------------------------------------------------- */


/*
 * PIPES
 *
 * This code was added to support the use of pipes instead of temporary
 * files.  The code consists of two routines 'pipesys' and 'pipeclear'.
 * The pipes option calls 'pipesys' instead of 'callsys' for each phase.
 * Pipesys merely stores the command and arguments in a buffer, instead of
 * execing a process.  Pipeclear is then called to execute all commands in
 * a pipeline.
 */
 
int pipesys(pass, v, na)
     int pass;          /* pass id */
     char *v[];	        /* argument vector */
     int na;		/* # of arguments */ 
{

     char *f = passes[pass].name   ;  /* name of pass */
     int cnt,cnt2;
     char *ptr;
     if (passes[pass].alt != 0) {
	  ptr = stralloc(strlen(f) + 4);
	  strcpy(ptr,"/usr");
	  strcat(ptr,f);
	  pipephase[pipenum].altf = ptr;
     }
     else {
	  pipephase[pipenum].altf = NULL;
     }
     ptr = stralloc(strlen(f));
     strcpy(ptr,f);
     pipephase[pipenum].f = ptr;
     pipephase[pipenum].alt = passes[pass].alt;
     pipephase[pipenum].v = (char **) stralloc(na * SCPTR + 3);
     pipephase[pipenum].pid = 0;
     pipephase[pipenum].term = 0;
     pipephase[pipenum].pass = pass;
     cnt2 = 0;
     for (cnt = 0;cnt < na;cnt++) {
	  /* Delete names of input files, except Xcp
	   *      [Xcp different because of possible cpp/ccom merge]
	   */
	  if( 
	     strcmp(v[cnt],infile[ Xc0 ])  == 0  ||
	     strcmp(v[cnt],infile[ Xc1 ])  == 0  ||
	     strcmp(v[cnt],infile[ Xcp2 ]) == 0  ||
	     strcmp(v[cnt],infile[ Xas ])  == 0  ||
	     strcmp(v[cnt],infile[ Xccom ])  == 0  || /* includes Xcp1 */
	     strcmp(v[cnt],infile[ Xc2 ])  == 0 )

	       continue; 
	  
	  ptr = stralloc(strlen(v[cnt]));
	  strcpy(ptr,v[cnt]);
	  pipephase[pipenum].v[cnt2++] = ptr;
     }

     if( ( pipenum == 0 ) && ( pass != Xcp /* possible cpp/ccom merge */)) {
	  /* first in pipe: get input file */ 
	  if( infile[ pass ] != NULL )
	       pipephase[pipenum].v[cnt2++] = infile[ pass ];
     }
     pipephase[pipenum++].v[cnt2] = NULL;
}

/* pipeclear - performs piped execvs
 */

int pipeclear( pass ) int pass; /* 'pass' is pass calling pipeclear() 
				   'pipenum' reset to 0 prior to return 
				   'pipenum' = actual number of pieces, i.e.,
				               not relative to 0 */
{
     int i,j;
     int file_id;
     int pid;
     char *outputfile = ( infile[ nextpass[ pass ]] != NULL ) ?
	                       infile[ nextpass[ pass ]] : user_outfile;
     pipe_killer = pipenum-1;
     pipe_error = 0;
     
     if( pipenum == 0 ) return( 0 );       /* nothing in the pipeline */
     
     if (verbose || debug) {
	  for (i = 0;i < pipenum;) {
	       if (debug) fprintf(stderr,"pipesys ");
	       else fprintf(stderr,"\t");
	       fprintf(stderr,"%s: ",pipephase[i].f);
	       for (j = 0;;j++) {
		    if (pipephase[i].v[j] == NULL) {break;}
		    fprintf(stderr," %s",formatoption(pipephase[i].v[j]));
	       }
	       if (++i < pipenum) fprintf(stderr," |\n");
	  }
	  if( outputfile != NULL )
	       fprintf(stderr," #[output file = '%s']", outputfile);
	  fprintf(stderr,"\n");
	  if (debug) {
	       pipenum = 0;
	       return(0);
	  }
     }

#ifdef TIMES
        if (timeflag)
                before = times(&buffer1);
#endif TIMES

     /* 
      * Set up pipes for processes and standard error
      */
     for( i = 0; i < pipenum-1; i++ ) {
	  if( ( pipe( pcpipe[i]) != 0 ) || ( pipe( std_err[ i ] ) != 0 )) {
	       error("Fatal error, cannot open pipes, errno = %d", errno );
	       dexit(300);
	  }
     }
     if( pipe( std_err[ pipenum-1 ] ) != 0 ) {
	  error("Fatal error, cannot open pipes, errno = %d", errno );
	  dexit( 300 );
     }
     /* 
      * Set up processes
      */
     for( i = 0; i < pipenum; i++ ) {

	  switch ( pid = fork() ){

	     case 0:
	       /* 
		* Set up plumbing (except for final output)
		*/
	       child = 1; 
	       for( j = 0; j < pipenum-1; j++ ) {
		    if ( i == j) { /* output pipe for piece 'i<pipenum-1' */
			 close(1);
			 dup( pcpipe[j][1] );
		    }
		    else if (j == i-1) { /* input pipe for piece 'i>0' */
			 close(0);
			 dup( pcpipe[j][0] );
 		    }
		    close( pcpipe[j][0] );
		    close( pcpipe[j][1] );
	       }
	       /* Redirect stderr */

	       close(2);
	       dup( std_err[ i ][ 1 ] );
	       for( j=0; j<pipenum; j++ ){
	       	    close( std_err[ j ][ 0 ] );
 		    close( std_err[ j ][ 1 ] );
	       }

	       /*
		* Set up outputfile for last process in pipe
		*/
	       if( ( i == pipenum-1 ) && ( outputfile != NULL )) {
		    if( ( file_id = open( outputfile, 
					  O_CREAT|O_TRUNC|O_WRONLY,
					  0666 & ~umask(0) ))
		               == -1 ) {
			 error("Can't open outputfile file %s", outputfile );
			 dexit(300);
		    }
		    close(1);
		    dup( file_id );
		    close( file_id );
	       }
	       /* 
		* Run processes 
		*/
	       execv(pipephase[i].f,pipephase[i].v);
	       if (pipephase[i].alt != 0) {
		    execv(pipephase[i].altf,pipephase[i].v);
		    error("Can't find %s or %s",pipephase[i].f,pipephase[i].altf);
		    dexit(300);
	       }
	       error("Can't find %s",pipephase[i].f);
	       dexit(300);

	     case -1:
	       error("Fatal error, can't fork processes\n",(char *) NULL);
	       dexit(300);	 

	     default:
	       pipephase[ i ].pid = pid;
	       close( std_err[ i ][ 1 ] );

	  } /* end switch */
     } /* end for( i = 0; ... ) */

     for( i = 0; i < pipenum-1; i++ ) {
	  close( pcpipe[i][0] );
	  close( pcpipe[i][1] );
     }

#ifdef PDEBUG
     fprintf( stderr, "\n...%d processes:\n", pipenum );
     for( i = 0; i < pipenum; i++ ){
	  fprintf( stderr, "\t\tpid, name = %d, %s\n", pipephase[i].pid,
		                                         pipephase[i].f);
     }
#endif

     /* dump (redirected) stderr when and if appropriate;
      * keep track of terminated passes
      */
     for( i=0; i<pipenum; i++ ) {
	  if( i<=pipe_killer ) dump_std_err( i );
	  if( !pipephase[ i ].term ) 
	       await_death_of( i );
     }

#ifdef TIMES
        if (timeflag)
        {
                after = times(&buffer2);
                /* make sure 'after' was successfully and correctly updated */
                if (after < before)
                {
                        fprintf(stderr,"time: command terminated abnormally.\n");
                        dexit(2);
                }

		if( !verbose ){
		     for( i = 0; i < pipenum - 1; i++ ) {
			  fprintf( stderr, "%s|", 
				   nameps[ pipephase[ i ].pass ] );
		     }
		     fprintf( stderr, "%s:\t", 
			      nameps[ pipephase[ pipenum-1 ].pass ] );
		}
		fprintf(stderr,"\t(secs) \treal %5.2f", (float)(after-before)/HZ);
		fprintf(stderr,"\tuser %5.2f", (float)(buffer2.childuser - buffer1.childuser)/HZ);
		fprintf(stderr,"\tsys  %5.2f", (float)(buffer2.childsys  - buffer1.childsys)/HZ);
		fprintf(stderr,"\n");
		fprintf(stderr,"\n");
	}
#endif TIMES

     /* All done
      */
    
     for( i=0; i<pipenum; i++ )
	  close( std_err[i][0] );

     pipenum = 0;
     return( pipe_error );
}

			 
/* global initializations for PIPES */

initialize_pipe_vars() {
     
     pass_in_pipe[ Xcp ] = pass_in_pipe[ Xccom ] = pass_in_pipe[ Xcp1 ]
	= pass_in_pipe[ Xcp2 ] = pass_in_pipe[ Xc2 ] = pass_in_pipe[ Xas ] 
	= 1;
     pass_in_pipe[ Xc0 ] =
     pass_in_pipe[ Xc1 ] = pass_in_pipe[ Xld ] = pass_in_pipe[ Xdummy] = 0;

     nextpass[ Xcp  ] = pflag  ? Xdummy : ( c1flag ? Xcp1   : Xccom );
     nextpass[ Xccom  ] = c2flag ? Xc2    : ( sflag  ? Xdummy : Xas );
     nextpass[ Xcp1 ] = c0flag ? Xc0 : Xc1;
     nextpass[ Xc0  ] = c1flag ? Xc1 : Xcp2;
     nextpass[ Xc1  ] = Xcp2;
     nextpass[ Xcp2 ] = Xc2;
     nextpass[ Xc2  ] = sflag ? Xdummy : Xas;
     nextpass[ Xas  ] = Xdummy;
     nextpass[ Xld  ] = Xdummy;     
}

int dispatch_pass( pass, av, na )
     /* decides what to do: callsys() or pipesys() (and perhaps pipeclear() ) 
      *
      * pipesys() will not be called as expected when the next pass will
      *     not be pipelined and there is only one pass in the pipe -
      *     callsys() is called instead
      *
      * if pipesys() is to be called and the next pass will not be pipelined,
      *     then the pipeclear() is called
      */
     int pass;
     char *av[];
     int na; 
{
     int callret = 0;

     if( pipeflag && pass_in_pipe[ pass ] ) {
	  if( nextpass[ pass ] != Xdummy
	      && pass_in_pipe[ nextpass[ pass ]] ) {

	       pipesys( pass, av, na ); 
	  }
	  else { /* put pass in pipe only if pipe nonempty */
	       if( pipenum ) {
		    pipesys( pass, av, na );
		    callret = pipeclear( pass );
	       }
	       else callret = callsys( pass, av );
	  }
     }
     else callret = callsys( pass, av );

     if( callret ) {
	  eflag++;
	  cflag++;
     }
     return( callret );
}

dump_std_err( i )   /* dumps redirected stderr for pipephase i */ {
     char buffer[256];
     FILE *file_des;
     
#ifdef PDEBUG
     fprintf( stderr, "\n...dumping stderr for %s\n", 
	      nameps[ pipephase[ i ].pass ] );
#endif
     if( ( file_des = fdopen( std_err[ i ][ 0 ], "r" )) == NULL ) {
	  error( "can't access (redirected) stderr for pass %s", 
		nameps[ pipephase[ i ].pass ] );
          dexit( 300);
     }

     for( ; fgets( buffer, 256, file_des ) != NULL ; ) {
	  fputs( buffer, stderr );
     }
#ifdef PDEBUG
     fprintf( stderr, "\n\t\t...done!!!\n");
#endif
     fclose( file_des );

}


await_death_of( pass ) {
 
/* wait for children to die; return when pass dies */

     int pid;
     int j;
     int done = 0;
     int status;
#ifdef PDEBUG
     fprintf( stderr, "\n...awaiting the death of a child\n" );
#endif
     pid = wait( &status );
#ifdef PDEBUG
     fprintf( stderr, "\n... %d returned!, status = 0%o\n", pid, status);
#endif
     
     /* mark phase as terminated */
     
     for( j = 0; j < pipenum ; j++ ) {
	  if( pipephase[ j ].pid == pid ) {
	       pipephase[ j ].term = 1;
	       if( pass == j ) done++;;
	       break;
	  }
     }
     /* see if abnormal return */
     
	  if( status & 0xffff ) {
	       status &= 0177;
	       pipe_error++;
	       
	       if( status != SIGPIPE ) {

		    if( status && ( status != SIGINT )) {
			 error("Fatal error in %s, signal = %d",
			       pipephase[ j ].f, status );
		    }
		    if( j < pipe_killer ) {
			 pipe_killer = j;
			 interrupt_later_passes( pipe_killer );
		    }
	       }
	  } /* end abnormal return */
     if( done ) return;
     else await_death_of( pass );
}

interrupt_later_passes( j ) {

	  /* kill all processes later in pipeline */
	  for( j = pipe_killer+1; j < pipenum ; j++ ) {
#ifdef PDEBUG
	       fprintf( stderr, "\t\tinterrupting %d ... ", pipephase[ j ].pid );
#endif
	       kill( pipephase[ j ].pid, SIGINT );
#ifdef PDEBUG
	       fprintf( stderr, "done !!\n" );
#endif
	  }
}

