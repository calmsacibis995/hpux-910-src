/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/netget.c,v $
 * $Revision: 1.22.83.3 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 16:27:56 $
 *
 */
/* 
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#if	defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) $Header: netget.c,v 1.22.83.3 93/09/17 16:27:56 root Exp $";
#endif


#include "../standard/inc.h"
#include "net.h"
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"


static int nipasfrags = 0;	/* number of ipasfrag structs read so far */
#ifdef	NS_MBUF_QA
int	ns_mbuf_qa_on = 0;
#endif	NS_MBUF_QA


/* nlist array of networking symbols */

struct nlist net_nl[] = {
#ifdef OLD_NNC
	{ "_mbutl"},		/*0*/
	{ "_Mbmap"},
	{ "_mbstat"},
	{ "_nmbclusters"},
	{ "_mfree"},
	{ "_mclfree"},
	{ "_mbmap"},
	{ "_mbuf_memory"},
	{ "_mastat"},
	{ "_mclrefcnt"},
	{ "_inetsw" },		/*10*/
	{ "_domains" },
        { "_ifnet" },
	{ "_lanift" },
	{ "_num_lan0" },
	{ "_xsap_head" },
	{ "_xsap_list_head" },
	{ "_sap_active" },
	{ "_ipqmaxlen" },
	{ "_ipstat" },
	{ "_lan0ift" },		/*20*/	/*RJ*/
	{ "_ipq" },
	{ "_icmpstat" },
	{ "_tcp_cb" },
	{ "_tcpstat" },
	{ "_xpb" },
	{ "_pxpstat" },
	{ "_udp_cb" },
	{ "_udpstat" },
	{ "_sbstat" },
	{ "_probestat" },	/*30*/
	{ "_pr_ntab" },
	{ "_pr_vnatab" },
	{ "_lan1ift" },		/*33*/		/* RAJ */
	{ "_num_lan1" },	/*34*/		/* RAJ */
#else  OLD_NNC
	{ "mbutl"},		/*0*/
	{ "Mbmap"},
	{ "mbstat"},
	{ "nmbclusters"},
	{ "mfree"},
	{ "mclfree"},
	{ "mbmap"},
	{ "mbuf_memory"},
	{ "mastat"},
	{ "mclrefcnt"},
	{ "inetsw" },		/*10*/
	{ "domains" },
        { "ifnet" },
	{ "lanift" },
	{ "num_lan0" },
	{ "xsap_head" },
	{ "xsap_list_head" },
	{ "sap_active" },
	{ "ipqmaxlen" },
	{ "ipstat" },
	{ "lan0ift" },		/*20*/		/* RJ */
	{ "ipq" },
	{ "icmpstat" },
	{ "tcp_cb" },
	{ "tcpstat" },
	{ "xpb" },
	{ "pxpstat" },
	{ "udp_cb" },
	{ "udpstat" },
	{ "sbstat" },
	{ "probestat" },	/*30*/
	{ "pr_ntab" },
	{ "pr_vnatab" },
	{ "lan1ift" },		/*33*/		/* RJ */
	{ "num_lan1" },		/*34*/		/* RJ */
#endif OLD_NNC
	{ "" }
};



/* net_nlist -- grab addresses of networking symbols from the kernel
 * text file 
 */
void net_nlist()
{
    struct nlist *np;
    for (np = net_nl; *np->n_name != '\0' ; np++)
        np->n_value = lookup(np->n_name);
    pty_nlist();
    mux_nlist();
}



/* netvread -- read n bytes of data from the kernel virtual address vaddr
 * into the buffer at buf.  Take care of page boundries as we go. Return
 * number of bytes read.
 */

int netvread (vaddr, buf, n)
uint vaddr;
char *buf;
int n;
{
    uint paddr, offset;
    register uint size, nread;
    register char *bp;

    size = n;
    bp = buf;
    offset = (vaddr & PGOFSET);

    while (size) {
        nread = ((size > (NBPG - offset)) ? NBPG - offset : size);
        paddr = (uint)getphyaddr(vaddr);
        /* NDBG(("cluster vaddr %#x = phys %#x", vaddr, paddr)); */
        if (!paddr) {
            NERROR("page %#x not mapped!", vaddr);
 	    return((int)(bp - buf));
	} else {
            lseek (fcore, (long)paddr, 0);
            if (read (fcore, bp, (int)nread) != nread) {
                NERROR("cannot read page at %#x", vaddr);
	        return((int)(bp - buf));
	    }
        } 
        offset = 0; /* we're page aligned now */
        vaddr += nread;
        bp += nread;
        size -= nread;
    }
    return((int)(bp - buf));
}



