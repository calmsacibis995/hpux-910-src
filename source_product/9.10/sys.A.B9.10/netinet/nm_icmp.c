/*
 * $Header: nm_icmp.c,v 1.3.83.4 93/09/17 19:04:37 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/nm_icmp.c,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:04:37 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nm_icmp.c $Revision: 1.3.83.4 $";
#endif

#include "../h/mib.h"
#include "../h/errno.h"
#include "../h/param.h"

/*
 *	ICMP counters
 */
int MIB_icmpcounter[MIB_icmpMAXCTR+1]={0};
/*
 *	Convert ICMP type to MIB object identifier
 */
int	XlateInId[] = {
	ID_icmpInEchoReps,		0,
	0,			ID_icmpInDestUnreachs,
	ID_icmpInSrcQuenchs,	ID_icmpInRedirects,
	0,			0,
	ID_icmpInEchos,		0,
	0,			ID_icmpInTimeExcds,
	ID_icmpInParmProbs,	ID_icmpInTimestamps,
	ID_icmpInTimestampReps,	0,
	0,			ID_icmpInAddrMasks,
	ID_icmpInAddrMaskReps,
	};

int	XlateOutId[] = {
	ID_icmpOutEchoReps,		0,
	0,			ID_icmpOutDestUnreachs,
	ID_icmpOutSrcQuenchs,	ID_icmpOutRedirects,
	0,			0,
	ID_icmpOutEchos,		0,
	0,			ID_icmpOutTimeExcds,
	ID_icmpOutParmProbs,	ID_icmpOutTimestamps,
	ID_icmpOutTimestampReps,	0,
	0,			ID_icmpOutAddrMasks,
	ID_icmpOutAddrMaskReps,
	};

/*
 *	Subsystem get_icmp routine
 */
nmget_icmp(id, ubuf, klen)
	int	id;			/* object identifier */
	char	*ubuf;			/* user buffer	*/
	int	*klen;			/* size of ubuf, in kernel */
{
	int	status=0,nbytes=0;
	char	*kbuf;

	switch (id) {

	case	ID_icmp : 	/* get all ICMP counters */

		nbytes = MIB_icmpMAXCTR * sizeof(counter);
		if (*klen < nbytes)
			return (EMSGSIZE);

		kbuf = (char *) &MIB_icmpcounter[1];
		*klen = nbytes;
		status = NULL;
		break;

	default	:		/*  get 1 ICMP counter */

		if ((id&INDX_MASK) > MIB_icmpMAXCTR)
			return (EINVAL);

		if (*klen < sizeof(counter))
			return (EMSGSIZE);

		kbuf = (char *) &MIB_icmpcounter[id & INDX_MASK];
		*klen = sizeof (counter);
		status = NULL;
	}
	if (status==NULL)
		status = copyout(kbuf, ubuf, *klen);
	return (status);
}
