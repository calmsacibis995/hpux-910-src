/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/lanc.h,v $
 * $Revision: 1.13.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:42:10 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */
 

#ifndef _SIO_LANC_INCLUDED
#define _SIO_LANC_INCLUDED

#ifdef _KERNEL 
#include "../h/mib.h"
#include "../h/spinlock.h"
#else
#include <sys/mib.h>
#include <sys/spinlock.h>
#endif /* _KERNEL */


/***************************************************************************
 * This file specifies lan common structures used for the lan_ift for      *
 * Series 800 (NIO & SIO) and the Series 300 (DIO) LAN interfaces.  The    *
 * NIO & CIO interfaces share much greater commanality than the 300, due   *
 * to the use of 'intelligent' interface logic.                            *
 *                                                                         *
 * For purposes of clarity and documentation, typedef declarations and     *
 * macros which are unique to the 800 are presented separately.  The       *
 * 300 specific stuff is also segregated.  Where ever possible, structures *
 * have been combined so as to make the interface to lan drivers as        *
 * transparent as possible.                                                *
 ***************************************************************************/

 
/****************************************************************************
 * LANIFT and related structures.  This is the main interface table.        *
 * The following section of the head file contains first supporting type    *
 * definitions for the LANIFT, next defines used for fields of the LANIFT,  *
 * and lastly, the LANIFT type definitions itself.                          * 
 ****************************************************************************/

/* lan_timer structure is used for DMA and CTRL timeouts. One exists
 * in each lan_ift structure.  The timer is used for S800 only.
 */

typedef struct {
	int flag;
	int timer;
} lan_timer; 

/* Defines for accessing parts of the arpcommon structure                     */

#define	is_if	is_ac.ac_if		/* network-visible interface          */
#define	is_addr	is_ac.ac_enaddr		/* hardware network address           */

 
/* Defines for LANIFT fields                                                  */
/****************************************************************
*                                                               *
* Defines for the flags field.  Controls driver 'state machine' *
*                                                               *
*****************************************************************/

#define	LAN_DEAD        0x20	        /* dead interface card                */


/****************************************************************************
*  Defines for hardware request codes to the hw_req function.               *
****************************************************************************/
#define LAN_REQ_BLK_MSK	       0x80000000  /* Blocking ctl req bit         */
#define LAN_REQ_DIAG_EXCL      0x40000000  /* Card diag exclusive access   */
#define LAN_REQ_MSK            0x07ffffff  /* ctl req code                 */

#define LAN_REQ_NONE           0 /*  No ctl request at the driver level    */ 
#define LAN_REQ_RESET          1 /*  ioctl reset and initilize DA          */
#define LAN_REQ_INIT           2 /*  ICS executable, initialize DA         */ 
#define LAN_REQ_MC             3 /*  ioctl set multicast list              */
#define LAN_REQ_RF             4 /*  ioctl set receive filter              */ 
#define LAN_REQ_SET_STAT       5 /*  ioclt set card statistics             */
#define LAN_REQ_GET_STAT       6 /*  ioctl obtain statistics               */ 
#define LAN_REQ_DOG_STAT       7 /*  watch dog obtain statistics           */
#define LAN_REQ_RAM_NA_CHG     8 /*  change the station RAM address        */
#define LAN_REQ_WRITE	       9 
#define LAN_REQ_WRITE_L	       10
#define LAN_REQ_GET_MIBSTAT    11 /* obtain mib statistics                  */
#define LAN_REQ_ACTIVE         12 /* used internally to lan1 code to denote */
                                  /* that a ctl request is active           */
#define	LAN_REQ_PROTOCOL_LOG   13 /* log protocol value			    */
#define	LAN_REQ_PROTOCOL_REMOVE 14/* remove protocol value		    */
#define LAN_REQ_TRN_FUNC_MASK   15/* Change functional address mask 802.5   */

#define LAN_REQ_DUMP           201 /* port dump card/hw state               */
#define LAN_REQ_ORD_DUMP       202 /* dump state without chip reset         */

#define LAN_STAT_ALL           0  /*  used to signify that all statistics   */
                                  /*  are to be reset.                      */

#define	LAN_REQ_BLOCK	       1  /* set to block the hardware request      */
#define	LAN_REQ_NOT_BLOCK      0  /* set to not block the hardware request  */


