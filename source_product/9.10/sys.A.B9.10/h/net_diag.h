
/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/h/RCS/net_diag.h,v $
 * $Revision: 1.6.83.4 $            $Author: kcs $
 * $State: Exp $                $Locker:  $
 * $Date: 93/09/17 18:30:51 $
 *
 *
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
   

/* Kernel modules pick up the kernel ../h file, and user get it from the BE */ 
#ifndef NS_LS_X25
#ifdef _KERNEL_BUILD
#include "../h/subsys_id.h"
#else  /* ! _KERNEL_BUILD */
#include <subsys_id.h>
#endif /* _KERNEL_BUILD */
#endif


/* define a min and max subsystem so I may define a data structure for */ 
/* filtering  easier */

extern int net_trace_on;
extern int net_log_on;
extern int netdiag_tr_map[];
extern int netdiag_log_map[];
 

/* initialize the log_instance value */
extern unsigned short ktl_log_inst;

/*                Filters Specific to Tracing                    */
typedef struct  {
        int     kind;
        int     uid;
	int     device_id;
} netdiag_t_filters;

/*                Filters Specific to Logging                    */
typedef struct  {
        int     class; 
	int     seq_num;
	int     device_id;
} netdiag_l_filters;

/*            Trace and Log filters for each subsystem           */
typedef struct {
        netdiag_t_filters    trace;
        netdiag_l_filters    log;
} netdiag_subsys_t;


/* this is ugly, but it beats having to rewrite the code because of a data */
/* structure change */
/*  Each Kernel Subsystem data structure                */

extern netdiag_subsys_t   netdiag_masks[];

/* some defines to make referencing ques easier */
#define nettrace_que netdiag_ques[NET_TRACE]
#define netlog_que   netdiag_ques[NET_LOG]


/* checks the range of the subsystem value */
#define KTL_RANGE(ss) (((ss >= 0 ) && (ss < (MAX_SUBSYS))) ? TRUE : FALSE ) 

/* Tracing macros */
#define KTRC_SUBSYS_ON(subsys_id)   \
	(net_trace_on && netdiag_tr_map[subsys_id]) 

/* if the ss is in range, then check the kind value, else return false  */
#define KTRC_ON(ss, kd) \
	((KTL_RANGE(ss)) && ((kd) & netdiag_masks[ss].trace.kind ? TRUE:FALSE))

#define KTRC_CK(ss,kd) \
	(KTRC_SUBSYS_ON(ss) ? KTRC_ON(ss,kd) : FALSE) 	 


/* logging macros  */
#define KLOGG_SUBSYS_ON(subsys_id) \
	(net_log_on && netdiag_log_map[subsys_id]) 

/* check that subsys is in range,and make sure classes match, */
#define KLOGG_ON(ss, cl)  \
	((KTL_RANGE(ss)) && ((cl) & netdiag_masks[ss].log.class ? TRUE:FALSE)) 

/* use one macro call to call KLOGG_SUBSYS_ON and KLOG_ON together */
#define KLOG_CK(ss,cl) \
	(KLOGG_SUBSYS_ON(ss) ? KLOGG_ON(ss,cl) : FALSE )



/* This next set of information is the old ns_diag.h file */
#ifndef _SYS_NS_DIAG_INCLUDED
#define _SYS_NS_DIAG_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/time.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/time.h>
#endif /* _KERNEL_BUILD */

/********************************************************************
 ****
 **** Macros
 *****   NS_LOG      (event, class, subsys, source)
 *****   NS_LOG_STR  (event, class, subsys, source, str)
 *****   NS_LOG_INFO (event, class, subsys, source, nparms, p0, p1)
 *****   NS_LOG_INFO5(event, class, subsys, source, nparms, p0, p1, p2, p3, p4)
 *****   NS_LOG_DATA (event, class, subsys, source, data, data_len, p0, p1)
 *****   NS_LOG_DATA (event, class, subsys, source, data, data_len, p0, p1)
 *****
 *****   X25_LOG      (event, class, subsys, source)
 *****   X25_LOG_STR  (event, class, subsys, source, str)
 *****   X25_LOG_INFO (event, class, subsys, source, nparms, p0, p1)
 *****   X25_LOG_INFO5(event, class, subsys, source, nparms, p0, p1, p2, p3, p4)
 *****   X25_LOG_DATA (event, class, subsys, source, data, data_len, p0, p1)
 ****
 ********************************************************************
 * Input Parameters
 *	event			event being logged (LE_xxx)
 *	class			event class (NS_LC_xxx)
 *	subsys			subsystem executing the log (NS_LS_xxx)
 *	source			source of event within subsystem.  Always 0
 *                              for release 2.0.
 *	str [_STR]		A NULL terminated C string which is to be
 *				printf'd into the log message.  The maximum
 *				length of this string is MLEN bytes.in the
 *
 *	data [_DATA]		Data to be dumped with the log msg.
 *	datalen [_DATA]		Number of bytes in data (max is MLEN)
 *
 *	nparms [_INFO]	        Number of words of associated info (max 5)
 *	p0-p4 [_INFO]		Info words which are to be printf'd into the
 *				log message.
 *	
 *
 * Output Parameters
 *	none
 * Return Value
 *	none
 * Globals Referenced
 *	none
 * Description
 *							   
 *	These macros check the log masks against the class of the input 
 *	event, and if either mask is set logs the event (to a file and/or to
 *	the console).
 *
 ********************************************************************
 */



#define EVENT_TURNED_ON(e,c,s) ((net_log_on && netdiag_log_map[s]))


#ifdef _KERNEL
#define _NS_LOG_(event, class, subsys, type, d, n, p0, p1, p2, p3, p4)       \
	{								     \
		if (EVENT_TURNED_ON(event, class, subsys))		     \
		        ns_log_event(event, class, subsys, type, d, n,       \
				p0, p1, p2, p3, p4);                         \
	}
#else  /* _KERNEL */
#define _NS_LOG_(event, class, subsys, type, d, n, p0, p1, p2, p3, p4)       \
	{								     \
		ns_log_event(event, class, subsys, type, d, n,               \
			p0, p1, p2, p3, p4);                                 \
	}
#endif /* _KERNEL */


