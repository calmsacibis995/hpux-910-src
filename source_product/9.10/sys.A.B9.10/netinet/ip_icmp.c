/*
 * $Header: ip_icmp.c,v 1.38.83.10 94/05/11 12:53:30 dkm Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/netinet/RCS/ip_icmp.c,v $
 * $Revision: 1.38.83.10 $		$Author: dkm $
 * $State: Exp $		$Locker:  $
 * $Date: 94/05/11 12:53:30 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) ip_icmp.c $Revision: 1.38.83.10 $";
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
 *	@(#)ip_icmp.c	7.8 (Berkeley) 6/29/88 plus MULTICAST 1.0
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/socket.h"
#include "../h/time.h"
#include "../h/kernel.h"

#include "../net/route.h"
#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/ip_icmp.h"
#include "../netinet/icmp_var.h"
#include "../h/mib.h"
#include "../netinet/mib_kern.h"
#include "../h/subsys_id.h"
#include "../h/net_diag.h"
extern	counter	MIB_icmpcounter[];
extern	int	XlateInId[];
extern	int	XlateOutId[];

#ifdef ICMPPRINTFS
/*
 * ICMP routines: error generation, receive packet processing, and
 * routines to turnaround packets back to the originator, and
 * host table maintenance routines.
 */
int	icmpprintfs = 0;
#endif

/*
 * Generate an error packet of type error
 * in response to bad packet ip.
 */
/*VARARGS4*/
icmp_error(oip, type, code, ifp, dest)
	struct ip *oip;
	int type, code;
	struct ifnet *ifp;
	struct in_addr dest;
{
	register unsigned oiplen = oip->ip_hl << 2;
	register struct icmp *icp;
	struct mbuf *m;
	struct ip *nip;
	unsigned icmplen;

#ifdef ICMPPRINTFS
	if (icmpprintfs)
		printf("icmp_error(%x, %d, %d)\n", oip, type, code);
#endif
	MIB_icmpIncrCounter(ID_icmpOutMsgs);
	if (type != ICMP_REDIRECT)
		icmpstat.icps_error++;
	/*
	 * Don't send error if not the first fragment of message.
	 * Don't error if the old packet protocol was ICMP
	 * error message, only known informational types.
	 */
	if (oip->ip_off &~ (IP_MF|IP_DF)) {
		MIB_icmpIncrCounter(ID_icmpOutErrors);
		goto free;
	}
	if (oip->ip_p == IPPROTO_ICMP && type != ICMP_REDIRECT &&
	  !ICMP_INFOTYPE(((struct icmp *)((caddr_t)oip + oiplen))->icmp_type)) {
		icmpstat.icps_oldicmp++;
		MIB_icmpIncrCounter(ID_icmpOutErrors);
		goto free;
	}

#ifdef MULTICAST
	/*
	 * Don't send error in response to a multicast or broadcast packet.
	 */
	if (IN_MULTICAST(ntohl(oip->ip_dst.s_addr)) ||
	    in_broadcast(oip->ip_dst))
		goto free;
#endif MULTICAST

	/*
	 * First, formulate icmp message
	 */
	m = m_get(M_DONTWAIT, MT_HEADER);
	if (m == NULL) {
		MIB_icmpIncrCounter(ID_icmpOutErrors);
		goto free;
	}
	icmplen = oiplen + MIN(8, oip->ip_len);
	m->m_len = icmplen + ICMP_MINLEN;
	m->m_off = MMAXOFF - m->m_len;
	icp = mtod(m, struct icmp *);
	if ((u_int)type > ICMP_MAXTYPE)
		panic("icmp_error");
	icmpstat.icps_outhist[type]++;
	MIB_icmpIncrCounter(XlateOutId[type]);
	icp->icmp_type = type;
	if (type == ICMP_REDIRECT)
		icp->icmp_gwaddr = dest;
	else
		icp->icmp_void = 0;
	if (type == ICMP_PARAMPROB) {
		icp->icmp_pptr = code;
		code = 0;
	}
	icp->icmp_code = code;
	bcopy((caddr_t)oip, (caddr_t)&icp->icmp_ip, icmplen);
	nip = &icp->icmp_ip;
	nip->ip_len += oiplen;
	nip->ip_len = htons((u_short)nip->ip_len);

