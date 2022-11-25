static char *HPUX_ID = "@(#) $Revision: 70.11 $";

/*LINTLIBRARY*/
#include <fcntl.h>
#include <msgbuf.h>
#include <stdio.h>
#include <nl_ctype.h>
#include <nl_types.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <values.h>
#include <langinfo.h>

#include <ulimit.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>			/* POSIX 12/12/91  defines LINE_MAX */

/* AW:
 * POSIX changes made on 12/12/91.
 * All changes are marked with the string  "POSIX 12/12/91"
 * These changes correspond to POSIX.2 as of draft 11.2
 */

#define	C	20
#define	N	16
#define MTHRESH	 8 /* threshhold for doing median of 3 qksort selection */
#define NF	10 /* number of fields */
#define TREEZ	32 /* no less than N and best if power of 2 */

#define min(a,b) ((a) < (b)) ? a : b	/* MINIMUM function */

/*
 * Memory administration
 *
 * Using a lot of memory is great when sorting a lot of data.
 * Using a megabyte to sort the output of `who' loses big.
 * MAXMEM, MINMEM and DEFMEM define the absolute maximum,
 * minimum and default memory requirements.  Administrators
 * can override any or all of these via defines at compile time.
 * Users can override the amount allocated (within the limits
 * of MAXMEM and MINMEM) on the command line.
 *
 * For PDP-11s, memory is limited by the maximum unsigned number, 2^16-1.
 * Administrators can override this too.
 * Arguments to core getting routines must be unsigned.
 * Unsigned long not supported on PDP-11s.
 */

#ifndef	MAXMEM
#  ifdef pdp11
#     define	MAXMEM ((1L << 16)-1)
#     define	MAXUNSIGNED ((1L << 16)-1)
#  else 
#     ifdef hp9000s300
#        define	MAXMEM	1048576		/* 1 Megabyte maximum on Motorolas */
#     else
#        define	MAXMEM	16777216	/* 16 Megabyte maximum on PA & others */
#     endif
#  endif
#endif

#ifndef	MINMEM
#  define	MINMEM	  16384	/* 16K minimum */
#endif

#ifndef	DEFMEM
#  define	DEFMEM	  131072 /* 128K default */
#endif

#ifndef hp9000s500
#  define UNSIGNED
#else hp9000s500
#  define UNSIGNED	unsigned
#endif hp9000s500

#define ASC 	0
#define MON	2
#define NUM	1

#define FALSE	0
#define TRUE	1

#if defined(NLS) || defined(NLS16)
#define	isblank(c) \
    ((c) == ' ' || (c) == '\t' || \
     (int)(unsigned char)(c) == _nl_space_alt)
#else
#define	isblank(c) ((c) == ' ' || (c) == '\t')
#endif /* defined(NLS) || defined(NLS16) */

/* POSIX 12/12/91
*/
#define DEFERR 2		/* normal error exits should be >= 2 */


FILE	*os;
char	*dirtry[] = {"/usr/tmp", "/tmp", NULL};
char	**dirs;
char	file1[100];
char	*file = file1;
char	*filep;
#define NAMEOHD 12 /* sizeof("/stm00000aa") */
#define TOFOLD(c)	toupper(c)
#define ABMON_MAX	7
int	nfiles;
UNSIGNED int	*lspace;
UNSIGNED int	*maxbrk;
unsigned tryfor;
long longtryfor;
unsigned alloc;
char bufin[LINE_MAX], bufout[LINE_MAX];	/* Use setbuf's to avoid malloc calls.
					** malloc seems to get heartburn
					** when brk returns storage.
					*/

long    max_open_files;		/* It is a tunable parameter and is configured
				 * at the time of system generation */
int	maxrec;
int	cmp(), cmpa();
int	(*compare)() = cmp; 	/*force use of cmp, cmpa does byte compare */
char	*eol();			/* move forward until end-of-line */
int	term();
int 	mflg;			/* merge only, input files assumed sorted */
int	nway;
int	cflg;			/* check that 1 input file is already sorted */
int	uflg;			/* unique, suppress all but 1 of dup lines */
char	*outfil;
int	unsafeout;		/* kludge to assure -m -o works*/
char	tabchar;		/* field break character for the -t option */
int	RadixChar = '.',	/* radix character */
	isOneToOne,		/* 1 to 1 collation */
	isOneByte,		/* 1-byte character encoding */
	isC_Locale;		/* C locale or n-computer */
nl_catd catd;

/* POSIX 12/12/91
*/
unsigned char	ThousSep = ',';	/* default value for Thousands Separator */
char	*tmpd;			/* for getting the TMPDIR environment */
int	kflg = 0;		/* to ensure that -k options start with 1 */

char USE_STR[] = "invalid use of command line options"; /* catgets 1 */
#define	USE	(catgets(catd,NL_SETN,1, USE_STR))

int 	eargc;
char	**eargv;
struct btree {
    char *rp;
    int  rn;
} tree[TREEZ], *treep[TREEZ];
int	blkcnt[TREEZ];
char	**blkcur[TREEZ];
long	wasfirst = 0, notfirst = 0;
int	bonus;

#if defined(NLS) || defined(NLS16)
extern int errno;
extern char *getenv();
extern int putenv();
extern char *nl_langinfo();
extern  int _nl_space_alt;
#endif /* defined(NLS) || defined(NLS16) */

unsigned char   fold[256] = {
        0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
        0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
        0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
        0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
        0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
        0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
        0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
        0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
        0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
        0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
        0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
        0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
        0140, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
        0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
        0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
        0130, 0131, 0132, 0173, 0174, 0175, 0176, 0177,
        0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
        0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
        0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
        0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
        0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
        0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
        0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
        0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
        0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
        0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
        0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
        0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
        0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
        0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
        0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
        0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377
};

unsigned char nofold [256] = {
	0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
	0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
	0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
	0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
	0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
	0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
	0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
	0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
	0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
	0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
	0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
	0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
	0140, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
	0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
	0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
	0170, 0171, 0172, 0173, 0174, 0175, 0176, 0177,
	0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
	0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
	0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
	0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
	0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
	0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
	0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
	0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
	0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
	0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
	0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
	0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
	0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
	0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
	0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
	0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377
};

char zero[256];