/* net_getstr -- gets the C string at the virtual location specified 
 * Stops on read errors, and substitutes unprintable characters
 * with '?'.
 * Needs to handle VM ###
 */

char *net_getstr(vaddr)
uint vaddr;
{
    static char buf[128];
    register char *sp, *ep;
    register int i;
    register uint paddr;

    sp = buf;
    *sp = '\0';
    ep = &buf[128];
    
    paddr = getphyaddr(vaddr);
    if (paddr == 0) {
        NERROR("attempt to read string from physical address 0x0", 0);
        goto error;
    }

    lseek(fcore, (long)clear(paddr), 0);
    while (sp < ep) {
        if (read(fcore, sp, sizeof (int)) != sizeof (int)) {
            NERROR("read of C string failed at address %#x", clear(paddr));
            goto error;
        }
        for (i = 0; i < sizeof(int); i++, sp++) {
            if (!*sp)
                return (buf);
            if ((*sp < ' ') || (*sp > '~'))
                *sp = '?';
        }
    }
    sp--;
    
error:
    *sp = '\0';
    return (buf);
}



/* netdata_read -- read "num" entries of data structure "index", starting
 * at the virtual address vaddr.  "index" is an X_INDEX into the netdata
 * array 
 */

void netdata_read (index, vaddr, num)
int  index, num;
uint vaddr;
{
    uint size;
    char *bp;

    if (NDNUM(index) >= NDMNUM(index)) {
        NERROR("tried to read %s beyond maximum buffer size", 
	    index <= X_LASTNLIST ? net_nl[index].n_name : "?");
        return;
    }

    bp = (char *)(LA(index, uint) + (NDNUM(index) * NDSIZEOF(index)));    
    size = num * NDSIZEOF(index);
        
    netvread(vaddr, bp, size);

    if ((num == 1) && (NDVLIST(index) == (uint *) 0))
        NDVLIST(index) = (uint *)net_malloc (sizeof (uint) * NDMNUM(index));

    if (NDVLIST(index) != (uint *) 0)
        NDVLIST(index)[NDNUM(index)] = vaddr;

    NDNUM(index) += num;
}



/* net_linkmbufdata -- instead of re-reading data from an mbuf, just
 * set the pointers so that this data structure's entry points to the
 * mbuf data.
 */

void net_linkmbufdata (index, vaddr, laddr)
int  index;
uint vaddr, laddr;
{
    if (NDNUM(index) >= NDMNUM(index)) {
        NERROR("tried to read %s beyond maximum buffer size", 
	    index <= X_LASTNLIST ? net_nl[index].n_name : "?");
        return;
    }

    /* link local address */
    if (NDLLIST(index) == (uint *) 0)
        NDLLIST(index) = (uint *)net_malloc (sizeof (uint) * NDMNUM(index));

    if (NDLLIST(index) != (uint *) 0)
        NDLLIST(index)[NDNUM(index)] = laddr;

    /* store virtual address */
    if (NDVLIST(index) == (uint *) 0)
        NDVLIST(index) = (uint *)net_malloc (sizeof (uint) * NDMNUM(index));

    if (NDVLIST(index) != (uint *) 0)
        NDVLIST(index)[NDNUM(index)] = vaddr;

    NDNUM(index) += 1;
}



/* netget_mbufs -- read in the networking mbuf memory, and fill in the
 * cluster and cluster free maps.
 */
 
