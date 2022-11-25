/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/nfs/RCS/nfs_vfsops.c,v $
 * $Revision: 1.8.83.5 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/10/28 14:51:19 $
 */

/* SCCSID strings for NFS/300 group */
/*	%I%	%E%	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../ufs/fsdir.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/pathname.h"
#include "../h/uio.h"
#include "../h/socket.h"
#include "../netinet/in.h"
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#include "../rpc/auth.h"
#include "../rpc/clnt.h"
#include "../nfs/nfs.h"
#include "../nfs/nfs_clnt.h"
#include "../nfs/rnode.h"

#include "../h/mount.h"

/*
 * added by gmf for dux/nfs interactions
 */
#include "../h/buf.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"
#include "../dux/cct.h"
extern struct vfs *find_mntinfo();
extern int enter_mntinfo();
extern int broadcast_mount();
extern site_t my_site;

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

struct vnode *makenfsnode();
int nfsmntno;

/*
 * nfs vfs operations.
 */
int nfs_mount();
int nfs_unmount();
int nfs_root();
int nfs_statfs();
int nfs_sync();
extern int nfs_notsupported();
#ifdef GETMOUNT
int nfs_getmount();
#endif GETMOUNT

struct vfsops nfs_vfsops = {
	nfs_mount,
	nfs_unmount,
	nfs_root,
	nfs_statfs,
	nfs_sync,
	nfs_notsupported,	/* nfs_vget() */
#ifdef GETMOUNT
	nfs_getmount
#endif GETMOUNT
};

/*
 * nfs mount vfsop
 * Set up mount info record and attach it to vfs struct.
 */
