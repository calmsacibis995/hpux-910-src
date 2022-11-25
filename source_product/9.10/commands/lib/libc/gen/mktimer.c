
/* @(#) $Revision: 70.3 $ */

/* 
  mktimer -- allocate a per-process timer
  rmtimer -- free a per-process timer
  gettimer -- get value of a per-process timer
  reltimer -- relatively arm a per-process timer

  Current implementation ONLY support:
  1. clock type: TIMEOFDAY and  notify type: DELIVERY_SIGNALS
     which is dependent on getitimer and setitimer
*/
#ifdef AES

/*  Lines added to clean up namespace */
#ifdef _NAMESPACE_CLEAN
#   define getitimer _getitimer
#   define getpid _getpid
#   define setitimer _setitimer
#   define sigblock __sigblock
#   define sigsetmask __sigsetmask
#   define mktimer _mktimer
#   define rmtimer _rmtimer
#   define gettimer _gettimer
#   define reltimer _reltimer
#endif /* _NAMESPACE_CLEAN */

#include <sys/timers.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

static long block();                  /* (un)block interrupts */
static void unblock();

#define _THOUSAND_MILLION 1000000000

typedef struct itimerval ITIMERVAL;

typedef struct timertype {           
	int clockType;
	int notifyType;
	pid_t pid;
} TIMERTYPE;

						/* a list of all supported  */
static TIMERTYPE tlist[]= {			/*   clock and notify types */
	{TIMEOFDAY, DELIVERY_SIGNALS, (pid_t) -1}
};

#define _MAX_NTIMERS sizeof(tlist)/sizeof(TIMERTYPE)      /* maximum timers */


/*
  put -- put in a timertype, tt, in a global list, tlist
	 if a timertype has been used return an unique id else return -1
*/
static timer_t put(tt)
TIMERTYPE *tt;
{
	pid_t pid=getpid();
	int i;
	timer_t retval=(timer_t) -1;

	for (i=0; i < _MAX_NTIMERS; i++)
		if (tlist[i].clockType == tt->clockType &&
		    tlist[i].notifyType == tt->notifyType &&
		    tlist[i].pid != pid)
		{
			tlist[i].pid=pid;
			retval=(timer_t) i;
			break;
		}
	return retval;
}

/*
  find -- lookup an id in the global list, tlist
	  if id is being used return a pointer to that timertype;
	  otherwise, return NULL
*/
static TIMERTYPE *find(id)
timer_t id;
{
	return((id >= 0 && id < _MAX_NTIMERS && 
		tlist[id].pid == getpid())?
		(TIMERTYPE *) &tlist[id]: (TIMERTYPE *)NULL);
}

/*
  delete -- mark id as being used in the global list, tlist
*/
static int delete(id)
timer_t id;   
{
	pid_t pid=getpid();
        int retval=-1;

	if (id >= 0 && id < _MAX_NTIMERS && tlist[id].pid == pid) {
		tlist[id].pid=-1;
		retval=0;
	}
	return retval;
}

/*
  block -- disable all signals delivery
*/
static long block()
{
	return sigblock(~0L &
                        ~(sigmask(SIGFPE)) &
			~(sigmask(SIGILL))&
			~(sigmask(SIGSEGV)) &
			~(sigmask(SIGBUS)));
}

/*
  unblock -- enable old signals delivery
*/
static void unblock(mask)
long mask;
{
	(void) sigsetmask (mask);
}