	/*
	 * Now, copy old ip header in front of icmp message.(without opts!)
	 */
	oiplen = sizeof(struct ip);
	m->m_off -= oiplen;
	m->m_len += oiplen;
	nip = mtod(m, struct ip *);
	bcopy((caddr_t)oip, (caddr_t)nip, oiplen);
	nip->ip_len = m->m_len;
	/* HP : Fix DTS INDaa09449.  System panic when orignial ip header 
		has record route options. 
	*/
	nip->ip_hl  = oiplen >>2;
	nip->ip_p = IPPROTO_ICMP;
	NS_LOG_INFO5( LE_IP_ICMP_ERROR_MSG, NS_LC_PROLOG, NS_LS_IP,
					0, 3, type, code, dest.s_addr, 0, 0);
	icmp_reflect(nip, ifp);

free:
	m_freem(dtom(oip));
}

static struct sockproto icmproto = { AF_INET, IPPROTO_ICMP };
static struct sockaddr_in icmpsrc = { AF_INET };
static struct sockaddr_in icmpdst = { AF_INET };
static struct sockaddr_in icmpgw = { AF_INET };
struct in_ifaddr *ifptoia();

#ifdef PING_DEBUG
/*
 *	"Ping debugging" - allow troubleshooting via ping(1m) packets.
 *	Things like doing an "osps" on the console can be done by anyone
 *	as long as the global "ping_debugging" is non-zero; things 
 *	like killing processes or panicking can only be done by a 
 *	system that passes the pd_addr & pd_mask tests
 */
int ping_debugging = 1;
extern int osps(), dump_stack_pid(), boot(), os_global_summary();
extern struct proc *pfind();
extern int vhandinfoticks, do_savecore;
extern char panic_msg[];
extern int panic_msg_len;
#ifdef OSDEBUG
unsigned long pd_addr1 = 0x0f062000;
unsigned long pd_addr2, pd_addr3;
unsigned long pd_mask1 = 0xffffff00;
unsigned long pd_mask2, pd_mask3;
#else
unsigned long pd_addr1, pd_addr2, pd_addr3;
unsigned long pd_mask1, pd_mask2, pd_mask3;
#endif
#endif

/*
 * Process a received ICMP message.
 */
icmp_input(m, ifp)
	register struct mbuf *m;
	struct ifnet *ifp;
{
	register struct icmp *icp;
	register struct ip *ip = mtod(m, struct ip *);
	int icmplen = ip->ip_len, hlen = ip->ip_hl << 2;
	register int i;
	struct in_ifaddr *ia;
	int (*ctlfunc)(), code;
	extern u_char ip_protox[];
	extern struct in_addr in_makeaddr();
	u_short *s_port;

#ifdef PING_DEBUG
	unsigned int cmd, arg, root_ok;
	unsigned long src_addr;
	struct proc *p;
#endif


	/*
	 * Locate icmp structure in mbuf, and check
	 * that not corrupted and of at least minimum length.
	 */
#ifdef ICMPPRINTFS
	if (icmpprintfs)
		printf("icmp_input from %x, len %d\n", ip->ip_src, icmplen);
#endif
	MIB_icmpIncrCounter(ID_icmpInMsgs);
	if (icmplen < ICMP_MINLEN) {
		icmpstat.icps_tooshort++;
		MIB_icmpIncrCounter(ID_icmpInErrors);
		goto free;
	}
	i = hlen + MIN(icmplen, ICMP_ADVLENMIN);
 	if ((m->m_off > MMAXOFF || m->m_len < i) &&
 		(m = m_pullup(m, i)) == 0)  {
		icmpstat.icps_tooshort++;
		MIB_icmpIncrCounter(ID_icmpInErrors);
		return;
	}
 	ip = mtod(m, struct ip *);
	m->m_len -= hlen;
	m->m_off += hlen;
	icp = mtod(m, struct icmp *);
	if (in_cksum(m, icmplen)) {
		icmpstat.icps_checksum++;
		MIB_icmpIncrCounter(ID_icmpInErrors);
		goto free;
	}
	m->m_len += hlen;
	m->m_off -= hlen;

#ifdef ICMPPRINTFS
	/*
	 * Message type specific processing.
	 */
	if (icmpprintfs)
		printf("icmp_input, type %d code %d\n", icp->icmp_type,
		    icp->icmp_code);
#endif
	NS_LOG_INFO5( LE_IP_ICMP_INPUT, NS_LC_PROLOG, NS_LS_IP, 0, 3,
		      icp->icmp_type, icp->icmp_code, ip->ip_src.s_addr, 0, 0);
 