/* Define for the watchdog interval */

#define	LANWATCHINTERVAL  1 	  /* once every 1  seconds                  */

 
/**********************************************************************/
/*                                                                    */
/*               MIB  Related  Definitions                            */
/*                                                                    */
/**********************************************************************/
/* The extra fields hold the stats which would have been */
/* reset during a card reset */

/* s => saved val of the component before it is reset */
/* C => Card component */
/* D => Driver component */

typedef struct {

	mib_ifEntry		mib_stats;

        counter                 if_DInDiscards;
        counter                 if_CInDiscards;
        counter			if_sCInDiscards;

        counter                 if_DOutDiscards;
        counter                 if_COutDiscards;
        counter                 if_sCOutDiscards;

        counter                 if_DInErrors;
        counter                 if_CInErrors;
        counter                 if_sCInErrors;

        counter                 if_DOutErrors;
        counter                 if_COutErrors;
        counter                 if_sCOutErrors;

} mib_xEntry;
 
/******************************************************************************
 * LANIFT definition                                                          * 
 ******************************************************************************/

typedef struct	{

        /* Network common part                                                */
	struct	arpcom is_ac;		/* contains:
                                           ac_if,        an ifnet struct        
					   ac_enaddr[6], the card network addr
					   ac_ac,     link to next arpcom     */
 
        /* LAN driver common state and hardware request information           */

	int		af_supported;   /* protocol (addr) families supported */

	/* the following 3 fields are used for lan0 and lan1 only             */
	int		mjr_num;	   /* major number for lan device     */
	int		mgr_index;	   /* manager index for lan device    */
	int		hdw_path[5];	   /* hardware address path	      */
	/* the following field is used for DIO only                           */
	int		select_code;       /* for DIO only  */
	int             lu;                /* for DIO only */

	int	        hdw_state;         /* driver/card states              */
        lan_timer       lantimer;          /* DMA/Cntrl timer lan0 and lan1   */

        /* S800 hardware specific procedure handles used by lan common code   */

        int       (*hw_req)();          /* hw interface request function      */
        int       (*dma_time)();        /* dma timeout error handling function*/
        	
        /* Status and statistics data area                                    */

	unsigned int  BAD_CONTROL;      /* number of in pkts w/ bad contl     */
	unsigned int  UNKNOWN_PROTO;    /* number of in pkts w/ bad proto     */
	unsigned int  RXD_XID;          /* number of received XID pkts        */
	unsigned int  RXD_TEST;         /* number of received TEST pkts       */
        unsigned int  RXD_SPECIAL_DROPPED;/* number of XID/TEST resp dropped  */
        
        short int   is_scaninterval;	/* interval of stat collection        */

        /* Configuration info                                                 */
	 
        u_char           station_addr[6];/* configured RAM station address    */
        int              num_multicast_addr; /* number of multicast addresses */
	int              broadcast_filter;   /* Read Packet Filter            */
	int              multicast_filter;   /* Read Packet Filter            */
	int              promiscuous_filter; /* Read Packet Filter            */
	unsigned char	 mcast[96];    /* Multicast Address List (16 entries) */                                       /* TRN hides func mask in mcast array  */
                                       /* at TRN_FUNC_MASK_LOC                */

	mib_xEntry	*mib_xstats_ptr;  /* MIB object ptr */

	lock_t		*lanc_lock;	/* protects lan_ift */
	lock_t		*mib_lock;	/* protects lan_ift.mib_xstats_ptr */
/************** temporary stuff until transport gets their changes in ********/
	lock_t		*if_lock;	/* tmp lock for ifnet */
	lock_t		*ac_lock;	/* tmp lock for arpcom */
/*****************************************************************************/

} lan_ift;

/******************************************************************************
 * Spinlock defines
 ******************************************************************************/
#define LAN_LANX_LOCK_ORDER	11	/* order of lanX_lock (X=0,1,2,...)   */
#define LAN_LANC_LOCK_ORDER	12	/* order of lanc_lock                 */
        /* order 13 is for lla_lock in the lla_cb in lla.h */
        /* order 14 is for log_lock in lanc.c */
#define LAN_MIB_LOCK_ORDER      15      /* order of mib_lock */

/* see comment below */
#ifdef LAN_REAL_LOCKS

