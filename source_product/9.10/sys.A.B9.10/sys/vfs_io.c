/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_io.c,v $
 * $Revision: 1.16.83.4 $       $Author: marshall $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/15 12:19:20 $
 */

/* HPUX_ID: @(#)vfs_io.c	55.1		88/12/23 */

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

/*	@(#)vfs_io.c 1.1 86/02/03 SMI	*/
/*      NFSSRC @(#)vfs_io.c	2.1 86/04/15 */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/ioctl.h"
#include "../h/kern_sem.h"
#include "../h/conf.h"
#ifdef ACLS
#include "../h/acl.h"
#endif
#include "../h/systm.h"
#include "../ufs/inode.h"
#include "../h/errno.h"
#include "../h/ld_dvr.h"
#include "../h/tty.h"

int vno_rw();
int vno_ioctl();
int vno_select();
int vno_close();
int vno_badf();

struct fileops vnodefops = {
	vno_rw,
	vno_ioctl,
	vno_select,
	vno_close
};
struct fileops vnodebadfops = {
	vno_badf,
	vno_badf,
	vno_badf,
	vno_close	/* need real close to free resources */
};

#ifdef __hp9000s300
int	net_select();
struct 	fileops netops =
	{ vno_rw, vno_ioctl, net_select, vno_close };
#endif

#ifdef HPNSE
struct inode *stream_ck();
#endif

int
vno_badf()
{
        return EBADF;
}

int
vno_rw(fp, rw, uiop)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uiop;
{
	register struct vnode *vp;
	register int count;
	register int error;
	sv_sema_t vno_rwSS;

	vp = (struct vnode *)fp->f_data;
	count = uiop->uio_resid;
	PXSEMA(&filesys_sema, &vno_rwSS);
	if (vp->v_type == VREG) {
		error =
		    VOP_RDWR(vp, uiop, rw,
			((fp->f_flag & FAPPEND) != 0?
			    IO_APPEND|IO_UNIT: IO_UNIT), fp->f_cred);
	} else {
		if (vp->v_type == VFIFO){
			SPINLOCK(file_table_lock);
			uiop->uio_offset = fp->f_offset = 0;
			SPINUNLOCK(file_table_lock);
		}
		error =
		    VOP_RDWR(vp, uiop, rw,
			((fp->f_flag & FAPPEND) != 0?
			    IO_APPEND: 0), fp->f_cred);
	}
	VXSEMA(&filesys_sema, &vno_rwSS);
	if (error)
		return(error);
	if (fp->f_flag & FAPPEND) {
		/*
		 * The actual offset used for append is set by VOP_RDWR
		 * so compute actual starting location
		 */
		SPINLOCK(file_table_lock);
		fp->f_offset = uiop->uio_offset - (count - uiop->uio_resid);
		SPINUNLOCK(file_table_lock);
	}
	return(0);
}

int
vno_ioctl(fp, com, data)
	struct file *fp;
	int com;
	caddr_t data;
{
	struct vattr vattr;
	struct vnode *vp;
	int error = 0;

	vp = (struct vnode *)fp->f_data;
	switch(vp->v_type) {

	case VREG:
	case VDIR:
	case VFIFO:
		switch (com) {

		case FIONREAD:
			error = VOP_GETATTR(vp, &vattr, u.u_cred, VIFSYNC);
			if (error == 0)
				if (vp->v_type == VFIFO)
					*(off_t *)data = vattr.va_size;
				else
					*(off_t *)data = vattr.va_size
						- fp->f_offset;
			break;

		case FIONBIO:
#ifdef	sun
		case FIOASYNC:
			break;
#endif	sun

		default:
			error = ENOTTY;
			break;
		}
		break;

	case VCHR:
		u.u_r.r_val1 = 0;
		if ((u.u_procp->p_flag & SOUSIG) == 0 && setjmp(&u.u_qsave)) {
			u.u_eosys = RESTARTSYS;
		} else {
			error = VOP_IOCTL(vp, com, data, fp->f_flag,fp->f_cred);
		}
		break;

	default:
		error = ENOTTY;
		break;
	}
	return (error);
}

