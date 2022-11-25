/* file c1.h */
/* @(#) $Revision: 70.1 $ */
/* KLEENIX_ID @(#)c1.h	16.2 90/11/15 */

# include <malloc.h>
# include "mfile2"

# define FTN_POINTERS   /* Changes to implement Fortran pointers. */

# ifdef MDEBUGGING	/* Define this to get an onionskin around malloc calls */
#	ifndef    NOCHKMEM
#		define    MALLOC_TYPE       void
#		define    malloc(a)	     chk_malloc(a,__FILE__,__LINE__)
#		define    calloc(a,b)	     chk_calloc(a,b,__FILE__,__LINE__)
#		define    realloc(a,b)	     chk_realloc(a,b,__FILE__,__LINE__)
#		define    free(a)	         chk_free(a,__FILE__,__LINE__)
		MALLOC_TYPE *chk_malloc(), *chk_calloc(), *chk_realloc();
#	endif     /* NOCHKMEM */

#	ifdef    CHKMEM_COMPILE

#		define   SIZE_TYPE            long

		static char *side = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
#		define   MEM_ALIGN            4

#		include  <signal.h>
#		define   ABORT                kill(getpid(),SIGQUIT)
	
#		define   START_FREE_SIZE      1000

		int chk_sides = 1;

#	else    /* CHKMEM_COMPILE */

		extern int chk_sides;

#	endif   /* CHKMEM_COMPILE */
# endif MDEBUGGING

typedef struct lbh LBLOCK;
typedef union hr HASHU;
typedef struct child CHILD;
typedef union bbl BBLOCK;
typedef struct link LINK;
typedef struct plink PLINK;
typedef struct clink CLINK;
typedef struct dft DFT;
typedef struct vferef VFEREF;
typedef struct vfecode VFECODE;
typedef union epb EPB;
typedef struct  cgu CGU;
typedef struct region REGION;
typedef struct defuseblock DUBLOCK;
typedef struct web WEB;
typedef struct loadst LOADST;
typedef struct deftab DEFTAB;
typedef struct stmt STMT;
typedef struct callduitem CALLDUITEM;
typedef struct reg_class_list REGCLASSLIST;
typedef union symtabattr SYMTABATTR;
typedef struct regioninfo REGIONINFO;

typedef union	/* label cracker */
		{
		int x;
		struct
			{
			unsigned l:24;
			unsigned op:8;
			} s;
		} LCRACKER;


struct lbh			/* label block header */
	{
	unsigned	val;		/* numerical value of the label */
	BBLOCK	        *bp;		/* ptr to associated BBLOCK */
	LINK		*pred;		/* ptr to LBLOCK predecessor list */
	PLINK		*pref_llistp;	/* ptr to list of "preferred" labels */
	SET		*in;		/* in[] set */
	SET		*out;		/* out[] set */
	flag		visited:1;
	flag		preferred:1;	/* YES iff target of indirect ref */
	ushort		fill:14;
	};

union bbl			/* basic block header */
	{
	struct			/* before symbolic links replaced */
		{
		uchar breakop;
		uchar deleted;	/* block is unreachable ?? */
		ushort 	node_position;	/* dfst traversal order */
		NODE *treep;	/* points to first semicolonop in block */
		unsigned llabel, rlabel;
		NODE *unused;
		SET *gen, *kill;/* topmost gen[S] and kill[S] */
		ushort	nestdepth;	/* number of regions contained in */
		ushort	nstmts;		/* number of statement trees in block*/
		long	firststmt;	/* first statement # in block */
		REGION	*region;	/* LOOP descriptor for this block */
		PLINK	*entries;
		LBLOCK l;	/* copied here from labelpool after coalescing
				   is complete.
				*/
		} l;
	struct
		{		/* after symbolic links replaced */
		uchar breakop;
		uchar deleted;	/* block is unreachable ?? */
		ushort	node_position;	/* dfst traversal order */
		NODE *treep;	/* points to first semicolonop in block */
		LBLOCK *llp, *rlp;	/* assume at most a binary tree */
		NODE *lastasgnode;/* used in gen_and_kill(). */
		SET *gen, *kill;/* topmost gen[S] and kill[S] */
		ushort	nestdepth;	/* number of regions contained in */
		ushort	nstmts;		/* number of statement trees in block*/
		long	firststmt;	/* first statement # in block */
		REGION	*region;	/* LOOP descriptor for this block */
		PLINK	*entries;
		LBLOCK l;	/* copied here from labelpool after coalescing
				   is complete.
				*/
		} b;
	struct
		{
		uchar breakop;
		uchar deleted;	/* block is unreachable ?? */
		ushort	node_position;	/* dfst traversal order */
		NODE *treep;	/* points to first semicolonop in block */
		BBLOCK *lbp, *rbp;	/* assume at most a binary tree */
		NODE *lastasgnode;/* used in gen_and_kill(). */
		SET *gen, *kill;/* topmost gen[S] and kill[S] */
		ushort	nestdepth;	/* number of regions contained in */
		ushort	nstmts;		/* number of statement trees in block*/
		long	firststmt;	/* first statement # in block */
		REGION	*region;	/* LOOP descriptor for this block */
		PLINK	*entries;
		LBLOCK l;	/* copied here from labelpool after coalescing
				   is complete.
				*/
		} bb;		/* After clean_flowgraph() is complete */
	struct
		{
		uchar breakop;
		uchar deleted;	/* block is unreachable ?? */
		ushort 	node_position;	/* dfst traversal order */
		NODE *treep;
		int nlabs;
		CGU *ll;
		NODE *lastasgnode;/* used in gen_and_kill(). */
		SET *gen, *kill;/* topmost gen[S] and kill[S] */
		ushort	nestdepth;	/* number of regions contained in */
		ushort	nstmts;		/* number of statement trees in block*/
		long	firststmt;	/* first statement # in block */
		REGION	*region;	/* LOOP descriptor for this block */
		PLINK	*entries;
		LBLOCK l;	/* copied here from labelpool after coalescing
				   is complete.
				*/
		} cg;
	};

/* Within the register allocator, a region descriptor is allocated for
 * each region (e.g., FORTRAN DO-loop).  There is also a descriptor for
 * the entire procedure.  These descriptors are arranged in a tree which
 * shows region containment.  The n-ary tree is represented via a binary
 * tree using the "child" and "sibling" fields.
 */

