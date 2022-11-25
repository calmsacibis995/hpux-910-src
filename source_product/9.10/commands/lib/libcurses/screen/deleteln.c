/* @(#) $Revision: 72.1 $ */    

#include	"curses.h"

/*
 *	This routine deletes a line from the screen.  It leaves
 *      (_cury,_curx) unchanged.
 *      if win is a subwindow or has a subwindow
 *         it row up the content by coping data (chtype)
 *      else
 *         simply row up by moving pointers
 *
 */
wdeleteln(win)
reg WINDOW	*win; 
{
	reg chtype	*temp;
	reg int		y, lastrow;
	reg chtype	*end;
	reg chtype	*sp;

	_showctrl();

	lastrow=win->_maxy-1;

	if ( win->_flags & (_SUBWIN | _HASSUBWIN) )
	{
		for (y = win->_cury; y < lastrow; y++) {
			sp = win->_y[y];
			end = sp + win->_maxx;
			temp = win->_y[y+1];
			while ( sp < end )
				*sp++ = *temp++;
			win->_firstch[y] = 0;
			win->_lastch[y] = win->_maxx - 1;
		}
		sp = win->_y[y];
		end = sp + win->_maxx;
		while ( sp < end )
			*sp++ = ' ';
		win->_firstch[y] = 0;
		win->_lastch[y] = win->_maxx - 1;
	}
	else
	{
		temp = win->_y[win->_cury];
		for (y = win->_cury; y < lastrow; y++) {
			win->_y[y] = win->_y[y+1];
			win->_firstch[y] = 0;
			win->_lastch[y] = win->_maxx - 1;
		}
		win->_y[y] = temp ;
		for (end = &temp[win->_maxx]; temp < end; )
			*temp++ = ' ';
		win->_firstch[y] = 0;
		win->_lastch[y] = win->_maxx - 1;
	}
}
