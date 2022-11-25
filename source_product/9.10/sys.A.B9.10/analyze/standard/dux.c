/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/dux.c,v $
 * $Revision: 1.14.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:30:55 $
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

/*
 * dux.c: diskless-specific modules for analyze
 */

#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"
#include "dux/dmmsgtype.h"

char *dm_opcode_descrp[] = {
"INVALID",
"DMSIGNAL",
"DM_CLUSTER",
"DM_ADD_MEMBER",
"DM_READCONF",
"DM_CLEANUP",
"DMNDR_READ",
"DMNDR_WRITE",
"DMNDR_OPEND",
"DMNDR_CLOSE",
"DMNDR_IOCTL",
"DMNDR_SELECT",
"DMNDR_STRAT",
"DMNDR_BIGREAD",
"DMNDR_BIGWRITE",
"DMNDR_BIGIOFAIL",
"DM_LOOKUP",
"DMNETSTRAT_READ",
"DMNETSTRAT_WRITE",
"DMSYNCSTRAT_READ",
"DMSYNCSTRAT_WRITE",
"DM_CLOSE",
"DM_GETATTR",
"DM_SETATTR",
"DM_IUPDAT",
"DM_SYNC",
"DM_REF_UPDATE",
"DM_OPENPW",
"DM_FIFO_FLUSH",
"DM_PIPE",
"DM_GETMOUNT",
"DM_INITIAL_MOUNT_ENTRY",
"DM_MOUNT_ENTRY",
"DM_UFS_MOUNT",
"DM_COMMIT_MOUNT",
"DM_ABORT_MOUNT",
"DM_UMOUNT_DEV",
"DM_UMOUNT",
"DM_SYNCDISC",
"DM_FAILURE",
"DM_SERSETTIME",
"DM_SERSYNCREQ",
"DM_RECSYNCREP",
"DM_GETPIDS",
"DM_RELEASEPIDS",
"DM_LSYNC",
"DM_FSYNC",
"DM_CHUNKALLOC",
"DM_CHUNKFREE",
"DM_TEXT_CHANGE",
"DM_XUMOUNT",
"DM_XRELE",
"DM_USTAT",
"DM_RMTCMD",
"DM_ALIVE",
"DM_DMMAX",
"DM_SYMLINK",
"DM_RENAME",
"DM_FSTATFS",
"DM_LOCKF",
"DM_PROCLOCKF",
"DM_LOCKWAIT",
"DM_INOUPDATE",
"DM_UNLOCKF",
"DM_NFS_UMOUNT",
"DM_COMMIT_NFS_UMOUNT",
"DM_ABORT_NFS_UMOUNT",
"DM_MARK_FAILED",
"DM_LOCK_MOUNT",
"DM_UNLOCK_MOUNT",
"DM_SETACL",
"DM_GETACL",
"DM_FPATHCONF",
"DM_SETEVENT",
"DM_AUDCTL",
"DM_AUDOFF",
"DM_GETAUDSTUFF",
"DM_SWAUDFILE",
"DM_CL_SWAUDFILE",
"DM_FSCTL",
"DM_QUOTACTL",
"DM_QUOTAONOFF",
"DM_LOCKED",
"DM_INVALID_83",
"DM_INVALID_84",
"DM_INVALID_85",
"DM_INVALID_86",
"DM_INVALID_87",
"DM_INVALID_88",
"DM_INVALID_89",
"DM_INVALID_90",
"DM_INVALID_91",
"DM_INVALID_92",
"DM_INVALID_93",
"DM_INVALID_94",
"DM_INVALID_95",
"DM_INVALID_96",
"DM_INVALID_97",
"DM_INVALID_98",
"DM_INVALID_99",
""	/* terminator! */
};

dumpsitemap(cp,sm)
char *cp;
struct sitemap sm;
{
    struct sitearray array, *ap;

    fprintf(outf, "%s s_maptype ", cp);
    switch (sm.s_maptype)
    {
    case S_MAPEMPTY:
	fprintf(outf, "EMPTY\n");
	return;
    case S_ONESITE:
	fprintf(outf, "ONESITE s_count 0x%04x  s_site 0x%02x  s_count 0x%04x\n", sm.s_count, sm.s_onesite.s_site, sm.s_onesite.s_count);
	return;
    case S_ARRAY:
	fprintf(outf, "ARRAY   s_count 0x%04x  su_array 0x%08x\n",
		sm.s_count, sm.s_array);
	break;
    case S_LOCKED:
	fprintf(outf, "LOCKED  s_count 0x%04x  su_array 0x%08x\n",
		sm.s_count, sm.s_array);
	break;
    case S_WAITING:
	fprintf(outf, "WAITING s_count 0x%04x  su_array 0x%08x\n",
		sm.s_count, sm.s_array);
	break;
    default:
	fprintf(outf, "bad s_maptype %d\n", sm.s_maptype);
	return;
    }

#ifdef notdef
    if (sm.s_count <= 2)
	fprintf(outf, "  site 0x%02x  count 0x%04x\n",
		sm.s_onesite.s_site, sm.s_onesite.s_count);
#endif

    ap = sm.s_array;
    while (ap != NULL)
    {
	int i, count = 0;
	if (getchunk(KERNELSPACE, ap, &array, sizeof (struct sitearray), "dumpsistemap"))
	    return (-1);
	ap = &array;
	for (i = 0; i < SITEARRAYSIZE && count < sm.s_count; i++)
	{
	    if (ap->s_entries[i].s_count && ap->s_entries[i].s_site)
	    {
		count++;
		fprintf(outf, "   site 0x%02x count 0x%04x",
		    ap->s_entries[i].s_site,
		    ap->s_entries[i].s_count);
	    }
	    if ((count % 3) == 0)
		fprintf(outf, "\n");
	}
	ap = ap->s_next;
	fprintf(outf, "   s_next 0x%08x\n", ap);
    }
}

