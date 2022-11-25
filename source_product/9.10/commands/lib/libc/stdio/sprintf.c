/* @(#) $Revision: 66.1 $ */     
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define sprintf _sprintf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>
#include <values.h>
#include "stdiom.h"

extern int _doprnt();

/*VARARGS2*/
#ifdef _NAMESPACE_CLEAN
#undef sprintf
#pragma _HP_SECONDARY_DEF _sprintf sprintf
#define sprintf _sprintf
#endif /* _NAMESPACE_CLEAN */

int
sprintf(string, format, va_alist)
char *string;
const char *format;
va_dcl
{
	register int count;
	FILE siop;
	va_list ap;

	siop._flag = _IOWRT|_IODUMMY;
	siop._cnt = MAXINT;
	siop._base = siop._ptr = (unsigned char *)string;
	siop.__fileL = siop.__fileH = 0xff;

	va_start(ap);
	count = _doprnt(format, ap, &siop);
	va_end(ap);
	*siop._ptr = '\0'; /* plant terminating null character */
	return count;
}
