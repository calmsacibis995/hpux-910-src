/* @(#) $Revision: 72.1 $ */      
# include	"curses.ext"
# include 	<signal.h>

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

extern  WINDOW *lwin;

/*
 *	This routine deletes a _window and releases it back to the system.
 *
 */
delwin(win)
reg WINDOW	*win; {

	reg int	i;

	if (!(win->_flags & _SUBWIN))
#ifdef  SIGWINCH
		/* _allocy rows were actually allocated */
		for (i = 0; i < win->_allocy && win->_y[i]; i++)
#else
		for (i = 0; i < win->_maxy && win->_y[i]; i++)
#endif
			cfree((char *) win->_y[i]);
	cfree((char *)win->_firstch);
	cfree((char *)win->_lastch);
	cfree((char *) win->_y);
	cfree((char *) win);
	/* make sure we don't doupdate later with a deleted window */
	if (lwin == win) 		
		lwin=NULL; 		
#ifdef	SIGWINCH
	_del_winlist(win);
#endif
}
