/*
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/net_diag.c,v $
 * $Revision: 1.8.83.5 $		$Author: rpc $
 * $State: Exp $		$Locker:  $
 * $Date: 93/11/11 13:06:32 $
 *
 *
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) $Header: net_diag.c,v 1.8.83.5 93/11/11 13:06:32 rpc Exp $";
#endif

/* BEGIN_IMS net_diag.c
*****************************************************************************
* net_diag.c
*****************************************************************************
* +----------------------------------------------------------+
* | (c) Copywrite Hewlett-Packard Company 1990. All rights   |
* | reserved. No part of this program may be photocopied,    |
* | reproduced or translated to another program language     |
* | without the prior written consent of Hewlett-Packard Co. |
* +----------------------------------------------------------+
* 
* Description:
*   This module contains the kernel library routines for tracing and logging.
*   Calls are made to ktrc_write() and klogg_write() via the function
*   ns_trace_link() or ns_log_event() which are also included in this file.
*
*   Interupt level calls have been placed around global parameters whenever
*   they are altered. The calls:
*   x = splimp()	 sets a new interupt level
*   splx(x)		 reset the interupt level to x.
*
* External Callable Routines:
*   diag_send_msg	 located in netdiag1.c
*   diag_recv_msg	 located in netdiag1.c
* 
* To Do List:
*   Update from code review. 
* Notes:
* Module size:
*  
*****************************************************************************
* END_IMS net_diag.c */

#define SUCCESS	      0

#include "../h/ioctl.h"
#include "../h/param.h"
#include "../h/uio.h"
#ifdef hp9000s800
#include "../h/dir.h"
#endif hp9000s800
#include "../h/user.h"
#include "../h/socket.h"
#include "../net/if.h"
#include "../h/types.h"
#include "../h/proc.h"
#include "../h/mbuf.h"
#include "../h/subsys_id.h"
#ifdef __hp9000s800
#include "../sio/llio.h"
#endif
#define NS_LOG_STRINGS
#include "../h/netdiag1.h"
#include "../h/net_diag.h"
#undef NS_LOG_STRINGS
#include "../h/time.h"
#include "../h/errno.h"
#include "../h/mp.h"

#define MBUFS	     -1
#define DF_TRACE_LEN -2

#ifndef MCLBYTES
#define MCLBYTES NETCLBYTES
#endif

int netdiag_tr_map[MAX_SUBSYS];
int netdiag_log_map[MAX_SUBSYS];
netdiag_subsys_t netdiag_masks[MAX_SUBSYS];


/* log instance, C means from the kernel and from a driver */
#ifdef NEVER_CALLED
unsigned short ktl_log_inst = 0xc000;
#endif /* NEVER_CALLED */
int	  net_log_on;
int	  net_trace_on;

extern void netdiag_send_msg();
extern que_elements_t netdiag_ques[]; /* for macros nettrace_ques and netlog_ques */
int trace_len = DF_TRACE_LEN;	/* determines how many bytes to trace */

struct mbuf *iov_to_mbuf();
extern struct mbuf *m_copy();  

/* extracted from the  ns_diag.c file 
 *
extern int nsdiag0_total_msgs;

int ns_log_mask = 0xffffffff;
int ns_console_log_mask = 0xffffffff;
int ns_log_subsys_mask = -1;
int ns_log_take_action = NS_ACT_NOACTION;
int ns_log_action_event = -1;
int ns_log_action_class = -1;
int ns_log_action_subsys = -1;
 *
 */
int ns_logtemp_event;
int ns_logtemp_subsys;
struct timeval ms_gettimeofday();

/* BEGIN_LOG_ES */
/* BEGIN_IMS ns_log_event *
 ********************************************************************
 ****
 ****	ns_log_event(event, class, subsys, log_type, data, count, 
 ****						     a, b, c, d, e)
 ****
 ********************************************************************
 * Input Parameters
 *	event			event being logged (NS_LE_xxx)
 *	class			event class (NS_LC_xxx)
 *	subsys			subsystem executing the log (WHAT DEFINE)
 * Output Parameters
 *	none
 * Return Value
 *	none
 * Globals Referenced
 *	ns_log_mask		logging mask of events to log.
 *
 * Description
 *	This procedure formats a log record and submits the
 *	record to the pseudo driver log facility.  This is only if the
 *	captured message passes a series of subsystem, and log class 
 *	filters to make sure it is enabled.
 *
 * Algorithm
 *
 *	o Format ns_log_rec:
 *
 *	o if passes the prefilting, then pass to netdiag_send_msgs to
 *	  be queued.
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	2/28/86 gr	created
 *	6/15/88 Rohit	added total number of messages logged and 
 *			other fixes.
 *	11/2/89 K.Vandiver 
 *			Changes for 8.0 common tracing and logging tool.
 *			o no longer filter on event
 *			o removed check on ins_log_isset and am using KTL_CK
 *			  instead.
 *			o removed console logging since that is now done in
 *			  user space.
 *			o made 'logevent' point to the data portion of an mbuf
 *			  so mbufs are passed in to klogg_write.
 *			o put data in an mbuf and concat'd to logevent in case
 *			  of logtype = NS_LF_STR, and send an mbuf chain to KTL
 *			o send to 'klog_write()' at end instead of 'send_msg'.
 *			o code complete for common tracing and logging
 *			o update after code review
 * 
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS ns_log_event */

