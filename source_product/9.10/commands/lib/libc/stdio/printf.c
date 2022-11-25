/* @(#) $Revision: 64.4 $ */      
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define printf _printf
#ifdef __lint
#define ferror _ferror
#endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/*VARARGS1*/
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef printf
#pragma _HP_SECONDARY_DEF _printf printf
#define printf _printf
#endif

int
printf(format, va_alist)
const char *format;
va_dcl
{
	register int count;
	va_list ap;

	va_start(ap);
	if ( _wrtchk(stdout) )
		return EOF;
	count = _doprnt(format, ap, stdout);
	va_end(ap);
	return(ferror(stdout)? EOF: count);
}
