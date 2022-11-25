/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/lanc_dda.c,v $
 * $Revision: 1.14.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:42:19 $
 */

#if  defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) lanc_dda.c $Revision: 1.14.83.3 $ $Date: 93/09/17 19:42:19 $";
#endif


#include "../h/systm.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/protosw.h"   
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/mbuf.h"
#include "../h/uio.h"
#include "../h/user.h"
#include "../h/kern_sem.h"

#include "../net/if.h"
#include "../net/netmp.h"

#include "../netinet/in.h"
#include "../netinet/if_ether.h"
#include "../netinet/if_ieee.h"

#include "../sio/netio.h"
#include "../sio/lanc.h"
#include "../sio/lla.h"

#ifdef __hp9000s300
#undef u
#ifndef udot
#define udot u
#endif /* udot */
#include "../s200io/lnatypes.h"
#endif /* __hp9000s300 */

/*
 *	These defines are undocumented features of LLA, they are primarily
 *	used for HP internal protocol development.  That is why they are
 *	defined here instead of netio.h.  This compatible with the S300
 *	implementation which serves DUX remote boot server.
 */

#define	LOG_XSSAP	801
#define	LOG_XDSAP	802


/*
 *	Forward and external function calls
 */

int	lanc_lla_rw();
int	lanc_lla_ioctl();
int	lanc_lla_select();
int	lanc_lla_close();
void	lanc_lla_timeout();
int	lanc_lla_input();

extern	int	lanc_remove_protocol();
extern	int	lanc_llc_control();
extern	int	lanc_ether_control();
extern	int	lanc_media_control();
extern	int	lanc_ctl_sleep();

/*
 *	External variables
 */

#ifdef __hp9000s800
extern	int	hz;
#endif

struct	fileops	lancops =
	{lanc_lla_rw, lanc_lla_ioctl, lanc_lla_select, lanc_lla_close};


/* BEGIN_IMS lanc_lla_open 
***************************************************************************
****							               ****
****		         lanc_lla_open (ifnet_ptr, dev, flag)          ****
****							               ****
***************************************************************************
* Description
*	Acquiring the file pointer fp, the file operation table
*	is replaced with a lla operation table so that subsequent
*	read/write/ioctl/select/close requests will go through
*	lla code.
*
* Notes
*	- Executes only on the kernel stack.
*
* Modifications
*  911030   mvk    Converted our f_type to DTYPE_LLA from a 
*                  DTYPE_SOCKET.  DTS INDaa09396
*
********************************************************************
* END_IMS lanc_lla_open */

 
extern struct fileops lancops;

#ifdef __hp9000s300
#define	LANC_PROTOCOL(a)  (a >> 24) - DIO_IEEE802
extern int    toplu;
#endif /* __hp9000s300 */

#ifdef __hp9000s800
#define	LANC_PROTOCOL(a)  (a & 0xff)
#endif /* __hp9000s800 */

lanc_lla_open (ifnet_ptr, dev, flag)
struct	ifnet	*ifnet_ptr;
int	dev;
int	flag;

{
   register struct lla_cb 	*lla_ptr;	/* pointer to created lla_cb */
   register struct file 	*file_ptr;      /* pointer to created fdesc */
   struct mbuf            	*m;
   int                           result;
   mib_ifEntry                  *mib_ptr;

   char 	*char_ptr;
   lan_ift 	*lanc_ift_ptr;
   int		i;    /* scratch loop index  */

   file_ptr = getf(udot.u_r.r_val1);	/* Get the global file descriptor. */ 
   if (file_ptr == NULL)
  	return(ENXIO);    /* File ptr is null */

   file_ptr->f_flag = FREAD|FWRITE;
   file_ptr->f_type = DTYPE_LLA;
   file_ptr->f_ops = &lancops;
   result = 0;
   lla_ptr = (struct lla_cb *)NULL;

/*
 * 	Obtain memory for the lla control block, the protocol control
 * 	block and the pktheader.  If memory is not available then cleanup,
 *	and exit with the ENOBUFS error. Outbound requirements are for
 * 	a burst rate of 5 packets (max size = 1500 bytes), each with a
 *	pkt header.  
 */
   MALLOC(lla_ptr, struct lla_cb *, sizeof(struct lla_cb), M_DYNAMIC, M_NOWAIT);
   if (!lla_ptr)
    	result = ENOBUFS;
   else {   /* got lla_cb */
    	bzero((caddr_t)lla_ptr, sizeof(struct lla_cb));  /* zeroize the block */
/*
 * 	Now we just need a mbuf to hold the place of last frame header
 */
    	m = m_getclr(M_DONTWAIT, MT_DATA);
    	if (m == NULL)
       		result = ENOBUFS;
    	else
       		lla_ptr->pktheader = m;	/* last pkt header initialization */
   }

/*
 * 	Now check the result, if we were unable to acquire all required
 * 	resources, then release everything and exit, otherwize finish the setup.
 * 	Since ma_expand does not provide a new pointer, we will only shrink the
 * 	inbound_macct if we were able to acquire a macct_ptr (inferential, so
 * 	order of getting the resource is important.
 */
    if (result) {
    	FREE(lla_ptr, M_DYNAMIC);
    	return(result);
    }

/*
 * 	Now set up the defaults for the lla control block as well as the
 * 	backward reference pointer from the protocol cb to the lla cb.
 */
    file_ptr->f_data = (caddr_t)lla_ptr;   /* point to the lla cb*/
    lla_ptr->lla_ifp = (struct arpcom *)ifnet_ptr;
                                           /* put in the ifnet pointer */
    lla_ptr->lla_msgsqd = 0;		   /* no messages are queued */
    lla_ptr->lla_maxmsgs = 1;		   /* default queue length to 1 */
    lla_ptr->head_packet = 0;		   /* set packet pointer to NULL */
    lla_ptr->last_packet = 0;		   /* set last packet pointer to NULL */
    lla_ptr->lla_timeo = 0;		   /* default to no timeout */
    lla_ptr->lan_signal_mask = 0;	   /* default signals to none */
    lla_ptr->lla_flags = 0;		   /* set flags to zero */
    lla_ptr->func_addr = 0;		   /* set FA to zero - 802.5 */

/*
 * 	the series 300 card does not provide the source address in its
 * 	outbound packets, so it must be copied from the lan_ift; 
 */
    lanc_ift_ptr = (lan_ift *) ifnet_ptr;
    if (lla_ptr->lla_ifp->ac_type == ACT_8025) {
      char_ptr = (caddr_t)lla_ptr->packet_header.proto.ieee8025.sourceaddr;
    } else {
      char_ptr = (caddr_t)lla_ptr->packet_header.proto.ether.sourceaddr;
    }
    for (i = 0; i < LAN_PHYSADDR_SIZE; i++, char_ptr++)
   	*char_ptr = lanc_ift_ptr->station_addr[i];

/*
 * 	if the user opened the file with FNDELAY, FWRITE or FREAD, then 
 * 	set the approprite flag in the control block
 */
    if (flag & FNDELAY)
   	lla_ptr->lla_flags |= LLA_FNDELAY;
    if (flag & FWRITE)
    	lla_ptr->lla_flags |= LLA_FWRITE;
    if (flag & FREAD)
    	lla_ptr->lla_flags |= LLA_FREAD;

    switch(lla_ptr->lla_ifp->ac_type) {
    case ACT_8023:
        if (LANC_PROTOCOL(dev) == LAN_PROTOCOL_ETHER) {  /* Ether protocol */
   	    lla_ptr->lla_flags |= LLA_IS_ETHER;
   	    lla_ptr->hdr_size = ETHER_HLEN;
        } else 
    	    lla_ptr->hdr_size = IEEE8023_HLEN;	/* IEEE Protocol */
        lla_ptr->lan_pkt_size = MTU_8023;
        break;

    case ACT_8025:
        lla_ptr->lla_flags |= LLA_IS_8025;
        lla_ptr->hdr_size = IEEE8025_HLEN;      /* IEEE .5 header, no rif */

        /* Get the mtu size from the mib info of this driver */
        lanc_ift_ptr = (lan_ift *) ifnet_ptr;
        mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
        lla_ptr->lan_pkt_size = mib_ptr->ifMtu;
        break;

    default:
        return(EINVAL);
    }
    LAN_ALLOC_LOCK(lla_ptr->lla_lock);
    if(!(lla_ptr->lla_lock)) {
        FREE(lla_ptr,M_DYNAMIC);
        return(ENOMEM);
    }
    LAN_INIT_LOCK(lla_ptr->lla_lock,LAN_LLA_LOCK_ORDER,"lan:lla_lock");

    return(EOPCOMPLETE);
}  /* Ending of lanc_lla_open function */