/*ARGSUSED*/
nfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	struct vnode *rootvp = NULL;	/* the server's root */
	struct mntinfo *mi = NULL;	/* mount info, pointed at by vfs */
	struct vattr va;		/* root vnode attributes */
	struct nfsfattr na;		/* root vnode attributes in nfs form */
	struct statfs sb;		/* server's file system stats */
	fhandle_t fh;			/* root fhandle */
	struct nfs_args args;		/* nfs mount arguments */

	/*
	 * get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof (args));
	if (error) {
		goto errout;
	}

	/*
	 * create a mount record and link it to the vfs struct
	 */
	mi = (struct mntinfo *)kmem_alloc((u_int)sizeof(*mi));
	mi->mi_refct = 0;
	mi->mi_stsize = 0;
	mi->mi_hard = ((args.flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((args.flags & NFSMNT_INT) == NFSMNT_INT);
	mi->mi_devs = ((args.flags & NFSMNT_NODEVS) == 0);
	mi->mi_ignore = ((args.flags & NFSMNT_IGNORE) == NFSMNT_IGNORE);
	mi->mi_noac = ((args.flags & NFSMNT_NOAC) == NFSMNT_NOAC);
	mi->mi_nocto = ((args.flags & NFSMNT_NOCTO) == NFSMNT_NOCTO);
	mi->mi_dynamic = ((args.flags & NFSMNT_DYNAMIC) == NFSMNT_DYNAMIC);

	/* Have to initialize this so delete_mntinfo doesn't freak if
	 * we goto errout.  If mntno is < 0, it can index below the start of
	 * the array. cwb
	 */
	mi->mi_mntno = 0;

	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto errout;
		}
	} else {
		mi->mi_retrans = NFS_RETRIES;
	}
	if (args.flags & NFSMNT_TIMEO) {
		mi->mi_timeo = args.timeo;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto errout;
		}
	} else {
		mi->mi_timeo = NFS_TIMEO;
	}

	mi->mi_acregmin = ACREGMIN;
	mi->mi_acregmax = ACREGMAX;
	mi->mi_acdirmin = ACDIRMIN;
	mi->mi_acdirmax = ACDIRMAX;
        if (args.flags & NFSMNT_ACREGMIN) {
		if (args.acregmin < 0) {
			mi->mi_acregmin = ACMINMAX;
		} else if (args.acregmin == 0) {
                        error = EINVAL;
                        goto errout;
                } else {
                        mi->mi_acregmin = MIN(args.acregmin, ACMINMAX);
                }
        }
        if (args.flags & NFSMNT_ACREGMAX) {
                if (args.acregmax < 0) {
                        mi->mi_acregmax = ACMAXMAX;
                } else if (args.acregmax < mi->mi_acregmin) {
                        error = EINVAL;
                        goto errout;
                } else {
                        mi->mi_acregmax = MIN(args.acregmax, ACMAXMAX);
                }
        }
        if (args.flags & NFSMNT_ACDIRMIN) {
		if (args.acdirmin < 0) {
			mi->mi_acdirmin = ACMINMAX;
		} else if (args.acdirmin == 0) {
                        error = EINVAL;
                        goto errout;
                } else {
                        mi->mi_acdirmin = MIN(args.acdirmin, ACMINMAX);
                }
        }
        if (args.flags & NFSMNT_ACDIRMAX) {
                if (args.acdirmax < 0) {
                        mi->mi_acdirmax = ACMAXMAX;
                } else if (args.acdirmax < mi->mi_acdirmin) {
                        error = EINVAL;
                        goto errout;
                } else {
                        mi->mi_acdirmax = MIN(args.acdirmax, ACMAXMAX);
		}
	}
        if (mi->mi_noac) {
                mi->mi_acregmin = 0;
                mi->mi_acregmax = 0;
                mi->mi_acdirmin = 0;
                mi->mi_acdirmax = 0;
        }

	do
	{
		mi->mi_mntno = (nfsmntno++)&0xffff | my_site<<16;

	} while (find_mntinfo(mi->mi_mntno));	/*avoid recycling mount IDs*/
	enter_mntinfo(mi);
	mi->mi_vfs = vfsp;
	mi->mi_printed = 0;
	error = copyin((caddr_t)args.addr, (caddr_t)&mi->mi_addr,
	    sizeof(mi->mi_addr));
	if (error) {
		goto errout;
	}
	/*
	 * For now we just support AF_INET
	 */
	if (mi->mi_addr.sin_family != AF_INET) {
		error = EPFNOSUPPORT;
		goto errout;
	}
	if (args.flags & NFSMNT_HOSTNAME) {
		error = copyin((caddr_t)args.hostname, (caddr_t)mi->mi_hostname,
		    HOSTNAMESZ);
		if (error) {
			goto errout;
		}
	} else {
		addr_to_str(&(mi->mi_addr), mi->mi_hostname);
	}
	mi->mi_hostname[HOSTNAMESZ-1] = '\0'; /* Just to make sure of a null */
				 /* terminated string.   MDS  (CND) 05/05/87 */

#ifdef	GETMOUNT
	if (args.flags & NFSMNT_FSNAME) {
		int l;
		error = copyinstr((caddr_t)args.fsname, vfsp->vfs_name,
				  sizeof(vfsp->vfs_name),
				  &l);
		/* ensure null termination */
		vfsp->vfs_name[sizeof(vfsp->vfs_name)-1] = '\0';
		if (error) {
			goto errout;
		}
	}
	else {
		/* Make sure we know there is no name there. */
		vfsp->vfs_name[0] = '\0';
	}
	(void) strncpy(mi->mi_fsmnt, path, sizeof(mi->mi_fsmnt));