#define NS_LOG(event, class, subsys, source)	     	                     \
	_NS_LOG_(event, class, subsys, NS_LF_NODATA, 0, 0, 0, 0, 0, 0, 0)

#define NS_LOG_STR(event, class, subsys, source, str)                        \
	_NS_LOG_(event, class, subsys, NS_LF_STR, str, 0, 0, 0, 0, 0, 0)

#define NS_LOG_DATA(event, class, subsys, source, data, dlen, p0, p1)        \
	_NS_LOG_(event, class, subsys, NS_LF_DATA, data, dlen, p0, p1, 0, 0, 0)

#define NS_LOG_INFO(event, class, subsys, source, n, p0, p1)                 \
	_NS_LOG_(event, class, subsys, NS_LF_INFO, 0, n, p0, p1, 0, 0, 0)

#define NS_LOG_INFO5(event, class, subsys, source, n, p0, p1, p2, p3, p4)    \
	_NS_LOG_(event, class, subsys, NS_LF_INFO, 0, n, p0, p1, p2, p3, p4)

#define X25_LOG(event, class, subsys, source)	     	                     \
	_NS_LOG_(event, class, subsys, NS_LF_NODATA, 0, 0, 0, 0, 0, 0, 0)

#define X25_LOG_STR(event, class, subsys, source, str)                        \
	_NS_LOG_(event, class, subsys, NS_LF_STR, str, 0, 0, 0, 0, 0, 0)

#define X25_LOG_DATA(event, class, subsys, source, data, dlen, p0, p1)        \
	_NS_LOG_(event, class, subsys, NS_LF_DATA, data, dlen, p0, p1, 0, 0, 0)

#define X25_LOG_INFO(event, class, subsys, source, n, p0, p1)                 \
	_NS_LOG_(event, class, subsys, NS_LF_INFO, 0, n, p0, p1, 0, 0, 0)

#define X25_LOG_INFO5(event, class, subsys, source, n, p0, p1, p2, p3, p4)    \
	_NS_LOG_(event, class, subsys, NS_LF_INFO, 0, n, p0, p1, p2, p3, p4)


/* The following macros are for release 1.x compatibility only.  
 * They WILL BE REMOVED soon.  Note that the last 5 parameters of
 * NS_LOG_EVENT_INFO are ignored.
 */

#define NS_LOG_EVENT(evt, cla, sub, err, ctx, loc)	     	             \
	_NS_LOG_ (evt, cla, sub, NS_LF_NODATA, 0, 0, 0, 0, 0, 0, 0)



/*
 * log class definitions
 */
#define NS_LC_LOGSTAT		0 
#define NS_LC_DISASTER	        1
#define NS_LC_ERROR	        2  	
#define NS_LC_WARNING		3
#define NS_LC_RESOURCELIM	4
#define NS_LC_PROLOG		5

#define NS_LC_COUNT		6


/*
 * Offset conventions for events within a subsystem
 *	(events within each class begin at the following offsets)
 */
#define LC_OFFSET		1000
#define LC_2_OFFSET(class)      ((class) * LC_OFFSET)

#define LC_LOGSTAT_OFFSET	LC_2_OFFSET(NS_LC_LOGSTAT)
#define LC_DISASTER_OFFSET	LC_2_OFFSET(NS_LC_DISASTER)
#define LC_ERROR_OFFSET		LC_2_OFFSET(NS_LC_ERROR)
#define LC_WARNING_OFFSET	LC_2_OFFSET(NS_LC_WARNING)
#define LC_RESOURCELIM_OFFSET	LC_2_OFFSET(NS_LC_RESOURCELIM)
#define LC_PROLOG_OFFSET	LC_2_OFFSET(NS_LC_PROLOG)



/*
 * log subsystem definitions should be defined in ../subsys_id.h  only!
 */
/*
#define NS_LS_LOGGING		0
#define NS_LS_NFT		1
#define NS_LS_LOOPBACK		2
#define NS_LS_NI		3
#define NS_LS_IPC		4
#define NS_LS_SOCKREGD		5
#define NS_LS_TCP		6
#define NS_LS_PXP		7
#define NS_LS_UDP		8
#define NS_LS_IP		9
#define NS_LS_PROBE		10
#define NS_LS_DRIVER    	11
#define NS_LS_RLBD		12
#define	NS_LS_BUFS		13
#define	NS_LS_CASE21		14
#define	NS_LS_ROUTER21		15
#define	NS_LS_NFS		16
#define	NS_LS_NETISR		17
#define NS_LS_X25               18
#define NS_LS_NSE               19
#define NS_LS_STRLOG		20
#define NS_LS_TIRDWR		21
#define NS_LS_TIMOD		22

#define NS_LS_COUNT		23
*/

#define NS_MAX_LOGMSGS	        200




/*
 * log actions: used to allow system action based on log 
 * classes/subsys's/events.  Only current action is to panic.
 */
#define NS_ACT_NOACTION		0		/* actions are off */
#define NS_ACT_PROCCALL		1		/* call a procedure */
#define NS_ACT_PANIC		2		/* call panic */

/*
 * These are the events themselves
 */



	/********************************
	 *	Subsystem: Logging	*
	 ********************************/

/* Logging Statistics */
#define LE_LOG_STARTING			0	/* logging has been started */
#define LE_LOG_TERM			1	/* logging has been terminated */
#define LE_LOG_TEST			2	/* logging test */
#define LE_LOG_TEST_STR			3	/* logging test w/string */
#define LE_LOG_TEST_INFO		4	/* logging test w/info */
#define LE_LOG_DROPPED			5	/* messages were dropped */
#define LE_LOG_TEST_DATA		6	/* logging test w/data */

/* Disasters */
#define LE_LOG_DISASTER_TEST		1000	/* logging disaster test */
/* Errors */
#define LE_LOG_ERROR_TEST		2000	/* logging error test */
/* Warnings */
#define LE_LOG_WARNING_TEST		3000	/* logging warning test */
#define LE_LOG_DROPPED_MSG		3001	/* dropped msg warning */
/* Resource Limitations */
#define LE_LOG_RESOURCELIM_TEST		4000	/* logging r. limit test */
/* Protocol Logs */
#define LE_LOG_PROLOG_TEST		5000	/* logging proto. log test */



	/***********************************
	 *	Subsystem: RFAR		   *
	 ***********************************/

/* Errors */

