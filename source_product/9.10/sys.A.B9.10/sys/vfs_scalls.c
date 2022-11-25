/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_scalls.c,v $
 * $Revision: 1.13.83.9 $	$Author: drew $
 * $State: Exp $      $Locker:  $
 * $Date: 94/11/10 11:41:13 $
 */

/* HPUX_ID: @(#)vfs_scalls.c	55.1		88/12/23 */

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

/*	@(#)vfs_scalls.c 1.1 86/02/03 SMI     */
/*      NFSSRC @(#)vfs_scalls.c	2.1 86/04/15 */
/*      USED TO BE CALLED vfs_syscalls.c, shortened it to 12 chars, DLP	*/

#include "../h/param.h"
#include "../h/sysmacros.h"
#ifdef POSIX
#include "../h/time.h"
#endif POSIX

#include "../h/user.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/uio.h"
#include "../h/proc.h"
#include "../h/tty.h"
#include "../h/vfs.h"
#include "../h/dir.h"
#include "../h/pathname.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"		/* to make mknod compatable */

#include "../machine/reg.h"	/* for R1 value */


#include "../dux/lookupops.h"
#include "../dux/lkup_dep.h"
#ifdef ACLS
#include "../h/getaccess.h"
#endif
#ifdef AUDIT
#include "../h/audit.h"
#endif AUDIT

#include "../h/kern_sem.h"

extern	struct fileops vnodefops;

extern site_t my_site;          /* this site's id */

/*
 * System call routines for operations on files other
 * than read, write and ioctl.  These calls manipulate
 * the per-process file table which references the
 * networkable version of normal UNIX inodes, called vnodes.
 *
 * Many operations take a pathname, which is read
 * into a kernel buffer by pn_get (see vfs_pathname.c).
 * After preparing arguments for an operation, a simple
 * operation proceeds:
 *
 *	error = lookupname(pname, seg, followlink, &dvp, &vp)
 *
 * where pname is the pathname operated on, seg is the segment that the
 * pathname is in (UIOSEG_USER or UIOSEG_KERNEL), followlink specifies
 * whether to follow symbolic links, dvp is a pointer to the vnode that
 * represents the parent directory of vp, the pointer to the vnode
 * referenced by the pathname. The lookupname routine fetches the
 * pathname string into an internal buffer using pn_get (vfs_pathname.c),
 * and iteratively running down each component of the path until the
 * the final vnode and/or it's parent are found. If either of the addresses
 * for dvp or vp are NULL, then it assumes that the caller is not interested
 * in that vnode.  Once the vnode or its parent is found, then a vnode
 * operation (e.g. VOP_OPEN) may be applied to it.
 *
 * One important point is that the operations on vnode's are atomic, so that
 * vnode's are never locked at this level.  Vnode locking occurs
 * at lower levels either on this or a remote machine. Also permission
 * checking is generally done by the specific filesystem. The only
 * checks done by the vnode layer is checks involving file types
 * (e.g. VREG, VDIR etc.), since this is static over the life of the vnode.
 *
 */


/*
 * Change current working directory (".").
 */
chdir(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{
	struct vnode *vp;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(uap->dirnamep);
	}
#endif /* AUDIT */
	PSEMA(&filesys_sema);
	u.u_error = chdirec(uap->dirnamep, &vp);
	VSEMA(&filesys_sema);
	if (u.u_error == 0) {
		release_cdir();
		u.u_cdir = vp;
	}
}

/*
 * Change notion of root ("/") directory.
 */
chroot(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{
	struct vnode *vp;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(uap->dirnamep);
	}
#endif /* AUDIT */

	if (!suser())
		return;

	PSEMA(&filesys_sema);
	u.u_error = chdirec(uap->dirnamep, &vp);
	if (u.u_error == 0) {
		if (u.u_rdir != (struct vnode *)0)
		{
			update_duxref(u.u_rdir,-1,0);
			VN_RELE(u.u_rdir);
		}
		u.u_rdir = vp;
	}
	VSEMA(&filesys_sema);
}

/*
 * Common code for chdir and chroot.
 * Translate the pathname and insist that it
 * is a directory to which we have execute access.
 * If it is replace u.u_[cr]dir with new vnode.
 */
chdirec(dirnamep, vpp)
	char *dirnamep;
	struct vnode **vpp;
{
	struct vnode *vp;		/* new directory vnode */
	register int error;

        error =
	    lookupname(dirnamep, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_LOOKUP, (caddr_t)0);
	if (error)
		return (error);
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
	} else {
		error = VOP_ACCESS(vp, VEXEC, u.u_cred);
	}
	if (error) {
		update_duxref(vp,-1,0);
		VN_RELE(vp);
	} else {
		*vpp = vp;
	}
	return (error);
}

/*
 * fchdir() -- system call to change the current working directory to a
 *	       directory referenced by an open file descriptor.
 */
void
fchdir()
{
    struct a {
	int fdes;
    } *uap;
    struct file *fp;
    struct vnode *vp;

    uap = (struct a *)u.u_ap;

    if ((fp = getf(uap->fdes)) == 0)
	return;	 /* getf() sets u.u_error */

    /*
     * Make sure that this is a directory.
     */
    if (fp->f_type != DTYPE_VNODE ||
	(vp = (struct vnode *)fp->f_data)->v_type != VDIR) {
	u.u_error = ENOTDIR;
	return;
    }

    /*
     * We have two choices for determining the access rights:
     *	 fp->f_cred --
     *	   This seems more correct, i.e. we would be using the access
     *	   rights (uid/gid, etc) at the time the directory was opened.
     *	   This is consistent with other f*() system calls.
     *
     *	 u.u_cred --
     *	   This is what BSD and ATT V.3 seem to do.  This is somewhat
     *	   inconsistent with other f*() system calls but makes sense
     *	   in a weird sort of way.  If the user cannot search this dir
     *	   now (using their current credentials), there probably is not
     *	   much point in making this their CWD -- other stuff will fail.
     *	   For consistency, I have decided to use u.u_cred.
     *
     * - rsh 06/21/91
     */
    if ((u.u_error = VOP_ACCESS(vp, VEXEC, u.u_cred)) != 0)
	return;

    /*
     * All tests passed, change the CWD.
     */
    PSEMA(&filesys_sema);
    VN_HOLD(vp);		/* another reference to this vnode */
    VSEMA(&filesys_sema);
    update_duxref(vp, 1, 0);	/* reference for DUX */

    release_cdir();
    u.u_cdir = vp;
}

