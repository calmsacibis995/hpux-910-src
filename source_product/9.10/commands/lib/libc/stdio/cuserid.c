/* @(#) $Revision: 64.10 $ */     
#ifndef XPG2
/*LINTLIBRARY*/
#ifdef _NAMESPACE_CLEAN
#define cuserid  _cuserid
#define endpwent _endpwent
#define geteuid   _geteuid
#define getpwuid _getpwuid
#define strcpy   _strcpy
#endif

#include <stdio.h>
#include <pwd.h>

extern char *strcpy();
extern int geteuid();
extern struct passwd *getpwuid();

#ifdef _NAMESPACE_CLEAN
#undef cuserid
#pragma _HP_SECONDARY_DEF _cuserid cuserid
#define cuserid _cuserid
#endif /* _NAMESPACE_CLEAN */

char *
cuserid(s)
char *s;
{
    static char res[L_cuserid];
    register struct passwd *pw;
    register char *p;

    if (s == NULL)
	p = res;
    else
	p = s;
    pw = getpwuid(geteuid());
    endpwent();
    if (pw != (struct passwd *)NULL)
	return strcpy(p, pw->pw_name);
    *p = '\0';
    if (s == NULL)
    	return (char *)NULL;
    else
	return(p);
}

#else /* XPG2 version */
/*LINTLIBRARY*/
#ifdef _NAMESPACE_CLEAN
#define cuserid  _cuserid
#define endpwent _endpwent
#define getlogin _getlogin
#define getuid   _getuid
#define getpwuid _getpwuid
#define strcpy   _strcpy
#endif

#include <stdio.h>
#include <pwd.h>

extern char *strcpy(), *getlogin();
extern int getuid();
extern struct passwd *getpwuid();
static char res[L_cuserid];

#ifdef _NAMESPACE_CLEAN
#undef cuserid
#pragma _HP_SECONDARY_DEF _cuserid cuserid
#define cuserid _cuserid
#endif /* _NAMESPACE_CLEAN */

char *
cuserid(s)
char	*s;
{
	register struct passwd *pw;
	register char *p;

	if (s == NULL)
		s = res;
	p = getlogin();
	if (p != NULL)
		return (strcpy(s, p));
	pw = getpwuid(getuid());
	endpwent();
	if (pw != NULL)
		return (strcpy(s, pw->pw_name));
	*s = '\0';
	return (NULL);
}
#endif