void
ns_log_event(event, class, subsys, log_type, data, count, a, b, c, d, e)
	int		event;
	int		class;
	int		subsys;
	int		log_type;
	char		*data;
	int		count;
	int		a, b, c, d, e;
{
	nsdiag_event_msg_type	*logevent; /* made into a ptr for an mbuf */
	struct timeval		time;
	struct mbuf		*m, *hdr_m;
	int			pid;
	int			device_id=0;
	unsigned short		log_inst=0;  /* not used for 8.0 release */
	int			s;    /* for setting interupt levels */

	if (!(KTL_RANGE(subsys))) return;

	/* 
	 * check to see of we are filtering out this class and subsys
	 */

	/* retain the old NS classes, but still check the filtering of them*/
	/* if the class sent if equivelent to the class wanted , log the   */
	/* event. */ 
	switch (class) {
	 case NS_LC_DISASTER : if (DISASTER & netdiag_masks[subsys].log.class)
				    break;
			       else
				    return;
	 case NS_LC_ERROR    : if (ERROR & netdiag_masks[subsys].log.class)
				    break;
			       else
				    return;
	 case NS_LC_RESOURCELIM:
	 case NS_LC_WARNING  : if (WARNING & netdiag_masks[subsys].log.class)
				    break;
			       else
				    return;
	 case NS_LC_PROLOG   : if (INFORMATIVE & netdiag_masks[subsys].log.class)
				    break;
			       else
				    return;
	 default:   return;
       }
 
	time =	ms_gettimeofday(); 

	/* check and see if we are on the ICS if so, retun -1 */
	/* else return proc pid of caller		      */
	pid = (ON_ICS) ? -1 : u.u_procp->p_pid;

	/*
	 * format log record, but lets put it into an mbuf  
	 */
	if ((hdr_m = m_get(M_DONTWAIT, MT_DATA)) == 0) {
	   s = splimp();
	   netlog_que.que_msgs_dropped++;
	   splx(s);
	   return;
	}  
	/* Set the length in the mbuf hdr to be the 
	 * size of the first structure in nsdiag_event_msg_type.
	 * Update the length later as more info is put in the mbuf.
	 */
	hdr_m->m_len = sizeof(logevent->log);	/* 22-Nov-91 TM */

	/* cast the mbuf data area so we can fill it with a header */
	logevent = mtod(hdr_m, nsdiag_event_msg_type*); 

	logevent->log.log_event		= event;
	logevent->log.log_time.tv_sec	= time.tv_sec;
	logevent->log.log_time.tv_usec	= time.tv_usec;
	logevent->log.log_class		= class;
	logevent->log.log_subsys	= subsys;
	logevent->log.log_location	= 0;
	logevent->log.log_error		= a;
	logevent->log.log_context	= b;
/*	logevent->log.log_mask		= ns_log_mask;	*/
	logevent->log.log_mask		= -1;
/*	logevent->log.log_console_mask	= ns_console_log_mask; */
	logevent->log.log_console_mask	= 0xffffffff;
	logevent->log.log_flags		= log_type;
	logevent->log.log_pid		= pid;
	logevent->log.log_dropped	= 0;
	logevent->log.log_data_dropped	= 0;

	/*
	 * get associated log data
	 */
	if (log_type == NS_LF_STR) 
		count = ns_strlen(data);
	else if (log_type == NS_LF_INFO) {
		logevent->data.words[0] = a;
		logevent->data.words[1] = b;
		logevent->data.words[2] = c;
		logevent->data.words[3] = d;
		logevent->data.words[4] = e;
		/* 
		 * count is the number of info words
		 */
		count *= sizeof(int);
		/* also update the size in the mbuf */
		hdr_m->m_len += sizeof(int) *5;
	} else if (log_type == NS_LF_NODATA)
		count = 0;
		
	/* if count is bigger than MLEN, set it to MLEN and only copy that*/
	/* many bytes, else don't worry about it  */
	if (count > MLEN) count = MLEN; 
	logevent->log.log_dlen = count;

	/*
	 * if there is no associated data or the data has already been
	 * placed in the logevent structure as in the info case, then
	 * go right ahead and send the log message
	 */
	if (count == 0 || log_type == NS_LF_INFO)
		goto send;

	/* else see if there is room left in the header mbuf structure */
	if ((log_type == NS_LF_STR && (count <= (MLEN - hdr_m->m_len))) ||
	    (log_type == NS_LF_DATA && (count <= (MLEN - hdr_m->m_len)))) {
		/*
		 * there is room within the diag mbuf for the associated
		 * data.
		 */
		bcopy(data, logevent->data.chars, count);
		hdr_m->m_len += count;
	} else {
		/* 
		 * need to get mbuf for the data.  
		 */
		m = m_get(M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			bcopy(data, logevent->data.chars, NSDIAG_MAX_DATA);
			logevent->log.log_data_dropped = count-NSDIAG_MAX_DATA;
			logevent->log.log_flags |= NS_LF_DATA_DROPPED;
			logevent->log.log_dlen = NSDIAG_MAX_DATA;
			hdr_m->m_len += NSDIAG_MAX_DATA;
		} else {
			/* This is some leftover 7.0 style. It should be */
			/* changed so data is accessed without data.ptr */ 
			logevent->data.ptr = mtod(m, caddr_t);
			if (count < MLEN) {
			   bcopy(data, logevent->data.ptr, count);
			   m->m_len = count;		
			}
			else {
			   bcopy(data,logevent->data.ptr, MLEN);
			   m->m_len = MLEN;
			}
			logevent->log.log_flags |= NS_LF_DATA_INMBUF;
		/* concatenate the mbufs before sending them to klogg_write */
		m_cat(hdr_m, m);
		}
	}

send:
	/*
	 * Send the message to the KTL diagnostics driver. 
	 */
	 klogg_write(subsys, class, device_id, log_inst, (caddr_t)hdr_m, -1);
	 m_freem(hdr_m);

}  /* end of ns_log_event() */



