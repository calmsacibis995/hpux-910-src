/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/ufs/RCS/ufs_vnops.c,v $
 * $Revision: 1.22.83.9 $       $Author: dkm $
 * $State: Exp $	$Locker:  $
 * $Date: 94/05/19 13:48:14 $
 */

/* HPUX_ID: @(#)ufs_vnops.c	55.1		88/12/23 */

/*
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
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

/*	@(#)ufs_vnodeops.c 1.1 86/02/03 SMI	*/
/*	@(#)ufs_vnodeops.c	2.1 86/04/14 NFSSRC */
/*      USED TO BE CALLED ufs_vnodeops.c, DLP   */

#include "../h/debug.h"
#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/vmmac.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../h/kernel.h"
#include "../h/pfdat.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../h/mount.h"
#include "../ufs/fsdir.h"
#include "../h/kern_sem.h"

#ifdef POSIX
#include "../h/tty.h"
#endif POSIX
#ifdef QUOTA
#include "../ufs/quota.h"
#endif
#include "../dux/dm.h"
#include "../dux/dux_dev.h"
#include "../dux/cct.h"
#include "../dux/duxfs.h"

#include "../h/unistd.h"

extern int ufs_open();
extern int ufs_close();
extern int ufs_rdwr();
extern int ufs_ioctl();
extern int ufs_select();
extern int ufs_getattr();
extern int ufs_setattr();
extern int ufs_access();
extern int ufs_lookup();
extern int ufs_create();
extern int ufs_remove();
extern int ufs_link();
extern int ufs_rename();
extern int ufs_mkdir();
extern int ufs_rmdir();
extern int ufs_readdir();
extern int ufs_symlink();
extern int ufs_readlink();
extern int ufs_fsync();
extern int ufs_inactive();
extern int ufs_bmap();
extern int ufs_badop();
extern int ufs_bread();
extern int ufs_brelse();
#ifdef ACLS
extern int ufs_setacl();
extern int ufs_getacl();
#endif
#ifdef POSIX
extern int ufs_pathconf();
extern int ufs_fpathconf();
#endif POSIX
extern int ufs_lockctl();
extern int ufs_lockf();
extern int ufs_fid();
extern int ufs_fsctl();
extern int vfs_prefill();
extern int vfs_pagein();
extern int vfs_pageout();
extern struct buf *breadan();

#define MAXREADAHEAD 32

struct vnodeops ufs_vnodeops = {
	ufs_open,
	ufs_close,
	ufs_rdwr,
	ufs_ioctl,
	ufs_select,
	ufs_getattr,
	ufs_setattr,
	ufs_access,
	ufs_lookup,
	ufs_create,
	ufs_remove,
	ufs_link,
	ufs_rename,
	ufs_mkdir,
	ufs_rmdir,
	ufs_readdir,
	ufs_symlink,
	ufs_readlink,
	ufs_fsync,
	ufs_inactive,
	ufs_bmap,
	ufs_badop,
	ufs_bread,
	ufs_brelse,
	ufs_badop,
#ifdef ACLS
	ufs_setacl,
	ufs_getacl,
#endif
#ifdef	POSIX
	ufs_pathconf,
	ufs_pathconf,
#endif	POSIX
	ufs_lockctl,
	ufs_lockf,
	ufs_fid,
	ufs_fsctl,
	vfs_prefill,
	vfs_pagein,
	vfs_pageout,
	NULL,
	NULL,
};




#if defined(UFS_GETSB) || defined(PF_ASYNC)
/*
 * ufs_fsctl()
 *
 * Return UFS-specific information about the filesystem on which vp resides
 *
 */
/*ARGSUSED*/
ufs_fsctl(vp, command, uiop, cred)
struct	vnode	*vp;
register int	command;
struct uio	*uiop;
int	cred;
{
	register struct	fs	*fsp;
	register struct inode	*ip;
	char	*infop;
	int	datalen;


	ip = VTOI(vp);
	fsp = ip->i_fs;

	switch (command) {

#ifdef UFS_GETSB
		case UFS_GET_SB:
			infop = fsp;
			datalen = sizeof(struct fs);			
			return(uiomove(infop, datalen, UIO_READ, uiop));
			break;
#endif
#ifdef PF_ASYNC
		case UFS_SET_ASYNC:
			ip->i_flags |= IC_ASYNC;
			break;

		case UFS_CLEAR_ASYNC:
			ip->i_flags &= ~IC_ASYNC;
			break;
#endif		
		default:
			return(EINVAL);
	}

	return(0);
}
#else
ufs_fsctl()
{
        return(EINVAL);
}
#endif


inc_sitecount(mapp, site)
register struct sitemap *mapp;
site_t site;
{
	INC_SITECOUNT(mapp, site);
}

dec_sitecount(mapp, site)
register struct sitemap *mapp;
site_t site;
{
	DEC_SITECOUNT(mapp, site);
}


/*
 * This function is called whenever a multiple open occurs on a character
 * devices, returning a new devnum that requires creation of a new
 * temporary inode.  (Note that for a dux inode dux_clone is called
 * instead).
 * Allocate new inode with new device number. Release old inode. Set vpp to
 * point to new one.  This inode will go away when the last reference to it
 * goes away.
 * Warning: if you stat this, and try to match it with a name in the
 * filesystem you will fail, unless you had previously put names in that
 * match.
 */
int
ufs_clone(ip,nipp,flag,devnum)
	struct inode *ip;
	struct inode **nipp;
	int flag;
	int devnum;
{
	struct inode *nip;

	use_dev_vp(ITOV(ip));
	nip = ialloc(ip, dirpref(ip->i_fs), (int)ip->i_mode);
	drop_dev_vp(ITOV(ip));
	if (nip == (struct inode *)0)
		return (ENXIO);
	imark(nip, IACC|IUPD|ICHG);
	nip->i_mode = ip->i_mode;
	nip->i_vnode.v_type = ip->i_vnode.v_type;
	nip->i_nlink = 0;
	nip->i_uid = ip->i_uid;
	nip->i_gid = ip->i_gid;
	if (ip->i_device != NODEV) {
		nip->i_rdev = devnum;
	} else if ((ip->i_mode & IFMT) == IFBLK) {
		nip->i_rdev = makedev(major(rootdev), devnum);
	} else {
		nip->i_rdev = makedev(major(block_to_raw(rootdev)), devnum);
	}
	nip->i_vnode.v_rdev = nip->i_device = nip->i_rdev;
	iunlock(nip);
	isynclock(ip);
	dec_sitecount(&(ip->i_opensites), u.u_site);
	if (flag & FWRITE)
		dec_sitecount(&(ip->i_writesites), u.u_site);
	checksync(ip);
	isyncunlock(ip);
	VN_RELE(ITOV(ip));
	isynclock(nip);
	inc_sitecount(&(nip->i_opensites), u.u_site);
	if (flag & FWRITE)
		inc_sitecount(&(nip->i_writesites), u.u_site);
	checksync(nip);
	isyncunlock(nip);
	*nipp = nip;
	return(0);
}


/*ARGSUSED*/
int
ufs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	struct inode *ip;
	int is_fifo;
	int error;
#ifdef notdef
	int opensites;
	int rdcnt;
	int wrcnt;
#endif /* notdef */

	ip = VTOI(*vpp);
	is_fifo = (ip->i_mode&IFMT) == IFIFO;
	/*
	 * Synchronize the inode if necessary
	 */
retry:
	isynclock(ip);
	if (flag & FWRITE) {
		inc_sitecount(&(ip->i_writesites), u.u_site);
		if ((*vpp)->v_flag & VTEXT) {
			/* xrele was done in vns_open */
			dec_sitecount(&(ip->i_writesites), u.u_site);
			isyncunlock(ip);
			return ETXTBSY;
		}
	}
	inc_sitecount(&(ip->i_opensites), u.u_site);
	if (is_fifo && (flag & FREAD))
		inc_sitecount(&(ip->i_fifordsites), u.u_site);
	/* if checksync returns an error, then it means that */
	/* a switch to synchronous mode was indicated, but at least */
	/* one site who was suppose to switch could not find the */
	/* inode in its incore table.  This can happen if the dm_reply */
	/* for a dux_copen_serve() has not gotten though yet. */
	ip->i_flag |= IOPEN;
#ifdef notdef
	if (is_fifo) {
		/* if fifo then if previous opens blocked then */
		/* they won't be found on the opening site yet */
		/* because they are asleep on the server waiting */
		/* for an open to happen which will wake them up */
		/* in openp().  therefore don't call checksync() */
		/* who calls notifysync() because there is noone */
		/* to notify. */
		opensites = gettotalsites(&ip->i_opensites);
		rdcnt = ip->i_frcnt;
		wrcnt = ip->i_fwcnt;
		if ((opensites > 1) &&
			(((flag & FWRITE) && (rdcnt == 0)) ||
			  ((flag & FREAD)  && (wrcnt == 0)) ||
			  ((flag & FWRITE) && (wrcnt == 0) && (rdcnt != 0)) ||
			  ((flag & FREAD)  && (rdcnt == 0) && (wrcnt != 0)))) {
				ip->i_flag |= ISYNC;
				error = 0;
		} else {
			error = checksync(ip);
		}
	} else {
		error = checksync(ip);
	}
#else
	error = checksync(ip);
#endif
	ip->i_flag &= ~IOPEN;
	if (error) {
		ip->i_flag &= ~ISYNC;
		dec_sitecount(&(ip->i_opensites), u.u_site);
		if (flag & FWRITE)
			dec_sitecount(&(ip->i_writesites), u.u_site);
		if (is_fifo && (flag & FREAD))
			dec_sitecount(&(ip->i_fifordsites), u.u_site);
		isyncunlock(ip);
		sleep((caddr_t)&lbolt, PINOD);
		goto retry;
	}
	isyncunlock(ip);
	/* If we are the client, open the device
	 * Also, if we are the server of a fifo open it
	 */
	if (!u.u_nsp || (is_fifo)) {
		error = openi(vpp, flag, ufs_clone);
		if (error) {
			isynclock(ip);
			dec_sitecount(&(ip->i_opensites), u.u_site);
			if (flag & FWRITE)
				dec_sitecount(&(ip->i_writesites), u.u_site);
			if (is_fifo && (flag & FREAD))
				dec_sitecount(&(ip->i_fifordsites), u.u_site);
			checksync(ip);
			isyncunlock(ip);
		}
		return(error);
	} else
		return (0);
}

