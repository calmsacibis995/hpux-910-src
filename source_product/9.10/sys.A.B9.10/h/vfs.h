/* $Header: vfs.h,v 1.9.83.4 93/09/17 18:38:40 kcs Exp $ */

#ifndef _SYS_VFS_INCLUDED
#define _SYS_VFS_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/param.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#ifdef GETMOUNT
#include <sys/param.h>
#endif /* GETMOUNT */
#endif /* not _KERNEL */


/*
 * file system statistics
 */
typedef long fsid_t[2];			/* file system ID type */


/*
 * Structure per mounted file system.
 * Each mounted file system has an array of
 * operations and an instance record.
 * The file systems are put on a singly linked list.
 */
struct vfs {
	struct vfs	*vfs_next;		/* next vfs in vfs list */
	struct vfsops	*vfs_op;		/* operations on vfs */
	struct vnode	*vfs_vnodecovered;	/* vnode we mounted on */
	int		vfs_flag;		/* flags */
	int		vfs_bsize;		/* native block size */
	u_short         vfs_exroot;             /* exported fs uid 0 mapping */
	short           vfs_exflags;            /* exported fs flags */
	caddr_t		vfs_data;		/* private data */
	int		vfs_icount;		/* ref count of processes */
						/* sleeping on the mnt inode */
	short		vfs_mtype;		/* Type of vfs */
	site_t		vfs_site;		/* Site holding vfs_lock */
	fsid_t		vfs_fsid;		/* file system ID for vfs_get */
	struct log_hdrT	*vfs_logp;		/* ptr to WA log */
#ifdef GETMOUNT
	time_t		vfs_mnttime;		/* time mounted */
	char 		vfs_name[MAXPATHLEN];	/* file sys identifier */
#endif
};

/*
 * vfs flags.
 * VFS_MLOCK lock the vfs so that name lookup cannot proceed past the vfs.
 * This keeps the subtree stable during mounts and unmounts.
 */
#define VFS_RDONLY	0x01		/* read only vfs */
#define VFS_MLOCK	0x02		/* lock vfs so that subtree is stable */
#define VFS_MWAIT	0x04		/* someone is waiting for lock */
#define VFS_NOSUID      0x08            /* someone is waiting for lock */
#define VFS_EXPORTED    0x10            /* file system is exported (NFS) */
#define VFS_HARDENED    0x20            /* hardened filesystem */
#ifdef	QUOTA
#define VFS_QUOTA	0x40            /* filesystem with quotas */
#endif	/* QUOTA */
#ifdef	__hp9000s800
#define VFS_MI_DEV	0x100		/* dev_t has mgr_index in it already */
#else	/* __hp9000s800 */
#define VFS_MI_DEV	0
#endif	/* __hp9000s9800 */
/*
 * exported vfs flags.
 */
#define EX_RDONLY       0x01            /* exported read only */
#define EX_RDMOSTLY     0x02            /* exported read mostly (NFS) */
#define EX_ASYNC        0x04            /* exported -async (NFS) */

/*
 * Operations supported on virtual file system.
 */
#ifdef __cplusplus
#  define __x  ...
   extern "C" {
#else
#  define __x
#endif

struct vfsops {
	int     (*vfs_mount)(__x);
	int	(*vfs_unmount)(__x);
	int	(*vfs_root)(__x);
	int	(*vfs_statfs)(__x);
	int	(*vfs_sync)(__x);
	int	(*vfs_vget)(__x);	/* get vnode from fid */
#ifdef GETMOUNT
	int	(*vfs_getmount)(__x);	/* get mount info */
#endif /* GETMOUNT */
};

#ifdef __cplusplus
   }
#  undef __x
#endif


#define VFS_MOUNT(VFSP, PATH, DATA) \
				(*(VFSP)->vfs_op->vfs_mount)(VFSP, PATH, DATA)
#define VFS_UNMOUNT(VFSP)		(*(VFSP)->vfs_op->vfs_unmount)(VFSP)
#define VFS_ROOT(VFSP,VPP,NAME)	(*(VFSP)->vfs_op->vfs_root)(VFSP,VPP,NAME)
#define VFS_STATFS(VFSP, SBP)		(*(VFSP)->vfs_op->vfs_statfs)(VFSP,SBP)
#define VFS_SYNC(VFSP)			(*(VFSP)->vfs_op->vfs_sync)(VFSP)
/*
 * VFS_VGET -- given a pointer to a file system, and the file ID (FID),
 * lookup the vnode pointer.  Primarily used by the NFS server code to
 * translate file handles into vnodes.
 */
#define VFS_VGET(VFSP, VPP, FIDP) (*(VFSP)->vfs_op->vfs_vget)(VFSP, VPP, FIDP)
#ifdef GETMOUNT
/*
 * VFS_GETMOUNT -- retrieve mount mounted on directory and other
 * file system information.  FSMNTDIR is a user address to which
 * the mounted on directory info is copied out.
 */
#define VFS_GETMOUNT(VFSP, FSMNTDIR, MNTDATA) \
	(*(VFSP)->vfs_op->vfs_getmount)(VFSP, FSMNTDIR, MNTDATA)
#endif /* GETMOUNT */


