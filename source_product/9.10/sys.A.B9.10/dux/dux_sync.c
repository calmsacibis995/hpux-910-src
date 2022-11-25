/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_sync.c,v $
 * $Revision: 1.5.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:42:45 $
 */

/* HPUX_ID: @(#)dux_sync.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988  Hewlett-Packard Company.
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
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/vfs.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../dux/dm.h"
#include "../dux/sitemap.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_dev.h"
#include "../dux/cct.h"

/*
 *This file contains a miscellaney of routines which deal with synchronization.
 *The first routines deal with synchronizing inodes.
 */

/*
 *setsync is called on the serving site whenever it is necessary to use
 *synchronous I/O on a file.  The following are some examples:
 *	*When a file open causes there to be at least two sites with the file
 *	open, with at least one of them open for write.
 *	*When truncating a file.
 *
 *If the file was already synchronized, nothing happens.  If it was not
 *synchronized, messages will be sent to all sites with the file open.
 */
setsync(ip)
register struct inode *ip;
{
		int error;

		error = 0;
		if (!(ip->i_flag & ISYNC))	/*if not already synchronized*/
		{
			ip->i_flag |= ISYNC;
		
			/*
			** If not clustered then don't do this.
			*/
			if( my_site_status & CCT_CLUSTERED )
			{
				error = notifysync(ip,0);
			}
		}
		return(error);
}


/*
 *lock and unlock an inode for synchronization.  Note that the inode may
 *not be locked with a regular lock (ilock) when locking for synchronization.
 */
  
isynclock(ip)
register struct inode *ip;
{
	while (ip->i_flag & ISYNCLOCKED)
	{
		ip->i_flag |= ISYNCWANT;
		sleep((caddr_t)ip, PINOD);
	}
	ip->i_flag |= ISYNCLOCKED;
}

isyncunlock(ip)
register struct inode *ip;
{
	ip->i_flag &= ~ISYNCLOCKED;
	if (ip->i_flag&ISYNCWANT)
	{
		ip->i_flag &= ~ISYNCWANT;
		wakeup((caddr_t)ip);
	}
}

/*Request that a file be synchronized*/
struct syncreq			/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;		/*device of the file*/
	ino_t ino;		/*inode of the file*/
	int isquick;		/*1=flush buffers & ino times, leave async*/
				/*2=clear inode times */
};

struct inotimes_rep
{
	int timeflag;
	struct timeval atime;
	struct timeval ctime;
	struct timeval mtime;
};

/*
 *Servesync is called to mark an inode as using synchronous I/O.
 *The steps are as follows:
 *	+Unpack the message
 *	+Find the inode
 *	+Make sure the inode is valid and in use
 *	+Lock the inode
 *	+Flush and if not quick, invalidate all buffers associated with the
 *		inode.
 *	+If the file is a fifo, send the fifo information to the server.
 *	+Iupdat the inode.
 *	+Mark the inode as using synchronous I/O if the quick flag is not set.
 *	+Unlock the inode.
 *	+Send back a reply when complete
 */