#endif	GETMOUNT

	vfsp->vfs_fsid[0] = mi->mi_mntno;
	vfsp->vfs_fsid[1] = MOUNT_NFS;

	vfsp->vfs_data = (caddr_t)mi;

	/*
	 * Make the root vnode
	 */
	error = copyin((caddr_t)args.fh, (caddr_t)&fh, sizeof(fh));
	if (error) {
		goto errout;
	}
	rootvp = makenfsnode(&fh, (struct nfsfattr *) 0, vfsp);
	if (rootvp->v_flag & VROOT) {
		error = EBUSY;
		goto errout;
	}

	/*
	 * get attributes of the root vnode then remake it to include
	 * the attributes.
	 */
	/* HPNFS
	 * extra paramter added for DUX compatiability
	 * HPNFS */
	error = VOP_GETATTR(rootvp, &va, u.u_cred,VSYNC);
	if (error) {
		goto errout;
	}

	VN_RELE(rootvp);
	vattr_to_nattr(&va, &na);
	rootvp = makenfsnode(&fh, &na, vfsp);
	rootvp->v_flag |= VROOT;
	mi->mi_rootvp = rootvp;

	/*
	 * Get server's filesystem stats.  Use these to set transfer
	 * sizes, filesystem block size, and read-only.
	 */
	error = VFS_STATFS(vfsp, &sb);
	if (error) {
		goto errout;
	}
	mi->mi_tsize = min(NFS_MAXDATA, nfstsize());
	mi->mi_curread = mi->mi_tsize;
	if (args.flags & NFSMNT_RSIZE) {
		if (args.rsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_tsize = MIN(mi->mi_tsize, args.rsize);
		mi->mi_curread = mi->mi_tsize;
	}
	if (args.flags & NFSMNT_WSIZE) {
		if (args.wsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_stsize = MIN(mi->mi_stsize, args.wsize);
		mi->mi_curwrite = mi->mi_stsize;
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 1,
	    "nfs_mount: hard %d timeo %d retries %d wsize %d rsize %d\n",
	    mi->mi_hard, mi->mi_timeo, mi->mi_retrans, mi->mi_stsize,
	    mi->mi_tsize);
#endif
	/*
	 * Should set read only here!
	 */

        /*
         * PHKL_2788
         *
         * Set filesystem block size to maximum data transfer size
         * per Sun nfs 4.2.
         *
         * Old code was:
         * mi->mi_bsize = MAX(va.va_blocksize, NBPG);
         * mi->mi_bsize = MIN(mi->mi_bsize, MAXBSIZE);
         */

        mi->mi_bsize = NFS_MAXDATA;

        /* End PHKL_2788 */

#ifdef  hpux
        /*
         * Fix for DSDe405988:  Code in nfs_fsync() and nfs_bmap() assumes
         * that the remote file system block size is a multiple of 1K.
         * Otherwise, funny things can happen, including loss of packets.
         * So...  force the remote file system block size to be a multiple
         * of 1K right here.         --jcm   4/7/1992  Added to WS branch
	 * by cwb.
         */
        mi->mi_bsize &= ~(DEV_BSIZE - 1);
#endif
	vfsp->vfs_bsize = mi->mi_bsize;

	error = broadcast_mount(vfsp);

errout:
	if (error) {
		if (mi) {
			delete_mntinfo(mi);
			kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
		}
		if (rootvp) {
			VN_RELE(rootvp);
		}
	}
	return (error);
}

#ifdef notneeded
/*
 * Called by vfs_mountroot when nfs is going to be mounted as root
 */
nfs_mountroot()
{

	return(EOPNOTSUPP);
}
#endif

/*
 * vfs operations
 */

nfs_unmount(vfsp)
	struct vfs *vfsp;
{
	int error = 0;
	struct mntinfo *mi = (struct mntinfo *)vfsp->vfs_data;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_unmount(%x) mi = %x\n", vfsp, mi);
#endif
	rflush(vfsp);

	/*
	 * free vnodes held in buffer cache
	 */
	rinval(vfsp);

	if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 1) {
		return (EBUSY);
	}
	if (error = send_nfs_unmount(vfsp))
		return (error);
	VN_RELE(mi->mi_rootvp);
	delete_mntinfo(mi);
	kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
	return(0);
}

/*
 * find root of nfs
 */
