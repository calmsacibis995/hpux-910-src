/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_exec.c,v $
 * $Revision: 1.7.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:40:40 $
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


/*Code dealing with execution in the DUX environment*/

#include "../h/param.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/conf.h"
#include "../ufs/inode.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/sitemap.h"
#include "../dux/dux_dev.h"
#include "../dux/dux_lookup.h"
#include "../h/mount.h"
#include "../dux/lookupmsgs.h"
#include "../dux/cct.h"
#include "../h/buf.h"
#include "../h/types.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs_hooks.h"

#include "../nfs/nfs_clnt.h"
#include "../h/kern_sem.h"

/*
 *A message for updating the text reference
 */
struct text_change_message	/*DUX MESSAGE STRUCTURE*/
{
	int	fstype;		/*MOUNT_CDFS, MOUNT_UFS*/
	int 	change;
	dev_t	dev;
	union {
		ino_t	un_ino;
		cdno_t	un_cdno;
	} un;
};
#define tc_ino un.un_ino
#define tc_cdno un.un_cdno


 
/*
 * increment or decriment the text reference at the server
 * vp->v_fstype has to be either VDUX or VDUX_CDFS
 */
int
update_text_change(vp,change,serverinitiated)
register struct vnode *vp;
int change;
int serverinitiated;
{
	register struct text_change_message *rp;
	register struct buf *bp;
	register struct bufhd *hp;
	int s;
	site_t site;
	sv_sema_t ss;
	int retval = 0;
	struct dcount *execdcount;
	int subtract;
	dm_message request;

#define dp ((struct buf *)hp)

	register enum vfstype	fstype;

	fstype = vp->v_fstype;
	/* update the dcount on our side */

	PXSEMA(&filesys_sema, &ss);
	if (fstype == VDUX)
		execdcount = &(VTOI(vp)->i_execdcount);
	else
		execdcount = &(VTOCD(vp)->cd_execdcount);

	/*
	 * check to see if we need to update the server.  This would
	 * only be done if our dcount transitioned from 0->1 or 1->0.
	 */
	subtract = updatedcount(execdcount, change, serverinitiated);
	if (subtract) {
		int flags;

		/*
		 * If we are to update the server, then we must first
		 * insure that all pending associated buffer I/O,
		 * readaheads, are complete.  This is for the case where
		 * the text change message gets through before the pending
		 * readahead does thus panicing the server upon the read
		 * of a free inode.
		 */
loop:
		s = spl6();
		for (hp = bufhash; hp < &bufhash[BUFHSZ]; hp++) {
			for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
				if (bp->b_vp == vp && (bp->b_flags & B_BUSY)) {
					bp->b_wanted = 1; /* MP safe */
					sleep((caddr_t)bp, PRIBIO+1);
					splx(s);
					goto loop;
				}
			}
		}
		splx(s);

		/* package up the message to the server and send it! */

		request = dm_alloc(sizeof(struct text_change_message), WAIT);
		rp = DM_CONVERT(request, struct text_change_message);
		rp->change = -subtract;
		if (fstype == VDUX) {
			rp->fstype = MOUNT_UFS;
		        rp->dev = VTOI(vp)->i_dev;
			rp->tc_ino = VTOI(vp)->i_number;
			site = devsite(VTOI(vp)->i_dev);
		}
		else {
			rp->fstype = MOUNT_CDFS;
			rp->dev = VTOCD(vp)->cd_dev;
			rp->tc_cdno = VTOCD(vp)->cd_num;
			site = devsite(VTOCD(vp)->cd_dev);
		}

		/*
		 * If we are adding to the count as part of a local
		 * request, we wait for the reply from the server to
		 * make sure that it is okay to set this vnode VTEXT.
		 */
		if (subtract < 0 && !serverinitiated && change == 1)
			flags = DM_SLEEP|DM_RELEASE_REQUEST;
		else
			flags = DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY;

		request = dm_send(request, flags, DM_TEXT_CHANGE, site,
		     DM_EMPTY, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		
		if (subtract < 0 && !serverinitiated && change == 1) {
			/*
			 * If the server returned an error, we must
			 * backout the local change to the count.  We
			 * will not have to go back to the server, as
			 * we are decrementing both counts, and you
			 * never need to update the server counts in
			 * such a situation.
			 */
			if (DM_RETURN_CODE(request) != 0) {
				updatedcount(execdcount, -1, 1);
				retval = 1;
			}
			dm_release(request, 1);
	        }
	}
	VXSEMA(&filesys_sema, &ss);
	return retval;
}

