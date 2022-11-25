/* @(#) $Revision: 70.1 $ */    

/* defines for the "stype" field of a symbol 
 * The defines combined in STYPE represent the "basic type". SEXTERN and
 * SALIGN are modifiers.
 */
#define SUNDEF	 0x0000
#define SABS	 0x0001
#define STEXT	 0x0002
#define SDATA	 0x0004
#define SBSS	 0x0008
#define SGNTT	 0x0010
#define SLNTT	 0x0020
#define SSLT	 0x0040		/* not currently used */
#define SVT	 0x0080
#define SXT	 0x0100
#define STYPE	 (SUNDEF|SABS|STEXT|SDATA|SBSS|SGNTT|SLNTT|SSLT|SVT|SXT)
#define SEXTERN	 0x1000
#define SALIGN	 0x2000
#define SEXTERN2 0x4000

/* symbol tag types -- for lookup matches */
#define USRNAME	0
#define MNEMON	1
#define PSEUDO	2
#define REGSYM	3
#define	SPSYM	4

/* Defines for control register symbols -- regno field */
/* The value here is set to agree with MOVEC encoding  */
# define SFCREG		0x000
# define DFCREG		0x001
# define CACRREG	0x002
# define USPREG		0x800
# define VBRREG		0x801
# define CAARREG	0x802
# define MSPREG		0x803
# define ISPREG		0x804
# ifdef OFORTY
# define ITT0REG	0x004
# define ITT1REG	0x005
# define DTT0REG	0x006
# define DTT1REG	0x007
# define MMUSRREG	0x805
# define URPREG		0x806
# define SRPREG		0x807
# endif

/* 68881 control registers
 * The value here is set to agree with fmovm, fmov encoding 
 */
# define FPCONTROL	0x4
# define FPSTATUS	0x2
# define FPIADDR	0x1

/* DRAGON control registers 
 * The value here is set to agree with the fpmov offsets
 */
# define DRGSTATUS	4
# define DRGCONTROL	8

/* 68851 control registers
 * *** NOTE: The values here are simple enumeration types.  An additional
 *		data array is used to give the values that will be set
 *		for "preg" and "num".
 */
# define PMCRP		0
# define PMSRP		1
# define PMDRP		2
# define PMTC		3
# define PMAC		4
# define PMPSR		5
# define PMPCSR		6
# define PMCAL		7
# define PMVAL		8
# define PMSCC		9
# define PMBAC0		10
# define PMBAC1		11
# define PMBAC2		12
# define PMBAC3		13
# define PMBAC4		14
# define PMBAC5		15
# define PMBAC6		16
# define PMBAC7		17
# define PMBAD0		18
# define PMBAD1		19
# define PMBAD2		20
# define PMBAD3		21
# define PMBAD4		22
# define PMBAD5		23
# define PMBAD6		24
# define PMBAD7		25
 

/* flags for lookup routine -- whether to install the name if not found. */
# define INSTALL	1
# define N_INSTALL	0


/* Symbol table structure definitions.
 *
 * Because "symbol" and "instr" structures are linked onto the same
 * hash lists, and searched via "lookup" the namep, hashlink,
 * and tag fields must overlap.
 */

typedef union USYMINS usymins;
typedef union UPSYMINS upsymins;
typedef struct HASHED_SYMBOL hashed_symbol;
typedef struct REGSYMBOL regsymbol;
typedef struct SYMBOL symbol;
typedef struct INSTR instr;

union  UPSYMINS
	{
		symbol	*stp;
		instr	*itp;
		regsymbol	*rsp;
		hashed_symbol	*hsp;
	};

/* Register symbol */
struct REGSYMBOL
	{
		char *	rnamep;
		symbol * hashlink;
		short	rtag;	/* always REGSYM */
		short	rtype;
		short	regno;
	};

/* User symbol */
struct SYMBOL
	{
		char *	snamep;
		symbol * hashlink;
		short	stag;
		short	stype;
		long	svalue;
		symbol * slink;
		long	slstindex;
		long	salmod;		/* for alignment symbols */
		struct sdi * ssdiinfo;
                unsigned char  got;
                unsigned char  plt;
                unsigned char  pc;
                unsigned char  ext;
                unsigned char  endoflist;
                unsigned char  internal;
		long           size;
	};


/* Instruction mneumonic symbol */
struct INSTR
	{
		char *	inamep;
		instr * hashlink;
		short	itag;		/* the symbol class. (MNEMONIC) */
		short	iopclass;	/* returned by scanner */
		short	ivalue;		/* op-code number 	*/
		short	imatch;		/* opnumber for template matching:
					 * allows for "generic" templates for
					 * things like all the I_Bcc.
					 */
	};


struct HASHED_SYMBOL
	{
		char * 	namep;
		upsymins  hashlink;
		short	tag;
	};


union USYMINS
	{
		symbol  usersym;
		instr	instrsym;
		regsymbol  regsym;
		hashed_symbol hsym;
	};

/*
 *	NSHASH	= the size of the instruction mneumonics hash table
 *	NIHASH	= the size of the user name hash table
 */
# define NSHASH	 997
# define NIHASH	 997

typedef struct
	{
		long htsize;
		upsymins * htchain;
	}  hashtable;


/* Space for user symbols is allocated in blocks of NSYMS_PER_BLK.
 * When one block overflows, malloc is used to allocate another.
 * These blocks are chained together for the fixup symbol routines
 * that must be able to traverse the entire symbol table.
 */
# define NSYMS_PER_BLK	4000

struct usersym_blk {
  struct usersym_blk * next_usersym_blk;
  long symblkcnt;
  symbol usersym[NSYMS_PER_BLK];
  };