int
openi (vpp, flag, clonefunc)		/* for device files */
	struct vnode **vpp;
	int flag;
	int (*clonefunc)();  /*function for cloning inode on multiple  opens*/
{

	struct inode *ip;
	dev_t dev;
	int newdev;
	int error;

	ip = VTOI(*vpp);

	/*
	 * Do open protocol for inode type.
	 */
	dev = ip->i_device;

	/*
	 * Setjmp in case open is interrupted.
	 * If it is, close and return error.
	 */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		PSEMA(&filesys_sema);
		/*we cannot call ufs_close, because this also decrements the
		 *sitemaps which the upper level expects to do on an error.
		 *Instead, we just close the device and/or pipe
		 */
		if (!setjmp(&u.u_qsave)) {
			switch(ip->i_mode & IFMT) {

			case IFCHR:
#ifdef FULLDUX
				closed (dev, IFCHR, flag, u.u_site);
#else
				closed (dev, IFCHR, flag);
#endif
				break;
			case IFBLK:
#ifdef FULLDUX
				closed (dev, IFBLK, flag, u.u_site);
#else
				closed (dev, IFBLK, flag);
#endif
				break;
			case IFIFO:
				use_dev_dvp(ip->i_devvp);
				closep(ip, flag);
				drop_dev_dvp(ip->i_devvp);
				break;
			}
		} else
			PSEMA(&filesys_sema);

		return (error);
	}

	switch (ip->i_mode & IFMT) {

	case IFCHR:
		newdev = dev;
#ifdef FULLDUX
		error = opend(&dev, IFCHR, ip->i_rsite, flag, &newdev);
#else  /* FULLDUX */
		if (ip->i_rsite != 0 && ip->i_rsite != my_site)
			return (EOPNOTSUPP);
		error = opend(&dev, IFCHR, flag, &newdev);
#endif /* FULLDUX */
		ip->i_vnode.v_rdev = ip->i_rdev = dev;
#ifdef __hp9000s800
		ip->i_vnode.v_flag |= VMI_DEV;
#endif /* hp9000s800 */

		/*
		 * Test for new minor device inode allocation
		 */
#ifdef	FULLDUX
		if (error == 0 && !cdevrmt(dev) && newdev != dev)
#else	FULLDUX
		if (error == 0 && newdev != dev)
#endif	FULLDUX
		{
			struct inode *nip;

			error = (*clonefunc)(ip, &nip, flag, newdev);
			if (error)
			{
				/*
				 * Must close the device we just opened,
				 * not the original.
				 */
#ifdef FULLDUX
				(void) closed (newdev, IFCHR, flag, u.u_site);
#else
				(void) closed (newdev, IFCHR, flag);
#endif
				return (error);
			}
			ip = nip;
			*vpp = ITOV(ip);
		}
		break;

	case IFBLK:
#ifdef FULLDUX
		error = opend(&dev, IFBLK, ip->i_rsite, flag, 0);
#else  /* FULLDUX */
		if (ip->i_rsite != 0 && ip->i_rsite != my_site)
			return (EOPNOTSUPP);
		error = opend(&dev, IFBLK, flag, 0);
#endif /* FULLDUX */
		ip->i_vnode.v_rdev = ip->i_rdev = dev;
#ifdef __hp9000s800
		ip->i_vnode.v_flag |= VMI_DEV;
#endif /* hp9000s800 */
		break;
	case IFSOCK:
		error = EOPNOTSUPP;
		break;
	case IFIFO:
		error = openp(ip,flag);
		break;
	default:
		error = 0;
		break;
	}
	return (error);
}

/*ARGSUSED*/
int
ufs_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct inode *ip;
	register int mode;
	dev_t dev;
	/*
	 * setjmp in case close is interrupted
	 */
	if (setjmp(&u.u_qsave)) {
		PSEMA(&filesys_sema);
		return (EINTR);
	}
	ip = VTOI(vp);
	unlock(ip);
	dev = ip->i_rdev;
	mode = ip->i_mode & IFMT;
	/* If a fragment needs to be relocated, do so */
	if (ip->i_flag & IFRAG)
	{
		use_dev_vp(vp);
		ilock(ip);
		if (ip->i_flag & IFRAG) /*may have changed while locking it*/
			frag_fit(ip);
		iunlock(ip);
		drop_dev_vp(vp);
	}
	/*
	 * Desynchronize the inode if necessary
	 */
	isynclock(ip);
	dec_sitecount(&(ip->i_opensites), u.u_site);
	if (flag & FWRITE)
	{
		dec_sitecount(&(ip->i_writesites), u.u_site);
		ip->i_flag |= ICHG;
		ip->i_fversion++;
	}
	if ((flag & FREAD) && (mode == IFIFO))
	{
		dec_sitecount(&(ip->i_fifordsites), u.u_site);
	}
	checksync(ip);
	isyncunlock(ip);
	switch(mode) {

	case IFCHR:
#ifdef FULLDUX
		closed (dev, IFCHR, flag, u.u_site);
#else
		closed (dev, IFCHR, flag);
#endif
		return (0);

	case IFBLK:
#ifdef FULLDUX
		closed (dev, IFBLK, flag, u.u_site);
#else
		closed (dev, IFBLK, flag);
#endif
		return (0);

	case IFIFO:
		use_dev_vp(vp);
		closep(ip, flag);
		drop_dev_vp(vp);
	}
	bordend();
	return (0);
}

/*
 * read or write a vnode
 */
/*ARGSUSED*/
int
ufs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;
	int type;
	struct flock flock;
	int do_use_dev;
 	struct vnode *dvp;

	ip = VTOI(vp);
	type = ip->i_mode&IFMT;

	do_use_dev = 0;
	if (type == IFREG || type == IFIFO) {
		++do_use_dev;
		dvp = ip->i_devvp;
	}
	else
	/* char devices blocked in driver */
	if (type == IFBLK) {
		++do_use_dev;
		dvp = devtovp(ip->i_rdev);
	}
	if (do_use_dev)
		use_dev_dvp(dvp);

	if (type == IFREG || type == IFIFO || type == IFNWK || type == IFDIR) {
		/* don't have file pointer */

		/*
		 * only check the lock if its a regular file
		 */
		if (type == IFREG) {
			/*
			 * See if the region is locked, and wait for it to
			 * become unlocked if it is.  Return only on DEADLOCK
			 * or out of table space.
			 */
			if (ip->i_locklist
			    && (error = locked(F_LOCK, ip, uiop->uio_offset,
				  uiop->uio_offset+uiop->uio_iov->iov_len,
				  (rw==UIO_READ)?L_READ:L_WRITE, NULL,
				  flock.l_type,uiop->uio_fpflags))) {
 				error = u.u_error;
 				goto out;
 			}
		}
		ilock(ip);
		if ((ioflag & IO_APPEND) && (rw == UIO_WRITE)) {
			/*
			 * in append mode start at end of file.
			 */
			uiop->uio_offset = ip->i_size;
		}
		error = rwip(ip, uiop, rw, ioflag);
		iunlock(ip);
	} else {
		error = rwip(ip, uiop, rw, ioflag);
	}
out:
	if (do_use_dev)
	{
		drop_dev_dvp(dvp);
		if (type == IFBLK)
			VN_RELE(dvp);
	}
	return (error);
}
/*
 * The semantics that are provided by rwip and the routines that it calls
 * (most notably bmap and realloccg) when synchronous i/o is specified
 * are that when the call completes the data is on the disk.  There is a
 * window of vulnerability in rwip where we could have "garbage" in the file
 * if we are doing synchronous i/o.  This is because we update the size of
 * the file after the call to bmap completes but before we have done the
 * uiomove to put the data into the block.  If bmap has allocated a new block
 * or extended a fragment to occupy a whole block, because we have updated the
 * size of the inode if the inode is synched to the disk and we crash before
 * doing the uiomove and writing out the data synchronously we would have
 * garbage in the file because we didn't synchronously write out the newly
 * allocated block.  The garbage that is in the file is whatever happened
 * to be in that block on the disk.  This is viewed as an acceptable risk
 * in order to obtain decent NFS and local synchronous write performance.
 * In the case of NFS the risk is small because the call wouldn't have completed
 * the client will keep retrying the write so when the server comes back up
 * the block will be rewritten correctly --  ghs 12/21/87.
 */

