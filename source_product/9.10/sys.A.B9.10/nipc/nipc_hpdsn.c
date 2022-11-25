/*
 * $Header: nipc_hpdsn.c,v 1.3.83.4 93/09/17 19:10:14 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_hpdsn.c,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:10:14 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_hpdsn.c $Revision: 1.3.83.4 $";
#endif

/*
 */


#include "../h/types.h"
#include "../h/types.h"
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
#include "../h/ns_ipc.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_protosw.h"
#include "../nipc/nipc_domain.h"
#include "../nipc/nipc_hpdsn.h"
#include "../nipc/nipc_err.h"
#include "../net/if.h"
#include "../net/route.h"
#include "../netinet/in.h"
#include "../netinet/in_var.h"
#include "../netinet/in_pcb.h"

int	mi_tcpaddr();
int	mi_pxpaddr();
int	hpdsn_reportvalid();
int	hpdsn_getnodeaddr();
int	hpdsn_getcsiteaddr();
extern	struct mbuf * hpdsn_buildcsite();
extern	struct mbuf * hpdsn_nodetocsite();
extern	struct mbuf * path_addreport();

extern struct nipc_domain hpdsn_domain;

struct nipc_protosw hpdsnsw[] = {
{	
	NSP_TCP,		/* np_protocol */
	NIPC_TCP_ADDRLEN,	/* address length */
	NSB_TCPCKSUM | NSB_TCP,	/* group service */
	NSB_TCPCKSUM,		/* service map */
	mi_tcpaddr,		/* proto_addr routine */
	0,			/* ptr to bsd protosw */
	&hpdsn_domain,		/* ptr to Netipc domain structure */
	NS_CALL,		/* type of Netipc socket */
},
{	
	NSP_TCP,		/* np_protocol */
	NIPC_TCP_ADDRLEN,	/* address length */
	NSB_TCPCKSUM | NSB_TCP,	/* group service */
	NSB_TCPCKSUM,	 	/* service map */
	mi_tcpaddr,		/* proto_addr routine */
	0,			/* ptr to bsd protosw */
	&hpdsn_domain,		/* ptr to Netipc domain structure */
	NS_VC,			/* type of Netipc socket */
},
{	
	NSP_HPPXP,		/* np_protocol */
	NIPC_PXP_ADDRLEN,	/* address length */
	NSB_HPPXP,		/* group service */
	0, 			/* service map */
	mi_pxpaddr,		/* proto_addr routine */
	0,			/* ptr to bsd protosw */
	&hpdsn_domain,		/* ptr to Netipc domain structure */
	NS_REQUEST,		/* type of Netipc socket */
},
{	
	NSP_HPPXP,		/* np_protocol */
	NIPC_PXP_ADDRLEN,	/* address length */
	NSB_HPPXP,		/* group service */
	0, 			/* service map */
	mi_pxpaddr,		/* proto_addr routine */
	0,			/* ptr to bsd protosw */
	&hpdsn_domain,		/* ptr to Netipc domain structure */
	NS_REPLY,		/* type of Netipc socket */
},
};

/* 
 * addr past last array element = start of array * sizeof array;
 */
#define hpdsnswNPROTOSW ((struct nipc_protosw *) (((char *) hpdsnsw) + sizeof(hpdsnsw)))

struct nipc_domain hpdsn_domain =
{
(HPDSN_VER << 8) + HPDSN_DOM,		/*	nd_domain;		*/
hpdsnsw,		/*	nd_protosw;		*/
hpdsnswNPROTOSW,	/*	nd_protoswNPROTOSW;		*/
hpdsn_reportvalid,	/* 	(*nd_reportvalid)();	*/
hpdsn_buildcsite,	/* 	(*nd_buildcsite)();	*/
hpdsn_getcsiteaddr,	/* 	(*nd_getcsiteaddr)();	*/
hpdsn_getnodeaddr,	/* 	(*nd_getnodeaddr)();	*/
hpdsn_nodetocsite,	/* 	(*nd_nodetocsite)();	*/
0			/* 	*nd_next_domain;	*/
};

