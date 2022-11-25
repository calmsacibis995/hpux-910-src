/* $Header: timers.h,v 70.1 92/03/26 11:53:42 ssa Exp $ */

#ifndef _SYS_TIMERS_INCLUDED
#define _SYS_TIMERS_INCLUDED

#ifdef _KERNEL
#include "../h/stdsyms.h"
#else
#include <sys/stdsyms.h>
#endif

#ifdef _INCLUDE_AES_SOURCE

#define DELIVERY_SIGNALS	1	/* notify process of timer          */
					/* expiration via SIGALRM           */

#define TIMEOFDAY		1	/* identifier for timer of the day  */
					/* clock type                       */

typedef long timer_t;			/* used as a handle for per-process */
					/* timers allocated by mktimer      */

struct timespec {			/* specifies a single time value    */
	unsigned long tv_sec;		/*    seconds                       */
	long tv_nsec;			/*    nanoseconds                   */
};

					/* specifies an initial timer value */
					/* and a repetition for use by the  */
struct itimerspec {			/* per-process timer functions      */
	struct timespec it_interval;	/*    timer period                  */
	struct timespec it_value;	/*    timer expiration              */
};

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif

#  ifdef _PROTOTYPES
	extern int getclock(int, struct timespec *);
	extern int gettimer(timer_t, struct itimerspec *);
	extern timer_t mktimer(int, int, void *);
	extern int reltimer(timer_t, struct itimerspec *, struct itimerspec *);
	extern int rmtimer(timer_t);
	extern int setclock(int, struct timespec *);
#  else /* not _PROTOTYPES */
	extern int getclock();
	extern int gettimer();
	extern timer_t mktimer();
	extern int reltimer();
	extern int rmtimer();
	extern int setclock();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif
#  endif /* _KERNEL */

#  endif /* _INCLUDE_AES_SOURCE */

#endif /* _SYS_TIMERS_INCLUDED */
