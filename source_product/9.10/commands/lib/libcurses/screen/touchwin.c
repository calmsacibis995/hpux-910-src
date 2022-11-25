/* @(#) $Revision: 27.1 $ */    
# include	"curses.ext"

/*
 * make it look like the whole window has been changed.
 *
 */
touchwin(win)
reg WINDOW	*win;
{
	reg int		y, maxy, maxx;

#ifdef DEBUG
	if (outf) fprintf(outf, "touchwin(%x)\n", win);
#endif
	maxy = win->_maxy;
	maxx = win->_maxx - 1;
	for (y = 0; y < maxy; y++) {
		win->_firstch[y] = 0;
		win->_lastch[y] = maxx;
	}
}
