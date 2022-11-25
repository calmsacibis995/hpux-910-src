/* @(#) $Revision: 64.1 $ */      
# include	"curses.ext"
# include	<signal.h>

extern	int	_poschanged;

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 *
 */

/*
 * Move to given location on stdscr.  Update curses notion of where we
 * are (stdscr). This could be a macro
 * and originally was, but if you do
 *	move(line++, 0)
 * it will increment line twice, which is a lose.
 */
m_move(row, col)
int row, col;
{
	stdscr->_cury=row;
	stdscr->_curx=col;

	_poschanged = 1;

	_ll_move(row,col);
}
