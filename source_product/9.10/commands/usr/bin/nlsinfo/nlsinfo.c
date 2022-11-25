static char *HPUX_ID = "@(#) $Revision: 70.3 $";

/*
**       The nlsinfo command displays table information associated
**       with a given locale.  These tables are used to control
**       language sensitive operations in commands and libraries.
**       The locale is determined by a call to setlocale or entered
**       directly by the -L locale option.  The major features of the
**       command include:
**
**             1.  Table information displayed by setlocale(LIBC) categories.
**             2.  A list of installed locales.
**             3.  A list of characters defined in the code set
**                 associated with a language.
**
**       Nlsinfo data is displayed as fields of characters, strings,
**       numbers or descriptions.  Graphic characters in the
**       character fields are shown as fonts in the operative
**       character set.  Non-printable characters are shown as hex or
**       octal numbers.  With the appropriate option, all characters
**       can be shown as hex or octal numbers.  Strings are delimited
**       by double-quotes (see the local customs data displays).
**       When characters within strings are shown as hex or octal
**       numbers, they are preceded by backslash 'x' (\x) or
**       backslash (\) respectively.  Numbers are displayed as
**       decimal digits.  Item numbers mentioned below in the local
**       customs display sections refer to the integers associated
**       with custom strings by the macros in langinfo.h.  The item
**       numbers are not displayed.  Descriptions are phrases that
**       describe what the character and string fields mean.
**
**       Headings are printed about every 16 lines by default so the
**       user won't lose track of what the columns mean.  The number
**       of lines between headings can be configured by an option.
**       All displays assume an 80 character output.
**
**       All descriptions and phrases that describe what the
**       character and string fields mean are set up in message
**       catalogs.  In addition, the format that determines the look
**       of the display is included in the message catalog also.
*/

/*
***************************************************************************
*** include files
***************************************************************************
*/

#include <stdio.h>		/* input - output */
#include <varargs.h>		/* variable arguments */
#include <ctype.h>		/* character classification */
#include <string.h>		/* string function declarations */
#include <nl_types.h>		/* for nl_catd */
#include <nl_ctype.h>		/* for 16-bit macros */
#include <langinfo.h>		/* for local customs */
#include <locale.h>		/* for setlocale & localeconv */
#include <limits.h>		/* for UINT_MAX */
#include <fcntl.h>              /* for O_RDONLY */
#include <collate.h>		/* collation types */
#include <setlocale.h>		/* setlocale types */

/*
***************************************************************************
*** external declarations
***************************************************************************
*/

extern nl_catd catopen();	/* open message catalog */
extern char *catgets();		/* get message from catalog */
extern int catclose();		/* close message catalog */
extern char *_errlocale();	/* get bad locle settings */
extern char *nl_langinfo();	/* get local custom information */
extern char *calloc();		/* allocate memory */
extern void perror();		/* system error messages */
extern void exit();		/* leave */
extern char *optarg;		/* pointer to start of option arg */
extern int optind;		/* argv index of next arg */
extern int opterr;		/* error message indicator */
extern int errno;		/* error number */
extern int sys_nerr;		/* maximum error number */
extern int __nl_char_size;	/* max bytes in character */

/*
***************************************************************************
*** forward references
***************************************************************************
*/

extern void Printf();		/* local system print routine */
extern void Perror();		/* local system print error message */
extern void error();		/* local system error message */
extern unsigned char *svis();	/* make string visable */
extern unsigned char *nvis();	/* make number visable */
extern unsigned char *tostr();	/* make a decimal number visable */
extern char *get_basename();	/* get basename of command name */

extern int list_lang();		/* list installed language */
extern int lc_all();		/* LC_ALL */
extern int lc_ctype();		/* LC_CTYPE */
extern int lc_collate();	/* LC_COLLATE */
extern int lc_numeric();	/* LC_NUMERIC */
extern int lc_time();		/* LC_TIME */
extern int lc_monetary();	/* LC_MONETARY */
extern int other();		/* other misc info */
extern int code_set();		/* code set characters */
extern int list_conv();		/* list installed code set conversion names */

/*
***************************************************************************
*** generic types
***************************************************************************
*/

typedef unsigned char UCHAR;	/* prevent sign extention on char */
typedef char *  CP;		/* ptr to char */
typedef UCHAR *  UCP;		/* ptr to UCHAR */
typedef unsigned int UINT;	/* prevent sign extention on int */
typedef UINT *  UIP;		/* ptr to UINT */
typedef int (*PFI) ();		/* ptr to function returning int type */

/*
***************************************************************************
*** locale types
***************************************************************************
*/

typedef struct table_header TABHDR; /* locale.def table header */
typedef struct catinfotype CATMOD; /* locale.def category/modifier structure */

/*
***************************************************************************
*** lconv types
***************************************************************************
*/

typedef struct lconv LCONV;	/* type for lconv structure */

typedef union {			/* things that are in lconv structure */
	char *str;
	char chr[2];
} LCONVELE;

typedef struct {	/* lconv structure display information */
	UINT flag;		/* display info? TRUE/FALSE */
	UINT type;		/* kind of element (char or char *) */
	LCONVELE ele;		/* the lconv element */
} LCONVINFO;

/*
***************************************************************************
*** collation types
***************************************************************************
*/

typedef struct col_21tab TAB21; /* 2-to-1 table structure */
typedef struct col_12tab TAB12; /* 1-to-2 table structure */

typedef struct cle {	/* collation link list structure */
	UCHAR type;		/* 0: normal, 1: 2to1, 2: 1to2 */
	UCHAR ch1;		/* character code */
	UCHAR seq1;		/* sequence number */
	UCHAR ch2;		/* 2nd character code if 1-to-2 or 2-to-1 */
	UCHAR seq2;		/* seqence number of 2nd character code */
	UCHAR first_ch;		/* for 1to2 cases, eg AE: first_ch=A */
	UCHAR pri;		/* priority */
	struct cle *next;
} COLL_LIST;

/*
*****************************************************************************
** jump table type
*****************************************************************************
*/

typedef struct {	/* jump table type */
	PFI routine;		/* routine to process option */
	UINT flag;		/* execute if TRUE */
} JMPTAB;

/*
***************************************************************************
*** generic constants
***************************************************************************
*/

#define	WARNING		0	/* warning error message */
#define	FATAL		1	/* fatal error message */
#define	GOOD		0	/* successful return value */
#define	BAD		-1	/* unsuccessful return value */
#define	TRUE		1	/* boolean true */
#define	FALSE		0	/* boolean false */

/*
***************************************************************************
*** limits
***************************************************************************
*/

#define	MAX_FNAME	(UINT) 256	/* maximum length of file name */
#define	MAX_PNAME	(UINT) 1024	/* maximum length of path name */
#define	MAX_LANG	MAX_FNAME	/* maximum length of language name */
#define	MAX_ERR		(UINT) 256	/* maximum length of Perror message */
#define	LCONV_SIZE	(UINT) 18	/* number of elements in lconv struct */
#define MAX_2TO1	(UINT) 30	/* max number of 2to1 characters in codeset */
#define MAX_2CHAR	(UINT) 0xffff	/* max 2-byte code set value */
#define MIN_2CHAR	(UINT) 0x8000	/* min 2-byte code set value */
#define MAX_1CHAR	(UINT) 0xff	/* max 1-byte code set value */
#define MIN_1CHAR	(UINT) 0x00	/* min 1-byte code set value */
#define MAX_1PLUS	(UINT) MAX_1CHAR+1	/* max 1-byte code set value  + 1 */

/*
***************************************************************************
*** non-printable character display
***************************************************************************
*/

#define	CHR		0
#define	HEX		1
#define	OCT		2
#define	DEC		3
#define	STR		4

/*
***************************************************************************
*** locale table constants
***************************************************************************
*/

#define HDR_SIZE	sizeof( TABHDR) 
#define TBL_SIZE	256
#define NLSDIR		"/usr/lib/nls/"
#define LOCALE		"/locale.inf"
#define CONFIG		"/config"

/*
***************************************************************************
*** conversion table constants
***************************************************************************
*/

#define ICONV		"/usr/lib/iconv/direct/"

/*
***************************************************************************
*** collation table constants
***************************************************************************
*/

#define ELSIZE		sizeof( COLL_LIST)	/* coll link list ele size */
#define NELEM		256 + MAX_2TO1		/* max num of link list ele */
#define ENDTABLE	0377			/* end mark of 2-1 character */

/* the two high bits in the priority field contain the character type */
#define	TNORMAL		0
#define	TTWOTO1		1
#define	TONETO2		2
#define	DONTCARE	3

/*
*****************************************************************************
** jump table indexes
*****************************************************************************
*/

#define List_lang	0	/* list installed language */
#define Lc_all		1	/* LC_ALL */
#define Lc_ctype	2	/* LC_CTYPE */
#define Lc_collate	3	/* LC_COLLATE */
#define Lc_numeric	4	/* LC_NUMERIC */
#define Lc_time		5	/* LC_TIME */
#define Lc_monetary	6	/* LC_MONETARY */
#define Other		7	/* other misc info */
#define Code_set	8	/* code set characters */
#define List_conv	9	/* list installed code set conversions names */

/*
***************************************************************************
*** message catalog numbers
***************************************************************************
*/

#define	ERR		10	/* error() */
#define	CTY		100	/* ctype() */
#define	CON		200	/* conv() */
#define	NLT		300	/* nl_tools_16() */
#define	CL1		400	/* collate1() */
#define	CL2		500	/* collate2() */
#define	TIM		600	/* lc_time() */
#define	MON		700	/* lc_monetary() */
#define NUM		800	/* lc_numeric() */
#define	OTH		900	/* other() */
#define	COD		1000	/* code_set() */
#define	LIS		1100	/* list_lang() */
#define	DSP		1200	/* disp_posn() */
#define	DSC		1300	/* disp_cs() */
#define	DSS		1400	/* disp_sep_space() */
#define	DSG		1500	/* disp_group() */
#define	LIC		1600	/* list_conv() */
#define	CCS		1700	/* cytpe_customs() */

/*
***************************************************************************
*** error message numbers
***************************************************************************
*/

#define	NL_SETN		1	/* message catalog set number */

#define BAD_USAGE	1	/* usage error message */ 
#define BAD_HLINES	2	/* bad number of line between headings */
#define BAD_LANG	3	/* unable to load locale */
#define BAD_LOWER	4	/* bad lower bound */
#define BAD_UPPER	5	/* bad upper bound */
#define BAD_NONPRINT	6	/* bad numeric representation of characters */
#define BAD_CONFIG	7	/* no config file */
#define BAD_INSTALL	8	/* language configured but not installed */