void netget_mbufs ()
{
    register struct mbuf *m;
    register uint vaddr, n;

    /* Adjust mbufs/clusters to be on a cluster boundry */
#ifdef	NS_MBUF_QA
if (ns_mbuf_qa_on)
    NDSIZE(X_MBUTL) = (mbstat.m_clusters+mbstat.m_mbufs)*NETCLBYTES;
else
#endif	NS_MBUF_QA
    NDSIZE(X_MBUTL) = mbstat.m_clusters*NETCLBYTES + mbstat.m_mbufs*MSIZE;
#ifdef	NS_MBUF_QA
if (!ns_mbuf_qa_on)
    if (NDSIZE(X_MBUTL) > (NETCLBYTES * 3*NMBCLUSTERS/19)) {
	NERROR2(
         "mbstat indicates %#x clusters used, using NMBCLUSTERS(%#x) instead",
	    mbstat.m_clusters + mbstat.m_mbufs/NMBPCL, 3*NMBCLUSTERS/19);
	NDSIZE(X_MBUTL) = 3*NMBCLUSTERS/19 * NETCLBYTES;
    }
else
#endif	NS_MBUF_QA
    if (NDSIZE(X_MBUTL) > (NETCLBYTES * NMBCLUSTERS)) {
	NERROR2(
         "mbstat indicates %#x clusters used, using NMBCLUSTERS(%#x) instead",
	    mbstat.m_clusters + mbstat.m_mbufs/NMBPCL, NMBCLUSTERS);
	NDSIZE(X_MBUTL) = NMBCLUSTERS * NETCLBYTES;
    }
    netdata[X_MBUTL].nd_l = net_malloc (NDSIZE(X_MBUTL) + NETCLBYTES);
    netdata[X_MBUTL].nd_l = 
	(char *)((LA(X_MBUTL, uint) + NETCLBYTES) & ~NETCLOFSET);
#ifdef	NS_MBUF_QA
if (ns_mbuf_qa_on)
    NDNUM(X_MBUTL) = NDSIZE(X_MBUTL) / NETCLBYTES;
else
#endif	NS_MBUF_QA
    NDNUM(X_MBUTL) = NDSIZE(X_MBUTL) / MSIZE;

    /* read mbufs */    
    n = NDSIZE(X_MBUTL);
    if (netvread(VA(X_MBUTL, uint), LA(X_MBUTL, char *), n) != n) {
	NERROR("error reading mbuf memory", 0);
	return;
    }

    /* initialize cluster maps */
    memset(mclfreemap, 0, sizeof(mclfreemap));
    memset(mclmap, 0, sizeof(mclmap));
    vaddr = (uint)mclfree;
    while (vaddr) {
        if (got_sigint) break;    /* break out of loop on sigint */
        m = (struct mbuf *)vtol(X_MBUTL, vaddr);
        if (m == (struct mbuf *) -1) {
            NERROR("mbuf read: invalid address tracing mclfree list", 0);
            break;
        }
	mclfreemap[mtocl(vaddr)]++;
	mclmap[mtocl(vaddr)]++;	    /* also remember that this is a cluster */
        vaddr = (uint)m->m_next;
    }

    n = mbstat.m_mbufs / NMBPCL + mbstat.m_clusters;
    if (n >= NMBCLUSTERS)
	n = NMBCLUSTERS - 1;
#ifdef	NS_MBUF_QA
if (ns_mbuf_qa_on)  {
    n = mbstat.m_mbufs + mbstat.m_clusters;
    if (n >= NMBCLUSTERS)
	n = NMBCLUSTERS - 1;
}
else  {
    if (n >= 3*NMBCLUSTERS/19)
	n = 3*NMBCLUSTERS/19 - 1;
}
#endif	NS_MBUF_QA
    while (n--) {
        if (got_sigint) break;      /* break out of loop on sigint */
        if (mclrefcnt[n])
	    mclmap[n]++;
    }
}


        
void netget_protosw (dp)
struct domain *dp;
{
    struct protosw *pr;

    for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++) {
        if (got_sigint) break;    /* break out of loop on sigint */
        netdata_read (X_INETSW, pr, 1);
    }
}


void netget_domains ()
{
    struct domain *dp;
    uint vaddr;
    
    vaddr = get(PA(X_DOMAINS, uint));
    dp = domains;
    
    do {
        if (got_sigint) break;    /* break out of loop on sigint */
        netdata_read (X_DOMAINS, vaddr, 1);
        netget_protosw (dp);
        vaddr = (uint)dp->dom_next;
        dp++;
    } while (vaddr);
}



void netget_ifnet ()
{
    struct ifnet *ip;
    uint vaddr;
    
    vaddr = get(PA(X_IFNET, uint));
    ip = ifnet;
    
    do {
        if (got_sigint) break;    /* break out of loop on sigint */
        netdata_read (X_IFNET, vaddr, 1);
        vaddr = (uint)ip->if_next;
        ip++;
    } while (vaddr);
}



void netget_xsaps ()
{
    register struct xsap_link *xp;
    register uint vaddr;
    register int i;
    
    vaddr = (uint)xsap_list_head;
    xp = LA(X_XSAP_LIST, struct xsap_link *);
    i = 0;
    
    do {
        if (got_sigint) break;    /* break out of loop on sigint */
        netdata_read (X_XSAP_LIST, vaddr, 1);
        vaddr = (uint)xp->next;
	xp++;
	if (i++ == MAX_XSAPS) {
	    NERROR("too many xsaps, aborting read after %#x", MAX_XSAPS);
	    break;
	}
    } while (((struct xsap_link *)vaddr != xsap_head) && 
	(vaddr != 0));
}



