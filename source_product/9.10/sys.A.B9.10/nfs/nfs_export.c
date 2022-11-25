/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/nfs_export.c,v $
 * $Revision: 1.4.83.4 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 19:07:51 $
 */

/*	@(#)nfs_export.c	1.3 90/07/02 NFSSRC4.1 from 1.15 90/01/22 SMI 	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "../h/types.h"
#include "../h/param.h"
#include "../h/time.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/socket.h"
#include "../h/errno.h"
#include "../h/uio.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/pathname.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/auth_unix.h"
/*
#include "../rpc/auth_des.h"
*/
#include "../rpc/svc.h"
#define NFSSERVER
#include "../nfs/nfs.h"
#include "../nfs/export.h"
#include "../dux/lookupops.h"

#include "../h/net_diag.h"

#include "../h/kern_sem.h"      /* MPNET */
#include "../net/netmp.h"       /* MPNET */

#define eqfsid(fsid1, fsid2)	\
	(bcmp((char *)fsid1, (char *)fsid2, (int)sizeof(fsid_t)) == 0)

#define eqfid(fid1, fid2) \
	((fid1)->fid_len == (fid2)->fid_len && \
	bcmp((char *)(fid1)->fid_data, (char *)(fid2)->fid_data,  \
	(int)(fid1)->fid_len) == 0)

#define exportmatch(exi, fsid, fid) \
	(eqfsid(&(exi)->exi_fsid, fsid) && eqfid((exi)->exi_fid, fid))


#ifdef NFSDEBUG
extern int nfsdebug;
#endif

struct exportinfo *exported = NULL;	/* the list of exported filesystems */

/*
 * Exportfs system call
 */

