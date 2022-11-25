/*@(#) $Revision: 1.9.83.4 $ */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
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

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#ifndef _SYS_PROTOCOL_INCLUDED	/* allows multiple inclusion */
#define _SYS_PROTOCOL_INCLUDED

#define ADDRESS_SIZE		6	/*size of ETHERNET ADDRESS*/

#ifdef _KERNEL

#if defined (__hp9000s800) && defined(flags)
/*
 * flags is defined in ../h/ptrace.h and conflicts with structures
 * we define.
 * ../machine/machdep.c includes cct.h, which includes this file.  The
 * following hack gets rid of this problem.  NOTE: cct.h only wants our
 * define of "ADDRESS_SIZE", Oh, tis a tangled web we weave.
 */
#undef flags
#endif

#ifdef __hp9000s300
#define MAX_ETHER_LENGTH        1514
#else
#define MAX_ETHER_LENGTH        1500 /*800 driver's limit*/
#define NUM_DUX_MBUFS 128*NMBPCL  /* number of mbufs "reserved" for DUX */
#endif

#define MINI_ETHER_LENGTH	60

/*
** The following params are the maximum values allowable for
** the corresponding configurable parameters.
*/
#define MAX_SERVING_ARRAY	200	/* conf.c: serving_array_size*/

/*
 * The following provide mechanisms for longer-term manipulations
 * of the DUX_Q and using/serving entries/elements
 */
#define spl_duxQ()   spl6()
#define spl_arrays() spl6()

/*
 * The following provide quick mechanisms for short manipulations
 * of the DUX_Q and using/serving entries/elements
 */
#if defined(__hp9000s700) || defined(__hp9000s800)
#define CRIT_duxQ()		spl6()
#define UNCRIT_duxQ(x)		splx(x)
#define CRIT_arrays()		spl6()
#define UNCRIT_arrays(x)	splx(x)
#else
#define CRIT_duxQ()		CRIT()
#define UNCRIT_duxQ(x)		UNCRIT(x)
#define CRIT_arrays()		CRIT()
#define UNCRIT_arrays(x)	UNCRIT(x)
#endif

/*
 *  Read the buffer address from a ring descriptor pointed to by 'addr'
 *  The 24 bit address is in the AMD LANCE format:
 *
 *		     even	    odd
 *		+-------------+-------------+
 *	addr	| middle byte |	 low byte   |  addr+1
 *		+-------------+-------------+
 *	addr+2	|	      |	 high byte  |  addr+3
 *		+-------------+-------------+
 *
 *  How it works:
 *	1.  read the higher order byte, and shift it.
 *	2.  read the lower order word.
 *	3.  add it and then type cast it.
 */

#define READ_ADDR( addr )        ( (unsbyte *)                       \
                                ( (*((unsbyte *) (addr) + 3) << 16) \
                                + *((unsword *) (addr))             \
                                + LOGICAL_IO_BASE))

#ifdef __hp9000s300
#define DUX_COPY_ADDRESS(from, to)	LAN_COPY_ADDRESS(from, to)
#else
#define DUX_COPY_ADDRESS(from, to)	bcopy(from, to, 6)
#endif

/*
** using_array structure  - Always manipulated at spl_arrays();
*/
struct using_entry {
	u_char		flags;
	u_char		no_retried;	/* # retries in sending request */
	short		byte_recved;	/* byte received, excluding header */
	u_long		rid;		/* request id			*/
	dm_message	req_mbuf;	/* points to request dmmsg 	*/
	dm_message	rep_mbuf;	/* points to reply dmmsg	*/
	struct using_entry *req_waiting_next;	/* Next request that is
						 * waiting to be sent.
						 */
	};

/*
** Using array flags
*/
#define	U_USED		0x01	/* this using_entry is used */
#define U_IDEMPOTENT	0x02	/* this request is idempotent */
#define U_IN_DUXQ	0x04	/* the request message is in duxQ */
#ifndef __hp9000s300
#define U_REPLY_RCVD	0x08	/*received reply while request in LAN driver Q*/
#endif
#define U_RETRY_IN_PROGRESS	0x10
#define U_REPLY_IN_PROGRESS	0x20
#define U_COUNTED		0x40	/* request counted in protocol window */
#define U_OUTSIDE_WINDOW	0x80	/* request is waiting in clustab[] Q */

