static char *HPUX_ID = "@(#) $Revision: 30.9 $";
#include <stdio.h>
#include <string.h>

#define MAXLINE	132	/* maximum line length */
#define EOL	10	/* end of line */

#define WS	1	/* white space */
#define	ID	2	/* identifier character */
#define IC	4	/* identifier character to change */
#define UC	8	/* uppercase */
#define	UP	16	/* (,{,[ */
#define DN	32	/* ),},] */
#define DG	64	/* digit */
#define EX	128	/* +,-,.,* */
#define AS	256	/* valid char to follow + or - and not be expression */
#define	UO	512	/* unary operator */
#define	BO	1024	/* binary operator */
#define SF	2048	/* 1,2,4,8 */

#define	OC	1	/* change opcode mnemonic */
#define NA	2	/* no arguments */
#define CM	4	/* comment out */
#define MT	8	/* unsupported opcode requiring manual translation */
#define SW	16	/* swap operands */
#define SP	32	/* statement requires special handling */
#define WR	64	/* comment out and give warning message */
#define SQ	128	/* dc opcode */
#define CC	256	/* '.w' may be required at end of opcode */

#define	BADIDCHAR 1	/* bad character in identifier */
#define MAN_TRANS 2	/* instruction requires manual translation */
#define LABEL	4	/* line has label */
#define SYNTAX	8	/* syntax error */
#define	WARNING	16	/* issue warning */
#define PCREL	32	/* PC addressing used */

#define NEXTFIELD(in,out,ch) while (charset[ch = *in++] & WS) *out++ = ch

static struct nodetype {
	char *ident,left,right,prec,token;
} node[MAXLINE];

FILE *infile;
static short globalflg,quoteflg,nextnode,nflag;
static char inline[MAXLINE+2],outline[2*MAXLINE+2],chartemp[MAXLINE],
	    namepool[2 * MAXLINE],*npoolptr;

extern int fprintf(),exit(),fputs(),fclose();

static char *longlinemsg = "Warning on line %d: truncated to %d characters\n";
static char *openmsg = "Cannot open %s for input\n";
static char *usagemsg = "Usage: atrans [-n] [file]\n";
static char *badoptmsg = "Unrecognized option\n";
static char *badcharmsg = "Warning on line %d: character in identifier not allowed\n";
static char *syntaxmsg = "Error on line %d: syntax error prevents translation\n";
static char *mtmsg = "Warning on line %d: will cause assembler error\n";
static char *warnmsg = "Warning on line %d: commented out - not supported with assembler\n";
static char *pcmsg = "Warning on line %d: PC relative addressing used - check offset\n";

static short charset[128] = {
/*   0 */	0,	0,	0,	0,	0,	0,	0,	0,
/*   8 */	0,	AS|WS,	AS,	AS|WS,	0,	0,	0,	0,
/*  16 */	0,	0,	0,	0,	0,	0,	0,	0,
/*  24 */	0,	0,	0,	0,	0,	0,	0,	0,
/*  32 */	AS|WS,	BO,	0,	0,	ID|IC,	0,	BO,	0,
/*  40 */	UP|AS,	DN,	EX,	EX|UO|BO, AS,	EX|UO|BO, EX,	0,
/*  48 */	ID|DG,	ID|DG|SF,ID|DG|SF,ID|DG,ID|DG|SF,ID|DG,ID|DG,ID|DG,
/*  56 */	ID|DG|SF,ID|DG,	0,	0,	0,	0,	0,	0,
/*  64 */	ID|IC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,
/*  72 */	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,
/*  80 */	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,	ID|UC,
/*  88 */	ID|UC,	ID|UC,	ID|UC,	UP,	0,	DN,	0,	ID,
/*  96 */	0,	ID,	ID,	ID,	ID,	ID,	ID,	ID,
/* 104 */	ID,	ID,	ID,	ID,	ID,	ID,	ID,	ID,
/* 112 */	ID,	ID,	ID,	ID,	ID,	ID,	ID,	ID,
/* 120 */	ID,	ID,	ID,	UP,	0,	DN,	0,	0
};

static short tableptr[21] = {
	0, 1, 35, 59, 78, 80, -307, -307, 307, -309, -309, 309, 313,
	314, 318, 320, -321, 321, 330, 337, -393 
};

