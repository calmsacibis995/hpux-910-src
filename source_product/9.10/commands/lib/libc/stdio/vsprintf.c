/* @(#) $Revision: 66.1 $ */     
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define vsprintf _vsprintf
#endif

#include <stdio.h>
#include <varargs.h>
#include <values.h>
#include "stdiom.h"

extern int _doprnt();

/*VARARGS2*/

#ifdef _NAMESPACE_CLEAN
#undef vsprintf
#pragma _HP_SECONDARY_DEF _vsprintf vsprintf
#define vsprintf _vsprintf
#endif

int
vsprintf(string, format, ap)
char *string, *format;
va_list ap;
{
	register int count;
	FILE siop;

	siop._flag = _IOWRT|_IODUMMY;
	siop._cnt = MAXINT;
	siop._base = siop._ptr = (unsigned char *)string;
	siop.__fileL = siop.__fileH = 0xff;
	count = _doprnt(format, ap, &siop);
	*siop._ptr = '\0'; /* plant terminating null character */
	return count;
}
