#ifdef MODULE_ID
/*
 * @(#)nfs_clnt.h: $Revision: 1.9.83.4 $ $Date: 93/09/17 19:07:45 $
 * $Locker:  $
 */
/* nfs_clnt.h: %I%	[%E%  %U%] */
#endif /* MODULE_ID */
/*
 * REVISION: @(#)10.1
 */

/*
 * (c) Copyright 1987 Hewlett-Packard Company
 * (c) Copyright 1984 Sun Microsystems, Inc.
 */

/*
 * vfs pointer to mount info
 */
#define	vftomi(vfsp)	((struct mntinfo *)((vfsp)->vfs_data))

/*
 * vnode pointer to mount info
 */
#define	vtomi(vp)	((struct mntinfo *)(((vp)->v_vfsp)->vfs_data))

/*
 * NFS vnode to server's block size
 */
#define	vtoblksz(vp)	(vtomi(vp)->mi_bsize)

#define		HOSTNAMESZ      32
#define ACREGMIN        3       /* min secs to hold cached file attr */
#define ACREGMAX        60      /* max secs to hold cached file attr */
#define ACDIRMIN        30      /* min secs to hold cached dir attr */
#define ACDIRMAX        60      /* max secs to hold cached dir attr */
#define ACMINMAX        3600    /* 1 hr is longest min timeout */
#define ACMAXMAX        36000   /* 10 hr is longest max timeout */

#ifdef GETMOUNT
#ifndef MAXMNTLEN
#define MAXMNTLEN 512	/* keep this consistent with fs.h/cdfs.h/filsys.h */
#endif
#endif /* GETMOUNT */

/*
 * NFS private data per mounted file system
 */
struct mntinfo {
	struct sockaddr_in mi_addr;	/* server's address */
	struct vnode	*mi_rootvp;	/* root vnode */
        u_int            mi_hard : 1;   /* hard or soft mount */
        u_int            mi_printed : 1;/* not responding message printed */
	u_int		 mi_int : 1;    /* interrupts allowed on hard mount */
	u_int		 mi_down : 1;   /* server is down */
	u_int		 mi_devs : 1; 	/* support device files */
	u_int		 mi_ignore : 1;	/* mark as ignore, for automounter */
	u_int		 mi_noac : 1;	/* no attribute or name caching */
	u_int		 mi_nocto : 1;	/* no fresh attributes on file open */
	u_int		 mi_dynamic : 1;/* dynamically modify read/write sizes*/
	int		 mi_refct;	/* active vnodes for this vfs */
	long		 mi_tsize;	/* transfer size (bytes) */
	long		 mi_stsize;	/* server's max transfer size (bytes) */
	long		 mi_bsize;	/* server's disk block size */
	int		 mi_mntno;	/* kludge to set client rdev for stat*/
        int              mi_timeo;      /* inital timeout in 10th sec */
        int              mi_retrans;    /* times to retry request */
        char             mi_hostname[HOSTNAMESZ];       /* server name */
#ifdef GETMOUNT
	char	         mi_fsmnt[MAXMNTLEN]; 	/* mounted on dir */
#endif
	struct mntinfo  *mi_next;	/* pointer to linked hash list */
	struct vfs	*mi_vfs;	/* pointer back to vfs */
	long		 mi_curread;	/* current read size (bytes) */
	long		 mi_curwrite;	/* current write size (bytes) */
        u_int            mi_acregmin;   /* min secs to hold cached file attr */
        u_int            mi_acregmax;   /* max secs to hold cached file attr */
        u_int            mi_acdirmin;   /* min secs to hold cached dir attr */
        u_int            mi_acdirmax;   /* max secs to hold cached dir attr */
};

/*
 * enum to specifiy cache flushing action when file data is stale
 */
enum staleflush	{NOFLUSH, SFLUSH};

#define	MNTHASHSIZE 16	/*# of hash entries for mntinfos*/

struct mntinfo *mntinfohash[MNTHASHSIZE];
#define MNTINFOHASH(mntno) \
	mntinfohash[mntno%MNTHASHSIZE]