/*
 * Open system call.
 */
open(uap)
	register struct a {
		char *fnamep;
		int fmode;
		int cmode;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = copen(uap->fnamep, uap->fmode - FOPEN, uap->cmode);
	VSEMA(&filesys_sema);
}

/*
 * Creat system call.
 */
creat(uap)
	register struct a {
		char *fnamep;
		int cmode;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = copen(uap->fnamep, FWRITE|FCREAT|FTRUNC, uap->cmode);
	VSEMA(&filesys_sema);
}


/*
 * Common code for open, creat.
 */
copen(pnamep, filemode, createmode)
	char *pnamep;
	int filemode;
	int createmode;
{
	register struct file *fp;
	struct vnode *vp;
	register int error;
	register int i;
	int the_pid;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(pnamep);
	}
#endif /* AUDIT */

	if ((filemode&(FREAD|FWRITE)) == 0) {
		return(EINVAL);
	}
#ifdef POSIX
	if ((filemode&(FNDELAY|FNBLOCK)) == (FNDELAY|FNBLOCK)) {
		return(EINVAL);		/* can't have it both ways */
	}
#endif

	/*
	 * allocate a user file descriptor and file table entry.
	 */
	fp = falloc();
	if (fp == NULL)
		return(u.u_error);

	i = u.u_r.r_val1;		/* this is bull*/
	fp->f_type = NULL;

	/*
	 * open the vnode.
	 */
	error =
	    vn_open(pnamep, UIOSEG_USER,
		filemode, ((createmode & 07777) & ~u.u_cmask), &vp);

	/*
	 * If there was an error, deallocate the file descriptor.
	 * Otherwise fill in the file table entry to point to the vnode.
	 */
	if (error) {
		uffree(i);
		crfree(fp->f_cred);
                fp->f_cred = (struct ucred *)NULL;
		fp->f_count = 0;
	}
	else if (fp->f_type == NULL)	/* Not munged by networking */
	{
		fp->f_type = DTYPE_VNODE;
	        fp->f_ops = &vnodefops;
		fp->f_data = (caddr_t)vp;
	        fp->f_flag = filemode & FMASK;
	}
	return(error);
}

/*
 * Create a special (or regular) file.
 */
/*
 * mkrnod includes the site, mknod doesn't
 */
mknod(uap)
	register struct a {
		char		*pnamep;
		int		fmode;
		int		dev;
	} *uap;
{
	mknod1(uap,0);
}

mkrnod(uap)
	register struct a {
		char		*pnamep;
		int		fmode;
		int		dev;
		int		site;
	} *uap;
{
	mknod1(uap,1);
}

mknod1(uap,is_mkrnod)
	register struct a {
		char		*pnamep;
		int		fmode;
		int		dev;
		int		site;
	} *uap;
	int is_mkrnod;
{
	struct vnode *vp;
	struct vattr vattr;
	enum vtype type;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(uap->pnamep);
	}
#endif /* AUDIT */

	/*
	 * Must be super user
	 */
	/* fix this up!!!!!!!  The way IFTOVT is set up, all */
	/* invalid parameters will map to VFIFO */
	if ((uap->fmode & IFMT) == 0)
		type = VREG;
	else
		type = IFTOVT(uap->fmode);
	if (type != VFIFO && !suser())
		return;

	/*
	 * Setup desired attributes and vn_create the file.
	 */
	vattr_null(&vattr);
	vattr.va_type = type;

	/*
	 * When we mknod a directory, we don't want to create it with "."
	 * and ".." in it, we therefore change VDIR to VEMPTYDIR for the
	 * moment and change it back in the file system code (dirmakeinode)
	 */
	if (vattr.va_type == VDIR) {
		vattr.va_type = VEMPTYDIR;
		vattr.va_mode = (uap->fmode & 0777) & ~u.u_cmask;
	}
	else {
		vattr.va_mode = (uap->fmode & 07777) & ~u.u_cmask;
	}

	switch (vattr.va_type) {
	case VCHR:
	case VBLK:
		vattr.va_rdev = uap->dev;
		vattr.va_rsite = is_mkrnod?(site_t)(uap->site):my_site;
		break;

	case VBAD:
	case VNON:
		u.u_error = EINVAL;
		return;

	default:
		break;
	}

	PSEMA(&filesys_sema);
	u.u_error = vn_create(uap->pnamep, UIOSEG_USER, &vattr, EXCL, 0, &vp,
		LKUP_MKNOD, 0);
	if (u.u_error == 0)
		VN_RELE(vp);
	VSEMA(&filesys_sema);
}

/*
 * Make a directory.
 */
