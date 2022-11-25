/* @(#) $Revision: 64.2 $ */    
/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * popen() and pclose()
 *
 * stolen from C library, modified to use SHELL variable
 */


#include <stdio.h>
#include <signal.h>

#define	tst(a,b) (*mode == 'r'? (b) : (a))
#define	RDR	0
#define	WTR	1

extern FILE *fdopen();
extern int execlp(), vfork(), pipe(), close(), fcntl();
static int popen_pid[20];

FILE *
popen(cmd, mode)
char	*cmd, *mode;
{
	int	p[2];
	register int myside, yourside, pid;
	char *sHELL, *value();

	if ((sHELL = value("SHELL"))==NULL)
		sHELL = "/bin/sh";
	if(pipe(p) < 0)
		return(NULL);
	myside = tst(p[WTR], p[RDR]);
	yourside = tst(p[RDR], p[WTR]);
	if((pid = vfork()) == 0) {
		/* myside and yourside reverse roles in child */
		int	stdio;
		stdio = tst(0, 1);
		(void) close(myside);
		(void) close(stdio);
		(void) fcntl(yourside, 0, stdio);
		(void) close(yourside);
		(void) execlp(sHELL, "sh", "-c", cmd, 0);
		(void) execlp("/bin/sh", "sh", "-c", cmd, 0);
		_exit(1);
	}
	if(pid == -1)
		return(NULL);
	popen_pid[myside] = pid;
	(void) close(yourside);
	return(fdopen(myside, mode));
}

int
pclose(ptr)
FILE	*ptr;
{
	register int f, r;
#ifdef VMUNIX
	int status, omask;
#else
	int status;
	void (*hstat)(), (*istat)(), (*qstat)();
#endif VMUNIX

	f = fileno(ptr);
	(void) fclose(ptr);
#ifdef VMUNIX
	omask = sigblock(mask(SIGINT)|mask(SIGQUIT)|mask(SIGHUP));
#else
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	hstat = signal(SIGHUP, SIG_IGN);
#endif VMUNIX
	while((r = wait(&status)) != popen_pid[f] && r != -1)
		;
	if(r == -1)
		status = -1;
#ifdef VMUNIX
	sigsetmask(omask);
#else
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	(void) signal(SIGHUP, hstat);
#endif VMUNIX
	return(status);
}
