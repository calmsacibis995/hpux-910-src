/* $Header: vnode.h,v 1.9.83.5 94/03/09 15:44:03 dkm Exp $ */

#ifndef _SYS_VNODE_INCLUDED
#define _SYS_VNODE_INCLUDED

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/sema.h"
#include "../h/time.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/sema.h>
#include <sys/time.h>
#endif /* _KERNEL_BUILD */

/*
 * The vnode is the focus of all file activity in UNIX.
 * There is a unique vnode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 */

/*
 * vnode types. VNON means no type.
 */
enum vtype 	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VBAD, VFIFO, VFNWK, VEMPTYDIR};
enum vfstype	{ VDUMMY, VNFS, VUFS, VDUX, VDUX_PV, VDEV_VN, VNFS_SPEC,
			VNFS_BDEV, VNFS_FIFO, VCDFS, VDUX_CDFS, VDUX_CDFS_PV };

struct vnode {
	u_short		v_flag;			/* vnode flags (see below)*/
	u_short		v_shlockc;		/* count of shared locks */
	u_short		v_exlockc;		/* count of exclusive locks */
	u_short		v_tcount;		/* private data for fs */
	int		v_count;		/* reference count */
	struct vfs	*v_vfsmountedhere; 	/* ptr to vfs mounted here */
	struct vnodeops	*v_op;			/* vnode operations */
	struct socket	*v_socket;		/* unix ipc */
	struct vfs	*v_vfsp;		/* ptr to vfs we are in */
	enum vtype	v_type;			/* vnode type */
	dev_t		v_rdev;			/* device (VCHR, VBLK) */
	caddr_t		v_data;			/* private data for fs */
	enum vfstype	v_fstype;		/* file system type*/
	struct vas	*v_vas;			/* vm data structures */
	vm_sema_t 	v_lock;			/* vnode lock */
	struct buf      *v_ord_lastdatalink;	/* for ordered writes */
	struct buf      *v_ord_lastmetalink;	/* for ordered writes */
	struct buf      *v_cleanblkhd;          /* clean buffer head */
	struct buf      *v_dirtyblkhd;          /* dirty buffer head */
#ifdef OSDEBUG
	short		dnlc_count;		/* Count for name cache */
	short		pad;
#endif
};

/*
 * vnode flags.
 */
#define	VROOT		0x01	/* root of its file system */
#define VTEXT		0x02	/* vnode is a pure text prototype */
#define VTRACE		0x04	/* trace pathname lookups of this file  */
#define	VEXLOCK		0x10	/* exclusive lock */
#define	VSHLOCK		0x20	/* shared lock */
#define	VLWAIT		0x40	/* proc is waiting on shared or excl. lock */
#define VMMF		0x100	/* Vnode memory mapped */
#ifdef  __hp9000s800
#define VMI_DEV		0x200	/* v_rdev has mgr_index in it */
#else   /* __hp9000s800 */
#define VMI_DEV		0
#endif	/* __hp9000s800 */


/*
 * Operations on vnodes.
 */
#ifdef __cplusplus
    extern "C" {
#   define __farg    ...	/* arguments to vnode op functions */
#else /* not __cplusplus */
#   define __farg
#endif /* not __cplusplus */

      struct vnodeops {
	int	(*vn_open)(__farg);
	int	(*vn_close)(__farg);
	int	(*vn_rdwr)(__farg);
	int	(*vn_ioctl)(__farg);
	int	(*vn_select)(__farg);
	int	(*vn_getattr)(__farg);
	int	(*vn_setattr)(__farg);
	int	(*vn_access)(__farg);
	int	(*vn_lookup)(__farg);
	int	(*vn_create)(__farg);
	int	(*vn_remove)(__farg);
	int	(*vn_link)(__farg);
	int	(*vn_rename)(__farg);
	int	(*vn_mkdir)(__farg);
	int	(*vn_rmdir)(__farg);
	int	(*vn_readdir)(__farg);
	int	(*vn_symlink)(__farg);
	int	(*vn_readlink)(__farg);
	int	(*vn_fsync)(__farg);
	int	(*vn_inactive)(__farg);
	int	(*vn_bmap)(__farg);
	int	(*vn_strategy)(__farg);
	int	(*vn_bread)(__farg);
	int	(*vn_brelse)(__farg);
	int	(*vn_pathsend)(__farg);
	int	(*vn_setacl)(__farg);
	int	(*vn_getacl)(__farg);
	int	(*vn_pathconf)(__farg);
	int	(*vn_fpathconf)(__farg);
	/*
	 * Add VOPs for support NFS 3.2 file locking.  See below for more info.
	 */
        int     (*vn_lockctl)(__farg);
	int	(*vn_lockf)(__farg);
	int	(*vn_fid)(__farg);
	int	(*vn_fsctl)(__farg);
	int	(*vn_prefill)(__farg);
	int	(*vn_pagein)(__farg);
	int	(*vn_pageout)(__farg);
	int	(*vn_dbddup)(__farg);
	int	(*vn_dbddealloc)(__farg);
     };

#ifdef __cplusplus
   }
