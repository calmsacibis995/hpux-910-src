/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dm.c,v $
 * $Revision: 1.11.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:39:24 $
 */
/* HPUX_ID: @(#)dm.c	55.1		88/12/23 */

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

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/


#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/mbuf.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/selftest.h"
#include "../dux/dux_hooks.h"
#include "../h/mp.h"
#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI
#include "../machine/vmparam.h"	/* ON_ISTACK */
extern int funcentrysize;
extern struct dm_funcentry dm_functions[];
#ifdef	MP
extern int getprocindex();
#endif



/*
** The following is a method of keeping track of the number
** of particular types of requests that are received/requested. It is simply
** a set of counters that match up with the definitions in dmmsgtype.h.
**
** NOTE: Should really size the array to be "funcetrysize". but the
**	 compiler complains about "expecting a constant".
*/
struct proto_opcode_stats inbound_opcode_stats;
struct proto_opcode_stats outbound_opcode_stats;





/*
** NOTE: All of the dm_XXX functions are now macros (defined in dm.h).
**	 This was necessary for making the networking and DUX code
**	 configurable. Actually, it wasn't necessary, but was  the
**	 path of least resistance. It was simply easier to make the
**	 calls macros, rather than change the literally hundreds of 
**	 calls to them. At any rate, the actual function calls are
**	 now renamed dm1_XXX, as indicated below. If DUX is configured
**	 into the kernel, then the following (dm1_XXX) functions are 
**	 called via the DUXCALL(DM_XXX) (dux_proc[]) set of pointers,
**	 otherwise they resolve to nop(){} functions.
**	
**	 Yea, Yea, I know its confusing, but it's really pretty simple.
**	 Study the following files if you are really interested:
**						daveg
**
**		o dux/dux_hooks.c
**		o dux/dux_hooks.h
**		o dux/dm.h
**		o dux/dm.c
**		o conf.c
**		o master
**		o dfile.*
**
*/


/*
 *Allocate a network packet which will hold a message of size nbytes.
 *To this size, is added the size of the standard headers.
 *A dm_message is actually an mbuf or an mcluster, depending on the
 *size of the message.  Mbufs are used only for convenience; in the
 *future, this mechanism could be replaced, with no changes to the
 *overlying code.
 */
dm_message
dm1_alloc(nbytes,canwait)
int nbytes;	/*the number of bytes in the users portion of the message*/
int canwait;	/*whether to sleep or return immediately if no mbufs available*/
{
	register struct mbuf *mb;

	/*
	** Request an mbuf.
	*/
	mb = (struct mbuf *)dux_mem_get(canwait, MT_DATA, nbytes);

	if (mb == NULL)
	{
		return (NULL);
	}
	else
	{
		/*
		** Record that we have allocated a
		** dm message for selftest purposes.
		*/
		TEST_PASSED(ST_DM_MESSAGE);
	}

	return ((dm_message)mb);

}



/*
 *Release a dm message.  If the release_buf parameter is set, also release the
 *associated buffer, unless, the DM_KEEP_BUF flag is set in the DM message.
 *This is a bit of a kluge, but it allows us to make use of automatic
 *release code, for example when sending a reply, which normally will
 *release the buffer, but can be overridden.  (One place that this is
 *particularly useful is if the buffer used for the reply is the same
 *buffer that was sent with the request.
 */
dm1_release(message,release_buf)
dm_message message;	/*the packet to be released*/
int release_buf;	/*if non-zero, also release (brelse) the
			 *associated buffer (if any)*/
{
	register struct buf *bp;
	register struct dm_header *hp = DM_HEADER(message);
	struct mbuf *mb = (struct mbuf *)message;



	/*
	** If there is an associated buffer is to be
	** released, then release it
	*/
	if (release_buf && (bp = hp->dm_bufp) != NULL &&
		!(hp->dm_flags&DM_KEEP_BUF))
	{
		brelse (bp);
	}


	/*
	** Now (finally) free the message.
	*/
	(void) dux_m_free(mb);

}


/*
 *Send a request to a remote site.  The return value is a pointer to the
 *response packet (if any).  See the DM interface for a complete definition
 *of all the parameters.
 */
/*VARARGS5*/
dm_message
dm1_send(request, flags, op, dest, resp_size, func,
	buf1, buf1size, buf1offset,
	buf2, buf2size, buf2offset)
