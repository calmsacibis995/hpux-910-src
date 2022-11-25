/* @(#) $Revision: 64.3 $ */      
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define mktemp _mktemp
#define strcpy _strcpy
#define strcat _strcat
#define tmpnam _tmpnam
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>

extern char *mktemp(), *strcpy(), *strcat();
static char str[L_tmpnam], seed[] = { 'a', 'a', 'a', '\0' };

#ifdef _NAMESPACE_CLEAN
#undef tmpnam
#pragma _HP_SECONDARY_DEF _tmpnam tmpnam
#define tmpnam _tmpnam
#endif /* _NAMESPACE_CLEAN */

char *
tmpnam(s)
char	*s;
{
	register char *p, *q;

	p = (s == NULL)? str: s;
	(void) strcpy(p, P_tmpdir);
	(void) strcat(p, seed);
	(void) strcat(p, "XXXXXX");

	q = seed;
	while(*q == 'z')
		*q++ = 'a';
	if (*q != '\0') ++*q;

	(void) mktemp(p);
	return(p);
}