#define LAN_ALLOC_LOCK(lock) \
        MALLOC(lock,lock_t *,((256 > sizeof(lock_t)) ? 256 : sizeof(lock_t)), \
                        M_DYNAMIC,M_NOWAIT)

#define LAN_INIT_LOCK(lock,order,name) initlock(lock,order,name)

#define LAN_FREE_LOCK(lock) FREE(lock,M_DYNAMIC)

#define LAN_LOCK(lock) spinlock(lock)

#define LAN_UNLOCK(lock) spinunlock(lock)

#define LAN_OWNS_LOCK(lock) owns_spinlock(lock)

#else /* ! LAN_REAL_LOCKS */

/*************
	This is a kludge.  It turns out that it is illegal to call spl() 
	while holding a spinlock; doing it can cause deadlocks on MP systems.
	This is a problem for us in 9.0 because some of the mbuf routines, 
	the tracing/logging calls, and the shared lanmux code, use spl() for 
	protection.  It would require a major re-structuring of lan3 to make 
	everything work correctly.  For 9.0 we are going to be a UP driver.  
	So as not to have to re-insert the locking calls at a later date, I 
	have simply re-defined the macros to use spl()'s instead of 
	spinlocks.  The return from spl5() is stored in the lock pointer 
	and is read from the lock pointer to do the splx().  A value 
	of LAN_LOCK_AVAIL in the lock pointer means that it is initialized 
	but not currently in use.  If LAN_TEST_FAKE_LOCKS is defined, error
	checking code is added to the macros to detect acquiring of a lock
	that is already owned and releasing of a lock that is not owned.
	There are areas in the code around sleep() calls that are also
	ifdef'ed on LAN_REAL_LOCKS.
*************/

#define LAN_LOCK_AVAIL (lock_t *)(0xbabababa)

#define LAN_ALLOC_LOCK(lock) (lock = LAN_LOCK_AVAIL)
#define LAN_INIT_LOCK(lock,order,name) /* do nothing here */
#define LAN_FREE_LOCK(lock) (lock = (lock_t *)0)
#define LAN_OWNS_LOCK(lock) ((lock == LAN_LOCK_AVAIL) ? 0 : 1)

#ifdef LAN_TEST_FAKE_LOCKS
#define LAN_LOCK(lock) { \
	if(lock != LAN_LOCK_AVAIL) panic("LAN_LOCK:  already own lock"); \
	lock = (lock_t *)spl5(); \
}
#define LAN_UNLOCK(lock) { \
	int spl=(int)(lock); \
	if(lock == LAN_LOCK_AVAIL) panic("LAN_UNLOCK:  don't own lock"); \
	lock = LAN_LOCK_AVAIL; \
	splx(spl); \
}
#else
#define LAN_LOCK(lock) lock = (lock_t *)spl5();
#define LAN_UNLOCK(lock) {int spl=(int)(lock);lock=LAN_LOCK_AVAIL;splx(spl);}
#endif


#endif /* LAN_REAL_LOCKS */


/************** simulate the future IP locks ********/
#define AC_LOCK_ORDER 18
#define IF_LOCK_ORDER 19
#define AC_LOCK(arpcom) LAN_LOCK(((lan_ift *)arpcom)->ac_lock)
#define AC_UNLOCK(arpcom) LAN_UNLOCK(((lan_ift *)arpcom)->ac_lock)
#define IF_LOCK(ifnet) LAN_LOCK(((lan_ift *)ifnet)->if_lock)
#define IF_UNLOCK(ifnet) LAN_UNLOCK(((lan_ift *)ifnet)->if_lock)
#define IFQ_LOCK(ifq)
#define IFQ_UNLOCK(ifq)
#define IF_ENQUEUEIF_NOLOCK(ifq,m,ifnet) IF_ENQUEUEIF(ifq,m,ifnet)

/******************************************************************************
 * Miscellaneous constant definitions                                         
 ******************************************************************************/

#define	LAN_PHYSADDR_SIZE	6	/* 6 bytes for network address size   */
#define FUNC_ADDR_SIZE          4       /* 4 bytes for functional addr mask   */

#define LAN0_MJR_NUM		50
#define LAN1_MJR_NUM		51
#define TOKEN1_MJR_NUM          102     /* Madge-EISA, temp value */

