/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/unsp.c,v $
 * $Revision: 1.7.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:46:36 $
 */

/* HPUX_ID: @(#)unsp.c	55.1		88/12/23 */

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


/*
 *This file contains the code for dealing with User level NSPs.  These are
 *processes which do most of their operation in user space, but need special
 *communication mechanisms with the kernel.
 */
#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../dux/dm.h"
#include "../dux/unsp.h"

extern int wakeup();
extern int nodev();
extern struct buf *brealloc();

/*allocate a reply if none present*/
static
alloc_reply(this_unsp)
struct unsp *this_unsp;
{
	register struct dm_header *hp;

	if (this_unsp->un_reply == NULL)
	{
		this_unsp->un_reply =
			dm_alloc(sizeof(struct unsp_message), WAIT);
		hp = DM_HEADER(this_unsp->un_reply);
		hp->dm_rc = 0;
		hp->dm_eosys = EOSYS_NORMAL;
	}
}

/*
 *This function is called under timeout if the UNSP takes to long to be
 *created.  The reason it is needed is because UNSPs are user processes
 *that are created from scratch whenever needed.  It is possible that we
 *will be unable to create the UNSP.  One example that could cause this is
 *the lack of slots in the process table.  If we didn't do anything about this,
 *we could hang forever.  Instead we schedule timeouts.  The timouts will
 *retry the creation of the UNSP.  For now we retry indefinitely.
 */
unsp_signal_timeout(this_unsp)
register struct unsp *this_unsp;
{
	/*signal the parent to fork*/
	signal_unsp_forker();

	/*reset the timeout*/
	timeout (unsp_signal_timeout,this_unsp,HZ*5);

	if (this_unsp->un_timeouts++ == MAX_UNSP_TIMEOUT)
		printf ("cannot start user csp for %d seconds\n",
			5*MAX_UNSP_TIMEOUT);
}

/*
 *First check the ID to determine if it is a new unsp or an old one.
 *If it is new, invoke a new one.  If it is old, attach the message and
 *wake it up (if necessary).  Reply is the reply to use.  If it is NULL,
 *a reply will be allocated if and when needed.
 *It might be a good idea not to overload the EIO error so much.  On the
 *other hand, if UNSPs are ever used for read/write, it is good not to
 *introduce new errors.
 */
int
invoke_unsp(request,reply)
dm_message request,reply;
{
	register int unsp_no;
	register struct unsp *this_unsp;
	struct unsp_message *message = DM_CONVERT(request,struct unsp_message);
	int id = message->um_id;


	if (id < 0)	/*Is the message new?*/
	{
		/* It is new.  First, find a slot in the UNSP table */
		this_unsp = NULL;
#ifdef FULLDUX
	fix this - window between test and set of un_flags
#else
	/* Fix this for FULLDUX - not a problem in 6.0.
	 * There is a window between test and set of un_flags
	 * if this routine can be called under interrupt.
	 */
#endif
		for (unsp_no = 0; unsp_no < MAX_UNSP; unsp_no++)
		{
			if (!(unsps[unsp_no].un_flags & UNSP_IN_USE))
			{
				/*we found a free one.  set up the entry.*/
				this_unsp = &unsps[unsp_no];
				this_unsp->un_flags = UNSP_IN_USE;
				if (reply) {
					this_unsp->un_flags |= UNSP_WAKE_REPLY;
					DM_RETURN_CODE(reply) = 0;
				}
				this_unsp->un_request = request;
				this_unsp->un_reply = reply;
				this_unsp->un_slot = unsp_no;
#ifdef FULLDUX
	fix this - an unsigned short is never < 0
	Also we increment the unique field twice (here and on close)
#else
	/* fix this for FULLDUX - path not taken in 6.0
	 * an unsigned short is never < 0
	 * Also we increment the unique field twice (here and on close)
	 */
#endif
				++(this_unsp->un_unique);
				this_unsp->un_site = DM_SOURCE(request);
				break; /*from for*/
			}
		}
		if (this_unsp == NULL)
		{
			/* We couldn't find a slot.  Just give up in
			 * disgust.  Note that this is an area for potential
			 * improvement, in that we could queue the message.
			 */
			tablefull ("User csp");
			return (EIO);
		}
		/*
		 *Signal the parent to fork.  
		 *Set a timeout for a few seconds, 
		 *in order to cover for any problems.
		 */
		signal_unsp_forker();

		/*set the timeout*/
		this_unsp->un_timeouts = 0;
		timeout (unsp_signal_timeout,this_unsp,HZ*5);
		return (0);
	}
	else	/*reinvoking unsp*/
	{
		/*check the id.  The least significant 16 bits is the
		 *slot number, and the most significant 16 bits is a
		 *unique ID.
		 */
		unsp_no = id & 0xffff;	/*get the slot number*/
		if (unsp_no >= MAX_UNSP || unsp_no < 0)
		{
			/*Impossible ID*/
			return (EIO);
		}
		this_unsp = unsps + unsp_no;
		if (id != this_unsp->un_id)
		{
			/*While the slot is valid, the unique ID doesn't match*/
			return (EIO);
		}
		/*hang the request on the hook*/
		this_unsp->un_request = request;
		this_unsp->un_reply = reply;
		/* If the UNSP is waiting for this request, wake it up */
		if (this_unsp->un_flags & UNSP_UWAITING)
		{
			wakeup (this_unsp);
			this_unsp->un_flags &= ~UNSP_UWAITING;
		}
		return (0);
		
	}
}

