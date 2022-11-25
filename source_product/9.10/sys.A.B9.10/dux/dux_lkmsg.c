/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_lkmsg.c,v $
 * $Revision: 1.9.83.9 $        $Author: craig $
 * $State: Exp $        $Locker:  $
 * $Date: 94/08/19 10:40:26 $
 */

/* HPUX_ID: @(#)dux_lkmsg.c	55.2		89/01/04 */

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
 *This file contains the function for packing, serving, and unpacking
 *all system calls that call lookup
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/stat.h"
#include "../h/pathname.h"
#include "../h/file.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/dux_lookup.h"
#include "../h/mount.h"
#include "../dux/lkup_dep.h"
#include "../dux/lookupmsgs.h"
#include "../dux/dux_dev.h"
#include "../h/systm.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs_hooks.h"
extern struct vnode *devtovp();
#ifdef QUOTA
#include "../ufs/quota.h"
#include "../ufs/fs.h"
#include "../h/buf.h"
#include "../dux/duxfs.h"
#include "../h/kernel.h"
#endif QUOTA

#define LOOKUP_REPLY_EXDEV	4

/*
 *Service a garden variety lookup request
 *Although to be purely general, both the dirvp and the compvp need to be
 *processed, for DUX, this routine only gets called in cases where the compvp
 *is needed.
 */
/*ARGSUSED*/
dux_pure_lookup_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	dm_message reply;
	register struct pure_lookup_reply *rp;

	/* send back the inode */
	reply = dm_alloc(sizeof (struct pure_lookup_reply), WAIT);
	rp = DM_CONVERT (reply, struct pure_lookup_reply);
	rp -> lookup_reply_flag = 0;

	if (dirvp && (dirvp->v_fstype != VUFS) && (dirvp->v_fstype != VCDFS)) {
	    /* We don't send back VDUX or VDUX_CDFS.  This isn't quite
	     * right but it only happens in the case of a vn_rename of
	     * a LMFS mount point and it should fail anyway with EXDEV.
	     * I'd like * to do a complete fix, but this is much better
	     * than panicing which is what it used to do.
	     * Notice we don't reply with EXDEV, we reply with 0, and
	     * let the unpack routine return EXDEV.  If we just replyed with
	     * with EXDEV, the dirvp pointer on the requesting site would
	     * not be zeroed and so it would return it and an extra VN_RELE
	     * would be done.  Really kludgy, but I hope we're going to rewrite
	     * this stuff for 10.0 anyways. cwb
	     */
	    update_duxref(dirvp, -1, 0);
	    VN_RELE(dirvp);
	    if (compvp)
	    	VN_RELE(compvp);
		
	    rp -> lookup_reply_flag |= LOOKUP_REPLY_EXDEV;
	}
	else {

	    /* mark the references in the sitemap */
	    if (compvp) 
	       if(compvp->v_fstype == VCDFS) {
		    updatesitecount(&((VTOCD(compvp))->cd_refsites), u.u_site, 1);
	       }
	       else {
		    updatesitecount(&((VTOI(compvp))->i_refsites), u.u_site, 1);
	       }
	    if (dirvp) 
	       if(dirvp->v_fstype == VCDFS) {
		    updatesitecount(&((VTOCD(dirvp))->cd_refsites), u.u_site, 1);
	       }
	       else {
		    updatesitecount(&((VTOI(dirvp))->i_refsites), u.u_site, 1);
	       }
	    if (compvp)  {
		rp -> lookup_reply_flag |= LOOKUP_REPLY_COMPVPP;
		pkrmv (compvp, &rp->pkv);
	    }
	    if (dirvp)  {
		rp -> lookup_reply_flag |= LOOKUP_REPLY_DIRVPP;
		pkrmv (dirvp, &rp->pkvdvp);
	    }
	}

	dm_reply(reply, 0, 0, NULL, NULL, NULL);
}

/*
 *Unpack a pure lookup reply
 *Note:  Unlike other dux operations, do not return EOPCOMPLETE for
 *a remote operation.  Instead, just return 0.  This is because the fact
 *that only a lookup has been requested implies that we want to do the same
 *thing with this vnode whether it is local or remote.
 */
