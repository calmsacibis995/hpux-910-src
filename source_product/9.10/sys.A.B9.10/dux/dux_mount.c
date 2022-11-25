/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/dux_mount.c,v $
 * $Revision: 1.13.83.5 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/11/11 11:57:52 $
 */

/* HPUX_ID: @(#)dux_mount.c	55.1		88/12/23 */

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

#ifndef _SYS_STDSYMS_INCLUDED
#    include "../h/stdsyms.h"
#endif   /* _SYS_STDSYMS_INCLUDED  */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/dux_lookup.h"
#include "../h/mount.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../dux/lookupops.h"
#include "../dux/lookupmsgs.h"
#include "../dux/dux_dev.h"
#include "../dux/cct.h"
#include "../dux/duxfs.h"
#include "../h/malloc.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"
#include "../cdfs/cdfs_hooks.h"

#define INV_K_ADR NULL

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

/* Used only on rootsite */
int mountlock = 0;		/* state of the mount lock */
int mount_waiting;		/* true if someone sleeping */
site_t mount_site;		/* site holding mount lock */
int mountlock_prevent;		/* no M_LOCK, but allow M_RECOVERY_LOCK */
struct proc *mount_procp;	/* process holding mount lock */
pid_t mount_pid;		/* process holding mount lock */

/* Externals */
extern site_t root_site;
#ifdef GETMOUNT
extern time_t mount_table_change_time;
extern volatile struct timeval time;
#endif GETMOUNT

/* Structure representing a packed vnode */
struct packedvnode		/*DUX MESSAGE STRUCTURE*/
{
	int vnodetype;			/*MOUNT_UFS, etc...*/
	union
	{
		struct	/*structure for UFS inode*/
		{
			dev_t	index;	/*device index*/
			ino_t	ino;	/*inode number*/
			site_t	site;	/*site of inode*/
			short	pad;
		} inodepacked;
		struct  /* structure for NFS rnode */
		{
			fhandle_t fhandle;	/*file handle*/
			struct nfsfattr fattr;	/*file attributes*/
			int	mntno;		/*mi_mntno field*/
		} rnodepacked;
		struct	/*structure for CDFS cdnode*/
		{
			dev_t	index;	/*device index*/
			cdno_t	cdno;	/*cdnode number*/
			site_t	site;	/*site of cdnode*/
			short	pad;
		} cdnodepacked;
	}un;
};

/* Structure for passing VFSs around */
struct vfsmsg			/*DUX MESSAGE STRUCTURE*/
{
	struct vfsid vfsid;
	int flag;
	int bsize;
	u_short exroot;
	short exflags;
	int isroot;		/*true for root vfs*/
#ifdef GETMOUNT
	time_t	mnttime;
	char fsname[MAXPATHLEN]; /* fs string identifier */
#endif GETMOUNT
	union
	{
		struct packedvnode coveringvnode;
		struct remino rootinode;
	}vnodeinfo;
	union 
	{
		struct 
		{
			dev_t rdev;		/*real device #*/
			struct duxfs dfs;	/*fs info for DUX*/
#ifdef QUOTA
                        u_short m_qflags;       /*are quotas enabled*/
                        short   spare;          /*pad to 4 bytes*/
#endif QUOTA
		}isufs;
		struct
		{
			struct sockaddr_in addr;	/*fields from mntinfo*/
			u_int hard;
			u_int mint;
			u_int devs;			/* do devices or not */
			long tsize;
			long stsize;
			long bsize;
			int retrans;
			int timeo;   /*Added later */
			struct packedvnode rootvnode;	/*root of vfs*/
			char hostname[HOSTNAMESZ];
#ifdef GETMOUNT
			char fsmnt[MAXMNTLEN];
#endif GETMOUNT
			u_int dynamic : 1;
			u_int ignore  : 1;
			u_int noac    : 1;
			u_int nocto   : 1;
			u_int filler  : 28;  /* Pad to 32 bits */
			u_int            acregmin;
			u_int            acregmax;
			u_int            acdirmin;
			u_int            acdirmax;
		} isnfs;
	}vfsinfo;
};

/*  Lock the clusterwide mount/umount semaphore.  Sleep if the semaphore is
 *  already locked.
 *
 *  On all nodes this is called by the mount and umount system calls when
 *  they begin, and by recovery code when it wishes to clean up local mounts.
 *
 *  On the rootserver only, this code is also called to serve client
 *  DM_LOCK_MOUNT requests and by the cluster server code when a client
 *  joins the cluster.
 *
 *  This can never be called under interrupt on client nodes, and may only
 *  be called by an interrupt when the M_TLOCK option is used.
 *
 *  The options are:
 *
 *	M_LOCK		Lock the semaphore.  If the semaphore is locked
 *			sleep until it is free, then lock it and return 0.
 *
 *	M_TLOCK		Test and lock the semaphore.  If the semaphore is
 *			not locked, lock it and return 0 otherwise return 1.
 *
 * IMPORTANT NOTE:
 *   To fix a deadlock condition, we now assume that M_TLOCK is *only*
 *   called by the diskless recovery code.  No other kernel code may
 *   call lock_mount with M_TLOCK at this time.	 The name M_TLOCK will
 *   be changed to M_RECOVERY_LOCK in the next release (IF2), and M_TLOCK
 *   will be added with a new value for future use.
 *
 *   The deadlock situation occurs when a node is re-joining a cluster.
 *   To re-add the node, all other nodes must cleanup their stuff for
 *   him.  Part of this requires the mount lock.  However, the node that
 *   is joining the cluster also might have the mount lock, which causes
 *   a deadlock.  To solve this, we do the following:
 *
 *	When a node is joining the cluster, rather than locking the mount
 *	table with M_LOCK, serve_clusterreq() indicates its intent to
 *	lock the mount table by performing the following operation:
 *
 *	   lock_mount(M_PREVENT_LOCK);
 *
 *	The M_PREVENT_LOCK prevents any processes from acquiring a lock
 *	with M_LOCK, but allows the cleanup code to acquire the mount
 *	lock with M_TLOCK.  This allows the mount table to be locked
 *	and unlocked for crash recovery, but prevents cnodes from
 *	sending messages to the cnode that we are adding that is in a
 *	transient state and is not yet able to respond to messages.
 *
 *	After serve_clusterreq() completes the DM_ADD_MEMBER operation,
 *	it then, atomically, allows normal lock_mount operations to
 *	continue, and acquires the mount lock by using:
 *
 *	   lock_mount(M_ALLOW_LOCK);
 */

struct lockmount_request {
	int	function;
};