static struct {
	char *name;
	short action;
	unsigned char p1,p2;
} table[393] = {
	"",		0,	0,	0,
	"bcc.s",	OC,	1,	0,
	"bcs.s",	OC,	5,	0,
	"beq.s",	OC,	8,	0,
	"bfchg",	SP,	0,	0,
	"bfclr",	SP,	0,	0,
	"bfexts",	SP,	0,	0,
	"bfextu",	SP,	0,	0,
	"bfffo",	SP,	0,	0,
	"bfins",	SP,	0,	0,
	"bfset",	SP,	0,	0,
	"bftst",	SP,	0,	0,
	"bge.s",	OC,	9,	0,
	"bgt.s",	OC,	10,	0,
	"bhi.s",	OC,	11,	0,
	"bhs",		OC,	0,	0,
	"bhs.b",	OC,	1,	0,
	"bhs.l",	OC,	2,	0,
	"bhs.s",	OC,	1,	0,
	"bhs.w",	OC,	3,	0,
	"ble.s",	OC,	12,	0,
	"blo",		OC,	4,	0,
	"blo.b",	OC,	5,	0,
	"blo.l",	OC,	6,	0,
	"blo.s",	OC,	5,	0,
	"blo.w",	OC,	7,	0,
	"bls.s",	OC,	13,	0,
	"blt.s",	OC,	14,	0,
	"bmi.s",	OC,	15,	0,
	"bne.s",	OC,	16,	0,
	"bpl.s",	OC,	17,	0,
	"bra.s",	OC,	18,	0,
	"bsr.s",	OC,	19,	0,
	"bvc.s",	OC,	20,	0,
	"bvs.s",	OC,	21,	0,
	"cas2",		SP,	0,	1,
	"cas2.b",	SP,	0,	1,
	"cas2.l",	SP,	0,	1,
	"cas2.w",	SP,	0,	1,
	"cmp",		SW,	0,	0,
	"cmp.b",	SW,	0,	0,
	"cmp.l",	SW,	0,	0,
	"cmp.w",	SW,	0,	0,
	"cmp2",		SW,	0,	0,
	"cmp2.b",	SW,	0,	0,
	"cmp2.l",	SW,	0,	0,
	"cmp2.w",	SW,	0,	0,
	"cmpa",		SW,	0,	0,
	"cmpa.l",	SW,	0,	0,
	"cmpa.w",	SW,	0,	0,
	"cmpi",		SW,	0,	0,
	"cmpi.b",	SW,	0,	0,
	"cmpi.l",	SW,	0,	0,
	"cmpi.w",	SW,	0,	0,
	"cmpm",		SW,	0,	0,
	"cmpm.b",	SW,	0,	0,
	"cmpm.l",	SW,	0,	0,
	"cmpm.w",	SW,	0,	0,
	"com",		MT,	0,	0,
	"dbhs",		OC,	23,	0,
	"dblo",		OC,	24,	0,
	"dc",		OC|SQ,	40,	2,
	"dc.b",		OC|SQ,	22,	1,
	"dc.d",		OC,	25,	0,
	"dc.l",		OC|SQ,	36,	4,
	"dc.w",		OC|SQ,	40,	2,
	"decimal",	CM,	0,	0,
	"def",		OC,	35,	0,
	"divsl.l",	OC,	227,	0,
	"divul.l",	OC,	228,	0,
	"ds",		OC|SP,	41,	2,
	"ds.b",		OC,	41,	0,
	"ds.d",		OC|SP,	41,	3,
	"ds.l",		OC|SP,	41,	4,
	"ds.p",		OC|SP,	41,	5,
	"ds.s",		OC|SP,	41,	4,
	"ds.w",		OC|SP,	41,	2,
	"ds.x",		OC|SP,	41,	5,
	"end",		CM,	0,	0,
	"equ",		OC|SP,	39,	6,
	"fabs",		CC,	0,	0,
	"facos",	CC,	0,	0,
	"fadd",		CC,	0,	0,
	"fasin",	CC,	0,	0,
	"fatan",	CC,	0,	0,
	"fatanh",	CC,	0,	0,
	"fcmp",		SW|CC,	0,	0,
	"fcmp.b",	SW,	0,	0,
	"fcmp.d",	SW,	0,	0,
	"fcmp.l",	SW,	0,	0,
	"fcmp.p",	SW,	0,	0,
	"fcmp.s",	SW,	0,	0,
	"fcmp.w",	SW,	0,	0,
	"fcmp.x",	SW,	0,	0,
	"fcos",		CC,	0,	0,
	"fcosh",	CC,	0,	0,
	"fdbeq.w",	OC,	90,	0,
	"fdbf.w",	OC,	26,	0,
	"fdbge.w",	OC,	91,	0,
	"fdbgl.w",	OC,	92,	0,
	"fdbgle.w",	OC,	93,	0,
	"fdbgt.w",	OC,	94,	0,
	"fdble.w",	OC,	95,	0,
	"fdblt.w",	OC,	96,	0,
	"fdbneq.w",	OC,	97,	0,
	"fdbnge.w",	OC,	98,	0,
	"fdbngl.w",	OC,	99,	0,
	"fdbngle.w",	OC,	100,	0,
	"fdbngt.w",	OC,	101,	0,
	"fdbnle.w",	OC,	102,	0,
	"fdbnlt.w",	OC,	103,	0,
	"fdboge.w",	OC,	104,	0,
	"fdbogl.w",	OC,	105,	0,
	"fdbogt.w",	OC,	106,	0,
	"fdbole.w",	OC,	107,	0,
	"fdbolt.w",	OC,	108,	0,
	"fdbor.w",	OC,	109,	0,
	"fdbra",	OC,	26,	0,
	"fdbra.w",	OC,	26,	0,
	"fdbseq.w",	OC,	110,	0,
	"fdbsf.w",	OC,	111,	0,
	"fdbsneq.w",	OC,	112,	0,
	"fdbst.w",	OC,	113,	0,
	"fdbt.w",	OC,	114,	0,
	"fdbueq.w",	OC,	115,	0,
	"fdbuge.w",	OC,	116,	0,
	"fdbugt.w",	OC,	117,	0,
	"fdbule.w",	OC,	118,	0,
	"fdbult.w",	OC,	119,	0,
	"fdbun.w",	OC,	120,	0,
	"fdiv",		CC,	0,	0,
	"fetox",	CC,	0,	0,
	"fetoxm1",	CC,	0,	0,
	"fgetexp",	CC,	0,	0,
	"fgetman",	CC,	0,	0,
	"fint",		CC,	0,	0,
	"fintrz",	CC,	0,	0,
	"flog10",	CC,	0,	0,
	"flog2",	CC,	0,	0,
	"flogn",	CC,	0,	0,
	"flognp1",	CC,	0,	0,
	"fmod",		CC,	0,	0,
	"fmove",	OC|CC,	27,	1,
	"fmove.b",	OC,	28,	0,
	"fmove.d",	OC,	29,	0,
	"fmove.l",	OC,	30,	0,
	"fmove.p",	OC,	31,	0,
	"fmove.s",	OC,	32,	0,
	"fmove.w",	OC,	33,	0,
	"fmove.x",	OC,	34,	0,
	"fmovecr",	OC,	122,	0,
	"fmovecr.x",	OC,	122,	0,
	"fmovem",	OC,	121,	0,
	"fmovem.l",	OC,	121,	0,
	"fmovem.x",	OC,	121,	0,
	"fmul",		CC,	0,	0,
	"fneg",		CC,	0,	0,
	"fnop",		NA,	0,	0,
	"frem",		CC,	0,	0,
	"fscale",	CC,	0,	0,
	"fsgldiv",	CC,	0,	0,
	"fsglmul",	CC,	0,	0,
	"fsin",		CC,	0,	0,
	"fsincos",	CC,	0,	0,
	"fsinh",	CC,	0,	0,
	"fsqrt",	CC,	0,	0,
	"fsub",		CC,	0,	0,
	"ftan",		CC,	0,	0,
	"ftanh",	CC,	0,	0,
	"ftentox",	CC,	0,	0,
	"fteq",		NA,	0,	0,
	"ftest",	CC,	0,	0,
	"ftf",		NA,	0,	0,
	"ftge",		NA,	0,	0,
	"ftgl",		NA,	0,	0,
	"ftgle",	NA,	0,	0,
	"ftgt",		NA,	0,	0,
	"ftle",		NA,	0,	0,
	"ftlt",		NA,	0,	0,
	"ftneq",	NA,	0,	0,
	"ftnge",	NA,	0,	0,
	"ftngl",	NA,	0,	0,
	"ftngle",	NA,	0,	0,
	"ftngt",	NA,	0,	0,
	"ftnle",	NA,	0,	0,
	"ftnlt",	NA,	0,	0,
	"ftoge",	NA,	0,	0,
	"ftogl",	NA,	0,	0,
	"ftogt",	NA,	0,	0,
	"ftole",	NA,	0,	0,
	"ftolt",	NA,	0,	0,
	"ftor",		NA,	0,	0,
	"ftrapeq",	OC|NA,	123,	0,
	"ftrapeq.l",	OC,	124,	0,
	"ftrapeq.w",	OC,	125,	0,
	"ftrapf",	OC|NA,	126,	0,
	"ftrapf.l",	OC,	127,	0,
	"ftrapf.w",	OC,	128,	0,
	"ftrapge",	OC|NA,	129,	0,
	"ftrapge.l",	OC,	130,	0,
	"ftrapge.w",	OC,	131,	0,
	"ftrapgl",	OC|NA,	132,	0,
	"ftrapgl.l",	OC,	133,	0,
	"ftrapgl.w",	OC,	134,	0,
	"ftrapgle",	OC|NA,	135,	0,
	"ftrapgle.l",	OC,	136,	0,
	"ftrapgle.w",	OC,	137,	0,
	"ftrapgt",	OC|NA,	138,	0,
	"ftrapgt.l",	OC,	139,	0,
	"ftrapgt.w",	OC,	140,	0,
	"ftraple",	OC|NA,	141,	0,
	"ftraple.l",	OC,	142,	0,
	"ftraple.w",	OC,	143,	0,
	"ftraplt",	OC|NA,	144,	0,
	"ftraplt.l",	OC,	145,	0,
	"ftraplt.w",	OC,	146,	0,
	"ftrapne",	OC|NA,	147,	0,
	"ftrapne.l",	OC,	148,	0,
	"ftrapne.w",	OC,	149,	0,
	"ftrapnge",	OC|NA,	150,	0,
	"ftrapnge.l",	OC,	151,	0,
	"ftrapnge.w",	OC,	152,	0,
	"ftrapngl",	OC|NA,	153,	0,
	"ftrapngl.l",	OC,	154,	0,
	"ftrapngl.w",	OC,	155,	0,
	"ftrapngle",	OC|NA,	156,	0,
	"ftrapngle.l",	OC,	157,	0,
	"ftrapngle.w",	OC,	158,	0,
	"ftrapngt",	OC|NA,	159,	0,
	"ftrapngt.l",	OC,	160,	0,
	"ftrapngt.w",	OC,	161,	0,
	"ftrapnle",	OC|NA,	162,	0,
	"ftrapnle.l",	OC,	163,	0,
	"ftrapnle.w",	OC,	164,	0,
	"ftrapnlt",	OC|NA,	165,	0,
	"ftrapnlt.l",	OC,	166,	0,
	"ftrapnlt.w",	OC,	167,	0,
	"ftrapoge",	OC|NA,	168,	0,
	"ftrapoge.l",	OC,	169,	0,
	"ftrapoge.w",	OC,	170,	0,
	"ftrapogl",	OC|NA,	171,	0,
	"ftrapogl.l",	OC,	172,	0,
	"ftrapogl.w",	OC,	173,	0,
	"ftrapogt",	OC|NA,	174,	0,
	"ftrapogt.l",	OC,	175,	0,
	"ftrapogt.w",	OC,	176,	0,
	"ftrapole",	OC|NA,	177,	0,
	"ftrapole.l",	OC,	178,	0,
	"ftrapole.w",	OC,	179,	0,
	"ftrapolt",	OC|NA,	180,	0,
	"ftrapolt.l",	OC,	181,	0,
	"ftrapolt.w",	OC,	182,	0,
	"ftrapor",	OC|NA,	183,	0,
	"ftrapor.l",	OC,	184,	0,
	"ftrapor.w",	OC,	185,	0,
	"ftrapseq",	OC|NA,	186,	0,
	"ftrapseq.l",	OC,	187,	0,
	"ftrapseq.w",	OC,	188,	0,
	"ftrapsf",	OC|NA,	189,	0,
	"ftrapsf.l",	OC,	190,	0,
	"ftrapsf.w",	OC,	191,	0,
	"ftrapsne",	OC|NA,	192,	0,
	"ftrapsne.l",	OC,	193,	0,
	"ftrapsne.w",	OC,	194,	0,
	"ftrapst",	OC|NA,	195,	0,
	"ftrapst.l",	OC,	196,	0,
	"ftrapst.w",	OC,	197,	0,
	"ftrapt",	OC|NA,	198,	0,
	"ftrapt.l",	OC,	199,	0,
	"ftrapt.w",	OC,	200,	0,
	"ftrapueq",	OC|NA,	201,	0,
	"ftrapueq.l",	OC,	202,	0,
	"ftrapueq.w",	OC,	203,	0,
	"ftrapuge",	OC|NA,	204,	0,
	"ftrapuge.l",	OC,	205,	0,
	"ftrapuge.w",	OC,	206,	0,
	"ftrapugt",	OC|NA,	207,	0,
	"ftrapugt.l",	OC,	208,	0,
	"ftrapugt.w",	OC,	209,	0,
	"ftrapule",	OC|NA,	210,	0,
	"ftrapule.l",	OC,	211,	0,
	"ftrapule.w",	OC,	212,	0,
	"ftrapult",	OC|NA,	213,	0,
	"ftrapult.l",	OC,	214,	0,
	"ftrapult.w",	OC,	215,	0,
	"ftrapun",	OC|NA,	216,	0,
	"ftrapun.l",	OC,	217,	0,
	"ftrapun.w",	OC,	218,	0,
	"ftseq",	NA,	0,	0,
	"ftsf",		NA,	0,	0,
	"ftsneq",	NA,	0,	0,
	"ftst",		OC|CC,	219,	0,
	"ftst.b",	OC,	220,	0,
	"ftst.d",	OC,	221,	0,
	"ftst.l",	OC,	222,	0,
	"ftst.p",	OC,	223,	0,
	"ftst.s",	OC,	224,	0,
	"ftst.w",	OC,	225,	0,
	"ftst.x",	OC,	226,	0,
	"ftt",		NA,	0,	0,
	"ftueq",	NA,	0,	0,
	"ftuge",	NA,	0,	0,
	"ftugt",	NA,	0,	0,
	"ftule",	NA,	0,	0,
	"ftult",	NA,	0,	0,
	"ftun",		NA,	0,	0,
	"ftwotox",	CC,	0,	0,
	"illegal",	NA,	0,	0,
	"include",	CM|WR,	0,	0,
	"list",		CM,	0,	0,
	"llen",		CM,	0,	0,
	"lmode",	MT,	0,	0,
	"lprint",	CM,	0,	0,
	"mname",	CM|WR,	0,	0,
	"nolist",	CM,	0,	0,
	"noobj",	CM,	0,	0,
	"nop",		NA,	0,	0,
	"nosyms",	CM,	0,	0,
	"org",		MT,	0,	0,
	"org.l",	MT,	0,	0,
	"page",		CM,	0,	0,
	"refa",		OC,	35,	0,
	"refr",		OC,	35,	0,
	"reset",	NA,	0,	0,
	"rmode",	MT,	0,	0,
	"rorg",		MT,	0,	0,
	"rorg.l",	MT,	0,	0,
	"rte",		NA,	0,	0,
	"rtr",		NA,	0,	0,
	"rts",		NA,	0,	0,
	"shs",		OC,	37,	0,
	"slo",		OC,	38,	0,
	"smode",	MT,	0,	0,
	"spc",		CM,	0,	0,
	"sprint",	CM,	0,	0,
	"src",		CM|WR,	0,	0,
	"start",	MT,	0,	0,
	"trapcc",	OC|NA,	42,	0,
	"trapcc.l",	OC,	54,	0,
	"trapcc.w",	OC,	55,	0,
	"trapcs",	OC|NA,	43,	0,
	"trapcs.l",	OC,	56,	0,
	"trapcs.w",	OC,	57,	0,
	"trapeq",	OC|NA,	44,	0,
	"trapeq.l",	OC,	58,	0,
	"trapeq.w",	OC,	59,	0,
	"trapf",	OC|NA,	45,	0,
	"trapf.l",	OC,	60,	0,
	"trapf.w",	OC,	61,	0,
	"trapge",	OC|NA,	46,	0,
	"trapge.l",	OC,	62,	0,
	"trapge.w",	OC,	63,	0,
	"trapgt",	OC|NA,	47,	0,
	"trapgt.l",	OC,	64,	0,
	"trapgt.w",	OC,	65,	0,
	"traphi",	OC|NA,	48,	0,
	"traphi.l",	OC,	66,	0,
	"traphi.w",	OC,	67,	0,
	"traphs",	OC|NA,	42,	0,
	"traphs.l",	OC,	54,	0,
	"traphs.w",	OC,	55,	0,
	"traple",	OC|NA,	49,	0,
	"traple.l",	OC,	69,	0,
	"traple.w",	OC,	70,	0,
	"traplo",	OC|NA,	43,	0,
	"traplo.l",	OC,	56,	0,
	"traplo.w",	OC,	57,	0,
	"trapls",	OC|NA,	50,	0,
	"trapls.l",	OC,	71,	0,
	"trapls.w",	OC,	72,	0,
	"traplt",	OC|NA,	51,	0,
	"traplt.l",	OC,	73,	0,
	"traplt.w",	OC,	74,	0,
	"trapmi",	OC|NA,	52,	0,
	"trapmi.l",	OC,	75,	0,
	"trapmi.w",	OC,	76,	0,
	"trapne",	OC|NA,	53,	0,
	"trapne.l",	OC,	77,	0,
	"trapne.w",	OC,	78,	0,
	"trappl",	OC|NA,	68,	0,
	"trappl.l",	OC,	79,	0,
	"trappl.w",	OC,	80,	0,
	"trapt",	OC|NA,	87,	0,
	"trapt.l",	OC,	81,	0,
	"trapt.w",	OC,	82,	0,
	"trapv",	NA,	0,	0,
	"trapvc",	OC|NA,	88,	0,
	"trapvc.l",	OC,	83,	0,
	"trapvc.w",	OC,	84,	0,
	"trapvs",	OC|NA,	89,	0,
	"trapvs.l",	OC,	85,	0,
	"trapvs.w",	OC,	86,	0,
	"ttl",		CM,	0,	0
};

