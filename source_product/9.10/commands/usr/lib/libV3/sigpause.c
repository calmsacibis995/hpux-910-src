/* @(#) $Revision: 63.2 $ */      

/*
 * Emulation of the 5.3 reliable signal routine sigpause().
 */

#include <signal.h>
#include <errno.h>

int sigpause(sig)
int sig;
{
	sigset_t mask;

	if (sig == SIGKILL || sig == SIGSTOP) {
	    errno = EINVAL;
	    return(-1);
	}

	(void) sigemptyset(&mask);
	(void) sigprocmask(SIG_BLOCK,(sigset_t *)0,&mask);
	(void) sigdelset(&mask, sig);
	return(sigsuspend(&mask));
}


