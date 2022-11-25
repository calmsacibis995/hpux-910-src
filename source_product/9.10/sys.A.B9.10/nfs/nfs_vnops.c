/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/nfs_vnops.c,v $
 * $Revision: 1.17.83.10 $       $Author: craig $
 * $State: Exp $        $Locker:  $
 * $Date: 93/12/16 14:25:26 $
 */

/* Kludge on top of kludge, that's NFS.
 * This flag is used to fix a problem that caused a hot site.
 * There are two processes, process A and process B.
 *	Process A is doing a stat on the file.
 *	It is asleep at the rfscall in nfs_getattr().
 *	Process B writes to the file, then does a seek to the end of the file.
 *	This causes another nfs_getattr().  The first thing is does is the
 *	nfs_fsync() to sync the file to the server if the RDIRTY bit is set.
 *	The RDIRTY is cleared, then Process B goes to sleep waiting for the
 *	writes to the server to finish.  Process A wakes up because the
 *	attributes from the server have arrived.  They are from before process
 *	B's writes however, so the file size is less than that of the rnode.
 *	But since the RDIRTY bit has been cleared, the file size of the rnode
 *	is changed to match that of the server.  When process B wakes up, it
 *	sets the file pointer to the shorter length because thats what
 *	nfs_getattr() returns.  The next write occurs over the top of the
 *	old write and data is lost.
 *	I am fixing this problem by adding a new bit that says we are flushing,
 *	and if it is set, then use the rnode size of the file, not the server
 *	size.
 *	The solution of clearing the RDIRTY bit after the flush succeeds does
 *	not work because another write could occur during the flush which would
 *	cause the RDIRTY bit to be incorrectly cleared.  The rnode is not
 *	locked during writes only during flushes.
 *	cwb 11/30/92
 */
#define RFLUSHING 0x80

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../ufs/fsdir.h"
#include "../h/errno.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../netinet/in.h"
#include "../h/proc.h"
#include "../rpc/types.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../rpc/xdr.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"
#include "../h/vmmac.h"
#include "../h/kern_sem.h"
#include "../h/stat.h"
#include "../nfs/snode.h"
#include "../nfs/lockmgr.h"
#ifdef POSIX
#include "../h/unistd.h"	/* for _PC_* */
#include "../h/tty.h"		/* for TTYHOG */
#endif POSIX
#ifdef	FSD_KI
#include "../h/ki_calls.h"
#endif	FSD_KI

#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"
#include "../h/debug.h"
#include "../machine/vmparam.h"	/* For KERNELSPACE */

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

/*
 * These macros are used make sure that the code will not use credentials
 * that are out of date or use a file handle that refers to a file that
 * is stale.  They are from NFS/3.2
 */

#define  check_stale_fh(errno, vp) if ((errno) == ESTALE) { dnlc_purge_vp(vp); }
#define  nfsattr_inval(vp)   (vtor(vp)->r_nfsattrtime.tv_sec = 0)

#define ISVDEV(t) ((t == VBLK) || (t == VCHR) || (t == VFIFO) || t == VSOCK)

extern int strcmp();

struct vnode *makenfsnode();
struct vnode *dnlc_lookup();
char *newname();

/*
 * These are the vnode ops routines which implement the vnode interface to
 * the networked file system.  These routines just take their parameters,
 * make them look networkish by putting the right info into interface structs,
 * and then calling the appropriate remote routine(s) to do the work.
 */

int nfs_open();
int nfs_close();
int nfs_rdwr();
int nfs_ioctl();
int nfs_select();
int nfs_getattr();
int nfs_setattr();
int nfs_access();
int nfs_lookup();
int nfs_create();
int nfs_remove();
int nfs_link();
int nfs_rename();
int nfs_mkdir();
int nfs_rmdir();
int nfs_readdir();
int nfs_symlink();
int nfs_readlink();
int nfs_fsync();
int nfs_inactive();
int nfs_bmap();
int nfs_strategy();
int nfs_badop();
int vfs_prefill();
int vfs_pagein();
int vfs_pageout();
int nfs_notsupported();
int nfs_noop();
#ifdef POSIX
int nfs_pathconf();
#endif POSIX
int nfs_lockctl();
int nfs_lockf();


struct vnodeops nfs_vnodeops = {
	nfs_open,
	nfs_close,
	nfs_rdwr,
	nfs_ioctl,
	nfs_select,
	nfs_getattr,
	nfs_setattr,
	nfs_access,
	nfs_lookup,
	nfs_create,
	nfs_remove,
	nfs_link,
	nfs_rename,
	nfs_mkdir,
	nfs_rmdir,
	nfs_readdir,
	nfs_symlink,
	nfs_readlink,
	nfs_fsync,
	nfs_inactive,
	nfs_bmap,
	nfs_strategy,
	nfs_badop,	/* bread */
	nfs_badop,	/* brelse */
	nfs_notsupported,	/* pathsend, only used with/by DUX */
#ifdef ACLS
	nfs_notsupported,	/* setacl */
	nfs_notsupported,	/* getacl */
#endif ACLS
#ifdef POSIX
	nfs_pathconf,	/* pathconf */
	nfs_pathconf,	/* fpathconf (same as pathconf?) */
#endif POSIX
	nfs_lockctl,	/* lockctl() */
	nfs_lockf,	/* lockf() */
	nfs_noop,	/* nfs_fid */ /* was nfs_notsupported, */
	nfs_notsupported,	/* fscntl() */
	vfs_prefill,
	vfs_pagein,
	vfs_pageout,
	NULL,
	NULL,
};

/*ARGSUSED*/
int
nfs_open(vpp, flag, cred)
	register struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	struct vattr va;
	int error;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_open %s %o %d flag %d\n",
	    vtomi(*vpp)->mi_hostname,
	    vtofh(*vpp)->fh_fsid, vtofh(*vpp)->fh_fno,
	    flag);
#endif
	if ((flag & FWRITE) && ((*vpp)->v_vfsp->vfs_flag & VFS_RDONLY)) {
		u.u_error = EROFS;
		return EROFS;
	}
	/*
	 * validate cached data by getting the attributes, unless -nocto
	 * option was specified when the filesystem was mounted.
	 */
	if (vtomi(*vpp)->mi_nocto) {
	    error = 0;
	}
	else {
		nfsattr_inval(*vpp);
		error = nfs_getattr(*vpp, &va, cred);
	}
	if (!error) {
		vtor(*vpp)->r_flags |= ROPEN;
	}
	return (error);
}

/*ARGSUSED*/
int
nfs_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct rnode *rp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_close %s %o %d flag %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
	    flag);
#endif

	rp = vtor(vp);
	rp->r_flags &= ~ROPEN;
	/*
	 * If this is a close of a file open for writing or an unlinked
	 * open file or a file that has had an asynchronous write error,
	 * flush synchronously. This allows us to invalidate the file's
	 * buffers if there was a write error or the file was unlinked.
	 * Invalidating the buffers kills their references to the vnode
	 * so that it will free up quickly.
	 * ENOSPC fix Added code from 3.2 to do a binvalfree and
	 * dnlc_purge_vp instead of just a binval   Mike Shipley 09/01/87
	 */
	if (flag & FWRITE || rp->r_unldvp != NULL || rp->r_error) {
		(void) nfs_fsync(vp, cred);
	}
	if (rp->r_unldvp != NULL || rp->r_error) {
		binvalfree(vp);
		dnlc_purge_vp(vp);
	}
	return (flag & FWRITE? rp->r_error: 0);
}

int
nfs_rdwr(vp, uiop, rw, ioflag, cred)
	register struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	int error = 0;
	struct vattr va;
	struct rnode *rp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rdwr: %s %o %d %s %x %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
	    rw == UIO_READ ? "READ" : "WRITE",
	    uiop->uio_iov->iov_base, uiop->uio_iov->iov_len);
#endif
	if (vp->v_type != VREG) {
		return (EISDIR);
	}

	if (rw == UIO_WRITE && (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		u.u_error = EROFS;
		return EROFS;
	}


        /* 
         * PHKL_2836
         *
         * This change solves the following problem:
         *         
         * There are two processes: P1 and P2
         *
         * P1 is uid user1 P2 is uid user2 
         * P1 and P2 are both gid sys
         * 
         * P1 owns and opens a file for O_WRONLY and begins writing 
  	 * it with permissions 640. 
         *
         * P2 starts doing a tail -f on the file. 
         *
         * Within a few writes of P1 and reads of P2, P1 recieves an
         * error (Permission denied - EACCES) and continues to do so
         * for each successive write.
         *
         * The problem is caused by the fact that P2 performing reads
         * over-writes the credentials in the rnode of the file. Since
         * the write buffers are still in memory (i.e. they are delayed
         * writes) once they go to be posted accross the lan, they
         * fail with EACCES because they attempted to use the credentials
         * of P2 (the read process) instead of the credentials for P1.
         *
         * This problem is solved by splitting the r_cred field of the
         * rnode into two fields (r_rcred and r_wcred) the last known
         * read credential is stored in r_rcred and the last known
	 * write credential is stored in r_wcred. Now, when do_bio 
	 * performs an nfs_write or nfs_read, uses r_wcred and r_rcred 
	 * instead of r_cred exclusively and thus avoids the EACCES problem. 
	 * 
	 * Note: We don't need to worry about the permissions on the file
	 * being changed with chmod because once the file is open, the
	 * current permissions don't pertain to processes already accessing 
         * the file.
	 *
	 * This solution also incorporates a fix for NFS which differs
	 * from the SUN implementation. In the read case, the credentials
	 * are over-written if the read credentials (cred) are not the
	 * same for this read as the the read credential in the vnode 
         * (r_rcred). 
      	 * 
	 * In the SUN implementation, the read credential was only
	 * over-written if the rnode read credential was NULL. This	
	 * resulted in problems if root was the first one to read or
	 * write the file and root did not have read priviledges accross
	 * the nfs mount. This error would cause a situation where no one
	 * else	could read the file even though they had the proper 
	 * permissions because the read credential in the rnode would 
	 * never be over-written. 
         *
         */

	rp = vtor(vp);
        if (rw == UIO_WRITE && rp->r_wcred != cred) {
                crhold(cred);
                if (rp->r_wcred) {
                        crfree(rp->r_wcred);
                }
                rp->r_wcred = cred;
        } else {
                if (rw == UIO_READ && rp->r_rcred != cred) {
                        crhold(cred);
                        if (rp->r_rcred) {
                                crfree(rp->r_rcred);
                        }
                        rp->r_rcred = cred;
                }
        }


#ifdef notdef
	if (ioflag & IO_UNIT) {
		rlock(rp);
	}
#endif
	/* Added this SUN 4.1 code, cwb */
        /* Fix for 1045993, huey */
        /* Do an OTW getattr if file is locked */
        if (rp->r_flags & RNOCACHE) {
                struct vattr va;

                error = nfsgetattr(vp, &va, cred);
                if (error)
                        goto out;
        }

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		rlock(rp);

		/*
		 * HPNFS
		 * Added extra paramter to match DUX requirements
		 * VSYNC causes a sync on the file before the
		 * attr's are gotten
		 * HPNFS
		*/
		error = VOP_GETATTR(vp, &va, cred, VSYNC);

		if (!error) {
			uiop->uio_offset = rp->r_size;
		}
	}

	if (!error) {
                error = rwvp(vp, uiop, rw, cred);
	}

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		runlock(rp);
	}

out:

