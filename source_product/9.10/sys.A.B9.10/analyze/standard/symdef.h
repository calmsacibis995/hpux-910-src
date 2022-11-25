

/* Series 300 symbol table and stack trace info */

#define VARB	11
#define VARD	13
#define VARE	14
#define VARF	15
#define VARM	22
#define VARR	27
#define VARS	28
#define VART	29

#define COREMAGIC 0140000

#define RD	1
#define WT	4
#define NSP	0
#define	ISP	1
#define	DSP	2
#define STAR	4
#define STARCOM 0200
#define DSYM	7
#define ISYM	2
#define ASYM	1
#define NSYM	0
#define ESYM	(-1)
#define NCHNAM  SYMLENGTH	/* max length of a symbol MFM */
#define MAXHDIGITS 8	/* max length of a hex number address MFM */


#ifdef KERNEL_TUNE_DLB
#define PS	SP+1
#define PC	SP+2
#endif KERNEL_TUNE_DLB

#ifndef	MANX
#define a6	14
#define sp	15
#define pc	16
#define ps	17		/* register offset definitions (local)	*/
#else
#define a6	AR6
#define sp	SP
#define ps	PS
#define pc	PC
#define fs	FSTATUS
#define fe	FERRBIT		/* register offset definitions (local)	*/
#endif



#define leng(a)		((long)((unsigned)(a)))
#define shorten(a)	((int)(a))

#define itol68(a,b)	((a << 16) | (b & 0xFFFF))


#define TYPE	typedef			/* algol-like statement definitions */
#define STRUCT	struct
#define UNION	union

#define BEGIN	{
#define END	}

#define IF	if(
#define THEN	){
#define ELSE	} else {
#define ELIF	} else if (
#define FI	}

#define FOR	for(
#define WHILE	while(
#define DO	){
#define OD	}
#define REP	do{
#define PER	}while(
#ifdef hp9000s300
#if defined(DONE)
#undef DONE
#endif /* defined */
#endif /* hp9000s300 */
#define DONE	);
#define LOOP	for(;;){
#define POOL	}


typedef	struct symb	SYMTAB;
typedef struct symb	*SYMPTR;
typedef	struct exec	BHDR;

/* this stuff is extracted from a.out.h, and should really
   be done with a #include */



struct symb	/* this struct parallels nlist_ in a.out.h */
{
	long	vals;			/* symbol value */
	unsigned char	symf;		/* symbol type */
	unsigned char   slength;	/* symbol length (no \0) MFM */
	int	smtp;			/* SYMTYPE */
	char	*symc;			/* pointer to symbol name */
};

#define SYMCHK 047
#define SYMTYPE(st)	((st>=041 || (st>=02 && st<=04))\
			? ((st&07)>=3 ? DSYM : (st&07)) : NSYM)
#define MAXCOM	64
#define MAXARG	32
#define LINSIZ	256
TYPE	int		INT;
TYPE	long int	L_INT;
TYPE	float		REAL;
TYPE	double		L_REAL;
TYPE	unsigned	POS;
TYPE	char		BOOL;
TYPE	char		CHAR;
TYPE	char		*STRING;
TYPE	char		MSG[];
TYPE	struct addrmap	MAP;
TYPE	MAP		*MAPPTR;
TYPE	struct bkpt	BKPT;
TYPE	BKPT		*BKPTR;


/*
/* file address maps */
struct addrmap {
	L_INT	b1;	/* entry point */
	L_INT	e1;	/* entry point + (0407 ? sizeof(data)+sizeof(text) :
						 sizeof(text) */
	L_INT	f1;	/* a.out header size */
	L_INT	b2;	/* data area base */
	L_INT	e2;
	L_INT	f2;
	INT	ufd;
};

struct bkpt {
	INT	loc;
	INT	ins;
	INT	count;
	INT	initcnt;
	INT	flag;
	CHAR	comm[MAXCOM];
	BKPT	*nxtbkpt;
};

TYPE	struct reglist	REGLIST;

TYPE	REGLIST		*REGPTR;
struct reglist {
	STRING	rname;
	INT	roffs;
	L_INT	rval;
};

struct {
	INT	junk[2];
	INT	fpsr;
	REAL	Sfr[6];
};

struct {
	INT	junk[2];
	INT	fpsr;
	L_REAL	Lfr[6];
};
