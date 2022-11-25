/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/netdump.c,v $
 * $Revision: 1.24.83.3 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 16:28:03 $
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
static char rcsid[]="@(#) $Header: netdump.c,v 1.24.83.3 93/09/17 16:28:03 root Exp $";
#endif


#include "../standard/inc.h"
#include "net.h"		/* all the network include files */
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"



/* dump contents of cluster */
void netdump_cluster(cluster)
register char *cluster;
{
    NDBG(("netdump_cluster(%#x)", cluster));
#ifdef	NS_MBUF_QA
    fprintf(outf,"  mclrefcnt   0x%08x\n", mclrefcnt[mtocl(neterr.vaddr)]);
#endif	NS_MBUF_QA

    hexdump(cluster, NETCLBYTES, 0, 0);
}



static struct netflags mbuf_flags[] = {
    MF_CLUSTER,	    "MF_CLUSTER",
    MF_EOR,	    "MF_EOR",
    MF_EOM,	    "MF_EOM",
    MF_INTRANSIT,   "MF_INTRANSIT",
    MF_LARGEBUF,    "MF_LARGEBUF",
    0, 0
};

/* dump contents of mbuf */
void netdump_mbuf(mbuf)
register struct mbuf *mbuf;
{
    NDBG(("netdump_mbuf(%#x)", mbuf));
    
    fprintf(outf,"  m_next    0x%08x  m_off     0x%08x  m_len     0x%04hx\n",
        mbuf->m_next, mbuf->m_off, mbuf->m_len);
    fprintf(outf,"\
  m_type    0x%02.2hx        m_flags   0x%02.2hx        m_acct    0x%08x\n",
        mbuf->m_type, mbuf->m_flags, mbuf->m_acct);
    if (m_isa_largebuf(mbuf)) {
      fprintf(outf, "  m_clfun 0x%08x    %s\n",
        mbuf->m_clfun, addr_to_sym(mbuf->m_clfun));
      fprintf(outf,"  m_clarg 0x%08x    m_clswp 0x%08x    m_cltype  0x%02.2hx\n",
	mbuf->m_clarg, mbuf->m_clswp, mbuf->m_cltype);
    }
#ifndef NEWBM
    fprintf(outf,"  m_filler  0x%08x  m_seqnum 0x%08x\n",
        mbuf->m_filler, mbuf->m_seqnum);
#endif NEWBM
#ifdef	NS_MBUF_QA
    if ( mbuf->m_type && (mbuf->m_flags & MF_CLUSTER) )
	    fprintf(outf,"  cluster @ 0x%08x  clrefcnt 0x%08x\n",
		vmtod(neterr.vaddr)&~NETCLOFSET,mclrefcnt[mtocl(neterr.vaddr)]);
#endif	NS_MBUF_QA

    netdump_flags(mbuf_flags, mbuf->m_flags);

    /* print type */
    fprintf (outf, "  %s\n", netdata_ntypname(mbuf->m_type));
}



static struct netflags macct_flags[] = {
    MA_EOM,	    "MA_EOM",
    MA_INUSE,	    "MA_INUSE",
    MA_SEQ_PRESENT, "MA_SEQ_PRESENT",
    MA_INBOUND,	    "MA_INBOUND",
    0, 0
};

/* dump contents of macct */
void netdump_macct(macct)
struct macct *macct;
{
    NDBG(("netdump_macct(%#x)", macct));

    fprintf(outf,"  ma_mbrsvd 0x%08x  ma_mbfree 0x%08x  ma_flag   0x%08x\n",
        macct->ma_mbrsvd, macct->ma_mbfree, macct->ma_flags);
    fprintf(outf,"  ma_cc     0x%08x  ma_msglen 0x%08x  ma_seqnum 0x%08x\n",
        macct->ma_cc, macct->ma_msglen, macct->ma_seqnum);
    fprintf(outf,"  ma_mb     0x%08x  ma_nxtmsg 0x%08x  ma_sb     0x%08x\n",
        macct->ma_mb, macct->ma_nextmsg, macct->ma_sb);

    netdump_flags(macct_flags, macct->ma_flags);
}



/* dump contents of msgmtbl */
void netdump_msgmtbl(msgmtbl)
struct msgmtbl *msgmtbl;
{
    register int i;

    NDBG(("netdump_msgmtbl(%#x)", msgmtbl));

    fprintf(outf,"  iqsize    0x%08x\n  imsgindx  0x%04hx  imsglen:\n",
        msgmtbl->iqsize, msgmtbl->imsgindx);
    for (i = 0; i <= MAXMM_MSGS; i++) {
	fprintf(outf, "  0x%04hx", msgmtbl->imsglen[i]);
	if ((i + 1) % 8 == 0)
	    fprintf(outf, "\n");
    }
    fprintf(outf,"\n  omsgindx  0x%04hx  omsglen:\n",
        msgmtbl->omsgindx);
    for (i = 0; i <= MAXMM_MSGS; i++) {
	fprintf(outf, "  0x%04hx", msgmtbl->omsglen[i]);
	if ((i + 1) % 8 == 0)
	    fprintf(outf, "\n");
    }
}



void netdump_domain(domain)
struct domain *domain;
{
    NDBG(("netdump_domain(%#x)", domain));

    fprintf(outf,
       "  dom_family   0x%08x  dom_domain   0x%08x\n",
          domain->dom_family, domain->dom_domain);
    fprintf(outf,
       "  dom_name     0x%08x  \"%s\"\n\n",
          domain->dom_name, net_getstr(domain->dom_name));
    fprintf(outf,
       "  dom_add_csite_domains    0x%08x   %s\n",
          domain->dom_add_csite_domains,
          addr_to_sym(domain->dom_add_csite_domains));
    fprintf(outf,
       "  dom_remote_csite_path    0x%08x   %s\n\n",
          domain->dom_remote_csite_path,
          addr_to_sym(domain->dom_remote_csite_path));
    fprintf(outf,
       "  dom_extract_path         0x%08x   dom_protosw  0x%08x\n", 
          domain->dom_extract_path, domain->dom_protosw);
    fprintf(outf,
       "  dom_protoswNPROTOSW      0x%08x   dom_next     0x%08x\n",
          domain->dom_protoswNPROTOSW, domain->dom_next);
}



void netdump_conn(conn)
register struct socket *conn;
{
    struct inpcb *pcb;
    uint vaddr;

    NDBG(("netdump_conn(%#x)", conn));

    vaddr = (uint) conn - LA(X_MBUTL, uint) + VA(X_MBUTL, uint);
    fprintf(outf, "\nConnection at socket 0x%08x:\n", vaddr);
    fprintf(outf, "================================\n");
    fprintf(outf, "\n  socket at address 0x%08x:\n\n", vaddr);
    netdump_socket(conn);
    if ((conn->so_type != SOCK_DEST) && (vaddr = conn->so_pcb)) {
        netdump(NT_INPCB, vaddr);
        pcb = vtol(X_INPCB, vaddr);
        if ((pcb != (struct inpcb *) -1) && (vaddr = pcb->inp_ppcb)) {
	    int laddr;
	    if ((laddr = vtol(X_TCP_CB, vaddr)) != -1)
                fprintf(outf, "\n  tcpcb at address 0x%08x:\n\n", vaddr);
	    else if ((laddr = vtol(X_PXP_CB, vaddr)) != -1)
                fprintf(outf, "\n  pxpcb at address 0x%08x:\n\n", vaddr);
	    if (laddr != -1)	    
                netdump_pcb(laddr);
	}
    }
}



