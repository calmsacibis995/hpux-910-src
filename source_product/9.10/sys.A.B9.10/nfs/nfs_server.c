/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/nfs_server.c,v $
 * $Revision: 1.17.83.9 $	$Author: craig $
 * $State: Exp $	$Locker:  $
 * $Date: 95/01/06 13:49:43 $
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
/* HPNFS
 * Including ../h/proc.h to be able to set p_flag in rfs_dispatch()
 * HPNFS
 */
#include "../h/proc.h"
#include "../h/buf.h"
#include "../ufs/fsdir.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/socketvar.h"
#include "../h/socket.h"	/*Added to check socket in nfs_svc()*/
#include "../h/domain.h"
#include "../h/protosw.h"	/*Added to check socket in nfs_svc()*/
#include "../h/errno.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/auth_unix.h"
#include "../rpc/svc.h"
#include "../rpc/xdr.h"
#include "../nfs/export.h"
#define NFSSERVER		/* To get the server's view of the fh */
#include "../nfs/nfs.h"
#include "../rpc/clnt.h"
#include "../h/mbuf.h"

#include "../ufs/inode.h"
#include "../dux/cct.h"
#include "../h/kern_sem.h"

#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"
#include "../cdfs/cdfs.h"

#include "../h/stat.h"
#include "../h/fcntl.h"
#include "../h/utsname.h"
#include "../nfs/lockmgr.h"

/*
 * Global variables for keeping track of the lock manager.
 */
pid_t lock_manager_pid = 0;
struct proc *lock_manager_proc = NULL;
#define EDROPPACKET -1

#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI
#include "../h/kern_sem.h"	/* MPNET */
#include "../net/netmp.h"	/* MPNET */

#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"

/*
 * rpc service program version range supported
 */
#define	VERSIONMIN	2
#define	VERSIONMAX	2

/*
 * Returns true iff filesystem for a given fsid is exported read-only
 * The test checks for being exported read-only or being exported
 * read-mostly and not being in the list of having write access.
 */
#define rdonly(exi, req) (((exi)->exi_export.ex_flags & EX_RDONLY) || \
			  (((exi)->exi_export.ex_flags & EX_RDMOSTLY) && \
			   !hostinlist((struct sockaddr *)\
					svc_getcaller((req)->rq_xprt), \
					&(exi)->exi_export.ex_writeaddrs)))

struct vnode	*fhtovp();
struct file	*getsock();
void		svcerr_progvers();
void		rfs_dispatch();

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

struct {
	int	ncalls;		/* number of calls received */
	int	nbadcalls;	/* calls that failed */
	int	reqs[32];	/* count for each request */
} svstat;

/*
 * NFS Server system call.
 * Does all of the work of running a NFS server.
 * sock is the fd of an open UDP socket.
 */
nfs_svc(uap)
	struct a {
		int     sock;
	} *uap;
{
	struct vnode	*rdir;
	struct vnode	*cdir;
	struct socket   *so;
	struct file	*fp;
	SVCXPRT *xprt;
	u_long vers;
	sv_sema_t savestate;		/* MPNET: MP save state */
	int oldlevel;

	/*
	 * HPNFS
	 * count is being added to keep track of the number of nfs daemons
	 * that are active.  This is to prevent doing a svc_unregister
	 * until the last nfs daemon is gone.
	 */
	static count = 0;

	/*MPNET: turn MP protection on */
	NETMP_GO_EXCLUSIVE(oldlevel, savestate);

	fp = getsock(uap->sock);
	if (fp == 0) {
		u.u_error = EBADF;
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		return;
	}
	so = (struct socket *)fp->f_data;
	if ( (so->so_type != SOCK_DGRAM) ||
	     (so->so_proto->pr_domain->dom_family != AF_INET) ) {
		u.u_error = EINVAL;
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		return;
	}

	
	/*
	 * Be sure that rdir (the server's root vnode) is set.
	 * Save the current directory and restore it again when
	 * the call terminates.  rfs_lookup uses u.u_cdir for lookupname.
	 */
	rdir = u.u_rdir;
	cdir = u.u_cdir;
	if (u.u_rdir == (struct vnode *)0) {
		u.u_rdir = u.u_cdir;
	}
	if ((xprt = svckudp_create(so, NFS_PORT)) == NULL) {
		u.u_error = ENOBUFS;
		NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
		/*MPNET: MP protection is now off. */
		return;
	}
	count++;
	for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
		(void) svc_register(xprt, NFS_PROGRAM, vers, rfs_dispatch,
		    FALSE);
	}
	if (setjmp(&u.u_qsave)) {
		/* MPNET: turn MP protection on.  We have to re-acquire
		** the semaphore.  If the previous NETMP_GO_UNEXCLUSIVE
		** gave up a semaphore from another empire to gain the n/w
		** semaphore, that fact will be stored in "savestate"
		** and we will re-acquire it upon return (via the
		** macro)
		 */
		NET_PSEMA();
		count--;
		if (count == 0)
		    	for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
			    	svc_unregister(NFS_PROGRAM, vers);
		    	}
		SVC_DESTROY(xprt);
		u.u_error = EINTR;
	} else {
		svc_run(xprt);  /* never returns */
	}
	u.u_rdir = rdir;
	u.u_cdir = cdir;
	NETMP_GO_UNEXCLUSIVE(oldlevel, savestate);
	/*MPNET: MP protection is now off. */
}



	
/*
 * These are the interface routines for the server side of the
 * Networked File System.  See the NFS protocol specification
 * for a description of this interface.
 */


/*
 * Get file attributes.
 * Returns the current attributes of the file with the given fhandle.
 */
int
rfs_getattr(fhp, ns, exi)
	fhandle_t *fhp;
	register struct nfsattrstat *ns;
	struct exportinfo *exi;
{
	int error;
	register struct vnode *vp;
	struct vattr va;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_getattr fh %o %d\n",
	    fhp->fh_fsid, fhp->fh_fno);
#endif
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}

	error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
	if (!error) {
		vattr_to_nattr(&va, &ns->ns_attr);
	}
	ns->ns_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_getattr: returning %d\n", error);
#endif
}

/*
 * Set file attributes.
 * Sets the attributes of the file with the given fhandle.  Returns
 * the new attributes.
 * NOTE: Checks for executing files (VTEXT and ETXTBSY) are performed by
 * the ufs_setattr() and iaccess() code in this case.
 */
int
rfs_setattr(args, ns, exi, req)
	struct nfssaargs *args;
	register struct nfsattrstat *ns;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *vp;
	struct vattr va;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_setattr fh %o %d\n",
	    args->saa_fh.fh_fsid, args->saa_fh.fh_fno);
#endif
	vp = fhtovp(&args->saa_fh, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req) || isrofile(vp)) {
		error = EROFS;
	} else {
		sattr_to_vattr(&args->saa_sa, &va);
		error = VOP_SETATTR(vp, &va, u.u_cred, 0);
		/*
		 * If changing ownership, a duplicate request will fail
		 * because the file has becomed owned by someone else.  We
		 * only do duplicate requests in that case because all other
		 * setattr calls are idempotent.
		 */
		if (!error || (error == EPERM && va.va_uid != (u_short) -1 &&
				svckudp_dup(req)) ) {

			error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
			if (!error) {
				vattr_to_nattr(&va, &ns->ns_attr);
				if ( va.va_uid != (u_short) -1 )
					svckudp_dupsave(req);
			}
		}
	}
	ns->ns_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_setattr: returning %d\n", error);
#endif
}

/*
 * Directory lookup.
 * Returns an fhandle and file attributes for file name in a directory.
 */
int
rfs_lookup(da, dr, exi)
	struct nfsdiropargs *da;
	register struct  nfsdiropres *dr;
	struct exportinfo *exi;
{
	int error;
	register struct vnode *dvp;
	struct vnode *vp;
	struct vattr va;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_lookup %s fh %o %d\n",
	    da->da_name, da->da_fhandle.fh_fsid, da->da_fhandle.fh_fno);
#endif

	/*
         *      Disallow NULL paths
         */
        if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
                dr->dr_status = NFSERR_ACCES;
                return;
        }
	
	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return;
	}

	/*
	 * do lookup.
	 */
	error = VOP_LOOKUP(dvp, da->da_name, &vp, u.u_cred,dvp);
	if (error) {
		vp = (struct vnode *)0;
	} else {

		error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, vp, exi);
		}
	}
	dr->dr_status = puterrno(error);
	if (vp) {
		VN_RELE(vp);
	}
	VN_RELE(dvp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_lookup: returning %d\n", error);
#endif
}

/*
 * Read symbolic link.
 * Returns the string in the symbolic link at the given fhandle.
 */
int
rfs_readlink(fhp, rl, exi)
	fhandle_t *fhp;
	register struct nfsrdlnres *rl;
	struct exportinfo *exi;
{
	int error;
	struct iovec iov;
	struct uio uio;
	struct vnode *vp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_readlink fh %o %d\n",
	    fhp->fh_fsid, fhp->fh_fno);
#endif
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		rl->rl_status = NFSERR_STALE;
		return;
	}

	/*
	 * Allocate data for pathname.  This will be freed by rfs_rlfree.
	 */
	rl->rl_data = (char *)kmem_alloc((u_int)MAXPATHLEN);

	/*
	 * Set up io vector to read sym link data
	 */
	iov.iov_base = rl->rl_data;
	iov.iov_len = NFS_MAXPATHLEN;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_seg = UIOSEG_KERNEL;
	uio.uio_offset = 0;
	uio.uio_resid = NFS_MAXPATHLEN;

	/*
	 * read link
	 */
	error = VOP_READLINK(vp, &uio, u.u_cred);

	/*
	 * Clean up
	 */
	if (error) {	
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
		rl->rl_count = 0;
		rl->rl_data = NULL;
	} else {
		rl->rl_count = NFS_MAXPATHLEN - uio.uio_resid;
	}
	rl->rl_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_readlink: returning '%s' %d\n",
	    rl->rl_data, error);
#endif
}

/*
 * Free data allocated by rfs_readlink
 */
rfs_rlfree(rl)
	struct nfsrdlnres *rl;
{
	if (rl->rl_data) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
	}
}

/*
 * Read data.
 * Returns some data read from the file at the given fhandle.
 */
int
rfs_read(ra, rr, exi)
	struct nfsreadargs *ra;
	register struct nfsrdresult *rr;
	struct exportinfo *exi;
{
	int error;
	struct vnode *vp;
	struct vattr va;
	struct iovec iov;
	struct uio uio;
	int offset, fsbsize;
	struct buf *bp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_read %d from fh %o %d\n",
	    ra->ra_count, ra->ra_fhandle.fh_fsid, ra->ra_fhandle.fh_fno);
