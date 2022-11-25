static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/*
***************************************************************************
*** include files
***************************************************************************
*/

#include <stdio.h>		/* input - output */
#include <string.h>		/* string function declarations */
#include <varargs.h>		/* variable arguments */
#include <nl_types.h>		/* for nl_catd */
#include <nl_ctype.h>		/* for nl_tools_16 */
#include <fcntl.h>		/* for O_RDONLY */
#include <locale.h>		/* for setlocale */
#include <iconv.h> 	        /* for conversion header file */

/*
***************************************************************************
*** external declarations
***************************************************************************
*/

extern nl_catd catopen();	/* open message catalog */
extern char *catgets();		/* get message from catalog */
extern int catopen();		/* close message catalog */
extern char *_errlocale();	/* get bad locale settings */
extern void perror();		/* system error messages */
extern void exit();		/* leave */
extern void *malloc();		/* allocate memory */
extern void free();		/* free memory */
extern char *optarg;		/* pointer to start of option arg */
extern int optind;		/* argv index of next arg */
extern int opterr;		/* error message indicator */
extern int errno;		/* error number */
extern int sys_nerr;		/* maximum error number */
extern char *_Dir;		/* translation table directory */

/*
***************************************************************************
*** forward references
***************************************************************************
*/

extern void Perror();		/* local system print error message */
extern void error();		/* local system error message */
extern void map_warn();		/* conversion warning error message */
extern char *get_basename();	/* get basename of command name */

/*
***************************************************************************
*** constants
***************************************************************************
*/

#define	WARNING		0	/* warning error message */
#define	FATAL		1	/* fatal error message */
#define	GOOD		0	/* successful return value */
#define	BAD		-1	/* unsuccessful return value */
#define	TRUE		1	/* boolean true */
#define	FALSE		0	/* boolean false */
#define	DEF1		0xff	/* single-byte default character */
#define	DEF2		0xffff	/* double-byte default character */
#define	MAX_ERR		256	/* max Perror message length */
#define	MAX_FMT		128	/* max message format length */
#define	MAX_WARN	128	/* max message warning length */

/*
***************************************************************************
*** special code set names
***************************************************************************
*/

#define J15		"japa5"
#define SJIS		"sjis"
#define R8_1		"roman8"
#define R8_2		"roma8"
#define ISO_1		"iso8859_1"
#define ISO_2		"iso81"

/*
***************************************************************************
*** return values from ICONV
***************************************************************************
*/

#define IN_CHAR_TRUNC	1
#define BAD_IN_CHAR	2
#define OUT_CHAR_TRUNC	3

/*
***************************************************************************
*** types
***************************************************************************
*/

typedef unsigned char UCHAR;	/* prevent sign extention on char */
typedef unsigned int UINT;	/* prevent sign extention on int */
typedef struct {
	UINT ch1;
	char *s1;
	UINT ch2;
	char *s2;
} MAP;

/*
***************************************************************************
*** error message numbers
***************************************************************************
*/

#define	NL_SETN		1	/* message catalog set number */
#define BAD_USAGE	1	/* usage error message */ 
#define BAD_SIZE	2	/* iconvsize */
#define BAD_CREATE	3	/* malloc */
#define BAD_OPEN	4	/* iconvopen */
#define BAD_CONVERSION	5	/* iconv */
#define BAD_CLOSE	6	/* iconvclose */
#define MAP_CH1		7	/* map character warning */
#define MAP_CH2		8	/* map character warning */
#define MAP_CH3		9	/* map character warning */
#define MAP_S1		10	/* map string warning */
#define MAP_S2		40	/* map string warning */

/*
**************************************************************************
** error message strings
**************************************************************************
*/

static char * Message[] = {
	"usage: %s -f fromcode -t tocode [files ...]\n",	/* catgets 1 */
	"can not find the size of the conversion table\n",	/* catgets 2 */
	"no room for conversion table\n",			/* catgets 3 */
	"can not initialize the conversion\n",			/* catgets 4 */
	"bad input character\n",				/* catgets 5 */
	"can not close the conversion\n",			/* catgets 6 */
	"%s to %s conversion maps the following characters:\n",	/* catgets 7 */
	"ROMAN8: \\x%x %-25sISO8859/1: \\x%x %-25s\n",		/* catgets 8 */
	"ISO8859/1: \\x%x %-25sROMAN8: \\x%x %-25s\n",		/* catgets 9 */
};

/*
**************************************************************************
** iso8859/1 <--> roman8 warnings
**************************************************************************
*/

