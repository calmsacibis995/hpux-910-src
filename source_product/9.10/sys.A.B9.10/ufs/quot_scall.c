/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/quot_scall.c,v $
 * $Revision: 1.9.83.4 $        $Author: dkm $
 * $State: Exp $        $Locker:  $
 * $Date: 94/09/30 14:02:45 $

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
 * Quota system calls.
 */

#include "../h/stdsyms.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/mount.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/vfs.h"
#include "../h/uio.h"
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../h/kernel.h"
#include "../ufs/fs.h"
#include "../h/kern_sem.h"
#include "../h/syscall.h"

#include "../h/buf.h"
#include "../h/conf.h"
#include "../dux/dm.h"
#include "../dux/duxfs.h"
#include "../dux/lookupops.h"
#include "../dux/dmmsgtype.h"
#include "../dux/nsp.h"
#include "../dux/dux_dev.h"
#include "../dux/duxparam.h"
#include "../dux/cct.h"
#include "../dux/dux_lookup.h"

#define Q_SETQCLEAN 25 /* value for internal quotactl cmd for setting and *
			* syncing the FS_QCLEAN bit in the superblock.    */

struct duxquotareq    /* DUX MESSAGE STRUCTURE for quotactl call requests */
  {
    int  dq_cmd;    /* quotactl command */
    int  dq_uid;    /* user id for command */
    struct dqblk dq_block;  /* structure to pass in data for set sys calls */
  };

struct duxquotamntreq  /* DUX MESSAGE STRUCTURE for informing clients that */
                       /* quotas are on or off.                            */
  {
    dev_t   rdev;         /* The real device number and site      */
    site_t  site;
    u_short on_or_off;    /* Are quotas being turned on or off */
    struct remino qinod;  /* quota file -> m_qinod */
  };


extern struct dux_context dux_context;

extern struct	dqhead	dqhead[NDQHASH];


#ifdef GETMOUNT
extern time_t mount_table_change_time;
#if defined(__hp9000s800) && defined(VOLATILE_TUNE)
extern volatile struct timeval time;
#else
extern struct timeval time;
#endif /* __hp9000s800 && VOLATILE_TUNE */
#endif

/*
 * Sys call to allow users to find out
 * their current position wrt quota's
 * and to allow super users to alter it.
 */
