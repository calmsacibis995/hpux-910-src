/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_pseudo.c,v $
 * $Revision: 1.5.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:42:04 $
 */

/* HPUX_ID: @(#)dux_pseudo.c	55.1		88/12/23 */

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
 *This file contains code to provide special pseudo dux vnodes.  These vnodes
 *are created when it is recognized in the lookup process that a vnode is
 *remote, but we don't have the real vnode.  The vnode is filled in with the
 *information necessary to send the remote request, including the dev and
 *inode number.  However, it is not put on the hash list, and is expected
 *to be released quickly.  The only vnode operations supported for this
 *type of vnode are inactive and pathsend.
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../h/systm.h"
#include "../ufs/inode.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../cdfs/cdfs_hooks.h"

extern pseudo_inval();
extern pseudo_inactive();
extern dux_pathsend();

struct vnodeops dux_pvnodeops =
{
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inactive,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	pseudo_inval,
	dux_pathsend,
#ifdef ACLS
	pseudo_inval,		/* setacl() */
	pseudo_inval,		/* getacl() */
#endif ACLS
#ifdef POSIX
	pseudo_inval,		/* pathconf() */
	pseudo_inval,		/* fpathconf() */
#endif POSIX
	pseudo_inval,		/* lockctl() */
	pseudo_inval,		/* lockf() */
	pseudo_inval,		/* fid() */
};

pseudo_inval()
{
	panic ("Unimplemented pseudo vnode operation");
}

/*ARGSUSED*/
int
pseudo_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct inode *ip;
	extern struct inode *ifreeh, **ifreet;

	if (vp->v_fstype == VDUX_PV) {
	/*set the i number to 0*/
	ip = VTOI(vp);
	ip->i_number = 0;
	ip->i_flag = IBUFVALID|IPAGEVALID;
	/*put the inode on the free list.*/
	if (ifreeh) {
		*ifreet = ip;
		ip->i_freeb = ifreet;
	} else {
		ifreeh = ip;
		ip->i_freeb = &ifreeh;
	}
	ip->i_freef = NULL;
	ifreet = &ip->i_freef;
	return (0);
	}
	else if (vp->v_fstype == VDUX_CDFS_PV) {
		return (CDFSCALL(DUXCD_PSEUDO_INACTIVE)(vp));
	}
	/*NOTREACHED*/
}

/*
 *Make a pseudo vnode for a dux file system
 */
int
stppsv(vpp, vfsp, dev, ino)
	struct vnode **vpp;
	struct vfs *vfsp;
	dev_t dev;
	ino_t ino;
{
	register struct inode *ip;
	register struct vnode *vp;
	extern struct inode *eiget();

	/* eiget can return -1 if it may have gone to sleep.  This allows
	 * iget to research for the inode.  Here we just want any inode, so
	 * if it returns -1, we just recall it.
	 */
	do
		ip = eiget(NULL);
	while (ip == (struct inode *)-1);
	if (ip == NULL)
		return (u.u_error);
	ip->i_dev = dev;
	ip->i_number = ino;
	vp = ITOV(ip);
	VN_INIT(vp, vfsp, VDIR, NODEV);
	vp->v_op = &dux_pvnodeops;
	vp->v_fstype = VDUX_PV;
	*vpp = vp;
	return (0);
}
/*
 *Make a pseudo vnode for a dux CDFS file system
 */
int
stppsv_cdfs(vpp, vfsp, dev, cdno)
	struct vnode **vpp;
	struct vfs *vfsp;
	dev_t dev;
	cdno_t cdno;
{
	register struct cdnode *cdp;
	register struct vnode *vp;

	cdp = (struct cdnode *)(CDFSCALL(ECDGET)(NULL));
	if (cdp == NULL)
		return (u.u_error);
	cdp->cd_dev = dev;
	cdp->cd_num = cdno;
	vp = CDTOV(cdp);
	VN_INIT(vp, vfsp, VDIR, NODEV);
	vp->v_op = &dux_pvnodeops;
	vp->v_fstype = VDUX_CDFS_PV;
	*vpp = vp;
	return (0);
}

/*
 *Make a pseudo root vnode for a dux file system
 */
int
dux_pseudo_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct mount *mp;
	int error;

	/*
	 *If this is the root file system, return the real root.  Otherwise,
	 *return a pseudo root
	 */
	if (vfsp == rootvfs)
	{
		*vpp = rootdir;
		VN_HOLD(*vpp);
	}
	else
	{
		mp = (struct mount *)vfsp->vfs_data;
		error = stppsv(vpp, vfsp, mp->m_dev, ROOTINO);
		if (error)
			return (error);
	}
	(*vpp)->v_flag |= VROOT;
	return (0);
}
