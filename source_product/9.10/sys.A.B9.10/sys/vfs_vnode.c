/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_vnode.c,v $
 * $Revision: 1.10.83.4 $	$Author: kcs $
 * $State: Exp $      $Locker:  $
 * $Date: 93/09/17 20:16:43 $
 */

/* HPUX_ID: @(#)vfs_vnode.c	55.2		88/12/28 */

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

/*      NFSSRC @(#)vfs_vnode.c	2.1 86/04/15 */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/pathname.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"

#include "../dux/lookupops.h"
#include "../dux/lkup_dep.h"

#include "../h/kern_sem.h"

#ifdef AUDIT
#include "../h/audit.h"
#endif AUDIT

#include "../h/debug.h"


extern struct fileops vnodefops;

/*
 * read or write a vnode
 */
int
vn_rdwr(rw, vp, base, len, offset, seg, ioflag, aresid, fpflags)
	enum uio_rw rw;
	struct vnode *vp;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int ioflag;
	int *aresid;
        int fpflags;
{
	struct uio auio;
	struct iovec aiov;
	int error;
#ifdef	MP
	sv_sema_t vn_rdwrSS;
#endif

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_seg = seg;
	auio.uio_resid = len;
	auio.uio_fpflags = fpflags;
	PXSEMA(&filesys_sema,&vn_rdwrSS);
	error = VOP_RDWR(vp, &auio, rw, ioflag, u.u_cred);
	VXSEMA(&filesys_sema,&vn_rdwrSS);
	if (aresid)
		*aresid = auio.uio_resid;
	else
		if (auio.uio_resid)
			error = EIO;
	return (error);
}

/*
 * release a vnode. Decrements reference count and
 * calls VOP_INACTIVE on last.
 */
void
vn_rele(vp)
	register struct vnode *vp;
{
	int count;
#ifdef	MP
	sv_sema_t vn_releSS;
#endif
	SPINLOCK(v_count_lock);
	count = --vp->v_count;
	SPINUNLOCK(v_count_lock);

#ifdef OSDEBUG
	if (count < vp->dnlc_count) {
		printf("vnode = 0x%x\n", vp);
		panic("count < dnlc_count");
	}
#endif
	if (vp->v_fstype == VDUX) {
		VASSERT(vp->v_count >= VTOI(vp)->i_refcount.d_rcount);
	}

	/*
	 * sanity check
	 * We now check to see if v_count is < 0, this is
	 * helpful in detecting reference count overruns,
	 * now that v_count is declared as a signed short.
	 */
	if (count == -1) {
#ifdef OSDEBUG
		printf("vnode = %x\n", vp);
#endif
		panic("vn_rele");
	}
	if (count == 0) {
		PXSEMA(&filesys_sema,&vn_releSS);
		(void)VOP_INACTIVE(vp, u.u_cred);
		VXSEMA(&filesys_sema,&vn_releSS);
	}
}

#ifdef OSDEBUG
/*
 * Hold a vnode. Only for validation when OSDEBUG defined.
 */
void
vn_hold(vp)
	register struct vnode *vp;
{
	if (vp->v_fstype == VUFS) {
		if (!(VTOI(vp)->i_flag & IREF)) {
			printf("vnode = 0x%x\n", vp);
			panic("vn_hold on unreferenced inode");
		}
	}
	vp->v_count++;
}
#endif

/*
 * Open/create a vnode.
 * This may be callable by the kernel, the only known side effect being that
 * the current user uid and gid are used for permissions.
 */
#ifdef hp9000s800
/*
 * If you are calling vn_open without an allocated u.u_ofile table entry,
 * then call vn_open with u.u_r.r_val1 with an invalid (e.g. -1) value.
 */
#endif /* hp9000s800 */
int
vn_open(pnamep, seg, filemode, createmode, vpp)
	char *pnamep;
	register int filemode;
	int createmode;
	struct vnode **vpp;
{
	struct vnode *vp;		/* ptr to file vnode */
	register int mode;
	register int error;

	mode = 0;
	if (filemode & FREAD)
		mode |= VREAD;
	if (filemode & (FWRITE | FTRUNC))
		mode |= VWRITE;