#define BAD_ROUTINE	10	/* trouble with a routine
				   ... must be last error message number */

/*
**************************************************************************
** error message strings
**************************************************************************
*/

static CP Message[] = {
	"COMMENT: error message strings", 						/* catgets 10 */
	"usage: %s [-acfhilmnstC] [-d n] [-e n] [-o n] [-r n1 n2] [-L language]\n",	/* catgets 11 */
	"number of lines between headings must be greater than zero\n",			/* catgets 12 */
	"unable to load locale \"%s\"\n",						/* catgets 13 */
	"invalid lower bound value\n",							/* catgets 14 */
	"invalid upper bound value\n",							/* catgets 15 */
	"option must be followed by \"o\" or \"h\" not \"%c\"\n",			/* catgets 16 */
	"\"/usr/lib/nls/config\" not installed\n",					/* catgets 17 */
	"language \"%s\" configured but not installed\n",				/* catgets 18 */
	"",										/* catgets 19 */

	"problems with list installed language option (-l)\n",				/* catgets 20 */
	"problems with LC_ALL option (-a)\n",						/* catgets 21 */
	"problems with LC_CTYPE option (-c)\n",						/* catgets 22 */
	"problems with LC_COLLATE option (-s)\n",					/* catgets 23 */
	"problems with LC_NUMERIC option (-n)\n",					/* catgets 24 */
	"problems with LC_TIME option (-t)\n",						/* catgets 25 */
	"problems with LC_MONETARY option (-m)\n",					/* catgets 26 */
	"problems with other customs option (-h)\n",					/* catgets 27 */
	"problems with list code set option (-C)\n",					/* catgets 28 */
	"problems with list conversion code set names option (-i)\n",			/* catgets 29 */
	};

/*
*****************************************************************************
** Some general purpose macros
*****************************************************************************
*/

/* point to the right format string based on the character */
#define PTFMT( i, isch)	\
	i = (Number || !isch) ? (Nonprint == HEX) ? HEX : OCT : CHR;

/* put out the title (assumes 1st string in format array is a comment) */
#define TITLE( fmt, messnum, i, flag) \
	putchar( flag ? Formfeed ? '\f' : '\n' : '\n'); \
	Printf( fmt, messnum, i+1); \
	Printf( fmt, messnum, i); \
	Printf( fmt, messnum, i+1);

/* if necessary put out the header */
#define HEADER( lines, fmt, messnum, i) \
	if ( ! (lines++ % Hlines) ) { \
		putchar( '\n'); \
		Printf( fmt, messnum, i+1); \
		Printf( fmt, messnum, i); \
		Printf( fmt, messnum, i+1); \
	}

/* put 2 separate bytes into a single multi-byte character */
#define CH2( i, j)	((i << 8) | j)

/* separte 1 2-byte character into 2 single-byte characters */
#define	SEP2( ch, ch1, ch2)	((ch1 = (ch >> 8) & 0xff), (ch2 = ch & 0xff));

/* TRUE if at end of collation link list */
#define COLL_LAST(p)	(p->next &&					\
			((p->seq1 < lastseq1) ||			\
			((p->seq1 == lastseq1) && (p->pri <= lastpri))))

/* number of elements in an array */
#define SIZE( array, type)	sizeof( array) / sizeof( type)

/*
*****************************************************************************
** global variables
*****************************************************************************
*/

static char *Error;		/* routine error message */
static char *Progname;		/* program name */
static nl_catd Catd;		/* message catalog descriptor */
static int Nonprint = HEX;	/* non-print characters as hex or octal */
static int Number = FALSE;	/* display all characters as hex or octal */
static int Lower = MIN_1CHAR;	/* element number lower bound */
static int Upper = MAX_1CHAR;	/* element number upper bound */
static int Hlines = 16;		/* lines between headings */
static int Formfeed = FALSE;	/* form feed before heading */
static int Everything = TRUE;	/* for lc_collate, all characters */
static int Lflag = FALSE;	/* for list_lang, one language */
static UCHAR Lang[MAX_LANG];	/* -L language option argument */
static COLL_LIST *Ptop;		/* start of collation linked list */
static char CMAX[] = { CHAR_MAX, '\0'};
				/* string version of CHAR_MAX */
static UCHAR SP[] = { ' ', '\0'};
				/* string version of space */

/*
**************************************************************************
** main()
**
** description:
**	driver routine for program
**
** assumptions:
**	all output goes to stdout
**	all errors go to stderr
** 
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

int
main(argc, argv)
int argc;			/* initial argument count */
char **argv;			/* ptr to ptr to first program argument */
{
	/* the jump table */
	static JMPTAB jtab[] = {
		{ list_lang,	FALSE },	/* list installed language */
		{ lc_all,	FALSE },	/* LC_ALL */
		{ lc_ctype,     FALSE },        /* LC_CTYPE */
		{ lc_collate,	FALSE },	/* LC_COLLATE */
		{ lc_numeric,	FALSE },	/* LC_NUMERIC */
		{ lc_time,	FALSE },	/* LC_TIME */
		{ lc_monetary,	FALSE },	/* LC_MONETARY */
		{ other,	FALSE },	/* other customs */
		{ code_set,	FALSE },	/* code set */
		{ list_conv,	FALSE },	/* list installed conversion code set names */
		{ (PFI) NULL,	FALSE },	/* must be last entry in jump table */
	};

	JMPTAB *j;				/* ptr to jump table routine */

	/* assume a sucessful return value*/
	int retval = GOOD;

	/* initialize, parse cmd line options, set up jump table */
	if (start( argc, argv, &jtab[0]) == BAD) {
		retval = BAD;
	}

	/* display native language support information */
	for (j = &jtab[0] ; j->routine ; j++) {
		if ( j->flag ) {
			if ((*j->routine)( ) == BAD) {
				error( WARNING, BAD_ROUTINE + (j - &jtab[0]));
				retval = BAD;
			}
		}
	}

	/* end the program */
	if (finish( ) == BAD) {
		retval = BAD;
	}

	return retval;
}

/*
**************************************************************************
** start() 
**
** description:
**	set up language tables
**	open message catalogs
**	parse command line
**	initialze global variables
**	set up jump table flags
** 
** global variables:
**	Catd: nl_catd message catalog descriptor
**	Progname: char pointer to the program name
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
start( argc, argv, j)
int argc;				/* current argument count */
char **argv;				/* ptr to ptr to current argument */
JMPTAB *j;				/* ptr to routine jump table */
{
	int optchar;			/* option character */
	UINT disp_opt = FALSE;		/* display flag */
	UINT num;			/* numeric option argument */

	/* get the program name */
	Progname = get_basename( *argv);

	/* get language & initialize environment table */
	if (!setlocale( LC_ALL, "")) {
		/* bad initialization */
		(void) fputs( _errlocale(), stderr);
		Catd = (nl_catd) -1;
		putenv( "LANG=");	/* for perror */
	}
	else {
		/* good initialization: open message catalog,
		   ... keep on going if it isn't there */
		Catd = catopen( Progname, 0);
	}

	/* parse command line options */

	opterr = 0;		/* disable getopt error message */
	while ((optchar = getopt( argc, argv, "lacsntmhifCvd:e:o:r:L:")) != EOF) {
		switch (optchar) {
		case 'l':
			j[List_lang].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'a':
			j[Lc_all].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'c':
			j[Lc_ctype].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 's':
			j[Lc_collate].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'n':
			j[Lc_numeric].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 't':
			j[Lc_time].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'm':
			j[Lc_monetary].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'h':
			j[Other].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'i':
			j[List_conv].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'C':
			j[Code_set].flag = TRUE;
			disp_opt = TRUE;
			break;
		case 'e':	/* printable & non-printable chars as numbers */
			if ((num = CHARAT( optarg)) == 'o') {
				Nonprint = OCT;
				Number = TRUE;
			}
			else if (num == 'h') {
				Nonprint = HEX;
				Number = TRUE;
			}
			else {
				error( WARNING, BAD_NONPRINT, num);
			}
			break;
		case 'f':	/* form feed before headings */
			Formfeed = TRUE;
			break;
		case 'd':	/* lines between headings */
			if (get_num( optarg, &num) == BAD) {
				error( WARNING, BAD_HLINES);
			}
			else if (num == 0) {
				error( WARNING, BAD_HLINES);
			}
			else {
				Hlines = num;
			}
			break;
		case 'o':	/* numeric representation of characters */
			if ((num = CHARAT( optarg)) == 'o') {
				Nonprint = OCT;
			}
			else if (num == 'h') {
				Nonprint = HEX;
			}
			else {
				error( WARNING, BAD_NONPRINT, num);
			}
			break;
		case 'r':	/* table element number range */
			if (get_num( optarg, &num) == BAD) {
				error( FATAL, BAD_LOWER);
			}
			if (num > MAX_2CHAR) {
				error( FATAL, BAD_LOWER);
			}
			else {
				Lower = num;
			}
			optarg = argv[optind++];
			if (get_num( optarg, &num) == BAD) {
				error( FATAL, BAD_UPPER);
			}
			if (num < Lower || num > MAX_2CHAR) {
				error( FATAL, BAD_UPPER);
			}
			else {
				Upper = num;
			}
			Everything = FALSE;
			break;
		case 'L':	/* language name */
			(void) strncpy( Lang, optarg, MAX_LANG);
			Lflag = TRUE;
			break;
		case 'v':	/* undocumented: dispaly summary of options */
			if (show_options() == BAD) {
				return BAD;
			}
			disp_opt = TRUE;
			break;
		case '?':	/* unrecognized option */
			error( FATAL, BAD_USAGE, Progname);
		}
	}

	/* should be nothing else left on the command line */
	if ((argc - optind) != 0) {
		error( FATAL, BAD_USAGE, Progname);
	}
	
	/* if none of the options l, a, c, s, n, t, m, h, y or z are specified
	   ... use l as default */

	if (!disp_opt) {
		j[List_lang].flag = TRUE;
	}

	/* get locale of the designated language
	   Note that this has to be done after parsing the command line
	   since option arguments must be parsed with the runtime locale.
	   Also notice that this should not affect error messages since
	   as of yet there is no LC_MESSAGES catagory. */

	if (Lflag) {
		if (! setlocale( LC_ALL, Lang)) {
			error( FATAL, BAD_LANG, Lang);
		}
	}

	return GOOD;
}

/*
**************************************************************************
** get_basename() 
**
** description:
**	get the basename of the command
**
** return value:
**	ptr to start of base name
**************************************************************************
*/

