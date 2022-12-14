/* @(#) $Revision: 27.1 $ */    

/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 */

# include	"curses.ext"

static WINDOW *lwin;

#ifdef DEBUG
_dumpwin(win)
register WINDOW *win;
{
	register int x, y;
	register chtype *nsp;

	if (!outf) {
		return;
	}
	if (win == stdscr)
		fprintf(outf, "_dumpwin(stdscr)--------------\n");
	else if (win == curscr)
		fprintf(outf, "_dumpwin(curscr)--------------\n");
	else
		fprintf(outf, "_dumpwin(%o)----------------\n", win);
	for (y=0; y<win->_maxy; y++) {
		if (y > 76)
			break;
		nsp = &win->_y[y][0];
		fprintf(outf, "%d: ", y);
		for (x=0; x<win->_maxx; x++) {
			_sputc(*nsp, outf);
			nsp++;
		}
		fprintf(outf, "\n");
	}
	fprintf(outf, "end of _dumpwin----------------------\n");
}
#endif