#ifdef notdef
	if (ioflag & IO_UNIT) {
		runlock(rp);
	}
#endif
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rdwr returning %d\n", error);
#endif
	return (error);
}

int
rwvp(vp, uio, rw, cred)
	register struct vnode *vp;
	register struct uio *uio;
	enum uio_rw rw;
        struct ucred *cred;
{
	struct buf *bp;
	struct rnode *rp;
	daddr_t bn;
	register int n, on;
	int size, total, diff;
	int error = 0;
	struct vnode *mapped_vp;
	daddr_t mapped_bn, mapped_rabn[2];
	int ra_size[3];
	int eof = 0;
	extern struct buf *breadan();

	if (uio->uio_resid == 0) {
		return (0);
	}
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0) {
		return(EINVAL);
	}

	total = uio->uio_resid;
	size = vtoblksz(vp);
	size &= ~(DEV_BSIZE - 1);
	if (size <= 0) {
		panic("rwvp: zero size");
	}
	rp = vtor(vp);
	do {
		bn = uio->uio_offset / size;
		on = uio->uio_offset % size;
		n = MIN((unsigned)(size - on), uio->uio_resid);
		VOP_BMAP(vp, bn, &mapped_vp, &mapped_bn);
		if (rp->r_flags & RNOCACHE) {
			bp = geteblk(size);
			if (rw == UIO_READ) {
				error = nfsread(vp, bp->b_un.b_addr+on,
				    uio->uio_offset, n, &bp->b_resid,cred);
				if (error) {
					brelse(bp);
					goto bad;
				}
				/* Make sure the size of the file gets updated
				 * if it has changed. cwb */
				if ( rp->r_size < rp->r_nfsattr.na_size) {
					rp->r_size = rp->r_nfsattr.na_size;
				}
			}
		}
		else
		if (rw == UIO_READ) {

			if ((long) bn < 0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else {
				if (incore(mapped_vp, mapped_bn)) {

					/*
					 * check attributes to determine whether
					 * incore data is stale
					 */
					if ( (error =
						nfs_validate_caches(mapped_vp,
							u.u_cred))  ) {
						goto bad;
					}
				}
				diff = (int)rp->r_size - uio->uio_offset;
				if (rp->r_lastr + 1 == bn && diff > size) {
					VOP_BMAP(vp, bn + 1,
						 &mapped_vp, &mapped_rabn[0]);
					ra_size[1] = size;
					ra_size[2] = size;
					if (diff > (size << 1)) {
						VOP_BMAP(vp, bn + 2,
                                                &mapped_vp, &mapped_rabn[1]);
						ra_size[0] = 2;
					} else
						ra_size[0] = 1;
				} else
					ra_size[0] = 0;
				bp = breadan(mapped_vp, mapped_bn, size,
#ifdef	FSD_KI
					mapped_rabn, ra_size, B_rwvp|B_data);
#else	FSD_KI
					mapped_rabn, ra_size);
#endif	FSD_KI
			}
		} else {
			int i, count;

			if (rw == UIO_WRITE && vp->v_type == VREG &&
	    		((uio->uio_offset + n - 1) >> 9 )   >=
			u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
				n = ((u.u_rlimit[RLIMIT_FSIZE].rlim_cur << 9)
				- uio->uio_offset);

				if (n <= 0)

					if (total == uio->uio_resid)
						return(EFBIG);
					else {
						if (error == 0)
							error = u.u_error;
						return(error);
					}
			}

			/*
			 * ENOSPC fix from 3.2.  If there was an asynchronous
			 * error, then all resulting IO on this rnode will
			 * be given an error.  Mike Shipley 09/01/87
			 */
			if (rp->r_error) {
				error = rp->r_error;
				goto bad;
			}

			count = howmany(size, DEV_BSIZE);
			for (i = 0; i < count; i += ptob(1)/DEV_BSIZE)
				munhash(vp, (daddr_t)(mapped_bn + i));
			if (n == size) {
#ifdef	FSD_KI
				bp = getblk(mapped_vp, mapped_bn, size,
						B_rwvp|B_data);
#else	FSD_KI
				bp = getblk(mapped_vp, mapped_bn, size);
#endif  FSD_KI
			} else {
#ifdef	FSD_KI
				bp = bread(mapped_vp, mapped_bn, size,
						B_rwvp|B_data);
#else	FSD_KI
				bp = bread(mapped_vp, mapped_bn, size);
#endif	FSD_KI
			}
		}
		if (bp->b_flags & B_ERROR) {
			error = geterror(bp);
			brelse(bp);
			goto bad;
		}
		/*
		 * Check was moved here to catch both the NOCACHE and CACHE
		 * case with NFS 3.2 file locking.
		 */
		if (rw == UIO_READ) {
			int difference;

			rp->r_lastr = bn;
			/*
			 * Handle cases at EOF.  Two things to worry about:
			 * (1) Read return x bytes, but in the mean time the
			 * file has been truncated.  In this case we truncate
			 * the amount we return also.  (2) read returns x
			 * bytes, x < n, but in the mean time the file size
			 * has increased to y, such that x < y < n.  Still
		 	 * only want to return x bytes.
			 */
			difference = MIN((int)rp->r_size - uio->uio_offset,
					n - (int)bp->b_resid);
			if (difference <= 0) {
				/* HPNFS
				 * If an error occurs when the offset is at the
				 * EOF, then you can't return 0 or the error
				 * will get lost.
				 */
				if (bp->b_flags & B_ERROR) {
					brelse(bp);
					return(geterror(bp));
				}
				brelse(bp);
				return(0);
			}
			if (difference < n) {
				n = difference;
				eof = 1;
			}
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if (rw == UIO_READ) {
			brelse(bp);
		} else {
			/*
			 * r_size is the maximum number of bytes known
			 * to be in the file.
			 * Make sure it is at least as high as the last
			 * byte we just wrote into the buffer.
			 */
			if (rp->r_size < uio->uio_offset) {
				rp->r_size = uio->uio_offset;
			}
                        if (rp->r_flags & RNOCACHE) {
                                error = nfswrite(vp, bp->b_un.b_addr+on,
                                    uio->uio_offset-n, n, cred);
                                brelse(bp);
                        } else  {
			rp->r_flags |= RDIRTY;

			/*
			 * If this file was opened with O_SYNCIO specified,
			 * do a bwrite so the biod's won't handle the req
			 */
			if (uio->uio_fpflags & FSYNCIO)
				bwrite(bp);
			else if (n + on == size && !(bp->b_flags & B_REWRITE)) {
				bawrite(bp);
				bp->b_flags |= B_REWRITE;
			} else {
				bdwrite(bp);
			}
                        }
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && !eof && n != 0);
	if (rw == UIO_WRITE && uio->uio_resid && u.u_error == 0) {
		NS_LOG_INFO5(LE_NFS_RWVP_SHORT_WRITE,NS_LC_ERROR,NS_LS_NFS,0,
			3,uio->uio_resid, vp, bn,0,0);
	}
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */

        /* If there was an ENOSPC write error, set the resid back to
	   the original number of bytes to write.  In rwuio a check is
	   going to be made to see if anything was written.  If any part
	   of the file is written, the ENOSPC error gets cleared (POSIX).
	   For NFS resid gets set to 0 as soon as all the data is put into
	   the buf and unless we reset resid back, it's going to look like
	   some (all) of the file was written and the ENOSPC will be
	   cleared. */
        if (error == ENOSPC)
                uio->uio_resid = total;
bad:
	return (error);
}

/*
 * Write to file.
 * Writes to remote server in largest size chunks that the server can
 * handle.  Write is synchronous.
 */
nfswrite(vp, base, offset, count, cred)
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	struct ucred *cred;
{
	int error;
	struct nfswriteargs wa;
	struct nfsattrstat *ns;
	int tsize;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfswrite %s %o %d offset = %d, count = %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
	    offset, count);
#endif
	ns = (struct nfsattrstat *)kmem_alloc((u_int)sizeof(*ns));
	do {
		tsize = min(vtomi(vp)->mi_curwrite, count);
		wa.wa_data = base;
		wa.wa_fhandle = *vtofh(vp);
		wa.wa_begoff = offset;
		wa.wa_totcount = tsize;
		wa.wa_count = tsize;
		wa.wa_offset = offset;
		error = rfscall(vtomi(vp), RFS_WRITE, xdr_writeargs, (caddr_t)&wa,
			xdr_attrstat, (caddr_t)ns, cred);
		if (!error) {
			error = geterrno(ns->ns_status);
			check_stale_fh(error, vp);
		}
		/* Print full file system messages */
		if (error == ENOSPC) {
			printf("\n%s: file system full\n", vtomi(vp)->mi_fsmnt);
		}
#ifdef NFSDEBUG
		dprint(nfsdebug, 3, "nfswrite: sent %d of %d, error %d\n",
		    tsize, count, error);
#endif
		count -= tsize;
		base += tsize;
		offset += tsize;
	} while (!error && count);

	if (!error) {
		nfs_attrcache(vp, &ns->ns_attr, NOFLUSH, cred);
	}
	kmem_free((caddr_t)ns, (u_int)sizeof(*ns));
	switch (error) {
	case 0:
	/*
	 * HPNFS
	 * 300 doesn't understand about quota's.  However, the other side
	 * may return this error.  Do we need to do something about this?
	 * dds 10/31/86
 	 * HPNFS
	 */
#ifdef QUOTA
	case EDQUOT:
#endif QUOTA
	case EINTR:
		break;

	case ENOSPC:
		NS_LOG_STR(LE_NFS_WRITE_ERROR,NS_LC_ERROR,NS_LS_NFS,0,
			vtomi(vp)->mi_hostname);
		break;

	default:
		{
		char spare_buffer[200];
		int *fh_int_p;
		/*
		 * Client isn't supposed to know what the fhandle looks like!
                 * BUT A CUSTOMER COMPLAINED!!!
                 * So... we are compromising by displaying the contents in
                 * hex and letting the customer decode it.  (This is a fix
                 * for a customer generated SR...)
		 */
#if   NFS_FHSIZE == 32
                fh_int_p = (int *) (wa.wa_fhandle.fh_data);
		sprintf(spare_buffer, sizeof(spare_buffer),
			"%s %d 0x%x %x %x %x %x %x %x %x",
                        vtomi(vp)->mi_hostname, error, fh_int_p[0],
                        fh_int_p[1], fh_int_p[2], fh_int_p[3], fh_int_p[4],
                        fh_int_p[5], fh_int_p[6], fh_int_p[7]);
#else
                FIX THE ABOVE sprintf statement.
                Watch out:  The ns logging code truncates the string to
                            MLEN, the length of the data area in an mbuf.
#endif

		NS_LOG_STR(LE_NFS_WRITE_ERROR_DEFAULT,NS_LC_ERROR,NS_LS_NFS,0,
			spare_buffer);
		}
		break;
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfswrite: returning %d\n", error);
#endif
	return (error);
}

/*
 * Read from a file.
 * Reads data in largest chunks our interface can handle
 */
nfsread(vp, base, offset, count, residp, cred)
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	int *residp;
	struct ucred *cred;
{
	int error;
	struct nfsreadargs ra;
	struct nfsrdresult rr;
	register int tsize;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfsread %s %o %d offset = %d, totcount = %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
	    offset, count);
#endif
	do {
		tsize = min(vtomi(vp)->mi_curread, count);
		rr.rr_data = base;
		ra.ra_fhandle = *vtofh(vp);
		ra.ra_offset = offset;
		ra.ra_totcount = tsize;
		ra.ra_count = tsize;
		error = rfscall(vtomi(vp), RFS_READ, xdr_readargs, (caddr_t)&ra,
			xdr_rdresult, (caddr_t)&rr, cred);
		if (!error) {
			error = geterrno(rr.rr_status);
			check_stale_fh(error, vp);
		}
#ifdef NFSDEBUG
		dprint(nfsdebug, 3, "nfsread: got %d of %d, error %d\n",
		    tsize, count, error);
#endif
		count -= rr.rr_count;
		base += rr.rr_count;
		offset += rr.rr_count;
	} while (!error && count && rr.rr_count == tsize);

	*residp = count;

	if (!error) {
		nfs_attrcache(vp, &rr.rr_attr, SFLUSH, cred);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfsread: returning %d, resid %d\n",
		error, *residp);
#endif
	return (error);
}

