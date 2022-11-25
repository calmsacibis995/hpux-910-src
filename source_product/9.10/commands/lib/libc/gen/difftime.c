/* @(#) $Revision: 64.2 $ */     
/* LINTLIBRARY */
/*
 *	difftime()
 */

#ifdef _NAMESPACE_CLEAN
#define difftime _difftime
#endif /* _NAMESPACE_CLEAN */

#include <time.h>

/*
 * difftime() returns the differences, in seconds, between two time_t values.
 * This is a trivial operation for HP-UX since time_t is a simple numeric type,
 * but for some O/S's time_t might be a structure for which more complicated
 * processing is needed to return the difference.
 */

#ifdef _NAMESPACE_CLEAN
#undef difftime
#pragma _HP_SECONDARY_DEF _difftime difftime
#define difftime _difftime
#endif /* _NAMESPACE_CLEAN */

double difftime(time1, time0)
register time_t time1, time0;
{
	return ((double)time1 - (double)time0);
}