/*ARGSUSED*/
int
dux_pure_lookup_unpack(request, reply, dirvpp, compvpp)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
{
	register struct pure_lookup_reply *rp =
		DM_CONVERT(reply, struct pure_lookup_reply);
	extern struct vnode *stprmv();
	dev_t dev;
	struct remvno *rvpcomp;
	struct remvno *rvpdir;
	register struct mount *mp;
	extern struct mount *getrmount();
	struct vnode *vp, *dvp;
	int error;
	
if (rp -> lookup_reply_flag & LOOKUP_REPLY_EXDEV) {
	/* See comments in dux_pure_lookup_server() */
	return(EXDEV);
}
	rvpcomp = &(rp->pkv);
	rvpdir = &(rp->pkvdvp);

if (rp -> lookup_reply_flag & LOOKUP_REPLY_COMPVPP) {
	if ((vp = stprmv(rvpcomp,NULL)) == NULL) {
		error = u.u_error;
/*the locations of dev and site in remkno and remino are the same*/
		mp = getrmount(rvpcomp->rv_ino.dev, rvpcomp->rv_ino.site);
		dev = mp->m_dev;
		send_ref_update_no_vno(dev, rvpcomp);
		if (rp->lookup_reply_flag&LOOKUP_REPLY_DIRVPP) {
			mp = getrmount(rvpdir->rv_ino.dev, rvpdir->rv_ino.site);
			dev = mp->m_dev;
			send_ref_update_no_vno(dev, rvpdir);
		}
		return (error);
	}
	/* Update the reference double count */
	update_duxref(vp,1,1);
	*compvpp = vp;
}
if (rp -> lookup_reply_flag & LOOKUP_REPLY_DIRVPP) {
	if ((dvp = stprmv(rvpdir,NULL)) == NULL) {
		error = u.u_error;
		mp = getrmount(rvpdir->rv_ino.dev, rvpdir->rv_ino.site);
		dev = mp->m_dev;
		send_ref_update_no_vno(dev, rvpdir);
		if (rp->lookup_reply_flag&LOOKUP_REPLY_COMPVPP) {
			send_ref_update_no_vno(dev, rvpcomp);
			update_duxref(*compvpp,-1,1);
			VN_RELE(*compvpp);
			*compvpp = (struct vnode *)0;
		}
		return (error);
	}
	/* Update the reference double count */
	update_duxref(dvp,1,1);
	*dirvpp = dvp;
}
	return (0);
}

/*
 * This function is similar to dux_pure_lookup_pack except that no state
 * is kept at the server.  It is used for functions like reboot.
 */
/*ARGSUSED*/
dux_norecord_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	dm_message reply;
	register struct pure_lookup_reply *rp;

	/* send back the inode */
	reply = dm_alloc(sizeof (struct pure_lookup_reply), WAIT);
	rp = DM_CONVERT (reply, struct pure_lookup_reply);
	pkrmv (compvp, &rp->pkv);
	dm_reply(reply, 0, 0, NULL, NULL, NULL);
	/* Now release the vnode */
	VN_RELE(compvp);
}

/*
 * Unpack a vnode without saving state
 * This unpack routine is also used with exec, since the state will be
 * added independently.
 */
/*ARGSUSED*/
int
dux_norecord_unpack(request, reply, dirvpp, compvpp)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
{
	register struct pure_lookup_reply *rp =
		DM_CONVERT(reply, struct pure_lookup_reply);
	extern struct vnode *stprmv();

	struct vnode *vp;

	if ((vp = stprmv(&rp->pkv,NULL)) == NULL)
		return (u.u_error);
	*compvpp = vp;
	return (0);
}

/* pack an open request */
/*ARGSUSED*/
dux_open_pack(request, opcode, dependent)
dm_message request;
int opcode;
caddr_t dependent;
{
	struct open_request *op = DM_CONVERT (request, struct open_request);

	op->mode = ((struct copenops *)dependent)->mode;
	op->filemode = ((struct copenops *)dependent)->filemode;
}

/* pack a create (or mknod/mkdir) request */
/*ARGSUSED*/
dux_create_pack(request, opcode, dependent)
dm_message request;
int opcode;
register caddr_t dependent;
{
	register struct create_request *cp =
		DM_CONVERT (request, struct create_request);

	cp->vattr = *(((struct copenops *)dependent)->vap);
	cp->excl = ((struct copenops *)dependent)->excl;
	cp->mode = ((struct copenops *)dependent)->mode;
	cp->filemode = ((struct copenops *)dependent)->filemode;
}

#ifdef QUOTA
void
reply_quota_flags(dirvp, error)
struct vnode *dirvp;
int error;
{
   struct dquot *dquotp;
   struct mount *mp;
   dm_message reply;
   struct quota_reply *rp;

   mp = VFSTOM((dirvp)->v_vfsp);
   reply = dm_alloc(sizeof (struct quota_reply), WAIT);
   rp = DM_CONVERT (reply, struct quota_reply);

   if (!getdiskquota(u.u_uid, mp, 0, &dquotp))
   {
      rp->dqreplflags = (dquotp->dq_flags & DQ_SOFT_FILES) ?
                         DQOVERSOFT :
                         0;
      rp->dqreplflags = (dquotp->dq_flags & DQ_HARD_FILES) ?
                         rp->dqreplflags | DQOVERHARD :
                         rp->dqreplflags & ~DQOVERHARD;
      rp->dqreplflags = (dquotp->dq_flags & DQ_TIME_FILES) ?
                         rp->dqreplflags | DQOVERTIME :
                         rp->dqreplflags & ~DQOVERTIME;

      rp->rdev = mp->m_rdev;
      rp->site = mp->m_site;

      dqput(dquotp);           
   }
   else
   {
      rp->dqreplflags = DQOFF;
   }
   dm_reply(reply, 0, error, NULL, NULL, NULL);
   return;
}