	if (icp->icmp_type > ICMP_MAXTYPE)
		goto raw;
	icmpstat.icps_inhist[icp->icmp_type]++;
	MIB_icmpIncrCounter( XlateInId [icp->icmp_type] );
	code = icp->icmp_code;
	switch (icp->icmp_type) {

	case ICMP_UNREACH:
		if (code > 5)
			goto badcode;
		code += PRC_UNREACH_NET;
		goto deliver;

	case ICMP_TIMXCEED:
		if (code > 1)
			goto badcode;
		code += PRC_TIMXCEED_INTRANS;
		goto deliver;

	case ICMP_PARAMPROB:
		if (code)
			goto badcode;
		code = PRC_PARAMPROB;
		goto deliver;

	case ICMP_SOURCEQUENCH:
		if (code)
			goto badcode;
		code = PRC_QUENCH;
	deliver:
		/*
		 * Problem with datagram; advise higher level routines.
		 */
		icp->icmp_ip.ip_len = ntohs((u_short)icp->icmp_ip.ip_len);
		if (icmplen < ICMP_ADVLENMIN || icmplen < ICMP_ADVLEN(icp)) {
			icmpstat.icps_badlen++;
			MIB_icmpIncrCounter(ID_icmpInErrors);
			goto free;
		}
#ifdef ICMPPRINTFS
		if (icmpprintfs)
			printf("deliver to protocol %d\n", icp->icmp_ip.ip_p);
#endif
		icmpsrc.sin_addr = icp->icmp_ip.ip_dst;

		/* if the port on a host is unreachable then get the source
	           port from the old TCP/UDP header and pass it up to 
		   in_pcbnotify(). */
                if ((icp->icmp_type == ICMP_UNREACH) &&
       		    (icp->icmp_code == ICMP_UNREACH_PORT)) {
		        s_port = (u_short *)&icp->icmp_ip;
		              /* multiply by 2 below because ip_hl is in
			         32 bit units and s_port is a 16 bit pointer */
		        s_port += (u_short)(icp->icmp_ip.ip_hl * 2);
		        icmpsrc.sin_port = (u_short)*s_port;
	        }
	        else
		        icmpsrc.sin_port = 0;

		if (ctlfunc = inetsw[ip_protox[icp->icmp_ip.ip_p]].pr_ctlinput)
			(*ctlfunc)(code, (struct sockaddr *)&icmpsrc);
		break;

	badcode:
		icmpstat.icps_badcode++;
		MIB_icmpIncrCounter(ID_icmpInErrors);
		break;

	case ICMP_ECHO:
		icp->icmp_type = ICMP_ECHOREPLY;
#ifdef PING_DEBUG		
		if (ping_debugging && (ip->ip_len == 109)) {
			cmd = *((unsigned int *) ip + 9);
    			arg = *((unsigned int *) ip + 10);	
    			src_addr = ip->ip_src.s_addr;
		    	if ((pd_addr1 && (src_addr & pd_mask1) == pd_addr1) || 
		    	    (pd_addr2 && (src_addr & pd_mask2) == pd_addr2) || 
		    	    (pd_addr3 && (src_addr & pd_mask3) == pd_addr3))
		        	root_ok = 1;
		        else
	    			root_ok = 0;
#ifdef OSDEBUG
	printf("PD: cmd is %c, arg is 0x%x, src_addr is 0x%x, root_ok is %d\n", 
		(char) cmd, arg, src_addr, root_ok);
#endif		
	
	                switch (cmd) {
	                	case 'D':
	                		if (root_ok)
	                			call_kdb();
	                		break;
	       	     		case 'K':
	            			if (root_ok && (p = pfind(arg)))
		            			psignal(p, SIGKILL);
			            	break;
			        case 'P':
			        	if (root_ok) {
			        		sprintf(panic_msg, panic_msg_len,
			        			"PD: 0x%x ", src_addr);
			        		panic(panic_msg);
			        	}
			        	break;
			        case 'R':
			        	if (root_ok)
				        	boot(arg);
		        		break;
				case 'c': 
					do_savecore = arg;
					break;
				case 'g':
					os_global_summary();
					break;
				case 'i':
					vhandinfoticks = arg;
					break;
				case 'p':
					osps();
					break;
				case 's':
					dump_stack_pid(arg);
					break;
				default:
					msg_printf("ICMP: got code %d\n", 
						   (unsigned int) cmd);
			}
		}
#endif		
		goto reflect;

	case ICMP_TSTAMP:
		if (icmplen < ICMP_TSLEN) {
			icmpstat.icps_badlen++;
			MIB_icmpIncrCounter(ID_icmpInErrors);
			break;
		}
		icp->icmp_type = ICMP_TSTAMPREPLY;
		icp->icmp_rtime = iptime();
		icp->icmp_ttime = icp->icmp_rtime;	/* bogus, do later! */
		goto reflect;
		
	case ICMP_IREQ:
#define	satosin(sa)	((struct sockaddr_in *)(sa))
		if (in_netof(ip->ip_src) == 0 && (ia = ifptoia(ifp)))
			ip->ip_src = in_makeaddr(in_netof(IA_SIN(ia)->sin_addr),
			    in_lnaof(ip->ip_src));
		icp->icmp_type = ICMP_IREQREPLY;
		goto reflect;

	case ICMP_MASKREQ:
		if (icmplen < ICMP_MASKLEN || (ia = ifptoia(ifp)) == 0)
			break;
		icp->icmp_type = ICMP_MASKREPLY;
		icp->icmp_mask = htonl(ia->ia_subnetmask);
		if (ip->ip_src.s_addr == 0) {
			if (ia->ia_ifp->if_flags & IFF_BROADCAST)
			    ip->ip_src = satosin(&ia->ia_broadaddr)->sin_addr;
			else if (ia->ia_ifp->if_flags & IFF_POINTOPOINT)
			    ip->ip_src = satosin(&ia->ia_dstaddr)->sin_addr;
		}
reflect:
		ip->ip_len += hlen;	/* since ip_input deducts this */
		icmpstat.icps_reflect++;
		icmpstat.icps_outhist[icp->icmp_type]++;
		MIB_icmpIncrCounter( XlateOutId [icp->icmp_type] );
		icmp_reflect(ip, ifp);
		return;

	case ICMP_REDIRECT:
		if (icmplen < ICMP_ADVLENMIN || icmplen < ICMP_ADVLEN(icp)) {
			icmpstat.icps_badlen++;
			MIB_icmpIncrCounter(ID_icmpInErrors);
			break;
		}
		/*
		 * Short circuit routing redirects to force
		 * immediate change in the kernel's routing
		 * tables.  The message is also handed to anyone
		 * listening on a raw socket (e.g. the routing
		 * daemon for use in updating its tables).
		 */
		icmpgw.sin_addr = ip->ip_src;
		icmpdst.sin_addr = icp->icmp_gwaddr;
#ifdef	ICMPPRINTFS
		if (icmpprintfs)
			printf("redirect dst %x to %x\n", icp->icmp_ip.ip_dst,
				icp->icmp_gwaddr);
#endif
		if (code == ICMP_REDIRECT_NET || code == ICMP_REDIRECT_TOSNET) {
			icmpsrc.sin_addr =
			 in_makeaddr(in_netof(icp->icmp_ip.ip_dst), INADDR_ANY);
			rtredirect((struct sockaddr *)&icmpsrc,
			  (struct sockaddr *)&icmpdst, RTF_GATEWAY,
			  (struct sockaddr *)&icmpgw);
			icmpsrc.sin_addr = icp->icmp_ip.ip_dst;
			pfctlinput(PRC_REDIRECT_NET,
			  (struct sockaddr *)&icmpsrc);
		} else {
			icmpsrc.sin_addr = icp->icmp_ip.ip_dst;
			rtredirect((struct sockaddr *)&icmpsrc,
			  (struct sockaddr *)&icmpdst, RTF_GATEWAY | RTF_HOST,
			  (struct sockaddr *)&icmpgw);
			pfctlinput(PRC_REDIRECT_HOST,
			  (struct sockaddr *)&icmpsrc);
		}
		break;

	/*
	 * No kernel processing for the following;
	 * just fall through to send to raw listener.
	 */
	case ICMP_ECHOREPLY:
	case ICMP_TSTAMPREPLY:
	case ICMP_IREQREPLY:
	case ICMP_MASKREPLY:
	default:
		break;
	}

raw:
	icmpsrc.sin_addr = ip->ip_src;
	icmpdst.sin_addr = ip->ip_dst;
	raw_input(m, &icmproto, (struct sockaddr *)&icmpsrc,
	    (struct sockaddr *)&icmpdst);
	return;

free:
	m_freem(m);
}

