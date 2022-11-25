/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/protocol.c,v $
 * $Revision: 1.13.83.4 $	$Author: rpc $
 * $State: Exp $	$Locker:  $
 * $Date: 93/10/20 10:53:29 $
 */
/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

		    RESTRICTED RIGHTS LEGEND

	  Use,	duplication,  or disclosure by the Government  is
	  subject to restrictions as set forth in subdivision (b)
	  (3)  (ii)  of the Rights in Technical Data and Computer
	  Software clause at 52.227-7013.

		     HEWLETT-PACKARD COMPANY
			3000 Hanover St.
		      Palo Alto, CA  94304
*/

#ifndef _SYS_STDSYMS_INCLUDED
#    include "../h/stdsyms.h"
#endif   /* _SYS_STDSYMS_INCLUDED  */

#include  "../h/param.h"
#include  "../h/buf.h"
#include  "../h/kernel.h"
#include  "../h/malloc.h"
#ifdef __hp9000s300
#include  "../wsio/timeout.h"
#include  "../wsio/intrpt.h"
#include  "../s200io/lnatypes.h"
#include  "../s200io/drvhw.h"
#endif /* __hp9000s300 */

#if defined(__hp9000s700) || defined(__hp9000s800)
#include  "../h/systm.h"
#include  "../h/protosw.h"
#endif

/*
** following 7 includes are needed by drv_lan0.h
*/
#include  "../h/socket.h"
#include  "../net/if.h"			/* defines struct ifnet	 */
#include  "../netinet/in.h"		/* defines struct arpcom */
#if defined(__hp9000s700) || defined(__hp9000s800)
#include  "../sio/llio.h"
#endif

#include  "../net/route.h"
#include  "../net/raw_cb.h"

#if defined(__hp9000s700) || defined(__hp9000s800)
#include  "../net/netisr.h"
#include  "../h/mbuf.h"
#include  "../machine/vmparam.h"
#include  "../h/sysmacros.h"
#endif /* s700 || s800 */

#include  "../netinet/if_ether.h"	/* defines struct arpcom */
#include  "../netinet/if_ieee.h"	/* defines struct arpcom */
#include  "../sio/netio.h"
#include  "../sio/lanc.h"		/* for LAN break detection */

#ifdef __hp9000s300
#include "../s200io/drvhw_ift.h"
#endif

#include  "../h/conf.h"
#include  "../h/user.h"
#include  "../h/kern_sem.h"
#include  "../net/netmp.h"

#ifdef __hp9000s300
#include  "../dux/duxlaninit.h"
#endif

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

#define DUX_MULTIPLE_LAN_CARDS
#ifdef DUX_MULTIPLE_LAN_CARDS
#define DUX_ASSIGN_ADDRESS(site, ph, dmp) dux_assign_address(site, ph, dmp)
#else
#define DUX_ASSIGN_ADDRESS(site, ph, dmp) \
	DUX_COPY_ADDRESS(clustab[site].net_addr, ph->iheader.destaddr)
#endif
/*

/*
** The following are needed to obtain the local link
** address of the server a client node booted from.
*/
extern dev_t rootdev;
extern int nchrdev;
extern int req_pkt_seqno;
extern volatile char *panicstr;
extern volatile int failure_sent;

#ifdef __hp9000s300
#include  "../s200io/drvmac.h"
#endif
#include  "../dux/dm.h"
#include  "../dux/dmmsgtype.h"
#include  "../dux/protocol.h"
#include  "../dux/cct.h"

/*
** Make sure we pick up the correct timeout routine.
*/
#ifdef __hp9000s300
#undef timeout
#endif

#ifdef __hp9000s300
#define MAX_DUX_LAN_CARDS 1
#else
#define MAX_DUX_LAN_CARDS 10
#endif

/* Array of cards the server will use for communicating with cnodes */
/* Client should only have one */
struct ifnet *dux_lan_cards[MAX_DUX_LAN_CARDS];
int num_dux_lan_cards = 0;

#if defined(__hp9000s700) || defined(__hp9000s800)
/*
 * s700, s800 version -- machine independent
 */
struct ifqueue dpintrq = {0, 0, 0, 50, 0}; /* set ifq_maxlen */

struct mbufchainptr { struct mbuf *rxring; } MYhw;
typedef struct mbufchainptr *hw_gloptr;
#define DPINTRQLEN		20
#define Myglob			dux_lan_cards[0]
#define Myhw			((hw_gloptr)(&MYhw))
#define skip_frame(x)		m_freem(x->rxring)
#define INPUT_HEADER(mbufp) \
	(mtod ((struct mbuf *)(mbufp), struct dm_header *))
#undef READ_ADDR
#define READ_ADDR(x)		(INPUT_HEADER(x))
#define IEEE802_EXP_ADR		IEEESAP_HP
#define NORMAL_FRAME		IEEECTRL_DEF
#define LAN_PACKET_SIZE		1500  /* should be 1514 */
#define LAN_MIN_LEN		60
#define END_OF_MESSAGE		0x1000
#define DELAY_MESSAGE		0x2000 /* XXX */

struct mbuf *pkt_alloc();
int call_dm_recv_reply();
int release_reply_msg();
int large_dux_pkt_inQ = 0;
int max_large_dux_pkt_inQ = 26;
int dux_send_delay = 9;
char dux_lan_full[256]; /* XXX */
#define CRIT()			spl6()
#define CRIT5()			spl5()
#define UNCRIT(x)		splx(x)
#endif /* s700 || s800 */

/*
 * Maximum number of outstanding normal requests on a per client basis.
 */
u_short dux_window_size = 12;

/*
** syncgeteblk() is the routine used to obtain a file system
** buffer from the netbuf pool. These bufs are allocatable
** under interrupt.
*/
extern struct buf *syncgeteblk();
extern struct cct clustab[];

/*
** clustercast addresses.  The clustercast address is
** determined on the basis of the rootservers link addres.
*/
char clustcast_addr[ADDRESS_SIZE];

/*
** HP approved DUXcast address. Only one for all DUX machines.
*/
char duxcast_addr[ADDRESS_SIZE] = {0x09, 0x00, 0x09, 0x00, 0x00, 0x05};

#if defined(__hp9000s700) || defined(__hp9000s800)
#ifdef _WSIO

	dev_t duxlan_dev[MAX_DUX_LAN_CARDS];

#else
	int duxlan_major[MAX_DUX_LAN_CARDS] = -1;
	int duxlan_mgr_index[MAX_DUX_LAN_CARDS] = -1;
#endif /* _WSIO vs non _WSIO */
#endif /* s700 || s800 */

/*
** space for the following configurable arrays is allocated in ../h/space.h
*/
extern struct	using_entry	using_array[];
extern int	using_array_size;	/* using_array[using_array_size] */
int	waiting_for_using_array = 0;	/* 1 when request is waiting for */
					/* a using_array[] entry */

short serv_recv_req_slot[MAXSITE];
#define FREE_SERVING_SLOT	serv_recv_req_slot[0]

extern volatile struct	serving_entry	serving_array[];
extern int	serving_array_size;	/* serving_array[serving_array_size] */

/*
** Static mbufs for P_NAK , P_DATAGRAM, P_ACK, and P_SUICIDE msgs.
*/
struct mbuf nak_mbuf;
struct mbuf dgram_mbuf;
struct mbuf suicide_mbuf;
struct mbuf ack_mbuf;
int init_static_mbuf();

/*
** Static mbuf for LAN_BREAK, can't use others because this code runs
** at a lower sw_trigger level.
*/
struct mbuf lan_break_mbuf;
#define LANBREAK_MSGS_TO_SEND	16


/*
** Globals
*/
#if defined(DUX_PROTOCOL_DEBUG) || defined(NET_DEBUG)
int	net_debug = 0;			/* used for debug code levels	 */
#endif
u_long	  dux_rid = 0;			/* unique rid in a cnode request */

#ifdef __hp9000s300
u_int	prev_retry_cntr = 0;		/* Detect a broken LAN.		 */
u_int	curr_retry_cntr = 0;

u_int	prev_unsendable = 0;
u_int	curr_unsendable = 0;

u_int	prev_all_xmit_cntr = 0;
u_int	curr_all_xmit_cntr = 0;

u_int	prev_xmit_no_buffer_send = 0;
u_int	curr_xmit_no_buffer_send = 0;

u_int	prev_xmit_hw_failure = 0;
u_int	curr_xmit_hw_failure = 0;
#endif /* s300 */

#ifdef KERNEL_DEBUG_ONLY
/*
** Standalone Debugger Node only
*/
#define P_DEBUGGER		0x8000
#define DEBUGGER		0
#define RECOVERY_ON		1
#define RECOVERY_OFF		2
#endif KERNEL_DEBUG_ONLY


#define INVALID_SITEID_0	0	/* not a valid cnode id */
#define MAXRETRYLIMIT	5		/* Used by request retry timeout code */
#define TRUE		1
#define FALSE		0

#ifdef __hp9000s300
/*
** The dux transmit Q; there is no dux input Q
*/
struct mbuf *duxQhead = NULL,
		*duxQtail = NULL;
#endif /* s300 */

int	retry_request();
int	retry_reply();
#ifdef __hp9000s300
struct	sw_intloc   duxQ_intloc;	 /*  sw_trigger of dux_hw_send() */
#endif
int	dux_hw_send();
extern int wakeup();

#ifdef DUX_LOG
int u_busy_recv_reply = 0;
int u_busy_retry_request = 0;

#define NPDLOG 16384

/*
 * Diskless protocol events to be logged to a circular buffer.
 * Useful for debugging.
 */

#define PD_NET_REQUEST		1	/* net_request() */
#define PD_DUX_SEND		2	/* dux_send() */
#define PD_MCLGETX		3	/* mclget_dux() */
#define PD_RETRY_REQUEST	4	/*  */
#define PD_FREE_DUX_BP		6
#define PD_RECV_REPLY		7
#define PD_UNTIMEOUT_RETRY	8
#define PD_CALL_DM_RECV_REPLY	9	/*  */

struct pdlog {
	u_int pd_op:8,
	      pd_spl: 4,
	      pd_index: 4;	/* using array index */
	u_short pd_rid;		/* lower half-word of request id */
	int pd_arg1;
	int pd_arg2;
	int pd_arg3;
};

struct pdlog pdlog[NPDLOG];
pdlog_index = 0;
dont_log = 0;

#define DISTACK (ON_ISTACK ? 8 : 0)	/* was I called under interrupt */

void
log_pd(op, index, rid, arg1, arg2, arg3)
u_char op,index;
u_short rid;
int arg1, arg2, arg3;
{
	char my_spl();

	if (dont_log)
		return;

	pdlog[pdlog_index].pd_op = op;

	pdlog[pdlog_index].pd_spl = my_spl() | DISTACK;
	pdlog[pdlog_index].pd_index = index;
	pdlog[pdlog_index].pd_rid = rid;
	pdlog[pdlog_index].pd_arg1 = arg1;
	pdlog[pdlog_index].pd_arg2 = arg2;
	pdlog[pdlog_index].pd_arg3 = arg3;
	if (++pdlog_index >= NPDLOG)
		pdlog_index = 0;
}

#include "../machine/spl.h"

char my_spl()
{
	register s;
	splx(s=spl7());

	switch (s)
	{
	case SPL2:
		return(2);
	case SPL5:
		return(5);
	case SPL6:
		return(6);
	case SPL7:
		return(7);
	case SPLPREEMPTOK:
	case SPLNOPREEMPT:
		return(0);
	}
}
#endif DUX_LOG


/*
**		IMPORTANT: PROTOCOL LAYER AND DISCLESS
**			   NETWORK BUFFER MANAGEMENT
**			      PORTING ISSUES
**				 daveg
**
**  The following issues must be resolved if considering a port
**  to a different architecture:
**
**				PROTOCOL LAYER
**
**  1) The protocol code is tied fairly tightly to the HP S300
**	DIO LAN Driver. The "hwvars" are specific to HP to identify
**	specific global hardware attributes of a specified LAN card.
**	This result in special LAN device driver support hooks for
**	reading/writing directly to/from the card's H/W buffers.
**	They will probably need to be changed so the calls are to
**	the more traditional upper device driver layer interface.
**
**  2) The selection of which LAN card to utilize for DUX traffic is
**	also specific to HP DIO architecture. This is very localized
**	and will be simple to modify. The code simply scans the
**	available LAN Card description/attribute structures trying
**	to find a matching Link Address to the one read in clusterconf.
**
**  3) The layout of the protocol header contains a transmitted and
**	non-transmitted portion. The S300 lower level driver support
**	hooks know how to skip this non-transmitted portion. This will
**	probably be a problem if the interface is moved higher up.
**	Three approaches are possible (specified in order of desirability).
**
**	1) The non-transmitted portion of the dm_header holds
**	information like the function to call when the op completes,
**	the local buf pointer, buf data offset, non-transmit flags,
**	message id, and dest id. This information could be placed in
**	a structure of type "dm_no_transmit" and put into an identical
**	structure, with a pointer to that structure held in the
**	transmitted portion of the header. Will have to be very careful
**	in making sure all the interactions between the dm and protocol
**	layers are covered. This should also allow the elimination of
**	dux_hw_send() special hook, in the device driver.
**
**	2) Simply send the non-transmitted part and put up with
**	the extra few bytes being needlessly transmitted over the wire.
**	For minimal sized packets this is not really a problem since
**	we have to pad to the minimal size of 60 bytes anyway. For
**	maximum sized packets the cost is 1.45%.
**
**	3) Rearrange the dm_header so the non-transmitted portion
**	is at the front of the header, then change the mbuf code to
**	recognize that the offsets will change. Note: moving the non-
**	transmitted portion to the end of the header will not work since
**	the message data immediately follows.
**
**	4) Double buffer the data and skip the non-transmitted portion
**	in the upper layers.
**
**  4) Software triggers (sw_trigger). This is definately specific to
**	HP-UX. They could simply be redefined to the target function.
**
**  5) SPL Levels. The HP-UX LAN card interrupts at H/W level 5. If
**	another card runs at a higher level, then there will be problems
**	in the critical sections and the mbuf code. Beware.
**
**  6) In the interest of speed, the protocol often uses CRIT/CRIT5/UNCRIT
**	to protect critical regions of code. If these are not avaliable,
**	then CRIT -> spl6(), CRIT5 -> spl5(), and UNCRIT -> splx(s).
**
**  7) Configurability. The method is specific to HP-UX. But the
**	configuration utility was derived from AT&T System V. This
**	code is very isolated and can be ported fairly easily.
**
**  8) Configurable parameters. Should port easily.
**
**  9) LAN Break detection. This is definately tied to the HP S300
**	LAN architecture and H/W Card. Perhaps only the upper-half
**	of the algorithm is sufficient.
**
**  10) Timeouts. The S300 value of HZ=50, so a tick is 20ms. The
**	S800 value of HZ=100, so the tick is 10ms. This should be
**	taken into consideration for retries, etc... Special attention
**	should begiven to assure that the recovery/selftest/protocol
**	all use the same values. Also notice the "#undef timeout" at
**	the front of this and most other files that use timeouts.
**
**  11) Monitor(1M) makes use of the proto_stats statistic structures.
**
**  12) Others???
**
**
**			   NETWORK BUFFER MANAGEMENT
**
**  1) In the interest of speed, the mbuf/cbuf code often uses
**	 CRIT/CRIT5/UNCRIT to protect critical regions of code. If these
**	 are not avaliable, then CRIT -> spl6(), CRIT5 -> spl5(),
**	 and UNCRIT -> splx(s).
**
**  2) The dux mbuf/cbuf code is based on BSD's 4.2 mbuf strategy. However,
**	the actual implementation has changed. I would highly recommend
**	using this implementation rather than the normal Berkeley mbuf code.
**	The HP version has the WAIT/DONTWAIT flag implemented and several
**	routines depend upon receiving a valid mbuf pointer if they state
**	they are willing to wait. Those DUX routines that say DONTWAIT all
**	check the returned pointer for NULL. Additionally, it has its
**	own map for page table entries and is completly configurable.
**
**  3) The dux mbuf code is based on the 4.2 VM scheme.
**
**  4) The size of the cbuf is 1kb and not the more familar 4kb.
**
**  5) The mbuf code assumes the pagesize is 4kb.
**
**  6)	Monitor(1M) makes use of the mbstat statistic structures.
**
*/


/*
 *			NET_REQUEST
 *
 * net_request() is called by DUX message interface to send a DUX message.
 * DUX message interface filled the DUX message headers in the mbuf.
 * This routine fills in the DUX protocol header in the mbuf.
 */

net_request(req_mbuf, rep_mbuf)
register dm_message req_mbuf;	/* points to request mbuf	*/
register dm_message rep_mbuf;	/* points to reply mbuf */
{
    register u_long rid;	/* request id, unique in a site */
    register int index;
    register struct using_entry *using_entry;
    register struct dm_header *dmp = DM_HEADER(req_mbuf);
    register struct proto_header *ph = &(dmp->dm_ph);
    int s;
    site_t dest_site,		/*destination site			  */
	first_dest,		/*first destination in a multisite request*/
	site;
    int first_dest_found, dest_count;
    struct dm_multisite *dest_list;
    struct dm_site *p_dm_site;
    extern short rootlink[];
    int counted = 0;		/* Flag true if this request should be counted
				 * in the protocol window.   Only single site
				 * requests are counted in the window
				 */

    /*
    ** Set the proto header flags to indicate this is a request
    ** and pull out the destination site id.
    */
    ph->p_flags = P_REQUEST;
    dest_site = dmp->dm_dest;

    /*
     * Default to single lan card.  Most common case
     */
    dmp->dm_lan_card = 0;

    /*
    ** initialize destination address
    */
    switch (dest_site)
    {
	/*
	** A DM_DIRECTEDCAST is only used during booting.
	*/
case DM_DIRECTEDCAST:
	DUX_COPY_ADDRESS(rootlink, ph->iheader.destaddr);
	break;

case DM_DUXCAST:
	DUX_COPY_ADDRESS(duxcast_addr, ph->iheader.destaddr);
	break;

case DM_MULTISITE:
case DM_CLUSTERCAST:
	/*
	** If this is a MULTISITE message :
	**		then it goes to each site serially. In this case the
	**		dest_list was created by the requesting function by
	**		calling createsitelist() prior to calling the dm layer.
	**
	** If this is a CLUSTERCAST message:
	**		 then it goes to all sites at the same time
	**		 via the cards multisite capability. In this case
	**		 it is the responsibility of the dm layer to build
	**		 the dest_list thereby freeing the upper level from
	**		 the burden.
	*/

	/*
	** The destination list is held by the reply mbuf in the buf structure.
	*/
	dest_list = ((struct dm_multisite *)((DM_BUF(rep_mbuf))->b_un.b_addr));

	dest_count = 0;
	first_dest_found = 0;

	dest_list->dm_sites[my_site].site_is_valid = FALSE; /*exclude itself*/

	/*
	** Need to initialize a few fields of site 0's dest_list
	*/
	dest_list->dm_sites[0].rc = 0;
	dest_list->dm_sites[0].acflag = 0;
	dest_list->dm_sites[0].tflags = 0;
	dest_list->dm_sites[0].eosys = EOSYS_NORMAL;

	/*
	** Scan the clustab table to find all destinations that are associated
	** with this cluster. If the dest_list has a site marked valid then
	** make sure that this corresponds with the clustab[] entry. If there
	** is an inconsistency then the clustab[] is the master.
	*/
	for (site = 1; site < MAXSITE; site++)
	{
	    p_dm_site = &(dest_list->dm_sites[site]);

	    if (p_dm_site->site_is_valid)
	    {
		/*
		** First a consistency check to make sure the node is
		** a valid cluster member.
		*/
		if ((clustab[site].status & CCT_STATUS_MASK) == CL_IS_MEMBER)
		{
		    /*
		    ** Assume that the message will not be deliverable up front
		    */
		    p_dm_site->rc = DM_CANNOT_DELIVER;
		    p_dm_site->status = DM_MULTI_NORESP;

		    /*
		    ** Increment the counter for sites sending to.
		    */
		    dest_count++;

		    if (!first_dest_found)
		    {
			/*
			** This site will be the the 1st one we try to talk to.
			*/
			first_dest = site;
			first_dest_found = TRUE;
		    }
		}
		else  /*destination is either dead or not a cluster member*/
		{
		    p_dm_site->site_is_valid = FALSE;
		}
	    }
	}

	/*
	** The case of no one to send to: this is a valid case.
	*/
	if (dest_count == 0)
	{
	    /*
	    ** Since we are NOT under interrupt it is OK to call
	    ** dm_recv_reply() directly.
	    */
	    protocol_cannot_deliver(rep_mbuf, 0);
	    dm_recv_reply(req_mbuf, rep_mbuf);
	    return (0);
	}

	/*
	** Regardless of whether we have a multisite or clustercast we
	** mark the flags as multicast.
	*/
	ph->p_flags |= P_MULTICAST;


	/*
	** Need to initialize the proto_headers destination link address.
	*/
	if (dest_site == DM_MULTISITE)
	{
	    /*
	    ** Destination address is the 1st node address.
	    ** On first glance it seems crazy that multisites are sent
	    ** out serially rather than as a clustercast. However, they
	    ** are based on the premise that we may only want to sent to a
	    ** subset of the cluster.
	    */
	    DUX_ASSIGN_ADDRESS(first_dest, ph, dmp);
	}
	else
	{
	    /*
	    ** Destination address is a clustercast address.
	    */
	    DUX_COPY_ADDRESS(clustcast_addr, ph->iheader.destaddr);
	}

	/*
	** Number of sites that have not sent back a reply
	*/
	dest_list->remain_sites = dest_count;

	/*
	** Number of sites that have not sent any response back.
	*/
	dest_list->no_response_sites = dest_count;

	/*
	** Start scanning from here for the next site to sent to.
	*/
	dest_list->next_dest = first_dest + 1;
	break;

default:  /* SINGLE DESTINATION request*/
	if ((dest_site >= 1) && (dest_site < MAXSITE) &&
	    ((clustab[dest_site].status & CCT_STATUS_MASK) == CL_IS_MEMBER))
	{
	    /* destination is up and valid */
	    DUX_ASSIGN_ADDRESS(dest_site, ph, dmp);

	    if ((dmp->dm_flags & DM_URGENT) == 0)
		counted = 1;	/* Flag that we should be considered in the
				 * protocol window for this site.
				 */
	}
	else
	{
	    /*
	    ** Destination is down or invalid.
	    ** Since we are not under interrupt we can call dm_recv_reply
	    ** directly. But first, we need to set up a few fields in the
	    ** reply mbuf.
	    */
	    protocol_cannot_deliver(rep_mbuf, DM_CANNOT_DELIVER);
	    STATS_req_not_member++;
	    dm_recv_reply(req_mbuf, rep_mbuf);
	    return(0);
	}
	break;
    } /* END of switch */

    /*
    ** Generate a unique rid
    */
    s = CRIT();
    rid = ++dux_rid;

    /*
    ** get and initialize using_array[index]
    */
    index = get_using_array();
    using_entry = &using_array[index];
    using_entry->rid = rid;	/* must protect setting of rid */

#ifdef DUX_LOG
    log_pd(PD_NET_REQUEST, index, rid, 0, 0, 0);
#endif
    UNCRIT(s);

    using_entry->req_mbuf = req_mbuf;
    using_entry->rep_mbuf = rep_mbuf;
    using_entry->no_retried = 0;
    using_entry->byte_recved = 0;

/*
#ifdef FILLIN_IEEEHDR
    dux_ieee802_header(ph);
#endif FILLIN_IEEEHDR
*/

    /*
    ** If the upper-level specified that this request/reply
    ** is repeatable, then set the idempotent flag.
    */
    if (dmp->dm_flags & DM_REPEATABLE)
    {
	ph->p_flags |= P_IDEMPOTENT;
    }

    /*
    ** Basic bookkeeping and initializations
    */
    ph->p_rid = rid;
    ph->p_byte_no = 0;
    ph->p_retry_cnt = 0;
    ph->p_dmmsg_length = dmp->dm_headerlen;
    ph->p_data_length = dmp->dm_datalen;
    ph->p_data_offset = 0;
    ph->p_req_index = index;

    /*
    ** Check the protocol window if this request should be "counted" in
    ** protocol windows.  Currently only single destination request are
    ** "window'ed".  The manipulation of the window queue in clustab as
    ** as well as the using array is manipulated at either spl_arrays()
    ** or CRIT_arrays().
    **
    ** If this site's window is full, then hang this request off the end
    ** of the request waiting queue for this site.
    ** If there is room in the window, make the window smaller and send
    ** the request.
    */
    s = CRIT_arrays();

    if (counted)
    {
	if (clustab[dest_site].req_count >= dux_window_size)
	{	/* This packet is outside the protocol window for this site,
		 * put it on the waiting queue */
	    if (clustab[dest_site].req_waiting_Qhead)
	    {
		clustab[dest_site].req_waiting_Qtail->req_waiting_next =
			using_entry;
		clustab[dest_site].req_waiting_Qtail = using_entry;
	    }
	    else
	    {
		clustab[dest_site].req_waiting_Qhead =
			clustab[dest_site].req_waiting_Qtail = using_entry;
	    }

	    using_entry->flags |= U_OUTSIDE_WINDOW;
	    using_entry->req_waiting_next = 0;

	    UNCRIT_arrays(s);
	}
	else
	{
	    clustab[dest_site].req_count++;

	    /* Remember this request is counted in the window for this site. */
	    using_entry->flags |= U_COUNTED;

	    goto send_request;
	}
    }
    else
    {
send_request:
	UNCRIT_arrays(s);

	/*
	** Set up flags in the using entry to indicate that the message
	** is in the queue and call dux_send to actually put the message
	** in the duxQ.
	*/
	STATS_xmit_op_P_REQUEST++;
#ifdef __hp9000s300
	using_entry->flags |= U_IN_DUXQ;
#endif
	dux_send(req_mbuf);
    }
    return(0);
} /* END: net_request */