void netget_ipasfrags (head, tail)
register struct ipasfrag *head, *tail;
{
    register struct ipasfrag *ipfp;
    register uint vaddr;

    vaddr = (uint) head;    
    ipfp = LA(X_IPASFRAG, struct ipasfrag *);
    ipfp = &ipfp[nipasfrags]; /* bump by amount we've read so far */

    do {
        if (got_sigint) break;    /* break out of loop on sigint */
        netdata_read (X_IPASFRAG, vaddr, 1);
        vaddr = (uint)ipfp->ipf_next;
	ipfp++;
	if (nipasfrags++ == MAX_IPASFRAGS) {
	    NERROR(
		"too many ipasfrags, aborting read after %#x", MAX_IPASFRAGS);
	    break;
	}
    } while ((vaddr != tail) && (vaddr != 0));
}



void netget_ipqs ()
{
    register struct ipq *ipqp;
    register uint vaddr, head;
    register int i;
    
    nipasfrags = 0;
    head = vaddr = net_nl[X_IPQ].n_value; 
    ipqp = LA(X_IPQ, struct ipq *);
    i = 0;

    do {
        if (got_sigint) break;    /* break out of loop on sigint */
        netdata_read (X_IPQ, vaddr, 1);
	if (ipqp->ipq_next)
	    netget_ipasfrags(ipqp->ipq_next, ipqp->ipq_prev);
        vaddr = (uint)ipqp->next;
	ipqp++;
	if (i++ == MAX_IPQS) {
	    NERROR("too many ipqs, aborting read after %#x", MAX_IPQS);
	    break;
	}
    } while ((vaddr != head) && (vaddr != 0));
}



void netget_sockets ()
{
    uint vaddr, i;
    struct mbuf *m;
    
    i = 0;
    while (i < NDNUM(X_MBUTL)) {
        if (got_sigint) break;    /* break out of loop on sigint */
        vaddr = ntov(X_MBUTL, i++);
	if (mclmap[mtocl(vaddr)])
	    continue;
	m = (struct mbuf *)vtol(X_MBUTL, vaddr);
	if (m->m_type == MT_SOCKET) 
	    net_linkmbufdata(X_SOCKET, vmtod(vaddr), mtod(m, uint));
    }
}



void netget_sockbufs ()
{
    uint vaddr, i;
    struct mbuf *m;
    
    i = 0;
    while (i < NDNUM(X_MBUTL)) {
        if (got_sigint) break;    /* break out of loop on sigint */
        vaddr = ntov(X_MBUTL, i++);
	if (mclmap[mtocl(vaddr)])
	    continue;
	m = (struct mbuf *)vtol(X_MBUTL, vaddr);
	if (m->m_type == MT_SOCKBUFS) {
	    net_linkmbufdata(X_SOCKBUF, vmtod(vaddr), mtod(m, uint));
	    net_linkmbufdata(X_SOCKBUF, vmtod(vaddr) + sizeof(struct sockbuf),
		mtod(m, uint) + sizeof(struct sockbuf));
	}
    }
}