#ifdef notdef
char *dux_mbuf_types[] = {
    "DUX_FREE",
    "DUX_DATA",
    "MT_DUX",
    "MT_DUX_PAGEPOOL"
};
#endif


struct dux_mbuf *
valid_mbufp(vaddr)
struct dux_mbuf *vaddr;
{
}


struct dux_mbuf *
valid_mclbufp(vaddr)
struct dux_mbuf *vaddr;
{
}

#define P_ALLFLAGS (P_REQUEST		| \
		    P_REPLY		| \
		    P_ACK		| \
		    P_DATAGRAM		| \
		    P_END_OF_MSG	| \
		    P_IDEMPOTENT	| \
		    P_SLOW_REQUEST	| \
		    P_MULTICAST		| \
		    P_ALIVE_MSG		| \
		    P_NAK		| \
		    P_SUICIDE)

#define DM_ALLFLAGS (DM_SLEEP		| \
		     DM_DONE		| \
		     DM_FUNC		| \
		     DM_REPEATABLE	| \
		     DM_RELEASE_REQUEST	| \
		     DM_RELEASE_REPLY	| \
		     DM_REQUEST_BUF	| \
		     DM_REPLY_BUF	| \
		     DM_SOUSIG		| \
		     DM_LONGJMP		| \
		     DM_SIGPENDING	| \
		     DM_LIMITED_OK	| \
		     DM_KEEP_REQUEST_BUF| \
		     DM_KEEP_REPLY_BUF	| \
		     DM_DATAGRAM	| \
		     DM_INTERRUPTABLE)

dumpdmheader(cp,dm,vdm)
char *cp;
struct dm_header *dm, *vdm;
{
    fprintf(outf, "%s address 0x%08x  data addr 0x%08x\n",
	    cp, vdm, (int)vdm - (int)dm + (int)dm->dm_params);
    fprintf(outf, "  hpxsap: dstaddr 0x%.2x%.2x%.2x%.2x%.2x%.2x  srcaddr 0x%.2x%.2x%.2x%.2x%.2x%.2x  len 0x%04x\n",
	dm->dm_ph.iheader.destaddr[0],
	dm->dm_ph.iheader.destaddr[1],
	dm->dm_ph.iheader.destaddr[2],
	dm->dm_ph.iheader.destaddr[3],
	dm->dm_ph.iheader.destaddr[4],
	dm->dm_ph.iheader.destaddr[5],
	dm->dm_ph.iheader.sourceaddr[0],
	dm->dm_ph.iheader.sourceaddr[1],
	dm->dm_ph.iheader.sourceaddr[2],
	dm->dm_ph.iheader.sourceaddr[3],
	dm->dm_ph.iheader.sourceaddr[4],
	dm->dm_ph.iheader.sourceaddr[5],
	dm->dm_ph.iheader.length);
    fprintf(outf, "  DUX proto: real_dest %d  p_flgs 0x%04x  p_rid      0x%08x\n",
	dm->dm_ph.real_dest, dm->dm_ph.p_flags, dm->dm_ph.p_rid);
    fprintf(outf, "   p_dmmsg_len 0x%04x  p_byte_no  0x%04x  p_data_len     0x%04x\n",
	dm->dm_ph.p_dmmsg_length, dm->dm_ph.p_byte_no,
	dm->dm_ph.p_data_length);
    fprintf(outf, "   p_dat_offst 0x%04x  p_req_indx 0x%04x  p_rep_indx     0x%04x\n",
	dm->dm_ph.p_data_offset, dm->dm_ph.p_req_index,
	dm->dm_ph.p_rep_index);
    fprintf(outf, "   p_srcsite     0x%02x  p_retry_cnt %5d  p_seqno    0x%08x\n",
	dm->dm_ph.p_srcsite, dm->dm_ph.p_retry_cnt,
	(u_long)dm->dm_ph.p_seqno);
    fprintf(outf, "   p_version     0x%02x ", dm->dm_ph.p_version);

#define pflags dm->dm_ph.p_flags

    if (pflags & P_REQUEST)
	fprintf(outf, " P_REQUEST");
    if (pflags & P_REPLY)
	fprintf(outf, " P_REPLY");
    if (pflags & P_ACK)
	fprintf(outf, " P_ACK");
    if (pflags & P_DATAGRAM)
	fprintf(outf, " P_DATAGRAM");
    if (pflags & P_END_OF_MSG)
	fprintf(outf, " P_END_OF_MSG");
    if (pflags & P_IDEMPOTENT)
	fprintf(outf, " P_IDEMPOTENT");
    if (pflags & P_SLOW_REQUEST)
	fprintf(outf, " P_SLOW_REQUEST");
    if (pflags & P_MULTICAST)
	fprintf(outf, " P_MULTICAST");
    if (pflags & P_ALIVE_MSG)
	fprintf(outf, " P_ALIVE_MSG");
    if (pflags & P_NAK)
	fprintf(outf, " P_NAK");
    if (pflags & P_SUICIDE)
	fprintf(outf, " P_SUICIDE");
    if (pflags & P_FORWARD)
	fprintf(outf, " P_FORWARD");
    if (pflags & ~(P_ALLFLAGS))
	fprintf(outf, " P_0x%04x", pflags & ~(P_ALLFLAGS));

    fprintf(outf, "\n  dmn_func  0x%08x  dmn_flags     0x%04x  dmn_bufoffset 0x%04x\n",
	dm->dm_func, (u_short)dm->dm_flags, dm->dm_bufoffset);
    fprintf(outf, "  dmn_bufp  0x%08x  dmn_mid   0x%08x  dmn_dest      ",
	dm->dm_bufp, dm->dm_mid);

