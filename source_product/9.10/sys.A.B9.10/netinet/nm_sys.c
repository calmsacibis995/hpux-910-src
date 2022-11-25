/*
 * $Header: nm_sys.c,v 1.4.83.4 93/09/17 19:04:49 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/nm_sys.c,v $
 * $Revision: 1.4.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:04:49 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nm_sys.c $Revision: 1.4.83.4 $";
#endif

#include "../h/mib.h"
#include "../netinet/mib_kern.h"
#include "../h/param.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/utsname.h"
#include "../h/malloc.h"
extern	mib_unsupp();
extern	nmget_sys();
/*
 *	Global Variables
 */
struct	timeval	sysInit;		/* Network Mgmt Init time */
/*	
 *	Initialization routine for Network Management
 */
nm_init()
{
	int	s;

	/* Time Network Mgmt system is initialized */
	s = spl7();
	sysInit.tv_sec	= time.tv_sec;
	sysInit.tv_usec = time.tv_usec;
	splx(s);

	/* Register support routines for System Group */
	NMREG (GP_sys, nmget_sys, mib_unsupp,  mib_unsupp, mib_unsupp);
}
/*
 *	Get routine for Systems Group 
 */
nmget_sys(id, ubuf, klen)
	int	id;			/* object identifier */
	char	*ubuf;			/* user buffer */
	int	*klen;			/* size of ubuf, in kernel */
{
	int	status=0;
	TimeTicks uptime;
	char	*kbuf;

	switch (id) {

	case ID_sysUpTime :
		
		if (*klen < sizeof(TimeTicks))
			return (EMSGSIZE);

		nmget_sysUpTime(&uptime);
		kbuf = (char *) &uptime;
		*klen = sizeof(TimeTicks);
		status = NULL;
		break;

	default :
		return (EINVAL);
	};

	if (status == NULL)
		status = copyout(kbuf, ubuf, *klen);
	return (status);
}
/*
 *	Get time since network mgmt was initialized, in 1/100 of seconds.
 */
nmget_sysUpTime(kbuf) 
	TimeTicks	*kbuf;
{
	struct timeval	diff;
	int	s;

	/* determine the lapsed time between current time and sysInit */
	s = spl7();
	diff.tv_sec = time.tv_sec - sysInit.tv_sec;
	diff.tv_usec = time.tv_usec - sysInit.tv_usec;
	splx(s);
	if (diff.tv_usec < 0) {
		diff.tv_sec --;
		diff.tv_usec += 1000000;
	}
	*kbuf = (TimeTicks) (100*diff.tv_sec + (diff.tv_usec)/10000);
	return (NULL);
}