struct region			/* region descriptor */
	{
	ushort		regionno;	/* region number */
	ushort		isFPA881loop;
	SET		*blocks;	/* all blocks in this region */
	SET		*onlyblocks;	/* blocks not in any child regions */
	REGION		*child;
	REGION		*sibling;
	REGION		*parent;
	ushort		nstmts;
	ushort		nestdepth;
	PLINK		*entryblocks;	/* list of entry blocks for region */
					/* inner dom for loops */
	LOADST		*exitstmts;	/* list of exit stmts for region */
					/* stmt1 == last stmt of block */
					/* stmt2 == first stmt of next block */
	SET		*containedin;	/* regions contained in, including */
					/* this one */
	SET		*ifset;		/* regions this region interferes
					/* with */
	long		n881ops;	/* number of must-881 ops seen */
	long		nFPAops;	/* number of FPA ops seen */
	unsigned	niterations;
	REGIONINFO	*regioninfo;
	};


struct regioninfo
	{
	ushort		type:8;		/* 0 == unknown; 1 == FOR; 2 == DO */
					/* 3 == DBRA */
	ushort		unreachable:1;
	ushort		empty:1;
	ushort		const_init_value:1;
	ushort		const_bound_value:1;
	ushort		index_modified_within_loop:1;
	ushort		bound_modified_within_loop:1;
	ushort		dbra_instruction_inserted:1;
	ushort		is_short_dbra:1;
	long		increment_value;  /* -1, 1 or other (0) */
	unsigned	niterations;	/* 0 == unknown
					 * >0 == actual number
					 */
	struct
	    {
	    ushort  srcblock;		/* source of back branch (dfo #) */
	    ushort  destblock;		/* dest of back branch (dfo #) */
	    }		backbranch;
	HASHU		*index_var;	/* index variable */
	NODE		*initial_load;
	NODE		*initial_test;
	NODE		*bound_load;
	NODE		*increment;
	NODE		*test;
	NODE		*epilog_increment;
	ushort		initial_load_block;
	ushort		initial_test_block;
	ushort		bound_load_block;
	ushort		increment_block;
	ushort		test_block;
	ushort		epilog_increment_block;
	};


/* The following three structures are assumed to be the same size.  All
 * are allocated by calling alloc_plink() in regweb.c.  If you change
 * the relative sizes, please check alloc_plink(), free_plink(),
 * file_init_storage() and proc_init_storage() in regweb.c for effects.
 */

struct link			/* linked-list node for predecessors */
	{
	BBLOCK		*bp;
	LINK		*next;
	};



struct plink			/* linked-list node for preferred labels */
	{
	unsigned	val;
	PLINK		*next;
	};


struct clink			/* linked-list node for common and array  */
	{			/* 	members */
	unsigned	val;	/* HASHU # in symtab */
	CLINK		*next;
	};


struct cgu			/* dynamic array of computed goto labels */
				/* also used for assigned-GOTO */
	{
	long		caseval;
	unsigned 	val;
	LBLOCK 		*lp;
	CGU		*next;
	int		nlabs;
	flag		nonexec;/* For assigned-GOTO only.Non executable label*/
	};

/* The vferef structure describes a single reference to a variable FORMAT
 * expression "thunk".  All references to a particular thunk are chained
 * off the vfethunk structure below.
 */

struct vferef			/* linked-list of vfe references */
	{
	NODE		*np;	/* jsr node */
	long		bp;	/* block containing jsr node */
	VFEREF		*next;	/* next vfe reference in chain */
	};


/* The vfethunk structure describes a single variable FORMAT expression
 * "thunk" (the procedure that processes the variable FORMAT expressions
 * and inserts the values in the FORMAT structure).
 */
struct vfethunk			/* variable FORMAT expression thunks */
	{
	char		*label;
	VFEREF		*refs;
	VFECODE		*thunk;
	BBLOCK		*startblock;
	LBLOCK		*startlabel;
	long		nblocks;
	ushort		asg_fmt_target:1;
	ushort		nthunkvars:15;
	ushort		*thunkvars;
	};

struct vfecode
	{
	NODE		*np;	/* code for this block */
	long		labval;	/* label for this block */
	VFECODE		*next;	/* next block of code, if any */
	};


/* Attribute structure for variable attributes passed in by C1NAME or C1OREG
 * record.
 */
union symtabattr{
    struct {
        flag common:1;
        flag farg:1;
        flag equiv:1;	/* equivalence*/
	flag isvolatile:1;
        flag complex:1;
        flag ptr:1;
        flag array:1;
        flag func:1;
        flag hiddenfarg:1;
        flag inregister:1;
        flag isexternal:1;
	flag isc0temp:1;
        flag filler:4;
        ushort fill;
        } a;
    long l;
    };


/* The "union hr" are the symbol table entries.  Each member of the union
 * is a structure for the particular "tag" field value.
 */ 

# define ISFARG		0x80000000
# define ISCOMMON	0x40000000
# define ISEQUIV	0x20000000
# define ISARRAY	0x10000000
# define ISFUNC		0x4000000
# define ISCOMPLEX 	0x1800000
# define ISHIDDENFARG	0x400000
# define ISSTRUCT 	0x100000
# define ISEXTERNAL	0x80000
# define SCONST 	0x40000
# define SEENININPUT	0x20000

enum hrtag {SFREE, ON, AN, A6N, CN, XN, X6N, SN, S6N};
typedef enum hrtag HRTAG;