lock_mount(f)
int f;
{
	int s;
	int error;
	dm_message request, reply;
	struct lockmount_request *rp;

	if ((my_site_status & CCT_CLUSTERED) && (my_site != root_site)) {
		request = dm_alloc(sizeof(struct lockmount_request), WAIT);
		rp = DM_CONVERT(request, struct lockmount_request);
		rp->function = f;
		reply = dm_send (request,
			DM_SLEEP | DM_RELEASE_REQUEST,
			DM_LOCK_MOUNT, root_site, DM_EMPTY, NULL, NULL, 0, 0,
			NULL, NULL, NULL);
		error = DM_RETURN_CODE(reply);
		dm_release(reply, 1);
		return(error);
	}

	s = spl6();

	/*
	 * M_ALLOW_LOCK -- obtain mountlock and allow normal locking
	 *		   stuff to proceed,
	 */
	if (f == M_ALLOW_LOCK) {
		if (mountlock_prevent == 0)
			panic("lock_mount: M_ALLOW_LOCK, prevent==0");

		/*
		 * Although we are guaranteed that all cleanup for the
		 * node that is currently being added has completed,
		 * someone might be doing recovery for a different
		 * node.   We must wait for them to finish with the
		 * mountlock.
		 */
		while (mountlock) {
			mount_waiting = 1;
			sleep(&mountlock, PSLEP);
		}
		mountlock_prevent = 0;
		goto lockit;
	}

	while (mountlock || mountlock_prevent) {
		switch (f) {
		case M_RECOVERY_LOCK:
			/*
			 * Special case of test and lock for recovery
			 * purposes.
			 */
			if (mountlock == 0)
				goto lockit;
			/* falls through */

		case M_TLOCK:
			splx(s);
			return(1);

		case M_LOCK:
		case M_PREVENT_LOCK:
			mount_waiting = 1;
			sleep(&mountlock, PSLEP);
			break;
		}
	}

lockit:
	if (f == M_PREVENT_LOCK)
		mountlock_prevent = 1;
	else
		mountlock = 1;
	mount_site = u.u_site;
	mount_procp = u.u_procp;
	mount_pid = mount_procp->p_pid;
	splx(s);
	return(0);
}

/*  Free the clusterwide mount/umount semaphore.  Wakeup other sites if needed.
 *  On all nodes this is called by the mount and umount system calls when
 *  they complete, and by recovery code when it cleans up local mounts.
 *
 *  On the rootserver only, this code is also called under interrupt to serve
 *  client DM_UNLOCK_MOUNT requests and when the initial mount table has
 *  been sent to a joining client.
 */

release_mount()
{
	int s;
	dm_message request;
	
	if ((my_site_status & CCT_CLUSTERED) && (my_site != root_site)) {
		request = dm_alloc(DM_EMPTY, WAIT);
		dm_send (request,
			DM_SLEEP | DM_RELEASE_REQUEST | DM_RELEASE_REPLY,
			DM_UNLOCK_MOUNT, root_site, DM_EMPTY, NULL, NULL, 0, 0,
			NULL, NULL, NULL);
		return(0);
	}

	s = spl6();
	mountlock = 0;
	if (mount_waiting) {
		mount_waiting = 0;
		wakeup(&mountlock);
	}
	splx(s);
	return(0);
}

/* Serve the lock/unlock request */

lockmount_serve(request)
dm_message request;
{
	struct lockmount_request *rp;
	int error = 0;

	if (DM_OP_CODE(request) == DM_LOCK_MOUNT) {
		rp = DM_CONVERT(request, struct lockmount_request);
		error = lock_mount(rp->function);
	} else {
		release_mount();
	}
	dm_quick_reply(error);
}


/*
 * Pack up a vnode.
 */
pack_vnode(vp, cvp)
register struct vnode *vp;
register struct packedvnode *cvp;
{
	register enum vfstype fstype;

	fstype = vp->v_fstype;
	/*For DUX, KFS and UFS vnodes we only need to pass the device index,
	 *the site, and the inode number.  At the server the rest can
	 *be gotten locally, and everywhere else only a pseudo vnode will
	 *be created anyway
	 */
	if (fstype == VDUX || fstype == VDUX_PV)
	{
		register struct inode *ip;

		ip = VTOI(vp);
		cvp->vnodetype = MOUNT_UFS;
		cvp->un.inodepacked.index = ip->i_dev & DEVINDEX;
		cvp->un.inodepacked.site = devsite(ip->i_dev);
		cvp->un.inodepacked.ino = ip->i_number;
	}
	else if (fstype == VDUX_CDFS || fstype == VDUX_CDFS_PV)
	{
		register struct cdnode *cdp;

		cdp = VTOCD(vp);
		cvp->vnodetype = MOUNT_CDFS;
		cvp->un.cdnodepacked.index = cdp->cd_dev & DEVINDEX;
		cvp->un.cdnodepacked.cdno = cdp->cd_num;
		cvp->un.cdnodepacked.site = devsite(cdp->cd_dev);
	}
	else if (fstype == VCDFS) {
		register struct cdnode *cdp;

		cdp = VTOCD(vp);
		cvp->vnodetype = MOUNT_CDFS;
		cvp->un.cdnodepacked.index = devindex(cdp->cd_dev, IFBLK);
		cvp->un.cdnodepacked.cdno = cdp->cd_num;
		cvp->un.cdnodepacked.site = my_site;
	}
	else if (fstype == VUFS)
	{
		register struct inode *ip;

		ip = VTOI(vp);
		cvp->vnodetype = MOUNT_UFS;
		cvp->un.inodepacked.index = devindex(ip->i_dev, IFBLK);
		cvp->un.inodepacked.site = my_site;
		cvp->un.inodepacked.ino = ip->i_number;
	}
	else if (fstype == VNFS)
	{
		register struct rnode *rp;

/*		rp = VTOR(vp);  */
		rp = (struct rnode *) (vp->v_data);
		cvp->vnodetype = MOUNT_NFS;
		cvp->un.rnodepacked.fhandle = rp->r_fh;
		cvp->un.rnodepacked.fattr = rp->r_nfsattr;
		cvp->un.rnodepacked.mntno = 
			((struct mntinfo *)vp->v_vfsp->vfs_data)->mi_mntno;
	}
	else
	{
		panic("Packing unknown vnode");
	}
}

/*
 *unpack a vnode
 */
int
unpack_vnode(cvp, vpp)
register struct packedvnode *cvp;
struct vnode **vpp;
{
	struct inode *ip;
	struct cdnode *cdp;
	register int vnodetype = cvp->vnodetype;

