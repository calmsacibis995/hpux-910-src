/* @(#) $Revision: 64.1 $ */   
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define fgetc _fgetc
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

#ifdef _NAMESPACE_CLEAN
#undef fgetc
#pragma _HP_SECONDARY_DEF _fgetc fgetc
#define fgetc _fgetc
#endif /* _NAMESPACE_CLEAN */

int
fgetc(fp)
register FILE *fp;
{
	return(getc(fp));
}
