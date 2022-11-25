/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/net.c,v $
 * $Revision: 1.16.83.3 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 16:27:49 $
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
static char rcsid[]="@(#) $Header: net.c,v 1.16.83.3 93/09/17 16:27:49 root Exp $";
#endif


#include "../standard/inc.h"
#include "net.h"
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"

/* Networking data structures */
int     nmbclusters;
struct	mbuf    *mfree, 
		*mclfree, 
		*mbuf_memory;
struct	mastat	mastat;
struct	mbstat	mbstat;
struct  domain  *domains;
struct  ifnet   *ifnet;
char    mclrefcnt[NMBCLUSTERS];
struct  lan_ift *lanift;
struct  lan0_ift *lan0ift;
struct  lan1_ift_t *lan1ift;
int     num_lan0;
int     num_lan1;		/*RJ*/
struct  xsap_link *xsap_head;
struct  xsap_link *xsap_list_head;
struct  sap_input sap_active[NSAPS];
int	ipqmaxlen;
struct	ipstat	ipstat;
struct	ipq *ipq;
struct	icmpstat icmpstat;
struct	inpcb tcp_cb;		/* head of queue of active tcpcb's */
struct	tcpstat tcpstat;	/* tcp statistics */
struct	inpcb pxp_cb;		/* head of queue of active pxpcb's */
struct	pxpstat pxpstat;	/* pxp statistics */
struct	inpcb udp_cb;		/* head of queue of active udpcb's */
struct	udpstat udpstat;	/* udp statistics */
struct	sbstat	sbstat[2];
struct	probestat probestat;



/* analyze data structures */

int	nflg = 0;
int	socket_conn = 0;
int	nmmtcp = 0;			    /* number of tcp cbs with 
					       msg mode */
struct	neterr	neterr = { -1, -1};
char    mclmap[NMBCLUSTERS];		    /* "mbuf is a cluster" map  */
char    mclfreemap[NMBCLUSTERS];	    /* cluster is free map	*/

struct netdata netdata[X_LASTNET + 1];