void netdump_dest(dest)
register struct dest_desc *dest;
{
    NDBG(("netdump_dest(%#x)", dest));

    fprintf(outf,
	"  dd_next      0x%08x  dd_prev     0x%08x   dd_kind    0x%08x\n",
        dest->dd_next, dest->dd_prev, dest->dd_kind);
    fprintf(outf,
	"  dt_addr      0x%08x  dt_services 0x%08x   dt_proto   0x%08x\n",
        dest->dd_tuple.dt_addr, dest->dd_tuple.dt_services,
	dest->dd_tuple.dt_proto);
    fprintf(outf,
	"  dt_path_rep  0x%08x\n", dest->dd_path_report);
}



static struct netflags socket_types[] = {
	SOCK_STREAM,	"SOCK_STREAM",
	SOCK_DGRAM,	"SOCK_DGRAM",
	SOCK_RAW,	"SOCK_RAW",
	SOCK_RDM,	"SOCK_RDM",
	SOCK_SEQPACKET,	"SOCK_SEQPACKET",
	SOCK_REQUEST,	"SOCK_REQUEST",
	SOCK_REPLY,	"SOCK_REPLY",
	SOCK_DEST,	"SOCK_DEST",
	0, 0
};

static struct netflags socket_options[] = {
	SO_DEBUG,		"SO_DEBUG",
	SO_ACCEPTCONN,		"SO_ACCEPTCONN",
	SO_REUSEADDR,		"SO_REUSEADDR",
	SO_KEEPALIVE,		"SO_KEEPALIVE",
	SO_DONTROUTE,		"SO_DONTROUTE",
	SO_USELOOPBACK,		"SO_USELOOPBACK",
	SO_LINGER,		"SO_LINGER",
	SO_MSG_MODE,		"SO_MSG_MODE",
	SO_MUX_VC,		"SO_MUX_VC",
	SO_OPTIONAL_CKSUM,	"SO_OPTIONAL_CKSUM",
	SO_ABRUPT_CLOSE,	"SO_ABRUPT_CLOSE",
	SO_NS_SELECT,		"SO_NS_SELECT",
	SO_SIGNAL_EVENT,		"SO_SIGNAL_EVENT", 
	/* SO_SIGNAL_OOB,		"SO_SIGNAL_OOB", for pre 2.0 */
	0, 0
};

static struct netflags socket_states[] = {
	SS_ISCONNECTED,		"SS_ISCONNECTED",
	SS_ISCONNECTING,	"SS_ISCONNECTING",
	SS_ISDISCONNECTING,	"SS_ISDISCONNECTING",
	SS_CANTSENDMORE,	"SS_CANTSENDMORE",
	SS_CANTRCVMORE,		"SS_CANTRCVMORE",
	SS_RCVATMARK,		"SS_RCVATMARK",
	SS_PRIV,		"SS_PRIV",
	SS_NBIO,		"SS_NBIO",
	SS_ASYNC,		"SS_ASYNC",
	SS_AWAITING_NSRECV,	"SS_AWAITING_NSRECV",
#if !defined(R91) && !defined(RVAX) && !defined(R10) && !defined(R11)
	SS_LOCK,		"SS_LOCK",
	SS_WANT,		"SS_WANT",
#endif 
	0, 0
};



void netdump_socket(socket)
register struct socket *socket;
{
    NDBG(("netdump_socket(%#x)", socket));

    if (socket_conn) {
	socket_conn = 0;
	netdump_conn(socket);
	socket_conn = 1;
	return;
    }
    
    fprintf(outf,
	"  so_type      0x%08x  so_options  0x%08x   so_refcnt  0x%08x\n",
        socket->so_type, socket->so_options, socket->so_refcnt);
    fprintf(outf,
        "  so_name      0x%08x\n",
        socket->so_name);

    if (socket->so_type == SOCK_DEST) {
	netdump_dest(socket);
        netdump_value(socket_types, socket->so_type);
        netdump_flags(socket_options, socket->so_options);
	return;
    }

    fprintf(outf,
        "  so_linger    0x%08x  so_state    0x%08x\n", 
        socket->so_linger, socket->so_state);
    fprintf(outf,
        "  so_error     0x%08x  so_pcb      0x%08x   so_proto   0x%08x\n", 
        socket->so_error, socket->so_pcb, socket->so_proto);
#ifdef NEWBM
    fprintf(outf,
        "  so_head      0x%08x  so_q0       0x%08x\n", 
        socket->so_head, socket->so_q0);
#else  NEWBM
    fprintf(outf,
        "  so_sock_bufs 0x%08x  so_head     0x%08x   so_q0      0x%08x\n", 
        socket->so_sock_bufs, socket->so_head, socket->so_q0);
#endif NEWBM
    fprintf(outf,
        "  so_q         0x%08x  so_q0len    0x%08x   so_qlen    0x%08x\n", 
        socket->so_q, socket->so_q0len, socket->so_qlen);
    fprintf(outf,
        "  so_qlimit    0x%08x  so_timeo    0x%08x   so_oobmark 0x%08x\n", 
        socket->so_qlimit, socket->so_timeo, socket->so_oobmark);
    fprintf(outf,
        "  so_pgrp      0x%08x\n", socket->so_pgrp);
    netdump_value(socket_types, socket->so_type);
    netdump_flags(socket_options, socket->so_options);
    netdump_flags(socket_states, socket->so_state);
#ifdef NEWBM
    fprintf(outf, "  so_rcv:\n");
    netdump_sockbuf(&socket->so_rcv);
    fprintf(outf, "  so_snd:\n");
    netdump_sockbuf(&socket->so_snd);
#endif NEWBM
}



static struct netflags sockbuf_flags[] = {
	SB_LOCK,	"SB_LOCK",
	SB_WANT,	"SB_WANT",
	SB_WAIT,	"SB_WAIT",
	SB_COLL,	"SB_COLL",
	SB_MSG_MODE,	"SB_MSG_MODE",
	SB_DATAGRAM_OUT,"SB_DATAGRAM_OUT",
	SB_SEQNUM,	"SB_SEQNUM",
	SB_OUTBOUND,	"SB_OUTBOUND",
	SB_INBOUND,	"SB_INBOUND",
	SB_RW_THRESHOLD,"SB_RW_THRESHOLD",
	0, 0
};