#endif
	rr->rr_data = NULL;
	rr->rr_count = 0;
	vp = fhtovp(&ra->ra_fhandle, exi);
	if (vp == NULL) {
		rr->rr_status = NFSERR_STALE;
		return;
	}

	error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
	if (error) {
		goto bad;
	}

	/* Make sure the user is not asking for some outrageous read size
	 * or we will panic in the MALLOC below.  64k should be a reasonable
	 * value.  8k may be better, but we can handle 64k.  cwb
	 */
	if (ra->ra_count > 64*1024) {
		error = EIO;
		goto bad;
	}

	/*
	 * This is a kludge to allow reading of files created
	 * with no read permission.  The owner of the file
	 * is always allowed to read it.
	 */
	if ((u_short)u.u_uid != va.va_uid) {
		error = VOP_ACCESS(vp, VREAD, u.u_cred);
		if (error) {
			/*
			 * Exec is the same as read over the net because
			 * of demand loading.
			 */
			error = VOP_ACCESS(vp, VEXEC, u.u_cred);
		}
		if (error) {
			goto bad;
		}
	}
	/*
	 * Check to see if the mode of the file specifies enforcement mode
	 * locking.  If it does, then we have to see if anyone has the file
	 * locked.  If so, then we use a special errno to tell rfsdispatch to
	 * not generate a reply.  We can't go ahead and do the request because
	 * it would block, potentially for a long time.  We can't send back
	 * an error reply, because most clients wouldn't know what to do
	 * with it.  Therefore we essentially drop the packet, causing a
	 * retransmission by the client.  Assuming the client has us hard
	 * mounted, the client will retransmit and eventually get through when
	 * the lock is released.  The one time we DON'T drop the packet is if
	 * the LM is the one that has the file locked.  In that case, the LM
	 * has been granted a lock, and THEN some other process has changed
	 * the mode to enforcement (LM won't grant enforcement mode locks).
	 * Since the UFS code can't tell that the nfsd may be the same person
	 * that has the lock, it would block, in this case forever.  To prevent
	 * this we have to return an error to the read request.
	 * Note that we only pay this overhead (except for the check) for
	 * enforcement mode locks.
	 */
	if ( (va.va_mode & S_ENFMT) && !(va.va_mode & S_IXGRP) ) {
		register off_t LB,UB;
		struct flock testflock;
		struct file testfile;
		/*
		 * Fill in a flock structure for the request.
		 */
		testflock.l_type = F_RDLCK;
		testflock.l_whence = 0;
		testflock.l_start = ra->ra_offset;
		LB = ra->ra_offset;
		testflock.l_len = ra->ra_count;
		UB = LB + ra->ra_count;
		testfile.f_flag = 0;
		if ( (error = VOP_LOCKCTL( vp, &testflock, F_GETLK, u.u_cred,
		    &testfile, LB, UB)) == 0 && testflock.l_type == F_WRLCK) {
			/*
			 * File is locked, handle appropriately.
			 */
			if ( testflock.l_pid == lock_manager_pid )
				error = EINVAL;
			else
				error = EDROPPACKET;
			goto bad;
		}
	}

	/*
	 * Check whether we can do this with bread, which would
	 * save the copy through the uio.
	 */
	fsbsize = vp->v_vfsp->vfs_bsize;
	if (fsbsize > 0 && vp->v_fstype != VCDFS && vp->v_fstype != VDUX_CDFS) {
	    offset = ra->ra_offset % fsbsize;
	    if (offset + ra->ra_count <= fsbsize) {
		if (ra->ra_offset >= va.va_size) {
			rr->rr_count = 0;
			vattr_to_nattr(&va, &rr->rr_attr);
			goto done;
		}
		error = VOP_BREAD(vp, ra->ra_offset / fsbsize, &bp);
		if (error == 0) {
			rr->rr_data = bp->b_un.b_addr + offset;
			rr->rr_count = min(
			    (u_int)(va.va_size - ra->ra_offset),
			    (u_int)ra->ra_count);
			rr->rr_bp = bp;
			rr->rr_vp = vp;
			VN_HOLD(vp);
			vattr_to_nattr(&va, &rr->rr_attr);
			goto done;
		} else {
			NS_LOG_INFO(LE_NFS_READ_ERROR,NS_LC_ERROR,NS_LS_NFS,
			    0,1,error,0);
		}
	    }
	}
	rr->rr_bp = (struct buf *) 0;
			
	/*
	 * Allocate space for data.  This will be freed by rfs_rdfree().
	 * Don't use MALLOC, since size not known at compile time.
	 */
	rr->rr_data = (caddr_t) kmalloc((u_long)ra->ra_count, M_TEMP, M_WAITOK);

	/*
	 * Set up io vector
	 */
	iov.iov_base = rr->rr_data;
	iov.iov_len = ra->ra_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_seg = UIOSEG_KERNEL;
	uio.uio_offset = ra->ra_offset;
	uio.uio_resid = ra->ra_count;
	/*
	 * for now we assume no append mode and ignore
	 * totcount (read ahead)
	 */
	error = VOP_RDWR(vp, &uio, UIO_READ, IO_SYNC, u.u_cred);
	if (error) {
		goto bad;
	}
	vattr_to_nattr(&va, &rr->rr_attr);
	rr->rr_count = ra->ra_count - uio.uio_resid;
bad:
	if (error && rr->rr_data != NULL) {
		FREE((caddr_t)rr->rr_data, M_TEMP);
		rr->rr_data = NULL;
		rr->rr_count = 0;
	}
done:
	rr->rr_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_read returning %d, count = %d\n",
	    error, rr->rr_count);
#endif
}

/*
 * Free data allocated by rfs_read.
 */
rfs_rdfree(rr)
	struct nfsrdresult *rr;
{
	if (rr->rr_bp == 0 && rr->rr_data) {
		FREE((caddr_t)rr->rr_data, M_TEMP);
	}
}

int check_dup_writes = 1;

/*
 * Write data to file.
 * Returns attributes of a file after writing some data to it.
 */
int
rfs_write(wa, ns, exi, req)
	struct nfswriteargs *wa;
	struct nfsattrstat *ns;
	struct exportinfo *exi;
	struct svc_req *req;
{
	register int error;
	register struct vnode *vp;
	struct vattr va;
	struct uio uio;
	struct inode *ip;
	int synced = 0;
	int sync_flag;  /* determines if writes are synchronous or asych. */

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_write: %d bytes fh %o %d\n",
	    wa->wa_count, wa->wa_fhandle.fh_fsid, wa->wa_fhandle.fh_fno);
#endif
	vp = fhtovp(&wa->wa_fhandle, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}

	/*
	 * Look at the file handle to determine whether to do synchronous or
	 * asynchronous writes.  This is initially set by rpc.mountd and
	 * passed along by rfs_create, rfs_lookup, and rfs_mkdir
	 */
	sync_flag = (exi->exi_export.ex_flags & EX_ASYNC) ? 0 : IO_SYNC;

	if (rdonly(exi, req)) {
		error = EROFS;
	} else if (vp->v_type == VDIR) {
		/* Don't allow a directory to be written to.  This can happen
		   if an old file handle is used and the inode and generation
		   numbers happen to line up.  Causes panic on server. cwb
		 */
		error = EISDIR;
	}
	else {
		error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
	}

	/*
	 * Check for a duplicate request.  While this is not necessary for
	 * unloaded systems, servers under load may see fairly frequent
	 * retransmission.  By checking for duplicate requests we prevent
	 * doing writes twice.  The cost of doing this is essentially the
	 * time to search a linked list in svckudp_dup(), and must be
	 * balanced versus the savings we make.  Since writes are an expensive
	 * operation, it doesn't take a very high percentage to make this
	 * worth while.  Since we already have the attributes, this is
	 * a good place to put this check in.  Note that in the async. write
	 * case, duplicate requests are less likely, so that checking for
	 * duplicate requests becomes less worth-while, hurting performance.
	 */
	if ( !error && sync_flag && check_dup_writes && svckudp_dup(req) ) {
		ns->ns_status = puterrno(error);
		vattr_to_nattr(&va, &ns->ns_attr);
		VN_RELE(vp);
		return;
	}
	/* HPNFS:
	 * Since there is no rfs_open() routine, we have to check for VEXT
	 * on every write (Oh!  The wonders of statelessness!) to insure
	 * we don't blow away a running program.  Try to free it up once, which
	 * is the standard algorithm used for this check.  NOTE: We are
	 * returning ETXTBSY even though it is not listed as one of the
	 * acceptable return values in nfs.h.  This is because SUN will also
	 * return this error under some conditions, so it should be ok for
	 * us to do that too.  See also rfs_rename(), rfs_remove().
	 */
	if (!error && (vp->v_flag & VTEXT)) {
		xrele(vp);
		if (vp->v_flag & VTEXT)
			error = ETXTBSY;
	}
	/*
	 * Check to see if the mode of the file specifies enforcement mode
	 * locking.  If it does, then we have to see if anyone has the file
	 * locked.  If so, then we use a special errno to tell rfsdispatch to
	 * not generate a reply.  We can't go ahead and do the request because
	 * it would block, potentially for a long time.  We can't send back
	 * an error reply, because most clients wouldn't know what to do
	 * with it.  Therefore we essentially drop the packet, causing a
	 * retransmission by the client.  Assuming the client has us hard
	 * mounted, the client will retransmit and eventually get through when
	 * the lock is released.  The one time we DON'T drop the packet is if
	 * the LM is the one that has the file locked.  In that case, the LM
	 * has been granted a lock, and THEN some other process has changed
	 * the mode to enforcement (LM won't grant enforcement mode locks).
	 * Since the UFS code can't tell that the nfsd may be the same person
	 * that has the lock, it would block, in this case forever.  To prevent
	 * this we have to return an error to the write request.
	 * Note that we only pay this overhead (except for the check) for
	 * enforcement mode locks.
	 */
	if ( !error && (va.va_mode & S_ENFMT) && !(va.va_mode & S_IXGRP) ) {
		register off_t LB,UB;
		struct flock testflock;
		struct file testfile;
		/*
		 * Fill in a flock structure for the request.
		 */
		testflock.l_type = F_WRLCK;
		testflock.l_whence = 0;
		testflock.l_start = wa->wa_offset;
		LB = wa->wa_offset;
		testflock.l_len = wa->wa_count;
		UB = LB + wa->wa_count;
		testfile.f_flag = 0;
		if ( (error = VOP_LOCKCTL( vp, &testflock, F_GETLK, u.u_cred,
		    &testfile, LB, UB)) == 0 && testflock.l_type != F_UNLCK) {
			/*
			 * File is locked, handle appropriately.
			 */
			if ( testflock.l_pid == lock_manager_pid )
				error = EINVAL;
			else
				error = EDROPPACKET;
		}
	}
	if (!error) {
		if ((u_short)u.u_uid != va.va_uid) {
			/*
			 * This is a kludge to allow writes of files created
			 * with read only permission.  The owner of the file
			 * is always allowed to write it.
			 */
			error = VOP_ACCESS(vp, VWRITE, u.u_cred);
		}
		if (!error) {
			uio.uio_iov = wa->wa_iov;
			uio.uio_iovcnt = wa->wa_iovcnt;
			uio.uio_seg = UIOSEG_KERNEL;
			uio.uio_offset = wa->wa_offset;
			uio.uio_resid = wa->wa_count;
			uio.uio_fpflags = (sync_flag ? FSYNCIO : 0 );
			/* synchronize the inode if necessary */
			/* this is done for ufs at the open code */
			/* but must be done here are there is no open for NFS */
			/* only bother to do it if you are clustered. */
			if ( (my_site_status & CCT_CLUSTERED) &&
			      va.va_type == VREG) {
				ip = VTOI(vp);
				isynclock(ip);
				updatesitecount (&(ip->i_writesites), u.u_site, 1);
				if (vp->v_flag & VTEXT) {
					updatesitecount (&(ip->i_writesites), u.u_site, -1);
					isyncunlock(ip);
					error = ETXTBSY;
					goto puterr;
				}
				updatesitecount (&(ip->i_opensites), u.u_site, 1);
retry:
				/* if checksync returns an error, then */
				/* it means that a switch to synchronous */
				/* mode was indicated, but at least */
				/* one site who was suppose to switch */
				/* could not find the inode in its incore */
				/* table.  This can happen if the dm_rely */
				/* for a dux_copen_serve() has not gotten */
				/* through yet. */
				ip->i_flag |= IOPEN;
				error = checksync(ip);
				ip->i_flag &= ~IOPEN;
				isyncunlock(ip);
				if (error) {
					ip->i_flag &= ~ISYNC;
					sleep((caddr_t)&lbolt, PINOD);
					isynclock(ip);
					goto retry;
				}
				synced = 1;

			}
			/*
			 * for now we assume no append mode
			 */
			error = VOP_RDWR(vp, &uio, UIO_WRITE, sync_flag, u.u_cred);
			/* unsync IF we synced */
			if (synced && (va.va_type == VREG) ) {
				isynclock(ip);
				updatesitecount (&(ip->i_opensites), u.u_site, -1);
				updatesitecount (&(ip->i_writesites), u.u_site, -1);
				ip->i_fversion++;
				checksync(ip);
				isyncunlock(ip);
			}
		}
	}
	if (!error) {
		/*
		 * Get attributes again so we send the latest mod
		 * time to the client side for his cache.
		 */

		error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
	}
