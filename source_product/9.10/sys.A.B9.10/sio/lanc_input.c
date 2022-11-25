/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/lanc_input.c,v $
 * $Revision: 1.12.83.4 $        $Author: rpc $
 * $State: Exp $        $Locker:  $
 * $Date: 93/10/20 13:34:59 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 
#if defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) lanc_input.c $Revision: 1.12.83.4 $ $Date: 93/10/20 13:34:59 $";
#endif


#include "../h/buf.h"
#include "../h/protosw.h"   
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/mbuf.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"

#include "../net/if.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/if_ieee.h"

#include "../sio/netio.h"
#include "../sio/lanc.h"
#include "../sio/lla.h"

#ifdef LAN_MS 
/*** measurement instrumentation definitions ***/
#include "../h/ms_macros.h"

struct timeval ms_gettimeofday();

int	ms_lan_read_queue = -2;
int	ms_lan_write_queue = -2;
int	ms_lan_netisr_recv = -2;
int	ms_lan_proto_return = -2;
int	ms_lan_write_complete = -2;
static struct {
	       struct timeval tv;
	       int	count;
	       int	unit;
	       int	protocol;
	       int	addr;
	      } ms_io_info;
#endif /* LAN_MS */

/*
 *	These defines are undocumented features of LLA, they are primarily
 *	used for HP internal protocol development.  That is why they are
 *	defined here instead of netio.h.  This compatible with the S300
 *	implementation which serves DUX remote boot server.
 */

#define	LOG_XSSAP	801
#define	LOG_XDSAP	802
#define	LOG_SNAP_TYPE	803


/*
 *	External function calls.
 */

extern	int	lanc_log_protocol();
extern	int	lanc_lla_input();
extern struct logged_link *lanc_lookup();

extern	int	lanc_ctl_sleep();

extern int	ms_lan_proto_return;


/* BEGIN_IMS lanc_llc_control
***************************************************************************
****
****                        lanc_llc_control
****
***************************************************************************
* Description
*       Handles any changes to the llc portion of the packet.   This
*       includes manipulation of D/SSAPs, CTL and extended header fields.
*
* Packet format:
*       Routine can handle 802.3 (ieee, extended, snap)
*                          802.5 (ieee, snap)
*
* Input Parameters  
*       lla_ptr       - pointer to the lla structure 
*       data_ptr      - pointer to buffer containing data information from
*                       the user.
*
*  lanc_llc_control determines the particular field to be updated, as well 
*  as acceptable conditions for change.   It may also need to determine the
*  particular link (802.3, 802.5 etc) which this change is to occur for. 
*
*
* Return Value
*        None.
*
* Algorithm
*                
* External Calls
*
* Called By
*
* To Do List
*
* Notes
*
* Modification History
*  When      Who  Description
*  ========  ===  ============================================================
*
***************************************************************************
* END_IMS lanc_llc_control */

lanc_llc_control(lla_ptr, data_ptr)
register struct lla_cb	*lla_ptr;	/* lla_control block */
register struct fis	*data_ptr;	/* user data area */
   
{
   int error = 0;             	/* error returns */
   caddr_t     mac_rif_ptr;

   LAN_LOCK(lla_ptr->lla_lock);
   switch(data_ptr->reqtype) {

   case LOG_XDSAP:
	if ((data_ptr->value.i < 0) || (data_ptr->value.i > 0xffff) ||
	    (lla_ptr->packet_header.proto.ieee.ssap != IEEESAP_NM) ||
            (lla_ptr->lla_flags & LLA_RAW_802MAC)) {
	     	error=EINVAL;
		break;
	}
	lla_ptr->packet_header.proto.ieee.dxsap = data_ptr->value.i;
	break;

   case LOG_XSSAP:
	if ((data_ptr->value.i < 0) || (lla_ptr->lla_flags & LLA_PROT_LOGGED) ||
	    (data_ptr->value.i > 0xffff) ||
            (lla_ptr->lla_flags & LLA_RAW_802MAC)) {
		error=EINVAL;
		break;
	}
	LAN_UNLOCK(lla_ptr->lla_lock);
	error = lanc_log_protocol((int)LAN_CANON, (int)data_ptr->value.i,
                  lanc_lla_input, lla_ptr->lla_ifp, 0, 1, 0, 0,
		  (int) lla_ptr, 0);
	LAN_LOCK(lla_ptr->lla_lock);
	if (error) {
	 	break;
	}
	lla_ptr->packet_header.proto.ieee.ssap = IEEESAP_NM;
	lla_ptr->packet_header.proto.ieee.dsap = IEEESAP_NM;
	lla_ptr->packet_header.proto.ieee.sxsap = data_ptr->value.i;
	lla_ptr->packet_header.proto.ieee.dxsap = data_ptr->value.i;
	lla_ptr->packet_header.proto.ieee.ctrl = (u_char) UI_CONTROL;
	lla_ptr->hdr_size = IEEE8023XSAP_HLEN;
	 
	lla_ptr->lla_flags |= LLA_PROT_LOGGED;
	break;

   case LOG_RAW_802MAC:
	if (lla_ptr->lla_flags & LLA_PROT_LOGGED) {
		error=EINVAL;
		break;
	}
/*
 *      For 802.3, set the hdr_size to be the Ethernet Header Size since
 *      the raw 802.3 header size is the same as the Ethernet header size
 */

        lla_ptr->hdr_size = IEEE_RAW8023_HLEN;
        lla_ptr->lla_flags |= LLA_RAW_802MAC;
        break;

   case LOG_CONTROL:
	if (((data_ptr->value.i != UI_CONTROL) &&
	    ((data_ptr->value.i | PF_BITMASK) != XID_CONTROL) &&
	    ((data_ptr->value.i | PF_BITMASK) != TEST_CONTROL)) ||
            (lla_ptr->lla_flags & LLA_RAW_802MAC) ||
            (!(lla_ptr->lla_flags & LLA_PROT_LOGGED))) {
		error=EINVAL;
		break;
	}

        switch(lla_ptr->lla_ifp->ac_type) {
        
        case ACT_8023:
	     lla_ptr->packet_header.proto.ieee.ctrl = (u_char)data_ptr->value.i;
             break;

        case ACT_8025:
             if (lla_ptr->packet_header.proto.snap8025.sourceaddr[0] & 
                 IEEE8025_RII){
             /* compute 802.2 head offset for 802.2 header */

                  mac_rif_ptr = (caddr_t)(&lla_ptr->packet_header.proto.
			snap8025.rif_plus[lla_ptr->packet_header.proto.
                            snap8025.rif_plus[0] & 0x1f]);
             }
             else {
                  mac_rif_ptr = (caddr_t)(&lla_ptr->packet_header.proto.
			snap8025.rif_plus[0]);
             }
             if (lla_ptr->lla_flags & LLA_IS_SNAP8025) {
	         lla_ptr->packet_header.proto.snap8025.ctrl = 
                         (u_char)data_ptr->value.i;
                 bcopy(&lla_ptr->packet_header.proto.snap8025.dsap,
                         mac_rif_ptr, 8);
             }
             else {
	         lla_ptr->packet_header.proto.ieee8025.ctrl = 
                         (u_char)data_ptr->value.i;
                 bcopy(&lla_ptr->packet_header.proto.ieee8025.dsap,
                         mac_rif_ptr, 3);
              }
           
             break;

        default:
             error=EINVAL;
        } /* end of ac_type switch */
        break;

   case LOG_DSAP:
        if ((data_ptr->value.i < MINSAP) || (data_ptr->value.i > MAXSAP) ||
            (!(lla_ptr->lla_flags & LLA_PROT_LOGGED)) ||
            (lla_ptr->lla_flags & LLA_RAW_802MAC)) {
		error=EINVAL;
		break;
	}

        switch(lla_ptr->lla_ifp->ac_type) {
        
        case ACT_8023:
	     lla_ptr->packet_header.proto.ieee.dsap = (u_char)data_ptr->value.i;
             break;

        case ACT_8025:
             if (lla_ptr->packet_header.proto.snap8025.sourceaddr[0] & 
                 IEEE8025_RII){
             /* compute 802.2 head offset for 802.2 header */

                  mac_rif_ptr = (caddr_t)(&lla_ptr->packet_header.proto.
			snap8025.rif_plus[lla_ptr->packet_header.proto.
                            snap8025.rif_plus[0] & 0x1f]);
             }
             else {
                  mac_rif_ptr = (caddr_t)(&lla_ptr->packet_header.proto.
			snap8025.rif_plus[0]);
             }
             if (lla_ptr->lla_flags & LLA_IS_SNAP8025) {
	         lla_ptr->packet_header.proto.snap8025.dsap = 
                         (u_char)data_ptr->value.i;
                 bcopy(&lla_ptr->packet_header.proto.snap8025.dsap,
                         mac_rif_ptr, 8);
             }
             else {
	         lla_ptr->packet_header.proto.ieee8025.dsap = 
                         (u_char)data_ptr->value.i;
                 bcopy(&lla_ptr->packet_header.proto.ieee8025.dsap,
                         mac_rif_ptr, 3);
              }
           
             break;

        default:
             error=EINVAL;
             break;
        } /* end of ac_type switch */
        break;

   case LOG_SSAP:
/*
 * 	1) check if the user has already logged a ssap
 *	2) check if the user's requested ssap is within allowed range
 *	3) check if the user's requested ssap not group
 */
	if ((data_ptr->value.i <= MINSAP) || (data_ptr->value.i > MAXSAP)  ||
	    (data_ptr->value.i& 1) || (lla_ptr->lla_flags & LLA_PROT_LOGGED)) {
		error=EINVAL;
		break;
	}
	 
/*
 * 	Now log the SAP to the interface, if logging routine passes
 * 	an error, then return it to the user
 */
	LAN_UNLOCK(lla_ptr->lla_lock);
        error = lanc_log_protocol((int)LAN_SAP, (int)data_ptr->value.i,
                                  lanc_lla_input, lla_ptr->lla_ifp, 0, 1, 0,
                                  (lla_ptr->lla_flags & LLA_RAW_802MAC),
			          (int)lla_ptr, 0);
	LAN_LOCK(lla_ptr->lla_lock);
	if (error) {
		break;
	}
/*	 
 * 	Put the SSAP into the 802.2 header within the r8023_raw control block
 */
    switch(lla_ptr->lla_ifp->ac_type) {
    case ACT_8023:
	lla_ptr->packet_header.proto.ieee.ssap =(u_char) data_ptr->value.i;
	lla_ptr->packet_header.proto.ieee.dsap = (u_char) data_ptr->value.i;
	lla_ptr->packet_header.proto.ieee.ctrl =(u_char) UI_CONTROL;
        break;

    case ACT_8025:
	lla_ptr->packet_header.proto.ieee8025.access_ctl = IEEE8025_AC;
	lla_ptr->packet_header.proto.ieee8025.frame_ctl = IEEE8025_FC;
	lla_ptr->packet_header.proto.ieee8025.ssap =(u_char) data_ptr->value.i;
	lla_ptr->packet_header.proto.ieee8025.dsap = (u_char) data_ptr->value.i;
	lla_ptr->packet_header.proto.ieee8025.ctrl =(u_char) UI_CONTROL;
        lla_ptr->hdr_size = IEEE8025_SR_HLEN;  /*mac+sr+llc*/
        break;

    default:
        error=EINVAL;
    }
	if(!error) {
		lla_ptr->lla_flags |= LLA_PROT_LOGGED;
	}
	break;

   default:
	error=EINVAL;
	break;

   }	/* end of reqtype switch */

   LAN_UNLOCK(lla_ptr->lla_lock);
   return(error);
}    /* end of lanc_llc_control */


