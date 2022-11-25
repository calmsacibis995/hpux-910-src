/* @(#) $Revision: 27.1 $ */      

# include	"curses.ext"
# include	<varargs.h>

/*
 *	This routine implements a scanf on the given window.
 */
/* VARARGS */
wscanw(win, fmt, va_alist)
WINDOW	*win;
char	*fmt;
va_dcl
{
	va_list	ap;

	va_start(ap);
	return __sscans(win, fmt, ap);
}