puterr:
	ns->ns_status = puterrno(error);
	if (!error) {
		vattr_to_nattr(&va, &ns->ns_attr);
		if ( sync_flag && check_dup_writes )
		    svckudp_dupsave( req );
	}
	VN_RELE(vp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_write: returning %d\n", error);
#endif
}

/*
 * Create a file.
 * Creates a file with given attributes and returns those attributes
 * and an fhandle for the new file.
 */
int
rfs_create(args, dr, exi, req)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct exportinfo *exi;
	struct svc_req *req;
{
	register int error;
	struct vattr va;
	struct vnode *vp;
	register struct vnode *dvp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_create: %s dfh %o %d\n",
	    args->ca_da.da_name, args->ca_da.da_fhandle.fh_fsid,
	    args->ca_da.da_fhandle.fh_fno);
#endif

	/*
	 *      Disallow NULL paths
	 */
	if ((args->ca_da.da_name == (char *) NULL) ||
	    (*(args->ca_da.da_name) == '\0')) {
		dr->dr_status = NFSERR_ACCES;
		return;
	}

	sattr_to_vattr(&args->ca_sa, &va);

	/*
	 * This is a completely gross hack to make mknod
	 * work over the wire until we can wack the protocol
	 */
	if ((va.va_mode & S_IFMT) == S_IFCHR) {
		va.va_type = VCHR;
		if (va.va_size == (u_long)NFS_FIFO_DEV)
			va.va_type = VFIFO;	/* xtra kludge for namedpipe */
		else
			va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
		va.va_mode &= ~S_IFMT;
	} else if ((va.va_mode & S_IFMT) == S_IFBLK) {
		va.va_type = VBLK;
		va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
		va.va_mode &= ~S_IFMT;
	} else if ((va.va_mode & S_IFMT) == IFSOCK) {
		va.va_type = VSOCK;
	} else {
		va.va_type = VREG;
	}

  	/* HPNFS Just to keep Client SuperHosers from making a file
  	 * with the sticky bit set.  MDS  08/28/87
  	 * Found out that suser sets u_error which causes problem in
  	 * creating a file.  BAD.  So I will just check uid here.
  	 */
  	if (u.u_uid != 0)
  		va.va_mode = va.va_mode & (u_short) ~VSVTX;
	/*
	 * XXX Should get exclusive flag and use it.
	 */
	dvp = fhtovp(&args->ca_da.da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return;
	}
	/*
	 * A duplicate create request that is passed down to
	 * ufs_create would (potentially) cause a second truncation if one
	 * or more write requests had been processed before the duplicate
	 * create request.
	 */
	if (svckudp_dup(req)) {
		error = VOP_LOOKUP(dvp, args->ca_da.da_name, &vp, u.u_cred,dvp);
		goto out;
	}

	/*
	 * Only super-user is allowed to create (via mknod) device files.
	 * However, it is possible that the file already exists, in which
	 * case it is legal to do the create (assuming the user has
	 * permissions to do so).
	 */
	if ( va.va_type == VCHR || va.va_type == VBLK ) {
	    error = VOP_LOOKUP(dvp, args->ca_da.da_name, &vp, u.u_cred,dvp);
	    if ( error ) {
		if (error == ENOENT && !suser() )
			error = EPERM;
		dr->dr_status = puterrno(error);
		return;
	    }
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		error = VOP_CREATE(dvp, args->ca_da.da_name,
		    &va, NONEXCL, VWRITE, &vp,u.u_cred);
	}

out:
	if (!error) {

		error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC);
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, vp, exi);
		}
		VN_RELE(vp);
	}
	dr->dr_status = puterrno(error);
	VN_RELE(dvp);
	if (!error) {
		svckudp_dupsave(req);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_create: returning %d\n", error);
#endif
}

/*
 * Remove a file.
 * Remove named file from parent directory.
 */
int
rfs_remove(da, status, exi, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *vp;
	struct vnode *rm_vp;
	struct vattr vattr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_remove %s dfh %o %d\n",
	    da->da_name, da->da_fhandle.fh_fsid, da->da_fhandle.fh_fno);
#endif

        /*
         *      Disallow NULL paths
         */
        if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
                *status = NFSERR_ACCES;
                return;
        }

	vp = fhtovp(&da->da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
  	/* HPNFS  Change added to prevent a unlink of a directory
  	 * by a non-superuser over NFS.  The rmdir() will go
  	 * through rfs_rmdir().  MDS 8/28/87
  	 */
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
  		error = VOP_LOOKUP(vp, da->da_name, &rm_vp, u.u_cred, vp);
  		if (!error) {
  			if ((rm_vp->v_type == VDIR) && !suser() ) {
  				error = EPERM;
				VN_RELE(rm_vp);
  				goto outahere;
  			}
			/* HPNFS:
			 * Check for an executing program.  We can remove
			 * everything but the last link to the program.
			 * See rfs_write for details.
			 */
			if (rm_vp->v_flag & VTEXT) {
			    error = VOP_GETATTR(rm_vp, &vattr, u.u_cred, VSYNC);
			    if ( error ) {
				    VN_RELE(rm_vp);
				    goto outahere;
			    }
			    xrele(rm_vp);
			    if ((rm_vp->v_flag & VTEXT) && vattr.va_nlink == 1){
				    error = ETXTBSY;
				    VN_RELE(rm_vp);
				    goto outahere;
			    }
			}
			VN_RELE(rm_vp);
  		}
/* HPNFS
 * To fix a CDF and retransmission problem.  Please see rfs_rename for more.
 */
		if (svckudp_dup(req)) {
			error = 0;
		} else
		{
			error = VOP_REMOVE(vp, da->da_name, u.u_cred);
			if (error == ENOENT) {
			       /*
			 	* check for dup request
			 	*/
				if (svckudp_dup(req)) {
					error = 0;
				}
			}
		}
	}
outahere:
	*status = puterrno(error);
	VN_RELE(vp);
	if (!error) {
		svckudp_dupsave(req);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_remove: %s returning %d\n",
	    da->da_name, error);
#endif
}

/*
 * rename a file
 * Give a file (from) a new name (to).
 */
int
rfs_rename(args, status, exi, req)
	struct nfsrnmargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error = 0;
	register struct vnode *fromvp;
	register struct vnode *tovp;
	struct vnode *rm_vp;	/* Used to lookup target file */
	struct vattr vattr;	/* Find the vnode attributes */
	struct exportinfo *exi2;
	fhandle_t *fh;
	struct ucred *newcr = NULL;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_rename %s ffh %o %d -> %s tfh %o %d\n",
	    args->rna_from.da_name,
	    args->rna_from.da_fhandle.fh_fsid,
	    args->rna_from.da_fhandle.fh_fno,
	    args->rna_to.da_name,
	    args->rna_to.da_fhandle.fh_fsid,
	    args->rna_to.da_fhandle.fh_fno);
#endif

	/*
         *      Disallow NULL paths
         */
        if ((args->rna_from.da_name == (char *) NULL) ||
            (*args->rna_from.da_name == '\0') ||
            (args->rna_to.da_name == (char *) NULL) ||
            (*args->rna_to.da_name == '\0')) {
                *status = NFSERR_ACCES;
                return;
        }
	fromvp = fhtovp(&args->rna_from.da_fhandle, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
		goto fromerr;
	}
	fh = &args->rna_to.da_fhandle;
	exi2 = findexport(&fh->fh_fsid, (struct fid *) &fh->fh_xlen);
	if (exi != exi2) {
		newcr = crget();
		if ((exi2 != NULL) && checkauth(exi2, req, newcr, 0)) {
			/* All I can think to do in this case is make
			   sure if we're root on source, we're root on target.
			 */
			if ((newcr->cr_uid != 0) && (u.u_uid == 0)) {
			    u.u_uid = newcr->cr_uid;
			    u.u_gid = newcr->cr_gid;
			}
		}
		else {
			error = EPERM;
			goto fromerr;
		}
		crfree(newcr);
	}

	tovp = fhtovp(fh, exi2);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		return;
	}

	if (rdonly(exi2, req)) {
		error = EROFS;
	} else {

/* HPNFS
 * This late change was made to prevent a situation of rename acting with
 * CDF's  If you have a CDF with a context of <hostname> and of default, then
 * you could do a rename of the CDF and have the first request rename the
 * <hostname> context and then on a duplicate request have a rename done on
 * the default context.  This then will blow away the data found in the
 * <hostname> context.  So to prevent this, I will check on a duplicate
 * request BEFORE doing the VOP call.  This is based on the assumption that
 * there would be a duplicate request registered with the server ONLY if a
 * previous request was successful and therefore we can just send a status
 * of 0.  This will also effect rmdir and rm.  Mike Shipley CND 09/14/87
 */
	 	if (svckudp_dup(req)) {
			error = 0;
	        } else {
			/*
			 * HPNFS:
			 * If the target filename exists, it will be blown
			 * away.  However, we don't want this to happen to
			 * executing files, so we have to look it up to check.
			 * See rfs_write() for details.
			 */
			error = VOP_LOOKUP(tovp, args->rna_to.da_name, &rm_vp,
					   u.u_cred, tovp);
			if (!error) {
				if (rm_vp->v_flag & VTEXT) {
					error = VOP_GETATTR(rm_vp, &vattr,
							    u.u_cred, VSYNC);
					if (error) {
						VN_RELE( rm_vp );
						goto to_err;
					}
					xrele(rm_vp);
					if (rm_vp->v_flag&VTEXT && vattr.va_nlink == 1){
						error = ETXTBSY;
						VN_RELE( rm_vp );
						goto to_err;
					}
				}
				VN_RELE( rm_vp);
			}
		    	error = VOP_RENAME(fromvp, args->rna_from.da_name,
		        	tovp, args->rna_to.da_name, u.u_cred);
/* HPNFS
 * Just leave the normal SUN check for duplicate requests if you don't
 * have any CDF's.
 */
		    	if (error == ENOENT) {
			       /*
			     	* check for dup request
			     	*/
			    	if (svckudp_dup(req)) {
				    	error = 0;
			    	}
			}
		}
	}