dm_message request;	/*The message being sent*/
register int flags;	/*Various flags*/
int op;			/*The packet operation type*/
site_t dest;		/*The destination*/
int resp_size;		/*The expected maximum size of the response*/
int (*func)();		/*Function to be called after response is
			 *returned.  This function will be passed the
			 *request and the response packets.  It must be
			 *capable of operating under interrupt (i.e. it
			 *cannot sleep).  If there is no function, func
			 *should be NULL or missing.*/
struct buf *buf1;	/*The request or response buffer (if any)*/
int buf1size;		/*The buffer size*/
int buf1offset;		/*The offset into the buffer*/
struct buf *buf2;	/*The response buffer (if a request also)*/
int buf2size;		/*The buffer size*/
int buf2offset;		/*The offset into the buffer*/
{
	register struct dm_header *hp = DM_HEADER(request);
	dm_message response = NULL;
	register struct dm_header *rhp;
	register struct buf *resp_buf;
	register int respb_size,respb_offset;
	register int is_request_buf;
	register int is_response_buf;
	extern struct proto_opcode_stats outbound_opcode_stats;
#ifdef	FSD_KI
	struct timeval	starttime;

	/* get start time of request */
	KI_getprectime(&starttime);
#endif	FSD_KI

	/*Fill in the header from the parameters passed, and from globals*/
	hp->dm_op = op;
	
	/*
	** Outbound opcode statistics.
	*/
	outbound_opcode_stats.opcode_stats[ hp->dm_op ]++;

	if (flags & DM_FUNC)
		hp->dm_func = func;
	hp->dm_flags = flags&DM_HEADERMASK;
	hp->dm_srcsite = my_site;
	if (!ON_ISTACK)
		hp->dm_pid = u.u_procp->p_pid;
	hp->dm_dest = dest;
	hp->dm_bufp = NULL;
	hp->dm_datalen = 0;
	if (!ON_ISTACK)
		hp->dm_tflags = u.u_procp->p_flag&SOUSIG ? DM_SOUSIG : 0;
	else
		hp->dm_tflags = 0;

	/*set up the buffers.  Note that buf1 can be either the request buffer
	 *or the reply buffer depending on the flags.  If there are both a
	 *request, and a reply, buf1 is the request, and buf2 is the reply.
	 *If there is only a request or a reply, but not both, it is in buf2.
	 *Note that the buffer associated with a multisite or clustercast
	 *message is considered to be a reply.
	 */
	is_request_buf = flags&DM_REQUEST_BUF;
	is_response_buf = ((flags&DM_REPLY_BUF) || dest == DM_MULTISITE ||
		dest == DM_CLUSTERCAST);
	if (is_request_buf)
	{
		/*set up the request buffer*/
		hp->dm_bufp = buf1;
		hp->dm_datalen = buf1size;
		hp->dm_bufoffset = buf1offset;
		if (buf1size == 0)
			hp->dm_bufp = NULL;
		if (is_response_buf)
		{
			/*Since there is both a request buffer and a response
			 *buffer, set up the response buffer from buf2.
			 *Set it up in temporary variables, since the response
			 *is not yet allocated.  (Another reason for using
			 *temporary variables is because there are several
			 *places where the response buffer must be dealt with,
			 *and it is cheaper to use the temporaries than to
			 *perform the indirections.)
			 */
			resp_buf = buf2;
			respb_size = buf2size;
			respb_offset = buf2offset;
		}
	}
	else if (is_response_buf)
	{
		/*Since there is only a response buffer, but no request
		 *buffer, set up the response buffer from buf1.
		 *Again use the temporary variables.
		 */
		resp_buf = buf1;
		respb_size = buf1size;
		respb_offset = buf1offset;
	}
	/*datagrams and regular messages have a different interface to the
	 *network*/
	if (!(flags & DM_DATAGRAM))	/*a regular message*/
	{
		/*allocate a response for the message.  The response is
		 *preallocated now, so as to avoid allocating it under
		 *interrupt when the request comes in.
		 */
		response = dm_alloc(resp_size,WAIT);
		rhp = DM_HEADER(response);
		rhp->dm_bufp = NULL;
		rhp->dm_datalen = 0;
		if (flags & DM_KEEP_REPLY_BUF)
			rhp->dm_flags |= DM_KEEP_BUF;
		if (is_response_buf)
		{
			/*Allocate the response buffer
			 *if it was not provided by the user.
			 */
			if (resp_buf == NULL)
			{
				if (dest == DM_CLUSTERCAST)
				{
					resp_buf = geteblk(
						sizeof(struct dm_multisite));
				}
				else if (dest != DM_MULTISITE &&
					respb_size != 0)
				{
					/*note that if the data is to be
					 *placed at an offset in the
					 *response buffer, we must allocate
					 *space both for the offset and for
					 *the data
					 */
					resp_buf = geteblk(
						respb_size+respb_offset);
					
				}
			}
			/*If the operation is a clustercast, format the
			 *buffer as appropriate.
			 */
			if (dest == DM_CLUSTERCAST)
				clusterlist(resp_buf->b_un.b_addr);
			/*assign the buffer to the response*/
			rhp->dm_bufp = resp_buf;
			rhp->dm_datalen = respb_size;
			rhp->dm_bufoffset = respb_offset;
		}
		/*give the request to the network*/
		net_request(request,response);
	}
	else /*a datagram request*/
	{
		/*simply give the datagram to the network*/
		send_datagram(request);
#ifdef	FSD_KI
		KI_dm1_send(hp, &starttime);
#endif	FSD_KI
		return(NULL);	/*after sending a datagram, there is nothing
				 *more to do.*/
	}
	if (flags&DM_SLEEP)
	{
		/*If the user asked us to sleep on the message, do so*/
		dm_wait(request);

		/* update the acflag in the u area to relect the use of
		 *super user privleges on the server.
		 */
		u.u_acflag |= rhp->dm_acflag;

		/* if this was a system call, then update the eosys in the
		 *u area and test for a long jump on the server.
		 */
		if (u.u_eosys != EOSYS_NOTSYSCALL){
		    u.u_eosys = rhp->dm_eosys;
		    /*If we got a signal, and a longjmp was requested, we must
		     *take it.  Before taking it, we MUST release the request
		     *and the response, REGARDLESS of the flags passed.  This
		     *is because the longjmp will take us out of the code that
		     *sent the messages in the first place.
		     */
		    if (rhp->dm_tflags & DM_LONGJMP){
#ifdef	FSD_KI
			KI_dm1_send(hp, &starttime);
#endif	FSD_KI
			dm_release(request,1);
			dm_release(response,1);
			longjmp (&u.u_qsave);
		     }
		}

		/*Release the request and the response if we were requested to
		 *do so.
		 */
#ifdef	FSD_KI
		KI_dm1_send(hp, &starttime);
#endif	FSD_KI
		if (flags & DM_RELEASE_REQUEST)
		{
			dm_release(request,1);
		}
		if (flags & DM_RELEASE_REPLY)
		{
			dm_release(response,1);
			response = NULL;
		}
		return(response);
	}
#ifdef	FSD_KI
	KI_dm1_send(hp, &starttime);
#endif	FSD_KI
	return(response);
}

