/* @(#) $Revision: 66.1 $ */   
/*
 * C shell - New routines using HPUX reliable signals.
 *
 *           old jobs library sigrelse meant unblock mask
 *	     AND reinstall handler, so we simulate it here.
 */
#include <signal.h>
#ifdef SIGTSTP
#define sigvector sigbsd
#endif

#ifdef DEBUG_SIGNAL
extern int child;
#endif

static	void (*actions[NSIG])();
static  int (*saveactions[NSIG])();
static	int achanged[NSIG];
static  int saveachanged[NSIG];
static  long savesigmask;

/*
 * Perform action and save handler state.
 */
/**********************************************************************/
void (*sigset (signo, f))()
register signo;
void (*f)();
/**********************************************************************/
{
	struct sigvec vec, ovec;

#ifdef DEBUG_SIGNAL
  if (signo == SIGINT) 
    {
      if (f == SIG_DFL)
        printf ("sigset (1): %d, setting SIGINT to SIG_DFL\n", getpid ());
      
      else if (f == SIG_IGN)
        printf ("sigset (1): %d, setting SIG_IGN for SIGINT\n", getpid ());

      else
        printf ("sigset (1): %d, setting SIGINT to %lo\n", getpid (), f);
    }
#endif

	actions[signo] = vec.sv_handler = f;
	achanged[signo] = 0;
	vec.sv_mask = vec.sv_onstack = 0;
#if defined(hpux) && defined(SIGTSTP)
	if (signo == SIGCLD)
		vec.sv_flags |= SV_BSDSIG;
#endif
	sigvector(signo, &vec, &ovec);
	return(ovec.sv_handler);
}

/* Same as sigset, except don't change actions and achanged arrays. This */
/* should only be called by a vforked child.                             */

/**********************************************************************/
void (*sigsys (signo, f))()
register signo;
void (*f)();
/**********************************************************************/
{
	struct sigvec vec, ovec;

	vec.sv_handler = f;
	vec.sv_mask = vec.sv_onstack = 0;
#if defined(hpux) && defined(SIGTSTP)
	if (signo == SIGCLD)
		vec.sv_flags |= SV_BSDSIG;
#endif
	sigvector(signo, &vec, &ovec);
	return(ovec.sv_handler);
}

/*
 * Ignore signal but maintain state so sigrelse
 * will restore handler.  We avoid the overhead
 * of doing a signal for each sigrelse call by
 * marking the signal's action as changed.
 */
/**********************************************************************/
void (*sigignore (signo))()
int signo;
/**********************************************************************/
{
	struct sigvec vec, ovec;

	if (actions[signo] != SIG_IGN)
		achanged[signo] = 1;
	vec.sv_handler = SIG_IGN;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvector(signo, &vec, &ovec);
	return(ovec.sv_handler);
}

/**********************************************************************/
sighold (signo)
int signo;
/**********************************************************************/
{
        sigblock(1L << (signo - 1));
}

/*
 * Release any masking of signal and
 * reinstall handler in case someone's
 * done a sigignore.
 */
/**********************************************************************/
sigrelse (signo)
int signo;
/**********************************************************************/
{
	int	mask;

#ifdef DEBUG_SIGNAL
  if (signo == SIGINT)
    {
      if (actions [signo] == SIG_DFL)
        printf ("sigrelse (1): %d, releasing SIG_DFL for SIGINT\n", getpid ());

      else if (actions [signo] == SIG_IGN)
        printf ("sigrelse (1): %d, releasing SIG_IGN for SIGINT\n", getpid ());

      else
        printf ("sigset (1): %d, releasing SIGINT to %lo\n", getpid (), 
		actions [signo]);
    }
  printf ("\tachanged: %d\n", achanged [signo]);

#endif

	if (achanged[signo]) {
		sigset(signo, actions[signo]);
		achanged[signo] = 0;
	}

	mask = sigblock(0);
	mask = mask & (~(1L << (signo - 1)));
	sigsetmask(mask);
}

#ifdef SIGTSTP
#undef sigvector
/**********************************************************************/
sigbsd(sig, vec, ovec)
int sig;
struct sigvec *vec, *ovec;
/**********************************************************************/
{
	if (sig == SIGCLD)
		vec->sv_flags |= SV_BSDSIG;
	return(sigvector(sig, vec, ovec));
}
#endif
