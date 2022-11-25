/* @(#) $Revision: 64.3 $ */

/*
 *  mkfifo - make a fifo
 *  create a fifo named pathname with permissions mode
 */
#ifdef _NAMESPACE_CLEAN
#define mknod _mknod
#define mkfifo _mkfifo
#endif

#include <sys/types.h>
#include <sys/stat.h>

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef mkfifo
#pragma _HP_SECONDARY_DEF _mkfifo mkfifo
#define mkfifo _mkfifo
#endif

mkfifo( pathname, mode)
char	*pathname;
mode_t	mode;

{
	return( mknod( pathname, S_IFIFO | (mode & 0777), 0));
}