/*ARGSUSED*/
int
nfs_root(vfsp, vpp, parm1)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *parm1;	/* an unused dummy -- the macro used to call this */
			/* function uses three parameters, but we have no */
			/* need for the third -- gmf */
{

	*vpp = (struct vnode *)((struct mntinfo *)vfsp->vfs_data)->mi_rootvp;
	VN_HOLD(*vpp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_root(0x%x) = %x\n", vfsp, *vpp);
#endif
	return(0);
}


/*Code for NFS unmount*/

struct nfs_umount_msg	/*DUX MESSAGE STRUCTURE*/
{
	long mntno;	/*VFS ID*/
};

/*
 * The following functions are used only when both NFS and DUX
 * are present.  They deal with the hashing of the mountinfo structures.
 */

/*
 *find_mntinfo--Find the vfs given the mount number
 */


struct vfs *
find_mntinfo(mntno)
register int mntno;
{
	register struct mntinfo *mp;

	mp = MNTINFOHASH(mntno);
	while (mp != NULL)
	{
		if (mp->mi_mntno == mntno)
			return(mp->mi_vfs);
		mp = mp->mi_next;
	}
	return (NULL);
}



/*
 *enter_mntinfo--Enter the mount info structure into the hash table.
 *This function does not perform error checking to guarantee that the
 *ID is already in use.
 */
enter_mntinfo(mi)
register struct mntinfo *mi;
{
	register struct mntinfo **mh = &(MNTINFOHASH(mi->mi_mntno));

	mi->mi_next = *mh;
	*mh = mi;
}

/*
 *delete_mntinfo--Delete the mount info structure from the hash table
 */
delete_mntinfo(mi)
register struct mntinfo *mi;
{
	register struct mntinfo **mh = &(MNTINFOHASH(mi->mi_mntno));

	while (*mh != NULL)
	{
		if (*mh == mi)
		{
			*mh = mi->mi_next;
			return;
		}
		mh = &((*mh)->mi_next);
	}
}
/*
 *Broadcast an NFS unmount to all members of the cluster.  If anyone reports
 *that they are busy, send an abort message.  Otherwise, send a commit message.
 *Return EBUSY if anyone was busy, 0 otherwise.
 */
int
send_nfs_unmount(vfsp)
struct vfs *vfsp;
{
	dm_message request, reply;
	struct nfs_umount_msg *rp;
	int error = 0;

	/* If we aren't clustered, just return */
	if (!(my_site_status & CCT_CLUSTERED))
		return (0);
	request = dm_alloc (sizeof (struct nfs_umount_msg), WAIT);
	rp = DM_CONVERT(request, struct nfs_umount_msg);
	rp->mntno = ((struct mntinfo *)(vfsp->vfs_data))->mi_mntno;
	reply = dm_send(request, DM_SLEEP, DM_NFS_UMOUNT, DM_CLUSTERCAST,
		DM_EMPTY, NULL, 0, 0, 0, 0, 0, 0);
 	error =  DM_RETURN_CODE(reply);
	dm_release(reply, 1);
	/*send a commit or abort message*/
	dm_send(request,DM_RELEASE_REQUEST|DM_RELEASE_REPLY|DM_SLEEP,
		error?DM_ABORT_NFS_UMOUNT:DM_COMMIT_NFS_UMOUNT, DM_CLUSTERCAST,
		DM_EMPTY, NULL, NULL, 0, 0, 0, 0, 0);
	return(error);
}

/*Service an NFS unmount request*/
nfs_umount_serve(request)
dm_message *request;
{
	struct vfs *vfsp;
	struct mntinfo *mi;
	struct nfs_umount_msg *rp;
	int error = 0;

	rp = DM_CONVERT(request, struct nfs_umount_msg);
	/* find the VFS */
	vfsp = find_mntinfo (rp->mntno);
	if (vfsp != NULL)
	{
		error = vfs_lock(vfsp);
		if (!error)
		{
			dnlc_purge();
			mi = (struct mntinfo *)vfsp->vfs_data;
			rflush(vfsp);

			/*
			 * free vnodes held in buffer cache
			 */
			if (mi->mi_refct != 1) {
				rinval(vfsp);
			}
			if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 1) {
 				error = EBUSY;
				/*Don't unlock because we will unlock when
				 *we abort*/
			}
		}
	}
 	dm_quick_reply(error);
	/*note:  mount table remains locked pending commit or abort*/
}

