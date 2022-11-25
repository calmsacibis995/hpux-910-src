/* @(#) $Revision: 64.5 $ */     
/* LINTLIBRARY */

/*
    8/27/87:

	1)  Revised to call new version of _printmsg(),
	    using local argument list.

	2)  Prior version supported only a BUFSIZ output
	    string.  Now, only the caller's format string
	    string has this size limitation.

    10/03/88:

	1)  pre-processing numbered argument format string
	    now done in _doprnt().
*/

#ifdef _NAMESPACE_CLEAN
#define nl_printf _nl_printf
#ifdef __lint
#define ferror _ferror
#endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/* VARARGS1 */

#ifdef _NAMESPACE_CLEAN
#undef nl_printf
#pragma _HP_SECONDARY_DEF _nl_printf nl_printf
#define nl_printf _nl_printf
#endif

int
nl_printf(fmt, va_alist)
const char *fmt;
va_dcl
{
	int cnt;
	va_list args;

	if (_wrtchk( stdout)) return EOF;

	va_start( args);
	cnt = _doprnt( fmt, args, stdout);
	va_end( args);

	return ferror( stdout) ? EOF: cnt;
}
