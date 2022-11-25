/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/netvfy.c,v $
 * $Revision: 1.15.83.3 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 16:28:39 $
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
static char rcsid[]="@(#) $Header: netvfy.c,v 1.15.83.3 93/09/17 16:28:39 root Exp $";
#endif


#include "../standard/inc.h"
#include "net.h"
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"



#define NVLOGSPBLOCK	30

struct verify_log {
    /* FIRST FIELD IS NOT YET USED, IT WILL ALWAYS BE CORRECT */
    int	    ntyp;	/* type that this address was verified as	*/
    uint    vaddr;	/* the address that was logged as verified	*/
};

struct verify_log_block {
    int	    nlogs;	/* number of logs entered into this block	*/
    struct  verify_log_block	*next;	/* next block on list		*/
    struct  verify_log		entry[NVLOGSPBLOCK];
};

struct verify_log_block	    *vlog[NT_LASTONE];



int netverify_waslogged(ntyp, vaddr)
{
    register int i;
    register struct verify_log_block *vp;

    for (vp = vlog[ntyp]; vp; vp = vp->next) {
        for (i = 0; i < vp->nlogs; i++) {
            if (vp->entry[i].vaddr == vaddr) {
    	        if (vp->entry[i].ntyp != ntyp) {
		    NESAV;
		    NESET(ntyp, vaddr);
                    NERROR("address logged twice, 1st was %s", 
		        netdata_ntypname(vp->entry[i].ntyp));
		    NERES;
	        }
                /* NMSG("address logged twice, %s - OK!", 
		    netdata_ntypname(vp->entry[i].ntyp)); */
	        return(1);
	    }
	}
    }
    return(0);
}



netverify_log(ntyp, vaddr)
{
    register int i;
    register struct verify_log_block *vp, *lastvp;

    /* NMSG2("address logged ONCE, %s, vaddr=%#x",  
	netdata_ntypname(ntyp), vaddr); */

    for (lastvp = vp = vlog[ntyp]; vp; lastvp = vp, vp = vp->next) {
        for (i = 0; i < vp->nlogs; i++) {
            if ((vp->entry[i].ntyp == ntyp) &&
	        (vp->entry[i].vaddr == vaddr))
	            return;
	}
        if (i != NVLOGSPBLOCK) {
            vp->entry[i].ntyp = ntyp;
            vp->entry[i].vaddr = vaddr;
            vp->nlogs++;
	    return;
	}
    }
    vp = net_malloc (sizeof (struct verify_log_block));
    if (!vlog[ntyp])
	vlog[ntyp] = vp;
    else 
	lastvp->next = vp;  
    vp->entry[0].ntyp = ntyp;
    vp->entry[0].vaddr = vaddr;
    vp->nlogs = 1;
    vp->next = NULL;
}



/* netverify_clearlog - clear all network address log entries */

netverify_clearlog()
{
    int i;
    register struct verify_log_block *vp;

    for (i = 0; i < NT_LASTONE; i++) 
        for (vp = vlog[i]; vp; vp = vp->next) {
	    /* NMSG3("clear log block %#x, ntyp %#x, nlogs %#x",
		vp, i, vp->nlogs); */
            vp->nlogs = 0;
	}
}



int netverify_nop(vaddr, laddr)
uint vaddr;
char *laddr;
{
	return (1);
}



netverify_ifnet(vaddr, ifnet)
uint vaddr;
struct ifnet *ifnet;
{
    netverify_ifqueue(vaddr + ((uint)&(ifnet->if_snd) - (uint)&ifnet), 
	&(ifnet->if_snd));
    