/* The following is the indirect table for use with the file operations */
int unsp_rw(), unsp_ioctl(), unsp_close();
struct fileops unsp_ops =
	{ unsp_rw, unsp_ioctl, nodev, unsp_close };
/*
 *Open a file descriptor for a user nsp.  This is a special file descriptor
 *with the special operations set up from the table above.
 */
unsp_open()
{
	register struct file *fp;
	register int count;
	register struct unsp *this_unsp;
	sv_sema_t ss;

	/* only the superuser can do this */
	if (!suser())
		return;
	PXSEMA(&filesys_sema, &ss);
	/* first see if there any pending requests */
	this_unsp = NULL;
	for (count = 0; count < MAX_UNSP; count++)
	{
		if ((unsps[count].un_flags&UNSP_IN_USE) &&
			 !(unsps[count].un_flags&UNSP_HAS_PROCESS))
		{
			/*we found the request*/
			this_unsp = &unsps[count];
			untimeout(unsp_signal_timeout,this_unsp);
			break;  /* from for */
			/* It might be a good idea to continue the scan
			 * and signal the forker if there are more requests
			 * to be serviced, since multiple signals can be
			 * collapsed into a single one.  Timeouts and retries
			 * suffice to insure that no requests are lost, but
			 * this could improve response on busy systems.
			 */
		}
	}
	if (this_unsp == NULL)	/*false alarm.  No request*/
	{
		u.u_error = EIO;
		VXSEMA(&filesys_sema, &ss);
		return;
	}
	/*allocate a file descriptor*/
	fp = falloc();
	if (fp == NULL)
	{
		/*If we couldn't allocate a file descriptor, send a
		 *message to the requestor, specifying that the error
		 *has occured.
		 */
		alloc_reply(this_unsp);
		DM_RETURN_CODE(this_unsp->un_reply) = u.u_error;
		unsp_send_reply(this_unsp);
		VXSEMA(&filesys_sema, &ss);
		return;
	}
	/*set up the file descriptor and other variables*/
	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_UNSP;
	fp->f_ops = &unsp_ops;
	fp->f_data = (caddr_t)this_unsp;
	this_unsp->un_flags |= UNSP_HAS_PROCESS;
	this_unsp->un_proc = u.u_procp;
	u.u_duxflags |= DUX_UNSP;
	/* If we got a signal before the process was created, signal the
	 * process
	 */
	VXSEMA(&filesys_sema, &ss);
	if (this_unsp->un_flags & UNSP_SIGNAL)
		psignal (u.u_procp, SIGUSR1);

}

