/*
 * $Header: nipc_init.c,v 1.5.83.5 93/10/12 13:56:44 root Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_init.c,v $
 * $Revision: 1.5.83.5 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/12 13:56:44 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_init.c $Revision: 1.5.83.5 $";
#endif

#ifndef lint
static char HPPROD_ID[]="@(#) FILESET NETIPC: lib nipc: Version: A.09.03";
#endif

#include "../h/types.h"
#include "../h/netfunc.h"
/* 
 * Netipc initialization routine.
 */

#include "../h/types.h"
#include "../h/netfunc.h"
#include "../h/syscall.h"
#include "../h/systm.h"
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/time.h"
#include "../h/uio.h"
#include "../h/malloc.h"
#include "../h/buf.h"
#include "../h/mbuf.h"
#include "../h/domain.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/protosw.h"
#include "../h/ns_ipc.h"
#include "../netinet/in.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_domain.h"

#include "../h/kern_sem.h"      /* MPNET */
#include "../net/netmp.h"       /* MPNET */


extern struct nipc_domain	hpdsn_domain;
struct nipc_domain		*nipc_domain_list=0;
extern void hpdsn_init();

/*
 * The following routines are used to initialize PXP.
 */

extern struct domain inetdomain;

void	srd_create();	/* kernel sockregd process initialization */
void	nm_soinit();	/* global pxp request socket initialization */

#ifdef _WSIO
nipc_link()
#else
nipc_init()
#endif /* _WSIO */
{
	int 	nipc_init2();
	int	 new_ipcconnect();
	int	 new_ipccontrol();
	int	 new_ipccreate();
	int	 new_ipcdest();
	int	 new_ipclookup();
	int	 new_ipcname();
	int	 new_ipcnamerase();
	int	 new_ipcrecv();
	int	 new_ipcrecvcn();
	int	 new_ipcselect();
	int	 new_ipcsend();
	int	 new_ipcshutdown();
	int	 new_ipcgetnodename();
	int	 new_ipcsetnodename();

	/* INITIALIZE THE SYSCALL ENTRY POINTS FOR 8.0 */
	sysent_assign(SYS_IPCCONNECT,	6,	 new_ipcconnect);
	sysent_assign(SYS_IPCCONTROL,	8,	 new_ipccontrol);
	sysent_assign(SYS_IPCCREATE,	6,	 new_ipccreate);
	sysent_assign(SYS_IPCDEST,	10,	 new_ipcdest);
	sysent_assign(SYS_IPCLOOKUP,	9,	 new_ipclookup);
	sysent_assign(SYS_IPCNAME,	4,	 new_ipcname);
	sysent_assign(SYS_IPCNAMERASE,	3,	 new_ipcnamerase);
	sysent_assign(SYS_IPCRECV,	6,	 new_ipcrecv);
	sysent_assign(SYS_IPCRECVCN,	7,	 new_ipcrecvcn);
	sysent_assign(SYS_IPCSELECT,	6,	 new_ipcselect);
	sysent_assign(SYS_IPCSEND,	6,	 new_ipcsend);
	sysent_assign(SYS_IPCSHUTDOWN,	4,	 new_ipcshutdown);
	sysent_assign(SYS_IPCGETNODENAME,3,	 new_ipcgetnodename);
	sysent_assign(SYS_IPCSETNODENAME,3,	 new_ipcsetnodename);

	nipc_compat_init(); 

	netproc_assign(NET_NIPCINIT,	nipc_init2);
}

nipc_init2()
{
	struct protosw	*prot;
	struct protosw	*pffindproto();
	int		pxp_usrreq();
	int		pxp_input();
	int		pxp_fasttimo();
	int		pxp_ctlinput();
	int		oldlevel;		/* MPNET */
	sv_sema_t	savestate;		/* MPNET: MP save state */

	/* HP-UX 8.05
	 * spl2 protects interdependent structure changes in pxp_init and
	 * nm_soinit.  problems arise because pxp_fasttimo might be called
	 * between related structure changes, which violates integrity of
	 * assumptions in pxp_fasttimo and intopxpcb
	 */

	NETMP_GO_EXCLUSIVE(oldlevel, savestate);  /* MPNET: MP prot on */

	prot = pffindproto(AF_INET, IPPROTO_PXP, SOCK_DGRAM);
	prot->pr_usrreq = pxp_usrreq;
	prot->pr_input = pxp_input;
	prot->pr_fasttimo = pxp_fasttimo;
	prot->pr_ctlinput = pxp_ctlinput;

	pxp_init();

	hpdsn_init();

	/* INITIALIZE THE NETIPC DOMAINS */
	nipc_domain_list = &hpdsn_domain;

	(void) nm_soinit();

	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);  /* MPNET: prot off */

	/* let srd choose it's own spl */

	srd_create();
}