#endif QUOTA

/*process an open request*/
/*ARGSUSED*/
dux_open_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	struct open_request *op = DM_CONVERT (request, struct open_request);
	register error;

	error = vns_open(&compvp, op->filemode, op->mode);
	if (error)
	{
		VN_RELE(compvp);
#ifdef QUOTA
                if (error == EDQUOT)
                   reply_quota_flags(dirvp, error);
                else
#endif QUOTA
                   dm_quick_reply(error);
	}
	else
		dux_copen_serve(&compvp,op->filemode);
}

/*ARGSUSED*/
dux_create_serve(request, dirvp, compvp, opcode, pnp)
dm_message request;
struct vnode *dirvp, *compvp;
int opcode;
struct pathname *pnp;
{
	struct create_request *cp = DM_CONVERT (request, struct create_request);
	register error;

	error = vns_create(pnp,dirvp,&cp->vattr,cp->excl,cp->mode,&compvp);
	if (error)
	{
#ifdef QUOTA
                if (error == EDQUOT)
                   reply_quota_flags(dirvp, error);
                else
#endif QUOTA
                   dm_quick_reply(error);
	}
	else
		dux_copen_serve(&compvp,cp->filemode);
}

/* Common server code for open and create */
/*ARGSUSED*/
dux_copen_serve(vpp,filemode)
	struct vnode **vpp;
	int filemode;
{
	int error;
	register struct inode *ip;
	dm_message reply;
	register struct copen_reply *rp;

	error = vns_copen(vpp,filemode);
	if (error)
	{
		VN_RELE(*vpp);
		dm_quick_reply(error);
	}
	else
	{
		ip = VTOI(*vpp);
		/*If we are serving a remote site for write to a file,
		 *add this site to the bookkeeping for the appropriate
		 *disc, converting to sync I/O if needed.
		 */
		if ((filemode & FWRITE) && ((*vpp)->v_type == VREG))
			mdev_update(ip, u.u_site, 1);
		reply = dm_alloc(sizeof (struct copen_reply), WAIT);
		rp = DM_CONVERT (reply, struct copen_reply);
		pkrmv (*vpp, &rp->pkv);
		if((*vpp)->v_fstype == VUFS) {
		   rp->openreplflags = (ip->i_flag&ISYNC) ? OPENSYNC : 0;
		   if (ip->i_mount->m_flag & M_IS_SYNC)
			rp->openreplflags |= OPENDEVSYNC;
#ifdef QUOTA
                   if (ip->i_dquot)
                   {
                      rp->openreplflags = 
                            (ip->i_dquot->dq_flags & DQ_SOFT_FILES) ?
                            rp->openreplflags | OPENDQOVERSOFT :
                            rp->openreplflags & ~OPENDQOVERSOFT;
                      rp->openreplflags = 
                            (ip->i_dquot->dq_flags & DQ_HARD_FILES) ?
                            rp->openreplflags | OPENDQOVERHARD :
                            rp->openreplflags & ~OPENDQOVERHARD;
                      rp->openreplflags = 
                            (ip->i_dquot->dq_flags & DQ_TIME_FILES) ?
                            rp->openreplflags | OPENDQOVERTIME :
                            rp->openreplflags & ~OPENDQOVERTIME;
		   }
                   else /* quotas off for this inodes filesystem */
                      rp->openreplflags |= OPENDQOFF;
#endif QUOTA
		}
		dm_reply(reply, 0, 0, NULL, NULL, NULL);
	}
}