/*
 * Reflect the ip packet back to the source
 */
icmp_reflect(ip, ifp)
	register struct ip *ip;
	struct ifnet *ifp;
{
	register struct in_ifaddr *ia;
	struct in_addr t;
	struct mbuf *opts = 0, *ip_srcroute();
	int optlen = (ip->ip_hl << 2) - sizeof(struct ip);

	t = ip->ip_dst;
	ip->ip_dst = ip->ip_src;
	/*
	 * If the incoming packet was addressed directly to us,
	 * use dst as the src for the reply.  Otherwise (broadcast
	 * or anonymous), use the address which corresponds
	 * to the incoming interface.
	 */
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
		if (t.s_addr == IA_SIN(ia)->sin_addr.s_addr)
			break;
		if ((ia->ia_ifp->if_flags & IFF_BROADCAST) &&
		    t.s_addr == satosin(&ia->ia_broadaddr)->sin_addr.s_addr)
			break;
	}
	if (ia == (struct in_ifaddr *)0)
		ia = ifptoia(ifp);
	if (ia == (struct in_ifaddr *)0)
		ia = in_ifaddr;
	t = IA_SIN(ia)->sin_addr;
	ip->ip_src = t;
	ip->ip_ttl = ipDefaultTTL;

	if (optlen > 0) {
		/*
		 * Retrieve any source routing from the incoming packet
		 * and strip out other options.  Adjust the IP length.
		 */
		opts = ip_srcroute();
		if (!opts)
		    if (opts = m_get(M_DONTWAIT, MT_SOOPTS))
			opts->m_len = 0;
		if (opts)
			ip_stripoptions(ip, opts);
		else  /* admit defeat */
			ip_stripoptions(ip, (struct mbuf *)0);
		ip->ip_len -= optlen;
	}
	icmp_send(ip, opts);
	if (opts)
		(void)m_free(opts);
}