static char *
get_basename( p)
char *p;			/* ptr to start of command name */
{
	char *slash;		/* pointer to char after slash */

	for (slash = p ; *p ; ADVANCE( p)) {
		if (CHARAT( p) == '/') {
			slash = p + 1;
		}
	}

	return slash;
}

/*
**************************************************************************
** show_options() 
**
** description:
**	display nlsinfo options
**	undocumented features
** 
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
show_options( )
{
	static CP list_options[] = {
		"-l: list of installed languages",
		"-a: LC_ALL locale  category",
		"-c: LC_CTYPE locale category",
		"-s: LC_COLLATE locale category",
		"-n: LC_NUMERIC locale category",
		"-t: LC_TIME locale category",
		"-m: LC_MONETARY locale category",
		"-h: other local custom information",
		"-i: conversion code set names",
		"-C: code set characters",
		"-f: form feed before all display headings",
		"-d n: n lines between headings",
		"-e n: all characters as numbers",
		"-o n: non-printable characters as numbers",
		"-r n1 n2: character range",
		"-L language: select locale associated with language",
		"-v: show summary of options",
	};
	UINT i;
	UINT n = SIZE( list_options, CP );

	for (i=0 ; i < n ; i++) {
		if (puts( list_options[i]) == BAD) {
			Perror( "show_options -- puts");
			return BAD;
		}
	}
	return GOOD;
}

/*
**************************************************************************
** get_num() 
**
** description:
**	get a number from command line
**	number can be hex, octal, decimal constant
**	or a character in current code set
** 
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
get_num( p, result)
UCP p;
UIP result;
{
	UINT base;
	UINT num;
	UINT digit;
	UINT c;

	/* handle the sign if any */
	if (*p == '-') {
		/* no negative number allowed */
		return BAD;
	}
	else if (*p == '+') {
		/* skip plus sign */
		p++;
	}

	if (!FIRSTof2( *p) && isdigit( *p)) {
		/* have a number */
		/* get the base */
		base = 10;
 		if (*p == '0') {
 			base = 8;
			p++;
			if (CHARAT( p) == 'x' || CHARAT( p) == 'X') {
				p++;
				base = 16;
 			}
		}

		/* get the number */
		num = 0;
		for ( ; c = *p ; p++) {
			if (!FIRSTof2( c) && (isdigit( c) || base == 16 && isxdigit( c))) {
				digit = c - (isdigit( c) ? '0' :
			    		isupper( c) ? 'A' - 10 : 'a' - 10);
				if (digit >= base) {
					break;
				}
				num = base * num + digit;
			}
			else {
				break;
			}
		}
	}
	else {
		/* have a character */
		if (CHARAT( p) == '\\') {
			p++;
		}
		if (( num = (UINT) CHARAT( p)) == '\0') {
			return BAD;
		}
	}

	*result = num;

	return GOOD;
}

/*
**************************************************************************
** finish() 
**
** description:
**	get ready to leave: close message catalogs,
**	and deallocate jump table routines.
** 
** global variables:
**	Catd: nl_catd message catalog descriptor
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
finish()
{
	/* close the message catalog 
	   ... and do not complain about a missing catalog */
	catclose( Catd);

	return GOOD;
}

/*
**************************************************************************
** Perror()
**
** description:
**	set up string with program name and the failed routine name
**	display system error message on stderr using perror(3)
**
** assumption:
**	perror string before the colon will not exceed MAX_ERR
** 
** global variables:
**	Progname: char pointer to the program name
**
** return value:
**	no return value
**************************************************************************
*/

/* VARARGS 1 */

static void
Perror( rname)
UCP rname;			/* bad routine name */
{
	UCHAR pstr[MAX_ERR];	/* perror string before the colon */

	/* set up perror string */
	(void) sprintf( pstr, "%s (%s)", Progname, rname);

	/* print the system message or errno */
	if (errno > 0  &&  errno < sys_nerr) {
		perror( pstr);
	}
	else {
		(void) fprintf( stderr, "%s: errno = %d\n", pstr, errno);
	}
}

/*
**************************************************************************
** error()
**
** description:
**	display error message on stderr and leave if fatal
**
** assumptions:
**	all errors go to stderr
**	only one message set
** 
** global variables:
**	Progname: char pointer to the program name
**	Message: array of char pointers to format string messages
**
** return value:
**	no return value
**************************************************************************
*/

/* VARARGS 2 */

static void
error( fatal, num, va_alist)
UINT fatal;			/* Warning or Fatal error */
UINT num;			/* message number */
va_dcl				/* optional arguments */
{
	CP fmt;			/* ptr to format string */
	va_list args;		/* points to optional argument list */

	/* sync stdout with stderr */
	if (fflush( stdout)) {
		Perror( "error -- fflush");
	}

	/* print the program name on stderr */
	if (fprintf( stderr, "%s: ", Progname) < 0) {
		Perror( "error -- fprintf");
	}

	/* get the format string */
	fmt = catgets( Catd, NL_SETN, ERR+num, Message[num]);

	/* set up the optional argument list */
	va_start( args);

	/* print the error message on stderr */
	if (vfprintf( stderr, fmt, args) < 0) {
		Perror( "error -- vfprintf");
	}

	/* close down the optional argument list */
	va_end( args);

	/* leave if a fatal error */
	if (fatal) {
		(void) finish( );
		exit( BAD);
	}
}

/*
**************************************************************************
** Printf()
**
** description:
**	print arguments in va_alist on standard out with error checking
**	get format string from message catalog.
**	if error, use Perror to record error message on stderr
**
** assumptions:
**	Perror messages will fit in a MAX_ERR sized buffer
**
** global variables:
**	Catd: nl_catd message catalog descriptor
** 
** return value:
**	no return value
**************************************************************************
*/

/* VARARGS 3 */

static void
Printf( fmt, num, i, va_alist)
CP *fmt;			/* printf format string array */
UINT num;			/* message number */
UINT i;				/* index */
va_dcl				/* optional arguments */
{
	CP f;			/* ptr to format string */
	va_list args;		/* points to optional argument list */

	/* get the format string */
	if (*(f = catgets( Catd, NL_SETN, num+i, fmt[i])) == '\0') {
		f = fmt[i];
	}

	/* set up the optional argument list */
	va_start( args);

	/* print the stuff */
	if (vprintf( f, args) < 0) {
		UCHAR mess[MAX_ERR];
		Perror( strcat( strncpy( mess, Error, MAX_ERR), " -- printf"));
	}

	/* close down the optional argument list */
	va_end( args);
}

/*
*****************************************************************************
** utilities used by the display routines
*****************************************************************************
*/

/*
**************************************************************************
** svis()
**
** description:
**	make a string visable.
**	chars in string represented as printable chars, hex or octal numbers.
**
** return value:
**	ptr to resultant visable string.
**	static buffer overwritten on successive calls.
**************************************************************************
*/

static UCP 
svis( s)
UCP s;					/* string to make visable */
{
	static CP fmt1[] = {		/* single-byte format strings */
		"%c",
		"\\x%.2x",
		"\\%.3o",
	};
	static CP fmt2[] = {		/* multi-byte format strings */
		"%c%c",
		"\\x%.2x\\x%.2x",
		"\\%.3o\\%.3o",
	};
	static UCHAR buf[MAX_FNAME];	/* buffer to hold return value */
	UCP p1;				/* generic ptr to string */
	UCP p2;				/* generic ptr to string */
	UINT i;				/* index into format arrays */
	int numbytes;			/* num bytes returned from sprintf */

	for (p1=s, p2=buf ; *p1 ; p1++, p2++) {
		if (FIRSTof2( *p1)) {
			PTFMT( i, SECof2( *(p1+1)))
			if ((numbytes = sprintf( p2, fmt2[i], *p1, *(p1+1))) < 0) {
				return (UCP) NULL;
			}
			if (((p2+numbytes) - buf) > MAX_FNAME) {
				return (UCP) NULL;
			}
			p2 += numbytes - 1;
			p1++;
		}
		else {
			PTFMT( i, isprint(*p1))
			if ((numbytes = sprintf( p2, fmt1[i], *p1)) < 0) {
				return (UCP) NULL;
			}
			if (((p2+numbytes) - buf) > MAX_FNAME) {
				return (UCP) NULL;
			}
			p2 += numbytes - 1;
		}
	}
	*p2 = '\0';
	return buf;
}

/*
**************************************************************************
** nvis()
**
** description:
**	make a single character visable.
**	char represented as printable chars, hex or octal numbers.
**	no \ or \x
**
** return value:
**	ptr to resultant visable character.
**	static buffer overwritten on successive calls.
**************************************************************************
*/

static UCP 
nvis( ch)
UINT ch;
{
	static CP fmt1[] = {
		" %c ",
		"%.3x",
		"%.3o",
	};
	static CP fmt2[] = {
		"  %c%c  ",
		" %.4x ",
		"%.6o",
	};
	static UCHAR buf[7];
	UINT ch1, ch2;
	UINT i;
	int n;

	if (ch < MAX_1PLUS) {
		PTFMT( i, isprint( ch))
		if ((n = sprintf( buf, fmt1[i], ch)) < 0) {
			return (UCP) NULL;
		}
		if (i == HEX) {
			buf[0] = ' ';
		}
	}
	else {
		SEP2( ch, ch1, ch2)
		if (FIRSTof2( ch1)) {
			PTFMT( i, SECof2( ch2))
			if (i == CHR) {
				if ((n = sprintf( buf, fmt2[i], ch1, ch2)) < 0) {
					return (UCP) NULL;
				}
			}
			else {
				if ((n = sprintf( buf, fmt2[i], ch)) < 0) {
					return (UCP) NULL;
				}
			}
		}
		else {
			i = (Nonprint == HEX) ? HEX : OCT;
			if ((n = sprintf( buf, fmt2[i], ch)) < 0) {
				return (UCP) NULL;
			}
		}
	}
	buf[n] = '\0';
	return buf;
}

/*
*****************************************************************************
** display routines
*****************************************************************************
*/

/*
**************************************************************************
** lc_ctype()
**
** description:
**	display (1) ctype (2) conv (3) 1st-of-2 2nd-of-2 ranges.
**	the 1st-of-2 2nd-of-2 ranges displayed only for 2-byte languages.
**	entry point for -c option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
lc_ctype()
{
	/* assume a sucessful return value*/
	int retval = GOOD;

	if (ctype() == BAD) {
		retval = BAD;
	}
	if (conv() == BAD) {
		retval = BAD;
	}
	if (__nl_char_size > 1) {
		if (nl_tools_16() == BAD) {
			retval = BAD;
		}
	}
	if (ctype_customs() == BAD) {
		retval = BAD;
	}
	return retval;
}