	if (vnodetype == MOUNT_UFS)
	{
		if (cvp->un.inodepacked.site == my_site)
		{
			/* I hold the vnode.  Use the real one.
			 * Note hat it should already be in core because
			 * of the original lookup
			 */
			ip = ifind(localdev(cvp->un.inodepacked.index),
				cvp->un.inodepacked.ino);
			if (ip == NULL)
			{
				return(ENOENT);
			}
			*vpp = ITOV(ip);
			/*
			 *We increment the reference count on the vnode.
			 *We do this even though the inode has already
			 *been igotten when we originally looked it up.
			 *This is because it was gotten on behalf of
			 *the original mounting site, and was
			 *entered into the appropriate sitemap.  We wish
			 *to remove any association from the mounting site,
			 *so this will permit the mounting site to release
			 *the vnode.
			 */
			VN_HOLD(*vpp);
			return (0);
		}
		else
		{
			return(stppsv(vpp, NULL,
				mkbrmtdev(cvp->un.inodepacked.site,
					cvp->un.inodepacked.index),
				cvp->un.inodepacked.ino));
		}
	}
	else if (vnodetype == MOUNT_CDFS)
	{
		if (cvp->un.cdnodepacked.site == my_site)
		{
			/* I hold the vnode.  Use the real one.
			 * Note hat it should already be in core because
			 * of the original lookup
			 */
			cdp = (struct cdnode *)
				CDFSCALL(CDFIND)(localdev(cvp->un.cdnodepacked.
				index),	cvp->un.cdnodepacked.cdno);
			if (cdp == NULL)
			{
				return(ENOENT);
			}
			*vpp = CDTOV(cdp);
			/*
			 *We increment the reference count on the vnode.
			 *We do this even though the cdnode has already
			 *been kgotten when we originally looked it up.
			 *This is because it was gotten on behalf of
			 *the original mounting site, and was
			 *entered into the appropriate sitemap.  We wish
			 *to remove any association from the mounting site,
			 *so this will permit the mounting site to release
			 *the vnode.
			 */
			VN_HOLD(*vpp);
			return (0);
		}
		else
		{
			return(stppsv_cdfs(vpp, NULL,
				mkbrmtdev(cvp->un.cdnodepacked.site,
					cvp->un.cdnodepacked.index),
				cvp->un.cdnodepacked.cdno));
		}
	}
	else if (vnodetype == MOUNT_NFS)
	{
		struct vfs *vfsp;

		vfsp = (struct vfs *)
			NFSCALL(NFS_FIND_MNT) (cvp->un.rnodepacked.mntno);
		if (vfsp == NULL)
		{
			return (ENOENT);
		}
		/*
		 * this call is indirect to allow optional configurability
		 * of NFS in the kernel.  This code will not even be
		 * executed if NFS is not present.
		 */
		*vpp = (struct vnode *) NFSCALL(NFS_MAKENFSNODE)
				(&(cvp->un.rnodepacked.fhandle),
				 &(cvp->un.rnodepacked.fattr), vfsp);
	}
	else
	{
		panic("Unpacking unknown vnode");
	}
	return (0);
}


/*
 *Pack a vfs and associated mount information into a DM message and a
 *buffer.  Return the message and buffers allocated and an OR of whichever
 *of the following are appropriate:  DM_REQUEST_BUF, DM_KEEP_REQUEST_BUF.
 */
pack_vfs(vfsp, messagep, bpp, bsizep, dmflagsp)
struct vfs *vfsp;
dm_message *messagep;
struct buf **bpp;
int *bsizep;
int *dmflagsp;
{
	register struct vfsmsg *rp;
	struct mount *mp;
	struct fs *fs;
	struct buf *fsbp;
	register int mtype;

	*messagep = dm_alloc(sizeof(struct vfsmsg), WAIT);
	rp = DM_CONVERT (*messagep, struct vfsmsg);
	rp->flag = vfsp->vfs_flag;
	rp->bsize = vfsp->vfs_bsize;
	rp->exroot = vfsp->vfs_exroot;
	rp->exflags = vfsp->vfs_exflags;
#ifdef	GETMOUNT
	rp->mnttime = vfsp->vfs_mnttime;
	strcpy(rp->fsname, vfsp->vfs_name);
#endif	GETMOUNT
	mtype = vfsp->vfs_mtype;
	/*XXX must handle covered NFS vnode*/
	if (vfsp->vfs_vnodecovered)	/*not root*/
	{
		rp->isroot = 0;
		pack_vnode(vfsp->vfs_vnodecovered,
			&rp->vnodeinfo.coveringvnode);
	}
	else	/*It's the root--Send the root vnode instead*/
	{
		if (mtype != MOUNT_UFS) panic("illegal fs type for root");
		rp->isroot = 1;
		pkrmi(VTOI(rootdir),&rp->vnodeinfo.rootinode);
	}
	if (mtype == MOUNT_UFS)
	{
		rp->vfsid.fstype = MOUNT_UFS;
		/* fill in the dev information from the mount table */
		mp = (struct mount *)(vfsp->vfs_data);
		if (bdevrmt(mp->m_dev))
		{
			rp->vfsid.un.isufs.dev_site = devsite(mp->m_dev);
			rp->vfsid.un.isufs.dev_index = mp->m_dev&DEVINDEX;
			rp->vfsinfo.isufs.rdev = mp->m_rdev;
			bcopy((caddr_t)mp->m_dfs,
			      (caddr_t)&rp->vfsinfo.isufs.dfs,
			      mp->m_dfs->dfs_structsize);
		}
		else
		{
			rp->vfsid.un.isufs.dev_index = devindex(mp->m_dev, IFBLK);
			rp->vfsid.un.isufs.dev_site = my_site;
			rp->vfsinfo.isufs.rdev = mp->m_rdev;
#ifdef QUOTA
			rp->vfsinfo.isufs.m_qflags = mp->m_qflags;
#endif QUOTA
			/* Copy the fs info that we need from the superblock */
			fs = mp->m_bufp->b_un.b_fs;
			rp->vfsinfo.isufs.dfs.dfs_structsize =
				sizeof(struct duxfs) - MAXMNTLEN +
					strlen(fs->fs_fsmnt) + 1;
			if (fs->fs_bsize <= MAXDUXBSIZE) {
				rp->vfsinfo.isufs.dfs.dfs_bsize = fs->fs_bsize;
				rp->vfsinfo.isufs.dfs.dfs_bmask = fs->fs_bmask;
				rp->vfsinfo.isufs.dfs.dfs_bshift = fs->fs_bshift;
			} else {
				/*
				 * These are defined in duxparam.h
				 * DUX can only deal with up to MAXDUXBSIZE
				 * blocks of IO, so fake DUX out to thinking
				 * everything is in MAXDUXBSIZE units.
				 */
				rp->vfsinfo.isufs.dfs.dfs_bsize = MAXDUXBSIZE;
				rp->vfsinfo.isufs.dfs.dfs_bmask = MAXDUXBMASK;
				rp->vfsinfo.isufs.dfs.dfs_bshift = MAXDUXBSHIFT;
			}
			rp->vfsinfo.isufs.dfs.dfs_fsize = fs->fs_fsize;
			rp->vfsinfo.isufs.dfs.dfs_fmask = fs->fs_fmask;
			rp->vfsinfo.isufs.dfs.dfs_fshift = fs->fs_fshift;
			strcpy(rp->vfsinfo.isufs.dfs.dfs_fsmnt, fs->fs_fsmnt);
		}
		/* We no longer send the whole superblock, so zero these
		 * fields
		 */
		*bpp = NULL;
		*bsizep = 0;
		*dmflagsp = 0;
	}
	else if (mtype == MOUNT_CDFS)
	{
		rp->vfsid.fstype = MOUNT_CDFS;
		/* fill in the dev information from the mount table */
		mp = (struct mount *)(vfsp->vfs_data);
		if (bdevrmt(mp->m_dev))
		{
			rp->vfsid.un.isufs.dev_site = devsite(mp->m_dev);
			rp->vfsid.un.isufs.dev_index = mp->m_dev&DEVINDEX;
			rp->vfsinfo.isufs.rdev = mp->m_rdev;
		}
		else
		{
			rp->vfsid.un.isufs.dev_index = devindex(mp->m_dev, IFBLK);
			rp->vfsid.un.isufs.dev_site = my_site;
			rp->vfsinfo.isufs.rdev = mp->m_rdev;
		}
		fsbp = mp->m_bufp;
		*bpp = fsbp;
		*bsizep = fsbp->b_bcount;
		*dmflagsp = DM_REQUEST_BUF|DM_KEEP_REQUEST_BUF;
	}
	else if (mtype == MOUNT_NFS)
	{
		struct mntinfo *mi = vftomi(vfsp);

		rp->vfsid.fstype = MOUNT_NFS;
		rp->vfsinfo.isnfs.addr = mi->mi_addr;
		rp->vfsinfo.isnfs.hard = mi->mi_hard;
		rp->vfsinfo.isnfs.mint = mi->mi_int;
		rp->vfsinfo.isnfs.devs = mi->mi_devs;
		rp->vfsinfo.isnfs.tsize = mi->mi_tsize;
		rp->vfsinfo.isnfs.stsize = mi->mi_stsize;
		rp->vfsinfo.isnfs.bsize = mi->mi_bsize;
		rp->vfsid.un.nfs_mntno = mi->mi_mntno;
		rp->vfsinfo.isnfs.timeo = mi->mi_timeo;
		rp->vfsinfo.isnfs.retrans = mi->mi_retrans;
		bcopy (mi->mi_hostname, rp->vfsinfo.isnfs.hostname,
			sizeof(mi->mi_hostname));
#ifdef GETMOUNT
		bcopy (mi->mi_fsmnt, rp->vfsinfo.isnfs.fsmnt,
		       sizeof(mi->mi_fsmnt));
#endif
		rp->vfsinfo.isnfs.dynamic  = mi->mi_dynamic;
		rp->vfsinfo.isnfs.ignore   = mi->mi_ignore;
		rp->vfsinfo.isnfs.noac     = mi->mi_noac;
		rp->vfsinfo.isnfs.nocto    = mi->mi_nocto;
		rp->vfsinfo.isnfs.acregmin = mi->mi_acregmin;
		rp->vfsinfo.isnfs.acregmax = mi->mi_acregmax;
		rp->vfsinfo.isnfs.acdirmin = mi->mi_acdirmin;
		rp->vfsinfo.isnfs.acdirmax = mi->mi_acdirmax;

		rp->vfsinfo.isnfs.nocto   = mi->mi_nocto;
		pack_vnode(mi->mi_rootvp,&rp->vfsinfo.isnfs.rootvnode);
		/*There is no superblock with this request so the buf should
		 *be NULL*/
		*bpp = NULL;
		*bsizep = 0;
		*dmflagsp = 0;
	}
}

