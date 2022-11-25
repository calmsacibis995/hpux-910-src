/* @(#) $Revision: 64.3 $ */    

#ifdef _NAMESPACE_CLEAN
#define stat _stat
#define ftok _ftok
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#define MASK 0x0000003f
#define mhash(n) ((n) ^ (n >> 6) ^ (n >> 12) ^ (n >> 18))
#define ihash(n) ((n >> 18) ^ (n >> 24) ^ (n >> 30))

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef ftok
#pragma _HP_SECONDARY_DEF _ftok ftok
#define ftok _ftok
#endif

key_t
ftok(path, id)
char *path;
char id;
{
	struct stat st;

	return(stat(path, &st) < 0 ? (key_t)-1 :
	    (key_t)((key_t)id << 24 |
		((mhash((long)(unsigned)minor(st.st_dev)) ^
			ihash((unsigned)st.st_ino)) & MASK) << 18 |
		(unsigned)st.st_ino & 0x3ffff));
}