/* BEGIN_TRACE_ES */
/* BEGIN_IMS ns_trace_link *
 ********************************************************************
 ****
 ****		ns_trace_link( ifp, event, m,subsys_id )
 ****
 ********************************************************************
 * Input Parameters
 *	ifp			ifnet structure indicating which interface
 *				is being traced.
 *	event			link event being traced (TR_LINK__xxx)
 *	m			mbuf pointer for data to be traced
 *
 * Output Parameters	
 *	none
 * Return Value
 *	none
 * Globals Referenced
 *	ns_trace_maxlen		The maximum length of data to be traced
 *	ns_trace_filter		Filter for receive or transmition packets
 *
 * Description
 *	This procedure formats a link trace record and submits the
 *	record to the link tracing facility.
 *
 * Algorithm
 *	o if link tracing is not enabled for the specified interface
 *		return;
 *
 *	o Format ns_trace_link_hdr:
 *		tr_event = event
 *		tr_time = current timeval
 *		if (ns_trace_maxlen) 
 *			tr_traced_len = MIN(data_len, ns_trace_maxlen)
 *		else	tr_traced_len = data_len
 *		tr_pkt_len = data_len
 *
 *	o copy the mbuf chain into my own data space
 *	o send mbuf trace message to the pseudo driver netdiag_send_msg()
 *
 * To Do List
 *
 * Notes
 *
 * Modification History
 *	2/24/86 gr	created
 *	8.0	K.Vandiver  Updated and modified for 8.0 release
 *		o added a subsys parameter to the call since can no longer
 *		  tell which subsys is calling due to ifnet changes
 *		o Leave everything in mbufs, and pre-pended header information
 *		  to it. 
 *		o sent off the ktrc_write instead of measurement system.
 *	6/90	K. Vandiver
 *		o code review updates
 *
 * External Calls
 *
 * Called By (optional)
 *
 ********************************************************************
 * END_IMS ns_trace_link */

int	ns_linktr_filter = 0;
int	ns_linktr_max_size = 0;
int	ns_linktr_ms_id = -2;
int	ns_linktr_buffer_wsize = 0;
int	ns_linktr_pid = 0;
#define NS_MAXIOV	25

int trace_debug = 0;


ns_trace_link(ifp, event, m, subsys)
	struct ifnet	*ifp;
	int		event;
	struct mbuf	*m;
	int		subsys;
{
	struct ns_trace_link_hdr	*trace; /* this will point to an mbuf*/
	struct timeval			time;
	int		data_len=0;		/* bytes in mbuf string */
	int		s, if_kind= 0;		/* holds the iftype/kind */
	struct mbuf	*tr_hdr;

	/* if Lan is trying to trace on a DUX cluster, we want to */
	/* drop it so we don't have a runaway trace problem	  */
	/* change this to allow inbound DUX packets.		  */
#ifdef hp9000s800
	if ((M_HASCL(m)) && (m->m_cltype == MCL_DUX) && (m->m_sid))
		return(0);
#endif

	/*
	 * If the specified subsys is not being traced, simply return
	 */
	if (subsys < 0 || subsys > NS_TRACE_MAX) {
		return(0);
	}
	if (! (net_trace_on && netdiag_tr_map[subsys]) ) {
		return(0);
	}


	if (event == TR_LINK_INBOUND)
		if_kind = PDU_IN_BIT;
	else if (event == TR_LINK_OUTBOUND)
		if_kind = PDU_OUT_BIT;
	else if (event == TR_LINK_LOOP)
		if_kind = LOOP_BACK_BIT;
	else {
		if (trace_debug)
			printf("Unmappable trace event\n");
		return(0);
	}
	
	/* check to make sure the LU is correct */ 
	if (!(netdiag_masks[subsys].trace.kind & if_kind)){
		if (trace_debug)
			printf("Kind %x filtered out\n", if_kind);
		return(0);
	}


	/* check for the device id/lu value to be less than 7, for */
	/* the subsystem being traced.				   */
	if (!(ifp->if_unit > 7))
	       if (!((1 << ifp->if_unit) & 
			netdiag_masks[subsys].trace.device_id)) {
			if (trace_debug) 
				printf("device id is bad\n");
			return (0);
		}


	/* Now we get an mbuf and put the old NS header into it.  The */
	/* header is then concatenated to the front of the mbuf that  */
	/* is to be traced.  Then, as one mbuf chain, it is sent off  */
	/* to ktrc_write().					      */

	/* get an mbuf for the header in 'trace' */
	if ((tr_hdr = m_get(M_DONTWAIT, MT_DATA )) == 0) { 
		s=splimp();
		nettrace_que.que_msgs_dropped++;
		splx(s);
		if (trace_debug)
			printf("m_get of header failed\n");
		return(0);
	}
	/* set the size */
	tr_hdr->m_len = sizeof(struct ns_trace_link_hdr);

	/* cast the data area to equal ns_trace_link_hdr */
	trace = mtod(tr_hdr, struct ns_trace_link_hdr* );

	 /* mask out low order bits that contain the device_id */
	 /* and store in the low byte of tr_if */
	trace->tr_if = (ifp->if_unit) & TR_IF_UNITMASK;

	/* Now check for NI,LAN, and LOOPBACK subsystems and store */
	/* in the high byte of trace.tr_if; next map the event type*/
	/* to if_kind so KTL gets a trace kind to filter	   */
	if (subsys == NS_LS_NI)
		trace->tr_if |= TR_IF_NI;
	else if (subsys == NS_LS_LOOPBACK)
		trace->tr_if |= TR_IF_LOOP;
	else
		trace->tr_if |= TR_IF_LAN;

	trace->tr_subsys = subsys;
	trace->tr_event = event;
	time = ms_gettimeofday(); 
	trace->tr_time.tv_sec = time.tv_sec;
	trace->tr_time.tv_usec = time.tv_usec;

	
	 /* note amount of data actually traced, If the user chooses to*/
	 /* use the -m option through nettl, then we only trace the    */
	 /* number of bytes they specify. Else, we will trace the whole*/
	 /* packet. The -2, means the ktl_daemon set it to trace the   */
	 /* whole packet. In net_set_masks, all other ioctl calls that */
	 /* come from t/l itself, send in -1, which I ignore.	       */

	/* Note:  The trace_len stuff was moved to ktrc_write.  But I  */
	/* left this assignment here so that the formatter will work   */
	/* properly for the NS subsystems.  2-Mar-92 TM                */

	data_len = m_len(m);
	trace->tr_traced_len = (trace_len > 0 ? MIN(trace_len, data_len) : data_len);
	trace->tr_pkt_len = data_len;


	/*
	 * If on the ICS (tell by looking at the address of a local var)
	 * pid should be -1, otherwise user pid
	 */
	 trace->tr_pid = (ON_ICS) ? -1 : u.u_procp->p_pid;

	/* point the header to the traced mbuf */
	tr_hdr->m_next =  m;	

	/* execute the trace */
	ktrc_write(subsys,
		   if_kind, 
		   -1, 
		   (ifp->if_unit) & TR_IF_UNITMASK,
		   (caddr_t)tr_hdr, 
		   -1);
	
	/* free the header mbuf */
	m_free(tr_hdr);

	return(0);

}
/* END_TRACE_ES */