	if (filemode & FCREAT) {
		struct vattr vattr;
		enum vcexcl excl;

		/*
		 * Wish to create a file.
		 */
		vattr_null(&vattr);
		vattr.va_type = VREG;
		vattr.va_mode = (createmode&07777&(~VSVTX));
		if (filemode & FTRUNC)
			vattr.va_size = 0;
		if (filemode & FEXCL)
			excl = EXCL;
		else
			excl = NONEXCL;
		filemode &= ~(FCREAT | FTRUNC | FEXCL);

		error = vn_create(pnamep, seg, &vattr, excl, mode, &vp,
			LKUP_CREATE, filemode);

		if (error)
			goto out;
	} else {
		/*
		 * Wish to open a file.
		 * Just look it up.
		 */
		struct copenops openops;

		openops.mode = mode;
		openops.filemode = filemode;
		openops.vap = NULL;
		openops.compvpp = &vp;	/*kludge*/
		/* Tell dux_copen_unpack() that we're an open() */
		openops.is_open = 1;

		error =
		    lookupname(pnamep, seg, FOLLOW_LINK,
			(struct vnode **)0, &vp,
			LKUP_OPEN,(caddr_t)&openops);
		if (error)
			goto out;
		error = vns_open(&vp,filemode,mode);
	}

	if (!error)
		error = vns_copen(&vp,filemode);
	if (error) {
		VN_RELE(vp);
	} else {
		*vpp = vp;
	}
out:
	/*
	 *If the pathname went remote, we already have the
	 *vnode set up, and EOPCOMPLETE was returned.  So
	 *just return the vnode, along with an error code of
	 *0
	 */
	if (error == EOPCOMPLETE)
	{
		*vpp = vp;
		return (0);
	}
	else
		return (error);
}

/*Server code for open*/
int
vns_open(vpp,filemode,mode)
struct vnode **vpp;
int filemode;
int mode;
{
	int error;
	register struct vnode *vp = *vpp;

	/*
	 * cannnot write directories, active texts or
	 * read only filesystems
	 */
	if (filemode & (FWRITE | FTRUNC)) {
		if (vp->v_type == VDIR) {
			return (EISDIR);
		}
		/*
		 * If there's shared text associated with
		 * the vnode, try to free it up once.
		 * If we fail, we can't allow writing.
		 */
		if (vp->v_flag & VTEXT) {
			xrele(vp);
			if (vp->v_flag & VTEXT) {
				return (ETXTBSY);
			}
		}
	}
	/*
	 * check permissions
	 */
	error = VOP_ACCESS(vp, mode, u.u_cred);
	if (error)
		return (error);
	/*
	 * Sockets in filesystem name space are not supported (yet?)
	 */
	if (vp->v_type == VSOCK) {
		return (EOPNOTSUPP);
	}
	return (error);
}

/*Server code common to both open and create*/
int
vns_copen(vpp,filemode)
register struct vnode **vpp;
register int filemode;
{
	int error;
	int is_fifo;
	struct inode *ip;

	/*
	 * do opening protocol.
	 */
	error = VOP_OPEN(vpp, filemode, u.u_cred);
	/*
	 * truncate if required
	 */
	if ((error == 0) && (filemode & FTRUNC)) {
		struct vattr vattr;

		filemode &= ~FTRUNC;
		vattr_null(&vattr);
		vattr.va_size = 0;
		error = VOP_SETATTR(*vpp, &vattr, u.u_cred, 0);  /* kludge !*/
		if (error && (*vpp)->v_fstype == VUFS) {
			ip = VTOI(*vpp);
			is_fifo = ((ip->i_mode & IFMT) == IFIFO);
			isynclock(ip);
			updatesitecount(&(ip->i_opensites), u.u_site, -1);
			if (filemode & FWRITE)
				updatesitecount(&(ip->i_writesites),
				   u.u_site, -1);
			if (is_fifo && (filemode & FREAD))
				updatesitecount(&(ip->i_fifordsites),
				   u.u_site, -1);
			checksync(ip);
			isyncunlock(ip);
		}
	}
	return (error);
}


/*
 * create a vnode (makenode)
 */
