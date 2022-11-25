/* @(#) $Revision: 72.1 $ */    
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

#ifdef SIGTSTP

# include	"curses.ext"

extern	WINDOW	*lwin;

/*
 * handle stop and start signals
 */
_tstp() {

#ifdef  SIGWINCH
	int olines = lines;
	int ocolumns = columns;
	struct winsize wsz;
#endif
# ifdef	hpux
	struct sigvec vec;
	vec.sv_handler = SIG_DFL;
	vec.sv_mask = vec.sv_onstack = 0;
	/* reset to default so we can really get stopped */
	(void) sigvector(SIGTSTP, &vec, (struct sigvec *)0);
	/*
	 * allow receipt of SIGTSTP (via kill())
	 * while we're still in this routine
	 */
	(void) sigsetmask (sigblock(-1L) & ~ (1L << (SIGTSTP -1)));
# endif	hpux
#ifdef DEBUG
	if (outf) fflush(outf);
#endif
	_ll_move(lines-1, 0);
	endwin();
	fflush(stdout);
	kill(0, SIGTSTP);

#ifndef	hpux
	signal(SIGTSTP, _tstp);
#else	hpux
	vec.sv_handler = _tstp;
	(void) sigvector(SIGTSTP, &vec, (struct sigvec *)0);
#endif	hpux

#ifdef  SIGWINCH
	/* if window size has changed upon return from SIGTSTP, fix screen */
	if (ioctl(0, TIOCGWINSZ, &wsz) < 0) 
		return ERR;
	if (wsz.ws_row != olines || wsz.ws_col != ocolumns) {
		_winchTrap();
		return;
	}
#endif
	fixterm();
	SP->doclear = 1;
	lwin=curscr;
	doupdate();
	/*wrefresh(curscr);*/
}
#endif