/*ARGSUSED*/
int
nfs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
/* HPNFS
 * u_error needs to get set for the error to get back to the user
 */
	u.u_error = EOPNOTSUPP;
	return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
nfs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{

/* HPNFS
 * u_error needs to get set for the error to get back to the user
 */
	u.u_error = EOPNOTSUPP;
	return (EOPNOTSUPP);
}

nfs_attrcache(vp, na, fflag, cred)
	struct vnode *vp;
	struct nfsfattr *na;
	enum staleflush fflag;
	struct ucred *cred;
{
	register struct rnode *rp;
	int delta;

	rp = vtor(vp);
	/*
	 * check the new modify time against the old modify time
	 * to see if cached data is stale
	 */
#ifdef hpux
	/*
	 * HPNFS
	 * To the user HP-UX files have a timestamp resolution
	 * of one second (via stat() and utime()).  So be consistent
	 * and check just the seconds fields.  The microseconds field
	 * of the modify time, an artifact of the BSD implmentation,
	 * is set to the "correct" value upon creation or modification
	 * but can be reset to zero by a call to utime() because
	 * HP-UX/Bell/POSIX utime() takes time_t's not struct timeval's.
	 * We've had instances of things like fbackup(1), ftio(1),
	 * file(1) etc. zero out the microseconds field and cause
	 * an unexpected "killed on text modification error" even
	 * though the contents of the file had not really changed.
	 * -byb 5/7/90
	 */
        /*
         * HPNFS
         * Well... another customer in Japan is now complaining because
         * the client code can have stale data (virtually) forever.  This
         * can happen if a server process writes a file within a second of
         * the last time the client read-in the attributes.  In such a
         * case, the seconds field of the modification time in the client's
         * rnode will have the same value as the seconds field of the
         * modification time coming across the wire; consequently, the
         * test comparing seconds would fail and the buffer cache is not
         * invalidated.
         *
         * So... for VTEXT vnodes, we examine the seconds only; for all
         * other vnodes, we examine the seconds and microseconds.
         *
         * --jcm 4/3/92
         */
        if ((vp->v_flag & VTEXT) ?
             (na->na_mtime.tv_sec != rp->r_nfsattr.na_mtime.tv_sec) :
            ((na->na_mtime.tv_sec != rp->r_nfsattr.na_mtime.tv_sec) ||
             (na->na_mtime.tv_usec != rp->r_nfsattr.na_mtime.tv_usec))) {
#else
	if (na->na_mtime.tv_sec != rp->r_nfsattr.na_mtime.tv_sec ||
	    na->na_mtime.tv_usec != rp->r_nfsattr.na_mtime.tv_usec) {
#endif
		/*
		 * The file has changed.
		 * If this was unexpected (fflag == SFLUSH),
		 * flush the delayed write blocks associated with this vnode
		 * from the buffer cache and mark the cached blocks on the
		 * free list as invalid. Also flush the page cache.
		 * If this is a text mark it invalid so that the next pagein
		 * from the file will fail.
		 * If the vnode is a directory, purge the directory name
		 * lookup cache.
		 */
		if (fflag == SFLUSH) {
			/*
			 * Don't flush the page cache for directories.  There
			 * will be no pages for directories in the cache and
			 * the flush is a linear search through the cache so
			 * it can get quite expensive.  cwb
			 */
			if (((vp->v_flag & VTEXT) == 0) && (vp->v_type != VDIR))
				mpurge(vp);
			binvalfree(vp);
		}
		if (vp->v_flag & VTEXT) {
			xinval(vp);
		}
		if (vp->v_type == VDIR) {
			dnlc_purge_vp(vp);
		}
	}
	rp->r_nfsattr = *na;
	rp->r_nfsattrtime = time;
	/*
	 * Save credentials of last process getting attributes.  The getattr()
	 * can check the credentials of the next process to see if they are
	 * the same.  If there not, the cache won't be used because ACLs on
	 * the file could cause different users to see different attributes.
	 */
	if ( rp->r_nfsattrcred != NULL )
		crfree(rp->r_nfsattrcred);
	crhold(cred);
	rp->r_nfsattrcred = cred;

        /*
         * Delta is the number of seconds that we will cache
         * attributes of the file.  It is based on the number of seconds
         * since the last change (i.e. files that changed recently
         * are likely to change soon), but there is a minimum and
         * a maximum for regular files and for directories.
         */
        delta = (time.tv_sec - rp->r_nfsattr.na_mtime.tv_sec) >> 4;
        if (vp->v_type == VDIR) {
                if (delta < vtomi(vp)->mi_acdirmin) {
                        delta = vtomi(vp)->mi_acdirmin;
                } else if (delta > vtomi(vp)->mi_acdirmax) {
                        delta = vtomi(vp)->mi_acdirmax;
                }
        } else {
                if (delta < vtomi(vp)->mi_acregmin) {
                        delta = vtomi(vp)->mi_acregmin;
                } else if (delta > vtomi(vp)->mi_acregmax) {
                        delta = vtomi(vp)->mi_acregmax;
                }
        }
        rp->r_nfsattrtime.tv_sec += delta;
}

int
nfs_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	int error;
#ifdef	MP
	sv_sema_t nfs_getattr_ss;
#endif

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_getattr %s %d %o\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif
	PXSEMA(&filesys_sema, &nfs_getattr_ss);
	(void) nfs_fsync(vp, cred);    /* sync blocks so mod time is right */
	error = nfsgetattr(vp, vap, cred);
	VXSEMA(&filesys_sema, &nfs_getattr_ss);
	return(error);
}


int
nfsgetattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	struct rnode *rp;
	int error;
	struct nfsattrstat *ns;

	rp = vtor(vp);
	/*
	 * With the addition of ACLS, it is possible to get different views
	 * of the mode of the file depending on the credentials of the
	 * process doing the asking.  Therefore, we save the credentials of
	 * the process doing the asking, and if they don't match we ignore
	 * the cache to make sure we get the right values.
	 * For NFS 4.1, we added the ability to mount the file system with
         * no attribute caching.  We still save the attributes so we can
         * tell when they have changed but we always get new ones when
         * nfs_getattr() is called.
         */
	/* cwb, Always go over the wire if RNOCACHE is set */
        if (!vtomi(vp)->mi_noac && ((rp->r_flags & RNOCACHE) == 0) &&
	    timercmp(&time, &rp->r_nfsattrtime, <) &&
				rp->r_nfsattrcred == cred) {
		/*
		 * Use cached attributes.
		 */
		nattr_to_vattr(&rp->r_nfsattr, vap);
		/* HP-UX uses 32 bit device id instead of 16 bits*/
		vap->va_fsid = 0xff000000 | vtomi(vp)->mi_mntno;

		/*
		 * This is PHKL_2853 for the 700/9.01 release and
		 *         PHKL_2874 for the 700/8.07 release and
		 *         PHKL_2885 for the 800/9.0  release.
		 *
		 * Before blindly decreasing the r_size, go over
		 * the wire to make sure this is really the case.
		 *
		 * The SUN 4.2 server code returns stale file
		 * attributes whenever a duplicate write request
		 * is made.  Consequently, we need to go over
		 * the wire here to really make sure that the
		 * size is in fact decreasing.  Otherwise, we
		 * could be incorrectly setting r_size to a
		 * smaller value.
		 */

		if (rp->r_size <= vap->va_size)
		   rp->r_size = vap->va_size;
		else if ((rp->r_flags & (RDIRTY|RFLUSHING)) == 0)
		     goto over_the_wire;
		else vap->va_size = rp->r_size;
		return (0);
	}

over_the_wire:
	ns = (struct nfsattrstat *)kmem_alloc((u_int)sizeof(*ns));
	error = rfscall(vtomi(vp), RFS_GETATTR, xdr_fhandle, (caddr_t)vtofh(vp),
	    xdr_attrstat, (caddr_t)ns, cred);
	if (!error) {
		error = geterrno(ns->ns_status);
		if (!error) {
			nattr_to_vattr(&ns->ns_attr, vap);
			/*
			 * Make sure the vnode type matches what the server
			 * says it is.  The automounter will change links
			 * to directories.
			 */
			vp->v_type = vap->va_type;

			/*
			 * this is a kludge to make programs that use dev from
			 * stat to tell file systems apart happy.  we kludge up
			 * an dev from the mount number and an arbitrary major
			 * number 255.
			*/
			/* HP-UX uses 32 bit device id instead of 16 bit */
			vap->va_fsid = 0xff000000 | vtomi(vp)->mi_mntno;
			if (rp->r_size <= vap->va_size ||
			    ((rp->r_flags & (RDIRTY|RFLUSHING)) == 0))
				rp->r_size = vap->va_size;
			else vap->va_size = rp->r_size;
			nfs_attrcache(vp, &ns->ns_attr, SFLUSH, cred);
		} else {
			check_stale_fh(error, vp);
		}
	}
	kmem_free((caddr_t)ns, (u_int)sizeof(*ns));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_getattr: returns %d\n", error);
#endif
	return (error);
}

int
nfs_validate_caches(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	struct vattr vap;
	int error;
#ifdef  MP
	sv_sema_t nfs_getattr_ss;
#endif

	PXSEMA(&filesys_sema, &nfs_getattr_ss);
	error = nfsgetattr(vp, &vap, cred);
	VXSEMA(&filesys_sema, &nfs_getattr_ss);
	return(error);
}

/*
 *The HP-UX semantics of utimes permit either a specific time to be
 *passed in, or NULL, indicating thet the modeify and access times
 *should be set to the current time.  In the former case, the process
 *is required to be the owner of the file, but in the latter case,
 *only write permission is required.  The NFS protocol cannot
 *distinguish between these two conditions, and is thus forced to
 *apply the more strict semantics.  To provide hp-ux semantics, we
 *will check for this error condition.  If it occurred (as indicated
 *by the presence of the null_time parameter and error condition
 *EPERM, we will determine if the process has write permission to the
 *file.  If so, we temporarily change the uid to be the owner of the
 *file, so as to allow the utimes to proceed.
 */
