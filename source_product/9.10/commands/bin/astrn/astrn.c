static char *HPUX_ID = "@(#) $Revision: 30.8 $";
#include <stdio.h>

#define MAXLINE	132
#define TABLESIZE 293
#define EOL 10	/* end of line */
#define CR 13	/* carriage return */

#define WS 1	/* white space */
#define ID 2	/* identifier character */
#define IC 4	/* identifier character to change */
#define RT 8	/* register terminator */
#define UO 16	/* unary operator */
#define BO 32	/* binary operator */
#define RG 64   /* a, d, s or p: first letter of register */

#define BADIDCHAR 1 /* bad character in identifier */
#define LABEL 2	/* line has a label */
#define UNREC 4 /* unrecognized opcode mnemonic */
#define PCREL 8 /* pc addressing mode used */
#define SEMICOLON 16 /* semicolon found in line */
#define LLABEL 32 /* line has local label */

#define OC 1	/* change opcode mnemonic */
#define US 2	/* unsupported opcode mnemonic */
#define AR 4	/* one or more arguments */
#define SP 8	/* statement requires special handling */
#define SW 16	/* swap arguments */

#define NEXTFIELD(in,out,ch) while (charset[(unsigned char) (ch = *in++)] & WS) if (ch != CR) *out++ = ch

struct nodetype {
	char *ident,left,right,prec,token;
} node[MAXLINE];

char namepool[2 * MAXLINE];

static short nextnode,parenlevel;
static char *npoolptr,*outline;

char charset[256] = {
0,	0,	0,	0,	0,	0,	0,	0,	/*   0 */
0,	WS|RT,	WS|RT,	WS|RT,	WS|RT,	WS|RT,	0,	0,	/*   8 */
0,	0,	0,	0,	0,	0,	0,	0,	/*  16 */
0,	0,	0,	0,	0,	0,	0,	0,	/*  24 */
WS|RT,	BO,	0,	0,	ID|IC,	BO,	BO,	0,	/*  32 */
0,	0,	BO,	UO|BO,	RT,	RT|UO|BO,ID,	RT|BO,	/*  40 */
ID,	ID,	ID,	ID,	ID,	ID,	ID,	ID,	/*  48 */
ID,	ID,	0,	0,	BO,	0,	BO,	ID|IC,	/*  56 */
ID|IC,	ID,	ID,	ID,	ID,	ID,	ID,	ID,	/*  64 */
ID,	ID,	ID,	ID,	ID,	ID,	ID,	ID,	/*  72 */
ID,	ID,	ID,	ID,	ID,	ID,	ID,	ID,	/*  80 */
ID,	ID,	ID,	0,	0,	0,	BO,	ID,	/*  88 */
0,	ID|RG,	ID,	ID,	ID|RG,	ID,	ID,	ID,	/*  96 */
ID,	ID,	ID,	ID,	ID,	ID,	ID,	ID,	/* 104 */
ID|RG,	ID,	ID,	ID|RG,	ID,	ID,	ID,	ID,	/* 112 */
ID,	ID,	ID,	0,	0,	0,	UO,	ID|IC,	/* 120 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 128 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 136 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 144 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 152 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 160 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 168 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 176 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 184 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 192 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 200 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 208 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 216 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 224 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 232 */
0,	0,	0,	0,	0,	0,	0,	0,	/* 240 */
0,	0,	0,	0,	0,	0,	0,	0	/* 248 */
};

FILE *infile;

static char inline[MAXLINE+3],lastlabel[MAXLINE],chartemp[MAXLINE];
static short globalflg;
static char *arg2;

char *optable[53] = {
	"jsr", "bcc.w", "bcs.w", "beq.w", "bge.w", "bgt.w", "bhi.w",
	"ble.w", "bls.w", "blt.w", "bmi.w", "bne.w", "bpl.w", "bra.w",
	"bvc.w", "bvs.w", "byte", "short", "long", "space", "lalign",
	"global", "bne.b", "bgt.b", "bmi.b", "bra.b", "bls.b", "blt.b",
	"bvc.b", "bsr.b", "bhi.b", "ble.b", "bpl.b", "bvs.b", "bcc.b",
	"bge.b", "bcs.b", "beq.b", "movp.w", "movp.l", "movs.w", "movs.b",
	"movs.l", "movc", "mova.w", "mova.l", "movm.w", "movm.l", "movq",
	"mov.w", "mov.b", "mov.l", "mov"
};

char *dntt_table[27] = {
	"srcfile","module","function","entry","begin","end","import",
	"label","fparam","svar","dvar","","const","typedef","tagdef",
	"pointer","enum","memenum","set","subrange","array","struct",
	"union","field","variant","file","functype"
};

