/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/quota_ufs.c,v $
 * $Revision: 1.14.83.3 $        $Author: kcs $
 * $State: Exp $        $Locker:  $
 * $Date: 93/09/17 20:20:55 $

(c) Copyright 1983, 1984, 1985, 1986 Hewlett-Packard Company.
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

#ifdef QUOTA
/*
 *
 * Routines used in checking limits on file system usage.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../h/mount.h"
#include "../ufs/fs.h"

#include "../dux/cct.h"
extern site_t my_site;


/*
 * Dquot reserved list.
 */
struct dquot dqresvlist;

#define dqinstailresv(DQP) { \
	(DQP)->dq_freeb = dqresvlist.dq_freeb; \
	(DQP)->dq_freef = &dqresvlist; \
	dqresvlist.dq_freeb->dq_freef = (DQP); \
	dqresvlist.dq_freeb = (DQP); \
}


/* 
 * Called by timeout() under interrupt, this routine will set a flag for
 * the reserved list of quota cache entries allowing them to be added to
 * the free list.
 */
dq_set_freelist()
{
   dqresvlist.dq_flags |= DQ_FREE;
}

/* 
 * If this is the last reference to this cache entry, instead of putting
 * it directly on the free list (which is what dqput would do), put it on
 * a reserved list that will ensure that it stays in the cache for a
 * while (up to DQ_RESVLIST_TIME seconds).  This will keep the cache entry
 * around a while for users without quotas so that a disk access is not 
 * needed to determine if they have quotas every time they add a file.
 */
void
dqreserve(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_cnt == 0 || (dqp->dq_flags & DQ_LOCKED) == 0)
		panic("dqput");

	DQUNLOCK(dqp);
	if (--dqp->dq_cnt == 0)
	{
		dqinstailresv(dqp);
                if ((dqresvlist.dq_flags & DQ_RESERVED) == 0) 
		{  /* timeout() has not yet been called */
                   dqresvlist.dq_flags |= DQ_RESERVED;
                   timeout(dq_set_freelist, (caddr_t)0, DQ_RESVLIST_TIME*hz);
		}
	}
}

/*
 * Find the dquot structure that should
 * be used in checking i/o on inode ip.
 */
struct dquot *
getinoquota(ip)
	register struct inode *ip;
{
	register struct dquot *dqp;
	register struct mount *mp;
	struct dquot *xdqp;

	mp = VFSTOM(ip->i_vnode.v_vfsp);

	/*
	 * Check for quotas enabled.
	 */
	if ((mp->m_qflags & MQ_ENABLED) == 0)
		return (NULL);
	/*
	 * Check for someone doing I/O to quota file.
	 */
        if (((my_site_status & CCT_CLUSTERED) == 0) ||
             mp->m_site == my_site) /* local filesystem */
           if (ip == mp->m_qinod)
		return (NULL);

	if (getdiskquota(ip->i_uid, mp, 0, &xdqp))
		return (NULL);
	dqp = xdqp;
	if ((dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) 
            && (((my_site_status & CCT_CLUSTERED) == 0) || 
                mp->m_site == my_site)
           )
        {
		dqreserve(dqp);
		dqp = NULL;
	} else {
		DQUNLOCK(dqp);
	}
	return (dqp);
}

/*
 * Update disk usage, and take corrective action.
 */
int
chkdq(ip, change, force)
	struct inode *ip;
	long change;
	int force;
{
	register struct dquot *dqp;
	register u_long ncurblocks;
	int error = 0;

	if (change == 0)
		return (0);
	dqp = ip->i_dquot;
	if (dqp == NULL)
		return (0);
	if (change < 0) {
		dqp->dq_flags |= DQ_MOD;
		if ((int)dqp->dq_curblocks + change >= 0)
			dqp->dq_curblocks += change;
		else
			dqp->dq_curblocks = 0;
		if ((dqp->dq_curblocks < dqp->dq_bsoftlimit)
		    && (dqp->dq_uid != 0))
			dqp->dq_btimelimit = 0;
		return (0);
	}

	ncurblocks = dqp->dq_curblocks + change;
	/*
	 * Allocation. Check hard and soft limits.
	 * Skip checks for super user.
	 */
	if (((u.u_uid == 0)
            /* since client write will show up as uid 0 if not yet notified *
             * that they are over hard limit, can't only test for uid of 0. *
             * if file being written to is owned by root, then skip tests.  *
             * Also, if the force flag is set, skip the tests since it is   *
             * not root running (non-root client is), then that user at the *
             * client is either doing a chown or in the frag_fit routine    *
             * when this gets called, so there is no need to check limits.  *
             */
             && ((ip->i_uid == 0) || force))
            || (dqp->dq_uid == 0))  /* cache entry is for root, skip tests */
		goto out;
	/*
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
	dqp->dq_flags |= DQ_MOD;
        if (!force)
           dqp->dq_flags &= ~(DQ_HARD_BLKS | DQ_SOFT_BLKS | DQ_TIME_BLKS);
	if (ncurblocks >= dqp->dq_bhardlimit && dqp->dq_bhardlimit && !force)
        {
		if (ip->i_uid == u.u_ruid) {
			uprintf("\nDISK LIMIT REACHED (%s) - WRITE FAILED\n",
			   ip->i_fs->fs_fsmnt);
		}
                dqp->dq_flags |= DQ_HARD_BLKS;
                if (u.u_uid != 0)  /* neither root nor client w/sync write */
                   error = EDQUOT;
	}
	else if (ncurblocks >= dqp->dq_bsoftlimit && dqp->dq_bsoftlimit) {
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit ||
		    dqp->dq_btimelimit == 0) {
			dqp->dq_btimelimit =
			    time.tv_sec +
			    VFSTOM(ip->i_vnode.v_vfsp)->m_btimelimit;
 			if ((ip->i_uid == u.u_ruid) && !force)
				uprintf("\nWARNING: disk quota (%s) exceeded\n",
				   ip->i_fs->fs_fsmnt);
		} else if (time.tv_sec > dqp->dq_btimelimit && !force) {
			if (ip->i_uid == u.u_ruid) 
                        {
          		   uprintf(
			      "\nOVER DISK QUOTA: (%s) NO MORE DISK SPACE\n",
 			      ip->i_fs->fs_fsmnt);
			}
                        dqp->dq_flags |= DQ_TIME_BLKS;
                        if (u.u_uid != 0) /* not root, not client w/sync */
	                   error = EDQUOT;
		}
                if (!force)
                   dqp->dq_flags |= DQ_SOFT_BLKS;
	}
