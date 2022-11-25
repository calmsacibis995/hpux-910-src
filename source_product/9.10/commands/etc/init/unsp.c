/* HPUX_ID: @(#) $Revision: 56.2 $  */

#include <stdio.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/unsp.h>
#include "unsp_serve.h"

#ifdef	SILENT_ON_ERRORS
#define	do_perror(s)	/* no-op */
#define	console(f,a)	/* no-op */
#endif	SILENT_ON_ERRORS

extern int errno;
extern int nuroutines;

/*
 *Invoke the user NSP.  The following steps are taken.
 *	Close file descriptor 0.
 *	Open the nsp file.  Should return fd 0.
 *	Use ioctl to get the operation.
 *	Execute the appropriate program.
 */

invoke_unsp()
{
	int fd;
	register op;
	register struct unsp_routine *urtn;
	int save_errno;

	close(0);	/*so open_nsp gets fd 0*/
	fd = unsp_open();
	if (fd != 0)
	{
		if (fd < 0)
			do_perror ("unsp_open");
		else
			console ("unsp_open: fd %d\n",fd);
		exit (1);
	}
	op = ioctl(fd,UNSP_GETOP,0);
	if (op < 0)
	{
		do_perror ("unsp getop");
		exit (1);
	}
	for (urtn = &(unsp_routines[nuroutines - 1]);
		urtn >= unsp_routines; urtn--)
	{
		if (urtn->op == op)
		{
			execl(urtn->func, urtn->func, 0);
			/*should not return*/

			save_errno = errno;

			/* We must call ioctl() before do_perror(), which */
			/* closes file descriptor 0 as a side effect.     */
			/* We reply with ENOEXEC rather than errno        */
			/* because it is more likely to be meaningful.    */
			ioctl(fd,UNSP_ERROR,ENOEXEC);
			ioctl(fd,UNSP_REPLY,0);
			errno = save_errno;
			do_perror (urtn->func);
			exit (1);
		}
	}

	/*if we are here, we never found the op */

	/* We must call ioctl() before console(), which */
	/* closes file descriptor 0 as a side effect.   */
	ioctl(fd,UNSP_ERROR,EINVAL);
	ioctl(fd,UNSP_REPLY,0);
	console ("invalid unsp op %d\n",op);
	exit(1);
}

/*
 *This routine is called whenever SIGUSR2 is received.  It forks, and
 *the child invokes the UNSP.
 */

need_unsp()
{
	struct PROC_TABLE *efork();

	if (efork(0, 0) == 0)  /* child */
	{
		(void) sigsetmask (0L); /* unmask signal that got us here */
		invoke_unsp();
		/*NOTREACHED*/
	}

	/*parent returns*/
}

#ifndef	SILENT_ON_ERRORS
do_perror (string)
char *string;
{
	extern int sys_nerr;
	extern char *sys_errlist[];

	if (errno <= 0 || errno > sys_nerr)
		console ("%s: unknown error %d\n", string, errno);
	else
		console ("%s: %s\n", string, sys_errlist[errno]);
}
#endif	SILENT_ON_ERRORS