/*
 *Send a response to the current message being operated on.  For a complete
 *definition of the interface to this function see the DM interface document.
 *Note:  The request to which we are replying is not passed with to this
 *function.  Instead, it is gotten from either dm_int_dm_message or from
 *u.u_request.  (Authors comment:  I don't wish to defend this decision.
 *In retrospect, it seems like a poor choice.  However, it seemed like a good
 *idea at the time, and everyone else agreed.  While it could be changed now,
 *that would be rather difficult, and besides, everything does work.)
 */
/*VARARGS3*/
dm1_reply(response, flags, return_code, resp_buf, resp_bufsize, resp_bufoffset)
dm_message response;	/*The packet being sent.  If only a return code and/or
			 *a buffer needs to be returned, this may be NULL, in
			 *which case a message will be allocated.*/
int flags;		/*various flags*/
int return_code;	/*The error code.  Should be 0 if no error.*/
struct buf *resp_buf;	/*A pointer to an additional data buffer to be
			 *transferred.  May be NULL*/
int resp_bufsize;	/*The number of bytes to be transferred in the
			 *data buffer.  Should be 0 if buffer is NULL*/
int resp_bufoffset;	/*The first byte of the data buffer to be
			 *transferred*/
{
	register struct dm_header *hp;
	dm_message request;
	struct buf *request_bp;

	/*find the request we are replying to*/
#ifdef	MP
	request = (dm_message) mpproc_info[getprocindex()].dux_int_pkt;
	if (request == (dm_message) 0)
		request = u.u_request;
#else
	request = dm_is_interrupt?dm_int_dm_message:u.u_request;
#endif
	request_bp = DM_BUF(request);
	/*If no response was passed, we can use the request as the response*/
	if (response == NULL)
	{
		DM_SHRINK(request,0);
		response = request;
	}
	/*fill in the various fields.*/
	hp = DM_HEADER(response);
	hp->dm_rc = return_code;
	hp->dm_srcsite = my_site;
	if (!ON_ISTACK)
		hp->dm_eosys = u.u_eosys;
	else
		/* Need to set this value to something because a client
		 * who receives this reply may use this value to update
		 * its u.u_eosys.  For example, a client receiving a
		 * reply to a DM_UNLOCK_MOUNT request will do this.
		 * Failure to set this value may result in arbitrary 
		 * values being returned by system calls.   mry 1/15/91
		 */
		hp->dm_eosys = EOSYS_NORMAL;

	hp->dm_acflag = 0;
	if (!ON_ISTACK)
		hp->dm_acflag |= u.u_acflag;
	hp->dm_tflags = flags & DM_LONGJMP;
	/*set up the buffer if any*/
	if (flags & DM_REPLY_BUF)
	{
		hp->dm_bufp = resp_buf;
		hp->dm_datalen = resp_bufsize;
		hp->dm_bufoffset = resp_bufoffset;
	}
	else
	{
		hp->dm_bufp = NULL;
		hp->dm_datalen = 0;
	}
	/*Since the network is responsible for releasing the reply,
	 *set the appropriate flag if we wish to avoid releasing the
	 *associated buffer
	 */
	hp->dm_flags = (flags&DM_KEEP_REPLY_BUF) ? DM_KEEP_BUF : 0;
	
	/* Get the destination site from the request */
	hp->dm_dest = DM_SOURCE(request);

	/*Release the request message, and if appropriate, the request
	 *buffer.  However, if the request was reused as the
	 *response, only release the buffer (if appropriate).
	 */
	if (response == request)
	{
		if (request_bp != NULL && !(flags&DM_KEEP_REQUEST_BUF))
			brelse(request_bp);
	}
	else
	{
		hp->dm_mid = DM_HEADER(request)->dm_mid;
		dm_release(request,!(flags&DM_KEEP_REQUEST_BUF));
	}
	net_reply(response);
}