#define LE_RFAR_COPY_FAIL		2000	/* user-kernel copy fail */
#define LE_RFAR_IPC_CREATE		2001	/* ipc create socket fail */
#define LE_RFAR_IPC_SHUTDOWN		2002	/* ipc shutdown fail */
#define LE_RFAR_IPC_MUX_CONN		2003	/* ipc mux connect fail */
#define LE_RFAR_IPC_CONTROL		2004	/* ipc control fail */
#define LE_RFAR_IPC_DEST		2005	/* ipc dest socket fail */
#define LE_RFAR_IPC_SEND		2006	/* ipc send fail */
#define LE_RFAR_IPC_MUX_RECV		2007	/* ipc receive fail */
#define LE_RFAR_CONN_FAIL		2008	/* ipc connect fail */


/* Warnings */

#define	LE_RFAR_INODE			3000	/* no memory for inode */
#define	LE_RFAR_PROCMEM			3001	/* no memory for process */ 

/* Resource Limitations */

#define	LE_RFAR_NOMEM_RFP		4000	/* no memory for inode */
#define	LE_RFAR_NOMEM_NETP		4001	/* no memory for netunam */
#define	LE_RFAR_NOMEM_PMP		4002	/* no memory for process */
#define	LE_RFAR_NETUNAMS_PER_PROC	4003	/* no more netunams */


/* Protocol Logs */



	/******************************************
	 *	Subsystem: IPC	(NS & 4.2BSD)     *
	 ******************************************/

/* Warnings */
#define	LE_IPC_SEND_NOMEM_VC		3000	/* no memory for send on vc */

/* Resource Limitations */
#define LE_NO_NAME_MEM			4000	/* No mem for a name record*/
#define LE_CALL_SOCK_Q_LIM		4001	/* call socket queue limit */

/* Protocol Logs */
#define LE_SOCK_ABORT			5000	/* sock aborted by protocol.*/



	/********************************
	 *	Subsystem: TCP		*
	 ********************************/
/* Warnings */
#define	LE_TCP_TERM			3000	/* TCP terminated */
#define	LE_TCP_INPUT_PKT_DROPPED	3001
#define	LE_TCP_CKSUM_ERR		3002
#define	LE_TCP_TOS_ERR			3003
#define	LE_TCP_SEC_ERR			3004
#define	LE_TCP_SHORT			3005
#define	LE_TCP_MM_MSG_2MANY		3006
#define	LE_TCP_MM_MSG_2BIG		3007
#define LE_TCP_MORE_DATA_DEFER		3008
#define LE_TCP_PKT_DROP_STATE		3009
#define LE_TCP_PKT_DROP_SOCK		3010
#define LE_TCP_CONN_DROP_RXMT		3011
#define LE_TCP_CONN_DROP_KEEP_ALIVE     3012
#define LE_TCP_CONN_DROP_UNREACH	3013
#define LE_TCP_REASS_PKT_DROPPED	3014
/* New warning entries for 7_0 */
#define LE_TCP_DROP			3015

/* Resource Limitations */
#define	LE_TCP_NO_MEM			4000

/* Protocol Logs */
#define	LE_TCP_DRAIN			5000
#define	LE_TCP_INPUT			5001
#define	LE_TCP_SPORT_DPORT		5002
#define	LE_TCP_ACK			5003
#define	LE_TCP_NEW_WINDOW		5004
#define	LE_TCP_INPUT_EXIT		5005
#define	LE_TCP_OPT_PROC			5006
#define	LE_TCP_REASS			5007
#define	LE_TCP_PRESENT			5008
#define	LE_TCP_MMUPDT			5009
#define	LE_TCP_USRREQ_ENTRY		5010
#define	LE_TCP_USRREQ_EXIT		5011
#define	LE_TCP_OUTPUT			5012
#define	LE_TCP_OUTPUT_NIL		5013
#define	LE_TCP_RESPOND			5014
#define	LE_TCP_REXMT			5015

/* New protocol log entries for 7_0 */
#define LE_TCP_DATA_IN			5016
#define LE_TCP_DATA_OUT			5017
#define LE_TCP_USRREQ			5018

/* 8.0 TCP protocol logs */
#define LE_TCP_CHANGE_STATE		5019
#define LE_TCP_RESET_SENT		5020




	/********************************
	 *	Subsystem: PXP		*
	 ********************************/
/* Errors */
#define LE_PXP_INVALID_USER_OPTIONS	2000	/* user made invalid choice.*/

/* Warnings */
#define LE_PXP_SEQNUM_NO_MATCH		3000	/* can't find sequence no. */

/* Resource Limitations */
#define LE_PXP_RESOURCE_LIMITATION	4000	/* max queue size or no mem */

/* Protocol Logs */
#define LE_PXP_BAD_HEADER		5001	/* pkt w/bad pxp header */
#define LE_PXP_DUPLICATE_REQUEST	5002	/* dup request arrived.*/
#define LE_PXP_CTLINPUT			5003	/* ip: link will be closed.*/
#define LE_PXP_FUNC_NOT_SUPPORTED	5004	/* func not supported by pxp*/
#define LE_PXP_RETRY_EXCEEDED		5005	/* retry timed out. */



	/********************************
	 *	Subsystem: UDP		*
	 ********************************/

/* Protocol Logs */
#define LE_UDP_CKSUM_ERR		5000	/* checksum err in UDP pkt */
#define LE_UDP_CTLINPUT			5003	/* ip: link will be closed. */



	/********************************
	 *	Subsystem: IP		*
	 ********************************/
/* Warnings */
#define	LE_IP_TERM			3000	/* IP terminated */
#define	LE_IP_SHORT			3001
#define	LE_IP_ICMP_SHORT		3002
#define	LE_IP_PKT_DROPPED		3003
#define	LE_IP_CKSUM_ERR			3004
#define	LE_IP_ICMP_CKSUM_ERR		3005
#define	LE_IP_PKT_LOST			3006
#define LE_ICMP_PKT_DROPPED		3007

/* Resource Limitations */
#define	LE_IP_ICMP_NO_MEM		4000	/* m_get for header failed */
#define	LE_IP_NO_MEM			4001

