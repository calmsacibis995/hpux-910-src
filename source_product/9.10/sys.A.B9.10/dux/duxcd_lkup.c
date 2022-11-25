/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/duxcd_lkup.c,v $
 * $Revision: 1.4.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:43:06 $
 */

/* HPUX_ID: @(#)duxcd_lkup.c	54.6		88/12/12 */

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
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_lookup.h"
#include "../dux/lookupmsgs.h"
#include "../h/mount.h"
#include "../h/buf.h"
#include "../dux/dux_dev.h"
extern site_t	my_site;
extern struct vnodeops duxcd_vnodeops;
extern struct inode *stprmi();
#define ISOFS	0x2

/*
 * pkrmcd - pack a remote cdnode into a message
 */
pkrmcd (cdp, rcdp)
register struct cdnode *cdp;
register struct remcdno *rcdp;
{
	rcdp->site	= my_site;
#ifdef hp9000s800
	/* we pass around lu dev's between cluster sites */
	rcdp->dev	= cdp->cd_mount->m_rdev;
#else
	rcdp->dev	= cdp->cd_dev;
#endif
	rcdp->cdno	= cdp->cd_num;
	rcdp->cdpno	= cdp->cd_pnum;
	rcdp->mode	= cdp->cd_mode;
	rcdp->uid	= cdp->cd_uid;
	rcdp->gid	= cdp->cd_gid;
	rcdp->size	= cdp->cd_size;
	rcdp->vflag	= cdp->cd_vnode.v_flag & VROOT;
	if(cdp->cd_format == ISO9660FS) rcdp->vflag |= ISOFS;
	rcdp->loc	= cdp->cd_loc;
	rcdp->fusize	= cdp->cd_fusize;
	rcdp->fugsize	= cdp->cd_fugsize;
	rcdp->xarlen	= cdp->cd_xarlen;
	rcdp->ftype	= cdp->cd_ftype;
}

/*
 * stprmcd - set up a remote cdnode from a message
 * mp is the mount entry.  Normally it will be null, in which case it will
 * be looked up.  However, when this is called while setting up the root
 * vnode, the mount entry is in transit and won't be found, so it is passed
 * in.
 */
struct cdnode *
stprmcd(rcdp,mp)
	register struct remcdno *rcdp;
	register struct mount *mp;
{
	dev_t newdev;
	register struct cdnode *cdp;
	struct cdnode *cdeget();
	struct vnode *vp;
	extern struct mount *getrmount();

	if (mp == NULL)
		mp = getrmount(rcdp->dev, rcdp->site);
	newdev = mp->m_dev;
	cdp = cdeget(newdev, mp->m_dfs, rcdp->cdno);
	if (cdp == NULL) 
		return(NULL);

	vp = CDTOV(cdp);

	if (vp->v_count == 0)
	{
		cdp->cd_mode = rcdp->mode;
		cdp->cd_uid = rcdp->uid;
		cdp->cd_pnum = rcdp->cdpno;
		cdp->cd_gid = rcdp->gid;
		cdp->cd_loc = rcdp->loc;
		cdp->cd_fusize = rcdp->fusize;
		cdp->cd_fugsize = rcdp->fugsize;
		cdp->cd_xarlen = rcdp->xarlen;
		cdp->cd_mount = mp;
		cdp->cd_size = rcdp->size;
		VN_INIT(vp, mp->m_vfsp, CDFTOVT(rcdp->ftype), NODEV);
		vp->v_op = &duxcd_vnodeops;
		vp->v_fstype = VDUX_CDFS;
		if (rcdp->vflag & ISOFS) cdp->cd_format = ISO9660FS;
		else cdp->cd_format = HSGFS;
	}
	vp->v_flag &= ~VROOT;
	vp->v_flag |= (rcdp->vflag & VROOT);
	cdunlock(cdp);
	return(cdp);
}


/*
 *send a request to decrement the reference count on an cdnode, releasing it
 *if necessary
 */
send_ref_update_cd (cdp, subtract)
register struct cdnode *cdp;
int subtract;
{
	dm_message request;
	register struct iref_update *rp;

	request = dm_alloc(sizeof (struct iref_update), WAIT);
	rp = DM_CONVERT(request, struct iref_update);
	rp->fstype = MOUNT_CDFS;
	rp->dev = cdp->cd_dev;
	rp->un.cdno = cdp->cd_num;
	rp->subtract = subtract;
	dm_send(request, DM_RELEASE_REQUEST|DM_RELEASE_REPLY, DM_REF_UPDATE,
		devsite(cdp->cd_dev), DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);
}

/*
 *send a request to decrement the reference count on a server's cdnode,
 * when was unable to get an inode on the client side.
 */
send_ref_update_no_cdno(dev, cdnum)
dev_t dev;
cdno_t cdnum;
{
	dm_message request;
	register struct iref_update *rp;

	request = dm_alloc(sizeof (struct iref_update), WAIT);
	rp = DM_CONVERT(request, struct iref_update);
	rp->fstype=MOUNT_CDFS;
	rp->dev = dev;
	rp->un.cdno = cdnum;
	rp->subtract = 1;
	dm_send(request, DM_RELEASE_REQUEST|DM_RELEASE_REPLY, DM_REF_UPDATE,
		devsite(dev), DM_EMPTY, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL);
}