static char *reg[5] = { "","cc","fpcr","fpiar","fpsr" };

static char reg1[] = {
	'a','0',0,'c','a','a','r',0,'c','r',0,'c','r',0,'d','0',0,'f',
	'c',0,'f','p','0',0,'c','o','n','t','r','o','l',0,'i','a','d',
	'd','r',0,'s','t','a','t','u','s',0,'i','s','p',0,'m','s','p',
	0,'s','f','c',0,'r',0,'u','s','p',0,'v','b','r',0,'z','p','c',
	0,'1',0,'2',0,'3',0,'4',0,'5',0,'6',0,'7',0,'r',0,'r',0,'r',0
};

static char reg2[] = {
	3,71,0,14,11,8,0,0,0,0,0,0,0,1,20,17,0,71,0,0,45,0,24,
	0,32,85,0,0,0,0,0,2,38,0,87,0,0,3,71,89,0,0,0,0,4,49,0,
	0,0,53,0,0,0,59,57,0,0,61,0,63,0,0,0,67,0,0,0,68,0,0,
	0,73,0,75,0,77,0,79,0,81,0,83,0,0,0,0,2,0,3,0,4
};

static char *optable[229] = {
	"bcc","bcc.b","bcc.l","bcc.w","bcs","bcs.b","bcs.l","bcs.w","beq.b",
	"bge.b","bgt.b","bhi.b","ble.b","bls.b","blt.b","bmi.b","bne.b",
	"bpl.b","bra.b","bsr.b","bvc.b","bvs.b","byte","dbcc","dbcs","double",
	"fdbf","fmov","fmov.b","fmov.d","fmov.l","fmov.p","fmov.s","fmov.w",
	"fmov.x","global","long","scc","scs","set","short","space","tcc",
	"tcs","teq","tf","tge","tgt","thi","tle","tls","tlt","tmi","tne",
	"tpcc.l","tpcc.w","tpcs.l","tpcs.w","tpeq.l","tpeq.w","tpf.l","tpf.w",
	"tpge.l","tpge.w","tpgt.l","tpgt.w","tphi.l","tphi.w","tpl","tple.l",
	"tple.w","tpls.l","tpls.w","tplt.l","tplt.w","tpmi.l","tpmi.w","tpne.l",
	"tpne.w","tppl.l","tppl.w","tpt.l","tpt.w","tpvc.l","tpvc.w","tpvs.l",
	"tpvs.w","tt","tvc","tvs","fdbeq","fdbge","fdbgl","fdbgle","fdbgt",
	"fdble","fdblt","fdbneq","fdbnge","fdbngl","fdbngle","fdbngt","fdbnle",
	"fdbnlt","fdboge","fdbogl","fdbogt","fdbole","fdbolt","fdbor","fdbseq",
	"fdbsf","fdbsne","fdbst","fdbt","fdbueq","fdbuge","fdbugt","fdbule",
	"fdbult","fdbun","fmovm","fmovcr","fteq","ftpeq.l","ftpeq.w","ftf",
	"ftpf.l","ftpf.w","ftge","ftpge.l","ftpge.w","ftgl","ftpgl.l","ftpgl.w",
	"ftgle","ftpgle.l","ftpgle.w","ftgt","ftpgt.l","ftpgt.w","ftle",
	"ftple.l","ftple.w","ftlt","ftplt.l","ftplt.w","ftne","ftpne.l",
	"ftpne.w","ftnge","ftpnge.l","ftpnge.w","ftngl","ftpngl.l","ftpngl.w",
	"ftngle","ftpngle.l","ftpngle.w","ftngt","ftpngt.l","ftpngt.w","ftnle",
	"ftpnle.l","ftpnle.w","ftnlt","ftpnlt.l","ftpnlt.w","ftoge","ftpoge.l",
	"ftpoge.w","ftogl","ftpogl.l","ftpogl.w","ftogt","ftpogt.l","ftpogt.w",
	"ftole","ftpole.l","ftpole.w","ftolt","ftpolt.l","ftpolt.w","ftor",
	"ftpor.l","ftpor.w","ftseq","ftpseq.l","ftpseq.w","ftsf","ftpsf.l",
	"ftpsf.w","ftsne","ftpsne.l","ftpsne.w","ftst","ftpst.l","ftpst.w",
	"ftt","ftpt.l","ftpt.w","ftueq","ftpueq.l","ftpueq.w","ftuge",
	"ftpuge.l","ftpuge.w","ftugt","ftpugt.l","ftpugt.w","ftule","ftpule.l",
	"ftpule.w","ftult","ftpult.l","ftpult.w","ftun","ftpun.l","ftpun.w",
	"ftest","ftest.b","ftest.d","ftest.l","ftest.p","ftest.s","ftest.w",
	"ftest.x", "tdivs.l","tdivu.l"
};

