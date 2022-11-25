/*LINTLIBRARY*/
/*
 * A subroutine version of the macros ferror, feof, clearerr
 * and fileno.
 */
#include <stdio.h>

#undef ferror

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _ferror ferror
#define ferror _ferror
#endif /* _NAMESPACE_CLEAN */

int
ferror(fp)
FILE *fp;
{
	return((fp)->_flag & _IOERR);
}

#undef feof

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _feof feof
#define feof _feof
#endif /* _NAMESPACE_CLEAN */

int 
feof(fp)
FILE *fp;
{
	return((fp)->_flag & _IOEOF);
}


#undef clearerr

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _clearerr clearerr
#define clearerr _clearerr
#endif /* _NAMESPACE_CLEAN */

void
clearerr(fp)
FILE *fp;
{
	((void) ((fp)->_flag &= ~(_IOERR | _IOEOF)));
}

#undef fileno

#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _fileno fileno
#define fileno _fileno
#endif /* _NAMESPACE_CLEAN */

int 
fileno(fp)
FILE *fp;
{
	return (fp->__fileH << 8) | fp->__fileL;
}