void netdump_sockbuf(sockbuf)
register struct sockbuf *sockbuf;
{
    NDBG(("netdump_sockbuf(%#x)", sockbuf));
    
    fprintf(outf,
    "    sb_cc       0x%04hx  sb_flags    0x%04hx   sb_msgsqd    0x%04hx\n",
        sockbuf->sb_cc, sockbuf->sb_flags, sockbuf->sb_msgsqd);
    fprintf(outf,
    "    sb_maxmsgs  0x%04hx  sb_msgsize  0x%04hx   sb_threshold 0x%04hx\n",
        sockbuf->sb_maxmsgs, sockbuf->sb_msgsize, sockbuf->sb_threshold);
    fprintf(outf,
    "    sb_mbpmsg   0x%04hx  sb_seqoff   0x%04hx\n",
        sockbuf->sb_mbpmsg, sockbuf->sb_seqoff);
    fprintf(outf,
    "    sb_sel      0x%08x   sb_macct    0x%08x   sb_lastmsg   0x%08x\n", 
        sockbuf->sb_sel, sockbuf->sb_macct, sockbuf->sb_lastmsg);
    netdump_flags(sockbuf_flags, sockbuf->sb_flags);
}



/* dump contents of an MT_PCB */
void netdump_pcb(pcb)
char *pcb;
{
    int ntyp;

    NDBG(("netdump_pcb(%#x)", pcb));

    ntyp = vton((uint) pcb - LA(X_MBUTL, uint) + VA(X_MBUTL, uint));
    switch (ntyp) {
        case NT_INPCB:
		netdump_inpcb(pcb);
		break;
        case NT_TCPCB:
		netdump_tcpcb(pcb);
		break;
        case NT_PXPCB:
		netdump_pxpcb(pcb);
		break;
	default:
		n_dump_nop(pcb);
		break;
    };
}



/* dump contents of pr_ntab */
void netdump_prntab(prntab)
struct ntab *prntab;
{
    char nname[NS_MAX_NODE_NAME+1];

    NDBG(("netdump_prntab(%#x)", prntab));

    strncpy(nname, prntab->nt_name, NS_MAX_NODE_NAME);
    nname[prntab->nt_length > NS_MAX_NODE_NAME ? 
	NS_MAX_NODE_NAME : prntab->nt_length] = '\0';
    fprintf(outf,
        "  nt_name    \"%s\"\n", nname);
    fprintf(outf,
"  nt_flags   0x%02.2x       nt_rtcnt     0x%02.2x        nt_length   0x%08x\n",
        prntab->nt_flags & 0xff, prntab->nt_rtcnt & 0xff, prntab->nt_length);
    fprintf(outf,
"  nt_path    0x%08x nt_nqnamelen 0x%08x  nt_pnrntype 0x%04hx\n",
        prntab->nt_path, prntab->nt_nqnamelen, prntab->nt_pnrntype);
    fprintf(outf,
"  nt_pnrqlen 0x%04hx     nt_seqno     0x%04hx      nt_timer    0x%04hx\n",
        prntab->nt_pnrqlen, prntab->nt_seqno, prntab->nt_timer);
    fprintf(outf,
	"  nt_refcnt  0x%08x\n", prntab->nt_refcnt);
}



static struct netflags prvnatab_flags[] = {
    PR_INUSE,	    "PR_INUSE",
    PR_COM,	    "PR_COM",
    ATF_INUSE,	    "ATF_INUSE",
    ATF_COM,	    "ATF_COM",
    0, 0
};

/* dump contents of pr_vnatab */
void netdump_prvnatab(prvnatab)
struct vnatab *prvnatab;
{
    int i;

    NDBG(("netdump_prvnatab(%#x)", prvnatab));

    fprintf(outf,
	"  vt_iaddr  0x%08x   vt_laddr  0x", 
	prvnatab->vt_iaddr);
    for (i = 0; i < LNK_ADDR_SIZE; i++) 
	fprintf(outf, "%02.2x", prvnatab->vt_laddr[i] & 0xff);
    fprintf(outf,
"\n  vt_timer  0x%04hx       vt_rtcnt  0x%02.2x         vt_flags  0x%02.2x\n",
	prvnatab->vt_timer, prvnatab->vt_rtcnt & 0xff, 
	prvnatab->vt_flags & 0xff);
    fprintf(outf,
	"  vt_seqno  0x%08x   vt_if     0x%08x\n", 
	prvnatab->vt_seqno, prvnatab->vt_if);
    fprintf(outf,
"  parm_ssap 0x%02.2x         parm_ctl8023 0x%02.2x      parm_sxsap  0x%04hx\n", 
	prvnatab->vt_po.if8023parm_ssap,
	prvnatab->vt_po.if8023parm_ctl8023,
	prvnatab->vt_po.if8023parm_sxsap);
    fprintf(outf, "  vt_packet 0x%08x\n", 
	prvnatab->vt_packet);
    netdump_sockaddr(&(prvnatab->vt_po.if8023parm_dst_sockaddr));
    if (prvnatab->vt_flags)
        netdump_flags(prvnatab_flags, prvnatab->vt_flags);
}



/* dump contents of protosw */
void netdump_protosw(protosw)
struct protosw *protosw;
{
    NDBG(("netdump_protosw(%#x)", protosw));

    fprintf(outf,
       "  pr_type     0x%08x  pr_family   0x%08x   pr_protocol  0x%08x\n",
          protosw->pr_type, protosw->pr_family, protosw->pr_protocol);
    fprintf(outf,
       "  pr_flags    0x%08x  pr_kinds    0x%08x   pr_services  0x%08x\n",
          protosw->pr_flags, protosw->pr_kinds, protosw->pr_services);
    fprintf(outf,
       "  pr_pid      0x%08x\n\n", protosw->pr_pid);
    fprintf(outf,
       "  pr_extract_path_elem    0x%08x  %s\n",
          protosw->pr_extract_path_elem, 
          addr_to_sym(protosw->pr_extract_path_elem));
    fprintf(outf,
       "  pr_add_path_elem        0x%08x  %s\n",
          protosw->pr_add_path_elem, 
          addr_to_sym(protosw->pr_add_path_elem));
    fprintf(outf,
       "  pr_input                0x%08x  %s\n",
          protosw->pr_input, addr_to_sym(protosw->pr_input));
    fprintf(outf,
       "  pr_output               0x%08x  %s\n",
          protosw->pr_output, addr_to_sym(protosw->pr_output));
    fprintf(outf,
       "  pr_ctloutput            0x%08x  %s\n",
          protosw->pr_ctloutput, addr_to_sym(protosw->pr_ctloutput));
    fprintf(outf,
       "  pr_usrreq               0x%08x  %s\n",
          protosw->pr_usrreq, addr_to_sym(protosw->pr_usrreq));
    fprintf(outf,
       "  pr_init                 0x%08x  %s\n",
          protosw->pr_init, addr_to_sym(protosw->pr_init));
    fprintf(outf,
       "  pr_fasttimo             0x%08x  %s\n",
          protosw->pr_fasttimo, addr_to_sym(protosw->pr_fasttimo));
    fprintf(outf,
       "  pr_slowtimo             0x%08x  %s\n",
          protosw->pr_slowtimo, addr_to_sym(protosw->pr_slowtimo));
    fprintf(outf,
       "  pr_drain                0x%08x  %s\n",
          protosw->pr_drain, addr_to_sym(protosw->pr_drain));
}




static struct netflags lanift_flags[] = {
    LAN_STAT,		"LAN_STAT",
    LAN_STATPENDING,	"LAN_STATPENDING",
    LAN_INIT,		"LAN_INIT",
    LAN_DEAD,		"LAN_DEAD",
    LAN_IOCTL,		"LAN_IOCTL",
    LAN_IOCTL_WANTED,	"LAN_IOCTL_WANTED",
    LAN_IOCTL_DONE,	"LAN_IOCTL_DONE",
    LAN_INIT_WANTED,	"LAN_INIT_WANTED",
    ARQ_WHEN_CAM_BUSY,	"ARQ_WHEN_CAM_BUSY",
    0, 0
};

