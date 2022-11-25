%{
static char *HPUX_ID = "@(#) $Revision: 64.2 $";
/*
** Description:
**	Findstr examines files of  C  source  code  for  uncommented
**	string constants, which it places along with the surrounding
**	quotes on the standard output, preceding each  by  the  file
**	name,  start position, and length.  This information will be
**	used by insertmsg.  Strings used with  "catgets",  "catopen"
**	and "nl_msg" are ignored.
** Assumptions:
**	1. Only C source code is supported.
**	2. Two-byte characters can only appear within string literals
**	   or comments.
** Possible Future Enhancements:
**	Support other programming languages thru start conditions
**	(C PASCAL FORTRAN, etc.).
*/

#include <string.h>

#if defined NLS || defined NLS16
#include <nl_types.h>
#include <locale.h>
#define NL_SETN 1
#endif

#if defined NLS16
#include <nl_ctype.h>
#else
#define FIRSTof2(c)	(0)
#define SECof2(c)	(0)
#define PCHARADV(c,p)	(*p++ = c)
#endif

#undef YYLMAX
#define YYLMAX	10000

#define TRUE	1
#define FALSE	0

char *csource;			/* points to source file name */
int curpos;			/* byte offset from start of input file */
%}

NAME		(catgets|catopen|nl_msg)
SPACE		[\ \t\n]*

%%

^.*{NAME}{SPACE}\(		do_args(yyleng);
^.*{NAME}{SPACE}\/\*		{ do_comment(); do_args(0); }
\'\"\'				curpos += 3;
\'\\\"\'				curpos += 4;
\"				do_string();
\/\*				do_comment();
.				curpos++;
\n				curpos++;

%%

/*
** do_string: process string literals by printing on stdout:
**	1. source file name
**	2. byte offset of string from start of file
**	3. length of string
**	4. the string itself
** Implementation note:
**	1. get input characters from getC to prevent byte
**	   re-definition problems (getC always returns a one-
**	   or two-byte character)
** 	2. store input characters in yytext
**	3. if input character is a slash, set a flag indicating
**	   that the next character is escaped
**	4. if the input character is a double-quote (that's not
**	   escaped), then terminate the string loop
**	5. make sure string is null terminated
**	6. print information (described above) to stdout
**	7. add string length to the byte offset
*/

do_string()
{
	extern int getC();

	register int ch;
	register int slen;
	register int slash;
	register unsigned char *p;

	p = yytext;
	slash = FALSE;
	while (ch = getC()) {
		PCHARADV(ch,p);
		if (ch == '\\' && !slash) {
			slash = TRUE;
		} else if (ch == '"' && !slash ) {
			break;
		} else if (slash) {
			slash = FALSE;
		}
	}
	*p = '\0';
	slen = strlen((char *)yytext) + 1;
	printf("%s %d %d \"%s\n",csource,curpos,slen,yytext);
	curpos += slen;
}

/*
** do_comment: bypass comments by incrementing the byte offset
**	from the start of the file.
** Implementation note:
**	1. increment byte offset past slash-star combination
**	2. get input characters from getC to prevent byte
**	   re-definition problems (getC always returns a one-
**	   or two-byte character)
**	3. increment byte offset taking account of one- or
**	   two-byte characters
**	4. if input character is a star, set a flag indicating
**	   that the next character might end the comment
**	5. if the input character is a slash and star flag set,
**	   then terminate the comment loop
*/

do_comment()
{
	register int ch;
	register int star;

	curpos += yyleng;
	star = FALSE;
	while (ch = getC()) {
		curpos += (ch < 256) ? 1 : 2;
		if (ch == '*') {
			star = TRUE;
		} else if (ch == '/' && star ) {
			break;
		} else if (star) {
			star = FALSE;
		}
	}
}

/*
** do_args: skip to end of arguments; i.e. to uncommented ")", handle the
**	continuation convention '\\' '\n', increment byte offset
** Implementation note:
**	1. get input characters from getC to prevent byte
**	   re-definition problems (getC always returns a one-
**	   or two-byte character)
**	2. increment byte offset taking account of one- or
**	   two-byte characters
**	3. input character == un-quoted ')', terminate scan
**	4. input character == '"', note string state
**	5. anything else, continue
**	6. NOTE: this assumes that ignored procedure calls have the
**	   form: name ( arg... ) where arg is a variable or a string.
*/

do_args(len)
int len;
{
	register int ch;
	register int quote;

	curpos += len;
	quote = FALSE;
	while (ch = getC()) {
		curpos += (ch < 256) ? 1 : 2;
		if (ch == ')' && !quote) {
			break;
		} else if (ch == '"') {
			quote = !quote;
		}
	}
}

/*
** getC: return a one- or two-byte character as an integer.
** Implementation note:
**	1. get a character from the lex routine input().
**	2. if a 1st-of-2 character, then get another character
**	   from input().
**	3. if the 2nd character is a valid 2nd-of-2, then return 
**	   the 1st-of-2 in the hi byte and the 2nd-of-2 in the lo byte
**	4. if the 2nd character is an invalid 2nd-of-2, then push it
**	   back (using the lex routine unput()) and return the
**	   valid 1st-of-2 as a one-byte character.
**	5. if the character is not a 1st-of-2 then, return it as a
**	   one-byte character.
*/

getC()
{
	register int ch;
	register int c;

	ch = input();
	if (FIRSTof2(ch)) {
		if (SECof2(c = input())) {
			ch = (ch << 8) | c;
		} else {
			unput(c);
		}
	}
	return ch;
}

/*
** main: driver program for findstr.
** Implementation note:
**	1. open message catalog
**	2. initialize language tables
**	3. check for proper usage
**	4. for each input file
**		a. open the file
**		b. exit with a message if can't open file
**		c. get input file name
**		d. initialize byte offset to 0
**		e. using lex function yylex() scan input file
**		   for strings.  See do_string comment to see
**		   what happens when a string is found.
**	5. close message catalog
*/

main( argc, argv )
int argc;
char *argv[];
{
	extern void exit();
	int i;
#if defined NLS || defined NLS16
	extern nl_catd catopen();
	extern char *catgets();

	nl_catd catd;

	if (!setlocale(LC_ALL,"")) {
		fputs(_errlocale("findstr"),stderr);
		catd = (nl_catd)-1;
	}
	else
		catd = catopen("findstr",0);
#endif
	if( --argc < 1 )
	{
#if defined NLS || defined NLS16
		fprintf(stderr,(catgets(catd,NL_SETN,1,"Usage: %s file.c ...\n")),argv[0]);
#else
		fprintf( stderr, "Usage: %s file.c ...\n", argv[0] );
#endif
		exit ( 1 );
	}

	for( i = 1; i <= argc; i++ )
	{
		if( freopen( argv[i], "r", stdin ) == NULL )
		{
#if defined NLS || defined NLS16
			fprintf(stderr,(catgets(catd,NL_SETN,2,"%s : cannot open %s\n")),argv[0],argv[i]);
#else
			fprintf(stderr,"%s : cannot open %s\n",argv[0],argv[i]);
#endif
			exit ( 1 );
		}
		csource = argv[i];
		curpos = 0;
		yylex();
	}
#if defined NLS || defined NLS16
	(void) catclose(catd);
#endif
	exit ( 0 );
}
