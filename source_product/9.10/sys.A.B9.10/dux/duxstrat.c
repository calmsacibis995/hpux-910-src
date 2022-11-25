/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/duxstrat.c,v $
 * $Revision: 1.8.83.5 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/01/05 15:51:44 $
 */

/* HPUX_ID: @(#)duxstrat.c	55.1		88/12/23 */

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
 *duxstrat.c
 *
 *This file contains the code for sending and processing remote strategy
 *requests.  These are requests which are used to process logical blocks.
 *
 *This file contains both the code for the requesting site and the serving
 *site.
 *
 *note:  Block operations directly to the disc (as opposed to the file system)
 *use the regular (local) block I/O.
 */

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/systm.h"
#include "../h/vnode.h"
#include "../h/pfdat.h"
#include "../h/uio.h"
#include "../rpc/types.h"
#include "../h/mount.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_dev.h"
#include "../dux/dux_lookup.h"
#include "../dux/lookupmsgs.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs_hooks.h"

#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA

extern struct buf *breadan();

#ifdef QUOTA
#define NS_DEVSYNC	1	/*device should be using sync writes*/
#define NS_DQ_OVER_SOFT	2	/*user is over the soft block limit*/
#define NS_DQ_OVER_HARD 4	/*user is over the hard block limit*/
#define NS_DQ_OVER_TIME 8	/*over soft limit and time expired*/
#endif QUOTA

#define NS_DEVSYNC	1	/*device should be using sync writes*/

/*
 * How much read ahead when server is reading off disc?
 * We now have the power to do a lot, possibly benefitting from something
 * like setup_readahead(), but not the time to know what is appropriate,
 * so just hard-code to 2, which mimics past behaviour.
 */
#define DUXSERVE_READAHEAD   2

/*
 *dux_strategy is called to read or write a block from the device.  It
 *translates it into a dm message and then sends it off.  It returns
 *immediately, since reading and writing from block devices should be
 *asynchronous.
 */

dux_strategy(bp)
register struct buf *bp; 
{ 
	register dm_message message;
	register struct duxstratreq *sp;
	register int reading;
	register struct inode *ip;
	extern int duxint();	/*forward*/
	register struct cdnode *cdp;
	struct	vnode	*vp=bp->b_vp;
	site_t	site;

	VASSERT((bp->b_flags & B_ORDWRI) == 0);
	VASSERT((bp->b_flags2 & B2_ORDMASK) == 0);
	bp->b_resid=0;	/*initialize it to be on the safe side*/
	/*Allocate a dm message*/
	message = dm_alloc(sizeof(struct duxstratreq),WAIT);
	/*Convert it to a dux strategy request*/
	sp = DM_CONVERT(message,struct duxstratreq);
	/*set the appropriate fields in the message*/
	reading = bp->b_flags & B_READ;
	if (vp->v_fstype == VDUX) {
		ip = (VTOI(vp));	/*get the inode */
		site = devsite(ip->i_dev);
		sp->ns_dev = ip->i_dev;
		sp->ns_ino = ip->i_number;
		sp->ns_fstype = MOUNT_UFS;

        	/* If we are doing a synchronous write, send along the uid field
         	*  If we are not, send 0.  The reason that the UID is needed is
         	*  for writes beyond the 10% reserve.  Only the superuser should
         	*  be permitted to perform these writes.
         	*/
        	sp->ns_uid = (bp->b_flags & B_SYNC)
#ifdef QUOTA
                   || (ip->i_dquot &&
                       ip->i_dquot->dq_flags & (DQ_HARD_BLKS | DQ_TIME_BLKS))
#endif QUOTA
                   ? u.u_uid : 0;
	}
	else {		/*VDUX_CDFS*/
		cdp = (VTOCD(vp));	/*get the inode */
		site = devsite(cdp->cd_dev);
		sp->ns_dev = cdp->cd_dev;
		sp->ns_ino = cdp->cd_num;
		sp->ns_fstype = MOUNT_CDFS;
		sp->ns_uid = u.u_uid;
	}
	sp->ns_flags = bp->b_flags;
	sp->ns_count = bp->b_bcount;
		/*XXX Check for dead site*/
	sp->ns_offset = bp->b_blkno<<DEV_BSHIFT;

	/*send the request*/
	if (reading)
	{
		dm_send(message, DM_FUNC|DM_REPEATABLE|DM_REPLY_BUF,
			DMNETSTRAT_READ, site,
			sizeof(struct duxstratresp), duxint,
			bp, bp->b_bcount, 0, NULL, NULL, NULL );
	}
	else	/*writing*/
	{
		dm_send(message, DM_FUNC|DM_REPEATABLE|DM_REQUEST_BUF,
			DMNETSTRAT_WRITE, site,
			sizeof(struct duxstratresp), duxint,
			bp, bp->b_bcount, 0, NULL, NULL, NULL);
	}
	/*return immediately*/
	return (0);
} 