union hr
	{
	struct
		{
		int top;
		int offset;
		SET *defset;
		int filler;
		NODE *cv;
		ushort	blds;
		ushort	cvbnum;
		int regval;
		unsigned attributes;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;	/* index of complex2 part, if any */
		} allo;		/* allocator only */

	struct
		{
		HRTAG	tag:4;	/* tag == ON */
		flag	useflag:1;/* YES iff all uses are reached only by the
				     candidate loop-invariant definition */
		uchar	register_type:3;
		unsigned index:24;
		int 	offset;	/* not used with this tag */
		SET     *defset;/* set of all definition stmt indices */
		int	filler;
		NODE 	*cv;	/* ptr to const ndu for const propagation */
				/* also used as ptr to linked list of */
				/* statement #'s which define fargs -- */
				/* regdefuse.c */
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort 	cvbnum;	/* block # for constant propagation use */
		int	regval;	/* register number */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} on;		/* terminal node ... register */
	struct
		{
		HRTAG	tag:4;	/* tag == AN */
		flag	useflag:1;/* YES iff all uses are reached only by the
				     candidate loop-invariant definition */
		uchar	register_type:3;
		unsigned index:24;
		int 	offset;
		SET     *defset;/* set of all definition stmt indices */
		int	filler;
		NODE 	*cv;
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort	cvbnum;
		char	*ap;	/* the name is a pointer to asciz table */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} an;		/* terminal node ... asciz name */
	struct
		{
		HRTAG	tag:4;	/* tag == A6N */
		flag	useflag:1;/* YES iff all uses are reached only by the
				     candidate loop-invariant definition */
		uchar	register_type:3;
		unsigned index:24;
		int 	offset;	/* the name is an offset off of A6 */
		SET     *defset;/* set of all definition stmt indices */
		int	filler;
		NODE 	*cv;
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort	cvbnum;
		int	nexthiddenfarg;   /* symtab index of next hidden farg */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} a6n;		/* terminal node ... relative address */
	struct
		{
		HRTAG	tag:4;	/* tag == CN */
		flag	useflag:1;
		uchar	register_type:3;
		unsigned index:24;
		int	offset;	/* offset from beginning of the region */
		SET     *defset;/* set of all definition stmt indices */
		CLINK	*member;/* ptr to linked list of member HASHU structs */
		NODE 	*cv;
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort	cvbnum;
		char	*ap;	/* the name is a pointer to asciz table */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} cn;		/* common node */
	struct
		{
		HRTAG	tag:4;	/* tag == XN */
		flag	useflag:1;
		uchar	register_type:3;
		unsigned index:24;
		int	offset;	/* offset from beginning of the region */
		SET     *defset;/* set of all definition stmt indices */
		CLINK	*member;/* ptr to linked list of member HASHU structs */
		NODE 	*cv;
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort	cvbnum;
		char	*ap;	/* the name is a pointer to asciz table */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} xn;		/* static array node */
	struct
		{
		HRTAG	tag:4;	/* tag == X6N */
		flag	useflag:1;
		uchar	register_type:3;
		unsigned index:24;
		int	offset;	/* offset from beginning of the region */
		SET     *defset;/* set of all definition stmt indices */
		CLINK	*member;/* ptr to linked list of member HASHU structs */
		NODE 	*cv;
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort	cvbnum;
		int	*fillp;	/* not used with this tag */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} x6n;		/* dynamic array node */
	struct
		{
		HRTAG	tag:4;	/* tag == SN */
		flag	useflag:1;
		uchar	register_type:3;
		unsigned index:24;
		int	offset;	/* offset from beginning of the region */
		SET     *defset;/* set of all definition stmt indices */
		CLINK	*member;/* ptr to linked list of member HASHU structs */
		NODE 	*cv;
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort	cvbnum;
		char	*ap;	/* the name is a pointer to asciz table */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} sn;		/* static structure node */
	struct
		{
		HRTAG	tag:4;	/* tag == S6N */
		flag	useflag:1;
		uchar	register_type:3;
		unsigned index:24;
		int	offset;	/* offset from beginning of the region */
		SET     *defset;/* set of all definition stmt indices */
		CLINK	*member;/* ptr to linked list of member HASHU structs */
		NODE 	*cv;
		ushort	blds;	/* dfo[i] of last definition seen */
		ushort	cvbnum;
		int	*fillp;	/* not used with this tag */
		flag 	farg:1;	/* YES iff this is a formal argument */
		flag	common:1;  /* YES iff in COMMON */
		flag	equiv:1;  /* YES iff in EQUIVALENCE or volatile */
		flag	array:1;	/* YES iff array or array element */
		flag	ptr:1;		/* YES iff pointer */
		flag	func:1;		/* YES iff dummy function arg */
		flag	arrayelem:1;	/* YES iff array element */
		flag	complex1:1;	/* YES iff first half of COMPLEX */
		flag	complex2:1;	/* YES iff second half of COMPLEX */
		flag	hiddenfarg:1;	/* YES iff hidden farg */
		flag	inregister:1;	/* YES iff "register" var */
		flag    isstruct:1;	/* YES iff struct or in struct */
		flag    isexternal:1;	/* YES iff external sym */
		flag    isconst:1;	/* YES iff constant */
		flag    seenininput:1;	/* YES iff seen in input stream */
		flag    dbra_index:1;	/* YES iff is dbra_index in a loop */
		flag    no_effects:1;	/* YES iff proc has no side effects */
		flag    uninit_warned:1;  /* YES iff already warned for
					   * uninitialized var */
		flag	isc0temp:1;	/* YES iff created by c0 */
		ushort	afill:13;
		TTYPE type;
		int array_size;
		ushort symtabindex;
		ushort wholearrayno;
		char *pass1name;
		DUBLOCK *du;
		TWORD 	lastdefno;
		ushort	regno;
		ushort	varno;
		ushort	stmtno;
		ushort	back_half;
		} s6n;		/* dynamic structure node */
	};




struct child
	{
	NODE *np;
	unsigned k;
	};


/* Definition table "deftab" element specification.  One per definition. */

struct dft
	{
	NODE *np;			/* ptr to controlling CALL or asgmt */
	NODE *stmt;			/* ptr to controlling ";" node */
	ushort bn;			/* controlling block number (dfo) */
	ushort children:15;		/* number of trailing dfts on same np */
	flag constrhs:1;		/* TRUE iff rhs is constant */
	int sindex;			/* symtable index of "defined" var */
	};


struct iv
	{
	HASHU *family_sp;	/* symtab index of "family" induction var */
	HASHU *j_sp;		/* symtab index of "j" (alg. 10.9 term) */
	HASHU *temp_sp;		/* symtab index of "s" (alg. 10.10 term) */
	NODE *mc;		/* scale factor */
	NODE *ac; 		/* additive offset */
	NODE *tnp;		/* used only for minor aivs */
	struct iv *sibling;	/* another occurrence of same iv in this loop */
	int dti;		/* deftab index of the assignment */
	int bn;			/* block # in which the aiv occurs */
	flag initted;		/* YES if preheader already has init code */
	};

/* Entry point descriptor.  Each descriptor describes a single entry to the
 * procedure.
 */

union epb
	{
	struct
		{
		int val;
		LBLOCK *lp;
		NODE *nodep;
		char name[NCHNAM];
		} l;
	struct
		{
		int val;
		BBLOCK *bp;
		NODE *nodep;
		char name[NCHNAM];
		} b;
	};


/* "web" descriptor used in the register allocator.  The three forms supported
 * by this structure are: inter-block web, intra-block web, and loop.
 */