/* Protocol Logs */
#define	LE_IP_DRAIN			5000
#define	LE_IP_ICMP_ERROR_MSG		5001
#define	LE_IP_ICMP_SEND			5002
#define	LE_IP_SRC_DST			5003
#define	LE_IP_INPUT			5004
#define	LE_IP_ICMP_INPUT		5005
#define	LE_IP_FRAG_PROC			5006
#define	LE_IP_FRAG_CMPLT		5007
#define	LE_IP_OPT_PROC			5008
#define	LE_IP_FORWARD			5009
#define	LE_IP_OUTPUT			5010
#define	LE_IP_OUTFRAG			5011
#define LE_IP_ICMP_DEST			5012
#define LE_IP_ICMP_SRC			5013


	/********************************
	 *	Subsystem: PROBE	*
	 ********************************/
/* Disasters */
#define LE_PR_NO_MACCT		1000	/* ma_create() fail at init*/

/* Errors */
#define LE_PR_BAD_INTERFACE	2000	/* neither IEEE or ETHER set */
#define LE_PR_UNSUP_FAMILY	2001	/* unsupported address family */
#define LE_PR_ZERO_SEQNO	2002	/* vna reply with 0 seq no */
#define LE_PR_VREQ_HDR_SMALL	2003	/* vna req with too small hdr */
#define LE_PR_VREQ_DATA_SHORT	2004	/* vna req with data too short */
#define LE_PR_VREQ_BAD_REPLEN	2005	/* vna req with bad report len */
#define LE_PR_VREQ_BAD_DREPLEN	2006	/* vna req with bad domain rep len*/
#define LE_PR_VREQ_BAD_VERSION	2007	/* vna req with bad version */
#define LE_PR_VREQ_BAD_DOMAIN	2008	/* vna req with bad domain */
#define LE_PR_VTAB_OVERFLOW	2009	/* vna table full w/no age */
#define LE_PR_ARP_SHORT		2010	/* arp packet too short */
#define LE_PR_ARP_DUP_IP	2012	/* arp packet duplicate IP addr */
#define LE_PR_BAD_MBUF_CHAIN	2013	/* pr_intr bad mbuf chain */
#define LE_PR_BAD_MBUF_LEN	2014	/* pr_intr bad mbuf length */
#define LE_PR_PULLUP_FAIL	2015	/* m_pullup fail in pr_intr */
#define LE_PR_BAD_HDR_LEN	2016	/* bad header length in pr_intr */
#define LE_PR_BAD_VERSION	2017	/* bad probe version in pr_intr */
#define LE_PR_BAD_PACKET_TYPE	2018	/* bad probe packet type */
#define LE_PR_NREQ_TOO_SMALL	2019	/* name req too short */
#define	LE_PR_SET_NODE_NAME	2020	/* node name changed */
#define	LE_PR_BAD_NPR_RCVD	2021	/* received a bad nodal path report */

/* Warnings */
#define LE_PR_BAD_PR_GLA	3000	/* bad ret from pr_g_lan_address*/
#define LE_PR_BAD_PR_VTNEW	3001	/* bad return from pr_vtnew */
#define LE_PR_VSEQ_NOT_FOUND	3002	/* vna reply has seq no not in table*/
#define LE_PR_BAD_GET_NODENAME	3004	/* bad return from ns_get_node_name */
#define LE_PR_BAD_SEND_UNSOL	3005	/* bad send of unsol reply */
#define	LE_PR_BAD_PR_NTNEW	3006	/* bad return from pr_ntnew */
#define LE_PR_NSEQ_NOT_FOUND	3007	/* name rep. has seq no not in table*/
#define LE_PR_BAD_BUILD_PATH	3008	/* build path report failure */
#define LE_PR_BAD_SEND_NREP	3009	/* bad send on name reply */

/* Resource Limitations */
#define LE_PR_BC_NO_MEM		4000	/* no mem for 2 interface broadcast */
#define LE_PR_VWHO_NO_MEM	4001	/* no mbuf in pr_vwhohas */
#define LE_PR_ARPWHO_NO_MEM	4002	/* no mbuf in arpwhohas */
#define LE_PR_UNSOL_NO_MEM	4003	/* mo mbuf for pr_unsol */
#define LE_PR_MA_CHARGE_FAIL	4004	/* could not charge mbuf in pr_intr */
#define LE_PR_NWHO_NO_MEM	4005	/* no mbuf in pr_nwhohas */
#define LE_PR_MPULLUP_NO_MEM	4006	/* no memory available for m_pullup()
					   of inbound packet */

/* Protocol Logs */
#define LE_PR_HOST_UNREACH	5000	/* cant find host */
#define LE_PR_VREQ_IP_NOT_IF	5001	/* vna req w/IP addr ~this interface*/
#define LE_PR_UNSUP_PACKET_TYPE	5002	/* unsupported probe packet type */
#define LE_PR_ARP_UNSUP_PROTO	5003	/* arp packet unsupported proto */



	/**********************************************
	 *	Subsystem: LAN  (ETHER, 802.3) Driver *
	 **********************************************/

/* Disasters */
#define LE_LAN_BIND_ERROR		1000	/* bind fail with CAM */
#define LE_LAN_CTRL_NOT_READY		1001	/*CIO CTRL msg reply HW ERROR*/
#define LE_LAN_DMA_NOT_READY		1002	/*CIO DMA msg reply HW ERROR*/
#define LE_LAN_TIMER_EVENT		1005	/*DMA or CTRL req timer has 
						  popped - card is dead*/
#define LE_LAN_EVENT_DOD		1008	/*LAN card status DEAD_OR_DYING;
						  reset or reboot*/
#define LE_LAN_EVENT_PER		1009    /*LAN card status PROTOCOL ERROR
						  - reset or reboot*/
#define LE_LAN_LLIO_HW_PROBLEM		1011    /* card has hardware problem */
#define LE_LAN_SOFTWARE_DISASTER        1014    /* internal error */
#define LE_LAN_EVENT_STF                1015    /* self test fail */
#define LE_LAN_EVENT_WTF                1016    /* write failure  */
#define LE_LAN_EVENT_APR                1017    /* Another Protocol Error    */
                                                /* (lan1 hw)                 */
#define LE_LAN_HARDWARE_DISASTER        1018    /* unknown hw error */
#define LE_LAN_DRAM_PARITY_ERR		1019	/* DRAM parity error detected */
#define LE_INIT_LOG_FAILURE             1020	/* failed to log reserved    */
						/* SAP/Type/Canonical address*/ 