/*
 *A quick way to send a reply.  Only the error code is sent back.
 */
dm1_quick_reply(return_code)
int return_code;
{
	dm_reply(NULL, 0, return_code, NULL, NULL, NULL);
}

/*
 *Send a followup signal to the nsp serving the specified message.  This
 *is needed if the local process receives a signal.  There is a potential
 *race condition, since the signal message could arrive at the serving site
 *before the original message.  Thus, when the reply comes back we check
 *to see if the signal was already delivered.  If it wasn't, and if the
 *reply to the original message has not yet come in, we resend the signal.
 */
senddmsig1(message)
dm_message message;
{
	register int got_thru = 0;
	register struct dm_header *reqhp = DM_HEADER(message);
	dm_message sigmsg;
	register struct dm_header *sighp;
	register dm_message response;

	/*If the request is multisite, don't bother sending the signal.
	 *This is made under the assumption that multisite messages are short
	 *and aren't abortable anyway.  The code as written will not work
	 *for a multisite message, and so we ignore them rather than writing
	 *new unnecessary code.
	 */
	if (DM_IS_MULTI(reqhp->dm_dest))
		return;

	/*Allocate a message for sending a signal*/
	sigmsg = dm_alloc(sizeof(long), WAIT);
	sighp = DM_HEADER(sigmsg);

	/*Place the request ID of the original message into the new
	 *message.  This will be used to find the message that is being
	 *signaled.
	 */
	*(DM_CONVERT(sigmsg, long)) = reqhp->dm_ph.p_rid;

	/*Loop until either we are informed that the signal message
	 *has gotten through and was delivered to the appropriate NSP, or
	 *until a reply comes back to the original message (in which case
	 *we don't need to deliver the signal at all).
	 */
	while (!got_thru && !(reqhp->dm_flags&DM_DONE))
	{

		/* took out the line to raise the priority to 5 to
		   block out interrupts until after the call to sleep.
		   this may need to be added again later if someone other
		   than dm_wait calls us.
		*/

		response = dm_send(sigmsg, DM_REPEATABLE, DMSIGNAL, 
			reqhp->dm_dest, sizeof(int), NULL, NULL, NULL,
			NULL, NULL, NULL, NULL);
		/*sleep on the signal message.  We can't use dm_wait to do
		 *the sleeping, because that would lead to infinite recursion
		 *as it received the signal and tried to send a followup.
		 */
		while (!(sighp->dm_flags & DM_DONE))
		{
			sighp->dm_flags |= DM_SLEEP;
			sleep((caddr_t)sigmsg, PZERO);
		}
		/*If the response came through without error, and the message
		 *was delivered by an nsp, set got_thru so we can complete
		 *the operation.
		 */
		got_thru = DM_RETURN_CODE(response) != 0 ||
			*(DM_CONVERT(response, int)) != 0;
		dm_release(response, 0);
	}
	dm_release(sigmsg, 0);
}