static struct netflags lanift_arq_flags[] = {
0x00,	"A Read Data has been posted to the LAN card.",
0x10,	"An AES has been received by the LAN driver.",
0x60,	"DOD: Product Verification TENA Test passed successfully .",
0x61,	"DOD: RAM image of NVRAM was corrupted before store operation started\
     (NVRAM unchanged).",
0x62,	"DOD: Internal multicast list sort error.",
0x63,	"DOD: 128 attempts to transmit a packet due to 1/2 second timeouts.",
0x64,	"DOD: Backplane card to system DMA didn't complete within 1.28 seconds.",
0x65,	"DOD: Backplane system to card DMA didn't complete within 1.28 seconds.",
0x66,	"DOD: LANCE Tx BUFF error (unknown length packet transmitted).",
0x67,	"DOD: LANCE Tx UFLO error (truncated packet early; late access to RAM).",
0x68,	"DOD: LANCE Rx OFLO error (lost packet due to late access to RAM).",
0x69,	"DOD: LANCE Rx data chain error (packet > 1518 bytes arrived).",
0x6A,	"DOD: LANCE Rx length error (e.g. RMD3 length < 0 with RMD2 = 1518).",
0x6B,	"DOD: LANCE Rx length error (e.g. RMD3 length > RMD2 with RMD2 = 1518).",
0x6C,	"DOD: LANCE reported transmitting a packet greater than 1518 bytes.",
0x6D,	"DOD: LANCE as Bus Master couldn't access RAM after 26.5 microseconds.",
0x6E,	"DOD: 128 attempts to correct a LANCE revision B Tx babble error.",
0x6F,	"DOD: Spurious (unknown origin) interrupt to 68000 microprocessor.",
0x70,	"PER: Product Verification TENA Test failed.",
0x71,	"PER: Unknown Write Control opcode or 1 byte Write Control buffer.",
0x72,	"PER: Off-line Write Control request while card is on-line.",
0x73,	"PER: WC (Receive Filter) buffer not exactly 4 bytes.",
0x74,	"PER: WC (Nodal Address) buffer not exactly 8 bytes.",
0x75,	"PER: WC (Multicast List) buffer not equal to 2+6*<address> formula.",
0x76,	"PER: WC (Clear Individual Statistic) buffer not exactly 4 bytes.",
0x77,	"PER: Bad WC parameter (e.g. individual address in multicast list\
     bad or clear individual statistic counter index < 0 or > 19).",
0x78,	"PER: Write Data order while card is in off-line state.",
0x79,	"PER: Write Data buffer less than minimum (e.g. less than 14 bytes).",
0x7A,	"PER: Write Data order when no free transmit buffers available.",
0x7B,	"PER: Unknown CIO order arrived.",
0x7C,	"PER: DSC subchannel number doesn't match assigned subchannel number.",
0x7D,	"PER: Another Create Subchannel received when already assigned a\
     subchannel.",
0x7E,	"PER: Unknown CIO command arrived.",
0x7F,	"PER: DMA write over-run (e.g. system sending more than 1514 bytes\
     for WD orders or 386 bytes for WC orders).",
0, 0
};



/*** DUMP CONTENTS OF LANIFTS' ***/
/*** As of 3.0, analyze supports both CIO and NIO card interfaces
 *** and therefore needs to dump the common lanift as well as the
 *** ift structures that are set up for lan1 and lan0.  The current
 *** driver design does not allow NIO & CIO cards to coexist within
 *** the same kernel, so if num_lan0 = 0 then the system must be
 *** using a NIO card (and num_lan1 >0) ***/

void netdump_lanift(lanift_p)
lan_ift *lanift_p;
{

    register int i;

    NDBG(("netdump_lanift(%#x)", lan_ift));
/*********************************************
 * print out the ifnet structure for this card
 *********************************************/
    if (num_lan0)
       fprintf(outf,"CIO interface\n");
    else
       fprintf(outf,"NIO interface\n");
    fprintf(outf, "  is_if:\n");
    netdump_ifnet(&(lanift_p->is_if));


/******************************
 * print out the arpcom address
 ******************************/
    fprintf(outf, "\n\n  ac_ac     0x%08x   is_addr     0x", 
	lanift_p->is_ac.ac_ac,
        lanift_p->is_addr);
    for (i = 0; i < 6; i++)
	fprintf(outf, "%02.2x", lanift_p->is_addr[i] & 0xff);
    fprintf(outf, "\n");
    
    fprintf(outf,
	"  flags     0x%08x   is_scani. 0x%08x   statbuf     0x%08x\n", 
	lanift_p->flags, lanift_p->is_scaninterval, lanift_p->statbuf);
    fprintf(outf, "\n  xtofree:  ");
    for (i = 0; i < 4; i++)
	fprintf(outf, "0x%08x    ", lanift_p->xtofree[i]);
    fprintf(outf, "\n");
    fprintf(outf, "  mbuf_rd:  ");
    for (i = 0; i < 4; i++)
	fprintf(outf, "0x%08x    ", lanift_p->mbuf_rd[i]);
    fprintf(outf, "\n\n");
    fprintf(outf,
	"  BAD_CONTROL  0x%08x  UNKNOWN_PROTO  0x%08x  RXD_XID      0x%08x\n",
	lanift_p->BAD_CONTROL, lanift_p->UNKNOWN_PROTO, lanift_p->RXD_XID);
    fprintf(outf,
	"  RXD_TEST     0x%08x  RXD_SPEC_DROP  0x%08x  LAN_ARQ_STAT 0x%08x\n",
	lanift_p->RXD_TEST, lanift_p->RXD_SPECIAL_DROPPED,
	lanift_p->LAN_ARQ_STATUS);

    netdump_value(lanift_arq_flags, lanift_p->LAN_ARQ_STATUS);

    fprintf(outf,
    "\n  num_pkt_onboard          0x%08x         num_outbuf_onboard   0x%08x\n",
	lanift_p->num_pkt_onboard, lanift_p->num_outbuf_onboard);
    fprintf(outf,
    "  num_outbuf_onboard       0x%08x         num_pkt_sent         0x%08x\n",
	lanift_p->num_outbuf_onboard, lanift_p->num_pkt_sent);
    fprintf(outf,
    "  num_pkt_read             0x%08x         num_multicast_addr   0x%08x\n",
	lanift_p->num_pkt_read, lanift_p->num_multicast_addr);

    fprintf(outf,
        "  mid        0x%08x  tid           0x%08x  timer       0x%08x\n",
	lanift_p->mid, lanift_p->tid, lanift_p->lantimer.timer);

    fprintf(outf,
        "  lan_port   0x%08x          pda         0x%08x\n",
	lanift_p->lan_port, lanift_p->pda);

    netdump_flags(lanift_flags, lanift_p->flags);

}



void netdump_ifqueue(ifqueue)
struct ifqueue *ifqueue;
{
    register int i;