#ifdef _NAMESPACE_CLEAN
#undef mktimer
#pragma _HP_SECONDARY_DEF _mktimer mktimer
#define mktimer _mktimer
#endif /* _NAMESPACE_CLEAN */
timer_t mktimer(clockType, notifyType, itimercbp)
int clockType, notifyType;
void *itimercbp;
{
	long mask;
	timer_t retval=(timer_t) -2;

	mask=block();
	switch(clockType) {
	case TIMEOFDAY:
		switch(notifyType) {
		case DELIVERY_SIGNALS:
			{
			TIMERTYPE t;
			t.clockType=clockType;
			t.notifyType=notifyType;
			retval=put(&t);
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	if (retval == (timer_t) -1) 
		errno=EAGAIN;
	else if (retval == (timer_t) -2) {
		errno=EINVAL;
		retval=-1;	
	}
	unblock(mask);
	return retval;
}

#ifdef _NAMESPACE_CLEAN
#undef rmtimer
#pragma _HP_SECONDARY_DEF _rmtimer rmtimer
#define rmtimer _rmtimer
#endif /* _NAMESPACE_CLEAN */
int rmtimer(id)
timer_t id;
{
	long mask;
	int retval=-1;
	
	mask=block();
	if (find(id) != (TIMERTYPE *) NULL) {
		struct itimerspec value, ovalue;
		value.it_value.tv_sec=value.it_value.tv_nsec=0;
		reltimer(id, &value, &ovalue);
		delete(id);
		retval=0;
	}
	else
		errno=EINVAL;
	unblock(mask);
	return retval;
}

#ifdef _NAMESPACE_CLEAN
#undef gettimer
#pragma _HP_SECONDARY_DEF _gettimer gettimer
#define gettimer _gettimer
#endif /* _NAMESPACE_CLEAN */
int gettimer(timerid, value)
timer_t timerid;
struct itimerspec *value;
{
   long mask;
   TIMERTYPE *tt;
   int retval=-1;

   mask=block();
   if ((tt=find(timerid)) != NULL) {
	switch(tt->clockType) {
   	case TIMEOFDAY:
		switch(tt->notifyType) {
		case DELIVERY_SIGNALS:
			{
   			ITIMERVAL tv;

  	 		if (getitimer(ITIMER_REAL, &tv) != -1) {
   				value->it_interval.tv_sec=tv.it_interval.tv_sec;
				value->it_interval.tv_nsec=tv.it_interval.tv_usec *1000;
   				value->it_value.tv_sec=tv.it_value.tv_sec;
   				value->it_value.tv_nsec=tv.it_value.tv_usec *1000;
				retval=0;
			}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
   	}
   }
   if (retval == -1)
	errno=EINVAL;
   unblock(mask);
   return retval;
}

#ifdef _NAMESPACE_CLEAN
#undef reltimer
#pragma _HP_SECONDARY_DEF _reltimer reltimer
#define reltimer _reltimer
#endif /* _NAMESPACE_CLEAN */
int reltimer(timerid, value, ovalue)
timer_t timerid;
struct itimerspec *value;
struct itimerspec *ovalue;
{
   long mask;
   TIMERTYPE *tt;
   int retval=-1;

   mask=block();
   if ((tt=find(timerid)) != NULL &&
       value->it_value.tv_nsec >= 0 &&
       value->it_interval.tv_nsec >= 0 &&
       value->it_value.tv_nsec < _THOUSAND_MILLION &&
       value->it_interval.tv_nsec < _THOUSAND_MILLION) {
	switch(tt->clockType) {
   	case TIMEOFDAY:
		switch(tt->notifyType) {
		case DELIVERY_SIGNALS:
			{
			ITIMERVAL tv, oldtv;

			tv.it_interval.tv_sec=value->it_interval.tv_sec;
	 	  	tv.it_interval.tv_usec=value->it_interval.tv_nsec / 1000;
	   		tv.it_value.tv_sec=value->it_value.tv_sec;
	  	 	tv.it_value.tv_usec=value->it_value.tv_nsec / 1000;
	   		if (setitimer(ITIMER_REAL, &tv, &oldtv) != -1) {
				if (ovalue != NULL) {
		   			ovalue->it_interval.tv_sec=oldtv.it_interval.tv_sec;
		   			ovalue->it_interval.tv_nsec=oldtv.it_interval.tv_usec * 1000;
		  		 	ovalue->it_value.tv_sec=oldtv.it_value.tv_sec;
		   			ovalue->it_value.tv_nsec=oldtv.it_value.tv_usec * 1000;   
				}
				retval=0;
			}
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
   	}
   }
   if (retval == -1)
	errno=EINVAL;
   unblock(mask);
   return retval;
}

#endif /* AES */