#  undef __farg
#endif /* __cplusplus */


#ifdef _KERNEL

#define VOP_OPEN(VPP,F,C)		(*(*(VPP))->v_op->vn_open)(VPP, F, C)
#define VOP_CLOSE(VP,F,C)		(*(VP)->v_op->vn_close)(VP,F,C)
#define VOP_RDWR(VP,UIOP,RW,F,C)	(*(VP)->v_op->vn_rdwr)(VP,UIOP,RW,F,C)
#define VOP_IOCTL(VP,C,D,F,CR)		(*(VP)->v_op->vn_ioctl)(VP,C,D,F,CR)
#define VOP_SELECT(VP,W,C)		(*(VP)->v_op->vn_select)(VP,W,C)
/*An additional parameter specifying synchronization has been added to getattr*/
#define VOP_GETATTR(VP,VA,C,S)		(*(VP)->v_op->vn_getattr)(VP,VA,C,S)
#define VOP_SETATTR(VP,VA,C,N)		(*(VP)->v_op->vn_setattr)(VP,VA,C,N)
#define VOP_ACCESS(VP,M,C)		(*(VP)->v_op->vn_access)(VP,M,C)
#define VOP_LOOKUP(VP,NM,VPP,C,MVP)		(*(VP)->v_op->vn_lookup)(VP,NM,VPP,C,MVP)
#define VOP_CREATE(VP,NM,VA,E,M,VPP,C)	(*(VP)->v_op->vn_create) \
						(VP,NM,VA,E,M,VPP,C)
#define VOP_REMOVE(VP,NM,C)		(*(VP)->v_op->vn_remove)(VP,NM,C)
#define VOP_LINK(VP,TDVP,TNM,C)		(*(VP)->v_op->vn_link)(VP,TDVP,TNM,C)
#define VOP_RENAME(VP,NM,TDVP,TNM,C)	(*(VP)->v_op->vn_rename) \
						(VP,NM,TDVP,TNM,C)
#define VOP_MKDIR(VP,NM,VA,VPP,C)	(*(VP)->v_op->vn_mkdir)(VP,NM,VA,VPP,C)
#define VOP_RMDIR(VP,NM,C)		(*(VP)->v_op->vn_rmdir)(VP,NM,C)
#define VOP_READDIR(VP,UIOP,C)		(*(VP)->v_op->vn_readdir)(VP,UIOP,C)
#define VOP_SYMLINK(VP,LNM,VA,TNM,C)	(*(VP)->v_op->vn_symlink) \
						(VP,LNM,VA,TNM,C)
#define VOP_READLINK(VP,UIOP,C)		(*(VP)->v_op->vn_readlink)(VP,UIOP,C)
#define VOP_FSYNC(VP,C, S)			(*(VP)->v_op->vn_fsync)(VP,C, S)
#define VOP_INACTIVE(VP,C)		(*(VP)->v_op->vn_inactive)(VP,C)
#define VOP_BMAP(VP,BN,VPP,BNP)		(*(VP)->v_op->vn_bmap)(VP,BN,VPP,BNP)
#define VOP_STRATEGY(BP)		(*(BP)->b_vp->v_op->vn_strategy)(BP)
#define VOP_BREAD(VP,BN,BPP)		(*(VP)->v_op->vn_bread)(VP,BN,BPP)
#define VOP_BRELSE(VP,BP)		(*(VP)->v_op->vn_brelse)(VP,BP)
#define VOP_PATHSEND(VPP,PNP,FOLLOW,NLINKP,DIRVPP,COMPVPP,OPCODE,DEPENDENT) \
	((*(*(VPP))->v_op->vn_pathsend) ? \
	(*(*(VPP))->v_op->vn_pathsend) \
		(VPP,PNP,FOLLOW,NLINKP,DIRVPP,COMPVPP,OPCODE,DEPENDENT) : \
		(panic("VOP_PATHSEND"),EINVAL))
