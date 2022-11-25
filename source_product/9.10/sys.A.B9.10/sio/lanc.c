/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/lanc.c,v $
 * $Revision: 1.16.83.6 $       $Author: randyc $
 * $State: Exp $        $Locker:  $
 * $Date: 94/02/03 09:57:46 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
#ifndef lint
static char HPPROD_ID[]="@(#) FILESET LAN: lib lan: Version: A.09.03";
#endif
 
#if  defined(MODULEID) && !defined(lint)
static char rcsid[] = "@(#) lanc.c  Revision: A.09.03";
#endif



#include "../h/buf.h"
#include "../h/protosw.h"   
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/errno.h"
#include "../h/mbuf.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
#include "../h/netfunc.h"
#include "../h/mp.h"

#include "../net/if.h"
#include "../net/netisr.h"

#include "../netinet/in.h"
#include "../netinet/in_var.h"
#include "../netinet/if_ether.h"
#include "../netinet/if_probe.h"
#include "../netinet/if_ieee.h"

#include "../nipc/nipc_hpdsn.h"

#include "../sio/netio.h"
#include "../sio/lanc.h"
#include "../sio/lla.h"

#include "../netinet/mib_kern.h" 


/*
 *	Forward and external function calls.
 */

int	lanc_init();
int	lanc_watch();
int	lanc_netcall_aarpinput();
int     lanc_ctl_sleep();
struct  logged_link *lanc_lookup();
struct  timeval  ms_gettimeofday();

extern  int	lanc_if_output();
extern  int	lanc_if_resolved_output();
extern	int	lanc_if_ioctl();
extern	int	lanc_if_control();
extern	int	lanc_media_control();
extern	void	lanc_802_2_test_ctl();
extern	void    lanc_ether_trail_intr();
extern	void	if_attach();
extern	void	arpintr();
extern	void	schednetisr();
extern  void    nmevenq();

/* Global plabel definition for hp_dlpi_wakeup function */
void	(*hp_dlpi_wakeup_plabel)()=NULL;

/* Definition for global DLPI promiscuous list */
caddr_t	hp_dlpi_promisc_list[10];

/* Global plabel definition for hp_dlpi_intr function */
void	(*hp_dlpi_intr_plabel)()=NULL;

/*
 *	Global LAN variables.
 */
int lan_hdrlen[] = {
	ETHER_HLEN,
	IEEE8023XSAP_HLEN,
	SNAP8023_HLEN,
	ETHERXT_HLEN,
	SNAP8024_HLEN,
	IEEE8023_HLEN,
	SNAP8023XT_HLEN,
	SNAP8024XT_HLEN,
	0,
	SNAP8025_HLEN,
	IEEE8025_HLEN
};

/*
 *	Local variables.
 */

int lan_nio_sys_card_count = 0;

int log_hashmask = (1 << HASHBITS) - 1; /* masks out all but the low order
                                         * HASHBITS number of bits.
                                         */
/*
 *	External variables.
 */
extern	struct logged_link	*log_hash_ptr[];
lock_t *log_lock;	/* protects log_hash_ptr[] */
#define LAN_LOG_LOCK_ORDER 14

#ifdef __hp9000s800
extern lan_ift          *lan_cio_ift_ptr[];
extern lan_ift          *lan_nio_ift_ptr[];
extern struct timeval   sysInit;
#endif /* __hp9000s800 */


/* BEGIN_IMS lanc_attach
***************************************************************************
****								
****		lanc_attach ( lanc_ift_ptr, lan_unit )
****								
***************************************************************************
* Description
*       Update I/O mapping and interface procedure handles.
*       Attach ifnet structure to linked list 
*
***************************************************************************
* END_IMS lanc_attach */

lanc_attach (lanc_ift_ptr, lan_unit, mtu, ac_type, ac_flags)
register lan_ift *lanc_ift_ptr;
int lan_unit;
int mtu;	/* mtu size */
int ac_type;	/* media access type */
int ac_flags;	/* media access flags */

{  /* Beginning of lanc_attach function */

   register struct ifnet *ifnet_ptr = &lanc_ift_ptr->is_if;
   int lock_size;

   mib_ifEntry *mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);

   ifnet_ptr->if_unit = (short)lan_unit;
   ifnet_ptr->if_name = "lan";
   ifnet_ptr->if_mtu = (short)mtu;
   ifnet_ptr->if_flags &= ~IFF_UP; 
   ifnet_ptr->if_flags |= IFF_BROADCAST|IFF_RUNNING|IFF_NOTRAILERS;
   ((struct arpcom *)ifnet_ptr)->ac_type = (u_short)ac_type;
   ((struct arpcom *)ifnet_ptr)->ac_flags = (u_short)ac_flags;
   ((struct arpcom *)ifnet_ptr)->ac_output = lanc_if_resolved_output;

/* Set up all the lan common routine names in the switch table.           */

   ifnet_ptr->if_init = lanc_init;
   ifnet_ptr->if_output = lanc_if_output;
   ifnet_ptr->if_ioctl = lanc_if_ioctl;
   ifnet_ptr->if_control = lanc_if_control;

/* Set up the interface's statistics timer/watchdog info.  */

   ifnet_ptr->if_watchdog = lanc_watch;
   lanc_ift_ptr->is_scaninterval = LANWATCHINTERVAL;
   ifnet_ptr->if_timer = LANWATCHINTERVAL;

/* allocate and initialize spinlocks */
   LAN_ALLOC_LOCK(lanc_ift_ptr->lanc_lock);
   LAN_ALLOC_LOCK(lanc_ift_ptr->mib_lock);
   if(!lanc_ift_ptr->lanc_lock || !lanc_ift_ptr->mib_lock) {
	panic("lanc_attach:  failed to allocate memory for spinlocks");
   }
   LAN_INIT_LOCK(lanc_ift_ptr->lanc_lock,LAN_LANC_LOCK_ORDER,"lan:lanc_lock");
   LAN_INIT_LOCK(lanc_ift_ptr->mib_lock,LAN_MIB_LOCK_ORDER,"lan:mib_lock");

LAN_ALLOC_LOCK(lanc_ift_ptr->ac_lock);
LAN_INIT_LOCK(lanc_ift_ptr->ac_lock,AC_LOCK_ORDER,"lan-tmp:ac_lock");
LAN_ALLOC_LOCK(lanc_ift_ptr->if_lock);
LAN_INIT_LOCK(lanc_ift_ptr->if_lock,IF_LOCK_ORDER,"lan-tmp:if_lock");

/* Call if_attach to link this ifnet structure into the linked list. */

   if_attach(ifnet_ptr);	

/* Initialize MIB parameters */

   mib_ptr->ifIndex = ifnet_ptr->if_index;
   mib_ptr->ifOper = LINK_DOWN;
   mib_ptr->ifAdmin = LINK_UP;
   mib_ptr->ifLastChange  = (TimeTicks)0;
   switch (ac_type) {
   case ACT_8023:	/* 802.3 media */ 
	mib_ptr->ifSpeed = ETHER_BANDWIDTH;
	break;
   case ACT_FDDI:
	mib_ptr->ifSpeed = FDDI_BANDWIDTH;
	break;
   case ACT_8025:       /* 802.5 media */
        /* NOTE this is actually selectable - 4/16 Mbs */
        /* We initially set it to 16, although it may  */
        /* be reset later when we query the card       */
        mib_ptr->ifSpeed = TOKEN_16_BANDWIDTH;
        break;
   default: 
	break;
   }
}  /* Ending of lanc_attach function */