mkdir(uap)
	struct a {
		char	*dirnamep;
		int	dmode;
	} *uap;
{
	struct vnode *vp;
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_type = VDIR;
	vattr.va_mode = (uap->dmode & 0777) & ~u.u_cmask;

	PSEMA(&filesys_sema);
	u.u_error = vn_create(uap->dirnamep, UIOSEG_USER, &vattr, EXCL, 0, &vp,
		LKUP_MKNOD, 0);
        if (u.u_error == 0){
                VN_RELE(vp);
        }
        else{
                /*
                 * the locallookuppn call in vn_create() will set error to
                 * EISDIR if we try to look up "/" with a non-null dvp,
                 * which is the case with "mkdir /".  since mkdir  "/"
                 * should return EEXIST, change error if it is EISDIR.
                 */
                if (u.u_error == EISDIR)
                        u.u_error = EEXIST;

        }
	VSEMA(&filesys_sema);
}

/*
 * make a hard link
 */
link(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = vn_link(uap->from, uap->to, UIOSEG_USER);
	VSEMA(&filesys_sema);
}

/*
 * rename or move an existing file
 */
rename(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = vn_rename(uap->from, uap->to, UIOSEG_USER);
	VSEMA(&filesys_sema);
}

/*
 * Create a symbolic link.
 * Similar to link or rename except target
 * name is passed as string argument, not
 * converted to vnode reference.
 */
symlink(uap)
	register struct a {
		char	*target;
		char	*linkname;
	} *uap;
{
	struct vnode *dvp = (struct vnode *) 0;
	struct vattr vattr;
	struct pathname tpn;
	struct pathname lpn;

	u.u_error = pn_get(uap->linkname, UIOSEG_USER, &lpn);
	if (u.u_error)
		return;
	PSEMA(&filesys_sema);
	u.u_error = lookuppn(&lpn, NO_FOLLOW, &dvp, (struct vnode **)0, LKUP_LOOKUP, (caddr_t) 0);
	if (u.u_error) {
		pn_free(&lpn);
		VSEMA(&filesys_sema);
		return;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
		goto out;
	}
	u.u_error = pn_get(uap->target, UIOSEG_USER, &tpn);
	vattr_null(&vattr);
	vattr.va_mode = 0777 & ~u.u_cmask;
	if (u.u_error == 0) {
		u.u_error =
		   VOP_SYMLINK(dvp, lpn.pn_path, &vattr, tpn.pn_path, u.u_cred);
		pn_free(&tpn);
	}
out:
	pn_free(&lpn);
	update_duxref(dvp, -1, 0);
	VN_RELE(dvp);
	VSEMA(&filesys_sema);
}

/*
 * Unlink (i.e. delete) a file.
 */
unlink(uap)
	struct a {
		char	*pnamep;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = vn_remove(uap->pnamep, UIOSEG_USER, 0);
	VSEMA(&filesys_sema);
}

/*
 * Remove a directory.
 */
rmdir(uap)
	struct a {
		char	*dnamep;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = vn_remove(uap->dnamep, UIOSEG_USER, 1);
	VSEMA(&filesys_sema);
}

/*
 * get directory entries in a file system independent format
 *	All routines which use VOP_READDIR (currently getdirentries() and
 *	rfs_readdir()) should have no knowledge of the size and alignment
 *	restrictions of the underlying file system.
 *	The statement on the man page that the read size must
 *	be greater than the block size has not been enforced by hp-ux,
 *	is meaningless because there is no offset restriction,
 *	and cannot be enforced over small-packet NFS links.
 *	See comments for rfs_readdir().
 *	The various VOP_READDIR routines must therefore deal reasonably
 *	with offsets into the middle of blocks and read sizes smaller
 *	than a full block. For an example see ufs_readdir().
 */
getdirentries(uap)
	register struct a {
		int	fd;
		char	*buf;
		unsigned count;
		long	*basep;
	} *uap;
{
	struct file *fp;
	struct uio auio;
	struct iovec aiov;

	fp = getf(uap->fd);
	if (fp == NULL) return;
	if ((fp->f_flag & FREAD) == 0) {
		u.u_error = EBADF;
		return;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_seg = UIOSEG_USER;
	auio.uio_resid = uap->count;

	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = EINVAL;
		return;
	}
	if ((int)uap->count < (int)sizeof (struct direct)) {
		u.u_error = EINVAL;
		return;
	}
	PSEMA(&filesys_sema);
	u.u_error = VOP_READDIR((struct vnode *)fp->f_data, &auio,
				 fp->f_cred);
	VSEMA(&filesys_sema);
	if (u.u_error)
		return;
	u.u_error =
	   copyout((caddr_t)&fp->f_offset, (caddr_t)uap->basep,
		   sizeof(long));

	if (u.u_error)
		return;

	u.u_r.r_val1 = uap->count - auio.uio_resid;
	SPINLOCK(file_table_lock);
	fp->f_offset = auio.uio_offset;
	SPINUNLOCK(file_table_lock);
}

/*
 * Seek on file.  Only hard operation
 * is seek relative to end which must
 * apply to vnode for current file size.
 *
 * Note: lseek(0, 0, L_XTND) costs much more than it did before.
 */

lseek(uap)
	register struct a {
		int	fd;
		off_t	off;
		int	sbase;
	} *uap;
{
	struct file *fp;
	register struct vnode *vp;

	fp = getf(uap->fd);
	if (fp == NULL)
		return;

	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = EINVAL;
		return;
	}

	vp = (struct vnode *)fp->f_data;
	/* check to see if user requests seek on pipe */
	if (vp->v_type == VFIFO) {
		u.u_error = ESPIPE;
		return;
	}

	switch (uap->sbase) {
	case L_INCR:
		uap->off += fp->f_offset;
		break;

	case L_XTND: {
		struct vattr vattr;
		PSEMA(&filesys_sema);
		u.u_error =
		     VOP_GETATTR((struct vnode *)fp->f_data, &vattr, u.u_cred,
				 VIFSYNC);
		VSEMA(&filesys_sema);
		if (u.u_error)
			return;
		uap->off += vattr.va_size;
		break;
	}

	case L_SET:
		break;

	default:
		u.u_error = EINVAL;
		return;
	}

	/* check for lseek request which make offset negative on non-device */
        /* files or which make offset equal to -1 on device files. */
	if ((uap->off < 0) && (vp->v_type != VCHR) && (vp->v_type != VBLK) ||
            (uap->off == -1)) {
		u.u_error = EINVAL;
		return;
	}
	SPINLOCK(file_table_lock);
	fp->f_offset = uap->off;
	SPINUNLOCK(file_table_lock);
	u.u_r.r_off = uap->off;
}