netget_inpcbs()
{
    uint vaddr;
    struct inpcb *pcb, *pcbhead;
    struct tcpcb *tcp;
    struct pxpcb *pxp;

    nmmtcp = 0;

    netvread (VA(X_TCP_CB, uint), &tcp_cb, sizeof(struct inpcb));
    netvread (VA(X_PXP_CB, uint), &pxp_cb, sizeof(struct inpcb));
    netvread (VA(X_UDP_CB, uint), &udp_cb, sizeof(struct inpcb));

    /* read TCP control blocks */
    pcbhead = VA(X_TCP_CB, struct inpcb *);
    vaddr = (uint) pcbhead;
    do { 
        if (got_sigint) break;    /* break out of loop on sigint */
        pcb = (vaddr == (uint) pcbhead) ? &tcp_cb : 
	    (struct inpcb *) vtol(X_MBUTL, vaddr);
	if (pcb == -1) {
            NERROR("invalid address encountered in tcp_cb list: 0x%08x", 
                vaddr);
            break;
	}
        net_linkmbufdata(X_INPCB, vaddr, pcb);
	if (pcb->inp_ppcb) {
	    net_linkmbufdata(X_TCP_CB, pcb->inp_ppcb, 
		tcp = vtol(X_MBUTL, pcb->inp_ppcb));
	    if (tcp == (struct tcpcb *) -1)
                NERROR("invalid inp_ppcb address in tcp_cb list: 0x%08x",
                    tcp);
	    else if (tcp->t_mmtblp) {
		uint daddr;
		nmmtcp++;
		daddr = vmtod(tcp->t_mmtblp);
	        net_linkmbufdata(X_MSGMTBL, daddr, vtol(X_MBUTL, daddr));
	    }
	}
	vaddr = (uint) pcb->inp_next;
    } while (pcb->inp_next != pcbhead);

    /* read PXP control blocks */
    pcbhead = VA(X_PXP_CB, struct inpcb *);
    vaddr = (uint) pcbhead;
    do { 
        if (got_sigint) break;    /* break out of loop on sigint */
        pcb = (vaddr == (uint) pcbhead) ? &pxp_cb : 
	    (struct inpcb *) vtol(X_MBUTL, vaddr);
	if (pcb == -1) {
            NERROR("invalid address encountered in xpb list: 0x%08x", 
                vaddr);
            break;
	}
        net_linkmbufdata(X_INPCB, vaddr, pcb);
	if (pcb->inp_ppcb) {
	    pxp = vtol(X_MBUTL, pcb->inp_ppcb);
	    if (pxp == (struct pxpcb *) -1)
                NERROR("invalid inp_ppcb address in pxp_cb list: 0x%08x",pxp);
	    else
	        net_linkmbufdata(X_PXP_CB, pcb->inp_ppcb, pxp);
	}
	vaddr = (uint) pcb->inp_next;
    } while (pcb->inp_next != pcbhead);

    /* read UDP control blocks */
    pcbhead = VA(X_UDP_CB, struct inpcb *);
    vaddr = (uint) pcbhead;
    do { 
        if (got_sigint) break;    /* break out of loop on sigint */
        pcb = (vaddr == (uint) pcbhead) ? &udp_cb : 
	    (struct inpcb *) vtol(X_MBUTL, vaddr);
	if (pcb == -1) {
            NERROR("invalid address encountered in udp_cb list: 0x%08x", 
                vaddr);
            break;
	}
        net_linkmbufdata(X_INPCB, vaddr, pcb);
	vaddr = (uint) pcb->inp_next;
    } while (pcb->inp_next != pcbhead);
}



netdata_getaddrs(xindx)
register int xindx;
{
    register uint a1, v1;
        
    v1 = netdata[xindx].nd_v = net_nl[xindx].n_value;
    a1 = netdata[xindx].nd_p = getphyaddr((uint)netdata[xindx].nd_v);
}



