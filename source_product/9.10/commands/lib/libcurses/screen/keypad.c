/* @(#) $Revision: 70.1 $ */      
#include "curses.ext"

/*
 * TRUE => special keys should be passed as a single character by getch.
 */
keypad(win,bf)
WINDOW *win; int bf;
{
#ifdef DEBUG
if (outf) fprintf(outf, "keypad: win->_use_keypad %d, SP->kp_state %d, boolean \
flag = %d\n", bf, win->_use_keypad, SP->kp_state);
#endif
	win->_use_keypad = bf;
#ifdef KEYPAD
	/* put keypad in proper state immediately (don't wait for getch call) */
	if (win->_use_keypad != SP->kp_state) {
		_kpmode(win->_use_keypad);
		fflush(stdout);
	}
#endif
}
