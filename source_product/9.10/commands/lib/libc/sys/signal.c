/* @(#) $Revision: 64.4 $ */   
/*	signal.c	*/

/*
 * Emulation of Bell signal(2) on top of HP-UX sigvector(2)
 */
#ifdef _NAMESPACE_CLEAN
#define signal _signal
#define sigvector __sigvector
#endif

#include <signal.h>
#include <errno.h>

#ifdef _NAMESPACE_CLEAN
#undef signal
#pragma _HP_SECONDARY_DEF _signal signal
#define signal _signal
#endif

void (*
signal(sig, action))()
	register int sig;
	register void (*action)();
{
	struct	sigvec	vec, ovec;

	/* Sigvector() allows SIGKILL and SIGSTOP if the action    */
	/* is SIG_DFL. Sigvector also allows SIGCONT to be ignored */

	if (sig == SIGKILL || sig == SIGSTOP) {
	    errno = EINVAL;
	    return(BADSIG);
	}

	vec.sv_mask = 0L;
	vec.sv_flags = SV_RESETHAND;
	vec.sv_handler = action;

	if (sigvector (sig, &vec, &ovec) == 0)
		return (ovec.sv_handler);
	else
		return (BADSIG);
}