struct {
	char *name;
	short next;
	char action,parameter;
} table[TABLESIZE] = {
	{ NULL,		0,	0,		0},
	{ "abcd",	0,	AR,		0},
	{ "add.l",	0,	AR,		0},
	{ "add.w",	2,	AR,		0},
	{ "adda.l",	0,	AR,		0},
	{ "adda.w",	4,	AR,		0},
	{ "ds.w",	0,	SP,		1},
	{ "bne.s",	70,	OC|AR,		22},
	{ "adda",	5,	AR,		0},
	{ "bgt.s",	154,	OC|AR,		23},
	{ "addi.b",	0,	AR,		0},
	{ "bmi.s",	0,	OC|AR,		24},
	{ "addi.l",	10,	AR,		0},
	{ "jeq",	0,	OC|AR,		3},
	{ "addi.w",	12,	AR,		0},
	{ "bra.s",	134,	OC|AR,		25},
	{ "addi",	14,	AR,		0},
	{ "jle",	0,	OC|AR,		7},
	{ "addq.b",	0,	AR,		0},
	{ "addq.l",	18,	AR,		0},
	{ "addq.w",	19,	AR,		0},
	{ "bvs",	0,	AR,		0},
	{ "addx.b",	0,	AR,		0},
	{ "addx.l",	22,	AR,		0},
	{ "addq",	128,	AR,		0},
	{ "jne",	193,	OC|AR,		11},
	{ "bchg",	0,	AR,		0},
	{ "bls.s",	151,	OC|AR,		26},
	{ "addx.w",	23,	AR,		0},
	{ "blt.s",	207,	OC|AR,		27},
	{ "and.l",	0,	AR,		0},
	{ "addx",	28,	AR,		0},
	{ "and.w",	30,	AR,		0},
	{ "asl.b",	161,	AR,		0},
	{ "dbge",	0,	AR,		0},
	{ "bvc.s",	0,	OC|AR,		28},
	{ "andi",	0,	AR,		0},
	{ "andi.b",	36,	AR,		0},
	{ "andi.l",	37,	AR,		0},
	{ "andi.w",	38,	AR,		0},
	{ "dbcs",	115,	AR,		0},
	{ "asl.l",	0,	AR,		0},
	{ "dbeq",	0,	AR,		0},
	{ "jmp",	0,	AR,		0},
	{ "dble",	0,	AR,		0},
	{ "asr.b",	158,	AR,		0},
	{ "asl.w",	41,	AR,		0},
	{ "jlt",	160,	OC|AR,		9},
	{ "dbne",	0,	AR,		0},
	{ "dbgt",	230,	AR,		0},
	{ "alias",	116,	US,		1},
	{ "asr.l",	0,	AR,		0},
	{ "data",	122,	0,		0},
	{ "bsr.s",	162,	OC|AR,		29},
	{ "asr.w",	51,	AR,		0},
	{ "ext",	0,	AR,		0},
	{ "align",	39,	AR,		0},
	{ "bclr",	54,	AR,		0},
	{ "dbls",	0,	AR,		0},
	{ "dblt",	118,	AR,		0},
	{ "bhi",	0,	AR,		0},
	{ "eor.b",	138,	AR,		0},
	{ "dbvc",	0,	AR,		0},
	{ "bhi.s",	0,	OC|AR,		30},
	{ "ble.s",	0,	OC|AR,		31},
	{ "bpl",	0,	AR,		0},
	{ "bpl.s",	0,	OC|AR,		32},
	{ "bvs.s",	0,	OC|AR,		33},
	{ NULL,		0,	AR,		0},
	{ "or.b",	233,	AR,		0},
	{ "bsr",	0,	AR,		0},
	{ "jsr",	196,	AR,		0},
	{ "bss",	0,	0,		0},
	{ "chk",	3,	AR,		0},
	{ "asciz",	0,	SP,		8},
	{ "lsl",	0,	AR,		0},
	{ "clr",	0,	AR,		0},
	{ "clr.b",	46,	AR,		0},
	{ "dbvs",	0,	AR,		0},
	{ "or.l",	0,	AR,		0},
	{ "clr.l",	77,	AR,		0},
	{ "scs",	277,	AR,		0},
	{ "clr.w",	80,	AR,		0},
	{ "nop",	0,	0,		0},
	{ "cmpa",	100,	AR|SW,		0},
	{ "jvs",	231,	OC|AR,		15},
	{ "cmp",	76,	AR|SW,		0},
	{ "lsr",	0,	AR,		0},
	{ "cmp.b",	82,	AR|SW,		0},
	{ "ori",	240,	AR,		0},
	{ "or.w",	0,	AR,		0},
	{ "not",	0,	AR,		0},
	{ "cmpi",	104,	AR|SW,		0},
	{ "cmp.l",	88,	AR|SW,		0},
	{ "cmp.w",	93,	AR|SW,		0},
	{ "cmpa.l",	0,	AR|SW,		0},
	{ "cmpm",	112,	AR|SW,		0},
	{ "bset",	247,	AR,		0},
	{ "comm",	0,	AR,		0},
	{ "sgt",	0,	AR,		0},
	{ "cmpa.w",	95,	AR|SW,		0},
	{ "ext.l",	243,	AR,		0},
	{ "cmpi.b",	0,	AR|SW,		0},
	{ "cmpi.l",	102,	AR|SW,		0},
	{ "cmpi.w",	103,	AR|SW,		0},
	{ "nbcd",	0,	AR,		0},
	{ "divs",	0,	AR,		0},
	{ "rol",	0,	AR,		0},
	{ "divu",	0,	AR,		0},
	{ "cmpm.b",	0,	AR|SW,		0},
	{ "cmpm.l",	109,	AR|SW,		0},
	{ "globl",	227,	OC|AR,		21},
	{ "cmpm.w",	110,	AR|SW,		0},
	{ "rte",	0,	0,		0},
	{ "dbcc",	20,	AR,		0},
	{ "dbhi",	0,	AR,		0},
	{ "dbmi",	0,	AR,		0},
	{ "sls",	0,	AR,		0},
	{ "dbpl",	0,	AR,		0},
	{ "jbsr",	249,	OC|AR,		0},
	{ "eori",	143,	AR,		0},
	{ "include",	167,	US,		0},
	{ "dbra",	0,	AR,		0},
	{ "dntt",	131,	0,		0},
	{ "illegal",	0,	0,		0},
	{ "svc",	0,	AR,		0},
	{ "dbt",	0,	AR,		0},
	{ "even",	0,	SP,		3},
	{ "dc",		114,	SP,		5},
	{ "btst",	0,	AR,		0},
	{ "dc.l",	0,	SP,		6},
	{ "dnttentry",	0,	SP,		9},
	{ "ds.l",	0,	SP, 		2},
	{ "lsr.b",	170,	AR,		0},
	{ "eor",	0,	AR,		0},
	{ "ori.b",	205,	AR,		0},
	{ "eor.l",	0,	AR,		0},
	{ "not.b",	202,	AR,		0},
	{ "eor.w",	136,	AR,		0},
	{ "rtr",	0,	0,		0},
	{ "eori.b",	0,	AR,		0},
	{ "rts",	0,	0,		0},
	{ "eori.l",	140,	AR,		0},
	{ "eori.w",	142,	AR,		0},
	{ "equ",	66,	SP,		7},
	{ "negx",	228,	AR,		0},
	{ "link",	0,	AR,		0},
	{ "exg",	144,	AR,		0},
	{ "ext.w",	0,	AR,		0},
	{ "jcs",	72,	OC|AR,		2},
	{ "jge",	32,	OC|AR,		4},
	{ "jgt",	0,	OC|AR,		5},
	{ "vt",		0,	0,		0},
	{ "rol.b",	210,	AR,		0},
	{ "jhi",	149,	OC|AR,		6},
	{ "tst",	0,	AR,		0},
	{ "sf",		0,	AR,		0},
	{ "svs",	0,	AR,		0},
	{ "jls",	57,	OC|AR,		8},
	{ "jmi",	147,	OC|AR,		10},
	{ "jpl",	0,	OC|AR,		12},
	{ "jra",	94,	OC|AR,		13},
	{ "jvc",	0,	OC|AR,		14},
	{ "lea",	150,	AR,		0},
	{ "lsl.b",	0,	AR,		0},
	{ "ror.b",	257,	AR,		0},
	{ "lsl.l",	164,	AR,		0},
	{ "lsl.w",	166,	AR,		0},
	{ "lsr.l",	0,	AR,		0},
	{ NULL,		0,	0,		0},
	{ "lsr.w",	168,	AR,		0},
	{ "movep",	0,	OC|AR,		38},
	{ "or",		0,	AR,		0},
	{ "movep.l",	171,	OC|AR,		39},
	{ "movep.w",	173,	OC|AR,		38},
	{ "moves",	174,	OC|AR,		40},
	{ "moves.b",	175,	OC|AR,		41},
	{ "moves.l",	176,	OC|AR,		42},
	{ "moves.w",	177,	OC|AR,		40},
	{ "movec",	178,	OC|AR,		43},
	{ "movea",	179,	OC|AR,		44},
	{ "movea.l",	180,	OC|AR,		45},
	{ "reset",	0,	0,		0},
	{ "movea.w",	181,	OC|AR,		44},
	{ "movem",	183,	OC|AR,		46},
	{ "movem.w",	184,	OC|AR,		46},
	{ "movem.l",	185,	OC|AR,		47},
	{ "moveq",	186,	OC|AR,		48},
	{ "move",	191,	OC|AR,		52},
	{ "move.b",	187,	OC|AR,		50},
	{ "move.w",	189,	OC|AR,		49},
	{ "move.l",	190,	OC|AR,		51},
	{ "muls",	0,	AR,		0},
	{ "neg",	0,	AR,		0},
	{ "neg.b",	0,	AR,		0},
	{ "neg.l",	194,	AR,		0},
	{ "neg.w",	195,	AR,		0},
	{ "negx.b",	0,	AR,		0},
	{ "negx.l",	197,	AR,		0},
	{ "negx.w",	198,	AR,		0},
	{ "not.l",	0,	AR,		0},
	{ "tst.b",	283,	AR,		0},
	{ "not.w",	200,	AR,		0},
	{ "ori.l",	0,	AR,		0},
	{ "add",	0,	AR,		0},
	{ "ori.w",	203,	AR,		0},
	{ "bcc",	192,	AR,		0},
	{ "pea",	159,	AR,		0},
	{ "mulu",	0,	AR,		0},
	{ "rol.l",	0,	AR,		0},
	{ "rol.w",	209,	AR,		0},
	{ "ror",	0,	AR,		0},
	{ "st",		0,	AR,		0},
	{ "ror.l",	0,	AR,		0},
	{ "dc.b",	0,	SP,		4},
	{ "ror.w",	213,	AR,		0},
	{ "suba",	260,	AR,		0},
	{ "roxl.b",	0,	AR,		0},
	{ "roxl.l",	217,	AR,		0},
	{ "roxl.w",	218,	AR,		0},
	{ "roxr.b",	0,	AR,		0},
	{ "roxr.l",	220,	AR,		0},
	{ "roxr.w",	221,	AR,		0},
	{ "text",	0,	0,		0},
	{ "dbf",	265,	AR,		0},
	{ "trap",	279,	AR,		0},
	{ "bge",	0,	AR,		0},
	{ "rtd",	0,	AR,		0},
	{ "sbcd",	199,	AR,		0},
	{ "sltnormal",	0,	SP,		10},
	{ "scc",	0,	AR,		0},
	{ "seq",	0,	AR,		0},
	{ "subq",	268,	AR,		0},
	{ "sge",	0,	AR,		0},
	{ "sltspecial",	285,	SP,		10},
	{ "dc.w",	0,	SP,		5},
	{ "shi",	152,	AR,		0},
	{ "swap",	0,	AR,		0},
	{ "bcs",	60,	AR,		0},
	{ "roxl",	275,	AR,		0},
	{ "sle",	0,	AR,		0},
	{ "slt",	211,	AR,		0},
	{ "beq",	0,	AR,		0},
	{ "smi",	148,	AR,		0},
	{ "and",	0,	AR,		0},
	{ "roxr",	222,	AR,		0},
	{ "ble",	0,	AR,		0},
	{ "sne",	0,	AR,		0},
	{ "spl",	241,	AR,		0},
	{ "sub",	248,	AR,		0},
	{ "add.b",	73,	AR,		0},
	{ "sub.b",	215,	AR,		0},
	{ "bcc.s",	126,	OC|AR,		34},
	{ "stop",	0,	AR,		0},
	{ "bne",	0,	AR,		0},
	{ "sub.l",	251,	AR,		0},
	{ "bgt",	0,	AR,		0},
	{ "sub.w",	255,	AR,		0},
	{ "bmi",	0,	AR,		0},
	{ "suba.l",	0,	AR,		0},
	{ "suba.w",	259,	AR,		0},
	{ "subi",	130,	AR,		0},
	{ "bra",	0,	AR,		0},
	{ "subi.b",	261,	AR,		0},
	{ "subi.l",	263,	AR,		0},
	{ "subi.w",	264,	AR,		0},
	{ "subq.b",	0,	AR,		0},
	{ "subq.l",	266,	AR,		0},
	{ "subq.w",	267,	AR,		0},
	{ "subx",	219,	AR,		0},
	{ "jcc",	0,	OC|AR,		1},
	{ "subx.b",	269,	AR,		0},
	{ "bge.s",	0,	OC|AR,		35},
	{ "subx.l",	271,	AR,		0},
	{ "bls",	0,	AR,		0},
	{ "subx.w",	273,	AR,		0},
	{ "blt",	65,	AR,		0},
	{ "tas",	236,	AR,		0},
	{ "ds.b",	0,	OC|AR,		19},
	{ "trapv",	0,	0,		0},
	{ "asl",	86,	AR,		0},
	{ "tst.l",	0,	AR,		0},
	{ "bvc",	0,	AR,		0},
	{ "tst.w",	281,	AR,		0},
	{ "bcs.s",	63,	OC|AR,		36},
	{ "unlk",	0,	AR,		0},
	{ NULL,		0,	0,		0},
	{ NULL,		0,	0,		0},
	{ "beq.s",	132,	OC|AR,		37},
	{ NULL,		0,	0,		0},
	{ "and.b",	163,	AR,		0},
	{ NULL,		0,	0,		0},
	{ "asr",	64,	AR,		0}
};