void net_gettables()    
{
    register int i;
    int nsenable, bsdenable;
    
    /* Reset all indices, since we may be re-reading with ":snap" */
    for (i = X_FIRSTNET; i <= X_LASTNET; i++)
        NDNUM(i) = 0;

#ifndef OLD_NS_CONFIG
    nsenable = lookup("ifnet");
    if (!nsenable)
	return;
#endif OLD_NS_CONFIG

    /* read counters, pointers, etc. */
    mfree = (struct mbuf *)get(net_nl[X_MFREE].n_value); 
    mclfree = (struct mbuf *)get(net_nl[X_MCLFREE].n_value); 
    mbuf_memory = (struct mbuf *)get(net_nl[X_MBUF_MEMORY].n_value); 
    num_lan0 = get(net_nl[X_NUM_LAN0].n_value); 
    num_lan1 = get(net_nl[X_NUM_LAN1].n_value); 
    xsap_head = (struct xsap_link *)net_nl[X_XSAP_HEAD].n_value; 
    xsap_list_head = 
	(struct xsap_link *)get(net_nl[X_XSAP_LIST].n_value); 
    ipqmaxlen = get(net_nl[X_IPQMAXLEN].n_value); 

#ifdef TRASH_NET
    mfree = mclfree = mbuf_memory = nmbclusters = num_lan0 = num_lan1 = xsap_head = 
	xsap_list_head = ipqmaxlen = TRASH_VAL;
#endif TRASH_NET

    if ((uint)nmbclusters > NMBCLUSTERS) {
        NERROR2(
	    "nmbclusters is %#x, using default of %#x instead", nmbclusters,
	    NMBCLUSTERS);
        nmbclusters = NMBCLUSTERS;
    }

    if ((uint)num_lan0 > MAX_NUM_LAN0) {
	NERROR2("num_lan0 is %#x, using default of %#x instead", num_lan0,
	    DEFAULT_NUM_LAN0);
        num_lan0 = DEFAULT_NUM_LAN0;
    }

    /* Read networking data structures, IMPORTANT: we must read mbstat
     * before we call netget_mbufs(), because that routine uses the 
     * values in mbstat to determine how much networking memory there
     * is.
     */
    netdata_read (X_MCLREFCNT, VA(X_MCLREFCNT, char *), NMBCLUSTERS);
						        /* mclrefcnt	*/
    netdata_read (X_MASTAT, VA(X_MASTAT, char *), 1);	/* mastat	*/
    netdata_read (X_MBSTAT, VA(X_MBSTAT, char *), 1);	/* mbstat	*/
    netdata_read (X_MBMAP,  VA(X_MBMAP,  char *),
	NMBCLUSTERS/2);					/* mbuf ptes    */
    netdata_read (X_mBMAP,  VA(X_mBMAP,  char *),
	nmbclusters/4);					/* resource map */
/*** as of 3.0, CIO and NIO LAN cards cannot operate within the
 *** same kernel so either num_lan0 or num_lan1 will be zero ***/
    netdata_read (X_LANIFT, VA(X_LANIFT, char *), (num_lan0+num_lan1)); 
    netdata_read (X_LAN0IFT, VA(X_LAN0IFT, char *), num_lan0); 
    netdata_read (X_LAN1IFT, VA(X_LAN1IFT, char *), num_lan1); 
    netdata_read (X_SAP_ACTIVE, VA(X_SAP_ACTIVE, char *), NSAPS); 
							/* sap_active	*/
    netdata_read (X_IPSTAT, VA(X_IPSTAT, char *), 1);   /* ipstat	*/
    netdata_read (X_ICMPSTAT, VA(X_ICMPSTAT, char *), 1); 
							/* icmpstat	*/
    netdata_read (X_TCPSTAT, VA(X_TCPSTAT, char *), 1); /* tcpstat	*/
    netdata_read (X_PXPSTAT, VA(X_PXPSTAT, char *), 1); /* pxpstat	*/
    netdata_read (X_SBSTAT, VA(X_SBSTAT, char *), 2);   /* sbstat	*/
    netdata_read (X_PROBESTAT, VA(X_PROBESTAT, char *), 1); 
							/* probestat	*/
    netdata_read (X_PR_NTAB, VA(X_PR_NTAB, char *), NTAB_SIZE); 
							/* pr_ntab	*/
    netdata_read (X_PR_VNATAB, VA(X_PR_VNATAB, char *), VNATAB_SIZE);   
							/* pr_vnatab	*/

#ifdef TRASH_NET
    for (i = X_FIRSTNET; i <= X_LASTNET; i++) {
	fprintf(outf,"setting %d to trash, size=%d\n", i,
		NDNUM(i) * NDSIZEOF(i));
	if (LA(i, char *) != NULL) 
    	    memset(LA(i, char *), TRASH_VAL, (NDMNUM(i) * NDSIZEOF(i)));
	else if (NDVLIST(i) != NULL) 
    	    memset(NDVLIST(i), TRASH_VAL, (NDMNUM(i) * sizeof(uint)));
    }
#endif TRASH_NET
    netget_mbufs ();
#ifdef TRASH_NET
    memset(LA(X_MBUTL,char *),TRASH_VAL,(NDMNUM(X_MBUTL)*NDSIZEOF(X_MBUTL)));
#endif TRASH_NET
#ifdef OLD_NNC
    netget_domains ();
    netget_ifnet ();
    netget_xsaps ();
    netget_ipqs ();
    netget_inpcbs();
#else  OLD_NNC
#ifdef OLD_NS_CONFIG 
    if (nsenable = lookup("rel1nsc_1_flag"))
        nsenable = get(nsenable);
    if (bsdenable = lookup("rel1nsc_2_flag"))
        bsdenable = get(bsdenable);
#endif OLD_NS_CONFIG
    if (nsenable || bsdenable) {
        netget_domains ();
        netget_ifnet ();
        netget_xsaps ();
        netget_ipqs ();
        netget_inpcbs();
    }
#endif OLD_NNC
    netget_sockets ();
#ifndef NEWBM
    netget_sockbufs ();
#endif  NEWBM

#ifdef TRASH_NET
    /* trash all of networking memory to check address dereferencing */
    for (i = X_FIRSTNET; i <= X_LASTNET; i++) {
	fprintf(outf,"setting %d to trash, size=%d\n", i,
		NDNUM(i) * NDSIZEOF(i));
	if (LA(i, char *) != NULL) 
    	    memset(LA(i, char *), TRASH_VAL, (NDMNUM(i) * NDSIZEOF(i)));
	else if (NDVLIST(i) != NULL) 
    	    memset(NDVLIST(i), TRASH_VAL, (NDMNUM(i) * sizeof(uint)));
    }
#endif TRASH_NET
}