/*******************************************************************
****								****
****			lanc_lla_close(f)			****
********************************************************************
* END_IMS lanc_lla_close */

lanc_lla_close(fp)
struct file    *fp;  /*file pointer from which we derive the lla_cb structure*/
{
   register  struct lla_cb	*lla_ptr;
   int protocol_val, logged_kind;
   int	error=0;
   int remproto=0;
   int protocol_val_ext = 0;
   struct fis fa_data;
   int remfa=0;

   lan_ift  *lanc_ift_ptr;
   mib_xEntry  *mib_xptr;
   mib_ifEntry  *mib_ptr;

   if (!(fp->f_data))
    	return(0);

   lla_ptr = (struct lla_cb *)fp->f_data;

   lanc_ift_ptr = (lan_ift *)lla_ptr->lla_ifp;
   mib_xptr = lanc_ift_ptr->mib_xstats_ptr;

   /* remove any pending read timeouts */
   untimeout(lanc_lla_timeout,lla_ptr);

   LAN_LOCK(lla_ptr->lla_lock);

/*
 * 	If we have logged a ssap or type on the connection, we need to remove it
 * 	prior to releasing the input queue.
 */
   if (lla_ptr->lla_flags & LLA_PROT_LOGGED) {
     	if (lla_ptr->lla_flags & LLA_IS_ETHER) {
         	protocol_val = lla_ptr->packet_header.proto.ether.log_type;
		logged_kind=(int)LAN_TYPE;
		remproto=1;
        } else if (lla_ptr->lla_flags & LLA_IS_8025) {
                protocol_val = (int) lla_ptr->packet_header.proto.snap8025.ssap;
                switch(protocol_val) {
                case IEEESAP_SNAP:

                        logged_kind = (int) LAN_SNAP;
			bcopy(lla_ptr->packet_header.proto.snap8025.orgid,
			       &protocol_val, 4);
                        protocol_val_ext  =  
			   lla_ptr->packet_header.proto.snap8025.orgid[4] << 24;
                        break;

                default:
                        logged_kind = (int)LAN_SAP;
                        break;
                }
		remproto=1;
	} else {
         	protocol_val = (int) lla_ptr->packet_header.proto.ieee.ssap;
	 	switch(protocol_val) {
	     	case IEEESAP_SNAP:
		 	logged_kind = (int)LAN_SNAP;
		        bcopy(lla_ptr->packet_header.proto.ieee.pad,
		              &protocol_val, 4);
			protocol_val_ext  =  
			      lla_ptr->packet_header.proto.ieee.pad[4] << 24;
	         	break;
	     	case IEEESAP_NM:
		 	logged_kind = (int) LAN_CANON;
		 	protocol_val =
				 (int)lla_ptr->packet_header.proto.ieee.sxsap;
		 	break;
	     	default:
		 	logged_kind = (int)LAN_SAP;
		 	break;
         	}
		remproto=1;
	}
   }
   LAN_UNLOCK(lla_ptr->lla_lock);

   /* looking at removing the functional addressing for 802.5 only */
   LAN_LOCK(lla_ptr->lla_lock);
   if ((lla_ptr->lla_flags & LLA_IS_8025) &&
       (lla_ptr->lla_flags & LLA_IS_FA8025)) {
       fa_data.value.i = lla_ptr->func_addr;
       remfa = 1;
   }
   LAN_UNLOCK(lla_ptr->lla_lock);

   if (remfa) {
      fa_data.reqtype = CLEAR_TRN_FUNC_ADDR_MASK;
      fa_data.vtype   = INTEGERTYPE;
      lanc_lla_ioctl(fp, NETCTRL, &fa_data);
   }

#ifdef LAN_TEST_TRAILERS
   if(remproto && !(protocol_val>=ETHERTYPE_TRAIL && 
		protocol_val<=(ETHERTYPE_TRAIL+3))) 
#else
   if(remproto) 
#endif
   {
	lanc_remove_protocol(logged_kind,protocol_val,(int)(lla_ptr->lla_ifp),
			       protocol_val_ext);
   }


   /* a bit of probably unnecessary paranoia here:  since we just called
		lanc_remove_protocol, we should not be getting anymore
		interrupts for this lla_cb; zero out f_data to stop any
		possibly forked processes from trying to start a new 
		lla syscall while we are freeing everything and then
		lock and unlock the lla_cb to be sure any currently
		running syscalls or interrupts have finished.
   */
   fp->f_data= 0;
   LAN_LOCK(lla_ptr->lla_lock);
   LAN_UNLOCK(lla_ptr->lla_lock);

   if (lla_ptr->lla_msgsqd)
    	m_freem(lla_ptr->head_packet);  /* free inbound queued messages */


   m_free(lla_ptr->pktheader);  /* free the pktheaser hanging off the lla_cb */

   LAN_LOCK(lanc_ift_ptr->mib_lock);
   mib_xptr->if_DInDiscards += lla_ptr->lla_msgsqd;
   LAN_UNLOCK(lanc_ift_ptr->mib_lock);

   LAN_FREE_LOCK(lla_ptr->lla_lock);
   FREE(lla_ptr, M_DYNAMIC);  /* free the lla control block */

 
   return(error);
} /* end of lanc_lla_close */


/* BEGIN_IMS lanc_lla_rw *
********************************************************************
****								****
****			lanc_lla_rw(fp)				****
********************************************************************
*
* Description:
*    This subroutine is the initial entry point for lla i/o.  It
*    mainly sets up the reads/writes after checking that the interface
*    and selected protocol are enabled on the system.
*
********************************************************************
* END_IMS lanc_lla_rw */
lanc_lla_rw(fp, rw, uio)
struct file *fp;
enum uio_rw rw;
struct uio *uio;
   
{
   int	rtnval;
   struct lla_cb  *lla_ptr;
 
   if(!(lla_ptr = (struct lla_cb *)fp->f_data))
     	rtnval = EINVAL;

/*
 * 	The read or the write must be at least equal to zero
 * 	This interface assumes that the kernel has checked for
 * 	negative size values (bug was fixed in 3.1).
 */     

   if (rw==UIO_READ)
     	rtnval = lanc_lla_read((struct lla_cb *)fp->f_data, uio);
   else
	rtnval = lanc_lla_write((struct lla_cb *)fp->f_data, uio);

   return(rtnval);
}


