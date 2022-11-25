/* @(#) $Revision: 64.2 $ */   

/*
**************************************************************************
** Include Files
**************************************************************************
*/

#include <nl_types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <varargs.h>
#include "justify.h"
#include "extern.h"

/*
**************************************************************************
** Constants
**************************************************************************
*/

#define NL_SETN		1		/* message set number */

/*
**************************************************************************
** External references
**************************************************************************
*/

extern char *catgets();			/* get message from catalog */
extern void free();			/* free memory */
extern void exit();			/* quit program */

/*
**************************************************************************
** Forward references
**************************************************************************
*/

extern void put_message();		/* put message on stderr */

/*
**************************************************************************
** Get the line from input stream
**************************************************************************
*/

int
get_line(buf1,buf2)
UCHAR *buf1;
UCHAR *buf2;
{
	register int retval = TRUE;

	InBuf = buf1; OutBuf = buf2;

	if (HaveWrap) {
		/* get input from a wrap */
		(void) strcpy(InBuf,WrapBuf);
		Len = strlen(InBuf);
	} else if (!(fgets(InBuf,MAX_LINE+1,Input))) {
		/* 0 chars read or EOF */
		retval = FALSE;
	} else if (isprint((int)InBuf[Len=strlen(InBuf)-1])) {
		/* input buffer overflow: terminate program */
		put_message(FATAL,IN_OVERFLOW);
	} else {
		/* have a good line from input stream */
		/* replace last character (usually newline) with null */
		LastChar = InBuf[Len];
		InBuf[Len] = EOL;
	}
	return retval;
}

/*
**************************************************************************
** Put Line Into Output Stream
**************************************************************************
*/

void
put_line()
{
	/* designate hebrew or arabic as primary char set */
	(void) fputs(Primary,stdout);

	if (Lang == ARABIC) {
		/* designate ascii as secondary char set */
		(void) fputs(Secondary,stdout);
	} else {
		/* select primary set */
		(void) fputc(SI,stdout);
	}

	/* put out the print line */
	(void) fputs(OutBuf,stdout);

	/* put out last character (usually newline) */
	if (NewLine) (void) fputc(LastChar,stdout);
}

/*
**************************************************************************
** Error Message Strings
**************************************************************************
*/

static char *mess[] = {
	"\"%s\" not a right-to-left language\n",				/* catgets 1 */
	"non-latin mode file with left justification\n",			/* catgets 2 */
	"latin mode file with right justification\n",				/* catgets 3 */
	"out of memory\n",							/* catgets 4 */
	"input buffer overflow\n",						/* catgets 5 */
	"output buffer overflow\n",						/* catgets 6 */
	"print width of \"%s\" exceeds input buffer length\n",			/* catgets 7 */
	"\"%s\" invalid print width\n",						/* catgets 8 */
	"wrap margin of \"%s\" exceeds input buffer length\n",			/* catgets 9 */
	"\"%s\" invalid wrap margin\n",						/* catgets 10 */
	"wrap margin changed to \"%s\" (can not exceed print width)\n",		/* catgets 11 */
	"tab stop of \"%s\" exceeds input buffer length\n",			/* catgets 12 */
	"\"%s\" invalid tab stop\n",						/* catgets 13 */
	"unable to open input file \"%s\"\n",					/* catgets 14 */
	"\"%s\" invalid justification (l or r)\n",				/* catgets 15 */
	"\"%s\" invalid mode (l or n)\n",					/* catgets 16 */
	"\"%s\" invalid order (k or s)\n",					/* catgets 17 */
	"usage: nljust [-aclnt] [-e seq] [-j just] [-m mode] [-o order] [-r margin] [-w width] [-x ck] [files ...]\n",	/* catgets 18 */
};

/*
**************************************************************************
** put_message()
**
** description:
**	display error message on stderr and leave if fatal
**
** assumptions:
**	all errors go to stderr
**	only one message set
** 
** global variables:
**	Cmd: char pointer to the program name
**	mess: array of char pointers to format string messages
**
** return value:
**	no return value
**************************************************************************
*/

/* VARARGS 2 */

void
put_message( fatal, num, va_alist)
int fatal;			/* Warning or Fatal error */
int num;			/* message number */
va_dcl				/* optional arguments */
{
	char *fmt;		/* ptr to format string */
	va_list args;		/* points to optional argument list */

	/* sync stdout with stderr */
	(void) fflush( stdout);

	/* print program name on stderr */
	(void) fprintf( stderr, "%s: ", Cmd);

	/* get the format string */
	fmt = catgets( Catd, NL_SETN, num, mess[num-1]);

	/* set up the optional argument list */
	va_start( args);

	/* print the error message on stderr */
	(void) vfprintf( stderr, fmt, args);

	/* close down the optional argument list */
	va_end( args);

	/* leave if a fatal error */
	if (fatal) {
		(void) catclose(Catd);		/* close the message catalog */
		(void) fclose(Input);		/* close input file */
		if (Escape) free(Primary);	/* free up font esc seq */
		exit(UNSUCCESSFUL);
	}
}
