/*
 * @(#)dux_lookup.h: $Revision: 1.5.83.3 $ $Date: 93/09/17 16:41:36 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */
#include "../rpc/types.h"
#include "../netinet/in.h"
#include "../nfs/nfs.h"
#include "../dux/duxparam.h"
#include "../cdfs/cdfsdir.h"
#include "../cdfs/cdnode.h"

/*
 *structures and declarations for Lookup operations
 */
/*
 *WARNING:  All structures tagged with "DUX MESSAGE STRUCTURE" are passed
 *between machines.  They must obey the following rules:
 *	1)  All integers (and larger) must be 32 bit aligned
 *	2)  They must be consistant between all versions of DUX in the
 *	cluster.
 */

/*
 *The u_information
 */
struct packed_ucred			/*DUX MESSAGE STRUCTURE*/
{
	u_short	puc_uid;
	u_short	puc_gid;
	int	puc_groups[NGROUPS];
	u_short	puc_ruid;
	u_short	puc_rgid;
};

/*PAD pads character fields to 4 byte sizes*/
#define PAD(size) (((size)+3)/4*4)
/*u area information needed for lookup requests*/
struct lk_usave				/*DUX MESSAGE STRUCTURE*/
{
	struct	packed_ucred	lk_ucred;
	struct	rlimit	lk_rlimit[RLIM_NLIMITS];
	short	lk_cmask;		/* file creation mask */
	short	lk_pad;			/* pad to 32 bits */
	struct	dux_context lk_context;	/* CDF context info */
};
#undef	PAD

/* all the info passed back when a "remote" inode is to be returned in */
/*  a reply message.  this is filled in by a ss and received by a us. */

struct remino			/*DUX MESSAGE STRUCTURE*/
{
	dev_t	dev;
	ino_t	ino;
	site_t	site;
	site_t	rsite;
	u_short mode;
	short	nlink;
	ushort	uid;
	ushort	gid;
	long	size;
	long	fversion;
	dev_t	rdev;
	struct	ic_fifo fifo;
	long	blocks;
	ushort	vflag;
#define NO_DQUOT 0
#define IS_DQUOT 1
        ushort  is_dquot;
};

/* all the info passed back when a "remote" cdnode is to be returned in */
/*  a reply message.  this is filled in by a ss and received by a us. */

struct remcdno			/*DUX MESSAGE STRUCTURE*/
{
	dev_t	dev;
	cdno_t	cdno;
	cdno_t	cdpno;
	site_t	site;
	site_t	rsite;
	u_short mode;
	short	nlink;
	ushort	uid;
	ushort	gid;
	long	size;
/*	dev_t	rdev;	*/
/*	long	blocks; */
	u_int	loc;
	u_int	fusize;
	u_int	fugsize;
	u_int	xarlen;
	ushort	vflag;
	u_short	ftype;
};
struct remvno			/*DUX MESSAGE STRUCTURE*/
{
	int	rv_fstype;
	union {
		struct remino ino;
		struct remcdno cdno;
	} rv_un;
};	
#define	rv_ino rv_un.ino
#define	rv_cdno rv_un.cdno
/*basic structure for all lookup request messages*/
struct dux_lookup_request	/*DUX MESSAGE STRUCTURE*/
{
	int	lk_opcode;		/*lookup opcode*/
	int	lk_flags;		/*see below*/
	dev_t	lk_dev;			/*starting device number*/
	int	lk_fstype;
	union {
		ino_t	lk_inode;		/*starting inode*/
		cdno_t	lk_cdnum;		/*starting cdnode*/
	} un1;
	int	lk_nlink;		/*sym links traversed so far*/
	dev_t	lk_rdev;		/* root device (or NODEV) */
	int	lk_rfstype;
	union {
		ino_t	lk_rinode;		/* root inode */
		cdno_t lk_rcdnum;
	} un2;
	struct	lk_usave lk_usave;	/* u area save information */
	struct  remvno	lk_dirvp;	/*only used when LK_MNT is set*/
};
#define lk_ino un1.lk_inode
#define lk_cdno un1.lk_cdnum
#define lk_rino un2.lk_rinode
#define lk_rcdno un2.lk_rcdnum

/* flags for lookup */
#define	LK_FOLLOWLINK	1	/*follow symbolic links*/
#define LK_DIR		2	/*must lookup parent directory*/
#define LK_COMP		4	/*must lookup component itself*/
#define LK_MNT		8	/*last component is a mount point*/

struct dux_lookup_reply		/*DUX MESSAGE STRUCTURE*/
{
	int	lkr_vnodetype;	/*MOUNT_UFS, MOUNT_NFS, ...*/
	union
	{
		struct
		{
			dev_t	lkru_dev;	/*device to continue with*/
			ino_t	lkru_ino;	/*inode to continue with*/
		} lkr_is_ufs;
		struct
		{
			dev_t	lkrcd_dev;	/*device to continue with*/
			cdno_t	lkrcd_cdno;/*inode to continue with*/
		} lkr_is_cdfs;
		struct 
		{
			int	lkrn_mntno;	/*nfs mount number*/
 			fhandle_t lkrn_fhandle;	/*nfs file handle*/
		} lkr_is_nfs;
	}lkr_un;
	int	lkr_nlink;	/*sym links traversed so far*/
	struct remvno	lkr_dirvp;
	int	lkr_mntdvp_flg;
};

#define lkr_dev		lkr_un.lkr_is_ufs.lkru_dev
#define lkr_ino		lkr_un.lkr_is_ufs.lkru_ino
#define lkr_cddev	lkr_un.lkr_is_cdfs.lkrcd_dev
#define lkr_cdno	lkr_un.lkr_is_cdfs.lkrcd_cdno
#define lkr_mntno	lkr_un.lkr_is_nfs.lkrn_mntno
#define lkr_fhandle	lkr_un.lkr_is_nfs.lkrn_fhandle



/* This structure uniquely identifies a VFS */
struct vfsid
{
	int fstype;	/*MOUNT_UFS, MOUNT_NFS,...*/
	union
	{
/*KFS and UFS share the same structure*/
		struct 
		{
			dev_t dev_index;	/*index of device #*/
			unsigned int dev_site;	/*site of device*/
		} isufs;
		int nfs_mntno;			/*NFS mount ID*/
	}un;
};