int
nfs_handle_null_time(error, vp, vap, cred)
	int error;
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	struct ucred *tcred;
	struct vattr tattr;
	
	/*If the original error was anything other than EPERM, this wasn't
	 *caused by this problem, so return the original error.
	 * Note:  if we have any problems throughout the remainder of this
	 * routine, just return the orignal error (EPERM), rather than
	 * returning the new, and potentially inapplicable one.
	 */
	if (error != EPERM)
		return (error);
	/*If null_time was set, we should not be setting anything other than
	 *the time.  Verify this fact.
	 */
	VASSERT(vap->va_mode == (u_short)-1);
	VASSERT(vap->va_uid == (u_short)(UID_NO_CHANGE));
	VASSERT(vap->va_gid == (u_short)(GID_NO_CHANGE));
	VASSERT(vap->va_size == (u_long)-1);
	/*Also verify that the times are being changed*/
	VASSERT(vap->va_atime.tv_sec != -1);
	VASSERT(vap->va_mtime.tv_sec != -1);
	/* Do we have write permission for the file? */
	if (nfs_access(vp, VWRITE, cred))
		return (EPERM);
	/* do a getattr to ascertain the owner */
	if (nfs_getattr(vp, &tattr, cred))
		return (EPERM);
	/* create a temporary ucred structure that holds the owner as the uid */
	tcred = crdup(cred);
	tcred->cr_uid = tattr.va_uid;
	error = 0;
	/*call nfs_setattr using the new cred structure.  Note that we pass
	 *null_time as zero to avoid infinite recursion
	 */
	if (nfs_setattr(vp, vap, tcred, 0))
		error = EPERM;
	/*release the tcred and return*/
	crfree(tcred);
	return (error);
}

int
nfs_setattr(vp, vap, cred, null_time)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
	int null_time;
{
	int error;
	struct nfssaargs args;
	struct nfsattrstat *ns;
	int was_a_CDF = 0;
	struct rnode *rp;
	u_long orig_size = (u_long)-1;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_setattr %s %o %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif
	rp = vtor(vp);
	if ((rp->r_nfsattr.na_mode & S_CDF) && (vp->v_type == VDIR))
		was_a_CDF = 1;
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_ctime.tv_sec != -1) || (vap->va_ctime.tv_usec != -1)) {
		error = EINVAL;
	} else {
		(void) nfs_fsync(vp, cred);
		/*
		 * Only update rsize if getting smaller.  This is because
		 * ftruncating larger is probably a noop(), and this will
		 * avoid weird side effects in the rest of the system from
		 * rsize being too large until after the rfscall returns.
		 */
		if (vap->va_size != -1 && vap->va_size < vtor(vp)->r_size) {
			/*
			 * If there's shared text associated with
			 * the rnode, try to free it up once.  If
			 * we fail, we can't allow truncation.
			 */
			if (vp->v_flag & VTEXT)
				xrele(vp);
			if (vp->v_flag & VTEXT)
				return(ETXTBSY);
			orig_size = vtor(vp)->r_size;
			vtor(vp)->r_size = vap->va_size;
		}
		ns = (struct nfsattrstat *)kmem_alloc((u_int)sizeof(*ns));
		vattr_to_sattr(vap, &args.saa_sa);
		args.saa_fh = *vtofh(vp);
		error = rfscall(vtomi(vp), RFS_SETATTR, xdr_saargs,
		    (caddr_t)&args, xdr_attrstat, (caddr_t)ns, cred);
		if (!error) {
			error = geterrno(ns->ns_status);
			if (!error) {
				nfs_attrcache(vp, &ns->ns_attr, SFLUSH, cred);
				/*
				 * HPNFS
				 * This is to fix a bug where you could
				 * truncate a file bigger then it really was.
				 * The file really didn't get bigger, but the
				 * client would see it as larger until the
				 * attribute cache timed out and a request was
				 * made to the server.
				 * In getattr, the file size is taken as the
				 * larger of r_size or v_size.  MDS  8/28/87
				 *
				 * For unknown reasons, SUN just allows
				 * the value passed down to be the value
				 * set for the length in the vnode.  This
				 * may be different from the actual length if
				 * an attempt is made to truncate beyond the
				 * end of the file.  So we will use the value
				 * returned by the NFS call to be used in the
				 * vnode.   Mike Shipley CND 09/08/87
				 */
				if (vap->va_size != -1) {
					(vtor(vp))->r_size = ns->ns_attr.na_size;
				}
				/*
				 * If changing attributes on a DIR, then
				 * we flush any names cached that were
				 * associated with the directory.  This
				 * guarantees that we truly have the right
				 * to access a directory.
				 * I added a dnlc_purge() because
				 * with CDF's, the cache was not getting purged
				 * and there were problems where the change
				 * from CDF to non-CDF was not getting seen.
				 */
				if ( (vp->v_type == VDIR) )
				     	dnlc_purge_vp(vp);
				if (was_a_CDF == 1)
					dnlc_purge();
			} else {
				check_stale_fh(error, vp);
				if (null_time)
					error = nfs_handle_null_time(error, vp,
								 vap, cred);
			}
		}
		kmem_free((caddr_t)ns, (u_int)sizeof(*ns));
	}

	/*
	 * If the file is memory mapped and we have shrunk the file,
	 * call mtrunc() with the new (smaller) size to invalidate
	 * any pages past the end of the file.
	 *
	 * We must do this work here (rather than above, before the
	 * remote call) so that any error recovery has already been
	 * performed.
	 */
	if (orig_size != (u_long)-1 && (vp->v_flag & VMMF) &&
	    vtor(vp)->r_size < orig_size) {
		mtrunc(vp, vtor(vp)->r_size);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_setattr: returning %d\n", error);
#endif
	return (error);
}

#define VNANY_EXEC	(VEXEC | VEXEC >> 3 | VEXEC >> 6)
int
nfs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	struct vattr va;
	int *gp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_access %s %o %d mode %d uid %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
	    mode, cred->cr_uid);
#endif
	u.u_error = nfs_getattr(vp, &va, cred);
	if (u.u_error) {
		return (u.u_error);
	}
	/*
	 * If you're the super-user,
	 * you always get read/write access,
	 * and exec access if any exec bit is
	 * set, or if it's a directory.
	 */
	if ((cred->cr_uid == 0) &&
	    (!(mode & VEXEC) ||
	     (va.va_type == VDIR) || (va.va_mode & VNANY_EXEC)))
	{
		return (0);
	}
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (cred->cr_uid != va.va_uid) {
		mode >>= 3;
		if (cred->cr_gid == va.va_gid)
			goto found;
		gp = (int *) (cred->cr_groups);
		for (; gp < (int *) (&cred->cr_groups[NGROUPS]) && *gp != NOGROUP; gp++)
			if (va.va_gid == *gp)
				goto found;
		mode >>= 3;
	}
found:
	if ((va.va_mode & mode) == mode) {
		return (0);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_access: returning %d\n", u.u_error);
#endif
	u.u_error = EACCES;
	return (EACCES);
}

int
nfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	int error;
	struct nfsrdlnres rl;
        extern int hpux_aes_override;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_readlink %s %o %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif
	if(vp->v_type != VLNK)
		return (ENXIO);
	rl.rl_data = (char *)kmem_alloc((u_int)NFS_MAXPATHLEN);
	error =
	    rfscall(vtomi(vp), RFS_READLINK, xdr_fhandle, (caddr_t)vtofh(vp),
	      xdr_rdlnres, (caddr_t)&rl, cred);
	if (!error) {
		error = geterrno(rl.rl_status);
		if (!error) {
                        /* if symbolic link is longer than the user's
                           buffer, AES states we should fail with ERANGE */
                        if( ((int)rl.rl_count > uiop->uio_resid)
                            && (!hpux_aes_override)){
                           error = ERANGE;
                        }
                        else {
                           error = uiomove(rl.rl_data, (int)rl.rl_count,
                               UIO_READ, uiop);
                        }
		} else {
			check_stale_fh(error, vp);
		}
	}
	kmem_free((caddr_t)rl.rl_data, (u_int)NFS_MAXPATHLEN);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_readlink: returning %d\n", error);
#endif
	return (error);
}

/*ARGSUSED*/
int
nfs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct rnode *rp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_fsync %s %o %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif
	rp = vtor(vp);
        /* HPNFS
         * This rlock was added to protect the rnode while it is getting
         * flushed out.  This is done to fix a defect where a file got
         * corrupted data in it when it was being written into with
         * O_APPEND and another process was doing a tail -f of the file.
         *     Mike Shipley  CND
         */
        rlock(rp);

	if (rp->r_flags & RDIRTY) {
		/* Clear the dirty bit but set a new bit so that we know
		 * we're flushing.  See comment by patch string. cwb
		 */
		rp->r_flags &= ~RDIRTY;
		rp->r_flags |= RFLUSHING;
		bflush(vp);	/* start delayed writes */
		/* Wait until they are all done.
		 * Note that dux_flush_cache would do the writes, but
		 * if a dirty buffer was at the end, it would wait for the
		 * others to have finished writing before doing a synchronous
		 * write of the dirty buffer.  Faster to start them all
		 * asynchronous.
		 */

		dux_flush_cache(vp);
		rp->r_flags &= ~RFLUSHING;
	}
	runlock(rp);
	return (rp->r_error);
}

/*
 * Weirdness: if the file was removed while it was open it got
 * renamed (by nfs_remove) instead.  Here we remove the renamed
 * file.
 */
/*ARGSUSED*/
int
nfs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	int error;
	struct nfsdiropargs da;
	enum nfsstat status;
	register struct rnode *rp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_inactive %s %o %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno);
#endif
	rp = vtor(vp);
	/*
	 * Pull rnode off of the hash list so it won't be found.
	 */
	runsave(rp);

	if (rp->r_unlname != NULL) {
		setdiropargs(&da, rp->r_unlname, rp->r_unldvp);
		error = rfscall( vtomi(rp->r_unldvp), RFS_REMOVE,
		    xdr_diropargs, (caddr_t)&da,
		    xdr_enum, (caddr_t)&status, rp->r_unlcred);
		if (!error) {
			error = geterrno(status);
		}
		VN_RELE(rp->r_unldvp);
		rp->r_unldvp = NULL;
		kmem_free((caddr_t)rp->r_unlname, (u_int)NFS_MAXNAMLEN);
		rp->r_unlname = NULL;
		crfree(rp->r_unlcred);
		rp->r_unlcred = NULL;
	}
	((struct mntinfo *)vp->v_vfsp->vfs_data)->mi_refct--;

        /*
         * PHKL_2836
         *
         * Split rnode entry r_cred into r_rcred and r_wcred to
         * avoid EACCES errors in the delayed buffer write case.
         *
         */

	if (rp->r_rcred) {
		crfree(rp->r_rcred);
		rp->r_rcred = NULL;
	}
	if (rp->r_wcred) {
		crfree(rp->r_wcred);
		rp->r_wcred = NULL;
	}
	if (rp->r_nfsattrcred) {
		crfree(rp->r_nfsattrcred);
		rp->r_nfsattrcred = NULL;
	}
	rfree(rp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_inactive done\n");
#endif
	return (0);
}

/*
 * Remote file system operations having to do with directory manipulation.
 */

nfs_lookup(dvp, nm, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfsdiropargs da;
	struct  nfsdiropres *dr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_lookup %s %o %d '%s'\n",
	    vtomi(dvp)->mi_hostname,
	    vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno,
	    nm);
