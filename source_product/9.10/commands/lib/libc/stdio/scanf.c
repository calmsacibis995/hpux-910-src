/* @(#) $Revision: 66.1 $ */   
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define strlen _strlen
#define scanf _scanf
#define fscanf _fscanf
#define sscanf _sscanf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>
#include "stdiom.h"

extern int _doscan();

/*VARARGS1*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef scanf
#pragma _HP_SECONDARY_DEF _scanf scanf
#define scanf _scanf
#endif

int
scanf(fmt, va_alist)
const char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return _doscan(stdin, fmt, ap);
}

/*VARARGS2*/
/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef fscanf
#pragma _HP_SECONDARY_DEF _fscanf fscanf
#define fscanf _fscanf
#endif

int
fscanf(iop, fmt, va_alist)
FILE *iop;
const char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);
	return _doscan(iop, fmt, ap);
}

/*VARARGS2*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef sscanf
#pragma _HP_SECONDARY_DEF _sscanf sscanf
#define sscanf _sscanf
#endif

int
sscanf(str, fmt, va_alist)
register char *str;
const char *fmt;
va_dcl
{
	va_list ap;
	FILE strbuf;

	va_start(ap);
	strbuf._flag = _IOREAD|_IODUMMY;
	strbuf._ptr = strbuf._base = (unsigned char*)str;
	strbuf._cnt = strlen(str);
	strbuf.__fileL = strbuf.__fileH = 0xff;
	return _doscan(&strbuf, fmt, ap);
}