    switch (dm->dm_dest)
    {
    case DM_MULTISITE:
	fprintf(outf, "MULTISITE\n");
	break;
    case DM_DUXCAST:
	fprintf(outf, "DUXCAST\n");
	break;
    case DM_CLUSTERCAST:
	fprintf(outf, "CLUSTERCAST\n");
	break;
    case DM_DIRECTEDCAST:
	fprintf(outf, "DIRECTEDCAST\n");
	break;
    default:
	fprintf(outf, "%6d\n", dm->dm_dest);
    }

    if (dm->dm_bufp)
    {
	struct buf bp;

	if (getchunk(KERNELSPACE, dm->dm_bufp, &bp, sizeof bp, cp) == 0)
	{
	    fprintf(outf, "  b_addr    0x%08x\n", bp.b_un.b_addr);
	}
    }

    fprintf(outf, "  dmt_tflgs     0x%04x  dmt_headerlen 0x%04x  dmt_datalen   0x%04x\n",
	dm->dm_tflags, dm->dm_headerlen, dm->dm_datalen);
    fprintf(outf, "  dmt_acflag      0x%02x  dmt_lan_card   %5d", dm->dm_acflag,
		dm->dm_lan_card);

    if (pflags & P_REQUEST)
    {
        fprintf(outf, "  dmt_pid     %8d  dmt_op        0x%04x\n",
	    dm->dm_pid, dm->dm_op);
	if (dm->dm_op <= DM_LOCKED) /* last DM opcode! */
	    fprintf(outf, "  [%s]\n", dm_opcode_descrp[dm->dm_op]);
	else
	    fprintf(outf, "  [UNKNOWN_OPCODE]\n");
    }

    if (pflags & P_REPLY)
    {
	fprintf(outf, "  dmt_rc        0x%04x\n  dmt_eosys     0x%04x\n",
	    dm->dm_rc, dm->dm_eosys);
    }
#undef pflags

    if (dm->dm_flags & DM_SLEEP)
	fprintf(outf, "  DM_SLEEP");
    if (dm->dm_flags & DM_DONE)
	fprintf(outf, "  DM_DONE");
    if (dm->dm_flags & DM_FUNC)
	fprintf(outf, "  DM_FUNC");
    if (dm->dm_flags & DM_REPEATABLE)
	fprintf(outf, "  DM_REPEATABLE");
    if (dm->dm_flags & DM_RELEASE_REQUEST)
	fprintf(outf, "  DM_RELEASE_REQUEST");
    if (dm->dm_flags & DM_RELEASE_REPLY)
	fprintf(outf, "  DM_RELEASE_REPLY");
    if (dm->dm_flags & DM_REQUEST_BUF)
	fprintf(outf, "  DM_REQUEST_BUF");
    if (dm->dm_flags & DM_REPLY_BUF)
	fprintf(outf, "  DM_REPLY_BUF");
    if (dm->dm_flags & DM_SOUSIG)
	fprintf(outf, "  DM_SOUSIG");
    if (dm->dm_flags & DM_LONGJMP)
	fprintf(outf, "  DM_LONGJMP");
    if (dm->dm_flags & DM_SIGPENDING)
	fprintf(outf, "  DM_SIGPENDING");
    if (dm->dm_flags & DM_LIMITED_OK)
	fprintf(outf, "  DM_LIMITED_OK");
    if (dm->dm_flags & DM_KEEP_REQUEST_BUF)
	fprintf(outf, "  DM_KEEP_REQUEST_BUF");
    if (dm->dm_flags & DM_KEEP_REPLY_BUF)
	fprintf(outf, "  DM_KEEP_REPLY_BUF");
    if (dm->dm_flags & DM_DATAGRAM)
	fprintf(outf, "  DM_DATAGRAM");
    if (dm->dm_flags & DM_INTERRUPTABLE)
	fprintf(outf, "  DM_INTERRUPTABLE");
    if ((u_short)dm->dm_flags & ~(DM_ALLFLAGS))
	fprintf(outf, "  DM_0x%04x", dm->dm_flags & ~(DM_ALLFLAGS));
    if (dm->dm_flags)
	fprintf(outf, "\n");
}

dumpcct(cp,cctp)
char *cp;
struct cct *cctp;
{
    /*
     * Only print information for entires that have been used
     */
    if (cctp->net_addr[0] == 0 && cctp->net_addr[1] == 0 &&
	cctp->net_addr[2] == 0 && cctp->net_addr[3] == 0 &&
	cctp->net_addr[4] == 0 && cctp->net_addr[5] == 0)
	return;

    fprintf(outf, "%s [%d]  CCT address 0x%08x  status 0x%02x",
	    cp, cctp - clustab, vclustab + (cctp - clustab), cctp->status);

    switch (cctp->status)
    {
    case CL_FAILED:
	fprintf(outf, " CL_FAILED\n");
	break;
    case CL_CLEANUP:
	fprintf(outf, " CL_CLEANUP\n");
	break;
    case CL_INACTIVE:
	fprintf(outf, " CL_INACTIVE\n");
	break;
    case CL_IS_MEMBER:
	fprintf(outf, " CL_IS_MEMBER\n");
	break;
    case CL_ALIVE:
	fprintf(outf, " CL_ALIVE\n");
	break;
    case CL_ACTIVE:
	fprintf(outf, " CL_ACTIVE\n");
	break;
    case CL_RETRY:
	fprintf(outf, " CL_RETRY\n");
	break;
    default:
	fprintf(outf, " bad status %d\n", cctp->status);
    }

    fprintf(outf,
	" net_addr 0x%.2x%.2x%.2x%.2x%.2x%.2x  nak_time 0x%08x  tot_bufs 0x%08x\n",
	cctp->net_addr[0], cctp->net_addr[1], cctp->net_addr[2],
	cctp->net_addr[3], cctp->net_addr[4], cctp->net_addr[5],
	cctp->nak_time, cctp->total_buffers);

