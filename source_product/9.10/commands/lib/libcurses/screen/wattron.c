/* @(#) $Revision: 27.1 $ */     
/*
 */

# include	"curses.ext"

/*
 * Turn on selected attributes.
 */
wattron(win, attrs)
register WINDOW	*win;
int attrs;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "WATTRON(%x, %o)\n", win, attrs);
#endif

	win->_attrs |= attrs;
	return 1;
}