struct web
	{
	unsigned isweb:1;	/* 1 == web, 0 == loop */
	unsigned isinterblock:1; /* 1 == interblock web, 0 == intrablock */
	unsigned isdefsonly:1;  /* 1 == web contains no USEs */
	unsigned hasdefiniteuse:1; /* 1 == web contains >= 1 definite use */
	unsigned inFPA881loop:1;	/* 1 == in 881 loop during +ffpa */
	char	regno;		/* assigned register number */
	struct {
	    SET *loops;		/* potential loops for register allocation */
	    } s;
	HASHU	*var;		/* pointer to symbol table record */
	long	tot_savings;	/* total savings for web/loop */
	long	adj_savings;	/* tot_savings div # trees */
	WEB	*next;		/* next web in list */
	WEB	*prev;		/* prev web in looplist */
	LOADST	*load_store;	/* load-store list */
	union {
	    struct {		/* inter-block web */
		DUBLOCK	*du;		/* def-use block list */
		SET	*if_regions;	/* interfering regions */
		SET	*reachset;	/* reachings defs of web */
		SET	*live_range;	/* live range block set */
		long	nstmts;		/* number of stmts in web */
		} inter;
	    struct {		/* intra-block web */
		DUBLOCK	*du;		/* def-use block list */
		SET	*if_regions;	/* interfering regions */
		SET	*reachset;	/* reachings defs of web */
		long	first_stmt;	/* first statement of live range */
		long	last_stmt;	/* last statement of live range */
		} intra;
	    struct {		/* loop */
		WEB	*web;		/* associated web */
		long	regionno;	/* region number */
		} loop;
	    } webloop;
	};

/* load-store descriptor.  For specifying a needed register load or store. */

struct loadst
	{
	unsigned short regionno;
        unsigned short blockno;
	unsigned short isload:1;	/* 1 == load, 0 == store */
	long	stmt1:31;	/* first statement:  <= 0 is an entry pt
						      > 0 is stmt num
						PREHEADNUM is loop preheader */
	long	stmt2;		/* second statement */
	LOADST	*next;		/* next item in list */
	};
	

/* def-use descriptor.  Each block describes a single definition or use
 * or a variable (actual or implied).
 */

struct defuseblock
	{
	unsigned isdef:1;	/* 1 == def block; 0 == use block */
	unsigned ismemory:1;	/* 1 == memory ref; 0 == reg ref */
	unsigned isdefinite:1;	/* 1 == definite; 0 == maybe def/use */
		unsigned hides_maybe_use:1; /* 1 == A maybe use was present but was
				     hidden by a definite use */
	unsigned deleted:1;	/* 1 == this DU block deleted (non reg alloc
				   only) */
	unsigned inFPA881loop:1;	/* 1 == in 881 loop during +ffpa */
	unsigned parent_is_asgop:1;	/* 1 == parent is INCR/DECR/asgop */
		unsigned defsetno:25;		/* number of this def */
					/* 0 means use block's reach def
					/*   in set */
	long	stmtno;		/* statement number */
	long	savings;	/* cpu cycles saved if allocated to reg */
	DUBLOCK	*next;		/* next DUBLOCK in chain */
	union
	    {
	    DUBLOCK	*nextloop;	/* next DUBLOCK in chain for loop */
	    NODE	*parent;	/* parent of this item in tree */
	    } d;
	};


/* register allocator definetab[] entry.  Each structure describes a single
 * definition.
 */

struct deftab
	{
	NODE	*np;		/* ptr to controlling CALL or asgmt */
	long	stmtno;		/* statement number */
	HASHU	*sym;		/* pointer to symbol table entry */
	ushort	nuses;
	unsigned deleted:1;
	unsigned isdefinite:1;
	unsigned hassideeffects:1;
	unsigned allusesindefstmt:1;
	unsigned OKforconstprop:1;
	unsigned OKfordeadstore:1;
	unsigned OKforcopyprop:1;
	unsigned inFPA881loop:1;
	unsigned filler:8;
	};


/* register allocator stmttab[] entry.  Each structure describes a single
 * statement.
 */

struct stmt
	{
	NODE	*np;		/* ptr to tree node */
	ushort	block;		/* block number */
	ushort	hassideeffects:1;   /* stmt contains CALL, imbedded ASSIGN or
				     * reference to volatile var */
	ushort	OKforcopyprop:1;    /* rhs is OK for use in copy prop */
	ushort	deleted:1;	/* this stmt has been deleted */
	ushort	filler:13;
	};


/* Descriptor for symtab items affected by a CALL.  An array of these is
 * allocated when processing a CALL during the first pass of the register
 * allocator.  The array is filled with items directly or implicitly
 * def'd or use'd by a CALL.  Each symtab entry may appear in the array
 * at most once.  After processing, individual defuseblocks are allocated
 * for each entry in the array.
 */

struct call_def_use_item
	{
	HASHU 	*sym;		/* ptr to symtab entry */
	ushort   ndefdefs;	/* # definite defs */
	ushort   ndefuses;	/* # definite uses */
		unsigned  has_non_definite_use:1; /* has at least 1 non definite use */
	unsigned def:1;		/* 1 == def */
	unsigned use:1;		/* 1 == use */
	unsigned ismem:1;	/* 1 == memory def and/or use */
	unsigned isdefinitedef:1;	/* 1 == definite def */
	unsigned isdefiniteuse:1;	/* 1 == definite use */
	};


/* Array of call_def_use_item's above.  See above narrative for function. */

struct callduitem
	{
	long	lastitem;		       /* index of last item in items */
	struct call_def_use_item items[1];	/* items affected by call */
	};

/* Colored vars descriptor. */

struct colored_var
	{
	HASHU	*var;		/* symtab pointer */
	SET	*varifset;	/* set of interfering vars, by varno */
	SET	*stmtset;	/* set of stmts this var is in register */
	short	regno;		/* allocated register number */
	short	deleted;	/* is this var deleted??? */
	};

/* Register class descriptor.  One for A-regs, D-regs and float-regs. */

struct reg_class_list
	{
	WEB	*list;		/* list of web/loop candidates */
	long	nlist_items;	/* # entries in list */
	WEB	**colored_items;  /* vector of colored webs/loops */
	long	ncolored_items;   /* # items in colored_items vector */
	struct colored_var *colored_vars;  /* vector of colored vars */
	long	ncolored_vars;	/* # items in colored_vars vector */
	ushort  *regmap;
	char	high_regno;
	char	nregs;
	short	storecost;
	};

/* Information about each register class.  One per A-reg, D-regs, 881-regs and
 * FPA-regs
 */

struct reg_type_info
	{
	char	high_regno;	/* highest available regno in class, e.g., 7
				   for D7 */
	char	nregs;		/* number of available regs in class */
	};