/* BEGIN_IMS  ktrc_write  *
**************************************************************************
****
****	int ktrc_write(subsys_id, kind, path_id, device_id, tl_packet,
****			 tl_packet_cnt)
****
***************************************************************************
*
* Input Parameters:
*		 short		  subsys_id;
*		 int		  kind;
*		 int		  path_id;
*		 int		  device_id;
*		 caddr_t	  *tl_packet;
*		 int		  tl_packet_cnt;
*
* Output Parameters:
* Return Value:
* Globals Referenced:
*		 netdiag_masks;	 holds all the filtering information 
*		 net_trace_on;	 holds tracing information
*		 netdiag_tr_map; holds subsystem information for tracing
*
* Description
*      To accept trace messages from the kernel architectures, check against
*      filters and send to the psuedo driver if the filters pass. 
*		 
* Algorithm:
*
*      - the trace message that came in has already been checked for the
*	 subsystem and trace kind.
*      - check the remaining filters, mainly per path 
*      - if all filters pass  
*	 then get a kernel buffer and put the KTL header in it followed by
*	 the trace or log data. 
*      - send to the driver ques with netdiag_send_msg().
*
* To Do List
* Notes
* Modification History
*   9/14/88   K.Cirimele   initial revision
*   2/1/89    K.Cirimele   updates from code review:
*	      - ktr_write returns an int and not a void
*	      - removed all exit(0)'s and replaced them with return(0)
*	      - used splx() around global variables when updating them.
*	      - moved mbuf_pkt_head=mbuf_pkt to after the m_cat call
*   10/13/89  K.Vandiver updates for 8.0
*	      - ktr_write called ktrc_write
*	      - made changes for multiple subsystems and not just three 
*    11/22/89  K.Vandiver
*	       -  no longer use ktl_copy, went back to m_copy.
*	       -  removed reference to MF_CLUSTER in m->m_flag, now use
*		  m->m_cltype and look for MCL_NORMAL.
*	       - use MCLBYTES instead of NETCLBYTES for 2048 in cluster 
*    6/11/90   K.Vandiver 
*	       - use of kernel buffers instead of mbufs. This will
*		 also reduce the code and increase efficiency.
*	       - Note the use of MALLOC. 
*		
*
* External Calls
*   netdiag_send_msg()
*   MALLOC()
*   m_copydata()
*
* Called by
*   network kernel modules:
*   
*   
*************************************************************************
* END_IMS ktrc_write   */