out:
	if (error == 0)
		dqp->dq_curblocks = ncurblocks;
	return (error);
}

/*
 * Check the inode limit, applying corrective action.
 */
int
chkiq(mp, ip, uid, force)
	struct mount *mp;
	struct inode *ip;
	int uid;
	int force;
{
	register struct dquot *dqp;
	register u_long ncurfiles;
	struct dquot *xdqp;
	int error = 0;

	/*
	 * Free.
	 */
	if (ip != NULL) {
		dqp = ip->i_dquot;
		if ((dqp == NULL)
#ifdef ACLS   
                    /* If a continuation inode, don't decrement here,  */
                    /* the "real" inode will decrement twice.          */
                    || ((ip->i_mode & IFMT) == IFCONT)
#endif ACLS
                   )
			return (0);
		dqp->dq_flags |= DQ_MOD;
		if (dqp->dq_curfiles && 
		   (dqp->dq_fsoftlimit != 0 || dqp->dq_fhardlimit != 0))
		{
			dqp->dq_curfiles--;
#ifdef ACLS   
                        /* If this is an inode with a continuation inode,  */
                        /* decrement a second time, since the continuation */
                        /* will not be decremented in this routine.        */
                        if (ip->i_contin && dqp->dq_curfiles)
   			   dqp->dq_curfiles--;
#endif ACLS
		}
		if ((dqp->dq_curfiles < dqp->dq_fsoftlimit)
		    && (dqp->dq_uid != 0))
			dqp->dq_ftimelimit = 0;
		return (0);
	}

	/*
	 * Allocation. Get dquot for for uid, fs.
	 * Check for quotas enabled.
	 */
	if ((mp->m_qflags & MQ_ENABLED) == 0)
		return (0);
	if (getdiskquota(uid, mp, 0, &xdqp))
		return (0);
	dqp = xdqp;
	if (dqp->dq_fsoftlimit == 0 && dqp->dq_fhardlimit == 0) {
		dqput(dqp);
		return (0);

	}
	/*
	 * Skip checks for super user.
	 */
	if ((u.u_uid == 0) || (dqp->dq_uid == 0))
		goto out;
	dqp->dq_flags |= DQ_MOD;
	ncurfiles = dqp->dq_curfiles + 1;
	/*
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
        dqp->dq_flags &= ~(DQ_HARD_FILES | DQ_SOFT_FILES | DQ_TIME_FILES);
	if (ncurfiles >= dqp->dq_fhardlimit && dqp->dq_fhardlimit && !force) {
		if (uid == u.u_ruid) {
			uprintf("\nFILE LIMIT REACHED - CREATE FAILED (%s)\n",
			    mp->m_bufp->b_un.b_fs->fs_fsmnt);
		}
                dqp->dq_flags |= DQ_HARD_FILES;
		error = EDQUOT;
	} else if (ncurfiles >= dqp->dq_fsoftlimit && dqp->dq_fsoftlimit) {
		if (ncurfiles == dqp->dq_fsoftlimit || dqp->dq_ftimelimit==0) {
			dqp->dq_ftimelimit = time.tv_sec + mp->m_ftimelimit;
			if ((uid == u.u_ruid) && !force)
				uprintf("\nWARNING - too many files (%s)\n",
				    mp->m_bufp->b_un.b_fs->fs_fsmnt);
		} else if (time.tv_sec > dqp->dq_ftimelimit && !force) {
			if (uid == u.u_ruid) {
				uprintf(
				    "\nOVER FILE QUOTA - NO MORE FILES (%s)\n",
				    mp->m_bufp->b_un.b_fs->fs_fsmnt);
			}
                        dqp->dq_flags |= DQ_TIME_FILES;
			error = EDQUOT;
		}
                if (!force)
                   dqp->dq_flags |= DQ_SOFT_FILES;
	}
out:
	if (error == 0)
		dqp->dq_curfiles++;
	dqput(dqp);
	return (error);
}

/*
 * Release a dquot.
 */
void
dqrele(dqp)
	register struct dquot *dqp;
{
	if (dqp != NULL) {
		DQLOCK(dqp);
		if (dqp->dq_cnt == 1 && dqp->dq_flags & DQ_MOD)
                   if (((my_site_status & CCT_CLUSTERED) == 0) ||
                       dqp->dq_mp->m_site == my_site) /* local filesystem */
			dqupdate(dqp);
		dqput(dqp);
	}
}

#endif QUOTA