#define LE_LAN_OFFLINE                  1021    /* card is offline */
#define LE_LAN_RAM_NA_CHG_FAILURE       1022	/* RAM node addr change failed*/
#define LE_LAN_HW_ID                    1023    /* wrong hardware id         */
#define LE_LAN_READ_NOVRAM_FAILURE      1024    /* cannot read card perm addr */


#define LE_LAN_EVENT_SCP_FAIL	       1025  /* 82596 couldnot init SCP addr */
#define LE_LAN_EVENT_596_SLFTST_FAIL   1026  /* 82596 did not pass its slftst */
#define LE_LAN_EVENT_596_INT_LB_FAIL   1027  /* 82596 did not pass its int lb */
#define LE_LAN_EVENT_INT_LB_501_FAIL   1028  /* ext lb through 501 failed    */
/* Errors */
#define LE_LAN_RINT_PKT_SIZE	       2001    /* the lan driver input routine
						   has detected a packet larger
						   than the posted read buffer*/
#define LE_LAN_EVENT_PER_RESET	       2002    /*LAN card status PROTOCOL ERROR
						  - card reset in progress */
#define LE_LAN_TIMER_EVENT_RESET       2003    /*DMA or CTRL timer popped */
						/* card reset in progress  */
#define LE_LAN_EVENT_EXT_LB_MAU_FAIL   2004  /* ext lb through MAU failed    */
#define LE_LAN_EVENT_596_CMD_TIMEOUT   2005  /* 82596 cmd timed out; driver  */
					     /* will be reset automatically  */
#define LE_LAN_EVENT_ILLEGAL_FRAME     2006  /* 82596 received an illegal  */
                                             /* sized frame */
#define LE_LAN_EVENT_82596_BUG         2007  /* 82596 chip bug detected */

/* Warnings */
#define LE_LAN_CTRL_POWERFAIL		3007	/*Powerfail, ctrl request 
						  aborted (CIO card) */
#define LE_LAN_EVENT_LAW		3016    /*Line error HW (NIO card) */
/* 3017 deleted */
/* 3018 deleted */
#define LE_LAN_UNEXPECTED_EVENT		3020	/*LAN received unexpected but */
                                                /*recoverable event           */

/* Resource Limitations */
#define LE_STRATEGY_NO_BUFFERS		4001	/*no mem to post read buffer 
						  to LAN card */
#define LE_IEOUTPUT_NO_BUFS		4002	/*No mem for outbound 802.3 pkt 
						  header*/
#define LE_LAN_CTRL_NO_RESOURCES	4003	/*No resources for ctrl event */

#define LE_OUTQ_FULL		        4007	/*outbound pkt dropped 
						  (LAN driver output q full) */
/* 4008 deleted */
/* 4009 deleted */

#define LE_LAN_EVENT_MBUF_ALLOC_FAIL    4010    /* no mem for rcving packets  */

/* Protocol Logs */
#define LE_OUTQ_NO_PACKETS		5000	/* driver output q empty, but 
						   write pending indicated*/
#define LE_IERINT_BAD_CTRL		5008	/*inbound 802.3 pkt dropped 
						 (unsupported/bad crtl field) */
#define LE_LOG_SAP                      5035	/* logged a SAP */
#define LE_LOG_TYPE                     5036	/* logged a Type */
#define LE_LOG_CANON                    5037	/* logged a Canonical addr */
#define LE_DROP_SAP                     5038    /* unlogged a SAP */
#define LE_DROP_TYPE                    5039    /* unlogged a Type */
#define LE_DROP_CANON                   5040    /* unlogged a Canonical addr */
#define LE_RINT_UNLOGGED_SAP            5041	/* received a frame for an  */
						/* unlogged SAP */
#define LE_RINT_UNLOGGED_TYPE           5042    /* received a frame for an  */
						/* unlogged Type */
#define LE_RINT_UNLOGGED_CANON          5043    /* received a frame for an  */
						/* unlogged Canonical addr  */
#define LE_TRAILER_PKT_DROPPED          5047    /* dropped a bad trailer pkt */
/* 5048 deleted */




	/**********************************************
	 *	Subsystem: X25 Driver                 *
	 **********************************************/
#ifdef PREX25ISU /* Before X.25 was an ISU */

#ifdef _KERNEL_BUILD
#include "../h/x25_diag.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/x25_diag.h>
#endif /* _KERNEL_BUILD */

#else /* PREX25ISU */

/* Now that X.25 is an ISU we must define the ioctl's */
/* for the X.25 driver ourselves.  This assumes that  */
/* the X.25 driver will not change the ioctl calls to */
/* turn on and off tracing and logging.               */
/* 31-Mar-92 TM                                       */
#define X25_TRACE_ON            _IOW('x', 16,  int)
#define X25_TRACE_OFF           _IOW('x', 17,  int)
#define X25_LOGGING_ON          _IOW('x', 18,  int)
#define X25_LOGGING_OFF         _IOW('x', 19,  int)
#define X25_RD_DRVTYPE          _IOR('x', 20,  int)
#define X25_RD_IFSTRUCT_INDEX   _IOR('x',21, int)

#endif /* PREX25ISU */



	/********************************
	 *   Subsystem: Buffer Manager	*
	 ********************************/

/* Errors */
#define LE_RCLFREE_NEG			2000	/* reserved clusters is neg */
/* Warnings */
#define	LE_BUFS_SBAPPEND_DROP		3000	/* sbappend returned data */

/* Resource Limitations */
#define LE_BUFS_NMBCLUSTERS_INUSE	4000	/* nmbcluster limit reached */
#define LE_BUFS_NO_CREDITS		4001	/* macct ran out of credits */

/* Protocol Events */
#define LE_BUFS_CDROPS		5001	/* m_get() failed, no credits */
#define LE_BUFS_ADROPS		5002	/* m_get(MA_NULL) failed */
#define LE_BUFS_RDROPS		5003	/* m_reserve() failed, no memory */



	/********************************
	 *	Subsystem: NFS		*
	 ********************************/
/* Disasters (100x) */