exportfs(uap)
	register struct a {
		char *dname;
		struct export *uex;
	} *uap;
{
	struct vnode *vp;
	struct export *kex;
	struct exportinfo *exi;
	struct exportinfo *ex, *prev;
	struct fid *fid;
	struct vfs *vfs;
	int mounted_ro;
	char nfs_log_str_mess[100];	/* String used for NS logging */

	if (! suser()) {
		/* suser() sets u.u_error to EPERM */
		NS_LOG_STR(LE_NFS_ROOT_ONLY, NS_LC_WARNING, NS_LS_NFS, 0, "exportfs");
		return;
	}

	/*
	 * Get the vfs id
	 */
	u.u_error = lookupname(uap->dname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **) NULL, &vp, LKUP_LOOKUP, (caddr_t)0);
	if (u.u_error) {
		NS_LOG_STR(LE_NFS_LOOKUP_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
			"exportfs1 ");
		return;	
	}
	u.u_error = VOP_FID(vp, &fid);
	vfs = vp->v_vfsp;
	mounted_ro = isrofile(vp);

	/* Just in case it is a VDUX vnode */
	update_duxref(vp, -1, 0);
	VN_RELE(vp);
	if (u.u_error) {
		return;	
	}

	if (uap->uex == NULL) {
		u.u_error = unexport(vfs->vfs_fsid, fid);
		freefid(fid);
		return;
	}
	exi = (struct exportinfo *) mem_alloc(sizeof(struct exportinfo));
	exi->exi_flags = 0;
	exi->exi_refcnt = 0;
	exi->exi_fsid[0] = vfs->vfs_fsid[0];
	exi->exi_fsid[1] = vfs->vfs_fsid[1];
	exi->exi_fid = fid;
	kex = &exi->exi_export;

	/*
	 * Load in everything, and do sanity checking
	 */	
	u.u_error = copyin((caddr_t) uap->uex, (caddr_t) kex,
		(u_int) sizeof(struct export));
	if (u.u_error) {
		NS_LOG_STR(LE_NFS_COPYIN_FAIL, NS_LC_ERROR, NS_LS_NFS, 0,
			"exportfs: copyin");
		goto error_return;
	}
	if (kex->ex_flags & ~(EX_RDONLY | EX_RDMOSTLY | EX_ASYNC)) {
		u.u_error = EINVAL;
		sprintf(nfs_log_str_mess, sizeof(nfs_log_str_mess),
			"bad export flags 0x%04x ",kex->ex_flags);
		NS_LOG_STR(LE_NFS_BAD_FLAGS, NS_LC_WARNING, NS_LS_NFS, 0,
			nfs_log_str_mess);
		goto error_return;
	}
	if (!(kex->ex_flags & EX_RDONLY) && mounted_ro) {
		u.u_error = EROFS;
		sprintf(nfs_log_str_mess, sizeof(nfs_log_str_mess),
			"mounted R/O, bad ex_flags 0x%04x ", kex->ex_flags);
		NS_LOG_STR(LE_NFS_BAD_FLAGS, NS_LC_WARNING, NS_LS_NFS, 0,
			nfs_log_str_mess);
		goto error_return;
	}
	if (kex->ex_flags & EX_RDMOSTLY) {
		u.u_error = loadaddrs(&kex->ex_writeaddrs);
		if (u.u_error) {
			NS_LOG_STR(LE_NFS_LOAD_ADDR, NS_LC_WARNING, NS_LS_NFS,
				0, "RDMOSTLY, loadaddrs fail");
			goto error_return;
		}
	}
	switch (kex->ex_auth) {
	case AUTH_UNIX:
		u.u_error = loadaddrs(&kex->ex_unix.rootaddrs);
		if (u.u_error)
			strcpy(nfs_log_str_mess, "AUTH_UNIX, loadaddrs fail");
		break;
#ifdef notdef
	case AUTH_DES:
		u.u_error = loadrootnames(kex);
		if (u.u_error)
			strcpy(nfs_log_str_mess, "AUTH_DES, loadrootnames fail");
		break;
#endif /* notdef */
	default:
		u.u_error = EINVAL;
		sprintf(nfs_log_str_mess, sizeof(nfs_log_str_mess),
			"unknown ex_auth [ 0x%04x ] loadaddrs fail", kex->ex_auth);
	}

	if (u.u_error) {	
		NS_LOG_STR(LE_NFS_LOAD_ADDR, NS_LC_WARNING, NS_LS_NFS, 0,
			 nfs_log_str_mess);
		goto error_return;
	}

	/*
	 * Insert the new entry at the front of the export list
	 */
	exi->exi_next = exported;
	exported = exi;

	/*
	 * Check the rest of the list for an old entry for the fs.
	 * If one is found then unlink it, wait until its refcnt
	 * goes to 0 and free it.
	 */
	prev = exported;
	for (ex = exported->exi_next; ex != NULL; prev = ex, ex = ex->exi_next){
		if (exportmatch(ex, exi->exi_fsid, exi->exi_fid)) {
			prev->exi_next = ex->exi_next;
			while (ex->exi_refcnt > 0) {
				ex->exi_flags |= EXI_WANTED;
				(void) sleep((caddr_t)ex, PZERO+1);
			}
			exportfree(ex);
			break;
		}
	}
	return;

error_return:	
	freefid(exi->exi_fid);
	mem_free((char *) exi, sizeof(struct exportinfo));
}


/*
 * Remove the exported directory from the export list
 */
unexport(fsid, fid)
	fsid_t *fsid;
	struct fid *fid;
{
	struct exportinfo **tail;	
	struct exportinfo *exi;

	tail = &exported;
again:
	while (*tail != NULL) {
		if (exportmatch(*tail, fsid, fid)) {
			exi = *tail;
			while (exi->exi_refcnt > 0) {
				exi->exi_flags |= EXI_WANTED;
				(void) sleep((caddr_t)exi, PZERO+1);
				goto again;
			}
			*tail = (*tail)->exi_next;
			exportfree(exi);
			return (0);
		} else {
			tail = &(*tail)->exi_next;
		}
	}
	NS_LOG_STR(LE_NFS_UNEXPORT_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
		 "unexport failed");
	return (EINVAL);
}

/*
 * Get file handle system call.
 * Takes file name and returns a file handle for it.
 * Also recognizes the old getfh() which takes a file
 * descriptor instead of a file name, and does the
 * right thing. This compatibility will go away in 5.0.
 * It goes away because if a file descriptor refers to
 * a file, there is no simple way to find its parent
 * directory.
 */