char nonprint [256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

char dict [256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

struct	field {
	unsigned char *code;
	char *ignore;
	int fcmp;
	int rflg;
	int bflg[2];
	int m[2];
	int n[2];
}	fields[NF+1];
struct field proto = {
	nofold,
	zero,
	ASC,
	1,
	0,0,
	0,-1,
	0,0
};
int	nfields = 0;
int 	error = DEFERR;			/* POSIX 12/12/91 default error */
char	*setfil();

extern  char    *optarg;                /* for getopt */
extern  int     optind, opterr;         /* for getopt */
	int	curr_optind;		/* for getopt, ? case */
	int     c = 0;                  /* for getopt, option character */

void 	*realloc();			/* sbrk, brk calls replaced with 
					 * realloc 
					 */
void 	*malloc();

main(argc, argv)
	int	argc;
	char	**argv;
{
	int len;			/* for option parsing,    */
	char *tmp;			/* case:  k, t, z         */
	char tmpbuf[20];		/* used in sizeof(tmpbuf) */

	register a;
	char *arg;
	struct field *p, *q;
	int i;
	long ulimit();

#if defined(NLS) || defined(NLS16)
	init_nls();
#endif /* defined(NLS) || defined(NLS16) */

	max_open_files = sysconf(_SC_OPEN_MAX);

	if (argc - 1 > max_open_files - 4) {
		sprintf(tmpbuf, "%ld", max_open_files - 4);
		diag("Number of input files exceeds limit of ", tmpbuf);
		exit(DEFERR);
	}

	/* close any file descriptors that may have been */
	/* left open -- we may need them all		*/
	for (i = 4; i < max_open_files; i++)	/* std* and msgcat */
		(void) close(i);

	/* POSIX 12/12/91
	 Get the TMPDIR environment variable if present. */
	if((tmpd = getenv("TMPDIR"))) dirtry[0] = tmpd;

	copyproto();
	initree();
	eargv = argv;
	tryfor = DEFMEM;

	opterr = 0;			/* getopt local error processing */
	do {				/* getopt with '+' options */
	    curr_optind = optind;	/* for getopt, ? case */
	    while (c != EOF &&
		   (c=getopt(argc, argv, "T:Mbdfinrclk:mo:t:uy:z:"))!=EOF) {

		switch (c) {
		case 'T':		/* temporary work file directory */
		    if ((strlen(optarg) + NAMEOHD) > sizeof(file1)) {
			diag(catgets(catd,NL_SETN,3, "path name too long: "),
			   *argv);
			exit(DEFERR);
		    }
		    else
			dirtry[0] = optarg;
		    break;

		case 'o':		/* output file name */
		    outfil = optarg;
		    break;

		case 'y':		/* kmem, amount of main memory to use */
		    /*
		     * If getopt has optarg pointing at a complete
		     * argument we must have the case of "-y 0".  This
		     * is treated as a no argument y (max memory) and a
		     * file named "0". In this case, we must fool
		     * getopt into reprocessing optarg as an argument
		     * (so we decrement optind).
		     *
		     * If getopt has optarg pointing in the middle of
		     * an argument, we must have the case of "-y0".
		     * This is treated as y argument with minimum (0)
		     * memory.
		     */
		    if (argv[optind-1] == optarg)
		    {
		        field("y", nfields>0 ?1:0);
			optind--;
		    }
		    else
		        field(argv[optind-1]+1, nfields>0 ?1:0);

		    break;

		case 'k':	/* parse -k field_start[type],field_end[type] */
		    {		/* sample: sort -t: -k2n,3 -k0r,1 /etc/passwd */

			int i,j = 0;

			/* UCSqm00972 : problem here. sort -t^ -k5 not working
			 * if ((len = strlen(optarg)+1) < sizeof(tmpbuf))
			 */
			if ((len = strlen(optarg)) < sizeof(tmpbuf))
			    tmp = tmpbuf;
			else 
			    tmp = (char *)malloc(len+1);

			for (i=0; i<len && optarg[i] != ','; i++)
			{
			    if (optarg[i] != '\0')
			        tmp[i] = optarg[i];
			}
			tmp[i] = '\0';
		        if (++nfields>=NF) {
			    diag(catgets(catd,NL_SETN,4, "too many keys"), "");
			    exit(DEFERR);
		        }
			/* POSIX 12/12/91
			*/
			kflg = 1;		/* set it here */
		        copyproto();
		        field(tmp, 0);		/* +pos */

			if (optarg[i] == ',')	/* more to come? */
			{
			    for (++i; i<len && optarg[i] != '\0'; i++)
			        tmp[j++] = optarg[i];
			    tmp[j] = '\0';
		            field(tmp, nfields>0 ?1:0);	/* -pos */
			}

			if (tmp != tmpbuf)	/* free if malloced */
			    free(tmp);
		    }
		    break;

		case 't':		/* field separator character */
		case 'z':		/* record size for merge buffers */
		    /* pass options with parameter */
		    /*
		     * For these, field() wants the args concatenated
		     * together.  So, we do this into a temporary
		     * string.  If the string is relatively small, we
		     * use a local buffer for it, otherwise we use
		     * malloc (and then free it later).
		     */
		    {
			if ((len = strlen(optarg)+2) < sizeof(tmpbuf))
			    tmp = tmpbuf;
			else
			    tmp = (char *)malloc(len+1);

			tmp[0] = c;
			strcpy(tmp+1, optarg);
			field(tmp, nfields>0 ?1:0);
			if (tmp != tmpbuf)
			    free(tmp);
		    }
		    break;

		case '?': /* -(numeric) */
		    if(curr_optind == optind) break;
		   /* check for '-y' here, as this option is allowed even
		    * without an argument. All others T,k,o,t,z require
		    * arguments.
		    */
		    if(*(argv[optind-1]+1) == 'y') {
			    field("y", nfields>0 ?1:0);
			    break;
		    }
		   /* will enter here for each numeric in the -nnn option,
		    * we need to process just once.
		    * if not a number, we have an invalid option 
		    */
		    if(!isdigit(argv[curr_optind][1])) {
			diag(USE, "");
			exit(DEFERR);
		    }
		    field(argv[curr_optind]+1, 1);	/* -pos */
		    curr_optind = optind;		/* update curr_optind */
		    break;

		default: 	/* pass options without parameter */
		    {
			/*
			 * field() wants an option-like string, so
			 * give it one with only one option.
			 */

			char tmpstr[2];

			tmpstr[0] = c;	/* create a string with this option */
			tmpstr[1] = '\0';
			field(tmpstr, nfields>0 ?1:0);
		    }
		    break;

		} /* switch (c) */

		/* Handle kflag here.
		 * POSIX 12/12/91         POSIX defines...
		 * +w.x -y.z  => -kw+1.x+1,y.0    if z = 0,
		 * 	      => -kw+1.x+1,y+1.z  if z > 0.
		 * all k option numbers excepting .z have been decremented
		 * by one to get to the obsolescent format.
		 * Here z is handled. If z = 0, y is to be incremented.
		 * (see field()).
		*/
		if(kflg) {
			kflg = 0;		/* ensure reset */
			p = &fields[nfields];
			if(p->m[1] >= 0)	/* if y.z defined */
				if(p->n[1] == 0) p->m[1]++;
		}

	    } /* while (c=getopt(argc,argv,)) */

	    if (optind > 0 && strcmp(argv[optind-1], "--") == 0)
		c = EOF;
	    else
		if (argv[optind][0] == '+') {	/* select +k */
		    if (++nfields>=NF) {
			diag(catgets(catd,NL_SETN,4, "too many keys"), "");
			exit(DEFERR);
		    }
		    copyproto();
		    field(argv[optind]+1, 0);	/* +pos */
		    c='0';		/* ensure not EOF */
		    ++optind;
		}

	} while (c != EOF && optind < argc);	/* do getopt */

	while (optind < argc)		/* everything else is a file name */
	    eargv[eargc++] = argv[optind++];

	/* POSIX 12/12/91 :
	 * Add logic to ignore keys when pos1 is greater than pos2 
	*/
	for(a=1; a<=nfields; a++) {
		p = &fields[a];
	    if(p->m[1] >= 0)
		if(p->m[0] > p->m[1] || (p->m[0] == p->m[1] &&
					 p->n[0] > p->n[1])) {
			for(i = a; i < nfields; i++)
				fields[a] = fields[a+1];
			nfields--;
		}
	}


	q = &fields[0];
	for(a=1; a<=nfields; a++) {	/* ensure all fields specs are set */
		p = &fields[a];
		if(p->code    != proto.code)    continue;
		if(p->ignore  != proto.ignore)  continue;
		if(p->fcmp    != proto.fcmp)    continue;
		if(p->rflg    != proto.rflg)    continue;
		if(p->bflg[0] != proto.bflg[0]) continue;
		if(p->bflg[1] != proto.bflg[1]) continue;
		p->code =    q->code;
		p->ignore =  q->ignore;
		p->fcmp =    q->fcmp;
		p->rflg =    q->rflg;
		p->bflg[0] = p->bflg[1] = q->bflg[0];
	}

	if(eargc == 0)			/* use std input? */
		eargv[eargc++] = "-";

	if(cflg && eargc>1) {
		diag((catgets(catd,NL_SETN,5, "can check only 1 file")), "");
		exit(1);
	}

	/* POSIX 12/12/91
	*/
	if(cflg) {				/* if -c is combined with */
		if(outfil != 0 || mflg) {	/* -o or -m, it is bad usage */
			diag(USE, "");
			exit(DEFERR);
		}
	}

	safeoutfil();

	lspace = realloc(NULL, (size_t) 0);
	maxbrk = (UNSIGNED int *) ulimit(UL_GETMAXBRK,0L);

	if (!mflg && !cflg)
		if ((alloc=grow_core(tryfor,(unsigned) 0)) == 0) {
			diag((catgets(catd,NL_SETN,6, "allocation error before sort")), "");
			exit(DEFERR);
		}

	a = -1;
	for(dirs=dirtry; *dirs; dirs++) {
		(void) sprintf(filep=file1, "%s/stm%.5uaa", *dirs, getpid());
		while (*filep)
			filep++;
		filep -= 2;
		if ( (a=creat(file, 0600)) >=0)
			break;
	}
	if(a < 0) {
		diag((catgets(catd,NL_SETN,7, "can't locate temp")), "");
		exit(DEFERR);
	}
	(void) close(a);
	(void) unlink(file);
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, term);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, term);
	(void) signal(SIGPIPE, term);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void) signal(SIGTERM, term);
	nfiles = eargc;
	if(!mflg && !cflg) {
		sort();
		if (ferror(stdin))
			rderror("stdin");
		(void) fclose(stdin);
	}

	/* POSIX 12/12/91
	 * change default maxrec to LINE_MAX , change all BUFSIZ to LINE_MAX 
	*/

	if (maxrec == 0)  maxrec = LINE_MAX;
	alloc = (N + 1) * maxrec + N * LINE_MAX;
	for (nway = N; nway >= 2; --nway) {
		if (alloc < (maxbrk - lspace) * sizeof(int *))
			break;
		alloc -= maxrec + LINE_MAX;
	}
	if (nway < 2 || (lspace=realloc(lspace, (size_t)alloc)) == NULL) {
		diag((catgets(catd,NL_SETN,8, "allocation error before merge")), "");
		term();
	}

	if (cflg)   checksort();

	wasfirst = notfirst = 0;
	a = mflg || cflg ? 0 : eargc;
	if ((i = nfiles - a) > nway) {	/* Do leftovers early */
		if ((i %= (nway - 1)) == 0)
			i = nway - 1;
		if (i != 1)  {
			newfile();
			setbuf(os, bufout);
			merge(a, a+i);
			a += i;
		}
	}
	for(; a+nway<nfiles || unsafeout&&a<eargc; a=i) {
		i = a+nway;
		if(i>=nfiles)
			i = nfiles;
		newfile();
		setbuf(os, bufout);
		merge(a, i);
	}
	if(a != nfiles) {
		oldfile();
		setbuf(os, bufout);
		merge(a, nfiles);
	}
	error = 0;
	term();
}