    if (findsym(ifnet->if_init) != 0) {
	NERROR("if_init pointer is invalid", 0);
	goto error;
    }
    if (findsym(ifnet->if_output) != 0) {
	NERROR("if_output pointer is invalid", 0);
	goto error;
    }
    if (findsym(ifnet->if_ioctl) != 0) {
	NERROR("if_ioctl pointer is invalid", 0);
	goto error;
    }
    if (findsym(ifnet->if_reset) != 0) {
	NERROR("if_reset pointer is invalid", 0);
	goto error;
    }
    if (findsym(ifnet->if_watchdog) != 0) {
	NERROR("if_watchdog pointer is invalid", 0);
	goto error;
    }

#ifndef NEWBM
    if ((ifnet->if_macct) &&
     (netverify(NT_MBUF, (uint)(ifnet->if_macct) & ~(MSIZE-1)) == 0)) {
        NERROR("if_macct, 0x%08x, is invalid", ifnet->if_macct);
	goto error;
    }
#endif NEWBM

    if ((ifnet->if_next) && (!netverify(NT_IFNET, ifnet->if_next)))
	NWARN("ifnet referenced by if_next is in error", 0);

    return(1);

error:
    return(0);
}



netverify_ifqueue(vaddr, ifqueue)
uint vaddr;
register struct ifqueue *ifqueue;
{
    register int i, errs;

    errs = 0;

    for (i = 0; i < IFQ_MAXLEN; i++) {
        if (ifqueue->ifq_slot[i] && 
	  netverify(NT_MBUF, ifqueue->ifq_slot[i]) == 0) {
            NERROR("ifqueue mbuf pointer, 0x%08x, is invalid", 
		ifqueue->ifq_slot[i]);
	    errs++;
        }
    }
   
    if (ifqueue->ifq_drops) 
        NWARN("ifq_drops == 0x%08x", ifqueue->ifq_drops);
    
    if (ifqueue->ifq_len >= ifqueue->ifq_maxlen) {
        NERROR("ifq_len >= ifqueue->ifq_maxlen", 0);
	errs++;
    }
    
    return(errs == 0);
}



netverify_inpcb(vaddr, inpcb)
uint vaddr;
struct inpcb *inpcb;
{
    int errs = 0;

    if (inpcb->inp_next && (!netverify(NT_INPCB, inpcb->inp_next))) {
	NERROR("inp_next field is invalid: 0x%08x", inpcb->inp_next);
	errs++;
    }
    if (inpcb->inp_prev && (!netverify(NT_INPCB, inpcb->inp_prev))) {
	NERROR("inp_prev field is invalid: 0x%08x", inpcb->inp_prev);
	errs++;
    }
    if (inpcb->inp_head && (!netverify(NT_INPCB, inpcb->inp_head))) {
	NERROR("inp_head field is invalid: 0x%08x", inpcb->inp_head);
	errs++;
    }
    if (inpcb->inp_socket && (!netverify(NT_SOCKET, inpcb->inp_socket))) {
	NERROR("inp_socket field is invalid: 0x%08x", inpcb->inp_socket);
	errs++;
    }
    if (inpcb->inp_ppcb && (!netverify(MT_PCB, dtom(inpcb->inp_ppcb)))) {
	NERROR("inp_ppcb field is invalid: 0x%08x", inpcb->inp_ppcb);
	errs++;
    }
    if (inpcb->inp_ppcb && (inpcb->inp_ttl == 0))
	NWARN("inp_ttl == %#x", inpcb->inp_ttl);
    
    return(errs == 0);
}



netverify_prntab(vaddr, prntab)
uint vaddr;
struct ntab *prntab;
{
    int errs = 0;

    if (prntab->nt_path && !netverify(MT_PATH_REPORT, 
	(uint) prntab->nt_path & ~(MSIZE-1))) {
	    NERROR("nt_path field is invalid: 0x%08x", prntab->nt_path);
	    errs++;
    }
    return(errs == 0);
}



netverify_prvnatab(vaddr, prvnatab)
uint vaddr;
struct vnatab *prvnatab;
{
    int errs = 0;

    if (prvnatab->vt_if && !netverify(NT_IFNET, prvnatab->vt_if)) {
	NERROR("vt_if, 0x%08x, is invalid", prvnatab->vt_if);
	errs++;
    }
    if (prvnatab->vt_packet && !netverify(MT_DATA, 
	(uint) prvnatab->vt_packet & ~(MSIZE-1))) {
	    NERROR("vt_packet field is invalid: 0x%08x", prvnatab->vt_packet);
	    errs++;
    }
    return(errs == 0);
}



