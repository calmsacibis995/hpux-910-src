/*
 * $Header: prb_name.c,v 1.5.83.8 93/10/27 11:36:12 donnad Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/prb_name.c,v $
 * $Revision: 1.5.83.8 $		$Author: donnad $
 * $State: Exp $		$Locker:  $
 * $Date: 93/10/27 11:36:12 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) prb_name.c $Revision: 1.5.83.8 $";
#endif

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1986. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */


/*
 * prb_init()		- initialize probe name
 * prb_getcache()	- get nodal cache entry
 * prb_freecache()	- remove nodal cache entry
 * prb_ntnew()		- new name table entry
 * prb_ntfree()		- free name table entry
 * prb_ninput()		- process a name packet
 * prb_nwhohas()	- send a request for name if necessary
 * prb_pxyioctl()	- process a proxy ioctl (add/delete/flush/show/
 *			  list/enable/disable)
 * prb_unsol()		- send unsolicited name replies
 * prb_buildpr()	- build a path report
 * prb_timer()		- probe name timer (calls itself via net_timeout())
 */

#include "../h/param.h"
#include "../h/ns_ipc.h"
#ifdef hp9000s800
#include "../h/dir.h"
#endif hp9000s800
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/kernel.h"
#include "../h/errno.h"
#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/in_var.h"
#include "../netinet/if_ether.h"
#include "../netinet/if_ieee.h"
#include "../netinet/if_probe.h"
#include "../nipc/nipc_hpdsn.h"
#include "../h/netfunc.h"

extern struct probestat probestat;
extern int prb_nameinit;
extern int prb_sequence;
extern char prb_mcast[][6];
extern int nipc_nodename_len;
extern char nipc_nodename[];
extern int nm_islocal();
extern void nm_downshift();

struct ntab *prb_ntnew();
struct mbuf *prb_buildpr();
struct mbuf *prb_unsolcopy();

#define NTAB_KILLC	20*60*2		/* kill completed entry in 20 mins */
#define NTAB_KILLI	10*60*2		/* kill incompleted entry in 10 mins */
#define RETRANSMIT_TIME 0x3F		/* retransmit every 64 seconds */
#define NTAB_BSIZ	5		/* bucket size */
#define NTAB_NB		19		/* number of buckets */
#define NTAB_SIZE	(NTAB_BSIZ * NTAB_NB)

int proxytab_size = NTAB_SIZE;		/* for the proxy command */
int prb_proxyserver = 0;		/* enable proxy server when non-zero */
static int scan_proxytab = 0;		/* to scan the proxy table or not */
struct ntab prb_ntab[NTAB_SIZE];	/* name table */
struct ntab prb_proxytab[NTAB_SIZE];	/* proxy table */

#define NTAB_HASH(name, n_len, index) { \
    char *cptr; \
    int n1; \
    (index) = 0; \
    for (cptr=(name), n1=0; n1 < (n_len); cptr++, n1++) \
	(index) += (int)*cptr; \
    (index) = (index) % NTAB_NB; \
}
#define NTAB_LOOK(nt, name, n_len) { \
    int n2; \
    (nt) = prb_ntab; \
    NTAB_HASH(name, n_len, n2); \
    (nt) = (nt) + (n2 * NTAB_BSIZ); \
    for ( n2=0; n2 < NTAB_BSIZ; n2++, (nt)++ ) { \
	if ((((nt)->nt_flags &  NTF_INUSE) == 0) || \
		((nt)->nt_nlen != (n_len))) \
	    continue; \
	if (bcmp((nt)->nt_name, (name), (n_len)) == 0) \
	    break; \
    } \
    if (n2 >= NTAB_BSIZ) \
	(nt) = (struct ntab *)NULL; \
}
#define NTAB_SEQ(nt, seq) { \
    int n1; \
    (nt) = prb_ntab; \
    for (n1=0; n1 < NTAB_SIZE; n1++, (nt)++) \
	if (((nt)->nt_seqno == (seq)) && ((nt)->nt_flags & NTF_INUSE)) \
	    break; \
    if (n1 >= NTAB_SIZE) \
	(nt) = (struct ntab *)NULL; \
}

#define PTAB_BSIZ	5			/* bucket size */
#define PTAB_NB		19			/* number of buckets */
#define PTAB_SIZE	(PTAB_BSIZ * PTAB_NB)
#define PTAB_HASH(name, n_len, index) { \
    u_char *cptr; \
    int n1; \
    (index) = 0; \
    for (cptr=(name), n1=0; n1 < (n_len); cptr++, n1++) \
	(index) += (int)*cptr; \
    (index) = (index) % PTAB_NB; \
}
#define PTAB_LOOK(nt, name, n_len) { \
    int n2; \
    (nt) = prb_proxytab; \
    PTAB_HASH(name, n_len, n2); \
    (nt) = (nt) + (n2 * PTAB_BSIZ); \
    for ( n2=0; n2 < PTAB_BSIZ; n2++, (nt)++ ) { \
	if ((((nt)->nt_flags & NTF_INUSE) == 0) || \
		((nt)->nt_nlen != (n_len))) \
	    continue; \
	if (bcmp((nt)->nt_name, (name), (n_len)) == 0) \
	    break; \
    } \
    if (n2 >= PTAB_BSIZ) \
	(nt) = (struct ntab *)NULL; \
}

/*
 * It will return a NULL pointer and an error code (ENOBUFS or
 * ETIMEDOUT) or a pointer to a path report for "name". cacheptr
 * will point to a cache entry and should be used while calling
 * prb_freecache() to decrement the reference count. We assume that
 * the calling routine has already made all letters in the name to be
 * uppercase.
 */