sort()
{
	register char *cp;
	register char **lp;
	FILE *iop;
	char *keep, *ekeep, **mp, **lmp, **ep;
	int n;
	int done, i, first;
	char *f;

	/*
	** Records are read in from the front of the buffer area.
	** Pointers to the records are allocated from the back of the buffer.
	** If a partially read record exhausts the buffer, it is saved and
	** then copied to the start of the buffer for processing with the
	** next coreload.
	*/
	first = 1;
	done = 0;
	keep = 0;
	i = 0;
	ep = (char **) (((char *) lspace) + alloc);
	if ((f=setfil(i++)) == NULL) /* open first file */
		iop = stdin;
	else if ((iop=fopen(f,"r")) == NULL)
		cant(f);
	setbuf(iop,bufin);
	do {
		lp = ep - 1;
		cp = (char *) lspace;
		*lp-- = cp;
		if (keep != 0) /* move record from previous coreload */
			for(; keep < ekeep; *cp++ = *keep++);
#ifndef pdp11
		while ((char *)lp - cp > 1) {
#ifndef NONULL
			n = fgetn(cp,(char *) lp - cp, iop);
#else NONULL
			if (fgets(cp,(char *) lp - cp, iop) == NULL)
				n = 0;
			else
				n = strlen(cp);
#endif NONULL
#else
		while ((char *)lp - cp > 1 || (char *)lp -cp < 0) {
			n = (char *)lp -cp;
			if(n < 0)
				n = MAXINT;
#ifndef NONULL
			n = fgets(cp,n,iop);
#else NONULL
			if (fgets(cp,n,iop) == NULL)
				n = 0;
			else
				n = strlen(cp);
#endif NONULL
#endif
			if (n == 0) {
				if (ferror(iop))
					rderror(iop);

				if (keep != 0 )
					/* The kept record was at
					   the EOF.  Let the code
					   below handle it.       */;
				else
				if (i < eargc) {
					if ((f=setfil(i++)) == NULL)
						iop = stdin;
					else if ((iop=fopen(f,"r")) == NULL )
						cant(f);
					setbuf(iop,bufin);
					continue;
				}
				else {
					done++;
					break;
				}
			}
			cp += n - 1;
			if ( *cp == '\n') {
				cp += 2;
				*lp-- = cp;
				keep = 0;
			}
			else
			if ( cp + 2 < (char *) lp ) {
				/* the last record of the input */
				/* file is missing a NEWLINE    */
				if(f == NULL) diag((catgets(catd,NL_SETN,9, "warning: missing NEWLINE added at EOF")), "");
				else diag((catgets(catd,NL_SETN,10, "warning: missing NEWLINE added at end of input file ")), f);
				*++cp = '\n';
				*++cp = '\0';
				*lp-- = ++cp;
				keep = 0;
			}
			else {  /* the buffer is full */
				keep = *(lp+1);
				ekeep = ++cp;
			}

#ifdef pdp11
			if ((char *)lp - cp <= 2 && (char *)lp -cp >= 0 && 
first == 1) {
#else
			if ((char *)lp - cp <= 2 && first == 1) {
#endif
				/* full buffer */
				tryfor = alloc;
				tryfor = grow_core(tryfor,alloc);
				if (tryfor == 0)
					/* could not grow */
					first = 0;
				else { /* move pointers */
					lmp = ep + 
					   (tryfor/sizeof(char **) - 1);
					for ( mp = ep - 1; mp > lp;)
						*lmp-- = *mp--;
					ep += tryfor/sizeof(char **);
					lp += tryfor/sizeof(char **);
					alloc += tryfor;
				}
			}
		}
		if (keep != 0 && *(lp+1) == (char *) lspace) {
			diag((catgets(catd,NL_SETN,11, "fatal: record too large")),"");
			term();
		}
		first = 0;
		lp += 2;
		if(done == 0 || nfiles != eargc)
			newfile();
		else
			oldfile();
		setbuf(os, bufout);
		msort(lp, ep);
		if (ferror(os))
			wterror((catgets(catd,NL_SETN,12, "sorting")));
		(void) fclose(os);
	} while(done == 0);
}


msort(a, b)
	char	**a, **b;
{
	register struct btree **tp;
	register int i, j, n;
	char *save;

	i = (b - a);
	if (i < 1)
		return;
	else if (i == 1) {
		wline(*a);
		return;
	}
	else if (i >= TREEZ)
		n = TREEZ; /* number of blocks of records */
	else n = i;

	/* break into n sorted subgroups of approximately equal size */
	tp = &(treep[0]);
	j = 0;
	do {
		(*tp++)->rn = j;
		b = a + (blkcnt[j] = i / n);
		qksort(a, b);
		blkcur[j] = a = b;
		i -= blkcnt[j++];
	} while (--n > 0);
	n = j;

	/* make a sorted binary tree using the first record in each group */
	for (i = 0; i < n;) {
		(*--tp)->rp = *(--blkcur[--j]);
		insert(tp, ++i);
	}
	wasfirst = notfirst = 0;
	bonus = cmpsave(n);


	j = uflg;
	tp = &(treep[0]);
	while (n > 0)  {
		wline((*tp)->rp);
		if (j) save = (*tp)->rp;

		/* Get another record and insert.  Bypass repeats if uflg */

		do {
			i = (*tp)->rn;
			if (j) while((blkcnt[i] > 1) &&
					(**(blkcur[i]-1) == '\0')) {
				--blkcnt[i];
				--blkcur[i];
			}
			if (--blkcnt[i] > 0) {
				(*tp)->rp = *(--blkcur[i]);
				insert(tp, n);
			} else {
				if (--n <= 0) break;
				bonus = cmpsave(n);
				tp++;
			}
		} while (j && (*compare)((*tp)->rp, save) == 0);
	}
}


/* Insert the element at tp[0] into its proper place in the array of size n */
/* Pretty much Algorith B from 6.2.1 of Knuth, Sorting and Searching */
/* Special case for data that appears to be in correct order */

insert(tp, n)
	struct btree **tp;
	int n;
{
    register struct btree **lop, **hip, **midp;
    register int c;
    struct btree *hold;

    midp = lop = tp;
    hip = lop++ + (n - 1);
    if ((wasfirst > notfirst) && (n > 2) &&
	((*compare)((*tp)->rp, (*lop)->rp) >= 0)) {
	wasfirst += bonus;
	return;
    }
    while ((c = hip - lop) >= 0) { /* leave midp at the one tp is in front of */
	midp = lop + c / 2;
	if ((c = (*compare)((*tp)->rp, (*midp)->rp)) == 0) break; /* match */
	if (c < 0) lop = ++midp;   /* c < 0 => tp > midp */
	else       hip = midp - 1; /* c > 0 => tp < midp */
    }
    c = midp - tp;
    if (--c > 0) { /* number of moves to get tp just before midp */
	hip = tp;
	lop = hip++;
	hold = *lop;
	do *lop++ = *hip++; while (--c > 0);
	*lop = hold;
	notfirst++;
    } else wasfirst += bonus;
}


merge(a, b)
int a,b;
{
	FILE *tfile[N];
	char *buffer = (char *) lspace;
	register int nf;		/* number of merge files */
	register struct btree **tp;
	register int i, j;
	char	*f;
	char	*save, *iobuf;

	save = (char *) lspace + (nway * maxrec);
	iobuf = save + maxrec;
	tp = &(treep[0]);
	for (nf=0, i=a; i < b; i++)  {
		f = setfil(i);
		if (f == 0)
			tfile[nf] = stdin;
		else if ((tfile[nf] = fopen(f, "r")) == NULL)
			cant(f);
		(*tp)->rp = buffer + (nf * maxrec);
		(*tp)->rn = nf;
		setbuf(tfile[nf], iobuf);
		iobuf += LINE_MAX;
		if (rline(tfile[nf], (*tp)->rp)==0) {
			nf++;
			tp++;
		} else {
			if(ferror(tfile[nf]))
				rderror(f);
			(void) fclose(tfile[nf]);
		}
	}


	/* make a sorted btree from the first record of each file */
	for (--tp, i = 1; i++ < nf;) insert(--tp, i);

	bonus = cmpsave(nf);
	tp = &(treep[0]);
	j = uflg;
	while (nf > 0) {
		wline((*tp)->rp);
		if (j) cline(save, (*tp)->rp);

		/* Get another record and insert.  Bypass repeats if uflg */

		do {
			i = (*tp)->rn;
			if (rline(tfile[i], (*tp)->rp)) {
				if (ferror(tfile[i]))
					rderror(setfil(i+a));
				(void) fclose(tfile[i]);
				if (--nf <= 0) break;
				++tp;
				bonus = cmpsave(nf);
			} else insert(tp, nf);
		} while (j && (*compare)((*tp)->rp, save) == 0 );
	}


	for (i=a; i < b; i++) {
		if (i >= eargc)
			(void) unlink(setfil(i));
	}
	if (ferror(os))
		wterror((catgets(catd,NL_SETN,13, "merging")));
	(void) fclose(os);
}

cline(tp, fp)
	register char *tp, *fp;
{
	while ((*tp++ = *fp++) != '\n');
}

rline(iop, s)
	register FILE *iop;
	register char *s;
{
	register int n;

#ifndef NONULL
	n = fgetn(s,maxrec,iop);
#else NONULL
	if (fgets(s,maxrec,iop) == NULL )
		n = 0;
	else
		n = strlen(s);
#endif NONULL
	if ( n == 0 )
		return(1);
	s += n - 1;
	if ( *s == '\n' )
		return(0);
	else
	if ( n < maxrec - 1) {
		diag((catgets(catd,NL_SETN,14, "warning: missing NEWLINE added at EOF")),"");
		*++s = '\n';
		return(0);
	}
	else {
		diag((catgets(catd,NL_SETN,15, "fatal: line too long")),"");
		term();
	}
}

wline(s)
	char *s;
{
#ifndef NONULL
	(void) fputn(s,os);
#else NONULL
	(void) fputs(s,os);
#endif NONULL
}

checksort()
{
	char *f;
	char *lines[2];
	register int i, j, r;
	register char **s;
	register FILE *iop;

	s = &(lines[0]);
	f = setfil(0);
	if (f == 0)
		iop = stdin;
	else if ((iop = fopen(f, "r")) == NULL)
		cant(f);
	setbuf(iop, bufin);

	i = 0;   j = 1;
	s[0] = (char *) lspace;
	s[1] = s[0] + maxrec;
	if ( rline(iop, s[0]) ) {
		if (ferror(iop)) {
			rderror(f);
		}
		(void) fclose(iop);
		exit(0);
	}
	/* POSIX 12/12/91
	*/
	error = 1;		/* exit value for failures under -c option */

	while ( !rline(iop, s[j]) )  {
		r = (*compare)(s[i], s[j]);
						/* POSIX 12/12/91
						*/
		if (r < 0) term();		/* no outputs allowed */
		if (r == 0 && uflg) term();	/* no outputs allowed */
		r = i;  i = j; j = r;
	}
	/* POSIX 12/12/91
	*/
	error = DEFERR;			/* back to first definition */
	if (ferror(iop))
		rderror(f);
	(void) fclose(iop);
	exit(0);
}

/* POSIX 12/12/91 : no longer required
disorder(s, t)
	char	*s, *t;
{
	register char *u;
	for(u=t; *u!='\n';u++) ;
	*u = 0;
	diag(s, t);
	term();
}
*/

newfile()
{
	register char *f;

	f = setfil(nfiles);
	if((os=fopen(f, "w")) == NULL) {
		diag((catgets(catd,NL_SETN,18, "can't create ")), f);
		term();
	}
	chmod(f,0600);
	nfiles++;
}

char *
setfil(i)
	register int i;
{
	if(i < eargc)
		if(eargv[i][0] == '-' && eargv[i][1] == '\0')
			return(0);
		else
			return(eargv[i]);
	i -= eargc;
	filep[0] = i/26 + 'a';
	filep[1] = i%26 + 'a';
	return(file);
}

oldfile()
{
	if(outfil) {
		if((os=fopen(outfil, "w")) == NULL) {
			diag((catgets(catd,NL_SETN,19, "can't create ")), outfil);
			term();
		}
	} else
		os = stdout;
}

safeoutfil()
{
	register int i;
	struct stat ostat, istat;

	if(!mflg||outfil==0)
		return;
	if(stat(outfil, &ostat)==-1)
		return;
	if ((i = eargc - N) < 0) i = 0;	/*-N is suff., not nec. */
	for (; i < eargc; i++) {
		if(stat(eargv[i], &istat)==-1)
			continue;
		if(ostat.st_dev==istat.st_dev&&
		   ostat.st_ino==istat.st_ino)
			unsafeout++;
	}
}

cant(f)
	char *f;
{
	diag((catgets(catd,NL_SETN,20, "can't open ")), f);
	term();
}

diag(s, t)
	char *s, *t;
{
	register FILE *iop;

	iop = stderr;
	(void) fputs("sort: ", iop);
	(void) fputs(s, iop);
	(void) fputs(t, iop);
	(void) fputs("\n", iop);
}

term()
{
	register i;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
	if(nfiles == eargc)
		nfiles++;
	for(i=eargc; i<=nfiles; i++) {	/*<= in case of interrupt*/
		(void) unlink(setfil(i));	/*with nfiles not updated*/
	}
	exit(error);
}

cmp(i, j)
	unsigned char	*i, *j;
{
	register unsigned char *pa, *pb;
	register unsigned char *ignore;
	register int sa;
	int sb;
	unsigned char *skip();
	unsigned char *code;
	int a, b;
	int k;
	unsigned char *la, *lb;
	unsigned char *ipa, *ipb, *jpa, *jpb;
	struct field *fp;

#ifdef NLS16
	int palen = 0;
	int pblen = 0;
	char *npa, *npb;
#endif /* NLS16 */

	for(k = nfields>0; k<=nfields; k++) {
		fp = &fields[k];
		pa = i;
		pb = j;
		if(k || fp->fcmp==NUM || fp->fcmp==MON) {
			la = skip(pa, fp, 1);
			pa = skip(pa, fp, 0);
			lb = skip(pb, fp, 1);
			pb = skip(pb, fp, 0);
		} else {
			la = (unsigned char *)eol(pa);
			lb = (unsigned char *)eol(pb);
		}
		if(fp->fcmp==NUM) {
			/*
			 * Even though skip() has skipped white space before
			 * the field started, we need to skip white space
			 * within the field, i.e. sort -t: +1.4 -2:
			 *     "xx:  y   123"
			 *           123456789
			 *
			 * skip() returns a string "   123", but we want to
			 * ignore the three leading blanks CRAP-CRAP
			 */
			while(isblank(*pa))
				ADVANCE(pa);
			while(isblank(*pb))
				ADVANCE(pb);

			sa = sb = fp->rflg;
			if(*pa == '-') {
				pa++;
				sa = -sa;
			}
			if(*pb == '-') {
				pb++;
				sb = -sb;
			}
			/* POSIX 12/12/91
			 * Adding the Thousands Separator logic -
			   find end of field before Radix character.
			*/
			for(ipa = pa; ipa<la && (isdigit((int)(unsigned char)
			   *ipa) || *ipa == ThousSep); ipa++) continue;
			for(ipb = pb; ipb<lb && (isdigit((int)(unsigned char)
			   *ipb) || *ipb == ThousSep); ipb++) continue;
			jpa = ipa;
			jpb = ipb;
			a = 0;
			ipa--;		/* points to last NUM character */
			ipb--;
			if(sa==sb)
				while(ipa >= pa && ipb >= pb) {
					if(*ipa == ThousSep) {	/* skip them */
						ipa--;
						continue;
					}
					if(*ipb == ThousSep) {
						ipb--;
						continue;
					}
					/* if unequal, set "a" equal to the
					difference */
					if(b = *ipb-- - *ipa--)
						a = b;
				}
			while(ipa >= pa) {
				if(*ipa == ThousSep) {	/* skip them */
					ipa--;
					continue;
				}
				if(*ipa-- != '0')
					return(-sa);
			}
			while(ipb >= pb) {
				if(*ipb == ThousSep) {	/* skip them */
					ipb--;
					continue;
				}
				if(*ipb-- != '0')
					return(sb);
			}
			if(a) return(a*sa);
			if(*(pa=jpa) == RadixChar)
				pa++;
			if(*(pb=jpb) == RadixChar)
				pb++;
			if(sa==sb)
				while(pa<la && isdigit((int)(unsigned char)*pa)
				   && pb<lb && isdigit((int)(unsigned char)*pb))
					if(a = *pb++ - *pa++)
						return(a*sa);
			while(pa<la && isdigit((int)(unsigned char)*pa))
				if(*pa++ != '0')
					return(-sa);
			while(pb<lb && isdigit((int)(unsigned char)*pb))
				if(*pb++ != '0')
					return(sb);
			continue;
		} else if(fp->fcmp==MON)  {
			sa = fp->rflg*(month(pb)-month(pa));
			if(sa)
				return(sa);
			else
				continue;
		}

#if defined(NLS) || defined(NLS16)
		if ( ! isOneToOne )  {	/* can't use tables */
			if ( fp->ignore == dict || fp->ignore == nonprint ||
				fp->code == fold ) { /* make copy */
				sa = - strcoll_1byte_cpy(pa,pb,la,lb,fp);
			} else {
				char la_c, lb_c;

				la_c = *la ; *la = '\0';
				lb_c = *lb ; *lb = '\0';
				sa = - strcoll(pa, pb);
				*la = la_c;
				*lb = lb_c;
				}
			if ( sa )
				return(sa*fp->rflg);
			else
				continue;  
		}
#endif /* defined(NLS) || defined(NLS16) */
#ifdef NLS16
		if ( ! isOneByte ) {	/* multi-byte code set */
			if ( k ) {
				npa = (char *)pa;
				npb = (char *)pb;
				palen = pblen = 0;
				while (npa < (char *)la){
					++palen;
					ADVANCE(npa);
				}
				while (npb < (char *)lb){
					++pblen;
					ADVANCE(npb);
				}
                        	sa = - nl_strncmp(pa, pb, min(palen, pblen));
			} else
                        	sa = - strcoll(pa, pb);

                	if ( sa )
				return(sa*fp->rflg);
                	else
				continue;
                }
#endif /* NLS16 */
		code = fp->code;
		ignore = (unsigned char *)fp->ignore;
loop: 
		while(ignore[*pa])
			pa++;
		while(ignore[*pb])
			pb++;
		if(pa>=la || *pa=='\n')
			if(pb<lb && *pb!='\n')
				return(fp->rflg);
			else
				continue;
		if(pb>=lb || *pb=='\n')
			return(-fp->rflg);
		if((sa = (code[*pb++] - code[*pa++])) == 0)
			goto loop;
		else
			return(sa*fp->rflg);
	}
	if(uflg)
		return(0);
	return(cmpa(i, j));
}

cmpa(pa, pb)
	register unsigned char *pa, *pb;

{

	while(*pa == *pb++)
		if(*pa++ == '\n')
			return(0);
	return(
		*pa == '\n' ? fields[0].rflg:
		*--pb == '\n' ?-fields[0].rflg:
		(*pb & 0xFF) > (*pa & 0xFF)  ? fields[0].rflg:
		-fields[0].rflg
	);
}

strcoll_1byte_cpy(pa, pb, la, lb, fp)
	register char *pa, *pb;
	register char *la, *lb;
	struct field *fp;

{
	/* Is this real?  Alas, Viginia, it is. */
		/* This routine allows use of strcoll with folded
		** keys.
		*/
	int sa;
	unsigned char tabuf[256], *ta = &tabuf[0];
	unsigned char tbbuf[256], *tb = &tbbuf[0];
	unsigned char c, *f, *t, *l;

	f=(unsigned char *)pa; t=ta; l=(la-pa<256) ? ta+(la-pa) : ta+255;
	while ( (c = *f++) != '\n' && t < l ) {
		if ( fp->ignore[c] ) continue;
		c = TOFOLD(c);
		*t++ = c;
		}
	*t = '\0';
	f=(unsigned char *)pb; t=tb; l=(lb-pb<256) ? tb+(lb-pb) : tb+255;
	while ( (c = *f++) != '\n' && t < l ) {
		if ( fp->ignore[c] ) continue;
		c = TOFOLD(c);
		*t++ = c;
		}
	*t = '\0';
	sa = strcoll(ta, tb);
	return(sa);
}

/*
 * skip() -- Skip until the beginning of a field, returning a
 *           pointer to the first or last char of the field, depending
 *           on the value of 'j' (0 == first, 1 == last).
 */
unsigned char *
skip(p, fp, j)
	struct field *fp;
	register unsigned char *p;
	int j;
{
	register int i;
	register unsigned char tbc;

	if( (i=fp->m[j]) < 0)
		return((unsigned char *)eol(p));

	if ((tbc = tabchar) != '\0')
		while (--i >= 0) {
			while(*p != tbc)
				if(*p != '\n')
					ADVANCE(p);	/* p++; */
				else goto ret;
			if (i > 0 || j == 0)
				ADVANCE(p);	/* p++; */
		}
	else
		while (--i >= 0) {
			while(isblank(*p))
				ADVANCE(p);	/* p++; */
			while(!isblank(*p))
				if(*p != '\n')
					ADVANCE(p);	/* p++; */
				else goto ret;
		}

	if (fp->bflg[j]) {
		if (j == 1 && fp->m[j] > 0)
			ADVANCE(p);	/* p++; */
		while(isblank(*p))
			ADVANCE(p);	/* p++; */
	}

	i = fp->n[j];
	while((i-- > 0) && (*p != '\n'))
		ADVANCE(p);	/* p++; */

ret:
	return(p);
}

char *
eol(p)					/* move forward until end-of-line */
	register char *p;
{
	while(*p != '\n') p++;
	return(p);
}

copyproto()
{
	memcpy(&fields[nfields], &proto, sizeof(proto));
}

initree()
{
	register struct btree **tpp, *tp;
	register int i;

	for (tp = &(tree[0]), tpp = &(treep[0]), i = TREEZ; --i >= 0;)
	    *tpp++ = tp++;
}

cmpsave(n)
	register int n;
{
	register int award;

	if (n < 2) return (0);
	for (n++, award = 0; (n >>= 1) > 0; award++);
	return (award);
}

#ifdef DEBUG
void
print_fields()
{
    int i;

    fputc('\n', stdout);
    for (i = 0; i <= nfields; i++)
    {
	printf("fields[%d].code\t\t= \"%s\"\n", i, fields[i].code);
	printf("fields[%d].ignore\t= \"%s\"\n", i, fields[i].ignore);
	printf("fields[%d].fcmp\t\t= %d\n",     i, fields[i].fcmp);
	printf("fields[%d].rflg\t\t= %d\n",     i, fields[i].rflg);
	printf("fields[%d].bflg[0]\t= %d\n",    i, fields[i].bflg[0]);
	printf("fields[%d].m[0]\t\t= %d\n",     i, fields[i].m[0]);
	printf("fields[%d].n[0]\t\t= %d\n",     i, fields[i].n[0]);
	printf("fields[%d].bflg[1]\t= %d\n",    i, fields[i].bflg[1]);
	printf("fields[%d].m[1]\t\t= %d\n",     i, fields[i].m[1]);
	printf("fields[%d].n[1]\t\t= %d\n\n",   i, fields[i].n[1]);
    }
    fputc('\n', stdout);
    fflush(stdout);
}

#endif /* DEBUG */

field(s, k)
	char	*s;
	int	k;
{
	register struct field *p;
	register int *d;
	int zflg;

	zflg = 1;		/* used to subtract 1 from all numbers (if -k
				   option is used), except the 'z' number */
	p = &fields[nfields];
	for(; *s!=0; s++) {
		switch(*s) {
		case '\0':
			return;

		case 'b':
			p->bflg[k] = 1;
			break;

		case 'c':
			cflg = 1;
			continue;

		case 'd':
			if ( isOneByte )
				p->ignore = dict;
			else
				diag((catgets(catd,NL_SETN,21, "warning: -d option ignored for multibyte languages")),"");
			break;

		case 'f':
			if ( isOneByte )
				p->code = fold;
			else
				diag((catgets(catd,NL_SETN,22, "warning: -f option ignored for multibyte languages")),"");
			break;

		case 'l':
			diag((catgets(catd,NL_SETN,23, "warning: -l not required, ignored")),"");
			break;

		case 'i':
			if ( isOneByte )
				p->ignore = nonprint;
			else
				diag((catgets(catd,NL_SETN,24, "warning: -i option ignored for multibyte languages")),"");
			break;

		case 'm':
			mflg = 1;
			continue;

		case 'M':
			p->fcmp = MON;
			p->bflg[0] = 1;
			break;

		case 'n':
			p->fcmp = NUM;
			/* dont set bflg automagically. This is a user option.*/
			/* p->bflg[0] = p->bflg[1] = 1; */
			break;

		case 't':
			tabchar = *++s;
			if(tabchar == 0) s--;
			continue;

		case 'r':
			p->rflg = -1;
			continue;

		case 'u':
			uflg = 1;
			continue;

		case 'y':
			if ( *++s ) {
				if (isdigit((int)(unsigned char)*s)) {
#ifdef pdp11
					longtryfor = ((long) number(&s)) * 1024L;
					if(longtryfor > MAXUNSIGNED) tryfor = (unsigned) MAXUNSIGNED;
					else tryfor = (unsigned) longtryfor;
#else
					tryfor = number(&s) * 1024;
#endif
				}
				else {
					diag(USE,"");
					exit(DEFERR);
				}
			}
			else {
				--s;
				tryfor = MAXMEM;
			}
			continue;

		case 'z':
			if ( *++s && isdigit((int)(unsigned char)*s))
				/*
				 * allow space for \n and NULL
				 */
				maxrec = number(&s) + 2;
			else {
				diag(USE,"");
				exit(DEFERR);
			}
			continue;

		/* POSIX 12/12/91		 POSIX defines...
		 * +w.x -y.z  => -kw+1.x+1,y.0    if z = 0,
		 * 	      => -kw+1.x+1,y+1.z  if z > 0.
		 * Here we convert everything into obsolescent format....
		 * Processing is done in the obsolescent format.
		 * Decrement w,x,and y when they occur. z is handled in
		 * the getopt() loop.
		*/
		case '.':
		default:
			if (*s == '.') {
				if(*++s == '\0') {
					s--;
					continue;
				}
				d = &(p->n[k]);
				if(k == 1) {
				/* handling for z when k=1 is done in the
				 * getopt loop. This is because,
				 * if z is not defined, it will not get here!
				 */
					zflg = 0;
					if(p->m[1] < 0) p->m[1] = 0;
				}
			} else d = &(p->m[k]);

			if (isdigit((int)(unsigned char)*s)) {
				*d = number(&s);
				if(kflg && zflg)
					if((*d)-- == 0) {
						diag(USE,"");
						exit(DEFERR);
					}
			} else {
				diag(USE,"");
				exit(DEFERR);
			}
		}
	}
#ifdef DEBUG
	print_fields();
#endif /* DEBUG */
}


number(ppa)
register char **ppa;
{
	register int n;
	register char *pa;

	pa = *ppa;
	n = 0;
	while(isdigit((int)(unsigned char)*pa)) {
		n = n*10 + *pa - '0';
		*ppa = pa++;
	}
	return(n);
}

#define qsexc(p,q) t= *p;*p= *q;*q=t
#define qstexc(p,q,r) t= *p;*p= *r;*r= *q;*q=t

qksort(a, l)
	char	**a, **l;
{
	register char **i, **j;
	register char **lp, **hp;
	char **k;
	int c, delta;
	char *t;
	unsigned n;


start:
	if((n=l-a) <= 1)
		return;

	n /= 2;
	if (n >= MTHRESH) {
		lp = a + n;
		i = lp - 1;
		j = lp + 1;
		delta = 0;
		c = (*compare)(*lp, *i);
		if (c < 0) --delta;
		else if (c > 0) ++delta;
		c = (*compare)(*lp, *j);
		if (c < 0) --delta;
		else if (c > 0) ++delta;
		if ((delta /= 2) && (c = (*compare)(*i, *j)))
		    if (c > 0) n -= delta;
		    else       n += delta;
	}
	hp = lp = a+n;
	i = a;
	j = l-1;


	for(;;) {
		if(i < lp) {
			if((c = (*compare)(*i, *lp)) == 0) {
				--lp;
				qsexc(i, lp);
				continue;
			}
			if(c < 0) {
				++i;
				continue;
			}
		}

loop:
		if(j > hp) {
			if((c = (*compare)(*hp, *j)) == 0) {
				++hp;
				qsexc(hp, j);
				goto loop;
			}
			if(c > 0) {
				if(i == lp) {
					++hp;
					qstexc(i, hp, j);
					i = ++lp;
					goto loop;
				}
				qsexc(i, j);
				--j;
				++i;
				continue;
			}
			--j;
			goto loop;
		}


		if(i == lp) {
			if(uflg)
				for(k=lp; k<hp;) **k++ = '\0';
			if(lp-a >= l-hp) {
				qksort(hp+1, l);
				l = lp;
			} else {
				qksort(a, lp);
				a = hp+1;
			}
			goto start;
		}


		--lp;
		qstexc(j, lp, i);
		j = --hp;
	}
}

char months[12][ABMON_MAX+1] = { /* allow space for nls abbreviations */
	"jan",
	"feb",
	"mar",
	"apr",
	"may",
	"jun",
	"jul",
	"aug",
	"sep",
	"oct",
	"nov",
	"dec"
};

month(s)
char *s;
{
	register char *t, *u;
	register i;
	register char *f = (char *)fold;

	if ( isOneByte )
		for(i=0; i<sizeof(months)/sizeof(*months); i++) {
			for(t=s, u=months[i]; f[*t++]==f[*u++]; )
				if(*u==0)
					return(i);
		}
	else
		for(i=0; i<sizeof(months)/sizeof(*months); i++) {
			for(t=s, u=months[i]; *t++==*u++; )
				if(*u==0)
					return(i);
		}
	return(-1);
}

rderror(s)
char *s;
{
	diag((catgets(catd,NL_SETN,25, "read error on ")), s == 0 ? "stdin" : s);
	term();
}

wterror(s)
char *s;
{
	diag((catgets(catd,NL_SETN,26, "write error while ")), s);
	term();
}

grow_core(size,cursize)
	unsigned size, cursize;
{
	unsigned newsize;
	/*The variable below and its associated code was written so this would work on */
	/*pdp11s.  It works on the vax & 3b20 also. */
	UNSIGNED long longnewsize;

	longnewsize = (UNSIGNED long) size + (UNSIGNED long) cursize;
	if (longnewsize < MINMEM)
		longnewsize = MINMEM;
	else
	if (longnewsize > MAXMEM)
		longnewsize = MAXMEM;
	newsize = (unsigned) longnewsize;

	/* 12/12/91 : This line appears redundant ..
	for (; ((char *)lspace+newsize) <= (char *)lspace; newsize >>= 1);
	*/
	if (longnewsize > (UNSIGNED long) (maxbrk - lspace) * (long) sizeof(int *))
		newsize = (maxbrk - lspace) * sizeof(int *);
	if (newsize <= cursize)
		return(0);
	if ((lspace=realloc(lspace,(size_t)newsize)) == NULL)
		return(0);
	return(newsize - cursize);
}




/*
** *****************************************************************************
** 
**  This section of code was added to accommodate sorting of strings with null
**  characters as part of the record.  To do this, fgets, fputs, and strlen code
**  has been included.  In each case the definition of a string has been changed
**  from a stream of bytes up to but not including a null byte to a stream of
**  bytes up to and including a newline character.
**
** *****************************************************************************
*/

/* **** taken from stdiom.h ************************************************* */

#define _BUFSYNC(iop)	if (_bufend(iop) - iop->_ptr <   \
				( iop->_cnt < 0 ? 0 : iop->_cnt ) )  \
					_bufsync(iop)
#define _WRTCHK(iop)	((((iop->_flag & (_IOWRT | _IOEOF)) != _IOWRT) \
				|| (iop->_base == NULL)  \
				|| (iop->_ptr == iop->_base && iop->_cnt == 0 \
					&& !(iop->_flag & (_IONBF | _IOLBF)))) \
			? _wrtchk(iop) : 0 )