/*
 *duxint is called under interrupt when the response to a strategy request
 *comes in.  It completes the I/O, and releases the buffers.
 */
duxint(request,response)
dm_message request, response;
{
	register struct duxstratreq *reqsp;
	register struct duxstratresp *respsp;
	register int reading;
	register struct buf *bp;
	dev_t dev;
#ifdef QUOTA
        struct dquot *dquotp;
#endif QUOTA

	/*convert the request and the response to the appropriate structures*/
	reqsp = DM_CONVERT(request,struct duxstratreq);
	respsp = DM_CONVERT(response,struct duxstratresp);
	reading = reqsp->ns_flags & B_READ;
	/*get the buffer pointer*/
	if (reading)
	{
		bp = DM_BUF(response);
	}
	else
	{
		bp = DM_BUF(request);
	}
	bp->b_resid = respsp->ns_resid;
	/*check for errors*/
	if ((bp->b_error = DM_RETURN_CODE(response)) != 0) /*an error occurred*/
	{
#ifdef QUOTA
            if (bp->b_error == EDQUOT) 
            {
               if (bp->b_vp->v_fstype == VDUX) {
                  if (dquotp = (VTOI(bp->b_vp))->i_dquot)
	          {
                    /* set clients flags for hard and time limits to those */
                    /* of server so that we know when to do sync writes.   */
                    dquotp->dq_flags = (respsp->ns_flags & NS_DQ_OVER_SOFT) ?
                                        dquotp->dq_flags | DQ_SOFT_BLKS :
                                        dquotp->dq_flags & ~DQ_SOFT_BLKS;
                    dquotp->dq_flags = (respsp->ns_flags & NS_DQ_OVER_HARD) ?
                                        dquotp->dq_flags | DQ_HARD_BLKS :
                                        dquotp->dq_flags & ~DQ_HARD_BLKS;
                    dquotp->dq_flags = (respsp->ns_flags & NS_DQ_OVER_TIME) ?
                                        dquotp->dq_flags | DQ_TIME_BLKS :
                                        dquotp->dq_flags & ~DQ_TIME_BLKS;

                  }
               }
            }
#endif QUOTA
		bp->b_flags |= B_ERROR;
	}
	else	/*check if device sync status has changed*/
	{
	    if(bp->b_vp->v_fstype == VDUX) {
#ifdef QUOTA
                if (dquotp = (VTOI(bp->b_vp))->i_dquot)

                {
                  dquotp->dq_flags = (respsp->ns_flags & NS_DQ_OVER_SOFT) ?
                                      dquotp->dq_flags | DQ_SOFT_BLKS :
                                      dquotp->dq_flags & ~DQ_SOFT_BLKS;
                  dquotp->dq_flags = (respsp->ns_flags & NS_DQ_OVER_HARD) ?
                                      dquotp->dq_flags | DQ_HARD_BLKS :
                                      dquotp->dq_flags & ~DQ_HARD_BLKS;
                  dquotp->dq_flags = (respsp->ns_flags & NS_DQ_OVER_TIME) ?
                                      dquotp->dq_flags | DQ_TIME_BLKS :
                                      dquotp->dq_flags & ~DQ_TIME_BLKS;

                  /* If under soft limit, clear last-message time */
                  if (((respsp->ns_flags & NS_DQ_OVER_SOFT) == 0) &&
		      (dquotp->dq_uid != 0))
                     dquotp->dq_btimelimit = 0;

                }
#endif QUOTA
		dev = (VTOI(bp->b_vp))->i_dev;
		if ((bp->b_flags & B_SYNC) && !(respsp->ns_flags & NS_DEVSYNC))	
		{	/*turn off sync*/
			register struct mount *mp = getmount (dev);

			mp->m_flag &= ~M_IS_SYNC;
		}
		else if (!(bp->b_flags & B_SYNC) && 
			(respsp->ns_flags & NS_DEVSYNC))	
		{	/*may need to turn on sync*/
			register struct mount *mp = getmount (dev);

			if (!(mp->m_flag & M_IS_SYNC))
				sched_turn_on_devsync(mp);
		}
	     }
		/* if we are reading, zero out the portion of the buffer
		 * which was not actually read */
		if (reading && bp->b_resid) {
#ifdef	hp9000s800
			privlbzero (bvtospace(bp, bp->b_un.b_addr),
#else	/* s300 */
			bzero (
#endif
				bp->b_un.b_addr+bp->b_bcount-bp->b_resid,
				bp->b_resid);
			bp->b_resid = 0;
		}
	}
	/*complete the I/O*/
	iodone(bp);
	/*release the messages*/
	dm_release(request,0);
	dm_release(response,0);
}

/*
 *The following routines serve the appropriate requests on the serving site.
 *Both the read and write are subject to the following requirements:
 *	-The requested block is contained within a single disc block.
 *	-Asynchronous I/O is being used on the file (so lockf checks need
 *		not be made, and so the function can be used with pipes).
 */

/*
 *This function will perform a read on the serving site.  The steps are:
 *
 *	1) Find the inode.
 *
 *	2) Figure out the block that must be read (there can be no more than
 *		one)
 *
 *	3) Read the block (from disc or core and with a possible readahead)
 *
 *	4) Send the block back.  It may be that only part of the block
 *		has been requested in which case only that part should
 *		be sent.
 */

servestratread(request)
dm_message request;
{
	register struct duxstratreq *sreqp =
		DM_CONVERT(request,struct duxstratreq);
	if (sreqp->ns_fstype == MOUNT_UFS) servestratreadi(sreqp);
	else if (sreqp->ns_fstype == MOUNT_CDFS) CDFSCALL(SERVESTRATREADCD)(sreqp);
	else panic("servestratread: illegal fs type");
}
servestratreadi(sreqp)
register struct duxstratreq *sreqp;
{
	register struct inode *ip;
	dm_message response;
	register struct duxstratresp *srespp;
	register long offset;
	register long length;
	register long start;
	dev_t dev = localdev(sreqp->ns_dev);
	daddr_t bn, lbn;
	struct buf *bp;
	struct fs *fs;
	long bsize;
	long resid;
	long size;
	daddr_t ra_block[DUXSERVE_READAHEAD];
	int ra_size[3];
#ifdef	FSD_KI
	struct timeval	starttime;

	/* get start time of request */
	KI_getprectime(&starttime);
#endif	FSD_KI

	/*look up and lock the inode*/
	ip = ifind(dev,sreqp->ns_ino);
	if (ip == NULL)
	{
		printf ("ifind failed: servestratread\n");
#ifdef OSDEBUG
                printf("servestratread: dev=%x and inode=%d.\n",dev,sreqp->ns_ino);
#endif
		dm_quick_reply(EIO);
		return;
	}
	use_dev_dvp(ip->i_devvp);
	ilock(ip);

	/*make sure that there is data here*/
	start = sreqp->ns_offset;
	length = sreqp->ns_count;
	resid = 0;
	if (start + length > ip->i_size)	/*shouldn't happen*/
	{
		resid = (start + length) - ip->i_size;
		length -= resid;
		if (length <= 0)
		{
			resid = sreqp->ns_count;
			length = 0;
			bp = NULL;
			goto send_reply;
		}
	}

	/*
	 * For fast symlinks, we intercept the read request and
	 * grab the data directly from the inode.  The above code
	 * has already ensured that length is <= ip->i_size.
	 */
	if ((ip->i_mode & IFMT) == IFLNK &&
	    (ip->i_flags & IC_FASTLINK) != 0)
	{
		if (start != 0)
		{
		    printf("servestratreadi: start is %d\n", start);
		    panic("servestratreadi: bad symlink start addr");
		}
		bp = geteblk(length);
		bp->b_resid = 0;
		strncpy(bp->b_un.b_addr, ip->i_symlink, length);
		offset = 0;
		goto send_reply;
	}

	/*read in the appropriate block from the disc*/
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	lbn = start / bsize;
	offset = start % bsize;
	if (bsize < offset + length)	/*doesn't fit in one block*/
	{
		panic("servestratread");
	}
	if (ip->i_lastr + 1 == lbn)
		ra_size[0] = DUXSERVE_READAHEAD;
	else
		ra_size[0] = 0;
	ip->i_lastr = lbn;
	bn = fsbtodb(fs, bmap(ip,lbn,B_READ,offset+length,0,ra_block,ra_size));
	if (u.u_error)
	{
		iunlock(ip);
		drop_dev_dvp(ip->i_devvp);
		dm_quick_reply(u.u_error);
		return;
	}
	size = blksize(fs, ip, lbn);
	/*make a vnode*/
	/*read (or otherwise get into memory) the block*/
	if ((long)bn < 0)	/*is this possible?*/
	{
		bp = geteblk(size);
		clrbuf(bp);
	} else {
#ifdef	FSD_KI
		bp = breadan(ip->i_devvp, bn, size, ra_block, ra_size, 
			B_servestratread|B_unknown);
#else	FSD_KI
		bp = breadan(ip->i_devvp, bn, size, ra_block, ra_size);
#endif	FSD_KI
		resid += bp->b_resid;
	}
	imark(ip, IACC);
#ifdef	FSD_KI
	KI_servestrat(bp, length-resid, &starttime);
#endif	FSD_KI
	/*release the vnode*/
	/*send back the reply*/
	if (bp->b_flags & B_ERROR)
	{
		iunlock(ip);
		drop_dev_dvp(ip->i_devvp);
		dm_quick_reply(geterror(bp));
		brelse(bp);
		return;
	}
send_reply:
	response = dm_alloc(sizeof(struct duxstratresp),WAIT);
	srespp = DM_CONVERT(response,struct duxstratresp);
	srespp->ns_resid = resid;
	srespp->ns_flags = 0;
	iunlock(ip);
	drop_dev_dvp(ip->i_devvp);
	dm_reply(response,DM_REPLY_BUF,0,bp,length,offset);
	/*reply will release bp*/
}

/*
 *This function will perform a write on the serving site.  The steps are:
 *
 *	1) Find the inode.
 *
 *	2) Figure out the block that must be written (there can be no more than
 *		one)
 *
 *	3) Get the block into memory.
 *
 *	4) If only part of the block is being written, copy the data onto it.
 *		If the complete block is being written, switch the pointers
 *		between the network buffer and the disc buffer.
 *
 *	5) Write the block to disc (may be a delayed write).
 *
 *	6) Send back the reply.
 */

servestratwrite(request)
dm_message request;
{
	register struct duxstratreq *sreqp =
		DM_CONVERT(request,struct duxstratreq);
	register struct inode *ip;
	dm_message response;
	register struct duxstratresp *srespp;
	register long offset;
	register long length;
	register long start;
	dev_t dev = localdev(sreqp->ns_dev);
	daddr_t bn, lbn;
	register struct buf *bp;
	register struct buf *reqbp;
	struct fs *fs;
	long bsize;
	long size;
	long resid=0;
	register struct ucred *tcred1, *tcred2;
	int save_uerror;
#ifdef	FSD_KI
	struct timeval	starttime;

	/* get start time of request */
	KI_getprectime(&starttime);
#endif	FSD_KI

	/*look up and lock the inode*/
	ip = ifind(dev,sreqp->ns_ino);
	if (ip == NULL)
	{
		printf ("ifind failed: servestratwrite\n");
		dm_quick_reply(EIO);
		return;
	}
	use_dev_dvp(ip->i_devvp);
	tcred2 = u.u_cred;
	tcred1 = crdup(tcred2);
	tcred1->cr_uid = sreqp->ns_uid;
	u.u_cred = tcred1;
	ilock(ip);
	/*determine what we are writing.*/
	start = sreqp->ns_offset;
	length = sreqp->ns_count;
	/*select the appropriate block from the disc*/
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	lbn = start / bsize;
	offset = start % bsize;
	if (bsize < offset + length)	/*doesn't fit in one block*/
	{
		panic("servestratwrite");
	}
	bn = fsbtodb(fs,
		bmap(ip,lbn,B_WRITE,offset+length,0,0,0));
	if (u.u_error)
	{
		iunlock(ip);
		drop_dev_dvp(ip->i_devvp);
		u.u_cred = tcred2;
		crfree(tcred1);
#ifdef QUOTA
                if (u.u_error == EDQUOT && ip->i_dquot)
                {
		   short saveflags = ip->i_dquot->dq_flags; /* inode might change */
         	   response = dm_alloc(sizeof(struct duxstratresp),WAIT);
		   srespp = DM_CONVERT(response,struct duxstratresp);
		   
		   srespp->ns_flags = (saveflags & DQ_SOFT_BLKS) ?
		                       NS_DQ_OVER_SOFT :
               		               0;
                   srespp->ns_flags = (saveflags & DQ_HARD_BLKS) ?
                                       srespp->ns_flags | NS_DQ_OVER_HARD :
                                       srespp->ns_flags & ~NS_DQ_OVER_HARD;
                   srespp->ns_flags = (saveflags & DQ_TIME_BLKS) ?
                                       srespp->ns_flags | NS_DQ_OVER_TIME :
                                       srespp->ns_flags & ~NS_DQ_OVER_TIME;
                   dm_reply(response, 0, u.u_error, NULL, NULL, NULL);
                }
                else
#endif QUOTA
   		   dm_quick_reply(u.u_error);
		return;
	}
	if (start + length > ip->i_size)
	{
		ip->i_size = start + length;
	}
	imark(ip, IUPD|ICHG);
	size = blksize(fs, ip, lbn);
	/* Invalidate the associated text pages */
	{
		int i, count;

		count = howmany(size, DEV_BSIZE);
		for (i = 0; i < count; i += NBPG/DEV_BSIZE)
			munhash(ip->i_devvp, (daddr_t)(bn + i));
	}
	/*make a vnode*/
	/* Get the block into memory*/
	if (length == bsize)
#ifdef	FSD_KI
		bp = getblk(ip->i_devvp,bn,size, 
			B_servestratwrite|B_unknown);
#else	FSD_KI
		bp = getblk(ip->i_devvp,bn,size);
#endif	FSD_KI
	else
#ifdef	FSD_KI
		bp = bread(ip->i_devvp,bn,size, 
			B_servestratwrite|B_unknown);
#else	FSD_KI
		bp = bread(ip->i_devvp,bn,size);
#endif	FSD_KI
	if (bp->b_flags & B_ERROR)
	{
		iunlock(ip);
		drop_dev_dvp(ip->i_devvp);
		u.u_cred = tcred2;
		crfree(tcred1);
		dm_quick_reply(geterror(bp));
		brelse(bp);
		return;
	}
	/*
	 *Transfer the data into the buffer.  This may be done by use of a
	 *copy, or, if the buffer sizes match exactly, by switching the
	 *pointers to the buffers.
	 */
	reqbp = DM_BUF(request);

	if ((length != size) || (dux_swap_buf (reqbp, bp) == 0))
	{
		bcopy(reqbp->b_un.b_addr,bp->b_un.b_addr+offset,length);
	}
	/*do the write of the appropriate type*/
	if ((sreqp->ns_flags & B_DELWRI) && !(sreqp->ns_flags & B_SYNC))
		bdwrite (bp);
	else if (sreqp->ns_flags & B_ASYNC)
		bawrite(bp);
	else
	{
		bwrite(bp);
		resid = bp->b_resid;
	}
	/*
	 * Turn off ISUID, ISGID, ISVTX bits iff:  not
	 * a directory, not enforcement mode locking,
	 * and I'm not the owner or root.
	 */
	if ((ip->i_mode & IFMT) != IFDIR) {
		if ((ip->i_mode & (ISUID|ISGID|ISVTX)) != 0) {
			if ((ip->i_mode & (ISUID|ISGID|(IEXEC>>3))) != ISGID) {
				/* no enforcement mode locking */
				save_uerror = u.u_error; /* OWNER_CR may reset
							    u_error */
				if ((OWNER_CR(u.u_cred, ip)) != 0) {
					/* not owner or root */
					ip->i_mode &= ~(ISUID|ISGID|ISVTX);
					imark(ip, ICHG);
				}
				u.u_error = save_uerror;
			}
		}
	}
	u.u_error = 0;	/* KLUDGE:  OWNER_CR may have called suser(), which
			   can set u_error */
#ifdef	FSD_KI
	KI_servestrat(bp, length-resid, &starttime);
#endif	FSD_KI
	iunlock(ip);
	drop_dev_dvp(ip->i_devvp);
	u.u_cred = tcred2;
	crfree(tcred1);
	/*release the vnode*/
	/*send back the reply*/
	response = dm_alloc(sizeof(struct duxstratresp),WAIT);
	srespp = DM_CONVERT(response,struct duxstratresp);
	srespp->ns_resid = resid;
	srespp->ns_flags = (ip->i_mount->m_flag&M_IS_SYNC) ? NS_DEVSYNC : 0;
#ifdef QUOTA
        if (ip->i_dquot)
        {
           srespp->ns_flags = (ip->i_dquot->dq_flags & DQ_SOFT_BLKS) ?
                               srespp->ns_flags | NS_DQ_OVER_SOFT :
                               srespp->ns_flags & ~NS_DQ_OVER_SOFT;
           srespp->ns_flags = (ip->i_dquot->dq_flags & DQ_HARD_BLKS) ?
                               srespp->ns_flags | NS_DQ_OVER_HARD :
                               srespp->ns_flags & ~NS_DQ_OVER_HARD;
           srespp->ns_flags = (ip->i_dquot->dq_flags & DQ_TIME_BLKS) ?
                               srespp->ns_flags | NS_DQ_OVER_TIME :
                               srespp->ns_flags & ~NS_DQ_OVER_TIME;
        }
#endif QUOTA
	dm_reply(response, 0, 0, NULL, NULL, NULL);
}


/*structure for sending sync request*/
extern struct dsync		/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;	/*device index*/
	dev_t realdev;	/*real device number for clearing flag*/
};


extern servesyncdisc1();

/*
 *Schedule an NSP to turn on the synchronization flag for the given device
 *This is called if it is determined under interrupt that a device should
 *be synchronized.
 */
sched_turn_on_devsync(mp)
register struct mount *mp;
{

	dm_message request;
	register struct dsync *dsp;

	if ((mp->m_flag & (M_IS_SYNC|M_NOTIFYING)))
		return;
	request = dm_alloc(sizeof(struct dsync),DONTWAIT);
	if (request == NULL)	/*if we can't get it, forget it*/
		return;
	DM_SOURCE(request) = 0;
	dsp = DM_CONVERT(request, struct dsync);
	dsp->dev = mp->m_dev;
	if (invoke_nsp(request, servesyncdisc1,1) != 0)
	{
		dm_release(request,0);
		return;
	}
}