/*
**************************************************************************
** ctype()
**
** description:
**	display ctype(3c) information.
**	only single-byte information displayed
**
** globals:
**	Lower, Upper, Hlines (thru HEADER)
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
ctype()
{
	static CP fmt[] = {
"COMMENT: ctype format strings",								/* catgets 100 */
" Character Classification\n",									/* catgets 101 */
" ========================\n", 									/* catgets 102 */
" char alpha upper lower digit xdigt alnum space punct print graph cntrl ascii\n",		/* catgets 103 */
" ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====\n",		/* catgets 104 */
" %3s%5d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n",							/* catgets 105 */
	};
	UINT ch;			/* character */
	UINT lines = 0;			/* number of lines displayed */
	Error = "cytpe";		/* routine error message string */

	/* put out the title */
	if (Lower < MAX_1PLUS || Upper < MAX_1PLUS) {
		TITLE( fmt, CTY, 1, TRUE);
	}

	for (ch = Lower ; ch <= Upper ; ch++) {
		/* ctype defined for 1-byte characters only */
		/* if (ch < MAX_1PLUS && !FIRSTof2( ch)) { */
		if (ch < MAX_1PLUS) {
			/* if necessary put out the header */
			HEADER( lines, fmt, CTY, 3)

			/* display ctype info about the character */
			Printf( fmt, CTY, 5, nvis(ch),
				isalpha(ch)?1:0, isupper(ch)?1:0,
				islower(ch)?1:0, isdigit(ch)?1:0,
				isxdigit(ch)?1:0, isalnum(ch)?1:0,
				isspace(ch)?1:0, ispunct(ch)?1:0, 
				isprint(ch)?1:0, isgraph(ch)?1:0,
				iscntrl(ch)?1:0, isascii(ch)?1:0);
		}
	}
	return GOOD;
}

/*
**************************************************************************
** conv()
**
** description:
**	display conv(3c) information.
**	only single-byte information displayed
**
** globals:
**	Lower, Upper
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
conv()
{
	static CP fmt[] = {
		"COMMENT: conv format strings",	/* catgets 200 */
		" Character Conversion\n",	/* catgets 201 */
		" ====================\n",	/* catgets 202 */
		" char upshift downshift\n",	/* catgets 203 */
		" ==== ======= =========\n",	/* catgets 204 */
		" %3s%7s%9s\n",			/* catgets 205 */
	};
	UCHAR s1[4], s2[4];		/* characters as strings to display */
	UINT ch;			/* character */
	UINT lines = 0;			/* upshift or downshift character */
	Error = "conv";			/* routine error message string */

	/* put out the title */
	if (Lower < MAX_1PLUS || Upper < MAX_1PLUS) {
		TITLE( fmt, CON, 1, TRUE);
	}

	for (ch = Lower ; ch <= Upper ; ch++) {

		/* only display case sensitive 1-byte characters */
		/* if ((ch < MAX_1PLUS
			&& !FIRSTof2(ch)) && (islower(ch) || isupper(ch))) { */
		if (ch < MAX_1PLUS && (islower(ch) || isupper(ch))) {

			/* if necessary put out the header */
			HEADER( lines, fmt, CON, 3)

			/* display conv info about the character */
			(void) strcpy( s1, nvis( ch));
			(void) strcpy( s2, nvis( (UINT)toupper( ch)));
			Printf( fmt, CON, 5, s1, s2, nvis((UINT)tolower( ch)));
		}
	}
	return GOOD;
}

/*
**************************************************************************
** nl_tools_16()
**
** description:
**	display the 1st-of-2 and 2nd-of-2 ranges
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
nl_tools_16()
{
	static CP fmt[] = {
		"COMMENT: nl_tools_16 format strings",	/* catgets 300 */
		" Multi-byte Range\n",			/* catgets 301 */
		" ================\n",			/* catgets 302 */
		" First-of-two\n",			/* catgets 303 */
		" ============\n",			/* catgets 304 */
		"   %3s - %3s\n",			/* catgets 305 */
		" Second-of-two\n",			/* catgets 306 */
		" =============\n",			/* catgets 307 */
	};
	Error = "nl_tools_16";				/* routine error message string */

	/* put out the title */
	TITLE( fmt, NLT, 1, TRUE);

	/* 1st-of-two range */
	if (first_second( firstof2, &fmt[0], 3) == BAD) {
		return BAD;
	}
	/* 2nd-of-two range */
	if (first_second( secof2, &fmt[0], 6) == BAD) {
		return BAD;
	}
	return GOOD;
}

/*
**************************************************************************
** first_second()
**
** description:
**	display either a 1st-of-2 or a 2nd-of-2 ranges
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
first_second( routine, fmt, index)
PFI routine;
CP *fmt;
UINT index;
{
	UCHAR s1[4];			/* save display character string */
	UINT ch;			/* character */
	UINT ch1, ch2;			/* range characters */
	UINT lines = 0;			/* number of displayed ranges */
	Error = "first_second";		/* routine error message string */

	for (ch = MIN_1CHAR ; ch < MAX_1PLUS ; ch++) {
		if (have_range( routine, &ch, &ch1, &ch2)) {
			/* if necessary put out the header */
			HEADER( lines, fmt, NLT, index)

			/* display range info about the character */
			(void) strcpy( s1, nvis( ch1));
			Printf( fmt, NLT, 5, s1, nvis( ch2));
		}
	}
	return GOOD;
}

/*
**************************************************************************
** ctype_customs()
**
** description:
**	display local customs not in lc_ctype
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
ctype_customs()
{
	static CP fmt[] = {
"COMMENT: ctype_customs format strings",			/* catgets 1700 */
" Ctype Customs\n",						/* catgets 1701 */
" =============\n",						/* catgets 1702 */
" description               symbol            string\n",	/* catgets 1703 */
" ===========               ======            ======\n",	/* catgets 1704 */
" bytes per character       BYTES_CHAR        \"%s\"\n",	/* catgets 1705 */
" alternative punctuation   ALT_PUNCT         \"%s\"\n",	/* catgets 1706 */
	};
	static UINT item[] = {
		BYTES_CHAR,
		ALT_PUNCT,
	};

	return local_customs(&fmt[0], CCS, &item[0], SIZE( item, int));
}


/*
**************************************************************************
** have_range()
**
** description:
**	see if given language has a 1st-of-2 or a 2nd-of-2 range.
**	store end points of valid range in ch1 and ch2.
**
** return value:
**	TRUE: have a range
**	FALSE: do not have a range
**************************************************************************
*/

static int
have_range( routine, ch, ch1, ch2)
PFI routine;			/* 1st- or 2nd-of-two routine */
UIP ch;				/* ptr to character to test */
UIP ch1;			/* ptr to first character in range */
UIP ch2;			/* ptr to last character in range */
{
	*ch1 = MIN_1CHAR;
	for ( ; *ch < MAX_1PLUS ; (*ch)++) {
		if ((*routine)( *ch)) {
			*ch1 = *ch;
			break;
		}
		else {
			continue;
		}
	}
	if (*ch1) {
		*ch2 = MIN_1CHAR;
		for ( ; *ch < MAX_1PLUS ; (*ch)++) {
			if (!(*routine)( *ch)) {
				*ch2 = *ch - 1;
			break;
			}
			else {
				continue;
			}
		}
		if (! *ch2) *ch2 = *ch - 1;
		return TRUE;
	}
	return FALSE;
}

/*
**************************************************************************
** lc_collate()
**
** description:
**	display collation data.
**	machine collation if 2-byte lang or no sequence table.
**	otherwise, display characters in collation order.
**	entry point for -s option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
lc_collate()
{
	if (_seqtab) {
		/* have a collation sequence */
		if (__nl_char_size == 1) {
			/* single-byte language */
			return collate1();
		}
		else if (__nl_char_size == 2) {
			/* multi-byte language */
			return collate2();
		}
	}
	else {
		/* machine collation */
		return collate2();
	}
	return BAD;
}

/*
**************************************************************************
** collate1()
**
** description:
**	display single-byte characters in collation order.
**	creates a link list of characters ordered first by sequence number
**	then by priority number.
**
** globals:
**	Everything, Ptop
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
collate1()
{
	static CP fmt[] = {
"COMMENT: collate single-byte format strings",		    /* catgets 400 */
" Character Collation\n",				    /* catgets 401 */
" ===================\n",				    /* catgets 402 */
" type      ch     ch1     ch2     seq     seq2     pri\n", /* catgets 403 */
" ====     ===     ===     ===     ===     ====     ===\n", /* catgets 404 */
" %3s%9s%8s%8s%8d%8s%8d\n",				    /* catgets 405 */
" %3s%9s\n",				    		    /* catgets 406 */
	};
	COLL_LIST *p;		/* ptr to list element */
	UCHAR ch;		/* the character code */
	UCHAR ch1;		/* 1st character code if 1-to-2 or 2-to-1 */
	UCHAR ch2;		/* 2nd character code if 1-to-2 or 2-to-1 */
	UCHAR s[4], s1[4];	/* characters as strings to display */
	CP type;		/* 1-1, 1-2, 2-1 */
	UINT lastseq1;		/* seqence no of last char in range */
	UINT lastpri;		/* priority of last char in range */
	UINT lines = 0;		/* number of lines displayed */
	Error = "collate1";	/* routine error message string */

	/* create character information linked list in sequence number order */
	if (col_list_fill( )) {
		return BAD;
	}

	if (Everything) {
		/* display the entire link list */
		p = Ptop;
		lastseq1 = UINT_MAX;
	}
	else {
		/* find the last character to display */
		for(p=Ptop ; p->next ; p=p->next) {
			if (p->ch1 == Upper) {
				break;
			}
		}
		lastseq1 = p->seq1;
		lastpri = p->pri;

		/* find the first character to display */
		for(p=Ptop ; p->next ; p=p->next) {
			if (p->ch1 == Lower) {
				break;
			}
		}
	}

	/* put out the title */
	TITLE( fmt, CL1, 1, TRUE);

	/* display the sequence number link list */
	for ( ; COLL_LAST( p) ; p=p->next) {

		/* if necessary put out the header */
		HEADER( lines, fmt, CL1, 3)

		/* char info based on type of char */
		switch (p->type) {
		case TNORMAL:
			type = "1-1";
			ch = p->ch1;
			ch1 = ' ';
			ch2 = ' ';
			break;
		case TTWOTO1:
			type = "2-1";
			ch = ' ';
			ch1 = p->ch1;
			ch2 = p->ch2;
			break;
		case TONETO2:
			type = "1-2";
			ch = p->ch1;
			ch1 = p->first_ch;
			ch2 = p->ch2;
			break;
		case DONTCARE:
			Printf( fmt, CL1, 6, "D-C", nvis( p->ch1));
			continue;
		default:
			return BAD;
		}

		/* display collation info about the character */
		(void) strcpy( s, nvis( ch));
		(void) strcpy( s1, nvis( ch1));
		Printf( fmt, CL1, 5, type, s,
			ch1 == ' ' ? SP : s1,
			ch2 == ' ' ? SP : nvis( ch2),
			p->seq1,
			p->seq2 == 0 ? SP : tostr( p->seq2),
			p->pri);
	}
	return GOOD;
}