int
vn_create(pnamep, seg, vap, excl, mode, vpp, opcode, filemode)
	char *pnamep;
	int seg;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct vnode **vpp;
	int opcode;
	int filemode;	/*meaningful if opcode is LKUP_CREATE*/
{
	struct vnode *dvp;	/* ptr to parent dir vnode */
	struct pathname pn;
	register int error;
	struct copenops createops;
	extern int hpux_aes_override; /* kludge to configure out AES changes */
        register enum symfollow link_follow;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(pnamep);
	}
#endif /* AUDIT */

	/*
	 * Lookup directory.
	 * If new object is a file, call lower level to create it.
	 * Note that it is up to the lower level to enforce exclusive
	 * creation, if the file is already there.
	 * This allows the lower level to do whatever
	 * locking or protocol that is needed to prevent races.
	 * If the new object is directory call lower level to make
	 * the new directory, with "." and "..".
	 */
	dvp = (struct vnode *)0;
	*vpp = (struct vnode *)0;	
	error = pn_get(pnamep, seg, &pn);
	if (error)
		return (error);
	/*
	 * lookup will find the parent directory for the vnode.
	 * When it is done the pn hold the name of the entry
	 * in the directory.
	 * If this is a non-exclusive create we also find the node itself.
	 */
	createops.vap = vap;
	createops.excl = excl;
	createops.mode = mode;
	createops.filemode = filemode;
	createops.compvpp = vpp;		/*kludge*/
	/* Tell dux_copen_unpack() that we're a creat() */
	createops.is_open = 0;			/*worse kludge*/

        /*
         * For OSF1 AES rev A conformance, mkdir() and mkfifo() will
         * not traverse a symbolic link when it is the last element
         * of the path. For open() with O_CREAT set, the
         * symbolic link will NOT be traversed if O_EXCL is set, and
         * will be traversed otherwise. 
         *
         * The astute observer will also notice that the directory and
         * fifo creating ability of mknod() will also be affected.
         *
         * Golly, I sure hope those OSF guys know what the're doing. 
         *
         */

        if( !hpux_aes_override && 
            ( vap->va_type == VFIFO || 
              vap->va_type == VDIR  || vap->va_type == VEMPTYDIR ||
              ((vap->va_type == VREG) && (excl == EXCL))
            )
        ){
           link_follow = NO_FOLLOW;
        }
        else {
           link_follow = FOLLOW_LINK;
        }

	error = lookuppn(&pn, link_follow, &dvp,
	    (excl == NONEXCL? vpp: (struct vnode **)0),
	    opcode, (caddr_t)&createops);
	if (!error)
	{
		error = vns_create(&pn, dvp, vap, excl, mode, vpp);
	}
	pn_free(&pn);
	return (error);
}

vns_create(pnp, dvp, vap, excl, mode, vpp)
	struct pathname *pnp;
	struct vnode *dvp;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct vnode **vpp;
{
	int error=0;

	sv_sema_t vn_createSS;
	PXSEMA(&filesys_sema, &vn_createSS);
	/*
	 * Make sure filesystem is writeable
	 */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		if (*vpp) {
			VN_RELE(*vpp);
		}
		error = EROFS;
	} else if (excl == NONEXCL && *vpp != (struct vnode *)0) {
		/*
		 * The file is already there.
		 * If we are writing, and there's a shared text
		 * associated with the vnode, try to free it up once.
		 * If we fail, we can't allow writing.
		 */
		if ((mode & VWRITE) && ((*vpp)->v_flag & VTEXT)) {
			xrele(*vpp);
			if ((*vpp)->v_flag & VTEXT) {
				error = ETXTBSY;
			}
		}
		/*
		 * we throw the vnode away to let VOP_CREATE truncate the
		 * file in a non-racy manner.
		 */
		VN_RELE(*vpp);
	}
	if (error == 0) {
		/*
		 * call mkdir if directory or create if other
		 */
		if ((vap->va_type == VDIR) || (vap->va_type == VEMPTYDIR)) {
			error = VOP_MKDIR(dvp, pnp->pn_path, vap, vpp, u.u_cred);
		} else {
			error = VOP_CREATE(
			    dvp, pnp->pn_path, vap, excl, mode, vpp, u.u_cred);
		}
	}
	update_duxref(dvp,-1,0);
	VN_RELE(dvp);
	VXSEMA(&filesys_sema, &vn_createSS);
	return (error);
}