int
vno_select(fp, flag)
	struct file *fp;
	int flag;
{
	struct vnode *vp;

	vp = (struct vnode *)fp->f_data;
	switch(vp->v_type) {

	case VFIFO:
	case VCHR:
		return (VOP_SELECT(vp, flag, fp->f_cred));

	default:
		/*
		 * Always selected
		 */
		switch (flag) {
		case FREAD:
		case FWRITE:
		case 0:			/* boneheaded name for exception */
			return 1;
		default:
			return 0;
		}
	}
}

int
vno_stat(vp, sb, cred, follow)
	register struct vnode *vp;
	register struct stat *sb;
	register struct ucred *cred;
	enum symfollow follow;
{
	register int error;
	struct vattr vattr;
	register long *clr, *end;

#ifdef ACLS

	/* zero out the ACL field for those file systems that don't know
	 * about them.
	 */

	vattr.va_acl = 0;
	vattr.va_basemode = 0;
#endif

	/* 0 these two fields for filesystems that don't know them */
	vattr.va_fssite = 0;
	vattr.va_rsite = 0;
	error = VOP_GETATTR(vp, &vattr, cred, VSYNC);
	if (error)
		return (error);
	sb->st_mode = vattr.va_mode;
#ifdef ACLS
	sb->st_basemode = vattr.va_basemode;
#endif
	sb->st_uid = vattr.va_uid;
	sb->st_gid = vattr.va_gid;
/* HACK For binary compatibility with pre POSIX commands */
	sb->st_reserved1 = (ushort)(sb->st_uid);
	sb->st_reserved2 = (ushort)(sb->st_gid);
/* End of binary compatability HACK */
	sb->st_dev = vattr.va_fsid;
	sb->st_ino = vattr.va_nodeid;
	sb->st_nlink = vattr.va_nlink;
	sb->st_size = vattr.va_size;
	sb->st_blksize = vattr.va_blocksize;
	sb->st_atime = vattr.va_atime.tv_sec;
	sb->st_fstype = (int) vattr.va_fstype;
	sb->st_spare1 = 0;
	sb->st_mtime = vattr.va_mtime.tv_sec;
	sb->st_spare2 = 0;
	sb->st_ctime = vattr.va_ctime.tv_sec;
	sb->st_spare3 = 0;
	sb->st_rdev = (dev_t)vattr.va_rdev;
	if ((sb->st_rdev == NODEV) && (follow == FOLLOW_LINK)) {
	    if ((sb->st_mode & IFMT) == IFBLK) {
		sb->st_rdev = rootdev;
#if defined(hp9000s800) && !defined(_WSIO)
		if (map_mi_to_lu(&sb->st_rdev, IFBLK))
		  sb->st_rdev = NODEV;
#endif /* hp9000s800 && ! _WSIO */
	    }
	    else if ((sb->st_mode & IFMT) == IFCHR) {
		sb->st_rdev = block_to_raw(rootdev);
#if defined(hp9000s800) && !defined(_WSIO)
		if (map_mi_to_lu(&sb->st_rdev, IFCHR))
		  sb->st_rdev = NODEV;
#endif /* hp9000s800 && ! _WSIO */

	    }
	}
	sb->st_blocks = vattr.va_blocks;
	sb->st_pad = 0;
	sb->st_remote = 0;
	sb->st_netdev = 0;
	sb->st_netsite = 0;
	sb->st_netino = 0;
	for(clr = &sb->st_spare4[0], end = &sb->st_spare4[SPARE4_SIZE];
		clr<end; *clr++ = 0);


	sb->st_site = vattr.va_fssite;
	sb->st_realdev = vattr.va_realdev;
	sb->st_rsite = vattr.va_rsite;
#ifdef ACLS
	sb->st_acl = vattr.va_acl;
#endif
	sb->st_spare4[0] = sb->st_spare4[1] = 0;
	return (0);
}

#ifdef ACLS

/* V N O _ G E T A C C E S S 
 *
 * Do the getaccess for this vnode. We can implement this at the vnode level
 * using the mode bits returned by stat. This will work correctly for
 * all vnode types due to change in the mode bits to reflect actual
 * access granted by the access control list.
 */