/*
**************************************************************************
** collate2()
**
** description:
**	display characters (possibly multi-byte) in machine collation.
**	all characters here are 1-1.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
collate2()
{
	static CP fmt[] = {
"COMMENT: collate multi-byte format strings",		    /* catgets 500 */
" Character Collation\n",				    /* catgets 501 */
" ===================\n",				    /* catgets 502 */
" type      ch     ch1     ch2     seq     seq2     pri\n", /* catgets 503 */
" ====     ===     ===     ===     ===     ====     ===\n", /* catgets 504 */
" %3s%9s%8s%8s%8d%8s%8d\n",				    /* catgets 505 */
" %3s%11s%8s%8s%7d%8s%7d\n",				    /* catgets 506 */
	};
	UINT lmsb;		/* lower most-significant byte */
	UINT umsb;		/* upper most-significant byte */
	UINT msb, lsb;		/* character most-least significant byte */
	UINT ch;		/* character to display */
	UINT lower1, upper1;	/* single-byte lower-upper bound */
	UINT lower2, upper2;	/* multi-byte lower-upper bound */
	UINT lines;		/* number of lines displayed */
	Error = "collate2";	/* routine error message string */
	
	/* set up the boundaries */
	if (bounds( &lower1, &upper1, &lower2, &upper2) == BAD) {
		return BAD;
	}

	/* put out the title */
	TITLE( fmt, CL2, 1, TRUE);

	/* print single-byte characters */
	lines = 0;
	for (ch = lower1 ; ch <= upper1 ; ch++) {
		if (((ch < MAX_1PLUS) && !FIRSTof2(ch))) {
			/* if necessary put out the header */
			HEADER( lines, fmt, CL2, 3)

			/* display collation info about the character */
			Printf( fmt, CL2, 5,
				"1-1", nvis(ch), SP, SP, ch, SP, 0);
		}
	}

	/* print multi-byte characters */
	lines = 0;
	lmsb = lower2 >> 8;
	umsb = upper2 >> 8;
	for (msb=lmsb ; msb <= umsb ; msb++) {
		for (lsb=MIN_1CHAR ; lsb < MAX_1PLUS ; lsb++) {
			if (lower2 > (ch = CH2( msb,lsb))) {
				continue;
			}
			if (upper2 >= ch && FIRSTof2(msb) && SECof2(lsb)) {
				/* if necessary put out the header */
				HEADER( lines, fmt, CL2, 3)

				/* display collation info about the character */
				Printf( fmt, CL2, 6,
					"1-1", nvis(ch), SP, SP, ch, SP, 0);
			}
		}
	}
	return GOOD;
}

/*
**************************************************************************
** tostr()
**
** description:
**	convert a number into a decimal string.
**	used in collate1().
**
** return value:
**	ptr to resultant decimal number string.
**	static buffer overwritten on successive calls.
**************************************************************************
*/

static UCP
tostr( i)
UINT i;
{
	static UCHAR buf[4];
	int n;

	if ((n = sprintf( buf, "%d", i)) < 0) {
		return (UCP) NULL;
	}
	buf[n] = '\0';
	return buf;
}

/*
**************************************************************************
** bounds()
**
** description:
**	finds upper and lower bounds of character range display.
**
** globals:
**	Everything, Upper, Lower
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
bounds( lower1, upper1, lower2, upper2)
UINT *lower1, *upper1;			/* single-byte lower-upper bound */
UINT *lower2, *upper2;			/* multi-byte lower-upper bound */
{
	if (Everything) {
		*lower1 = MIN_1CHAR;
		*upper1 = MAX_1CHAR;
		*lower2 = MIN_2CHAR;
		*upper2 = MAX_2CHAR;
	}
	else {
		if (Lower > Upper) {
			return BAD;
		}
		else if (Lower < MAX_1PLUS && Upper < MAX_1PLUS) {
			*lower1 = Lower;
			*upper1 = Upper;
			*lower2 = MAX_2CHAR;
			*upper2 = MIN_2CHAR;
		}
		else if (Lower < MAX_1PLUS && Upper > MAX_1CHAR) {
			*lower1 = Lower;
			*upper1 = MAX_1CHAR;
			*lower2 = MIN_2CHAR;
			*upper2 = Upper;
		}
		else if (Lower > MAX_1CHAR && Upper > MAX_1CHAR) {
			*lower1 = MAX_1CHAR;
			*upper1 = MIN_1CHAR;
			*lower2 = Lower;
			*upper2 = Upper;
		}
	}
	return GOOD;
}

/*
**************************************************************************
** col_list_fill()
**
** description:
**	collation order utility.
**	creates a link list of characters ordered first by sequence number
**	then by priority number.
**
** globals:
**	Ptop
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
col_list_fill( )
{
	UINT i,j;
	TAB21 *p21;
	TAB12 *p12;
	COLL_LIST *pnew_elem;	/* ptr to new list element */

	Ptop = NULL;

	/* allocate linked list */
	if ((pnew_elem = (COLL_LIST *) calloc( NELEM, ELSIZE)) == (COLL_LIST *) NULL) {
		Perror("col_list_fill -- calloc");
		return BAD;
	}

	/* for each character, place it and any 1to2 or 2to1 characters in 
	   the correct place in the linked list */
	for (i = MIN_1CHAR; i < MAX_1PLUS; i++) {
		switch (_pritab[i] >> 6) {
		case TNORMAL:
			pnew_elem->type = TNORMAL;
			pnew_elem->ch1 = i;
			pnew_elem->seq1 = _seqtab[i];
			pnew_elem->ch2 = NULL; 
			pnew_elem->seq2 = NULL;
			pnew_elem->pri = _pritab[i];
			if (link_elem( pnew_elem) == BAD) {
				return BAD;
			}
			pnew_elem++;
			break;
		case TTWOTO1:
			p21 = &_tab21[_pritab[i]&077];
			while (p21->ch1 != ENDTABLE) {
				pnew_elem->type = TTWOTO1;
				pnew_elem->ch1 = i;
				pnew_elem->seq1 = p21->seqnum;
				pnew_elem->ch2 = p21->ch2; 
				pnew_elem->seq2 = NULL;
				pnew_elem->pri = p21->priority & 077;
				if (link_elem( pnew_elem) == BAD) {
					return BAD;
				}
				pnew_elem++;
				p21++;
			}
			break;
		case TONETO2:
			p12 = &_tab12[_pritab[i]&077];
			pnew_elem->type = TONETO2;
			pnew_elem->ch1 = i;
			pnew_elem->seq1 = _seqtab[i];
			for(j=MIN_1CHAR; (j<MAX_1PLUS) && (_seqtab[j]!=pnew_elem->seq1); j++)
				pnew_elem->first_ch = j+1;
			for(j=MIN_1CHAR; (j<MAX_1PLUS) && (_seqtab[j]!=p12->seqnum); j++)
				pnew_elem->ch2 = j+1;
			pnew_elem->seq2 = p12->seqnum;
			pnew_elem->pri = p12->priority & 077;
			if (link_elem( pnew_elem) == BAD) {
				return BAD;
			}
			pnew_elem++;
			break;
		case DONTCARE:
			pnew_elem->type = DONTCARE;
			pnew_elem->ch1 = i;
			pnew_elem->pri = _pritab[i];
			pnew_elem->seq1 = _seqtab[i];
			pnew_elem->next = Ptop;
			Ptop = pnew_elem;
			pnew_elem++;
			break;
		default: 
			return BAD;
		}
	}
	return GOOD;
}

/*
**************************************************************************
** link_elem()
**
** description:
**	collation order utility.
**	link a new node into the list.
**
** globals:
**	Ptop
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
link_elem( pnew)
COLL_LIST *pnew;		/* ptr to new list element to be added */
{
	COLL_LIST *tptr;	/* loop temp structure pointer */
	COLL_LIST *otptr;	/* previous tptr value */

	if (Ptop == NULL) {
		/* first time here:
			the list is empty so simply add the element */
		Ptop= pnew;
		pnew->next = NULL;
	}
	else {
		/* not an empty list:
			find out where the character belongs and add it */
		for(tptr = otptr = Ptop;
			(tptr->next != NULL) && (pnew->seq1>tptr->seq1) ;
				tptr=tptr->next) {
					/* save current ptr */
					otptr=tptr;
		}

		if (pnew->seq1 == tptr->seq1) {
			/* seq #s equal, use priority */
			for(; (tptr->next != NULL)
			    &&(pnew->pri>tptr->pri) && (pnew->seq1==tptr->seq1);
				tptr=tptr->next) {
					/*save current ptr*/
					otptr=tptr;
			}

			/* seq/pri must be unique */
			/* seq/pri don't have to be unique.
			   If you want to enforce it, fix localedef.
			if ( (pnew->pri == tptr->pri) && (pnew->seq1 == tptr->seq1) ) {
			   	return BAD;
			}
			*/
		}

	
		if ((tptr==Ptop) && (tptr->next)){
			/* place new elem. at top of list */
			pnew->next = Ptop;
			Ptop = pnew;
		}
		else if ( (tptr->next) || (pnew->seq1<tptr->seq1) ) {
			/* new element added before current element */
			pnew->next = otptr->next;
			otptr->next = pnew;
		}
		else { 
			/* add new element to end of list */
			tptr->next = pnew;
			pnew->next = NULL;
		}

	}
	return GOOD;
}

/*
**************************************************************************
** lc_time()
**
** description:
**	display local custom time information.
**	entry point for -t option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
lc_time()
{
	static CP fmt[] = {
"COMMENT: lc_time format strings",				/* catgets 600 */
" Time Customs\n",						/* catgets 601 */
" ============\n",						/* catgets 602 */
" description               symbol            string\n",	/* catgets 603 */
" ===========               ======            ======\n",	/* catgets 604 */
" date-time format string   D_T_FMT           \"%s\"\n",	/* catgets 605 */
" date format string        D_FMT             \"%s\"\n",	/* catgets 606 */
" time format string        T_FMT             \"%s\"\n",	/* catgets 607 */

