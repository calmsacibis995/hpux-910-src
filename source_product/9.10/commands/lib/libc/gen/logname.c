/* @(#) $Revision: 64.3 $ */     

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define getenv _getenv
#define logname _logname
#endif

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef logname
#pragma _HP_SECONDARY_DEF _logname logname
#define logname _logname
#endif

char *
logname()
{
	return((char *)getenv("LOGNAME"));
}
