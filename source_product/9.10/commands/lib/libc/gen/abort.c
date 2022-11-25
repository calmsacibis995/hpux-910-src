/* @(#) $Revision: 66.1 $ */   
/*LINTLIBRARY*/

/* Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#define abort _abort
#define raise _raise
#define signal _signal
#define sigemptyset _sigemptyset
#define sigaction _sigaction
#define sigaddset _sigaddset
#define sigfillset _sigfillset
#define sigismember _sigismember
#define sigprocmask _sigprocmask
#define exit ___exit
#endif

#include <signal.h>
#include <stdlib.h>

static pass = 0;   /* counts how many times abort has tried to call _cleanup */

/* Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef abort
#pragma _HP_SECONDARY_DEF _abort abort
#define abort _abort
#endif
void
abort()
{
	struct sigaction act;
	sigset_t set;


	/*
	 * If SIGABRT is being caught and is unblocked, give the user's
	 * handler a chance before closing files).
	 */
	sigemptyset(&set);
	sigprocmask(SIG_UNBLOCK, (sigset_t *)0, &set);
	if (!sigismember(&set, SIGABRT)) {
		/* SIGABRT is not blocked - is it being caught? */
		sigemptyset(&act.sa_mask);
		if (sigaction(SIGABRT, (struct sigaction *)0, &act) == 0  &&
		    act.sa_handler != SIG_DFL && act.sa_handler != SIG_IGN) {
			raise(SIGABRT);
		}
	}

	/*
	 * At this point either the user was not chatching SIGABRT, or the
	 * handler has returned - it's now time to close files and terminate
	 * the process.
	 */

	/* increment first to avoid any hassle with interupts */
	if (++pass == 1)
		_cleanup();

	/*
	 * Now block all signals to ensure atomicity.  The only thing this
	 * prevents is some signal handler coming in and undoing one of the
	 * steps below - which is pretty marginal value.  This is done after
	 * closing files, because there's a slight chance of hanging during
	 * a close of a device, and we don't want to block an interactive
	 * SIGINT or SIGQUIT should that happen.
	 */
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, (sigset_t *)0);

	/* Make sure SIGABRT is fatal */
	act.sa_handler = SIG_DFL;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGABRT, &act, (struct sigaction *)0);
	sigemptyset(&set);
	sigaddset(&set, SIGABRT);
	sigprocmask(SIG_UNBLOCK, &set, (sigset_t *)0);

	/* Send the abort signal */
	raise(SIGABRT);

	/* insure no return to caller */
	/* This should be unnecessary due to blocking of signals above */
	exit(EXIT_FAILURE);
}