#endif
	if (!vtomi(dvp)->mi_noac) {

		/*
		 * Before checking dnlc, call nfs_validate_caches to be
		 * sure directory hasn't changed.  nfs_validate_caches
		 * will purge dnlc if a change has occurred.
		 */
		if (error = nfs_validate_caches(dvp, cred)) {
			return (error);
		}
	}
	/*
	 * Because we have to handle device files/fifos as special case
 	 * no matter what, don't return immediately and lock rnode.
	 */
	rlock(vtor(dvp));
	if (vtomi(dvp)->mi_noac) {
		*vpp = NULL;
	}
	else {
		/* Only look in the name cache if -noac was not specified */
		*vpp = (struct vnode *) dnlc_lookup(dvp, nm, cred);
	}
	if (*vpp) {
		VN_HOLD(*vpp);
	}
	else {
	dr = (struct  nfsdiropres *)kmem_alloc((u_int)sizeof(*dr));
	setdiropargs(&da, nm, dvp);
	error = rfscall(vtomi(dvp), RFS_LOOKUP, xdr_diropargs, (caddr_t)&da,
	    xdr_diropres, (caddr_t)dr, cred);
	if (!error) {
		error = geterrno(dr->dr_status);
		check_stale_fh(error, dvp);
	}
	if (!error) {
		*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr, dvp->v_vfsp);
		if (!vtomi(*vpp)->mi_noac) {
			/*
			 * Only add the vnode to the name cache if -noac
			 * was not specified
			 */
			dnlc_enter(dvp, nm, *vpp, cred);
		}
	} else {
		*vpp = (struct vnode *)0;
	}
	kmem_free((caddr_t)dr, (u_int)sizeof(*dr));
        }
        /*
         * If vnode is a device create special vnode.  Note that we
	 * have the capability to turn off device access on the mount, but
	 * we still allow FIFOS because there is no security hole associated
	 * with them.
         */
        if (!error && ISVDEV((*vpp)->v_type) && ((*vpp)->v_type == VFIFO ||
	    (*vpp)->v_type == VSOCK || vtomi(*vpp)->mi_devs) ) {
                struct vnode *newvp;

                newvp = specvp(*vpp, (*vpp)->v_rdev);
                VN_RELE(*vpp);
                *vpp = newvp;
        }
        runlock(vtor(dvp));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_lookup returning %d vp = %x\n", error, *vpp);
#endif
	return (error);
}

/*ARGSUSED*/
nfs_create(dvp, nm, va, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct  nfsdiropres *dr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_create %s %o %d '%s' excl=%d, mode=%o\n",
	    vtomi(dvp)->mi_hostname,
	    vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno,
	    nm, exclusive, mode);
#endif
	if (exclusive == EXCL) {
		/*
		 * This is buggy: there is a race between the lookup and the
		 * create.  We should send the exclusive flag over the wire.
		 */
		error = nfs_lookup(dvp, nm, vpp, cred);
		if (!error) {
			VN_RELE(*vpp);
			return (EEXIST);
		}
	}
	*vpp = (struct vnode *)0;

        /*
         * This is a completely gross hack to make mknod
         * work over the wire until we can wack the protocol
	 * NOTE: disallow those not specifically supported by Sun.
         */
        if (va->va_type == VCHR) {
                va->va_mode |= S_IFCHR;
                va->va_size = (u_long)va->va_rdev;
        } else if (va->va_type == VBLK) {
                va->va_mode |= S_IFBLK;
                va->va_size = (u_long)va->va_rdev;
        } else if (va->va_type == VFIFO) {
                va->va_mode |= S_IFCHR;         /* xtra kludge for namedpipe */
                va->va_size = (u_long)NFS_FIFO_DEV;     /* blech */
        } else if (va->va_type == VSOCK) {
                va->va_mode |= S_IFSOCK;
        }
#ifdef hpux
	else if (va->va_type != VREG ) {
		return(EINVAL);
	}
#endif hpux


	dr = (struct  nfsdiropres *)kmem_alloc((u_int)sizeof(*dr));
	setdiropargs(&args.ca_da, nm, dvp);
	vattr_to_sattr(va, &args.ca_sa);
	error = rfscall(vtomi(dvp), RFS_CREATE, xdr_creatargs, (caddr_t)&args,
	    xdr_diropres, (caddr_t)dr, cred);
	nfsattr_inval(dvp);   /* mod time changed */
	if (!error) {
		error = geterrno(dr->dr_status);
		if (!error) {
			*vpp = makenfsnode(
			    &dr->dr_fhandle, &dr->dr_attr, dvp->v_vfsp);

  			/*
  			 * A fix from NFS3.2 to fix a defect where two writers
  			 * could affect each other if they open a file to write
  			 * to by using the O_CREAT flag.  The file could be
  			 * truncated when opened the second time resulting in
  			 * lost data.   If the file is being created with
			 * truncate, va_size will be zero.  mds   11/3/88
  			 */
  			if (va->va_size == 0) {
  				(vtor(*vpp))->r_size = 0;
  			}

			nattr_to_vattr(&dr->dr_attr, va);
                        /*
                         * If vnode is a device create special vnode
                         */
                        if (ISVDEV((*vpp)->v_type) && ((*vpp)->v_type==VFIFO ||
			    (*vpp)->v_type == VSOCK || vtomi(*vpp)->mi_devs) ) {
                                struct vnode *newvp;

                                newvp = specvp(*vpp, (*vpp)->v_rdev);
                                VN_RELE(*vpp);
                                *vpp = newvp;
                        }
		} else {
			check_stale_fh(error, dvp);
		}
	}
	kmem_free((caddr_t)dr, (u_int)sizeof(*dr));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_create returning %d\n", error);
#endif
	return (error);
}

/*
 * Weirdness: if the vnode to be removed is open
 * we rename it instead of removing it and nfs_inactive
 * will remove the new name.
 */
nfs_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	int error;
	struct nfsdiropargs da;
	enum nfsstat status;
	struct vnode *vp;
	char *tmpname;
	struct vnode *oldvp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_remove %s %o %d '%s'\n",
	    vtomi(dvp)->mi_hostname,
	    vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno,
	    nm);
#endif
	status = NFS_OK;
	error = nfs_lookup(dvp, nm, &vp, cred);
        /*
         * Lookup may have returned a non-nfs vnode!
         * get the real vnode.  Then, instead of checking if it
	 * is open, check the reference count on the vnode.
	 * NOTE: Sun had a vnode function called VOP_REALVP() which they
	 * added to the 4.0 code.  Since we don't want to mess with the
	 * NFS 4.0 code yet, do the test we know will work.  This relies
	 * on the vnode pointing to a special or fifo structure if not NFS!
         */
        /*if (error == 0 && VOP_REALVP(vp, &realvp) == 0) { */
        if (error == 0 && vp->v_fstype != VNFS ) {
                oldvp = vp;
                vp = VTOS(vp)->s_realvp;
        } else {
                oldvp = NULL;
        }
        if (error == 0 && vp != NULL) {
                rlock(vtor(dvp));
                /*
                 * We need to flush the name cache so we can
                 * check the real reference count on the vnode
                 */
                dnlc_purge_vp(vp);
		/*
		 * The file system may have references to the vnode in
		 * the free block list.  Get rid of them to get the
		 * right reference count on the vnode.  NOTE:  This call is
		 * in Sun 3.2, but not 4.0, but seems required.  Hmm..???
		 */
		if (vp->v_count > 1)
		{
			binvalfree(vp);

			if ((vp->v_count > 1) && ((vp->v_flag & VTEXT) == 0))
			mpurge(vp);
		}
		/*
		 * Sun's test was wrong here.  They only checked the count
		 * on the real vp.  But with fifos and device files, the count
		 * on the real vp will be one (from the snode), but the
		 * fifonode or snode may have a larger count.  In that
		 * case we want to rename and clean up later. dds.
		 */
                /*if ((vp->v_count > 1) && vtor(vp)->r_unldvp == NULL) { */
                if (((vp->v_count > 1) || (oldvp && (oldvp->v_count > 1)))
		    && vtor(vp)->r_unldvp == NULL) {
			runlock(vtor(dvp));
			tmpname = newname();
			error = nfs_rename(dvp, nm, dvp, tmpname, cred);
			rlock(vtor(dvp));
			if (error) {
				kmem_free((caddr_t)tmpname,
				    (u_int)NFS_MAXNAMLEN);
			} else {
				VN_HOLD(dvp);
				vtor(vp)->r_unldvp = dvp;
				vtor(vp)->r_unlname = tmpname;
				if (vtor(vp)->r_unlcred != NULL) {
					crfree(vtor(vp)->r_unlcred);
				}
				crhold(cred);
				vtor(vp)->r_unlcred = cred;
			}
		} else {
			setdiropargs(&da, nm, dvp);
			error = rfscall(
			    vtomi(dvp), RFS_REMOVE, xdr_diropargs, (caddr_t)&da,
			    xdr_enum, (caddr_t)&status, cred);
			nfsattr_inval(dvp);   /* mod time changed */
			nfsattr_inval(vp);    /* link count changed */
		 	check_stale_fh(error ? error:geterrno(status), dvp);
		}
                runlock(vtor(dvp));
                if (oldvp) {
                        bflush(oldvp);
                        VN_RELE(oldvp);
                } else {
                        bflush(vp);
                        VN_RELE(vp);
                }
	}
	if (!error) {
		error = geterrno(status);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_remove: returning %d\n", error);
#endif
	return (error);
}

nfs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	int error;
	struct nfslinkargs args;
	enum nfsstat status;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_link from %s %o %d to %s %o %d '%s'\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno,
	    vtomi(tdvp)->mi_hostname,
	    vtofh(tdvp)->fh_fsid, vtofh(tdvp)->fh_fno,
	    tnm);
#endif

	/*
	 * vnode may point to an snode or fifonode.  In that case,
	 * get the real vnode to work on.  Note that in Sun code they
	 * have a new VOP routine in 4.0, instead we use a special
	 * test since we know only NFS will be involved.
	 */
	if ( vp->v_fstype != VNFS ) {
		vp = VTOS(vp)->s_realvp;
	}
	args.la_from = *vtofh(vp);
	setdiropargs(&args.la_to, tnm, tdvp);
	error = rfscall(vtomi(vp), RFS_LINK, xdr_linkargs, (caddr_t)&args,
	    xdr_enum, (caddr_t)&status, cred);
	nfsattr_inval(tdvp);   /* mod time changed */
	nfsattr_inval(vp);     /* link count changed */
	if (!error) {
		error = geterrno(status);
		check_stale_fh(error, vp);
		check_stale_fh(error, tdvp);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_link returning %d\n", error);
#endif
	return (error);
}

nfs_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsrnmargs args;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rename from %s %o %d '%s' to %s %o %d '%s'\n",
	    vtomi(odvp)->mi_hostname,
	    vtofh(odvp)->fh_fsid, vtofh(odvp)->fh_fno, onm,
	    vtomi(ndvp)->mi_hostname,
	    vtofh(ndvp)->fh_fsid, vtofh(ndvp)->fh_fno, nnm);
#endif

	if (!strcmp(onm, ".") || !strcmp(onm, "..") || !strcmp(nnm, ".")
	    || !strcmp (nnm, "..")) {
		error = EINVAL;
	} else {
		dnlc_purge();
		setdiropargs(&args.rna_from, onm, odvp);
		setdiropargs(&args.rna_to, nnm, ndvp);
		error = rfscall(vtomi(odvp), RFS_RENAME,
		    xdr_rnmargs, (caddr_t)&args,
		    xdr_enum, (caddr_t)&status, cred);
		nfsattr_inval(odvp);   /* mod time changed */
		nfsattr_inval(ndvp);   /* mod time changed */
		if (!error) {
			error = geterrno(status);
			check_stale_fh(error, odvp);
			check_stale_fh(error, ndvp);
		}
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rename returning %d\n", error);
#endif
	return (error);
}

