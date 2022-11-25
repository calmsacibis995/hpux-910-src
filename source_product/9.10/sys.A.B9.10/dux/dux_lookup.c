/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_lookup.c,v $
 * $Revision: 1.8.83.5 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/25 17:26:47 $
 */

/* HPUX_ID: @(#)dux_lookup.c	55.1		88/12/23 */

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
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/pathname.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/devices.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_lookup.h"
#include "../dux/lookup_def.h"
#include "../dux/lookupops.h"
#include "../dux/lkup_dep.h"
#include "../h/mount.h"
#include "../dux/dux_dev.h"
#include "../dux/nsp.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../cdfs/cdfs_hooks.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#include "../ufs/fs.h"
#include "../dux/lookupmsgs.h"
#include "../dux/duxfs.h"
#endif QUOTA

/*lookup reply message when operation must be continued*/

/*
 *Pack a ucred structure into the passed packed_ucred structure
 */
pack_ucred(ucred,pu)
struct ucred *ucred;
struct packed_ucred *pu;
{
	pu->puc_uid = ucred->cr_uid;
	pu->puc_gid = ucred->cr_gid;
	pu->puc_ruid = ucred->cr_ruid;
	pu->puc_rgid = ucred->cr_rgid;
	bcopy (ucred->cr_groups, pu->puc_groups, sizeof (ucred->cr_groups));
}

/*
 *unpack a packed ucred structure into a real ucred structure
 */
unpack_ucred(pu,ucred)
struct packed_ucred *pu;
struct ucred *ucred;
{
	ucred->cr_uid = pu->puc_uid;
	ucred->cr_gid = pu->puc_gid;
	ucred->cr_ruid = pu->puc_ruid;
	ucred->cr_rgid = pu->puc_rgid;
	bcopy (pu->puc_groups, ucred->cr_groups, sizeof (ucred->cr_groups));
}

/*
 *Pack the fields in the u area needed for lookup operations into the
 *passed lk_usave structure
 *NOTE:  This does not include the root directory.  That is only packed
 *when sending to a site that includes it.
 */
lookup_upack(usave)
register struct lk_usave *usave;
{
	pack_ucred(u.u_cred, &usave->lk_ucred);
	usave->lk_cmask = u.u_cmask;
	bcopy (u.u_rlimit, usave->lk_rlimit, sizeof (u.u_rlimit));
	bcopy (u.u_cntxp, &(usave->lk_context), sizeof(struct dux_context));
}

/*
 *Unpack the fields in the u area needed for lookup operations from the
 *passed lk_usave structure.  Use the passed ucred structure to store the
 *ucred info.
 */
lookup_uunpack(usave,ucred)
register struct lk_usave *usave;
register struct ucred *ucred;
{
	register add;
	register char **cntxpp;
 
	unpack_ucred(&usave->lk_ucred, ucred);
	u.u_cred = ucred;
	bcopy (usave->lk_rlimit, u.u_rlimit, sizeof (u.u_rlimit));
	u.u_cmask = usave->lk_cmask;

/* adjust all the context pointers to make sense on this machine */
	add = &(usave->lk_context.buf[0]) - usave->lk_context.ptr[0];
	u.u_cntxp = cntxpp = usave->lk_context.ptr;
	do {
		*cntxpp += add;
	} while (**(cntxpp++) != '\0');
}

static struct lk_backup
{
	struct rlimit lkb_rlimit[RLIM_NLIMITS];
	short lkb_cmask;
}backup_u;

/*
 *Make a backup copy of the area of the u area modified by lookup.
 *This is called by the first NSP to be created.
 */
backup_ulookup()
{
	backup_u.lkb_cmask = u.u_cmask;
	bcopy (u.u_rlimit, backup_u.lkb_rlimit, sizeof (u.u_rlimit));
	/* No need to bother SDOs, since they are never referenced except
	 * for lookup, and the correct values are always provided.
	 * Also, the ucred area, which has a count, must be backed up
	 * independently for each NSP
	 */
}

/*
 *Restore the u area to its pristine state.
 *Called after every lookup service.
 */

restore_ulookup()
{
	u.u_cred = u.u_nsp->nsp_cred;
	u.u_cmask = backup_u.lkb_cmask;
	u.u_rdir = NULL;
	bcopy (backup_u.lkb_rlimit, u.u_rlimit, sizeof (u.u_rlimit));
}

