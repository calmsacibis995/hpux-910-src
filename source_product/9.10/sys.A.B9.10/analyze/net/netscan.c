/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/net/RCS/netscan.c,v $
 * $Revision: 1.17.83.3 $		$Author: root $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 16:28:33 $
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
static char rcsid[]="@(#) $Header: netscan.c,v 1.17.83.3 93/09/17 16:28:33 root Exp $";
#endif


#include "../standard/inc.h"
#include "net.h"
#include "../standard/defs.h"
#include "../standard/types.h"
#include "../standard/externs.h"



netscan_stats()
{
    /* mbstat */
    if (mbstat.m_mtypes[MT_OPT] != 0)
	NERROR("number of MT_OPT mbufs is non-zero (%0#x)", 
	    mbstat.m_mtypes[MT_OPT]);
    if (mbstat.m_mtypes[MT_MSG_MODE_TBL] != nmmtcp)
	NERROR(
	    "# of MT_MSG_MODE_TBL mbufs != # of tcp mm control blocks (%0#x)",
	    nmmtcp);
    if ((mbstat.m_uclfree == 0) && (mbstat.m_rclfree == 0))
	NERROR("mbstat.m_uclfree and mbstat.m_rclfree are both zero", 0);

    /* mastat */
    if (mastat.ma_maccts_reserved < mastat.ma_maccts_freed) 
        NERROR("ma_maccts_reserved < ma_maccts_freed", 0);
    if (mastat.ma_maccts_freed == 0)
        NERROR("ma_maccts_freed == 0", 0);

    /* tcpstat */

    /* ipstat */

    /* icmpstat */

    /* pxpstat */

    /* udpstat */

    /* probestat */

    /* rtstat */
}



/* netscan_mbuf -- scan mbuf memory for inconsistencies.  Scan the mbuf
 * and cluster free lists as well.  This procedure is an adaptation of
 * the routine m_check() is sys/so_mbuf.c.  The semantics of the checks
 * are the same.
 */

