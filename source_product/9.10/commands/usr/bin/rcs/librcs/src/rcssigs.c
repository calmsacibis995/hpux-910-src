/*
 *                 RCS signal processing functions
 *
 *  rcscatchsig(), rcssigs(), rcsholdsigs(), rcsallowsigs()
 *
 * $Header: rcssigs.c,v 66.2 90/07/11 10:00:13 egeland Exp $
 * Copyright Hewlett Packard Co. 1986
 */

#include <signal.h>
#include "rcsbase.h"

extern int cleanup();
extern int diagnose();
extern void exit();
extern int faterror();
extern long sigsetmask();

extern char *cmdid;	/*command identification for error messages     */
static long rcssigmask;

void
rcscatchsig()
/*
 * Function: clean-up RCS command environment and abort the command.
 */
{
        diagnose("\nRCS: cleaning up\n");
        cleanup();
        exit(127);
}

void
rcssigs()
/*
 * Function: set-up initial signal environment to allow RCS commands
 * to clean-up the working environment before termination.
 * The following signals are enabled:
 *	SIGHUP	- hangup	SIGINT	- interrupt
 *	SIGQUIT	- quit		SIGPIPE	- write to plugged pipe
 *	SIGTERM	- software termination
 *
 * Except that SIGHUP, SIGINT, & SIGQUIT will be ignored if they are
 * ignored by the parent process, (normal condition for background
 * processes).
 */
{
	struct sigvec catchvec, ignorevec, prevvec;
	int sigrtn = 0;

	/* build mask for signals to be held off */
	rcssigmask=(1L<<(SIGHUP-1))|(1L<<(SIGINT-1))|(1L<<(SIGQUIT-1))|
		(1L<<(SIGPIPE-1))|(1L<<(SIGTERM-1));

	catchvec.sv_handler = rcscatchsig;
	catchvec.sv_mask = RCSSIGMASK;
	catchvec.sv_onstack = 0;

	ignorevec.sv_handler = SIG_IGN;
	ignorevec.sv_mask = RCSSIGMASK;
	ignorevec.sv_onstack = 0;

	sigrtn=sigrtn|sigvector(SIGHUP, 0, &prevvec);
	if (prevvec.sv_handler == SIG_IGN)
		sigrtn=sigrtn|sigvector(SIGHUP, &ignorevec, &prevvec);
	else	sigrtn=sigrtn|sigvector(SIGHUP, &catchvec, &prevvec);

	sigrtn=sigrtn|sigvector(SIGINT, 0, &prevvec);
	if (prevvec.sv_handler == SIG_IGN)
		sigrtn=sigrtn|sigvector(SIGINT, &ignorevec, &prevvec);
	else	sigrtn=sigrtn|sigvector(SIGINT, &catchvec, &prevvec);

	sigrtn=sigrtn|sigvector(SIGQUIT, 0, &prevvec);
	if (prevvec.sv_handler == SIG_IGN)
		sigrtn=sigrtn|sigvector(SIGQUIT, &ignorevec, &prevvec);
	else	sigrtn=sigrtn|sigvector(SIGQUIT, &catchvec, &prevvec);

	sigrtn=sigrtn|sigvector(SIGPIPE, &catchvec, &prevvec);
	sigrtn=sigrtn|sigvector(SIGTERM, &catchvec, &prevvec);

#ifndef hp9000ipc
	if (sigrtn != 0)
		faterror("sigvector failure");
#endif
}

void
rcsholdsigs()
/*
 * Function: mask off all signals during critical sections of RCS commands
 */
{
	(void) sigsetmask(rcssigmask);
}

void
rcsallowsigs()
/*
 * Function: reset signal mask to allow all signals
 */
{

	(void) sigsetmask(RCSSIGMASK);
}