quotactl(uap)
	register struct a {
		int	cmd;
		caddr_t	fdev;
		int	uid;
		caddr_t	addr;
	} *uap;
{
	struct mount *mp;

        dm_message msg;
        struct duxquotareq *dqreqp;
        struct dqblk *dqrespp;
        struct buf *bp;
        struct dquot *dqp;
	struct dqhead *dhp;
        int len,len2;
        int error;
        int return_size;
        extern struct buf *geteblk();

	if (uap->uid < 0)
		uap->uid = u.u_ruid;
        /* Must be SU unless cmd is Q_SYNC or cmd is Q_GETQUOTA and uid is 
           that of caller  */
	if ((uap->cmd != Q_SYNC) &&
             (!((uap->cmd == Q_GETQUOTA) && (uap->uid == u.u_ruid))) && 
            !suser()) 
	{
                u.u_error = EPERM;
		return;
        }

	PSEMA(&filesys_sema);

        if (uap->fdev == NULL)
        {
           if (uap->cmd != Q_SYNC)
           {
              u.u_error = ENOENT;
              VSEMA(&filesys_sema);
              return;
	   }
           else  /* Q_SYNC *all* filesystems */
           {
              int count=0;
              for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++)
	      {
loop:
                 dqp = dhp->dqh_forw;
                 while (dqp != (struct dquot *)dhp)  
	         {
                    if (dqp->dq_mp->m_vfsp->vfs_mtype == MOUNT_UFS)
                       if (((my_site_status & CCT_CLUSTERED) == 0) ||
                           dqp->dq_mp->m_site == my_site) /*local filesystem*/
		       {      /* do local sync's while here */

                          if ((dqp->dq_flags & DQ_MOD) &&
                              (dqp->dq_mp->m_qflags & MQ_ENABLED) &&
                              (dqp->dq_flags & DQ_LOCKED) == 0)
                          {
                             dqp->dq_flags |= DQ_LOCKED;
                             dqupdate(dqp); 
                             DQUNLOCK(dqp);
			     /*
			      * If the dqupdate did not succeed, do
			      *	not try again.  We could infinite loop here.
			      */
                             if ((dqp->dq_flags & DQ_MOD) == 0) {
			     	goto loop;
			     }
                          }
		       }
                       else
                          count++;

                    dqp = dqp->dq_forw;
                 }
	      }
              
              if (count)
              {
                 msg = dm_alloc(sizeof(struct duxquotareq), WAIT);

                 dqreqp = DM_CONVERT(msg, struct duxquotareq);
                 dqreqp->dq_cmd = uap->cmd;  /* only field needed here */

                 dm_send(msg,DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY|DM_REPEATABLE,
                         DM_QUOTACTL, DM_CLUSTERCAST, DM_EMPTY, NULL, 
                         NULL, 0, 0, NULL, NULL, NULL);
              }
  	      VSEMA(&filesys_sema);
              return;
           }
        }    
        else  /*  uap->fdev != NULL  */    
        {
           u.u_error = fdevtomp(uap->fdev, &mp, DQ_LOCAL);
           if (u.u_error)
           {
   	      VSEMA(&filesys_sema);
              return;
	   }

           if ((my_site_status & CCT_CLUSTERED) && mp->m_site != my_site)
	   {
	      if (uap->cmd == Q_SETQCLEAN) { /* must be local fs for this */
		 u.u_error = EINVAL;
		 VSEMA(&filesys_sema);
		 return;
	      }
              bp = geteblk(2*MAXPATHLEN); 

              u.u_error = 
                   copyinstr(uap->fdev, bp->b_un.b_addr, MAXPATHLEN,&len);
              if (u.u_error)
	      {
                 brelse(bp);
	         VSEMA(&filesys_sema);
                 return;
	      }

              if (uap->cmd == Q_QUOTAON)  /* addr needs to be sent */
              {
                 u.u_error = 
                   copyinstr(uap->addr, bp->b_un.b_addr+len,MAXPATHLEN,&len2);
                 if (u.u_error)
		 {
                    brelse(bp);
	            VSEMA(&filesys_sema);
                    return;
		 }
                 len += len2;
	      }

              msg = dm_alloc(sizeof(struct duxquotareq), WAIT);
              dqreqp = DM_CONVERT(msg, struct duxquotareq);
              dqreqp->dq_cmd = uap->cmd;
              dqreqp->dq_uid = uap->uid;

              if (uap->cmd == Q_SETQUOTA || uap->cmd == Q_SETQLIM)
              {
                 error = copyin(uap->addr, (caddr_t)&dqreqp->dq_block,
                                sizeof(struct dqblk));
                 if (error)
                 {
                    brelse(bp);
                    dm_release(msg, 0);
                    u.u_error = error;
	            VSEMA(&filesys_sema);
                    return;
                 }

              }

              if (uap->cmd == Q_GETQUOTA)
                 return_size = sizeof(struct dqblk);
              else
                 return_size = DM_EMPTY;

              msg = dm_send(msg, DM_SLEEP|DM_REQUEST_BUF|
                            DM_RELEASE_REQUEST, DM_QUOTACTL, 
                            mp->m_site,
                            return_size, NULL,
                            bp, len, 0, NULL, NULL, NULL);

              if ((error=DM_RETURN_CODE(msg)) == 0)
              {
                 if (uap->cmd == Q_GETQUOTA) 
                 {
                    dqrespp = DM_CONVERT(msg, struct dqblk);

                    error=copyout((caddr_t)dqrespp, uap->addr, 
                                  sizeof(struct dqblk));
                 }

              }
              dm_release(msg, 0);
              u.u_error = error;
	      VSEMA(&filesys_sema);
              return;

           }
           else  /*  At fileserver site, "fall through" to the switch stmt  */
           {  
              extern void cmd_switch();
              
              cmd_switch(uap,mp,DQ_LOCAL);
	      VSEMA(&filesys_sema);
           }
        }
}