nfs_getfh(uap)
	register struct a {
		char *fname;
		fhandle_t   *fhp;
	} *uap;
{
	register struct file *fp;
	fhandle_t fh;
	struct vnode *vp;
	struct vnode *dvp;
	struct exportinfo *exi;	
	int error;
	int oldgetfh = 0;
#ifdef MP
	sv_sema_t savestate;            /* MPNET: MP save state */
#endif /* MP */
	int oldlevel;


	if (!suser()) {
		/* u.u_error = EPERM; in suser() */
		NS_LOG_STR(LE_NFS_ROOT_ONLY, NS_LC_WARNING, NS_LS_NFS, 0, "nfs_getfh");
		return;
	}
	if ((u_int)uap->fname < maxfiles_lim) {
		/*
		 * old getfh()
		 */
		oldgetfh = 1;
		fp = getf((int)uap->fname);
		if (fp == NULL) {
			char nfs_log_str_mess[100];
			u.u_error = EBADF;

			sprintf(nfs_log_str_mess, sizeof(nfs_log_str_mess),
				"nfs_getfh:  old style bad fd = %d ", (int)uap->fname);
			NS_LOG_STR(LE_NFS_GETFH_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
				nfs_log_str_mess);
			return;
		}
		vp = (struct vnode *)fp->f_data;
		dvp = NULL;
	} else {
		/*
		 * new getfh()
		 */
		dvp = vp = NULL;
		u.u_error = lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
				       &dvp, &vp, LKUP_LOOKUP, (caddr_t)0);
		if (u.u_error == EISDIR) {
			/*
			 * if fname resolves to / we get EEXIST error
			 * since we wanted the parent vnode. Try again
		 	 * with NULL dvp.
			 * HPNFS: We get a EISDIR. cwb
			 */
			u.u_error = lookupname(uap->fname, UIOSEG_USER,
				FOLLOW_LINK, (struct vnode **)NULL, &vp,
				LKUP_LOOKUP, (caddr_t)0);
			dvp = NULL;
			if (u.u_error) {
				NS_LOG_STR(LE_NFS_LOOKUP_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
					"nfs_getfh1 ")
			}
                } else if (u.u_error) {
                        NS_LOG_STR(LE_NFS_LOOKUP_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
                                "nfs_getfh2 ");
                }
		if (u.u_error == 0 && vp == NULL) {
			/*
			 * Last component of fname not found
			 */
			if (dvp) {
				update_duxref(dvp, -1, 0);
				VN_RELE(dvp);
			}
			u.u_error = ENOENT;
			NS_LOG_STR(LE_NFS_LOOKUP_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
							 "nfs_getfh3 ");
		}
		if (u.u_error) {
			return;
		}
	}
	error = findexivp(&exi, dvp, vp);
        if (error) {
                NS_LOG_STR(LE_NFS_FINDEXIVP_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
                        "nfs_getfh: findexivp");
        } else {
                PSEMA(&filesys_sema);
		error = makefh(&fh, vp, exi);
                VSEMA(&filesys_sema);
                if (error) {
                        NS_LOG_STR(LE_NFS_MAKEFH_FAIL, NS_LC_WARNING, NS_LS_NFS, 0,
                                "nfs_getfh4 ");
                } else if (error = copyout((caddr_t)&fh, (caddr_t)uap->fhp,sizeof(fh)))
                {
                        NS_LOG_STR(LE_NFS_COPYOUT_FAIL, NS_LC_ERROR, NS_LS_NFS, 0,
                                "nfs_getfh: copyout");
                }
	}
	if (!oldgetfh) {
		/*
		 * new getfh(): release vnodes
		 */
		update_duxref(vp, -1, 0);
		VN_RELE(vp);
		if (dvp != NULL) {
			update_duxref(dvp, -1, 0);
			VN_RELE(dvp);
		}
	}
	u.u_error = error;

        /*
         * Hack time.  PC-NFS starts over with a XID of zero on each reboot.
         * This causes problems because we use the XID (in part) to detect
         * duplicate requests.  In particular, this caused a data corruption
         * problem because we thought we had a duplicate write request, when
         * we really didn't.  To get around this, we flush all write requests
         * from the dup cache.  Ideally, this would be done based on IP addr
         * also, but we don't have that at this point in time.  We do this
         * here since getfh() is called from rpc.mountd when a remote system
         * mounts, which is the best shot we have a when an XID might get
         * reused by PC-NFS (have to remount after a reboot).
         */

        /*MPNET: turn MP protection on */
        NETMP_GO_EXCLUSIVE(oldlevel, savestate);

        svckudp_dupflush( RFS_WRITE, 0);
        NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
        /*MPNET: MP protection is now off. */
}