int ktrc_write(subsys_id, kind, path_id, device_id, tl_packet, tl_packet_cnt)
short	 subsys_id;
int	 kind;
int	 path_id;
int	 device_id;
caddr_t	 tl_packet;
int	 tl_packet_cnt;
{
 
   caddr_t	     bpoff;		/* offset into the kernel buffer    */
   caddr_t	     bp;		/* pointer to kernel buffer	    */
   struct mbuf	     *mbuf_tlpkt;
   ktl_msg_hdr_type  *hdr;		/* cast for ktl header info into bp */
   struct iovec*     tl_iovecs;		/* cast for tl_pkt if iovecs sent   */
   struct ktl_buf_t  *bp_buf;		/* the Malloc'd data is cast to this*/
   int		     i;
   int		     packet_len = 0;	/* len of packet data passed in     */
   int		     traced_len = 0;	/* len of packet data passed out    */
   int		     count;
   short	     iov;		/* flag for iovec or mbuf	    */

   if (!(KTL_RANGE(subsys_id))) return (0);

  /* make sure the tl_packet_cnt is valid */
  if ((tl_packet_cnt != MBUFS) && (!(tl_packet_cnt > 0)) )
	 return(0);

  /* check to see what we were sent in, iovecs or mbufs, Then be prepared */
  /* to create a big kernel buffer to netdiag_send_msg().		  */
  if (tl_packet_cnt == MBUFS) {
      iov = FALSE;
      mbuf_tlpkt = (struct mbuf*) tl_packet;
      packet_len = m_len(mbuf_tlpkt);
  }
  else {   
      iov = TRUE; 
      tl_iovecs = (struct iovec*) tl_packet;
      for (i = 0; i < tl_packet_cnt; i++) {
	  packet_len += tl_iovecs[i].iov_len;
      }
  }
  /*----------------------------------------------------------------------*/
  /* note amount of data actually traced, If the user chooses to use      */
  /* the -m option through nettl, then we only trace the number of        */
  /* bytes they specify. Else, we will trace the whole packet. The -2,    */
  /* means the ktl_daemon set it to trace the whole packet. In            */
  /* net_set_masks, all other ioctl calls that come from t/l itself,      */
  /* send in -1, which I ignore.                                          */
  /*----------------------------------------------------------------------*/
   if (trace_len > 0)
   {
	   if ((subsys_id >= 0) && (subsys_id <= NS_TRACE_MAX))
	   {
		   /* If this is an NS subsystem then packet_len 
		    * includes the size of ns_trace_link_hdr.  
		    * Adjust the traced_len accrodingly.
		    */
		   traced_len = MIN(trace_len + sizeof(struct ns_trace_link_hdr), packet_len);
	   }
	   else
	   {
		   traced_len = MIN(trace_len, packet_len);
	   }
	   
   }
   else
   {
	   traced_len = packet_len;
   }

  /*----------------------------------------------------------------------*/
  /* We malloc, cast to ktl_buf_t so we can store the lngth in the struct.*/
  /* Cast again to ktl_hdr_msg_type info at the 'base' and fill in the ktl*/
  /* header info. Then, bcopy in the mbuf or iovec.			  */
  /*----------------------------------------------------------------------*/
  /*
   *	Changed MALLOC call to kmalloc to save space. When
   *	MALLOC is called with a variable size, the text is
   *	large. When size is a constant, text is smaller due to
   *	optimization by the compiler. (RPC, 11/11/93)
   */
  bp = (caddr_t) kmalloc(traced_len + sizeof(ktl_msg_hdr_type) + sizeof(int),
			 M_DYNAMIC, M_NOWAIT);

  if (bp == NULL ) {
#ifdef QAON
      printf("Failed to get memory for ktrc_write() \n");
#endif
      return(0);
  }


  /* cast the bp to iovecs so we can add a length to the buffer */
  bp_buf = (struct ktl_buf_t*)bp;
  bp_buf->len = traced_len + sizeof(ktl_msg_hdr_type);

  /* mask the top of the buffer to ktl_msg_hdr_type and put in KTL hdr info */
  hdr			= (ktl_msg_hdr_type*) &bp_buf->base; 
  hdr->subsys_id	= subsys_id;
  hdr->kind		= kind;
  hdr->class		= 0;
  hdr->log_instance	= 0;  /* nothing for tracing */
  /*
   * If on the ICS (tell by looking at the address of a local var)
   * uid should be -1, otherwise use effective uid
   * pid should be -1, otherwise use pid
   */
  hdr->uid		= (ON_ICS) ? -1 : u.u_procp->p_suid;
  hdr->pid		= (ON_ICS) ? -1 : u.u_procp->p_pid;    
  hdr->dev_id		= device_id; 
  hdr->cm_path_id	= path_id;
  hdr->packetlen	= packet_len;
  hdr->tracedlen	= traced_len;
  hdr->time		= ms_gettimeofday();
   
   /* find offset into bp so we can copy trace data there  */
   bpoff = (caddr_t)&bp_buf->base + sizeof(*hdr);
   if (iov ) {
	   for (i = 0; i < tl_packet_cnt && traced_len > 0; i++) 
	   {
		   count = MIN(tl_iovecs[i].iov_len, traced_len);
		   bcopy(tl_iovecs[i].iov_base, bpoff, count);
		   bpoff      += count;
		   traced_len -= count;
	   }
   }
   else { 
	   while (traced_len > 0) 
	   {
		   count = MIN(mbuf_tlpkt->m_len, traced_len);
		   bcopy(mtod(mbuf_tlpkt, caddr_t), bpoff, count);
		   traced_len -= count;
		   bpoff      += count;
		   mbuf_tlpkt = mbuf_tlpkt->m_next;
	   }	 
   }

  (void)netdiag_send_msg (NET_TRACE, bp);
   return(0);

} /* end of ktr_write() */





