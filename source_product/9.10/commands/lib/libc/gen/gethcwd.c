/* @(#) $Revision: 64.9 $ */      
/*LINTLIBRARY*/
/*
 * Library routine to GET the Current Working Directory.
 * arg1 is a pointer to a character buffer into which the
 * path name of the current directory is placed by the
 * subroutine.  arg1 may be zero, in which case the 
 * subroutine will call malloc to get the required space.
 * arg2 is the length of the buffer space for the path-name.
 * If the actual path-name is longer than (arg2-2), or if
 * the value of arg2 is not at least 3, the subroutine will
 * return a value of zero, with errno set as appropriate.
 */

#ifdef _NAMESPACE_CLEAN
#define popen _popen
# ifdef __lint
# define fileno _fileno  /* get function instead of macro */
# endif
#define read _read
#define pclose _pclose
#define gethcwd _gethcwd
#       ifdef   _ANSIC_CLEAN
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif

#include <stdio.h>
#include <sys/errno.h>

extern FILE *popen();
extern char *malloc(), *fgets(), *strchr();
extern int errno, pclose();
extern int read();
extern _findbuf();
#define MIN(x, y)	(x < y ? x : y)

#ifdef _NAMESPACE_CLEAN
#undef gethcwd
#pragma _HP_SECONDARY_DEF _gethcwd gethcwd
#define gethcwd _gethcwd
#endif
char *
gethcwd(arg1, arg2)
char	*arg1;
int	arg2;
{
	FILE	*pipe;
	int     buf_read;
        int     errno_save;

	if(arg2 <= 0) {
		errno = EINVAL;
		return(0);
	}
	if(arg1 == 0)
		if((arg1 = malloc((unsigned)arg2)) == 0) {
			errno = ENOMEM;
			return(0);
		}
	errno = 0;
#if defined(DUX) || defined(DISKLESS)
	if((pipe = popen("pwd -H", "r")) == 0)
#else
	if((pipe = popen("pwd", "r")) == 0)
#endif
		return(0);
/*
   Fix for POSIX standard. Original code use fgets() which needs
   arg2 >= pathname+2. New code use read() which needs only
   arg2 >= pathname+1 and could eliminate the problem for
   a new line character (\n) embedded in the path name 
*/
	if (pipe->_base == NULL)  /* get buffer if we don't have one */
		_findbuf(pipe);
	buf_read = read(fileno(pipe), arg1,
			(unsigned) MIN( arg2 , _bufsiz(pipe)) );
/* Save errno returned from read */
        errno_save = errno;

/* pclose() return -1 for failure. */
	if (pclose(pipe) < 0 ) {
		errno = EACCES;
		return(0);
	}
/* Before returning to caller, restore errno from read */
        if (errno_save != 0)  errno = errno_save;

/* read() return -1 for error */
	if (buf_read < 0 )	/* read error */
		return(0);
/* pipe is always \n terminated */
	if (arg1[buf_read - 1] != '\n') { 
		errno = ERANGE;
		return(0);
	}
/* overwrite \n with NULL */
	arg1[buf_read - 1] = '\0';
	return(arg1);
}