#ifdef QUOTA
/*
 *Determine if the user should be given a message
 *telling that they are over quota.  Also update flags for that user.
 */

unpack_quota_flags(reply)
dm_message reply;
{
   /* handle over-file-quota messages here */
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
         if ((qrp->dqreplflags & DQOVERHARD)
             && (dquotp->dq_uid == u.u_ruid))
         {
            uprintf("\nFILE LIMIT REACHED - CREATE FAILED (%s)\n",
                    mp->m_dfs->dfs_fsmnt);
         }
         else 
         if ((qrp->dqreplflags & DQOVERTIME)
             && (dquotp->dq_uid == u.u_ruid))
         {
            uprintf("\nDISK LIMIT REACHED (%s) - WRITE FAILED\n",
                    mp->m_dfs->dfs_fsmnt);
         }

         dqput(dquotp); 
      }
   }
}
#endif QUOTA

/*
 *send a lookup request along with the opcode information to the appropriate
 *site.
 */
dux_pathsend(vpp,pnp,followlink,nlinkp,dirvpp,compvpp,opcode,dependent)
	struct vnode **vpp;
	struct pathname *pnp;
	enum symfollow followlink;
	int *nlinkp;
	struct vnode **dirvpp;
	struct vnode **compvpp;
	int opcode;
	caddr_t dependent;
{
	dm_message request,reply;
	register struct dux_lookup_request *lkp;
	struct buf *bp;
	register struct inode *ip = VTOI(*vpp);
	register struct dux_lookup_reply *rp;
	int error;
	register dev_t dev;
	site_t site;
	ino_t ino;
	register struct cdnode *cdp = VTOCD(*vpp);
	cdno_t cdno;
	register int is_cdfs;

	/* allocate the request */
	request = dm_alloc(lookup_ops[opcode].lk_request_size,WAIT);
	lkp = DM_CONVERT(request, struct dux_lookup_request);
	/* Fill in the invariant fields of the request */
	lkp->lk_opcode = opcode;
	lkp->lk_flags = 0;
	if (followlink)
		lkp->lk_flags |= LK_FOLLOWLINK;
	if (dirvpp)
		lkp->lk_flags |= LK_DIR;
	if (compvpp)
		lkp->lk_flags |= LK_COMP;
	lookup_upack (&lkp->lk_usave);
	if ((*vpp)->v_fstype == VDUX_CDFS) {
		is_cdfs = 1;
		dev = cdp->cd_dev;
		site = devsite(cdp->cd_dev);
		cdno = cdp->cd_num;
	}
	else {
		is_cdfs = 0;
		dev = ip->i_dev;
		site = devsite(ip->i_dev);
		ino = ip->i_number;
	}
	/*
	 * call the opcode dependent function to fill in the rest of the
	 * message
	 */
	if (lookup_ops[opcode].lk_request_pack)
		(*(lookup_ops[opcode].lk_request_pack))
			(request, opcode, dependent);
	/*release the vnode*/
	VN_RELE(*vpp);
	/*Unfortuately, the pathname is not in a buffer, and all we can send
	 *(and receive replies in) are buffers.  Therefore, copy the pathname
	 *to a buffer.  (We may want to look into trying to avoid this in the
	 *future)  Make sure and copy the trailing NULL.
	 */
	bp = geteblk(MAXPATHLEN);
	bcopy(pnp->pn_path, bp->b_un.b_addr, pnp->pn_pathlen+1);
	if ((pnp->pn_pathlen == 0) && dirvpp && *dirvpp) {
		lkp->lk_flags |= LK_MNT;

		/* This is going to get bounced to the site that has
		 * the local disc, so update the sitecount for that site.
		 */
		update_vnode_refcount(*dirvpp, site, 1);
		pkrmv (*dirvpp, &lkp->lk_dirvp);
	}

	/*
	 * If we receive a request to bounce to another site,
	 * we bounce to here.
	 */
bounce:
	/*fill in the recomputable part of the request*/
	if (is_cdfs) {
		lkp->lk_dev = dev;
		lkp->lk_cdno = cdno;
		lkp->lk_nlink = *nlinkp;
		lkp->lk_fstype = MOUNT_CDFS;
	}
	else {
		lkp->lk_dev = dev;
		lkp->lk_ino = ino;
		lkp->lk_nlink = *nlinkp;
		lkp->lk_fstype = MOUNT_UFS;
	}
	/* If the root is at the server (but isn't /) send it along */
	if (u.u_rdir && u.u_rdir->v_fstype == VDUX &&
		devsite ((VTOI(u.u_rdir))->i_dev) == site)
	{
		lkp->lk_rdev = (VTOI(u.u_rdir))->i_dev;
		lkp->lk_rino = (VTOI(u.u_rdir))->i_number;
		lkp->lk_rfstype = MOUNT_UFS;
	}
	else if (u.u_rdir && u.u_rdir->v_fstype == VDUX_CDFS &&
		devsite ((VTOCD(u.u_rdir))->cd_dev) == site)
	{
		lkp->lk_rdev = (VTOCD(u.u_rdir))->cd_dev;
		lkp->lk_rcdno = (VTOCD(u.u_rdir))->cd_num;
		lkp->lk_rfstype = MOUNT_CDFS;
	}
	else
		lkp->lk_rdev = NODEV;
	reply = dm_send(request,
		DM_SLEEP|DM_REQUEST_BUF|DM_REPLY_BUF|DM_INTERRUPTABLE,
		DM_LOOKUP, site, lookup_ops[opcode].lk_reply_size,
		NULL, bp, pnp->pn_pathlen+1, 0,
		bp, MAXPATHLEN, 0);
	error = DM_RETURN_CODE(reply);
	if (error == 0)
	{
		if (dirvpp)
			*dirvpp = NULL;
		if (compvpp)
			*compvpp = NULL;
		if (lookup_ops[opcode].lk_reply_unpack) {
			error = (*(lookup_ops[opcode].lk_reply_unpack))
				(request, reply, dirvpp, compvpp,
				opcode, dependent);
	/* if there is no error,
	   we reset the pnp with the last component of the path.  */
			if (!error) {
				pn_setlastcomp(pnp);
			}
		}
		else {
			error = EOPCOMPLETE;
		}
	}
	else if (error == EPATHREMOTE)
	{
		/* Need to add code for non UFS file systems */
		rp = DM_CONVERT(reply, struct dux_lookup_reply);
		*nlinkp = rp->lkr_nlink;
		if (rp->lkr_vnodetype == MOUNT_UFS)
		{
			dev = rp->lkr_dev;
			site = devsite(dev);
			ino = rp->lkr_ino;
			if (site == my_site)
			{
				struct mount *mp;

				dev = localdev(dev);
				mp = getmp(dev);
				if (mp == NULL)	/*someone unmounted under us?*/
				{
					error = ENOENT;
					ip = NULL;
				}
				else
				{
					ip = iget(dev, mp, ino);
					if (ip == NULL)	/* shouldn't happen */
					{
						error = u.u_error;
					}
					else
					{
						iunlock(ip);
						*vpp = ITOV(ip);
						pn_set(pnp,bp->b_un.b_addr);
					}
				}
				if (rp -> lkr_mntdvp_flg)
				{
				      struct remvno *remdvp;
				      struct vnode  *vip;
				      struct vnode  *stprmv();
				   
				      remdvp = &(rp -> lkr_dirvp);
				      if ((vip = stprmv(remdvp,NULL)) == NULL) {
				           send_ref_update_no_remvno(remdvp);
				   	   error = u.u_error;
				           if (ip != NULL) {
					       VN_RELE(ITOV(ip));
				           }
				      }
				      else {
					   if (ip == NULL) {
						/* Error, from above.
						 * Free vip back up.
						 */
						VN_RELE(vip);
					   }
					   else {
				   	        /* Update the reference double count */
				   	        update_duxref(vip,1,1);
				   		*dirvpp = vip; 
						if (compvpp) {
						    *compvpp = ITOV(ip);
						}
						else {
						    /* Make sure we
						     * release ip if we're
						     * not using it
						     */
						    VN_RELE(ITOV(ip));
						}
				   		error = 0;
					   }
				      }
				} /* (rp -> lkr_mntdvp_flg) */
			}
			else /* site == my_site */
			{
				if (rp -> lkr_mntdvp_flg) {
					lkp->lk_flags |= LK_MNT;
					lkp->lk_dirvp = rp->lkr_dirvp;
				}
				dm_release(reply,0);
				/*
				 * Note, reply filled the buffer with the new
				 * pathname so we can send it off again
				 * unchanged.  However, we need to reset
				 * the pn structure to reflect our new path.
				 */
				pn_set(pnp,bp->b_un.b_addr);
				goto bounce;
			}
		}
		else if (rp->lkr_vnodetype == MOUNT_CDFS) {
			panic("pathsend: local mount for cdfs is not yet implemented");
		}
		else if (rp->lkr_vnodetype == MOUNT_NFS)
		{
			struct vfs *vfsp;
			struct rnode *rnp;

			vfsp = (struct vfs *)
				NFSCALL(NFS_FIND_MNT)(rp->lkr_mntno); 
			if (vfsp == NULL)
			{
				error = ENOENT;
			}
			else
			{
				rnp = (struct rnode *)
				      NFSCALL(NFS_RFIND)(&rp->lkr_fhandle,vfsp);
				if (rnp == NULL)
				{
					error = ENOENT;
				}
				else
				{
					*vpp = rtov(rnp);
					pn_set(pnp,bp->b_un.b_addr);
					/* Just for the automounter */
					if (((*vpp)->v_type == VLNK) &&
					    ((*vpp)->v_flag & VROOT) &&
					    ((followlink == FOLLOW_LINK) ||
					     pn_pathleft(pnp))) {
						struct pathname lnkpnp;
						struct pathname tmp_pnp;
						
						if (++(*nlinkp) > MAXSYMLINKS){
							error = ELOOP;
							goto bad_link;
						}

						/* Read the symbolic link from
						 * the automounter.
						 */
						error = getsymlink(*vpp,
							&lnkpnp);
						if (error)
							goto bad_link;

						if (pn_pathleft(&lnkpnp) == 0)
							(void) pn_set(&lnkpnp,
								".");
						pn_alloc(&tmp_pnp);
						/* We've already lost the
						 * slash so we have to add
						 * it back on.  What fun.
						 */
						pn_set(&tmp_pnp,"/");
						error = pn_combine(&tmp_pnp,
								&lnkpnp);
						pn_free(&lnkpnp);
						if (!error)
							error = pn_combine(pnp,
									&tmp_pnp);
						pn_free(&tmp_pnp);
						if (!error) {
						    /* Check for starting with
						     * '/'
						     */
						    rootcheck(pnp, vpp);
						    if (pathisremote(*vpp))
							error = EPATHREMOTE;
						}
						else {
				bad_link:
						    VN_RELE(*vpp);
						     *vpp = NULL;
						}
					}
						
/*Yuk! In case of pure pathname lookup of a nfs mount point (the last component
  is a nfs mount), we have gotten the compvpp we need.  We return to caller,
  which is lookuppn, with error code 0.  So lookuppn() will not try to parse
  the next component.  Since the server did not pass the needed info for dirvpp,
  we don't any thing here for it.  This is a kludge.  Fortunately,  vn_rename()
  is the only one who needs dirvpp in the case of pure pathname lookup.  If 
  we try to rename a nfs mount point, EXDEV should be reported to the user.
  Since dirvpp contains 0, vn_rename() will return EXDEV.   This is not the
  fix I should have done, but the fix involve a redesign of dux/nfs remote 
  pathname lookup. */
					else if ((pnp->pn_path[0] == '\000') &&
					    (opcode == LKUP_LOOKUP)) {
						*compvpp = *vpp;
						error = 0;
					}
				}
			}
		}
	}
#ifdef QUOTA
        else if (error == EDQUOT)
	{  
           unpack_quota_flags(reply);
        }
#endif QUOTA
	dm_release(request,1);
	dm_release(reply,0);
	return (error);
}

	int dux_used_pn = 0;
	int dux_used_buf = 0;