/*******************************************************************
****
****  lanc_init ( ifnet_ptr )
****
********************************************************************
* Purpose
*    Initialization of the interface driver and the Device Adapter.
*    It assures that the current LAN configuration (default values
*    at boot time) is correctly downloaded to the LAN card.
*    It also initializes the 1-second watchdog timer, and initializes
*    interface routing info.
*
*
*******************************************************************/

lanc_init(ifnet_ptr)
struct ifnet *ifnet_ptr;

{  /* Beginning of lanc_init function */

   register int lan_unit = ifnet_ptr->if_unit;      
   register lan_ift *lanc_ift_ptr = (lan_ift*) ifnet_ptr;
   int i;

   if (!(lanc_ift_ptr->hdw_state & LAN_DEAD)) {
	/* Call hw_req to initiate the download of the card's configuration */
	(*lanc_ift_ptr->hw_req) (lanc_ift_ptr, LAN_REQ_INIT, (caddr_t)NULL);

   } 

   /* logging Trailer Type Addresses */

#ifdef LAN_TEST_TRAILERS
   for (i = ETHERTYPE_TRAIL; i < ETHERTYPE_TRAIL+4; i++) {
#else
   for (i = ETHERTYPE_TRAIL+1; i < ETHERTYPE_TRAIL+4; i++) {
#endif
	(void)lanc_log_protocol(LAN_TYPE, i, lanc_ether_trail_intr,
			ifnet_ptr, FALSE, TRUE, FALSE, 0, 0, 0);

        (void)lanc_log_protocol(LAN_SNAP, i >> 8, lanc_ether_trail_intr,
                        ifnet_ptr, FALSE, TRUE, FALSE, 0, 0, i << 24);
   }

   return(0);
} /* end of lanc_init */


/* BEGIN_IMS lanc_watch
***************************************************************************
****								
****		lanc_watch (lanc_ift_ptr)
****								
***************************************************************************
* Description
*
*    Watch dog routine.       
*    lanc_watch sets flag to collect card statistics every LANWATCHINTERVAL 
*    seconds.  It also maintains driver's DMA/CTRL request timer.
*    On 300, does nothing.
*
* Notes
*	- Executes only on the ICS.
*
*
***************************************************************************
* END_IMS lanc_watch */

lanc_watch (lanc_ift_ptr)
lan_ift *lanc_ift_ptr;

{  /* Beginning of lanc_watch function */
	
   register struct ifnet *ifnet_ptr = &lanc_ift_ptr->is_if;
   register lan_timer *timer = &lanc_ift_ptr->lantimer;

#if !defined(__hp9000s800) || !defined(VOLATILE_TUNE)
   extern char *panicstr;
#endif /* !__hp9000s800 || !VOLATILE_TUNE */

   if (panicstr)
  	return(0);

/* If we have a DMA/CTRL timer set, decrement it.                         */
/* If it has timed out, mark the card dead and call the hw timer event    */
/* function in the lan_ift.                                               */
	
   LAN_LOCK(lanc_ift_ptr->lanc_lock);
   if (timer->flag) {   /* DMA/ctrl timer is on */
  	timer->timer--;
  	if (timer->timer <= 0) {  /* Timer has expired */
    		timer->flag = 0;
		LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
    		(*lanc_ift_ptr->dma_time)(lanc_ift_ptr);
    	}  /* Timer has expired */
   }   /* DMA/ctrl timer is on */
   if(LAN_OWNS_LOCK(lanc_ift_ptr->lanc_lock)) {
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
   }

/* Call hw_req to initiate status req and set the stat pending flag. */

   (*lanc_ift_ptr->hw_req) (lanc_ift_ptr, LAN_REQ_DOG_STAT, (caddr_t)NULL);
   IF_LOCK(ifnet_ptr);
   ifnet_ptr->if_timer = lanc_ift_ptr->is_scaninterval;
   IF_UNLOCK(ifnet_ptr);
   return (0);

}  /* Ending of lanc_watch function */


lanc_ctl_sleep (lanc_ift_ptr, status)
lan_ift *lanc_ift_ptr;
int status;
{
   int s,error=0;

#ifdef LAN_REAL_LOCKS
   s=sleep_lock();
   if(LAN_OWNS_LOCK(lanc_ift_ptr->lanc_lock)) {
	LAN_UNLOCK(lanc_ift_ptr->lanc_lock);
   } 
   sleep_then_unlock(-status, PRIBIO-1,s);
#else
   sleep(-status, PRIBIO-1);
#endif
   if (lanc_ift_ptr->hdw_state & LAN_DEAD) {
	error=ENXIO;
   }
   return(error);
} /* lanc_ctl_sleep */


/*   - Create a routine for calling aarpinput() via a NETCALL
     - Gives an address for registering the sap, and separates
       need to have APPLETALK in the kernel with the LAN driver.
*/
lanc_netcall_aarpinput(ac, m)
register struct arpcom *ac;
struct mbuf *m;
{
    int result;