struct mbuf *
prb_getcache(name, n_len, cacheptr, error)
char *name;
int n_len;
struct ntab **cacheptr;
int *error;
{
	int err;
	struct ntab *nt = NULL;

	*error = 0;
	nm_downshift(name, n_len);			/* uppercase */
	NTAB_LOOK(nt, name, n_len);
	if ((nt == 0) && prb_proxyserver)		/* check proxy table */
	    PTAB_LOOK(nt, (u_char *)name, n_len);   
	if (nt == 0) {
	    if ((nt = prb_ntnew(name, n_len)) == NULL) {
		    *error = ENOBUFS;			/* XXX */
		    goto out;
	    }
	    nt->nt_refcnt = 1;		/* entry is referenced */
	    /* Compute the length.  Add in the name header. */
	    nt->nt_nrqlen = PRB_PHDRSIZE + PRB_NMREQSIZE + 
			(n_len - NS_MAX_NODE_NAME);
	    if (n_len & 01)			/* Optional pad byte? */
		    nt->nt_nrqlen++;
	    if (++prb_sequence == 0) 
		    ++prb_sequence;		/* skip when 0 */
	    nt->nt_seqno = prb_sequence;
	    nt->nt_ntype = NRQT_NMNODE;
	    prb_nwhohas(name, n_len, nt);	/* Multicast request */
	    if (sleep(nt, PZERO+1 | PCATCH)) {
		*error = EINTR;
		nt->nt_refcnt--;
		nt = NULL;
	    } else if (!(nt->nt_flags & NTF_COM)) {
		*error = ETIMEDOUT;
		nt->nt_refcnt--;
		nt = NULL;
	    }
	} else {
	    nt->nt_timer = 0;
	    if (nt->nt_flags & NTF_COM)
		nt->nt_refcnt++;
	    else {
		if (nt->nt_state == NTS_HOLD) {
		    *error = ETIMEDOUT;
		    nt = NULL;
		} else {
		    nt->nt_refcnt++;
		    if (sleep(nt, PZERO+1 | PCATCH)) {
			*error = EINTR;
			nt->nt_refcnt--;
			nt = NULL;
		    } else if (!(nt->nt_flags & NTF_COM)) {
			*error = ETIMEDOUT;
			nt->nt_refcnt--;
			nt = NULL;
		    }
		}
	    }
	}

out:
	*cacheptr = nt;
	if (nt)
	    return((struct mbuf *)nt->nt_path);
	else
	    return((struct mbuf *)NULL);
}


/*
 * Decrements the use counter in the name cache entry which the
 * pointer references.  The entry is not freed when the reference
 * count reaches zero.  This simply means there is no process that
 * needs to be woken up.  An entry is freed only via prb_timer().
 */

prb_freecache(nt)
caddr_t nt;
{
	if (((struct ntab *)nt)->nt_flags == 0)
	    return;		/* panic instead ??? -XXX */
	if (((struct ntab *)nt)->nt_refcnt)
	    ((struct ntab *)nt)->nt_refcnt--;
}


/*
 * Enter a new address in ntab, pushing out the oldest entry 
 * from the bucket if there is no room.
 */

struct ntab *
prb_ntnew(name, n_len)
char *name;
int n_len;
{
	int n;
	int oldest = -1;
	struct ntab *nt, *nto=NULL;

	NTAB_HASH(name, n_len, n);
	nto = nt = (prb_ntab + (n * NTAB_BSIZ));
	for (n = 0 ; n < NTAB_BSIZ ; n++, nt++) {
	    if (nt->nt_flags == 0)
		goto out;
	    if ((nt->nt_timer > oldest) && (nt->nt_refcnt == 0)) {
		oldest = nt->nt_timer;
		nto = nt;
	    }
	}
	if (nto == NULL)
	    return(NULL);
	nt = nto;
	if (nt->nt_refcnt > 0)
	    return(NULL);
out:
	bcopy(name, nt->nt_name, n_len);
	if (nt->nt_path)
	    m_freem(nt->nt_path);
	nt->nt_flags = NTF_INUSE;
	nt->nt_path = NULL;
	nt->nt_rtcnt = 0;
	nt->nt_timer = 0;
	nt->nt_refcnt = 0;
	nt->nt_state = NTS_INCOMP;
	nt->nt_nlen = n_len;
	nt->nt_timer2 = 0;

	return(nt);
}


/*
 * Free an ntab entry. Should be called with a zero reference count only.
 */

prb_ntfree(nt)
struct ntab *nt;
{
	int i;

	if (nt->nt_refcnt != 0)
	    return(nt->nt_refcnt);

	for(i = 0; i < NS_MAX_NODE_NAME; i++)
	    nt->nt_name[i] = NULL;
	nt->nt_nlen = 0;
	nt->nt_rtcnt = 0;
	nt->nt_timer = 0;
	nt->nt_timer2 = 0;
	nt->nt_flags = 0;
	nt->nt_seqno = 0;
	nt->nt_refcnt = 0;
	nt->nt_state = NTS_NULL;
	if (nt->nt_path)
	    m_freem(nt->nt_path);
	nt->nt_path = (struct mbuf *)NULL;

	return(0);
}

/*
 * This procedure handles inbound name request, name reply, and
 * unsolicited reply packets. This routine is called via netisr()
 * by probeinput() which schedules a netisr event.
 */

static struct sockaddr null_sa = { AF_UNSPEC };
static struct etherxt_hdr etherxt_prbrepl = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_HP,
    IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct ieee8023xsap_hdr ieee8023_prbrepl = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_HP, IEEESAP_HP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