/* BEGIN_IMS lanc_lla_read *
********************************************************************
****								****
****			lanc_lla_read(lla_ptr,uio)	       	****
********************************************************************
*
* Description:
*    This subroutine allows the user to read any data that has been queued to
*    the lla control block.  Actual data is queued the lla connection by
*    lanc_append.  This routine examines the number of messages queued and
*    if non-zero, copies the data to the user area and then relinks the 
*    mbuf queue.  The queue is treated FIFO.
*
*    If the user specified O_NDELAY, the read will not block but will return
*    the error, EWOULDBLOCK.  This error is used so that the user can distinguish
*    between having no data and having been sent a zero length packet.
*
*    If the user did not specify O_NDELAY, the read will block (through sleep)
*    until a packet is received (and the process is awaken by lanc_append) or
*    a timeout is received.  A timeout value of zero will suspend indefinitely
*    as sleep is not invoked with this value.
*
* Input Parameters:
*	lla_ptr  -  lla control block
*       uio      -  user io area
*
* Output Parameters:
*    uio - data (if any) from packet read as well as the read length.
*
* Return Value:
*    EWOULDBLOCK  -  no data exists and user has O_NDELAY set.
*    EIO          -  timeout or signal interrupt.
*    <read count> -  for a successful read, the number of bytes in the data
*                    area of a packet. 
*
* Modification History :
*   920120    mvk    Mods to remove MF_EOM dependence DTS INDaa09380
*   920228    mvk    Mods for functional addressing 
*
********************************************************************
* END_IMS lanc_lla_read */
lanc_lla_read(lla_ptr,uio)
struct lla_cb *lla_ptr;
struct uio    *uio;
{
   int  time_out_set = 0;
   int  s;
   int  error,			/* returned value */
        current_hdr_size,       /* mostly for 8025 packets with opt RIF */
        eom,			/* end of message boolean */
        len;			/* length intermedate for loop control */
   struct mbuf  *mbuf_ptr, *orig_mbuf_ptr;
   struct mbuf *t1_mbuf_ptr=0;
   struct mbuf *t2_mbuf_ptr=0;

   LAN_LOCK(lla_ptr->lla_lock);
   if (!(lla_ptr->lla_flags & LLA_PROT_LOGGED)) {

      if ((!(lla_ptr->lla_flags & LLA_IS_8025)) ||
         ((lla_ptr->lla_flags & LLA_IS_8025) &&
         !(lla_ptr->lla_flags & LLA_IS_FA8025))) {
	   LAN_UNLOCK(lla_ptr->lla_lock);
     	   return(EDESTADDRREQ);
         }
   }
 
/*
 * 	loop while awaiting data unless the user specified no-wait
 */
   for(error=0;(lla_ptr->lla_msgsqd==0) && !error; ) {
        if(lla_ptr->lla_flags & LLA_FNDELAY) {
                error=EWOULDBLOCK;
        } else {
                if(time_out_set = lla_ptr->lla_timeo) {
                        timeout(lanc_lla_timeout,lla_ptr,lla_ptr->lla_timeo);
                }
                lla_ptr->lla_flags |= LLA_WAIT;
                lla_ptr->lla_flags &= ~LLA_TIMEOUT;
#ifdef LAN_REAL_LOCKS
                s=sleep_lock();
                LAN_UNLOCK(lla_ptr->lla_lock);
                error=sleep_then_unlock((caddr_t)&lla_ptr->lla_flags,PZERO+1,s);
                LAN_LOCK(lla_ptr->lla_lock);
#else
		s = (int)(lla_ptr->lla_lock);
		lla_ptr->lla_lock = LAN_LOCK_AVAIL;
                error=sleep((caddr_t)&lla_ptr->lla_flags,PZERO+1);
		lla_ptr->lla_lock = (lock_t *)s;
#endif
                if(lla_ptr->lla_flags & LLA_TIMEOUT) {
                        error=EIO;
                } else {
                        untimeout(lanc_lla_timeout,lla_ptr);
                        if(error) {
                                error=EINTR;
                        }
                }
        }
   }



/*
 * 	either  1) error occurred, or
 *         	2) data is available
 */
   if (!(error)) {

/*
 * 	1) Save 1st mbuf ptr so that it may be re-threaded to the pktheader 
 *	   ptr for use by the FRAME_HEADER netstat call.
 * 	2) Adjust offset by the protocol header size since the header is not 
 *	   copied into the users buffer.
 * 	3) Loop through the mbufs and stop when:
 *      	a) EOM flag is picked up on the mbuf.
 *      	b) mbuf->next is undefined
 *      	c) or an error is encountered during the uiomove.
 *      	d) we have exhausted the users input buffer.
 */
     	orig_mbuf_ptr = mbuf_ptr = lla_ptr->head_packet;  /*head of rcv chain */
     	eom = 0;		/* set end of message to false */

        if (lla_ptr->lla_flags & LLA_IS_8025)  {

            if (lla_ptr->lla_flags & LLA_IS_FA8025) {
                  current_hdr_size = IEEE_8025_MAC_HLEN;
            } else {
              /* Determine for 802.5 size of header */
              if (lla_ptr->lla_flags & LLA_IS_SNAP8025)
                  current_hdr_size = SNAP8025_HLEN;
              else
                 current_hdr_size = IEEE8025_HLEN;
              } 
              /* Now determine if RIF info is present */
              if ((mtod(mbuf_ptr,caddr_t))[8] & 0x80)
                  current_hdr_size += (u_long)((mtod(mbuf_ptr,caddr_t))[14] 
                                                & 0x1f);
            }
        else {
            current_hdr_size = lla_ptr->hdr_size;
            }
        mbuf_ptr->m_off += current_hdr_size;
        mbuf_ptr->m_len -= current_hdr_size;

       /*  This logic was changed to remove the dependence of
        *  MF_EOM indicator (in 9.0)
        */

	LAN_UNLOCK(lla_ptr->lla_lock);
     	while (mbuf_ptr && !(error) && !eom) {
	 	if ((len = uio->uio_resid) <= 0)
	    		break;
         	if (len > mbuf_ptr->m_len)
	    		len = mbuf_ptr->m_len;
         	error = uiomove(mtod(mbuf_ptr,caddr_t), len,UIO_READ, uio);
                if (mbuf_ptr->m_next) {
                   mbuf_ptr = mbuf_ptr->m_next;
                } else {
                   eom = 1;
                }
	}

/*
 * 	if the users buffer had been exhausted, then we need to chain through 
 *	the mbufs so that we will be able to de-queue the entire packet.  We
 *      are relying on the fact that lanc_lla_input() has set the m_next to
 *      NULL when it reached the end of the current packet, and set the m_act
 *      of the last mbuf of the current packet chain to point to the beginning
 *      of the next packet chain.  We've changed this to remove MF_EOM
 *      dependence in 9.0
 */
        while (!eom) {
                if (mbuf_ptr->m_next)
                        mbuf_ptr = mbuf_ptr->m_next;
                else
                        eom = 1;
        }
 
/*
 * 	housekeeping for the queue:
 *  	1) relink forward queue ptr
 *  	2) adjust pkt count
 *  	3) terminate just-read packet so that it may be free (except for
 *         1st mbuf which contains the protocol header
 *  	4) terminate the last pktheader mbuf and link this one on
 */
	LAN_LOCK(lla_ptr->lla_lock);
     	lla_ptr->head_packet = mbuf_ptr->m_act;  /*set current head of chain*/
        lla_ptr->lla_msgsqd--;		/* reduce # of msgs queued*/
        if (!(lla_ptr->lla_msgsqd))
		lla_ptr->last_packet = 0;
     	mbuf_ptr->m_next = 0;			/* terminate this packet chain*/
	t1_mbuf_ptr = lla_ptr->pktheader;
     	orig_mbuf_ptr->m_off -= current_hdr_size;
     	orig_mbuf_ptr->m_len += current_hdr_size;
     	lla_ptr->pktheader = orig_mbuf_ptr;	/* thread the new header */
     	if (orig_mbuf_ptr->m_next) {
		t2_mbuf_ptr = orig_mbuf_ptr->m_next;
	}
     	lla_ptr->pktheader->m_next = 0;
   }
   LAN_UNLOCK(lla_ptr->lla_lock);

