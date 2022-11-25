/* HPUX_ID: @(#) $Revision: 64.2 $  */
#ifdef _NAMESPACE_CLEAN
#define memset _memset
#define sigaddset _sigaddset
#define sigdelset _sigdelset
#define sigemptyset _sigemptyset
#define sigfillset _sigfillset
#define sigismember _sigismember
#endif

#include <sys/errno.h>
#include <signal.h>

/* This file contains the POSIX routines to manipulate the bits that */
/* are "hidden" by the sigset_t data type.                           */

#define MAXSIGNAL    256   /* Maximum legal signal number */
#define SIGSHIFT       5
#define SIGMASK     0x1f

extern int errno;

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef sigemptyset
#pragma _HP_SECONDARY_DEF _sigemptyset sigemptyset
#define sigemptyset _sigemptyset
#endif

int
sigemptyset(set)
    sigset_t *set;
{
    memset((unsigned char *)set,0,sizeof(sigset_t));
    return(0);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef sigfillset
#pragma _HP_SECONDARY_DEF _sigfillset sigfillset
#define sigfillset _sigfillset
#endif

int
sigfillset(set)
    sigset_t *set;
{
    memset((unsigned char *)set,0xff,sizeof(sigset_t));
    return(0);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef sigaddset
#pragma _HP_SECONDARY_DEF _sigaddset sigaddset
#define sigaddset _sigaddset
#endif

int
sigaddset(set,signo)
    sigset_t *set;
    register int signo;
{
    register int offset;

    if (signo < 1 || signo > MAXSIGNAL) {
	errno = EINVAL;
	return(-1);
    }

    offset = (signo - 1) >> SIGSHIFT;
    set->sigset[offset] |= (1L <<  ((signo - 1) & SIGMASK));
    return(0);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef sigdelset
#pragma _HP_SECONDARY_DEF _sigdelset sigdelset
#define sigdelset _sigdelset
#endif

int
sigdelset(set,signo)
    sigset_t *set;
    register int signo;
{
    register int offset;

    if (signo < 1 || signo > MAXSIGNAL) {
	errno = EINVAL;
	return(-1);
    }

    offset = (signo - 1) >> SIGSHIFT;
    set->sigset[offset] &= ~(1L <<  ((signo - 1) & SIGMASK));
    return(0);
}

/*  Lines added to clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef sigismember
#pragma _HP_SECONDARY_DEF _sigismember sigismember
#define sigismember _sigismember
#endif

int
sigismember(set,signo)
    sigset_t *set;
    register int signo;
{
    register int offset;

    if (signo < 1 || signo > MAXSIGNAL) {
	errno = EINVAL;
	return(-1);
    }

    offset = (signo - 1) >> SIGSHIFT;
    if (set->sigset[offset] & (1L <<  ((signo - 1) & SIGMASK)))
	return(1);

    return(0);
}
