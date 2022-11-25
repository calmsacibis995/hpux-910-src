/* @(#) $Revision: 27.2 $ */    
/*
 * tgoto: function included only for upward compatibility with old termcap
 * library.  Assumes exactly two parameters in the wrong order.
 */
char *
tgoto(cap, col, row)
char *cap;
int col, row;
{
	char *cp;
	char *tparm();

	cp = tparm(cap, row, col);

	if (cp == 0) {
		/*
		 * ``We don't do that under BOZO's big top''
		 */
		return "OOPS";
	}

	return cp;
}
