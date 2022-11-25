/* @(#) $Revision: 70.1 $ */    

typedef unsigned char BYTE;

# ifdef M68851
# define MAXOPERAND  4
# else
# define MAXOPERAND  3
# endif

/*
 * operand syntax types
 */

# define A_DREG		0
# define A_AREG		1
# define A_ABASE	2
# define A_AINCR	3
# define A_ADECR	4
# define A_ADISP16	5
# define A_AINDXD8	6
# define A_AINDXBD	7
# define A_AMEM		8
# define A_AMEMX	9
# define A_PCDISP16	10
# define A_PCINDXD8	11
# define A_PCINDXBD	12
# define A_PCMEM	13
# define A_PCMEMX	14
# define A_ABS16	15
# define A_ABS32	16
# define A_IMM		17

/* mark end of modes that should appear in <ea>'s by pass2 */
# define A_MAX_EAMODE	17

/* additional modes used by the yacc grammar, and verification pattern
 * tables.   
 */
# define A_CCREG	18
# define A_SRREG	19
# define A_CREG		20
# define A_DREGPAIR	21
# define A_REGPAIR	22
# define A_REGLIST	23
# define A_CACHELIST	24
/*# define A_LABEL	25*/
/* for A_PMUREG use the slot currently unused by A_LABEL */
# define A_PMUREG	25
# define A_FPDREG	26
# define A_FPCREG	27
# define A_BFSPECIFIER	28
# define A_FPDREGPAIR	29
# define A_FPDREGLIST	30
# define A_FPCREGLIST	31
# define A_FPPKSPECIFIER 32


/* Following are additional modes and "generic" forms used by the yacc grammar
 * rules but not directly matching a machine addressing mode.
 * Defines here should not be used in building verification table patterns.
 */
# define A_ABS		33
# define A_ADISP	34
# define A_PCDISP	35
# define A_AINDX	36
# define A_PCINDX	37
# define A_SOPERAND	38

/* DRAGON: data and control register modes */
# define A_DRGDREG	41
# define A_DRGCREG	42

/*
 * expression data types
 */
/* Integral types first, including TYPNULL, TSYM, and TDIFF.
 * These should fit in first 8 bits, else the
 * fixup recs need to allow a short field (currently a char).
 */
#define TYPNULL	0x0001	/* no expression */
#define TYPB	0x0002	/* byte */
#define TYPW	0x0004	/* word */
#define TYPL	0x0008	/* long */

#define TSYM	0x0010	/* symbol + (-)offset which is in expval */
#define TDIFF	0x0020	/* symbol - symbol expression, the ptr to the left */
			/* symbol is stored in expval.lngval and the symptr
			   contains the right symbol */

#define TYPF	0x0100	/* floating */
#define TYPD	0x0200	/* double floating */
#define TYPP	0x0400	/* packed float */
#define TYPX	0x0800	/* extended float */

#define TINT	(TYPB|TYPW|TYPL)
/*#define TFLT	(TYPF|TYPD)*/
#define TFLT	(TYPF|TYPD|TYPP|TYPX)

#define TANY	0x0fff

/* index mode scale factor */
# define XSF_NULL  0

/* defines for expression and instruction sizes */
# define SZNULL		0
# define SZBYTE		1
# define SZWORD		2
# define SZLONG		3
# define SZSINGLE	4
# define SZDOUBLE	5
# define SZEXTEND	6
# define SZPACKED	7
# define SZALIGN	8	/* just for reloc records for align */

/* structures used to build operands */
typedef	union {
		long	lngval;
		unsigned short shtval;
		unsigned char  bytval;
		float	fltval;	
		double	dblval;
		struct { long msw; long lsw; } lngpair;
		struct { unsigned char bytes[12]; } pckval;
		struct { unsigned char bytes[12]; } extval;
	      } expvalue;

typedef struct {
	short	exptype;
	short	expsize;
	symbol	*symptr;
	expvalue	expval;
} rexpr;

typedef struct {
	short regkind;
	short regno;
} reg;

typedef struct {
	reg  xrreg;
	BYTE xrscale;
	BYTE xrsize;
} xreg;

typedef struct {
	BYTE admode;
	BYTE adsize;	/* for IMM.  ?? could access operation-size instead */
	reg  adreg1;
	xreg  adxreg;
	rexpr adexpr1;
	rexpr adexpr2;
} addrmode;

typedef struct {
	reg rpreg1;
	reg rpreg2;
} register_pair;

typedef struct {
	short  opadtype;
	union {
	   addrmode soperand;
	   unsigned long reglist;
	   register_pair regpair;
	} unoperand;
} operand_addr;

typedef struct {
	BYTE admode;
	BYTE adsize;	/* for IMM.  ?? could access instruction-size instead */
	reg  adreg1;
	xreg  adxreg;
	rexpr *adexptr1;
	rexpr *adexptr2;
} addrmode1;

typedef struct {
	short  opadtype;
	union {
	   addrmode1 *soperand;
	   unsigned long reglist;
	   register_pair *regpair;
	} unoperand;
} operand_addr1;

/* define a union for crude space allocation */
typedef union {
	expvalue	ndexpval;
	rexpr	ndrexpr;
	reg	ndreg;
	xreg	ndxreg;
	addrmode1	ndaddrmode;
	register_pair	ndregpair;
	operand_addr1	ndoperand;
	} node;


/* macro for allocating nodes while building parsing structures */
# ifdef BBA
# define ALLOCATE_NODE(type)   (nodes[inode]=zero_node, (type*)&nodes[inode++])
# else
# define ALLOCATE_NODE(type)  (inode<maxnode ? (nodes[inode]=zero_node, \
	(type*)&nodes[inode++]): (type*)aerror("node array overflow"))
# endif