/*
 * Determine accessibility of file, by
 * reading its attributes and then checking
 * against our protection policy.
 */
saccess(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	struct vnode *vp;
	register u_short mode;
	register struct ucred *tcred1, *tcred2; /*Temp cred's structure for
						  access checking */
	u_int amode;	/*addressible place for putting mode */

	/* return error if extra bits are set in access mode */
	if (uap->fmode & ~(R_OK|W_OK|X_OK)) {
	    u.u_error = EINVAL;
	    return;
	}

	/*determine the mode before lookup so we can pass it as a dependent*/

	mode = 0;
	/*
	 * fmode == 0 means only check for exist
	 */
	if (uap->fmode) {
		if (uap->fmode & R_OK)
			mode |= VREAD;
		if (uap->fmode & W_OK) {
			mode |= VWRITE;
		}
		if (uap->fmode & X_OK)
			mode |= VEXEC;
	}
	amode = mode;

	/* we need to do this before the lookup since it will be
	   checking permission bits along the pathname.
	*/

	/*
	 * Use the real uid and gid and check access
	 */
        tcred2 = u.u_cred;
        tcred1 = crdup(tcred2);
        tcred1->cr_uid = tcred1->cr_ruid;
        tcred1->cr_gid = tcred1->cr_rgid;
        u.u_cred = tcred1;


	PSEMA(&filesys_sema);
	/*
	 * Lookup file
	 */
	u.u_error =
	    lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_ACCESS, &amode);
	if (u.u_error) {
		goto out1;
	}
	/*
	 * fmode == 0 means only check for exist
	 */
	if (uap->fmode) {
		if((mode & VWRITE) && (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
			u.u_error = EROFS;
			goto out;
		}
		u.u_error = VOP_ACCESS(vp, mode, u.u_cred);
	}

	/*
	 * release the vnode and restore the uid and gid
	 */
out:
	VN_RELE(vp);
out1:
	VSEMA(&filesys_sema);
	u.u_cred = tcred2;
	crfree(tcred1);
}

#ifdef ACLS

/* G E T A C C E S S
 *
 * The getaccess system call. We do two lookups here. Once to determine
 * if the caller has access, once to determine the access being asked for.
 * For the second lookup we set up the credentials from the parameters
 * handed us. vno_getaccess() determines the actual mode.
 */

getaccess(uap)
	register struct a {
		char	*fname;
		int	uid;
		int	gid_count;
		int	*gid_array;
		int	label;			/* currently unused */
		int	privileges;		/* currently unused */
	} *uap;
{
	struct vnode *vp;
	register u_short mode;
	register int i = 10;
	register struct ucred *tcred1, *tcred2; /*Temp cred's structure for */
						/*access checking */
	unsigned int gid = NOGROUP;

	if (uap->gid_count > NGROUPS + 1)
	{
		u.u_error = EINVAL;
		return;
	}

	/* Do a lookup with the callers ucred structure first--he must
	 * have access to something before he is allowed to see if anyone
	 * else does
	 */

	u.u_error =
	    lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_LOOKUP, 0);
	if (u.u_error)
		return;

	update_duxref(vp,-1,0);
	VN_RELE(vp);

	/* We need to set the u_cred structure to look like what we want it
	 * to before we do the lookup so that the path can be properly
	 * checked.
	 */

	tcred2 = u.u_cred;
	tcred1 = crdup(tcred2);
	tcred1->cr_uid = uap->uid;

	/* check for the recognized values of uid */

	switch (tcred1->cr_uid) {

	case UID_EUID:
		tcred1->cr_uid = tcred2->cr_uid;
		break;

	case UID_RUID:
		tcred1->cr_uid = tcred2->cr_ruid;
		break;

	case UID_SUID:
		tcred1->cr_uid = u.u_procp->p_suid;
		break;
	}

	/* if gid count is greater than zero then the caller is providing
	 * the list of gid's to be checked. copyin the first gid value.
	 * that becomes the u.u_gid value. The rest are copyin'd to the
	 * group array.
	 */

	if (uap->gid_count > 0)
	{
		u.u_error = copyin(uap->gid_array,&gid, sizeof (int));
		if (u.u_error)
			goto out1;
		if (gid >= (unsigned int)MAXUID)
		{
			u.u_error = EINVAL;
			goto out1;
		}
		tcred1->cr_gid  = gid;
		for (i=0; i<NGROUPS; i++)
			tcred1->cr_groups[i] = NOGROUP;
		u.u_error = copyin(&uap->gid_array[1],tcred1->cr_groups,
				(uap->gid_count-1) * sizeof (int));
		if (u.u_error)
			goto out1;
		for (i=0; i<NGROUPS; i++)
			if (tcred1->cr_groups[i] >= MAXUID)
			{
				u.u_error = EINVAL;
				goto out1;
			}
	}
	else
	{
		/* check the recognized gid count values */

		switch (uap->gid_count) {

		case NGROUPS_EGID:	/* process's effective gid only */
			tcred1->cr_gid = tcred2->cr_gid;
			for (i=0; i<NGROUPS; i++)
				tcred1->cr_groups[i] = NOGROUP;
			break;

		case NGROUPS_RGID:	/* process's real gid only */
			tcred1->cr_gid = tcred2->cr_rgid;
			for (i=0; i<NGROUPS; i++)
				tcred1->cr_groups[i] = NOGROUP;
			break;

		case NGROUPS_SGID:	/* process's saved gid only */
			tcred1->cr_gid = u.u_sgid;
			for (i=0; i<NGROUPS; i++)
				tcred1->cr_groups[i] = NOGROUP;
			break;

		case NGROUPS_SUPP:	/* process's group list only */

			/* move group list[0] into gid and move all
			 * group list elements down one
			 */

			tcred1->cr_gid = tcred1->cr_groups[0];
			for (i=1; i<NGROUPS; i++)
				tcred1->cr_groups[i-1] =
					tcred1->cr_groups[i];
			break;

		case NGROUPS_EGID_SUPP:	/* effective plus group list */
			/*  everything already set */
			break;

		case NGROUPS_RGID_SUPP:	/* real plus group list */
			tcred1->cr_gid = tcred2->cr_rgid;
			break;

		case NGROUPS_SGID_SUPP:	/* real plus group list */
			tcred1->cr_gid = u.u_sgid;
			break;

		default:
			u.u_error = EINVAL;
			goto out1;
		}

	}
	u.u_cred = tcred1;

	u.u_error =
	    lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_GETACCESS, 0);
	if (u.u_error) {
		goto out1;
	}

	mode = vno_getaccess(vp);
	if (!u.u_error)
		u.u_r.r_val1 = mode;

	/*
	 * release the vnode and restore the uid and gid
	 */

	VN_RELE(vp);