prb_ninput(ac, m, type, src_addr)
struct arpcom *ac;
struct mbuf *m;
int type;
char *src_addr;				/* source address of incoming packet */
{
	struct prb_hdr *ph;		/* PROBE header pointer */
	struct ntab *nt;		/* pointer to name cache */
	struct mbuf *mproxy;		/* mbuf chain for proxy reply */
	struct mbuf *mhdr;
	struct prb_nmreq *pnrq;
	char *cp, *name;
	int n_len, flag, mlength, error = 0;

	/* 
         * m_pullup below clobbers src_addr, which happens to point into m.
	 * we copy it into a local array for now.  might be more efficient
         * to copy into ieee8023_repl (or etherxt_repl, based on type) to
         * avoid 2nd copy under "send_reply".  but this seems more flexible.
	 */

	char save_srcaddr[sizeof(etherxt_prbrepl.sourceaddr)];
	
	bcopy((caddr_t)src_addr, (caddr_t)save_srcaddr,
              sizeof(etherxt_prbrepl.sourceaddr));

	/*
	 * Process all inbound packets one by one. 
	 * Convention: the 0'th element of the quad area is used by
	 * IF_ENQUEUE and is reserved; the 1'st elemetn of the quad area
	 * is used to pass up the packet type (ether or ieee); and
	 * the next 6 bytes (quad 2 and 1/2 of 3) are used to pass
	 * up the ethernet hardware address of the interface the
	 * packet was recieved on.
	 */
	mlength = m_len(m);
	if ((m = m_pullup(m, min(mlength, MLEN))) == NULL)
	    return;
	ph = mtod(m, struct prb_hdr *);
	/* should we check for ph->ph_len -- XXX */
	if ((ph->ph_type != PHT_NAMEREP) && (ph->ph_type != PHT_PROXYREP)) {
	    if (m->m_len < (PRB_PHDRSIZE + 2)) {
		m_freem(m);
		return;
	    }
	    pnrq = (struct prb_nmreq *)(ph + 1);
	    if (m->m_len < PRB_PHDRSIZE + 2 + pnrq->nrq_nlen) {
		m_freem(m);
		return;
	    }
	    name = (char *)(pnrq->nrq_name);
	    n_len = pnrq->nrq_nlen;
	    if (n_len > NS_MAX_NODE_NAME) {  /* PHNE_2819 */
	       m_freem(m);
	       return;
            }
	    nm_downshift(name, n_len);		/* uppercase */
	}
	/*
	 * Figure out what type of packet this is.  It should be a
	 *  1. reply : name request reply or an unsolicited name reply or
	 *  2. request: name request or proxy request
	 */
	if (ph->ph_type == PHT_NAMEREQ)
	    goto request;
	if (ph->ph_type == PHT_PROXYREQ)
	    goto proxyreq;
reply:
	/*
	 * Match the packet's sequence number to an entry in the table.
	 * Wakeup anyone who is waiting on the path report if the cache
	 * entry exists.
	 */
	if ((ph->ph_type == PHT_NAMEREP) || (ph->ph_type == PHT_PROXYREP)) {
							/* solicited */
	    NTAB_SEQ(nt, ph->ph_seq);
	    if (!nt) {
		m_freem(m);
		return;
	    }
	    if (nt->nt_path)		/* free the old path report */
		m_freem(nt->nt_path);
	    m_adj(m, PRB_PHDRSIZE);		/* remove probe header */
	    /* should we m_compress(m, 0) the path report -XXX */
	    nt->nt_path = m;
	    nt->nt_flags |= NTF_COM;
	    if (nt->nt_refcnt > 0)
		wakeup(nt);
	    nt->nt_seqno = 0;
	    nt->nt_state = NTS_COMP;
	    return;
	} else {			/* Unsolicited reply coming in ... */
	    /* Check if this is our own unsolicited reply.If so, drop it. */
	    if (nm_islocal(name, n_len)) {
		m_freem(m);
		return;
	    }
	    NTAB_LOOK(nt, name, n_len);
	    if (nt == 0) {
		if ((nt = prb_ntnew(name, n_len)) == NULL) {
		    m_freem(m);
		    return;
		}
	    } else {
		if (nt->nt_path) 		/* update the path report. */
		    m_freem(nt->nt_path);
		nt->nt_path = 0;
	    }
	    /*
	     * Now adjust the packet to provide an mbuf that
	     * will be the path report.
	     */
	    if (n_len & 01) 
		n_len++;			/* include pad byte if any */
	    m_adj(m, PRB_PHDRSIZE + 2 + n_len);
	    if (m->m_len == 0)
		m = m_free(m);
	    /* do we need to m_compress(m, 0);  -XXX */
	    nt->nt_path = m;
	    nt->nt_flags |= NTF_COM;
	    if (nt->nt_refcnt > 0)
		wakeup(nt);
	    nt->nt_state = NTS_COMP;
	    return;
	}
proxyreq:					/* proxy request */
	if ((prb_proxyserver == 0) || (nm_islocal(name, n_len))) {
	    m_freem(m);
	    return;
	}
	/*
	 * Prepare to turn the packet around and send it out as a reply.
	 * Free everything past the probe header.
	 */
	ph->ph_type = PHT_PROXYREP;
	m->m_len = sizeof(struct prb_hdr);
	if (m->m_next != 0) {
	    m_freem(m->m_next);
	    m->m_next = 0;
	}
	/*
	 * We look in the proxy table only.
	 */
	PTAB_LOOK(nt, (u_char *)name, n_len);
	if (nt == 0) {
	    m_freem(m);
	    return;
	}
	mproxy = m_copy(nt->nt_path, 0, M_COPYALL);
	if (mproxy == 0) {
	    m_freem(m);
	    return;
	}
	m->m_next = mproxy;
	ph->ph_len = m_len(m);
	goto send_reply;
request:					/* name request */
	if (!nm_islocal(name, n_len)) {
	    m_freem(m);
	    return;
	}
	ph->ph_type = PHT_NAMEREP;
	m->m_len = sizeof(struct prb_hdr);
	if (m->m_next != 0) {
	    m_freem(m->m_next);
	    m->m_next = 0;
	}
	m->m_next = prb_buildpr(0);
	if (m->m_next == 0) {
	    m_freem(m);
	    return;
	}
	ph->ph_len = m_len(m);
send_reply:

	switch (type) {
	case ETHERXT_PKT:
	    bcopy((caddr_t)save_srcaddr, (caddr_t)etherxt_prbrepl.destaddr, 6);
	    error = (*ac->ac_build_hdr)(ac, ETHERXT_PKT,
		    (caddr_t)&etherxt_prbrepl, &m);
	    if (error) {
	       m_freem(m);
	       return;
            }

	    (void)(*((struct ifnet *)ac)->if_output)((struct ifnet *)ac, m, 
				&null_sa);
	    break;
	case IEEE8023XSAP_PKT:
	    bcopy((caddr_t)save_srcaddr, (caddr_t)ieee8023_prbrepl.destaddr, 6);
	    ieee8023_prbrepl.length = ph->ph_len + IEEE8023XSAP_LEN;
	    error = (*ac->ac_build_hdr)(ac, IEEE8023XSAP_PKT,
		    (caddr_t)&ieee8023_prbrepl, &m);
	    if (error) {
	       m_freem(m);
	       return;
            }

	    (void)(*((struct ifnet *)ac)->if_output)((struct ifnet *)ac, m, 
				&null_sa);
	    break;
	default:
	    m_freem(m);
	}
}