   if(t1_mbuf_ptr) {
     	m_free(t1_mbuf_ptr);		/* free the old header */
   }
   if(t2_mbuf_ptr) {
	m_freem(t2_mbuf_ptr);		/* free the old m_next */
   }

/*
 * 	Common exit, if timeout has been set then take it out, else just
 * 	return interrupt level and exit.
 */
   if (time_out_set) {
     	untimeout(lanc_lla_timeout,lla_ptr);
   }
   return(error);
}


/* BEGIN_IMS lanc_lla_write *
********************************************************************
****								****
****			lanc_lla_write(lla_ptr,uio)		****
********************************************************************
* Input Parameters:
*	lla_ptr    -- ptr to the lla control block for the user
*       uio        -- ptr to the user's io area
*
* Return Value:
*       ENOBUFS  -  unable to allocate networking memory for request
*                   no room on interface queue for packet
*       ENOSPC   -  send queue full
*
********************************************************************
 * END_IMS lanc_lla_write */

lanc_lla_write(lla_ptr, uio)
register struct lla_cb *lla_ptr;
register struct uio    *uio;
{
   int loop, error = 0;
   struct ifnet	  *ifnet_ptr;
   struct mbuf	  *mbuf_ptr, *head_mbuf_ptr;
   register struct mbuf *mbuf_ptr2;  /* fix for OSDEBUG */
   register int	*p, *q;
   int avail_len;   /* amount on memory available in mbuf/cluster */
   lan_ift *lanc_ift_ptr;

   mib_xEntry	*mib_xptr;
   mib_ifEntry	*mib_ptr;

   ifnet_ptr = (struct ifnet *)(lla_ptr->lla_ifp);
   lanc_ift_ptr = (lan_ift *) lla_ptr->lla_ifp;
   mib_xptr = lanc_ift_ptr->mib_xstats_ptr;
   mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);

/*
 * 	Check that the user has logged both a sap/type and a destination
 * 	address, if not then return an error.
 */
   LAN_LOCK(lla_ptr->lla_lock);
   if (lla_ptr->lla_flags & LLA_IS_8025) {

     if (((lla_ptr->lla_flags & (LLA_DEST_LOGGED | LLA_RIF_LOGGED)) != 
        (LLA_DEST_LOGGED | LLA_RIF_LOGGED)) ||  
        (((lla_ptr->lla_flags & LLA_PROT_LOGGED) != (LLA_PROT_LOGGED)) &&  
        ((lla_ptr->lla_flags & LLA_IS_FA8025) != (LLA_IS_FA8025)))) {
	  LAN_UNLOCK(lla_ptr->lla_lock);
   	  return(EDESTADDRREQ);
         }
   } else {
     if ((lla_ptr->lla_flags & (LLA_PROT_LOGGED | LLA_DEST_LOGGED)) != 
         (LLA_PROT_LOGGED | LLA_DEST_LOGGED)) {
	  LAN_UNLOCK(lla_ptr->lla_lock);
   	  return(EDESTADDRREQ);
     }
   }

/*
 * Check the user's buffer size, if writing more than protocol allows,
 * return an error.  For IEEE 8023, set the supplied length into the header
 * before moving it into the mbuf. Note that for IEEE, the length field
 * is actually user_data_bytes + header_size - MAC header, where the
 * MAC header is 14 bytes long (DA, SA, and length).
 */ 
   if ((lla_ptr->hdr_size + uio->uio_resid) > lla_ptr->lan_pkt_size) {
	LAN_UNLOCK(lla_ptr->lla_lock);
	LAN_LOCK(lanc_ift_ptr->mib_lock);
     	mib_xptr->if_DOutErrors++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
     	return(EMSGSIZE);
   }
   if ((!(lla_ptr->lla_flags & LLA_IS_ETHER)) &&
       (!(lla_ptr->lla_flags & LLA_IS_8025))) {
        lla_ptr->packet_header.proto.ieee.length =
                uio->uio_resid + (lla_ptr->hdr_size - 14);
   }

   ifnet_ptr = (struct ifnet *)(lla_ptr->lla_ifp);

   /* have to unlock and re-lock because m_get() calls spl() */
   LAN_UNLOCK(lla_ptr->lla_lock);

   mbuf_ptr= head_mbuf_ptr = m_get(M_DONTWAIT, MT_DATA);
   if (!mbuf_ptr) {
	LAN_LOCK(lanc_ift_ptr->mib_lock);
      	mib_ptr->ifOutDiscards++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
     	return (ENOBUFS);
   }

   LAN_LOCK(lla_ptr->lla_lock);

/*
 * 	if ieee packet then move the packet length into the header.
 * 	Then move the packet header into the mbuf
 */
 
   head_mbuf_ptr->m_off = ((MMINOFF+3)/4)*4; /* integer aligned */
   p = mtod(head_mbuf_ptr, int *);
   q = (((int *) &lla_ptr->packet_header));
 
   *p++ = *q++;		/* move 4 bytes at a time */
   *p++ = *q++;		/* rather than test header size, just move the */
   *p++ = *q++;   	/* largest size (40 bytes ~ 10 words) */
   *p++ = *q++;
   *p++ = *q++;
   *p++ = *q++;
   *p++ = *q++;
   *p++ = *q++;
   *p++ = *q++;
   *p++ = *q++;

   head_mbuf_ptr->m_len = lla_ptr->hdr_size;
   avail_len = MLEN - lla_ptr->hdr_size - ((((MMINOFF+3)/4)*4) - MMINOFF); 
 
/*
 * 	Check to see if the packet will fit in the mbuf that contains the
 * 	packet header, if not, then get a cluster for the data.  Note that
 * 	we also check for a write length = 0, this is in case the user wants
 * 	to just send the header.
 */

   head_mbuf_ptr->m_off += lla_ptr->hdr_size;

   LAN_UNLOCK(lla_ptr->lla_lock);

   if (avail_len < uio->uio_resid) {
     	MGET(mbuf_ptr2, M_DONTWAIT, MT_DATA);
     	if (!mbuf_ptr2)
	 	error = ENOBUFS;
     	else {
	 	mbuf_ptr->m_next = mbuf_ptr2;
	 	MCLGET(mbuf_ptr2);
	 	if (!M_HASCL(mbuf_ptr2))
/*
 * 	unable to get a cluster, we will try to satisfy the request using
 * 	mbufs, hopefully, this will be a rare condition.
 */
	     		avail_len = MLEN;
	 	else
	     		avail_len = mbuf_ptr2->m_clsize;
	 	mbuf_ptr = mbuf_ptr2;
	 
/*
 * 	Check if the user is on a byte (non-word boundary).  If so,
 *	move three bytes into the header mbuf so that the users buffer
 * 	will now start on a word boundary upon entry to the uiomove
 * 	function.  Adjust mbuf header info appropriately
 */
		if (loop = (int)uio->uio_iov->iov_base & 3) {
			error = uiomove(mtod(head_mbuf_ptr,caddr_t), loop,
					UIO_WRITE, uio);
	     		head_mbuf_ptr->m_len += loop;
		}
   	}
     	if (!(error))
	 	mbuf_ptr->m_len = 0;
   }  /* end of cluster/mbuf setup loop */

/*
 * 	Now loop (if we are using mbufs) to move the data from the
 * 	user area to newly acquired cluster of mbuf.  The loop is
 * 	continued until all data is moved, memory is exhausted or
 * 	an error occurs during the uiomove
 */
 
   while ((uio->uio_resid) && (mbuf_ptr) && (!(error))) {
     	if (avail_len >= uio->uio_resid)
		avail_len = uio->uio_resid;
     	error = uiomove(mtod(mbuf_ptr,caddr_t), avail_len, UIO_WRITE, uio);
     	mbuf_ptr->m_len += avail_len;
     	if (!(error)) {
	 	if (uio->uio_resid != 0) {
	     		MGET(mbuf_ptr2, M_DONTWAIT, MT_DATA);
	     		if (!(mbuf_ptr2))
		 		error = ENOBUFS;
	     		else {
		 		mbuf_ptr->m_next = mbuf_ptr2;
		 		mbuf_ptr = mbuf_ptr2;
		 		mbuf_ptr->m_len = 0;
		 		avail_len = MLEN;
			}
	    	}
	}
   }
