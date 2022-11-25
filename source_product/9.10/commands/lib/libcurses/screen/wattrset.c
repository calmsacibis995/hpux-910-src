/* @(#) $Revision: 27.1 $ */    
/*
 */

# include	"curses.ext"

/*
 * Set selected attributes.
 */
wattrset(win, attrs)
register WINDOW	*win;
int attrs;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "WATTRON(%x, %o)\n", win, attrs);
#endif

	win->_attrs = attrs;
	return 1;
}