/*
** serving_array structure - always manipulated at spl_arrays()
*/
struct serving_entry {
	u_char	flags;
	u_char	req_address[ADDRESS_SIZE];
	u_char  lan_card;    /* Lan card if req_site is 0 */
	short	req_index;   /* index to requesting using_array[]	*/
	site_t	req_site;    /* Site that sent this request */
	u_long	rid;	     /* requesting rid			*/
	dm_message msg_mbuf; /* points request before calling dm_recv_request;
				points reply while sending the reply */
	short	byte_recved; /* number of bytes recved */
	u_char	state;	     /* NOT_ACTIVE, RECVING_REQUEST, SERVING,
				SENDING_REPLY, WAITING_ACK, ACK_RECEIVED */
	u_char  no_retried;  /* number of retries in sending the reply */
	int	time_stamp;  /* lbolt value of last rec'ed packet */
   };

/*
** Serving array flags
*/
#define S_USED		0x01
#define S_IDEMPOTENT	0x02	/* this operation is idempotent */
#define S_IN_DUXQ	0x04	/* the reply message is in duxQ */
#define S_SLOW_REQUEST  0x08
#define S_MULTICAST	0x10	/* the request is a clustercast */
#ifndef __hp9000s300
#define	S_ACK_RCVD	0x20    /* received ack while reply in LAN driver Q*/
#endif

/* state */
#define NOT_ACTIVE	0
#define RECVING_REQUEST 1
#define SERVING		2
#define SENDING_REPLY	3
#define WAITING_ACK	4
#define ACK_RECEIVED	5	/* ACK arrived while SENDING_REPLY */

#ifndef __hp9000s300
#define copy_network_data(s, d, l)	bcopy(s, d, l)
#endif

#if defined(__hp9000s700) || defined(__hp9000s800)
/*
 * Defines for the lanc_link_status returns, used only by DUX
 */
#define LANC_LS_OPEN_LAN     1
#define LANC_LS_BAD_MAU      2
#define LANC_LS_CARRIER      4
#define LANC_LS_DEAD         8
#define LANC_LS_MAU_CONECT  16
#define LANC_LS_HUNG        32
#endif  /* s700 || s800 */
#endif /* _KERNEL */

/*
** The following definitions are only used by the protocol code
** for statistical purposes. Since the counters are mainly in
** error condition types of code, I felt that they should be kernel
** resident all of the time. The stats will allow a knowledgeable kernel
** engineer to determine if important resources are properly configured.
** (e.g. mbufs/cbufs/using_arrays/serving_arrays/fsbufs/timeouts..)
**
*/

struct receive_request_stats {

	u_short lost_first_pkt;		/* recv_request()	*/
	u_short no_mbuf;
	u_short no_cluster;
	u_short no_buf;
	u_short no_buf_hdr;
	u_short not_clustered;
	u_short dup_req;
	u_short OOS;
	u_short reply_alive_nobuf;
	u_short pkt_alloc_nobuf;
};

struct retry_request_stats {
	u_short RetryReq;		/* retry_request()	*/
};

struct receive_reply_stats {
	u_short unexpected_msg;		/* recv_reply()		*/
	u_short RetryReply;		/* retry_reply()	*/
};

struct recv_ops {
	u_int type[7];
};

struct xmit_ops {
	u_int type[7];
};

struct xmit_rcodes {
	u_int e_llp_no_buffer_send;
	u_int e_hw_failure;
};

struct protocol_stats {

	/*
	** recv_request() and retry_request() specific stats.
	*/
	struct receive_request_stats	recv_req;
	struct retry_request_stats	retry_req;

	/*
	** recv_reply() and retry_reply() specific stats.
	*/
 	struct receive_reply_stats	recv_rep;

	/*
	** dux_recv_routines(): Different types of req/rep/ack/nak/etc...
	** messages that were received by this node.
	*/
	struct recv_ops recv_ops;

	/*
	** dux xmit routines: Different types of req/rep/ack/nak/etc...
	** messages that were transmitted by this node.
	*/
	struct xmit_ops xmit_ops;

	/*
	** dux xmit routines: Different types of error conditions that
	** could be returned by hw_send_dux().
	*/
	struct xmit_rcodes xmit_rcodes;

	/*
	** misc. function stats.
	*/

	u_short	WaitingUsingEntry;
	u_short	NoFreeServingEntry;
	u_short	SendAckNoMbuf;
	u_short	NetSlowReqNoMbuf;
	u_short	DuxHwSendDeltaSec;
	u_short	RecvDgramNoBuf;
	u_short	RecvDgramNoBufHdr;
	u_short	RecvDgramNoMbuf;
	u_short	RecvDgramNoCluster;
	u_short	DuxRecvRouBadFlags;
	u_short	DuxRecvRouBadSrcsite;
	u_short	DuxRecvRouNotMember;
	u_short	NetReqNotMember;

