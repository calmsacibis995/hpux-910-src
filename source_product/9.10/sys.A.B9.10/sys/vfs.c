/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs.c,v $
 * $Revision: 1.18.83.6 $	$Author: craig $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/12/13 17:26:13 $
 */

/* HPUX_ID: @(#)vfs.c	55.1		88/12/23 */

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

/*	@(#)vfs.c 1.1 86/02/03 SMI	*/
/*      NFSSRC @(#)vfs.c	2.1 86/04/15 */

#ifndef _SYS_STDSYMS_INCLUDED
#    include "../h/stdsyms.h"
#endif   /* _SYS_STDSYMS_INCLUDED  */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/vfs.h"
#include "../h/socket.h"
#include "../h/syscall.h"
#include "../h/mount.h"
#include "../h/pathname.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/systm.h"
#include "../h/swap.h"
#include "../dux/lookupops.h"
#include "../dux/cct.h"
#include "../dux/dux_dev.h"
#include "../netinet/in.h"
#include "../nfs/nfs_clnt.h"
#if defined(__hp9000s700) || defined(__hp9000s800)
#include "../h/buf.h"
#include "../ufs/fs.h"
#endif /* s700 || s800 */

#include "../h/kern_sem.h"
#ifdef AUDIT
#include "../h/audit.h"
#endif /* AUDIT */

#ifndef VOLATILE_TUNE
/*
 * If VOLATILE_TUNE isn't defined, define 'volatile' so it expands to
 * the empty string.
 */
#define volatile
#endif /* VOLATILE_TUNE */

/*
**  Next few lines for VFS registration.  - John McCarthy 92jul13
*/
#define MOUNT_HOLDTYPE 4        /* 0-3 taken by UFS, NFS, CDFS, and AFS */
int next_fs=MOUNT_HOLDTYPE;     /* next available fs type in vfssw[] */
struct reg_fs_type {
        struct reg_fs_type *next;
        char    *fs_name;
        int     fs_type;
} *reg_fs_list=NULL;            /* linked list of registered file systems */
extern int  nvfssw;             /* from conf.c file */


/*
 * vfs global data
 */
struct vnode *rootdir;			/* pointer to root vnode */

struct vfs *rootvfs;			/* pointer to root vfs. This is */
					/* also the head of the vfs list */
static void vfs_next_lock(), vfs_next_unlock();

/*
 * counter for swap chunk release calls from xsync
 */
int swap_rel_ctr = 0;
int swap_rel_max = 4;

#ifdef GETMOUNT 
/*
 * mount_table_change_time is a global timestamp that is updated
 * whenever the mounted file system status changes in any way.
 * It is updated for every successful mount and unmount operation (including that
 * done in cluster recovery in fs_cleanup()).  In a cluster, changes to this
 * timestamp are propagated to all nodes through DUX messages.  Because the
 * timestamp is not updated simultaneously on all cluster nodes, a successful
 * unmount or crash recovery operation may leave the timestamp inconsistent
 * between cluster nodes by a second or more.
 *
 * The following routines modify mount_table_change_time:
 * 
 *	vfs_mount()
 * 	umount()
 * 
 * 	mount_commit()
 *	nfs_umount_commit()
 *	global_umount_serve()
 *	mcleanup()
 *
 */
time_t mount_table_change_time = 0;
extern volatile struct timeval time;
#endif /* GETMOUNT */

/*
 * System calls
 */

/*
 * mount system call
 */
smount(uap)
	register struct a {
		caddr_t	data;
		char	*dir;
		int	flags;
	} *uap;
{
	struct b {
		int	type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} vfsmount_parms, *uap_mnt;

	uap_mnt = &vfsmount_parms;
	uap_mnt->type = MOUNT_UFS;
	uap_mnt->dir = uap->dir;
	uap_mnt->flags = uap->flags;
	uap_mnt->data = uap->data;

	vfsmount(uap_mnt);
}

/**************************************************************************
**
**  get_vfs_type: 92jul13
**      allows user to get registered vfs type by passing in a string
**      that matches the string registered by a file systems linking
**      routine.
**      This routines will OVERWRITE the user's string with the ASCII
**      value of the registered fs's type.
**      See register_vfs_type() below.
*/
static int
get_vfs_type(udata)
        caddr_t udata;
{
        struct reg_fs_type *fstp=reg_fs_list;
        int     error;
        int     match;
        unsigned lcp;   /* dummy var */
        char    tdata[MAXPATHLEN];


    if (udata == (caddr_t)0) {
	return EINVAL;
    }
    error = copyinstr(udata, tdata, MAXPATHLEN, &lcp);
    if (error)
        return(error);

    error = EINVAL;	/* assume not found! */

    /*
    **  Compare the user filesystem name against the
    **  VFS *_link() registered file system names
    */
    while (fstp != NULL) {
        match = strncmp(tdata, fstp->fs_name, MAXPATHLEN);
        if (match == 0) {
            /* match found */
            sprintf(tdata, MAXPATHLEN, "%d", fstp->fs_type);
            error = copyoutstr(tdata, udata, MAXPATHLEN, &lcp);
            break;
        }
        fstp = fstp->next;
    }
    return error;
}