serve_text_change(request)
dm_message request;
{
	register struct text_change_message *rp;
	register struct inode *ip;
	register struct cdnode *cdp;
	int retval = 0;

	rp = DM_CONVERT(request, struct text_change_message);
	if (rp->fstype == MOUNT_UFS) {
		ip = ifind(localdev(rp->dev), rp->tc_ino);
		if (ip != NULL)	/*should never be NULL*/
		{
			if (rp->change > 0) {
				/*
				 * openforwrite() will check if the
				 * vnode is open for write, and return
				 * an error if so.  If not, the text
				 * reference count for the vnode will
				 * be incremented.
				 */
				retval = openforwrite(ITOV(ip), 1);
			}
			else {
				dectext(ITOV(ip), -rp->change);
			}
		}
	}
	else {
		cdp = (struct cdnode *)
			CDFSCALL(CDFIND)(localdev(rp->dev), rp->tc_cdno);
		if (cdp != NULL)	/*should never be NULL*/
		{
			if (rp->change > 0) {
				/*
				 * We do not need to check if the vnode
				 * is open for write, as CDROM is always
				 * read-only.
				 */
				inctext(CDTOV(cdp), 0);
			}
			else {
				dectext(CDTOV(cdp), -rp->change);
			}
		}
	}
	dm_quick_reply(retval);
}

/*
 *Service a lookup request for exec.  This is similar to a pure lookup except:
 *   A check is made for mutual exclusion
 *   We immediately check for type VREG.  (Note:  DUX doesn't try to optimize
 *	for error cases and it would normally be OK to let the error be
 *	detected at the client.  However, the exec sitemap is unioned with
 *	the FIFO read map.  If someone tried to exec a FIFO, all havoc could
 *	result.
 *   The exec sitemap is used instead of the reference sitemap
 *   We set the ITEXT flag.  This is a bit of a kluge.  The itext flag
 *	is not meant for this purpose.  However we wish to prevent concurrent
 *	update to this file while we are loading the executable file, and
 *	we can't keep the inode locked.  The ITEXT flag will suffice.
 */

/*ARGSUSED*/
dux_exec_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	dm_message reply;
	register struct pure_lookup_reply *rp;
	int error;
	register struct inode *ip;
	register enum vfstype	fstype;

	fstype = compvp->v_fstype;

	ip = VTOI(compvp);
	
	/* We first  make a couple of checks to insure  that the file is
	   executable  at this  moment.  If for some  reason  it is not,
	   then we set the appropriate error and return.*/

	/* first check that it is a regular file */
	if (compvp->v_type != VREG)
	{
		error = EACCES;
		goto bad;
	}

	/* second  check to see if someone has it open for  writing.  If
	   so we can't exec it at this time.  */
	if (fstype == VUFS) {
		if (gettotalsites(&ip->i_writesites) != 0)
		{
			error = ETXTBSY;
			goto bad;
		}
	}

	/* 
	 * call inctext to mark the reference in the sitemap,
	 * and set the vnode to indicate the text is being executed.
	 */
	inctext(compvp, 0);

	/* We can now release the vnode.  Text_ref will have done an
	 * additional hold on the vnode if there were not already
	 * references.
	 */
	VN_RELE(compvp);
	
	/* send back the inode to the requestor */
	reply = dm_alloc(sizeof (struct pure_lookup_reply), WAIT);
	rp = DM_CONVERT (reply, struct pure_lookup_reply);
	pkrmv (compvp, &rp->pkv);
	dm_reply(reply, 0, 0, NULL, NULL, NULL);
	if(fstype == VUFS)
	imark(ip, IACC);
	return;