int
rwip(ip, uio, rw, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	dev_t dev;
	struct vnode *devvp;
	struct buf *bp;
	struct fs *fs;
	daddr_t lbn, bn;
	register int n, on, type;
	int size;
	long bsize;
	long bshift;
	long bmask;
#ifdef __hp9000s300
	extern int ieee802_no;
	extern int ethernet_no;
#endif
	int error = 0;
	int total;
	daddr_t lastblock;
	int sequential;
	int synch_write = 0;
	int iupdat_flag = 0;
	int checked_inode = 0;
	int save_uerror;
	daddr_t ra_block[MAXREADAHEAD];	/* read ahead blocks */
	int ra_size[3];			/* read ahead amount and sizes */
#ifdef MP
	sv_sema_t	sema_save;
#endif
#ifdef POSIX
	int new_n;
#endif /* POSIX */
#ifdef KPREEMPT
	int x;
#ifdef MP
	x = spl2();	/* spl/preemption: kernel not preemptible, I/O must
			 * be at spl2 */
#else
	x = splnopreempt();	/* spl/preemption: kernel not preemptible */
#endif
#endif /* KPREEMPT */


	dev = (dev_t)ip->i_rdev;
	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwip");
	if (rw == UIO_READ && uio->uio_resid == 0)
		return (0);

	/*
	 * check and see if we're doing synchronous writes.  If we're doing
	 * synchronous writes for NFS, syncio will be set to IO_SYNC.  If
	 * we're doing them because the file was opened with O_SYNCIO
	 * specified, we have to look in the u area.  This is dumb, we should
	 * have a single mechanism.  However....    ghs 2/2/88
	 */

	synch_write = ((ioflag & IO_SYNC) | (uio->uio_fpflags & FSYNCIO));
	if (synch_write)
		ip->i_flag |= IFRAGSYNC;
	type = ip->i_mode&IFMT;

	/* uio_offset can go negative for software drivers that open, read,
	   or write continuously, and never close */
	if ((uio->uio_offset < 0) || (uio->uio_offset + uio->uio_resid) < 0) {
		if (type != IFCHR && type != IFBLK)
			return(EINVAL);
#ifdef __hp9000s300
		else
			if (major(dev) == ethernet_no || major(dev) == ieee802_no)
				/* kludge!  how do we set f_offset to 0 ??? */
				uio->uio_offset = 0;
#endif /* hp9000s300 */

	}
	/* If the inode is remote, call the appropriate routine.  Note,
	 * We could completely separate out all DUX code from this
	 * routine by having a separate vnode entry, but it would mean
	 * duplicating all the preliminary tests.
	 */
	if ((type != IFCHR && type != IFBLK) && remoteip(ip))
		return (dux_rwip(ip, uio, rw, ioflag));
	if (rw == UIO_READ)
		imark(ip, IACC);
	switch (type) {
	case IFIFO:
		KPREEMPTPOINT();
		if (rw == UIO_READ)
			error = fifo_read(ip, uio);
		else
			error = fifo_write(ip, uio);
		KPREEMPTPOINT();
		return(error);
	case IFCHR:
		KPREEMPTPOINT();
		if (rw == UIO_READ) {
#ifdef MP
			p_io_sema(&sema_save);
#endif /* MP */
			error = (*cdevsw[major(dev)].d_read)(dev, uio);
#ifdef MP
			v_io_sema(&sema_save);
#endif /* MP */
		} else {
			imark(ip, IUPD|ICHG);
#ifdef MP
			p_io_sema(&sema_save);
#endif /* MP */
			error = (*cdevsw[major(dev)].d_write)(dev, uio);
#ifdef MP
			v_io_sema(&sema_save);
#endif /* MP */
		}
		KPREEMPTPOINT();
		return (error);

	case IFREG:
	case IFBLK:
	case IFDIR:
	case IFLNK:
	case IFNWK:
	if (uio->uio_resid == 0)
		return (0);
	if (type != IFBLK) {
		devvp = ip->i_devvp;
		fs = ip->i_fs;
		bsize = fs->fs_bsize;
		bmask = ~fs->fs_bmask;
		bshift = fs->fs_bshift;
	} else {
		devvp = devtovp(dev);
		bsize = BLKDEV_IOSIZE;
		bmask = BLKDEV_IOMASK;
		bshift = BLKDEV_IOSHIFT;
	}
	VASSERT(1<<bshift == bsize);
	VASSERT(bmask + 1 == bsize);
	u.u_error = 0;
	total = uio->uio_resid;
	if (rw == UIO_READ) {
	    daddr_t startblock = (u_long)uio->uio_offset >> bshift;
	    lastblock = ((u_long)uio->uio_offset + total - 1) >> bshift;
	    sequential = ip->i_lastr == -1 && uio->uio_offset == 0 || 
			 ip->i_lastr >=  0 && (startblock == ip->i_lastr || 
					       startblock == ip->i_lastr + 1);
	}
	do {
		lbn = (u_long)uio->uio_offset >> bshift;
		on = (u_long)uio->uio_offset & bmask;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);
		if (type != IFBLK) {
			if (rw == UIO_READ) {
				int diff = ip->i_size - uio->uio_offset;
				if (diff <= 0)
					return (0);
				if (diff < n)
					n = diff;
			}
			else if (rw == UIO_WRITE && type == IFREG) {
				if (((uio->uio_offset + n - 1) >> 9) >=
				u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
					n = ((u.u_rlimit[RLIMIT_FSIZE].rlim_cur << 9)
					- uio->uio_offset);
					if (n <= 0)
						if (total == uio->uio_resid)
							return(EFBIG);
						else {
							if (error == 0)
								error=u.u_error;
							return(error);
						}
				}
			}
			KPREEMPTPOINT();

			if (rw == UIO_READ) {
				setup_readahead(ip, lbn, ra_size, 
						sequential ? MAXREADAHEAD : 
							     lastblock - lbn);
			} else
				ra_size[0] = 0;
			bn =
			    fsbtodb(fs, bmap(ip, lbn,
				 rw == UIO_WRITE ? B_WRITE: B_READ,
				 (int)(on+n), (synch_write? &iupdat_flag : 0),
				 ra_block, ra_size));

			if (u.u_error || rw == UIO_WRITE && (long)bn<0)
#ifdef POSIX
			{
				if (u.u_error == ENOSPC && rw == UIO_WRITE &&
					type != IFREG) {
					/*
					 * In POSIX, we must write all the
					 * data we have space left for and
					 * not return an error unless we
					 * have no space at all left.  Since
					 * the smallest unit that can be
					 * allocated in the file system is
					 * a fragment, we will back off
					 * one fragment at a time from
					 * the amount we wanted to write
					 * until the bmap succeeds.
					 */
					new_n = n - fs->fs_fsize;
					while (new_n > 0) {
						u.u_error = 0;
						bn = fsbtodb(fs, bmap(ip, lbn,
						  B_WRITE, (int)(on+new_n),
						  (synch_write? &iupdat_flag:0),
						  ra_block, ra_size));


						if (!u.u_error && (long)bn >= 0)
						{
							/*
							 * Success! Change n
							 */
							n = new_n;
							break;
						} else {
							if (u.u_error==ENOSPC) {
							/*
							 * Ask for one less
							 * fragment.
							 */
								new_n -=
								  fs->fs_fsize;
							} else {
							/*
							 * new problem --
							 * leave loop.
							 */
								new_n = -1;
							}
						} /* if !u.u_error */
					} /* while new_n > 0 */
					if (new_n <= 0)
						/*
						 * really not enough space,
						 * or encountered another
						 * error
						 */
						return (u.u_error);
				} else {
					/*
					 * Not an ENOSPC problem
					 */
					return(u.u_error);
				} /* if u.u_error == ENOSPC */
			} /* if u.u_error */
#else
				return (u.u_error);
#endif /* POSIX */
			/* treat network special files like regular files
			 * for reading and writing
			 */
			if (rw == UIO_WRITE &&
			   (uio->uio_offset + n > ip->i_size) &&
			   (type == IFDIR || type == IFREG || type == IFLNK ||
			    type == IFNWK)) {
				ip->i_size = uio->uio_offset + n;
				iupdat_flag = 1;
			}

			size = blksize(fs, ip, lbn);
		} else {
			bn = lbn * (BLKDEV_IOSIZE/DEV_BSIZE);
			ra_block[0] = bn + (BLKDEV_IOSIZE/DEV_BSIZE);
			ra_size[0] = 1;
			ra_size[2] = size = bsize;
		}
		KPREEMPTPOINT();
		if (rw == UIO_READ) {
			if ((long)bn<0 && type != IFBLK) {
				bp = geteblk(size);
				clrbuf(bp);
			} else {
				bp = breadan(devvp, bn, size, ra_block, 
#ifdef	FSD_KI
						 ra_size, B_rwip|B_data);
#else	FSD_KI
						 ra_size);
#endif	FSD_KI
			}
			ip->i_lastr = lbn;
		} else {
			int i, count;

			count = howmany(size, DEV_BSIZE);
			for (i = 0; i < count; i += ptob(1)/DEV_BSIZE)
				munhash(devvp, (daddr_t)(bn + i));
			if (n == bsize)
#ifdef	FSD_KI
				bp = getblk(devvp, bn, size,
					B_rwip|B_data);
#else	FSD_KI
				bp = getblk(devvp, bn, size);
#endif	FSD_KI
			else
#ifdef	FSD_KI
				bp = bread(devvp, bn, size,
					B_rwip|B_data);
#else	FSD_KI
				bp = bread(devvp, bn, size);
#endif	FSD_KI
		}
		n = MIN(n, size - bp->b_resid - on);

		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
#ifdef KPREEMPT
		(void) splpreemptok();
#endif
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);
#ifdef KPREEMPT
#ifdef MP
		x = spl2();	/* spl/preemption: kernel not preemptible,
				 * I/O must be at spl2 */
#else
		(void) splnopreempt();
#endif
#endif

		/*
		 * Bug Fix.
		 * This makes us both Bell compatible and keeps
		 * rwip() from going into an infinite loop.
		 */
		 if (bp->b_resid)
			n = 0;
		if (rw == UIO_READ) {
			brelse(bp);
		} else {
			if ((synch_write) || (ip->i_mode&IFMT) == IFDIR
							 && (fs_async < 0))
				bwrite(bp);
			else if ((ip->i_mode&IFMT) == IFDIR && fs_async == 0)
				bxwrite(bp,0);
			else if (n + on == bsize && !(bp->b_flags & B_REWRITE)
						 && ip->i_nlink > 0) {
				bp->b_flags |= B_REWRITE;
				bawrite(bp);
			} else
				bdwrite(bp);
			/*
			 * Turn off ISUID, ISGID, ISVTX bits iff: not a
			 * directory, not enforcement mode locking, and
			 * not owner or root.
			 */
			if ((u.u_error == 0) && (checked_inode == 0)) {
				checked_inode = 1;
				if ((ip->i_mode & IFMT) != IFDIR) {
					if ((ip->i_mode & (ISUID|ISGID|ISVTX)) != 0) {
						if ((ip->i_mode & (ISUID|ISGID|(IEXEC>>3))) != ISGID) {
							save_uerror = u.u_error;
							if (OWNER_CR(u.u_cred, ip) != 0) {
								ip->i_mode &= ~(ISUID|ISGID|ISVTX);
							}
						     /* OWNER_CR may have called
						     suser(), which sets this.*/
							u.u_error = save_uerror;
						}
					}
				}
			}

			/*
			 * mark the inode to reflect the fact that the file
			 * has changed but let the syncer write it out
			 */
			imark(ip, IUPD|ICHG);
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);


	/*
	 * If synchronous i/o has been specified, we are writing and the
	 * inode has been changed, write the inode out synchronously.
	 * iupdat_flag will be true if bmap had to allocate new blocks or
	 * reallocate existing blocks to the file.
	 */
	if (synch_write && (rw == UIO_WRITE) && iupdat_flag)
		iupdat(ip, 1, 0);

	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
	break;

	default:
		u.u_error = ENODEV;
		return(u.u_error);
	}
bad:
	if (type == IFBLK)
		VN_RELE(devvp);
#ifdef	HPUXBOOT
	/*
	 * This is a bug fix. Since the disk driver in pre-release
	 * 11 did not return a 0 at end-of-media.  This fix allows
	 * hp-ux boot to detect the end-of-media gracefully.
	 * This code can be (although it will not have an effect if
	 * it is not) removed when the disk driver begins to return
	 * a 0 for reads at end-of-media.
	 * If we have managed to copy at least some data
	 * then don't return and error.  The next request
	 * will get an error.
	 */

	if (uio->uio_resid < total) {
		u.u_error = 0;
		return(0);
	}
#endif
	KPREEMPTPOINT();
#ifdef KPREEMPT
	splx(x);	    /* return to entry spl/preemption level */
#endif
	return (error);
}


/*ARGSUSED*/
int
ufs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	int error;
#ifdef MP
	sv_sema_t	sema_save;
#endif /* MP */
	register struct inode *ip;

	ip = VTOI(vp);
	if ((ip->i_mode & IFMT) != IFCHR)
		panic("ufs_ioctl");
#ifdef MP
 	p_io_sema(&sema_save);
#endif
 	error = (*cdevsw[major(ip->i_rdev)].d_ioctl)
 			(ip->i_rdev, com, data, flag);
#ifdef	MP
 	v_io_sema(&sema_save);
#endif
	return(error);
}

#ifdef __hp9000s300
net_select(fp, which)
	struct file *fp;
	int which;
{
	register struct vnode *vp = (struct vnode *)fp->f_data;
	register struct inode *ip = VTOI(vp);

	switch (vp->v_type) {
    default:
		return (1);		/* XXX */

    case VCHR:

		return
		    (*cdevsw[major(ip->i_device)].d_select)(ip->i_device,which,fp);
	}
}
#endif


/*ARGSUSED*/
int
ufs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
#ifdef MP
	sv_sema_t	sema_save;
#endif MP
	register struct inode *ip;
	int error;

	ip = VTOI(vp);
	switch (ip->i_mode & IFMT) {
	case IFIFO:
		if (ip->i_flag&ISYNC) {
			u.u_error = EINVAL;
			return(u.u_error);
		}
		switch(which) {
			case FREAD:
				if (((ip->i_fifosize > 0) &&
				     (ip->i_frcnt != 0)) ||
 				     (ip->i_fwcnt == 0))
					return(1);
				fselqueue(&ip->i_fselr);
				break;
			case FWRITE:
				if ((ip->i_fifosize < PIPSIZ) &&
				    (ip->i_frcnt != 0) &&
				    (ip->i_fwcnt != 0))
					return(1);
				fselqueue(&ip->i_fselw);
				break;
			}
			return(0);
	case IFCHR:
#ifdef MP
		p_io_sema(&sema_save);
#endif /* MP */
 		error = (*cdevsw[major(ip->i_rdev)].d_select)(ip->i_rdev, which);
#ifdef MP
		v_io_sema(&sema_save);
#endif /* MP */
		return error;
	default:
		u.u_error = EBADF;
		return(u.u_error);
	}
	/*NOTREACHED*/
}

/*ARGSUSED*/
int
ufs_getattr(vp, vap, cred, vsync)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
	enum vsync vsync;
{
	register struct inode *ip;
	sv_sema_t ss;

