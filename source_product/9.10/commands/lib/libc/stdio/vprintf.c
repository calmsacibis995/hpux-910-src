/* @(#) $Revision: 64.2 $ */      
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define vprintf _vprintf
#ifdef __lint
#define ferror _ferror
#endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/*VARARGS1*/

#ifdef _NAMESPACE_CLEAN
#undef vprintf
#pragma _HP_SECONDARY_DEF _vprintf vprintf
#define vprintf _vprintf
#endif

int
vprintf(format, ap)
char *format;
va_list ap;
{
	register int count;

	if (_wrtchk(stdout))
		return EOF;

	count = _doprnt(format, ap, stdout);
	return(ferror(stdout)? EOF: count);
}