/*
**			   CHECK_PROTOCOL_WINDOW
**
** check_protocol_window(site)
**
** This routine implements a sliding window in the DUX protocol on a per client
** basis.  The window size is set with the global variable, "dux_window_size".
** This routine is called when a message coming in might open the window for
** a site.  Currently only two kinds of messages are checked for:
**   1. Replys to non-slow requests in recv_reply()
**   2. Slow request pseudo requests in recv_slow_request()
** This routine checks to see if the argument "site" has requests waiting to
** come into the window, it opens the window by one request (represented by
** the message that caused us to be called) and then sends one or more of
** these requests until the window closes.
**/

check_protocol_window(site)
site_t site;
{
    register struct using_entry *using_entry;
    register struct cct *Pcct = &clustab[site];
    register int s = CRIT_arrays();

    if (((Pcct->status & CCT_STATUS_MASK) == CL_IS_MEMBER) && (Pcct->req_count != 0))
    {
	Pcct->req_count--;

	/* If (while) the window is open, then send any waiting messages
	 * until the window is full again */
	while ((Pcct->req_count < dux_window_size) &&
	       ((using_entry = Pcct->req_waiting_Qhead) != 0))
	{	/* Window is open, send the request */
	    if (!(using_entry->flags & U_USED))
		panic("Empty req in protocol window Q");

	    if (!(using_entry->flags & U_OUTSIDE_WINDOW))
		panic("Bad req in protocol window Q");

	    if ((Pcct->req_waiting_Qhead = using_entry->req_waiting_next) == 0)
		Pcct->req_waiting_Qtail = 0;

	    Pcct->req_count++;

	    /* Include this request in the window and count it */
	    using_entry->flags &= ~U_OUTSIDE_WINDOW;
	    using_entry->flags |= U_COUNTED;
	    using_entry->req_waiting_next = 0;

	    /*
	    ** Send the request
	    **
	    ** Set up flags in the using entry to indicate that the
	    ** message is in the queue and call dux_send to actually
	    ** put the message in the duxQ.
	    */
	    STATS_xmit_op_P_REQUEST++;
#ifdef __hp9000s300
	    using_entry->flags |= U_IN_DUXQ;
#endif
	    UNCRIT_arrays(s);	/* restore pri before dux_send() */
	    dux_send(using_entry->req_mbuf);
	    s = CRIT_arrays();	/* change priority back */
	}
    }

    UNCRIT_arrays(s);
}


/*
**		PROTOCOL_CANNOT_DELVER
**
** This function is called when the protocol cannot deliver a message
** (i.e. It never made it out of the requesting node.)
**
*/
protocol_cannot_deliver(rep_mbuf, return_code)
register dm_message rep_mbuf;
register short return_code;
{
    DM_RETURN_CODE (rep_mbuf) = return_code;
    DM_EOSYS(rep_mbuf) = EOSYS_NORMAL;
    DM_ACFLAG(rep_mbuf) = 0;

    /*
    ** DM_TFLAGS (rep_mbuf) are left as is for this case.
    */
}


/*
**		FREE_USING_ENTRY
**
** This function provides a single location where using_array entries
** are freed. If there are processes waiting on an available using_array
** slot, they are sent a wakeup signal.
*/
free_using_entry(using_entry)
register struct using_entry *using_entry;
{
    register int s = spl_arrays();

    if (using_entry->flags & U_OUTSIDE_WINDOW)
    {
	panic("Freeing using entry that's in proto window queue");
    }

    using_entry->flags = 0;	/* free_using_array(index); */
    using_entry->rid = 0;
    using_entry->req_mbuf = 0;
    using_entry->rep_mbuf = 0;
    using_entry->no_retried = 0;
    using_entry->byte_recved = 0;
    using_entry->req_waiting_next = 0;

    /*
    ** Wakeup any sleepers.
    */
    if (waiting_for_using_array)
    {
	waiting_for_using_array = 0;
	wakeup(&(using_array[0]));
    }

    splx(s);
}


/*
**			GET_USING_ARRAY
**
** get_using_array() is called by net_request() to
** find a free entry in using_array[].
*/
get_using_array()
{
    register int i;
    register int stats_already_bumped = 0;
    register struct using_entry *using_entry;

loop:
    using_entry = using_array;

    for (i = 0;	 i < using_array_size; i++, using_entry++)
	if (using_entry->flags	== 0)
	{
	    using_entry->flags |= U_USED;
	    return(i);
	}

    /*
    ** no free entry, set the global flag and go to sleep.
    */
    waiting_for_using_array = 1;

    if (stats_already_bumped == 0)
    {
	STATS_waiting_using++;
	stats_already_bumped++;
    }

    sleep(&(using_array[0]),PZERO);
    goto loop;
} /* END: get_using_array */


/*
**			DUX_RECV_ROUTINES
**
** dux_recv_routines() is called from the driver and runs under
** interrupt. Its purpose is to look at the proto-header flags
** and determine which receiving side protocol function to call
** in order to get things going.
*/

dux_recv_routines(hwvars)
register hw_gloptr hwvars;
{
    register struct dm_header *p_dm_header =
	    (struct dm_header *) READ_ADDR(hwvars->rxring);
    register u_int dux_flags = p_dm_header->dm_ph.p_flags;
    register site_t src_site = p_dm_header->dm_ph.p_srcsite;
#if defined(KERNEL_DEBUG_ONLY) && defined (__hp9000s300)
    register int debugger_action = p_dm_header->dm_ph.p_rid;
#endif

    /* panicstr is used to indicate if a panic has
     * occured. If so, then we are dumping and do
     * not want to be interrupted by dux messages
     * (such as a suicide message from the server
     * which will cause a panic and invalidate the
     * dump we are really interested in) -- Mike B.
     * oops-- the same time window is there during
     * reboot. After we broadcast_failure we may
     * retry a request. Since we have told the server
     * we failed, we get a suicide message back.
     * the failure_sent flag is set when we
     * do a broadcast_failure and is used here to
     * ingore any incoming messages.
     */
    if (panicstr || failure_sent)
    {
	return;
    }

    /*
    ** As much as I hate to waste time doing this sort of thing it
    ** does seem like a minimal precaution to take. This same check was
    ** performed in the transmit code, but since we don't have any sort
    ** of H/W -or- S/W data-integrity checking, it seems to be worth
    ** the effort.
    */

    /*
    ** 1st Check if the src_site is within the range of valid sites.
    ** If src_site == 0, this could be the special case of a site
    ** attempting to cluster (doesn't know what its site_id is yet!).
    */
    if (src_site > MAXSITE)
    {
	STATS_recv_bad_srcsite++;	/* Not displayed by monitor */
	goto bail_out;
    }

    /*
    ** Now the case of where the server has already declared a site dead
    ** but for some reason the site starts sending requests/replies. We
    ** will dump the frame and hopefully the client will soon declare
    ** itself dead since it is no longer receiving root server alive
    ** messages.
    */
    else if ((src_site > 0) &&
	     ((clustab[src_site].status & CCT_STATUS_MASK) != CL_IS_MEMBER))
    {
	/*
	** The case where the client is booting. Remember... when the
	** client is booting its siteid == 0 up until the time it
	** receives a response from the ws_cluster request. So, we have
	** to check my_site, else you cannot boot diskless nodes....
	*/
	if (my_site != 0)
	{
	    /*
	    ** 1st log stats
	    */
	    STATS_recv_not_member++;

	    /*
	    ** 2nd send a commit-suicide msg to this site.
	    */
	    if (IM_SERVER)
	    {
		send_intrpt_dgram(src_site, P_SUICIDE);
	    }
	    goto bail_out;
	}
    }

    /*
    ** On S800, the hardware will receive its' own multicast messages
    ** We need to filter them out for DUX multicast and clustercast
    */
    if (src_site == my_site) {
    goto bail_out;
    }

    /*
    ** Set the clustab status flag to ACTIVE, this way the upper-level
    ** sanity checking, (i.e. reception of I-am-alive dgrams) is
    ** supplemented by the knowledge that we have received messages of some
    ** sort from this particular cnode. This is a method of acounting for
    ** received msgs from a particular cnode, even though the I-am-alive
    ** msg may have been lost.
    */
    if (src_site > 0)
    {
	clustab[src_site].status = CL_ACTIVE;
    }

    /*
     * Check the forward bit to see if this is a message from a cnode to
     * another cnode on a different subnet.
     */

    if (dux_flags & P_FORWARD) {
#ifdef __hp9000s300
	/* Make sure we dump this. */
	goto bail_out;
#else
	STATS_recv_op_P_FORWARD++;
	if (!forward_msg(hwvars)) goto bail_out;
#endif
    }
    else if (dux_flags & P_REQUEST)
    {
	/*
	** If this site isn't clustered then dump the frame.
	** we can check this here and maybe save a function call.
	*/
	if (!(my_site_status & CCT_CLUSTERED))
	{
	    STATS_not_clustered++;
	    goto bail_out;
	}
	else
	{
	    STATS_recv_op_P_REQUEST++;
	    recv_request(hwvars);
	}
    }
    else if (dux_flags & P_REPLY)
    {
	STATS_recv_op_P_REPLY++;
	recv_reply(hwvars);
    }
    else if (dux_flags & P_ACK)
    {
	STATS_recv_op_P_ACK++;
	recv_ack(hwvars);
    }
    else if (dux_flags & P_NAK)
    {
	STATS_recv_op_P_NAK++;
	recv_nak_msg(src_site);
	goto bail_out;
    }
    else if (dux_flags & P_SLOW_REQUEST)
    {
	STATS_recv_op_P_SLOW_REQUEST++;
	recv_slow_request(hwvars);
    }
    else if (dux_flags & P_DATAGRAM)
    {
	STATS_recv_op_P_DATAGRAM++;
	recv_datagram(hwvars);
    }
    else if (dux_flags & P_SUICIDE)
    {
	recv_suicide_msg();
    }
    else if (dux_flags & P_ALIVE_MSG)
    {
	/*
	** only send reply alive message if we are clustered
	*/
	if (my_site_status & CCT_CLUSTERED)
	    send_intrpt_dgram(src_site, P_DATAGRAM);

	goto bail_out;
    }
    else
    {
	/*
	** Could be a special message that puts the
	** cnode into the debugger remotely.
	*/
#ifdef KERNEL_DEBUG_ONLY
	if (dux_flags & P_DEBUGGER)
	{
	    invoke_debugger_action(debugger_action);
	}
	else
	{
	    STATS_recv_bad_flags++;
	}
#else
	STATS_recv_bad_flags++;
#endif KERNEL_DEBUG_ONLY

bail_out:
	skip_frame(hwvars);
    }
}


/*
 *			RECV_REQUEST
 *
 * recv_request is called when a  request packet is  received.
 * It runs under interrupt and is called from dux_recv_routines.
 */
#define	 XMIT_OFFSET_MINUS_HDRLEN  XMIT_OFFSET-DM_HEADERLENGTH

recv_request(hwvars)
register hw_gloptr hwvars;
{
    register struct dm_header *dmp =
	    (struct dm_header *) READ_ADDR(hwvars->rxring);
    register struct proto_header *ph = &(dmp->dm_ph);
    register dm_message	 dmmbuf;	/* ptr to mbuf holding remote request */
    register int dmmsg_size;	/* dmmsg length */
    register struct serving_entry *serving_entry;
    struct buf *bufp;		/* buf ptr for request data */
    short index,		/* using/serving array indices */
	length;			/* length of useful bytes recved */
    int s;			/* for spl's */
    int send_nak = FALSE;	/* negative ack (NAK) for flow control */

    /*
    ** Check to see if this packet has a valid sequence number
    */
    if (badseq(ph))
	goto drop_packet;

    /*
    ** 1st determine if we are already serving this request, or if this
    ** is a new request. If the proto-headers rid matches up with one
    ** of this sites serving_array entries then we are already serving the
    ** the request.
    */

    /*
    ** If the proto hdr byte number is 0, normaly this would mean
    ** it is a new msg and we would want to allocate resources.
    ** However, it could be a retry and the resources are already
    ** allocated; hence we need to additionally look at the retry_cnt.
    */
    if ((ph->p_byte_no == 0) && (ph->p_retry_cnt == 0))
    {
	index = -1;  /* new message */
    }
    else
    {
	s = CRIT();
	index = locate_serving_array(ph->iheader.sourceaddr,
			     ph->p_rid, ph->p_srcsite);
	UNCRIT(s);
    }


   if (index >= 0)
   {
	serving_entry = &serving_array[index];
#ifdef DUX_PROTOCOL_DEBUG
	if (net_debug > 3)
	{
	    msg_printf("recv_request: p_byte_no=%x,byte_recved=%x,rid=%x\n",
		       ph->p_byte_no, serving_entry->byte_recved, ph->p_rid);
	    msg_printf("serving_entry->state = %x\n", serving_entry->state);
	}
#endif DUX_PROTOCOL_DEBUG

	/*
	** Now lets examine the current state of the request and from
	** here we can determine what actually needs to be done.
	*/
	switch (serving_entry->state)
	{
    case RECVING_REQUEST:
	    /*
	    ** Is this an expected packet? If yes, then then the protocol
	    ** headers byte number should equal the serving entries byte
	    ** received.
	    */
	    if (ph->p_byte_no == serving_entry->byte_recved)
	    {
		/*
		** This packet contains new bytes of this message
		*/
		dmmbuf = serving_entry->msg_mbuf;
		dmp = DM_HEADER(dmmbuf);

		/*
		** Read protocol header and actually copy in the bytes
		*/
		DUX_COPY_PROTO(ph, &(dmp->dm_ph));

		ph = &(dmp->dm_ph);
		length = dux_copyin(hwvars, dmp);

		serving_entry->byte_recved += length;

		serving_entry->time_stamp = lbolt;

		goto check_end_of_message;
	    }
	    else				/* expected packet*/
	    {
		/*
		** This was an UNexpected packet, dump it.
		** Actually this is an out-of-sequence pkt. We don't
		** want to sent a NAK here. Simply let the requestors
		** retry code resend the entire msg. We will only
		** gather the missing portion of the message.
		**
		*/
#ifdef DUX_PROTOCOL_DEBUG
		if (net_debug > 3)
		{
		    msg_printf("recv_request: UNexpected packet!\n");
		    msg_printf("recv_request:p_byte_no=%x,byte_recved=%x,rid=%x\n",
			       ph->p_byte_no, serving_entry->byte_recved,
			       ph->p_rid);
		}
#endif DUX_PROTOCOL_DEBUG

		STATS_recv_req_OOS++;
		goto drop_packet;
	    }

    case SERVING:
	    /*
	    ** We are already serving this request so it must be
	    ** be a duplicate request. The serving side will reply by:
	    **		1) discarding this dup request and
	    **		2) sending the requesting site a slow_request pkt,
	    **		   to prevent it from sending more dup requests.
	    */
#ifdef DUX_PROTOCOL_DEBUG
	    if (net_debug > 3)
	    {
		msg_printf("recv_request: SERVING, rid= 0x%x\n" ,ph->p_rid);
	    }
#endif DUX_PROTOCOL_DEBUG
	    net_slow_request(index);
	    goto drop_packet;

    case SENDING_REPLY:
	    /*
	    ** We have already served this request and are in the process
	    ** of sending a reply. This is a duplicate request just dump it.
	    */
	    STATS_dup_req++;
	    goto drop_packet;

    case WAITING_ACK:
    case ACK_RECEIVED:
	    /* non-idempotent case only. Possible causes:
	    ** 1) the reply message has not arrived before the using
	    **	  site sends this duplicate request,
	    ** 2) the reply message was lost,
	    **
	    **	In either case, this can be treated like SENDING_REPLY
	    **	by simply dropping the packet. It doesn't make any sense
	    **	to reset the p_byte_no to zero, untimeout any outstanding
	    **	reply retry, and put the request back into the duxQ for
	    **	retransmission. All of that work will be done by the reply
	    **	retry code. The retry reply takes place every 1/2 sec anyway.
	    */

	    STATS_dup_req++;
	    goto drop_packet;

    case NOT_ACTIVE:
    default:
	    panic("recv_request: invalid serving_array state!");
	    break;
	}  /* END:  switch */
    }
    else
    {
	/*
	** NEW REQUEST!
	** (req_address, rid) is not in serving_array yet
	**
	** NOTE:
	**
	** If this is truely a new request, then the proto-headers byte_no
	** should equal zero; if not, then we simply cannot "assume" that
	** the LAN cards receive buffers are all busy (overflow). We could
	** implement an elaborate scheme here to try and send a quick message
	** to the requestor to restart this msg. But by the time it got there
	** we probably already have the entire msg on this machines LAN card.
	** We do not want to sent a NAK here since it would only aggravate the
	** situation,  but rather allow the requestors retry code to re-send
	** the request. This makes a distinction between dropping a frame due
	** to lack of local resources -vs- an out of sequence msg...
	*/
	if (ph->p_byte_no != 0)
	{
	    STATS_lost_first++;
	    goto drop_packet;
	}

	/*
	** Ok, we now assume that this is really a  NEW REQUEST!
	**
	** 1st we have to go get an mbuf/cbuf to hold the basic info. Since
	** dm_alloc() adds DM_HEADERLENGTH to the length, we have to
	** request sufficient space for message minus the DM_HEADERLENGTH.
	**
	** Since we are still under interrupt we certainly cannot wait for
	** for the mbuf/cbuf. If one is not available, then we only have a
	** couple of choices:
	**
	**	1: dump the pkt: The sending sides retry_request timer will
	**			 expire and re-send the request.
	**
	**	2: send a NAK:	 Send a NAK to slow down the requestor.
	**			  (This is what we will do!) The 6.0 diskless
	**			   release staticly allocates all of its
	**			   available pool of mbufs up front. We cannot
	**			   go out and get more! Hopefully, slowing
	**			   down the requestor will allow some replies
	**			   to go out and free up some of the mbufs.
	*/

	dmmsg_size = ph->p_dmmsg_length + XMIT_OFFSET_MINUS_HDRLEN;

	dmmbuf = dm_alloc(dmmsg_size, DONTWAIT);

	if (dmmbuf == NULL)
	{
	    /*
	    ** Gather stats.
	    */
	    if (dmmsg_size > MLEN)
		STATS_recv_no_cluster++;
	    else
		STATS_recv_no_mbuf++;

	    send_nak = TRUE;
	    goto drop_packet;
	}

	/*
	** 2nd thing to do: is determine if we need a file system buffer,
	** we now look at the data portion of the message. If none are
	** available from the DUX pre_allocated dskless_fsbuf pool then we have
	** to release the already obtained mbuf/cbuf and send back a
	** NAK to slow down the requestor.
	*/
	if (ph->p_data_length == 0)
	{
	    bufp = NULL;
	}
	else
	{
	    bufp = syncgeteblk(ph->p_data_length);

	    if ((bufp  == NULL) || (bufp == ((struct buf *)-1)))
	    {
		dm_release(dmmbuf, FALSE);
		send_nak = TRUE;
		if (bufp == NULL)
		{
		    STATS_recv_no_buf++;
		}
		else
		{
		    STATS_recv_no_buf_hdr++;
		}
		goto drop_packet;
	    }
	}

	/*
	** 3rd thing to do: is go get a new serving array entry.
	** If none are available then we have to release the already
	** allocated mbuf, possibly release the allocated buf, and again
	** send back a NAK to slow down the requestor.
	*/
	s = CRIT_arrays();
	if ((index = get_serving_array()) < 0)
	{
	    /* MUST drop priority prior to calling dm_release */
	    UNCRIT_arrays(s);
	    dm_release(dmmbuf, 0);

	    if (bufp != NULL)
	    {
		brelse(bufp);
	    }

	    /*
	    ** Short of serving array entry. This should rarely happen.
	    ** DSDe403814
	    ** Do not do a NAK. This can cause us to block because
	    ** the NAK will stop the client from sending ANYTHING for
	    ** a while and therefore we won't complete any requests
	    ** and free up a serving entry. So... just drop this one.
	    ** send_nak = TRUE;
	    **/
	    goto drop_packet;
	}

	/*
	** 4th, get a pointer to the dm_transmit part
	*/

	dmp = DM_HEADER(dmmbuf);

	/*
	** 5th, Read the protocol header from the LAN card
	** We have to read in the proto header and quickly set the
	** serving_entry->req_addr and serving_entry->rid fields to prevent
	** the function locate_serving_array() from possibly trying to find
	** this serving entry on a request retry.
	*/
	DUX_COPY_PROTO(ph, &(dmp->dm_ph));
	ph = &(dmp->dm_ph);
	dmp->dm_bufp = bufp;
	dmp->dm_mid = index; /* this will be passed back in net_reply */

	/*
	** 6th, start initializing serving_array[index] for this request.
	** and drop priority.
	*/
	serving_entry = &serving_array[index];
	DUX_COPY_ADDRESS(ph->iheader.sourceaddr, serving_entry->req_address);
	serving_entry->req_site = ph->p_srcsite;
	if (ph->p_srcsite == 0) {
		/* save lan cards this came in on for case where client
		 * is not yet in clustab. cwb
		 */
		serving_entry->lan_card = get_lan_card(ph->iheader.destaddr);
	}
	serving_entry->rid = ph->p_rid;
	serving_entry->state = RECVING_REQUEST;
	serving_entry->msg_mbuf = dmmbuf;
	serving_entry->no_retried = 0;
	serving_entry->time_stamp = lbolt;

	UNCRIT_arrays(s);

	/*
	** 7th, We can read in the rest of the data at this lower priority
	** and finish initializing the serving_array[index].
	*/
	length = dux_copyin(hwvars, dmp);

	if (ph->p_flags & P_IDEMPOTENT)
	    serving_entry->flags |= S_IDEMPOTENT;

	if (ph->p_flags & P_MULTICAST)
	     serving_entry->flags |= S_MULTICAST;

	serving_entry->req_index = ph->p_req_index;

	s = CRIT();
	serving_entry->byte_recved = length;
	UNCRIT(s);

	serving_entry->no_retried = 0;
    } /* else, NEW REQUEST */

check_end_of_message:
    /*
    ** If this is the end of message then set the serving state to
    ** SERVING and call dm_recv_request to move the operations upstairs.
    */
    if (ph->p_flags & P_END_OF_MSG)
    {
	s = CRIT();
	serving_entry->state = SERVING;
	serv_recv_req_slot[ph->p_srcsite] = -1;
	UNCRIT(s);

	/*
	 ** The following is a consistency check
	 */
	if (serving_entry->byte_recved < ph->p_dmmsg_length+ph->p_data_length)
	{
	    panic("recv_request: premature end of packet!");
	}
	dm_recv_request(dmmbuf);
    }
    return(0);

drop_packet:
    /*
    ** Release this packet.  We must do this before sending the NAK (if
    ** any) because the NAK we send may be a broadcast packet.  If we
    ** send a broadcast packet, we will re-enter this code and will have
    ** lost the original value of hwvars, causing us to free an mbuf
    ** twice ("panic: freeing free mbuf").
    **
    ** By freeing the first mbuf here, we avoid this entire problem.
    */
    skip_frame(hwvars);

    /*
    ** If we are going to send a NAK to slow down the requestor
    ** then call send_nak_msg, else dump the frame.
    */
    if (send_nak)
    {
	/*
	** If this is the rootserver then clustercast
	** else send it to the specific cnode.
	*/
	if (IM_SERVER)
	{
	    send_intrpt_dgram(DM_CLUSTERCAST, P_NAK);
	}
	else
	{
	    send_intrpt_dgram(ph->p_srcsite, P_NAK);
	}
    }

    return(1);
} /* END: recv_request */


/*
**			LOCATE_SERVING_ARRAY
**
** Try to find a serving_array[] entry with both the
** req_address[] and rid identical to addr and rid.
**
** NOTE: To tune this routine is difficult for several reasons:
**
**	1) The size of the serving array can vary between 16 - 100
**	   dependent upon whether it is on a diskless cnode or a server.
**	   As a result hashing functions don't disperse nicely for the
**	   small (16) and large (100) serving entries. You end up with
**	   lots of multiple hits and having to maintain long chains at
**	   each hash point... Locally mounted discs, where any cnode
**	   can be a server compound the problem.
**	2) The design is tuned for quick lookups once the message is
**	   received. The request & reply indices are actually passed
**	   as info in the protocol header. To change this is expensive;
**	   this eliminates ideas of maintaining an array[MAXSITE] that
**	   can be quickly indexed and then a chain for each cnode. The
**	   problem is how the chains are manipulated, the indexes passed
**	   are meaningless, also the chains must be a unique datum, i.e.
**	   no pointers to a location allowed cause they may be re-arranged.
**
** Solution: maintain an array[MAXSITE] which holds the index into the
**	     serving_array for a particular cnode's last message that is
**	     still in the state of RECVING_REQUEST.
**
**    Index by cnode_id				     Serving Array
**
**     0   _____				     ______  0
**	  |	|				    |	   |
**	  ~	~				    ~	   ~
**	  |	|				    |	   |
**	   _____|				    |______|
**	  |  X	|----------> X is an index -------->|	   |
**    255  _____	     to a serving_entry	     ______  [15 thru 99]
**
**
**	Now once we get the 1st pkt of a multi-pkt message, we maintain
**	this cnodes index into the serving array for the duration of
**	the time this message is in a state = RECVING_REQUEST. This is
**	a win of up to 5 out of 6 pkts.
*/