/*
 * 	if we somehow got an error, we release acquired memory, and return
 * 	the error to the user
 */
   if (error) {
     	m_freem(head_mbuf_ptr);
	LAN_LOCK(lanc_ift_ptr->mib_lock);
     	mib_ptr->ifOutDiscards++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
     	return(error);
   }
 
/*
 * 	Now its time to put the data on the interface
 */
 
   head_mbuf_ptr->m_off = ((MMINOFF+3)/4)*4;

/* 	check to see if port type is RAW_802MAC.  if it is, then we need to
 * 	overwrite the value the user has written for the SSAP of the
 * 	802.2 header.  recall that with RAW_802MAC ports, the user must
 * 	write the 802.2 header and it must be at least three bytes long.
 */

   LAN_LOCK(lla_ptr->lla_lock);
   if (lla_ptr->lla_flags & LLA_RAW_802MAC) {
    	struct ieee8023_hdr *ptr;
/*
 * 	first check to see that at least three bytes of data has been
 * 	written ...
 */
    	if (m_len(head_mbuf_ptr) < (lla_ptr->hdr_size + MIN_IEEE8022_HLEN)) {
		LAN_UNLOCK(lla_ptr->lla_lock);
     		m_freem(head_mbuf_ptr);  /* packet too small, return mbuf */
		LAN_LOCK(lanc_ift_ptr->mib_lock);
     		mib_xptr->if_DOutErrors++;
		LAN_UNLOCK(lanc_ift_ptr->mib_lock);
     		return (EMSGSIZE);
    	}
/*
 * 	now put the SSAP into the packet ... if only it was that
 * 	that easy .... we must check the head mbuf since it's possible
 * 	that up to three bytes were moved in to align the next mbuf.
 */
    	if (head_mbuf_ptr->m_len >= (lla_ptr->hdr_size + 2)) {
		/* so the ssap was found in the head mbuf ...  */
      		ptr = mtod(head_mbuf_ptr, struct ieee8023_hdr *);
		ptr->ssap=(lla_ptr->packet_header.proto.ieee.ssap & 0xfe) |
				(ptr->ssap & 0x01);
    	} else {
      		if (head_mbuf_ptr->m_next) {
        		u_char *sptr;

        		sptr = mtod(head_mbuf_ptr->m_next,u_char *);
        		sptr += (1-(head_mbuf_ptr->m_len - lla_ptr->hdr_size));
			*sptr=(lla_ptr->packet_header.proto.ieee.ssap & 0xfe) |
					(*sptr & 0x01);
      		}
    	}
   }
   LAN_UNLOCK(lla_ptr->lla_lock);

   lanc_ift_ptr = (lan_ift *) ifnet_ptr;
   return((*lanc_ift_ptr->hw_req)(lanc_ift_ptr, LAN_REQ_WRITE, 
				  (caddr_t)head_mbuf_ptr));

} /* end of lanc_lla_write */


/* BEGIN_IMS lanc_lla_select *
************************************************************
****							****
****		lanc_lla_select(fp, which)		****
************************************************************
*
* Description:
*	This routine checks the LLA control block to see whether
*	there are inbound packets queued (for read) or whether
*	there is room on the interface queue (for writes).
*
************************************************************
* END_IMS lanc_lla_select */

lanc_lla_select(fp, which)
struct file *fp;
int which;

{
   struct lla_cb *lla_ptr;
   int	result;		/* boolean result to select */

   lla_ptr = (struct lla_cb *)fp->f_data;
   if (!(lla_ptr))
    	return(0);
/*
 * 	In order for a select to return true, the interface must be configured
 * 	up and for the protocol specified in the lla connection.  Failure of
 * 	either of these two conditions returns false.
 */ 

/*
 * 	Now check the actual lla control block or interface message queue for
 * 	read/write capability
 */
   switch(which) {
   case FREAD:
	LAN_LOCK(lla_ptr->lla_lock);
     	if (lla_ptr->lla_msgsqd > 0) {
	 	result = 1;
	} else {
         	if (lla_ptr->lla_rsel && 
		    lla_ptr->lla_rsel->p_wchan == (caddr_t)&selwait)
	     		lla_ptr->lla_flags |= LLA_SELECT_RCOLL;
	 	else
	     		lla_ptr->lla_rsel = u.u_procp;
	 	result = 0;
	}
	LAN_UNLOCK(lla_ptr->lla_lock);
        break;
   case FWRITE:
     	result = 1;
        IFQ_LOCK(&(lla_ptr->lla_ifp->ac_if.if_snd));
     	if (IF_QFULL(&lla_ptr->lla_ifp->ac_if.if_snd))
	 	result=0;
        IFQ_UNLOCK(&(lla_ptr->lla_ifp->ac_if.if_snd));
     	break;
   default:
     	result=0;
   } /* end of switch */
   return(result);
} /* end of lanc_lla_select */


lanc_lla_ioctl(file_ptr, cmd, data_ptr)
struct file *file_ptr;		/* global file pointer */
int cmd;			/* command mask */
register struct fis *data_ptr;	/* user data area */
   