netverify_tcpcb(vaddr, tcpcb)
uint vaddr;
struct tcpcb *tcpcb;
{
    int errs = 0;
    int i;

    if (!netverify(NT_INPCB, tcpcb->t_inpcb)) {
	NERROR("t_inpcb field is invalid: 0x%08x", tcpcb->t_inpcb);
	errs++;
    }
/* never used:
    if (tcpcb->t_tcpopt && !netverify(MT_DATA, 
	(uint) tcpcb->t_tcpopt & ~(MSIZE-1))) {
	    NERROR("t_tcpopt field is invalid: 0x%08x", tcpcb->t_tcpopt);
	    errs++;
    }
    if (tcpcb->t_ipopt && !netverify(MT_DATA, 
	(uint) tcpcb->t_ipopt & ~(MSIZE-1))) {
	    NERROR("t_ipopt field is invalid: 0x%08x", tcpcb->t_ipopt);
	    errs++;
    }
*/
    if (tcpcb->t_mmtblp && !netverify(MT_MSG_MODE_TBL, 
	(uint) tcpcb->t_mmtblp & ~(MSIZE-1))) {
	    NERROR("t_mmtblp field is invalid: 0x%08x", tcpcb->t_mmtblp);
	    errs++;
    }
    /* too verbose - should print only for ESTABLISHED states
    for (i = 0; i < TCPT_NTIMERS; i++)
        if (tcpcb->t_timer[i] == 0) 
	    NWARN2("t_timer[%#x] == %#x", i, tcpcb->t_timer[i]);
    */

    return(errs == 0);
}



netverify_ipq(vaddr, ipq)
uint vaddr;
register struct ipq *ipq;
{
    register int errs = 0;

    /* don't check first ipq in list, it's static */
    if ((vaddr != VA(X_IPQ, uint)) &&
	!netverify(MT_FTABLE, vaddr & ~(MSIZE-1))) {
	    NERROR("invalid MT_FTABLE mbuf", 0);
	    errs++;
    }
    if (ipq->next && !netverify(NT_IPQ, ipq->next)) {
	    NERROR("next field is invalid: 0x%08x", ipq->next);
	    errs++;
    }
    if (ipq->prev && !netverify(NT_IPQ, ipq->prev)) {
	    NERROR("prev field is invalid: 0x%08x", ipq->prev);
	    errs++;
    }
    if (ipq->ipq_next && !netverify(NT_IPASFRAG, ipq->ipq_next)) {
	    NERROR("ipq_next field is invalid: 0x%08x", ipq->ipq_next);
	    errs++;
    }
    if (ipq->ipq_prev && !netverify(NT_IPASFRAG, ipq->ipq_prev)) {
	    NERROR("ipq_prev field is invalid: 0x%08x", ipq->ipq_prev);
	    errs++;
    }
    if (ipq->ipq_next && (ipq->ipq_ttl == 0)) 
	    NMSG("ipq_ttl field is zero", 0);
    return(errs == 0);
}



netverify_ipasfrag(vaddr, ipf)
uint vaddr;
register struct ipasfrag *ipf;
{
    register int errs = 0;
 
    if (ipf->ipf_next && !netverify(NT_IPASFRAG, ipf->ipf_next)) {
	    NERROR("ipf_next field is invalid: 0x%08x", ipf->ipf_next);
	    errs++;
    }
    if (ipf->ipf_prev && !netverify(NT_IPASFRAG, ipf->ipf_prev)) {
	    NERROR("ipf_prev field is invalid: 0x%08x", ipf->ipf_prev);
	    errs++;
    }
    return(errs == 0);
}