struct statfs {
	long f_type;			/* type of info, zero for now */
	long f_bsize;			/* fundamental file system block size */
	long f_blocks;			/* total blocks in file system */
	long f_bfree;			/* free block in fs */
	long f_bavail;			/* free blocks avail to non-superuser */
	long f_files;			/* total file nodes in file system */
	long f_ffree;			/* free file nodes in fs */
	fsid_t f_fsid;			/* file system ID */
	long f_magic;			/* file system magic number */
	long f_featurebits;		/* file system features */
#ifdef LOCAL_DISC
	long f_spare[4];
	site_t f_cnode;			/* cluster node where mounted */
	short f_pad;
#else
	long f_spare[5];		/* spare for later */
#endif
};

#ifdef _KERNEL
/*
 * public operations
 */
extern void	vfs_mountroot();	/* mount the root */
extern int	vfs_add();		/* add a new vfs to mounted vfs list */
extern void	vfs_remove();		/* remove a vfs from mounted vfs list */
extern int	vfs_lock();		/* lock a vfs */
extern void	vfs_unlock();		/* unlock a vfs */


#define VFS_INIT(VFSP, OP, DATA, MTYPE)	{ \
	(VFSP)->vfs_next = (struct vfs *)0; \
	(VFSP)->vfs_op = (OP); \
	(VFSP)->vfs_flag = 0; \
	(VFSP)->vfs_exflags = 0; \
	(VFSP)->vfs_icount = 0; \
	(VFSP)->vfs_data = (DATA); \
	(VFSP)->vfs_mtype = (MTYPE); \
	(VFSP)->vfs_logp = 0; \
}

/*
 * globals
 */
extern struct vfs *rootvfs;		/* ptr to root vfs structure */

#ifdef NDCLIENT
/*see changes below */
#endif /* NDCLIENT */

/* values below same as in mount.h */
#define MOUNT_UFS   0
#define MOUNT_NFS   1
#define MOUNT_CDFS  2		/*ISO-9660 and HSG file system*/
#define MOUNT_PC    3
#define MOUNT_DCFS  4		/* data compression fs */

#define       vfssw_assign(a1, a2) \
      { \
              vfssw[a1] =    &a2; \
      }

#endif /* _KERNEL */

/*
 * File identifier. Should be unique per filesystem on a single machine.
 */
#define	MAXFIDSZ	16
#define	freefid(fidp) \
    kmem_free((caddr_t)(fidp), sizeof (struct fid) - MAXFIDSZ + (fidp)->fid_len)

struct fid {
	u_short		fid_len;		/* length of data in bytes */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
};


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
  extern "C" {
#endif

#ifdef _PROTOTYPES
# ifdef __cplusplus
    extern struct vfs *getvfs(...);		/* return vfs given fsid */
# else /* not __cplusplus */
    extern struct vfs *getvfs();		/* return vfs given fsid */
# endif /* not __cplusplus */
    extern int statfs(const char *, struct statfs *);
    extern int fstatfs(int, struct statfs *);
    extern int statfsdev(const char *, struct statfs *);
    extern int fstatfsdev(int, struct statfs *);
#else /* not _PROTOTYPES */
    extern struct vfs *getvfs();		/* return vfs given fsid */
    extern int statfs();
    extern int fstatfs();
    extern int statfsdev();
    extern int fstatfsdev();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
  }
#endif /* not __cplusplus */
#endif /* not _KERNEL */

#ifdef GETMOUNT
struct mount_data
{
	int	md_fstype;	/* type of fs (e.g. MOUNT_UFS, MOUNT_NFS */
	int	md_fsopts;	/* generic fs options: rw/ro, suid/nosuid */
	time_t	md_mnttime;	/* time of file system mount */
	site_t	md_msite;	/* cnode ID of mounting site */
	short	md_spare;	/* pad */
	dev_t	md_dev;		/* encoded major/minor dev */
	union {
		struct {
			dev_t	mdu_rdev;	/*  real major/minor dev */
			int	mdu_ufsopts;	/* ufs flags/options */
		} md_ufs;
		struct {
			int	mdn_nfsopts;	/* nfs mount options */
			int	mdn_retrans;	/* # retries */
			int	mdn_timeo;	/* timeout for retry */
			int	mdn_rsize;	/* read size */
			int	mdn_wsize;	/* write size */
			u_long	mdn_port;	/* IP port number */
			int	mdn_acregmin;	/* min secs cache file attr */
			int	mdn_acregmax;	/* max secs cache file attr */
			int	mdn_acdirmin;	/* min secs cache dir attr */
			int	mdn_acdirmax;	/* max secs cache dir attr */
		} md_nfs;
	} md_un;
};

#define md_rdev		md_un.md_ufs.mdu_rdev
#define md_ufsopts 	md_un.md_ufs.mdu_ufsopts
#define md_nfsopts 	md_un.md_nfs.mdn_nfsopts
#define md_retrans 	md_un.md_nfs.mdn_retrans
#define md_timeo	md_un.md_nfs.mdn_timeo
#define md_rsize	md_un.md_nfs.mdn_rsize
#define md_wsize	md_un.md_nfs.mdn_wsize
#define md_port		md_un.md_nfs.mdn_port
#define md_acregmin	md_un.md_nfs.mdn_acregmin
#define md_acregmax	md_un.md_nfs.mdn_acregmax
#define md_acdirmin	md_un.md_nfs.mdn_acdirmin
#define md_acdirmax	md_un.md_nfs.mdn_acdirmax
#endif /* GETMOUNT */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_VFS_INCLUDED */