/* BEGIN_IMS klogg_write   *
***********************************************************************
****
**** int klogg_write (subsys_id, class, device_id, log_instance,
****		       tl_packet, tl_packet_cnt)
****
***********************************************************************
*
* Input Parameters:
*		short		 subsys_id;
*		int		 class;
*		int		 device_id;
*		unsigned short	 log_instance;
*		caddr_t		 *tl_packet;
*		int		 tl_packet_cnt;
*
* Output Parameters:
* Return Value:
* Globals Referenced:
*		 netdiag_log_on;
*
* Function:
*      To accept log messages from the kernel architectures, check against
*      filters and send to the psuedo driver if the filters pass. 
*		 
* Algorithm:
*      - this packet has already passed filtering in the OSI modules
*      - get a kernel buffer and copy in the KTL header followed by the 
*	 log data. 
*      - send the packet to the psuedo driver with netdiag_send_msg(). 
*
* To Do List
* Notes
* Modification History
*  9/14/88  K.Cirimele	 initial revision
*  2/1/89   K.Cirimele	 updates from code reveiw:
*	    - put splx() around updates of global parameters
*	    - moved mbuf_pkt_head=mbuf_hdr to after m_cat call
*  10/13/89 K.Vandiver
*	    Update for 8.0 migration
*  6/11/90  K.Vandiver 
*	    - changed from mbuf dependency to kernel buffers. This
*	      will make it easier to read and more efficient.
*	    - Note the use of MALLOC 
*
* External Calls
*  netdiag_send_msg()
*  MALLOC()
*  m_copydata()
*  
* Called By
*  Network kernel modules
*
**************************************************************************
* END_IMS klogg_write  */

int klogg_write(subsys_id, class, device_id, log_instance, 
		tl_packet, tl_packet_cnt)
short	 subsys_id;
int	 class;
int	 device_id;
short	 log_instance;
caddr_t	 tl_packet;
int	 tl_packet_cnt;
{
   caddr_t	     bp;		  /* pointer to kernel buffer */
   caddr_t	     bpoff;		  /* offset into the kernel buffer */
   struct mbuf	     *mbuf_tlpkt;
   ktl_msg_hdr_type  *hdr;		  
   struct ktl_buf_t  *bp_buf;
   struct iovec	     *tl_iovecs;
   int		     i,len = 0;
   short	     iov;		  /* flag for iovec or mbuf  */
   register int	     s;
   int		     count;

   if (!(KTL_RANGE(subsys_id))) return (0);

  /* make sure the tl_packet_cnt is valid */
    if ((tl_packet_cnt != MBUFS) && (!(tl_packet_cnt > 0)) )
	 return(0);

  /* check to see what we were sent in, iovecs or mbufs. Then be   */
  /* prepared to create a big kernel buffer to netdiag_send_msg(). */ 
  if (tl_packet_cnt == MBUFS) {
      iov = FALSE;
      mbuf_tlpkt = (struct mbuf*) tl_packet;
      len = m_len(mbuf_tlpkt);
  }
  else {   
      iov = TRUE; 
      tl_iovecs = (struct iovec*) tl_packet;
      for (i = 0; i < tl_packet_cnt; i++) {
	  len += tl_iovecs[i].iov_len;
      }
  }

  /*----------------------------------------------------------------------*/
  /* We malloc, cast to ktl_buf_t so we can store the lngth in the struct.*/
  /* Cast again to ktl_hdr_msg_type info at the 'base' and fill in the ktl*/
  /* header info. Then, bcopy in the mbuf or iovec.			  */
  /*----------------------------------------------------------------------*/
  MALLOC(bp, caddr_t, len + sizeof(ktl_msg_hdr_type) + sizeof(int), M_DYNAMIC,
	 M_NOWAIT);
  if (bp == NULL ) {
#ifdef QAON
      printf("Failed to get memory for klogg_write() \n");
#endif
      return(0);
  }

  /* cast to ktl_buf_t and put in length and data  */
  bp_buf= (struct ktl_buf_t*) bp;
  bp_buf->len = len + sizeof(ktl_msg_hdr_type);
 
  /* mask  the top of the buffer to ktl_msg_hdr_type and put in KTL hdr info */
  hdr = (ktl_msg_hdr_type*) &bp_buf->base;
  hdr->subsys_id     = subsys_id;
  hdr->kind	     = 0;
  hdr->class	     = class;
  hdr->log_instance  = log_instance;  
  /*
   * If on the ICS (tell by looking at the address of a local var)
   * uid should be -1, otherwise use effective uid
   * pid should be -1, otherwise use pid
   */
  hdr->uid	     = (ON_ICS) ? -1 : u.u_procp->p_suid;
  hdr->pid	     = (ON_ICS) ? -1 : u.u_procp->p_pid;
  hdr->dev_id	     = device_id;
  hdr->cm_path_id    = 0;
  hdr->packetlen     = len;
  hdr->tracedlen     = len;
  hdr->time	     = ms_gettimeofday();

  /* find offset into bp so we can copy trace data there */
  bpoff = (caddr_t) &bp_buf->base + sizeof(*hdr); 
  if (iov ) {
      for (i = 0; i < tl_packet_cnt; i++) {
	  bcopy(tl_iovecs[i].iov_base, bpoff, tl_iovecs[i].iov_len);
	  bpoff += tl_iovecs[i].iov_len;
      }
  }
  else { 
      while (len > 0) {
	   count = MIN(mbuf_tlpkt->m_len, len);
	   bcopy(mtod(mbuf_tlpkt,caddr_t), bpoff, count);
	   len	 -= count;
	   bpoff += count;
	   mbuf_tlpkt = mbuf_tlpkt->m_next;
       } 
  }

   (void)netdiag_send_msg (NET_LOG, bp);
   return(0);

} /* end of klogg_write() */
 


