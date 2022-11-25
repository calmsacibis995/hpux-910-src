/* @(#) $Revision: 64.2 $ */     
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define vfprintf _vfprintf
#ifdef __lint
#define ferror _ferror
#endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/*VARARGS2*/

#ifdef _NAMESPACE_CLEAN
#undef vfprintf
#pragma _HP_SECONDARY_DEF _vfprintf vfprintf
#define vfprintf _vfprintf
#endif /* _NAMESPACE_CLEAN */

int
vfprintf(iop, format, ap)
FILE *iop;
char *format;
va_list ap;
{
	register int count;

	if (_wrtchk(iop))
		return EOF;

	count = _doprnt(format, ap, iop);
	return(ferror(iop)? EOF: count);
}
