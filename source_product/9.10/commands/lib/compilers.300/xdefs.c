/* SCCS xdefs.c    REV(64.1);       DATE(92/04/03        14:22:41) */
/* KLEENIX_ID @(#)xdefs.c	64.1 91/08/28 */
/* file xdefs.c */

# include "mfile1"

/*	communication between lexical routines	*/

int	lineno;		/* line number of the input file */
#ifdef CXREF
int	id_lineno;	/* set in scanner when NAME seen */
#endif

CONSZ lastcon;  	/* the last constant read by the lexical analyzer */
double dcon;   	/* the last double read by the lexical analyzer */
# ifdef ANSI
	QUAD qcon;	/* last long double read by the lexical analyzer */
# endif /* ANSI */
struct symtab *stab_blk; /* pointer to array of symtab structures */
struct hashtab stab_hashtab[SYMHASHSZ]; /* hash table for symbol lookup */
SYMLINK stab_lev_head[NSYMLEV]; /* array of headers to linked list */
SYMLINK stab_freelist;  /* pointer to linked list of symtab entries that are free */
int stab_nextfree; 	/* index to next free element in symtab array */
int typ_initialize = 0; /* flag set in pftn.c to allow one initialization of
			   a const value */
int was_class_set = 0;  /* flag to indicate storage classes was changed */

#ifdef ANSI
int typeseen = 0;	/* flag to indicate a type keyword was seen */
int typedefseen = 0;	/* indicates typedef seen */
#endif /*ANSI*/

int type_signed = 0;	/* flag to indicate "signed" was seen */
#ifdef ANSI
flag signed_bitfields = 1; /* default is signed */
#else
flag signed_bitfields = 0; /* default is unsigned */
#endif

/*	symbol table maintainence */


struct symtab  *curftn;  /* "current" function */
int	ftnno;  /* "current" function number */

int	instruct,	/* "in structure" flag */
	stwart,		/* for accessing names which are structure members or names */
	blevel,		/* block level: 0 for extern, 1 for ftn args, >=2 inside function */
	curdim;		/* current offset into the dimension table */
	
int	paramno;  	/* the number of parameters */

#ifndef IRIF
int	autooff; 	/* the next unused automatic offset */
int	argoff;		/* the next unused argument offset */
#endif /* not IRIF */

int	strucoff;	/*  the next structure offset position */
#ifdef ANSI
int	last_enum_const;/* last value used (so far) in enum declaration */
#endif /*ANSI*/
int	regvar;		/* the next free register for register variables */
int	minrvar;	/* the smallest that regvar gets witing a function */
int	fdregvar;	/* the next free flt pt regs for register variables */
int	minrfvar;	/* the smallest that (f)dregvar gets in a function */
int	minrdvar;	/* the smallest that f(d)regvar gets in a function */
OFFSZ	inoff;		/* offset of external element being initialized */

struct sw *swp;  	/* pointer to next free entry in swtab */
int swx;  		/* index of beginning of cases for current switch */

char	curclass;	  /* current storage class */
volatile flag	double_to_float = 0; /* on during dtof conversions */
volatile flag	constant_folding = 0; /* on during constant folding */
volatile flag	fcon_to_icon = 0; /* on during fcon to icon conversion */
flag	brkflag = 0;	/* complain about break statements not reached */
flag reached;		/* true if statement can be reached... */
# ifdef LINT_TRY
int reachflg;		/* true if NOTREACHED encountered in function */
# endif

flag is_initializer;	/* if on, strings are to be treated as lists */

# ifdef DEBUGGING
flag xdebug = 0;	/* pass 2 debug flag to show how myreader works */
#endif

#ifdef ANSI
flag ansiflag = 0;	/* 1 = ANSI, 0 = K&R mode */
#endif
flag cflag = 0;  	/* do we check for funny casts */
flag hflag = 0;  	/* do we check for various heuristics which may
			   indicate errors */
flag pflag = 0;  	/* do we check for portable constructions */
flag sflag = 0;		/* do we check for alignment problems */
flag oflag;		/* true for additional optimizations. */

flag inlineflag = 0;	/* inline strcpy and monadics */

struct symtab *idname;		/* tunnel to buildtree for name id's */


int brklab;
int contlab;

#ifdef IRIF
int swlab;
#endif /* IRIF */

int flostat;
int retlab = NOLAB;
unsigned int svfdefaultlab;	/* safe exit label for falling out the end of a 
			   structure valued function. */

flag apollo_align = FALSE;
#ifdef IRIF
short align_like = ALIGN800;
#else
short align_like = ALIGN300;
#endif

#ifndef IRIF
#ifndef LINT
unsigned int al_char[NUMALIGNS]
	= {ALCHAR,AL500CHAR,AL800CHAR,ALCOMMONCHAR,ALCHAR};
unsigned int al_short[NUMALIGNS]
	= {ALSHORT,AL500SHORT,AL800SHORT,ALCOMMONSHORT,ALCHAR};
