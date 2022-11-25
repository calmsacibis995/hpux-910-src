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
 * allocate space for and set up defaults for a new _window
 *
 * 1/26/81 (Berkeley).  This used to be newwin.c
 */

WINDOW *
newwin(nlines, ncols, by, bx)
register int	nlines, ncols, by, bx;
{
	register WINDOW	*win;
	register chtype	*sp;
	register int i;
	char *calloc();

	/* with SIGWINCH, no need to trim windows that are bigger */
	/* than the screen */
#ifndef SIGWINCH
	if (by + nlines > LINES)
		nlines = LINES - by;
	if (bx + ncols > COLS)
		ncols = COLS - bx;
#endif

	if (nlines == 0)
		nlines = LINES - by;
	if (ncols == 0)
		ncols = COLS - bx;

	if ((win = makenew(nlines, ncols, by, bx)) == NULL)
		return NULL;
	for (i = 0; i < nlines; i++)
		if ((win->_y[i] = (chtype *) calloc(ncols, sizeof (chtype))) == NULL) {
			register int j;

			for (j = 0; j < i; j++)
				cfree((char *)win->_y[j]);
			cfree((char *)win->_firstch);
			cfree((char *)win->_lastch);
			cfree((char *)win->_y);
			cfree((char *)win);
			return NULL;
		}
		else
			for (sp = win->_y[i]; sp < win->_y[i] + ncols; )
				*sp++ = ' ';

#ifdef  SIGWINCH
	if (by >= lines || bx >= columns)    /* window is totally invisible */
		win->_maxx = win->_maxy = 0;
	else {
		if (by + nlines > lines) {
			win->_maxy = lines - by;
			win->_bmarg = win->_maxy - 1;
		}
		if (bx + ncols > columns) 
			win->_maxx = columns - bx;
	}

	win->_allocx = ncols;
	_ins_winlist(win);
#endif
	return win;
}