/*
 *This function is called before most file operations of the UNSP.  It
 *has two purposes.
 *
 *   1)  It will wait until a request comes in to operate on.
 *
 *   2)  It will verify that the site on whos behalf the UNSP is executing
 *       is still alive.
 */
int
unsp_check(fp)
struct file *fp;
{
	register struct unsp *this_unsp = (struct unsp *)(fp->f_data);
	int s;

	/* Wait until the request comes in (or the site is declared dead) */
	s = spl5();
	while (!(this_unsp->un_flags & UNSP_DEAD) &&
		(this_unsp->un_request == NULL))
	{
		this_unsp->un_flags |= UNSP_UWAITING;
		sleep (this_unsp,PSLEP);
	}
	splx(s);
	/* check for the dead site */
	if (this_unsp->un_flags & UNSP_DEAD)
	{
		return (EIO);
	}
	return (0);
}

/*
 *Perform the read or write operation for a UNSP.  This operation transfers
 *data to or from the disc buffer associated with the message.
 */
unsp_rw(fp, rw, uio)
struct file *fp;
enum uio_rw rw;
struct uio *uio;
{
	register struct unsp *this_unsp = (struct unsp *)(fp->f_data);
	int error;
	register int flags;
	register struct buf *bp;
	int tcount;	/*transfer count*/
	sv_sema_t ss;

	PXSEMA(&filesys_sema, &ss);
	/* check the fd and wait for a message */
	if ((error = unsp_check(fp)) != 0){
		VXSEMA(&filesys_sema, &ss);
		return (error);
	}
	flags = this_unsp->un_flags;
	if (rw == UIO_READ)
	{
		/* We are reading.  Transfer the data from the buffer to
		 * the UNSP.
		 */
		register dm_message request = this_unsp->un_request;

		bp = DM_BUF(request);
		if (bp == NULL){
			VXSEMA(&filesys_sema, &ss);
			return (0);	/*nothing to read*/
		}
		tcount = MIN (bp->b_bcount, uio->uio_resid);
		error = uiomove(bp->b_un.b_addr, tcount,
			UIO_READ, uio);
		if (!error && (flags & UNSP_RREAD))
		{
			/* If the UNSP_RREAD flag is set, we must send a reply
			 */
			unsp_send_reply(this_unsp);
		}
	}
	else
	{
		/* We are writing data.  Transfer the data from the UNSP to
		 * the buffer.
		 */
		register dm_message reply;

		tcount = MIN(MAXDUXBSIZE,uio->uio_resid);
		alloc_reply(this_unsp);
		reply = this_unsp->un_reply;
		bp = DM_BUF(reply);
		/* If there is no buffer, we need to allocate one before
		 * filling it.  If there is a buffer, but it is the wrong
		 * size, make it the right size.
		 */
		if (bp == NULL)
			bp = geteblk(tcount);
		else
			bp = brealloc(bp,tcount);
		DM_BUF(reply) = bp;
		DM_HEADER(reply)->dm_datalen = tcount;
		error = uiomove(bp->b_un.b_addr, tcount,
			UIO_WRITE, uio);
		if (!error && (flags & UNSP_RWRITE))
		{
			/* If the UNSP_RWRITE flag is set, we must send a reply
			 */
			unsp_send_reply(this_unsp);
		}
	}
	VXSEMA(&filesys_sema, &ss);
	return (error);
}

/*
 *Perform various operations as specified in the UNSP interface
 */
