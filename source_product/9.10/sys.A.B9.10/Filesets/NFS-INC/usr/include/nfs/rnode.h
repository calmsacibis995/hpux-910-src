#ifdef MODULE_ID
/*
 * @(#)rnode.h: $Revision: 1.8.83.5 $ $Date: 93/11/05 15:28:58 $
 * $Locker:  $
 */
#endif /* MODULE_ID */
/*
 * REVISION: @(#)10.7
 */

/*
 * (c) Copyright 1987 Hewlett-Packard Company
 * (c) Copyright 1984 Sun Microsystems, Inc.
 */

/*
 * Remote file information structure.
 * The rnode is the "inode" for remote files.  It contains
 * all the information necessary to handle remote file on the
 * client side.
 */

/*
 * Change for HP-UX 9.03:
 *
 * Broke field r_cred into two unique fields, r_rcred and r_wcred
 * to avoid READs over-writing WRITE credentials and causing EACCES
 * errors on delayed WRITEs.
 */

struct rnode {
	struct rnode	*r_next;	/* active rnode list */
	struct vnode	r_vnode;	/* vnode for remote file */
	fhandle_t	r_fh;		/* file handle */
	u_short		r_flags;	/* flags, see below */
	short		r_error;	/* async write error */
	daddr_t		r_lastr;	/* last block read (read-ahead) */
	u_long		r_size;		/* file size in bytes */
	struct ucred	*r_rcred;	/* current READ credentials */
	struct ucred	*r_wcred;	/* current WRITE credentials */
	struct ucred	*r_unlcred;	/* unlinked credentials */
	char		*r_unlname;	/* unlinked file name */
	struct vnode	*r_unldvp;	/* parent dir of unlinked file */
	struct nfsfattr	r_nfsattr;	/* cached nfs attributes */
	struct timeval	r_nfsattrtime;	/* time attributed cached */
	struct ucred 	*r_nfsattrcred; /* Cred of last process who got attrs*/
					/* See nfs_getattr(),nfs_attrcache() */
        short           r_owner;        /* proc index for locker of rnode */
        short           r_count;        /* number of rnode locks for r_owner */

};

/*
 * Flags
 */
#define	RLOCKED		0x01		/* rnode is in use */
#define	RWANT		0x02		/* someone wants a wakeup */
#define	RATTRVALID	0x04		/* Attributes in the rnode are valid */
#define	REOF		0x08		/* EOF encountered on read */
#define	RDIRTY		0x10		/* dirty buffers may be in buf cache */
#define ROPEN		0x20		/* the vnode is currently open */
#define RNOCACHE        0x40            /* don't cache read and write blocks */

/*
 * Convert between vnode and rnode
 */
#define	rtov(rp)	(&(rp)->r_vnode)
#define	vtor(vp)	((struct rnode *)((vp)->v_data))
#define	vtofh(vp)	(&(vtor(vp)->r_fh))
#define	rtofh(rp)	(&(rp)->r_fh)

/*
 * Lock and unlock rnodes
 * HPNFS
 * A change was made to the rlock definition.   It was picked up from the
 * NFS4.0 code.  The change allows multiple locks to be done on a rnode
 * IF it is the same process doing the rlocks.  This is necessary because
 * there are pathways through the nfs_vnops code that would encounter
 * multiple rlocks.  Also a count was added to the rnode that will prevent
 * a wakeup() from being done until all of the multiple locks are unlocked.
 */
#define rlock(rp) { \
        while ( ((rp)->r_flags & RLOCKED) && \
	       (rp)->r_owner != pindx(u.u_procp)) { \
	       (rp)->r_flags |= RWANT; \
                sleep((caddr_t)(rp), PINOD); \
        } \
        (rp)->r_flags |= RLOCKED; \
	(rp)->r_owner = pindx(u.u_procp); \
	(rp)->r_count++; \
}

#define runlock(rp) { \
	if (--(rp)->r_count == 0) { \
		(rp)->r_flags &= ~RLOCKED; \
        	if ((rp)->r_flags&RWANT) { \
                	(rp)->r_flags &= ~RWANT; \
                	wakeup((caddr_t)(rp)); \
		} \
	} \
}