void
cmd_switch(uap,mp,call_site)
struct a {
    int       cmd;
    caddr_t   fdev;
    int       uid;
    caddr_t   addr;
} *uap;
struct mount *mp;
u_short call_site;
{

      switch (uap->cmd) {

      case Q_QUOTAON:
	 u.u_error = opendq(mp, uap->addr, call_site);
#ifdef GETMOUNT
	 if (u.u_error == 0)
		 mount_table_change_time = time.tv_sec;
#endif
	 break;

      case Q_QUOTAOFF:
	 u.u_error = closedq(mp);
#ifdef GETMOUNT
	 if (u.u_error == 0)
		 mount_table_change_time = time.tv_sec;
#endif
	 break;

      case Q_SETQUOTA:
      case Q_SETQLIM:
	 u.u_error = setquota(uap->cmd, uap->uid, mp, uap->addr, call_site);
	 break;

      case Q_GETQUOTA:
	 u.u_error = getquota(uap->uid, mp, uap->addr, call_site);
	 break;

      case Q_SYNC:
	 u.u_error = qsync(mp);
	 break;

      case Q_SETQCLEAN:
	u.u_error = setqclean(mp);
	break;

      default:
	 u.u_error = EINVAL;
	 break;
      }
}



/* DUX code that server runs to handle quota system call made on client */

int
serve_quotactl(request)
dm_message request;
{
   struct duxquotareq *dqreqp = DM_CONVERT(request, struct duxquotareq);
   dm_message response;
   struct buf *reqbp;
   struct mount *mp;
   struct a {
      int	cmd;
      caddr_t	fdev;
      int	uid;
      caddr_t	addr;
   } ua;
   caddr_t namep;

   reqbp = DM_BUF(request);  /* If req buf in dm_send, this is NULL */
   
   if (reqbp == NULL && dqreqp->dq_cmd == Q_SYNC) /* Q_sync all fs here */
      mp = NULL;
   else
   {
      u.u_error = fdevtomp((char *)reqbp->b_un.b_addr, &mp, DQ_REMOTE);
      
      if (u.u_error)
      {
         dm_quick_reply(u.u_error);
         return(0);
      }        
   }
   ua.cmd = dqreqp->dq_cmd;
   ua.uid = dqreqp->dq_uid;
   /* fdev field is no longer needed */

   if (dqreqp->dq_cmd == Q_QUOTAON)  /* addr needs to be found */
   {
      namep = (caddr_t)reqbp->b_un.b_addr;
      while (*namep++) ;  /* get past the fdev string to pt at name of file */
      
      ua.addr = namep;
   }
   else if (dqreqp->dq_cmd == Q_SETQUOTA || 
            dqreqp->dq_cmd == Q_SETQLIM) /* have quota info passed in */
   {
      ua.addr = (caddr_t)&dqreqp->dq_block;
   }
   else if (dqreqp->dq_cmd == Q_GETQUOTA)  /* Quota info will be returned */
   {
      response = dm_alloc(sizeof(struct dqblk),WAIT);
      ua.addr = (caddr_t) DM_CONVERT(response, struct dqblk);
   }

   cmd_switch(&ua,mp,DQ_REMOTE);

   if (dqreqp->dq_cmd != Q_GETQUOTA)  /* no info needs to be returned */
      dm_quick_reply(u.u_error);

   else  /* Must send back quota info to client  */
      dm_reply(response, 0, u.u_error, NULL, NULL, NULL);
   return(0);
}


/* Routine to send msg to other sites that quotas are on or off for a fs  */

void
quota_update_sites(on_or_off, mp)
u_short on_or_off;
struct mount *mp;

{
   struct duxquotamntreq *reqp;   
   dm_message msg;

   msg = dm_alloc(sizeof(struct duxquotamntreq), WAIT);
   reqp = DM_CONVERT(msg, struct duxquotamntreq);
   reqp->on_or_off = on_or_off;
   reqp->rdev = mp->m_rdev;
   reqp->site = mp->m_site;
   if (on_or_off == Q_QUOTAON) {
	pkrmi (mp->m_qinod, &(reqp->qinod));
   }

   dm_send(msg, 
           DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
           DM_QUOTAONOFF, DM_CLUSTERCAST, DM_EMPTY, NULL, 
           NULL, 0, 0, NULL, NULL, NULL);
   return;
}