nfs_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct  nfsdiropres *dr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_mkdir %s %o %d '%s'\n",
	    vtomi(dvp)->mi_hostname,
	    vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, nm);
#endif
/*
 ***********************************************************************
 * See related changes in files h/vnode.h sys/vfs_scalls.c and ufs_dir.c
 ***********************************************************************
 * To maintain SysV compatability, the local file system is allowing the
 * creation of a directory, using mknod, without "." and "..".  In this
 * case NFS will just return EINVAL.  mds  05/27/87
 */

	if (va->va_type == VEMPTYDIR)
		return(EINVAL);

	dr = (struct  nfsdiropres *)kmem_alloc((u_int)sizeof(*dr));
	setdiropargs(&args.ca_da, nm, dvp);
	vattr_to_sattr(va, &args.ca_sa);
	error = rfscall(vtomi(dvp), RFS_MKDIR, xdr_creatargs, (caddr_t)&args,
	    xdr_diropres, (caddr_t)dr, cred);
	nfsattr_inval(dvp);   /* mod time changed */
	if (!error) {
		error = geterrno(dr->dr_status);
		check_stale_fh(error, dvp);
	}
	if (!error) {
		*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr, dvp->v_vfsp);
	} else {
		*vpp = (struct vnode *)0;
	}
	kmem_free((caddr_t)dr, (u_int)sizeof(*dr));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_mkdir returning %d\n", error);
#endif
	return (error);
}

nfs_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsdiropargs da;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rmdir %s %o %d '%s'\n",
	    vtomi(dvp)->mi_hostname,
	    vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, nm);
#endif
	setdiropargs(&da, nm, dvp);
	dnlc_purge();
	error = rfscall(vtomi(dvp), RFS_RMDIR, xdr_diropargs, (caddr_t)&da,
	    xdr_enum, (caddr_t)&status, cred);
	nfsattr_inval(dvp);   /* mod time changed */
	if (!error) {
		error = geterrno(status);
		check_stale_fh(error, dvp);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rmdir returning %d\n", error);
#endif
	return (error);
}

nfs_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{
	int error;
	struct nfsslargs args;
	enum nfsstat status;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_symlink %s %o %d '%s' to '%s'\n",
	    vtomi(dvp)->mi_hostname,
	    vtofh(dvp)->fh_fsid, vtofh(dvp)->fh_fno, lnm, tnm);
#endif
	setdiropargs(&args.sla_from, lnm, dvp);
	vattr_to_sattr(tva, &args.sla_sa);
	args.sla_tnm = tnm;
	error = rfscall(vtomi(dvp), RFS_SYMLINK, xdr_slargs, (caddr_t)&args,
	    xdr_enum, (caddr_t)&status, cred);
	nfsattr_inval(dvp);   /* mod time changed */
	if (!error) {
		error = geterrno(status);
		check_stale_fh(error, dvp);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_sysmlink: returning %d\n", error);
#endif
	return (error);
}

/*
 * Read directory entries.
 * There are some weird things to look out for here.  The uio_offset
 * field is either 0 or it is the offset returned from a previous
 * readdir.  It is an opaque value used by the server to find the
 * correct directory block to read.  The byte count must be at least
 * vtoblksz(vp) bytes.  The count field is the number of blocks to
 * read on the server.  This is advisory only, the server may return
 * only one block's worth of entries.  Entries may be compressed on
 * the server.
 */
nfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	int error = 0;
	struct iovec *iovp;
	int count;
	struct nfsrddirargs rda;
	struct nfsrddirres  rd;
	struct rnode *rp;

	rp = vtor(vp);
	if ((rp->r_flags & REOF) && (rp->r_size == (u_long)uiop->uio_offset)) {
		return (0);
	}
	iovp = uiop->uio_iov;
	if ((count = iovp->iov_len) < 0)
		return(EINVAL);
#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
	    "nfs_readdir %s %o %d count %d offset %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno, count, uiop->uio_offset);
#endif
	/*
	 * XXX We should do some kind of test for count >= DEV_BSIZE
	 */
	if (uiop->uio_iovcnt != 1) {
		return (EINVAL);
	}
	count = MIN(count, vtomi(vp)->mi_curread);
	rda.rda_count = count;
	rda.rda_offset = uiop->uio_offset;
	rda.rda_fh = *vtofh(vp);
	rd.rd_size = count;
	rd.rd_entries = (struct direct *)kmem_alloc((u_int)count);


	error = rfscall(vtomi(vp), RFS_READDIR, xdr_rddirargs, (caddr_t)&rda,
	    xdr_getrddirres, (caddr_t)&rd, cred);
	if (!error) {
		error = geterrno(rd.rd_status);
		check_stale_fh(error, vp);
	}
	if (!error) {
		/*
		 * move dir entries to user land
		 */
		if (rd.rd_size) {
			error = uiomove((caddr_t)rd.rd_entries,
			    (int)rd.rd_size, UIO_READ, uiop);
			rda.rda_offset = rd.rd_offset;
			uiop->uio_offset = rd.rd_offset;
		}
		if (rd.rd_eof) {
			rp->r_flags |= REOF;
			rp->r_size = uiop->uio_offset;
		}
	}
	kmem_free((caddr_t)rd.rd_entries, (u_int)count);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_readdir: returning %d resid %d, offset %d\n",
	    error, uiop->uio_resid, uiop->uio_offset);
#endif
	return (error);
}

/*
 * Convert from file system blocks to device blocks
 */
int
nfs_bmap(vp, bn, vpp, bnp)
	struct vnode *vp;	/* file's vnode */
	daddr_t bn;		/* fs block number */
	struct vnode **vpp;	/* RETURN vp of device */
	daddr_t *bnp;		/* RETURN device block number */
{
	int bsize;		/* server's block size in bytes */

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_bmap %s %o %d blk %d\n",
	    vtomi(vp)->mi_hostname,
	    vtofh(vp)->fh_fsid, vtofh(vp)->fh_fno, bn);
#endif
	if (vpp)
		*vpp = vp;
	if (bnp) {
		bsize = vtoblksz(vp);
		*bnp = (bn * bsize) / DEV_BSIZE;
	}
	return (0);
}

/*
 * the next few structures and definitions are for the async daemon, which
 * will perform asynchronous i/o on behalf of a client NFS process.
 * The original code has a problem with interrupts and the async_daemon.
 * The scenario is the following:
 *	-- the async_daemon is blocked waiting for a read-ahead block
 *	-- the client process is blocked waiting for the async_daemon
 *	   to complete its read of that block (sleeping in getblk()).
 *	-- the server system is down or the NFS server processes are dead,
 *	   and the read will not return.
 *	-- interrupts are not caught by the async_daemon because it's
 *	   unrelated to the client process, and the client process is
 *	   sleeping at an uninterruptable priority (PRIBIO+1).
 *	-- The result is that the process is hung and cannot be killed,
 *	   except by killing the async_daemon process.
 *
 * The FIX:
 *	-- set the process group of the async_daemon process to that of
 *	   the client process it is working on behalf of, and reset it
 *	   when the i/o is complete.
 *
 *	   This allows the async_daemon to be interrupted in the RPC code.
 *
 *	   The async_daemon will release the buffer, which will wake up
 *	   the client process, which will then see its interrupt, and exit.
 *
 *	-- This will not work if the process is being killed with the kill()
 *	   command, since that signal will not be seen by all other processes
 *	   in the process group.  If a background process is hung, it will
 *	   be necessary to send a specific signal to any async_daemon
 *	   processes that are working for it; this will result in them
 *	   awakening, which will awaken the client process
 *
 *	-- gmf
 */

struct nfs_async {
	struct nfs_async *next;
	struct buf       *buffer;
	short		 pgrp;
	int              p_sigmask;
	int              p_sigignore;
	int              p_sigcatch;
};
	

/*
 * we have had systems deadlock in the following scenario:
 * machine A does a cp of a large file over an NFS mount point to B
 * machine B does a cp of a large file over an NFS mount point to A
 *
 * because we allow the queue to grow without bound, we get into a deadlock
 * where there are no buffers available on either machine and therefore
 * neither of the writes will complete.  As a fix, we therefore limit the
 * total number of outstanding asynchronous writes to allocate no more
 * than half of the total number of buffers available on the system
 * system - ghs 10/12/87.
 * Somebody changed this to be only a 1/4 of the buffer cache at some time.
 * For 9.0 we have a dynamic buffer cache so we limit the total number of
 * outstanding asynchronous writes to 1/8 of the current number of pages in
 * the buffer cache.  nbuf does not float with the buffer cache so we had
 * to use bufpages.  NFS buffers are usually 2 pages per write so the
 * 1/8 number of pages translates into 1/4 the number of buffers, approximately.
 * Since buffers can be greater > 2 pages this isn't perfect, but too few
 * can also cause hangs when enqueue_seq_blocks() goes to sleep when it
 * expects the write to be asynchronous.  cwb 8/20/92 FSDdt10381
 */
extern int nbuf;

#if defined(_WSIO)
#define MAX_QUEUE	(bufpages/8)
#else
#define MAX_QUEUE nbuf / 4
#endif hp9000s200

#define NFS_GETBUF(ptr) \
			if (ptr = nfs_bufhead) { \
				nfs_bufhead = nfs_bufhead->next; \
			} else { \
				if (buffers_allocated < MAX_QUEUE) { \
				     buffers_allocated++; \
				     ptr = (struct nfs_async *) \
			             kmem_alloc( (u_int) sizeof(struct nfs_async));\
				     } \
				else ptr = NULL; \
		 	}

#define NFS_PUTBUF(ptr) { \
			ptr->next = nfs_bufhead; \
			nfs_bufhead = ptr; \
			}

#define biod_enqueue(nfsp) \
     if (async_buftail) { \
	     nfsptmp = async_buftail; \
	     async_buftail = nfsp; \
	     nfsptmp->next = async_buftail; \
	     nfsp->next = NULL; \
	     } \
     else { \
	     async_bufhead = nfsp; \
	     async_buftail = nfsp; \
	     nfsp->next = NULL; \
	 } \

#define biod_dequeue(nfsp) \
	     nfsp = async_bufhead; \
	     async_bufhead = nfsp->next; \
	     if (async_bufhead == NULL) \
		     async_buftail = NULL;



struct nfs_async *nfs_bufhead;
struct nfs_async *async_bufhead, *async_buftail;
struct nfs_async *nfsptmp;
int async_daemon_count, idle_biod;
				
int buffers_allocated;
int requests_queued;

/*
 * Buffer's data is in userland, or in some other
 *  currently inaccessable place.  We map in a page at a time
 *  and read it in.
 */