static struct etherxt_hdr etherxt_prbreq = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_HP,
    IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct etherxt_hdr etherxt_pxyreq = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_HP,
    IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct ieee8023xsap_hdr ieee8023_prbreq = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_HP, IEEESAP_HP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct ieee8023xsap_hdr ieee8023_pxyreq = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_HP, IEEESAP_HP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, IEEEXSAP_PROBE, IEEEXSAP_PROBE
};

/*
 * Multicast a probe name or proxy request packet on each up and
 * running csma/cd interface.
 */
static struct prb_ptr {
    caddr_t header;
}prb_reqptr[][NT_SLOTS] = {
    { {(caddr_t)NULL}, {(caddr_t)NULL}, {(caddr_t)NULL}, {(caddr_t)NULL} },
			/* 0 */
    { {(caddr_t)&etherxt_prbreq}, {(caddr_t)&etherxt_pxyreq}, 
		{(caddr_t)NULL}, {(caddr_t)NULL} },
			/* ACF_ETHER */
    { {(caddr_t)&ieee8023_prbreq}, {(caddr_t)&ieee8023_pxyreq}, 
		{(caddr_t)NULL}, {(caddr_t)NULL} },
			/* ACF_IEEE8023 */
    { {(caddr_t)&ieee8023_prbreq}, {(caddr_t)&etherxt_prbreq}, 
		{(caddr_t)&ieee8023_pxyreq}, {(caddr_t)&etherxt_pxyreq} },
			/* ACF_ETHER & ACF_IEEE8023 */
};

prb_nwhohas(name, n_len, nt)
char	*name;
int	n_len;
struct ntab *nt;
{
	struct mbuf *m, *mhdr;
	struct prb_hdr *ph;		/* probe (generic) header */
	struct prb_nmreq *pnrq;		/* name request header */
	struct ifnet *ifp;		/* Pointer to interfaces */
	struct arpcom *ac;
	caddr_t head_ptr;
	int error = 0;

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
	    /* 
	     * Do not send the request on point to point/loopback
	     * interfaces or interfaces that are not up and running.
	     */
	    if ((ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK)) ||
		((ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
		    (IFF_UP | IFF_RUNNING)))
		continue;
	    ac = (struct arpcom *)ifp;
	    /*
	     * Skip a non csma/cd interface or if there is not
	     * retransmission in this slot
	     */
	    head_ptr = prb_reqptr[(ac->ac_flags & (ACF_ETHER | ACF_IEEE8023))]
		    [nt->nt_rtcnt & NT_SLOTS-1].header;
	    if ((ac->ac_type != ACT_8023) || (head_ptr == NULL))
		continue;
	    if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
		continue;
	    m->m_len = nt->nt_nrqlen;
	    ph = mtod(m, struct prb_hdr *);
	    ph->ph_version = PHV_VERSION;
	    if ((head_ptr == (caddr_t)&etherxt_prbreq) ||
			(head_ptr == (caddr_t)&ieee8023_prbreq))
		ph->ph_type = PHT_NAMEREQ;
	    else
		ph->ph_type = PHT_PROXYREQ;
	    ph->ph_len = nt->nt_nrqlen;
	    ph->ph_seq = nt->nt_seqno;
	    pnrq = (struct prb_nmreq *)(ph + 1);
	    pnrq->nrq_type = nt->nt_ntype;
	    pnrq->nrq_nlen = nt->nt_nlen;
	    bcopy(name, pnrq->nrq_name, nt->nt_nlen);
	    if (nt->nt_nlen & 0x01)
		pnrq->nrq_name[nt->nt_nlen] = '\0';
	    if ((head_ptr == (caddr_t)&etherxt_prbreq) ||
	        (head_ptr == (caddr_t)&etherxt_pxyreq)) {
	        /* ethernet packet */
	        mhdr = 0;
	        error = (*ac->ac_build_hdr)(ac, ETHERXT_PKT,
	    	     head_ptr, &mhdr);
		if (error == ENOBUFS) {
		     m_freem(m);
		     return;
		}
	    } else {	/* ieee 802.3 packet */
	        ((struct ieee8023xsap_hdr *)head_ptr)->length =
	        		ph->ph_len + 10;
	        mhdr = 0;
	        error = (*ac->ac_build_hdr)(ac, IEEE8023XSAP_PKT,
	    	     head_ptr, &mhdr);
		if (error == ENOBUFS) {
		     m_freem(m);
		     return;
		}
	    }
	    mhdr->m_next = m;
	    if (error) 
	    	m_freem(mhdr);
	    else
	    	(void) (*ifp->if_output)(ifp, mhdr, &null_sa);
	}
}