/* BEGIN_IMS lanc_802_2_ics
***************************************************************************
****
****                        lanc_802_2_ics
****
***************************************************************************
* Description
*       Inbound packet handler for 802.2 packet. This function runs on the
*       ICS. Does IEEE 802.2 specific processing, then
*       does lookup on protocol to obtain SAP specific data. Based on ICS
*       flag in this structure, either passes packet on to input processing 
*       routine or calls schednetisr to queue packet for processing by netisr.
*
* Packet format:
*       DA(6bytes) SA(6bytes) ... Filler(x bytes) ... DATA(incl 802.2 hdr)
*
* Input Parameters  
*       ifnet_ptr     - pointer to the ifnet structure for the lan unit
*                       for the card the packet came in on
*       mbuf_ptr      - pointer to buffer containing inbound packet.  The
*                       packet starts of with the Destination Address.
*       offset        - integer offset to start of 802.2 header/data
*       link_header_type -  Value indicating the format of the link level
*                           header
*
*  lanc_802.2_ics figures out the LLC protocol and identifies the llc_type
*  format.  This needs to be combined with the link_header_type to be sent
*  to the ULP.  Currently,  since there are only two link_header_type formats
*  present (802.3 and ether) and only one (802.3) of which gets to this
*  routine.  They are combined with llc_type to provide
*  one value.  This needs to be split as well as the number of values 
*  expanded.  One suggestion would be to divide the ulp_type which is 32
*  bits long into two 16 bit short fields :  the least significant short
*  will be the llc_type,  as it is currently and the most significant short
*  will consist of the link_header_type.  Currently this field is zero and
*  this can be used to represent the 8023 Mac to preserve backward
*  compatibility.
*
*     --------------------------------------
*     |  link_header_type  |  llc_type     |
*     --------------------------------------
*
* Return Value
*        None.
*
* Algorithm
*                
* External Calls
*       m_adj, m_freem, lanc_802_2_test_ctl, lanc_lookup, schednetisr
*       or input processing routine for SAP, Type or Canonical Address.
*
* Called By
*       lanx_dma_process_reply
*
* To Do List
*
* Notes
*
* Modification History
*  When      Who  Description
*  ========  ===  ============================================================
*
***************************************************************************
* END_IMS lanc_802_2_ics */

#define IEEE8022_SAP_HLEN     3      /* SAP, DSAP, CNTRL */
#define IEEE8022_XSAP_HLEN    10     /* SAP, DSAP, CNTRL,
                                        FILLER(3), XDSAP(2), XSSAP(2) */
#define IEEE8022_SNAP_HLEN    8      /* SAP, DSAP, CNTRL, FILLER(3), TYPE(2) */
#define IEEE8022_XSNAP_HLEN   12     /* SAP, DSAP, CNTRL, FILLER(3), TYPE(2)
                                        DCANONADDR(2), SCANNONADDR(2) */
