/* $Revision: 70.2 $ */
/*
Copyright (c) 1984, 19885, 1986, 1987 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#include <regex.h>
#if defined NLS || defined NLS16
#include <locale.h>
#include <nl_types.h>
#include <nl_ctype.h>
#define Strcmp(s1,s2)	strcoll(s1,s2)	/* language sensitive string compare */
extern char *catgets();
#else
#define _CS_SBYTE	1
#define CHARAT(p)	((*(p)&0377))
#define PCHARADV(c, p)	*(p)++ = c
#define ADVANCE(p)	(p)++
#define getC(f)		getc(f)
#define FIRSTof2(c)	0
#define Strcmp( s1, s2)	strcmp( s1, s2)
#define catgets( i, sn, mn, s)	(s)
#endif

#define NL_SETN	1


typedef double	Awkfloat;
typedef	unsigned char uchar;

#define	xfree(a)	{ if ((a) != NULL) { free(a); a = NULL; } }

#define DEBUG
#ifdef	DEBUG
#	define	dprintf	if(dbg)printf
#else
#	define	dprintf(x1, x2, x3, x4)
#endif

extern	char	errbuf[200];
extern error();
#define VERYFATAL	0	/* exit without showing context */
#define	FATAL		1	/* exit */
#define	WARNING		2
#define	SYNTAX		3	/* call yyerror() */

extern int	compile_time;	/* 1 if compiling, 0 if running */

#if defined LINE_MAX
#define RECSIZE LINE_MAX	/* POSIX 1003.2 buffer size */
#else
#define	RECSIZE	(3 * 1024)	/* sets limit on records, fields, etc., etc. */
#endif

extern uchar	**FS;
extern uchar	**RecordSep;
extern uchar	**ORS;
extern uchar	**OFS;
extern uchar	**OFMT;
extern uchar	**CONVFMT;
extern Awkfloat *NR;
extern Awkfloat *FNR;
extern Awkfloat *NF;
extern uchar	**FILENAME;
extern uchar	**SUBSEP;
extern Awkfloat *RSTART;
extern Awkfloat *RLENGTH;

extern uchar	*record;
extern int	dbg;
extern int	lineno;
extern int	errorflag;
extern int	donefld;	/* 1 if record broken into fields */
extern int	donerec;	/* 1 if record is valid (no fld has changed */

#define	CBUFLEN	400
extern uchar	cbuf[CBUFLEN];	/* miscellaneous character collection */

extern	uchar	*patbeg;	/* beginning of pattern matched */
extern	int	patlen;		/* length.  set in b.c */

/* Cell:  all information about a variable or constant */

typedef struct Cell {
	uchar	ctype;		/* OCELL, OBOOL, OJUMP, etc. */
	uchar	csub;		/* CCON, CTEMP, CFLD, etc. */
	uchar	*nval;		/* name, for variables only */
	uchar	*sval;		/* string value */
	Awkfloat fval;		/* value as number */
	unsigned tval;		/* type info: STR|NUM|ARR|FCN|FLD|CON|DONTFREE */
	struct Cell *cnext;	/* ptr to next if chained */
	int	stamp;		/* timestamp for OFMT/CONVFMT */
} Cell;

typedef struct {		/* symbol table array */
	int	nelem;		/* elements in table right now */
	int	size;		/* size of tab */
	Cell	**tab;		/* hash table pointers */
} Array;

#define	NSYMTAB	50	/* initial size of a symbol table */
extern Array	*symtab, *makesymtab();
extern Cell	*setsymtab(), *lookup();

extern Cell	*recloc;	/* location of input record */
extern Cell	*nrloc;		/* NR */
extern Cell	*fnrloc;	/* FNR */
extern Cell	*nfloc;		/* NF */
extern Cell	*rstartloc;	/* RSTART */
extern Cell	*rlengthloc;	/* RLENGTH */

extern int ofmt_stamp;
extern int convfmt_stamp;

/* Cell.tval values: */
#define	NUM	01	/* number value is valid */
#define	STR	02	/* string value is valid */
#define DONTFREE 04	/* string space is not freeable */
#define	CON	010	/* this is a constant */
#define	ARR	020	/* this is an array */
#define	FCN	040	/* this is a function name */
#define FLD	0100	/* this is a field $1, $2, ... */
#define	REC	0200	/* this is $0 */
#define NUMCON	0400	/* numeric constant */
#define STRCON	01000	/* string constant */

#define freeable(p)	(!((p)->tval & DONTFREE))

Awkfloat setfval(), getfval();
uchar	*setsval(), *getsval();
uchar	*tostring(), *tokname(), *malloc(), *calloc(), *qstring();
double	log(), sqrt(), exp(), atof();

/* function types */
#define	FLENGTH	1
#define	FSQRT	2
#define	FEXP	3
#define	FLOG	4
#define	FINT	5
#define	FSYSTEM	6
#define	FRAND	7
#define	FSRAND	8
#define	FSIN	9
#define	FCOS	10
#define	FATAN	11
#define	FTOUPPER 12
#define	FTOLOWER 13

/* Node:  parse tree is made of nodes, with Cell's at bottom */

typedef struct Node {
	int	ntype;
	struct	Node *nnext;
	int	lineno;
	int	nobj;
	struct Node *narg[1];	/* variable: actual size set by calling malloc */
} Node;

#define	NIL	((Node *) 0)