unsp_ioctl(fp,com,data)
struct file *fp;
int com;
caddr_t data;
{
	register struct unsp *this_unsp = (struct unsp *)(fp->f_data);
	int error;
	register struct dm_header *hp;
	sv_sema_t ss;

	PXSEMA(&filesys_sema, &ss);
	switch (com)
	{

	case UNSP_GETOP:
		/*Get the opcode out of the request and return it to the user.*/
		if ((error = unsp_check(fp)) != 0){
			VXSEMA(&filesys_sema, &ss);
			return (error);
		}
		u.u_r.r_val1 = DM_OP_CODE(this_unsp->un_request);
		VXSEMA(&filesys_sema, &ss);
		return (0);

	case UNSP_GETSITE:
		/*Get the site requesting the operation and return it*/
		u.u_r.r_val1 = this_unsp->un_site;
		VXSEMA(&filesys_sema, &ss);
		return (0);

	case UNSP_SETFL:
		/*Set the flags specified in the parameter.  Return the old
		 *flags.  Note that only the flags specified by UNSP_FLAGS
		 *may be set.
		 */
		u.u_r.r_val1 = this_unsp->un_flags & UNSP_UFLAGS;
		this_unsp->un_flags = (this_unsp->un_flags & ~UNSP_UFLAGS) |
			(*(int *)data & UNSP_UFLAGS);
		VXSEMA(&filesys_sema, &ss);
		return (0);

	case UNSP_REPLY:
		/*Send a reply to the current message, allocating one if
		 *needed
		 */
		if ((error = unsp_check(fp)) != 0){
			VXSEMA(&filesys_sema, &ss);
			return (error);
		}
		unsp_send_reply(this_unsp);
		VXSEMA(&filesys_sema, &ss);
		return (u.u_error);	/* can be set by unsp_send_reply() */

	case UNSP_CHECK_REPLY:
		/*If we have set up a reply, but haven't sent it, do so.
		 */
		if (this_unsp->un_reply && this_unsp->un_request)
			unsp_send_reply(this_unsp);
		VXSEMA(&filesys_sema, &ss);
		return (u.u_error);	/* can be set by unsp_send_reply() */

	case UNSP_ERROR:
		/*Set the error code to the specified value*/
		alloc_reply(this_unsp);
		DM_RETURN_CODE(this_unsp->un_reply) = *(int *)data;
		VXSEMA(&filesys_sema, &ss);
		return (0);

	case UNSP_READ:
		/*Transfer data from the request to the UNSP.  If the
		 *UNSP_RIREAD flag is set, also send a reply.
		 */
		if ((error = unsp_check(fp)) != 0) {
			VXSEMA(&filesys_sema, &ss);
			return (error);
		}
		bcopy ( DM_CONVERT(this_unsp->un_request,
				struct unsp_message)->um_msg,
			data,UNSP_PARM_SIZE);
		if (this_unsp->un_flags & UNSP_RIREAD)
		{
			unsp_send_reply(this_unsp);
		}
		VXSEMA(&filesys_sema, &ss);
		return (u.u_error);	/* can be set by unsp_send_reply() */

	case UNSP_WRITE:
		/*Transfer data from the UNSP to the reply.  If the
		 *UNSP_RIREAD flag is set, also send the reply.
		 */
		if ((error = unsp_check(fp)) != 0){
			VXSEMA(&filesys_sema, &ss);
			return (error);
		}
		alloc_reply(this_unsp);
		bcopy (data, DM_CONVERT(this_unsp->un_reply,
				struct unsp_message)->um_msg,
			UNSP_PARM_SIZE);
		if (this_unsp->un_flags & UNSP_RIWRITE)
		{
			unsp_send_reply(this_unsp);
		}
		VXSEMA(&filesys_sema, &ss);
		return (u.u_error);	/* can be set by unsp_send_reply() */

	case UNSP_REPLY_CNTL:
		/*Set the RESTARTSYS and/or LONGJMP flags in the next reply*/
		alloc_reply(this_unsp);
		hp = DM_HEADER(this_unsp->un_reply);
		/*set appropriate fields*/
		if (*(int*)data & UNSP_EOSYS_RESTART)
			hp->dm_eosys = RESTARTSYS;
		if (*(int*)data & UNSP_LONGJMP)
			hp->dm_tflags |= DM_LONGJMP;
		VXSEMA(&filesys_sema, &ss);
		return (0);

	default:
		/* unrecognized ioctl command */
		VXSEMA(&filesys_sema, &ss);
		return (EINVAL);
	}
}