	ip = VTOI(vp);
	/*may need to synchronize inode to guarantee that the size and the
	 *number of blocks is correct
	 */
	if (vsync == VSYNC)
	{
		if( my_site_status & CCT_CLUSTERED ) {
			PXSEMA(&filesys_sema, &ss);
			isynclock(ip);
			notifysync(ip,1); /* flush buffers & ino times,
					     leave async */
			isyncunlock(ip);
			VXSEMA(&filesys_sema, &ss);
		}
	}
	/*
	 * Copy from inode table.
	 */
	vap->va_type = IFTOVT(ip->i_mode);
#ifndef ACLS
	vap->va_mode = ip->i_mode;
#else
	vap->va_mode = get_imode(ip);
	vap->va_basemode = ip->i_mode;
	if (ip->i_contin)
		vap->va_acl = 1;	/* there is an acl */
	else
		vap->va_acl = 0;	/* no acl */
#endif
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_fsid = ip->i_dev;
#if defined(__hp9000s800) && !defined(_WSIO)
	/*
	 * On a diskless client, map_mi_to_lu will be a no-op
	 * leaving va_fsid unchanged since the mi->lu mapping
	 * is not present for a remote device.  This is the desired
	 * behavior.
	 */
	(void)map_mi_to_lu(&vap->va_fsid, IFBLK);
#endif /* hp9000s800 && !_WSIO*/
	vap->va_nodeid = ip->i_number;
	vap->va_nlink = ip->i_nlink;
	if ((ip->i_mode & IFMT) == IFIFO)
		vap->va_size = ip->i_fifosize;
	else
	vap->va_size = ip->i_size;
	vap->va_atime = ip->i_atime;
	vap->va_mtime = ip->i_mtime;
	vap->va_ctime = ip->i_ctime;
	vap->va_rdev = ip->i_device;
#if defined(__hp9000s800) && !defined(_WSIO)
	if (vap->va_rdev == NODEV) {
		switch (ip->i_mode & IFMT) {
		case IFCHR:
		case IFBLK:
			map_mi_to_lu(&(vap->va_rdev), ip->i_mode & IFMT);
		}
	}
#endif	/* hp9000s800 && !_WSIO */
	vap->va_rsite = ip->i_rsite;
	vap->va_realdev = (dev_t)(remoteip(ip)
			  ? ((struct mount *)(vp->v_vfsp->vfs_data))->m_rdev
			  : vap->va_fsid);
	vap->va_fssite = my_site;
	vap->va_blocks = ip->i_blocks;
	vap->va_fstype = MOUNT_UFS;
	switch(ip->i_mode & IFMT) {

	case IFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
		break;

	case IFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = remoteip(ip) ? ip->i_dfs->dfs_bsize :
			ip->i_fs->fs_bsize;
		break;
	}
	return (0);
}

#ifdef ACLS

/* G E T _ I M O D E
 *
 * get the i_mode value for this inode, folding the acl if necessary.
 * For su return the mode bits with the user's execute bit set if
 * this is a regular file and none of the base tuples have the execute
 * bit set, but one is set in an optional tuple. For non-superusers
 * change whichever of (user, group, or other) apply to this process.
 */

get_imode(ip)
register struct inode *ip;
{
	if (ip->i_contin)			/* there are optional tuples */
	{
		if (u.u_uid == 0)		/* super-user */
		{
			if ((ip->i_mode&IFMT) == IFREG &&
			    (ip->i_mode & (X_OK|X_OK<<ACL_GROUP|X_OK<<ACL_USER)) == 0)
			{
				if (check_acl_X(ip))
					return ip->i_mode | (X_OK << ACL_USER);
			}
		}
		else
		{
			return setbasemode(ip->i_mode,iget_access(ip),
					base_match(ip->i_uid,ip->i_gid));
		}
	}
	return ip->i_mode;
}

/* B A S E _ M A T C H
 *
 * Does this user match owner, group or other?
 */

base_match(owner, group)
register int owner;
register int group;
{
	register int *gp, *gpmax;

	if (u.u_uid == owner)
		return ACL_USER;
	if (u.u_gid == group)
		return ACL_GROUP;
	gp = (int *) (u.u_groups);
	gpmax = (int *) (&u.u_groups[NGROUPS]);
	for (; *gp != NOGROUP && gp < gpmax; gp++)
		if (*gp == group)
			return ACL_GROUP;
	return ACL_OTHER;
}
#endif

int
/* KLUDGE!!  FIX IF POSSIBLE */
ufs_setattr(vp, vap, cred, null_time)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
	int null_time;
{
	register struct inode *ip;
	int chtime = 0;
	int error = 0;
	int shrinking = 0;
	register int was_enf = 0;
	register int is_enf  = 0;

	/*
	 * cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1) ||
	    ((short)vap->va_rsite != -1) || ((short)vap->va_fssite != -1) ||
	    ((int)vap->va_type != -1)) {
		return (EINVAL);
	}

 	use_dev_vp(vp);

	ip = VTOI(vp);
	ilock(ip);
	/*
	 * Change file access modes. Must be owner or su.
	 */
	if (vap->va_mode != (u_short)-1) {
		if (ip->i_number == ROOTINO && (vap->va_mode&VSUID)){
			error = EINVAL;	/* we don't support CDF root */
			goto out;
		}
		/* name conflict with DIL */
		error = OWNER_CR(cred, ip);
		if (error)
			goto out;

		if((ip->i_mode&IENFMT) && ((ip->i_mode&IFMT) == IFREG) &&
		   (ip->i_mode&(IEXEC>>3)) == 0)
			was_enf = 1;
		ip->i_mode &= IFMT;
		ip->i_mode |= vap->va_mode & ~IFMT;

		/*
		 * Enhancement for Defect DSDe405824
		 * Allow setting of ISVTX on directories for non super-user
		 */
		if (cred->cr_uid != 0) {
			if ((ip->i_mode&IFMT) != IFDIR)
				ip->i_mode &= ~ISVTX;
			if (!groupmember(ip->i_gid))
				ip->i_mode &= ~ISGID;
		}
		imark(ip, ICHG);
		if ((vp->v_flag & VTEXT) && ((ip->i_mode & ISVTX) == 0)) {
			xrele(ITOV(ip));
		}

		/* update all sites that have the file open */
		if( my_site_status & CCT_CLUSTERED )
			ino_update(ip);

		if((ip->i_mode&IENFMT) && ((ip->i_mode&IFMT) == IFREG) &&
		   (ip->i_mode&(IEXEC>>3)) == 0)
			is_enf = 1;

		/* check if sync. I/O is necessary */
		if(is_enf != was_enf) {
			iunlock(ip);
			isynclock(ip);
			checksync(ip);
			isyncunlock(ip);
			ilock(ip);
		}

#ifdef ACLS
		/* remove the optional tuples of the access control list
		 * when doing a chmod
		 */

		delete_all_tuples(ip);
#endif
	}

	/*
	 * Change file ownership. Must be su.
	 */
	if ( (vap->va_uid != (ushort)(UID_NO_CHANGE)) ||
	     (vap->va_gid != (ushort)(GID_NO_CHANGE)) ) {
		error = OWNER_CR(cred, ip);
		if (error)
			goto out;
		error = chown1(ip, vap->va_uid, vap->va_gid, cred);
		if (error)
			goto out;
	}
	/*
	 * Truncate file. Must have write permission and not be a directory.
	 */

	if (vap->va_size != (u_long)-1) {
		if (   (ip->i_locklist != NULL)
		    && ((ip->i_mode&IFMT) == IFREG)
		    && (locked(F_TEST,ip,0,MAX_LOCK_SIZE,L_COPEN,
			       NULL,0,0))) {
			error = u.u_error;
			goto out;
		}
		if (vap->va_size > MAX_LOCK_SIZE) {
			error = EINVAL;
			goto out;
		}
		if ((ip->i_mode & IFMT) == IFDIR) {
			error = EISDIR;
			goto out;
		}
		if (iaccess(ip, IWRITE)) {
			error = u.u_error;
			goto out;
		}

		shrinking = (vap->va_size < ip->i_size);
		/*May need to synchronize the inode */
		if ((ip->i_mode & IFMT) == IFREG)
		{
			iunlock(ip);
			isynclock(ip);
			setsync(ip);  /* ?why */
			ilock(ip);
			ip->i_fversion++;
			itrunc(ip, vap->va_size);
			iunlock(ip);	/* can't hold during checksync due  */
					/* to possible deadlock with client */
			checksync(ip);
			isyncunlock(ip);
			/*
			 * If the file is memory mapped and we have
			 * shrunk the file, call mtrunc() with the new
			 * (smaller) size to invalidate any pages past
			 * the end of the file.
			 */
			if (shrinking && (vp->v_flag & VMMF)) {
				mtrunc(vp, vap->va_size);
			}
			ilock(ip);
		}
		else
			itrunc(ip, vap->va_size);
	}
	/*
	 * Change file access or modified times.
	 */
	/* We can't just do a straight comparision of cred->cr_uid against
	 * ip->i_uid.  We also need to check the case where we are NFS,
	 * and network root (-2) and the inode is owned by "nobody"
	 * because i_uid is an ushort and -2 is stored as 65534.
	 */
	if ((vap->va_atime.tv_sec != -1) || (vap->va_mtime.tv_sec != -1)) {
		if (cred->cr_uid != ip->i_uid &&
		    !(cred->cr_uid == -2 && ip->i_uid == (ushort)-2) &&
		    cred->cr_uid != 0) {
			if (!null_time)
				error = EPERM;
			else {
				iaccess(ip, IWRITE);
				error = u.u_error;
			}
		}
		if (error)
			goto out;
		if (vap->va_atime.tv_sec != -1) {
			ip->i_atime = vap->va_atime;
		}
		if (vap->va_mtime.tv_sec != -1) {
			ip->i_mtime = vap->va_mtime;
		}
		chtime++;
	}
	if (chtime) {
		ip->i_flag |= IACC|IUPD|ICHG;
		ip->i_ctime = time;
		if (my_site_status & CCT_CLUSTERED) {
			iunlock(ip);
			isynclock(ip);
			notifysync(ip, 2);  /* cancel client ino update times */
			isyncunlock(ip);
			ilock(ip);
		}
	}
out:
	iupdat(ip, 0, shrinking ? 2 : 0);
	iunlock(ip);
 	drop_dev_vp(vp);
	bordend();
	return (error);
}

/*
 * Perform chown operation on inode ip;
 * inode must be locked prior to call.
 */
chown1(ip, uid, gid, cred)
	register struct inode *ip;
	u_short uid, gid;
	struct ucred *cred;
{
#ifdef QUOTA
	register long change;
#endif
#ifdef ACLS
	struct acl_tuple tuple;
#endif
				    /* NB that externally *ID_NO_CHANGE */
				    /* is a 32 bit quantity. */
	if ((uid >= MAXUID) && (uid != (u_short) (UID_NO_CHANGE)))
		return EINVAL;
	if ((gid >= MAXUID) && (gid != (u_short) (GID_NO_CHANGE)))
		return EINVAL;
	if (uid == (u_short) (UID_NO_CHANGE))
		uid = ip->i_uid;
	if (gid == (u_short) (GID_NO_CHANGE))
		gid = ip->i_gid;

	if ((u.u_uid != 0) &&	/* not super user */
	    (in_privgrp(PRIV_CHOWN, cred) == NULL) &&	/* not mem of chown privgrp */
	    ((uid != ip->i_uid) || (!groupmember(gid))) && /* not mem of grp
							      changing to */
	    ((uid != ip->i_uid) || (gid != ip->i_gid)))    /* not a no op */
			return(EPERM);

	/* see if inode is associated with a read only file system */
	if (ip->i_fs->fs_ronly) {
		return(EROFS);
	}

#ifdef QUOTA
	if (ip->i_uid == uid)		/* this just speeds things a little */
		change = 0;
	else
		change = ip->i_blocks;

	(void) chkdq(ip, -change, 0);
	(void) chkiq(VFSTOM(ip->i_vnode.v_vfsp), ip, ip->i_uid, 1);
	dqrele(ip->i_dquot);
	ip->i_dquot = NULL;
#endif QUOTA

#ifdef ACLS
	/* If we are setting a new owner of the file remove any optional (u,*)
	 * tuple from the access control list which would match the new
	 * owner. Similarly, if we are setting the group, remove any
	 * optional (*,g) tuple which matches the new group. If setting
	 * both, remove both.
	 */

