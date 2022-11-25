/* @(#) $Revision: 72.1 $ */     
# include	"curses.ext"
# include	<signal.h>

/*----------------------------------------------------------------------*/
/*									*/
/* The following ifdef is a method of determining whether or not we are */
/* on a 9.0 system or 8.xx system.					*/
/* The specific problem is that if we are using a 8.07 os then SIGWINCH */
/* is defined in signal.h however, curses does not yet support this     */
/* feature; thus the header /usr/include/curses.h does not check and    */
/* create/not create the SIGWINCH entries into the window structure.    */
/* The method that we will use to determine if this is a 9.0 os, which  */
/* is a valid system for sigwinch is to check the SIGWINDOW flag.  If   */
/* This flag is set, then it's 9.0 otherwise, it is a os prior to 9.0   */
/*  									*/
/*  Assumptions:  							*/
/*     		OS		SIGWINCH	SIGWINDOW		*/
/*		-----------------------------------------		*/
/*		9.0 +		True		True			*/
/*		8.07		True		Not Set			*/
/*		Pre-8.07	Not Set		Not Set			*/
/*									*/
/* This should probably be removed after support for 8.xx is dropped	*/
/*									*/
/*	Note: SIGWINCH should be Defined for 9.0 and forward.  If it 	*/
/*	      is defined for Pre-9.0, then it should be undefined.	*/
/*									*/

#ifndef _SIGWINDOW
#  ifdef SIGWINCH
#    undef SIGWINCH
#  endif
#endif
/*									*/
/*		Done with special SIGWINCH definitions.			*/
/*----------------------------------------------------------------------*/

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 *	This routine sets up a window buffer and returns a pointer to it.
 */
WINDOW *
makenew(num_lines, num_cols, begy, begx)
int	num_lines, num_cols, begy, begx;
{

	register int	i;
	register WINDOW	*win;
	register int	by, bx, nlines, ncols;
	char *malloc(), *calloc();

	_showctrl();

	by = begy;
	bx = begx;
	nlines = num_lines;
	ncols = num_cols;

# ifdef SIGWINCH
	/* allow making window initially beyond the screen size */
	if (nlines <= 0 || ncols <= 0) 
# else
	if (nlines <= 0 || ncols <= 0 || by > LINES || bx > COLS)
# endif
		return NULL;

# ifdef	DEBUG
	if(outf) fprintf(outf, "MAKENEW(%d, %d, %d, %d)\n", nlines, ncols, by, bx);
# endif
	if ((win = (WINDOW *) calloc(1, sizeof (WINDOW))) == NULL)
		return NULL;
# ifdef DEBUG
	if(outf) fprintf(outf, "MAKENEW: nlines = %d\n", nlines);
# endif
	if ((win->_y = (chtype **) calloc(nlines, sizeof (chtype *))) == NULL) {
		cfree((char *)win);
		return (WINDOW *) NULL;
	}
# ifdef SIGWINCH
	win->_allocy = nlines;
# endif
	if ((win->_firstch = (short *) calloc(nlines, sizeof (short))) == NULL) {
		cfree((char *)win);
		cfree((char *)win->_y);
	}
	if ((win->_lastch = (short *) calloc(nlines, sizeof (short))) == NULL) {
		cfree((char *)win);
		cfree((char *)win->_y);
		cfree((char *)win->_firstch);
	}
# ifdef DEBUG
	if(outf) fprintf(outf, "MAKENEW: ncols = %d\n", ncols);
# endif

	win->_cury = win->_curx = 0;
	win->_clear = (nlines == LINES && ncols == COLS);
	win->_maxy = nlines;
	win->_maxx = ncols;
	win->_begy = by;
	win->_begx = bx;
	win->_scroll = win->_leave = win->_use_idl = FALSE;
	win->_tmarg = 0;
	win->_bmarg = nlines - 1;
	for (i = 0; i < nlines; i++)
		win->_firstch[i] = win->_lastch[i] = _NOCHANGE;
	if (bx + ncols == COLS) {
		win->_flags |= _ENDLINE;
		/* Full window: scrolling heuristics (linefeed) work */
		if (nlines == LINES && ncols == COLS &&
		    by == 0 && bx == 0 && scroll_forward)
			win->_flags |= _FULLWIN;
		/* Scrolling window: it might scroll on us by accident */
		if (by + nlines == LINES && auto_right_margin)
			win->_flags |= _SCROLLWIN;
	}
# ifdef DEBUG
	if(outf) fprintf(outf, "MAKENEW: win->_clear = %d\n", win->_clear);
	if(outf) fprintf(outf, "MAKENEW: win->_leave = %d\n", win->_leave);
	if(outf) fprintf(outf, "MAKENEW: win->_scroll = %d\n", win->_scroll);
	if(outf) fprintf(outf, "MAKENEW: win->_flags = %0.2o\n", win->_flags);
	if(outf) fprintf(outf, "MAKENEW: win->_maxy = %d\n", win->_maxy);
	if(outf) fprintf(outf, "MAKENEW: win->_maxx = %d\n", win->_maxx);
	if(outf) fprintf(outf, "MAKENEW: win->_begy = %d\n", win->_begy);
	if(outf) fprintf(outf, "MAKENEW: win->_begx = %d\n", win->_begx);
# endif
	return win;
}
