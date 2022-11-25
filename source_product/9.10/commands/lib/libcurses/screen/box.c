/* @(#) $Revision: 70.1 $ */     
# include	"curses.ext"

/*
 *	This routine draws a box around the given window with "vert"
 * as the vertical delimiting char, and "hor", as the horizontal one.
 *
 */

/* Defaults - might someday be terminal dependent using graphics chars */
#define DEFVERT '|'
#define DEFHOR  '-'

box(win, vert, hor)
register WINDOW	*win;
chtype	vert, hor;
{
	register int	i;
	register int	endy, endx;
	register chtype	*fp, *lp;

	_showctrl();

	if (vert == 0)
		vert = DEFVERT;
	if (hor == 0)
		hor = DEFHOR;
	endx = win->_maxx -  1;
	endy = win->_maxy -  1;
	fp = win->_y[0];
	lp = win->_y[endy];
	for (i = 0; i <= endy; i++)
	{
		if ( win->_y[i][0] & A_NLSATTR )
		{
			if ( win->_y[i][0] & A_FIRSTOF2 )
			{
				win->_y[i][1] = win->_y[i][1] & A_ATTRIBUTES | ' ';
			}
			else
			{
				win->_y[i][-1] = win->_y[i][-1] & A_ATTRIBUTES | ' ';
			}
		}
		if ( win->_y[i][endx] & A_NLSATTR )
		{
			if ( win->_y[i][endx] & A_SECOF2 )
			{
				win->_y[i][endx-1] = win->_y[i][endx-1] & A_ATTRIBUTES | ' ';
			}
			else
			{
				win->_y[i][endx+1] = win->_y[i][endx+1] & A_ATTRIBUTES | ' ';
			}
		}
		win->_y[i][0] = win->_y[i][endx] = (vert | win->_attrs);

	}
	for (i = 1; i < endx; i++)
		fp[i] = lp[i] = (hor | win->_attrs);
	touchwin(win);
}