/**************************************************************************
**
**  register_vfs_type: 92jul13
**      Register non-hardcoded filesystems and returns an index into
**      the vfssw[] table.  This routine should be called by the
**      filesystem's linking routine.
**
**      errors:
**	    fs_name null / vfssw[] full / empty string
**	    malloc failed / CLUSTERED(dux)
*/
int
register_vfs_type(fs_name)
        char *fs_name;
{
        struct reg_fs_type *fstp=reg_fs_list;
        struct reg_fs_type *new_type=NULL;
        int     fs_type=0;
        int     s;
#ifdef	MP
	sv_sema_t vfs_SS;
#endif
        if (fs_name == NULL) {
            return(-1);
        }
	else if (strlen(fs_name) == 0) {
	    return(-1);	/* don't allow registration of empty string */
	}

        if (my_site_status & CCT_CLUSTERED) {
            /* no 3rd party file systems on DUX server/cnode */
            /* may need to re-examine this for DUX-Plus %%%, 92jul13 */
	    msg_printf("FileSys %s not supported on a clustered system\n",
								fs_name);
            return(-1);
        }
        while (fstp != NULL) {
            if ( strcmp(fstp->fs_name, fs_name) == 0) {
                /* fs already registered  */
                fs_type = fstp->fs_type;
		msg_printf("FileSys %s ALREADY registered as type %d \n",
						    fs_name, fs_type);
                return(fs_type);
            }
            else
                fstp = fstp->next;
        }

#ifdef MP
	PXSEMA(&filesys_sema,&vfs_SS);
#endif /* MP */
        /*
        **  Want the while loop here in case user hardcoded vfsops in
        **  the conf.c file after config was executed.
        */
        while ( (next_fs < nvfssw) &&
                (vfssw[next_fs] != (struct vfsops *)0) ) {
            next_fs += 1;
        }
        fs_type = next_fs;      /* save open location */
        next_fs += 1;
#ifdef MP
	VXSEMA(&filesys_sema,&vfs_SS);
#endif /* MP */

        if (next_fs > nvfssw) {
            /* not enough room in vfssw[] */
            return(-1);
        }

        new_type=(struct reg_fs_type *)kmem_alloc(sizeof(struct reg_fs_type));
        if ( new_type == NULL ) {
            return(-1);
        }
        new_type->fs_name = (char *)kmem_alloc(strlen(fs_name) + 1);
        if (new_type->fs_name == NULL) {
            kmem_free(new_type, sizeof(struct reg_fs_type) );
            return(-1);
        }

        /*
        ** now build up the new node
        */
        bcopy(fs_name, new_type->fs_name, strlen(fs_name)+1);
        new_type->fs_type = fs_type;

        /*
        ** put new node at the head of the list
        */
#ifdef MP
	PXSEMA(&filesys_sema,&vfs_SS);
#endif /* MP */
        new_type->next = reg_fs_list;
        reg_fs_list = new_type;
#ifdef MP
	VXSEMA(&filesys_sema,&vfs_SS);
#endif /* MP */

        return(fs_type);
}
 
#define M_EXIT goto out
extern site_t root_site, my_site;

/*
 * vfsmount system call
 */
#ifdef MP
vfsmount(uap)
	register struct a {
		int	type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} *uap;
{
	PSEMA(&filesys_sema);
	_vfsmount(uap);
	VSEMA(&filesys_sema);
}

