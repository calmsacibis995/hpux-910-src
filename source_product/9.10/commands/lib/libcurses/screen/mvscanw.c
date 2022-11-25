/* @(#) $Revision: 37.1 $ */     
# include	"curses.ext"
# include	<varargs.h>

/*
 * implement the mvscanw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Another sigh....
 *
 */

/* VARARGS */
mvscanw(y, x, fmt, va_alist)
reg int		y, x;
char		*fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return move(y, x) == OK ? __sscans(stdscr, fmt, ap) : ERR;
}