void
hpdsn_init()
{
/* SET THE BSD_PROTOSW POINTERS IN THE hpdsn PROTOSW STRUCTS */
hpdsnsw[0].np_bsd_protosw = pffindproto(AF_INET, IPPROTO_TCP, SOCK_STREAM);
if (hpdsnsw[0].np_bsd_protosw == NULL)
	panic("hpdsn_init: cant find tcp ");

hpdsnsw[1].np_bsd_protosw = hpdsnsw[0].np_bsd_protosw;

hpdsnsw[2].np_bsd_protosw = pffindproto(AF_INET, IPPROTO_PXP, SOCK_DGRAM);
if (hpdsnsw[2].np_bsd_protosw == NULL)
	panic("hpdsn_init: cant find pxp ");

hpdsnsw[3].np_bsd_protosw = hpdsnsw[2].np_bsd_protosw;
/* next is already null */
}



/*
 *  Illustrated below is the "generic" format of hpdsn domain reports.  An
 *  hpdsn domain report consists of a domain header, which includes an
 *  internet address, followed by a list of one or more paths that apply to
 *  the internet address.  Each path is further divided into path elements
 *  which provide information about the services/protocols for the various
 *  layers of the path.
 *
 *
 *   +--------------------------------+<-----------------  
 *   |      domain report length      |                  \ 
 *   +--------------------------------+                   |
 *   | domain version|   domain id    |                   | 
 *   +--------------------------------+                   |
 *   |    internet address (word 1)   |                   |  
 *   +--------------------------------+                   |  
 *   |    internet address (word 2)   |                   |  
 *   +--------------------------------+<--------          |  
 *   |          path length           |         \         |  
 *   +--------------------------------+<         |        |  
 *   | element  pid  |  element  len  | \        |        |  
 *   +--------------------------------+  path    |        |  
 *   | element service map (optional) | element  |     domain
 *   +--------------------------------+  /       |     report
 *   |    element address (optional)  | /        |        |  
 *   +--------------------------------+<        path      |  
 *   | element pid   |  element len   | \        |        |  
 *   +--------------------------------+  path    |        |  
 *   | element service map (optional) | element  |        |  
 *   +--------------------------------+   /      |        |  
 *   |    element address (optional)  |  /      /         |  
 *   +--------------------------------+<--------          |  
 *   |          path length           |         \         |  
 *   +--------------------------------+<         |        |  
 *   | element  pid  |  element  len  | \        |        |  
 *   +--------------------------------+  path   path      |  
 *   | element service map (optional) | element  |        |  
 *   +--------------------------------+  /       |        |  
 *   |    element address (optional)  | /       /        /   
 *   +--------------------------------+<-----------------
 *   
 */

/* The defines given below are used in parsing hpdsn domain reports */

#define DR_LEN		0	/* offset to domain report length	      */
#define DR_ID		1	/* offset to domain report version & id       */
#define DR_ADDR1	2	/* offset to domain report address (word 1)   */
#define DR_ADDR2	3	/* offset to domain report address (word 2)   */
#define DR_PATH		4	/* offset to first path of domain report      */
#define P_LEN		0	/* path relative offset to path length        */
#define P_ELMT		1	/* path relative offset to first path element */
#define PE_ID		0	/* path elmt relative offset to pid & length  */
#define PE_SERVICE	1	/* path elmt relative offset to service map   */
#define PE_ADDR		2	/* path elmt relative offset to address	      */


/* The following structure is used in building an HP-UX hpdsn "connect site"  */
/* domain report.  A connect site domain report for HP-UX always has the same */
/* format consisting of a single path containing two path elements.  One path */
/* element is for the layer 4 protocol (TCP) and includes a service map and   */
/* an address field, the address being the protocol port address of the       */
/* connect site.  The other path element is for the IP protocol.  This path   */
/* element merely includes an address field containing the IP sap.	      */

struct csdr {
	u_short		csdr_len;	/* domain report length */
	u_char		csdr_vers;	/* domain report version */
	u_char		csdr_domain;	/* domain id */
	u_long		csdr_ipaddr;	/* internet address */
	u_short		csdr_pathlen;	/* length of csdr path */
	u_char		csdr_L4pid;  	/* L4 path element protocol id */
	u_char		csdr_L4len;	/* L4 path element length */
	u_short		csdr_L4servmap; /* L4 path element service map */
	u_short		csdr_L4addr;	/* L4 path element address */
	u_char		csdr_L3pid;	/* L3 path element protocol id */
	u_char		csdr_L3len;	/* L3 path element length */
	u_short		csdr_L3addr;	/* L3 path element address */
};