    fprintf(outf, "  ifq_slot:");
    for (i = 0; i < IFQ_MAXLEN; i++) {
	if ((i % 6) == 0)
            fprintf(outf,"\n  ");
        fprintf(outf, "  0x%08x", ifqueue->ifq_slot[i]);
    }
    fprintf(outf,
        "\n  ifq_head    0x%08x  ifq_tail    0x%08x\n",
	ifqueue->ifq_head, ifqueue->ifq_tail);
    fprintf(outf,
        "  ifq_len     0x%08x  ifq_maxlen  0x%08x   ifq_drops 0x%08x\n",
        ifqueue->ifq_len, ifqueue->ifq_maxlen, ifqueue->ifq_drops);
}



void netdump_ipq(ipq)
register struct ipq *ipq;
{
    fprintf(outf,
        "  next        0x%08x  prev        0x%08x\n",
	ipq->next, ipq->prev);
    fprintf(outf,
        "  ipq_ttl     0x%02.2x        ifq_p       0x%02.2x",
       ipq->ipq_ttl & 0xff, ipq->ipq_p & 0xff);
    fprintf(outf,
        "       ifq_id  0x%04hx\n", ipq->ipq_id);
    fprintf(outf,
        "  ipq_next    0x%08x  ipq_prev    0x%08x\n",
	ipq->ipq_next, ipq->ipq_prev);
    fprintf(outf, 
	"  ipq_src     0x%08x  ipq_dst     0x%08x\n",
	ipq->ipq_src.s_addr, ipq->ipq_dst.s_addr);
}



void netdump_ipasfrag(ipf)
register struct ipasfrag *ipf;
{
    fprintf(outf, 
	"  ip_v         0x%02.2x        ip_hl        0x%02.2x\n",
	ipf->ip_v, ipf->ip_hl);
    fprintf(outf, 
"  ip_tos       0x%02.2x        ip_len       0x%04hx      ip_id       0x%04hx\n",
	ipf->ip_tos & 0xff, ipf->ip_len, ipf->ip_id);
    fprintf(outf, 
"  ip_off       0x%04hx      ip_ttl       0x%02.2x        ip_p        0x%02.2x\n",
	ipf->ip_off, ipf->ip_ttl, ipf->ip_p);
    fprintf(outf, 
	"  ip_sum       0x%04hx\n",
	ipf->ip_sum);
    fprintf(outf, 
	"  ipf_next     0x%08x  ipf_prev     0x%08x\n",
	ipf->ipf_next, ipf->ipf_prev);
}



static struct netflags ifnet_flags[] = {
    IFF_UP,		"IFF_UP",
    IFF_BROADCAST,	"IFF_BROADCAST",
    IFF_DEBUG,		"IFF_DEBUG",
    IFF_ROUTE,		"IFF_ROUTE",
    IFF_POINTOPOINT,	"IFF_POINTOPOINT",
    IFF_NOTRAILERS,	"IFF_NOTRAILERS",
    IFF_RUNNING,	"IFF_RUNNING",
    IFF_NOARP,		"IFF_NOARP",
    IFF_LOCAL_LOOPBACK,	"IFF_LOCAL_LOOPBACK",
    IFF_IEEE,		"IFF_IEEE",
    IFF_ETHER,		"IFF_ETHER",
    0, 0
};

/* dump contents of ifnet */
void netdump_ifnet(ifnet)
struct ifnet *ifnet;
{
    NDBG(("netdump_ifnet(%#x)", ifnet));

    fprintf(outf,
	"  if_name     0x%08x  \"%s\"\n", 
	ifnet->if_name, net_getstr(ifnet->if_name));
    fprintf(outf,
        "  if_unit     0x%04hx      if_mtu      0x%04hx     if_next  0x%08x\n",
        ifnet->if_unit, ifnet->if_mtu, ifnet->if_next);
    fprintf(outf, "  if_flags    0x%04hx      if_timer    0x%04hx",
        ifnet->if_flags, ifnet->if_timer);
#ifdef SIOCGIFNETMASK 
    /* This is a subnetted kernel */
    fprintf(outf,
        "\n  if_net      0x%08x  if_netmask    0x%08x\n",
        ifnet->if_net, ifnet->if_netmask);
    fprintf(outf,
        "  if_subnet   0x%08x  if_subnetmask 0x%08x\n",
        ifnet->if_subnet, ifnet->if_subnetmask);
#else
    fprintf(outf, "     if_net   0x%08x\n", ifnet->if_net);
#endif SIOCGIFNETMASK
    fprintf(outf,
	"  if_host     0x%08x  0x%08x\n",
	ifnet->if_host[0], ifnet->if_host[1]);
    netdump_ifqueue(&(ifnet->if_snd));
#ifndef NEWBM
    fprintf(outf,
        "  if_macct    0x%08x\n",
	ifnet->if_macct);
#endif NEWBM
    netdump_flags(ifnet_flags, ifnet->if_flags);
    fprintf(outf, "\n");

    fprintf(outf,
        "  if_init         0x%08x   %s\n",
          ifnet->if_init, addr_to_sym(ifnet->if_init));
    fprintf(outf,
        "  if_output       0x%08x   %s\n",
          ifnet->if_output, addr_to_sym(ifnet->if_output));
    fprintf(outf,
        "  if_ioctl        0x%08x   %s\n",
          ifnet->if_ioctl, addr_to_sym(ifnet->if_ioctl));
    fprintf(outf,
        "  if_reset        0x%08x   %s\n",
          ifnet->if_reset, addr_to_sym(ifnet->if_reset));
    fprintf(outf,
        "  if_watchdog     0x%08x   %s\n\n",
          ifnet->if_watchdog, addr_to_sym(ifnet->if_watchdog));

    fprintf(outf,
        "  if_ipackets     0x%08x   if_ierrors    0x%08x\n",
        ifnet->if_ipackets, ifnet->if_ierrors);
    fprintf(outf,
        "  if_opackets     0x%08x   if_oerrors    0x%08x\n",
        ifnet->if_opackets, ifnet->if_oerrors);
    fprintf(outf,
	"  if_collisions   0x%08x\n", ifnet->if_collisions);
}



/* dump contents of inpcb */
void netdump_inpcb(inpcb)
struct inpcb *inpcb;
{
    NDBG(("netdump_inpcb(%#x)", inpcb));

    fprintf(outf,
	"  inp_next     0x%08x  inp_prev     0x%08x  inp_head     0x%08x\n", 
	inpcb->inp_next, inpcb->inp_prev, inpcb->inp_next);
    fprintf(outf,
	"  inp_faddr    0x%08x  inp_laddr    0x%08x\n",
	inpcb->inp_faddr.s_addr, inpcb->inp_laddr.s_addr);
    fprintf(outf,
	"  inp_fport    0x%04hx      inp_lport    0x%04hx\n",
	inpcb->inp_fport, inpcb->inp_lport);
    fprintf(outf,
	"  inp_socket   0x%08x  inp_ppcb     0x%08x\n",
	inpcb->inp_socket, inpcb->inp_ppcb);
    fprintf(outf,
	"  inp_ttl      0x%02x        inp_tos      0x%02x\n",
	inpcb->inp_ttl & 0xff, inpcb->inp_tos & 0xff);
    fprintf(outf,
	"  inp_security 0x%04hx      inp_compart  0x%04hx",
	inpcb->inp_security, inpcb->inp_compartment);
    fprintf(outf,
	"      inp_handling 0x%04hx\n", 
	inpcb->inp_handling);
    fprintf(outf,
	"  inp_tcc      0x%08x\n",
	inpcb->inp_tcc);
    fprintf(outf, "\n");
    netdump_route(&(inpcb->inp_route));
}