unsigned int al_int[NUMALIGNS]
	= {ALINT,AL500INT,AL800INT,ALCOMMONINT,ALCHAR};
unsigned int al_long[NUMALIGNS]
	= {ALLONG,AL500LONG,AL800LONG,ALCOMMONLONG,ALCHAR};
unsigned int al_point[NUMALIGNS]
	= {ALPOINT,AL500POINT,AL800POINT,ALCOMMONPOINT,ALCHAR};
unsigned int al_float[NUMALIGNS]
	= {ALFLOAT,AL500FLOAT,AL800FLOAT,ALCOMMONFLOAT,ALCHAR};
unsigned int al_double[NUMALIGNS]
	= {ALDOUBLE,AL500DOUBLE,AL800DOUBLE,ALCOMMONDOUBLE,ALCHAR};
unsigned int al_longdouble[NUMALIGNS]
	= {ALLONGDOUBLE,AL500LONGDOUBLE,AL800LONGDOUBLE,ALCOMMONLONGDOUBLE,ALCHAR};
unsigned int al_struct[NUMALIGNS]
	= {ALSTRUCT,AL500STRUCT,AL800STRUCT,ALCOMMONSTRUCT,ALCHAR};
#else
/* The first column of lint numbers are hard coded constants that match those
 * in lint.c for ALCHAR,ALSHORT,ALINT,...etc (these are variables in lint).
 * The values used correspond to the "portability" option alignments.  In the
 * case where the portability option is not given, lint will reset the initial
 * values of this table to constants more suitable to the architecture. */
unsigned int al_char[NUMALIGNS]
	= {8,AL500CHAR,AL800CHAR,ALCOMMONCHAR,ALCOMMONCHAR };
unsigned int al_short[NUMALIGNS]
	= {16,AL500SHORT,AL800SHORT,ALCOMMONSHORT,ALCOMMONCHAR};
unsigned int al_int[NUMALIGNS]
	= {16,AL500INT,AL800INT,ALCOMMONINT,ALCOMMONCHAR};
unsigned int al_long[NUMALIGNS]
	= {32,AL500LONG,AL800LONG,ALCOMMONLONG,ALCOMMONCHAR};
unsigned int al_point[NUMALIGNS]
	= {16,AL500POINT,AL800POINT,ALCOMMONPOINT,ALCOMMONCHAR};
unsigned int al_float[NUMALIGNS]
	= {32,AL500FLOAT,AL800FLOAT,ALCOMMONFLOAT,ALCOMMONCHAR};
unsigned int al_double[NUMALIGNS]
	= {64,AL500DOUBLE,AL800DOUBLE,ALCOMMONDOUBLE,ALCOMMONCHAR};
unsigned int al_longdouble[NUMALIGNS]
	= {64,AL500LONGDOUBLE,AL800LONGDOUBLE,ALCOMMONLONGDOUBLE,ALCOMMONCHAR};
unsigned int al_struct[NUMALIGNS]
	= {16,AL500STRUCT,AL800STRUCT,ALCOMMONSTRUCT,ALCOMMONCHAR};
#endif
#endif /* not IRIF */

int retstat;
int ccoptlevel=0;  /* optimization level given from cc line */
int optlevel=0;	   /* derived optimization level from cc line and pragmas */

#ifdef ANSI
int inproto = 0;
NODE * proto_head[NSYMLEV+1];	/* proto headers during parsing -- allow for
				 * one per symbol level possible.
				 */
long curclass_save[NSYMLEV];    /* These arrays are used to save/restore the */
long instruct_save[NSYMLEV];    /* globals "curclass" and "instruct"
				 * around a prototype scope since structs
				 * and prototypes can get nested.
				 */

#endif /*ANSI*/

# ifdef DEBUGGING
static char *
ccnames[] = { /* names of storage classes */
	"SNULL",
	"AUTO",
	"EXTERN",
	"STATIC",
	"REGISTER",
	"EXTDEF",
	"LABEL",
	"ULABEL",
	"MOS",
	"PARAM",
	"STNAME",
	"MOU",
	"UNAME",
	"TYPEDEF",
	"FORTRAN",
	"ENAME",
	"MOE",
	"UFORTRAN",
	"USTATIC",
	};

char * scnames( c ) register c; {
	/* return the name for storage class c */
	static char buf[12];
	if( c&FIELD ){
		(void)sprntf( buf, "FIELD[%d]", c&FLDSIZ );
		return( buf );
		}
	return( ccnames[c] );
	}
# endif /* DEBUGGING */

#ifdef ANSI
     flag cpp_ccom_merge = 0;   /* true if preprocessor/scanner merged */
#endif /* ANSI */


#ifdef SILENT_ANSI_CHANGE
#  ifndef LINT_TRY
flag silent_changes = 0;
#  else
flag silent_changes = 1;
#  endif /* LINT_TRY */
#endif
