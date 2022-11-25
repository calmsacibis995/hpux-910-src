/* @(#) $Revision: 72.1 $ */      
/*
 * Define global variables
 *
 */
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

char	*Def_term	= "unknown";	/* default terminal type	*/
WINDOW *stdscr, *curscr;
int	LINES, COLS;
struct screen *SP;

char *curses_version = "Packaged for USG UNIX 6.0, 3/6/83";

# ifdef DEBUG
FILE	*outf;			/* debug output file			*/
# endif

struct	term _first_term;
struct	term *cur_term = &_first_term;

WINDOW *lwin;

int _endwin = FALSE;

int	tputs();

int	_poschanged = 0;
chtype	_oldctrl = 0;
short	_oldx = -1;
short	_oldy = -1;
WINDOW	*_oldwin = 0;

char    **attrstr = NULL;		/* set-one-attribute strings */

#ifdef 	SIGWINCH
struct _winlink *_winlist = NULL;
WINDOW  *_sumscr;
#endif