static struct netflags tcp_states[] = {
	TCPS_CLOSED,		"TCPS_CLOSED",
	TCPS_LISTEN,		"TCPS_LISTEN",
	TCPS_SYN_SENT,		"TCPS_SYN_SENT",
	TCPS_SYN_RECEIVED,	"TCPS_SYN_RECEIVED",
	TCPS_ESTABLISHED,	"TCPS_ESTABLISHED",
	TCPS_CLOSE_WAIT,	"TCPS_CLOSE_WAIT",
	TCPS_FIN_WAIT_1,	"TCPS_FIN_WAIT_1",
	TCPS_CLOSING,		"TCPS_CLOSING",
	TCPS_LAST_ACK,		"TCPS_LAST_ACK",
	TCPS_FIN_WAIT_2,	"TCPS_FIN_WAIT_2",
	TCPS_TIME_WAIT,		"TCPS_TIME_WAIT",
	0, 0
};

static struct netflags tcp_conopts[] = {
	FORCEOUT,	"FORCEOUT",
	NOCHKSUM,	"NOCHKSUM",
	0, 0
};

static struct netflags tcp_flags[] = {
	TF_ACKNOW,	"TF_ACKNOW",
	TF_DELACK,	"TF_DELACK",
	TF_DONTKEEP,	"TF_DONTKEEP",
	TF_NOOPT,	"TF_NOOPT",
	TF_MMREXMT,	"TF_MMREXMT",
	0, 0
};

/* dump contents of tcpcb */
void netdump_tcpcb(tcpcb)
struct tcpcb *tcpcb;
{
    register int i;

    NDBG(("netdump_tcpcb(%#x)", tcpcb));

    fprintf(outf,
	"  seg_next     0x%08x  seg_prev    0x%08x  t_state     0x%04hx\n", 
	tcpcb->seg_next, tcpcb->seg_prev, tcpcb->t_state);
    fprintf(outf, 
	"  t_timer:     ");
    for (i = 0; i < TCPT_NTIMERS; i++) {
	if ((i + 1) % 5 == 0) 
	    fprintf(outf, "\n");
	fprintf(outf, "0x%04hx      ", tcpcb->t_timer[i]);
    }
    fprintf(outf,
  "\n  t_rxtshift   0x%08x  t_maxseg    0x%04hx      t_conopts   0x%02.2x\n", 
	tcpcb->t_rxtshift, tcpcb->t_maxseg, tcpcb->t_conopts & 0xff);
    fprintf(outf, 
	"  t_tcpopt     0x%08x  t_ipopt     0x%08x  t_mmtblp    0x%08x\n", 
	tcpcb->t_tcpopt, tcpcb->t_ipopt, tcpcb->t_mmtblp);
    netdump_value(tcp_states, tcpcb->t_state);
    if (tcpcb->t_conopts)
        netdump_flags(tcp_conopts, tcpcb->t_conopts);
    if (tcpcb->t_flags)
        netdump_flags(tcp_flags, tcpcb->t_flags);
    fprintf(outf, 
	"  t_template   0x%08x  t_inpcb     0x%08x  snd_wnd     0x%04hx\n", 
	tcpcb->t_template, tcpcb->t_inpcb, tcpcb->snd_wnd);
    fprintf(outf, 
	"  snd_una      0x%08x  snd_nxt     0x%08x  snd_up      0x%08x\n", 
	tcpcb->snd_una, tcpcb->snd_nxt, tcpcb->snd_up);
    fprintf(outf, 
	"  snd_wl1      0x%08x  snd_wl2     0x%08x  iss         0x%08x\n", 
	tcpcb->snd_wl1, tcpcb->snd_wl2, tcpcb->iss);
    fprintf(outf, 
	"  rcv_up       0x%08x  rcv_nxt     0x%08x  rcv_wnd     0x%04hx\n", 
	tcpcb->rcv_up, tcpcb->rcv_nxt, tcpcb->rcv_wnd);
    fprintf(outf, 
	"  rcv_adv      0x%08x  snd_max     0x%08x  irs         0x%08x\n", 
	tcpcb->rcv_adv, tcpcb->snd_max, tcpcb->irs);
    fprintf(outf, 
    "  t_rtseq      0x%08x  t_idle      0x%04hx      t_rtt       0x%04hx\n", 
	tcpcb->t_rtseq, tcpcb->t_idle, tcpcb->t_rtt);
    fprintf(outf, 
"  t_srtt         %8.2f  t_oobflags  0x%02.2x        t_iobc      0x%02.2x\n", 
	tcpcb->t_srtt, tcpcb->t_oobflags & 0xff, tcpcb->t_iobc & 0xff);
    fprintf(outf, 
        "  snd_cwnd     0x%08x\n",
	tcpcb->snd_cwnd);
}



/* dump contents of pxpcb */
void netdump_pxpcb(pxpcb)
struct pxpcb *pxpcb;
{
    register int i;

    NDBG(("netdump_pxpcb(%#x)", pxpcb));

    fprintf(outf,
	"  xp_next      0x%08x  xp_prev     0x%08x  xp_xpopt    0x%08x\n", 
	pxpcb->xp_next, pxpcb->xp_prev, pxpcb->xp_xpopt);
    fprintf(outf,
	"  xp_ipopt     0x%08x  xp_inpcb    0x%08x  xp_sin_port 0x%04hx\n", 
	pxpcb->xp_ipopt, pxpcb->xp_inpcb, pxpcb->xp_sin_port);
/*	  xp_sin_addr pre ic3 rel2.0 , xp_sin_laddr rel2.0 ic3 */
    fprintf(outf,
	"  xp_sin_laddr  0x%08x\n", pxpcb->xp_sin_laddr);
    fprintf(outf,
"  xp_timer     0x%04hx      xp_uresndtm 0x%04hx      xp_rxtshift 0x%04hx\n", 
	pxpcb->xp_timer, pxpcb->xp_uresndtm, pxpcb->xp_rxtshift);
    fprintf(outf,
"  xp_uretry    0x%04hx      xp_creq_num 0x%04hx      xp_ureq_cnt 0x%04hx\n", 
	pxpcb->xp_uretry, pxpcb->xp_creq_num, pxpcb->xp_ureq_cnt);
    fprintf(outf,
	"  xp_msg_len   0x%08x  xp_sndmsgid 0x%08x  xp_rcvmsgid 0x%08x\n", 
	pxpcb->xp_msg_len, pxpcb->xp_snd_msgid, pxpcb->xp_rcv_msgid);
}



void netdump_route(route)
struct route *route;
{
    /* netdump_rtentry(route->ro_rt); */
    fprintf(outf,"  ro_dst:\n");
    netdump_sockaddr(&(route->ro_dst));
}



