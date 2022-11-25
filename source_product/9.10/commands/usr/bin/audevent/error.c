/*
 * @(#) $Revision: 63.1 $
 */

#include <varargs.h>
#include <stdio.h>

int
error(function, format, va_alist)
char *function;
char *format;
va_dcl
{
	va_list ap;

	va_start(ap);
	(void)fflush(stdout);
	(void)fprintf(stderr, "%s: ", function);
	(void)_doprnt(format, ap, stderr);
	va_end(ap);
	return(ferror(stderr) ? (EOF) : (0));
}
