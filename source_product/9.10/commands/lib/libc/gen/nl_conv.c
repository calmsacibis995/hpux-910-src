/* @(#) $Revision: 64.6 $ */   

/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _nl_failid __nl_failid
#define idtolang _idtolang
#define langinit _langinit
#define nl_tolower _nl_tolower
#define nl_toupper _nl_toupper
#endif

#include	<setlocale.h>
#include	<ctype.h>
#include	<locale.h>

extern int	 langinit();
extern char	*idtolang();
extern int	 _nl_failid;

#ifdef _NAMESPACE_CLEAN
#undef nl_toupper
#pragma _HP_SECONDARY_DEF _nl_toupper nl_toupper
#define nl_toupper _nl_toupper
#endif /* _NAMESPACE_CLEAN */

int nl_toupper(c, langid)
register int c, langid;
{
	if (langid != __nl_langid[LC_CTYPE] && langid != _nl_failid)
		langinit(idtolang(langid));

	if (_sh_low <= c && c <= _sh_high)
		return _toupper(c);
	else
		return (c);
}

#ifdef _NAMESPACE_CLEAN
#undef nl_tolower
#pragma _HP_SECONDARY_DEF _nl_tolower nl_tolower
#define nl_tolower _nl_tolower
#endif /* _NAMESPACE_CLEAN */

int nl_tolower(c, langid)
register int c, langid;
{
	if (langid != __nl_langid[LC_CTYPE] && langid != _nl_failid)
		langinit(idtolang(langid));

	if (_sh_low <= c && c <= _sh_high)
		return _tolower(c);
	else
		return (c);
}
