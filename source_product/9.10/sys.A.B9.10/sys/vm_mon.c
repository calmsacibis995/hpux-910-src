/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_mon.c,v $
 * $Revision: 1.10.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:17:45 $
 */


/*	vm_mon.c	6.1	83/07/29	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/vmmeter.h"
#include "../h/trace.h"
#include "../h/kernel.h"

#ifdef PGINPROF

int pmonmin = PMONMIN;
int pres = PRES;
int rmonmin = RMONMIN;
int rres = RRES;

vmsizmon()
{
	register int i;

	i = (DATASIZE(u.u_procp) / DRES) < NDMON ? 
			(DATASIZE(u.u_procp) / DRES):NDMON;
	dmon[i] += u.u_procp->p_utime.tv_sec - u.u_outime;

	i = (USTACKSIZE(u.u_procp) / SRES) < NSMON ? 
			(USTACKSIZE(u.u_procp) / SRES):NSMON;
	smon[i] += u.u_procp->p_utime.tv_sec - u.u_outime;
	u.u_outime = u.u_procp->p_utime.tv_sec;
}

vmfltmon(hist, atime, amin, res, nmax)
	register unsigned int *hist;
	int atime, amin, res, nmax;
{
	register int i;

	i = (atime - amin) / res;
	if (i>=0 && i<nmax)
		hist[i+1]++;
	else 
		i<0 ? hist[0]++ : hist[nmax+1]++;
}
#endif

#ifdef TRACE
/*VARARGS*/
/*ARGSUSED*/
trace1(args, arg1, arg2, arg3)
	int args, arg1, arg2, arg3;
{
	register int nargs;
	register int x;
	register int *argp, *tracep;

	nargs = 4;
	x = tracex % TRCSIZ;
	if (x + nargs >= TRCSIZ) {
		tracex += (TRCSIZ - x);
		x = 0;
	}
	argp = &args;
	tracep = &tracebuf[x];
	tracex += nargs;
	*tracep++ = (time.tv_sec%1000)*1000 + (time.tv_usec/1000);
	nargs--;
	do
		*tracep++ = *argp++;
	while (--nargs > 0);
}
#endif
