/* @(#) $Revision: 64.2 $ */     
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	"global.h"

int	gotstr;

/* numeric_init: initialize monetary table.
** Get here when you see a LC_NUMERIC keyword.
*/
void
numeric_init(token)
int	token;				/* keyword token */
{
	extern void numeric_str();
	extern void numeric_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = numeric_str;		/* set string function for numeric.c */
	finish = numeric_finish;	/* set up numeric finish */

	gotstr = FALSE;			/* no numeric string yet */
}

/* numeric_str: process one numeric string.
** Check length of the numeric string, and remove quote characters.
*/
void
numeric_str()
{
	extern char *strcpy(),
		    *malloc();
	extern void getstr();
	unsigned char buf[LEN_INFO_MSGS];
	unsigned char *cp;

	if (gotstr) error(STATE);		/* only one string allowed */

	(void) getstr(buf);			/* process monetary string */
	if (strlen(buf) > LEN_INFO_MSGS-1)	/* check length */
		error(NMRC_LEN);
	if ((cp = (unsigned char *)malloc((unsigned) strlen((char *)buf)+1))
		== NULL)
		error(NOMEM);
	else {					/* store numeric string */
		nmrc_tab[num_value] = cp;
		(void) strcpy((char *)cp, (char *)buf);
	}

	gotstr = TRUE;				/* have one string */
}

/* numeric_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Make sure that there are no left-over metachars or item number.
*/
void
numeric_finish()
{
	if (META) error(EXPR);

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}