to_err:
	VN_RELE(tovp);
	if (!error) {
		svckudp_dupsave(req);
	}
fromerr:
	VN_RELE(fromvp);
	*status = puterrno(error);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_rename: returning %d\n", error);
#endif
}

/*
 * Link to a file.
 * Create a file (to) which is a hard link to the given file (from).
 */
int
rfs_link(args, status, exi, req)
	struct nfslinkargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *fromvp;
	register struct vnode *tovp;
	struct exportinfo *exi2;
	fhandle_t *fh;
	struct ucred *newcr = NULL;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_link ffh %o %d -> %s tfh %o %d\n",
	    args->la_from.fh_fsid, args->la_from.fh_fno,
	    args->la_to.da_name,
	    args->la_to.da_fhandle.fh_fsid, args->la_to.da_fhandle.fh_fno);
#endif
	/*
         *      Disallow NULL paths
         */
        if ((args->la_to.da_name == (char *) NULL) ||
            (*args->la_to.da_name == '\0')) {
                *status = NFSERR_ACCES;
                return;
        }

	fromvp = fhtovp(&args->la_from, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;
		return;
	}

	fh = &args->la_to.da_fhandle;
	exi2 = findexport(&fh->fh_fsid, (struct fid *) &fh->fh_xlen);
	if (exi != exi2) {
		newcr = crget();
		if ((exi2 != NULL) && checkauth(exi2, req, newcr, 0)) {
			/* All I can think to do in this case is make
			   sure if we're root on source, we're root on target.
			 */
			if ((newcr->cr_uid != 0) && (u.u_uid == 0)) {
			    u.u_uid = newcr->cr_uid;
			    u.u_gid = newcr->cr_gid;
			}
		}
		else {
                	*status = NFSERR_ACCES;
			VN_RELE(fromvp);
			return;
		}
		crfree(newcr);
	}

	tovp = fhtovp(fh, exi2);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		return;
	}

	/*
	 * Both file systems must be writable.  We don't want the case where
	 * two directories on a file system are exported, one rw and one ro,
	 * and the client making a link to the ro system from the rw one and
	 * then changing a file.  cwb
	 */
	if (rdonly(exi, req) || rdonly(exi2, req)) {
		error = EROFS;
	} else {
		error = VOP_LINK(fromvp, tovp, args->la_to.da_name, u.u_cred);
		if (error == EEXIST) {
			/*
			 * check for dup request
			 */
			if (svckudp_dup(req)) {
				error = 0;
			}
		}
	}
	*status = puterrno(error);
	VN_RELE(fromvp);
	VN_RELE(tovp);
	if (!error) {
		svckudp_dupsave(req);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_link: returning %d\n", error);
#endif
}

/*
 * Symbolicly link to a file.
 * Create a file (to) with the given attributes which is a symbolic link
 * to the given path name (to).
 */
int
rfs_symlink(args, status, exi, req)
	struct nfsslargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{		
	int error;
	struct vattr va;
	register struct vnode *vp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_symlink %s ffh %o %d -> %s\n",
	    args->sla_from.da_name,
	    args->sla_from.da_fhandle.fh_fsid,
	    args->sla_from.da_fhandle.fh_fno,
	    args->sla_tnm);
#endif
	/*
         *      Disallow NULL paths
         */
        if ((args->sla_from.da_name == (char *) NULL) ||
            (*args->sla_from.da_name == '\0')) {
                *status = NFSERR_ACCES;
                return;
        }

	sattr_to_vattr(&args->sla_sa, &va);
	va.va_type = VLNK;
	vp = fhtovp(&args->sla_from.da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		error = VOP_SYMLINK(vp, args->sla_from.da_name,
		    &va, args->sla_tnm, u.u_cred);
		if (error == EEXIST) {
			/*
			 * check for dup request
			 */
			if (svckudp_dup(req)) {
				error = 0;
			}
		}
	}
	*status = puterrno(error);
	VN_RELE(vp);
	if (!error) {
		svckudp_dupsave(req);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_symlink: returning %d\n", error);
#endif
}

/*
 * Make a directory.
 * Create a directory with the given name, parent directory, and attributes.
 * Returns a file handle and attributes for the new directory.
 */
int
rfs_mkdir(args, dr, exi, req)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	struct vattr va;
	struct vnode *dvp;
	register struct vnode *vp;
	register char *name = args->ca_da.da_name;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_mkdir %s fh %o %d\n",
	    args->ca_da.da_name, args->ca_da.da_fhandle.fh_fsid,
	    args->ca_da.da_fhandle.fh_fno);
#endif

        /*
         *      Disallow NULL paths
         */
        if ((name == (char *) NULL) || (*name == '\0')) {
                dr->dr_status = NFSERR_ACCES;
                return;
        }

	sattr_to_vattr(&args->ca_sa, &va);
	va.va_type = VDIR;
	/*
	 * Should get exclusive flag and pass it on here
	 */
	vp = fhtovp(&args->ca_da.da_fhandle, exi);
	if (vp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		error = VOP_MKDIR(vp, name, &va, &dvp, u.u_cred);
		if (error == EEXIST) {
			/*
			 * check for dup request
			 */
			if (svckudp_dup(req)) {
				error = VOP_LOOKUP(vp, name, &dvp, u.u_cred,vp);
				if (!error) {
					error = VOP_GETATTR(dvp, &va, u.u_cred, VSYNC);
				}
			}
		}
	}
	if (!error) {
		error = VOP_GETATTR(dvp, &va, u.u_cred,VSYNC);
                if (!error) {
			/* Attributes of the newly created directory
			 * should be returned to the client.
			 */
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, dvp, exi);
                }
		VN_RELE(dvp);
	}
	dr->dr_status = puterrno(error);
	VN_RELE(vp);
	if (!error) {
		svckudp_dupsave(req);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_mkdir: returning %d\n", error);
#endif
}

/*
 * Remove a directory.
 * Remove the given directory name from the given parent directory.
 */
int
rfs_rmdir(da, status, exi, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *vp;
	struct vnode *rm_vp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_rmdir %s fh %o %d\n",
	    da->da_name, da->da_fhandle.fh_fsid, da->da_fhandle.fh_fno);
#endif
        /*
         *      Disallow NULL paths
         */
        if ((da->da_name == (char *) NULL) || (*da->da_name == '\0')) {
                *status = NFSERR_ACCES;
                return;
        }

	vp = fhtovp(&da->da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
  		error = VOP_LOOKUP(vp, da->da_name, &rm_vp, u.u_cred, vp);
  		if (!error) {
		/*
		 * Don't try to remove a directory that is a mount point.
		 */
  			if (rm_vp->v_vfsmountedhere) {
  				error = EBUSY;
				VN_RELE(rm_vp);
  				goto outahere;
  			}
			VN_RELE(rm_vp);
		}
/* HPNFS
 * To fix a CDF and retransmission problem.  Please see rfs_rename for more.
 */
		if (svckudp_dup(req)) {
			error = 0;
		} else {
			error = VOP_RMDIR(vp, da->da_name, u.u_cred);
			if (error == ENOENT) {
			       /*
			 	* check for dup request
			 	*/
				if (svckudp_dup(req)) {
					error = 0;
				}
			}
		}
	}
outahere:
	*status = puterrno(error);
	VN_RELE(vp);
	if (!error) {
		svckudp_dupsave(req);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rmdir returning %d\n", error);
#endif
}

/*	Because it takes at least 4 packets to read one CDFS directory
 *	block over a 512-byte max packet link, we let the implementations
 *	of VOP_READDIR handle the cases of unaligned offsets and sizes.
 *	It is *NOT* OK for them to return entries we have already
 *	read (no forward progress). See also comments for getdirentries().
 *	This code comes from Sun 4.0.
 *	Although it isn't spelled out in the protocol, the XDR/protocol
 *	layer insists that you send back at least one directory
 *	entry where d_ino != 0. Getdirentries, the other user of
 *	VOP_READDIR, has no such restriction, so it seems that the
 *	code to skip empty directory blocks belongs here. Sun gets
 *	around this by bypassing the VOP layer when getdirentries
 *	is for UFS.
 */
int
rfs_readdir(rda, rd, exi)
	struct nfsrddirargs *rda;
	register struct nfsrddirres  *rd;
	struct exportinfo *exi;
{
	int error;
	struct iovec iov;
	struct uio uio;
	register struct vnode *vp;
	struct direct *dp;
	int bytes_read;
	int badreclen = 0;

	vp = fhtovp(&rda->rda_fh, exi);
	if (vp == NULL) {
		rd->rd_status = NFSERR_STALE;
		return;
	}

	/*
	 * Initialize result values in case we "goto bad".
	 */

	rd->rd_offset = rda->rda_offset;
	rd->rd_size = 0;
	rd->rd_eof = FALSE;
	rd->rd_entries = NULL;
	rd->rd_bufsize = 0;

	/*
	 * check read access of dir.  we have to do this here because
	 * the opendir doesn't go over the wire.
	 */
	error = VOP_ACCESS(vp, VREAD, u.u_cred);
	if (error) {
		goto bad;
	}

	if (rda->rda_count == 0) {
		goto bad;
	}

	rda->rda_count = MIN(rda->rda_count, NFS_MAXDATA);

	/*
	 * Allocate data for entries.  This will be freed by rfs_rdfree.
	 */
	rd->rd_entries = (struct direct *)kmem_alloc((u_int)rda->rda_count);
	rd->rd_bufsize = rda->rda_count;

	/*
	 * Set up io vector to read directory data
	 */
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_seg = UIOSEG_KERNEL;
	uio.uio_offset = rda->rda_offset;
skipempty:
	iov.iov_base = (caddr_t)rd->rd_entries;
	iov.iov_len = rda->rda_count;
	/*	If looping we want the new uio_offset left by uiomove */
	uio.uio_resid = rda->rda_count;

	/*
	 * read directory
	 */
	error = VOP_READDIR(vp, &uio, u.u_cred);

	/*
	 * Clean up
	 */
	if (error) {
		rd->rd_size = 0;
		goto bad;
	}

