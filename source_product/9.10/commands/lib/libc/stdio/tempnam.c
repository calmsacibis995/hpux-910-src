/* @(#) $Revision: 64.5 $ */     
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define getenv _getenv
#define mktemp _mktemp
#define access _access
#define strlen _strlen
#define strcat _strcat
#define strcpy _strcpy
#define strncat _strncat
#define tempnam _tempnam
#       ifdef   _ANSIC_CLEAN
#define malloc _malloc
#       endif  /* _ANSIC_CLEAN */
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <string.h>

#define max(A,B) (((A)<(B))?(B):(A))

extern char *malloc(), *getenv(), *mktemp();
extern int access();

static char *pcopy(), *seed="AAA";

#ifdef _NAMESPACE_CLEAN
#undef tempnam
#pragma _HP_SECONDARY_DEF _tempnam tempnam
#define tempnam _tempnam
#endif /* _NAMESPACE_CLEAN */

char *
tempnam(dir, pfx)
char *dir;		/* use this directory please (if non-NULL) */
char *pfx;		/* use this (if non-NULL) as filename prefix */
{
	register char *p, *q, *tdir;
	int x=0, y=0, z;

	z=strlen(P_tmpdir);
	if((tdir = getenv("TMPDIR")) != NULL) {
		x = strlen(tdir);
	}
	if(dir != NULL) {
		y=strlen(dir);
	}
	if((p=malloc((unsigned)(max(max(x,y),z)+16))) == NULL)
		return(NULL);
	if(x > 0 && access(pcopy(p, tdir), 3) == 0)
		goto OK;
	if(y > 0 && access(pcopy(p, dir), 3) == 0)
		goto OK;
	if(access(pcopy(p, P_tmpdir), 3) == 0)
		goto OK;
	if(access(pcopy(p, "/tmp"), 3) != 0)
		return(NULL);
OK:
	(void)strcat(p, "/");
	if(pfx) {
		*(p+strlen(p)+5) = '\0';
		(void)strncat(p, pfx, 5);
	}
	(void)strcat(p, seed);
	(void)strcat(p, "XXXXXX");
	q = seed;
	while(*q == 'Z')
		*q++ = 'A';
	if (*q != '\0') ++*q;
	if(*mktemp(p) == '\0')
		return(NULL);
	return(p);
}

static char*
pcopy(space, arg)
char *space, *arg;
{
	char *p;

	if(arg) {
		(void)strcpy(space, arg);
		p = space-1+strlen(space);
		if(*p == '/')
			*p = '\0';
	}
	return(space);
}
