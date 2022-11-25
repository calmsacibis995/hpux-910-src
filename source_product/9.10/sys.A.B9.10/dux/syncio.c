/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/syncio.c,v $
 * $Revision: 1.9.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:46:20 $
 */

/* HPUX_ID: @(#)syncio.c	55.1		88/12/23 */

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
 *Execute synchronous I/O to a file.  Send the exact data rather than
 *bufferring.  It will be necessary to break the file up into blocks of
 *no larger than MAXDUXBSIZE, in order to guarantee correct block allocation.
 *This routine should only be called with a regular file, or something that
 *looks like it, such as a directory or a fifo.
 *It should not be called for devices.
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/systm.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/unistd.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_dev.h"
#include "../dux/duxfs.h"

#ifdef SYSCALLTRACE
int  lockfdbg;
#endif SYSCALLTRACE

#define EDUXRESTART 0xfe

#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI

#ifdef QUOTA
#include "../ufs/quota.h"
#include "../h/kernel.h"
#endif QUOTA
#include "../h/kern_sem.h"

/*synchronous strategy request*/

/*note:  The following structure contains fields named "this" and "real".
 *If the system call is broken up, "this" will contain information on the
 *individual message, whereas "real" will contain information on the complete
 *system call.
 */

struct syncstratreq		/*DUX MESSAGE STRUCTURE*/
{
	dev_t ss_dev;		/*device*/
	ino_t ss_ino;		/*i number of file*/
	int ss_pid;		/*process ID for lockf*/
	daddr_t ss_this_offset;	/*offset of this request*/
	long ss_this_count;	/*number of bytes this request*/
	daddr_t ss_real_offset;	/*offset by initial sc*/
	long ss_real_count;	/*total bytes requested by sc*/
	int ss_fflag;		/*flags from fd*/
	u_short ss_uid;		/*effective uid*/
	short ss_flags;		/*see below*/
	int ss_ulimit;		/* processes current file ulimit */
	int ss_ioflag;		/* rwip flag used by NFS */
};

/*synchronous strategy response*/

struct syncstratresp		/*DUX MESSAGE STRUCTURE*/
{
	unsigned int ss_resid;	/*bytes not transferred*/
	int ss_version;		/*new version number*/
	int ss_size;		/*new size*/
	short ss_flags;		/*see below*/
};

/*request flags:*/
#define SS_READ 1		/*read request*/
#define SS_UNLOCK 2		/*just force the inode unlocked*/

/*response flags:*/
#define SS_ASYNC 4		/*ok to use async I/O in future*/
#define SS_MORE 8		/*expecting additional request*/
#ifdef QUOTA
#define SS_DQ_OVER_SOFT  16     /*user is over soft limits*/
#define SS_DQ_OVER_HARD  32     /*user is over hard limits*/
#define SS_DQ_OVER_TIME  64     /*user over soft limits and time has expired*/
#endif QUOTA

/*
 *Perform a synchronous strategy request.  If the request is less than
 *8K in size, we can send it in one request.  If it is larger than 8K,
 *we need to break the request into multiple messages.  In between these
 *requests we will need to keep the inode locked at the server in order to
 *guarantee atomicity.
 */