main(argc,argv) int argc; char **argv;
{
	register char *inptr,*outptr,*t1ptr,*t2ptr,c,d;
	int line_no,opcode,i,j,operands();
	short level,quote;
	char longline,*codeptr,*opptr,*src,*dst;

	nflag = 0;
	if (argc == 1) infile = stdin;
	else {
		i = 1;
		if (argv[1][0] == '-') {
			if (strcmp(argv[1],"-n")) {
				fprintf(stderr,badoptmsg);
				exit(1);
			} else {
				nflag++;
				i = 2;
			};
		};
		if (nflag && (argc == 2)) infile = stdin;
		else if (i == (argc - 1)) {
			if ((infile = fopen(argv[i],"r")) == NULL) {
				fprintf(stderr,openmsg,argv[i]);
				exit(1);
			};
		} else {
			fprintf(stderr,usagemsg);
			exit(1);
		};
	};

	line_no = 0;
	longline = 0;

	while (fgets(inline,MAXLINE + 2,infile) == inline) {

		/* Truncate lines to 132 characters */

		t1ptr = inline;
		while (*t1ptr++);
		if ((t1ptr - inline == MAXLINE + 2) &&
			inline[MAXLINE] != EOL) {
			if (longline) continue;
			longline = 1;
			inline[MAXLINE] = EOL;
		} else if (longline) {
			longline = 0;
			continue;
		};

		/* Initialization */

		line_no++;
		globalflg = 0;
		quoteflg = 0;
		inptr = inline;
		outptr = outline;
		
		/* Is this line a comment? */

		if (*inptr == '*') {
			*inptr = '#';
			lput(inline,stdout);
			goto loopend;
		};

		/* Scan line for first non-whitespace */

		NEXTFIELD(inptr,outptr,c);
		if (c == EOL) {	/* "blank" line? */
			*outptr++ = c;
			*outptr++ = 0;
			lput(outline,stdout);
			goto loopend;
		};
		inptr--;

		/* Check for a label */

		if (inptr == inline) {
			globalflg |= LABEL;
			c = *inptr;
			/* move label */
			while (charset[c = *inptr++] & ID) {
				if (charset[c] & IC) {
					globalflg |= BADIDCHAR;
					c = c == '@' ? 'A' : 'S';
				} else if (charset[c] & UC)
					c += 'a' - 'A';
				*outptr++ = c;
			};
			*outptr++ = ':';
			/* whitespace must follow label */
			if (!(charset[c] & WS || c == EOL)) {
				globalflg |= SYNTAX;
				goto loopend;
			};
			--inptr;
			/* find next non-whitespace */
			NEXTFIELD(inptr,outptr,c);
			/* Does line only contain a label? */
			if (c == EOL) {
				*outptr++ = c;
				*outptr++ = 0;
				lput(outline,stdout);
				goto loopend;
			};
			--inptr;
		};

		/* Search table for opcode mnemonic */

		codeptr = outptr;
		opptr = inptr;
		while (!(charset[c = *inptr++] & WS || c == EOL)) {
			if (charset[c] & UC) c += 'a' - 'A';
			*outptr++ = c;
		};
		--inptr;
		t1ptr = codeptr;
		c = *t1ptr;
		opcode = 0;
		if (c > 'a' && c <= 't' && ((opcode = tableptr[c-'a']) > 0)) {
			*outptr = 0;
			if ((i = tableptr[c-'a'+1]) < 0) i = -i;
			for (; opcode < i; opcode++) {
				t1ptr = codeptr;
				t2ptr = table[opcode].name;
				while ((*t1ptr++ == (c = *t2ptr++)) && c);
				if (*--t1ptr != c) {
					if (*t1ptr < c) {
						opcode = 0;
						break;
					};
				} else break;
			};
			if (opcode == i) opcode = 0;
		};

		if (opcode < 0) opcode = 0;
		if (opcode) {

			i = table[opcode].action;

			/* Change opcode mnemonic? */

			if (i & OC) {
				if (i & SQ) quoteflg = table[opcode].p2;
				t1ptr = codeptr;
				t2ptr = optable[table[opcode].p1];
				while (*t1ptr++ = *t2ptr++);
				outptr = --t1ptr;
			};

			/* Add '.w' to default mnemonic? */

			if (i & CC) {
				j = 1;
				t1ptr = inptr;
				while (charset[*t1ptr++] & WS);
				t1ptr--;
				if (((*t1ptr++ | 32) == 'f') &&
				    ((*t1ptr++ | 32) == 'p') &&
				    ((c = *t1ptr++) >= '0')  &&
				    (c <= '7')) {
					c = *t1ptr++;
					if ((charset[c] & WS) || (c == EOL))
						j = 0;
					else if ((c == ',') &&
					    ((*t1ptr++ | 32) == 'f') &&
					    ((*t1ptr++ | 32) == 'p') &&
					    ((c = *t1ptr++) >= '0')  &&
					    (c <= '7')) {
						c = *t1ptr;
						if ((charset[c] & WS) || 
						   (c == EOL) ||
						   (c == ':')) j = 0;
					};
				};
				if (j && table[opcode].p2) {
					t1ptr = inptr;
					while (charset[*t1ptr++] & WS);
					t1ptr--;
					t2ptr = chartemp;
					do {
						c = *t1ptr++;
						if (c >= 'A' && c <= 'Z') 
							c += 32;
						*t2ptr++ = c;
					} while (c);
					src = chartemp;
					if (regcheck(&src) >= 2) j = 0;
					else {
						level = 0;
						t1ptr = chartemp;
						for (;;) {
						  c = *t1ptr++;
						  if (charset[c] & UP) 
							level++;
						  else if (charset[c] & DN) 
							level--;
						  else if ((c == ',' && !level) 
							   || !c) break;
						};
						src = t1ptr;
						if (c && (regcheck(&src) >= 2))
								j = 0;
					};
				};
				if (j) {
					*outptr++ = '.';
					*outptr++ = 'w';
				};
			};

			/* Comment line out? */

			if (i & CM) {
				inptr = inline;
				outptr = outline;
				*outptr++ = '#';
				while (*outptr++ = *inptr++);
				if (i & WR) globalflg |= WARNING;
				lput(outline,stdout);
				goto loopend;
			};

			if (i & MT) {
				globalflg |= MAN_TRANS;
				lput(inline,stdout);
				goto loopend;
			};
		};

		NEXTFIELD(inptr,outptr,c);
		if (c == EOL) {
			*outptr++ = c;
			*outptr++ = 0;
			lput(outline,stdout);
			goto loopend;
		};
		--inptr;

		if (opcode) {
			i = table[opcode].action;
			if (i & NA) goto comment;
			if (i & SP) {
				switch (table[opcode].p2) {
				case 1: /* cas2 */
					t1ptr = inptr;
					for (j = 0; j < 21; j++)
						if (*t1ptr++ == EOL) {
							globalflg |= SYNTAX;
							goto loopend;
						};
					c = '%';
					for (j = 3; j >= 0; j--) {
						*outptr++ = c;
						*outptr++ = *inptr++ | 32;
						*outptr++ = *inptr++;
						*outptr++ = *inptr++;
					};
					*outptr++ = c;
					++inptr;
					*outptr++ = *inptr++ | 32;
					*outptr++ = *inptr++;
					++inptr;
					*outptr++ = *inptr++;
					*outptr++ = c;
					++inptr;
					*outptr++ = *inptr++ | 32;
					*outptr++ = *inptr++;
					++inptr;
					goto comment_find;
				case 2: /* ds, ds.w */
					*outptr++ = '2';
					*outptr++ = '*';
					*outptr++ = '(';
					break;
				case 3: /* ds.d */
					*outptr++ = '8';
					*outptr++ = '*';
					*outptr++ = '(';
					break;
				case 4: /* ds.l, ds.s */
					*outptr++ = '4';
					*outptr++ = '*';
					*outptr++ = '(';
					break;
				case 5: /* ds.p, ds.x */
					*outptr++ = '1';
					*outptr++ = '2';
					*outptr++ = '*';
					*outptr++ = '(';
					break;
				case 6: /* equ */
					if (!(globalflg & LABEL)) {
						globalflg |= SYNTAX;
						goto loopend;
					};
					t1ptr = outline;
					while ((c = *t1ptr) != ':') {
						*outptr++ = c;
						*t1ptr++ = ' ';
					};
					t2ptr = t1ptr + 1;
					while (t2ptr < outptr) 
						*t1ptr++ = *t2ptr++;
					*(outptr - 1) = ',';
					break;
				};
			};
		};
		
		src = inptr;
		dst = outptr;

		/* translate arguments */

		if ((i = operands(&src,&dst)) < 0) {
			if (i == -1) globalflg |= SYNTAX;
			else {
				t1ptr = opptr;
				t2ptr = codeptr;
				*t2ptr++ = '#';
				while (*t2ptr++ = *t1ptr++);
				lput(outline,stdout);
			};
			goto loopend;
		};

		/* do the arguments have to be swapped? */

		if (table[opcode].action & SW) {
			t1ptr = outptr;
			level = 0;
			quote = 0;
			for (;;) {
				c = *t1ptr++;
				if (!c) {
					globalflg |= SYNTAX;
					goto loopend;
				};
				if (!quote) {
					if (charset[c] & UP) level++;
					else if (charset[c] & DN) level--;
					else if (c == ',' && !level) {
						codeptr = t1ptr;
						break;
					};
				} else if (c == '\'') quote = !quote;
			};
			t2ptr = outptr;
			t1ptr = chartemp;
			for (i = codeptr - outptr - 1; i > 0; i--)
				*t1ptr++ = *t2ptr++;
			t1ptr = outptr;
			++t2ptr;
			for (i = dst - codeptr; i > 0; i--)
				*t1ptr++ = *t2ptr++;
			*t1ptr++ = ',';
			t2ptr = chartemp;
			for (i = codeptr - outptr - 1; i > 0; i--)
				*t1ptr++ = *t2ptr++;
		};

		if (table[opcode].action & SP) {
			if (!(i = table[opcode].p2)) { /* bit field */
				t1ptr = outptr;
				for (i = 1; i >= 0; i--) {
					d = i ? '{' : ':';
					while (((c = *t1ptr++) != d) && c);
					if (!c) {
						globalflg |= SYNTAX;
						goto loopend;
					};
					if (*t1ptr != '%') {
						c = *t1ptr;
						*t1ptr++ = '&';
						t2ptr = t1ptr;
						while (t2ptr <= dst) {
							d = *t2ptr;
							*t2ptr++ = c;
							c = d;
						};
						dst++;
					};
				};	
			} else 

			/* ds, ds.w, ds.l, ds.s, ds.d, ds.p, ds.x */
			
			if (i >= 2 && i <= 5) *dst++ = ')';
		};

		inptr = src;
		outptr = dst;

	comment_find:
		NEXTFIELD(inptr,outptr,c);
		if (c == EOL) {
			*outptr++ = c;
			*outptr++ = 0;
			lput(outline,stdout);
			goto loopend;
		};
		--inptr;

	comment:
		*outptr++ = '#';
		while (*outptr++ = *inptr++);
		lput(outline,stdout);

	loopend:
		if (globalflg) {
			if (globalflg & SYNTAX) {
				lput(inline,stdout);
				fprintf(stderr,syntaxmsg,line_no);
			} else {
				if (globalflg & BADIDCHAR)
					fprintf(stderr,badcharmsg,line_no);
				if (globalflg &	MAN_TRANS)
					fprintf(stderr,mtmsg,line_no);
				if (globalflg & WARNING)
					fprintf(stderr,warnmsg,line_no);
				if (globalflg & PCREL) 
					fprintf(stderr,pcmsg,line_no);
			};
		};
		if (longline)
			fprintf(stderr,longlinemsg,line_no,MAXLINE);
	};

	fclose(infile);
	exit(0);
}

