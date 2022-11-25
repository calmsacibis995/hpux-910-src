/* @(#) $Revision: 64.6 $ */     

/*
    8/26/87:

	1)  Rewritten to use the revised version of _printmsg(),
	    using a local argument list.

	2)  Prior version limited caller's total output string
	    to BUFSIZ.  Now, only the caller's format string
	    has that limitation.  This limit was apparently
	    never documented.
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define strlen _strlen
#define printmsg __printmsg
#endif

#include <stdio.h>
#include <varargs.h>
#include <limits.h>
#include <errno.h>

extern int errno;

int	_printmsg();

/*VARARGS*/
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef printmsg
#pragma _HP_SECONDARY_DEF __printmsg printmsg
#define printmsg __printmsg
#endif

int
printmsg(fmt, va_alist)
const char *fmt;
va_dcl
{
	int	cnt;
 	char	newfmt[BUFSIZ];
	va_list newarg[NL_ARGMAX*4];
	va_list	args, newargs;

	if (strlen(fmt) >= BUFSIZ) {
		errno = EINVAL;
		return(EOF);
	}
	va_start(args);
	newargs = (va_list) &newarg[NL_ARGMAX*2]; 
	if (!_printmsg(fmt, newfmt, args, newargs)) {
		errno = EINVAL;
		cnt = EOF;
	}
	else {
	    if ( _wrtchk(stdout) )
		return EOF;
	    cnt = _doprnt(newfmt, newargs, stdout);
	}
	va_end(ap);
	return(cnt);
}