/* Common unpacking routine for open and create */
/*ARGSUSED*/
int
dux_copen_unpack(request, reply, dirvpp, compvpp, opcode, dependent)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
caddr_t dependent;
#define cop ((struct copenops *)dependent)
{
	register struct copen_reply *rp = DM_CONVERT(reply, struct copen_reply);
	struct inode *ip;
	struct vnode *vp;
	extern struct vnode *stprmv();
	int filemode = cop->filemode;
	int error = 0;
	register struct remvno *rvp;
	extern int dux_clone();		/*forward*/
#ifdef QUOTA
        struct mount *mp;
#endif QUOTA

	rvp = &(rp->pkv);
	if ((vp = stprmv(rvp,NULL)) == NULL) {
		error = u.u_error;
		close_send_no_vno(rvp, filemode);
		return (error);
	}
	if (vp->v_fstype == VDUX) {
	   ip = VTOI(vp);

	   /* Keep track of vnodes we have open that are regular files.
	    * This is so we can keep track of when we do a final close so
	    * we know when to flush all the buffers.
	    */
	   if (vp->v_type == VREG) {
		updatesitecount (&(ip->i_opensites), u.u_site, 1);
	   }

	   if (rp->openreplflags & OPENSYNC)
		ip->i_flag |= ISYNC;
	   if (rp->openreplflags & OPENDEVSYNC)
		discsync(ip->i_mount);
#ifdef QUOTA
           if ((rp->openreplflags & OPENDQOFF) == 0)  
           /* quotas were on for this filesystem at server */
	   {
              if (ip->i_dquot && (ip->i_dquot->dq_uid != 0)) 
              /* Client's i_dquot pointer does exist and it's not for the   *
	       * super user, so see look up flags using it.                 */
	      {
                 if (rp->openreplflags & OPENDQOVERSOFT)
                 {
                    if ((time.tv_sec - ip->i_dquot->dq_ftimelimit > 
                         DQ_MSG_TIME)
                        && (ip->i_uid == u.u_ruid))
	            {
                         mp = VFSTOM(ip->i_vnode.v_vfsp);
	                 uprintf("\nWARNING - too many files (%s)\n",
                                 mp->m_dfs->dfs_fsmnt);
                         /* set time for soft warning issued */
                         ip->i_dquot->dq_ftimelimit = time.tv_sec;
	            }
                 }
                 else
                    /* clear time message issued, since under limits again */
                    ip->i_dquot->dq_ftimelimit = 0;
	      }
	   }
#endif QUOTA
	/* Open the device as necessery
	 * There is, unfortunately, one special case.  If this is a synchronous
	 * Fifo, it has been opened at the server, so do NOT open it now.
	 */
	if (cop->is_open || vp->v_type != VSOCK)
	    error = openi (&vp, filemode, dux_clone);
	}
	if (error)	/* need to release the inode */
	{
		/*if VDUX_CDFS, we should not be here*/
		close_send(ip, filemode);
		VN_RELE(vp);
		return (error);
	}
	else		/*everything OK*/
	{
		*cop->compvpp = vp;
		if(compvpp) *compvpp=vp;
		if (cop->vap != NULL)
			(void)VOP_GETATTR(*cop->compvpp, cop->vap, u.u_cred, VASYNC);
		return (EOPCOMPLETE);
	}
}
#undef cop

/*
 *Service a dux stat request
 */
/*ARGSUSED*/
dux_stat_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	register error;
	dm_message reply;
	struct stat_reply *sp;

	reply=dm_alloc(sizeof(struct stat_reply), WAIT);
	sp = DM_CONVERT(reply, struct stat_reply);
	error = vno_stat(compvp, &sp->stat, u.u_cred, FOLLOW_LINK);
	sp->my_site = my_site;
	VN_RELE(compvp);
	dm_reply(reply, 0, error, NULL, NULL, NULL);
}

/*
 *Unpack a dux Stat reply
 */
/*ARGSUSED*/
int
dux_stat_unpack(request, reply, dirvpp, compvpp, opcode, ub)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
int opcode;
caddr_t ub;
{
	struct stat_reply *sp = DM_CONVERT(reply, struct stat_reply);
	struct mount *mp;
	int error;
	extern struct mount *getrmount();

	mp = getrmount(sp->stat.st_dev,
		sp->stat.st_site?sp->stat.st_site:sp->my_site);
	sp->stat.st_dev = mp->m_dev;
	error = copyout ((caddr_t)&sp->stat, ub, sizeof(struct stat));
	return (error?error:EOPCOMPLETE);
}

/*ARGSUSED*/
dux_access_pack(request,opcode,dependent)
dm_message request;
int opcode;
caddr_t dependent;
{
	struct access_request *ap = DM_CONVERT(request, struct access_request);
	ap->mode = *((u_int *)dependent);
}

/*
 *service a dux access request
 */
/*ARGSUSED*/
dux_access_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	register error = 0;
	register struct access_request *ap =
		DM_CONVERT(request, struct access_request);

	/*if the mode was 0, we just checked for existence.  Since we
	 *are here, the test passed.
	 */
	if (ap->mode)
	{
		/*see if writing to a read only file system */
		if ((ap->mode&VWRITE) && (compvp->v_vfsp->vfs_flag & VFS_RDONLY))
		{
			error = EROFS;
		}
		else
		{
			/*set the uid and gid to the real uids.  (it is not necessary
			 *to save the old values because this is in an NSP with bogus
			 *values in the u area anyway.
			 */
			u.u_uid = u.u_ruid;
			u.u_gid = u.u_rgid;
			/*perform the check*/
			error = VOP_ACCESS(compvp, ap->mode, u.u_cred);
		}
	}
	VN_RELE(compvp);
	dm_quick_reply(error);
}