" day names                 DAY_1             \"%s\"\n",	/* catgets 608 */
"                           DAY_2             \"%s\"\n",	/* catgets 609 */
"                           DAY_3             \"%s\"\n",	/* catgets 610 */
"                           DAY_4             \"%s\"\n",	/* catgets 611 */
"                           DAY_5             \"%s\"\n",	/* catgets 612 */
"                           DAY_6             \"%s\"\n",	/* catgets 613 */
"                           DAY_7             \"%s\"\n",	/* catgets 614 */
" abbreviated day names     ABDAY_1           \"%s\"\n",	/* catgets 615 */
"                           ABDAY_2           \"%s\"\n",	/* catgets 616 */
"                           ABDAY_3           \"%s\"\n",	/* catgets 617 */
"                           ABDAY_4           \"%s\"\n",	/* catgets 618 */
"                           ABDAY_5           \"%s\"\n",	/* catgets 619 */
"                           ABDAY_6           \"%s\"\n",	/* catgets 620 */
"                           ABDAY_7           \"%s\"\n",	/* catgets 621 */
" month names               MON_1             \"%s\"\n",	/* catgets 622 */
"                           MON_2             \"%s\"\n",	/* catgets 623 */
"                           MON_3             \"%s\"\n",	/* catgets 624 */
"                           MON_4             \"%s\"\n",	/* catgets 625 */
"                           MON_5             \"%s\"\n",	/* catgets 626 */
"                           MON_6             \"%s\"\n",	/* catgets 627 */
"                           MON_7             \"%s\"\n",	/* catgets 628 */
"                           MON_8             \"%s\"\n",	/* catgets 629 */
"                           MON_9             \"%s\"\n",	/* catgets 630 */
"                           MON_10            \"%s\"\n",	/* catgets 631 */
"                           MON_11            \"%s\"\n",	/* catgets 632 */
"                           MON_12            \"%s\"\n",	/* catgets 633 */
" abbreviated month names   ABMON_1           \"%s\"\n",	/* catgets 634 */
"                           ABMON_2           \"%s\"\n",	/* catgets 635 */
"                           ABMON_3           \"%s\"\n",	/* catgets 636 */
"                           ABMON_4           \"%s\"\n",	/* catgets 637 */
"                           ABMON_5           \"%s\"\n",	/* catgets 638 */
"                           ABMON_6           \"%s\"\n",	/* catgets 639 */
"                           ABMON_7           \"%s\"\n",	/* catgets 640 */
"                           ABMON_8           \"%s\"\n",	/* catgets 641 */
"                           ABMON_9           \"%s\"\n",	/* catgets 642 */
"                           ABMON_10          \"%s\"\n",	/* catgets 643 */
"                           ABMON_11          \"%s\"\n",	/* catgets 644 */
"                           ABMON_12          \"%s\"\n",	/* catgets 645 */
" AM string                 AM_STR            \"%s\"\n",	/* catgets 646 */
" PM string                 PM_STR            \"%s\"\n",	/* catgets 647 */
" year symbol               YEAR_UNIT         \"%s\"\n",	/* catgets 648 */
" month symbol              MON_UNIT          \"%s\"\n",	/* catgets 649 */
" day symbol                DAY_UNIT          \"%s\"\n",	/* catgets 650 */
" hour symbol               HOUR_UNIT         \"%s\"\n",	/* catgets 651 */
" minute symbol             MIN_UNIT          \"%s\"\n",	/* catgets 652 */
" second symbol             SEC_UNIT          \"%s\"\n",	/* catgets 653 */
" era format string         ERA_FMT           \"%s\"\n",	/* catgets 654 */
	};
	static UINT item[] = {
		D_T_FMT,
		D_FMT,
		T_FMT,
		DAY_1,
		DAY_2,
		DAY_3,
		DAY_4,
		DAY_5,
		DAY_6,
		DAY_7,
		ABDAY_1,
		ABDAY_2,
		ABDAY_3,
		ABDAY_4,
		ABDAY_5,
		ABDAY_6,
		ABDAY_7,
		MON_1,
		MON_2,
		MON_3,
		MON_4,
		MON_5,
		MON_6,
		MON_7,
		MON_8,
		MON_9,
		MON_10,
		MON_11,
		MON_12,
		ABMON_1,
		ABMON_2,
		ABMON_3,
		ABMON_4,
		ABMON_5,
		ABMON_6,
		ABMON_7,
		ABMON_8,
		ABMON_9,
		ABMON_10,
		ABMON_11,
		ABMON_12,
		AM_STR,
		PM_STR,
		YEAR_UNIT,
		MON_UNIT,
		DAY_UNIT,
		HOUR_UNIT,
		MIN_UNIT,
		SEC_UNIT,
		ERA_FMT,
	};

	return local_customs(&fmt[0], TIM, &item[0], SIZE( item, int));
}

/*
**************************************************************************
** lc_monetary()
**
** description:
**	display local custom and lconv monetary information.
**	entry point for -m option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
lc_monetary()
{
	static CP fmt[] = {
"COMMENT: lc_monetary format strings",						/* catgets 700 */
" Monetary Customs\n",								/* catgets 701 */
" ================\n",								/* catgets 702 */
" description                                     symbol               string\n",	/* catgets 703 */
" ===========                                     ======               ======\n",	/* catgets 704 */
" currency symbol                                 CRNCYSTR             \"%s\"\n",/* catgets 705 */
" Monetary lconv Values\n",							/* catgets 706 */
" =====================\n",							/* catgets 707 */
" description                                     lconv element        value\n",	/* catgets 708 */
" ===========                                     =============        =====\n",	/* catgets 709 */
" international currency symbol                   int_curr_symbol      \"%s\"\n",/* catgets 710 */
" local currency symbol                           currency_symbol      \"%s\"\n",/* catgets 711 */
" monetary decimal point                          mon_decimal_point    \"%s\"\n",/* catgets 712 */
" monetary thousands separator                    mon_thousands_sep    \"%s\"\n",/* catgets 713 */
" size of monetary digit group                    mon_grouping         \"%s\"\n",/* catgets 714 */
" monetary positive sign                          positive_sign        \"%s\"\n",/* catgets 715 */
" monetary negative sign                          negative_sign        \"%s\"\n",/* catgets 716 */
" number of international fractional digits       int_frac_digits      \'%s\'\n",/* catgets 717 */
" number of fractional digits                     frac_digits          \'%s\'\n",/* catgets 718 */
" symbol precedes/succeeds positive value         p_cs_precedes        \'%s\'\n",/* catgets 719 */
" space between symbol and positive value         p_sep_by_space       \'%s\'\n",/* catgets 720 */
" symbol precedes/succeeds negative value         n_cs_precedes        \'%s\'\n",/* catgets 721 */
" space between symbol and negative value         n_sep_by_space       \'%s\'\n",/* catgets 722 */
" position of positive sign                       p_sign_posn          \'%s\'\n",/* catgets 723 */
" position of negative sign                       n_sign_posn          \'%s\'\n",/* catgets 724 */
	};
	static UINT item[] = {
		CRNCYSTR,
	};

	LCONVINFO info[LCONV_SIZE];
	UINT i;

	/* display local customs information */
	if (local_customs( &fmt[0], MON, &item[0], SIZE( item, int)) == BAD ) {
		return BAD;
	}

	/* select the parts of the lconv structure to display */
	for (i=0 ; i < 3 ; i++) {
		info[i].flag = FALSE;
	}
	for ( ; i < 10 ; i++) {
		info[i].flag = TRUE;
		info[i].type = STR;
	}
	for ( ; i < LCONV_SIZE ; i++) {
		info[i].flag = TRUE;
		info[i].type = CHR;
	}

	/* display lconv structure information */
	if (disp_lconv( &fmt[6], MON+6, &info[0]) == BAD) {
		return BAD;
	}

	/* display meaning of various values in lconv structure */
	if (disp_group( ) == BAD) {
		return BAD;
	}
	if (disp_cs( ) == BAD) {
		return BAD;
	}
	if (disp_sep_space( ) == BAD) {
		return BAD;
	}
	return disp_posn( );
}

/*
**************************************************************************
** lc_numeric()
**
** description:
**	display local custom and lconv numeric information.
**	entry point for -n option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
lc_numeric()
{
	static CP fmt[] = {
"COMMENT: lc_numeric format strings",				/* catgets 800 */
" Numeric Customs\n",						/* catgets 801 */
" ===============\n",						/* catgets 802 */
" description               symbol            string\n",	/* catgets 803 */
" ===========               ======            ======\n",	/* catgets 804 */
" radix character           RADIXCHAR         \"%s\"\n",	/* catgets 805 */
" thousands separator       THOUSEP           \"%s\"\n",	/* catgets 806 */
" alternative digits        ALT_DIGIT         \"%s\"\n",	/* catgets 807 */
" Numeric lconv Values\n",					/* catgets 808 */
" ====================\n",					/* catgets 809 */
" description               name              value\n",		/* catgets 810 */
" ===========               ====              =====\n",		/* catgets 811 */
" radix character           decimal_point     \"%s\"\n",	/* catgets 812 */
" thousands separator       thousands_sep     \"%s\"\n",	/* catgets 813 */
" grouping                  grouping          \"%s\"\n",	/* catgets 814 */
	};
	static UINT item[] = {
		RADIXCHAR,
		THOUSEP,
		ALT_DIGIT
	};
	LCONVINFO info[LCONV_SIZE];
	UINT i;

	/* display local customs information */
	if (local_customs( &fmt[0], NUM, &item[0], SIZE( item, int)) == BAD) {
		return BAD;
	}

	/* select the parts of the lconv structure to display */
	for (i=0 ; i < 3 ; i++) {
		info[i].flag = TRUE;
		info[i].type = STR;
	}
	for ( ; i < LCONV_SIZE ; i++) {
		info[i].flag = FALSE;
	}

	/* display lconv structure information */
	if (disp_lconv( &fmt[8], NUM+8, &info[0]) == BAD) {
		return BAD;
	}

	/* display meaning of various values in lconv structure */
	return disp_group( );
}

/*
**************************************************************************
** other()
**
** description:
**	display local customs not in lc_ctype, lc_conv, lc_time, lc_numeric
**	and lc_monetary.
**	entry point for -h option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
other()
{
	static CP fmt[] = {
"COMMENT: other format strings",				/* catgets 900 */
" Other Customs\n",						/* catgets 901 */
" =============\n",						/* catgets 902 */
" description               symbol            string\n",	/* catgets 903 */
" ===========               ======            ======\n",	/* catgets 904 */
" yes string                YESSTR            \"%s\"\n",	/* catgets 905 */
" no string                 NOSTR             \"%s\"\n",	/* catgets 906 */
" direction                 DIRECTION         \"%s\"\n",	/* catgets 907 */
	};
	static UINT item[] = {
		YESSTR,
		NOSTR,
		DIRECTION,
	};

	return local_customs(&fmt[0], OTH, &item[0], SIZE( item, int));
}