static int operands(src,dest) char **src,**dest;
{
	register char c,d,*sptr = *src,*dptr = *dest;
	short tree,expr();
	char *tptr,*listexpr();
	int i,j,k,regcheck();

	for (;;) {
		c = *sptr;
		if ((charset[c] & WS) || c == EOL) break;
		if (c == '#') {
			*dptr++ = '&';
			sptr++;
			continue;
		};
		if (c == '\'') {
			sptr++;
			if (quoteflg == 1) {
				*dptr++ = '"';
				while ((c = *sptr++) != '\'') {
					if (!c) return -1;
					if (c == '"') *dptr++ = '\\';
					*dptr++ = c;
				};
				*dptr++ = '"';
				if (*sptr == ',') {
					*dptr++ = ',';
					sptr++;
				};
			} else for (;;) {
				if (!(j = quoteflg)) j = 4;
				k = 0;
				for (i = 0; i < j; i++) {
					c = *sptr++;
					if (c != '\'' || *sptr == '\'') {
						if (!c) return -1;
						k <<= 8;
						k += c;
					} else {
						--sptr;
						break;
					};
					if (c == '\'') ++sptr;
				};
				if (i) {
					if (quoteflg) {
						k <<= 8 * (j - i);
						i = quoteflg;
					};
					i *= 2;
					*dptr++ = '0';
					*dptr++ = 'x';
					for (j = i - 1; j >= 0; j--) {
						c = ((k >> j * 4) & 0xF) + '0';
						if (c > '9') c += 7;
						*dptr++ = c;
					};
				} else {
					if (!quoteflg) return -1;
					if (*++sptr == ',') sptr++;
					break;
				};
				if (*sptr == '\'') {
					sptr++;
					break;
				};
				if (!quoteflg) return -1;
				*dptr++ = ',';
			};
			continue;
		};
		if (c == '.')
			if (!(charset[*++sptr] & DG)) {
				*dptr++ = c;
				continue;
			} else sptr--;
		if (c == '+' || c == '-')
			if (charset[*(tptr = sptr + 1)] & AS || 
			    regcheck(&tptr) != -1) {
				*dptr++ = c;
				sptr++;
				continue;
			};
		if (c == '*')
			if (charset[*++sptr] & SF) {
				*dptr++ = c;
				continue;
			} else sptr--;
		if (charset[c] & (ID | EX)) {
			tptr = sptr;
			if ((i = regcheck(&tptr)) == -1) {
				nextnode = 0;
				npoolptr = namepool;
				if ((tree = expr(&tptr)) == -1) return -1;
				sptr = tptr;
				dptr = listexpr(tree,dptr,-1);
			} else {
				*dptr++ = '%';
				if (i) {
					sptr = reg[i];
					while (*dptr++ = *sptr++);
					--dptr;
					sptr = tptr;
				} else for (i = tptr - sptr; i > 0; i--) {
						c = *sptr++;
						if (c > '9') c |= 32;
						*dptr++ = c;
					};
			};
			continue;
		};
		*dptr++ = c;
		sptr++;
	};
	if (*dest == dptr) return -2;
	*src = sptr;
	*dest = dptr;
	return 0;
}

