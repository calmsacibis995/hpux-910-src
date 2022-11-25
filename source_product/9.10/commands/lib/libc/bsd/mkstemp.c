/* @(#) $Revision: 66.1 $ */

#ifdef _NAMESPACE_CLEAN
#define mkstemp _mkstemp
#define getpid _getpid
#define open _open
#define strlen _strlen
#endif /* _NAMESPACE_CLEAN */

#include <sys/file.h>

extern int getpid(), open(), strlen();

#ifdef _NAMESPACE_CLEAN
#undef mkstemp
#pragma _HP_SECONDARY_DEF _mkstemp mkstemp
#define mkstemp _mkstemp
#endif /* _NAMESPACE_CLEAN */

int
mkstemp(as)
char *as;
{
	register char *s=as;
	register unsigned int pid;
	register int fd;
	register unsigned i=0;

	pid = getpid();
	s += strlen(as);  /* point at the terminal null */
	while (*--s == 'X') {
		*s = (pid % 10) + '0';
		pid /= 10;
		i++;
	}
	if ((*++s) && i>5) {
		*s = 'a';
		while ((fd = open(as, O_CREAT|O_EXCL|O_RDWR, 0600)) == -1) {
			if (++*s > 'z')
				return(-1);
		}
	} else {
		fd = open(as, O_CREAT|O_EXCL|O_RDWR, 0600);
	}
	return(fd);
}