/*
 *Unpack a vfs.  Create the appropriate vfs structure, mount table entry,
 *etc...
 *If we fail (e.g. mount table full), send back the appropriate error.
 *Note that during the initial mount table setup we do not yet have an
 *NSP (not even a limited one) so this function must be called under
 *interrupt.  It performs certain functions that are normally unsafe under
 *interrupt, such as allocating inodes.  However, there are not yet any
 *other processes running, process 0 is sleeping waiting for a response,
 *and there are not yet any allocated inodes, so it should all be safe.
 *When this function is called during a real mount, it is called by an NSP.
 *The variable is_real_mount is set up to indicate whether this is
 *from in initial mount or not.  A few actions behave differently in the
 *two cases.
 */
unpack_vfs(message)
dm_message message;
{
	register struct vfsmsg *rp;
	register struct vfs *vfsp=NULL;
	register struct mount *mp=NULL;
	register struct mntinfo *mi=NULL;
	int error;
	dev_t dev;
	struct inode *ip;
	struct vnode *coveredvp = NULL;
	struct vnode *nfsrootvp = NULL;
	extern struct inode *stprmi();
	int is_real_mount;
	struct duxfs *dfs = NULL;
	struct	buf	*np;
	struct	buf	*bp=NULL;
	register int	fstype;
	struct cdfs *cdfs = NULL;

	rp = DM_CONVERT (message, struct vfsmsg);
	is_real_mount = (DM_OP_CODE(message) != DM_INITIAL_MOUNT_ENTRY);
	if (is_real_mount)
	{
		dnlc_purge();
	}
	/*set up the covering vnode*/
	if (!rp->isroot)
	{
		error = unpack_vnode(&rp->vnodeinfo.coveringvnode,
			&coveredvp);
		if (error)
			goto errout;
	}
	/*set up the VFS*/
	MALLOC(vfsp, struct vfs *, sizeof(struct vfs), M_SUPERBLK, 
	is_real_mount? M_WAITOK:M_NOWAIT);
	if (vfsp == NULL)
		goto errout;
	VFS_INIT(vfsp, vfssw[rp->vfsid.fstype], (caddr_t)0, rp->vfsid.fstype);
	error = vfs_add(coveredvp,vfsp,0);
	if (error) {
		if (is_real_mount) {
			kmem_free((caddr_t)vfsp, sizeof(struct vfs));
			vfsp = (struct vfs *)NULL;
			goto errout;
		} else {
			panic("could not add to vfs list");
		}
	}
	/* Set up a mount entry*/
	fstype = rp->vfsid.fstype;
	if (fstype == MOUNT_UFS)
	{
		/*
		 *	Changed MALLOC call to kmalloc to save space. When
		 *	MALLOC is called with a variable size, the text is
		 *	large. When size is a constant, text is smaller due to
		 *	optimization by the compiler. (RPC, 11/11/93)
		 */
		dfs = (struct duxfs *)
			kmalloc((rp->vfsinfo.isufs.dfs.dfs_structsize),
			M_SUPERBLK, is_real_mount? M_WAITOK:M_NOWAIT);
		if (dfs == NULL)
			goto errout;
		bcopy((caddr_t)(&rp->vfsinfo.isufs.dfs),
		      dfs, rp->vfsinfo.isufs.dfs.dfs_structsize);
		if (rp->vfsid.un.isufs.dev_site == my_site)
			dev = localdev(rp->vfsid.un.isufs.dev_index);
		else
			dev = mkbrmtdev(rp->vfsid.un.isufs.dev_site, rp->vfsid.un.isufs.dev_index);
		/*allocate a mount entry.  We do this prior searching the mount
		 *table, because it is capable of sleeping.  During this sleep
		 *there is a small possibility that the same device will
		 *be mounted a second time.  If it turns out that the device
		 *is already mounted, we will release the memory.
		 */
		MALLOC(mp, struct mount *, 
		       sizeof(struct mount), M_DYNAMIC, 
		       is_real_mount? M_WAITOK:M_NOWAIT);
		if (mp == NULL) {
			error = EBUSY;
			goto errout;
		}
		if (getmp(dev))
		{
			error = EBUSY;
			/*We must free the mp here rather than letting
			 *errout do it because we have not set up the
			 *fields yet and errout will try to remove it from
			 *the hash list
			 */
			if (mp)
				kmem_free(mp, sizeof(struct mount));
			mp = NULL;
			goto errout;
		}

		if (mp == NULL)
		{
			error = ENOMEM;
			goto errout;
		}
		bzero(mp,sizeof(struct mount));
		mp->m_flag = MINTER;
		mp->m_dev = dev;
		mp->m_rdev = rp->vfsinfo.isufs.rdev;
		mp->m_site = rp->vfsid.un.isufs.dev_site;
#ifdef QUOTA
		mp->m_qflags = rp->vfsinfo.isufs.m_qflags;
#endif QUOTA

		/* m_bufp is only valid on the dux server for ufs file systems
		 * and we are setting up the mount table for the client here.
		 * Set the pointer to an invalid kernel address so that an
		 * attempt to dereference it will lead to a bus error.
		 */
		mp->m_bufp = (struct buf *)INV_K_ADR;
		mp->m_dfs = dfs;

		/* Set up the vfs's file system id */
		vfsp->vfs_fsid[0] = (long)dev;
		vfsp->vfs_fsid[1] = MOUNT_UFS;

		vfsp->vfs_data = (caddr_t)mp;
		mp->m_vfsp = vfsp;
		mountinshash(mp);
		/*If this is the root, set up the root vnode*/
		/*Note, we assume that the root is a UFS file system*/
		if (rp->isroot)
		{
			ip = stprmi(&rp->vnodeinfo.rootinode,mp);
			if (ip ==NULL)
				panic ("no inode for root");
			rootdir = ITOV(ip);
		}
	}
	else if (fstype == MOUNT_CDFS)
	{
		np = DM_BUF(message);
		MALLOC(cdfs, struct cdfs *, sizeof(struct cdfs), 
		       M_SUPERBLK, is_real_mount? M_WAITOK:M_NOWAIT);
		if (cdfs == NULL) {
			error = EBUSY;
			goto errout;
		}
		bcopy((caddr_t)np->b_un.b_addr, (caddr_t)cdfs,
			(u_int)np->b_bcount);
		if (rp->vfsid.un.isufs.dev_site == my_site)
			dev = localdev(rp->vfsid.un.isufs.dev_index);
		else
			dev = mkbrmtdev(rp->vfsid.un.isufs.dev_site, rp->vfsid.un.isufs.dev_index);
		if (getmp(dev)) {
			error = EBUSY;
			goto errout;
		}
		/*kmem_alloc will sleep until memory is available*/
		MALLOC(mp, struct mount *, 
		       sizeof(struct mount), M_DYNAMIC, 
		       is_real_mount? M_WAITOK:M_NOWAIT);
		if (mp == NULL) {
			error = EBUSY;
			goto errout;
		}
		bzero(mp, sizeof(struct mount));
		mp->m_flag = MINTER;
		mp->m_dev = dev;
		mp->m_rdev = rp->vfsinfo.isufs.rdev;
		mp->m_site = rp->vfsid.un.isufs.dev_site;
		mp->m_bufp = (struct buf *)INV_K_ADR;
		mp->m_dfs = (struct duxfs *)cdfs;

		/* Set up the vfs's file system id */
		vfsp->vfs_fsid[0] = (long)dev;
		vfsp->vfs_fsid[1] = MOUNT_CDFS;

		vfsp->vfs_data = (caddr_t)mp;
		mp->m_vfsp = vfsp;
		mountinshash(mp);
		/*If this is the root, set up the root vnode*/
		/*Note, we assume that the root is a UFS file system*/
		if (rp->isroot)
		{
			ip = stprmi(&rp->vnodeinfo.rootinode,mp);
			if (ip ==NULL)
				panic ("no inode for root");
			rootdir = ITOV(ip);
		}
	}
	else if (fstype == MOUNT_NFS)
	{
		MALLOC(mi, struct mntinfo *, sizeof(struct mntinfo), 
		       M_DYNAMIC, is_real_mount? M_WAITOK:M_NOWAIT);
		if (mi == NULL)
			goto errout;
		mi->mi_refct = 0;
		mi->mi_addr = rp->vfsinfo.isnfs.addr;
		mi->mi_hard = rp->vfsinfo.isnfs.hard;
		mi->mi_int = rp->vfsinfo.isnfs.mint;
		mi->mi_devs = rp->vfsinfo.isnfs.devs;
		mi->mi_tsize = rp->vfsinfo.isnfs.tsize;
		mi->mi_stsize = rp->vfsinfo.isnfs.stsize;
		mi->mi_bsize = rp->vfsinfo.isnfs.bsize;
		mi->mi_mntno = rp->vfsid.un.nfs_mntno;
		mi->mi_timeo = rp->vfsinfo.isnfs.timeo;
		mi->mi_retrans = rp->vfsinfo.isnfs.retrans;
		NFSCALL(NFS_ENTER_MNT)(mi);
		mi->mi_vfs = vfsp;
		mi->mi_printed = 0;
		bcopy (rp->vfsinfo.isnfs.hostname, mi->mi_hostname,
			sizeof(mi->mi_hostname));
#ifdef GETMOUNT
		bcopy (rp->vfsinfo.isnfs.fsmnt, mi->mi_fsmnt,
		       sizeof(mi->mi_fsmnt));
#endif GETMOUNT
		/*
		 * NFS 4.1 features
		 * Here we are doing something pretty slimy.  To allow
		 * 8.3 400s to work with 8.0 800s as servers, I am checking
		 * if the size of the message is as big as 8.3 expects.  If
		 * it is not, then we are talking to an 8.0 server and just
		 * zero out the extra fields. cwb 11/19/91
		 */

		if ((DM_HEADER(message)->dm_headerlen -
			sizeof(struct dm_transmit)) == sizeof (struct vfsmsg)) {
			mi->mi_dynamic = rp->vfsinfo.isnfs.dynamic;
			mi->mi_ignore  = rp->vfsinfo.isnfs.ignore;
			mi->mi_noac    = rp->vfsinfo.isnfs.noac;
			mi->mi_nocto   = rp->vfsinfo.isnfs.nocto;
                        mi->mi_acregmin = rp->vfsinfo.isnfs.acregmin;
                        mi->mi_acregmax = rp->vfsinfo.isnfs.acregmax;
                        mi->mi_acdirmin = rp->vfsinfo.isnfs.acdirmin;
                        mi->mi_acdirmax = rp->vfsinfo.isnfs.acdirmax;
		}
		else {
			mi->mi_dynamic = 0;
			mi->mi_ignore  = 0;
			mi->mi_noac    = 0;
			mi->mi_nocto   = 0;
                        mi->mi_acregmin = ACREGMIN;
                        mi->mi_acregmax = ACREGMIN; /* Set to mimic 8.0 times */
                        mi->mi_acdirmin = ACDIRMIN;
                        mi->mi_acdirmax = ACDIRMIN; /* Set to mimic 8.0 times */
		}
		mi->mi_curread  = mi->mi_tsize;
		mi->mi_curwrite = mi->mi_stsize;

		/* Set up the vfs's file system id */
		vfsp->vfs_fsid[0] = mi->mi_mntno;;
		vfsp->vfs_fsid[1] = MOUNT_NFS;

		vfsp->vfs_data = (caddr_t)mi;
		/*set up the root of the vfs*/
		error = unpack_vnode(&rp->vfsinfo.isnfs.rootvnode,
			&nfsrootvp);
		mi->mi_rootvp = nfsrootvp;
		nfsrootvp->v_flag |= VROOT;
		if (error)
			goto errout;
	}
	vfsp->vfs_bsize = rp->bsize;
	vfsp->vfs_exroot = rp->exroot;
	vfsp->vfs_exflags = rp->exflags;
	vfsp->vfs_flag = rp->flag|VFS_MLOCK;
#ifdef GETMOUNT
	vfsp->vfs_mnttime = rp->mnttime;
	strcpy(vfsp->vfs_name, rp->fsname);
	if (vfsp->vfs_mnttime > mount_table_change_time)
		mount_table_change_time = vfsp->vfs_mnttime;
#endif GETMOUNT
	/* If this is an initial mount, the mount entry is good and we
	 * can unlock the vfs and set the mount entry good.  However, if
	 * this is a real mount, we don't want to do that until the mount
	 * is confirmed.
	 */
	if (!is_real_mount)
	{
		vfs_unlock(vfsp);
		if(mp)
			mp->m_flag = MINUSE;
	}
	dm_quick_reply(0);
	return;
errout:
	/* restore mount table */
	if (mp) {
		mountremhash(mp);
		kmem_free((caddr_t)mp,sizeof(struct mount));
	}
	if (mi)
	{
		NFSCALL(NFS_DELETE_MNT)(mi);
		kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
		if (nfsrootvp)
			VN_RELE(nfsrootvp);
	}
	if (bp) brelse(bp);
	if (cdfs)
		kmem_free((caddr_t)cdfs, sizeof(struct cdfs));
	if (vfsp)
	{
		vfs_remove(vfsp);
		kmem_free((caddr_t)vfsp, sizeof(struct vfs));
	}
	if (coveredvp)
		VN_RELE(coveredvp);
	if (dfs)
		kmem_free((caddr_t)dfs, dfs->dfs_structsize);
	dm_quick_reply(error);
	/* note: mount table remains locked pending commit or abort */
}