/*
 * close a vnode
 */
int
vn_close(vp, flag)
register struct vnode *vp;
int flag;
{

	return (VOP_CLOSE(vp, flag, u.u_cred));
}

/*
 * Link.
 */
int
vn_link(from_p, to_p, seg)
	char *from_p;
	char *to_p;
	int seg;
{
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct pathname pn;
	register int error;
	enum symfollow follow_mode;
	extern int hpux_aes_override;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(from_p);
		(void)save_pn_info(to_p);
	}
#endif /* AUDIT */

	fvp = tdvp = (struct vnode *)0;
	error = pn_get(to_p, seg, &pn);
	if (error)
		return (error);
retry:
	error = lookupname(from_p, seg, FOLLOW_LINK, (struct vnode **)0, &fvp,
		LKUP_LOOKUP, NULL);
	if (error)
		goto out;

	if( hpux_aes_override )
		follow_mode = FOLLOW_LINK;
	else 
		follow_mode = NO_FOLLOW;

	 error = lookuppn(&pn, follow_mode, &tdvp, (struct vnode **)0,
		   LKUP_LINK, fvp);
	if (error) {
		if (error == (EREMOVE & 0xffff)) {
			/*Decrement the dux reference dcount*/
			update_duxref(fvp,-1,0);
			VN_RELE(fvp);
			if (tdvp) VN_RELE(tdvp);
			fvp = tdvp = (struct vnode *)0;
			pn_reset(&pn);
			goto retry;
		}
		goto out;
	}
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
	 * For AES conformance, we must fail if target is a symbolic link
	 */
	if( ! hpux_aes_override ){
		if( tdvp->v_type == VLNK){
			error = EEXIST;
			goto out;
		}
	}
	/*
	 * do the link
	 */
	error = VOP_LINK(fvp, tdvp, pn.pn_path, u.u_cred);
out:
	pn_free(&pn);
	if (fvp)
	{
		/*Decrement the dux reference dcount*/
		update_duxref(fvp,-1,0);
		VN_RELE(fvp);
	}
	if (tdvp)
		VN_RELE(tdvp);
	return (error);
}

/*
 * Rename.
 */
int
vn_rename(from_p, to_p, seg)
	char *from_p;
	char *to_p;
	int seg;
{
	struct vnode *fdvp;		/* from directory vnode ptr */
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct vnode *tvp;		/* to vnode ptr */
	struct pathname fpn;		/* from pathname */
	struct pathname tpn;		/* to pathname */
	register int error;

	fvp = fdvp = tdvp = tvp = (struct vnode *)0;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(from_p);
		(void)save_pn_info(to_p);
	}
