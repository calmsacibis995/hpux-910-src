/* @(#) $Revision: 64.1 $ */   
# include	"curses.ext"
# include	<nl_ctype.h>

extern	WINDOW	*_oldwin;
extern	short	_oldy;
extern	short	_oldx;

/*
 *	This routine performs an insert-char on the line, leaving
 * (_cury,_curx) unchanged.
 *
 */
winsch(win, c)
reg WINDOW	*win;
chtype		c; {

	register chtype	*temp1, *temp2;
	register chtype	*end;
	short minx;

	_showctrl();

	end = &win->_y[win->_cury][win->_curx];
	temp1 = &win->_y[win->_cury][win->_maxx - 1];
	temp2 = temp1 - 1;
	minx = win->_curx;

	if ( *temp2 & A_FIRSTOF2 ){
		*temp2 = *temp2 & A_ATTRIBUTES | ' ';
	}
	if ( *end & A_SECOF2 ){
		*(end-1) = *(end-1) & A_ATTRIBUTES | ' ';
		*end = *end & A_ATTRIBUTES | ' ';
		minx--;
	}
		
	while (temp1 > end)
		*temp1-- = *temp2--;

	if (win->_attrs)
	{
		c |= win->_attrs;
	}

	*temp1 = c;

	/* Set NLS Attributes */

	if ( win->_curx>0 && _oldwin==win && _oldx==win->_curx-1 && _oldy==win->_cury &&
	     !(win->_y[_oldy][_oldx] & A_NLSATTR) )
	{
		if ( FIRSTof2(win->_y[_oldy][_oldx] & A_CHARTEXT) &&
		     SECof2(c & A_CHARTEXT) )	
		{
			*temp1-- |= A_SECOF2;
			*temp1 |= A_FIRSTOF2;
		}
	}

	win->_lastch[win->_cury] = win->_maxx - 1;
	if (win->_firstch[win->_cury] == _NOCHANGE ||
	    win->_firstch[win->_cury] > minx)
		win->_firstch[win->_cury] = minx;
	if (win->_cury == win->_bmarg && win->_y[win->_bmarg][win->_maxx-1] != ' ')
		if (win->_scroll && !(win->_flags&_ISPAD)) {
			wrefresh(win);
			scroll(win);
			win->_cury--;
		}
		else
			return ERR;

	_oldwin = win;
	_oldy = win->_cury;
	_oldx = win->_curx;

	return OK;
}