    fprintf(outf, " cpu_arch ");
    switch(cctp->cpu_arch)
    {
    case CCT_CPU_ARCH_300:
	fprintf(outf, "ARCH_300");
	break;
#ifdef CCT_CPU_ARCH_700
    case CCT_CPU_ARCH_700:
	fprintf(outf, "ARCH_700");
	break;
#endif
    case CCT_CPU_ARCH_800:
	fprintf(outf, "ARCH_800");
	break;
    default:
	fprintf(outf, "ARCH_??? (%d)", cctp->cpu_arch);
	break;
    }

    fprintf(outf, " cpu_model %3d  swap_site %3d  req_count %d\n",
	cctp->cpu_model, cctp->swap_site, cctp->req_count);
    fprintf(outf, " Qhead 0x%08x  Qtail 0x%08x  site_seqno 0x%08x\n",
	cctp->req_waiting_Qhead, cctp->req_waiting_Qtail,
	cctp->site_seqno);
    fprintf(outf, " lan card %d",
	cctp->lan_card);
}

char *nsp_timeouts[] = {
	"SHRT",
	"MED",
	"LONG"
	};

#define NSP_ALLFLAGS (NSP_BUSY		| \
		      NSP_VALID		| \
		      NSP_TIMED_OUT	| \
		      NSP_LIMITED)

dumpnsp(cp,np)
char *cp;
struct nsp *np;
{

    /*
     * Only print information for entires that have been used
     */
    if (np->nsp_proc == 0)
	return;

    fprintf(outf,
	"%s [%d]  address 0x%08x  nsp_flags 0x%02x  nsp_timeout_type %s\n",
	cp, np - nsp, vnsp + (int)(np - nsp), np->nsp_flags,
	nsp_timeouts[np->nsp_timeout_type]);
    fprintf(outf,
	" nsp_proc 0x%08x  nsp_rid 0x%08x  nsp_site 0x%04x  nsp_pid %d\n",
	np->nsp_proc, np->nsp_rid, np->nsp_site, np->nsp_pid);
    if (np->nsp_cred)
	dumpcred(" nsp_cred: ", np->nsp_cred);

    if (np->nsp_flags & NSP_BUSY)
	fprintf(outf, " NSP_BUSY");
    if (np->nsp_flags & NSP_VALID)
	fprintf(outf, " NSP_VALID");
    if (np->nsp_flags & NSP_TIMED_OUT)
	fprintf(outf, " NSP_TIMED_OUT");
    if (np->nsp_flags & NSP_LIMITED)
	fprintf(outf, " NSP_LIMITED");
    if (np->nsp_flags & ~(NSP_ALLFLAGS))
	fprintf(outf, " NSP_0x%02x", np->nsp_flags & ~(NSP_ALLFLAGS));
    if (np->nsp_flags)
	fprintf(outf, "\n");
}

getdm_header(addr, dmh, data_addr, cp)
u_long addr;
struct dm_header *dmh;
u_long *data_addr;
char *cp;
{
    struct mbuf mb;

    if (getchunk(KERNELSPACE, addr, &mb, sizeof mb, cp))
    {
	fprintf(outf, "*** %s: Cannot read mbuf at 0x%08x\n", cp, addr);
	return -1;
    }

    /*
     * Simple mbuf, data is in mbuf, copy to *dmh and return
     */
    if (mb.m_off == 0)
    {
	memcpy(dmh, &mb, sizeof (struct dm_header));
	*data_addr = addr;
	return 0;
    }

    /*
     * Data is in cluster, perform mtod(mbuf) and read the data
     * from that address.
     */
    addr = (u_long)((int)addr + mb.m_off);
    if (getchunk(KERNELSPACE, addr, dmh, sizeof (struct dm_header), cp))
    {
	fprintf(outf, "*** %s: Cannot read mcluster at 0x%08x\n",
	    cp, addr);
	return -1;
    }

    *data_addr = addr;
    return 0;
}

dump_multisite(addr)
u_long addr;
{
    struct dm_header hdr;
    struct dm_multisite mult;
    struct buf buf;
    u_long data_addr;

    /*
     * Get the mbuf holding the msg.
     */
    if (getdm_header(addr, &hdr, &data_addr, "dump_multisite"))
	return;

    /*
     * Now read the buffer
     */
    addr = (u_long)hdr.dm_bufp;
    if (getchunk(KERNELSPACE, addr, &buf, sizeof buf, "dump_multisite"))
    {
	fprintf(outf, "*** Cannot read rep_mbuf.dm_bufp at 0x%08x\n",
	    addr);
	return;
    }

    /*
     * Now read the data pointed to by the buffer.
     */
    addr = (u_long)buf.b_un.b_addr;
    if (getchunk(KERNELSPACE, addr, &mult, sizeof (mult), "dump_multisite"))
    {
	fprintf(outf, "*** Cannot read multisite structure at 0x%08x\n",
	    addr);
	return;
    }

    /*
     * Whew!  Finally got the data, print it out.
     */
    fprintf(outf, "** Multisite Structure:\n");
    fprintf(outf, "    maxsite           %3d   remain_sites %3d\n",
	    mult.maxsite, mult.remain_sites);
    fprintf(outf, "    no_response_sites %3d   next_dest    %3d\n",
	    mult.no_response_sites, mult.next_dest);
    for (addr = 0; addr <= mult.maxsite; addr++)
    {
	struct dm_site *p = &mult.dm_sites[addr];

	/*
	 * Only print info on valid sites.
	 */
	if (p->site_is_valid == 0)
	    continue;

	fprintf(outf, "   dm_sites[%02d]:\n", addr);
	fprintf(outf, "     site_is_valid %1d   status ",
		p->site_is_valid);
	if (p->status == DM_MULTI_NORESP)
	    fprintf(outf, "DM_MULTI_NORESP\n");
	else if (p->status == DM_MULTI_SLOWOP)
	    fprintf(outf, "DM_MULTI_SLOWOP\n");
	else if (p->status == DM_MULTI_DONE)
	    fprintf(outf, "DM_MULTI_DONE\n");
	else
	    fprintf(outf, "UNKNOWN (0x%02x)\n", p->status);
	fprintf(outf, "     rc         %4d   eosys  %4d\n",
		p->rc, p->eosys);
	fprintf(outf, "     acflag   0x%04x   tflags 0x%04x\n",
		p->acflag, p->tflags);
    }
}