lanc_802_2_ics(ifnet_ptr, mbuf_ptr, offset, link_header_type)
struct ifnet *ifnet_ptr;
struct mbuf *mbuf_ptr;
int    offset, link_header_type;
{
   /* Local variables. */

   lan_ift *lanc_ift_ptr = (lan_ift *) ifnet_ptr;
   register lan_rheader *lan;
   register struct logged_link  *link_ptr;
   int header_size, protocol_val, protocol_val_ext, protocol_kind, pkt_format;
   struct ieee8023_hdr *ie_hdr;
   struct ieee8023xsap_hdr *ie_canon_hdr;
   struct snap8023_hdr *ie_snap_hdr;
   struct snap8023xt_hdr *ie_snap_xt_hdr;
   struct ifqueue *ifq;
   struct mbuf *m_temp;
   u_short acflags;
#ifdef LAN_MS
   int ms_turned_on_flag;
#endif /* LAN_MS */

   mib_ifEntry *mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
   mib_xEntry *mib_xptr = lanc_ift_ptr->mib_xstats_ptr;

   lan = mtod (mbuf_ptr, lan_rheader *);

   LAN_LOCK(lanc_ift_ptr->mib_lock);
   if (lan->destination[0] & 0x01)
	mib_ptr->ifInNUcastPkts++ ;
   else
        mib_ptr->ifInUcastPkts++ ;
   LAN_UNLOCK(lanc_ift_ptr->mib_lock);

   ie_hdr = mtod(mbuf_ptr, struct ieee8023_hdr *);

   /* if length is < MIN_IEEE8022_HLEN we don't have a valid 802.2 header */
   /*     and we shouldn't be in here anyway; drop the sucker */
   if(ie_hdr->length < MIN_IEEE8022_HLEN) {
	goto drop;
   }

   protocol_val = ie_hdr->dsap;
   switch(protocol_val){
   case IEEESAP_HP:
   case IEEESAP_NM:
	ie_canon_hdr = mtod(mbuf_ptr, struct ieee8023xsap_hdr *);
        protocol_val = ie_canon_hdr->dxsap;
        protocol_kind = (int) LAN_CANON;
        header_size = IEEE8022_XSAP_HLEN;
        pkt_format = IEEE8023XSAP_PKT;
        break;

   case IEEESAP_SNAP:
        {
        u_char *protocol_val_ptr = (u_char *)(&protocol_val);
        ie_snap_hdr = mtod(mbuf_ptr, struct snap8023_hdr *);
        *(protocol_val_ptr + 0) = ie_snap_hdr->hdr_fill[0];
        *(protocol_val_ptr + 1) = ie_snap_hdr->hdr_fill[1];
        *(protocol_val_ptr + 2) = ie_snap_hdr->hdr_fill[2];
        *(protocol_val_ptr + 3) = ie_snap_hdr->hdr_fill[3];
        protocol_val_ext = ie_snap_hdr->hdr_fill[4] << 24;
        protocol_kind = (int) LAN_SNAP;
        header_size = SNAP8023_HLEN;
        pkt_format = SNAP8023_PKT;
        }
        break;

   default:
	protocol_kind = (int) LAN_SAP;
        header_size = IEEE8022_SAP_HLEN;
        pkt_format = IEEE8023_PKT;
        break;
   }

   AC_LOCK((struct arpcom *)ifnet_ptr);
   acflags=((struct arpcom *)ifnet_ptr)->ac_flags;
   AC_UNLOCK((struct arpcom *)ifnet_ptr);

/*
 *   	The SAP is not logged in the hash table or
 *   	A check to make sure that the interface is not logically down or
 *   	encapsulation scheme accepted does not match the incoming packet
 *      IEEE 802.3 or SNAP or ETHER
 *   	Check whether the packet is a TEST or XID packet
 */
   if (((link_ptr=lanc_lookup(protocol_val,protocol_kind,ifnet_ptr,
                              protocol_val_ext))== NULL) || 
       ((link_ptr->log.flags & LANC_CHECK_FLAGS) &&
        (!(ifnet_ptr->if_flags & IFF_UP) ||
          (((pkt_format == IEEE8023_PKT) || (pkt_format == IEEE8023XSAP_PKT)) &&
          !(acflags & ACF_IEEE8023)) ||
           (((pkt_format == SNAP8023_PKT) || (pkt_format == SNAP8023XT_PKT)) &&
           !(acflags & ACF_SNAP8023))))) {

 /* special kludge for driver TEST or XID packets */
	if ( /* always true (ieee_packet) && */
            !(ie_hdr->ssap & 1) && (ie_hdr->dsap == 0) &&
            ((ie_hdr->ctrl == IEEECTRL_XID) || 
            (ie_hdr->ctrl == (IEEECTRL_XID &~ PF_BITMASK)) ||
            (ie_hdr->ctrl == IEEECTRL_TEST) ||
            (ie_hdr->ctrl == (IEEECTRL_TEST &~ PF_BITMASK)))) {
		lanc_802_2_test_ctl(mbuf_ptr, lanc_ift_ptr);
            	return;
        }        
        /* Not logged in or appropriate flag not set*/
 
	LAN_LOCK(lanc_ift_ptr->mib_lock);
        mib_ptr->ifInUnknownProtos++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
 
	LAN_LOCK(lanc_ift_ptr->lanc_lock);
        lanc_ift_ptr->UNKNOWN_PROTO++;
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);

        NS_LOG_INFO((LE_RINT_UNLOGGED_SAP+protocol_kind), NS_LC_PROLOG,
		NS_LS_DRIVER, 0, 2, protocol_val, lanc_ift_ptr->is_if.if_unit);
        goto drop;
   }

   switch(ie_hdr->ctrl) {
   case IEEECTRL_XID:
   case (IEEECTRL_XID & ~ PF_BITMASK):
   case IEEECTRL_TEST:
   case (IEEECTRL_TEST & ~ PF_BITMASK):
   case (IEEECTRL_DEF | PF_BITMASK):
   case IEEECTRL_DEF:
   	break;

   default:
/*
 *	check to see if this is a raw port.  if it is, then pass
 *	any of these packets on to the port.
 */
 	if (!(link_ptr->log.flags & LANC_RAW_802MAC)) {
		LAN_LOCK(lanc_ift_ptr->lanc_lock);
            	lanc_ift_ptr->BAD_CONTROL++;
		LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
                NS_LOG_INFO (LE_IERINT_BAD_CTRL, NS_LC_PROLOG, NS_LS_DRIVER, 
            		     0, 1, lanc_ift_ptr->is_if.if_unit, 0);
		LAN_LOCK(lanc_ift_ptr->mib_lock);
                mib_ptr->ifInUnknownProtos++;
		LAN_UNLOCK(lanc_ift_ptr->mib_lock);
                goto drop;
        }
   }

   /* strip header if so requested in flags field */
   if (link_ptr->log.flags & LANC_STRIP_HEADER){
   	mbuf_ptr->m_off += (offset + header_size) ;
        mbuf_ptr->m_len -= (offset + header_size) ;
        /*
         *  Do we need to free the mbuf, if m_len falls to zero ??
         *  Header/Data Split for instance..
         *  if (mbuf_ptr->m_len == 0) {
         *     struct mbuf *temp;
         *     temp = mbuf_ptr;
         *     mbuf_ptr = mbuf_ptr->m_next;
         *     temp->m_next = 0;
         *     m_free(temp);
         *  }
         */
   }
   
   if (link_ptr->log.flags & LANC_ON_ICS){
        /* call protocol input routine directly if ON_ICS bit set */
   	(void)(*link_ptr->log.rint)(ifnet_ptr,mbuf_ptr,
        link_ptr->log.lu_protocol_info, pkt_format);
   } else { 
        /* if ON_ICS bit not set, then queue input processing on netisr */
        ifq = (struct ifqueue *)link_ptr->log.rint;
	IFQ_LOCK(ifq);
        if (IF_QFULL(ifq)) {
            IF_DROP(ifq);

	    IFQ_UNLOCK(ifq);

	    LAN_LOCK(lanc_ift_ptr->mib_lock);
            mib_xptr->if_DInDiscards++;
	    LAN_UNLOCK(lanc_ift_ptr->mib_lock);

            goto drop;
        }
        IF_ENQUEUEIF_NOLOCK(ifq, mbuf_ptr, ifnet_ptr);
	IFQ_UNLOCK(ifq);
        (void)schednetisr(link_ptr->log.lu_protocol_info);
   }
#ifdef LAN_MS
    /*
     * measurement instrumentation - measure time at which ON_ICS protocol
     * returned, or if _NOT_ ON_ICS, the time at which the packet was  queued 
     * on to the protocol queue and schednetisr was called.  Note that for 
     * the measurement, SNAP packets are classified as IEEE 802.3 packets.
     */
   if(ms_turned_on(ms_lan_proto_return)) {
        ms_io_info.protocol = 1; /* IEEE packet */
        ms_io_info.count = ie_hdr->length;
        /*
         *  This is not actually exact.  The driver is getting rid of the
         *  filler bytes to get to the minimum size 802.3 packet and
         *  hence the packet received by this routine already has lost the
         *  filler bytes.   We can put in a check to figure out if the
         *  tot_len is less than minimum 802.3 length and if so go for the
         *  minimum.  The additional problem is that the minimum 802.3
         *  frame length could vary for different physical implementations
         */

#define IEEE_8023_MIN_FRAME_SIZE  64  /* For 10BASE5 implementation */
        /*
         *   calculate total data bytes in packet 
         */
        tot_len = 0;
        m_temp = mbuf_ptr;
        while(m_temp){
            tot_len += m_temp->m_len;
            m_temp = m_temp->m_next;
        }
        ms_io_info.count = ( tot_len < IEEE_8023_MIN_FRAME_SIZE ) 
            ? IEEE_8023_MIN_FRAME_SIZE : tot_len;

        ms_io_info.addr = ie_hdr->dsap;
        ms_io_info.unit = ifnet_ptr->if_unit;
        ms_io_info.tv = ms_gettimeofday();
        ms_data(ms_lan_proto_return, &ms_io_info, sizeof(ms_io_info));
   }

#endif /* LAN_MS */
   return;

drop:
   m_freem(mbuf_ptr);
}


/* BEGIN_IMS lanc_802_2_test_ctl
***************************************************************************
****
****			lanc_802_2_test_ctl
****
***************************************************************************
* Description
*       This function deals with incoming IEEE 802.2 TEST or XID packets
*       that are requests (not replies).
*
* Input Parameters  
*	mbuf_ptr      - pointer to inbound packet
*       lanc_ift_ptr  - 
*
* Return Value
*	None.
*
* Algorithm
*		
* External Calls
*       bcopy, m_freem, splimp, splx, IF_QFULL, IFDROP, NS_LOG.       
*
* Called By
*	lanc_802_2_ics
*
* To Do List
*
* Notes
*
* Modification History
*  When      Who  Description
*  ========  ===  ============================================================
*
***************************************************************************
* END_IMS lanc_802_2_test_ctl */

lanc_802_2_test_ctl(mbuf_ptr, lanc_ift_ptr)
struct mbuf *mbuf_ptr;
lan_ift * lanc_ift_ptr;
{   
   /* local variables */

   register struct ieee8023_hdr *ie_hdr;
   u_char tempsap;
   struct ns_xid_info_field *xid;

   ie_hdr = mtod(mbuf_ptr, struct ieee8023_hdr *);

   /* Switch the dsap and ssap, and the destaddr and sourceaddr, 
    * then stick the packet immediately on the output queue and exit.
    */
	
   tempsap = ie_hdr->dsap;
   ie_hdr->dsap = ie_hdr->ssap;
   ie_hdr->ssap = tempsap | 1;			      /* for response */
 
   bcopy((caddr_t)ie_hdr->sourceaddr, (caddr_t)ie_hdr->destaddr,
	 LAN_PHYSADDR_SIZE);

   LAN_LOCK(lanc_ift_ptr->lanc_lock);
   /* DIO card does not put in sourceaddr; doesn't hurt others */
   bcopy((caddr_t) lanc_ift_ptr->station_addr,
         (caddr_t)ie_hdr->sourceaddr, LAN_PHYSADDR_SIZE);

  /* If an XID packet, drop the inbound info field and send back ours.    */
		
   if ((ie_hdr->ctrl | PF_BITMASK) == IEEECTRL_XID) {   /* XID pkt */
  
	/* unlock in case we need to do the m_freem */
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);

      /* Header ALWAYS short (non-xsap) since driver only returns for null  */ 
      /* dsap.                                                              */

	mbuf_ptr->m_off += IEEE8023_HLEN;
      	xid = mtod (mbuf_ptr, struct ns_xid_info_field *);
      	xid->xid_info_format = XID_IEEE_FORMAT;
      	xid->xid_info_type = XID_TYPE1_CLASS1;
      	xid->xid_info_window = XID_WINDOW_SIZE;

      	if (mbuf_ptr->m_next != 0) {  /* Free any additional buffers */
         	m_freem(mbuf_ptr->m_next);
         	mbuf_ptr->m_next = 0;
      	}

      	mbuf_ptr->m_off -= IEEE8023_HLEN;
      	mbuf_ptr->m_len  = IEEE8023_HLEN + XID_INFO_FIELD_SIZE;
      	ie_hdr->length   = mbuf_ptr->m_len - ETHER_HLEN;
	LAN_LOCK(lanc_ift_ptr->lanc_lock);
      	lanc_ift_ptr->RXD_XID++;
   } else {   /* TEST pkt */
      	lanc_ift_ptr->RXD_TEST++;
   }
   LAN_UNLOCK(lanc_ift_ptr->lanc_lock);

   return((*lanc_ift_ptr->hw_req) (lanc_ift_ptr, LAN_REQ_WRITE, 
	  			   (caddr_t)mbuf_ptr));
} /* end of lanc_802_2_test_ctl */ 