	/*	Make sure we have one where d_ino != 0 */
	if (bytes_read = (rda->rda_count - uio.uio_resid)){
		dp = (struct direct *) rd->rd_entries;
		while ((caddr_t)dp < ((caddr_t)rd->rd_entries + bytes_read)
				&& dp->d_ino == 0) {
                        /*
                         * If the directory entry is corrupted, we could have an
                         * invalid reclen that could cause an infinite loop or
                         * a misaligned data reference through dp.
                         */
                        if ((dp->d_reclen == 0) || (dp->d_reclen & 03)) {
                            badreclen++;
                            break;
                        }
			dp = (struct direct *)((caddr_t)dp + dp->d_reclen);
		}
		if ((caddr_t)dp >= ((caddr_t)rd->rd_entries + bytes_read))
			goto skipempty;
	}

	/*
	 * set size and eof
	 * resid needs to be reduced by diff because there could be a number
	 * of bytes left to read that was greater than the amount requested
	 * by the client.  If so, then resid would be positive (and falsely
	 * report EOF) since this relationship can exist
	 *         amount_requested < bytes_left_to_read < amt2read.
	 * amtread is the actual number of bytes read.  This is used in the
	 * case where the read requested less than rsize bytes.
	 */
	if (rda->rda_count && uio.uio_resid == rda->rda_count) {
		rd->rd_size = 0;
		rd->rd_eof = TRUE;
	} else {
		rd->rd_size = rda->rda_count - uio.uio_resid;
		rd->rd_eof = FALSE;
	}
	rd->rd_offset = uio.uio_offset - rd->rd_size;

        if (badreclen) {
           /*
            * Return EINVAL to be consistent with cdfs and ufs errors.
            * If you hit this code, you  should fsck your file system.
            */
           error = EINVAL;
           printf("\n%s: rfs_readdir: bad directory: mangled entry.\n",
                 vp->v_vfsp->vfs_name);
        }

bad:
	rd->rd_status = puterrno(error);
	VN_RELE(vp);
}

rfs_rddirfree(rd)
	struct nfsrddirres *rd;
{

	kmem_free((caddr_t)rd->rd_entries, /* ignored */ (u_int)rd->rd_bufsize);
}

rfs_statfs(fh, fs, exi)
	fhandle_t *fh;
	register struct nfsstatfs *fs;
	struct exportinfo *exi;
{
	int error;
	struct statfs sb;
	register struct vnode *vp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "rfs_statfs fh %o %d\n", fh->fh_fsid, fh->fh_fno);
#endif
	vp = fhtovp(fh, exi);
	if (vp == NULL) {
		fs->fs_status = NFSERR_STALE;
		return;
	}
	error = VFS_STATFS(vp->v_vfsp, &sb);
	fs->fs_status = puterrno(error);
	if (!error) {
		fs->fs_tsize = nfstsize();
/* HPNFS  Name conflict over fs_bsize, see nfs.h	*/
		fs->fs_bsize_nfs = sb.f_bsize;
		fs->fs_blocks = sb.f_blocks;
		fs->fs_bfree = sb.f_bfree;
		fs->fs_bavail = sb.f_bavail;
	}
	VN_RELE(vp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "rfs_statfs returning %d\n", error);
#endif
}

/*ARGSUSED*/
rfs_null(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	/* do nothing */
	return (0);
}

/*ARGSUSED*/
rfs_error(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	return (EOPNOTSUPP);
}

int
nullfree()
{
}

/*
 * rfs dispatch table
 * Indexed by version,proc
 */

struct rfsdisp {
	int	  (*dis_proc)();	/* proc to call */
	xdrproc_t dis_xdrargs;		/* xdr routine to get args */
	int	  dis_argsz;		/* sizeof args */
	xdrproc_t dis_xdrres;		/* xdr routine to put results */
	int	  dis_ressz;		/* size of results */
	int	  (*dis_resfree)();	/* frees space allocated by proc */
} rfsdisptab[][RFS_NPROC]  = {
	{
	/*
	 * VERSION 2
	 * Changed rddirres to have eof at end instead of beginning
	 */
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof(fhandle_t),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_saargs, sizeof(struct nfssaargs),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof(struct nfsdiropargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof(fhandle_t),
	    xdr_rdlnres, sizeof(struct nfsrdlnres), rfs_rlfree},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof(struct nfsreadargs),
	    xdr_rdresult, sizeof(struct nfsrdresult), rfs_rdfree},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof(struct nfswriteargs),
	    xdr_attrstat, sizeof(struct nfsattrstat), nullfree},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_creatargs, sizeof(struct nfscreatargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof(struct nfsdiropargs),
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof(struct nfsrnmargs),
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof(struct nfslinkargs),
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_slargs, sizeof(struct nfsslargs),
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_creatargs, sizeof(struct nfscreatargs),
	    xdr_diropres, sizeof(struct nfsdiropres), nullfree},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof(struct nfsdiropargs),
	    xdr_enum, sizeof(enum nfsstat), nullfree},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof(struct nfsrddirargs),
	    xdr_putrddirres, sizeof(struct nfsrddirres), rfs_rddirfree},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof(fhandle_t),
	    xdr_statfs, sizeof(struct nfsstatfs), nullfree},
	}
};

struct rfsspace {
	struct rfsspace *rs_next;
	caddr_t		rs_dummy;
};

struct rfsspace *rfsfreesp = NULL;

int rfssize = 0;

caddr_t
rfsget()
{
	int i;
	struct rfsdisp *dis;
	caddr_t ret;

	if (rfssize == 0) {
		for (i = 0; i < 1 + VERSIONMAX - VERSIONMIN; i++) {
			for (dis = &rfsdisptab[i][0];
			     dis < &rfsdisptab[i][RFS_NPROC];
			     dis++) {
				rfssize = MAX(rfssize, dis->dis_argsz);
				rfssize = MAX(rfssize, dis->dis_ressz);
			}
		}
	}

	if (rfsfreesp) {
		ret = (caddr_t)rfsfreesp;
		rfsfreesp = rfsfreesp->rs_next;
	} else {
		ret = kmem_alloc((u_int)rfssize);
	}
	return (ret);
}

rfsput(rs)
	struct rfsspace *rs;
{
	rs->rs_next = rfsfreesp;
	rfsfreesp = rs;
}


/*
 * If nfs_portmon is set, then clients are required to use
 * privileged ports (ports < IPPORT_RESERVED) in order to get NFS services.
 */
int nfs_portmon = 0;



void
rfs_dispatch(req, xprt)
	struct svc_req *req;
	register SVCXPRT *xprt;
{
	int which;
	int vers;
	caddr_t	*args = NULL;
	caddr_t	*res = NULL;
	register struct rfsdisp *disp = (struct rfsdisp *)0;
	struct ucred *tmpcr;
	struct ucred *newcr = NULL;
	int error;
	struct exportinfo *exi = NULL;
	label_t qsave;
        int     anon_ok = 0;    /* Fix for bug 1038302 - corbin */
#ifdef	MP
	sv_sema_t rfs_disSS;
#endif
#ifdef	FSD_KI
	struct timeval	starttime;
#endif	FSD_KI

	svstat.ncalls++;
	error = 0;
	which = req->rq_proc;
	if (which < 0 || which >= RFS_NPROC) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 2,
		    "rfs_dispatch: bad proc %d\n", which);
#endif
		svcerr_noproc(req->rq_xprt);
		error++;
		goto done;
	}
/*FFM  Good for debugging
printf("Serving for procedure %d\n", which);
*/
	vers = req->rq_vers;
	if (vers < VERSIONMIN || vers > VERSIONMAX) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 2,
		    "rfs_dispatch: bad vers %d low %d high %d\n",
		    vers, VERSIONMIN, VERSIONMAX);
#endif
		svcerr_progvers(req->rq_xprt, (u_long)VERSIONMIN,
		    (u_long)VERSIONMAX);
		error++;
		goto done;
	}
	vers -= VERSIONMIN;
	disp = &rfsdisptab[vers][which];

	/*
	 * Clean up as if a system call just started
	 */
	u.u_error = 0;

	/*
	 * Allocate args struct and deserialize into it.
	 */
	args = (caddr_t *)rfsget();
	bzero((caddr_t)args, rfssize);
	if ( ! SVC_GETARGS(xprt, disp->dis_xdrargs, args)) {
		svcerr_decode(xprt);
		error++;
		goto done;
	}
	/*
         * Find export information and check authentication,
         * setting the credential if everything is ok.
         */
        if (which != RFS_NULL) {
                /*
                 * XXX: this isn't really quite correct. Instead of doing
                 * this blind cast, we should extract out the fhandle for
                 * each NFS call. What's more, some procedures (like rename)
                 * have more than one fhandle passed in, and we should check
                 * that the two fhandles point to the same exported path.
                 */
		/* I fixed rfs_rename and rfs_link to check the to args.
		 * Its still not perfect but its better than Sun's code. cwb
 		 */
                fhandle_t *fh = (fhandle_t *) args;
/* HPNFS
 * Since HP-UX has the setprvgrp() call, the nfs server process needs to have
 * the p_flag set so anything privileges inherited by belonging to special
 * groups will be inherited for process.  I think that only chown and file
 * locking are affected, so the setting could be moved there even though
 * this is a more general place to do it.
 * HPNFS
 */
                SPINLOCK(sched_lock);
                u.u_procp->p_flag |= SPRIV;
                SPINUNLOCK(sched_lock);
                /*
                 * Fix for bug 1038302 - corbin
                 * There is a problem here if anonymous access is
                 * disallowed.  If the current request is part of the
                 * client's mount process for the requested filesystem,
                 * then it will carry root (uid 0) credentials on it, and
                 * will be denied by checkauth if that client does not
                 * have explicit root=0 permission.  This will cause the
                 * client's mount operation to fail.  As a work-around,
                 * we check here to see if the request is a getattr or
                 * statfs operation on the exported vnode itself, and
                 * pass a flag to checkauth with the result of this test.
                 *
                 * The filehandle refers to the mountpoint itself if
                 * the fh_data and fh_xdata portions of the filehandle
                 * are equal.
                 */

                if (   (which == RFS_GETATTR || which == RFS_STATFS)
                     && (fh->fh_len == fh->fh_xlen)
                     && (bcmp(fh->fh_data, fh->fh_xdata, fh->fh_len) == 0)) {
                        anon_ok = 1;
                }

                newcr = crget();
                tmpcr = u.u_cred;
                u.u_cred = newcr;
                exi = findexport(&fh->fh_fsid, (struct fid *) &fh->fh_xlen);
                if (exi != NULL && !checkauth(exi, req, newcr, anon_ok)) {
                        svcerr_weakauth(xprt);
                        error++;
			goto done;
		}
	}

	if (nfs_portmon) {
		/*
		* Check for privileged port number
		*/
		static count = 0;
		if (ntohs(xprt->xp_raddr.sin_port) >= IPPORT_RESERVED) {
			svcerr_weakauth(xprt);
			if (count == 0) {
				NS_LOG_INFO(LE_NFS_UNPRIV_REQ,NS_LC_WARNING,
				    NS_LS_NFS,0,1, xprt->xp_raddr.sin_addr, 0);
			}
			count++;
			count %= 256;
			error++;
			goto done;
		}
	}


	/*
	 * Allocate results struct.
	 */
	res = (caddr_t *)rfsget();
	bzero((caddr_t)res, rfssize);

	svstat.reqs[which]++;

	if (exi != NULL) {
                exi->exi_refcnt++;
        }

        /*
         * Need to catch exiting nfsd here to decr exi_refcnt
         */
        qsave = u.u_qsave;
        if (setjmp(&u.u_qsave)) {
                if (exi != NULL) {
                        if (--exi->exi_refcnt == 0 &&
                                exi->exi_flags & EXI_WANTED)
                                wakeup((caddr_t)exi);
                }
                u.u_qsave = qsave;
                longjmp(&u.u_qsave);
        }

	PXSEMA(&filesys_sema, &rfs_disSS);
	/*
	 * Call service routine with arg struct and results struct
	 */
