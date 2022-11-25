/* @(#) $Revision: 64.4 $ */     
/*LINTLIBRARY*/
/*
 * A subroutine version of the macro putchar
 */

#ifdef _NAMESPACE_CLEAN
#define _flsbuf __flsbuf
#  ifdef __lint
#  define putc _putc
#  define putchar _putchar
#  endif /* __lint */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#undef putchar

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _putchar putchar
#define putchar _putchar
#endif

int
putchar(c)
register char c;
{
	return(putc(c, stdout));
}

#undef putc

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _putc putc
#define putc _putc
#endif

int 
putc(c, fp)
register char c;
register FILE *fp;
{
	if (--(fp)->_cnt < 0)
	   return(_flsbuf((unsigned char) (c), (fp)));
	else
	   return((int) (*(fp)->_ptr++ = (unsigned char) (c)));
}
