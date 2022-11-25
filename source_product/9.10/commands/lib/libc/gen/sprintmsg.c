/* @(#) $Revision: 66.1 $ */     

/*
    8/26/87:

	1)  Rewritten to use the revised version of _printmsg(),
	    using a local argument list.

	2)  The caller's format string is now limited to BUFSIZ
	    bytes.
*/

#ifdef _NAMESPACE_CLEAN
#define sprintmsg _sprintmsg
#define strlen _strlen
#endif

#include <stdio.h>
#include <varargs.h>
#include <values.h>
#include <limits.h>
#include <errno.h>

extern int errno;

int	_printmsg();

/*VARARGS*/

#ifdef _NAMESPACE_CLEAN
#undef sprintmsg
#pragma _HP_SECONDARY_DEF _sprintmsg sprintmsg
#define sprintmsg _sprintmsg
#endif

int
sprintmsg(str, fmt, va_alist)
char *str;
const char *fmt;
va_dcl
{
	int	cnt;
 	char	newfmt[BUFSIZ];
	va_list newarg[NL_ARGMAX*4];
	va_list	args, newargs;
	FILE siop;

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
		siop._cnt  = MAXINT;
		siop._base = siop._ptr = (unsigned char *)str;
		siop._flag = _IOWRT|_IODUMMY;
		siop.__fileL = siop.__fileH = 0xff;
		cnt = _doprnt(newfmt, newargs, &siop);
		*siop._ptr = '\0'; /* plant terminating null character */
	}
	va_end(ap);
	return(cnt);
}