/* Probe proxy ioctl handler. Processes user proxy commands. */
prb_pxyioctl(cmd, data)
	int 	cmd;
	caddr_t	data;
{
	struct ntab *proxy, *endproxy;
	int error = 0;
	int n;
	struct prxentry *pr_entry = (struct prxentry *)data;
	struct mbuf *m, *old_pr;
	struct prb_pathrep *pr_pr;
	struct proxy_path *pr_pp;
	struct proxy_services *pr_ps;
	struct proxy_drep *pr_dr;
	struct proxy_dpath *pr_dp;
	struct proxy_dom *pr_dm;
	struct in_addr ip_addr;     	/* ipaddr from ioctl request */
	struct in_addr ip_match;     	/* ipaddr from dom rept */
	int append_size;		/* size of struct appended */
	int this_domain = 0;      	/* flag for domain to append to */
	struct ifnet *ifp;

	if ((!suser()) && (cmd != SIOCPROXYLIST) && (cmd != SIOCPROXYSHOW))
	    return(EPERM);
	if ((prb_proxyserver == 0) && (cmd != SIOCPROXYON))
	    return(EACCES);
	switch (cmd) {
	case SIOCPROXYON:
		if (prb_proxyserver)
			error = EINVAL;
		else {
		    prb_proxyserver = 1;
		    ifcontrol(IFC_PRBPROXYBIND, 0, 0, 0);
		}
		break;
	case SIOCPROXYOFF:
		if (!(prb_proxyserver))
			error = EINVAL;
		else {
		    /* disable the server and unbind the proxy multicast addr */
		    prb_proxyserver = 0;
		    ifcontrol(IFC_PRBPROXYUNBIND, 0, 0, 0);
		    /* now flush the existing proxy table */
	 	    proxy = prb_proxytab;
		    endproxy = proxy + PTAB_SIZE;
		    while (proxy < endproxy) {
		    	/*
		     	* We cannot nuke the nt_refcnt. So we mark the entry
		     	* and indicate to prb_timer() that it has to scan the
		     	* proxytable for entries which are to be deleted when
		     	* the nt_refcnt becomes zero.
		     	*/
		    	if (proxy->nt_refcnt != 0) {
				proxy->nt_flags |= NTF_DELETE;
				scan_proxytab++;
		    	} else
				(void) prb_ntfree(proxy);
		    	proxy++;
		    }
		}
		break;
	case SIOCPROXYADD:
		nm_downshift(pr_entry->nodename, pr_entry->name_len);
		PTAB_LOOK(proxy, pr_entry->nodename, pr_entry->name_len);
		if (proxy != 0) {
		    error = EINVAL;
		    break;
		}
		PTAB_HASH(pr_entry->nodename, pr_entry->name_len, n);
		proxy = (prb_proxytab + (n * PTAB_BSIZ));
		for (n = 0; n < PTAB_BSIZ; n++, proxy++) {
		    if (proxy->nt_flags == 0)
			goto found_one;
		}
		error = ENOSPC;
		break;
found_one:
		if ((m = m_getclr(M_DONTWAIT, MT_DATA)) == NULL) {
		    error = ENOBUFS;
		    break;
		}	
		bcopy(pr_entry->nodename, proxy->nt_name, pr_entry->name_len);
		proxy->nt_nlen = pr_entry->name_len;
		proxy->nt_flags = (NTF_COM | NTF_INUSE);
		proxy->nt_state = NTS_COMP;
		proxy->nt_nrqlen = PRB_PHDRSIZE + PRB_NMREQSIZE -
		   NS_MAX_NODE_NAME + proxy->nt_nlen;
		if (proxy->nt_nlen & 0x1)
			proxy->nt_nrqlen++;
		proxy->nt_ntype = NRQT_NMNODE;
		pr_pr = mtod(m, struct prb_pathrep *);
		m->m_len = sizeof(struct prb_pathrep);

		pr_pr->rep_len = sizeof(struct proxy_dom);
		pr_pr->dom.d_replen = sizeof(struct proxy_drep);
		pr_pr->dom.d_rep.vna.version = PRQV_VNAVERSION;
		pr_pr->dom.d_rep.vna.domain = pr_entry->domain;
		bcopy(&pr_entry->ipaddr, pr_pr->dom.d_rep.vna.ipaddr,
		  sizeof(struct in_addr));
		pr_pr->dom.d_rep.d_path.d_pathlen = sizeof(struct prb_pr);

		pr_ps = &pr_pr->dom.d_rep.d_path.path_rep.services;
		pr_ps->pid = NSP_SERVICES;
		pr_ps->elem_len = 2;
		pr_ps->service_mask = NSM_SERVICE;

		pr_pp = &pr_pr->dom.d_rep.d_path.path_rep.path;
		pr_pp->xport_grp_pid = NSP_TRANSPORT;
		pr_pp->xport_grp_elem_len = 2;
		pr_pp->xport_grp_service_mask = NSM_XPORT;
		pr_pp->xport_pid = NSP_IP;
		pr_pp->xport_elem_len = 2;
		pr_pp->xport_sap = 0;		/* it isn't used... */
		pr_pp->link_pid = pr_entry->medium;
		pr_pp->link_elem_len = 2;
		if (pr_pp->link_pid == NSP_IEEE802) 
		    pr_pp->link_sap = IEEESAP_IP;
		else if (pr_pp->link_pid == NSP_ETHERNET)
		    pr_pp->link_sap = ETHERTYPE_IP;
		else if (pr_pp->link_pid == NSP_X25)
		    pr_pp->link_sap = 0;

		proxy->nt_path = m;
		break;
	case SIOCPROXYAPPEND:
		nm_downshift(pr_entry->nodename, pr_entry->name_len);
		PTAB_LOOK(proxy, pr_entry->nodename, pr_entry->name_len);
		if (proxy == 0) {		/* no entry found for name */
			error = EINVAL;
			break;
		}
		old_pr = proxy->nt_path;	/* start of path report */
		pr_pr = mtod(old_pr, struct prb_pathrep *);
		pr_dm = &pr_pr->dom;		/* overall domain */
		pr_dr = &pr_dm->d_rep;	    	/* individual domain rept */
		pr_dp = &pr_dr->d_path;	      	/* individual path rept */
		/*
		 * Save the IP address being mapped to. Use it to find 
		 * the correct domain report for the new entry. The domain 
		 * report is matched when the request IP address and domain 
		 * match those in the domain report. Check to be sure 
		 * we're not appending a duplicate path report
		 */
		ip_match.s_addr = pr_entry->ipaddr.s_addr;
		bcopy(pr_dr->vna.ipaddr, &ip_addr, sizeof(struct in_addr));
		if (ip_addr.s_addr == ip_match.s_addr) {
		   	if (pr_dr->vna.domain == pr_entry->domain) {
				if (pr_dp->path_rep.path.link_pid ==
				       pr_entry->medium)
				    /*
				     * A complete duplicate path report. 
				     * Ignore it, and don't return any error 
				     * value (since the path report has been 
				     * appended; just not now)
				     */
				    goto out;
				this_domain = 1; /* add to this domain report */
			}
		}
		/* 
		 * Loop through the existing parts of this nodal path report.
		 * Place the new entry as an added path at the end of the 
		 * appropriate (matching IP address) domain report.
		 * If no IP address match, place the new entry at the end of 
		 * the current chain as the start of a new domain report.
		 */
		while(old_pr->m_next) {
			if ((this_domain) && (m_len(old_pr->m_next) >
			    sizeof(struct proxy_dpath))) {
				break;	/* catch corner case entering loop with
					   this_domain set */
			}
			old_pr = old_pr->m_next;
			if (m_len(old_pr) > sizeof(struct proxy_dpath)) {
				/* entering next IP address domain */
				pr_dm = mtod(old_pr, struct proxy_dom *);
				pr_dr = &pr_dm->d_rep;
				pr_dp = &pr_dr->d_path;
				bcopy(pr_dr->vna.ipaddr, &ip_addr,
				  sizeof(struct in_addr));
				if (ip_addr.s_addr == ip_match.s_addr) {
				    	if (pr_dr->vna.domain ==
					   pr_entry->domain)
						/* add to thisdomain report */
						this_domain = 1;  
				}
			} else { /* in next path erport of same domain report */
				pr_dp = mtod(old_pr,struct proxy_dpath *);
			}
			if (this_domain) {
			    /*
			     * First we want to check to make sure we aren't 
			     * adding an entry that's already there. 
			     * Only add a new path report if it is unique.
			     */
			    if (pr_dp->path_rep.path.link_pid ==
				   pr_entry->medium) 
				goto out;
			    /* 
			     * If we are appending to this IP address domain 
			     * report, we will want to do it at the end of the 
			     * current domain.
			     */
			    if (old_pr->m_next == 0)
				/* end of domain path and of nodal path rept -
				   add new path report to this domain */
				    break;	         

			    if (m_len(old_pr->m_next) >
				   sizeof(struct proxy_dpath))
				    /* end of domain path - insert new 
				       path report before next domain */
				break;           
			}
			continue;
		}
		if (error)
			goto out;
		/*
		 * when we get here one of three things has happened:
		 * 1) we didn't find an IP address domain report to 
		 *    append a path report to, so we're adding a new domain 
		 *    report to the end of the chain
		 * 2) we DID find an IP address domain report to append the 
		 *    path report to, but it's the last domain report, so 
		 *    the new path report is being added to the end of the chain
		 * 3) we DID find an IP address domain report, but it's 
		 *    not the last one in the chain, so we're inserting the 
		 *    path report into the chain at the end of the domain 
		 *    report. In any case, old_pr points to the mbuf holding 
		 *    the part of the chain to which we will append/insert 
		 *    the new report. So, get an mbuf for the addendum. 
		 *    If none available, log an event and return ENOBUFS
		 */
		if ((m = m_getclr(M_DONTWAIT, MT_DATA)) == NULL) {
		    error = ENOBUFS;
		    break;
		}
		/*
		 * link it in if just adding path, set up for that and skip 
		 * all the domain report stuff
		 */
		m->m_next = old_pr->m_next;	/* could be zero... */
		old_pr->m_next = m;
		if (this_domain) {
		    append_size = sizeof(struct proxy_dpath);
		    pr_dp = mtod(m, struct proxy_dpath *);
		    pr_dm->d_replen += append_size;
		    goto add_path;
		}
		/* adding new domain report */
		append_size = sizeof(struct proxy_dom);
		pr_dm = mtod(m, struct proxy_dom *);
		pr_dr = &pr_dm->d_rep;
		pr_dp = &pr_dr->d_path;
		pr_dm->d_replen = sizeof(struct proxy_drep);
		bcopy(&pr_entry->ipaddr, pr_dr->vna.ipaddr,
		  sizeof(struct in_addr));
		pr_dr->vna.version = PRQV_VNAVERSION;
		pr_dr->vna.domain = pr_entry->domain;
		/* now filling in the path report */
add_path:
		pr_dp->d_pathlen = sizeof(struct prb_pr);
		pr_pp = &pr_dp->path_rep.path;
		pr_ps = &pr_dp->path_rep.services;

		pr_ps->pid = NSP_SERVICES;
		pr_ps->elem_len = 2;
		pr_ps->service_mask = NSM_SERVICE;
		pr_pp->xport_grp_pid = NSP_TRANSPORT;
		pr_pp->xport_grp_elem_len = 2;
		pr_pp->xport_grp_service_mask = NSM_XPORT;
		pr_pp->xport_pid = NSP_IP;
		pr_pp->xport_elem_len = 2;
		pr_pp->xport_sap = 0;		/* isn't currently used... */
		pr_pp->link_pid = pr_entry->medium;
		pr_pp->link_elem_len = 2;
		if (pr_pp->link_pid == NSP_IEEE802)
		    pr_pp->link_sap = IEEESAP_IP;
		else if (pr_pp->link_pid == NSP_ETHERNET)
		    pr_pp->link_sap = ETHERTYPE_IP;
		else if (pr_pp->link_pid == NSP_X25)
		    pr_pp->link_sap = 0;
		pr_pr->rep_len += append_size;
		m->m_len = append_size;
		break;
	case SIOCPROXYDELETE:
		nm_downshift(pr_entry->nodename, pr_entry->name_len);
		PTAB_LOOK(proxy, pr_entry->nodename, pr_entry->name_len);
		if (proxy == 0)
		    error = EINVAL;
		else {
		    /*
		     * We cannot nuke the nt_refcnt. So we mark the entry
		     * and indicate to prb_timer() that it has to scan the
		     * proxytable for entries which are to be deleted when
		     * the nt_refcnt becomes zero.
		     */
		    if (proxy->nt_refcnt != 0) {
			proxy->nt_flags |= NTF_DELETE;
			scan_proxytab++;
		    } else
			(void) prb_ntfree(proxy);
		}
		break;
	case SIOCPROXYSHOW:
		break;
	case SIOCPROXYLIST:
		break;
	case SIOCPROXYFLUSH:
		proxy = prb_proxytab;
		endproxy = proxy + PTAB_SIZE;
		while (proxy < endproxy) {
		    /*
		     * We cannot nuke the nt_refcnt. So we mark the entry
		     * and indicate to prb_timer() that it has to scan the
		     * proxytable for entries which are to be deleted when
		     * the nt_refcnt becomes zero.
		     */
		    if (proxy->nt_refcnt != 0) {
			proxy->nt_flags |= NTF_DELETE;
			scan_proxytab++;
		    } else
			(void) prb_ntfree(proxy);
		    proxy++;
		}
		break;
	default:
		error = EINVAL;
	}
out:
	return(error);
}