#define HPDSN_CSDR_LEN	     18	/* length of hpdsn csite domain report */
#define HPDSN_CSDR_PATH_LEN  10	/* length of hpdsn csite path	       */
#define HPDSN_CSDR_L4_LEN    4	/* length of layer 4 path element      */
#define HPDSN_CSDR_L3_LEN    2	/* length of layer 3 (IP) path element */
#define HPDSN_CSDR_IP_SAP    6  /* service access point for IP protocol*/

struct mbuf *
hpdsn_buildcsite (so, m, loopback)

struct socket	*so;	/* socket to create csite domain report(s) for */
struct mbuf	*m;	/* mbuf to which the csdrs are to be added     */
int		loopback; /* nonzero if loopback interface should be included */

{
	struct nipc_protosw *proto;	/* protocol info for the socket  */
	struct inpcb *inp;	/* inpcb for the socket		 */
	struct csdr csdr;	/* connect site domain report */
	struct in_ifaddr *ia;  /* AFINET interface address list */
	int s;				/* spl setting			 */

	/* initialize pointers to the protosw and inpcb */
	proto = ((struct nipccb *)(so->so_ipccb))->n_protosw;
	inp   = (struct inpcb *)so->so_pcb;
	if (!inp) {
		m = NULL;
		return (m);
	}

	/* initialize the csdr for the socket */
	csdr.csdr_len		= HPDSN_CSDR_LEN;
	csdr.csdr_vers		= HPDSN_VER;
	csdr.csdr_domain	= HPDSN_DOM;
	csdr.csdr_pathlen	= HPDSN_CSDR_PATH_LEN;
	csdr.csdr_L4pid		= proto->np_protocol;
	csdr.csdr_L4len		= HPDSN_CSDR_L4_LEN;
	csdr.csdr_L4servmap	= proto->np_service_map;
	csdr.csdr_L4addr	= inp->inp_lport;
	csdr.csdr_L3pid		= NSP_IP;
	csdr.csdr_L3len		= HPDSN_CSDR_L3_LEN;
	csdr.csdr_L3addr	= HPDSN_CSDR_IP_SAP;


	/* go through the AFINET interface address list, adding a csdr  */
	/* for each internet address on the list which has a route; pay */
	/* attention to whether or not loopback should be included      */
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
		if ((!loopback) && (ia->ia_ifp->if_flags & IFF_LOOPBACK))
			continue;
		if (!(ia->ia_flags & IFA_ROUTE))
			continue;
		csdr.csdr_ipaddr = IA_SIN(ia)->sin_addr.s_addr;
		m = path_addreport(&csdr,m);
		if (!m)
			break;
	}			

	return (m);

}  /* hpdsn_buildcsite */


hpdsn_getcsiteaddr(csdr, proto, addr)

u_short		    *csdr;  /* connect site domain report in contiguous memory*/
struct nipc_protosw *proto; /* specified protocol the caller wishes to reach  */
struct sockaddr     *addr;  /* address space to be filled in from csdr info   */

{
	u_short		*path;		/* pointer to path of domain report */
	u_short		*pelmt;		/* pointer to path element of path  */
	u_short		*csdr_end;	/* pointer to end of domain report  */
	u_short		*p_end;		/* pointer to end of path 	    */
	short		plen;		/* length of a path		    */
	short		pelen;		/* length of a path element         */
	int		proto_supp = 0; /* set if protocol found in csdr    */
	int		score;		/* score of address route	    */
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

	/* initialize the caller's sockaddr with the internet address */
	/* from the domain report 				      */
	addr_in->sin_family = AF_INET;
	addr_in->sin_port   = 0;
	addr_in->sin_addr.s_addr = (csdr[DR_ADDR1] << 16) + csdr[DR_ADDR2];
	addr_in->sin_addr.s_addr = htonl(addr_in->sin_addr.s_addr);

	/* loop through the path elements of the paths in this domain     */
	/* report searching for a path element for the specified protocol */

	csdr_end = csdr + csdr[DR_LEN]/2;  /* calculate end of domain report */
	for (path = &csdr[DR_PATH]; path <= csdr_end; path += plen+1) {
		plen = path[P_LEN]/2;
		p_end = path + plen;
		for (pelmt = &path[P_ELMT]; pelmt <= p_end; pelmt += pelen+1) {
			pelen = ((pelmt[PE_ID] & 0xff) + 1) >> 1;
			if ((pelmt[PE_ID] >> 8) == proto->np_protocol) { 
				proto_supp++;
				addr_in->sin_port = htonl(pelmt[pelen]);
				break;
			}
		}
		if (proto_supp)
			break;
	}

