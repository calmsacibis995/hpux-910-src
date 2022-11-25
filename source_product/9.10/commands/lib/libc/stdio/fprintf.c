/* @(#) $Revision: 64.6 $ */     
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#ifdef __lint
#define ferror _ferror
#endif /* __lint */
#define fprintf _fprintf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/*VARARGS2*/

#ifdef _NAMESPACE_CLEAN
#undef fprintf
#pragma _HP_SECONDARY_DEF _fprintf fprintf
#define fprintf _fprintf
#endif /* _NAMESPACE_CLEAN */

int fprintf(iop, format, va_alist)
FILE *iop;
const char *format;
va_dcl
{
	register int count;
	va_list ap;

	va_start(ap);
	if ( _wrtchk(iop) )
		return EOF;
	count = _doprnt(format, ap, iop);
	va_end(ap);
	return(ferror(iop)? EOF: count);
}