/*
 * Process an incoming lookup request.  If the component is found, perform
 * the operation.
 */
dux_pathrecv(request)
dm_message request;
{
	dm_message reply;
	register struct dux_lookup_request *lkp;
	register struct dux_lookup_reply *rp;
	struct buf *bp;
	struct inode *ip;
	struct vnode *vp;
	struct pathname pn;
	register error;
	struct vnode *dirvp=0;
	struct vnode *compvp=0;
	dev_t dev;
	struct mount *mp;
	struct inode *rip;
	struct ucred ucred;
	struct cdnode *rcdp;
	struct cdnode *cdp;
	int	is_cdfs;
	int     pn_allocated;

	lkp = DM_CONVERT(request, struct dux_lookup_request);
	if (lkp->lk_fstype == MOUNT_CDFS) is_cdfs=1;
	else if (lkp->lk_fstype == MOUNT_UFS) is_cdfs=0;
	else panic("pathrecv: illegal fs type");

	bp = DM_BUF(request);
	lookup_uunpack(&lkp->lk_usave, &ucred);
	/* set up the root vnode if needed */
	if (lkp->lk_rdev != NODEV)
	{
	   if(lkp->lk_rfstype == MOUNT_CDFS) {
		rcdp = (struct cdnode *)CDFSCALL(CDFIND)
			(localdev(lkp->lk_rdev), lkp->lk_rcdno);
		if (rcdp != NULL)
			u.u_rdir = CDTOV(rcdp);
	   }
	   else if(lkp->lk_rfstype == MOUNT_UFS) {
		rip = ifind (localdev(lkp->lk_rdev), lkp->lk_rino);
		if (rip != NULL)
			u.u_rdir = ITOV(rip);
	   }
	   else panic("pathrecv: illegal root fs type");
	}
	dev = localdev(lkp->lk_dev);
	mp = getmp(dev);
	if (mp==NULL)	/*Not found; someone probably unmounted under us*/
	{
		dm_quick_reply(ENOENT);
		goto out;
	}
	if (is_cdfs) {
		if(mp->m_vfsp->vfs_mtype != MOUNT_CDFS) {
			 panic("pathrecv:wrong fs type, should be cdfs");
		}
		cdp = (struct cdnode *)CDFSCALL(CDGET)(dev,
			mp->m_bufp->b_un.b_fs, lkp->lk_cdno);
		if (cdp==NULL)	/*shouldn't happen*/
		{
			dm_quick_reply(ENOENT);
			goto out;
		}
		CDFSCALL(CDUNLOCK_PROC)(cdp);
		vp = CDTOV(cdp);
	}
	else {
		if(mp->m_vfsp->vfs_mtype != MOUNT_UFS) {
			 panic("pathrecv:wrong fs type, should be ufs");
		}
		ip = iget(dev, mp, lkp->lk_ino);
		if (ip==NULL)	/*shouldn't happen*/
		{
			dm_quick_reply(ENOENT);
			goto out;
		}
		iunlock(ip);
		vp = ITOV(ip);
	}


       if (bp->b_bufsize > MAXPATHLEN) {

            /* Make a pn out of all the extra space we have in this buffer */
            pn.pn_buf = (char *)bp->b_un.b_addr;
            pn.pn_path = pn.pn_buf;
            pn.pn_pathlen = strlen(pn.pn_path);
            pn_allocated = 0;
            dux_used_buf++;

            error = 0;
        }
        else {
            /*copy the pathname from the buffer to a pnp (yuk)*/
            error = pn_get(bp->b_un.b_addr, UIOSEG_KERNEL, &pn);
            pn_allocated = 1;
            dux_used_pn++;
        }

	if (error) {		/*should never happen*/
		dm_quick_reply(error);
		goto out;
	}
	if(lkp -> lk_flags & LK_MNT) {
		struct remvno   *remdvp;
		struct vnode	*vip;
		struct vnode	*stprmv();

		remdvp = &(lkp->lk_dirvp);
		if ((vip = stprmv(remdvp, NULL)) == NULL) {
		   error = u.u_error;
		   send_ref_update_no_remvno(remdvp);
		   VN_RELE(vp);
		}
		else {
		   /* Update the reference double count */
		   update_duxref(vip,1,1);
	   	   dirvp = vip;
		   if (lkp->lk_flags & LK_COMP) {
			compvp = vp;
		   }
		   else {
			/* Have to do it here because no one else below
			 * uses it.  cwb
			 */
			VN_RELE(vp);
		   }
		   error = 0;
		 }
	}
	else {
		error = locallookuppn(&pn,
			lkp->lk_flags&LK_FOLLOWLINK?FOLLOW_LINK:NO_FOLLOW,
			(lkp->lk_flags&LK_DIR) ? &dirvp : (struct vnode **)0,
		        (lkp->lk_flags&LK_COMP) ? &compvp : (struct vnode **)0,
			&vp, &lkp->lk_nlink);
	}
	if (error == 0)	/*found it*/
	{
		(*(lookup_ops[lkp->lk_opcode].lk_serve))
			(request,dirvp,compvp,lkp->lk_opcode,&pn);
		/*the service routine will send back the reply*/
	}
	else if (error == EPATHREMOTE)	/*send it back for more work*/
	{
		pn_skipslash(&pn);
		reply = dm_alloc(sizeof(struct dux_lookup_reply), WAIT);
		rp = DM_CONVERT(reply, struct dux_lookup_reply);
		if (vp->v_fstype == VDUX || vp->v_fstype == VDUX_PV ||
			vp->v_fstype == VUFS)
		{
			ip = (VTOI(vp));
			rp->lkr_vnodetype = MOUNT_UFS;
			rp->lkr_dev = ip->i_dev;
			rp->lkr_ino = ip->i_number;
			rp->lkr_nlink = lkp->lk_nlink;	/*may have been changed by locallookup*/
			if((pn.pn_pathlen == 0) && dirvp) {
				update_vnode_refcount(dirvp, 
						 devsite(ip->i_dev), 1);
				pkrmv (dirvp, &rp->lkr_dirvp);
				rp->lkr_mntdvp_flg = 1;
			}
			else {
				rp->lkr_mntdvp_flg = 0;
			}
		}
		if (vp->v_fstype == VDUX_CDFS || vp->v_fstype == VDUX_CDFS_PV ||
			vp->v_fstype == VCDFS)
		{
			cdp = (VTOCD(vp));
			rp->lkr_vnodetype = MOUNT_UFS;
			rp->lkr_dev = cdp->cd_dev;
			rp->lkr_cdno = cdp->cd_num;
			rp->lkr_nlink = lkp->lk_nlink;	/*may have been changed by locallookup*/
			if((pn.pn_pathlen == 0) && dirvp) {
				update_vnode_refcount(dirvp, 
						 devsite(cdp->cd_dev), 1);
				pkrmv (dirvp, &rp->lkr_dirvp);
				rp->lkr_mntdvp_flg = 1;
			}
			else {
				rp->lkr_mntdvp_flg = 0;
			}
		}
		else if (vp->v_fstype == VNFS)
		{
			rp->lkr_vnodetype = MOUNT_NFS;
			rp->lkr_mntno =
				((struct mntinfo *)(vp->v_vfsp->vfs_data))->
					mi_mntno;
			rp->lkr_fhandle = *(vtofh(vp)); 
			rp->lkr_nlink = lkp->lk_nlink;
		}
		/*copy the path back to the buffer*/
		bcopy(pn.pn_path, bp->b_un.b_addr, pn.pn_pathlen+1);
		dm_reply(reply,DM_REPLY_BUF|DM_KEEP_REQUEST_BUF, EPATHREMOTE,
			bp, pn.pn_pathlen+1, 0);
		VN_RELE(vp);
	}
	else
		dm_quick_reply(error);
	/* Release the path, if we allocated a pn.  We probably used the buf */
	if (pn_allocated) {
		pn_free(&pn);
	}
out:
	restore_ulookup();
}
 
