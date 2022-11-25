/*	SCCS	REV(47.7);	DATE(90/05/16	13:31:35) */
/* KLEENIX_ID @(#)c0.h	47.7 90/05/11 */


#define NO  0
#define YES 1

# ifdef DEBUGGING
#   define prntf printf
# endif

# define AUTOREG ARGREG

/* MAXLINES is the default "size" criteria for integration -- smaller procs
 * get integrated, larger ones don't
 */
#define MAXLINES 20
extern long	dlines;		/* default lines threshold */
extern long	defaultlines;		/* default lines threshold */

#define MAXLABELS 5000
extern long 	maxlabels;	/* maximum labels in "labarray" */
extern long 	*labarray;	/* temp array for collecting labels in proc */
extern long	nxtfreelab;	/* next free slot in "labarray" */
extern long nxtlabel;		/* next free label for use by integrated proc */

extern flag globaloptim;	/*expecting the global optimizer (c1) to run? */

#define C2PATTERN "#7 %ld(%%a6),%ld"

typedef struct volrec {
	long off, siz;
	struct volrec *next;
	} VOLREC;

/* Procedure descriptor record -- constructed in pass1 */
typedef struct pd {
	long	pnpoffset;	/* name of procedure/function */
	long	inoffset;	/* offset of "global" in input file */
	long	outoffset;	/* offset of LBRACKET in output file */
	long	outroffset;	/* offset of RBRACKET in output file */
	long	nlines;		/* # lines of executable source */
	struct callrec *callsto;	/* chain of called procedures */
	struct callrec *callsfrom;	/* chain of calling procedures */
	long	stackuse;	/* local stack usage */
	long	regsused;	/* register usage mask */
	long	fdregsused;	/* 68881 & dragon flt pt register usage mask */
	long	nlab;		/* # labels in "labels" array following */
	long	*labels;
	long	retoffset;	/* return value offset from %a6 */
	long	dimbytes;	/* bytes used for adjustable dim info */
	struct pd *next;	/* pointer to next in chain */
	struct pd *nextout;	/* next in integration order */
	VOLREC	*vollist;	/* c2 volatile flag list */
	long	integorder;	/* relative output order */
	long	nforced;	/* # forced offsets in array */
	long	nomitted;	/* # omitted offsets in array */
	long	*forceoffsets;	/* pointer to array of pnpoffsets of forced */
	long	*omitoffsets;	/* pointer to array of pnpoffsets of omit */
	long	begindlines;	/* dlines value at proc beginning */
	long	c1opts;		/* C1 options setting for this proc */
	char	typeproc;	/* SUBROUTINE/FUNCTION/ENTRY ? */
	char	inlineOK;	/* OK to inline? */
	char	noemitflag;	/* 1 = don't emit standalone code for this */
	char	hasaltrets;	/* 1 = has alternate returns */
	char	hasCharOrCmplxParms;	/* 1 = has CHAR or COMPLEX parameters */
	char	hasasgnfmts;	/* 1 = has ASSIGN'd FORMAT's */
	char	imbeddedAsm;	/* 1 = imbedded asm statments in routine */
	TWORD	rettype;	/* function return type */
	short	nparms;		/* number of parameters */
	TWORD	parmtype[1];	/* types of all parameters */
	} PD, *pPD;

extern pPD	firstpd;		/* first proc descriptor in chain */
extern pPD	lastpd;			/* last proc descriptor in chain */
extern pPD	currpd;			/* current proc descriptor */
extern pPD	firstoutpd;		/* first proc descriptor in out chain */
extern pPD	lastoutpd;		/* last proc descriptor in out chain */

typedef struct callrec {
	pPD 	calledfrom;		/* calling proc */
	pPD	callsto;		/* called proc */
	struct callrec *nextto;		/* next called proc for calling proc */
	struct callrec *nextfrom;	/* next calling proc for called proc */
	unsigned short count;		/* # times this calling/called pair */
	char	forced;			/* 1 = force integrate called proc */
	long	clines;			/* current default lines threshold */
	} CallRec, *pCallRec;

