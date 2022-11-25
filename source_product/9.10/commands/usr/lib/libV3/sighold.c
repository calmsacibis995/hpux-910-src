/* @(#) $Revision: 63.2 $ */      

/*
 * Emulation of the SVID 3 reliable signal routine sighold().
 */

#include <signal.h>
#include <errno.h>

int
sighold (sig)
int sig;
{
	struct  sigaction  osv;
	sigset_t mask;

	if (sig == SIGKILL || sig == SIGSTOP) {
	    errno = EINVAL;
	    return(-1);
	}

	/* let the kernel error-check the rest of the signals for us */

	if (sigaction(sig, (struct sigaction *)0, &osv) != 0)
		return (-1);

	(void) sigemptyset(&mask);
	(void) sigaddset(&mask,sig);
	(void) sigprocmask(SIG_BLOCK,&mask,(sigset_t *)0);
	return (0);
}

