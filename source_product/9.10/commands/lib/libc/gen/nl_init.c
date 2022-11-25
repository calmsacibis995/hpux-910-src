/* @(#) $Revision: 64.4 $ */   
/* LINTLIBRARY */
/*
** nl_init() - initialize the NLS environment of a program
*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define nl_init _nl_init
#define langtoid _langtoid
#define setlocale _setlocale
#endif

#include <locale.h>

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef nl_init
#pragma _HP_SECONDARY_DEF _nl_init nl_init
#define nl_init _nl_init
#endif

int nl_init(langname)
char *langname;
{

	char *locale;

	if (langname==0 || *langname=='\0')
		locale=setlocale(LC_ALL,"n-computer");
	else
		locale=setlocale(LC_ALL,langname);

	return((locale == 0) ? -1 : 0);
}

