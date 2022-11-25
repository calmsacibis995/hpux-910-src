/* @(#) $Revision: 66.1 $ */      
/*LINTLIBRARY*/
/*	calloc - allocate and clear memory block
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#       ifdef   _ANSIC_CLEAN
#define malloc _malloc
#define free _free 
#       endif  /* _ANSIC_CLEAN */
#define calloc _calloc
#define cfree _cfree
#define memset _memset
#endif

#include <stdlib.h>

extern void *memset();
extern void free();
extern char *_curbrk;

#ifdef _NAMESPACE_CLEAN
#undef calloc
#pragma _HP_SECONDARY_DEF _calloc calloc
#define calloc _calloc
#endif /* _NAMESPACE_CLEAN */

void *
calloc(num, size)
size_t num, size;
{
	register char *mp,*b;

	b = _curbrk;
	if((mp = malloc(num *= size)) != NULL) {
		num=(b>mp && b-mp<num)?b-mp:num;
		(void)memset(mp, 0, (int)num);
	}
	return(mp);

}

/*ARGSUSED*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef cfree
#pragma _HP_SECONDARY_DEF _cfree cfree
#define cfree _cfree
#endif

void
cfree(p, num, size)
char *p;
size_t num, size;
{
	free(p);
}
