/* @(#) $Revision: 70.3 $ */

/*
 * _fixWindow.c: Upon SIGWINCH, readjust an "overlay" window. (see 
 *		 _fixScreen.c for handling "ground" windows.)
 *
 * Note: _fixScreen.c, _fixWindow.c, _winchTrap.c and _winlist.c
 *       are used only when SIGWINCH is active. 
 */

#include "curses.ext"
#include <signal.h>

_fixWindow(win)
WINDOW *win;
{
	register short  miny, minx;
	register chtype **ty;
	register short  *tfirstch, *tlastch;
	register chtype *tln;
	register int 	i, j;
	register chtype *sp;

	register short  bx, by;

	if (win == stdscr || win == curscr || win == _sumscr) 
		return OK;   /* these screen resize handled in _fixScreen.c */

	bx = win->_begx;
	by = win->_begy;

	/* window completely trimmed */
	if (by >= lines || bx >= columns) {
		win->_maxx = win->_maxy = 0;
		return OK;
	}

	if (by + win->_allocy > lines) {
		win->_maxy = lines - by;
		win->_bmarg = win->_maxy - 1;
	} else if (win->_allocy > win->_maxy) {
		win->_maxy = win->_allocy;
		win->_bmarg = win->_maxy - 1;
	}

	if (bx + win->_allocx > columns) 
		win->_maxx = columns - bx;
	else if (win->_allocx > win->_maxx) 
		win->_maxx = win->_allocx;
		
        if (bx + win->_maxx == columns) {
                win->_flags |= _ENDLINE;
                if (win->_maxy == lines && win->_maxx == columns &&
                    by == 0 && bx == 0 && scroll_forward)
                        win->_flags |= _FULLWIN;
		else
			win->_flags &= (~_FULLWIN);
                if (by + win->_maxy == lines && auto_right_margin)
                        win->_flags |= _SCROLLWIN;
		else
			win->_flags &= (~_SCROLLWIN);
        } else
		win->_flags &= (~_ENDLINE);

	wnoutrefresh(win);
}