/* BEGIN_IMS net_set_masks   *
**************************************************************************
**** 
****	    int net_set_masks(dev, arg)
****
**************************************************************************
*
* Input Parameters:
*		 int		   dev;
*		 dm_mask_t	   *arg;  
*
* Output Parameters:
* Return Value:
*		 0:   no error
*		-1:   bad subsys
*
* Globals Referenced:
* Description:
*     To set the filters wanted for the subsystem given.  This is for both
*     tracing and logging.
*
* Algorithm:
*     - set the global data structures to the filter values passed in.
*
* To Do List
* Notes
* Modification History
*   9/14/88  K.Cirimele	   initial revision
*   2/1/89   K.Cirimele	   Updates from code review:
*	     - return(EINVAL) if bad subsytem.
*
* External Calls
*   
* Called by
*   netdiag0
**************************************************************************
* END_IMS net_set_masks	  */
 
int net_set_masks(dev, arg)
int	   dev;
dm_mask_t  *arg;
{

   netdiag_l_filters  *subptr_l;
   netdiag_t_filters  *subptr_t;

   if (!(KTL_RANGE(arg->subsys_id))) return (EINVAL);

   if (dev == NET_TRACE) {
       subptr_t = &netdiag_masks[arg->subsys_id].trace; 
       /* call a Macro to turn on the subsystem */
       TRC_SUBSYS_MAP(arg->subsys_id, NETDIAG_ON);
       subptr_t->kind  = arg->kind;
       subptr_t->device_id = arg->device_id;

       /* if t_len == -2 then the daemon wants a default of all bytes to be   */
       /* traced. If the t_len is -1, then the t/l is calling so don't change */
       /* anything. If the t_len is > 0 then set trace_len to that value      */
       /* This is solely to ignore a -1 or 0  from the t/l.		      */
       /* And this with arg->t_len < MAX_T_LEN == 2000			      */
       if (((arg->t_len == DF_TRACE_LEN ) || (arg->t_len >0))&& 
	    (arg->t_len < MAX_T_LEN))
	    trace_len = arg->t_len;

       return(0); 
   }  /* if dev is trace */


   else	  {
      subptr_l = &netdiag_masks[arg->subsys_id].log;
      /* turn on the subsystem for logging */
      LOGG_SUBSYS_MAP(arg->subsys_id,NETDIAG_ON)
      /* set up log class mask */
      subptr_l->class = arg->class;
      return(0);

    } /* end of if dev is logging */
}   /* end of net_set_masks */




/* BEGIN_IMS net_reset_masks   *
**************************************************************************
**** 
****	int net_reset_masks(dev, arg, status)
****
**************************************************************************
*
* Input Parameters:
*		 int	    dev;
*		 rs_masks   *arg;
*
* Output Parameters:
*		 int   *status;
*
* Return Value:
* Globals Referenced:
*		 int	     netdiag_trace_on
*		 int	     netdiag_log_on
*		 netdiagfilters	 netdiag_mask
*
* Description
*    To reset specified filters to default values.
*
* Algorithm:
*
*   - determine if this is for tracing or logging and what subsystem
*     dev is from the host T/L facilities.
*   - then send off to a function to reset the filters, osidiag_reset_log
*     for logging and osidiag_reset_tr() for tracing. 
*
*   int osidiag_reset_log
*   *********************
*
*      check to see if the reset bits are set then set that parameter to
*      default. 
*	    
*   int osidiag_reset_tr 
*   ********************
*
*      check to see if the reset bits are set then set that parameter
*      to default.
*
* To Do List
* Notes
* Modification History
*   9/14/88   K.Cirimele   initial revision
*   2/1/89    K.Cirimele   Changes from code review:
*	      - use return(EINVAL) instead of return(-1) on bad subsystem is
*		this way netdiag0 doesn't have to look for it.

*
* External Calls
* Called by(optional)
*  netdiag0
***************************************************************************
* END_IMS net_reset_masks   */	

int net_reset_masks(dev,arg)
int	  dev;
rs_masks  *arg;
{ 

   netdiag_l_filters   *subptr_l;
   netdiag_t_filters   *subptr_t;


   if (KTL_RANGE(arg->subsys_id))  {
	 if (dev == NET_TRACE ) {
	     subptr_t = &netdiag_masks[arg->subsys_id].trace;
	     netdiag_reset_tr(subptr_t, arg);
	     return(0);
	 }
	  
	 if (dev == NET_LOG)  {
	     subptr_l = &netdiag_masks[arg->subsys_id].log;
	     netdiag_reset_log(subptr_l, arg);	
	     return(0);
	 }
     }
     else
	return (EINVAL);

} /* end of net_reset_masks() */
	   

/***************************************************************************
*  netdiag_reset_log(sub_pr, arg);
***************************************************************************/

int netdiag_reset_log (sub_ptr, arg)
netdiag_l_filters     *sub_ptr; 
rs_masks	      *arg;
{

   /* check to see if the class bit is set */
   if (arg->rs_filters & RS_CLASS) { 
      /* set the class filter to default */
      sub_ptr->class = DF_CLASS;
   }

   /* check to see if the subsystem bit is set, turn off logging for subsys */
   if (arg->rs_filters & RS_SUBSYS) {
      LOGG_SUBSYS_MAP(arg->subsys_id,NETDIAG_OFF)
   }
   return(0);

}  /* end of netdiag_reset_log() */


/***************************************************************************
*  netdiag_reset_tr(sub_pr, arg);
***************************************************************************/

