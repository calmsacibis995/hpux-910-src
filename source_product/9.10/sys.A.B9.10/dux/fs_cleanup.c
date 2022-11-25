/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/fs_cleanup.c,v $
 * $Revision: 1.10.83.6 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/08/15 00:23:30 $
 */

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
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/conf.h"
#include "../h/vfs.h"
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../h/kern_sem.h"

#include "../cdfs/cdfs_hooks.h"


#ifdef __hp9000s300
#define INV_K_ADR	0x7fffffff
#else
#define INV_K_ADR	NULL
#endif

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

lockf_cleanup(ip, site)
  struct inode *ip;
  register site_t site;
  { register struct locklist *cl = (struct locklist *)(&(ip->i_locklist));
    register struct locklist *nl;

    while((nl = cl->ll_link) != NULL)
      if((nl->ll_flags & L_REMOTE) && nl->ll_psite == site)
  	{ cl->ll_link = nl->ll_link;
	  lockfree(nl);
	}
      else cl = nl;
  }

/*
 * Perform all the necessary cleanup of inodes including:
 *
 *	1) Local inodes referenced by the remote site
 *
 *	2) Remote inodes referenced by this site
 *
 *	3) Remote inodes refering to devices at the remote site.
 */
ino_cleanup(site)
register site_t site;
{
	register struct inode *ip;
	register struct vnode *vp;
	register int numref;	/*the number of references of a various type*/
	int err = 0;

	/*traverse the list of inodes and cleanup anything held by a remote
	 *site*/
	for (ip = inode; ip < inodeNINODE; ip++)
	{
		vp = ITOV(ip);

	    /* The first set of checks are only valid for referenced vnodes.
	     * The ifdef'ed FULLDUX and ifdef'ed LOCAL_DISC cleanup code needs
	     * to be executed for unreferenced vnodes, so it is outside of
	     * this if statement.
	     * This check isn't needed, but it's a quick optimization for all
	     * of the enclosed cleanup code.
	     */
	    if(vp->v_count > 0)
	    {
		/*first check for a lock held by the site on the inode*/
		if (ilocked(ip) && (ip->i_ilocksite == site))
			iunlock(ip);
		/*determine if the site has the file open*/
		if ((numref = getsitecount(&ip->i_opensites,site)) > 0)
		{
			/*close the file as appropriate*/
			register numwrite;

			numwrite = getsitecount(&ip->i_writesites,site);

			/* Need synclock so we can change fifo r/w counts
			** and free up lockfs later on, BUT, we can NOT
			** sleep waiting, for the lock because we're the
			** only guy who can cleanup further failed sites
			** and we could end up sleeping on a lock held by
			** a failed site!  So, first check to see if the
			** lock is free, if it is, the go ahead and get the
			** lock.  Otherwise, return an error to clean_up, 
			** break out of this inode loop, and it will try 
			** again later...  Try to clean up all other inodes,
			** tho to prevent a circular deadlock!
			*/
			if (ip->i_flag & ISYNCLOCKED) {
#ifdef CLEANUP_DEBUG
			    msg_printf("fs_cleanup: returning err on site %d due to unavailable ISYNCLOCK\n",site);
#endif CLEANUP_DEBUG
			    err = 1;
			    continue;
			}
			else {
			    isynclock(ip);
			}

			/*if its is a fifo, do the closeps*/
			if (vp->v_type == VFIFO)
			{
				int numread;
				register count;
				numread = getsitecount(&ip->i_fifordsites,site);
				/*
				** Closep does a call to invalip() which
				** can sleep on the close of a remote inode.
				** However, since we own the sitemap, this
				** is a local inode, so we'll never sleep.
				*/
				for (count = numread; count > 0; count--)
					closep(ip, FREAD);
				for (count = numwrite; count > 0; count--)
					closep(ip, FWRITE);
				if (numread >0)
					updatesitecount(&ip->i_fifordsites,
						site,-numread);
			}
			/*update the sitecounts*/
			updatesitecount(&ip->i_opensites, site, -numref);
			if (numwrite > 0)
			{
				updatesitecount(&ip->i_writesites, site,
					-numwrite);
				/*decriment the device reference count*/
				if (vp->v_type == VREG)
					mdev_update(ip,site,-numwrite);
			}
			/*  Throw away locks from the failed site	*/
			lockf_cleanup(ip, site);
			/*
			** Update sync status.  This routine will send a
			** notify sync msg and sleep on the reply if the
			** sync status goes FROM async TO sync.  We cannot
			** allow the sleep (in case another site fails), but
			** we should be ok here, because we are always 
			** decreasing (never increasing) the r/w counts, and
			** therefore we should only do the following 
			**	sync -> sync 
			** 	sync -> async 
			**	async -> async
			** all of which are safe.
			*/
			checksync(ip);
			isyncunlock(ip);
			/* Throw out all but one reference */
			SPINLOCK(v_count_lock);
			vp->v_count -= numref-1;
			SPINUNLOCK(v_count_lock);
			/*and release the last reference*/
			VN_RELE(vp);
		}
		/*Is anyone referencing this file for anything else?*/
		if ((numref = getsitecount(&ip->i_refsites,site)) > 0)
		{
			updatesitecount(&(ip->i_refsites),site,-numref);
			SPINLOCK(v_count_lock);
			vp->v_count -= numref-1;
			SPINUNLOCK(v_count_lock);
			VN_RELE(vp);
		}
		/*determine if anyone is execing to this file*/
		if ((vp->v_type == VREG) &&
			(numref = getsitecount(&ip->i_execsites,site)) > 0)
		{
			/*
			** dectext cld send an updated count (w/ DM_SLEEP)
			** to the server if this were a remote inode, however,
			** since we own the sitemap, we are the server and
			** therefore, we won't do the send or sleep.
			*/
			dectext(vp, numref);
		}
	  }
#ifdef FULLDUX
		/* If the inode represents a remote device that was on the
		 * lost site, change the device number to the "error"
		 * device, so subsequent operations will return an error
		 */
		if (vp->v_type == VCHR && cdevrmt(ip->i_rdev) &&
			devsite(ip->i_rdev) == site)
		{
			/*	
			** A check for a locked inode must
			** be done here and the err variable set
			** if the inode is locked because we cannot
			** sleep in cleanup waiting for another site
			** to do something as that other site might
			** crash and we can't clean it up if we're
			** sleeping!
			*/
			if (ilocked(ip)) {
				err=1;
				continue;
			}
			ilock(ip);
			ip->i_rdev = mkcrmtdev(0,0);
			iunlock(ip);
		}
		else if (vp->v_type == VBLK && bdevrmt(ip->i_rdev) &&
			devsite(ip->i_rdev) == site)
		{
			/*
			** A check for a locked inode must
			** be done here and err code must be set
			** if the inode is locked because we cannot
			** sleep in cleanup waiting for another site
			** to do something as that other site might
			** crash and we can't clean it up if we're
			** sleeping!
			*/
			if (ilocked(ip)) {
				err=1;
				continue;
			}
			ilock(ip);
			ip->i_rdev = mkbrmtdev(0,0);
			iunlock(ip);
		}
#endif FULLDUX
#ifdef LOCAL_DISC
		/* If the inode is on a device on the remote site, unhash it
		 * and change the site to site 0, so subsequent I/O operations
		 * will result in errors.  Also, nullify
		 * the fs and the mount pointers since we should never
		 * reference them (If we do it is an error).
		 * Also invalidate active text files.
		 */
		if (remoteip(ip) && devsite(ip->i_dev) == site)
		{
			/*
			** A check for a locked inode must
			** be done here and the err code must be set
			** if the inode is locked because we cannot
			** sleep in cleanup waiting for another site
			** to do something as that other site might
			** crash and we can't clean it up if we're
			** sleeping!
			*/
			if (ilocked(ip)) {
				err=1;
				continue;
			}
			ilock(ip);
			ip->i_dev = mkbrmtdev(0,0);
			ip->i_fs = NULL;
			ip->i_dfs = NULL;
			ip->i_mount = NULL;
			/*remove it from the hash queue*/
			ip->i_back->i_forw = ip->i_forw;
			ip->i_forw->i_back = ip->i_back;
			/*and put it on a queue consisting only of itself
			 *(like a virgin inode)
			 */
			ip->i_forw = ip;
			ip->i_back = ip;
			/* If it is a fifo, wakeup any sleepers */
			if (vp->v_type == VFIFO)
			{
				wakeup (ip->i_frcnt);
				wakeup (ip->i_fwcnt);
			}
			/* Are we keeping track of the number of opens
			 * that have been done on this site?
			 */
			if ((numref = getsitecount(&ip->i_opensites,my_site))>0)
			{
				/* Zero it back out */
				updatesitecount(&(ip->i_opensites),my_site,
						-numref);
			}
			/*
			 * If the inode corresponds to an active
			 * text segment, tell the VM to invalidate
			 * it.  All future faults to this file
			 * will kill the faulting process.
			 */
			if (vp->v_flag & VTEXT)
				xinval(vp);
			/*
			 * nullify vfsp for this vnode since the
			 * underlying vfs will be yanked away
			 * in mcleanup() anyway.  The only interfaces
			 * that will still have access to this file
			 * and attempt to access v_vfsp is fstatfs().
			 */
			vp->v_vfsp = NULL;
			iunlock(ip);
		}
#endif LOCAL_DISC
	}    /* end of for (inodes) loop */
	return(err);
}