static char *openmsg = "Cannot open %s for input\n";
static char *usagemsg = "Usage: astrn [file]\n";
static char *longlinemsg = "Warning on line %d: truncated to %d characters\n";
static char *inclmsg = "Error on line %d: include not supported\n";
static char *aliasmsg = "Error on line %d: alias not supported\n";
static char *badcharmsg = "Warning on line %d: character in identifier not allowed\n";
static char *unrecmsg = "Warning on line %d: unrecognized opcode\n";
static char *syntaxmsg = "Error on line %d: syntax error prevents translation\n";
static char *pcmsg = "Warning on line %d: PC relative addressing used - check offset\n";

main(argc,argv) int argc; char *argv[];
{
	register char *inptr,*outptr,*tempptr,c;
	char *codeptr,longline,*src,*dst,c1,c2,*semiptr;
	char *listexpr(),*malloc();
	int line_no,length,exit(),free();
	short tableptr,i,tree,expr();

	if (argc == 1) infile = stdin;
	else if (argc == 2) {
		if ((infile = fopen(argv[1],"r")) == NULL) {
			fprintf(stderr,openmsg,argv[1]);
			exit(1);
		};
	} else {
		fprintf(stderr,usagemsg);
		exit(1);
	};

	outline = malloc((int)((MAXLINE+2)*MAXLINE/3));

	line_no = 0;
	longline = 0;
	tempptr = lastlabel;
	inptr = "ASM_";
	while (*tempptr++ = *inptr++);

	while (fgets(inline,MAXLINE + 2,infile) == inline) {

		/* Truncate lines to 132 characters */

		tempptr = inline;
		while (*tempptr++);
		if ((tempptr - inline == MAXLINE + 2) &&
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
		arg2 = NULL;
		inptr = inline;
		outptr = outline;
		
		/* Is this line a comment? */

		if (*inptr == '*') {
			fputc(' ',stdout);
			*inptr = '#';
			fputs(inline,stdout);
			goto loopend;
		};

		/* Search for semicolon */

		tempptr = inline;
		while (c = *tempptr++)
			if (c == ';') {
				globalflg |= SEMICOLON;
				semiptr = --tempptr;
				tempptr = &inline[MAXLINE + 2];
				while (tempptr > semiptr) 
					*tempptr-- = *(tempptr - 1);
				*tempptr++ = ' ';
				*tempptr = '\0';
			};

		/* Scan line for first non-whitespace */

		NEXTFIELD(inptr,outptr,c);
		if (!c) {	/* "blank" line? */
			*outptr++ = c;
			fputs(outline,stdout);
			goto loopend;
		};
		inptr--;

		/* Check for a label */

		if (inptr == inline) {
			globalflg |= LABEL;
			c = *inptr;
			/* check for local label */
			if (c <= '9' && c >= '0') {
				globalflg &= ~LABEL;
				globalflg |= LLABEL;
				tempptr = lastlabel;
				while (*outptr++ = *tempptr++);
				--outptr;
			};
			/* move label */
			while (charset[(unsigned char) (c = *inptr++)] & ID) {
				if (charset[(unsigned char) c] & IC) {
					globalflg |= BADIDCHAR;
					c = (c <= '?') ? 
					    (c == '$' ? 'S' : 'Q') :
					    (c == '@' ? 'A' : 'D');
				};
				*outptr++ = c;
			};
			*outptr++ = ':';
			/* whitespace must follow label */
			if (!(charset[(unsigned char) c] & WS)) goto syntaxerr;
			*outptr++ = c;
			/* find next non-whitespace */
			NEXTFIELD(inptr,outptr,c);
			/* Does line only contain a label? */
			if (!c) {
				*outptr++ = c;
				fputs(outline,stdout);
				goto loopend;
			};
			--inptr;
		};

		/* Search table for opcode mnemonic */

		codeptr = inptr;
		while (!(charset[(unsigned char) *inptr++] & WS));
		length = --inptr - codeptr;
		tempptr = codeptr;
		/* hash it into the table */
		tableptr = *tempptr++ << 3;
		if (length >= 2) {
			tableptr += *tempptr++ << 2;
			if (length >= 3) {
				tableptr += *tempptr++ << 1;
				if (length >= 4) tableptr += *tempptr;
			};
		};
		tableptr %= TABLESIZE;

		/* search linked list for correct mnemonic */
		while (table[tableptr].name[length] ||
		       strncmp(codeptr,table[tableptr].name,length))
			if (!(tableptr = table[tableptr].next)) {
				globalflg |= UNREC;
				if (globalflg & SEMICOLON) {
					tempptr = semiptr;
					*tempptr++ = ';';
					while (*tempptr++ = *(tempptr + 1));
					globalflg &= ~SEMICOLON;
				};
				fputs(inline,stdout);
				goto loopend;
			};

		/* Is this an unsupported opcode? */

		if (table[tableptr].action & US) {
			if (globalflg & SEMICOLON) {
				tempptr = semiptr;
				*tempptr++ = ';';
				while (*tempptr++ = *(tempptr + 1));
			};
			fputs(inline,stdout);
			switch (table[tableptr].parameter) {
			case 0:
				fprintf(stderr,inclmsg,line_no);
				break;
			case 1:
				fprintf(stderr,aliasmsg,line_no);
				break;
			};
			fclose(infile);
			free(outline);
			exit(1);
		};

		if (table[tableptr].action & SP) {
			switch (table[tableptr].parameter) {
			case 1: /* ds.w */
			case 2: /* ds.l */
				if (table[tableptr].parameter == 1)
					fputs("\tlalign\t2\n",stdout);
				else fputs("\tlalign\t4\n",stdout);
				tempptr = "space";
				while (*outptr++ = *tempptr++);
				--outptr;
				NEXTFIELD(inptr,outptr,c);
				if (!c) goto syntaxerr;
				tempptr = table[tableptr].parameter == 1 ?
					"2*(" : "4*(";
				while (*outptr++ = *tempptr++);
				--outptr;
				src = --inptr;
				dst = outptr;
				if (operands(&src,&dst) == -1) goto syntaxerr;
				inptr = src;
				outptr = dst;
				*outptr++ = ')';
				goto comment;
			case 3: /* even */
				tempptr = "lalign\t2";
				while (*outptr++ = *tempptr++);
				--outptr;
				goto comment;
			case 8: /* asciz */
			case 4: /* dc.b */
				tempptr = "byte";
				while (*outptr++ = *tempptr++);
				--outptr;
				NEXTFIELD(inptr,outptr,c);
				--inptr;
				for (;;) {
					c = *inptr++;
					if (c == 34 || c == 39) {
						c1 = c;
						*outptr++ = 34;
						while ((c = *inptr++) != c1) {
							if (!c) goto syntaxerr;
							if (c == 34) {
								*outptr++ = c;
								tempptr = ",34,";
								while (*outptr++ = *tempptr++);
								*(outptr - 1) = c;
							} else {
								if (c == '\\')
									*outptr++ = c;
								*outptr++ = c;
							};
						};
						*outptr++ = 34;
					} else {
						nextnode = 0;
						npoolptr = namepool;
						parenlevel = 0;
						src = --inptr;
						if ((tree = expr(&src)) == -1) 
							goto syntaxerr;
						inptr = src;
						outptr = listexpr(tree,outptr,-1,0);
					};
					if (charset[c = *inptr] & WS) {
						if (table[tableptr].parameter == 8) {
							*outptr++ = ',';
							*outptr++ = '0';
						};
						goto comment;
					};
					if (c != ',') goto syntaxerr;
					*outptr++ = *inptr++;
				};
			case 5: /* dc,dc.w */
				tempptr = "short";
				goto dc;
			case 6: /* dc.l */
				tempptr = "long";
			dc:
				while (*outptr++ = *tempptr++);
				--outptr;
				NEXTFIELD(inptr,outptr,c);
				--inptr;
				for (;;) {
					c = *inptr++;
					if (c == 34 || c == 39) {
						c1 = c;
						while ((c = *inptr++) != c1) {
							if (!c) goto syntaxerr;
							*outptr++ = '0';
							*outptr++ = 'x';
							c2 = (0xF & c >> 4) + '0';
							if (c2 > '9') c2 += 7;
							*outptr++ = c2;
							c2 = (0xF & c) + '0';
							if (c2 > '9') c2 += 7;
							*outptr++ = c2;
							*outptr++ = ',';
						};
						--outptr;
					} else {
						nextnode = 0;
						npoolptr = namepool;
						parenlevel = 0;
						src = --inptr;
						if ((tree = expr(&src)) == -1) 
							goto syntaxerr;
						inptr = src;
						outptr = listexpr(tree,outptr,-1,0);
					};
					if (charset[c = *inptr] & WS)
						goto comment;
					if (c != ',') goto syntaxerr;
					*outptr++ = *inptr++;
				};
				case 7: /* equ */
					if (!(globalflg & (LABEL | LLABEL))) 
						goto syntaxerr;
					globalflg &= ~LABEL;
					tempptr = "set";
					while (*outptr++ = *tempptr++);
					--outptr;
					NEXTFIELD(inptr,outptr,c);
					tempptr = outline;
					while ((c = *tempptr) != ':') {
						*outptr++ = c;
						*tempptr++ = ' ';
					};
					while (tempptr < outptr)
						*tempptr++ = *(tempptr + 1);
					*(outptr - 1) = ',';
					nextnode = 0;
					npoolptr = namepool;
					parenlevel = 0;
					src = --inptr;
					if ((tree = expr(&src)) == -1) 
						goto syntaxerr;
					inptr = src;
					outptr = listexpr(tree,outptr,-1,0);
					if (!charset[c = *inptr] & WS)
						goto syntaxerr;
					goto comment;
				case 9: /* dnttentry */
					tempptr = "dnt_";
					while (*outptr++ = *tempptr++);
					--outptr;
					tempptr = inptr;
					while (charset[*tempptr++] & WS);
					--tempptr;
					if (!(c1 = *tempptr++)) goto syntaxerr;
					if (!(c2 = *tempptr++))	goto syntaxerr;
					c1 -= '0';
					if (c2 != ',') {
						if (*tempptr != ',') 
							goto syntaxerr;
						c1 *= 10;
						c1 += c2 - '0';
					};
					if (c1 < 0 || c1 > 26 || c1 == 11)
						goto syntaxerr;
					tempptr = dntt_table[c1];
					while (*outptr++ = *tempptr++);
					--outptr;
					NEXTFIELD(inptr,outptr,c);
					while (*inptr++ != ',');
					goto slt;
				case 10: /* sltnormal,sltspecial */
					tempptr = table[tableptr].name;
					while (*outptr++ = *tempptr++);
					outptr--;
					NEXTFIELD(inptr,outptr,c);
					if (!c) goto syntaxerr;
					--inptr;
				slt:	for (;;) {
						if (charset[c = *inptr++] & WS) {
							--inptr;
							goto comment;
						};
						if (c == 34) {
							do {
								if (!(*outptr++ = c)) goto syntaxerr;
								c = *inptr++;
							} while (c != 34);
							*outptr++ = c;
						} else *outptr++ = c;
					};
			};
		};

		/* Does the opcode mnemonic have to be changed? */

		if (table[tableptr].action & OC) 
			tempptr = optable[table[tableptr].parameter];
		else
			tempptr = table[tableptr].name;
		while (*outptr++ = *tempptr++); /* transfer mnemonic */
		outptr--;
		
		/* Are there one or more arguments? */

		if (table[tableptr].action & AR) {
			/* search for argument field */
			NEXTFIELD(inptr,outptr,c);
			if (!c) goto syntaxerr;
			src = --inptr;
			dst = outptr;
			/* translate arguments */
			if (operands(&src,&dst) == -1) goto syntaxerr;
			/* do the arguments have to be swapped? */
			if (table[tableptr].action & SW) {
				if (arg2 == NULL) goto syntaxerr;
				inptr = outptr;
				tempptr = chartemp;
				for (i = arg2 - outptr - 1; i > 0; i--)
					*tempptr++ = *inptr++;
				tempptr = outptr;
				++inptr;
				for (i = dst - arg2; i > 0; i--)
					*tempptr++ = *inptr++;
				*tempptr++ = ',';
				inptr = chartemp;
				for (i = arg2 - outptr - 1; i > 0; i--)
					*tempptr++ = *inptr++;
			};
			inptr = src;
			outptr = dst;
		};

		/* Search for any comments */

		comment: 
		NEXTFIELD(inptr,outptr,c);
		if ((globalflg & SEMICOLON) && ((inptr - 2) <= semiptr)) {
			tempptr = semiptr;
			*tempptr++ = ';';
			while (*tempptr++ = *(tempptr + 1));
			globalflg &= ~SEMICOLON;
			if (inptr > semiptr) {
				inptr = semiptr + 1;
				c = *inptr++;
			};
		};
		if (c) { /* precede comments with a '#' */
			*outptr++ = '#';
			*outptr++ = c;
			while (*outptr++ = *inptr++);
		} else *outptr++ = c;

		fputs(outline,stdout);

		loopend: 
			if (globalflg & SEMICOLON) {
				tempptr = semiptr;
				*++tempptr = '#';
				fputs(tempptr,stdout);
			};
			if (longline) 
				fprintf(stderr,longlinemsg,line_no,MAXLINE);
			if (globalflg) {
				if (globalflg & BADIDCHAR)
					fprintf(stderr,badcharmsg,line_no);
				if (globalflg & UNREC)
					fprintf(stderr,unrecmsg,line_no);
				if (globalflg & PCREL) 
					fprintf(stderr,pcmsg,line_no);
				if (globalflg == LABEL) {
					inptr = outline;
					tempptr = lastlabel;
					while ((c = *inptr++) != ':') 
						*tempptr++ = c;
					*tempptr = 0;
				};
			};
	};
	fclose(infile);
	free(outline);
	exit(0);

	syntaxerr: 
		if (globalflg & SEMICOLON) {
			tempptr = semiptr;
			*tempptr++ = ';';
			while (*tempptr++ = *(tempptr + 1));
		};
		fputs(inline,stdout);
		fprintf(stderr,syntaxmsg,line_no);
		fclose(infile);
		free(outline);
		exit(1);
}

int operands(src,dest) register char **src,**dest;
{
	int exprchk();
	short expr(),tree;
	register char c,*sptr = *src,*dptr = *dest;
	char *ptr, *listexpr();

	for (;;) {
		switch(exprchk(sptr)) {
		case 2:	/* immediate operand */
			*dptr++ = '&';
			sptr++;
		case 0: /* expression */
			nextnode = 0;
			npoolptr = namepool;
			parenlevel = 0;
			ptr = sptr;
			if ((tree = expr(&ptr)) == -1) return -1;
			sptr = ptr;
			dptr = listexpr(tree,dptr,-1,0);
			if (*sptr == '(') {
				*dptr++ = *sptr++;
				do {
					c = *sptr++;
					if (charset[(unsigned char) c] & RG) {
						*dptr++ = '%';
						*dptr++ = c;
						if (c == 'p') {
							if ((*dptr++ = *sptr++) == 'c') globalflg |= PCREL;
						} else *dptr++ = *sptr++;
					} else *dptr++ = c;
					if (!c) return -1;
				} while (c != ')');
			};
			if ((*dptr++ = *sptr++) == ',') {
				arg2 = dptr;
				continue;
			};
			*src = --sptr;
			*dest = --dptr;
			return 0;
		case 1: /* argument other than expression */
			for (;;) {
				while ((c = *sptr++) < 'a' || c > 'z') 
					*dptr++ = c;
				*dptr++ = '%';
				do {
					*dptr++ = c;
				} while (!(charset[(unsigned char) (c = *sptr++)] & RT));
				if (charset[(unsigned char) c] & WS) {
					*src = --sptr;
					*dest = dptr;
					return 0;
				};
				*dptr++ = c;
				if (c == ',') break;
			};
			arg2 = dptr;
			continue;
		case 3: /* argument is "ccr" */
			sptr += 3;
			*dptr++ = '%';
			*dptr++ = 'c';
			*dptr++ = 'c';
			if (charset[(unsigned char) (c = *sptr++)] & WS) {
				*src = --sptr;
				*dest = dptr;
				return 0;
			};
			*dptr++ = c;
		};
	};
}

short expr(p) char **p;
{
	register char c,*ptr = *p;
	short head = -1, snode, count;
	char d,*idstart;

	for (;;) {
		/* Scan for unary operators */
		while (charset[(unsigned char) (c = *ptr++)] & UO)
			if (c != '+') {
				node[nextnode].token = c;
				node[nextnode].prec = 7;
				node[nextnode].left = head;
				head = nextnode;
				node[nextnode++].right = -1;
			};
		/* Subexpression in parentheses? */
		if (c == '(') {
			parenlevel++;
			*p = ptr;
			if ((snode = expr(p)) == -1) return -1;
			if (head != -1) node[head].right = snode;
			else head = snode;
			ptr = *p;
			parenlevel--;
		} else {
			/* Current pc location? */
			if (c == '*') {
				node[nextnode].ident = npoolptr;
				*npoolptr++ = '.';
				globalflg |= PCREL;
			/* identifier or constant? */
			} else if (charset[(unsigned char) c] & ID) {
				node[nextnode].ident = npoolptr;
				/* local label? */
				if (c >= '0' && c <= '9') {
					idstart = ptr;
					while ((c = *ptr++) >= '0' && c <= '9');
					if (c == '$') *npoolptr++ = ' ';
					ptr = idstart;
				};
				--ptr;
				/* save identifier or constant */
				while (charset[(unsigned char) (c = *ptr++)] & ID) {
					if (charset[(unsigned char) c] & IC) {
						globalflg |= BADIDCHAR;
						c = (c <= '?') ? 
						    (c == '$' ? 'S' : 'Q') :
						    (c == '@' ? 'A' : 'D');
					};
					*npoolptr++ = c;
				};
				--ptr;
			/* single quoted string? */
			} else if (c == 39) {
				node[nextnode].ident = npoolptr;
				*npoolptr++ = '0';
				*npoolptr++ = 'x';
				count = 0;
				/* convert up to 4 chars to hex format */
				while ((c = *ptr++) != 39) {
					if (++count > 4) return -1;
					d = ((c >> 4) & 0xF) + '0';
					if (d > '9') d += 7;
					*npoolptr++ = d;
					d = (c & 0xF) + '0';
					if (d > '9') d += 7;
					*npoolptr++ = d;
				};
			} else return -1;
			*npoolptr++ = 0;
			node[nextnode].token = 0;
			node[nextnode].prec = 9;
			node[nextnode].left = -1;
			node[nextnode].right = -1;
			if (head != -1) node[head].right = nextnode++;
			else head = nextnode++;
		};
		/* Is next token a binary operator? */
		if (charset[(unsigned char) (c = *ptr++)] & BO) {
			/* Convert % to @ and ! to | */
			if (c == '%') c = '@';
			else if (c == '!') c = '|';
			node[nextnode].token = c;
			/* Define operator precedence */
			node[nextnode].prec = c == '+' || c == '-' ? 5 :
				(c == '<' || c == '>' ? 4 : (c == '&' ? 3 :
				(c == '^' ? 2 : (c == '|' ? 1 : 6))));
			node[nextnode].left = head;
			head = nextnode;
			node[nextnode++].right = -1;
		/* End of subexpression in parentheses? */
		} else if (c == ')') {
			if (!parenlevel) return -1;
			*p = ptr;
			return head;
		/* End of total expression? */
		} else if (charset[(unsigned char) c] & WS || c == ',' || c == '(') {
			if (parenlevel) return -1;
			*p = --ptr;
			return head;
		} else return -1;
	};
}

char *listexpr(nptr,dptr,lastprec,rflg) short nptr;
register char *dptr; int lastprec,rflg;
{
	register char *sptr;
	short paren;

	/* in order tree traversal */

	paren = rflg ? ((node[nptr].prec <= lastprec) ? 1 : 0) :
		       ((node[nptr].prec < lastprec) ? 1 : 0);
	if (paren && lastprec == 7 && node[nptr].right == (char) -1) paren = 0;
	if (paren) *dptr++ = '(';
	if (node[nptr].left != (char) -1) /* left child? */
		dptr = listexpr(node[nptr].left,dptr,node[nptr].prec,0);
	if (!(*dptr++ = node[nptr].token)) {
		--dptr;
		sptr = node[nptr].ident;
		if (*sptr == ' ') { /* special case for local labels */
			if (globalflg & LABEL) {
				sptr = outline;
				while ((*dptr++ = *sptr++) != ':');
			} else {
				sptr = lastlabel;
				while (*dptr++ = *sptr++);
			};
			--dptr;
			sptr = node[nptr].ident + 1;
		};
		while (*dptr++ = *sptr++);
		--dptr;
	};
	if (node[nptr].right != (char) -1) /* right child? */
		dptr = listexpr(node[nptr].right,dptr,node[nptr].prec,1);
	if (paren) *dptr++ = ')';
	return dptr;
}

int exprchk(ptr) register char *ptr;
{
	register char c;
	char *start = ptr;
	int retval;
	
	/* Determine type of expression */

	if ((c = *ptr++) == '#') return 2;	/* immediate operand? */
	if ((c == 'a' || c == 'd') &&
	   ((c = *ptr++) >= '0' && c <= '7') &&
	   (charset[(unsigned char) *ptr] & RT)) return 1; /* direct reg? */
	/* indirect register (including predecrement & postincrement) */
	else if (c == '-' && *ptr++ == '(' || c == '(') {
		/* must be 'a' register or stack pointer */
		if ((c = *ptr++) == 'a') { 
			if ((c = *ptr++) >= '0' && c <= '7' && *ptr == ')') 
				return 1;
		} else if (c == 's' && *ptr++ == 'p' && *ptr == ')')
				return 1;
	};
	ptr = start;
	retval = 0;
	if ((c = *ptr++) == 'c') { /* ccr? */
		if (*ptr++ == 'c' && *ptr++ == 'r') retval = 3;
	} else if (c == 'd') {  /* dfc? */
		if (*ptr++ == 'f' && *ptr++ == 'c') retval = 1;
	} else if (c == 'v') {	/* vbr? */
		if (*ptr++ == 'b' && *ptr++ == 'r') retval = 1;
	} else if (c == 'u') {  /* usp? */
		if (*ptr++ == 's' && *ptr++ == 'p') retval = 1;
	} else if (c == 's')  { /* sp, sr, or sfc? */
		if ((c = *ptr++) == 'p' || c == 'r') retval = 1;
		else if (c == 'f' && *ptr++ == 'c') retval = 1;
	};
	/* check for proper terminator */
	if (retval && (charset[(unsigned char) (c = *ptr)] & WS || c == ',')) 
		return retval;
	else return 0;
}
