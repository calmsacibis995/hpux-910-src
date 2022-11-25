/* @(#) $Revision: 64.3 $ */     
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#pragma _HP_SECONDARY_DEF _abs abs
#define abs _abs
#endif

int
abs(arg)
register int arg;
{
	return (arg >= 0 ? arg : -arg);
}