/* These two structures overlap -- the "onext", "nlongs" and "name" fields
 * must be at the same relative offsets
 */
typedef struct namerec {
	long	onext;			/* offset of next item in chain */
	char 	isthunk;		/* 0 -> regular proc call name */
	pPD	pd;			/* pointer to proc descriptor */
	pCallRec ffrom;			/* first "from" call record */
	unsigned forceflag : 1;		/* 1 = this proc currently forced */
	unsigned everforced : 1;	/* 1 = proc has been forced */
	unsigned filling : 6;
	char	omitflag;		/* 1 = this proc currently omitted */
	char 	noemitflag;		/* 1 = don't emit as stand-alone */
	char	initflag;		/* -1 = init'd FORCE; 1 = init'd OMIT */
	unsigned short nlongs;		/* # longs containing name */
	long name[1];			/* procedure name */
	} NameRec, *pNameRec;

typedef struct tnamerec {		/* for var expr format "thunks" */
	long	onext;			/* offset of next item in chain */
	char	isthunk;		/* 1 -> format thunk name */
	long	fileoffset;		/* offset of thunk in input file */
	long 	isempty;		/* 1 = thunk is empty */
	long	unused;
	unsigned short nlongs;		/* # longs containing name */
	long name[1];			/* procedure name */
	} TNameRec, *pTNameRec;

/* Call Record free blocks */
#define MAXNODES 200
typedef struct callrecblock {
	struct callrecblock *next;
	CallRec node[MAXNODES];
	} CallRecBlock, *pCallRecBlock;
extern long nextnode;
extern pCallRecBlock callreclist;

/* Hidden Vars Record */
typedef struct hvrec {
		struct hvrec *next;
		long cnt;
		short op;
		long rest;
		short val;
		long off;
		char *str;
		     } HVREC;

/* Nested C0 temporary opd list, for relocating array/struct refs */

typedef struct c0temprec {
	struct c0temprec *next;
	long baseoff;
	} C0TEMPREC;

/* HASHSIZE == size of hash table */
#define HASHSIZE 257
extern long hash[];

/* MAXPNPBYTES == max size of string pool */
#define MAXPNPBYTES 8192
extern long	maxpnpbytes;	/* max size of permanent name pool */
extern long	maxpnp;		/* end of perm name pool */
extern char	*pnp;			/* permanent name pool */
extern long	freepnp;		/* offset of next free byte in pnp */
extern long	maxtnpbytes;	/* max size of temporary name pool */
extern char	*maxtnp;		/* end of temp name pool */
extern char	*tnp;			/* temp name pool */
extern char	*freetnp;		/* next free byte in tnp */
extern long	maxipnpbytes;	/* max size of ip name pool */
extern char	*maxipnp;		/* end of ip name pool */
extern char	*ipnp;			/* ip name pool */
extern char	*freeipnp;		/* next free byte in ipnp */

extern FILE * ofd;			/* output file */
extern FILE * ifd;			/* intermediate form input file */
					/* all data is read with this FILE* */
extern FILE * isrcfd;			/* intermediate form input from pass1 */
extern FILE * ithunkfd;			/* intermediate form input from pass1 */
extern FILE * opermfd;			/* final output file */
extern FILE * otmpfd;			/* output file for "no emit" procs */
extern FILE * ipermfd;			/* input fd for opermfd */
extern FILE * itmpfd;			/* input fd for otmpfd */
extern FILE * oasfd;			/* .s file from pass1 -- append to it */

#define MAXSTRBYTES 300
#define MAXTEXTBYTES 2048
extern char infilename[MAXSTRBYTES];		/* name of input file */
extern char outfilename[MAXSTRBYTES];		/* name of output file */
extern char asfilename[MAXSTRBYTES];		/* name of .s file */
extern char	strbuf[MAXTEXTBYTES+1]; /* a temporary name building buffer */