/*
 * pkrmi - pack a remote inode into a message
 */
pkrmi (ip, rip)
register struct inode *ip;
register struct remino *rip;
{
	rip->site	= my_site;
#ifdef hp9000s800
	/* we pass around lu dev numbers, not mi devs */
	rip->dev	= ip->i_mount->m_rdev;
#else
	rip->dev	= ip->i_dev;
#endif
	rip->ino	= ip->i_number;
#ifdef ACLS
	rip->mode	= get_imode(ip);	/* used by exec */
#else
	rip->mode	= ip->i_mode;
#endif
	rip->nlink	= ip->i_nlink;
	rip->uid	= ip->i_uid;
	rip->gid	= ip->i_gid;
	rip->size	= ip->i_size;
	rip->fversion	= ip->i_fversion;
	rip->rdev	= ip->i_device;
	rip->rsite	= ip->i_rsite;
	/* NOTE:  no longer sending fifo fields */
	rip->blocks	= ip->i_blocks;
	rip->vflag	= ip->i_vnode.v_flag & VROOT;
#ifdef QUOTA
        if (ip->i_dquot == NULL) 
           rip->is_dquot = NO_DQUOT;
        else 
           rip->is_dquot = IS_DQUOT;
#endif QUOTA
}

/*
 * stprmi - set up a remote inode from a message
 * mp is the mount entry.  Normally it will be null, in which case it will
 * be looked up.  However, when this is called while setting up the root
 * vnode, the mount entry is in transit and won't be found, so it is passed
 * in.
 */
