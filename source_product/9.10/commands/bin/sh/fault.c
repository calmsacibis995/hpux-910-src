/* @(#) $Revision: 70.6 $ */
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */


#include	"defs.h"
#include	<sys/termio.h>

#ifdef NLS
#define NL_SETN 1
#endif

extern int	done();

#ifdef RFA
extern int in_netunam;
#endif /* RFA */

tchar	*trapcom[MAXTRAP];
BOOL	trapflg[MAXTRAP] =
{
	0,
	0,	/* SIGHUP     (1) - hangup */
	0,	/* SIGINT     (2) - interrupt */
	0,	/* SIGQUIT    (3) - quit */
	0,	/* SIGILL     (4) - illegal instr */
	0,	/* SIGTRAP    (5) - trace trap */
	0,	/* SIGIOT     (6) - IOT */
	0,	/* SIGEMT     (7) - EMT */
	0,	/* SIGFPE     (8) - float pt. exp */
	0,	/* SIGKILL    (9) - kill */
	0, 	/* SIGBUS    (10) - bus error */
	0,	/* SIGSEGV   (11) - memory faults */
	0,	/* SIGSYS    (12) - bad sys call */
	0,	/* SIGPIPE   (13) - bad pipe call */
	0,	/* SIGALRM   (14) - alarm */
	0, 	/* SIGTERM   (15) - software termination */
	0,	/* SIGUSR1   (16) - unassigned */
	0,	/* SIGUSR2   (17) - unassigned */
	0,	/* SIGCLD    (18) - death of child */
	0,	/* SIGPWR    (19) - power fail */
	0,	/* SIGVTALRM (20) - virtual time alarm */
	0,	/* SIGPROF   (21) - profiling timer alarm */
	0,	/* SIGIO     (22) - asynch I/O signal */
	0,	/* SIGWINCH  (23) - window size change */
	0,	/* SIGSTOP   (24) - stop */
	0,	/* SIGTSTP   (25) - keyboard stop signal */
	0,	/* SIGCONT   (26) - continue after stop */
	0,	/* SIGTTIN   (27) - background read attempt */
	0,	/* SIGTTOU   (28) - background write attempt */
	0,	/* SIGURG    (29) - urgent data arrival */
	0	/* SIGLOST   (30) - file lock lost */
};

int 	(*(sigval[]))() =
{
	0,
	done,   /* SIGHUP     (1) - hangup */
	fault,  /* SIGINT     (2) - interrupt */
#ifdef DEBUG
	0,	/* SIGQUIT    (3) - quit */
#else
	fault,  /* SIGQUIT    (3) - quit */
#endif
	done,   /* SIGILL     (4) - illegal instr */
	done,   /* SIGTRAP    (5) - trace trap */
	done,   /* SIGIOT     (6) - IOT */
	done,   /* SIGEMT     (7) - EMT */
	done,   /* SIGFPE     (8) - float pt. exp */
	0,	/* SIGKILL    (9) - kill */
#ifdef DEBUG
	0, 	/* SIGBUS    (10) - bus error */
	0,	/* SIGSEGV   (11) - memory faults */
#else
	done,   /* SIGBUS    (10) - bus error */
	done,   /* SIGSEGV   (11) - memory faults */
#endif
	done,   /* SIGSYS    (12) - bad sys call */
	done,   /* SIGPIPE   (13) - bad pipe call */
	fault,  /* SIGALRM   (14) - alarm */
	fault,  /* SIGTERM   (15) - software termination */
	done,   /* SIGUSR1   (16) - unassigned */
	done,   /* SIGUSR2   (17) - unassigned */
	fault,  /* SIGCLD    (18) - death of child */
	done,   /* SIGPWR    (19) - power fail */
	fault,  /* SIGVTALRM (20) - virtual time alarm */
	fault,  /* SIGPROF   (21) - profiling timer alarm */
	0,      /* SIGIO     (22) - asynch I/O signal */
	fault,  /* SIGWINCH  (23) - window size change */
	0,      /* SIGSTOP   (24) - stop */
	0,      /* SIGTSTP   (25) - keyboard stop signal */
	0,      /* SIGCONT   (26) - continue after stop */
	0,      /* SIGTTIN   (27) - background read attempt */
	0,      /* SIGTTOU   (28) - background write attempt */
	0,      /* SIGURG    (29) - urgent data arrival */
	0,      /* SIGLOST   (30) - file lock lost */
};


/* ========	fault handling routines	   ======== */


