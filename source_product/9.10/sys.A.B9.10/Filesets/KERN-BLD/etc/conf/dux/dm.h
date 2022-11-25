/*
 * @(#)dm.h: $Revision: 1.10.83.3 $ $Date: 93/09/17 16:39:57 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */

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

	  Use,	duplication,  or disclosure by the Government  is
	  subject to restrictions as set forth in subdivision (b)
	  (3)  (ii)  of the Rights in Technical Data and Computer
	  Software clause at 52.227-7013.

		     HEWLETT-PACKARD COMPANY
			3000 Hanover St.
		      Palo Alto, CA  94304
*/

#ifndef _SYS_DM_INCLUDED	/* allows multiple inclusion */
#define _SYS_DM_INCLUDED

#ifndef _SYS_TYPES_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/types.h>
#endif /* _KERNEL_BUILD */
#endif /* _SYS_TYPES_INCLUDED */

typedef long mid_t;	/*for now*/

/* structures for Distribution Manager */

/*
 * array element indicating who to call when request comes in.
 */
struct dm_funcentry
{
	char dm_how;	/* how function should be called (see below) */
	int (*dm_callfunc)();
};

#ifdef _KERNEL_BUILD
#include "../h/mbuf.h"
#include "../dux/duxparam.h"
#include "../dux/dux_hooks.h"
#include "../dux/protocol.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/mbuf.h>
#include <dux/duxparam.h>
#include <dux/dux_hooks.h>
#include <dux/protocol.h>
#endif /* _KERNEL_BUILD */

/*
 * WARNING:
 *    All structures tagged with "DUX MESSAGE STRUCTURE" are passed
 *    between machines.	 They must obey the following rules:
 *	1)  All integers (and larger) must be 32 bit aligned
 *	2)  They must be consistent between all versions of DUX in the
 *	    cluster.
 */

/*
 * IEEE802 Header.  This header is used in HP's NS architecture on
 * PA-RISC machines.
 */
struct hpxsap_headers {
	u_char destaddr[ADDRESS_SIZE]; /*destination address */
	u_char sourceaddr[ADDRESS_SIZE];/* source address */
	u_short length; /* packet length, excluding the addresses and length*/
	u_char dsap; /*remote sap should be IEEE802_EXP_ADR for DUX */
	u_char ssap; /*local sap */
	u_char ctrl; /*control field, should always be NORMAL_FRAME for DUX*/
	u_char hdr_fill[3]; /*padding for 32 bit alignment */
	u_short dxsap; /*extended remote sap;should be DUX_PROTOCOL for DUX*/
	u_short sxsap; /*extended local sap */
};

struct proto_header {		/*DUX MESSAGE STRUCTURE*/
	struct hpxsap_headers iheader;	 /* 802.3 header */
	site_t  real_dest; /* Used to be p_length. If p_flags & P_FORWARD
			      then this is real destination */
	u_short p_flags;	/* request/response, idempotent, etc.	*/
	u_long	p_rid;		/* request id	*/
	short	p_byte_no; /* first byte of the message in this packet	*/
			   /* first byte of a message is byte 0		*/
	short	p_dmmsg_length; /* in this message	*/
	short	p_data_length;	/* in this message	*/
	short	p_data_offset;	/* in this packet, from XMIT_OFFSET	*/
	short	p_req_index;	/* index of the using_array	*/
	short	p_rep_index; /* index of the serving_array, used in the */
			     /* 3-way handshake of non-idempotent reply and */
			     /* in dux_hw_send to release serving_array[].  */
	site_t	p_srcsite;	/*source site id*/
	u_char	p_retry_cnt;	/* retry counter */
	u_char	p_version;	/* track changes */
	int	p_seqno;	/* prevent out-of-order reception */
	};

/* p_flags */
#define P_REQUEST	0x01
#define P_REPLY		0x02
#define P_ACK		0x04
#define P_DATAGRAM	0x08
#define P_END_OF_MSG	0x10
#define P_IDEMPOTENT	0x20
#define P_SLOW_REQUEST	0x40
#define P_MULTICAST	0x80
#define P_ALIVE_MSG	0x0100
#define P_NAK		0x0200
#define P_SUICIDE	0x0400
#define P_FORWARD	0x0800

/* p_version */
#define P_VERS_8_0	0xa5	/* added sequence number */

