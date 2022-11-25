/* @(#) $Revision: 27.1 $ */    
/*
 * make the current screen look like "win" over the area covered by
 * win.
 *
 */

#include	"curses.ext"

/* Like wrefresh but refreshing from a pad. */
prefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol)
WINDOW	*pad;
int pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol;
{
	pnoutrefresh(pad, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);
	return doupdate();
}