static MAP chrs1[] = {
	{
	0xa0,  "NA (Do not use)",			/* catgets 10 */
	0xa0,  "No break space"				/* catgets 11 */
	},
	{
	0xa9,  "Accent grave",				/* catgets 12 */
	0x60,  "Left single quote"			/* catgets 13 */
	},
	{
	0xaa,  "Circumflex accent",			/* catgets 14 */
	0x5e,  "Caret"					/* catgets 15 */
	},
	{
	0xac,  "Tilde accent",				/* catgets 16 */
	0x7e,  "Tilde"					/* catgets 17 */
	},
	{
	0xaf,  "Italian lira symbol",			/* catgets 18 */
	0xa3,  "Pound sign"				/* catgets 19 */
	},
	{
	0xb0,  "Overline",				/* catgets 20 */
	0xaf,  "Macron"					/* catgets 21 */
	},
	{
	0xbe,  "Dutch guilder",				/* catgets 22 */
	0x66,  "Lowercase f"				/* catgets 23 */
	},
	{
	0xeb,  "Uppercase S caron",			/* catgets 24 */
	0x53,  "Uppercase S"				/* catgets 25 */
	},
	{
	0xec,  "Lowercase s caron",			/* catgets 26 */
	0x73,  "Lowercase s"				/* catgets 27 */
	},
	{
	0xee,  "Uppercase Y umlaut",			/* catgets 28 */
	0x59,  "Uppercase Y"				/* catgets 29 */
	},
	{
	0xf6,  "Horizonal bar",				/* catgets 30 */
	0x2d,  "Hyphen"					/* catgets 31 */
	},
	{
	0xfc,  "Solid box",				/* catgets 32 */
	0x2a,  "Asterisk"				/* catgets 33 */
	},
	{
	0xff,  "NA (Do not use)",			/* catgets 34 */
	0xa0,  "No break space"				/* catgets 35 */
	},
	{ 0, "", 0, ""},
};

static MAP chrs2[] = {
	{
	0xa0,  "No break space",			/* catgets 40 */
	0xa0,  "NA (Do not use)"			/* catgets 41 */
	},
	{
	0xa6,  "Broken bar",				/* catgets 42 */
	0x7c,  "Vertical line"				/* catgets 43 */
	},
	{
	0xa9,  "Copyright sign",			/* catgets 44 */
	0x63,  "Lowercase c"				/* catgets 45 */
	},
	{
	0xac,  "Not sign",				/* catgets 46 */
	0x7e,  "Tilde"					/* catgets 47 */
	},
	{
	0xad,  "Soft hyphen",				/* catgets 48 */
	0x2d,  "Hyphen"					/* catgets 49 */
	},
	{
	0xae,  "Registered Trade Mark",			/* catgets 50 */
	0x52,  "Uppercase R"				/* catgets 51 */
	},
	{
	0xaf,  "Macron",				/* catgets 52 */
	0xb0,  "Top bar"				/* catgets 53 */
	},
	{
	0xb2,  "Superscript two",			/* catgets 54 */
	0x32,  "Digit two"				/* catgets 55 */
	},
	{
	0xb3,  "Superscript three",			/* catgets 56 */
	0x33,  "Digit three"				/* catgets 57 */
	},
	{
	0xb8,  "Cedilla",				/* catgets 58 */
	0x2c,  "Comma"					/* catgets 59 */
	},
	{
	0xb9,  "Superscript one",			/* catgets 60 */
	0x31,  "Digit one"				/* catgets 61 */
	},
	{
	0xd7,  "Multiplication sign",			/* catgets 62 */
	0x78,  "Lowercase x"				/* catgets 63 */
	},
	{
	0xf7,  "Division sign",				/* catgets 64 */
	0x2f,  "Solidus"				/* catgets 65 */
	},
	{ 0, "", 0, ""},
};

/*
*****************************************************************************
** global variables
*****************************************************************************
*/

static char *Progname;		/* program name */
static nl_catd Catd;		/* message catalog descriptor */
static int Input;		/* input file descriptor */
static char **Filename;		/* ptr to ptr to current file name */
static char *Fromcode;		/* ptr to "from" code set name */
static char *Tocode;		/* ptr to "to" code set name */
static char Japan5[] = J15;	/* switch for sjis name */

/*
**************************************************************************
** main()
**
** description:
**	driver routine for program
**
** assumptions:
**	all input come from stdin or named files
**	all output goes to stdout
**	all errors go to stderr
** 
** global variables:
**	Input: FILE descriptor to the current input file
**	Filename: ptr to ptr to current file name
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/


main(argc, argv)
int argc;			/* initial argument count */
char **argv;			/* ptr to ptr to first program argument */
{

	/* assume a sucessful return  value*/
	register int retval = GOOD;

	/* initialize, parse cmd line options, etc. */
	if (start( argc, argv) < 0) {
		retval = BAD;
	}
	
	/* open and process input files one at a time */

	for ( ; *Filename ; Filename++) {

		/* open input file and get next if can't open */
		if (!strcmp( *Filename, "-")) {
			Input = 0;
		}
		else if ((Input = open( *Filename, O_RDONLY)) < 0) {
			Perror( "open");
			retval = BAD;
			continue;
		}

		/* process the file */
		if (process( ) < 0) {
			retval = BAD;
		}

		/* close input file unless it's stdin */
		if (! Input) {
			if (close( Input) < 0) {
				Perror( "close");
				retval = BAD;
			}
		}
	}

	/* end the program */
	if (finish( ) < 0) {
		retval = BAD;
	}
	return retval;
}