/* Routine used to release release dquots for fs with quotas now turned off */
void
quota_release(mp)
struct mount *mp;
{
   register struct dquot *dqp, *tmp_dqp;
   register struct dqhead *dhp;
   register struct inode *ip;

loop:
     /*
      * Run down the inode table and release all dquots assciated with
      * inodes on this filesystem.
      */
      for (ip = inode; ip < inodeNINODE; ip++) {
	  dqp = ip->i_dquot;
	  if (dqp != NULL && dqp->dq_mp == mp) {
	     if (dqp->dq_flags & DQ_LOCKED) {
		dqp->dq_flags |= DQ_WANT;
		(void) sleep((caddr_t)dqp, PINOD+2);
		goto loop;
	     }
	     dqp->dq_flags |= DQ_LOCKED;
	     dqput(dqp);
             ip->i_dquot = NULL;
	  }
      }
      /*
       * Run through the dquot entries and clean and invalidate the
       * dquots for this file system.
       */
       for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++)
       {
           dqp = dhp->dqh_forw;
           while (dqp != (struct dquot *)dhp)  /* while not an empty list */
           {
              if (dqp->dq_mp == mp) {
	         if (dqp->dq_flags & DQ_LOCKED) {
	            dqp->dq_flags |= DQ_WANT;
	            (void) sleep((caddr_t)dqp, PINOD+2);
	            goto loop;
	         }
	         dqp->dq_flags |= DQ_LOCKED;
                 if (((my_site_status & CCT_CLUSTERED) == 0) ||
                     mp->m_site == my_site) /* local filesystem */
                 {
                    if (dqp->dq_flags & DQ_MOD) {
                       dqupdate(dqp);
                    }
                 }
                 tmp_dqp = dqp;
                 dqp = dqp->dq_forw;
	         dqinval(tmp_dqp);
	      }
              else
                 dqp = dqp->dq_forw;
           }
       }
       return;
}


/* routine for client when quotas are turned on or off at server */

int
quota_mnt_update(request)
dm_message request;        
{
   struct duxquotamntreq *reqp = DM_CONVERT(request, struct duxquotamntreq);
   struct mount *mp;
   extern struct mount *getrmp();
   extern struct inode *stprmi();
   struct inode *ip;

   mp = getrmp(reqp->rdev,reqp->site);

   if (mp == NULL)
   {
      dm_quick_reply(ENODEV);
      return(0);
   }
   else
   {
      if (reqp->on_or_off == Q_QUOTAON)
      {
	 ip = stprmi(&(reqp->qinod), mp);
	 if (ip == NULL) {
		dm_quick_reply(u.u_error);
		return 0;
	 }
	 mp->m_qinod = ip;
         mp->m_qflags = MQ_ENABLED;
         dm_quick_reply(0);
      }
      else /*  quotas are being turned off for this fs  */
      {
         mp->m_qflags = 0;  /* quotas are disabled */
         quota_release(mp);
         dm_quick_reply(0);
      }
   }
#ifdef GETMOUNT
   mount_table_change_time = time.tv_sec;  /* need to update /etc/mnttab */
#endif
   return(0);
}

/*
 * Set the quota file up for a particular file system.
 * Called as the result of a setquota system call.
 */
