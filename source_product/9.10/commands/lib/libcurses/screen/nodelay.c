/* @(#) $Revision: 27.1 $ */     
#include "curses.ext"

/*
 * TRUE => don't wait for input, but return -1 instead.
 */
nodelay(win,bf)
WINDOW *win; int bf;
{
	_fixdelay(win->_nodelay, bf);
	win->_nodelay = bf;
}