struct inode *
stprmi(rip,mp)
	register struct remino *rip;
	register struct mount *mp;
{
	dev_t newdev;
	register struct inode *ip;
	struct inode *ieget();
	register int type;
	struct vnode *vp;
	extern struct mount *getrmount();
#ifdef	LOCAL_DISC
	int i;
#endif	LOCAL_DISC

	if (mp == NULL)
		mp = getrmount(rip->dev, rip->site);
	newdev = mp->m_dev;
	ip = ieget(newdev, mp, rip->ino);
	if (ip == NULL) 
		return(NULL);

	vp = ITOV(ip);
	type = rip->mode&IFMT;
	/*always copy certain fields in.  These fields can be trusted
	 *immediately after doing a stprmi*/
	ip->i_mode = rip->mode;
	ip->i_nlink = rip->nlink;
	ip->i_uid = rip->uid;
	ip->i_gid = rip->gid;
	ip->i_blocks = rip->blocks;

	/* if newly allocated or old version... */
	if ((vp->v_count == 0) || (ip->i_fversion != rip->fversion))
	{
		/* Note that NFS guarantees that there will be no
		 * buffers in core if the inode wasn't in core.  If,
		 * however, the inode was in core, but the version
		 * changed, mark the pages as invalid.
		 */
		if (vp->v_count != 0) /*if inode was in core*/
				      /* and we have not already invalidated the */
				      /* buffers */
			ip->i_flag &= ~(IBUFVALID|IPAGEVALID);
		ip->i_mount = mp;
		ip->i_size = rip->size;
		ip->i_fversion = rip->fversion;
		ip->i_flag &= ~ISYNC;
		if (vp->v_count == 0)
		{
			VN_INIT(vp, mp->m_vfsp, IFTOVT(ip->i_mode), NODEV);
			/*note, v_rdev is set to 0, but if the device is
			 *actually opened, it will be reset in ufs_open.
			 */
#ifdef	LOCAL_DISC
			/* zero out direct & indirect blocks which */
			/* still may be around from inode on locally */
			/* mounted disc.  if this is not done, then */
			/* fifos will have problem when using i_ib */
			/* fields. */
			for (i = 0; i < NDADDR; i++)
				ip->i_db[i] = 0;
			for (i = 0; i < NIADDR; i++)
				ip->i_ib[i] = 0;
#endif	LOCAL_DISC
#ifdef QUOTA
                        if (rip->is_dquot == IS_DQUOT)
                        {
                           ip->i_dquot = getinoquota(ip);
                        }
                        else
                           ip->i_dquot = NULL;
#endif QUOTA
		}
		else
		{
			vp->v_vfsp = mp->m_vfsp;
			vp->v_type = IFTOVT(ip->i_mode);
			vp->v_rdev = NODEV;
#ifdef QUOTA
                        /* inode is getting updated.  If owner or mp has  *
                         * changed, release old quota cache entry and get *
                         * a new one (if needed). If no i_dquot, get one. */
                        if (rip->is_dquot == IS_DQUOT)
			{
                           if (ip->i_dquot)
			   {
                              if ((ip->i_dquot->dq_uid != ip->i_uid) ||
                                  (ip->i_dquot->dq_mp  != mp))
   			      {
                                 dqrele(ip->i_dquot);
                                 ip->i_dquot = NULL;	/* see quota_release */
                                 ip->i_dquot = getinoquota(ip);
			      }
			   }
                           else
                              ip->i_dquot = getinoquota(ip);
                        }
                        else /* No quotas needed here */
			{
                           if (ip->i_dquot) /* preveously had quotas */
                           {
                              dqrele(ip->i_dquot);
                              ip->i_dquot = NULL;
			   }
                        }
#endif QUOTA
		}

		if (type == IFBLK || type == IFCHR) {
			ip->i_device = rip->rdev;
			ip->i_rsite = rip->rsite;
		}

		vp->v_op = &dux_vnodeops;
		vp->v_fstype = VDUX;
	}
	vp->v_flag &= ~VROOT;
	vp->v_flag |= rip->vflag;
	iunlock(ip);
	return(ip);
}

