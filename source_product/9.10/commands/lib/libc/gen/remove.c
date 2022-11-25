/* @(#) $Revision: 64.4 $ */     
/* LINTLIBRARY */
/*
 *	remove()
 */

#ifdef _NAMESPACE_CLEAN
#define rmdir _rmdir
#define unlink _unlink
#define lstat _lstat
#define remove _remove
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef SYMLINKS
#   define STAT  lstat
#else
#   define STAT  stat
#endif

/*
 * remove() removes a file regardless of whether it is a directory or a
 * regular file.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef remove
#pragma _HP_SECONDARY_DEF _remove remove
#define  remove _remove
#endif

remove(path)
register const char *path;
{
	struct stat st;

	if (STAT(path, &st))			/* get info on file type */
		return -1;
	
	if ((st.st_mode & S_IFMT) == S_IFDIR)
		return rmdir(path);		/* remove directory */
	else
		return unlink(path);		/* remove file */
}