int
opendq(mp, addr, call_site)
	register struct mount *mp;
	caddr_t addr;			/* quota file */
        u_short call_site;
{
	struct vnode *vp;
	struct dquot *dqp;
	struct fs *fsp;
	int error;

        if (call_site == DQ_REMOTE)
	{
	    /* u.u_cntxp must be set here so that lookupname uses the *
	     * context of the server.                                 */
            u.u_cntxp = (char **)&dux_context;

            error = lookupname(addr, UIOSEG_KERNEL, FOLLOW_LINK,
		    (struct vnode **)0, &vp, LKUP_NORECORD, (caddr_t *)0);
	}
        else
	{
            error = lookupname(addr, UIOSEG_USER, FOLLOW_LINK,
		    (struct vnode **)0, &vp, LKUP_NORECORD, (caddr_t *)0);
	}
	if (error)
		return (error);
	if (VFSTOM(vp->v_vfsp) != mp || vp->v_type != VREG) {
		VN_RELE(vp);
		return (EACCES);
	}
	fsp = mp->m_bufp->b_un.b_fs;
	if (mp->m_qflags & MQ_ENABLED) {
		int val = FS_QFLAG( fsp);
		(void) closedq(mp);
		FS_QSET( fsp, val);
#ifdef AUTOCHANGER
		sbupdate(mp,0);
#else
		sbupdate(mp);
#endif /* AUTOCHANGER */
	}
	if (mp->m_qinod != NULL) {	/* open/close in progress */
		VN_RELE(vp);
		return (EBUSY);
	}
	mp->m_qinod = VTOI(vp);
	/*
	 * The file system time limits are in the super user dquot.
	 * The time limits set the relative time the other users
	 * can be over quota for this file system.
	 * If it is zero a default is used (see quota.h).
	 */
	error = getdiskquota(0, mp, 1, &dqp);
	if (error == 0) {
		mp->m_btimelimit =
		    (dqp->dq_btimelimit? dqp->dq_btimelimit: DQ_BTIMELIMIT);
		mp->m_ftimelimit =
		    (dqp->dq_ftimelimit? dqp->dq_ftimelimit: DQ_FTIMELIMIT);
		dqput(dqp);
		mp->m_qflags = MQ_ENABLED;	/* enable quotas */
                /* let other sites know that quotas are on for this fs */
                if (my_site_status & CCT_CLUSTERED)
                   quota_update_sites(Q_QUOTAON, mp);
		if (FS_QFLAG( fsp) != FS_QOK) {
			FS_QSET( fsp, FS_QNOTOK);
		}
	} else {
		/*
		 * Some sort of I/O error on the quota file.
		 */
		irele(mp->m_qinod);
		mp->m_qinod = NULL;
		FS_QSET( fsp, FS_QNOTOK);
	}
	return (error);
}

/*
 * Close off disk quotas for a file system.
 */
int
closedq(mp)
	register struct mount *mp;
{
	register struct inode *qip;
	register struct fs *fsp;

	if ((mp->m_qflags & MQ_ENABLED) == 0)
		return (0);
	qip = mp->m_qinod;
	if (qip == NULL)
		panic("closedq");
	mp->m_qflags = 0;	/* disable quotas */
	fsp = mp->m_bufp->b_un.b_fs;
	if (FS_QFLAG( fsp) != FS_QCLEAN) {
		FS_QSET( fsp, FS_QNOTOK);
#ifdef AUTOCHANGER
		sbupdate(mp,0);
#else
		sbupdate(mp);
#endif /* AUTOCHANGER */
	}
        /* let other sites know that quotas are off for this fs */
        if (my_site_status & CCT_CLUSTERED)
           quota_update_sites(Q_QUOTAOFF, mp);
        quota_release(mp);
	/*
	 * Sync and release the quota file inode.
	 */
	ilock(qip);
	(void) syncip(qip, 0, 1);
	iput(qip);
	mp->m_qinod = NULL;
	return (0);
}
	
/*
 * Set various fields of the dqblk according to the command.
 * Q_SETQUOTA - assign an entire dqblk structure.
 * Q_SETQLIM - assign a dqblk structure except for the usage.
 */