	/*
	** these stats are incremented whenever an ACK arrives while we
	** are still in the process of sending the reply.
	*/
	u_short fast_ack;		/* one ACK arrived */
	u_short many_fast_acks;		/* more than one ACK arrived */

	/*
	** An _old_ serving_entry.  This statistic is incremented when
	** we run out of serving entries but find that there are _old_
	** ones that we can free and reuse.
	*/
	u_short old_serving_entry;	/* an _old_ serving_entry */
} proto_stats;

/*
** The following are short hand notations for otherwise long definitions.
*/
#define	STATS_lost_first	     proto_stats.recv_req.lost_first_pkt
#define	STATS_recv_no_mbuf	     proto_stats.recv_req.no_mbuf
#define	STATS_recv_no_cluster	     proto_stats.recv_req.no_cluster
#define	STATS_recv_no_buf	     proto_stats.recv_req.no_buf
#define	STATS_recv_no_buf_hdr	     proto_stats.recv_req.no_buf_hdr
#define	STATS_not_clustered	     proto_stats.recv_req.not_clustered
#define	STATS_dup_req		     proto_stats.recv_req.dup_req
#define STATS_recv_req_OOS	     proto_stats.recv_req.OOS
#define STATS_reply_alive_nobuf	     proto_stats.recv_req.reply_alive_nobuf
#define STATS_pkt_alloc_nobuf	     proto_stats.recv_req.pkt_alloc_nobuf
#define	STATS_req_retries            proto_stats.retry_req.RetryReq
#define	STATS_unexpected	     proto_stats.recv_rep.unexpected_msg
#define	STATS_retry_reply	     proto_stats.recv_rep.RetryReply
#define	STATS_waiting_using	     proto_stats.WaitingUsingEntry
#define	STATS_serving_entry	     proto_stats.NoFreeServingEntry
#define	STATS_ack_no_mbuf	     proto_stats.SendAckNoMbuf
#define	STATS_slow_no_mbuf	     proto_stats.NetSlowReqNoMbuf
#define	STATS_delta_sec		     proto_stats.DuxHwSendDeltaSec
#define	STATS_recv_dgram_no_mbuf     proto_stats.RecvDgramNoMbuf
#define	STATS_recv_dgram_no_cluster  proto_stats.RecvDgramNoCluster
#define	STATS_recv_dgram_no_buf      proto_stats.RecvDgramNoBuf
#define	STATS_recv_dgram_no_buf_hdr  proto_stats.RecvDgramNoBufHdr
#define	STATS_recv_bad_flags	     proto_stats.DuxRecvRouBadFlags
#define	STATS_recv_bad_srcsite	     proto_stats.DuxRecvRouBadSrcsite
#define	STATS_recv_not_member        proto_stats.DuxRecvRouNotMember
#define	STATS_req_not_member	     proto_stats.NetReqNotMember
#define STATS_recv_op_P_REQUEST	     proto_stats.recv_ops.type[0]
#define STATS_recv_op_P_REPLY	     proto_stats.recv_ops.type[1]
#define STATS_recv_op_P_ACK	     proto_stats.recv_ops.type[2]
#define STATS_recv_op_P_NAK	     proto_stats.recv_ops.type[3]
#define STATS_recv_op_P_SLOW_REQUEST proto_stats.recv_ops.type[4]
#define STATS_recv_op_P_DATAGRAM     proto_stats.recv_ops.type[5]
#define STATS_recv_op_P_FORWARD      proto_stats.recv_ops.type[6]
#define STATS_xmit_op_P_REQUEST	     proto_stats.xmit_ops.type[0]
#define STATS_xmit_op_P_REPLY	     proto_stats.xmit_ops.type[1]
#define STATS_xmit_op_P_ACK	     proto_stats.xmit_ops.type[2]
#define STATS_xmit_op_P_NAK	     proto_stats.xmit_ops.type[3]
#define STATS_xmit_op_P_SLOW_REQUEST proto_stats.xmit_ops.type[4]
#define STATS_xmit_op_P_DATAGRAM     proto_stats.xmit_ops.type[5]
#define STATS_xmit_op_P_FORWARD      proto_stats.xmit_ops.type[6]
#define STATS_xmit_no_buffer_send   proto_stats.xmit_rcodes.e_llp_no_buffer_send
#define STATS_xmit_hw_failure	     proto_stats.xmit_rcodes.e_hw_failure
#define STATS_fast_ack		     proto_stats.fast_ack
#define STATS_many_fast_acks	     proto_stats.many_fast_acks
#define STATS_old_serving_entry	     proto_stats.old_serving_entry

#endif /* _SYS_PROTOCOL_INCLUDED */