out1:
	u.u_cred = tcred2;
	crfree(tcred1);
}
#endif

/*
 * Get attributes from file or file descriptor.
 * Argument says whether to follow links, and is
 * passed through in flags.
 */
stat(uap)
	struct a {
		char	*fname;
		struct	stat *ub;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = stat1(uap, FOLLOW_LINK);
	VSEMA(&filesys_sema);
}

lstat(uap)
	struct a {
		char	*fname;
		struct	stat *ub;
	} *uap;
{

	PSEMA(&filesys_sema);
	u.u_error = stat1(uap, NO_FOLLOW);
	VSEMA(&filesys_sema);
}

stat1(uap, follow)
	register struct a {
		char	*fname;
		struct	stat *ub;
	} *uap;
	enum symfollow follow;
{
	struct vnode *vp;
	struct stat sb;
	register int error;

	bzero (&sb, sizeof(struct stat));
	error =
	    lookupname(uap->fname, UIOSEG_USER, follow,
		(struct vnode **)0, &vp, LKUP_STAT, (caddr_t)uap->ub);
	if (error)
		return (error);
	error = vno_stat(vp, &sb, u.u_cred, follow);
	VN_RELE(vp);
	if (error)
		return (error);
	return (copyout((caddr_t)&sb, (caddr_t)uap->ub, sizeof (sb)));
}

/*
 * Read contents of symbolic link.
 */
readlink(uap)
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap;
{
	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;
        extern int hpux_aes_override;

	PSEMA(&filesys_sema);
	u.u_error =
	    lookupname(uap->name, UIOSEG_USER, NO_FOLLOW,
		(struct vnode **)0, &vp, LKUP_LOOKUP, 0);
	if (u.u_error) {
		VSEMA(&filesys_sema);
		return;
	}
	if (vp->v_type != VLNK) {
		u.u_error = EINVAL;
		goto out;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_seg = UIOSEG_USER;
	auio.uio_resid = uap->count;
	u.u_error = VOP_READLINK(vp, &auio, u.u_cred);
out:
	update_duxref(vp,-1,0);
	VN_RELE(vp);
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	/*
	 * AES spec says we must null terminate the returned string.
         * Only do it if there has been no errors and there is at least
         * one byte of free space in the user's buffer.
	 */
	if (!u.u_error && (auio.uio_resid > 0) && (!hpux_aes_override))
           subyte( (caddr_t)uap->buf + u.u_r.r_val1, '\0');

	VSEMA(&filesys_sema);
}

/*
 * Change mode of file given path name.
 */
chmod(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	/* KLUDGE!!! FIX IF POSSIBLE */
	PSEMA(&filesys_sema);
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr, 0);
	VSEMA(&filesys_sema);
}


/*
 * Change mode of file given path name, but don't follow if it's a symlink
 */
lchmod(uap)
        register struct a {
                char    *fname;
                int     fmode;
        } *uap;
{
        struct vattr vattr;

        vattr_null(&vattr);
        vattr.va_mode = uap->fmode & 07777;
        /* KLUDGE!!! FIX IF POSSIBLE */
        PSEMA(&filesys_sema);
        u.u_error = namesetattr(uap->fname, NO_FOLLOW, &vattr, 0);
        VSEMA(&filesys_sema);
}

/*
 * Change mode of file given file descriptor.
 */
fchmod(uap)
	register struct a {
		int	fd;
		int	fmode;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	PSEMA(&filesys_sema);
	u.u_error = fdsetattr(uap->fd, &vattr);
	VSEMA(&filesys_sema);
}


/*
 * Change ownership of file given file name.
 */
chown(uap)
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap;
{
	struct vattr vattr;
        extern int hpux_aes_override;
	enum symfollow followlink;

	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
        if( hpux_aes_override ) 
	     followlink =  NO_FOLLOW;
        else followlink =  FOLLOW_LINK;

	PSEMA(&filesys_sema);
	/* KLUDGE!!! FIX IF POSSIBLE */
	u.u_error = namesetattr(uap->fname, followlink,  &vattr, 0);
	VSEMA(&filesys_sema);
}

/*
 * Change ownership of file given file descriptor.
 */