/* BEGIN_IMS lanc_ether_control
***************************************************************************
****
****                        lanc_ether_control
****
***************************************************************************
* Description
*       Handles any changes to the ether portion of the packet.   This
*       includes manipulation of Types (SNAP or Ether).
*
* Packet format:
*       Routine can handle 802.3 (snap)
*                          802.5 (snap)
*                          Ethernet
*
* Input Parameters  
*       lla_ptr       - pointer to the lla structure 
*       data_ptr      - pointer to buffer containing data information from
*                       the user.
*
*  lanc_ether_control determines the particular field to be updated, as well 
*  as acceptable conditions for change.   It may also need to determine the
*  particular link (802.3, 802.5 etc) which this change is to occur for. 
*
*
* Return Value
*        None.
*
* Algorithm
*                
* External Calls
*
* Called By
*
* To Do List
*
* Notes
*
* Modification History
*  When      Who  Description
*  ========  ===  ============================================================
*
***************************************************************************
* END_IMS lanc_ether_control */

lanc_ether_control(lla_ptr, data_ptr)
register struct lla_cb	*lla_ptr;	/* lla_control block */
register struct fis	*data_ptr;	/* user data area */
   
{
   int protocol_val;
   int protocol_val_ext = 0;
   int error = 0;             	/* error returns */
   lan_ift *lanc_ift_ptr;
   mib_ifEntry  *mib_ptr;

   LAN_LOCK(lla_ptr->lla_lock);

   switch(data_ptr->reqtype) {

   case LOG_SNAP_TYPE:
/*
 *      log the 5 bytes of SNAP info
*/
        if (lla_ptr->lla_flags & LLA_PROT_LOGGED) {
                error = EINVAL;
                break;
                }

        protocol_val = data_ptr->value.i;
        protocol_val_ext = (data_ptr->value.s[4] << 24);

	LAN_UNLOCK(lla_ptr->lla_lock);
        error = lanc_log_protocol((int)LAN_SNAP, protocol_val,
                                 lanc_lla_input, lla_ptr->lla_ifp, 0,
                                 1, 0, 0, (int)lla_ptr, protocol_val_ext);
	LAN_LOCK(lla_ptr->lla_lock);
        if (error)
                break;
	switch(lla_ptr->lla_ifp->ac_type) {
        case ACT_8023:
	    lla_ptr->packet_header.proto.ieee.dsap = IEEESAP_SNAP;
	    lla_ptr->packet_header.proto.ieee.ssap = IEEESAP_SNAP;
	    lla_ptr->packet_header.proto.ieee.ctrl = (u_char) UI_CONTROL;
            bcopy(data_ptr->value.s, lla_ptr->packet_header.proto.ieee.pad, 5);
	    lla_ptr->hdr_size = SNAP8023_HLEN;

            /* Get the mtu size from the mib info */
            lanc_ift_ptr = (lan_ift *)lla_ptr->lla_ifp;
            mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
          
            break;
    
        case ACT_8025:
	    lla_ptr->packet_header.proto.snap8025.access_ctl = IEEE8025_AC;
	    lla_ptr->packet_header.proto.snap8025.frame_ctl = IEEE8025_FC;
	    lla_ptr->packet_header.proto.snap8025.dsap = IEEESAP_SNAP;
	    lla_ptr->packet_header.proto.snap8025.ssap = IEEESAP_SNAP;
	    lla_ptr->packet_header.proto.snap8025.ctrl = (u_char) UI_CONTROL;
            bcopy(data_ptr->value.s, 
                  lla_ptr->packet_header.proto.snap8025.orgid, 5);
	    lla_ptr->hdr_size = SNAP8025_SR_HLEN;
	    lla_ptr->lla_flags |= LLA_IS_SNAP8025;

            /* Get the mtu size from the mib info */
            lanc_ift_ptr = (lan_ift *)lla_ptr->lla_ifp;
            mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
            lla_ptr->lan_pkt_size = mib_ptr->ifMtu;
            break;
    
        default:
            error = EINVAL;
        }

        if(!error) {
                lla_ptr->lla_flags |= LLA_PROT_LOGGED;
        }
        break;
	 
   case LOG_TYPE_FIELD:
/*
 *	1) check if the user has already logged a type
 *	2) check if the user's requested log_field_type is within allowed range
 *	3) make sure that user is not attempting to log a reserved type
 */
	if (lla_ptr->lla_flags & LLA_PROT_LOGGED)
		error = EINVAL;
	if ((data_ptr->value.i < MIN_ETHER_TYPE) || 
	    (data_ptr->value.i > MAXTYPE))
		error = EINVAL;
	if (error)
		break;
	 
        protocol_val= data_ptr->value.i;
/*
 * 	This code is special in that these types are used to generate Berkeley
 * 	Trailer Protocols.  By definition, a process cannot receive packets
 * 	on these log types.  We allow a super user to 'log' them so that we
 * 	can test Trailer Protocol by generating them.  Since the protocols have
 * 	the ability to write to any protocol (including IP) we are restricting
 * 	their use to super users.  Note that we need to test for this condition
 * 	at close.  Inbound trailer protocols are manipulated by the driver where
 * 	it re-assembles a trailer header to be standard.
 */
        if ((protocol_val >= ETHERTYPE_TRAIL) &&
		(protocol_val <= ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER)) {
#ifdef LAN_TEST_TRAILERS
		if(!suser()) {
			error = EBUSY;
		} else {
			error = 0;
		}
#else
		 	error = EBUSY;
#endif
	} else {
		LAN_UNLOCK(lla_ptr->lla_lock);
	     	error = lanc_log_protocol((int)LAN_TYPE, protocol_val,
					  lanc_lla_input, lla_ptr->lla_ifp, 0,
					  1, 0, 0, (int)lla_ptr, 0);
		LAN_LOCK(lla_ptr->lla_lock);
	}
	if (error)
		break;
	lla_ptr->packet_header.proto.ether.log_type =
					 (u_short)data_ptr->value.i;
	lla_ptr->lla_flags |= LLA_PROT_LOGGED;
	break;
	 
   }	/* end of reqtype switch */
   LAN_UNLOCK(lla_ptr->lla_lock);
   return(error);
}    /* end of lanc_ether_control */


/* BEGIN_IMS lanc_ether_ics
***************************************************************************
****
****            lanc_ether_ics
****
***************************************************************************
* Description
*       Inbound packet handler. This function runs on the ICS. Decodes
*       Ethernet packets. Does lookup on protocol to obtain SAP/Type specific
*       data. Based on ICS flag in this structure, either passes packet on
*       to input processing routine or calls schednetisr to queue packet
*       for processing by netisr.
*
* Packet format:
*       DA(6bytes) SA(6bytes) Type(2bytes) DATA(x bytes)
*
* Input Parameters  
*       ifnet_ptr     - pointer to the ifnet structure for the lan unit
*                       for the card the packet came in on
*       mbuf_ptr      - pointer to buffer containing inbound packet
*
* Return Value
*    None.
*
* Algorithm
*        
* External Calls
*       m_adj, m_freem, lanc_lookup, schednetisr
*       or input processing routine for SAP, Type or Canonical Address.
*
* Called By
*       lanx_dma_process_reply
*
* To Do List
*
* Notes
*
* Modification History
*  When      Who  Description
*  ========  ===  ============================================================
*
***************************************************************************
* END_IMS lanc_ether_ics */
lanc_ether_ics(ifnet_ptr, mbuf_ptr)
struct ifnet *ifnet_ptr;
struct mbuf *mbuf_ptr;
{
   /* Local variables. */

   lan_ift *lanc_ift_ptr = (lan_ift *) ifnet_ptr;
   register lan_rheader *lan;
   register struct logged_link  *link_ptr;
   int header_size, protocol_val, protocol_kind, pkt_format;
   struct etherxt_hdr *eth_canon_hdr;
   struct ifqueue *ifq;
   u_short acflags;

   mib_ifEntry *mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
   mib_xEntry *mib_xptr = lanc_ift_ptr->mib_xstats_ptr;

#ifdef LAN_MS
   int ms_turned_on_flag;
#endif /* LAN_MS */

   lan = mtod (mbuf_ptr, lan_rheader *);

   LAN_LOCK(lanc_ift_ptr->mib_lock);
   if (lan->destination[0] & 0x01)
      mib_ptr->ifInNUcastPkts++ ;
   else
      mib_ptr->ifInUcastPkts++ ;
   LAN_UNLOCK(lanc_ift_ptr->mib_lock);

