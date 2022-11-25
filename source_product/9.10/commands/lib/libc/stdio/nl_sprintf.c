/* @(#) $Revision: 66.1 $ */    
/* LINTLIBRARY */

/*
    8/27/87:

	1)  Rewritten to use new version of _printmsg(),
	    featuring use of a local argument list and
	    16 bit character support.

	2)  The caller's format string is now limited to
	    BUFSIZ bytes.

    10/03/88:

	1)  pre-processing numbered argument format string
	    now done in _doprnt().
*/

#ifdef _NAMESPACE_CLEAN
#define nl_sprintf _nl_sprintf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>
#include <values.h>
#include "stdiom.h"

extern int _doprnt();

/* VARARGS2 */

#ifdef _NAMESPACE_CLEAN
#undef nl_sprintf
#pragma _HP_SECONDARY_DEF _nl_sprintf nl_sprintf
#define nl_sprintf _nl_sprintf
#endif /* _NAMESPACE_CLEAN */

int
nl_sprintf(str, fmt, va_alist)
char *str;
const char *fmt;
va_dcl
{
	int cnt;
	va_list args;
	FILE siop;

	siop._flag = _IOWRT|_IODUMMY;
	siop._cnt  = MAXINT;
	siop._base = siop._ptr = (unsigned char *)str;
	siop.__fileL = siop.__fileH = 0xff;

	va_start(args);
	cnt = _doprnt(fmt, args, &siop);
	va_end(args);

	*siop._ptr = '\0';	/* terminating null character */

	return cnt;
}