/*ARGSUSED*/
dux_namesetattr_pack(request, opcode, dependent)
dm_message request;
int opcode;
caddr_t dependent;
{
	struct namesetattr_request *nsp =
		DM_CONVERT (request, struct namesetattr_request);

	nsp->vattr = *((struct setattrops *)dependent)->vap;
	nsp->null_time = ((struct setattrops *)dependent)->null_time; /*kludge*/
}

/*ARGSUSED*/
dux_namesetattr_serve(request,dirvp,compvp,opcode)
dm_message request;
struct vnode *dirvp, *compvp;
int opcode;
{
	register error;
	register struct namesetattr_request *nsp =
		DM_CONVERT(request, struct namesetattr_request);

	if (compvp->v_vfsp->vfs_flag & VFS_RDONLY)
		error = EROFS;
	else
	{
		error = VOP_SETATTR(compvp, &nsp->vattr, u.u_cred,
			nsp->null_time);
	}
	VN_RELE(compvp);
	dm_quick_reply(error);
}

/* pack a remove (unlink/rmdir) request */
/*ARGSUSED*/
dux_remove_pack(request, opcode, dependent)
dm_message request;
int opcode;
caddr_t dependent;
{
	struct remove_request *up = DM_CONVERT (request, struct remove_request);

	up->dirflag = *(enum rm *)dependent;
}

/* service a remove request */
/*ARGSUSED*/
dux_remove_serve(request, dirvp, compvp, opcode, pnp)
dm_message request;
struct vnode *dirvp, *compvp;
int opcode;
struct pathname *pnp;
{
	struct remove_request *up = DM_CONVERT (request, struct remove_request);

	dm_quick_reply(vns_remove(compvp, dirvp, pnp, up->dirflag));
}

	/* This routine isn't used.  Should it be?  Its not used in 8.0
	 * either.  cwb
	 */
#ifdef notdef
#ifdef QUOTA
/* unpacking routine for mknod if quotas are configured in the system */
/*ARGSUSED*/
int
dux_mknod_unpack(request, reply, dirvpp, compvpp, opcode, dependent)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
caddr_t dependent;

{
   extern struct mount *getrmp();
   struct dquot *dquotp;
   struct mount *mp;
   struct quota_reply *qrp;
          
   qrp = DM_CONVERT (reply, struct quota_reply);
   if ((qrp->dqreplflags & DQOFF) == 0)  
   /* quotas were on for this filesystem at server */
   {
      mp = getrmp(qrp->rdev, qrp->site);
           
      if ((mp != NULL) && !getdiskquota(u.u_ruid, mp, 0, &dquotp))
      {
         if (qrp->dqreplflags & DQOVERSOFT)
         {
            if ((time.tv_sec - dquotp->dq_ftimelimit > DQ_MSG_TIME)
                && (dquotp->dq_uid == u.u_ruid)
                && (dquotp->dq_uid != 0))
            {
	       uprintf("\nWARNING - too many files (%s)\n",
                        mp->m_dfs->dfs_fsmnt);
               /* set time for soft warning issued */
               dquotp->dq_ftimelimit = time.tv_sec;
	    }
         }
         else if (dquotp->dq_uid != 0)
            /* clear time message issued, since under limits again */
            dquotp->dq_ftimelimit = 0;

         dqput(dquotp);
      }
   }
   return(EOPCOMPLETE);
}
#endif QUOTA
#endif /* notdef */

/* service a mknod/mkdir request */
/*ARGSUSED*/
dux_mknod_serve(request, dirvp, compvp, opcode, pnp)
dm_message request;
struct vnode *dirvp, *compvp;
int opcode;
struct pathname *pnp;
{
	struct create_request *cp = DM_CONVERT (request, struct create_request);
	register error;

	error = vns_create(pnp,dirvp,&cp->vattr,cp->excl,cp->mode,&compvp);
	if (!error)
        {
		VN_RELE(compvp);
#ifdef QUOTA
                reply_quota_flags(dirvp, error);
#else not QUOTA
        	dm_quick_reply(error);
#endif QUOTA
	}
        else
	{
#ifdef QUOTA
                if (error == EDQUOT)
                   reply_quota_flags(dirvp, error);
                else
#endif QUOTA
                   dm_quick_reply(error);
        }
}

/* pack a link request */
/*ARGSUSED*/
dux_link_pack(request, opcode, dependent)
dm_message request;
int opcode;
caddr_t dependent;
{
	register struct link_request *lp =
		DM_CONVERT (request, struct link_request);
	register struct vnode *vp;
	register struct inode *ip;
	register struct cdnode *cdp;

	vp = (struct vnode *)dependent;
	if (vp->v_fstype == VDUX) {
		ip = VTOI(vp);
		lp->dev = ip->i_dev;
		lp->ino = ip->i_number;
		lp->fstype = MOUNT_UFS;
	}
	else if (vp->v_fstype == VDUX_CDFS) {
		cdp = VTOCD(vp);
		lp->dev = cdp->cd_dev;
		lp->cdno = cdp->cd_num;
		lp->fstype = MOUNT_CDFS;
	}
}