locate_serving_array(addr, rid, cnodeID)
register u_char *addr;
register u_long rid;
register u_char cnodeID;
{
    register int i;
    register struct serving_entry *serving_entry;

    /*
    ** First examine the single-deep outstanding request
    ** table to see if it matches (i.e. new requests that are still
    ** in the state of RECVING_REQUEST). In most multi-pkt messages
    ** it will match up since pkts are xmitted back-to-back. If no match
    ** then revert to a linear	search of the serving_array.
    */
    i = serv_recv_req_slot[cnodeID];

    if (i >= 0)
    {
	serving_entry = &serving_array[i];

	if ((serving_entry->flags & S_USED) && (serving_entry->rid == rid) &&
	    eq_address(serving_entry->req_address, addr))
	    return(i);
    }

    serving_entry = serving_array;

    for (i = 0; i < serving_array_size; i++, serving_entry++)
	if ((serving_entry->flags & S_USED) && (serving_entry->rid == rid) &&
	    eq_address(serving_entry->req_address, addr))
	{
	    serv_recv_req_slot[cnodeID] = i;
	    return(i);
	}

    return(-1); /* Couldn't locate; return an error */
} /* locate_serving_array */


/*
**			EQ_ADDRESS
**
** eq_address() compares if two address[6] are identical.
** 1 means true ,0 means false.
**
** NOTE: change it to macro later
*/
eq_address(a1, a2)
register u_char *a1, *a2;
{
    register int i;

    for (i = 0; i < ADDRESS_SIZE; i++)
	if (*(a1++) != *(a2++))
	    return(0);

    return(1);
} /* END:  eq_address */

/*
**			REMOVE_OLD_SERVING_ENTRIES
**
** search for OLD (> 10 minutes) IDEMPOTENT requests in the
** RECVING_REQUEST state and clean them up.
**
** Calls to the routine MUST be at spl_ARRAYS!!
**
** Usually, the serving entry will be released as soon as the
** reply to the request arrives.  However, there is a situation
** that ocurrs occasionally when a requesting side retries at the
** same time as the serving side replies.  They cross on the
** network (where "network" is defined to include the input and
** output queues).  The serving side cleans up after it replies to
** idempotent requests, so it sees the retry as a new request.
** The client side cleans up after seeing the reply, so if one of
** the packets making up the retry gets dropped, the serving entry
** ends up in this state (i.e. RECVING_REQUEST but very old).
**
** What we do here is to assume that a serving entry that is in
** such a state is free-able and we free it.
**
** Returns the index of last serving entry it freed or -1 if it freed none.
*/
int remove_old_serving_entries()
{
    register int i, j;
    register struct serving_entry *serving_entry;

    j = -1;
    for (i = 0, serving_entry = serving_array;
	 i < serving_array_size; i++, serving_entry++)
    {
	if (((serving_entry->flags & (S_USED|S_IDEMPOTENT)) ==
	    				      (S_USED|S_IDEMPOTENT)) &&
	    (serving_entry->state == RECVING_REQUEST) &&
	    ((lbolt - serving_entry->time_stamp) > (10*60*HZ)))
	{
	    STATS_old_serving_entry++;
	    clean_s_entry(serving_entry);
	    j = i;	/* return this one, its free now */
	}
    }

    return(j);
}

/*
**			GET_SERVING_ARRAY
**
** get_serving_array() is called by recv_request()
** to find a free entry in the serving_array[].
** The call to this function is spl protected.
*/
get_serving_array()
{
    register int i;
    register struct serving_entry *serving_entry;

    for (i = 0, serving_entry = serving_array;
	 i < serving_array_size; i++, serving_entry++)
    {
	if (serving_entry->flags == 0)
	{
	    serving_entry->flags |= S_USED;
	    return(i);
	}
    }

    i = remove_old_serving_entries();

    if (i != -1)
    {
	serving_array[i].flags |= S_USED;
	return(i);
    }

    /*
    ** No free entry
    */
    STATS_serving_entry++;
    return(-1);
} /* END: get_serving_array */


/*
**			NET_REPLY
**
** After a server process finishes its task for a request, it calls the dm
** to send the reply. Dm then calls net_reply().
*/

net_reply(rep_mbuf)
register dm_message rep_mbuf; /* points to the reply mbuf */
{
    register struct dm_header *dmp = DM_HEADER(rep_mbuf);
    register int index = dmp->dm_mid;
    register struct proto_header *ph = &(dmp->dm_ph);
    register struct serving_entry *serving_entry;
    int s;

    s = CRIT();
    serving_entry = &serving_array[index];
    serving_entry->msg_mbuf = rep_mbuf;
    serving_entry->state = SENDING_REPLY;
    UNCRIT(s);

#ifdef DUX_PROTOCOL_DEBUG
    if (net_debug > 0)
    {
	msg_printf("net_reply: rid=%x, rep_mbuf=%x\n",
		   serving_entry->rid, (int)rep_mbuf);
	msg_printf("           serving array index=%x, serving array addr = %x\n",
		   index, serving_entry);
    }
#endif DUX_PROTOCOL_DEBUG

    /* initialize protocol header */
    ph->p_flags = P_REPLY;

    if (serving_entry->req_site == 0) {

	/*
	 * Special case the cluster message.  The site is not in clustab
	 * yet so we have to use value we saved in recv_request.
	 */
        dmp->dm_lan_card = serving_entry->lan_card;
	DUX_COPY_ADDRESS(serving_entry->req_address, ph->iheader.destaddr);
    }
    else {
    	DUX_ASSIGN_ADDRESS(serving_entry->req_site, ph, dmp);
    }

/*
#ifdef FILLIN_IEEEHDR
    dux_ieee802_header(ph);
#endif FILLIN_IEEEHDR
*/

    /*
    ** reply for slow idempotent requests should be treated
    ** the same way as the reply for  non-idempotent requests
    */
    if (serving_entry->flags & S_IDEMPOTENT)
	if (!(serving_entry->flags & S_SLOW_REQUEST))
	    ph->p_flags |= P_IDEMPOTENT;

    if (serving_entry->flags & S_MULTICAST)
    {
	ph->p_flags |= P_MULTICAST;
	ph->p_srcsite = my_site;
    }

    ph->p_rid = serving_entry->rid;
    ph->p_byte_no = 0;
    ph->p_retry_cnt = 0;
    ph->p_dmmsg_length = dmp->dm_headerlen;
    ph->p_data_length = dmp->dm_datalen;
    ph->p_data_offset = 0;
    ph->p_req_index = serving_entry->req_index;
    ph->p_rep_index = index;

    STATS_xmit_op_P_REPLY++;
#ifdef __hp9000s300
    s = CRIT();
    serving_entry->flags |= S_IN_DUXQ;
    UNCRIT(s);
#endif /* s300 */

    dux_send(rep_mbuf);
}  /* END: net_reply */


/*
**			RECV_REPLY
**
** recv_reply is called when a reply packet is received.
** It runs under interrupt
*/
recv_reply(hwvars)
register hw_gloptr hwvars;
{
    register struct dm_header *dmp =
	    (struct dm_header *) READ_ADDR(hwvars->rxring);
    register struct proto_header *ph = &(dmp->dm_ph);
    register struct using_entry *using_entry;
    int index,			/* index in using array for request/reply */
	length,			/* length of bytes in byte_recved */
	s;			/* for spl's */
    dm_message req_mbuf;
    dm_message rep_mbuf;
    site_t reply_site;
    struct dm_multisite *dest_list;
    int reply_done = 0;

    /*
    ** Pull the using array index out of the headers req_index.
    */
    index = ph->p_req_index;
    using_entry = &using_array[index];

#ifdef DUX_PROTOCOL_DEBUG
    if (net_debug > 3)
    {
	msg_printf("recv_reply: ph= %x, rid= %x\n", (int) ph, ph->p_rid);
	msg_printf("     index = %x, using_entry = %x\n",index, (int)using_entry);
    }
#endif DUX_PROTOCOL_DEBUG

    /*
    ** Check if this is a duplicate reply, if yes dump the frame.
    */
    if ((using_entry->flags == 0) || (using_entry->rid != ph->p_rid))
    {
	/*
	** send an ack for replies of non-idempotent requests or slow-requests.
	** The send_ack() func itself will allocate the mbuf for usage.
	** since we are currently working off of the card.
	*/
	if (!(ph->p_flags & P_IDEMPOTENT))
	    send_ack(ph);

	skip_frame(hwvars);
	return(0);
    }

    /*
    ** If a retry is in progress, dump this reply and let the server
    ** send another one, because we probably have the reply to the
    ** request being retried.  If we process this reply, the server
    ** could see the retried request as a new (and thus duplicate)
    ** request.
    */
    if (using_entry->flags & U_RETRY_IN_PROGRESS)
    {
	skip_frame(hwvars);
#ifdef DUX_LOG
	/*
	** A retry of the request is in progress, log this event.
	*/
	u_busy_recv_reply++;
#endif
	return(0);
    }

    using_entry->flags |= U_REPLY_IN_PROGRESS;

#ifdef DUX_LOG
    log_pd(PD_RECV_REPLY, index, ph->p_rid, 0, 0, 0);
#endif

    /*
    ** If this is an expected pkt, then the received proto byte number
    ** will match up with the using_entries byte received.
    */

    if (ph->p_byte_no == using_entry->byte_recved)
    {
	dmp = DM_HEADER(using_entry->rep_mbuf);

	/*
	** read protocol header from LAN card
	*/
	DUX_COPY_PROTO(ph, &(dmp->dm_ph));
	ph = &(dmp->dm_ph);
	length	= dux_copyin(hwvars, dmp);
	using_entry->byte_recved += length;

	if (ph->p_flags & P_END_OF_MSG)
	{
	    if (ph->p_flags & P_MULTICAST)
	    {
		/*
		** IMPORTANT NOTE: By definition the should only be a
		** quick_reply for multisites as well as clustercasts.
		** If data were to come back where would we put it?
		** The dm_multisite dest list only handles dmt_u_rr stuff.
		*/

		/*
		** This is an end-of-msg and a multicast. We have to
		** update the already established destination list
		** with this reply sites dmt_u_rr elements.
		*/
		dest_list = ((struct dm_multisite *)
			     ((DM_BUF(using_entry->rep_mbuf))->b_un.b_addr));
		reply_site = ph->p_srcsite;

		/*
		** save the highest rc in site 0's rc.	(site 0 is reserved)
		*/
		dest_list->dm_sites[reply_site].rc = dmp->dm_rc;
		if (dmp->dm_rc > dest_list->dm_sites[0].rc || dmp->dm_rc == -1)
		{
		    dest_list->dm_sites[0].rc = dmp->dm_rc;
		}

		/*
		** save the highest eosys in site 0's eosys.
		*/
		dest_list->dm_sites[reply_site].eosys  = dmp->dm_eosys;
		if (dmp->dm_eosys > dest_list->dm_sites[0].eosys)
		{
		    dest_list->dm_sites[0].eosys = dmp->dm_eosys;
		}

		/*
		** save the  logical OR of acflag in site 0's acflag.
		*/
		dest_list->dm_sites[reply_site].acflag = dmp->dm_acflag;
		dest_list->dm_sites[0].acflag	      |= dmp->dm_acflag;

		/*
		** save the  logical OR of tflags in site 0's tflags.
		*/
		dest_list->dm_sites[reply_site].tflags = dmp->dm_tflags;
		dest_list->dm_sites[0].tflags	      |= dmp->dm_tflags;

		/*
		** If this was not idempotent, send the ACK.
		*/
		if (!(ph->p_flags & P_IDEMPOTENT))
		{
		    send_ack(ph);
		}

		/*
		** Decrement the response_sites counter and determine if
		** we have received the last reply from all of the sites.
		*/
		if (dest_list->dm_sites[reply_site].status != DM_MULTI_DONE)
		{
		    dest_list->dm_sites[reply_site].status = DM_MULTI_DONE;
		    dest_list->no_response_sites--;
		    reply_done = ((--dest_list->remain_sites) == 0);
		}

		/*
		** Regardless of whether reply_done is true we can
		** still reset the byte received counter in preparation
		** for the next site.
		*/
		using_entry->byte_recved = 0;
	    }		 /* End of multicast */
	    else
	    {
		 reply_done = 1;
	    }

	    if (reply_done)
	    {
		rep_mbuf = using_entry->rep_mbuf;
		req_mbuf = using_entry->req_mbuf;

		if (ph->p_flags & P_MULTICAST)
		{
		    /*
		    ** Since this was a multicast reply, we now
		    ** have to copy a few of site 0's dmt_u_rr
		    ** elements to the reply_mbuf.
		    */
		    DM_RETURN_CODE(rep_mbuf)= dest_list->dm_sites[0].rc;
		    DM_EOSYS(rep_mbuf) = dest_list->dm_sites[0].eosys;
		    DM_TFLAGS(rep_mbuf) = dest_list->dm_sites[0].tflags;
		    DM_ACFLAG(rep_mbuf) = dest_list->dm_sites[0].acflag;
		}

		s = spl_arrays();
#ifdef DUX_LOG
		log_pd(PD_UNTIMEOUT_RETRY, ph->p_req_index, ph->p_rid, 0, 0, 0);
#endif DUX_LOG
		untimeout(retry_request, using_entry->rid);
#ifdef __hp9000s300
		/*
		** If retry_request has put request msg in the duxQ, remove it
		*/
		if (using_entry->flags & U_IN_DUXQ)
		{
		    using_entry->flags &= (~U_IN_DUXQ);
		    remove_from_duxQ(req_mbuf);
		}
#endif /* s300 */
		splx(s);

		/*
		** send ack for reply of single dest non-idempotent request
		*/
		if ((!(ph->p_flags & P_IDEMPOTENT)) &&
		    (!(ph->p_flags & P_MULTICAST)))
		{
		    send_ack(ph);
		}

		/*
		** If this reply is counted in the window for this site,
		** then check if there are req(s) waiting outside the window.
		*/
		if ((using_entry->flags & U_COUNTED) &&
		    ((reply_site = DM_HEADER(req_mbuf)->dm_dest) > 0) &&
		    (reply_site < MAXSITE))
		{
		    /*
		    ** Don't count this request in the window any more and
		    ** check to see if there is something waiting to be sent.
		    */
		    using_entry->flags &= ~U_COUNTED;
		    check_protocol_window(reply_site);
		}
#ifdef __hp9000s300
		free_using_entry(using_entry);
		dm_recv_reply(req_mbuf, rep_mbuf);
#else /* s700, s800 */
		if (!(using_entry->flags & U_REPLY_RCVD))
		    call_dm_recv_reply(using_entry);
#endif /* s300 vs. s700, s800 */
	    } /* if (reply_done) */
	} /* if (end_of_message) */
    }
    else
    {
	/*
	** This was NOT an expected message, dump the frame
	*/
	STATS_unexpected++;
	skip_frame(hwvars);
    }

    /*
    ** Clear the U_REPLY_IN_PROGRESS flag (thus allowing reties to requests)
    ** if the reply is not done yet, if the reply is done, free_using_array()
    **	will clear all flags, including this one.
    */
    if (!reply_done)
	using_entry->flags &= ~U_REPLY_IN_PROGRESS;

    return(0);
} /* END: recv_reply */


#if defined(__hp9000s700) || defined(__hp9000s800)
/*
**			    CALL_DM_RECV_REPLY
**
** Free using entry and call dm_recv_reply.  If there is a buffer
** associated with the request, make sure that there isn't a request in
** the lan output queue before calling dm_recv_reply()
*/
call_dm_recv_reply (using_entry)
register struct using_entry *using_entry;
{
    struct dm_header *dmp;
    register struct buf *bp;
    dm_message req_mbuf, rep_mbuf;
    int s;

    dmp = DM_HEADER(using_entry->req_mbuf);
    bp = dmp->dm_bufp;

    /* if the request has already been scheduled for retransmission
    ** when the reply arrives, wait until the retransmission is
    ** completely done for all packets in the message
    */

    s = spl_arrays();
#ifdef DUX_LOG
/* XXX */
    if (bp && (bp->b_flags2 & B2_DUXCANTFREE) && (using_entry->no_retried == 0))
	panic("call_recv_reply: no retry");
/* XXX */
    {
	struct proto_header *ph = &(dmp->dm_ph);
	log_pd(PD_CALL_DM_RECV_REPLY, ph->p_req_index, ph->p_rid, bp, 
				bp ? (bp->b_flags2 & B2_DUXFLAGS) : 0, 0);
    }
#endif
    if (bp  && (bp->b_flags2 & B2_DUXCANTFREE))
    {
	/* set U_REPLY_RCVD flag to prevent duplicate reply actions
	** if duplicate replies arrive back-to-back.
	*/
	using_entry->flags |= U_REPLY_RCVD;
	splx(s);
	timeout(call_dm_recv_reply, using_entry, 2);
    }
    else
    {
	req_mbuf = using_entry->req_mbuf;
	rep_mbuf = using_entry->rep_mbuf;
	free_using_entry(using_entry);
	splx(s);
	dm_recv_reply(req_mbuf, rep_mbuf);
    }
}
#endif /* s700 || s800 */

/*
**			NET_SLOW_REQUEST
**
** net_slow_request() is called, in the serving site, from recv_request()
** when a dup slow request is received.	 It changes the flags of the
** serving_array entry to S_SLOW_REQUEST and sends a "pseudo reply".
*/

net_slow_request(index)
register short index;
{
    register dm_message dmp;	/* mbuf pointer for the pseudo reply message */
    register struct proto_header *ph;	/* points to the proto header */
    register struct serving_entry *serving_entry;
    register int s;

    /*
    ** Consistency check
    */
    if (index < 0)
    {
	panic("diskless: net_slow_request: Invalid serving_array index.");
    }

    serving_entry = &serving_array[index];

    /*
    ** The state had better still be SERVING else someone stomped on this
    ** serving entry. Consistency Check.
    */
    if (serving_entry->state != SERVING)
    {
	panic("diskless: net_slow_request: Invalid serving_entry state ");
    }

    s = CRIT();
    serving_entry->flags |= S_SLOW_REQUEST;
    UNCRIT(s);

    if (dmp = dm_alloc(DM_EMPTY, DONTWAIT))
    {
	/*
	** dmp is !=  NULL. If we cannot get an mbuf  the using site will send
	** a dup request; since the flags are already set to S_SLOW_REQUEST
	** the serving site will regenerate another pseudo reply.)
	*/
	ph = &((DM_HEADER(dmp))->dm_ph);

	/* compose the packet */
	ph->p_flags = P_SLOW_REQUEST;
	if (serving_entry->flags & S_MULTICAST)
	{
	    ph->p_flags |= P_MULTICAST;
	}

	DUX_ASSIGN_ADDRESS(serving_entry->req_site, ph, dmp);

	/* dm_dest needs to be set for flow control!! */
	DM_HEADER(dmp)->dm_dest = serving_entry->req_site;
/*
#ifdef FILLIN_IEEEHDR
	dux_ieee802_header(ph);
#endif FILLIN_IEEEHDR
*/
	ph->p_rid = serving_entry->rid;
	ph->p_req_index = serving_entry->req_index;
	ph->p_byte_no = ph->p_dmmsg_length = ph->p_data_length = 0;
	ph->p_srcsite = my_site;

	STATS_xmit_op_P_SLOW_REQUEST++;
	dux_send(dmp);
    }
    else
    {
#ifdef __hp9000s300
	/*
	** try to send more packets out in order to release mbuf
	*/
	sw_trigger(&duxQ_intloc, dux_hw_send, 0 , LAN_PROC_LEVEL,
		   LAN_PROC_SUBLEVEL+1);
#endif
	STATS_slow_no_mbuf++;		/* Not displayed by monitor */
	STATS_recv_no_mbuf++;
    }
} /* END: net_slow_request */


#define ACK_IHEADER_LEN		46	/* (LAN_MIN_LEN - 14) */
/*
**			INIT_STATIC_ACK
**
** One time initialization of the static mbuf used for
** ack's of all messages. The intent is to save as much
** time as possible in sending acks, so we will populate
** the generic fields once and only once. The other dynamic
** fields will be filled in the actual send_ack() routine.
*/
init_static_ack()
{
    register struct mbuf *reqp;
    register struct dm_header *dmp;
    register struct proto_header *ack_ph;	/* points to the ack	*/
    register int err;

    err = init_static_mbuf(DM_EMPTY, &ack_mbuf);
    if (err == -1)
    {
	panic("diskless: send_ack: can't init dskless_mbuf\n");
    }

    reqp = &ack_mbuf;
    dmp	 = DM_HEADER(reqp);
    ack_ph = &(dmp->dm_ph);

    dux_ieee802_header(ack_ph);

    ack_ph->p_flags = P_ACK;
    ack_ph->p_byte_no = 0;
    ack_ph->p_dmmsg_length = 0;
    ack_ph->p_data_length = 0;
    ack_ph->iheader.length = ACK_IHEADER_LEN;
}


/*
**			INIT_STATIC_MISC_BUFS
**
** One time initialization of the misc static mbufs.
** The intent is to save as much ** time as possible in
** sending (typically) datagrams, so we will populate
** the generic fields once and only once.
*/
init_static_misc_bufs()
{
    register int err;

    err = init_static_mbuf(DM_EMPTY, &nak_mbuf);
    err = init_static_mbuf(DM_EMPTY, &dgram_mbuf);
    err = init_static_mbuf(DM_EMPTY, &suicide_mbuf);
    err = init_static_mbuf(DM_EMPTY, &lan_break_mbuf);

    if (err != 0)
    {
	panic("init_static_misc_bufs: Cannot initialize!");
    }
}


/*
**			   INITIALIZE_DUX_PROTOCOL
**
** called by root_cluster() on the root server and ws_cluster() on a
** client.  This routine is called once to initialize various low-level
** protocol variables and/or structures.
*/
initialize_dux_protocol()
{
    register int i;

    /*
    ** Set the "single deep outstanding request table" to -1 (an invalid
    ** serving array index).  See the comment above "locate_serving_array()"
    ** for an explanation on how this works.
    */
    for (i = 1; i < MAXSITE; i++)
	serv_recv_req_slot[i] = -1;

    init_static_ack();
    init_static_misc_bufs();

    init_dux_boot();
}


#ifndef __hp9000s300
/*
**			   DUX_PRINT_SERVING_ENTRY
**
** Called before a panic() to print the contents of a serving
** entry.
*/
void
dux_print_serving_entry(entry)
register struct serving_entry *entry;
{
    msg_printf("Serving_entry:\n");
    msg_printf("\tflags		 = 0x%02x\n", entry->flags);
    msg_printf("\treq_address	 = %02x%02x%02x%02x%02x%02x\n",
	entry->req_address[0], entry->req_address[1],
	entry->req_address[2], entry->req_address[3],
	entry->req_address[4], entry->req_address[5]);
    msg_printf("\treq_index	 = %d\n",	entry->req_index);
    msg_printf("\trid		 = 0x%08x\n",	entry->rid);
    msg_printf("\tmsg_mbuf	 = 0x%08x\n",	entry->msg_mbuf);
    msg_printf("\tbyte_recved	 = %d\n",	entry->byte_recved);
    msg_printf("\tstate		 = %d\n",	entry->state);
    msg_printf("\tno_retried	 = %d\n",	entry->no_retried);
    msg_printf("\tmtod(msg_mbuf) = 0x%08x\n\n",
	mtod((struct mbuf *)(entry->msg_mbuf), unsigned long));
}
#endif /* ! __hp9000s300 */

/*
**			SEND_ACK
**
** called by recv_reply() when it receives a non-idempotent reply.
** It is used on the using site.
*/
send_ack(ph)
register struct proto_header *ph;	/* points to the reply to ack	 */
{
    register struct mbuf *reqp;
    register struct dm_header *dmp;
    register struct proto_header *ack_ph;	/* points to the ack		*/
#ifdef __hp9000s300
    lan_gloptr myglobal;
    myglobal = Myglob_dux_lan_gloptr;
#endif

    /*
    **	 There are no retries on an ACK message. As far as the
    **	 protocol is concerned this transaction is complete. If the
    **	 serving side doesn't receive this ACK then it will re-send the
    **	 reply. The ack will be re-sent but the method is a little tricky.
    **	 The using array entry for the original request has already been
    **	 released. The recv_reply() code checks if the
    **	 (using_entry->flags == 0 || using_entry->rid != ph->p_rid) ,
    **	 if this is true then we simply re-send the ack. The scheme is
    **	 based on the premise of: If we don't have a record of this transaction,
    **	 then we have already sent the ACK, so send it again. The routine
    **	 dux_hw_send() ensures that ACKS are not retried. This is a fast ACK
    **	 routine, it uses/re-uses a statically allocated mbuf and calls the
    **	 drivers hw_send routine directly; no waiting in the xmit Q.
    **
    **   On the 700/800 we don't directly call the lan driver but dux_send
    **   doesn't retry ACKS, so we can get away with using a stack mbuf for
    **   acks.
    **
    */

    reqp = &ack_mbuf;
    dmp	 = DM_HEADER(reqp);
    ack_ph = &(dmp->dm_ph);

    /* Have to make sure we have a valid site before using DUX_ASSIGN_ADDRESS,
     * clustab will not be valid if this is the ack to the reply for the
     * request to cluster.
     */
#ifdef DUX_MULT_LAN_CARDS
    if ((clustab[ph->p_srcsite].status & CCT_STATUS_MASK) != CL_IS_MEMBER) {
#endif

        DUX_COPY_ADDRESS(ph->iheader.sourceaddr, ack_ph->iheader.destaddr);

#ifdef DUX_MULT_LAN_CARDS
	/* We must be a client, so we have only one lan card. */
        dmp->dm_lan_card = 0;
    }
    else {
    	DUX_ASSIGN_ADDRESS(ph->p_srcsite, ack_ph, dmp);
    }
#endif
    dmp->dm_dest = ph->p_srcsite;

    ack_ph->p_rid = ph->p_rid;
    ack_ph->p_rep_index = ph->p_rep_index;

    STATS_xmit_op_P_ACK++;
#ifdef __hp9000s300
    /*
    ** send it out directly through DIO lan driver
    */
    log_hw_send_status(DUXCALL(HW_SEND_DUX)((&(myglobal->hwvars)),
					     (u_char *)dmp, LAN_MIN_LEN));
#else
    dux_send(reqp);
#endif
    return(0);
}


