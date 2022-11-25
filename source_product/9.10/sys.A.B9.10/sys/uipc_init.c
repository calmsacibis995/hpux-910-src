/*
 * $Header: uipc_init.c,v 1.5.83.4 93/10/13 12:39:45 root Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/uipc_init.c,v $
 * $Revision: 1.5.83.4 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/13 12:39:45 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) uipc_init.c $Revision: 1.5.83.4 $";
#endif

#ifndef lint
static char HPPROD_ID[]="@(#) FILESET BSDIPC-SOCKET: lib uipc: Version: A.09.03";
#endif

#include "../h/types.h"
#include "../h/netfunc.h"
#include "../h/syscall.h"
#include "../h/systm.h"

extern int domaininit();
extern int soo_stat();
extern int unp_gc();

/*
 * Berkeley IPC System Calls
 */

extern int mbinit();
extern int accept();
extern int bind();
extern int connect();
extern int getpeername();
extern int getsockname();
extern int getsockopt();
extern int listen();
extern int recv();
extern int recvfrom();
extern int recvmsg();
extern int send();
extern int sendmsg();
extern int sendto();
extern int setsockopt();
extern int shutdown();
extern int socket();
extern int socketpair();
extern int ntimo_init();

#ifdef _WSIO
uipc_link()
#else
uipc_init()
#endif
{
	netproc_assign(NET_MBINIT,      mbinit);

	netproc_assign(NET_DOMAININIT,	domaininit);
	netproc_assign(NET_SOO_STAT,	soo_stat);
	netproc_assign(NET_UNP_GC,	unp_gc);
	netproc_assign(NET_NTIMO_INIT,  ntimo_init);

	sysent_assign(SYS_ACCEPT,	3, accept);
	sysent_assign(SYS_BIND,		3, bind);
	sysent_assign(SYS_CONNECT,	3, connect);
	sysent_assign(SYS_GETPEERNAME,	3, getpeername);
	sysent_assign(SYS_GETSOCKNAME,	3, getsockname);
	sysent_assign(SYS_GETSOCKOPT,	5, getsockopt);
	sysent_assign(SYS_LISTEN,	2, listen);
	sysent_assign(SYS_RECV,		4, recv);
	sysent_assign(SYS_RECVFROM,	6, recvfrom);
	sysent_assign(SYS_RECVMSG,	3, recvmsg);
	sysent_assign(SYS_SEND,		4, send);
	sysent_assign(SYS_SENDMSG,	3, sendmsg);
	sysent_assign(SYS_SENDTO,	6, sendto);
	sysent_assign(SYS_SETSOCKOPT,	5, setsockopt);
	sysent_assign(SYS_SHUTDOWN,	2, shutdown);
	sysent_assign(SYS_SOCKET,	3, socket);
	sysent_assign(SYS_SOCKETPAIR,	4, socketpair);

	netisr_init();
	ntimo_init();
	uipc_init_compat();

}