#ifdef LOCAL_DISC
#ifdef QUOTA

/* Routine used to cleanup dquot entries for a file system that was on a
   diskless client that crashed. 

   Returns 0 if it was able to clean up all dquots.
	   1 if it did not finish because it would have had to sleep.
*/

int
dquot_cleanup(mp)
struct mount *mp;
{
   register struct dquot *dqp, *tmp_dqp;
   register struct dqhead *dhp;
   register struct inode *ip;
   extern struct dqhead dqhead[];
   int failed=0;

     /* Nothing to do if quotas weren't enabled on this disk */
     if ((mp->m_qflags & MQ_ENABLED) == 0) {
	return(0);
     }

     /*
      * Run down the inode table and release all dquots assciated with
      * inodes on this filesystem.
      */
      for (ip = inode; ip < inodeNINODE; ip++) {
	  dqp = ip->i_dquot;
	  if (dqp != NULL && dqp->dq_mp == mp) {
	     /* We can't go to sleep in dux clean up routines. */
	     if (dqp->dq_flags & DQ_LOCKED) {
		/* Mark that we didn't succeed but go ahead and do all
		   the other inodes we can.
		 */
		failed = 1;
		continue;
	     }
	     dqp->dq_flags |= DQ_LOCKED;
	     dqput(dqp);
             ip->i_dquot = NULL;
	  }
      }
      /*
       * Run through the dquot entries and clean and invalidate the
       * dquots for this file system.
       * Valid dquot entries one the dqfreelist and dqresvlist are still
       * on the hash list so we don't have to traverse those lists separately.
       * If they're not valid, dq_mp is NULL so we don't have to worry about
       * them anyway.  CWB
       */
       for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++)
       {
           dqp = dhp->dqh_forw;
           while (dqp != (struct dquot *)dhp)  /* while not an empty list */
           {
              if (dqp->dq_mp == mp) {
		 /* We can't go to sleep in dux clean up routines. */
	         if (dqp->dq_flags & DQ_LOCKED) {
		    /* Mark that we didn't succeed but go ahead and do all
		       the other dquot entries we can.
		     */
		    failed = 1;
                    dqp = dqp->dq_forw;
	         }
		 else {
	            dqp->dq_flags |= DQ_LOCKED;
                    tmp_dqp = dqp;
                    dqp = dqp->dq_forw;
		    /* Have to clear flags and count so dqinval doesn't panic */
		    tmp_dqp->dq_flags &= ~(DQ_MOD|DQ_WANT);
		    tmp_dqp->dq_cnt = 0;
	            dqinval(tmp_dqp);
		 }
	      }
              else
                 dqp = dqp->dq_forw;
           }
       }

       return failed;
}