#ifdef GETMOUNT
extern struct timeval time;
extern time_t mount_table_change_time;
#endif

/*commit or abort an nfs unmount*/
nfs_umount_commit(request)
dm_message *request;
{
	struct vfs *vfsp;
	struct mntinfo *mi;
	struct nfs_umount_msg *rp;

	rp = DM_CONVERT(request, struct nfs_umount_msg);
	/* find the VFS */
	vfsp = find_mntinfo (rp->mntno);
	if (vfsp != NULL)
	{
		if (DM_OP_CODE(request) == DM_COMMIT_NFS_UMOUNT)
		{
			struct vnode *coveredvp = vfsp->vfs_vnodecovered;

			mi=vftomi(vfsp);
			VN_RELE(mi->mi_rootvp);
			delete_mntinfo(mi);
			kmem_free((caddr_t)mi, (u_int)sizeof(*mi));
			vfs_remove(vfsp);
			VN_RELE(coveredvp);
			kmem_free((caddr_t)vfsp, sizeof (struct vfs));
#ifdef GETMOUNT
			mount_table_change_time = time.tv_sec;
#endif
		}
		else	/*abort*/
		{
 			/* Don't attempt unlock if above lock failed */
 			if (vfsp->vfs_flag & VFS_MLOCK)
 				vfs_unlock(vfsp);
		}
	}
	dm_quick_reply(0);
}


/*
 * Get file system statistics.
 */
int
nfs_statfs(vfsp, sbp)
register struct vfs *vfsp;
struct statfs *sbp;
{
	struct nfsstatfs fs;
	struct mntinfo *mi;
	fhandle_t *fh;
	int error = 0;
#ifdef LOCAL_DISC
	extern site_t root_site;
#endif

	mi = vftomi(vfsp);
	fh = vtofh(mi->mi_rootvp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_statfs fh %o %d\n", fh->fh_fsid, fh->fh_fno);
#endif
	error = rfscall(mi, RFS_STATFS, xdr_fhandle,
	    (caddr_t)fh, xdr_statfs, (caddr_t)&fs, u.u_cred);
	if (!error) {
		error = geterrno(fs.fs_status);
	}
	if (!error) {
		if (mi->mi_stsize) {
			mi->mi_stsize = min(mi->mi_stsize, fs.fs_tsize);
		} else {
			mi->mi_stsize = fs.fs_tsize;
			mi->mi_curwrite = mi->mi_stsize;
		}
/* HPNFS   Name conflict over fs_bsize, see nfs.h  */
		sbp->f_type = 0;
		sbp->f_bsize = fs.fs_bsize_nfs;
		sbp->f_blocks = fs.fs_blocks;
		sbp->f_bfree = fs.fs_bfree;
		sbp->f_bavail = fs.fs_bavail;
		sbp->f_files = -1;   /*NFS does not support this*/
		sbp->f_ffree = -1;   /*NFS does not support this*/
		/*
		 * XXX This is wrong - should be a real fsid  (SUN's comment)
		 */
		bcopy((caddr_t)vfsp->vfs_fsid,
		    (caddr_t)sbp->f_fsid, sizeof (fsid_t));
		/*
		bcopy((caddr_t)&fh->fh_fsid, (caddr_t)sbp->f_fsid,
		    sizeof(fsid_t));
		*/
#ifdef LOCAL_DISC
		sbp->f_cnode = root_site;
#endif
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_statfs returning %d\n", error);
#endif
	return (error);
}

/*
 * Flush any pending I/O.
 */
/*ARGSUSED*/
int
nfs_sync(vfsp)
	struct vfs * vfsp;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_sync %x\n", vfsp);
#endif
/*
 *	Make nfs_sync() a no-op.  The rflush here is redundant
 *	with the update() call in sync().  Moreover, rflush is synchronous
 *	and could cause sync() to hang if an NFS server is down and if
 *	somehow NFS B_DELWRI buffers still existed after the update().
 *
 *	rflush(vfsp);
 */
	return(0);
}

#ifdef GETMOUNT

/*
 * get mount table information
 */

int
nfs_getmount(vfsp, fsmntdir, mdp)
struct vfs *vfsp;
caddr_t fsmntdir;
struct mount_data *mdp;
{
	struct mntinfo *mi;
	int l;
	extern site_t root_site;