/* service a link request */
/*ARGSUSED*/
dux_link_serve(request, tdvp, compvp, opcode, pnp)
dm_message request;
struct vnode *tdvp, *compvp;
int opcode;
struct pathname *pnp;
{
	struct link_request *lp = DM_CONVERT (request, struct link_request);
	register error;
	struct inode *fip;
	register struct vnode *fvp;

	struct cdnode *fcdp;
	/* Is the from inode on this site?  Is the vnode valid? */
	if (devsite(lp->dev) != my_site) {
		error = EXDEV;
		goto out;
	}
	if (lp->fstype == MOUNT_UFS) {
		if ((fip = ifind (localdev(lp->dev), lp->ino)) == NULL) {
			error = EXDEV;
			goto out;
		}
		fvp = ITOV(fip);
		if (fip -> i_nlink == 0) {
			error = EREMOVE;
			goto out;
		} 
	}
	else if (lp->fstype == MOUNT_CDFS) {
		if ((fcdp = (struct cdnode *) CDFSCALL(CDFIND) 
				(localdev(lp->dev), lp->cdno)) == NULL) {
			error = EXDEV;
			goto out;
		}
		fvp = CDTOV(fcdp);
	}
	/* Note:  The following code is taken almost verbatim from vn_link.
	 * It could be combined with it in a single function.
	 */
	/*
	 * Make sure both source vnode and target directory vnode are
	 * in the same vfs and that it is writeable.
	 */
	if (fvp->v_vfsp != tdvp->v_vfsp) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * do the link
	 */
	error = VOP_LINK(fvp, tdvp, pnp->pn_path, u.u_cred);
out:
#ifdef	NOTDEF
	/* If there was no error, we can release the from vnode here.  If
	 * there was an error, we require the client site to send a separate
	 * release message to whoever owns the from vnode.  This is bacause
	 * if there was an error, the vnode will not necessarily be here.
	 * (In some cases, we could release the vnode, however errors are
	 * infrequent, and performance is irrelevent in error cases, so why
	 * bother.)
	 */
	if (!error)
	{
	     /* updatesitecount (&(fip->i_refsites), u.u_site, -1); */
		VN_RELE(fvp);
	}
#endif	NOTDEF
	if (tdvp)
		VN_RELE(tdvp);
	dm_quick_reply(error);
}

/*
 *Service a dux statfs request
 */
/*ARGSUSED*/
dux_statfs_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	register error;
	dm_message reply;
	struct statfs_reply *sp;

	reply=dm_alloc(sizeof(struct statfs_reply), WAIT);
	sp = DM_CONVERT(reply, struct statfs_reply);
	error = VFS_STATFS(compvp->v_vfsp, &sp->statfs);
	VN_RELE(compvp);
	dm_reply(reply, 0, error, NULL, NULL, NULL);
}

/*
 *Unpack a dux statfs reply
 */
/*ARGSUSED*/
int
dux_statfs_unpack(request, reply, dirvpp, compvpp, opcode, ub)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
int opcode;
caddr_t ub;
{
	struct statfs_reply *sp = DM_CONVERT(reply, struct statfs_reply);
	int error;

	error = copyout ((caddr_t)&sp->statfs, ub, sizeof(struct statfs));
	return (error?error:EOPCOMPLETE);
}

#ifdef ACLS

/* L K U P _ S E T A C L _ P A C K
 *
 * Package up a setacl lookup request. We need to send over both the acl
 * itself and the tuple count. These were passed down in the setacl_ops.
 */

/*ARGSUSED*/
lkup_setacl_pack(request, opcode, setacl_ops)
dm_message request;
int opcode;
register struct setaclops *setacl_ops;
{
	register int i;
	register struct setacl_request *srqp;

	srqp = DM_CONVERT(request, struct setacl_request);
	srqp->ntuples = setacl_ops->ntuples;
	for (i=0; i<NACLTUPLES; i++) {
		srqp->acl[i].uid = setacl_ops->acl[i].uid;
		srqp->acl[i].gid = setacl_ops->acl[i].gid;
		srqp->acl[i].mode = setacl_ops->acl[i].mode;
	}
}

/* L K U P _ S E T A C L _ S E R V E
 *
 * service a setacl lookup request. The request contains the acl and the
 * number of tuples. The service routine then simply call the vnodeop to
 * handle the request. The reply returns any error found.
 */