/*
 *Check if a path is remote.
 */
int
pathisremote(vp)
struct vnode *vp;
{

	return (vp->v_fstype == VDUX || vp->v_fstype == VDUX_PV ||
		vp->v_fstype == VDUX_CDFS || vp->v_fstype == VDUX_CDFS_PV ||
		(vp->v_fstype == VNFS && u.u_nsp));
}





extern int pkrmcd(), send_ref_update_cd();
extern struct cdnode *stprmcd();


pkrmv(vp, rmv)
register struct	vnode	*vp;
register struct	remvno *rmv;
{
	switch(vp->v_fstype) {
	case VCDFS:
		rmv->rv_fstype=MOUNT_CDFS;
		CDFSCALL(PKRMCD)(VTOCD(vp), &(rmv->rv_cdno));
		break;
	case VUFS:
		rmv->rv_fstype=MOUNT_UFS;
		pkrmi (VTOI(vp), &(rmv->rv_ino));
		break;
	default:
		panic("pkrmv: illegal fs type");
	}
}

struct	vnode *
stprmv(rvp,mp)
	register struct remvno *rvp;
	register struct mount *mp;
{
	struct cdnode *cdp;
	struct inode *ip;

	switch(rvp->rv_fstype) {
	case MOUNT_CDFS:
		if((cdp = (struct cdnode *)
			CDFSCALL(STPRMCD)(&(rvp->rv_cdno), mp)) == NULL) {
			 return(NULL);
		}
		return(CDTOV(cdp));
	case MOUNT_UFS:
		if ((ip=stprmi (&(rvp->rv_ino), mp)) == NULL) return(NULL);
		return(ITOV(ip));
	default:
		panic("stprmv: illegal fs type");
	}
	/*NOTREACHED*/
}