/*
**			RECV_ACK
**
**  recv_ack is called by the driver when an ack packet arrives.
**  This ack packet acks the receiving of a non-idempotent reply.
**  recv_ack is used in the serving site.
*/

recv_ack(hwvars)
register hw_gloptr hwvars;
{
    register struct dm_header *dmp =
	    (struct dm_header *)READ_ADDR(hwvars->rxring);
    register struct proto_header *ph = &(dmp->dm_ph);
    register struct serving_entry *serving_entry;
    register int s;

    /*
    ** 1st get the serving array entry.
    */
    serving_entry = &serving_array[ph->p_rep_index];

    /*
    ** If the serving array entry flags indicate that this entry is
    ** still being used and the rids match up, then we have to untimeout
    ** the retry, release the associated mbuf, and release this entry.
    */

    if ((serving_entry->flags & S_USED) && (serving_entry->rid == ph->p_rid))
    {
	s = spl_arrays();
	untimeout(retry_reply, serving_entry);
#ifdef __hp9000s300
	/*
	** if retry_reply has put a reply msg in the duxQ remove it
	*/
	if (serving_entry->flags & S_IN_DUXQ)
	{
	    serving_entry->flags &= (~S_IN_DUXQ);
	    remove_from_duxQ(serving_entry->msg_mbuf);
	}

	serving_entry->flags = 0;	/* free this entry */
	/*
	** Must drop priority to free the mbuf
	*/
	splx(s);
	dm_release(serving_entry->msg_mbuf, 1);
#else
	if ((serving_entry->flags != 0) &&
	    !(serving_entry->flags & S_ACK_RCVD))
	{
	    switch (serving_entry->state)
	    {
	    case WAITING_ACK:
		/*
		 * Normal case, we were waiting for an ACK and it
		 * arrived.  Restore the spl level and free the reply
		 * message.
		 */
		splx(s);
		release_reply_msg(serving_entry);
		break;

	    case SENDING_REPLY:
		/*
		 * WOW!	 The client ACKed before we even finished
		 * performing all of the stuff that we do to send the
		 * reply.  This is possible since there are several
		 * things that we do in between the time that we
		 * actually send the packet over the LAN and when we
		 * set the state to WAITING_ACK.  Since the sending
		 * stuff will continue to access the reply message, we
		 * cannot yet free it.	Instead, we set the state to
		 * ACK_RECEIVED.  The code in dux_send() will check
		 * the state before setting the state to WAITING_ACK.
		 * If it sees that the ACK has already been received,
		 * it will release the reply message itself.
		 *
		 * This case rarely ocurrs, mainly when the server is
		 * relatively busy.
		 */
		serving_entry->state = ACK_RECEIVED;
		STATS_fast_ack++;	/* keep track of these */
		splx(s);
		break;

	    case ACK_RECEIVED:
		/*
		 * This is similar to the SENDING_REPLY case, but in
		 * this case, we got _two_ (or more!) ACKs while still
		 * sending the reply.  We just ignore this ACK (but we
		 * do update another statistic for tracking purposes).
		 */
		STATS_many_fast_acks++; /* keep track of these */
		splx(s);
		break;

	    default:
		/*
		 * A bad state.	 Print some useful info and panic.
		 */
		printf("recv_ack: invalid state in serving_array[%d]\n",
		    ph->p_rep_index);
		dux_print_serving_entry(serving_entry);
		splx(s);
		panic("recv_ack: bad serving_array state");
		break;
	    }
	}
	else
	{
	    splx(s);
	}
#endif /* s300 vs s700, s800 */
    }

    skip_frame(hwvars);
} /* END: recv_ack */


#if defined(__hp9000s700) || defined(__hp9000s800)
/*
**			     RELEASE_REPLY_MSG
**
** Release reply message by calling dm_release.	 If there is a buffer
** associated with this reply, then make sure the reply has been sent.
**/

release_reply_msg(serving_entry)
register struct serving_entry *serving_entry;
{
    register int s = spl_arrays();
    register struct dm_header *dmp;
    register struct buf *bp;
    register dm_message reply_msg = serving_entry->msg_mbuf;

    dmp = DM_HEADER(reply_msg);
    bp = dmp->dm_bufp;
    /* if the reply has been scheduled for retransmission when
    ** ack arrives, don't release the message until the channel finishes
    ** writing out the entire message
    */
    if (bp && (bp->b_flags2 & B2_DUXCANTFREE))
    {
	/*
	** turn on S_ACK_RCVD flag to prevent duplicate release
	** when duplicate acks are received back-to-back
	*/
	serving_entry->flags |= S_ACK_RCVD;
	timeout(release_reply_msg, serving_entry, 2);
	splx(s);
    }
    else
    {
	serving_entry->flags = 0;
	serving_entry->state = NOT_ACTIVE;
	untimeout(release_reply_msg, serving_entry);
	splx(s);
	dm_release(reply_msg, 1);
    }
}
#endif /* s700 || s800 */


/*
**			RECV_SLOW_REQUEST
**
** recv_slow_request() is called in the using site by the driver when
** it sees a pseudo reply. recv_slow_request() calls untimeout() to
** clear the retry of this request.
*/

recv_slow_request(hwvars)
register hw_gloptr hwvars;
{
    register struct dm_header *dmp =
	    (struct dm_header *)READ_ADDR(hwvars->rxring);
    register struct proto_header *ph = &(dmp->dm_ph);
    register struct using_entry *using_entry = &using_array[ph->p_req_index];
    register dm_message rep_mbuf;
    register int s;
    site_t reply_site;
    struct dm_multisite *dest_list;
    int stop_retry;

    reply_site = ph->p_srcsite;

    if ((using_entry->rid == ph->p_rid) && (using_entry->flags & U_USED))
    {
	if (ph->p_flags & P_MULTICAST)
	{
	    /*receiving a slow request for a clustercast or multisite send*/
	    rep_mbuf = using_entry->rep_mbuf;
	    dest_list = ((struct dm_multisite *)
			 ((DM_BUF(rep_mbuf))->b_un.b_addr));

	    if (dest_list->dm_sites[reply_site].status != DM_MULTI_SLOWOP)
	    {
		/* could get duplicate slow_request */
		dest_list->dm_sites[reply_site].status = DM_MULTI_SLOWOP;

		stop_retry = ((--(dest_list->no_response_sites)) <= 0);
	    }
	}
	else stop_retry = 1;

	if (stop_retry)
	{
	    s = spl_arrays();
	    untimeout(retry_request, using_entry->rid);
#ifdef __hp9000s300
	    /*
	    ** retry request has put request msg in the DUXQ, remove it
	    */
	    if (using_entry->flags & U_IN_DUXQ)
	    {
		using_entry->flags &= (~U_IN_DUXQ);
		remove_from_duxQ(using_entry->req_mbuf);
	    }
#endif /* s300 */
	    splx(s);

	    /*
	    ** If this slow request is counted in the protocol window for this
	    ** site, then lets open the window and check to see if there are
	    ** request(s) waiting outside the window.
	    */
	    if ((using_entry->flags & U_COUNTED) &&
		(reply_site > 0) && (reply_site < MAXSITE))
	    {
		/* Don't count this request in the window any more */
		using_entry->flags &= ~U_COUNTED;

		check_protocol_window(reply_site);
	    }
	}
    }
    skip_frame(hwvars);
} /* END: recv_slow_request */


#ifdef __hp9000s300
/*
**		LOG_HW_SEND_STATUS
*/
log_hw_send_status(return_code)
register u_int return_code;
{
    switch(return_code)
    {
case E_LLP_NO_BUFFER_SEND:
	STATS_xmit_no_buffer_send++;
	break;

case E_HARDWARE_FAILURE:
	STATS_xmit_hw_failure++;
	msg_printf("WARNING: hw_send_dskless, E_HARDWARE_FAILURE\n");
	break;

default:
	break;
    }
}


/*
**			CHECK_LAN_BREAK
**
** This code is fairly unusual. The actions are identical for both
** clients and servers, with the exception of possible panics on the
** client that are avoided on a server.
**
** The basic ideas are as follows:
**	We don't come here unless the selftest code is ready to
**	declare a cnode dead. It calls this function which takes a
**	"before" snapshot of certain important hwvars statistics as
**	well as certain protocol statistics. Then it attempts to send
**	16 small "are-you-alive" messages to the node in question.
**	After attempting to send the msgs it takes the "after"
**	snapshots of the same statistics and compares them. If they
**	indicate that the transmit interrupt wasn't able to call isr_proc()
**	to update the stats, then the LAN is considered broken.
**	At this point we enter the 2nd stage of the algorithm. We "think"
**	the LAN is broken, but to find out for sure we reset the card
**	and allow the LAN diagnostics to run and make the final diagnosis.
**	If the LAN is indeed broken/unterminated the diagnostics leave it
**	marked as DOWN and we have to reset the card again if we want to
**	continue. Since the LAN is broken we simply report this to the
**	calling routine and allow its timeout mechanism(s) to call us again
**	at a later time. If the LAN is re-connected after some period of
**	time, then the selftest code is still going to attempt to declare
**	the node dead and recalls this code. We redo the 1st part of the
**	algorithm, which is still not able to xmit (HW is DOWN), then
**	we call the 2nd stage of the algorithm which resets the LAN and
**	this time  marks it as being UP. Since there was a discrepency
**	between the 1st and 2nd stages we will re-run the 1st stage. If the
**	1st stage passes ok, we still report the LAN as broken
**	and allow another selftest period to pass. This is necessary
**	to allow this as well as the other nodes to begin xmitting again.
**	If the 1st stage fails again and this is a client, then
**	panic, else if the server printf an error.
**
*/
char *lanbrk_err_35 = "MAU disconnect";
char *lanbrk_err_36 = "Suspected  backbone cable not properly terminated";
char *lanbrk_err_37 = "Suspected AUI cable disconnect from MAU or grounded backbone cable";
char *card_state_msg = "FATAL ERROR: DISKLESS LAN FAILURE: Card State";
extern int drv_restart();

int
check_lan_break(dest)
register site_t dest;
{
    register dm_message reqp;
    register struct dm_header *dmp;
    register struct proto_header *ph;
    extern struct mbuf lan_break_mbuf;
    register int i, err;
    unsigned int card_state;
    int card_reset_count = 0;
    int s;
    lan_gloptr myglobal = Myglob_dux_lan_gloptr;

    /*
    ** Ok, 1st lets get the current INT_RETRY, UNSENDABLE, and ALL_TRANSMIT
    ** counters from the the card statistics area. Also get the current
    ** diskless protocol stats concerning hw_failures and unavailability of
    ** xmit buffers.
    */

retry:

    prev_retry_cntr = (&(myglobal->hwvars))->statistics[INT_RETRY];
    prev_unsendable = (&(myglobal->hwvars))->statistics[UNSENDABLE];
    prev_all_xmit_cntr =(&(myglobal->hwvars))->statistics[ALL_TRANSMIT];
    prev_xmit_no_buffer_send = STATS_xmit_no_buffer_send;
    prev_xmit_hw_failure = STATS_xmit_hw_failure;

    for (i = 0; i < LANBREAK_MSGS_TO_SEND; i++) /* Must be N*16 */
    {
	/*
	** 2nd: Prepare to send a probe message - actually just another
	** clustercast of a DM_ALIVE msg.
	*/
	if ((reqp = (dm_message) &lan_break_mbuf) != NULL)
	{
	    dmp = DM_HEADER(reqp);
	    ph = &(dmp->dm_ph);
	    ph->p_flags = P_DATAGRAM;
	    ph->p_srcsite = my_site;
	    dm_send(reqp, DM_DATAGRAM, DM_ALIVE, dest,
		    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	else
	{
	    panic("diskless: check_lan_break: NULL dskless_mbuf\n");
	}
    }

    /*
    ** Ok, lets get the new (hopefully) updated stats.
    */
    curr_retry_cntr = (&(myglobal->hwvars))->statistics[INT_RETRY];
    curr_unsendable = (&(myglobal->hwvars))->statistics[UNSENDABLE];
    curr_all_xmit_cntr = (&(myglobal->hwvars))->statistics[ALL_TRANSMIT];
    curr_xmit_no_buffer_send = STATS_xmit_no_buffer_send;
    curr_xmit_hw_failure = STATS_xmit_hw_failure;

    if (((&(myglobal->hwvars))->card_state != HARDWARE_UP) ||
	((curr_xmit_hw_failure - prev_xmit_hw_failure) > 0) ||
	((curr_xmit_no_buffer_send - prev_xmit_no_buffer_send) > 0) ||
	((curr_unsendable - prev_unsendable) >= LANBREAK_MSGS_TO_SEND) ||
	(curr_all_xmit_cntr == prev_all_xmit_cntr))
    {
#ifdef LANBREAK_DEBUG
	printf("DG_DEBUG: lan break code thinks lan is broken - ");
	if ((&(myglobal->hwvars))->card_state != HARDWARE_UP)
	    printf("card_state != HARDWARE_UP\n");
	else if ((curr_xmit_hw_failure - prev_xmit_hw_failure) > 0)
	    printf("curr_xmit_hw_failure - prev_xmit_hw_failure is > 0\n");
	else if ((curr_xmit_no_buffer_send - prev_xmit_no_buffer_send) > 0)
	    printf("curr_xmit_no_buffer_send - prev_xmit_no_buffer_send is > 0\n");
	else if ((curr_unsendable - prev_unsendable) >= LANBREAK_MSGS_TO_SEND)
	    printf("curr_unsendable - prev_unsendable  is >= LANBREAK_MSGS_TO_SEND\n");
	else if (curr_all_xmit_cntr == prev_all_xmit_cntr)
	    printf("curr_all_xmit_cntr == prev_all_xmit_cntr\n");
#endif LANBREAK_DEBUG

	/*
	** Possible broken lan! The only real way to know for
	** sure is to reset the card and let the extended selftests
	** take place. We have to be careful to avoid infinite loops,
	** only allow 1 card reset, then panic.
	*/
	if (card_reset_count < 1)
	{
	    card_reset_count++;
	}
	else
	{
	    if (!IM_SERVER)	/* diskless cnode */
	    {
		panic("Diskless: LAN Failure, Unknown Cause\n");
	    }
	    else
	    {
		printf("Diskless: LAN Failure, Unknown Cause\n");
		return(-1);
	    }
	}

	/*
	** Here goes the reset....
	*/
	s = splimp();
	err = drv_restart(myglobal);
	splx(s);
	msg_printf("WARNING: Diskless: Performed card reset on LAN card at select code %d\n",
		   (&(myglobal->hwvars))->select_code);
#ifdef LANBREAK_DEBUG
	printf("DG_DEBUG: drv_restatr(myglobal) returns %d\n", err);
	printf("DG_DEBUG: card_state = %d\n", (&(myglobal->hwvars))->card_state);
#endif LANBREAK_DEBUG

	if ((err == 0) && ((&(myglobal->hwvars))->card_state == HARDWARE_UP))
	{
	    /*
	    ** LAN ok! The above code thinks the lan was broken or
	    ** unterminated. However, the LAN drivers selftest code
	    ** thinks all is ok. Lets try to send the messages
	    ** again. If we can now xmit, then the card must have
	    ** gone catatonic and is now functioning. Since we have
	    ** already bumped the card_reset retry counter simply retry.
	    */
#ifdef LANBREAK_DEBUG
	    printf("DG_DEBUG: lan_break thinks lan broken, drv_restarts says ok, retry\n");
#endif LANBREAK_DEBUG
	    goto retry;
	}
	else
	{
	    /*
	    ** Failure.	 Examine the hwvars card state
	    */
	    card_state = (&(myglobal->hwvars))->card_state;
#ifdef LANBREAK_DEBUG
	    printf("DG_DEBUG: myglobal->hwvars->card_state = %d\n", card_state);
#endif LANBREAK_DEBUG
	    switch(card_state)
	    {
	case 35:
		/*
		** Cable is unterminated at one end or
		** MAU is not securely tapped into the
		** backbone.
		*/
		printf("%s or %s\n", lanbrk_err_36, lanbrk_err_35);
		return(0);	/* allow recovery to retry */

	case 36:
		/*
		** Cable is unterminated at both ends.
		*/
		printf("%s\n", lanbrk_err_36);
		return(0);	/* allow recovery to retry */

	case 37:
		/*
		** AUI cable is not connected to the MAU
		** or the backbone cable is grounded and
		** should not be.
		*/
		printf("%s\n", lanbrk_err_37);
		return(0);	/* allow recovery to retry */

	default:
		/*
		** The following are unrecoverable errors.
		** If they happen on the client node then we will
		** panic. However, if they happen on the rootserver
		** then we will allow the client node to be declared
		** dead and hopefully the system administrator will
		** be able to procede to a normal shutdown via
		* user-land...
		*/
		if (card_state >= 1 && card_state <= 34)
		{
		    /*
		    ** LAN interface card failure
		    */
		    printf(" %s = %d\n", card_state_msg,card_state);

		    if (!IM_SERVER)
		    {
			panic("Diskless: LAN Interface Card Failure");
		    }
		    else
		    {
			printf("Diskless: LAN Interface Card Failure");
			return(-1);
		    }
		}
		else if (card_state >= 39 && card_state <= 42)
		{
		    /*
		    ** Link Failure
		    */
		    printf(" %s = %d\n", card_state_msg,card_state);

		    if (!IM_SERVER)
		    {
			panic("Diskless: LAN Link Failure");
		    }
		    else
		    {
			printf("Diskless: LAN Link Failure");
			return(-1);
		    }
		}
		else if (card_state == 43 || card_state == 44)
		{
		    /*
		    ** HW Failure
		    */
		    printf(" %s = %d\n", card_state_msg,card_state);
		    if (!IM_SERVER)
		    {
			panic("Diskless: LAN Hardware Failure");
		    }
		    else
		    {
			printf("Diskless: LAN Hardware Failure");
			return(-1);
		    }
		}
		else
		{
		    if (!IM_SERVER)
		    {
			panic("Diskless: LAN Failure, Invalid card_state");
		    }
		    else
		    {
			printf("Diskless: LAN Failure, Invalid card_state");
			return(-1);
		    }
		}
	    } /* end of case */
	} /* end if-then-else E_NO_ERROR */
    } /* End of if */
    else
    {
	/*
	** Go ahead and declare the cnode dead... Could be a result of
	** a good lan but we are actually not receiving messages from
	** the cnode in question, or we had a bad news HW failure on the
	** root server and are trying to recover. BUT, if we did go through
	** and actual card reset, then lets try one more time via the
	** recovery code.
	*/
	if (card_reset_count == 0)
	{
	    return(-1);
	}
	else
	{
	    return(0);
	}
    }
} /* End of check_lan_break */
#else /* s700, s800 */
/*
 * We have adapted the code of lanc_link_status (in sio/lanc.c)
 * so that we don't sleep() under interrupt since we do lan break
 * detection under timeout.  This requires some coordination with
 * the recovery code:  init_check_lan_break() should be called at least
 * 3 seconds before the actual lan break check is performed in order
 * to allow us to compare some lan card statistics over a two
 * second interval. The 300 lan break code looks directly at the
 * hardware registers, so it doesn't have to wait around, but for
 * portability (e.g. between CIO & NIO) on the 800, we are looking at
 * the statistics maintained by the lanc interface which has at best
 * a one second resolution.
 */

#ifdef _WSIO
/*
 * The s700 lan interface may go into a DEAD state when the lan tee
 * is removed from the machine.  Because of this, we must reset the
 * card when the it goes DEAD.
 */
#define RESET_CARD_ON_LAN_FAILURE
#endif

/*
 * globals used to snapshot lan interface stats
 */
int xmit_noerr = 0,
    xmit_retrys = 0,
    xmit_lost_carrier = 0,
    xmit_restart = 0,
    xmit_deadmau = 0;

#ifdef RESET_CARD_ON_LAN_FAILURE
/*
 * num_lan_resets keeps track of how many times we have attempted
 * a LAN reset.  We will try a reset about every 5 seconds, so we
 * only allow 4 resets in a row.
 */
#define MAX_LAN_RESETS 4

int dux_lan_reset_count;
#endif /* RESET_CARD_ON_LAN_FAILURE */

/*
 * xmit_flags keeps track of our current lanbreak detection state.
 */
#define LANBREAK_CHECK_IN_PROGRESS	0x01
#define LANBREAK_RESET_IN_PROGRESS	0x02
#define LANBREAK_RESET_FAILED		0x04

int xmit_flags[MAX_DUX_LAN_CARDS] = 0;


/*
**			    DUX_IDENTIFY_LAN
**
** Routine to print out the identifying information for our lan
** interface.  This is a function to minimize the number of copies
** of our printf string (we do this in alot of places).
*/
void
dux_identify_lan(lan_card)
	int lan_card;
{
#ifdef _WSIO
    printf(", major %d, minor 0x%06x\n",
	major(duxlan_dev[lan_card]), minor(duxlan_dev[lan_card]));
#else
    printf(", major %d, mi %d\n", duxlan_major[lan_card],
    					duxlan_mgr_index[lan_card]);
#endif
}


/*
**			    DUX_LANC_CHECK_RESET
**
** Routine executed under timeout that clears
** LANBREAK_CHECK_IN_PROGRESS flag.
*/

dux_lanc_check_reset(lan_card)
	int lan_card;
{
    xmit_flags[lan_card] &= ~LANBREAK_CHECK_IN_PROGRESS;
}


#ifdef RESET_CARD_ON_LAN_FAILURE
/*
**			    DUX_LANC_RESET_RESET
**
** Routine executed under timeout that clears
** LANBREAK_RESET_IN_PROGRESS flag.
**
** If we reset the card, and the reset failed, we set xmit_flags
** to indicate that the reset of the card failed.
*/
dux_lanc_reset_reset(lan_card)
	int lan_card;
{
    /*
     * If we have initiated a lan reset, we check to see if the lan
     * is still dead.  If so, we mark that the reset failed.  The next
     * time that init_check_lan_break() is called (also under timeout)
     * and finds that the lan is in the DEAD state, it looks at
     * LANBREAK_RESET_IN_PROGRESS and LANBREAK_RESET_FAILED to decide
     * how to handle the dead condition.
     */
    if (xmit_flags[lan_card] & LANBREAK_RESET_IN_PROGRESS)
    {
	lan_ift *lanc_ift_ptr = (lan_ift *)dux_lan_cards[lan_card];
	int s;

	/*
	 * The CRIT/UNCRIT pair here is necessary to protect xmit_flags
	 * from being modified behind our back.  It also serves to
	 * keep the printf() calls together.
	 */
	s = CRIT();
	printf("reset of diskless network interface ");
	if (lanc_ift_ptr->hdw_state & LAN_DEAD)
	{
	    printf("failed");
	    xmit_flags[lan_card] |= LANBREAK_RESET_FAILED;
	}
	else
	{
	    printf("succeeded");
	    xmit_flags[lan_card] &= ~LANBREAK_RESET_FAILED;
	    dux_lan_reset_count = 0; /* reset our max reset counter */
	}
	xmit_flags[lan_card] &= ~LANBREAK_RESET_IN_PROGRESS;
	dux_identify_lan(lan_card);
	UNCRIT(s);
    }
}
#endif /* RESET_CARD_ON_LAN_FAILURE */


/*
**			     INIT_CHECK_LAN_BREAK
*/
init_check_lan_break(dest)
{
    dm_message reqp;
    struct dm_header *dmp;
    struct proto_header *ph;
    int lan_card;
    lan_ift *lanc_ift_ptr;
    int i;
    extern struct mbuf lan_break_mbuf;

    /* Get which lan card it is */
    lan_card = clustab[dest].lan_card;
    lanc_ift_ptr = (lan_ift *)dux_lan_cards[lan_card];
    /*
     * if the check has been set up by someone else already do nothing
     */
    if (xmit_flags[lan_card] & LANBREAK_CHECK_IN_PROGRESS)
	return;

    xmit_flags[lan_card] |= LANBREAK_CHECK_IN_PROGRESS;

    /*
     * If the lan is DEAD, we might want to reset the interface.
     * Even if we don't, there is no need in trying to send packets,
     * they will just be thrown away.
     *
     * We do not reset the interface more than MAX_LAN_RESETS times.
     */
    if (lanc_ift_ptr->hdw_state & LAN_DEAD)
    {
#ifdef RESET_CARD_ON_LAN_FAILURE
	/*
	 * Only try to reset if a reset is not already in progress.
	 */
	if ((xmit_flags[lan_card] & LANBREAK_RESET_IN_PROGRESS) == 0)
	{
	    /*
	     * Either our last reset failed (and we have not exceeded
	     * our max number of resets) or the lan just went dead.
	     */
	    if ((xmit_flags[lan_card] & LANBREAK_RESET_FAILED) == 0)
	    {
		/*
		 * The interface just went from OK to DEAD.
		 */
		printf("diskless network interface failure");
		dux_identify_lan(lan_card);
		printf("resetting network interface");
		dux_identify_lan(lan_card);
	    }

	    /*
	     * Only try to reset if we have not exceeded our lan reset
	     * count.
	     */
	    if (dux_lan_reset_count < MAX_LAN_RESETS)
	    {
		int s;

		dux_lan_reset_count++;

		/*
		 * Initiate a LAN_REQ_RESET on our interface.  We setup
		 * a timeout to check the status 5 seconds from now.
		 */
		s = spl2();
		(*lanc_ift_ptr->hw_req)(lanc_ift_ptr, LAN_REQ_RESET, 0);
		timeout(dux_lanc_reset_reset, lan_card, HZ*5);
		splx(s);
	    }
	}
#endif /* RESET_CARD_ON_LAN_FAILURE */
	timeout(dux_lanc_check_reset, lan_card, HZ*2);	/* check 2 sec later */
	return;
    }

    /*
     * Get the statistics.  We call the lan ioctl() routine to do
     * this -- that way it is driver independent.  These routines
     * will not block, so it is safe to call them, even on the ICS.
     */
    {
	register struct ifnet *ifp = (struct ifnet *)lanc_ift_ptr;
	register int (*if_ioctl)() = ifp->if_ioctl;
	struct fis fis_data;

	fis_data.reqtype = RX_FRAME_COUNT;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) == 0)
	    xmit_noerr = fis_data.value.i;
	else
	    xmit_noerr = 0;

	fis_data.reqtype = EXCESS_RETRIES;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) == 0)
	    xmit_retrys = fis_data.value.i;
	else
	    xmit_retrys = 0;

	fis_data.reqtype = CARRIER_LOST;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) == 0)
	    xmit_lost_carrier = fis_data.value.i;
	else
	    xmit_lost_carrier = 0;

	fis_data.reqtype = LAN_RESTART;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) == 0)
	    xmit_restart = fis_data.value.i;
	else
	    xmit_restart = 0;