static int regcheck(ptr) char **ptr;
{
	register char *sptr = *ptr,c,d;
	register short i;

	i = 0;
	for (;;) {
		if (charset[c = *sptr] & UC) c |= 32;
		if ((d = reg1[i]) && c == d) {
			i++;
			sptr++;
		} else if (!(d || (charset[c] & ID))) {
			*ptr = sptr;
			if (i == 70) globalflg |= PCREL;
			return reg2[i];
		} else if (d && c != d && (i = reg2[i]));
		else return -1;
	};
}

static short expr(p) char **p;
{
	register char c,*ptr = *p;
	short head = -1;
	char hex;

	for (;;) {

		/* Scan for unary operators */
		while (charset[c = *ptr++] & UO)
			if (c != '+') {
				node[nextnode].token = c;
				node[nextnode].prec = 4;
				node[nextnode].left = head;
				head = nextnode;
				node[nextnode++].right = -1;
			};

		/* Current pc location? */
		if (c == '*') {
			node[nextnode].ident = npoolptr;
			*npoolptr++ = '.';
			globalflg |= PCREL;
		}

		/* identifier or constant? */
		else if (c == '.' || charset[c] & ID) {
			ptr--;
			node[nextnode].ident = npoolptr;
			if (c == '$' || c == '.' || charset[c] & DG) {
				*npoolptr++ = '0';
				if (c == '$') {
					hex = 1;
					*npoolptr++ = 'x';
					ptr++;
				} else {
					hex = 0;
					*npoolptr++ = 'f';
				};
				while (charset[c = *ptr++] & DG) 
					*npoolptr++ = c;
				if (c != '.') {
					if (!hex) node[nextnode].ident += 2;
					ptr--;
				} else {
					*npoolptr++ = c;
					while (charset[c = *ptr++] & DG) 
						*npoolptr++ = c;
					if ((c | 32) != 'e') ptr--;
					else {
						*npoolptr++ = c;
						c = *ptr;
						if (c == '+' || c == '-')
							*npoolptr++ = *ptr++;
						while (charset[c = *ptr++] & DG)
							*npoolptr++ = c;
						ptr--;
					};
				};
			} else {
				while (charset[c = *ptr++] & ID) {
					if (charset[c] & IC) {
						globalflg |= BADIDCHAR;
						c = c == '@' ? 'A' : 'S';
					} else if (charset[c] & UC) c |= 32;
					*npoolptr++ = c;
				};
				ptr--;
			};
		} else return -1;
		*npoolptr++ = 0;
		node[nextnode].token = 0;
		node[nextnode].prec = 5;
		node[nextnode].left = -1;
		node[nextnode].right = -1;
		if (head != -1) node[head].right = nextnode++;
		else head = nextnode++;

		/* Is next token a binary operator? */
		if (charset[c = *ptr++] & BO) {
			/* Convert ! to | */
			if (c == '!') c = '|';
			node[nextnode].token = c;
			/* Define operator precedence */
			node[nextnode].prec = c == '&' ? 2 : (c == '|' ? 1 : 3);
			node[nextnode].left = head;
			head = nextnode;
			node[nextnode++].right = -1;
		}

		/* End of expression */
		else {
			*p = --ptr;
			return head;
		};
		c = *ptr;
		if (!(c == '.' || c == '*' || charset[c] & (ID|UO)))
			return -1;
	};
}