struct in_ifaddr *
ifptoia(ifp)
	struct ifnet *ifp;
{
	register struct in_ifaddr *ia;

	for (ia = in_ifaddr; ia; ia = ia->ia_next)
		if (ia->ia_ifp == ifp)
			return (ia);
	return ((struct in_ifaddr *)0);
}

/*
 * Send an icmp packet back to the ip level,
 * after supplying a checksum.
 */
icmp_send(ip, opts)
	register struct ip *ip;
	struct mbuf *opts;
{
	register int hlen;
	register struct icmp *icp;
	register struct mbuf *m;

	m = dtom(ip);
	hlen = ip->ip_hl << 2;
	m->m_off += hlen;
	m->m_len -= hlen;
	icp = mtod(m, struct icmp *);
	icp->icmp_cksum = 0;
	icp->icmp_cksum = in_cksum(m, ip->ip_len - hlen);
	m->m_off -= hlen;
	m->m_len += hlen;
#ifdef ICMPPRINTFS
	if (icmpprintfs)
		printf("icmp_send dst %x src %x\n", ip->ip_dst, ip->ip_src);
#endif
	(void) ip_output(m, opts, (struct route *)0, 0);
}

n_time
iptime()
{
	struct timeval atv;
	struct timeval ms_gettimeofday();
	u_long t;

	atv = ms_gettimeofday();
	t = (atv.tv_sec % (24*60*60)) * 1000 + atv.tv_usec / 1000;
	return (htonl(t));
}