	if (uid != ip->i_uid)
	{
		tuple.uid = uid;
		tuple.gid = ACL_NSGROUP;
		check_acl_tuple(ip,&tuple);
	}
	if (gid != ip->i_gid)
	{
		tuple.uid = ACL_NSUSER;
		tuple.gid = gid;
		check_acl_tuple(ip,&tuple);
	}
#endif
	ip->i_uid = uid;
	ip->i_gid = gid;
	imark(ip, ICHG);
	if ((u.u_uid != 0) && ((ip->i_mode&IFMT) == IFREG))
		ip->i_mode &= ~(ISUID|ISGID);
#ifdef QUOTA
	ip->i_dquot = getinoquota(ip);
	(void) chkdq(ip, change, 1);
	(void) chkiq(VFSTOM(ip->i_vnode.v_vfsp), (struct inode *)NULL, uid, 1);
#ifdef ACLS
	/* If this inode has a continuation inode make the call again to    */
	/* account for the continuation inode.				    */
	if (ip->i_contin)
	   (void) chkiq(VFSTOM(ip->i_vnode.v_vfsp),
			(struct inode *)NULL, uid, 1);
#endif ACLS
	/* update all sites that have the file open that owner has changed */
	if( my_site_status & CCT_CLUSTERED )
		ino_update(ip);
	return (u.u_error);		/* should == 0 ALWAYS !! */
#else not QUOTA
	return (0);
#endif QUOTA
}

/*ARGSUSED*/
int
ufs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;
	sv_sema_t SS;

	ip = VTOI(vp);

	/* If the serving site has died, references to inode fields
	 * which were cleaned up by ino_cleanup may result in
	 * segmentation violations, so return an error.   mry 1/6/91
	 */
	if (remoteip(ip) && (devsite(ip->i_dev) == 0))
		return (EIO);

	PXSEMA(&filesys_sema, &SS);
	ilock(ip);
	error = iaccess(ip, mode);
	iunlock(ip);
	VXSEMA(&filesys_sema, &SS);
	return (error);
}

/*ARGSUSED*/
int
ufs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	register struct inode *ip;
	register int error;
	extern int hpux_aes_override;

	if (vp->v_type != VLNK)
		return (EINVAL);
	ip = VTOI(vp);
	ilock(ip);
	if ((ip->i_size > uiop->uio_resid) && (!hpux_aes_override)){
	   /* check user buffer size */
		error = ERANGE;
		goto out;
	}
	if (ip->i_flags & IC_FASTLINK) {
		error = uiomove(ip->i_symlink, ip->i_size, UIO_READ, uiop);
		imark(ip, IACC);
	}
	else {
		error = rwip(ip, uiop, UIO_READ, 0);
	}
out:
	iunlock(ip);
	return (error);
}

/*ARGSUSED*/
int
ufs_fsync(vp, cred, ino_chg)
	struct vnode *vp;
	struct ucred *cred;
	int ino_chg;
{
	register struct inode *ip;

	ip = VTOI(vp);
	ilock(ip);
	syncip(ip, ino_chg, 1);
	ip->i_flag |= IFRAGSYNC;
	iunlock(ip);
	bordend();
	return (0);
}

/*ARGSUSED*/
int
ufs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
#ifdef	NFS_DISKLESS
	struct vnode temp;


	temp = *vp;
#endif	NFS_DISKLESS
	/* hold vnode since we may sleep in use_dev_vp */
	VN_HOLD(vp);

	use_dev_vp(vp);

	/*
	 * if anyone grabbed the vnode, don't deactivate it
	 * (note - don't call VN_RELE, since that would
	 * recurse indefinitely.)
	 */
	SPINLOCK(v_count_lock);
	--vp->v_count;
	SPINUNLOCK(v_count_lock);

#ifdef	NFS_DISKLESS
#define PIPE_DEV_MAGIC 0x51964151
#endif	NFS_DISKLESS

	if (vp->v_count == 0)
#ifdef	NFS_DISKLESS
	    if (VTOI(vp)->i_devvp == devtovp(PIPE_DEV_MAGIC))
		    kfree(VTOI(vp),M_TEMP);
	    else
		    iinactive(VTOI(vp));
#else	NFS_DISKLESS
	    iinactive(VTOI(vp));
#endif	NFS_DISKLESS

#ifdef	NFS_DISKLESS
	drop_dev_vp(&temp);
#else	NFS_DISKLESS
	drop_dev_vp(vp);
#endif	NFS_DISKLESS
	bordend();
	return (0);
}

/*
 * Unix file system operations having to do with directory manipulation.
 */
/*ARGSUSED*/
ufs_lookup(dvp, nm, vpp, cred, mvp)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct vnode *mvp;
{
	struct inode *ip;
	register int error;

 	error = sdo_lookup(VTOI(dvp), nm, &ip, mvp, 0, 0);
	if (error == 0) {
		*vpp = ITOV(ip);
		iunlock(ip);
	}

	return (error);
}

ufs_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	register int error;
	struct inode *ip;

	/*
	 * can't create directories. use ufs_mkdir.
	 */
	if (vap->va_type == VDIR)
		return (EISDIR);

 	use_dev_vp(dvp);

	ip = (struct inode *) 0;
	error = direnter(VTOI(dvp), nm, DE_CREATE,
		(struct inode *)0, (struct inode *)0, vap, &ip);
	/*
	 * if file exists and this is a nonexclusive create,
	 * check for not directory and access permissions
	 * If create/read-only an existing directory, allow it.
	 */
	if (error == EEXIST) {
		if (exclusive == NONEXCL) {
			if ((ip->i_mode & IFMT) == IFDIR && (mode & IWRITE)) {
				error = EISDIR;
			} else if (mode) {
				error = iaccess(ip, mode);
			} else {
				error = 0;
			}
		}
		if (error) {
			iput(ip);
		} else if ((ip->i_mode & IFMT) == IFREG && vap->va_size == 0) {
			/*
			 * truncate regular files, if requried
			 */

			/* check for an enforcement mode lock */
			if (   ip->i_locklist != NULL
			    && (ip->i_mode&IFMT) == IFREG
			    && locked(F_TEST,ip,0,(long)(1L<<30),L_COPEN,
				       NULL,0,0)) {
				/* ?iput(ip); */
				iunlock(ip);
 				drop_dev_vp(dvp);
				return(u.u_error);
			}

			/*synchronize*/
			iunlock(ip);	/*to get sync lock*/
			isynclock(ip);
			setsync(ip);
			ilock(ip);
			ip->i_fversion++;
			itrunc(ip, (u_long)0);
			iunlock(ip);	/* can't hold during checksync due  */
					/* to possible deadlock with client */
			checksync(ip);
			isyncunlock(ip);
			/*
			 * If the file is memory mapped, call mtrunc()
			 * with the new (0) size to invalidate any pages
			 * past the end of the file (which in this case
			 * is all pages).
			 */
			if (ITOV(ip)->v_flag & VMMF) {
				mtrunc(ITOV(ip), (u_long)0);
			}
			ilock(ip);
		}
	}


	if (error) {
		goto out;
	}
	*vpp = ITOV(ip);
	iunlock(ip);
	if (vap != (struct vattr *)0) {
		(void) ufs_getattr(*vpp, vap, cred, VASYNC);
	}
out:
	drop_dev_vp(dvp);
	bordend();
	return (error);
}

/*ARGSUSED*/
ufs_remove(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register int error;
	use_dev_vp(vp);

	error = dirremove(VTOI(vp), nm, (struct inode *)0, 0);

	drop_dev_vp(vp);
	bordend();
	return (error);
}

/*
 * link a file or a directory
 * If source is a directory, must be superuser
 */
/*ARGSUSED*/
ufs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	register struct inode *sip;
	register int error;

	sip = VTOI(vp);
	if (((sip->i_mode & IFMT) == IFDIR) && !suser()) {
		return (EPERM);
	}

	use_dev_vp(vp);

 	/* this is for System V compatibility */
  	if (sip->i_nlink >= MAXLINK) {
 		error = EMLINK;
 		goto out;
 	}
	error =
	    direnter(VTOI(tdvp), tnm, DE_LINK,
		(struct inode *)0, sip, (struct vattr *)0, (struct inode **)0);

out:
	drop_dev_vp(vp);
	bordend();
	return (error);
}

/*
 * Rename a file or directory
 * We are given the vnode and entry string of the source and the
 * vnode and entry string of the place we want to move the source to
 * (the target). The essential operation is:
 *	unlink(target);
 *	link(source, target);
 *	unlink(source);
 * but "atomically". Can't do full commit without saving state in the inode
 * on disk, which isn't feasible at this time. Best we can do is always
 * guarantee that the TARGET exists.
 */
/*ARGSUSED*/
ufs_rename(sdvp, snm, tdvp, tnm, cred)
	struct vnode *sdvp;		/* old (source) parent vnode */
	char *snm;			/* old (source) entry name */
	struct vnode *tdvp;		/* new (target) parent vnode */
	char *tnm;			/* new (target) entry name */
	struct ucred *cred;
{
	struct inode *sip;		/* source inode */
	register struct inode *sdp;	/* old (source) parent inode */
	register struct inode *tdp;	/* new (target) parent inode */
	register int error;


	struct	inode *ldp = 0;
	char	realname[MAXNAMLEN+1];

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);
	/*
	 * make sure we can delete the source entry
	 */
	error = iaccess(sdp, IWRITE);
	if (error) {
		return (error);
	}

	use_dev_vp(sdvp);

	/*
	 * look up inode of file we're supposed to rename.
	 */
	/* pathname lookup is done once already */
	error = sdo_lookup(sdp, snm, &sip, sdvp, &ldp, realname);
	if (error) {
		drop_dev_vp(sdvp);
		return(error);
	}

	iunlock(sip);			/* unlock inode (it's held) */

	/*
	 * Enhancement for Defect DSDe405824
	 * If source directory has ISVTX set and not super user
	 * allow rename only when the source file/directory is
	 * owned by the current process's uid.
	 */
	if ((sdp->i_mode & ISVTX) && u.u_uid != 0) {
		if (u.u_uid != sip->i_uid && u.u_uid != sdp->i_uid) {
			error = EPERM;
			goto out;
		}
	}

	/*
	 * check for renaming '.' or '..' or alias of '.'
	 */
	if ((strcmp(snm, ".") == 0) || (strcmp(snm, "..") == 0) ||
	    (sdp == sip)) {
		error = EINVAL;
		goto out;
	}
	/*
	 * link source to the target
	 */
	error =
	    direnter(tdp, tnm, DE_RENAME,
		(ldp? ldp: sdp), sip, (struct vattr *)0, (struct inode **)0);
	if (error) {
		goto out;
	}

	/*
	 * Unlink the source
	 * Remove the source entry. Dirremove checks that the entry
	 * still reflects sip, and returns an error if it doesn't.
	 * If the entry has changed just forget about it.
	 * Release the source inode.
	 */
	error = dirremove((ldp? ldp: sdp), (ldp ? realname : snm), sip, 0);
	if (error == ENOENT) {
		error = 0;
	} else if (error) {
		goto out;
	}

out:    /* if we've already done the work and it turns out */
	/* the source and target are the same, just return */
	if (error == ESAME) {
		error = 0;
	}
	if(ldp)  VN_RELE(ITOV(ldp));
	VN_RELE(ITOV(sip));

	drop_dev_vp(sdvp);
	bordend();
	return (error);
}

/*ARGSUSED*/
ufs_mkdir(dvp, nm, vap, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *vap;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct inode *ip;
	register int error;

	use_dev_vp(dvp);
	error =
	    direnter(VTOI(dvp), nm, DE_CREATE,
		(struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0) {
		*vpp = ITOV(ip);
		iunlock(ip);
	} else if (error == EEXIST) {
		iput(ip);
	}

	drop_dev_vp(dvp);
	bordend();
	return (error);
}

/*ARGSUSED*/
ufs_rmdir(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register int error;

	use_dev_vp(vp);

	error = dirremove(VTOI(vp), nm, (struct inode *)0, 1);

	drop_dev_vp(vp);
	bordend();
	return (error);
}


#define	roundtoint(x)	(((x) + (sizeof(int) - 1)) & ~(sizeof(int) - 1))
#define	reclen(dp)	roundtoint(((dp)->d_namlen + 1 + sizeof(u_long) +\
				2 * sizeof(u_short)))