void
nfs_strat_map(bp)
    register struct buf *bp;
{
    register daddr_t blkno = bp->b_blkno;
    register caddr_t base_vaddr;
    register long count;
    register int pfn, nbytes, npage = 0, off;
    caddr_t vaddr;
    caddr_t old_vaddr;
    space_t old_space;
    long    old_count;
    extern caddr_t hdl_kmap();
    sv_sema_t nfsSS;

    /* Record old addr from bp, set to KERNELSPACE */
    old_vaddr = bp->b_un.b_addr;
    old_space = bp->b_spaddr;
    old_count = bp->b_bcount;
    bp->b_spaddr = KERNELSPACE;
    base_vaddr = (caddr_t)((unsigned long)old_vaddr & ~(NBPG-1));

    VASSERT((bp->b_flags & B_ORDWRI) == 0);
    VASSERT((bp->b_flags2 & B2_ORDMASK) == 0);

    /*
     * See how much to move, set count to one page max.  If we're
     *  reading partway into a page, go to the extra trouble of
     *  figuring out the correct initial count.
     */
    off = (int)old_vaddr & (NBPG-1);
    nbytes = bp->b_bcount;
    if (off) {
	count = NBPG-off;
	if (count > nbytes)
	    count = nbytes;
    } else {
	if (bp->b_bcount > NBPG)
	    count = NBPG;
	else
	    count = bp->b_bcount;
    }

    /* Move in data in page-size chunks */
    while (nbytes > 0) {

	/* Map the page into kernel memory, point bp to it */
	pfn = hdl_vpfn(old_space, base_vaddr+ptob(npage));
	vaddr = hdl_kmap(pfn);
	bp->b_un.b_addr = vaddr+off;

	/* Set up block field */
	bp->b_blkno = blkno;
	bp->b_bcount = count;
	bp->b_flags &= ~B_DONE;

	PXSEMA(&filesys_sema, &nfsSS);
	/*
	 * Do the I/O and then remap the page back to user space. 
	 */
	do_bio(bp);
	hdl_remap(old_space, base_vaddr+ptob(npage), pfn);
	hdl_kunmap(vaddr);
	VXSEMA(&filesys_sema, &nfsSS);

	/* Call it quits on error */
	if (geterror(bp) || bp->b_resid) break;

	/* Update counts and offsets*/
	nbytes -= count;
	blkno += btodb(count);
	++npage;
	off = 0;
    }

    /* Restore bp fields and done */
    bp->b_un.b_addr = old_vaddr;
    bp->b_spaddr = old_space;
    bp->b_bcount = old_count;

    iodone(bp);
}

int
nfs_strategy(bp)
	register struct buf *bp;
{
	register struct nfs_async *nfsp;
	sv_sema_t nfsSS;
	struct rnode *rp = vtor(bp->b_vp);


#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_strategy bp %x\n", bp);
#endif
	
	/*
	 * If there was an asynchronous write error on this rnode
	 * then we just return the old error code. This continues
	 * until the rnode goes away (zero ref count). We do this because
	 * there can be many procs writing this rnode.
	 */
	/* We ran into a hot site where a customer's applications was writing
	 * a log file that filled the disk, but the application continued and
	 * did not close the file.  When cat or more was used to try and look
	 * at the file, nfs_strategy() was setting the read error to r_error.
	 * cat and more were not checking that the read returned -1, they just
	 * noticed no bytes were returned, thus they acted like the file was
	 * empty.  Very confusing to the customer and thus a hot site.  The
	 * fix is to take this check out.  This allows the tests for r_error
	 * in rwvp() and do_bio() to only return r_error on writes not reads.
	 */

	/* if (rp->r_error) { */
	/* 	bp->b_error = rp->r_error; */
	/* 	bp->b_flags |= B_ERROR; */
	/* 	iodone(bp); */
	/* 	return; */
	/* } */

#ifdef	FSD_KI
	/* stamp bp with start time */
	bp->b_queuelen = 0;
	KI_getprectime(&bp->b_timeval_eq);
#endif	FSD_KI

        /*
         * PHKL_2836
         *
         * Split rnode entry r_cred into r_rcred and r_wcred to
         * avoid EACCES errors in the delayed buffer write case.
         *
         */

	if (rp->r_rcred == NULL) {
		crhold(u.u_cred);
		rp->r_rcred = u.u_cred;
	}
	if (rp->r_wcred == NULL) {
		crhold(u.u_cred);
		rp->r_wcred = u.u_cred;
	}
	/* Buffer isn't in kernel virtual address space, so handle specially */
	if (bvtospace(bp,bp->b_un.b_addr) != KERNELSPACE) {
		nfs_strat_map(bp);
		return;
	}
	PXSEMA(&filesys_sema, &nfsSS);
	if (async_daemon_count && bp->b_flags & B_ASYNC) {
		/*
		 * get a free buffer and initialize the pgrp field
		 * for use by an async_daemon process.
		 * Added for interruptability -- gmf.
		 */
		NFS_GETBUF(nfsp);
		if (nfsp != NULL) {
			nfsp->pgrp = u.u_procp->p_pgrp;
			nfsp->p_sigignore = u.u_procp->p_sigignore;
			nfsp->p_sigcatch = u.u_procp->p_sigcatch;
			nfsp->p_sigmask = u.u_procp->p_sigmask;
			nfsp->buffer = bp;
			biod_enqueue(nfsp);
			requests_queued++;
#ifdef	FSD_KI
			bp->b_queuelen = requests_queued;
#endif	FSD_KI
			/*
			 * Don't do the wakeup if all of the biod's are
			 * already working.
			 */
			if (idle_biod)
				wakeup((caddr_t) &async_bufhead);
	    	}
		else {
			/*
			 * couldn't get bufhead from available list, so do it now
			 */
			do_bio(bp);
			iodone(bp);
		}
	}
	else {
		/*
		 * no async_daemon running, do the i/o now.
		 */
		do_bio(bp);
		iodone(bp);
	}
	VXSEMA(&filesys_sema, &nfsSS);
}

async_daemon()
{
	register struct nfs_async *nfsp;
	short saved_pgrp;
	int saved_sigmask, saved_sigignore, saved_sigcatch;

	/*
	 * First release resources.
	 */
	dispreg(u.u_procp, 0, PROCATTACHED);

	/*
	 * Allocate a NFS async i/o buffer and put it on the free list.
	 * If this allocation fails, the async_daemon will not work, and
	 * no asynchronous i/o will be done.
	 * Added for interruptability -- gmf.
	 */
	
	nfsp = (struct nfs_async *)
		kmem_alloc( (u_int) sizeof(struct nfs_async));
	PSEMA(&filesys_sema);
	if (nfsp) {
		buffers_allocated++;
		NFS_PUTBUF(nfsp);
	}

	if (setjmp(&u.u_qsave)) {
		PSEMA(&filesys_sema);
		async_daemon_count--;
		/*
		 * Fix the idle_biod count which didn't get decremented
		 * because we long-jumped here out of the sleep below.
		 */
		idle_biod--;
		/*
		 * Free an async buffer header
	 	 * Added for interruptability -- gmf.
		 */
		NFS_GETBUF(nfsp);
		if (nfsp) {
		    kmem_free((caddr_t)nfsp,(u_int)sizeof(struct nfs_async));
		    buffers_allocated--;
		}
		VSEMA(&filesys_sema);
		exit(0);
	}

	/*
	 * HPNFS.  Save our entry signal status information to restore
	 * after each service request.
	 */
	saved_pgrp = u.u_procp->p_pgrp;
	saved_sigignore = u.u_procp->p_sigignore;
	saved_sigcatch = u.u_procp->p_sigcatch;
	saved_sigmask = u.u_procp->p_sigmask;

/* FFM
 * Attempt to add queuing to NFS I/O
 */
	async_daemon_count++;
	for (;;) {
		idle_biod++;
		while (async_bufhead == NULL) {
			sleep((caddr_t)&async_bufhead, PZERO + 1);
		}
		idle_biod--;
		/* FFM async_daemon_count--; */
		biod_dequeue(nfsp);
		requests_queued--;
		/*
		 * save process group, and change mine to match the
		 * process I'm working on behalf of in order to catch
		 * signals at the RPC level.
	 	 * Added for interruptability -- gmf.
		 */
		/*u.u_procp->p_pgrp = nfsp->pgrp;*/
		if ( !gchange(u.u_procp, nfsp->pgrp) )
			panic("biod: could not change process group");
		u.u_procp->p_sigmask = nfsp->p_sigmask;
		u.u_procp->p_sigignore = nfsp->p_sigignore;
		u.u_procp->p_sigcatch = nfsp->p_sigcatch;
		/*
		 * do the i/o
		 */
		do_bio(nfsp->buffer);
		iodone(nfsp->buffer);
		/*
		 * restore original process group and release the header
		 * to the free list.
		 */
		/*u.u_procp->p_pgrp = saved_pgrp; */
		if ( !gchange( u.u_procp, saved_pgrp) )
			panic("biod: could not change process group");
		u.u_procp->p_sigmask = saved_sigmask;
		u.u_procp->p_sigignore = saved_sigignore;
		u.u_procp->p_sigcatch = saved_sigcatch;
		/*
		 * A signal may have been generated as pending while in the
		 * rpc call.  However, we no longer want to honor those signals
		 * which could have been generated by the user intending only
		 * to kill the foreground process and not the biod.  Therefore
		 * we clear the list of pending signals before going to sleep.
		 * This may cause some signals that would normally kill the
		 * biod to be lost, but this is better than having the biod's
		 * die for unknown reasons.  This will not affect the delivery
		 * of the signal to the foreground process.
		 */
		u.u_procp->p_sig = 0;
		NFS_PUTBUF(nfsp);
	}
}

/* HPNFS
 * ENOSPC fix from 3.2.  This is where the error is set into r_error
 * if there is a problem with an asynchronous write
 * Mike Shipley 09/01/87
 */

do_bio(bp)
	register struct buf *bp;
{
	register struct rnode *rp = vtor(bp->b_vp);
	int count, resid;
 	struct ucred *cred;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
	    "do_bio: addr %x, blk %d, offset %d, size %d, B_READ %d\n",
	    bp->b_un.b_addr, bp->b_blkno, bp->b_blkno * DEV_BSIZE,
	    bp->b_bcount, bp->b_flags & B_READ);
#endif
#ifdef	FSD_KI
	/* stamp bp with start time */
	KI_getprectime(&bp->b_timeval_qs);
#endif  FSD_KI

 	/*BUG FIX:
 	  This routine passes (rp->r_rcred or rp->r_wcred) as a parameter 
          to lower level routines.  Those routines may sleep before using 
	  this cred structure.  Meanwhile, another process may access the 
	  same vnode and cause the cred structure to be released (for 
	  example in nfs_rdwr).  If this is the last reference to this cred
	  structure, the reference count will go to zero, and the cred will
	  be released, and subsequently referenced by the lower level 
          routines.  To fix this, we must save rp->r_cred in a temporary 
	  variable, and do a crhold on it, doing a crfree at the end of 
      	  the routine.
 	  */

        /* PHKL_2836
    	 * 
         * The following change insures that do_bio will use
         * the new field r_wcred in the rnode to perform all delayed
         * writes. This insures no conflict of credentials from
         * intermixed reads occurring prior to this buffer being
         * flushed.
         */

        if ((bp->b_flags & B_READ) == B_READ) {
                cred = rp->r_rcred;
        } else {
                cred = rp->r_wcred;
        }
        crhold(cred);
	if ((bp->b_flags & B_READ) == B_READ) {
		bp->b_error = nfsread(bp->b_vp, bp->b_un.b_addr,
			bp->b_blkno * DEV_BSIZE, (int)bp->b_bcount,
			&resid, cred);
		/*
		 * added check to see if there was an error on the nfsread
		 * because we panic when we call bzero from here if there
		 * was an error - ghs
		 */
		if (bp->b_error == 0) {
			if (resid) {
#ifdef __hp9000s800
				privlbzero(bvtospace(bp, bp->b_un.b_addr),
				       bp->b_un.b_addr + bp->b_bcount - resid,
				       (u_int) resid);
#else 				
				bzero(bp->b_un.b_addr + bp->b_bcount - resid,
				     	    (u_int) resid);
#endif hp9000s800				
			}
			if ( rp->r_size < rp->r_nfsattr.na_size) {
				rp->r_size = rp->r_nfsattr.na_size;
			}
		}
	} else {
		/*
		 * ENOSPC fix from 3.2.
		 * If the write fails and it was asynchronous
		 * then all future writes will get an error.
		 *      Mike Shipley 09/01/87
		 */
		if (rp->r_error == 0) {
			count = MIN(bp->b_bcount,
				(int)rp->r_size - bp->b_blkno * DEV_BSIZE);
			if (count > 0) {
				bp->b_error=nfswrite(bp->b_vp, bp->b_un.b_addr,
			     	    (int) bp->b_blkno * DEV_BSIZE, count, cred);
				if (bp->b_flags & B_ASYNC)
					rp->r_error = bp->b_error;
			}
		} else {
			bp->b_error = rp->r_error;
		}
	}
	if (bp->b_error) {
		bp->b_flags |= B_ERROR;
	}
 	crfree(cred);