/*
 * Get the mount table from the root server.  Actually calling this function
 * will cause the root server to send us the mount table in multiple requests,
 * one per mount entry.
 */
get_mount_table()
{
	dm_message request, reply;

	request = dm_alloc(DM_EMPTY, WAIT);
	reply = dm_send (request, DM_SLEEP|DM_RELEASE_REQUEST,
		DM_GETMOUNT, root_site, DM_EMPTY, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL);
	if (DM_RETURN_CODE(reply))
	{
		printf ("return = %d\n", DM_RETURN_CODE(reply));
		panic ("get_mount_table");
	}
	dm_release(reply,0);
}

/*
 * Send the complete mount table to the requesting site.  Traverse the list
 * of vfs's, sending a message for each one.  This function is called at the
 * root server when a new site joins the cluster.  For simplicity, this
 * function actually called by an NSP, sends a remote REQUEST, causing an
 * NSP (the limited one) to be invoked at the original requestor.
 */
/*ARGSUSED*/
send_mount_table(inmessage)
dm_message inmessage;
{
	register struct vfs *vfsp;
	dm_message request;
	struct buf *bp;
	int bsize;
	int dmflags;

	for (vfsp = rootvfs; vfsp != (struct vfs *)0; vfsp = vfsp->vfs_next)
	{
		pack_vfs (vfsp, &request, &bp, &bsize, &dmflags);
		dm_send (request,
			DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY|dmflags,
			DM_INITIAL_MOUNT_ENTRY, u.u_site, DM_EMPTY, NULL,
			bp, bsize, 0, NULL, NULL, NULL);
	}
	release_mount();
	dm_quick_reply(0);
}

