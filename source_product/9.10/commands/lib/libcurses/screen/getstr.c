/* @(#) $Revision: 64.1 $ */      
# include	"curses.ext"
# include	<nl_ctype.h>

extern	WINDOW	*_oldwin;
extern	short	_oldy;
extern	short	_oldx;
extern	chtype	_oldctrl;

/*
 *	This routine gets a string starting at (_cury,_curx)
 *
 */
wgetstr(win,str)
WINDOW	*win; 
char	*str;
{
	char myerase, mykill;
	char rownum[256], colnum[256];
	int doecho = SP->fl_echoit;
	int savecb = SP->fl_rawmode;
	register int cpos = 0;
	register int ch;
	register char *cp = str;

#ifdef DEBUG
	if (outf) fprintf(outf, "doecho %d, savecb %d\n", doecho, savecb);
#endif

	myerase = erasechar();
	mykill = killchar();
	noecho(); crmode();

	for (;;) {
		rownum[cpos] = win->_cury;
		colnum[cpos] = win->_curx;
		ch = wgetch(win);
		if (ch <= 0 ||ch == ERR || ch == '\n' || ch == '\r')
			break;
		if (ch == myerase || ch == KEY_LEFT || ch == KEY_BACKSPACE) {
			if (cpos > 0) {
				cp--; cpos--;
				if ( SECof2((chtype) (unsigned char) *cp) && FIRSTof2((chtype) (unsigned char) *(cp-1))){
					*(cp-1) = ' ';
				}
				if (doecho) {
					wmove(win, rownum[cpos], colnum[cpos]);
					wclrtoeol(win);
				}
			}
		} else if (ch == mykill) {
			cp = str;
			cpos = 0;
			if (doecho) {
				wmove(win, rownum[cpos], colnum[cpos]);
				wclrtoeol(win);
			}
		} else {
			*cp++ = ch;
			cpos++;
			if (doecho) {
				waddch(win, ch);
			}
		}
		if ( doecho & !(win->_flags&_ISPAD)){

			chtype	ch;
			short	x;
			short	y;

			x = _oldx;
			y = _oldy;
			ch = _oldctrl;

			_oldctrl = 0;

			wrefresh(win);

			_oldx = x;
			_oldy = y;
			_oldwin = win;
			_oldctrl = ch;
		}
	}

	*cp = '\0';

	if (doecho)
		echo();
	if (!savecb)
		nocrmode();
	waddch(win, '\n');
	if (win->_flags & _ISPAD);
		wrefresh(win);
	if (ch == ERR)
		return ERR;
	return OK;
}