_vfsmount(uap)
#else /* not MP */
vfsmount(uap)
#endif /* MP */
	register struct a {
		int	type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} *uap;
{
	struct pathname pn;
	struct vnode *vp;
	struct vfs *vfsp;
	struct vattr vattr;
	struct devandsite fsdev;
	int release_vnode=0;
#ifdef	GETMOUNT || QUOTA
	unsigned l;  /* dummy for copyinstr */
	char *devname = (char *)NULL;
#endif	/* GETMOUNT */

	if (uap->type == -1) {
            /* User level inquiry for registered vfs type, 92jul13 */
            u.u_error=get_vfs_type(uap->data);
            return;
        }

#ifdef AUDIT
	if (AUDITEVERON()) {
		if (u.u_syscall == SYS_MOUNT) {		/* mount */
			(void)save_pn_info(uap->data);
			(void)save_pn_info(uap->dir);
		}
		else {					/* vfsmount */
			/*
			 * always store dir
			 */
			(void)save_pn_info(uap->dir);

			/*
			 * if type is MOUNT_UFS, store just
			 * as in mount syscall, but note that
			 * the order is switched.  must first
			 * copy in the structure, since that
			 * is not the data itself.
			 */
			if (uap->type == MOUNT_UFS) {
				struct ufs_args args;

				u.u_error = copyin(uap->data, (caddr_t)&args, 
						sizeof(struct ufs_args));
				if (u.u_error) {
					return;
				}
				(void)save_pn_info(args.fspec);
			}
			else {
				/*
				 * store a bogus record, since data
				 * is not a simple char * to a path
				 */
				(void)save_pn_info(0);
			}
		}
	}
#endif /* AUDIT */

	/*
	 * Must be super user
	 */
	if (!suser())
		return;

	/*
	 * Check data for MOUNT_UFS, make sure its a BLK vnode
	 * XXX This is a kluge doesn't take care of all cases.
	 *     want to assure that 'data' is not below 'dir'
	 *     in the hierarchy
	 */
	 if(uap->type == MOUNT_UFS) {
		struct ufs_args args;
		
		if(u.u_syscall == SYS_MOUNT) {
			/* Old style mount system call */
			args.fspec = uap->data;
		} else {
			u.u_error = copyin(uap->data, (caddr_t)&args, 
						sizeof (struct ufs_args));
			if (u.u_error)
				return;
		}
#ifdef GETMOUNT || QUOTA
		devname = args.fspec;
#endif /* GETMOUNT */
		u.u_error = getmdev(args.fspec, &fsdev.dev, &fsdev.site);
		if(u.u_error)
			return;
		uap->data = (caddr_t)&fsdev;
	}
	 else if(uap->type == MOUNT_CDFS) {
		struct cdfs_args args;
		u.u_error = copyin(uap->data, (caddr_t)&args, 
					sizeof (struct cdfs_args));
		if (u.u_error) return;
#ifdef GETMOUNT || QUOTA	
		devname = args.fspec;
#endif /* GETMOUNT */
		u.u_error = getmdev(args.fspec, &fsdev.dev, &fsdev.site);
		if(u.u_error)
			return;
		uap->data = (caddr_t)&fsdev;
	}
	/*
	 * Get vnode to be covered
	 */
	(void)lock_mount(M_LOCK);
	u.u_error = lookupname(uap->dir, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_LOOKUP, NULL);
	if (u.u_error) {
		release_mount();
		return;
	}
	VOP_GETATTR(vp, &vattr, u.u_cred, VASYNC);
	dnlc_purge();
	if (vp->v_type != VDIR) {
		release_vnode=1;
		u.u_error = ENOTDIR;
		M_EXIT;
	}
	if (vattr.va_mode&VSUID) {	/* CDF, but root CDF doesn't work */
		release_vnode=1;
		u.u_error = EINVAL;
		M_EXIT;
	}
/*  we tossed VOP_GETATTR above and changed test below...  */

	if (vp->v_count != 1) {
		/* Release pages and buffers associated with the
		 * vp in an attempt to get v_count decremented to 1.
		 * These pages and buffers may have been associated
		 * with a previous use of this vnode, but are now
		 * stale.  matt 3/19/91
		 */
		mpurge(vp);
		binval(vp);
	}

	if (vp->v_count != 1 || vp->v_flag & VROOT) {
		release_vnode=1;
		u.u_error = EBUSY;
		M_EXIT;
	}
#ifdef	LOCAL_DISC
	/*
	 * If we are a DUX client, check to make sure the mount point/
	 * mount type combination is supported.
	 */
	if (my_site != root_site) {
		if (uap->type == MOUNT_CDFS) {
			/*
			 * Local mounts of cdfs not yet supported
			 */
			release_vnode=1;
			u.u_error = EINVAL;
			M_EXIT;
		}
		if ((devsite((VTOI(vp))->i_dev) != root_site) &&
		    (vp->v_fstype == VUFS)) {
			/*
			 * Mount point is an LMFS (locally mounted
			 * UFS file system)
			 */
			if ((uap->type == MOUNT_NFS) ||
			    ((uap->type == MOUNT_UFS) &&
			    (remoteip(VTOI(vp))))) {
				/* 
				 * LMFS to NFS mounts not yet supported
				 * Non-local UFS cascading mounts not supported
				 */
				release_vnode=1;
				u.u_error = EINVAL;
				M_EXIT;
			}
		}
	}
#endif	/* LOCAL_DISC */
	if (uap->type >= nvfssw ||  /* used to be >MOUNT_MAXTYPE, 92jul13 */
	    uap->type < 0 ||	    /* new check, 92jul13 */
	    vfssw[uap->type] == (struct vfsops *)0) {
		u.u_error = ENODEV;
		release_vnode=1;
		M_EXIT;
	}
	u.u_error = pn_get(uap->dir, UIOSEG_USER, &pn);
	if (u.u_error) {
		release_vnode=1;
		M_EXIT;
	}

	/*
	 * Mount the filesystem.
	 * Lock covered vnode (XXX this currently only works if it is type ufs)
	 */

	/* lock inode before possible sleep in kmem_alloc/getpages */
	if (vp->v_fstype == VUFS)
		ilock(VTOI(vp));

	vfsp = (struct vfs *)kmem_alloc(sizeof (struct vfs));
	if(vfsp == NULL) {
		u.u_error = EBUSY;
		release_vnode=1;
		pn_free(&pn);
		M_EXIT;
	}
#ifdef GETMOUNT
	if (!devname ||
		(copyinstr(devname, vfsp->vfs_name, sizeof(vfsp->vfs_name),
			&l) != 0)) {
		vfsp->vfs_name[0] = '\0';  /* let's initialize this variable */
	}
#endif /* GETMOUNT */
	VFS_INIT(vfsp, vfssw[uap->type], (caddr_t)0, uap->type);
	u.u_error = vfs_add(vp, vfsp, uap->flags); /* locks vfs */
#ifdef GETMOUNT
	vfsp->vfs_mnttime = time.tv_sec;
#endif /* GETMOUNT */
	if (!u.u_error) {
		u.u_error = VFS_MOUNT(vfsp, pn.pn_path, uap->data);
	}
	pn_free(&pn);
	if (vp->v_fstype == VUFS)
		iunlock(VTOI(vp));
	if (!u.u_error) {
		vfs_unlock(vfsp);
#ifdef GETMOUNT
		/* a mount was performed */
		mount_table_change_time = vfsp->vfs_mnttime;
#endif /* GETMOUNT */
	} else {
		vfs_remove(vfsp);
		kmem_free((caddr_t)vfsp, sizeof (struct vfs));
		release_vnode=1;
	}


out:
	release_mount();
	/* In order to disassociate this lookup with this site, we have
	 * independently incremented the at the server, so decrement the
	 * dcount here.
	 */
	update_duxref(vp, -1, 0);
	/* Have to do the VN_RELE() after the update_duxref() or the vnode
	 * could go away and be reused before we modified the count.
	 */
	if (release_vnode) {
		VN_RELE(vp);
	}
#ifdef	QUOTA
	(void) mnt_opendq( uap, devname);
#endif	/* QUOTA */
	return;
}

