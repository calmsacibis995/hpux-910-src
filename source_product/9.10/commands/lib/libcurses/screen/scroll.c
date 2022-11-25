/* @(#) $Revision: 62.1 $ */      
# include	"curses.ext"

/*
 *	This routine scrolls the window up a line.
 *
 */
scroll(win)
WINDOW *win;
{
	_tscroll(win);
}