#ifdef	FSD_KI
	/* stamp bp with enqueue time */
	KI_getprectime(&starttime);
#endif	FSD_KI
	(*disp->dis_proc)(args, res, exi, req);
	VXSEMA(&filesys_sema, &rfs_disSS);
#ifdef	FSD_KI
	KI_rfs_dispatch(args, req, res, &starttime);
#endif	FSD_KI

	/*
	 * HPNFS
	 * Translate any non-zero status returned
	 */
	if ( ((struct gen_result *) res)->gen_status)	
		((struct gen_result *) res)->gen_status = (enum nfsstat)
		   hp_to_nfs_errors( (int) (((struct gen_result *) res)->gen_status));

        if (exi != NULL) {
                if (--exi->exi_refcnt == 0 &&
                        exi->exi_flags & EXI_WANTED)
                        wakeup((caddr_t)exi);
                exi = NULL;
        }
        u.u_qsave = qsave;

done:
        /* NIKE - Moved SVC_FREEARGS call after reply send - this change
         * is most beneficial for write which has a lot of resources to
         * free.  NO, DON'T DO, can cause hangs.  cwb
         */

	/*
	 * Free arguments struct
	 */
        if (!SVC_FREEARGS(xprt,  disp != NULL ? disp->dis_xdrargs : NULL,
                disp != NULL ? args : NULL)) {
                error++;
        }

	/*
	 * Serialize and send results struct
	 */
	/*
	 * Don't reply if the routine we called wanted us to act like
	 * we dropped the packet.  This relies on the first field of
	 * the res structure being the status.
	 */
	if (!error && ( *(int *)res != EDROPPACKET)) {
		if (!svc_sendreply(xprt, disp->dis_xdrres, (caddr_t)res)) {
			error++;
		}
	}

	/*
	 * Free results struct
	 */
	if (res != NULL) {
		if ( disp->dis_resfree != nullfree ) {
			(*disp->dis_resfree)(res);
		}
		rfsput((struct rfsspace *)res);
	}

	if (args != NULL) {
		rfsput((struct rfsspace *)args);
	}

	/*
	 * restore original credentials
	 */
	if (newcr) {
		crfree(u.u_cred);
		u.u_cred = tmpcr;
	}
	svstat.nbadcalls += error;
}

/*
 * Determine if two addresses are equal
 * Only AF_INET supported for now
 */
eqaddr(addr1, addr2)
	struct sockaddr *addr1;
	struct sockaddr *addr2;
{

	if (addr1->sa_family != addr2->sa_family) {
		return (0);
	}
	switch (addr1->sa_family) {
	case AF_INET:
		return (((struct sockaddr_in *) addr1)->sin_addr.s_addr ==
			((struct sockaddr_in *) addr2)->sin_addr.s_addr);
#ifdef notdef
	case AF_NS:
		/* coming soon? */
		break;
#endif /* notdef */
	}
	return (0);
}

hostinlist(sa, addrs)
	struct sockaddr *sa;
	struct exaddrlist *addrs;
{
	int i;

	for (i = 0; i < addrs->naddrs; i++) {
		if (eqaddr(sa, &addrs->addrvec[i])) {
			return (1);
		}
	}
	return (0);
}

#ifdef notdef  /* We don't have AUTH_DES yet, dhb */
/*
 * Check to see if the given name corresponds to a
 * root user of the exported filesystem.
 */
rootname(ex, netname)
	struct export *ex;
	char *netname;
{
	int i;
	int namelen;

	namelen = strlen(netname) + 1;
	for (i = 0; i < ex->ex_des.nnames; i++) {
		if (bcmp(netname, ex->ex_des.rootnames[i], namelen) == 0) {
			return (1);
		}
	}
	return (0);
}
#endif /* notdef */

checkauth(exi, req, cred, anon_ok)
	struct exportinfo *exi;
	struct svc_req *req;
	struct ucred *cred;
	int    anon_ok;
{
	struct authunix_parms *aup;
	int flavor;
	gid_t *gp;

	/*
	 * Set uid, gid, and gids to auth params
	 */
	flavor = req->rq_cred.oa_flavor;
	if (flavor != exi->exi_export.ex_auth) {
		flavor = AUTH_NULL;
                /*
                 * Fix for bug 1038302 - corbin
                 * only allow anon override if credentials are of the
                 * correct flavor.  XXX is this really necessary?
                 */
                anon_ok = 0;
	}
	switch (flavor) {
	case AUTH_NULL:
		cred->cr_uid = exi->exi_export.ex_anon;
		cred->cr_gid = exi->exi_export.ex_anon;
		gp = cred->cr_groups;
		break;

	case AUTH_UNIX:
		aup = (struct authunix_parms *)req->rq_clntcred;

		/* Somebody figured out that since cr_uid is only 16 bits
		 * but aup_uid is 32, that they could send a non-zero aup_uid
		 * that has the low 16 bits zero and get root permissions.
		 * What will they think of next.  So if someone sends an
		 * invalid uid they become anonymous.
		 * The second part of this check is if you are root but not
		 * in the export list for root you become anonymous.
		 */
		if (((unsigned) aup->aup_uid >= MAXUID) ||
		    (aup->aup_uid == 0 &&
		     !hostinlist((struct sockaddr *)svc_getcaller(req->rq_xprt),
				&exi->exi_export.ex_unix.rootaddrs))) {
			cred->cr_uid = exi->exi_export.ex_anon;
			cred->cr_gid = exi->exi_export.ex_anon;
			gp = cred->cr_groups;
		} else {
			cred->cr_uid = aup->aup_uid;
			cred->cr_gid = aup->aup_gid;
			bcopy((caddr_t)aup->aup_gids, (caddr_t)cred->cr_groups,
			    aup->aup_len * sizeof (cred->cr_groups[0]));
			gp = &cred->cr_groups[aup->aup_len];
		}
		break;


	case AUTH_DES:
#ifdef notdef  /* We don't have AUTH_DES yet, cwb */
		adc = (struct authdes_cred *)req->rq_clntcred;
		if (adc->adc_fullname.window > exi->exi_export.ex_des.window) {
			return (0);
		}
		if (!authdes_getucred(adc, &cred->cr_uid, &cred->cr_gid,
		    &grouplen, cred->cr_groups)) {
			if (rootname(&exi->exi_export,
			    adc->adc_fullname.name)) {
				cred->cr_uid = 0;
			} else {
				cred->cr_uid = exi->exi_export.ex_anon;
			}
			cred->cr_gid = exi->exi_export.ex_anon;
			grouplen = 0;
		}
		gp = &cred->cr_groups[grouplen];
		break;
#else
		return (0);
#endif /* notdef */

	default:
		return (0);
	}

        /*
         * Set real UID/GID to effective UID/GID
         * corbin 6/19/90 - Fix bug 1029628
         */
        cred->cr_ruid = cred->cr_uid;
        cred->cr_rgid = cred->cr_gid;

	while (gp < &cred->cr_groups[NGROUPS]) {
		*gp++ = NOGROUP;
	}

	return (anon_ok || cred->cr_uid != (uid_t) -1);
}

/*
 * A stat structure for counting some of the activity that goes on.
 */

struct {
	u_int fcntl_calls;
	u_int lock_calls;
	u_int fh_searches;	/* fhtovp returned null, search list */
	u_int fh_hits;		/* fh found in lm's list */
	u_int fh_estales;	/* fh not found or cmd is SETLK read or write*/
	u_int call_backs;	/* Number of free_lock_with_lm() calls */
	u_int clean_ups;	/* Number of times clean_up_lm() called */
	u_int vnodes_held;	/* Current count, not cumulative */
	u_int slm_requests[3][4];/* Count of F_RDLCK, WRLCK, UNLCK requests */
} slmstats;	/* Server Lock Manager stats */

/*
 * nfs_fcntl() -- This is a special system call being added to support the
 * lock manager.  Basically, it is the same as the normal fcntl call, with
 * a few exceptions. (1) it works on file handles instead of file descriptors.
 * (2) it only supports lock requests (at the moment).  However, this
 * code is structured such that it should be easy to add new cmds as
 * desired, e.g. to change kernel NFS params.
*/

nfs_fcntl()
{
	register struct a {
		fhandle_t	*fhp;
		int	cmd;
		union	{
			int val;
			struct flock *lockdes;
		} arg;
	} *uap;
	fhandle_t	fhandle;
	struct flock	flock;
	struct vnode *vp;
	struct vnode *search_lm_list_for_fh();
	struct exportinfo *exi;

	uap = (struct a *)u.u_ap;
	slmstats.fcntl_calls++;

	/*
 	 * Only the super-user is allowed to do nfs_fcntl() calls.
	 * NOTE: u.u_error is set as a side-effect of suser().
	 */
	if (!suser()) {
		return;
	}

	PSEMA(&filesys_sema);
	switch(uap->cmd) {

	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:

		/*
		 * First do the work of copying in the file handle
		 * and the flock structure.  Boring stuff.
		 */

		if ( copyin( uap->fhp, &fhandle, sizeof ( fhandle ) )) {
			u.u_error = EFAULT;
			VSEMA(&filesys_sema);
			return;
		}

		if (copyin(uap->arg.lockdes, &flock, sizeof(struct flock))) {
			u.u_error = EFAULT;
			VSEMA(&filesys_sema);
			return;
		}

                exi = findexport(&fhandle.fh_fsid,
			(struct fid *) &fhandle.fh_xlen);
		vp = fhtovp(&fhandle, exi);
		if ( vp == NULL ) {
		    /*
		     * Weirdness here, and a kludge to get around Sun
		     * design bogosities.  It is possible that the file
		     * has been removed (fhtovp returns NULL), but the
		     * vp is still in our list, but with a link count on
		     * the inode of zero.  In that case the fhtovp() above
		     * returns NULL (ESTALE).  Thus, we will return ESTALE
		     * on GETLK and any attempts to set a lock.  However,
		     * we allow UNLOCK requests to work, since that will
		     * eventually cause the locks and vnode to get cleaned
		     * up.  We could just go ahead and clean up, but
		     * due to the LM protocol there is a high likelyhood
		     * that Sun's would go into an infinite loop!
		     */
		    slmstats.fh_searches++;
		    vp = search_lm_list_for_fh(&fhandle);
		    if ( vp == NULL ) {
			u.u_error = ESTALE;
			slmstats.fh_estales++;
			VSEMA(&filesys_sema);
			return;
		    }
		    slmstats.fh_hits++;
		    if ( uap->cmd == F_GETLK || ( (uap->cmd == F_SETLK ||
			    uap->cmd == F_SETLKW) && flock.l_type != F_UNLCK)){
				slmstats.fh_estales++;
				u.u_error = ESTALE;
				VSEMA(&filesys_sema);
				return;
		    }
		    VN_HOLD(vp);
		}

		nfs_fcntl_lock( vp, uap->cmd, &fhandle, &flock );

		/*
		 * If GETLK, return the information to the user.
		 */
                if ( (uap->cmd == F_GETLK) && !u.u_error )
                        if( copyout( &flock, uap->arg.lockdes,
                                        sizeof( struct flock)))
                                u.u_error = EFAULT;

		/*
	 	 * Free vnode held from fhtovp()
		 */
		VN_RELE(vp);
		break;
	default:
		u.u_error = EINVAL;
	}
	VSEMA(&filesys_sema);
	return;
}

