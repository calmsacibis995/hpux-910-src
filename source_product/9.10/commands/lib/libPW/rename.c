/* @(#) $Revision: 37.1 $ */      
# include "errno.h"
# include "fatal.h"
# include "signal.h"
/*
	rename (unlink/link)
	Calls xlink() and xunlink().
*/

rename(oldname,newname)
char *oldname, *newname;
{
	extern int errno;
	int retval;
#ifndef	hpux
	int (* holdsig[3])();
#else	hpux
	long old_mask;
	extern long sigblock();
#endif	hpux
	
	/* Ignore SIGHUP, SIGINT, SIGQUIT [ SIGTSTP ] */
#ifndef	hpux
	holdsig[0] = signal(SIGHUP,SIG_IGN);
	holdsig[1] = signal(SIGINT,SIG_IGN);
	holdsig[2] = signal(SIGQUIT,SIG_IGN);
#else	hpux
	old_mask = sigblock((1<<(SIGHUP-1)) |
			    (1<<(SIGINT-1)) |
#  ifdef SIGTSTP
			    (1<<(SIGTSTP-1)) |
#  endif SIGTSTP
			    (1<<(SIGQUIT-1)));
#endif	hpux
	if (unlink(newname) < 0 && errno != ENOENT)
		retval = xunlink(newname);

	if (xlink(oldname,newname) == Fvalue)
		retval = -1;
	retval = (xunlink(oldname));
	/*re establish signals */
#ifndef	hpux
	signal(SIGHUP,holdsig[0]);
	signal(SIGINT,holdsig[1]);
	signal(SIGQUIT,holdsig[2]);
#else	hpux
	(void) sigsetmask(old_mask);
#endif	hpux
	return(retval);
}
