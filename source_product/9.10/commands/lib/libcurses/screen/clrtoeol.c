/* @(#) $Revision: 64.1 $ */    
# include	"curses.ext"

/*
 *	This routine clears up to the end of line
 *
 */
wclrtoeol(win)
reg WINDOW	*win; {

	reg chtype	*sp, *end;
	reg int		y, x;
	reg chtype	*maxx;
	reg int		minx;

	_showctrl();

	y = win->_cury;
	x = win->_curx;

	end = &win->_y[y][win->_maxx-1];
	minx = _NOCHANGE;
	maxx = &win->_y[y][x];
	sp = maxx;
	if ( (sp <= end) && (*sp & A_SECOF2) ) {
		*(sp-1) = *(sp-1) & A_ATTRIBUTES | ' ';
		minx = sp - 1 - win->_y[y];
	}
	if ( (sp <= end) && (*end & A_FIRSTOF2) ) {
		*(end+1) = *(end+1) & A_ATTRIBUTES | ' ';
	}
	for (; sp <= end ; sp++)
		if (*sp != ' ') {
			maxx = sp;
			if (minx == _NOCHANGE)
				minx = sp - win->_y[y];
			*sp = ' ';
		}
	/*
	 * update firstch and lastch for the line
	 */
# ifdef DEBUG
	if(outf) fprintf(outf, "CLRTOEOL: line %d minx = %d, maxx = %d, firstch = %d, lastch = %d, next firstch %d\n", y, minx, maxx - win->_y[y], win->_firstch[y], win->_lastch[y], win->_firstch[y+1]);
# endif
	if (minx != _NOCHANGE) {
		if (win->_firstch[y] > minx || win->_firstch[y] == _NOCHANGE)
			win->_firstch[y] = minx;
		if (win->_lastch[y] < maxx - win->_y[y])
			win->_lastch[y] = maxx - win->_y[y];
	}
}