/*
 *	Send an unsolicited reply via the indicated interface.
 *	If the argument was NULL send over all non loopback and non
 *	point-to-point interfaces.
 */

static struct etherxt_hdr etherxt_prbunsol = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00 }, ETHERTYPE_HP,
    IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
static struct ieee8023xsap_hdr ieee8023_prbunsol = {
    { 0x09, 0x00, 0x09, 0x00, 0x00, 0x01 },
    { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, IEEESAP_HP, IEEESAP_HP, IEEECTRL_DEF,
    { 0x00, 0x00, 0x00 }, IEEEXSAP_PROBE, IEEEXSAP_PROBE
};
prb_unsol(ifptr)
struct ifnet *ifptr;
{
	struct mbuf *m, *m1, *mhdr;
	struct ifnet *ifp;
	struct prb_hdr *ph;
	struct prb_nmreq *pnrq;
	int n_length, error = 0;
	struct arpcom *ac;

	if (nipc_nodename_len == 0) 	/* nodename not set */
		return(ENXIO);

	if ((m = m_get(M_DONTWAIT, MT_DATA)) == NULL) {
		probestat.prb_nombufs++;
		return(ENOBUFS);
	}
	ph = mtod(m, struct prb_hdr * );
	ph->ph_version = PHV_VERSION;
	ph->ph_type = PHT_USNAMEREP;
	if (++prb_sequence == 0) 
		prb_sequence++;
	ph->ph_seq = prb_sequence;
	pnrq = (struct prb_nmreq *)(ph + 1);
	pnrq->nrq_type = NRQT_NMNODE;
	pnrq->nrq_nlen = nipc_nodename_len;
	bcopy((caddr_t)nipc_nodename, (caddr_t)pnrq->nrq_name, 
		nipc_nodename_len);
	n_length = nipc_nodename_len;
	if (n_length & 0x01) 		/* must be even for messages */
		pnrq->nrq_name[n_length++] = '\0';
	m->m_next = prb_buildpr(0);
	if (m->m_next == 0) {
		error = ENOBUFS;
		goto out;
	}
	m->m_len = PRB_PHDRSIZE + n_length + 2; 
	ph->ph_len = m_len(m);