/*
 *Close the UNSP file descriptor.  Called either explicitly or implicitly
 *by exiting.  If there are any pending requests, send out a reply.
 */
unsp_close(fp)
struct file *fp;
{

	sv_sema_t ss;
	register struct unsp *this_unsp = (struct unsp *)(fp->f_data);

	PXSEMA(&filesys_sema, &ss);
	/*send a reply with an error if anyone is waiting*/
	if (this_unsp->un_request)
	{
		alloc_reply(this_unsp);
		DM_RETURN_CODE(this_unsp->un_reply) = EIDRM;
		unsp_send_reply(this_unsp);
	}
	/*release the unsp*/
	this_unsp->un_flags = 0;
	this_unsp->un_unique++;	/*change the id number*/
	this_unsp->un_site = 0;
	VXSEMA(&filesys_sema, &ss);
}

/*
 *cause the reply to be sent.  Allocate a reply if necessary
 *If the UNSP_WAKE_REPLY flag is set, wakeup a sleeper on the reply rather
 *than actually sending this back.  (Normally, this flag will be set if
 *and only if the reply was included with the invocation).
 */
unsp_send_reply(this_unsp)
register struct unsp *this_unsp;
{
	/*first make sure there is something to reply to*/
	if (!this_unsp->un_request)
	{
		u.u_error = EIO;
		return;
	}
	/*allocate the reply if none is present*/
	alloc_reply(this_unsp);
	/*set the reply ID*/
	DM_CONVERT(this_unsp->un_reply,struct unsp_message)->um_id =
		this_unsp->un_id;
	DM_HEADER(this_unsp->un_reply)->dm_mid =
		DM_HEADER(this_unsp->un_request)->dm_mid;
	DM_HEADER(this_unsp->un_reply)->dm_dest =
		DM_HEADER(this_unsp->un_request)->dm_srcsite;
	dm_release(this_unsp->un_request,1);
	this_unsp->un_request = NULL;
	/*Wake up any sleepers*/
	if (this_unsp->un_flags & UNSP_WAKE_REPLY)
	{
		wakeup(this_unsp->un_reply);
		(DM_HEADER(this_unsp->un_reply))->dm_flags |= DM_DONE;
	}
	else
		net_reply(this_unsp->un_reply);
	this_unsp->un_reply = NULL;
}

/*
 *Scan the unsp table to determine if there are unsps still running for the 
 *failed site.  If yes, send a signal to kill those processes, mark the site
 *as dead,and return 0.
 *Otherwise, return  1 (no unsp is running)
 *One special case is if the UNSP is scheduled but not yet running.  In this
 *case, just clear the entry.  When the UNSP gets into the kernel it will
 *see that there is nothing to do and return.
 */
no_unsp (crashed_site)
site_t crashed_site;
{
	register struct unsp *unspp;
	register int no_process;
		 
	no_process = 1;

	for (unspp = unsps; unspp < unsps + MAX_UNSP; unspp++)
	{
		if ( (unspp->un_flags & UNSP_IN_USE) && 
		    (unspp->un_site == crashed_site))
		{
			/*found one.  Is it active yet?*/
			if (unspp->un_flags & UNSP_HAS_PROCESS)
			{
				/*active.  Signal it*/
				psignal (unspp->un_proc, SIGUSR1);
				/*mark it dead*/
				unspp->un_flags |= UNSP_DEAD;
				/*wake up any sleepers*/
				if (unspp->un_flags & UNSP_UWAITING)
				{
					wakeup (unspp);
					unspp->un_flags &= ~UNSP_UWAITING;
				}
				/*make sure we don't clean up yet*/
				no_process = 0;	
			}
			else
			{
				/*It's scheduled, but not yet active
				 *Clear the flags, so when the open occurs,
				 *it won't find a request to process.
				 */
				unspp->un_flags = 0;
			}
		}
	}
	return (no_process);
}

/*
** Send a signal to the unsp forker (/etc/init - process 1)
*/
signal_unsp_forker()
{
	psignal(&proc[1], UNSP_SIG);
}