/*ARGSUSED*/
lkup_setacl_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	dm_message reply;
	struct setacl_request *srqp;
	struct acl_tuple acl_list[NACLTUPLES];
	int i;

	srqp = DM_CONVERT(request, struct setacl_request);
	reply=dm_alloc(0, WAIT);
	
        for (i=0; i<NACLTUPLES; i++) {
                acl_list[i].uid = srqp->acl[i].uid;
                acl_list[i].gid = srqp->acl[i].gid;
                acl_list[i].mode = srqp->acl[i].mode;
        }
	VOP_SETACL(compvp, srqp->ntuples, acl_list);
	VN_RELE(compvp);
	dm_reply(reply, 0, u.u_error, NULL, NULL, NULL);
}

/* L K U P _ G E T A C L _ P A C K
 *
 * Package up a lookup getacl request. Send over the value of ntuples
 * that was passed in as part of getacl_ops.
 */

/*ARGSUSED*/
lkup_getacl_pack(request, opcode, getacl_ops)
dm_message request;
int opcode;
struct getaclops *getacl_ops;
{
	struct getacl_request *grqp;

	grqp = DM_CONVERT(request, struct getacl_request);
	grqp->ntuples = getacl_ops->ntuples;
}

/* L K U P _ G E T A C L _ S E R V E
 *
 * Service a dux getacl lookup request. The request contains the value of
 * ntuples that was passed in. This is sent to the routine that does the
 * getacl. The acl to be returned is placed in the reply message. Any
 * error to be returned is contained in the reply message as well.
 */

/*ARGSUSED*/
lkup_getacl_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	dm_message reply;
	struct getacl_reply *grpp;
	struct getacl_request *grqp;
	struct acl_tuple acl[NACLTUPLES];
   	register int i;

	grqp = DM_CONVERT(request, struct getacl_request);
	reply=dm_alloc(sizeof(struct getacl_reply), WAIT);
	grpp = DM_CONVERT(reply, struct getacl_reply);
	grpp->tuple_count = 
		vno_getacl(compvp, grqp->ntuples, acl);
	if (grqp->ntuples && !u.u_error)
	{
		for (i=0; i<grqp->ntuples; i++) {
			grpp->acl[i].uid = acl[i].uid;
			grpp->acl[i].gid = acl[i].gid;
			grpp->acl[i].mode = acl[i].mode;
		}
	}	
	VN_RELE(compvp);
	dm_reply(reply, 0, u.u_error, NULL, NULL, NULL);
}

/* L K U P _ G E T A C L _ U N P A C K
 *
 * Unpack a dux getacl reply. If the number of tuples passed down in 
 * getacl_ops was non-zero we copy out the acl returned in the reply
 * message to the address which was passed down in the getacl_ops.
 * Any errors returned by the server were detected above this. The
 * copyout generate an EACESS if given a bad address.
 */

/*ARGSUSED*/
int
lkup_getacl_unpack(request, reply, dirvpp, compvpp, opcode, getacl_ops)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
int opcode;
struct getaclops *getacl_ops;
{
	struct getacl_reply *grpp;
	register int error = 0;
	struct acl_tuple_user tmp_tupleset[NACLTUPLES];
	int i;

	grpp = DM_CONVERT(reply, struct getacl_reply);
	if (getacl_ops->ntuples) {
                for (i=0; i<getacl_ops->ntuples; i++){
                        tmp_tupleset[i].uid=grpp->acl[i].uid;
                        tmp_tupleset[i].gid=grpp->acl[i].gid;
                        tmp_tupleset[i].mode=grpp->acl[i].mode;
		}
		error = copyout(tmp_tupleset, getacl_ops->tupleset,
			getacl_ops->ntuples * sizeof (struct acl_tuple_user));
	}

	/*
	 * I am setting the return value here. This means that it
	 * must not be set by any routines above me.
	 */

	u.u_r.r_val1 = grpp->tuple_count;
	return (error?error:EOPCOMPLETE);
}

/* L K U P _ G E T A C C E S S _ S E R V E
 *
 * service a getaccess lookup request. vno_getaccess sets the mode. We
 * then release the vnode and make our reply
 */

/*ARGSUSED*/
lkup_getaccess_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	register int mode;
	dm_message reply;
	struct getaccess_reply *gp;

	reply=dm_alloc(sizeof(struct getaccess_reply), WAIT);
	gp = DM_CONVERT(reply, struct getaccess_reply);
	mode = vno_getaccess(compvp);
	if (!u.u_error)
		gp->mode = mode;

	VN_RELE(compvp);
	dm_reply(reply, 0, u.u_error, NULL, NULL, NULL);
}

/* L K U P _ G E T A C C E S S _ U N P A C K
 *
 * unpack the getacces lookup request
 */

/*ARGSUSED*/
int
lkup_getaccess_unpack(request, reply, dirvpp, compvpp)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
{
	struct getaccess_reply *gp;

	gp = DM_CONVERT(reply, struct getaccess_reply);
	u.u_r.r_val1 = gp->mode;
	return (EOPCOMPLETE);
}
#endif /* ACLS */

