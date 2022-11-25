/* @(#) $Revision: 64.1 $ */     
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _labs labs
#define labs _labs
#endif

long int
labs(arg)
long int arg;
{
	return (arg >= 0 ? arg : -arg);
}
