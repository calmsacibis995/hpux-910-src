/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/netdump2.c,v $
 * $Revision: 1.14.83.3 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 16:28:10 $
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
static char rcsid[]="@(#) $Header: netdump2.c,v 1.14.83.3 93/09/17 16:28:10 root Exp $";
#endif


#include "../standard/inc.h"
#include "net.h"
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"



void n_dump_nop(addr)
register char *addr;
{
    hexdump(addr, MLEN, 0, 0);
}



netdump_address(i)
register int i;
{
    register struct nlist *np;
    
    for (np = net_nl; np->n_name && *np->n_name; np++, i++) {
	fprintf(outf," %8.8s: 0x%08x  ", np->n_name, np->n_value);
	if (i == 3) {
	    i = 0;
	    fprintf(outf,"\n");
	}
    }
    return (i);
}



/* netdump_flags -- dump flags contained in "flags" 
 */

netdump_flags(ft, flags)
struct netflags *ft;
int flags;
{
    fprintf(outf, "\n  ");
    while (ft->name) {
        if (flags & ft->val) {
            fprintf(outf, "%s ", ft->name);
	    flags &= ~(ft->val);
	}	  
        ft++;
    }
    if (flags)
	fprintf(outf, "??? "); /* there is something we don't understand */
    fprintf(outf, "\n");
}



/* netdump_value -- dump value contained in "value" 
 */

netdump_value(vt, value)
struct netflags *vt;
int value;
{
    fprintf(outf, "\n  ");
    while (vt->name) {
        if (value == vt->val) {
            fprintf(outf, "%s\n", vt->name);
	    return;
	}	  
        vt++;
    }
    fprintf(outf, "???\n"); /* there is something we don't understand */
}



static char *icmp_names[] = {
	"echo reply",
	"type 1",
	"type 2",
	"dst unreach",
	"src quench",
	"redirect",
	"type 6",
	"type 7",
	"echo",
	"type 9",
	"type 10",
	"time xceed",
	"param error",
	"tmstmp",
	"tmstmp rply",
	"info req",
	"info reply",
	"submsk req",
	"submsk rply",
	"type 19",
	"type 20",
	"type 21"
};