struct nlookup_entry nlookup[] = {
/* MT_FREE	*/	{"MT_FREE",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_DATA	*/	{"MT_DATA",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_HEADER	*/	{"MT_HEADER",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_SOCKET	*/	{"MT_SOCKET",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	netdump_socket},
/* MT_PCB	*/	{"MT_PCB",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	netdump_pcb},
/* MT_RTABLE	*/	{"MT_RTABLE",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_HTABLE	*/	{"MT_HTABLE",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_ATABLE	*/	{"MT_ATABLE",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_SONAME	*/	{"MT_SONAME",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_ZOMBIE	*/	{"MT_ZOMBIE",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_SOOPTS	*/	{"MT_SOOPTS",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_FTABLE	*/	{"MT_FTABLE",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_MACCT	*/	{"MT_MACCT",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	netdump_macct},
/* MT_OPT	*/	{"MT_OPT",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_NETUNAM   */	{"MT_NETUNAM",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_RINODE	*/	{"MT_RINODE",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_PRCMEM	*/	{"MT_PRCMEM",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_PATH_REPORT  */	{"MT_PATH_REPORT",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* MT_MSG_MODE_TBL */	{"MT_MSG_MODE_TBL",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	netdump_msgmtbl},
/* MT_NAME	*/	{"MT_NAME",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
#ifdef NEWBM
/* MT_SOCKBUFS are gone in the new buffer manager */
/* MT_SOCKBUFS	*/	{"MT_SOCKBUFS",	NT_ANY,	-1, -1,
			netverify_nop,	netdump_sockbuf},
#else  NEWBM
/* MT_SOCKBUFS	*/	{"MT_SOCKBUFS",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	netdump_sockbuf},
#endif NEWBM
/* MT_PROTOARGS	*/	{"MT_PROTOARGS",NT_ANY,	X_MBUTL, -1,
			netverify_nop,	n_dump_nop},
/* NT_MBUF	*/	{"mbuf",	NT_ANY,	X_MBUTL, -1,
			netverify_mbuf,	netdump_mbuf},
/* NT_CLUSTER	*/	{"cluster",	NT_ANY,	X_MBUTL, -1,
			netverify_nop,	netdump_cluster},
/* NT_NONE	*/	{"?",		NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_ANY	*/	{"?",		NT_NONE, -1, -1, 
			netverify_nop,	n_dump_nop},
/* NT_IFNET	*/	{"ifnet",	NT_NONE, X_IFNET,  -1, 
			netverify_ifnet,    netdump_ifnet},
/* NT_LANIFT	*/	{"lan_ift",	NT_NONE, X_LANIFT, -1,
			netverify_lanift,   netdump_lanift},
/* NT_SAPINPUT	*/	{"sap_input",	NT_NONE, X_SAP_ACTIVE, -1,
			netverify_sapinput, netdump_sapinput},
/* NT_XSAPLINK	*/	{"xsap_link",	NT_NONE, X_XSAP_LIST, -1,
			netverify_xsaplink, netdump_xsaplink},
/* NT_PRNTAB	*/	{"pr_ntab",	NT_NONE, X_PR_NTAB, -1,
			netverify_prntab, netdump_prntab},
/* NT_PRVNATAB	*/	{"pr_vnatab",	NT_NONE, X_PR_VNATAB, -1,
			netverify_prvnatab, netdump_prvnatab},
/* NT_ROUTE	*/	{"route",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_RTENTRY	*/	{"rtentry",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_LAN0IFT	*/	{"lan0ift",	NT_NONE, X_LAN0IFT, -1,
			netverify_nop,  netdump_lan0ift},  /*RJ*/
/* NT_IPQ	*/	{"ipq",		NT_NONE, X_IPQ, -1,
			netverify_ipq,  netdump_ipq},
/* NT_IPASFRAG	*/	{"ipasfrag",	NT_NONE, X_IPASFRAG, -1,
			netverify_ipasfrag, netdump_ipasfrag},
/* NT_INPCB	*/	{"inpcb",	NT_NONE, X_INPCB, -1,
			netverify_inpcb, netdump_inpcb},
/* NT_TCPCB	*/	{"tcpcb",	NT_NONE, X_TCP_CB, -1,
			netverify_tcpcb, netdump_tcpcb},
/* NT_MSGMTBL	*/	{"msgmtbl",	NT_NONE, X_MSGMTBL, -1,
			netverify_nop,	netdump_msgmtbl},
/* NT_PXPCB	*/	{"pxpcb",	NT_NONE, X_PXP_CB, -1,
 			netverify_nop,	netdump_pxpcb},
/* NT_RAWCB	*/	{"rawcb",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_RAW8023CB	*/	{"raw8023cb",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_DOMAIN	*/	{"domain",	NT_NONE, X_DOMAINS, -1, 
			netverify_nop,	netdump_domain},
/* NT_PROTOSW	*/	{"protosw",	NT_NONE, X_INETSW, -1, 
			netverify_nop,	netdump_protosw},
/* NT_SOCKET	*/	{"socket",	NT_NONE, X_SOCKET, -1,
			netverify_socket,  netdump_socket},
#ifdef NEWBM
/* MT_SOCKBUFS are gone in the new buffer manager */
/* NT_SOCKBUF	*/	{"@sockbuf@",	NT_NONE, -1, -1,
			netverify_sockbuf, netdump_sockbuf},
#else  NEWBM
/* NT_SOCKBUF	*/	{"sockbuf",	NT_NONE, X_SOCKBUF, -1,
			netverify_nop,	netdump_sockbuf},
#endif NEWBM
/* NT_NAMERECORD*/	{"namerecord",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_RFAUSER	*/	{"rfauser",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_RFAINODE	*/	{"rfainode",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_RFAPROCMEM*/	{"rfaprocmem",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_NETUNAM	*/	{"netunam",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
/* NT_LAN1IFT	*/	{"lan1ift",	NT_NONE, X_LAN1IFT, -1,
			netverify_nop,  netdump_lan1ift}, /*RJ*/
#ifdef OLD_
/* dummy entry for "conn": */
/* NT_CONN	*/	{"conn",	NT_NONE, -1, -1,
			netverify_nop,	n_dump_nop},
#endif
};

struct netopt netopts[] = {
    { 0, "mdump"    }, 
    { 0, "mfollow"  },
    { 0, "inuse"    },
    { 0, 0	    }
};



net_error (errtyp, errmsg, errval, errval2, errval3)
register int errtyp, errval, errval2, errval3;
char *errmsg;
{
    switch(errtyp) {
        case NE_FATAL:
            fprintf(outf, "fatal error: ");
            break;
        case NE_ERROR:
            fprintf(outf, "error: ");
            break;
        case NE_WARNING:
            fprintf(outf, "warning: ");
            break;
    }

    if (neterr.ntyp != -1) {
        fprintf (outf, "%s", 
            ((neterr.ntyp < 0) || (neterr.ntyp > NT_LASTONE)) ? "?" :
            nlookup[neterr.ntyp].nl_name);
        if (neterr.vaddr != -1) 
            fprintf (outf, " 0x%08x", neterr.vaddr);
        fprintf (outf, ", ");
    }
    fprintf(outf, errmsg, errval, errval2, errval3);
    fprintf(outf, "\n");

    switch(errtyp) {
        case NE_FATAL:
            exit(1);
    }
}



/* Address manipulation routines */

/* vton -- given a virtual address, return a type, such as 
 * NT_MBUF.  Returns -1 if the virtual address is illegal.
 */

int vton(vaddr)
register uint vaddr;
{
    register int i, j, k;
    
    /* cycle through nlookup table, looking for the data which
     * contains vaddr.  Go backwards so we hit the mbufs last, 
     * since some data (such as sockets) are in the mbuf area.
     */
    /* first a sanity check, no net vaddr can ever be 0 */
    if (vaddr == (uint) 0)
	return(-1);

    for (k = NT_LASTONE; k >= NT_MBUF; k--) {
	if ((i = nlookup[k].nl_xindx) == -1)
	    continue; /* there is no data associated with this entry */
	if (NDVLIST(i) == (uint *)0) {
	    /* this entry was contiguous in networking memory */
            if ((vaddr >= VA(i, uint)) &&
                (vaddr < (VA(i, uint) + NDSIZE(i)))) {
		    if ((k == NT_MBUF) && mclmap[mtocl(vaddr)])
			return (NT_CLUSTER);	/* this is a cluster */
		    if ((k == NT_CLUSTER) && !mclmap[mtocl(vaddr)])
			return (NT_MBUF);	/* this is an mbuf   */
                    return (k);
		}
	} else {
	    /* compare vaddr with the vaddr of each data structure */
            for (j = 0; j < NDNUM(i); j++)
                if (vaddr == NDVLIST(i)[j])
                    return(k);
        }
    }
    NDBG(("vton(%#x), vaddr is invalid", vaddr));
    return (-1);
}



/* ntvton -- see vton
 * Returns -1 if the virtual address is illegal.
 */

int ntvton(ntyp, vaddr)
int ntyp;
register uint vaddr;
{
    register int i, j;
    
    /* first a sanity check, no net vaddr can ever be 0 */
    if (vaddr == (uint) 0)
	return(-1);

    if ((i = nlookup[ntyp].nl_xindx) == -1)
        return (-1); /* there is no data associated with this entry */

    if (NDVLIST(i) == (uint *)0) {
        /* this entry was contiguous in networking memory */
        if ((vaddr >= VA(i, uint)) &&
            (vaddr < (VA(i, uint) + NDSIZE(i)))) {
                if ((ntyp == NT_MBUF) && mclmap[mtocl(vaddr)])
		    return (NT_CLUSTER);	/* this is a cluster */
	        if ((ntyp == NT_CLUSTER) && !mclmap[mtocl(vaddr)])
		    return (NT_MBUF);	/* this is an mbuf   */
                return (ntyp);
	}
    } else {
        /* compare vaddr with the vaddr of each data structure */
        for (j = 0; j < NDNUM(i); j++)
            if (vaddr == NDVLIST(i)[j])
                return(ntyp);
    }
    NDBG(("ntvton(%#x), vaddr is invalid", vaddr));
    return (-1);
}



/* ntov -- given a type, such as NT_MBUF, and an index, return
 * the virtual address of the data structure.  For example:
 * ntov(NT_MBUF, 0) would return the virtual address of the first
 * mbuf.  Returns -1 if the index or type is invalid, or out of range.
 */

uint ntov (xindx, index)
register int xindx, index;
{
    NDBG(("ntov(%#x,%#x)", xindx, index));
    
    if ((xindx < 0) || (xindx > X_LASTNET)) {
	NERROR("nlist index %d, out of range", xindx);
	return(-1);
    }

    if ((index < 0) || (index >= NDNUM(xindx))) {
	NERROR("index out of range", 0);
	return(-1);
    }

    if (NDVLIST(xindx) != (uint *) 0)
	return (NDVLIST(xindx)[index]);
    else
	return (VA(xindx, uint) + (index * NDSIZEOF(xindx)));
}



/* vtol -- given a virtual address of type "xindx", return the local
 * (in memory) address of this data structure.
 */

uint vtol (xindx, vaddr)
register int  xindx;
register uint vaddr;
{
    register uint i;
    
    if ((xindx < 0) || (xindx > X_LASTNET)) {
	NERROR("nlist index %d, out of range", xindx);
	return(-1);
    }

    /* an optimization */
    if (xindx == X_MBUTL) {
        if (!NTVALIDV(NT_MBUF, vaddr))
	    return(-1);
    } else if (!NVALIDV(vaddr)) 
	return(-1);

    if (NDVLIST(xindx) != (uint *) 0) {
        for (i = 0; i < NDNUM(xindx); i++)
	    if (NDVLIST(xindx)[i] == vaddr) {
		if (NDLLIST(xindx) != (uint *) 0)
		    /* this is a link to an mbuf data area */
		    return (NDLLIST(xindx)[i]);
	        else
		    return (LA(xindx, uint) + (NDSIZEOF(xindx) * i));
	    }
	return (-1); /* not found */
    } 
    else {
	if (vaddr < VA(xindx, uint))
		return (-1);
	if (vaddr >= (VA(xindx, uint) + (NDNUM(xindx) * NDSIZEOF(xindx))))
		return (-1);
	return (LA(xindx, uint) + (vaddr - VA(xindx, uint)));
    }
}



/* vmtod -- emulation of mtod() macro for virtual addresses.  See h/mbuf.h
 * for a definition of what it's supposed to do.  Returns zero on error.
 */

vmtod(vaddr)
register uint vaddr;
{
    register struct mbuf *m;
    
    if ((ntvton(NT_MBUF, vaddr) != NT_MBUF) || (vaddr & (MSIZE-1))) 
	return(0); /* silently choke */
    m = (struct mbuf*) vtol(X_MBUTL, vaddr);
    return (VA(X_MBUTL, uint) + (mtod(m, uint) - LA(X_MBUTL, uint)));
}



char *addr_to_sym(addr)
uint addr;
{
    uint diff = findsym(addr);
    static char symstr[128];

    if (diff > 0x0fffffff)
        return ("?");
    sprintf(symstr,"%s+0x%04x", cursym, diff);
    return (symstr);
}



char *net_malloc (size)
uint size;
{
    register char *p;
    extern char *malloc(); 

    NDBG(("netmalloc(%#x)", size));
    
    p = malloc (size);
    if (p == (char *) 0) {
        perror ("out of memory");
        exit(1);
    }
    return (p);
}



/* net_inuse() - given a structure of type "ntyp" at virtual address
 * "vaddr", return non-zero if it's active.  This should really be table
 * driven like everything else.
 */

net_inuse(ntyp, vaddr)
int ntyp;
uint vaddr;
{
    uint laddr;

    if (!netopts[NETO_INUSE].val)
	return (1);

    laddr = vtol(nlookup[ntyp].nl_xindx, vaddr);
    if (laddr != -1) {
        switch (ntyp) {
	    case NT_PRVNATAB:
		return(((struct vnatab *)laddr)->vt_flags);
	    case NT_PRNTAB:
		return(((struct ntab *)laddr)->nt_flags);
	}
    }

    return (1);
}



/* naming routines */

char *netdata_ntypname (ntyp)
register int ntyp;
{
    NDBG(("netdata_ntypname(%#x)", ntyp));
    
    if ((ntyp < 0) || (ntyp > NT_LASTONE)) {
	NERROR("unknown type %d", ntyp);
	return("?");
    }
    return (nlookup[ntyp].nl_name);
}



/* strsubcmp - compare two strings, return length of match if s1 is a 
 * subset of s2, else return 0.
 */

static int strsubcmp(s1, s2)
register char *s1, *s2;
{
    register int len = 0;
    while (*s1) {
	if ((*s1 != '_') && (*s2 == '_'))   
	    s2++;			/* ignore implicit _ in names	    */
	if (*s1++ != *s2++) {
	    if (*s2)
		return (0);		/* This wasn't a proper sub-string  */
	    else
		return (len);
	}
	len++;
    }
    return (len);
}

/* nametont -- given the name of a data structure, return its type, such 
 * as NT_MBUF.  Returns -1 if there is no match.
 */

nametont (name)
char *name;
{
    register int i;
    int found = -1;
    int match = 0;
    int newmatch = -1;
    
    for (i = 0; i <= NT_LASTONE; i++) {
	/* special kludge for "conn", which is really a variation of socket */
	newmatch = strlen(name);
	if (newmatch <= 4) {
	    if (!strncmp(name, "conn", newmatch)) {
		socket_conn = 1;
	        return (NT_SOCKET);
	    }
	}
	newmatch = MAX(strsubcmp(name, nlookup[i].nl_name), match);
	if (newmatch > match) {
	    found = i;
	    match = newmatch;	    
	}
    }
    return (found);
}



net_apply(ntyp, vaddr, dump)
int ntyp;
register uint vaddr;
int dump;
{
    register uint laddr;
    struct mbuf *mbuf;
    int status = 1;
    int already_verified = 0;
#if	NS_MBUF_QA > 1
    uint next_vaddr;
#endif	NS_MBUF_QA > 1

    NESAV;
    NECLR;
    
    NDBG(("net_apply(%#x,%#x,%#x)", ntyp, vaddr, dump));

    if (dump)
        fprintf (outf, "\n");
    else {
        /* check if we've already verified this address */
	already_verified = netverify_waslogged(ntyp, vaddr);
        if (!already_verified)
	    netverify_log(ntyp, vaddr);  /* log as verified */
    }
    
    if ((ntyp < 0) || (ntyp > NT_LASTONE)) {
        NERROR("unknown type %d", ntyp);
        return;
    }

    NESET(ntyp, vaddr);

    if (dump && !socket_conn)
	fprintf (outf, 
	    "  %s at address 0x%08x:\n\n", netdata_ntypname(ntyp), vaddr);

    if (!NTVALIDV(ntyp, vaddr)) {
        NERROR("invalid networking address", 0);
	goto error;
    } else if (((ntyp == NT_MBUF) || (ntyp == NT_CLUSTER)) &&
      (ntvton(ntyp, vaddr) != ntyp)) {
        NERROR("address is not a %s", netdata_ntypname(ntyp));
	goto error;
    } else {
        /* get in-core copy of structure */
        laddr = vtol(nlookup[ntyp].nl_xindx, vaddr);
        if (laddr == (uint)-1) {
            NERROR("invalid networking address", 0);
            goto error;
        }
        if ((ntyp > MT_LASTONE) && (ntyp != NT_MBUF)) {
	    if (dump)
                (*nlookup[ntyp].nl_dump)(laddr);
	    else if (!already_verified)
		status &= (*nlookup[ntyp].nl_verify)(vaddr, laddr);
        } else {
	    NESAV;
            mbuf = (struct mbuf *)laddr;
	    if (mclmap[mtocl(vaddr)])
	        NWARN("mbuf is a cluster", 0);
            ntyp = mbuf->m_type;
	    NESETNT(ntyp);
	    /* must check if not FREE or cluster first */
	    if ((ntyp != MT_FREE) && !mclmap[mtocl(vaddr)]) {
	        if ((vmtod(vaddr) < mbuf_memory) || 
		  (vmtod(vaddr) >= mbuf_memory + NMBCLUSTERS * NETCLBYTES))
		{
		    NERROR("invalid mbuf", 0);
		    goto error;
		}
	    }
#if	NS_MBUF_QA > 1
	    next_vaddr = mbuf->m_next;
free_mbuf_dump:
#endif	NS_MBUF_QA > 1
    	    if (dump)
		netdump_mbuf(laddr);
	    else if (!already_verified)
    	        status &= netverify_mbuf(vaddr, laddr);
            if (status && ((ntyp >= 0) && (ntyp <= MT_LASTONE)) &&
	      ((ntyp != MT_FREE) || netopts[NETO_MDUMP].val)) {
		/* this is an mbuf, so verify its contents as well */
		uint mbvaddr;
                mbvaddr = vmtod(vaddr); 
                NESETVA(mbvaddr);
                laddr = mtod(mbuf, uint);
		if (ntyp != MT_FREE) {
		    /* It's not free, contents better be valid */
		    if (laddr == (uint) -1) {
			NERROR("invalid networking address", 0);
			goto error;
		    }
		}
		if (laddr != (uint) -1) {
		    if (dump)
			(*nlookup[ntyp].nl_dump)(laddr);
		    else if (!already_verified)
			status &= (*nlookup[ntyp].nl_verify)(mbvaddr, laddr);
		    if ((nlookup[ntyp].nl_dump != n_dump_nop) &&
		      netopts[NETO_MDUMP].val) {
			fprintf (outf, "\n");
		        n_dump_nop(laddr);
			fprintf (outf, "\n");
		    }
#if	NS_MBUF_QA > 1
		    while (ns_mbuf_qa_on && dump && (neterr.ntyp!=MT_FREE) &&
		      (ntyp==MT_MACCT)) {
			struct macct *macct;
			macct = mtod(mbuf, struct macct *);
			if (macct->ma_flags != 0)
			    break;
			NESETNT(MT_FREE);
			mbuf++;
			ntyp = mbuf->m_type;
			if (ntyp==MT_MACCT) {
			    vaddr += MSIZE;
			    laddr = (uint)mbuf;
			    fprintf (outf, "\n  previous existance was:\n\n");
			    goto free_mbuf_dump;
			}
		    }
#endif	NS_MBUF_QA > 1
#if	NS_MBUF_QA > 2
		    if (ns_mbuf_qa_on && dump && status &&
		       (neterr.ntyp==MT_FREE)) {
			int diff, i, j, k;
			uint *trace;
			char free_by[120];

			for (k=1; k<=(ntyp==MT_MACCT?1:2); k++)  {
			    trace = (uint *)(mbuf + k);
			    if ((i = *trace++) == 0)
				break;
			    fprintf(outf,"\n  freed by pid #%d ",i);
			    sprintf(free_by,"at %s",ctime(((time_t *)trace)++));
			    sprintf(&free_by[27],"   x%lx\n",((long *)trace)++);
			    fprintf(outf,free_by);
			    for (i = 2 ,j = 1; i <= trace[1]+1; i++,j++)  {
				/* look up closet symbol */
				diff = findsym(trace[i]);
				if (diff == 0xffffffff)
				    fprintf(outf,"    0x%04x",trace[i]);
				else
				    fprintf(outf,"    %s+0x%04x",cursym,diff);
		    		if ((j%4) == 0)
				    fprintf(outf,"\n");
			    }
			    fprintf(outf,"\n");
			}
		    }
#endif	NS_MBUF_QA > 2
		}
	    }
#if	NS_MBUF_QA > 1
            else if (ns_mbuf_qa_on && dump && status && (ntyp==MT_FREE)) {
		mbuf++;
		ntyp = mbuf->m_type;
		if ((ntyp>0) && (ntyp<=MT_LASTONE) &&
		    (mbuf->m_next?ntvton(NT_MBUF, mbuf->m_next) == NT_MBUF:1) &&
		    (mbuf->m_acct?ntvton(MT_MACCT, mbuf->m_acct)==MT_MACCT:1)) {
		    vaddr += MSIZE; laddr += MSIZE;
		    fprintf (outf, "\n  previous existance was:\n\n");
		    goto free_mbuf_dump;
		}
	    }
#endif	NS_MBUF_QA > 1
	    if ((!mclmap[mtocl(vaddr)]) && ((vmtod(vaddr) >= mbuf_memory) &&
	      (vmtod(vaddr) < mbuf_memory + NMBCLUSTERS * NETCLBYTES))) {
	        if (!got_sigint) {    /* stop chasing m_next on sigint */
#if	NS_MBUF_QA > 1
		    if (netopts[NETO_MFOLLOW].val && next_vaddr) {
			fprintf (outf, "\n\n");
			netdump(NT_MBUF, next_vaddr);
		        NERES;
		        return(status);
		    }
#else	! NS_MBUF_QA > 1
		    if (netopts[NETO_MFOLLOW].val && mbuf->m_next) {
			fprintf (outf, "\n\n");
			netdump(NT_MBUF, mbuf->m_next);
		        NERES;
		        return(status);
		    }
#endif	NS_MBUF_QA > 1
		}
	    }
	    NERES;
        }
    }

    if (dump)
	fprintf(outf,"\n");

    NERES;
    return(status);

error:
    NERES;
    return(0);
}



void net_display(typename,loc,opt,redir,path)
char *typename;
int loc, redir;
char opt;
char *path;
{
    int ntyp, nextv;
    register uint vaddr;

    NESAV;
    NECLR;
    
    NDBG(("net_display(%#x,%#x,%#x,%#x,%#x)",typename,loc,opt,redir,path));

    if (redir) {
        if ((outf = fopen(path,((redir == 2)?"a+":"w+"))) == NULL) {
            fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
            goto out;
        }
    }

    if ((ntyp = nametont(typename)) == -1) {
        NERROR("%s, unknown data structure name", typename);
	return;
    }

    NESETNT(ntyp);

    if (nlookup[ntyp].nl_xindx == -1) {
        NERROR("not yet supported", 0);
	goto reset;
    }
    
    switch (opt) {

    case 'n':
        /* Get vaddr from index into table */
        if ((vaddr = ntov(nlookup[ntyp].nl_xindx, loc)) == (uint) -1)
            goto reset;
        break;
    case '\0' :
        /* Check validity of address */
        if ((vaddr = loc) == -1) {
	    /* check for default */
	    if ((vaddr = nlookup[ntyp].nl_defaultv) == -1) {
	        NERROR("no default address", 0);
		goto reset;
	    }
	}
	NESETVA(vaddr);
        if (!NTVALIDV(ntyp, vaddr)) {
            NERROR("invalid address", 0);
            goto reset;
        }
        break;
    case 'a' :
        /* Display all, get first address */
	nextv = 0;
        vaddr = ntov(nlookup[ntyp].nl_xindx, nextv); /* get first address */
	if (vaddr == -1) {
	    NERROR("no %s(s) in kernel", netdata_ntypname(ntyp));
            goto reset;
	}
        break;
    default: 
        /* bad option */
        NERROR("bad option", 0);
        goto reset;
    }

    if (opt != 'a') {
        NESETVA(vaddr);
        netdump(ntyp, vaddr);
        /* reset our default address */
	nlookup[ntyp].nl_defaultv = vaddr;
    } else {
        /* display all structures of type ntyp */
	while (nextv < NDNUM(nlookup[ntyp].nl_xindx)) {
            if (got_sigint) break;    /* break out of loop on sigint */
	    vaddr = ntov(nlookup[ntyp].nl_xindx, nextv++);
   	    NESETVA(vaddr);
	    if (!net_inuse(ntyp, vaddr)) 
		continue;
	    if (((ntyp != NT_MBUF) && (ntyp != NT_CLUSTER)) ||
	      (ntvton(ntyp, vaddr) == ntyp)) {
		fprintf(outf, "\n");
		netdump(ntyp, vaddr);
	    }
	}
    }

reset:
    socket_conn = 0;
    /* close file if we redirected output */
    if (redir)
        fclose(outf);

out:
    outf = stdout;

    NERES;
}



/* netopt - process networking option */

netopt(name)
char *name;
{
    register int i;
    int found = -1;
    int match = 0;
    int newmatch = -1;
    
    if (name) {
	for (i = 0; netopts[i].name; i++) {
	    newmatch = MAX(strsubcmp(name, netopts[i].name), match);
	    if (newmatch > match) {
		found = i;
		match = newmatch;	    
	    }
	}
        if (found != -1) {
	    netopts[found].val ^= 1;
	    fprintf(outf, "\n  option %s is %s\n", netopts[found].name, 
		netopts[found].val ? "on" : "off");
	} else
	    NERROR("unknown option name: %s", name);
        return;
    }

    fprintf(outf,"\n");
    for (i = 0; netopts[i].name; i++) {
        fprintf(outf,"%12.12s:  %s\n", netopts[i].name,
	    netopts[i].val ? "on" : "off");
    }
    
    fprintf(outf,"\n");

}



/* nethelp - print networking help */

nethelp(redir, path)
int redir;
char *path;
{
    int i, col;

    if (redir) {
        if ((outf = fopen(path,((redir == 2)?"a+":"w+"))) == NULL) {
            fprintf(stderr,"Can't open file %s, errno = %d\n",path,errno);
            goto out;
        }

    }

    fprintf(outf,"\n");
    fprintf(outf,"net    help           Display this message.\n");
    fprintf(outf,"net    option [opt]   Display or toggle net options.\n");
    fprintf(outf,"    where <opt> is mdump or mfollow\n");
    fprintf(outf,"net    type [addr][n] Display network data structure.\n");
    fprintf(outf,"    where <type> is one of:\n%18.18s", "conn");
    for (i = NT_MBUF, col = 2; i <= NT_LASTONE; i++) {
	if (nlookup[i].nl_name[0] == '?')
	    continue;
	if (nlookup[i].nl_xindx == -1) 
	    continue;
	fprintf(outf, "%18.18s", nlookup[i].nl_name);
	if ((col % 3) == 0) 
	    fprintf(outf,"\n");
        col++;
    }
    fprintf(outf,"\n");

reset:
    /* close file if we redirected output */
    if (redir)
        fclose(outf);

out:
    outf = stdout;
}
