/* @(#) $Revision: 27.1 $ */   

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine implements a scanf on the standard screen.
 */
/* VARARGS */
scanw(fmt, va_alist)
char	*fmt;
va_dcl
{
	va_list	ap;

	va_start(ap);
	return __sscans(stdscr, fmt, ap);
}