sync()
{
	if ((my_site_status & CCT_CLUSTERED))
		global_sync();
	else
		lsync();
}

#ifdef	NSYNC
tsync()
{
	register struct a {	/* caller will have 1 param on stack */
		int	flag;
	} *uap = (struct a *)u.u_ap;
	register struct vfs *vfsp;

	PSEMA(&filesys_sema);
	if ((!uap->flag) || !(my_site_status & CCT_CLUSTERED))
		xsync(1);
	else 
		global_sync();
	VSEMA(&filesys_sema);
}
#endif	/* NSYNC */

/*
 * Sync system call. sync each vfs
 */
/* Local sync.  For global sync see dux_syscalls.c */
lsync()
{
	xsync(0);
}

xsync(trickle)
	int trickle;
{
	register struct vfs *vfsp;
	register int which;
	sv_sema_t xsyncSS;

	/* release swap chunks.
	 * Dux clients doing remote swapping and dux swap servers examine
	 * all chunks in the swap table.
	 * Dux clients doing local swap and standalone systems examine
	 * file system chunks only.
	 * the var which is used to distinguish the two types
	 * Do this only every swap_rel_max entries into xsync.
	 */
	
	which = my_site_status & (CCT_SLWS | CCT_SWPSERVER);
	
	if (which || fswdevt[0].fsw_enable) 
		if (++swap_rel_ctr >= swap_rel_max ) {
 		    swap_rel_ctr = 0;
                    chunk_release(which);
		}

	PXSEMA(&filesys_sema, &xsyncSS);
	/*	moved here from ufs_sync() - once is enough */
#ifdef	NSYNC
	update(0,0,trickle);
#else
	update(0,0);
#endif	/* NSYNC */
	/*
	 *	Prevent mount or unmount from messing up the chain
	 *	while sleeping.
	 */
	vfs_next_lock();

	for (vfsp = rootvfs; vfsp != (struct vfs *)0; vfsp = vfsp->vfs_next) {
		VFS_SYNC(vfsp);
	}

	vfs_next_unlock();
	VXSEMA(&filesys_sema, &xsyncSS);
}

/*
 * get filesystem statistics
 */
statfs(uap)
	struct a {
		char *path;
		struct statfs *buf;
	} *uap;
{
	struct vnode *vp;
	PSEMA(&filesys_sema);
         u.u_error =
             lookupname(uap->path, UIOSEG_USER, FOLLOW_LINK,
                 (struct vnode **)0, &vp, LKUP_STATFS, (caddr_t)uap->buf);
	if (u.u_error) {
		VSEMA(&filesys_sema);
		return;
	}
	cstatfs(vp->v_vfsp, uap->buf);
	VN_RELE(vp);
	VSEMA(&filesys_sema);
}

fstatfs(uap)
	struct a {
		int fd;
		struct statfs *buf;
	} *uap;
{
	struct file *fp;

	PSEMA(&filesys_sema);
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0) {
#ifdef LOCAL_DISC
		/*
		 * We may be trying to fstatfs()
		 * a file that has been "disowned"
		 * after its serving site has crashed
		 * Not clear what the right error for
		 * this case is, but let's be consistent
		 * with the read/write cases and generate EIO.
		 */
		struct vfs *vfsp;

		vfsp = ((struct vnode *)fp->f_data)->v_vfsp;
		if (vfsp == NULL) {
			u.u_error = EIO;
			VSEMA(&filesys_sema);
			return;
		}
		cstatfs(vfsp, uap->buf);
#else	/* LOCAL_DISC */
		cstatfs(((struct vnode *)fp->f_data)->v_vfsp, uap->buf);
#endif	/* LOCAL_DISC */
	}
	VSEMA(&filesys_sema);
}

cstatfs(vfsp, ubuf)
	struct vfs *vfsp;
	struct statfs *ubuf;
{
	struct statfs sb;

	bzero (&sb, sizeof(struct statfs));
	u.u_error = VFS_STATFS(vfsp, &sb);
	if (u.u_error)
		return;
	u.u_error = copyout((caddr_t)&sb, (caddr_t)ubuf, sizeof(sb));
}

/* This routine allows the user to unmount NFS file systems if it is mounted
 * on an NFS file system whose server is not responding.  In that case the
 * lookup would fail, but this routine allows us to find the vfs.
 * This routine looks through the mounted vfs list looking for the mount
 * point if it is an NFS file system.  It only checks NFS file systems so
 * it should be extremely fast if there are no NFS mounts.   It will be a
 * little slower if there are NFS mount points, but unmounting is such a slow
 * operation that nobody will notice.
 * If the path name in the mi structure does not match, then this routine won't
 * find the vfs.  For instance if the user passes in //ss/m/ instead of /ss/m
 * then this routine will fail.  It could be smarter but the failure mode
 * reverts to 9.0 behavior.  The only problem could come about if we succeeded
 * when we should have failed and I don't see how that could happen.
 */

