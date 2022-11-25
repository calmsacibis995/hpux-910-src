/* @(#) $Revision: 56.1 $ */     
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	error handling	======== */

failed(s1, s2)
char	*s1, *s2;
{
	prp();
	prs_cntl(s1);
	if (s2)
	{
		prs(colon);
		prs(s2);
	}
	newline();
	exitsh(ERROR);
}

#ifdef NLS
tfailed(s1, s2)	
tchar	*s1;
char	*s2;
{
	prp();
	prst_cntl(s1);
	if (s2)
	{
		prs(colon);	
		prs(s2);
	}
	newline();
	exitsh(ERROR);
}
#endif NLS

error(s)
char	*s;
{
	failed(s, NIL);
}

exitsh(xno)
int	xno;
{
	/*
	 * Arrive here from `FATAL' errors
	 *  a) exit command,
	 *  b) default trap,
	 *  c) fault with no trap set.
	 *
	 * Action is to return to command level or exit.
	 */
	exitval = xno;
	flags |= eflag;
	if ((flags & (forked | errflg | ttyflg)) != ttyflg)
		done();
	else
	{
		clearup();
		restore(0);
		clear_buff();
		execbrk = breakcnt = funcnt = 0;
		longjmp(errshell, 1);
	}
}

done()
{
	register tchar	*t;
	int savxit = exitval;

	if (t = trapcom[0])
	{
		trapcom[0] = 0;
		if (!setjmp(errshell))
			execexp(t, 0);
		exitval = savxit;
		free(t);
	}
	else
		chktrap();

	rmtemp(0);
	rmfunctmp();

#ifdef ACCT
	doacct();
#endif
	exit(exitval);
}

rmtemp(base)
struct ionod	*base;
{
	while (iotemp > base)
	{
		unlink(to_char(iotemp->ioname));
		free(iotemp->iolink);
		iotemp = iotemp->iolst;
	}
}

rmfunctmp()
{
	while (fiotemp)
	{
		unlink(to_char(fiotemp->ioname));
		fiotemp = fiotemp->iolst;
	}
}