/* Errors (200x) */
#define LE_NFS_MACCT_CREATE		2000	/* failure to create macct */
#define LE_NFS_XDR_CALLHDR		2001	/* xdr failure on call hdr */
#define LE_NFS_PROTO_FIND		2002	/* failure to find protocol */
#define LE_NFS_SOCREATE			2003	/* failure to create socket */
#define LE_NFS_SOBIND			2004	/* failure to bind socket */

#define LE_NFS_XDRREPL			2020	/* couldn't encode reply */
#define	LE_NFS_READ_ERROR		2021	/* read failed */
#define LE_NFS_RWVP_SHORT_WRITE		2022	/* incomplete message */
#define LE_NFS_WRITE_ERROR		2023	/* write error */
#define LE_NFS_WRITE_ERROR_DEFAULT	2024	/* write error (default) */
#define LE_NFS_FUNC_FAIL_ERROR		2025	/* an NFS function fails */

/*
 * The following errors are associated with the NFS lock manager (LM)
 */
#define LE_NFS_LM_LOCKDENIED		2030	/* Blocking lock was denied */
#define LE_NFS_LM_UNLOCKDENIED		2031	/* Unlock request denied */
#define LE_NFS_LM_CANCEL_NOLOCKS	2032	/* Received ENOLCK on cancel */
#define LE_NFS_LM_NOT_REGISTERED	2033	/* Lock manager not registered*/
#define LE_NFS_LM_RPCERROR		2034	/* RPC protocol error with LM */
#define LE_NFS_LM_PORTMAP_NOT_UP	2035	/* Portmap isn't even up! */
#define LE_NFS_LM_NOT_UP		2036	/* LM not bound at given port*/

#define LE_NFS_TEST_ERROR		2099	/* debugging routine failure */

/*
 * Errors associated with the NFS export code
 */
#define	LE_NFS_COPYIN_FAIL		2100	/* copyin has failed */
#define	LE_NFS_COPYOUT_FAIL		2101	/* copyout has failed */

/* Warnings (300x) */
#define	LE_NFS_FUNC_FAIL_WARN		3000	/* a NFS function fails	*/
#define LE_NFS_SERVER_OK		3001	/* server responding */
#define LE_NFS_SERVER_GIVE_UP		3002	/* give up trying for server */
#define LE_NFS_SERVER_TRYING		3003	/* try again for server	*/
#define LE_NFS_NO_BODY			3004	/* message without data	*/
#define LE_NFS_BAD_LEN			3005	/* message too long for	*/
#define LE_NFS_PCBADDR			3006	/* pcbsetaddr failed */
#define LE_NFS_FSEND_FAIL		3007	/* lan driver send failure */
#define LE_NFS_UNPRIV_REQ		3008	/* unprivileged request	*/

#define	LE_NFS_AUTH_LEN 		3020	/* authentication too long */
#define LE_NFS_MARSHAL_FAIL		3021	/* authkern_marhsal fails */
#define	LE_NFS_U_SHORT_DECODE		3022	/* u_short decode fails	*/
#define	LE_NFS_BOOL_DECODE		3023	/* bool decode fails */
#define	LE_NFS_OPAQUE_DECODE		3024	/* opaque decode fails */
#define	LE_NFS_OPAQUE_ENCODE		3025	/* opaque encode fails */
#define	LE_NFS_BYTES_SIZE		3026	/* bytes: get size fails*/
#define	LE_NFS_BYTES_BAD		3027	/* bytes: wrong size */
#define	LE_NFS_ENUM_DSCMP		3028	/* enum get wrong discriminant*/
#define	LE_NFS_STRING_SIZE		3029	/* string: get size fails */
#define	LE_NFS_STRING_BAD		3030	/* string: wrong size */
#define	LE_NFS_ARRAY_SIZE		3031	/* array: get size fails */
#define	LE_NFS_ARRAY_BAD		3032	/* array: wrong size */	

#define LE_NFS_OVERLAP			3040	/* long crosses mbuf boundary */
#define	LE_NFS_LONG_BADOP		3041	/* operation on long fails */
#define	LE_NFS_U_LONG_BADOP		3042	/* operation on u_long fails */
#define	LE_NFS_STRING_BADOP		3043	/* string gets bad op */
#define	LE_NFS_BOOL_BADOP		3044	/* xdr_bool gets a bad op */
#define	LE_NFS_BYTES_BADOP		3045	/* xdr_bytes gets bad op */
#define	LE_NFS_U_SHORT_BADOP		3046	/* xdr_u_short gets a bad op */
#define	LE_NFS_OPAQUE_BADOP		3047	/* xdr_opaque gets bad op */

/*
 * Warnings associated with NFS lock manager (LM)
 */
#define LE_NFS_LM_PORTMAP_GIVE_UP       3050	/* Give up talking to portmap*/
#define LE_NFS_LM_PORTMAP_TRYING	3051	/* Still trying for portmap */
#define LE_NFS_LM_PORTMAP_OK		3052	/* Portmap responded */

/*
 * Warnings associated with the NFS export code
 */
#define	LE_NFS_ROOT_ONLY		3100	/* to be done only by root */
#define	LE_NFS_LOOKUP_FAIL		3101	/* lookupname() failed */
#define	LE_NFS_BAD_FLAGS		3102	/* bad flags in export info */
#define	LE_NFS_LOAD_ADDR		3103	/* loadaddrs failed */
#define	LE_NFS_UNEXPORT_FAIL		3104	/* unexport failed */
#define	LE_NFS_GETFH_FAIL		3105	/* old nfs_getfh failed */
#define	LE_NFS_MAKEFH_FAIL  		3106	/* makefh failed */
#define	LE_NFS_FINDEXIVP_FAIL		3107	/* findexivp failed */

/* Resource Limitations (400x) */
#define LE_NFS_BIND_MGET		4000	/* no mbufs for bind */
#define LE_NFS_FSEND_MGET		4001	/* no mbufs for udp/ip hdr */
#define LE_NFS_FRAG_MGET		4002	/* no mbufs for fragmentation */
#define LE_NFS_DUP_MGET			4003	/* no mbufs for dup udp/ip hdr*/
#define LE_NFS_MCLGETX_FAIL		4004 	/* mclgetx failure in putbuf */
#define LE_NFS_MGET_FAIL		4005    /* MGET failure in putbuf */
#define LE_NFS_RROK_MBUF_FAIL		4006	/* MGET failure in xdr_rrok */	
#define LE_NFS_EXPAND			4007	/* can't expand macct */
#define LE_NFS_OVERDRAW			4008	/* NFS macct overdrawn */
#define LE_NFS_ENOMEM			4009	/* out of memory */