/*
 *Wait for a message to complete
 *We originally sleep at an interruptable priority.  If an interrupt comes
 *in, and the message is interruptable,
 *we send a follow up signal message, and then sleep at a non interruptable
 *priority.  This prevents infinite looping.
 */
dm1_wait(message)
dm_message message;
{
	int s;
	register struct dm_header *hp = DM_HEADER(message);
	int priority=PRIDM;	/*this is an interruptable priority*/
	label_t saveq;
	sv_sema_t ss;

	saveq = u.u_qsave;	/*save old value of u.u_qsave*/

#ifdef	MP
	/*
	**	Non-zero return from setjmp would have thrown
	**	away any semaphores, so we'll release them 
	**	here so we can remember which ones we need to
	**	return to the caller.
	*/
	release_semas(&ss);
#endif
	/* raise our priority to block out the interrupt until after
	   we have have succeessfully called sleep. */
	s = spl5();

	if (hp->dm_flags & DM_INTERRUPTABLE)
	{
		int unset_skeep=0;

		/* Make sure that SKEEP is set so that we don't get
		 * swapped out while waiting for our reply.  Could
		 * cause nasty deadlocks.  Make sure we save whether or
		 * not this routine set SKEEP, so we don't clear it here
		 * unless we set it here.
		 */
		if ((u.u_procp->p_flag & SKEEP) == 0) {
			u.u_procp->p_flag |= SKEEP;
			unset_skeep=1;
		}

		priority = PRIDM;
		if (setjmp(&u.u_qsave))
		{
			/*If we get here, we have been interrupted so send the
			 *followup message*/
			senddmsig(message);
			priority=PZERO;	/*don't get interrupted again*/
		}
		/*sleep until the reply comes in*/
		while (!(hp->dm_flags&DM_DONE))
		{
			hp->dm_flags |= DM_SLEEP;
			sleep((caddr_t)message,priority);
		}
		/* Unset SKEEP if it wasn't set when we came in */
		if (unset_skeep)
			u.u_procp->p_flag &= ~SKEEP;
	}
	else
	{
		/*sleep until the reply comes in*/
		while (!(hp->dm_flags&DM_DONE))
		{
			hp->dm_flags |= DM_SLEEP;
			sleep((caddr_t)message,PZERO);
		}
	}
	splx(s);
#ifdef	MP
	reaquire_semas(&ss);
#endif
	u.u_qsave = saveq;	/*restore u.u_qsave*/
}
  
/*
 *Receive a reply to a message.  This function is called by the network whenever
 *a reply comes in.  It is responsible for waking up sleepers, calling
 *functions, and whaever else was specified in the request.
 */
dm1_recv_reply(request,response)
dm_message request, response;
{
	register struct dm_header *reqhp = DM_HEADER(request);
	register int flags = reqhp->dm_flags;
	int (*func)() = reqhp->dm_func;

	reqhp->dm_flags |= DM_DONE;
	reqhp->dm_flags &= ~DM_FUNC;
	/*If the caller is sleeping wake him up.  If noone is sleeping on
	 *this request, and we are instructed to, release the request
	 *and/or the response.  We don't release them if there was a sleeper,
	 *because in that case the sleeper is responsible for releasing them.
	 */
	if (flags & DM_SLEEP)
	{
		wakeup(request);
	}
	else
	{
		 if (flags & DM_RELEASE_REQUEST)
		 {
			dm_release(request,1);
			request = NULL;
		 }
		 if (flags & DM_RELEASE_REPLY)
		 {
			dm_release(response,1);
			response = NULL;
		 }
	}
	/*If there is a function to call, call it*/
	if ((flags & DM_FUNC) && func != NULL)
	{
		(*(func))(request,response);
	}
}