{
   register struct lla_cb *lla_ptr;	/* lla control block */
   int i;                               /* scratch variable for loop, etc */
   int llaflags,llaactype;
   int result = 0;	                /* error returns */
   short lan_unit;			/* lan lu for this lla */
   lan_ift	*lanc_ift_ptr;		/* lan_ift ptr */
   struct fis   func_addr_data;         /* kludge for fa on 802.5 */

/*
 *  	Obtain the lla_ptr for this file descriptor.  If undefined, then return
 *  	error, otherwise get the raw control block ptr for subsequent processing
 */

   lla_ptr = (struct lla_cb *)file_ptr->f_data;
   if (!(lla_ptr))
     	return(EINVAL);

   lanc_ift_ptr = (lan_ift *)lla_ptr->lla_ifp;
   lan_unit = lla_ptr->lla_ifp->ac_if.if_unit;

   /*This saves having to do several LAN_LOCK calls later on in this routine.*/
   /*************
        While this technique is OK here, it is not good to use it in general.
        In general, the flags could change between the time we read them and
        the time we check them and take action based on that check.  Here, the
        flags which are checked will not change after the open.
   *************/
   LAN_LOCK(lla_ptr->lla_lock);
   llaflags=lla_ptr->lla_flags;
   llaactype=lla_ptr->lla_ifp->ac_type;
   LAN_UNLOCK(lla_ptr->lla_lock);

/*
 *  	For lla users, only NETCTRL and NETSTAT commands are valid.  NETCTRL
 *  	commands which are interface-oriented will be passed on to different
 *  	routines for processing depending upon the interface (i.e. 802.2, 802.3
 *  	Ethernet or LLA).  If a subrequest is undefined, the EINVAL error will
 *  	be returned.
 */

   if (cmd == NETCTRL) {

/*
 *  	Do not allow NETCTRL functions if lla device was opened with READ
 *  	only permissions.
 */

     	if (!(llaflags & LLA_FWRITE)) {
         	return(EBADF);
     	}

     	switch(data_ptr->reqtype) {
/*
 *  	The following control types belong to LLA specifics:
 *		LOG_READ_TIMEOUT, LOG_READ_CACHE, LLA_SIGNAL_MASK
 *  	After checking the parameters, the lanc_lla_control() will be called
 *  	to perform the function.
 */
     	case LOG_READ_TIMEOUT:
     	case LOG_READ_CACHE:
     	case LLA_SIGNAL_MASK:
 		if (data_ptr->vtype != INTEGERTYPE) {
     			return(EINVAL);
 		}
 		result = lanc_lla_control(lla_ptr, data_ptr);
 		return(result);
/*
 *  	The following control types belong to llc/802.2 function:
 *		LOG_XDSAP, LOG_XSSAP, LOG_RAW_802MAC, LOG_CONTROL,
 *		LOG_DSAP, LOG_SSAP.
 *  	After checking the parameters, the lanc_802_2_control() will be called
 *  	to perform the function.
 */
     	case LOG_XDSAP:
     	case LOG_XSSAP:
     	case LOG_RAW_802MAC:
 		if (!(suser())) {
	    		return(EPERM);
		}
                if (llaactype == ACT_8025) {
                        return(EINVAL);
                }
     	case LOG_CONTROL:
     	case LOG_DSAP:
     	case LOG_SSAP:
		if (llaflags & LLA_IS_ETHER) {
	    		return(EINVAL);
		}
		if (llaflags & LLA_IS_FA8025) {
	    		return(EINVAL);
		}
		if (data_ptr->vtype != INTEGERTYPE) {
	     		return(EINVAL);
		}
 		result = lanc_llc_control(lla_ptr, data_ptr);
		return(result);
/*
 *  	The following control types belong to Ethernet function:
 *		LOG_SNAP_TYPE, LOG_TYPE_FIELD.
 *  	After checking the parameters, the lanc_ether_control() will be called
 *  	to perform the function.
 */
     	case LOG_SNAP_TYPE:
		if (llaflags & LLA_IS_ETHER) {
	    		return(EINVAL);
		}
		if (llaflags & LLA_IS_FA8025) {
	    		return(EINVAL);
		}
		if (data_ptr->vtype != 5) {
	     		return(EINVAL);
		}
 		result = lanc_ether_control(lla_ptr, data_ptr);
		return(result);

     	case LOG_TYPE_FIELD:
		if (!(llaflags & LLA_IS_ETHER)) {
	    		return(EINVAL);
		}
		if (llaflags & LLA_IS_FA8025) {
	    		return(EINVAL);
		}
		if (data_ptr->vtype != INTEGERTYPE) {
	     		return(EINVAL);
		}
 		result = lanc_ether_control(lla_ptr, data_ptr);
		return(result);
/*
 *  	The following control types belong to media control function:
 *		LOG_DEST_ADDR, ENABLE_PROMISCUOUS, DISABLE_PROMISCUOUS,
 *		ADD_MULTICAST, DELETE_MULTICAST, CHANGE_NODE_RAM_ADDR,
 *		ENABLE_BROADCAST, DISABLE_BROADCAST ,RESET_INTERFACE,
 *              RESET_STATISTICS, LOG_RIF_ADDR, SET_TRN_FUNC_ADDR_MASK
 *	The LOG_DEST_ADDR request can be handled in the lanc_lla_ioctl(). It
 *	does not need to go down to lanc_media_control().  For the rest of the
 *	requests, after checking the parameters, the lanc_media_control() will
 *	be called to perform the function.
 *
 */
     	case LOG_DEST_ADDR:
		if (data_ptr->vtype != LAN_PHYSADDR_SIZE) {
	     		return(EINVAL);
		}
		LAN_LOCK(lla_ptr->lla_lock);
                switch(llaactype) {
                case ACT_8023:
                   bcopy((caddr_t)data_ptr->value.s,
                      (caddr_t)lla_ptr->packet_header.proto.ieee.destaddr, 6);
                   break;
                case ACT_8025:
                   bcopy((caddr_t)data_ptr->value.s,
                      (caddr_t)lla_ptr->packet_header.proto.ieee8025.destaddr,
                       6);
                   break;
                default:
		   LAN_UNLOCK(lla_ptr->lla_lock);
                   return(EINVAL);
                }
		lla_ptr->lla_flags |= LLA_DEST_LOGGED;
		LAN_UNLOCK(lla_ptr->lla_lock);
		return(0);

        case LOG_RIF_ADDR:
                if ((data_ptr->vtype < 0) ||
                    (data_ptr->vtype > LAN_8025_MAX_RIF_SIZE) ||
                    (data_ptr->vtype != ((int)data_ptr->value.s[0] & 0x1f)) ||
                    (data_ptr->vtype & 0x01) ||
                    (!(llaflags & LLA_IS_8025))) {
                        return(EINVAL);
                }

		LAN_LOCK(lla_ptr->lla_lock);
                bcopy((caddr_t)data_ptr->value.s,
                     (caddr_t)lla_ptr->packet_header.proto.ieee8025.rif_plus,
                     data_ptr->vtype);
                /* Now we have to squash the header */
                /* This means we move the LLC portion up against */
                /* the bottom of the actual RIF information */


                /* Now move the data - squish it till it bleeds */
                /* and set the new header size */
                if (lla_ptr->lla_flags & LLA_IS_FA8025) {
                   /* for FA, just adjust the hdr size since there is */
                   /* no llc to move */
                   lla_ptr->hdr_size = IEEE_8025_MAC_HLEN + data_ptr->vtype;
                } else {
                  if (lla_ptr->lla_flags & LLA_IS_SNAP8025) {
                     bcopy((caddr_t)&lla_ptr->packet_header.proto.snap8025.dsap,
                           (caddr_t)&lla_ptr->packet_header.proto.snap8025.
                                    rif_plus[data_ptr->vtype],
                            SNAP_802_2_HLEN);
                     lla_ptr->hdr_size = SNAP8025_HLEN + data_ptr->vtype;
                  }
                  else {
                     bcopy((caddr_t)&lla_ptr->packet_header.proto.ieee8025.dsap,
                           (caddr_t)&lla_ptr->packet_header.proto.ieee8025.
                                    rif_plus[data_ptr->vtype],
                            MIN_IEEE8022_HLEN);
                     lla_ptr->hdr_size = IEEE8025_HLEN + data_ptr->vtype;
                  }
                }
                /* Set up the indicator bit in the SA */
                if (data_ptr->vtype)
                   lla_ptr->packet_header.proto.snap8025.sourceaddr[0] =
                            lla_ptr->packet_header.proto.snap8025.sourceaddr[0]
                            | IEEE8025_RII;
                else
                   lla_ptr->packet_header.proto.snap8025.sourceaddr[0] =
                            lla_ptr->packet_header.proto.snap8025.sourceaddr[0]
                            & (~IEEE8025_RII);

                lla_ptr->lla_flags |= LLA_RIF_LOGGED;
		LAN_UNLOCK(lla_ptr->lla_lock);
                return(0);

        case SET_TRN_FUNC_ADDR_MASK:
        case CLEAR_TRN_FUNC_ADDR_MASK:
                if (!suser()) {
                        return(EPERM);
                }
                if  (!(llaflags & LLA_IS_8025)) {
                        return(EINVAL);
                }
                if  (llaflags & LLA_PROT_LOGGED) {
                        return(EINVAL);
                }
                if ((!(llaflags & LLA_IS_FA8025) &&
                   (data_ptr->reqtype == CLEAR_TRN_FUNC_ADDR_MASK)) ||
                   ((llaflags & LLA_IS_FA8025) &&
                   (data_ptr->reqtype == SET_TRN_FUNC_ADDR_MASK))) {
                        return(EBUSY);
                }
                if  (data_ptr->vtype != INTEGERTYPE) {
                        return(EINVAL);
                }
 
                /* we are, behind the users back, passing the */
                /* lanc_lla_input routine as the input routine */
                /* for this functional address */
                func_addr_data.reqtype = data_ptr->reqtype;
                func_addr_data.vtype   = 20;
                func_addr_data.value.i = data_ptr->value.i; /* func addr */

                *((int *)(&func_addr_data.value.s[4])) = (int)lanc_lla_input;

                /* zero out the reserved fields */
                for (i=8;i<20;i++) {
                    func_addr_data.value.s[i] = 0;
                }

                /* need the lla pointer for lanc_lla_input routine */
                *((int *)(&func_addr_data.value.s[12])) = (int)lla_ptr;

                /* set the flags */
                /* Strip header = 0, on_ics = 1, check_flags = 0 */
                func_addr_data.value.s[11] = func_addr_data.value.s[11] | 0x01;

                result = lanc_media_control(lla_ptr->lla_ifp, &(func_addr_data),
                                            LAN_REQ_BLOCK);
                if (!result) {
                   if (data_ptr->reqtype == CLEAR_TRN_FUNC_ADDR_MASK) {
		     LAN_LOCK(lla_ptr->lla_lock);
                     lla_ptr->packet_header.proto.ieee8025.access_ctl = IEEE8025_AC;
                     lla_ptr->packet_header.proto.ieee8025.frame_ctl = IEEE8025_FC;
                     lla_ptr->hdr_size = IEEE8025_HLEN;
                     lla_ptr->lla_flags = lla_ptr->lla_flags ^ LLA_IS_FA8025;
                     lla_ptr->func_addr = 0;
		     LAN_UNLOCK(lla_ptr->lla_lock);
                   } else {
		     LAN_LOCK(lla_ptr->lla_lock);
                     lla_ptr->packet_header.proto.ieee8025.access_ctl = IEEE8025_AC;
                     lla_ptr->packet_header.proto.ieee8025.frame_ctl = IEEE8025_FC;
                     lla_ptr->hdr_size = IEEE_8025_MAC_HLEN;
                     lla_ptr->lla_flags |= LLA_IS_FA8025;
                     lla_ptr->func_addr = data_ptr->value.i;
		     LAN_UNLOCK(lla_ptr->lla_lock);
                   }
                }
                return(result);

     	case ADD_MULTICAST:
     	case DELETE_MULTICAST:
     	case CHANGE_NODE_RAM_ADDR:
		if (!suser()) {
			return(EPERM);
		}
		if (data_ptr->vtype != LAN_PHYSADDR_SIZE) {
			return(EINVAL);
		}
 		result = lanc_media_control(lla_ptr->lla_ifp, data_ptr,
					    LAN_REQ_BLOCK);
		return(result);

     	case ENABLE_BROADCAST:
     	case DISABLE_BROADCAST:
     	case RESET_INTERFACE:
     	case RESET_STATISTICS:
#ifdef __hp9000s800
	case LAN_REQ_DUMP:		/* internal use by lan2 */
	case LAN_REQ_ORD_DUMP:		/* internal use by lan2 */
#endif /* __hp9000s800 */
		if (!suser()) {
			return(EPERM);
		}
 		result = lanc_media_control(lla_ptr->lla_ifp, data_ptr,
					    LAN_REQ_BLOCK);
		return(result);

     	default:
		return(EINVAL);

     	}  /* end of reqtype switch */
   }  /* end of NETCTL */

   if (cmd == NETSTAT) {
      	switch (data_ptr->reqtype) {
       	case BAD_CONTROL_FIELD:
#ifdef __hp9000s800
       	case RX_XID:
       	case RX_TEST:
       	case RX_SPECIAL_DROPPED:
#endif
		if (llaflags & LLA_IS_ETHER) {
	    		return(EINVAL);
	 	}
	 	break;
/*
 *  	Frame header can be handled in this module since it is available
 *	from accessing a pointer within the lla cb.
 *
 *      For 802.5, we need to calculate the header from the read packet
 *      since it may vary from packet to packet.
 */
        case FRAME_HEADER:
		LAN_LOCK(lla_ptr->lla_lock);
                if (llaflags & LLA_IS_8025)  {

                /* Determine for 802.5 type of header */
                   if (llaflags & LLA_IS_FA8025) {
                         data_ptr->vtype = IEEE_8025_MAC_HLEN;
                   } else {
                     if (llaflags & LLA_IS_SNAP8025) {
                         data_ptr->vtype = SNAP8025_HLEN;
                     } else {
                         data_ptr->vtype = IEEE8025_HLEN;
                     }
                   }

                /* Now determine if RIF info is present */
                if ((mtod(lla_ptr->pktheader, caddr_t))[8] & IEEE8025_RII)
                data_ptr->vtype += (u_long)((mtod(lla_ptr->pktheader, caddr_t))[
14] & 0x1f);
                }
                else
	 	   data_ptr->vtype = lla_ptr->hdr_size;

	 	bcopy((mtod(lla_ptr->pktheader, caddr_t)),
	       	      (caddr_t)(struct fis *)data_ptr->value.s,
	       	      data_ptr->vtype);
		LAN_UNLOCK(lla_ptr->lla_lock);
	 	return(result);

        }  /* end of reqtype switch */

#ifdef __hp9000s800
	i=spl5();
#endif
	result = (*lanc_ift_ptr->hw_req)
		 (lanc_ift_ptr, (LAN_REQ_GET_STAT | LAN_REQ_BLK_MSK), 
	         (caddr_t)data_ptr);
	if (result < 0)
		result = lanc_ctl_sleep(lanc_ift_ptr, result);
#ifdef __hp9000s800
	splx(i);
#endif

	return(result);
   }  /* end of NETSTAT */
/*
 *	If every thing goes right we should have returned to the user already.
 */
   return(EINVAL);
}   /* end of lanc_lla_ioctl */


