/* @(#) $Revision: 66.2 $ */   

#ifdef _NAMESPACE_CLEAN
#define sleep _sleep
#define sigblock __sigblock
#define getitimer _getitimer
#define sigvector __sigvector
#define setitimer _setitimer
#define sigpause __sigpause
#define kill _kill
#define sigsetmask __sigsetmask
#define getpid _getpid
#endif

#include <time.h>
#include <signal.h>

#define	mask(s)	(1L<<((s)-1))
#define	setvec(vec, a) \
	vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0
#define	ONE_MILLION	1000000		/* microseconds in a second */
#ifdef hp9000s500
#define OLD_BELL_SIG    -1
#endif

static int caught_sig;

#ifdef _NAMESPACE_CLEAN
#undef sleep
#pragma _HP_SECONDARY_DEF _sleep sleep
#define sleep _sleep
#endif

unsigned
sleep(n)
	unsigned n;
{
	void sleepx();
	int alrm_flg;
	long omask;
	struct itimerval sleep_tm, alrm_tm, unslept;
	struct timeval left_ovr;
	struct sigvec vec, ovec;

	/* When we enter sleep() there are 3 possible scenarios:  */
	/*      1) ITIMER_REAL is not set.                        */
	/*      2) ITIMER_REAL is set to go off at a time >  n.   */
	/*      3) ITIMER_REAL is set to go off at a time <= n.   */
	/* In cases 1 & 2 we reset ITIMER_REAL to go off in n     */
	/* seconds. Then in case 2 we restore the previous timer  */
	/* after subtracting n from the time. In case 3 we just   */
	/* install our own handler and let the already scheduled  */
	/* timer go off. We then reinstall the original handler   */
	/* and send ourselves another SIGALRM.                    */

	if (n == 0)
		return(0);

	/* Initialize */

	alrm_flg = caught_sig = 0;
	timerclear(&left_ovr);
	timerclear(&sleep_tm.it_interval);
	timerclear(&sleep_tm.it_value);
	sleep_tm.it_value.tv_sec = n;        /* set sleep_tm to n */

	/* Block all signals so we know we can't be interrupted   */

	omask = sigblock(~0L);
	/* Install our own handler */

	setvec(vec, sleepx);
	(void) sigvector(SIGALRM, &vec, &ovec);

	(void) getitimer(ITIMER_REAL, &alrm_tm);
	if (timerisset(&alrm_tm.it_value)) {
		if (timercmp(&alrm_tm.it_value, &sleep_tm.it_value, >)) {

			/* Readjust alrm_tm so that when we reinstall */
			/* the handler we have corrected for sleep_tm */

			alrm_tm.it_value.tv_sec -= sleep_tm.it_value.tv_sec;
			++alrm_flg;
		} else {

			/* Initialize left_ovr to be sleep_tm - alrm_tm */

			left_ovr = sleep_tm.it_value;
			sub_timeval(&left_ovr, &alrm_tm.it_value);
			--alrm_flg;
		}
	}

	if (alrm_flg >= 0)
		(void) setitimer(ITIMER_REAL, &sleep_tm, (struct itimerval *)0);

	/* Wait for a signal (that wasn't being blocked before sleep() was */
	/* entered. We explicitly unblock SIGALRM since the user may have  */
	/* been blocking it before calling sleep().                        */

	(void) sigpause(omask &~ mask(SIGALRM));

	if (alrm_flg >= 0) {

		/* First, shut off the timer, just in case the sigpause()  */
		/* terminated due to a signal other than SIGALRM.          */

		timerclear(&sleep_tm.it_interval);
		timerclear(&sleep_tm.it_value);
		(void) setitimer (ITIMER_REAL, &sleep_tm, &unslept);

		/* After we stop the timer we need to set the function for */
		/* SIGALRM to be SIG_IGN before reinstalling the old       */
		/* handler so that any pending alarm signals will be thrown*/
		/* away. This is necessary in the scenario where the       */
		/* above sigpause is terminated due to a different signal  */
		/* other than SIGALRM and then a SIGALRM is delivered      */
		/* before we shut off the timer with setitimer().          */
		/* If this was not done, a SIGALRM would be received when  */
		/* we unblock all signals just before exiting, possibly    */
		/* causing unexpected results (possibly termination).      */

		setvec(vec, SIG_IGN);
		(void) sigvector(SIGALRM, &vec, (struct sigvec *)0);
		if (alrm_flg > 0) {

			/* readjust alrm_tm if we were interrupted by a    */
			/* signal other than SIGALRM.                      */

			add_timeval (&alrm_tm.it_value, &unslept.it_value);
			(void) setitimer (ITIMER_REAL, &alrm_tm, (struct itimerval *)0);
		}
	}
	else
		(void) getitimer(ITIMER_REAL, &unslept);

	/* Reinstall the original handler for SIGALRM */
#ifdef hp9000s500
	/* Determine whether signal(2) or sigvector(2) was used to set    */
	/* up the original SIGALRM handler and reinstall it accordingly.  */
	/* Since implementations other than series 500 use a signal       */
	/* emulation on top of sigvector, we don't need to do this check  */
	/* and can unconditionally restore the handler with sigvector().  */

	if ( ovec.sv_mask == OLD_BELL_SIG )
	    (void)signal(SIGALRM, ovec.sv_handler);
	else
#endif hp9000s500
	    (void) sigvector(SIGALRM, &ovec, (struct sigvec *)0);

	add_timeval (&left_ovr, &unslept.it_value);

	/* If we didn't schedule our own timer, (i.e. we used an already  */
	/* scheduled one) then we need to send ourselves a SIGALRM so the */
	/* original handler will be called.                               */

	if (alrm_flg < 0 && caught_sig)
		kill(getpid(), SIGALRM);

	/* Restore the original signal mask */

	(void) sigsetmask (omask);

	/* Return the left over seconds, rounding up even if we are only */
	/* 1 tic over.                                                   */

	return (left_ovr.tv_usec == 0 ? left_ovr.tv_sec : left_ovr.tv_sec + 1);
}

static void
sleepx()
{
	++caught_sig;
}

static
add_timeval (dest, source)
register struct timeval *dest, *source;
{
	dest->tv_sec += source->tv_sec;
	dest->tv_usec += source->tv_usec;
	if (dest->tv_usec >= ONE_MILLION) {
		++dest->tv_sec;
		dest->tv_usec -= ONE_MILLION;
	}
}

static
sub_timeval (dest, source)
register struct timeval *dest, *source;
{
	dest->tv_sec -= source->tv_sec;
	dest->tv_usec -= source->tv_usec;
	if (dest->tv_usec < 0) {
		--dest->tv_sec;
		dest->tv_usec += ONE_MILLION;
	}
}