#ifdef DEBUGGING
/* DEBUGGING FLAGS */
extern long xdebugflag;			/* echo input records as read */
#endif
extern long cfdumpflag;
extern long ctdumpflag;
extern long hdumpflag;
extern long ldumpflag;
extern long pdumpflag;			/* dump pd list between passes */
extern long verboseflag;		/* tell which calls are in-lined */
extern char caseflag;
extern char noemitseen;			/* "no emit" directive has been seen */
extern char assignflag;			/* FORTRAN ASSIGN has been seen */
extern char nowarnflag;			/* do not emit any warnings */
extern char ftnflag;

/*	masks for unpacking longs */
#define FOP(x) (int)((x)&0377)
#define VAL(x) (int)(((x)>>8)&0377)
#define REST(x) (((x)>>16)&0177777)

extern int dope[ DSIZE ];
extern char *opst[DSIZE];


/*	stack for reading nodes in postfix form */

# define NSTACKSZ 250
extern NODE **fstack;	/* used by pass2 (calling procedure) */
extern long fmaxstack;
extern NODE *node;
extern NODE *freenode;  /* pointer to next free node; (for allocator) */
extern NODE *maxnode;  /* pointer to last node; (for allocator) */

extern NODE **ipstack;	/* used by doip (called procedure */
extern long ipmaxstack;
extern NODE *ipnode;
extern NODE *ipfreenode;  /* pointer to next free node; (for allocator) */
extern NODE *ipmaxnode;  /* pointer to last node; (for allocator) */

extern long	maxstack;	/* max stack usage per procedure */

extern long 	nargs;		/* number of arguments in call */
extern long	cstackoffset;
#define MAXARGS  98
extern long	maxargs;	/* max supported # args */
extern long	minargoffset;	/* last arg offset from %a6 */
extern NODE	*arg[MAXARGS];	/* pointers to argument expressions */
extern flag	argvf[MAXARGS];	/* 1=c0 temp used, may need c2 volatile flag */

extern pPD	ippd;		/* pd for integrated proc */
extern long tempbase;		/* offset to subtract from all temporaries */

/* Expression call structures */

#define MAXEXPRCALLS 200
typedef struct exprcallrec {
	NODE *nptr;		/* pointer to NODE in expression tree */
	long retloc;		/* offset of return val from %a6 */
	pPD pd;			/* target pd */
	} ExprCallRec;
extern ExprCallRec call[MAXEXPRCALLS];
extern long nxtcall;		/* next free slot in "call" */
extern long callerSetregsMask;	/* SETREGS environment in calling routine */
extern long callerSetregsFDMask;/* SETREGS flt envir. in calling routine */
extern long callerC1opts;	/* C1 Options envir. in calling routine */

#define CLEARSETREGS 0		/* "clear" setting for setregs mask */
#define CLEARFDSETREGS 0	/* "clear" setting for flt setregs mask */
#define CLEARC1OPTS ( 0xF000 )	/* "clear" setting for C1 options mask */
#define CLEARC1OPTLEV ( 2 )	/* "clear" setting for C1 option level */

#define INCOMPAT_C1OPTS(a,b) ((a != b) || (((a & 0x0ff00) != 0x300) && (!ftnflag)))

extern char warnbuff[];		/* buffer for building warning message */

/* force/omit offset stuff */
extern long	nforced;	/* current # forced procs in array */
extern long	nomitted;	/* current # omitted procs in array */
extern long	maxforced;	/* size of array */
extern long	maxomitted;	/* size of array */
extern long	*forceoffsets;	/* pnpoffsets of current forced procs */
extern long	*omitoffsets;	/* pnpoffsets of current omitted procs */
/* define default size of offset arrays */
#define NOFFSETS 100

FILE *fileopen();
NODE *getnode();
NODE *getipnode();
long addtopnp();
char *addtotnp();
char *addtoipnp();
char *strcpy();
char *strchr();
pPD  pdfromname();
pPD  getpd();
pCallRec getcallrec();
pPD  pdfname();
HVREC *gethvrec();
HVREC *mapc1hvoreg();


extern long farg_high, farg_low, farg_pos;
