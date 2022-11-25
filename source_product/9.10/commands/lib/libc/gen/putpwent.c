/* @(#) $Revision: 64.7 $ */
/*LINTLIBRARY*/
/*
 * format a password file entry
 */

#ifdef _NAMESPACE_CLEAN
#define putpwent _putpwent
#define fputc _fputc
#define fputs _fputs
#define ltoa  _ltoa
#define ultoa _ultoa
#define strlen _strlen
#ifdef __lint
#   define ferror _ferror
#   define isspace _isspace
#endif
#ifdef SHADOWPWD
#   define putspwent _putspwent
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <ctype.h>

extern char *ltoa();
extern char *ultoa();

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef putpwent
#pragma _HP_SECONDARY_DEF _putpwent putpwent
#define putpwent _putpwent
#endif


int
putpwent(p, f)
register struct passwd *p;
register FILE *f;
{
	if(_validate_pw(p))
			return(ferror(f));
	fputs(p->pw_name, f); fputc(':', f);
	fputs(p->pw_passwd, f);

	if ((*p->pw_age) != '\0')
	{
		fputc(',', f);
		fputs(p->pw_age, f);
	}
	fputc(':', f);

#ifdef HP_NFS
	fputs(ltoa(p->pw_uid), f);  fputc(':', f);
#else
	fputs(ultoa(p->pw_uid), f); fputc(':', f);
#endif /* HP_NFS */
	fputs(ultoa(p->pw_gid), f); fputc(':', f);
	fputs(p->pw_gecos, f);      fputc(':', f);
	fputs(p->pw_dir, f);        fputc(':', f);
	fputs(p->pw_shell, f);      fputc('\n', f);
	return(ferror(f));
}

/*
 * validate(struct passwd p)
 * 	checks for p->pw_name != NULL
 * 	and p->pw_name not just white space.
 */
_validate_pw(p)
struct	passwd	*p;
{
	char	*s = p->pw_name;

	if(strlen(s) <= 0)
		return(1);
	while(*s++ != NULL)
		if(!isspace((int)(*s)))
			return(0);
	if(--*s == NULL)
		return(1);
	return(0);
}

#ifdef SHADOWPWD

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef putspwent
#pragma _HP_SECONDARY_DEF _putspwent putspwent
#define putspwent _putspwent
#endif

int
putspwent(p, f)
register struct s_passwd *p;
register FILE *f;
{
	fputs(p->pw_name, f); fputc(':', f);
	fputs(p->pw_passwd, f);

	if ((*p->pw_age) != '\0')
	{
		fputc(',', f);
		fputs(p->pw_age, f);
	}
	fputc(':', f);

	fputs(ultoa(p->pw_audid), f);  fputc(':', f);
	fputs(ultoa(p->pw_audflg), f); fputc('\n', f);

	return ferror(f);
}
#endif