	/*
	 * Each time we want to send out a path report pointed to by m,
	 * we make a copy and send it out. This way we do not have to call
	 * prb_buildpr() more than once.
	 */
	for (ifp=ifnet; ifp; ifp = ifp->if_next) {
		if ((ifptr != NULL) && (ifptr != ifp))
			continue;
		if (ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK))
			continue;
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING | IFF_BROADCAST))
		     != (IFF_UP | IFF_RUNNING | IFF_BROADCAST)) 
			continue;
		ac = (struct arpcom *)ifp;
		switch (ac->ac_type) {
		case ACT_8023:
		    if (ac->ac_flags & ACF_ETHER) {
			if ((m1 = m_copy(m, 0, M_COPYALL)) == NULL) {
	    		    probestat.prb_nombufs++;
			    error = ENOBUFS;
			    goto out;
			}
			mhdr = 0;
			error = (*ac->ac_build_hdr)(ac, ETHERXT_PKT,
				 (caddr_t)&etherxt_prbunsol, &mhdr);
			switch (error) {
			case NULL:
				mhdr->m_next = m1;
				error = (*ifp->if_output)(ifp, mhdr, &null_sa);
				break;
			case ENOBUFS:
				m_freem(m1);
				break;
			default:
				m_freem(mhdr);
				m_freem(m1);
			} /* end switch */
		    }
		    if (ac->ac_flags & ACF_IEEE8023) {
			if ((m1 = m_copy(m, 0, M_COPYALL)) == NULL) {
	    		    probestat.prb_nombufs++;
			    error = ENOBUFS;
			    goto out;
			}
			ieee8023_prbunsol.length = m_len(m1) + 10;
			mhdr = 0;
			error = (*ac->ac_build_hdr)(ac, IEEE8023XSAP_PKT,
				 (caddr_t)&ieee8023_prbunsol, &mhdr);
			switch (error) {
			case NULL:
				mhdr->m_next = m1;
				error = (*ifp->if_output)(ifp, mhdr, &null_sa);
				break;
			case ENOBUFS:
				m_freem(m1);
				break;
			default:
				m_freem(mhdr);
				m_freem(m1);
			} /* end switch */
		    }
		    break;
		default:
		    break;
		}
	}
out:
	m_freem(m);
	return(error);
}