/* comment_buf is the structure used to create a linked list of FTEXT nodes.
 * All FTEXT nodes are saved so that they can be emitted after a FLBRAC node.
 */
struct comment_buf 
         {
	 int ftextint;		/* FTEXT NODE INDICATOR */
	 char *comment;		/* actual FTEXT comment */
	 struct comment_buf *next;  /* next comment in list */
	 };


extern FILE 	*lrd;
extern char	*procname;	/* name of current procedure */
extern CGU	*asgtp;
extern int	ficonlabsz;
extern flag	arrayflag;	/* c1.c */
extern flag	arrayelement;	/* c1.c */
extern flag	structflag;	/* c1.c */
extern flag	isstructprimary;	/* c1.c */

extern BBLOCK 	*blockpool;
extern BBLOCK 	*topblock;	/* ptr to first basic block in the function */
extern BBLOCK 	*lastblock;	/* ptr to last active block in table */
extern BBLOCK   *top_preheader;	/* ptr to preheader of region including dfo[0],
				   if any
				*/
extern NODE    	**seqtab;	/* sequence table ... array of np types */
extern struct iv *ivtab;	/* induction variable table */
extern DFT      *deftab;	/* definitions table */
extern CHILD    *common_child;	/* common child stack */
extern union epb *ep;		/* entry point table */
extern BBLOCK  	**dfo;		/* dfst ordering table */
extern SET     	**region;	/* a natural "loop" bitvector array */
extern SET     	**lastregion;	/* a natural "loop" bitvector array */
extern SET     	**currentregion;/* a natural "loop" bitvector array */
extern SET     	**blockdefs;	/* array of SETs of definitions per block */
extern SET	**predset;
extern SET	**succset;
extern SET	**dom;
extern int	*preheader_index;
				/* dfo indice of preheader for the region(s) */
extern REGIONINFO *regioninfo;	/* region structure information */
extern long	lastregioninfo;	/* last filled entry in regioninfo array */
extern flag	needregioninfo;
extern flag	reducible;	/* flowgraph is t1_t2 reducible */
extern 		maxregion;	/* max # of regions per function */
extern		lastn;		/* number of last active non-terminal NODE */
extern		numblocks;	/* number of basic blocks ... changeable */
extern int	maxnumblocks;
extern		lastdef;	/* number of last active define (lhs of asgop)*/
extern int	nodecount;	/* node numbering for strength reduction */
extern		maxdefs;	/* max number of defines (including temp assigns) */
extern int 	dcount;
extern int 	acount;
extern		maxivsz;	/* allocated size of the ivtab[] */
extern		pseudolbl;
extern int	maxlabstksz;	/* max size of the label stack */
extern unsigned *labvals;	/* the label stack */
extern unsigned *lstackp;	/* the label TOS */
extern flag	defsets_on;	/* auto creation of sp->defset */
extern flag	changing;	/* loops.c. Communication flag */
extern SET	*looppreheaders;  /* indexed by blockno, 1 if a preheader */
extern int	lastep;		/* index of last entry point in ep table */
extern int	maxepsz;	/* size of ep table */

extern void	oreg2plus();			/* in misc.c */
extern void	tfree();			/* in misc.c */
extern void	tfree2();			/* in misc.c */
extern void	tinit();			/* in misc.c */
extern void	proctinit();			/* in misc.c */
extern void	lccopy();			/* utils.c */
extern void	lcread();			/* utils.c */
extern void	addexpression();		/* utils.c */
extern void	pushlbl();			/* utils.c */
extern void	bfree();			/* utils.c */
extern void	do_hiddenvars();		/* utils.c */
extern void	clean_flowgraph();		/* utils.c */
extern void	complete_asgtp_table();		/* utils.c */
extern void	funcinit();			/* utils.c */
extern void	node_constant_collapse();	/* utils.c */
extern void	cerror();			/* in misc.c */
extern void	uerror();			/* in misc.c */
extern void	canon();			/* in misc.c */
extern void	node_constant_fold();		/* in misc.c */
extern void	global_optimize();		/* in oglobal.c */
extern void	find_doms();			/* in oglobal.c */
extern void	init_defs();			/* in oglobal.c */
extern void	update_flow_of_control();	/* in oglobal.c */
extern void	setup_global_data();		/* in oglobal.c */
extern void	free_global_data();		/* in oglobal.c */
extern void	local_optimizations();		/* in file dag.c */
extern void	rewrite_comops();		/* in misc.c */
extern void	rewrite_comops_in_block();	/* in misc.c */
extern void 	addgoto();			/* in loops.c */
extern void	p2name();			/* in p2out.c */
extern void	p2word();			/* in p2out.c */
extern void	p2flush();			/* in p2out.c */
extern void	process_vfes();			/* in vfe.c */
extern void rm_farg_aliases();
extern void check_tail_recursion();
extern LBLOCK	*enterlblock();			/* utils.c */
extern BBLOCK  	*mkblock();			/* c1.c */
extern BBLOCK 	*mknewpreheader();		/* in loops.c */
extern NODE  	*delete_stmt();			/* c1.c */
extern NODE	*mktemp();			/* in misc.c */
extern NODE	*cptemp();			/* in misc.c */
extern TTYPE	decref();			/* in misc.c */
extern TTYPE	incref();			/* in misc.c */
extern long	find_or_add_vfe();		/* in vfe.c */
extern int	flags_and_files();		/* utils.c */
extern SET     	*new_set();			/* in misc.c */
extern SET     	**new_set_n();			/* in misc.c */
extern ushort 	new_definition();		/* in oglobal.c */
extern flag	hardconv();			/* in misc.c */
extern flag	worthit();			/* dag.c */
extern flag	differentsets();		/* in sets.s */
extern		clearset();			/* in sets.s */
extern		setassign();			/* in sets.s */
extern flag	gosf();				/* oglobal.c */

extern flag 	node_in_tree();
extern int 	get_statement_no();
extern int 	find_node_statement();





#define VFE_TABLE_SIZE 20
extern flag	vfes_to_emit;	/* 1 == non-empty vfes to emit */
extern struct vfethunk *vfe_thunks;
extern VFEREF	*vfe_anon_refs;		/* refs to vfes in assigned FMTS */
extern long	last_vfe_thunk;		/* index into vfe_thunks table */
extern long	curr_vfe_thunk;
extern long	max_vfes;               /* size of vfe_thunks table */
extern flag	non_empty_vfe_seen;

