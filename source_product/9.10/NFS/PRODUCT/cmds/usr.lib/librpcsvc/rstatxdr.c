/*	@(#)rstatxdr.c	$Revision: 1.22.109.1 $	$Date: 91/11/19 14:25:12 $
rstatxdr.c	2.1 86/04/14 NFSSRC 
static  char sccsid[] = "rstatxdr.c 1.1 86/02/05 Copyr 1984 Sun Micro";
*/

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
**	rstat.h defines the values of DK_NDRIVE, CPUSTATES and N_AVENRUN!
**	Sun brain-damage assumed that these values would be the
**	same for all systems; not true for the s800.  With this
**	way of doing things, we only use the "sun-defined" values
**	expected (implicitly) by the rstat protocol.
*/
#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>

rstat(host, statp)
	char *host;
	struct statstime *statp;
{
	return (callrpc(host, RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
	    xdr_void, 0, xdr_statstime, statp));
}

havedisk(host)
	char *host;
{
	long have;
	
	if (callrpc(host, RSTATPROG, RSTATVERS_SWTCH, RSTATPROC_HAVEDISK,
	    xdr_void, 0, xdr_long,  &have) != 0)
		return (-1);
	else
		return (have);
}

xdr_stats(xdrs, statp)
	XDR *xdrs;
	struct stats *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	/*
	 * NFS 3.0 implementations of this protocol incorrectly left
	 * out if_opackets.  So, so that we can talk to those previous
	 * implementations, we will not return an error if this call 
 	 * fails.
	 */
	(void) xdr_int (xdrs, &statp->if_opackets);
	return (1);

}

xdr_statsswtch(xdrs, statp)		/* version 2 */
	XDR *xdrs;
	struct statsswtch *statp;
{
	int i;

	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < N_AVENRUN; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	/*
	 * NFS 3.0 implementations of this protocol incorrectly left
	 * out if_opackets.  So, so that we can talk to those previous
	 * implementations, we will not return an error if this call 
 	 * fails.
	 */
	(void) xdr_int (xdrs, &statp->if_opackets);
	return (1);
}

xdr_statstime(xdrs, statp)		/* version 3 */
	XDR *xdrs;
	struct statstime *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < N_AVENRUN; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	if (xdr_timeval(xdrs, &statp->curtime) == 0)
		return (0);
	/*
	 * NFS 3.0 implementations of this protocol incorrectly left
	 * out if_opackets.  So, so that we can talk to those previous
	 * implementations, we will not return an error if this call 
 	 * fails.
	 */
	(void) xdr_int (xdrs, &statp->if_opackets);
	return (1);
}

xdr_timeval(xdrs, tvp)
	XDR *xdrs;
	struct timeval *tvp;
{
	if (xdr_long(xdrs, &tvp->tv_sec) == 0)
		return (0);
	if (xdr_long(xdrs, &tvp->tv_usec) == 0)
		return (0);
	return (1);
}