   protocol_val = lan->type;
   if(protocol_val == HP_EXPANDTYPE) {/* Case Canonical Type */ 
   	eth_canon_hdr = mtod(mbuf_ptr, struct etherxt_hdr *);
        protocol_val = eth_canon_hdr->dcanon_addr;
        protocol_kind = (int) LAN_CANON;
        /*
         *  I am hoping here that I do not receive ethernet packets from
         *  other than CSMA/CD medium.  That is,  the only driver that is
         *  calling this routine is CSMA/CD drivers.
         */
        header_size = ETHERXT_HLEN;
        pkt_format = ETHERXT_PKT;
   } else {
        protocol_kind = (int) LAN_TYPE;
        header_size = ETHER_HLEN;
        pkt_format = ETHER_PKT;
   }

   AC_LOCK((struct arpcom *)ifnet_ptr);
   acflags=((struct arpcom *)ifnet_ptr)->ac_flags;
   AC_UNLOCK((struct arpcom *)ifnet_ptr);


   if (((link_ptr=lanc_lookup(protocol_val,protocol_kind, ifnet_ptr, 0)) 
       == NULL) || ((link_ptr->log.flags & LANC_CHECK_FLAGS) &&
        (!(ifnet_ptr->if_flags & IFF_UP) ||
     /*
      * (((pkt_format == IEEE8023_PKT) || (pkt_format == IEEE8023XSAP_PKT)) &&
      * !(((struct arpcom *)ifnet_ptr)->ac_flags & ACF_IEEE8023)) || 
      * (((pkt_format == SNAP8023_PKT) || (pkt_format == SNAP8023XT_PKT)) &&
      * !(((struct arpcom *)ifnet_ptr)->ac_flags & ACF_SNAP8023)) ||  
      */
        (
     /*  always true!!
      *  ((pkt_format == ETHER_PKT) || (pkt_format == ETHERXT_PKT)) && 
      */
        !(acflags & ACF_ETHER))))) {
     /*  cannot expect TEST and XID packets over ethernet since they are
      *  not defined for ethernet
      *  special kludge for driver TEST or XID packets
      * if ((ieee_packet) && !(ie_hdr->ssap & 1) && (ie_hdr->dsap == 0) &&
      *   ((ie_hdr->ctrl == IEEECTRL_XID) || 
      *    (ie_hdr->ctrl == (IEEECTRL_XID &~ PF_BITMASK)) ||
      *    (ie_hdr->ctrl == IEEECTRL_TEST) ||
      *    (ie_hdr->ctrl == (IEEECTRL_TEST &~ PF_BITMASK)))){
      *lanc_802_2_test_ctl(mbuf_ptr, lanc_ift_ptr);
      *return;
      *}    
      */

     /* Not logged in or appropriate flag not set*/

	LAN_LOCK(lanc_ift_ptr->mib_lock);
        mib_ptr->ifInUnknownProtos++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);

	LAN_LOCK(lanc_ift_ptr->lanc_lock);
        lanc_ift_ptr->UNKNOWN_PROTO++;
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);

        NS_LOG_INFO((LE_RINT_UNLOGGED_SAP+protocol_kind), NS_LC_PROLOG,
	       NS_LS_DRIVER, 0, 2, protocol_val, lanc_ift_ptr->is_if.if_unit);
        goto drop;
   } 

    /* strip header if so requested in flags field */
   if (link_ptr->log.flags & LANC_STRIP_HEADER){
        mbuf_ptr->m_off += header_size;
        mbuf_ptr->m_len -= header_size;
   }
   
   if (link_ptr->log.flags & LANC_ON_ICS){
        /* call protocol input routine directly if ON_ICS bit set */
        (void)(*link_ptr->log.rint)(ifnet_ptr,mbuf_ptr,
            link_ptr->log.lu_protocol_info, pkt_format);
   }
   else { 
        /* if ON_ICS bit not set, then queue input processing on netisr */
        ifq = (struct ifqueue *)link_ptr->log.rint;
	IFQ_LOCK(ifq);
        if (IF_QFULL(ifq)) {
            IF_DROP(ifq);

	    IFQ_UNLOCK(ifq);

	    LAN_LOCK(lanc_ift_ptr->mib_lock);
            mib_xptr->if_DInDiscards++;
	    LAN_UNLOCK(lanc_ift_ptr->mib_lock);
 
            goto drop;
        }
        IF_ENQUEUEIF_NOLOCK(ifq, mbuf_ptr, ifnet_ptr);
	IFQ_UNLOCK(ifq);
        (void)schednetisr(link_ptr->log.lu_protocol_info);
   }
#ifdef LAN_MS
    /*
     * measurement instrumentation - measure time at which ON_ICS protocol
     * returned, or if _NOT_ ON_ICS, the time at which the packet was  queued 
     * on to the protocol queue and schednetisr was called.  Note that for 
     * the measurement, SNAP packets are classified as IEEE 802.3 packets.
     */
   if(ms_turned_on(ms_lan_proto_return)) {
        int  tot_len;
        struct mbuf *m_temp;
        ms_io_info.protocol = 0; /* Ethernet packet */
        /*
         *  calculate total data bytes in packet 
         */
        tot_len = 0;
        m_temp = mbuf_ptr;
        while(m_temp){
            tot_len += m_temp->m_len;
            m_temp = m_temp->m_next;
        }

        /*
         *   calculate total data bytes in packet 
         */
        tot_len = 0;
        m_temp = mbuf_ptr;
        while(m_temp){
            tot_len += m_temp->m_len;
            m_temp = m_temp->m_next;
        }
        
        ms_io_info.count = tot_len;
        ms_io_info.addr = lan->type;
        ms_io_info.unit = ifnet_ptr->if_unit;
        ms_io_info.tv = ms_gettimeofday();
        ms_data(ms_lan_proto_return, &ms_io_info, sizeof(ms_io_info));
   }
#endif /* LAN_MS */
   return;

drop:
   m_freem(mbuf_ptr);
}

/* BEGIN_IMS lanc_ether_trail_intr
***************************************************************************
****
****			lanc_ether_trail_intr
****
***************************************************************************
* Description
*       This is the "input processing" routine for Ethernet packets
*       with trailer types. This function does some mbuf surgery to 
*       make the trailer packet look like a regular ethernet packet
*       and then calls lanc_ether_ics recursively which routes the packet
*       to its actual destination protocol.
*
* Input Parameters  
*       ifnet_ptr     - pointer to the ifnet structure for the lan unit
*                       for the card the packet came in on
*	mbuf_ptr      - pointer to buffer containing inbound packet
*       logged_info   - a 32 bit value stored in the log structure at protocol
*                       logging time.
*
* Return Value
*	None.
*
* Algorithm
*		
* External Calls
*       m_copy, m_freem, NS_LOG, m_adj, m_cat, m_pullup, lanc_ether_ics.
*
* Called By
*	lanc_rint_ics
*
* To Do List
*
* Notes
*
* Modification History
*  When      Who  Description
*  ========  ===  ============================================================
*
***************************************************************************
* END_IMS lanc_trail_intr */
void lanc_ether_trail_intr(ifnet_ptr, mbuf_ptr, logged_info, pkt_format)
struct ifnet * ifnet_ptr;
struct mbuf * mbuf_ptr;
int logged_info;
int pkt_format;

/* ths function converts a packet in the ethernet trailer protocol format to
     a regular ethernet packet format */

/* Deal with trailer protocol: if type is PUP trailer get true type from  */
/* first 16-bit word past data.  (Remember that a type was trailer by     */
/* setting value of off to be non-zero.)  Prepend the trailing headers to */
/* the packet.                                                            */
	
/* ETHERTYPE_TRAIL    = 0x1000                                            */
/* ETHERTYPE_NTRAILER = 16                                                */
/* ETHERMTU          = 1500                                               */
/* ETHERMIN          = (60-14)                                            */

/**************************************************************************/
/*                       Format of trailer protocol                       */
/*                                                                        */
/*+----------------------------------------------------------------------+*/
/*|                                                                      |*/
/*| DA  SA  TRAIL_TYPE  DATA PAGES  ETHER_TYPE  TRAILER_LEN  IP_H  TCP_H |*/
/*|                                                                      |*/
/*+----------------------------------------------------------------------+*/
/**************************************************************************/

/**************************************************************************/
/*    Converted to a Regular Ethernet ethernet packet as shown below      */
/*                                                                        */
/*+----------------------------------------------------------------------+*/
/*|                                                                      |*/
/*|             DA  SA  ETHER_TYPE  IP_H TCP_H DATA PAGES                |*/
/*|                                                                      |*/
/*+----------------------------------------------------------------------+*/
/**************************************************************************/