#define	MINSAP			0	/* possible SAPs are 0-255            */
#define MAXSAP                  255
#define MAXTYPE                 0xFFFF
#define	MINTYPE			0 
#define HP_EXPANDTYPE		0x8005
#define	HP_RESERVETYPE		0x805
#define	MAX_MULTICAST_ADDRESSES	16

#define MAX_8025_MULTICAST_ADDRESSES    1
#define TRN_FUNC_MASK_LOC  24   /* loc in mcast where TRN stores func mask */

#define	MTU_8023		1514	/* maximum length of packet (includes */
					/* header			      */
#define XID_INFO_FIELD_SIZE	3	/* 3 bytes for XID info field size    */


/******************************************************************************
 * Miscellaneous Data Structures                                              *
 ******************************************************************************/

/*  Structure of an lan RD Header (ether header) */

typedef struct {
	unsigned char    destination[6];/* destination node address          */
	unsigned char    source[6];     /* source node address               */
        unsigned short   type;          /* message type                      */
} lan_rheader;

/*
 * structure and values for XID response information field
 * (Type I, Class I)
 */

struct ns_xid_info_field {
	u_char	xid_info_format;	/* format; IEEE 802.3 format */
	u_char	xid_info_type;		/* type and class (Type I, Class I) */
	u_char	xid_info_window;	/* window size; will be zero 1st rel. */
};

#define XID_IEEE_FORMAT		0x81		/* 10000001 */
#define	XID_TYPE1_CLASS1	1		/* 00000001 */
#define XID_WINDOW_SIZE		0		/* 00000000 */

/*
 * This mask is used to determine if the protocol value being logged
 * is from a STREAMS request.  The protocol kind will have the upper
 * most bit set if it was from a STREAMS request.
 */
#define	STREAMS_LOG_MASK 0x80000000

/*
 * The following enumeration defines the kinds of packet formats
 * the LAN driver recognises. IEEE 802.[23] SAPs, Ethernet Types, and
 * HP Canonical addressing.
 */

enum packet_kinds {LAN_SAP, LAN_TYPE, LAN_CANON, LAN_SNAP, LAN_SNAP_EXT};
 
/*
 * Structures to store information on the protocols that have been
 * logged with the LAN driver and the associated input handling
 * routine for each protocol. These are maintained as linked lists.
 */


struct logged_info{
   int protocol_val[5];/* SAP, Type or Canonical value indexed by logged_kind */
   void (*rint)();     /* pointer to protocol input routine                   */
   struct ifnet *ifnet_ptr;   /* pointer to ifnet of interface to be logged   */
                     /* negative value implies protocol logged globally       */
   int lu_protocol_info;  /* scratch field to store log-specific info         */
                       /* currently only used by LLA functions                */
   int flags;          /* LANC_ON_ICS and LANC_STRIP_HEADER bits              */
};

struct logged_link{
   struct logged_link *next;
   struct logged_info log;
};

/* The structures above are stored as linked lists. For faster search
 * times, there is more than one linked list. The head of each linked list
 * is an element of an array. The actual element number is obtained by 
 * masking out all but the lower order HASHBITS number of bits from 
 * the SAP, Type or Canonical Address value (protocol_val) contained in 
 * the incoming packet.
 */
#define HASHBITS 		8

#define PF_BITMASK        	0x10 /* Poll/Final bit for CTRL field in IEEE
                                      * packets. */
#define LANC_ON_ICS       	0x01 /* This bit set if protocol runs on ICS */
#define LANC_STRIP_HEADER 	0x02 /* This bit set if Level 2 header to be
                                      * stripped.  */
#define LANC_CHECK_FLAGS 	0x04 /* flag set tells driver to check for
                                        interface flags being set */
#define LANC_RAW_802MAC         0x08 /* Raw 802.3 port */
#define LANC_TOKEN_FLAG        0x100 /* This bit is a placeholder 802.5 fa */
#define LANC_GLOB_IF		-1   /* If the protocol to be logged for all
                                        Logical Units */

#define	NSAPS		256	/* number of IEEE dsaps potentially supported */
				/* The range is 0 - 255. 
				 * Anything with the lsb set is a group sap
				 * and cannot be logged in (we don't currently
				 * support them). Only even-numbered are kosher.
				 */

#define	IEEE8023_0	0	/* MAC type for 802.3 with no pad */
#endif  /* _SIO_LANC_INCLUDED  */
