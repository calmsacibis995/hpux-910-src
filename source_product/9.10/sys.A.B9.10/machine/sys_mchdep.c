/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/sys_mchdep.c,v $
 * $Revision: 1.4.84.5 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/11 16:45:44 $
 */
/* HPUX_ID: @(#)sys_mchdep.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/tty.h"
#include "../h/termio.h"
#include "../h/ttold.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/mtio.h"
#include "../h/buf.h"
#include "../h/trace.h"

#ifdef TRACE
int	nvualarm;

vtrace()
{
	register struct a {
		int	request;
		int	value;
	} *uap;
	int vdoualarm();

	uap = (struct a *)u.u_ap;
	switch (uap->request) {

	case VTR_DISABLE:		/* disable a trace point */
	case VTR_ENABLE:		/* enable a trace point */
		if (uap->value < 0 || uap->value >= TR_NFLAGS)
			u.u_error = EINVAL;
		else {
			u.u_r.r_val1 = traceflags[uap->value];
			traceflags[uap->value] = uap->request;
		}
		break;

	case VTR_VALUE:		/* return a trace point setting */
		if (uap->value < 0 || uap->value >= TR_NFLAGS)
			u.u_error = EINVAL;
		else
			u.u_r.r_val1 = traceflags[uap->value];
		break;

	case VTR_UALARM:	/* set a real-time ualarm, less than 1 min */
		if (uap->value <= 0 || uap->value > 60 * hz ||
		    nvualarm > 5)
			u.u_error = EINVAL;
		else {
			nvualarm++;
			timeout(vdoualarm, (caddr_t)u.u_procp->p_pid,
			    uap->value);
		}
		break;

	case VTR_STAMP:
		trace(TR_STAMP, uap->value, u.u_procp->p_pid);
		break;
	}
}

vdoualarm(arg)
	int arg;
{
	register struct proc *p;

	p = pfind(arg);
	if (p)
		psignal(p, 16);
	nvualarm--;
}
#endif

/*
 * Note: these tables are sorted by
 * ioctl "code" (in ascending order).
 */
int mctls[] = { /* MTIOCTOP, MTIOCGET XXX (twg !@#$) , */ 0 };
int tctls[] = { TIOCGETP, TIOCSETP, 0 };
int Tctls[] = { TCGETA, TCSETA, TCSETAW, TCSETAF, TCSBRK, TCXONC, TCFLSH, 0 };

/*
 * Map an old style ioctl command to new.
 */
mapioctl(cmd)
	int cmd;
{
	register int *map, c;

	switch ((cmd >> 8) & 0xff) {

	case 'm':
		map = mctls;
		break;

	case 't':
		map = tctls;
		break;
	
	case 'T':
		map = Tctls;
		break;

	default:
		return (0);
	}
	while ((c = *map) && (c&0xff) < (cmd&0xff))
		map++;
	if (c && (c&0xff) == (cmd&0xff))
		return (c);
	return (0);
}

    /* Validate buffer pointer */
bpcheck(bp, size, log2secsz, offset)
register struct buf *bp;
register daddr_t size;
register long offset;
{
	register int range_mask = ((1 << log2secsz) - 1);

/*
**  if device is 2G or smaller, don't allow offsets to be negative
*/
	if (size <= ((unsigned)0x80000000 >> log2secsz)) {
		range_mask |= 0x80000000;
	}

/*
**  odd buffer address?
*/
	if ((int)bp->b_un.b_addr & 1) {
		bp->b_error = EFAULT;
		goto error;
	}

/*
**  start of the transfer device sector alligned?
*/
	if (bp->b_offset & range_mask || (offset += bp->b_offset) & range_mask)
		goto enxioerr;
	bp->b_un2.b_sectno = (unsigned)offset >> log2secsz;

/*
**  length of the transfer a whole number of device sectors?
*/
	if (bp->b_bcount & range_mask) 
		goto enxioerr;

/*  
**  driver strategy for handling end of volume:
**
**    .  b_resid set to requested amount (b_bcount)
**    .  b_bcount possibly cut back due to end of volume
**    .  driver attempts to transfer up to b_bcount bytes,
**       decrementing b_resid as it goes
**    .  afterwards, b_resid reflects residual due to either eov or error
*/
bp->b_resid = bp->b_bcount;

/*
**  does the request start within range?
*/
	if (bp->b_un2.b_sectno >= size)
		if (bp->b_un2.b_sectno == size && bp->b_flags & B_READ)
			goto done;
		else {
			bp->b_error = ENOSPC;
			goto error;
		}

/*
**  if the request goes beyond the end of volume, cut it back if it is
**  a user raw request not from the pageout deamon.
*/
	if ((unsigned)bp->b_bcount>>log2secsz > (size -= bp->b_un2.b_sectno))
		if (bp->b_flags & B_PHYS)
/*
** trim count to just the number of bytes remaining on device 
*/
			bp->b_bcount = (unsigned)size << log2secsz;
		else
			goto enxioerr;
	return(0);

enxioerr:      
	bp->b_error = ENXIO;
error:
	bp->b_flags |= B_ERROR;
done:
	iodone(bp);
	return(1);
}

/*
 * XXX this is just to allow us to use the existing config.  We'll have
 *  to clean up config & remove these before release.  Andy.
 */
swap_strategy()
{
	return(ENODEV);
}
swap1_strategy()
{
	return(ENODEV);
}
swap_read()
{
	return(ENODEV);
}
swap_write()
{
	return(ENODEV);
}