vno_getaccess(vp)
register struct vnode *vp;
{
	register int mode;
	struct vattr vattr;

	u.u_error = VOP_GETATTR(vp, &vattr, u.u_cred, VSYNC);
	if (u.u_error)
		return(-1);

	/* bug fix: if the caller is root and the owner is non-root,
 	 *          base_match will return ACL_OTHER.  This is not
	 *	    correct for a u.u_uid = 0, which should match as
	 *	    ACL_USER.
	 */
	if (u.u_uid == 0)
		mode = getbasetuple(vattr.va_mode, ACL_USER);
	else
		mode = getbasetuple(vattr.va_mode,base_match(vattr.va_uid,
				vattr.va_gid));

	/* always set RWX for SU for non-regular files. For regular
	 * files the 'x' bit was set by VOP_GETATTR if needed.
	 */

	if (u.u_uid == 0)
		if (vp->v_type == VREG)
			mode |= (R_OK|W_OK);
		else
			mode = (R_OK|W_OK|X_OK);

	/* If its a read-only file system turn off the write bit */

	if(vp->v_vfsp->vfs_flag & VFS_RDONLY) 
		mode &= ~W_OK;

	/*
	 * If there's shared text associated with
	 * the inode, try to free it up once.  If
	 * we fail, we can't allow writing.
	 */

	if (vp->v_flag & VTEXT)
		xrele(vp);
	if (vp->v_flag & VTEXT)
		mode &= ~W_OK;

	return mode;
}
#endif

int
vno_close(fp)
	register struct file *fp;
{
	register struct vnode *vp;

	sv_sema_t vno_closeSS;

	vp = (struct vnode *)fp->f_data;
	PXSEMA(&filesys_sema, &vno_closeSS);
#ifdef	sun
	if (fp->f_flag & (FSHLOCK | FEXLOCK))
		vno_unlock(fp, (FSHLOCK | FEXLOCK));
#endif	sun


	/*
	 * Call vno_lockrelease() to release all locks on the file.  It
	 * does an unlock [0, infinity), which should have essentially the
 	 * same effect as unlock(ip), except it works with NFS 3.2 also.
	 * See vno_lockrelease() in kern_dscrp.c
	 */
	(void) vno_lockrelease(fp);


	u.u_error = vn_close(vp, fp->f_flag);
	/* should this go before vn_close ? */
	SPINLOCK(file_table_lock);
	fp->f_type = 0;
	fp->f_data = (caddr_t)0;
	SPINUNLOCK(file_table_lock);
	VN_RELE(vp);
	VXSEMA(&filesys_sema, &vno_closeSS);
	return (u.u_error);
}

/*
 * Revoke access the current tty by all processes.
 * Used only by the super-user in init
 * to give ``clean'' terminals at login.
 */
vhangup()
{
	struct tty	*tp;

	if (!suser())
		return(EPERM);
	if ((tp = u.u_procp->p_ttyp) == NULL)
		return(ENOTTY);
	forceclose(u.u_procp->p_ttyd);
#ifdef __hp9000s800
	(*cdevsw[major(u.u_procp->p_ttyd)].d_option1)
	 (u.u_procp->p_ttyd, SDC_VHANGUP, 1);
#else
	/* -1 is temporary until T_VHANGUP can be defined properly in tty.h */
	(*tp->t_proc)(tp, -1, 1);
#endif
	return(0);
}

/*	for processes who ignore SIGHUP during mux reset */
forceclose(dev)
	dev_t dev;
{
	register struct file *fp;
	register struct vnode *vp;

	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_count == 0)
			continue;
		if (fp->f_type != DTYPE_VNODE)
			continue;
		SPINLOCK(file_table_lock);
		if ((fp->f_type == DTYPE_VNODE)
		&&  ((vp = (struct vnode *)fp->f_data) != 0)
		&&  (vp->v_type == VCHR)
		&&  (vp->v_rdev == dev))
			fp->f_ops = &vnodebadfops;
		SPINUNLOCK(file_table_lock);
	}
}


#ifdef	sun
/*
 * Place an advisory lock on an inode.
 */
