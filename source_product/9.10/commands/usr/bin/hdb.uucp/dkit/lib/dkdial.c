/*	@(#) $Revision: 64.1 $	*/
/*
 *	create a Datakit connection to a remote destination
 */
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

#include <fcntl.h>
#include <dk.h>
#include <stdio.h>
#include <signal.h>
#ifdef	SVR3
#	define	SIGRTN	void
#else
#	define	SIGRTN	int
#endif
#include <setjmp.h>
#include <sysexits.h>
#include <errno.h>


#define DK_DEFWAIT	89	/* default time to wait for dial return */
#define	DK_MAXWAIT	600	/* maximum wait to allow the caller - 10 min */


unsigned int	dk_timewait = DK_DEFWAIT; /* Caller to dkdial might modify */

#if hpux
static char	Conn_Msg[] = "[%d] Can't connect to %s: %s\n";
#else
static char	Conn_Msg[] = "Can't connect to %s: %s\n";
#endif
static char	Resp_Msg[] = "No response from Datakit";

SIGRTN		timout();	/* Alarm signal handler */
static int	Elapsed;	/* Alarm time elapsed during dial */
static int	Timer;		/* Current alarm setting */
static short	TimeErr;	/* Alarm clock rang */

extern char	*getenv();
extern int	dk_verbose, dk_errno;

dkdial(dest)
	char *dest;
{
	return(dkndial(dest, atoi(getenv("DKINTF"))));
}

dkndial(dest, intf)
	char *dest;
{
	short		fd;		/* Channel Descriptor	*/
	SIGRTN		(*SigWas)();	/* Caller's alarm handler */
	unsigned int	TimWas;		/* Caller's alarm clock */
	char		*key;
	struct diocdial {
			struct	diocreq iocb;
			char	dialstring[128];
		}	ioreq;
	char		dial_dev[32];
#if hpux
	int chan;
#endif


	sprintf(dial_dev, "/dev/dk/dial%d", intf);

	/*
	** Clear our elapsed time and save caller's alarm stuff.
	*/

	Timer = Elapsed = 0;
	SigWas = signal(SIGALRM, timout);
	TimWas = alarm(0);

	/*
	** If requested timeout interval is unreasonable, use the default.
	*/

	if ((dk_timewait == 0)  || (dk_timewait > DK_MAXWAIT))
		dk_timewait = DK_DEFWAIT;

	/*
	** Do an alarm protected open of the dial device
	*/

	setalarm(dk_timewait);

	if ((fd = open(dial_dev, O_RDWR)) < 0) {
#if hpux
		extern char *sys_errlist[];
#endif
		setalarm(0);
		if (dk_verbose)
#if hpux
			fprintf(stderr, "dkdial: Can't open %s - %s\n", dial_dev, sys_errlist[errno]);
#else
			fprintf(stderr, "dkdial: Can't open %s\n", dial_dev);
#endif
		usralarm(TimWas, SigWas);
		if (errno == EBUSY)
			return(dk_errno = -EX_TEMPFAIL);
		else
			return(dk_errno = -EX_OSFILE);
	}

	/*
	** If the caller has a DKKEY, use it.
	*/

	if((key = getenv("DKKEY")) != NULL && getuid() == geteuid())
		sprintf(ioreq.dialstring, "%s\n%s", dest, key);
	else
		strcpy(ioreq.dialstring, dest);

	ioreq.iocb.req_traffic = 0;
	ioreq.iocb.req_1param = 0;
	ioreq.iocb.req_2param = 0;

	/*
	** Try to dial the call.  If the alarm expires during the ioctl,
	** the ioctl will return in error.
	*/

#if hpux
	chan = dkminor(fd);
#endif
	if (ioctl(fd, DKIODIAL, &ioreq) < 0) {
		setalarm(0);
		if (dk_verbose)
		if (TimeErr)
#if hpux
			fprintf(stderr, Conn_Msg, chan, ioreq.dialstring, Resp_Msg);

		else
			fprintf(stderr, Conn_Msg, chan, ioreq.dialstring, dkerr(ioreq.iocb.req_error));
#else
			fprintf(stderr, Conn_Msg, Resp_Msg, ioreq.dialstring);
		else
			fprintf(stderr, Conn_Msg, ioreq.dialstring, dkerr(ioreq.iocb.req_error));
#endif

		setalarm(2);		/* Don't wait forever on close */
		close(fd);
		usralarm(TimWas, SigWas);
		if (errno == EBUSY)
			return(-dkerrmap(dk_errno = -EX_TEMPFAIL));
		else
			return(-dkerrmap(dk_errno = ioreq.iocb.req_error));
	}
	usralarm(TimWas, SigWas);
	return (fd);
}

/*
** timout() is the alarm clock signal handling routine.  It is called
** whenever the alarm clock expires during dial processing.
*/

static SIGRTN
timout()
{
	TimeErr++;
}

/*
** setalarm() is called to request an alarm at a future time.  The residual
** from the previous alarm (if any) is added to the elapsed time counter.
*/

static
setalarm(Seconds)
{
	TimeErr = 0;
	(void) signal(SIGALRM, timout);
	Elapsed += Timer - alarm(Seconds);
	Timer = Seconds;
}

/*
** usralarm() is used to restore the alarm service for the caller.
*/

static
usralarm(TimWas, SigWas)
	int		TimWas;		/* Caller's alarm clock */
	SIGRTN		(*SigWas)();	/* Caller's alarm handler */
{
	Elapsed += Timer - alarm(0);
	(void) signal(SIGALRM, SigWas);
	if (TimWas > 0) {
		TimWas -= Elapsed;
		if (TimWas < 2)
			TimWas = 2;
	}
	alarm(TimWas);
}
