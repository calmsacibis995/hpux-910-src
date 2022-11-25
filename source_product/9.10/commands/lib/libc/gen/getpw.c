/* @(#) $Revision: 64.5 $ */      
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define getpw _getpw
#define fopen _fopen
#define rewind _rewind
#endif

#include <stdio.h>
#include <ctype.h>

extern void rewind();
extern FILE *fopen();

static FILE *pwf;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef getpw
#pragma _HP_SECONDARY_DEF _getpw getpw
#define getpw _getpw
#endif

int
getpw(uid, buf)
int	uid;
char	buf[];
{
	register n, c;
	register char *bp;

	if(pwf == 0)
		pwf = fopen("/etc/passwd", "r");
	if(pwf == NULL)
		return(1);
	rewind(pwf);

	while(1) {
		bp = buf;
		while((c=getc(pwf)) != '\n') {
			if(c == EOF)
				return(1);
			*bp++ = c;
		}
		*bp = '\0';
		bp = buf;
		n = 3;
		while(--n)
			while((c = *bp++) != ':')
				if(c == '\n')
					return(1);
		while((c = *bp++) != ':')
			if(isdigit(c))
				n = n*10+c-'0';
			else
				continue;
		if(n == uid)
			return(0);
	}
}
