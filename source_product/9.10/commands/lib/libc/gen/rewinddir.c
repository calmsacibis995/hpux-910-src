/* @(#) $Revision: 66.1 $ */

#ifdef _NAMESPACE_CLEAN
#   define seekdir _seekdir
#endif

#include <sys/types.h>
#include <dirent.h>
#undef rewinddir

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _rewinddir rewinddir
#define rewinddir _rewinddir
#endif

void
rewinddir(dirp)
register DIR *dirp;
{
     seekdir(dirp, (long)0);
}