/* **** taken from fgets.c ************************************************** */
/*
 * This version reads directly from the buffer rather than looping on getc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */

extern int _filbuf();

int
fgetn(ptr, size, iop)
	char	*ptr;
	register int size;
	register FILE *iop;
{
	char *p, *ptr0 = ptr;
	register int n;

	for (size--; size > 0; size -= n) {
		if (iop->_cnt <= 0) { /* empty buffer */
			if (_filbuf(iop) == EOF) {
				if (ptr0 == ptr)
#ifndef NONULL
					return (0);
#else NONULL
					return (NULL);
#endif NONULL
				break; /* no more data */
			}
			iop->_ptr--;
			iop->_cnt++;
		}
		n = min(size, iop->_cnt);
#ifndef NONULL
		if ((p = memccpy(ptr, (char *) iop->_ptr, '\n', n)) != NULL)
#else NONULL
		if ((p = memccpy(ptr, (char *) iop->_ptr, '\0', n)) != NULL)
#endif NONULL
			n = p - ptr;
		ptr += n;
		iop->_cnt -= n;
		iop->_ptr += n;
		_BUFSYNC(iop);
		if (p != NULL)
			break; /* found '\n' in buffer */
	}
	*ptr = '\0';
#ifndef NONULL
	return (ptr - ptr0);
#else NONULL
	return (ptr0);
#endif NONULL
}