/* symtab.c defines */
extern HASHU  **hashsymtab;
extern HASHU  **symtab;
extern long	maxhashsym;
extern long	lastexternsym;
extern long	lastfilledsym;
extern long	maxsym;
extern int 	comtsize;
extern int 	fargtsize;
extern int 	ptrtsize;
extern int	lastcom;
extern int	lastfarg;
extern int	lastptr;
extern unsigned	*comtab;
extern unsigned	*fargtab;
extern unsigned	*ptrtab;
extern flag	f_find_array;	/* flag to find() to look for XN, X6N */
extern flag	f_find_common;	/* flag to find() to look for CN */
extern flag	f_find_extern;	/* flag to find() that we're adding an extern */
extern flag	f_find_struct;	/* flag to find() to look for SN, S6N */
extern flag	f_find_pass1_symtab;	/* flag to find() that we're adding
					 * because of pass1 symtab info
					 */
extern flag     f_do_not_insert_symbol; /* flag to find() not to insert
					 * symbol if it is not found 
					 */
extern flag     disable_look_harder;    /* flag to find() to disable
					 * calls to look_harder() when it
					 * is not necessary.
					 */

extern flag	verbose;	/* Flag to enable selected warnings. */
extern flag	gcp_disable;	/* flag to disable global const. prop. */
extern flag	sr_disable;	/* flag to disable strength reduction */
extern flag	deadstore_disable;	/* disable dead store removal */
extern flag	copyprop_disable;	/* disable local copy propagation */
extern flag	constprop_disable;	/* disable dead store const prop */
extern flag	disable_unreached_loop_deletion;
extern flag	disable_empty_loop_deletion;
extern flag	dbra_disable;
extern flag	loop_unroll_disable;
extern flag	coalesce_disable;
extern flag	warn_disable;
extern flag	comops_disable;
extern flag	udisable;		/* turn off combine_cmtemps() */

extern flag	paren_enable;

extern flag	loop_allocation_flag;	/* 1 == do register allocation by
					     loops and webs,
					   0 == do reg alloc only by webs */
extern flag	in_reg_alloc_flag;	/* 1 == in register allocator */
					/* 0 == not */
	extern flag	pic_flag;		/* YES: position independant code */

extern long	farg_low;		/* lowest -addr in local space,eg  -4 */
extern long	farg_high;		/* high -addr in local space, eg -27 */
extern long	farg_diff;		/* diff for translation, eg 4, then */
					/* new = diff - (stack offset) */
					/* e.g, 8 = 4 - (-4) */

extern HASHU	**varno_to_symtab;	/* varno to symtab vector */
extern ushort	lastvarno;		/* # items for reg alloc purposes */
					/* also, # items in varno_to_symtab */
extern STMT	*stmttab;		/* statement descriptor array */
extern long	laststmtno;		/* last entry in stmttab */
extern long	maxstmtno;
extern long	nexprs;			/* number of expressions */
extern DEFTAB	*definetab;		/* definition structure */
extern long	lastdefine;		/* last entry in definetab */
extern long	maxdefine;
extern long	ncalls;			/* number of calls in the proc */
extern SET	**genreach;		/* reaching defs "gen" sets for each 
						block */
extern SET	**killreach;		/* reaching defs "kill" sets for each
						block */
extern SET	**inreach;		/* reaching defs "in" set for each
						block */
extern SET	**outreach;		/* reaching defs "out" set for each
						block */
extern SET	**outreachep;		/* reaching defs "out" sets for each
						ep */
extern SET	**genlive;		/* live vars "gen" sets for each 
						block */
extern SET	**killlive;		/* live vars "kill" sets for each 
						block */
extern SET	**inlive;		/* live vars "in" sets for each block */
extern SET	**outlive;		/* live vars "out" sets for each 
						block */
extern SET	*inliveexit;		/* live vars "in" set for EXIT */
extern SET	**blockreach;		/* blocks for each reaching def in
						which def is in "in" set */
extern SET	**blocklive;		/* blocks for each live var in which
						var is in "in" set */
extern REGION	*topregion;		/* top of REGION tree -- whole pgm
						region */
extern REGION 	**num_to_region;	/* regionno to REGION map vector */
extern short	lastregionno;		/* last entry in num_to_region table */
extern REGCLASSLIST regclasslist[];	/* 3 element array of list blocks --
					   1 element each for int, addr, float
					 */
extern struct reg_type_info reg_types[]; /* info about D, A, 881, and FPA regs*/

extern long	maxtempintreg;
extern long	maxtempfloatreg;
extern long	hiddenfargchain;
extern long	exitblockno;
extern flag	pass1_collect_attributes;   /* register.c -- flag to regdefuse.c
						files that attribute collection
						into stmttab is to occur
					     */
extern flag	array_forms_fixed;	/* register.c */
extern void	(*simple_use_proc)();	/* proc called for simple use in
					   regdefuse.c
					 */
extern void	(*simple_def_proc)();	/* proc called for simple def in
					   regdefuse.c
					 */
extern flag	fpa881loopsexist;
extern REGION	*currregiondesc;
extern SET	*fpa881loopblocks;
extern long	block_not_on_main_path;

#define MAX_FARG_SLOTS 25
extern long	max_farg_slots;
extern long	*farg_slots;

extern char olevel;			/* User-specified optimization level */
extern ushort assumptions;		/* Bit mask of C1 assumptions */

/* bit masks locations for optimizing assumptions */
# define NO_PARM_OVERLAPS	(assumptions & 0x8000)
# define PARM_TYPES_MATCHED	(assumptions & 0x4000)
# define NO_EXTERNAL_PARMS	(assumptions & 0x2000)
# define NO_SHARED_COMMON_PARMS	(assumptions & 0x1000)
# define NO_SIDE_EFFECTS	(assumptions & 0x800)
# define NO_HIDDEN_POINTER_ALIASING  (assumptions & 0x200)


extern unsigned topseq;
extern int blockcount;		/* used in loops.c and oglobal.c */

extern int godefcutoff;
extern int goblockcutoff;
extern int goregioncutoff;

