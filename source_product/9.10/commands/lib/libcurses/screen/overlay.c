/* @(#) $Revision: 70.1 $ */      
# include	"curses.ext"
# include	<ctype.h>
# include	<nl_ctype.h>

# define	min(a,b)	(a < b ? a : b)
# define	max(a,b)	(a > b ? a : b)

/*
 * find the maximum line and column of a window in screen coords
 */
#define		MAX_LINE(win)	(win->_maxy + win->_begy)
#define		MAX_COL(win)	(win->_maxx + win->_begx)

/*
 *	This routine writes win1 on win2 non-destructively.
 *
 */
overlay(win1, win2)
reg WINDOW	*win1, *win2; {

	reg chtype	*sp, *end;
	reg int         starty, startx, y_top, x_left,
			x2, endline, y2, ncols,  x2start;

# ifdef DEBUG
	if(outf) fprintf(outf, "OVERLAY(%0.2o, %0.2o);\n", win1, win2);
# endif
	/*
	 * "y_top" and "x_left" are starting line and column
	 * of overlapping section:
	 */
	y_top = max(win1->_begy, win2->_begy);
	x_left = max(win1->_begx, win2->_begx);

	/* "endline" is position of last line of overlapping section */
	endline = min( MAX_LINE(win1),  MAX_LINE(win2) ) - win1->_begy;
	/* "ncols" is number of columns in overlapping section */
	ncols   = min( MAX_COL(win1),  MAX_COL(win2) ) - x_left;

	/*
	 * "starty" and "startx" are starting
	 * row and line relative to window 1:
	 */
	starty = y_top - win1->_begy;
	startx = x_left - win1->_begx;

	/*
	 * "y2" and "x2start" are starting
	 * row and line releative to window 2:
	 */
	y2 = y_top - win2->_begy;
	x2start = x_left - win2->_begx;

	/* for every line in overlapping section do: */
	while ( starty < endline ) {
		sp = &win1->_y[starty++][startx]; /* get next character */
		end = sp + ncols; /* address of last character in line */
		x2 = x2start;

		/* for every character in current line do: */
		if (!_CS_SBYTE) {
			for ( ; sp < end; sp++ ) {
				if (!isspace(*sp & A_CHARTEXT)){
					if (((x2==x2start) && (*sp & A_SECOF2)) || ((sp==end-1) && (*sp & A_FIRSTOF2))){
						mvwaddch(win2, y2, x2, *sp & A_ATTRIBUTES | ' ');
					} else {
						mvwaddch(win2, y2, x2, *sp & ~A_NLSATTR);
					}
				}
				x2++; /* next x position in window 2 */
			}
		} else {
			for ( ; sp < end; sp++ ) {
				if (!isspace(*sp & A_CHARTEXT))
					mvwaddch(win2, y2, x2, *sp & ~A_NLSATTR);
				x2++; /* next x position in window 2 */
			}
		}
		y2++; /* next y position in window 2 */
	}
}