struct vfs *find_nfs_vfs(mntpt)
	char *mntpt;
{
	char mount_name[MAXPATHLEN];
	int len;
	int i;
	struct vfs *vfsp;
#ifdef	GETMOUNT

	/* Get the name from user space */
	if (copyinstr(mntpt, mount_name, sizeof(mount_name), &len) != 0) {
		return;
	}

	/* Make sure there are no trailing slashes so the match works */
	for (i = len-1; i > 0; i--) {
		if (mount_name[i] == '/') {
			mount_name[i] = '\0';
			len--;
		}
		else {
			break;
		}
	}

	/* Look for the mount point in the vfs list */
	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next) {
		if (vfsp->vfs_mtype == MOUNT_NFS) {
			if (!strncmp(vftomi(vfsp)->mi_fsmnt, mount_name,
							sizeof(mount_name))) {
				return(vfsp);
			}
		}
	}
#endif	/* GETMOUNT */

	return(NULL);
}


/*
 * Unmount system call.
 *
 * Note: This version of umount has been extended to accept either a
 * directory or a special file as the argument.
 */

umount(uap)
	struct a {
		char	*pathp;
	} *uap;
{
	struct vnode *vp;
	struct vfs *vfsp;

#ifdef AUDIT
	if (AUDITEVERON()) {
		(void)save_pn_info(uap->pathp);
	}
#endif /* AUDIT */

	if (!suser()) {
		u.u_error = EPERM;
		return;
	}
	PSEMA(&filesys_sema);
	/* For DUX */
	(void)lock_mount(M_LOCK);

	/* If its an NFS mount then we can find it another way.  We do this
	 * in case its server is not responding.
	 */
	if (((vfsp = find_nfs_vfs(uap->pathp)) != NULL)) {
		u.u_error = umount_vfs(vfsp);

		/* If it succeeded or the file system is busy then quit */
		if ((u.u_error == 0) || (u.u_error == EBUSY)) {
			goto out;
		}

		/* It failed, clear the error and try it the normal way.
		 * This is just in case there is a bug in my code.  It might
		 * save a problem some day.
		 */
		u.u_error = 0;
	}
	/*
	 * lookup path (either root of fs or mounted device)
	 */
	u.u_error = lookupname(uap->pathp, UIOSEG_USER, FOLLOW_LINK,
			(struct vnode **)0, &vp, LKUP_UMOUNT, NULL);
	if (u.u_error){
		goto out;
	}

	if (vp->v_type == VBLK)
	{
		u.u_error = umount_dev_vp(vp);
	}
	else if (vp->v_flag & VROOT)
	{
		u.u_error = umount_root(vp);
	}
	else
	{
		u.u_error = ENOTBLK;
		VN_RELE(vp);
	}
out:
#ifdef	GETMOUNT
	/* an unmount was successfully performed */
	if (!u.u_error)
		mount_table_change_time = time.tv_sec;
#endif	/* GETMOUNT */

	release_mount();
	VSEMA(&filesys_sema);
}

/*
 * Unmount, given the vfs
 */
umount_vfs(vfsp)
	register struct vfs *vfsp;
{
	register struct vnode *coveredvp;
	int error;

	/*
	 * get covered vnode
	 */
	coveredvp = vfsp->vfs_vnodecovered;
	/*
	 * lock vnode to maintain fs status quo during unmount
	 */
	error = vfs_lock(vfsp);
	if (error)
		goto errout;

	xumount(vfsp);	/* remove unused sticky files from text table */
	dnlc_purge();	/* remove dnlc entries for this file sys */
#ifdef	NSYNC
	update(0,1,0);
#else
	update(0,1);
#endif	/* NSYNC */

	if (vfsp->vfs_icount) { /* someone is sleeping on the mnt'd on inode */
		vfs_unlock(vfsp);
		error = EBUSY;
		goto errout;
	}
	error = VFS_UNMOUNT(vfsp);
	if (error) {
		vfs_unlock(vfsp);
	} else {
		int do_release = (vfsp != rootvfs);
		vfs_remove(vfsp);
		/* Make sure you release vnode AFTER vfs_remove since it
		 * references the vnode.
		 */
		if (do_release)
			VN_RELE(coveredvp);
		kmem_free((caddr_t)vfsp, (u_int)sizeof(*vfsp));
	}
errout:
	return (error);
}


/*
 * Unmount a file system given the device vnode
 */
int 
umount_dev_vp(vp)
struct vnode *vp;
{
	struct inode *ip;
	dev_t dev;
	site_t site;

	/* If this is not a ufs vnode, we can't unmount it.  (Note that it
	 * can't be a dux vnode, because this is always executed at the vnode
	 * serving site
	 */
	if (vp->v_fstype != VUFS)
		return (EINVAL);
	ip = (VTOI(vp));
	dev = ip->i_device;
#if defined(LOCAL_DISC) || defined(FULLDUX)
	site = ip->i_rsite;
	if (site == 0)
		site = u.u_site;
#endif 	/* LOCAL_DISC || FULLDUX */
	VN_RELE(vp);
	if (site != my_site) {
		int encoded;
#ifndef _WSIO
		if ((vp->v_flag & VMI_DEV) == 0)
			/* The logical unit within "dev" has not been mapped
			 * to a mgr index.  The serving site will have to
			 * perform this mapping before a getmp() lookup.
			 * matt 2/15/91
			 */
			encoded = DEV_LU_UNMAPPED;
		else
#endif	/* not _WSIO */
			encoded = DEV_NOT_ENCODED;
		return (send_umount_dev(dev, site, encoded));
	}
	else {
#ifndef _WSIO
		if ((vp->v_flag & VMI_DEV) != 0)
			dev = vp->v_rdev;
		else if (map_lu_to_mi(&dev, IFBLK, 0) != 0)
			return(EINVAL);	/* No such dev_t, so can't be mounted */
#else
		/*
		 * Fix for FSDdt09649.  If a umount() of /dev/root is
		 * attempted, we need to map the magic root device
		 * (NODEV) to the actual root device.  The umount
		 * will ultimately fail, but we want it to fail with
		 * the correct errno value (for XPG conformance).
		 */
		if (dev == NODEV)
			dev = rootdev;
#endif	/* not _WSIO */
		return (umount_dev(dev));
	}
}