/*ARGSUSED*/
ufs_readdir(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	struct inode *ip;
	off_t offset;
	struct iovec t_iovec;
	struct uio t_uio;
	caddr_t readbuf;
	int readlen;
	int bytes_read, bytes2return;
	caddr_t first_new_dp;
	int error = 0;
	caddr_t kmem_alloc();
	struct direct *dp, *lastdp;

	ip = VTOI(vp);
	if ((ip->i_mode&IFMT) != IFDIR)
		return EINVAL;
	if (uiop->uio_offset >= ip->i_size)
		return 0;	/* EOF */
	if (uiop->uio_iovcnt != 1)
		panic("ufs_readdir: bad iovcnt");
	if ((int)uiop->uio_resid < (int)sizeof (struct direct))
		return EINVAL;

	/*
	**	Normal callers - blocks on block boundaries.
	**	i.e. offset % DIRBLKSIZ == 0 and
	**	resid % DIRBLKSIZ == 0.
	*/
	if ((uiop->uio_offset & (DIRBLKSIZ-1)) == 0
	&& (uiop->uio_resid & (DIRBLKSIZ-1)) == 0){
		ilock(ip);
		error = rwip(ip, uiop, UIO_READ, 0);
		iunlock(ip);
		return error;
	}

	/*	Refer to comments in getdirentries() to see why we
	**	need to be able to accept any offset and size.
	**	Read in all directory blocks which contain
	**	the desired span.
	**	Skip all entries which start before uiop->uio_offset.
	**	Since the user is free to set the file offset, or
	**	somebody may have added or deleted entries, the
	**	first entry returned may lie anywhere past the initial
	**	offset.
	**	Also, when an empty UFS directory is created, the
	**	directory is only 24 or so bytes long, but the last
	**	d_reclen points to byte 1025. This "works", too.
	*/
	offset = uiop->uio_offset & ~(DIRBLKSIZ-1);	/* round down */
	readlen = ((uiop->uio_offset & (DIRBLKSIZ-1))	/* round up */
			+ uiop->uio_resid + (DIRBLKSIZ-1)) & ~(DIRBLKSIZ-1);
	readbuf = kmem_alloc(readlen);
	t_uio.uio_seg = UIOSEG_KERNEL;
	t_uio.uio_fpflags = 0;
	t_uio.uio_iov = &t_iovec;
	t_uio.uio_iovcnt = 1;

nextblk:
	t_uio.uio_offset = offset;
	t_uio.uio_resid = t_iovec.iov_len = readlen;
	t_iovec.iov_base = readbuf;
	ilock(ip);
	error = rwip(ip, &t_uio, UIO_READ, 0);
	iunlock(ip);
	if (error)
		goto out;
	bytes_read = readlen - t_uio.uio_resid;
	lastdp = dp = (struct direct *)readbuf;
	first_new_dp = (caddr_t) 0;

	/*
	**	Skip entries before uio_offset and null entries.
	*/
	while ((caddr_t)dp < (readbuf + bytes_read)
	&& (offset < uiop->uio_offset || dp->d_ino == 0)){
		/*
		 * If the directory entry is corrupted, we could have an
		 * infinite loop or a misaligned data reference through dp.
		 * Check it here and return EINVAL to be consistent with cdfs.
		 */
		if ((dp->d_reclen == 0) || (dp->d_reclen & 03)) goto error_out;

		offset += dp->d_reclen;
		dp = (struct direct *)((caddr_t)dp + dp->d_reclen);
	}
	/*
	**	Because we rounded up we have no partial entries.
	*/
	if ((caddr_t)dp < (readbuf + bytes_read))
		first_new_dp = (caddr_t) dp;
	else if (offset < ip->i_size)
		goto nextblk;
	/* else EOF */

	uiop->uio_offset = offset;
	/*
	**	See how much we can return.
	*/
	while ((caddr_t)dp < (readbuf + bytes_read)
	&& ((caddr_t)dp - first_new_dp + reclen(dp)) <= uiop->uio_resid){
		/*
		 * If the directory entry is corrupted, we could have an
		 * infinite loop or a misaligned data reference through dp.
		 * Check it here and return EINVAL to be consistent with cdfs.
		 */
		if ((dp->d_reclen == 0) || (dp->d_reclen & 03)) goto error_out;

		lastdp = dp;
		dp = (struct direct *)((caddr_t)dp + dp->d_reclen);
	}
	if (first_new_dp){
		/*
		**	d_reclen may have been as big as 1024.
		**	We'll lie to get something that will fit.
		**	Since the right offset is returned, they'll leap over
		**	the empty gap on their next call.
		*/
		lastdp->d_reclen = reclen(lastdp);
		bytes2return = (caddr_t) lastdp + lastdp->d_reclen
				- first_new_dp;
		error = uiomove(first_new_dp, bytes2return, UIO_READ, uiop);
	} /* else EOF */
out:
	kmem_free(readbuf, readlen);
	return error;

	/*
	 * We come here if we found a bad directory entry.  Disk needs to be
	 * unmounted and fsck'd to fix the directory.
	 */
error_out:
	printf("\n%s: ufs_readdir: bad dir ino %d: mangled entry.\n",
			 ip->i_fs->fs_fsmnt, ip->i_number);
	error = EINVAL;
	goto out;
}
/*
 * create_fastlinks --
 *    If this variable is non-zero, fast symlinks will be created when
 *    possible.  If this variable is zero, only "normal" symlinks will
 *    be created.  This variable is defined in machine/space.h and can
 *    be configured using config(1m).
 */
extern int create_fastlinks;

/*ARGSUSED*/
ufs_symlink(dvp, lnm, vap, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *vap;
	char *tnm;
	struct ucred *cred;
{
	struct inode *ip;
	register int error;

	use_dev_vp(dvp);
	ip = (struct inode *) 0;
	vap->va_type = VLNK;
	vap->va_rdev = 0;

	error = direnter(VTOI(dvp), lnm, DE_CREATE,
			 (struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0) {
		int len = strlen(tnm);

		/*
		 * Fast symlink support.  If the length of this
		 * symlink is such that it will fit into the inode,
		 * put it there to save space and time.
		 */
		if (len < MAX_FASTLINK_SIZE && create_fastlinks) {
			/*
			 * Path is short enough, copy to the inode in
			 * the ip->i_symlink field.  We use len+1 so
			 * that the trailing '\0' byte is included.
			 */
			bcopy(tnm, ip->i_symlink, len+1);
			ip->i_size = len;
			ip->i_flags |= IC_FASTLINK;
			imark(ip, IACC|IUPD|ICHG);
			iput(ip);
		}
		else {
			error = rdwri(UIO_WRITE, ip, tnm, len, 0,
				      UIOSEG_KERNEL, (int *)0);
			if (error != 0) {
			    /* 
			     * We need to clear u.u_error because dirremove() 
			     * trickles down to dircheckforname() which returns 
			     * u.u_error and if it is != 0 the removal of the
			     * entry in the directory is not completed.
			     */
			    int save_error = u.u_error;

			    u.u_error = 0; 
			    iunlock(ip);
			    dirremove(VTOI(dvp), lnm, ip, 0);
			    irele(ip);
			    u.u_error = save_error;
			}
			else
			    iput(ip);
		}
	} else if (error == EEXIST) {
		iput(ip);
	}

	drop_dev_vp(dvp);
	bordend();
	return (error);
}

rdwri(rw, ip, base, len, offset, seg, aresid)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int *aresid;
{
	struct uio auio;
	struct iovec aiov;
	register int error;

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_seg = seg;
	auio.uio_resid = len;
	auio.uio_fpflags = 0;

	error = ufs_rdwr(ITOV(ip), &auio, rw, 0, u.u_cred);
	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
#ifdef QUOTA
	     if (error != EDQUOT)
#endif QUOTA
		error = EIO;
	}
	return (error);
}

/*
 *This function is the same as rdwri except that it calls rwip
 *directly without locking the inode.  It is intended to be called
 *if it is necessary to to read from an inode that is already locked
 *Currently, the only routine that calls it is dirempty.
 */
rdwri_nolock(rw, ip, base, len, offset, seg, aresid)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int *aresid;
{
	struct uio auio;
	struct iovec aiov;
	register int error;

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_seg = seg;
	auio.uio_resid = len;
	auio.uio_fpflags = 0;
	error = rwip(ip, &auio, rw, 0);
	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
		error = EIO;
	}
	return (error);
}

int
ufs_bmap(vp, lbn, vpp, bnp)
	struct vnode *vp;
	daddr_t lbn;
	struct vnode **vpp;
	daddr_t *bnp;
{
	register struct inode *ip;

	ip = VTOI(vp);
	if (vpp)
		*vpp = ip->i_devvp;
	if (bnp)
		*bnp = fsbtodb(ip->i_fs, bmap(ip, lbn, B_READ, 0, (int *)0, 
						(daddr_t *)0, (int *)0));
	return (0);
}

/*
 * read a logical block and return it in a buffer
 */
int
ufs_bread(vp, lbn, bpp)
	struct vnode *vp;
	daddr_t lbn;
	struct buf **bpp;
{
	register struct inode *ip;
	register struct buf *bp;
	register daddr_t bn;
	register int size;
	daddr_t ra_block[MAXREADAHEAD];
	int ra_size[3];

	ip = VTOI(vp);
	size = blksize(ip->i_fs, ip, lbn);
	if (ip->i_lastr + 1 == lbn)
		setup_readahead(ip, lbn, ra_size, MAXREADAHEAD);
	else
		ra_size[0] = 0;
	bn = fsbtodb(ip->i_fs, bmap(ip, lbn, B_READ, 0, (int *)0, 
						ra_block, ra_size));
	if ((long)bn < 0) {
		bp = geteblk(size);
		clrbuf(bp);
	} else {
#ifdef	FSD_KI
		bp = breadan(ip->i_devvp, bn, size, ra_block, ra_size,
			     B_ufs_bread|B_unknown);
#else	FSD_KI
		bp = breadan(ip->i_devvp, bn, size, ra_block, ra_size);
#endif	FSD_KI
	}
	ip->i_lastr = lbn;
	imark(ip, IACC);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	} else {
		*bpp = bp;
		return (0);
	}
}

/*
 * release a block returned by ufs_bread
 */
/*ARGSUSED*/
ufs_brelse(vp, bp)
	struct vnode *vp;
	struct buf *bp;
{
	bp->b_resid = 0;
	brelse(bp);
}

int
ufs_badop()
{
	panic("ufs_badop");
}