/*
 * Common code for both old getfh() and new getfh().
 * If old getfh(), then dvp is NULL.
 * Strategy: if vp is in the export list, then
 * return the associated file handle. Otherwise, ".."
 * once up the vp and try again, until the root of the
 * filesystem is reached.
 */
findexivp(exip, dvp, vp)
	struct exportinfo **exip;
	struct vnode *dvp;  /* parent of vnode want fhandle of */
	struct vnode *vp;   /* vnode we want fhandle of */
{
	struct fid *fid;
	int error;

	VN_HOLD(vp);
	update_duxref(vp, 1, 0);
	if (dvp != NULL) {
		VN_HOLD(dvp);
		update_duxref(dvp, 1, 0);
	}
	for (;;) {
		error = VOP_FID(vp, &fid);
		if (error) {
			break;
		}
		*exip = findexport(vp->v_vfsp->vfs_fsid, fid);
		freefid(fid);
		if (*exip != NULL) {
			/*
			 * Found the export info
			 */
			error = 0;
			break;
		}

		/*
		 * We have just failed finding a matching export.
		 * If we're at the root of this filesystem, then
		 * it's time to stop (with failure).
		 */
		if (vp->v_flag & VROOT) {
			error = EINVAL;
			break;	
		}

		/*
		 * Now, do a ".." up vp. If dvp is supplied, use it,
	 	 * otherwise, look it up.
		 */
		if (dvp == NULL) {
			error = VOP_LOOKUP(vp, "..", &dvp, u.u_cred, vp);
			if (error) {
				break;
			}
		}
		update_duxref(vp, -1, 0);
		VN_RELE(vp);
		vp = dvp;
		dvp = NULL;
	}
	update_duxref(vp, -1, 0);
	VN_RELE(vp);
	if (dvp != NULL) {
		update_duxref(dvp, -1, 0);
		VN_RELE(dvp);
	}
	return (error);
}

/*
 * Make an fhandle from a vnode
 */
makefh(fh, vp, exi)
	register fhandle_t *fh;
	struct vnode *vp;
	struct exportinfo *exi;
{
	struct fid *fidp;
	int error;

	if (vp->v_fstype == VNFS) {
		return (EREMOTE);
	}

	error = VOP_FID(vp, &fidp);
	if (error || fidp == NULL) {
		/*
		 * Should be something other than EREMOTE
		 */
		return (EREMOTE);
	}
	if (fidp->fid_len + exi->exi_fid->fid_len + sizeof(fsid_t)
		> NFS_FHSIZE)
	{
		freefid(fidp);
		return (EREMOTE);
	}
	bzero((caddr_t) fh, sizeof(*fh));
	fh->fh_fsid[0] = vp->v_vfsp->vfs_fsid[0];
	fh->fh_fsid[1] = vp->v_vfsp->vfs_fsid[1];
	fh->fh_len = fidp->fid_len;
	bcopy(fidp->fid_data, fh->fh_data, fidp->fid_len);
	fh->fh_xlen = exi->exi_fid->fid_len;
	bcopy(exi->exi_fid->fid_data, fh->fh_xdata, fh->fh_xlen);
#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "makefh: vp %x fsid %x %x len %d data %d %d\n",
		vp, fh->fh_fsid[0], fh->fh_fsid[1], fh->fh_len,
		*(int *)fh->fh_data, *(int *)&fh->fh_data[sizeof(int)]);
#endif
	freefid(fidp);
	return (0);
}

/*
 * Find the export structure associated with the given filesystem
 */
struct exportinfo *
findexport(fsid, fid)
	fsid_t *fsid;	
	struct fid *fid;
{
	struct exportinfo *exi;

	for (exi = exported; exi != NULL; exi = exi->exi_next) {
		if (exportmatch(exi, fsid, fid)) {
			return (exi);
		}
	}
	return (NULL);
}

/*
 * Load from user space, a list of internet addresses into kernel space
 */
loadaddrs(addrs)
	struct exaddrlist *addrs;
{
	int error;
	int allocsize;
	struct sockaddr *uaddrs;