/*
 * Unmount a file system given the root vnode
 */
int 
umount_root(vp)
struct vnode *vp;
{
	struct vfs *vfsp;
	dev_t dev;

	vfsp = vp->v_vfsp;
	/* If this is a ufs vnode, find the device and send it to the
	 * device server.  Otherwise, just do the regular unmount
	 */
	if (vp->v_fstype == VUFS)
	{
		dev = ((struct mount *)(vfsp->vfs_data))->m_dev;
		if (bdevrmt(dev))
		{
			VN_RELE(vp);
			return (send_umount_dev(dev, devsite(dev), DEV_RMT));
		}
	}
	VN_RELE(vp);
	return (umount_vfs(vfsp));
}

/*
 * Unmount the vfs associated with the (local) device dev.
 */
umount_dev(dev)
dev_t dev;
{
	struct mount *mp;

	mp = getmp(dev);
	if (mp && (mp->m_flag & MINUSE))
		return (umount_vfs(mp->m_vfsp));
	return (EINVAL);	/*not found in mount table */
}




/*
 * External routines
 */

/*
 * vfs_mountroot is called by main (init_main.c) to
 * mount the root filesystem.
 */
void
vfs_mountroot(flag)
	int flag;
{
	register int error;
	int old_lbolt = 0;
	extern int (*rootfsmount)();	/* pointer to root mounting routine */
					/* set by (auto)configuration */
	/*
	 * Rootfsmount is a pointer to the routine which will mount a specific
	 * filesystem as the root. It is setup by autoconfiguration.
	 * If error panic.
	 */
	while ((error = (*rootfsmount)(flag)) == EBUSY)
		if (old_lbolt == 0 || (lbolt - old_lbolt) / HZ > 1) {
			printf("Root device busy, retrying ...\n");
			old_lbolt = lbolt;
		}
	if (error) {
		panic("rootmount cannot mount root");
	}
	/*
	 * Get vnode for '/'.
	 * Setup rootdir, u.u_rdir and u.u_cdir to point to it.
	 * These are used by lookuppn so that it knows where
	 * to start from '/' or '.'.
	 */
	error = VFS_ROOT(rootvfs, &rootdir, "/");
	if (error)
		panic("rootmount: cannot find root vnode");
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);
	u.u_rdir = NULL;
	pipevp = rootdir;
	VN_HOLD(pipevp);

#ifndef _WSIO /* Disk mirroring only supported on s800s */
	/*
	 * Log mirror states for root and swap in root superblock.
	 * This should be done only AFTER root is mounted.
	 * If DUX is defined, only call log_mirror_info() on the
	 * root server.  Mirroring is not allowed on clients.
	 */
	if (!(rootvfs->vfs_flag & VFS_RDONLY))
		if (!(my_site_status & CCT_CLUSTERED) || (my_site == root_site))
			log_mirror_info();
#endif /* not _WSIO */
}

/*
 * vfs_add is called by a specific filesystem's mount routine to add
 * the new vfs into the vfs list and to cover the mounted on vnode.
 * The vfs is also locked so that lookuppn will not venture into the
 * covered vnodes subtree.
 * coveredvp is zero if this is the root.
 */
int
vfs_add(coveredvp, vfsp, mflag)
	register struct vnode *coveredvp;
	register struct vfs *vfsp;
	int mflag;
{
	register int error;
	register struct vfs *tvfsp;

	error = vfs_lock(vfsp);
	if(error)
		return(error);
	if (coveredvp != (struct vnode *)0) {
		/*
		 * Return EBUSY if the covered vp is already mounted on.
		 */
		if (coveredvp->v_vfsmountedhere != (struct vfs *)0) {
			vfs_unlock(vfsp);
			return(EBUSY);
		}
		/*
		 * Add the vfs to the end of the list of vfs's.  The
		 * code used to add it in the list right after rootvfs.
		 * It has been changed to add it at the end of the list
		 * instead, so that when the mount table is transmitted
		 * for DUX, it is transmitted in the same order that the
		 * mounts took place.  This guarantees that a lower file
		 * system is not transmitted before the file system on
		 * which it has been mounted.  --jdt
		 *
		 * Point the covered vnode at the new vfs so lookuppn
		 * (vfs_lookup.c) can work its way into the new file system.
		 */
		vfs_next_lock();	/* may not be necessary */
		for (tvfsp = rootvfs; tvfsp->vfs_next != NULL;
		     tvfsp = tvfsp->vfs_next)
			;
		vfsp->vfs_next = NULL;
		tvfsp->vfs_next = vfsp;
		vfs_next_unlock();
		coveredvp->v_vfsmountedhere = vfsp;
	} else {
		/*
		 * This is the root of the whole world.
		 */
		rootvfs = vfsp;
		vfsp->vfs_next = (struct vfs *)0;
	}
	vfsp->vfs_vnodecovered = coveredvp;
	if (mflag & M_RDONLY) {
		vfsp->vfs_flag |= VFS_RDONLY;
	} else {
		vfsp->vfs_flag &= ~VFS_RDONLY;
	}
	if (mflag & M_NOSUID) {
		vfsp->vfs_flag |= VFS_NOSUID;
	} else {
		vfsp->vfs_flag &= ~VFS_NOSUID;
	}
#ifdef	QUOTA
	if (mflag & M_QUOTA) {
		vfsp->vfs_flag |= VFS_QUOTA;
	} else {
		vfsp->vfs_flag &= ~VFS_QUOTA;
	}
#endif	/* QUOTA */
	return(0);
}