fchown(uap)
	register struct a {
		int	fd;
		int	uid;
		int	gid;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	PSEMA(&filesys_sema);
	u.u_error = fdsetattr(uap->fd, &vattr);
	VSEMA(&filesys_sema);
}

#ifdef	POSIX
/*
 * Set flag for pathname lookup to decide whether to truncate or not.
 */
#ifdef S2POSIX_NO_TRUNC
set_no_trunc(uap)
	register struct a {
		int	flag;
	} *uap;
{
	SPINLOCK(sched_lock);
	u.u_procp -> p_flag2 &= ~S2POSIX_NO_TRUNC;

	if (uap -> flag) {
		u.u_procp -> p_flag2 |= S2POSIX_NO_TRUNC;
	}
	SPINUNLOCK(sched_lock);
}
#endif


pathconf(uap)
	register struct a {
		char	*path;
		int	name;
	} *uap;
{
	struct vnode *vp;
	int	result;

	PSEMA(&filesys_sema);
	u.u_error = lookupname(uap->path, UIOSEG_USER, 1, (struct vnode **)0,
			       &vp, LKUP_PATHCONF, (caddr_t)uap->name);
	if (u.u_error) {
		VSEMA(&filesys_sema);
		return;
	}

	u.u_error = VOP_PATHCONF(vp, (caddr_t)uap->name, &result, u.u_cred);
	VSEMA(&filesys_sema);
	VN_RELE(vp);
	u.u_r.r_val1 = result;
}


fpathconf(uap)
	register struct a {
		int	fd;
		int	name;
	} *uap;
{
	register struct vnode *vp;
	struct file *fp;
	int	result;

	GETF(fp,uap->fd);

	vp = (struct vnode *) fp->f_data;

	if (fp->f_type != DTYPE_VNODE) {
		u.u_error=EINVAL;
		return;
	}
	PSEMA(&filesys_sema);
	u.u_error=VOP_FPATHCONF(vp, (caddr_t)uap->name, &result, fp->f_cred);
	VSEMA(&filesys_sema);
	u.u_r.r_val1 = result;
}
#endif	POSIX


#if defined(__hp9000s300) && defined(BSD_ONLY)
/*
 * Set access/modify times on named file.
 */
utimes(uap)
	register struct a {
		char	*fname;
		struct	timeval *tptr;
	} *uap;
{
	struct timeval tv[2];
	struct vattr vattr;

	u.u_error = copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof (tv));
	if (u.u_error)
		return;
	vattr_null(&vattr);
	vattr.va_atime = tv[0];
	vattr.va_mtime = tv[1];
	/* KLUDGE!!! FIX IF POSSIBLE */
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr, 0);
}
#endif /* hp9000s200 && BSD_ONLY */


/*
 * Truncate a file given its path name.
 */
truncate(uap)
	register struct a {
		char	*fname;
		int	length;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_size = uap->length;

        /*
         * First, check that the truncate length does not exceed the 
         * UL_SETFSIZE specified via ulimit().
         */
        if( vattr.va_size > (u.u_rlimit[RLIMIT_FSIZE].rlim_cur * 512)){
           u.u_error = EFBIG;
        }
        else {
	   PSEMA(&filesys_sema);
	   /* KLUDGE!!! FIX IF POSSIBLE */
	   u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr, 0);
	   VSEMA(&filesys_sema);
        }
}

/*
 * Truncate a file given a file descriptor.
 */
ftruncate(uap)
	register struct a {
		int	fd;
		int	length;
	} *uap;
{
	register struct vnode *vp;
	struct file *fp;

	fp = getf(uap->fd);
	if (fp == NULL)
		return;

	if ((fp->f_flag & FWRITE) == 0) {
		u.u_error = EINVAL;
		return;
	}

        /*
         * Check that the truncate length does not exceed the 
         * UL_SETFSIZE specified via ulimit().
         */
        if( uap->length > (u.u_rlimit[RLIMIT_FSIZE].rlim_cur * 512)){
           u.u_error = EFBIG;
           return;
        }

	vp = (struct vnode *)fp->f_data;

	switch (fp->f_type) {
	case DTYPE_VNODE:
		break;
	case DTYPE_SOCKET:
	case DTYPE_UNSP:
	case DTYPE_LLA:
		u.u_error = EINVAL;
		return;
	default:
		panic("ftruncate:  unsupported file type");

	}

#ifdef LOCAL_DISC
	/* See comments in fdsetattr why this is neccessary */
	if (vp->v_vfsp == NULL) {
		u.u_error = EIO;
	} else
#endif
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
	} else {
		struct vattr vattr;

		vattr_null(&vattr);
		vattr.va_size = uap->length;
		PSEMA(&filesys_sema);
		/*
		 * KLUDGE!!! FIX IF POSSIBLE
		 * The null_time parameter (used to enforce utime()
		 * permission checking) to VOP_SETATTR
		 * should be wrapped into the vattr structure).
		 *
		 * we are reverting to passing fp->f_cred (we used
		 * to pass u.u_cred) so that ftruncate will
		 * use the credentials
		 */
		u.u_error = VOP_SETATTR(vp, &vattr, fp->f_cred, 0);
		VSEMA(&filesys_sema);
	}
}

/*
 * Common routine for modifying attributes
 * of named files.
 */
/* KLUDGE!!! FIX IF POSSIBLE - null_time should be merged into vap */
namesetattr(fnamep, followlink, vap, null_time)
	char *fnamep;
	enum symfollow followlink;
	struct vattr *vap;
	int null_time;
{
	struct vnode *vp;
	register int error;
	struct setattrops setattrops;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(fnamep);
	}