#ifdef U_REPLY_RCVD
#define U_ALLFLAGS (U_USED		| \
		    U_IDEMPOTENT	| \
		    U_IN_DUXQ		| \
		    U_REPLY_RCVD	| \
		    U_RETRY_IN_PROGRESS	| \
		    U_REPLY_IN_PROGRESS	| \
		    U_COUNTED		| \
		    U_OUTSIDE_WINDOW)
#else
#define U_ALLFLAGS (U_USED		| \
		    U_IDEMPOTENT	| \
		    U_IN_DUXQ		| \
		    U_RETRY_IN_PROGRESS	| \
		    U_REPLY_IN_PROGRESS	| \
		    U_COUNTED		| \
		    U_OUTSIDE_WINDOW)
#endif /* U_REPLY_RCVD */

dumpusingarray(cp, uep)
char *cp;
struct using_entry *uep;
{
    /*
     * Only print information for entires that have been used
     */
    if (uep->req_mbuf == 0 && uep->rep_mbuf == 0 && uep->flags == 0)
	return;

    fprintf(outf, "%s [%d]  address 0x%08x  flags 0x%02x  rid 0x%08x\n",
	cp, (uep - using_array), vusing_array + (uep - using_array),
	uep->flags, uep->rid);
    fprintf(outf, " req_mbuf 0x%08x  rep_mbuf 0x%08x  no_retry 0x%02x  byte_rcvd 0x%04x\n",
	uep->req_mbuf, uep->rep_mbuf, uep->no_retried, uep->byte_recved);

    if (uep->flags & U_USED)
	fprintf(outf, " U_USED");
    if (uep->flags & U_IDEMPOTENT)
	fprintf(outf, " U_IDEMPOTENT");
    if (uep->flags & U_IN_DUXQ)
	fprintf(outf, " U_IN_DUXQ");
#ifdef U_REPLY_RCVD
    if (uep->flags & U_REPLY_RCVD)
	fprintf(outf, " U_REPLY_RCVD");
#endif
    if (uep->flags & U_RETRY_IN_PROGRESS)
	fprintf(outf, " U_RETRY_IN_PROGRESS");
    if (uep->flags & U_REPLY_IN_PROGRESS)
	fprintf(outf, " U_REPLY_IN_PROGRESS");
    if (uep->flags & U_COUNTED)
	fprintf(outf, " U_COUNTED");
    if (uep->flags & U_OUTSIDE_WINDOW)
	fprintf(outf, " U_OUTSIDE_WINDOW");
    if (uep->flags & ~(U_ALLFLAGS))
	fprintf(outf, " U_0x%02x", uep->flags & ~(U_ALLFLAGS));
    if (uep->flags)
	fprintf(outf, "\n");

    /*
     * Do not reference pointers in an entry that is not
     * currently in use.
     * If this entry is used, print some info about the message,
     * including the multisite status, if applicable.
     */
    if ((uep->flags & U_USED) &&
	uep->req_mbuf != 0 && uep->rep_mbuf != 0)
    {
	struct dm_header hdr;
	u_long data_addr;

	/*
	 * Read the header part of the request.  Print it out.
	 */
	if (getdm_header(uep->req_mbuf, &hdr, &data_addr, "dumpusingarray"))
	    return;

	dumpdmheader("  req_mbuf", &hdr, data_addr);

	/*
	 * If this is a MULTISITE message, print the multisite stuff.
	 */
	if (hdr.dm_dest == DM_MULTISITE || hdr.dm_dest == DM_CLUSTERCAST)
	    dump_multisite(uep->rep_mbuf);
    }
}

#ifdef S_ACK_RCVD
#define S_ALLFLAGS (S_USED		| \
		    S_IDEMPOTENT	| \
		    S_IN_DUXQ		| \
		    S_SLOW_REQUEST	| \
		    S_MULTICAST		| \
		    S_ACK_RCVD)
#else
#define S_ALLFLAGS (S_USED		| \
		    S_IDEMPOTENT	| \
		    S_IN_DUXQ		| \
		    S_SLOW_REQUEST	| \
		    S_MULTICAST)
#endif

dumpservingarray(cp, sep)
char *cp;
struct serving_entry *sep;
{
    /*
     * Only print information for entires that have been used
     */
    if (sep->time_stamp == 0)
	return;

    fprintf(outf, "%s [%d]  address 0x%08x  flags 0x%02x  req_indx 0x%02x  state 0x%02x\n",
	cp, (sep - serving_array), vserving_array + (sep - serving_array),
	sep->flags, sep->req_index, sep->state);
    fprintf(outf, " rid 0x%08x  msg_mbuf 0x%08x  byte_rcvd 0x%04x  no_retry 0x%02x\n",
	sep->rid, sep->msg_mbuf, sep->byte_recved, sep->no_retried);
    fprintf(outf, " req_addr 0x%.2x%.2x%.2x%.2x%.2x%.2x ",
	sep->req_address[0], sep->req_address[1], sep->req_address[2],
	sep->req_address[3], sep->req_address[4], sep->req_address[5]);
    fprintf(outf, "time_stamp %lu ", sep->time_stamp);