/*
 * Remove a vfs from the vfs list, and destory pointers to it.
 * Should be called by filesystem implementation after it determines
 * that an unmount is legal but before it destroys the vfs.
 */
void
vfs_remove(vfsp)
register struct vfs *vfsp;
{
	register struct vfs *tvfsp;
	register struct vnode *vp;

	/*
	 * can't unmount root. Should never happen, because fs will be busy.
	 */
	vfs_next_lock();
	if (vfsp == rootvfs) {
		if (rootvfs->vfs_next)
			panic("vfs_remove: unmounting root");
		rootvfs = NULL;
		vfs_unlock(vfsp);
		vfs_next_unlock();
		return;
	}
	for (tvfsp = rootvfs;
	    tvfsp != (struct vfs *)0; tvfsp = tvfsp->vfs_next) {
		if (tvfsp->vfs_next == vfsp) {
			/*
			 * remove vfs from list, unmount covered vp.
			 */
			tvfsp->vfs_next = vfsp->vfs_next;
			vfs_next_unlock();
			vp = vfsp->vfs_vnodecovered;
			vp->v_vfsmountedhere = (struct vfs *)0;
			/*
			 * release lock and wakeup anybody waiting
			 */
			vfs_unlock(vfsp);
			return;
		}
	}
	/*
	 * can't find vfs to remove
	 */
	panic("vfs_remove: vfs not found");
}

/*
 * Lock a filesystem to prevent access to it while mounting and unmounting.
 * Returns error if already locked.
 * XXX This totally inadequate for unmount right now - srk
 */
int
vfs_lock(vfsp)
	register struct vfs *vfsp;
{
	if (vfsp->vfs_flag & VFS_MLOCK)
		return(EBUSY);
	vfsp->vfs_flag |= VFS_MLOCK;
	/* Save site id for crash recovery */
	vfsp->vfs_site = u.u_site;
	return(0);
}

/*
 * Unlock a locked filesystem.
 * Panics if not locked
 */
void
vfs_unlock(vfsp)
	register struct vfs *vfsp;
{
	if ((vfsp->vfs_flag & VFS_MLOCK) == 0)
		panic("vfs_unlock");
	vfsp->vfs_flag &= ~VFS_MLOCK;
	/*
	 * Wake anybody waiting for the lock to clear
	 */
	if (vfsp->vfs_flag & VFS_MWAIT) {
		vfsp->vfs_flag &= ~VFS_MWAIT;
		wakeup((caddr_t)vfsp);
	}
}


/* 	Does this make the above redundant? The reason for not using
 *	vfs_lock() & vfs_unlock() in sync is that we don't want unmount
 *	to return EBUSY just because sync is running. missimer 880708
 */

int	vfs_chain_lock;

static void
vfs_next_lock()
{
	while (vfs_chain_lock & VFS_MLOCK){
		vfs_chain_lock |= VFS_MWAIT;
		sleep ((caddr_t)&vfs_chain_lock, PRIBIO);
	}
	vfs_chain_lock |= VFS_MLOCK;
}	

static void
vfs_next_unlock()
{
	vfs_chain_lock &= ~VFS_MLOCK;
	if (vfs_chain_lock & VFS_MWAIT){
		vfs_chain_lock &= ~VFS_MWAIT;
		wakeup ((caddr_t)&vfs_chain_lock);
	}
}	

/*
 * Find the file system pointer given its id.
 */

struct vfs *
getvfs(fsid)
	fsid_t fsid;
{
	register struct vfs *vfsp;

	for (vfsp = rootvfs; vfsp; vfsp = vfsp->vfs_next) {
		if (vfsp->vfs_fsid[0] == fsid[0] &&
		    vfsp->vfs_fsid[1] == fsid[1]) {
			break;
		}
	}
	return (vfsp);
}

/*
 * Get the device and site represented by a pathname
 */