#if defined(__hp9000s700)
	/*
	 * The NO_HEARTBEAT stuff on the s700 is broken, due to a
	 * hardware defect.  So that we do not think the LAN is
	 * broken, when a cnode is really dead, we are disabling
	 * the HEARTBEAT check for the s700 at this time.
	 *    -- rsh 05/30/91
	 */
#else
	fis_data.reqtype = NO_HEARTBEAT;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) == 0)
	    xmit_deadmau = fis_data.value.i;
	else
	    xmit_deadmau = 0;
#endif
    }

    for (i = 0; i < LANBREAK_MSGS_TO_SEND; i++)	/* Must be N*16 */
    {
	/*
	** clustercast of a DM_ALIVE msg.
	*/
	if ((reqp =  &lan_break_mbuf) != NULL)
	{
	    dmp = DM_HEADER(reqp);
	    ph = &(dmp->dm_ph);
	    ph->p_flags = P_DATAGRAM;
	    ph->p_srcsite = my_site;
	    dm_send(reqp, DM_DATAGRAM, DM_ALIVE, dest,
		    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	else
	{
	    panic("init_check_lan_break: NULL dskless_mbuf\n");
	}
    }
    timeout(dux_lanc_check_reset, lan_card, HZ*2);	/* check 2 sec later */
}


/*
**				CHECK_LAN_BREAK
**
** check for lan card/cable failure
*/
check_lan_break(dest)
    site_t dest;
{
    int link_status = 0;
    lan_ift  *lanc_ift_ptr;
    int lan_card;

    /* Get which lan card it is */
    lan_card = clustab[dest].lan_card;
    lanc_ift_ptr = (lan_ift *)dux_lan_cards[lan_card];

    /*
     * Card is dead, decide how to handle it.
     */
    if (lanc_ift_ptr->hdw_state & LAN_DEAD)
    {
#ifdef RESET_CARD_ON_LAN_FAILURE
	/*
	 * If a LAN_REQ_RESET has been initiated, but has not yet
	 * completed, simply pretend that the interface is broken in
	 * a recoverable way while the reset is in progress.
	 */
	if (xmit_flags[lan_card] & LANBREAK_RESET_IN_PROGRESS)
	    return 0; /* lan is broken, but may be recoverable */

	/*
	 * If the LAN_REQ_RESET was performed, but the reset failed we
	 * see if we have exceeded our max number of reset attempts.
	 * If so, we return -1, indicating that the lan is too broken
	 * to recover.
	 */
	if (xmit_flags[lan_card] & LANBREAK_RESET_FAILED)
	{
	    /*
	     * If we have tried to reset the card more than the max
	     * number of tries, indicate that the lan is ok.
	     * If we are a client, this will cause us to reboot
	     * (i.e. lost contact with root/swap server).  If we
	     * are the server, this will cause us to declare our
	     * cnodes dead.
	     */
	    if (dux_lan_reset_count > MAX_LAN_RESETS)
	    {
		printf("diskless network interface failure");
		dux_identify_lan(lan_card);
		return -1; /* lan is too broken to recover */
	    }
	    else
		return 0; /* lan is broken, but may be recoverable */
	}

	/*
	 * We have not initiated a reset yet.  The lan was okay when
	 * we called init_check_lan_break(), but then died.  We assume
	 * for now that we can recover.  The next time that we call
	 * init_check_lan_break(), we will attempt a reset and find
	 * out the real story.
	 */
	return 0; /* lan is broken, but may be recoverable */
#else/* RESET_CARD_ON_LAN_FAILURE */
	/*
	 * We cannot handle this situation, print a failure message
	 * and indicate an unrecoverable lan hardware error.
	 */
	printf("diskless network interface failure");
	dux_identify_lan(lan_card);
	return(-1); /* lan is too broken to recover */
#endif /* RESET_CARD_ON_LAN_FAILURE */
    }

#ifdef RESET_CARD_ON_LAN_FAILURE
    /*
     * The lan is not dead.  If we think a LAN_REQ_RESET is in progress
     * it must be finished.  We untimeout() the 5 second timer for
     * dux_lanc_reset_reset() and then explictly call it to cleanup
     * our state and print the messages that the lan reset succeeded.
     *
     * NOTE:
     *   Since the reset succeeded, we assume that the lan is ok now.
     *   We return 0, indicating that the lan has a recoverable
     *   hardware error (even though we have now just recovered from
     *   it).
     */
    if (xmit_flags[lan_card] & LANBREAK_RESET_IN_PROGRESS)
    {
	untimeout(dux_lanc_reset_reset, lan_card);
	dux_lanc_reset_reset(lan_card);
	return 0; /* lan is in a recoverable state now */
    }
#endif /* RESET_CARD_ON_LAN_FAILURE */

    /*
     * The card is alive, look at various statistics and make an
     * educated guess about the state of the lan.
     */
    {
	register struct ifnet *ifp = (struct ifnet *)lanc_ift_ptr;
	register int (*if_ioctl)() = ifp->if_ioctl;
	struct fis fis_data;

	fis_data.reqtype = EXCESS_RETRIES;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) != 0)
	    fis_data.value.i = 0;
	if (fis_data.value.i - xmit_retrys != 0)
	    link_status |= LANC_LS_OPEN_LAN;

	fis_data.reqtype = CARRIER_LOST;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) != 0)
	    fis_data.value.i = 0;
	if  (fis_data.value.i - xmit_lost_carrier != 0)
	    link_status |= LANC_LS_CARRIER;

#if defined(__hp9000s700)
	/*
	 * The NO_HEARTBEAT stuff on the s700 is broken, due to a
	 * hardware defect.  So that we do not think the LAN is
	 * broken, when a cnode is really dead, we are disabling
	 * the HEARTBEAT check for the s700 at this time.
	 *    -- rsh 05/30/91
	 */
#else
	fis_data.reqtype = NO_HEARTBEAT;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) != 0)
	    fis_data.value.i = 0;
	if (fis_data.value.i - xmit_deadmau != 0)
	    link_status |= LANC_LS_BAD_MAU;
#endif

	fis_data.reqtype = LAN_RESTART;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) != 0)
	    fis_data.value.i = 0;
	if (fis_data.value.i - xmit_restart != 0)
	    link_status |= LANC_LS_MAU_CONECT;

	fis_data.reqtype = RX_FRAME_COUNT;
	if ((*if_ioctl)(ifp, NETSTAT, &fis_data) != 0)
	    fis_data.value.i = 0;

	/*
	 * No transmissions and no errors!  Must be a hung interface.
	 * We tried to fix it, but failed, return a status that will
	 * make us think that the lan is OK, which will end up forcing
	 * us to think we lost contact with our server/clients.
	 */
	if (fis_data.value.i - xmit_noerr == 0 && link_status == 0)
	{
#ifdef NOT_DEFINED
	    printf("diskless network interface is hung");
	    dux_identify_lan(lan_card);
#endif /* not_defined */
	    return(-1);
	}
    }

    if (link_status & LANC_LS_OPEN_LAN)
    {
	printf("Suspected backbone cable not properly terminated");
	dux_identify_lan(lan_card);
    }

#if defined(__hp9000s700)
	/*
	 * The NO_HEARTBEAT stuff on the s700 is broken, due to a
	 * hardware defect.  So that we do not think the LAN is
	 * broken, when a cnode is really dead, we are disabling
	 * the HEARTBEAT check for the s700 at this time.
	 *    -- rsh 05/30/91
	 */
#else
    if (link_status & LANC_LS_BAD_MAU)
    {
	printf("broken MAU or poor connection to MAU");
	dux_identify_lan(lan_card);
    }
#endif

    if (link_status & LANC_LS_CARRIER)
    {
	printf("Suspected AUI cable disconnect from MAU or grounded backbone cable");
	dux_identify_lan(lan_card);
    }

    if (link_status & LANC_LS_MAU_CONECT)
    {
	printf("MAU is incorrectly connected to backbone cable");
	dux_identify_lan(lan_card);
    }

    /*
     * If we have some sort of recoverable hardware error, we return 0.
     * Otherwise, we return -1.
     */
    return link_status ? 0 : -1;
}
#endif /* s300 vs. s700, s800 */


/*
**			SEND_INTRPT_DGRAM
**
** This function is called to send a datagram under interrupt by the
** protocol code:
**
** For flow control: the server will send a negative acknowledgement (P_NAK)
**	which is basically a "choke" packet to the requestor if the
**	LAN card buffer, DUX network buffer, or the serving array overflows.
**
** For system integrity: This routine is also used to send a suicide msg
**	 to a node that is sending messages but is not a member of the
**	 cluster. (P_SUICIDE).
**
** For I-am-alive: This routine is also used to do a quick protocol level
**	 reply (P_DATAGRAM) to DM_ALIVE requests (P_ALIVE_MSG).
**
** Since each of the three supported functions are very important tasks
** and are datagrams with no data, static mbufs are utilized to ensure that
** we always have an mbuf available for the task. Of course we could re-use
** the same one over and over but it is sometimes confusing.
*/

