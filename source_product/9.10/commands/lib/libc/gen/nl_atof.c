/* @(#) $Revision: 64.8 $ */      
/*LINTLIBRARY*/
/*
 *	C library - ascii to floating (atof) and string to double (strtod)
 *		    while accounting for radix character differences
 *
 *	10/29/86:  changed to call atof(), strtod() directly, which have
 *		   been modified to handle the radix char directly.
 *
 *		   Chris Butler, Contractor
 */

#ifdef _NAMESPACE_CLEAN
#define atof _atof
#define strtod _strtod
#define nl_atof _nl_atof
#define nl_strtod _nl_strtod
#define langinit _langinit
#define idtolang _idtolang
#define _nl_failid __nl_failid
#endif /* _NAMESPACE_CLEAN */

#include <setlocale.h>
#include <locale.h>

extern int	 langinit();
extern char	*idtolang();
extern int	 _nl_failid;

#ifdef _NAMESPACE_CLEAN
#undef nl_atof
#pragma _HP_SECONDARY_DEF _nl_atof nl_atof
#define nl_atof _nl_atof
#endif /* _NAMESPACE_CLEAN */

double nl_atof(str, langid)
char *str;
int langid;
{
	double atof();
	char *idtolang();

	if (langid != __nl_langid[LC_NUMERIC] && langid != _nl_failid)
		langinit(idtolang(langid));

	return(atof(str));
}

#ifdef _NAMESPACE_CLEAN
#undef nl_strtod
#pragma _HP_SECONDARY_DEF _nl_strtod nl_strtod
#define nl_strtod _nl_strtod
#endif /* _NAMESPACE_CLEAN */

double nl_strtod(str, ptr, langid)
char *str;
char **ptr;
int langid;
{
	double strtod();
	char *idtolang();

	if (langid != __nl_langid[LC_NUMERIC]  && langid != _nl_failid)
		langinit(idtolang(langid));

	return(strtod(str, ptr));
}
