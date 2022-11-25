/* @(#) $Revision: 38.1 $ */     
/*
 * Simulation of termcap using terminfo.
 */

#include "curses.ext"


short NSflag = FALSE; /* value of termcap "ns" capability */

int
tgetent(bp, name)
char *bp, *name;
{
	int rv;

	if (setupterm(name, 1, &rv) >= 0)
		/* Leave things as they were (for compatibility) */
		reset_shell_mode();

	/* This scheme offers better termcap database emulation.
	 * When converting termcap to terminfo, AT&T added to
	 * terminfo the "ind" scroll forward capability (with value
	 * '\n') for every entry in termcap with no "ns" capability.
	 * Thus a change in semantics was created when termcap
	 * was converted to terminfo. Our vi(1) (Berkeley) scrolls
	 * incorrectly because of this change.
	 *
	 * The following code undefines the termcap "sf" capability
	 * if it's a '\n' and sets the "ns" capability to false. Hence
	 * the semantics of the "old" termcap database are restored.
	 * 
	 * Note: If the user needs to bypass this scheme, he or
	 *	 she must prefix the "ind" capability in terminfo
	 *	 with a '!'. This sets "ns" to true and if wanted,
	 *	 the value of "sf" becomes whatever string is after
	 *	 the '!' (e.g. if sf=!foo, then the "ns" capability
	 *	 becomes false and "sf" becomes "foo").
	 * 
	 */

	if ( rv == 1 ) {        /* initialization complete */
		if ( scroll_forward == (char *)0 )
			NSflag = TRUE;
		else if ( scroll_forward[0] == '\n' ) {
			NSflag = FALSE;
			scroll_forward = (char *)0;
		} else if ( scroll_forward[0] == '!' ) {
			/* see note above */
			NSflag = TRUE;
			scroll_forward++;
		}
	}

	return rv;
}
