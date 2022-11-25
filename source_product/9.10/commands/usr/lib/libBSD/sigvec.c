/* HPUX_ID: @(#) $Revision: 56.2 $  */

/* sigvec() - 4.2BSD compatible signal facility [see BSDPROC(OS)] */

#include <signal.h>


static int _42sgh0();

static int (*_42sghdlr[NSIG])();     /* all initialized to NULL pointers */

/*
 * Because HP-UX's SIGCLD has the System V side-effects when set to SIG_IGN
 * but BSD's SIGCHLD does not, we map SIG_IGN to SIG_DFL for this signal
 * when setting it, and on subsequent calls map the kernel's returned value
 * (if SIG_DFL) back to what the user actually passed us.  This variable
 * stores the last real handler (SIG_DFL or SIG_IGN) that we passed to
 * the kernel as SIG_DFL.  This value can be overwritten in a vforked child,
 * but only when the application is switching between SIG_DFL and SIG_IGN
 * (which is pointless in BSD due to identical semantics).
 */
static int (*old_sigcld_handler)() = SIG_DFL;

int sigvec(sig, vec, ovec)
int sig;
register struct sigvec *vec, *ovec;
{
    struct sigvec nvec, *nvecp;
    long omask, sigblock(), sigsetmask();
    int sigvector(), rval;
    int (*new_handler)();

	if (vec != 0)
	{
	    nvec = *vec;	/* use copy of vec; don't ruin user's */
	    nvecp = &nvec;

	    /* save since *vec could be overwritten if vec == ovec */
	    new_handler = vec->sv_handler;
		
	    /*
	     * Only honor the least significant bit of sv_flags/sv_onstack
	     * which is the only bit supported in 4.2.  Don't risk
	     * interpreting other bits, which might be meaningful in HP-UX
	     * or in 4.3 and later BSD releases.  Perhaps we should even
	     * return an error if other bits are set, but we'll just
	     * ignore them.
	     */
	    if ((vec->sv_flags & 1) != 0)
		nvec.sv_flags = SV_ONSTACK;
	    else
		nvec.sv_flags = 0;

	    if (sig == SIGCLD)
	    {
#ifdef SV_BSDSIG
		nvec.sv_flags |= SV_BSDSIG;
#endif
		if (nvec.sv_handler == SIG_IGN)
			nvec.sv_handler = SIG_DFL;
	    }

	    if (vec->sv_handler != SIG_DFL && vec->sv_handler != SIG_IGN)
		nvec.sv_handler = _42sgh0;
	}
	else
	{
	    nvecp = 0;
	}

omask = sigsetmask(~0L);

	if ((rval = sigvector(sig, nvecp, ovec)) != -1)
	{

	    if (ovec != 0)
	    {
		/* only pass back flags defined by 4.2 */
		if (ovec->sv_flags & SV_ONSTACK)
		    ovec->sv_flags = 1;
		else
		    ovec->sv_flags = 0;

		if (ovec->sv_handler == _42sgh0)
		    ovec->sv_handler = _42sghdlr[sig];
		else if (sig == SIGCLD && ovec->sv_handler == SIG_DFL)
		    /* note - value is SIG_DFL if no previous call */
		    ovec->sv_handler = old_sigcld_handler;
	    }

	    if (vec != 0)
	    {
		/*
		 * Only store the handler here if it is a true function
		 * pointer (not SIG_DFL or SIG_IGN).  This is because
		 * our table can be written by a vfork'd child's call
		 * to sigvec().  Such calls make sense when setting up
		 * SIG_DFL or SIG_IGN, but not true function pointers.
		 * Note that we only look at local variables here, since
		 * *vec could be overwritten if vec == ovec.
		 */
		if (new_handler != SIG_DFL && new_handler != SIG_IGN)
		    _42sghdlr[sig] = new_handler;
		if (sig == SIGCLD && nvec.sv_handler == SIG_DFL)
		    old_sigcld_handler = new_handler;
	    }
	}

(void) sigsetmask(omask);

	return rval;
}


static int _42sgh0(sig, code, scp)
int sig;
int code;
struct sigcontext *scp;
{
    int returnval;
    register int (*func)() = _42sghdlr[sig];
    int osc_syscall = scp->sc_syscall;

	/*
	 * This check should be overly paranoid - note that we never
	 * store SIG_DFL or SIG_IGN in the table.
	 */
	if (func != 0)
	    returnval = (*func)(sig, code, scp);

	switch (osc_syscall)
	{
	    case SYS_READ:
	    case SYS_WRITE:
#ifdef SYS_READV
	    case SYS_READV:
#endif
#ifdef SYS_WRITEV
	    case SYS_WRITEV:
#endif
#ifdef SYS_WAIT
	    case SYS_WAIT:
#endif
	    case SYS_SEMOP:
	    case SYS_MSGSND:
	    case SYS_MSGRCV:
		scp->sc_syscall_action = SIG_RESTART;
		break;

	    default:
		scp->sc_syscall_action = SIG_RETURN;
		break;
	}

	return returnval;
}