    result = NETCALL(NETDDP_AARPINPUT) (ac,m);
    if (result == ENOSYS) {
	/* No aarpinput() routine in this system.
	** We must free the mbuf chain, and return.
	*/
	m_freem(m);
	result = 0;
    }
    return(result);
}


int 
lanc_setup_protocol_logging(ifp, cmd, data_ptr, netisr_event,
			    q_or_function)
struct ifnet *ifp;
int cmd;
caddr_t data_ptr, netisr_event, q_or_function;

{
   int error=0;
   lan_ift *lanc_ift_ptr= (lan_ift *)ifp;

   switch (cmd) {
   case IFC_IFBIND:
	switch ((int)data_ptr) {
	case AF_INET:
	        lanc_log_protocol(LAN_TYPE, ETHERTYPE_IP, 
		          q_or_function, ifp, TRUE, FALSE, TRUE, 0,
		          netisr_event, 0);
		lanc_log_protocol(LAN_SNAP, ETHERTYPE_IP >> 8,
                          q_or_function, ifp, TRUE, FALSE, TRUE, 0, 
                          netisr_event, ETHERTYPE_IP << 24);
		lanc_log_protocol(LAN_SAP, IEEESAP_IP, 
			  q_or_function, ifp, TRUE, FALSE, TRUE, 0, 
			  netisr_event, 0);
		/* also log arp TYPE -- runs on ICS (i.e. NOT thru netisr)*/
		lanc_log_protocol(LAN_TYPE, ETHERTYPE_ARP, 
			  arpintr, ifp, TRUE, TRUE, TRUE, 0, 0, 0);
		lanc_log_protocol(LAN_SNAP, ETHERTYPE_ARP >> 8,
			  arpintr, ifp, TRUE, TRUE, TRUE, 0, 0, 
			  ETHERTYPE_ARP << 24);
                arpinit();

		return(0);

#ifdef APPLETALK
	case AF_APPLETALK:      /* log ddp */
       		error = lanc_log_protocol(LAN_TYPE, ETHERTYPE_APPLETALK,
			  q_or_function, ifp, TRUE, FALSE, TRUE, 0, 
			  netisr_event, 0);

		if (error)
                       return(error);

                error = lanc_log_protocol(LAN_TYPE, ETHERTYPE_AARP,
		          lanc_netcall_aarpinput, ifp, TRUE, TRUE, TRUE,
			  0, 0, 0);

                return(error);
#endif /* APPLETALK */

	default:
		return(0);
	}

   case IFC_IFATTACH:
	return(0);

   case IFC_IFDETACH:
        return(0);

   case IFC_IFPATHREPORT:
	{
/*
 * 	For IFC_IFPATHREPORT, s1 points to the buffer where we
 * 	build the path report. The first two bytes of the path
 * 	report contains the length of the path report.
 */
		u_short *sptr, *length;
		u_long ip_addr;
		struct arpcom *ac = (struct arpcom *)ifp;
		struct in_ifaddr *ia;

		length = sptr = (u_short *)data_ptr;
		*length = 0;
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
			if (ia->ia_ifp == ifp)
				break;
		if (ia == (struct in_ifaddr *) 0)       /* not configured */
			return(0);
		if ((ip_addr = IA_SIN(ia)->sin_addr.s_addr) == 0)
			return(0);
		AC_LOCK(ac);
		if ((!(ac->ac_flags & ACF_ETHER)) && 
				(!(ac->ac_flags & ACF_IEEE8023))) {
			AC_UNLOCK(ac);
			return(0);
		}

		sptr++;				/* skip length field */
		*sptr++ = ((HPDSN_VER << 8) + (HPDSN_DOM));
		*sptr++ = ip_addr >> 16;
		*sptr++ = ip_addr & 0xFFFF;
		*length += 3 * sizeof(u_short);
		if (ac->ac_flags & ACF_ETHER) {
			*sptr++ = 12 + 10;
	    		*sptr++ = (NSP_SERVICES << 8) + 2;
			*sptr++ = (NSB_NFT | NSB_IPCSR | NSB_LOOPBACK);
			*sptr++ = (NSP_TRANSPORT << 8) + 2;
			*sptr++ = (NSB_TCPCKSUM | NSB_TCP | NSB_HPPXP);
			*sptr++ = (NSP_IP << 8) + 2;
			*sptr++ = 0;
			*sptr++ = (NSP_ETHERNET << 8) + 8;
			*sptr++ = (ETHERTYPE_IP);
			bcopy((caddr_t)ac->ac_enaddr, (caddr_t)sptr, 6);
			sptr += 3;
			*length += 12 * sizeof(u_short);
		}
		if (ac->ac_flags & ACF_IEEE8023) {
			*sptr++ = 12 + 10;
			*sptr++ = (NSP_SERVICES << 8) + 2;
			*sptr++ = (NSB_NFT | NSB_IPCSR | NSB_LOOPBACK);
			*sptr++ = (NSP_TRANSPORT << 8) + 2;
			*sptr++ = (NSB_TCPCKSUM | NSB_TCP | NSB_HPPXP);
			*sptr++ = (NSP_IP << 8) + 2;
			*sptr++ = 0;
			*sptr++ = (NSP_IEEE802 << 8) + 8;
			*sptr++ = (IEEESAP_IP);
			bcopy((caddr_t)ac->ac_enaddr, (caddr_t)sptr, 6);
			sptr += 3;
			*length += 12 * sizeof(u_short);
		}
		AC_UNLOCK(ac);
		return(0);
	}

   case IFC_OSIBIND:
	{
		/* This is special code which allows OSI to have
		 * configurable SAP's.
		 */
                if (error = lanc_log_protocol(LAN_SAP,
			    ((struct fis *)data_ptr)->value.i, q_or_function,
                            ifp, FALSE, TRUE, TRUE, 0, 0))
			return(error);
		return(error);
	}

   case IFC_OSIUNBIND:
	{
		/* This is special code which allows OSI to have
		 * configurable SAP's.
		 */
                if (lanc_lookup(((struct fis *)data_ptr)->value.i, LAN_SAP,
				ifp, 0) == (struct logged_link *)NULL)
                        return(EINVAL);
		(void) lanc_remove_protocol( LAN_SAP,
			((struct fis *)data_ptr)->value.i, ifp, 0);
		return(0);
	}

   case IFC_PRBVNABIND:
	{
		struct fis mcast;
		extern char prb_mcast[][6];

		if (error = lanc_log_protocol(LAN_CANON, IEEEXSAP_PROBE, 
			    q_or_function, ifp, FALSE, TRUE, TRUE, 0, 0, 0))
			return(error);
		mcast.reqtype = ADD_MULTICAST;
		mcast.vtype = 6;
		bcopy((caddr_t)prb_mcast[PRB_VNA], (caddr_t)mcast.value.s, 6);
		error = lanc_media_control(ifp, &mcast, LAN_REQ_NOT_BLOCK);
		return(error);
	}

   case IFC_PRBPROXYBIND:
	{
		struct fis mcast;
		extern char prb_mcast[][6];

                if (lanc_lookup(IEEEXSAP_PROBE, LAN_CANON, ifp, 0) ==
                        (struct logged_link *)NULL)
			return(EINVAL);
		mcast.reqtype = ADD_MULTICAST;
		mcast.vtype = 6;
		bcopy((caddr_t)prb_mcast[PRB_PROXY],(caddr_t)mcast.value.s,6);
		error = lanc_media_control(ifp, &mcast, LAN_REQ_NOT_BLOCK);
		return(error);
	}

   case IFC_PRBPROXYUNBIND:
	{
		struct fis mcast;
		extern char prb_mcast[][6];

                if (lanc_lookup(IEEEXSAP_PROBE, LAN_CANON, ifp, 0) ==
                        (struct logged_link *)NULL)
                        return(EINVAL);
		mcast.reqtype = DELETE_MULTICAST;
		mcast.vtype = 6;
		bcopy((caddr_t)prb_mcast[PRB_PROXY],(caddr_t)mcast.value.s,6);
		error = lanc_media_control(ifp, &mcast, LAN_REQ_NOT_BLOCK);
		return(error);
	}
   }
}


/* BEGIN_IMS lanc_init_log_struct
 **************************************************************************
 ****
 ****			lanc_init_log_struct()
 ****
 **************************************************************************
 * Description
 *      Initializes the SAP/ Type/ Canonical Address logging structure    
 *      with the addresses and protocol input routines of layer 3 protocols.
 *    
 *      All Layer 3 protocols are logged here and no other function need
 *      be modified if a new protocol is to be added.
 *
 * Input Parameters
 *      none
 *
 * Return Value
 *      none
 *
 * Algorithm
 *      Zero out array of pointers to log-structures.      
 * 
 * External Calls
 *	ma_create, NS_LOG, NS_LOG_STR, lanc_log_protocol
 *
 * Called By
 *      lan0_dobind, lan1_dobind	
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *****************************************************************************
 */
/* This 'global' variable is used to insure that the list is not init.    */
/* more than once.                                                        */

static int log_struct_init_done = 0;

void 
lanc_init_log_struct()

{  /* Beginning of lanc_init_log_struct function */

   int i, hash_array_size;
   
   if (log_struct_init_done++) {
	return;
   }

   /* allocate log_lock */
   LAN_ALLOC_LOCK(log_lock);
   if(!log_lock) {
	panic("lanc_init_log_struct:  failed to allocate memory for spinlock");
   }
   LAN_INIT_LOCK(log_lock,LAN_LOG_LOCK_ORDER,"lan:log_lock");

  /* Zero out array, each of the elements of which form the head of 
   * a linked list 
   */
   hash_array_size = (1 << HASHBITS);
   for (i = 0; i < hash_array_size; i++) { 
 	log_hash_ptr[i] = NULL;
   }
}


/* BEGIN_IMS lanc_lookup
***************************************************************************
****
****			lanc_lookup
****
***************************************************************************
* Description
*       Given an integer value (protocol_val) and an index (protocol_kind)
*	identifying which of a SAP, Type or Canonical address it represents, 
*       and an ifnet pointer identifying the interface, this function does 
*       a lookup of the log structure list and returns a pointer to the 
*       structure that matches the set of input parameters. If no matching 
*       structure was  logged, a NULL pointer is returned.
*
* Input Parameters  
*	protocol_val    Number value of SAP, Type or Canonical Address
*	protocol_kind   Integer identifying whether the protocol_val above is of
*                  	LAN_SAP, LAN_TYPE, or LAN_CANON
*       input_ifp  	ifnet pointer of interface card
*
* Return Value
*	Pointer to logged_link stucture.
*
* Algorithm
*       Obtain index into array of list head pointers by zeroing
*       out all but the rightmost HASHBITS number of bits of protocol_val.
*       We now have a pointer to the head of a linked list. 
*       Search this list linearly till the logged protocol_val (indexed by
*	protocol_kind) matches the input value of protocol_val.
*       Now if the ifnet_ptr is global (-1), or the input_ifp value is
*       global (-1) or logged_ifp equals the input_ifp, return pointer to
*       logged structure else continue search. 
*       If end of list is reached, return NULL (not logged).
*		
* External Calls
*       None
*
* Called By
*	lanc_802_2_ics, lanc_ether_ics, lanc_log_protocol, lanc_remove_protocol
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
* END_IMS lanc_lookup */
struct logged_link *
lanc_lookup(protocol_val, protocol_kind, input_ifp, protocol_val_ext)
register int protocol_val;  	/* value of SAP, TYPE or CANONical address  */
register int protocol_kind;     /* LAN_TYPE, LAN_SAP, or LAN_CANON          */
register struct ifnet *input_ifp; /* pointer to ifnet of interface card     */
register int protocol_val_ext;    /* extention of STC value used by SNAP    */


{
   register struct logged_link * link_ptr;
   register struct ifnet *logged_ifp ;
      
   LAN_LOCK(log_lock);
   for (link_ptr = log_hash_ptr[protocol_val & log_hashmask]; link_ptr != NULL;
        link_ptr = link_ptr->next) {
        if ((link_ptr->log.protocol_val[protocol_kind] == protocol_val) &&
            ((LAN_SNAP != protocol_kind) || (link_ptr->log.protocol_val[protocol_kind+1] == protocol_val_ext))) {
    		logged_ifp = link_ptr->log.ifnet_ptr;
         	if (logged_ifp == input_ifp) {
			LAN_UNLOCK(log_lock);
            		return(link_ptr);
		}
 	}
   }
   LAN_UNLOCK(log_lock);
   return(NULL);
}




/* BEGIN_IMS lanc_log_protocol
***************************************************************************
****
****			lanc_log_protocol
****
***************************************************************************
* Description
*       log a SAP, Type or Canonical Address so that incoming packets
*       are routed by the driver to the corresponding protocol
*
* Input Parameters  
*       logged_kind  	 LAN_TYPE, LAN_SAP, LAN_SNAP or  LAN_CANON
*       protocol_val 	 SAP, Type, or Cannonical value
*       q_or_func    	 protocol input routine pointer if on_ics is 
*                        true (see below) otherwise the protocol ifqueue pointer
*       ifnet_ptr    	 Pointer to ifnet of interface to be logged
*                    	 (negative value implies protocol logged for all LAN
*		     	 interfaces)
*       strip_header 	 non-zero if header to be stripped
*       on_ics       	 non-zero if protocol to run on ICS (i.e. at splimp()
*                    	 on driver stack -- _not_ thru netisr())
*       check_flags  	 non-zero if IFF_UP or ACF_* flags should be checked
*       lu_protocol_info used to store EVENT for protocols that run on
*                        netisr (i.e. on_ics is false), and also used to store 
*                        rawcb pointer for LLA (which runs on the ICS)
*       protocol_val_ext extention of STC for SNAP
*
* Return Value
*	EINVAL       protocol_val or logged_kind are invalid
*       EBUSY        SAP/Type/Canonical Address has already been logged
*       ENOMEM       no mbufs available for logging structure
*       0            Logged successfully
*
* Algorithm
*       Check to see if the protocol_val is out of range or is reserved. (The
*       actual range or reserved values depend on the protocol_kind i.e. if
*       protocol_kind is LAN_SAP then protocol_val == 5000 is invalid)
*       Return EINVAL if value is illegal or EBUSY if it is reserved.
* 
*       Now check to see if the layer 3 protocol has been previously logged and
*       return EBUSY if so.
*
*       Obtain an mbuf, cast it as a logged_link struct and store the
*       logging information in it.  Attach this mbuf to the hashed-list
*       logging stucture. This "hashed-list" logging structure is designed
*       to facilitate fast lookup and is shown below.
*       ______________        ___       ___       ___
*       |  list ptr  | ----->/log\---->/log\---->/log\---->0
*       |____________|       \___/     \___/     \___/		
*       |  list ptr  | --->0
*       |____________|        ___ 
*       |  list ptr  | ----->/log\---->0
*       |____________|       \___/    
*       |  list ptr  | ---->0
*       |____________|		
*            ...      
*       |____________|        ___ 
*       |  list ptr  | ----->/log\---->0
*       |____________|       \___/    
*
*       The index into the array of list pointers is obtained by zeroing
*       out all but the low order HASHBITS bits of protocol_val. The value at
*       that index is a pointer to the head of a linked list. The logging
*       structure associated with the protocol to be logged is inserted
*       in this linked list sorted by time of logging. The last structure
*       to be logged is at the end of the linked list. This way, if the 
*       protocol_val's for two protocols map to the same array index, lookup 
*       favors the protocol that has been logged first.
*       
* External Calls
*       lanc_lookup
*
* Called By
*	lanc_init_log_struct (and by LLA functions)
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
* END_IMS lanc_log_protocol */
int 
lanc_log_protocol(logged_kind, protocol_val, q_or_func, ifnet_ptr,
                      strip_header, on_ics, check_flags, raw_8023,
		      lu_protocol_info, protocol_val_ext)

