/*
 * $Header: net_init.c,v 1.4.83.5 93/10/12 13:58:18 root Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/net/RCS/net_init.c,v $
 * $Revision: 1.4.83.5 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/12 13:58:18 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) net_init.c $Revision: 1.4.83.5 $";
#endif

#ifndef lint
static char HPPROD_ID[]="@(#) FILESET NET: lib net: Version: A.09.03";
#endif

#include "../h/netfunc.h"
#include "../h/mbuf.h"

extern int ifinit();
extern int ifioctl();
extern int null_init();
extern int rtioctl();
extern int netisr_daemon();
extern int ntimo_init();

struct mbuf trace_mbuf;
struct mbuf *trace_m;

/* Initialize the networking subsystem */
net_init()
{
	static int initialized = 0;

	if (!initialized) {
		loattach();

		netproc_assign(NET_IFINIT,	ifinit);
		netproc_assign(NET_IFIOCTL,	ifioctl);
		netproc_assign(NET_NULL_INIT,	null_init);
		netproc_assign(NET_RTIOCTL,	rtioctl);
                netproc_assign(NET_NTIMO_INIT,  ntimo_init);
                netproc_assign(NET_NETISR_DAEMON, netisr_daemon);

		trace_mbuf.m_next = 0;
		trace_mbuf.m_act = 0;
		trace_mbuf.m_flags = 0;
		trace_mbuf.m_off = MMINOFF;
		trace_mbuf.m_len = sizeof(short);
		trace_mbuf.m_type = MT_HEADER;
		trace_m = &trace_mbuf;

		initialized = 1;
	}
}
