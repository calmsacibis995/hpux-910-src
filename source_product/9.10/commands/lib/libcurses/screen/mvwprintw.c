/* @(#) $Revision: 37.1 $ */   
# include	"curses.ext"
# include	<varargs.h>

/* VARARGS */
mvwprintw(win, y, x, fmt, va_alist)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return wmove(win, y, x) == OK ? _sprintw(win, fmt, ap) : ERR;
}
