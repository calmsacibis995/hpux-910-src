/* @(#) $Revision: 27.1 $ */   
# include	"curses.ext"

/*
 *	This routine clears the _window.
 *
 */
wclear(win)
reg WINDOW	*win; {

	if (win == curscr)
		win = stdscr;
	werase(win);
	win->_clear = TRUE;
	return OK;
}
