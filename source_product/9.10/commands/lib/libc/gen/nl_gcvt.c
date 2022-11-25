/* @(#) $Revision: 64.7 $ */      


/*
 *	C library - floating to ascii (nl_gcvt) inserting the proper radix
 *		    character according to the native language.
 *
 *	10/31/86:  modified to simply call gcvt(), which now directly
 *		   supports NLS radix characters.
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define _nl_failid __nl_failid
#define idtolang _idtolang
#define langinit _langinit
#define nl_gcvt _nl_gcvt
#define gcvt _gcvt
#endif

#include <setlocale.h>
#include <locale.h>

extern int _nl_failid;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_gcvt
#pragma _HP_SECONDARY_DEF _nl_gcvt nl_gcvt
#define nl_gcvt _nl_gcvt
#endif

char *nl_gcvt(value, ndigit, buf, langid)
double value;
int ndigit;
char *buf;
int langid;
{
	char *gcvt();
	char *idtolang();

	if (langid != __nl_langid[LC_NUMERIC] && langid != _nl_failid)
		langinit(idtolang(langid));

	return(gcvt(value, ndigit, buf));
}	