# ifdef DEBUGGING
	extern CHILD *max_common_child;
	extern char** first_dublock_bank;
	extern char** curr_dublock_bank;
	extern char* next_dublock;
	extern char* last_dublock;

	extern char* free_dublock_list;
	extern long  max_dublock;
	extern long  size_dublock;
	extern char** first_loadst_bank;
	extern char** curr_loadst_bank;

	extern char* next_loadst;
	extern char* last_loadst;
	extern char* free_loadst_list;
	extern long  max_loadst;
	extern long  size_loadst;

	extern char** first_plink_bank;
	extern char** curr_plink_bank;
	extern char* next_plink;
	extern char* last_plink;
	extern char* free_plink_list;

	extern long  max_plink;
	extern long  size_plink;
	extern char** first_web_bank;
	extern char** curr_web_bank;
	extern char* next_web;

	extern char* last_web;
	extern char* free_web_list;
	extern long  max_web;
	extern long  size_web;
	extern char *initial_sbrk;

	extern flag emitname;
# endif DEBUGGING

/*	masks for unpacking longs */

# ifndef FOP
# 	define FOP(x) (int)((x)&0377)
# endif

# ifndef VAL
# 	define VAL(x) (int)(((x)>>8)&0377)
# endif

# ifndef REST
# 	define REST(x) (((x)>>16)&0177777)
# endif

# define FBUFSIZE 300


# define max(a,b) ( (b) > (a) ? (b) : (a) )
# define SAME_TYPE(t1,t2) (t1.base==t2.base && t1.mods1==t2.mods1 \
				&& t1.mods2==t2.mods2)



/* hash function for terminal (LTYPE) nodes */
# define HASHFT(a)	( ((unsigned)(a)) % maxhashsym )

# define NO_ARRAY	65535
# define NO_STRUCT	65535


# define ASSIGNMENT(o) (asgop(o)) /* more here for ambiguous assign ops */
# define COMMON_OP(op) (!nocommonop(op))


/* bitmask definitions for fwalk2() calls  ... Sensitive to ndu ordering! */

# define COMMON_NODE	0x80000000
# define INVARIANT	0x40000000
# define AREF		0x20000000
# define EREF		0x10000000
# define DAGOBSOLETE	0x8000000
# define TEMPTARGET	0x4000000
# define LHSIREF	0x2000000
# define CSEFOUND	0x1000000
# define CSEREPLACED	0x800000
# define COMMON_BASE	0x400000
# define CALLREF	0x200000
# define IV_CHECKED	0x100000
# define ENTRY		0x80000
# define ARRAYELEM	0x40000
# define ISLOAD		0x20000
# define SREF		0x10000
# define ISPTRINDIR	0x8000
# define ABASEADDR	0x4000
# define FPA881STMT	0x2000
# define ARRAYFORM	0x1000
# define PTMATCHED	0x800
# define COMMA_SS	0x400
# define NOARGUSES	0x200
# define NOARGDEFS	0x100
# define NOCOMUSES	0x80
# define NOCOMDEFS	0x40
# define ISC0TEMP	0x20
# define SAFECALL	0x10
# define CSE_NODE_FREEABLE 0x8			/* DEW - 5/91 - FSDdt07343 */
# define CXM		(AREF|EREF|LHSIREF) /* mask for arrayref,equivref,lhsiref */

# define LITNO	6	/* Max number of iterations in loop_optimization() */

# define PROCLISTSIZE	10
extern char 	**optz_procnames;
extern char	**pass_procnames;
extern long	last_optz_procname;
extern long	last_pass_procname;
extern long	max_optz_procname;
extern long	max_pass_procname;
extern flag	pass_flag;
extern flag	in_procedure;		/* currently in procedure ? */

/* register allocation defines */

#define NREGTYPES 4
#define REGATYPE 0
#define REGDTYPE 1
#define REG8TYPE 2
#define REGFTYPE 3

#define PREHEADNUM 1000000
#define LOADSTORE  65000

/* reg_alloc_second_pass() depends on the relative ordering of the CLASSES */
#define NREGCLASSES 4
#define INTCLASS 0
#define ADDRCLASS 1
#define F881CLASS 2
#define FPACLASS 3

#define DISJOINT 0
#define ONE_IN_TWO 1
#define TWO_IN_ONE 2
#define ONE_IS_TWO 3
#define OVERLAP 4

#define MEMORY 1
#define REGISTER 0
#define DEF 1
#define USE 0
#define LEFT 1
#define RIGHT 0
#define TREE_NOT_PROCESSED 0
#define TREE_PROCESSED 1

#define REG_A_SAVINGS 0
#define REG_D_SAVINGS 1
#define REG_881_FLOAT_SAVINGS 2
#define REG_881_DOUBLE_SAVINGS 3
#define REG_FPA_FLOAT_SAVINGS 4
#define REG_FPA_DOUBLE_SAVINGS 5
#define N_REG_SAVINGS_TYPES 6

/* a vector containing savings for ops */
extern long reg_savings[DSIZE][N_REG_SAVINGS_TYPES];
extern long reg_store_cost[N_REG_SAVINGS_TYPES];
extern long reg_save_restore_cost[N_REG_SAVINGS_TYPES];

#define WHOLE_ARRAY 0
#define ARB_ELEMENT 1
#define CONST_ELEMENT 2
#define VALUE 0
#define REFERENCE 1

#define DEFAULTLOOPITERATIONS 10
#define SAVINGSSHIFT 12
#define MAXINT 0x7fffffff
#define LOOPUNROLLMAXSTMTS 10

#define DUBLOCKS_IN_BANK 100
#define LOADSTS_IN_BANK 100
#define PLINKS_IN_BANK 100
#define WEBS_IN_BANK 100

/* loop types */
#define UNKNOWN 0
#define DO_LOOP 1
#define FOR_LOOP 2
#define DBRA_LOOP 3
#define WHILE_LOOP 4
#define UNTIL_LOOP 5

extern DUBLOCK* alloc_dublock();		/* regweb.c */
extern LOADST* alloc_loadst();			/* regweb.c */
extern PLINK* alloc_plink();			/* regweb.c */
extern WEB *alloc_web();			/* regweb.c */
extern void allocate_regs();			/* regallo.c */
extern void analyze_regions();			/* loopxforms.c */
extern long calculate_savings();		/* regpass1.c */
extern void coalesce_blocks();			/* loopxforms.c */
extern void compute_reaching_defs();		/* register.c */
extern void computed_ptrindir();		/* c1.c */
extern void constant_fold();			/* duopts.c */
extern flag delete_empty_unreached_loops();	/* loopxforms.c */
extern void discard_web();			/* regweb.c */
extern void extract_constant_subtree();		/* c1.c */
extern void pass_procedure();			/* utils.c */
extern void copy_in_to_out();			/* utils.c */

