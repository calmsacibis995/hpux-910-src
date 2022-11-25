/* @(#) $Revision: 66.1 $ */    
/* LINTLIBRARY */

#include <stdio.h>
#include "error.h"

/* error message array */
static char *messages[] = {
	"invalid keyword",
	"invalid number",
	"invalid character",
	"invalid statement",
	"invalid expression",
	"invalid range",
	"invalid langname length",
	"invalid langid number",
	"invalid language name length in langname",
	"invalid territory name length in langname",
	"invalid codeset name length in langname",
	"invalid revision string length",
	"invalid lowest character code value",
	"invalid character code",
	"invalid isfirst character code",
	"invalid issecond character code",
	"invalid langinfo string length",
	"invalid monetary string length",
	"invalid numeric string length",
	"invalid modifier string length",
	"malloc() failed",
	"priority queue overfow",
	"yes and no strings can't have the same first char",
	"invalid era string length",
	"invalid era string format",
#ifdef EUC
	"too many era strings are specified",
	"invalid code_scheme name length",
	"invalid code_scheme name",
	"invalid cswidth string length",
	"invalid cswidth string format"
#else /* EUC */
	"too many era strings are specified"
#endif /* EUC */
};

/* error:
** Error handling routine for errors encountered when reading buildlang script.
** print error message and exit.
*/
void
error(i)
int i;				/* index to error message array */
{
	extern int fprintf();
	extern void exit();
	extern int lineno;	/* line number from yylex() */

	fprintf(stderr, "buildlang: %s on line %d\n",messages[i],lineno);
	exit(1);
}

/* Error:
** Error handling routine for errors encountered other than reading script.
** print error message and exit.
*/
void
Error(msg)
char *msg;			/* pointer to error message */
{
	extern int fprintf();
	extern void exit();

	fprintf(stderr, "buildlang: ");
	fprintf(stderr, "%s\n", msg);
	exit(1);
}