int
getmdev(pathname, devp, sitep)
char *pathname;
dev_t *devp;
site_t *sitep;
{
	int error;
	struct vnode *vp;
	struct devandsite devandsite;
#if defined(LOCAL_DISC) || defined(FULLDUX)
	extern dev_t block_to_raw();
#endif	/* LOCAL_DISC || FULLDUX */

	error = lookupname(pathname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp, LKUP_GETMDEV, &devandsite);
	if (error == EOPCOMPLETE)
		error = 0;
	else if (error)
		return (error);
	else
	{
		if (vp->v_type != VBLK)
		{
			VN_RELE(vp);
			return (ENOTBLK);
		}
		if ( vp->v_fstype != VUFS && vp->v_fstype != VDUX ) {
			*devp = vp->v_rdev;
#ifdef	LOCAL_DISC
			*sitep = my_site;
#else
			*sitep = root_site;
#endif	/* LOCAL_DISC */
			VN_RELE(vp);
			return(0);
		}
		devandsite.dev = (VTOI(vp))->i_device;
#if defined(LOCAL_DISC) || defined(FULLDUX)
		devandsite.site = (VTOI(vp))->i_rsite;
#endif	/* LOCAL_DISC || FULLDUX */
		VN_RELE(vp);
	}
#if defined(LOCAL_DISC) || defined(FULLDUX)
	if (devandsite.dev == NODEV)
		devandsite.dev = (((VTOI(vp)->i_mode & IFMT) == IFBLK)
			? rootdev : block_to_raw(rootdev));
	if (devandsite.site == 0)
		devandsite.site = my_site;
#else
	devandsite.site = root_site;
#endif /* LOCAL_DISC || FULLDUX */
	*devp = devandsite.dev;
	*sitep = devandsite.site;
	return (0);
}

#ifdef GETMOUNT

/*
 * getmount_cnt system call: return number of mounted file systems
 * and notify last time that a change may have occurred in status
 * of mounted file systems
 */
getmount_cnt(uap)
	register struct a {
		time_t chgtime;
	} *uap;
{
	struct vfs *vfsp = rootvfs;
	int i = 0;

	PSEMA(&filesys_sema);

	while (vfsp) {
		/*
		 * Ignore any file systems that are in
		 * a transition state.
		 */
		if (!(vfsp->vfs_flag & VFS_MLOCK))
		    i++;
		vfsp=vfsp->vfs_next;
	}
	u.u_r.r_val1 = i;
	if (uap->chgtime)
		u.u_error = copyout((caddr_t)&mount_table_change_time,
				    (caddr_t)uap->chgtime,
				    sizeof(mount_table_change_time));

	VSEMA(&filesys_sema);
}

/*
 * getmount_entry system call
 */
getmount_entry(uap)
	register struct a {
		u_int	index;
		char	*fsname;
		char	*fsmntdir;
		struct	mount_data *mdp;
	} *uap;
{
	struct vfs *vfsp;
	int i;
	struct mount_data mnt;

	PSEMA(&filesys_sema);

	/*
	 * find file system corresponding to index
	 */
	for (i=0, vfsp=rootvfs;
	     vfsp && (i < uap->index);
	     vfsp->vfs_flag & VFS_MLOCK ? 0 : i++, vfsp=vfsp->vfs_next);
	/*
	 * The above "for" loop does not check the last vfsp in the chain
	 * for the VFS_MLOCK flag so we check it here.  If the VFS_MLOCK 
	 * flag is set, then this must be the last vfsp, so return NULL.
	 * mry 3/4/91
	 */
	if ((vfsp == (struct vfs *)NULL) || (vfsp->vfs_flag & VFS_MLOCK)) {
	        u.u_error = ENOENT;
		VSEMA(&filesys_sema);
		return;
	}

	/*
	 * fill in data common to all file system types here
	 */
	mnt.md_fstype = vfsp->vfs_mtype;
	mnt.md_fsopts = 0;
	if (vfsp->vfs_flag & VFS_RDONLY)
		mnt.md_fsopts |= M_RDONLY;
	if (vfsp->vfs_flag & VFS_NOSUID)
		mnt.md_fsopts |= M_NOSUID;
#ifdef	QUOTA
	if (vfsp->vfs_flag & VFS_QUOTA)
		mnt.md_fsopts |= M_QUOTA;
#endif	/* QUOTA */
	mnt.md_mnttime = vfsp->vfs_mnttime;

	/*
	 * VFS_GETMOUNT will copyout to fsmntdir
	 * and fill in fs-specific fields in mount_data structure
	 */
	u.u_error = VFS_GETMOUNT(vfsp, uap->fsmntdir, &mnt);
	u.u_error = copyoutstr(vfsp->vfs_name, uap->fsname,
			       sizeof(vfsp->vfs_name), (unsigned *)&i);
	u.u_error = copyout((caddr_t)&mnt, (caddr_t)uap->mdp, sizeof(mnt));

	VSEMA(&filesys_sema);
}

#endif /* GETMOUNT */

#ifndef _WSIO
#define D2BLKMAJ	10

/*
 * log states of root and swap mirrors in root superblock 
 */
log_mirror_info()
{
	struct mount *mp;
	struct fs *fs;
	extern dev_t rootmir;
	extern volatile struct timeval time;
	extern int (*log_mirror_state)();


	/*
	 * If type of rootdev is disc2 and root is not mirrored,
	 * clear mirror time stamp in root superblock to
	 * invalidate old state information.
	 */
	if (major(rootdev) == D2BLKMAJ && rootmir == NODEV) {
		mp = (struct mount *) (rootvfs->vfs_data);
		fs = mp->m_bufp->b_un.b_fs;
		if (fs->fs_mirror.mirtime) {
			fs->fs_mirror.mirtime = 0;
			fs->fs_fmod = 0;
			fs->fs_time = time.tv_sec;
#ifdef  AUTOCHANGER
			sbupdate(mp,1);
#else
			sbupdate(mp);
#endif  /* AUTOCHANGER */
		}
	}

	/*
	 * If mirroring is not turned on on the system, 
	 * log_mirror_state would have been initialized to 0.
	 */
	if (log_mirror_state != 0)
		(*log_mirror_state)();
}
#endif /* not _WSIO */