int
vno_lock(fp, cmd)
	register struct file *fp;
	int cmd;
{
	register int priority;
	register struct vnode *vp;

	/*
	 * Avoid work.
	 */
	if ((fp->f_flag & FEXLOCK) && (cmd & LOCK_EX) ||
	    (fp->f_flag & FSHLOCK) && (cmd & LOCK_SH))
		return (0);

	priority = PLOCK;
	vp = (struct vnode *)fp->f_data;

	if ((cmd & LOCK_EX) == 0)
		priority++;
	/*
	 * If there's a exclusive lock currently applied
	 * to the file, then we've gotta wait for the
	 * lock with everyone else.
	 */
again:
	while (vp->v_flag & VEXLOCK) {
		/*
		 * If we're holding an exclusive
		 * lock, then release it.
		 */
		if (fp->f_flag & FEXLOCK) {
			vno_unlock(fp, FEXLOCK);
			goto again;
		}
		if (cmd & LOCK_NB)
			return (EWOULDBLOCK);
		vp->v_flag |= VLWAIT;
		sleep((caddr_t)&vp->v_exlockc, priority);
	}
	if (cmd & LOCK_EX) {
		cmd &= ~LOCK_SH;
		/*
		 * Must wait for any shared locks to finish
		 * before we try to apply a exclusive lock.
		 */
		while (vp->v_flag & VSHLOCK) {
			/*
			 * If we're holding a shared
			 * lock, then release it.
			 */
			if (fp->f_flag & FSHLOCK) {
				vno_unlock(fp, FSHLOCK);
				goto again;
			}
			if (cmd & LOCK_NB)
				return (EWOULDBLOCK);
			vp->v_flag |= VLWAIT;
			sleep((caddr_t)&vp->v_shlockc, PLOCK);
		}
	}
	if (fp->f_flag & (FSHLOCK|FEXLOCK))
		panic("vno_lock");
	if (cmd & LOCK_SH) {
		vp->v_shlockc++;
		vp->v_flag |= VSHLOCK;
		fp->f_flag |= FSHLOCK;
	}
	if (cmd & LOCK_EX) {
		vp->v_exlockc++;
		vp->v_flag |= VEXLOCK;
		fp->f_flag |= FEXLOCK;
	}
	return (0);
}

/*
 * Unlock a file.
 */
int
vno_unlock(fp, kind)
	register struct file *fp;
	int kind;
{
	register struct vnode *vp;
	register int flags;

	vp = (struct vnode *)fp->f_data;
	kind &= fp->f_flag;
	if (vp == NULL || kind == 0)
		return;
	flags = vp->v_flag;
	if (kind & FSHLOCK) {
		if ((flags & VSHLOCK) == 0)
			panic("vno_unlock: SHLOCK");
		if (--vp->v_shlockc == 0) {
			vp->v_flag &= ~VSHLOCK;
			if (flags & VLWAIT)
				wakeup((caddr_t)&vp->v_shlockc);
		}
		fp->f_flag &= ~FSHLOCK;
	}
	if (kind & FEXLOCK) {
		if ((flags & VEXLOCK) == 0)
			panic("vno_unlock: EXLOCK");
		if (--vp->v_exlockc == 0) {
			vp->v_flag &= ~(VEXLOCK|VLWAIT);
			if (flags & VLWAIT)
				wakeup((caddr_t)&vp->v_exlockc);
		}
		fp->f_flag &= ~FEXLOCK;
	}
}
#endif	sun

#ifdef HPNSE
/*
 * Checks to see if fp points to a stream type of a file.
 * Pointer to an inode is returned if true,
 * otherwise NULL.
 */

struct inode *
stream_ck( fp )
	struct file  *fp;
{
	struct inode *ip;
	struct vnode *vp;

	/**
	* Get the ip pointer via the vnode structure.
	**/

	if ( fp->f_type == DTYPE_VNODE ) {
		vp = (struct vnode *) fp->f_data;
		if (!vp) {
			/*
			 * it is not possible for the f_count field to be 1 and
			 * there is not yet an associated vnode ptr. We could be
			 * in the middle of an open. System V assigns the inode
			 * in the falloc() call, so there's no problem assuming
			 * a positive f_count means a valid inode ptr.
			 */
			return((struct inode *) 0);
		}
		if ( vp->v_type == VCHR ) {
			ip = VTOI(vp);
		}  else  {
			return((struct inode *) 0);
		}
	}  else  {
		return((struct inode *) 0);
	}
	if (((ip->i_mode & IFMT) != IFCHR) || (ip->i_sptr == NULL)) {
		return((struct inode *) 0);
	}
	return(ip);
}
#endif