extern Node	*winner;
extern Node	*nullstat;
extern Node	*nullnode;

/* ctypes */
#define OCELL	1
#define OBOOL	2
#define OJUMP	3

/* Cell subtypes: csub */
#define CFREE	7
#define CCOPY	6
#define CCON	5
#define CTEMP	4
#define CNAME	3 
#define CVAR	2
#define CFLD	1

/* bool subtypes */
#define BTRUE	11
#define BFALSE	12

/* jump subtypes */
#define JEXIT	21
#define JNEXT	22
#define	JBREAK	23
#define	JCONT	24
#define	JRET	25

/* node types */
#define NVALUE	1
#define NSTAT	2
#define NEXPR	3
#define	NFIELD	4

extern	Cell	*(*proctab[])();
extern	Cell	*nullproc();
extern	Cell	*fieldadr();

extern	Node	*stat1(), *stat2(), *stat3(), *stat4(), *pa2stat();
extern	Node	*op1(), *op2(), *op3(), *op4();
extern	Node	*linkum(), *valtonode(), *rectonode(), *exptostat();
extern	Node	*makearr();

#define notlegal(n)	(n <= FIRSTTOKEN || n >= LASTTOKEN || proctab[n-FIRSTTOKEN] == nullproc)
#define isvalue(n)	((n)->ntype == NVALUE)
#define isexpr(n)	((n)->ntype == NEXPR)
#define isjump(n)	((n)->ctype == OJUMP)
#define isexit(n)	((n)->csub == JEXIT)
#define	isbreak(n)	((n)->csub == JBREAK)
#define	iscont(n)	((n)->csub == JCONT)
#define	isnext(n)	((n)->csub == JNEXT)
#define	isret(n)	((n)->csub == JRET)
#define isstr(n)	((n)->tval & STR)
#define isnum(n)	((n)->tval & NUM)
#define isarr(n)	((n)->tval & ARR)
#define isfunc(n)	((n)->tval & FCN)
#define istrue(n)	((n)->csub == BTRUE)
#define istemp(n)	((n)->csub == CTEMP)

typedef struct fa {
	uchar	*restr;	/* original text of regular expression */
	int	use;	/* frequency of use flag */
	regex_t	re;	/* compiled ERE */
} fa;

extern	fa	*makedfa();

/* error() error numbers */

#define ERR01	1
#define ERR02	2
#define ERR03	3
#define ERR04	4
#define ERR05	5
#define ERR06	6
#define ERR07	7
#define ERR08	8
#define ERR09	9
#define ERR10	10
#define ERR11	11
#define ERR12	12
#define ERR13	13
#define ERR14	14
#define ERR15	15
#define ERR16	16
#define ERR17	17
#define ERR18	18
#define ERR19	19
#define ERR20	20
#define ERR21	21
#define ERR22	22
#define ERR23	23
#define ERR24	24
#define ERR25	25
#define ERR26	26
#define ERR27	27
#define ERR28	28
#define ERR29	29
#define ERR30	30
#define ERR31	31
#define ERR32	32
#define ERR33	33
#define ERR34	34
#define ERR35	35
#define ERR36	36
#define ERR37	37
#define ERR38	38
#define ERR39	39
#define ERR40	40
#define ERR41	41
#define ERR42	42
#define ERR43	43
#define ERR44	44
#define ERR45	45
#define ERR46	46
#define ERR47	47
#define ERR48	48
#define ERR49	49
#define ERR50	50
#define ERR51	51
#define ERR52	52
#define ERR53	53
#define ERR54	54
#define ERR55	55
#define ERR56	56
#define ERR57	57
#define ERR58	58
#define ERR59	59
#define ERR60	60
#define ERR61	61
#define ERR62	62
#define ERR63	63
#define ERR64	64
#define ERR65	65
#define ERR66	66
#define ERR67	67
#define ERR68	68
#define ERR69	69
#define ERR70	70
#define ERR71	71
#define ERR72	72
#define ERR73	73
#define ERR74	74
#define ERR75	75
#define ERR76	76
#define ERR77	77
#define ERR78	78
#define ERR79	79
#define ERR80	80
#define ERR81	81
#define ERR82	82
#define ERR83	83
#define ERR84	84
#define ERR85	85
#define ERR86	86
#define ERR87	87
#define ERR88	88
#define ERR89	89
#define ERR90	90
#define ERR91	91
#define ERR92	92
#define ERR93	93
#define ERR94	94
#define ERR95	95
#define ERR96	96
#define ERR97	97
#define ERR98	98
#define ERR99	99
#define ERR100	100

#define ERRREC	91
#define ERRFILE	92
#define ERRLINE	93
#define	ERRCNTX	94
#define	YYERR1	95
#define YYERR2	96
#define RERR101	101
#define RERR102	102
#define RERR103	103
#define RERR104	104
#define RERR105	105
#define RERR106	106
#define RERR107	107
#define RERR108	108
#define RERR109	109
#define RERR110	110
#define RERR111	111
#define RERR112	112
#define RERR113	113
#define RERR114	114
#define RERR115	115
#define RERR116	116
#define RERR117	117
#define RERR118	118
#define ERR119	119
#define ERR120	120
#define ERR121	121
#define ERR122	122
#define ERR123	123
#define ERR124	124
#define ERR125	125
#define ERR126	126
#define ERR127	127
#define ERR128	128
#define ERR129	129
#define ERR130	130
#define ERR131	131
