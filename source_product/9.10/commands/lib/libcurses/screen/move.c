/* @(#) $Revision: 64.1 $ */    
# include	"curses.ext"

extern	int	_poschanged;

/*
 *	This routine moves the cursor to the given point
 *
 */
wmove(win, y, x)
reg WINDOW	*win;
reg int		y, x;
{

# ifdef DEBUG
	if(outf) fprintf(outf, "MOVE to win ");
	if( win == stdscr )
	{
		if(outf) fprintf(outf, "stdscr ");
	}
	else
	{
		if(outf) fprintf(outf, "%o ", win);
	}
	if(outf) fprintf(outf, "(%d, %d)\n", y, x);
# endif
	if( x >= win->_maxx || y >= win->_maxy )
	{
		return ERR;
	}
	win->_curx = x;
	win->_cury = y;

	_poschanged = 1;

	return OK;
}