#endif /* AUDIT */

	setattrops.vap = vap;
	setattrops.null_time = null_time;

	/*
	 * Special code to handle DUX memory mapped files...
	 *
	 * If the size of the file is changing, we need to perform the
	 * operation in two steps so that we know if the file shrunk or
	 * not.  This is so that if the file is memory mapped, we can
	 * invalidate pages past the new end of the object.  If we
	 * simply do the LKUP_NAMESETATTR call, the remote site will do
	 * all the work, and we will not find out what happened.
	 */
	if (vap->va_size == -1) {
		error = lookupname(fnamep, UIOSEG_USER, followlink,
			     (struct vnode **)0, &vp, LKUP_NAMESETATTR,
			     (caddr_t)&setattrops);
	}
	else {
		error = lookupname(fnamep, UIOSEG_USER, followlink,
			     (struct vnode **)0, &vp, LKUP_LOOKUP, (caddr_t)0);
	}
	if (error)
		return(error);
	if(vp->v_vfsp->vfs_flag & VFS_RDONLY)
		error = EROFS;
	else
		error = VOP_SETATTR(vp, vap, u.u_cred, null_time);
	update_duxref(vp, -1, 0);
	VN_RELE(vp);
	return(error);
}

/*
 * Common routine for modifying attributes
 * of file referenced by descriptor.
 */
fdsetattr(fd, vap)
	int fd;
	struct vattr *vap;
{
	struct file *fp;
	register struct vnode *vp;
	register int error;

	error = getvnodefp(fd, &fp);
	if (error == 0) {
#ifdef LOCAL_DISC
                struct vfs *vfsp;
#endif /* LOCAL_DISC */

		vp = (struct vnode *)fp->f_data;
#ifdef LOCAL_DISC
		vfsp = vp->v_vfsp;

                /*
                 * We may be trying to fdsetattr() a file that has been
		 * "disowned" after its serving site has crashed.  Not clear
		 * what the right error for this case is, but let's be
		 * consistent with the read/write and fstatfs cases and
		 * generate EIO.
                 */

                if (vfsp == NULL) {
                        return EIO;
                }
		if(vfsp->vfs_flag & VFS_RDONLY)
			return(EROFS);
#else   /* LOCAL_DISC */
		if(vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return(EROFS);
#endif  /* LOCAL_DISC */
		/* KLUDGE!!! FIX IF POSSIBLE */
		/* we pass u.u_cred instead of fp->f_cred so
		 * that fchmod's and fchown's (which are the only
		 * operations that utilize fdseattr()),
		 * always use the current credentials.
		 */
		error = VOP_SETATTR(vp, vap, u.u_cred, 0);
	}
	return(error);
}

/*
 * Flush output pending for file.
 */
fsync(uap)
	struct a {
		int	fd;
	} *uap;
{
	struct file *fp;

	PSEMA(&filesys_sema);
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0)
		u.u_error = VOP_FSYNC((struct vnode *)fp->f_data, fp->f_cred,1);
		/* KLUDGE:  for update inode time, ties with closef */
	VSEMA(&filesys_sema);
}

/*
 * Set file creation mask.
 */
umask(uap)
	register struct a {
		int mask;
	} *uap;
{
	u.u_r.r_val1 = u.u_cmask;
	u.u_cmask = uap->mask & 0777;
}


/*
 * Get the file structure entry for the file descrpitor, but make sure
 * its a vnode.
 */
int
getvnodefp(fd, fpp)
	int fd;
	struct file **fpp;
{
	register struct file *fp;

	fp = getf(fd);
	if (fp == (struct file *)0)
		return(EBADF);
	if (fp->f_type != DTYPE_VNODE)
		return(EINVAL);
	*fpp = fp;
	return(0);
}

#ifdef ACLS
/*
 * S E T A C L
 *
 * This is the setacl system call
 * It sets up the lookup structure, copies in the data,
 * gets the vnode and calls the vnodeop to handle it
 */

setacl(uap)
	register struct a {
		char	*fname;			/* file name */
		int	ntuples;		/* number of tuples to set */
		struct  acl_tuple_user *tupleset; /* address of the acl */
	} *uap;
{
	struct vnode *vp;
	struct setaclops setacl_ops;	/* structure used to pass
					 *information used by lookup */
	struct acl_tuple_user acl_user[NACLTUPLES];
	register int i;
	register struct acl_tuple_user *tpu;

#ifdef MP
		/* for MP systems disable setting acls, reading acls OK */
	if(!uniprocessor){
		u.u_error = EINVAL;
		return;
	}
#endif
#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(uap->fname);
	}
#endif /* AUDIT */

	/* The user cannot set more than NACLTUPLES tuples */

	if (uap->ntuples > NACLTUPLES)
	{
		u.u_error = EINVAL;
		return;
	}


	setacl_ops.ntuples = uap->ntuples;


	/* If the user specifies < 0 tuples, skip what follows.
	 * Just lookup the name and broadcast the operation, passing
	 * on the negative number of tuples.
	 */

	if (uap->ntuples >= 0) {


		/* copy the acl from user space to kernel space. EFAULT is
	 	 * generated if the user has passed in a bad address.
	 	 */

		u.u_error = copyin(uap->tupleset, acl_user,
			uap->ntuples * sizeof (struct acl_tuple_user));
#ifdef AUDIT
		if (AUDITEVERON())
			save_acl((char *)acl_user,
				uap->ntuples * sizeof(struct acl_tuple_user));
#endif AUDIT
		if (u.u_error)
		{
			return;
		}

		for (i=0, tpu=acl_user; i<uap->ntuples; i++, tpu++)
		{
			if ((tpu->uid >= MAXUID && tpu->uid != ACL_NSUSER) ||
			    (tpu->gid >= MAXUID && tpu->gid != ACL_NSGROUP)) {
				u.u_error = EINVAL;
				return;
			}
			setacl_ops.acl[i].uid = tpu->uid;
			setacl_ops.acl[i].gid = tpu->gid;
			setacl_ops.acl[i].mode = tpu->mode;
		}

	} else {
#ifdef AUDIT
		/* set the pointer to the tuple parameters to null */
		u.u_audxparam = NULL;
#endif AUDIT
	}

	/* As part of the lookup we pass along the information needed to
	 * perform the actual operation. This is only used on a diskless
	 * node to save the cost of an extra message.
	 * ERRORS that can be generated are ENOTDIR, ENOENT, EACCES,
	 * EFAULT, ENFILE. On a diskless node all possible errors can
	 * be generated.
	 */
	u.u_error = lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_SETACL, &setacl_ops);
	if (u.u_error)
		return;

	VOP_SETACL(vp,uap->ntuples,setacl_ops.acl);
	VN_RELE(vp);
}

