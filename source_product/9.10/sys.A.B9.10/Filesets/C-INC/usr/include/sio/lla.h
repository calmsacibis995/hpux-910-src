/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sio/RCS/lla.h,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:43:18 $
 */

/*
 * (C) COPYRIGHT HEWLETT-PACKARD COMPANY 1990. ALL RIGHTS
 * RESERVED. NO PART OF THIS PROGRAM MAY BE PHOTOCOPIED,
 * REPRODUCED, OR TRANSLATED TO ANOTHER PROGRAM LANGUAGE WITHOUT
 * THE PRIOR WRITTEN CONSENT OF HEWLETT PACKARD COMPANY
 */

#ifndef _SIO_LLA_INCLUDED	/* allows multiple inclusion */
#define _SIO_LLA_INCLUDED

#ifdef _KERNEL
struct lla_cb
	{
	 struct rawcb *so_pcb;		/* protocol control block */
	 struct mbuf *pktheader;	/* ptr to header for last read pkt */
	 struct mbuf *head_packet;	/* first inbound queue packet*/
	 struct mbuf *last_packet;	/* last inbound packet in queue*/
	 struct arpcom *lla_ifp;	/* ptr to arpcom structure */
	 unsigned    lan_signal_mask;	/* LAN events which will signal user */
	 unsigned    lan_signal_pid;	/* pid to which SIGIO will be sent */
	 int	     lan_pkt_size;	/* maximum length of packet (includes */
					/* link protocol header)              */
	 int         lla_timeo;		/* timeout on reads */
	 struct proc *lla_rsel;         /* process pointer for select reads */
	 struct lla_hdr
	   {
	     union
		{
		 struct
		    {
	             u_char destaddr[6];
	             u_char sourceaddr[6];
		     u_short length;
	  	     u_char  dsap;
	             u_char  ssap;
	             u_char  ctrl;
	             u_char  pad[3];
	             u_short dxsap;
	             u_short sxsap;
		    } ieee;
		 struct
		    {
	             u_char destaddr[6];
	             u_char sourceaddr[6];
		     u_short log_type;
		     u_short dxsap;
		     u_short sxsap;
		    } ether;
		 struct
		    {
                     u_char access_ctl;
                     u_char frame_ctl;
                     u_char destaddr[6];
                     u_char sourceaddr[6];
                     u_char rif_plus[18+8];
                     u_char  dsap;
                     u_char  ssap;
                     u_char  ctrl;
                     u_char  orgid[3];
                     u_short etype;
		    } snap8025;
		 struct
		    {
                     u_char access_ctl;
                     u_char frame_ctl;
                     u_char destaddr[6];
                     u_char sourceaddr[6];
                     u_char rif_plus[18+8];
                     u_char  dsap;
                     u_char  ssap;
                     u_char  ctrl;
		    } ieee8025;
		} proto;
           } packet_header;
 	 short       lla_msgsqd;	/*msgs qued on lla file (inbound)*/
	 short	     lla_maxmsgs;	/*max msgs for inbound queue */
	 u_short     lla_flags;		/*flags for control*/
	 short       hdr_size;		/*header size, based on protocol*/
	 int         func_addr;		/*functional address value 802.5 */
	 lock_t      *lla_lock;		/* protects lla_cb */
        };
/* Defined LLA control flags */
#define LLA_IS_ETHER	0x1     /* lla opened as ethernet = True */
#define LLA_WAIT	0x2     /* user is blocked on read = True */
#define LLA_FNDELAY     0x4     /* user opened w no delay = True */
#define LLA_TIMEOUT     0x8     /* user has a read timeout = True */
#define LLA_PROT_LOGGED 0x10    /* user has logged a sap/type = True */
#define LLA_DEST_LOGGED 0x20    /* user has logged a dest. addr. = True */
#define LLA_SELECT_RCOLL 0x40   /* collision on read select = true */
#define LLA_RAW_802MAC  0x80    /* Raw LLA connection = True       */
#define LLA_FWRITE      0x100   /* user opened w write permission */
#define LLA_FREAD       0x200   /* user opened w read permission */
#define LLA_RIF_LOGGED  0x400   /* user has logged a RIF addr = True .5 only*/
#define LLA_IS_8025     0x800   /* lla opened as 8025 = True */
#define LLA_IS_SNAP8025 0x1000  /* lla opened as 8025 SNAP = True */
#define LLA_IS_FA8025   0x4000  /* lla opened as 8025 func address */

#define LAN_8025_MIN_RIF_SIZE  2   /* for test/xid to establish rif*/
#define LAN_8025_MAX_RIF_SIZE  18  /* max for 802.5 */

#define LAN_PROTOCOL_ETHER	1	/* LLA use only */
#define LAN_PROTOCOL_IEEE	0	/* LLA use only */
#define MIN_CACHE               1       /* minimum cache for LAN(4) input q   */
#define MAX_CACHE               16      /* maximum cache for LAN(4) input q   */
#define SU_MAX_CACHE            64      /* max. cache for super user input q  */

#define LLA_MBUFS_PKT   (NMBPCL+3)   /* # mbufs required for largest packet*/

#define LAN_LLA_LOCK_ORDER 13		/* order of lla_lock */

#ifdef NASA_PROMISCUOUS
typedef struct
{
 caddr_t  lla_ptr;
 int      filter_cnt;
} nasa_prom_type;
#endif /* NASA_PROMISCUOUS */
#endif /* _KERNEL */
#endif /* _SIO_LLA_INCLUDED */