#endif /* AUDIT */

	/*
	 * get to and from pathnames
	 */
	error = pn_get(from_p, seg, &fpn);
	if (error)
		return (error);

	error = pn_get(to_p, seg, &tpn);
	if (error) {
		pn_free(&fpn);
		return (error);
	}

	/*
	 * lookup to and from directories
	 */
	error = lookuppn(&fpn, NO_FOLLOW, &fdvp, &fvp, LKUP_LOOKUP, (caddr_t)0);
	if (error) {
		/*
		 * locallookuppn will set error to EISDIR if we try to
		 * look up "/" with a non-null dvp, which is what we will
		 * do if we're trying to rename "/".  Renaming "/" should
		 * give EBUSY, not EISDIR, so change error here.
		 */
		if (error == EISDIR)
			error = EBUSY;
		goto out;
	}
	/*
	 * make sure there is an entry
	 */
	if (fvp == (struct vnode *)0) {
		error = ENOENT;
		goto out;
	}

	error = lookuppn(&tpn, NO_FOLLOW, &tdvp, &tvp, LKUP_LOOKUP, (caddr_t) 0);
	if (error) {
		/*
		 * locallookuppn will set error to EISDIR if we try to
		 * look up "/" with a non-null dvp, which is what we will
		 * do if we're trying to rename "/".  Renaming "/" should
		 * give EBUSY, not EISDIR, so change error here.
		 */
		if (error == EISDIR)
			error = EBUSY;
		goto out;
	}

	if (tvp != (struct vnode*)0) {
		/* if "to" or "from" are the root of a mount file system,
		   exit with EBUSY*/
		if ((tvp -> v_flag & VROOT) || (fvp -> v_flag & VROOT)) {
			error = EBUSY;
		}
		/* if "from" and "to" are in different file system, exit with
		   EXDEV*/
		else if (fvp -> v_vfsp != tvp -> v_vfsp) {
			error=EXDEV;
		}
		/*
		 * If there's shared text associated with
		 * the vnode, try to free it up once.
		 * If we fail, we can't allow writing.
		 */
		else if (tvp->v_flag & VTEXT) {
			xrele(tvp);
			if (tvp->v_flag & VTEXT) {
				error = ETXTBSY;
			}
		}
		/*
		 * don't know why NFS has to do pathname lookup twice, one here.
		 * The other in ufs_rename().  Since I have to pass the same
		 * number of args to it, I can't change it.  I simply release
		 * the vnode here.
		 */
		update_duxref(tvp, -1, 0);
		VN_RELE(tvp);
		if (error) goto out;
	}

	/*
	 * Make sure both the from vnode and the to directory are
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
	 * do the rename
	 */
	error = VOP_RENAME(fdvp, fpn.pn_path, tdvp, tpn.pn_path, u.u_cred);
out:
	pn_free(&fpn);
	pn_free(&tpn);
	if (fvp)
	{
		update_duxref (fvp, -1, 0);
		VN_RELE(fvp);
	}
	if (fdvp)
	{
		update_duxref (fdvp, -1, 0);
		VN_RELE(fdvp);
	}
	if (tdvp)
	{
		update_duxref (tdvp, -1, 0);
		VN_RELE(tdvp);
	}
	return (error);
}

/*
 * remove a file or directory.
 */
int
vn_remove(fnamep, seg, dirflag)
	char *fnamep;
	int seg;
	enum rm dirflag;
{
	struct vnode *vp;		/* entry vnode */
	struct vnode *dvp;		/* ptr to parent dir vnode */
	struct pathname pn;		/* name of entry */
	register int error;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(fnamep);
	}
#endif /* AUDIT */

	error = pn_get(fnamep, seg, &pn);
	if (error)
		return (error);
	dvp = vp = (struct vnode *)0;

	/* check to see if we are removing the current directory for SVVS 3.
	   Note that the rmdir routine for dux should be changed to get the
	   vnode as a parameter, but headers are frozen so we cannot add the
	   vnode parameter to the remove_request.  Sigh.
	   Also, by having two lookups there is a race condition such that if
	   a component of the path changed between the lookups it might be
	   possible to rmdir() your current directory.  The window is small
	   and the failure is not critical.
	*/

	if (dirflag) /* was called by rmdir so do additional checking for svvs*/
	{
		error = lookupname(fnamep, seg, NO_FOLLOW, (struct vnode *)0,
				   &vp, LKUP_LOOKUP, (caddr_t *)0);
		if ((error != 0) && (error != EPATHREMOTE))
		{
			pn_free(&pn);
			return(error);
		}
		update_duxref(vp,-1,0); /* free the server vnode if needed */
		VN_RELE(vp); 		/* free the vnode locally */
		if (vp == u.u_cdir)
		{
			pn_free(&pn);
			return(EINVAL);
		}
	}
	error = lookuppn(&pn, NO_FOLLOW, &dvp, &vp, LKUP_REMOVE, &dirflag);
	if (error) {
		pn_free(&pn);
		return (error);
	}
	error = vns_remove(vp, dvp, &pn, dirflag);
	pn_free(&pn);
	return (error);
}

vns_remove(vp, dvp, pnp, dirflag)
	struct vnode *vp;	/*entry vnode*/
	struct vnode *dvp;	/*parent dir vnode*/
	struct pathname *pnp;
	enum rm dirflag;
{
	enum vtype vtype;
	register int error;
	struct vattr vattr;
	/*
	 * make sure there is an entry
	 */
	if (vp == (struct vnode *)0) {
		error = ENOENT;
		goto out;
	}