	mi = (struct mntinfo *)vfsp->vfs_data;

	mdp->md_msite = 0; /* my_site ? -- nfs is not site-specific */

	/*
	 * the following is identical to the hack in nfs_getattr()
	 * to give NFS files "meaningful" st_dev fields from stat().
	 */
	mdp->md_dev = 0xff000000 | mi->mi_mntno;

	mdp->md_retrans = mi->mi_retrans;
	mdp->md_timeo	= mi->mi_timeo;
	mdp->md_rsize	= mi->mi_tsize;
	mdp->md_wsize	= mi->mi_stsize;
	mdp->md_port	= mi->mi_addr.sin_port;
	mdp->md_acregmin = mi->mi_acregmin;
	mdp->md_acregmax = mi->mi_acregmax;
	mdp->md_acdirmin = mi->mi_acdirmin;
	mdp->md_acdirmax = mi->mi_acdirmax;

	/*
	 * The code below used to be:
	 *
	 * 	mdp->md_nfsopts = 0;
	 *
	 * To fix problem with update_mnttab() library call which will
	 * not print the corresponding values into the mnttab file unless
	 * the bits NFSMNT_RSIZE, WSIZE, TIMEO, and RETRANS were set
	 * in md_nfsopts.  When a mount is done, these bits are set in
	 * an argument flag to notify the kernel mount code that the user
	 * has specified a value for these fields.  The kernel uses the
	 * user-specified value, but does not save the knowledge that the
	 * user asked for a specific value.  Always setting these flags
	 * in md_nfsoopts is a kludge that will make update_mnttab() work
	 * properly.
	 */
	mdp->md_nfsopts =
		(NFSMNT_RSIZE|NFSMNT_WSIZE|NFSMNT_TIMEO|NFSMNT_RETRANS|
		 NFSMNT_ACREGMIN|NFSMNT_ACREGMAX|NFSMNT_ACDIRMIN|NFSMNT_ACDIRMAX
		);

	if (!mi->mi_hard)
		mdp->md_nfsopts |= NFSMNT_SOFT;
	if (mi->mi_int)
		mdp->md_nfsopts |= NFSMNT_INT;
	if (!mi->mi_devs)
		mdp->md_nfsopts |= NFSMNT_NODEVS;
	if (mi->mi_ignore)
		mdp->md_nfsopts |= NFSMNT_IGNORE;
	if (mi->mi_noac)
		mdp->md_nfsopts |= NFSMNT_NOAC;
	if (mi->mi_nocto)
		mdp->md_nfsopts |= NFSMNT_NOCTO;
	if (mi->mi_dynamic)
		mdp->md_nfsopts |= NFSMNT_DYNAMIC;

	return(copyoutstr(mi->mi_fsmnt, fsmntdir, sizeof(mi->mi_fsmnt), &l));

}
#endif /* GETMOUNT */

static char *
itoa(n, str)
	u_short n;
	char *str;
{
	char prbuf[11];
	register char *cp;

	cp = prbuf;
	do {
		*cp++ = "0123456789"[n%(u_short)10];
		n /= (u_short)10;
	} while (n);
	do {
		*str++ = *--cp;
	} while (cp > prbuf);
	return (str);
}

/*
 * Convert a INET address into a string for printing
 */
/*
static
*/
addr_to_str(addr, str)
	struct sockaddr_in *addr;
	char *str;
{
	u_char *p = (u_char *) &addr->sin_addr;
	str = itoa(*p++, str);
	*str++ = '.';
	str = itoa(*p++, str);
	*str++ = '.';
	str = itoa(*p++, str);
	*str++ = '.';
	str = itoa(*p++, str);
	*str = '\0';
}