/* Routine used to cleanup quotas for a site that crashed.

   Returns 0 if it was able to clean up all quotas.
	   1 if it did not finish because it would have had to sleep.
*/

int
quota_cleanup(site)
site_t site;
{
	register struct mount *mp;
	register struct mounthead *mhp;
	int err=0;

	/* cruise the mount chains looking for mount entries that are from
	 * the crashed site.
	 */
	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++) {
	    mp = mhp->m_hforw;
	    while (mp != (struct mount *)mhp) {
	        /* Look for one for this site */
	        if (mp->m_flag&(MINUSE|MINTER) && bdevrmt(mp->m_dev) &&
	            devsite(mp->m_dev) == site) {
	            if (dquot_cleanup(mp)) {
		        err = 1;
	            }
	        }
	        mp = mp->m_hforw;
	    }
	}

	return(err);
}
#endif /* QUOTA */

/*
 * Invalidate all buffers associated with the failed site.  As with
 * the function binval, we do this by setting the invalid flag.
 */
buf_cleanup(site)
register site_t site;
{
	register struct buf *bp;
	register struct bufhd *hp;
#define dp ((struct buf *)hp)

	for (hp = bufhash; hp < &bufhash[BUFHSZ]; hp++)
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
			if (bdevrmt(bp->b_dev) &&
				devsite(bp->b_dev) == site)
					bp->b_flags |= B_INVAL;
#undef dp
}
#endif LOCAL_DISC

/*
 * fs_cleanup() -- called by error-detection code to clean up after a
 * failed workstation
 */