#ifdef	POSIX
/*ARGSUSED*/
lkup_pathconf_pack(request, opcode, pathconf_ops)
dm_message request;
int opcode;
int pathconf_ops;
{
	struct pathconf_request *prqp;

	prqp = DM_CONVERT(request, struct pathconf_request);
	prqp->name = pathconf_ops;
}

/*
 *Service a dux pathconf lookup request. The request contains the value of
 * name that was passed in. This is sent to the routine that does the
 * pathconf. The result to be returned is placed in the reply message. Any
 * error to be returned is contained in the reply message as well.
 */

/*ARGSUSED*/
lkup_pathconf_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	dm_message reply;
	struct pathconf_reply *prpp;
	struct pathconf_request *prqp;
	int error;

	prqp = DM_CONVERT(request, struct pathconf_request);
	reply=dm_alloc(sizeof(struct pathconf_reply), WAIT);
	prpp = DM_CONVERT(reply, struct pathconf_reply);
	error = VOP_PATHCONF(compvp, prqp->name, &(prpp->result), 0);
	VN_RELE(compvp);
	dm_reply(reply, 0, error, NULL, NULL, NULL);
}

/*
 * Unpack a dux pathconf reply. If the number of tuples passed down in 
 * pathconf_ops was non-zero we copy out the acl returned in the reply
 * message to the address which was passed down in the pathconf_ops.
 * Any errors returned by the server were detected above this. The
 * copyout generate an EACESS if given a bad address.
 */

/*ARGSUSED*/
int
lkup_pathconf_unpack(request, reply, dirvpp, compvpp, opcode, dependent)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
int opcode;
caddr_t dependent;
{
	struct pathconf_reply *prpp;

	prpp = DM_CONVERT(reply, struct pathconf_reply);

	u.u_r.r_val1 = prpp->result;
	return (EOPCOMPLETE);
}
#endif	POSIX

/*
 * This function is called whenever a multiple open occurs in openi on a
 * character devices, returning a new minnum that requires creation of a
 * new temporary dux inode.
 * Grab an empty inode, and copy all the necessary fields in.  This
 * temporary inode is the same as the original inode except:
 *   1)  It is not on a hash chain, so it will never be found
 *   2)  Its reference count is 1 (The original inode is VN_RELE'd decrementing
 *       its reference count
 *   3)  It has the new device number specified by minnum
 * Note that since both the original and the clone inode refer to the same
 * inode number, and that the total number of references is the same as
 * the number of opens, the server's inode will not be released until both
 * copies of the inode are closed.
 */
/*ARGSUSED*/
int
dux_clone(ip,nipp,flag,minnum)
register struct inode *ip;
struct inode **nipp;
int flag;
int minnum;
{
	register struct inode *nip;
	register struct vnode *nvp,*vp;
	extern struct inode *eiget();

	do
		nip = eiget(NULL);
	while (nip == (struct inode *)-1);
	if (nip == NULL)
		return (u.u_error);
	if (ilocked(nip))
		panic("dux_clone");
	nip->i_flag = IREF;
	ilock(nip);
	nvp = ITOV(nip);
	vp = ITOV(ip);
	nip->i_pid = u.u_procp->p_pid;
	nip->i_dev = ip->i_dev;
	if (nip->i_devvp) {
		VN_RELE(nip->i_devvp);
		nip->i_devvp = NULL;
	}
	nip->i_devvp = devtovp(ip->i_dev);
	nip->i_fs = ip->i_fs;
	nip->i_dfs = ip->i_dfs;
	nip->i_number = ip->i_number;
	nip->i_lastr = 0;
	nip->i_mode = ip->i_mode;
	nip->i_nlink = ip->i_nlink;
	nip->i_uid = ip->i_uid;
	nip->i_gid = ip->i_gid;
	nip->i_blocks = ip->i_blocks;
#ifdef QUOTA
        nip->i_dquot = ip->i_dquot;
#endif QUOTA
	if (ip->i_device != NODEV) {
		nvp->v_rdev = nip->i_rdev = minnum;
	} else if ((ip->i_mode & IFMT) == IFBLK) {
		nvp->v_rdev = nip->i_rdev =
			makedev(major(rootdev), minnum);
	} else {
		nvp->v_rdev = nip->i_rdev =
			makedev(major(block_to_raw(rootdev)), minnum);
	}

	nip->i_mount = ip->i_mount;
	nip->i_size = ip->i_size;
	nip->i_fversion = ip->i_fversion;
	nvp->v_flag = vp->v_flag;
	nvp->v_count = 1;
	nvp->v_vfsp = vp->v_vfsp;
	nvp->v_type = vp->v_type;
	nvp->v_shlockc = nvp->v_exlockc = 0;
	nvp->v_op = vp->v_op;
	nvp->v_fstype = vp->v_fstype;
	VN_RELE(vp);
	iunlock(nip);
	*nipp = nip;
	return (0);
}