syncio(ip, uio, rw, ioflag)
register struct inode *ip;
register struct uio *uio;
enum uio_rw rw;
register int ioflag;
{
	dev_t dev = ip->i_dev;
	ino_t ino = ip->i_number;
	register struct buf *bp;
	register int type = ip->i_mode & IFMT;
	dm_message request, response;
	register struct syncstratreq *sreqp;
	register struct syncstratresp *sresp;
	register int packetsize;	/*number of bytes to be transferred*/
	int ioresid;			/*number of bytes not transferred*/
	int done;
	register int dm_rcode;
	int send_flags;
	short resp_flags = 0;
#ifdef	FSD_KI
	struct timeval	starttime;

	/* get start time of request */
	KI_getprectime(&starttime);
#endif	FSD_KI

	if (type != IFREG && type != IFDIR && type != IFIFO && type != IFNWK)
	{
		panic ("syncio != IFREG");
	}
	/*allocate the message.  We will reuse the same message to send
	 *each of the packages.  After allocating it, fill in the device,
	 *inode, flags, size of the complete request, etc...
	 */
	request = dm_alloc(sizeof(struct syncstratreq),WAIT);
	sreqp = DM_CONVERT(request,struct syncstratreq);
	bp = geteblk(MIN(uio->uio_resid,MAXDUXBSIZE));
	sreqp->ss_dev = dev;
	sreqp->ss_ino = ino;
	sreqp->ss_flags = (rw == UIO_READ) ? SS_READ : 0;
	sreqp->ss_real_offset = uio->uio_offset;
	sreqp->ss_real_count = uio->uio_resid;
	sreqp->ss_fflag = uio->uio_fpflags;
	sreqp->ss_uid = u.u_uid;
	sreqp->ss_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	sreqp->ss_ioflag = ioflag;
	/*loop as long as there is more data to read or write*/
	done=0;
	while (u.u_error == 0 && uio->uio_resid > 0 && !done)
	{
		/*The packetsize should either be 8K or the number of
		 *remaining bytes, whichever is left.  Calculate it
		 *and fill it in to the message.
		 */
		packetsize = MIN(uio->uio_resid,MAXDUXBSIZE);
		sreqp->ss_this_count = packetsize;
		sreqp->ss_this_offset = uio->uio_offset;
		/*for reading, send the message, and then copyout*/
		if (rw == UIO_READ)
		{
		lockf_retry:
			send_flags = DM_SLEEP | DM_REPLY_BUF;
			/*a fifo can't be locked because the server may sleep
			 *We do want to respond to interrupts for a fifo*/
			if (type == IFIFO)
			{
				iunlock(ip);
				send_flags |= DM_INTERRUPTABLE;
			}
			/* send the read request */
			response = dm_send(request,send_flags,
				DMSYNCSTRAT_READ,devsite(dev),
				sizeof (struct syncstratresp),
				NULL,
				bp,packetsize,0,
				NULL, NULL, NULL);
			/* relock the fifo */
			if (type == IFIFO)
				ilock(ip);
			if ((dm_rcode = DM_RETURN_CODE(response)) != 0)
			{
				dm_release(response, 0);
				if (dm_rcode == EDUXRESTART)
				{
					/*  Have to unlock inode here */
					if (ilocked(ip))
						iunlock(ip);

					/*  Set the forwarding address	*/
					u.u_procp->p_faddr = devsite(dev);

					response = dm_send(request,
							DM_SLEEP|DM_REPEATABLE|
							   DM_INTERRUPTABLE,
							DM_LOCKWAIT,
							devsite(dev), DM_EMPTY,
							NULL, NULL, NULL, NULL,
							NULL, NULL, NULL);

					/*  Reset the forward address	*/
					u.u_procp->p_faddr = 0;

					/*  Re-lock the inode		*/
					ilock(ip);

					dm_rcode = DM_RETURN_CODE(response);
					dm_release(response, 0);

					if(dm_rcode == 0)
						goto lockf_retry;
				}
				u.u_error = dm_rcode;
				break;	/*from while*/
			}
			/*determine how much data was actually read*/
			sresp = DM_CONVERT(response, struct syncstratresp);
			ioresid=sresp->ss_resid;
			/* copy the data to user space */
			u.u_error = uiomove (bp->b_un.b_addr,
				packetsize-ioresid, UIO_READ, uio);
			if (u.u_error)
			{
				/* If we had an error and the server is
				 * holding the inode locked, send a followup
				 * message.  If this was the last message,
				 * we will just fall through and quit anyway.
				 */
				if (sresp->ss_flags & SS_MORE)
				{
					dm_release(response,0);
					goto send_unlock;
				}
			}
		}
		/* for writing first copyin, then send the message*/
		else
		{
			/* copy in the data */
			u.u_error = uiomove (bp->b_un.b_addr, packetsize,
				UIO_WRITE, uio);
			if (u.u_error)
			{
				if (sreqp->ss_real_offset !=
					sreqp->ss_this_offset)
					/*this wasn't the first packet*/
					goto send_unlock;
				break;
			}
			send_flags = DM_SLEEP | DM_REQUEST_BUF;
		lockf_wretry:
			/*a fifo can't be locked because the server may sleep
			 *We do want to respond to interrupts for a fifo*/
			if (type == IFIFO)
			{
				iunlock(ip);
				send_flags |= DM_INTERRUPTABLE;
			}
			/* send the request */
			response = dm_send(request,send_flags,
				DMSYNCSTRAT_WRITE,devsite(dev),
				sizeof (struct syncstratresp),
				NULL,
				bp,packetsize,0,
				NULL, NULL, NULL);
			/* relock the fifo */
			if (type == IFIFO)
				ilock(ip);
			if ((dm_rcode = DM_RETURN_CODE(response)) != 0)
			{
				dm_release(response, 0);
				if (dm_rcode == EDUXRESTART)
				{
					/*  Have to unlock inode here */
					if (ilocked(ip))
						iunlock(ip);

					/*  Set the forwarding address	*/
					u.u_procp->p_faddr = devsite(dev);

					response = dm_send(request,
							DM_SLEEP|DM_REPEATABLE|
							   DM_INTERRUPTABLE,
							DM_LOCKWAIT,
							devsite(dev), DM_EMPTY,
							NULL, NULL, NULL, NULL,
							NULL, NULL, NULL);

					/*  Reset the forward address	*/
					u.u_procp->p_faddr = 0;

					/*  Re-lock the inode		*/
					ilock(ip);

					dm_rcode = DM_RETURN_CODE(response);
					dm_release(response, 0);

					if (dm_rcode == 0)
						goto lockf_wretry;
				}
				u.u_error = dm_rcode;
				uio->uio_resid += packetsize;
				uio->uio_offset -= packetsize;
				break;	/*from while*/
			}
			/*determine how much data was actually written*/
			sresp = DM_CONVERT(response, struct syncstratresp);
			ioresid=sresp->ss_resid;
			if (ioresid != 0)
			{
				uio->uio_resid += ioresid;
				uio->uio_offset -= ioresid;
					/*we want to know what we really wrote*/
			}
		}
		/* The message may indicate that we no longer need sync I/O.
		 * If so, clear the flag and fill in the new size and version
		 * number.
		 */
		if (sresp->ss_flags & SS_ASYNC)
		{
			ip->i_flag &= ~ISYNC;
			ip->i_size = sresp->ss_size;
			ip->i_fversion = sresp->ss_version;
		}
		/* If the server says no more data, believe it */
		if (!(sresp->ss_flags & SS_MORE))
			done=1;
		resp_flags = sresp->ss_flags;
		dm_release(response,0);
	}
#ifdef	FSD_KI
	KI_servsyncio(dev, rw, sreqp->ss_real_count - uio->uio_resid, &starttime);
#endif	FSD_KI
	/* We are done with the request.  Clean up */
	dm_release(request,0);
	brelse (bp);
	if (u.u_error == ENOSPC)
		uprintf("\n%s: write failed, file system is full\n",
			ip->i_dfs->dfs_fsmnt);
#ifdef QUOTA
        if (ip->i_dquot && (ip->i_dquot->dq_uid != 0))
        {
           if (u.u_error == EDQUOT)
           {
              if (resp_flags & SS_DQ_OVER_HARD)
              {
                 if (ip->i_uid == u.u_ruid) 
                 {
			uprintf("\nDISK LIMIT REACHED (%s) - WRITE FAILED\n",
			   ip->i_dfs->dfs_fsmnt);
                 }
              }
              else /* must be over soft limit and time has expired */
                 if (ip->i_uid == u.u_ruid) 
                 {
        	    uprintf("\nOVER DISK QUOTA: (%s) NO MORE DISK SPACE\n",
		             ip->i_dfs->dfs_fsmnt);
                 }
	   }
           else  /* no error but may need a warning for over soft limits */
	   {
              if (resp_flags & SS_DQ_OVER_SOFT)
	      {
                 if ((time.tv_sec - ip->i_dquot->dq_btimelimit > DQ_MSG_TIME)
                      && ip->i_uid == u.u_ruid)
                 {
  		    uprintf("\nWARNING: disk quota (%s) exceeded\n",
			    ip->i_dfs->dfs_fsmnt);
                    ip->i_dquot->dq_btimelimit = time.tv_sec;
	         }
	      }
              else
                 /* clear warning issued time since under limits again */
                 ip->i_dquot->dq_btimelimit = 0;
	   }

           ip->i_dquot->dq_flags = (resp_flags & SS_DQ_OVER_SOFT) ?
                                    ip->i_dquot->dq_flags | DQ_SOFT_BLKS :
                                    ip->i_dquot->dq_flags & ~DQ_SOFT_BLKS;
           ip->i_dquot->dq_flags = (resp_flags & SS_DQ_OVER_HARD) ?
                                    ip->i_dquot->dq_flags | DQ_HARD_BLKS :
                                    ip->i_dquot->dq_flags & ~DQ_HARD_BLKS;
           ip->i_dquot->dq_flags = (resp_flags & SS_DQ_OVER_TIME) ?
                                    ip->i_dquot->dq_flags | DQ_TIME_BLKS :
                                    ip->i_dquot->dq_flags & ~DQ_TIME_BLKS;

        }
#endif QUOTA
	return (u.u_error);

	/* If we had a local error in the middle of a multimessage request,
	 * the server still holds the inode locked.  Send a special message
	 * telling it to unlock the inode.
	 */
send_unlock:
	sreqp->ss_flags = SS_UNLOCK;
	dm_send(request,DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
		DMSYNCSTRAT_READ,devsite(dev), DM_EMPTY,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	brelse(bp);
	return (u.u_error);
}

/*
 *Strategy routine for synchronous I/O.  This routine is to a large extent
 *a front end to rdwri.
 */
servestratsync(request)
dm_message request;
{
	register struct syncstratreq *sreqp =
		DM_CONVERT(request,struct syncstratreq);
	dev_t dev = localdev(sreqp->ss_dev);
	register int reading = sreqp->ss_flags & SS_READ;
	register struct buf *bp;
	dm_message response;
	struct syncstratresp *sresp;
	register struct inode *ip;
	register int type;
	int error;
	int resid;
	int offset;
	int save_ulimit;
	struct uio auio;
	struct iovec aiov;
	struct flock flock;
	label_t lqsave;
	register struct ucred *tcred1, *tcred2;

	/* First find the inode */
	ip = ifind(dev,sreqp->ss_ino);
	if (ip == NULL)
	{
		printf ("ifind failed: servstratsync\n");
		dm_quick_reply(EIO);
		return;
	}
	/* If this is the first message in the request, we need to lock the
	 * inode.  If not, we already have the inode locked.
	 */
	if (sreqp->ss_this_offset == sreqp->ss_real_offset)
	{   /*  For regular files, if enforcement mode is set and there
             *	are locks in the way, force the requesting process to
             *	wait for the lock to clear.  
	     *  This extra step is necessary to prevent the process from
	     *  blocking on another site while holding the inode lock 
	     *  from this site.
	     */
	    type = ip->i_mode & IFMT;
	    if((type == IFREG) && 
	       (ip->i_mode & IENFMT) && !(ip->i_mode & (IEXEC >> 3)) &&
	       (locked(F_TEST, ip, sreqp->ss_real_offset,
		      sreqp->ss_real_offset+sreqp->ss_real_count,
		      reading ? L_READ : L_WRITE, NULL, flock.l_type, 0)))
	      {
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf ("servestratsync: set EDUXRESTART\n");
#endif SYSCALLTRACE
		dm_quick_reply(EDUXRESTART);
		return;
	      }
	    use_dev_dvp(ip->i_devvp);
	    ilock(ip);
	}
	/* If this message was merely an unlock followup, just unlock the
	 * inode and return.
	 */
	if (sreqp->ss_flags & SS_UNLOCK)
	{
		iunlock(ip);
		drop_dev_dvp(ip->i_devvp);
		dm_quick_reply(0);
		return;
	}
	/* Copy in the uid.  This is used for writes beyond the 10% boundary */
	tcred2 = u.u_cred;
	tcred1 = crdup(tcred2);
	tcred1->cr_uid = sreqp->ss_uid;
	u.u_cred = tcred1;
	/* set the correct ulimit for the file */
	save_ulimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = sreqp->ss_ulimit;
	/* If we're reading get a buffer for the reply.  If we are writing,
	 * the buffer was passed to us.
	 */
	if (reading)
	{
		bp = geteblk(sreqp->ss_this_count);
	}
	else
	{
		bp = DM_BUF(request);
	}
	/*calculate the offset in the file.  Normally, it is the passed offset.
	 *However, if we are writing, and the append flag is set, it is actually
	 *the file size.
	 *NOTE:  doing things in exactly this way introduces a small
	 *inconsistancy with the mechanism in the local system, since the offset
	 *in the file descriptor is not updated to match the new file size.
	 *However, since that offset is not guaranteed to be the offset at which
	 *a write will take place (since someone else could append to the file
	 *in the mean time, this should not break any programs.
	 */
	if (!reading && (sreqp->ss_fflag&FAPPEND) && (ip->i_mode&IFMT) == IFREG)
		offset = ip->i_size;
	else
		offset = sreqp->ss_this_offset;
	/* Perform the I/O */
	/*unfortunately, rdwri has been modified to call ufs_rdwr which locks
	 *the vnode itself.  Thus, we need to make up the io vector ourself.
	 */
	aiov.iov_base = bp->b_un.b_addr;
	auio.uio_resid = aiov.iov_len = sreqp->ss_this_count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_seg = UIOSEG_KERNEL;
	auio.uio_fpflags = sreqp->ss_fflag;
	/* this setjmp is needed because fifo's can sleep at an 
	   interruptable priority.  so if rwip() receives an interrupt
	   it would bypass this code and not release the resources
	   gotten by this routine.
	*/
	lqsave = u.u_qsave;
	if (setjmp(&u.u_qsave)) {
		PSEMA(&filesys_sema);
		error = EINTR;
		if (auio.uio_resid ==  sreqp->ss_this_count)
			u.u_eosys = RESTARTSYS;
	} else {
	    if (!(sreqp->ss_ioflag & IO_SYNC))
			sreqp->ss_ioflag |= IO_SYNC;
	    error=rwip(ip, &auio, reading?UIO_READ:UIO_WRITE, sreqp->ss_ioflag);
	}
	u.u_qsave = lqsave;
	resid = auio.uio_resid;
	/* Reset the uid to 0 for the next request the NSP processes */
	u.u_cred = tcred2;
	crfree(tcred1);
	/* restore the correct ulimit for the next NSP process */
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = save_ulimit;
	/* Send back the response */
	response = dm_alloc(sizeof(struct syncstratresp),WAIT);
	sresp = DM_CONVERT(response,struct syncstratresp);
	sresp->ss_resid = resid;
	if (resid < 0)
	{
		printf ("resid=%x\n",resid);
		panic ("resid");
	}
	sresp->ss_flags=0;
	/* If synchronous I/O is no longer required, we still process the
	 * request.  However, piggyback a flag stating that asynchronous I/O
	 * can be used.  Also include the new size and versionB number.
	 */
	if (!(ip->i_flag&ISYNC))
	{
		sresp->ss_flags = SS_ASYNC;
		sresp->ss_size = ip->i_size;
		sresp->ss_version = ip->i_fversion;
	}
#ifdef QUOTA
        if (ip->i_dquot)
        {
           sresp->ss_flags = (ip->i_dquot->dq_flags & DQ_SOFT_BLKS) ?
                              sresp->ss_flags | SS_DQ_OVER_SOFT :
                              sresp->ss_flags & ~SS_DQ_OVER_SOFT;
           sresp->ss_flags = (ip->i_dquot->dq_flags & DQ_HARD_BLKS) ?
                              sresp->ss_flags | SS_DQ_OVER_HARD :
                              sresp->ss_flags & ~SS_DQ_OVER_HARD;
           sresp->ss_flags = (ip->i_dquot->dq_flags & DQ_TIME_BLKS) ?
                              sresp->ss_flags | SS_DQ_OVER_TIME :
                              sresp->ss_flags & ~SS_DQ_OVER_TIME;
         }
#endif QUOTA

	/* If this is the last request, unlock the inode.  Otherwise, tell
	 * the client that we can accept more data.
	 */
	if ((sreqp->ss_this_offset + sreqp->ss_this_count ==
		sreqp->ss_real_offset + sreqp->ss_real_count ||
		resid || error) && error != EINTR)
	{
		iunlock(ip);
		drop_dev_dvp(ip->i_devvp);
	}
	else
		sresp->ss_flags |= SS_MORE;
	/*send the reply*/
	if (reading)
	{
		dm_reply(response,DM_REPLY_BUF,error,bp,sreqp->ss_this_count,0);
	}
	else
	{
		dm_reply(response,0,error, NULL, NULL, NULL);
	}
	/*note that the messages are automatically released*/
}

dux_lockwait(reqp)
  dm_message reqp;
  { register struct syncstratreq *sreqp =
				DM_CONVERT(reqp, struct syncstratreq);
    register dev_t dev = localdev(sreqp->ss_dev);
    register struct inode *ip;
    struct flock flock;

    /* First find the inode */
    ip = ifind(dev, sreqp->ss_ino);
    if(ip == NULL)
      { 
#ifdef SYSCALLTRACE
		if (lockfdbg)
			printf ("dux_lockwait: ifind failed\n");
#endif SYSCALLTRACE
	dm_quick_reply(EIO);
	return;
      }

    /*  Wait for the lock that would have blocked the read/write 
     *	initially, leaving the inode locked at the calling site.  
     */
#ifdef SYSCALLTRACE
		if (lockfdbg)
    			printf ("dux_lockwait: call locked to wait for the lock that blocked us\n");
#endif SYSCALLTRACE

    if(locked(F_LOCK, ip, sreqp->ss_real_offset,
		      sreqp->ss_real_offset+sreqp->ss_real_count,
		      L_LOCKF, NULL, flock.l_type, 0))
      dm_quick_reply(u.u_error);
    else
      dm_quick_reply(0);
  }
