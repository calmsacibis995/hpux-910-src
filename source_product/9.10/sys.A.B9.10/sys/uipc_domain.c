/*
 * $Header: uipc_domain.c,v 1.3.83.3 93/09/17 20:13:52 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/uipc_domain.c,v $
 * $Revision: 1.3.83.3 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 20:13:52 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) uipc_domain.c $Revision: 1.3.83.3 $";
#endif

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)uipc_domain.c	7.3 (Berkeley) 6/29/88
 */

#include "../h/param.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/time.h"
#include "../h/kernel.h"
#include "../h/netfunc.h"

/* moved ADDDOMAIN macro to domain.h */

domaininit()
{
	register struct domain *dp;
	register struct protosw *pr;

#ifndef lint
	ADDDOMAIN(unix);
#ifdef IMP
#include "imp.h"
#if NIMP > 0
	ADDDOMAIN(imp);
#endif
#endif
#endif

	for (dp = domains; dp; dp = dp->dom_next) {
		if (dp->dom_init)
			(*dp->dom_init)();
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_init)
				(*pr->pr_init)();
	}
	NETCALL(NET_NULL_INIT)();
	pffasttimo();
	pfslowtimo();
	/*	initialize netwk mgmt time */
	nm_init();
}

struct protosw *
pffindtype(family, type)
	int family, type;
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_type && pr->pr_type == type)
			return (pr);
	return (0);
}

struct protosw *
pffindproto(family, protocol, type)
	int family, protocol, type;
{
	register struct domain *dp;
	register struct protosw *pr;
	struct protosw *maybe = 0;

	if (family == 0)
		return (0);
	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++) {
		if ((pr->pr_protocol == protocol) && (pr->pr_type == type))
			return (pr);

		if (type == SOCK_RAW && pr->pr_type == SOCK_RAW &&
		    pr->pr_protocol == 0 && maybe == (struct protosw *)0)
			maybe = pr;
	}
	return (maybe);
}

pfctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_ctlinput)
				(*pr->pr_ctlinput)(cmd, sa);
}

pfslowtimo()
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_slowtimo)
				(*pr->pr_slowtimo)();
	net_timeout(pfslowtimo, (caddr_t)0, hz/2);
}

pffasttimo()
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_fasttimo)
				(*pr->pr_fasttimo)();
	net_timeout(pffasttimo, (caddr_t)0, hz/5);
}
