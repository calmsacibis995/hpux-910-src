/* @(#) $Revision: 64.6 $ */     
/*LINTLIBRARY*/
/*
 * A subroutine version of the macro getchar.
 */

#ifdef _NAMESPACE_CLEAN
# ifdef __lint
#define getc _getc
#define getchar _getchar
# endif /* __lint */
#define _filbuf __filbuf
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

#undef getchar

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _getchar getchar
#define getchar _getchar
#endif

int
getchar()
{
	return(getc(stdin));
}


#undef getc

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _getc getc
#define getc _getc
#endif
int 
getc(fp)
register FILE *fp;
{
	if (--(fp)->_cnt < 0)
		return(_filbuf(fp));
	else
		return((int) *(fp)->_ptr++);
}