	/* if the protocol was found then return a score for the inet addr */
	if (!proto_supp)
		return PATH_NO_PATH;
	else {
		score = hpdsn_scoreaddr(addr_in);
		return (score);
	}

}  /* hpdsn_getcsiteaddr */


hpdsn_getnodeaddr(ndr, proto, addr)

u_short		    *ndr;   /* nodal domain report in contiguous memory      */
struct nipc_protosw *proto; /* specified protocol the caller wishes to reach */
struct sockaddr     *addr;  /* address space to be filled in from ndr info   */

{
	u_short		*path;		/* pointer to path of domain report */
	u_short		*pelmt;		/* pointer to path element of path  */
	u_short		*ndr_end;	/* pointer to end of domain report  */
	u_short		*p_end;		/* pointer to end of path 	    */
	short		plen;		/* length of a path		    */
	short		pelen;		/* length of a path element         */
	int		proto_supp = 0; /* set if protocol supported in dr  */
	struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

	/* loop through the path elements of the paths in this domain */
	/* report to verfiy that the specified protocol is supported  */

	ndr_end = ndr + ndr[DR_LEN]/2;	/* calculate end of domain report */
	for (path = &ndr[DR_PATH]; path <= ndr_end; path += plen+1) {
		plen = path[P_LEN]/2;
		p_end = path + plen;
		for (pelmt = &path[P_ELMT]; pelmt < p_end; pelmt += pelen+1) {
			pelen = ((pelmt[PE_ID] & 0xff) + 1) >> 1;
			if ((pelmt[PE_ID] >> 8) == NSP_TRANSPORT &&
			    (pelmt[PE_SERVICE] & proto->np_grp_service)) {
				proto_supp++;
				break;
			}
		}
		if (proto_supp)
			break;
	}

	/* if the protocol is supported initialize the caller's sockaddr */
	/* with the internet address from the domain report		 */
	if (!proto_supp)
		return PATH_NO_PATH;
	else {
		addr_in->sin_family = AF_INET;
		addr_in->sin_port   = 0;
		addr_in->sin_addr.s_addr = (ndr[DR_ADDR1] << 16) + ndr[DR_ADDR2];
		addr_in->sin_addr.s_addr = htonl(addr_in->sin_addr.s_addr);
	}

	/* return the score for the reachability of this internet address */
	return (hpdsn_scoreaddr(addr_in));

}  /* hpdsn_getnodeaddr */
				

struct mbuf *
hpdsn_nodetocsite(ndr, m, proto, protoaddr)

u_short			*ndr;	/* nodal domain report in contigous memory */
struct mbuf		*m;	/* mbuf to add csite domain report to      */
struct nipc_protosw	*proto; /* protocol info for the connect site	   */
char		*protoaddr;	/* protocol address of the connect site	   */

{
	u_short		*path;		/* pointer to path in ndr           */
	u_short		*pelmt;		/* pointer to path element in ndr   */
	u_short		*ndr_end;	/* pointer to end of ndr            */
	u_short		*p_end;		/* pointer to end of path in ndr    */
	short		plen;		/* length of ndr path		    */
	short		pelen;		/* length of ndr path element       */
	int		proto_supp = 0; /* set if protocol supported in ndr */
	struct csdr	csdr;		/* connect site domain reprot	    */

	/* loop through the path elements of the paths in the nodal domain */
	/* report to verfiy that the specified protocol is supported       */

	ndr_end = ndr + ndr[DR_LEN]/2;	/* calculate end of domain report */
	for (path = &ndr[DR_PATH]; path <= ndr_end; path += plen+1) {
		plen = path[P_LEN]/2;
		p_end = path + plen;
		for (pelmt = &path[P_ELMT]; pelmt < p_end; pelmt += pelen+1) {
			pelen = ((pelmt[PE_ID] & 0xff) + 1) >> 1;
			if ((pelmt[PE_ID] >> 8) == NSP_TRANSPORT &&
			    (pelmt[PE_SERVICE] & proto->np_grp_service)) {
				proto_supp++;
				break;
			}
		}
		if (proto_supp)
			break;
	}

	if (!proto_supp)
		return (m);

	/* build a connect site domain report from the input parameters */
	csdr.csdr_len		= HPDSN_CSDR_LEN;
	csdr.csdr_vers		= HPDSN_VER;
	csdr.csdr_domain	= HPDSN_DOM;
	csdr.csdr_ipaddr	= (ndr[DR_ADDR1] << 16) + ndr[DR_ADDR2];
	csdr.csdr_pathlen	= HPDSN_CSDR_PATH_LEN;
	csdr.csdr_L4pid		= proto->np_protocol;
	csdr.csdr_L4len		= HPDSN_CSDR_L4_LEN;
	csdr.csdr_L4servmap	= proto->np_service_map;
	csdr.csdr_L4addr	= *((u_short*) protoaddr);
	csdr.csdr_L3pid		= NSP_IP;
	csdr.csdr_L3len		= HPDSN_CSDR_L3_LEN;
	csdr.csdr_L3addr	= HPDSN_CSDR_IP_SAP;

	/* add the connect site domain report to the input mbuf chain */
	m = path_addreport(&csdr, m);

	return (m);

} /* hpdsn_nodetocsite */