void netdump_rtentry(rtentry)
struct rtentry *rtentry;
{
    fprintf(outf,"  rt_dst:\n");
    netdump_sockaddr(&(rtentry->rt_dst));
    fprintf(outf,"  rt_gateway:\n");
    netdump_sockaddr(&(rtentry->rt_gateway));
}



void netdump_sockaddr(sockaddr)
struct sockaddr *sockaddr;
{
    register int i;

    fprintf(outf,
	"  sa_family   0x%04hx   sa_data   0x", 
	sockaddr->sa_family);
    for (i = 0; i < SOCK_ADDR_DATA_LEN; i++)
	fprintf(outf, "%02.2x", sockaddr->sa_data[i] & 0xff);
    fprintf(outf,"\n");
}



void netdump_sapinput(sap_input)
struct sap_input *sap_input;
{
    fprintf(outf,
        "  inq      0x%08x   %s\n", 
          sap_input->inq, addr_to_sym(sap_input->inq));
    fprintf(outf,
        "  rint     0x%08x   %s\n",
          sap_input->rint, addr_to_sym(sap_input->rint));
}



void netdump_xsaplink(xsaplink)
struct xsap_link *xsaplink;
{
    fprintf(outf,
        "  m        0x%08x\n", xsaplink->m);
    fprintf(outf,
        "  next     0x%08x   prev     0x%08x\n",
        xsaplink->next, xsaplink->prev);
    netdump_sapinput(&(xsaplink->log.input));
    fprintf(outf,
        "  dxsap    0x%04hx       dsap     0x%02.2x\n",
        xsaplink->log.dxsap, (xsaplink->log.dsap) & 0xff);
}


/* dump contents of ifnet */
void netdump_lan1ift(lan1ift)

  lan1_ift_t   *lan1ift;
{ lan1_cb_t    *cb_buf;
  lan1_pquad_t *pq_buf;
  lan1_pquad_t *scratch_ptr;

  int cb_size;
  int pq_size;
  int option;
  char a_char;

  register int i;

  cb_size = sizeof(*cb_buf);
  pq_size = sizeof(*pq_buf);
  cb_buf = malloc(cb_size);
  pq_buf = malloc(pq_size);
  option =1;
  while(option != 0)
    {
     printf("\n\nLAN1IFT OPTIONS:\n");
     printf("  0) Exit\n");
     printf("  1) Hardware State & Event Flags\n");
     printf("  2) State/Event Tracing\n");
     printf("  3) Card data\n");
     printf("  4) Hardware Errors\n");
     printf("  5) Read Info\n");
     printf("  6) Write Info\n");
     printf("  7) Control Info\n\n");
     printf("Select option ");

     scanf("%d",&option);
     switch(option)
       {
	case 0: break;
	case 1:
          fprintf(outf,"hw state: (0x%08x)\n",lan1ift->hw_state);
          if (lan1ift->hw_state & LAN1_HW_IO) fprintf(outf,"  IO ");
          if (lan1ift->hw_state & LAN1_HW_CTL) fprintf(outf,"  CTL ");
  	  if (lan1ift->hw_state & LAN1_HW_DEAD) fprintf(outf,"  DEAD");
          fprintf(outf,"\n");

          fprintf(outf,"hw flags:  (0x%08x)\n",lan1ift->hw_flags);
          if (lan1ift->hw_flags & LAN1_HW_IDLE) fprintf(outf,"  IDLE ");
  	  if (lan1ift->hw_flags & LAN1_HW_RESET) fprintf(outf,"  RESET ");
  	  if (lan1ift->hw_flags & LAN1_HW_STRATEGY_ACT) fprintf(outf,"  STRAT ACT ");
  	  if (lan1ift->hw_flags & LAN1_HW_STRATEGY_REQ) fprintf(outf,"  STRAT REQ");
  	  fprintf(outf,"\n");
  	  fprintf(outf,"\nhw_ctl_seq_ptr: 0x%08x  hw_ctl_index:  %d  reset_timer  %d\n",
	 		lan1ift->hw_ctl_seq_ptr,
	 		lan1ift->hw_ctl_index,
	 		lan1ift->reset_timer);
	  break;

	case 2:
  	  fprintf(outf,"\nState and event tracing:\n");
  	  fprintf(outf,"  trace index = %d\n",lan1ift->hw_trace_index);
  	  fprintf(outf,"  index\tstate\t\tevent\n");
  	  for (i=0; i < 16; i++)
      	    fprintf(outf,"  %2d\t0x%08x\t0x%08x\n",i,
	            lan1ift->hw_loc_trace_array[i],
	            lan1ift->hw_parm_trace_array[i]);
	  break;

	case 3:
  	  fprintf(outf,"\n=====card register pointers and info======\n");
  	  fprintf(outf,
     	  "eim_eir_bit     0x%08x hpa_ptr         0x%08x io_eim_ptr      0x%08x\n",
     	  lan1ift->eim_eir_bit, lan1ift->hpa_ptr, lan1ift->io_eim_ptr);
  	  fprintf(outf,
     	  "io_dc_ptr       0x%08x io_ii_ptr       0x%08x io_flex_ptr     0x%08x\n",
     	  lan1ift->io_dc_ptr, lan1ift->io_ii_ptr, lan1ift->io_flex_ptr);
  	  fprintf(outf,
     	  "io_cmd_ptr      0x%08x io_status_ptr   0x%08x io_dma_link_ptr 0x%08x\n",
     	  lan1ift->io_cmd_ptr, lan1ift->io_status_ptr, lan1ift->io_dma_link_ptr);
  	  fprintf(outf,
     	  "io_dma_cmd_ptr  0x%08x io_dma_addr_ptr 0x%08x io_dma_cnt_ptr  0x%08x\n",
     	  lan1ift->io_dma_cmd_ptr, lan1ift->io_dma_addr_ptr,lan1ift->io_dma_cnt_ptr);
  	  fprintf(outf,
     	  "fw_cmd_ptr      0x%08x fw_status_ptr   0x%08x fw_receive_ptr  0x%08x\n",
     	  lan1ift->fw_cmd_ptr, lan1ift->fw_status_ptr, lan1ift->fw_receive_ptr);
  	  fprintf(outf,
     	  "fw_errors_ptr   0x%08x fw_data_ptr     0x%08x dma_iireenb_ptr 0x%08x\n",
     	  lan1ift->fw_errors_ptr, lan1ift->fw_data_ptr, lan1ift->dma_iireenb_ptr);
  	  fprintf(outf,
     	  "dma_iidsnb_ptr  0x%08x dma_control_ptr 0x%08x\n",
     	  lan1ift->dma_iidisnb_ptr, lan1ift->dma_control_ptr);
	  break;

	case 4:
  	  fprintf(outf,"\n***HARDWARE ERRORS***\n");
  	  fprintf(outf,"er_flags:\tx0%08x\n",lan1ift->er_flags);
  	  fprintf(outf,"er_timer_cnt:\tx0%08x\n",lan1ift->er_timer_cnt);
  	  fprintf(outf,"er_io_status:\tx0%08x\n",lan1ift->er_io_status);
  	  fprintf(outf,"er_io_cmd:\tx0%08x\n",lan1ift->er_io_cmd);
  	  fprintf(outf,"er_fw_cmd:\tx0%08x\n",lan1ift->er_fw_cmd);
  	  fprintf(outf,"er_fw_status:\tx0%08x\n",lan1ift->er_fw_status);
  	  fprintf(outf,"er_fw_error:\tx0%08x\n",lan1ift->er_fw_error);
  	  fprintf(outf,"er_cb:\t\tx0%08x\n",lan1ift->er_cb);
	  break;

	case 5:
/* print out the read quads */
  	  fprintf(outf,"\n****READ CONTROL BLOCK****\n");
  	  netvread(lan1ift->cb_rd_ptr,cb_buf,cb_size);
  	  fprintf(outf,"op_ext\t 0x%08x\n",cb_buf->op_extension);
  	  fprintf(outf,"opcode\t 0x%08x\n",cb_buf->opcode);
  	  fprintf(outf,"length\t 0x%08x\n",cb_buf->length);
  	  fprintf(outf,"\n***READ BUFFER***\n");
  	  fprintf(outf,"state_buf_rd: 0x%08x\n",lan1ift->state_buf_rd);
  	  fprintf(outf,"eoc_rd_index: 0x%08x\n",lan1ift->eoc_rd_index);
  	  fprintf(outf,"head_rd_addr: 0x%08x\n",lan1ift->head_rd_addr);
  	  fprintf(outf,"cb_rd_ptr:    0x%08x\n",lan1ift->cb_rd_ptr);
  	  fprintf(outf,"quads_rd_ptr: 0x%08x\n",lan1ift->quads_rd_ptr);
  	  fprintf(outf,"\n***READ QUADS***\n");
  	  net_print_quads(lan1ift->quads_rd_ptr,pq_buf,pq_size,LAN1_MAX_RD_QUADS);
	  break;

	case 6:
  	  fprintf(outf,"\n****WRITE CONTROL BLOCK****\n");
  	  netvread(lan1ift->cb_wrt_ptr,cb_buf,cb_size);
  	  fprintf(outf,"op_ext\t 0x%08x\n",cb_buf->op_extension);
  	  fprintf(outf,"opcode\t 0x%08x\n",cb_buf->opcode);
  	  fprintf(outf,"length\t 0x%08x\n",cb_buf->length);
  	  fprintf(outf,"\n***WRITE BUFFER***\n");
  	  fprintf(outf,"state_buf_wrt: 0x%08x\n",lan1ift->state_buf_wrt);
  	  fprintf(outf,"eoc_wrt_index: 0x%08x\n",lan1ift->eoc_wrt_index);
  	  fprintf(outf,"head_wrt_addr: 0x%08x\n",lan1ift->head_wrt_addr);
  	  fprintf(outf,"cb_wrt_ptr:    0x%08x\n",lan1ift->cb_wrt_ptr);
  	  fprintf(outf,"quads_wrt_ptr: 0x%08x\n",lan1ift->quads_wrt_ptr);
  	  fprintf(outf,"buflet_wrt_ptr 0x%08x\n",lan1ift->buflet_wrt_ptr);
  	  fprintf(outf,"\n***WRITE QUADS***\n");
  	  net_print_quads(lan1ift->quads_wrt_ptr,pq_buf,pq_size,LAN1_MAX_WRT_QUADS);
	  break;

	case 7:
  	  fprintf(outf,"\n****CONTROL CONTROL BLOCK****\n");
  	  netvread(lan1ift->cb_ctl_ptr,cb_buf,cb_size);
  	  fprintf(outf,"op_ext\t 0x%08x\n",cb_buf->op_extension);
  	  fprintf(outf,"opcode\t 0x%08x\n",cb_buf->opcode);
  	  fprintf(outf,"length\t 0x%08x\n",cb_buf->length);
  	  fprintf(outf,"\n***CONTROL BUFFER***\n");
  	  fprintf(outf,"state_buf_ctl: 0x%08x\t\teoc_ctl_index: 0x%08x\n",
	  	  lan1ift->state_buf_ctl, lan1ift->eoc_ctl_index);
  	  fprintf(outf,"head_ctl_addr: 0x%08x\t\tcb_ctl_ptr: 0x%08x\n",
	  	  lan1ift->head_ctl_addr, lan1ift->cb_ctl_ptr);
  	  fprintf(outf,"quads_ctl_ptr: 0x%08x\n",lan1ift->quads_ctl_ptr);
  	  fprintf(outf,"\n***CONTROL QUADS***\n");
  	  net_print_quads(lan1ift->quads_ctl_ptr,pq_buf,pq_size,LAN1_MAX_CTL_QUADS);
	  break;

	default:
	  printf("\nBAD option.... try again\n");
	  break;
       }
       a_char = getc(stdin);
       if (option != 0)
	 {
	  printf("\nHit carriage return to continue...");
          while (getc(stdin) != '\012');
	 }
     }
  free(cb_buf);
  free(pq_buf);
  return;
}

