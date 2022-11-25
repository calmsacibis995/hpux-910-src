/* @(#) $Revision: 37.2 $ */      
# include	"curses.ext"
# include	<signal.h>

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 */

#ifdef SIGTSTP

/*
 * handle stop and start signals
 */
m_tstp() {

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
# ifdef DEBUG
	if (outf) fflush(outf);
# endif
	_ll_move(lines-1, 0);
	endwin();
	fflush(stdout);
	kill(0, SIGTSTP);

# ifndef hpux
	signal(SIGTSTP, m_tstp);
# else	hpux
	vec.sv_handler = m_tstp;
	(void) sigvector(SIGTSTP, &vec, (struct sigvec *)0);
# endif	hpux
	reset_prog_mode();
	SP->doclear = 1;
	_ll_refresh(0);
}
#endif