/*
 * The header
 */
struct dm_header
{
	/* protocol header */
	struct proto_header dm_ph;

	/* the following are not transmitted */
	struct dm_no_transmit {
		int		(*dmn_func)();	/*function to call when
							op complete.
							also function for net
							server to call*/
		u_int		dmn_bufoffset;	/*offset of first
							significant byte in
							the buffer*/
		struct buf	*dmn_bufp;	/*buf header pointer*/
		mid_t		dmn_mid;	/*the message ID*/
		site_t		dmn_dest;	/*the destination*/
		short		dmn_flags;	/*non transmitted flags*/
	} dm_no_transmit;

	/* the following is transmitted */
	struct dm_transmit {			/*DUX MESSAGE STRUCTURE*/
		short		dmt_tflags;	/*transmitted flags*/
		short		dmt_headerlen;	/*size of transmitted data
						 *(including dm_transmit
						 *and dm_params)
						 */
		short		dmt_datalen;	/*size of disc buffer (if any)*/
		u_char		dmt_acflag;	/*value of the u.u_acflag*/
		u_char		dmt_lan_card;	/*Which lan card to use*/
		union
		{
			struct			/*stuff used only with request*/
			{
				u_short dmt_op; /*remote op to be invoked*/
				u_short dmt_pid;/*process ID of requetor*/
			} dmt_requeststuff;
			struct			/*stuff used only with reply*/
			{
				u_short dmt_rc; /*The return code should be set
						 *as follows:  If everything
						 *works correctly, it should be
						 *0.  If an error occurred on
						 *the remote site, it should be
						 *u.u_error.  If there was an
						 *error in delivering the
						 *message, it should be an
						 *appropriate error code*/
				u_short dmt_eosys;	/*value of u.u_eosys*/
			} dmt_replystuff;
		} dmt_u_rr;
	} dm_transmit;
	/*the message itself*/
	char		dm_params[2];		/* arbitrary length message*/
};

#define dm_flags	dm_no_transmit.dmn_flags
#define dm_func		dm_no_transmit.dmn_func
#define dm_bufp		dm_no_transmit.dmn_bufp
#define dm_bufoffset	dm_no_transmit.dmn_bufoffset
#define dm_mid		dm_no_transmit.dmn_mid
#define dm_dest		dm_no_transmit.dmn_dest
#define dm_tflags	dm_transmit.dmt_tflags
#define dm_headerlen	dm_transmit.dmt_headerlen
#define dm_datalen	dm_transmit.dmt_datalen
#define dm_acflag	dm_transmit.dmt_acflag
#define dm_lan_card	dm_transmit.dmt_lan_card
#define dm_srcsite	dm_ph.p_srcsite
#define dm_op		dm_transmit.dmt_u_rr.dmt_requeststuff.dmt_op
#define dm_pid		dm_transmit.dmt_u_rr.dmt_requeststuff.dmt_pid
#define dm_rc		dm_transmit.dmt_u_rr.dmt_replystuff.dmt_rc
#define dm_eosys	dm_transmit.dmt_u_rr.dmt_replystuff.dmt_eosys

#define DM_CANNOT_DELIVER	EIO

#define DM_HEADERLENGTH (sizeof (struct dm_header) - sizeof (char [2]))


#define DUX_PROTOCOL	0x164f	/* DUX Canonical Address */
#define BUF_ADDR(dmp)  (dmp->dm_bufp->b_un.b_addr + dmp->dm_bufoffset)
#define MINIMUM2(a,b)  ((a<b) ? a : b)

/* exclude dm_params */
/*
 * maximum permitted dm_message (including headers).  Must be less than
 * or equal to maximum dux_mbuf size on ALL members of the cluster.
 */
#define DM_MAX_MESSAGE	MCLBYTES

/*
** Minimum request size possible to force allocation of a cluster
** in dm_alloc.	 Used by selftest code.
*/
#define DM_MIN_CLUSTER	(MLEN - DM_HEADERLENGTH + 1)

/*
 * special constants
 */
#define NO_RESPONSE	-1	/* a size of NO_RESPONSE indicates that
				 * no response is expected*/

/*
 * flags
 */
