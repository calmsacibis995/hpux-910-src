/* @(#) $Revision: 64.3 $ */     
/*LINTLIBRARY*/

#ifdef _NAMESPACE_CLEAN
#define ctermid _ctermid
#define strcpy  _strcpy
#endif

#include <stdio.h>

extern char *strcpy();
static char res[L_ctermid];

#ifdef _NAMESPACE_CLEAN
#undef ctermid
#pragma _HP_SECONDARY_DEF _ctermid ctermid
#define ctermid _ctermid
#endif /* _NAMESPACE_CLEAN */

char *
ctermid(s)
register char *s;
{
	return (strcpy(s != NULL ? s : res, "/dev/tty"));
}
