/* @(#) $Revision: 64.1 $ */    
# include	"curses.ext"

/*
 *	This routine erases everything on the window.
 *
 */
wclrtobot(win)
reg WINDOW	*win; {

	reg int		y;
	reg chtype	*sp, *end, *maxx;
	reg int		startx, minx;

	_showctrl();

	startx = win->_curx;
	minx = _NOCHANGE;
	
	y = win->_cury;

	if ( y < win->_maxy && startx < win->_maxx ){
 		sp = &win->_y[y][startx];
 		if ( *sp & A_SECOF2 ) {
			*(sp-1) = *(sp-1) & A_ATTRIBUTES | ' ';
			minx = sp - 1 - win->_y[y];
 		}
	}

	for (; y < win->_maxy; y++) {
		end = &win->_y[y][win->_maxx-1];
		if ( *end & A_FIRSTOF2 ) {
			*(end+1) = *(end+1) & A_ATTRIBUTES | ' ';
		}
		for (sp = &win->_y[y][startx]; sp <= end; sp++)
			if (*sp != ' ') {
				maxx = sp;
				if (minx == _NOCHANGE)
					minx = sp - win->_y[y];
				*sp = ' ';
			}
		if (minx != _NOCHANGE) {
			if (win->_firstch[y] > minx
			     || win->_firstch[y] == _NOCHANGE)
				win->_firstch[y] = minx;
			if (win->_lastch[y] < maxx - win->_y[y])
				win->_lastch[y] = maxx - win->_y[y];
		}
		minx = _NOCHANGE;
		startx = 0;
	}
	/* win->_curx = win->_cury = 0; */
}