lanc_lla_control(lla_ptr, data_ptr)
struct lla_cb *lla_ptr;		/* lla control block */
struct fis *data_ptr;		/* user data area */

{
   int old_level;		/* interrupt level on entry */
   int i, t;      

/*
 *	This routine handles the following NETCTRL command requests:
 *	  LOG_READ_TIMEOUT:  to alter the I/O timeout interval
 *	  LOG_READ_CACHE:    to alter the read cache
 *	  LLA_SIGNAL_MASK:   request the generation of a SIGIO signal to
 *			     the user process upon certain events
 */

   LAN_LOCK(lla_ptr->lla_lock);
   switch(data_ptr->reqtype) {

	case LOG_READ_TIMEOUT:
	   	if (data_ptr->value.i < 0) {
			LAN_UNLOCK(lla_ptr->lla_lock);
	    		return(EINVAL);
		}
/*
 * 	The resolution of the timer is expressed in 'hz', since LLA timeouts
 * 	are advertized in ms resolution, we obtain the number of ms per clock
 *	tick by dividing 1000 by hz.  To obtain the number of clock ticks for
 *	the requested timeout interval, we divide the timeout interval by t
 * 	and round up to accomodate clock resolution problems.
 */
	   	t = 1000/hz;
	   	lla_ptr->lla_timeo = data_ptr->value.i / t;
	   	i = data_ptr->value.i % t;
	   	if (i)
	     		lla_ptr->lla_timeo++;
	   	if (lla_ptr->lla_timeo)
             		lla_ptr->lla_timeo++;
	   	break;
	 
      	case LOG_READ_CACHE:
		if (data_ptr->value.i < MIN_CACHE) {
			LAN_UNLOCK(lla_ptr->lla_lock);
	    		return(EINVAL);
		}
	 
	   	if (suser()) {   /* super user process */
	    		if (lla_ptr->lla_maxmsgs < SU_MAX_CACHE) {
		   		i = data_ptr->value.i;
		   		if ((i + lla_ptr->lla_maxmsgs) > SU_MAX_CACHE)
					i = SU_MAX_CACHE - lla_ptr->lla_maxmsgs;
		   		lla_ptr->lla_maxmsgs += i;
			}
	   	} else {  /* normal user process */
	    		if (lla_ptr->lla_maxmsgs < MAX_CACHE) {
	     	   		i = data_ptr->value.i;
	     	   		if ((i + lla_ptr->lla_maxmsgs) > MAX_CACHE)
		 			i = MAX_CACHE - lla_ptr->lla_maxmsgs;
		   		lla_ptr->lla_maxmsgs += i;
	        	}
	   	}
	   	break;
	 
        case LLA_SIGNAL_MASK:
	   	if (data_ptr->value.i & (~(LLA_Q_OVERFLOW | LLA_PKT_RECV))) {
			LAN_UNLOCK(lla_ptr->lla_lock);
	     		return(EINVAL);
		}
	   	lla_ptr->lan_signal_mask = data_ptr->value.i;
	   	lla_ptr->lan_signal_pid = (unsigned)udot.u_procp;
	   	break;
   }
   LAN_UNLOCK(lla_ptr->lla_lock);
   return(0);
}    /* end of lanc_lla_control */