int
setquota(cmd, uid, mp, addr, call_site)
	int cmd;
	uid_t uid;
	struct mount *mp;
	caddr_t addr;
        u_short call_site;
{
	register struct dquot *dqp;
	register struct fs *fsp;
	struct dquot *xdqp;
	struct dqblk newlim;
	int error;

	if ((mp->m_qflags & MQ_ENABLED) == 0)
		return (ESRCH);
        if (call_site == DQ_LOCAL)  /* local filesystem */
        {
           error = copyin(addr, (caddr_t)&newlim, sizeof (struct dqblk));
           if (error)
		return (error);
        }
        else
        {
           newlim = *(struct dqblk *)addr;
        }
	error = getdiskquota(uid, mp, 0, &xdqp);
	if (error)
		return (error);
	dqp = xdqp;
	/*
	 * Don't change disk usage on Q_SETQLIM
	 */
	if (cmd == Q_SETQLIM) {
		newlim.dqb_curblocks = dqp->dq_curblocks;
		newlim.dqb_curfiles = dqp->dq_curfiles;
	}
	fsp = mp->m_bufp->b_un.b_fs;
	if (dqp->dq_curblocks != newlim.dqb_curblocks ||
	    dqp->dq_curfiles  != newlim.dqb_curfiles) {
		/* change in usage */
		FS_QSET( fsp, FS_QNOTOK);
	}
	else if (!dqp->dq_bhardlimit && !dqp->dq_bsoftlimit && 
	         !dqp->dq_fhardlimit && !dqp->dq_fsoftlimit) {
		if (newlim.dqb_bhardlimit || newlim.dqb_bsoftlimit ||
		    newlim.dqb_fhardlimit || newlim.dqb_fsoftlimit) {
			/* 0 to non-0 change in limits */
			FS_QSET( fsp, FS_QNOTOK);
		}
	}
	dqp->dq_dqb = newlim;
	if (uid == 0) {
		/*
		 * Timelimits for the super user set the relative time
		 * the other users can be over quota for this file system.
		 * If it is zero a default is used (see quota.h).
		 */
		mp->m_btimelimit =
		    newlim.dqb_btimelimit? newlim.dqb_btimelimit: DQ_BTIMELIMIT;
		mp->m_ftimelimit =
		    newlim.dqb_ftimelimit? newlim.dqb_ftimelimit: DQ_FTIMELIMIT;
	} else {
		/*
		 * If the user is now over quota, start the timelimit.
		 * The user will not be warned.
		 */
		if (dqp->dq_curblocks >= dqp->dq_bsoftlimit &&
		    dqp->dq_bsoftlimit && dqp->dq_btimelimit == 0)
			dqp->dq_btimelimit = time.tv_sec + mp->m_btimelimit;
		else
			dqp->dq_btimelimit = 0;
		if (dqp->dq_curfiles >= dqp->dq_fsoftlimit &&
		    dqp->dq_fsoftlimit && dqp->dq_ftimelimit == 0)
			dqp->dq_ftimelimit = time.tv_sec + mp->m_ftimelimit;
		else
			dqp->dq_ftimelimit = 0;
		/* dqp->dq_flags &= ~(DQ_BLKS|DQ_FILES); *not used in hp-ux */
	}
	dqp->dq_flags |= DQ_MOD;
	dqupdate(dqp);
	dqput(dqp);
	return (0);
}

/*
 * Q_GETDLIM - return current values in a dqblk structure.
 */
int
getquota(uid, mp, addr,call_site)
	uid_t uid;
	struct mount *mp;
	caddr_t addr;
        u_short call_site;
{
	register struct dquot *dqp;
	struct dquot *xdqp;
	int error;

	if ((mp->m_qflags & MQ_ENABLED) == 0)
		return (ESRCH);
	error = getdiskquota(uid, mp, 0, &xdqp);
	if (error)
		return (error);
	dqp = xdqp;
	if (dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) {
		error = ESRCH;
	} 
        else 
        {
           if (call_site == DQ_LOCAL)  /* local filesystem */
           {
              error = copyout((caddr_t)&dqp->dq_dqb, addr, sizeof(struct dqblk));
           }
           else
	   {
              *(struct dqblk *)addr = dqp->dq_dqb; 
           }
        }
	dqput(dqp);
	return (error);
}

/*
 * Q_SYNC - sync quota files to disk.
 */
int
qsync(mp)
	register struct mount *mp;
{
	register struct dquot *dqp;
	register struct dqhead *dhp;

	if (mp != NULL && (mp->m_qflags & MQ_ENABLED) == 0)
		return (ESRCH);
        for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++)
        {
loop:
           dqp = dhp->dqh_forw;
           while (dqp != (struct dquot *)dhp)  
	   {
		if ((dqp->dq_flags & DQ_MOD) &&
		    (mp == NULL || dqp->dq_mp == mp) &&
		    (dqp->dq_mp->m_qflags & MQ_ENABLED) &&
		    (dqp->dq_flags & DQ_LOCKED) == 0) {
			dqp->dq_flags |= DQ_LOCKED;
			dqupdate(dqp);
			DQUNLOCK(dqp);
			/*
			 * If the dqupdate did not succeed, do
			 *	not try again.  We could infinite loop here.
			 */
                        if ((dqp->dq_flags & DQ_MOD) == 0) {
			    goto loop;
			}
		}
                dqp = dqp->dq_forw;
	   }
	}
	return (0);
}