/* Protocol Logs (500x) */

	/*************************
	 *   Subsystem: NetISR   *
	 *************************/

/* Errors */

#define	LE_NETISR_QUEUE_FULL	2001	/* NetISR event queue full */
#define	LE_NETISR_UNKNOWN_EVENT	2002	/* Unknown/illegal event */

/* Warnings */

#define	LE_NETISR_NOT_RUNNING	3001	/* NetISR daemon not running */
#define	LE_NETISR_DROPPED_MBUF	3002	/* Receive queue full */
/* Disasters (100x) */
/* Errors (200x) */
/* Protocol Logs (500x) */

	/********************************
	 *	Subsystem: NSE		*
	 ********************************/

/* Warnings */

#define LE_NSE_CANT_UNLINK		3000	/* smunlink couldn't unlink */

/* Resource Limitations */

#define LE_NSE_NO_STREAMS		4000	/* out of streams */
#define LE_NSE_NO_QUEUES		4001	/* out of queues */
#define LE_NSE_NO_STREVENT		4002	/* out of stream events */
#define LE_NSE_NO_KMEM			4003	/* out of kmem in sealloc */

/* Protocol Logs */

	/********************************
	 *	Subsystem: STRLOG	*
	 ********************************/

/* Warnings */

/* Resource Limitations */

#define LE_STRLOG_NO_LOG		4000	/* out of log drivers */

/* Protocol Logs */
	/********************************
	 *	Subsystem: TIRDWR	*
	 ********************************/

/* Warnings */

#define LE_TIRDWR_STRHEAD_WARN		3000	/* smunlink couldn't unlink */

/* Resource Limitations */

#define LE_TIRDWR_NO_TIRDWR		4000	/* out of tirdwr */
#define LE_TIRDWR_CANT_ALLOCB		4001	/* cannot allocb at open */

/* Protocol Logs */

	/********************************
	 *	Subsystem: TIMOD	*
	 ********************************/

/* Warnings */

/* Resource Limitations */

#define LE_TIMOD_NO_TIMOD		4000

/* Protocol Logs */


/*
 * network trace record definition
 */
struct ns_trace_link_hdr {
	short			tr_event;	/* event traced:TR_LINK_xxx */
	short			tr_if;		/* i.f. traced (0 @ rel. 1) */
	short			tr_traced_len;	/* byte len of data traced  */
	short			tr_pkt_len;	/* byte len of original pkt */
	int			tr_pid;		/* curr. process (-1=ICS)   */
	struct timeval	        tr_time;	/* std timeval		    */
	short  			tr_subsys;	/* subsystem                */
};

/*
 * link trace events
 */

#define TR_LINK_INBOUND		0x0001		/* Inbound packet 	*/
#define TR_LINK_OUTBOUND	0x0002		/* Outbound packet	*/
#define TR_LINK_START		0x0004		/* Trace start rec.	*/
#define TR_LINK_QUIT		0x0008		/* Trace terminate rec.	*/
#define TR_LINK_TEST		0x0010		/* Test packet		*/
#define TR_LINK_DROPPED		0x0020		/* Data was dropped	*/
						/* the pkt_len contains */
						/* the # of dropped bytes*/
#define TR_LINK_LOOP		0x0040		/* loopback packet	*/

#define TR_X25_L2_IN_HEADER     0x0080          /* x25 Level 2 inbound header */
#define TR_X25_L2_IN_FRAME      0x0100          /* X25 Level 2 inbound frame */
#define TR_X25_L2_OUT_HEADER    0x0200          /* X25 Level 2 outbound header*/
#define TR_X25_L2_OUT_FRAME     0x0400          /* X25 Level 2 outbound frame */
#define TR_X25_L3_IN_HEADER     0x0800          /* X25 Level 3 inbound header */
#define TR_X25_L3_IN_PACKET     0x1000          /* X25 Level 3 inbound packet */
#define TR_X25_L3_OUT_HEADER    0x2000          /* X25 Level 3 outbound header*/
#define TR_X25_L3_OUT_PACKET    0x4000          /* X25 Level 3 outbound packet*/
#define TR_X25_LOOPBACK         0x8000          /* X25 loopback driver */

/*
 * link trace tr_if definitions
 */

  /* link type and unit number are coded in ONE value */
  /* First 2 bytes are Link Type: Last 2 Bytes are Unit Number */

#define TR_IF_UNITMASK		0x00ff		/* If unit number mask	*/
#define TR_IF_TYPEMASK		0xff00		/* If type mask		*/
#define TR_IF_NUNITS		8		/* Number of bits we can 
						   encode unit no. in 	*/
#define TR_IF_NTYPES		7		/* number of types we can
						   encode in tr_if - 1  */
#define TR_IF_LAN		(1<<TR_IF_NUNITS) /* Flag for lan if	*/
#define TR_IF_LOOP		(2<<TR_IF_NUNITS) /* Loopback if 	*/
#define TR_IF_NI		(4<<TR_IF_NUNITS) /* Point-to-Point if	*/
#define TR_IF_X25               (8<<TR_IF_NUNITS) /* X25 if             */


/*
 * EN Filtering
 */
#ifndef NS_QA
#define NS_LINKTR_ENFILTER(p, l) /* a comment */
#else  /* NS_QA */

/* #define NS_LINKTR_ENFILTER(p, l) \
	if ((ns_linktr_en_len > 0) && !ns_linktr_dofilter(p,l/sizeof(short))) \
                return(0);  */
#endif /* NS_QA */

#define NS_TR_ENMAXFILTERS	600	/* maximum total filter short words */

/* 
 * The ENFILTER op-codes.
 */

/* these two must sum to 16!  */
#define	NS_TR_ENF_NBPA	10			/* # bits / action */
#define	NS_TR_ENF_NBPO	6			/* # bits / operator */

