/* @(#) $Revision: 64.1 $ */   
# include	"curses.ext"

/*
 *	This routine erases everything on the _window.
 *
 */
werase(win)
reg WINDOW	*win; {

	reg int		y;
	reg chtype	*sp, *end, *start, *maxx;
	reg int		minx;

# ifdef DEBUG
	if(outf) fprintf(outf, "WERASE(%0.2o), _maxx %d\n", win, win->_maxx);
# endif

	_showctrl();

	for (y = 0; y < win->_maxy; y++) {
		minx = _NOCHANGE;
		maxx = NULL;
		start = win->_y[y];
		end = &start[win->_maxx-1];
		if ( *start & A_SECOF2 ) {
			*(start-1) = *(start-1) & A_ATTRIBUTES | ' ';
		}
		if ( *end & A_FIRSTOF2 ) {
			*(end+1) = *(end+1) & A_ATTRIBUTES | ' ';
		}
		for (sp = start; sp <= end; sp++) {
#ifdef DEBUG
			if (y == 23) if(outf) fprintf(outf,
				"sp %x, *sp %c %o\n", sp, *sp, *sp);
#endif
			if (*sp != ' ') {
				maxx = sp;
				if (minx == _NOCHANGE)
					minx = sp - start;
				*sp = ' ';
			}
		}
		if (minx != _NOCHANGE) {
			if (win->_firstch[y] > minx
			     || win->_firstch[y] == _NOCHANGE)
				win->_firstch[y] = minx;
			if (win->_lastch[y] < maxx - win->_y[y])
				win->_lastch[y] = maxx - win->_y[y];
		} else {
			if (win->_firstch[y] == _NOCHANGE) {
				win->_firstch[y] = 0;
				win->_lastch[y] = win->_maxx - 1;
			}
		}
# ifdef DEBUG
	if(outf) fprintf(outf, "WERASE: minx %d maxx %d _firstch[%d] %d, start %x, end %x\n",
		minx, maxx ? maxx-start : NULL, y, win->_firstch[y], start, end);
# endif
	}
	win->_curx = win->_cury = 0;
}
