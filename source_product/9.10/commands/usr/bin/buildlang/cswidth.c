/* @(#) $Revision: 66.2 $ */     
/* LINTLIBRARY */

#ifdef EUC
#include	<stdio.h>
#include	<limits.h>
#include	"global.h"

int	gotstr;

/* cswidth_init: initialize io_charsize table.
** Get here when you see a 'cswidth' keyword.
*/
void
cswidth_init(token)
int	token;				/* keyword token */
{
	extern void cswidth_str();
	extern void cswidth_finish();

	if (op) {
		/* if we're not the first, finish up last keyword */
		if (finish == NULL) error(STATE);
		(*finish)();
	}

	op = token;			/* save off the token */
	string = cswidth_str;		/* set string function for info.c */
	number = NULL;			/* no numbers allowed */
	finish = cswidth_finish;	/* set up info finish */

	gotstr = FALSE;			/* no cswidth string initially */
}

/* cswidth_str: process cswidth string.
** Remove quote characters, check length and format (n:n,n:n,n:n) of the
** cswidth string, store integer values in io_charsize[] array.
*/

void
cswidth_str()
{
	extern void getstr();
	unsigned char buf[12], *pbuf;
	int cset = 1;

	if (gotstr) error(STATE);		/* only one string allowed */

	(void) getstr(buf);			/* process cswidth string */
	if (strlen(buf) > 12-1)			/* check length */
		error(CSWIDTH_LEN);

	pbuf = buf;

	while (*pbuf && cset < MAX_CODESET) {
		if (!isdigit(pbuf[0]) || (pbuf[1] != ':') || !isdigit(pbuf[2]))
			error(CSWIDTH_FMT);
		else {
			io_charsize[cset] = atoi(&pbuf[0]);
			io_charsize[cset+MAX_CODESET] = atoi(&pbuf[2]);
			if (cset == 2 || cset == 3)
				io_charsize[cset] += 1;
		}
		if (pbuf[3] == '\0')
			break;
		else if (pbuf[3] == ',')
			pbuf += 4, cset++;
		else
			error(CSWIDTH_FMT);
	}

	gotstr = TRUE;			/* have one string */
}

/* cswidth_finish: finish up any left-overs from last keyword.
** Called when next keyword or end of file appears.
** Make sure that there are no left-over metachars.
*/
void
cswidth_finish()
{
	if (META) error(EXPR);

	if (!gotstr) {
		lineno--;
		error(STATE);
	}
}
#else /* EUC */
extern int errno;	/* suppress compiler warning message*/
#endif /* EUC */