/*
**************************************************************************
** process()
**
** description:
**	create the conversion table
**	convert the characters
**	print out the converted characters
**	delete the conversion table
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
process()
{ 
	register iconvd cd;		/* conversion descriptor */
	register int size;		/* size of translation table */
	register UCHAR *table;		/* ptr to translation table */

	register int bytesread;		/* num bytes read into input buffer */

	UCHAR inbuf[BUFSIZ];		/* input buffer */
	UCHAR *inchar;			/* ptr to input character */
	int inbytesleft;		/* num bytes left in input buffer */

	UCHAR outbuf[BUFSIZ];		/* output buffer */
	UCHAR *outchar;			/* ptr to output character */
	int outbytesleft;		/* num bytes left in output buffer */

	/* create conversion table */
	if ((size = iconvsize( Tocode, Fromcode)) == BAD) {
		error( FATAL, BAD_SIZE);
	}
	else if (size == 0) {
		table = (UCHAR *) NULL;
	}
	else if ((table = (UCHAR *) malloc ( (unsigned int) size)) == (UCHAR *) NULL) {
		error( FATAL, BAD_CREATE);
	}

	/* start up a conversion */
	if ((cd = iconvopen( Tocode, Fromcode, table, DEF1, DEF2)) == (iconvd) BAD) {
		error( FATAL, BAD_OPEN);
	}

	inchar = inbuf;
	inbytesleft = 0;
	outchar = outbuf;
	outbytesleft = BUFSIZ;

	for ( ;; ) {

		/* translate the characters */
		switch (ICONV( cd, &inchar, &inbytesleft, &outchar, &outbytesleft)) {
		case GOOD:
		case IN_CHAR_TRUNC:
			/*
			** Empty buffer or character spans input buffer
			** boundary.  Move any remaining stuff to start
			** of buffer, get more characters and reinitialize
			** input variables.  If at EOF, flush output buffer
			** and leave; otherwise, convert the characters.
			*/
			(void) strncpy( inbuf, inchar, inbytesleft);
			if ((bytesread = read( Input, inbuf+inbytesleft, BUFSIZ-inbytesleft)) < 0) {
				Perror( "read");
				return BAD;
			}
			if (! (inbytesleft += bytesread)) {
				if (write( 1, outbuf, BUFSIZ - outbytesleft) < 0) {
					Perror( "write");
					return BAD;
				}
				goto END_CONVERSION;
			}
			inchar = inbuf;
			break;
		case BAD_IN_CHAR:
			error( FATAL, BAD_CONVERSION);
		case OUT_CHAR_TRUNC:
			/*
			** Full buffer or output character spans output buffer
			** boundary.  Send the output buffer to stdout,
			** reinitialize the output variables.
			*/
			if (write( 1, outbuf, BUFSIZ - outbytesleft) < 0) {
				Perror( "write");
				return BAD;
			}
			outchar = outbuf;
			outbytesleft = BUFSIZ;
		}
	}
END_CONVERSION:

	/* end conversion & get rid of the conversion table */
	if (iconvclose( cd) == BAD) {
		error( FATAL, BAD_CLOSE);
	}
	if (size) {
		free( (void *) table);
	}

	return GOOD;
}

/*
**************************************************************************
** start() 
**
** description:
**	set up language tables
**	open message catalogs
**	parse command line
**	set up global variables
** 
** global variables:
**	Catd: nl_catd message catalog descriptor
**	Progname: char pointer to the program name
**	Filename: pointer to pointer to current file name
**
** return value:
**	0: everything went ok
**	-1: had some trouble
**************************************************************************
*/