{ 
   /* local variables */
   lan_rheader *lan;
   struct mbuf *m0, *m1, *m_temp;
   int len, off;

   lan_ift *lanc_ift_ptr = (lan_ift *)ifnet_ptr;
   mib_xEntry *mib_xptr = lanc_ift_ptr->mib_xstats_ptr;

 
   /*
    * If the packet format is not Ethernet (it could be SNAP, for example)
    * throw away the packet.
    */
   if (pkt_format != ETHER_PKT) {
      	m_freem(mbuf_ptr);

	LAN_LOCK(lanc_ift_ptr->mib_lock);
      	mib_xptr->if_DInErrors++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
	
      	goto bad_trailer;
   }
   /* copy the ethernet header to a new mbuf */
   if ((m0 = m_copy (mbuf_ptr, 0, ETHER_HLEN)) == 0) { 
      /* Copy failed */ 
      	m_freem (mbuf_ptr);

	LAN_LOCK(lanc_ift_ptr->mib_lock);
      	mib_xptr->if_DInDiscards++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);

      	goto bad_trailer;
   }
   lan = mtod(m0, lan_rheader *);

   /* delete ethernet header from old mbuf chain */
   mbuf_ptr->m_off += ETHER_HLEN;
   mbuf_ptr->m_len -= ETHER_HLEN;

   off = (lan->type - ETHERTYPE_TRAIL) * 512;
   if ( (off >= ETHERMTU) ||  (off < 0) ) { /* Garbage packet */ 
      	m_freem (mbuf_ptr);
      	m_freem (m0);

	LAN_LOCK(lanc_ift_ptr->mib_lock);
      	mib_xptr->if_DInErrors++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);

      	goto bad_trailer;
   }

   /* Copy the trailing headers into an mbuf (chain) and prepend it to the */
   /* packet.                                                              */

   len = 0;
   m_temp = mbuf_ptr;
   while (m_temp) {
      	len += m_temp->m_len;
      	m_temp = m_temp->m_next;
   }

   if (off >= len) { /* Garbage packet */ 
      	m_freem (mbuf_ptr);
      	m_freem (m0);

	LAN_LOCK(lanc_ift_ptr->mib_lock);
      	mib_xptr->if_DInErrors++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);

      	goto bad_trailer;
   } 
		
   if ((m1 = m_copy(mbuf_ptr, off, M_COPYALL)) == NULL) { /* Copy failed */ 
      	m_freem (mbuf_ptr);
      	m_freem (m0);

	LAN_LOCK(lanc_ift_ptr->mib_lock);
      	mib_xptr->if_DInDiscards++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);

      	goto bad_trailer;
   }
   /* assign actual type value in packet header */
   lan->type = * mtod(m1, u_short *);

   /* delete trailing headers from old chain*/
   len = 0;
   m_temp = m1;
   while (m_temp) {
      	len += m_temp->m_len;
      	m_temp = m_temp->m_next;
   }
   m_adj (mbuf_ptr, -len);

   m1->m_off += 2 * sizeof (u_short);
   m1->m_len -= 2 * sizeof (u_short);
 		

   m_cat(m1, mbuf_ptr);
   m_cat(m0, m1);
   
  /* Driver code assumes that entire level 2 header is in the first mbuf.
   * The largest possible size of level 2 ethernet header is 
   * IEEE8023XSAP_HLEN. So pullup this many bytes into the first mbuf.
   */
   if ((mbuf_ptr = m_pullup(m0, IEEE8023XSAP_HLEN)) == NULL) {

	LAN_LOCK(lanc_ift_ptr->mib_lock);
      	mib_xptr->if_DInDiscards++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);

      	goto bad_trailer;
   }

   /* now call lanc_rint_ics to process a regular (i.e. not trailer) packet */
/*   lanc_rint_ics(ifnet_ptr, mbuf_ptr); */
   lanc_ether_ics(ifnet_ptr, mbuf_ptr);

   return;

bad_trailer: 
   NS_LOG_INFO(LE_TRAILER_PKT_DROPPED, NS_LC_PROLOG, NS_LS_DRIVER,0, 1,
	       lanc_ift_ptr->is_if.if_unit, 0);
   return;
}


/* BEGIN_IMS lanc_media_control
***************************************************************************
****
****                        lanc_media_control
****
***************************************************************************
* Description
*       Handles any changes to the media portion of the link.   This
*       includes manipulation of addresses, receive filter.....
*
* Packet format:
*       Routine can handle 802.3 
*                          802.5 
*                          Ethernet
*
* Input Parameters  
*       lanc_ift_ptr  - pointer to the ift structure for the link
*       data_ptr      - pointer to buffer containing data information from
*                       the user.
*       block_flag    - indicates whether we block the requestor
*
*  lanc_media_control determines the particular field to be updated, as well 
*  as acceptable conditions for change.   It may also need to determine the
*  particular link (802.3, 802.5 etc) which this change is to occur for. 
*
*
* Return Value
*        None.
*
* Algorithm
*                
* External Calls
*
* Called By
*
* To Do List
*
* Notes
*
* Modification History
*  When      Who  Description
*  ========  ===  ============================================================
*  92/01/16  mvk  fix to delete multicast to check for 802.3/802.5 
*                 fix to change station addr to check for 802.3/802.5
*                 fix to functional address to get address pointer    
*  92/02/26  mvk  Added changes to correct support of functional address-802.5
*
***************************************************************************
* END_IMS lanc_media_control */

lanc_media_control(lanc_ift_ptr, data_ptr, block_flag)
register lan_ift	*lanc_ift_ptr;	/* pointer to either lla_cb or ifnet */
register struct fis	*data_ptr;	/* user data area */
register int		block_flag;	/* block the request if set */
   