/* netdata_init -- intialize the fields of an entry in the netdata
 * array.  index is the X_INDEX into the array; sizeofdata is the
 * size of the data structure that this entry refers to, (sizeof 
 * struct protosw), etc.; maxnum is the total number of these data
 * structures which we can handle; and alloc is -1, 0, or a local
 * address: -1 says allocate data for maxnum entries, 0 says do nothing,
 * otherwise it's a pointer to a buffer used to store up to maxnum
 * entries in.
 */

#define NDI_ALLOC	-1
#define NDI_NOALLOC	0

static void netdata_init(index, sizeofdata, maxnum, alloc)
register int index;
int sizeofdata, maxnum, alloc;
{
    NDSIZE(index)   = sizeofdata * maxnum;
    NDNUM(index)    = 0;
    NDMNUM(index)   = maxnum;
    NDSIZEOF(index) = sizeofdata;
    switch (alloc) {
	case NDI_ALLOC:
	    /* we need to allocate a buffer for the data */
	    netdata[index].nd_l = net_malloc (NDSIZE(index));
	    break;
	case NDI_NOALLOC:
	    /* do nothing */
	    netdata[index].nd_l = (char *) NULL;
	    break;
	default:
	    /* alloc is a pointer to a local buffer, use that buffer */
	    netdata[index].nd_l = (char *)alloc;
	    break;
    }
}