dumpnet()
{
    int msize = MSIZE;
    int mlen = MLEN;
    register int i;

    fprintf(outf,"\n");
    
    fprintf(outf," Networking stats:\n");
    fprintf(outf,"  MSIZE        0x%08x  MLEN              0x%08x\n", 
        msize, mlen);
    fprintf(outf,"  NETCLBYTES   0x%08x  NMBPCL            0x%08x\n", 
        NETCLBYTES, NMBPCL);
    fprintf(outf,"  mfree        0x%08x  mclfree           0x%08x\n", 
        mfree, mclfree);
    fprintf(outf,"  mbuf_memory  0x%08x\n\n", mbuf_memory);

    fprintf(outf," Mbstat:\n");
#ifdef NEWBM
    fprintf(outf,
	"  m_mbufs      0x%08x  m_umbfree   0x%08x  m_rmbfree   0x%08x\n",
        mbstat.m_mbufs, mbstat.m_umbfree, mbstat.m_rmbfree);
    fprintf(outf,
	"  m_clusters   0x%08x  m_uclfree   0x%08x  m_rclfree   0x%08x\n",
        mbstat.m_clusters, mbstat.m_uclfree, mbstat.m_rclfree);
    fprintf(outf,
	"  malloc failures  0x%08x   no credits       0x%08x\n",
        mbstat.m_adrops, mbstat.m_cdrops);
    fprintf(outf,
	"  reserve failures 0x%08x   overdrawn        0x%08x\n",
        mbstat.m_rdrops, mbstat.m_overdrawn);
    fprintf(outf,
	"  m_rmbs_in_rcl    0x%08x   m_num_cls        0x%08x\n",
        mbstat.m_rmbs_in_rcl, mbstat.m_num_cls);
    fprintf(outf,
	"  m_copy_by_move   0x%08x   m_copy_by_refcnt 0x%08x\n",
        mbstat.m_copy_by_move, mbstat.m_copy_by_refcnt);
    fprintf(outf,
	"  bytes compressed 0x%08x   bytes fragmented 0x%08x\n",
	mbstat.m_bytes_compressed, mbstat.m_bytes_fragmented);
    fprintf(outf,
	"  mg_mfree         0x%08x   mg_clfree        0x%08x\n",
	mbstat.mg_mfree, mbstat.mg_mclfree);
#else  NEWBM
    fprintf(outf,"  m_mbufs          0x%04hx  m_mbfree          0x%04hx\n",
        mbstat.m_mbufs, mbstat.m_mbfree);
    fprintf(outf,"  m_clusters       0x%04hx  m_clfree          0x%04hx\n",
        mbstat.m_clusters, mbstat.m_clfree);
    fprintf(outf,"  m_drops          0x%04hx  m_copy_by_refcnt  0x%04hx\n",
        mbstat.m_drops, mbstat.m_copy_by_refcnt);
    fprintf(outf,"  m_copy_by_move   0x%04hx\n",
        mbstat.m_copy_by_move);
#endif NEWBM

    fprintf(outf, "  m_mtypes:\n");
    for (i = 0; i <= MT_LASTONE; i++) {
        fprintf (outf, "  %-15.15s  0x%04hx", 
            netdata_ntypname(i), mbstat.m_mtypes[i]);
        if (((i + 1) % 3) == 0)
            fprintf (outf, "\n");
    }
    fprintf (outf, "\n\n");

    fprintf(outf," Mastat:\n");
    fprintf(outf,"  maccts_reserved 0x%08x  maccts_allocd    0x%08x\n",
        mastat.ma_maccts_reserved, mastat.ma_maccts_allocd);
    fprintf(outf,"  maccts_freed    0x%08x  macct_free_list  0x%08x\n",
        mastat.ma_maccts_freed, mastat.ma_macct_free_list);

    fprintf(outf, "\n lan0:\n");
    fprintf(outf, "  num_lan0        0x%08x  xsap_list_head   0x%08x\n",
	num_lan0, xsap_list_head);

    fprintf(outf, "\n ipstat:\n");
    fprintf(outf, "  ips_badsum     0x%08x   ips_tooshort  0x%08x\n",
	ipstat.ips_badsum, ipstat.ips_tooshort);
    fprintf(outf, "  ips_toosmall   0x%08x   ips_badhlen   0x%08x\n",
	ipstat.ips_toosmall, ipstat.ips_badhlen);
    fprintf(outf, "  ips_badlen     0x%08x\n",
	ipstat.ips_badlen);

    fprintf(outf, "\n icmpstat:\n");
    fprintf(outf, "  icps_error     0x%08x   icps_oldshort 0x%08x\n",
	icmpstat.icps_error, icmpstat.icps_oldshort);
    fprintf(outf, "  icps_oldicmp   0x%08x   icps_badcode  0x%08x\n",
	icmpstat.icps_oldicmp, icmpstat.icps_badcode);
    fprintf(outf, "  icps_tooshort  0x%08x   icps_checksum 0x%08x\n",
	icmpstat.icps_tooshort, icmpstat.icps_checksum);
    fprintf(outf, "  icps_badlen    0x%08x   icps_reflect  0x%08x\n",
	icmpstat.icps_badlen, icmpstat.icps_reflect);

    fprintf(outf, "  icps_outhist:\n     ");
    for (i = 0; i < ICMP_MAXTYPE + 1; i++) {
        fprintf (outf, "%-11.11s  0x%06x   ", 
		icmp_names[i], icmpstat.icps_outhist[i]);
	if ((i + 1) % 3 == 0)
	    fprintf(outf, "\n     ");
    }
    
    fprintf(outf, "\n  icps_inhist:\n     ");
    for (i = 0; i < ICMP_MAXTYPE + 1; i++) {
        fprintf (outf, "%-11.11s  0x%06x   ", 
		icmp_names[i], icmpstat.icps_inhist[i]);
	if ((i + 1) % 3 == 0)
	    fprintf(outf, "\n     ");
    }

    fprintf(outf, "\n\n tcpstat:\n");
    fprintf(outf, "  tcps_badsum     0x%08x   tcps_badoff  0x%08x\n",
	tcpstat.tcps_badsum, tcpstat.tcps_badoff);
    fprintf(outf, "  tcps_hdrops     0x%08x   tcps_badsegs 0x%08x\n",
	tcpstat.tcps_hdrops, tcpstat.tcps_badsegs);
    fprintf(outf, "  tcps_unack      0x%08x\n  tcps_timer:\n",
	tcpstat.tcps_unack);
    fprintf(outf, "    delayed_acks  0x%08x   retrans      0x%08x\n",
	tcpstat.tcps_timer_delayed_acks, tcpstat.tcps_timer_retransmissions);
    fprintf(outf, "    keep_alive    0x%08x   persist      0x%08x\n",
	tcpstat.tcps_timer_keep_alive, tcpstat.tcps_timer_persist);

    fprintf(outf, "\n\n probestat:\n");
    fprintf(outf, "  ps_seqnomissing 0x%08x   ps_nombufs   0x%08x\n",
	probestat.ps_seqnomissing, probestat.ps_nombufs);
    fprintf(outf, "  ps_tooshort     0x%08x   ps_toosmall  0x%08x\n",
	probestat.ps_tooshort, probestat.ps_toosmall);
    fprintf(outf, "  ps_badhlen      0x%08x   ps_badlen    0x%08x\n",
	probestat.ps_badhlen, probestat.ps_badlen);
    fprintf(outf, "  ps_no8023       0x%08x   ps_noif      0x%08x\n",
	probestat.ps_no8023, probestat.ps_noif);
    fprintf(outf, "  ps_badversion   0x%08x   ps_badtype   0x%08x\n",
	probestat.ps_badversion, probestat.ps_badtype);
    fprintf(outf, "  ps_senderr      0x%08x   ps_badseq    0x%08x\n",
	probestat.ps_senderr, probestat.ps_badseq);
    fprintf(outf, "  ps_badreplen    0x%08x   ps_baddrepln 0x%08x\n",
	probestat.ps_badreplen, probestat.ps_baddreplen);
    fprintf(outf, "  ps_badvnavers   0x%08x   ps_baddomain 0x%08x\n",
	probestat.ps_badvnaversion, probestat.ps_baddomain);
    fprintf(outf, "  ps_badalignment 0x%08x\n",
	probestat.ps_badalignment);

    fprintf(outf, "\n");
}



void netdump(ntyp, vaddr)
int ntyp;
register uint vaddr;
{
    net_apply(ntyp, vaddr, 1);
}

