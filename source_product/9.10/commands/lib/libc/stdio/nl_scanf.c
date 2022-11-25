/* @(#) $Revision: 66.1 $ */      
/*LINTLIBRARY*/

/*
*****************************************************************************
** The nls functions -- nl_scanf, nl_fscanf and nl_sscanf --
** now have the same functionality as their regular scanf counterparts.
** Two kinds of conversion specifications are recognized: "%c" and
** "%n$c" where "c" is a conversion character and "n" is a digit between
** 1 and NL_ARGMAX.  The format can contain either form of the conversion
** specification, but they can't be mixed.  Skip fields are an exception,
** either "%*" or "%n$*" may be used to designate skip fields.  The digit
** in "%n$*" is ignored.
**
** Format strings containing "%n$c" specifications and the corresponding
** argument list are rewritten to use regular conversion specifications 
** of the form "%c" in _doscan().
*****************************************************************************
*/

#ifdef _NAMESPACE_CLEAN
#define nl_scanf _nl_scanf
#define nl_sscanf _nl_sscanf
#define nl_fscanf _nl_fscanf
#define strlen _strlen
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <varargs.h>
#include "stdiom.h"

extern int _doscan();

/* VARARGS1 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_scanf
#pragma _HP_SECONDARY_DEF _nl_scanf nl_scanf
#define nl_scanf _nl_scanf
#endif

int
nl_scanf(fmt, va_alist)
const char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);

	return _doscan(stdin, fmt, ap);
}

/* VARARGS2 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_fscanf
#pragma _HP_SECONDARY_DEF _nl_fscanf nl_fscanf
#define nl_fscanf _nl_fscanf
#endif

int
nl_fscanf(iop, fmt, va_alist)
FILE *iop;
const char *fmt;
va_dcl
{
	va_list ap;

	va_start(ap);

	return _doscan(iop, fmt, ap);
}

/* VARARGS2 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_sscanf
#pragma _HP_SECONDARY_DEF _nl_sscanf nl_sscanf
#define nl_sscanf _nl_sscanf
#endif

int
nl_sscanf(str, fmt, va_alist)
register char *str;
const char *fmt;
va_dcl
{
	va_list ap;
	FILE strbuf;

	va_start(ap);

	strbuf._flag = _IOREAD | _IODUMMY;
	strbuf._ptr = strbuf._base = (unsigned char*)str;
	strbuf._cnt = strlen(str);
	strbuf.__fileL = strbuf.__fileH = 0xff;

	return _doscan(&strbuf, fmt, ap);
}