/*
**************************************************************************
** local_custom()
**
** description:
**	general utility to display local custom information
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
local_customs( fmt, messnum, item, n)
CP *fmt;				/* ptr to start of format array */
UINT messnum;				/* format number */
UIP item;				/* item number of custom */
UINT n;					/* number of items to display */
{
	UCP s;				/* ptr to local customs string */
	UINT i;				/* generic counter */
	UINT lines = 0;			/* number of lines displayed */
	Error = "local_customs";	/* routine error message string */

	/* put out the title */
	TITLE( fmt, messnum, 1, TRUE);

	for (i=0 ; i < n ; i++) {
		/* if necessary put out the header */
		HEADER( lines, fmt, messnum, 3)

		/* get a visable local custom string */
		if ((s = svis( (UCP) nl_langinfo( item[i]))) == (UCP) NULL) {
			return BAD;
		}
		
		/* display local custom string */
		Printf( fmt, messnum, 5+i, s);
	}
	return GOOD;
}

/*
**************************************************************************
** disp_posn()
**
** description:
**	display explanatory information about lconv
**	p_sign_posn and n_sign values
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
disp_posn()
{
	static CP fmt[] = {
"COMMENT: format string lconv p_sign_posn and n_sign values",		/* catgets 1200 */
" lconv p_sign_posn and n_sign values\n",				/* catgets 1201 */
" ===================================\n",				/* catgets 1202 */
" %-6s parentheses surround the quantity and currency_symbol\n",	/* catgets 1203 */
" %-6s sign precedes the quantity and currency_symbol\n",		/* catgets 1204 */
" %-6s sign succeeds the quantity and currency_symbol\n",		/* catgets 1205 */
" %-6s sign immediately precedes the currency_symbol\n",		/* catgets 1206 */
" %-6s sign immediately succeeds the currency_symbol\n",		/* catgets 1207 */
" %-6s information not available in the currrent locale\n",		/* catgets 1208 */
	};
	static CP value[] = {
		"0",
		"1",
		"2",
		"3",
		"4",
		CMAX,
	};
	UINT i;
	UINT n = SIZE( fmt, CP);
	Error = "disp_posn";

	TITLE( fmt, DSP, 1, FALSE)

	for (i=3 ; i < n ; i++) {
		Printf( fmt, DSP, i, svis((UCP) value[i-3]));
	}

	return GOOD;
}

/*
**************************************************************************
** disp_cs()
**
** description:
**	display explanatory information about lconv
**	p_cs_precedes and n_cs_precedes values.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
disp_cs()
{
	static CP fmt[] = {
"COMMENT: format strings for p_cs_precedes and n_cs_precedes",	/* catgets 1300 */
" lconv p_cs_precedes and n_cs_precedes values\n",		/* catgets 1301 */
" ============================================\n",		/* catgets 1302 */
" %-6s currency_symbol succeeds quantity\n",			/* catgets 1303 */
" %-6s currency_symbol precedes quantity\n",			/* catgets 1304 */
" %-6s information not available in the currrent locale\n",	/* catgets 1305 */
	};
	static CP value[] = {
		"0",
		"1",
		CMAX,
	};
	UINT i;
	UINT n = SIZE( fmt, CP);
	Error = "disp_cs_precedes";

	TITLE( fmt, DSC, 1, FALSE)

	for (i=3 ; i < n ; i++) {
		Printf( fmt, DSC, i, svis((UCP) value[i-3]));
	}

	return GOOD;
}

/*
**************************************************************************
** disp_sep_space()
**
** description:
**	display explanatory information about lconv
**	p_sep_by_space and n_sep_by_space values.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
disp_sep_space()
{
	static CP fmt[] = {
"COMMENT: format string for p_sep_by_space and n_sep_by_space",		/* catgets 1400 */
" lconv p_sep_by_space and n_sep_by_space values\n",			/* catgets 1401 */
" ==============================================\n",			/* catgets 1402 */
" %-6s currency_symbol is not separated by a space from quantity\n",	/* catgets 1403 */
" %-6s currency_symbol is separated by a space from quantity\n",	/* catgets 1404 */
" %-6s information not available in the currrent locale\n",		/* catgets 1405 */
	};
	static CP value[] = {
		"0",
		"1",
		CMAX,
	};
	UINT i;
	UINT n = SIZE( fmt, CP);
	Error = "disp_sep_space";

	TITLE( fmt, DSS, 1, FALSE)

	for (i=3 ; i < n ; i++) {
		Printf( fmt, DSS, i, svis((UCP) value[i-3])); 
	}

	return GOOD;
}

/*
**************************************************************************
** disp_group()
**
** description:
**	display explanatory information about lconv
**	grouping and mon_grouping values.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
disp_group()
{
	static CP fmt[] = {
"COMMENT: format strings for grouping and mon_grouping",		/* catgets 1500 */
" lconv grouping and mon_grouping values\n",				/* catgets 1501 */
" ======================================\n",				/* catgets 1502 */
"   %-6s no further grouping\n",					/* catgets 1503 */
"   %-6s previous element repeatedly used for remainder of digits\n",	/* catgets 1504 */
"   %-6s number of digits in the current group\n",			/* catgets 1505 */
	};
	static CP value[] = {
		CMAX,
		"0",
		"other",		/* catgets 1506 */
	};
	UINT i;
	UINT n = SIZE( fmt, CP);
	CP o;
	UCHAR buf[MAX_FNAME];
	Error = "disp_grouping";

	TITLE( fmt, DSG, 1, FALSE)

	for (i=3 ; i < n-1 ; i++) {
		Printf( fmt, DSG, i, svis((UCP) value[i-3])); 
	}

	n = SIZE( value, CP) - 1;
	if (*(o = catgets( Catd, NL_SETN, DSG+6, value[n])) == '\0') {
		o = value[n];
	}
	(void) strncpy( buf, o, MAX_FNAME);

	Printf( fmt, DSG, 5, buf);

	return GOOD;
}

/*
**************************************************************************
** disp_lconv()
**
** description:
**	display elements of the lconv structure
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
disp_lconv( fmt, messnum, info)
CP *fmt;
UINT messnum;
LCONVINFO *info;
{
	UCP s;			/* ptr to display string */
	CP p;			/* ptr to lconv element */
	UINT i,j;		/* counters */
	UINT lines = 0;		/* number of lines displayed */
	Error = "disp_lconv";	/* routine error message string */

	/* get the lconv structure information */
	if (get_lconv( info) == BAD) {
		return BAD;
	}

	/* put out the title */
	TITLE( fmt, messnum, 0, FALSE);

	/* display the lconv structure information */
	for (i=0, j=0 ; j < LCONV_SIZE ; j++) {
		if (info[j].flag) {

			/* if necessary put out the header */
			HEADER( lines, fmt, messnum, 2)

			/* get lconv structure element */
			if (info[j].type == STR) {
				p = info[j].ele.str;
			}
			else if (info[j].type == CHR) {
				p = info[j].ele.chr;
			}
			else {
				/* shouldn't happen */
				return BAD;
			}
			/* get lconv structure string */
			if ((s = svis( (UCP) p)) == (UCP) NULL) {
				return BAD;
			}
			/* display lconv structure string */
			Printf( fmt, messnum, 4+i, s);
			i++;
		}

	}
	return GOOD;
}

/*
**************************************************************************
** get_lconv()
**
** description:
**	fetch elements of the lconv structure
**	convert character elements to strings
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
get_lconv( info)
LCONVINFO *info;
{

	LCONV *lc;

	if ((lc = localeconv()) == (LCONV *) NULL) {
		return BAD;
	}

	info[0].ele.str = lc->decimal_point;
	info[1].ele.str = lc->thousands_sep;
	info[2].ele.str = lc->grouping;
	info[3].ele.str = lc->int_curr_symbol;
	info[4].ele.str = lc->currency_symbol;
	info[5].ele.str = lc->mon_decimal_point;
	info[6].ele.str = lc->mon_thousands_sep;
	info[7].ele.str = lc->mon_grouping;
	info[8].ele.str = lc->positive_sign;
	info[9].ele.str = lc->negative_sign;
	info[10].ele.chr[0] = lc->int_frac_digits;
	info[10].ele.chr[1] = (UCHAR) NULL;
	info[11].ele.chr[0] = lc->frac_digits;
	info[11].ele.chr[1] = (UCHAR) NULL;
	info[12].ele.chr[0] = lc->p_cs_precedes;
	info[12].ele.chr[1] = (UCHAR) NULL;
	info[13].ele.chr[0] = lc->p_sep_by_space;
	info[13].ele.chr[1] = (UCHAR) NULL;
	info[14].ele.chr[0] = lc->n_cs_precedes;
	info[14].ele.chr[1] = (UCHAR) NULL;
	info[15].ele.chr[0] = lc->n_sep_by_space;
	info[15].ele.chr[1] = (UCHAR) NULL;
	info[16].ele.chr[0] = lc->p_sign_posn;
	info[16].ele.chr[1] = (UCHAR) NULL;
	info[17].ele.chr[0] = lc->n_sign_posn;
	info[17].ele.chr[1] = (UCHAR) NULL;

	return GOOD;
}

/*
**************************************************************************
** lc_all()
**
** description:
**	display LC_CTYPE, LC_CONV, LC_TIME, LC_NUMERIC, LC_MONETARY
**	and other information.
**	entry point for -a option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
lc_all()
{
	static PFI routine[] = {
		lc_ctype,	/* LC_CTYPE */
		lc_collate,	/* LC_COLLATE */
		lc_numeric,	/* LC_NUMERIC */
		lc_time,	/* LC_TIME */
		lc_monetary,	/* LC_MONETARY */
		other,		/* other misc info */
		(PFI) NULL,	/* must be last entry */
	};
	PFI *p;

	for (p = &routine[0] ; *p ; p++) {
		if ((*p)( ) == BAD) {
			return BAD;
		}
	}
	return GOOD;
}