/* Internally used (undocumented) quota system call used to set and
 * sync the FS_QCLEAN bit in the superblock.
 */
int
setqclean(mp)
struct mount *mp;
{
	FS_QSET(mp->m_bufp->b_un.b_fs, FS_QCLEAN);
#ifdef AUTOCHANGER
	sbupdate(mp,0);
#else
	sbupdate(mp);
#endif /* AUTOCHANGER */
	return (0);
}

int
fdevtomp(fdev, mpp, call_site)
	char *fdev;
	struct mount **mpp;
        u_short call_site;
{
        struct inode *ip;
        site_t site;
        dev_t dev;
	struct vnode *vp;
	int error;

        if (call_site == DQ_REMOTE)
	{
            /* u.u_cntxp must be set here so that lookupname uses the *
             * context of the server.                                 */
	    u.u_cntxp = (char **)&dux_context;

            error = lookupname(fdev, UIOSEG_KERNEL, FOLLOW_LINK,
		    (struct vnode **)0, &vp, LKUP_NORECORD, (caddr_t *)0);
        }
        else
	{
            error = lookupname(fdev, UIOSEG_USER, FOLLOW_LINK,
		    (struct vnode **)0, &vp, LKUP_NORECORD, (caddr_t *)0);
	}
	if (error)
		return (error);
	if (vp->v_type != VBLK) {
		VN_RELE(vp);
		return (ENOTBLK);
	}
        ip = VTOI(vp);
        site = (size_t)ip->i_rsite;
        dev = ip->i_device;
        if ((my_site_status & CCT_CLUSTERED) && (site != 0) && 
            (site != my_site))
	{
	   if (dev == NODEV)   /* /dev+/localroot/root */
	     dev = rootdev;
           VN_RELE(vp);
           *mpp = getrmp(dev,site); 
	}
        else
        {
#ifndef _WSIO
	   /* 
	    * getmp expects the device number parameter to have had its
	    * logical unit mapped to a manager index.  It appears
	    * that we can't always depend upon the vp->v_rdev field
	    * being the mapped device number, so we check it here and
	    * map the number if necessary.  mry 12/28/90
	    */
	   if (!(vp->v_flag & VMI_DEV))
	      (void) map_lu_to_mi(&dev, IFBLK, 0);
#endif /* not _WSIO */
           VN_RELE(vp);
           *mpp = getmp(dev);        
        }
	if (*mpp == NULL)
		return (ENODEV);
	else
		return (0);
}

/*
 * Open quotas when a file system is mounted.
 */

int
mnt_opendq( uap, devname)
	register struct a {
		int	type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} *uap;
	char *devname;
{
	int error = 1;

	if ((uap->flags & M_QUOTA) && (uap->type == MOUNT_UFS) && (u.u_syscall == SYS_VFSMOUNT)) {
		char qfname[MAXPATHLEN];
		char *s1 = qfname;
		char *s2 = QFILENAME;
		int len;
		struct mount *mp;
	 
		/* get quotas filename */
		error = copyinstr( uap->dir, qfname, MAXPATHLEN, &len);
		if (error)
			return error;
		while (*s1++); --s1;
		while (*s1++ = *s2++);

		/* get pointer to mount table and superblock */
		error = fdevtomp( devname, &mp, DQ_LOCAL);
		if (error)
			return error;
        	if ((my_site_status & CCT_CLUSTERED) && mp->m_site && (mp->m_site != my_site)) {
			uprintf("can not enable quotas on remote device\n");
		}
		else {
			if (error = opendq( mp, (caddr_t) qfname, DQ_REMOTE)) {
				struct fs *fsp = mp->m_bufp->b_un.b_fs;
				FS_QSET( fsp, FS_QNOTOK);
			}
		}
	}
	return error;
}

#endif	/* QUOTA */

