/* @(#) $Revision: 66.2 $ */     
/*LINTLIBRARY*/
/*
 *	ssignal, gsignal: software signals
 */

#ifdef _NAMESPACE_CLEAN
#define ssignal _ssignal
#define gsignal _gsignal
#endif

#include <signal.h>

/* Highest allowable user signal number */
#define MAXSIG 16

/* Lowest allowable signal number (lowest user number is always 1) */
#define MINSIG (-4)

/* Table of signal values */
static int (*sigs[MAXSIG-MINSIG+1])();

#ifdef _NAMESPACE_CLEAN
#undef ssignal
#pragma _HP_SECONDARY_DEF _ssignal ssignal
#define ssignal _ssignal
#endif

int
(*ssignal(sig, fn))()
register int sig, (*fn)();
{
	register int (*savefn)();

	if(sig >= MINSIG && sig <= MAXSIG) {
		savefn = sigs[sig-MINSIG];
		sigs[sig-MINSIG] = fn;
	} else
		savefn = SIG_DFL;

	return(savefn);
}

#ifdef _NAMESPACE_CLEAN
#undef gsignal
#pragma _HP_SECONDARY_DEF _gsignal gsignal
#define gsignal _gsignal
#endif

int
gsignal(sig)
register int sig;
{
	register int (*sigfn)();

	if(sig < MINSIG || sig > MAXSIG ||
				(sigfn = sigs[sig-MINSIG]) == SIG_DFL)
		return(0);
	else if(sigfn == SIG_IGN)
		return(1);
	else {
		sigs[sig-MINSIG] = SIG_DFL;
		return((*sigfn)(sig));
	}
}
