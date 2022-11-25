/* @(#) $Revision: 27.1 $ */    
#include "curses.ext"

/*
 * TRUE => OK to scroll screen up when you run off the bottom.
 */
scrollok(win,bf)
WINDOW *win;
int bf;
{
	/* Should consider using scroll/page mode of some terminals. */
	win->_scroll = bf;
}