/*
 *send a request to decrement the reference count on an vnode, releasing 
 *it if necessary
 */
send_ref_update_v (vp, subtract)
register struct vnode *vp;
int subtract;
{
	switch(vp->v_fstype) {
	case VDUX:
		send_ref_update(VTOI(vp), subtract);
		break;
	case VDUX_CDFS:
		CDFSCALL(SEND_REF_UPDATE_CD)(VTOCD(vp), subtract);
		break;
	default:
		panic("send_ref_update_v: illegal fs type");
	}
}


/*
 *send a request to decrement the reference count on a server's inode,
 * when was unable to get an inode on the client side.
 */
send_ref_update_no_vno(dev, rmvp)
dev_t dev;
struct remvno *rmvp;
{
	switch(rmvp->rv_fstype) {
	case MOUNT_CDFS:
		CDFSCALL(SEND_REF_UPDATE_NO_CDNO)(dev, rmvp->rv_cdno.cdno);
		break;
	case MOUNT_UFS:
		send_ref_update_no_ino(dev, rmvp->rv_ino.ino);
		break;
	default:
		panic("send_ref_update_no_vno: illegal fs type");
	}
}

send_ref_update_no_remvno(rpv)
	struct remvno *rpv;
{
	register struct mount *mp;
	extern struct mount *getrmount();

