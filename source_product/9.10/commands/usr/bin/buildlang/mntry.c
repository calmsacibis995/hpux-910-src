/* @(#) $Revision: 64.4 $ */     
/* LINTLIBRARY */

#include	<stdio.h>
#include	<limits.h>
#include	"global.h"

int	gotstr;

/* mntry_init: initialize monetary table.
** Get here when you see a lconv-item keyword.
*/
void
mntry_init(token)
int	token;				/* keyword token */
{
	extern void mntry_str();
	extern void mntry_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = mntry_str;		/* set string function for mntry.c */
	finish = mntry_finish;		/* set up mntry finish */

	gotstr = FALSE;			/* no monetary string yet */
}

/* mntry_str: process one monetary string.
** Check length of the monetary string, and remove quote characters.
*/
void
mntry_str()
{
	extern char *strcpy(),
		    *malloc();
	extern void getstr();
	unsigned char buf[LEN_INFO_MSGS];
	unsigned char *cp;

	if (gotstr) error(STATE);		/* only one string allowed */

	(void) getstr(buf);			/* process monetary string */
	if (strlen(buf) > LEN_INFO_MSGS-1)	/* check length */
		error(MNTRY_LEN);
	if ((cp = (unsigned char *)malloc((unsigned) strlen((char *)buf)+1))
		== NULL)
		error(NOMEM);
	else {					/* store monetary string */
		mntry_tab[num_value] = cp;
		if (num_value <= N_SIGN && buf[0] == '\0') {
		/* num_value <= N_SIGN are really chars, got a null char */
			buf[0] = CHAR_MAX;
			buf[1] = '\0';
		}
		(void) strcpy((char *)cp, (char *)buf);
	}

	gotstr = TRUE;				/* have one string */
}

/* mntry_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Make sure that there are no left-over metachars or item number.
*/
void
mntry_finish()
{
	if (META) error(EXPR);

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}
