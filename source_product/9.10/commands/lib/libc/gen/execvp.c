/* @(#) $Revision: 70.2 $ */      
/*LINTLIBRARY*/
/*
 *	execlp(name, arg,...,0)	(like execl, but does path search)
 *	execvp(name, argv)	(like execv, but does path search)
 */

#ifdef _NAMESPACE_CLEAN
#define getenv _getenv
#define strchr _strchr
#define strlen _strlen
#define execlp _execlp
#define execv _execv
#define	sleep _sleep
#endif

#if defined(hp9000s500) || defined(hp9000s800)
/*
 *	execlp(name, arg,...,0)	removed from this file
 *				(its been rewritten by AJS)
 */
#endif hp9000s500 || hp9000s800

#include <sys/param.h>
#include <sys/errno.h>
#define	NULL	0

static char *execat(), shell[] = "/bin/sh";
extern char *getenv(), *strchr();
extern unsigned sleep();
extern int errno, execv();

#if !(defined hp9000s500) & !(defined hp9000s800)
/*
 * s500 and hp9000s800 execlp are written in assembler
 * to reverse the order of the arguments.
 */
/*VARARGS1*/

#ifdef _NAMESPACE_CLEAN
#undef execlp
#pragma _HP_SECONDARY_DEF _execlp execlp
#define execlp _execlp
#endif /* _NAMESPACE_CLEAN */

int
execlp(name, argv)
char	*name, *argv;
{
	return(execvp(name, &argv));
}
#endif !(defined hp9000s500) & !(defined hp9000s800)

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef execvp
#pragma _HP_SECONDARY_DEF _execvp execvp
#define execvp _execvp
#endif

int
execvp(name, argv)
char	*name, **argv;
{
	char	*pathstr;
	char	fname[MAXPATHLEN];
	char	*newargs[256];
	int	i;
	register char	*cp;
	register unsigned etxtbsy=1;
	register int soft_errno=0;

	if (name == NULL || *name == '\0') {
		errno = ENOENT;
		return(-1);
	}

	if ( !(strlen (name) < MAXPATHLEN) ) {
		errno = ENAMETOOLONG;
		return (-1);
	}

	if((pathstr = getenv("PATH")) == NULL)
		pathstr = ":/bin:/usr/bin";
	cp = strchr(name, '/')? "": pathstr;

	do {
		cp = execat(cp, name, fname);
	retry:
		(void) execv(fname, argv);
		switch(errno) {
		case ENOENT:
		case ENOTDIR:
			break;	/* try next PATH element, if any */
		case ENOEXEC:
			newargs[0] = "sh";
			newargs[1] = fname;
			for(i=1; newargs[i+1]=argv[i]; ++i) {
				if(i >= 254) {
					errno = E2BIG;
					return(-1);
				}
			}
			(void) execv(shell, newargs);
			return(-1);
		case ETXTBSY:
			if(++etxtbsy > 5)
				return(-1);
			(void) sleep(etxtbsy);
			goto retry;
		case ENOMEM:
		case E2BIG:
			return(-1);
		default:
			/* On these error (including EACCES, EFAULT,
			 * ENAMETOOLONG, ELOOP)
			 * continue trying other elements of PATH.
			 * If we ultimately fail, return the first of
			 * these soft errors encountered.
			 */
			if (soft_errno == 0)
				soft_errno = errno;
			break;
		}
	} while(cp);
	if(soft_errno != 0)
		errno = soft_errno;
	return(-1);
}

static char *
execat(s1, s2, si)
register char *s1, *s2;
char	*si;
{
	register char	*s;

	s = si;
	while(*s1 && *s1 != ':')
		*s++ = *s1++;
	if(si != s)
		*s++ = '/';
	while(*s2)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}
