/* @(#) $Revision: 27.1 $ */   
#include "curses.ext"

/*
 * TRUE => flush input when an interrupt key is pressed
 */
intrflush(win,bf)
WINDOW *win; int bf;
{
#ifdef USG
	if (bf)
		(cur_term->Nttyb).c_lflag &= ~NOFLSH;
	else
		(cur_term->Nttyb).c_lflag |= NOFLSH;
#else
	/* can't do this in 4.1BSD or V7 */
#endif
	reset_prog_mode();
}