#define VOP_SETACL(VP,NT,BP)		(*(VP)->v_op->vn_setacl)(VP,NT,BP)
#define VOP_GETACL(VP,NT,BP)		(*(VP)->v_op->vn_getacl)(VP,NT,BP)
#define VOP_PATHCONF(VP,NT,BP,CR)	(*(VP)->v_op->vn_pathconf)(VP,NT,BP,CR)
#define VOP_FPATHCONF(VP,NT,BP,CR)	(*(VP)->v_op->vn_fpathconf)(VP,NT,BP,CR)

/*
 * VOPs for NFS 3.2 file locking.  Ours are different because we support
 * local file locking already in the kernel.  VOP_LOCKCTL() is called from
 * fcntl() to process a lock request.  We have an extra parameters because
 * the lower level routines will need the file structure for the file
 * being locked.  The Lower Bound and Upper Bound are passed in because the
 * higher level routine already computed them for error checking.  This means
 * that ALL functions calling these routines MUST include reasonable values
 * for LB and UB.   Also, Sun does not have a VOP_LOCKF() because they
 * emulate lockf() as a library on top of fcntl(), instead of two separate
 * system calls like ours.
 */
#define VOP_LOCKCTL(VP,LD,CMD,C,FP,LB,UB)  (*(VP)->v_op->vn_lockctl) \
                                              (VP,LD,CMD,C,FP,LB,UB)
#define VOP_LOCKF(VP,CMD,SIZE,C,FP,LB,UB)	 (*(VP)->v_op->vn_lockf) \
					      (VP,CMD,SIZE,C,FP,LB,UB)
/*
 * Support for NFS 3.2 file handles.  Given a vnode pointer, generate
 * a "file id" which can be used to recreate the vnode later on.
 */
#define VOP_FID(VP, FIDPP)		(*(VP)->v_op->vn_fid)(VP, FIDPP)
#define VOP_FSCTL(VP, COMMAND, UIOP, CRED) (*(VP)->v_op->vn_fsctl) \
						(VP, COMMAND, UIOP, CRED)
#define VOP_PREFILL(VP,PRP)	(*(VP)->v_op->vn_prefill)(PRP)
#define VOP_DBDDUP(VP,DBD) 	(*(VP)->v_op->vn_dbddup)(VP, DBD)
#define VOP_DBDDEALLOC(VP,DBD) \
	(((VP)->v_op->vn_dbddealloc)?(*(VP)->v_op->vn_dbddealloc)(VP,DBD):1)
#define VOP_PAGEOUT(VP,PRP,START,END,FLAGS) \
	(*(VP)->v_op->vn_pageout)(PRP,START,END,FLAGS)

#define VOP_PAGEIN(VP,PRP,WRT,SPACE,VADDR,START) \
	(*(VP)->v_op->vn_pagein)(PRP,WRT,SPACE,VADDR,START)

/*
 * flags for above
 */
#define IO_UNIT		0x01		/* do io as atomic unit for VOP_RDWR */
#define IO_APPEND	0x02		/* append write for VOP_RDWR */
#define IO_SYNC		0x04		/* sync io for VOP_RDWR */

#endif /* _KERNEL */

/*
 * Vnode attributes.  A field value of -1
 * represents a field whose value is unavailable
 * (getattr) or which is not to be changed (setattr).
 */
/*DUX MESSAGE STRUCTURE*/
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	u_short		va_uid;		/* owner user id */
	u_short		va_gid;		/* owner group id */
	/*moved va_nlink for alignment*/
	short		va_nlink;	/* number of references to file */
	long		va_fsid;	/* file system id (dev for now) */
	long		va_nodeid;	/* node id */
	u_long		va_size;	/* file size in bytes (quad?) */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct timeval	va_atime;	/* time of last access */
	struct timeval	va_mtime;	/* time of last modification */
	struct timeval	va_ctime;	/* time file ``created */
	dev_t		va_rdev;	/* device the file represents */
	long		va_blocks;	/* kbytes of disk space held by file */
	site_t		va_rsite;	/* site the device file represents */
	site_t		va_fssite;	/* file system site (dev site ) */
	dev_t		va_realdev;	/* The real devcie number of device
					   containing the inode for this file */
	u_short		va_basemode;	/* the base mode bits unaltered */
	u_short		va_acl:1,	/* set if optional ACL entries */
			va_fstype:3,
			:12;
};

/*
 *  Modes. Some values same as Ixxx entries from inode.h for now
 */
#define	VSUID	04000		/* set user id on execution */
#define	VSGID	02000		/* set group id on execution */
#define VSVTX	01000		/* save swapped text even after use */
#define	VREAD	0400		/* read, write, execute permissions */
#define	VWRITE	0200
#define	VEXEC	0100


#ifdef _KERNEL

/*
 *  Macros to lock and unlock the vnode.
 */