char *listexpr(nptr,dptr,lastprec) short nptr;
register char *dptr; int lastprec;
{
	register char *sptr;
	short paren;

	/* in order tree traversal */

	paren = 0;
	if (node[nptr].prec < lastprec)
		if (!(lastprec == 4 && node[nptr].right == (char) -1))
			paren = 1;
	if (paren) *dptr++ = '(';
	if (node[nptr].left != (char) -1) /* left child? */
		dptr = listexpr(node[nptr].left,dptr,node[nptr].prec);
	if (!(*dptr++ = node[nptr].token)) {
		--dptr;
		sptr = node[nptr].ident;
		while (*dptr++ = *sptr++);
		--dptr;
	};
	if (node[nptr].right != (char) -1) /* right child? */
		dptr = listexpr(node[nptr].right,dptr,node[nptr].prec);
	if (paren) *dptr++ = ')';
	return dptr;
}

int lput(ptr,stream) char *ptr; FILE *stream;
{
	register char c,*tptr;
	short quote,count;

	if (nflag && !(globalflg & MAN_TRANS)) {
		quote = 0;
		count = 4;
		tptr = ptr;
		for (;;) {
			c = *tptr++;
			if (!c) {
				fputc(c,stream);
				return 0;
			};
			if (quote) {
				fputc(c,stream);
				if ((c == '\\') && (*tptr == '"')) {
					fputc('"',stream);
					tptr++;
				};
			} else {
				if (c == '#') {
					fputs(--tptr,stream);
					return 0;
				};
				if (charset[c] & WS) {
					while (charset[*tptr++] & WS);
					count--;
					if ((c = *--tptr) != EOL) {
						fputc('\t',stream);
						if (c == '#')
							while (count-- > 0)
								fputc('\t',stream);
					};
					continue;
				} else fputc(c,stream);
			};
			if (c == '"') quote = !quote;
		}; 
	} else return fputs(ptr,stream);
}