#ifdef	MP
#define	SETINTDM mpproc_info[getprocindex()].dux_int_pkt = \
	(char *) request
#define	CLEARINTDM mpproc_info[getprocindex()].dux_int_pkt = \
	(char *) 0
#else
#define SETINTDM dm_is_interrupt = 1
#define CLEARINTDM dm_is_interrupt = 0
#endif

/*Receive an incoming request.  The request should either be processed under
 *interrupt, given to an NSP, or given to a UNSP.  Determine how to process it
 *by looking up the opcode in the funcentry table.  The function will panic
 *if the request opcode is not in the funcentry table.
 */
dm1_recv_request(request)
dm_message request;
{
	register struct dm_header *hp = DM_HEADER(request);
	register struct dm_funcentry *funcentry = dm_functions+hp->dm_op;
	int error;
	
	extern struct proto_opcode_stats inbound_opcode_stats;
	extern int funcentrysize;

	/*save the message so we can send a reply to it.  This variable
	 *is normally used when processing a request under interrupt.  However,
	 *we set it out here rather than in the DM_INTERRUPT case because it
	 *can also be used if, for example there are no NSPs available.
	 */
#ifndef	MP
	dm_int_dm_message = request;
#endif




	/*verify the validity of the opcode*/
	if (hp->dm_op <= 0 || hp->dm_op >= funcentrysize)
	{
		/* 
		** Rather than panic, send back an error if this
		** is not a datagram. If it is drop it!
		*/
		if (hp->dm_ph.p_flags & P_DATAGRAM) {
			dm_release(request,1);
		} else {
			SETINTDM;
			dm_quick_reply(EACCES);
			CLEARINTDM;
		}
	}
	else
        {
#ifdef	FSD_KI
		struct timeval	starttime;

		/* get start time of request */
		KI_getprectime(&starttime);
#endif	FSD_KI
   		/*
   		** Inbound Op-code statistics?
   		*/
		inbound_opcode_stats.opcode_stats[ hp->dm_op ]++;


	switch (funcentry->dm_how&DM_HOWMASK)
	{
	case DM_KERNEL:		/*invoke a kernel NSP*/
		if (funcentry->dm_callfunc == NULL)
		{
		  printf ("dm_recv_request: case DM_KERNEL, op=%d\n",hp->dm_op);
		  panic ("DM_KERNEL: Invalid dm function");
		}

		/* Try to invoke the NSP*/
		if ((error = invoke_nsp(request,funcentry->dm_callfunc,
			funcentry->dm_how&DM_LIMITED)) != 0)
		{
			/* error in invoking NSP, not in function itself
			** set up as if interrupt so can reply with error.
			** If its a datagram drop it!
			*/
		        if (hp->dm_ph.p_flags & P_DATAGRAM) {
		        	dm_release(request,1);
		        } else {
		        	SETINTDM;
		        	dm_quick_reply(error);
		        	CLEARINTDM;
		        }
		}
		break;

	case DM_INTERRUPT:	/*process under interrupt*/
		if (funcentry->dm_callfunc == NULL)
		{
		 printf("dm_recv_request: case DM_INTERRUPT,op=%d\n",hp->dm_op);
		 panic ("DM_INTERRUPT: Invalid dm function");
		}
		SETINTDM;
		/*call the function specified*/
		(*funcentry->dm_callfunc)(request);
		CLEARINTDM;
		break;

	case DM_USER:		/*invoke a user NSP*/
		if ((error = invoke_unsp(request,NULL)) != 0)
		{
			/* error in invoking UNSP, not in function itself
			** set up as if interrupt so can reply with error.
			** If its a datagram drop it!
			*/
		        if (hp->dm_ph.p_flags & P_DATAGRAM) {
		        	dm_release(request,1);
		        } else {
		        	SETINTDM;
		        	dm_quick_reply(error);
		        	CLEARINTDM;
		        }
		}
		break;
	default:
		printf (" dm_recv_request: case default, op=%d\n",hp->dm_op);
		panic ("Invalid dm function");
	}
#ifdef	FSD_KI
	KI_dm1_recv(hp, &starttime);
#endif	FSD_KI
     }	/* End of the else */
}