fs_cleanup(site)
register site_t site;
{
	int err;
	sv_sema_t ss;

	PXSEMA(&filesys_sema, &ss);
#ifdef LOCAL_DISC
#ifdef QUOTA
	/* First clean up the quota entries -- return if err
	   (didn't clean em up!).  Have to finish quotas before doing inodes.
	 */
	if (err = quota_cleanup(site)){
		VXSEMA(&filesys_sema, &ss);
		return(err);
	}
#endif /* QUOTA */
#endif /* LOCAL_DISC */
	/* Then clean up the inodes -- return if err (didn't clean em up! */
	if (err = ino_cleanup(site)){
		VXSEMA(&filesys_sema, &ss);
		return(err);
	}
	if (err = CDFSCALL(CDNO_CLEANUP)(site)){
		VXSEMA(&filesys_sema, &ss);
		return(err);
	}
	dev_cleanup(site);
#ifdef LOCAL_DISC
	/* Next clean up the buffers */
	buf_cleanup(site);
	/* 
	** write out what we can, in case the loss of the failed site 
	** prevents us from syncing (it'd better not! - AR)
	** (Could we deadlock on a wait in dux_strategy when we do 
	** this update??? -- AR)
	**
	** Disabling update for now.  The update was a precaution against
	** the situation where the file system containing /bin/sync was
	** lost.  We are not supporting a configuration where /bin/sync
	** is not on the root file system.
	*/
# ifdef  notdef
#  ifdef	NSYNC
	update(0,1,0);
#  else
	update(0,1);
#  endif	/* NSYNC */
# endif  /* notdef */
#endif LOCAL_DISC
	VXSEMA(&filesys_sema, &ss);
	return(0);
}

/*  These routines clean up any mount table entries for a failed site.	*/

mcleanup(site)
register site_t site;
{ 
	register struct mount *mp;
	register struct mounthead *mhp;
	struct mount *omp;
	extern site_t mount_site;
	extern site_t root_site;
	extern int mountlock;
        register struct vfs *vfsp;
	sv_sema_t ss;

	PXSEMA(&filesys_sema, &ss);
	/*
	**  If the site had the mount table locked, unlock it.
	*/
	if ((my_site == root_site) && mountlock && (site == mount_site)) {
		release_mount();
	}

	/*
	**  Search the vfs list for a vfs which has been locked by the
	**  failed site.
	*/
	for (vfsp = rootvfs; vfsp != 0; vfsp = vfsp->vfs_next) {
		if ((vfsp->vfs_flag & VFS_MLOCK) && (vfsp->vfs_site == site))
			vfs_unlock(vfsp);
	}
#ifdef LOCAL_DISC
	/*
	** A check for a locked mount table must be done here and a 
	** return(1) must be done if the mount table is locked because 
	** we cannot sleep in cleanup waiting for another site to do 
	** something as that other site might crash and we can't clean 
	** it up if we're sleeping!
	*/

	if (lock_mount(M_RECOVERY_LOCK)) {
		VXSEMA(&filesys_sema, &ss);
		return(1);
	}

	/*
	 * Quick and dirty mount table clean up.  Assumes we have no remote
	 *  inodes from site.  Just find file systems and remove the entries
	 * 
	 *  NOTE:
	 *	This may not be adequate for the case where the site crashed
	 *	in the process of mounting or unmounting the local disk.
	 *		Fred R.
	 */
	for (mhp = mounthash; mhp < &mounthash[MNTHASHSZ]; mhp++)
	{
		mp = mhp->m_hforw;
		while (mp != (struct mount *)mhp)
		{
			if (mp->m_flag&(MINUSE|MINTER) && bdevrmt(mp->m_dev) &&
			    devsite(mp->m_dev) == site)
			{
#ifdef GETMOUNT
				extern time_t mount_table_change_time;
				extern volatile struct timeval time;
#endif GETMOUNT
			        if(mp->m_bufp && mp->m_bufp != (struct buf *)INV_K_ADR)
				    brelse(mp->m_bufp);
			        if(mp->m_dfs && mp->m_dfs != (struct duxfs *)INV_K_ADR)
				    kmem_free((caddr_t)mp->m_dfs, 1);
				mp->m_bufp = 0;
				mp->m_dev = 0;
				vfs_lock(mp->m_vfsp);
				{
					struct vnode *coveredvp;
					coveredvp =mp->m_vfsp->vfs_vnodecovered;
					vfs_remove(mp->m_vfsp);
					VN_RELE(coveredvp);
				}
				kmem_free((caddr_t)mp->m_vfsp, sizeof (struct vfs));
				mp->m_flag = MAVAIL;
				omp = mp;
				mp = mp->m_hforw;
				mountremhash(omp);
				kmem_free((caddr_t)omp,sizeof(struct mount));
#ifdef GETMOUNT
				/* we've unmounted a crashed file system */

				mount_table_change_time = time.tv_sec;
#endif GETMOUNT
			}
			else
				mp = mp->m_hforw;
		}
	}
	release_mount();
#endif LOCAL_DISC
	VXSEMA(&filesys_sema, &ss);
	return(0);
}