bad:
	VN_RELE(compvp);
	dm_quick_reply(error);
}



/* 
 * free the swap image of all unused sticky text
 * which are from device dev
 */
struct xumountreq {		/*DUX MESSAGE STRUCTURE*/
	union {
		dev_t idev;
		int mnt_num;
	} xum_data;
	int mount_type;
};

dux_xumount(vfsp)
register struct vfs *vfsp;
{
	register dm_message request, response;
	register struct xumountreq *sreqp;

	if (!(my_site_status & CCT_CLUSTERED)) 
		return;
	request = dm_alloc(sizeof(struct xumountreq), WAIT);
	sreqp = DM_CONVERT(request, struct xumountreq);
	sreqp->mount_type = vfsp->vfs_mtype;
	switch (vfsp->vfs_mtype) {

	case MOUNT_UFS: {
		dev_t dev;

		dev = ((struct mount *)(vfsp->vfs_data))->m_dev;
		sreqp->xum_data.idev = devindex(dev, IFBLK);
		break;
		}

	case MOUNT_NFS:
		sreqp->xum_data.mnt_num = 
				((struct mntinfo *)vfsp->vfs_data)->mi_mntno;
		break;

	default:
		panic("xumount: unknown mount type");
	}

	response = dm_send( request, DM_SLEEP | DM_RELEASE_REQUEST,
			DM_XUMOUNT, DM_CLUSTERCAST, DM_EMPTY, NULL, NULL, 0, 0,
			NULL, NULL, NULL);
	u.u_error = u.u_error ? u.u_error : DM_RETURN_CODE(response);
	dm_release(response, 1);
}

xumount_recv(reqp)
dm_message reqp;
{
	register struct xumountreq *sreqp;
	struct mount *mp;

	sreqp = DM_CONVERT(reqp, struct xumountreq);
	switch (sreqp->mount_type) {

	case MOUNT_UFS:
		mp = getmount (mkbrmtdev(u.u_site, sreqp->xum_data.idev));
		if (mp != NULL)
			xumount1(mp->m_vfsp);
		break;
	case MOUNT_NFS: {
		struct vfs *vfsp;

		vfsp = (struct vfs *)NFSCALL(NFS_FIND_MNT)(sreqp->xum_data.mnt_num);
		if (vfsp != NULL)
			xumount1(vfsp);
		break;
		}
	default:
		panic("xumount_recv: illegal fs type");
	}
	dm_quick_reply(u.u_error);
}

struct xrelereq {	/*DUX MESSAGE STRUCTURE*/
	int	fstype;		/*MOUNT_UFS, MOUNT_CDFS*/
	dev_t 	idev;
	union {
		ino_t	un_ino;
		cdno_t	un_cdno;
	} un;
	u_short mode;
};
#define	xr_ino un.un_ino
#define	xr_cdno un.un_cdno