int netdiag_reset_tr (sub_ptr, arg)
netdiag_t_filters  *sub_ptr;  
rs_masks	   *arg;
{

   /* check for the kind bit  */
   if (arg->rs_filters & RS_KIND) {
      sub_ptr->kind = DF_KIND;
   }

   /* check for the subsys bit being set to turn off tracing on the subsys */
   if (arg->rs_filters & RS_SUBSYS) {
      TRC_SUBSYS_MAP(arg->subsys_id, NETDIAG_OFF) 
   } 

   return(0);

}  /* end of netdiag_reset_tr */



#ifdef KTL_GET_STATUS 

/* BEGIN_IMS net_get_status    *
**************************************************************************
****
****	  int net_get_status(dev, arg)
****
**************************************************************************
*
* Input Parameters:
*		 int		  dev;
*		 dm_mask_t	 *arg;
*
* Output Parameters:
*		 int   *status;
*
* Return Value:
* Globals Referenced:
*		 int netdiag_trace_on
*		 int netdiag_log_on
*
* Description
*    To report the status or current state of a given subsystem
*
* Algorithm:
*    - determine if this is for tracing or logging and the subsystem
*      dev is from the host T/L 
*    - then call a function to get the status for tracing or logging.
*      The functions are: osidiag_trstat for tracing and osidiag_logstat for 
*      logging
*
* netdiag_logstat
* ***************
*   - the sub_ptr sent in is of type tl_filters 
*   - get the current class state and pass it back out
*   - arg->class = sub_ptr->class;
*
*
* netdiag_trstat
* ***************
*   - get the current kind value and pass it out
*   - get the uid value and pass it out
*
* Modification History
*  9/14/89  K.Cirimele	 inital revision
*  2/1/89   K.Cirimele	 Updates from code review:
*	    - reutrn(EINVAL) instead of -1 on bad subsystem id 
*
******************************************************************************
* END_IMS net_get_status  */


int net_get_status(dev, arg)
int	   dev;
dm_mask_t  *arg;
{

   netdiag_t_filters  *subptr_t;
   netdiag_l_filters  *subptr_l;

   if (!(KTL_RANGE(arg->subsys_id))) return (EINVAL);

   /* zero out all fields in *arg except subsys, this way, not a lot */
   /* of extra junk is sent out with the call */
   arg->kind = arg->class = arg->device_id = 0;
   arg->size = arg->uid = 0;

   if (dev == NET_TRACE) {
       subptr_t = &netdiag_masks[arg->subsys_id].trace;	  
       netdiag_trstat(subptr_t, arg);
   }
   else	if (dev == NET_LOG) {
	 subptr_l = &netdiag_masks[arg->subsys_id].log; 
	 netdiag_logstat(subptr_t, arg);
   }
   else
	 return(EINVAL);

   return(0);
} /* end of net_get_status */ 


/***************************************************************************
*  netdiag_reset_log(sub_pr, arg);
***************************************************************************/

int netdiag_logstat(sub_ptr, arg)  
netdiag_l_filters     *sub_ptr;
dm_mask_t	      *arg;
{

   /* the subptr is already pointing to the correct subsystem */
   arg->class = sub_ptr->class;
   return(0);

} /* end of get_log_stat() */


/***************************************************************************
*  netdiag_reset_trstat(sub_pr, arg);
***************************************************************************/

int netdiag_trstat(sub_ptr, arg)
netdiag_t_filters     *sub_ptr;
dm_mask_t	      *arg;
{
   /* get the current kind value and pass it out */
   arg->kind = sub_ptr->kind;
   return(0);
} /* end of get_tr_stat */
#endif	KTL_GET_STATUS 



#ifdef NEVER_CALLED
/* BEGIN_IMS kget_log_instance	  *
**************************************************************************
**** 
****	 (unsigned short) kget_log_instance()  
**** 
**************************************************************************
* 
* Input Parameters:
* Output Parameters:
* Return values:
*      0xCxxx	  - unsigned short for log instance 
*		    
* Globals Referenced:
* Function:
*     To return to the calling function a unique log instance. This number
*     will have bits 15 and 14 set with random generation of the remaining 
*     14 bits. 
*
* Algorithm:
*     - an unsigned short will have the log instance in it, this 
*	variable will be initialized at 0xC000 and count up.
*	when the value reaches 0xFFFF, it is rolled over to 0xC000.
*
* To Do List
* Notes
* Modification history
*    9/14/88  K.Cirimele     initial revision
*
* External Calls
* Called by(optional)
*   OSI kernel modules
*     ulipc
*     cia
*     osi0
******************************************************************************
* END_IMS kget_log_instance  */ 


unsigned short kget_log_instance()
{
    return ((unsigned short)ktl_log_inst > 0xfffe ? ktl_log_inst = 0xc000 : ++ktl_log_inst);
}				  
#endif /* NEVER_CALLED*/




/* BEGIN_IMS  ns_strlen*
**************************************************************************
****
****	 ns_strlen(s) 
****
***************************************************************************
*
* Input Parameters:
*	 *s   char    
* Output Parameters:
*		 none.
* Return Value:
* Globals Referenced:
*
* Description
*		 
* Algorithm:
*
* To Do List
* Notes
* Modification History
*   12/4/89   K.Cirimele   stolen from NS
*
* External Calls
* Called by
*   routines in net_diag.c
*   
*************************************************************************
* END_IMS ns_strlen*/


ns_strlen(s)
      char *s;
{
   int len=0;

   /*
    * count length of string up to one mbufs worth
    */
   while(*s++ != 0 && len < MLEN)
	 len++;

   return(len);
}