#ifdef	FSD_KI
	KI_do_bio(bp);
#endif	FSD_KI
}



int
nfs_badop()
{
	panic("nfs_badop");
}


/*
 * We have all these new features for POSIX and security that aren't supported
 * with the current NFS protocol.  Therefore we return EOPNOTSUP, the same
 * as we do with select, etc.
 */

/* VARARGS */
int nfs_notsupported()
{
	u.u_error = EOPNOTSUPP;
	return ( EOPNOTSUPP );
}

/*
 * This will allow the returning of EREMOTE (as sun does)
 * for the fid call instead of returning EOPNOTSUPP.
 */
int
nfs_noop()
{
        u.u_error = EREMOTE;
        return (EREMOTE);
}


#ifdef POSIX

/*
 * The POSIX pathconf() call is kinda weird.  Some of the requests are
 * about ttys, etc., and are not file system specific.  Therefore we
 * can give reasonable results for those values.  However, the rest of
 * the values must be returned as errors, since there is currently
 * no support in the protocol.
 */

/*ARGSUSED*/
nfs_pathconf(vp, name, resultp, cred)
struct vnode *vp;
int	name;
int	*resultp;
struct ucred *cred;	/* unused */
{
	switch(name) {

	case _PC_LINK_MAX:	/* Maximum number of links to a file */
	case _PC_NAME_MAX:	/* Max length of file name */
	case _PC_PATH_MAX:	/* Maximum length of Path Name */
	case _PC_PIPE_BUF:	/* Max atomic write to pipe.  See fifo_vnops */
	case _PC_CHOWN_RESTRICTED:	/* Anybody can chown? */
	case _PC_NO_TRUNC:	/* No file name truncation on overflow? */
		u.u_error = EOPNOTSUPP;
		return(EOPNOTSUPP);
		break;

	case _PC_MAX_CANON:	/* TTY buffer size for canonical input */
		/* need more work here for pty, ite buffer size, if differ */
		if (vp->v_type != VCHR) {
			u.u_error = EINVAL;
			return(EINVAL);
		}
		*resultp = CANBSIZ;	/*for tty*/
		break;

	case _PC_MAX_INPUT:
		/* need more work here for pty, ite buffer size, if differ */
		if (vp->v_type != VCHR) {	/* TTY buffer size */
			u.u_error = EINVAL;
			return(EINVAL);
		}
		*resultp = TTYHOG;	/*for tty*/
		break;

	case _PC_VDISABLE:	
		/* Terminal special characters can be disabled? */
		if (vp->v_type != VCHR) {
			u.u_error = EINVAL;
			return(EINVAL);
		}
		*resultp = 1;
		break;

	default:
		return(EINVAL);
	}

	return(0);
}
#endif POSIX

/*
 * Record-locking requests are passed to the local Lock-Manager daemon.
 */
/*ARGSUSED*/
int
nfs_lockctl(vp, ld, cmd, cred, fp, LB, UB)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
	struct file *fp;
	off_t LB, UB;
{
	lockhandle_t lh;
	register int oldwhence = ld->l_whence;
	struct flock flock;

#ifndef lint
#ifdef NS_QA
	if (sizeof (lh.lh_id) != sizeof (fhandle_t))
		panic("fhandle and lockhandle-id are not the same size!");
#endif NS_QA
#endif
	/*
	 * Create a local copy of the structure so that we can mess
	 * with the values without worrying about the caller
	 */
	flock = *ld;
	/*
 	 * Sun has the following code in fcntl(), but we have it here
	 * as it becomes NFS specific since HP-UX supports local
	 * locks in a different manner.  Basically, we have to normalize
	 * the lock request that goes out to the lock manager.
	 */

	if ( u.u_error = rewhence( &flock, fp, 0))
		    return(u.u_error);

	/* convert negative lengths to positive */
	if ( flock.l_len < 0 ) {
		flock.l_start += flock.l_len;
		flock.l_len = -(flock.l_len);
	}

	/*
	 * If we are setting a lock mark the rnode NOCACHE so the buffer
	 * cache does not give inconsistent results on locked files shared
	 * between clients. The NOCACHE flag is never turned off as long
	 * as the vnode is active because it is hard to figure out when the
	 * last lock is gone.
	 */
	if (((vtor(vp)->r_flags & RNOCACHE) == 0) &&
	    (ld->l_type != F_UNLCK) && (cmd != F_GETLK)) {
		vtor(vp)->r_flags |= RNOCACHE;
		binvalfree(vp);
	}
	lh.lh_vp = vp;
	lh.lh_servername = vtomi(vp)->mi_hostname;
	bcopy((caddr_t)vtofh(vp), (caddr_t)&lh.lh_id, sizeof(fhandle_t));

	/*
 	 * Call the lock manager code, and then translate errors
	 * to be POSIX compatible.
	 */
	switch (u.u_error = klm_lockctl(&lh, &flock, cmd, cred)) {

	case 0:
		break;          /* continue, if successful */
	/*
	 * Sun had a special case here for EWOULDBLOCK, but the lock
	 * manager returns EACCESS??  Bogosity!
	 */
#ifdef notdef
	case EWOULDBLOCK:
		u.u_error = EACCES;     /* EAGAIN ??? */
		return (u.u_error);
#endif notdef
	default:
		return (u.u_error);         /* some other error code */
	}

	/*
	 * if F_GETLK, modify the passed in flock structure, which will
	 * later be copied to the user.  If the file is unlock'd, then
	 * modify only the l_type field per SVID, otherwise, normalize
         * the result we got from the lock manager relative to the "whence"
	 * value we got passed in.
	 */
	if (cmd == F_GETLK) {
		if (flock.l_type == F_UNLCK) {
			ld->l_type = flock.l_type;
		} else {
			if (u.u_error = rewhence(&flock, fp, oldwhence))
				return (u.u_error);
			*ld = flock;
		}
	}

	return (u.u_error);
}

/*
 * nfs_lockf().  Sun doesn't haven't an nfs_lockf(), so this code is
 * totally new.  This came about because HP-UX has lockf() implemented as
 * a system call while Sun has it implemented as a library (apparently).
 * To handle this, we have to translate the lockf() request into an
 * fcntl() looking request, and then translate the results back if necessary.
 * we call klm_lockctl() directly instead of nfs_lockctl() because we have
 * to translate the results slightly differently and we can avoid some
 * of the overhead of doing extra copies in nfs_lockctl().
 */

/*ARGSUSED*/
int
nfs_lockf( vp, flag, len, cred, fp, LB, UB )
    struct vnode *vp;
    int flag;
    off_t len;
    struct ucred *cred;
    struct file *fp;
    off_t LB, UB;
{
	struct flock flock;
	int cmd;
	lockhandle_t lh;


	/*
	 * Create a flock structure and translate the lockf request
	 * into an appropriate looking fcntl() type request for klm_lockctl()
	 */

	flock.l_whence = 0;
	flock.l_len = len;
	flock.l_start = fp->f_offset;

	/* convert negative lengths to positive */
	if ( flock.l_len < 0 ) {
		flock.l_start += flock.l_len;
		flock.l_len = -(flock.l_len);
	}
	/*
	 * Adjust values to look like fcntl() requests.
	 * All locks are write locks, only F_LOCK requests
	 * are blocking.  F_TEST has to be translated into
	 * a get lock and then back again.
	 */

	flock.l_type = F_WRLCK;
	cmd = F_SETLK;
	switch ( flag ) {

	case F_ULOCK:
		flock.l_type = F_UNLCK;
		break;
	case F_LOCK:
		cmd = F_SETLKW;
		break;
	case F_TEST:
		cmd = F_GETLK;
		break;
	}

        /*
         * If we are setting a lock mark the rnode NOCACHE so the buffer
         * cache does not give inconsistent results on locked files shared
         * between clients. The NOCACHE flag is never turned off as long
         * as the vnode is active because it is hard to figure out when the
         * last lock is gone.
         */
        if (((vtor(vp)->r_flags & RNOCACHE) == 0) &&
            (flock.l_type != F_UNLCK) && (cmd != F_GETLK)) {
                vtor(vp)->r_flags |= RNOCACHE;
                binvalfree(vp);
        }

        lh.lh_vp = vp;
        lh.lh_servername = vtomi(vp)->mi_hostname;
        bcopy((caddr_t)vtofh(vp), (caddr_t)&lh.lh_id, sizeof(fhandle_t));


	/*
	 * Dispatch out to klm_lockctl to do the actual locking.
	 * Then, translate error codes for SVID compatibility
	 */
	switch (u.u_error = klm_lockctl( &lh, &flock, cmd, fp->f_cred)) {

	case 0:
		break;		/* continue, if successful */
	/*
	 * Note that Sun had a special case here for EWOULDBLOCK to
	 * translate it to EACCES, but the lock manager returns EACCES!
	 */
	default:
		return(u.u_error);		/* some other error code */
	}

	/*
	 * if request is F_TEST, and GETLK changed
	 * the lock type to ULOCK, then return 0, else
	 * set errno to EACCESS and return.
	 */
	if ( flag == F_TEST && flock.l_type != F_UNLCK) {
		u.u_error = EACCES;
		return (u.u_error);
	}
	return (0);
}

/*
 * Normalize SystemV-style record locks
 */
rewhence(ld, fp, newwhence)
	struct flock *ld;
	struct file *fp;
	int newwhence;
{
	struct vattr va;
	register int error;

	/* if reference to end-of-file, must get current attributes */
	if ((ld->l_whence == 2) || (newwhence == 2)) {
		if (error =
		    VOP_GETATTR((struct vnode *)fp->f_data, &va,u.u_cred,VSYNC))
			return(error);
	}

	/* normalize to start of file */
	switch (ld->l_whence) {
	case 0:
		break;
	case 1:
		ld->l_start += fp->f_offset;
		break;
	case 2:
		ld->l_start += va.va_size;
		break;
	default:
		return(EINVAL);
	}

	/* renormalize to given start point */
	switch (ld->l_whence = newwhence) {
	case 1:
		ld->l_start -= fp->f_offset;
		break;
	case 2:
		ld->l_start -= va.va_size;
		break;
	}
	return(0);
}


/*
 * Determine if this vnode is a file that is read-only
 */
isrofile(vp)
	struct vnode *vp;
{
	return (vp->v_type != VCHR &&
		vp->v_type != VBLK &&
		vp->v_type != VFIFO &&
		(vp->v_vfsp->vfs_flag & VFS_RDONLY));
}