send_intrpt_dgram(dest_site, what_to_do)
register site_t dest_site;
register unsigned long what_to_do;	/* P_DATAGRAM, P_NAK,or P_SUICIDE */
{
    register dm_message reqp;
    struct dm_header *dmp;
    register struct proto_header *ph;


    /*
    ** Initialize the static mbuf.
    */
    switch(what_to_do)
    {
case P_DATAGRAM:
	reqp = (dm_message) &dgram_mbuf;
	break;

case P_NAK:
#ifdef __hp9000s300
	sw_trigger(&duxQ_intloc, dux_hw_send, 0 ,
		   LAN_PROC_LEVEL,
		   LAN_PROC_SUBLEVEL+1);
#endif
	reqp = (dm_message) &nak_mbuf;
	STATS_xmit_op_P_NAK++;
	break;

case P_SUICIDE:
	reqp = (dm_message) &suicide_mbuf;
	break;

default:
	return(-1);
    }

    /*
    ** If the reqp is null, then simply continue, datagrams aren't
    ** guaranteed delivery anyway.
    */
    if (reqp != NULL)
    {
	dmp = DM_HEADER(reqp);
	ph = &(dmp->dm_ph);
	ph->p_flags = what_to_do;
	ph->p_srcsite = my_site;

	/*
	** The DM_ALIVE op is only used as a place-holder in
	** this situation. The "what_to_do" flag will be
	** interpreted at the protocol layer and the DM layer
	** is not actually called for these special messages.
	** (e.g. P_NAK and P_SUICIDE). However, it is very
	** important to use this op-code since it is also used
	** by the send_datagram() function to determine whether
	** the mbuf should be given away.
	*/
	dm_send(reqp, DM_DATAGRAM, DM_ALIVE, dest_site,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	return(0);

    }
    else
    {
	panic("send_intrpt_dgram: NULL discless_mbuf.");
    }
    /*NOTREACHED*/
}


#if defined(KERNEL_DEBUG_ONLY) && defined(__hp9000s300)
/*
**		INVOKE_DEBUGGER_ACTION
**
*/
invoke_debugger_action(action)
register int action;
{
    switch(action)
    {
case DEBUGGER:
	debugger_trap();
	break;

case RECOVERY_ON:
	recovery_on();
	break;

case RECOVERY_OFF:
	recovery_off();
	break;

default:
	msg_printf("invoke_debugger_action: invalid request\n");
	break;
    }
}


/*
**		     DEBUGGER_TRAP
*/
debugger_trap()
{
    asm("	trap &14");
}


/*
**			  DELIVER_DEBUGGER_ACTION
**
** Only used for S300's for the "Centralized Debugger Console"
** From user land a link address is supplied of the destination
** for this packet. It should be either a single cnode or a
** clustercast address of some diskless cluster.
*/
struct mbuf debugger_mbuf;
deliver_debugger_action()
{
    register dm_message reqp;
    register struct dm_header *dmp;
    lan_gloptr myglobal = Myglob_dux_lan_gloptr;

    reqp = (dm_message)&debugger_mbuf;

    if (reqp == NULL)
	panic("deliver_debugger_action: NULL mbuf");

    dmp	 = DM_HEADER(reqp);
    DUXCALL(HW_SEND_DUX)((&(myglobal->hwvars)),
			 (u_char *)dmp, LAN_MIN_LEN);
}
#endif /* KERNEL_DEBUG_ONLY && __hp9000s300 */


/*
**			RECV_NAK_MSG
**
** recv_nak_msg() is called by driver's isr_proc() when a NAK message
** is received on the requesting site.	It records the arrival time of
** the NAK message plus some delta in the clustab[] entry of the site
** which sends the NAK back. If a NAK message is received in the past
** delta period from a destination,  dux_hw_send() will stop sending
** requests to that site and schedule a retry.
**
*********************************************************************
**
**			CHOKE_PERIOD Strategy:
**
**	Experimentation with several methods have resulted in the
**	following method of calculating the choke period. It is not
**	the best, but seems to be somewhat fair, easy to calculate,
**	and doesn't require special assembly hooks for generating a
**	a random number.
**
**	Requirements: We need a method to flow-control the
**		entire cluster that has the following attributes:
**
**		1) Fairness
**		2) Creates a set of logical sub-lans
**		3) Easy and fast to calculate.
**		4) Fairly random, but bounded for every cnode.
**
**	General Algorithm:
**
**		CHOKE_PERIOD = (random * logical_sublan) + disk_factor;
**				  ^		^	       ^
**				  A		B	       C
**	Where:
**
**	A) Fairly random, but bounded for every cnode.
**
**			random = (((lbolt % 8) + 2)
**
**		Lbolt can be considered to be a fairly random
**		number from cnode to cnode. There is guaranteed
**		delay in booting discless cnodes since	the bootserver
**		can only handle a limited number of simultaneous
**		sessions and each session will complete at different
**		times.
**
**		Since lbolt is monitonically increasing, there
**		is a natural backoff for each cnode within a
**		given range.
**
**	B) logical_sublan = ((my_site % 4) +1));
**
**		This idea breaks the 255 cnode cluster into 4 logical
**		sublans 1,2,3, and 4.
**
**			grp #1: Cnodes id# 4, 8, 12, 16, 20, ...
**			grp #2: Cnodes id# 1, 5,  9, 13, 17, ...
**			grp #3: Cnodes id# 2, 6, 10, 14, 18, ...
**			grp #4: Cnodes id# 3, 7, 11, 15, 19, ...
**
**
**	C) Disk Factor: We know that we have a disc speed of approx.
**		450 Kbs. This means that we probably want to back
**		off for about a 1/4 of this bandwidth, or the
**		functional equivalent in netbufs = 0.25 sec.
**		This is important to prevent really short delays
**		that make the problem worse.
**
**	Ranges of backoff:  (Assuming HZ=50)
**
**			grp #1: 0.29 - 0.43 sec.
**			grp #2: 0.33 - 0.61 sec.
**			grp #3: 0.37 - 0.79 sec.
**			grp #4: 0.41 - 0.97 sec.
**
**	Note: modulo values are powers of 2.
**
*/

#define QTR_SEC	  (HZ/4)
u_int	CHOKE_PERIOD = 0;

recv_nak_msg(source_site)
register site_t source_site;
{
    register int s;
    extern u_int CHOKE_PERIOD;

    /*
    ** Register the arrival time of NAK message and add in the delta.
    */
    CHOKE_PERIOD =
	((((lbolt % 8) + 2)*((my_site % 4) + 1)) * (HZ/50)) + QTR_SEC;

    s = CRIT();
    clustab[source_site].nak_time = lbolt + CHOKE_PERIOD;
    UNCRIT(s);
}


/*
**			RECV_SUICIDE_MSG
*/
recv_suicide_msg()
{
    dux_boot("No longer a valid cnode. Declared dead by root server.");
}


/*
**			DUX_HW_SEND
**
** Send the next packet from the dux transmit Q.
** After the last packet of a message has been sent, deque.
** If the message is an idempotent reply, its message and data buffer
** are freed. The retry logic will handle rare sending errors.
** This routine is not re-entrant.  Only one instance of this code can
** be executed at a time.  It is scheduled to run under software
** interrupt at two places: dux_send AND isr_proc() of LAN driver.
** Isr_proc() will directly recall this routine on a transmit interrupt
** if and only if the transmit buffers have been released and the
** transmit stats have been updated on the card.
** Both will schedule the software interrupt to run at the level of
** (LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL + 1)
*/
#ifdef __hp9000s300
dux_hw_send()
#else /* s700, s800 */
dux_hw_send(duxmsg)
register dm_message duxmsg;
#endif
{
#ifdef __hp9000s300
    register dm_message duxmsg;
    int s;
#endif
    register struct dm_header *dmp;
    register struct proto_header *ph;
    int length;	     /* len of packet to be sent, needed by hw_send() */
    int end_of_msg;  /* boolean var */
    int index;
    struct dm_multisite *dest_list;
    struct dm_site *p_dm_site;
    site_t dest_site;
    int send_this_msg;	 /* whether to send this message for flow control */
    int bypass_flow_cntrl;
    int err;
    u_short saved_p_flags;	/* saved  fields used to restore a message  */
    u_short saved_iheader_length;   /* if not LAN card xmit buffer is available */
#ifdef __hp9000s300
    lan_gloptr myglobal = Myglob_dux_lan_gloptr; /*driver interface structure*/
#endif

    /*
    ** Since dux_send() queues requests onto the duxQ, we simply have
    ** to look at the Qhead to determine if there is anthing waiting to
    ** be transmitted. If the Qhead is NULL then simply return.
    */

    /*
    ** Start off with a positive attitude.
    */
    send_this_msg     = TRUE;
    bypass_flow_cntrl = FALSE;

#ifdef __hp9000s300
    retry_new_duxmsg:

    s = CRIT();
    if ((duxmsg = (dm_message)duxQhead) == NULL)
    {
	UNCRIT(s);
	return(0);
    }
    UNCRIT(s);

    /*
    ** Q is not empty, send a packet by calling hw_send
    */
#endif /* s300 */

    dmp = DM_HEADER(duxmsg);
    ph = &(dmp->dm_ph);

    saved_p_flags = ph->p_flags;	/* save the flags */

    if ((ph->p_flags & P_REQUEST))
    {
	/*
	** If a DM_DUXCAST or DM_CLUSTERCAST then the destination is
	**		a multisite address. They are not subject to flow
	**		control.  DM_CLUSTERCAST is subject to flow control
	**		after it becomes a multisite request during retries.
	**		The flow control mechanism is based on the inability
	**		to send to a particular site. This is meaningless for
	**		 multisite addresses.
	*/

	switch(dmp->dm_dest)
	{
    case DM_MULTISITE:
	    /*
	    ** It ends up being very difficult to allow flow
	    ** control for a multisite message, even though we send
	    ** serially to all sites. Remember that we only have a single
	    ** using_entry for the entire multisite. If we don't bypass
	    ** flow control for the multisites, the following could happen:
	    **
	    ** Scenerio: If we were attempting to send the 2nd packet
	    **	of a multipkt message out and could not due to flow
	    **	control, then we would skip the rest of the message
	    **	for that site and determine the next dest_site to
	    **	send to. We would send to all of the other sites
	    **	and setup a request retry on the using_entry.
	    **	Since at least one site would not have received
	    **	a complete message  and we would be guaranteed of having
	    **	to complete a retry. If we "might" not have to retry
	    **	then we may as well xmit all of the pkts, and hope
	    **	for the best. No sense in making the whole thing more
	    **	complicated than necessary.
	    **
	    ** The proto headers destination address has already been
	    ** populated by either net_request or the retry_request code.
	    ** All we need to do here is a consistency check on the reply
	    ** mbuf.
	    */
	    index     = ph->p_req_index;
#if defined(__hp9000s700) || defined(__hp9000s800)
	    if (using_array[index].rid != ph->p_rid)
	    {
		/*
		** recovery code has declared our remaining
		** targets dead and freed up using_entry
		*/
		return END_OF_MESSAGE;
	    }
#endif /* s700 || s800 */
	    if (using_array[index].rep_mbuf == NULL)
	    {
		panic("dskless_hw_send: P_REQUEST, invalid using_array index");
	    }

	    /*
	    ** No - I didn't forget the break, fall through to the
	    **	next statements.
	    */

    case DM_CLUSTERCAST:
    case DM_DUXCAST:
    case DM_DIRECTEDCAST:
	    bypass_flow_cntrl = TRUE;
	    break;				/* Now break */

    case INVALID_SITEID_0:
	    /*
	    ** Consistency check, site id 0 is invalid.
	    */
	    panic("dskless_hw_send: P_REQUEST, invalid cnode id 0.");

    default:
	    /*
	    ** Single destination request.
	    */
	    if (dmp->dm_dest > 0 && dmp->dm_dest < MAXSITE)
	    {
		dest_site = dmp->dm_dest;
	    }
	    else
	    {
		panic("dskless_hw_send: P_REQUEST, invalid cnode id.");
	    }
	} /* END of switch */

	/*
	** Remember Replies are not subject to flow control. The space
	** for the reply was allocated prior to making the request.
	*/

	/*
	** If we cannot send this message because of flow control we might
	** be able to xmit to other sites. Search the duxQ for messages destined
	** for another site and determine if we can send to it.
	**
	** Remember the song: And the bus driver says move-on-back, ...
	**		 And the daddies on the bus go read-read-read, ...
	**		 And the mommies on the bus go shuh-shuh-shuh, ...
	**		 And the children on the bus go up-and-down,....
	**		 All thru the town...
	*/
	if (bypass_flow_cntrl == FALSE)
	{
	    switch (flow_control(dest_site))
	    {
	case -1:
		/*
		** No one to send to... All under flow control
		*/
		send_this_msg = FALSE;
#if defined(__hp9000s700) || defined(__hp9000s800)
		return(DELAY_MESSAGE);
#endif
		break;
#ifdef __hp9000s300
	case  1:
		/*
		** The duxQ has been shuffled. The new duxQhead is
		** different from the previous. Bypass flow control.
		*/
		bypass_flow_cntrl = TRUE;
		send_this_msg	  = TRUE;

		goto retry_new_duxmsg;
#endif /* s300 */
	case 0:
	default:
		/*
		** Send to the original destination.
		*/
		send_this_msg = TRUE;
	    } /* END of switch */
	}
    } /* End P_REQUEST */

    /*
    ** Take this path if we are actually going to try to send this message.
    */
    if (send_this_msg == TRUE)
    {
	/*
	** decide packet length
	*/
	length = MINIMUM2(LAN_PACKET_SIZE,
			  ph->p_dmmsg_length + ph->p_data_length
			  - ph->p_byte_no + PROTO_LENGTH);
#if defined(__hp9000s700) || defined(__hp9000s800)
	/*
	** If a data buffer is to be carved up into multiple packets, ensure
	** that each mbuf is mapped on to data that is CPU_IOLINE-byte aligned and
	** of a length that is a multiple of CPU_IOLINE bytes.	(In general, the
	** latter will not necessarily be true of the last data packet, but
	** it will be for paging-related data since the total data length is
	** always a multiple 2kb.  We really shouldn't have to do this,
	** but the VM and I/O systems assumes that swap drivers do I/O on
	** CPU_IOLINE-aligned addresses.  This re-alignment is actually
	** beneficial even for non-swap data in that some additional
	** buffering and copying by the CAM is eliminated.
	**			-byb
	**
	** Changed CPU_IOLINE to LAN1_DATA_ALIGNMENT (64 bytes);
	** apparently NIO Lanbrusca likes 64-byte alignment.
	** This should cover both cases.
	**			-byb 12/27/88
	**
	** For the Series 700, 4 byte alignment is sufficient.  The
	** 700 lan driver does DMA chaining and can handle packets
	** of any alignemnt.  Using 4 byte alignment eliminates the
	** need to copy misalgined (1..3) data into a separate buffer.
	**			-rsh 04/11/91
	*/

#define LAN1_DATA_ALIGNMENT    64 /* WARNING: hard-coded -- (from lan1.h) */

#ifdef __hp9000s700
#define GOOD_DATA_ALIGNMENT	4
#else
#define GOOD_DATA_ALIGNMENT	LAN1_DATA_ALIGNMENT
#endif

	if (length == LAN_PACKET_SIZE && ph->p_data_length)
	{
	    int data_length = LAN_PACKET_SIZE - PROTO_LENGTH -
			((ph->p_byte_no < ph->p_dmmsg_length) ?
				ph->p_dmmsg_length : 0);

	    if (data_length > GOOD_DATA_ALIGNMENT)
		length -= data_length % GOOD_DATA_ALIGNMENT;
	}
#endif /* s700 || s800 */

	/*
	** check if the next packet is the last one of a message
	*/
	end_of_msg = ph->p_byte_no + (length - PROTO_LENGTH)
			>= ph->p_dmmsg_length + ph->p_data_length;

	/*
	** This is the end of the message. Set the proto flags to indicate
	** P_END_MSG and make sure the message is at least the minimum legal
	** length.
	*/
	if (end_of_msg)
	{
	    ph->p_flags	 |= P_END_OF_MSG;

	    if (length < LAN_MIN_LEN)
	    {
		length = LAN_MIN_LEN;
	    }
	}
	else
        {
            /*
            ** Make sure P_END_OF_MSG flag is not set.  When retrying
            ** MULTISITE messages this can get set when the message
            ** is sent to the first destination, causing a premature
            ** end of packet panic on following destinations.
            */
            ph->p_flags  &= ~P_END_OF_MSG;
        }


	/*
	** Set the length field in the packet header. It equals to
	** the packet length minus the length of address fields and
	** the length field
	*/
	if (length < 14)
	{
	    panic("dux_hw_send: ph->iheader.length going negative!");
	}

	saved_iheader_length = ph->iheader.length;     /* save the length */
	ph->iheader.length = length - 14;

#ifdef DUX_PROTOCOL_DEBUG
	if (net_debug > 5)
	{
	    msg_printf("dux_hw_send: calling hw_send_dux with duxmsg = %x, dmp = %x, length = %x\n",
		       duxmsg, dmp, length);
	}
#endif /* DUX_PROTOCOL_DEBUG */

	/*
	** Make the actual call to the driver to have this pkt sent off.
	**
	** If the driver interface tells us there are no xmit
	** buffers available, then it makes real good sense to simply
	** attempt to send out this same packet. This could simply be a
	** single packet message or it could be the 2nd-6th packet of
	** a multi-packet message. In the latter case, if we don't attempt
	** to re-send the same packet, the next one out will certainly result
	** is the receiving side getting an out-of-sequence packet, and
	** having to wait for the re-transmit via a retry. We only have to
	** re-establish the diskless protocol iheader.length and p_flags
	** fields and return.
	**
	*/
	if (req_pkt_seqno == 0) /* excluded: see comment for badseq() */
	    req_pkt_seqno++;

	ph->p_seqno = req_pkt_seqno++;
	ph->p_version = P_VERS_8_0;
#ifdef __hp9000s300
	err = DUXCALL(HW_SEND_DUX)((&(myglobal->hwvars)),
			(u_char *) dmp, length);
	log_hw_send_status(err);
#else
	if ((num_dux_lan_cards > 1) && ((dmp->dm_dest == DM_CLUSTERCAST) ||
					(dmp->dm_dest == DM_CLUSTERCAST))) {
	    /* Have to send these out on all lan cards */
	    for (dmp->dm_lan_card = 0; dmp->dm_lan_card < num_dux_lan_cards;
						dmp->dm_lan_card++) {
		if (err = hw_send(dmp, length)) {
			break;
		}
	    }
	}
	else {
		err = hw_send(dmp, length);
	}
#endif

#ifdef __hp9000s300
	if (err == E_LLP_NO_BUFFER_SEND)
#else
	if (err)
#endif
	{
	    /*
	    ** We couldn't get a transmit buffer. Restore the already
	    ** modified fields of the proto header. This same packet will be
	    ** re-sent by the network transmit interrupt, isr_proc(),
	    ** when a transmit buffer is available.  This is a simple
	    ** method of preventing the transmission of out-of-sequence
	    ** packets if we temporarily run out of xmit buffers.
	    */
	    ph->iheader.length = saved_iheader_length;
	    ph->p_flags = saved_p_flags;

	    return(err);
	}

	/*
	** If this isn't the end-of-message, we have to update the
	** proto-headers byte count to reflect the number of bytes
	** that we think were sent out.
	*/
	if (!end_of_msg)
	{
#ifdef __hp9000s300
	    /*
	    ** The next packet will be sent by the network transmit
	    ** interrupt, isr_proc(), after the current packet is transmitted.
	    ** The card is set up to interrupt on transmit. When the H/W isr
	    ** of the LAN driver fields this interrupt it does some basic
	    ** bookkeeping and cleaning up. If there is space avail. on the
	    ** cards transmit buffer we will jump directly back into this
	    ** code to send the next pkt of a multi-pkt message. We will
	    ** continue to repeat the operation until the duxQ is exhausted.
	    **
	    ** Note: The xmit interrupt is asynchronous, we don't want to wait
	    ** or drop priority at this point.
	    **/
#endif /* s300 */
	    ph->p_byte_no += length - PROTO_LENGTH;
	    return(0);
	}
    } /* End send_this_msg == TRUE */

    /*
    ** Come here if this message was skipped because of flow control or
    ** if the message has been completely sent. A CLUSTERCAST message
    ** isn't turned into a multisite until it retries.
    */
    if (ph->p_flags & P_REQUEST && dmp->dm_dest == DM_MULTISITE)
    {
	/*
	** Find out the next destination for multisite message
	** Need to get the destination list out of the reply mbufs buf struct.
	*/
	index = ph->p_req_index;
	dest_list = ((struct dm_multisite *)
		     ((DM_BUF(using_array[index].rep_mbuf))->b_un.b_addr));

	dest_site = dest_list->next_dest;

	/*
	** Need to find the next site to send to and put its link address
	** in the destination address, zero the byte count sent, and bump
	** the next_dest address.
	**
	** Don't want to use dest_list->maxsite here, see following check.
	*/
	while (dest_site < MAXSITE)
	{
	    p_dm_site = & (dest_list->dm_sites[dest_site]);
	    if ((p_dm_site->site_is_valid) &&
		(!(p_dm_site->status & DM_MULTI_SLOWOP)) &&
		(!(p_dm_site->status & DM_MULTI_DONE)))
	    {
		/*
		** We have not received an ack for a duplicate request
		** or the reply from the next destination.
		*/
	        DUX_ASSIGN_ADDRESS(dest_site, ph, dmp);
		ph->p_byte_no = 0;
		dest_list->next_dest = dest_site + 1;
		break;	/* from the while loop */
	    }
	    dest_site++;
	} /*end of while*/

	if (dest_site <= dest_list->maxsite)
	{
#ifdef __hp9000s300
	    /*
	    ** We are still working on the same duxQ message. The
	    ** transmit interrupt will recall this function and get
	    ** things going again for the next site.
	    */
#endif
	    return(0);		/*more destinations to be sent*/
	}
	else
	{
	    /*
	    ** We have sent the message to all destinations,
	    **	deQ the	 multi destination request
	    */
#ifdef __hp9000s300
	    remove_from_duxQ(duxmsg);

	    s = CRIT();
	    using_array[ph->p_req_index].flags &= (~U_IN_DUXQ);
	    UNCRIT(s);
#else
	    return(END_OF_MESSAGE);
#endif
	}
    } /* endif multisite message */
    /*
    ** the message is a SINGLE DESTINATION request/reply  -or-
    ** a CLUSTERCAST that hasn't gone into a retry yet (e.g. an
    ** original clustercast request).
    */
    else
#if defined(__hp9000s700) || defined(__hp9000s800)
	return(END_OF_MESSAGE);
#else
    {
	/*
	** deQ the single destination message and turn off the bit
	** that indicates it was in the duxQ.
	*/
	remove_from_duxQ(duxmsg);

	s = CRIT();
	if (ph->p_flags & P_REQUEST)
	{
	    using_array[ph->p_req_index].flags &= (~U_IN_DUXQ);
	}

	if (ph->p_flags & P_REPLY)
	{
	    serving_array[ph->p_rep_index].flags &= (~S_IN_DUXQ);
	}
	UNCRIT(s);
    }

    /*
    ** start timer for retry - this is a very important task.
    */

    /*
    ** Arrange for retransmission. If we are not able to send the request
    ** bad things could be happening, like:
    **
    **	1) LAN receive buffer overflows: in which case the serving side
    **		has probably been sending back NAKs to slow down the
    **		requestor. While we are not sending requests, the
    **		retry timers are still in effect.
    **
    **	2) Serving side could be out of resources such as:
    **		o mbufs/cbufs/fsbufs			- configurable
    **		o serving array entries.		- configurable
    **
    **	3) LAN is saturated
    */
    if (ph->p_flags & P_REQUEST)
    {
	/*
	** Request retries take place in 2,3,4,5,5,5.... secs forever!
	*/
	if (using_array[ph->p_req_index].no_retried < MAXRETRYLIMIT)
	{
	    timeout(retry_request, using_array[ph->p_req_index].rid,
		    HZ*(using_array[ph->p_req_index].no_retried + 2));
	}
	else
	{
	    timeout(retry_request, using_array[ph->p_req_index].rid,
		    HZ*MAXRETRYLIMIT);
	}
    }
    else if (ph->p_flags & P_IDEMPOTENT)
    {
	/*
	** This	 must be an idempotent reply,
	** release the	dmmsg, buf, and serving_array
	*/
	dm_release(duxmsg, 1);

	s = CRIT();
	serving_array[ph->p_rep_index].flags = 0;
	UNCRIT(s);
    }
    else if (ph->p_flags & P_REPLY)
    {
	/*
	**  A non-idempotent reply or a reply for slow request,
	**  start timer	 for the  retry packet and set the serving
	**  state entry to indicate we are now waiting for an ACK.
	**  Since ACKs are sent using a static mbuf and placed directly
	**  on the card we shouldn't wait to long before resending the reply.
	**
	** Reply timeouts take place every 1/2 secs.
	**
	*/
	serving_array[ph->p_rep_index].state = WAITING_ACK;
	timeout(retry_reply, &(serving_array[ph->p_rep_index]), HZ/2);
    }
    else if (ph->p_flags & P_SLOW_REQUEST)
    {
	/*
	** After sending out a slow request message release the mbuf.
	*/
	dm_release(duxmsg, 0);
    }
    return(0);
#endif /* s700, s800 vs. s300 */
} /* END: dux_hw_send */


/*
**			FLOW_CONTROL
**
**	Is flow control in affect for this site? If yes, then we have to
**	search the rest of the duxQ to look for another site, re-verify
**	that the new site isn't under flow_control, shuffle the duxQ, and
**	finally return. The idea is that we want to send to other nodes
**	if they can take in the message.
*/
flow_control(dest_site)
site_t dest_site;
{
    register long delta_sec;		/* Must be signed */
#ifdef __hp9000s300
    register int s;
#endif
    register int err;

    if (clustab[dest_site].nak_time > 0)
    {
	delta_sec = lbolt - clustab[dest_site].nak_time;

	if (delta_sec <= 0)
	{
	    STATS_delta_sec++;	/* Cannot send to this site */
#ifdef __hp9000s300
	    s = CRIT_duxQ();
	    err = Qdriver_says_move_on_back(dest_site);
	    UNCRIT_duxQ(s);
#else
	    /*
	    ** There is no DUX_Q allowing us to shuffle messages
	    ** nor can we  shuffle the messages in the LAN driver
	    ** Q anyway.
	    */
	    err = -1;
#endif /* s300 vs s700, s800 */
	    /*
	    ** -1 cannot send to anyone in the duxQ
	    **	1 Send to the new duxQhead
	    */
	    return(err);
	}
    }

    /*
    ** Yes - send this message. This was the original head of the Q.
    */

    return(0);
}


#ifdef __hp9000s300
/*
**		QDRIVER_SAYS_MOVE_ON_BACK
**
**	NOTE: A lot of this code is duplicated in the calling func
**		but the statistics show that this routine will not
**		be called often, and speed is important.
*/
Qdriver_says_move_on_back(dest_site)
register site_t dest_site;
{
    register struct dm_header *dmp;
    register struct mbuf *p, *prev_m_act;
    register long delta_sec;

    /*
    ** Consistency check: the function remove_from_duxQ
    ** could have sandwiched in a Q manipulation  prior to this
    ** call.
    */
    if ((duxQhead == duxQtail) || (duxQhead->m_act == NULL))
    {
	return(-1);
    }

    /*
    **	Get a ptr to Qhead
    */
    p = duxQhead->m_act;
    prev_m_act = duxQhead;

    /*
    ** Walk the linked list trying to find a message that is destined
    ** for another cnode. If the next message is for the same cnode
    ** then keep looking. We will rotate chunks of messages if need be.
    */
    while ((p != duxQtail) && (p->m_act != NULL))
    {
	dmp = DM_HEADER(p);

	if ((dmp->dm_dest != dest_site) || (dmp->dm_ph.p_flags & P_REPLY))
	{
	    /*
	    ** Remember any sort of multisite isn't a candidate
	    ** for flow control.
	    */
	    switch (dmp->dm_dest)
	    {
	case DM_MULTISITE:
	case DM_CLUSTERCAST:
	case DM_DIRECTEDCAST:
	case DM_DUXCAST:
		goto adjust_Q;

	default:
		/*
		** Check if flow-cntrl in affect for this site
		*/
		if (clustab[dmp->dm_dest].nak_time > 0)
		{
		    delta_sec = lbolt - clustab[dmp->dm_dest].nak_time;

		    if (delta_sec <= 0)
		    {
			STATS_delta_sec++;
			goto shuffle_Q;
		    }
		}
		goto adjust_Q;

	    } /* END of switch */
	}
	else
	{
	    /*
	    ** Get the next position in the linked list
	    ** This was the same site.
	    */
shuffle_Q:
	    STATS_delta_sec++;
	    prev_m_act = p;
	    p = p->m_act;
	}
    } /* end of while loop */

    if (p == duxQtail)
    {
	goto adjust_Q;
    }

    /*
    ** No other site available.
    */
    return(-1);

adjust_Q:
    prev_m_act->m_act = NULL;
    duxQtail->m_act = duxQhead;
    duxQtail = prev_m_act;
    duxQhead = p;
    return(1);
}
#endif /* s300 */


#ifdef __hp9000s300
/*
**			DUX_SEND
**
**  dux_send() puts the message (mbuf) in the dux driver queue and kicks
**  dux_hw_send if the queue has been empty. It is called by net_request,
**  net_reply, send_ack, retry_request, and retry_reply. This code is protected
**  by spl6(), because retry_request() and retry_reply() are both invoked
**  via timeouts and want to put their retries back into the duxQ.
*/

dux_send(msg)
register struct mbuf *msg;
{
    register int s = spl_duxQ();

    /*
    ** This msg will become the duxQtail so need
    ** to set its active pointer to NULL.
    */

    msg->m_act = NULL;

    /*
    ** 1st check if the queue is already empty. If so, then this
    ** msg will become both the head and tail of the Q.
    */
    if (duxQtail == NULL)
    {
	duxQhead = duxQtail = msg;	/* enque */

	/* Q was empty, kick it.
	**
	** All calls to dux_hw_send must be mutually exclusive.	 Since
	** dux_hw_send is also invoked by the LAN driver's isr_proc()
	** under interrupt at priority (LAN_PROC_LEVEL, LAN_PROC_SUBLEVEL+1),
	** we have to schedule a sw_trigger at the same level. Only the
	** first trigger will be scheduled, others ignored, because the
	** same intloc structure is reused. This is ok, the duxQ is
	** exhausted when the scheduled trigger is worked upon.
	*/
	sw_trigger(&duxQ_intloc, dux_hw_send, 0 , LAN_PROC_LEVEL,
		   LAN_PROC_SUBLEVEL+1);
    }
    else
    {
	duxQtail = duxQtail->m_act = msg;	/* enque */
    }

    splx(s);
} /* END: dux_send */
#else /* s700, s800 */
/*
**			DUX_SEND
**
**  dux_send() calls dux_hw_send to  put the message (mbuf) into LAN driver
**  queue.  Once the packets is queued in the LAN driver, the protocol cannot
**  regulate their flow. To prevent a fast sender overflowing slow/busy server,
**  we restrict the number of large packets (messages with data buffers) that
**  can be queued for transmission.
**  It is called by net_request, net_reply, send_ack, retry_request,
**  and retry_reply.
*/

dux_send(msg)
dm_message msg;
{
    int err;
    register struct dm_header *dmp;
    register struct proto_header *ph;
    int index, s, s1;

    dmp = DM_HEADER(msg);
    ph = &(dmp->dm_ph);

    if (dmp->dm_bufp)
    {
	/*
	** Data buffers are directly chained into mbufs (to avoid double
	** buffering).	We need to tag the data buffers while they are being
	** written out so that recv_reply() and recv_ack() will not
	** release them when reply or ack arrives. The tag will be reset
	** when the driver and io routines complete copying  the last packet
	** of the message into LAN card's buffer.
	*/
	s = CRIT();
	dmp->dm_bufp->b_flags2 |= B2_DUXCANTFREE;
	UNCRIT(s);
    }

#ifdef DUX_LOG
    log_pd(PD_DUX_SEND, ph->p_req_index,ph->p_rid, dmp->dm_bufp, 
	    dmp->dm_bufp ? (dmp->dm_bufp->b_flags2 & B2_DUXFLAGS) : 0, 0);
#endif

    err = 0;
    while (err != END_OF_MESSAGE)
    {
	/* send the message one packet at a time until the entire message
	** has been transmitted to all destinations
        **
        ** Added ENXIO.  It is possible that the lan card is trying to
        ** reset itself in which case the write routine will return ENXIO.
        ** We had a hang where the system was hung in this loop because
        ** dux_hw_send was returning ENXIO and we just kept retrying.
        ** Unfortunately since it was from retry_reply we were already at
        ** spl5() and the lan card timeout never occured to finish resetting
        ** the card.  cwb 8/29/91
	*/
	if (err == DELAY_MESSAGE || err == ENOBUFS || err == ENOSPC ||
	    err == ENXIO || dux_lanQ_nearly_full())
	{
	    if (!ON_ISTACK)
	    {
		/*
		** Delay if not running under interrupt.
		** There are potentially many messages that
		** could be delayed by the network if things
		** are really busy.  To prevent one wakeup
		** from flooding the destination site, we
		** need to make the wakeup events for each message
		** distinct from others so that each message
		** is delayed for the intended amount of
		** time.  Sleeping on an address "randomized"
		** from (request id % 256) should provide a
		** reasonable approximation of this behavior.
		**
		** dts DSDe400918. on a very busy client, it is
		** possible to wakeup before we sleep!!
		*/
		s1 = spl6();
		timeout(wakeup, &dux_lan_full[ph->p_rid % 256], dux_send_delay);
		sleep((&dux_lan_full[ph->p_rid % 256]), PZERO);
		splx(s1);
	    }
	    else
	    {
		/*
		** Just escape and let retries and
		** recovery code fix things.
		*/
		break;
	    }
	}
	s1 = spl5();
	err = dux_hw_send(msg);
	if (err != END_OF_MESSAGE)
	    splx(s1);
    }

    /*
    ** If we have not sent out the complete message (perhaps due
    ** to flow control), we must make sure to clear the B2_DUXCANTFREE
    ** flag so that retries don't back off until eternity.
    */
    if (!(ph->p_flags & P_END_OF_MSG))
	if (dmp->dm_bufp)
	{
	    s = CRIT();
	    dmp->dm_bufp->b_flags2 &= ~B2_DUXCANTFREE;
	    UNCRIT(s);
	}

    /* The message has already been completely transmitted
    **
    ** start timer for retry - this is a very important task.
    */

    if (ph->p_flags & P_REQUEST)
    {
	/*
	** Arrange for retransmission. If we are not able to send the request
	** bad things could be happening, like:
	**
	**   1) LAN receive buffer overflows: in which case the serving side
	**	     has probably been sending back NAKs to slow down the
	**	     requestor. While we are not sending requests, the
	**	     retry timers are still in effect.
	**
	**   2) Serving side could be out of resources such as:
	**	     o mbufs/cbufs/fsbufs		     - configurable
	**	     o serving array entries.		     - configurable
	**
	**   3) LAN is saturated
	*/

	/*
	** Request retries take place in 2,3,4,5,5,5.... secs forever!
	*/
	if (using_array[ph->p_req_index].no_retried < MAXRETRYLIMIT)
	{
	    timeout(retry_request, using_array[ph->p_req_index].rid,
		    HZ*(using_array[ph->p_req_index].no_retried + 2));
	}
	else
	{
	    timeout(retry_request, using_array[ph->p_req_index].rid,
		    HZ*MAXRETRYLIMIT);
	}
    }
    else if (ph->p_flags & P_IDEMPOTENT)
    {
	/*
	** This	 must be an idempotent reply,
	** release the	dmmsg, buf, and serving_array
	*/
	release_reply_msg(&serving_array[ph->p_rep_index]);
    }
    else if (ph->p_flags & P_REPLY)
    {
	/*
	**  A non-idempotent reply or a reply for slow request,
	**  start timer	 for the  retry packet and set the serving
	**  state entry to indicate we are now waiting for an ACK.
	**  Since ACKs are sent using a static mbuf and placed directly
	**  on the card we shouldn't wait to long before resending the reply.
	**
	** Reply timeouts take place every 1/2 secs.
	**
	*/
	index = ph->p_rep_index;
	s = CRIT_arrays();
	switch (serving_array[index].state)
	{
	case SENDING_REPLY:
	    /*
	     * Normal case.  We are now done with sending the reply.
	     * Set the state to WAITING_ACK.  When the ACK arrives,
	     * recv_ack() will release the reply buffer.
	     */
	    serving_array[index].state = WAITING_ACK;
	    UNCRIT_arrays(s);
	    timeout(retry_reply, &(serving_array[index]), HZ/2);
	    break;

	case ACK_RECEIVED:
	    /*
	     * WOW!  The remote site was so fast with its ACK to our
	     * reply that it has already arrived!  When recv_ack()
	     * processed the ACK, it did not release the reply buffer
	     * (because we were still using it).  So it is up to us to
	     * release it.
	     */
	    UNCRIT_arrays(s);
	    release_reply_msg(&(serving_array[index]));
	    break;

	default:
	    /*
	     * A bad state.  Print some useful info and panic.
	     */
	    printf("dux_send: invalid state in serving_array[%d]\n",
		index);
	    dux_print_serving_entry(&(serving_array[index]));
	    UNCRIT_arrays(s);
	    panic("dux_send: bad serving_array state");
	    break;
	}
    }
    else if (ph->p_flags & P_SLOW_REQUEST)
    {
	/*
	** After sending out a slow request message release the mbuf.
	*/
	dm_release(msg, 0);
    }
    splx(s1);
    return(0);
} /* end of dux_send, S800 version*/
#endif /* s300 vs. s700, s800 */

#ifdef __hp9000s300
/*
**			REMOVE_FROM_DUXQ
**
** Remove a msg from duxQ. When remove_from_duxQ() is called,
** msg should be in duxQ. It is the responsibility of the
** calling function to change either the U_IN_DUXQ or S_IN_DUXQ
** flags as appropriate for the using/serving entry.
*/

remove_from_duxQ(msg)
register struct mbuf *msg;
{
    register int s;
    register struct mbuf *p, *q;

    s = CRIT();
    if (duxQhead == NULL)
    {
	UNCRIT(s);
	return;
    }
    else if (duxQhead == msg)
    {
	if ((duxQhead = msg->m_act) == NULL)
	    duxQtail = NULL;
    }
    else
    {
	q = duxQhead;
	while ((p = q->m_act) && (p != msg))
	    q = p;

	/* p == msg or p == NULL */
	if (p != NULL)
	{
	    if ((q->m_act = p->m_act) == NULL)
	    {
		/* msg is the last one in the duxQ, update duxQtail */
		duxQtail = q;
	    }
	}
    }

    UNCRIT(s);
} /* END: remove_from_duxQ */
#endif /* s300 */

/*
**			DUX_COPYIN
**
** Dux_copyin copies data from the lan card into a mbuf or buf.
** Duplicate or lost packet has been detected in the calling routine.
** Protocol header has been read to the dm_ph part of the dmp.
*/

dux_copyin(hwvars, dmp)
register hw_gloptr hwvars;	/* points to hw lan struct		   */
register struct dm_header *dmp; /* the dmmsg struct to hold the new dmmsg  */
{
#if defined(__hp9000s700) || defined(__hp9000s800)
    space_t space;
#endif
    register struct proto_header *ph = &(dmp->dm_ph);
    register int length;       /* number of useful bytes */

    if (ph->p_data_length == 0)
    {
	/*
	** no data in this message
	*/
#ifdef __hp9000s300
	length = DUXCALL(DUX_COPYIN_FRAME)(hwvars,
			   (u_char *) dmp + XMIT_OFFSET + ph->p_byte_no,
			   0, 0);
#else
	length = copyin_frame(hwvars,
			   (u_char *) dmp + XMIT_OFFSET + ph->p_byte_no,
			   KERNELSPACE);
#endif /* s300 vs. s700, s800 */
    }

    /*
    ** In order to keep parameters of copyin_frame unchanged
    ** from the CNO version, I inserted code into copyin_frame
    ** to skip PROTO_LENGTH for all dux packets
    */
    else if (ph->p_byte_no >= ph->p_dmmsg_length)
    {
	/*
	** no dmmsg in this packet
	*/
#if defined(__hp9000s700) || defined(__hp9000s800)
	/* TEMPORARY FIX UNTIL REGION KERNEL */
	/* Buffer may be in user space (swap). */
	/* Calculate space */

	space = bvtospace(dmp->dm_bufp,dmp->dm_bufp->b_un.b_addr);
#endif /* s700, s800 */

	dmp->dm_bufoffset = 0; /* zero or put it in transmit part */

#ifdef __hp9000s300
	length = DUXCALL(DUX_COPYIN_FRAME)(hwvars,
		 (u_char *)(BUF_ADDR(dmp) + ph->p_byte_no - ph->p_dmmsg_length),
		 0, 0);
#else
	length = copyin_frame(hwvars,
		 (u_char *)(BUF_ADDR(dmp) + ph->p_byte_no - ph->p_dmmsg_length),
		 space);
#endif /* s300 vs. s700, s800 */
    }
    else
    {
	/*
	** There are both dmmsg and data in this packet
	*/
#ifdef __hp9000s300
	/* Dux_copyin_frame allows copying parts of a packet into
	** different receiving buffers, hence no need to double buffer.
	*/

	length = DUXCALL(DUX_COPYIN_FRAME)(hwvars,
		 (u_char *) dmp + XMIT_OFFSET + ph->p_byte_no,
		 ph->p_data_offset,
		 BUF_ADDR(dmp));
#else
	/*
	** copyin_frame cannot copy data into multiple receive
	** buffers, so we need double buffering
	** FIX IT LATER !!!  ---------- csl 10/20/88
	*/
	static u_char tmp[LAN_PACKET_SIZE];
	length = copyin_frame(hwvars, tmp, KERNELSPACE);
	copy_network_data(tmp, (u_char *) dmp + XMIT_OFFSET +
			  ph->p_byte_no, ph->p_data_offset);
	dmp->dm_bufoffset = 0; /* zero or put it in transmit part */

	/* Buffer may be in user space (swap). */
	/* Calculate space */
	space = bvtospace(dmp->dm_bufp,dmp->dm_bufp->b_un.b_addr);
	privlbcopy(KERNELSPACE, tmp + ph->p_data_offset, space,
		   BUF_ADDR(dmp), length - ph->p_data_offset);
#endif /* s300 vs. s700, s800 */
    }

    return(length);
} /* END: dux_copyin */


/*
**			RETRY_REQUEST
**
**	This functionis called when a request timeout expires. From the
**	DUX protocol perspective, we have no idea why the timeout expired;
**	we only know that we did not receive a reply to our request, so
**	we will attempt to try again. Since this code runs via timeouts,
**	we are now under interrupt at spl5 (on Series 300). This code
**	will remain at this
**	level throughout the final call to dux_send() which will enqueue
**	the request for re-transmission.
**
**	Note: We absolutely cannot lower our priority level here....
**	     we must guard against slow replies crossing our
**	     paths.
**
*/

retry_request(rid)
register u_long rid;
{
/*
 * We must always be at least at spl_arrays() to prevent replies from
 * crossing our attempt to retry and free the using_entry, etc
 * out from under us.  On the 300, timeout puts us at spl5 (==splimp)
 * but on the 800, timeouts are spl2.
 */
    register int s = spl_arrays();
    register struct using_entry *using_entry;
    register struct dm_header *dmp;
    register struct proto_header *ph;
    register int index;
    struct dm_multisite *dest_list;
    site_t dest_site;
    struct dm_site *p_dm_site;
#if defined(__hp9000s700) || defined(__hp9000s800)
    struct buf *bp;
#endif

    using_entry = using_array;

    for (index = 0;  index < using_array_size; index++, using_entry++)
	if (using_entry->rid == rid)
	    break;

    if ((index >= using_array_size) || (using_entry->flags == 0))
    {
	/*
	** Couldn't find using_entry for our rid, it must be free.
	** This shouldn't happen, could be that the
	** timeout happened between the setting of the flags and the untimeout.
	** No sense in worrying over this.
	*/
	splx(s);
	return;
    }

    if (using_entry->flags & U_REPLY_IN_PROGRESS)
    {
	/*
	** Reply in progress, don't send a retry, as he might send an
	** ACK.	 If he does, we will be untimeout'ed or we won't be
	** found in the using array.  If the reply is incomplete, we
	** need to poke the server to send the reply again and thus we
	** do need to retry.
	*/
	splx(s);
#ifdef DUX_LOG
	u_busy_retry_request++;
#endif
	timeout(retry_request, rid, HZ/2);
	return;
    }

    using_entry->flags |= U_RETRY_IN_PROGRESS;

    dmp =  DM_HEADER(using_entry->req_mbuf);
#if defined(__hp9000s700) || defined(__hp9000s800)
    bp = dmp->dm_bufp;

#ifdef DUX_LOG
    ph	=  &(dmp->dm_ph);
    log_pd(PD_RETRY_REQUEST, ph->p_req_index, ph->p_rid, bp, 
		bp ? (bp->b_flags2 & B2_DUXFLAGS) : 0, 0);
#endif
    if (bp)
    {
	if (bp->b_flags2 & B2_DUXCANTFREE)
	{
	    /*
	    ** skip this round of retry because the previous attemp of send
	    ** is still buffered in the driver Q; restart retry
	    */
	    using_entry->flags &= ~U_RETRY_IN_PROGRESS;
	    splx(s);
	    timeout(retry_request, rid, HZ/2);
	    return;
	}
    }
#endif /* s700, s800 */

#ifndef DUX_LOG
    ph	=  &(dmp->dm_ph);
#endif

    ph->p_flags &= ~P_END_OF_MSG;
    ph->p_byte_no =  0;		 /* reset p_byte_no before retry */

    switch(dmp->dm_dest)
    {
case DM_CLUSTERCAST:
	/*
	** During retransmissions of clustercasts, send to one
	** destination at a time. Turn the destination into a multisite.
	*/
	dmp->dm_dest = DM_MULTISITE;

	/* No I didn't forget to put a break here */

case DM_MULTISITE:
	/*
	** Find out the next destination for multisite message
	** Need to get the destination list out of the reply mbufs buf struct.
	*/
	dest_list = ((struct dm_multisite *)
		     ((DM_BUF(using_array[index].rep_mbuf))->b_un.b_addr));

	/*
	** Must search the entire dest_list to find the next valid site
	*/
	dest_site = 1;


	/*
	** Need to find the next site to send to and put its link address
	** in the destination address, zero the byte count sent, and bump
	** the next_dest address.
	**
	** Don't want to use dest_list->maxsite here, see following check.
	*/
	while (dest_site < MAXSITE)
	{
	    p_dm_site = & (dest_list->dm_sites[dest_site]);
	    if ((p_dm_site->site_is_valid) &&
		(!(p_dm_site->status & DM_MULTI_SLOWOP)) &&
		(!(p_dm_site->status & DM_MULTI_DONE)))
	    {
		/*
		** We have not received an ack for a duplicate request
		** or the reply from the next destination.
		*/
	        DUX_ASSIGN_ADDRESS(dest_site, ph, dmp);
		ph->p_byte_no = 0;
		dest_list->next_dest = dest_site + 1;
		break;	/* from the while loop */
	    }
	    dest_site++;
	} /*end of while*/

	/*
	** If we don't have any sites to send to then I guess we just return.
	*/
	if (dest_site > dest_list->maxsite)
	{
	    using_entry->flags &= ~U_RETRY_IN_PROGRESS;
	    splx(s);
	    return;
	}
	break;

default:
	break;
    } /* END of switch */

#ifdef __hp9000s300
    /*
    ** If this request is NOT already in the duxQ from another retry
    ** then enqueue it and bump the protocol stats.
    */
    if ((using_entry->flags & U_IN_DUXQ) == 0)
    {
	using_entry->flags |= U_IN_DUXQ;
#endif /* s300 */

	if (using_entry->no_retried < 255)
	    using_entry->no_retried++;	/* Bump using array retry count */

	ph->p_retry_cnt = using_entry->no_retried;
	STATS_req_retries++;		/* Bump request retry stats */

	dux_send(using_entry->req_mbuf);
#ifdef __hp9000s300
    }
#endif

    using_entry->flags &= ~U_RETRY_IN_PROGRESS;

    splx(s);
} /* END: retry_request */


/*
**			RETRY_REPLY
**
** Retry_reply(): sends duplicate replys for non-idempotent operations.
** It is run when the timeout for this particular reply has expired.
** We are now at level 5.  We cannot lower our priority at this point.
*/

retry_reply(p_serving_array)
register struct serving_entry *p_serving_array;
{
    register struct proto_header *ph;
    register int s;
#if defined(__hp9000s700) || defined(__hp9000s800)
    register int s_orig;
    struct dm_header *dmp;
    struct buf *bp;

    /*
    ** We must always be at least at splimp() to prevent ACKs from
    ** crossing our attempt to retry and free the using_entry, etc
    ** out from under us.  On the 300, timeout puts us at spl5 (==splimp)
    ** but on the 800, timeouts are spl2.
    */
    s_orig = splimp();
#endif /* s700, s800 */

    s = CRIT();
    if (!((p_serving_array->flags & S_USED) &&
	  (p_serving_array->state == WAITING_ACK)))
    {
	/*
	** p_serving_array is re-used or free, this shouldn't happen, could be
	** that the timeout happened between the setting of the flags and the
	** untimeout.
	** No sense in worrying over this.
	*/
#ifdef __hp9000s300
	UNCRIT(s);
#else
	UNCRIT(s_orig);
#endif
	return;
    }
    UNCRIT(s);

    ph = &((DM_HEADER(p_serving_array->msg_mbuf))->dm_ph);

#if defined(__hp9000s700) || defined(__hp9000s800)
    dmp = DM_HEADER(p_serving_array->msg_mbuf);
    bp = dmp->dm_bufp;
    s = spl6();
    if (bp)
    {
	if (bp->b_flags2 & B2_DUXCANTFREE)
	{
	    /*
	    ** skip this round of retry because the previous attemp of send
	    ** is still buffered in the driver Q; restart retry
	    */
	    timeout(retry_reply, p_serving_array, HZ/2);
	    splx(s_orig);
	    return;
	}
    }
    splx(s);
#endif /* s700, s800 */

    s = spl_arrays();
    ph->p_retry_cnt++;
    ph->p_byte_no = 0;		/* reset p_byte_no before retry */
    ph->p_flags &= ~P_END_OF_MSG;

    /*
     * Since we are going to resend the reply, we must set the state
     * back to SENDING_REPLY.  Otherwise, recv_ack() might receive
     * an ACK and release the buffer out from under us.
     */
    p_serving_array->state = SENDING_REPLY;

#ifdef __hp9000s300
    /*
    ** If this retry isn't already in the duxQ then put it there.
    */
    if ((p_serving_array->flags & S_IN_DUXQ) == 0)
    {
	p_serving_array->flags |= S_IN_DUXQ;
#endif /* s300 */
	if (p_serving_array->no_retried == 0xFF) {
		int i;

		/* See if this is a reply we are never going to get an
		 * ack for.  This case could happen when we send an error reply
		 * to a site that is trying to cluster and it does not succeed
		 * in ACKing before dying.  cwb
		 */
		for (i = 1; i < MAXSITE; i++) {
			/* See if the site is not in cluster */

			if ((clustab[i].status != CL_INACTIVE) &&
				eq_address(p_serving_array->req_address,
			      		 clustab[i].net_addr)) {
				break;
			}
		}
		if (i >= MAXSITE) {
			/* Put it in a state clean_s_entry likes */
    			p_serving_array->state = ACK_RECEIVED;

			/* Set the rid to 0 so recv_ack() can't free the
			 * mbuf before clean_s_entry does.
			 */
    			p_serving_array->rid = 0;
#if defined(__hp9000s700) || defined(__hp9000s800)
    			splx(s_orig);
#else
			splx(s);
#endif
			clean_s_entry(p_serving_array);
			return;
		}
	}
	p_serving_array->no_retried++;
	splx(s);

	STATS_retry_reply++;

	/*
	** It is ok to call dux_send at spl5() since it
	** protects itself at spl_duxQ().
	*/
	dux_send(p_serving_array->msg_mbuf);
#ifdef __hp9000s300
    }
    else
    {
	splx(s);
    }
#endif
#if defined(__hp9000s700) || defined(__hp9000s800)
    splx(s_orig);
#endif
} /* END: retry_reply */


/*
**			SEND_DATAGRAM
**
** Send one packet out. Caller of this routine will not wait for reply.
** Lost messages will be handled by the user. Called by dm_send().
*/

send_datagram(req_mbuf)
register dm_message req_mbuf;
{
    register struct dm_header *dmp = DM_HEADER(req_mbuf);
    register struct proto_header *ph = &(dmp->dm_ph);
    register int length;
    int multi_card = 0;
#ifdef __hp9000s300
    lan_gloptr myglobal = Myglob_dux_lan_gloptr;
#endif

    /*
     * Default to single lan card.  Most common case
     */
    dmp->dm_lan_card = 0;

    /*
    ** If the flags indicate that the message is to be handled
    ** in a special way, then don't override them with the
    ** P_DATAGRAM designator. These special messages will be
    ** send as datagrams but are handled at the protocol layer.
    */
    switch(ph->p_flags)
    {
case P_NAK:
case P_ALIVE_MSG:
case P_SUICIDE:
	break;

default:
	ph->p_flags = P_DATAGRAM;
    }

    dux_ieee802_header(ph);
    ph->p_byte_no = 0;
    ph->p_dmmsg_length = dmp->dm_headerlen;
    ph->p_data_length = dmp->dm_datalen;
    ph->p_data_offset = 0;

    /*
    ** Need to fill in the full destination link address.
    */
    switch(dmp->dm_dest)
    {
case DM_DUXCAST:
	DUX_COPY_ADDRESS(duxcast_addr, ph->iheader.destaddr);
	multi_card = num_dux_lan_cards > 1;
	break;

case DM_CLUSTERCAST:
	DUX_COPY_ADDRESS(clustcast_addr, ph->iheader.destaddr);
	multi_card = num_dux_lan_cards > 1;
	break;

case DM_MULTISITE:
	/*
	** Multisite datagrams don't make sense, should be Clustercasts.
	** Release the resource and return an error to dm_send.
	*/
	dm_release(req_mbuf, 1);
	return (-1);

default:
	DUX_ASSIGN_ADDRESS(dmp->dm_dest, ph, dmp);
    }

    length = MINIMUM2(LAN_PACKET_SIZE,
		      ph->p_dmmsg_length + ph->p_data_length
		      - ph->p_byte_no + PROTO_LENGTH);

    if (length < LAN_MIN_LEN)
    {
	length = LAN_MIN_LEN;
    }
    ph->iheader.length = length - 14;

    STATS_xmit_op_P_DATAGRAM++;
#ifdef __hp9000s300
    log_hw_send_status(DUXCALL(HW_SEND_DUX)((&(myglobal->hwvars)),
					     (u_char *)dmp, length));
#else
    if (multi_card) {
	for (dmp->dm_lan_card = 0; dmp->dm_lan_card < num_dux_lan_cards;
						dmp->dm_lan_card++) {
    		hw_send(dmp, length);
	}
    }
    else {
    	hw_send(dmp, length);
    }
#endif
    /*
    ** Caller should not be waiting or replying and must not release req_mbuf,
    ** However, a couple of critical routines use static mbufs that MUST NOT
    ** be released, so check here.
    */
    switch(dmp->dm_op)
    {
case DM_ALIVE:
case DM_FAILURE:
case DM_SERSYNCREQ:
	break;

default:
	dm_release(req_mbuf, 1);
    }

    return(0);
} /* END: send_datagram */


/*
**			RECV_DATAGRAM
**
** recv_datagram() is called by the network driver's interrupt
** service routine isr_proc(). It runs under interrupt.
*/

recv_datagram(hwvars)
register hw_gloptr hwvars;
{
    register struct dm_header *dmp =
	    (struct dm_header *) READ_ADDR (hwvars->rxring);
    register struct proto_header *ph = &(dmp->dm_ph);
    register dm_message dmmsg;
    register int dmmsg_size;
    register struct buf *bufp;

    /*
    ** 1st try to allocate an mbuf/cbuf. If none are avail, don't wait,
    ** simply dump the message. A datagram, by definition, is not guaranteed to
    ** to actually make it through, no return codes or ACK/NAK are returned.
    */
    dmmsg_size = ph->p_dmmsg_length+XMIT_OFFSET-DM_HEADERLENGTH;

    if ((dmmsg = dm_alloc(dmmsg_size, DONTWAIT)) == NULL)
    {
	/*
	** Gather stats.
	*/
	if (dmmsg_size > MLEN)
	    STATS_recv_dgram_no_cluster++;
	else
	    STATS_recv_dgram_no_mbuf++;

	goto drop_datagram;
    }

    /*
    ** Do we need a dskless_fsbuf?
    */
    if (ph->p_data_length == 0)
    {
	bufp = NULL;
    }
    else
    {
	/*
	** Looks like we need to try and get a buf. If none are available
	** then release the already allocated mbuf and dump the frame.
	*/
	bufp = syncgeteblk(ph->p_data_length);

	if ((bufp == NULL) || (bufp == ((struct buf *)-1)))
	{
	    dm_release(dmmsg, 0);
	    if (bufp == NULL)
	    {
		STATS_recv_dgram_no_buf++;
	    }
	    else
	    {
		STATS_recv_dgram_no_buf_hdr++;
	    }
	    goto drop_datagram;
	}
    }
    /*
    ** copyin the dm_tranmit part and buffer data to dmmsg and buffer.
    **
    ** 1st need to read in the proto-header
    */

    dmp = DM_HEADER(dmmsg);
    DUX_COPY_PROTO(ph, &(dmp->dm_ph));
    ph = &(dmp->dm_ph);
    dmp->dm_bufp = bufp;

    /*
    ** Since we don't have a serving entry for a datagram, we should simply
    ** ignore the return value from dux_copyin().
    */
    (void) dux_copyin(hwvars, dmp);

    dm_recv_request(dmmsg);
    return (0);

drop_datagram:
#ifdef __hp9000s300
    sw_trigger(&duxQ_intloc, dux_hw_send, 0 , LAN_PROC_LEVEL,
	       LAN_PROC_SUBLEVEL+1);
#endif
    skip_frame(hwvars);
    return (-1);
} /* END: recv_datagram */


/*
**			DUX_IEEE802_HEADER
**
** Fill in the IEEE802 header for DUX packets
*/

dux_ieee802_header(ph)
register struct proto_header *ph;
{
    ph->iheader.dsap = IEEE802_EXP_ADR;
    ph->iheader.ssap = IEEE802_EXP_ADR;
    ph->iheader.ctrl = NORMAL_FRAME;
    ph->iheader.dxsap = DUX_PROTOCOL;
    ph->iheader.sxsap = DUX_PROTOCOL;
} /* END: dux_ieee802_header */


/*
**			CLEANUP_SERVING_ARRAY
**
** Cleanup function for DUX network after site failure is
** detected.
*/
cleanup_serving_array(crashed_site)
site_t crashed_site;
{
    register int index;
    register struct serving_entry *serving_entry;

    serving_entry = serving_array;

    for (index= 0; index < serving_array_size; index++, serving_entry++)
	if (serving_entry->flags & S_USED)
	{
	    switch (serving_entry->state)
	    {
	case SENDING_REPLY:
		/*
		 * We really don't think that we would be in either
		 * of these states (SENDING_REPLY or ACK_RECEIVED)
		 * when cleanup_serving_array() is invoked.  However,
		 * to be safe, we handle them anyway.
		 *
		 * Since we are in the process of sending the reply,
		 * we can't release the msg_mbuf yet.  We just pretend
		 * that we received the ACK for this reply (by setting
		 * the state to ACK_RECEIVED), the code in dux_send()
		 * will free this entry up when it is finishes.
		 */
		serving_entry->state = ACK_RECEIVED;
		/* falls through */
	case ACK_RECEIVED:
		break;

	case WAITING_ACK:
		/* if WAITING_ACK, compare req addr to net_addr from clustab
		** since reply has already been sent using msg_mbuf.
		*/
		if (eq_address(serving_entry->req_address,
			       clustab[crashed_site].net_addr))
		{
#if defined(__hp9000s700) || defined(__hp9000s800)
		    untimeout(release_reply_msg, serving_entry);
#endif
		    clean_s_entry(serving_entry);
		}
		break;

	case RECVING_REQUEST:
		if (DM_SOURCE(serving_entry->msg_mbuf) == crashed_site)
		    clean_s_entry(serving_entry);
		break;

	case SERVING:
		/* When clean up operation runs, the serving site should have
		** completed serving or aborted the requests originated from the
		** crashed site.  The entries in the SERVING
		** state should have been turned into SENDING_REPLY or
		** WAITING_ACK state except when  the crashed site is rebooted
		** and wants to join cluster  before the serving site complets
		** clean up. These newly arrived clustering requests will be put
		** to sleep until the clean up	operation is done.  The
		** serving_entry for these newly arrived clustering requests
		** should be left alone.
		*/
	case NOT_ACTIVE:
		break;
	default:
		panic("serving array in unknown state");
	    } /* END of switch */
	}

    /* return 0 so cleanup knows we're successful */
    return(0);
}


/*
**			CLEANUP_USING_ARRAY
**
** Using array cleanup is now done prior to killing of all nsp's acting
** on behalf of a site.	 This is done so that nsps won't be sleeping
** uninterruptably in dm_wait().
*/

cleanup_using_array(crashed_site)
site_t crashed_site;
{
    register int index;
    register int s;
    register struct dm_header *dmp;
    register struct using_entry *using_entry;
    using_entry = using_array;

    for (index = 0; index < using_array_size; index++, using_entry++)
    {
	/*
	 * We have to block out interrupts here because a message could
	 * come in and free the using entry out from under us.  This could
	 * happen if it is a multi-site message and we're just waiting for
	 * a client other than the dead client to reply.  If it does reply
	 * at the wrong time, the using entry can be freed and our pointers
	 * are invalid.  This has actually happened to a customer. cwb
	 */
	s = spl6();
	if (using_entry->flags & U_USED)
	{
	    dmp = DM_HEADER(using_entry->req_mbuf);
	    if (dmp->dm_dest == crashed_site ||
		dmp->dm_dest == DM_CLUSTERCAST ||
		dmp->dm_dest == DM_MULTISITE)
		clean_u_entry(using_entry, crashed_site);
	}
	splx(s);
    }

    /* return 0 so cleanup knows we're successful */
    return(0);
}


/*
**			  CLEAN_S_ENTRY
**
** clean up a serving array entry
*/

clean_s_entry(serving_entry)
struct serving_entry *serving_entry;
{
    register int s;

    switch (serving_entry->state)
    {
case WAITING_ACK:
case ACK_RECEIVED:
	untimeout(retry_reply, serving_entry);
#ifdef __hp9000s300
	if (serving_entry->flags & S_IN_DUXQ)
	    remove_from_duxQ(serving_entry->msg_mbuf);
	dm_release(serving_entry->msg_mbuf, 1);
 	s = CRIT_arrays();
	serving_entry->flags = 0;
	UNCRIT_arrays(s);
#endif
#if defined(__hp9000s700) || defined(__hp9000s800)
	release_reply_msg(serving_entry);
#endif
	break;

case  RECVING_REQUEST:
	dm_release(serving_entry->msg_mbuf, 1);
 	s = CRIT_arrays();
	serving_entry->flags = 0;
	UNCRIT_arrays(s);
	break;

default:
	printf("clean_s_entry: serving array in unknown state %d\n",
	    serving_entry->state);
	panic("clean_s_entry: serving array in unknown state");
    } /* END of switch */
}


/*
**				CLEAN_U_ENTRY
**
** cleanup a using array entry
*/

clean_u_entry(using_entry, crashed_site)
struct using_entry *using_entry;
site_t crashed_site;
{
    struct dm_header *dmp = DM_HEADER(using_entry->req_mbuf);
    dm_message req_mbuf, rep_mbuf;
    int stop_retry, end_of_duxrequest;
    u_char status;
    int s;
    struct dm_multisite *dest_list;

    stop_retry = end_of_duxrequest = FALSE;
    req_mbuf = using_entry->req_mbuf;
    rep_mbuf = using_entry->rep_mbuf;

    if (dmp->dm_dest == DM_MULTISITE || dmp->dm_dest == DM_CLUSTERCAST)
    {
	dest_list = ((struct dm_multisite *)
		     ((DM_BUF(rep_mbuf))->b_un.b_addr));

	if (dest_list->dm_sites[crashed_site].site_is_valid)
	{
	    status = dest_list->dm_sites[crashed_site].status;
	    if (status == DM_MULTI_SLOWOP || status == DM_MULTI_NORESP)
	    {
		dest_list->dm_sites[crashed_site].status = DM_MULTI_DONE;

		/*
		** Only set the rc for this site to cannot deliver, leave
		** the eosys, acflag, and tflags alone. Save the highest
		** rc in site 0's rc.  (site 0 is reserved)
		*/
		dest_list->dm_sites[crashed_site].rc = DM_CANNOT_DELIVER;
		if (dest_list->dm_sites[0].rc == 0)
		{
		    dest_list->dm_sites[0].rc = DM_CANNOT_DELIVER; /* 5 */
		}

		s = CRIT_arrays();
		dest_list->remain_sites --;
		if (status == DM_MULTI_NORESP)
		    dest_list->no_response_sites--;
		UNCRIT_arrays(s);

		if (status == DM_MULTI_NORESP &&
		    dest_list->no_response_sites == 0)
		{
		    /*All destinations have either replied or ack'ed the request
		    ** or crashed.  Stop retry request.
		    */
		    stop_retry = TRUE;
		}

		if (dest_list->remain_sites == 0)
		{
		    /*if all destinations have either replied or crashed*/

		    DM_RETURN_CODE(rep_mbuf) = dest_list->dm_sites[0].rc;
		    DM_EOSYS(rep_mbuf) = dest_list->dm_sites[0].eosys;
		    DM_TFLAGS(rep_mbuf) = dest_list->dm_sites[0].tflags;
		    DM_ACFLAG(rep_mbuf) = dest_list->dm_sites[0].acflag;

		    stop_retry = TRUE; /* must stop retry on this site */
		    end_of_duxrequest = TRUE;
		}
	    }
	    else
	    {
		/*status should be == DM_MULTI_DONE: */
		if (status != DM_MULTI_DONE)
		    panic("using_array in unknown state");

		/* already received reply from the crashed site before
		** crash is detected.
		** No clean up needs to be done !
		*/
	    }
	} /*end if site is valid*/
    }
    else
    {
	/*destination is a single site */
	protocol_cannot_deliver(rep_mbuf, DM_CANNOT_DELIVER);
	stop_retry = TRUE;
	end_of_duxrequest = TRUE;
    }

    if (stop_retry)
    {
	untimeout(retry_request, using_entry->rid);
#ifdef __hp9000s300
	if (using_entry->flags & U_IN_DUXQ)
	{
	    using_entry->flags &= (~U_IN_DUXQ);
	    remove_from_duxQ(using_entry->req_mbuf);
	}
#endif
    }

    if (end_of_duxrequest)
    {
	/*
	** Release using_array entry, and return result
	** Clear protocol window flag before calling free_using_entry()
	*/
	using_entry->flags &= ~U_OUTSIDE_WINDOW;
	free_using_entry(using_entry);
	dm_recv_reply(req_mbuf, rep_mbuf);
    }
}


/*
**			INIT_STATIC_MBUF
**
** This function is used by the protocol (nak), crash detection and
** broadcast_failure code to initialize static mbufs that
** are reused and not allocated by calling dm_alloc.
** These are special circumstances, deemed special enough that
** if we are going to utilize the datagram interface we shouldn't
** be waiting for mbufs.
*/

init_static_mbuf(nbytes, mb)
register int nbytes;
register struct mbuf *mb;
{
    register struct dm_header *hp;

    if (mb == NULL)
    {
	return(-1);
    }

    mb->m_type = MT_DATA;
    mb->m_next = NULL;
    mb->m_off = MMINOFF;
    mb->m_act = 0;
    mb->m_len = nbytes + DM_HEADERLENGTH;

    hp = DM_HEADER(mb);
    hp->dm_flags = hp->dm_tflags = 0;
    hp->dm_headerlen = nbytes + sizeof(struct dm_transmit);
    hp->dm_bufp = NULL;
    hp->dm_datalen = 0;
    hp->dm_bufoffset = 0;
    hp->dm_mid = -1;
    hp->dm_srcsite = my_site;
    hp->dm_ph.p_flags = 0;

    return(0);
}


#if defined(__hp9000s700) || defined(__hp9000s800)
		/* 800 only code to interface to lan driver. */
/*
**				FREE_DUX_BP
**
** Free dux buffer pointer
** This routine is called to clear b_flags2 field in buffer so it can be freed.
** This routine is "attached" to the last mbuf of a buffer.  This routine is
** called when m_free is called after the packet has been sent.
** This routine also decrements the "large_dux_pkt_inQ" variable, used to
** limit the amount of packets we drop due to overrunning the lan output queue.
*/
free_dux_bp(bp)
struct buf *bp;
{
    int s = CRIT();

    large_dux_pkt_inQ--;
    bp->b_flags2 &= ~B2_DUXFLAGS;
#ifdef DUX_LOG
    log_pd(PD_FREE_DUX_BP, 0, 0, bp, bp->b_flags2 & B2_DUXFLAGS, 0);
#endif
    UNCRIT(s);
}


/*
**				  DUX_NULL
**
** Book keeping routine for large packets.  This routine is called to decrement
** the "large_dux_pkt_inQ" variable, used to limit the amount of packets we drop
** due to overrunning the lan output queue.  This routine is "attached" to
** packets that point to buf's (large packets).	 It is attached to all such
** packets, except the last packet, where we attach "free_dux_bp()".
*/
/*ARGSUSED*/
dux_null(bp)
struct buf *bp;
{
    int s = CRIT();
    large_dux_pkt_inQ--;
    UNCRIT(s);
}


/*
**				 HW_SEND
**
** Send packet to lan driver.  This routine sends one packet, allocating mbuf's
** and pointing into buffers as appropriate.  "dmp" points to the dux message.
** "frame_len" is the length of this packet.
*/

hw_send(dmp, frame_len)
struct dm_header *dmp;
int frame_len;
{
    struct proto_header *ph;
    int dm_length, data_length;
    int byte_sent;
    struct mbuf *mb;
    struct mbuf *m, *m_last;
    struct sockaddr dst;
    int error;
    int addr;
    int large_buf = 0;
    int len = 0;
    int oldlevel;
    extern struct mbuf *mclget_dux();
    sv_sema_t ss;

    ph = &(dmp->dm_ph);
    byte_sent = ph->p_byte_no;
    /* if an NAK message for flow control or an ALIVE message for
    ** crash detection, fill up to the minimal packet length.
    ** These message's type are not struct dm_message.
    */
    if (ph->p_flags & P_NAK || ph->p_flags & P_ALIVE_MSG)
    {
	mb = pkt_alloc(frame_len);
	if (mb == NULL)
	{
	    if (ph->p_flags & P_ALIVE_MSG)
		STATS_reply_alive_nobuf++;
	    return(0);	/* just retry later */
	}

	copy_pkt_data((u_char *)dmp, mb, 0, frame_len);
    }
    else
    {
	/*other kinds of DUX messages whose types are struct dm_message*/
	/* copy dmmsg if any */
	dm_length = MINIMUM2(ph->p_dmmsg_length - byte_sent,
			     frame_len - PROTO_LENGTH);
	mb = pkt_alloc(dm_length + PROTO_LENGTH);
	if (mb == NULL)
	    return(ENOBUFS);
	copy_pkt_data((u_char *)dmp, mb, 0, PROTO_LENGTH);
	if (dm_length <= 0)
	{
	    dm_length = 0;
	}
	else
	{
	    /* there is dmmsg to be sent in this packet */
	    copy_pkt_data((u_char *) dmp + XMIT_OFFSET + byte_sent, mb,
			  PROTO_LENGTH, dm_length);
	    byte_sent += dm_length;
	}

	/* copy data if any */
	data_length=MINIMUM2(ph->p_data_length + ph->p_dmmsg_length -
			     byte_sent, frame_len - PROTO_LENGTH - dm_length);
	if (data_length > 0)
	{
	    m_last = mb;
	    while (m_last->m_next)
		m_last = m_last->m_next;

	    addr = (int)(BUF_ADDR(dmp) + (byte_sent - ph->p_dmmsg_length));
	    /*
	    ** Link the data buffer directly into mbuf chains
	    ** to avoid double buffering.  The buffer cannot
	    ** be released until the entire message has been
	    ** written into LAN card buffer.
	    */
	    if (ph->p_flags & P_END_OF_MSG)
		m = mclget_dux(free_dux_bp, dmp->dm_bufp, addr, data_length,
			       M_NOWAIT);
	    else
		m = mclget_dux(dux_null, dmp->dm_bufp, addr, data_length,
			       M_NOWAIT);

#ifdef DUX_LOG
	    log_pd(PD_MCLGETX, ph->p_req_index, ph->p_rid, dmp->dm_bufp,
		   dmp->dm_bufp->b_flags2 & B2_DUXFLAGS, 0);
#endif
	    if (m)
	    {
		m_last->m_next = m;
		m->m_next = NULL;
		large_buf = 1;
	    }
	    else
	    {
		m_free(mb);
		return(ENOBUFS);
	    }

	    /*
	    ** If the buffer is not in KERNELSPACE, mark
	    ** the mbuf so.
	    */
	    if (dmp->dm_bufp->b_spaddr != KERNELSPACE)
	    {
		m->m_sid = bvtospace(dmp->dm_bufp,
				     dmp->dm_bufp->b_un.b_addr);
		m->m_sid_off = m->m_off;
		m->m_off = 1024; /* so that M_HASCL is true */
	    }
	}
	ph = mtod(mb, struct proto_header *) ; /* ph is changed */
	ph->p_data_offset = dm_length;
    }

    m = mb;
    while (m)
    {
	len += m->m_len;
	m = m->m_next;
    }
    ph->iheader.length = len - 14;
    dst.sa_family = AF_UNSPEC;
    /*
    ** On the 700's this was causing the processor priority to drop to
    ** SPLNET which is spl2.  Since this is called by dux_hw_send() which is
    ** called by dux_send() where the call to dux_hw_send() is made at spl5(),
    ** the dropping of the priority can cause the reply to a request to come
    ** in before dux_hw_send expects it.  This can cause a segmentation
    ** violation as dux_hw_send() trys to find the next destination in a
    ** multi site operation and the using entry has already been freed.
    ** This is the fix that Sunil Chitgopekar, Duncan Missimier and I decided
    ** upon.  CWB
    ** NETISR_MP_GO_EXCLUSIVE(oldlevel, ss); */
    error = (*dux_lan_cards[dmp->dm_lan_card]->if_output)
				(dux_lan_cards[dmp->dm_lan_card], mb, &dst);
    /* NETISR_MP_GO_UNEXCLUSIVE(oldlevel, ss); */

    if (error == 0 && large_buf)
	large_dux_pkt_inQ++;

    if (error && dmp->dm_lan_card) printf("hw_send: error = %d\n", error);

    return (error);
}


/*
**				PKT_ALLOC
**
** Get mbuf or mcluster as needed, based upon "len" argument
**/

struct mbuf *
pkt_alloc(len)
int len;
{
    register struct mbuf *mb,*mc = NULL;
    register int len2 =0;
    extern struct mbuf *m_clget();

    if (len > LAN_PACKET_SIZE)	/* too big */
	return((struct mbuf *)NULL);
    if (len <= MLEN)
	mb = m_getclr(M_DONTWAIT, MT_HEADER);
    else if (len <= ETHERMTU)
	mb = m_clget(M_DONTWAIT, MT_HEADER);
    else if (len <= (ETHERMTU+MLEN))
    {
	mb = m_getclr(M_DONTWAIT, MT_HEADER);
	if (mb)
	{
	    mc = m_clget(M_DONTWAIT, MT_HEADER);
	    if (mc)
	    {
		len2 = len - MLEN;
		len = MLEN;
	    }
	    else
	    {
		(void) m_free(mb);
		mb = (struct mbuf *) NULL;
	    }
	}
    }
    else
    {
	mb = m_clget(M_DONTWAIT, MT_HEADER);
	if (mb)
	{
	    mc = m_clget(M_DONTWAIT, MT_HEADER);
	    if (mc)
	    {
		len2 = len - ETHERMTU;
		len = ETHERMTU;
	    }
	    else
	    {
		(void) m_free(mb);
		mb = NULL;
	    }
	}
    }
    if (mb)
	mb->m_len = len;
    else
	STATS_pkt_alloc_nobuf++;

    if (mc)
    {
	mc->m_len = len2;
	mb->m_next = mc;
	mc->m_next = NULL;
    }
    else if (mb)
	mb->m_next = NULL;

    return(mb);
}


/*
**				COPY_PKT_DATA
**
** Copy packet data from arbitrary structures to mbuf/mcluster
*/

copy_pkt_data(from, to, offset, len)
u_char *from;
struct mbuf *to;
int offset, len;
{
    int byte_copy, msize;
    struct mbuf *mb;

    mb = to;
    /* get the size. Could be mbuf or mcluster */
    msize = (M_HASCL(mb))? mb->m_clsize : MLEN;
    /* find the starting point */
    while (msize <= offset)
    {
	offset -= msize;
	mb = mb->m_next;
	msize = (M_HASCL(mb))? mb->m_clsize : MLEN;
    }
    /* continue till no more data to copy */
    while (len)
    {
	/* determine the size and start data copy */
	byte_copy = ((offset+len) > msize)? (msize - offset) : len;
	bcopy(from, mtod(mb, u_char *)+offset, byte_copy);

	/* m_len field has been set in pkt_alloc() */
	/* set m_len to byte_copy for the first copy */
	if (offset == 0)
	    mb->m_len  = byte_copy;
	else
	    mb->m_len += byte_copy;

	/* see how many left */
	len -= byte_copy;
	/*
	** if there are more data to copy, find the next one
	** and ensure next copy starts from the beginning
	*/
	if (len)
	{
	    offset = 0;
	    mb = mb->m_next;
	    msize = (M_HASCL(mb))? mb->m_clsize : MLEN;
	    from += byte_copy;
	}
    }
}


/*
**				COPYIN_FRAME
**
**  Copy a inbound packet into system buffer
**
**  TO BE DONE:
**	should fix it so that it can copy the dux message header and
**	file data into dux mbuf and file system buffer at the same time
**
**	----- csl 10/20/88
*/

int
copyin_frame(hwvars, dest, space)
register hw_gloptr hwvars;
register u_char * dest;
space_t space;
{
    struct mbuf *mb;
    int accum_size, skip, size;
    u_char *src;

    accum_size = 0;
    mb = hwvars->rxring;
    while (mb)
    {
	skip = accum_size ? 0 : PROTO_LENGTH;
	size = mb->m_len - skip;
	src = mtod(mb, u_char *) + skip;
		/* TEMPORARY FIX UNTIL REGION KERNEL */
	privlbcopy(KERNELSPACE, src, space, dest+accum_size, size);
	accum_size += size;
	mb = mb->m_next;
    }
    skip_frame(hwvars);
    return(accum_size);
}


/*
**				RE_ALIGN
**
** Realign packet data to be on word boundary
*/

#ifdef __hp9000s700
#define re_align(x)	{ /* not needed on s700 */ }
#else
re_align(mb)
struct mbuf *mb;
{
    u_char *data;
    register int off;

    data = mtod(mb, u_char *);
    off = ((int) data) & 0x03;
    if (off)
    {
	copy_network_data(data, (u_char *) data-off, mb->m_len);
	mb->m_off -= off;
    }
}
#endif /* __hp9000s700 */


/*
**				    DPINTR
**
**	DUX message interrupt handler
*/

/*ARGSUSED*/
dpintr(ifp, m, logged_info)
struct ifnet *ifp;
struct mbuf *m;
int logged_info;
{
    hw_gloptr hwvars;
    int s;
    extern int netisr_initialized;

    if (m == (struct mbuf*)0)
    {
	return;
    }
#ifdef	MP
    if (uniprocessor || ! netisr_initialized)
    {
#endif
	re_align(m);
	hwvars = Myhw;
	hwvars->rxring = m;
	dux_recv_routines(hwvars);
#ifdef	MP
    }
    else
    {
	s = splimp();
	if (IF_QFULL(&dpintrq))
	{
	    IF_DROP(&dpintrq);
	    m_freem(m);
	}
	else
	{
	    IF_ENQUEUE(&dpintrq, m);
	}
	(void) splx(s);
	schednetisr(NETISR_DUX);
    }
#endif /* MP */
}


/*
**				MPDUXINTR
**
** MP version of dpintr.  On MP machines, dpintr queues up packets
** and we dequeue them and call dux_recv_routines.
*/

mpduxintr()
{
    hw_gloptr hwvars;
    int s;
    struct mbuf *m;

    for(;;)
    {
	s = splimp();
	IF_DEQUEUE(&dpintrq, m);
	(void) splx(s);
	if (m == (struct mbuf *)0)
	    return;
	re_align(m);
	hwvars = Myhw;
	hwvars->rxring = m;
	dux_recv_routines(hwvars);
    }
}

/*
**			    DUX_LANQ_NEARLY_FULL
**
** Check LAN driver's queue length to prevent sender's queue length
** gets too long which can cause sever packet loss on the client
*/

dux_lanQ_nearly_full()
{
    int s = splimp();

    /* check lan driver's queue length */
    if (large_dux_pkt_inQ > max_large_dux_pkt_inQ)
    {
	splx(s);
	return(1);
    }
    else
    {
	splx(s);
	return(0);
    }
}
#endif /* s700, s800 */


int req_pkt_seqno = 1;	/* seq. no for first packet we transmit */
int seq_discard = 0;	/* statistics */

/*
 *	DUX was originally designed to work on a single LAN
 *	segment incapable of generating duplicate packets.
 *	The following is an attempt to beef up the protocol
 *	to handle a noisier environment. Each individual
 *	packet transmission is given an increasing serial
 *	number. Only a packet with a serial number higher
 *	than the last received from that site will be
 *	accepted. Packets arriving out of order or duplicated
 *	will be dropped in recv_request. The serial number
 *	"req_pkt_seqno" is a signed integer incremented in
 *	"dux_hw_send". The value of 0 is excluded so it can
 *	be used to mark a site whose sequence number is unknown.
 *
 *	IMPORTANT: No assumptions can be made as to the first sequence
 *	number which will be seen coming from a given client.
 *	Example: Node A is a cluster client but a server for a
 *	locally mounted file system. Node A can reboot at any time
 *	without bringing down the cluster. When A comes back up the
 *	clients' sequence numbers could be anything except 0.
 *
 *	Resetting the sequencer -
 *	1) At the server's end
 *		When a client comes up, add_site sets the
 *		sequence number for that site to unknown (0).
 *		When a server first boots this array is all zeros.
 *
 *	2) At the client end
 *		"req_pkt_seqno" is reset to 1 on boot. Any number
 *		other than 0 could be used.
 *
 *	Cluster requests have a site number of 0 and are
 *	always allowed through.
 *
 *	wraparound: because sequence numbers are signed,
 *	the "new > old" test works even as the two wrap,
 *	as long as the difference is smaller than 2^31-1.
 *
 */

#ifdef	BAD
#undef	BAD
#endif

#ifdef	OK
#undef	OK
#endif

#define BAD 1
#define OK 0

/*
**				   BADSEQ
**
** Check for bad sequence number in packet, reject packet if the
** sequence number is bad.
*/

badseq(ph) struct proto_header *ph;
{
    extern struct cct clustab[];
    int seq = ph->p_seqno;
    int site = ph->p_srcsite;

    if (ph->p_version != P_VERS_8_0)
    {
	u_char *bp = ph->iheader.sourceaddr;

	printf("bad DUX protocol version from ");
	printf("%02x%02x%02x%02x%02x%02x\n",
	       *bp, *(bp+1), *(bp+2),
	       *(bp+3), *(bp+4), *(bp+5));
	return BAD;
    }

    /*
    **	normal case
    */
    if ((seq - clustab[site].site_seqno) > 0)
    {
	clustab[site].site_seqno = seq;
	return OK;
    }

    /*
    **	cluster request - let all these through.
    **	no real danger (we think).
    */
    if (site == 0)
    {
	return OK;
    }

    /*
    **	first non-cluster request from "site"; initialize sequence
    */
    if (clustab[site].site_seqno == 0)
    {
	clustab[site].site_seqno = seq;
	return OK;
    }

    /*
    **	packet out of order
    */
    seq_discard++;
    return BAD;
}

#ifdef DUX_MULTIPLE_LAN_CARDS

#define SITE_ON_SAME_SUBNET(site)	(!clustab[site].lan_card)

dux_assign_address(site, ph, dmp)
    site_t site;
    register struct proto_header *ph;
    struct dm_header *dmp;
{
#ifdef MULTI_LAN_CARDS
    /* See if this site is on our local subnet */
    if (IM_SERVER) {
#endif
    	DUX_COPY_ADDRESS(clustab[site].net_addr, ph->iheader.destaddr);

	/* Make sure it is not forwarded */
	ph->p_flags &= ~P_FORWARD;
        dmp->dm_lan_card = clustab[site].lan_card;
#ifdef MULTI_LAN_CARDS
    }
    else if (SITE_ON_SAME_SUBNET(site)) {
    	DUX_COPY_ADDRESS(clustab[site].net_addr, ph->iheader.destaddr);

	/* Make sure it is not forwarded */
	ph->p_flags &= ~P_FORWARD;
        dmp->dm_lan_card = 0;  /* Only one lan card allowed on clients */
    }
    else {
	DUX_COPY_ADDRESS(clustab[root_site].net_addr, ph->iheader.destaddr);

	/* Make sure it is forwarded */
	ph->p_flags |= P_FORWARD;
	ph->real_dest = site;
        dmp->dm_lan_card = clustab[root_site].lan_card;
    }
#endif
}
#endif

int get_lan_card(addr)
	u_char *addr;
{
#ifdef __hp9000s300
	return(0);
#else
	int i;
	extern u_char *get_lan_addr();

	for (i = 0; i < num_dux_lan_cards; i++) {
		if (bcmp(get_lan_addr(i), addr, ADDRESS_SIZE) == 0) {
			return(i);
		}
	}
	printf("request addr= 0x%x%x\n", *(int*)addr, *(int*)(addr+4));
	printf("lan card addr= 0x%x%x\n", *(int*)get_lan_addr(0), *(int*)(get_lan_addr(0)+4));
	printf("Could not find lan card for request\n");
	panic("SDFSF");
	/* 0 will always be right if there are not multiple lan cards */
	return(0);
#endif
}

#if defined(__hp9000s700) || defined(__hp9000s800)
#define register

forward_msg(hwvars)
register hw_gloptr hwvars;
{
    struct mbuf *mb = hwvars->rxring;
    struct dm_header *dmp =
	    (struct dm_header *) READ_ADDR(mb);
    register struct proto_header *ph = &(dmp->dm_ph);
    struct sockaddr dst;
	struct mbuf * pkt_alloc();
    register struct dm_header *dmp2;
    register struct proto_header *ph2;
    struct mbuf *mb2;
    int length;
    int error;

    if (!IM_SERVER) {
#ifdef OSDEBUG
	printf("dm_header = 0x%x\n", dmp);
	panic("forward_msg: Received P_FORWARD message and not root server");
#endif
	return(0);
    }
#ifdef OSDEBUG
    if (ph->real_dest > MAXSITE) {

	printf("dm_header = 0x%x real_dest = %d\n", dmp, ph->real_dest);
	panic("Bad real destination on forwarded message");
    }
#endif

    /* Remember length does not count the addresses and length field itself */
    /*
    mb2 = pkt_alloc(ph->iheader.length+14);
    if (mb2 == NULL) {
	return 0;
    }
    */
    mb2 = mb;
    dmp2 = (struct dm_header *) READ_ADDR(mb2);
    ph2 = &(dmp2->dm_ph);

    /*
    copy_dux_mbuf_data(mb, mb2);
    */

    /* Free the old message */
    /*
    skip_frame(hwvars);
    */

    /*
     * Assign the real address.  Clears P_FORWARD bit.
     */
    DUX_ASSIGN_ADDRESS(ph2->real_dest, ph2, dmp2);

    /* Push it out on the correct lan card */
    dst.sa_family = AF_UNSPEC;
    error = (*dux_lan_cards[dmp2->dm_lan_card]->if_output)
			(dux_lan_cards[dmp2->dm_lan_card], mb2, &dst);

    return(1);
}

copy_dux_mbuf_data(from, to)
	register struct mbuf *from;
	register struct mbuf *to;
{
    register int len;
    register u_char *addr;
#ifdef OSDEBUG
    struct mbuf *save = from;
#endif

    /* to buffer is big enough to hold all data from "from" */
    addr = mtod(to, u_char *);

    /* continue till no more data to copy */
    while (from)
    {
    	len = from->m_len;
	bcopy(mtod(from, u_char *), addr, len);
	addr += len;
	from = from->m_act;
    }

#ifdef OSDEBUG
    if ((addr - mtod(to, u_char *)) != save->m_len) {
	printf("addr - mtod(to, u_char *) = %d, save->m_len = %d",
			addr - mtod(to, u_char *), save->m_len);
	panic("copy_dux_mbuf_data: Overwrote buffer");
    }
#endif
}

#endif /* defined(__hp9000s700) || defined(__hp9000s800) */