#define vnodelock(VP)	 vm_psema(&(VP)->v_lock, PZERO)
#define vnodeunlock(VP)  {VASSERT(vm_valusema(&(VP)->v_lock) <= 0); \
			  vm_vsema(&(VP)->v_lock, 0);}


/*
 * public vnode manipulation functions
 */
extern int vn_open();			/* open vnode */
extern int vn_create();			/* creat/mkdir vnode */
extern int vn_rdwr();			/* read or write vnode */
extern int vn_close();			/* close vnode */
extern void vn_rele();			/* release vnode */
extern int vn_link();			/* make hard link */
extern int vn_rename();			/* rename (move) */
extern int vn_remove();			/* remove/rmdir */
extern void vattr_null();		/* set attributes to null */
extern int getvnodefp();		/* get fp from vnode fd */
extern void unmapvnode();		/* unmap a vnode */

#ifdef OSDEBUG
#define VN_HOLD(VP)	{ \
	vn_hold(VP); \
}
#else
#define VN_HOLD(VP)	{ \
	(VP)->v_count++; \
}
#endif

#define VN_RELE(VP)	{ \
	vn_rele(VP); \
}

#ifdef OSDEBUG
#define DNLC_INCR(vp)		(vp)->dnlc_count++;
#define DNLC_DECR(vp)		(vp)->dnlc_count--;
#define INIT_DNLC_COUNT(vp)	(vp)->dnlc_count = 0;
#else
#define DNLC_INCR(vp)
#define DNLC_DECR(vp)
#define INIT_DNLC_COUNT(vp)
#endif

#define VN_INIT(VP, VFSP, TYPE, DEV)	{ \
	(VP)->v_flag = 0; \
	(VP)->v_count = 1; \
	(VP)->v_shlockc = (VP)->v_exlockc = 0; \
	(VP)->v_vfsp = (VFSP); \
	(VP)->v_type = (TYPE); \
	(VP)->v_rdev = (DEV); \
	(VP)->v_ord_lastdatalink = NULL; \
	(VP)->v_ord_lastmetalink = NULL; \
	(VP)->v_cleanblkhd = NULL; \
	(VP)->v_dirtyblkhd = NULL; \
	vm_initsema(&(VP)->v_lock, 1, VNODE_LOCK_ORDER, "vnode sema"); \
	INIT_DNLC_COUNT(VP); \
}

#ifdef MP
#ifndef _SPINLOCK_INCLUDED
#include "../h/spinlock.h"
#endif /* not _SPINLOCK_INCLUDED */
extern lock_t *v_count_lock;
#undef VN_HOLD
#undef VN_INIT

#ifdef OSDEBUG
#define VN_HOLD(VP)	{ \
	SPINLOCK(v_count_lock); \
	vn_hold(VP); \
	SPINUNLOCK(v_count_lock); \
}
#else
#define VN_HOLD(VP)	{ \
	SPINLOCK(v_count_lock); \
	(VP)->v_count++; \
	SPINUNLOCK(v_count_lock); \
}
#endif

#define VN_INIT(VP, VFSP, TYPE, DEV)	{ \
	(VP)->v_flag = 0; \
	SPINLOCK(v_count_lock); \
	(VP)->v_count = 1; \
	SPINUNLOCK(v_count_lock); \
	(VP)->v_shlockc = (VP)->v_exlockc = 0; \
	(VP)->v_vfsp = (VFSP); \
	(VP)->v_type = (TYPE); \
	(VP)->v_rdev = (DEV); \
	(VP)->v_ord_lastdatalink = NULL; \
	(VP)->v_ord_lastmetalink = NULL; \
	(VP)->v_cleanblkhd = NULL; \
	(VP)->v_dirtyblkhd = NULL; \
	vm_initsema(&(VP)->v_lock, 1, VNODE_LOCK_ORDER, "vnode sema"); \
	INIT_DNLC_COUNT(VP); \
}
#endif /* MP */


/*
 * flags for above
 */
enum rm		{ FILE, DIRECTORY };		/* rmdir or rm (remove) */
enum symfollow	{ NO_FOLLOW, FOLLOW_LINK };	/* follow symlinks (lookuppn) */
enum vcexcl	{ NONEXCL, EXCL};		/* (non)excl create (create) */

enum vsync	{ VASYNC, VIFSYNC, VSYNC };	/* whether to synchconize (and
						 * go to server) on getattr.
						 * VASYNC = no
						 * VIFSYNC = go to server if
						 *    inode synchronous
						 * VSYNC = yes
						 */
/*
 * Global vnode data.
 */
extern struct vnode	*rootdir;		/* root (i.e. "/") vnode */

#endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_VNODE_INCLUDED */
