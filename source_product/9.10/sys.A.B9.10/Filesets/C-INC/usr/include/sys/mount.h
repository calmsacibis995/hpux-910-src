/* $Header: mount.h,v 1.17.83.4 93/09/17 18:30:10 kcs Exp $ */

#ifndef _SYS_MOUNT_INCLUDED
#define _SYS_MOUNT_INCLUDED

/*
 * mount.h: Definitions for mounting file systems and the mount table
 *
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */


#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#  include "../dux/sitemap.h"
#else /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#  include <sys/sitemap.h>
#endif /* ! _KERNEL_BUILD */


/* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
       extern int mount(const char *, const char *, int);
       extern int umount(const char *);
       extern int vfsmount(int, const char *, int, caddr_t);
#  else /* not _PROTOTYPES */
       extern int mount();
       extern int umount();
       extern int vfsmount();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */

   /*
    * Information to be used with vfsmount()
    */

   /* mount flags */
#  define M_RDONLY	0x01		/* mount fs read only */
#  define M_NOSUID	0x02		/* mount fs with setuid not allowed */
#    define M_QUOTA	0x04		/* mount fs with quotas enabled */

   /* File system types, these correspond to entries in fsconf */
#  define MOUNT_UFS		0
#  define MOUNT_NFS		1
#  define MOUNT_CDFS		2
#  define MOUNT_PC		3
#  define MOUNT_DCFS		4	/* data compression fs */

/* maximum number of file systems allowed -- >= the MOUNT_* values above */
#  define MOUNT_MAXTYPE	10

   struct ufs_args {
	   char	*fspec;
   };

   struct cdfs_args {
	   char	*fspec;
   /*	char	*vol_id;	for multi-vloume
	   int	unused[4];
   */
   };

#  ifdef PCFS
       struct pc_args {
	       char	*fspec;
       };
#  endif /* PCFS */


#  ifdef _KERNEL
   /*
    * Mount structure.
    * One allocated on every mount.
    * Used to find the super block.
    */
   /* The mount table was changed from a fixed size array to a dynamically
    * allocated set of structures linked on hash chains.  This was done
    * to increase the number of file systems that can be concurrently
    * mounted without using up lots of fixed storage.
    */

   struct mount
   {
	/* the hash pointers must be the first elements in the structure */
	struct	mount *m_hforw;  /* forward hash pointer */
	struct	mount *m_hback;  /* backward hash pointer */
	struct	mount *m_rhforw; /* forward hash pointer for real device */
	struct	mount *m_rhback; /* backward hash pointer for real device */
	struct vfs	*m_vfsp; /* vfs structure for this filesystem */
	dev_t	m_dev;		 /* device mounted */
	struct buf *m_bufp;	 /* buffer for super block */
	struct	duxfs	*m_dfs;	 /* diskless client superblock info */
	struct	sitemap m_dwrites; /* sites with device open for file writes */
	int	m_maxbufs;	/* maximum free space before sync I/O needed */
	dev_t   m_rdev;         /* The real device number and site      */
	site_t  m_site;
	union mnodes_u {
		struct inode *m_inodp;	/* pointer to mounted on inode */
		struct cdnode *m_cdnodp;/* pointer to mounted on cdnode */
	} m_nodes;
	struct	inode *m_qinod;	/* QUOTA: pointer to quota file */
	int	m_flag;		/* status flags; see below */
	unsigned short	m_qflags;	/* QUOTA: filesystem flags */
	unsigned long	m_btimelimit;	/* QUOTA: block time limit */
	unsigned long	m_ftimelimit;	/* QUOTA: file time limit */
	struct vnode	*m_rootvp;	/* for JAWS performance */
	unsigned int    m_ref;          /* If non zero file system should not
					 * be unmounted. */
	char            **m_iused;	/* private inode use maps */
   };

#  define m_inodp	m_nodes.m_inodp
#  define m_cdnodp	m_nodes.m_cdnodp

   /* m_flag values */
#  define MAVAIL	0x0	/* empty */
#  define MINUSE	0x1	/* holds a valid entry */
#  define MINTER	0x2	/* an intermediate state */
#  define MUNMNT	0x4	/* currently being unmounted */
#  define M_IS_SYNC	0x10	/* synchronous I/O is needed */
#  define M_WAS_SYNC	0x20	/* synchronous I/O was needed in last interval*/
#  define M_NOTIFYING	0x40	/* notifying other sites about synchrony */
#  define M_RMTMDIR	0x80	/* mount'd on a remote inode		 */
#  define M_FLOATING	0x100	/* mount'd inode on dead site		 */
#  define M_WANTED      0x200   /* umount command wants to remove file system */

   /* The mount hash table */
#  ifdef HPUXBOOT
   /*
    * The boot kernel needs only a few devices to mount and can live without
    * hashing.
    */
#    define MNTHASHSZ	1	/* number of entries */
#    define MNTHASHMASK	0	/* bits for MNTHASHSZ */
#  else /* not HPUXBOOT */
#    define MNTHASHSZ	32	/* number of entries */
#    define MNTHASHMASK	0x1f	/* bits for MNTHASHSZ */
#  endif /* not HPUXBOOT */

#  define MOUNTHASH(dev) \
	((struct mount *)&mounthash[((dev)+((dev)>>8)+((dev)>>16)) & \
			MNTHASHMASK])

   /* mounthead structures used at the head of the hashed mount entries */

   struct mounthead {
	struct mount *m_hforw;	/* forward hash pointer */
	struct mount *m_hback;  /* backward hash pointer */
	struct mount *m_rhforw; /* forward hash pointer for real device */
	struct mount *m_rhback; /* backward hash pointer for real device */
   };

   /*
    * Convert vfs ptr to mount ptr. ONLY WORKS IF m_vfs IS FIRST.
    */
   /*
    *Note:  The above assumption is no longer true.  However, this
    *macro is only associated with quotas, which will need rewriting
    *anyway --jdt
    */
#    define VFSTOM(VFSP)  ((struct mount *)(VFSP->vfs_data))

   /*
    * mount table
    */
   extern struct mounthead mounthash[MNTHASHSZ];

   /*
    * Operations
    */
   struct mount *getmp();
   struct mount *getrmp();
   extern struct mount *getmount();

   /*status of mount table*/
#  define M_UNLOCKED	0	/*  Mount table being locked  */
#  define M_BOOTING	1	/*  Site is booting  */
#  define M_MODIFY	2	/*  Mount table being modified  */
#  define M_COPY	3	/*  Mount table being copied  */

   /* lock function */
#  define M_LOCK	   0	/* Lock mount table  */
#  define M_RECOVERY_LOCK  1	/* must be 1 for protocol compat. */
#  define M_TLOCK	   2	/* not used (yet) */
#  define M_PREVENT_LOCK   3	/* only used by serve_clusterreq() */
#  define M_ALLOW_LOCK	   4	/* only used by serve_clusterreq() */

   /*
    * mount filesystem type switch table
    */
   extern struct vfsops *vfssw[];

#  endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_MOUNT_INCLUDED */