netverify_lanift(vaddr, lanift)
uint vaddr;
register lan_ift *lanift;
{
    register int i;

    for (i = 0; i < 4; i++) {
        if (lanift->xtofree[i] && 
	  netverify(NT_MBUF, lanift->xtofree[i]) == 0) {
            NERROR("lan_ift xtofree mbuf pointer, 0x%08x, is invalid", 
		lanift->xtofree[i]);
	    goto error;
        }
        if (lanift->mbuf_rd[i] && 
	  netverify(NT_MBUF, lanift->mbuf_rd[i]) == 0) {
            NERROR("lan_ift mbuf_rd mbuf pointer, 0x%08x, is invalid", 
		lanift->mbuf_rd[i]);
	    goto error;
        }
    }

    if (!netverify_ifnet(vaddr, &(lanift->is_if)))
        goto error;

    /* verify arpcom forward link ### */

#ifndef NEWBM
    if ((lanift->macct) &&
      (netverify(NT_MBUF, (uint)(lanift->macct) & ~(MSIZE-1)) == 0)) {
        NERROR("lan_ift macct, 0x%08x, is invalid", lanift->macct);
	goto error;
    }
#endif NEWBM

    if (lanift->flags & LAN_DEAD) {
        NWARN("lan_ift flags: LAN_DEAD", 0);
	goto error;
    }

    return(1);

error:
    return(0);
}



netverify_mbuf(vaddr, m)
uint vaddr;
register struct mbuf *m;
{
    NESAV;
    NESETNT(NT_MBUF);
    NESETVA(vaddr);

    if (vaddr < mbuf_memory) {
        NWARN("address is less than beginning of mbuf memory", 0);
	goto error;
    }
    if (vaddr >= (mbuf_memory+NMBCLUSTERS*NETCLBYTES))
    {
        NWARN("address is greater than end of mbuf memory", 0);
	goto error;
    }
    if (!m_isa_largebuf(m) && (vmtod(vaddr) < (caddr_t) mbuf_memory)) {
        NWARN("references address less than beginning of mbuf memory", 0);
	goto error;
    }
    if (!m_isa_largebuf(m) && 
	(vmtod(vaddr) >= (caddr_t) (mbuf_memory+NMBCLUSTERS*NETCLBYTES)))
    {
        NWARN("references address greater than end of mbuf memory", 0);
	goto error;
    }
    if (!m_isa_largebuf(m) && 
	(m->m_type == MT_DATA || m->m_type == MT_HEADER || 
	 m->m_type == MT_SONAME)) {
        if (vmtod(vaddr) + m->m_len < (caddr_t) mbuf_memory) {
	    NWARN("end of mbuf data is less than beginning of mbuf memory",0);
	    goto error;
	}
        if (vmtod(vaddr) + m->m_len >
            (caddr_t) (mbuf_memory + (NMBCLUSTERS)*NETCLBYTES))
	{
	    NWARN("end of mbuf data is greater than end of mbuf memory", 0);
	    goto error;
	}
        if (m->m_len > (m->m_flags&MF_CLUSTER ? NETCLBYTES : MLEN)) {
	    NWARN("length of mbuf data greater than NETCLBYTES or MLEN",0);
	    goto error;
	}
    }
    if (vaddr & (MSIZE-1)) {
        NWARN("not aligned on an mbuf boundry",0);
#ifdef	NS_MBUF_QA
	if (ns_mbuf_qa_on == 0)
#endif	NS_MBUF_QA
	goto error;
    }
    if (m->m_type < MT_FREE || m->m_type > MT_LASTONE) {
        NWARN("invalid mbuf m_type field",0);
	goto error;
    }
    if (m->m_type != MT_FREE) {
        if (m->m_acct && !netverify(NT_MBUF, (dtom(m->m_acct)))) {
            NWARN("m_acct references an invalid m_acct: 0x%08x", m->m_acct);
            goto error;
        }
        if ((m->m_flags & MF_CLUSTER) && mclrefcnt[vmbtocl(vaddr)] <= 0) {
                NWARN("mbuf points to an unreferenced cluster",0);
                goto error;
        }
    }
    NERES;
    return(1);

error:
    NERES;
    return(0);
}



netverify_socket(vaddr, socket)
uint vaddr;
struct socket *socket;
{
    register int i, errs;
    NDBG(("netverify_socket(%#x,%#x)", vaddr, socket));
   
