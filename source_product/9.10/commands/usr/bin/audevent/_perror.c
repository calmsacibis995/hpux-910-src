/*
 * @(#) $Revision: 63.1 $
 */

#include <errno.h>

extern	char		*sys_errlist[];

char *
_perror()
{
	auto	int		save_errno;

	save_errno = errno;
	errno = 0;
	return(sys_errlist[save_errno]);
}
