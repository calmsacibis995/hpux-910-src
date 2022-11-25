/* @(#) $Revision: 66.1 $ */    
/*LINTLIBRARY*/

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define clock _clock
#define times _times
#endif

#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>	/* for HZ (clock frequency in Hz) */
#define TIMES(B)	(B.tms_utime+B.tms_stime+B.tms_cutime+B.tms_cstime)

#ifdef _NAMESPACE_CLEAN
#undef clock
#pragma _HP_SECONDARY_DEF _clock clock
#define clock _clock
#endif /* _NAMESPACE_CLEAN */

clock_t
clock()
{
	struct tms buffer;
	static clock_t first = 0L;
	static int first_set = 0;

	if (times(&buffer) != -1L && first_set == 0)
        {
		first = TIMES(buffer);
		first_set = 1;
        }
	return ((TIMES(buffer) - first) * (1000000L/HZ));
}
