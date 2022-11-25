/* @(#) $Revision: 27.1 $ */    
/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 */

# include	"curses.ext"

/* Put out window and update screen */
wrefresh(win)
WINDOW	*win;
{
	wnoutrefresh(win);
	return doupdate();
}
