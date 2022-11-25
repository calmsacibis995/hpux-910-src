/* @(#) $Revision: 64.1 $ */   
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define fputc _fputc
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

#ifdef _NAMESPACE_CLEAN
#undef fputc
#pragma _HP_SECONDARY_DEF _fputc fputc
#define fputc _fputc
#endif /* _NAMESPACE_CLEAN */

int
fputc(c, fp)
int	c;
register FILE *fp;
{
	return(putc(c, fp));
}