servesync(reqp)
dm_message reqp;
{
	register struct syncreq *sp = DM_CONVERT(reqp,struct syncreq);
	register struct inode *ip;
	register struct mount *mp;
	dev_t dev;
	int error;
	dm_message reply;
	register struct inotimes_rep *irep;

	error = 0;
	/*First find the mount entry and the inode*/
	mp = getrmp(sp->dev,u.u_site);
	if (!mp)
	{
		error = ENOENT;
			 /* NOTE:  this cannot be EIO bacause that is */
			 /*        what DM_CANNOT_DELIVER is and we   */
			 /*        need to distinguish between inode  */
			 /*        not here and site has gone done.   */
		goto done;
	}
	dev = mp->m_dev;
	ip = ifind(dev, sp->ino);
	if (ip != NULL && (ITOV(ip)->v_count != 0))
	{

		reply = dm_alloc(sizeof(struct inotimes_rep), WAIT);
		irep = DM_CONVERT(reply, struct inotimes_rep);

		/* It's possible (although unlikely) that the inode is
		 * already marked as synchronous in which case we
		 * ignore the request
		 */
		if (ip->i_flag&ISYNC) {
			irep->timeflag = 0;
			goto done;
		}
		ilock(ip);
		if (sp->isquick == 2) {
			ip->i_flag &= ~(IUPD|IACC|ICHG);
			irep->timeflag = 0;
			iunlock(ip);
			goto done;
		}

		/* Write out all dirty buffers associated with the inode */
		syncip(ip, 0, 1);
		/* If the isquick flag is set it means that all we wanted
		 * was to have the buffers flushed.  If it is not set,
		 * mark the inode as synchronous.  Also, mark it as having
		 * no valid buffers, so that if in the future we perform
		 * any bufferred I/O on the inode we will invalidate the
		 * buffers.
		 */
		if (!sp->isquick)
		{
			ip->i_flag = (ip->i_flag | ISYNC) &
				~(IBUFVALID|IPAGEVALID);
		}
		/* Fifos need to have the cursors flushed too.  See next
		 * function.
		 */
		if ((ip->i_mode & IFMT) == IFIFO) {
			fifo_flush(ip);
			if (!sp->isquick) {
				ip->i_size = 0;
				ip->i_blocks = 0;
				ip->i_fifosize = 0;
				ip->i_frptr = 0;
				ip->i_fwptr = 0;
				ip->i_fflag = 0;
			}
		}
		irep->timeflag = ip->i_flag & (ICHG|IUPD|IACC);
		if (irep->timeflag != 0) {
			irep->atime.tv_sec = ip->i_atime.tv_sec;
			irep->ctime.tv_sec = ip->i_ctime.tv_sec;
			irep->mtime.tv_sec = ip->i_mtime.tv_sec;
			ip->i_flag &= ~(IUPD|IACC|ICHG);
		}
		iunlock(ip);
	} else {
		error = ENOENT;		/* NOTE:  this cannot be EIO */
	}
done:
	if (error)
		dm_quick_reply(error);
	else
		dm_reply(reply, 0, 0, NULL, NULL, NULL);
}

/* fifo flush request message */

struct fifo_flush_req {
	dev_t dev;
	ino_t ino;
	struct ic_fifo fifo;
};

/*
 *Send the fifo cursors from the inode to the server.
 *This is done whenever a fifo inode is synchronized.
 *NOTE:  It would be nice if this could be done with the response to
 *the synchronize message.  That would save us this extra message.
 *However, that would create a race condition, since, at the time of the
 *response, the inode is not locked at either the using site or the serving
 *site.  Therefore, it is possible that the response would get temporarily
 *delayed being sent back to the SS.  Meanwhile, a synchronous request could
 *come be sent from the US to the SS.  If it arrived before the response
 *came in, the fifo pointers would be incorrect.  (Note that it is not
 *possible for a request to come in from any site other than the US, since
 *fifos require synchronous I/O whenever two sites are involved, and therefore
 *no other site could yet have the fifo open before the response comes back
 *(although there is at least one site in the process of opening the fifo).)
 */