#ifdef DEBUGGING
	extern void lcrdebug();	
	extern void dumpblockpool();
	extern void dumplblpool();
	extern void dump_webloop();
	extern void dump_webs_and_loops();
	extern void dump_work_loop_list();
	extern void dumpdefinetab();
	extern void dumpdublock();
	extern void dumpsym();
	extern void dumpsymtab();
	extern void dumpvarnos();

	extern char *sbrk();
# endif DEBUGGING

extern void file_init_storage();		/* regweb.c */
extern void final_reg_cleanup();
extern void free_dublock();			/* regweb.c */
extern void free_loadst();			/* regweb.c */
extern void free_plink();			/* regweb.c */
extern void free_web();				/* regweb.c */
extern void global_def_use_opts();		/* duopts.c */
extern void init_du_storage();			/* regweb.c */
extern void insert_loads_and_stores();		/* regpass2.c */
extern void make_webs_and_loops();		/* regweb.c */
extern void number_vars();			/* register.c */
extern long offset_from_subtree();		/* regpass2.c */
extern long pop_unreached_block_from_stack();	/* duopts.c */
extern void proc_init_storage();		/* regweb.c */
extern void process_name_icon();		/* c1.c */
extern void process_oreg_foreg();		/* c1.c */
extern void push_unreached_block_on_stack();	/* duopts.c */
extern void reg_alloc_first_pass();		/* regdefuse.c */
extern void reg_alloc_second_pass();		/* regpass2.c */
extern void register_allocation();		/* register.c */
extern void rm_farg_moves_from_prolog();	/* regpass2.c */
extern flag traverse_stmt_1();			/* regpass1.c */
extern void uninitialized_var();		/* regweb.c */

extern void add_arrayelement();			/* symtab.c */
extern void add_common_region();		/* symtab.c */
extern void add_comelement();			/* symtab.c */
extern void add_farg();				/* symtab.c */
extern void add_pointer_target();		/* symtab.c */
extern void add_structelement();		/* symtab.c */
extern void complete_ptrtab();			/* symtab.c */
extern void file_init_symtab();			/* symtab.c */
extern unsigned find();				/* symtab.c */
extern flag locate();				/* symtab.c */
extern HASHU *location();			/* symtab.c */
extern void proc_init_symtab();			/* symtab.c */
extern void symtab_insert();			/* symtab.c */
extern void insert_array_element();		/* symtab.c */

extern SET *loop_ovhd_stmt_set;			/* duopts.c */
extern short revrel[];				/* misc.c */
extern char filename[];				/* c1.c */

	extern struct comment_buf *comment_head;	/* utils.c */
	extern void read_comment();			/* utils.c */
	extern void write_comments();			/* utils.c */

	extern flag saw_dragon_access;			/* register.c */

	extern flag saw_global_access;

	extern flag mc68040;				/* 68040 reg alloc */

#	define reg_A_savings_factor 1	/* Weighting factor to be used in  */
#	define reg_D_savings_factor 1	/* calculating register allocation */
#	define reg_F_savings_factor 3	/* savings.                        */


/* comm_name, comm_equiv_field and com_equiv_check define a data
 * structure used to check for additional equivalence relationships
 * caused by equivalencing items in COMMON.
 */
typedef struct comm_name COMM_NAME;
typedef struct comm_equiv_field COMM_EQUIV_FIELD;

struct comm_name
  {
  char             *name;
  COMM_NAME        *next_name;
  COMM_EQUIV_FIELD  *first_field; 
  };

struct comm_equiv_field
  {
  HASHU            *sp;
  COMM_EQUIV_FIELD *next_field;
  };

extern COMM_NAME  *com_equiv_check;


# ifdef COMPLEXITY
	extern void complexitize();
#	ifndef COUNTING
#		define COUNTING
#	endif COUNTING
# 	define INITIALIZATION 0
# 	define SAMPLING 1
# 	define REPORTING 2
# endif COMPLEXITY

	extern flag allow_insertion;

#ifdef COUNTING
	extern long _n_FPA881_loops;
	extern long _n_procs_passed;
	extern long _n_procs_optzd;
	extern long _n_cbranches_folded;
	extern long _n_int_const_prop;
	extern long _n_real_const_prop;
	extern long _n_unreached_blocks_deleted;
	extern long _n_copy_prop;
	extern long _n_dead_stores_removed;
	extern long _n_do_loops;
	extern long _n_for_loops;
	extern long _n_block_merges;
	extern long _n_empty_do_loops;
	extern long _n_dbra_loops;
	extern long _n_loops_unrolled;
	extern long _n_consts_folded;
	extern long _n_nodes_constant_collapsed;
	extern long _n_comops_rewritten;
	extern long _n_vfes_emitted;
	extern long _n_real_consts_emitted;
	extern long _n_colored_webloops;
	extern long _n_colored_vars;
	extern long _n_subsumed_webloops;
	extern long _n_pruned_register_vars;
	extern long _n_regions;
	extern long _n_blocks_between_blocks;
	extern long _n_loads;
	extern long _n_stores;
	extern long _n_preheaders;
	extern long _n_loadstores_between_blocks;
	extern long _n_loadstores_in_preheaders;
	extern long _n_loadstores_within_blocks;
	extern long _n_farg_translations;
	extern long _n_mem_to_reg_rewrites;
	extern long _n_farg_prolog_moves_removed;
	extern long _n_FPA881_loads;
	extern long _n_FPA881_stores;
	extern long _n_aryelems;
	extern long _n_common_regions;
	extern long _n_comelems;
	extern long _n_fargs;
	extern long _n_ptr_targets;
	extern long _n_structelems;
	extern long _n_symtab_entries;
	extern long _n_symtab_infos;
	extern long _n_vfe_calls_deleted;
	extern long _n_vfe_calls_replaced;
	extern long _n_anon_vfes;
	extern long _n_empty_vfes;
	extern long _n_oneblock_vfes;
	extern long _n_multiblock_vfes;
	extern long _n_vfe_vars;
	extern long _n_overlapping_regions;
	extern long _n_nonreducible_procs;
	extern long _n_uninitialized_vars;
	extern long _n_cses;
	extern long _n_useful_cses;
	extern long _n_code_motions;
	extern long _n_strength_reductions;
	extern long _n_gcps;
	extern long _n_cm_deletenodes;
	extern long _n_cm_minors;
	extern long _n_cm_cmnodes;
	extern long _n_cm_safecalls;
	extern long _n_cm_dis;
	extern long _n_cm_combines;
	extern long _n_minor_aivs;
	extern long _n_constructed_aivs;
	extern long _n_aivs;
	extern long _n_bivs;
	extern flag print_counts;
	extern char *count_file;
#endif /* COUNTING */