    if (socket->so_type == SOCK_DEST) {
	/* none of these checks apply */
	return (1);
    }
 
    errs = 0;

    if (socket->so_refcnt == 0) {
	NWARN("so_refcnt == 0", 0);
	errs++;
    }

    /* release 2, verify so_name ### */

    if (dtom(socket->so_pcb) && netverify(MT_PCB, dtom(socket->so_pcb)) == 0){
        NERROR("so_pcb mbuf pointer, 0x%08x, is invalid", socket->so_pcb);
	errs++;
    }
    if (netverify(NT_PROTOSW, socket->so_proto) == 0) {
        NERROR("so_proto pointer, 0x%08x, is invalid", socket->so_proto);
	errs++;
    }
#ifdef NEWBM
#define SB_OFFSET(f)	vaddr + (uint)(&(socket->f) - &(socket->so_sc))
    if (netverify_sockbuf(SB_OFFSET(so_rcv), &(socket->so_rcv)) == 0) {
        NERROR("sockbuf so_rcv, 0x%08x, is invalid", SB_OFFSET(so_rcv));
	errs++;
    }
    if (netverify_sockbuf(SB_OFFSET(so_snd), &(socket->so_snd)) == 0) {
        NERROR("sockbuf so_snd, 0x%08x, is invalid", SB_OFFSET(so_snd));
	errs++;
    }
#else  NEWBM
    if (netverify(NT_SOCKBUF, socket->so_rcv) == 0) {
        NERROR("sockbuf so_rcv, 0x%08x, is invalid", socket->so_rcv);
	errs++;
    }
    if (netverify(NT_SOCKBUF, socket->so_snd) == 0) {
        NERROR("sockbuf so_snd, 0x%08x, is invalid", socket->so_snd);
	errs++;
    }
#endif NEWBM
    if (socket->so_head && (uint)socket->so_head != vaddr && 
	!netverify(NT_SOCKET, socket->so_head)) {
            NERROR("socket q so_head, 0x%08x, is invalid", socket->so_head);
	    errs++;
    }
    if (socket->so_q0 && (uint)socket->so_q0 != vaddr && 
	!netverify(NT_SOCKET, socket->so_q0)) {
            NERROR("socket q so_q0, 0x%08x, is invalid", socket->so_q0);
	    errs++;
    }
    if (socket->so_q && (uint)socket->so_q != vaddr && 
	!netverify(NT_SOCKET, socket->so_q)) {
            NERROR("socket q so_q, 0x%08x, is invalid", socket->so_q);
	    errs++;
    }
    if ((socket->so_qlimit) && 
        (socket->so_qlen + socket->so_q0len >= socket->so_qlimit)) {
            NWARN("so_qlen + so_q0len >= so_qlimit", 0);
            errs++;
    }
    return(errs == 0);
}



netverify_sockbuf(vaddr, sockbuf)
uint vaddr;
register struct sockbuf *sockbuf;
{
    register int i, errs = 0;
    NDBG(("netverify_sockbuf(%#x,%#x)", vaddr, sockbuf));

    /* Too verbose ...
    if (sockbuf->sb_cc)
        NMSG("sb_cc == %#x", sockbuf->sb_cc);
    */

    if (sockbuf->sb_sel && ((sockbuf->sb_sel < vproc) ||
	(sockbuf->sb_sel >= (struct proc *)&vproc[nproc]))) {
	    NERROR("sb_sel does not point to a valid process: %#x", 
		sockbuf->sb_sel);
	    errs++;
    }
    if (sockbuf->sb_msgsqd > sockbuf->sb_maxmsgs) {
            NERROR("sb_msgsqd > sb_maxmsgs", 0);
            errs++;
    }
    if (sockbuf->sb_seqoff > sockbuf->sb_msgsize) {
            NERROR("sb_seqoff > sb_msgsize", 0);
            errs++;
    }
    if (sockbuf->sb_msgsize && 
	(sockbuf->sb_threshold > sockbuf->sb_msgsize)) {
            NERROR("sb_threshold > sb_msgsize", 0);
            errs++;
    }
    netfollow_msg (sockbuf->sb_macct, sockbuf->sb_msgsqd, sockbuf->sb_cc,
	sockbuf->sb_lastmsg, vaddr);

    return (errs == 0);
}



