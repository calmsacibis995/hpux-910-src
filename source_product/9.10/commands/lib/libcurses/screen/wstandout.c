/* @(#) $Revision: 27.1 $ */   
/*
 */

# include	"curses.ext"

/*
 * enter standout mode
 */
wstandout(win)
register WINDOW	*win;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "WSTANDOUT(%x)\n", win);
#endif

	win->_attrs |= A_STANDOUT;
	return 1;
}