/*
 * Get the mount table from the server, and do any other necessary
 * initialization
 */
dux_mountroot()
{
	get_mount_table();
	return(0);
}

/*
 * Service a getmdev request
 */
/*ARGSUSED*/
dux_getmdev_serve(request, dirvp, compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	dm_message reply;
	struct getmdev_reply *rp;

	if (compvp->v_type != VBLK)
		dm_quick_reply(ENOTBLK);
	else
	{
		reply = dm_alloc(sizeof(struct getmdev_reply), WAIT);
		rp = DM_CONVERT(reply, struct getmdev_reply);
		rp->dev = (VTOI(compvp))->i_device;
		if (rp->dev == NODEV)
			rp->dev = (((VTOI(compvp)->i_mode & IFMT) == IFBLK)
				? rootdev : block_to_raw(rootdev));
#ifdef	FULLDUX
		rp->site = (VTOI(compvp))->i_rsite;
#else	FULLDUX
#ifdef	LOCAL_DISC
		rp->site = u.u_site;
#else
		rp->site = root_site;
#endif	LOCAL_DISC
#endif	FULLDUX
		dm_reply(reply, 0, 0, NULL, NULL, NULL);
	}
	VN_RELE(compvp);
}

/*
 * Unpack a getmdev reply
 */
/*ARGSUSED*/
int
dux_getmdev_unpack(request, reply, dirvpp, compvpp, opcode, dependent)
dm_message request, reply;
struct vnode **dirvpp, **compvpp;
int opcode;
struct devandsite *dependent;
{
	struct getmdev_reply *rp = DM_CONVERT(reply, struct getmdev_reply);

	dependent->dev = rp->dev;
	dependent->site = rp->site;
	return (EOPCOMPLETE);
} 

/*
 * A mount confirmation (or abort) message
 */
struct commit_request		/*DUX MESSAGE STRUCTURE*/
{
	struct vfsid vfsid;	/*VFS identification*/
};

/*
 * A mount request message
 */
struct ufsmount_request		/*DUX MESSAGE STRUCTURE*/
{
	int flag;
	dev_t dev;		/*device being mounted (real device #)*/
	struct packedvnode coveringvnode;
};

/*
 *Send a ufs mount request to the device serving site
 */
