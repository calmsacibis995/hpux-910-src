/* @(#) $Revision: 27.1 $ */     
# include	"curses.ext"
# include	<signal.h>

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 *
 */

/*
 * Erase the contents of the desired screen - set to all _blanks.
 */
m_erase()
{
	register int n;

#ifdef DEBUG
	if (outf) fprintf(outf, "M_ERASE, erasing %d lines\n", lines);
#endif
	for (n = 0; n < lines; n++) {
		_clearline(n);
	}
}