   int logged_kind;  	    /* LAN_TYPE, LAN_SAP, LAN_CANON                   */
   int protocol_val;        /* SAP, Type, or Cannonical value                 */
   void (*q_or_func)();     /* protocol input routine pointer if on_ics       */
                            /* true else input queue pointer		      */
   struct ifnet *ifnet_ptr; /* Pointer to ifnet of interface to be logged     */
                            /* negative value implies protocol logged for all */
			    /* LAN interfaces                                 */
   int strip_header; 	    /* non-zero if header to be stripped              */
   int on_ics;       	    /* non-zero if protocol to run on ICS             */
   int check_flags;  	    /* non-zero if IFF_* flags should be checked      */
   int lu_protocol_info;    /* used to store EVENT for protocols that run on  */
                     	    /* netisr, and also used to store rawcb pointer   */
                     	    /* for LLA (which runs on the ICS)      	      */
   int protocol_val_ext;    /* extention of STC for SNAP */
{
   register struct logged_link *link_ptr;
   struct logged_link *hash_ptr;
   int i;
   lan_ift *lanc_ift_ptr = (lan_ift *)ifnet_ptr;
   struct ifnet *ifp;
   int error = 0;

   /* we no longer support global interface protocol logging */
   if (ifnet_ptr == (struct ifnet *) -1)
	return (EINVAL);

   switch(logged_kind) {
   case LAN_SAP:
         /* check for illegal SAP values */
   	if ((protocol_val >= NSAPS) || (protocol_val < 0))
          	return(EINVAL);
         /* check for reserved SAPs (canonical addressing, SNAP) */
   	if ((protocol_val == IEEESAP_HP) || (protocol_val == IEEESAP_NM) ||
	    (protocol_val == IEEESAP_SNAP))
            	return(EBUSY);
        break;

   case LAN_TYPE:
         /* check for illegal TYPE values */
        if ((protocol_val > 0xffff) || (protocol_val < MIN_ETHER_TYPE))
            	return(EINVAL);
         /* check for Types reserved for canonical addressing */
        if (protocol_val == HP_EXPANDTYPE)
          	return(EBUSY);
        break;

   case LAN_CANON:
         /* check for reserved canonical address */
        if (protocol_val == 0)
       		return(EBUSY); 
   case LAN_SNAP:
        break;

   default:
        return(EINVAL);
   }

   if ((link_ptr = (struct logged_link *)
       lanc_lookup(protocol_val, logged_kind, ifnet_ptr, protocol_val_ext)) != NULL)
	return(EBUSY);

   /*Get a memory buffer to store logging info for new SAP or type */
   MALLOC(link_ptr, struct logged_link *, sizeof(struct logged_link), 
          M_DYNAMIC, M_NOWAIT); 
   if (link_ptr == NULL)
  	return(ENOMEM);

   /* Stick logged_link structure in the buffer and link it to the 
      list of xsap_link structures. */

   link_ptr->log.rint = q_or_func;
   link_ptr->log.ifnet_ptr = ifnet_ptr;
   link_ptr->log.lu_protocol_info = lu_protocol_info;
   link_ptr->log.flags = strip_header ? LANC_STRIP_HEADER : 0;
   link_ptr->log.flags |= on_ics ? LANC_ON_ICS : 0;
   link_ptr->log.flags |= check_flags ? LANC_CHECK_FLAGS : 0;
   link_ptr->log.flags |= raw_8023 ? LANC_RAW_802MAC : 0;

   for(i=0;i<4;i++)
   	link_ptr->log.protocol_val[i] = -1;
   link_ptr->log.protocol_val[logged_kind] = protocol_val;
   if (LAN_SNAP == logged_kind)
      link_ptr->log.protocol_val[logged_kind+1] = protocol_val_ext;

   link_ptr->next = NULL;

   error = (*lanc_ift_ptr->hw_req)(lanc_ift_ptr, LAN_REQ_PROTOCOL_LOG, 
				        (caddr_t)link_ptr);

   if (error) {  /* hardware is not able to log the protocol */
	FREE(link_ptr, M_DYNAMIC);	/* free the log entry */
	return(EINVAL);
   }

   /* Add node to *end* of linked list - during lookup this favors 
    * types that were logged first.
    * For singly linked lists, to facilitate insertion and deletion, it
    * is useful to keep an unused (blank) list node as the first list item.
    * However, since in this case that would mean 40 * 256 wasted bytes, some
    * tricks involving casting of pointers have been used so that an item
    * in the array log_hash_pointer is made to appear as though it is a
    * struct logged_link, whereas in reality it is declared as a pointer
    * to the struct logged_link (4 * 256 bytes). In order for this to work,
    * "next" should always be the first field of struct logged_link, and
    * the code should not refer to any other field when using the first
    * item in a list (which is also an element of the array log_hash_pointer).
    */
   
   LAN_LOCK(log_lock);
   hash_ptr=(struct logged_link *)&(log_hash_ptr[protocol_val & log_hashmask]);
   for(;hash_ptr->next != NULL; hash_ptr = hash_ptr->next) ;
   hash_ptr->next = link_ptr;
   LAN_UNLOCK(log_lock);

   NS_LOG_INFO((LE_LOG_SAP + logged_kind), NS_LC_PROLOG, NS_LS_DRIVER, 0, 2,
	       protocol_val, ((lan_ift *)ifnet_ptr)->is_if.if_unit);
   return(0);
}


/* BEGIN_IMS lanc_remove_protocol
***************************************************************************
****
****			lanc_remove_protocol
****
***************************************************************************
* Description
*       Remove a SAP, Type or Canonical Address so that
*       packets destined for that particular protocol will 
*       be dropped by the driver.
*
* Input Parameters  
*       logged_kind  LAN_TYPE, LAN_SAP, LAN_CANON
*       protocol_val      SAP/Type/Canonical value by logged_kind
*       ifnet_ptr    Pointer to ifnet of interface to be logged
*                    (negative value implies protocol logged for all LAN interfaces)
*       protocol_val_ext    SNAP extiontion
*
* Return Value
*       EINVAL	     The SAP, Type or Canon is not currently logged, 
*                    or is of a reserved type that cannot be removed.
*            0       protocol removed.
*
* Algorithm
*       For Types or SAPs, check protocol_val to see if it is reserved 
*       for extended canonical addressing. If so , return EINVAL.
*       Do a lanc_lookup to verify that the protocol has actually been
*       logged. If not, return EINVAL.
*       Remove the log struct from the linked list structure.
*		
* External Calls
*       NS_LOG, lanc_lookup
*
* Called By
*       LLA functions.
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
* END_IMS lanc_remove_protocol */
int 
lanc_remove_protocol(logged_kind, protocol_val, ifnet_ptr, protocol_val_ext)
int logged_kind;  /* LAN_TYPE, LAN_SAP, LAN_CANON                    */
int protocol_val;      /* SAP/Type/Canonical value by logged_kind         */
struct ifnet *ifnet_ptr; /* Pointer to ifnet of interface to be logged */
                           /* negative value implies protocol logged for all  */
			   /* LAN interfaces                             */
int protocol_val_ext;      /* extention protocol for SNAP */
{
   register struct logged_link *link_ptr, *hash_ptr;
   lan_ift *lanc_ift_ptr = (lan_ift *)ifnet_ptr;
   struct ifnet *ifp;
   int error = 0,s, streams_flg;

   streams_flg = ((logged_kind & STREAMS_LOG_MASK) ? 1 : 0);
   logged_kind = logged_kind & ~STREAMS_LOG_MASK;
   /* we no longer support global interface protocol logging */
   if (ifnet_ptr == (struct ifnet *) -1)
	return (EINVAL);

   /* check for extended types or SAPs, which cannot be removed */
   switch(logged_kind) {
   case LAN_SAP:
 	if (protocol_val == IEEESAP_HP || protocol_val == IEEESAP_NM)
        	return(EINVAL);      /* cannot remove these saps       */
        break;

   case LAN_TYPE:
        if (protocol_val == HP_EXPANDTYPE)
        	return(EINVAL);       /* cannot remove this types      */
        break;

   case LAN_CANON:
        if (protocol_val == 0)
                return(EINVAL);      /* cannot remove this Canonical address*/
   case LAN_SNAP:
        break;

   default:
   	return(EINVAL);    /* illegal value of logged_kind */

   }

     /* Deny removing request if:
      * a) entity has not been logged.
      * b) It has been logged globally, but request
      *    is to remove it for a specific interface.
      * c) It has been logged for specific inteface(s) but the request is
      *    to remove it globally.
      * d) It has been logged for a specific inteface(s), but not for the
      *    interface that is being requested.
      * Note that the function lanc_lookup will return a valid (non-NULL)
      * struct pointer even if conditions (b) and (c) above are violated. This
      * is because lanc_lookup was written to work correctly during lookup for
      * incoming packets. For the remove request here, we have to make an 
      * extra check after lanc_lookup returns.
      */
   link_ptr = (struct logged_link *)lanc_lookup(protocol_val, logged_kind,
						ifnet_ptr, protocol_val_ext);
   if (link_ptr == NULL || link_ptr->log.ifnet_ptr != ifnet_ptr)
         return(EINVAL);

#ifdef __hp9000s800
   s=spl5();
#endif

   error = (*lanc_ift_ptr->hw_req)(lanc_ift_ptr, LAN_REQ_PROTOCOL_REMOVE
					| LAN_REQ_BLK_MSK, (caddr_t)link_ptr);

   if ((error < 0) && (!streams_flg))
        error = lanc_ctl_sleep(lanc_ift_ptr, error);

#ifdef __hp9000s800
   splx(s);
#endif

   /* delete node from linked list and free mbuf which contained it */
   LAN_LOCK(log_lock);
   hash_ptr=(struct logged_link *) &(log_hash_ptr[protocol_val & log_hashmask]);
   for(;hash_ptr->next != link_ptr; hash_ptr = hash_ptr->next) ;
   hash_ptr->next = link_ptr->next;
   LAN_UNLOCK(log_lock);
   FREE(link_ptr, M_DYNAMIC);

   NS_LOG_INFO((LE_DROP_SAP + logged_kind), NS_LC_PROLOG, NS_LS_DRIVER, 0, 2,
	       protocol_val, ((lan_ift *)ifnet_ptr)->is_if.if_unit);
   return(error);
}


/**************         Beginning the lanc_if.c         ********************/

#ifdef __hp9000s800
/* BEGIN_IMS lanc_if_unit_init 
***************************************************************************
****							               ****
****		         lanc_if_unit_init ()                          ****
****							               ****
***************************************************************************
* Description
*	Create local lan_if_unit_info by taking information from
*	lan_nio_ift_ptr and lan_cio_ift_ptr
*	Sort the local array lan_if_unit_info according to lan
*	hardware path of ifterface cards
*	Assigne the if_unit in if_net structure
*
* Input Parameters:	(None)
*
* Output Parameters:	(none)
*
* Globals Referenced:
*	lan_nio_ift_ptr, lan_cio_ift_ptr, if_net structure
*
* Algorithm
*	binary sorting algorithm
*	
* To Do List:
*
* Notes:
*	- Executes only on the kernel stack.
*
*
* External Calls:
*
********************************************************************
* END_IMS lanc_if_unit_init */

struct lan_if_unit_info {
	int mjr_num;
	int mgr_index;
	int hdw_path[5];
};

lanc_if_unit_init()
{
   int 			i, j, k, n, gap;
   struct lan_if_unit_info temp_info, if_unit_info[10];
   struct ifnet	*ifnet_ptr;
   lan_ift	*lanc_ift_ptr;

   n = 0;
   for (i = 0; i < 10; i++) {  /* look for NIO */
	if (lan_nio_ift_ptr[i] != 0) {
		lanc_ift_ptr = lan_nio_ift_ptr[i];
	   	if_unit_info[n].mjr_num = lanc_ift_ptr->mjr_num;
	   	if_unit_info[n].mgr_index = lanc_ift_ptr->mgr_index;
	   	for (j = 0; j < 5; j++)
			if_unit_info[n].hdw_path[j] = lanc_ift_ptr->hdw_path[j];
	   	n++;
	}
   }
   for (i = 0; i < 10; i++) {  /* look for CIO */
	if (lan_cio_ift_ptr[i] != 0) {
	   	lanc_ift_ptr = lan_cio_ift_ptr[i];
	   	if_unit_info[n].mjr_num = lanc_ift_ptr->mjr_num;
	   	if_unit_info[n].mgr_index = lanc_ift_ptr->mgr_index;
	   	for (j = 0; j < 5; j++)
			if_unit_info[n].hdw_path[j] = lanc_ift_ptr->hdw_path[j];
	   	n++;
	}
   }
   for (gap = n/2; gap > 0; gap /= 2) {  /* sorting if_unit_info */
	for (i = gap; i < n; i++)
	   	for (j = i - gap; j >= 0; j -= gap) {
			k = 0;
			while (if_unit_info[j].hdw_path[k] ==
		       	       if_unit_info[j + gap].hdw_path[k]) {
				if (k == 4)
					break;
				else
					k++;
			}
			if (if_unit_info[j].hdw_path[k] <
		    	    if_unit_info[j + gap].hdw_path[k])
				break;

			temp_info = if_unit_info[j];
			if_unit_info[j] = if_unit_info[j + gap];
			if_unit_info[j + gap] = temp_info;
	   	}
   }
   for (i = 0; i < n; i++) {  /* adjust if_unit in ifnet struct */
	if (if_unit_info[i].mjr_num == LAN0_MJR_NUM) {  /* CIO */
	   	for (j = 0; j < 10; j++) {
			if (if_unit_info[i].mgr_index == 
		            lan_cio_ift_ptr[j]->mgr_index) {  /* found it */
		   		ifnet_ptr = (struct ifnet*) lan_cio_ift_ptr[j];
		   		ifnet_ptr->if_unit = i;   /* adjust if_unit */
		   		break;
			}
	   	}
	} else {  /* NIO */
	   	for (j = 0; j < 10; j++) {
			if (if_unit_info[i].mgr_index == 
		            lan_nio_ift_ptr[j]->mgr_index) {  /* found it */
		   		ifnet_ptr = (struct ifnet*) lan_nio_ift_ptr[j];
		   		ifnet_ptr->if_unit = i;   /* adjust if_unit */
		   		break;
			}
	   	}
	}
   }
}
#endif /* __hp9000s800 */


lanc_if_ioctl(ifnet_ptr, cmd, data_ptr)
register struct ifnet *ifnet_ptr;
int cmd;
register struct fis *data_ptr;
   
{
   register struct ifreq *ifr = (struct ifreq *)data_ptr;
 
   int        status = 0;
   lan_ift    *lanc_ift_ptr = (lan_ift *) ifnet_ptr;
   struct     sockaddr_in *sin;
   int		s;
 
   switch (cmd) {
   case SIOCSIFADDR:
	{
		struct ifaddr *ifa = (struct ifaddr *)data_ptr;
        	switch (ifa->ifa_addr.sa_family) {
		case AF_INET:
		     {
        		struct arpcom *ac = (struct arpcom *)ifnet_ptr;
        		struct in_ifaddr *ia = (struct in_ifaddr *)data_ptr;

        		sin = (struct sockaddr_in *)&ia->ia_addr;
			AC_LOCK(ac);
			ac->ac_ipaddr.s_addr = sin->sin_addr.s_addr;
			AC_UNLOCK(ac);
			break;
		     }

		}
/*
 * 	set IFF_UP flag if it has gone down and the LAN interface is OK
 */
		if ((ifnet_ptr->if_flags & IFF_UP) == 0) {
			if ((lanc_ift_ptr->hdw_state & LAN_DEAD) == 0) {
				lanc_mib_event (lanc_ift_ptr, NMV_LINKUP);
				ifnet_ptr->if_flags |= IFF_UP;
			}
		}
		break;
	}

   case SIOCSIFFLAGS:		
	{
		struct ifaddr *ifa = (struct ifaddr *)data_ptr;
		if ((ifa->ifa_addr.sa_family == AF_NIT) &&
		    (!bcmp(ifnet_ptr->if_name, "atr", 3)))
			/* Currently only ATR supports promiscuous mode. */
			status = (*lanc_ift_ptr->hw_req)(ifnet_ptr, SIOCSIFFLAGS, AF_NIT);
		else {
			if (lanc_ift_ptr->hdw_state & LAN_DEAD)
				status = ENXIO;
/*
 *	we don't send trailer packets although we receive them; the
 *	IFF_NOTRAILERS flag should be set all the time regardless what
 *	the user wants
 */
			ifr->ifr_flags |= IFF_NOTRAILERS;
		}
		break;
	}
/*
 * 	we're always asked, but in these cases the guys above us do
 * 	the work; OK by us.
 */
   case SIOCSIFMETRIC:		
   case SIOCSIFBRDADDR:
   case SIOCSIFNETMASK:
	break;
     
   case SIOCGACFLAGS:
	{
		AC_LOCK((struct arpcom *)ifnet_ptr);
		ifr->ifr_flags = ((struct arpcom *)ifnet_ptr)->ac_flags;
		AC_UNLOCK((struct arpcom *)ifnet_ptr);
		break;
	}

   case SIOCSACFLAGS:
	{
		struct arpcom *ac = (struct arpcom *) ifnet_ptr;
		u_short save_flags;

                if (ac->ac_type == ACT_8025) {
                        return(EINVAL);
		}
		AC_LOCK(ac);
                if (ac->ac_flags & ACF_SNAPFDDI) {
			AC_UNLOCK(ac);
                        return (EINVAL);
		}
		save_flags = ac->ac_flags;
		ac->ac_flags = ifr->ifr_flags & (ACF_ETHER | ACF_IEEE8023 | 
			    	   ACF_SNAP8023 | ACF_SNAP8024 | ACF_IEEE8024);

		IF_LOCK(ifnet_ptr);
		if (ac->ac_flags & ACF_SNAP8023) {
        	   ifnet_ptr->if_mtu = SNAP8023_MTU;
		} else {
		   if (ac->ac_flags & ACF_IEEE8023) {
		      ifnet_ptr->if_mtu = IEEE8023_MTU;
		   } else {
	    	      ifnet_ptr->if_mtu = ETHERMTU;
		   }
		}
		IF_UNLOCK(ifnet_ptr);
/*
 * 	If the flags have changed, send out an unsolicited probe name
 * 	reply.
 */
		if (save_flags != ac->ac_flags) {
			AC_UNLOCK(ac);
			(void) NETCALL(NET_PRBUNSOL)(ac);
		} else {
			AC_UNLOCK(ac);
		}

     		break;
	}

   case NETCTRL:
	switch (data_ptr->reqtype) {
	case RESET_INTERFACE:
	case RESET_STATISTICS:
	case ADD_MULTICAST:
	case DELETE_MULTICAST:
	case CHANGE_NODE_RAM_ADDR:
		if (!suser())
			return(EPERM);
		if (data_ptr->vtype != LAN_PHYSADDR_SIZE)
			return(EINVAL);
		break;

	case ENABLE_BROADCAST:
	case DISABLE_BROADCAST:
	case ENABLE_PROMISCUOUS:
	case DISABLE_PROMISCUOUS:
		if (!suser())
			return(EPERM);
		break;
	case SET_TRN_FUNC_ADDR_MASK:        /* fix for func address */
	case CLEAR_TRN_FUNC_ADDR_MASK:      /* 802.5 */
		if (!suser())
			return(EPERM);
		break;
	}

	status = lanc_media_control(lanc_ift_ptr, data_ptr, LAN_REQ_NOT_BLOCK);

	break;

   case NETSTAT:

#ifdef __hp9000s800
	s=spl5();
#endif
	if (ON_ICS)	/* not block the request */
		status = (*lanc_ift_ptr->hw_req)
		 	 (lanc_ift_ptr, LAN_REQ_GET_STAT, (caddr_t)data_ptr);
	else	/* block the request */
		status = (*lanc_ift_ptr->hw_req)
		 	 (lanc_ift_ptr, (LAN_REQ_GET_STAT | LAN_REQ_BLK_MSK), 
	    	 	 (caddr_t)data_ptr);
	if (status < 0)
		status = lanc_ctl_sleep(lanc_ift_ptr, status);

#ifdef __hp9000s800
	splx(s);
#endif

	break;
		 
   default:
	return(EINVAL);

   }
   return(status);
}


/*
 * 	This is the entry point for the protocol stack's if_control() function
 * 	in the ifnet structure.
 */

lanc_if_control(ifp, cmd, data_ptr, netisr_event, q_or_function)
struct ifnet *ifp;
int cmd;
caddr_t	data_ptr, netisr_event, q_or_function;

{
   int error = 0;

   switch(cmd) {
   case IFC_IFNMGET:
   case IFC_IFNMSET:
   case IFC_IFNMEVENT:
	error = lanc_mib_request(ifp, cmd, data_ptr);
	break;

   case IFC_IFBIND:
   case IFC_IFATTACH:
   case IFC_IFDETACH:
   case IFC_IFPATHREPORT:
   case IFC_OSIBIND:
   case IFC_OSIUNBIND:
   case IFC_PRBVNABIND:
   case IFC_PRBPROXYBIND:
        error = lanc_setup_protocol_logging(ifp, cmd, data_ptr,
					    netisr_event, q_or_function);
	break;

   default:
	error = 0;
	break;
   }
   return(error);
}


lanc_if_output(ac, m, sa) 	
struct arpcom *ac;
struct mbuf *m;
struct sockaddr *sa;

{
   int error = 0;
   lan_ift *lanc_ift_ptr = (lan_ift *)ac;
   struct ifnet *ifp = (struct ifnet *)ac;