xrele_send(vp)
register struct vnode *vp;
{
	register struct inode *ip = (VTOI(vp));
	register struct sitemap *mapp;
	register struct xrelereq *sreqp;
	dm_message request;
	site_t dest;
	struct buf *bp;
	sv_sema_t ss;

	struct cdnode	*cdp = (VTOCD(vp));

	PXSEMA(&filesys_sema, &ss);
	if (vp->v_fstype == VCDFS) {
		mapp = &cdp->cd_execsites;
	}
	else {
		mapp = &ip->i_execsites;
	}
	if (mapp->s_maptype == S_ONESITE) {
		if (mapp->s_onesite.s_site == my_site ||
			mapp->s_onesite.s_site == 0){
				VXSEMA(&filesys_sema, &ss);
				return;
		}
		dest = mapp->s_onesite.s_site;
		bp = NULL;
	} else {
		extern struct buf *createsitelist();

		bp = createsitelist(*mapp, 0);
		if (bp == NULL){
			VXSEMA(&filesys_sema, &ss);
			return;
		}
		dest = DM_MULTISITE;
	}
	request = dm_alloc(sizeof(struct xrelereq), WAIT);
	sreqp = DM_CONVERT(request, struct xrelereq);
	if (vp->v_fstype == VCDFS) {
		sreqp->fstype = MOUNT_CDFS;
		sreqp->mode = cdp->cd_mode;
		sreqp->xr_cdno = cdp->cd_num;
		sreqp->idev = devindex(cdp->cd_dev, IFBLK);	
	}
	else {
		sreqp->fstype = MOUNT_UFS;
		sreqp->mode = ip->i_mode;
		sreqp->xr_ino = ip->i_number;
		sreqp->idev = devindex(ip->i_dev, IFBLK);	
	}
	dm_send(request,
		DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
		DM_XRELE, dest, DM_EMPTY, NULL, bp, 0, 0, NULL, NULL, NULL);
	VXSEMA(&filesys_sema, &ss);
}		

xrele_recv(reqp)
dm_message reqp;
{
	register struct xrelereq *sp;
	register struct inode *ip;
	register struct cdnode *cdp;
	register struct vnode *vp;

	sp =  DM_CONVERT(reqp, struct xrelereq);
	if (sp->fstype == MOUNT_CDFS) {
		cdp = (struct cdnode *) CDFSCALL(CDFIND)
			(mkbrmtdev(u.u_site,sp->idev), sp->xr_cdno);
		if (cdp != NULL) {
			cdp->cd_mode = sp->mode;		/*???*/
			if (CDTOV(cdp)->v_count)
				xrele1(CDTOV(cdp));
		} 
	}
	else {
		ip = ifind(mkbrmtdev(u.u_site,sp->idev), sp->xr_ino);
		if (ip != NULL) {
			ip->i_mode = sp->mode;		/*???*/
			vp = ITOV(ip);
			if (vp->v_count)
				xrele1(vp);
		} 
	}
	dm_quick_reply(0);
}

/*
 * Unpack an exec request.  This is the same as dux_pure_lookup_unpack
 * except that the exec dcount is used instead of the ref dcount
 */
/*ARGSUSED*/
int
dux_exec_unpack(request, reply, dirvpp, compvpp)
dm_message request,reply;
struct vnode **dirvpp, **compvpp;
{
	register struct pure_lookup_reply *rp =
		DM_CONVERT(reply, struct pure_lookup_reply);
	register struct text_change_message *erp;
	register struct mount *mp;
	extern struct mount *getrmount();
        dm_message reqst;

	struct vnode *vp;
	extern struct vnode *stprmv();

	if ((vp = stprmv(&rp->pkv,NULL)) == NULL) {
		 /* our inode table is full and we couldn't get an inode
		    so we must clean up the server and return an error. */

		reqst = dm_alloc(sizeof(struct text_change_message), WAIT);
		erp = DM_CONVERT(reqst, struct text_change_message);
		erp->change = -1;
		switch (rp->pkv.rv_fstype) {
		case MOUNT_UFS:
			mp = getrmount(rp->pki.dev,rp->pki.site);
			erp->dev = mp->m_dev;
			erp->tc_ino = rp->pki.ino;
			dm_send(reqst, DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
				DM_TEXT_CHANGE, rp->pki.site, DM_EMPTY,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			break;
		case MOUNT_CDFS:
			mp = getrmount(rp->pkcd.dev,rp->pkcd.site);
			erp->dev = mp->m_dev;
			erp->tc_cdno = rp->pkcd.cdno;
			dm_send(reqst, DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
				DM_TEXT_CHANGE, rp->pkcd.site, DM_EMPTY,
				NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			break;
		default:
			panic("dux_exec_unpack: illegal fstype");
		}
		return (u.u_error);
	}
		
	/* Update the reference double count */
	*compvpp = vp;

	return (0);
}