/* **** taken from fputs.c ************************************************** */
/*
 * This version writes directly to the buffer rather than looping on putc.
 * Ptr args aren't checked for NULL because the program would be a
 * catastrophic mess anyway.  Better to abort than just to return NULL.
 */

int
fputn(ptr, iop)
	char	*ptr;
	register FILE *iop;
{
	register int ndone = 0, n;
	register unsigned char *cptr, *bufend;
	char *p;

	if (_WRTCHK(iop))
		return (EOF);
	bufend = _bufend(iop);

	if ((iop->_flag & _IONBF) == 0)  {
		for ( ; ; ptr += n) {
			while ((n = bufend - (cptr = iop->_ptr)) <= 0)  
				/* full buf */
				if (_xflsbuf(iop) == EOF)
					return(EOF);
#ifndef NONULL
			if ((p = memccpy((char *) cptr, ptr, '\n', n)) != NULL)
				n = (p - (char *) cptr);
#else NONULL
			if ((p = memccpy((char *) cptr, ptr, '\0', n)) != NULL)
				n = (p - (char *) cptr) - 1;
#endif NONULL
			iop->_cnt -= n;
			iop->_ptr += n;
			_BUFSYNC(iop);
			ndone += n;
			if (p != NULL)  { 
				/* done; flush buffer if line-buffered */
	       			if (iop->_flag & _IOLBF)
	       				if (_xflsbuf(iop) == EOF)
	       					return(EOF);
	       			return(ndone);
	       		}
		}
	}  else  {
		/* write out to an unbuffered file */
#ifndef NONULL
		register unsigned int cnt = strlenn(ptr);
#else NONULL
		register unsigned int cnt = strlen(ptr);
#endif NONULL

		if (write(fileno(iop), ptr, cnt) != cnt)
		    return(EOF);
		else
		    return(cnt);
	}
}

