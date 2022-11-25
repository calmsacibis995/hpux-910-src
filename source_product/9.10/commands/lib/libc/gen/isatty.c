/* @(#) $Revision: 66.1 $ */   
/*LINTLIBRARY*/
/*
 * Returns 1 iff file is a tty
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define isatty _isatty
#define ioctl _ioctl
#endif

#include <sys/termio.h>
#include <errno.h>

extern int ioctl();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef isatty
#pragma _HP_SECONDARY_DEF _isatty isatty
#define isatty _isatty
#endif

int
isatty(f)
int	f;
{
        register int sav_err;
        register int ret_val;
	struct termio tty;

        sav_err = errno;
	errno = 0;
	ret_val = ioctl(f, TCGETA, &tty);

	if (errno != 0)
	{
		if (errno == EINVAL)
			errno = ENOTTY;
	}
	else
		errno = sav_err;

	return (ret_val < 0 ? 0 : 1);
}