{

   int error = 0;             	/* error returns */
   int hw_req_code = 0;		/* request to be made to hardware level */
   int i;

   LAN_LOCK(lanc_ift_ptr->lanc_lock);
   switch(data_ptr->reqtype) {

   case ADD_MULTICAST:
/*
 * 	ADD_MULTICAST requirements:
 *	1) Room in current list
 *	2) Multicast bit is set
 *   	3) Supplied address not already in list
 *
 *      Note : 802.5 only supports 1 multicast address
 */

     	if ((lanc_ift_ptr->num_multicast_addr >= MAX_MULTICAST_ADDRESSES) ||
           ((lanc_ift_ptr->num_multicast_addr >= MAX_8025_MULTICAST_ADDRESSES)
           && (lanc_ift_ptr->is_ac.ac_type == ACT_8025))) {
		error=EINVAL;
		break;
	}
     	if ((!(data_ptr->value.s[0] & 0x01)
		&& (lanc_ift_ptr->is_ac.ac_type == ACT_8023)) ||
		(!(data_ptr->value.s[0] & 0x80) 
		&& (lanc_ift_ptr->is_ac.ac_type == ACT_8025))) {
			error=EINVAL;
			break;
	}

        /* this is a 802.5 functional address check */
        if ((lanc_ift_ptr->is_ac.ac_type == ACT_8025) &&
           (data_ptr->value.s[0] == 0xc0) && (data_ptr->value.s[1] == 0x00) &&
           ((data_ptr->value.s[2] & 0x80) == 0x00)) {
			error=EINVAL;
			break;
	}
            
        /* this is another 802.5 broadcast address check */
        if ((lanc_ift_ptr->is_ac.ac_type == ACT_8025) &&
           (data_ptr->value.s[0] == 0xc0) && (data_ptr->value.s[1] == 0x00) &&
           (data_ptr->value.s[2] == 0xff) && (data_ptr->value.s[3] == 0xff) &&
           (data_ptr->value.s[4] == 0xff) && (data_ptr->value.s[5] == 0xff)) {
			error=EINVAL;
			break;
	}

     	for (i=0; i<lanc_ift_ptr->num_multicast_addr; i++) {
		if (!bcmp((caddr_t) data_ptr->value.s, 
		    (caddr_t) &lanc_ift_ptr->mcast[LAN_PHYSADDR_SIZE * i],
		    LAN_PHYSADDR_SIZE )) {
			LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
			return(EINVAL);	 	/* Address in list */
		}
	}
	bcopy ((caddr_t) data_ptr->value.s,                                  
	       (caddr_t) &(lanc_ift_ptr->mcast[LAN_PHYSADDR_SIZE*lanc_ift_ptr->
		num_multicast_addr]), LAN_PHYSADDR_SIZE);  /* add to list */
     
     	hw_req_code = LAN_REQ_MC;
     	lanc_ift_ptr->num_multicast_addr++;
     	break;

   case DELETE_MULTICAST:
/*
 * 	Delete Multicast requirements:
 *   	1) Current address list not empty
 *   	2) Supplied operand has multicast bit set
 *   	3) Supplied address is found in current list
 */
	if (lanc_ift_ptr->num_multicast_addr == 0) {
		error=EINVAL;
		break;
	}
     	if ((!(data_ptr->value.s[0] & 0x01)
		&& (lanc_ift_ptr->is_ac.ac_type == ACT_8023)) ||
		(!(data_ptr->value.s[0] & 0x80) 
		&& (lanc_ift_ptr->is_ac.ac_type == ACT_8025))) {
			error=EINVAL;
			break;
	}

        /* this is a 802.5 functional address check */
        if ((lanc_ift_ptr->is_ac.ac_type == ACT_8025) &&
           (data_ptr->value.s[0] == 0xc0) && (data_ptr->value.s[1] == 0x00) &&
           ((data_ptr->value.s[2] & 0x80) == 0x00)) {
			error=EINVAL;
			break;
	}
            
        /* this is another 802.5 broadcast address check */
        if ((lanc_ift_ptr->is_ac.ac_type == ACT_8025) &&
           (data_ptr->value.s[0] == 0xc0) && (data_ptr->value.s[1] == 0x00) &&
           (data_ptr->value.s[2] == 0xff) && (data_ptr->value.s[3] == 0xff) &&
           (data_ptr->value.s[4] == 0xff) && (data_ptr->value.s[5] == 0xff)) {
			error=EINVAL;
			break;
	}

     	error = EINVAL;
     	for (i=0; i < lanc_ift_ptr->num_multicast_addr; i++) {
		if (!bcmp ((caddr_t) data_ptr->value.s,
		    (caddr_t) &lanc_ift_ptr->mcast[LAN_PHYSADDR_SIZE * i],
		    LAN_PHYSADDR_SIZE )) {  /* Match found */
			error = 0;
			break;
		}
	}
     	if (error) {
		break;	/* address not matched */
	}
     
/* Put the last addr in the list in the place of the deleted one.     */
     	if (i != (lanc_ift_ptr->num_multicast_addr - 1))
		bcopy ((caddr_t) &(lanc_ift_ptr->mcast[LAN_PHYSADDR_SIZE *
		       (lanc_ift_ptr->num_multicast_addr-1)]),
	       	       (caddr_t) &(lanc_ift_ptr->mcast[LAN_PHYSADDR_SIZE * i]),
		       LAN_PHYSADDR_SIZE );
     
     	hw_req_code = LAN_REQ_MC;
     	lanc_ift_ptr->num_multicast_addr--;

     	break;

   case CHANGE_NODE_RAM_ADDR:
    	if ((data_ptr->value.s[0] == 0x00) && (data_ptr->value.s[1] == 0x00) &&
            (data_ptr->value.s[2] == 0x00) && (data_ptr->value.s[3] == 0x00) &&
            (data_ptr->value.s[4] == 0x00) && (data_ptr->value.s[5] == 0x00)) {
		error=EINVAL;	/* null address */
		break;
	}

    	if ((data_ptr->value.s[0] == 0xff) && (data_ptr->value.s[1] == 0xff) &&
            (data_ptr->value.s[2] == 0xff) && (data_ptr->value.s[3] == 0xff) &&
            (data_ptr->value.s[4] == 0xff) && (data_ptr->value.s[5] == 0xff)) {
		error=EINVAL;	/* broadcast address */
		break;
	}

     	if (((data_ptr->value.s[0] & 0x01)
		&& (lanc_ift_ptr->is_ac.ac_type == ACT_8023)) ||
		((data_ptr->value.s[0] & 0x80) 
		&& (lanc_ift_ptr->is_ac.ac_type == ACT_8025))) {
			error=EINVAL;
			break;
	}

	AC_LOCK((struct arpcom *)lanc_ift_ptr);
    	if (!bcmp((caddr_t)data_ptr->value.s, (caddr_t)lanc_ift_ptr->is_addr,
	  	  LAN_PHYSADDR_SIZE)) {
		AC_UNLOCK((struct arpcom *)lanc_ift_ptr);
		LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
		return(0);		/* address is already set */
	}
	AC_UNLOCK((struct arpcom *)lanc_ift_ptr);

    	bcopy((caddr_t)data_ptr->value.s, (caddr_t)lanc_ift_ptr->station_addr,
	      LAN_PHYSADDR_SIZE);	/* store the new station address */

    	hw_req_code = LAN_REQ_RAM_NA_CHG;

    	break;

   case SET_TRN_FUNC_ADDR_MASK:
   case CLEAR_TRN_FUNC_ADDR_MASK:
        /* note - we're planning on passing down our data to the */
        /* hw_req routine...  this is because the data in the    */
        /* request is of a specific nature to the token drivers  */
        /* only and thus is not in the lanc portion of the data  */
        /* structure....                                         */
    	hw_req_code = LAN_REQ_TRN_FUNC_MASK;
    	break;

   case ENABLE_BROADCAST:
     	lanc_ift_ptr->broadcast_filter = 1;
     	hw_req_code = LAN_REQ_RF;
     	break;

   case DISABLE_BROADCAST:
     	lanc_ift_ptr->broadcast_filter = 0;
     	hw_req_code = LAN_REQ_RF;
     	break;

   case RESET_INTERFACE:
/*
 *	Reset the hardware interface card.
 */
       	hw_req_code = LAN_REQ_RESET;  /* set hardware request */
       	lanc_ift_ptr->BAD_CONTROL = 0;	/* reset driver statistics */
       	lanc_ift_ptr->UNKNOWN_PROTO = 0;
       	lanc_ift_ptr->RXD_XID = 0;
       	lanc_ift_ptr->RXD_TEST = 0;
       	lanc_ift_ptr->RXD_SPECIAL_DROPPED = 0;
       	break;

   case RESET_STATISTICS:
/*
 *	reset the driver statistics.
 */
       	hw_req_code = LAN_REQ_SET_STAT;
       	lanc_ift_ptr->BAD_CONTROL = 0;	/* reset driver statistics */
       	lanc_ift_ptr->UNKNOWN_PROTO = 0;
       	lanc_ift_ptr->RXD_XID = 0;
       	lanc_ift_ptr->RXD_TEST = 0;
       	lanc_ift_ptr->RXD_SPECIAL_DROPPED = 0;
       	break;

   case LAN_REQ_DUMP:		/* internal use by lan2 */
       	hw_req_code = LAN_REQ_DUMP;
	break;
	
   case LAN_REQ_ORD_DUMP:	/* internal use by lan2 */
       	hw_req_code = LAN_REQ_ORD_DUMP;
	break;

   default:
	error=EINVAL;
	break;

   }	/* end of reqtype switch */
   LAN_UNLOCK(lanc_ift_ptr->lanc_lock);

   if (!error && hw_req_code) {  
#ifdef __hp9000s800
	i=spl5();
#endif
	/* check the driver's status and send req to card */
	if (block_flag == LAN_REQ_BLOCK)	/* block the request */
                if (hw_req_code == LAN_REQ_TRN_FUNC_MASK) 
		   error = (*lanc_ift_ptr->hw_req)
			   (lanc_ift_ptr, (hw_req_code | LAN_REQ_BLK_MSK),
			    data_ptr);
                else
		   error = (*lanc_ift_ptr->hw_req)
			   (lanc_ift_ptr, (hw_req_code | LAN_REQ_BLK_MSK),
			    (caddr_t)NULL);
	else	/* not block the request */
                if (hw_req_code == LAN_REQ_TRN_FUNC_MASK) 
		   error = (*lanc_ift_ptr->hw_req)
			   (lanc_ift_ptr, hw_req_code, data_ptr);
                else
		   error = (*lanc_ift_ptr->hw_req)
			   (lanc_ift_ptr, hw_req_code, (caddr_t)NULL);
	 
	if (error < 0)
		error = lanc_ctl_sleep(lanc_ift_ptr, error);
#ifdef __hp9000s800
	splx(i);
#endif
   }
   return(error);
}    /* end of lanc_media_control */


#ifndef __hp9000s300