netfollow_msg (vmacct, msgsqd, cc, lastmsg, vsockbuf)
uint vmacct;
int msgsqd, cc;
uint lastmsg;
uint vsockbuf;
{
    register struct macct *macct;
    register int real_cc = 0;

    while (vmacct) {
        macct = (struct macct *) vtol (X_MBUTL, vmacct);
        if (macct == (struct macct *) -1) {
            NERROR("invalid macct address in message queue: 0x%08.8x", vmacct);
    	    return;
        }   
	real_cc += macct->ma_cc;
        msgsqd--;
	if ((!macct->ma_nextmsg) && (lastmsg != vmacct)) 
	    NERROR("sb_lastmsg does not point to last message in sockbuf", 0);
	vmacct = macct->ma_nextmsg;
    }
    if (msgsqd) 
	NERROR("sb_msgsqd != number of messages queued", 0);
    if (real_cc != cc)
	NERROR("sb_cc != number of characters queued (0x%08.8x)", real_cc);
} 



netverify_sapinput(vaddr, sapinput)
uint vaddr;
register struct sap_input *sapinput;
{
    NESAV;
    NESETNT(NT_SAPINPUT);
    NESETVA(vaddr);

    if ((uint)sapinput->inq || (uint)sapinput->rint) {
        if (findsym(sapinput->inq) != 0) {
	    NERROR("intr queue pointer, 0x%08x, is invalid", sapinput->inq);
	    goto error;
        }
        if (findsym(sapinput->rint) != 0) {
	    NERROR("intr input routine, 0x%08x, is invalid", sapinput->rint);
	    goto error;
	}
    }

    NERES;
    return(1);

error:
    NERES;
    return(0);
}



netverify_xsaplink(vaddr, xsaplink)
uint vaddr;
register struct xsap_link *xsaplink;
{
    NESAV;
    NESETNT(NT_XSAPLINK);
    NESETVA(vaddr);

    if (vaddr == (uint)xsap_head)
	goto out;
	
    if (netverify(NT_MBUF, xsaplink->m) == 0) {
        NERROR("xsap_link mbuf pointer, 0x%08x, is invalid", xsaplink->m);
	goto error;
    }

    if (netverify_sapinput(vaddr + 
	(uint)&(xsaplink->log.input) - (uint)xsaplink, 
	    &(xsaplink->log.input)) == 0) {
		NERROR("xsap_link sap_input struct is invalid", 0);
		goto error;
    }

    if (!netverify(NT_XSAPLINK, xsaplink->next)) 
	NWARN("xsap_link referenced by next is in error", 0);

    if (!netverify(NT_XSAPLINK, xsaplink->prev))
	NWARN("xsap_link referenced by prev is in error", 0);

out:
    NERES;
    return(1);

error:
    NERES;
    return(0);
}



/* netverify -- generic networkig data structure verify.  Verify the
 * networking data structure of type "ntyp" at virtual address vaddr.
 * Returns non-zero if the data is ok.  All calls to verify data structures
 * pass through this routine.  Nobody calls the individual verify routines
 * directly.  We don't print recusive verifications, even if the -v flag 
 * is set, because that would result in a chaotic output order.
 */

int netverify(ntyp, vaddr)
int ntyp;
register uint vaddr;
{
    static int apply_recursion = 0;
    int status;

    /* fprintf(outf,"netverify(%#x,%#x)\n",ntyp,vaddr); */

    apply_recursion++;
    
    status = net_apply(ntyp, vaddr, 0); /* verify */
    if ((apply_recursion <= 1) && vflg) 
        net_apply(ntyp, vaddr, 1); /* dump if verbose and no recursion */

    apply_recursion--;
    return (status);
}