#define DM_SLEEP		0x01	/*sleep until response comes in*/
#define DM_DONE			0x02	/*network operation is compelte*/
#define DM_FUNC			0x04	/*call a function upon release*/
#define DM_REPEATABLE		0x08	/*operation can be safely repeated*/
#define DM_RELEASE_REQUEST	0x10	/*release request when op complete*/
#define DM_RELEASE_REPLY	0x20	/*release reply when op complete*/
#define DM_REQUEST_BUF		0x40	/*buffer associated with request*/
#define DM_REPLY_BUF		0x80	/*buffer associated with reply*/
/*
 * Yes DM_URGENT and DM_SOUSIG are the same, but I needed a flag in
 * the non-transmitted flags field (short).  Since DM_SOUSIG is only
 * used with the transmitted flags, I use the same bit.
 * Don't change the code to start setting DM_SOUSIG lightly.
 * DM_URGENT is used to cause a request to not go through the
 * protocol windows code (sets "counted" to be false in net_request()).
 * mls - 26-Feb-91
 */
#define DM_URGENT		0x100	/*don't count req in proto windows*/
#define DM_SOUSIG		0x100	/*SOUSIG flag on in proc area*/
#define DM_LONGJMP		0x200	/*do a longjmp upon return*/
#define DM_SIGPENDING		0x400	/*a signal is pending*/
#define DM_LIMITED_OK		0x800	/*limited nsp can process request*/
#define DM_KEEP_REQUEST_BUF	0x1000	/*don't release request buffer*/
#define DM_KEEP_REPLY_BUF	0x2000	/*don't release reply buffer*/
#define DM_DATAGRAM		0x4000	/*don't allocate reply*/
#define DM_INTERRUPTABLE	0x8000	/*request can be interrupted*/

#define DM_KEEP_BUF	DM_KEEP_REQUEST_BUF	/*for internal use only*/

/*
 * flags which are passed to send
 */
#define DM_HEADERMASK \
       (DM_SLEEP|DM_FUNC|DM_REPEATABLE|DM_RELEASE_REQUEST| \
	DM_RELEASE_REPLY|DM_REQUEST_BUF|DM_REPLY_BUF|DM_URGENT| \
	DM_LONGJMP|DM_KEEP_BUF|DM_DATAGRAM|DM_INTERRUPTABLE)

/*
 * useful macros
 */

#define DM_HEADER(dmp)	(mtod((struct mbuf *)(dmp),struct dm_header *))

#define DM_RETURN_CODE(dmp) \
	(DM_HEADER(dmp)->dm_rc)

#define DM_EOSYS(dmp) \
	(DM_HEADER(dmp)->dm_eosys)

#define DM_ACFLAG(dmp) \
	(DM_HEADER(dmp)->dm_acflag)

#define DM_TFLAGS(dmp) \
	(DM_HEADER(dmp)->dm_tflags)

#define DM_BUF(dmp) \
	(DM_HEADER(dmp)->dm_bufp)

#define DM_OP_CODE(dmp) \
	(DM_HEADER(dmp)->dm_op)

#define DM_SOURCE(dmp) \
	(DM_HEADER(dmp)->dm_srcsite)

#define DM_CONVERT(dmp,packettype) \
	((packettype *)((DM_HEADER(dmp))->dm_params))

/*
 * resize the dm message.  May only be used for shrinking.  No check
 * is made for incorrect usage
 */
#define DM_SHRINK(dmp,newsize) \
	(DM_HEADER(dmp)->dm_headerlen) = (newsize) + sizeof (struct dm_transmit)

/*
 *structure used with multisite requests
 */
struct dm_site		/*DUX MESSAGE STRUCTURE*/
{
	u_char site_is_valid;	/*true if site to receive message*/
	u_char status;		/*used by network		 */
	u_short rc;		/*return code			 */
	u_short eosys;		/*u.u_eosys for multisites	 */
	u_short acflag;		/*u.u_acflag for multisites	 */
	u_short tflags;		/*longjump for multisites      */
};

struct dm_multisite		/*DUX MESSAGE STRUCTURE*/
{
	int maxsite;		/*highest site id to receive message*/
	struct dm_site dm_sites[MAXSITE];
	/* The following fields are used by network only*/
	int remain_sites;	/*no of sites that have not sent reply back
				 *(does not include slow request back*/
	int no_response_sites;	/*no of sites that have not sent any response
				 *back (either slow request or reply)*/
	int next_dest;		/*next dest site in the list*/
};