void net_getaddrs()	
{
    register int i;
    int nsenable;
    
#ifndef OLD_NS_CONFIG
    nsenable = lookup("ifnet");
    if (!nsenable)
	return;
#endif OLD_NS_CONFIG
    
    /* Get some networking stats and space */
    nmbclusters = (int)get(net_nl[X_NMBCLUSTER].n_value); 
    num_lan0 = get(net_nl[X_NUM_LAN0].n_value); 
    num_lan1 = get(net_nl[X_NUM_LAN1].n_value); 

#ifdef TRASH_NET
    nmbclusters = num_lan0 = num_lan1 = TRASH_VAL;
#endif TRASH_NET

#ifdef	NS_MBUF_QA
    if ((uint)nmbclusters > 3*NMBCLUSTERS/19)
	ns_mbuf_qa_on++;
#endif	NS_MBUF_QA
    if ((uint)nmbclusters > NMBCLUSTERS) {
        NERROR2(
	    "nmbclusters is %#x, using default of %#x instead", nmbclusters,
	    NMBCLUSTERS);
        nmbclusters = NMBCLUSTERS;
    }

/*** as of 3.0, CIO & NIO LAN cards cannot operate within the same
 *** box so either num_lan0 or num_lan1 will be zero, so here we
 *** will use the same default and maximum ***/
    if ((uint)num_lan0 > MAX_NUM_LAN0) {
	NERROR2("num_lan0 is %#x, using default of %#x instead", num_lan0,
	    DEFAULT_NUM_LAN0);
        num_lan0 = DEFAULT_NUM_LAN0;
    }

    if ((uint)num_lan1 > MAX_NUM_LAN0) {
	NERROR2("num_lan1 is %#x, using default of %#x instead", num_lan1,
	    DEFAULT_NUM_LAN0);
        num_lan1 = DEFAULT_NUM_LAN0;
    }

    /* Get virtual and physical addresses of network symbols */
    for (i = X_FIRSTNET; i <= X_LASTNET; i++)
        netdata_getaddrs(i);

#ifdef	NS_MBUF_QA
    if ( !ns_mbuf_qa_on )
    netdata_init(X_MCLREFCNT, sizeof (char), 3*NMBCLUSTERS/19, mclrefcnt);
    else
#endif	NS_MBUF_QA
    netdata_init(X_MCLREFCNT, sizeof (char), NMBCLUSTERS, mclrefcnt);
    netdata_init(X_MASTAT, sizeof (struct mastat), 1, &mastat);
    netdata_init(X_MBSTAT, sizeof (struct mbstat), 1, &mbstat);
    netdata_init(X_INETSW, sizeof (struct protosw), MAX_INETSW, NDI_ALLOC);
    netdata_init(X_DOMAINS, sizeof (struct domain), MAX_DOMAINS, NDI_ALLOC);
    netdata_init(X_IFNET, sizeof (struct ifnet), MAX_IFNETS, NDI_ALLOC);
#ifdef	NS_MBUF_QA
    if ( ns_mbuf_qa_on )
    netdata_init(X_MBUTL, NETCLBYTES, 1, NDI_NOALLOC);
    else
#endif	NS_MBUF_QA
    netdata_init(X_MBUTL, sizeof (struct mbuf), 1, NDI_NOALLOC);
#ifdef	NS_MBUF_QA
    if ( !ns_mbuf_qa_on )  {
    netdata_init(X_mBMAP, sizeof (struct map), 3*NMBCLUSTERS/76, NDI_ALLOC);
    netdata_init(X_MBMAP, sizeof (struct pte), 3*NMBCLUSTERS/38, NDI_ALLOC);
    }  else  {
#endif	NS_MBUF_QA
    netdata_init(X_mBMAP, sizeof (struct map), NMBCLUSTERS/4, NDI_ALLOC);
    netdata_init(X_MBMAP, sizeof (struct pte), NMBCLUSTERS/2, NDI_ALLOC);
#ifdef	NS_MBUF_QA
    }
#endif	NS_MBUF_QA
    netdata_init(X_LANIFT, sizeof (lan_ift), (num_lan0 + num_lan1), NDI_ALLOC);
    netdata_init(X_LAN0IFT, sizeof (lan0_ift), num_lan0, NDI_ALLOC);
    netdata_init(X_LAN1IFT, sizeof (lan1_ift_t), num_lan1, NDI_ALLOC);
    netdata_init(X_XSAP_LIST, sizeof(struct xsap_link), MAX_XSAPS, NDI_ALLOC);
    netdata_init(X_SAP_ACTIVE, sizeof (struct sap_input), NSAPS, sap_active);
    netdata_init(X_IPSTAT, sizeof (struct ipstat), 1, &ipstat);
    netdata_init(X_IPQ, sizeof (struct ipq), MAX_IPQS, NDI_ALLOC);
    netdata_init(X_IPASFRAG,sizeof (struct ipasfrag),MAX_IPASFRAGS,NDI_ALLOC);
    netdata_init(X_ICMPSTAT, sizeof (struct icmpstat), 1, &icmpstat);
    netdata_init(X_TCPSTAT, sizeof (struct tcpstat), 1, &tcpstat);
    netdata_init(X_TCP_CB, sizeof (struct inpcb), MAX_SOCKETS, NDI_ALLOC);
    netdata_init(X_MSGMTBL, sizeof(struct msgmtbl), MAX_SOCKETS, NDI_NOALLOC);
    netdata_init(X_PXPSTAT, sizeof (struct pxpstat), 1, &pxpstat);
    netdata_init(X_PXP_CB, sizeof (struct inpcb), MAX_SOCKETS, NDI_ALLOC);
    netdata_init(X_UDPSTAT, sizeof (struct udpstat), 1, &udpstat);
    netdata_init(X_UDP_CB, sizeof (struct inpcb), 1, &udp_cb);
    netdata_init(X_INPCB, sizeof (struct inpcb), MAX_SOCKETS, NDI_NOALLOC);
    netdata_init(X_SOCKET, sizeof (struct socket), MAX_SOCKETS, NDI_NOALLOC);
#ifndef NEWBM
    netdata_init(X_SOCKBUF, sizeof(struct sockbuf),MAX_SOCKBUFS, NDI_NOALLOC);
#endif  NEWBM
    netdata_init(X_SBSTAT, sizeof (struct sbstat), 2, sbstat);
    netdata_init(X_PROBESTAT, sizeof (struct probestat), 1, &probestat);
    netdata_init(X_PR_NTAB, sizeof (struct ntab), NTAB_SIZE, NDI_ALLOC);
    netdata_init(X_PR_VNATAB, sizeof (struct vnatab), VNATAB_SIZE, NDI_ALLOC);

    /* Set up some data structures */
    domains = LA(X_DOMAINS, struct domain *);
    ifnet = LA(X_IFNET, struct ifnet *);
    lanift = LA(X_LANIFT, lan_ift *);
    lan0ift = LA(X_LAN0IFT, lan0_ift *);	/*RJ*/
    lan1ift = LA(X_LAN1IFT, lan1_ift_t *);	/*RJ*/
    ipq = LA(X_IPQ, struct ipq *);

    pty_getaddrs();
    mux_getaddrs();
}