    if (sep->flags & S_USED)
	fprintf(outf, " S_USED");
    if (sep->flags & S_IDEMPOTENT)
	fprintf(outf, " S_IDEMPOTENT");
    if (sep->flags & S_IN_DUXQ)
	fprintf(outf, " S_IN_DUXQ");
    if (sep->flags & S_SLOW_REQUEST)
	fprintf(outf, " S_SLOW_REQUEST");
    if (sep->flags & S_MULTICAST)
	fprintf(outf, " S_MULTICAST");
#ifdef S_ACK_RCVD
    if (sep->flags & S_ACK_RCVD)
	fprintf(outf, " S_ACK_RCVD");
#endif
    if (sep->flags & ~(S_ALLFLAGS))
	fprintf(outf, " S_0x%02x", sep->flags & ~(S_ALLFLAGS));

    switch (sep->state)
    {
    case NOT_ACTIVE:
	fprintf(outf, " NOT_ACTIVE");
	break;
    case RECVING_REQUEST:
	fprintf(outf, " RECVING_REQUEST");
	break;
    case SERVING:
	fprintf(outf, " SERVING");
	break;
    case SENDING_REPLY:
	fprintf(outf, " SENDING_REPLY");
	break;
    case WAITING_ACK:
	fprintf(outf, " WAITING_ACK");
	break;
    case ACK_RECEIVED:
	fprintf(outf, " ACK_RECEIVED");
	break;
    default:
	fprintf(outf, " bad state %d", sep->state);
    }
    fprintf(outf, "\n");

    /*
     * Do not reference pointers in an entry that is not
     * currently in use.
     * If this entry is used, print some info about the message.
     */
    if ((sep->flags & S_USED) && sep->msg_mbuf != 0)
    {
	struct dm_header hdr;
	u_long data_addr;

	/*
	 * Read the header part of the msg_mbuf and print it out.
	 */
	if (getdm_header(sep->msg_mbuf, &hdr, &data_addr, "dumpservingarray"))
	    return;

	dumpdmheader("  msg_mbuf", &hdr, data_addr);
    }
}

selftest_status()
{
    fprintf(outf, "\n DUX selftest status: 0x%08x  \n Passed",
	selftest_status);
    if (selftest_passed & ST_BUFFER)
	fprintf(outf, " ST_BUFFER");
#ifdef hp9000s200
    if (selftest_passed & (int)ST_NETBUF)
	fprintf(outf, " ST_NETBUF");
#endif
    if (selftest_passed & (int)ST_DM_MESSAGE)
	fprintf(outf, " ST_DM_MESSAGE");
    if (selftest_passed & (int)ST_NSP)
	fprintf(outf, " ST_NSP");
    fprintf(outf, "\n");
}

nsp_table()
{
    static char *nsp_vars[] =
    {
	"num_nsps",
	"free_nsps",
	"nsps_to_invoke",
	"nsps_started",
	"nsp_first_try",
	"max_nsp",
	(char *)0
    };
    int i;
    u_long x;
    int y;
    struct csp_stats csp_stats;
    int active_nsps;	/* num_nsps */
    int free_nsps;	/* free_nsps */

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                           SCANNING NSP TABLE                        *\n");
    fprintf(outf, "***********************************************************************\n\n");

    /*
     * Print the NSP variables that we are interested in
     */
    for (i = 0; nsp_vars[i] != (char *)0; i++)
    {
	fprintf(outf, "  %s: ", nsp_vars[i]);
	if ((x = lookup(nsp_vars[i])) == 0)
	    fprintf(outf, "<symbol not found>\n");
	else
	{
	    int val = get(x);
	    fprintf(outf, "%d\n", val);

	    /*
	     * Save some values for CSP stats
	     */
	    if (strcmp(nsp_vars[i], "num_nsps") == 0)
		active_nsps = val;
	    else if (strcmp(nsp_vars[i], "free_nsps") == 0)
		free_nsps = val;
	}
    }

    /*
     * Now get CSP statistics
     */
    if ((x = lookup("csp_stats")) == 0 ||
	getchunk(KERNELSPACE, x, &csp_stats, sizeof csp_stats, "nsp_table"))
    {
	fprintf(outf, "\nCannot find CSP statistics\n");
    }
    else
    {

	fprintf(outf, "\n *** CSP STATISTICS ***\n");
	fprintf(outf, "Limited CSP queue length: %3d (%2d max)\n",
	    csp_stats.limitedq_curlen,
	    csp_stats.limitedq_maxlen);
	fprintf(outf, "Limited CSP requests: %7d",
	    csp_stats.requests[CSPSTAT_LIMITED]);
	fprintf(outf, " (%6.3f second maximum service time)\n",
	    (double) csp_stats.max_lim_time / (double) HZ);
	fprintf(outf, "General CSP queue length: %3d (%2d max)",
	    csp_stats.generalq_curlen, csp_stats.generalq_maxlen);
	fprintf(outf, " %2d active %2d idle (%d minimum idle)\n",
	    active_nsps,
	    free_nsps,
	    csp_stats.min_gen_free);
	fprintf(outf, "GCSP requests by timeout:");
	fprintf(outf, " %9d short %9d medium %9d long\n",
	    csp_stats.requests[CSPSTAT_SHRT],
	    csp_stats.requests[CSPSTAT_MED],
	    csp_stats.requests[CSPSTAT_LONG]);
	fprintf(outf, "GCSP timeouts:");
	fprintf(outf, " %20d short %9d medium %9d long\n",
	    csp_stats.timeouts[CSPSTAT_SHRT],
	    csp_stats.timeouts[CSPSTAT_MED],
	    csp_stats.timeouts[CSPSTAT_LONG]);
    }

    for (i = 0; i < ncsp; i++)
	dumpnsp("\nNSP entry: ", &nsp[i]);
}