/*
 * status code for multisite, clustercast request.
 * (used by network only)
 */
#define DM_MULTI_NORESP	 0x00	/* destination has not sent reply or ack to
				 * duplicate requests back (i.e. slow req)
				 */
#define DM_MULTI_SLOWOP	 0x01	/* destination has acknowleged request but
				 * has not sent reply back yet
				 */
#define DM_MULTI_DONE	 0x02	/* destination has sent reply back */

/*
 *special site ids for DUX cluster broadcast and multicast
 */
#define DM_MULTISITE	0xffff
#define DM_DUXCAST	0xfff0
#define DM_CLUSTERCAST	0xff00
#define DM_DIRECTEDCAST 0xf000	/* Only Used During Booting */

/*
 * The following macro is non zero if and only if the destination
 * is multisite
 */
#define DM_IS_MULTI(site) ((site)&0x0800)

/*
 * values for dm_how
 */
#define DM_UNUSED	0x0	/*unused entry*/
#define DM_KERNEL	0x1	/*process with a kernel NSP*/
#define DM_INTERRUPT	0x2	/*process under interrupt*/
#define DM_USER		0x3	/*process with a user NSP*/
#define DM_HOWMASK	0x3	/*mask for above values*/
#define DM_LIMITED	0x10	/*may be processed with a limited NSP*/

/*
 * We may want an additional field, dm_pri, which is the priority
 * with which should be applied to the network process.	 Thus some
 * requests (such as page fault handling) could be given higher
 * priorities, while other requests (such as copy propagation), could
 * be given lower priorities.  This number could also be temperred by
 * a modifyier sent in the packet itself, giving low priority processes
 * low priority messages, and high priority processes high priority
 * messages.
 */

/* useful constants */
#define WAIT 1
#define DONTWAIT 0
#define DM_EMPTY 0
#define MT_DUX	2

#define splmsg() spl6()

#define PROTO_LENGTH	((int)sizeof(struct proto_header))
#define XMIT_OFFSET sizeof(struct dm_no_transmit) + PROTO_LENGTH

/*
** Macro for to copy the proto_header using in-line code, rather
** than calling bcopy().  This is much faster, since the size of
** the proto_header structure is so small.
*/
#define DUX_COPY_PROTO(from, to) \
{ *(struct proto_header *)(to) = *(struct proto_header *)(from); }

/*
** The following is a method of keeping track of the number
** of particular types of DM requests that are received/requested. It
** is simply a set of counters that match of with the definitions in
** dmmsgtype.h.
**
** NOTE: Should really size the array to be "funcetrysize". but the
**	 compiler complains about "expecting a constant".
*/
struct proto_opcode_stats {
	u_int opcode_stats[100];
};

#ifdef _KERNEL
/************************************************************
** The following dm_XXX functions were originally defined in
** dm.c. However, in order to configure out portions of DUX
** it was deemed easier to turn the dux_XXX functions into
** macro calls rather than change the hundreds of calls to
** them throughout the DUX kernel. The macros in turn utilize
** the DUXCALL (indirect procedure call) which allows us to
** configure the entire dm/protocol layer out of the kernel.
** When DUX is configured in, the actual dm_XXX functions are
** called via the duxproc[] set of pointers to functions, else
** a nop is utilized.
*/

/*
 * Allocate a network packet which will hold a message of size nbytes.
 * To this size, is added the size of the standard headers.
 * A dm_message is actually an mbuf or an mcluster, depending on the
 * size of the message.	 Mbufs are used only for convenience; in the
 * future, this mechanism could be replaced, with no changes to the
 * overlying code.
 */
#define dm_alloc(nbytes,canwait)\
	(dm_message)DUXCALL(DM_ALLOC)(nbytes,canwait)

/*
 * Release a dm message.  If the release_buf parameter is set, also
 * release the associated buffer, unless, the DM_KEEP_BUF flag is set
 * in the DM message.
 * This is a bit of a kluge, but it allows us to make use of automatic
 * release code, for example when sending a reply, which normally will
 * release the buffer, but can be overridden.  (One place that this is
 * particularly useful is if the buffer used for the reply is the same
 * buffer that was sent with the request.
 */