/*
 * F S E T A C L
 *
 * This is the fsetacl system call
 * It copies in the data, gets the vnode and calls the vnodeop to handle it
 */

fsetacl(uap)
	register struct a {
		int	fd;			/* a file descriptor */
		int	ntuples;		/* number of tuples to set */
		struct  acl_tuple_user *tupleset; /* addr of the acl */
	} *uap;
{
	register struct vnode *vp;
	struct file *fp;
	struct acl_tuple acl[NACLTUPLES];
	struct acl_tuple_user acl_user[NACLTUPLES];
	register int i;
	register struct acl_tuple *tp;
	register struct acl_tuple_user *tpu;

	/* The user cannot set more than NACLTUPLES tuples */

	if (uap->ntuples > NACLTUPLES)
	{
		u.u_error = EINVAL;
		return;
	}

	/* If the user specifies < 0 tuples, skip what follows.
	 * Just lookup the name and broadcast the operation, passing
	 * on the negative number of tuples.
	 */

	if (uap->ntuples >= 0) {

		/* copy the acl from user space to kernel space. EFAULT is
	 	 * generated if the user has passed in a bad address.
	 	 */

		u.u_error = copyin(uap->tupleset, acl_user,
			uap->ntuples * sizeof (struct acl_tuple_user));
#ifdef AUDIT
		if (AUDITEVERON())
			save_acl((char *)acl_user,
				uap->ntuples * sizeof(struct acl_tuple_user));
#endif AUDIT
		if (u.u_error)
			return;

		for (i=0, tp=acl, tpu=acl_user;
			i<uap->ntuples; i++, tp++, tpu++) {

			if ((tpu->uid >= MAXUID && tpu->uid != ACL_NSUSER) ||
		    	(tpu->gid >= MAXUID && tpu->gid != ACL_NSGROUP)) {
				u.u_error = EINVAL;
				return;
			}
			tp->uid = tpu->uid;
			tp->gid = tpu->gid;
			tp->mode = tpu->mode;
		}

	} else {
#ifdef AUDIT
		/* set the pointer to the tuple parameters to null */
		u.u_audxparam = NULL;
#endif AUDIT
	}

	/* get the vnode. EBADF can be generated here */

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error) {
			return;
	}
	vp = (struct vnode *)fp->f_data;
	VOP_SETACL(vp,uap->ntuples,acl);
}

/*
 * G E T A C L
 *
 * This is the getacl system call
 * It sets up the lookup structure, gets the vnode and calls
 * the vnodeop to handle it.
 */

getacl(uap)
	register struct a {
		char	*fname;
		int	ntuples;
		struct	acl_tuple_user *tupleset;
	} *uap;
{
	struct vnode *vp;
	struct getaclops getacl_ops;	/* structure used to pass information */
					/* used by lookup */

	getacl_ops.ntuples = uap->ntuples;
	getacl_ops.tupleset = uap->tupleset;

	/* As part of the lookup we pass along the information needed to
	 * perform the actual operation. This is only used on a diskless
	 * node to save the cost of an extra message.
	 * ERRORS that can be generated are ENOTDIR, ENOENT, EACCES,
	 * EFAULT, ENFILE. On a diskless node all possible errors can
	 * be generated.
	 */

	u.u_error = lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_GETACL, &getacl_ops);
	if (u.u_error)
		return(-1);
	VOP_GETACL(vp,uap->ntuples,uap->tupleset);
	VN_RELE(vp);
	return 0;
}

/*
 * F G E T A C L
 *
 * This is the fgetacl system call
 * It gets the vnode and calls the vnodeop to handle it
 */

fgetacl(uap)
	register struct a {
		int	fd;
		int	ntuples;
		struct	acl_tuple_user *tupleset;
	} *uap;
{
	struct file *fp;
	register struct vnode *vp;

	/* get the vnode. EBADF can be generated here */

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error) {
		return;
	}
	vp = (struct vnode *)fp->f_data;
	VOP_GETACL(vp,uap->ntuples,uap->tupleset);
}
#endif	ACLS

fsctl(uap)
	register struct a {
		int	fd;
		int	command;
		char	*buf;
		int	count;
	} *uap;
{
	struct file *fp;
	struct uio auio;
	struct iovec aiov;

	fp = getf(uap->fd);
	if (fp == NULL) return;
	if ((fp->f_flag & FREAD) == 0) {
		u.u_error = EBADF;
		return;
	}
	aiov.iov_base = uap->buf;
	if((aiov.iov_len = uap->count) < 0) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_seg = UIOSEG_USER;
	auio.uio_resid = uap->count;
	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = EINVAL;
		return;
	}
	PSEMA(&filesys_sema);
	u.u_error = VOP_FSCTL((struct vnode *)fp->f_data, uap->command,
				 &auio, fp->f_cred);
	VSEMA(&filesys_sema);
	if (u.u_error) return;
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	SPINLOCK(file_table_lock);
	fp->f_offset = auio.uio_offset;
	SPINUNLOCK(file_table_lock);
}
