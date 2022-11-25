/* @(#) $Revision: 64.12 $ */      
/*LINTLIBRARY*/
/*
 *	langinfo()
 */

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define langinfo _langinfo
#define idtolang _idtolang
#define langinit _langinit
#define nl_langinfo _nl_langinfo
#define _nl_failid __nl_failid
#endif

#include	<nl_types.h>

extern char	*nl_langinfo();
extern int	langinit();
extern char	*idtolang();
extern int	_nl_failid;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef langinfo
#pragma _HP_SECONDARY_DEF _langinfo langinfo
#define langinfo _langinfo
#endif

char *langinfo(langid, item)
int langid;
nl_item item;
{
	if (langid != _nl_failid)
		(void) langinit(idtolang(langid));

	return nl_langinfo(item);
}