fifo_flush(ip)
register struct inode *ip;
{
	dm_message request;
	register struct fifo_flush_req *frp;

	request = dm_alloc(sizeof(struct fifo_flush_req),WAIT);
	frp = DM_CONVERT(request,struct fifo_flush_req);
	/* The following fields are used to identify the inode*/
	frp->dev = ip->i_dev;
	frp->ino = ip->i_number;
	/* The i_fifo field contains all the cursors we need to send */
	frp->fifo = ip->i_fifo;
	dm_send(request,
		DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
		DM_FIFO_FLUSH, devsite(ip->i_dev), DM_EMPTY,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	/*wakeup anyone sleeping on the fifo.  They need to retry with
	 *synchronous I/O*/
	wakeup(&(ip->i_frcnt));
	wakeup(&(ip->i_fwcnt));
}

/*
 * Receive the fifo flush request at the server.  Fill in the cursor into
 * the local copy of the inode.
 */
fifo_flush_recv(request)
dm_message *request;
{
	register struct fifo_flush_req *frp;
	register struct inode *ip;

	frp = DM_CONVERT(request,struct fifo_flush_req);
	ip = ifind(localdev(frp->dev),frp->ino);
	if (ip != NULL) {
		ip->i_frptr = frp->fifo.if_frptr;
		ip->i_fwptr = frp->fifo.if_fwptr;
		ip->i_fflag = frp->fifo.if_fflag;
		ip->i_fifosize = frp->fifo.if_fifosize;
	}
	else {
		printf("ifind failed:  fifo_flush_recv\n");
	}
	dm_quick_reply(0);
}

/*
 *notifysync will notify all machines with the file open that the file is to
 *be changed to use synchronous I/O.
 *If isquick is 1, we only need to flush the buffers, but can
 *continue with async I/O.  This is used with calls like stat, that need
 *to make sure the inode sizes at the server are up to date.
 *Since only the writing sites have buffers to flush, we only send it to them.
 */
notifysync(ip,isquick)
register struct inode *ip;
int isquick;
{
	register struct sitemap *mapp;
	dm_message request;
	register struct syncreq *sreqp;
	site_t dest;
	register type = ip->i_mode & IFMT;
	int error=0;
	int count;
	register struct sitearray *array;
	int ret;

	/*don't notify on directories or network files because they are
	 *already synchronous
	 */
	if (type == IFDIR || type == IFNWK)
		return(0);

	mapp = &ip->i_opensites;

	/* If there are no sites with the file open, we can return immediately*/
	if (mapp->s_maptype == S_MAPEMPTY)
		return(0);

	/* If this is a quick request and it is already in synchronous */
	/* mode, then we can return immediately */
	if (isquick && (ip->i_flag & ISYNC))
		return(0);

	/* Allocate a request and fill in the device identifying information*/
	request = dm_alloc(sizeof(struct syncreq),WAIT);
	sreqp = DM_CONVERT(request,struct syncreq);
	sreqp->ino = ip->i_number;
#ifdef hp9000s800
	/*
	 * With advent of auto-config, pass LU device number to client
	 * from server, not MI dev.
	 */
	sreqp->dev = ip->i_mount->m_rdev;
#else
	sreqp->dev = ip->i_dev;
#endif 
	sreqp->isquick = isquick;
	/* If there is only one site in the map, send the message to that site
	 * (unless that site is my own site in which case we can return)
	 */
	if (mapp->s_maptype == S_ONESITE)
	{
		if (mapp->s_onesite.s_site == my_site ||
			mapp->s_onesite.s_site == 0 ||
			(ip->i_flag & IOPEN && 
			 mapp->s_onesite.s_site == u.u_site &&
			 mapp->s_onesite.s_count == 1))
		{
			goto finished;
		}
		dest = mapp->s_onesite.s_site;
		error = send_sync_request(request, dest, ip);
		if (error == DM_CANNOT_DELIVER)  /*site has gone down */
			error = 0;
	}
	else
	{
		waitlock(mapp);
		if (mapp->s_maptype == S_MAPEMPTY) {
			goto finished;
		}

		if (mapp->s_maptype == S_ONESITE)
		{
			dest = mapp->s_onesite.s_site;
			if (dest == my_site || dest == 0 ||
			   (ip->i_flag & IOPEN && dest == u.u_site &&
			   mapp->s_onesite.s_count == 1)) {
				goto finished;
			}
			error=send_sync_request(request, dest, ip);
			if (error == DM_CANNOT_DELIVER)  /*site has gone down */
				error = 0;
			goto finished;
		}
		else
		{
			array = mapp->s_array;
			while (array != NULL)
			{
				for (count = 0; count < SITEARRAYSIZE; count++)
				{
					if (array->s_entries[count].s_count!=0)
					{
					   dest=array->s_entries[count].s_site;
					   if (dest == my_site || dest == 0 ||
					      (ip->i_flag & IOPEN && 
					       dest == u.u_site &&
					       array->s_entries[count].s_count==1))
						continue;
					    ret = 
					     send_sync_request(request,dest,ip);
					    if (ret != 0 && ret != 
					       DM_CANNOT_DELIVER)
					     error = ret;

					}
				}
				array = array->s_next;
			}
		}
	}
finished:
	dm_release(request, NULL);
	return(error);
}

send_sync_request(request, dest, ip)
dm_message request;
site_t dest;
struct inode *ip;
{
	dm_message reply;
	register struct inotimes_rep *irep;
	int flag = 0;
	int error = 0;

	reply = dm_send(request,
		DM_SLEEP, DM_SYNC, dest,
		sizeof(struct inotimes_rep), NULL, NULL, NULL, NULL, NULL,
		NULL, NULL);
	error = DM_RETURN_CODE(reply);
	if (!error)
	{
		irep = (DM_CONVERT(reply, struct inotimes_rep));
		if (irep->timeflag) {
			ilock(ip);
			if (irep->timeflag & IACC) {
				if (irep->atime.tv_sec > ip->i_atime.tv_sec) {
					ip->i_atime.tv_sec=irep->atime.tv_sec;
					flag |= IACC;
				}
			}
			if (irep->timeflag & ICHG) {
				if (irep->ctime.tv_sec > ip->i_ctime.tv_sec) {
					ip->i_ctime.tv_sec=irep->ctime.tv_sec;
					flag |= ICHG;
				}
			}
			if (irep->timeflag & IUPD) {
				if (irep->mtime.tv_sec > ip->i_mtime.tv_sec) {
					ip->i_mtime.tv_sec=irep->mtime.tv_sec;
					flag |= IUPD;
				}
			}
			ip->i_flag |= flag;
			iunlock(ip);
		}
	}
	dm_release(reply, 0);
	return(error);
}

/*
 *setasync is called on the serving site whenever asynchronous I/O can
 *be used.  No conditions requiring synchronous I/O may be in force.
 *
 *Nothing needs to be sent with asynchronous I/O.  Instead, the flag is
 *simply reset.  If someone makes a synchronous request they will be
 *informed that asynchronous I/O should be used.
 *
 *This function is being replaced by a macro.
 */
#define setasync(ip) (ip)->i_flag &= ~ISYNC

/*
 *checksync determines if synchronous or asynchronous I/O is necessary and
 *calls the appropriate function (setsync or setasync).  Checksync makes
 *its determination based on the information in the sitemaps (who is reading
 *and writing).  Other considerations (such as currently truncating the file)
 *may override and require synchronization regardless of the effects of this
 *routine.  The synchronization lock should be in effect whenever such a
 *condition exists, as well as whenever calling this function.  That will
 *prevent potential conflicts between them.
 */
checksync(ip)
register struct inode *ip;
{
	/*synchronization is required whenever:
	 *	a) at least two sites have the file open
	 *			and
	 *	b1) at least one site has the file open for write
	 *			or
	 *	b2) the file is a pipe
	 *		or
	 *	b3) at least one site has the file locked (lockf)
	 *
	 *			or
	 *
	 *	c) the file is a directory
	 *
	 *	Also, once a pipe is synchronized, it cannot be desynchronized
	 *	if any remote site has it open
	 */

	register int type = ip->i_mode&IFMT;
	register int issync;
	register int opensites;
	int error;

	error = 0;
	opensites = gettotalsites(&ip->i_opensites);
	switch (type)
	{
	case IFDIR:
	case IFNWK:
		/*Directories and NSFs are always synchronous*/
		issync = 1;
		break;
	case IFREG:
		/*Regular files are synchronous if more than one site
		 *has the file open and at least one site is writing
		 */
		issync = opensites > 1 &&
			(gettotalsites(&ip->i_writesites) > 0 ||
			  (ip->i_locklist && (ip->i_mode & IENFMT) &&
				(ip->i_mode & (IEXEC >> 3)) == 0));
		break;
	case IFIFO:
		/*Fifos are synchronous if more than one site has them open
		 *or if they were previously synchrnous
		 */
		issync = opensites > 1 ||
			((ip->i_flag&ISYNC) && opensites > 0);
		break;
	default:
		/*Other file types (in particular devices) are never
		 *synchronous.
		 */
		issync=0;
		break;
	}
	/*Set the appropriate mode, based on the choices made above*/
	if (issync)
	{
		error = setsync(ip);
	}
	else
	{
		setasync(ip);
	}
	return(error);
}

/*
 *The following routines deal with synchronization involving complete discs.
 *These are used when a disc is at or near being full, requiring that all
 *I/O to it be waited for.
 */

/*structure for sending sync request*/
struct dsync		/*DUX MESSAGE STRUCTURE*/
{
	dev_t dev;	/*device index*/
	dev_t realdev;	/*real device number for clearing flag*/
};

#define CHECKTIME 60	/*number of seconds to wait before clearing SYNC flag*/

/*
 *Clear the notify flag.  This function is called under interrupt after the
 *last response to the syncdisc request comes in.  The notify flag is used
 *to indicate that a synchronize request is in progress.
 */
/*ARGSUSED*/
static
clear_notify(request,response)
dm_message request, response;
{
	register struct dsync *dsp = DM_CONVERT(request, struct dsync);
	register struct mount *mp = getmount(dsp->realdev);

	mp->m_flag &= ~M_NOTIFYING;
	dm_release(request,0);
}

/*
 *This function is called at the SS to convert a disc to using sync I/O.
 *Set the flag and send a clustercast to all workstations.
 */
syncdisc(mp)
register struct mount *mp;
{
	dm_message request;
	register struct dsync *dsp;
	extern syncdisccheck();		/*forward*/

	if (mp->m_flag & M_IS_SYNC)
		return;	/*already synchronized*/
	/* Set the flags indicating synchronization*/
	mp->m_flag |= M_IS_SYNC|M_WAS_SYNC|M_NOTIFYING;
	/* Allocate, fill in, and send the request */
	request = dm_alloc(sizeof(struct dsync),WAIT);
	dsp = DM_CONVERT(request, struct dsync);
	dsp->realdev = mp->m_dev;
#ifdef	hp9000s800
	/* With autoconfig, the LU device number does not match the real
	 * device number.  The dev field, which goes to the client
	 * should have the real device number, while the realdev field,
	 * which is used by the server, should have the LU device number.
	 * (True, the names of the fields sound backwards, but they
	 * are as they are for historical reasons.
	 */
	dsp->dev = mp->m_rdev;
#else
	dsp->dev = mp->m_dev;
#endif

	/* dm_send allocates a reply buffer for CLUSTERCAST messages,
	   and the reply isn't used in clear_notify so set flag to 
	   release reply & buffer */
	dm_send(request, DM_REPEATABLE | DM_FUNC | DM_RELEASE_REPLY,
		DM_SYNCDISC, DM_CLUSTERCAST, DM_EMPTY,
		clear_notify, NULL, 0, 0, NULL, NULL, NULL);
	fserr(mp->m_bufp->b_un.b_fs,"File system nearly full");
	/* Set a timeout to check if the disc is still full in a minute.
	 * This timeout will continue to reset itself as long as necessary.
	 */
	timeout(syncdisccheck,(caddr_t)mp,HZ*CHECKTIME);
}

/*
 *serve a request for synchronizing a disc.  Do this by setting the flag
 *and by flushing all buffers.
 */
servesyncdisc(request)
dm_message request;
{
	register struct dsync *dsp = DM_CONVERT(request, struct dsync);
	register struct mount *mp;

	mp = getrmp(dsp->dev, u.u_site);
	if (mp)
		discsync(mp);
	dm_quick_reply(0);
}

/*
 *This is the same as servesyncdisc except that it is invoked by a local
 *request rather than a remote one.
 */
servesyncdisc1(request)
dm_message request;
{
	register struct dsync *dsp = DM_CONVERT(request, struct dsync);
	register struct mount *mp = getmp(dsp->dev);

	if (mp)
		discsync(mp);
	dm_release(request,0);
}

/*
 *perform the actual syncing and flushing of the disc.
 */
discsync(mp)
register struct mount *mp;
{
	struct vnode *dev_vp;
	if (mp == NULL)
		return;
	mp->m_flag &= ~M_NOTIFYING;
	if (!(mp->m_flag&M_IS_SYNC))
	{
		mp->m_flag |= M_IS_SYNC;
		dev_vp = devtovp(mp->m_dev);
		bflush(dev_vp);
		VN_RELE(dev_vp);
	}
}

/*
 *every 60 seconds, check to see if we still need synchronizing on this
 *disc.  In order to prevent rapid oscillation, require two timeouts
 *before the flag is actually turned off.
 */
syncdisccheck(mp)
register struct mount *mp;
{
	register struct fs *fs = mp->m_bufp->b_un.b_fs;

	if (!(mp->m_flag & M_IS_SYNC))
		return;		/*false alarm*/
	/* Check to see if the disc has enough free space.  Also, don't
	 * permit the flag to be turned off if we are still waiting for
	 * a reply from any sites
	 */
	if (!(mp->m_flag & M_NOTIFYING) &&
		fs->fs_cstotal.cs_nbfree > mp->m_maxbufs &&
		freespace(fs, fs->fs_minfree) > 0)
	{	/*if we have enough free space*/
		if (mp->m_flag & M_WAS_SYNC)
		{	/*first time out*/
			mp->m_flag &= ~M_WAS_SYNC;
		}
		else
		{	/*second time out--actually desynchronize*/
			mp->m_flag &= ~M_IS_SYNC;
			return;
		}
	}
	else	/*There wasn't enough space.  Require two more timeouts */
		mp->m_flag |= M_WAS_SYNC;
	/*unless we have returned, we need to start another timeout*/
	timeout(syncdisccheck,(caddr_t)mp,HZ*CHECKTIME);
}

/*
 *Update if necessary the space quota for a the disc holding the file with
 *inode ip.  This is normally called whenever a file is opened or closed for
 *write.  Site specifies the remote site doing the opening.  Direction should
 *be +1 for an open, -1 for a close.  In the case of error recovery, direction
 *could be less than -1.  If this is the first open for the site,
 *increase the quota.  If this is the last close, decrease it.
 */
mdev_update(ip,site,direction)
struct inode *ip;
site_t site;
int direction;
{
	register struct mount *mp = ip->i_mount;
	register struct fs *fs = mp->m_bufp->b_un.b_fs;
	extern struct cct clustab[];

	if (site == my_site || site == 0)
		return;		/*shouldn't happen*/
	if (direction > 0)	/*increment site count*/
	{
		if (updatesitecount(&mp->m_dwrites,site,direction)==direction)
		{	/*need to increment quota*/
			mp->m_maxbufs += 
				lblkno (fs,clustab[site].total_buffers);
			/*If we are over quota, sync the disc*/
			if (fs->fs_cstotal.cs_nbfree < mp->m_maxbufs &&
				(!(mp->m_flag & M_IS_SYNC)))
				syncdisc(mp);
		}
	}
	else			/*decrement site count*/
	{
		if (updatesitecount(&mp->m_dwrites,site,direction) == 0)
		{	/*need to decrement quota*/
			mp->m_maxbufs -= 
				lblkno (fs,clustab[site].total_buffers);
			/*note that we don't desync the disc, but instead
			 *wait for a timeout to do that
			 */
		}
	}
}


struct ino_data		/*DUX MESSAGE STRUCTURE*/
{ 
	dev_t  i_dev;
    ino_t  i_number;
    /*  This is the new info for remote inodes				*/
    int    i_newmode;
	site_t i_site;
};

ino_update(ip)
register struct inode *ip;
{ 
	register dm_message request = dm_alloc(sizeof(struct ino_data), WAIT);
    register struct ino_data *idp;
    register struct sitemap *mapp = &(ip->i_opensites);
    register struct buf *bp = NULL;
    register site_t dest;

    /* If there is only one site in the map, send the message to that site
     * (unless that site is my own site in which case we can return)
     */

    if(mapp->s_maptype == S_ONESITE)
	{ 
		if(mapp->s_onesite.s_site == my_site || mapp->s_onesite.s_site == 0)
		{ 
			dm_release(request, NULL);
	    return;
	  }
	dest = mapp->s_onesite.s_site;
      }
    else
      { /*send a multisite message*/
	extern struct buf *createsitelist();

	if((bp = createsitelist(*mapp, 0)) == NULL)
		{ 
			dm_release(request, NULL);
	    return;
	  }
	dest = DM_MULTISITE;
      }

    idp = DM_CONVERT(request, struct ino_data);
    
#ifdef hp9000s800
    /*
     * With advent of auto-config, pass LU device number to client
     * from server, not MI dev.
     */
    idp->i_dev = ip->i_mount->m_rdev;
#else
    idp->i_dev    = ip->i_dev;
#endif
    idp->i_site   = my_site;
    idp->i_number = ip->i_number;
    /*  Load up the info to be updated on the using sites		*/
    idp->i_newmode = ip->i_mode;

    dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY|DM_REPEATABLE,
	    DM_INOUPDATE, dest, DM_EMPTY, NULL, bp, 0, 0, NULL, NULL, NULL );
}

dux_ino_update(reqp)
dm_message reqp;
{ 
	register struct ino_data *idp = DM_CONVERT(reqp, struct ino_data);
	register struct mount *mp;

	mp = getrmp(idp->i_dev, idp->i_site);
	if (mp)
	{
		register struct inode *ip = ifind(mp->m_dev, idp->i_number);

		if (ip != NULL)
		{ /*  Set the fields to values passed over */
			ip->i_mode = idp->i_newmode;
		}
	}
	dm_quick_reply(0);
}
