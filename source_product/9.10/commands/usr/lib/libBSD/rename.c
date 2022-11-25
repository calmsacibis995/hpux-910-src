/* HPUX_ID: @(#) $Revision: 64.1 $  */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#define TRUE	1
#define FALSE	0

/* emulate the BSD rename call */
rename(from, to)
char *from, *to;
{
	struct stat from_stat, to_stat;
	int to_there = 0;
	if ((stat(to, &to_stat) == 0))
		to_there++;
	if (stat(from, &from_stat)) return -1;
	if (!same_type(&from_stat, &to_stat))
		return -1;	/* errno is set is the routine */
	if (!same_fs(&from_stat, &to_stat))
		return -1;
	if ((to_there++) && unlink(to)) return -1;
	if (link(from, to) || unlink(from))
		return -1;
	else
		return 0;
}

/* test to make sure both are directories or that both are not */
same_type(from_stat, to_stat)
struct stat *from_stat, *to_stat;
{
	ushort ftype, ttype;
	ftype = from_stat->st_mode & S_IFMT;
	ttype = to_stat->st_mode & S_IFMT;
	if ((ftype != S_IFDIR) && (ttype != S_IFDIR))
		return TRUE;
	else if ((ftype == S_IFDIR) && (ttype == S_IFDIR))
		return TRUE;
	else if ((ftype == S_IFDIR) && (ttype != S_IFDIR)) {
		errno = ENOTDIR;
		return FALSE;
	} else { /* ((ftype != S_IFDIR) && (ttype == S_IFDIR)) */
		errno = EISDIR;
		return FALSE;
	}
}

/* test to make sure both are on the same file system */
same_fs(from_stat, to_stat)
struct stat *from_stat, *to_stat;
{
	if (from_stat->st_rdev == to_stat->st_rdev)
		return TRUE;
	else {
		errno = EXDEV;
		return FALSE;
	}
}
