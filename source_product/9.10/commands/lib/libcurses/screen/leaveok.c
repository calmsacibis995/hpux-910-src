/* @(#) $Revision: 27.1 $ */     
#include "curses.ext"

/*
 * TRUE => OK to leave cursor where it happens to fall after refresh.
 */
leaveok(win,bf)
WINDOW *win; int bf;
{
	win->_leave = bf;
}
