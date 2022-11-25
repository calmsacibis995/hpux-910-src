/* @(#) $Revision: 66.3 $ */   
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define vscanf  _vscanf
#define vfscanf _vfscanf
#define vsscanf _vsscanf
#define strlen  _strlen
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>
#include "stdiom.h"

extern int _doscan();

/* VARARGS1 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef vscanf
#pragma _HP_SECONDARY_DEF _vscanf vscanf
#define vscanf _vscanf
#endif

int
vscanf(fmt, ap)
const char *fmt;
va_list ap;
{
	return _doscan(stdin, fmt, ap);
}

/* VARARGS2 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef vfscanf
#pragma _HP_SECONDARY_DEF _vfscanf vfscanf
#define vfscanf _vfscanf
#endif

int
vfscanf(iop, fmt, ap)
FILE *iop;
const char *fmt;
va_list ap;
{
	return _doscan(iop, fmt, ap);
}

/* VARARGS2 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef vsscanf
#pragma _HP_SECONDARY_DEF _vsscanf vsscanf
#define vsscanf _vsscanf
#endif

int
vsscanf(str, fmt, ap)
register char *str;
const char *fmt;
va_list ap;
{
	FILE strbuf;

	strbuf._flag = _IOREAD|_IODUMMY;
	strbuf._cnt = strlen(str);
	strbuf._ptr = strbuf._base = (unsigned char*)str;
	strbuf.__fileL = strbuf.__fileH = 0xff;

	return _doscan(&strbuf, fmt, ap);
}