/* **** taken from strlen.c ************************************************* */
/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */

int
strlenn(s)
	register char *s;
{
#ifndef NONULL
	register char *s0 = s;
#else NONULL
	register char *s0 = s + 1;
#endif NONULL

	if (s == NULL)
		return(0);

#ifndef NONULL
	while (*s++ != '\n')
#else NONULL
	while (*s++ != '\0')
#endif NONULL
		;
	return (s - s0);
}

#if defined(NLS) || defined(NLS16)
#include <locale.h>
#include <setlocale.h>

init_nls()
{
	int i, j, k, m;		/* N.B. j must be signed */
	nl_catd catopen();

	if (!setlocale(LC_ALL, "")) {
		diag(_errlocale(""),"");
		putenv("LANG=");
		catd = (nl_catd)-1;
		}
	else
		catd = catopen("sort", 0);
	isC_Locale = ! _nl_collate_on;	/* or n-computer */
	isOneByte = ! _nl_mb_collate;
	isOneToOne = TRUE;
	if ( isC_Locale ) {
#ifdef Tables
		print_tables();
		exit(DEFERR);
#endif
		return;
		}

	/* Assumption: ABMON_(i+1) == ABMON_i + 1 */
	for(i=0,m=ABMON_1;i<12;i++,m++)
		strncpy(months[i], __nl_info[m],ABMON_MAX);

	/* Assumption: RADIXCHAR is always 1 character */
	RadixChar = _nl_radix;

	/* POSIX 12/12/91
	 * The Thousands Separator is also 1 byte character
	*/
	ThousSep = *nl_langinfo(THOUSEP);

	/* generate tables for 8-bit languages with characters */
		/* These tables allow faster comparison of characters
		** but can be used only if the language does not have
		** 1-2 or 2-1 characters.  The tables are generated
		** according to the LANG and LC_COLLATE values at the
		** time sort is run.  The macro Tables provides 
		** for testing the tables.  When defined, the compiled
		** program will generate the tables, print them and
		** terminate.  Note that tables are not generated for
		** the C locale.
		*/
	if ( isOneByte ) {
		unsigned int c, cv[256], cc[256];
		int s, p, l, d;

		/* assign collation value for each character i so that */
		/*   if i preceeds j then cv[i] < cv[j] */
		for(i=0;i<256;i++) {
			s = _seqtab[i];
			p = _pritab[i];
			switch (p >> 6) {
				case 0: break;
				case 1: p = _tab21[p&077].priority; break;
				case 2: p = _tab12[p&077].priority; break;
				case 3: s = 0; p = 0; break; /* don't care */
				default: exit(99);	/* error */
				}
			cv[i] = (s << 8) | (p & 0377);
			cc[i] = i;
			}
		/* sort by collation value, cf. DK, V3 pg 95 (8) */
		for(k=40;k>0;k=(k-1)/3)
			for(i=k;i<256;i++)
				for(j=i-k;j>=0 && cv[j]>cv[j+k];j-=k) {
					c=cv[j]; cv[j]=cv[j+k]; cv[j+k]=c;
					c=cc[j]; cc[j]=cc[j+k]; cc[j+k]=c;
					}
		/* compress collation values to the range 0..255 */
		c = cv[0]; j = 0; cv[0] = j;
		for(i=1;i<256;i++)
			cv[i] = ( cv[i] > c ) ? c = cv[i], ++j : j;
		/* build nofold table */
		for(i=0;i<256;i++)
			nofold[ cc[i] ] = cv[i];
		/* build all other tables */
		for(i=0;i<256;i++) {
			fold[i] = nofold[TOFOLD(i)];
			d = _pritab[i]>>6 == 3;	/* don't care */
			zero[i] = d;
			nonprint[i] = !isprint(i) || d;
			dict[i] = !(isalnum(i) || isblank(i)) || d;
			isOneToOne = isOneToOne &&
				_pritab[i]>>6 != 1 && _pritab[i]>>6 != 2;
			}
		dict[(unsigned int)'\n'] = 0;	/* the way it was */
		}
	isOneToOne = FALSE;	/* do not use tables */
#ifdef Tables
	print_tables();
	exit(DEFERR);
#endif
}
#ifdef Tables
print_tables()
{
	printf("LANG: %s\n",getenv("LANG"));
	printf("LC_COLLATE : %s\n",setlocale(LC_COLLATE,(char *)0));
	printf("\nchar months [12][8] = {\n");
	print_mon();
	printf("\nchar fold [256] = {\n");
	print_code(fold);
	printf("\nchar nofold [256] = {\n");
	print_code(nofold);
	printf("\nchar zero [256] = {\n");
	print_flag(zero);
	printf("\nchar nonprint [256] = {\n");
	print_flag(nonprint);
	printf("\nchar dict [256] = {\n");
	print_flag(dict);
	printf("isOneToOne: %d\n",isOneToOne);
}