int
send_ufs_mount(dev, site, vfsp, path)
dev_t dev;
site_t site;
struct vfs *vfsp;
char *path;
{
	dm_message request, reply;
	register struct ufsmount_request *rp;
	struct buf *bp;
	int stringlen = strlen(path)+1;
	int error;
	static struct vnode dummy_vnode;

	/* We allocate enough space to include the path.  Note that we add
	 * the length of the path to the size of the request message.
	 */
	request = dm_alloc(sizeof(struct ufsmount_request), WAIT);
	rp = DM_CONVERT (request, struct ufsmount_request);
	bp = geteblk(stringlen);
	rp->flag = vfsp->vfs_flag;
	rp->dev = dev;
	pack_vnode(vfsp->vfs_vnodecovered, &rp->coveringvnode);
	strcpy(bp->b_un.b_addr, path);
	/*The following code is grotesque, but I can't think of a better
	 *solution.  When the request gets to the device serving site, it
	 *will broadcast the mount to all sites, including this site.  At
	 *that point, this site will try to set up a new vfs, covering vnode,
	 *etc.  If the covering vnode happens to really be at this site,
	 *we will get a mount device busy when we try to readd the new vfs,
	 *because, in setting up for the mount, we have already marked this
	 *vfs as mounted upon.  Therefore, we don't want this vnode marked
	 *as mounted on.  Now if we simply release it, then when if we complete
	 *the mount with an error, it will try to release it again, resulting
	 *in a panic.  Therefore, what we do is to change the original vfs
	 *to point to a dummy vnode and uncover the real vnode.  We always
	 *return with an error (EOPCOMPLETE if everything worked), and
	 *let the original mount code release the dummy vfs and uncover the
	 *dummy vnode.
	 */
	vfsp->vfs_vnodecovered->v_vfsmountedhere = (struct vfs *)0;
	vfsp->vfs_vnodecovered = &dummy_vnode;
	reply = dm_send(request, DM_SLEEP|DM_RELEASE_REQUEST|DM_REQUEST_BUF,
		DM_UFS_MOUNT, site, DM_EMPTY, NULL, bp, stringlen, 0,
		NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	dm_release(reply, 0);
	/* We originally allocated a VFS in smount.  However, since we
	 * are letting the device server do the actual mount, and since when
	 * we receive the propagated mount we will create a new VFS, return
	 * EOPCOMPLETE so that the original VFS is released.
	 */
	if (error == 0)
		error = EOPCOMPLETE;
	return (error);
}

/*
 *Service the UFS mount request
 */
serve_ufs_mount(request)
dm_message request;
{
	register struct ufsmount_request *rp;
	struct vfs *vfsp = NULL;
	struct vnode *coveredvp;
	int error;

	rp = DM_CONVERT(request, struct ufsmount_request);
	/* Set up the covering vnode */
	error = unpack_vnode(&rp->coveringvnode,&coveredvp);
	if (error)
	{
		dm_quick_reply(error);
		return;
	}
	/* lock_mount(); */
	dnlc_purge();
	/* Initialize the vfs */
	MALLOC(vfsp, struct vfs *, sizeof(struct vfs), M_SUPERBLK, 
		M_NOWAIT);
	if (vfsp == NULL) {
		error = ENOMEM;
		goto alloc_error;
	}
	VFS_INIT (vfsp, vfssw[MOUNT_UFS], (caddr_t)0, MOUNT_UFS);
	if (coveredvp->v_fstype == VUFS)	/*If it's local*/
	{
		ilock(VTOI(coveredvp));
	}
	error = vfs_add(coveredvp, vfsp, 0);
	if (!error)
	{
		vfsp->vfs_flag = rp->flag|VFS_MLOCK;
		error = mountfs(rp->dev, (DM_BUF(request))->b_un.b_addr, vfsp);
	}
	if (coveredvp->v_fstype == VUFS)	/*If it's local*/
	{
		iunlock(VTOI(coveredvp));
	}
	if (!error)
		vfs_unlock(vfsp);
	else
	{
		vfs_remove(vfsp);
		VN_RELE(coveredvp);
		kmem_free((caddr_t)vfsp, sizeof(struct vfs));
	}
alloc_error:
	dm_quick_reply (error);
}


/*
 * Broadcast the mount to all other sites in the cluster
 */
int
broadcast_mount(vfsp)
struct vfs *vfsp;
{
	dm_message request, reply;
	struct vfsmsg *rp;
	struct commit_request *cp;
	struct buf *bp;
	int bsize;
	int dmflags;
	int error;
	struct vfsid vfsid;

	/* If there's noone to listen, don't shout at everyone */
	if (!(my_site_status & CCT_CLUSTERED))
		return (0);
	/* Good, we have an attentive listening audience.  Send out the info*/
	pack_vfs (vfsp, &request, &bp, &bsize, &dmflags);
	/*save the vfs ID for the commit message*/
	rp = DM_CONVERT(request, struct vfsmsg);
	vfsid = rp->vfsid;
	reply = dm_send (request,
		DM_SLEEP|DM_RELEASE_REQUEST|dmflags,
		DM_MOUNT_ENTRY, DM_CLUSTERCAST, DM_EMPTY, NULL,
		bp, bsize, 0, NULL, 0, 0);
	/* Should make error checking more intelligent.  For example, a dead
	 * site should not cause an error.
	 */
	error = DM_RETURN_CODE(reply);
	dm_release(reply, 1);
	/* Either commit or abort */
	request = dm_alloc(sizeof(struct commit_request), WAIT);
	cp = DM_CONVERT(request, struct commit_request);
	/* Copy the device info from the original request to the new one */
	cp->vfsid = vfsid;
	reply = dm_send(request,DM_RELEASE_REQUEST|DM_SLEEP,
		error?DM_ABORT_MOUNT:DM_COMMIT_MOUNT, DM_CLUSTERCAST, DM_EMPTY,
		NULL, NULL, 0, 0, NULL, NULL, NULL);
	if (!error) error = DM_RETURN_CODE(reply);
	dm_release(reply, 1);
	return (error);
}

/* Commit (or abort) a mount */
mount_commit(request)
dm_message request;
{
	struct commit_request *cp;
	register struct vfs *vfsp=NULL;

	cp = DM_CONVERT(request, struct commit_request);
	switch (cp->vfsid.fstype) {
	case MOUNT_CDFS: 
	case MOUNT_UFS: {
		register dev_t dev;
		register struct mount *mp;

		/* determine the device number that I use locally */
		if (cp->vfsid.un.isufs.dev_site == my_site)
			dev = localdev(cp->vfsid.un.isufs.dev_index);
		else
			dev = mkbrmtdev(cp->vfsid.un.isufs.dev_site,
				cp->vfsid.un.isufs.dev_index);
		/*now search the mount table */
		mp = getmp(dev);
		if (mp && (mp->m_flag & MINTER))
		{
			vfsp = mp->m_vfsp;
			if (DM_OP_CODE(request) == DM_COMMIT_MOUNT)
			{
				mp->m_flag = MINUSE;
				vfs_unlock(vfsp);
			}
			else	/*abort*/
			{
			   if(mp->m_bufp && mp->m_bufp != (struct buf *)INV_K_ADR)
				brelse(mp->m_bufp);
			   if(mp->m_dfs && mp->m_dfs != (struct duxfs *)INV_K_ADR)
				kmem_free((caddr_t)mp->m_dfs,
					  mp->m_dfs->dfs_structsize);
				mountremhash(mp);
				kmem_free((caddr_t)mp, sizeof(struct mount));
			}
		}
		break;
	}
	case MOUNT_NFS:

		vfsp = (struct vfs *)
			NFSCALL(NFS_FIND_MNT) (cp->vfsid.un.nfs_mntno);
		if (vfsp)	/*if we found it*/
		{
			if (DM_OP_CODE(request) == DM_COMMIT_MOUNT)
			{
				vfs_unlock(vfsp);
			}
			else	/*abort*/
			{
				struct mntinfo *mi =
					((struct mntinfo *)vfsp->vfs_data);
				VN_RELE(mi->mi_rootvp);
				NFSCALL(NFS_DELETE_MNT)(mi);
				kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
			}
		}
		break;
	default:
		panic("mount_commit");
	}

	if (DM_OP_CODE(request) == DM_ABORT_MOUNT && vfsp)
	{
		struct vnode *coveredvp = vfsp->vfs_vnodecovered;

		vfs_remove(vfsp);
		VN_RELE(coveredvp);
		kmem_free(vfsp, sizeof(struct vfs));
	}
	dm_quick_reply(0);
}

/* An umount device message */
struct umount_dev_msg		/*DUX MESSAGE STRUCTURE*/
{
	int	encoded;	/* True if encoded device number */
	dev_t	dev;
};

/*
 * Send an umount to the site holding the device specified by the inode
 */
int
send_umount_dev(dev, site, encoded)
dev_t dev;
site_t site;
{
	dm_message request, reply;
	struct umount_dev_msg *rp;
	int error;

	request = dm_alloc(sizeof(struct umount_dev_msg), WAIT);
	rp = DM_CONVERT(request, struct umount_dev_msg);
	rp->dev = dev;
	rp->encoded = encoded;
	reply = dm_send (request, DM_SLEEP|DM_RELEASE_REQUEST,
		DM_UMOUNT_DEV, site, DM_EMPTY, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL);
	error = DM_RETURN_CODE(reply);
	dm_release(reply, 0);
	return (error);
}

/*
 * Serve the umount request
 */
umount_dev_serve(request)
dm_message request;
{
	struct umount_dev_msg *rp;
	dev_t dev;

	rp = DM_CONVERT(request, struct umount_dev_msg);
	dev = rp->dev;
	if (rp->encoded == DEV_RMT)
		dev = localdev(dev);
#ifndef _WSIO
	/*
	 * "dev" was sent from a remote site and the logical unit within
	 * "dev" has not been mapped to an mgr index yet so map it here
	 * before attempting a mount structure lookup.   mry 2/15/91
	 */
	else if (rp->encoded == DEV_LU_UNMAPPED)
		(void) map_lu_to_mi(&dev, IFBLK, 0);
#endif /* not _WSIO */

	dm_quick_reply(umount_dev(dev));
}

/*
 * An unmount request
 */
struct umount_request		/*DUX MESSAGE STRUCTURE*/
{
	dev_t	 dev_index;	/*index of device being unmounted*/
	site_t	 dev_site;	/*site of device being unmounted*/
};

global_unmount(dev)
dev_t dev;
{
	dm_message request;
	struct umount_request *rp;

	/* If there's noone to listen, don't shout at everyone */
	if (!(my_site_status & CCT_CLUSTERED))
		return;
	/* Good, we have an attentive listening audience.  Send out the info*/
	request = dm_alloc(sizeof(struct umount_request), WAIT);
	rp = DM_CONVERT(request, struct umount_request);
	rp->dev_index = devindex(dev, IFBLK);
	rp->dev_site = my_site;
	dm_send(request,
		DM_SLEEP|DM_RELEASE_REQUEST|DM_RELEASE_REPLY,
		DM_UMOUNT, DM_CLUSTERCAST, DM_EMPTY, NULL, NULL, 0, 0,
		NULL, NULL, NULL);
}

/*
 * Service a global unmount
 */
global_umount_serve(request)
dm_message request;
{
	struct umount_request *rp;
	dev_t dev;
	register struct mount *mp;
	register struct vfs *vfsp;
	int	fstype;

#ifdef	NSYNC
	update(0,1,0);
#else
	update(0,1);
#endif	/* NSYNC */
	rp = DM_CONVERT(request, struct umount_request);
	dev = mkbrmtdev(rp->dev_site, rp->dev_index);
	/*now search the mount table */
	mp = getmp(dev);
	if (mp && (mp->m_flag & MINUSE))
	{
		vfsp = mp->m_vfsp;
		vfs_lock(vfsp);
		dnlc_purge();
		fstype = vfsp->vfs_mtype;
		if(mp->m_bufp && mp->m_bufp != (struct buf *)INV_K_ADR)
			brelse(mp->m_bufp);
		if(mp->m_dfs && mp->m_dfs != (struct duxfs *)INV_K_ADR)
		kmem_free((caddr_t)mp->m_dfs,
			  mp->m_dfs->dfs_structsize);
		mountremhash(mp);
		kmem_free((caddr_t)mp,sizeof(struct mount));
		{
			struct vnode *coveredvp = vfsp->vfs_vnodecovered;

			vfs_remove(vfsp);
			VN_RELE(coveredvp);
		}
		kmem_free((caddr_t)vfsp, sizeof(struct vfs));
		if (fstype == MOUNT_UFS) 
                {
#ifdef QUOTA
                   (void)iflush(dev, (struct inode *)NULL);
#else not QUOTA
                   (void)iflush(dev);
#endif QUOTA
                }
		else if (fstype == MOUNT_CDFS) (void)CDFSCALL(CDFLUSH)(dev);
		else panic("global_umount_serve: illegal fs type");
#ifdef GETMOUNT
		mount_table_change_time = time.tv_sec;
#endif
	}
	dm_quick_reply(0);
}

/*
 * Service the initial unmount request.  Called when the parameter to unmount
 * is a remote pathname.  If the device is remote from here we will go remote
 * again.
 */
/*ARGSUSED*/
dux_umount_serve(request,dirvp,compvp)
dm_message request;
struct vnode *dirvp, *compvp;
{
	int error;

	if (compvp->v_type == VBLK)
		error = umount_dev_vp(compvp);
	else if (compvp->v_flag & VROOT) {
#if !defined(FULLDUX) || !defined(LOCAL_DISC)
		if (compvp->v_vfsp->vfs_mtype == MOUNT_UFS) {
			error = EINVAL;
			VN_RELE(compvp);
		} else
#endif	!FULLDUX || !LOCAL_DISC
		error = umount_root(compvp);
	}
	else {
		error = ENOTBLK;
		VN_RELE(compvp);
	}
	dm_quick_reply(error);
}