	if (rpv->rv_fstype == MOUNT_UFS) {
		mp = getrmount(rpv->rv_ino.dev, rpv->rv_ino.site);
	}
	else {
		mp = getrmount(rpv->rv_cdno.dev, rpv->rv_cdno.site);
	}
	send_ref_update_no_vno(mp->m_dev, rpv);
}

update_vnode_refcount(vp, site, count)
	struct vnode *vp;
	site_t site;
	int    count;
{
	if (vp-> v_fstype == VUFS)
		updatesitecount (&((VTOI(vp))->i_refsites), site, count);
	else
		updatesitecount (&((VTOCD(vp))->cd_refsites), site, count);
}
/*
 * Gets symbolic link into pathname.  Lifted directly from vfs_lookup.c.
 * This way I only have to patch one file.
 */
static int
getsymlink(vp, pnp)
	struct vnode *vp;
	struct pathname *pnp;
{
	struct iovec aiov;
	struct uio auio;
	register int error;

	pn_alloc(pnp);
	aiov.iov_base = pnp->pn_buf;
	aiov.iov_len = MAXPATHLEN;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_seg = UIOSEG_KERNEL;
	auio.uio_resid = MAXPATHLEN;
	error = VOP_READLINK(vp, &auio, u.u_cred);
	if (error)
		pn_free(pnp);
	pnp->pn_pathlen = MAXPATHLEN - auio.uio_resid;
	return (error);
}
