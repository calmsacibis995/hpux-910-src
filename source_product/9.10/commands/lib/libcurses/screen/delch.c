/* @(#) $Revision: 64.1 $ */   
# include	"curses.ext"

/*
 *	This routine performs an delete-char on the line, leaving
 * (_cury,_curx) unchanged.
 *
 */
wdelch(win)
reg WINDOW	*win; {

	reg chtype	*temp1, *temp2;
	reg chtype	*end;
	reg short	minx;

	_showctrl();

	minx = win->_curx;
	end = &win->_y[win->_cury][win->_maxx - 1];
	temp2 = &win->_y[win->_cury][minx + 1];
	temp1 = temp2 - 1;

	if ( *temp1 & A_NLSATTR ){
		if ( *temp1 & A_SECOF2 ){
			*(temp1-1) = *(temp1-1) & A_ATTRIBUTES | ' ';
			minx--;
		} else{
			*(temp1+1) = *(temp1+1) & A_ATTRIBUTES | ' ';
		}
	}
			
	while (temp1 < end)
		*temp1++ = *temp2++;
	*temp1 = ' ';
	win->_lastch[win->_cury] = win->_maxx - 1;
	if (win->_firstch[win->_cury] == _NOCHANGE ||
	    win->_firstch[win->_cury] > minx)
		win->_firstch[win->_cury] = minx;
	return OK;
}