/*
 * This routine builds a path report and is called by nipc code and when we
 * want to send out an unsolicited probe name reply. The input variable
 * loopback indicates whether or not to include the loopback interface in the
 * path report.
 */
struct mbuf *
prb_buildpr(loopback)
int loopback;
{
	struct mbuf *m, *m0, *m1;
	struct ifnet *ifp;
	u_short *pr_len, pr_buffer[MLEN];
	int added_pr = 0;

	if ((m0 = m_get(M_DONTWAIT, MT_DATA)) == NULL)
	    return(NULL);
	m = m0;
	pr_len = mtod(m0, u_short *);
	m0->m_len = 2;
	for (ifp = ifnet; ifp && m ; ifp = ifp->if_next) {
	    if (((ifp->if_flags&IFF_LOOPBACK) && loopback) ||
		(!(ifp->if_flags & IFF_LOOPBACK))) {
		/*
		 * Call the if_control() routine for the interface.
		 * The loopback if_control() routine is called only
		 * if we want loopback included in the path report.
		 * It is necessary to include the path report element 
		 * for loopback since loopback nipc should work.
		 */
		pr_buffer[0] = 0;
		(*ifp->if_control)(ifp, IFC_IFPATHREPORT, &pr_buffer[0], 0, 0);
		if (pr_buffer[0] == 0)	/* no path report for the interface */
		    continue;
		added_pr++;
		if (m->m_off + m->m_len + pr_buffer[0] + sizeof(u_short) 
			>= MMAXOFF) {
		    if ((m1 = m_get(M_DONTWAIT, MT_DATA)) == NULL) {
			m_freem(m0);
			return(NULL);
		    }
		    m1->m_len = 0;
		    m->m_next = m1;
		    m = m1;
		}
		bcopy((caddr_t)pr_buffer, mtod(m, caddr_t) + m->m_len,
			(u_int)pr_buffer[0]+2);
		m->m_len += pr_buffer[0]+2;
	    }
	}
	if (!added_pr) {			/* no path report elements */
	    m_freem(m0);
	    return(NULL);
	}
	*pr_len = m_len(m0) - 2;
	return(m0);
}

/*
 * The timer routine goes through the probe name cache and determines whether
 * to retransmit requests for cache entries in the incompleted and holddown
 * states.
 */
prb_timer()
{
	struct ntab *nt;
	int i;

	nt = &prb_ntab[0];
	for (i=0; i <  NTAB_SIZE; i++, nt++) {
	    if (nt->nt_flags == 0)
		continue;
	    if (++nt->nt_timer >= ((nt->nt_state == NTS_COMP) ?
		    NTAB_KILLC : NTAB_KILLI)) {
		if (nt->nt_refcnt > 0)
		    continue;
		else
		    (void) prb_ntfree(nt);
	    }
	    nt->nt_timer2++;
	    switch (nt->nt_state) {
	    case NTS_INCOMP:
		prb_nwhohas(nt->nt_name, nt->nt_nlen, nt);
		nt->nt_rtcnt++;
		if (nt->nt_rtcnt >= (NT_SLOTS * NT_RETRANS)) {
		    nt->nt_state = NTS_HOLD;
		    if (nt->nt_refcnt > 0)
			wakeup(nt);
		}
		break;
	    case NTS_HOLD:
		if ((nt->nt_timer2 & (RETRANSMIT_TIME)) == 0) {
		    nt->nt_state = NTS_INCOMP;
		    nt->nt_rtcnt = NT_SLOTS;
		}
		break;
	    case NTS_COMP:
	    default:
		break;
	    }
	}
	/*
	 * If indicated, we scan the proxy table for entries which could not
	 * be deleted by the proxy command due to non-zero reference counts.
	 * If the reference count as indicated by nt_refcnt is indeed zero now,
	 * we delete the entry else check for it the next time we scan the 
	 * table.
	 */
	if (scan_proxytab) {
	    scan_proxytab = 0;
	    nt = &prb_proxytab[0];
	    for (i=0; i < NTAB_SIZE; i++, nt++) {
		if (nt->nt_flags == 0)
		    continue;
		else if (nt->nt_flags & NTF_DELETE) {
		    if (nt->nt_refcnt == 0)
			(void) prb_ntfree(nt);
		    else
			scan_proxytab++;
		}
	    }
	}
	net_timeout(prb_timer, (caddr_t)0, hz/2);
}

/*
 * Probe name initialization routine. Called by nipc_init() routine. This
 * initializes the multicast addresses in the different pre-serialized
 * headers and calls prb_timer() which is self perpectuating. The probe
 * name multicast address is the same as the probe vna multicast address
 * and is logged with the card by the probe vna code.
 */
prb_init()
{
    if (prb_nameinit)
	return;
    prb_nameinit = 1;

    bcopy((caddr_t)prb_mcast[PRB_NAME], (caddr_t)ieee8023_prbrepl.destaddr, 6);
    bcopy((caddr_t)prb_mcast[PRB_NAME], (caddr_t)etherxt_prbrepl.destaddr, 6);
    bcopy((caddr_t)prb_mcast[PRB_NAME], (caddr_t)ieee8023_prbreq.destaddr, 6);
    bcopy((caddr_t)prb_mcast[PRB_NAME], (caddr_t)etherxt_prbreq.destaddr, 6);
    bcopy((caddr_t)prb_mcast[PRB_NAME],(caddr_t)ieee8023_prbunsol.destaddr, 6);
    bcopy((caddr_t)prb_mcast[PRB_NAME], (caddr_t)etherxt_prbunsol.destaddr, 6);

    bcopy((caddr_t)prb_mcast[PRB_PROXY], (caddr_t)ieee8023_pxyreq.destaddr, 6);
    bcopy((caddr_t)prb_mcast[PRB_PROXY], (caddr_t)etherxt_pxyreq.destaddr, 6);

    netproc_assign(NET_PRBUNSOL, prb_unsol);

    net_timeout(prb_timer, (caddr_t)0, hz/2);
}