/*  binary operators  */
#define NS_TR_ENF_NOP	(0<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_EQ	(1<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_LT	(2<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_LE	(3<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_GT	(4<<NS_TR_ENF_NBPA)
#define NS_TR_ENF_GE	(5<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_AND	(6<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_OR	(7<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_XOR	(8<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_COR	(9<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_CAND	(10<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_CNOR	(11<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_CNAND	(12<<NS_TR_ENF_NBPA)
#define	NS_TR_ENF_NEQ		(13<<NS_TR_ENF_NBPA)

/*  stack actions  */
#define	NS_TR_ENF_NOPUSH	0
#define	NS_TR_ENF_PUSHLIT	1	
#define	NS_TR_ENF_PUSHZERO	2
#define	NS_TR_ENF_PUSHWORD	16



/*
 * tracing globals
 */
#ifdef _KERNEL
/* extern int	ns_linktr_filter;
extern int	ns_linktr_max_size;
extern int	ns_linktr_ms_id;
extern int	ns_linktr_buffer_wsize;
extern int	ns_linktr_pid;
extern struct   ns_tr_ifmask ns_linktr_ifmask; */
#ifdef NS_QA
extern int  	ns_linktr_en_len;
extern u_short  ns_linktr_en_filter[];
#endif /* NS_QA */
#endif /* _KERNEL */



/*
 * network log record definition
 */
struct ns_log_rec {
	short			log_event;	/* event logged: LE_xxx */
	short			log_location;	/* location of log	*/
	short			log_class;	/* log class: NS_LC_xxx	*/
	short			log_subsys;	/* log subsys: NS_LS_xxx */
	struct timeval	        log_time;	/* std timeval */
	int			log_error;	/* error if any	*/
	int			log_context;	/* option context info */
	int			log_mask;	/* current file log mask*/
	int			log_console_mask;	/* console mask */
	int			log_pid;	/* curr. process (-1=ICS)*/
	short			log_flags;	/* see NS_LF_ below */
	unsigned short		log_data_dropped;/* amount of data lost */
	unsigned short		log_dropped;	/* # dropped events */
	unsigned short		log_dlen;	/* total dlen of associated */
						/* log data in bytes (header*/
						/* not included. */
};

#define NSDIAG_MAX_DATA         20

typedef struct {
	struct ns_log_rec log;
            	union {
			caddr_t ptr;
			char    chars[NSDIAG_MAX_DATA];
			int     words[NSDIAG_MAX_DATA/sizeof(int)];
		} data;
	} nsdiag_event_msg_type;

/*
 * log flags see log_flags above (also a parameter to ns_log_event)
 */
#define NS_LF_NODATA		1
#define NS_LF_DATA		2
#define NS_LF_STR		4
#define NS_LF_INFO		8
#define NS_LF_DATA_DROPPED	16
#define NS_LF_DATA_INMBUF	32



/* 
 * Log event bitmaps 
 */

#define NSL_EVENT2CLASS(e)	(e)/LC_OFFSET
#define NSL_EVENT2BYTE(e)	((e)%LC_OFFSET)/8 
#define NSL_EVENT2BIT(e)	((e)%LC_OFFSET)%8	

/* This struct is used by kernel logging routines to FILTER various
   events.  The logging is broken into Subsystems, Classes and Events 

   Subsystem is a major Protocol or Module

   Classes  are: Logging Events,
                 Disasters
                 Errors
                 Warnings
                 Resource Limitations
                 Protocol Specific Information

   Events are:   Events indentify where in the code that logging event
	         was called from.

   In the structure below the logging EVENTS are BIT coded in the
   nslbm_bitmap array.  Each bit is one event.  If the bit is 0
   that means do not filter this event else filter this event.

   FILTER means: dump the log event.

   I hope this code makes more sense now that it has some comments.

*/

struct ns_log_ioctl_bm {
	char 	nslbm_subsys;			/* Subsystem of this bitmask */
	char	nslbm_class;			/* Class of this bitmask     */
	char	nslbm_bitmap[LC_OFFSET/8];	/* 8 bits/byte * LC_OFFSET/8 
						 * bytes = LC_OFFSET events 
						 * per class bitmask.	     */
};

/* Kernel log event bitmap structure.  Each {subsystem,class} tuple has
 * one of these, and it takes two mbufs to store the bitmask data.
 */

#define	NS_BMF_INUSE	0x1
#define	NS_BMF_	0x1

struct  ns_log_kern_bm {
	int		nslbm_flags;		/* NS_BMF_*		     */
	struct	mbuf	*nslbm_mb1;		/* First mbuf for mask data  */
	struct	mbuf	*nslbm_mb2;		/* 2nd mbuf for mask data    */
};


/*
 * logging globals
 */
#ifdef _KERNEL	
/*
extern int	ns_log_mask;
extern int	ns_console_log_mask;
extern int	ns_log_subsys_mask;
extern int	ns_log_take_action;
extern int	ns_log_action_event;
extern int	ns_log_action_subsys;
extern int	ns_log_action_class;
extern struct   ns_log_kern_bm	nsl_bitmap[NS_LS_COUNT][NS_LC_COUNT];
extern int      log_on;
extern int      netdiag_log_map[];
*/
#else  /* _KERNEL */
/*
 * return values from ns_log_init()
 */
#define ELOG_FILE_OPEN		1	/* can't open log file	*/
#endif /* _KERNEL */


#define MSG_OFFSET	3		/* Offset of 2nd line of log msg */


#ifdef NS_LOG_STRINGS
#ifdef _KERNEL
char 	*ns_log_str = "%s GMT: Network %s %s %u, pid %s\r\n";
#else /* not _KERNEL */
char 	*ns_log_str = "%s: Network %s %s %u, pid %s\n";
#endif

/* char *ns_logclass_str[NS_LC_COUNT] ={"Logging Status", "Disaster", "Error", 
		"Warning", "Resource Limit", "Protocol Log"};

char *ns_logsubsys_str[NS_LS_COUNT] ={"NSDIAG", "NFT", "RFADAEMON", 
		"RFA REQUESTOR", "IPC", "SOCKREGD", 
		"TCP", "PXP", "UDP", "IP", "PROBE", "LAN", 
		"RLBDAEMON", "BUFS", "CASELIB", "CASEROUTER", "NFS",
		"NetISR", "X25", "NSE", "STRLOG", "TIRDWR", "TIMOD" };
*/
#endif /* NS_LOG_STRINGS */

#endif /* not _SYS_NS_DIAG_INCLUDED */
