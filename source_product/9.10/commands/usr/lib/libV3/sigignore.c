/* @(#) $Revision: 63.2 $ */      

/*
 * Emulation of the SVID 3 reliable signal routine sigignore().
 */

#include <signal.h>
#include <errno.h>

int
sigignore (sig)
int sig;
{
	struct  sigaction sv;

	if (sig == SIGKILL || sig == SIGSTOP) {
	    errno = EINVAL;
	    return(-1);
	}

	sv.sa_handler = SIG_IGN;
	(void) sigemptyset(&sv.sa_mask);
	sv.sa_flags = 0;
	return (sigaction (sig, &sv, (struct sigaction *)0));
}