void net_print_quads(pq_ptr,pq_buf,pq_size,max_quads)
lan1_pquad_t *pq_ptr, *pq_buf;
int max_quads, pq_size;

{
int i;
lan1_pquad_t *scratch_ptr;
  scratch_ptr = pq_ptr;
  pq_buf->link_ptr = 0;
  fprintf(outf,"quad #\tlink_ptr\tcommand\t\tdata_ptr\tcount\n");
  for (i=0; (i < max_quads) &&
	   ((pq_buf->link_ptr & LAN1_DMA_EOC) ==0); i++)
    {netvread(scratch_ptr,pq_buf,pq_size);
     fprintf(outf,"%d\t0x%08x\t0x%08x\t0x%08x\t0x%08x\n",i,pq_buf->link_ptr,pq_buf->command,pq_buf->data_ptr,pq_buf->count);
     scratch_ptr = pq_buf->link_ptr;
    }
}

void netdump_lan0ift(lan0ift)
lan0_ift  *lan0ift;

{
int i;

    fprintf(outf,
	"\n  dev_addr   0x%02x        cam_subq      0x%02x        cam_port    0x%08x\n",
	lan0ift->dev_addr, lan0ift->cam_subq, lan0ift->cam_port);
    fprintf(outf,
        "  omsg_ioctl 0x%08x  omsg_init     0x%08x  omsg_normal 0x%08x\n",
	lan0ift->omsg_ioctl, lan0ift->omsg_init, lan0ift->omsg_normal);
    fprintf(outf,
        "  req_init   0x%08x  rep_init      0x%08x  idy_block   0x%08x\n\n",
	lan0ift->req_init, lan0ift->rep_init, lan0ift->idy_block);
    fprintf(outf,
        "  vq_rs      0x%08x  vq_wc         0x%08x\n\n", 
	lan0ift->vq_rs, lan0ift->vq_wc);

    fprintf(outf, "  vq_rd:  ");
    for (i = 0; i < 4; i++)
	fprintf(outf, "0x%08x    ", lan0ift->vq_rd[i]);
    fprintf(outf, "\n");
    fprintf(outf, "  vq_wd:  ");
    for (i = 0; i < 4; i++)
	fprintf(outf, "0x%08x    ", lan0ift->vq_wd[i]);
    fprintf(outf, "\n\n");
}