token_802_2_ics(ifnet_ptr, mbuf_ptr, offset_to_8022, link_header_type)
struct ifnet *ifnet_ptr;
struct mbuf *mbuf_ptr;
int    offset_to_8022, link_header_type;
{
   /* Local variables. */

   lan_ift *lanc_ift_ptr = (lan_ift *) ifnet_ptr;
   register lan_rheader *lan;
   register struct logged_link  *link_ptr;
   int llc_header_size, protocol_val, protocol_val_ext, protocol_kind, pkt_format;
   struct ieee8025_sr_hdr *ie_8025_sr_hdr;
   struct snap8025_sr_hdr *ie_snap_sr_hdr;
   struct ieee8022_hdr    *ie_8022_hdr;
   struct snap8022_hdr    *ie_snap_hdr;
   struct ifqueue *ifq;
   struct mbuf *m_temp;
   u_short acflags;
#ifdef LAN_MS
   int ms_turned_on_flag;
#endif /* LAN_MS */

   mib_ifEntry *mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
   mib_xEntry *mib_xptr = lanc_ift_ptr->mib_xstats_ptr;

   ie_8025_sr_hdr = mtod (mbuf_ptr, struct ieee8025_sr_hdr *);

   LAN_LOCK(lanc_ift_ptr->mib_lock);
   if (ie_8025_sr_hdr->destaddr[0] & 0x80)
	mib_ptr->ifInNUcastPkts++ ;
   else
        mib_ptr->ifInUcastPkts++ ;
   LAN_UNLOCK(lanc_ift_ptr->mib_lock);

   ie_snap_hdr = (struct snap8022_hdr *)(((caddr_t)ie_8025_sr_hdr) + 
		offset_to_8022);
   protocol_val = ie_snap_hdr->dsap;

   switch(protocol_val){
   case IEEESAP_HP:
   case IEEESAP_NM:
        /* not supported for 802.5 */
        goto drop; 
        break;

   case IEEESAP_SNAP:
        {
        u_char *protocol_val_ptr = (u_char *)(&protocol_val);
        *(protocol_val_ptr + 0) = ie_snap_hdr->orgid[0];
        *(protocol_val_ptr + 1) = ie_snap_hdr->orgid[1];
        *(protocol_val_ptr + 2) = ie_snap_hdr->orgid[2];
        *(protocol_val_ptr + 3) = ie_snap_hdr->orgid[3];
        protocol_val_ext = ie_snap_hdr->orgid[4] << 24;
        protocol_kind = (int) LAN_SNAP;
        llc_header_size = IEEE8022_SNAP_HLEN;
        pkt_format = SNAP8025_PKT;
        }
        break;


   default:
	protocol_kind = (int) LAN_SAP;
        llc_header_size = IEEE8022_SAP_HLEN;
        pkt_format = IEEE8025_PKT;
        break;
   }

   AC_LOCK((struct arpcom *)ifnet_ptr);
   acflags=((struct arpcom *)ifnet_ptr)->ac_flags;
   AC_UNLOCK((struct arpcom *)ifnet_ptr);

/*
 *   	The SAP is not logged in the hash table or
 *   	A check to make sure that the interface is not logically down or
 *   	encapsulation scheme accepted does not match the incoming packet
 *      IEEE 802.3 or SNAP or ETHER
 *   	Check whether the packet is a TEST or XID packet
 */
    if (((link_ptr=lanc_lookup(protocol_val,protocol_kind,ifnet_ptr,
                               protocol_val_ext))==NULL) ||
       ((link_ptr->log.flags & LANC_CHECK_FLAGS) &&
        (!(ifnet_ptr->if_flags & IFF_UP) ||
          ((pkt_format == IEEE8025_PKT) &&
          !(acflags & ACF_IEEE8025)) ||
           ((pkt_format == SNAP8025_PKT) &&
           !(acflags & ACF_SNAP8025))))) {

 /* special kludge for driver TEST or XID packets */
	if ( /* always true (ieee_packet) && */
            !(ie_snap_hdr->ssap & 1) && (ie_snap_hdr->dsap == 0) &&
            ((ie_snap_hdr->ctrl == IEEECTRL_XID) || 
            (ie_snap_hdr->ctrl == (IEEECTRL_XID &~ PF_BITMASK)) ||
            (ie_snap_hdr->ctrl == IEEECTRL_TEST) ||
            (ie_snap_hdr->ctrl == (IEEECTRL_TEST &~ PF_BITMASK)))) {
		token_802_2_test_ctl(mbuf_ptr, offset_to_8022, lanc_ift_ptr);
            	return;
        }        
        /* Not logged in or appropriate flag not set*/
 
	LAN_LOCK(lanc_ift_ptr->mib_lock);
        mib_ptr->ifInUnknownProtos++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
 
	LAN_LOCK(lanc_ift_ptr->lanc_lock);
        lanc_ift_ptr->UNKNOWN_PROTO++;
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);


        NS_LOG_INFO((LE_RINT_UNLOGGED_SAP+protocol_kind), NS_LC_PROLOG,
		NS_LS_DRIVER, 0, 2, protocol_val, lanc_ift_ptr->is_if.if_unit);
        goto drop;
   }

   switch(ie_snap_hdr->ctrl) {
   case IEEECTRL_XID:
   case (IEEECTRL_XID & ~ PF_BITMASK):
   case IEEECTRL_TEST:
   case (IEEECTRL_TEST & ~ PF_BITMASK):
   case (IEEECTRL_DEF | PF_BITMASK):
   case IEEECTRL_DEF:
   	break;

   default:
/*
 *	check to see if this is a raw port.  if it is, then pass
 *	any of these packets on to the port.
 */
 	if (!(link_ptr->log.flags & LANC_RAW_802MAC)) {
		LAN_LOCK(lanc_ift_ptr->lanc_lock);
            	lanc_ift_ptr->BAD_CONTROL++;
		LAN_UNLOCK(lanc_ift_ptr->lanc_lock);

                NS_LOG_INFO (LE_IERINT_BAD_CTRL, NS_LC_PROLOG, NS_LS_DRIVER, 
            		     0, 1, lanc_ift_ptr->is_if.if_unit, 0);
		LAN_LOCK(lanc_ift_ptr->mib_lock);
                mib_ptr->ifInUnknownProtos++;
		LAN_UNLOCK(lanc_ift_ptr->mib_lock);
                goto drop;
        }
   }

   /* strip header if so requested in flags field */
   if (link_ptr->log.flags & LANC_STRIP_HEADER){
   	mbuf_ptr->m_off += (offset_to_8022 + llc_header_size) ;
        mbuf_ptr->m_len -= (offset_to_8022 + llc_header_size) ;
   }
   
   if (link_ptr->log.flags & LANC_ON_ICS){
        /* call protocol input routine directly if ON_ICS bit set */
   	(void)(*link_ptr->log.rint)(ifnet_ptr,mbuf_ptr,
        link_ptr->log.lu_protocol_info, pkt_format);
   } else { 
        /* if ON_ICS bit not set, then queue input processing on netisr */
        ifq = (struct ifqueue *)link_ptr->log.rint;
	IFQ_LOCK(ifq);
        if (IF_QFULL(ifq)) {
            IF_DROP(ifq);

	    IFQ_UNLOCK(ifq);

	    LAN_LOCK(lanc_ift_ptr->mib_lock);
            mib_xptr->if_DInDiscards++;
	    LAN_UNLOCK(lanc_ift_ptr->mib_lock);

            goto drop;
        }
        IF_ENQUEUEIF_NOLOCK(ifq, mbuf_ptr, ifnet_ptr);
	IFQ_UNLOCK(ifq);
        (void)schednetisr(link_ptr->log.lu_protocol_info);
   }
#ifdef LAN_MS
    /*
     * measurement instrumentation - measure time at which ON_ICS protocol
     * returned, or if _NOT_ ON_ICS, the time at which the packet was  queued 
     * on to the protocol queue and schednetisr was called.  Note that for 
     * the measurement, SNAP packets are classified as IEEE 802.3 packets.
     */
   if(ms_turned_on(ms_lan_proto_return)) {
        ms_io_info.protocol = 1; /* IEEE packet */
        /*
         *   calculate total data bytes in packet 
         */
        tot_len = 0;
        m_temp = mbuf_ptr;
        while(m_temp){
            tot_len += m_temp->m_len;
            m_temp = m_temp->m_next;
        }
        ms_io_info.count =  tot_len; 
        ms_io_info.addr = ie_snap_hdr->dsap;
        ms_io_info.unit = ifnet_ptr->if_unit;
        ms_io_info.tv = ms_gettimeofday();
        ms_data(ms_lan_proto_return, &ms_io_info, sizeof(ms_io_info));
   }

#endif /* LAN_MS */
   return;

drop:
   m_freem(mbuf_ptr);
}



token_802_2_test_ctl(mbuf_ptr, offset_to_8022, lanc_ift_ptr)
struct mbuf *mbuf_ptr;
lan_ift * lanc_ift_ptr;
int       offset_to_8022;
{   
   /* local variables */

   register struct snap8025_sr_hdr *snap_8025_hdr_ptr;
   register struct snap8022_hdr *snap_8022_hdr_ptr;
   u_char tempsap;
   struct ns_xid_info_field *xid;
   int old_level;
   struct mbuf * m;

   mib_ifEntry *mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
   mib_xEntry *mib_xptr = lanc_ift_ptr->mib_xstats_ptr;

   snap_8025_hdr_ptr = mtod(mbuf_ptr, struct snap8025_sr_hdr *);
   snap_8022_hdr_ptr = (struct snap8022_hdr *)(mtod(mbuf_ptr, caddr_t) + 
		offset_to_8022);

   /* Switch the dsap and ssap, and the destaddr and sourceaddr, 
    * then stick the packet immediately on the output queue and exit.
    */
	
   tempsap = snap_8022_hdr_ptr->dsap;
   snap_8022_hdr_ptr->dsap = snap_8022_hdr_ptr->ssap;
   snap_8022_hdr_ptr->ssap = tempsap | 1;	     /* for response */
 
   bcopy((caddr_t)snap_8025_hdr_ptr->sourceaddr, 
         (caddr_t)snap_8025_hdr_ptr->destaddr,
	 LAN_PHYSADDR_SIZE);

  /* If an XID packet, drop the inbound info field and send back ours.    */
		
   if ((snap_8022_hdr_ptr->ctrl | PF_BITMASK) == IEEECTRL_XID) {   /* XID pkt */
  
      /* Header ALWAYS short (non-xsap) since driver only returns for null  */ 
      /* dsap.                                                              */

      	xid =  (struct ns_xid_info_field *)snap_8022_hdr_ptr->orgid;
      	xid->xid_info_format = XID_IEEE_FORMAT;
      	xid->xid_info_type = XID_TYPE1_CLASS1;
      	xid->xid_info_window = XID_WINDOW_SIZE;

      	if (mbuf_ptr->m_next != 0) {  /* Free any additional buffers */
         	m_freem(mbuf_ptr->m_next);
         	mbuf_ptr->m_next = 0;
      	}
        /* kludge to support only type 1 xid */
      	mbuf_ptr->m_len  = offset_to_8022 + 3 + XID_INFO_FIELD_SIZE;
	LAN_LOCK(lanc_ift_ptr->lanc_lock);
      	lanc_ift_ptr->RXD_XID++;
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
   } else {   /* TEST pkt */
	LAN_LOCK(lanc_ift_ptr->lanc_lock);
      	lanc_ift_ptr->RXD_TEST++;
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
   }

   /* play with rif */
   /* change RII bit in DA, SA RII bit allready set from bcopy of SA to DA */
   snap_8025_hdr_ptr->destaddr[0] = snap_8025_hdr_ptr->destaddr[0] & 
                                    (~IEEE8025_RII);
   /* change other routing fileds in source routing field */
   snap_8025_hdr_ptr->rif[1] = snap_8025_hdr_ptr->rif[1] ^ IEEE8025_RIF_DIR; 


   return((*lanc_ift_ptr->hw_req) (lanc_ift_ptr, LAN_REQ_WRITE, 
	  			   (caddr_t)mbuf_ptr));

} /* end of lanc_802_2_test_ctl */ 

#endif /* ! __hp9000s300 */