print_mon()
{
	int i;
	char *mn = months[0];

	for (i=0;i<12;i++) printf("\t\"%s\",\n",months[i]);
	printf("};\n");
	printf("/*****************\n");
	printf("sizeof(months) %d\n",sizeof(months));
	printf("sizeof(*months) %d\n",sizeof(*months));
	print_char(months[0],sizeof(months));
	printf("*****************/\n");
}

print_code(tbl)
unsigned char tbl[];
{
	unsigned int c;
	int i,j;

	for(i=0;i<256;i+=8) {
		printf("\t");
		for(j=i;j<i+8;j++) {
			c = tbl[j];
			printf("%.4o, ",c);
			}
		printf("\n");
		}
	printf("};\n");
}

print_flag(tbl)
	char	tbl[];
{
	int i,j;
	unsigned char c;

	for(i=0;i<256;i+=16) {
		printf("\t");
		for(j=i;j<i+16;j++) {
			c = tbl[j];
			printf("%.1o, ",c);
			}
		printf("\n");
		}
	printf("};\n");
}

print_char(c,l)
	unsigned char *c;
	int	l;
{
        unsigned int ch,i,j;

	for (i=0;i<l;i+=10) {
		for(j=i;j<((i+10<l)?i+10:l);j++) {
			ch = *c++;
			printf ("%x ",ch);
			}
		printf("\n");
		}
}

print_int(tbl)
	int	tbl[];
{
	unsigned int i,j,c;

	for(i=0;i<256;i+=8) {
		printf("\t");
		for(j=i;j<i+8;j++) {
			c = tbl[j];
			printf("%.6x, ",c);
			}
		printf("\n");
		}
}
#endif /* Tables */
#endif /* NLS || NLS16 */