/*
 * nfs_fcntl_lock() -- Service lock requests for the lock manager.  Very
 * similar to fcntl() in structure, with a few gotchas, mainly having to
 * do with not having an open file.  Also, these requests DON'T block
 * because we want the lock manager to continue servicing requests even if
 * it can't get this lock.  Thus, the lock manager will queue this request,
 * and he will be notified when the lock is released.  This is accomplished
 * in the locked() code in ufs_lockf.c.
 *
 * NOTES FOR SUPPORT OF FUTURE FILE SYSTEMS:
 *
 * This routine ws originally coded to work only with UFS file systems.
 * The choice to add extra commands to the VOP_LOCKCTL list was made to
 * allow for expansion to future different types of file systems.  To
 * support this, the following actions should be taken:
 *
 * 	F_SETLK_CALLBACK : This is a blocking lock, but we don't want the
 *	mainline code to block.  Therefore when the VOP_LOCKCTL() routine
 *	detects a point where it would block, it should instead return
 *	EACCESS.  Further, it should save the function pointer in
 *	u.u_procp->call_back and call the function when a lock gets freed.
 *	The parameters should be the vnode pointer of the lock and
 *	the lower and upper bounds of what got freed.
 *
 *	F_QUERY_ANY_LOCKS : This is asking whether the calling process
 *	has any locks on the given vnode pointer.  Instead of passing in
 *	a flock pointer, a pointer to an int is given.  If the calling
 *	process has locks, the pointer should be set to 1 (true), otherwise
 *	it should be set to 0 (false).
 *
 *	F_QUERY_ANY_NFS_CALLBACKS : This is asking whether any locks on
 *	the given vnode pointer have a the flag set to call NFS back.  We need
 *	to know so we know if we put the vnode on the lock list so we can
 *	get its file handle when we are called back.
 *
 * If one of the type of requests is not supported EINVAL should be returned.
 *
 */


/*
 * Macros to add vnodes to and from a linked list.
 */
struct lm_linked_list {
	struct lm_linked_list *next_lock;
	struct vnode *vp;
	fhandle_t fh;
};

struct lm_linked_list *lm_v_lock_list;	    /* The list of vnodes the lock
					       manager has locks on. */

struct lm_linked_list *lm_v_callback_list;  /* The list of vnodes the lock
					       manager has callbacks on */

#define ADD_TO_LM_LIST(vp, fhp, list) { \
	    struct lm_linked_list *lp = (struct lm_linked_list *) kmem_alloc(sizeof(struct lm_linked_list)); \
		lp->vp = vp ; \
		lp->fh = *(fhp) ; \
		lp->next_lock = list; \
		list = lp; \
		slmstats.vnodes_held++; \
	}

#define REMOVE_FROM_LM_LIST(vp, list) { \
			struct lm_linked_list *prevlp = NULL; \
			struct lm_linked_list *lp = list; \
			\
			for ( ; lp ; lp = lp->next_lock ) { \
			    if ( lp->vp == vp ) { \
				/* found, remove from list */ \
				if ( prevlp != NULL ) \
				    prevlp->next_lock = lp->next_lock; \
				else \
				    list = lp->next_lock; \
				kmem_free(lp, sizeof(struct lm_linked_list)); \
				slmstats.vnodes_held--; \
				break; \
			    } \
			    else { \
				prevlp = lp; \
			    } \
			} \
		    }

#define ADD_TO_LOCK_LIST(vp, fhp)	\
		ADD_TO_LM_LIST(vp, fhp, lm_v_lock_list);

#define ADD_TO_CALLBACK_LIST(vp, fhp)	\
		ADD_TO_LM_LIST(vp, fhp, lm_v_callback_list);

#define REMOVE_FROM_LOCK_LIST(vp)	\
		REMOVE_FROM_LM_LIST(vp, lm_v_lock_list);

#define REMOVE_FROM_CALLBACK_LIST(vp)	\
		REMOVE_FROM_LM_LIST(vp, lm_v_callback_list);


static struct lm_linked_list *find_on_callback_list(vp)
	struct vnode *vp;
{
	struct lm_linked_list *lp = lm_v_callback_list;

	for ( ; lp ; lp = lp->next_lock ) {
		if ( lp->vp == vp ) {
			return lp;
		}
	}
	return NULL;
}

nfs_fcntl_lock( vp, cmd, fhandle, flockp )
struct vnode *vp;
int cmd;
fhandle_t *fhandle;
struct flock *flockp;
{
	struct file pfile;		/* only used as a valid param */
	off_t LB,UB;
	int held_locks_at_start, held_locks_at_end;
	int nfs_callbacks_at_start, nfs_callbacks_at_end;
	int error;
	int save_error;
	struct vattr va;

	slmstats.lock_calls++;
	/*
	 * Initialize so as to not confuse lower layers which use
	 * this field to determine how to act.
	 */
	pfile.f_flag = 0;
	/*
	 * Guarantee that only one Lock Manager is executing at a time.
	 * If another one comes up while another is executing, blow away
	 * the others stuff.  This is essentially what will be going on
	 * in user space.
	 */
	if ( lock_manager_pid != u.u_procp->p_pid  ) {
		clean_up_lm();
		lock_manager_proc = u.u_procp;
		lock_manager_pid = u.u_procp->p_pid;
		SPINLOCK(sched_lock);
		u.u_procp->p_flag2 |= SISNFSLM;
		SPINUNLOCK(sched_lock);
	}
	


	/*
	 * NOTE: most of the following checks are completely for
	 * sanity's sake.  The lock manager should always be giving
	 * us valid requests.  However, we assume nothing....
	 */
	/*
	 * Compute upper, lower bounds of request, same as always.
	 * NOTE: Only support whence's of 0, since the LOCK
	 * manager always gives us zeros.
	 */
	LB = flockp->l_start;
	if (flockp->l_whence != 0)  {
		u.u_error = EINVAL;
		return;
	}
	if (LB > MAX_LOCK_SIZE)
		LB = MAX_LOCK_SIZE;
	else
		if (LB < 0)  {
			u.u_error = EINVAL;
			return;
		}

	if (flockp->l_len == 0)
		UB = MAX_LOCK_SIZE;
	else {
		UB = LB + flockp->l_len;
		if (UB > MAX_LOCK_SIZE)
			UB = MAX_LOCK_SIZE;
		/*
		 * Sanity check, should never happen.
		 */
		if ((UB < 0) || (UB < LB)) {
			u.u_error = EINVAL;
			return;
		}
	}

	if (!(flockp->l_type == F_RDLCK || flockp->l_type == F_WRLCK ||
	      flockp->l_type == F_UNLCK))  {
			u.u_error = EINVAL;
			return;
	}

	/*
	 * Check access permissions.  Since  don't have any
	 * credentials to check against, the only thing we can
	 * really do is verify it's not a Read-Only file system.
	 */
	if (cmd == F_SETLK || cmd == F_SETLKW) {
		if ( (flockp->l_type == F_WRLCK ) &&
			((vp->v_vfsp->vfs_flag & VFS_RDONLY ) != 0 ))
		{
			u.u_error = EROFS;
			return;
		}
	}
	/*
	 * Don't allow enforcement mode locks.  We do this because the system
	 * has no way of matching the lockmanager with the nfsd's that would
	 * service a write request.  Thus, enforcement mode locks would
	 * prevent the process from being able to write to a file it had a
	 * lock on.
	 */
	if (!(u.u_error = VOP_GETATTR(vp, &va, u.u_cred, VSYNC)) &&
	    ( (cmd == F_SETLK) || (cmd == F_SETLKW) ) &&
	    (va.va_mode & S_ENFMT) && !(va.va_mode & S_IXGRP) ) {
		u.u_error = EINVAL;
		return;
	}
	slmstats.slm_requests[cmd-F_GETLK][0]++;
	if ( (cmd == F_SETLK) || (cmd == F_SETLKW) )
		slmstats.slm_requests[cmd-F_GETLK][flockp->l_type]++;

	/*
	 * Find out whether we currently hold any locks and callbacks.  This
	 * uses new fnctl() commands supported only in the kernel,
	 * F_QUERY_ANY_LOCKS to find out if we have any locks
	 * and F_QUERY_ANY_NFS_CALLBACKS to find out if any locks are marked
	 * NFS_WANTS_LOCK on this vnode. This is used later to figure out
	 * if we need to hold the vnode.  The rest of the params
	 * are only there to fill in the necessary param list.
	 * Again, we should theoretically never hit the case of
	 * getting an error here, but just in case...
	 */
	u.u_error =  VOP_LOCKCTL(vp, &held_locks_at_start,
			 F_QUERY_ANY_LOCKS, u.u_cred, &pfile, LB, UB);
	if ( u.u_error )
		return;

	u.u_error =  VOP_LOCKCTL(vp, &nfs_callbacks_at_start,
			 F_QUERY_ANY_NFS_CALLBACKS, u.u_cred, &pfile, LB, UB);
	if ( u.u_error )
		return;

	/*
	 * Do the lock request.  If the request was a blocking lock,
	 * tell the lockctl() code to call us back later.  See
	 * locked() and lockfree() in ufs_lockf.c and description
	 * at the beginning of this function and the function
	 * free_lock_with_lm below.
	 */
	if ( cmd  == F_SETLKW ) {
		cmd = F_SETLK_NFSCALLBACK;
	}

	u.u_error = VOP_LOCKCTL( vp, flockp, cmd, u.u_cred, &pfile, LB, UB);

	/*
	 * Find out if any change in the number of locks on the
	 * file by this pid has occured.  If we went from having
	 * no locks to having a lock, we have to do an extra
	 * count on the vnode to hold, because we don't have an
	 * open file.  If we went from having locks to not having
	 * any, we undo the previous hold.  Also keep track of
	 * things in a linked list so that if this process gets
	 * killed these locks and holds can get cleaned up.
	 */