#define dm_release(message,release_buf)\
	DUXCALL(DM_RELEASE)(message,release_buf)

/*
 * Send a request to a remote site.  The return value is a pointer to
 * the response packet (if any).  See the DM interface for a complete
 * definition of all the parameters.
 */
#define dm_send(request, flags, op, dest, resp_size, func, buf1, buf1size, buf1offset, buf2, buf2size, buf2offset)	\
	(dm_message)DUXCALL(DM_SEND)( request, flags, op,	\
				 dest, resp_size, func,		\
				 buf1, buf1size, buf1offset,	\
				 buf2, buf2size, buf2offset)

/*
 * Send a response to the current message being operated on.  For a
 * complete definition of the interface to this function see the DM
 * interface document.
 *
 * Note:
 *   The request to which we are replying is not passed with to this
 *   function.	Instead, it is gotten from either dm_int_dm_message or
 *   from u.u_request.	(Authors comment:  I don't wish to defend this
 *   decision. In retrospect, it seems like a poor choice.  However, it
 *   seemed like a good idea at the time, and everyone else agreed.
 *   While it could be changed now, that would be rather difficult,
 *   and besides, everything does work.)
 */
#define dm_reply(response,flags,return_code,resp_buf,resp_bufsize,resp_bufoffset)\
	DUXCALL(DM_REPLY)( response, flags, return_code,\
		    resp_buf, resp_bufsize, resp_bufoffset)

/*
 * A quick way to send a reply.	 Only the error code is sent back.
 */
#define dm_quick_reply(return_code)\
	DUXCALL(DM_QUICK_REPLY)(return_code)

/*
 * Send a followup signal to the nsp serving the specified message.
 * This is needed if the local process receives a signal.  There is a
 * potential race condition, since the signal message could arrive at
 * the serving site before the original message.  Thus, when the reply
 * comes back we check to see if the signal was already delivered.  If
 * it wasn't, and if the reply to the original message has not yet come
 * in, we resend the signal.
 */
#define senddmsig(message)\
	DUXCALL(SENDDMSIG)(message)

/*
 * Wait for a message to complete.
 * We originally sleep at an interruptable priority.  If an interrupt
 * comes in, we send a follow up signal message, and then sleep at a
 * non interruptable priority.	This prevents infinite looping.
 */
#define dm_wait(message)\
	DUXCALL(DM_WAIT)(message)

/*
 * Receive a reply to a message.  This function is called by the
 * network whenever a reply comes in.  It is responsible for waking
 * up sleepers, calling functions, and whaever else was specified in
 * the request.
 */
#define dm_recv_reply(request,response)\
	DUXCALL(DM_RECV_REPLY)(request, response)

/*
 * Receive an incoming request.	 The request should either be processed
 * under interrupt, given to an NSP, or given to a UNSP.  Determine how
 * to process it by looking up the opcode in the funcentry table.  The
 * function will panic if the request opcode is not in the funcentry
 * table.
 */
#define dm_recv_request(request)\
	DUXCALL(DM_RECV_REQUEST)(request)

/*
 * this macro is used for indirect calls from DUX to NFS.
 * usage: NFSCALL(func) (parm1, parm2, ...);  --gmf.
 */
extern int (*nfsproc[])();

#define NFSCALL(func) (*nfsproc[func])

#endif /* _KERNEL */

#define NFS_RFIND	0
#define NFS_MAKENFSNODE 1
#define NFS_FIND_MNT	2
#define NFS_ENTER_MNT	3
#define NFS_DELETE_MNT	4
#define NFS_INFORMLM	5   /* Notify Lock Manager a lock is free */
#define NFS_LMEXIT	6   /* NFS lock manager is exiting, clean up */

#ifdef _KERNEL
/*
 * The following are used to indicate if the serving dm function
 * is under interrupt.	If so, the message structure to operate
 * on under interrupt is "dm_int_dm_message".
 */
int dm_is_interrupt;
dm_message dm_int_dm_message;

/*
 * Array of functions to call for each request.
 */
extern struct dm_funcentry dm_functions[];

/*
 * Root site id
 */
extern site_t root_site;

/*
 * My site id
 */
extern site_t my_site;

extern struct buf *syncgeteblk();
#endif /* _KERNEL */

#endif /* _SYS_DM_INCLUDED */
