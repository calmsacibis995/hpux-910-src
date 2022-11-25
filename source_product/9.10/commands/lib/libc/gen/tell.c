/* @(#) $Revision: 64.3 $ */    
/*LINTLIBRARY*/
/*
 * return offset in file.
 */
#ifdef _NAMESPACE_CLEAN
#define lseek _lseek
#endif

extern long lseek();
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _tell tell
#define tell _tell
#endif

long
tell(f)
int	f;
{
	return(lseek(f, 0L, 1));
}
