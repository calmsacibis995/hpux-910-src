/* @(#) $Revision: 70.2 $ */

/*
 *  _winchTrap.c: the default SIGWINCH handler 
 *
 *   Note: _fixScreen.c, _fixWindow.c, _winchTrap.c and _winlist.c
 *         are used only when SIGWINCH is active.
 */

#include "curses.ext"
#include <termios.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <signal.h> 

_winchTrap()
{
	struct winsize	wsz;
	char		lines_env_string[15], cols_env_string[15];
	struct _winlink *w;
	register short	ocury, ocurx, miny, minx;
	int		n;
#ifdef hpux
	struct sigvec	wvec, owvec;
	wvec.sv_handler = SIG_DFL;
	(void) sigvector(SIGWINCH, &wvec, &owvec);
#else
	(void) signal(SIGWINCH, SIG_DFL);
#endif

	ocury    = SP->phys_y;
	ocurx    = SP->phys_x;

	if (ioctl(0, TIOCGWINSZ, &wsz) < 0)
		return ERR;

	/* update the shell and global  parameters */
	sprintf(lines_env_string, "LINES=%d", wsz.ws_row);
	putenv(lines_env_string);
	sprintf(cols_env_string, "COLUMNS=%d", wsz.ws_col);
	putenv(cols_env_string);

	lines = LINES = wsz.ws_row;
	columns = COLS  = wsz.ws_col;

	_fixScreen();
	
	/* fix all windows in the screen */
	w = _winlist;
	while (w != NULL) {
		_fixWindow(w->win);
		w = w->next;
	}

	SP->doclear = 1;
	wnoutrefresh(_sumscr);

	miny = (ocury > lines-1) ? lines-1 : ocury;
	minx = (ocurx > columns-1) ? columns-1 : ocurx;
	wmove(_sumscr, miny, minx);

	doupdate(); 
	fixterm();

#ifdef hpux
	(void) sigvector(SIGWINCH, &owvec, (struct sigvec *)0);
#else 
	(void) signal(SIGWINCH, _winchTrap);
#endif
}
