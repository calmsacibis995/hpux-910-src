/* @(#) $Revision: 70.3 $ */

/* 
  getclock - get current value of system-value clock
  setclock - set value of system-value clock

  We currently ONLY support clock type: TIMEOFDAY
*/

#ifdef AES

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#   define gettimeofday _gettimeofday
#   define settimeofday _settimeofday
#   define getclock _getclock
#   define setclock _setclock
#endif /* _NAMESPACE_CLEAN */

#include <sys/timers.h>
#include <time.h>
#include <errno.h>

static int GetTimeOfDay();
static int SetTimeOfDay();

#ifdef _NAMESPACE_CLEAN
#undef getclock
#pragma _HP_SECONDARY_DEF _getclock getclock
#define getclock _getclock
#endif /* _NAMESPACE_CLEAN */
int getclock(clock_type, tp)
int clock_type;
struct timespec *tp;
{
	switch(clock_type) {
	case TIMEOFDAY:
		return(GetTimeOfDay(tp));
	default:
		errno=EINVAL;
		return -1;
	}
}

#ifdef _NAMESPACE_CLEAN
#undef setclock
#pragma _HP_SECONDARY_DEF _setclock setclock
#define setclock _setclock
#endif /* _NAMESPACE_CLEAN */
int setclock(clock_type, tp)
int clock_type;
struct timespec *tp;
{
	switch(clock_type) {
	case TIMEOFDAY:
		return(SetTimeOfDay(tp));
	default:
		errno=EINVAL;
		return -1;
	}
}

static int GetTimeOfDay(tp)
struct timespec *tp;
{
	struct timeval value;
	struct timezone zone;

	zone.tz_minuteswest = zone.tz_dsttime = 0;
	if (gettimeofday(&value, &zone) == -1) {
		errno=EIO;
		return -1;
	}
	tp->tv_sec = value.tv_sec;
	tp->tv_nsec = value.tv_usec * 1000;
	return 0;
}

static int SetTimeOfDay(tp)
struct timespec *tp;
{
	struct timeval value;
	struct timezone zone;

	if (tp->tv_nsec < 0 || tp->tv_nsec >= 1000000000) {
		errno=EINVAL;
		return -1;
	}
	zone.tz_minuteswest = zone.tz_dsttime = 0;
	value.tv_sec = tp->tv_sec;
	value.tv_usec = tp->tv_nsec / 1000;
	if (settimeofday(&value, &zone) == -1) {
		if (errno != EPERM)
			errno=EIO;
		return -1;
	}
	return 0;
}

#endif /* AES */