static int
start( argc, argv)
int argc;				/* current argument count */
char **argv;				/* ptr to ptr to current argument */
{
	register int optchar;		/* option character */
	static char *deffiles[] = { "-", (char*) NULL };
					/* default file name */

	/* get the program base name */
	Progname = get_basename( *argv);

	/* get language & initialize environment table */
	if (!setlocale( LC_ALL, "")) {
		/* bad initialization */
		(void) fputs( _errlocale(), stderr);
		Catd = (nl_catd) -1;
		(void) putenv( "LANG=");	/* for perror */
	}
	else {
		/* good initialization: open message catalog,
		   ... keep on going if it isn't there */
		Catd = catopen( "iconv", 0);
	}

	/* parse command line options */

	opterr = 0;		/* disable getopt error message */
	while ((optchar = getopt( argc, argv, "f:t:T:")) != EOF) {
		switch (optchar) {
		case 'f':	/* "from" name */
			Fromcode = optarg;
			break;
		case 't':	/* "to" name */
			Tocode = optarg;
			break;
		case 'T':	/* undocumented test directory option */
			_Dir = optarg;
			break;
		case '?':	/* unrecognized option */
			error( FATAL, BAD_USAGE, Progname);
		}
	}
	
	/* must have both a "from" and "to" name */
	if (! (Fromcode && Tocode)) {
		error( FATAL, BAD_USAGE, Progname);
	}

	/* "sjis" same as "japanese15" */
	if ( !strcmp( Fromcode, SJIS)) {
		Fromcode = &Japan5[0];
	}
	if ( !strcmp( Tocode, SJIS)) {
		Tocode = &Japan5[0];
	}

	/* warning for "roman8" <--> "iso8859" conversion */
	if( (!strcmp( Fromcode, R8_1) || !strcmp( Fromcode, R8_2))
	  && (!strcmp( Tocode, ISO_1) || !strcmp( Tocode, ISO_2))) {
		map_warn( R8_1, ISO_1, chrs1);
	}
	else if( (!strcmp( Tocode, R8_1) || !strcmp( Tocode, R8_2))
	   && (!strcmp( Fromcode, ISO_1) || !strcmp( Fromcode, ISO_2))) {
		map_warn( ISO_1, R8_1, chrs2);
	}

	/* set up input file arguments */
	Filename = ((argc - optind) < 1) ? deffiles : argv + optind ;

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
** finish() 
**
** description:
**	get ready to leave: close message catalogs
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
	(void) catclose( Catd);

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
char *rname;			/* bad routine name */
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
**	Catd: nl_catd message catalog descriptor
**	Progname: char pointer to the program name
**	Input: FILE pointer to the current input file
**	Filename: pointer to pointer to current file name
**	Message: array of char pointers to format string messages
**
** return value:
**	no return value
**************************************************************************
*/

/* VARARGS 2 */

static void
error( fatal, num, va_alist)
int fatal;			/* Warning or Fatal error */
int num;			/* message number */
va_dcl				/* optional arguments */
{
	register char *fmt;	/* points to format string */
	va_list args;		/* points to optional argument list */

	/* set up the optional argument list */
	va_start( args);

	/* sync stdout with stderr */
	if (fflush( stdout)) {
		Perror( "fflush");
	}

	/* get the message format string */
	fmt = catgets( Catd, NL_SETN, num, Message[num-1]);

	/* print the program name on stderr */
	if (fprintf( stderr, "%s: ", Progname) < 0) {
		Perror( "fprintf");
	}

	/* print the error message on stderr */
	if (vfprintf( stderr, fmt, args) < 0) {
		Perror( "vfprintf");
	}

	/* close down the optional argument list */
	va_end( args);

	/* leave if a fatal error */
	if (fatal) {
		(void) finish( );
		if (close( Input) < 0) {
			Perror( "close");
		}
		exit( BAD);
	}
}

/*
**************************************************************************
** map_warn()
**
** description:
**	display iso8859/1 <--> roman8 warning on stderr
**
** return value:
**	no return value
**************************************************************************
*/

static void
map_warn( from, to, chrs)
char *from;			/* from code name */
char *to;			/* to code name */
MAP *chrs;			/* warning messages */
{
	MAP *p;			/* ptr to character mappings */
	int num;		/* message number */
	int wnum;		/* another message number */
	char fmt[MAX_FMT];	/* format string buffer */
	char warn1[MAX_WARN];	/* warning message buffer */
	char *warn2;		/* warning message pointer */

	/* print what conversion we have */
	error( WARNING, MAP_CH1, from, to);

	/* get message numbers */
	if (chrs == chrs1) {
		num = MAP_CH2;
		wnum = MAP_S1;
	}
	else {
		num = MAP_CH3;
		wnum = MAP_S2;
	}
	
	/* get format string */
	(void) strncpy( fmt, catgets( Catd, NL_SETN, num, Message[num-1]), MAX_FMT);

	/* print the character mapping for all characters */
	for (p=chrs ; p->ch1 ; p++, wnum+=2) {
		(void) strncpy( warn1, catgets( Catd, NL_SETN, wnum, p->s1), MAX_WARN);
		warn2 = catgets( Catd, NL_SETN, wnum+1, p->s2);
		if (fprintf( stderr, fmt,  p->ch1, warn1, p->ch2, warn2) < 0) {
			Perror( "fprintf");
		}
	}
}
