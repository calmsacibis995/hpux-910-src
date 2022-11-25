/* @(#) $Revision: 37.1 $ */    
# include	"curses.ext"
# include	<varargs.h>

/*
 * implement the mvprintw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Sigh....
 *
 */

/* VARARGS */
mvprintw(y, x, fmt, va_alist)
reg int		y, x;
char		*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return move(y, x) == OK ? _sprintw(stdscr, fmt, ap) : ERR;
}