fault(sig,trap,scp)
register int	sig;
int	trap;
struct sigcontext *scp;
{
	register int	flag;
	register tchar *t;

#if defined(DEBUG) && defined(__hp9000s800)
	printf((nl_msg(201, "got signal #%d\n")), sig);
#endif /* DEBUG */
	if (sig != SIGCLD)
		signal(sig, fault);
	if (sig == SIGSEGV)
	{
#if defined(__hp9000s200)
		error((nl_msg(203, "Memory fault in shell: cleaning up.")));
#else
		if (setbrk(brkincr) == -1)
			error(nl_msg(603,nospace));
#endif
	}
	else if (sig == SIGCLD) {	/* fix bell bug now trap SIGCLD */
		int chld;
		chld=wait(0);
		trapnote |= SIGCAUGHT;
		trapflg[sig] |= SIGCAUGHT;
		signal(sig, fault);
	}
	else if (sig == SIGALRM)
	{
		if (flags & waiting)
			done();
	}
#ifdef RFA
	else if (sig == SIGSYS)
	{
		in_netunam = 1;
	}
#endif /* RFA */
	else if (sig == SIGHUP && (t = trapcom[sig]))
	{
		execexp(t, 0);
                exitval = 129;  /* exit value for SIGHUP (trust me) */
                done();
        }
#ifdef SIGWINCH
	else if (sig == SIGWINCH)
	{
		struct winsize wsize;

		if(!ioctl(0, TIOCGWINSZ, &wsize)) {
                    set_l_and_c(wsize.ws_row, wsize.ws_col);
		    flags |= noprompt;
                    if((t = trapcom[sig])) {
			int	savxit = exitval;
			if((flags & prompt) && (flags & waiting))
			    newline();
			execexp(t, 0);
			flags |= sigwtrap;
			exitval = savxit;
			exitset();
		    }
		    if(flags & prompt) {
			trapnote |= SIGSET;
			trapflg[sig] |= SIGSET;
		    }
                }
        }
#endif /* SIGWINCH */
	else
	{
		flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
		if (sig == SIGINT)
			wasintr++;
	}
}

stdsigs()
{
	setsig(SIGHUP);
	setsig(SIGINT);
#ifndef DEBUG
	ignsig(SIGQUIT);
#endif
	setsig(SIGILL);
	setsig(SIGTRAP);
	setsig(SIGIOT);
	setsig(SIGEMT);
	setsig(SIGFPE);
#ifndef DEBUG
	setsig(SIGBUS);
	signal(SIGSEGV, fault);
#endif
	setsig(SIGSYS, fault);
	setsig(SIGPIPE);
	setsig(SIGALRM);
	setsig(SIGTERM);
	setsig(SIGUSR1);
	setsig(SIGUSR2);
#ifdef SIGWINCH
	setsig(SIGWINCH);
#endif
	signal(SIGCLD, fault);
}

ignsig(n)
{
	register int	s, i;
	char nlmsg[BUFSIZ];			/* buffer to hold nl_msg str */

	if ((i = n) == SIGSEGV)
	{
		clrsig(i);
		movstr(nl_msg(626,badtrap), nlmsg);
		failed(nlmsg, (nl_msg(204, "cannot trap 11")));
	}
	else if (i == SIGCLD) 		/* no longer allow this trap  */
	{				/* these 5 lines fix bell bug */
		clrsig(i);
		movstr(nl_msg(626,badtrap), nlmsg);
		failed(nlmsg, (nl_msg(205, "cannot trap 18")));
	}
	else if ((s = (signal(i, SIG_IGN) == SIG_IGN)) == 0)
	{
		trapflg[i] |= SIGMOD;
	}
	return(s);
}

getsig(n)
{
	register int	i;

	if (trapflg[i = n] & SIGMOD || ignsig(i) == 0)
		signal(i, fault);
}


setsig(n)
{
	register int	i;

	if (ignsig(i = n) == 0)
		signal(i, sigval[i]);
}

oldsigs()
{
	register int	i;
	register tchar	*t;

	i = MAXTRAP;
	while (i--)
	{
		t = trapcom[i];
		if (t == 0 || *t)
			clrsig(i);
		trapflg[i] = 0;
	}
	trapnote = 0;
}

clrsig(i)
int	i;
{
	free(trapcom[i]);
	trapcom[i] = 0;
	if (trapflg[i] & SIGMOD)
	{
		trapflg[i] &= ~SIGMOD;
		signal(i, sigval[i]);
	}
}

/*
 * check for traps
 */
chktrap()
{
	register int	i = MAXTRAP;
	register tchar	*t;

	trapnote &= ~TRAPSET;
	while (--i)
	{
		if (trapflg[i] & TRAPSET)
		{
			trapflg[i] &= ~TRAPSET;
			if (t = trapcom[i])
			{
				int	savxit = exitval;

				execexp(t, 0);
				exitval = savxit;
				exitset();
			}
		}
	}
}
