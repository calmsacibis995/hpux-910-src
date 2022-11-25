/* @(#) $Revision: 63.2 $ */      

/*
 * Emulation of the SVID 3 reliable signal routine sigset().
 */

#include <signal.h>
#include <errno.h>

void (*
sigset (sig, handler))()
int	sig;
void    (*handler)();
{
	sigset_t  old_mask;
	sigset_t  tmp_mask;
	void    (*old_handler)();
	struct  sigaction  sv, osv;

	if (sig == SIGKILL || sig == SIGSTOP) {
	    errno = EINVAL;
	    return(SIG_ERR);
	}

	(void) sigemptyset(&old_mask);
	(void) sigemptyset(&tmp_mask);
	(void) sigaddset(&tmp_mask,sig);
	if (handler == SIG_HOLD) {
		if (sigaction(sig, (struct sigvec *)0, &osv) != 0)
			return (SIG_ERR);

		(void) sigprocmask(SIG_BLOCK,&tmp_mask,&old_mask);
		if (sigismember(&old_mask,sig))
			return (SIG_HOLD);
		return (osv.sa_handler);
	}

	(void) sigfillset(&tmp_mask);
	(void) sigprocmask(SIG_BLOCK,&tmp_mask,&old_mask);

	sv.sa_handler = handler;
	(void) sigemptyset(&sv.sa_mask);
	if (sig == SIGCLD)
	    sv.sa_flags = SA_NOCLDSTOP;
	else
	    sv.sa_flags = 0;

	if (sigaction(sig, &sv, &osv) != 0) {
		(void) sigprocmask (SIG_SETMASK,&old_mask,(sigset_t *)0);
		return (SIG_ERR);
	}

	if (sigismember(&old_mask,sig))
		old_handler = SIG_HOLD;
	else
		old_handler = osv.sa_handler;

	(void) sigdelset(&old_mask, sig);
	(void) sigprocmask(SIG_SETMASK,&old_mask,(sigset_t *)0);

	return(old_handler);
}