/*******************************************************************
****								****
****			lanc_lla_timeout(lla_ptr)		****
********************************************************************
*
* Description:
*  This routine is called when a timeout has occurred on a lla read.
*  It sets a timeout bit so that the read routine can divine that it
*  has encountered a timeout.  The wait flag is also cleared to reflect
*  the process's current state.
*
********************************************************************
* END_IMS lanc_lla_timeout */
void lanc_lla_timeout(lla_ptr)
struct lla_cb *lla_ptr;
{
   int s;
 
   LAN_LOCK(lla_ptr->lla_lock);
   lla_ptr->lla_flags |= LLA_TIMEOUT;
   lla_ptr->lla_flags &= ~LLA_WAIT;

   s=sleep_lock();
   wakeup((caddr_t)&lla_ptr->lla_flags);
   sleep_unlock(s);
   LAN_UNLOCK(lla_ptr->lla_lock);
}


/* BEGIN_IMS lanc_lla_input *
********************************************************************
****								****
**** lanc_lla_input(ifnet_ptr, mbuf_ptr, lla_ptr, pkt_format)	****
********************************************************************
*
* Notes: We assume that this function runs on the ICS (therefore
*        no splimp() calls are made to critical sections).
*
* Modification History :
*   920120  mvk   Changes to remove dependence on MF_EOM indicator.
*                 DTS INDaa09380
*
********************************************************************
* END_IMS lanc_lla_input */
lanc_lla_input(ifnet_ptr, mbuf_ptr, lla_ptr, pkt_format)
struct mbuf *mbuf_ptr;		/* pointer to data chain */
struct ifnet *ifnet_ptr;	/* pointer to ifnet structure */
struct lla_cb *lla_ptr;
int pkt_format; 		/* format of the arriving packet */

{
   lan_ift *lanc_ift_ptr = (lan_ift *)ifnet_ptr;
   mib_xEntry *mib_xptr = lanc_ift_ptr->mib_xstats_ptr;
   int s;

/*
 * 	this check to make sure that packet format conforms to the information
 * 	in the LLA control block, which is initialized during open. Also, it
 * 	prevents Ethernet packets being delivered if LLA expects SNAP packets
 * 	and vice versa.
 */

   LAN_LOCK(lla_ptr->lla_lock);
   switch (pkt_format){
   case ETHER_PKT:
	if(!(lla_ptr->lla_flags & LLA_IS_ETHER)) { /* lla block not associated 
						      with Ethernet device */ 
		LAN_UNLOCK(lla_ptr->lla_lock);
		m_freem(mbuf_ptr);
		LAN_LOCK(lanc_ift_ptr->mib_lock);
		mib_xptr->if_DInErrors++;
		LAN_UNLOCK(lanc_ift_ptr->mib_lock);
	     	return(0);
	}
	break;
   case IEEE8023XSAP_PKT:
   case IEEE8023_PKT:
   case SNAP8023_PKT:
	if((lla_ptr->lla_flags & LLA_IS_ETHER) ||
	   (lla_ptr->lla_flags & LLA_IS_8025))  { /* lla block is associated 
					            with Ethernet device */
		LAN_UNLOCK(lla_ptr->lla_lock);
		m_freem(mbuf_ptr);
		LAN_LOCK(lanc_ift_ptr->mib_lock);
             	mib_xptr->if_DInErrors++;
		LAN_UNLOCK(lanc_ift_ptr->mib_lock);
	     	return(0);
	}
	break;
   case ETHERXT_PKT: /* these two formats not currently supported in LLA */
   case SNAP8023XT_PKT:
	LAN_UNLOCK(lla_ptr->lla_lock);
	m_freem(mbuf_ptr);
	LAN_LOCK(lanc_ift_ptr->mib_lock);
        mib_xptr->if_DInErrors++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
	return(0);
	break;

   case IEEE8025_PKT: 
   case SNAP8025_PKT:
        if(!(lla_ptr->lla_flags & LLA_IS_8025)) { /* lla block not associated
                                                     with 8025 device */
		LAN_UNLOCK(lla_ptr->lla_lock);
	        m_freem(mbuf_ptr);
		LAN_LOCK(lanc_ift_ptr->mib_lock);
                mib_xptr->if_DInErrors++;
		LAN_UNLOCK(lanc_ift_ptr->mib_lock);
	        return(0);
        }
	break;
   }

/*
 * 	check that there is room in the lla queue for this data
 * 	if there is no room, then drop the packet.  If the user has
 * 	set up for signal norification, then notify through the pid.
 */
   if (lla_ptr->lla_msgsqd >= lla_ptr->lla_maxmsgs) {
     	if (lla_ptr->lan_signal_mask & LLA_Q_OVERFLOW) {
         	psignal(lla_ptr->lan_signal_pid,SIGIO);
	}
	LAN_UNLOCK(lla_ptr->lla_lock);
     	m_freem(mbuf_ptr);
	LAN_LOCK(lanc_ift_ptr->mib_lock);
     	mib_xptr->if_DInDiscards++;
	LAN_UNLOCK(lanc_ift_ptr->mib_lock);
     	return(0);
   }
/*
 *  link the last message in the lla queue buffer to this mbuf
 *  Note : this logic has been changed as of 9.0 to remove
 *         the dependence on the mbuf MF_EOM indicator.
 *  The m_next of the last mbuf in the current packet chain is set to NULL
 *  already by the low layer driver.  To link the last message in the lla
 *  queue buffer we need to link the m_act field of the last mbuf to the
 *  beginning of the current packet chain.
 */
   if (lla_ptr->lla_msgsqd == 0)
    	lla_ptr->head_packet = mbuf_ptr;
   else
        lla_ptr->last_packet->m_act = mbuf_ptr;

   (int)lla_ptr->lla_msgsqd++;

/*
 * loop to the end of this packet to adjust the lla_ptr->last_packet
 * pointer.
 */
   while (mbuf_ptr->m_next)
    	mbuf_ptr = mbuf_ptr->m_next;
   lla_ptr->last_packet = mbuf_ptr;

/*
 * 	Check if the user has configured for signaling on packet receipt,
 * 	if so, send the signal
 */
   if(lla_ptr->lla_flags & LLA_WAIT) {
    	lla_ptr->lla_flags &= ~LLA_WAIT;
	s=sleep_lock();
    	wakeup((caddr_t)&lla_ptr->lla_flags);
	sleep_unlock(s);
	LAN_UNLOCK(lla_ptr->lla_lock);
   } else {
    	if (lla_ptr->lan_signal_mask & LLA_PKT_RECV) {
        	psignal(lla_ptr->lan_signal_pid,SIGIO);
	}
    	if (lla_ptr->lla_rsel) {
		struct proc *tmp_rsel = lla_ptr->lla_rsel;
		int tmp_flags = lla_ptr->lla_flags;

		lla_ptr->lla_rsel = NULL;
		lla_ptr->lla_flags &= ~LLA_SELECT_RCOLL;
		LAN_UNLOCK(lla_ptr->lla_lock);
		selwakeup(tmp_rsel, (tmp_flags & LLA_SELECT_RCOLL));
        } else {
		LAN_UNLOCK(lla_ptr->lla_lock);
	}
   }
   return(0);
} /* end of lanc_lla_input */