hpdsn_reportvalid(dr)

u_short *dr;	/* pointer to domain report in contiguous memory */

{
	u_short		*path;		/* pointer to path of domain report */
	u_short		*pelmt;		/* pointer to path element of path  */
	u_short		*dr_end;	/* pointer to end of domain report  */
	u_short		*p_end;		/* pointer to end of path 	    */
	short		plen;		/* length of a path		    */
	short		pelen;		/* length of a path element         */

	/* loop through the path elements in each path of this domain     */
	/* report, verifying that the path element length does not exceed */
	/* the path length and that the path length does not exceed the   */
	/* domain report length 					  */

	dr_end = dr + dr[DR_LEN]/2;	/* calculate end of domain report */
	for (path = &dr[DR_PATH]; path <= dr_end; path += plen+1) {
		plen = path[P_LEN]/2;
		if (path + plen > dr_end)
			return E_PATHREPORT;
		p_end = path + plen;
		for (pelmt = &path[P_ELMT]; pelmt <= p_end; pelmt += pelen+1) {
			/* note: the element length can be odd and padded to */
			/* the even byte;  if this is the case we round up   */
			/* to the even byte before converting to 16-bit units*/
			pelen = ((pelmt[PE_ID] & 0xff) + 1) >> 1;
			if (pelmt + pelen > p_end)
				return E_PATHREPORT;
		}
	}

	return 0;

} /* hpdsn_reportvalid */


hpdsn_scoreaddr(addr)

struct sockaddr_in *addr;

{
	int	score = PATH_NO_PATH;/* score for the internet address    */
	struct in_ifaddr *ia;	/* AFINET interface address list     */	
	struct route iproute;		/* used to check for a route to addr */
	struct route *ro = &iproute;

	/* go through the AFINET interface address list checking for an  */
	/* address match, subnet match, or net match with the input addr */
	for (ia = in_ifaddr; ia; ia = ia->ia_next) {
		if (!(ia->ia_flags & IFA_ROUTE))
			continue;
		if (IA_SIN(ia)->sin_addr.s_addr == addr->sin_addr.s_addr) {
			score = PATH_THIS_HOST;
			break;  /* best case; stop searching! */
		}
		if (ia->ia_subnet == (addr->sin_addr.s_addr & ia->ia_subnetmask))
			score = PATH_LOCAL_SUBNET;
		else
			if ((ia->ia_net == (addr->sin_addr.s_addr & ia->ia_netmask))
			    && (score < PATH_LOCAL_NET))
				score = PATH_LOCAL_NET;
	}

	/* if we didn't score anything then see if we have */
	/* a route to the caller's internet address	   */
	if (score == PATH_NO_PATH) {
		bzero((caddr_t)ro, sizeof(struct route));
		ro->ro_dst.sa_family = AF_INET;
		((struct sockaddr_in *) &ro->ro_dst)->sin_addr = addr->sin_addr;
		rtalloc(ro);
		if (ro->ro_rt) {
			score = PATH_HAVE_ROUTE;
			rtfree(ro->ro_rt);
		}
	}

	return (score);

}  /* hpdsn_scoreaddr */