cluster_table()
{
    int i;

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                       SCANNING CLUSTER TABLE                        *\n");
    fprintf(outf, "***********************************************************************\n\n");

    fprintf(outf, " my_site: 0x%04x  root_site: 0x%04x  swap_site: 0x%04x\n",
	my_site, root_site, swap_site);
    fprintf(outf, " my_site_status: 0x%08x\n", my_site_status);

    for (i = 0; i < MAXSITE; i++)
	dumpcct("\nCCT entry: ", &clustab[i]);
}

scan_using_array()
{
    int i;

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                         SCANNING USING ARRAY                        *\n");
    fprintf(outf, "***********************************************************************\n\n");
    for (i = 0; i < using_array_size; i++)
	dumpusingarray("\nUsing entry:", &using_array[i]);
}

scan_serving_array()
{
    int i;

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                       SCANNING SERVING ARRAY                        *\n");
    fprintf(outf, "***********************************************************************\n\n");
    for (i = 0; i < serving_array_size; i++)
	dumpservingarray("\nServing entry:", &serving_array[i]);
}

dump_nsp_queue(ptr, str, fn)
struct mbuf *ptr;
char *str;
char *fn;
{
    u_long data_addr;
    struct dm_header hdr;
    struct mbuf m;

    while (ptr != (struct mbuf *)0)
    {
	if (getchunk(KERNELSPACE, ptr, &m, sizeof m, fn))
	    return;

	/*
	 * Read the header part of the msg_mbuf and print it out.
	 */
	if (getdm_header(ptr, &hdr, &data_addr, fn))
	    return;

	dumpdmheader(str, &hdr, data_addr);
	fprintf(outf, "\n");

	ptr = m.m_act;
    }
}

scan_limited_queue()
{
    u_long x;

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                         SCANNING LCSP QUEUE                         *\n");
    fprintf(outf, "***********************************************************************\n\n");

    if ((x = lookup("limited_head")) == 0)
    {
	fprintf(outf, "\nCannot find \"limited_head\"\n");
	return;
    }
    dump_nsp_queue(get(x), "  lcsp_req", "scan_limited_queue");
}
    
scan_general_queue()
{
    u_long x;

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                         SCANNING GCSP QUEUE                         *\n");
    fprintf(outf, "***********************************************************************\n\n");

    if ((x = lookup("nsp_head")) == 0)
    {
	fprintf(outf, "\nCannot find \"nsp_head\"\n");
	return;
    }
    dump_nsp_queue(get(x), "  gcsp_req", "scan_general_queue");
}
    
scan_dux_mbufs()
{
}

