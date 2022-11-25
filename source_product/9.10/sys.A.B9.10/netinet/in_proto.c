/*
 * $Header: in_proto.c,v 1.24.83.4 93/09/17 19:03:00 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/in_proto.c,v $
 * $Revision: 1.24.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:03:00 $
 */
#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) in_proto.c $Revision: 1.24.83.4 $ $Date: 93/09/17 19:03:00 $";
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
 *	@(#)in_proto.c	7.3 (Berkeley) 6/29/88 plus MULTICAST 1.0
 */

#include "../h/param.h"
#include "../h/socket.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/mbuf.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"

/*
 * TCP/IP protocol family: IP, ICMP, UDP, TCP.
 */
int	ip_output(),ip_ctloutput();
int	ip_init(),ip_slowtimo(),ip_drain();
int	icmp_input();
#ifdef	MULTICAST
int	igmp_init(),igmp_input(),igmp_fasttimo();
#endif	/* MULTICAST */
int	udp_input(),udp_ctlinput();
int	udp_usrreq();
int	udp_init();
#ifdef	__hp9000s800
int	udp_usrsend(),tcp_usrsend();
#endif	/* __hp9000s800 */
int	tcp_input(),tcp_ctlinput();
int	tcp_usrreq(),tcp_ctloutput();
int	tcp_init(),tcp_fasttimo(),tcp_slowtimo(),tcp_drain();
int	rip_input(),rip_output(),rip_ctloutput();
extern	int raw_usrreq(), raw_init();
/*
 * IMP protocol family: raw interface.
 * Using the raw interface entry to get the timer routine
 * in is a kludge.
 */
#ifdef IMP
#include "imp.h"
#if NIMP > 0
int	rimp_output(), hostslowtimo();
#endif
#endif

#ifdef NSIP
int	idpip_input(), nsip_ctlinput();
#endif

extern	struct domain inetdomain;

struct protosw inetsw[] = {
{ 0,		&inetdomain,	0,		0,
  0,		ip_output,	0,		0,
  0,		0,
  ip_init,	0,		ip_slowtimo,	ip_drain,
},
{ SOCK_STREAM,	&inetdomain,	IPPROTO_TCP,	
#ifdef	__hp9000s800
  PR_CONNREQUIRED|PR_WANTRCVD|PR_COPYAVOID,
#else
  PR_CONNREQUIRED|PR_WANTRCVD,
#endif	/* __hp9000s800 */
  tcp_input,	0,		tcp_ctlinput,	tcp_ctloutput,
#ifdef	__hp9000s800
  tcp_usrreq,	tcp_usrsend,
#else
  tcp_usrreq,	0,
#endif	/* __hp9000s800 */
  tcp_init,	tcp_fasttimo,	tcp_slowtimo,	tcp_drain,
},
{ SOCK_DGRAM,	&inetdomain,	IPPROTO_UDP,	
#ifdef	__hp9000s800
  PR_ATOMIC|PR_ADDR|PR_COPYAVOID,
#else
  PR_ATOMIC|PR_ADDR,
#endif	/* __hp9000s800 */
  udp_input,	0,		udp_ctlinput,	ip_ctloutput,
#ifdef	__hp9000s800
  udp_usrreq,	udp_usrsend,
#else
  udp_usrreq,	0,
#endif	/* __hp9000s800 */
  udp_init,	0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_RAW,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,	0,
  0,		0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_ICMP,	PR_ATOMIC|PR_ADDR,
  icmp_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,	0,
  0,		0,		0,		0,
},
#ifdef MULTICAST
{ SOCK_RAW,	&inetdomain,	IPPROTO_IGMP,	PR_ATOMIC|PR_ADDR,
  igmp_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,	0,
  igmp_init,	igmp_fasttimo,	0,		0,
},
#endif /* MULTICAST */
{ SOCK_RAW,	&inetdomain,	IPPROTO_EGP,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,	0,
  0,		0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_IGP,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,	0,
  0,		0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_OSPF,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,	0,
  0,		0,		0,		0,
},
{ SOCK_DGRAM,	&inetdomain,	IPPROTO_PXP,	PR_ATOMIC,
  0,		0,		0,		0,
  0,		0,
  0,		0,		0,		0,
},
#ifdef NSIP
{ SOCK_RAW,	&inetdomain,	IPPROTO_IDP,	PR_ATOMIC|PR_ADDR,
  idpip_input,	rip_output,	nsip_ctlinput,	0,
  raw_usrreq,	0,
  0,		0,		0,		0,
},
#endif
	/* raw wildcard */
{ SOCK_RAW,	&inetdomain,	0,		PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  raw_usrreq,	0,
  raw_init,	0,		0,		0,
},
};

struct domain inetdomain =
    { AF_INET, "internet", 0, 0, 0, 
      inetsw, &inetsw[sizeof(inetsw)/sizeof(inetsw[0])] };

#if NIMP > 0
extern	struct domain impdomain;

struct protosw impsw[] = {
{ SOCK_RAW,	&impdomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		rimp_output,	0,		0,
  raw_usrreq,	0,
  0,		0,		hostslowtimo,	0,
},
};

struct domain impdomain =
    { AF_IMPLINK, "imp", 0, 0, 0,
      impsw, &impsw[sizeof (impsw)/sizeof(impsw[0])] };
#endif

#ifdef HY
#include "hy.h"
#if NHY > 0
/*
 * HYPERchannel protocol family: raw interface.
 */
int	rhy_output();
extern	struct domain hydomain;

struct protosw hysw[] = {
{ SOCK_RAW,	&hydomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		rhy_output,	0,		0,
  raw_usrreq,	0,
  0,		0,		0,		0,
},
};

struct domain hydomain =
    { AF_HYLINK, "hy", 0, 0, 0, hysw, &hysw[sizeof (hysw)/sizeof(hysw[0])] };
#endif
#endif
