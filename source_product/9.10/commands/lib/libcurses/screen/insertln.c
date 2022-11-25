/* @(#) $Revision: 72.1 $ */    
# include	"curses.ext"

/*
 *	This routine performs an insert-line on the _window, leaving
 * (_cury,_curx) unchanged.
 *
 */
winsertln(win)
reg WINDOW	*win; {

	reg chtype	*temp;
	reg int		y;
	reg chtype	*end;
	reg chtype	*sp;

	_showctrl();
	if ( win->_flags & _SUBWIN || win->_flags & _HASSUBWIN )
	{
		for (y = win->_maxy - 1; y > win->_cury; --y) {
			sp = win->_y[y];
			end = sp + win->_maxx;
			temp = win->_y[y-1];
			while ( sp < end )
				*sp++ = *temp++;
			win->_firstch[y] = 0;
			win->_lastch[y] = win->_maxx - 1;
		}
		sp = win->_y[win->_cury];
		end = sp + win->_maxx;
		while ( sp < end )
			*sp++ = ' ';
		win->_firstch[win->_cury] = 0;
		win->_lastch[win->_cury] = win->_maxx - 1;
	}
	else
	{
		temp = win->_y[win->_maxy-1];
		win->_firstch[win->_cury] = 0;
		win->_lastch[win->_cury] = win->_maxx - 1;
		for (y = win->_maxy - 1; y > win->_cury; --y) {
			win->_y[y] = win->_y[y-1];
			win->_firstch[y] = 0;
			win->_lastch[y] = win->_maxx - 1;
		}
		win->_y[win->_cury] = temp;
		for (end = &temp[win->_maxx]; temp < end; )
			*temp++ = ' ';
	}
/*	if ( win->_cury == win->_bmarg && win->_y[win->_bmarg][win->_maxx-1] != ' ')
**		if (win->_scroll && !(win->_flags&_ISPAD)) {
**			wrefresh(win);
**			scroll(win);
**			win->_cury--;
**		}
**		else
**			return ERR;
*/
	return OK;
}