dump_net_bchain()
{
	register struct buf *vbp, *pbp;
	struct buf dummy;
        struct buf *bp = &dummy;
	unsigned absaddr;

        if (net_bchain.av_forw == NULL){
                fprintf(outf,"   Buffer list empty\n");
        } else {

		pbp = vnet_bchain;
        	vbp = net_bchain.av_forw;
		absaddr = getphyaddr ((unsigned) vbp);
                longlseek(fcore, (long)clear(absaddr), 0);
                if (longread(fcore, (char *)bp, sizeof (struct buf))
                        != sizeof (struct buf)) {
                        perror("dm_info buf longread");
                        return;
                }
                for (; bp;){

			dumpbuf(" net_bchain ",bp, vbp);

                        /* Check for end of chain */
                        if (vbp == vnet_bchain)
                            break;

                	/* get next entry */
                	pbp = vbp;
                	vbp = bp->av_forw;
                        absaddr = getphyaddr ((unsigned) vbp);
                        longlseek(fcore, (long)clear(absaddr), 0);
                        if (longread(fcore, (char *)bp, sizeof (struct buf))
                                != sizeof (struct buf)) {
                                perror("net_bchain longread");
                                break;
                        }

		}
	}
}
dux_mbstat_info()
{
        printf("\n------------------------ dux_mbstat --------------------\n");
        printf("struct dux_mbstat{\n");
        printf("  buf pool size              = %d\n",dux_mbstat.m_netbufs);
        printf("  bufs on free list          = %d\n",dux_mbstat.m_nbfree);
        printf("  total bufs used            = %d\n",dux_mbstat.m_nbtotal);

        printf("  net pages                  = %d\n",dux_mbstat.m_netpages);
        printf("  free pages                 = %d\n",dux_mbstat.m_freepages);
        printf("  total pages used           = %d\n",dux_mbstat.m_totalpages);
        printf("}\n");
}
dm_info()
{
    int i;
    int good;

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                         SCANNING DISKLESS FSBUFS                    *\n");
    fprintf(outf, "***********************************************************************\n\n");

    dux_mbstat_info();
    dump_net_bchain();

    fprintf(outf, "\n\n\n***********************************************************************\n");
    fprintf(outf, "*                         SCANNING DUX DM INFO                        *\n");
    fprintf(outf, "***********************************************************************\n\n");

    fprintf(outf, "dm_is_interrupt: 0x%08x  dm_int_dm_messagge: 0x%08x\n\n");

    /*
     * We print all the inbound opcode stats except ones that are
     * DM_INVALID.  We only print the DM_INVALID ones if they have
     * a non-zero count
     */
    fprintf(outf, "INBOUND OPCODE STATS:\n\n");
    for (i = 0; *dm_opcode_descrp[i] != '\0'; i++)
	if (inbound_opcode_stats.opcode_stats[i] != 0 ||
	    strncmp(dm_opcode_descrp[i], "DM_INVAL", 8) != 0)
	    fprintf(outf, "%-22s %d\n", dm_opcode_descrp[i],
		inbound_opcode_stats.opcode_stats[i]);

    /*
     * We print all the outbound opcode stats except ones that are
     * DM_INVALID.  We only print the DM_INVALID ones if they have
     * a non-zero count
     */
    fprintf(outf, "\n\nOUTBOUND OPCODE STATS:\n\n");
    for (i = 0; *dm_opcode_descrp[i] != '\0'; i++)
	if (outbound_opcode_stats.opcode_stats[i] != 0 ||
	    strncmp(dm_opcode_descrp[i], "DM_INVAL", 8) != 0)
	    fprintf(outf, "%-22s %d\n", dm_opcode_descrp[i],
		outbound_opcode_stats.opcode_stats[i]);

    /*
     * Now print the proto_stats structure
     */
    if ((i = lookup("proto_stats")) == 0)
	return;

    if (getchunk(KERNELSPACE, i, &proto_stats, sizeof proto_stats, "dm_info"))
	return;

    fprintf(outf, "\n\nPROTOCOL STATS:\n\n");
    printf("STATS_lost_first              = %d\n",
	    STATS_lost_first);
    printf("STATS_recv_no_mbuf            = %d\n",
	    STATS_recv_no_mbuf);
    printf("STATS_recv_no_cluster         = %d\n",
	    STATS_recv_no_cluster);
    printf("STATS_recv_no_buf             = %d\n",
	    STATS_recv_no_buf);
    printf("STATS_recv_no_buf_hdr         = %d\n",
	    STATS_recv_no_buf_hdr);
    printf("STATS_not_clustered           = %d\n",
	    STATS_not_clustered);
    printf("STATS_dup_req                 = %d\n",
	    STATS_dup_req);
    printf("STATS_recv_req_OOS            = %d\n",
	    STATS_recv_req_OOS);
    printf("STATS_req_retries             = %d\n",
	    STATS_req_retries);
    printf("STATS_unexpected              = %d\n",
	    STATS_unexpected);
    printf("STATS_retry_reply             = %d\n",
	    STATS_retry_reply);
    printf("STATS_waiting_using           = %d\n",
	    STATS_waiting_using);
    printf("STATS_serving_entry           = %d\n",
	    STATS_serving_entry);
    printf("STATS_ack_no_mbuf             = %d\n",
	    STATS_ack_no_mbuf);
    printf("STATS_slow_no_mbuf            = %d\n",
	    STATS_slow_no_mbuf);
    printf("STATS_delta_sec               = %d\n",
	    STATS_delta_sec);
    printf("STATS_recv_dgram_no_mbuf      = %d\n",
	    STATS_recv_dgram_no_mbuf);
    printf("STATS_recv_dgram_no_cluster   = %d\n",
	    STATS_recv_dgram_no_cluster);
    printf("STATS_recv_dgram_no_buf       = %d\n",
	    STATS_recv_dgram_no_buf);
    printf("STATS_recv_dgram_no_buf_hdr   = %d\n",
	    STATS_recv_dgram_no_buf_hdr);
    printf("STATS_recv_bad_flags          = %d\n",
	    STATS_recv_bad_flags);
    printf("STATS_recv_bad_srcsite        = %d\n",
	    STATS_recv_bad_srcsite);
    printf("STATS_recv_not_member         = %d\n",
	    STATS_recv_not_member);
    printf("STATS_req_not_member          = %d\n",
	    STATS_req_not_member);
    printf("STATS_recv_op_P_REQUEST       = %d\n",
	    STATS_recv_op_P_REQUEST);
    printf("STATS_recv_op_P_REPLY         = %d\n",
	    STATS_recv_op_P_REPLY);
    printf("STATS_recv_op_P_ACK           = %d\n",
	    STATS_recv_op_P_ACK);
    printf("STATS_recv_op_P_NAK           = %d\n",
	    STATS_recv_op_P_NAK);
    printf("STATS_recv_op_P_SLOW_REQUEST  = %d\n",
	    STATS_recv_op_P_SLOW_REQUEST);
    printf("STATS_recv_op_P_DATAGRAM      = %d\n",
	    STATS_recv_op_P_DATAGRAM);
    printf("STATS_xmit_op_P_REQUEST       = %d\n",
	    STATS_xmit_op_P_REQUEST);
    printf("STATS_xmit_op_P_REPLY         = %d\n",
	    STATS_xmit_op_P_REPLY);
    printf("STATS_xmit_op_P_ACK           = %d\n",
	    STATS_xmit_op_P_ACK);
    printf("STATS_xmit_op_P_NAK           = %d\n",
	    STATS_xmit_op_P_NAK);
    printf("STATS_xmit_op_P_SLOW_REQUEST  = %d\n",
	    STATS_xmit_op_P_SLOW_REQUEST);
    printf("STATS_xmit_op_P_DATAGRAM      = %d\n",
	    STATS_xmit_op_P_DATAGRAM);
    printf("STATS_xmit_no_buffer_send     = %d\n",
	    STATS_xmit_no_buffer_send);
    printf("STATS_xmit_hw_failure         = %d\n",
	    STATS_xmit_hw_failure);
#ifdef STATS_fast_ack
    printf("STATS_fast_ack                = %d\n",
	    STATS_fast_ack);
#endif
#ifdef STATS_many_fast_acks
    printf("STATS_many_fast_acks          = %d\n",
	    STATS_many_fast_acks);
#endif
#ifdef STATS_old_serving_entry
    printf("STATS_old_serving_entry       = %d\n",
	    STATS_old_serving_entry);
#endif
#ifdef STATS_reply_alive_nobuf
    printf("STATS_reply_alive_nobuf       = %d\n",
	    STATS_reply_alive_nobuf);
#endif
#ifdef STATS_pkt_alloc_nobuf
    printf("STATS_pkt_alloc_nobuf         = %d\n",
	    STATS_pkt_alloc_nobuf);
#endif
}