/*
 * release_cdir
 *
 * this function releases the current working directory.
 * It works for either a local or a remote working directory.
 *
 * It is called from chdir and from exit.
 *
 * input
 * -----
 *
 *	u.u_rfa.ur_rcdir
 *	u.u_cdir
 *
 *		If the current working dir is local, then ur_rcdir
 *		will be NULL and u_cdir will point to the inode for the
 *		current working dir.
 *
 *		If the current working dir is remote, then u_cdir will
 *		be NULL, and ur_rcdir will point to the rfa_inode
 *		structure for the current working dir.
 *
 *		If both of the above pointers are NULL, the process does
 *		not have a working dir.
 *
 *		NOTE:  u.u_cdir should NOT be locked when this is called.
 *		(find out why exit locks the inode, then calls iput which
 *		immediately unlocks it).  (It appears that this is true, so
 *		exit will not lock the inode and release_cdir will not check
 *		for a locked inode -- 8/29/84).
 *
 * output
 * ------
 *
 *	u.u_rfa.ur_rcdir
 *	u.u_cdir
 *
 *		Both of these pointers should be NULL when this function
 *		returns.  If one of these was non-NULL when this function
 *		was called, the inode or rfa_inode that it pointed to will
 *		have been de-allocated appropriately.
 *
 * return value
 * ------------
 *
 *	none.
 *
 * algorithm
 * ---------
 *
 *	(As a QA check, the u_cdir and ur_rcdir pointers should
 *	not both be NULL)
 *
 *	if (u_cdir is not NULL) {
 *		call irele to de-allocate the inode
 *		set u_cdir to NULL
 *		(As a QA check, u_rcdir should already be NULL)
 *	}
 *
 *	if (ur_rcdir is not NULL) {
 *		call rfa_close_cdir to close the remote working dir.
 *
 *		this is a separate routine so that the kernel can
 *		be configurable. If networking is not installed,
 *		the rfa_close_cdir routine will not be loaded into
 *		the kernel.
 *	}
 */

release_cdir()
{
	register struct vnode *vp;

	if ((vp = u.u_cdir) != NULL) {
		update_duxref(vp,-1,0);
		VN_RELE(vp);
		u.u_cdir = NULL;
	}
}


#ifdef ACLS
/*
 * U F S _ S E T A C L
 *
 * This is the vnodeop for the ufs case
 * This is the routine which does the actual work for the system call
 */

ufs_setacl(vp,ntuples,tupleset)
register struct vnode * vp;
int ntuples;
struct acl_tuple *tupleset;
{
	struct inode *ip;
	register struct inode *cip;
	register struct acl_tuple *tp;
	register int i, imode;
	register int nopttuples=0;
	register int chkbase;
	struct vattr vattr;
	struct acl_tuple acl[NOPTTUPLES];

	/* If you are attempting to do a setacl on a file on a read-only
	 * file system set errno to EROFS
	 */

	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
	{
		u.u_error = EROFS;
		return;
	}

	ip = VTOI(vp);

	/*
	 * Must be the owner or su in order to do a setacl
	 * OWNER_CR sets u.u_error and returns that value
	 */

	if (OWNER_CR(u.u_cred,ip))
		return;

	/* As a special case, if the number of optional tuples to set is
	 * less than zero we will delete all the optional tuples and return
	 */

	if (ntuples < 0)
	{
		use_dev_vp(vp);
		delete_all_tuples(ip);
		drop_dev_vp(vp);
		return;
	}

	/* We must know if we are going to get an error doing this
	 * operation before we can change the inode itself. Therefore
	 * we set temporary locations first. We copy the tuples one by
	 * one in order to sort them.
	 */

	imode = 0;
	chkbase = 0;
	init_acl(acl,NOPTTUPLES);

	for (i=0, tp=tupleset; i<ntuples; i++, tp++)
	{

		/* does this tuple have valid values? */

		if ((tp->uid >= MAXUID && tp->uid != ACL_NSUSER) ||
		    (tp->gid >= MAXUID && tp->gid != ACL_NSGROUP) ||
		    (tp->mode > 7))
		{
			u.u_error = EINVAL;
			return;
		}

		/* Is this a base tuple? If so set the mode bits in imode */

		if (tp->uid == ACL_NSUSER)
		{
			if (tp->gid == ACL_NSGROUP)
			{
				/* has it been set already? */

				if (chkbase & (1<<ACL_OTHER))
				{
					u.u_error = EINVAL;
					return;
				}
				chkbase |= 1<<ACL_OTHER;

				/* set the "other" base mode bits */

				imode = setbasemode(imode,tp->mode,ACL_OTHER);
				continue;
			}
			if (tp->gid == ip->i_gid)
			{
				if (chkbase & (1<<ACL_GROUP))
				{
					u.u_error = EINVAL;
					return;
				}
				chkbase |= 1<<ACL_GROUP;

				/* set the "group" base mode bits */

				imode = setbasemode(imode,tp->mode,ACL_GROUP);
				continue;
			}
		}
		else
		{
			if (tp->gid == ACL_NSGROUP && tp->uid == ip->i_uid)
			{
				if (chkbase & (1<<ACL_USER))
				{
					u.u_error = EINVAL;
					return;
				}
				chkbase |= 1<<ACL_USER;

				/* set the "user" base mode bits */

				imode = setbasemode(imode,tp->mode,ACL_USER);
				continue;
			}
		}

		/* We have an optional tuple */

		nopttuples++;
		insert_tuple(acl,tp->uid,tp->gid,tp->mode,NOPTTUPLES);
		if (u.u_error)
			return;
	}

	/*
	 * We are setting the 'extra' mode bits to be what they
	 * were before
	 */

	imode |= ip->i_mode & 07000;

	/* this is the way we set the mode bits. As a side effect of
	 * ufs_setattr the optional tuples are removed. If there are
	 * no optional tuples we can return after setting the mode bits.
	 * ufs_setattr will lock the inode itself, so we don't lock until
	 * after we return.
	 */

	vattr_null(&vattr);
	vattr.va_mode = imode;
	ufs_setattr(ITOV(ip),&vattr,u.u_cred,0);
	if (u.u_error)				/* should never happen */
		return;

	if (nopttuples == 0)
		return;

	use_dev_vp(vp);
	ilock(ip);

	/* The continuation inode number will be zero since it was
	 * just freed by the ufs_setattr call. So we need to
	 * get one to store the acl in. If ialloc failed the error
	 * has been placed in u.u_error, so we simply return.
	 * If we succeeded, we need to initialize the continuation
	 * inode, and set up the primary inode to point to it.
	 * The errors that can be generated by the ialloc call
	 * are ENFILE and ENOSPC.
	 */

	cip = ialloc(ip,ip->i_number,IFCONT);
	if (cip == NULL)
	{
		iunlock(ip);
		drop_dev_vp(vp);
		return;
	}

	ip->i_contin = cip->i_number;
	ip->i_contip = cip;

	cip->i_mode = IFCONT;

	/* mark it as referenced, so it will be kept in core
	 * set nlink to 1 so the inode isn't returned.
	 */

	cip->i_flag |= IREF;
	cip->i_nlink = 1;

	/* Initialize the continuation inode's acl and copy the tuples
	 * set in the temporary acl
	 */

	init_acl(cip->i_acl,NOPTTUPLES);
	for(i=0; i<nopttuples; i++)
		cip->i_acl[i] = acl[i];

	/* mark the primary inode as changed; update ctime */

	imark(ip,ICHG);

	/* we must mark continuation inode as changed in order to have it
	 * flushed out to disk. Can't call imark though since that would
	 * modify ctime (and ctime doesn't exist on a continuation inode).
	 */

	cip->i_flag |= ICHG;

	iunlock(cip);
	iunlock(ip);
	drop_dev_vp(vp);
	return;
}

/*
 * U F S _ G E T A C L
 *
 * This is the vnodeop for the ufs case (local file sys and RFA)
 * We call vno_getacl to copy the acl into a local variable
 * and then call copyout to copy it out to user land (if requested)
 */

ufs_getacl(vp, ntuples, tupleset)
struct vnode *vp;
int ntuples;
struct acl_tuple_user *tupleset;
{
	int count;

	struct acl_tuple acl[NACLTUPLES];
	struct acl_tuple_user acl_user[NACLTUPLES];
	register int i;
	register struct acl_tuple *tp;
	register struct acl_tuple_user *tpu;

	count = vno_getacl(vp, ntuples, acl);
	if (u.u_error)
		return;

	if (ntuples)
	{
		for (i=0, tp=acl, tpu=acl_user; i<ntuples; i++, tp++, tpu++) {
			tpu->uid = tp->uid;
			tpu->gid = tp->gid;
			tpu->mode = tp->mode;
		}
		u.u_error = copyout(acl_user, tupleset, ntuples *
				sizeof (struct acl_tuple_user));
		if (u.u_error)
			return;
	}
	u.u_r.r_val1 = count;
}
#endif

#ifdef POSIX
/*
 * _POSIX_VDISABLE used to be in h/unistd.h but we've moved it here
 * because we did not want to make it visible to the user and create
 * possible compatibility problems in the future.  Watch out for
 * duplicate definition in h/tty.h.
 * HP REVISIT.  Remove this when tty.h is fixed.
 */
#undef _POSIX_VDISABLE
#define _POSIX_VDISABLE         ((unsigned char)'\377')

/*ARGSUSED*/
ufs_pathconf(vp, name, resultp, cred)
struct vnode *vp;
int	name;
int	*resultp;
struct ucred *cred;
{
	extern struct privgrp_map priv_global;
	enum vtype tp = vp->v_type;
#if defined(__hp9000s300) && !defined(POSIX_SET_NOTRUNC)
	struct inode *ip = VTOI(vp);
#endif /* hp9000s300 && !POSIX_SET_NOTRUNC */

	switch(name) {
	case _PC_LINK_MAX:
		*resultp = MAXLINK;
		break;
	case _PC_MAX_CANON:
#ifdef __hp9000s300
		*resultp = CANBSIZ;
#else  hp9000s800
		*resultp = TTYHOG;
#endif
		break;
	case _PC_MAX_INPUT:
		*resultp = TTYHOG;	/*for tty*/
		break;
	case _PC_NAME_MAX:
		if (tp != VDIR)
			return(EINVAL);
		*resultp = (IS_LFN_FS(VTOI(vp))) ? MAXNAMLEN : DIRSIZ_CONSTANT;
		break;
	case _PC_PATH_MAX:
		if (tp != VDIR)
			return(EINVAL);
		*resultp = (MAXPATHLEN - 1);
		break;
	case _PC_PIPE_BUF:
		if ((tp != VDIR) && (tp != VFIFO))
			return(EINVAL);
		*resultp = PIPSIZ;
		break;
	case _PC_CHOWN_RESTRICTED:
		if (wisset(priv_global.priv_mask, PRIV_CHOWN-1))
			*resultp = -1; /* chown is not restricted */
		else
			*resultp = 1; /* it is restricted */
		break;
	case _PC_NO_TRUNC:
		if (tp != VDIR)
			return(EINVAL);
#ifdef __hp9000s800
		if (IS_LFN_FS(VTOI(vp))
#ifdef S2POSIX_NO_TRUNC
		    || (u.u_procp->p_flag2 & S2POSIX_NO_TRUNC)
#endif
		    )
			*resultp = 1;
		else
			*resultp = -1;
#else /* hp9000s300 */
#ifdef POSIX_SET_NOTRUNC
		*resultp = (u.u_procp->p_flag2 & POSIX_NO_TRUNC?1:-1);
#else
		*resultp = (IS_LFN_FS(ip)?1:-1);
#endif
#endif /* hp9000s300 */
		break;
	case _PC_VDISABLE:
		*resultp = (int)_POSIX_VDISABLE;
		break;
	default:
		return(EINVAL);
	}
	return(0);
}
#endif POSIX



/*
 *  ufs_lockctl() -- provide the locking functions that would be called
 *  from fcntl().  This functions assumes that parameter error checking
 *  has already occured, and that both LB and UB are valid.  The
 *  parameter *flock will be modified if the request if F_GETLK, and we
 *  assume that the calling routine (fcntl()) will take responsibility
 *  for copying the changed structure back to user space.  Other than
 *  the previous sentence, this code is essentially what was in fcntl()
 *  for the local case, modified for the fact that some things are params.
 */