   /* check if the protocol family is supported by the LAN interface */
   if (!(lanc_ift_ptr->af_supported & (1<<sa->sa_family))) {
	m_freem_train(m);
	return (EAFNOSUPPORT);
   } 

   switch (sa->sa_family) {
   case AF_UNSPEC: 
   	error = (*lanc_ift_ptr->hw_req)
		(lanc_ift_ptr, LAN_REQ_WRITE, (caddr_t)m);
	break;
#ifdef APPLETALK
   case AF_APPLETALK:
	/* Call atalk_output() indirectly, if it's in the kernel. */
	/* If it isn't, free the mbuf chain ourselves. */
	error = NETCALL(NETDDP_ATALK_OUTPUT)(ac, m, sa);
	if (error == ENOSYS) /* Appletalk isn't linked into the kernel */
		m_freem_train(m);  /* ... so we must release the mbuf chain. */
	break;
#endif /* APPLETALK */
   case AF_INET:
	/* Call arp to resolve the IP address to the station address */
	error = arp_resolve(ifp, m, sa);
	break;
   default: 
	m_freem_train(m);
	error = EAFNOSUPPORT;
	break;
   } /* switch on address family */
   return (error);
} /* end of lanc_if_output */

lanc_if_resolved_output(ifp, m, pkt_type, hdr_info)
struct ifnet *ifp;
struct mbuf *m;
int pkt_type;
caddr_t hdr_info;
{
    lan_ift *lanc_ift_ptr = (lan_ift *)ifp;
    int error;

    /*
     * If the m_act field is not set it means that we have to send out
     * just a packet and therefore we call hw_req directly.
     * If the m_act field is set it means we have to send out a 
     * chain of packets (fragments). Therefore we call a routine
     * lanc_if_fragmented_output to send out a chain of fragments.
     */ 

    if (m->m_act == 0)  { /* packet not part of fragment chain */
        error = (*lanc_ift_ptr->hw_req)
                        (lanc_ift_ptr, LAN_REQ_WRITE, (caddr_t)m);
    }
    else {
        error = lanc_if_fragmented_output(lanc_ift_ptr, m, pkt_type, hdr_info);
    } 
    return (error);
} /* lanc_if_resolved_output */

lanc_if_fragmented_output(lanc_ift_ptr, mtrain, pkt_type, hdr_info)
lan_ift *lanc_ift_ptr;
struct mbuf *mtrain;
int pkt_type;
caddr_t hdr_info;
{
    struct mbuf *mhdr;
    struct mbuf *m;
    int snd_ifq_length; 
    int error;

    /*************************************************************************
     * The purpose of this routine is to output a packet with resolved address
     * by making a call to hw_req to write the packet. This routine has been
     * written to handle a multi-fragment-chain connected by the m_act field.
     ************************************************************************/

     /* Disconnect first fragment (m) from remaining fragment train (mtrain) */
     m = mtrain;
     mtrain = m->m_act;
     m->m_act = 0; 

     /*
      * Send the first fragment since ac_build_hdr has already been called
      * for the first packet of the train.
      */
     error = (*lanc_ift_ptr->hw_req)(lanc_ift_ptr, LAN_REQ_WRITE, (caddr_t)m);
     if (error)
        goto freem;

     /* 
      * Since first fragment was sent without error, raise send queue limit so
      * remaining fragments can be sent successfully.
      */
     snd_ifq_length = lanc_ift_ptr->is_if.if_snd.ifq_maxlen;
     lanc_ift_ptr->is_if.if_snd.ifq_maxlen = 32767;

     /*
      * For each fragment call ac_build_hdr and then call hw_req.
      */
     while (mtrain) {

        /* Build link level header for fragment */
	mhdr = 0;
        error = (*lanc_ift_ptr->is_ac.ac_build_hdr) 
                    (lanc_ift_ptr, pkt_type, hdr_info, &mhdr);
	switch (error) {
	case NULL:
	    break;
	case ENOBUFS:
	    goto reset;
	default:
	    goto badhdr;
	} /* end switch */

        /* Attach link level header head fragment of fragment train */
        for (m = mhdr; m->m_next; m = m->m_next);
        m->m_next = mtrain;

        /* Disconnect head fragment (mhdr) from remaining fragments (mtrain) */
        m = mtrain;
        mtrain = m->m_act;
        m->m_act = 0;

        /* Send head fragment */
        error = (*lanc_ift_ptr->hw_req)
                    (lanc_ift_ptr, LAN_REQ_WRITE, (caddr_t)mhdr);
        if (error)
            goto reset;
     }

     /* Restore send queue limit and exit */
     lanc_ift_ptr->is_if.if_snd.ifq_maxlen = snd_ifq_length;
     return (0);

badhdr: error = lanc_if_fragment_error(mhdr, error); /* ac_build_hdr failed */
reset:  lanc_ift_ptr->is_if.if_snd.ifq_maxlen = snd_ifq_length;
freem:  error = lanc_if_fragment_error(mtrain, error);
        return (error);
} /* lanc_if_fragmented_output */

lanc_if_fragment_error(m,error)
struct mbuf *m;
int error;
{
     m_freem_train(m);
     return (error);
} /* lanc_if_fragment_error */



/***************        Beginning of lanc_nm.c          *******************/

/*
 *	- generate an event of the specified type
 *	- calculate ifLastChange and save in mib_struct
 */

lanc_mib_event (lanc_ift_ptr, event_id)
lan_ift *lanc_ift_ptr;
int event_id;

{
   struct evrec *this_event;
   struct timeval diff;
   struct timeval time;
   int error=0, ifindex;


   mib_ifEntry *mib_ptr = &(lanc_ift_ptr->mib_xstats_ptr->mib_stats);

   LAN_LOCK(lanc_ift_ptr->mib_lock);

   if (event_id == NMV_LINKUP && !(lanc_ift_ptr->is_if.if_flags & IFF_UP)) {
   	mib_ptr->ifOper = LINK_UP;
   } else {
	if(event_id == NMV_LINKDOWN && lanc_ift_ptr->is_if.if_flags & IFF_UP) {
		mib_ptr->ifOper = LINK_DOWN;
        } else {
		LAN_UNLOCK(lanc_ift_ptr->mib_lock);
		goto mib_done;
	}
   }

   ifindex = mib_ptr->ifIndex;

   LAN_UNLOCK(lanc_ift_ptr->mib_lock);

   time = ms_gettimeofday();
#ifdef __hp9000s800
   diff.tv_sec = time.tv_sec - sysInit.tv_sec;
   diff.tv_usec = time.tv_usec - sysInit.tv_usec;
#endif

   if (diff.tv_usec < 0) {
	diff.tv_sec --;
	diff.tv_usec += 1000000;
   }

   mib_ptr->ifLastChange = (TimeTicks) 100*diff.tv_sec + (diff.tv_usec)/10000;

   MALLOC (this_event, struct evrec *, sizeof (struct evrec), M_TEMP, M_NOWAIT);

   if (this_event) {
	this_event->ev.code = event_id;
	sprintf (this_event->ev.info, sizeof (u_long), "%d", mib_ptr->ifIndex);
	this_event->ev.len = sizeof (u_long);
	nmevenq (this_event);
   } else {
	error=ENOMEM;
   }

mib_done:

   return(error);
}


/*
 *	This routine handles the MIB get and set requests.
 */

lanc_mib_request(ifp, cmd, data_buf_ptr)
struct ifnet *ifp;
int cmd;
caddr_t data_buf_ptr;

{
   int error = 0;

   switch(cmd) {
   case IFC_IFNMGET:
        {
                char *kbuf = (char *)data_buf_ptr;
                lan_ift *lanc_ift_ptr = (lan_ift *)ifp;

                error = (*lanc_ift_ptr->hw_req)
                        (lanc_ift_ptr, LAN_REQ_GET_MIBSTAT, (caddr_t)kbuf);
                break;
        }

   case IFC_IFNMSET:
        {
                mib_ifEntry  *kbuf = (mib_ifEntry *)data_buf_ptr;
                lan_ift  *lanc_ift_ptr = (lan_ift *)ifp;
                mib_ifEntry *mib_ptr=&(lanc_ift_ptr->mib_xstats_ptr->mib_stats);
                int if_status = ((mib_ifEntry *)kbuf)->ifAdmin;


                switch (if_status) {
                case LINK_DOWN:
			LAN_LOCK(lanc_ift_ptr->mib_lock);
                        mib_ptr->ifAdmin =  if_status;
			LAN_UNLOCK(lanc_ift_ptr->mib_lock);
                        if (ifp->if_flags & IFF_UP) {
                                lanc_mib_event (lanc_ift_ptr, NMV_LINKDOWN);
                                ifp->if_flags &= ~IFF_UP;
                        }
                        break;

                case LINK_UP:
			if(lanc_ift_ptr->hdw_state & LAN_DEAD) {
				error=ENXIO;
				break;
			}
			LAN_LOCK(lanc_ift_ptr->mib_lock);
                        mib_ptr->ifAdmin =  if_status;
			LAN_UNLOCK(lanc_ift_ptr->mib_lock);
                        if (! (ifp->if_flags & IFF_UP)) {
                                lanc_mib_event (lanc_ift_ptr, NMV_LINKUP);
                                ifp->if_flags |= IFF_UP;
                        }
                        break;

                default:
                        error = EINVAL;
                }
                break;
        }

   case IFC_IFNMEVENT:
        {
                lan_ift  *lanc_ift_ptr = (lan_ift *)ifp;
                int event = (int)data_buf_ptr;
                error = lanc_mib_event(lanc_ift_ptr, event);
                break;
        }

   default:
        error = EINVAL;
   }
   return(error);
}