	if (addrs->naddrs > EXMAXADDRS) {
		char nfs_log_str_mess[100];

                sprintf(nfs_log_str_mess, sizeof(nfs_log_str_mess),
                        "loadaddrs: out of range %d ", addrs->naddrs);
                NS_LOG_STR(LE_NFS_LOAD_ADDR, NS_LC_WARNING, NS_LS_NFS, 0,
                         nfs_log_str_mess);
		return (EINVAL);
	}
	if (addrs->naddrs == 0) {
		addrs->addrvec = (struct sockaddr *)NULL;
		return(0);
	}
	allocsize = addrs->naddrs * sizeof(struct sockaddr);
	uaddrs = addrs->addrvec;

	addrs->addrvec = (struct sockaddr *)mem_alloc(allocsize);
	error = copyin((caddr_t)uaddrs, (caddr_t)addrs->addrvec,
		       (u_int)allocsize);	
	if (error) {
		mem_free((char *)addrs->addrvec, allocsize);
                NS_LOG_STR(LE_NFS_COPYIN_FAIL, NS_LC_ERROR, NS_LS_NFS, 0,
                        "loadaddrs: copyin");
	}
	return (error);
}

#ifdef notdef
/*
 * Load from user space the root user names into kernel space
 * (AUTH_DES only)
 */
loadrootnames(kex)
	struct export *kex;
{
	int error;
	char *exnames[EXMAXROOTNAMES];
	int i;
	u_int len;
	char netname[MAXNETNAMELEN+1];
	u_int allocsize;

	if (kex->ex_des.nnames > EXMAXROOTNAMES) {
		return (EINVAL);
	}

	/*
	 * Get list of names from user space
	 */
	allocsize =  kex->ex_des.nnames * sizeof(char *);
	error = copyin((char *)kex->ex_des.rootnames, (char *)exnames,
		allocsize);
	if (error) {
		return (error);
	}
	kex->ex_des.rootnames = (char **) mem_alloc(allocsize);
	bzero((char *) kex->ex_des.rootnames, allocsize);

	/*
	 * And now copy each individual name
	 */
	for (i = 0; i < kex->ex_des.nnames; i++) {
		error = copyinstr(exnames[i], netname, sizeof(netname), &len);
		if (error) {
			goto freeup;
		}
		kex->ex_des.rootnames[i] = (char *) mem_alloc(len + 1);
		bcopy(netname, kex->ex_des.rootnames[i], len);
		kex->ex_des.rootnames[i][len] = 0;
	}
	return (0);

freeup:
	freenames(kex);
	return (error);
}

/*
 * Figure out everything we allocated in a root user name list in
 * order to free it up. (AUTH_DES only)
 */
freenames(ex)
	struct export *ex;
{
	int i;

	for (i = 0; i < ex->ex_des.nnames; i++) {
		if (ex->ex_des.rootnames[i] != NULL) {
			mem_free((char *) ex->ex_des.rootnames[i],
				strlen(ex->ex_des.rootnames[i]) + 1);
		}
	}	
	mem_free((char *) ex->ex_des.rootnames, ex->ex_des.nnames * sizeof(char *));
}
#endif /* notdef */


/*
 * Free an entire export list node
 */
exportfree(exi)
	struct exportinfo *exi;
{
	struct export *ex;

	ex = &exi->exi_export;
	switch (ex->ex_auth) {
	case AUTH_UNIX:
		if (ex->ex_unix.rootaddrs.addrvec != NULL) {
			mem_free((char *)ex->ex_unix.rootaddrs.addrvec,
			 	(ex->ex_unix.rootaddrs.naddrs *
			  	sizeof(struct sockaddr)));
		}
		break;
#ifdef notdef
	case AUTH_DES:
		freenames(ex);
		break;
#endif /* notdef */
	}
	if (ex->ex_flags & EX_RDMOSTLY) {
		if (ex->ex_writeaddrs.addrvec != NULL) {
			mem_free((char *)ex->ex_writeaddrs.addrvec,
			 	ex->ex_writeaddrs.naddrs * sizeof(struct sockaddr));
		}
	}
	freefid(exi->exi_fid);
	mem_free(exi, sizeof(struct exportinfo));
}
