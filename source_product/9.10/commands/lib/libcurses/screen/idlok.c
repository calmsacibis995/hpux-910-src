/* @(#) $Revision: 27.1 $ */   
#include "curses.ext"

/*
 * TRUE => OK to use insert/delete line.
 */
idlok(win,bf)
WINDOW *win;
int bf;
{
	win->_use_idl = bf;
}