void netscan_mbuf()
{
    register int i, j;
    register struct mbuf *m, *n;
    register uint vaddr;
    int free_mbufs, random_mbufs, mbufs_inuse;
    int clusters_on_free_list, referenced_clusters;
    int l_mtypes[MT_LASTONE+1];
    char l_mclrefcnt[NMBCLUSTERS];

    NESAV;
    NECLR;

    memset(l_mtypes, 0, sizeof(l_mtypes));
    memset(l_mclrefcnt, 0, sizeof(mclrefcnt));
    
    i = 0;
    m = mfree;
    fprintf(outf,"SCANNING MFREE LIST\n");
    while (m) {
        if (got_sigint) goto skip1;    /* break out of loop on sigint */
	NESETVA(m);
        if (!NTVALIDV(NT_MBUF, m)) {
            NERROR("invalid address tracing mfree list", 0);
            goto skip1;
        }
        NDBG(("tracing mfree %#x", m));
        m = (struct mbuf *)vtol(X_MBUTL, m);
	if (m == -1) {
            NERROR("invalid address tracing mfree list", 0);
            goto skip1;
	}
        i++;
        if (i > (NMBCLUSTERS * NMBPCL)) {
            NERROR("loop encountered tracing mfree list", 0);
            goto skip1;
        }
        m = m->m_next;
    }

    NECLRVA;
#ifdef NEWBM
    if (i != (mbstat.m_umbfree + mbstat.m_rmbfree))
        NERROR(
            "mbstat.m_[u+r]mbfree does not match number of free mbufs (%#x)",i);
#else  NEWBM
    if (i != mbstat.m_mbfree)
        NERROR("mbstat.m_mbfree does not match number of free mbufs (%#x)",i);
#endif NEWBM

skip1:
    NECLRVA;
    i = 0;
    m = mclfree;
    fprintf(outf,"SCANNING MCLFREE LIST\n");
    while (m) {
        if (got_sigint) goto skip2;    /* break out of loop on sigint */
	NESETVA(m);
        if (!NTVALIDV(NT_MBUF, m)) {
            NERROR("invalid address tracing mclfree list", 0);
            goto skip2;
        }
        if (i > NMBCLUSTERS) {
            NERROR("loop encountered tracing mclfree list", 0);
            goto skip2;
        }
        NDBG(("tracing mclfree %#x", m));
        m = (struct mbuf *)vtol(X_MBUTL, m);
        if (m == -1) {
            NERROR("invalid address tracing mclfree list", 0);
            goto skip2;
        }
        i++;
        m = m->m_next;
    }

    NECLRVA;
#ifdef NEWBM
    if (i != (mbstat.m_uclfree + mbstat.m_rclfree))
      NERROR(
        "mbstat.m_[u+r]clfree does not match number of free clusters (%#x)",i);
#else  NEWBM
    if (i != mbstat.m_clfree)
        NERROR(
            "mbstat.m_clfree does not match number of free clusters (%#x)",i);
#endif NEWBM

skip2:

/*
 *    step through mbuf memory, counting number of mbufs which are not free
 *    must skip of clusters.
 */
    NECLRVA;
    m = mbuf_memory;
#ifdef	NS_MBUF_QA
if (ns_mbuf_qa_on)
    i = mbstat.m_mbufs + mbstat.m_clusters;
else {
    i = mbstat.m_mbufs / NMBPCL + mbstat.m_clusters;
    if (i >= 3*NMBCLUSTERS/19)
	i = 3*NMBCLUSTERS/19 - 1;
    }
#else	! NS_MBUF_QA
    i = mbstat.m_mbufs / NMBPCL + mbstat.m_clusters;
#endif	! NS_MBUF_QA
    if (i == 0) 
        return;
    if (i >= NMBCLUSTERS)
	i = NMBCLUSTERS - 1;

    random_mbufs = free_mbufs = mbufs_inuse = 0;
    clusters_on_free_list = referenced_clusters = 0;

    fprintf(outf,"SCANNING MCLREFCNT\n");
    while (i--) {
        if (got_sigint) goto skip3;    /* break out of loop on sigint */
        NDBG(("mclrefcnt[%#x]=%#x", i, mclrefcnt[i]));
        if (mclrefcnt[i]) {
            referenced_clusters++;
	    goto next;
        }
	n = mclfree;
        m = cltom(i);

	while (n) {
	    if (got_sigint) break;
	    if (n == m) {
	        clusters_on_free_list++;
		goto next;
	    }
	    n = (struct mbuf *)vtol(X_MBUTL, n);
            if (n == -1) {
                NERROR("invalid address tracing mclfree list", 0);
                goto skip3;
            }
	    n = n->m_next;
	}

	/*
	 * cluster has no references and is not on the free list
	 */
	vaddr = (uint)cltom(i);
        m = (struct mbuf *)vtol(X_MBUTL, cltom(i));
        if (m == -1) {
            NERROR("invalid address tracing mclfree list", 0);
            goto skip3;
        }
#ifdef	NS_MBUF_QA
        for (j = 0 ; j < (ns_mbuf_qa_on?1:NMBPCL);
	  j++, m++, vaddr += sizeof(struct mbuf)) {
#else	! NS_MBUF_QA
        for (j = 0 ; j < NMBPCL; j++, m++, vaddr += sizeof(struct mbuf)) {
#endif	NS_MBUF_QA
            if (got_sigint) goto skip3;    /* break out of loop on sigint */
            if(m->m_type != MT_FREE) 
	            netverify(NT_MBUF, vaddr);
            if(m->m_type == MT_FREE)
                free_mbufs++;
            else if (m->m_type <= MT_LASTONE) {
                mbufs_inuse++;
                if (m_isa_cluster(m)) 
                    l_mclrefcnt[vmbtocl(vaddr)]++;
		l_mtypes[m->m_type]++;
            } else {
                random_mbufs++;
		NERROR("mbuf has unknown m_type: %#x", m->m_type);
	    }
        }
next:
	continue;
    }

skip3:
    NECLR;
    
    if (random_mbufs)
        NERROR("%#x unclaimed mbufs found", random_mbufs);
    if (mbufs_inuse + free_mbufs != mbstat.m_mbufs)
        NERROR2("mbufs inuse(%#x) + mbufs free(%#x) != mbstat.m_mbufs", 
            mbufs_inuse, free_mbufs);
    if (clusters_on_free_list + referenced_clusters != mbstat.m_clusters)
        NERROR2(
         "clusters referenced(%#x) + clusters free(%#x) != mbstat.m_clusters",
            referenced_clusters, clusters_on_free_list);
    for (i = 0; i < NMBCLUSTERS; i++) {
        if (mclrefcnt[i] != l_mclrefcnt[i]) {
	    char buf[128];
	    sprintf(buf, 
	       "%#x references to cluster %#x does not match mclrefcnt, %#x",
                    l_mclrefcnt[i], cltom(i), mclrefcnt[i]);
#ifndef TRASH_NET
	    NERROR(buf, 0);
#endif  TRASH_NET
	}
    }
    
    for (i = 0; i <= MT_LASTONE; i++) {
        if ((i != MT_FREE) && (mbstat.m_mtypes[i] != l_mtypes[i])) {
	    NERROR2("mbstat count for %s doesn't match actual count %08x",
		netdata_ntypname(i), l_mtypes[i]);
	}
    }

    NERES;
}



/* netscan_proc -- scan proc table for interesting things.  This includes
 * procs waiting on networking, procs which have open sockets, and the
 * current process.
 */

netscan_proc()
{
    register struct proc *p;
    register int i;
    register uint vaddr, ovaddr;
    
    fprintf(outf,"SCANNING PROC TABLE\n");
    NMSG("PID   COMMAND    WCHAN                       SOCKETS", 0);
    for (p = &proc[0]; p < proc+nproc; p++) {
        if (got_sigint) break;    /* break out of loop on sigint */
        if (p->p_stat == 0) continue;
        vaddr = (uint)p->p_wchan;
	i = NVALIDV(vaddr);
	/* NMSG2("vaddr %#x == ntyp %#x", vaddr, i); */
	netget_u(p, (i ? vaddr : (uint) -1));
    }
    fprintf(outf,"\n");
}



/* netscan_lan -- scan lan driver data structures */
netscan_lan()
{
    register int i;
    
    for (i = 0; i < NDNUM(X_LANIFT); i++) {
	netverify(NT_LANIFT, ntov(X_LANIFT, i));
    }
    for (i = 0; i < NDNUM(X_XSAP_LIST); i++) {
	netverify(NT_XSAPLINK, ntov(X_XSAP_LIST, i));
    }
    for (i = 0; i < NDNUM(X_SAP_ACTIVE); i++) {
	netverify(NT_SAPINPUT, ntov(X_SAP_ACTIVE, i));
    }
}



/* netscan_pcbs -- scan protocol control blocks */
netscan_pcbs()
{
    register int i;
    
    for (i = 0; i < NDNUM(X_INPCB); i++) {
	netverify(NT_INPCB, ntov(X_INPCB, i));
    }
    for (i = 0; i < NDNUM(X_TCP_CB); i++) {
	netverify(NT_TCPCB, ntov(X_TCP_CB, i));
    }
}



/* netscan_ipq -- scan ip input queue */
netscan_ipq()
{
    register int i;
    
    for (i = 0; i < NDNUM(X_IPQ); i++) {
	netverify(NT_IPQ, ntov(X_IPQ, i));
    }
    for (i = 0; i < NDNUM(X_IPASFRAG); i++) {
	netverify(NT_IPASFRAG, ntov(X_IPASFRAG, i));
    }
}



/* netscan_probe -- scan probe tables */
netscan_probe()
{
    register int i;
    
    for (i = 0; i < NDNUM(X_PR_NTAB); i++) {
	netverify(NT_PRNTAB, ntov(X_PR_NTAB, i));
    }
    for (i = 0; i < NDNUM(X_PR_VNATAB); i++) {
	netverify(NT_PRVNATAB, ntov(X_PR_VNATAB, i));
    }
}



/* netscan_sockets -- scan socket data structures */
netscan_sockets()
{
    register int i;
    
    for (i = 0; i < NDNUM(X_SOCKET); i++) {
	netverify(NT_SOCKET, ntov(X_SOCKET, i));
    }
    for (i = 0; i < NDNUM(X_SOCKBUF); i++) {
	netverify(NT_SOCKBUF, ntov(X_SOCKBUF, i));
    }
}

 

/* netscan -- scan the entire networking memory for inconsistent
 * data structures, printing out the problems we find.
 */

void netscan()
{
    NESAV;
    NECLR;

    NDBG(("netscan()"));

    /* clear log tables */
    netverify_clearlog();

    netscan_proc();
    netscan_stats();
    netscan_lan();
    fprintf(outf,"SCANNING SOCKETS\n");
    netscan_sockets();
    fprintf(outf,"SCANNING PROTOCOL CONTROL BLOCKS\n");
    netscan_pcbs();
    fprintf(outf,"SCANNING IP QUEUE\n");
    netscan_ipq();
    fprintf(outf,"SCANNING PROBE TABLES\n");
    netscan_probe();
    netscan_mbuf(); /* do last because it's big */

    ptyscan();

    fprintf(outf,"\n");

    NERES;
}



static struct sockbuf off_sockbuf;
static struct socket  off_socket;
#define OFFSETOF(s,f)	(uint)&s.f - (uint)&s
#define SOCKETOFF(f)	OFFSETOF(off_socket, f)
#ifdef NEWBM
#define SOCKBUFOFF(f, sb)	SOCKETOFF(sb) + OFFSETOF(off_sockbuf, f)
#else  NEWBM
#define SOCKBUFOFF(f, sb)	OFFSETOF(off_sockbuf, f)
#endif NEWBM

/* Get uarea for process */
netget_u(p, netwaitaddr)
    register struct proc *p;
    uint netwaitaddr;
{
    register int i;
    register uint uvirt, vaddr;
    register struct file *ft;
    int oldvflg;
    int said_name = 0;
    int said_sockets = 0;
    uint ovaddr;
    char buf[128], *wtype;
    int ntyp;

    if (netwaitaddr == (uint) -1) {
	if (p->p_wchan) { 
	    uint diff = findsym(p->p_wchan);
	    char symstr[128], *sp;
	    strcpy(symstr, "=");
	    if (diff > 0x0fffffff)
		strcat(symstr, "?");
	    else {
		strcat(symstr, cursym);
		if (diff) {
		    symstr[13] = '\0';
		    sprintf(buf, "+0x%04.4x", diff);
		    strcat(symstr, buf);
		}
	    }
            sprintf(buf, "0x%08x%-18.18s", p->p_wchan, symstr);
	} 
	else
	    sprintf(buf, "%-28.28s", "");
    }
    else {
    	ovaddr = netwaitaddr;
        ntyp = vton(netwaitaddr);
        if (ntyp == NT_MBUF) {
            netwaitaddr = vmtod((uint)dtom(netwaitaddr)); /* get base */
            ntyp = vton(netwaitaddr);
        }
	wtype = "";
	if (ntyp == NT_SOCKET) {
	    if ((ovaddr - netwaitaddr) == SOCKETOFF(so_timeo))
	        wtype = "conn";
#ifdef NEWBM
	    if ((ovaddr - netwaitaddr) == SOCKBUFOFF(sb_cc, so_rcv))
	        wtype = "data";
	    if ((ovaddr - netwaitaddr) == SOCKBUFOFF(sb_flags, so_rcv))
	        wtype = "lock";
	    if ((ovaddr - netwaitaddr) == SOCKBUFOFF(sb_cc, so_snd))
	        wtype = "data";
	    if ((ovaddr - netwaitaddr) == SOCKBUFOFF(sb_flags, so_snd))
	        wtype = "lock";
#else  NEWBM
	} else if (ntyp == NT_SOCKBUF) {
	    if ((ovaddr - netwaitaddr) == SOCKBUFOFF(sb_cc, 0))
	        wtype = "data";
	    if ((ovaddr - netwaitaddr) == SOCKBUFOFF(sb_flags, 0))
	        wtype = "lock";
#endif NEWBM
	}
        sprintf(buf, "0x%08x(%-4.4s) %-10.10s ", 
	    p->p_wchan, wtype, netdata_ntypname(ntyp));
    }

    oldvflg = vflg;
    vflg = 0;
    
    /* if u pages are swaped out, just return badly because we don't
     * support this.
     */
#ifndef REGION
    if (!(p->p_flag & SUPAGEH))
	goto out;

    /* u pages are in memory, read them */
#endif
#ifdef REGION

	goto out;
#else

    /* get starting virtual address */
    uvirt = (uint)uvadd(p);
    if (uptr == uvirt)
        NMSG("%05u [current process]", p->p_pid);
#endif

    if (netvread(uvirt, (char *)u_area.buf, sizeof(struct user)) !=
	sizeof(struct user))
	    goto out;

    if (netwaitaddr != (uint) -1) {
        fprintf(outf, "%05u %-10.10s %s", p->p_pid, u.u_comm, buf);
	said_name++;
    }

    /* print open files */
    for (i = 0; i < NOFILE; i++) {
        if ((vaddr = u.u_ofile[i]) != 0) {
	    if ((vaddr < vfile) || (vaddr >= &vfile[nfile])) {
		if (!activeflg)
	            NERROR("file struct address 0x%08x out of range", vaddr);
	    } else {
                ft = &file[(struct file *)vaddr - vfile];
	        if (ft->f_type == DTYPE_SOCKET) {
		    if (!said_name) {
	                fprintf(outf,"%05u %-10.10s %s", 
			    p->p_pid, u.u_comm, buf);
			said_name++;
		    }
		    if (said_sockets && (said_sockets % 3 == 0))
			fprintf(outf,"\n%-45.45s", " ");
		    said_sockets++;
		    fprintf(outf,"0x%08x ", ft->f_data);
		}
	    }
        }
    }
    if (said_sockets || said_name)
        fprintf(outf,"\n");

    if (u.u_procp != (vproc + (p - proc))) {
	if (!activeflg)
            NERROR("u_procp 0x%08x != proc struct address", u.u_procp);
        /* if outside of proc table range then this is
         * really bad and we will filter that up  
         */
        if ((u.u_procp < vproc) || (u.u_procp > vproc + nproc))
            goto out;
    }
    vflg = oldvflg;
    return (1);

out:
    vflg = oldvflg;
    return(0);
}