/*ARGSUSED*/
int
ufs_lockctl( vp, flock, cmd, cred, fp, LB, UB )
    struct vnode *vp;
    struct flock *flock;
    int cmd;
    struct ucred *cred;
    struct file *fp;
    off_t LB, UB;
{

	struct inode *ip = VTOI(vp);

	switch(cmd) {

	case F_GETLK: 	/* F_TEST */
		if ( !(locked( F_GETLK, ip, LB, UB, L_FCNTL,
		      flock, flock->l_type, fp->f_flag)))
		   flock->l_type = F_UNLCK;
		/*
		 * else struct flock is updated as a side effect in locked()
		 */
		break;

	/*
	 * NFS doesn't want to block, so we have to call it back later.
	 */
	case F_SETLK_NFSCALLBACK:
	case F_SETLK: 	/* F_TLOCK & F_ULOCK */
	case F_SETLKW: 	/* F_LOCK */
		switch (flock->l_type) {

		case F_RDLCK:
		case F_WRLCK:
			if ( !(locked( cmd, ip, LB, UB, L_FCNTL,
			      NULL, flock->l_type, fp->f_flag))) {
			   insert_lock( ip, LB, UB, L_FCNTL, flock->l_type);
			}
			break;

		case F_UNLCK:
			/* look for lock & adjust lock list */
			delete_lock( ip, LB, UB, L_FCNTL );

		}  /* switch: flock.l_type */
		break;
	/*
	 * New function added to support NFS 3.2 lock manager.  Nfs_fcntl()
	 * needs to know whether the lock manager has any locks on the inode
	 * so it can hold the vnode if necessary.  See nfs_fcntl() in
	 * nfs_server.c and locked() and checks_for_locks() in ufs_lockf.c.
	 * For this function, instead of getting a pointer to a flock
	 * structure, we got a pointer to an int that we need to fill in.
	 */
	case F_QUERY_ANY_LOCKS:
		{
		    int *locks_held = (int *)flock;
		    *locks_held = check_for_locks(VTOI(vp), u.u_procp->p_pid);
		}
		break;
	/*
	 * New function added to support NFS 4.1 lock manager.  Nfs_fcntl()
	 * needs to know whether the lock manager has any callbacks on the
	 * inode lock list.  so it can hold the vnode if necessary.  See
	 * nfs_fcntl() in nfs_server.c and locked() and checks_for_locks()
	 * in ufs_lockf.c. For this function, instead of getting a pointer
	 * to a flock structure, we got a pointer to an int that we need to
	 * fill in.
	 */
	case F_QUERY_ANY_NFS_CALLBACKS:
		{
		    int *nfs_callbacks_held = (int *)flock;
		    *nfs_callbacks_held = check_for_nfs_callbacks(VTOI(vp));
		}
		break;

	}  /* switch: cmd */

	return (u.u_error);	/* u.u_error could be set in locked.. */
}

/*
 *  ufs_lockf() -- provide the locking functions that would be called
 *  from lockf().  This functions assumes that parameter error checking
 *  has already occured, and that both LB and UB are valid.
 *  This code is essentially what was in lockf() for the local case,
 *  modified for the fact that some things are params.
 */

/*ARGSUSED*/
int
ufs_lockf( vp, flag, len, cred, fp, LB, UB )
    struct vnode *vp;
    int flag;
    off_t len;
    struct ucred *cred;
    struct file *fp;
    off_t LB, UB;
{
	struct inode *ip = VTOI(vp);

	/*
	 * test for unlock request
	 */
	if ( flag == F_ULOCK ) {
		delete_lock(ip, LB, UB, L_LOCKF);
		return (u.u_error);
	}

	/*
 	* request must be a lock of some kind
 	* check to see if the region is lockable by this
 	* process
 	*/

	if ( locked( flag, ip, LB, UB, L_LOCKF, NULL, F_WRLCK, fp->f_flag)
	    || flag == F_TEST)
		return (u.u_error);

	insert_lock(ip, LB, UB, L_LOCKF, F_WRLCK);

	return (u.u_error);
}

/*
 * ufs_fid() -- given a vnode, return a pointer to a "file id" that can
 * be used to identify the file later on.
 */
ufs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct ufid *ufid;

	ufid = (struct ufid *)kmem_alloc(sizeof(struct ufid));
	bzero((caddr_t)ufid, sizeof(struct ufid));
	ufid->ufid_len = sizeof(struct ufid) -
		(sizeof(struct fid) - MAXFIDSZ);
	ufid->ufid_ino = VTOI(vp)->i_number;
	ufid->ufid_gen = VTOI(vp)->i_gen;
	*fidpp = (struct fid *)ufid;
	return (0);
}


#ifdef MMF
int
vasrwip(ip, uio, rw, ioflag)
	struct inode *ip;
	struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	int type;
	int synch_write;
	int total;
	int growth;
	int error;

	VASSERT((ip->i_mode&IFMT) == IFREG);

	type = ip->i_mode&IFMT;

	if (uio->uio_resid == 0)
		return (0);
	synch_write = ((ioflag & IO_SYNC) |(uio->uio_fpflags &FSYNCIO));
	if (rw == UIO_READ) {
		/*
		 * Read past end of file.
		 */
		if (uio->uio_offset >= ip->i_size)
			return(0);
		total = min(uio->uio_resid, ip->i_size - uio->uio_offset);
	} else {
		vas_t *vas;
		preg_t *prp;
		reg_t *rp;

		/*
		 * Since we are appending, we can write as much data
		 * as we wish up to rlimit.
		 */
		total = uio->uio_resid;
		if (uio->uio_offset+total > ip->i_size) {
			/*
			 * Must check rlimit here, but not now - XXX.
			 */

			/*
			 * Grow the file to the appropriate size.
			 */
			ip->i_size = (uio->uio_offset + total);
			vas = ITOV(ip)->v_vas;

			/*
			 * How do you correctly find the pregion? XXX
			 */
			VASSERT(vas);
			prp = vas->va_next;
			VASSERT(prp);
			rp = prp->p_reg;
			VASSERT(rp);
			reglock(rp);
			growth = btorp(ip->i_size) -  rp->r_pgsz;
			if (growth < 0)
				panic("vasrwip: growth < 0");
			if (growpreg(prp, growth,
					0, DBD_FSTORE, ADJ_REG, 1) < 0) {
				regrele(rp);
				return(ENOMEM);
			}
			regrele(rp);
			imark(ip, IUPD|ICHG);
		}
	}

	/*
	 * Now move the data, up to the limit of the file
	 * size.
	 */
	error = vasuiomove(ITOV(ip)->v_vas, total, rw, uio);
	if (synch_write) {
		vas_t *vas;
		preg_t *prp;
		reg_t *rp;

		/*
		 * If synch_write ensure that all of the vnode
		 * is written out to disk.
		 */
		vas = ITOV(ip)->v_vas;

		/*
		 * How do you correctly find the pregion? XXX
		 */
		VASSERT(vas);
		prp = vas->va_next;
		VASSERT(prp);
		rp = prp->p_reg;
		VASSERT(rp);
		reglock(rp);
		VOP_PAGEOUT(rp->r_bstore, prp, btop(uio->uio_offset),
			    btorp(total), PAGEOUT_HARD|PAGEOUT_WAIT);
		regrele(rp);
		iupdat(ip, 1, 0);
	} else {
		iupdat(ip, 0, 0);
	}
	return(error);
}
#endif

/*
 * Globals that impact how much read ahead is done and when.  Read ahead
 * efficiency is related to how long the disc head does useful work
 * versus how long it took the head to get there.  This is not a
 * function of file-system block size, so neither are these parameters.
 * It is however a function of media transfer rate, which unfortunately
 * is an unknown today in the file system.  Hence the globals, which if
 * someone really had a need, could be tuned for a radically different
 * speed device than the middle of the road they are set for.
 *
 * These values are sizes that include the current I/O about to be
 * scheduled.  E.g.  in an 8K file system, a ra_zerosize of 24
 * will cause 2 blocks of read-ahead to be scheduled when at block zero.
 */
/*
 * Large amounts of read-ahead are not done on the series 300 because
 * it does not have a disc sort to pull the I/O's together and its
 * scsi controller is really slow anyway.
 */
int ra_zerosize = 24;	/* how much read ahead when at block zero */
#ifdef __hp9000s800
int ra_startsize = 32;	/* how much read ahead when before ra_crossover */
int ra_runsize = 64;	/* how much read ahead when after ra_crossover */
#else
int ra_startsize = 24;	/* how much read ahead when before ra_crossover */
int ra_runsize = 24;	/* how much read ahead when after ra_crossover */
#endif
int ra_crossover = 64;	/* when to change from ra_startsize to ra_runsize */
int ra_shift = 10;	/* units of above numbers */

extern int freemem, desfree, minfree;

/*
 * Initialize the read ahead size array (format commented in breadan())
 * for the given situation.  We have imperfect information, which could
 * be remedied in the future through header file changes.  Today we keep
 * it localized.  One missing piece of information is how strong the
 * trend is, which could be used to dictate amount of read-ahead.  We
 * guess now:
 *     - if we start reading the file at block zero, assume sequential
 *     - if we are reading sequentially near the beginning of a file
 *		assume a weak trend
 *     - if we are reading sequentially after the beginning, assume a
 *		strong trend
 *     - if we are reading sequentially, but the last read was
 *		random, assume the sequentiality is an accident
 *		(e.g. randomly reading 100 bytes across a block boundary)
 *
 * There is a trade-off between amount of read-ahead and resources
 * needed to back it.  If we are low on memory, be more conservative
 * with amount of read-ahead.
 */
/* 
 * This is really gross stuff because it is two days before the end of 
 * the release.  When this is done right by really knowing the speeds
 * of logical devices and basing read ahead on that, none of this sds
 * stuff will be necessary.
 */
#ifdef __hp9000s800
#undef d_name
#include "../wsio/sds.h"
#endif

setup_readahead(ip, curb, ra_size, maxra)
    struct inode *ip;
    daddr_t curb;	/* current block we're trying to read */
    int *ra_size;
    daddr_t maxra;	/* don't do any more read-ahead than this */
{
    int size;		/* how much reading to do, start units: ra_shift */

    if (maxra <= 0)
	size = 0;
    else if (curb < lblkno(ip->i_fs, ra_crossover << ra_shift)) {
	/* near the beginning of the file */
	if (curb != 0)
	    size = ra_startsize;
	else {
	    if (freemem >= desfree)
		size = ra_zerosize;
	    else
		size = 0;
	}
    } else {
	/* past the beginning of the file */
	if (freemem >= desfree)
	    size = ra_runsize;
	else {
	    if (freemem >= minfree)
		size = ra_runsize >> 2;
	    else
		size = ra_runsize >> 3;
	}
#ifdef __hp9000s800
	{
	    extern struct Volume *sds_lookup();
	    struct Volume *readvol;	/* volume we are reading from */
	    dev_t dev;			/* device we are reading from */

	    dev = ip->i_devvp->v_rdev;
	    if (m_volume(dev) && (readvol = sds_lookup(dev)) && 
				  readvol->deviceCount > 1)
		size *= readvol->deviceCount;
	}
#endif
    }
    size <<= ra_shift;	/* change units to bytes */

    if (size <= ip->i_fs->fs_bsize)
	ra_size[0] = 0;
    else {
	/* Change to blocks.  Want to round partial blocks up, and also
	 * want to subtract a block for the non-read-ahead part so that
	 * the ra_* relate to total amount of I/O scheduled.  This is
	 * equivalent to decrementing size and rounding down.
	 */
	size = (size - 1) >> ip->i_fs->fs_bshift;
	if (size <= maxra)
	    ra_size[0] = size;
	else
	    ra_size[0] = maxra;
    }
}
