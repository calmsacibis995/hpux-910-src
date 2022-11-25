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
#ifdef __lint
#define ferror _ferror
#endif /* __lint */
#define nl_fprintf _nl_fprintf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/* VARARGS2 */

#ifdef _NAMESPACE_CLEAN
#undef nl_fprintf
#pragma _HP_SECONDARY_DEF _nl_fprintf nl_fprintf
#define nl_fprintf _nl_fprintf
#endif /* _NAMESPACE_CLEAN */

int
nl_fprintf(fp, fmt, va_alist)
FILE *fp;
const char *fmt;
va_dcl
{
	int cnt;
	va_list args;

	if (_wrtchk( fp)) return EOF;

	va_start( args);
	cnt = _doprnt( fmt, args, fp);
	va_end( args);

	return ferror( fp) ? EOF: cnt;
}
