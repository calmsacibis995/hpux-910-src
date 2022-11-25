/*
 * $Header: inet_init.c,v 1.8.83.5 93/10/12 13:49:58 root Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/inet_init.c,v $
 * $Revision: 1.8.83.5 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/12 13:49:58 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) inet_init.c $Revision: 1.8.83.5 $";
#endif

#ifndef lint
static char HPPROD_ID[]="@(#) FILESET NETINET: lib inet: Version: A.09.03";
#endif

#include "../h/types.h"
#include "../h/domain.h"
#include "../h/netfunc.h"
#include "../h/socket.h"
#include "../net/af.h"

extern int ipintr();
extern int rawintr();
extern int inet_hash();
extern int inet_netmatch();
extern int nvs_ioc_join();
extern int nvs_input();
extern int arpinit();
extern int arpintr();

#ifdef _WSIO
inet_link()
#else
inet_init()
#endif /* _WSIO */
{
	net_init();
	ADDDOMAIN(inet);
	netproc_assign(NET_IPINTR, ipintr);
	netproc_assign(NET_RAWINTR, rawintr);
	netproc_assign(NET_NVSJOIN, nvs_ioc_join);
	netproc_assign(NET_NVSINPUT, nvs_input);
	netproc_assign(NET_ARPINTR, arpintr);
	netproc_assign(NET_ARPINIT, arpinit);
	afswitch[AF_INET].af_hash = inet_hash;
	afswitch[AF_INET].af_netmatch = inet_netmatch;
}

#ifdef	hp9000s800
ovbcopy(src, dst, len)
char *src, *dst;
int len;
{
	if (len <= 0)
		return;

	if ((unsigned) src < (unsigned) dst &&
	    (unsigned) src + len > (unsigned) dst) {
		len--;
		src += len;
		dst += len;
		do {
			*dst-- = *src--;
		} while (len--);
	} else
		bcopy(src, dst, len);
}
#endif	/* hp9000s800 */