	/*
	 * don't unlink the root of a mounted filesystem.  If vfs of dvp
	 * is not the same as vp's, we know we are trying to unlink
	 * a mount point.  Otherwise, it is a link to a mount point.
	 * We therefore allow it to be removed.
	 */
	if ((vp->v_flag & VROOT) && (vp -> v_vfsp != dvp -> v_vfsp)) {
		error = EBUSY;
		goto out;
	}

	/*
	 * make sure filesystem is writeable
         * moved down because SVVS.3 expects mount point test first.
	 */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}

	/*
	 * If there's shared text associated with
	 * the vnode, try to free it up once.
	 * If we fail, we can't allow writing.
	 */
	if (vp->v_flag & VTEXT) {
		error = VOP_GETATTR(vp, &vattr, u.u_cred, VASYNC);
		if (error)
			goto out;
		xrele(vp);
		if (vp->v_flag & VTEXT && vattr.va_nlink == 1) {
			error = ETXTBSY;
			goto out;
		}
	}
	/*
	 * release vnode before removing
	 */
	vtype = vp->v_type;
	VN_RELE(vp);
	vp = (struct vnode *)0;
	if (vtype == VDIR) {
		/*
		 * if caller thought it was removing a directory, go ahead
		 */
		if (dirflag == DIRECTORY)
			error = VOP_RMDIR(dvp, pnp->pn_path, u.u_cred);
		else {
			if (suser())
				error = VOP_REMOVE(dvp, pnp->pn_path, u.u_cred);
			else
			error = EPERM;
		}
	} else {
		/*
		 * if caller thought it was removing a directory, barf.
		 */
		if (dirflag == FILE)
			error = VOP_REMOVE(dvp, pnp->pn_path, u.u_cred);
		else
			error = ENOTDIR;
	}
out:
	/*Decrement the dux reference dcount, for the mount pt case.*/
	update_duxref(dvp,-1,0);
	if (vp != (struct vnode *)0)
		VN_RELE(vp);
	VN_RELE(dvp);
	return (error);
}

/*
 * Set vattr structure to a null value.
 * Boy is this machine dependent!
 */
void
vattr_null(vap)
struct vattr *vap;
{
	register int n;
	register char *cp;

	n = sizeof(struct vattr);
	cp = (char *)vap;
	while (n--) {
		*cp++ = -1;
	}
}

#ifdef DEBUG
prvnode(vp)
	register struct vnode *vp;
{

	printf("vnode vp=0x%x ", vp);
	printf("flag=0x%x,count=%d,shlcnt=%d,exclcnt=%d\n",
		vp->v_flag,vp->v_count,vp->v_shlockc,vp->v_exlockc);
	printf("	vfsmnt=0x%x,vfsp=0x%x,type=%d,dev=0x%x\n",
		vp->v_vfsmountedhere,vp->v_vfsp,vp->v_type,vp->v_rdev);
	printf ("	data=0x%x\n",vp->v_data);
	if (vp->v_fstype == VUFS)
	{
		printf ("ufs inode\n");
		ipcheck(VTOI(vp));
	}
	else if (vp->v_fstype == VDUX)
	{
		printf ("dux inode\n");
		ipcheck(VTOI(vp));
	}
}

prvattr(vap)
	register struct vattr *vap;
{

	printf("vattr: vap=0x%x ", vap);
	printf("type=%d,mode=0%o,uid=%d,gid=%d\n",
		vap->va_type,vap->va_mode,vap->va_uid,vap->va_gid);
	printf("fsid=%d,nodeid=%d,nlink=%d,size=%d,bsize=%d\n",
		vap->va_fsid,vap->va_nodeid,vap->va_nlink,
		vap->va_size,vap->va_blocksize);
	printf("atime=(%d,%d),mtime=(%d,%d),ctime=(%d,%d)\n",
		vap->va_atime.tv_sec,vap->va_atime.tv_usec,
		vap->va_mtime.tv_sec,vap->va_mtime.tv_usec,
		vap->va_ctime.tv_sec,vap->va_ctime.tv_usec);
	printf("rdev=0x%x, blocks=%d\n",vap->va_rdev,vap->va_blocks);
}
#endif DEBUG