	if ( ! u.u_error ) {
		u.u_error =  VOP_LOCKCTL(vp, &held_locks_at_end,
			 F_QUERY_ANY_LOCKS, u.u_cred, &pfile, LB, UB);
		if ( ! u.u_error ) {
			if (!held_locks_at_start  && held_locks_at_end ) {
				VN_HOLD(vp);
				ADD_TO_LOCK_LIST(vp,fhandle);
			}
	    		else if (held_locks_at_start && !held_locks_at_end ) {
				REMOVE_FROM_LOCK_LIST(vp);
				VN_RELE(vp);
			}
		}
	}

	/* Have to save the old u.u_error and set it to 0, because ufs_lockctl()
	 * returns u.u_error and it will be set to EACCESS if this would have
	 * been a blocking lock.  ufs_lockctl with F_QUERY_ANY_NFS_CALLBACKS,
	 * will not fail, but we will check the return code anyways.
	 */
	save_error = u.u_error;
	u.u_error = 0;
	error = VOP_LOCKCTL(vp, &nfs_callbacks_at_end,
			 F_QUERY_ANY_NFS_CALLBACKS, u.u_cred, &pfile, LB, UB);
	u.u_error = save_error;

	/*
	 * Find out if any change in the number of callbacks on the
	 * file has occured.  If we went from having
	 * no callbacks to having a callback, we have to do an extra
	 * count on the vnode to hold, because we don't have an
	 * open file.  If we went from having callbacks to not having
	 * any, we undo the previous hold.  Also keep track of
	 * things in a linked list so that if this process gets
	 * killed these holds can get cleaned up.
	 *
	 * We're doing all of this so that free_lock_with_lm() can get the
	 * file handle for the vnode.  It is called when the lock that we
	 * have marked for callback is freed.  free_lock_with_lm() can not
	 * compute the file handle, so we have to save the file handle for it.
	 */
	if ( ! error ) {
		if (!nfs_callbacks_at_start  && nfs_callbacks_at_end ) {
			VN_HOLD(vp);
			ADD_TO_CALLBACK_LIST(vp,fhandle);
		}
	    	else if (nfs_callbacks_at_start && !nfs_callbacks_at_end ) {
			REMOVE_FROM_CALLBACK_LIST(vp);
			VN_RELE(vp);
		}
	}

	if ( cmd == F_GETLK )	/* keep results for GETLK */
		slmstats.slm_requests[0][flockp->l_type]++;

	return;
}

/*
 * free_lock_with_lm() -- This routine is called when a blocking
 * lock was attempted by the NFS lock manager via nfs_fcntl().  In that
 * case, nfs_fcntl() returned an error, but the local code kept track of
 * the fact that somebody wanted the lock.  When the lock gets freed, this
 * routine will get called to let the lock manager know that it can try
 * again.
 */

/*ARGSUSED*/
void
free_lock_with_lm( vp, LB, UB, pid )
struct vnode *vp;
off_t LB, UB;
u_short pid;
{
	struct flock flock;
	lockhandle_t lh;
	int error;
	int save_error;
	int nfs_callbacks;
	struct lm_linked_list *ll;
	struct file pfile;		/* only used as a valid param */

	slmstats.call_backs++;

	/*
	 * If lock manager is no longer up, don't try to call him!
	 */
	if ( lock_manager_pid == 0 )
		return;

	/*
	 * Create flock structure for request to lock manager
	 */
	flock.l_type = F_UNLCK;		/* We are unlocking the file */
	flock.l_whence = 0;
	flock.l_pid = pid;
	/*
	 * This is a gross hack.  We ignore the UB and LB passed in and tell
	 * the lock manager we are unlocking the entire file.  This will force
	 * it to run the entire queue for this file, retrying ALL waiting
	 * requests.  This will insure that we don't lose any blocking locks
	 * Those requests that are still blocked will remain blocked when
	 * they try nfs_fcntl() again.
	 */
	flock.l_start = 0;
	flock.l_len = 0;

	/* Convert vnode into lockhandle-id. */
	
	if ((ll = find_on_callback_list(vp)) == NULL) {
		/*
	 	* Lock manager must have died and we lost this vp off the lock
	 	* list.  Nothing to do but quit since we can't create a file
		* handle without the exportinfo pointer.
	 	*/
#ifdef OSDEBUG
	printf("free_lock_with_lm Could not find vnode");
#endif
		return;
	}
	
	bcopy((caddr_t)&(ll->fh), (caddr_t) &lh.lh_id, sizeof(fhandle_t));

	/* Add in vnode and server and call to common code */
	lh.lh_vp = vp;

	/*
	 * NOTE: servername should be set from hostname variable when
	 * it gets added to the kernel.
	 */
	lh.lh_servername = utsname.nodename;

	(void) klm_lockctl(&lh, &flock, F_SETLK,u.u_cred);

	/*
	 * Initialize so as to not confuse lower layers which use
	 * this field to determine how to act.
	 */
	pfile.f_flag = 0;

	/* See if we have to remove this vnode from the callback list */

	save_error = u.u_error;
	u.u_error = 0;
	error = VOP_LOCKCTL(vp, &nfs_callbacks,
			 F_QUERY_ANY_NFS_CALLBACKS, u.u_cred, &pfile, LB, UB);
	u.u_error = save_error;

	if (!error && !nfs_callbacks) {
		REMOVE_FROM_CALLBACK_LIST(vp);
		VN_RELE(vp);
	}
}

/*
 * clean_up_lm() -- routine to clean up the lock manager.  This will be
 * called from exit() when the lock manager gets killed.  This routine is
 * responsible for cleaning up after the lock manager, including the following
 * areas:
 *
 *	(1) Free any locks held by the lock manager.
 * 	(2) Release any vnodes held by the lock manager.
 *	(3) Set flag that lock manager is no longer up.
 *
 * 1 and 2 are accomplished by keeping a linked list of vnodes that the lock
 * manager is holding because it has locks.  This list must be walked, freeing
 * all locks and the releasing the vnode.  3 is accomplished by having
 * nfs_fcntl() store its pid in a global variable.  This pid can then be
 * checked to see if the lock manager is up (in free_lock_with_lm()) or if
 * it is the lock manager that holds the lock (will be necessary in
 * nfs_write()).
 *
 * I added code to make sure that only one process is doing a clean_up_lm()
 * at any time.
 */

short clean_up_lm_lock;
short clean_up_lm_want;

clean_up_lm()
{
	struct lm_linked_list *lp, *tmplp;
	struct flock ld;
	off_t LB, UB;
	struct file fp;

	slmstats.clean_ups++;

	/*
	 * Make sure only one process is going to mess with the vnode lists
	 * at a time.
	 */
	while (clean_up_lm_lock) {
		clean_up_lm_want++;
		sleep((caddr_t) &clean_up_lm_lock, PINOD);
	}
	clean_up_lm_lock++;

	/*
	 * If v_lock_list is non-NULL, it represents a list of vnodes held
	 * by this process because locks were applied to them.  This is
	 * used by nfs_fcntl() so that if it gets hard killed it can clean up
	 * after itself.  See nfs_server.c.  Note that if more than one
	 * process decides to use this mechanism it could cause major probs.
	 */
	if ( lm_v_lock_list != NULL) {

		lp = lm_v_lock_list;
		while ( lp != NULL ) {

		    ld.l_type = F_UNLCK;    /* set to unlock entire file */
		    ld.l_whence = 0;        /* unlock from start of file */
		    ld.l_start = 0;
		    ld.l_len = 0;           /* do entire file */
		    LB = 0;
		    UB = MAX_LOCK_SIZE;	    /* NOTE: Defined in param.h */
		    fp.f_flag = 0;

		    (void) VOP_LOCKCTL(lp->vp, &ld, F_SETLK, u.u_cred, &fp, LB, UB);
		    VN_RELE(lp->vp);
		    slmstats.vnodes_held--;
		    tmplp = lp;
		    lp = lp->next_lock;
		    kmem_free(tmplp, sizeof(struct lm_linked_list));
		}
		lm_v_lock_list = NULL;
	}
	if ( lm_v_callback_list != NULL) {

		lp = lm_v_callback_list;
		while ( lp != NULL ) {

		    /* We'll just release the vnode, when free_lock_with_lm()
		     * is called it won't be found on the list and it will
		     * just exit.
		     */
		    VN_RELE(lp->vp);
		    slmstats.vnodes_held--;
		    tmplp = lp;
		    lp = lp->next_lock;
		    kmem_free(tmplp, sizeof(struct lm_linked_list));
		}
		lm_v_callback_list = NULL;
	}
	lock_manager_pid = 0;	/* assumes 0 is invalid pid */
	if ( lock_manager_proc != (struct proc *) NULL ) {
		SPINLOCK(sched_lock);
		lock_manager_proc->p_flag2 &= ~SISNFSLM;
		SPINUNLOCK(sched_lock);
	}

	/* Release the lock and wakeup anyone waiting for it */
	clean_up_lm_lock = 0;
	if (clean_up_lm_want) {
		clean_up_lm_want = 0;
		wakeup((caddr_t) &clean_up_lm_lock);
	}

        return (0);
}

/*
 * Search the LM linked list for a matching fhandle.  If found, return
 * the vnode.
 */

struct vnode *
search_lm_list_for_fh( fhp )
fhandle_t *fhp;
{
	struct lm_linked_list *lp;

	for ( lp = lm_v_lock_list ; lp != NULL ; lp = lp->next_lock ) {
		if ( bcmp(fhp, &(lp->fh), sizeof(fhandle_t)) == 0 )
			return (lp->vp);
	}

	return (NULL); /* not in list */

}

/*
 * Convert an fhandle into a vnode. (NFS 4.1 version)
 * Uses the file id (fh_len + fh_data) in the fhandle to get the vnode.
 * WARNING: users of this routine must do a VN_RELE on the vnode when they
 * are done with it.
 */
struct vnode *
fhtovp(fh, exi)
	fhandle_t *fh;
	struct exportinfo *exi;
{
	register struct vfs *vfsp;
	struct vnode *vp;
	int error;
	extern struct vfs * getvfs();

	if  (exi == NULL) {
                return (NULL);  /* not exported */
        }

loop:
	vfsp = getvfs(fh->fh_fsid);
	if (vfsp == NULL) {
		return (NULL);
	}
	/* 
	 * Check to see if an unmount is in progress.  If so sleep, letting
	 * the unmount remove the vfs pointer so the subequent check will fail.
	 *
	 */
	while (vfsp->vfs_flag & VFS_MLOCK) {
		vfsp->vfs_flag |= VFS_MWAIT;
		sleep((caddr_t)vfsp, PVFS);
		goto loop;
	}
	error = VFS_VGET(vfsp, &vp, (struct fid *)&(fh->fh_len));
	if (error || vp == NULL) {
#ifdef NFSDEBUG
		dprint(nfsdebug, 1, "fhtovp(%x) couldn't vget\n", fh);
#endif
		return (NULL);
	}
	return (vp);
}