/*
**************************************************************************
** code_set()
**
** description:
**	display code set information in machine collation order.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
code_set()
{
	static CP fmt[] = {
"COMMENT: code set format strings",		/* catgets 1000 */
" Code Set Values\n",				/* catgets 1001 */
" ===============\n",				/* catgets 1002 */
" chr     hex     oct     dec\n",		/* catgets 1003 */
" ===========================\n",		/* catgets 1004 */
" %1$3s%2$8x%2$8o%2$8d\n",			/* catgets 1005 */
"   chr     hex     oct     dec\n",		/* catgets 1006 */
" ===============================\n",		/* catgets 1007 */
" %1$6s%2$7x%2$9o%2$8d\n",			/* catgets 1008 */
	};
	UINT lmsb;		/* lower most-significant byte */
	UINT umsb;		/* upper most-significant byte */
	UINT msb, lsb;		/* character most-least significant byte */
	UINT ch;		/* character to display */
	UINT lower1, upper1;	/* single-byte lower-upper bound */
	UINT lower2, upper2;	/* multi-byte lower-upper bound */
	UINT lines;		/* number of lines displayed */
	Error = "code_set";	/* routine error message string */
	
	/* set up the boundaries */
	if (bounds( &lower1, &upper1, &lower2, &upper2) == BAD) {
		return BAD;
	}

	/* put out the title */
	TITLE( fmt, COD, 1, TRUE);

	/* print single-byte characters */
	lines = 0;
	for (ch = lower1 ; ch <= upper1 ; ch++) {
		if (((ch < MAX_1PLUS) && !FIRSTof2(ch))) {
			/* if necessary put out the header */
			HEADER( lines, fmt, COD, 3)

			/* display collation info about the character */
			Printf( fmt, COD, 5, nvis(ch), ch);
		}
	}

	/* print multi-byte characters */
	lines = 0;
	lmsb = lower2 >> 8;
	umsb = upper2 >> 8;
	for (msb=lmsb ; msb <= umsb ; msb++) {
		for (lsb=MIN_1CHAR ; lsb < MAX_1PLUS ; lsb++) {
			if (lower2 > (ch = CH2( msb,lsb))) {
				continue;
			}
			if (upper2 >= ch && FIRSTof2(msb) && SECof2(lsb)) {
				/* if necessary put out the header */
				HEADER( lines, fmt, COD, 6)

				/* display collation info about the character */
				Printf( fmt, COD, 8, nvis(ch), ch);
			}
		}
	}
	return GOOD;
}

/*
**************************************************************************
** list_lang()
**
** description:
**	display installed languages.
**	actually read locale.def
**	entry point for -l option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
list_lang()
{
	static CP fmt[] = {
"COMMENT: list_lang format strings",		/* catgets 1100 */
" Installed Locales\n",				/* catgets 1101 */
" =================\n",				/* catgets 1102 */
" Locales           Modifiers\n",		/* catgets 1103 */
" =======           =========\n", 		/* catgets 1104 */
" %-18s%s%s%s%s\n",				/* catgets 1105 */
" See NLS Concepts and Tutorials for more information about locale names\n",	/* catgets 1106 */
" ======================================================================\n",	/* catgets 1107 */
	};

	static CP cats[] = {		/* LC_ category names */
		"LC_ALL ",
		"LC_COLLATE ",
		"LC_CTYPE ",
		"LC_MONETARY ",
		"LC_NUMERIC ",
		"LC_TIME ",
	};
	UCHAR cffn[MAX_FNAME];		/* config path */
	UCHAR lcfn[MAX_FNAME];		/* locale.def path */
	UCHAR buf[MAX_FNAME];		/* config file buffer */
	UCHAR lcname2[MAX_FNAME];	/* locale name with terr - cs dir */
	UCHAR table[TBL_SIZE];		/* locale.def table */
	TABHDR *table_hdr;		/* locale header ptr */
	CATMOD *cat_hdr_ptr;		/* category-modiifer ptr */
	UCP lcname;			/* locale name */
	UCP p1;				/* loop ptr */
	UCP p2;				/* loop ptr */
	FILE *fp;			/* file ptr of config */
	int fd = -1;			/* file desc of locale.def */
	UINT have_tab;			/* true if loacale has locale.def */
	UINT first_mod;			/* true if first modifier */
	UINT search_mod;		/* true if must search for modifiers */
	UINT num_mod;			/* number of modifiers */
	UINT catid;			/* category id of locale */
	UINT cat_mod_size;		/* size of category-modifier */
	UINT lines = 0;			/* num lines displayed */

	int retval = GOOD;		/* assume a good return */
	Error = "list_lang";		/* routine error message string */

	/* get config filename & open it*/
	(void) strcpy( cffn, NLSDIR);
	(void) strcat( cffn, CONFIG);
	if ((fp = fopen( cffn, "r")) == (FILE *) NULL) {
		error( WARNING, BAD_CONFIG);
		return BAD;
	}
	
	/* put out the title */
	TITLE( fmt, LIS, 1, TRUE);

	/* read the config file until EOF */
	while (fgets( buf, MAX_FNAME, fp)) {

		/* clobber newline at end */
		buf[strlen(buf)-1] = (UCHAR) NULL;

		/* scan past langid and white space */
		for (lcname = buf; isdigit( *lcname) || isspace( *lcname) ; lcname++) ;

		/* if -L then display one locale */
		if (Lflag) {
			if (strncmp( lcname, Lang, strlen( lcname))) {
				continue;
			}
		}

		have_tab = FALSE;
		if (strcmp( lcname, "n-computer") && strcmp( lcname, "C") &&
		    strcmp( lcname, "POSIX")) {
			have_tab = TRUE;

			/* handle territory - codeset */
			for (p1=lcname, p2=lcname2 ; *p1 ; p1++, p2++) {
				if (*p1 == '_') {
					/* have a territory */
					*p2 = '/';
				}
				else if (*p1 == '.') {
					/* have a codeset */
					*p2 = '/';
				}
				else {
					/* copy locale name */
					*p2 = *p1;
				}
			}
			*p2 = '\0';

			/* get locale.def filename & open it*/
			(void) strcpy( lcfn, NLSDIR);
			(void) strcat( lcfn, lcname2);
			(void) strcat( lcfn, LOCALE);
			if ((fd = open( lcfn, O_RDONLY)) < 0) {
				error( WARNING, BAD_INSTALL, lcname);
				continue;
			}

			/* read locale table header */
			if (read( fd, table, HDR_SIZE) < HDR_SIZE) {
				return BAD;
			}
			table_hdr = (TABHDR *)table;

			/* get size of header and category-modifier structures */
			cat_mod_size = (table_hdr->cat_no + table_hdr->mod_no) * sizeof(CATMOD);

			/* read locale category-modifier structures */
			if (read( fd, table, cat_mod_size) < cat_mod_size) {
				return BAD;
			}
		}

		/* display modifiers */
		first_mod = TRUE;
		num_mod = 0;
		for (catid = 0 ; catid < N_CATEGORY ; catid++) {
			cat_hdr_ptr = have_tab ? (CATMOD *)(table + catid * sizeof( CATMOD)) : (CATMOD *) NULL;
			search_mod = TRUE;
			while (search_mod) {
				for (;;) {
					if (cat_hdr_ptr && *cat_hdr_ptr->mod_name) {
						HEADER( lines, fmt, LIS, 3)
						Printf( fmt, LIS, 5,
							first_mod ? lcname : SP, cats[catid], lcname, "@", cat_hdr_ptr->mod_name);
						first_mod = FALSE;
						num_mod++;
					}
		        		if (!cat_hdr_ptr || !cat_hdr_ptr->mod_addr) {
						search_mod = FALSE;
						break;
					}
		       	   		cat_hdr_ptr = (CATMOD *)(table + cat_hdr_ptr->mod_addr - sizeof( TABHDR));
				}
			}
		}
		if (!num_mod) {
			HEADER( lines, fmt, LIS, 3)
			Printf( fmt, LIS, 5, lcname, SP, SP, SP, SP);
		}
		if (fd > -1)
			close(fd);
	}

	/* put out the disclaimer */
	TITLE( fmt, LIS, 6, FALSE);

	fclose (fp);

	return retval;
}

/*
**************************************************************************
** list_conv()
**
** description:
**	display installed conversion code set tables
**	entry point for -i option.
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
list_conv( )
{

	static CP fmt[] = {
"COMMENT: iconv codeset name format strings",	/* catgets 1600 */
" Installed Codeset Conversions\n",		/* catgets 1601 */
" =============================\n", 		/* catgets 1602 */
" Fromname            Toname\n",		/* catgets 1603 */
" ========            ======\n", 		/* catgets 1604 */
" %-20s%-20s\n",				/* catgets 1605 */
" Names made up of first four and last letters of the code set name\n",	/* catgets 1606 */
" =================================================================\n",	/* catgets 1607 */
	};
	UCHAR cmd[MAX_PNAME];		/* command to find directory names */
	UCHAR buf[MAX_PNAME];		/* buffer to read the pipe */
	UCHAR fromname[MAX_FNAME];	/* from codeset name */
	UCHAR toname[MAX_FNAME];	/* to codeset name */
	UCP p1;				/* ptr to fromcode name */
	UCP p2;				/* ptr to tocode name */
	FILE *fp;			/* used to read pipe */
	UINT no_name;			/* flags full path name */
	UINT endd;			/* num bytes in code conv directory */
	UINT lines = 0;			/* num lines displayed */
	Error = "list_conv";		/* routine error message string */

	/* set up the find command */
	(void) strcat( strcat( strcat( strcpy( cmd, "find "), ICONV), "* "), "-print");

	/* get index of end of conversion table directory name */
	endd = (UINT) strlen( ICONV);

	/* open up a pipe to command output */
	if ((fp = popen( cmd, "r")) == NULL) {
		Perror( "list_conv -- popen");
		return BAD;
	}

	/* put out the title */
	TITLE( fmt, LIC, 1, TRUE);

	/* read the pipe until end of file */
	while (fgets( buf, MAX_PNAME, fp)) {

		/* clobber newline at end */
		buf[strlen(buf)-1] = (UCHAR) NULL;

		/* get fromname and toname */
		no_name = TRUE;
		for (p1=p2=(&buf[endd]); *p2 ; p2++) {
			if (*p2 == '}') {
				no_name = FALSE;
				break;
			}
		}
		if (no_name) {
			/* only directory name */
			continue;
		} 
		else {
			/* full file name with fromcode/tocode */
			*p2++ = '\0';
			(void) strcpy( fromname, p1);
			(void) strcpy( toname, p2);
		}

		/* if necessary put out the header */
		HEADER( lines, fmt, LIC, 3)

		/* display fromcode and tocode names */
		Printf( fmt, LIC, 5, fromname, toname);
	}

	/* put out the disclaimer */
	TITLE( fmt, LIC, 6, FALSE);

	return GOOD;
}
